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
#include <mtcc.h>

#include <ulsd.h>
#include <ulsdnExecute.h>
#include <ulsdnFailover.h>
#include <ulsdDistTxInfo.h>
#include <ulsdnTrans.h>
#include <ulsdRebuild.h>

void ulsdClearOnTransactionNode(ulnDbc *aMetaDbc)
{
    aMetaDbc->mShardDbcCxt.mShardOnTransactionNodeIndex = ACP_UINT16_MAX;
    aMetaDbc->mShardDbcCxt.mShardIsNodeTransactionBegin = ACP_FALSE;
}

void ulsdSetOnTransactionNodeIndex(ulnDbc       *aMetaDbc,
                                   acp_uint16_t  aNodeIndex)
{
    if ( aMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        aMetaDbc->mShardDbcCxt.mShardOnTransactionNodeIndex = aNodeIndex;
        aMetaDbc->mShardDbcCxt.mShardIsNodeTransactionBegin = ACP_TRUE;
    }
    else
    {
        /* Nothing to do */
    }
}

void ulsdGetOnTransactionNodeIndex(ulnDbc       *aMetaDbc,
                                   acp_uint16_t *aNodeIndex)
{
    (*aNodeIndex) = aMetaDbc->mShardDbcCxt.mShardOnTransactionNodeIndex;
}

void ulsdIsValidOnTransactionNodeIndex(ulnDbc       *aMetaDbc,
                                       acp_uint16_t  aDecidedNodeIndex,
                                       acp_bool_t   *aIsValid)
{
    /* Transaction 이 시작되지 않았고 || 현재 사용중인 Node 면 -> Valid Index */
    (*aIsValid) = (( aMetaDbc->mShardDbcCxt.mShardIsNodeTransactionBegin == ACP_FALSE) ||
                  (aDecidedNodeIndex == aMetaDbc->mShardDbcCxt.mShardOnTransactionNodeIndex)) ?
                  ACP_TRUE : ACP_FALSE;
}

