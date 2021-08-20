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

/*****************************************************************************
 * Server 내에서 사용하는 SQL처리를 담당한다.
 * 모든 함수는 callback function이 된다.
 ****************************************************************************/

#include <qci.h>
#include <mmErrorCode.h>
#include <mmcTask.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmtInternalSql.h>
#include <mmtAuditManager.h>

qciInternalSQLCallback mmtInternalSql::mCallback =
{
    mmtInternalSql::allocStmt,
    mmtInternalSql::prepare,
    mmtInternalSql::paramInfoSet,
    mmtInternalSql::bindParamData,
//     mmtInternalSql::columnInfoSet,
    mmtInternalSql::execute,
    mmtInternalSql::fetch,
    mmtInternalSql::freeStmt,
    mmtInternalSql::checkBindParamCount,
    mmtInternalSql::checkBindColumnCount,
    mmtInternalSql::getQciStmt,
    mmtInternalSql::endFetch
};

static IDE_RC fetchEnd( mmcStatement * aStatement )
{
    IDE_RC sRet;
    UShort sEnableResultSetCount;

    /* TAKS-7219 
     *  Cursor Close 전에
     *  mmcStatement::makePlanTreeBeforeCloseCursor 를 호출해야 하는데,
     *  아래의 qci::closeCursor 외에도 
     *  aStatement->endFetch 에서 qci::closeCursor 로
     *  Temp Table Cursor를 Close 합니다.
     *  따라서 모든 Cursor Close 이전의 위치로 변경해서 호출하도록 수정합니다.
     */
    // BUG-47831 View plan of query in PSM
    mmcStatement::makePlanTreeBeforeCloseCursor( aStatement,
                                                 aStatement );

    sEnableResultSetCount = aStatement->getEnableResultSetCount();

    if ( aStatement->getResultSetState(MMC_RESULTSET_FIRST) !=
         MMC_RESULTSET_STATE_FETCH_CLOSE )
    {
        IDE_TEST(qci::closeCursor(aStatement->getQciStmt(),
                                  aStatement->getSmiStmt())
                 != IDE_SUCCESS);

        // Fetch 가능한 Result Set을 하나 감소
        sEnableResultSetCount--;
        aStatement->setEnableResultSetCount(sEnableResultSetCount);
    }

    IDE_TEST(aStatement->endFetch(MMC_RESULTSET_FIRST) != IDE_SUCCESS);

    if (sEnableResultSetCount <= 0)
    {
        /* PROJ-2223 Altibase Auditing */
        mmtAuditManager::auditByAccess( aStatement, MMC_EXECUTION_FLAG_SUCCESS );

        IDE_TEST(qci::closeCursor(aStatement->getQciStmt(),
                                  aStatement->getSmiStmt())
                 != IDE_SUCCESS);

        sRet = aStatement->clearStmt(MMC_STMT_BIND_NONE);

        IDE_TEST(aStatement->endStmt(MMC_EXECUTION_FLAG_SUCCESS) != IDE_SUCCESS);

        aStatement->setExecuteFlag(ID_FALSE);

        IDE_TEST(sRet != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC doFetch(void * aUserContext)
{
    qciSQLFetchContext * sArg;
    mmcStatement       * sStatement;
    qciBindData        * sBindColumnData;

    sArg = (qciSQLFetchContext*)aUserContext;
    sStatement = (mmcStatement*)sArg->mmStatement;

    for( sBindColumnData = sArg->bindColumnDataList;
         sBindColumnData != NULL;
         sBindColumnData = sBindColumnData->next )
    {
        IDE_TEST(qci::fetchColumn(sArg->memory,
                                  sStatement->getQciStmt(),
                                  sBindColumnData->id,
                                  sBindColumnData->column,
                                  sBindColumnData->data) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC doRetry(mmcStatement *aStmt)
{
    IDE_TEST(qci::retry(aStmt->getQciStmt(), aStmt->getSmiStmt()) != IDE_SUCCESS);

    IDE_TEST(aStmt->endStmt(MMC_EXECUTION_FLAG_RETRY) != IDE_SUCCESS);

    IDE_TEST(aStmt->beginStmt() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC doRebuild(mmcStatement *aStmt)
{
    IDE_TEST(qci::closeCursor(aStmt->getQciStmt(), aStmt->getSmiStmt())
             != IDE_SUCCESS);

    IDE_TEST(aStmt->endStmt(MMC_EXECUTION_FLAG_REBUILD) != IDE_SUCCESS);

    // BUG_12177
    aStmt->setCursorFlag(SMI_STATEMENT_ALL_CURSOR);

    IDE_TEST(aStmt->beginStmt() != IDE_SUCCESS);

    IDE_TEST(aStmt->rebuild() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC sendBindOut(void * aUserContext)
{
    qciSQLExecuteContext * sArg;
    mmcStatement         * sStatement;
    qciStatement         * sQciStmt;
    qciBindParam           sBindParam;
    qciBindData          * sBindParamData;

    sArg = (qciSQLExecuteContext*)aUserContext;
    sStatement = (mmcStatement*)sArg->mmStatement;

    sQciStmt = sStatement->getQciStmt();

    for ( sBindParamData = sArg->outBindParamDataList;
          sBindParamData != NULL;
          sBindParamData = sBindParamData->next )
    {
        sBindParam.id = sBindParamData->id;

        IDE_TEST(qci::getBindParamInfo(sQciStmt, &sBindParam) != IDE_SUCCESS);

        if ((sBindParam.inoutType == CMP_DB_PARAM_INPUT_OUTPUT) ||
            (sBindParam.inoutType == CMP_DB_PARAM_OUTPUT))
        {
            IDE_TEST(qci::getBindParamData( sQciStmt,
                                            sBindParamData->id,
                                            sBindParamData->data ) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC doEnd(void * aUserContext)
{
    qciSQLExecuteContext * sArg;
    mmcStatement         * sStatement;
    IDE_RC                 sRet  = IDE_SUCCESS;

    sArg = (qciSQLExecuteContext*)aUserContext;
    sStatement = (mmcStatement*)sArg->mmStatement;

    sRet = sendBindOut(aUserContext);

    /* PROJ-2223 Altibase Auditing */
    mmtAuditManager::auditByAccess( sStatement, MMC_EXECUTION_FLAG_SUCCESS );
    
    IDE_TEST(sStatement->clearStmt(MMC_STMT_BIND_NONE) != IDE_SUCCESS);

    IDE_TEST(sStatement->endStmt(MMC_EXECUTION_FLAG_SUCCESS) != IDE_SUCCESS);

    sStatement->setExecuteFlag(ID_FALSE);

    IDE_TEST_RAISE(sRet != IDE_SUCCESS, SendBindOutError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SendBindOutError);

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mmtInternalSql::allocStmt( void * aUserContext )
{
    qciSQLAllocStmtContext * sArg;
    mmcSession          * sSession;
    mmcStatement        * sStatement;

    sArg = (qciSQLAllocStmtContext*)aUserContext;

    sSession = (mmcSession*)sArg->mmSession;

    IDE_TEST( mmcStatementManager::allocStatement(
                  &sStatement,
                  sSession,
                  (mmcStatement*)sArg->mmParentStatement)
              != IDE_SUCCESS );

    /* BUG-43003
     * job session의 statement 같은 어디에도 속하지 않는
     * 독립적인 statement인 경우, session에 기록한다.
     */
    if ( sArg->dedicatedMode == ID_TRUE )
    {
        sSession->getInfo()->mCurrStmtID = sStatement->getStmtID();
        
        IDU_SESSION_CLR_CANCELED( *sSession->getEventFlag() );
        IDU_SESSION_CLR_TIMEOUT( *sSession->getEventFlag() );
    }
    else
    {
        /* Nothing to do. */
    }
    
    sArg->mmStatement = (void*)sStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mmtInternalSql::prepare( void * aUserContext )
{
    qciSQLPrepareContext * sArg;
    mmcStatement         * sStatement;

    SChar                * sQuery;
    qciStatement         * sQciStmt = NULL;

    sArg = (qciSQLPrepareContext*)aUserContext;

    sStatement       = (mmcStatement*)sArg->mmStatement;

    /* PROJ-2223 Altibase Auditing */
    mmtAuditManager::auditBySession( sStatement );

    IDE_TEST( sStatement->clearPlanTreeText()
             != IDE_SUCCESS );

    IDE_TEST_RAISE( sStatement->getStmtState() >=
                    MMC_STMT_STATE_EXECUTED,
                    InvalidStatementState );

    sQciStmt = sStatement->getQciStmt();

    IDE_TEST( qci::clearStatement( sQciStmt,
                                   sStatement->getSmiStmt(),
                                   QCI_STMT_STATE_INITIALIZED )
              != IDE_SUCCESS);

    sStatement->setStmtState( MMC_STMT_STATE_ALLOC );

    IDE_TEST_RAISE( sArg->sqlStringLen == 0, NullQuery );

    IDU_FIT_POINT_RAISE( "mmtInternalSql::prepare::malloc::Query",
                          InsufficientMemory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_MMC,
                                       sArg->sqlStringLen + 1,
                                       (void **)&sQuery,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, InsufficientMemory );

    idlOS::memcpy( sQuery,
                   sArg->sqlString,
                   sArg->sqlStringLen );

    sQuery[sArg->sqlStringLen] = 0;

    sStatement->setStmtExecMode( ( sArg->execMode == ID_TRUE ) ?
                                 MMC_STMT_EXEC_DIRECT :
                                 MMC_STMT_EXEC_PREPARED );

    if (sdi::isShardEnable() == ID_TRUE)
    {
        /* BUG-49075 샤딩에서는 FAC가 지원되지 않아 HOLD_OFF로 설정한다. */
        sStatement->setCursorHold(MMC_STMT_CURSOR_HOLD_OFF);
    }
    else
    {
        /* PROJ-1381 FAC : InternalSql은 항상 HOLD_ON으로 실행한다. */
        sStatement->setCursorHold(MMC_STMT_CURSOR_HOLD_ON);
    }

    // BUG-41030 Set called by PSM Flag
    qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_TRUE );

    IDE_TEST( sStatement->prepare( sQuery,
                                   sArg->sqlStringLen )
              != IDE_SUCCESS );

    // BUG-41030 Unset called by PSM Flag
    qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_FALSE );

    sArg->stmtType = sStatement->getStmtType();

    IDE_TEST( qci::checkInternalProcCall( sStatement->getQciStmt() )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NullQuery);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INSUFFICIENT_QUERY_ERROR));
    }
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if ( sQciStmt != NULL )
    {
        // BUG-41030 Unset called by PSM Flag
        qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_FALSE );
    }
    return IDE_FAILURE;
}

IDE_RC
mmtInternalSql::paramInfoSet( void * aUserContext )
{
    qciSQLParamInfoContext * sArg;
    mmcStatement           * sStatement;
    UShort                   sParamCount;
    qciBindParam           * sBindParam;
    UShort                   sParamNumber;

    sArg = (qciSQLParamInfoContext*)aUserContext;
    sStatement = (mmcStatement*)sArg->mmStatement;

    sBindParam = &sArg->bindParam;

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    // BUG-41248 DBMS_SQL package
    if ( sBindParam->id != ID_USHORT_MAX )
    {
        sParamNumber = sBindParam->id + 1;
 
        sParamCount = qci::getParameterCount(sStatement->getQciStmt());
 
        IDE_TEST_RAISE(sParamNumber > sParamCount, NoParameter);
        IDE_TEST_RAISE(sParamNumber == 0, InvalidParameter);
    }
    else
    {
        // Nothing to do.
    }

    if (sStatement->getBindState() > MMC_STMT_BIND_NONE)
    {
        IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                     NULL,
                                     QCI_STMT_STATE_PREPARED) != IDE_SUCCESS);

        sStatement->setBindState(MMC_STMT_BIND_NONE);
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41248 DBMS_SQL package
    if ( sBindParam->id != ID_USHORT_MAX )
    {
        IDE_TEST(qci::setBindParamInfo(sStatement->getQciStmt(), sBindParam) != IDE_SUCCESS);
 
        // prj-1697
        if ( sParamNumber == sParamCount )
        {
            sStatement->setBindState(MMC_STMT_BIND_INFO);
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        IDE_TEST( qci::setBindParamInfoByName( sStatement->getQciStmt(),
                                               sBindParam ) != IDE_SUCCESS );

        if ( qci::isBindParamEnd( sStatement->getQciStmt() ) == ID_TRUE )
        {
            sStatement->setBindState( MMC_STMT_BIND_INFO );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NoParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_PARAMETER));
    }
    IDE_EXCEPTION(InvalidParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_PARAMETER_NUMBER));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-19669
// client에서 qci::setBindColumnInfo()를 사용하지 않으므로
// PSM도 사용하지 않도록 수정한다.
//
// IDE_RC
// mmtInternalSql::columnInfoSet( void * aUserContext )
// {
//     qciSQLColumnInfoContext * sArg;
//     mmcStatement            * sStatement;
//     UShort                    sColumnCount;
//     qciBindColumn           * sBindColumn;
//     UShort                    sColumnNumber;

//     sArg = (qciSQLColumnInfoContext*)aUserContext;
//     sStatement = (mmcStatement*)sArg->mmStatement;

//     sBindColumn = &sArg->bindColumn;

//     IDE_TEST_RAISE( sStatement->getStmtState() < MMC_STMT_STATE_EXECUTED,
//                     NoCursor );

//     sColumnNumber = sBindColumn->id + 1;

//     sColumnCount = qci::getColumnCount(sStatement->getQciStmt());

//     IDE_TEST_RAISE(sColumnNumber > sColumnCount, NoColumn);
//     IDE_TEST_RAISE(sColumnNumber == 0, InvalidColumn);

//     IDE_TEST(qci::setBindColumnInfo(sStatement->getQciStmt(), sBindColumn) != IDE_SUCCESS);

//     return IDE_SUCCESS;

//     IDE_EXCEPTION(NoCursor);
//     {
//         IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_FETCH_ERROR));
//     }
//     IDE_EXCEPTION(NoColumn);
//     {
//         IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_COLUMN));
//     }
//     IDE_EXCEPTION(InvalidColumn);
//     {
//         IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_COLUMN_NUMBER));
//     }
//     IDE_EXCEPTION_END;

//     return IDE_FAILURE;
// }

IDE_RC
mmtInternalSql::bindParamData( void * aUserContext )
{
    qciSQLParamDataContext * sArg;
    mmcStatement           * sStatement;
    UShort                   sParamCount;
    UShort                   sParamNumber;
    qciBindData            * sBindParamData;

    sArg = (qciSQLParamDataContext*)aUserContext;

    sStatement = (mmcStatement*)sArg->mmStatement;

    sBindParamData = &sArg->bindParamData;

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    if (sStatement->getBindState() > MMC_STMT_BIND_INFO)
    {
        sStatement->setBindState(MMC_STMT_BIND_INFO);
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41248 DBMS_SQL package
    if ( sBindParamData->id != ID_USHORT_MAX )
    {
        sParamNumber = sBindParamData->id + 1;

        sParamCount = qci::getParameterCount(sStatement->getQciStmt());
    
        IDE_TEST_RAISE(sParamNumber > sParamCount, NoParameter);
        IDE_TEST_RAISE(sParamNumber == 0, InvalidParameter);

        IDE_TEST( qci::setBindParamData( sStatement->getQciStmt(),
                                         sBindParamData->id,
                                         sBindParamData->data,
                                         sBindParamData->size )
                 != IDE_SUCCESS );
 
 
        if ( sParamNumber == sParamCount )
        {
            sStatement->setBindState(MMC_STMT_BIND_DATA);
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        IDE_TEST( qci::setBindParamDataByName( sStatement->getQciStmt(),
                                               sBindParamData->name,
                                               sBindParamData->data,
                                               sBindParamData->size )
                  != IDE_SUCCESS );

        if ( qci::isBindDataEnd( sStatement->getQciStmt() ) == ID_TRUE )
        {
            sStatement->setBindState( MMC_STMT_BIND_DATA );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NoParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_PARAMETER));
    }
    IDE_EXCEPTION(InvalidParameter);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_BIND_PARAMETER_NUMBER));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mmtInternalSql::execute( void * aUserContext )
{
    qciSQLExecuteContext * sArg;
    mmcSession           * sSession = NULL;
    mmcStatement         * sStatement;
    qciStatement         * sQciStmt;
    smiStatement         * sSmiStmt;
    idBool                 sRetry;
    idBool                 sRecordExist;
    UShort                 sResultSetCount;
    idBool                 sBeginFetchTime = ID_FALSE;
    UInt                   sOldResultSetHWM = 0;
    UInt                   sStmtRetryMax;
    UInt                   sRebuildRetryMax = SDI_SHARD_RETRY_LOOP_MAX;

    sArg = (qciSQLExecuteContext*)aUserContext;
    sStatement = (mmcStatement*)sArg->mmStatement;
    sSession = sStatement->getSession();

    sStmtRetryMax = sSession->getShardStatementRetry();

    sArg->affectedRowCount = -1;
    sArg->fetchedRowCount = -1;
    sRecordExist = ID_FALSE;
    sResultSetCount = 0;

    sQciStmt = sStatement->getQciStmt();
    sSmiStmt = sStatement->getSmiStmt();

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    sStatement->setArray(ID_FALSE);
    sStatement->setRowNumber(1);

    // PROJ-2163
    // BUG-36203 PSM Optimize
    if (sArg->isFirst == ID_TRUE )
    {
        // BUG-41030 Set called by PSM Flag
        qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_TRUE );
        IDE_TEST(sStatement->reprepare() != IDE_SUCCESS);
    }
    else
    {
        // BUG-36203 PSM Optimize
        if( sStatement->getBindState() == MMC_STMT_BIND_INFO )
        {
            IDE_TEST( sStatement->changeStmtState()
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-41030 Unset called by PSM Flag
    qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_FALSE );

    IDE_TEST(sStatement->beginStmt() != IDE_SUCCESS);

    do
    {
        do
        {
            sRetry = ID_FALSE;

            if (sStatement->execute(&sArg->affectedRowCount,
                                    &sArg->fetchedRowCount) != IDE_SUCCESS)
            {
                IDE_TEST_RAISE( sSession->processShardRetryError( sStatement,
                                                                  &sStmtRetryMax,
                                                                  &sRebuildRetryMax )
                                != IDE_SUCCESS,
                                ExecuteFailAbort );

                switch (ideGetErrorCode() & E_ACTION_MASK)
                {
                    case E_ACTION_IGNORE:
                        IDE_RAISE(ExecuteFailIgnore);
                        break;

                    case E_ACTION_RETRY:
                        IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_REBUILD:
                        if (sArg->isFirst == ID_TRUE )
                        {
                            // BUG-47707 rebuil시 invalid host variable error가 발생합니다.
                            qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_TRUE );
                        }

                        IDE_TEST(doRebuild(sStatement) != IDE_SUCCESS);

                        qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_FALSE );

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_ABORT:
                        IDE_RAISE(ExecuteFailAbort);
                        break;

                    case E_ACTION_FATAL:
                        IDE_CALLBACK_FATAL("fatal error returned from mmcStatement::execute()");
                        break;

                    default:
                        IDE_CALLBACK_FATAL("invalid error action returned from mmcStatement::execute()");
                        break;
                }
            }

        } while (sRetry == ID_TRUE);

        sStatement->setStmtState(MMC_STMT_STATE_EXECUTED);

        sOldResultSetHWM = sStatement->getResultSetHWM();
        sResultSetCount = qci::getResultSetCount(sStatement->getQciStmt());
        sStatement->setResultSetCount(sResultSetCount);
        sStatement->setEnableResultSetCount(sResultSetCount);
        //fix BUG-27198 Code-Sonar  return value ignore하여, 결국 mResultSet가
        // null pointer일때 이를 de-reference를 할수 있습니다.
        IDE_TEST_RAISE(sStatement->initializeResultSet(sResultSetCount) != IDE_SUCCESS,
                       RestoreResultSetValues);


        if (sResultSetCount > 0)
        {
            // fix BUG-31195
            // qci::moveNextRecord() 시간을 Fetch Time에 추가
            sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());
            /* BUG-19456 */
            sStatement->setFetchEndTime(0);

            IDV_SQL_OPTIME_BEGIN( sStatement->getStatistics(),
                                  IDV_OPTM_INDEX_QUERY_FETCH);

            sBeginFetchTime = ID_TRUE;

            sStatement->setFetchFlag(MMC_FETCH_FLAG_PROCEED);

            if (qci::moveNextRecord(sQciStmt,
                                    sStatement->getSmiStmt(),
                                    &sRecordExist) != IDE_SUCCESS)
            {
                // fix BUG-31195
                sBeginFetchTime = ID_FALSE;

                sStatement->setFetchStartTime(0);
                /* BUG-19456 */
                sStatement->setFetchEndTime(0);

                IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                    IDV_OPTM_INDEX_QUERY_FETCH );

                IDE_TEST_RAISE( sSession->processShardRetryError( sStatement,
                                                                  &sStmtRetryMax,
                                                                  &sRebuildRetryMax )
                                != IDE_SUCCESS,
                                MoveNextRecordFailAbort );

                switch (ideGetErrorCode() & E_ACTION_MASK)
                {
                    case E_ACTION_IGNORE:
                        IDE_RAISE(MoveNextRecordFailIgnore);
                        break;

                    case E_ACTION_RETRY:
                        IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_REBUILD:
                        if (sArg->isFirst == ID_TRUE )
                        {
                            // BUG-47707 rebuil시 invalid host variable error가 발생합니다.
                            qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_TRUE );
                        }

                        IDE_TEST(doRebuild(sStatement) != IDE_SUCCESS);

                        qciMisc::setPSMFlag( ((void*)&(sQciStmt->statement)), ID_FALSE);

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_ABORT:
                        IDE_RAISE(MoveNextRecordFailAbort);
                        break;

                    case E_ACTION_FATAL:
                        IDE_CALLBACK_FATAL("fatal error returned from qci::moveNextRecord()");
                        break;

                    default:
                        IDE_CALLBACK_FATAL("invalid error action returned from qci::moveNextRecord()");
                        break;
                }
            }
            else
            {
                if (sRecordExist == ID_TRUE)
                {
                    IDE_TEST(sStatement->beginFetch(MMC_RESULTSET_FIRST) != IDE_SUCCESS);
                }
                else
                {
                    // fix BUG-31195
                    sBeginFetchTime = ID_FALSE;

                    sStatement->setFetchStartTime(0);
                    /* BUG-19456 */
                    sStatement->setFetchEndTime(0);

                    IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                        IDV_OPTM_INDEX_QUERY_FETCH );

                    if (sResultSetCount == 1)
                    {
                        sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);

                        // Nothing to do.
                        // resultSet이 있는 경우 bind정보를 주기 위해
                        // record가 없어도 statement를 닫지 않는다.
                        // IDE_TEST(doEnd(aUserContext) != IDE_SUCCESS);
                    }
                }
            }
        }
        else
        {
            if (qci::closeCursor(sQciStmt, sSmiStmt) != IDE_SUCCESS)
            {
                IDE_TEST(ideIsRetry() != IDE_SUCCESS);

                IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                sRetry = ID_TRUE;
            }
            else
            {
                // BUG-47831 View plan of query in PSM
                mmcStatement::makePlanTreeBeforeCloseCursor( sStatement,
                                                             sStatement );

                IDE_TEST(doEnd(aUserContext) != IDE_SUCCESS);
            }
        }

    } while (sRetry == ID_TRUE);

    sArg->recordExist = sRecordExist;
    sArg->resultSetCount = sResultSetCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(ExecuteFailIgnore);
    {
        SChar sMsg[64];

        idlOS::snprintf(sMsg, ID_SIZEOF(sMsg), "mmcStatement::execute code=0x%x", ideGetErrorCode());

        IDE_SET(ideSetErrorCode(mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, sMsg));
    }
    IDE_EXCEPTION(ExecuteFailAbort);
    {
        // No Action
    }
    IDE_EXCEPTION(MoveNextRecordFailIgnore);
    {
        SChar sMsg[64];

        idlOS::snprintf(sMsg, ID_SIZEOF(sMsg), "qci::moveNextRecord code=0x%x", ideGetErrorCode());

        IDE_SET(ideSetErrorCode(mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, sMsg));
    }
    IDE_EXCEPTION(MoveNextRecordFailAbort);
    {
        // No Action
    }
    IDE_EXCEPTION( RestoreResultSetValues );
    {
        sStatement->setResultSetCount(sOldResultSetHWM);
        sStatement->setEnableResultSetCount(sOldResultSetHWM);
    }
    IDE_EXCEPTION_END;
    {
        IDE_PUSH();

        // fix BUG-31195
        if (sBeginFetchTime == ID_TRUE)
        {
            sStatement->setFetchStartTime(0);
            /* BUG-19456 */
            sStatement->setFetchEndTime(0);

            IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                IDV_OPTM_INDEX_QUERY_FETCH );

            sBeginFetchTime = ID_FALSE;
        }

        if (sStatement->isStmtBegin() == ID_TRUE)
        {
            /* PROJ-2223 Altibase Auditing */
            mmtAuditManager::auditByAccess( sStatement, MMC_EXECUTION_FLAG_FAILURE );
            
            sStatement->clearStmt(MMC_STMT_BIND_NONE);

            IDE_ASSERT(sStatement->endStmt(MMC_EXECUTION_FLAG_FAILURE) == IDE_SUCCESS);
        }

        sStatement->setExecuteFlag(ID_FALSE);

        IDE_POP();
    }

    return IDE_FAILURE;
}

IDE_RC
mmtInternalSql::fetch( void * aUserContext )
{
    qciSQLFetchContext * sArg;
    mmcStatement       * sStatement;

    sArg       = (qciSQLFetchContext*)aUserContext;
    sStatement = (mmcStatement*)sArg->mmStatement;

    sArg->nextRecordExist = ID_FALSE;

    IDE_TEST_RAISE( sStatement->getStmtState() !=
                    MMC_STMT_STATE_EXECUTED,
                    NoRows );

    switch( sStatement->getResultSetState(MMC_RESULTSET_FIRST) )
    {
        case MMC_RESULTSET_STATE_INITIALIZE:
            IDE_RAISE(NoCursor);
            break;

        case MMC_RESULTSET_STATE_FETCH_READY:
            IDE_TEST(sStatement->setResultSetState(
                         MMC_RESULTSET_FIRST,
                         MMC_RESULTSET_STATE_FETCH_PROCEED) != IDE_SUCCESS);
            break;

        case MMC_RESULTSET_STATE_FETCH_PROCEED:
            break;

        default:
            IDE_RAISE(NoCursor);
            break;
    }

    sStatement->setFetchEndTime(0); /* BUG-19456 */
    sStatement->setFetchStartTime( mmtSessionManager::getBaseTime() );

    IDE_TEST_RAISE( doFetch( aUserContext ) != IDE_SUCCESS,
                    FetchError );

    /* BUG-37797 Support dequeue statement at PSM
     * Dequeue statement는 무조건 1건의 row만 가져오는데,
     * execute 할 때 1건의 row를 가져왔기 때문에 moveNextRecord를 하지 않고,
     * nextRecordExist를 ID_FALSE로 설정하여 fetch를 완료하도록 한다. */
    if( sStatement->getStmtType() != QCI_STMT_DEQUEUE )
    {
        IDE_TEST_RAISE( qci::moveNextRecord( sStatement->getQciStmt(),
                                             sStatement->getSmiStmt(),
                                             &sArg->nextRecordExist )
                        != IDE_SUCCESS,
                       FetchError);
    }
    else
    {
        sArg->nextRecordExist = ID_FALSE;
    }

    /* BUG-19456 */
    sStatement->setFetchEndTime(mmtSessionManager::getBaseTime());

    if (sArg->nextRecordExist != ID_TRUE)
    {
        IDE_TEST(fetchEnd(sStatement) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoRows);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(NoCursor);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_FETCH_ERROR));
    }
    IDE_EXCEPTION(FetchError);
    {
        IDE_PUSH();

        fetchEnd(sStatement);

        IDE_POP();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
mmtInternalSql::freeStmt( void * aUserContext )
{
    qciSQLFreeStmtContext * sArg;
    mmcStatement          * sStatement;

    sArg = (qciSQLFreeStmtContext*)aUserContext;

    sStatement = (mmcStatement*)sArg->mmStatement;

    if( sArg->freeMode == ID_TRUE )
    {
        // FREE_DROP
        IDE_TEST(sStatement->closeCursor(ID_TRUE) != IDE_SUCCESS);

        IDE_TEST(mmcStatementManager::freeStatement(sStatement) != IDE_SUCCESS);
    }
    else
    {
        // FREE_CLOSE
        IDE_TEST(sStatement->closeCursor(ID_TRUE) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
mmtInternalSql::checkBindParamCount( void * aUserContext )
{
    qciSQLCheckBindContext * sArg;
    mmcStatement           * sStatement;

    sArg = (qciSQLCheckBindContext*)aUserContext;

    sStatement = (mmcStatement*)sArg->mmStatement;


    IDE_TEST_RAISE( sStatement->getStmtState() <
                    MMC_STMT_STATE_PREPARED,
                    InvalidStatementState );


    IDE_TEST( qci::checkBindParamCount(
                  sStatement->getQciStmt(),
                  sArg->bindCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

IDE_RC
mmtInternalSql::checkBindColumnCount( void * aUserContext )
{
    qciSQLCheckBindContext * sArg;
    mmcStatement           * sStatement;

    sArg = (qciSQLCheckBindContext*)aUserContext;

    sStatement = (mmcStatement*)sArg->mmStatement;


    IDE_TEST_RAISE( sStatement->getStmtState() <
                    MMC_STMT_STATE_EXECUTED,
                    NoCursor );

    IDE_TEST( qci::checkBindColumnCount(
                  sStatement->getQciStmt(),
                  sArg->bindCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoCursor);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_FETCH_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2197 PSM Renewal */
IDE_RC
mmtInternalSql::getQciStmt( void * aUserContext )
{
    qciSQLGetQciStmtContext * sArg;
    mmcStatement            * sStatement;

    sArg = (qciSQLGetQciStmtContext*)aUserContext;

    sStatement = (mmcStatement*)sArg->mmStatement;

    sArg->mQciStatement = sStatement->getQciStmt();

    return IDE_SUCCESS;
}

// BUG-36203 PSM Optimize
IDE_RC mmtInternalSql::endFetch( void * aUserContext )
{
    qciSQLFetchEndContext * sArg;
    mmcStatement          * sStatement;

    sArg = (qciSQLFetchEndContext*)aUserContext;
    sStatement = (mmcStatement*)sArg->mmStatement;

    IDE_TEST( fetchEnd( sStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

