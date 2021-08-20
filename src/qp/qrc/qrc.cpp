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
 * $Id: qrc.cpp 91331 2021-07-23 08:00:11Z minku.kang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qrc.h>
#include <qdpPrivilege.h>
#include <qdpRole.h>
#include <rp.h>

IDE_RC qrc::validateFailover( qcStatement * aStatement )
{
    return qci::mValidateReplicationCallback.mValidateFailover( aStatement ); 
}
    

/***********************************************************************
 * VALIDATE
 **********************************************************************/

IDE_RC qrc::validateCreate(qcStatement * aStatement)
{
        return qci::mValidateReplicationCallback.mValidateCreate( aStatement );
}

IDE_RC qrc::validateOneReplItem(qcStatement  * aStatement,
                                qriReplItem  * aReplItem,
                                SInt           aRole,
                                idBool         aIsRecoveryOpt,
                                SInt           aReplMode)
{
        return qci::mValidateReplicationCallback.mValidateOneReplItem( aStatement,
                                                                       NULL,
                                                                       aReplItem,
                                                                       aRole,
                                                                       ID_FALSE,
                                                                       aIsRecoveryOpt,
                                                                       aReplMode );
}

IDE_RC qrc::validateAlterAddTbl(qcStatement * aStatement)
{
        return qci::mValidateReplicationCallback.mValidateAlterAddTbl( aStatement );
}


IDE_RC qrc::validateAlterDropTbl(qcStatement * aStatement)
{
        return qci::mValidateReplicationCallback.mValidateAlterDropTbl( aStatement );
}

IDE_RC qrc::validateAlterAddHost(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterAddHost( aStatement );
}

IDE_RC qrc::validateAlterDropHost(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterDropHost( aStatement );
}

IDE_RC qrc::validateAlterSetHost(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetHost( aStatement );
}

IDE_RC qrc::validateAlterSetMode(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetMode( aStatement );
}

IDE_RC qrc::validateDrop(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateDrop( aStatement );
}

IDE_RC qrc::validateStart(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateStart( aStatement );
}

/* PROJ-1915 
 * SYS_REPL_OFFLINE_DIR_ 가 있는지 조회 한다.
 */
IDE_RC qrc::validateOfflineStart(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateOfflineStart ( aStatement );
}

IDE_RC qrc::validateQuickStart(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateQuickStart( aStatement );
}

IDE_RC qrc::validateSync(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateSync( aStatement );
}

IDE_RC qrc::validateSyncTbl(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateSyncTbl( aStatement );
}

IDE_RC qrc::validateTempSync(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateTempSync( aStatement );
}

IDE_RC qrc::validateReset(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateReset( aStatement );
}

IDE_RC qrc::validateDeleteItemReplaceHistory(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateDeleteItemReplaceHistory( aStatement );
}

IDE_RC qrc::validateFailback(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateFailback( aStatement );
}
/***********************************************************************
 * EXECUTE
 **********************************************************************/

IDE_RC qrc::executeCreate(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteCreate( aStatement );
}

IDE_RC qrc::executeAlterAddTbl(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterAddTbl( aStatement );
}

IDE_RC qrc::executeAlterDropTbl(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterDropTbl( aStatement );
}

IDE_RC qrc::executeAlterAddHost(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterAddHost( aStatement );
}

IDE_RC qrc::executeAlterDropHost(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterDropHost( aStatement );
}

IDE_RC qrc::executeAlterSetHost(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetHost( aStatement );
}

IDE_RC qrc::executeAlterSetMode(qcStatement * aStatement )
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetMode( aStatement );
}

IDE_RC qrc::executeDrop(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteDrop( aStatement );
}

