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

#include <ulsd.h>
#include <ulsdFailover.h>
#include <ulsdDistTxInfo.h>
#include <ulsdnEx.h>
#include <ulsdRebuild.h>
#include <ulsdUtils.h>

ACI_RC ulsdModuleHandshake_META(ulnFnContext *aFnContext)
{
    return ulsdHandshake(aFnContext);
}

SQLRETURN ulsdModuleNodeDriverConnect_META(ulnDbc       *aDbc,
                                           ulnFnContext *aFnContext,
                                           acp_char_t   *aConnString,
                                           acp_sint16_t  aConnStringLength)
{
    return ulsdNodeDriverConnect(aDbc,
                                 aFnContext,
                                 aConnString,
                                 aConnStringLength);
}

SQLRETURN ulsdModuleNodeConnect_META( ulnDbc       * aDbc,
                                      ulnFnContext * aFnContext,
                                      acp_char_t   * aServerName,
                                      acp_sint16_t   aServerNameLength,
                                      acp_char_t   * aUserName,
                                      acp_sint16_t   aUserNameLength,
                                      acp_char_t   * aPassword,
                                      acp_sint16_t   aPasswordLength )
{
    return ulsdNodeConnect( aDbc,
                            aFnContext,
                            aServerName,
                            aServerNameLength,
                            aUserName,
                            aUserNameLength,
                            aPassword,
                            aPasswordLength );
}

