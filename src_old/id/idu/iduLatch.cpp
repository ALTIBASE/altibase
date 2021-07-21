/***********************************************************************
 * Copyright 1999-2000, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduLatch.cpp 84854 2019-01-31 09:47:48Z yoonhee.kim $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <iduProperty.h>
#include <iduLatch.h>

/*
 *       initializeStatic();
 *
 *   다중 CPU 환경에서만 LATCH 인자 처리함.
 *   단일 CPU 환경에서는 spin lock이 의미가 없음.
 *
 *   그  값은 아래의 환경변수에서 얻는다.
 *
 *   ALTIBASE_LATCH_SPINLOCK_COUNT = VALUE;
 *  Latch 객체를 초기화한다.
 */

SInt            iduLatch::mLatchSpinCount;
iduPeerType     iduLatch::mPeerType = IDU_CLIENT_TYPE;
iduMutex        iduLatch::mIdleLock[IDU_LATCH_TYPE_MAX];
iduLatchObj*    iduLatch::mIdle[IDU_LATCH_TYPE_MAX];
iduMutex        iduLatch::mInfoLock;
iduLatchObj*    iduLatch::mInfo;
iduMemSmall     iduLatch::mLatchPool;
idBool          iduLatch::mUsePool;

IDE_RC iduLatch::initializeStatic(iduPeerType aType)
{
    SInt i;

    if (idlVA::getProcessorCount() <= 1)
    {
        /* 단일 CPU에서는 1로 고정. */
        mLatchSpinCount = 1;
    }
    else
    {
        mLatchSpinCount = iduProperty::getLatchSpinCount();

        if(mLatchSpinCount == 0)
        {
            mLatchSpinCount = 1;
        }
    }

    mPeerType = aType;


    if(mPeerType == IDU_SERVER_TYPE)
    {
        if(iduMemMgr::isUseResourcePool() == ID_TRUE)
        {
            mUsePool = ID_TRUE;

            IDE_TEST_RAISE(
                mLatchPool.initialize((SChar*)"LATCH_POOL",
                                      iduProperty::getMemoryPrivatePoolSize())
                != IDE_SUCCESS, ENOTENOUGHMEMORY);
        }
        else
        {
            mUsePool = ID_FALSE;
        }
        /*
         * Initialize Allocation and information mutexes
         */
        for(i = (SInt)IDU_LATCH_TYPE_BEGIN;
            i < (SInt)IDU_LATCH_TYPE_MAX;
            i++)
        {
            IDE_TEST_RAISE(
                allocLatch(sizeof(iduLatchObj), &(mIdle[i])) != IDE_SUCCESS,
                ENOTENOUGHMEMORY);
            IDE_TEST(mIdleLock[i].initialize("LATCH_MGR",
                                             IDU_MUTEX_KIND_POSIX,
                                             IDV_WAIT_ID_SYSTEM)
                     != IDE_SUCCESS);
            mIdle[i]->mIdleLink = NULL;
        }

        IDE_TEST(mInfoLock.initialize("LATCH_INFO",
                                      IDU_MUTEX_KIND_POSIX,
                                      IDV_WAIT_ID_SYSTEM)
                 != IDE_SUCCESS);
        IDE_TEST_RAISE(allocLatch(sizeof(iduLatchObj), &mInfo)
                       != IDE_SUCCESS,
                       ENOTENOUGHMEMORY);
        mInfo->mInfoLink = NULL;
    }
    else
    {
        /* No need of init */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ENOTENOUGHMEMORY)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_MEMORY_ALLOCATION,
                                "iduLatch::initializeStatic"));
    }

    IDE_EXCEPTION_END;
    IDE_ASSERT(0);
    return IDE_FAILURE;
}

/*
 *  destroyStatic();
 *  Latch 객체를 소멸시킨다..
 */