SQLRETURN ulsdEndTranDbc(acp_sint16_t  aHandleType,
                         ulnDbc       *aMetaDbc,
                         acp_sint16_t  aCompletionType)
{
    SQLRETURN       sRet = SQL_ERROR;
    ulnFnContext    sFnContext;
    acp_sint16_t    sTouchNodeCount;

    ACP_UNUSED(aHandleType);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_ENDTRAN, aMetaDbc, ULN_OBJ_TYPE_DBC);

    /* BUG-47553 */
    ACI_TEST_RAISE( ulsdEnter( &sFnContext ) != ACI_SUCCESS, LABEL_ENTER_ERROR );

    switch ( aMetaDbc->mAttrGlobalTransactionLevel )
    {
        case ULN_GLOBAL_TX_ONE_NODE:
            sRet = ulsdNodeEndTranDbc( & sFnContext,
                                       aMetaDbc->mShardDbcCxt.mShardDbc,
                                       aCompletionType );
            break;

        case ULN_GLOBAL_TX_MULTI_NODE:
            sRet = ulsdTouchNodeEndTranDbc( & sFnContext,
                                            aMetaDbc->mShardDbcCxt.mShardDbc,
                                            aCompletionType );
            break;

        case ULN_GLOBAL_TX_GLOBAL:
            /* fall through */
        case ULN_GLOBAL_TX_GCTX:
            ulsdGetTouchNodeCount(aMetaDbc->mShardDbcCxt.mShardDbc,
                                  &sTouchNodeCount);

            /* BUG-45801 Global Tx Rollback을 2PC로 처리하고 있습니다. */
            if ( ( sTouchNodeCount >= 2 ) && ( aCompletionType == ULN_TRANSACT_COMMIT ) )
            {
                /* 2노드 이상이고 Commit인 경우만 global transaction을 사용한다. */
                sRet = ulsdShardEndTranDbc( & sFnContext,
                                            aMetaDbc->mShardDbcCxt.mShardDbc );
            }
            else
            {
                sRet = ulsdTouchNodeShardEndTranDbc( & sFnContext,
                                                     aMetaDbc->mShardDbcCxt.mShardDbc,
                                                     aCompletionType );
            }

            if ( sRet == SQL_SUCCESS )
            {
                ulnDbcSet2PcCommitState( aMetaDbc, ULSD_2PC_NORMAL );
            }
            else
            {
                ulnDbcSet2PcCommitState( aMetaDbc, ULSD_2PC_COMMIT_FAIL );
            }
            break;

        default:
            ACI_RAISE( LABEL_NO_SET_GLOBAL_TRANSACTION_LEVEL );
            break;
    }

    /* BUGBUG retrun result not matched with FNCONTEXT result */
    ULN_FNCONTEXT_SET_RC( &sFnContext, sRet );

    ulsdCleanupShardSession( aMetaDbc,
                             &sFnContext );

    ACI_TEST( SQL_SUCCEEDED( ULN_FNCONTEXT_GET_RC( &sFnContext ) ) == 0 );

    ulsdClearOnTransactionNode( aMetaDbc );

    /* TASK-7219 Non-shard DML */
    ulnDbcInitStmtExecSeqForShardTx(aMetaDbc);

    return ULN_FNCONTEXT_GET_RC( &sFnContext );

    ACI_EXCEPTION( LABEL_ENTER_ERROR )
    {
        /* Nothing to do */
    }
    ACI_EXCEPTION( LABEL_NO_SET_GLOBAL_TRANSACTION_LEVEL )
    {
        ulnError( &sFnContext,
                  ulERR_ABORT_SHARD_INTERNAL_ERROR,
                  "No set Global Transaction level" );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdNodeEndTranDbc(ulnFnContext      *aFnContext,
                             ulsdDbc           *aShard,
                             acp_sint16_t       aCompletionType)
{
    SQLRETURN           sRet = SQL_SUCCESS;
    acp_uint16_t        sOnTransNodeIndex;

    if ( aShard->mMetaDbc->mShardDbcCxt.mShardIsNodeTransactionBegin == ACP_TRUE )
    {
        sOnTransNodeIndex = aShard->mMetaDbc->mShardDbcCxt.mShardOnTransactionNodeIndex;

        SHARD_LOG("(EndTranDbc) OnTransNodeIndex[%d], NodeId=%d, Server=%s:%d\n",
                  sOnTransNodeIndex,
                  aShard->mNodeInfo[sOnTransNodeIndex]->mNodeId,
                  aShard->mNodeInfo[sOnTransNodeIndex]->mServerIP,
                  aShard->mNodeInfo[sOnTransNodeIndex]->mPortNo);

        sRet = ulnEndTran( SQL_HANDLE_DBC,
                           (ulnObject *)aShard->mNodeInfo[sOnTransNodeIndex]->mNodeDbc,
                           (acp_sint16_t)aCompletionType );

        if ( sRet == SQL_SUCCESS )
        {
            /* Nothing to do */
        }
        else if ( sRet == SQL_SUCCESS_WITH_INFO )
        {
            ulsdNativeErrorToUlnError( aFnContext,
                                       SQL_HANDLE_DBC,
                                       (ulnObject *)aShard->mNodeInfo[sOnTransNodeIndex]->mNodeDbc,
                                       aShard->mNodeInfo[sOnTransNodeIndex],
                                       (aCompletionType == SQL_COMMIT) ? (acp_char_t *)"Commit" :
                                                                         (acp_char_t *)"Rollback" );
        }
        else
        {
            ulsdNativeErrorToUlnError( aFnContext,
                                       SQL_HANDLE_DBC,
                                       (ulnObject *)aShard->mNodeInfo[sOnTransNodeIndex]->mNodeDbc,
                                       aShard->mNodeInfo[sOnTransNodeIndex],
                                       (aCompletionType == SQL_COMMIT) ? (acp_char_t *)"Commit" :
                                                                         (acp_char_t *)"Rollback" );

            ACI_TEST( 1 );
        }
    }
    else
    {
        /* Do Nothing */
    }

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

ACI_RC ulsdCallbackShardTransactionResult(cmiProtocolContext *aProtocolContext,
                                          cmiProtocol        *aProtocol,
                                          void               *aServiceSession,
                                          void               *aUserContext)
{
    ulnFnContext   *sFnContext = (ulnFnContext *)aUserContext;
    ulnDbc         *sDbc       = sFnContext->mHandle.mDbc;

    acp_uint64_t    sSCN = 0;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD8(aProtocolContext, &sSCN);  /* PROJ-2733-Protocol */

    /* PROJ-2733-DistTxInfo */
    if (sSCN > 0)
    {
        ulsdUpdateSCNToEnv(sDbc->mParentEnv, &sSCN);
    }

    if ( sDbc->mShardDbcCxt.mParentDbc != NULL )
    {
        ulsdInitDistTxInfo( sDbc->mShardDbcCxt.mParentDbc );
    }
    else
    {
        ulsdInitDistTxInfo( sDbc );
    }

    return ACI_SUCCESS;
}

ACI_RC ulsdCallbackShardHandshakeResult( cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aServiceSession,
                                         void               *aUserContext )
{
    ACP_UNUSED( aProtocol );
    ACP_UNUSED( aServiceSession );
    ACP_UNUSED( aUserContext );

    acp_uint8_t sVerMajor;
    acp_uint8_t sVerMinor;
    acp_uint8_t sVerPatch;
    acp_uint8_t sOptReserved;

    CMI_RD1( aProtocolContext, sVerMajor );
    CMI_RD1( aProtocolContext, sVerMinor );
    CMI_RD1( aProtocolContext, sVerPatch );
    CMI_RD1( aProtocolContext, sOptReserved );

    ACP_UNUSED( sVerMajor );
    ACP_UNUSED( sVerMinor );
    ACP_UNUSED( sVerPatch );
    ACP_UNUSED( sOptReserved );

    return ACI_SUCCESS;
}

ACI_RC ulsdShardTranDbcMain( ulnFnContext     * aFnContext,
                             ulnDbc           * aDbc,
                             ulnTransactionOp   aCompletionType,
                             acp_uint32_t     * aTouchNodeArr,
                             acp_uint16_t       aTouchNodeCount )
{
    ULN_FLAG(sNeedFinPtContext);

    ulnErrorMgr sErrorMgr;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    ulnStmt            *sStmt     = NULL;
    acp_list_node_t    *sIterator = NULL;
    cmiProtocolContext *sCtx      = NULL;

    ACI_TEST_RAISE( aDbc->mIsConnected == ACP_FALSE, LABEL_ABORT_NO_CONNECTION );

    /*
     * Note The Driver Manager does not call SQLEndTran when the connection is
     * in auto-commit mode; it simply returns SQL_SUCCESS,
     * even if the application attempts to roll back the transaction.
     *  -- from MSDN ODBC Spec - Committing and Rolling Back Transactions
     */
    /* mAttrAutoCommit is SQL_AUTOCOMMIT_ON, SQL_AUTOCOMMIT_OFF, SQL_UNDEF */
    if ( ( aDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF ) &&
         ( aDbc->mIsConnected == ACP_TRUE ) )
    {
        /*
         * protocol context 초기화
         */
        // fix BUG-17722
        ACI_TEST(ulnInitializeProtocolContext(aFnContext,
                                              &(aDbc->mPtContext),
                                              &(aDbc->mSession))
                 != ACI_SUCCESS);
        ULN_FLAG_UP(sNeedFinPtContext);

        /*
         * 패킷 쓰기
         */
        ACI_TEST( ulsdShardTransactionCommitRequest( aFnContext,
                                                     &(aDbc->mPtContext),
                                                     aCompletionType,
                                                     aTouchNodeArr,
                                                     aTouchNodeCount )
                  != ACI_SUCCESS );

        /*
         * 패킷 전송
         */
        ACI_TEST( ulnFlushProtocol( aFnContext, &(aDbc->mPtContext ) ) != ACI_SUCCESS );

        /*
         * BUG-45509 shard nested commit
         */
        ulsdDbcCallback(aDbc);

        /*
         * Waiting for Transaction Result Packet
         */
        sCtx = &aDbc->mPtContext.mCmiPtContext;
        if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
        {
            ACI_TEST(ulnReadProtocolIPCDA(aFnContext,
                                          &(aDbc->mPtContext),
                                          aDbc->mConnTimeoutValue) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST(ulnReadProtocol(aFnContext,
                                     &(aDbc->mPtContext),
                                     aDbc->mConnTimeoutValue) != ACI_SUCCESS);
        }

        /* 
         * PROJ-2047 Strengthening LOB - LOBCACHE
         *
         * Stmt List의 LOB Cache를 제거하자.
         */
        ACP_LIST_ITERATE(&(aDbc->mStmtList), sIterator)
        {
            sStmt = (ulnStmt *)sIterator;
            ulnLobCacheDestroy(&sStmt->mLobCache);
        }

        /*
         * Protocol Context 정리
         */
        ULN_FLAG_DOWN(sNeedFinPtContext);
        // fix BUG-17722
        ACI_TEST( ulnFinalizeProtocolContext( aFnContext, &(aDbc->mPtContext) ) != ACI_SUCCESS );
    }
    else
    {
        ulsdDbcCallback(aDbc);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_NO_CONNECTION)
    {
        /* BUG-47143 샤드 All meta 환경에서 Failover 를 검증합니다. */
        ulnErrorMgrSetCmError( aDbc, &sErrorMgr, aciGetErrorCode() );
        ulsdModuleOnCmError(aFnContext, aDbc, &sErrorMgr);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        // fix BUG-17722
        (void)ulnFinalizeProtocolContext( aFnContext, &(aDbc->mPtContext) );
    }

    return ACI_FAILURE;
}

/* coordinator에 touch node list와 commit을 보낸다. */
SQLRETURN ulsdShardEndTranDbc( ulnFnContext * aFnContext,
                               ulsdDbc      * aShard )
{
    acp_uint32_t    sTouchNodeArr[ULSD_SD_NODE_MAX_COUNT];
    acp_uint16_t    sTouchNodeCount = 0;
    acp_uint16_t    i = 0;

    ULN_FLAG(sNeedExit);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(aFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /* PROJ-1573 XA */
    ACI_TEST_RAISE(aShard->mMetaDbc->mXaEnlist == ACP_TRUE, LABEL_XA_COMMIT_ERROR);

    /*
     * make touch node array
     */
    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        /* Pass checking node that already closed on execute time */
        if ( aShard->mNodeInfo[i]->mTouched == ACP_TRUE )
        {
            SHARD_LOG("(ulsdShardEndTranDbc) NodeIndex[%d], NodeId=%d, Server=%s:%d\n",
                      i,
                      aShard->mNodeInfo[i]->mNodeId,
                      aShard->mNodeInfo[i]->mServerIP,
                      aShard->mNodeInfo[i]->mPortNo);

            sTouchNodeArr[sTouchNodeCount] = aShard->mNodeInfo[i]->mNodeId;
            sTouchNodeCount++;
        }
        else
        {
            /* Nothing to do. */
        }
    }

    aShard->mTouched = ACP_TRUE;

    /*
     * Shard Transaction 메인 루틴
     */
    ACI_TEST( ulsdShardTranDbcMain( aFnContext,
                                    aShard->mMetaDbc,
                                    SQL_COMMIT,
                                    sTouchNodeArr,
                                    sTouchNodeCount )
              != ACI_SUCCESS );

    /*
     * clean touch node
     */
    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        aShard->mNodeInfo[i]->mTouched = ACP_FALSE;
    }

    aShard->mTouched = ACP_FALSE;

    /*
     * Exit
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(aFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(aFnContext);

    /* BUG-18796*/
    ACI_EXCEPTION(LABEL_XA_COMMIT_ERROR);
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_ERROR);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        (void)ulnExit( aFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdnSimpleShardEndTranDbc( ulnDbc *aDbc, ulnTransactionOp aCompletionType )
{
    ULN_FLAG(sNeedExit);
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_ENDTRAN, aDbc, ULN_OBJ_TYPE_DBC);
    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /* PROJ-1573 XA */
    ACI_TEST_RAISE(aDbc->mXaEnlist == ACP_TRUE, LABEL_XA_COMMIT_ERROR);

    /*
     * Shard Transaction 메인 루틴
     */
    ACI_TEST( ulsdShardTranDbcMain( &sFnContext,
                                    aDbc,
                                    aCompletionType,
                                    NULL,
                                    0 )
              != ACI_SUCCESS );

    /*
     * Exit
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-18796*/
    ACI_EXCEPTION(LABEL_XA_COMMIT_ERROR);
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_ERROR);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        (void)ulnExit( &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

void ulsdGetTouchNodeCount(ulsdDbc       *aShard,
                           acp_sint16_t  *aTouchNodeCount)
{
    acp_uint16_t       sTouchNodeCnt = 0;
    acp_uint16_t       i = 0;

    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        /* Pass checking node that already closed on execute time */
        if ( aShard->mNodeInfo[i]->mTouched == ACP_TRUE )
        {
            sTouchNodeCnt++;
        }
        else
        {
            /* Do Nothing */
        }
    }

    if ( aShard->mTouched == ACP_TRUE )
    {
        sTouchNodeCnt++;
    }
    else
    {
        /* Do Nothing */
    }

    *aTouchNodeCount = sTouchNodeCnt;
}

SQLRETURN ulsdTouchNodeEndTranDbc(ulnFnContext      *aFnContext,
                                  ulsdDbc           *aShard,
                                  acp_sint16_t       aCompletionType)
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult = SQL_ERROR;
    ulsdFuncCallback  *sCallback = NULL;
    acp_bool_t         sSuccess = ACP_TRUE;
    acp_uint16_t       i = 0;

    /* commit할 node 등록 */
    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        if ( aShard->mNodeInfo[i]->mTouched == ACP_TRUE )
        {
            sNodeResult = ulsdEndTranAddCallback( i,
                                                  aShard->mNodeInfo[i]->mNodeDbc,
                                                  aCompletionType,
                                                  &sCallback );
            if ( sNodeResult == SQL_SUCCESS )
            {
                /* Nothing to do */
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                sSuccessResult = sNodeResult;
            }
            else
            {
                sErrorResult = sNodeResult;
                sSuccess = ACP_FALSE;
            }
        }
        else
        {
            /* Do Nothing */
        }
    }

    /* meta node는 병렬수행하지 않고 commit */
    if ( aShard->mTouched == ACP_TRUE )
    {
        sNodeResult = ulnEndTran( SQL_HANDLE_DBC,
                                  (ulnObject *)aShard->mMetaDbc,
                                  (acp_sint16_t)aCompletionType );

        SHARD_LOG( "(EndTranDbcTouchNode) MetaNode\n" );

        if ( sNodeResult == SQL_SUCCESS )
        {
            aShard->mTouched = ACP_FALSE;
        }
        else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
        {
            aShard->mTouched = ACP_FALSE;

            sSuccessResult = sNodeResult;
        }
        else
        {
            sErrorResult = sNodeResult;
            sSuccess = ACP_FALSE;
        }
    }
    else
    {
        /* Do Nothing */
    }

    /* commit 병렬수행 */
    ulsdDoCallback( sCallback );

    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        /* Pass checking node that already closed on execute time */
        if ( aShard->mNodeInfo[i]->mTouched == ACP_TRUE )
        {
            sNodeResult = ulsdGetResultCallback( i,
                                                 sCallback,
                                                 (aCompletionType == SQL_ROLLBACK) ?
                                                 (acp_uint8_t)1: (acp_uint8_t)0 );

            SHARD_LOG( "(EndTranDbcTouchNode) NodeIndex[%d], NodeId=%d, Server=%s:%d\n",
                       i,
                       aShard->mNodeInfo[i]->mNodeId,
                       aShard->mNodeInfo[i]->mServerIP,
                       aShard->mNodeInfo[i]->mPortNo );

            if ( sNodeResult == SQL_SUCCESS )
            {
                aShard->mNodeInfo[i]->mTouched = ACP_FALSE;
            }
            else if ( sNodeResult == SQL_NO_DATA )
            {
                /* BUG-47143 샤드 All meta 환경에서 Failover 를 검증합니다.
                 * Touch 를 ACP_FALSE 로 만들면 안된다.
                 * 다음 SQLEndTran 에서 clinet-side 로 end tran 해야한다. */
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                aShard->mNodeInfo[i]->mTouched = ACP_FALSE;

                /* 에러 전달 */
                ulsdNativeErrorToUlnError( aFnContext,
                                           SQL_HANDLE_DBC,
                                           (ulnObject *)aShard->mNodeInfo[i]->mNodeDbc,
                                           aShard->mNodeInfo[i],
                                           ( aCompletionType == SQL_COMMIT ) ?
                                               (acp_char_t *)"Commit" :
                                               (acp_char_t *)"Rollback" );

                sSuccessResult = sNodeResult;
            }
            else
            {
                /* 에러 전달 */
                ulsdNativeErrorToUlnError(aFnContext,
                                          SQL_HANDLE_DBC,
                                          (ulnObject *)aShard->mNodeInfo[i]->mNodeDbc,
                                          aShard->mNodeInfo[i],
                                          (aCompletionType == SQL_COMMIT) ?
                                          (acp_char_t *)"Commit" :
                                          (acp_char_t *)"Rollback");

                sErrorResult = sNodeResult;
                sSuccess = ACP_FALSE;
            }
        }
        else
        {
            /* Do Nothing */
        }
    }

    ACI_TEST( sSuccess == ACP_FALSE );

    ulsdRemoveCallback( sCallback );

    return sSuccessResult;

    ACI_EXCEPTION_END;

    ulsdRemoveCallback( sCallback );

    return sErrorResult;
}

