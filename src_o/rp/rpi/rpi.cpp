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
 * $Id: rpi.cpp 90266 2021-03-19 05:23:09Z returns $
 **********************************************************************/

#include <idl.h>
#include <smi.h>
#include <rpi.h>

#include <rpcManager.h>
#include <rpdMeta.h>
#include <rpcValidate.h>
#include <rpcExecute.h>
#include <rpdCatalog.h>

IDE_RC rpi::initRPProperty()
{
    return rpuProperty::initProperty();
}

IDE_RC
rpi::initREPLICATION()
{
    return rpcManager::initREPLICATION();
}

IDE_RC
rpi::finalREPLICATION()
{
    return rpcManager::finalREPLICATION();
}

IDE_RC
rpi::createReplication( void * aQcStatement )
{
    return rpcManager::createReplication( aQcStatement );
}

IDE_RC
rpi::alterReplicationFlush( smiStatement  * aSmiStmt,
                            SChar         * aReplName,
                            rpFlushOption * aFlushOption,
                            idvSQL        * aStatistics )
{
    if ( aFlushOption->flushType == RP_FLUSH_XLOGFILE )
    {
        /* ALTER REPLICATION rep_name FLUSH FROM XLOGFILE */
        IDE_TEST( rpcManager::alterReplicationFlushWithXLogfiles( aSmiStmt,
                                                                  aReplName,
                                                                  aStatistics )
                  != IDE_SUCCESS );
    }
    else
    {
        /* ALTER REPLICATION rep_name FLUSH WAIT sec | ALL */
         IDE_TEST( rpcManager::alterReplicationFlushWithXLogs( aSmiStmt,
                                                               aReplName,
                                                               aFlushOption,
                                                               aStatistics )
                   != IDE_SUCCESS );


    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpi::alterReplicationAddTable( void * aQcStatement )
{
    return rpcManager::alterReplicationAddTable( aQcStatement );
}

IDE_RC
rpi::alterReplicationDropTable( void * aQcStatement )
{
    return rpcManager::alterReplicationDropTable( aQcStatement );
}

IDE_RC
rpi::alterReplicationAddHost( void * aQcStatement )
{
    return rpcManager::alterReplicationAddHost( aQcStatement );
}

IDE_RC
rpi::alterReplicationDropHost( void * aQcStatement )
{
    return rpcManager::alterReplicationDropHost( aQcStatement );
}

IDE_RC
rpi::alterReplicationSetHost( void * aQcStatement )
{
    return rpcManager::alterReplicationSetHost( aQcStatement );
}

IDE_RC 
rpi::alterReplicationSetRecovery( void * aQcStatement )
{
    return rpcManager::alterReplicationSetRecovery( aQcStatement );
}

/* PROJ-1915 */
IDE_RC 
rpi::alterReplicationSetOfflineEnable( void * aQcStatement )
{
    return rpcManager::alterReplicationSetOfflineEnable( aQcStatement );
}

IDE_RC 
rpi::alterReplicationSetOfflineDisable( void * aQcStatement )
{
    return rpcManager::alterReplicationSetOfflineDisable( aQcStatement );
}

/* PROJ-1969 */
IDE_RC rpi::alterReplicationSetGapless( void * aQcStatement )
{
    return rpcManager::alterReplicationSetGapless( aQcStatement );
}

IDE_RC rpi::alterReplicationSetParallel( void * aQcStatement )
{
    return rpcManager::alterReplicationSetParallel( aQcStatement );
}

IDE_RC rpi::alterReplicationSetGrouping( void * aQcStatement )
{
    return rpcManager::alterReplicationSetGrouping( aQcStatement );
}

IDE_RC rpi::alterReplicationSetDDLReplicate( void * aQcStatement )
{
    return rpcManager::alterReplicationSetDDLReplicate( aQcStatement );
}

IDE_RC
rpi::dropReplication( void * aQcStatement )
{
    return rpcManager::dropReplication( aQcStatement );
}

IDE_RC
rpi::startSenderThread( idvSQL                  * aStatistics,
                        iduVarMemList           * aMemory,
                        smiStatement            * aSmiStmt,
                        SChar                   * aReplName,
                        RP_SENDER_TYPE            aStartType,
                        idBool                    aTryHandshakeOnce,
                        smSN                      aStartSN,
                        qciSyncItems            * aSyncItemList,
                        SInt                      aParallelFactor,
                        void                    * aLockTable )
{
    IDE_TEST( rpcManager::startSenderThread( aStatistics,
                                             aMemory,
                                             aSmiStmt,
                                             aReplName,
                                             aStartType,
                                             aTryHandshakeOnce,
                                             aStartSN,
                                             aSyncItemList,
                                             aParallelFactor,
                                             aLockTable )
              != IDE_SUCCESS);

    /* BUG-34316 Replication giveup_time must be updated
     * when replication pair were synchronized.*/
    IDE_TEST( rpcManager::resetGiveupInfo(aStartType,
                                          aSmiStmt,
                                          aReplName) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpi::startTempSync( void * aQcStatement )
{
    IDE_TEST( rpcManager::startTempSync( aQcStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpi::stopSenderThread(smiStatement * aSmiStmt,
                      SChar        * aReplName,
                      idvSQL       * aStatistics,
                      idBool         aIsImmediate )
{
    return rpcManager::stopSenderThread( aSmiStmt,
                                         aReplName,
                                         aStatistics,
                                         aIsImmediate );
}

IDE_RC rpi::resetReplication(smiStatement * aSmiStmt,
                             SChar        * aReplName,
                             idvSQL       * aStatistics)
{
    return rpcManager::resetReplication(aSmiStmt, aReplName, aStatistics);
}


extern iduFixedTableDesc gReplManagerTableDesc;
extern iduFixedTableDesc gReplSenderTableDesc;
extern iduFixedTableDesc gReplSenderStatisticsTableDesc;
extern iduFixedTableDesc gReplReceiverTableDesc;
extern iduFixedTableDesc gReplReceiverParallelApplyTableDesc;
extern iduFixedTableDesc gReplReceiverStatisticsTableDesc;
extern iduFixedTableDesc gReplGapTableDesc;
extern iduFixedTableDesc gReplSyncTableDesc;
extern iduFixedTableDesc gReplSenderTransTblTableDesc;
extern iduFixedTableDesc gReplReceiverTransTblTableDesc;
extern iduFixedTableDesc gReplReceiverColumnTableDesc;
extern iduFixedTableDesc gReplSenderColumnTableDesc;
extern iduFixedTableDesc gReplRecoveryTableDesc;
extern iduFixedTableDesc gReplLogBufferTableDesc;
extern iduFixedTableDesc gReplOfflineStatusTableDesc;
extern iduFixedTableDesc gReplSenderSentLogCountTableDesc;
extern iduFixedTableDesc gReplicatedTransGroupInfoTableDesc;
extern iduFixedTableDesc gReplicatedTransSlotInfoTableDesc;
extern iduFixedTableDesc gAheadAnalyzerInfoTableDesc;
extern iduFixedTableDesc gXLogTransferTableDesc;
extern iduFixedTableDesc gXLogfileManagerInfoDesc;

IDE_RC rpi::initSystemTables()
{
    // initialize fixed table for RP
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplManagerTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplSenderTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplSenderStatisticsTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplReceiverTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplReceiverParallelApplyTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplReceiverStatisticsTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplGapTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplSyncTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplSenderTransTblTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplReceiverTransTblTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplReceiverColumnTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplSenderColumnTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplRecoveryTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplLogBufferTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplOfflineStatusTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplSenderSentLogCountTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplicatedTransGroupInfoTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gReplicatedTransSlotInfoTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gAheadAnalyzerInfoTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gXLogTransferTableDesc);
    IDU_FIXED_TABLE_DEFINE_RUNTIME(gXLogfileManagerInfoDesc);

    // initialize performance view for RP
    SChar * sPerfViewTable[] = { RP_PERFORMANCE_VIEWS, NULL };
    SInt    i = 0;

    while(sPerfViewTable[i] != NULL)
    {
        qciMisc::addPerformanceView( sPerfViewTable[i] );
        i++;
    }

    return IDE_SUCCESS;
}

void rpi::applyStatisticsForSystem()
{
    if(smiGetStartupPhase() == SMI_STARTUP_SERVICE)
    {
        /* 모든 sender와 receiver 쓰레드들의 세션에 있는 통계정보를 시스템에 반영 */
        rpcManager::applyStatisticsForSystem();
    }
}

qciValidateReplicationCallback rpi::getReplicationValidateCallback( )
{
    return rpcValidate::mCallback;
};

qciExecuteReplicationCallback rpi::getReplicationExecuteCallback( )
{
    return rpcExecute::mCallback;
};

qciCatalogReplicationCallback rpi::getReplicationCatalogCallback( )
{
    return rpdCatalog::mCallback;
};

qciManageReplicationCallback rpi::getReplicationManageCallback( )
{
    return rpcManager::mCallback;
};

IDE_RC rpi::ddlSyncBegin( qciStatement  * aQciStatement, 
                          smiStatement  * aSmiStatement )
{
    smiTrans     * sDDLTrans     = NULL;
    UInt           sStmtFlag     = 0;
    idBool         sIsStmtBegin  = ID_TRUE;
    idBool         sSwapSmiStmt  = ID_FALSE;
    smiStatement * sSmiStatementOrig = NULL;

    IDE_TEST_CONT( qciMisc::isDDLSync( aQciStatement ) != ID_TRUE, NORMAL_EXIT );
    IDE_TEST_CONT( qciMisc::isReplicableDDL( aQciStatement ) != ID_TRUE, NORMAL_EXIT );
    IDE_TEST_RAISE( qciMisc::isLockTableUntillNextDDL( aQciStatement ) == ID_TRUE,
                    ERR_DDL_SYNC_WITH_LOCK_UNTIL_NEXT_DDL );

    sDDLTrans    = aSmiStatement->getTrans();
    sStmtFlag    = aSmiStatement->mFlag;

    IDE_TEST_RAISE( qciMisc::getTransactionalDDL( (void*)&(aQciStatement->statement) ) == ID_TRUE, 
                    ERR_NOT_SUPPORT_TRANSACTIONAL_DDL );

    qciMisc::getSmiStmt( aQciStatement, &sSmiStatementOrig );
    qciMisc::setSmiStmt( aQciStatement, aSmiStatement );
    sSwapSmiStmt = ID_TRUE;

    IDE_TEST( aSmiStatement->end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtBegin = ID_FALSE;

    IDE_TEST( rpcManager::ddlSyncBegin( aQciStatement ) != IDE_SUCCESS );

    sIsStmtBegin = ID_TRUE;
    IDE_ASSERT( aSmiStatement->begin( sDDLTrans->getStatistics(),
                                      sDDLTrans->getStatement(),
                                      sStmtFlag )
                == IDE_SUCCESS );

    qciMisc::setSmiStmt( aQciStatement, sSmiStatementOrig );
    sSwapSmiStmt = ID_FALSE;

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_TRANSACTIONAL_DDL );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  "Transactional DDL not supported DDL synchronization." ) )
    }
    IDE_EXCEPTION( ERR_DDL_SYNC_WITH_LOCK_UNTIL_NEXT_DDL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_DDL_SYNC_WITH_LOCK_UNTIL_NEXT_DDL ) );
    } 
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsStmtBegin != ID_TRUE )
    {
        sIsStmtBegin = ID_TRUE;
        IDE_ASSERT( aSmiStatement->begin( sDDLTrans->getStatistics(),
                                          sDDLTrans->getStatement(),
                                          sStmtFlag )
                    == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( sSwapSmiStmt == ID_TRUE )
    {
        qciMisc::setSmiStmt( aQciStatement, sSmiStatementOrig );
        sSwapSmiStmt = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpi::ddlSyncEnd( qciStatement * aQciStatement,
                        smiStatement * aSmiStatement )
{
    idBool         sIsStmtBegin  = ID_TRUE;
    smiTrans     * sDDLTrans     = NULL;
    UInt           sStmtFlag     = 0;
    idBool         sSwapSmiStmt  = ID_FALSE;
    smiStatement * sSmiStatementOrig = NULL;

    IDE_TEST_CONT( qciMisc::isDDLSync( aQciStatement ) != ID_TRUE, NORMAL_EXIT );
    IDE_TEST_CONT( qciMisc::isReplicableDDL( aQciStatement ) != ID_TRUE, NORMAL_EXIT );
    IDE_DASSERT( qciMisc::isLockTableUntillNextDDL( aQciStatement ) != ID_TRUE );

    sDDLTrans    = aSmiStatement->getTrans();
    sStmtFlag    = aSmiStatement->mFlag;

    IDE_TEST_RAISE( qciMisc::getTransactionalDDL( (void*)&(aQciStatement->statement) ) == ID_TRUE, 
                    ERR_NOT_SUPPORT_TRANSACTIONAL_DDL );

    qciMisc::getSmiStmt( aQciStatement, &sSmiStatementOrig );
    qciMisc::setSmiStmt( aQciStatement, aSmiStatement );
    sSwapSmiStmt = ID_TRUE;

    IDE_TEST( aSmiStatement->end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtBegin = ID_FALSE;

    IDE_TEST( rpcManager::ddlSyncEnd( sDDLTrans ) != IDE_SUCCESS );

    sIsStmtBegin = ID_TRUE;
    IDE_ASSERT( aSmiStatement->begin( sDDLTrans->getStatistics(),
                                      sDDLTrans->getStatement(),
                                      sStmtFlag )
                == IDE_SUCCESS );

    qciMisc::setSmiStmt( aQciStatement, sSmiStatementOrig );
    sSwapSmiStmt = ID_FALSE;

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_TRANSACTIONAL_DDL );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  "Transactional DDL not supported DDL synchronization." ) )
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsStmtBegin != ID_TRUE )
    {
        sIsStmtBegin = ID_TRUE;
        IDE_ASSERT( aSmiStatement->begin( sDDLTrans->getStatistics(),
                                          sDDLTrans->getStatement(),
                                          sStmtFlag )
                    == IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( aSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE ) == IDE_SUCCESS );
        sIsStmtBegin = ID_FALSE;

        rpcManager::ddlSyncException( sDDLTrans ); 

        sIsStmtBegin = ID_TRUE;
        IDE_ASSERT( aSmiStatement->begin( sDDLTrans->getStatistics(),
                                          sDDLTrans->getStatement(),
                                          sStmtFlag )
                    == IDE_SUCCESS );
    }

    if ( sSwapSmiStmt == ID_TRUE )
    {
        qciMisc::setSmiStmt( aQciStatement, sSmiStatementOrig );
        sSwapSmiStmt = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;

}

void rpi::ddlSyncException( qciStatement * aQciStatement, 
                            smiStatement * aSmiStatement )

{
    smiTrans     * sDDLTrans     = NULL;
    UInt           sStmtFlag     = 0;
    smiStatement * sSmiStatementOrig = NULL;

    IDE_TEST_CONT( qciMisc::isDDLSync( aQciStatement ) != ID_TRUE, NORMAL_EXIT );
    IDE_TEST_CONT( qciMisc::isReplicableDDL( aQciStatement ) != ID_TRUE, NORMAL_EXIT );

    qciMisc::getSmiStmt( aQciStatement, &sSmiStatementOrig );
    qciMisc::setSmiStmt( aQciStatement, aSmiStatement );

    sDDLTrans    = aSmiStatement->getTrans();
    sStmtFlag    = aSmiStatement->mFlag;

    IDE_ASSERT( aSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE ) == IDE_SUCCESS );

    rpcManager::ddlSyncException( sDDLTrans );

    IDE_ASSERT( aSmiStatement->begin( sDDLTrans->getStatistics(),
                                      sDDLTrans->getStatement(),
                                      sStmtFlag )
                == IDE_SUCCESS );

    qciMisc::setSmiStmt( aQciStatement, sSmiStatementOrig );

    RP_LABEL( NORMAL_EXIT );
}

