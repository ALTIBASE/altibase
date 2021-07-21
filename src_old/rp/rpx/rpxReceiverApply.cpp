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
 * $Id: rpxReceiverApply.cpp 90444 2021-04-02 10:15:58Z minku.kang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idm.h>

#include <smi.h>
#include <rpDef.h>
#include <rpuProperty.h>
#include <rpdQueue.h>
#include <rpxReceiverApply.h>
#include <rpdMeta.h>
#include <rpxReceiver.h>
#include <rpxSender.h>
#include <rpcManager.h>
#include <rpdCatalog.h>
#include <rpdConvertSQL.h>
#include <rpsSQLExecutor.h>
#include <qci.h>

#define RECEIVER_SQL_BUFFER_MAX         ( 64 * 1024 )

rpxApplyFunc rpxReceiverApply::mApplyFunc[] =
{
    applyNA,
    applyNA,                           // Transaction Begin
    applyTrCommit,                     // Transaction Commit
    applyTrAbort,                      // Transaction Rollback
    applyInsert,                       // DML: Insert
    applyUpdate,                       // DML: Update
    applyDelete,                       // DML: Delete
    applyNA,                           // Implicit Savepoint Set
    applySPSet,                        // Savepoint Set
    applySPAbort,                      // Abort to Savepoint
    applyNA,                           // Statement Begin
    applyNA,                           // Statement End
    applyNA,                           // Cursor Open
    applyNA,                           // Cursor Close
    applyLobCursorOpen,                // LOB Cursor open
    applyLobCursorClose,               // LOB Cursor close
    applyLobPrepareWrite,              // LOB Prepare for write
    applyLobPartialWrite,              // LOB Partial write
    applyLobFinishWrite,               // LOB Finish to write
    applyIgnore,                       // Keep Alive
    applyNA,                           // ACK
    applyIgnore,                       // Replication Stop
    applyIgnore,                           // Replication Flush
    applyNA,                           // Replication Flush ACK
    applyNA,                           // Stop ACK
    applyIgnore,                       // Handshake
    applyNA,                           // Handshake Ack
    applySyncPKBegin,                  // Incremental Sync Primary Key Begin
    applySyncPK,                       // Incremental Sync Primary Key
    applySyncPKEnd,                    // Incremental Sync Primary Key End
    applyFailbackEnd,                  // Failback End
    applySyncStart,                    // Sync Start
    applySyncEnd,                      // Sync End 
    applySyncInsert,                   // Sync Insert
    applyNA,                           // Ack End Rebuild index for RP Sync    
    applyLobTrim,                      // LOB Trim
    applyAckOnDML,                     // Ack On DML
    applyNA,                           // Ack Eager
    applyIgnore,                       // DDL Replicate Handshake
    applyNA,                           // DDL Replicate Handshake Ack
    applyNA,                           // DDL Replicate Execute
    applyNA,                           // DDL Replicate Execute Ack
    applyTruncate,                     // Truncate 
    applyNA,                           // Truncate Ack
    applyIgnore,                       // XA_START_REQ
    applyIgnore,                       // XA_START_REQ_ACK
    applyIgnore,                       // XA_PREPARE_REQ
    applyIgnore,                       // XA_PREPARE
    applyTrCommit,                     // XA_COMMIT
    applyIgnore                        // XA_END
};



rpxReceiverApply::rpxReceiverApply()
{
}

IDE_RC rpxReceiverApply::initialize( rpxReceiver        * aReceiver,
                                     rpReceiverStartMode  aStartMode )
{
    mTransTbl       = NULL;
    mAbortTxList    = NULL;
    mAbortTxCount   = 0;
    mClearTxList    = NULL;
    mClearTxCount   = 0;
    mSNMapMgr       = NULL;
    mSenderInfo     = NULL;
    mLastCommitSN       = 0;    // BUG-17634
    mIsBegunSyncStmt = ID_FALSE;
    mIsOpenedSyncCursor = ID_FALSE;
    /* disk table에 대해 rp sync를 하면, dpath insert를 위해 인덱스 상태가 invalid하게 변경됨.
     * sync작업을 실패 또는 완료하면 rebuild index하여 valid한 상태로 변경해야한다. */
    mRemoteTable = NULL;
    mSyncTableNumber = 0;
    mSQLBuffer = NULL;

    mInsertSuccessCount = 0;
    mInsertFailureCount = 0;
    mUpdateSuccessCount = 0;
    mUpdateFailureCount = 0;
    mDeleteSuccessCount = 0;
    mDeleteFailureCount = 0;
    mCommitCount = 0;
    mAbortCount = 0;
    mSyncTupleSuccessCount = 0;

    //BUG-27831 : valgrind uninitialize values
    mRestartSN      = SM_SN_NULL;
    mApplyXSN       = SM_SN_NULL;

    mPolicy = RPX_APPLY_POLICY_BY_PROPERTY;

    mAckForTransactionCommit = ID_FALSE;

    mTransactionFlag = SMI_TRANSACTION_NORMAL | (UInt)RPU_ISOLATION_LEVEL;

    mReceiver = aReceiver;
    mStatistics = &(aReceiver->mStatistics);

    mRepName = aReceiver->mRepName;

    /* BUG-38533 numa aware thread initialize */
    mStartMode = aStartMode;

    mRangeColumn = NULL;
    mRangeColumnCount = 0;

    mRole = (RP_ROLE)aReceiver->mMeta.mReplication.mRole;

    idlOS::memset( mImplSvpName, 
                   0, 
                   RP_SAVEPOINT_NAME_LEN + 1 );

    return IDE_SUCCESS;
}

void rpxReceiverApply::destroy()
{
    return;
}

