/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutexEntryNative.c 68698 2015-01-28 02:32:20Z djin $
 **********************************************************************/

#include <acp.h>
#include <iduBridge.h>
#include <iduMutexObj.h>
#include <iduMutexRes.h>

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
 *    2. IDU_NATIVEMUTEX_IS_UNLOCKED(a) : whether the mutex is unlocked.
 *    3. IDU_NATIVEMUTEX_HOLD_POST_OP(a) : sync op after hold mutex.
 *    4. initNativeMutex();     init value
 *    5. tryHoldNativeMutex()   try Hold Native Mutex
 *    6. releaseNativeMutex();  Release Native Mutex
 *
 * =========================================================================== */

#if defined(ALTI_CFG_OS_WINDOWS)
#include "iduNativeMutex-X86-WIN.ic"
#else
#include "iduNativeMutex.ic"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

static void iduNativeProfileLockedTime(iduMutexResNative * aMutexRes )
{
    if ( idv_TIME_AVAILABLE() )
    {
        idv_TIME_GET( (iduBridgeTime*)(&aMutexRes->mLastLockedTime) );
        aMutexRes->mIsLockTimeValid = ID_TRUE;
    }
}

static void iduNativeProfileUnlockedTime(iduMutexResNative * aMutexRes,
                                         iduMutexStat *aStat)
{
    iduBridgeTime sUnlockTime ;
    ULong         sLockedUS;

    if ( idv_TIME_AVAILABLE() )
    {
        if ( aMutexRes->mIsLockTimeValid == ID_TRUE )
        {
            idv_TIME_GET( & sUnlockTime );

            sLockedUS = idv_TIME_DIFF_MICRO( (iduBridgeTime*)(&aMutexRes->mLastLockedTime),
                                             (iduBridgeTime*)(&sUnlockTime) );

            aStat->mTotalLockTimeUS += sLockedUS;

            if ( sLockedUS > aStat->mMaxLockTimeUS )
            {
                aStat->mMaxLockTimeUS = sLockedUS ;
            }
        }
    }
}

static IDE_RC iduNativeInitializeStatic()
{
    IDE_DCASSERT( idv_SizeOf_IdvTime() <= ID_SIZEOF(iduBridgeTime) );

    return IDE_SUCCESS;
}

static IDE_RC iduNativeDestroyStatic()
{
    return IDE_SUCCESS;
}

static IDE_RC iduNativeCreate(void *aRsc)
{
    ACP_UNUSED(aRsc);

    return IDE_SUCCESS;
}

static IDE_RC iduNativeInitialize(void *aRsc, UInt aBusyValue)
{
    iduMutexResNative * sMutexRes = (iduMutexResNative *)aRsc;

    aBusyValue = aBusyValue; /* To Prevent Compiler Warning */

    initNativeMutex( & sMutexRes->mMutexObj );

    sMutexRes->mIsLockTimeValid = ID_FALSE;

    return IDE_SUCCESS;
}

static IDE_RC iduNativeFinalize(void* aRsc)
{
    iduMutexResNative * sMutexRes = (iduMutexResNative *)aRsc;

    initNativeMutex( & sMutexRes->mMutexObj );

    return IDE_SUCCESS;
}

static IDE_RC iduNativeDestroy(void *aRsc)
{
    iduMutexResNative * sMutexRes = (iduMutexResNative *)aRsc;

    initNativeMutex( & sMutexRes->mMutexObj );

    return IDE_SUCCESS;
}

static void iduNativeLock( void         * aRsc,
                           iduMutexStat * aStat,
                           void         * aStatSQL,
                           void         * aWeArgs )
{
    UInt                sWaitGap = iduLatchMinSleep();
    UInt                sMax     = iduLatchMaxSleep();
    UInt                sType    = iduMutexSleepType();
    UInt                sSpins;

    iduMutexResNative * sMutexRes = (iduMutexResNative *)aRsc;
    iduNativeMutexObj * sObj = & sMutexRes->mMutexObj;

    /*  Spin Lock */
    IDU_MUTEX_STAT_INCREASE_TRY_COUNT( aStat );

    for(sSpins = 0; sSpins < aStat->mBusyValue; sSpins++)
    {
        if( IDU_NATIVEMUTEX_IS_UNLOCKED(sObj))
        {
            if (tryHoldNativeMutex(sObj) == 1)
            {
                IDU_NATIVEMUTEX_HOLD_POST_OP(sObj);
                IDU_MUTEX_STAT_INCREASE_LOCK_COUNT( aStat );
                goto locked2;
            }
        }
    }

    /* SpinLock은 Wait Event Time을 측정하지 않는다. */
    idv_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
    IDU_MUTEX_STAT_INCREASE_MISS_COUNT( aStat );

    /* Wait Forever and Sleep Exponential Style */
    while(1)
    {
        if(sType == IDU_MUTEX_SLEEP)
        {
            idlOS_sleep(0, sWaitGap);
            sWaitGap = ( (sWaitGap * 2) > sMax) ? sMax : sWaitGap * 2;
        }
        else
        {
            idlOS_thryield();
        }


        if (tryHoldNativeMutex(sObj) == 1)
        {
            IDU_NATIVEMUTEX_HOLD_POST_OP(sObj);
            IDU_MUTEX_STAT_INCREASE_LOCK_COUNT( aStat );
            goto locked1;
        }
    }

locked1:
    idv_END_WAIT_EVENT( aStatSQL, aWeArgs );

locked2:
    if ( iduBridge_getCheckMutexDurationTimeEnable() == 1 )
    {
        iduNativeProfileLockedTime( sMutexRes );
    }

}

static void iduNativeTryLock( void         * aRsc,
                              idBool       * aRet,
                              iduMutexStat * aStat )
{
    iduMutexResNative * sMutexRes = (iduMutexResNative *)aRsc;
    iduNativeMutexObj * sObj = & sMutexRes->mMutexObj;

    IDU_MUTEX_STAT_INCREASE_TRY_COUNT( aStat );

    if (tryHoldNativeMutex(sObj) == 1)
    {
        if ( iduBridge_getCheckMutexDurationTimeEnable() == 1 )
        {
            iduNativeProfileLockedTime( sMutexRes );
        }

        IDU_MUTEX_STAT_INCREASE_LOCK_COUNT( aStat );
        *aRet = ID_TRUE;
    }
    else
    {
        IDU_MUTEX_STAT_INCREASE_MISS_COUNT( aStat );
        *aRet = ID_FALSE;
    }
}

static void iduNativeUnlock(void *aRsc, iduMutexStat * aStat)
{
    iduMutexResNative * sMutexRes = (iduMutexResNative *)aRsc;
    iduNativeMutexObj * sObj = & sMutexRes->mMutexObj;

    aStat = aStat; /* To Prevent Compiler Warning */

    if ( iduBridge_getCheckMutexDurationTimeEnable() ==1 )
    {
        iduNativeProfileUnlockedTime( sMutexRes, aStat );
    }

    sMutexRes->mIsLockTimeValid = ID_FALSE;

    releaseNativeMutex(sObj);
}

/* fix PROJ-1749 */
iduMutexOP gNativeMutexServerOps =
{
    iduNativeInitializeStatic,
    iduNativeDestroyStatic,
    iduNativeCreate,
    iduNativeInitialize,
    iduNativeFinalize,
    iduNativeDestroy,
    iduNativeLock,
    iduNativeTryLock,
    iduNativeUnlock
};

#if defined(__cplusplus)
}
#endif