IDE_RC qrc::executeStart(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );
    
    IDE_TEST(qci::mExecuteReplicationCallback.mExecuteStart( aStatement ) != IDE_SUCCESS);
    
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    IDE_PUSH();
    switch ( sState )
    {
    case 3:
        qcg::setSmiStmt( aStatement, sSmiStmtOrg );
        
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    case 2:
        (void)sSmiTrans.rollback();
    case 1:
        (void)sSmiTrans.destroy( aStatement->mStatistics );
    case 0:
    default:
        break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qrc::executeQuickStart(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );

    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteQuickStart( aStatement ) != IDE_SUCCESS );
    
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    IDE_PUSH();
    switch ( sState )
    {
    case 3:
        qcg::setSmiStmt( aStatement, sSmiStmtOrg );
        
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    case 2:
        (void)sSmiTrans.rollback();
    case 1:
        (void)sSmiTrans.destroy( aStatement->mStatistics );
    case 0:
    default:
        break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qrc::executeSync(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );
    
    
    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteSync( aStatement ) != IDE_SUCCESS );
    
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    IDE_PUSH();
    switch ( sState )
    {
    case 3:
        qcg::setSmiStmt( aStatement, sSmiStmtOrg );
        
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    case 2:
        (void)sSmiTrans.rollback();
    case 1:
        (void)sSmiTrans.destroy( aStatement->mStatistics );
    case 0:
    default:
        break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qrc::executeTempSync(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );
    
    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteTempSync( aStatement ) != IDE_SUCCESS );
    
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;
    
    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    IDE_PUSH();
    switch ( sState )
    {
    case 3:
        qcg::setSmiStmt( aStatement, sSmiStmtOrg );
        
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    case 2:
        (void)sSmiTrans.rollback();
    case 1:
        (void)sSmiTrans.destroy( aStatement->mStatistics );
    case 0:
    default:
        break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qrc::executeSyncCondition(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );
    
    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteSyncCondition( aStatement ) != IDE_SUCCESS );
    
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    IDE_PUSH();
    switch ( sState )
    {
    case 3:
        qcg::setSmiStmt( aStatement, sSmiStmtOrg );
        
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    case 2:
        (void)sSmiTrans.rollback();
    case 1:
        (void)sSmiTrans.destroy( aStatement->mStatistics );
    case 0:
    default:
        break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qrc::executeStop(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    idBool            sIsExist      = ID_FALSE;
    SChar             sReplName[QC_MAX_OBJECT_NAME_LEN + 1] = { '\0', };

    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;

    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );

    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;

    // TASK-2401 Disk/Memory Log분리
    //           LFG=2일때 Trans Commit시 로그 플러쉬 하도록 설정
    IDE_TEST( sSmiTrans.setMetaTableModified() != IDE_SUCCESS );

    IDE_TEST( qdpRole::checkDDLReplicationPriv( aStatement ) != IDE_SUCCESS );

    IDE_TEST( qci::mCatalogReplicationCallback.mCheckReplicationExistByName( aStatement,
                                                                             sParseTree->replName,
                                                                             &sIsExist )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsExist != ID_TRUE, ERR_NOT_EXIST_REPLICATION );

    QC_STR_COPY( sReplName, sParseTree->replName );

    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteStop( aStatement,
                                                             sReplName )
              != IDE_SUCCESS );

    qcg::setSmiStmt( aStatement, sSmiStmtOrg );

    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_REPLICATION )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPC_NOT_EXIST_REPLICATION ) );
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();
    switch ( sState )
    {
        case 3:
            qcg::setSmiStmt( aStatement, sSmiStmtOrg );

            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void)sSmiTrans.rollback();
        case 1:
            (void)sSmiTrans.destroy( aStatement->mStatistics );
        case 0:
        default:
            break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qrc::executeFailover( qcStatement * aStatement )
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;

    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );

    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteFailover( aStatement ) 
              != IDE_SUCCESS );

    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
    case 3:
        qcg::setSmiStmt( aStatement, sSmiStmtOrg );
        
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    case 2:
        (void)sSmiTrans.rollback();
    case 1:
        (void)sSmiTrans.destroy( aStatement->mStatistics );
    case 0:
    default:
        break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC qrc::executeReset(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteReset( aStatement );
}

