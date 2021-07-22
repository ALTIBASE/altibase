/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutexMgr.cpp 90742 2021-04-28 06:25:02Z kclee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduRunTimeInfo.h>
#include <iduMutexMgr.h>
#include <iduMemPool.h>
#include <iduProperty.h>
#include <idp.h>

//fix PROJ-1749
extern       iduMutexOP gPosixMutexServerOps;
extern       iduMutexOP gPosixMutexClientOps;
IDL_EXTERN_C iduMutexOP gNativeMutexServerOps;
IDL_EXTERN_C iduMutexOP gNativeMutexClientOps;

iduMutexOP *iduMutexMgr::mMutexOpArrayServer[IDU_MUTEX_KIND_MAX] =
{
    &gPosixMutexServerOps,
    &gNativeMutexServerOps,
    &gNativeMutexServerOps,
    &gNativeMutexServerOps // BUG-28856 logging 병목제거
};

iduMutexOP *iduMutexMgr::mMutexOpArrayClient[IDU_MUTEX_KIND_MAX] =
{
    &gPosixMutexClientOps,
    &gNativeMutexClientOps,
    &gNativeMutexServerOps,
    &gNativeMutexClientOps // BUG-28856 logging 병목제거
};

iduMutexEntry      iduMutexMgr::mPosixEntry;
iduMutexEntry      iduMutexMgr::mNativeEntry;
iduMutexEntry      iduMutexMgr::mInfoHeadEntry;
iduMutexEntry      iduMutexMgr::mInfoTailEntry;
ULong              iduMutexMgr::mPoolCount;
ULong              iduMutexMgr::mPoolMaxCount;
iduPeerType        iduMutexMgr::mMutexMgrType;
iduMemSmall        iduMutexMgr::mMutexPool;
idBool             iduMutexMgr::mUsePool;


//BUG-21080
static pthread_once_t     gMutexMgrInitOnce  = PTHREAD_ONCE_INIT;
static PDL_thread_mutex_t gMutexMgrInitMutex;
static SInt               gMutexMgrInitCount;

static void iduMutexMgrInitializeOnce( void )
{
    IDE_ASSERT(idlOS::thread_mutex_init(&gMutexMgrInitMutex) == 0);

    gMutexMgrInitCount = 0;
}

