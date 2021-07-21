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
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnStateMachine.h>
#include <mtcc.h>

#include <ulsd.h>
#include <ulsdFailover.h>
#include <ulsdCommunication.h>
#include <ulsdnExecute.h>
#include <ulsdDistTxInfo.h>
#include <ulsdRebuild.h>

/* BUG-47459 */
#define SAVEPOINT_FOR_SHARD_CLONE_PROC_PARTIAL_ROLLBACK ("$$SHARD_CLONE_PROC_PARTIAL_ROLLBACK")
#define ULN_SHARD_NONE_PARTIAL_ROLLBACK       0
#define ULN_SHARD_STMT_PARTIAL_ROLLBACK       1
#define ULN_SHARD_CLONE_PROC_PARTIAL_ROLLBACK 2

static acp_uint32_t ulsdGetShardPartialRollbackIsNeedCursorClose( ulnStmt * aStmt )
{
    acp_bool_t   sRet              = ACP_FALSE;
    acp_uint32_t sStatementType    = ulnStmtGetStatementType( aStmt );
    acp_uint8_t  sAutoCommit       = aStmt->mParentDbc->mAttrAutoCommit;

    if ( sAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
         switch ( sStatementType )
         {
             case ULN_STMT_SELECT:
             case ULN_STMT_SELECT_FOR_UPDATE:
                 sRet = ACP_TRUE;
                 break;

             default:
                 break;
         }
    }
    else
    {
        /* Nothing to do */
    }

    return sRet;
}

static acp_uint32_t ulsdGetShardPartialRollbackType( ulnStmt * aStmt )
{
    acp_uint32_t sReturnType       = ULN_SHARD_NONE_PARTIAL_ROLLBACK;
    acp_uint32_t sStatementType    = ulnStmtGetStatementType( aStmt );
    acp_uint8_t  sAutoCommit       = aStmt->mParentDbc->mAttrAutoCommit;
    acp_uint8_t  sShardSplitMethod = aStmt->mShardStmtCxt.mShardSplitMethod;

    if ( sAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
         switch ( sStatementType )
         {
             case ULN_STMT_INSERT:
             case ULN_STMT_UPDATE:
             case ULN_STMT_DELETE:
             case ULN_STMT_SELECT_FOR_UPDATE:
                 sReturnType = ULN_SHARD_STMT_PARTIAL_ROLLBACK;
                 break;

             case ULN_STMT_EXEC_PROC:
                 switch ( sShardSplitMethod )
                 {
                     case ULN_SHARD_SPLIT_CLONE:
                         sReturnType = ULN_SHARD_CLONE_PROC_PARTIAL_ROLLBACK;
                         break;

                     default:
                         break;
                 }

             default:
                 break;
         }
    }
    else
    {
        /* Nothing to do */
    }

    return sReturnType;
}

static acp_rc_t ulsdShardPartialRollbackToSavepoint( ulnFnContext      * aFnContext,
                                                     ulsdDbc           * aShard,
                                                     ulsdFuncCallback  * aCallback )
{
    ulsdFuncCallback * sCallback = aCallback;
    acp_rc_t           sRet      = ACI_SUCCESS;

    while ( sCallback != NULL )
    {
        switch( sCallback->mRet )
        {
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
            case SQL_NO_DATA:
                if ( ulnRollbackToSavepoint( sCallback->mStmt->mParentDbc,
                                             SAVEPOINT_FOR_SHARD_CLONE_PROC_PARTIAL_ROLLBACK,
                                             acpCStrLen( SAVEPOINT_FOR_SHARD_CLONE_PROC_PARTIAL_ROLLBACK, ACP_SINT32_MAX ) )
                     != SQL_SUCCESS )
                {
                    ulsdNativeErrorToUlnError( aFnContext,
                                               SQL_HANDLE_DBC,
                                               (ulnObject*)sCallback->mStmt->mParentDbc,
                                               aShard->mNodeInfo[sCallback->mIndex],
                                               (acp_char_t *)"ShardPartialRollbackToSavepointCloneProc" );

                    sRet = ACI_FAILURE;
                }
                break;

            default:
                break;
        }

        sCallback = sCallback->mNext;
    }

    return sRet;
}