IDE_RC qrc::executeFlush( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    To Fix PR-10590
 *    ALTER REPLICATION name FLUSH 에 대한 Execution
 *
 * Implementation :
 *
 ***********************************************************************/

    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    idBool            sIsExist      = ID_FALSE;
    idBool            sIsConsistent = ID_FALSE;
    SChar             sReplName[QC_MAX_OBJECT_NAME_LEN + 1] = { '\0', };

    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;

    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT,
                               SMX_NOT_REPL_TX_ID - 1 )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );

    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;

    // TASK-2401 Disk/Memory Log분리
    //           LFG=2일때 Trans Commit시 로그 플러쉬 하도록 설정
    IDE_TEST( sSmiTrans.setMetaTableModified() != IDE_SUCCESS );

    IDE_TEST( qdpRole::checkDDLReplicationPriv( aStatement ) != IDE_SUCCESS );

    IDE_TEST( qci::mCatalogReplicationCallback.mCheckReplicationExistByName( aStatement,
                                                                             sParseTree->replName,
                                                                             &sIsExist )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsExist != ID_TRUE, ERR_NOT_EXIST_REPLICATION );

    if ( sParseTree->flushOption.flushType == RP_FLUSH_XLOGFILE )
    {
        IDE_TEST( qci::mCatalogReplicationCallback.mIsConsistentReplication( aStatement,
                                                                             sParseTree->replName,
                                                                             &sIsConsistent )
                != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsConsistent != ID_TRUE, ERR_IS_NOT_CONSISTENT_REPLICATION );

    }

    QC_STR_COPY( sReplName, sParseTree->replName );

    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteFlush( sDummySmiStmt,
                                                              sReplName,
                                                              &(sParseTree->flushOption),
                                                              aStatement->mStatistics )
              != IDE_SUCCESS );

    qcg::setSmiStmt( aStatement, sSmiStmtOrg );

    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_REPLICATION )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPC_NOT_EXIST_REPLICATION ) );
    }
    IDE_EXCEPTION( ERR_IS_NOT_CONSISTENT_REPLICATION )
    {
        //R2HA
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                        "Flushing xlogfiles can be execute in consistent replication mode" ) );
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();
    switch ( sState )
    {
        case 3:
            qcg::setSmiStmt( aStatement, sSmiStmtOrg );

            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void)sSmiTrans.rollback();
        case 1:
            (void)sSmiTrans.destroy( aStatement->mStatistics );
        case 0:
        default:
            break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qrc::executeAlterSetRecovery(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetRecovery( aStatement );
}

/* PROJ-1915 */
IDE_RC qrc::executeAlterSetOfflineEnable(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetOfflineEnable( aStatement );
}

IDE_RC qrc::executeAlterSetOfflineDisable(qcStatement * aStatement)
{

    return qci::mExecuteReplicationCallback.mExecuteAlterSetOfflineDisable( aStatement );
}

idBool qrc::isValidIPFormat(SChar * aIP)
{
    return qci::mValidateReplicationCallback.mIsValidIPFormat( aIP );
}

//PROJ-1608 recovery from replication
IDE_RC qrc::validateAlterSetRecovery(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetRecovery( aStatement );
}

/* PROJ-1915 off-line replicator
 * ALTER REPLICATION replicatoin_name SET OFFLINE ENABLE WITH 경로 ... 경로 ...
 */
IDE_RC qrc::validateAlterSetOffline(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetOffline( aStatement );
}
/* PROJ-1969 */
IDE_RC qrc::validateAlterSetGapless(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetGapless( aStatement );
}

IDE_RC qrc::executeAlterSetGapless(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetGapless( aStatement );
}

IDE_RC qrc::validateAlterSetParallel(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetParallel( aStatement );
}

IDE_RC qrc::executeAlterSetParallel(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetParallel( aStatement );
}

IDE_RC qrc::validateAlterSetGrouping(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetGrouping( aStatement );
}

IDE_RC qrc::executeAlterSetGrouping(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetGrouping( aStatement );
}

IDE_RC qrc::validateAlterSetDDLReplicate(qcStatement * aStatement)
{
    return qci::mValidateReplicationCallback.mValidateAlterSetDDLReplicate( aStatement );
}

IDE_RC qrc::executeAlterSetDDLReplicate(qcStatement * aStatement)
{
    return qci::mExecuteReplicationCallback.mExecuteAlterSetDDLReplicate( aStatement );
}

IDE_RC qrc::executeDeleteItemReplaceHistory(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );
   
    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteDeleteItemReplaceHistory( aStatement ) != IDE_SUCCESS);
    
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch ( sState )
    {
        case 3:
            qcg::setSmiStmt( aStatement, sSmiStmtOrg );
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void)sSmiTrans.rollback();
        case 1:
            (void)sSmiTrans.destroy( aStatement->mStatistics );
        case 0:
        default:
            break;
    }
    IDE_POP();
   
    return IDE_FAILURE;
}

