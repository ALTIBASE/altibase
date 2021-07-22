/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idp.cpp 89621 2020-12-24 05:40:35Z emlee $
 * Description:
 *
 * 이 클래스는 알티베이스의 프로퍼티를 관리하는 주 클래스이다.
 *
 * 일반적으로 사용자는 ::read(), ::update()만을 사용한다.
 * ==============================================================
 *
 * idp 모듈의 전체적인 구조는
 * 프로퍼티를 관리하고, 사용자에게 UI를 제공하는 class idp와
 * 각 프로퍼티의 추상적 기초 API를 선언하고, 유지하는 idpBase 기초 클래스,
 * 그리고, idpBase 를 실제 각 데이타 타입에 대해서 구현한 하위 클래스로
 * 구성된다.
 * 현재 설계된 데이타 타입은 SInt, UInt, SLong, ULong, String 이
 * 있으며,
 * 각 프로퍼티는 속성을 가진다.
 * 즉, 각 프로퍼티는 외부/내부 속성, 읽기전용/쓰기 속성, 단일값/다수값 속성,
 *     값범위 검사허용/검사거부 속성, 데이타 타입 속성 등이 있다.
 *
 * 이 속성은 프로퍼티를 등록할 때 주어지며, 세심하게 설계되어야 한다.
 *
 * 등록법)
 * idpDescResource.cpp에 각 프로퍼티에 대한 등록 리스트가 존재한다.
 * 여기에 등록할 프로퍼티의 이름/속성/타입 등을 적어주면 된다.
 *
 *
 * idpBase --> idpEachType(상속)
 *
 *
 * 에러처리)
 * 프로퍼티 로딩 단계에서는 에러코드 및 에러 데이타가 로딩되기 이전이다.
 * 따라서, 프로퍼티 로딩과정에서 발생한 에러는, 즉 리턴이 IDE_FAILURE일 경우
 * idp::getErrorBuf()를 호출하여, 저장된 에러 메시지를 출력하면 된다.
 *
 *
 *   내부 버퍼로 에러가 설정되는 단계의 함수
 *     idp::initialize()
 *     idp::regist();
 *     idp::insert()
 *     idp::readConf()
 *
 *   시스템 에러 버퍼로 설정되는 단계 함수 (에러 메시지 로딩후에  호출되는 것들)
 *     idp::read()
 *     idp::update()
 *     idp::setupBeforeUpdateCallback()
 *     idp::setupAfterUpdateCallback
 *
 *   BUGBUG - Logging Level 변경 시 DDL을 막을 수 있어야 함.
 *
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idp.h>
#include <idErrorCode.h>

SChar    idp::mErrorBuf[IDP_ERROR_BUF_SIZE];
SChar   *idp::mHomeDir;
SChar   *idp::mConfName;
PDL_thread_mutex_t idp::mMutex;
UInt     idp::mCount;
iduList  idp::mArrBaseList[IDP_MAX_PROP_COUNT];
SChar   *idp::mSID = NULL;

extern IDE_RC registProperties();

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::initialize()
 *
 * Description :
 * idp 객체 초기화하고, Descriptor를 등록한다.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC idp::initialize(SChar *aHomeDir, SChar *aConfName)
{
    SChar *sHomeDir;
    UInt   sHomeDirLen;
    UInt   i;

    // 하위 클래스에서 발생한 에러를 idp 클래스의 버퍼로 받아오기 위함.
    idpBase::initializeStatic(mErrorBuf);

    /* ---------------------
     *  [1] Home Dir 설정
     * -------------------*/
    if (aHomeDir == NULL) // 디폴트 환경변수에서 프로퍼티 로딩
    {
        sHomeDir = idlOS::getenv(IDP_HOME_ENV);
    }
    else // HomeDir이 NULL이 아님
    {
        // BUG-29649 Initialize Global Variables of shmutil
        if(idlOS::strlen(aHomeDir) == 0)
        {
            sHomeDir = idlOS::getenv(IDP_HOME_ENV);
        }
        else
        {
            sHomeDir = aHomeDir;
        }
    }

    // HomeDir이 설정되어있는지 검사한다.
    IDE_TEST_RAISE(sHomeDir == NULL, home_env_error);
    sHomeDirLen = idlOS::strlen(sHomeDir);
    IDE_TEST_RAISE(sHomeDirLen == 0, home_env_error);

    /* BUG-15311:
     * HomeDir 맨 끝에 디렉터리 구분자가 붙어있으면
     * (예: /home/altibase/altibase_home4/)
     * 메타에 저장된 데이터 파일 경로명에서
     * 디렉터리 구분자가 반복되어 나타납니다.
     * (예: /home/altibase/altibase_home4//dbs/user_tbs.dbf)
     * 이러한 데이터 파일을 경로명으로 지정하려 할 경우,
     * 메타에 저장되어있는대로 디렉터리 구분자를 반복해서 적어주지 않으면
     * 데이터 파일을 찾을 수 없다는 오류가 납니다.
     * 이 문제를 해결하기 위해,
     * HomeDir 맨 끝에 디렉터리 구분자가 붙어있으면
     * 이를 제거한 문자열을 HomeDir로 사용하도록 수정합니다. */

    // HomeDir이 디렉터리 구분자로 끝나지 않는 경우
    if ( (sHomeDir[sHomeDirLen - 1] != '/') &&
         (sHomeDir[sHomeDirLen - 1] != '\\') )
    {
        // 환경변수 또는 인자로 받은 HomeDir을 그대로 mHomeDir에 설정한다.
        mHomeDir = sHomeDir;
    }
    // HomeDir이 디렉터리 구분자로 끝나는 경우
    else
    {
        // 맨 끝의 디렉터리 구분자를 제외한 부분의 길이를 구한다.
        while (sHomeDirLen > 0)
        {
            if ( (sHomeDir[sHomeDirLen - 1] != '/') &&
                 (sHomeDir[sHomeDirLen - 1] != '\\') )
            {
                break;
            }
            sHomeDirLen--;
        }

        // to remove false alarms from codesonar test
        IDE_TEST_RAISE( sHomeDirLen >= ID_UINT_MAX -1, too_long_home_env_error);

        // 맨 끝의 디렉터리 구분자를 제외한 HomeDir을 mHomeDir에 복사한다.
        mHomeDir = (SChar *)iduMemMgr::mallocRaw(sHomeDirLen + 1);
        IDE_ASSERT(mHomeDir != NULL);
        idlOS::strncpy(mHomeDir, sHomeDir, sHomeDirLen);
        mHomeDir[sHomeDirLen] = '\0';
    }

    /* ---------------------
     *  [2] Conf File  설정
     * -------------------*/

    if (aConfName == NULL)
    {
        mConfName = IDP_DEFAULT_CONFFILE;
    }
    else
    {
        if (idlOS::strlen(aConfName) == 0)
        {
            mConfName = IDP_DEFAULT_CONFFILE;
        }
        else
        {
            mConfName = aConfName;
        }
    }

    IDE_TEST_RAISE(idlOS::thread_mutex_init(&mMutex) != 0,
                   mutex_error);

    //mArrBaseList는 registProperties 전에 초기화 되어야 함.
    mCount = 0; // counter for mArrBaseList
    for(i = 0; i < IDP_MAX_PROP_COUNT; i++)
    {
        IDU_LIST_INIT(&mArrBaseList[i]);
    }

    // 프로퍼티 Descriptor를 등록한다.
    IDE_TEST(registProperties() != IDE_SUCCESS);

    IDE_TEST(readPFile() != IDE_SUCCESS);

    /*Pfile과 Env로 부터 설정된 SID를 Local Instance의 Property에 반영한다.*/
    IDE_TEST(setLocalSID() != IDE_SUCCESS);

    IDE_TEST(readSPFile() != IDE_SUCCESS);

    /*Source에서 가장 높은 우선순위로 설정된 값을 프로퍼티의 Memory Value로 복제하여 삽입한다.*/
    IDE_TEST(insertMemoryValueByPriority() != IDE_SUCCESS);

    IDE_TEST(verifyMemoryValues() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(too_long_home_env_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp initialize() Error : Too Long ALTIBASE_HOME Environment.(>=%"ID_UINT32_FMT")\n",
                        ID_UINT_MAX - 1);
    }

    IDE_EXCEPTION(home_env_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp initialize() Error : No [%s] Environment Exist.\n", IDP_HOME_ENV);
    }

    IDE_EXCEPTION(mutex_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp initialize() Error : Mutex Initialization Failed.\n");
    }

    IDE_EXCEPTION_END;

    ideSetErrorCodeAndMsg(idERR_ABORT_idp_Initialize_Error,
                          idp::getErrorBuf());
    return IDE_FAILURE;
}



