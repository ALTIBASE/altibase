/***********************************************************************
 * Copyright 1999-2012, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>

#include <ulsd.h>
#include <ulsdnExecute.h>
#include <ulsdnTrans.h>
#include <ulsdDistTxInfo.h>

ACI_RC ulsdCallbackShardPrepareResult(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aServiceSession,
                                      void               *aUserContext)
{
    ulnFnContext   *sFnContext = (ulnFnContext *)aUserContext;
    ulnDbc         *sDbc       = sFnContext->mHandle.mDbc;

    acp_uint8_t     sReadOnly;
    acp_uint64_t    sPrepareSCN = 0;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD1(aProtocolContext, sReadOnly);
    CMI_RD8(aProtocolContext, &sPrepareSCN);

    /* ReadOnly 인 경우 sPrepareSCN 값이 0 이다.
     * Prepare 를 수행하지 않기 때문에 0 값 그대로 sd 모듈로 넘겨야 한다. */
    ulsdUpdateSCNToDbc(sDbc, &sPrepareSCN);  /* PROJ-2733-DistTxInfo */

    /* BUG-45967 Data Node의 Shard Session 정리 */
    return ulsdCallbackShardPrepareResultInternal( aUserContext,
                                                   sReadOnly );
}

ACI_RC ulsdCallbackShardPrepareResultInternal( void        * aUserContext,
                                               acp_uint8_t   aReadOnly )
{
    ulnFnContext           *sFnContext  = (ulnFnContext *)aUserContext;
    ulnDbc                 *sDbc        = sFnContext->mHandle.mDbc;

    ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sDbc) != ULN_OBJ_TYPE_DBC, LABEL_MEM_MANAGE_ERR);

    if ( aReadOnly == (acp_uint8_t)1 )
    {
        sDbc->mShardDbcCxt.mReadOnlyTx = ACP_TRUE;
    }
    else
    {
        sDbc->mShardDbcCxt.mReadOnlyTx = ACP_FALSE;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "CallbackShardPrepareResultInternal, Object is not a DBC handle.");
    }
    ACI_EXCEPTION_END;

    /* CM 콜백 함수는 communication error가 아닌 한 ACI_SUCCESS를 반환해야 한다.
     * 콜백 에러는 function context에 설정된 값으로 판단한다. */
    return ACI_SUCCESS;
}

ACI_RC ulsdCallbackShardEndPendingTxResult(cmiProtocolContext *aProtocolContext,
                                           cmiProtocol        *aProtocol,
                                           void               *aServiceSession,
                                           void               *aUserContext)
{
    ulnFnContext   *sFnContext = (ulnFnContext *)aUserContext;
    ulnDbc         *sDbc       = sFnContext->mHandle.mDbc;
    acp_uint64_t    sSCN       = 0;
    
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD8(aProtocolContext, &sSCN);  /* PROJ-2733-Protocol */

    if (sSCN > 0)
    {
        ulsdUpdateSCNToDbc(sDbc, &sSCN);  /* PROJ-2733-DistTxInfo */
    }

    return ACI_SUCCESS;
}

ACI_RC ulsdShardPrepareRequest(ulnFnContext     *aFnContext,
                               ulnPtContext     *aPtContext,
                               acp_uint32_t      aXIDSize,
                               acp_uint8_t      *aXID)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_ShardPrepareV3;  /* PROJ-2733-Protocol */

    CMI_WRITE_CHECK(sCtx, 1 + 4 + aXIDSize);  
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ShardPrepareV3);

    /* xid */
    CMI_WR4(sCtx, &aXIDSize);
    CMI_WCP(sCtx, aXID, aXIDSize);

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulsdShardEndPendingTxRequest(ulnFnContext     *aFnContext,
                                    ulnPtContext     *aPtContext,
                                    acp_uint32_t      aXIDSize,
                                    acp_uint8_t      *aXID,
                                    ulnTransactionOp  aTransactOp)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    ulnDbc             *sDbc = aFnContext->mHandle.mDbc;

    sPacket.mOpID = CMP_OP_DB_ShardEndPendingTxV3;  /* PROJ-2733-Protocol */

    CMI_WRITE_CHECK(sCtx, 1 + 4 + aXIDSize + 1 + 8);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ShardEndPendingTxV3);

    /* xid */
    CMI_WR4(sCtx, &aXIDSize);
    CMI_WCP(sCtx, aXID, aXIDSize);

    /* trans op */
    switch(aTransactOp)
    {
        case ULN_TRANSACT_COMMIT:
            CMI_WR1(sCtx, 1);
            break;

        case ULN_TRANSACT_ROLLBACK:
            CMI_WR1(sCtx, 2);
            break;

        default:
            ACI_RAISE(LABEL_INVALID_OPCODE);
            break;
    }

    /* PROJ-2733-DistTxInfo */
    CMI_WR8(sCtx, &(sDbc->mSCN));  /* GlobalCommitSCN */

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_OPCODE)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_TRANSACTION_OPCODE);
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulsdShardPrepareTranMain(ulnFnContext    *aFnContext,
                                ulnDbc          *aDbc,
                                acp_uint32_t     aXIDSize,
                                acp_uint8_t     *aXID)
{
    ULN_FLAG(sNeedFinPtContext);

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
    if ((aDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF) && (aDbc->mIsConnected == ACP_TRUE))
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
        ACI_TEST(ulsdShardPrepareRequest(aFnContext,
                                         &(aDbc->mPtContext),
                                         aXIDSize,
                                         aXID)
                 != ACI_SUCCESS);

        /*
         * 패킷 전송
         */
        ACI_TEST(ulnFlushProtocol(aFnContext,&(aDbc->mPtContext)) != ACI_SUCCESS);

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
        ACI_TEST(ulnFinalizeProtocolContext(aFnContext,&(aDbc->mPtContext)) != ACI_SUCCESS);
    }
    else
    {
        ulsdDbcCallback(aDbc);  /* PROJ-2733 */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_NO_CONNECTION)
    {
        ulnError(aFnContext, ulERR_ABORT_NO_CONNECTION, "");
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        // fix BUG-17722
        ulnFinalizeProtocolContext(aFnContext,&(aDbc->mPtContext));
    }

    return ACI_FAILURE;
}

