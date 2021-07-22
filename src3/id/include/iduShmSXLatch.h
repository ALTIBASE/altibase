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

#ifndef _O_IDU_SHM_SX_LATCH_H_
#define _O_IDU_SHM_SX_LATCH_H_ 1

#include <iduShmLatch.h>

#define IDU_SHM_SX_LATCH_INFINITE_WAIT ID_ULONG_MAX

#define IDU_SHM_SX_SLATCH_CNT_INVALID ID_SINT_MAX

struct iduShmLatchInfo;

typedef enum iduShmSXLatchMode
{
    IDU_SHM_LATCH_MODE_EXCLUSIVE,
    IDU_SX_LATCH_MODE_SHARED,
    IDU_SX_LATCH_MODE_EXCLUSIVE,
    IDU_SET_SX_LATCH_MODE_SHARED,
    IDU_SET_SX_LATCH_MODE_EXCLUSIVE
} iduShmSXLatchMode;

typedef enum iduShmLatchState
{
    IDU_SHM_LATCH_STATE_NULL,
    IDU_SHM_LATCH_STATE_LOCK,
    IDU_SHM_LATCH_STATE_UNLOCKING,
    IDU_SHM_LATCH_STATE_UNLOCKED
} iduShmLatchState;

typedef struct iduShmSXLatch
{
    iduShmLatch          mLatch;
    idBool               mCheckAcquired;
    SInt                 mSharedLatchCnt;
} iduShmSXLatch;

/**
 * initializes a spinlock
 * @param aLock the spinlock object
 * @param aSpinCount spin count prior to sleep
 */
ACP_INLINE void iduShmSXLatchInit( idShmAddr       aSelf,
                                   idBool          aCheckAcquired,
                                   SInt            aSpinCount,
                                   iduShmSXLatch * aLock )
{
    iduShmLatchInit( aSelf, aSpinCount, &aLock->mLatch );
    aLock->mSharedLatchCnt    = 0;
    aLock->mCheckAcquired     = aCheckAcquired;
}

ACP_INLINE void iduShmSXLatchDest( iduShmSXLatch *aLock )
{
    IDE_ASSERT( aLock->mLatch.mLock == 0 );
    IDE_ASSERT( aLock->mSharedLatchCnt == 0 );
}


IDE_RC iduShmSXLatchAcquire( idvSQL            * aStatistics,
                             iduShmTxInfo      * aShmTxInfo,
                             iduShmSXLatch     * aSXLatch,
                             iduShmSXLatchMode   aMode,
                             ULong               aLockWaitMicroSec );

IDE_RC iduShmSXLatchRelease( idvSQL            * aStatistics,
                             iduShmTxInfo      * aShmTxInfo,
                             iduShmLatchInfo   * aLatchOPInfo,
                             iduShmSXLatchMode   aMode );

#endif  // _O_IDU_SHM_SX_LATCH_H_