IDE_RC iduMutexMgr::allocEntry(iduMutexEntry** aEntry)
{
    if(mUsePool == ID_TRUE)
    {
        IDE_TEST(mMutexPool.malloc(IDU_MEM_ID_MUTEX_MANAGER,
                                   sizeof(iduMutexEntry),
                                   (void**)aEntry) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_ID_MUTEX_MANAGER,
                                   sizeof(iduMutexEntry),
                                   (void**)aEntry) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMutexMgr::freeEntry(iduMutexEntry* aEntry)
{
    if(mUsePool == ID_TRUE)
    {
        IDE_TEST(mMutexPool.free(aEntry) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemMgr::free(aEntry) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMutexMgr::initializeStatic(iduPeerType aMutexMgrType)
{
    UInt i;
    
    //BUG-21080   
    idlOS::pthread_once(&gMutexMgrInitOnce, iduMutexMgrInitializeOnce);

    IDE_ASSERT(idlOS::thread_mutex_lock(&gMutexMgrInitMutex) == 0);

    IDE_TEST(gMutexMgrInitCount < 0);

    if (gMutexMgrInitCount == 0)
    {
        mMutexMgrType = aMutexMgrType;

        //fix PROJ-1749
        if (mMutexMgrType == IDU_SERVER_TYPE)
        {
            /* 
             * Project 2408 
             * Memory pool for mutex entries
             */
            if(iduMemMgr::isUseResourcePool() == ID_TRUE)
            {
                mUsePool = ID_TRUE;
                IDE_TEST(mMutexPool.initialize((SChar*)"MUTEX_POOL",
                                               iduProperty::getMemoryPrivatePoolSize()
                                              ) != IDE_SUCCESS);
            }
            else
            {
                mUsePool = ID_FALSE;
            }

            for (i = 0; i < IDU_MUTEX_KIND_MAX; i++)
            {
                IDE_TEST(mMutexOpArrayServer[i]->mInitializeStatic() != IDE_SUCCESS);
            }
        }
        else
        {
            mUsePool = ID_FALSE;

            if (mMutexMgrType == IDU_CLIENT_TYPE)
            {
                for (i = 0; i < IDU_MUTEX_KIND_MAX; i++)
                {
                    IDE_TEST(mMutexOpArrayClient[i]->mInitializeStatic() != IDE_SUCCESS);
                }
            }
        }

        IDE_ASSERT( mPosixEntry.create(IDU_MUTEX_KIND_POSIX) == IDE_SUCCESS );
        IDE_ASSERT( mNativeEntry.create(IDU_MUTEX_KIND_POSIX) == IDE_SUCCESS );
        IDE_ASSERT( mInfoHeadEntry.create(IDU_MUTEX_KIND_POSIX) == IDE_SUCCESS );
        IDE_ASSERT( mInfoTailEntry.create( IDU_MUTEX_KIND_POSIX ) == IDE_SUCCESS );
        IDE_ASSERT( mPosixEntry.initialize((SChar *)"MUTEX_MGR_POSIX",
                                           IDV_WAIT_INDEX_NULL,
                                           1 ) == IDE_SUCCESS );
        IDE_ASSERT( mNativeEntry.initialize((SChar *)"MUTEX_MGR_NATIVE",
                                            IDV_WAIT_INDEX_NULL,
                                            1 ) == IDE_SUCCESS );
        IDE_ASSERT( mInfoHeadEntry.initialize((SChar *)"MUTEX_MGR_INFO",
                                          IDV_WAIT_INDEX_NULL,
                                          1 ) == IDE_SUCCESS );
        IDE_ASSERT( mInfoTailEntry.initialize( (SChar *)"MUTEX_MGR_INFOFREE",
                                              IDV_WAIT_INDEX_NULL,
                                              1 ) == IDE_SUCCESS );

        mPosixEntry.setNext( &mPosixEntry );
        mPosixEntry.setIdle( ID_FALSE );
        mNativeEntry.setNext( &mNativeEntry );
        mNativeEntry.setIdle( ID_FALSE );
        mInfoHeadEntry.setNext( &mInfoHeadEntry );
        mInfoHeadEntry.setIdle( ID_FALSE );
        mInfoTailEntry.setNext( &mInfoTailEntry );
        mInfoTailEntry.setIdle( ID_FALSE );


        mPosixEntry.setNextInfo( &mNativeEntry );
        mNativeEntry.setNextInfo( &mInfoHeadEntry );
        mInfoHeadEntry.setNextInfo( &mInfoTailEntry );
        mInfoTailEntry.setNextInfo( NULL );

        mPosixEntry.setPrevInfo( NULL );
        mNativeEntry.setPrevInfo( &mPosixEntry );
        mInfoHeadEntry.setPrevInfo( &mNativeEntry );
        mInfoTailEntry.setPrevInfo( &mInfoHeadEntry );

        /*
         * BUG-45416 IDLE mutex의 정리가 필요하다.
         * __MUTEX_POOL_MAX_SIZE 를 추가하여, 설정 값 이상인 경우 mutex를 free한다.
         * freeIdles에서 스레드 동기화를 위해 mPoolCount를 atomic하게 감소 후
         * while 조건문에서 mPoolMaxCount와 비교하는데, 이 과정에서 unsigned int 형인
         * mPoolCount가 underflow를 발생시키지 않도록 초기값을 1로 설정한다.
         * */
        mPoolCount = 1;
        setPoolMaxCount( iduProperty::getMutexPoolMaxSize() );
    }
    
    gMutexMgrInitCount++;

    IDE_ASSERT(idlOS::thread_mutex_unlock(&gMutexMgrInitMutex) == 0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    {
        gMutexMgrInitCount = -1;

        IDE_ASSERT(idlOS::thread_mutex_unlock(&gMutexMgrInitMutex) == 0);

    }

    return IDE_FAILURE;
}

IDE_RC iduMutexMgr::destroyStatic()
{
    UInt    i;
    UInt    sShowMutexLeakList = 1; //1:출력   0:출력안함

    //BUG-21080 
    IDE_ASSERT(idlOS::thread_mutex_lock(&gMutexMgrInitMutex) == 0);

    IDE_TEST(gMutexMgrInitCount < 0);

    gMutexMgrInitCount--;

    sShowMutexLeakList = iduProperty::getShowMutexLeakList();

    if (gMutexMgrInitCount == 0)
    {
        /*
         * BUG-21487    Mutex Leak List출력을 property화 해야합니다.
         */
#if !defined(SMALL_FOOTPRINT) || defined(WRS_VXWORKS)
        if ( sShowMutexLeakList == 1 ) //1:출력
        {
            iduMutexEntry *sEntry;
            iduMutexEntry *sNextEntry;
    
            ideLogEntry sLog(IDE_MISC_0);
            
            sLog.append(ID_TRC_MUTEXMGR_LEAK_HEAD);
    
            i      = 0;
            sEntry = getInfo()->getNextInfo();
    
            while ( sEntry != getInfoTail() )
            {
                if( sEntry->getIdle() == ID_FALSE )
                {
                    sLog.appendFormat(ID_TRC_MUTEXMGR_LEAK_BODY, ++i, sEntry->mName);
                }

                sNextEntry = sEntry->getNextInfo();
                (void)sEntry->destroy();
                freeEntry(sEntry);
                sEntry = sNextEntry;
            }

            if(i == 0)
            {
                sLog.append(ID_TRC_MUTEXMGR_LEAK_TAIL_CLEAN);
            }
            else
            {
                sLog.appendFormat(ID_TRC_MUTEXMGR_LEAK_TAIL_LEAK, i);
            }

            sLog.write();
        }
#endif
    
        IDE_ASSERT(mInfoHeadEntry.destroy() == IDE_SUCCESS);
        IDE_ASSERT(mPosixEntry.destroy() == IDE_SUCCESS);
        IDE_ASSERT(mNativeEntry.destroy() == IDE_SUCCESS);
        IDE_ASSERT(mInfoTailEntry.destroy() == IDE_SUCCESS);

        //fix PROJ-1749
        if (mMutexMgrType == IDU_SERVER_TYPE)
        {
            for (i = 0; i < IDU_MUTEX_KIND_MAX; i++)
            {
                IDE_TEST(mMutexOpArrayServer[i]->mDestroyStatic() != IDE_SUCCESS);
            }
            
            if(mUsePool == ID_TRUE)
            {
                IDE_TEST(mMutexPool.destroy() != IDE_SUCCESS);
                mUsePool = ID_FALSE;
            }
            else
            {
                /* fall through */
            }
        }
        else
        {
            if (mMutexMgrType == IDU_CLIENT_TYPE)
            {
                for (i = 0; i < IDU_MUTEX_KIND_MAX; i++)
                {
                    IDE_TEST(mMutexOpArrayClient[i]->mDestroyStatic() != IDE_SUCCESS);
                }
            }
        }
    }
    
    IDE_ASSERT(idlOS::thread_mutex_unlock(&gMutexMgrInitMutex) == 0);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    {
        gMutexMgrInitCount = -1;

        IDE_ASSERT(idlOS::thread_mutex_unlock(&gMutexMgrInitMutex) == 0);
    }
    
    return IDE_FAILURE;
}

IDE_RC iduMutexMgr::makeNewEntry(iduMutexEntry **aEntry, iduMutexKind aKind)
{
    iduMutexEntry* sNewEntry;
    iduMutexEntry* sPrevEntry;
    
    IDE_TEST(allocEntry(&sNewEntry) != IDE_SUCCESS);
    IDE_TEST_RAISE(sNewEntry->create(aKind) != IDE_SUCCESS, ECREATEFAILED);

    if (mMutexMgrType == IDU_SERVER_TYPE)
    {
        lockInfoTail();
        sPrevEntry = mInfoTailEntry.getPrevInfo();

        sPrevEntry->setNextInfo( sNewEntry );

        sNewEntry->setPrevInfo( sPrevEntry );
        sNewEntry->setNextInfo( &mInfoTailEntry );
        
        mInfoTailEntry.setPrevInfo( sNewEntry );
        unlockInfoTail();
    }
    else
    {
        /* Do nothing */
    }

    *aEntry = sNewEntry;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ECREATEFAILED)
    {
        (void)freeEntry(sNewEntry);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMutexMgr::alloc(iduMutexEntry **aEntry, iduMutexKind aKind)
{
    iduMutexEntry* sEntry;

    sEntry = NULL;

    if (mMutexMgrType == IDU_SERVER_TYPE)
    {
        switch( aKind )
        {
        case IDU_MUTEX_KIND_POSIX:
            lockPosix();
            if(mPosixEntry.getNext() != &mPosixEntry)
            {
                sEntry = mPosixEntry.getNext();
                mPosixEntry.setNext(sEntry->getNext());
            }
            unlockPosix();
            break;
        case IDU_MUTEX_KIND_NATIVE:
        case IDU_MUTEX_KIND_NATIVE2:
        case IDU_MUTEX_KIND_NATIVE_FOR_LOGGING:
            lockNative();
            if(mNativeEntry.getNext() != &mNativeEntry)
            {
                sEntry = mNativeEntry.getNext();
                mNativeEntry.setNext(sEntry->getNext());
            }
            unlockNative();
            break;
        default:
            IDE_ASSERT(0);
            break;
        }
    }
    else
    {
        /* Just stay NULL */
    }

    if(sEntry == NULL)
    {
        /* Allocate a new entry and add it to info list */
        IDE_TEST(makeNewEntry(&sEntry, aKind) != IDE_SUCCESS);
    }
    else
    {
        acpAtomicDec64( &mPoolCount );
    }

    sEntry->setIdle(ID_FALSE);
    *aEntry = sEntry;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMutexMgr::free(iduMutexEntry *aEntry)
{
    IDE_DASSERT(aEntry != NULL);

    if (mMutexMgrType == IDU_CLIENT_TYPE)
    {
        IDE_TEST(aEntry->destroy() != IDE_SUCCESS);
        IDE_TEST(freeEntry(aEntry) != IDE_SUCCESS);
    }
    else
    {
        aEntry->finalize();
        aEntry->setIdle(ID_TRUE);
        acpAtomicInc64( &mPoolCount ); 

        switch( aEntry->mKind )
        {
            case IDU_MUTEX_KIND_POSIX:
                lockPosix();
                aEntry->setNext(mPosixEntry.getNext());
                mPosixEntry.setNext(aEntry);
                unlockPosix();
                break;
            case IDU_MUTEX_KIND_NATIVE:
            case IDU_MUTEX_KIND_NATIVE2:
            case IDU_MUTEX_KIND_NATIVE_FOR_LOGGING:
                lockNative();
                aEntry->setNext(mNativeEntry.getNext());
                mNativeEntry.setNext(aEntry);
                unlockNative();
                break;
            default:
                IDE_ASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMutexMgr::freeIdles()
{
    iduMutexEntry* sEntry;
    iduMutexEntry* sPrevEntry;
    iduMutexEntry* sNextEntry;
    sEntry = NULL;

    while ( acpAtomicDec64( &mPoolCount ) > (SLong)mPoolMaxCount ) 
    {
        /* Free */

        if ( mPosixEntry.getNext() != &mPosixEntry )
        {
            lockPosix();
            sEntry = mPosixEntry.getNext();
            mPosixEntry.setNext( sEntry->getNext() );
            unlockPosix();
        }
        else if ( mNativeEntry.getNext() != &mNativeEntry )
        {
            lockNative();
            sEntry = mNativeEntry.getNext();
            mNativeEntry.setNext(sEntry->getNext());
            unlockNative();
        }
        else
        {
            /* Do nothing */
        }

        if ( sEntry != NULL )
        {
            lockInfoHead();
            lockInfoTail();

            sPrevEntry = sEntry->getPrevInfo();
            sNextEntry = sEntry->getNextInfo();

            sNextEntry->setPrevInfo( sPrevEntry );
            sPrevEntry->setNextInfo( sNextEntry );

            unlockInfoTail();
            unlockInfoHead();

            IDE_TEST(sEntry->destroy() != IDE_SUCCESS);
            IDE_TEST(freeEntry(sEntry) != IDE_SUCCESS);
        }
        else
        {
            acpAtomicInc64(&mPoolCount); 
        }
    
        sEntry = NULL;
    }

    acpAtomicInc64(&mPoolCount); 
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

void iduMutexMgr::unlockAllMyThread()
{
    iduMutexEntry* sBase;
    iduMutexEntry* sRoot;
    ULong          sMyThreadID = idlOS::getThreadID();

    lockInfoHead();

    sRoot = getInfo();
    IDE_DASSERT(sRoot != NULL);


    for( sBase  = sRoot->getNextInfo();
         sBase != NULL;
         sBase  = sBase->getNextInfo() )
    {
        if ( sMyThreadID == sBase->getOwner() )
        {
            sBase->unlock();
        }

    }

    unlockInfoHead();
}