SQLRETURN ulsdShardPrepareTran(ulnDbc       *aDbc,
                               acp_uint32_t  aXIDSize,
                               acp_uint8_t  *aXID,
                               acp_uint8_t  *aReadOnly)
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
     * Prepare Transaction 메인 루틴
     */
    ACI_TEST(ulsdShardPrepareTranMain(&sFnContext,
                                      aDbc,
                                      aXIDSize,
                                      aXID)
             != ACI_SUCCESS);

    /*
     * return readonly tx
     */
    if ( aDbc->mShardDbcCxt.mReadOnlyTx == ACP_TRUE )
    {
        *aReadOnly = (acp_uint8_t)1;
    }
    else
    {
        *aReadOnly = (acp_uint8_t)0;
    }

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
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

ACI_RC ulsdEndPendingTxCheckArgs(ulnFnContext *aFnContext, ulnTransactionOp aCompletionType)
{
    ACI_TEST_RAISE( aCompletionType != ULN_TRANSACT_COMMIT && 
                    aCompletionType != ULN_TRANSACT_ROLLBACK, LABEL_INVALID_TRANSACTION_OPCODE );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_TRANSACTION_OPCODE)
    {
        /*
         * HY012 : Invalid transaction operation code
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_TRANSACTION_OPCODE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulsdShardEndPendingTranMain(ulnFnContext     *aFnContext,
                                   ulnDbc           *aDbc,
                                   acp_uint32_t      aXIDSize,
                                   acp_uint8_t      *aXID,
                                   ulnTransactionOp  aCompletionType)
{
    ULN_FLAG(sNeedFinPtContext);

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
    if ((aDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF) && (aDbc->mIsConnected == ACP_TRUE))
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
        ACI_TEST(ulsdShardEndPendingTxRequest(aFnContext,
                                              &(aDbc->mPtContext),
                                              aXIDSize,
                                              aXID,
                                              aCompletionType)
                 != ACI_SUCCESS);

        /*
         * 패킷 전송
         */
        ACI_TEST(ulnFlushProtocol(aFnContext,&(aDbc->mPtContext)) != ACI_SUCCESS);

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
        ACI_TEST(ulnFinalizeProtocolContext(aFnContext,&(aDbc->mPtContext)) != ACI_SUCCESS);
    }
    else
    {
        ulsdDbcCallback(aDbc);  /* PROJ-2733 */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_NO_CONNECTION)
    {
        ulnError(aFnContext, ulERR_ABORT_NO_CONNECTION, "");
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        // fix BUG-17722
        ulnFinalizeProtocolContext(aFnContext,&(aDbc->mPtContext));
    }

    return ACI_FAILURE;
}

SQLRETURN ulsdShardEndPendingTran(ulnDbc       *aDbc,
                                  acp_uint32_t  aXIDSize,
                                  acp_uint8_t  *aXID,
                                  acp_sint16_t  aCompletionType)
{
    ULN_FLAG(sNeedExit);
    ulnFnContext     sFnContext;
    ulnTransactionOp aOp;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_ENDTRAN, aDbc, ULN_OBJ_TYPE_DBC);
    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /* PROJ-1573 XA */
    ACI_TEST_RAISE(aDbc->mXaEnlist == ACP_TRUE, LABEL_XA_COMMIT_ERROR);

    switch(aCompletionType)
    {
        case SQL_COMMIT:
            aOp = ULN_TRANSACT_COMMIT;
            break;

        case SQL_ROLLBACK:
            aOp = ULN_TRANSACT_ROLLBACK;
            break;

        default:
            aOp = ULN_TRANSACT_INVALID_OP;
            break;
    }

    ACI_TEST(ulsdEndPendingTxCheckArgs(&sFnContext, aOp) != ACI_SUCCESS);

    /*
     * Prepare Transaction 메인 루틴
     */
    ACI_TEST(ulsdShardEndPendingTranMain(&sFnContext,
                                         aDbc,
                                         aXIDSize,
                                         aXID,
                                         aOp)
             != ACI_SUCCESS);

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
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