static acp_rc_t ulsdShardStmtPartialRollback( ulnFnContext      * aFnContext,
                                              ulsdDbc           * aShard,
                                              ulsdFuncCallback  * aCallback )
{
    ulsdFuncCallback * sCallback = aCallback;
    acp_rc_t           sRet = ACI_SUCCESS;

    while ( sCallback != NULL )
    {
        switch( sCallback->mRet )
        {
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
            case SQL_NO_DATA:
                if ( ulsdnStmtShardStmtPartialRollback( sCallback->mStmt->mParentDbc ) != SQL_SUCCESS )
                {
                    ulsdNativeErrorToUlnError( aFnContext,
                                               SQL_HANDLE_DBC,
                                               (ulnObject*)sCallback->mStmt->mParentDbc,
                                               aShard->mNodeInfo[sCallback->mIndex],
                                               (acp_char_t *)"ShardStmtPartialRollback" );

                    sRet = ACI_FAILURE;
                }
                break;

            default:
                break;
        }
        
        sCallback = sCallback->mNext;
    }

    return sRet;
}

void ulsdProcessExecuteError( ulnFnContext       * aFnContext,
                              ulnStmt            * aStmt,
                              ulsdFuncCallback   * aCallback )
{
    ulsdNodeReport   sReport;
    ulsdDbc        * sShard       = NULL;
    SQLRETURN        sBackupSqlRC = SQL_SUCCESS;
    acp_rc_t         sRet         = ACI_SUCCESS;

    if ( ulsdGetShardPartialRollbackIsNeedCursorClose( aStmt ) == ACP_TRUE )
    {
        sRet = ulsdNodeCloseCursor( aStmt );
        ACI_TEST_CONT( SQL_SUCCEEDED( sRet ) == 0,
                       NODE_REPORT );
    }

    /* BUG-47459 */
    switch ( ulsdGetShardPartialRollbackType( aStmt ) )
    {
        case ULN_SHARD_STMT_PARTIAL_ROLLBACK:
            ulsdGetShardFromDbc( aStmt->mParentDbc, &sShard );

            sRet = ulsdShardStmtPartialRollback( aFnContext,
                                                 sShard,
                                                 aCallback );
            break;

        case ULN_SHARD_CLONE_PROC_PARTIAL_ROLLBACK:
            ulsdGetShardFromDbc( aStmt->mParentDbc, &sShard );

            sRet = ulsdShardPartialRollbackToSavepoint( aFnContext,
                                                        sShard,
                                                        aCallback );
            break;

        default:
            break;
    }

ACI_EXCEPTION_CONT( NODE_REPORT );

    if ( SQL_SUCCEEDED( sRet ) == 0 )
    {
        if ( ulsdIsFailoverExecute( aFnContext ) == ACP_TRUE )
        {
            /* do nothing */
        }
        else
        {
            sReport.mType = ULSD_REPORT_TYPE_TRANSACTION_BROKEN;

            /* Node Report 를 정상적으로 수행하기 위하여 SQL Return 을
             * backup 했다가 Node Report 수행 이후에 복구 한다.
             */
            sBackupSqlRC = ULN_FNCONTEXT_GET_RC( aFnContext );
            ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );
            sShard->mTouched = ACP_TRUE;
            if ( ulsdShardNodeReport( aFnContext, &sReport ) != ACI_SUCCESS )
            {
                if ( ulsdIsFailoverExecute( aFnContext ) == ACP_TRUE )
                {
                    /* do nothing */
                }
                else
                {
                    ulnError( aFnContext,
                              ulERR_ABORT_SHARD_INTERNAL_ERROR,
                              "ShardNodeReport(Transaction Broken) fails" );
                }
            }

            ULN_FNCONTEXT_SET_RC( aFnContext, sBackupSqlRC );
        }
    }
}

SQLRETURN ulsdFetch( ulnStmt *aStmt )
{
    /* PROJ-2598 altibase sharding */
    SQLRETURN     sRet = SQL_ERROR;
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FETCH, aStmt, ULN_OBJ_TYPE_STMT);

    /* BUG-47553 */
    ACI_TEST_RAISE( ulsdEnter( &sFnContext ) != ACI_SUCCESS, LABEL_ENTER_ERROR );

    sRet = ulsdModuleFetch(&sFnContext, aStmt);

    return sRet;

    ACI_EXCEPTION( LABEL_ENTER_ERROR )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION_END;

    return sRet;
}