IDE_RC rpxReceiverApply::initializeInLocalMemory( void )
{
    UInt        sTransactionPoolInitSize = 0;
    UInt        sStage                   = 0;
    idBool      sIsConflictWhenNotEnoughSpace;

    /* Thread의 run()에서만 사용하는 메모리를 할당한다. */

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::initializeThread::calloc::TransTbl",
                          ERR_MEMORY_ALLOC_TRANS_TABLE );
    // Replication Transaction Table을 생성한다.
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       1,
                                       ID_SIZEOF( rpdTransTbl ),
                                       (void **)&mTransTbl,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_TRANS_TABLE );

    new (mTransTbl) rpdTransTbl;
    if ( mStartMode == RP_RECEIVER_SYNC )
    {
        sTransactionPoolInitSize = 1;
    }
    else
    {
        sTransactionPoolInitSize = RPU_REPLICATION_TRANSACTION_POOL_SIZE;
    }

    if ( mReceiver->mStartMode != RP_RECEIVER_FAILOVER_USING_XLOGFILE )
    {
        IDE_TEST( mTransTbl->initialize( RPD_TRANSTBL_USING_TRANS_POOL,
                                         sTransactionPoolInitSize )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mTransTbl->initialize( RPD_TRANSTBL_USING_MEMORY_ALLOC,
                                         sTransactionPoolInitSize )
                  != IDE_SUCCESS );
    }
    sStage = 1;

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::initializeThread::calloc::AbortTxList",
                          ERR_MEMORY_ALLOC_ABORT_TX_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       smiGetTransTblSize(),
                                       ID_SIZEOF( rpTxAck ),
                                       (void **)&mAbortTxList,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_ABORT_TX_LIST );

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::initializeThread::calloc::ClearTxList",
                          ERR_MEMORY_ALLOC_ABORT_TX_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       smiGetTransTblSize(),
                                       ID_SIZEOF( rpTxAck ),
                                       (void **)&mClearTxList,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_CLEAR_TX_LIST );

    sIsConflictWhenNotEnoughSpace = mReceiver->isEagerReceiver();

    IDE_TEST( mSmExecutor.initialize( mStatistics,
                                      &(mReceiver->mMeta),
                                      sIsConflictWhenNotEnoughSpace )
              != IDE_SUCCESS );

    sStage = 2;

    IDU_FIT_POINT( "rpxReceiverApply::initializeThread::calloc::mSQLBuffer" );
    mSQLBufferLength = RECEIVER_SQL_BUFFER_MAX + 1;
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       mSQLBufferLength,
                                       ID_SIZEOF( SChar ),
                                       (void **)&mSQLBuffer,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_SQL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_TRANS_TABLE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiverApply::initializeThread",
                                  "mTransTbl" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_ABORT_TX_LIST );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiverApply::initializeThread",
                                  "mAbortTxList" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CLEAR_TX_LIST );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiverApply::initializeThread",
                                  "mClearTxList" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_SQL )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiverApply::initializeThread",
                                  "mSQLBuffer" ) );
    }
    IDE_EXCEPTION_END;

    /* TODO : 나중에 별도 Thread로 구현하면, 실패 로그를 남겨야 한다.
     * IDE_ERRLOG( IDE_RP_0 );
     */

    IDE_PUSH();

    switch( sStage )
    {
        case 2:
            mSmExecutor.destroy();
        case 1:
            mTransTbl->destroy();
            break;

        default :
            break;
    }

    if ( mSQLBuffer != NULL )
    {
        (void)iduMemMgr::free( mSQLBuffer );
        mSQLBuffer = NULL;
    }
    else
    {
        /* do nothing */
    }

    if( mAbortTxList != NULL )
    {
        (void)iduMemMgr::free( mAbortTxList );
        mAbortTxList = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( mClearTxList != NULL )
    {
        (void)iduMemMgr::free( mClearTxList );
        mClearTxList = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( mTransTbl != NULL )
    {
        (void)iduMemMgr::free( mTransTbl );
        mTransTbl = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();
    return IDE_FAILURE;
}

void rpxReceiverApply::finalizeInLocalMemory( void )
{
    if ( mTransTbl != NULL )
    {
        mTransTbl->destroy();

        (void)iduMemMgr::free( mTransTbl );
        mTransTbl = NULL;

        mSmExecutor.destroy();

        if ( mAbortTxList != NULL )
        {
            (void)iduMemMgr::free( mAbortTxList );
            mAbortTxList = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( mClearTxList != NULL )
        {
            (void)iduMemMgr::free( mClearTxList );
            mClearTxList = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( mSQLBuffer != NULL )
        {
            (void)iduMemMgr::free( mSQLBuffer );
            mSQLBuffer = NULL;
        }
        else
        {
            /* do nothing */
        }
    }

    return;
}

void rpxReceiverApply::shutdown() // call by executor
{
    if ( mSNMapMgr != NULL )
    {
        mSNMapMgr->setMaxSNs();
    }
    else
    {
        /* do nothing */
    }

    finalize();
    return;
}

//남아있는 트랜잭션 롤백은 스스로 처리되어야 함
void rpxReceiverApply::finalize() // call by receiver itself
{
    if ( mIsOpenedSyncCursor == ID_TRUE )
    {
        (void)mCursor.close();
    }
    if ( mIsBegunSyncStmt == ID_TRUE )
    {
        (void)mSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS );
    }
    if( mTransTbl != NULL )
    {
        /* 모든 Active Replication Transaction을 Rollback시킨다.*/
        mTransTbl->rollbackAllATrans();
    }

    if ( rpdMeta::isUseV6Protocol( &( mReceiver->mMeta.mReplication ) ) != ID_TRUE )
    {
        /* BUG-40557 sync 가 아니면 RebuildIndices 를 하지 않아도 된다. */
        if ( ( mStartMode == RP_RECEIVER_SYNC ) || 
             ( mSyncTableNumber != 0 ) )
        {
            if ( applyRebuildIndices ( this ) != IDE_SUCCESS )
            {
                IDE_SET( ideSetErrorCode( rpERR_ABORT_REBUILD_INDEX ) );
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }

        if( mStartMode == RP_RECEIVER_SYNC_CONDITIONAL )
        {
            (void)mReceiver->recoveryCondition( ID_FALSE );
        }
    }
    else
    {
        /* do nothing */
    }

    if ( mRemoteTable != NULL )
    {
        (void) iduMemMgr::free( mRemoteTable );
        mRemoteTable = NULL;
    }

    if ( mRangeColumn != NULL )
    {
        (void)iduMemMgr::free( mRangeColumn );
        mRangeColumn = NULL;
        mRangeColumnCount = 0;
    }
    else
    {
        /* do nothing */
    }

    return;
}

IDE_RC rpxReceiverApply::execXLog(rpdXLog *aXLog)
{
    if(mTransTbl->isATrans(aXLog->mTID) != ID_TRUE)
    {
        switch(aXLog->mType)
        {
            case RP_X_COMMIT:
            case RP_X_XA_COMMIT:
            case RP_X_ABORT:
                if ( mTransTbl->isGTrans( aXLog->mTID ) != ID_TRUE )
                {
                    /* BUG-48391 */
                    IDE_CONT( NORMAL_EXIT );
                    //                  IDE_RAISE(ERR_TX_NOT_BEGIN);
                }
                else
                {
                    IDE_CONT( NORMAL_EXIT );
                }
                break;

            case RP_X_BEGIN:
            case RP_X_IMPL_SP_SET:
            case RP_X_SP_ABORT:
            case RP_X_LOB_CURSOR_CLOSE:
            case RP_X_LOB_PREPARE4WRITE:
            case RP_X_LOB_PARTIAL_WRITE:
            case RP_X_LOB_FINISH2WRITE:
            case RP_X_LOB_TRIM:
                IDE_RAISE(ERR_TX_NOT_BEGIN);

            case RP_X_INSERT:
            case RP_X_UPDATE:
            case RP_X_DELETE:
            case RP_X_SP_SET:
            case RP_X_LOB_CURSOR_OPEN:
            case RP_X_SYNC_INSERT:
                IDE_TEST( applyTrBegin( this, aXLog ) != IDE_SUCCESS );
                break;

            case RP_X_KEEP_ALIVE:
            case RP_X_REPL_STOP:
            case RP_X_HANDSHAKE:
            case RP_X_DDL_REPLICATE_HANDSHAKE:
            case RP_X_SYNC_PK_BEGIN:
            case RP_X_SYNC_PK:
            case RP_X_SYNC_PK_END:
            case RP_X_FAILBACK_END:
            case RP_X_SYNC_START:
            case RP_X_SYNC_END:
            case RP_X_FLUSH:
            case RP_X_ACK_ON_DML:
            case RP_X_TRUNCATE:
                break;

            case RP_X_XA_START_REQ:
                mTransTbl->setGTrans( aXLog->mTID );
                break;

            case RP_X_XA_PREPARE:
                mTransTbl->setGTrans( aXLog->mTID );
                break;

            case RP_X_XA_PREPARE_REQ:
            case RP_X_XA_END:
                break;

            default:
                IDE_RAISE(ERR_TX_NOT_BEGIN);
        }
    }

    //BUG-21858 : LOB 로그 무시 플레그를 언셋 한다.
    switch(aXLog->mType)
    {
        case RP_X_LOB_CURSOR_CLOSE:
        case RP_X_LOB_PREPARE4WRITE:
        case RP_X_LOB_PARTIAL_WRITE:
        case RP_X_LOB_FINISH2WRITE:
        case RP_X_LOB_CURSOR_OPEN:
        case RP_X_LOB_TRIM:
        case RP_X_KEEP_ALIVE:
        case RP_X_REPL_STOP:
        case RP_X_HANDSHAKE:
        case RP_X_DDL_REPLICATE_HANDSHAKE:
        case RP_X_SYNC_PK_BEGIN:
        case RP_X_SYNC_PK:
        case RP_X_SYNC_PK_END:
        case RP_X_FAILBACK_END:
        case RP_X_ACK_ON_DML:
        case RP_X_FLUSH:
            break;

        case RP_X_COMMIT:
        case RP_X_XA_COMMIT:
        case RP_X_ABORT:
            break;

        default:
            mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_FALSE);
            break;
    }

    IDE_TEST(rpxReceiverApply::mApplyFunc[aXLog->mType](this,
                                                        aXLog)
             != IDE_SUCCESS);

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TX_NOT_BEGIN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_TX_NOT_BEGIN,
                                aXLog->mType,
                                aXLog->mTID,
                                aXLog->mSN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReceiverApply::setTransactionFlagReplReplicated( void )
{
    mTransactionFlag = (mTransactionFlag & ~SMI_TRANSACTION_REPL_MASK) |
        SMI_TRANSACTION_REPL_REPLICATED;
}

/*
 *
 */
void rpxReceiverApply::setTransactionFlagReplRecovery( void )
{
    mTransactionFlag = (mTransactionFlag & ~SMI_TRANSACTION_REPL_MASK) |
        SMI_TRANSACTION_REPL_RECOVERY;
}

/*
 *
 */
void rpxReceiverApply::setTransactionFlagCommitWriteWait( void )
{
    mTransactionFlag = (mTransactionFlag & ~SMI_COMMIT_WRITE_MASK)
        | SMI_COMMIT_WRITE_WAIT;
}

/*
 *
 */
void rpxReceiverApply::setTransactionFlagCommitWriteNoWait( void )
{
    mTransactionFlag = (mTransactionFlag & ~SMI_COMMIT_WRITE_MASK)
        | SMI_COMMIT_WRITE_NOWAIT;
}

IDE_RC rpxReceiverApply::begin( smiTrans *aSmiTrans, idBool aIsConflictResolutionTX )
{
    smiStatement     *spRootStmt = NULL;
    UInt              sFlag = mTransactionFlag;
    idvSQL           *sStatistics = NULL;
    PDL_Time_Value    sWaitTime;
    UInt              sCnt;
    UInt              i;

    IDE_ASSERT(aSmiTrans != NULL);

    /* BUG-33539
     * standby에서 LOCK_ESCALATION_MEMORY_SIZE가 active 보다 작을 때
     * receiver에서 lock escalation이 발생하면 receiver가 self deadlock 상태가 됩니다. */
    if ( qci::getInplaceUpdateDisable() == ID_FALSE )
    {
        sFlag = (sFlag & ~SMI_TRANS_INPLACE_UPDATE_MASK) | SMI_TRANS_INPLACE_UPDATE_TRY;
    }
    else
    {
        /* PROJ-2626 Snapshot Export
         * Snapshot 이 begin 중 이라면 이전 Row를 읽어야하는데
         * Inplace Update가 되면 이전 Row를 볼 수 었기때문에 Disable  시킵니다.
         */
        sFlag = (sFlag & ~SMI_TRANS_INPLACE_UPDATE_MASK) | SMI_TRANS_INPLACE_UPDATE_DISABLE;
    }

    sWaitTime.set(0, 100000);                   // 0.1초
    /* 다음의 sCnt를 구하는 식은 waitTime이 100000microsec로 고정되어 있으므로 구해지는 값이다 */
    sCnt = RPU_REPLICATION_TX_VICTIM_TIMEOUT * 10;

    if ( mRole == RP_ROLE_PROPAGABLE_LOGGING )
    {
        /* propagation이 설정된 것은 receciver에의해 수행되었다 하더라도
         * 다시 전송해야 하기 때문에 replicated가 아닌 것으로 트랜잭션 실행
         */
        sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK)
                        | SMI_TRANSACTION_REPL_PROPAGATION;
    }
    else
    {
        /* nothing to do */
    }

    if ( RPU_REPLICATION_COMMIT_WRITE_WAIT_MODE != 0 )
    {
        // 복구된 트랜잭션이 다시 사라지는 것을 방지하기 위해 commit write wait으로 수행 함
        // Receiver가 Commit 시 Disk Sync 대기를 할 수 있도록 함
        sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK)
                        | SMI_COMMIT_WRITE_WAIT;
    }
    else
    {
        /* nothing to do */
    }

    /* TASK-6548 unique key duplication */
    if ( aIsConflictResolutionTX == ID_TRUE )
    {
        sFlag = ( sFlag & ~SMI_TRANSACTION_REPL_CONFLICT_RESOLUTION_MASK ) |
                           SMI_TRANSACTION_REPL_CONFLICT_RESOLUTION;
    }
    else
    {
        /* nothing to do */
    }

    if ( mReceiver->mStartMode == RP_RECEIVER_FAILOVER_USING_XLOGFILE )
    {
        sFlag = (sFlag & ~SMI_TRANS_GCTX_MASK) | SMI_TRANS_GCTX_ON;
        sStatistics = NULL;
    }
    else
    {
        sStatistics = mStatistics;
    }

    /* BUG-27709 receiver에 의한 반영 중, 트랜잭션 alloc실패 시 해당 receiver종료. */
    for(i = 0; i <= sCnt; i++)
    {
        if( aSmiTrans->begin(&spRootStmt,
                             sStatistics,
                             sFlag,
                             mReceiver->mReplID, // PROJ-1553 Self Deadlock
                             ID_TRUE)
           != IDE_SUCCESS )
        {
            IDE_TEST_RAISE(i >= sCnt, ERR_ALLOC_TIMEOUT);
            idlOS::sleep(sWaitTime);
        }
        else
        {
            IDE_ASSERT(spRootStmt != NULL);
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALLOC_TIMEOUT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TX_ALLOC_TIMEOUT ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::abort( rpdXLog * aXLog )
{
    rpdTransTblNode * sTblNode = NULL;

    sTblNode = mTransTbl->getTrNode( aXLog->mTID );
    IDE_TEST_RAISE( sTblNode->mTrans.rollback( aXLog->mTID ) != IDE_SUCCESS, ERR_TX_ABORT );

    if ( IDE_TRC_RP_CONFLICT_3 )
    {
        if ( mTransTbl->getIsConflictFlag( aXLog->mTID ) == ID_TRUE )
        {
            ideLogEntry sLog( IDE_RP_CONFLICT_3 );
            (void)abortConflictLog( sLog, aXLog );
            sLog.write();
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

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TX_ABORT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_TX_ERROR));
        IDE_ERRLOG(IDE_RP_0);

        if( IDE_TRC_RP_3 )
        {
            abortErrLog();
        }

        IDE_CALLBACK_FATAL("[Repl ReceiverApply] Transaction Rollback Failure");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpxReceiverApply::commit( rpdXLog * aXLog )
{
    rpdTransTblNode * sTblNode = NULL;

    sTblNode = mTransTbl->getTrNode( aXLog->mTID );

    IDU_FIT_POINT( "1.TASK-2004@rpxReceiverApply::commit" );

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::commit::Erratic::rpERR_ABORT_TX_COMMIT2",
                         err_tx,
                         smERR_ABORT_smiUpdateStatementExist );

    IDE_TEST_RAISE( sTblNode->mTrans.commit( aXLog->mTID ) != IDE_SUCCESS, err_tx );

    if ( IDE_TRC_RP_CONFLICT_3 )
    {
        if ( mTransTbl->getIsConflictFlag( aXLog->mTID ) == ID_TRUE )
        {
            ideLogEntry sLog( IDE_RP_CONFLICT_3 );
            (void)commitConflictLog( sLog, aXLog );
            sLog.write();
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

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_tx);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_TX_COMMIT2));
        IDE_ERRLOG(IDE_RP_0);

        if( IDE_TRC_RP_3 )
        {
            commitErrLog();
        }
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if( sTblNode != NULL )
    {
        //BUG-16777 when commit fail by max row
        IDE_ASSERT( sTblNode->mTrans.rollback( aXLog->mTID ) == IDE_SUCCESS);
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::insertSyncXLog( smiTrans       * aSmiTrans,
                                         rpdMetaItem    * aMetaItem,
                                         rpdXLog        * aXLog )
{
    idBool              sIsResolutionNeed   = ID_FALSE;
    smiRange            sKeyRange;
    idBool              sCheckRowExistence = ID_TRUE;
    rpApplyFailType     sFailType = RP_APPLY_FAIL_NONE;

    IDE_DASSERT( aSmiTrans != NULL );

    // BUG-10343
    if ( mSmExecutor.executeSyncInsert( aXLog,
                                        aSmiTrans,
                                        &mSmiStmt,
                                        &mCursor,
                                        aMetaItem,
                                        mSyncTupleSuccessCount,
                                        &mIsBegunSyncStmt,
                                        &mIsOpenedSyncCursor,
                                        &sFailType )
         != IDE_SUCCESS )
    {
        IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );

        if ( (sFailType == RP_APPLY_FAIL_BY_CONFLICT) || 
             (sFailType == RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX) )
        {
            IDE_TEST_RAISE( mSyncTupleSuccessCount > 0, ERR_SYNC_FAILED_BY_CONFLICT );

            IDE_TEST( getKeyRange( aMetaItem, &sKeyRange, aXLog->mACols, ID_FALSE )
                      != IDE_SUCCESS );

            IDE_TEST( getCheckRowExistenceAndResolutionNeed( aSmiTrans,
                                                             aMetaItem,
                                                             &sKeyRange,
                                                             aXLog,
                                                             &sCheckRowExistence,
                                                             &sIsResolutionNeed,
                                                             &sFailType )
                      != IDE_SUCCESS );


            if ( sIsResolutionNeed == ID_TRUE )  
            {
                IDE_TEST( insertReplace( aXLog,
                                         aMetaItem,
                                         &sFailType,
                                         sCheckRowExistence,
                                         NULL )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            IDE_DASSERT( sFailType == RP_APPLY_FAIL_BY_CORRUPTED_PAGE );
        }

    }

    /* RP_APPLY_FAIL_BY_CONFLICT || RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX || RP_APPLY_FAIL_BY_CORRUPTED_PAGE */
    if ( sFailType != RP_APPLY_FAIL_NONE )
    {
        printInsertErrLog( aMetaItem, aMetaItem, aXLog, sFailType );

        // Increase INSERT_FAILURE_COUNT in V$REPRECEIVER
        mInsertFailureCount++;

        //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
        mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);
    }
    else
    {
        // Increase INSERT_SUCCESS_COUNT in V$REPRECEIVER
        mInsertSuccessCount++;
        mSyncTupleSuccessCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SYNC_FAILED_BY_CONFLICT );
    {
        printInsertErrLog( aMetaItem, aMetaItem, aXLog, sFailType );

        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SYNC_FAILED_BY_BULK_INSERT ) );
    }
    IDE_EXCEPTION_END;

    // Increase INSERT_FAILURE_COUNT in V$REPRECEIVER
    mInsertFailureCount++;

    //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
    mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::insertXLog( smiTrans       * aSmiTrans,
                                     rpdMetaItem    * aMetaItem,
                                     rpdXLog        * aXLog,
                                     SChar          * aSPName )
{
    idBool              sIsResolutionNeed   = ID_FALSE;
    smiRange            sKeyRange;
    idBool              sCheckRowExistence = ID_TRUE;
    rpApplyFailType     sFailType = RP_APPLY_FAIL_NONE;
    smiTrans          * sConflictResolutionTrans = NULL;

    IDE_ASSERT(aSmiTrans != NULL);

    // BUG-10343
    if(mSmExecutor.executeInsert(aSmiTrans,
                                 aXLog,
                                 aMetaItem,
                                 aMetaItem->mIndexTableRef,
                                 &sFailType )
       != IDE_SUCCESS)
    {
        IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );

        if ( sFailType == RP_APPLY_FAIL_BY_CONFLICT )
        {
            IDE_TEST( getKeyRange( aMetaItem, 
                                   &sKeyRange, 
                                   aXLog->mACols, 
                                   ID_FALSE )
                      != IDE_SUCCESS ); 

            IDE_TEST( getCheckRowExistenceAndResolutionNeed( aSmiTrans,
                                                             aMetaItem,
                                                             &sKeyRange,
                                                             aXLog,
                                                             &sCheckRowExistence,
                                                             &sIsResolutionNeed,
                                                             &sFailType )
                      != IDE_SUCCESS );
            
            if ( sIsResolutionNeed == ID_TRUE )  
            {
                IDE_TEST( insertReplace( aXLog,
                                         aMetaItem,
                                         &sFailType,
                                         sCheckRowExistence,
                                         aSPName )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else if ( sFailType ==  RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX )
        {   /* TASK-6548 unique key duplication */

            sConflictResolutionTrans = mTransTbl->getSmiTransForConflictResolution( aXLog->mTID );

            if ( sConflictResolutionTrans != NULL )
            {
                if ( mSmExecutor.executeInsert( sConflictResolutionTrans,
                                                aXLog,
                                                aMetaItem,
                                                aMetaItem->mIndexTableRef,
                                                &sFailType )
                     != IDE_SUCCESS )
                {
                    IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );
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
        }
        else
        {
            IDE_DASSERT( sFailType == RP_APPLY_FAIL_BY_CORRUPTED_PAGE );
        }
    }

    /* RP_APPLY_FAIL_BY_CONFLICT || RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX || RP_APPLY_FAIL_BY_CORRUPTED_PAGE */
    if ( sFailType != RP_APPLY_FAIL_NONE )
    {
        printInsertErrLog( aMetaItem, aMetaItem, aXLog, sFailType );

        // Increase INSERT_FAILURE_COUNT in V$REPRECEIVER
        mInsertFailureCount++;
        addAbortTx(aXLog->mTID, aXLog->mSN);

        //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
        mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);
    }
    else
    {
        // Increase INSERT_SUCCESS_COUNT in V$REPRECEIVER
        mInsertSuccessCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    if ( sFailType == RP_APPLY_FAIL_INTERNAL )
    {
        ideLogEntry sLog(IDE_RP_0); // insertErrLog에서 복잡하기 때문에 상위에서 open,close를 함. more safe
        (void)insertErrLog(sLog, aMetaItem, aXLog, sFailType );
        sLog.write();
    }
    else
    {
        /* do nothing */
    }
    // Increase INSERT_FAILURE_COUNT in V$REPRECEIVER
    mInsertFailureCount++;
    addAbortTx(aXLog->mTID, aXLog->mSN);
    
    //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
    mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::insertReplace( rpdXLog          * aXLog,
                                        rpdMetaItem      * aMetaItem,
                                        rpApplyFailType  * aFailType,
                                        idBool             aCheckRowExistence,
                                        SChar            * aSPName )
{
    smiTrans * sTrans = NULL;
    UInt       sColCnt = 0;
    idBool     sIsSetSVP = ID_FALSE;
    smiRange   sKeyRange;
    UInt       sReplLockTimeout = 0;

    IDE_TEST( getConfictResolutionTransaction( aXLog->mTID,
                                               &sTrans )
              != IDE_SUCCESS );

    sReplLockTimeout = sTrans->getReplTransLockTimeout();
    IDE_TEST( sTrans->setReplTransLockTimeout( 0 ) != IDE_SUCCESS );

    IDE_TEST( getKeyRange( aMetaItem,
                           &sKeyRange,
                           aXLog->mACols,
                           ID_FALSE )
              != IDE_SUCCESS );

    /* Delete + Insert를 적용하기 전에 Savepoint를 두 개 설정한다. */
    if ( aSPName != NULL )
    {
        IDE_TEST_RAISE( sTrans->savepoint( aSPName ) != IDE_SUCCESS, ERR_SET_SVP );
    }
    else
    {
        /* Nothing to do */
    }
    IDE_TEST_RAISE( sTrans->savepoint( RP_CONFLICT_RESOLUTION_SVP_NAME )
                    != IDE_SUCCESS, ERR_SET_SVP );

    sIsSetSVP = ID_TRUE;

    // PROJ-1705
    // insert xlog정보로 delete를 하기 위한 fetch column list를 만들기
    // 위해서는 pk 정보가 필요하다.
    sColCnt = aXLog->mColCnt;
    aXLog->mPKColCnt = aMetaItem->mPKColCount;
    aXLog->mColCnt = 0;

    if ( mSmExecutor.executeDelete( sTrans,
                                    aXLog,
                                    aMetaItem,
                                    aMetaItem->mIndexTableRef,
                                    &sKeyRange,
                                    aFailType,
                                    aCheckRowExistence )
         != IDE_SUCCESS )
    {
        /* mPKColCnt와 mColCnt를 복원한다. */
        aXLog->mPKColCnt = 0;
        aXLog->mColCnt   = sColCnt;

        IDE_TEST( *aFailType == RP_APPLY_FAIL_INTERNAL );
        IDE_CONT(ERR_PASS);
    }
    else
    {
        /* nothing to do */
    }
    
    // PROJ-1705
    aXLog->mPKColCnt = 0;
    aXLog->mColCnt   = sColCnt;

    if( mSmExecutor.executeInsert( sTrans,
                                   aXLog,
                                   aMetaItem,
                                   aMetaItem->mIndexTableRef,
                                   aFailType )
        != IDE_SUCCESS)
    {
        IDE_TEST( *aFailType == RP_APPLY_FAIL_INTERNAL );
        IDE_CONT( ERR_PASS );
    }
    else
    {
        /* nothing to do */
    }

    sIsSetSVP = ID_FALSE;

    RP_LABEL( ERR_PASS );

    /* Conflict Resolution이 실 패하면, Delete + Insert를 취소한다. */
    if ( sIsSetSVP == ID_TRUE )
    {
        sIsSetSVP = ID_FALSE;
        IDE_TEST_RAISE( sTrans->rollback( RP_CONFLICT_RESOLUTION_SVP_NAME, SMI_DO_NOT_RELEASE_TRANSACTION )
                        != IDE_SUCCESS, ERR_ABORT_SVP );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( sTrans->setReplTransLockTimeout( sReplLockTimeout ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_SVP );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN ) );
    }
    IDE_EXCEPTION(ERR_ABORT_SVP);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_ABORT_SAVEPOINT_ERROR_IN_RUN));
    }
    IDE_EXCEPTION_END;
    
    if ( sIsSetSVP == ID_TRUE )
    {
        sIsSetSVP = ID_FALSE;
        (void)sTrans->rollback( RP_CONFLICT_RESOLUTION_SVP_NAME, 
                                SMI_DO_NOT_RELEASE_TRANSACTION );
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::updateXLog( smiTrans       * aSmiTrans,
                                     rpdMetaItem    * aMetaItem,
                                     rpdXLog        * aXLog )
{
    smiRange           sKeyRange;
    idBool             sCompareBeforeImage = ID_TRUE;
    rpApplyFailType    sFailType = RP_APPLY_FAIL_NONE;
    smiTrans *         sConflictResolutionTrans = NULL;

    IDE_ASSERT(aSmiTrans != NULL);

    switch ( mPolicy )
    {
        case  RPX_APPLY_POLICY_BY_PROPERTY:
            if ( RPU_REPLICATION_UPDATE_REPLACE == 1 )
            {
                sCompareBeforeImage = ID_FALSE;
            }
            else
            {
                sCompareBeforeImage = ID_TRUE;
            }
            break;

        case RPX_APPLY_POLICY_CHECK:
            sCompareBeforeImage = ID_TRUE;
            break;

        case RPX_APPLY_POLICY_FORCE:
            sCompareBeforeImage = ID_FALSE;
            break;
    }
    IDE_TEST( getKeyRange( aMetaItem, &sKeyRange, aXLog->mPKCols, ID_TRUE )
             != IDE_SUCCESS);

    if ( mTransTbl->isNullTransForConflictResolution( aXLog->mTID ) == ID_TRUE )
    {
        /*do nothing*/
    }
    else /* not null */
    {
        IDE_TEST( aSmiTrans->setReplTransLockTimeout( 0 ) != IDE_SUCCESS );
    }

    if(mSmExecutor.executeUpdate(aSmiTrans,
                                 aXLog,
                                 aMetaItem,
                                 aMetaItem->mIndexTableRef,
                                 &sKeyRange,
                                 &sFailType,
                                 sCompareBeforeImage,
                                 aMetaItem->mTsFlag)
       != IDE_SUCCESS)
    {
        IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );

        if ( sFailType == RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX )
        {
            sConflictResolutionTrans = mTransTbl->getSmiTransForConflictResolution( aXLog->mTID );

            if ( sConflictResolutionTrans != NULL )
            {
                if ( mSmExecutor.executeUpdate( sConflictResolutionTrans,
                                                aXLog,
                                                aMetaItem,
                                                aMetaItem->mIndexTableRef,
                                                &sKeyRange,
                                                &sFailType,
                                                sCompareBeforeImage,
                                                aMetaItem->mTsFlag )
                     != IDE_SUCCESS )
                {
                    IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );
                    IDE_ERRLOG( IDE_RP_0 );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Noghing to do */
            }
        }
        else
        {
            /* Noghing to do */
        }
    }

    if ( sFailType != RP_APPLY_FAIL_NONE )
    {
        printUpdateErrLog( aMetaItem, aMetaItem, aXLog, sCompareBeforeImage, sFailType );

        // Increase UPDATE_FAILURE_COUNT in V$REPRECEIVER
        mUpdateFailureCount++;
        addAbortTx(aXLog->mTID, aXLog->mSN);

        //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
        mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);
    }
    else
    {
        // Increase UPDATE_SUCCESS_COUNT in V$REPRECEIVER
        mUpdateSuccessCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    if ( sFailType == RP_APPLY_FAIL_INTERNAL )
    {
        ideLogEntry sLog(IDE_RP_0); // insertErrLog에서 복잡하기 때문에 상위에서 open,close를 함. more safe
        /* BUG-36555 : Before image logging */
        (void)updateErrLog( sLog,
                            aMetaItem, 
                            aMetaItem,
                            aXLog, 
                            sCompareBeforeImage,
                            sFailType );
        sLog.write();
    }
    else
    {
        /* do nothing */
    }
    // Increase UPDATE_FAILURE_COUNT in V$REPRECEIVER
    mUpdateFailureCount++;
    addAbortTx(aXLog->mTID, aXLog->mSN);

    //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
    mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::deleteXLog(smiTrans *aSmiTrans,
                                    rpdXLog  *aXLog)
{
    rpdMetaItem       *sMetaItem   = NULL;
    smiRange           sKeyRange;
    rpApplyFailType    sFailType = RP_APPLY_FAIL_NONE;
    idBool             sCheckRowExistence = ID_TRUE;
    smiTrans         * sConflictResolutionTrans = NULL;

    IDE_ASSERT(aSmiTrans != NULL);

    switch ( mPolicy )
    {
        case RPX_APPLY_POLICY_BY_PROPERTY:
        case RPX_APPLY_POLICY_CHECK:
            sCheckRowExistence = ID_TRUE;
            break;

        case RPX_APPLY_POLICY_FORCE:
            sCheckRowExistence = ID_FALSE;
            break;
    }

    // check existence of TABLE
    IDE_TEST( mReceiver->searchRemoteTable( &sMetaItem, aXLog->mTableOID )
              != IDE_SUCCESS );

    IDE_TEST_RAISE(sMetaItem == NULL, ERR_NOT_EXIST_TABLE);

    IDE_TEST(getKeyRange(sMetaItem, &sKeyRange, aXLog->mPKCols, ID_TRUE)
             != IDE_SUCCESS);
    if ( mTransTbl->isNullTransForConflictResolution( aXLog->mTID ) == ID_TRUE )
    {
        /*do nothing*/
    }
    else /* not null */
    {
        IDE_TEST( aSmiTrans->setReplTransLockTimeout( 0 ) != IDE_SUCCESS );
    }
    if(mSmExecutor.executeDelete(aSmiTrans,
                                 aXLog,
                                 sMetaItem,
                                 sMetaItem->mIndexTableRef,
                                 &sKeyRange,
                                 &sFailType,
                                 sCheckRowExistence)
       != IDE_SUCCESS)
    {
        IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );

        if ( sFailType == RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX )
        {
            sConflictResolutionTrans = mTransTbl->getSmiTransForConflictResolution( aXLog->mTID );

            if ( sConflictResolutionTrans != NULL )
            {
                if ( mSmExecutor.executeDelete( sConflictResolutionTrans,
                                                aXLog,
                                                sMetaItem,
                                                sMetaItem->mIndexTableRef,
                                                &sKeyRange,
                                                &sFailType,
                                                sCheckRowExistence)
                     != IDE_SUCCESS )
                {
                    IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );
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
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sFailType != RP_APPLY_FAIL_NONE )
    {
        printDeleteErrLog( sMetaItem, aXLog, sFailType );

        // Increase DELETE_FAILURE_COUNT in V$REPRECEIVER
        mDeleteFailureCount++;
        addAbortTx(aXLog->mTID, aXLog->mSN);
    }
    else
    {
        // Increase DELETE_SUCCESS_COUNT in V$REPRECEIVER
        mDeleteSuccessCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        ideLog::log(IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                              aXLog->mType,
                              aXLog->mTID,
                              aXLog->mSN,
                              aXLog->mTableOID);

        IDE_SET(ideSetErrorCode(rpERR_ABORT_NOT_EXIST_TABLE_DEL));
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    if ( sFailType == RP_APPLY_FAIL_INTERNAL )
    {
        ideLog::log( IDE_RP_0, " delete XLog error occur " );
        ideLogEntry sLog(IDE_RP_0); // insertErrLog에서 복잡하기 때문에 상위에서 open,close를 함. more safe
        (void)deleteErrLog(sLog, sMetaItem, aXLog, sFailType);
        sLog.write();
    }
    else
    {
        /* do nothing */
    }
    // Increase DELETE_FAILURE_COUNT in V$REPRECEIVER
    mDeleteFailureCount++;
    addAbortTx(aXLog->mTID, aXLog->mSN);
    return IDE_FAILURE;
}

//* BUG-10344 E *//
IDE_RC rpxReceiverApply::getKeyRange(rpdMetaItem *aMetaItem,
                                     smiRange    *aKeyRange,
                                     smiValue    *aColArray,
                                     idBool       aIsPKColArray)
{
    UInt          i;
    UInt          sOrder;
    UInt          sFlag;
    UInt          sCompareType;
    UInt          sPKColumnID;

    IDE_ASSERT(aMetaItem->mPKIndex.keyColumns != NULL);
    IDE_ASSERT(aMetaItem->mPKIndex.keyColsFlag != NULL);

    /* Proj-1872 DiskIndex 최적화
     * Range를 적용하는 대상이 DiskIndex일 경우, Stored 타입을 고려한
     * Range가 적용 되어야 한다. */
    sFlag = (aMetaItem->mPKIndex.keyColumns)->column.flag;
    if(  ( ( sFlag & SMI_COLUMN_STORAGE_MASK ) == SMI_COLUMN_STORAGE_DISK ) &&
         ( ( sFlag & SMI_COLUMN_USAGE_MASK   ) == SMI_COLUMN_USAGE_INDEX  ) )
    {
        sCompareType = MTD_COMPARE_STOREDVAL_MTDVAL;
    }
    else
    {
        sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
    }

    qtc::initializeMetaRange( aKeyRange, sCompareType );

    for ( i = 0; i < aMetaItem->mPKIndex.keyColCount; i++ )
    {
        IDE_DASSERT( i <= mRangeColumnCount );
        sOrder = aMetaItem->mPKIndex.keyColsFlag[i] & SMI_COLUMN_ORDER_MASK;

        if(aIsPKColArray == ID_TRUE)
        {
            sPKColumnID = i;
        }
        else
        {   
            sPKColumnID = aMetaItem->mPKIndex.keyColumns[i].column.id & SMI_COLUMN_ID_MASK;
        }

        qtc::setMetaRangeColumn( &mRangeColumn[i],
                                 &aMetaItem->mPKIndex.keyColumns[i],
                                 aColArray[sPKColumnID].value,
                                 sOrder,
                                 i );

        qtc::addMetaRangeColumn( aKeyRange,
                                 &mRangeColumn[i] );
    }

    qtc::fixMetaRange( aKeyRange );

    return IDE_SUCCESS;
}

IDE_RC rpxReceiverApply::openLOBCursor(smiTrans        *aSmiTrans,
                                       rpdXLog         *aXLog,
                                       rpdTransTbl     *aTransTbl)
{
    rpdMetaItem * sMetaItem = NULL;
    smiRange      sKeyRange;
    idBool        sIsSync = ID_FALSE;
    idBool        sIsLOBOperationException = ID_FALSE;
    smiTrans    * sTrans = NULL;
    UInt          sReplLockTimeout = 0;
    UInt          sConFlictReplLockTimeout = 0;
    idBool        sIsBegunSyncStmt;
    rpApplyFailType sFailType = RP_APPLY_FAIL_NONE;
    
    IDE_ASSERT(aSmiTrans != NULL);
       
    // check existence of TABLE
    IDE_TEST( mReceiver->searchRemoteTable( &sMetaItem, aXLog->mTableOID )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::openLOBCursor::Erratic::rpERR_ABORT_NOT_EXIST_TABLE_INS",
                         ERR_NOT_EXIST_TABLE );
    IDE_TEST_RAISE(sMetaItem == NULL, ERR_NOT_EXIST_TABLE);

    IDE_TEST_RAISE( getKeyRange( sMetaItem,
                                 &sKeyRange,
                                 aXLog->mPKCols,
                                 ID_TRUE )
                    != IDE_SUCCESS, ERR_GET_KEYRANGE );

    sIsBegunSyncStmt = mIsBegunSyncStmt;
    
    if ( sIsBegunSyncStmt == ID_TRUE )
    {
        sIsSync = ID_TRUE;
        IDE_TEST( mSmExecutor.stmtEndAndCursorClose( &mSmiStmt,
                                                     &mCursor,
                                                     &mIsBegunSyncStmt,
                                                     &mIsOpenedSyncCursor,
                                                     &mSyncTupleSuccessCount,
                                                     SMI_STATEMENT_RESULT_SUCCESS )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    sReplLockTimeout = aSmiTrans->getReplTransLockTimeout();
    if ( mTransTbl->isNullTransForConflictResolution( aXLog->mTID ) == ID_TRUE )
    {
        /*do nothing*/
    }
    else /* not null */
    {
        IDE_TEST( aSmiTrans->setReplTransLockTimeout( 0 ) != IDE_SUCCESS );
    }

    if ( mSmExecutor.openLOBCursor( aSmiTrans,
                                    aXLog,
                                    sMetaItem,
                                    &sKeyRange,
                                    aTransTbl,
                                    &sIsLOBOperationException,
                                    &sFailType )
         != IDE_SUCCESS )
    {
        if ( sFailType == RP_APPLY_FAIL_BY_CONFLICT )
        {            
            IDE_TEST( getConfictResolutionTransaction( aXLog->mTID,
                                                       &sTrans )
                      != IDE_SUCCESS );
            
            sConFlictReplLockTimeout = sTrans->getReplTransLockTimeout();
            IDE_TEST( sTrans->setReplTransLockTimeout( 0 ) != IDE_SUCCESS );
            
            if ( mSmExecutor.openLOBCursor( sTrans,
                                            aXLog,
                                            sMetaItem,
                                            &sKeyRange,
                                            aTransTbl,
                                            &sIsLOBOperationException,
                                            &sFailType )
                 != IDE_SUCCESS )
            {
                IDE_TEST_RAISE( (sIsLOBOperationException != ID_TRUE) &&
                                (sFailType == RP_APPLY_FAIL_INTERNAL),
                                ERR_LOB_EXCEPTION );  
                
                addAbortTx(aXLog->mTID, aXLog->mSN);
            }
            else
            {
                /* Nothing to do */
            }
            
            IDE_TEST( sTrans->setReplTransLockTimeout( sConFlictReplLockTimeout ) != IDE_SUCCESS );  
        }
        else
        {
            IDE_TEST_RAISE( (sIsLOBOperationException != ID_TRUE) &&
                            (sFailType == RP_APPLY_FAIL_INTERNAL),
                            ERR_LOB_EXCEPTION );  

            addAbortTx(aXLog->mTID, aXLog->mSN);
        }
    }
    else
    {
        /* Nothing to do */
    }
    
    if ( sIsBegunSyncStmt == ID_TRUE )
    {
        IDE_TEST( mSmExecutor.stmtBeginAndCursorOpen( aSmiTrans,
                                                      &mSmiStmt,
                                                      &mCursor,
                                                      sMetaItem,
                                                      &mIsBegunSyncStmt,
                                                      &mIsOpenedSyncCursor )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( aSmiTrans->setReplTransLockTimeout( sReplLockTimeout ) != IDE_SUCCESS );    

    if ( sFailType != RP_APPLY_FAIL_NONE ) 
    {
        printLobErrLog( sMetaItem, aXLog, sFailType );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        ideLog::log(IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                    aXLog->mType,
                    aXLog->mTID,
                    aXLog->mSN,
                    aXLog->mTableOID);

        IDE_SET(ideSetErrorCode(rpERR_ABORT_NOT_EXIST_TABLE));
    }
    IDE_EXCEPTION( ERR_GET_KEYRANGE );
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                 "[rpxReceiverApply::openLOBCursor] failed getKeyRange" ) );
    }
    IDE_EXCEPTION( ERR_LOB_EXCEPTION );
    {
        IDE_SET(ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                 "[rpxReceiverApply::openLOBCursor] LOBOperationException is FALSE" ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    IDE_PUSH();

    if ( ( mIsBegunSyncStmt == ID_FALSE ) && ( sIsSync == ID_TRUE ) )
    {
        (void)mSmExecutor.stmtBeginAndCursorOpen( aSmiTrans,
                                                  &mSmiStmt,
                                                  &mCursor,
                                                  sMetaItem,
                                                  &mIsBegunSyncStmt,
                                                  &mIsOpenedSyncCursor );
    }

    addAbortTx(aXLog->mTID, aXLog->mSN);

    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::closeLOBCursor( smiTrans        *aSmiTrans,
                                         rpdXLog         *aXLog,
                                         rpdTransTbl     *aTransTbl )
{
    idBool        sIsLOBOperationException;

    IDE_ASSERT(aSmiTrans != NULL);

    if ( mSmExecutor.closeLOBCursor( aXLog,
                                     aTransTbl,
                                     &sIsLOBOperationException )
         != IDE_SUCCESS )
    {
        IDE_TEST( sIsLOBOperationException != ID_TRUE );

        addAbortTx(aXLog->mTID, aXLog->mSN);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    addAbortTx(aXLog->mTID, aXLog->mSN);
    
    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::prepareLOBWrite( smiTrans        *aSmiTrans,
                                          rpdXLog         *aXLog,
                                          rpdTransTbl     *aTransTbl )
{
    idBool        sIsLOBOperationException;

    IDE_ASSERT(aSmiTrans != NULL);

    if ( mSmExecutor.prepareLOBWrite( aXLog,
                                      aTransTbl,
                                      &sIsLOBOperationException )
         != IDE_SUCCESS )
    {
        IDE_TEST( sIsLOBOperationException != ID_TRUE );

        addAbortTx(aXLog->mTID, aXLog->mSN);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    addAbortTx(aXLog->mTID, aXLog->mSN);
    
    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::finishLOBWrite( smiTrans        *aSmiTrans,
                                         rpdXLog         *aXLog,
                                         rpdTransTbl     *aTransTbl )
{
    idBool        sIsLOBOperationException;

    IDE_ASSERT(aSmiTrans != NULL);

    if ( mSmExecutor.finishLOBWrite( aXLog,
                                     aTransTbl,
                                     &sIsLOBOperationException )
         != IDE_SUCCESS )
    {
        IDE_TEST( sIsLOBOperationException != ID_TRUE );

        addAbortTx(aXLog->mTID, aXLog->mSN);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    addAbortTx(aXLog->mTID, aXLog->mSN);
    
    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::trimLOB( smiTrans        *aSmiTrans,
                                  rpdXLog         *aXLog,
                                  rpdTransTbl     *aTransTbl )
{
    idBool        sIsLOBOperationException;

    IDE_ASSERT(aSmiTrans != NULL);

    if ( mSmExecutor.trimLOB( aXLog,
                              aTransTbl,
                              &sIsLOBOperationException )
         != IDE_SUCCESS )
    {
        IDE_TEST( sIsLOBOperationException != ID_TRUE );

        addAbortTx(aXLog->mTID, aXLog->mSN);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    addAbortTx(aXLog->mTID, aXLog->mSN);
    
    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::writeLOBPiece( smiTrans        *aSmiTrans,
                                        rpdXLog         *aXLog,
                                        rpdTransTbl     *aTransTbl )
{
    idBool        sIsLOBOperationException;

    IDE_ASSERT(aSmiTrans != NULL);

    if ( mSmExecutor.writeLOBPiece( aXLog,
                                    aTransTbl,
                                    &sIsLOBOperationException )
         != IDE_SUCCESS )
    {
        IDE_TEST( sIsLOBOperationException != ID_TRUE );

        addAbortTx(aXLog->mTID, aXLog->mSN);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    addAbortTx(aXLog->mTID, aXLog->mSN);
    
    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::closeAllLOBCursor( smiTrans        *aSmiTrans,
                                            smTID            aTID )
{
    smLobLocator     sRemoteLL;
    smLobLocator     sLocalLL;
    idBool           sIsExist;
    SChar            sErrorBuffer[256];

    IDE_ASSERT(aSmiTrans != NULL);

    IDE_TEST_RAISE( mTransTbl->getFirstLocator( aTID,
                                                &sRemoteLL,
                                                &sLocalLL,
                                                &sIsExist )
                    != IDE_SUCCESS, ERR_GET_FIRST_LOCATOR );

    while(sIsExist == ID_TRUE)
    {
        mTransTbl->removeLocator(aTID, sRemoteLL);

        IDE_TEST_RAISE( smiLob::closeLobCursor(NULL, /* idvSQL* */
                                               sLocalLL)
                        != IDE_SUCCESS, ERR_LOB_CURSOR_CLOSE );

        IDE_TEST_RAISE( mTransTbl->getFirstLocator( aTID,
                                                    &sRemoteLL,
                                                    &sLocalLL,
                                                    &sIsExist )
                        != IDE_SUCCESS, ERR_GET_FIRST_LOCATOR );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_LOB_CURSOR_CLOSE);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_CLOSE_LOB_CURSOR));
    }
    IDE_EXCEPTION( ERR_GET_FIRST_LOCATOR );
    {
        IDE_ERRLOG(IDE_RP_0);

        idlOS::snprintf( sErrorBuffer, 256,
                         "[rpxReceiverApply::closeAllLOBCursor] failed getFirstLocator "
                         "[TABLEID: %"ID_vULONG_FMT"]", aTID );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorBuffer ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyTrBegin
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   Begin Transaction Log이다. 새로운 Replication Transaction을 시작한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyTrBegin(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans      *sSmiTrans = NULL;
    rprSNMapEntry *sNewEntry = NULL;
    SInt           sStage    = 0;
    rpdMetaItem  * sMetaItem = NULL;
    rpdTransTblNode * sTblNode = NULL;

    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_R_TX_BEGIN);

    RP_OPTIMIZE_TIME_BEGIN(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_BEGIN);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_FALSE)
    {
        /* 새로운 Transaction을 Replication Transaction Tbl에 추가한다. */
//        IDE_TEST_RAISE(aApply->mTransTbl->insertTrans(NULL, /* memory allocator : not used */
        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyTrBegin::Erratic::rpERR_ABORT_PROTOCOL_RUN",
                             ERR_INSERT,
                             rpERR_ABORT_RP_ENTRY_EXIST,
                             "rpxReceiverApply::applyTrBegin::Erratic",
                             "cased by FIT TEST FOR Erratic Fault" ); 
        IDE_TEST_RAISE(aApply->mTransTbl->insertTrans(aApply->mReceiver->mAllocator, /* memory allocator : not used */
                                                      aXLog->mTID,
                                                      aXLog->mSN,
                                                      NULL)
                       != IDE_SUCCESS, ERR_INSERT);
        sStage = 1;

       /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
        * Transaction Table에서 구한다.
        */
        sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);
        IDE_TEST_RAISE( aApply->begin( sSmiTrans, ID_FALSE )
                       != IDE_SUCCESS, ERR_BEGIN);
        aApply->mTransTbl->setMyTID(aXLog->mTID);

        sStage = 2;

        if ( ( aApply->mSNMapMgr != NULL ) && ( aXLog->mSN != SM_SN_NULL ) )
        {
            /* recovery option이 set된 Normal receiver에 의해 실행된 트랜잭션은
             * sn map에 삽입한다.
             */
            IDE_TEST(aApply->mSNMapMgr->insertNewTx(aXLog->mSN, &sNewEntry)
                     != IDE_SUCCESS);
            aApply->mTransTbl->setSNMapEntry(aXLog->mTID, sNewEntry);
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* Begin Log가 왔는데 이미 Begin된 Transaction이다. */
        IDE_RAISE(ERR_PROTOCOL);
    }

    if ( aXLog->mType == RP_X_SYNC_INSERT )
    {
        IDE_TEST( aApply->mReceiver->searchRemoteTable( &sMetaItem, aXLog->mTableOID )
                  != IDE_SUCCESS );

        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyTrBegin::Erratic::rpERR_ABORT_NOT_EXIST_TABLE",
                             ERR_NOT_EXIST_TABLE );
        IDE_TEST_RAISE( sMetaItem == NULL, ERR_NOT_EXIST_TABLE);
        
        if ( sMetaItem->getApplyMode() == RP_APPLY_XLOG )
        {
            IDE_TEST( aApply->mSmExecutor.stmtBeginAndCursorOpen( sSmiTrans,
                                                                  &(aApply->mSmiStmt),
                                                                  &(aApply->mCursor),
                                                                  sMetaItem,
                                                                  &aApply->mIsBegunSyncStmt,
                                                                  &aApply->mIsOpenedSyncCursor )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }
    }

    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_BEGIN);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INSERT);
    {
        IDE_ERRLOG(IDE_RP_0);

        IDE_SET(ideSetErrorCode(rpERR_ABORT_PROTOCOL_RUN));

        IDE_ERRLOG(IDE_RP_0);

        IDE_CLEAR();
    }
    IDE_EXCEPTION(ERR_BEGIN);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_BEGIN_ERROR_IN_RUN));
    }
    IDE_EXCEPTION(ERR_PROTOCOL);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_PROTOCOL_RUN));
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE );
    {
        ideLog::log( IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                     aXLog->mType,
                     aXLog->mTID,
                     aXLog->mSN,
                     aXLog->mTableOID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_EXIST_TABLE ) );
    }
    IDE_EXCEPTION_END;
    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_BEGIN);

    IDE_ERRLOG(IDE_RP_0);

    if ( aXLog->mType != RP_X_SYNC_INSERT )
    {
        aApply->addAbortTx(aXLog->mTID, aXLog->mSN);
    }
    else
    {
        /* do nothing */
    }

    IDE_PUSH();

    switch(sStage)
    {
        case 2:
            sTblNode = aApply->mTransTbl->getTrNode( aXLog->mTID );
            (void)sTblNode->mTrans.rollback( aXLog->mTID );
        case 1:
            aApply->mTransTbl->removeTrans(aXLog->mTID);
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


//===================================================================
//
// Name:          applyTrCommit
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   Transaction Commit Log이다. Transaction을 Commit시킨다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyTrCommit(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans*      sSmiTrans = NULL;
    rpXLogAck      sAck;
    rprSNMapEntry* sSNMapEntry = NULL;
    SInt           sStage = 0;

    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_R_TX_COMMIT);

    RP_OPTIMIZE_TIME_BEGIN(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_COMMIT);

    if ( aApply->mIsBegunSyncStmt == ID_TRUE )
    {
        IDE_TEST( aApply->mSmExecutor.stmtEndAndCursorClose( &(aApply->mSmiStmt),
                                                             &(aApply->mCursor),
                                                             &aApply->mIsBegunSyncStmt,
                                                             &aApply->mIsOpenedSyncCursor,
                                                             &aApply->mSyncTupleSuccessCount,
                                                             SMI_STATEMENT_RESULT_SUCCESS )
                  != IDE_SUCCESS );
    }

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE)
    {
        /* Transaction 종료 시점에 남아있는 LOB Cursor가 있다면
         * 모두 Close 시킨다.
         */
        sStage = 2;

        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyTrCommit::Erratic::rpERR_ABORT_CLOSE_LOB_CURSOR",
                             ERR_CLOSE_LOB_CURSOR );
        IDE_TEST_RAISE(aApply->closeAllLOBCursor(sSmiTrans, aXLog->mTID)
                       != IDE_SUCCESS, ERR_CLOSE_LOB_CURSOR);

        //BUG-16777 when commit fail by max row
        sStage = 1;
        IDE_TEST_RAISE( aApply->commit( aXLog ) != IDE_SUCCESS, ERR_COMMIT );
        (aApply->mCommitCount)++;

        if ( ( aApply->mSNMapMgr != NULL ) && ( aXLog->mSN != SM_SN_NULL ) )
        {
            //setSNs
            sSNMapEntry = aApply->mTransTbl->getSNMapEntry(aXLog->mTID);

            aApply->mSNMapMgr->setSNs(sSNMapEntry,
                                      aXLog->mSN,
                                      sSmiTrans->getBeginSN(),
                                      sSmiTrans->getCommitSN());
        }
        else
        {
            /* nothing to do */
        }

        /* Replication Transaction Table에서 aXLog->mTID에 해당하는
         * Transaction Slot을 제거한다.
         */
        sStage = 0;
        aApply->mTransTbl->removeTrans(aXLog->mTID);
    }

    if ( aApply->mAckForTransactionCommit == ID_TRUE )
    {
        sAck.mAckType           = RP_X_ACK;
        sAck.mAbortTxCount      = 0;
        sAck.mClearTxCount      = 0;
        sAck.mRestartSN         = SM_SN_NULL;
        sAck.mLastCommitSN      = aXLog->mSN;
        sAck.mLastArrivedSN     = aXLog->mSN;
        sAck.mLastProcessedSN   = aXLog->mSN;
        sAck.mAbortTxList       = NULL;
        sAck.mClearTxList       = NULL;
        sAck.mFlushSN           = SM_SN_NULL;

        IDE_TEST( aApply->mReceiver->sendAck( &sAck ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_COMMIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CLOSE_LOB_CURSOR);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_CLOSE_LOB_CURSOR));
    }
    IDE_EXCEPTION(ERR_COMMIT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_COMMIT_ERROR_IN_RUN));
        if ( ( aApply->mSNMapMgr != NULL ) && ( aXLog->mSN != SM_SN_NULL ) )
        {
            //setSNs
            sSNMapEntry = aApply->mTransTbl->getSNMapEntry(aXLog->mTID);
            aApply->mSNMapMgr->deleteTxByEntry(sSNMapEntry);
        }
        else
        {
            /* nothing to do */
        }
    }
    IDE_EXCEPTION_END;
    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_COMMIT);

    IDE_ERRLOG(IDE_RP_0);

    IDE_PUSH();

    switch(sStage)
    {
        case 2:
            (void)aApply->abort( aXLog );
        case 1:
            aApply->mTransTbl->removeTrans(aXLog->mTID);
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyTrAbort
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   Transaction ABORT log이다. Begin된 Replication을 Abort시킨다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyTrAbort(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    SInt         sStage = 0;
    smiTrans*    sSmiTrans = NULL;

    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_R_TX_ABORT);

    RP_OPTIMIZE_TIME_BEGIN(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_ABORT);

    if ( aApply->mIsBegunSyncStmt == ID_TRUE )
    {
        IDE_TEST( aApply->mSmExecutor.stmtEndAndCursorClose( &(aApply->mSmiStmt),
                                                             &(aApply->mCursor),
                                                             &aApply->mIsBegunSyncStmt,
                                                             &aApply->mIsOpenedSyncCursor,
                                                             &aApply->mSyncTupleSuccessCount,
                                                             SMI_STATEMENT_RESULT_FAILURE )
                  != IDE_SUCCESS );
    }

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    if(aApply->mTransTbl->isATrans( aXLog->mTID ) == ID_TRUE)
    {
        /* Transaction 종료 시점에 남아있는 LOB Cursor가 있다면
         * 모두 Close 시킨다.
         */
        sStage = 2;
        IDE_TEST_RAISE( aApply->closeAllLOBCursor(sSmiTrans, aXLog->mTID)
                        != IDE_SUCCESS, ERR_CLOSE_LOB_CURSOR );

        sStage = 1;
        IDE_TEST_RAISE( aApply->abort( aXLog ) != IDE_SUCCESS,
                        ERR_ABORT );
        (aApply->mAbortCount)++;

        /* Replication Transaction Table에서 aXLog->mTID에 해당하는
         * Transaction Slot을 제거한다.
         */
        sStage = 0;
        aApply->mTransTbl->removeTrans(aXLog->mTID);
    }

    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_ABORT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CLOSE_LOB_CURSOR);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_CLOSE_LOB_CURSOR));
    }
    IDE_EXCEPTION(ERR_ABORT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_ABORT_ERROR_IN_RUN));
    }
    IDE_EXCEPTION_END;
    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_ABORT);

    IDE_ERRLOG(IDE_RP_0);

    IDE_PUSH();

    switch(sStage)
    {
        case 2:
            (void)aApply->abort( aXLog );
        case 1:
            aApply->mTransTbl->removeTrans(aXLog->mTID);
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyInsert
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   DML Insert Log이다. record를 Insert시킨다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyInsert(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans       * sSmiTrans = NULL;
    SChar          * sSpName   = NULL;
    rpdTransTblNode * sTblNode = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans( aXLog->mTID );

    IDE_DASSERT( aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE );

    sTblNode = aApply->mTransTbl->getTrNode( aXLog->mTID );

    if(aXLog->mImplSPDepth != SMI_STATEMENT_DEPTH_NULL)
    {
        sSpName = aApply->getSvpName( aXLog->mImplSPDepth, aApply->mImplSvpName );
        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyInsert::Erratic::rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN",
                             ERR_SET_SVP );

        IDE_TEST_RAISE( sTblNode->mTrans.setSavepoint( aXLog->mTID,
                                                       RP_SAVEPOINT_EXPLICIT,
                                                       sSpName )
                        != IDE_SUCCESS, ERR_SET_SVP );

        /* BUG-18028 Eager Mode에서 Partial Rollback 지원 */
        IDE_TEST_RAISE(aApply->mTransTbl->addLastSvpEntry(aXLog->mTID,
                                                          aXLog->mSN,
                                                          RP_SAVEPOINT_IMPLICIT,
                                                          sSpName,
                                                          aXLog->mImplSPDepth)
                       != IDE_SUCCESS, ERR_SET_SVP);

    }

    IDE_TEST( aApply->executeInsert( sSmiTrans,
                                     aXLog,
                                     sSpName ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SET_SVP);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN));
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    aApply->addAbortTx(aXLog->mTID, aXLog->mSN);

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReceiverApply::applySyncInsert( rpxReceiverApply * aApply, rpdXLog * aXLog )
{
    smiTrans        * sSmiTrans = NULL;
    rpdTransTblNode * sTblNode = NULL;

    sSmiTrans = aApply->mTransTbl->getSMITrans( aXLog->mTID );

    IDE_DASSERT( aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE );

    IDE_TEST( aApply->executeSyncInsert( sSmiTrans, aXLog ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void) aApply->mSmExecutor.stmtEndAndCursorClose( &(aApply->mSmiStmt),
                                                      &(aApply->mCursor),
                                                      &aApply->mIsBegunSyncStmt,
                                                      &aApply->mIsOpenedSyncCursor,
                                                      &aApply->mSyncTupleSuccessCount,
                                                      SMI_STATEMENT_RESULT_FAILURE );

    sTblNode = aApply->mTransTbl->getTrNode( aXLog->mTID );
    (void) sTblNode->mTrans.rollback( aXLog->mTID );

    aApply->mTransTbl->removeTrans(aXLog->mTID);

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyUpdate
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   DML Update Log이다. record를 Update 시킨다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyUpdate(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans       * sSmiTrans = NULL;
    SChar          * sSpName   = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    IDE_DASSERT( aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE );

    if(aXLog->mImplSPDepth != SMI_STATEMENT_DEPTH_NULL)
    {
        sSpName = aApply->getSvpName( aXLog->mImplSPDepth, aApply->mImplSvpName );

        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyUpdate::Erratic::rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN",
                             ERR_SET_SVP );
        IDE_TEST_RAISE(sSmiTrans->savepoint(sSpName)
                       != IDE_SUCCESS, ERR_SET_SVP);

        /* BUG-18028 Eager Mode에서 Partial Rollback 지원 */
        IDE_TEST_RAISE(aApply->mTransTbl->addLastSvpEntry(aXLog->mTID,
                                                          aXLog->mSN,
                                                          RP_SAVEPOINT_IMPLICIT,
                                                          sSpName,
                                                          aXLog->mImplSPDepth)
                       != IDE_SUCCESS, ERR_SET_SVP);
    }

    IDE_TEST( aApply->executeUpdate( sSmiTrans,
                                     aXLog ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SET_SVP);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN));
    } 
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    aApply->addAbortTx(aXLog->mTID, aXLog->mSN);

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyDelete
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   DML Delete 로그이다. record를 Delete한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyDelete(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans *sSmiTrans = NULL;
    SChar    *sSpName   = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    IDE_DASSERT( aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE );

    if(aXLog->mImplSPDepth != SMI_STATEMENT_DEPTH_NULL)
    {
        sSpName = aApply->getSvpName( aXLog->mImplSPDepth, aApply->mImplSvpName );

        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyDelete::Erratic::rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN",
                             ERR_SET_SVP );
        IDE_TEST_RAISE(sSmiTrans->savepoint(sSpName)
                       != IDE_SUCCESS, ERR_SET_SVP);

        /* BUG-18028 Eager Mode에서 Partial Rollback 지원 */
        IDE_TEST_RAISE(aApply->mTransTbl->addLastSvpEntry(aXLog->mTID,
                                                          aXLog->mSN,
                                                          RP_SAVEPOINT_IMPLICIT,
                                                          sSpName,
                                                          aXLog->mImplSPDepth)
                       != IDE_SUCCESS, ERR_SET_SVP);
    }

    IDE_TEST(aApply->deleteXLog(sSmiTrans, aXLog) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SET_SVP);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN));
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    aApply->addAbortTx(aXLog->mTID, aXLog->mSN);

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applySPSet
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   Explicit Savepoint Set Log이다. 지정된 이름으로 Savepoint를 Set한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applySPSet(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    SChar           *sSavepointName    = NULL;
    rpSavepointType  sType             = RP_SAVEPOINT_EXPLICIT;
    UInt             sImplicitSvpDepth = SMI_STATEMENT_DEPTH_NULL;
    rpdTransTblNode * sTblNode = NULL;

    IDE_DASSERT( aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE );

    sSavepointName = aApply->getSvpNameAndEtc( aXLog->mSPName, 
                                               &sType, 
                                               &sImplicitSvpDepth,
                                               aApply->mImplSvpName );

    sTblNode = aApply->mTransTbl->getTrNode( aXLog->mTID );

    IDE_TEST_RAISE( sTblNode->mTrans.setSavepoint( aXLog->mTID, sType, sSavepointName )
                    != IDE_SUCCESS, ERR_SET_SVP );

    /* BUG-18028 Eager Mode에서 Partial Rollback 지원 */
    IDE_TEST_RAISE(aApply->mTransTbl->addLastSvpEntry(aXLog->mTID,
                                                      aXLog->mSN,
                                                      sType,
                                                      sSavepointName,
                                                      sImplicitSvpDepth)
                   != IDE_SUCCESS, ERR_SET_SVP);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SET_SVP);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN));
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    aApply->addAbortTx(aXLog->mTID, aXLog->mSN);

    return IDE_FAILURE;
}

