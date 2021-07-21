/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: rpxSender.cpp 18930 2006-11-14 01:00:47Z raysiasd $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>

#include <qci.h>

#include <rpDef.h>
#include <rpuProperty.h>
#include <rpcManager.h>
#include <rpxSenderApply.h>

rpxSenderApply::rpxSenderApply() : idtBaseThread()
{
}

IDE_RC rpxSenderApply::initialize(rpxSender        *aSender,
                                  idvSQL           *aOpStatistics,
                                  idvSession      * aStatSession,
                                  rpdSenderInfo    *aSenderInfo,
                                  rpnMessenger    * aMessenger,
                                  rpdMeta          *aMeta,
                                  void             *aRsc,
                                  UInt             *aRetryCount,
                                  idBool           *aRetryError,
                                  idBool           *aApplyFaultFlag,
                                  idBool           *aSenderStopFlag,
                                  idBool            aIsSupportRecovery,
                                  RP_SENDER_TYPE   *aSenderType,
                                  UInt              aParallelID,
                                  RP_SENDER_STATUS *aStatus,
                                  RP_SOCKET_TYPE    aSocketType)
{
	smLSN sTempLSN;

    IDE_ASSERT(aSenderInfo != NULL);
    IDE_ASSERT(aMessenger != NULL);
    IDE_ASSERT(aMeta != NULL);
    IDE_ASSERT(aRetryError != NULL);
    IDE_ASSERT(aApplyFaultFlag != NULL);
    IDE_ASSERT(aSenderStopFlag != NULL);
    IDE_ASSERT(aSenderType != NULL);
    IDE_ASSERT(aStatus != NULL);

    mSender            = aSender;
    mSenderInfo        = aSenderInfo;
    mMessenger         = aMessenger;
    mMeta              = aMeta;
    mRsc               = aRsc;
    mRetryCount        = aRetryCount;
    mRetryError        = aRetryError;
    mApplyFaultFlag    = aApplyFaultFlag;
    *mApplyFaultFlag   = ID_FALSE;
    mSenderStopFlag    = aSenderStopFlag;
    mExitFlag          = ID_FALSE;
    mIsSupportRecovery = aIsSupportRecovery;
    mPrevRestartSN     = SM_SN_NULL;
    mIsSuspended       = ID_FALSE;
    mOpStatistics      = aOpStatistics;
    mStatSession       = aStatSession;
    mSocketType        = aSocketType;

    /* PROJ-1915 */
    mSenderType        = aSenderType;
    mParallelID        = aParallelID;

    mStatus            = aStatus;

    mRepName           = mMeta->mReplication.mRepName;

    SM_LSN_INIT( sTempLSN );
    if ( mMeta->mReplication.mReplMode == RP_EAGER_MODE )
    {
        SM_SET_LSN( sTempLSN, 
                    RPU_REPLICATION_EAGER_KEEP_LOGFILE_COUNT,
                    0 );
        mEagerUpdatedRestartSNGap = SM_MAKE_SN( sTempLSN );
    }
    else
    {
        mEagerUpdatedRestartSNGap = 0;
    }

    if ( ( mMeta->mReplication.mRole != RP_ROLE_ANALYSIS ) &&   // PROJ-1537
         ( mMeta->mReplication.mRole != RP_ROLE_ANALYSIS_PROPAGATION ) &&
         ( *mSenderType != RP_OFFLINE ) &&                        // PROJ-1915
         ( mSocketType != RP_SOCKET_TYPE_IB ) )
    {
        IDE_ASSERT(aRsc != NULL);
    }

    (void)idlOS::memset(&mReceivedAck, 0, ID_SIZEOF(rpXLogAck));

    if ( mIsSupportRecovery == ID_TRUE )
    {
        IDU_LIST_INIT( &mRestartSNList );
    }
    else
    {
        /* Nothing to do */
    }

    mMinRestartSN = 0;

    return IDE_SUCCESS;
}

void rpxSenderApply::shutdown()
{
    mExitFlag = ID_TRUE;
}

void rpxSenderApply::destroy()
{
    (void)idlOS::memset(&mReceivedAck, 0, ID_SIZEOF(rpXLogAck));

    *mApplyFaultFlag = ID_FALSE;
    return;
}