SQLRETURN ulsdFetchNodes(ulnFnContext *aFnContext,
                         ulnStmt      *aStmt)
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult = SQL_ERROR;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt;
    acp_uint16_t       sNodeDbcIndex;
    acp_uint16_t       i;

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    ACI_TEST_RAISE( aStmt->mShardStmtCxt.mNodeDbcIndexCur == -1, LABEL_NOT_EXECUTED );

    while ( 1 )
    {
        i = aStmt->mShardStmtCxt.mNodeDbcIndexCur;

        if ( i < aStmt->mShardStmtCxt.mNodeDbcIndexCount )
        {
            sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[i];
            sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

            sNodeResult = ulnFetch(sNodeStmt);

            SHARD_LOG("(Fetch) NodeId=%d, Server=%s:%d, StmtID=%d\n",
                      sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
                      sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
                      sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
                      sNodeStmt->mStatementID);

            if ( sNodeResult == SQL_SUCCESS )
            {
                /* Nothing to do */
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                /* info의 전달 */
                ulsdNativeErrorToUlnError(aFnContext,
                                          SQL_HANDLE_STMT,
                                          (ulnObject *)sNodeStmt,
                                          sShard->mNodeInfo[sNodeDbcIndex],
                                          (acp_char_t *)"Fetch");

                sSuccessResult = sNodeResult;
            }
            else if ( sNodeResult == SQL_NO_DATA )
            {
                /* next 데이터 노드를 확인해야 함 */
                aStmt->mShardStmtCxt.mNodeDbcIndexCur++;
                continue;
            }
            else
            {
                sErrorResult = sNodeResult;

                ACI_RAISE(LABEL_NODE_FETCH_FAIL);
            }
        }
        else
        {
            /* 모든 데이터 노드에서 no_data이면 no_data 리턴 */
            sSuccessResult = SQL_NO_DATA;
        }

        break;
    }

    return sSuccessResult;

    ACI_EXCEPTION(LABEL_NOT_EXECUTED)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Not executed");
    }
    ACI_EXCEPTION(LABEL_NODE_FETCH_FAIL)
    {
        ulsdNativeErrorToUlnError(aFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)sNodeStmt,
                                  sShard->mNodeInfo[sNodeDbcIndex],
                                  (acp_char_t *)"Fetch");
    }
    ACI_EXCEPTION_END;

    return sErrorResult;
}

/* BUG-47433 */
SQLRETURN ulsdGetData( ulnStmt      * aStmt,
                       acp_uint16_t   aColumnNumber,
                       acp_sint16_t   aTargetType,
                       void         * aTargetValue,
                       ulvSLen        aBufferLength,
                       ulvSLen      * aStrLenOrInd )
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;
    acp_sint16_t   sNodeDbcIndex = -1;
    ulnStmt      * sNodeStmt = NULL;
    ulsdDbc      * sShard = NULL;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_GETDATA, aStmt, ULN_OBJ_TYPE_STMT );

    /* BUG-47553 */
    ACI_TEST_RAISE( ulsdEnter( &sFnContext ) != ACI_SUCCESS, LABEL_ENTER_ERROR );

    sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexCur;

    if ( sNodeDbcIndex == -1 )
    {
        // Not Fetched
        sRet = ulnGetData( aStmt,
                           aColumnNumber,
                           aTargetType,
                           aTargetValue,
                           aBufferLength,
                           aStrLenOrInd );
    }
    else
    {
        // Fetched
        ACI_TEST_RAISE( sNodeDbcIndex >= aStmt->mShardStmtCxt.mNodeDbcIndexCount, LABEL_INVALID_CURSOR_POSITION_AE_OR_BS );

        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        sRet = ulnGetData( sNodeStmt,
                           aColumnNumber,
                           aTargetType,
                           aTargetValue,
                           aBufferLength,
                           aStrLenOrInd );

        ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_NODE_GETDATA_FAIL );
    }

    return sRet;

    ACI_EXCEPTION( LABEL_ENTER_ERROR )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION( LABEL_INVALID_CURSOR_POSITION_AE_OR_BS )
    {
        ulnError( &sFnContext, ulERR_ABORT_INVALID_CURSOR_POSITION_GD );
    }
    ACI_EXCEPTION( LABEL_NODE_GETDATA_FAIL )
    {
        ulsdGetShardFromDbc( aStmt->mParentDbc, &sShard );

        ulsdNativeErrorToUlnError( &sFnContext,
                                   SQL_HANDLE_STMT,
                                   (ulnObject *)sNodeStmt,
                                   sShard->mNodeInfo[sNodeDbcIndex],
                                   (acp_char_t *)"GetData" );
    }
    ACI_EXCEPTION_END;

    return sRet;
}