/*
 * aXLogSPName(IN) : xlog's savepoint name
 * aType(Out) : savepoint type, psmsvp,implicit,explicit
 * aImplicitSvpDepth(Out) : if aType is implicit then it means depth of implicit svp.
 * return : translated svp name 
 */
SChar* rpxReceiverApply::getSvpNameAndEtc( SChar           *  aXLogSPName, 
                                           rpSavepointType *  aType,
                                           UInt            *  aImplicitSvpDepth,
                                           SChar           *  aImplSvpName )
{
    SChar * sSmiPsmSvpName = NULL;
    SChar * sTranslatedSPName = NULL;

    *aImplicitSvpDepth = SMI_STATEMENT_DEPTH_NULL;

    sSmiPsmSvpName = smiGetPsmSvpName();
    if( idlOS::strcmp( aXLogSPName, sSmiPsmSvpName ) == 0 )
    {
        *aType = RP_SAVEPOINT_PSM;
        sTranslatedSPName = sSmiPsmSvpName;
    }
    else
    {
        if(idlOS::strncmp( aXLogSPName,
                           (SChar *)SMR_IMPLICIT_SVP_NAME,
                           SMR_IMPLICIT_SVP_NAME_SIZE ) == 0 )
        {
            *aType = RP_SAVEPOINT_IMPLICIT;
            *aImplicitSvpDepth = (UInt)idlOS::atoi(
                &aXLogSPName[SMR_IMPLICIT_SVP_NAME_SIZE] );

            if ( mRole == RP_ROLE_PROPAGABLE_LOGGING )
            {
                idlOS::memset( aImplSvpName, 
                               0, 
                               RP_SAVEPOINT_NAME_LEN + 1 );
                idlOS::snprintf( aImplSvpName, RP_SAVEPOINT_NAME_LEN, 
                  "%s%s", aXLogSPName, RP_IMPLICIT_SVP_FOR_PROPAGATION_POSTFIX );
                sTranslatedSPName = aImplSvpName;
            }
            else
            {
                sTranslatedSPName = aXLogSPName;
            }
        }
        else
        {
            *aType = RP_SAVEPOINT_EXPLICIT;
            sTranslatedSPName = aXLogSPName;
        }
    }

    return sTranslatedSPName;
}
/*
 *
 */
