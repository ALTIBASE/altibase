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

#include <qci.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcTask.h>
#include <mmtAdminManager.h>
#include <mmuProperty.h>
#include <mmuOS.h>
#include <rpi.h>

mmcStmtExecFunc mmcStatement::mExecuteFunc[] =
{
    mmcStatement::executeDDL,
    mmcStatement::executeDML,
    mmcStatement::executeDCL,
    NULL,
    mmcStatement::executeSP,
    mmcStatement::executeDB,
};


IDE_RC mmcStatement::executeDDL(mmcStatement *aStmt, SLong * /*aAffectedRowCount*/, SLong * /*aFetchedRowCount*/)
{
    mmcSession   * sSession   = aStmt->getSession();
    mmcTransObj  * sTrans     = NULL;
    idBool         sIsDDLStmtBegin = ID_TRUE;
    UInt           i          = 0;
    smiTrans     * sSmiTrans  = NULL;
    qciStatement * sQciStatement = aStmt->getQciStmt();
    UInt           sSrcTableOIDCount        = 0;
    smOID        * sSrcTableOIDArray        = qciMisc::getDDLSrcTableOIDArray( sQciStatement, 
                                                                               &sSrcTableOIDCount );    
    UInt           sSrcPartOIDCount         = 0;
    smOID        * sSrcPartOIDArray         = qciMisc::getDDLSrcPartTableOIDArray( sQciStatement, 
                                                                                   &sSrcPartOIDCount );

    ideLog::log(IDE_QP_2, QP_TRC_EXEC_DDL_BEGIN, aStmt->getQueryString());
    sTrans = sSession->allocTrans(aStmt);
    
    IDE_TEST(aStmt->endSmiStmt(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sIsDDLStmtBegin = ID_FALSE;

    if (( sdi::hasShardCoordPlan(
              qci::getSelectStmtOfDDL( &((aStmt->getQciStmt())->statement) ) ) == ID_TRUE ) ||
        (( sSession->getQciSession()->mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) ==
         QC_SESSION_SHARD_DDL_TRUE ))
    {
        aStmt->mInfo.mFlag |= MMC_STMT_NEED_UNLOCK_TRUE;
    }

    if ( sSession->globalDDLUserSession() != ID_TRUE )
    {
        /* PROJ-2735 DDL Transaction */
        if ( sSession->isDDLAutoCommit() != ID_TRUE )
        {
            sSmiTrans = &(sTrans->mSmiTrans);

            if ( sSrcTableOIDCount > 0 )
            {
                for ( i = 0; i < sSrcTableOIDCount; i++ )
                {
                    IDE_TEST( qciMisc::checkRollbackAbleDDLEnable( sSmiTrans, 
                                                                   sSrcTableOIDArray[i],
                                                                   ID_FALSE ) 
                              != IDE_SUCCESS );
                    
                    IDE_TEST( sSmiTrans->setExpSvpForBackupDDLTargetTableInfo( sSrcTableOIDArray[i], 
                                                                               sSrcPartOIDCount,
                                                                               sSrcPartOIDArray + 
                                                                               (i * sSrcPartOIDCount),
                                                                               SM_OID_NULL,
                                                                               0,
                                                                               NULL )
                              != IDE_SUCCESS );
                }

                IDE_TEST( qciMisc::rebuildStatement( sQciStatement,
                                                     sSmiTrans,
                                                     aStmt->getCursorFlag() |
                                                     SMI_STATEMENT_NORMAL )
                          != IDE_SUCCESS );
            }
        }

        /* BUG-45371 */
        IDU_FIT_POINT( "mmcStatement::executeDDL::beginSmiStmt" );
        IDE_TEST( aStmt->beginSmiStmt( sTrans, SMI_STATEMENT_NORMAL ) != IDE_SUCCESS );
        sIsDDLStmtBegin = ID_TRUE;

        IDE_TEST( executeLocalDDL( aStmt ) != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2736 Global DDL */
        IDE_TEST( executeGlobalDDL( aStmt ) != IDE_SUCCESS );

        IDE_TEST( aStmt->beginSmiStmt( sTrans, SMI_STATEMENT_NORMAL ) != IDE_SUCCESS ); 
        sIsDDLStmtBegin = ID_TRUE;
    }

    ideLog::log(IDE_QP_2, QP_TRC_EXEC_DDL_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsDDLStmtBegin != ID_TRUE )
    {
        IDE_ASSERT( aStmt->beginSmiStmt( sTrans, SMI_STATEMENT_NORMAL ) == IDE_SUCCESS );        
        sIsDDLStmtBegin = ID_TRUE; 
    }

	IDE_POP();
	IDE_ERRLOG(IDE_QP_0);
    /* bug-37143: unreadable error msg of DDL failure */
    ideLog::log(IDE_QP_2, QP_TRC_EXEC_DDL_FAILURE,
                E_ERROR_CODE(ideGetErrorCode()),
                ideGetErrorMsg(ideGetErrorCode()));

    return IDE_FAILURE;
}

IDE_RC mmcStatement::executeDML(mmcStatement *aStmt, SLong *aAffectedRowCount, SLong *aFetchedRowCount)
{
    mmcSession   *sSession   = aStmt->getSession();
    qciStatement *sStatement = aStmt->getQciStmt();
    qciStmtType   sStmtType  = aStmt->getStmtType();

    switch (sStmtType)
    {
        case QCI_STMT_LOCK_TABLE:
            if (sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT)
            {
                IDE_RAISE(AutocommitError);
            }
            else
            {
                // Nothing to do.
            }
            
        default:
            IDE_TEST(qci::execute(sStatement, aStmt->getSmiStmt()) != IDE_SUCCESS);

            qci::getRowCount(aStmt->getQciStmt(), aAffectedRowCount, aFetchedRowCount);

            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(AutocommitError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_MMC_CANT_LOCK_TABLE_IN_AUTOCOMMIT_MODE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::executeDCL(mmcStatement *aStmt, SLong * /*aAffectedRowCount*/, SLong * /*aFetchedRowCount*/)
{
    mmcSession  *sSession      = aStmt->getSession();
    qciStmtType  sStmtType     = aStmt->getStmtType();
    mmmPhase     sStartupPhase = mmm::getCurrentPhase();
    idBool       sIsDDLLogging = ID_FALSE;
    mmcTransObj *sTrans        = NULL;
    smiTrans    *sSmiTrans     = NULL;

    sdiSessionType sShardSessionType = sSession->getShardSessionType();

    switch (sStmtType)
    {
        case QCI_STMT_SET_PLAN_DISPLAY_ON:
        case QCI_STMT_SET_PLAN_DISPLAY_OFF:
        case QCI_STMT_SET_PLAN_DISPLAY_ONLY:
        case QCI_STMT_SET_AUTOCOMMIT_TRUE:
        case QCI_STMT_SET_AUTOCOMMIT_FALSE:
            IDE_RAISE(InvalidSessionProperty);
            break;

        case QCI_STMT_SET_SESSION_PROPERTY:
            IDE_TEST_RAISE(sStartupPhase < MMM_STARTUP_PROCESS, InvalidServerPhaseError);
            /* PROJ-2733 DBMS_SHARD.EXECUTE_IMMEDIATE()로 ALTER SESSION은 허용하지 않는다. */
            IDE_TEST_RAISE(((sShardSessionType == SDI_SESSION_TYPE_COORD) ||
                            (sShardSessionType == SDI_SESSION_TYPE_LIB)) &&  /* BUG-47891 */
                           (mmcSession::getShardInternalLocalOperation(sSession) == SDI_INTERNAL_OP_NOT),
                           AlterSessionNotAllowError);
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            break;

        case QCI_STMT_SET_SYSTEM_PROPERTY:
            IDE_TEST_RAISE(sStartupPhase < MMM_STARTUP_PROCESS, InvalidServerPhaseError);
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            break;

        case QCI_STMT_ALT_TABLESPACE_DISCARD:
        case QCI_STMT_ALT_TABLESPACE_CHKPT_PATH:
        case QCI_STMT_ALT_DATAFILE_ONOFF:
        case QCI_STMT_ALT_RENAME_DATAFILE:
            IDE_TEST_RAISE(sStartupPhase != MMM_STARTUP_CONTROL, InvalidServerPhaseError);
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            break;

        /* BUG-21230 */
        case QCI_STMT_COMMIT:
        case QCI_STMT_ROLLBACK:
        case QCI_STMT_ROLLBACK_TO_SAVEPOINT:  /* BUG-48216 */
        case QCI_STMT_SET_TX:
            IDE_TEST_RAISE(sStartupPhase < MMM_STARTUP_META, InvalidServerPhaseError);
            IDE_TEST_RAISE(sSession->getXaAssocState() !=
                           MMD_XA_ASSOC_STATE_NOTASSOCIATED, DCLNotAllowedError); 
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            break;
            
        case QCI_STMT_SAVEPOINT:
        case QCI_STMT_SET_STACK:
        case QCI_STMT_SET:
        case QCI_STMT_ALT_SYS_CHKPT:
        case QCI_STMT_ALT_SYS_VERIFY:
        case QCI_STMT_ALT_SYS_MEMORY_COMPACT:
        case QCI_STMT_ALT_SYS_ARCHIVELOG:
            IDE_TEST_RAISE(sStartupPhase < MMM_STARTUP_META, InvalidServerPhaseError);
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            break;

        case QCI_STMT_SET_REPLICATION_MODE:
            IDE_TEST_RAISE(sdi::isShardEnable() == ID_TRUE, AlterRepNotSupportInSharding);  /* BUG-47891 */
            IDE_TEST_RAISE(sStartupPhase < MMM_STARTUP_META, InvalidServerPhaseError);
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            break;

        case QCI_STMT_ALT_TABLESPACE_BACKUP:
        case QCI_STMT_ALT_SYS_SWITCH_LOGFILE:
        case QCI_STMT_ALT_SYS_FLUSH_BUFFER_POOL:
        case QCI_STMT_ALT_SYS_COMPACT_PLAN_CACHE:
        case QCI_STMT_ALT_SYS_RESET_PLAN_CACHE:
        case QCI_STMT_ALT_SYS_SHRINK_MEMPOOL:
        case QCI_STMT_ALT_SYS_DUMP_CALLSTACKS:
        case QCI_STMT_CONTROL_DATABASE_LINKER:
        case QCI_STMT_CLOSE_DATABASE_LINK:
            IDE_TEST_RAISE(sStartupPhase != MMM_STARTUP_SERVICE, InvalidServerPhaseError);
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            break;

        case QCI_STMT_CONNECT:
        case QCI_STMT_DISCONNECT:
            IDE_RAISE(UnsupportedSQL);
            break;
        /* BUG-42639 Monitoring query */
        case QCI_STMT_SELECT_FOR_FIXED_TABLE:
            /* BUG-42639 Monitoring query
             * X$, V$ 만을 실행하는 Select Query는
             * Trans 를 할당할 필요가 없다
             */
            break;
        case QCI_STMT_ALT_REPLICATION_STOP :  /* BUG-42852 STOP과 FLUSH를 DCL로 변환합니다. */
        case QCI_STMT_ALT_REPLICATION_FLUSH : /* BUG-42852 STOP과 FLUSH를 DCL로 변환합니다. */
        case QCI_STMT_ALT_REPLICATION_START:
        case QCI_STMT_ALT_REPLICATION_QUICKSTART:
        case QCI_STMT_ALT_REPLICATION_SYNC:
        case QCI_STMT_ALT_REPLICATION_SYNC_CONDITION:
        case QCI_STMT_ALT_REPLICATION_TEMP_SYNC:
        case QCI_STMT_ALT_REPLICATION_FAILOVER:
            IDE_TEST_RAISE(sStartupPhase < MMM_STARTUP_META, InvalidServerPhaseError);
            sTrans = sSession->allocTrans();
            sSmiTrans = mmcTrans::getSmiTrans(sTrans);
            /* BUG-42915 STOP과 FLUSH의 로그를 DDL Logging Level로 기록합니다. */
            sIsDDLLogging = ID_TRUE;
            break;
        default:
            break;
    }

    if ( sIsDDLLogging == ID_TRUE )
    {
        ideLog::log( IDE_QP_2, QP_TRC_EXEC_DCL_BEGIN, aStmt->getQueryString() );
    }
    else
    {
        ideLog::log( IDE_QP_4, QP_TRC_EXEC_DCL_BEGIN, aStmt->getQueryString() );
    }

    IDE_TEST(qci::executeDCL(aStmt->getQciStmt(),
                             aStmt->getSmiStmt(),
                             sSmiTrans ) != IDE_SUCCESS);

    if ( sIsDDLLogging == ID_TRUE )
    {
        ideLog::log( IDE_QP_2, QP_TRC_EXEC_DCL_SUCCESS );
    }
    else
    {
        ideLog::log( IDE_QP_4, QP_TRC_EXEC_DCL_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidSessionProperty)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_SESSION_PROPERTY));
    }
    IDE_EXCEPTION(InvalidServerPhaseError)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SERVER_PHASE_MISMATCHES_QUERY_TYPE));
    }
    IDE_EXCEPTION(UnsupportedSQL)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, "Unsupported SQL"));
    }
    IDE_EXCEPTION(DCLNotAllowedError)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION(AlterSessionNotAllowError)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ALTER_SESSION_NOT_ALLOW));
    }
    IDE_EXCEPTION(AlterRepNotSupportInSharding)
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "ALTER SESSION REPLICATION restrictions",
                                  "" ) );
    }
    IDE_EXCEPTION_END;
    {
        if ( sIsDDLLogging == ID_TRUE )
        {
            ideLog::log( IDE_QP_2,
                         QP_TRC_EXEC_DCL_FAILURE,
                         E_ERROR_CODE( ideGetErrorCode() ),
                         ideGetErrorMsg( ideGetErrorCode() ) );
        }
        else
        {
            ideLog::log( IDE_QP_4,
                         QP_TRC_EXEC_DCL_FAILURE,
                         E_ERROR_CODE( ideGetErrorCode() ),
                         ideGetErrorMsg( ideGetErrorCode() ) );
        }
    }
    
    return IDE_FAILURE;
}

