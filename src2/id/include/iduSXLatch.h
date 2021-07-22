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

#ifndef _O_IDU_SX_LATCH_H_
#define _O_IDU_SX_LATCH_H_ 1

#include <iduShmLatch.h>

#define IDU_SHM_SX_LATCH_INFINITE_WAIT ID_ULONG_MAX

struct iduShmLatchInfo;

#define iduSXLatchMode iduShmSXLatchMode

typedef struct iduSXLatch
{
    volatile SInt     mLock;
    SInt              mSpinCount;
    SInt              mSharedLatchCnt;
} iduSXLatch;

/**
 * initializes a spinlock
 * @param aLock the spinlock object
 * @param aSpinCount spin count prior to sleep
 */
ACP_INLINE void iduSXLatchInit( SInt            aSpinCount,
                                iduSXLatch    * aLatch )
{
    aLatch->mLock           = 0;
    aLatch->mSpinCount      = aSpinCount;
    aLatch->mSharedLatchCnt = 0;
}

ACP_INLINE void iduSXLatchDest( iduSXLatch *aLatch )
{
    IDE_ASSERT( aLatch->mLock == 0 );
    IDE_ASSERT( aLatch->mSharedLatchCnt == 0 );
}

void iduSXLatchAcquire( iduSXLatch     * aSXLatch,
                        iduSXLatchMode   aMode );

void iduSXLatchRelease( iduSXLatch     * aSXLatch );

#endif  // _O_IDU_SX_LATCH_H_