SChar* rpxReceiverApply::getSvpName( UInt aDepth, SChar * aImplSvpName )
{
    SChar * sTranslatedSPName = NULL;

    sTranslatedSPName = rpcManager::getImplSPName( aDepth );

    if ( mRole == RP_ROLE_PROPAGABLE_LOGGING )
    {
        idlOS::memset( aImplSvpName, 
                       0, 
                       RP_SAVEPOINT_NAME_LEN + 1 );
        idlOS::snprintf( aImplSvpName, RP_SAVEPOINT_NAME_LEN, 
          "%s%s", sTranslatedSPName, RP_IMPLICIT_SVP_FOR_PROPAGATION_POSTFIX );
        sTranslatedSPName = aImplSvpName;
    }
    else
    {
        /*do nothing*/
    }

    return sTranslatedSPName;
}

//===================================================================
//
// Name:          applySPAbort
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   Savepoint Abort Log이다. Savepoint까지 Abort한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applySPAbort(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smSN      sSN            = SM_SN_NULL;
    SChar           *sSavepointName    = NULL;
    rpSavepointType  sType             = RP_SAVEPOINT_EXPLICIT;
    UInt             sImplicitSvpDepth = SMI_STATEMENT_DEPTH_NULL;
    rpdTransTblNode * sTblNode = NULL;

    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_R_TX_ABORT);

    RP_OPTIMIZE_TIME_BEGIN(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_ABORT);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE)
    {
        sSavepointName = aApply->getSvpNameAndEtc( aXLog->mSPName,
                                           &sType,
                                           &sImplicitSvpDepth,
                                           aApply->mImplSvpName );

        sTblNode = aApply->mTransTbl->getTrNode( aXLog->mTID );

        IDE_TEST( sTblNode->mTrans.abortSavepoint( sType, sSavepointName )
                  != IDE_SUCCESS );

        if ( IDE_TRC_RP_CONFLICT_3 )
        {
            if ( aApply->mTransTbl->getIsConflictFlag( aXLog->mTID ) == ID_TRUE )
            {
                ideLogEntry sLog( IDE_RP_CONFLICT_3 );
                (void)aApply->abortToSavepointConflictLog( sLog, aXLog, sSavepointName );
                sLog.write();
            }
            else
            {
                /* Nothing to do */
            }

            /* 트랜잭션 전체 취소가 아니므로 conflictTIDList에서 노드를 지우지 않는다. */
        }
        else
        {
            /* Nothing to do */
        }

        /* BUG-18028 Eager Mode에서 Partial Rollback 지원 */
        aApply->mTransTbl->applySvpAbort(aXLog->mTID, sSavepointName, &sSN);
        if(sSN <= aApply->mTransTbl->getAbortSN(aXLog->mTID))
        {
            aApply->addClearTx(aXLog->mTID, aXLog->mSN);
        }
    }

    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_ABORT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    RP_OPTIMIZE_TIME_END(aApply->mStatistics, IDV_OPTM_INDEX_RP_R_TX_ABORT);

    IDE_ERRLOG(IDE_RP_0);

    aApply->addAbortTx(aXLog->mTID, aXLog->mSN);

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyLobCursorOpen
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   LOB Cursor를 open한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyLobCursorOpen(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans*    sSmiTrans = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    IDE_DASSERT( aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE );

    /* PROJ-1442 Replication Online 중 DDL 허용
     * Lob Column이 Replication 대상일 때만 LOB Cursor를 Open한다.
     */
    if(aXLog->mLobPtr->mLobColumnID != RP_INVALID_COLUMN_ID)
    {
        //BUG-21858 : insert, update 실패가 되어 플레그가 세팅 되면 LOB 로그를 무시 한다.
        if(aApply->mTransTbl->getSkipLobLogFlag(aXLog->mTID) != ID_TRUE)
        {
            IDE_TEST( aApply->openLOBCursor( sSmiTrans, aXLog, aApply->mTransTbl )
                      != IDE_SUCCESS );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyLobCursorClose
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   LOB Cursor를 close한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyLobCursorClose(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans*    sSmiTrans = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE)
    {
        IDE_TEST( aApply->closeLOBCursor( sSmiTrans, aXLog, aApply->mTransTbl )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyLobPrepareWrite
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   LOB column value를 write하기 위한 prepare를 수행한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyLobPrepareWrite(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans*    sSmiTrans = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE)
    {
        IDE_TEST( aApply->prepareLOBWrite( sSmiTrans, aXLog, aApply->mTransTbl )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyLobPartialWrite
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   LOB Piece를 write한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyLobPartialWrite(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans*    sSmiTrans = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE)
    {
        IDE_TEST( aApply->writeLOBPiece( sSmiTrans, aXLog, aApply->mTransTbl )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyLobFinishWrite
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   LOB Partial write를 마친 이후에, Write 작업을 종료한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyLobFinishWrite(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans*    sSmiTrans = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE)
    {
        IDE_TEST( aApply->finishLOBWrite( sSmiTrans, aXLog, aApply->mTransTbl )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyLobTrim
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   LOB column value를 trim한다.
//
//===================================================================
IDE_RC rpxReceiverApply::applyLobTrim(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    smiTrans*    sSmiTrans = NULL;

    /* aXLog->mTID에 해당하는 SMI Transaction 객체를 Replication
     * Transaction Table에서 구한다.
     */
    sSmiTrans = aApply->mTransTbl->getSMITrans(aXLog->mTID);

    if(aApply->mTransTbl->isATrans(aXLog->mTID) == ID_TRUE)
    {
        IDE_TEST( aApply->trimLOB( sSmiTrans, aXLog, aApply->mTransTbl )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Incremental Sync를 위한 Primary Key가 이후에 수신될 것이다.
 *               (Failback Slave -> Failback Master)
 *
 ******************************************************************************/
IDE_RC rpxReceiverApply::applySyncPKBegin(rpxReceiverApply *aApply, rpdXLog */*aXLog*/)
{
    idBool         sIsFull = ID_FALSE;
    UInt           sSecond = 0;
    PDL_Time_Value sTimeValue;

    sTimeValue.initialize(1, 0);

    IDE_WARNING(IDE_RP_0, RP_TRC_RA_NTC_SYNC_PK_BEGIN);

    // Sender를 검색하여 SenderInfo를 얻는다.
    // Failback Master가 없으면, 다시 Failback 절차를 수행해야 한다.
    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPKBegin::Erratic::rpERR_ABORT_FAILBACK_SENDER_NOT_EXIST",
                        ERR_FAILBACK_SENDER_NOT_EXIST ); 
    IDE_TEST_RAISE( rpcManager::isAliveSender( aApply->mRepName ) != ID_TRUE,
                    ERR_FAILBACK_SENDER_NOT_EXIST );

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPKBegin::Erratic::rpERR_ABORT_SENDER_INFO_NOT_EXIST",
                         ERR_SENDER_INFO_NOT_EXIST ); 
    aApply->mSenderInfo = rpcManager::getSenderInfo(aApply->mRepName);
    IDE_TEST_RAISE( aApply->mSenderInfo == NULL, ERR_SENDER_INFO_NOT_EXIST );

    IDE_TEST( aApply->mSenderInfo->initSyncPKPool( aApply->mRepName ) != IDE_SUCCESS );

    // Incremental Sync Primary Key Begin를 Queue에 추가한다.
    while(sSecond < RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT)
    {
        IDE_TEST(aApply->mSenderInfo->addLastSyncPK(RP_SYNC_PK_BEGIN,
                                                    0,
                                                    0,
                                                    NULL,
                                                    &sIsFull)
                != IDE_SUCCESS);
        if(sIsFull != ID_TRUE)
        {
            break;
        }

        idlOS::sleep(sTimeValue);
        sSecond++;
    }

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPKBegin::Erratic::rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED",
                         ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED );
    IDE_TEST_RAISE(sSecond >= RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT,
                   ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_FAILBACK_SENDER_NOT_EXIST);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_SENDER_NOT_EXIST));
    }
    IDE_EXCEPTION( ERR_SENDER_INFO_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SENDER_INFO_NOT_EXIST,
                                  aApply->mRepName ) );
    }
    IDE_EXCEPTION(ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED));
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Incremental Sync Primary Key를 처리한다.
 *               (Failback Slave -> Failback Master)
 *
 ******************************************************************************/
IDE_RC rpxReceiverApply::applySyncPK(rpxReceiverApply *aApply, rpdXLog *aXLog)
{
    idBool           sIsFull = ID_FALSE;
    UInt             sSecond = 0;
    PDL_Time_Value   sTimeValue;
    rpdMetaItem    * sMetaItem = NULL;

    SChar            sValueData[QD_MAX_SQL_LENGTH + 1] = { 0, };
    UInt             sPKCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };
    UInt             i = 0;
    idBool           sIsValid = ID_FALSE;

    ideLogEntry      sLog(IDE_RP_0);

    IDE_ASSERT(aApply->mSenderInfo != NULL);

    sTimeValue.initialize(1, 0);

    // check existence of TABLE
    IDE_TEST( aApply->mReceiver->searchRemoteTable( &sMetaItem,
                                                    aXLog->mTableOID )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPK::Erratic::rpERR_ABORT_NOT_EXIST_TABLE",
                         ERR_NOT_EXIST_TABLE ); 
    IDE_TEST_RAISE(sMetaItem == NULL, ERR_NOT_EXIST_TABLE);

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPK::Erratic::rpERR_ABORT_COLUMN_COUNT_MISMATCH",
                         ERR_COL_COUNT_MISMATCH );
    IDE_TEST_RAISE(aXLog->mPKColCnt != sMetaItem->mPKIndex.keyColCount,
                   ERR_COL_COUNT_MISMATCH);

    // PK 정보를 로그에 기록한다.
    sLog.append(RP_TRC_RA_NTC_SYNC_PK);
    sLog.appendFormat(" User Name=%s, Table Name=%s, PK=[",
                      sMetaItem->mItem.mLocalUsername,
                      sMetaItem->mItem.mLocalTablename);

    for ( i = 0; i < sMetaItem->mPKIndex.keyColCount; i++ )
    {
        sPKCIDArray[i] = sMetaItem->mPKIndex.keyColumns[i].column.id;
    }

    rpdConvertSQL::getColumnListClause( sMetaItem,
                                        sMetaItem,
                                        sMetaItem->mPKIndex.keyColCount,
                                        sPKCIDArray,
                                        sMetaItem->mMapColID,
                                        aXLog->mPKCols,
                                        ID_TRUE,
                                        ID_TRUE,
                                        ID_TRUE,
                                        (SChar*)",",
                                        sValueData,
                                        QD_MAX_SQL_LENGTH + 1,
                                        &sIsValid );
    (void)sLog.appendFormat( "%s", sValueData );

    (void)sLog.append("]\n");
    (void)sLog.write();

    // Incremental Sync Primary Key를 Queue에 추가한다.
    while(sSecond < RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT)
    {
        IDE_TEST(aApply->mSenderInfo->addLastSyncPK(RP_SYNC_PK,
                                                    sMetaItem->mItem.mTableOID,
                                                    aXLog->mPKColCnt,
                                                    aXLog->mPKCols,
                                                    &sIsFull)
                 != IDE_SUCCESS);
        if(sIsFull != ID_TRUE)
        {
            break;
        }

        idlOS::sleep(sTimeValue);
        sSecond++;
    }

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPK::Erratic::rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED",
                         ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED ); 
    IDE_TEST_RAISE(sSecond >= RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT,
                   ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        ideLog::log(IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                              aXLog->mType,
                              aXLog->mTID,
                              aXLog->mSN,
                              aXLog->mTableOID);

        IDE_SET(ideSetErrorCode(rpERR_ABORT_NOT_EXIST_TABLE));
    }
    IDE_EXCEPTION(ERR_COL_COUNT_MISMATCH);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_COLUMN_COUNT_MISMATCH,
                                "[Sync PK] PK",
                                sMetaItem->mItem.mLocalTablename,
                                aXLog->mPKColCnt,
                                sMetaItem->mPKIndex.keyColCount));
    }
    IDE_EXCEPTION(ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED));
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Incremental Sync를 위한 Primary Key가 더 이상 수신되지 않는다.
 *               (Failback Slave -> Failback Master)
 *
 ******************************************************************************/
