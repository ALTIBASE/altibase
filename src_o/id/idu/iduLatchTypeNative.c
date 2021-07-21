/***********************************************************************
 * Copyright 1999-2000, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduLatchTypeNative.c 66837 2014-09-24 02:05:41Z returns $
 **********************************************************************/

#include <iduLatchObj.h>
#include <iduBridge.h>

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

#include "iduNativeMutex.ic"

/* ------------------------------------------------
 *  Common Operation 
 *  holdMutex()
*   sleepForNativeMutex()
 
 * ----------------------------------------------*/

#include "iduNativeMutex-COMMON.ic"

/* =============================================================================
 *
 *  Wrappers Implementations
 *
 * =========================================================================== */

static UInt gLatchSpinCount = 1;

static void initializeStaticNativeLatch(UInt aSpinCount)
{
    gLatchSpinCount = (aSpinCount == 0 ? 1 : aSpinCount);
}

static void destroyStaticNativeLatch()
{}

static void initializeNativeLatch(iduLatchObj *aLatch, SChar *aName)
{
    iduNativeMutexObj *aObj = (iduNativeMutexObj *)IDU_LATCH_CORE(aLatch);

    /* To remove compilation warining, aName is not used currently */
    (void)(aName);

    initNativeMutex(aObj);
}

/*
 *  destroy();
 *  Latch 객체를 소멸시킨다..
 */
static void destroyNativeLatch(iduLatchObj *aLatch)
{
    /* To Remove Compile Error */
    (void)(aLatch);
}

/*
 *  sleepForValueChange();
 *  Latch가 풀릴때 까지 대기
 *  대기 시간은 200 usec에서 부터 *2 로 증가하여,
 *  최대 99999 usec까지 증가한다.
 */

static void sleepForLatchValueChange(iduLatchObj *aObj,
                                     UInt         aFlag /* 0 : read 1 : write */
                                     )
{
    UInt   i;
    UInt   sStart = iduLatchMinSleep();
    UInt   sMax   = iduLatchMaxSleep();
    UInt   sType  = iduMutexSleepType();
    /* busy wait */
    while(1)
    {
        for (i = 0; i < gLatchSpinCount; i++)
        {
            if (aFlag == 0)
            {
                /*  read wait.. */
                if (aObj->mMode >= 0)
                {
                    return;
                }
            }
            else
            {
                /*  write wait */
                if (aObj->mMode == 0)
                {
                    return;
                }
            }
        }

        if(sType == IDU_MUTEX_SLEEP)
        {
            idlOS_sleep(0, sStart);
            sStart = ( (sStart * 2) > sMax) ? sMax : sStart * 2;
        }
        else
        {
            idlOS_thryield();
        }
    }
}


/*
 *  lockReadInternal();
 *  read latch를 수행하는 내부 함수이며, 외부에서 접근 불가능하다.
 *  입력된 인수에 따라 무한대기 혹은 대기없음검사를 수행하며,
 *  후자의 경우 성공/실패의 플래그를 리턴한다.
 */

static IDE_RC lockReadInternalNativeLatch (iduLatchObj *aLatch,
                                           idBool       aWaitForever,
                                           void*       aStatSQL,
                                           void*       aWeArgs,
                                           idBool      *aSuccess)
{
    iduNativeMutexObj *aObj = (iduNativeMutexObj *)IDU_LATCH_CORE(aLatch);

    UInt sOnce = 0;

    holdNativeMutex(aObj, gLatchSpinCount);

    if( (aLatch->mMode >= 0) || (aLatch->mWriteThreadID != idlOS_getThreadID()))
    {
        aLatch->mGetReadCount++;
    }
    else
    {
        /*
         * BUG-30204 만약 현재 자신이 이미 X Latch를 잡고 있는 경우
         *           잡고있던 X Latch의 Count만 추가한다.
         * X Latch는 음수로 처리
         */
        aLatch->mMode--;
        aLatch->mGetWriteCount++;
        *aSuccess = ID_TRUE;
        goto my_break;
    }

    while (aLatch->mMode < 0) /* now write latch held */
    {
        if (sOnce++ == 0)
        {
            aLatch->mReadMisses++;
        }

        if (aWaitForever == ID_TRUE)
        {
            aLatch->mSleeps++;
            releaseNativeMutex(aObj);
            if ( sOnce++ == 1 )
            {
                /* SpinLock은 Wait Event Time을 측정하지 않는다. */
                idv_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
            }
            sleepForLatchValueChange(aLatch, 0);
            holdNativeMutex(aObj, gLatchSpinCount);
            continue;
        }
        else
        {
            *aSuccess = ID_FALSE;
            goto my_break;
        }
    }

    if ( sOnce > 1 )
    {
        /* Wait Event Time 측정완료한다. */
        idv_END_WAIT_EVENT( aStatSQL, aWeArgs );
    }
    
    *aSuccess = ID_TRUE;

    aLatch->mMode++;

    IDE_DCASSERT(aLatch->mMode > 0);

  my_break:;
    releaseNativeMutex(aObj);

    return IDE_SUCCESS;
}

