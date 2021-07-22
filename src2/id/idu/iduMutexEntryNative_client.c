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
 * $Id$
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

static IDE_RC iduNativeInitializeStatic()
{
    /* 크기가 변할 경우 확인 요망. mMutex[128] */
    IDE_DCASSERT( ID_SIZEOF(iduNativeMutexObj) <= 128);

    return IDE_SUCCESS;
}

static IDE_RC iduNativeDestroyStatic()
{
    return IDE_SUCCESS;
}

static IDE_RC iduNativeCreate(void *aRsc)
{
    aRsc = aRsc;

    return IDE_SUCCESS;
}

static IDE_RC iduNativeInitialize(void *aRsc, UInt aBusyValue)
{
    iduNativeMutexObj *sObj = (iduNativeMutexObj *)aRsc;

    aBusyValue = aBusyValue; /* to fix compile warning */

    initNativeMutex(sObj);

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
    iduNativeMutexObj *sObj = (iduNativeMutexObj *)aRsc;

    initNativeMutex(sObj);

    return IDE_SUCCESS;
}

static void iduNativeLock(void         *aRsc,
                          iduMutexStat *aStat,
                          void         *aStatSQL,
                          void         *aWeArgs)
{
    UInt               sWaitGap = 200;
    UInt               sSpins;
    iduNativeMutexObj *sObj = (iduNativeMutexObj *)aRsc;

    ACP_UNUSED(aStatSQL);
    ACP_UNUSED(aWeArgs);

    /*  Spin Lock */
    IDU_MUTEX_STAT_INCREASE_TRY_COUNT( aStat );
    for(sSpins = 0; sSpins < aStat->mBusyValue; sSpins++)
    {
        if( IDU_NATIVEMUTEX_IS_UNLOCKED(sObj))
        {
            if (tryHoldNativeMutex(sObj) == 1)
            {
                IDU_NATIVEMUTEX_HOLD_POST_OP(sObj);
                IDU_MUTEX_STAT_INCREASE_TRY_COUNT( aStat );
                return;
            }
        }
    }

    IDU_MUTEX_STAT_INCREASE_MISS_COUNT( aStat );

    /* Wait Forever and Sleep Exponential Style */
    while(1)
    {
        idlOS_sleep(0, sWaitGap);

        sWaitGap = ( (sWaitGap * 2) > 99999) ? 99999 : sWaitGap * 2;

        if (tryHoldNativeMutex(sObj) == 1)
        {
            IDU_NATIVEMUTEX_HOLD_POST_OP(sObj);
            IDU_MUTEX_STAT_INCREASE_LOCK_COUNT( aStat );
            return;
        }
    }
}

static void iduNativeTryLock(void *aRsc, idBool *aRet, iduMutexStat *aStat)
{
    iduNativeMutexObj *sObj = (iduNativeMutexObj *)aRsc;

    IDU_MUTEX_STAT_INCREASE_TRY_COUNT( aStat );
    if (tryHoldNativeMutex(sObj) == 1)
    {
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
    iduNativeMutexObj *sObj = (iduNativeMutexObj *)aRsc;
    aStat = aStat + 0;
    releaseNativeMutex(sObj);
}

/* fix PROJ-1749 */
iduMutexOP gNativeMutexClientOps =
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