IDE_RC rpxReceiverApply::applySyncPKEnd(rpxReceiverApply *aApply, rpdXLog */*aXLog*/)
{
    idBool         sIsFull = ID_FALSE;
    UInt           sSecond = 0;
    PDL_Time_Value sTimeValue;

    IDE_ASSERT(aApply->mSenderInfo != NULL);

    sTimeValue.initialize(1, 0);

    IDE_WARNING(IDE_RP_0, RP_TRC_RA_NTC_SYNC_PK_END);

    // Incremental Sync Primary Key End를 Queue에 추가한다.
    while(sSecond < RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT)
    {
        IDE_TEST(aApply->mSenderInfo->addLastSyncPK(RP_SYNC_PK_END,
                                                    0,
                                                    0,
                                                    NULL,
                                                    &sIsFull)
                 != IDE_SUCCESS);
        if(sIsFull != ID_TRUE)
        {
            break;
        }

        idlOS::sleep(sTimeValue);
        sSecond++;
    }
    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPKEnd::Erratic::rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED",
                         ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED ); 
    IDE_TEST_RAISE(sSecond >= RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT,
                   ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);

    // SenderInfo가 더 이상 필요하지 않다.
    aApply->mSenderInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED));
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Failback이 완료되었다.
 *               (Failback Master -> Failback Slave)
 *
 ******************************************************************************/
IDE_RC rpxReceiverApply::applyFailbackEnd(rpxReceiverApply *aApply, rpdXLog */*aXLog*/)
{
    rpdSenderInfo * sSndrInfo = NULL;

    IDE_WARNING(IDE_RP_0, RP_TRC_RA_NTC_FAILBACK_END);

    // Sender를 검색하여 SenderInfo를 얻는다.
    // Failback Slave가 없으면, 다시 Failback 절차를 수행해야 한다.
    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyFailbackEnd::Erratic::rpERR_ABORT_FAILBACK_SENDER_NOT_EXIST",
                         ERR_FAILBACK_SENDER_NOT_EXIST );
    IDE_TEST_RAISE( rpcManager::isAliveSender( aApply->mRepName ) != ID_TRUE,
                    ERR_FAILBACK_SENDER_NOT_EXIST );

    IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyFailbackEnd::Erratic::rpERR_ABORT_SENDER_INFO_NOT_EXIST",
                         ERR_SENDER_INFO_NOT_EXIST );
    sSndrInfo = rpcManager::getSenderInfo(aApply->mRepName);
    IDE_TEST_RAISE( sSndrInfo == NULL, ERR_SENDER_INFO_NOT_EXIST );

    // Failback Slave가 있으면, SenderInfo에 Failback 완료를 설정한다.
    sSndrInfo->setPeerFailbackEnd(ID_TRUE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_FAILBACK_SENDER_NOT_EXIST);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_SENDER_NOT_EXIST));
    }
    IDE_EXCEPTION( ERR_SENDER_INFO_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SENDER_INFO_NOT_EXIST,
                                  aApply->mRepName ) );
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

//===================================================================
//
// Name:          applyAckOnDML
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//    AckOnDML 프로토콜을 받았을 경우 
//
//===================================================================
IDE_RC rpxReceiverApply::applyAckOnDML(rpxReceiverApply * aApply, rpdXLog * /*aXLog*/ )
{
    aApply->mReceiver->mAckEachDML = ID_TRUE;

    return IDE_SUCCESS;
}
//===================================================================
//
// Name:          applyIgnore
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   무시해도 되는 XLog Type이 도착한 경우
//
//===================================================================
IDE_RC rpxReceiverApply::applyIgnore(rpxReceiverApply */*aApply*/, rpdXLog */*aXLog*/ )
{
    // do nothing, for keeping alive
    // not error state, normal state!
    
    return IDE_SUCCESS;
}

//===================================================================
//
// Name:          applyNA
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//   처리할 수 없는 Type의 XLog가 도착한 경우 에러를 설정하고
//   후속 대책을 처리한다. (임시로 죽게 만들었음 - 수정해야 함)
//
//===================================================================
IDE_RC rpxReceiverApply::applyNA(rpxReceiverApply */* aApply */, rpdXLog *aXLog)
{
    IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_INVALID_XLOG_TYPE, aXLog->mType));
    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

/*
 *  Eager Repliaction 에서 receiver 에서 작업이 실패 할 경우에
 *  해당 transaction 을 commit 을 못하도록 한다.
 *  그 관련 정보들을 receiver 에서 저장 하고 있다가
 *  ack 로 해당 정보들을 전달하여 준다.
 *
 *  - sync mode 동작시에는 이 함수가 호출되어서는 안된다.
 *    해당 transaction 이 현재 살아 있는 Transaction이 아니므로
 *    ( 이미 commit 된 데이터를 적용 )
 *    Transaction Commit 을 못하도록 하는것이 의미가 없다.
 */
void rpxReceiverApply::addAbortTx(smTID aTID, smSN aSN)
{
    UInt sTxListIdx;

    if ( ( mTransTbl->getAbortFlag(aTID) != ID_TRUE ) &&
         ( aSN != SM_SN_NULL ) &&
         ( mReceiver->isEagerReceiver() == ID_TRUE ) )
    {
        /* Clear Tx List에서 해당 TID를 제거한다. */
        sTxListIdx = mTransTbl->getTxListIdx(aTID);
        if(sTxListIdx < mClearTxCount)
        {
            /* ACK를 전송한 후에는 sTxListIdx의 값이 유효하지 않으므로,
             * Transaction ID를 반드시 확인해야 한다.
             */
            if(aTID == mClearTxList[sTxListIdx].mTID)
            {
                mClearTxCount--;
                mClearTxList[sTxListIdx].mTID = mClearTxList[mClearTxCount].mTID;
                mClearTxList[sTxListIdx].mSN  = mClearTxList[mClearTxCount].mSN;
            }
        }

        /* Abort Tx로 설정하고, Abort Tx List에 추가한다. */
        mTransTbl->setAbortInfo(aTID, ID_TRUE, aSN);
        mTransTbl->setTxListIdx(aTID, mAbortTxCount);
        mAbortTxList[mAbortTxCount].mTID = aTID;
        mAbortTxList[mAbortTxCount].mSN  = aSN;
        mAbortTxCount++;
        IDE_DASSERT( mAbortTxCount <= smiGetTransTblSize() );
    }
    else
    {
        /* do nothing */
    }

    return;
}

void rpxReceiverApply::addClearTx(smTID aTID, smSN aSN)
{
    UInt sTxListIdx;

    if ( ( mTransTbl->getAbortFlag(aTID) == ID_TRUE ) &&
         ( mReceiver->isEagerReceiver() == ID_TRUE ) )
    {
        /* Abort Tx List에서 해당 TID를 제거한다. */
        sTxListIdx = mTransTbl->getTxListIdx(aTID);
        if(sTxListIdx < mAbortTxCount)
        {
            /* ACK를 전송한 후에는 sTxListIdx의 값이 유효하지 않으므로,
             * Transaction ID를 반드시 확인해야 한다.
             */
            if(aTID == mAbortTxList[sTxListIdx].mTID)
            {
                mAbortTxCount--;
                mAbortTxList[sTxListIdx].mTID = mAbortTxList[mAbortTxCount].mTID;
                mAbortTxList[sTxListIdx].mSN  = mAbortTxList[mAbortTxCount].mSN;
            }
        }

        /* Clear Tx로 설정하고, Clear Tx List에 추가한다. */
        mTransTbl->setAbortInfo(aTID, ID_FALSE, SM_SN_NULL);
        mTransTbl->setTxListIdx(aTID, mClearTxCount);
        mClearTxList[mClearTxCount].mTID = aTID;
        mClearTxList[mClearTxCount].mSN  = aSN;
        mClearTxCount++;
    }
    else
    {
        /* do nothing */
    }

    return;
}

/*
 *
 */