SQLRETURN ulsdExecute( ulnFnContext * aFnContext,
                       ulnStmt      * aStmt )
{
    /* PROJ-2598 altibase sharding */
    SQLRETURN     sNodeResult;

    sNodeResult = ulsdCheckStmtSMN( aFnContext, aStmt );
    ACI_TEST( !SQL_SUCCEEDED( sNodeResult ) );

    sNodeResult = ulsdModuleExecute(aFnContext, aStmt);

    // PROJ-2727
    if ( ulnStmtGetStatementType( aStmt ) == ULN_STMT_SET_SESSION_PROPERTY )
    {
        if (( sNodeResult == SQL_SUCCESS ) ||
            ( sNodeResult == SQL_NO_DATA_FOUND ))
        {
            if ( aStmt->mParentDbc->mAttributeCStr == NULL )
            {
                ACI_TEST( ulsdSetConnectAttr( aStmt->mParentDbc,
                                              aStmt->mParentDbc->mAttributeCID,
                                              (SQLPOINTER)aStmt->mParentDbc->mAttributeCVal,
                                              0)
                          != ACI_SUCCESS);
            }
            else
            {
                ACI_TEST( ulsdSetConnectAttr( aStmt->mParentDbc,
                                              aStmt->mParentDbc->mAttributeCID,
                                              (SQLPOINTER)aStmt->mParentDbc->mAttributeCStr,
                                              aStmt->mParentDbc->mAttributeCLen )
                          != ACI_SUCCESS);
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }
    
    return sNodeResult;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdExecuteNodes(ulnFnContext *aFnContext,
                           ulnStmt      *aStmt)
{
    SQLRETURN          sNodeResult    = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult   = SQL_ERROR;
    acp_bool_t         sSuccess       = ACP_TRUE;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt;
    acp_uint16_t       sNodeDbcIndex;
    ulsdFuncCallback  *sCallback = NULL;
    ulnResult         *sResult   = NULL;
    acp_uint16_t       sNoDataCount = 0;
    acp_uint16_t       i;
    acp_uint64_t       sAffectedRowCount = 0;

    /* TASK-7219 Non-shard DML */
    acp_uint32_t       sStmtExecSeqForShardTx = 0;
    acp_bool_t         sStmtExecSeqIncreased  = ACP_FALSE;

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    aStmt->mShardStmtCxt.mRowCount = 0;

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    for ( i = 0; i < aStmt->mShardStmtCxt.mNodeDbcIndexCount; i++ )
    {
        sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[i];
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        sErrorResult = ulsdCheckDbcSMN( aFnContext, sNodeStmt->mParentDbc );
        ACI_TEST( sErrorResult != SQL_SUCCESS );

        sErrorResult = ulsdCheckStmtSMN( aFnContext, sNodeStmt );
        ACI_TEST( sErrorResult != SQL_SUCCESS );
    }

    /* PROJ-2733-DistTxInfo DistTxInfo를 계산해서 Meta DBC에 저장 */
    ulsdCalcDistTxInfoForMeta(sShard->mMetaDbc,
                              aStmt->mShardStmtCxt.mNodeDbcIndexCount,
                              aStmt->mShardStmtCxt.mNodeDbcIndexArr);

    /* TASK-7219 Non-shard DML */
    if ( sShard->mMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        /* metaDbc의 execution sequence 를 증가시킨다. */
        ulnDbcIncreaseStmtExecSeqForShardTx(sShard->mMetaDbc);
        sStmtExecSeqIncreased = ACP_TRUE;

        sStmtExecSeqForShardTx = ulnDbcGetExecSeqForShardTx(sShard->mMetaDbc);
        ACI_TEST_RAISE( sStmtExecSeqForShardTx > ULN_DBC_SHARD_STMT_EXEC_SEQ_MAX, ERR_SHARD_STMT_SEQUENCE_OVERFLOW );
    }

    for ( i = 0; i < aStmt->mShardStmtCxt.mNodeDbcIndexCount; i++ )
    {
        sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[i];
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];
        sNodeStmt->mShardStmtCxt.mMyNodeDbcIndexCur = i; // PROJ-2739

        /* PROJ-2733-DistTxInfo Meta DBC에 설정된 분산정보를 각 노드로 전파한다. */
        ulsdPropagateDistTxInfoToNode(sNodeStmt->mParentDbc, sShard->mMetaDbc);

        /* TASK-7219 Non-shard DML Meta DBC에 설정된 execution sequence를 각 노드의 DBC로 전파한다. */
        if ( sShard->mMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
        {
            ulnDbcSetExecSeqForShardTx(sNodeStmt->mParentDbc, sStmtExecSeqForShardTx);
        }

        sErrorResult = ulsdExecuteAddCallback( sNodeDbcIndex,
                                               sNodeStmt,
                                               &sCallback );
        ACI_TEST( sErrorResult != SQL_SUCCESS );
    }

    /* node execute 병렬수행 */
    ulsdDoCallback( sCallback );

    for ( i = 0; i < aStmt->mShardStmtCxt.mNodeDbcIndexCount; i++ )
    {
        sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[i];
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        sNodeResult = ulsdGetResultCallback( sNodeDbcIndex, sCallback, (acp_uint8_t)0 );

        SHARD_LOG("(Execute) NodeId=%d, Server=%s:%d, StmtID=%d\n",
                  sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
                  sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
                  sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
                  sNodeStmt->mStatementID);

        if ( sNodeResult == SQL_SUCCESS )
        {
            /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
            sResult = ulnStmtGetCurrentResult( sNodeStmt );
            sAffectedRowCount += ulnResultGetAffectedRowCount( sResult );
        }
        else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
        {
            /* info의 전달 */
            ulsdNativeErrorToUlnError(aFnContext,
                                      SQL_HANDLE_STMT,
                                      (ulnObject *)sNodeStmt,
                                      sShard->mNodeInfo[sNodeDbcIndex],
                                      (acp_char_t *)"Execute");

            sSuccessResult = sNodeResult;

            /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
            sResult = ulnStmtGetCurrentResult( sNodeStmt );
            sAffectedRowCount += ulnResultGetAffectedRowCount( sResult );
        }
        else if ( sNodeResult == SQL_NO_DATA )
        {
            sNoDataCount++;
        }
        else
        {
            /* BUGBUG 여러개의 데이터 노드에 대해서 retry가 가능한가? */
            if ( ulsdNodeFailConnectionLost( SQL_HANDLE_STMT,
                                             (ulnObject *)sNodeStmt ) == ACP_TRUE )
            {
                if ( aStmt->mParentDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
                {
                    ulsdClearOnTransactionNode(aStmt->mParentDbc);
                }
                else
                {
                    /* Do Nothing */
                }
            }

            ulsdNativeErrorToUlnError(aFnContext,
                                      SQL_HANDLE_STMT,
                                      (ulnObject *)sNodeStmt,
                                      sShard->mNodeInfo[sNodeDbcIndex],
                                      (acp_char_t *)"Execute");

            sErrorResult = sNodeResult;
            sSuccess = ACP_FALSE;
        }
    }

    /* 모두 no_data이면 no_data 리턴, 에러가 하나라도 있으면 에러 리턴 */
    if ( sNoDataCount == aStmt->mShardStmtCxt.mNodeDbcIndexCount )
    {
        sSuccessResult = SQL_NO_DATA;
    }
    else
    {
        ACI_TEST_RAISE( sSuccess == ACP_FALSE, ERR_EXECUTE_FAIL );
        aStmt->mShardStmtCxt.mRowCount = sAffectedRowCount;
    }

    /* execute가 성공하고, select인 경우 fetch할 준비가 됨 */
    aStmt->mShardStmtCxt.mNodeDbcIndexCur = 0;

    /* PROJ-2739 Client-side Sharding LOB */
    if ( aStmt->mShardStmtCxt.mHasLocatorParamToCopy == ACP_TRUE )
    {
        sNodeResult = ulsdLobCopy( aFnContext,
                                   sShard,
                                   aStmt );
        if ( sNodeResult != SQL_ERROR )
        {
            sSuccessResult = sNodeResult;
        }
        else
        {
            sErrorResult = sNodeResult;
            ACI_RAISE( ERR_EXECUTE_FAIL );
        }
    }
    else
    {
        /* Nothing to do */
    }

    ulsdRemoveCallback( sCallback );

    return sSuccessResult;

    ACI_EXCEPTION( ERR_EXECUTE_FAIL )
    {
        ulsdProcessExecuteError( aFnContext, aStmt, sCallback );
    }
    ACI_EXCEPTION( ERR_SHARD_STMT_SEQUENCE_OVERFLOW )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_SHARD_INTERNAL_ERROR,
                  "Shard statement execution sequence overflow" );
    }
    ACI_EXCEPTION_END;

    /* TASK-7219 Non-shard DML */
    if ( sStmtExecSeqIncreased == ACP_TRUE )
    {
        ulnDbcDecreaseStmtExecSeqForShardTx(sShard->mMetaDbc);
    }

    ulsdRemoveCallback( sCallback );

    return sErrorResult;
}

SQLRETURN ulsdExecuteAndRetry( ulnFnContext * aFnContext,
                               ulnStmt      * aStmt )
{
    SQLRETURN    sExecuteRet = SQL_ERROR;
    SQLRETURN    sPrepareRet = SQL_ERROR;
    acp_sint32_t sRetryMax   = ulnDbcGetShardStatementRetry( aStmt->mParentDbc );
    acp_sint32_t sLoopMax    = ULSD_SHARD_RETRY_LOOP_MAX; /* For prohibit infinity loop */
    ulsdStmtShardRetryType   sRetryType;

    sExecuteRet = ulsdExecute( aFnContext, aStmt );

    /* BUGBUG retrun result not matched with FNCONTEXT result */
    ULN_FNCONTEXT_SET_RC( aFnContext, sExecuteRet );

    while ( SQL_SUCCEEDED( sExecuteRet ) == 0 )
    {
        ACI_TEST( (sLoopMax--) <= 0 );

        ACI_TEST( ulsdProcessShardRetryError( aFnContext,
                                              aStmt,
                                              &sRetryType,
                                              &sRetryMax )
                  != SQL_SUCCESS );

        switch ( sRetryType )
        {
            case ULSD_STMT_SHARD_REBUILD_RETRY :
            case ULSD_STMT_SHARD_SMN_PROPAGATION :
                sPrepareRet = ulsdAnalyzePrepareAndRetry( aStmt,
                                                          aStmt->mShardStmtCxt.mOrgPrepareTextBuf,
                                                          aStmt->mShardStmtCxt.mOrgPrepareTextBufLen,
                                                          NULL );
                ULN_FNCONTEXT_SET_RC( aFnContext, sPrepareRet );
                ACI_TEST( !SQL_SUCCEEDED( sPrepareRet ) );
                break;

            default:
                break;
        }

        sExecuteRet = ulsdExecute( aFnContext, aStmt );

        /* BUGBUG retrun result not matched with FNCONTEXT result */
        ULN_FNCONTEXT_SET_RC( aFnContext, sExecuteRet );
    }

    /* PROJ-2745
     * SMN 변경이 현재 Statement 수행에 영향을 미치지 않으면
     * Rebuild retry error 가 발생하지 않고
     * 대신 SMN 을 올리라는 Rebuild Noti 로 수신되어
     * ulnDbcGetTargetShardMetaNumber() 값이 상승한다.
     */
    if ( ulsdCheckRebuildNoti( aStmt->mParentDbc ) == ACP_TRUE )
    {
        ulsdUpdateShardSession_Silent( aStmt->mParentDbc,
                                       aFnContext );
    }

    return ULN_FNCONTEXT_GET_RC( aFnContext );

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

SQLRETURN ulsdRowCount(ulnStmt *aStmt,
                       ulvSLen *aRowCountPtr)
{
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_ROWCOUNT, aStmt, ULN_OBJ_TYPE_STMT);

    /* BUG-47553 */
    ACI_TEST( ulsdEnter( &sFnContext ) != ACI_SUCCESS );

    return ulsdModuleRowCount(&sFnContext, aStmt, aRowCountPtr);

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdRowCountNodes(ulnFnContext *aFnContext,
                            ulnStmt      *aStmt,
                            ulvSLen      *aRowCount)
{
    ACI_TEST_RAISE( aRowCount == NULL, LABEL_INVALID_NULL );

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    *aRowCount = (ulvSLen)aStmt->mShardStmtCxt.mRowCount;

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_INVALID_NULL )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

SQLRETURN ulsdMoreResults(ulnStmt *aStmt)
{
    SQLRETURN    sRet = SQL_ERROR;
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_MORERESULTS, aStmt, ULN_OBJ_TYPE_STMT );

    /* BUG-47553 */
    ACI_TEST_RAISE( ulsdEnter( &sFnContext ) != ACI_SUCCESS, LABEL_ENTER_ERROR );

    sRet = ulsdModuleMoreResults(&sFnContext, aStmt);

    return sRet;

    ACI_EXCEPTION( LABEL_ENTER_ERROR )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION_END;

    return sRet;
}

SQLRETURN ulsdMoreResultsNodes(ulnFnContext *aFnContext,
                               ulnStmt      *aStmt)
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult = SQL_ERROR;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt;
    acp_uint16_t       sNodeDbcIndex;
    acp_uint16_t       sNoDataCount = 0;
    acp_uint16_t       i;

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    for ( i = 0; i < aStmt->mShardStmtCxt.mNodeDbcIndexCount; i++ )
    {
        sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[i];
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        sNodeResult = ulnMoreResults(sNodeStmt);

        SHARD_LOG("(More Results) NodeId=%d, Server=%s:%d, StmtID=%d\n",
                  sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
                  sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
                  sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
                  sNodeStmt->mStatementID);

        if ( sNodeResult == SQL_SUCCESS )
        {
            /* Nothing to do */
        }
        else if ( sNodeResult == SQL_NO_DATA )
        {
            sNoDataCount++;
        }
        else
        {
            sErrorResult = sNodeResult;

            ACI_RAISE(LABEL_NODE_MORERESULTS_FAIL);
        }
    }

    /* 모두 no_data이면 no_data 리턴 */
    if ( sNoDataCount == aStmt->mShardStmtCxt.mNodeDbcIndexCount )
    {
        sSuccessResult = SQL_NO_DATA;
    }
    else
    {
        /* Nothing to do */
    }

    return sSuccessResult;

    ACI_EXCEPTION(LABEL_NODE_MORERESULTS_FAIL)
    {
        ulsdNativeErrorToUlnError(aFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)sNodeStmt,
                                  sShard->mNodeInfo[sNodeDbcIndex],
                                  (acp_char_t *)"MoreResults");
    }
    ACI_EXCEPTION_END;

    return sErrorResult;
}

acp_bool_t ulsdStmtHasNoDataOnNodes( ulnStmt * aMetaStmt )
{
    ulnStmt      * sNodeStmt     = NULL;
    acp_uint16_t   sNodeDbcIndex = 0;
    acp_uint16_t   i             = 0;

    for ( i = 0; i < aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount; i++ )
    {
        sNodeDbcIndex = aMetaStmt->mShardStmtCxt.mNodeDbcIndexArr[i];
        sNodeStmt = aMetaStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        if ( ulnStmtIsPrepared( sNodeStmt ) == ACP_TRUE )
        {
            ACI_TEST( ulnCursorHasNoData( ulnStmtGetCursor( sNodeStmt ) ) == ACP_FALSE );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return ACP_TRUE;

    ACI_EXCEPTION_END;

    return ACP_FALSE;
}
