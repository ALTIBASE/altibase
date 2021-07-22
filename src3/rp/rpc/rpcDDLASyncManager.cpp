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
* $Id:
***********************************************************************/

#include <rpcDDLSyncManager.h>
#include <rpcDDLASyncManager.h>
#include <rpdCatalog.h>

IDE_RC rpcDDLASyncManager::ddlASynchronization( rpxSender * aSender,
                                                smTID       aTID, 
                                                smSN        aDDLCommitSN )
{
    IDE_TEST( ddlASyncStart( aSender ) != IDE_SUCCESS );
    IDE_TEST_CONT( aSender->checkInterrupt() != RP_INTR_NONE, NORMAL_EXIT );

    IDE_TEST( executeDDLASync( aSender,
                               aTID,
                               aDDLCommitSN )
              != IDE_SUCCESS );
    IDE_TEST_CONT( aSender->checkInterrupt() != RP_INTR_NONE, NORMAL_EXIT );

    RP_LABEL( NORMAL_EXIT );

    aSender->resumeApply();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aSender->resumeApply();

    IDE_ERRLOG( IDE_RP_0 );

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::ddlASyncStart( rpxSender * aSender )
{
    rpXLogType sType = RP_X_NONE;

    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization start" );

    if ( aSender->sendDDLASyncStart( (UInt)RP_X_DDL_REPLICATE_HANDSHAKE ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
        aSender->setRestartErrorAndDeactivate();
        ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization failure" );

        IDE_CONT( NORMAL_EXIT );
    }

    while( aSender->isSuspendedApply() != ID_TRUE )
    {
        IDE_TEST_CONT( aSender->checkInterrupt() != RP_INTR_NONE, NORMAL_EXIT );
        idlOS::thr_yield();
    }

    if ( aSender->recvDDLASyncStartAck( (UInt*)&sType ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
        aSender->setRestartErrorAndDeactivate();
        ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization failure" );

        IDE_CONT( NORMAL_EXIT );
    }
    IDE_TEST_RAISE( sType != RP_X_DDL_REPLICATE_HANDSHAKE_ACK, ERR_INVALID_XLOG );

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_XLOG );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Wrong xlog type" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::executeDDLASync( rpxSender * aSender,
                                            smTID       aTID,
                                            smSN        aDDLCommitSN )
{
    UInt     sErrCode     = 0;
    UInt     sTargetCount = 0;
    idBool   sIsSuccess   = ID_FALSE;
    SChar  * sTargetPartNames = NULL;
    SChar    sErrMsg[RP_MAX_MSG_LEN + 1];
    SChar    sUserName[SMI_MAX_NAME_LEN + 1];
    SChar    sDDLStmt[RP_DDL_SQL_BUFFER_MAX + 1];
    SChar    sTargetTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    rpXLogType sType = RP_X_NONE;

    idlOS::memset( sTargetTableName, 0, QC_MAX_OBJECT_NAME_LEN + 1 );
    idlOS::memset( sUserName, 0, SMI_MAX_NAME_LEN + 1 );
    idlOS::memset( sDDLStmt, 0, RP_DDL_SQL_BUFFER_MAX + 1 );

    IDE_TEST( getDDLASyncInfo( aTID,
                               aSender,
                               RP_DDL_SQL_BUFFER_MAX,
                               &sTargetCount,
                               sTargetTableName,
                               &sTargetPartNames,
                               sUserName,
                               sDDLStmt )
              != IDE_SUCCESS );

    if ( aSender->sendDDLASyncExecute( (UInt)RP_X_DDL_REPLICATE_EXECUTE,
                                       sUserName,
                                       RPU_REPLICATION_DDL_ENABLE_LEVEL,
                                       sTargetCount,
                                       sTargetTableName,
                                       sTargetPartNames,
                                       aDDLCommitSN,
                                       sDDLStmt )
         != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
        aSender->setRestartErrorAndDeactivate();
        ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization failure" );

        IDE_CONT( NORMAL_EXIT );
    }

    ideLog::log( IDE_RP_0, "[DDLASyncManager] Send DDL <%s> to Standby <Replication : %s> for DDL asynchronization", 
                 sDDLStmt, aSender->getRepName() );

    printTargetInfo( sTargetCount,
                     sUserName,
                     sTargetTableName,
                     sTargetPartNames );

    if ( aSender->recvDDLASyncExecuteAck( (UInt*)&sType,
                                          (UInt*)&sIsSuccess,
                                          &sErrCode,
                                          sErrMsg )
         != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
        aSender->setRestartErrorAndDeactivate();
        ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization failure" );

        IDE_CONT( NORMAL_EXIT );
    }
    IDE_TEST_RAISE( sType != RP_X_DDL_REPLICATE_EXECUTE_ACK, ERR_INVALID_XLOG );

    if ( sIsSuccess == ID_TRUE )
    {
        ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization success" );
    }
    else
    {
        ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization receive failure message : %s", sErrMsg );

        IDE_TEST_RAISE( isRetryDDLError( sErrCode ) != ID_TRUE, ERR_DDL_ASYNC_FAIL );
        aSender->setRestartErrorAndDeactivate();
        ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization retry" );

        idlOS::sleep( 1 );
    }

    RP_LABEL( NORMAL_EXIT );

    (void)iduMemMgr::free( sTargetPartNames );
    sTargetPartNames = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_XLOG );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Wrong xlog type" ) );
    }
    IDE_EXCEPTION( ERR_DDL_ASYNC_FAIL );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL asynchronization failure." ) );
    }
    IDE_EXCEPTION_END;

    if ( sTargetPartNames != NULL )
    {
        (void)iduMemMgr::free( sTargetPartNames );
        sTargetPartNames = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::getDDLASyncInfo( smTID       aTID,
                                            rpxSender * aSender,
                                            SInt        aMaxDDLStmtLen,
                                            UInt      * aTargetCount,
                                            SChar     * aTargetTableName,
                                            SChar    ** aTargetPartNames,
                                            SChar     * aUserName,
                                            SChar     * aDDLStmt )
{
    UInt sTargetCount = 0;

    IDE_TEST( aSender->getTargetNamesFromItemMetaEntry( aTID,
                                                        &sTargetCount,
                                                        aTargetTableName,
                                                        aTargetPartNames )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sTargetCount == 0, ERR_NO_TARGET );

    IDE_TEST( aSender->getDDLInfoFromDDLStmtLog( aTID,
                                                 aMaxDDLStmtLen,
                                                 aUserName,
                                                 aDDLStmt )
              != IDE_SUCCESS );

    *aTargetCount = sTargetCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_TARGET );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL asynchronization target not found." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::ddlASynchronizationInternal( rpxReceiver * aReceiver )
{
    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization start" );

    IDE_TEST( ddlASyncStartInternal( aReceiver ) != IDE_SUCCESS );

    IDE_TEST( executeDDLASyncInternal( aReceiver ) != IDE_SUCCESS );

    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization success" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization failure : %s",
                 ideGetErrorMsg() );

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::ddlASyncStartInternal( rpxReceiver * aReceiver )
{
    return aReceiver->sendDDLASyncStartAck( RP_X_DDL_REPLICATE_HANDSHAKE_ACK, 
                                            RPU_REPLICATION_SENDER_SEND_TIMEOUT );

}

IDE_RC rpcDDLASyncManager::executeDDLASyncInternal( rpxReceiver * aReceiver )
{
    rpdVersion   sVersion;
    rpXLogType   sType              = RP_X_NONE;
    UInt         sErrCode           = 0;
    SChar      * sErrMsg            = NULL;
    SChar      * sDDLStmt           = NULL;
    idBool       sDoSendAck         = ID_FALSE;
    smSN         sRemoteDDLCommitSN = SM_SN_NULL;
    UInt         sDDLEnableLevel    = 0;
    UInt         sTargetCount       = 0;
    SChar      * sTargetPartNames   = NULL;
    SChar        sUserName[SMI_MAX_NAME_LEN + 1];
    SChar        sTargetTableName[QC_MAX_OBJECT_NAME_LEN + 1];

    idlOS::memset( sUserName, 0, SMI_MAX_NAME_LEN + 1 );
    idlOS::memset( sTargetTableName, 0, QC_MAX_OBJECT_NAME_LEN + 1 );

    IDE_TEST( aReceiver->recvDDLASyncExecute( (UInt*)&sType,
                                              sUserName,
                                              &sDDLEnableLevel,
                                              &sTargetCount,
                                              sTargetTableName,
                                              &sTargetPartNames,
                                              &sRemoteDDLCommitSN,
                                              &sVersion,
                                              &sDDLStmt,
                                              RPU_REPLICATION_RECEIVE_TIMEOUT )
              != IDE_SUCCESS );
    sDoSendAck = ID_TRUE;
    IDE_TEST_RAISE( sType != RP_X_DDL_REPLICATE_EXECUTE, ERR_INVALID_XLOG );

    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL asynchronization receive DDL <%s>", sDDLStmt );

    IDE_TEST( checkDDLASyncAvailable( aReceiver,
                                      sUserName,
                                      sDDLEnableLevel,
                                      sTargetCount,
                                      sTargetTableName,
                                      sTargetPartNames,
                                      sDDLStmt,
                                      &sVersion )
              != IDE_SUCCESS );

    if ( aReceiver->isAlreadyAppliedDDL( sRemoteDDLCommitSN ) != ID_TRUE )
    {
        ideLog::log( IDE_RP_0, "[DDLASyncManager] Exectue DDL <%s>", sDDLStmt );
        IDE_TEST( runDDLNMetaRebuild( aReceiver,
                                      sDDLStmt,
                                      sUserName,
                                      sRemoteDDLCommitSN,
                                      &sDoSendAck )
                  != IDE_SUCCESS );
    }
    else
    {
        ideLog::log( IDE_RP_0, "DDL <%s> is already applied", sDDLStmt );
    }

    sDoSendAck = ID_FALSE;
    IDE_TEST( aReceiver->sendDDLASyncExecuteAck( (UInt)RP_X_DDL_REPLICATE_EXECUTE_ACK,
                                                 (UInt)ID_TRUE,
                                                 0,
                                                 NULL,
                                                 RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    (void)iduMemMgr::free( sTargetPartNames );
    sTargetPartNames = NULL;

    (void)iduMemMgr::free( sDDLStmt );
    sDDLStmt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_XLOG );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Wrong xlog type" ) );
    }
    IDE_EXCEPTION_END;

    sErrMsg = ideGetErrorMsg();
    sErrCode = ideGetErrorCode();

    IDE_PUSH();

    if ( sTargetPartNames != NULL )
    {
        (void)iduMemMgr::free( sTargetPartNames );
        sTargetPartNames = NULL;
    }

    if ( sDDLStmt != NULL )
    {
        (void)iduMemMgr::free( sDDLStmt );
        sDDLStmt = NULL;
    }

    if ( sDoSendAck == ID_TRUE )
    {
        sDoSendAck = ID_FALSE;
        (void)aReceiver->sendDDLASyncExecuteAck( RP_X_DDL_REPLICATE_EXECUTE_ACK,
                                                 (UInt)ID_FALSE,
                                                 sErrCode,
                                                 sErrMsg,
                                                 RPU_REPLICATION_SENDER_SEND_TIMEOUT );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::checkDDLASyncAvailable( rpxReceiver * aReceiver,
                                                   SChar       * aUserName,
                                                   UInt          aDDLEnableLevel,
                                                   UInt          aTargetCount,
                                                   SChar       * aTargetTableName,
                                                   SChar       * aTargetPartNames,
                                                   SChar       * aDDLStmt,
                                                   rpdVersion  * aVersion )
{
    printTargetInfo( aTargetCount,
                     aUserName,
                     aTargetTableName,
                     aTargetPartNames );

    IDE_TEST_RAISE( RPU_REPLICATION_DDL_ENABLE == 0, ERR_REPLICATION_DDL_DISABLED );
    IDE_TEST_RAISE( aDDLEnableLevel > RPU_REPLICATION_DDL_ENABLE_LEVEL, ERR_DDL_ENABLE_LEVEL );

    IDE_TEST( aReceiver->checkLocalAndRemoteNames( aUserName,
                                                   aTargetCount,
                                                   aTargetTableName,
                                                   aTargetPartNames )
              != IDE_SUCCESS );

    IDE_TEST( rpcDDLSyncManager::checkDDLSyncRemoteVersion( aVersion ) != IDE_SUCCESS );

    IDE_TEST_RAISE( idlOS::strlen( aDDLStmt ) > RP_DDL_SQL_BUFFER_MAX + 1, ERR_SQL_LENGTH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DDL_ENABLE_LEVEL );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, 
                                  "Cannot execute this DDL on a replicated table "
                                  "when the remote property REPLICATION_DDL_ENABLE_LEVEL is higher then local property" ) );
    }
    IDE_EXCEPTION( ERR_SQL_LENGTH );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_SQL_LENGTH_ERROR, RP_DDL_SQL_BUFFER_MAX ) );
    }
    IDE_EXCEPTION( ERR_REPLICATION_DDL_DISABLED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPLICATION_DDL_DISABLED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::runDDLNMetaRebuild( rpxReceiver * aReceiver,
                                               SChar       * aDDLStmt,
                                               SChar       * aUserName,
                                               smSN          aLastRemoteDDLXSN,
                                               idBool      * aDoSendAck )
{
    smiTrans       sDDLTrans;
    smiStatement * sRootStmt = NULL;
    UInt           sStage    = 0;
    idBool         sAfterRun = ID_FALSE;

    smiStatement   sStatement;

    IDE_TEST( sDDLTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT_RAISE( "rpcDDLASyncManager::runDDLNMetaRebuild::begin::sDDLTrans",
                         ERR_TRANS_BEGIN );
    IDE_TEST_RAISE( sDDLTrans.begin( &sRootStmt,
                                     aReceiver->getStatistics(),
                                     ( SMI_ISOLATION_NO_PHANTOM     |
                                       SMI_TRANSACTION_NORMAL       |
                                       SMI_TRANSACTION_REPL_DEFAULT |
                                       SMI_COMMIT_WRITE_WAIT ),
                                     aReceiver->mReplID )
                    != IDE_SUCCESS, ERR_TRANS_BEGIN );
    sStage = 2;

    aReceiver->setSelfExecuteDDLTransID( sDDLTrans.getTransID() );
    
    IDE_TEST( sDDLTrans.setReplTransLockTimeout( RPU_REPLICATION_DDL_SYNC_TIMEOUT ) != IDE_SUCCESS );

    IDE_TEST( updateRemoteLastDDLXSN( &sDDLTrans, 
                                      aReceiver->getRepName(),
                                      aLastRemoteDDLXSN ) 
              != IDE_SUCCESS );

    IDE_TEST( sStatement.begin( sDDLTrans.getStatistics(),
                                sDDLTrans.getStatement(),
                                SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );

    sStage = 3;
    IDE_TEST ( rpcDDLSyncManager::runDDL( sDDLTrans.getStatistics(),
                                          aDDLStmt,
                                          aUserName,
                                          &sStatement )
               != IDE_SUCCESS );
    sAfterRun = ID_TRUE;
    
    IDE_TEST( sStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( aReceiver->metaRebuild( &sDDLTrans ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sDDLTrans.commit() != IDE_SUCCESS, ERR_TRANS_COMMIT );
    sStage = 1;
    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL <%s> commit success", aDDLStmt );

    aReceiver->setSelfExecuteDDLTransID( SM_NULL_TID );

    sStage = 0;
    IDE_TEST( sDDLTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_BEGIN );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLASyncManager] Transaction begin failure") );
    }
    IDE_EXCEPTION( ERR_TRANS_COMMIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLASyncManager] Transaction commit failure") );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    aReceiver->setSelfExecuteDDLTransID( SM_NULL_TID );

    switch(sStage)
    {
        case 3:
            (void)sStatement.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            if ( sAfterRun != ID_TRUE )
            {
                IDE_ASSERT(sDDLTrans.rollback() == IDE_SUCCESS);
            }
            else
            {
                /* Meta Rebuild 실패시에는 DDL 은 Commit 하고 Receiver 를 내려 Meta 를 최신으로 유지 */
                if ( sDDLTrans.commit() == IDE_SUCCESS )
                {
                    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL <%s> commit success", aDDLStmt );
                    *aDoSendAck = ID_FALSE;
                }
                else
                {
                    ideLog::log( IDE_RP_0, "[DDLASyncManager] DDL <%s> commit failure", aDDLStmt );

                    IDE_ASSERT( sDDLTrans.rollback() == IDE_SUCCESS );
                }
            }
            /* fall through */
        case 1:
            (void)sDDLTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLASyncManager::updateRemoteLastDDLXSN( smiTrans * aTrans, 
                                                   SChar    * aRepName,
                                                   smSN       aSN )
{
    smiStatement   sStatement;
    idBool         sIsStmtBegin = ID_FALSE;

    IDE_TEST( sStatement.begin( aTrans->getStatistics(),
                                aTrans->getStatement(),
                                SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sIsStmtBegin = ID_TRUE;

    IDE_TEST( rpdCatalog::updateRemoteLastDDLXSN( &sStatement,
                                                  aRepName,
                                                  aSN )
              != IDE_SUCCESS );

    IDE_TEST( sStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStmtBegin = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsStmtBegin == ID_TRUE )
    {
        sIsStmtBegin = ID_FALSE;
        (void)sStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    IDE_POP();

    return IDE_FAILURE;
}

idBool rpcDDLASyncManager::isRetryDDLError( UInt aErrCode )
{
    idBool sIsRetry = ID_FALSE;

    if ( ( ideIsRetry( aErrCode ) == IDE_SUCCESS ) ||
         ( ideIsRebuild( aErrCode ) == IDE_SUCCESS ) ||
         ( aErrCode == smERR_ABORT_smcExceedLockTimeWait ) ) 
    {
        sIsRetry = ID_TRUE;
    }

    return sIsRetry;
}

void rpcDDLASyncManager::printTargetInfo( UInt    aTargetCount, 
                                          SChar * aUserName,
                                          SChar * aTargetTableName,
                                          SChar * aTargetPartNames )
{
    UInt i = 0;
    UInt sOffset = 0;
    SChar * sTargetPartName = NULL;

    ideLogEntry sLog( IDE_RP_0 );
    sLog.appendFormat( "[DDLASyncManager] DDL asynchronization Target Info\n"
                       "==================================================\n"
                       "User : %s\nTarget Table : %s\n",
                       aUserName,
                       aTargetTableName );

    for ( i = 0; i < aTargetCount; i++ )
    {
        sTargetPartName = aTargetPartNames + sOffset;

        sLog.appendFormat( "Target Partition[%u] : %s\n", i + 1, sTargetPartName );

        sOffset += QC_MAX_OBJECT_NAME_LEN + 1;
    }
    
    (void)sLog.write();
}