IDE_RC mmcStatement::executeSP(mmcStatement *aStmt, SLong * aAffectedRowCount, SLong *aFetchedRowCount)
{
    IDE_TEST(qci::execute(aStmt->getQciStmt(), aStmt->getSmiStmt()) != IDE_SUCCESS);

    qci::getRowCount(aStmt->getQciStmt(), aAffectedRowCount, aFetchedRowCount);

    /*  BUG-47148 
     *  [sharding] shard meta 가 변경되는 procedure 수행시 trc log 에 정보를 남겨야 합니다.  
     */
    if ( sdi::isShardMetaChange( aStmt->getQciStmt() ) == ID_TRUE )
    {
        sdi::printChangeMetaMessage( aStmt->getQueryString(), IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /*  BUG-47148 
     *  [sharding] shard meta 가 변경되는 procedure 수행시 trc log 에 정보를 남겨야 합니다.  
     */
    if ( sdi::isShardMetaChange( aStmt->getQciStmt() ) == ID_TRUE )
    {
        sdi::printChangeMetaMessage( aStmt->getQueryString(), IDE_FAILURE );
    }

    return IDE_FAILURE;
}

IDE_RC mmcStatement::executeDB(mmcStatement *aStmt, SLong * /*aAffectedRowCount*/, SLong * /*aFetchedRowCount*/)
{
    IDE_TEST(qci::execute(aStmt->getQciStmt(), aStmt->getSmiStmt()) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mmcStatement::executeLocalDDL( mmcStatement * aStmt )
{
    qciStatement * sStatement = aStmt->getQciStmt();
    idBool         sIsDDLSyncBegin = ID_FALSE;

    // TASK-2401 Disk/Memory Log분리
    //           LFG=2일때 Trans Commit시 로그 플러쉬 하도록 설정
    IDE_TEST( aStmt->getSmiStmt()->getTrans()->setMetaTableModified()
              != IDE_SUCCESS );

    /* PROJ-1442 Replication Online 중 DDL 허용
     * DDL Transaction 임을 기록하여, Replication Sender가 DML을 무시하도록 함
     */
    IDE_TEST(aStmt->getSmiStmt()->getTrans()->writeDDLLog() != IDE_SUCCESS);

    IDE_TEST( rpi::ddlSyncBegin( sStatement,
                                 aStmt->getSmiStmt() )
              != IDE_SUCCESS );
    sIsDDLSyncBegin = ID_TRUE;

    IDE_TEST(qci::execute(sStatement, aStmt->getSmiStmt()) != IDE_SUCCESS);

    sIsDDLSyncBegin = ID_FALSE;
    IDE_TEST( rpi::ddlSyncEnd( sStatement,
                               aStmt->getSmiStmt() )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsDDLSyncBegin == ID_TRUE )
    {
        sIsDDLSyncBegin = ID_FALSE;
        rpi::ddlSyncException( sStatement,
                               aStmt->getSmiStmt() );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC mmcStatement::executeGlobalDDL( mmcStatement * aStmt )
{
    UInt           sExecCount    = 0;
    mmcSession   * sSession      = aStmt->getSession();
    qciStatement * sQciStatement = aStmt->getQciStmt();
    qcStatement  * sQcStatement  = &(sQciStatement->statement);

    IDE_TEST_RAISE( sSession->getGlobalTransactionLevel() != 2, ERR_TRANS_LEVEL );

    IDE_TEST( sdi::checkShardLinker( sQcStatement ) != IDE_SUCCESS );

    /* IDE_TEST( sdiZookeeper::getShardMetaLock( sSmiTrans->getTransID() ) != IDE_SUCCESS ); */

    IDE_TEST( lockTableAllShardNodes( aStmt, SMI_TABLE_LOCK_X ) != IDE_SUCCESS );

    IDE_TEST( sdi::shardExecDirect( sQcStatement,
                                    NULL,
                                    QCI_STMTTEXT( sQciStatement ),
                                    QCI_STMTTEXTLEN( sQciStatement ),
                                    SDI_INTERNAL_OP_NOT,
                                    &sExecCount,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_LEVEL );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "Global DDL can only be executed when GLOBAL_TRANSACTION_LEVEL property is 2." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::lockTableAllShardNodes( mmcStatement * aStmt, smiTableLockMode aLockMode )
{
    SChar          sBuffer[QCI_MAX_NAME_LEN + QCI_MAX_OBJECT_NAME_LEN + 128];
    SChar          sLockMode[16];
    smSCN          sSCN;
    const void   * sTable      = NULL;
    qciTableInfo * sTableInfo  = NULL;
    smiTrans     * sSmiTrans   = aStmt->getSmiStmt()->getTrans();

    UInt           i             = 0;
    UInt           sExecCount    = 0;
    qciStatement * sQciStatement = aStmt->getQciStmt();
    qcStatement  * sQcStatement  = &(sQciStatement->statement);
    UInt           sDDLSrcTableOIDCount = 0;
    smOID        * sDDLSrcTableOIDArray = NULL;

    sDDLSrcTableOIDArray = qciMisc::getDDLSrcTableOIDArray( sQcStatement, &sDDLSrcTableOIDCount );

    switch( aLockMode )
    {
        case SMI_TABLE_LOCK_X:
            idlOS::strncpy( sLockMode,
                            "EXCLUSIVE",
                            ID_SIZEOF(sLockMode));
            sLockMode[9] = '\0';
            break;

        case SMI_TABLE_LOCK_S:
        case SMI_TABLE_LOCK_IS:
        case SMI_TABLE_LOCK_IX:
        case SMI_TABLE_LOCK_SIX:
            IDE_RAISE( ERR_NOT_SUPPORT_TABLE_LOCK );
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    for ( i = 0; i < sDDLSrcTableOIDCount; i++ )
    {
        sTable = smiGetTable( sDDLSrcTableOIDArray[i] );
        sSCN = smiGetRowSCN( sTable );

        IDE_TEST( smiValidateAndLockObjects( sSmiTrans,
                                             sTable,
                                             sSCN,
                                             SMI_TBSLV_DDL_DML,
                                             aLockMode,
                                             ID_ULONG_MAX,
                                             ID_FALSE )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableTempInfo( sTable, (void **)&sTableInfo ) != IDE_SUCCESS );

        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "LOCK TABLE %s.%s IN %s MODE;",
                         sTableInfo->tableOwnerName,
                         sTableInfo->name,
                         sLockMode );

        IDE_TEST( sdi::shardExecDirect( sQcStatement,
                                        NULL,
                                        sBuffer,
                                        idlOS::strlen( sBuffer ),
                                        SDI_INTERNAL_OP_NOT,
                                        &sExecCount,
                                        NULL,
                                        NULL,
                                        0,
                                        NULL )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_TABLE_LOCK );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "lockTableAllShardNodes function currently only supports is lock." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

