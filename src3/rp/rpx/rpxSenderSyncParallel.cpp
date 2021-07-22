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
 * $Id: rpxSenderSyncParallel.cpp 88435 2020-08-27 08:31:39Z yoonhee.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>
#include <smErrorCode.h>
#include <rpuProperty.h>
#include <rpxSender.h>
#include <rpxPJMgr.h>

IDE_RC
rpxSender::syncParallel()
{
    SInt               i = 0;
    SInt               sStage = 0;
    smiStatement     * sParallelStmts = NULL;
    smiTrans         * sSyncParallelTrans = NULL;
    idBool             sIsError;
    PDL_Time_Value     sPDL_Time_Value;
    SChar            * sUsername          = NULL;
    SChar            * sTablename         = NULL;
    SChar            * sPartitionname     = NULL;

    rpxPJMgr         * sSyncer            = NULL;

    SChar            * sIPAddress         = NULL;
    SInt               sPortNo            = 0;
    rpIBLatency        sIBLatency         = RP_IB_LATENCY_NONE;

    sPDL_Time_Value.initialize(1, 0);

    IDE_TEST( allocSCN( &sParallelStmts, &sSyncParallelTrans ) != IDE_SUCCESS );
    sStage = 1;


    IDU_FIT_POINT_RAISE( "rpxSender::syncParallel::malloc::Syncer",
                          ERR_MEMORY_ALLOC_SYNCER );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_SYNC,
                                       ID_SIZEOF(rpxPJMgr),
                                       (void **)&sSyncer,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_SYNCER );
    sStage = 2;

    new ( sSyncer ) rpxPJMgr();

    if ( mSocketType == RP_SOCKET_TYPE_TCP )
    {
        getRemoteAddress( &sIPAddress, &sPortNo );
    }
    else if ( mSocketType == RP_SOCKET_TYPE_IB )
    {
        getRemoteAddressForIB( &sIPAddress, &sPortNo, &sIBLatency );
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    IDE_TEST( sSyncer->initialize( mRepName,
                                   &mPJStmt,
                                   &mMeta,
                                   mSocketType,
                                   sIPAddress,
                                   sPortNo,
                                   sIBLatency,
                                   &mExitFlag,
                                   mParallelFactor,
                                   sParallelStmts )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_ASSERT( mSyncerMutex.lock( NULL ) == IDE_SUCCESS );

    mSyncer = sSyncer;

    IDE_ASSERT( mSyncerMutex.unlock() == IDE_SUCCESS );

    //bug-13851
    for ( i = 0; i < mMeta.mReplication.mItemCount; i++ )
    {
        sUsername  = mMeta.mItemsOrderByTableOID[i]->mItem.mLocalUsername;
        sTablename = mMeta.mItemsOrderByTableOID[i]->mItem.mLocalTablename;
        sPartitionname = mMeta.mItemsOrderByTableOID[i]->mItem.mLocalPartname;

        // Replication Table이 지정한 Sync Table인 경우에만 계속 진행한다.
        if ( isSyncItem( mSyncInsertItems,
                         sUsername,
                         sTablename,
                         sPartitionname )
             != ID_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sSyncer->allocSyncItem( mMeta.mItemsOrderByTableOID[i] ) 
                  != IDE_SUCCESS );
    }

    /* Start Sync Apply 모드를 flag에 setting */
    rpdMeta::setReplFlagStartSyncApply( &(mMeta.mReplication) );
    
    IDU_FIT_POINT( "rpxSender::syncParallel::Thread::mSyncer",
                   idERR_ABORT_THR_CREATE_FAILED,
                   "rpxSender::SyncParallel",
                   "mSyncer" );
    IDE_TEST( sSyncer->start() != IDE_SUCCESS );
    sStage = 4;

    //bug-13851
    while ( sSyncer->mPJMgrExitFlag != ID_TRUE )
    {
        IDE_TEST(addXLogKeepAlive() != IDE_SUCCESS);

        IDE_TEST_RAISE(checkInterrupt() != RP_INTR_NONE, ERR_SYNC);

        idlOS::sleep(sPDL_Time_Value);
    }

    sStage = 3;
    if ( sSyncer->join() != IDE_SUCCESS )
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_JOIN_THREAD));
        IDE_ERRLOG(IDE_RP_0);
        IDE_CALLBACK_FATAL("[Repl PJMgr] Thread join error");
    }

    rpdMeta::clearReplFlagStartSyncApply( &(mMeta.mReplication) );

    sStage = 2;

    sIsError = sSyncer->getError();

    IDE_ASSERT( mSyncerMutex.lock( NULL ) == IDE_SUCCESS );

    mSyncer = NULL;

    IDE_ASSERT( mSyncerMutex.unlock() == IDE_SUCCESS );

    sSyncer->destroy();

    sStage = 1;

    (void)iduMemMgr::free( sSyncer );
    sSyncer = NULL;
   
    sStage = 0;
    destroySCN( sParallelStmts, sSyncParallelTrans );
    sParallelStmts = NULL;
    sSyncParallelTrans = NULL;
    IDE_TEST_RAISE(sIsError == ID_TRUE, ERR_SYNC);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MEMORY_ALLOC_SYNCER);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxSender::syncParallel",
                                "mSyncer"));
    }
    IDE_EXCEPTION(ERR_SYNC)
    {
        //PJMgr과 PJChild들을 종료시키기 위해서 exitflag를 설정한다.
        mExitFlag = ID_TRUE;
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    rpdMeta::clearReplFlagStartSyncApply( &(mMeta.mReplication) );

    switch( sStage )
    {
        case 4:
            if(mSyncer->join() != IDE_SUCCESS)
            {
                IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_JOIN_THREAD));
                IDE_ERRLOG(IDE_RP_0);
                IDE_CALLBACK_FATAL("[Repl PJMgr] Thread join error");
            }
        case 3:
            IDE_ASSERT( mSyncerMutex.lock( NULL ) == IDE_SUCCESS );

            mSyncer = NULL;

            IDE_ASSERT( mSyncerMutex.unlock() == IDE_SUCCESS );

            sSyncer->destroy();
        case 2:
            if ( sSyncer != NULL )
            {
                (void)iduMemMgr::free( sSyncer );
                sSyncer = NULL;
            }
        case 1:
            destroySCN( sParallelStmts, sSyncParallelTrans );

            break;
        default:
            break;
    }

    IDE_WARNING(IDE_RP_0, RP_TRC_SSP_ERR_SYNC );

    IDE_POP();

    IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_SYNCSTART));
    return IDE_FAILURE;
}