ACI_RC ulsdModuleEnvRemoveDbc_META(ulnEnv *aEnv, ulnDbc *aDbc)
{
    ACI_TEST(ulnEnvRemoveDbc(aEnv, aDbc) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulsdModuleStmtDestroy_META(ulnStmt *aStmt)
{
    ulsdNodeStmtDestroy(aStmt);
}

SQLRETURN ulsdModulePrepare_META(ulnFnContext *aFnContext,
                                 ulnStmt      *aStmt,
                                 acp_char_t   *aStatementText,
                                 acp_sint32_t  aTextLength,
                                 acp_char_t   *aAnalyzeText)
{
    return ulsdPrepareNodes(aFnContext,
                            aStmt,
                            aStatementText,
                            aTextLength,
                            aAnalyzeText);
}

SQLRETURN ulsdModuleExecute_META(ulnFnContext *aFnContext,
                                 ulnStmt      *aStmt)
{
    ulsdDbc      *sShard;
    acp_uint16_t  sNodeDbcIndex;
    acp_uint16_t  sOnTransactionNodeIndex;
    acp_bool_t    sIsValidOnTransactionNodeIndex;
    SQLRETURN     sNodeResult;
    acp_uint16_t  i;

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    sNodeResult = ulsdCheckDbcSMN( aFnContext, aStmt->mParentDbc );
    ACI_TEST( sNodeResult != SQL_SUCCESS );

    /* 수행할 데이터노드를 결정함 */
    sNodeResult = ulsdNodeDecideStmt(aStmt, ULN_FID_EXECUTE);
    ACI_TEST_RAISE(!SQL_SUCCEEDED(sNodeResult), LABEL_SHARD_NODE_DECIDE_STMT_FAIL);

    if ( aStmt->mParentDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        if ( aStmt->mParentDbc->mAttrGlobalTransactionLevel == ULN_GLOBAL_TX_ONE_NODE )
        {
            /* one node tx에서 1개이상 노드가 선택된 경우 에러 */
            ACI_TEST_RAISE(aStmt->mShardStmtCxt.mNodeDbcIndexCount > 1,
                           LABEL_SHARD_MULTI_NODE_SELECTED);

            sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[0];

            /* one node tx에서 이전 노드와 다르면 에러 */
            ulsdIsValidOnTransactionNodeIndex(aStmt->mParentDbc,
                                              sNodeDbcIndex,
                                              &sIsValidOnTransactionNodeIndex);
            ACI_TEST_RAISE(sIsValidOnTransactionNodeIndex != ACP_TRUE,
                           LABEL_SHARD_NODE_INVALID_TOUCH);
        }
        else
        {
            /* Do Nothing */
        }

        /* node touch */
        for ( i = 0; i < aStmt->mShardStmtCxt.mNodeDbcIndexCount; i++ )
        {
            sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[i];

            sShard->mNodeInfo[sNodeDbcIndex]->mTouched = ACP_TRUE;
        }
    }
    else
    {
        /* Do Nothing */
        /* Autocommit on + global transaction 이면서
         * 여러 참여 노드로 실행되는 구문은
         * Server-side 로 수행되게 되어있다. (BUG-45640 ulsdCallbackAnalyzeResult) */
#if defined(DEBUG)
        if ( ulsdIsGTx( aStmt->mParentDbc->mAttrGlobalTransactionLevel ) == ACP_TRUE )
        {
            ACE_DASSERT( aStmt->mShardStmtCxt.mNodeDbcIndexCount == 1 );
        }
#endif
    }

    ulsdSetOnTransactionNodeIndex(aStmt->mParentDbc, sNodeDbcIndex);

#if 0
    /* BUG-48384 정책변경으로 LIB Session으로는 보내지 않아도 된다.
                 (Shard, Solo Clone 프로시져)
                 차후 변경될 수 있으므로 코드는 유지하자. */
    ACI_TEST( ulsdBuildClientTouchNodeArr( aFnContext,
                                           sShard,
                                           ulnStmtGetStatementType(aStmt) )
              != ACI_SUCCESS );

    for ( i = 0; i < aStmt->mShardStmtCxt.mNodeDbcIndexCount; i++ )
    {
        sNodeDbcIndex = aStmt->mShardStmtCxt.mNodeDbcIndexArr[i];
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        ulsdPropagateClientTouchNodeArrToNode( sNodeStmt->mParentDbc, sShard->mMetaDbc );
    }
#endif

    sNodeResult = ulsdExecuteNodes(aFnContext, aStmt);

    return sNodeResult;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION(LABEL_SHARD_MULTI_NODE_SELECTED)
    {
        (void)ulnError( aFnContext,
                        ulERR_ABORT_SHARD_ERROR,
                        "Shard Executor",
                        "Multi-node transaction required");
    }
    ACI_EXCEPTION(LABEL_SHARD_NODE_INVALID_TOUCH)
    {
        ulsdGetOnTransactionNodeIndex(aStmt->mParentDbc, &sOnTransactionNodeIndex);

        (void)ulnError( aFnContext,
                        ulERR_ABORT_SHARD_NODE_INVALID_TOUCH,
                        sShard->mNodeInfo[sOnTransactionNodeIndex]->mNodeName,
                        sShard->mNodeInfo[sOnTransactionNodeIndex]->mServerIP,
                        sShard->mNodeInfo[sOnTransactionNodeIndex]->mPortNo,
                        sShard->mNodeInfo[sNodeDbcIndex]->mNodeName,
                        sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
                        sShard->mNodeInfo[sNodeDbcIndex]->mPortNo);

        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail: %"ACI_INT32_FMT,
            "ulsdModuleExecute", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleFetch_META(ulnFnContext *aFnContext,
                               ulnStmt      *aStmt)
{
    SQLRETURN     sNodeResult;

    /* 수행한 데이터노드를 가져옴 */
    sNodeResult = ulsdNodeDecideStmt(aStmt, ULN_FID_FETCH);
    ACI_TEST_RAISE(!SQL_SUCCEEDED(sNodeResult), LABEL_SHARD_NODE_DECIDE_STMT_FAIL);

    sNodeResult = ulsdFetchNodes(aFnContext, aStmt);

    return sNodeResult;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModuleFetch", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleCloseCursor_META(ulnStmt *aStmt)
{
    ulsdDbc  *sShard;

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    return ulsdNodeSilentCloseCursor(sShard, aStmt);
}

SQLRETURN ulsdModuleRowCount_META(ulnFnContext *aFnContext,
                                  ulnStmt      *aStmt,
                                  ulvSLen      *aRowCount)
{
    SQLRETURN     sNodeResult;

    /* 수행한 데이터노드를 가져옴 */
    sNodeResult = ulsdNodeDecideStmt(aStmt, ULN_FID_ROWCOUNT);
    ACI_TEST_RAISE(!SQL_SUCCEEDED(sNodeResult), LABEL_SHARD_NODE_DECIDE_STMT_FAIL);

    sNodeResult = ulsdRowCountNodes(aFnContext, aStmt, aRowCount);

    return sNodeResult;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModuleRowCount", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleMoreResults_META(ulnFnContext *aFnContext,
                                     ulnStmt      *aStmt)
{
    SQLRETURN     sNodeResult;

    /* 수행한 데이터노드를 가져옴 */
    sNodeResult = ulsdNodeDecideStmt(aStmt, ULN_FID_MORERESULTS);
    ACI_TEST_RAISE(!SQL_SUCCEEDED(sNodeResult), LABEL_SHARD_NODE_DECIDE_STMT_FAIL);

    sNodeResult = ulsdMoreResultsNodes(aFnContext, aStmt);

    return sNodeResult;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModuleMoreResults", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

ulnStmt* ulsdModuleGetPreparedStmt_META(ulnStmt *aStmt)
{
    ulsdDbc      *sShard;
    ulnStmt      *sStmt = NULL;
    acp_uint16_t  i;

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    /* execute후에 호출한 경우 직전에 execute한 stmt를 넘겨준다. */
    if ( ( aStmt->mShardStmtCxt.mNodeDbcIndexCur == -1 ) ||
         ( aStmt->mShardStmtCxt.mNodeDbcIndexCount <= 0 ) )
    {
        for ( i = 0; i < sShard->mNodeCount; i++ )
        {
            if ( ulnStmtIsPrepared(aStmt->mShardStmtCxt.mShardNodeStmt[i]) == ACP_TRUE )
            {
                sStmt = aStmt->mShardStmtCxt.mShardNodeStmt[i];
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        i = aStmt->mShardStmtCxt.mNodeDbcIndexArr[0];

        sStmt = aStmt->mShardStmtCxt.mShardNodeStmt[i];
    }

    return sStmt;
}

void ulsdModuleOnCmError_META(ulnFnContext     *aFnContext,
                              ulnDbc           *aDbc,
                              ulnErrorMgr      *aErrorMgr)
{
    ulsdDbc      *sShard = NULL;

    (void)ulsdFODoSTF(aFnContext, aDbc, aErrorMgr);

    if ( aDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        sShard = aDbc->mShardDbcCxt.mShardDbc;
        ulsdSetTouchedToAllNodes( sShard );
    }

    return;
}

ACI_RC ulsdModuleUpdateNodeList_META(ulnFnContext  *aFnContext,
                                     ulnDbc        *aDbc)
{
    ACI_RC sRc;

    /* BUG-47131 Stop shard META DBC failover. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_ON_STATE );

    sRc = ulsdUpdateNodeList(aFnContext, &(aDbc->mPtContext));
    ACI_TEST( sRc != ACI_SUCCESS );

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;

    ACI_EXCEPTION_END;

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;
}

ACI_RC ulsdModuleNotifyFailOver_META( ulnDbc *aDbc )
{
    ACI_RC sRc;

    /* BUG-47131 Stop shard META DBC failover. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_ON_STATE );

    sRc = ulsdNotifyFailoverOnMeta( aDbc );
    ACI_TEST( sRc != ACI_SUCCESS );

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;

    ACI_EXCEPTION_END;

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;
}

void ulsdModuleAlignDataNodeConnection_META( ulnFnContext * aFnContext,
                                             ulnDbc       * aNodeDbc )
{
    ulsdAlignDataNodeConnection( aFnContext,
                                 aNodeDbc );
}

void ulsdModuleErrorCheckAndAlignDataNode_META( ulnFnContext * aFnContext )
{
    ulsdErrorCheckAndAlignDataNode( aFnContext );
}

acp_bool_t ulsdModuleHasNoData_META( ulnStmt * aStmt )
{
    return ulsdStmtHasNoDataOnNodes( aStmt );
}

/*
 * PROJ-2739 Client-side Sharding LOB
 */
SQLRETURN ulsdModuleGetLobLength_META( ulnFnContext * aFnContext,
                                       ulnStmt      * aStmt,
                                       acp_uint64_t   aLocator,
                                       acp_sint16_t   aLocatorType,
                                       acp_uint32_t * aLengthPtr )
{
    /* 수행한 데이터노드를 가져옴 */
    ACI_TEST_RAISE( ulsdNodeDecideStmt(aStmt, ULN_FID_GETLOBLENGTH)
                    != SQL_SUCCESS, LABEL_SHARD_NODE_DECIDE_STMT_FAIL );

    ACI_TEST( ulsdGetLobLengthNodes( aFnContext,
                                     aStmt,
                                     aLocator,
                                     aLocatorType,
                                     aLengthPtr )
              != SQL_SUCCESS );

    return ULN_FNCONTEXT_GET_RC(aFnContext);;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModuleGetLobLength", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleGetLob_META( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt,
                                 acp_sint16_t   aLocatorCType,
                                 acp_uint64_t   aSrcLocator,
                                 acp_uint32_t   aFromPosition,
                                 acp_uint32_t   aForLength,
                                 acp_sint16_t   aTargetCType,
                                 void         * aBuffer,
                                 acp_uint32_t   aBufferSize,
                                 acp_uint32_t * aLengthWritten )
{
    /* 수행한 데이터노드를 가져옴 */
    ACI_TEST_RAISE( ulsdNodeDecideStmt(aStmt, ULN_FID_GETLOB)
                    != SQL_SUCCESS, LABEL_SHARD_NODE_DECIDE_STMT_FAIL );

    ACI_TEST( ulsdGetLobNodes( aFnContext,
                               aStmt,
                               aLocatorCType,
                               aSrcLocator,
                               aFromPosition,
                               aForLength,
                               aTargetCType,
                               aBuffer,
                               aBufferSize,
                               aLengthWritten )
              != SQL_SUCCESS );

    return ULN_FNCONTEXT_GET_RC(aFnContext);;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModuleGetLob", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModulePutLob_META( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt,
                                 acp_sint16_t   aLocatorCType,
                                 acp_uint64_t   aLocator,
                                 acp_uint32_t   aFromPosition,
                                 acp_uint32_t   aForLength,
                                 acp_sint16_t   aSourceCType,
                                 void         * aBuffer,
                                 acp_uint32_t   aBufferSize )
{
    /* 수행한 데이터노드를 가져옴 */
    ACI_TEST_RAISE( ulsdNodeDecideStmt(aStmt, ULN_FID_PUTLOB)
                    != SQL_SUCCESS, LABEL_SHARD_NODE_DECIDE_STMT_FAIL );

    ACI_TEST( ulsdPutLobNodes( aFnContext,
                               aStmt,
                               aLocatorCType,
                               aLocator,
                               aFromPosition,
                               aForLength,
                               aSourceCType,
                               aBuffer,
                               aBufferSize )
              != SQL_SUCCESS );

    return ULN_FNCONTEXT_GET_RC(aFnContext);;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModulePutLob", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleFreeLob_META( ulnFnContext * aFnContext,
                                  ulnStmt      * aStmt,
                                  acp_uint64_t   aLocator)
{
    /* 수행한 데이터노드를 가져옴 */
    ACI_TEST_RAISE( ulsdNodeDecideStmt(aStmt, ULN_FID_FREELOB)
                    != SQL_SUCCESS, LABEL_SHARD_NODE_DECIDE_STMT_FAIL );

    ACI_TEST( ulsdFreeLobNodes( aFnContext,
                                aStmt,
                                aLocator )
              != SQL_SUCCESS );

    return ULN_FNCONTEXT_GET_RC(aFnContext);;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModuleFreeLob", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleTrimLob_META( ulnFnContext * aFnContext,
                                  ulnStmt      * aStmt,
                                  acp_sint16_t   aLocatorCType,
                                  acp_uint64_t   aLocator,
                                  acp_uint32_t   aStartOffset )
{
    /* 수행한 데이터노드를 가져옴 */
    ACI_TEST_RAISE( ulsdNodeDecideStmt(aStmt, ULN_FID_TRIMLOB)
                    != SQL_SUCCESS, LABEL_SHARD_NODE_DECIDE_STMT_FAIL );

    ACI_TEST( ulsdTrimLobNodes( aFnContext,
                                aStmt,
                                aLocatorCType,
                                aLocator,
                                aStartOffset )
              != SQL_SUCCESS );

    return ULN_FNCONTEXT_GET_RC(aFnContext);;

    ACI_EXCEPTION(LABEL_SHARD_NODE_DECIDE_STMT_FAIL)
    {
        return SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail: %"ACI_INT32_FMT,
                  "ulsdModuleTrimLob", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

ulsdModule gShardModuleMETA =
{
    ulsdModuleHandshake_META,
    ulsdModuleNodeDriverConnect_META,
    ulsdModuleNodeConnect_META,
    ulsdModuleEnvRemoveDbc_META,
    ulsdModuleStmtDestroy_META,
    ulsdModulePrepare_META,
    ulsdModuleExecute_META,
    ulsdModuleFetch_META,
    ulsdModuleCloseCursor_META,
    ulsdModuleRowCount_META,
    ulsdModuleMoreResults_META,
    ulsdModuleGetPreparedStmt_META,
    ulsdModuleOnCmError_META,
    ulsdModuleUpdateNodeList_META,
    ulsdModuleNotifyFailOver_META,
    ulsdModuleAlignDataNodeConnection_META,
    ulsdModuleErrorCheckAndAlignDataNode_META,
    ulsdModuleHasNoData_META,

    /* PROJ-2739 Client-side Sharding LOB */
    ulsdModuleGetLobLength_META,
    ulsdModuleGetLob_META,
    ulsdModulePutLob_META,
    ulsdModuleFreeLob_META,
    ulsdModuleTrimLob_META
};
