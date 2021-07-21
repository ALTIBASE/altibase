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
 * $Id:
 ******************************************************************************/
#include <idl.h>
#include <ideErrorMgr.h>
#include <ideMsgLog.h>
#include <idErrorCode.h>
#include <iduSessionEvent.h>
#include <idrLogMgr.h>
#include <iduVLogShmLatch.h>
#include <iduProperty.h>
#include <iduShmLatch.h>
#include <iduFitManager.h>

IDE_RC iduShmLatchTryAcquire( idvSQL       * /*aStatistics*/,
                              iduShmTxInfo * aShmTxInfo,
                              iduShmLatch  * aLatch,
                              idBool         aPushLatchOP2Stack,
                              idBool       * aLocked )
{
    SInt              sLatchValue;
    UInt              sLstLatchStackIdx;

    sLatchValue = ( aShmTxInfo->mThrID << 1 | 0x00000001 );

    if( aPushLatchOP2Stack == ID_TRUE )
    {
        IDE_TEST( idrLogMgr::pushLatchOp( aShmTxInfo,
                                          aLatch,
                                          IDU_SHM_LATCH_MODE_EXCLUSIVE,
                                          NULL /* aPushLatchOP2Stack */ )
                  != IDE_SUCCESS );
    }

    if( iduShmLatchTryLock( aLatch, sLatchValue ) == ID_TRUE )
    {
        *aLocked = ID_TRUE;
    }
    else
    {
        if( aPushLatchOP2Stack == ID_TRUE )
        {
            sLstLatchStackIdx = idrLogMgr::getLatchCntInStack( aShmTxInfo );

            idrLogMgr::clearLatchStack( aShmTxInfo,
                                        sLstLatchStackIdx,
                                        ID_FALSE );
        }

        *aLocked = ID_FALSE;
    }

    IDU_FIT_POINT( "iduShmLatch::iduShmLatchAcquire::return" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * locks a spinlock
 * @param aLock the spinlock object
 */
IDE_RC iduShmLatchAcquire( idvSQL       * aStatistics,
                           iduShmTxInfo * aShmTxInfo,
                           iduShmLatch  * aLock,
                           idBool         aCheckSession,
                           idBool         aPushLatchOP2Stack )
{
    SInt              sSpinLoop;
    SInt              sSpinCount;
    idBool            sIsMyLock;
    SInt              sLatchValue;
    iduShmProcInfo  * sProcInfo;
    UInt              sSleepCnt = 0;
    UInt              i;
    PDL_Time_Value    sSleepDuration;

    /* set up conditions */
    sIsMyLock      = ID_FALSE;

    sProcInfo    = (iduShmProcInfo*)IDV_WP_GET_PROC_INFO( aStatistics );

    static iduShmTxInfo * sMainThrInfo = &sProcInfo->mMainTxInfo;

    sLatchValue  = ( aShmTxInfo->mThrID << 1 | 0x00000001 );

    IDE_ASSERT( aLock->mSpinCount >= 0 );

    if( aLock->mSpinCount == 0 )
    {
        sSpinCount = IDU_SPIN_WAIT_DEFAULT_SPIN_COUNT;
    }
    else
    {
        sSpinCount = aLock->mSpinCount;
    }

    sSleepDuration.set( 0, iduProperty::getShmLatchSleepDuration() );

    /* MainThrInfo는 동시에 여러개의 Thread가 접근할 수 있기 때문에
     * MainThrInfo의 Latch Stack은 사용하지 않는다. 이때문에 Latch를
     * 잡고 죽을 경우 풀어줄 방법이 없다. 다행히 MainThrInfo가 잡는
     * Latch는 ThreadInfo Alloc할때 잡는 Latch * 가 전부이다.
     * 때문에 Process Undo시 먼저 MainThrInfo의 LogBuffer있는 Log를 Undo하고
     * Thread Info Alloc Latch를 푸는 연산을 무조건 시도한다.
     * 만약 잡고 죽었으면 Latch는 풀리고, 잡지않았을 경우에는 LatchRelease
     * 에서 아무것도 하지 않도록 구현되어 있다.
     * */
    if( ( aLock              != &sProcInfo->mLatch ) &&
        ( aPushLatchOP2Stack == ID_TRUE ) )
    {
        IDE_TEST( idrLogMgr::pushLatchOp( aShmTxInfo,
                                          aLock,
                                          IDU_SHM_LATCH_MODE_EXCLUSIVE,
                                          NULL /* aNewLatchInfo */ )
                  != IDE_SUCCESS );
    }

    while(1)
    {
        /* Only read lock variable from hardware-cache or memory
         * to reduce system-bus locking.
         */
        for( sSpinLoop = 0;
             sSpinLoop <= sSpinCount; /* sSpinCount can be 0 in UP! */
             sSpinLoop++ )
        {
            /* for cache hit, more frequency case is first */
            if( iduShmLatchIsLocked( aLock ) == ID_FALSE )
            {
                if( iduShmLatchTryLock( aLock, sLatchValue ) == ID_TRUE )
                {
                    /* success to lock */
                    /* return; is better than goto for recucing jump */
                    sIsMyLock = ID_TRUE;

                    break;
                }
                else
                {
                    /* Other process takes the lock.
                     * The process takes time in the critical section,
                     * therefore I better to sleep, not try lock.
                     * */
                    break;
                }
            }
            else
            {
                // 여러 Thread가 AllocShmTx를 동시에 요청하게 되면
                // 동시성 제어가 깨질 수 있으므로 MainShmTx는 Latch를
                // 중복해서 잡을 수 없다.
                if( sMainThrInfo != aShmTxInfo )
                {
                    if( aLock->mLock == sLatchValue )
                    {
                        /* try acquire에는 실패했지만, 자신이 이미
                         * 잡고 있는 lock이므로 recursive lock을
                         * 획득 */
                        sIsMyLock = ID_TRUE;
                        aLock->mRecursive++;
                        break;
                    }
                }
            }
        }

        if( sIsMyLock == ID_TRUE )
        {
            /* I win the lock */
            break;
        }
        else
        {
            if( aCheckSession == ID_TRUE )
            {
                IDE_TEST( iduCheckSessionEvent( aStatistics )
                          != IDE_SUCCESS );
            }

            /* sleep and try again to yield chance for others.
             * This can avoid excessive racing condition.
             */
            sSleepCnt++;
            
            if( sSleepCnt < iduProperty::getShmLatchYieldCount() )
            {
                idlOS::thr_yield();
            }
            else
            {
                idlOS::sleep(sSleepDuration);
                sSleepCnt = 0;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * locks a spinlock
 * @param aLock the spinlock object
 * @param aLockWaitMicroSec lock timeout
 */
IDE_RC iduShmLatchAcquire( idvSQL            * aStatistics,
                           iduShmTxInfo      * aShmTxInfo,
                           iduShmLatch       * aLatch,
                           idBool              aCheckSession,
                           idBool              aPushLatchOP2Stack,
                           ULong               aLockWaitMicroSec,
                           ULong             * aWaitedMicroSec )
{
    PDL_Time_Value   sMinTime;
    idBool           sLatchAcquired     = ID_FALSE;
    idBool           sLatchStackClear   = ID_FALSE;
    UInt             sLstLatchStackIdx;
    iduShmProcInfo * sProcInfo;
    iduShmTxInfo   * sMainThrInfo;
    SInt             sLatchValue;
    ULong            sSleepTime;

    if( aLockWaitMicroSec == IDU_SHM_SX_LATCH_INFINITE_WAIT )
    {
        /* lock wait time이 infinite인 경우에는
         * 기존의 blocking acquire함수를 호출하도록 한다. */
        IDE_TEST( iduShmLatchAcquire( aStatistics,
                                      aShmTxInfo,
                                      aLatch,
                                      aCheckSession,
                                      aPushLatchOP2Stack )
                  != IDE_SUCCESS );
    }
    else
    {
        sProcInfo = (iduShmProcInfo*)IDV_WP_GET_PROC_INFO( aStatistics );

        sMainThrInfo = &sProcInfo->mMainTxInfo;

        sLatchValue  = ( aShmTxInfo->mThrID << 1 | 0x00000001 );

        sMinTime.set( 0, IDU_SPIN_WAIT_SLEEP_TIME_MAX );

        sSleepTime = 0;

        if( aPushLatchOP2Stack == ID_TRUE )
        {
            IDE_TEST( idrLogMgr::pushLatchOp( aShmTxInfo,
                                              aLatch,
                                              IDU_SHM_LATCH_MODE_EXCLUSIVE,
                                              NULL )
                      != IDE_SUCCESS );

            sLatchStackClear = ID_TRUE;
        }

        do
        {
            if( iduShmLatchTryLock( aLatch, sLatchValue ) == ID_TRUE )
            {
                sLatchAcquired = ID_TRUE;
                sLatchStackClear = ID_FALSE;
                break;
            }
            else
            {
                if( sMainThrInfo != aShmTxInfo )
                {
                    if( aLatch->mLock == sLatchValue )
                    {
                        /* try acquire에는 실패했지만, 자신이 이미
                         * 잡고 있는 lock이므로 recursive lock을
                         * 획득 */
                        sLatchAcquired = ID_TRUE;
                        aLatch->mRecursive++;
                        break;
                    }
                }

                idlOS::sleep( sMinTime );

                sSleepTime += IDU_SPIN_WAIT_SLEEP_TIME_MAX;
            }
        } while( sSleepTime < aLockWaitMicroSec );

        if( aWaitedMicroSec != NULL )
        {
            *aWaitedMicroSec = sSleepTime;
        }

        IDE_TEST_RAISE( sLatchAcquired == ID_FALSE, err_lock_timeout );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_lock_timeout );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_LATCH_TIMEOUT ) );
    }
    IDE_EXCEPTION_END;

    if( sLatchStackClear == ID_TRUE )
    {
        sLstLatchStackIdx = idrLogMgr::getLatchCntInStack( aShmTxInfo );

        idrLogMgr::clearLatchStack( aShmTxInfo,
                                    sLstLatchStackIdx,
                                    ID_FALSE );
    }

    return IDE_FAILURE;
}

/**
 * Unlocks a spinlock object.  iduShmLatch supports recursive locking.
 * This means if a thread locks a latch multiple times, it has to
 * unlock the latch the same number of times before the latch is
 * finally released.
 *
 * @param aLock the spinlock object
 */
IDE_RC iduShmLatchReleaseByUndo( UInt          aThrID,
                                 iduShmLatch * aLatch )
{
    SInt sLatchValue;

    ACP_MEM_BARRIER();

    sLatchValue = ( aThrID << 1 | 0x00000001 );

    if ( ( aLatch->mRecursive > 0 ) && ( aLatch->mLock == sLatchValue ) )
    {
        aLatch->mRecursive--;
    }
    else
    {
        (void)idCore::acpAtomicCas32( &aLatch->mLock, 0, sLatchValue );
    }

    return IDE_SUCCESS;
}

/*******************************************************************************
 * [!] 이 함수를 바로 호출해선 안된다.
 *     반드시 idrLogMgr::xxx2Svp함수를 통해 호출해야 한다.
 *     현재(2014.02.28) 구조상 비정상 종료시
 *     Latch를 release 못해 ShmRecovery시 Hang에 걸릴 수 있다.
 *******************************************************************************/
IDE_RC iduShmLatchRelease( iduShmTxInfo * aShmTxInfo,
                           iduShmLatch  * aLatch )
{
    iduShmLatchReleaseByUndo( aShmTxInfo->mThrID, aLatch );

    return IDE_SUCCESS;
}
