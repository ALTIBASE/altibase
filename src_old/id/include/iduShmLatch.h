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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDU_SHM_LATCH_H_)
#define _O_IDU_SHM_LATCH_H_

/**
 * @file
 * @ingroup CoreAtomic
 * @ingroup CoreThr
 */
#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idTypes.h>
#include <idCore.h>

struct iduShmTxInfo;

typedef struct iduShmLatchStat
{
    UInt   mTryCount;        /* How Many Time Tried on BusyWait Loop. */
    UInt   mLockCount;       /* Plus when Lock Success  */
    UInt   mMissCount;       /* Plus When First Lock Miss */
    UInt   mBusyValue;       /* Spin Lock Count */
    ULong  mTotalLockTimeUS; /* Total time of Interval of lock() ~ unlock()
                                In micro seconds */
    ULong  mMaxLockTimeUS;   /* Max time of Interval of lock() ~ unlock()
                                In micro seconds */
} iduShmLatchStat;

/**
 * spinlock object
 */
typedef struct iduShmLatch
{
    idShmAddr       mAddrSelf;
    volatile SInt   mLock;
    UInt            mRecursive; /* Recursive lock count */
    SInt            mSpinCount;
    SInt            mWaitThrCnt;
    iduShmLatchStat mStat;
} iduShmLatch;

#define IDU_SHM_LATCH_SPIN_COUNT_DEFAULT (iduProperty::getShmLatchSpinLockCount())

inline SInt iduShmLatchGetValue( iduShmLatch *aLatch )
{
    return idCore::acpAtomicGet32( &aLatch->mLock );
}

/**
 * initializes a spinlock
 * @param aLock the spinlock object
 * @param aSpinCount spin count prior to sleep
 */
inline void iduShmLatchInit( idShmAddr      aSelf,
                             SInt           aSpinCount,
                             iduShmLatch  * aLock )
{
    aLock->mLock       = 0;
    aLock->mRecursive  = 0;
    aLock->mAddrSelf   = aSelf;
    aLock->mSpinCount  = aSpinCount;
    aLock->mWaitThrCnt = 0;

    idlOS::memset( &aLock->mStat, 0, ID_SIZEOF( iduShmLatchStat ) );
}

inline void iduShmLatchDest( iduShmLatch     * aLock )
{
    IDE_ASSERT( aLock->mLock == 0 );
}

/**
 * trys a lock for a spinlock object
 * @param aLock the spinlock object
 * @return #ACP_TRUE if locked, #ACP_FALSE if not
 * TODO Handle recursive locking.
 */
inline idBool iduShmLatchTryLock( iduShmLatch *aLock,
                                  SInt         aLatchValue )
{
    SInt sVal;

    sVal = idCore::acpAtomicCas32( &aLock->mLock,
                                   aLatchValue,
                                   0/*Compare Value*/ );
    if( sVal == 0 )
    {
        ACP_MEM_BARRIER();
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/**
 * returns a lock state of a spinlock object
 * @param aLock the spinlock object
 * @return #ACP_TRUE if locked, #ACP_FALSE if not
 */
inline idBool iduShmLatchIsLocked( iduShmLatch *aLock )
{
    return ( aLock->mLock != 0 ) ? ID_TRUE : ID_FALSE;
}

/**
 * Returns whether the iduShmLatch object has been locked by the
 * specified thread.
 * @param aLock the iduShmLatch object
 * @param aThrID the ID of the calling thread
 * @return #ACP_TRUE if locked by the given thread, #ACP_FALSE otherwise
 */
inline idBool iduShmLatchIsLockedByThread( iduShmLatch *aLock,
                                           idGblThrID   aThrID )
{
    SInt sLatchValue = ( aThrID << 1 | 0x00000001 );
    return ( aLock->mLock == sLatchValue ) ? ID_TRUE : ID_FALSE;
}

IDE_RC iduShmLatchAcquire( idvSQL            * aStatistics,
                           iduShmTxInfo      * aShmTxInfo,
                           iduShmLatch       * aLock,
                           idBool              aCheckSession,
                           idBool              aPushLatchOP2Stack );

inline IDE_RC iduShmLatchAcquire( idvSQL       * aStatistics,
                                  iduShmTxInfo * aShmTxInfo,
                                  iduShmLatch  * aLock )
{
    return iduShmLatchAcquire( aStatistics,
                               aShmTxInfo,
                               aLock,
                               ID_FALSE, /* aCheckSession */
                               ID_TRUE   /* aPushLatchOP2Stack */  );
}

IDE_RC iduShmLatchAcquire( idvSQL            * aStatistics,
                           iduShmTxInfo      * aShmTxInfo,
                           iduShmLatch       * aLatch,
                           idBool              aCheckSession,
                           idBool              aPushLatchOP2Stack,
                           ULong               aLockWaitMicroSec,
                           ULong             * aWaitedMicroSec );

IDE_RC iduShmLatchTryAcquire( idvSQL       * aStatistics,
                              iduShmTxInfo * aShmTxInfo,
                              iduShmLatch  * aLock,
                              idBool         aPushLatchOP2Stack,
                              idBool       * aLocked );

IDE_RC iduShmLatchReleaseByUndo( UInt aThrID, iduShmLatch *aLatch );
IDE_RC iduShmLatchRelease( iduShmTxInfo * aShmTxInfo, iduShmLatch *aLatch );

#endif /* _O_IDU_SHM_LATCH_H_ */