/*-----------------------------------------------------------------------------
 * Name :
 *     idp::destroy()
 *
 * Description :
 * idp 객체 해제한다.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::destroy()
{
    UInt   i;
    UInt   j;

    /* remove property array */
    for (i = 0; i < mCount; i++)
    {
        iduListNode *sIterator = NULL;
        iduListNode *sNodeNext = NULL;

        IDU_LIST_ITERATE_SAFE( &mArrBaseList[i], sIterator, sNodeNext )
        {
            idpBase *sObj = (idpBase *)sIterator->mObj;
            IDU_LIST_REMOVE(sIterator);

            // freeing memory that store value and
            // freeing descriptor should be separated
            
            for (j = 0; j < sObj->mMemVal.mCount; j++)
            {
                iduMemMgr::freeRaw(sObj->mMemVal.mVal[j]);
            }

            iduMemMgr::freeRaw(sObj);
            iduMemMgr::freeRaw(sIterator);
        }
    }
    mCount = 0;

    for(i = 0; i < IDP_MAX_PROP_COUNT; i++)
    {
        IDE_TEST_RAISE(IDU_LIST_IS_EMPTY(&mArrBaseList[i]) != ID_TRUE,
                       prop_error);
    }

    IDE_TEST_RAISE(idlOS::thread_mutex_destroy(&mMutex) != 0,
                   mutex_error);
    idpBase::destroyStatic();

    return IDE_SUCCESS;

    IDE_EXCEPTION(prop_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp destroy() Error : Removing Property failed.\n");
    }

    IDE_EXCEPTION(mutex_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp destroy() Error : Mutex Destroy Failed.\n");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::regist(등록할 Descriptr 객체 포인터)
 *
 * Description :
 * idp Descriptor를 하나씩 등록한다.
 * 실제 입력된 값은 나중에 insert()에 의해 기록되며, 이 단계에서는
 * default 값이 기록된다.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::regist(idpBase *aBase) /* called by each prop*/
{

    idpBase     *sBase;
    iduListNode *sNode;

    IDE_TEST_RAISE(mCount >= IDP_MAX_PROP_COUNT, overflow_error);

    sBase = findBase(aBase->getName());

    IDE_TEST_RAISE(sBase != NULL, already_exist_error);

    sNode = (iduListNode*)iduMemMgr::mallocRaw(ID_SIZEOF(iduListNode));

    IDE_TEST_RAISE(sNode == NULL, memory_alloc_error);

    IDU_LIST_INIT_OBJ(sNode, aBase);
    IDU_LIST_ADD_LAST(&mArrBaseList[mCount], sNode);
    mCount++;

    aBase->registCallback(); // registration callback

    return IDE_SUCCESS;
    IDE_EXCEPTION(overflow_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp regist() Error : "
                        "Overflow of Max Property Count (%"ID_UINT32_FMT").\n",
                        (UInt)IDP_MAX_PROP_COUNT);
    }
    IDE_EXCEPTION(already_exist_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp regist() Error : Property %s is duplicated!!\n",
                        aBase->getName());
    }
    IDE_EXCEPTION(memory_alloc_error)
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp regist() Error : memory allocation error\n");
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
*
* Description :
*  프로퍼티에 설정된 Memory Value의 값에대해서 Cluster Instance인 것과
*  그렇지 않은 Nomal Instance로 구분하여 다른 Verify 함수를 호출한다.
*
**********************************************************************/
IDE_RC idp::verifyMemoryValues()
{
    UInt sClusterDatabase = 0;
    UInt sShardEnable = 0;
    UInt sShardAllowAutoCommit = 0;
    UInt sAutoCommit = 0;
    UInt sGlobalTransactionLevel = 0;

    IDE_TEST(read("CLUSTER_DATABASE", &sClusterDatabase ,0) != IDE_SUCCESS);

    if(sClusterDatabase == 0)
    {
        /*값의 범위 검사 및 다중 값 검사 NORMAL INSTANCE*/
        IDE_TEST(verifyMemoryValue4Normal() != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST_RAISE(sClusterDatabase != 1, err_cluster_database_val_range);

        /*값의 범위 검사 및 다중 값 검사,공유, 유일값,동일값검사  CLUSTER INSTANCE*/
        IDE_TEST(verifyMemoryValue4Cluster() != IDE_SUCCESS);
    }

    /* BUG-48247 */
    IDE_TEST( read("SHARD_ENABLE", &sShardEnable, 0) != IDE_SUCCESS );
    IDE_TEST( read("AUTO_COMMIT", &sAutoCommit, 0) != IDE_SUCCESS );
    IDE_TEST( read("__SHARD_ALLOW_AUTO_COMMIT", &sShardAllowAutoCommit, 0) != IDE_SUCCESS );

    if ( sShardEnable == 1 )
    {
        IDE_TEST_RAISE( (sAutoCommit == 1) && (sShardAllowAutoCommit == 0), err_shard_not_applicable );
    }

    /* BUG-48352 */
    if ( sShardEnable == 0 )
    {
        IDE_TEST( read("GLOBAL_TRANSACTION_LEVEL", &sGlobalTransactionLevel, 0) != IDE_SUCCESS );
        
        IDE_TEST_RAISE( sGlobalTransactionLevel == 3, err_Cannot_set_GlobalTransactionLevel ); 
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cluster_database_val_range)
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp verifyMemoryValues() Error : "
                        "Property [CLUSTER_DATABASE] "
                        "Overflowed the Value Range.(%d~%d)",
                        0, 1);
    }
    IDE_EXCEPTION( err_shard_not_applicable )
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE,     
                        "idp verifyMemoryValues() Error : "
                        "Cannot set AUTO_COMMIT =1 when SHARD_ENABLE = 1.");
    }
    IDE_EXCEPTION( err_Cannot_set_GlobalTransactionLevel )
    {
        idlOS::snprintf(mErrorBuf, 
                        IDP_ERROR_BUF_SIZE,     
                        "idp verifyMemoryValues() Error : "
                        "Cannot set GLOBAL_TRANSACTION_LEVEL = 3 when SHARD_ENABLE = 0.");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
*
* Description :
*  Normal Instance를 띄우기 위해서 자신의 프로퍼티들의 값이 범위내에 있는지 검사한다.
*
**********************************************************************/
IDE_RC idp::verifyMemoryValue4Normal()
{
    UInt         i;
    iduListNode* sNode;
    iduList*     sBaseList;
    idpBase*     sBase;

    /*모든 프로퍼티에 대한 리스트*/
    for (i = 0; i < mCount; i++)
    {
        sBaseList = &mArrBaseList[i];
        /*하나의 프로퍼티 리스트의 각 항목에 대해서*/
        IDU_LIST_ITERATE(sBaseList, sNode)
        {
            sBase = (idpBase*)sNode->mObj;
            IDE_TEST(sBase->verifyInsertedValues() != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
*
* Description :
*  cluster Instance를 띄우기 위해서 프로퍼티들의 공유 속성, 유일/동일값 속성을
*  만족하는지 검사하고, 설정된 값의 범위를 검사한다.
**********************************************************************/
IDE_RC idp::verifyMemoryValue4Cluster()
{
    UInt         i;
    iduListNode* sFstNode;
    iduList*     sBaseList;
    idpBase*     sFstBase;
    idpBase*     sBase;
    iduListNode* sNode;

    /*모든 프로퍼티에 대한 리스트*/
    for(i = 0; i < mCount; i++)
    {
        sBaseList = &mArrBaseList[i];
        sFstNode  = IDU_LIST_GET_FIRST(sBaseList);
        sFstBase  = (idpBase*)sFstNode->mObj;

        /*공유 속성 체크 */
        if(sFstBase->isMustShare() == ID_TRUE)
        {

            IDE_TEST_RAISE((sFstBase->existSPFileValBySID() != ID_TRUE) &&
                           (sFstBase->existSPFileValByAsterisk() != ID_TRUE),
                           err_not_shared);
        }

        /*Identical/Unique 속성 체크*/
        if((sFstBase->getAttr() & IDP_ATTR_IU_MASK) != IDP_ATTR_IU_ANY)
        {
            /*Identical*/
            if((sFstBase->getAttr() & IDP_ATTR_IU_MASK) ==
                IDP_ATTR_IU_IDENTICAL)
            {
                IDE_TEST(verifyIdenticalAttr(sBaseList) != IDE_SUCCESS);
            }
            else /*Unique*/
            {
                IDE_TEST(verifyUniqueAttr(sBaseList) != IDE_SUCCESS);
            }
        }

        /*하나의 프로퍼티 리스트의 각 항목에 대해서*/
        IDU_LIST_ITERATE(sBaseList, sNode)
        {
            sBase = (idpBase*)sNode->mObj;
            IDE_TEST(sBase->verifyInsertedValues() != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_shared);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "verify value error : "
                        "Property [%s] must be shared in SPFILE!\n",
                        sFstBase->getName());
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
*
* Description :
*   리스트 내의 프로퍼티들의 Memory 값이 모두 동일한지 확인한다.
*   aBaseList      - [IN]  특정 프로퍼티에 대해 다수의 SID를 갖는 프로퍼티들의 리스트
*****************************************************************************/
IDE_RC idp::verifyIdenticalAttr(iduList* aBaseList)
{
    iduListNode* sFstNode;
    idpBase*     sFstBase;
    iduListNode* sCmpNode;
    idpBase*     sCmpBase;
    UInt         i;
    void*        sVal1;
    void*        sVal2;

    IDE_ASSERT(IDU_LIST_IS_EMPTY(aBaseList) == ID_FALSE);

    sFstNode = IDU_LIST_GET_FIRST(aBaseList);
    sFstBase = (idpBase*)sFstNode->mObj;

    IDU_LIST_ITERATE_AFTER2LAST(aBaseList, sFstNode, sCmpNode)
    {
        sCmpBase = (idpBase*)sCmpNode->mObj;

        IDE_TEST_RAISE(sFstBase->getValCnt() != sCmpBase->getValCnt(),
                       err_not_identical);

        for(i = 0; i < sFstBase->getValCnt(); i++)
        {
            IDE_TEST(sFstBase->readPtr4Internal(i, &sVal1) != IDE_SUCCESS);
            IDE_TEST(sCmpBase->readPtr4Internal(i, &sVal2) != IDE_SUCCESS);
            IDE_TEST_RAISE(sFstBase->compare(sVal1, sVal2) != 0,
                           err_not_identical);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_identical);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "check value error : "
                        "Property [%s] must be identical on all instances!!\n",
                        sFstBase->getName());
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
*
* Description :
*   리스트 내의 프로퍼티들의 Memory 값이 모두 서로 다른지 확인한다.
*   aBaseList      - [IN]  특정 프로퍼티에 대해 다수의 SID를 갖는 프로퍼티들의 리스트
*****************************************************************************/
IDE_RC idp::verifyUniqueAttr(iduList* aBaseList)
{
    idpBase*     sCmpBase1;
    idpBase*     sCmpBase2;
    iduListNode* sCmpNode1;
    iduListNode* sCmpNode2;
    void*        sVal1;
    void*        sVal2;
    UInt         i, j;

    IDE_ASSERT(IDU_LIST_IS_EMPTY(aBaseList) == ID_FALSE);

    IDU_LIST_ITERATE(aBaseList, sCmpNode1)
    {
        sCmpBase1 = (idpBase*)sCmpNode1->mObj;

        IDU_LIST_ITERATE_AFTER2LAST(aBaseList, sCmpNode1, sCmpNode2)
        {
            sCmpBase2 = (idpBase*)sCmpNode2->mObj;

            for(i = 0; i < sCmpBase1->getValCnt(); i++)
            {
                IDE_TEST(sCmpBase1->readPtr4Internal(i, &sVal1)
                        != IDE_SUCCESS);

                for(j = 0; j < sCmpBase2->getValCnt(); j++)
                {
                    IDE_TEST(sCmpBase2->readPtr4Internal(j, &sVal2)
                            != IDE_SUCCESS);
                    IDE_TEST_RAISE(sCmpBase1->compare(sVal1, sVal2) == 0,
                                   err_not_unique);
                }
            }
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_unique);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "verify value error : "
                        "Property [%s] must be unique on all instances!\n",
                        sCmpBase1->getName());
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 *
 * Description :
 * 프로퍼티의 이름을 이용하여 자신의(Local Instance) 프로퍼티를 찾는다.
 * aName -[IN] 찾고자하는 프로퍼티 이름
 *
 *****************************************************************************/
idpBase *idp::findBase(const SChar *aName)
{
    UInt         i;
    iduListNode* sNode;
    idpBase*     sBase;

    for(i = 0; i < mCount; i++)
    {
        if(IDU_LIST_IS_EMPTY(&mArrBaseList[i]) != ID_TRUE)
        {
            sNode = IDU_LIST_GET_FIRST(&mArrBaseList[i]);
            sBase = (idpBase*)sNode->mObj;

            if(idlOS::strcasecmp(sBase->getName(), aName) == 0)
            {
                return (idpBase*)sBase;
            }
        }
    }

    return NULL;
}

/****************************************************************************
 *
 * Description :
 * 프로퍼티의 이름을 이용하여 프로퍼티 리스트를 찾아서 반환한다.
 * aName -[IN] 찾고자하는 프로퍼티 리스트의 프로퍼티 이름
 *
 *****************************************************************************/
iduList *idp::findBaseList(const SChar *aName)
{
    UInt         i;
    iduListNode* sNode;
    idpBase*     sBase;

    for (i = 0; i < mCount; i++)
    {
        if(IDU_LIST_IS_EMPTY(&mArrBaseList[i]) != ID_TRUE)
        {
            sNode = IDU_LIST_GET_FIRST(&mArrBaseList[i]);
            sBase = (idpBase*)sNode->mObj;

            if (idlOS::strcasecmp(sBase->getName(), aName) == 0)
            {
                return &mArrBaseList[i];
            }
        }
    }

    return NULL;
}

/****************************************************************************
 *
 * Description :
 *  aBaseList의 프로퍼티 리스트에서 SID를 이용하여 SID를 갖는 프로퍼티를 찾는다.
 * aBaseList -[IN] 대상이 되는 프로퍼티 리스트
 * aSID      -[IN] 찾고자하는 프로퍼티 리스트의 프로퍼티 이름
 *
 *****************************************************************************/
idpBase* idp::findBaseBySID(iduList* aBaseList, const SChar* aSID)
{
    idpBase*        sBase;
    iduListNode*    sNode;

    if(aBaseList != NULL)
    {
        IDU_LIST_ITERATE(aBaseList, sNode)
        {
            sBase = (idpBase*)sNode->mObj;

            if (idlOS::strcasecmp(sBase->getSID(), aSID) == 0)
            {
                return sBase;
            }
        }
    }

    return NULL;
}

/****************************************************************************
 *
 * Description :
 *   Local Instance Property(모든 프로퍼티 리스트의 첫 번째 항목)에 자신의 SID를 설정한다.
 *
 *****************************************************************************/
IDE_RC idp::setLocalSID()
{
    UInt         i;
    iduListNode* sNode;
    idpBase*     sBase;
    SChar*       sSID = NULL;
    idBool       sIsFound = ID_FALSE;

    readPtrBySrc("SID",
                 0, /*n번째 값*/
                 IDP_VALUE_FROM_ENV,
                 (void**)&sSID,
                 &sIsFound);

    if(sIsFound == ID_FALSE)
    {
        readPtrBySrc("SID",
                     0, /*n번째 값*/
                     IDP_VALUE_FROM_PFILE,
                     (void**)&sSID,
                     &sIsFound);
    }

    if(sIsFound == ID_FALSE)
    {
        readPtrBySrc("SID",
                     0, /*n번째 값*/
                     IDP_VALUE_FROM_DEFAULT,
                     (void**)&sSID,
                     &sIsFound);
    }

    IDE_TEST_RAISE(sIsFound == ID_FALSE, error_not_exist_sid);
    IDE_ASSERT(sSID != NULL);

    mSID = sSID;

    for (i = 0; i < mCount; i++)
    {
        sNode = IDU_LIST_GET_FIRST(&mArrBaseList[i]);
        sBase = (idpBase*)sNode->mObj;
        sBase->setSID(mSID);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_not_exist_sid);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "idp setLocalSID() Error : Local SID is not found!!\n");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*  aName에 해당하는 Local 프로퍼티에 aValue로 들어온 스트링 형태의 값을 aSrc로 지정한 위치에 삽입한다.
*
*   const SChar     *aName,    - [IN]  Local 프로퍼티 이름
*   SChar           *aValue,   - [IN]  스트링 형태의 값
*   idpValueSource   aSrc,     - [IN]  삽입할 Source의 위치
*                                      (default/env/pfile/spfile by asterisk, spfile by sid)
*   idBool          *aFindFlag - [OUT] 프로퍼티가 정상적으로 검색 되었는지 여부
*
*******************************************************************************************/
IDE_RC idp::insertBySrc(const SChar     *aName,
                        SChar           *aValue,
                        idpValueSource   aSrc,
                        idBool          *aFindFlag)
{
    idpBase *sBase;

    *aFindFlag = ID_FALSE;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    *aFindFlag = ID_TRUE;

    IDE_TEST(sBase->insertBySrc(aValue, aSrc) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp insert() Error : Property [%s] was not found!!\n",
                        aName);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*  aName에 해당하는프로퍼티 리스트의 모든 Base 객체에 스트링 형태의 값을 aSrc로 지정한 위치에 삽입한다.
*
*   iduList         *aBaseList, - [IN]  프로퍼티 리스트
*   SChar           *aValue,    - [IN]  스트링 형태의 값
*   idpValueSource   aSrc,      - [IN]  삽입할 Source의 위치
*                                      (default/env/pfile/spfile by asterisk, spfile by sid)
*
*******************************************************************************************/
IDE_RC idp::insertAll(iduList          *aBaseList,
                      SChar            *aValue,
                      idpValueSource    aSrc)
{
    idpBase     *sBase;
    iduListNode *sNode;

    IDU_LIST_ITERATE(aBaseList, sNode)
    {
        sBase = (idpBase*)sNode->mObj;
        IDE_TEST(sBase->insertBySrc(aValue, aSrc) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*-----------------------------------------------------------------------------
 * Name :
 *     idp::read(이름, 값, 번호)
 *
 * Description :
 * 프로퍼티의 이름을 이용하여 해당 프로퍼티의 값을 얻는다.
 * 그 값은 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
 * 사용해야 한다.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::read(const SChar *aName, void *aOutParam, UInt aNum)
{
    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    // To Fix BUG-18324 alter system set LOGICAL_AGER_COUNT수행 하는동안
    //                  client 접속이 안됨
    //
    // 이 안에서 해당 Property의 Mutex를 잡는다.
    IDE_TEST(sBase->read(aOutParam, aNum) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idp::readFromEnv(const SChar *aName, void *aOutParam, UInt aNum)
{
    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    // To Fix BUG-18324 alter system set LOGICAL_AGER_COUNT수행 하는동안
    //                  client 접속이 안됨
    //
    // 이 안에서 해당 Property의 Mutex를 잡는다.
    IDE_TEST(sBase->readBySrc(aOutParam, IDP_VALUE_FROM_ENV, aNum) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
* 프로퍼티의 이름과 SID를 이용하여 해당 프로퍼티의 포인터를 얻는다.
* 그 값은 더블 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
* 사용해야 한다.
* 대상 프로퍼티가 읽기전용이고, String 타입일 경우에만 유효하다.
*
* const SChar   *aSID       - [IN]  찾고자하는 프로퍼티의 SID
* const SChar   *aName,     - [IN]  찾고자하는 프로퍼티 이름
* UInt           aNum,      - [IN]  프로퍼티에 저장된 n 번째 값을 의미하는 인덱스
* void          *aOutParam, - [OUT] 결과 값
*******************************************************************************************/
IDE_RC idp::readBySID(const SChar *aSID,
                      const SChar *aName,
                      UInt         aNum,
                      void        *aOutParam)
{
    idpBase *sBase;
    iduList *sBaseList;

    sBaseList = findBaseList(aName);

    IDE_TEST_RAISE(sBaseList == NULL, not_found_error);

    sBase = findBaseBySID(sBaseList, aSID);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    IDE_TEST(sBase->read(aOutParam, aNum) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_Property_NotFound, aSID, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
* 프로퍼티의 이름과 SID를 이용하여 해당 프로퍼티의 포인터를 얻는다.
* 그 값은 더블 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
* 사용해야 한다.
* 대상 프로퍼티가 읽기전용이고, String 타입일 경우에만 유효하다.
* 찾고자 하는 값이 없으면 aIsFound에 ID_FALSE를 setting하고, 있으면
* ID_TRUE을 setting한다.
*
* const SChar   *aSID       - [IN]  찾고자하는 프로퍼티의 SID
* const SChar   *aName,     - [IN]  찾고자하는 프로퍼티 이름
* UInt           aNum,      - [IN]  프로퍼티에 저장된 n 번째 값을 의미하는 인덱스
* void         **aOutParam, - [OUT] 결과 값의 포인터
* idBool        *aIsFound)  - [OUT] 검색 성공 여부
*******************************************************************************************/
IDE_RC idp::readPtrBySID(const SChar *aSID,
                         const SChar *aName,
                         UInt         aNum,
                         void       **aOutParam,
                         idBool      *aIsFound)
{
    idpBase *sBase;
    iduList* sBaseList;

    *aIsFound = ID_FALSE;

    sBaseList = findBaseList(aName);

    sBase = findBaseBySID(sBaseList, aSID);

    if(sBase != NULL)
    {
        IDE_TEST(sBase->readPtr(aOutParam, aNum) != IDE_SUCCESS);

        *aIsFound = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::readPtr(이름, 포인터 ** , 번호, aIsFound)
 *
 * Description :
 * 프로퍼티의 이름을 이용하여 해당 프로퍼티의 포인터를 얻는다.
 * 그 값은 더블 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
 * 사용해야 한다.
 * 대상 프로퍼티가 읽기전용이고, String 타입일 경우에만 유효하다.
 * 찾고자 하는 값이 없으면 aIsFound에 ID_FALSE를 setting하고, 있으면
 * ID_TRUE을 setting한다.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::readPtr(const SChar *aName,
                    void       **aOutParam,
                    UInt         aNum,
                    idBool      *aIsFound)
{
    idpBase *sBase;

    *aIsFound = ID_FALSE;

    sBase = findBase(aName);

    if(sBase != NULL)
    {
        *aIsFound = ID_TRUE;

        // To Fix BUG-18324 alter system set LOGICAL_AGER_COUNT수행 하는동안
        //                  client 접속이 안됨
        //
        // 이 안에서 해당 Property의 Mutex를 잡는다.
        IDE_TEST(sBase->readPtr(aOutParam, aNum) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
* 프로퍼티의 이름을 이용하여 해당하는 Local Instance 프로퍼티를 찾고
* 찾은 프로퍼티에서 source의 위치에 저장된 값의 포인터를 얻는다.
* 그 값은 더블 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
* 사용해야 한다.
* 대상 프로퍼티가, String 타입일 경우에만 유효하다.
* 찾고자 하는 값이 없으면 aIsFound에 ID_FALSE를 setting하고, 있으면
* ID_TRUE을 setting한다.
* const SChar   *aName,     - [IN]  찾고자하는 프로퍼티 이름
* UInt           aNum,      - [IN]  프로퍼티에 저장된 n 번째 값을 의미하는 인덱스
* idpValueSource aSrc,      - [IN]  어떤 설정 방법에 의해서 설정된 값인지를 나타내는 Value Source
* void         **aOutParam, - [OUT] 결과 값의 포인터
* idBool        *aIsFound)  - [OUT] 검색 성공 여부
*******************************************************************************************/
void idp::readPtrBySrc(const SChar   *aName,
                       UInt           aNum,
                       idpValueSource aSrc,
                       void         **aOutParam,
                       idBool        *aIsFound)
{
    idpBase *sBase;

    *aIsFound  = ID_FALSE;
    *aOutParam = NULL;

    sBase = findBase(aName);

    if(sBase != NULL)
    {
        if(sBase->readPtrBySrc(aNum, aSrc, aOutParam) == IDE_SUCCESS)
        {
            IDE_ASSERT(*aOutParam != NULL);

            *aIsFound = ID_TRUE;
        }
        else
        {
            *aIsFound = ID_FALSE;
        }
    }

    return;
}

/******************************************************************************************
*
* Description :
* 프로퍼티의 이름을 이용하여 해당하는 Local Instance 프로퍼티를 찾고
* 찾은 프로퍼티에서 source의 위치에 저장된 값에 대해 복사할 메모리를 할당하여 할당된 위치에 값을 복사한 후,
* 복사된 메모리의 포인터를 얻는다. 그러므로, 이 함수를 호출한 쪽에서 메모리를 해제해야한다.
* 이 값은 String 타입의 프로퍼티에 대해서는 ?를 $ALTIBASE_HOME으로 변경한
* 값을 반환한다.
* 찾고자 하는 값이 없으면 aIsFound에 ID_FALSE를 setting하고, 있으면
* ID_TRUE을 setting한다.
* const SChar   *aName,     - [IN]  찾고자하는 프로퍼티 이름
* UInt           aNum,      - [IN]  프로퍼티에 저장된 n 번째 값을 의미하는 인덱스
* idpValueSource aSrc,      - [IN]  어떤 설정 방법에 의해서 설정된 값인지를 나타내는 Value Source
* void         **aOutParam, - [OUT] 메모리 생성후 복사된 프로퍼티 값의 포인터
* idBool        *aIsFound)  - [OUT] 검색 성공 여부
*******************************************************************************************/
void idp::readClonedPtrBySrc(const SChar   *aName,
                             UInt           aNum,
                             idpValueSource aSrc,
                             void         **aOutParam,
                             idBool        *aIsFound)
{
    idpBase *sBase;
    void    *sSrcValue;

    *aIsFound  = ID_FALSE;
    *aOutParam = NULL;

    sBase = findBase(aName);

    if(sBase != NULL)
    {
        if(sBase->readPtrBySrc(aNum, aSrc, &sSrcValue) == IDE_SUCCESS)
        {
            IDE_ASSERT(sSrcValue != NULL);

            sBase->cloneValue(sBase, sSrcValue, aOutParam);

            *aIsFound = ID_TRUE;
        }
        else
        {
            *aIsFound = ID_FALSE;
        }
    }

    return;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::readPtr(이름, 포인터 ** , 번호)
 *
 * Description :
 * 프로퍼티의 이름을 이용하여 해당 프로퍼티의 포인터를 얻는다.
 * 그 값은 더블 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
 * 사용해야 한다.
 * 대상 프로퍼티가 읽기전용이고, String 타입일 경우에만 유효하다.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::readPtr(const SChar *aName, void **aOutParam, UInt aNum)
{
    idBool sIsFound;

    IDE_TEST(readPtr(aName, aOutParam, aNum, &sIsFound)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sIsFound == ID_FALSE, not_found_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
* 프로퍼티의 이름과 SID를 이용하여 해당 프로퍼티의 포인터를 얻는다.
* 그 값은 더블 포인터 형태이며, 호출자가 적절한 데이터 타입으로 변경해서
* 사용해야 한다.
* 대상 프로퍼티가 읽기전용이고, String 타입일 경우에만 유효하다.
*
* const SChar   *aSID       - [IN]  찾고자하는 프로퍼티의 SID
* const SChar   *aName,     - [IN]  찾고자하는 프로퍼티 이름
* UInt           aNum,      - [IN]  프로퍼티에 저장된 n 번째 값을 의미하는 인덱스
* void         **aOutParam, - [OUT] 결과 값의 포인터
*******************************************************************************************/
IDE_RC idp::readPtrBySID(const SChar *aSID,
                         const SChar *aName,
                         UInt         aNum,
                         void       **aOutParam)
{
    idBool sIsFound;

    IDE_TEST(readPtrBySID(aSID, aName, aNum, aOutParam, &sIsFound)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sIsFound == ID_FALSE, not_found_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_Property_NotFound,
                                aSID,
                                aName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::update(이름, Native값, 번호)
 *
 * Description :
 * 프로퍼티의 이름을 이용하여 해당 프로퍼티의 값을 변경한다.
 * 그 값은 포인터 형태이며, 가리키는 값이 해당 데이타 타입이어야 한다.
 * 스트링 형태일 경우, 정확한 동작을 보장할 수 없다.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::update(idvSQL      * aStatistics,
                   const SChar * aName,
                   UInt          aInParam,
                   UInt          aNum,
                   void        * aArg)
{
    SChar sCharVal[16];

    idlOS::snprintf(sCharVal, ID_SIZEOF(sCharVal), "%"ID_UINT32_FMT, aInParam);

    return idp::update(aStatistics, aName, sCharVal, aNum, aArg);
}

IDE_RC idp::update(idvSQL      * aStatistics,
                   const SChar * aName,
                   ULong         aInParam,
                   UInt          aNum,
                   void        * aArg)
{
    SChar sCharVal[32];

    idlOS::snprintf(sCharVal, ID_SIZEOF(sCharVal), "%"ID_UINT64_FMT, aInParam);

    return idp::update(aStatistics, aName, sCharVal, aNum, aArg);
}

// BUG-43533 OPTIMIZER_FEATURE_ENABLE
IDE_RC idp::update4Startup(idvSQL      * aStatistics,
                           const SChar * aName,
                           SChar       * aInParam,
                           UInt          aNum,
                           void        * aArg)
{

    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    // To Fix BUG-18324 alter system set LOGICAL_AGER_COUNT수행 하는동안
    //                  client 접속이 안됨
    //
    // 이 안에서 해당 Property의 Mutex를 잡는다.
    IDE_TEST(sBase->update4Startup(aStatistics, aInParam, aNum, aArg) != IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idp::update(idvSQL      * aStatistics,
                   const SChar * aName,
                   SChar       * aInParam,
                   UInt          aNum,
                   void        * aArg)
{

    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    // To Fix BUG-18324 alter system set LOGICAL_AGER_COUNT수행 하는동안
    //                  client 접속이 안됨
    //
    // 이 안에서 해당 Property의 Mutex를 잡는다.
    IDE_TEST(sBase->update(aStatistics, aInParam, aNum, aArg) != IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::updateForce(이름, Native값, 번호)
 *
 * Description :
 * 프로퍼티의 이름을 이용하여 해당 프로퍼티의 값을 "강제로" 변경한다.
 * 그 값은 포인터 형태이며, 가리키는 값이 해당 데이타 타입이어야 한다.
 * 스트링 형태일 경우, 정확한 동작을 보장할 수 없다.
 * READ-ONLY 프로퍼티를 수정할 때 활용하며 유닛테스트 전용으로 사용한다.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::updateForce(const SChar *aName, UInt aInParam,  UInt aNum, void *aArg)
{
    SChar sCharVal[16];

    idlOS::snprintf(sCharVal, ID_SIZEOF(sCharVal), "%"ID_UINT32_FMT, aInParam);

    return idp::updateForce(aName, sCharVal, aNum, aArg);
}

IDE_RC idp::updateForce(const SChar *aName, ULong aInParam,  UInt aNum, void *aArg)
{
    SChar sCharVal[32];

    idlOS::snprintf(sCharVal, ID_SIZEOF(sCharVal), "%"ID_UINT64_FMT, aInParam);

    return idp::updateForce(aName, sCharVal, aNum, aArg);
}


IDE_RC idp::updateForce(const SChar *aName, SChar *aInParam,  UInt aNum, void *aArg)
{
    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    // To Fix BUG-18324 alter system set LOGICAL_AGER_COUNT수행 하는동안
    //                  client 접속이 안됨
    //
    // 이 안에서 해당 Property의 Mutex를 잡는다.
    IDE_TEST(sBase->updateForce(aInParam, aNum, aArg) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idp::validate( const SChar * aName,
                      SChar       * aInParam,
                      idBool        aIsSystem )
{
    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    IDE_TEST(sBase->validate( aInParam, aIsSystem )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

// PROJ-2727
IDE_RC idp::getPropertyAttribute( const SChar * aName,
                                  UInt        * aOutParam )
{
    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    IDE_TEST(sBase->getPropertyAttribute( aOutParam )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::setupBeforeUpdateCallback(이름, 호출될 콜백 함수 )
 *
 * Description :
 * 변경시 호출될 콜백을 등록한다.
 * 어떤 Name의 프로퍼티가 변경될 때, 이 함수가 호출되는데,
 * 이때 이전값과 이후값이 함께 돌아온다.
 *
 * 이런 콜백 매커니즘이 필요한 이유는, 어떤 프로퍼티의 경우 변경작업시에
 * 특별히 사전 조치가 필요하기 때문이다.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC idp::setupBeforeUpdateCallback(const SChar *aName,
                                      idpChangeCallback mCallback)
{
    idBool sLocked = ID_FALSE;

    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    IDE_TEST_RAISE(idlOS::thread_mutex_lock(&mMutex) != 0,
                   lock_error);
    sLocked = ID_TRUE;
    sBase->setupBeforeUpdateCallback(mCallback);
    sLocked = ID_FALSE;


    IDE_TEST_RAISE(idlOS::thread_mutex_unlock(&mMutex) != 0,
                   unlock_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }
    IDE_EXCEPTION(lock_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "idp setupBeforeUpdateCallback() Error : Mutex Lock Failed.\n");
    }

    IDE_EXCEPTION(unlock_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "idp setupBeforeUpdateCallback() Error : Mutex Unlock Failed.\n");
    }

    IDE_EXCEPTION_END;
    if (sLocked == ID_TRUE)
    {
        IDE_ASSERT(idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     idp::setupAfterUpdateCallback(이름, 호출될 콜백 함수 )
 *
 * Description :
 * 변경시 호출될 콜백을 등록한다.
 * 어떤 Name의 프로퍼티가 변경될 때, 이 함수가 호출되는데,
 * 이때 이전값과 이후값이 함께 돌아온다.
 *
 * 이런 콜백 매커니즘이 필요한 이유는, 어떤 프로퍼티의 경우 변경작업시에
 * 특별히 사전 조치가 필요하기 때문이다.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC idp::setupAfterUpdateCallback(const SChar *aName,
                                     idpChangeCallback mCallback)
{
    idBool sLocked = ID_FALSE;

    idpBase *sBase;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    IDE_TEST_RAISE(idlOS::thread_mutex_lock(&mMutex) != 0,
                   lock_error);
    sLocked = ID_TRUE;
    sBase->setupAfterUpdateCallback(mCallback);
    sLocked = ID_FALSE;
    IDE_TEST_RAISE(idlOS::thread_mutex_unlock(&mMutex) != 0,
                   unlock_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }
    IDE_EXCEPTION(lock_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "idp setupAfterUpdateCallback() Error : Mutex Lock Failed.\n");
    }

    IDE_EXCEPTION(unlock_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE,
                        "idp setupAfterUpdateCallback() Error : Mutex Unlock Failed.\n");
    }

    IDE_EXCEPTION_END;
    if (sLocked == ID_TRUE)
    {
        IDE_ASSERT(idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}


/*-----------------------------------------------------------------------------
 * Name :
 *     idp::readPFile()
 *
 * Description :
 * configuration 화일을 파싱하고, 내용을 입력한다.(insert())
 *
 * ---------------------------------------------------------------------------*/

IDE_RC idp::readPFile()
{
    SChar       sConfFile[512];
    SChar       sLineBuf[IDP_MAX_PROP_LINE_SIZE];
    PDL_HANDLE  sFD = PDL_INVALID_HANDLE;
    SChar       *sName;
    SChar       *sValue;
    idBool sFindFlag;
    idBool sOpened = ID_FALSE;

    idlOS::memset(sConfFile, 0, ID_SIZEOF(sConfFile));

#if !defined(VC_WIN32)
    if ( mConfName[0] == IDL_FILE_SEPARATOR )
#else
    if ( mConfName[1] == ':'
      && mConfName[2] == IDL_FILE_SEPARATOR )
#endif /* VC_WIN32 */
    {   // Absolute Path
        idlOS::snprintf(sConfFile, ID_SIZEOF(sConfFile), "%s", mConfName);
    }
    else
    {
        idlOS::snprintf(sConfFile, ID_SIZEOF(sConfFile), "%s%c%s", mHomeDir, IDL_FILE_SEPARATOR, mConfName);
    }

    sFD = idf::open(sConfFile, O_RDONLY);

    IDE_TEST_RAISE(sFD == PDL_INVALID_HANDLE, open_error);
    // fix BUG-25544 [CodeSonar::DoubleClose]
    sOpened = ID_TRUE;

    while(!idf::fdeof(sFD))
    {
        idlOS::memset(sLineBuf, 0, IDP_MAX_PROP_LINE_SIZE);
        if (idf::fdgets(sLineBuf, IDP_MAX_PROP_LINE_SIZE, sFD) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }
        // sLineBuf에 한줄의 정보가 있음

        sName  = NULL;
        sValue = NULL;

        IDE_TEST(parseBuffer(sLineBuf, &sName, &sValue) != IDE_SUCCESS);

        if ((sName != NULL) && (sValue != NULL)) // 프로퍼티 라인에 정보가 있음(이름+값 쓰기)
        {
            sFindFlag = ID_FALSE;

            if (insertBySrc(sName, sValue, IDP_VALUE_FROM_PFILE, &sFindFlag) != IDE_SUCCESS)
            {
                IDE_TEST_RAISE(sFindFlag != ID_FALSE, err_insert);
                /* 단순히 Name을 못찾을 경우에는 진행하도록 함*/
                ideLog::log(IDE_SERVER_0, "%s\n", idp::getErrorBuf());
            }
        }
    }

    // fix BUG-25544 [CodeSonar::DoubleClose]
    sOpened = ID_FALSE;
    IDE_TEST_RAISE(idf::close(sFD) != 0, close_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(open_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp readConf() Error : Open File [%s] Error.\n", sConfFile);
    }
    IDE_EXCEPTION(close_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp readConf() Error : Close File [%s] Error.\n", sConfFile);
    }
    IDE_EXCEPTION(err_insert);
    {
        /*insertBySrc에서 에러 메세지가 설정되었음.*/
    }

    IDE_EXCEPTION_END;

    if (sFD != PDL_INVALID_HANDLE)
    {
        // fix BUG-25544 [CodeSonar::DoubleClose]
        if (sOpened == ID_TRUE)
        {
            (void)idf::close(sFD);
        }
    }


    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*           SPFile을 파싱하고, 내용을 입력한다.
*******************************************************************************************/
IDE_RC idp::readSPFile()
{
    SChar       *sSPFileName = NULL;
    SChar        sLineBuf[IDP_MAX_PROP_LINE_SIZE];
    FILE        *sFP = NULL;
    SChar       *sSID;
    SChar       *sName;
    SChar       *sValue;
    iduList     *sBaseList;
    idpBase     *sBase;
    idpBase     *sCloneBase;
    iduListNode *sNode;
    idBool       sIsFound = ID_FALSE;
    idBool       sOpened  = ID_FALSE;

    /* ?가 $ALTIBASE_HOME으로 변환되어서 반환되어야 하므로, readClonedPtrBySrc를
     * 통해서 프로퍼티 값을 가져와야한다.*/
    readClonedPtrBySrc("SPFILE",
                       0, /*n번째 값*/
                       IDP_VALUE_FROM_ENV,
                       (void**)&sSPFileName,
                       &sIsFound);

    if(sIsFound != ID_TRUE)
    {
        IDE_ASSERT(sSPFileName == NULL);
        readClonedPtrBySrc("SPFILE",
                           0, /*n번째 값*/
                           IDP_VALUE_FROM_PFILE,
                           (void**)&sSPFileName,
                           &sIsFound);
    }
    else
    {
        /*SPFILE 프로퍼티는 default로 값이 없다. 그러므로, default 값은 검색하지 않는다.*/
    }

    /* SPFILE이 없더라도 ENV/PFILE을 통해 모든 프로퍼티를 읽어들이고 서버를 시작할 수 있으므로
     * 파일 분석을 건너뛰고 다음 과정을 진행한다.*/
    IDE_TEST_RAISE(sIsFound == ID_FALSE, cont_next_step);

    IDE_TEST_RAISE(sSPFileName == NULL, null_spfile_name);
    sFP = idlOS::fopen(sSPFileName, "r");

    IDE_TEST_RAISE(sFP == NULL, open_error);
    sOpened = ID_TRUE;

    while(!feof(sFP))
    {
        idlOS::memset(sLineBuf, 0, IDP_MAX_PROP_LINE_SIZE);
        if (idlOS::fgets(sLineBuf, IDP_MAX_PROP_LINE_SIZE, sFP) == NULL)
        {
            // 화일의 끝까지 읽음
            break;
        }

        // sLineBuf에 한줄의 정보가 있음
        sSID   = NULL;
        sName  = NULL;
        sValue = NULL;

        IDE_TEST(parseSPFileLine(sLineBuf, &sSID, &sName, &sValue) != IDE_SUCCESS);

        if((sSID != NULL) && (sName != NULL) && (sValue != NULL))
        {
            sBaseList = findBaseList(sName);

            if(sBaseList == NULL)
            {
                idlOS::snprintf(mErrorBuf,
                                IDP_ERROR_BUF_SIZE,
                                "idp readSPFile() Error : "
                                "Property [%s] Not Registered.\n",
                                sName );
                /* 단순히 Name을 못찾을 경우에는 진행하도록 함*/
                ideLog::log(IDE_SERVER_0, "%s\n", idp::getErrorBuf());
                continue;
            }

            /*SID가 *로 표기된 경우*/
            if(idlOS::strcmp(sSID, "*") == 0)
            {
                IDE_TEST(insertAll(sBaseList,
                         sValue,
                         IDP_VALUE_FROM_SPFILE_BY_ASTERISK)
                        != IDE_SUCCESS);
            }
            else
            {
                sBase = findBaseBySID(sBaseList, sSID);

                if (sBase != NULL)
                {
                    /*sSID를 갖는 프로퍼티에 sValue값을 저장*/
                    IDE_TEST(sBase->insertBySrc(sValue, IDP_VALUE_FROM_SPFILE_BY_SID) != IDE_SUCCESS);
                }
                else
                {
                    /*Local Instance의 프로퍼티와 동일한 타입의 프로퍼티를 생성하여 리스트에 연결*/
                    sNode = IDU_LIST_GET_FIRST(sBaseList);
                    sBase = (idpBase*)sNode->mObj;

                    IDE_TEST(sBase->clone(sSID, &sCloneBase) != IDE_SUCCESS);

                    IDE_TEST(sCloneBase->insertBySrc(sValue, IDP_VALUE_FROM_SPFILE_BY_SID) != IDE_SUCCESS);

                    sNode = (iduListNode*)iduMemMgr::mallocRaw(ID_SIZEOF(iduListNode));

                    IDE_TEST_RAISE(sNode == NULL, memory_alloc_error);

                    IDU_LIST_INIT_OBJ(sNode, sCloneBase);
                    IDU_LIST_ADD_LAST(sBaseList, sNode);
                }
            }
        }
    }

    sOpened = ID_FALSE;
    IDE_TEST_RAISE(idlOS::fclose(sFP) != 0, close_error);

    if(sSPFileName != NULL)
    {
        iduMemMgr::freeRaw(sSPFileName);
        sSPFileName = NULL;
    }

    IDE_EXCEPTION_CONT(cont_next_step);

    return IDE_SUCCESS;

    IDE_EXCEPTION(open_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp readSPFile() Error : Open File [%s] Error.\n",
                        sSPFileName);
    }
    IDE_EXCEPTION(close_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp readSPFile() Error : Close File [%s] Error.\n",
                        sSPFileName);
    }
    IDE_EXCEPTION(memory_alloc_error)
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp regist() Error : memory allocation error\n");
    }
    IDE_EXCEPTION(null_spfile_name)
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp readSPFile() Error : NULL file name\n");
    }

    IDE_EXCEPTION_END;

    if (sFP != NULL)
    {
        if (sOpened == ID_TRUE)
        {
            (void)idlOS::fclose(sFP);
        }
    }

    if(sSPFileName != NULL)
    {
        iduMemMgr::freeRaw(sSPFileName);
        sSPFileName = NULL;
    }

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
*           SPFile에서 읽은 라인을 분석하여 SID, Name, Value로 분리해 준다.
*
* SChar  *aLineBuf, -[IN] SPFile에서 읽은 라인 버퍼
* SChar **aSID,     -[OUT] SID
* SChar **aName,    -[OUT] Name
* SChar **aValue    -[OUT] Value
*******************************************************************************************/
IDE_RC idp::parseSPFileLine(SChar  *aLineBuf,
                            SChar **aSID,
                            SChar **aName,
                            SChar **aValue)
{
    SChar *sNameWithSID = NULL;

    *aSID   = NULL;
    *aName  = NULL;
    *aValue = NULL;

    IDE_TEST(parseBuffer(aLineBuf, &sNameWithSID, aValue) != IDE_SUCCESS);

    if(sNameWithSID != NULL)
    {
        parseSID(sNameWithSID, aSID, aName);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 스트링에서 첫글자를 지움 */
void idp::eraseCharacter(SChar *aBuf)
{
    SInt i;

    for (i = 0; aBuf[i]; i++)
    {
        aBuf[i] = aBuf[i + 1];
    }
}

/* 스트링에 존재하는 모든 WHITE-SPACE를 제거 */
void idp::eraseWhiteSpace(SChar *aLineBuf)
{
    SInt   i;
    SInt   sLen;
    idBool sIsValue;
    SChar  sQuoteChar;

    sLen       = idlOS::strlen(aLineBuf);
    sIsValue   = ID_FALSE;
    sQuoteChar = 0;

    for (i = 0; i < sLen && aLineBuf[i]; i++)
    {
        if (aLineBuf[i] == '=')
        {
            sIsValue = ID_TRUE;
        }
        if (sIsValue == ID_TRUE)
        {
            if (sQuoteChar != 0)
            {
                if (sQuoteChar == aLineBuf[i])
                {
                    sQuoteChar = 0;
                    eraseCharacter(&aLineBuf[i]);
                    i--;
                }
            }
            else if (aLineBuf[i] == '"' || aLineBuf[i] == '\'')
            {
                sQuoteChar = aLineBuf[i];
                eraseCharacter(&aLineBuf[i]);
                i--;
            }
        }
        if (sQuoteChar == 0)
        {
            if (aLineBuf[i] == '#')
            {
                aLineBuf[i]= 0;
                return;
            }
            if (isspace(aLineBuf[i]) != 0) // 스페이스 임
            {
                eraseCharacter(&aLineBuf[i]);
                i--;
            }
        }
    }
}

IDE_RC idp::parseBuffer(SChar  *aLineBuf,
                        SChar **aName,
                        SChar **aValue)
{
    SInt i;
    SInt sLen;

    // 1. White Space 제거
    eraseWhiteSpace(aLineBuf);

    // 2. 내용이 없거나 주석이면 무시
    sLen = idlOS::strlen(aLineBuf);

    if (sLen > 0 && aLineBuf[0] != '#')
    {
        *aName = aLineBuf;

        // 3. value 존재유무 검사
        for (i = 0; i < sLen; i++)
        {
            if (aLineBuf[i] == '=')
            {
                // 구분자가 존재하면,
                aLineBuf[i] = 0;

                if (aLineBuf[i + 1] != 0) // Value가 존재함.
                {
                    *aValue = &aLineBuf[i + 1];

                    IDE_TEST_RAISE(idlOS::strlen(&aLineBuf[i + 1]) > IDP_MAX_VALUE_LEN,
                                   too_long_value);
                }
                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(too_long_value);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp parseBuffer() Error : [%s] Value is too long. "
                        "(not over %"ID_UINT32_FMT")\n", *aName, (UInt)IDP_MAX_VALUE_LEN);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC idp::parseSID(SChar  *aBuf,
                     SChar **aSID,
                     SChar **aName)
{
    SInt i;
    SInt sLen;

    IDE_ASSERT(aBuf != NULL);

    *aSID  = NULL;
    *aName = NULL;

    sLen = idlOS::strlen(aBuf);

    if(sLen > 0)
    {
        *aSID = aBuf;

        for (i = 0; i < sLen; i++)
        {
            if (aBuf[i] == '.')
            {
                // 구분자가 존재하면,
                aBuf[i] = 0;

                if (aBuf[i + 1] != 0) // Property Name이 존재함.
                {
                    *aName = &aBuf[i + 1];

                    IDE_TEST_RAISE(idlOS::strlen(&aBuf[i + 1]) > IDP_MAX_VALUE_LEN,
                                   err_too_long_propertyName);
                }

                break;
            }
        }

        IDE_TEST_RAISE(idlOS::strlen(*aSID) > IDP_MAX_SID_LEN,
                       err_too_long_propertySID);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_long_propertyName);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp parseSID() Error : [%s] Property name is too long. "
                        "(not over %"ID_UINT32_FMT")\n",
                        *aName,
                        (UInt)IDP_MAX_VALUE_LEN);
    }

    IDE_EXCEPTION(err_too_long_propertySID);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp parseSID() Error : [%s] Property SID is too long. "
                        "(not over %"ID_UINT32_FMT")\n",
                        *aSID,
                        (UInt)IDP_MAX_VALUE_LEN);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*-----------------------------------------------------------------------------
 * Name :
 *     getPropertyCount(이름)
 *
 * Description :
 * property가 몇개 정의 되었는가.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::getMemValueCount(const SChar *aName, UInt  *aPropertyCount)
{
    idpBase *sBase;
    idBool sLocked = ID_FALSE;

    sBase = findBase(aName);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    IDE_TEST_RAISE(idlOS::thread_mutex_lock(&mMutex) != 0,
                   lock_error);

    sLocked = ID_TRUE;
    *aPropertyCount = sBase->getMemValueCount();
    sLocked = ID_FALSE;

    IDE_TEST_RAISE(idlOS::thread_mutex_unlock(&mMutex) != 0,
                   unlock_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION(lock_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp read() Error : Mutex Lock Failed.\n");
    }

    IDE_EXCEPTION(unlock_error);
    {
        idlOS::snprintf(mErrorBuf, IDP_ERROR_BUF_SIZE, "idp read() Error : Mutex Unlock Failed.\n");
    }

    IDE_EXCEPTION_END;
    if (sLocked == ID_TRUE)
    {
        IDE_ASSERT(idlOS::thread_mutex_unlock(&mMutex) == 0);
    }


    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     getStoredCountBySID(SID,이름)
 *
 * Description :
 * sid와 name을 이용하여 검색한 property의 값이 몇개 정의 되었는가.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC idp::getStoredCountBySID(const SChar* aSID,
                                const SChar *aName,
                                UInt        *aPropertyCount)
{
    idBool sLocked = ID_FALSE;

    idpBase *sBase;
    iduList *sBaseList;

    sBaseList = findBaseList(aName);

    sBase = findBaseBySID(sBaseList, aSID);

    IDE_TEST_RAISE(sBase == NULL, not_found_error);

    IDE_TEST_RAISE(idlOS::thread_mutex_lock(&mMutex) != 0,
                   lock_error);
    sLocked = ID_TRUE;

    *aPropertyCount = sBase->getMemValueCount();

    sLocked = ID_FALSE;
    IDE_TEST_RAISE(idlOS::thread_mutex_unlock(&mMutex) != 0,
                   unlock_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idp_NameNotFound, aName));
    }

    IDE_EXCEPTION(lock_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp read() Error : Mutex Lock Failed.\n");
    }

    IDE_EXCEPTION(unlock_error);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp read() Error : Mutex Unlock Failed.\n");
    }

    IDE_EXCEPTION_END;

    if (sLocked == ID_TRUE)
    {
        IDE_ASSERT(idlOS::thread_mutex_unlock(&mMutex) == 0);
    }

    return IDE_FAILURE;
}

/******************************************************************************************
*
* Description :
* aName을 갖는 프로퍼티 리스트를 검색하여, aName을 설정한 인스턴스의 SID를 모두
* 찾아서, aSIDArray에 넣어서 돌려주며, 검색된 SID들의 개수를 return 값과 aCount로 반환한다.
*
* UInt           aNum,      - [IN]  프로퍼티에 저장된 n 번째 값을 의미하는 인덱스
* void         **aSIDArray, - [OUT] SID들을 저장할 수 있는 포인터 배열
* UInt          *aCount     - [OUT] 검색된 SID 개수 (return 값과 동일)
 *******************************************************************************************/
void idp::getAllSIDByName(const SChar *aName, SChar** aSIDArray, UInt* aCount)
{
    UInt            sCount = 0;
    idpBase        *sBase;
    iduListNode    *sNode;
    iduList        *sBaseList;

    sBaseList = findBaseList(aName);

    if(sBaseList != NULL)
    {
        IDU_LIST_ITERATE(sBaseList, sNode)
        {
            sBase = (idpBase*)sNode->mObj;
            aSIDArray[sCount] = sBase->getSID();
            sCount++;
        }
    }

    *aCount = sCount;

    return;
}

/******************************************************************************************
*
* Description :
* Source 우선순위에 따라 프로퍼티의 Memory Value를 결정하고, Memory Value를 저장할 공간을 할당한 후,
* 결정된 값을 설정하여, 프로퍼티 객체(idpBase)에 삽입한다.
 *******************************************************************************************/
IDE_RC idp::insertMemoryValueByPriority()
{
    UInt            i, j;
    iduListNode    *sNode;
    iduList        *sBaseList;
    idpBase        *sBase;
    UInt            sCount;
    idpValueSource  sValSrc;
    void           *sVal;

    /*모든 프로퍼티에 대한 리스트*/
    for (i = 0; i < mCount; i++)
    {
        sBaseList = &mArrBaseList[i];
        /*하나의 프로퍼티 리스트의 각 항목에 대해서*/
        IDU_LIST_ITERATE(sBaseList, sNode)
        {
            sBase = (idpBase*)sNode->mObj;

            /* SID를 이용하여 SPFILE에 설정한 값이 있는지 확인*/
            if(sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_SID].mCount > 0)
            {
                sCount = sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_SID].mCount;
                sValSrc = IDP_VALUE_FROM_SPFILE_BY_SID;
            }
            /* "*"를 이용하여 SPFILE에 설정한 값이 있는지 확인*/
            else if(sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_ASTERISK].mCount > 0)
            {
                sCount = sBase->mSrcValArr[IDP_VALUE_FROM_SPFILE_BY_ASTERISK].mCount;
                sValSrc = IDP_VALUE_FROM_SPFILE_BY_ASTERISK;
            }
            /* ENV를 이용하여 설정한 값이 있는지 확인*/
            else if(sBase->mSrcValArr[IDP_VALUE_FROM_ENV].mCount > 0)
            {
                sCount = sBase->mSrcValArr[IDP_VALUE_FROM_ENV].mCount;
                sValSrc = IDP_VALUE_FROM_ENV;
            }
            /* PFILE을 이용하여 설정한 값이 있는지 확인*/
            else if(sBase->mSrcValArr[IDP_VALUE_FROM_PFILE].mCount > 0)
            {
                sCount = sBase->mSrcValArr[IDP_VALUE_FROM_PFILE].mCount;
                sValSrc = IDP_VALUE_FROM_PFILE;
            }
            /* default 값으로 설정될 수 있는가 확인*/
            else if(sBase->allowDefault() == ID_TRUE)//default 아무 값도 설정되지 않았음
            {
                sCount = sBase->mSrcValArr[IDP_VALUE_FROM_DEFAULT].mCount;
                sValSrc = IDP_VALUE_FROM_DEFAULT;
            }
            else
            {
                /* property가 어떤 값도 설정되지 않았고, Default를 사용할 수도 없으면 에러
                 * 다른 노드의 프로퍼티이면 SPFILE에 기술된 값이 있으므로
                 * 이 부분으로 들어올 수 없다.*/
                IDE_RAISE(default_value_not_allowed);
            }

            for(j = 0; j < sCount; j++) //모든 값을 values에 삽입
            {
                sVal = sBase->mSrcValArr[sValSrc].mVal[j];
                IDE_TEST(sBase->insertMemoryRawValue(sVal) != IDE_SUCCESS);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(default_value_not_allowed);
    {
        idlOS::snprintf(mErrorBuf,
                        IDP_ERROR_BUF_SIZE,
                        "idp insertMemoryValueByPriority() Error : "
                        "Property [%s] should be specified by configuration.",
                        sBase->getName());
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
