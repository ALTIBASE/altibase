/***********************************************************************
 * Copyright 1999-2013 ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFitManager.cpp 87881 2020-06-29 08:04:01Z kclee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idp.h>
#include <iduLatch.h>
#include <idErrorCode.h>
#include <iduFitManager.h>

#if defined(ALTIBASE_FIT_CHECK)

idBool iduFitManager::mIsInitialized = ID_FALSE;
idBool iduFitManager::mIsPluginLoaded = ID_FALSE;
iduFitStatus iduFitManager::mIsFitEnabled = IDU_FIT_NONE;
UInt iduFitManager::mInitFailureCount = 0;

/* ServerID 및 sleepNode 관리 리스트 */
SChar iduFitManager::mSID[IDU_FIT_SID_MAX_LEN + 1];
iduSleepList iduFitManager::mSleepList;

void *iduFitManager::mShmPtr = NULL;
volatile SInt *iduFitManager::mShmBuffer = NULL;

/* FIT 서버랑 통신용 함수 및 동적 라이브러리 핸들 */
fitTransmission iduFitManager::mTransmission = NULL;
PDL_SHLIB_HANDLE iduFitManager::mFitHandle = PDL_SHLIB_INVALID_HANDLE;

/* iduFitManager 초기화 함수 */
IDE_RC iduFitManager::initialize()
{
    /* FIT_ENABLE = 1 이면 loading 하고, 0 이면 하지 않는다. */
    IDE_TEST( iduFitManager::getFitEnable() == IDU_FIT_FALSE );

    /* iduFitManager 초기화 상태를 확인 */
    if ( mIsInitialized == ID_TRUE )
    {
        IDE_TEST( iduFitManager::finalize() != IDE_SUCCESS );
    }
    else
    {
        /* DO NOTHING */
    }

    /* 서버 식별자 파싱하기 */
    IDE_TEST_RAISE( iduFitManager::parseSID() != IDE_SUCCESS, ERR_FIT_INITIALIZATION );

    /* 메모리 할당 */
    IDE_TEST_RAISE( initSleepList(&iduFitManager::mSleepList) != IDE_SUCCESS, ERR_FIT_INITIALIZATION );

    /* 동적 라이브러리 fitPlugin 로딩 */
    IDE_TEST_RAISE( loadFitPlugin() != IDE_SUCCESS, ERR_FIT_INITIALIZATION );

    /* Shared Memory 는 성능을 위한 목적이므로 Failure 가 발생하더라도 무시한다 */
    (void)attachSharedMemory();

    /* fitPluging 로딩 상태 값을 변경 */
    mIsPluginLoaded = ID_TRUE;

    /* iduFitManager 상태 값을 변경 */
    mIsInitialized = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FIT_INITIALIZATION )
    {
        /* 실패 하더라도 log 만 남기고 계속 진행한다. */
        IDU_FIT_LOG( "FIT Initialization Failure." );

        mInitFailureCount++;

        /* Failure 이 IDU_FIT_INIT_RETRY_MAX 이상으로 반복될 경우FIT_ENABLE 을 0 으로 변경한다. */
        if ( mInitFailureCount > IDU_FIT_INIT_RETRY_MAX ) 
        {
            mIsFitEnabled = IDU_FIT_FALSE;
            IDU_FIT_LOG( "Set value of FIT_ENABLE to 0" );
        }
        else 
        {
            /* Do nothing */
        }

    }

    IDE_EXCEPTION_END;
    
    return IDE_SUCCESS; 
}

/* iduFitManager 소멸 함수 */
IDE_RC iduFitManager::finalize()
{
    UInt sState = 1;

    /* fitPluging 로딩을 해제 */
    IDE_TEST_RAISE( iduFitManager::unloadFitPlugin() != IDE_SUCCESS, ERR_FIT_FINALIZATION );

    /* sleepList를 해제하기 */
    IDE_TEST_RAISE( finalSleepList(&iduFitManager::mSleepList) != IDE_SUCCESS, ERR_FIT_FINALIZATION );

    if ( ( mShmPtr != NULL ) && ( mShmPtr != (void *)-1 ) )
    {
        (void)idlOS::shmdt( mShmPtr );
        mShmBuffer = NULL;
        mShmPtr = NULL;
    }
    else
    {
        /* Do Nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FIT_FINALIZATION )
    {
        /* 실패 하더라도 log 만 남기고 계속 진행한다. */
        IDU_FIT_LOG( "FIT Finalization Failure." );
    }

    IDE_EXCEPTION_END;
    
    switch ( sState )
    {
        case 1:
            (void)finalSleepList(&iduFitManager::mSleepList);
            /* fall through */
        default:
        {
            break;
        }
    }

    return IDE_SUCCESS;
}

/* ErrorCode 세팅함수 */
void iduFitManager::setErrorCode( const SChar *aFormat, ... )
{
    UInt sErrorCode = 0;
    va_list sArgsList;
    
    va_start(sArgsList, aFormat);

    sErrorCode = va_arg(sArgsList, UInt);

    IDE_SET( ideFitSetErrorCode(sErrorCode, sArgsList) );

    va_end(sArgsList);
}