IDE_RC rpxSenderApply::initializeThread()
{
    SChar          sName[IDU_MUTEX_NAME_LEN];

    /* Thread의 run()에서만 사용하는 메모리를 할당한다. */

    IDU_FIT_POINT_RAISE( "rpxSenderApply::initializeThread::calloc::AbortTxList",
                         ERR_MEMORY_ALLOC_ABORT_TX_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_SENDER,
                                       smiGetTransTblSize(),
                                       ID_SIZEOF( rpTxAck ),
                                       (void **)&(mReceivedAck.mAbortTxList),
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_ABORT_TX_LIST );

    IDU_FIT_POINT_RAISE( "rpxSenderApply::initializeThread::calloc::ClearTxList",
                         ERR_MEMORY_ALLOC_CLEAR_TX_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_SENDER,
                                       smiGetTransTblSize(),
                                       ID_SIZEOF( rpTxAck ),
                                       (void **)&(mReceivedAck.mClearTxList),
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_CLEAR_TX_LIST );

    if ( mIsSupportRecovery == ID_TRUE )
    {
        idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_RESTART_SN_LIST",
                         mRepName );
        IDE_TEST( mRestartSNPool.initialize( IDU_MEM_RP_RPX_SENDER,
                                             sName,
                                             1,
                                             ID_SIZEOF( rpxRestartSN ),
                                             256,
                                             IDU_AUTOFREE_CHUNK_LIMIT, //chunk max(default)
                                             ID_FALSE,                 //use mutex(no use)
                                             8,                        //align byte(default)
                                             ID_FALSE,				   //ForcePooling
                                             ID_TRUE,				   //GarbageCollection
                                             ID_TRUE,                  /* HWCacheLine */
                                             IDU_MEMPOOL_TYPE_LEGACY   /* mempool type */ ) 
                 != IDE_SUCCESS);			
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_ABORT_TX_LIST )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxSenderApply::initializeThread",
                                  "mReceivedAck.mAbortTxList" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CLEAR_TX_LIST )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxSenderApply::initializeThread",
                                  "mReceivedAck.mClearTxList" ) );
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG( IDE_RP_0 );
    IDE_PUSH();

    if ( mReceivedAck.mAbortTxList != NULL )
    {
        (void)iduMemMgr::free( mReceivedAck.mAbortTxList );
        mReceivedAck.mAbortTxList = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( mReceivedAck.mClearTxList != NULL )
    {
        (void)iduMemMgr::free( mReceivedAck.mClearTxList );
        mReceivedAck.mClearTxList = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();
    return IDE_FAILURE;
}

void rpxSenderApply::finalizeThread()
{
    if ( mIsSupportRecovery == ID_TRUE )
    {
        if ( mRestartSNPool.destroy( ID_FALSE ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( mReceivedAck.mAbortTxList != NULL )
    {
        (void)iduMemMgr::free( mReceivedAck.mAbortTxList );
        mReceivedAck.mAbortTxList = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( mReceivedAck.mClearTxList != NULL )
    {
        (void)iduMemMgr::free( mReceivedAck.mClearTxList );
        mReceivedAck.mClearTxList = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

IDE_RC rpxSenderApply::updateXSN(smSN aSN)
{
    smiTrans          sTrans;
    SInt              sStage = 0;
    idBool            sIsTxBegin = ID_FALSE;
    smiStatement     *spRootStmt;
    //PROJ- 1677 DEQ
    UInt              sFlag = 0;

    // Recovery Sender는 SenderApply를 띄우지 않는다.
    IDE_DASSERT(*mSenderType != RP_RECOVERY);

    //----------------------------------------------------------------//
    // no commit log occurred
    //----------------------------------------------------------------//
    IDE_TEST_CONT((aSN == SM_SN_NULL) || (aSN == 0), NORMAL_EXIT);

    /* DDL Sync 의 경우 DDL 중 일부 SYS_REPLICATIONS_ 를 업데이트 하는 경우가 있다.
     * 만약 DDL 트랜잭션보다 먼저 updateXSN 으로 SYS_REPLICATIONS_ 를 수정할 경우
     * DDL Sync 가 실패하게 되어 DDL Sync 가 완료된 이후에 업데이트 한다. */
    IDE_TEST_CONT( mSenderInfo->getSkipUpdateXSN() == ID_TRUE, NORMAL_EXIT );

    /* Eager 의 경우 mUpdatedRestartSNGap 이 설정된다.
     * Remote 에서 commit 이 완료 되었지만 해당 로그가 디스크에 아직
     * sync 가 안되어 있을수 있다. 그렇기 때문에 Restart SN 값을 
     * 특정값을 뺀 이후에 업데이트 한다. 
     */
    IDE_TEST_RAISE( aSN <= mEagerUpdatedRestartSNGap, NORMAL_EXIT );
    aSN = aSN - mEagerUpdatedRestartSNGap;

    // Do not need to update XSN again with the same value
    IDE_TEST_CONT(mPrevRestartSN >= aSN, NORMAL_EXIT);

    if((isParallelChild() == ID_TRUE) ||
       (*mSenderType == RP_OFFLINE))
    {
        // Parallel Child, Offline Sender는 Restart SN을 갱신하지 않으며,
        // 뒤에 오는 작업이 없다.
        mSenderInfo->setRestartSN(aSN);
        mPrevRestartSN = aSN;
    }
    else if((*mStatus == RP_SENDER_FAILBACK_NORMAL) ||
            (*mStatus == RP_SENDER_FAILBACK_MASTER) ||
            (*mStatus == RP_SENDER_FAILBACK_SLAVE) ||
            (*mStatus == RP_SENDER_CONSISTENT_FAILBACK) ||
            (*mSenderType == RP_XLOGFILE_FAILBACK_MASTER) ||
            (*mSenderType == RP_XLOGFILE_FAILBACK_SLAVE) ||
            (*mSenderType == RP_SYNC) ||
            (*mSenderType == RP_SYNC_ONLY))
    {
        // Failback, Sync, Sync Only의 SenderApply는 Restart SN을 갱신하지 않는다.
        // Sync중인 Sender가 Restart SN을 갱신하면, Sender와 SenderApply간에 Deadlock이 발생한다.
        // PROJ-2725 do no update remote XSN when processing consistent failback 
    }
    else
    {
        // Lazy/Acked Sender, Parallel Mgr Sender에서 XSN을 갱신한다.

        IDU_FIT_POINT_RAISE( "rpxSenderApply::updateXSN::Erratic::rpERR_ABORT_RP_SENDER_UPDATE_XSN",
                             ERR_TRANS_INIT );         
        IDE_TEST_RAISE( sTrans.initialize() != IDE_SUCCESS, ERR_TRANS_INIT );
        sStage = 1;

        sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
        sFlag = (sFlag & ~SMI_TRANSACTION_MASK) | SMI_TRANSACTION_NORMAL;
        sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
        sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_NOWAIT;

        IDE_TEST_RAISE( sTrans.begin(&spRootStmt, NULL, sFlag, SMX_NOT_REPL_TX_ID )
                        != IDE_SUCCESS, ERR_TRANS_BEGIN );
        sIsTxBegin = ID_TRUE;
        sStage = 2;

        IDE_TEST_RAISE( rpcManager::updateXSN( spRootStmt,
                                               mRepName,
                                               aSN )
                        != IDE_SUCCESS, ERR_UPDATE_XSN );

        sStage = 1;

        IDE_TEST_RAISE( sTrans.commit() != IDE_SUCCESS, ERR_TRANS_COMMIT );
        sIsTxBegin = ID_FALSE;

        mMeta->mReplication.mXSN = aSN;
        mSenderInfo->setRestartSN(aSN);
        mPrevRestartSN = aSN;
        *mRetryCount = 0;
            
        sStage = 0;
        IDE_TEST_RAISE( sTrans.destroy( NULL ) != IDE_SUCCESS, ERR_TRANS_DESTROY );
    }

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_INIT )
    {
        IDE_WARNING( IDE_RP_0, RP_TRC_SA_UPDATE_XSN_TRANS_INIT );
    }
    IDE_EXCEPTION( ERR_TRANS_BEGIN )
    {
        IDE_WARNING( IDE_RP_0, RP_TRC_SA_UPDATE_XSN_TRANS_BEGIN );
    }
    IDE_EXCEPTION( ERR_UPDATE_XSN )
    {
        IDE_WARNING( IDE_RP_0, RP_TRC_SA_UPDATE_XSN_UPDATE_XSN );

        if ( *mRetryCount < RPU_REPLICATION_SENDER_RETRY_COUNT )
        {
            *mRetryError = ID_TRUE;
            *mRetryCount += 1;
        }
    }
    IDE_EXCEPTION( ERR_TRANS_COMMIT )
    {
        IDE_WARNING( IDE_RP_0, RP_TRC_SA_UPDATE_XSN_TRANS_COMMIT );
    }
    IDE_EXCEPTION( ERR_TRANS_DESTROY )
    {
        IDE_WARNING( IDE_RP_0, RP_TRC_SA_UPDATE_XSN_TRANS_DESTROY );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    IDE_PUSH();

    switch(sStage)
    {
        case 2:
            IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
            sIsTxBegin = ID_FALSE;
        case 1:
            if(sIsTxBegin == ID_TRUE)
            {
                IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
                sIsTxBegin = ID_FALSE;
            }
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_UPDATE_XSN));

    return IDE_FAILURE;
}

void rpxSenderApply::run()
{
    ULong  sWaitTime        = 0;
    idBool sIsTimeOut       = ID_FALSE;
    ULong  sBasicCheckTime  = 0;
    ULong  sRemainCheckTime = 0;
    ULong  sRealCheckTime   = 0;
    smSN   sRestartSN       = SM_SN_NULL;
    IDE_RC sRC;
    idBool sIsOfflineStatusFound = ID_FALSE;
    PDL_Time_Value sSleepTv;

    sSleepTv.initialize( 0, RPU_SENDER_SLEEP_TIME );

    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_S_RECV_ACK);
    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_S_SET_ACKEDVALUE);

    IDE_CLEAR();

    ideLog::log( IDE_RP_0, RP_TRC_SA_SENDER_APPLY_START, mRepName );

    if(RPU_REPLICATION_RECEIVE_TIMEOUT <= RP_SENDER_APPLY_CHECK_INTERVAL)
    {
        sBasicCheckTime  = RPU_REPLICATION_RECEIVE_TIMEOUT;
        sRemainCheckTime = RPU_REPLICATION_RECEIVE_TIMEOUT;
    }
    else //(RP_SENDER_APPLY_CHECK_INTERVAL < RPU_REPLICATION_RECEIVE_TIMEOUT)
    {
        sBasicCheckTime  = RP_SENDER_APPLY_CHECK_INTERVAL;
        sRemainCheckTime = RPU_REPLICATION_RECEIVE_TIMEOUT %
                           RP_SENDER_APPLY_CHECK_INTERVAL;
    }

    while(mExitFlag != ID_TRUE)
    {
        IDE_CLEAR();

        sWaitTime = 0;
        sRealCheckTime = sBasicCheckTime;
        do
        {
            IDU_FIT_POINT( "rpxSenderApply::run::RECV_ACK::SLEEP" );
            IDE_TEST_CONT(mExitFlag == ID_TRUE, NORMAL_EXIT);
            IDE_TEST_RAISE(sWaitTime >= RPU_REPLICATION_RECEIVE_TIMEOUT,
                           ERR_RECV_ACK_TIMEOUT);

            /* BUG-31545 sender apply 쓰레드와 sender 쓰레드의 통계정보 값 변경 시점이
             * 겹치지 않도록 각자 따로 세션에 반영하고 초기화한다.
             * 만약 이 작업을 sender에서 하게되면, sender가 init하는 도중
             * apply가 통계정보값을 쓸 수 있다.
             */
            idvManager::applyOpTimeToSession(mStatSession, mOpStatistics);
            idvManager::initRPSenderApplyAccumTime(mOpStatistics);

            IDE_TEST( checkHBT() != IDE_SUCCESS );

            RP_OPTIMIZE_TIME_BEGIN(mOpStatistics, IDV_OPTM_INDEX_RP_S_RECV_ACK);

            IDU_FIT_POINT( "rpxSenderApply::run::receiveAck::SLEEP" );
            sRC = mMessenger->receiveAck( &mReceivedAck,
                                          &mExitFlag,
                                          sRealCheckTime,
                                          &sIsTimeOut );

            RP_OPTIMIZE_TIME_END(mOpStatistics, IDV_OPTM_INDEX_RP_S_RECV_ACK);

            if(sRC != IDE_SUCCESS)
            {
                /* BUG-30341 ACK 수신 중에 mExitFlag가 설정되면, 정상 종료로 처리 */
                IDE_TEST_CONT(mExitFlag == ID_TRUE, NORMAL_EXIT);

                IDE_TEST( checkHBT() != IDE_SUCCESS );

                IDE_RAISE(ERR_RECV_ACK);
            }

            sWaitTime += sRealCheckTime;
            if((sWaitTime + sRealCheckTime) > RPU_REPLICATION_RECEIVE_TIMEOUT)
            {
                sRealCheckTime = sRemainCheckTime;
            }

            mSenderInfo->signalToAllServiceThr( ID_FALSE, mReceivedAck.mTID );
        }
        while(sIsTimeOut == ID_TRUE);

        // Stop Message Ack Arrived
        if(mReceivedAck.mAckType == RP_X_STOP_ACK)
        {
            if ( *mSenderType == RP_OFFLINE )
            {
                rpcManager::setOfflineStatus( mRepName, RP_OFFLINE_END, &sIsOfflineStatusFound );
                IDE_ASSERT( sIsOfflineStatusFound == ID_TRUE );
            }
            mExitFlag = ID_TRUE;
            mEagerUpdatedRestartSNGap = 0;
        }
        else if ( mReceivedAck.mAckType == RP_X_SYNC_END_ACK )
        {
            mSenderInfo->setFlagRebuildIndex( ID_TRUE );
            continue;
        }
        else if ( mReceivedAck.mAckType == RP_X_TRUNCATE_ACK )
        {
            mSenderInfo->setFlagTruncate( ID_TRUE );
            continue;
        }
        else if ( mReceivedAck.mAckType == RP_X_XA_START_REQ_ACK ) 
        {
            mSenderInfo->setGlobalTxAckRecvSN( mReceivedAck.mTID, mReceivedAck.mLastProcessedSN );
        }
        else
        {
            /* Nothing to do */
        }

        RP_OPTIMIZE_TIME_BEGIN(mOpStatistics, IDV_OPTM_INDEX_RP_S_SET_ACKEDVALUE);
        mSenderInfo->setAckedValue( mReceivedAck.mLastProcessedSN,
                                    mReceivedAck.mLastArrivedSN,
                                    mReceivedAck.mAbortTxCount,
                                    mReceivedAck.mAbortTxList,
                                    mReceivedAck.mClearTxCount,
                                    mReceivedAck.mClearTxList,
                                    mReceivedAck.mTID );

        RP_OPTIMIZE_TIME_END(mOpStatistics, IDV_OPTM_INDEX_RP_S_SET_ACKEDVALUE);

        mSenderInfo->signalToAllServiceThr( ID_FALSE, mReceivedAck.mTID );

        if ( mIsSupportRecovery == ID_TRUE ) //recovery option이 set되어있는 경우
        {
            IDE_TEST(insertRestartSNforRecovery(mReceivedAck.mRestartSN)
                     != IDE_SUCCESS);
            getRestartSNforRecovery(&sRestartSN, mReceivedAck.mFlushSN);

            if ( isParallelParent() == ID_TRUE )
            {
                getMinRestartSNFromAllApply( &sRestartSN );
            }
            else
            {
                if ( sRestartSN != SM_SN_NULL )
                {
                    mMinRestartSN = sRestartSN;
                }
                else
                {
                    /* do nothing */
                }
            }
        }
        else
        {
            sRestartSN = mReceivedAck.mRestartSN;
        }

        if ( mPrevRestartSN == SM_SN_NULL )
        {
            mPrevRestartSN = mSenderInfo->getRestartSN();
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST(updateXSN(sRestartSN) != IDE_SUCCESS);

        mSenderInfo->setRmtLastCommitSN(mReceivedAck.mLastCommitSN);

        /* PROJ-1442 Replication Online 중 DDL 허용
         * Handshake 중에는 Sender Apply를 중지한다.
         */
        if ( ( mReceivedAck.mAckType == RP_X_HANDSHAKE_ACK ) ||
             ( mReceivedAck.mAckType == RP_X_DDL_REPLICATE_HANDSHAKE_ACK ) )
        {
            mIsSuspended = ID_TRUE;

            while((mIsSuspended   == ID_TRUE) &&
                  (mExitFlag      != ID_TRUE) &&
                  (*mRetryError != ID_TRUE) &&
                  // BUG-24290 [RP] DDL로 인해 SenderApply가 대기 중일 때
                  //     Sender를 STOP시키면, SenderApply의 대기를 풀어야 합니다
                  (*mSenderStopFlag != ID_TRUE))
            {
                IDE_TEST( checkHBT() != IDE_SUCCESS );

                idlOS::sleep( sSleepTv );
            }
        }
    }
    RP_LABEL(NORMAL_EXIT);
    IDE_DASSERT(mExitFlag == ID_TRUE);

    ideLog::log( IDE_RP_0, RP_TRC_SA_SENDER_APPLY_END, mRepName );

    // Commit 대기 중인 Service Thread를 더 이상 깨울 수 없으므로, deActivate()를 수행한다.
    mSenderInfo->checkAndRunDeactivate();
    mSenderInfo->signalToAllServiceThr(ID_TRUE, SM_NULL_TID );

    return;

    IDE_EXCEPTION(ERR_RECV_ACK);
    {
        *mRetryError = ID_TRUE;
        mSenderInfo->checkAndRunDeactivate();
        mSenderInfo->signalToAllServiceThr(ID_TRUE, SM_NULL_TID );
    }
    IDE_EXCEPTION(ERR_RECV_ACK_TIMEOUT);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_TIMEOUT_EXCEED));
        *mRetryError = ID_TRUE;
        mSenderInfo->checkAndRunDeactivate();
        mSenderInfo->signalToAllServiceThr(ID_TRUE, SM_NULL_TID );
    }
    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, RP_TRC_SA_SENDER_APPLY_GOT_ERROR, mRepName );

    IDE_ERRLOG(IDE_RP_0);

    // Network 오류가 아닌 경우에는 Service Thread의 Commit을 지연시켜야 하므로,
    // SenderInfo를 조작하지 않는다.

    *mApplyFaultFlag = ID_TRUE;
    return;
}

IDE_RC rpxSenderApply::insertRestartSNforRecovery(smSN aRestartSN)
{
    iduListNode   *  sNode;
    rpxRestartSN  *  sRestartSNEntry;
    rpxRestartSN  *  sNewEntry;

    IDE_TEST_CONT( aRestartSN == SM_SN_NULL, NORMAL_EXIT );

    if(IDU_LIST_IS_EMPTY(&mRestartSNList) != ID_TRUE)
    {
        IDU_LIST_ITERATE_BACK(&mRestartSNList, sNode)
        {
            sRestartSNEntry = (rpxRestartSN*)sNode->mObj;
            if(sRestartSNEntry->mRestartSN == aRestartSN)
            {
                return IDE_SUCCESS;
            }
        }
    }

    IDE_TEST(mRestartSNPool.alloc((void**)&sNewEntry) != IDE_SUCCESS);
    sNewEntry->mRestartSN = aRestartSN;
    IDU_LIST_INIT_OBJ(&(sNewEntry->mNode), sNewEntry);

    IDU_LIST_ADD_LAST(&mRestartSNList, &(sNewEntry->mNode));

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxSenderApply::getRestartSNforRecovery(smSN* aRestartSN,
                                             smSN  aFlushSN)
{
    iduListNode   *  sNode = NULL;
    iduListNode   *  sDummy = NULL;
    rpxRestartSN  *  sRestartSNEntry = NULL;

    *aRestartSN = SM_SN_NULL;

    if(aFlushSN != SM_SN_NULL)
    {
        if(IDU_LIST_IS_EMPTY(&mRestartSNList) != ID_TRUE)
        {
            IDU_LIST_ITERATE_SAFE(&mRestartSNList, sNode, sDummy)
            {
                sRestartSNEntry = (rpxRestartSN*)sNode->mObj;

                if(sRestartSNEntry->mRestartSN <= aFlushSN)
                {
                    *aRestartSN = sRestartSNEntry->mRestartSN;
                    IDU_LIST_REMOVE(sNode);
                    (void)mRestartSNPool.memfree(sRestartSNEntry);
                }
                else //(sRestartSNEntry->mRestartSN > aFlushSN)
                {
                    break;
                }
            }
        }
    }

    return;
}

smSN rpxSenderApply::getMinRestartSN( void )
{
    return mMinRestartSN;
}

void rpxSenderApply::getMinRestartSNFromAllApply( smSN* aRestartSN )
{
    return mSender->getMinRestartSNFromAllApply( aRestartSN );
}

IDE_RC rpxSenderApply::checkHBT( void )
{
    if ( ( mMeta->mReplication.mRole != RP_ROLE_ANALYSIS ) &&   // PROJ-1537
         ( mMeta->mReplication.mRole != RP_ROLE_ANALYSIS_PROPAGATION ) &&
         ( *mSenderType != RP_OFFLINE ) &&                      // PROJ-1915
         ( mSocketType != RP_SOCKET_TYPE_IB ) )
    {
        IDU_FIT_POINT( "rpxSenderApply::checkHBT::SLEEP::rpcHBT::checkFault" );
        IDE_TEST_RAISE( rpcHBT::checkFault(mRsc) == ID_TRUE,
                        ERR_NETWORK );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) );
        *mRetryError = ID_TRUE;
        mSenderInfo->checkAndRunDeactivate();
        mSenderInfo->signalToAllServiceThr( ID_TRUE, SM_NULL_TID );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
