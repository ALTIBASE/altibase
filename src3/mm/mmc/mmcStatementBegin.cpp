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

#include <idsCrypt.h>
#include <qci.h>
#include <sdiZookeeper.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmuProperty.h>
#include <mmuOS.h>
#include <mmtAdminManager.h>
#include <mmtSnapshotExportManager.h>

mmcStmtBeginFunc mmcStatement::mBeginFunc[] =
{
    mmcStatement::beginDDL,
    mmcStatement::beginDML,
    mmcStatement::beginDCL,
    NULL,
    mmcStatement::beginSP,
    mmcStatement::beginDB,
};

mmcStmtEndFunc mmcStatement::mEndFunc[] =
{
    mmcStatement::endDDL,
    mmcStatement::endDML,
    mmcStatement::endDCL,
    NULL,
    mmcStatement::endSP,
    mmcStatement::endDB,
};


IDE_RC mmcStatement::beginDDL(mmcStatement *aStmt)
{
    mmcSession  *sSession  = aStmt->getSession();
    mmcTransObj *sTrans;
    UInt         sFlag = 0;
    UInt         sTxIsolationLevel;
    UInt         sTxTransactionMode;
    idBool       sIsReadOnly = ID_FALSE;
    idBool       sTxBegin = ID_TRUE;
    idBool       sIsDummyBegin = ID_FALSE;
    idBool       sIsSetProperty = ID_FALSE;
    
#ifdef DEBUG
    qciStmtType  sStmtType = aStmt->getStmtType();

    IDE_DASSERT(qciMisc::isStmtDDL(sStmtType) == ID_TRUE);
#endif
    
    if( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) ||
         ( aStmt->isRootStmt() == ID_FALSE ) )
    {
        sSession->setActivated(ID_TRUE);

        IDE_TEST_RAISE(sSession->isAllStmtEnd() != ID_TRUE, StmtRemainError);

        // 현재 transaction 및 transaction의 속성을 얻음
        // autocommit인데 child statement인 경우 부모 statement
        // 로부터 transaction을 받는다.
        if( ( sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
            ( aStmt->isRootStmt() == ID_FALSE ) )
        {
            sTrans = aStmt->getParentStmt()->getTransPtr();
            aStmt->setTrans(sTrans);
        }
        else
        {
            sTrans = sSession->getTransPtr();

            sTxBegin = sSession->getTransBegin();
            if ( sTxBegin == ID_TRUE )
            {
                // BUG-17497
                // non auto commit mode인 경우, 트랜잭션이 read only 인지 검사
                IDE_TEST_RAISE(sSession->isReadOnlyTransaction() == ID_TRUE,
                               TransactionModeError);
            }
            else
            {
                // BUG-47024
                IDE_TEST_RAISE(sSession->isReadOnlySession() == ID_TRUE,
                               TransactionModeError);
            }
        }
        
        /* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
        if ( ( sSession->getLockTableUntilNextDDL() == ID_TRUE ) ||
             ( sSession->isDDLAutoCommit() != ID_TRUE ) )
        {
            if ( ( aStmt->isRootStmt() == ID_TRUE ) &&
                 ( sSession->getTransBegin() != ID_TRUE ) )
            {
                mmcTrans::begin( sTrans,
                                 sSession->getStatSQL(),
                                 sSession->getSessionInfoFlagForTx(),
                                 sSession,
                                 &sIsDummyBegin );
            }

            if ( ( sSession->isShardUserSession() == ID_TRUE )  &&
                 ( sSession->isDDLAutoCommit() != ID_TRUE ) )
            {
                IDE_TEST_RAISE( qciMisc::getTransactionalDDLAvailable( aStmt->getQciStmt() ) != ID_TRUE,
                                ERR_NOT_SUPPORT_DDL_TRANSACTION );

                IDE_TEST( mmcTrans::savepoint( sTrans,
                                               sSession,
                                               MMC_DDL_BEGIN_SAVEPOINT )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
               
            if ( sSession->getLockTableUntilNextDDL() == ID_TRUE ) 
            {
                if ( sTxBegin == ID_TRUE )
                {
                    /*
                     * PROJ-2701 Sharding online data rebuild
                     * Shard meta 의 변경과 partition swap을 one transaction으로 수행 할 수 있어야 한다.
                     * DDL의 실패시 이전에 수행한 DML까지 rollback됨을 유념해야 한다.
                     */ 
                    if ( SDU_SHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE == 0 )
                    {
                        /* 데이터를 변경하는 DML과 함께 사용할 수 없다. */
                        IDE_TEST( mmcTrans::isReadOnly( sTrans, &sIsReadOnly ) != IDE_SUCCESS );

                        IDE_TEST_RAISE( sIsReadOnly != ID_TRUE,
                                        ERR_CANNOT_LOCK_TABLE_UNTIL_NEXT_DDL_WITH_DML );
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
                /* Transaction Commit & Begin을 수행하지 않는다. */
            }
        }
        else
        {
            if ( sTxBegin == ID_TRUE )
            {
                sTxIsolationLevel  = sSession->getTxIsolationLevel(sTrans);
                sTxTransactionMode = sSession->getTxTransactionMode(sTrans);
            }
            else
            {
                /* Nothing to do. */
            }

            // 현재 transaction을 commit 한 후,
            // 동일한 속성으로 transaction을 begin 함
            IDE_TEST(mmcTrans::commit(sTrans, sSession) != IDE_SUCCESS);

            // 해당 shard ddl로 일반 Tx가 shard Tx로 전환되는 경우 
            // realloc을 수행하여 shard tx로 재할당 받오록 한다. 
            if( ( sdiZookeeper::mRunningJobType == ZK_JOB_ADD ) ||
                ( sdiZookeeper::mRunningJobType == ZK_JOB_JOIN ) ||
                ( sdiZookeeper::mRunningJobType == ZK_JOB_FAILBACK ) )
            {
                sSession->reallocTrans();

                // realloc을 수행했을 경우 tx가 변경되므로 tx를 다시 받는다.
                if( ( sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
                    ( aStmt->isRootStmt() == ID_FALSE ) )
                {
                    sTrans = aStmt->getParentStmt()->getTransPtr();
                    aStmt->setTrans(sTrans);
                }
                else
                {
                    sTrans = sSession->getTransPtr();
                }
            }

            // session 속성 중 isolation level과 transaction mode는
            // transaction 속성으로 transaction 마다 다를 수 있고,
            // session의 transaction 속성과도 다를 수 있다.
            // 따라서 transaction을 위한 session info flag에서
            // transaction isolation level과 transaction mode는
            // commit 전에 속성을 가지고 있다가 begin 할때
            // 그 속성을 그대로 따르도록 설정해야 한다.
            sFlag = sSession->getSessionInfoFlagForTx();

            if ( sTxBegin == ID_TRUE )
            {
                sFlag &= ~SMI_TRANSACTION_MASK;
                sFlag |= sTxTransactionMode;

                sFlag &= ~SMI_ISOLATION_MASK;
                sFlag |= sTxIsolationLevel;
            }
            else
            {
                /* Nothing to do. */
            }

            mmcTrans::begin( sTrans, 
                             sSession->getStatSQL(), 
                             sFlag, 
                             sSession,
                             &sIsDummyBegin );
        }

        if ( sSession->isDDLAutoCommit() == ID_TRUE )
        {
            sSession->setActivated(ID_FALSE);
        }

        if ( aStmt->getStmtType() == QCI_STMT_SHARD_DDL )
        {
            IDE_TEST( qci::setPropertyForShardMeta( aStmt->getQciStmt() )
                      != IDE_SUCCESS );
            sIsSetProperty = ID_TRUE;
        }
    }
    else
    {
        // BUG-17497
        // auto commit mode인 경우,
        // transaction이 begin 전이므로 세션이 read only 인지 검사
        IDE_TEST_RAISE(sSession->isReadOnlySession() == ID_TRUE,
                       TransactionModeError);

        sSession->setActivated(ID_TRUE);

        IDE_TEST_RAISE(sSession->isAllStmtEnd() != ID_TRUE, StmtRemainError);

        sTrans = aStmt->allocTrans();

        mmcTrans::begin( sTrans,
                         sSession->getStatSQL(),
                         sSession->getSessionInfoFlagForTx(),
                         sSession,
                         &sIsDummyBegin );
    }

    IDE_TEST_RAISE(aStmt->beginSmiStmt(sTrans,
                                       SMI_STATEMENT_NORMAL)
                   != IDE_SUCCESS, BeginError);

    sSession->changeOpenStmt(1);

    aStmt->setStmtBegin(ID_TRUE);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_DDL_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "This DDL does not support DDL transaction." ) );
    }
    IDE_EXCEPTION(TransactionModeError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_MMC_ACCESS_MODE));
    }
    IDE_EXCEPTION(StmtRemainError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_OTHER_STATEMENT_REMAINS));
    }
    IDE_EXCEPTION(BeginError);
    {
        if ( sSession->isDDLAutoCommit() == ID_TRUE )
        {
            // BUG-27953 : Rollback 실패시 에러 남기고 Assert 처리
            /* PROJ-1832 New database link */
            IDE_ASSERT( mmcTrans::rollbackForceDatabaseLink(
                    sTrans, sSession )
                == IDE_SUCCESS );
        }
    }
    IDE_EXCEPTION( ERR_CANNOT_LOCK_TABLE_UNTIL_NEXT_DDL_WITH_DML )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMX_CANNOT_LOCK_TABLE_UNTIL_NEXT_DDL_WITH_DML ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsSetProperty == ID_TRUE )
    {
        (void) qci::revertPropertyForShardMeta( aStmt->getQciStmt() );
    }
    
    return IDE_FAILURE;
}