IDE_RC iduLatch::destroyStatic()
{
    SInt i;

    if(mPeerType == IDU_SERVER_TYPE)
    {
        ideLogEntry     sLog(IDE_MISC_0);
        iduLatchObj*    sRoot;
        iduLatchObj*    sBase;
        iduLatchObj*    sNext;

        sRoot = (iduLatchObj*)iduLatch::getFirstInfo();
        IDE_DASSERT(sRoot != NULL);
        sBase = (iduLatchObj*)sRoot->getNextInfo();

        sLog.append(ID_TRC_LATCH_LEAK_HEAD);
        /*
         * Free latches
         */
        i = 0;
        do
        {
            sNext = (iduLatchObj*)sBase->getNextInfo();
            if(sBase->mMode != 0)
            {
                /*
                 * Not an idle latch
                 */
                sLog.appendFormat(ID_TRC_LATCH_LEAK_BODY,
                                  i++,
                                  sBase->mName);

            }
            else
            {
                /* Idle latch */
            }
            IDE_TEST(sBase->destroy() != IDE_SUCCESS);
            IDE_TEST(freeLatch(sBase) != IDE_SUCCESS);
            sBase = sNext;
        } while(sBase != NULL);

        if(i == 0)
        {
            sLog.append(ID_TRC_LATCH_LEAK_TAIL_CLEAN);
        }
        else
        {
            sLog.appendFormat(ID_TRC_LATCH_LEAK_TAIL_LEAK, i);
        }
        
        sLog.write();

        /*
         * Destroy idle link and mutexes
         */
        for(i = (SInt)IDU_LATCH_TYPE_BEGIN;
            i < (SInt)IDU_LATCH_TYPE_MAX;
            i++)
        {
            IDE_TEST(freeLatch(mIdle[i]) != IDE_SUCCESS);
            IDE_TEST(mIdleLock[i].destroy() != IDE_SUCCESS);
        }

        IDE_TEST(mInfoLock.destroy() != IDE_SUCCESS);
        IDE_TEST(freeLatch(mInfo) != IDE_SUCCESS);

        if(mUsePool == ID_TRUE)
        {
            IDE_TEST(mLatchPool.destroy() != IDE_SUCCESS);
            mUsePool = ID_FALSE;
        }
        else
        {
            /* Pool not used */
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 *  initialize();
 *  Latch 객체를 초기화한다.
 */

IDE_RC iduLatch::initialize(SChar *aName)
{
    return iduLatch::initialize(aName,
                                (iduLatchType)iduProperty::getLatchType());
}

IDE_RC iduLatch::initialize(SChar       *aName,
                            iduLatchType aLatchType)
{
    IDE_DASSERT( aLatchType < IDU_LATCH_TYPE_MAX );

#if defined(ALTIBASE_USE_VALGRIND)
    aLatchType = IDU_LATCH_TYPE_POSIX;
#endif

    /* Pop a latch with appropriate type */
    if(mPeerType == IDU_SERVER_TYPE)
    {
        IDE_TEST(mIdleLock[aLatchType].lock(NULL) != IDE_SUCCESS);

        if(mIdle[aLatchType]->mIdleLink == NULL)
        {
            mLatch = NULL;
        }
        else
        {
            mLatch = (iduLatchObj*)mIdle[aLatchType]->mIdleLink;
            mIdle[aLatchType]->mIdleLink = mLatch->mIdleLink;
        }
        IDE_TEST(mIdleLock[aLatchType].unlock() != IDE_SUCCESS);
    }
    else
    {
        mLatch = NULL;
    }

    if(mLatch == NULL)
    {
        switch(aLatchType)
        {
        case IDU_LATCH_TYPE_POSIX:
            IDE_TEST(allocLatch(sizeof(iduLatchPosix),
                                &mLatch) != IDE_SUCCESS);
            mLatch = new(mLatch) iduLatchPosix;
            break;
        case IDU_LATCH_TYPE_POSIX2:
            IDE_TEST(allocLatch(sizeof(iduLatchPosix2),
                                &mLatch) != IDE_SUCCESS);
            mLatch = new(mLatch) iduLatchPosix2;
            break;
        case IDU_LATCH_TYPE_NATIVE:
            IDE_TEST(allocLatch(sizeof(iduLatchNative),
                                &mLatch) != IDE_SUCCESS);
            mLatch = new(mLatch) iduLatchNative;
            break;
        case IDU_LATCH_TYPE_NATIVE2:
            IDE_TEST(allocLatch(sizeof(iduLatchNative2),
                                &mLatch) != IDE_SUCCESS);
            mLatch = new(mLatch) iduLatchNative2;
            break;
        default:
            IDE_ASSERT(0);
            break;
        }

        if(mPeerType == IDU_SERVER_TYPE)
        {
            IDE_TEST_RAISE(mInfoLock.lock(NULL) != IDE_SUCCESS, EFREE);
            mLatch->mInfoLink = mInfo->mInfoLink;
            mInfo->mInfoLink = mLatch;
            IDE_TEST_RAISE(mInfoLock.unlock() != IDE_SUCCESS, EFREE);
        }
        else
        {
            /* do not link to information links */
        }
    }
    else
    {
        /* do nothing */
    }

    mLatch->initialize(aName);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EFREE)
    {
        (void)freeLatch(mLatch);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 *  destroy(); 
 *  Server Mode : Latch 객체를 IDLE 리스트에 넣는다.
 *  Client Mode : Latch 객체를 소멸시킨다..
 */
IDE_RC iduLatch::destroy()
{
    /* Make sure this latch is idle */
    IDE_ASSERT(mLatch->mMode == 0);
    mLatch->mMode = 0;
    /* mLatch->mWriteThreadID = 0; */
    
    if(mPeerType == IDU_SERVER_TYPE)
    {
        IDE_TEST(mIdleLock[mLatch->mType].lock(NULL) != IDE_SUCCESS);
        mLatch->mIdleLink = mIdle[mLatch->mType]->mIdleLink;
        mIdle[mLatch->mType]->mIdleLink = mLatch;
        IDE_TEST(mIdleLock[mLatch->mType].unlock() != IDE_SUCCESS);
    }
    else
    {
        /* just free in client mode */
        freeLatch(mLatch);
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/* --------------------------------------------------------------------
 * latch를 dump한다.
 * ----------------------------------------------------------------- */
IDE_RC iduLatch::dump()
{
#define IDE_FN "iduLatch::dump()"
    ideLogEntry  sLog(IDE_SERVER_0);

    IDE_ERROR( mLatch != NULL );

    sLog.append( "----------- Latch Begin ----------" );

    sLog.appendFormat( "Mutex\t: %"ID_xPOINTER_FMT"\n",
                       mLatch );

    sLog.appendFormat( "Mode\t: %d\n",
                       mLatch->mMode );

    sLog.appendFormat( "WriteThreadID\t: %llu\n",
                       mLatch->mWriteThreadID );

    sLog.append( "----------- Latch End ----------" );

    sLog.write();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC iduLatch::allocLatch(ULong aSize, iduLatchObj** aPtr)
{
    if(mUsePool == ID_TRUE)
    {
        IDE_TEST(mLatchPool.malloc(IDU_MEM_ID_LATCH, aSize, (void**)aPtr)
                 != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_ID_LATCH, aSize, (void**)aPtr)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduLatch::freeLatch(iduLatchObj* aPtr)
{
    if(mUsePool == ID_TRUE)
    {
        IDE_TEST(mLatchPool.free(aPtr) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemMgr::free(aPtr) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void iduLatch::unlockWriteAllMyThread()
{
    iduLatchObj*    sBase;
    iduLatchObj*    sRoot;
    ULong           sMyThreadID = idlOS::getThreadID();

    sRoot = (iduLatchObj*)iduLatch::getFirstInfo();
    IDE_DASSERT( sRoot != NULL );

    for( sBase  = (iduLatchObj*)sRoot->getNextInfo();
         sBase != NULL;
         sBase  = (iduLatchObj*)sBase->getNextInfo() )
    {
        if( sMyThreadID == sBase->mWriteThreadID )
        {
            sBase->unlockWriteAll();
        }
    }
}