/*
 *  lockWriteInternal();
 *  write latch를 수행하는 내부 함수이며, 외부에서 접근 불가능하다.
 *  입력된 인수에 따라 무한대기 혹은 대기없음검사를 수행하며,
 *  후자의 경우 성공/실패의 플래그를 리턴한다.
 */
static IDE_RC lockWriteInternalNativeLatch (iduLatchObj *aLatch,
                                           idBool       aWaitForever,
                                           void        *aStatSQL,
                                           void        *aWeArgs,
                                           idBool      *aSuccess)
{
    UInt sOnce = 0;
    ULong sThreadID;
    iduNativeMutexObj *aObj = (iduNativeMutexObj *)IDU_LATCH_CORE(aLatch);

    sThreadID = idlOS_getThreadID();

    /* ---------- */

    holdNativeMutex(aObj, gLatchSpinCount);

    aLatch->mGetWriteCount++;

    /* somebody held it except me for xlock! */
    while ( (aLatch->mMode > 0) ||   /* latched on shared mode  or  */
            (aLatch->mMode < 0 &&
             (aLatch->mWriteThreadID != sThreadID)
             )
            )
    {
        if (sOnce++ == 0)
        {
            aLatch->mWriteMisses++;
        }

        if (aWaitForever == ID_TRUE)
        {
            aLatch->mSleeps++;
            releaseNativeMutex(aObj);
            if ( sOnce++ == 1 )
            {
                /* SpinLock은 Wait Event Time을 측정하지 않는다. */
                idv_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
            }
            sleepForLatchValueChange(aLatch, 1);
            holdNativeMutex(aObj, gLatchSpinCount);
        }
        else
        {
            *aSuccess = ID_FALSE;
            goto my_break;
        }
    }

    if ( sOnce > 1 )
    {
        /* Wait Event Time 측정완료한다. */
        idv_END_WAIT_EVENT( aStatSQL, aWeArgs );
    }
    *aSuccess = ID_TRUE;

    aLatch->mMode--;

    aLatch->mWriteThreadID = sThreadID;

  my_break:;

    releaseNativeMutex(aObj);

    return IDE_SUCCESS;
}

/*
 *  unlock();
 *
 *  잡은 latch를 해제한다.
 *
 *  정확한 동작을 구현하기 위해서는 lock을  성공했던 쓰레드 아이디를
 *  모두 유지하고, Unlock시에 실제로 Lock을 성공했던 쓰레드인지
 *  검사를 해야 하나, 비용이 너무 많이 들기 때문에,
 *  Write Latch의 경우에만 자신이 잡은 Latch에 대한 UnLock인지 검사하였다.
 *
 */

static IDE_RC unlockNativeLatch (iduLatchObj *aLatch)
{
    iduNativeMutexObj *aObj = (iduNativeMutexObj *)IDU_LATCH_CORE(aLatch);
    ULong sThreadID;

    sThreadID = idlOS_getThreadID();

    holdNativeMutex(aObj, gLatchSpinCount);

    IDE_DCASSERT(aLatch->mMode != 0);

    if (aLatch->mMode > 0) /* for read */
    {
        aLatch->mMode--; /*   decrease read latch count */
    }
    else /*   for write */
    {
        IDE_DCASSERT(aLatch->mMode < 0);

        IDE_DASSERT(aLatch->mWriteThreadID == sThreadID);

        aLatch->mMode++; /*   Decrease write latch count */
        if (aLatch->mMode == 0)
        {
            aLatch->mWriteThreadID = 0;
        }
    }

    releaseNativeMutex(aObj);

    return IDE_SUCCESS;
}


iduLatchFunc gNativeLatchFunc =
{
    initializeStaticNativeLatch,
    destroyStaticNativeLatch,
    initializeNativeLatch,
    destroyNativeLatch,
    lockReadInternalNativeLatch,
    lockWriteInternalNativeLatch,
    unlockNativeLatch
};




