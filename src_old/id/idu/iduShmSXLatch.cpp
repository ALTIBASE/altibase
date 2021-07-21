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
#include <idl.h>
#include <ideErrorMgr.h>
#include <ideMsgLog.h>
#include <idErrorCode.h>
#include <iduSessionEvent.h>
#include <iduShmDef.h>
#include <idrLogMgr.h>
#include <iduShmLatch.h>
#include <iduVLogShmLatch.h>
#include <iduShmSXLatch.h>
#include <iduProperty.h>

/***********************************************************************
 * Description : aSXLatch를 acquire한다.
 *
 * aStatistics       - [IN] Thread의 Context정보
 * aShmTxInfo        - [IN] Shared Memory Tx
 * aSXLatch          - [IN] SX latch
 * aMode             - [IN] latch acquire mode : shared or exclusive
 * aLockWaitMicroSec - [IN] latch acquire timeout
 **********************************************************************/
IDE_RC iduShmSXLatchAcquire( idvSQL            * aStatistics,
                             iduShmTxInfo      * aShmTxInfo,
                             iduShmSXLatch     * aSXLatch,
                             iduShmSXLatchMode   aMode,
                             ULong               aLockWaitMicroSec )
{
    idrSVP              sVSavepoint;
    SInt                sSpinLoop;
    SInt                sSpinCount;
    UInt                sSpinSleepTime;
    PDL_Time_Value      sSleepTime;
    idBool              sIsMyLock;
    iduShmLatchInfo   * sLatchOPInfo;

    idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint );

    if( aSXLatch->mCheckAcquired == ID_TRUE )
    {
        if( idrLogMgr::isLatchAcquired( aShmTxInfo,
                                        aSXLatch,
                                        aMode ) == ID_TRUE )
        {
            IDE_RAISE( cont_latch_already_acquired );
        }
    }

    if( aSXLatch->mLatch.mSpinCount == 0 )
    {
        sSpinCount = IDU_SPIN_WAIT_DEFAULT_SPIN_COUNT;
    }
    else
    {
        sSpinCount = aSXLatch->mLatch.mSpinCount;
    }

    /* set up conditions */
    sSpinSleepTime = IDU_SPIN_WAIT_SLEEP_TIME_MIN;
    sIsMyLock      = ID_FALSE;

    if( aMode == IDU_SX_LATCH_MODE_SHARED )
    {
        IDE_TEST( idrLogMgr::pushLatchOp( aShmTxInfo,
                                          aSXLatch,
                                          aMode,
                                          &sLatchOPInfo )
                  != IDE_SUCCESS );


        IDE_TEST( iduShmLatchAcquire( aStatistics,
                                      aShmTxInfo,
                                      &aSXLatch->mLatch,
                                      aSXLatch->mCheckAcquired,
                                      ID_FALSE /* Not Push Lock OP to Stack */ )
                  != IDE_SUCCESS );

        sLatchOPInfo->mLatchOPState = IDU_SHM_LATCH_STATE_LOCK;


        ID_SERIAL_BEGIN( sLatchOPInfo->mOldSLatchCnt = aSXLatch->mSharedLatchCnt );


        ID_SERIAL_END( aSXLatch->mSharedLatchCnt++ );


        IDE_TEST( iduShmLatchRelease( aShmTxInfo, &aSXLatch->mLatch )
              != IDE_SUCCESS );


        sLatchOPInfo->mOldSLatchCnt = IDU_SHM_SX_SLATCH_CNT_INVALID;

    }
    else
    {
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
                if( aSXLatch->mSharedLatchCnt == 0 )
                {
                    idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint );

                    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                                  aShmTxInfo,
                                                  &aSXLatch->mLatch )
                              != IDE_SUCCESS );

                    if( aSXLatch->mSharedLatchCnt == 0 )
                    {
                        sIsMyLock = ID_TRUE;
                        break;
                    }

                    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, 
                                                     aShmTxInfo, 
                                                     &sVSavepoint )
                              != IDE_SUCCESS );
                }
                else
                {
                    if( idrLogMgr::isLatchAcquired( aShmTxInfo,
                                                    aSXLatch,
                                                    IDU_SX_LATCH_MODE_SHARED ) 
                        == ID_TRUE )
                    {
                        idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint );
                        IDE_TEST( iduShmLatchAcquire( aStatistics,
                                                      aShmTxInfo,
                                                      &aSXLatch->mLatch )
                                  != IDE_SUCCESS );

                        if( aSXLatch->mSharedLatchCnt == 1 )
                        {
                            if( idrLogMgr::isLatchAcquired( 
                                   aShmTxInfo,
                                   aSXLatch,
                                   IDU_SX_LATCH_MODE_SHARED ) == ID_TRUE )
                            {
                                sIsMyLock = ID_TRUE;
                            }

                            break;
                        }

                       IDE_TEST( idrLogMgr::commit2Svp( aStatistics, 
                                                        aShmTxInfo, 
                                                        &sVSavepoint )
                                 != IDE_SUCCESS );
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
                IDE_TEST_RAISE( aLockWaitMicroSec == 0, err_lock_timeout );

                if( aSXLatch->mCheckAcquired == ID_TRUE )
                {
                    IDE_TEST( iduCheckSessionEvent( aStatistics )
                              != IDE_SUCCESS );


                    /* sleep and try again to yield chance for others.
                     * This can avoid excessive racing condition.
                     */
                    sSleepTime.set( 0, sSpinSleepTime );

                    idlOS::sleep( sSleepTime );

                    if( sSpinSleepTime < IDU_SPIN_WAIT_SLEEP_TIME_MAX )
                    {
                         sSpinSleepTime *= 2;
                    }
                    else
                    {
                         /* reach maximum sleep time */
                    }
                }
            }
        }
    }

    IDE_EXCEPTION_CONT( cont_latch_already_acquired );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_lock_timeout );
    {
       IDE_SET(ideSetErrorCode(idERR_ABORT_LATCH_TIMEOUT)); 
    }
    IDE_EXCEPTION_END;

    IDE_TEST( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sVSavepoint )
              != IDE_SUCCESS );

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aLatchOPInfo에 해당하는 SXLatch를 release한다.
 *
 * aStatistics       - [IN] Thread의 Context정보
 * aShmTxInfo        - [IN] Shared Memory Tx
 * aLatchOPInfo      - [IN] latch stack OP info
 * aMode             - [IN] latch acquire mode : shared or exclusive
 **********************************************************************/
