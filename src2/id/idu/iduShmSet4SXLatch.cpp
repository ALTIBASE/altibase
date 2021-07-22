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
#include <idrLogMgr.h>
#include <iduVLogShmLatch.h>
#include <iduProperty.h>
#include <iduShmLatch.h>
#include <iduShmMgr.h>
#include <iduShmSet4SXLatch.h>

/***********************************************************************
 * Description : aLock이 가리키는 ShmSetLatch를 초기화한다.
 *
 * aStatistics    - [IN] Thread의 Context정보
 * aShmTxInfo     - [IN] Shared Memory Tx
 * aIndex         - [IN] Memory Type
 * aSelf          - [IN] aLock이 할당된 Shared Memory의 Address
 * aLatchCnt      - [IN] Set SXLatch가 가지게 될 Latch의 갯수
 * aSpinCount     - [IN] Spin Lock Count
 * aLock          - [IN-OUT] Set SXLatch Pointer
 **********************************************************************/
IDE_RC iduShmSet4SXLatchInit( idvSQL               * aStatistics,
                              iduShmTxInfo         * aShmTxInfo,
                              iduMemoryClientIndex   aIndex,
                              idShmAddr              aSelf,
                              UInt                   aLatchCnt,
                              UInt                   aSpinCount,
                              iduShmSet4SXLatch    * aLock )
{
    UInt          i;
    idShmAddr     sAddr4Latch;
    iduShmLatch * sArrLatch;

    aLock->mAddrSelf = aSelf;
    aLock->mLatchCnt = aLatchCnt;

    IDE_TEST( iduShmMgr::allocMem( aStatistics,
                                   aShmTxInfo,
                                   aIndex,
                                   ID_SIZEOF(iduShmLatch) * aLatchCnt,
                                   (void**)&sArrLatch )
              != IDE_SUCCESS );

    sAddr4Latch = iduShmMgr::getAddr( sArrLatch );

    aLock->mAddr4ArrLatch = sAddr4Latch;

    for( i = 0; i < aLatchCnt; i++ )
    {
        iduShmLatchInit( sAddr4Latch,
                         aSpinCount,
                         sArrLatch + i );

        sAddr4Latch += ID_SIZEOF(iduShmLatch);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aLock이 가리키는 ShmSetLatch에 할당된 자원을 정리한다.
 *
 * aStatistics    - [IN] Thread의 Context정보
 * aShmTxInfo     - [IN] Shared Memory Tx
 * aLock          - [IN-OUT] Set SXLatch Pointer
 **********************************************************************/
IDE_RC iduShmSet4SXLatchDest( idvSQL               * aStatistics,
                              iduShmTxInfo         * aShmTxInfo,
                              iduShmSet4SXLatch    * aLock )
{
    UInt          i;
    iduShmLatch * sArrLatch;

    IDE_ASSERT( aLock->mLatchCnt != 0 );
    IDE_ASSERT( aLock->mAddr4ArrLatch != IDU_SHM_NULL_ADDR );

    sArrLatch = (iduShmLatch*)iduShmMgr::getPtrOfAddr( aLock->mAddr4ArrLatch );

    for( i = 0; i < aLock->mLatchCnt; i++ )
    {
        iduShmLatchDest( sArrLatch + i );
    }

    IDE_ASSERT( iduShmMgr::freeMem( aStatistics,
                                    aShmTxInfo,
                                    NULL, /* aSavepoint */
                                    sArrLatch )
                == IDE_SUCCESS );

    aLock->mAddr4ArrLatch = IDU_SHM_NULL_ADDR;
    aLock->mLatchCnt      = 0;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : aLock이 가리키는 ShmSetLatch에 aMode로 Latch를 잡는다.
 *
 * aStatistics       - [IN] Thread의 Context정보
 * aShmTxInfo        - [IN] Shared Memory Tx
 * aSet4SXLatch      - [IN] Set SXLatch Pointer
 * aLatchIdx         - [IN] Set SXLatch에 존재하는 Latch 중에서 몇 번째
 *                          에 존재하는 Latch를 잡을 지 결정.
 * aMode             - [IN] IDU_SET_SX_LATCH_MODE_SHARED or
 *                          IDU_SET_SX_LATCH_MODE_EXCLUSIVE
 * aLockWaitMicroSec - [IN] Latch잡지 못하고 대기할 경우 몇 USec동안 대기해야
 *                          하는지를 결정.
 * **********************************************************************/
IDE_RC iduShmSet4SXLatchAcquire( idvSQL              * aStatistics,
                                 iduShmTxInfo        * aShmTxInfo,
                                 iduShmSet4SXLatch   * aSet4SXLatch,
                                 UInt                  aLatchIdx,
                                 iduShmSXLatchMode     aMode,
                                 ULong                 aLockWaitMicroSec )
{
    UInt          i = 0;
    UInt          j;
    iduShmLatch * sTgtLatch;
    iduShmLatch * sArrLatch;
    UInt          sLatchCnt          = aSet4SXLatch->mLatchCnt;
    idBool        sLatchAcquired     = ID_FALSE;
    idBool        sLatchPushed       = ID_FALSE;
    UInt          sLstLatchStackIdx;
    ULong         sWaitedMicroSec    = 0;                 /*Latch 잡지 못하고 실제 대기한 시간*/
    ULong         sLockWaitMicroSec  = aLockWaitMicroSec; /* Latch 잡지 못할때 대기 할 시간 */

    IDE_ASSERT( aLatchIdx < sLatchCnt );

    sArrLatch = (iduShmLatch*)IDU_SHM_GET_ADDR_PTR_CHECK( aSet4SXLatch->mAddr4ArrLatch );

    if( aMode == IDU_SET_SX_LATCH_MODE_SHARED )
    {
        sTgtLatch = sArrLatch + aLatchIdx;

        if( idrLogMgr::isSetLatchAcquired( aShmTxInfo,
                                           sTgtLatch,
                                           aMode ) == ID_FALSE )
        {
            IDE_TEST( idrLogMgr::pushSetLatchOp( aShmTxInfo,
                                                 sTgtLatch,
                                                 aMode,
                                                 NULL )
                      != IDE_SUCCESS );

            sLatchPushed = ID_TRUE;

            IDE_TEST( iduShmLatchAcquire( aStatistics,
                                          aShmTxInfo,
                                          sTgtLatch,
                                          ID_TRUE,   /* aCheckSession */
                                          ID_FALSE,  /* aPushLatchOP2Stack */
                                          sLockWaitMicroSec,
                                          NULL )
                      != IDE_SUCCESS );

            sLatchAcquired = ID_TRUE;
        }
    }
    else
    {
        IDE_DASSERT( aMode == IDU_SET_SX_LATCH_MODE_EXCLUSIVE );

        if( idrLogMgr::isSetLatchAcquired( aShmTxInfo,
                                           aSet4SXLatch,
                                           aMode ) == ID_FALSE )
        {
            /* exclusive set latch에 대해서는 latch stack에 SXSetLatch만 push하고
             * 개별 ShmLatch는 push 하지 않는다.
             * 이는 latch stack의 overflow를 막고, 구현을 효율적으로 하기 위함이다. */
            IDE_TEST( idrLogMgr::pushSetLatchOp( aShmTxInfo,
                                                 aSet4SXLatch,
                                                 aMode,
                                                 NULL )
                      != IDE_SUCCESS );

            sLatchPushed = ID_TRUE;

            for( i = 0; i < sLatchCnt; i++ )
            {
            	if( i != aLatchIdx )
            	{
                    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                                  aShmTxInfo,
                                                  sArrLatch + i,
                                                  ID_TRUE,   /* aCheckSession */
                                                  ID_FALSE , /* aPushLatchOP2Stack */
                                                  sLockWaitMicroSec,
                                                  &sWaitedMicroSec )
                              != IDE_SUCCESS );
            	}
            	else
            	{
                    if( idrLogMgr::isSetLatchAcquired( aShmTxInfo,
                                                       sArrLatch + i,
                                                       IDU_SET_SX_LATCH_MODE_SHARED ) == ID_FALSE )
                    {
                        IDE_TEST( iduShmLatchAcquire( aStatistics,
                                                      aShmTxInfo,
                                                      sArrLatch + i,
                                                      ID_TRUE,   /* aCheckSession */
                                                      ID_FALSE , /* aPushLatchOP2Stack */
                                                      sLockWaitMicroSec,
                                                      &sWaitedMicroSec )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        sWaitedMicroSec = 0;
                    }
            	}

                if( sLockWaitMicroSec == 0 )
                {
                    IDE_TEST_RAISE( sWaitedMicroSec > 0, err_lock_timeout );
                }
                else
                {
                    IDE_TEST_RAISE( sLockWaitMicroSec < sWaitedMicroSec, err_lock_timeout );
                    sLockWaitMicroSec -= sWaitedMicroSec;
                }
            }

            sLatchAcquired = ID_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_lock_timeout );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_LATCH_TIMEOUT ) );
    }
    IDE_EXCEPTION_END;

    if( sLatchAcquired == ID_FALSE )
    {
        if( aMode == IDU_SET_SX_LATCH_MODE_EXCLUSIVE )
        {
            for( j = 0; j < i; j++ )
            {
                IDE_ASSERT( iduShmLatchRelease( aShmTxInfo,
                                                sArrLatch + j )
                            == IDE_SUCCESS );
            }
        }

        if( sLatchPushed == ID_TRUE )
        {
            sLstLatchStackIdx = idrLogMgr::getSetLatchCntInStack( aShmTxInfo );

            idrLogMgr::clearSetLatchStack( aShmTxInfo,
                                           sLstLatchStackIdx );
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aLock이 가리키는 ShmSetLatch에 aMode로 잡힌 Latch를 해제한다.
 *
 * aStatistics    - [IN] Thread의 Context정보
 * aShmTxInfo     - [IN] Shared Memory Tx
 * aLatchOPInfo   - [IN] Latch stack에 들어가있는 SXSetLatch 정보
 **********************************************************************/
IDE_RC iduShmSet4SXLatchRelease( idvSQL              * /*aStatistics*/,
                                 iduShmTxInfo        * aShmTxInfo,
                                 iduShmLatchInfo     * aLatchOPInfo )
{
    UInt                i;
    UInt                sLatchCnt;
    iduShmLatch       * sArrLatch;
    iduShmSet4SXLatch * sSet4SXLatch;

    sSet4SXLatch = (iduShmSet4SXLatch*)aLatchOPInfo->mObject;
    sLatchCnt    = sSet4SXLatch->mLatchCnt;

    sArrLatch = (iduShmLatch*)IDU_SHM_GET_ADDR_PTR_CHECK( sSet4SXLatch->mAddr4ArrLatch );

    IDE_DASSERT( aLatchOPInfo->mMode == IDU_SET_SX_LATCH_MODE_EXCLUSIVE );

    for( i = 0; i < sLatchCnt; i++ )
    {
        IDE_TEST( iduShmLatchRelease( aShmTxInfo,
                                      sArrLatch + i )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
