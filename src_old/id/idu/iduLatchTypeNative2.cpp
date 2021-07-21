/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id: iduLatchTypeNative.cpp -1   $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <iduLatch.h>

/* =============================================================================
 *
 *  Primitive Mutex Implementations
 *
 *  => 모든 플랫폼은 아래의 3개의 Define과 3개의 함수를 구현하면,
 *     자동으로 Latch의 동작이 구현되도록 되어 있다.
 *  Wrappers Implementations
 *
 *  - Used Symbol
 *    1. iduNativeMutexObj : Primitive Mutex Values
 *    2. IDU_LATCH_IS_UNLOCKED(a) : whether the mutex is unlocked.
 *    3. IDU_HOLD_POST_OPERATION(a) : sync op after hold mutex.
 *    4. initNativeMutex();     init value
 *    5. tryHoldPricMutex()   try Hold Native Mutex
 *    6. releaseNativeMutex();  Release Native Mutex
 *
 * =========================================================================== */

/* =============================================================================
 *
 *  Wrappers Implementations
 *
 * =========================================================================== */

IDE_RC iduLatchNative2::initialize(SChar *aName)
{
    SChar  sEnvName[128];
    SChar* sEnvValue;

    (void)idlOS::strcpy(mLatchName, aName);
    mName = mLatchName;
    mType = IDU_LATCH_TYPE_NATIVE2;
    IDE_TEST(iduLatchObj::initialize(mLatchName));

    /* get latch spin count */
    (void)idlOS::snprintf(sEnvName, sizeof(sEnvName), "%s_SPINCOUNT", aName);
    sEnvValue = idlOS::getenv(sEnvName);

    if(sEnvValue == NULL)
    {
        mSpinCount = iduLatch::mLatchSpinCount;
    }
    else
    {
        mSpinCount = idlOS::atoi(sEnvValue);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduLatchNative2::destroy()
{
    return IDE_SUCCESS;
}

IDE_RC iduLatchNative2::tryLockRead(idBool* aSuccess, void*)
{
    SInt    sOldMode;
    SInt    sNewMode;
    SInt    sCurMode;
                
    *aSuccess = ID_FALSE;

    if(mMode >= 0)
    {
        do
        {
            sCurMode = mMode;
            sNewMode = sCurMode + 1;
            sOldMode = acpAtomicCas32(&mMode, sNewMode, sCurMode);

            if(sOldMode == sCurMode)
            {
                /* CAS successful. S latch got */
                (void)acpAtomicInc64(&mGetReadCount);
                *aSuccess = ID_TRUE;
            }
            else
            {
                /* *aSuccess = ID_FALSE; */
            }
        } while(*aSuccess == ID_FALSE);
    }
    else
    {
        if(mWriteThreadID == idlOS::getThreadID())
        {
            /*
             * Current thread holds X latch
             */
            mMode--;
            mGetWriteCount++;
            *aSuccess = ID_TRUE;
        }
        else
        {
            /* *aSuccess = ID_FALSE; */
        }
    }
    IDL_MEM_BARRIER;

    return IDE_SUCCESS;
}

IDE_RC iduLatchNative2::tryLockWrite(idBool* aSuccess, void*)
{
    ULong sThreadID = idlOS::getThreadID();

    if(mMode < 0 && mWriteThreadID == sThreadID)
    {
        /* This thread is holding X latch */
        mMode--;
        mGetWriteCount++;
        *aSuccess = ID_TRUE;
    }
    else
    {
        if(mMode == 0)
        {
            if(acpAtomicCas32(&mMode, -1, 0) == 0)
            {
                /*
                 * CAS successful
                 * Now, this thread is holding X latch
                 */
                mGetWriteCount++;
                mWriteThreadID = sThreadID;

                *aSuccess = ID_TRUE;
            }
            else
            {
                *aSuccess = ID_FALSE;
            }
        }
        else
        {
            /* Do not try */
        }
    }
    IDL_MEM_BARRIER;

    return IDE_SUCCESS;
}

IDE_RC iduLatchNative2::lockRead(void*  aStatSQL, void* aWeArgs)
{
    idBool  sSuccess;

    (void)tryLockRead(&sSuccess, NULL);

    if(sSuccess == ID_FALSE)
    {
        acpAtomicInc64(&mReadMisses);

        IDV_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
        do
        {
            acpAtomicInc64(&mSleepCount);
            sleepForLatchValueChange(0);
            (void)tryLockRead(&sSuccess, NULL);
        } while(sSuccess != ID_TRUE);
        /* 대기이벤트 Wait Time 측정을 완료한다. */
        IDV_END_WAIT_EVENT( aStatSQL, aWeArgs );
        IDE_ASSERT(mMode > 0);
    }
    else
    {
        /* Do nothing */
    }

    return IDE_SUCCESS;
}

IDE_RC iduLatchNative2::lockWrite(void* aStatSQL, void* aWeArgs)
{
    idBool  sSuccess;

    (void)tryLockWrite(&sSuccess, NULL);

    if(sSuccess == ID_FALSE)
    {
        (void)acpAtomicInc64(&mWriteMisses);

        IDV_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
        do
        {
            acpAtomicInc64(&mSleepCount);
            sleepForLatchValueChange(1);
            (void)tryLockWrite(&sSuccess, NULL);
        } while(sSuccess != ID_TRUE);
        /* 대기이벤트 Wait Time 측정을 완료한다. */
        IDV_END_WAIT_EVENT( aStatSQL, aWeArgs );
        IDE_ASSERT(mMode < 0);
    }
    else
    {
        /* Do nothing */
    }
    return IDE_SUCCESS;
}

IDE_RC iduLatchNative2::unlock( idBool* aIsUnlockedAll )
{
    IDE_DASSERT(mMode != 0);

    if (mMode > 0) /* for read */
    {
        (void)acpAtomicDec32(&mMode); /*   decrease read latch count */
    }
    else /*   for write */
    {
        IDE_ASSERT(mMode < 0);
        IDE_ASSERT(mWriteThreadID == idlOS::getThreadID()); 

        mMode++; /*   Decrease write latch count */
        if (mMode == 0)
        {
            if ( aIsUnlockedAll != NULL )
            {
                *aIsUnlockedAll = ID_TRUE;
            }

            mWriteThreadID = 0;
        }
        IDL_MEM_BARRIER;
    }
    return IDE_SUCCESS;
}

IDE_RC iduLatchNative2::unlockWriteAll()
{
    idBool sIsUnlockedAll = ID_FALSE;

    do
    {
        unlock( &sIsUnlockedAll );
    }
    while ( sIsUnlockedAll != ID_TRUE );

    return IDE_SUCCESS;
}

/*
 *  sleepForValueChange();
 *  Latch가 풀릴때 까지 대기
 *  대기 시간은 200 usec에서 부터 *2 로 증가하여,
 *  최대 99999 usec까지 증가한다.
 */

/* 0 : Read, 1 : write */
void iduLatchNative2::sleepForLatchValueChange(SInt aFlag)
{
    SInt   i;
    UInt   sStart = iduProperty::getLatchMinSleep();
    UInt   sMax   = iduProperty::getLatchMaxSleep();
    UInt   sType  = iduProperty::getMutexSleepType();
    PDL_Time_Value      sTimeOut;

    /* busy wait */
    while(1)
    {
        for (i = 0; i < mSpinCount; i++)
        {
            if (aFlag == 0)
            {
                /*  read wait.. */
                if (mMode >= 0)
                {
                    return;
                }
            }
            else
            {
                /*  write wait */
                if (mMode == 0)
                {
                    return;
                }
            }
        }

        if(sType == IDU_MUTEX_SLEEP)
        {
            sTimeOut.set(0, sStart);
            idlOS::sleep(sTimeOut);
            sStart = ( (sStart * 2) > sMax) ? sMax : sStart * 2;
        }
        else
        {
            idlOS::thr_yield();
        }
    }
}

