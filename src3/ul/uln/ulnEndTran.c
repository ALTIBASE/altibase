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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulsdnExecute.h>
#include <ulnEndTran.h>
#include <ulsdDistTxInfo.h>

ACI_RC ulnCallbackTransactionResult(cmiProtocolContext *aProtocolContext,
                                    cmiProtocol        *aProtocol,
                                    void               *aServiceSession,
                                    void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

static ACI_RC ulnEndTranCheckArgs(ulnFnContext *aFnContext, ulnTransactionOp aCompletionType)
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

static ACI_RC ulnEndTranDbcMain(ulnFnContext    *aFnContext,
                                ulnDbc          *aDbc,
                                ulnTransactionOp aCompletionType)
{
    ULN_FLAG(sNeedFinPtContext);

#ifdef COMPILE_SHARDCLI
    ulnErrorMgr sErrorMgr;
#endif /* COMPILE_SHARDCLI */

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
        ACI_TEST(ulnWriteTransactionREQ(aFnContext,
                                        &(aDbc->mPtContext),
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
#ifdef COMPILE_SHARDCLI
        /* BUG-47143 샤드 All meta 환경에서 Failover 를 검증합니다. */
        ulnErrorMgrSetCmError( aDbc, &sErrorMgr, aciGetErrorCode() );
        ulsdModuleOnCmError(aFnContext, aDbc, &sErrorMgr);
#else
        ulnError(aFnContext, ulERR_ABORT_NO_CONNECTION, "");
#endif /* COMPILE_SHARDCLI */
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        // fix BUG-17722
        ulnFinalizeProtocolContext(aFnContext,&(aDbc->mPtContext));
    }

    return ACI_FAILURE;
}

/*
 * 이 함수는
 *  ulnEndTran 에서 바로 호출되기도 하며 (이 경우, DBC 는 context 에서 얻음)
 *  ulnEndTranInEnv 에서 호출되기도 한다 (이 경우, Context 의 obj 는 ENV 이다)
 * 그래서 ulnDbc *aDbc 가 필요하다.
 */
static SQLRETURN ulnEndTranDbc(ulnDbc *aDbc, ulnTransactionOp aCompletionType)
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

    ACI_TEST(ulnEndTranCheckArgs(&sFnContext, aCompletionType) != ACI_SUCCESS);

    /*
     * End Transaction 메인 루틴
     */
    ACI_TEST(ulnEndTranDbcMain(&sFnContext, aDbc, aCompletionType) != ACI_SUCCESS);

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

static SQLRETURN ulnEndTranEnv(ulnEnv *aEnv, ulnTransactionOp aCompletionType)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext sFnContext;
    SQLRETURN    rc;

    acp_list_node_t *sIterator;
    acp_bool_t       sIsFailOccurred;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_ENDTRAN, aEnv, ULN_OBJ_TYPE_ENV);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST(ulnEndTranCheckArgs(&sFnContext, aCompletionType) != ACI_SUCCESS);

    sIsFailOccurred = ACP_FALSE;

    ACP_LIST_ITERATE(&(aEnv->mDbcList), sIterator)
    {
        /*
         * BUGBUG : DBC 를 lock 해야 하나? lock 해야 하네.
         *          DBC 에다가 Diagnostic record 도 매달아야 하는데.. -_-;
         */
        // if(ulnEndTranDbcMain(&sFnContext, (ulnDbc *)sIterator, aCompletionType) != ACI_SUCCESS)
        rc = ulnEndTranDbc((ulnDbc *)sIterator, aCompletionType);
        if(rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            sIsFailOccurred = ACP_TRUE;
        }
    }

    if(sIsFailOccurred == ACP_TRUE)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_ERROR);
    }

    /*
     * Exit
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

/*
 * BUGBUG: 일단은 Auto commit 모드만 사용하는 것으로 가정하자.
 *         Manual Commit 모드일 경우 cursor 와 관련된 것도 고려해 줘야 한다.
 */
SQLRETURN ulnEndTran(acp_sint16_t aHandleType, ulnObject *aObject, acp_sint16_t aCompletionType)
{
    SQLRETURN        sRet;
    ulnTransactionOp aOp;

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

    /*
     * Check if the arguments are valid
     */
    switch(aHandleType)
    {
        case SQL_HANDLE_ENV:
            sRet = ulnEndTranEnv((ulnEnv *)aObject, aOp);
            break;

        case SQL_HANDLE_DBC:
            sRet = ulnEndTranDbc((ulnDbc *)aObject, aOp);
            break;

        default:
            /*
             * BUGBUG : HY092 : Invalid attribute / option identifier
             *          Diagnostic record 를 매달 객체는?
             */
            sRet = SQL_ERROR;
            break;
    }

    ULN_TRACE_LOG(NULL, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| [handle: %"ACI_INT32_FMT" type: %"ACI_INT32_FMT
            "] %"ACI_INT32_FMT,
            "ulnEndTran", aHandleType, aCompletionType, sRet);

    return sRet;
}