SQLRETURN ulsdTouchNodeShardEndTranDbc(ulnFnContext      *aFnContext,
                                       ulsdDbc           *aShard,
                                       acp_sint16_t       aCompletionType)
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult = SQL_ERROR;
    ulsdFuncCallback  *sCallback = NULL;
    acp_bool_t         sSuccess = ACP_TRUE;
    acp_uint16_t       i = 0;

    /* commit할 node 등록 */
    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        if ( aShard->mNodeInfo[i]->mTouched == ACP_TRUE )
        {
            sNodeResult = ulsdShardEndTranAddCallback( i,
                                                       aShard->mNodeInfo[i]->mNodeDbc,
                                                       aCompletionType,
                                                       &sCallback );
            if ( sNodeResult == SQL_SUCCESS )
            {
                /* Nothing to do */
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                sSuccessResult = sNodeResult;
            }
            else
            {
                sErrorResult = sNodeResult;
                sSuccess = ACP_FALSE;
            }
        }
        else
        {
            /* Do Nothing */
        }
    }

    /* meta node는 병렬수행하지 않고 commit */
    if ( aShard->mTouched == ACP_TRUE )
    {
        sNodeResult = ulsdnSimpleShardEndTranDbc( aShard->mMetaDbc,
                                                  (acp_sint16_t)aCompletionType );

        SHARD_LOG( "(TouchNodeShardEndTranDbc) MetaNode\n" );

        if ( sNodeResult == SQL_SUCCESS )
        {
            aShard->mTouched = ACP_FALSE;
        }
        else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
        {
            aShard->mTouched = ACP_FALSE;

            sSuccessResult = sNodeResult;
        }
        else
        {
            sErrorResult = sNodeResult;
            sSuccess = ACP_FALSE;
        }
    }
    else
    {
        /* Do Nothing */
    }

    /* commit 병렬수행 */
    ulsdDoCallback( sCallback );

    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        /* Pass checking node that already closed on execute time */
        if ( aShard->mNodeInfo[i]->mTouched == ACP_TRUE )
        {
            sNodeResult = ulsdGetResultCallback( i,
                                                 sCallback,
                                                 (aCompletionType == SQL_ROLLBACK) ?
                                                 (acp_uint8_t)1: (acp_uint8_t)0 );

            SHARD_LOG( "(TouchNodeShardEndTranDbc) NodeIndex[%d], NodeId=%d, Server=%s:%d\n",
                       i,
                       aShard->mNodeInfo[i]->mNodeId,
                       aShard->mNodeInfo[i]->mServerIP,
                       aShard->mNodeInfo[i]->mPortNo );

            if ( sNodeResult == SQL_SUCCESS )
            {
                aShard->mNodeInfo[i]->mTouched = ACP_FALSE;
            }
            else if ( sNodeResult == SQL_NO_DATA )
            {
                /* BUG-47143 샤드 All meta 환경에서 Failover 를 검증합니다.
                 * Touch 를 ACP_FALSE 로 만들면 안된다.
                 * 다음 SQLEndTran 에서 clinet-side 로 end tran 해야한다. */
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                aShard->mNodeInfo[i]->mTouched = ACP_FALSE;

                /* 에러 전달 */
                ulsdNativeErrorToUlnError( aFnContext,
                                           SQL_HANDLE_DBC,
                                           (ulnObject *)aShard->mNodeInfo[i]->mNodeDbc,
                                           aShard->mNodeInfo[i],
                                           ( aCompletionType == SQL_COMMIT ) ?
                                               (acp_char_t *)"Commit" :
                                               (acp_char_t *)"Rollback" );

                sSuccessResult = sNodeResult;
            }
            else
            {
                /* 에러 전달 */
                ulsdNativeErrorToUlnError(aFnContext,
                                          SQL_HANDLE_DBC,
                                          (ulnObject *)aShard->mNodeInfo[i]->mNodeDbc,
                                          aShard->mNodeInfo[i],
                                          (aCompletionType == SQL_COMMIT) ?
                                          (acp_char_t *)"Commit" :
                                          (acp_char_t *)"Rollback");

                sErrorResult = sNodeResult;
                sSuccess = ACP_FALSE;
            }
        }
        else
        {
            /* Do Nothing */
        }
    }

    ACI_TEST_RAISE( sSuccess == ACP_FALSE,
                    ERR_ENDTRAN_FAIL );

    ulsdRemoveCallback( sCallback );

    return sSuccessResult;

    ACI_EXCEPTION( ERR_ENDTRAN_FAIL )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, sErrorResult );
    }

    ACI_EXCEPTION_END;

    ulsdRemoveCallback( sCallback );

    return sErrorResult;
}
