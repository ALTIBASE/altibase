/***********************************************************************
 * Copyright 1999-2000, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduLatchTypePosix.cpp 84854 2019-01-31 09:47:48Z yoonhee.kim $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <iduLatch.h>
#include <iduProperty.h>

IDE_RC iduLatchPosix::initialize(SChar *aName)
{
    IDE_ASSERT(mMutex.initialize(aName,
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL) == IDE_SUCCESS);
    mType = IDU_LATCH_TYPE_POSIX;

    IDE_TEST(iduLatchObj::initialize(mMutex.mEntry->mName) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduLatchPosix::destroy()
{
    IDE_ASSERT(mMutex.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

IDE_RC iduLatchPosix::tryLockRead(idBool* aSuccess, void* aStatSQL)
{
    IDE_TEST_RAISE(mMutex.lock( (idvSQL *)aStatSQL ) != IDE_SUCCESS, mutex_lock_error);

    if(mMode >= 0)
    {
        /* one more S latch */
        mMode++;
        mGetReadCount++;
        *aSuccess = ID_TRUE;
    }
    else
    {
        if(mWriteThreadID == idlOS::getThreadID())
        {
            /*
             * This thread holds X latch
             * negative mode value represents X latch
             */
            mMode--;
            mGetWriteCount++;
            *aSuccess = ID_TRUE;
        }
        else
        {
            /* Other thread holds X latch */
            mReadMisses++;
            *aSuccess = ID_FALSE;
        }
    }

    IDE_TEST_RAISE(mMutex.unlock() != IDE_SUCCESS, mutex_unlock_error);
    return IDE_SUCCESS;

    IDE_EXCEPTION(mutex_lock_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduLatchPosix::tryLockWrite(idBool* aSuccess, void* aStatSQL)
{
    ULong sThreadID = idlOS::getThreadID();

    IDE_TEST_RAISE(mMutex.lock( (idvSQL *)aStatSQL ) != IDE_SUCCESS, mutex_lock_error);

    if(mMode == 0)
    {
        /* I am.... your owner */
        mMode = -1;
        mGetWriteCount++;
        mWriteThreadID = sThreadID;
        *aSuccess = ID_TRUE;
    }
    else
    {
        if(mMode > 0)
        {
            *aSuccess = ID_FALSE;
            mWriteMisses++;
        }
        else
        {
            if(mWriteThreadID == sThreadID)
            {
                mMode--;
                *aSuccess = ID_TRUE;
            }
            else
            {
                mWriteMisses++;
                *aSuccess = ID_FALSE;
            }
        }
    }

    IDE_TEST_RAISE(mMutex.unlock() != IDE_SUCCESS, mutex_unlock_error);
    return IDE_SUCCESS;

    IDE_EXCEPTION(mutex_lock_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduLatchPosix::lockRead(void*  aStatSQL, void* aWeArgs)
{
    idBool  sSuccess;

    IDE_TEST(tryLockRead(&sSuccess, aStatSQL) != IDE_SUCCESS);

    if(sSuccess == ID_FALSE)
    {
        IDV_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
        do
        {
            mSleepCount++;
            /* wait for write latch unlock */
            idlOS::thr_yield();
            IDE_TEST(tryLockRead(&sSuccess, NULL) != IDE_SUCCESS);
        } while(sSuccess == ID_FALSE);
        /* 대기이벤트 Wait Time 측정을 완료한다. */
        IDV_END_WAIT_EVENT( aStatSQL, aWeArgs );
    
        IDE_ASSERT(mMode > 0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduLatchPosix::lockWrite(void* aStatSQL, void* aWeArgs)
{
    idBool  sSuccess;

    IDE_TEST(tryLockWrite(&sSuccess, aStatSQL) != IDE_SUCCESS);

    if(sSuccess == ID_FALSE)
    {
        IDV_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
        do
        {
            /* wait for write latch unlock */
            mSleepCount++;
            idlOS::thr_yield();
            IDE_TEST(tryLockWrite(&sSuccess, NULL) != IDE_SUCCESS);
        } while(sSuccess == ID_FALSE);
        /* 대기이벤트 Wait Time 측정을 완료한다. */
        IDV_END_WAIT_EVENT( aStatSQL, aWeArgs );

        IDE_ASSERT(mMode < 0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduLatchPosix::unlock( idBool *aIsUnlockedAll )
{
    IDE_TEST_RAISE(mMutex.lock(NULL) != IDE_SUCCESS, mutex_lock_error);

    IDE_ASSERT(mMode != 0);

    if (mMode > 0) /* for read */
    {
        mMode--; // decrease read latch count
    }
    else // for write
    {
        IDE_ASSERT(mMode < 0);
        IDE_ASSERT(mWriteThreadID == idlOS::getThreadID());

        mMode++; // Decrease write latch count

        if (mMode == 0)
        {
            if ( aIsUnlockedAll != NULL )
            {
                *aIsUnlockedAll = ID_TRUE;
            }
            mWriteThreadID = 0;
        }
        else
        {
            /* Still I'm holding this latch */
        }
    }

    IDE_TEST_RAISE(mMutex.unlock() != IDE_SUCCESS, mutex_unlock_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(mutex_lock_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduLatchPosix::unlockWriteAll()
{
    idBool sIsUnlockedAll = ID_FALSE;

    do
    {
        IDE_TEST( unlock( &sIsUnlockedAll ) != IDE_SUCCESS );
    }
    while ( sIsUnlockedAll != ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