IDE_RC rpxReceiverApply::getLocalFlushedRemoteSN( smSN aRemoteFlushSN,
                                                  smSN aRestartSN,
                                                  smSN * aLocalFlushedRemoteSN )
{
    smSN         sLocalFlushSN          = SM_SN_NULL;
    smSN         sLocalFlushedRemoteSN  = SM_SN_NULL;

    if ( mSNMapMgr != NULL )
    {
        IDE_TEST( smiGetSyncedMinFirstLogSN( &sLocalFlushSN )
                  != IDE_SUCCESS );

        if ( sLocalFlushSN != SM_SN_NULL )
        {
            mSNMapMgr->getLocalFlushedRemoteSN( sLocalFlushSN,
                                                aRemoteFlushSN,
                                                aRestartSN,
                                                &sLocalFlushedRemoteSN );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    *aLocalFlushedRemoteSN = sLocalFlushedRemoteSN;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReceiverApply::buildXLogAck( rpdXLog * aXLog, rpXLogAck * aAck )
{
    smSN sLocalFlushedRemoteSN = SM_SN_NULL;
    smSN sRestartSN = SM_SN_NULL;

    switch ( aXLog->mType )
    {
        case RP_X_HANDSHAKE:   // PROJ-1442 Replication Online 중 DDL 허용
            IDE_WARNING( IDE_RP_0, RP_TRC_RA_NTC_HANDSHAKE_XLOG );
            aAck->mAckType = RP_X_HANDSHAKE_ACK;
            break;
            
        case RP_X_REPL_STOP :
            IDE_WARNING( IDE_RP_0, RP_TRC_RA_NTC_REPL_STOP_XLOG );
            aAck->mAckType = RP_X_STOP_ACK;
            break;
    
        case RP_X_DDL_REPLICATE_HANDSHAKE :
            aAck->mAckType = RP_X_DDL_REPLICATE_HANDSHAKE_ACK;
            break;

        case RP_X_SYNC_END:
            aAck->mAckType = RP_X_SYNC_END_ACK;
            aAck->mAbortTxCount      = 0;
            aAck->mClearTxCount      = 0;
            aAck->mRestartSN         = SM_SN_NULL;
            aAck->mLastCommitSN      = SM_SN_NULL;
            aAck->mLastArrivedSN     = SM_SN_NULL;
            aAck->mLastProcessedSN   = SM_SN_NULL;
            aAck->mAbortTxList       = NULL;
            aAck->mClearTxList       = NULL;
            aAck->mFlushSN           = SM_SN_NULL;
            IDE_CONT( NORMAL_EXIT );

        case RP_X_TRUNCATE:
            aAck->mAckType = RP_X_TRUNCATE_ACK;
            aAck->mAbortTxCount      = 0;
            aAck->mClearTxCount      = 0;
            aAck->mRestartSN         = SM_SN_NULL;
            aAck->mLastCommitSN      = SM_SN_NULL;
            aAck->mLastArrivedSN     = SM_SN_NULL;
            aAck->mLastProcessedSN   = SM_SN_NULL;
            aAck->mAbortTxList       = NULL;
            aAck->mClearTxList       = NULL;
            aAck->mFlushSN           = SM_SN_NULL;
            IDE_CONT( NORMAL_EXIT );

        case RP_X_XA_START_REQ:
            aAck->mAckType = RP_X_XA_START_REQ_ACK;
            break;

        case RP_X_XA_PREPARE_REQ:
        case RP_X_XA_PREPARE:
            aAck->mAckType = RP_X_ACK_WITH_TID;
            break;

        default :
            if ( mReceiver->isEagerReceiver() == ID_TRUE )
            {
                aAck->mAckType = RP_X_ACK_WITH_TID;
            }
            else
            {
                aAck->mAckType = RP_X_ACK;
            }
            break;
    }

    // BUG-17748
    if ( ( aXLog->mType == RP_X_KEEP_ALIVE ) ||
         ( aXLog->mType == RP_X_REPL_STOP ) )
    {
        sRestartSN = aXLog->mRestartSN;

        IDE_TEST( getLocalFlushedRemoteSN( aXLog->mSyncSN,
                                           sRestartSN,
                                           &sLocalFlushedRemoteSN )
                  != IDE_SUCCESS );
    }
    else
    {
        sRestartSN = SM_SN_NULL;
    }

    aAck->mAbortTxCount      = mAbortTxCount;
    aAck->mClearTxCount      = mClearTxCount;
    aAck->mRestartSN         = sRestartSN;
    aAck->mLastCommitSN      = mLastCommitSN;
    aAck->mAbortTxList       = mAbortTxList;
    aAck->mClearTxList       = mClearTxList;
    aAck->mFlushSN           = sLocalFlushedRemoteSN;

    // Incremental Sync 시에 aXLog->mSN이 0이 될 수 있는데, 로그의 SN이 아니므로 무의미하다.
    if ( aXLog->mSN == 0 )
    {
        aAck->mLastArrivedSN   = SM_SN_NULL;
        aAck->mLastProcessedSN = SM_SN_NULL;
    }
    else
    {
        aAck->mLastArrivedSN     = aXLog->mSN;
        aAck->mLastProcessedSN   = aXLog->mSN;
    }

    aAck->mTID = aXLog->mTID;

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
idBool rpxReceiverApply::isTimeToSendAck( void )
{
    idBool sResult = ID_FALSE;

    if ( ( mAbortTxCount == smiGetTransTblSize() ) ||
         ( mClearTxCount == smiGetTransTblSize() ) )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

/*
 *
 */
void rpxReceiverApply::resetCounterForNextAck( void )
{
    mAbortTxCount       = 0;
    mClearTxCount       = 0;
}

/*
 *
 */
void rpxReceiverApply::checkAndResetCounter( void )
{
    if ( ( mAbortTxCount == smiGetTransTblSize() ) ||
         ( mClearTxCount == smiGetTransTblSize() ) )
    {
        mAbortTxCount       = 0;
        mClearTxCount       = 0;
    }
    else
    {
        /* nothing to do */
    }
}

/*BUG-16807 때문에 스레드 하나로 수행하게 하기위해 apply를 만들었음*/
/* PROJ-1915 aXLog->mSkip == ID_TRUE off-line 센더가 이미 반영한 로그를 스킵 한다. */
IDE_RC rpxReceiverApply::apply( rpdXLog * aXLog )
{
    IDE_DASSERT(aXLog->mType != RP_X_ACK);
    IDE_DASSERT(aXLog->mType != RP_X_STOP_ACK);
    IDE_DASSERT(aXLog->mType != RP_X_HANDSHAKE_ACK);

    // TASK-2359
    if ( RPU_REPLICATION_PERFORMANCE_TEST == 0 )
    {
        IDE_TEST( execXLog( aXLog ) != IDE_SUCCESS );
    }
    else
    {
        /*
         * 성능측정 스크립트 AT-P26에서 receiver의 반영없이
         * 로그분석 시간과 네트워크 전송 시간만을 측정할 때,
         * receiver에 반영이 된 것으로 취급하기 위해 증가시킨다.
         */
        if ( ( aXLog->mType == RP_X_INSERT ) || ( aXLog->mType == RP_X_SYNC_INSERT ) )
        {
            mInsertSuccessCount++;
        }
        else if ( aXLog->mType == RP_X_DELETE )
        {
            mDeleteSuccessCount++;
        }
        else if ( aXLog->mType == RP_X_UPDATE )
        {
            mUpdateSuccessCount++;
        }
    }

    switch ( aXLog->mType )
    {
        case RP_X_COMMIT:
        case RP_X_XA_COMMIT:
        case RP_X_ABORT:
            mLastCommitSN = aXLog->mSN;
            break;

        case RP_X_KEEP_ALIVE:
            mLastCommitSN = aXLog->mSN;
            mRestartSN = aXLog->mRestartSN;
            break;

        case RP_X_REPL_STOP:
            mRestartSN = aXLog->mRestartSN;
            break;

        default:
            break;
    }

    // for Performance View
    if ( ( aXLog->mSN != SM_SN_NULL ) && ( aXLog->mSN != 0 ) )
    {
        mApplyXSN = aXLog->mSN;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReceiverApply::setFlagToSendAckForEachTransactionCommit( void )
{
    mAckForTransactionCommit = ID_TRUE;
}

/*
 *
 */
void rpxReceiverApply::setFlagNotToSendAckForEachTransactionCommit( void )
{
    mAckForTransactionCommit = ID_FALSE;
}

/*
 *
 */
void rpxReceiverApply::setSNMapMgr( rprSNMapMgr * aSNMapMgr )
{
    mSNMapMgr = aSNMapMgr;
}

/*
 * @brief check policy means receiver applier checks previous state
 */
void rpxReceiverApply::setApplyPolicyCheck( void )
{
    mPolicy = RPX_APPLY_POLICY_CHECK;
}

/*
 * @brief force policy means receiver applier ignore previous state
 */
void rpxReceiverApply::setApplyPolicyForce( void )
{
    mPolicy = RPX_APPLY_POLICY_FORCE;
}

/*
 *
 */
void rpxReceiverApply::setApplyPolicyByProperty( void )
{
    mPolicy = RPX_APPLY_POLICY_BY_PROPERTY;
}

/*
 *
 */
smSN rpxReceiverApply::getApplyXSN( void )
{
    return mApplyXSN;
}

ULong rpxReceiverApply::getInsertSuccessCount( void )
{
    return mInsertSuccessCount;
}

ULong rpxReceiverApply::getInsertFailureCount( void )
{
    return mInsertFailureCount;
}

ULong rpxReceiverApply::getUpdateSuccessCount( void )
{
    return mUpdateSuccessCount;
}

ULong rpxReceiverApply::getUpdateFailureCount( void )
{
    return mUpdateFailureCount;
}

ULong rpxReceiverApply::getDeleteSuccessCount( void )
{
    return mDeleteSuccessCount;
}

ULong rpxReceiverApply::getDeleteFailureCount( void )
{
    return mDeleteFailureCount;
}

ULong rpxReceiverApply::getCommitCount( void )
{
    return mCommitCount;
}

ULong rpxReceiverApply::getAbortCount( void )
{
    return mAbortCount;
}

IDE_RC rpxReceiverApply::applySyncStart( rpxReceiverApply * aApply,
                                         rpdXLog * /*aXLog*/ )
{
    IDE_TEST( aApply->mReceiver->recvSyncTablesInfo( &(aApply->mSyncTableNumber),
                                                     &(aApply->mRemoteTable) )
              != IDE_SUCCESS);

    if ( aApply->mReceiver->mStartMode == RP_RECEIVER_SYNC_CONDITIONAL )
    {
        IDE_TEST ( aApply->applyUpdateConditionAct( aApply, ID_TRUE ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::applySyncEnd( rpxReceiverApply * aApply, 
                                       rpdXLog * /*aXLog*/ )
{
    IDE_TEST ( aApply->applyRebuildIndices( aApply ) != IDE_SUCCESS );

    if ( aApply->mReceiver->mStartMode == RP_RECEIVER_SYNC_CONDITIONAL )
    {
        IDE_TEST ( aApply->applyUpdateConditionAct( aApply, ID_FALSE ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpxReceiverApply::applyUpdateConditionAct( rpxReceiverApply    * aApply, 
                                                  idBool                aIsConditionSynced )
{
    smiTrans sTrans;
    smiStatement sStmt;
    smiStatement * sRootStmt;
    smSCN sDummySCN = SM_SCN_INIT;
    UInt sStage = 0;
    rpdMetaItem * sMetaItem = NULL;
    SInt i;
    UInt j;
    rpdMeta       * sMeta = &(aApply->mReceiver->mMeta);

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sRootStmt,
                                  aApply->mStatistics,
                                  ( SMI_ISOLATION_NO_PHANTOM     |
                                    SMI_TRANSACTION_NORMAL       |
                                    SMI_TRANSACTION_REPL_NONE |
                                    SMI_COMMIT_WRITE_WAIT ) )
                    != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sStmt.begin( sTrans.getStatistics(),
                           sRootStmt,
                           SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) != IDE_SUCCESS );
    sStage = 3;

    IDU_FIT_POINT( "rpxReceiverApply::applyUpdateConditionAct::stage3" );
    
    for ( i = 0; i < sMeta->mReplication.mItemCount; i++ )
    {
        for ( j = 0; j < aApply->mSyncTableNumber; j++ )
        {
            IDE_TEST( aApply->mReceiver->searchRemoteTable( &sMetaItem, 
                                                            aApply->mRemoteTable[j].mItem.mTableOID )
                      != IDE_SUCCESS );
            IDE_TEST_RAISE( sMetaItem == NULL, ERR_NOT_EXIST_TABLE);

            if ( sMeta->mItemsOrderByTableOID[i]->mItem.mTableOID == sMetaItem->mItem.mTableOID )
            {
                sMetaItem->mItem.mIsConditionSynced = aIsConditionSynced;
                IDE_TEST( rpdCatalog::updateConditionalSyncedWithOID( &sStmt,
                                                                      aApply->mRepName,
                                                                      sMetaItem->mItem.mTableOID,
                                                                      aIsConditionSynced )
                          != IDE_SUCCESS);
                break;
            }
        }
    }

    IDE_TEST( sStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit( &sDummySCN ) != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );


    return IDE_SUCCESS;
    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_NOT_EXIST_TABLE));
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);
    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            (void)sStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
            /* fall through */
        case 1:
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;   
}

IDE_RC rpxReceiverApply::applyRebuildIndices( rpxReceiverApply * aApply )
{
    smiTrans sTrans;
    smiStatement sStmt;
    smiStatement * sRootStmt;
    UInt sStage = 0;
    rpdMetaItem * sLocalTable = NULL;
    const void  * sTable = NULL;
    rpdMetaItem * sMetaItem = NULL;
    UInt i;
    qciTableInfo ** sTableInfo = NULL;
    qciTableInfo  * sNewTableInfo = NULL;
    void          * sNewTableHandle = NULL;
    UInt sNewCachedMetaCount = 0;
    smSCN           sSCN      = SM_SCN_INIT;
    UInt            sUserID   = 0;
    void          * sTableHandle = NULL;

    /* BUG-40557 */
    IDE_TEST_RAISE ( aApply->mRemoteTable == NULL, ERR_NOT_EXIST_TABLE );

    IDE_TEST_RAISE ( aApply->mSyncTableNumber == 0, ERR_INVALID_SYNC_TABLE_NUMBER );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sRootStmt,
                            aApply->mStatistics,
                            ( SMI_ISOLATION_NO_PHANTOM     |
                              SMI_TRANSACTION_NORMAL       |
                              SMI_TRANSACTION_REPL_DEFAULT |
                              SMI_COMMIT_WRITE_NOWAIT ) )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sStmt.begin( sTrans.getStatistics(),
                           sRootStmt,
                           SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) != IDE_SUCCESS );
    sStage = 3;

    IDU_FIT_POINT( "rpxReceiverApply::applyRebuildIndices::calloc::TableInfo",
                    rpERR_ABORT_MEMORY_ALLOC, 
                    "rpxReceiverApply::applyRebuildIndices",
                    "TalbeInfo" );
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                 aApply->mSyncTableNumber,
                                 ID_SIZEOF(qciTableInfo*),
                                 (void**)&sTableInfo,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    for ( i = 0; i < aApply->mSyncTableNumber; i++ )
    {
        IDE_TEST( aApply->mReceiver->searchRemoteTable( &sMetaItem, aApply->mRemoteTable[i].mItem.mTableOID )
                  != IDE_SUCCESS );

        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applyRebuildIndices::Erratic::rpERR_ABORT_NOT_EXIST_TABLE",
                             ERR_NOT_EXIST_TABLE );
        IDE_TEST_RAISE( sMetaItem == NULL, ERR_NOT_EXIST_TABLE);

        IDE_TEST( aApply->mReceiver->mMeta.searchTable( &sLocalTable, sMetaItem->mItem.mTableOID )
                  != IDE_SUCCESS);
        
        IDE_TEST_RAISE( sLocalTable == NULL, ERR_NOT_FOUND_TABLE );

        sTable = smiGetTable( (smOID)(sLocalTable->mItem.mTableOID) );

        if ( smiIsConsistentIndices( sTable ) != ID_TRUE )
        {
            ideLog::log(IDE_RP_0, "Table OID: %"ID_INT64_FMT" Start rebuild index", sMetaItem->mItem.mTableOID );

            if ( idlOS::strlen( sMetaItem->mItem.mLocalPartname ) > 0 )
            {
                IDE_TEST( qciMisc::getUserID( NULL,
                                              &sStmt,
                                              sMetaItem->mItem.mLocalUsername,
                                              (UInt)idlOS::strlen( sMetaItem->mItem.mLocalUsername ),
                                              &sUserID )
                          != IDE_SUCCESS); 

                IDE_TEST( qciMisc::getTableHandleByName( &sStmt,
                                                         sUserID,
                                                         (UChar*)sMetaItem->mItem.mLocalTablename,
                                                         (SInt)idlOS::strlen( sMetaItem->mItem.mLocalTablename ),
                                                         (void**)&sTableHandle,
                                                         &sSCN )
                          != IDE_SUCCESS );    
            
                IDE_TEST( smiValidateAndLockObjects( sStmt.getTrans(),
                                                     sTableHandle,
                                                     sSCN,
                                                     SMI_TBSLV_DDL_DML,
                                                     SMI_TABLE_LOCK_IX,
                                                     smiGetDDLLockTimeOut(sStmt.getTrans()),
                                                     ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }            
            
            sSCN = smiGetRowSCN(sTable);
            IDE_TEST( smiValidateAndLockObjects( sStmt.getTrans(),
                                                 sTable,
                                                 sSCN,
                                                 SMI_TBSLV_DDL_DML,
                                                 SMI_TABLE_LOCK_X,
                                                 smiGetDDLLockTimeOut(sStmt.getTrans()),
                                                 ID_FALSE )
                      != IDE_SUCCESS );
            
            IDE_TEST_RAISE( smiTable::rebuildAllIndex( &sStmt,
                                                       sTable )
                            != IDE_SUCCESS, ERR_REBUILD_INDEX );
            
            sTableInfo[sNewCachedMetaCount] = (qciTableInfo *)rpdCatalog::rpdGetTableTempInfo(
                smiGetTable( (smOID)sLocalTable->mItem.mTableOID) );

            IDU_FIT_POINT( "rpxReceiverApply::applyRebuildIndices::makeAndSetQcmTableInfo" );

            if ( (sTableInfo[sNewCachedMetaCount])->tablePartitionType
                 == QCM_NONE_PARTITIONED_TABLE )
            {
                IDE_TEST( qciMisc::makeAndSetQcmTableInfo( &sStmt,
                                                           sTableInfo[sNewCachedMetaCount]->tableID,
                                                           (smOID)sLocalTable->mItem.mTableOID )
                          != IDE_SUCCESS );

            }
            else
            {
                IDE_TEST( qciMisc::makeAndSetQcmPartitionInfo(
                        &sStmt,
                        sTableInfo[sNewCachedMetaCount]->partitionID,
                        (smOID)sLocalTable->mItem.mTableOID,
                        (qciTableInfo *)rpdCatalog::rpdGetTableTempInfo( sTableHandle ) )
                    != IDE_SUCCESS );

            }
            ideLog::log(IDE_RP_0, "Table OID: %"ID_INT64_FMT" End rebuild index", sMetaItem->mItem.mTableOID );

            sNewCachedMetaCount++;

        }
        else
        {
            /* nothing to do */
        }
    }
    
    IDE_TEST( sStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    for ( i = 0; i < sNewCachedMetaCount; i++ )
    {
        if ( (sTableInfo[i])->tablePartitionType
             == QCM_NONE_PARTITIONED_TABLE )
        {
            (void)qciMisc::destroyQcmTableInfo( sTableInfo[i] );
        }
        else
        {
            (void)qciMisc::destroyQcmPartitionInfo( sTableInfo[i] );
        }
    }
    sNewCachedMetaCount = 0;

    (void)iduMemMgr::free( sTableInfo );
    sTableInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REBUILD_INDEX );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_REBUILD_INDEX ) );
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_NOT_EXIST_TABLE));
    }
    IDE_EXCEPTION( ERR_INVALID_SYNC_TABLE_NUMBER );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_SYNC_TABLE_NUMBER ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_TABLE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_META_NO_SUCH_DATA ) );
    }    
    IDE_EXCEPTION_END;

    if( sNewCachedMetaCount > 0 )
    {
        /* error occurred, recover cached meta info */
        for ( i = 0; i < sNewCachedMetaCount; i++ )
        {
            sNewTableHandle = sTableInfo[i]->tableHandle;
            sNewTableInfo = (qciTableInfo *)(rpdCatalog::rpdGetTableTempInfo(sNewTableHandle));

            if ( (sNewTableInfo)->tablePartitionType
                 == QCM_NONE_PARTITIONED_TABLE )
            {
                (void)qciMisc::destroyQcmTableInfo( sNewTableInfo );
            }
            else
            {
                (void)qciMisc::destroyQcmPartitionInfo( sNewTableInfo );
            }
            smiSetTableTempInfo( sTableInfo[i]->tableHandle, (void*)sTableInfo[i] );
        }
    }

    if ( sTableInfo != NULL )
    {
        (void) iduMemMgr::free( sTableInfo );
    }
    switch ( sStage )
    {
        case 3:
            sStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            sTrans.rollback();
        case 1:
            sTrans.destroy( NULL );
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::applyTruncate( rpxReceiverApply * aApply, rpdXLog * aXLog )
{
    rpdMetaItem    * sMetaItem = NULL;
    
    IDE_TEST( aApply->mReceiver->searchRemoteTable( &sMetaItem, aXLog->mTableOID )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sMetaItem == NULL, ERR_NOT_EXIST_TABLE );

    IDE_TEST( executeTruncate( aApply->mReceiver, sMetaItem, ID_FALSE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE );
    {
        ideLog::log( IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                     aXLog->mType,
                     aXLog->mTID,
                     aXLog->mSN,
                     aXLog->mTableOID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_EXIST_TABLE ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::allocRangeColumn( UInt     aCount )
{
    idBool      sIsAlloc = ID_FALSE;

    IDU_FIT_POINT( "rpxReceiverApply::allocRangeColumn::calloc::mRangeColumn",
                   rpERR_ABORT_MEMORY_ALLOC,
                   "rpxReceiverApply::allocRangeColumn",
                   "mRangeColumn" );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       aCount,
                                       ID_SIZEOF(qriMetaRangeColumn),
                                       (void **)&mRangeColumn,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_RANGE_COLUMN );
    sIsAlloc = ID_TRUE;
    mRangeColumnCount = aCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_RANGE_COLUMN );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiverApply::allocRangeColumn",
                                  "mRangeColumn" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAlloc == ID_TRUE )
    {
        (void)iduMemMgr::free( mRangeColumn );
        mRangeColumn = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

smSN rpxReceiverApply::getRestartSN( void )
{
    return mRestartSN;
}

smSN rpxReceiverApply::getLastCommitSN( void )
{
    return mLastCommitSN;
}

IDE_RC rpxReceiverApply::buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                                             void                    * aDumpObj,
                                                             iduFixedTableMemory     * aMemory,
                                                             SChar                   * aRepName,
                                                             UInt                      aParallelID,
                                                             SInt                      aApplierIndex )
{

    IDE_TEST( mTransTbl->buildRecordForReplReceiverTransTbl( aHeader,
                                                             aDumpObj,
                                                             aMemory,
                                                             aRepName,
                                                             aParallelID,
                                                             aApplierIndex )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::runDML( smiStatement       * aRootSmiStmt,
                                 rpdMetaItem        * aRemoteMetaItem,
                                 rpdMetaItem        * aLocalMetaItem,
                                 rpdXLog            * aXLog,
                                 SChar              * aQueryString,
                                 idBool               aCompareBeforeImage,
                                 SLong              * aRowCount,
                                 rpApplyFailType    * aFailType )
{
    SLong               sRowCount = 0;

    *aFailType = RP_APPLY_FAIL_NONE;

    IDE_TEST( rpsSQLExecutor::executeSQL( aRootSmiStmt,
                                          aRemoteMetaItem,
                                          aLocalMetaItem,
                                          aXLog,
                                          aQueryString,
                                          idlOS::strlen( aQueryString ),
                                          aCompareBeforeImage,
                                          &sRowCount )
              != IDE_SUCCESS );

    *aRowCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( ideGetErrorCode() == smERR_ABORT_NOT_ENOUGH_SPACE ) &&
         ( mReceiver->isEagerReceiver() == ID_FALSE ) )
    {
        *aFailType = RP_APPLY_FAIL_INTERNAL;
    }
    else if ( RP_IS_CORRUPTED_PAGE() == ID_TRUE )
    {
        *aFailType = RP_APPLY_FAIL_BY_CORRUPTED_PAGE;
    }
    else
    {
        if ( ( ideGetErrorCode() == smERR_ABORT_smnUniqueViolationInReplTrans ) ||
             ( ideGetErrorCode() == smERR_ABORT_smcExceedLockTimeWait ) )
        {
            /* 
             * conflict resolution tx이 먼저 lock을 잡아 실패했다면,
             * 위에서 다음의 작업을 한다.
             * 1. conflict resolution tx이 있을 경우, 이 작업을 conflict resolution tx에 넣고 재시도
             * 2. conflict  resolution tx이 없으면, 다른 tx가 conflict resolution처리중인것이므로 실패처리
             */
            *aFailType = RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX;
        }
        else
        {
            *aFailType = RP_APPLY_FAIL_BY_CONFLICT;
        }
    }

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::getCheckRowExistenceAndResolutionNeed( smiTrans        * aSmiTrans,
                                                                rpdMetaItem     * aMetaItem,
                                                                smiRange        * aKeyRange,
                                                                rpdXLog         * aXLog,
                                                                idBool          * aCheckRowExistence,
                                                                idBool          * aIsResolutionNeed,
                                                                rpApplyFailType * aFailType )
{
    idBool      sIsConflict = ID_FALSE;
    idBool      sCheckRowExistence = ID_FALSE;
    idBool      sIsResolutionNeed = ID_FALSE;

    switch ( mPolicy )
    {
        case RPX_APPLY_POLICY_BY_PROPERTY:
            if ( RPU_REPLICATION_INSERT_REPLACE == 1 )
            {
                sCheckRowExistence = ID_FALSE;
            }
            else
            {
                sCheckRowExistence = ID_TRUE;
            }
            break;

        case RPX_APPLY_POLICY_CHECK:
            sCheckRowExistence = ID_TRUE;
            break;

        case RPX_APPLY_POLICY_FORCE:
            sCheckRowExistence = ID_FALSE;
            break;

        default:
            break;
    }

    if ( aMetaItem->mTsFlag != NULL )
    {
        if ( mSmExecutor.compareInsertImage( aSmiTrans,
                                              aXLog,
                                              aMetaItem,
                                              aKeyRange,
                                              &sIsConflict,
                                              aMetaItem->mTsFlag,
                                              aFailType )
             != IDE_SUCCESS )
        {
          
            IDE_TEST( *aFailType != RP_APPLY_FAIL_BY_CORRUPTED_PAGE ) ;
        }
        else
        {
            if ( sIsConflict != ID_TRUE )
            {
                sIsResolutionNeed = ID_TRUE;
            }
            else
            {
                IDE_SET( ideSetErrorCode( rpERR_IGNORE_TIMESTAMP_INSERT_CONFLICT ) );
            }
        }
    }
    else
    {
        if ( sCheckRowExistence == ID_FALSE )
        {
            sIsResolutionNeed = ID_TRUE;
        }
        else
        {
            /* do nothing */
        }
    }

    *aCheckRowExistence = sCheckRowExistence;
    *aIsResolutionNeed = sIsResolutionNeed;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::insertReplaceSQL( rpdMetaItem      * aLocalMetaItem,
                                           rpdMetaItem      * aRemoteMetaItem,
                                           rpdXLog          * aXLog,
                                           smiRange         * aKeyRange,
                                           idBool             aCheckRowExistence,
                                           SChar            * aSPName,
                                           SChar            * aQueryString,
                                           rpApplyFailType  * aFailType )
{
    smiTrans          * sTrans = NULL;
    UInt                sColCnt = 0;
    UInt                sReplLockTimeout = 0;
    idBool              sIsResolutionCancel = ID_FALSE;
    SLong               sRowCount = 0;

    IDE_TEST( getConfictResolutionTransaction( aXLog->mTID,
                                               &sTrans )
              != IDE_SUCCESS );

    sReplLockTimeout = sTrans->getReplTransLockTimeout();
    IDE_TEST( sTrans->setReplTransLockTimeout( 0 ) != IDE_SUCCESS );

    /* Delete + Insert를 적용하기 전에 Savepoint를 두 개 설정한다. */
    if ( aSPName != NULL )
    {
        IDE_TEST_RAISE( sTrans->savepoint( aSPName ) != IDE_SUCCESS, ERR_SET_SVP );
    }
    else
    {
        /* Nothing to do */
    }
    
    IDU_FIT_POINT( "rpxReceiverApply::insertReplaceSQL::savepoint::aSmiTrans" );
    IDE_TEST_RAISE( sTrans->savepoint( RP_CONFLICT_RESOLUTION_SVP_NAME )
                    != IDE_SUCCESS, ERR_SET_SVP );
    sIsResolutionCancel = ID_TRUE;

    /* delete */
    sColCnt = aXLog->mColCnt;
    aXLog->mPKColCnt = aLocalMetaItem->mPKColCount;
    aXLog->mColCnt   = 0;
          
    if ( mSmExecutor.executeDelete( sTrans,
                                    aXLog,
                                    aLocalMetaItem,
                                    aLocalMetaItem->mIndexTableRef,
                                    aKeyRange,
                                    aFailType,
                                    aCheckRowExistence )
         != IDE_SUCCESS )
    {
        /* mPKColCnt와 mColCnt를 복원한다. */
        aXLog->mPKColCnt = 0;
        aXLog->mColCnt   = sColCnt;

        IDE_TEST( *aFailType == RP_APPLY_FAIL_INTERNAL ); 
        IDE_CONT( ERR_PASS );
    }
    else
    {
        /* nothing to do */
    }

    aXLog->mPKColCnt = 0;
    aXLog->mColCnt   = sColCnt;

    /* insert */
    if ( runDML( sTrans->getStatement(),
                 aRemoteMetaItem,
                 aLocalMetaItem,
                 aXLog,
                 aQueryString,
                 ID_FALSE,
                 &sRowCount,
                 aFailType )
         != IDE_SUCCESS )
    {
        IDE_TEST( *aFailType == RP_APPLY_FAIL_INTERNAL ); 
    }
    else
    {
        sIsResolutionCancel = ID_FALSE;
    }

    RP_LABEL( ERR_PASS );

    if ( sIsResolutionCancel == ID_TRUE )
    {
        sIsResolutionCancel = ID_FALSE;
        IDU_FIT_POINT( "rpxReceiverApply::insertReplaceSQL::rollback::aSmiTrans" );
        IDE_TEST_RAISE( sTrans->rollback( RP_CONFLICT_RESOLUTION_SVP_NAME, 
                                          SMI_DO_NOT_RELEASE_TRANSACTION )
                        != IDE_SUCCESS, ERR_ABORT_SVP );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( sTrans->setReplTransLockTimeout( sReplLockTimeout ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_SVP );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN ) );
    }
    IDE_EXCEPTION( ERR_ABORT_SVP );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_ABORT_SAVEPOINT_ERROR_IN_RUN ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsResolutionCancel == ID_TRUE )
    {
        sIsResolutionCancel = ID_FALSE;
        (void)sTrans->rollback( RP_CONFLICT_RESOLUTION_SVP_NAME, 
                                SMI_DO_NOT_RELEASE_TRANSACTION );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpxReceiverApply::printInsertErrLog( rpdMetaItem   * aMetaItem,
                                          rpdMetaItem   * aMetaItemForPK,
                                          rpdXLog       * aXLog,
                                          rpApplyFailType aFailType )
{
    RP_CONFLICT_ERRLOG2();

    if ( IDE_TRC_RP_3 != 0 )
    {
        ideLogEntry sLog( IDE_RP_3 ); // insertErrLog에서 복잡하기 때문에 상위에서 open,close를 함. more safe
        (void)insertErrLog( sLog, aMetaItem, aXLog, aFailType );
        (void)sLog.write();
    }
    else
    {
        /* do nothing */
    }

    if ( IDE_TRC_RP_CONFLICT_3 != 0 )
    {
        ideLogEntry sLog( IDE_RP_CONFLICT_3 );
        (void)insertErrLog( sLog, aMetaItem, aXLog, aFailType );
        (void)sLog.write();

        mTransTbl->setIsConflictFlag( aXLog->mTID, ID_TRUE );
    }
    else
    {
        /* do nothing */
    }

    if ( IDE_TRC_RP_5 != 0 )
    {
        ideLogEntry sLog( IDE_RP_5 );
        printPK( sLog, 
                 "INSERT",
                 aMetaItemForPK, 
                 aXLog->mACols );
        (void)sLog.write();
    }
    else
    {
        /* do nothing */
    }

    if ( IDE_TRC_RP_CONFLICT_5 != 0 )
    {
        ideLogEntry sLog( IDE_RP_CONFLICT_5 );
        printPK( sLog, 
                 "INSERT",
                 aMetaItemForPK, 
                 aXLog->mACols );
        (void)sLog.write();
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiverApply::printUpdateErrLog( rpdMetaItem   * aMetaItem,
                                          rpdMetaItem   * aMetaItemForPK,
                                          rpdXLog       * aXLog,
                                          idBool          aCompareBeforeImage,
                                          rpApplyFailType aFailType )
{
    RP_CONFLICT_ERRLOG2();

    if ( IDE_TRC_RP_3 != 0 )
    {
        ideLogEntry sLog( IDE_RP_3 ); // insertErrLog에서 복잡하기 때문에 상위에서 open,close를 함. more safe

        /* BUG-36555 : Before image logging */
        (void)updateErrLog( sLog,
                            aMetaItem, 
                            aMetaItemForPK,
                            aXLog, 
                            aCompareBeforeImage,
                            aFailType );
        (void)sLog.write();
    }
    else
    {
        /* do nothing */
    }
    if ( IDE_TRC_RP_CONFLICT_3 != 0 )
    {
        ideLogEntry sLog( IDE_RP_CONFLICT_3 );

        /* BUG-36555 : Before image logging */
        (void)updateErrLog( sLog,
                            aMetaItem, 
                            aMetaItemForPK,
                            aXLog, 
                            aCompareBeforeImage,
                            aFailType );
        (void)sLog.write();

        mTransTbl->setIsConflictFlag( aXLog->mTID, ID_TRUE );
    }
    else
    {
        /* do nothing */
    }

    if ( IDE_TRC_RP_5 != 0 )
    {
        ideLogEntry sLog( IDE_RP_5 );
        printPK( sLog, 
                 "UPDATE",
                 aMetaItemForPK, 
                 aXLog->mPKCols );
        (void)sLog.write();
    }
    else
    {
        /* do nothing */
    }

    if ( IDE_TRC_RP_CONFLICT_5 != 0 )
    {
        ideLogEntry sLog( IDE_RP_CONFLICT_5 );
        printPK( sLog, 
                 "UPDATE",
                 aMetaItemForPK, 
                 aXLog->mPKCols );
        (void)sLog.write();
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiverApply::printDeleteErrLog( rpdMetaItem    * aMetaItem,
                                          rpdXLog        * aXLog,
                                          rpApplyFailType  aFailType )
{
    if ( mSmExecutor.getDeleteRowCount() <= 1 )
    {
        RP_CONFLICT_ERRLOG2();

        if ( IDE_TRC_RP_3 != 0 )
        {
            ideLogEntry sLog( IDE_RP_3 ); // insertErrLog에서 복잡하기 때문에 상위에서 open,close를 함. more safe
            (void)deleteErrLog( sLog, aMetaItem, aXLog, aFailType );
            (void)sLog.write();
        }
        else
        {
            /* do nothing */
        }

        if ( IDE_TRC_RP_CONFLICT_3 != 0 )
        {
            ideLogEntry sLog( IDE_RP_CONFLICT_3 );
            (void)deleteErrLog( sLog, aMetaItem, aXLog, aFailType );
            (void)sLog.write();

            mTransTbl->setIsConflictFlag( aXLog->mTID, ID_TRUE );
        }
        else
        {
            /* do nothing */
        }

        if ( IDE_TRC_RP_5 != 0 )
        {
            ideLogEntry sLog( IDE_RP_5 );
            printPK( sLog, 
                     "DELETE",
                     aMetaItem, 
                     aXLog->mPKCols );
            (void)sLog.write();
        }
        else
        {
            /* do nothing */
        }

        if ( IDE_TRC_RP_CONFLICT_5 != 0 )
        {
            ideLogEntry sLog( IDE_RP_CONFLICT_5 );
            printPK( sLog, 
                     "DELETE",
                     aMetaItem, 
                     aXLog->mPKCols );
            (void)sLog.write();
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        ideLogEntry sLog( IDE_ERR_0 );
        printPK( sLog, 
                 "DELETE",
                 aMetaItem, 
                 aXLog->mPKCols );
        (void)sLog.appendFormat( "\nFailed to delete, succeeded by retry" );
        (void)sLog.write();
    }

}
void rpxReceiverApply::printLobErrLog( rpdMetaItem   * aMetaItem,
                                       rpdXLog       * aXLog,
                                       rpApplyFailType aFailType )
{
    RP_CONFLICT_ERRLOG2();

    if ( IDE_TRC_RP_3 != 0 )
    {
        ideLogEntry sLog( IDE_RP_3 ); // 상위에서 open,close를 함. more safe
        (void)lobErrLog( sLog, aMetaItem, aXLog, aFailType );
        (void)sLog.write();
    }
    else
    {
        /* do nothing */
    }

    if ( IDE_TRC_RP_CONFLICT_3 != 0 )
    {
        ideLogEntry sLog( IDE_RP_CONFLICT_3 );
        (void)lobErrLog( sLog, aMetaItem, aXLog, aFailType );
        (void)sLog.write();

        mTransTbl->setIsConflictFlag( aXLog->mTID, ID_TRUE );
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpxReceiverApply::insertSQL( smiTrans    * aSmiTrans,
                                    rpdMetaItem * aMetaItem,
                                    rpdXLog     * aXLog,
                                    SChar       * aSPName )
{
    rpdMetaItem       * sRemoteMetaItem = NULL;
    rpApplyFailType     sFailType = RP_APPLY_FAIL_NONE;
    SLong               sRowCount = 0;
    smiRange            sKeyRange;
    idBool              sCheckRowExistence = ID_FALSE;
    idBool              sIsResolutionNeed = ID_FALSE;
    smiTrans          * sConflictResolutionTrans = NULL;

    IDE_DASSERT( aSmiTrans != NULL );

    IDE_TEST( mReceiver->searchTableFromRemoteMeta( &sRemoteMetaItem,
                                                    aXLog->mTableOID )
              != IDE_SUCCESS );

    if ( rpdConvertSQL::getInsertSQL( sRemoteMetaItem,
                                      aMetaItem,
                                      aXLog,
                                      mSQLBuffer,
                                      mSQLBufferLength )
         != IDE_SUCCESS )
    {
        sFailType = RP_APPLY_FAIL_INTERNAL;

        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        if ( runDML( aSmiTrans->getStatement(),
                     sRemoteMetaItem,
                     aMetaItem,
                     aXLog,
                     mSQLBuffer,
                     ID_FALSE,
                     &sRowCount,
                     &sFailType ) 
             == IDE_SUCCESS )
        {
            /* do nothing */
        }
        else
        {
            IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );

            if ( sFailType == RP_APPLY_FAIL_BY_CONFLICT )
            {
                /* conflict 처리 */
                IDE_TEST( getKeyRange( aMetaItem, 
                                       &sKeyRange, 
                                       aXLog->mACols, 
                                       ID_FALSE )
                          != IDE_SUCCESS ); 

                IDU_FIT_POINT( "rpxReceiverApply::insertSQL::corrupt");
                IDE_TEST( getCheckRowExistenceAndResolutionNeed( aSmiTrans, 
                                                                 aMetaItem,
                                                                 &sKeyRange,
                                                                 aXLog,
                                                                 &sCheckRowExistence,
                                                                 &sIsResolutionNeed,
                                                                 &sFailType )
                          != IDE_SUCCESS );

                if ( sIsResolutionNeed == ID_TRUE ) 
                {
                    IDE_TEST( insertReplaceSQL( aMetaItem,
                                                sRemoteMetaItem,
                                                aXLog,
                                                &sKeyRange,
                                                sCheckRowExistence,
                                                aSPName,
                                                mSQLBuffer,
                                                &sFailType )
                              != IDE_SUCCESS )
                }
                else
                {
                    /* do noting */
                }
            }
            else if ( sFailType == RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX )
            {
                 sConflictResolutionTrans = mTransTbl->getSmiTransForConflictResolution( aXLog->mTID );

                 if ( sConflictResolutionTrans != NULL )
                 {
                     if ( runDML( sConflictResolutionTrans->getStatement(),
                                  sRemoteMetaItem,
                                  aMetaItem,
                                  aXLog,
                                  mSQLBuffer,
                                  ID_FALSE,
                                  &sRowCount,
                                  &sFailType )
                          != IDE_SUCCESS )
                     {
                         IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );
                         IDE_ERRLOG( IDE_RP_0 );
                     }
                     else
                     {
                         /* do nothing */
                     }
                 }
                 else
                 {
                     /* do nothing */
                 }
            }
            else
            {
                IDE_DASSERT( sFailType == RP_APPLY_FAIL_BY_CORRUPTED_PAGE );
            }
        }
    }

    if ( sFailType != RP_APPLY_FAIL_NONE )
    {
        printInsertErrLog( sRemoteMetaItem, aMetaItem, aXLog, sFailType );

        // Increase INSERT_FAILURE_COUNT in V$REPRECEIVER
        mInsertFailureCount++;
        addAbortTx(aXLog->mTID, aXLog->mSN);

        //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
        mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);
    }
    else
    {
        // Increase INSERT_SUCCESS_COUNT in V$REPRECEIVER
        mInsertSuccessCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sFailType == RP_APPLY_FAIL_INTERNAL )
    {
        ideLogEntry sLog( IDE_RP_0) ; // insertErrLog에서 복잡하기 때문에 상위에서 open,close를 함. more safe
        (void)insertErrLog( sLog, sRemoteMetaItem, aXLog, sFailType );
        sLog.write();
    }
    else
    {
        /* do nothing */
    }


    addAbortTx(aXLog->mTID, aXLog->mSN);

    // Increase INSERT_FAILURE_COUNT in V$REPRECEIVER
    mInsertFailureCount++;

    //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
    mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::updateSQL( smiTrans    * aSmiTrans,
                                    rpdMetaItem * aMetaItem,
                                    rpdXLog     * aXLog )
{
    rpdMetaItem       * sRemoteMetaItem = NULL;
    SLong               sRowCount = 0;
    idBool              sCompareBeforeImage = ID_TRUE;
    rpApplyFailType     sFailType = RP_APPLY_FAIL_NONE;
    smiTrans          * sConflictResolutionTrans = NULL;

    IDE_ASSERT( aSmiTrans != NULL );

    switch ( mPolicy )
    {
        case  RPX_APPLY_POLICY_BY_PROPERTY:
            if ( RPU_REPLICATION_UPDATE_REPLACE == 1 )
            {
                sCompareBeforeImage = ID_FALSE;
            }
            else
            {
                sCompareBeforeImage = ID_TRUE;
            }
            break;

        case RPX_APPLY_POLICY_CHECK:
            sCompareBeforeImage = ID_TRUE;
            break;

        case RPX_APPLY_POLICY_FORCE:
            sCompareBeforeImage = ID_FALSE;
            break;

        default:
            IDE_DASSERT( 0 );
            break;
    }

    IDE_TEST( mReceiver->searchTableFromRemoteMeta( &sRemoteMetaItem,
                                                    aXLog->mTableOID )
              != IDE_SUCCESS );

    /* before 이미지 검사 해야 하면 where 절에 before 이미지 값 모두 포함 */
    if ( rpdConvertSQL::getUpdateSQL( sRemoteMetaItem,
                                      aMetaItem,
                                      aXLog,
                                      sCompareBeforeImage,
                                      mSQLBuffer,
                                      mSQLBufferLength )
         != IDE_SUCCESS )
    {
        sFailType = RP_APPLY_FAIL_BY_CONFLICT;

        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        if ( runDML( aSmiTrans->getStatement(),
                     sRemoteMetaItem,
                     aMetaItem,
                     aXLog,
                     mSQLBuffer,
                     sCompareBeforeImage,
                     &sRowCount,
                     &sFailType )
             != IDE_SUCCESS )
        {
            IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );

            if ( sFailType == RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX )
            {
                sConflictResolutionTrans = mTransTbl->getSmiTransForConflictResolution( aXLog->mTID );
                if ( sConflictResolutionTrans != NULL )
                {
                    if ( runDML( sConflictResolutionTrans->getStatement(),
                                 sRemoteMetaItem,
                                 aMetaItem,
                                 aXLog,
                                 mSQLBuffer,
                                 sCompareBeforeImage,
                                 &sRowCount,
                                 &sFailType )
                         != IDE_SUCCESS )
                    {
                        IDE_TEST( sFailType == RP_APPLY_FAIL_INTERNAL );
                        IDE_ERRLOG( IDE_RP_0 );
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    /* do noting */
                }
                
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* update 된 row 가 존재하지 않음 */
            if ( sRowCount == 0 )
            {
                sFailType = RP_APPLY_FAIL_BY_CONFLICT;
            }
            else
            {
                /* do nothing */
            }
        }
    }

    if ( sFailType != RP_APPLY_FAIL_NONE )
    {
        printUpdateErrLog( sRemoteMetaItem,
                           aMetaItem,
                           aXLog,
                           sCompareBeforeImage,
                           sFailType );

        mUpdateFailureCount++;
        addAbortTx(aXLog->mTID, aXLog->mSN);

        mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);
    }
    else
    {
        mUpdateSuccessCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    mUpdateFailureCount++;
    addAbortTx(aXLog->mTID, aXLog->mSN);

    mTransTbl->setSkipLobLogFlag(aXLog->mTID, ID_TRUE);

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::getConfictResolutionTransaction( smTID        aTID,
                                                          smiTrans  ** aTrans )
{
    smiTrans          * sTrans = NULL;
    idBool              sIsAlloced = ID_FALSE;
    idBool              sIsBegun = ID_FALSE;
    rpdTransTblNode   * sTblNode = NULL;

    sTblNode = mTransTbl->getTrNode( aTID );

    if ( mTransTbl->isNullTransForConflictResolution( aTID ) == ID_TRUE )
    {
        IDE_TEST( mTransTbl->allocConflictResolutionTransNode( aTID ) != IDE_SUCCESS );

        sIsAlloced = ID_TRUE;

        sTrans = mTransTbl->getSmiTransForConflictResolution( aTID );

        IDE_TEST( begin( sTrans, ID_TRUE ) != IDE_SUCCESS );
        sIsBegun = ID_TRUE;

        IDE_TEST_RAISE( sTrans->savepoint( RP_CONFLICT_RESOLUTION_BEGIN_SVP_NAME ) != IDE_SUCCESS, ERR_SET_SVP );

        if ( mTransTbl->isSetPSMSavepoint( aTID ) == ID_TRUE )
        {
            sTrans->reservePsmSvp( ID_FALSE );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sTrans = mTransTbl->getSmiTransForConflictResolution( aTID );
    }
    
    *aTrans = sTrans;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_SVP );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SET_SAVEPOINT_ERROR_IN_RUN ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegun == ID_TRUE )
    {
        IDE_ASSERT( sTrans->rollback() == IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsAlloced == ID_TRUE )
    {
        mTransTbl->removeTransNode( sTblNode->mTrans.mTransForConflictResolution );
        sTblNode->mTrans.mTransForConflictResolution = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::executeTruncate( rpxReceiver      * aReceiver,
                                          rpdMetaItem      * aMetaItem,
                                          idBool             aIsConditionSynced )
{
    smiTrans         sSmiTrans;
    smiStatement   * sRootStmt = NULL;
    smiStatement     sSmiStmt;
    UInt             sStage = 0;
    SChar            sSqlStr[QD_MAX_SQL_LENGTH];

    UInt sReplLockTimeoutOrig = 0;

    /* 이 함수의 용도
     *   1. Sync Conditional 에서 조건이 trucnate 인 경우            ( aIsConditionSynced == ID_FALSE )
     *   2. Sync Conditional 에서 조건은 sync. 도중 실패로 인한 원복 ( aIsConditionSynced == ID_TRUE )
     * */
    if ( aMetaItem->mItem.mIsPartition[0] == 'N' ) 
    {
        idlOS::snprintf( sSqlStr, ID_SIZEOF(sSqlStr),
                         "TRUNCATE TABLE %s.%s", 
                         aMetaItem->mItem.mLocalUsername,
                         aMetaItem->mItem.mLocalTablename );
    }
    else
    {
        idlOS::snprintf( sSqlStr, ID_SIZEOF(sSqlStr),
                         "ALTER TABLE %s.%s TRUNCATE PARTITION %s",
                         aMetaItem->mItem.mLocalUsername,
                         aMetaItem->mItem.mLocalTablename,
                         aMetaItem->mItem.mLocalPartname );
    }

    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sSmiTrans.begin( &sRootStmt,
                               NULL,//aApply->mReceiver->getStatistics(),
                               ( SMI_ISOLATION_NO_PHANTOM     |
                                 SMI_TRANSACTION_NORMAL       |
                                 SMI_TRANSACTION_REPL_DEFAULT |
                                 SMI_COMMIT_WRITE_WAIT ),
                               aReceiver->mReplID )
              != IDE_SUCCESS );
    sStage = 2;

    aReceiver->setSelfExecuteDDLTransID( sSmiTrans.getTransID() );

    IDE_TEST( sSmiStmt.begin( NULL,//sSmiTrans.getStatistics(),
                              sSmiTrans.getStatement(),
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sStage = 3;

    sReplLockTimeoutOrig = sSmiTrans.getReplTransLockTimeout();
    sSmiTrans.setReplTransLockTimeout( 5 ); 

    IDE_TEST( qciMisc::runDDLforInternal( sSmiTrans.getStatistics(),
                                          &sSmiStmt,
                                          QCI_EMPTY_USER_ID,
                                          QCI_SESSION_INTERNAL_DDL_TRUE,
                                          sSqlStr )
              != IDE_SUCCESS );

    sSmiTrans.setReplTransLockTimeout( sReplLockTimeoutOrig ); 
   
    if ( aIsConditionSynced == ID_TRUE )
    {
        IDE_TEST( rpdCatalog::updateConditionalSyncedWithItem( &sSmiStmt,
                                                               &aMetaItem->mItem,
                                                               ID_FALSE )
                  != IDE_SUCCESS);
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sStage = 2;
    }
    else
    {
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sStage = 2;

        IDE_TEST( aReceiver->metaRebuild( &sSmiTrans ) != IDE_SUCCESS );
    }

    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    aReceiver->setSelfExecuteDDLTransID( SM_NULL_TID );

    sStage = 0;
    IDE_TEST( sSmiTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);
    IDE_PUSH();

    aReceiver->setSelfExecuteDDLTransID( SM_NULL_TID );

    switch(sStage)
    {
        case 3:
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            IDE_ASSERT(sSmiTrans.rollback() == IDE_SUCCESS);
            /* fall through */
        case 1:
            (void)sSmiTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


IDE_RC rpxReceiverApply::executeInsert( smiTrans         * aSmiTrans,
                                        rpdXLog          * aXLog,
                                        SChar            * aSpName ) 
{
    rpdMetaItem      * sMetaItem = NULL;
    
    IDE_TEST( mReceiver->searchRemoteTable( &sMetaItem, aXLog->mTableOID )
              != IDE_SUCCESS );
    IDU_FIT_POINT_RAISE( "rpxReceiverApply::insertXLog::Erratic::rpERR_ABORT_NOT_EXIST_TABLE_INS",
                         ERR_NOT_EXIST_TABLE ); 
    IDE_TEST_RAISE( sMetaItem == NULL, ERR_NOT_EXIST_TABLE );
    
    switch ( sMetaItem->getApplyMode() )
    {
        case RP_APPLY_XLOG :
            IDE_TEST( insertXLog( aSmiTrans, 
                                  sMetaItem, 
                                  aXLog, 
                                  aSpName ) != IDE_SUCCESS );
            break;
            
        case RP_APPLY_SQL :
            IDE_TEST( insertSQL( aSmiTrans, 
                                 sMetaItem, 
                                 aXLog, 
                                 aSpName ) != IDE_SUCCESS );
            break;
            
        case RP_APPLY_SKIP :
            insertSkip( sMetaItem,
                        aXLog );
            break;
            
        default :
            IDE_DASSERT( 0 );
            IDE_RAISE( ERR_INVALID_APPLY_MODE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE );
    {
        ideLog::log( IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                     aXLog->mType,
                     aXLog->mTID,
                     aXLog->mSN,
                     aXLog->mTableOID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_EXIST_TABLE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_APPLY_MODE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Invalid Apply Mode" ) )
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::executeSyncInsert( smiTrans    * aSmiTrans,
                                            rpdXLog     * aXLog ) 
{
    rpdMetaItem      * sMetaItem = NULL;
    
    IDE_TEST( mReceiver->searchRemoteTable( &sMetaItem, aXLog->mTableOID ) 
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sMetaItem == NULL, ERR_NOT_EXIST_TABLE );
    
    switch ( sMetaItem->getApplyMode() )
    {
        case RP_APPLY_XLOG :
            IDE_TEST( insertSyncXLog( aSmiTrans,
                                      sMetaItem,
                                      aXLog )
                      != IDE_SUCCESS );
            break;
            
        case RP_APPLY_SQL :
            IDE_TEST( insertSQL( aSmiTrans, 
                                 sMetaItem, 
                                 aXLog,
                                 NULL ) 
                      != IDE_SUCCESS );
            break;
            
        case RP_APPLY_SKIP :
            insertSkip( sMetaItem, 
                        aXLog );
            break;
            
        default :
            IDE_DASSERT( 0 );
            IDE_RAISE( ERR_INVALID_APPLY_MODE );
            break;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE );
    {
        ideLog::log( IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                              aXLog->mType,
                              aXLog->mTID,
                              aXLog->mSN,
                              aXLog->mTableOID );
    
        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_EXIST_TABLE_INS ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION( ERR_INVALID_APPLY_MODE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Invalid Apply Mode" ) )
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::executeUpdate( smiTrans         * aSmiTrans,
                                        rpdXLog          * aXLog ) 
{
    rpdMetaItem      * sMetaItem = NULL;
    
    /* PROJ-1442 Replication Online 중 DDL 허용
     * Update된 Column 중에 Replication 대상이 없을 수 있다.
     */
    if ( aXLog->mColCnt > 0 )
    {
        IDE_TEST( mReceiver->searchRemoteTable( &sMetaItem, aXLog->mTableOID )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE(sMetaItem == NULL, ERR_NOT_EXIST_TABLE);
        
        switch ( sMetaItem->getApplyMode() )
        {
            case RP_APPLY_XLOG :
                IDE_TEST( updateXLog( aSmiTrans,
                                      sMetaItem,
                                      aXLog ) != IDE_SUCCESS );
                break;
                
            case RP_APPLY_SQL :
                IDE_TEST( updateSQL( aSmiTrans,
                                     sMetaItem,
                                     aXLog ) != IDE_SUCCESS );
                break;
                
            case RP_APPLY_SKIP :
                updateSkip( sMetaItem,
                            aXLog );
                break;
                
            default :
                IDE_DASSERT( 0 );
                IDE_RAISE( ERR_INVALID_APPLY_MODE );
                break;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE );
    {
        ideLog::log( IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                     aXLog->mType,
                     aXLog->mTID,
                     aXLog->mSN,
                     aXLog->mTableOID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_EXIST_TABLE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_APPLY_MODE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Invalid Apply Mode" ) )
    }       
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiverApply::insertSkip( rpdMetaItem    * aMetaItem,
                                   rpdXLog        * aXLog )
{    
    printInsertErrLog( aMetaItem, aMetaItem, aXLog, RP_APPLY_FAIL_SKIP );
    
    // Increase INSERT_FAILURE_COUNT in V$REPRECEIVER
    mInsertFailureCount++;
    addAbortTx( aXLog->mTID, aXLog->mSN );
    
    //BUG-21858 : insert, update 실패시 다음에 LOB관련 로그를 수행하면 문제가 발생 LOB로그를 무시하도록 세팅
    mTransTbl->setSkipLobLogFlag( aXLog->mTID, ID_TRUE );
}

void rpxReceiverApply::updateSkip( rpdMetaItem    * aMetaItem,
                                   rpdXLog        * aXLog )
{
    idBool              sCompareBeforeImage = ID_TRUE;
    
    switch ( mPolicy )
    {
        case  RPX_APPLY_POLICY_BY_PROPERTY:
            if ( RPU_REPLICATION_UPDATE_REPLACE == 1 )
            {
                sCompareBeforeImage = ID_FALSE;
            }
            else
            {
                sCompareBeforeImage = ID_TRUE;
            }
            break;

        case RPX_APPLY_POLICY_CHECK:
            sCompareBeforeImage = ID_TRUE;
            break;

        case RPX_APPLY_POLICY_FORCE:
            sCompareBeforeImage = ID_FALSE;
            break;
    }
    
    printUpdateErrLog( aMetaItem,
                       aMetaItem,
                       aXLog,
                       sCompareBeforeImage,
                       RP_APPLY_FAIL_SKIP );

    mUpdateFailureCount++;
    addAbortTx( aXLog->mTID, aXLog->mSN );

    mTransTbl->setSkipLobLogFlag( aXLog->mTID, ID_TRUE );
}

