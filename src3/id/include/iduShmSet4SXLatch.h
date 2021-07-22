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

#ifndef _O_IDU_SHM_SET_4_SX_LATCH_H_
#define _O_IDU_SHM_SET_4_SX_LATCH_H_ 1

#include <iduShmLatch.h>

#define IDU_SHM_SX_LATCH_INFINITE_WAIT      ID_ULONG_MAX

struct iduShmLatchInfo;

// Shared Memory SXLatch를 정의
typedef struct iduShmSet4SXLatch
{
    // Shared Memory Latch의 Self Address
    idShmAddr       mAddrSelf;
    // Shared Memory Latch의 Array의 시작 Address
    idShmAddr       mAddr4ArrLatch;
    // SX SetLatch에 존재하는 Latch의 갯수
    UInt            mLatchCnt;
} iduShmSet4SXLatch;

IDE_RC iduShmSet4SXLatchInit( idvSQL               * aStatistics,
                              iduShmTxInfo         * aShmTxInfo,
                              iduMemoryClientIndex   aIndex,
                              idShmAddr              aSelf,
                              UInt                   aLatchCnt,
                              UInt                   aSpinCount,
                              iduShmSet4SXLatch    * aLock );

IDE_RC iduShmSet4SXLatchDest( idvSQL               * aStatistics,
                              iduShmTxInfo         * aShmTxInfo,
                              iduShmSet4SXLatch    * aLock );

IDE_RC iduShmSet4SXLatchAcquire( idvSQL              * aStatistics,
                                 iduShmTxInfo        * aShmTxInfo,
                                 iduShmSet4SXLatch   * aSet4SXLatch,
                                 UInt                  aLatchIdx,
                                 iduShmSXLatchMode     aMode,
                                 ULong                 aLockWaitMicroSec );

IDE_RC iduShmSet4SXLatchRelease( idvSQL              * aStatistics,
                                 iduShmTxInfo        * aShmTxInfo,
                                 iduShmLatchInfo     * aLatchOPInfo );

#endif  // _O_IDU_SHM_SET_4_SX_LATCH_H_