IDE_RC mmcStatement::beginDML(mmcStatement *aStmt)
{
    mmcSession    *sSession  = aStmt->getSession();
    mmcTransObj   *sTrans;
    smiTrans      *sSmiTrans;
    qciStmtType    sStmtType = aStmt->getStmtType();
    UInt           sFlag     = 0;
    mmcCommitMode  sSessionCommitMode = sSession->getCommitMode();
    smSCN          sSCN = SM_SCN_INIT;
    idBool         sIsDummyBegin = ID_FALSE;

    IDE_DASSERT(qciMisc::isStmtDML(sStmtType) == ID_TRUE);
    
    if ( sSessionCommitMode == MMC_COMMITMODE_NONAUTOCOMMIT )
    {
        sTrans = sSession->getTransPtr();
        if ( sSession->getTransBegin() == ID_FALSE )
        {
            mmcTrans::begin( sTrans,
                             sSession->getStatSQL(),
                             sSession->getSessionInfoFlagForTx(),
                             sSession,
                             &sIsDummyBegin );
        }
        else
        {
            /* Nothing to do */
            IDE_DASSERT( sTrans->mSmiTrans.isBegin() == ID_TRUE );
        }

        // BUG-17497
        // non auto commit mode인 경우이며,
        // root statement일수도 있고, child statement일수 있다.
        // transaction이 begin 한 후이므로 트랜잭션이 read only 인지 검사
        IDE_TEST_RAISE((sStmtType != QCI_STMT_SELECT) &&
                       (sSession->isReadOnlyTransaction() == ID_TRUE),
                       TransactionModeError);

        //fix BUG-24041 none-auto commit mode에서 select statement begin할때
        //mActivated를 on시키면안됨
        if(sStmtType == QCI_STMT_SELECT)
        {
            //nothing to do
        }
        else
        {
            sSession->setActivated(ID_TRUE);
        }
    }
    else
    {
        //AUTO-COMMIT-MODE, child statement.
        // fix BUG-28267 [codesonar] Ignored Return Value
        if( aStmt->isRootStmt() == ID_FALSE  )
        {
            sTrans = aStmt->getParentStmt()->getTransPtr();
            aStmt->setTrans(sTrans);
        }
        else
        {
            //AUTO-COMMIT-MODE,Root statement. 
            // BUG-17497
            // auto commit mode인 경우,
            // transaction이 begin 전이므로 세션이 read only 인지 검사
            IDE_TEST_RAISE((sStmtType != QCI_STMT_SELECT) &&
                           (sSession->isReadOnlySession() == ID_TRUE),
                           TransactionModeError);

            sTrans = aStmt->allocTrans();
            mmcTrans::begin( sTrans,
                             sSession->getStatSQL(),
                             sSession->getSessionInfoFlagForTx(),
                             sSession,
                             &sIsDummyBegin );
        }//else
        sSession->setActivated(ID_TRUE);

    }//else

    IDU_FIT_POINT("mmcStatement::beginDML::beginTrans");

    /* BUG-47029 */
    sFlag = aStmt->getSmiStatementFlag(sSession, aStmt, sTrans);

    /*
     * PROJ-2701 Sharding online data rebuild
     * 
     * Shard coordinator로 인해 하나의 transaction이 두 개의 DML statement를 연속으로 begin하지만,
     * 그 중 하나의 DML statement(hasShardCoordPlan() == true)는 DML result에 대한 전달만을 수행 하기 때문에
     * SMI_STATEMENT_SELF_TRUE로 smiStmt를 begin한다.
     */
    if ( sdi::hasShardCoordPlan( &((aStmt->getQciStmt())->statement) ) == ID_TRUE )
    {
        /* BUG-49088
         * SELECT FOR UPDATE 를 포함하여 Shard coordinator 를 통해 수행되는
         * DML statement 에 대해서는 SMI_STATEMENT_SELF_TRUE로 설정한다.
         */
        if ( sdi::isShardDML( &( ( aStmt->getQciStmt() )->statement ) ) == ID_TRUE )
        {
            /*
             * TASK-7219 Non-shard DML
             * Partial execution DML(non-shard DML) 의 경우 data nodes에서 DML 이 실제 수행되기 때문에
             * Update cursor를 사용하게 되므로, SMI_STATEMENT_SELF_TRUE 를 설정하지 않는다.
             */
            if ( sdi::isPartialCoordinator( &((aStmt->getQciStmt())->statement) ) == ID_FALSE )
            {
                sFlag |= SMI_STATEMENT_SELF_TRUE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }

        aStmt->mInfo.mFlag |= MMC_STMT_NEED_UNLOCK_TRUE;
    }
    sSmiTrans = mmcTrans::getSmiTrans(sTrans);
    IDE_TEST_RAISE(aStmt->beginSmiStmt(sTrans, sFlag) != IDE_SUCCESS, BeginError);
    sSession->changeOpenStmt(1);

    /* PROJ-1381 FAC : Holdable Fetch로 열린 Stmt 개수 조절 */
    if ((aStmt->getStmtType() == QCI_STMT_SELECT)
     && (aStmt->getCursorHold() == MMC_STMT_CURSOR_HOLD_ON))
    {
        sSmiTrans->setCursorHoldable();

        sSession->changeHoldFetch(1);
    }

    aStmt->setStmtBegin(ID_TRUE);  /* PROJ-2733 */

    /* PROJ-2626 Snapshot Export
     * iLoader 세션이고 begin Snapshot 이 실행중이고 또 Select 구문이면
     * begin Snapshot시에서 설정한 SCN을 얻어와서 자신의 smiStatement
     * 의 SCN에 설정한다. */
    if ( sSession->getClientAppInfoType() == MMC_CLIENT_APP_INFO_TYPE_ILOADER )
    {
        if ( ( aStmt->getStmtType() == QCI_STMT_SELECT ) &&
             ( mmtSnapshotExportManager::isBeginSnapshot() == ID_TRUE ))
        {
            /* REBUILD 에러가 발생했다는 것은 begin Snapshot에서 설정한 SCN으로
             * select 할때 SCN이 달라졌다는 의미이다. SCN이 달라졌다는 것은
             * 그 Table에 DDL일 발생했다는 것이고 이럴경우 Error를 발생시킨다.  */
            IDE_TEST_RAISE( ( ideGetErrorCode() & E_ACTION_MASK ) == E_ACTION_REBUILD,
                            INVALID_SNAPSHOT_SCN );

            /* Begin 시의 SCN 을 가져온다 */
            IDE_TEST( mmtSnapshotExportManager::getSnapshotSCN( &sSCN )
                      != IDE_SUCCESS );

            /* 현재 Statement에 SCN을 설정한다 */
            aStmt->mSmiStmt.setSCNForSnapshot( &sSCN );
        }
        else
        {
            IDU_FIT_POINT("mmc::mmcStatementBegin::beginDML::ILoader");
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(TransactionModeError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_MMC_ACCESS_MODE));
    }
    IDE_EXCEPTION(BeginError);
    {
        if ( ( sSession->getCommitMode() != MMC_COMMITMODE_NONAUTOCOMMIT ) &&
             ( aStmt->isRootStmt() == ID_TRUE ) )
        {
            // BUG-27953 : Rollback 실패시 에러 남기고 Assert 처리
            /* PROJ-1832 New database link */
            IDE_ASSERT( mmcTrans::rollbackForceDatabaseLink(
                            sTrans, sSession )
                        == IDE_SUCCESS );
        }
    }
    IDE_EXCEPTION( INVALID_SNAPSHOT_SCN )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INVALID_SNAPSHOT_SCN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::beginDCL(mmcStatement *aStmt)
{
    mmcSession    *sSession = aStmt->getSession();

    IDE_DASSERT(qciMisc::isStmtDCL(aStmt->getStmtType()) == ID_TRUE);

    if ( sSession != NULL )
    {
        sSession->transBeginForGTxEndTran();
    }

    aStmt->setStmtBegin(ID_TRUE);

    return IDE_SUCCESS;
}

IDE_RC mmcStatement::beginSP(mmcStatement *aStmt)
{
    mmcSession    * sSession  = aStmt->getSession();
    mmcTransObj   * sTrans    = NULL;
    smiDistTxInfo   sDistTxInfo;
    idBool          sIsDummyBegin = ID_FALSE;
    idBool          sIsSetProperty = ID_FALSE;
    
#ifdef DEBUG
    qciStmtType  sStmtType = aStmt->getStmtType();

    IDE_DASSERT(qciMisc::isStmtSP(sStmtType) == ID_TRUE);
#endif

    if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) ||
         ( aStmt->isRootStmt() == ID_FALSE ) )
    {
        if ( qci::isShardDbmsPkg( aStmt->getQciStmt() ) == ID_TRUE )
        {
            IDE_TEST( qci::setPropertyForShardMeta( aStmt->getQciStmt() )
                      != IDE_SUCCESS );
            sIsSetProperty = ID_TRUE;
        }
         
        sSession->setActivated(ID_TRUE);

        // autocommit모드이나 child statement인 경우.
        if( ( sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
            ( aStmt->isRootStmt() == ID_FALSE ) )
        {
            sTrans = aStmt->getParentStmt()->getTransPtr();
            aStmt->setTrans(sTrans);
        }
        else
        {
            sTrans = sSession->getTransPtr();
            if ( sSession->getTransBegin() == ID_FALSE )
            {
                mmcTrans::begin( sTrans,
                                 sSession->getStatSQL(),
                                 sSession->getSessionInfoFlagForTx(),
                                 sSession,
                                 &sIsDummyBegin );
            }
            else
            {
                /* Nothing to do */
            }

            // BUG-17497
            // non auto commit mode인 경우,
            // transaction이 begin 한 후이므로 트랜잭션이 read only 인지 검사
            IDE_TEST_RAISE(sSession->isReadOnlyTransaction() == ID_TRUE,
                           TransactionModeError);
        }

        // TASK-7244 PSM Partial rollback
        if ( SDU_SHARD_ENABLE == 1 )
        {
            switch ( sSession->getShardSessionType() )
            {
                case SDI_SESSION_TYPE_USER:
                {
                    qciMisc::setBeginSP( &(aStmt->getQciStmt()->statement) );
                    /* fall through */
                }
                case SDI_SESSION_TYPE_LIB:
                {
                    mmcTrans::reservePsmSvp(sTrans, ID_TRUE);
                    break;
                }
                case SDI_SESSION_TYPE_COORD:
                {
                    IDE_TEST( mmcTrans::savepoint( sTrans,
                                                   sSession,
                                                   SAVEPOINT_FOR_SHARD_SHARD_PROC_PARTIAL_ROLLBACK )
                              != IDE_SUCCESS );
                    break;
                }
                default:
                {
                    IDE_DASSERT(0);
                }
            }
        }
        else
        {
            mmcTrans::reservePsmSvp(sTrans, ID_TRUE);
        }
    }
    else
    {
        // BUG-17497
        // auto commit mode인 경우,
        // transaction이 begin 전이므로 세션이 read only 인지 검사
        IDE_TEST_RAISE(sSession->isReadOnlySession() == ID_TRUE,
                       TransactionModeError);

        sSession->setActivated(ID_TRUE);

        sTrans = aStmt->allocTrans();

        mmcTrans::begin( sTrans,
                         sSession->getStatSQL(),
                         sSession->getSessionInfoFlagForTx(),
                         aStmt->getSession(),
                         &sIsDummyBegin );
    }

    /* BUG-47459 */
    if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) &&
         ( sSession->getShardSessionType() != SDI_SESSION_TYPE_USER ) &&
         ( aStmt->isRootStmt() == ID_TRUE ) )
    {
        IDE_TEST( mmcTrans::savepoint( sTrans,
                                       sSession,
                                       sdi::getShardSavepointName( aStmt->getStmtType() ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    aStmt->setTransID(mmcTrans::getTransID(sTrans));
    
    aStmt->setSmiStmt(mmcTrans::getSmiStatement(sTrans));

    /* BUG-48115 */
    /* 분산정보를 smxTrans에 세팅한다. */
    aStmt->buildSmiDistTxInfo( &sDistTxInfo );
    sTrans->mSmiTrans.setDistTxInfo( &sDistTxInfo );

    aStmt->setStmtBegin(ID_TRUE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(TransactionModeError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_MMC_ACCESS_MODE));
    }
    IDE_EXCEPTION_END;

    if ( sIsSetProperty == ID_TRUE )
    {
        (void) qci::revertPropertyForShardMeta( aStmt->getQciStmt() );
    }

    return IDE_FAILURE;
}

#ifdef DEBUG
IDE_RC mmcStatement::beginDB(mmcStatement *aStmt)
{
    qcStatement* sQcStmt = &((aStmt->getQciStmt())->statement);

    IDE_DASSERT(sQcStmt != NULL);

    IDE_DASSERT(qciMisc::isStmtDB(aStmt->getStmtType()) == ID_TRUE);
#else
IDE_RC mmcStatement::beginDB(mmcStatement */*aStmt*/)
{
#endif

    return IDE_SUCCESS;
}

IDE_RC mmcStatement::endDDL(mmcStatement *aStmt, idBool aSuccess)
{
    mmcSession   * sSession = aStmt->getSession();
    mmcTransObj  * sTrans;
    IDE_RC         sRc = IDE_SUCCESS;
    UInt           sFlag = 0;
    UInt           sTxIsolationLevel;
    UInt           sTxTransactionMode;
    mmcStatement * sRootStatement = NULL;
    idBool         sIsDummyBegin  = ID_FALSE;
    UInt           i = 0;
    UInt           sDestTableOIDCount = 0;
    smOID        * sDestTableOIDArray = qciMisc::getDDLDestTableOIDArray( aStmt->getQciStmt(), 
                                                                          &sDestTableOIDCount );
    UInt           sDestPartOIDCount  = 0;
    smOID        * sDestPartOIDArray  = qciMisc::getDDLDestPartTableOIDArray( aStmt->getQciStmt(),
                                                                              &sDestPartOIDCount );
   
    IDE_DASSERT(qciMisc::isStmtDDL(aStmt->getStmtType()) == ID_TRUE);

    IDE_TEST( aStmt->endSmiStmt( ( aSuccess == ID_TRUE ) ?
                                 SMI_STATEMENT_RESULT_SUCCESS : SMI_STATEMENT_RESULT_FAILURE )
              != IDE_SUCCESS);

    sSession->changeOpenStmt(-1);
    aStmt->setStmtBegin(ID_FALSE);

    if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) ||
         ( aStmt->isRootStmt() == ID_FALSE ) )
    {
        // autocommit모드이나 child statement인 경우.
        if( ( sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
            ( aStmt->isRootStmt() == ID_FALSE ) )
        {
            sTrans = aStmt->getTransPtr();
        }
        else
        {
            sTrans = sSession->getTransPtr();
        }

        // BUG-17495
        // non auto commit 인 경우, 현재 transaction 속성을 기억해뒀다가
        // 동일한 속성으로 transaction을 begin 해야 함
        sTxIsolationLevel  = sSession->getTxIsolationLevel(sTrans);
        sTxTransactionMode = sSession->getTxTransactionMode(sTrans);

        // BUG-17878
        // session 속성 중 isolation level과 transaction mode는
        // transaction 속성으로 transaction 마다 다를 수 있고,
        // session의 transaction 속성과도 다를 수 있다.
        // 따라서 transaction을 위한 session info flag에서
        // transaction isolation level과 transaction mode는
        // commit 전에 속성을 가지고 있다가 begin 할때
        // 그 속성을 그대로 따르도록 설정해야 한다.
        sFlag = sSession->getSessionInfoFlagForTx();
        sFlag &= ~SMI_TRANSACTION_MASK;
        sFlag |= sTxTransactionMode;

        sFlag &= ~SMI_ISOLATION_MASK;
        sFlag |= sTxIsolationLevel;
    }
    else
    {
        sTrans = aStmt->getTransPtr();
    }

    if ( sSession->isDDLAutoCommit() == ID_TRUE )
    {
        if (aSuccess == ID_TRUE)
        {
            sRc = mmcTrans::commit(sTrans, sSession);
            if (sRc == IDE_SUCCESS)
            {
                // 해당 shard ddl로 일반 Tx가 shard Tx로 전환되는 경우 
                // realloc을 수행하여 shard tx로 재할당 받오록 한다. 
                if( ( sdiZookeeper::mRunningJobType == ZK_JOB_ADD ) ||
                    ( sdiZookeeper::mRunningJobType == ZK_JOB_JOIN ) ||
                    ( sdiZookeeper::mRunningJobType == ZK_JOB_FAILBACK ) )
                {
                    sSession->reallocTrans();

                    // realloc을 수행했을 경우 tx가 변경되었을 수 있으므로 tx를 다시 받는다.
                    if( ( sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
                        ( aStmt->isRootStmt() == ID_FALSE ) )
                    {
                        sTrans = aStmt->getTransPtr();
                    }
                    else
                    {
                        sTrans = sSession->getTransPtr();
                    }
                }


                if (sSession->getQueueInfo() != NULL)
                {
                    mmqManager::freeQueue(sSession->getQueueInfo());
                }
            }
            else
            {
                /* PROJ-1832 New database link */
                IDE_ASSERT( mmcTrans::rollbackForceDatabaseLink(
                        sTrans, sSession )
                    == IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( mmcTrans::rollbackForceDatabaseLink(
                    sTrans, sSession )
                != IDE_SUCCESS );
        }
    }
    else
    {
        if ( aSuccess == ID_TRUE )
        {
            for ( i = 0; i < sDestTableOIDCount; i++ )
            {
                IDE_TEST( sTrans->mSmiTrans.setExpSvpForBackupDDLTargetTableInfo( SM_OID_NULL,
                                                                                  0,
                                                                                  NULL,
                                                                                  sDestTableOIDArray[i],
                                                                                  sDestPartOIDCount,
                                                                                  sDestPartOIDArray +
                                                                                  (i * sDestPartOIDCount) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            if ( sSession->getShardSessionType() == SDI_SESSION_TYPE_USER )
            {
                IDE_TEST( mmcTrans::rollback( sTrans,
                                              sSession,
                                              MMC_DDL_BEGIN_SAVEPOINT,
                                              SMI_DO_NOT_RELEASE_TRANSACTION )
                          != IDE_SUCCESS );
            }
        }
        IDE_RAISE( NORMAL_EXIT );
    }

    if ( ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) &&
           ( sSession->getTransLazyBegin() == ID_FALSE ) ) || // BUG-45772 TRANSACTION_START_MODE 지원
         ( aStmt->isRootStmt() == ID_FALSE ) )
    {
        // BUG-17497
        // non auto commit mode인 경우,
        // commit 전 저장해둔 transaction 속성으로 transaction을 begin 한다.
        mmcTrans::begin( sTrans,
                         sSession->getStatSQL(),
                         sFlag,
                         aStmt->getSession(),
                         &sIsDummyBegin );

        // BUG-20673 : PSM Dynamic SQL
        if ( aStmt->isRootStmt() == ID_FALSE )
        {
            mmcTrans::reservePsmSvp(sTrans, ID_FALSE);
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-47459 */
    if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) &&
         ( sSession->getShardSessionType() != SDI_SESSION_TYPE_USER ) &&
         ( aStmt->isRootStmt() == ID_FALSE ) )
    {
        sRootStatement = aStmt->getRootStmt();

        IDE_TEST( mmcTrans::savepoint( sTrans,
                                       sSession,
                                       sdi::getShardSavepointName( sRootStatement->getStmtType() ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    sSession->setQueueInfo(NULL);    

    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    if ( sSession->isDDLAutoCommit() == ID_TRUE )
    {
        sSession->setActivated(ID_FALSE);
    }    

    if( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) ||
        ( aStmt->isRootStmt() == ID_FALSE ) )
    {            
        if ( aStmt->getStmtType() == QCI_STMT_SHARD_DDL )
        {
            IDE_TEST( qci::revertPropertyForShardMeta( aStmt->getQciStmt() )
                      != IDE_SUCCESS );
        }                
    }
        
    IDE_TEST(sRc != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        sSession->setQueueInfo(NULL);
    }
    if( aStmt->isStmtBegin() == ID_TRUE )
    {
        sSession->changeOpenStmt(-1);
        aStmt->setStmtBegin(ID_FALSE);
    }
    if ( sSession->isDDLAutoCommit() == ID_TRUE )
    {
        sSession->setActivated(ID_FALSE);
    }

    if ( aStmt->getStmtType() == QCI_STMT_SHARD_DDL )
    {
        (void)qci::revertPropertyForShardMeta( aStmt->getQciStmt() );
    }
    
    return IDE_FAILURE;
}

IDE_RC mmcStatement::endDML(mmcStatement *aStmt, idBool aSuccess)
{
    mmcSession  *sSession = aStmt->getSession();
    mmcTransObj *sTrans;
    
    IDE_DASSERT(qciMisc::isStmtDML(aStmt->getStmtType()) == ID_TRUE);

    IDE_TEST(aStmt->endSmiStmt((aSuccess == ID_TRUE) ?
                               SMI_STATEMENT_RESULT_SUCCESS : SMI_STATEMENT_RESULT_FAILURE)
             != IDE_SUCCESS);

    /* BUG-29224
     * Change a Statement Member variable after end of method.
     */
    sSession->changeOpenStmt(-1);

    /* PROJ-1381 FAC : Holdable Fetch로 열린 Stmt 개수 조절 */
    if ( ( aStmt->getStmtType() == QCI_STMT_SELECT ) &&
         ( aStmt->getCursorHold() == MMC_STMT_CURSOR_HOLD_ON ) )
    {
        sSession->changeHoldFetch(-1);
    }

    aStmt->setStmtBegin(ID_FALSE);

    if ( ( sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
         ( aStmt->isRootStmt() == ID_TRUE ) )
    {
        sTrans = aStmt->getTransPtr();

        if (sTrans == NULL)
        {
            /*  PROJ-1381 Fetch AcrossCommit
             * Holdable Stmt는 Tx보다 나중에 끝날 수 있다.
             * 에러가 아니니 조용히 넘어간다. */
            IDE_ASSERT(aStmt->getCursorHold() == MMC_STMT_CURSOR_HOLD_ON);
        }
        else
        {
            if (aSuccess == ID_TRUE)
            {
                /* BUG-37674 */
                IDE_TEST_RAISE( mmcTrans::commit( sTrans, sSession ) 
                                != IDE_SUCCESS, CommitError );
            }
            else
            {
                /* PROJ-1832 New database link */
                IDE_TEST( mmcTrans::rollbackForceDatabaseLink(
                              sTrans, sSession )
                          != IDE_SUCCESS );
            }
        }

        sSession->setActivated(ID_FALSE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(CommitError);
    {
        /* PROJ-1832 New database link */
        IDE_ASSERT( mmcTrans::rollbackForceDatabaseLink(
                        sTrans, sSession )
                    == IDE_SUCCESS );
        sSession->setActivated(ID_FALSE);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatement::endDCL(mmcStatement *aStmt, idBool /*aSuccess*/)
{
    IDE_DASSERT(qciMisc::isStmtDCL(aStmt->getStmtType()) == ID_TRUE);

    aStmt->setStmtBegin(ID_FALSE);

    return IDE_SUCCESS;
}

IDE_RC mmcStatement::endSP(mmcStatement *aStmt, idBool aSuccess)
{
    mmcSession  *sSession = aStmt->getSession();
    mmcTransObj *sTrans;
    idBool       sTxBegin = ID_TRUE;

    IDE_DASSERT(qciMisc::isStmtSP(aStmt->getStmtType()) == ID_TRUE);

    aStmt->setStmtBegin(ID_FALSE);

    if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) ||
         ( aStmt->isRootStmt() == ID_FALSE ) )
    {
        if( ( sSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
            ( aStmt->isRootStmt() == ID_FALSE ) )
        {
            sTrans = aStmt->getParentStmt()->getTransPtr();
            aStmt->setTrans(sTrans);
        }
        else
        {
            sTrans = sSession->getTransPtr();
            sTxBegin = sSession->getTransBegin();
        }

        if ( sTxBegin == ID_TRUE )
        {
            if (aSuccess == ID_TRUE)
            {
                // TASK-7244 PSM Partial rollback
                if ( SDU_SHARD_ENABLE == 1 )
                {
                    switch ( sSession->getShardSessionType() )
                    {
                        case SDI_SESSION_TYPE_USER:
                        {
                            IDE_TEST( sdi::clearPsmSvp( sSession->getQciSession()->mQPSpecific.mClientInfo )
                                      != IDE_SUCCESS );
                            /* fall through */
                        }
                        case SDI_SESSION_TYPE_LIB:
                        {
                            mmcTrans::clearPsmSvp(sTrans);
                            break;
                        }
                        case SDI_SESSION_TYPE_COORD:
                        {
                            // Coord session에서는 할 것이 없다.
                            break;
                        }
                        default:
                        {
                            IDE_DASSERT(0);
                        }
                    }
                }
                else
                {
                    mmcTrans::clearPsmSvp(sTrans);
                }
            }
            else
            {
                // TASK-7244 PSM Partial rollback
                if ( SDU_SHARD_ENABLE == 1 )
                {
                    switch ( sSession->getShardSessionType() )
                    {
                        case SDI_SESSION_TYPE_USER:
                        {
                            IDE_TEST( sdi::rollbackForPSM( &(aStmt->getQciStmt()->statement),
                                                           sSession->getQciSession()->mQPSpecific.mClientInfo )
                                      != IDE_SUCCESS );
                            IDE_TEST( sdi::clearPsmSvp( sSession->getQciSession()->mQPSpecific.mClientInfo )
                                      != IDE_SUCCESS );
                            /* fall through */
                        }
                        case SDI_SESSION_TYPE_LIB:
                        {
                            IDE_TEST(mmcTrans::abortToPsmSvp(sTrans) != IDE_SUCCESS);
                            break;
                        }
                        case SDI_SESSION_TYPE_COORD:
                        {
                            IDE_TEST( mmcTrans::rollback( sTrans,
                                                          sSession,
                                                          SAVEPOINT_FOR_SHARD_SHARD_PROC_PARTIAL_ROLLBACK,
                                                          SMI_DO_NOT_RELEASE_TRANSACTION )
                                      != IDE_SUCCESS );
                            break;
                        }
                        default:
                        {
                            IDE_DASSERT(0);
                        }
                    }
                }
                else
                {
                    IDE_TEST(mmcTrans::abortToPsmSvp(sTrans) != IDE_SUCCESS);
                }
            }
        }
        else
        {
            /* Nothing to do */
        }

        if ( SDU_SHARD_ENABLE == 1 )
        {
            qciMisc::unsetBeginSP( &(aStmt->getQciStmt()->statement) );
        }
    }
    else
    {
        // BUG-36203 PSM Optimize
        IDE_TEST( aStmt->freeChildStmt( ID_TRUE,
                                        ID_FALSE )
                  != IDE_SUCCESS );

        sTrans = aStmt->getTransPtr();

        if (aSuccess == ID_TRUE)
        {
            /* BUG-37674 */
            IDE_TEST_RAISE( mmcTrans::commit( sTrans, sSession ) 
                            != IDE_SUCCESS, CommitError );
        }
        else
        {
            /* PROJ-1832 New database link */
            IDE_TEST( mmcTrans::rollbackForceDatabaseLink(
                          sTrans, sSession )
                     != IDE_SUCCESS );
        }

        sSession->setActivated(ID_FALSE);
    }

    if ( ( sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT ) ||
         ( aStmt->isRootStmt() == ID_FALSE ) )
    {        
        if ( qci::isShardDbmsPkg( aStmt->getQciStmt() ) == ID_TRUE )
        {
            IDE_TEST( qci::revertPropertyForShardMeta( aStmt->getQciStmt() )
                      != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(CommitError);
    {
        // BUG-27953 : Rollback 실패시 에러 남기고 Assert 처리
        /* PROJ-1832 New database link */
        IDE_ASSERT ( mmcTrans::rollbackForceDatabaseLink(
                         sTrans, sSession )
                     == IDE_SUCCESS );
        sSession->setActivated(ID_FALSE);
    }
    IDE_EXCEPTION_END;

    if ( qci::isShardDbmsPkg( aStmt->getQciStmt() ) == ID_TRUE )
    {
        (void)qci::revertPropertyForShardMeta( aStmt->getQciStmt() );
    }
    
    return IDE_FAILURE;
}

IDE_RC mmcStatement::endDB(mmcStatement * /*aStmt*/, idBool /*aSuccess*/)
{
    return IDE_SUCCESS;
}