IDE_RC iduShmSXLatchRelease( idvSQL            * aStatistics,
                             iduShmTxInfo      * aShmTxInfo,
                             iduShmLatchInfo   * aLatchOPInfo,
                             iduShmSXLatchMode   aMode )
{
    iduShmTxInfo    * sTxInfo;
    SInt              sLatchValue;
    iduShmSXLatch   * sSXLatch;
    iduShmLatchState  sBfrLatchState;

    sSXLatch    = (iduShmSXLatch*)aLatchOPInfo->mObject;
    sTxInfo     = aShmTxInfo;
    sLatchValue = ( sTxInfo->mThrID << 1 | 0x00000001 );

    if( aMode == IDU_SX_LATCH_MODE_SHARED )
    {
        if( sLatchValue == sSXLatch->mLatch.mLock )
        {
            if( aLatchOPInfo->mOldSLatchCnt != IDU_SHM_SX_SLATCH_CNT_INVALID )
            {
                sSXLatch->mSharedLatchCnt = aLatchOPInfo->mOldSLatchCnt;
            }

            if(( aLatchOPInfo->mLatchOPState == IDU_SHM_LATCH_STATE_UNLOCKING ) ||
               ( aLatchOPInfo->mLatchOPState == IDU_SHM_LATCH_STATE_UNLOCKED ))
            {
                sSXLatch->mSharedLatchCnt--;
            }
        }
        else
        {
            sBfrLatchState = aLatchOPInfo->mLatchOPState;
            aLatchOPInfo->mLatchOPState = IDU_SHM_LATCH_STATE_UNLOCKING;


            iduShmLatchAcquire( aStatistics,
                                aShmTxInfo,
                                &sSXLatch->mLatch,
                                ID_FALSE, /* aCheckSession */
                                ID_FALSE  /* Not Push Lock OP to Stack */ );


            if(( sBfrLatchState == IDU_SHM_LATCH_STATE_UNLOCKING ) ||
               ( sBfrLatchState == IDU_SHM_LATCH_STATE_LOCK ))
            {
                ID_SERIAL_BEGIN( aLatchOPInfo->mOldSLatchCnt = sSXLatch->mSharedLatchCnt );

                IDE_ASSERT( sSXLatch->mSharedLatchCnt != 0 );

                ID_SERIAL_END( sSXLatch->mSharedLatchCnt-- );

            }

            aLatchOPInfo->mLatchOPState = IDU_SHM_LATCH_STATE_UNLOCKED;

        }

        IDE_TEST( iduShmLatchRelease( aShmTxInfo, &sSXLatch->mLatch )
                  != IDE_SUCCESS );


        aLatchOPInfo->mOldSLatchCnt = IDU_SHM_SX_SLATCH_CNT_INVALID;

    }
    else
    {
        IDE_TEST( iduShmLatchRelease( aShmTxInfo, &sSXLatch->mLatch )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