IDE_RC qrc::executeFailback(qcStatement * aStatement)
{
    qriParseTree    * sParseTree    = (qriParseTree *)QC_PARSETREE(aStatement);
    
    smiTrans          sSmiTrans;
    smiStatement    * sDummySmiStmt = NULL;
    smiStatement    * sSmiStmtOrg   = NULL;
    smiStatement      sSmiStmt;
    SInt              sState        = 0;
    
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aStatement->mStatistics,
                               SMI_ISOLATION_CONSISTENT |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_WAIT )
              != IDE_SUCCESS );
    sState = 2;
    
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sDummySmiStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    
    qcg::getSmiStmt( aStatement, &sSmiStmtOrg );
    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 3;
    
    IDE_TEST( sParseTree->common.validate( aStatement ) != IDE_SUCCESS );
   
    IDE_TEST( qci::mExecuteReplicationCallback.mExecuteFailback( aStatement ) != IDE_SUCCESS);
    
    qcg::setSmiStmt( aStatement, sSmiStmtOrg );
    
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch ( sState )
    {
        case 3:
            qcg::setSmiStmt( aStatement, sSmiStmtOrg );
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void)sSmiTrans.rollback();
        case 1:
            (void)sSmiTrans.destroy( aStatement->mStatistics );
        case 0:
        default:
            break;
    }
    IDE_POP();
   
    return IDE_FAILURE;
}
/* PROJ-2677 DDL Synchronization */
/* PROJ-2735 DDL Transaction */
void qrc::setDDLSrcInfo( qcStatement * aQcStatement,
                         idBool        aTransactionalDDLAvailable,
                         UInt          aSrcTableOIDCount,
                         smOID       * aSrcTableOIDArray,
                         UInt          aSrcPartOIDCountPerTable,
                         smOID       * aSrcPartOIDArray )
{
    aQcStatement->mDDLInfo.mTransactionalDDLAvailable = aTransactionalDDLAvailable;
    aQcStatement->mDDLInfo.mSrcTableOIDCount          = aSrcTableOIDCount;
    aQcStatement->mDDLInfo.mSrcTableOIDArray          = aSrcTableOIDArray;
    aQcStatement->mDDLInfo.mSrcPartOIDCountPerTable   = aSrcPartOIDCountPerTable;
    aQcStatement->mDDLInfo.mSrcPartOIDArray           = aSrcPartOIDArray;
}

void qrc::setDDLDestInfo( qcStatement * aQcStatement, 
                          UInt          aDestTableOIDCount,
                          smOID       * aDestTableOIDArray,
                          UInt          aDestPartOIDCountPerTable,
                          smOID       * aDestPartOIDArray)
{
    aQcStatement->mDDLInfo.mDestTableOIDCount        = aDestTableOIDCount;
    aQcStatement->mDDLInfo.mDestTableOIDArray        = aDestTableOIDArray;
    aQcStatement->mDDLInfo.mDestPartOIDCountPerTable = aDestPartOIDCountPerTable;
    aQcStatement->mDDLInfo.mDestPartOIDArray         = aDestPartOIDArray;

}

idBool qrc::isDDLSync( qcStatement * aQcStatement )
{
    idBool sResult = ID_FALSE;

    if ( ( QCG_GET_SESSION_REPLICATION_DDL_SYNC( aQcStatement ) == 1 ) ||
         ( ( aQcStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_DDL_SYNC_MASK )
           == QC_SESSION_INTERNAL_DDL_SYNC_TRUE) )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

idBool qrc::isInternalDDL( qcStatement * aQcStatement )
{
    idBool sResult = ID_FALSE;

    if ( ( ( aQcStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_DDL_SYNC_MASK )
         == QC_SESSION_INTERNAL_DDL_SYNC_TRUE ) ||
         ( ( aQcStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_DDL_MASK )
           == QC_SESSION_INTERNAL_DDL_TRUE ) )
    {
        sResult = ID_TRUE;
    }
    
    return sResult;
}

idBool qrc::isLockTableUntillNextDDL( qcStatement * aQcStatement )
{
    idBool sResult = ID_FALSE;

    if ( QCG_GET_SESSION_LOCK_TABLE_UNTIL_NEXT_DDL( aQcStatement ) == ID_TRUE )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}