void iduFitManager::setLogInfo( const SChar *aFileName, 
                                UInt aCodeLine,
                                iduFitAction *aFitAction )
{
    /* 파일 이름을 저장 */
    idlOS::strncpy( aFitAction->mFileName, 
                    aFileName, 
                    IDU_FIT_NAME_MAX_LEN );

    /* 라인을 저장 */
    aFitAction->mCodeLine = aCodeLine;
}

/* $ALTIBASE_HOME 경로로 SID 데이터를 파싱 */
IDE_RC iduFitManager::parseSID()
{
    SChar *sEnvPath = NULL;
    SChar sHomePath[IDU_FIT_PATH_MAX_LEN + 1] = { 0, };

    SChar *sStrPtr = NULL;
    SChar *sSidPtr = NULL;

    /* 식별자 SID 구성하기 */
    sEnvPath = idlOS::getenv(IDP_HOME_ENV);
    IDE_TEST( sEnvPath == NULL );

    idlOS::strncpy(sHomePath, sEnvPath, IDU_FIT_PATH_MAX_LEN);
    sHomePath[IDU_FIT_PATH_MAX_LEN] = IDE_FIT_STRING_TERM;

    /* HDBHomePath를 자르기. 예를 들자면 
       /home/user/work/altidev4/altibase_home 
       문자열을 자르면 altibase_home  */
    sStrPtr = idlOS::strtok(sHomePath, IDL_FILE_SEPARATORS);
    
    /* 더 이상 구할 문자열이 없을 때까지 반복 */
    while ( sStrPtr != NULL )
    {
        /* 다음번 자르기 위해서 위치 저장 */
        sSidPtr = sStrPtr;
        sStrPtr = idlOS::strtok(NULL, IDL_FILE_SEPARATORS);
    }

    /* SID 복사 */
    idlOS::strncpy(iduFitManager::mSID, sSidPtr, IDU_FIT_SID_MAX_LEN);
    iduFitManager::mSID[IDU_FIT_SID_MAX_LEN] = IDE_FIT_STRING_TERM;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFitManager::requestFitAction( const SChar *aUID, iduFitAction *aFitAction )
{

    SChar sSendData[IDU_FIT_DATA_MAX_LEN + 1] = { 0, };
    SChar sRecvData[IDU_FIT_DATA_MAX_LEN + 1] = { 0, };

    /* FIT NODE 에 아무것도 없으면 통신하지 않는다 */
    if ( ( mShmBuffer != NULL ) && ( mShmBuffer != (void *)-1 ) ) 
    {
        IDE_TEST( *mShmBuffer != IDU_FIT_POINT_EXIST );
    }
    else
    {
        /* Do Nothing */
    }

    IDE_TEST( aUID == NULL );
    
    if ( mIsInitialized == ID_FALSE )
    {
        IDE_TEST( iduFitManager::initialize() != IDE_SUCCESS );
    }
    else
    {
        /* DO NOTHING */
    }

    IDE_TEST( mFitHandle == PDL_SHLIB_INVALID_HANDLE );

    /* UID 및 SID Append 처리 */
    idlOS::strncpy(sSendData, aUID, ID_SIZEOF(sSendData));
    sSendData[IDU_FIT_DATA_MAX_LEN] = IDE_FIT_STRING_TERM;

    idlOS::strncat( sSendData, IDU_FIT_VERTICAL_BAR, IDU_FIT_DATA_MAX_LEN );
    idlOS::strncat( sSendData, iduFitManager::mSID, IDU_FIT_DATA_MAX_LEN );

    /* FIT 서버와 송수신 하기 */
    IDE_TEST( mTransmission( (SChar *)sSendData, IDU_FIT_DATA_MAX_LEN,
                             (SChar *)sRecvData, IDU_FIT_DATA_MAX_LEN ) != 0 );

    /* 수신 받던 결과에 대한 파싱 */
    IDE_TEST( iduFitManager::parseFitAction(sRecvData, aFitAction) != IDE_SUCCESS );

    /* 아무런 Action 이 필요하지 않을경우 */
    IDE_TEST( aFitAction->mProtocol != IDU_FIT_DO_ACTION );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 수신 정보를 분석하고 FitAction을 초기화 */
IDE_RC iduFitManager::parseFitAction( SChar *aRecvData, iduFitAction *aFitAction )
{
    SInt sValue = 0;
    SInt sStrLen = 0;

    SLong sTimeout = 0;
    SLong sProtocol = 0;
    SLong sActionType = 0;

    SChar *sStrPtr = NULL;
    SChar *sLeftPtr = NULL;
    SChar *sRightPtr = NULL;
    SChar *sSavePtr = NULL;

    IDE_TEST( aRecvData == NULL );

    /* aRecvData 문자열에서 '{' 시작하는 위치를 구함 */
    sLeftPtr = idlOS::strstr(aRecvData, IDE_FIT_LEFT_BRACE);
    IDE_TEST( sLeftPtr == NULL );
    
    /* aRecvData 문자열이 '{' 있는지 확인 */
    IDE_TEST( idlOS::strncmp(sLeftPtr, IDE_FIT_LEFT_BRACE, 1) != 0 );
    
    /* aRecvData 문자열에서 '}' 시작하는 위치를 구함 */
    sRightPtr = idlOS::strstr(aRecvData, IDE_FIT_RIGHT_BRACE);
    IDE_TEST( sRightPtr == NULL);
    
    /* aRecvData 문자열이 '}' 있는지 확인 */
    IDE_TEST( idlOS::strncmp(sRightPtr, IDE_FIT_RIGHT_BRACE, 1) != 0 );
    
    /* 문자열이 파싱 */
    sStrPtr = idlOS::strtok_r(aRecvData, IDE_FIT_LEFT_BRACE, &sSavePtr);
    sStrPtr = idlOS::strtok_r(sStrPtr, IDE_FIT_RIGHT_BRACE, &sSavePtr);

    while ( 1 )
    {
        sStrPtr = idlOS::strtok_r(sStrPtr, IDU_FIT_VERTICAL_BAR, &sSavePtr);
        if ( NULL == sStrPtr )
        {
            break;
        }
        else
        {
            /* DO NOTHING */
        }

        switch ( sValue )
        {
            case 0:
            {
                /* Protocol 코드 파싱 */
                sStrLen = idlOS::strlen(sStrPtr);
                IDE_TEST(idlVA::strnToSLong(sStrPtr, sStrLen, &sProtocol, NULL) != 0);

                /* Protocol 설정 */
                aFitAction->mProtocol = (iduFitProtocol)sProtocol;

                break;
            }
            case 1:
            {
                /* ActionType 파싱 */
                sStrLen = idlOS::strlen(sStrPtr);
                IDE_TEST(idlVA::strnToSLong(sStrPtr, sStrLen, &sActionType, NULL) != 0);

                /* ActionType 설정 */
                aFitAction->mType = (iduFitActionType)sActionType;

                break;
            }
            case 2:
            {
                /* TID 파싱 */
                idlOS::strncpy(aFitAction->mTID, sStrPtr, ID_SIZEOF(aFitAction->mTID));
                aFitAction->mTID[IDU_FIT_TID_MAX_LEN] = IDE_FIT_STRING_TERM;

                break;
            }
            case 3:
            {
                /* Timeout 시간 파싱 */
                sStrLen = idlOS::strlen(sStrPtr);
                IDE_TEST(idlVA::strnToSLong(sStrPtr, sStrLen, &sTimeout, NULL) != 0);

                /* Timeout 설정 */
                aFitAction->mTimeout = (UInt)sTimeout;

                break;
            }
            default:
            {
                break;
            }
        }

        sValue++;
        sStrPtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDU_FIT_DEBUG_LOG( "FIT ParseFitAction Failure." );

    return IDE_FAILURE;
}

/* 주어진 fitAction의 유형을 따라서 기능 실행하는 함수 */
IDE_RC iduFitManager::validateFitAction( iduFitAction *aFitAction )
{

    switch ( aFitAction->mType )
    {
        case IDU_FIT_ACTION_HIT:
        {
            IDE_TEST( hitServer(aFitAction) != IDE_SUCCESS );
            break;
        }
        case IDU_FIT_ACTION_JUMP:
        {
            IDE_TEST( abortServer(aFitAction) != IDE_SUCCESS );
            break;
        }
        case IDU_FIT_ACTION_KILL:
        {
            IDE_TEST( killServer(aFitAction) != IDE_SUCCESS );
            break;
        }
        case IDU_FIT_ACTION_SLEEP:
        {
            IDE_TEST( sleepServer(aFitAction) != IDE_SUCCESS );
            break;
        }
        case IDU_FIT_ACTION_TIME_SLEEP:
        {
            IDE_TEST( timeSleepServer(aFitAction) != IDE_SUCCESS );
            break;
        }
        case IDU_FIT_ACTION_WAKEUP:
        {
            IDE_TEST( wakeupServerBySignal(aFitAction) != IDE_SUCCESS );
            break;
        }
        case IDU_FIT_ACTION_WAKEUP_ALL:
        {
            IDE_TEST( wakeupServerByBroadcast(aFitAction) != IDE_SUCCESS );
            break;
        }
        case IDU_FIT_ACTION_SIGSEGV:
        {
            IDE_TEST( sigsegvServer( aFitAction ) != IDE_SUCCESS );
            break;
        }
        default:
        {
           IDE_TEST( 0 );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDU_FIT_DEBUG_LOG( "FIT validateFitAction Failure." );

    return IDE_FAILURE;
}

/* 통신용 라이브러리를 로딩하는 함수 */
IDE_RC iduFitManager::loadFitPlugin()
{
    SChar *sEnvPath = NULL;
    SChar sPluginPath[IDU_FIT_PATH_MAX_LEN + 1] = { 0, };

    /* $ATAF_HOME 경로를 얻어오기 */
    sEnvPath = idlOS::getenv(IDU_FIT_ATAF_HOME_ENV);
    IDE_TEST( sEnvPath == NULL );

#if defined(VC_WIN32)
    /* coming soon */
    return IDE_FAILURE;

#else
    idlOS::snprintf( sPluginPath,
                     IDU_FIT_PATH_MAX_LEN,
                     "%s%s%s%s%s%s",
                     sEnvPath,
                     IDL_FILE_SEPARATORS,
                     "lib",
                     IDL_FILE_SEPARATORS,
                     IDU_FIT_PLUGIN_NAME,
                     ".so" );

#endif

    /* 라이브러리를 열기 */
    mFitHandle = idlOS::dlopen(sPluginPath, RTLD_LAZY | RTLD_LOCAL);
    IDE_TEST_RAISE( mFitHandle == PDL_SHLIB_INVALID_HANDLE, ERR_INVALID_HANDLE );

    IDE_TEST( loadFitFunctions() != IDE_SUCCESS );

    mIsPluginLoaded = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_HANDLE )
    {
         IDU_FIT_LOG( "FIT Plugin Load Failure." );

    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC iduFitManager::unloadFitPlugin()
{
    /* If the FitPlugin Handle avaliable */
    if ( iduFitManager::mFitHandle != PDL_SHLIB_INVALID_HANDLE )
    {
        IDE_TEST( idlOS::dlclose(iduFitManager::mFitHandle) != IDE_SUCCESS );

        /* Unload Success */
        iduFitManager::mFitHandle = PDL_SHLIB_INVALID_HANDLE;
    }
    else
    {
        /* DO NOTHING */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        iduFitManager::mFitHandle = PDL_SHLIB_INVALID_HANDLE;
    }

    return IDE_FAILURE;
}

IDE_RC iduFitManager::loadFitFunctions()
{

    mTransmission = (fitTransmission)idlOS::dlsym(mFitHandle,IDU_FIT_PLUGIN_TRANSMISSION);
    
    IDE_TEST( mTransmission == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC iduFitManager::initSleepList( iduSleepList *aSleepList )
{
    /* Setting double-linked list */
    aSleepList->mHead.mPrev = NULL;
    aSleepList->mTail.mNext = NULL;

    aSleepList->mHead.mNext = &(aSleepList->mTail);
    aSleepList->mTail.mPrev = &(aSleepList->mHead);

    /* Setting preporties */
    aSleepList->mCount = 0;

    /* Mutex 초기화 */
    IDE_TEST_RAISE( aSleepList->mMutex.initialize((SChar *) "FIT_SLEEPLIST_MUTEX",
                                                  IDU_MUTEX_KIND_POSIX,
                                                  IDV_WAIT_INDEX_NULL)
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT )
    {
        /* Mutex 초기화 실패로 인한 에러 */
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFitManager::finalSleepList( iduSleepList *aSleepList )
{
    UInt sState = 0;

    /* Clear the sleep list */
    IDE_TEST( resetSleepList(aSleepList) != IDE_SUCCESS );
    sState = 1;

    /* sleepList 용 Mutex 해제 */
    IDE_TEST_RAISE( aSleepList->mMutex.destroy() != IDE_SUCCESS, ERR_MUTEX_DESTROY );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_DESTROY )
    {
        /* Mutex 해제 실패로 인한 에러 */
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexDestroy));
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            (void)aSleepList->mMutex.destroy();
            /* fall through */
        default:
        {
            break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC iduFitManager::resetSleepList( iduSleepList *aSleepList )
{
    iduSleepNode *sDestNode = NULL;
    iduSleepNode *sNextNode = NULL;
    iduSleepNode *sTailNode = NULL;

    sTailNode = &(aSleepList->mTail);

    for ( sDestNode = aSleepList->mHead.mNext;
          sDestNode != sTailNode;
          sDestNode = sNextNode )
    {
        sNextNode = sDestNode->mNext;
        
        IDE_TEST( deleteSleepNode(aSleepList, sDestNode) != IDE_SUCCESS );

        (void)freeSleepNode(sDestNode);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC iduFitManager::createSleepNode( iduSleepNode **aSleepNode,
                                       iduFitAction *aFitAction )
{
    UInt sState = 0;
    SInt sTIDLength = 0;

    iduSleepNode *sSleepNode = NULL;

    IDE_TEST( aSleepNode == NULL );

    /* 메모리 할당 */
    sSleepNode = (iduSleepNode *) iduMemMgr::mallocRaw(ID_SIZEOF(iduSleepNode));
    IDE_TEST( sSleepNode == NULL );
    sState = 1;
    
    /* TID 복사하기 */
    idlOS::strncpy( sSleepNode->mAction.mTID, 
                    aFitAction->mTID, 
                    IDU_FIT_TID_MAX_LEN );

    sTIDLength = idlOS::strlen(aFitAction->mTID);
    sSleepNode->mAction.mTID[sTIDLength] = IDE_FIT_STRING_TERM;

    /* ActionType 복사하기 */
    sSleepNode->mAction.mType = aFitAction->mType;

    /* Time 복사하기 */
    sSleepNode->mAction.mTimeout = aFitAction->mTimeout;

    /* Mutex 초기화 */
    IDE_TEST_RAISE( sSleepNode->mMutex.initialize((SChar *)"FIT_SLEEPNODE_MUTEX",
                                                  IDU_MUTEX_KIND_POSIX,
                                                  IDV_WAIT_INDEX_NULL)
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    sState = 2;

    /* Condition 초기화 */
    IDE_TEST_RAISE( sSleepNode->mCond.initialize((SChar *)"FIT_SLEEPNODE_COND")
                    != IDE_SUCCESS, ERR_COND_INIT );

    *aSleepNode = sSleepNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT )
    {
        /* Mutex 초기화 실패로 인한 에러 */
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION( ERR_COND_INIT )
    {
        /* Condition 초기화 실패로 인한 에러 */
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            (void)sSleepNode->mMutex.destroy();
            /* fall through */
        case 1:
            (void)iduMemMgr::freeRaw(sSleepNode);
            /* fall through */
        default:
        {
            break;
        }
    }
    
    return IDE_FAILURE;
}

IDE_RC iduFitManager::addSleepNode( iduSleepList *aSleepList,
                                    iduSleepNode *aSleepNode )
{
    IDE_TEST( (aSleepList == NULL) || (aSleepNode == NULL) );

    aSleepNode->mNext = NULL;
    aSleepNode->mPrev = NULL;

    IDE_TEST( aSleepList->mMutex.lock(NULL) != IDE_SUCCESS );
    
    /* Append node at list head */
    aSleepNode->mPrev = &(aSleepList->mHead);
    aSleepNode->mNext = aSleepList->mHead.mNext;

    aSleepList->mHead.mNext->mPrev = aSleepNode;
    aSleepList->mHead.mNext = aSleepNode;

    /* Increase list count */
    aSleepList->mCount++;

    IDE_TEST( aSleepList->mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFitManager::deleteSleepNode( iduSleepList *aSleepList,
                                       iduSleepNode *aSleepNode )
{
    IDE_TEST( (aSleepList == NULL) || (aSleepNode == NULL) );

    IDE_TEST( aSleepList->mMutex.lock(NULL) != IDE_SUCCESS );

    IDE_TEST( (aSleepNode == &(aSleepList->mHead)) ||
              (aSleepNode == &(aSleepList->mTail)) );

    /* Remove from list */
    aSleepNode->mPrev->mNext = aSleepNode->mNext;
    aSleepNode->mNext->mPrev = aSleepNode->mPrev;

    aSleepNode->mNext = NULL;
    aSleepNode->mPrev = NULL;

    /* Decrease list count */
    aSleepList->mCount--;

    IDE_TEST( aSleepList->mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFitManager::freeSleepNode( iduSleepNode *aSleepNode )
{
    IDE_TEST( aSleepNode == NULL );

    IDE_TEST_RAISE( aSleepNode->mCond.destroy() != IDE_SUCCESS,
                    ERR_COND_DESTROY );

    IDE_TEST_RAISE( aSleepNode->mMutex.destroy() != IDE_SUCCESS,
                    ERR_MUTEX_DESTROY );

    /* 메모리 해제 */
    (void)iduMemMgr::freeRaw(aSleepNode);

    /* 포인터를 NULL로 설정 */
    aSleepNode = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_DESTROY )
    {
        /* Condition 해제 실패로 인한 에러 */
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION( ERR_MUTEX_DESTROY )
    {
        /* Mutex 해제 실패로 인한 에러 */
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFitManager::getSleepNode( const SChar *aUID, 
                                    iduSleepNode **aSleepNode )
{
    iduSleepNode *sDestNode = NULL;
    iduSleepNode *sTailNode = NULL;

    *aSleepNode = NULL;

    IDE_TEST( (aUID == NULL) || (aSleepNode == NULL) );

    sTailNode = &(iduFitManager::mSleepList.mTail);

    for ( sDestNode = iduFitManager::mSleepList.mHead.mNext;
          sDestNode != sTailNode;
          sDestNode = sDestNode->mNext )
    {
        /* UID 및 TID 이름이 같는지 확인 */
        if ( 0 == idlOS::strncmp(aUID, sDestNode->mAction.mTID, IDU_FIT_TID_MAX_LEN) )
        {
            *aSleepNode = sDestNode;
            break;
        }
        else
        {
            /* Continue */
        }
    }

    /* 없으면 실패 처리 */
    IDE_TEST( *aSleepNode == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFitManager::hitServer( iduFitAction *aFitAction )
{
    /* BUG-39414 */
    ideLogEntry sLog(IDE_FIT_0);

    sLog.append( "HDB server was hitted.\n" );
    sLog.appendFormat( "[%s:%"ID_UINT32_FMT":\"%s\"]\n",
                        aFitAction->mFileName,
                        aFitAction->mCodeLine,
                        aFitAction->mTID );
    sLog.write();

    return IDE_SUCCESS;
}

IDE_RC iduFitManager::abortServer( iduFitAction *aFitAction )
{
    /* BUG-39414 */
    ideLogEntry sLog(IDE_FIT_0);

    sLog.setTailless(ACP_TRUE);
    sLog.append( "HDB server was aborted.\n" );
    sLog.appendFormat( "[%s:%"ID_UINT32_FMT":\"%s\"]\n",
                        aFitAction->mFileName,
                        aFitAction->mCodeLine,
                        aFitAction->mTID );
    sLog.append( "IDU_FIT_POINT_JUMP\n" );
    sLog.write();

    return IDE_SUCCESS;
}

IDE_RC iduFitManager::killServer( iduFitAction *aFitAction )
{
    /* BUG-39414 */
    ideLogEntry sLog(IDE_FIT_0);

    /* 로그 파일 끝에 이동 */
    sLog.setTailless(ACP_TRUE);
    sLog.append( "HDB server was terminated.\n" );
    sLog.appendFormat( "[%s:%"ID_UINT32_FMT":\"%s\"]\n",
                        aFitAction->mFileName,
                        aFitAction->mCodeLine,
                        aFitAction->mTID );
    sLog.append( "IDU_FIT_POINT_KILL\n" );
    sLog.write();

    /* Flush all logs */
    ideLog::flushAllModuleLogs();

    /* Need not close file descriptor */
    idlOS::exit(-1);

    return IDE_SUCCESS;
}

IDE_RC iduFitManager::sleepServer( iduFitAction *aFitAction )
{
    IDE_RC sRC = IDE_FAILURE;

    UInt sState = 0;
    iduSleepNode *sSleepNode = NULL;

    /* 메모리 할당 */
    sRC = createSleepNode(&sSleepNode, aFitAction);
    IDE_TEST( sRC != IDE_SUCCESS );
    sState = 1;

    /* 락을 잡고 노드를 추가 */
    sRC = addSleepNode(&iduFitManager::mSleepList, sSleepNode);
    IDE_TEST( sRC != IDE_SUCCESS );
    sState = 2;

    /* SLEEP 동작하기 위한 노드 락 잡기 */
    IDE_TEST( sSleepNode->mMutex.lock(NULL) != IDE_SUCCESS );

    /* SLEEP 로그를 기록하기 */
    ideLog::log ( IDE_FIT_0, 
                  "HDB server is sleeping.\n[%s:%"ID_UINT32_FMT":\"%s\":INFINATE msec]",
                  aFitAction->mFileName,
                  aFitAction->mCodeLine,
                  aFitAction->mTID );

    /* 시그널을 발생할때까지 멈춤 */
    IDE_TEST_RAISE( sSleepNode->mCond.wait(&(sSleepNode->mMutex))
                    != IDE_SUCCESS, ERR_COND_WAIT );

    /* WAKEUP 로그를 기록하기 */
    ideLog::log ( IDE_FIT_0, 
                  "HDB server received a wakeup signal.\n[%s:%"ID_UINT32_FMT":\"%s\"]\n",
                  aFitAction->mFileName,
                  aFitAction->mCodeLine,
                  aFitAction->mTID );


    /* SLEEP 해제하기 위한 노드락 풀기 */
    IDE_TEST( sSleepNode->mMutex.unlock() != IDE_SUCCESS );

    /* 락을 잡고 노드를 삭제 */
    sRC = deleteSleepNode(&iduFitManager::mSleepList, sSleepNode);
    IDE_TEST( sRC != IDE_SUCCESS );
    sState = 1;

    /* 메모리 해제 */
    sRC = freeSleepNode(sSleepNode);
    IDE_TEST( sRC != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_WAIT )
    {
        /* Condition Wait 실패로 인한 서버 FATAL */
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            (void)deleteSleepNode(&iduFitManager::mSleepList, sSleepNode);
            /* fall through */
        case 1:
            (void)freeSleepNode(sSleepNode);
            /* fall through */
        default:
        {
            break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC iduFitManager::timeSleepServer( iduFitAction *aFitAction )
{
    IDE_RC sRC = IDE_FAILURE;

    UInt sState = 0;
    UInt sUSecond = 0;
    iduSleepNode *sSleepNode = NULL;

    IDE_TEST( aFitAction->mTimeout == 0 );

    /* 메모리 할당 */
    sRC = createSleepNode(&sSleepNode, aFitAction);
    IDE_TEST( sRC != IDE_SUCCESS );
    sState = 1;

    /* 락을 잡고 노드를 추가 */
    sRC = addSleepNode(&iduFitManager::mSleepList, sSleepNode);
    IDE_TEST( sRC != IDE_SUCCESS );
    sState = 2;

    /* TIME SLEEP 동작하기 위한 노드락 잡기 */
    IDE_TEST( sSleepNode->mMutex.lock(NULL) != IDE_SUCCESS );

    /* TIME SLEEP 로그를 기록하기 */
    ideLog::log ( IDE_FIT_0,
                  "HDB server is sleeping.\n[%s:%"ID_UINT32_FMT":\"%s\":%"ID_UINT32_FMT" msec]",
                  aFitAction->mFileName,
                  aFitAction->mCodeLine,
                  aFitAction->mTID,
                  aFitAction->mTimeout );

    /* TIME 설정하기 */
    sUSecond = aFitAction->mTimeout;
    sSleepNode->mTV.set((idlOS::time(NULL) + (sUSecond / 1000)), 0);

    /* TIMEOUT 발생하거나 시그널을 발생할때까지 멈춤 */
    IDE_TEST_RAISE( sSleepNode->mCond.timedwait(&(sSleepNode->mMutex),
                                                &(sSleepNode->mTV),
                                                ID_TRUE)
                    != IDE_SUCCESS, ERR_COND_TIME_WAIT );

    /* WAKEUP 로그를 기록하기 */
    ideLog::log ( IDE_FIT_0,
                  "HDB server received a wakeup signal.\n[%s:%"ID_UINT32_FMT":\"%s\"]",
                  aFitAction->mFileName,
                  aFitAction->mCodeLine,
                  aFitAction->mTID );

    /* TIME SLEEP 해제하기 위한 노드락 풀기 */
    IDE_TEST( sSleepNode->mMutex.unlock() != IDE_SUCCESS );

    /* 락을 잡고 노드를 삭제 */
    sRC = deleteSleepNode(&iduFitManager::mSleepList, sSleepNode);
    IDE_TEST( sRC != IDE_SUCCESS );
    sState = 1;
    
    /* 메모리 해제 */
    sRC = freeSleepNode(sSleepNode);
    IDE_TEST( sRC != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_TIME_WAIT )
    {
        /* Condition timedWait 실패로 인한 서버 FATAL */
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;
    
    switch ( sState )
    {
        case 2:
            (void)deleteSleepNode(&iduFitManager::mSleepList, sSleepNode);
            /* fall through */
        case 1:
            (void)freeSleepNode(sSleepNode);
            /* fall through */
        default:
        {
            break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC iduFitManager::wakeupServerBySignal( iduFitAction *aFitAction )
{
    IDE_RC sRC = IDE_FAILURE;

    /* BUG-39414 */
    ideLogEntry sLog(IDE_FIT_0);

    iduSleepNode *sDestNode = NULL;

    /* 락을 잡고 노드를 검색하기 */
    sRC = getSleepNode(aFitAction->mTID, &sDestNode);
    IDE_TEST( sRC != IDE_SUCCESS );

    /* WAKEUP 동작하기 위한 노드락 잡기 */
    IDE_TEST( sDestNode->mMutex.lock(NULL) != IDE_SUCCESS );

    /* 시그널을 발송하기 */   
    IDE_TEST_RAISE( sDestNode->mCond.signal() != IDE_SUCCESS, ERR_COND_SIGNAL );

    /* WAKEUP 로그를 기록하기 */
    sLog.append("HDB server sent a wakeup signal.\n");
    sLog.appendFormat( "[%s:%"ID_UINT32_FMT":\"%s\"]\n",
                        aFitAction->mFileName,
                        aFitAction->mCodeLine,
                        aFitAction->mTID );
    sLog.write();

    /* WAKEUP 완료하기 위한 노드락 풀기 */
    IDE_TEST( sDestNode->mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_SIGNAL )
    {
        /* Condition Signal 발송 실패로 인한 서버 FATAL */
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduFitManager::wakeupServerByBroadcast( iduFitAction *aFitAction )
{
    iduSleepNode *sDestNode = NULL;
    iduSleepNode *sTailNode = NULL;

    sTailNode = &(iduFitManager::mSleepList.mTail);
   
    IDE_TEST( iduFitManager::mSleepList.mMutex.lock(NULL) != IDE_SUCCESS );

    for ( sDestNode = iduFitManager::mSleepList.mHead.mNext;
          sDestNode != sTailNode;
          sDestNode = sDestNode->mNext )
    {
        /* 시그널을 발송하기 */
        IDE_TEST_RAISE( sDestNode->mCond.signal() != IDE_SUCCESS, ERR_COND_SIGNAL );

        /* WAKEUP 로그를 기록하기 */
        ideLog::log ( IDE_FIT_0, 
                      "HDB server sent a wakeup signal.\n[%s:%"ID_UINT32_FMT":\"%s\"]",
                      aFitAction->mFileName,
                      aFitAction->mCodeLine,
                      aFitAction->mTID );

    }

    IDE_TEST( iduFitManager::mSleepList.mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_SIGNAL )
    {
        /* Condition Signal 발송 실패로 인한 서버 FATAL */
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC iduFitManager::sigsegvServer( iduFitAction *aFitAction )
{

    ideLogEntry sLog(IDE_FIT_0);

    sLog.setTailless(ACP_TRUE);
    sLog.append( "Server was recevied by SIGSEGV signal\n" );
    sLog.appendFormat( "[%s:%"ID_UINT32_FMT":\"%s\"]\n",
                        aFitAction->mFileName,
                        aFitAction->mCodeLine,
                        aFitAction->mTID );
    sLog.append( "IDU_FIT_POINT_SIGNAL\n" );
    sLog.write();

    ideLog::flushAllModuleLogs();

    return IDE_SUCCESS;
}

/**************************************************************************************** 
 * FITSERVER 에 FITPOINT 가 없을 경우 통신을 할 필요가 없다.
 * 따라서, Shared Memory 를 통해 FITPOINT 있는지 확인하고, 없다면 FITSERVER 와 통신을 하지 않는다. 
 ****************************************************************************************/
IDE_RC iduFitManager::attachSharedMemory()
{
    SInt sShmid = PDL_INVALID_HANDLE;
    key_t sKey;

    IDE_TEST( getSharedMemoryKey( &sKey ) != IDE_SUCCESS );

    if ( sKey != 0 )
    {

        sShmid = idlOS::shmget( (key_t)sKey, sizeof(SInt), 0666|IPC_CREAT );

        IDE_TEST_RAISE( sShmid == PDL_INVALID_HANDLE, ERR_GET_SHM );
        
        mShmPtr = idlOS::shmat( sShmid, (void *)0, 0 );

        if ( mShmPtr == (void *)-1 )
        {
            IDE_RAISE( ERR_ATTACH_SHM );
        }
        else
        {
            /* Do Nothing */
        }

        mShmBuffer = (int *)mShmPtr;

        IDU_FIT_LOG( "Shared Memory Attach Success" );
    }
    else
    {
        /* FIT_SHM_KEY = 0 */
        IDU_FIT_LOG( "Disabled Shared Memory" );
        mShmBuffer = NULL;
        mShmPtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GET_SHM )
    {
        IDU_FIT_DEBUG_LOG( "Failed to shmget()" );
    }

    IDE_EXCEPTION( ERR_ATTACH_SHM )
    {
        IDU_FIT_DEBUG_LOG( "Failed to shmat()" );
    }

    IDE_EXCEPTION_END;

    mShmBuffer = NULL;
    mShmPtr = NULL;

    return IDE_FAILURE;
}

/**************************************************************************************** 
 * FIT_SHM_KEY 가 있다면 그 값을 key 로 사용
 * FIT_SHM_KEY 가 0 이라면 shared memory 사용하지 않음
 * FIT_SHM_KEY 가 없으면 ( FIT_SHM_KEY_DEFAULT * FIT_SHM_KEY_WEIGHT_NUMBER ) 를 key 로 사용
 * FIT_SHM_KEY_DEFAULT 는 ALTIBASE_PORT_NO 가 default 
*****************************************************************************************/
IDE_RC iduFitManager::getSharedMemoryKey( key_t *aKey )
{

    SChar *sFitShmKeyEnv = NULL;
    UInt sFitShmKey = 0;

    SChar *sFitAltibasePortNoEnv = NULL;
    UInt sAltibasePortNo = 0;

    SChar *sFitShmKeyDefaultEnv = NULL;
    UInt sFitShmKeyDefault = 0;

    /* FIT_SHM_KEY 가 있으면 FIT_SHM_KEY 를 key 로 사용 */
    sFitShmKeyEnv = idlOS::getenv( IDU_FIT_SHM_KEY );

    if ( sFitShmKeyEnv != NULL )
    {
        sFitShmKey = idlOS::strtol( sFitShmKeyEnv, NULL, 10 );

        *aKey = (key_t)sFitShmKey;
    }
    else
    {
        /* ALTIBASE_PORT_NO */
        sFitAltibasePortNoEnv = idlOS::getenv( IDU_FIT_ALTIBASE_PORT_NO );

        IDE_TEST_RAISE( sFitAltibasePortNoEnv == NULL, NOT_FOUND_ALTIBASE_PORT_NO );

        sAltibasePortNo = idlOS::strtol( sFitAltibasePortNoEnv, NULL, 10 );

        IDE_TEST_RAISE( sAltibasePortNo == 0, ERR_GET_ALTIBASE_PORT_NO );

        /* FIT_SHM_KEY_DEFAULT */        
        sFitShmKeyDefaultEnv = idlOS::getenv( IDU_FIT_SHM_KEY_DEFAULT );

        /* FIT_SHM_KEY_DEFAULT 가 없으면 ALTIBASE_PORT_NO 를 사용 */
        if ( sFitShmKeyDefaultEnv == NULL ) 
        {
            *aKey = sAltibasePortNo * IDU_FIT_SHM_KEY_WEIGHT_NUMBER;
        }
        else
        {
            /* FIT_SHM_KEY_DEFAULT 가 있으면 FIT_SHM_KEY_DEFAULT 를 사용 */
            sFitShmKeyDefault = idlOS::strtol( sFitShmKeyDefaultEnv, NULL, 10 );

            IDE_TEST_RAISE( sFitShmKeyDefault == 0, ERR_GET_FIT_SHM_KEY_DEFAULT );       
 
            *aKey = sFitShmKeyDefault * IDU_FIT_SHM_KEY_WEIGHT_NUMBER;

        }
    }

          
    return IDE_SUCCESS;

    IDE_EXCEPTION( NOT_FOUND_ALTIBASE_PORT_NO )
    {
        IDU_FIT_DEBUG_LOG( "getSharedMemoryKey : ALTIBASE_PORT_NO Not Found \n" );
    }

    IDE_EXCEPTION( ERR_GET_ALTIBASE_PORT_NO )
    {
        IDU_FIT_DEBUG_LOG( "getSharedMemoryKey : Failed to get ALTIBASE_PORT_NO \n" );
    }

    IDE_EXCEPTION( ERR_GET_FIT_SHM_KEY_DEFAULT ) 
    {
        IDU_FIT_DEBUG_LOG( "getSharedMemoryKey : Failed to get FIT_SHM_KEY_DEFAULT \n" );
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif /* ALTIBASE_FIT_CHECK */