/*
 * BUG-19770 SYNC_RECORD_COUNT
 * 주의: 함수 호출 전 PJ_lock을 잡아야 함
 * aMetaItem으로 들어온 테이블의 SYNC_RECORD_COUNT를 반환
 * Sync가 끝난 경우 ULong Max을 반환
 */
ULong
rpxSender::getJobCount( SChar *aTableName )
{
    ULong       sCount = ID_ULONG_MAX;

    IDE_ASSERT( mSyncerMutex.lock( NULL ) == IDE_SUCCESS );

    if ( mSyncer != NULL )
    {
        sCount = mSyncer->getSyncedCount( aTableName );
    }
    else
    {
        sCount = ID_ULONG_MAX;
    }

    IDE_ASSERT( mSyncerMutex.unlock() == IDE_SUCCESS );

    return sCount;
}

IDE_RC
rpxSender::allocSCN( smiStatement ** aParallelStatements,
                     smiTrans     ** aSyncParallelTrans )
{
    UInt            sFlag = 0;
    idBool          sIsPJStmtBegin = ID_FALSE;
    idBool          sIsParallelStatementsAlloced = ID_FALSE;
    smiTrans      * sTransForLock;
    smiStatement  * sRootStmt = NULL;
    smiStatement    sStmtForLock;
    idBool          sIsTxBegin = ID_FALSE;
    SInt            sStep = 0;

    IDU_FIT_POINT( "rpxSender::allocSCN::malloc::TransForLock" );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPX_SYNC,
                                 ID_SIZEOF(smiTrans),
                                 (void **)&sTransForLock,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sStep = 1;

    sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
    sFlag = (sFlag & ~SMI_TRANSACTION_MASK)| SMI_TRANSACTION_UNTOUCHABLE;
    sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
    sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_NOWAIT;

    IDE_TEST( sTransForLock->initialize() != IDE_SUCCESS );
    sStep = 2;

    IDE_TEST( sTransForLock->begin( &sRootStmt,
                                   NULL,
                                   sFlag,
                                   SMX_NOT_REPL_TX_ID )
              != IDE_SUCCESS );
    sIsTxBegin = ID_TRUE;
    sStep = 3;

    sTransForLock->savepoint( RPX_SENDER_SVP_NAME, NULL );

    /* Lock을 유지하기 위해 Statement Begin */
    IDE_TEST( sStmtForLock.begin( NULL,
                                  sRootStmt,
                                  SMI_STATEMENT_UNTOUCHABLE |
                                  SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sStep = 4;

    IDE_TEST( lockTableforSync( &sStmtForLock ) != IDE_SUCCESS );

    IDE_TEST( setRestartSNforSync() != IDE_SUCCESS );

    /* 현재 SCN의 View를 SYNC 종료까지 유지하기 위해 Statement Begin */
    IDE_TEST( mPJStmt.begin( NULL,
                             sRootStmt,
                             SMI_STATEMENT_UNTOUCHABLE |
                             SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sIsPJStmtBegin = ID_TRUE;

    IDE_TEST( allocNBeginParallelStatements( aParallelStatements )
              != IDE_SUCCESS );
    sIsParallelStatementsAlloced = ID_TRUE;

    IDE_TEST( sStmtForLock.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStep = 3;

    sStep = 2;
    IDE_TEST( sTransForLock->rollback( RPX_SENDER_SVP_NAME ) != IDE_SUCCESS );

    *aSyncParallelTrans = sTransForLock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    IDE_PUSH();

    if ( sIsPJStmtBegin == ID_TRUE )
    {
        (void)mPJStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    if ( sIsParallelStatementsAlloced == ID_TRUE )
    {
        destroyParallelStatements( *aParallelStatements );
    }

    switch ( sStep )
    {
        case 4 :
            (void)sStmtForLock.end( SMI_STATEMENT_RESULT_FAILURE );
        case 3 :
            IDE_ASSERT( sTransForLock->rollback() == IDE_SUCCESS );
            sIsTxBegin = ID_FALSE;
        case 2 :
            if ( sIsTxBegin == ID_TRUE )
            {
                IDE_ASSERT( sTransForLock->rollback() == IDE_SUCCESS );
            }
            (void)sTransForLock->destroy( NULL );
        case 1 :
            (void)iduMemMgr::free( sTransForLock );
        default :
            break;
    }

    IDE_POP();

    IDE_WARNING( IDE_RP_0, RP_TRC_SSP_ERR_ALLOC_SCN );

    return IDE_FAILURE;
}

void rpxSender::destroySCN( smiStatement * aParallelStatements,
                            smiTrans     * aSyncParallelTrans )
{
    if(mPJStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_CLEAR();
        IDE_WARNING(IDE_RP_0, RP_TRC_SSP_ERR_DESTOY_SCN );

        (void)mPJStmt.end(SMI_STATEMENT_RESULT_FAILURE);
    }

    destroyParallelStatements( aParallelStatements );
    (void)aSyncParallelTrans->rollback();
    (void)aSyncParallelTrans->destroy( NULL );
    (void)iduMemMgr::free( aSyncParallelTrans );
    return;
}

/**
 * @breif  Sync 이후에 Replication을 시작할 위치를 메타 캐시/테이블에 설정한다.
 *
 *         Replication 시작 SN = 마지막으로 사용한 SN
 *
 * @return 작업 성공/실패
 */
IDE_RC rpxSender::setRestartSNforSync()
{
    SInt    i;
    smSN    sCurrentSN;
 
    IDE_TEST_CONT( mIsSetRestartSNforSync == ID_TRUE, NORMAL_EXIT );
    
    sCurrentSN = smiGetValidMinSNOfAllActiveTrans();
    IDE_TEST_RAISE( sCurrentSN == SM_SN_NULL, ERR_RESTARTSN_FOR_SYNC );

    if ( ( mSyncInsertItems == NULL ) &&
         ( mSyncTruncateItems == NULL ) )
    {
        /* 일반 sync */
        mXSN = sCurrentSN;
        IDE_TEST( updateXSN( mXSN ) != IDE_SUCCESS );
        mCommitXSN = mXSN;
    }
    else
    {
        for ( i = 0; i < mMeta.mReplication.mItemCount; i++ )
        {
            /* 하나의 table이 mSyncInsertItems, mSyncTruncateItems에 중복으로 존재할 수 없다. */
            IDE_TEST( findAndUpdateInvalidMaxSN( mSvcThrRootStmt,
                                                 mSyncInsertItems,
                                                 &mMeta.mItemsOrderByTableOID[i]->mItem,
                                                 sCurrentSN )
                      != IDE_SUCCESS );

            IDE_TEST( findAndUpdateInvalidMaxSN( mSvcThrRootStmt,
                                                 mSyncTruncateItems,
                                                 &mMeta.mItemsOrderByTableOID[i]->mItem,
                                                 sCurrentSN )
                      != IDE_SUCCESS );
        }
    }

    mIsSetRestartSNforSync = ID_TRUE;
    
    RP_LABEL(NORMAL_EXIT);
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RESTARTSN_FOR_SYNC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  "rpxSender::setRestartSNforSync" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * @breif  Sync하기 전에 Table에 S, IS Lock을 잡는다.
 *
 *         S Lock  : Sync 전에 다수의 Child가 같은 SCN을 얻을 수 있게 한다.
 *         IS Lock : Sync 중에 DDL을 수행할 수 없도록 한다.
 *
 * @param  aStatementForLock    S Lock을 잡을 Transaction의 Statement
 *
 * @return 작업 성공/실패
 */
IDE_RC rpxSender::lockTableforSync( smiStatement * aStatementForLock )
{
    SInt    i;
    SChar * sUsername = NULL;
    SChar * sTablename = NULL;
    SChar * sPartname  = NULL;
    SChar   sMsgBuffer[256];

    if ( ( RPU_REPLICATION_SYNC_LOCK_TIMEOUT != 0 ) &&
         ( mCurrentType != RP_SYNC_CONDITIONAL ) )
    {
        for ( i = 0; i < mMeta.mReplication.mItemCount; i++ )
        {
            sUsername  = mMeta.mItemsOrderByTableOID[i]->mItem.mLocalUsername;
            sTablename = mMeta.mItemsOrderByTableOID[i]->mItem.mLocalTablename;
            sPartname = mMeta.mItemsOrderByTableOID[i]->mItem.mLocalPartname;

            /* BUG-24185 [RP] Table을 지정하여 SYNC하는 경우,
             *           지정한 Table에만 Lock을 잡아야 합니다
             *
             * Replication Table이 지정한 Sync Table인 경우에만 계속 진행한다.
             */
            if ( isSyncItem( mSyncInsertItems,
                             sUsername,
                             sTablename,
                             sPartname )
                 != ID_TRUE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            /* Table S lock */
            IDE_TEST( mMeta.mItemsOrderByTableOID[i]->lockReplItem( aStatementForLock->getTrans(),
                                                                    aStatementForLock,
                                                                    SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                    SMI_TABLE_LOCK_S,
                                                                    (ULong)RPU_REPLICATION_SYNC_LOCK_TIMEOUT * 1000000 )
                      != IDE_SUCCESS );

            /* Table IS lock
             *
             * PROJ-1442 Replication Online 중 DDL 허용
             *           Service Thread의 Transaction으로 IS Lock을 잡는다.
             */
            IDE_TEST( mMeta.mItemsOrderByTableOID[i]->lockReplItem( mSvcThrRootStmt->getTrans(),
                                                                    aStatementForLock,
                                                                    SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                    SMI_TABLE_LOCK_IS,
                                                                    (ULong)RPU_REPLICATION_SYNC_LOCK_TIMEOUT * 1000000 )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /*nothing to do*/
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sPartname == NULL )
    {
        idlOS::snprintf( sMsgBuffer, ID_SIZEOF(sMsgBuffer),
                         "[SenderSyncParallel] Failed to lock table for replication synchronization. (User:%s, Table:%s)",
                         sUsername, sTablename );
    }
    else
    {
        if ( sPartname[0] == '\0' )
        {
            idlOS::snprintf( sMsgBuffer, ID_SIZEOF(sMsgBuffer),
                             "[SenderSyncParallel] Failed to lock table for replication synchronization. (User:%s, Table:%s)",
                             sUsername, sTablename );
        }
        else
        {
            idlOS::snprintf( sMsgBuffer, ID_SIZEOF(sMsgBuffer),
                             "[SenderSyncParallel] Failed to lock table for replication synchronization. (User:%s, Table:%s, Partition:%s)",
                             sUsername, sTablename, sPartname );
        }
    }
    ideLog::log( IDE_RP_0, "%s", sMsgBuffer );

    return IDE_FAILURE;
}

/**
 * @breif  mParallelFactor 만큼의 Statement 배열을 할당하고 시작한다.
 *
 * @param  aParallelStatements 할당하고 시작한 Statement 배열
 *
 * @return 작업 성공/실패
 */
IDE_RC rpxSender::allocNBeginParallelStatements(
                  smiStatement ** aParallelStatements )
{
    SInt           i = 0;
    smiTrans     * sTransArray = NULL;
    smiStatement * sStmtArray = NULL;
    smiStatement * sRootStmt = NULL;
    UInt           sFlag = 0;
    UInt           sStep = 0;

    sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
    sFlag = (sFlag & ~SMI_TRANSACTION_MASK)| SMI_TRANSACTION_UNTOUCHABLE;
    sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
    sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_NOWAIT;

    IDU_FIT_POINT_RAISE( "rpxSender::allocNBeginParallelStatements::malloc::TransArry",
                          ERR_MEMORY_ALLOC_TRANS_ARRAY );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_SYNC,
                                       ID_SIZEOF(smiTrans) * mParallelFactor,
                                       (void **)&sTransArray,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_TRANS_ARRAY );

    IDU_FIT_POINT_RAISE( "rpxSender::allocNBeginParallelStatements::malloc::StmtArray",
                          ERR_MEMORY_ALLOC_STMT_ARRAY );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_SYNC,
                                       ID_SIZEOF(smiStatement) * mParallelFactor,
                                       (void **)&sStmtArray,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_STMT_ARRAY );

    for ( i = 0; i < mParallelFactor; i++ )
    {
        sStep = 0;

        IDE_TEST( sTransArray[i].initialize() != IDE_SUCCESS );
        sStep = 1;

        IDE_TEST( sTransArray[i].begin( &sRootStmt,
                                        NULL,
                                        sFlag,
                                        SMX_NOT_REPL_TX_ID )
                  != IDE_SUCCESS );
        sStep = 2;

        IDE_TEST( sStmtArray[i].begin( NULL, sRootStmt, SMI_STATEMENT_UNTOUCHABLE |
                                                        SMI_STATEMENT_ALL_CURSOR )
                  != IDE_SUCCESS );
    }

    *aParallelStatements = sStmtArray;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_TRANS_ARRAY );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxSender::allocNBeginParallelStatements",
                                  "sTransArray" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_STMT_ARRAY );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxSender::allocNBeginParallelStatements",
                                  "sStmtArray" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch (sStep)
    {
        case 2 :
            IDE_ASSERT( sTransArray[i].rollback() == IDE_SUCCESS );
        case 1 :
            (void)sTransArray[i].destroy( NULL );
        default :
            break;
    }

    for ( i--; i >= 0; i-- )
    {
        (void)sStmtArray[i].end( SMI_STATEMENT_RESULT_FAILURE );
        IDE_ASSERT( sTransArray[i].rollback() == IDE_SUCCESS );
        (void)sTransArray[i].destroy( NULL );
    }

    if ( sStmtArray != NULL )
    {
        (void)iduMemMgr::free( sStmtArray );
    }
    if ( sTransArray != NULL )
    {
        (void)iduMemMgr::free( sTransArray );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/**
 * @breif  Statement 배열을 종료하고, Statement의 Transaction을 제거한다.
 *
 *         rpxSender::allocNBeginParallelStatements()에서 시작한 작업을 종료한다.
 *
 * @param  aParallelStatements 제거할 Statement 배열
 */
void rpxSender::destroyParallelStatements( smiStatement * aParallelStatements )
{
    smiTrans * sTransArray = NULL;
    SInt       i;

    /* Statement와 Transaction을 종료한다. */
    for ( i = 0; i < mParallelFactor; i++ )
    {
        sTransArray = aParallelStatements[i].getTrans();
        if ( aParallelStatements[i].end( SMI_STATEMENT_RESULT_SUCCESS )
             != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
            IDE_CLEAR();

            (void)aParallelStatements[i].end( SMI_STATEMENT_RESULT_FAILURE );
        }
        if ( sTransArray->commit() != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
            IDE_CLEAR();

            IDE_ASSERT( sTransArray->rollback() == IDE_SUCCESS );
        }
        if ( sTransArray->destroy( NULL ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
            IDE_CLEAR();
        }
    }

    /* Transaction 배열 메모리를 해제한다. */
    sTransArray = aParallelStatements[0].getTrans();
    (void)iduMemMgr::free( sTransArray );

    /* Statement 배열 메모리를 해제한다. */
    (void)iduMemMgr::free( aParallelStatements );

    return;
}
