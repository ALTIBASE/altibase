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
#include <ulnFailOver.h>
#include <ulnCommunication.h>

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnFetchOpAsync.h>
#include <ulnSemiAsyncPrefetch.h>

#include <ulsdnExecute.h>

#include <ulsdDef.h>

/*
 * ======================================
 * Basic cmi Wrapper functions
 * ======================================
 */

typedef struct ulnCmProtocolEntry
{
    acp_sint32_t      mNeedResult;
    const acp_char_t *mProtocolName;
} ulnCmProtocolEntry;

static ulnCmProtocolEntry gUlnCmProtocolTable[CMP_OP_DB_MAX + 1] =
{
    /*  0 */ { 0, "CMP_OP_DB_Message" },
    /*  1 */ { 0, "CMP_OP_DB_ErrorResult" },
    /*  2 */ {-1, "CMP_OP_DB_ErrorInfo" },
    /*  3 */ {-1, "CMP_OP_DB_ErrorInfoResult" },
    /*  4 */ {-1, "CMP_OP_DB_Connect" }, /* replaced with CMP_OP_DB_ConnectEx */
    /*  5 */ {-1, "CMP_OP_DB_ConnectResult" }, /* replaced with CMP_OP_DB_ConnectExResult */
    /*  6 */ { 1, "CMP_OP_DB_Disconnect" },
    /*  7 */ { 0, "CMP_OP_DB_DisconnectResult" },
    /*  8 */ { 1, "CMP_OP_DB_PropertyGet" },
    /*  9 */ { 0, "CMP_OP_DB_PropertyGetResult" },
    /* 10 */ {-1, "CMP_OP_DB_PropertySet" },   /* replaced with PropertySetV2 */
    /* 11 */ { 0, "CMP_OP_DB_PropertySetResult" },
    /* 12 */ {-1, "CMP_OP_DB_Prepare" },
    /* 13 */ {-1, "CMP_OP_DB_PrepareResult" },
    /* 14 */ { 1, "CMP_OP_DB_PlanGet" },
    /* 15 */ { 0, "CMP_OP_DB_PlanGetResult" },
    /* 16 */ { 1, "CMP_OP_DB_ColumnInfoGet" },
    /* 17 */ { 0, "CMP_OP_DB_ColumnInfoGetResult" },
    /* 18 */ { 0, "CMP_OP_DB_ColumnInfoGetListResult" },
    /* 19 */ {-1, "CMP_OP_DB_ColumnInfoSet" },
    /* 20 */ {-1, "CMP_OP_DB_ColumnInfoSetResult" },
    /* 21 */ { 1, "CMP_OP_DB_ParamInfoGet" },
    /* 22 */ { 0, "CMP_OP_DB_ParamInfoGetResult" },
    /* 23 */ {-1, "CMP_OP_DB_ParamInfoSet" },
    /* 24 */ {-1, "CMP_OP_DB_ParamInfoSetResult" },
    /* 25 */ { 1, "CMP_OP_DB_ParamInfoSetList" },
    /* 26 */ { 0, "CMP_OP_DB_ParamInfoSetListResult" },
    /* 27 */ { 0, "CMP_OP_DB_ParamDataIn" },
    /* 28 */ {-1, "CMP_OP_DB_ParamDataOut" },
    /* 29 */ { 0, "CMP_OP_DB_ParamDataOutList" },
    /* 30 */ {-1, "CMP_OP_DB_ParamDataInList" },       /* replaced with ParamDataInListV2 */
    /* 31 */ {-1, "CMP_OP_DB_ParamDataInListResult" }, /* replaced with ParamDataInListV2Result */
    /* 32 */ {-1, "CMP_OP_DB_Execute" },               /* replaced with ExecuteV2 */
    /* 33 */ {-1, "CMP_OP_DB_ExecuteResult" },         /* replaced with ExecuteV2Result */
    /* 34 */ { 1, "CMP_OP_DB_FetchMove" },
    /* 35 */ { 0, "CMP_OP_DB_FetchMoveResult" },
    /* 36 */ {-1, "CMP_OP_DB_Fetch" },   /* replaced with FetchV2 */
    /* 37 */ { 0, "CMP_OP_DB_FetchBeginResult" },
    /* 38 */ { 0, "CMP_OP_DB_FetchResult" },
    /* 39 */ {-1, "CMP_OP_DB_FetchListResult" },
    /* 40 */ { 0, "CMP_OP_DB_FetchEndResult" },
    /* 41 */ { 1, "CMP_OP_DB_Free" },
    /* 42 */ { 0, "CMP_OP_DB_FreeResult" },
    /* 43 */ { 1, "CMP_OP_DB_Cancel" },
    /* 44 */ { 0, "CMP_OP_DB_CancelResult" },
    /* 45 */ { 1, "CMP_OP_DB_Transaction" },
    /* 46 */ { 0, "CMP_OP_DB_TransactionResult" },
    /* 47 */ { 1, "CMP_OP_DB_LobGetSize" },
    /* 48 */ { 0, "CMP_OP_DB_LobGetSizeResult" },
    /* 49 */ { 1, "CMP_OP_DB_LobGet" },
    /* 50 */ { 0, "CMP_OP_DB_LobGetResult" },
    /* 51 */ { 1, "CMP_OP_DB_LobPutBegin" },
    /* 52 */ { 0, "CMP_OP_DB_LobPutBeginResult" },
    /* 53 */ { 0, "CMP_OP_DB_LobPut" },
    /* 54 */ { 1, "CMP_OP_DB_LobPutEnd" },
    /* 55 */ { 0, "CMP_OP_DB_LobPutEndResult" },
    /* 56 */ { 0, "CMP_OP_DB_LobFree" },               /* bug-19138 */
    /* 57 */ { 0, "CMP_OP_DB_LobFreeResult" },
    /* 58 */ { 0, "CMP_OP_DB_LobFreeAll" },            /* PROJ-1752 */
    /* 59 */ { 0, "CMP_OP_DB_LobFreeAllResult" },
             /* PROJ-1573 XA */
    /* 60 */ { 1, "CMP_OP_DB_XaOperation" },
    /* 61 */ { 1, "CMP_OP_DB_XaXid" },
    /* 62 */ { 0, "CMP_OP_DB_XaResult" },
    /* 63 */ { 1, "CMP_OP_DB_XaTransaction" },
    /* 64 */ { 0, "CMP_OP_DB_LobGetBytePosCharLen"  },
    /* 65 */ { 0, "CMP_OP_DB_LobGetBytePosCharLenResult" },
    /* 66 */ { 0, "CMP_OP_DB_LobGetCharPosCharLen"  },
    /* 67 */ { 0, "CMP_OP_DB_LobBytePos"            },
    /* 68 */ { 0, "CMP_OP_DB_LobBytePosResult"      },
    /* 69 */ { 0, "CMP_OP_DB_LobCharLength"         },
    /* 70 */ { 0, "CMP_OP_DB_LobCharLengthResult"   },
    /* 71 */ { 0, "CMP_OP_DB_ParamDataInResult" }, // bug-28259 for ipc
    /* 72 */ { 0, "CMP_OP_DB_Handshake" },         // proj_2160 cm_type
    /* 73 */ { 0, "CMP_OP_DB_HandshakeResult" },
             /* PROJ-2177 User Interface - Cancel */
    /* 74 */ {-1, "CMP_OP_DB_PrepareByCID" },
    /* 75 */ { 1, "CMP_OP_DB_CancelByCID" },
             /* PROJ-2047 Strengthening LOB - Added Interfaces */
    /* 76 */ { 1, "CMP_OP_DB_LobTrim" },
    /* 77 */ { 0, "CMP_OP_DB_LobTrimResult" },
             /* BUG-38496 */
    /* 78 */ {-1, "CMP_OP_DB_ConnectEx" },
    /* 79 */ {-1, "CMP_OP_DB_ConnectExResult" },
             /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
    /* 80 */ {-1, "CMP_OP_DB_FetchV2" },
             /* BUG-41793 Keep a compatibility among tags */
    /* 81 */ { 1, "CMP_OP_DB_PropertySetV2" },
             /* PROJ-2616 */
    /* 82 */ { 0, "CMP_OP_DB_IPCDALastOpEnded" },
             /* BUG-44572 */
    /* 83 */ { -1, "CMP_OP_DB_ExecuteV2" },
    /* 84 */ { -1, "CMP_OP_DB_ExecuteV2Result" },
    /* 85 */ { -1, "CMP_OP_DB_ParamDataInListV2" },
    /* 86 */ { -1, "CMP_OP_DB_ParamDataInListV2Result" },
             /* PROJ-2598 altibase sharding */
    /* 87 */ { 1, "CMP_OP_DB_ShardAnalyze" },
    /* 88 */ { 0, "CMP_OP_DB_ShardAnalyzeResult" },
    /* 89 */ { 1, "CMP_OP_DB_ShardNodeUpdateList" },
    /* 90 */ { 0, "CMP_OP_DB_ShardNodeUpdateListResult" },
    /* 91 */ { 1, "CMP_OP_DB_ShardNodeGetList" },
    /* 92 */ { 0, "CMP_OP_DB_ShardNodeGetListResult" },
    /* 93 */ { 1, "CMP_OP_DB_ShardHandshake" },
    /* 94 */ { 0, "CMP_OP_DB_ShardHandshakeResult" },
    /* 95 */ {-1, "CMP_OP_DB_ShardTransaction" },
    /* 96 */ {-1, "CMP_OP_DB_ShardTransactionResult" },
    /* 97 */ {-1, "CMP_OP_DB_ShardPrepare" },
    /* 98 */ {-1, "CMP_OP_DB_ShardPrepareResult" },
    /* 99 */ {-1, "CMP_OP_DB_ShardEndPendingTx" },
    /*100 */ {-1, "CMP_OP_DB_ShardEndPendingTxResult" },
             /* BUG-46785 Shard statement partial rollback */
    /*101 */ { 1, "CMP_OP_DB_SetSavepoint" },
    /*102 */ { 0, "CMP_OP_DB_SetSavepointResult" },
    /*103 */ { 1, "CMP_OP_DB_RollbackToSavepoint" },
    /*104 */ { 0, "CMP_OP_DB_RollbackToSavepointResult" },
    /*105 */ { 1, "CMP_OP_DB_ShardStmtPartialRollback" },
    /*106 */ { 0, "CMP_OP_DB_ShardStmtPartialRollbackResult" },
    /*107 */ { 1, "CMP_OP_DB_ShardNodeReport" },
    /*108 */ { 0, "CMP_OP_DB_ShardNodeReportResult" },
             /* PROJ-2733-Protocol */
    /*109 */ { 0, "CMP_OP_DB_ErrorV3Result" },
    /*110 */ { 1, "CMP_OP_DB_ConnectV3" },
    /*111 */ { 0, "CMP_OP_DB_ConnectV3Result" },
    /*112 */ { 1, "CMP_OP_DB_PropertySetV3" },
    /*113 */ { 0, "CMP_OP_DB_PropertySetV3Result" },
    /*114 */ { 1, "CMP_OP_DB_ParamDataInListV3" },
    /*115 */ { 0, "CMP_OP_DB_ParamDataInListV3Result" },
    /*116 */ { 1, "CMP_OP_DB_ExecuteV3" },
    /*117 */ { 0, "CMP_OP_DB_ExecuteV3Result" },
    /*118 */ { 1, "CMP_OP_DB_ShardTransactionV3" },
    /*119 */ { 0, "CMP_OP_DB_ShardTransactionV3Result" },
    /*120 */ { 1, "CMP_OP_DB_ShardPrepareV3" },
    /*121 */ { 0, "CMP_OP_DB_ShardPrepareV3Result" },
    /*122 */ { 1, "CMP_OP_DB_ShardEndPendingTxV3" },
    /*123 */ { 0, "CMP_OP_DB_ShardEndPendingTxV3Result" },
             /* PROJ-2728 Sharding LOB */
    /*124*/  { 1, "CMP_OP_DB_LobGetSizeV3" },
    /*125*/  { 0, "CMP_OP_DB_LobGetSizeV3Result" },
    /*126*/  { 1, "CMP_OP_DB_FetchV3" },
    /*127*/  { 0, "CMP_OP_DB_ShardRebuildNotiV3" },
             /* BUG-48775 */
    /*128*/  { 1, "CMP_OP_DB_PrepareV3" },
    /*129*/  { 1, "CMP_OP_DB_PrepareByCIDV3" },
    /*130*/  { 0, "CMP_OP_DB_PrepareV3Result" },
    /*131*/  {-1, "CMP_OP_DB_MAX" }
};

ACI_RC ulnWriteProtocol(ulnFnContext *aFnContext, ulnPtContext *aPtContext, cmiProtocol *aPacket)
{
    ulnDbc       *sDbc;
    ulnErrorMgr   sErrorMgr;

    cmiProtocolContext *sCtx = &aPtContext->mCmiPtContext;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    ACI_TEST_RAISE(ulnDbcIsConnected(sDbc) != ACP_TRUE, LABEL_CON_CLOSED);

    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        aPtContext->mNeedReadProtocol += 1;

        /* BUG-46502 */
        cmiIPCDAIncDataCount(sCtx);
    }
    else
    {
        aPtContext->mNeedFlush = ACP_TRUE;

        ACE_ASSERT(aPacket->mOpID < CMP_OP_DB_MAX);
        ACE_DASSERT(gUlnCmProtocolTable[aPacket->mOpID].mNeedResult >= 0);
        aPtContext->mNeedReadProtocol += gUlnCmProtocolTable[aPacket->mOpID].mNeedResult;

        // bug-28259: ipc needs paramDataInResult
        // paramDataIn 요청에 대해 ipc인 경우에만 서버에서 응답을 줌.
        if((sDbc->mConnType == ULN_CONNTYPE_IPC) &&
           (aPacket->mOpID == CMP_OP_DB_ParamDataIn))
        {
            aPtContext->mNeedReadProtocol += 1;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CON_CLOSED)
    {
        ulnErrorMgrSetUlError( &sErrorMgr,
                               ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                               "Server closed the connection." );

        /* BUG-47131 샤드 All meta 환경에서 Client failover 시 hang 발생 */
        sCtx->mWriteBlock->mDataSize = CMP_HEADER_SIZE;
        sCtx->mWriteBlock->mCursor = CMP_HEADER_SIZE;

#ifdef COMPILE_SHARDCLI
        ulsdModuleOnCmError( aFnContext, sDbc, &sErrorMgr );
#else
        //PROJ-1645 UL FailOver.
        if (ulnFailoverDoSTF(aFnContext) == ACI_FAILURE)
        {
            ulnError(aFnContext,
                 ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                 "Server closed the connection.");
        }
        else
        {
            ulnError( aFnContext, ulERR_ABORT_FAILOVER_SUCCESS,
                      ulnErrorMgrGetErrorCode(&sErrorMgr),
                      ulnErrorMgrGetSQLSTATE(&sErrorMgr),
                      ulnErrorMgrGetErrorMessage(&sErrorMgr) );
        }
#endif /* COMPILE_SHARDCLI */
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnWriteProtocol : object type is neither DBC nor STMT.");
    }
    ACI_EXCEPTION_END;

    aPtContext->mNeedReadProtocol = 0;
    aPtContext->mNeedFlush        = ACP_FALSE;

    return ACI_FAILURE;
}

/**
 * ulnReadProtocolIPCDA.
 *
 * @param[in] aFnContext
 * @param[in] aPtContext
 * @param[in] aTimeout
 *
 * 주어진 protocol context 에 수신된 패킷을 읽고, 해당하는 콜백을 호출한다.
 */
ACI_RC ulnReadProtocolIPCDA(ulnFnContext   *aFnContext,
                            ulnPtContext   *aPtContext,
                            acp_time_t      aTimeout)
{
    ulnDbc       *sDbc;
    ulnErrorMgr   sErrorMgr;

    ACP_UNUSED(aTimeout);
    
    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);
    /*
     * 연결 안되어 있으면 조용히 리턴
     */
    ACI_TEST_RAISE(ulnDbcIsConnected(sDbc) != ACP_TRUE, LABEL_CON_CLOSED);

    /*
     * ReadProtocol 할 필요 없어도 그냥 조용히 리턴
     */
    ACI_TEST_RAISE( cmiRecvIPCDA( &(aPtContext->mCmiPtContext),
                                  aFnContext,
                                  sDbc->mIPCDAULExpireCount,
                                  sDbc->mIPCDAMicroSleepTime )
                    != ACI_SUCCESS, LABEL_CM_ERR);

    aPtContext->mNeedReadProtocol = 0;

    /*
     * Note : 서버에서 에러가 전송되어 올 경우, ErrorResult 패킷이 수신된다.
     *        그렇지만, cmiReadProtocolAndCallback() 함수는 ACI_SUCCESS 를 리턴한다.
     *        그래서, 직접 Function Context 의 mSqlReturn 값을 체크해서 서버로부터의
     *        에러 전송 여부를 판단해야 한다.
     */
    ACI_TEST(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext)) == 0);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CON_CLOSED)
    {
        ulnErrorMgrSetUlError( &sErrorMgr,
                               ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                               "Server closed the connection." );
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnReadProtocol : object type is neither DBC nor STMT.");
    }

    ACI_EXCEPTION(LABEL_CM_ERR)
    {
        ulnErrorMgrSetCmError( sDbc, &sErrorMgr, aciGetErrorCode() );
        ulnErrHandleCmError(aFnContext, NULL); /* BUG-45713  */
        /*
         * cm 의 MARSHALL_ERROR 등이 발생했을 경우 여기서 에러가 리턴되고,
         * 프로토콜을 read 한 ulnFetch() 등의 함수의 ACI_EXCEPTION() 루틴에서
         * 다시한번 flush / read 를 하게 되는데, 이때 다시한번 read 를 하게 되므로
         * 무한대기를 하게 된다.
         *
         * 이를 방지하기 위해서 mNeedReadProtocol 을 ACP_FALSE 로 만들어 준다.
         */
        aPtContext->mNeedReadProtocol = 0;
    }

    ACI_EXCEPTION_END;
    
#ifdef COMPILE_SHARDCLI
    /* BUG-46092 */
    ulsdModuleErrorCheckAndAlignDataNode( aFnContext );
#endif

    return ACI_FAILURE;
}

/*
 * BUGBUG : read 와 flush 는 static 으로 집어 넣어야 한다.
 *          일단은 모두 고치기 귀찮으니 그대로 두자.
 */

/**
 * ulnReadProtocol.
 *
 * @param[in] aFnContext
 * @param[in] aPtContext
 * @param[in] aTimeout
 *
 * 주어진 protocol context 에 수신된 패킷을 읽고, 해당하는 콜백을 호출한다.
 * 만약, semi-async prefetch 에 의해 비동기적으로 fetch protocol 을 요청하였다면,
 * fetch result 를 먼저 read 한다.
 */
ACI_RC ulnReadProtocol(ulnFnContext   *aFnContext,
                       ulnPtContext   *aPtContext,
                       acp_time_t      aTimeout)
{
    ulnDbc       *sDbc;
    ulnStmt      *sAsyncPrefetchStmt;
    ulnErrorMgr   sErrorMgr;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    /*
     * 연결 안되어 있으면 조용히 리턴
     */
    ACI_TEST_RAISE(ulnDbcIsConnected(sDbc) != ACP_TRUE, LABEL_CON_CLOSED);

    /*
     * ReadProtocol 할 필요 없어도 그냥 조용히 리턴
     */

    if(aPtContext->mNeedReadProtocol > 0)
    {
        /*
         * 콜백 호출
         */

        /**
         * PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
         * Should read CMP_OP_DB_FetchResult before, if asynchronous CMP_OP_DB_FetchV2 was sent
         */

        sAsyncPrefetchStmt = ulnDbcGetAsyncPrefetchStmt(sDbc);
        if (sAsyncPrefetchStmt != NULL)
        {
            ACI_TEST(ulnFetchReceiveFetchResultForSync(aFnContext,
                                                       aPtContext,
                                                       sAsyncPrefetchStmt)
                    != ACI_SUCCESS);
        }
        else
        {
            /* nothing to do */
        }

        ACI_TEST_RAISE( cmiRecv( &(aPtContext->mCmiPtContext),
                                 aFnContext,
                                 aTimeout)
                        != ACI_SUCCESS, LABEL_CM_ERR);

        aPtContext->mNeedReadProtocol = 0;

        /*
         * Note : 서버에서 에러가 전송되어 올 경우, ErrorResult 패킷이 수신된다.
         *        그렇지만, cmiReadProtocolAndCallback() 함수는 ACI_SUCCESS 를 리턴한다.
         *        그래서, 직접 Function Context 의 mSqlReturn 값을 체크해서 서버로부터의
         *        에러 전송 여부를 판단해야 한다.
         */
        ACI_TEST(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext)) == 0);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CON_CLOSED)
    {
        ulnErrorMgrSetUlError( &sErrorMgr,
                               ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                               "Server closed the connection." );

#ifdef COMPILE_SHARDCLI
        ulsdModuleOnCmError( aFnContext, sDbc, &sErrorMgr );
#else
        //PROJ-1645 UL FailOver.
        if (ulnFailoverDoSTF(aFnContext) == ACI_FAILURE)
        {
            ulnError(aFnContext,
                 ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                 "Server closed the connection.");
        }
        else
        {
            ulnError( aFnContext, ulERR_ABORT_FAILOVER_SUCCESS,
                      ulnErrorMgrGetErrorCode(&sErrorMgr),
                      ulnErrorMgrGetSQLSTATE(&sErrorMgr),
                      ulnErrorMgrGetErrorMessage(&sErrorMgr) );
        }
#endif /* COMPILE_SHARDCLI */
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnReadProtocol : object type is neither DBC nor STMT.");
    }

    ACI_EXCEPTION(LABEL_CM_ERR)
    {
        ulnErrorMgrSetCmError( sDbc, &sErrorMgr, aciGetErrorCode() );

#ifdef COMPILE_SHARDCLI
        ulsdModuleOnCmError(aFnContext, sDbc, &sErrorMgr);
#else
        //PROJ-1645 UL FailOver.
        if (ulnFailoverDoSTF(aFnContext) == ACI_FAILURE)
        {
            ulnErrHandleCmError(aFnContext, NULL);
        }
        else
        {
            ulnError( aFnContext, ulERR_ABORT_FAILOVER_SUCCESS,
                      ulnErrorMgrGetErrorCode(&sErrorMgr),
                      ulnErrorMgrGetSQLSTATE(&sErrorMgr),
                      ulnErrorMgrGetErrorMessage(&sErrorMgr) );
        }
#endif /* COMPILE_SHARDCLI */

        /*
         * cm 의 MARSHALL_ERROR 등이 발생했을 경우 여기서 에러가 리턴되고,
         * 프로토콜을 read 한 ulnFetch() 등의 함수의 ACI_EXCEPTION() 루틴에서
         * 다시한번 flush / read 를 하게 되는데, 이때 다시한번 read 를 하게 되므로
         * 무한대기를 하게 된다.
         *
         * 이를 방지하기 위해서 mNeedReadProtocol 을 ACP_FALSE 로 만들어 준다.
         */

        aPtContext->mNeedReadProtocol = 0;
    }

    ACI_EXCEPTION_END;

#ifdef COMPILE_SHARDCLI
    /* BUG-46092 */
    ulsdModuleErrorCheckAndAlignDataNode( aFnContext );
#endif

    return ACI_FAILURE;
}

/**
 * ulnReadProtocolAsync
 *
 * @param[in] aFnContext
 * @param[in] aPtContext
 * @param[in] aTimeout
 *
 * PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
 * 주어진 protocol context 에 수신된 패킷을 읽고, 해당하는 콜백을 호출한다.
 */
ACI_RC ulnReadProtocolAsync(ulnFnContext   *aFnContext,
                            ulnPtContext   *aPtContext,
                            acp_time_t      aTimeout)
{
    ulnDbc       *sDbc = NULL;
    ulnErrorMgr   sErrorMgr;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    /*
     * 연결 안되어 있으면 조용히 리턴
     */
    ACI_TEST_RAISE(ulnDbcIsConnected(sDbc) != ACP_TRUE, LABEL_CON_CLOSED);

    /*
     * ReadProtocol 할 필요 없어도 그냥 조용히 리턴
     */

    /*
     * 콜백 호출
     */

    ACI_TEST_RAISE( cmiRecv( &(aPtContext->mCmiPtContext),
                             aFnContext,
                             aTimeout)
                    != ACI_SUCCESS, LABEL_CM_ERR);

    /*
     * Note : 서버에서 에러가 전송되어 올 경우, ErrorResult 패킷이 수신된다.
     *        그렇지만, cmiReadProtocolAndCallback() 함수는 ACI_SUCCESS 를 리턴한다.
     *        그래서, 직접 Function Context 의 mSqlReturn 값을 체크해서 서버로부터의
     *        에러 전송 여부를 판단해야 한다.
     */
    ACI_TEST(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext)) == 0);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CON_CLOSED)
    {
        ulnErrorMgrSetUlError( &sErrorMgr,
                               ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                               "Server closed the connection." );

#ifdef COMPILE_SHARDCLI
        ulsdModuleOnCmError( aFnContext, sDbc, &sErrorMgr );
#else
        //PROJ-1645 UL FailOver.
        if (ulnFailoverDoSTF(aFnContext) == ACI_FAILURE)
        {
            ulnError(aFnContext,
                 ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                 "Server closed the connection.");
        }
        else
        {
            ulnError( aFnContext, ulERR_ABORT_FAILOVER_SUCCESS,
                      ulnErrorMgrGetErrorCode(&sErrorMgr),
                      ulnErrorMgrGetSQLSTATE(&sErrorMgr),
                      ulnErrorMgrGetErrorMessage(&sErrorMgr) );
        }
#endif /* COMPILE_SHARDCLI */
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnReadProtocol : object type is neither DBC nor STMT.");
    }

    ACI_EXCEPTION(LABEL_CM_ERR)
    {
        ulnErrorMgrSetCmError( sDbc, &sErrorMgr, aciGetErrorCode() );

#ifdef COMPILE_SHARDCLI
        ulsdModuleOnCmError( aFnContext, sDbc, &sErrorMgr );
#else
        //PROJ-1645 UL FailOver.
        if (ulnFailoverDoSTF(aFnContext) == ACI_FAILURE)
        {
            ulnErrHandleCmError(aFnContext, NULL);
        }
        else
        {
            ulnError( aFnContext, ulERR_ABORT_FAILOVER_SUCCESS,
                      ulnErrorMgrGetErrorCode(&sErrorMgr),
                      ulnErrorMgrGetSQLSTATE(&sErrorMgr),
                      ulnErrorMgrGetErrorMessage(&sErrorMgr) );
        }
#endif /* COMPILE_SHARDCLI */
    }

    ACI_EXCEPTION_END;

#ifdef COMPILE_SHARDCLI
    /* BUG-46092 */
    ulsdModuleErrorCheckAndAlignDataNode( aFnContext );
#endif

    return ACI_FAILURE;
}

/**
 * ulnFlushProtocol.
 *
 * @param[in] aFnContext
 * @param[in] aPtContext
 *
 * 주어진 protocol context 에 쓰인 패킷을 전송한다.
 */
ACI_RC ulnFlushProtocol(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    //fix PROJ-1749
    ulnDbc             *sDbc        = NULL;
    cmiProtocolContext *sCtx        = &aPtContext->mCmiPtContext; /* PROJ-2616 */
    cmbBlockIPCDA      *sWriteBlock = (cmbBlockIPCDA *)sCtx->mWriteBlock; /* PROJ-2616 */
    ulnErrorMgr         sErrorMgr;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        /* PROJ-2616 */
        sWriteBlock->mBlock.mDataSize = sWriteBlock->mBlock.mCursor;
        sWriteBlock->mBlock.mBlockSize = CMB_BLOCK_DEFAULT_SIZE - sizeof(cmbBlockIPCDA);
        /* Finalize to write data block. */
        (void)cmiFinalizeSendBufferForIPCDA(sCtx);
    }
    else
    {
        ACI_TEST_RAISE(cmiSend(&(aPtContext->mCmiPtContext), ACP_TRUE) != ACI_SUCCESS, LABEL_CM_ERR);
    }

    aPtContext->mNeedFlush = ACP_FALSE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CM_ERR)
    {
#ifdef COMPILE_SHARDCLI
        ulsdModuleOnCmError(aFnContext, sDbc, &sErrorMgr);
#else
        ulnErrorMgrSetCmError( sDbc, &sErrorMgr, aciGetErrorCode() );

        //PROJ-1645 UL FailOver.
        if (ulnFailoverDoSTF(aFnContext) == ACI_FAILURE)
        {
            ulnErrHandleCmError(aFnContext, aPtContext);
        }
        else
        {
            ulnError( aFnContext, ulERR_ABORT_FAILOVER_SUCCESS,
                      ulnErrorMgrGetErrorCode(&sErrorMgr),
                      ulnErrorMgrGetSQLSTATE(&sErrorMgr),
                      ulnErrorMgrGetErrorMessage(&sErrorMgr) );
        }
#endif /* COMPILE_SHARDCLI */
    }
    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnFlushProtol : object type is neither DBC nor STMT.");
    }


    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnFlushAndReadProtocol(ulnFnContext   *aFnContext,
                               ulnPtContext   *aPtContext,
                               acp_time_t      aTimeout)
{
    if (cmiGetLinkImpl(&(aPtContext->mCmiPtContext)) == CMI_LINK_IMPL_IPCDA)
    {
        (void)cmiFinalizeSendBufferForIPCDA(&aPtContext->mCmiPtContext);
        ACI_TEST(ulnReadProtocolIPCDA(aFnContext,
                                      aPtContext, 
                                      aTimeout) 
                 != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

        if ( aFnContext->mObjType == ULN_OBJ_TYPE_STMT )
        {
            ulsdStmtCallback(aFnContext->mHandle.mStmt);
        }

        if(aPtContext->mNeedReadProtocol > 0)
        {
            ACI_TEST(ulnReadProtocol(aFnContext, aPtContext, aTimeout) != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnInitializeProtocolContext(ulnFnContext *aFnContext,
                                    ulnPtContext *aPtContext,
                                    cmiSession   *aSession)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aSession);

    aPtContext->mNeedReadProtocol = 0;
    aPtContext->mNeedFlush        = ACP_FALSE;


    return ACI_SUCCESS;
}

/*
 * BUG-45286 PDO에서 프로토콜 전송을 지연시키기 위해 ulnFinalizeProtocolContext()를 Skip.
 *           ParamDataIn      (ulnParamData.c)
 *           DBFree with DROP (ulnFreeHandle.c)
 *           ParamInfoSetList (ulnExecute.c)
 */
ACI_RC ulnFinalizeProtocolContext(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnDbc *sDbc;

    ACI_TEST(aFnContext->mHandle.mObj == NULL);      //BUG-28184 : [CodeSonar] Null Pointer Dereference

    ACI_TEST_RAISE(cmiGetLinkImpl(&(aPtContext->mCmiPtContext)) == CMI_LINK_IMPL_IPCDA,
                   ContFinalizeCTX);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    if(aPtContext->mNeedFlush == ACP_TRUE)
    {
        ACI_TEST_RAISE(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS,
                       LABEL_PT_CTXT_FIN);
    }

    if(aPtContext->mNeedReadProtocol > 0)
    {
        /*
         * Note : ReadProtocol 에서 에러가 나도 finalize 해야 한다.
         *        그 에러는
         *          1. cm error
         *          2. sql error
         *        의 두가지 경우가 있는데 어찌되었든, finalize 해야 한다.
         */
        if(ulnDbcIsConnected(sDbc) == ACP_TRUE)
        {
            ACI_TEST_RAISE(ulnReadProtocol(aFnContext,
                                           aPtContext,
                                           sDbc->mConnTimeoutValue) != ACI_SUCCESS,
                           LABEL_PT_CTXT_FIN);
        }
    }

    ACI_EXCEPTION_CONT(ContFinalizeCTX);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_PT_CTXT_FIN)
    {
        /*
        if(cmiFinalizeProtocolContext(&(aPtContext->mCmiPtContext)) != ACI_SUCCESS)
        {
            ACI_RAISE(LABEL_CM_ERR);
        }
        */
    }
    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnFinalizeProtocolContext : object type is neither DBC nor STMT.");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ======================================
 * Protocol Write Functions
 * ======================================
 */

ACI_RC ulnWritePrepareREQ(ulnFnContext  *aFnContext,
                          ulnPtContext  *aProtocolContext,
                          acp_char_t    *aString,
                          acp_sint32_t   aLength,
                          acp_uint8_t    aExecMode)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aProtocolContext->mCmiPtContext);
    ulnStmt            *sStmt           = aFnContext->mHandle.mStmt;
    acp_uint8_t        *sRow            = (acp_uint8_t*)aString;
    acp_uint8_t         sPrepareMode    = ulnStmtGetPrepareMode(sStmt, aExecMode);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;
    acp_uint64_t        sValue8         = 0;
    acp_uint8_t         sValue1         = 0;

    sPacket.mOpID = CMP_OP_DB_PrepareV3;

    sStmt->mIsSimpleQuery = ACP_FALSE; /*PROJ-2616*/
   
    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx, 27, 27 + aLength);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_PrepareV3);
    CMI_WR4(sCtx, &(sStmt->mStatementID));
    CMI_WR1(sCtx, sPrepareMode);
    /* BUG-48775 Reserved 17 bytes */
    CMI_WR8(sCtx, &sValue8);
    CMI_WR8(sCtx, &sValue8);
    CMI_WR1(sCtx, sValue1);
    CMI_WR4(sCtx, (acp_uint32_t*)&aLength);

    ACI_TEST( cmiSplitWrite( sCtx,
                             aLength,
                             sRow )
              != ACI_SUCCESS );

    ACI_TEST(ulnWriteProtocol(aFnContext, aProtocolContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulnWriteColumnInfoGetREQ(ulnFnContext  *aFnContext,
                                ulnPtContext  *aPtContext,
                                acp_uint16_t   aColumnNumber)
{
    cmiProtocol                sPacket;
    cmiProtocolContext*        sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t               sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t                sState          = 0;

    sPacket.mOpID = CMP_OP_DB_ColumnInfoGet;

    CMI_WRITE_CHECK(sCtx, 9);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ColumnInfoGet);
    CMI_WR4(sCtx, &aFnContext->mHandle.mStmt->mStatementID);
    CMI_WR2(sCtx, &aFnContext->mHandle.mStmt->mCurrentResultSetID);
    CMI_WR2(sCtx, &aColumnNumber);

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

ACI_RC ulnWriteParamInfoGetREQ(ulnFnContext *aFnContext,
                               ulnPtContext *aPtContext,
                               acp_uint16_t  aParamNumber)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_ParamInfoGet;

    CMI_WRITE_CHECK(sCtx, 7);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ParamInfoGet);
    CMI_WR4(sCtx, &(aFnContext->mHandle.mStmt->mStatementID));
    CMI_WR2(sCtx, &aParamNumber);

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

ACI_RC ulnWriteParamInfoSetListREQ(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   acp_uint16_t  aParamCount)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sParamNumber;
    ulnBindInfo*        sBindInfo;
    acp_uint32_t        sDataType;
    ulnStmt            *sStmt = aFnContext->mHandle.mStmt;
    ulnDescRec         *sDescRecApd;
    acp_sint16_t        sCType          = 0;
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_ParamInfoSetList;

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx, 7 + (aParamCount * 20), 7 + (aParamCount * 22));
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ParamInfoSetList);
    CMI_WR4(sCtx, &(sStmt->mStatementID));
    CMI_WR2(sCtx, &aParamCount);

    for (sParamNumber = 1;
         sParamNumber <= aParamCount;
         sParamNumber++)
    {
        sDescRecApd = ulnStmtGetApdRec(sStmt, sParamNumber);

        /* BUG-46052 codesonar Null Pointer Dereference */
        ACI_TEST(sDescRecApd == NULL);

        sBindInfo = &(sDescRecApd->mBindInfo);
        sDataType = ulnTypeMap_MTYPE_MTD(sBindInfo->mMTYPE);

        CMI_WR2(sCtx, &sParamNumber);
        CMI_WR4(sCtx, &sDataType);
        CMI_WR4(sCtx, &(sBindInfo->mLanguage));
        CMI_WR1(sCtx, sBindInfo->mArguments);
        CMI_WR4(sCtx, (acp_uint32_t*)&(sBindInfo->mPrecision));
        CMI_WR4(sCtx, (acp_uint32_t*)&(sBindInfo->mScale));
        CMI_WR1(sCtx, ulnBindInfoCmParamInOutType(sBindInfo));

        /****************************************************************
         * PROJ-2616
         * 1) IPCDA-SIMPLEQUERY에서는 사용자 데이터의 CTYPE을 바로 받아서,
         * 데이터에 MT 타입으로 변환하여 저장한다. 따라서, CTYPE에 관한
         * 정보가 필요하다. 이것을 위하여 ulnWriteParamInfoSetListREQ에서
         * 해당 정보를 전송한다.
         *
         * 2) FastExecute에서 DATE와 같은 타입을 사용자 버퍼 타입에 맞게 변환하기
         * 위하여 실제 SQLCTYPE을 알 필요가 있다. 위의 CTYPE은 ULNCTYPE이다.
         ****************************************************************/
        if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
        {
            /* mCTYPE */
            sCType = ulnTypeMap_CTYPE_SQLC(sDescRecApd->mMeta.mCTYPE);
            CMI_WR2(sCtx, (acp_uint16_t*)&sCType);
        }
    }

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

ACI_RC ulnWriteParamDataInListREQForSimpleQuery(ulnFnContext *aFnContext,
                                                ulnPtContext *aPtContext,
                                                acp_uint8_t   aExecuteOption)
{
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    ulnStmt            *sStmt           = aFnContext->mHandle.mStmt;
    acp_uint32_t        sDataSizeCurPos = 0;
    acp_uint32_t        sCursorCurPos   = 0;
    acp_uint64_t        sDataSize       = 0;

    acp_uint32_t        sFromRowNumber  = 1;
    acp_uint32_t        sToRowNumber    = 1;
    acp_list_node_t    *sIterator       = NULL; /*PROJ-2616*/
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;
    ulnDbc             *sDbc            = NULL;
    acp_uint64_t        i               = 0;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx,
                               51 + 2 + (sDbc->mClientTouchNodeCount * 4),
                               51 + 2 + (sDbc->mClientTouchNodeCount * 4) +
                               2 + (sStmt->mAttrArd->mDescRecCount * 2));
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ParamDataInListV3);
    CMI_WR4(sCtx, &(sStmt->mStatementID));
    CMI_WR4(sCtx, &sFromRowNumber);
    CMI_WR4(sCtx, &sToRowNumber);
    CMI_WR1(sCtx, aExecuteOption);
    /* PROJ-2733-DistTxInfo */
    CMI_WR8(sCtx, &(sDbc->mSCN));  /* RequestSCN */
    CMI_WR8(sCtx, &(sDbc->mTxFirstStmtSCN));
    CMI_WR8(sCtx, (acp_uint64_t *)&(sDbc->mTxFirstStmtTime));
    CMI_WR1(sCtx, (acp_uint8_t)(sDbc->mDistLevel));
    /* TASK-7219 Non-shard DML */
    CMI_WR4(sCtx, (acp_uint32_t*)&(sDbc->mStmtExecSeqForShardTx));
    /* BUG-48489 */
    CMI_WR2(sCtx, &(sDbc->mClientTouchNodeCount));
    for (i = 0; i < sDbc->mClientTouchNodeCount; i++)
    {
        CMI_WR4(sCtx, &(sDbc->mClientTouchNodeArr[i]));
    }

    /* 아래는 데이터의 전체 크기를 쓰는 위치이다.
     * 현재 시점에서는 데이터의 크기를 알 수 없기 때문에 sDataSizeCurPos에 현재의
     * 위치를 기억 시키고, 아래의 ulnParamProcess_IPCDA_DATAs를 실행 후에,
     * 실제 ROW의 데이터 크기를 sDataSizeCurPos 지점에 기록하게 된다.
     */
    sDataSizeCurPos = sCtx->mWriteBlock->mCursor;
    sCtx->mWriteBlock->mCursor += 8;

    /* SimpleQuery-Fetch에서 실제 컬럼의 바인드 정보를 보고 데이터를
     * 디코딩하기 위해서 아래의 정보가 필요하다.
     */
    CMI_NE_WR2(sCtx, &sStmt->mAttrArd->mDescRecCount);
    ACP_LIST_ITERATE(&sStmt->mAttrArd->mDescRecList, sIterator)
    {
        CMI_NE_WR2(sCtx, &((ulnDescRec *)sIterator)->mIndex);
    }

    /* 사용자 입력 데이터의 기록 */
    ACI_TEST_RAISE(ulnParamProcess_IPCDA_DATAs(aFnContext,
                                               aPtContext,
                                               sFromRowNumber,
                                               &sDataSize) != ACI_SUCCESS,
                   err_param_process);

    /* ulnParamProcess_IPCDA_DATAs에서 반환 된 sDataSize를 sCursorCurPos에 기록한다. */
    sCursorCurPos = sCtx->mWriteBlock->mCursor;
    sCtx->mWriteBlock->mCursor = sDataSizeCurPos;
    CMI_WR8(sCtx, &sDataSize);
    /* 원래의 데이터 CURSOR로 복원 시킨다. */
    sCtx->mWriteBlock->mCursor = sCursorCurPos;

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, NULL) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* array 이면 에러건만 skip 하고 다음건 처리 */
    ACI_EXCEPTION(err_param_process)
    {
        ulnStmtIncreaseParamsProcessedValue(sStmt);
        ulnStmtSetAttrParamStatusValue(sStmt, sToRowNumber, SQL_PARAM_ERROR);
    }
    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnWriteParamDataInListREQForSimpleQuery : object type is neither DBC nor STMT.");
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulnWriteParamDataInListREQ(ulnFnContext *aFnContext,
                                  ulnPtContext *aPtContext,
                                  acp_uint32_t  aFromRowNumber,
                                  acp_uint32_t  aToRowNumber,
                                  acp_uint8_t   aExecuteOption)
{
    acp_uint64_t        sRowSize;
    acp_uint8_t        *sRow;

    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    ulnStmt            *sStmt           = aFnContext->mHandle.mStmt;

    acp_list_node_t    *sIterator       = NULL; /*PROJ-2616*/
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;
    acp_uint64_t        i               = 0;
    ulnDbc             *sDbc            = NULL;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    sRowSize = ulnStmtChunkGetSize( sStmt );
    sRow     = ulnStmtChunkGetData( sStmt );

    sPacket.mOpID = CMP_OP_DB_ParamDataInListV3;

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx,
                               51 + 2 + (sDbc->mClientTouchNodeCount * 4),
                               51 + 2 + (sDbc->mClientTouchNodeCount * 4) +
                               2 + (sStmt->mAttrArd->mDescRecCount * 2) + sRowSize);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ParamDataInListV3);
    CMI_WR4(sCtx, &(sStmt->mStatementID));
    CMI_WR4(sCtx, &aFromRowNumber);
    CMI_WR4(sCtx, &aToRowNumber);
    CMI_WR1(sCtx, aExecuteOption);
    /* PROJ-2733-DistTxInfo */
    CMI_WR8(sCtx, &(sDbc->mSCN));  /* RequestSCN */
    CMI_WR8(sCtx, &(sDbc->mTxFirstStmtSCN));
    CMI_WR8(sCtx, (acp_uint64_t *)&(sDbc->mTxFirstStmtTime));
    CMI_WR1(sCtx, (acp_uint8_t)(sDbc->mDistLevel));
    /* TASK-7219 Non-shard DML */
    CMI_WR4(sCtx, (acp_uint32_t*)&(sDbc->mStmtExecSeqForShardTx));
    /* BUG-48489 */
    CMI_WR2(sCtx, &(sDbc->mClientTouchNodeCount));
    for (i = 0; i < sDbc->mClientTouchNodeCount; i++)
    {
        CMI_WR4(sCtx, &(sDbc->mClientTouchNodeArr[i]));
    }
    CMI_WR8(sCtx, &sRowSize);

    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        /* SimpleQuery-Fetch에서 실제 컬럼의 바인드 정보를 보고 데이터를
         * 디코딩하기 위해서 아래의 정보가 필요하다.
         *
         * - 아래의 정보는 Prepare를 보내고 바로 Execute를 보내는
         * 경우에 SimpleQuery인지를 알 수 없기 때문에 IPCDA인 경우에는
         * 무조건 이 정보를 보내게 된다.
         */
        CMI_NE_WR2(sCtx, &sStmt->mAttrArd->mDescRecCount);
        ACP_LIST_ITERATE(&sStmt->mAttrArd->mDescRecList, sIterator)
        {
            CMI_NE_WR2(sCtx, &((ulnDescRec *)sIterator)->mIndex);
        }
    }
    ACI_TEST( cmiSplitWrite( sCtx,
                             sRowSize,
                             sRow )
              != ACI_SUCCESS );

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnWriteParamDataInListREQ : object type is neither DBC nor STMT.");
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    /* BUG-45441 */
    ulnStmtSetParamsProcessedValue(sStmt, 0);
    for (i = 0; i < ulnStmtGetAttrParamsetSize(sStmt); i++)
    {
        ulnStmtSetAttrParamStatusValue(sStmt, i, SQL_PARAM_UNUSED);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);
    
    return ACI_FAILURE;
}

/*
 * aDescRecApd 가 묘사하는 Descriptor Record 의
 * aRow 번째 row 에 해당하는 사용자 바인드 데이터를
 * BINDDATA IN REQ 에 실어서 보낸다.
 */
ACI_RC ulnWriteParamDataInREQ(ulnFnContext *aFnContext,
                              ulnPtContext *aPtContext,
                              acp_uint16_t  aParamNumber,
                              void         *aUserDataPtr,
                              acp_sint32_t  aUserOctetLength,
                              ulnMeta      *aApdMeta,
                              ulnMeta      *aIpdMeta)
{
    ulnStmt                 *sStmt    = aFnContext->mHandle.mStmt;
    acp_uint32_t             sRowSize;
    acp_uint8_t             *sRow;

    cmiProtocol              sPacket;
    cmiProtocolContext      *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint8_t              sBitData;
    acp_uint16_t             sState          = 0;
    acp_uint16_t             sOrgWriteCursor = CMI_GET_CURSOR(sCtx);

    ACP_UNUSED(aUserOctetLength);
    ACP_UNUSED(aIpdMeta);

    sRowSize = ulnStmtChunkGetSize( sStmt );
    sRow     = ulnStmtChunkGetData( sStmt );

    sPacket.mOpID = CMP_OP_DB_ParamDataIn;

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx, 11, 11 + sRowSize);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ParamDataIn);
    CMI_WR4(sCtx, &(aFnContext->mHandle.mStmt->mStatementID));
    CMI_WR2(sCtx, &aParamNumber);
    CMI_WR4(sCtx, &sRowSize);

    /*
     * 사용자가 바인드한 메모리 영역에서 데이터를 가져와서 쓴다.
     */

    /*
     * Note : BUG-16050
     *        mm 에서 bit 타입을 왼쪽에서 오른쪽으로 한비트씩 읽는다.
     *        즉, 0xc0 을 precision 3 인 cmtBit 로 해서 서버로 주게 되면,
     *              1 1 0
     *        의 비트 열로 인식하게 된다.
     *        그런 이유로, 일반적인 1, 즉, 0x01 을 cmtBit 에 넣어서 주면 비트 0 으로 인식해 버린다.
     *
     *        그래서 할 수 없이 스택에 sBitData 를 만들어서 거기에 0 혹은 1 을 집어넣어 둔다.
     *        cmtVariable 은 데이터의 포인터만 가지고 있고, cmiWriteProtocol 시에
     *        포인터가 가리키는 메모리를 읽어서 하기 때문에 ulnDataWriteBufferToPacket() 함수의
     *        안쪽에 sBitData 를 둘 수는 없는 노릇이다.
     *
     *        여기서 스택 변수인 sBitData 를 사용해도 되는 것이, cmiWriteProtocol() 에서
     *        포인터가 가리키는 메모리를 복사하기 때문에 그 이후에는 스택변수가 사라져도
     *        문제 없기 때문에 스택을 써도 된다. 단, 이 함수에서만 써야 한다.
     */
    if(ulnMetaGetCTYPE(aApdMeta) == ULN_CTYPE_BIT)
    {
        if(aUserDataPtr != NULL)
        {
            if(*(acp_uint8_t *)aUserDataPtr != 0)
            {
                sBitData = 0x80;
            }
            else
            {
                sBitData = 0;
            }

            aUserDataPtr = &sBitData;
        }
    }

    ACI_TEST( cmiSplitWrite( sCtx,
                             sRowSize,
                             sRow )
              != ACI_SUCCESS );

    /*
     * 패킷을 쓴다.
     */
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

ACI_RC ulnWriteFetchMoveREQ(ulnFnContext *aFnContext,
                            ulnPtContext *aPtContext,
                            acp_uint8_t   aWhence,
                            acp_sint64_t  aOffset)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_FetchMove;

    CMI_WRITE_CHECK(sCtx, 16);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_FetchMove);
    CMI_WR4(sCtx, &(aFnContext->mHandle.mStmt->mStatementID));
    CMI_WR2(sCtx, &(aFnContext->mHandle.mStmt->mCurrentResultSetID));
    CMI_WR1(sCtx, aWhence);
    CMI_WR8(sCtx, (acp_uint64_t*)&aOffset);

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

/* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
ACI_RC ulnWriteFetchREQ(ulnFnContext *aFnContext,
                        ulnPtContext *aPtContext,
                        acp_uint32_t  aRowCountToFetch,
                        acp_uint16_t  aFirstColumnIndex,
                        acp_uint16_t  aLastColumnIndex)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx  = &(aPtContext->mCmiPtContext);

    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint64_t         sReserved  = 0;
    acp_uint8_t          sState     = 0;

    sPacket.mOpID = CMP_OP_DB_FetchV3;

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx, 23, 23 + 1);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_FetchV3);
    CMI_WR4(sCtx, &(aFnContext->mHandle.mStmt->mStatementID));
    CMI_WR2(sCtx, &(aFnContext->mHandle.mStmt->mCurrentResultSetID));
    CMI_WR4(sCtx, &aRowCountToFetch);
    CMI_WR2(sCtx, &aFirstColumnIndex);
    CMI_WR2(sCtx, &aLastColumnIndex);
    CMI_WR8(sCtx, &sReserved);

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

/**
 * ulnExecWriteExecuteREQ.
 *
 * @param[in] aOption
 *  아래의 값들 중에 하나
 *      CMP_DB_EXECUTE_NOAMAL_EXECUTE
 *      CMP_DB_EXECUTE_ARRAY_EXECUTE
 *      CMP_DB_EXECUTE_ARRAY_BEGIN
 *      CMP_DB_EXECUTE_ARRAY_END
 */
ACI_RC ulnWriteExecuteREQ(ulnFnContext *aFnContext,
                          ulnPtContext *aPtContext,
                          acp_uint32_t  aStatementID,
                          acp_uint32_t  aRowNumber,
                          acp_uint8_t   aOption)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx             = &(aPtContext->mCmiPtContext);
    acp_uint32_t        sRowNumber       = aRowNumber + 1; // 1 베이스이다.
    acp_uint16_t        sOrgWriteCursor  = CMI_GET_CURSOR(sCtx);
    acp_uint32_t        sOldReadProtocol = aPtContext->mNeedReadProtocol;
    ulnStmt            *sStmt            = aFnContext->mHandle.mStmt;
    acp_list_node_t    *sIterator        = NULL; /*PROJ-2616*/
    acp_uint8_t         sState           = 0;
    ulnDbc             *sDbc             = NULL;
    acp_uint32_t        i                = 0;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    sPacket.mOpID = CMP_OP_DB_ExecuteV3;

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx,
                               39 + 2 + (sDbc->mClientTouchNodeCount * 4),
                               39 + 2 + (sDbc->mClientTouchNodeCount * 4) +
                               2 + (sStmt->mAttrArd->mDescRecCount * 2));
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ExecuteV3);
    CMI_WR4(sCtx, &aStatementID);
    CMI_WR4(sCtx, &sRowNumber);
    CMI_WR1(sCtx, aOption);
    /* PROJ-2733-DistTxInfo */
    CMI_WR8(sCtx, &(sDbc->mSCN));  /* RequestSCN */
    CMI_WR8(sCtx, &(sDbc->mTxFirstStmtSCN));
    CMI_WR8(sCtx, (acp_uint64_t *)&(sDbc->mTxFirstStmtTime));
    CMI_WR1(sCtx, (acp_uint8_t)(sDbc->mDistLevel));
    /* TASK-7219 Non-shard DML */
    CMI_WR4(sCtx, (acp_uint32_t*)&(sDbc->mStmtExecSeqForShardTx));
    /* BUG-48315 */
    CMI_WR2(sCtx, &(sDbc->mClientTouchNodeCount));
    for (i = 0; i < sDbc->mClientTouchNodeCount; i++)
    {
        CMI_WR4(sCtx, &(sDbc->mClientTouchNodeArr[i]));
    }

   /* PROJ-2616 
    * SimpleQuery-Fetch에서 실제 컬럼의 바인드 정보를 보고 데이터를
     * 디코딩하기 위해서 아래의 정보가 필요하다.
     *
     * - 아래의 정보는 Prepare를 보내고 바로 Execute를 보내는
     * 경우에 SimpleQuery인지를 알 수 없기 때문에 IPCDA인 경우에는
     * 무조건 이 정보를 보내게 된다.
     */
    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        CMI_NE_WR2(sCtx, &sStmt->mAttrArd->mDescRecCount);
        ACP_LIST_ITERATE(&sStmt->mAttrArd->mDescRecList, sIterator)
        {
            CMI_NE_WR2(sCtx, &((ulnDescRec *)sIterator)->mIndex);
        }
    }

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    if( aOption == CMP_DB_EXECUTE_ARRAY_BEGIN ||
        aOption == CMP_DB_EXECUTE_ARRAY_END   ||
        aOption == CMP_DB_EXECUTE_ATOMIC_EXECUTE ||
        aOption == CMP_DB_EXECUTE_ATOMIC_BEGIN )
    {
        aPtContext->mNeedReadProtocol = sOldReadProtocol;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnWriteExecuteREQ : object type is neither DBC nor STMT.");
    }
    ACI_EXCEPTION_END;

    if( aOption == CMP_DB_EXECUTE_ARRAY_BEGIN ||
        aOption == CMP_DB_EXECUTE_ARRAY_END   ||
        aOption == CMP_DB_EXECUTE_ATOMIC_EXECUTE ||
        aOption == CMP_DB_EXECUTE_ATOMIC_BEGIN )
    {
        aPtContext->mNeedReadProtocol = sOldReadProtocol;
    }

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulnWritePlanGetREQ(ulnFnContext *aFnContext, ulnPtContext *aPtContext, acp_uint32_t aStatementID)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState = 0;

    sPacket.mOpID = CMP_OP_DB_PlanGet;

    CMI_WRITE_CHECK(sCtx, 5);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_PlanGet);
    CMI_WR4(sCtx, &aStatementID);

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

ACI_RC ulnWriteTransactionREQ(ulnFnContext     *aFnContext,
                              ulnPtContext     *aPtContext,
                              ulnTransactionOp  aTransactOp)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_Transaction;

    CMI_WRITE_CHECK(sCtx, 2);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_Transaction);

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

/*
 * ===================================
 * LOB packet write functions
 * ===================================
 */

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
ACI_RC ulnWriteLobPutBeginREQ(ulnFnContext *aFnContext,
                              ulnPtContext *aPtContext,
                              acp_uint64_t  aLocatorID,
                              acp_uint32_t  aOffset,
                              acp_uint32_t  aSize)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    ACI_TEST(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA);

    sPacket.mOpID = CMP_OP_DB_LobPutBegin;

    CMI_WRITE_CHECK(sCtx, 17);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobPutBegin);
    CMI_WR8(sCtx, &aLocatorID);
    CMI_WR4(sCtx, &aOffset);
    CMI_WR4(sCtx, &aSize);

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

ACI_RC ulnWriteLobPutEndREQ(ulnFnContext *aFnContext, ulnPtContext *aPtContext, acp_uint64_t aLocatorID)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ACI_TEST(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA);

    sPacket.mOpID = CMP_OP_DB_LobPutEnd;

    CMI_WRITE_CHECK(sCtx, 9);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobPutEnd);
    CMI_WR8(sCtx, &aLocatorID);

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

ACI_RC ulnWriteLobFreeREQ(ulnFnContext *aFnContext, ulnPtContext *aPtContext, acp_uint64_t aLocator)
{
    ulnDbc              *sDbc;
    cmiProtocolContext  *sCtx            = &(aPtContext->mCmiPtContext);
    cmiProtocol          sPacket;
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    ACI_TEST(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sPacket.mOpID = CMP_OP_DB_LobFree;

    CMI_WRITE_CHECK(sCtx, 9);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobFree);
    CMI_WR8(sCtx, &aLocator);

    if(sDbc->mConnType == ULN_CONNTYPE_IPC || sDbc->mConnType == ULN_CONNTYPE_IPCDA)
    {
        gUlnCmProtocolTable[CMP_OP_DB_LobFree].mNeedResult = 1;
    }

    /*
     * 패킷 제작
     */
    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulnWriteLobFreeAllREQ(ulnFnContext *aFnContext,
                             ulnPtContext *aPtContext,
                             acp_uint64_t  aLocatorCount,
                             void         *aBuffer )
{
    ulnDbc              *sDbc;
    cmiProtocol          sPacket;
    acp_uint32_t         sCount;
    acp_uint64_t        *sBuffer         = (acp_uint64_t*)aBuffer;
    cmiProtocolContext  *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    ACI_TEST(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sPacket.mOpID = CMP_OP_DB_LobFreeAll;

    CMI_WRITE_CHECK(sCtx, 9 + (aLocatorCount * 8));
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobFreeAll);
    CMI_WR8(sCtx, &aLocatorCount);

    for( sCount = 0; sCount < aLocatorCount; sCount++ )
    {
        CMI_WR8(sCtx, &(sBuffer[sCount]));
    }

    if(sDbc->mConnType == ULN_CONNTYPE_IPC || sDbc->mConnType == ULN_CONNTYPE_IPCDA)
    {
        gUlnCmProtocolTable[CMP_OP_DB_LobFreeAll].mNeedResult = 1;
    }

    /*
     * 패킷 제작
     */
    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

/*
 * Getting lob data from the server
 */

ACI_RC ulnWriteLobGetREQ(ulnFnContext *aFnContext,
                         ulnPtContext *aPtContext,
                         acp_uint64_t  aLobLocator,
                         acp_uint32_t  aStartingOffset,
                         acp_uint32_t  aSizeToGet)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ACI_TEST(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA);

    sPacket.mOpID = CMP_OP_DB_LobGet;

    CMI_WRITE_CHECK(sCtx, 17);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobGet);
    CMI_WR8(sCtx, &aLobLocator);
    CMI_WR4(sCtx, &aStartingOffset);
    CMI_WR4(sCtx, &aSizeToGet);

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

/*
 * Getting Lob Size
 */

ACI_RC ulnWriteLobGetSizeREQ(ulnFnContext *aFnContext, ulnPtContext *aPtContext, acp_uint64_t aLocator)
{
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t          sState          = 0;

    ACI_TEST(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA);

    sPacket.mOpID = CMP_OP_DB_LobGetSizeV3;  /* BUG-48028 */

    CMI_WRITE_CHECK(sCtx, 9);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobGetSizeV3);
    CMI_WR8(sCtx, &aLocator);
    
    /*
     * 패킷 쓰기
     */
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

/* PROJ-2047 Strengthening LOB - Added Interfaces */
ACI_RC ulnWriteLobTrimREQ(ulnFnContext *aFnContext,
                          ulnPtContext *aPtContext,
                          acp_uint64_t  aLocatorID,
                          acp_uint32_t  aOffset)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    ACI_TEST(cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA);

    sPacket.mOpID = CMP_OP_DB_LobTrim;

    CMI_WRITE_CHECK(sCtx, 13);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_LobTrim);
    CMI_WR8(sCtx, &aLocatorID);
    CMI_WR4(sCtx, &aOffset);

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

/*
 * ulnWritePropertySetREQ.
 *
 * protocol context 에 Property Set Req 를 쓰기만 한다.
 */
ACI_RC ulnWritePropertySetREQ(ulnFnContext  *aContext,
                              ulnPtContext  *aPtContext,
                              ulnPropertyId  aCmPropertyId,
                              void          *aValuePtr)
{
    cmiProtocolContext       *sCtx = &(aPtContext->mCmiPtContext);
    acp_uint8_t               s8   = 0;
    acp_uint32_t              s32  = 0;
    acp_uint64_t              s64  = 0;
    acp_uint32_t              sLen = 0;
    cmiProtocol               sPacket;
    acp_uint16_t              sID             = (acp_uint16_t) aCmPropertyId;
    acp_uint16_t              sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t               sState          = 0;

    /* 
     * BUG-41793 Keep a compatibility among tags
     *
     * CMP_OP_DB_PropertySetV2(1) | PropetyID(2) | ValueLen(4) | Value(?)
     */
    acp_uint32_t         sValueLen = 0;
    acp_uint8_t          sPropertySetPrefixLen = 1 + 2 + 4;  /* PROJ-2733-Protocol */

    /*
     * protocol 초기화
     */

    /* PROJ-2733-Protocol */
    sPacket.mOpID = CMP_OP_DB_PropertySetV3;  

    switch(sID)
    {
        /*
         * Null terminated string
         */
        case ULN_PROPERTY_CLIENT_PACKAGE_VERSION:
        case ULN_PROPERTY_CLIENT_TYPE:
        case ULN_PROPERTY_DATE_FORMAT:
        case ULN_PROPERTY_NLS:
        case ULN_PROPERTY_NLS_CHARACTERSET:       /* PROJ-1579 NCHAR */
        case ULN_PROPERTY_NLS_NCHAR_CHARACTERSET: /* PROJ-1579 NCHAR */
        case ULN_PROPERTY_APP_INFO:
        case ULN_PROPERTY_FAILOVER_SOURCE: /* BUG-31390 Failover info for v$session */
        case ULN_PROPERTY_TIME_ZONE:              /* PROJ-2209 DBTIMEZONE */
        case ULN_PROPERTY_NLS_TERRITORY: // PROJ-2727
        case ULN_PROPERTY_NLS_ISO_CURRENCY:
        case ULN_PROPERTY_NLS_CURRENCY:
        case ULN_PROPERTY_NLS_NUMERIC_CHARACTERS:
        case ULN_PROPERTY_INVOKE_USER: /* BUG-47862 AUTHID Definer for SHARD */
            
            sLen = acpCStrLen((acp_char_t *)aValuePtr, ACP_SINT32_MAX);
            sValueLen = 4 + sLen;

            CMI_WRITE_CHECK(sCtx, sPropertySetPrefixLen + sValueLen);
            sState = 1;

            CMI_WOP(sCtx, sPacket.mOpID);
            CMI_WR2(sCtx, &sID);
            CMI_WR4(sCtx, &sValueLen);
            CMI_WR4(sCtx, &sLen);
            CMI_WCP(sCtx, aValuePtr, sLen);
            break;

            /*
             * acp_uint8_t
             */
        case ULN_PROPERTY_AUTOCOMMIT:
        case ULN_PROPERTY_EXPLAIN_PLAN:
        case ULN_PROPERTY_MESSAGE_CALLBACK:  /* BUG-46019 */
            s8 = (acp_uint8_t)((acp_ulong_t)aValuePtr);
            sValueLen = 1;

            CMI_WRITE_CHECK(sCtx, sPropertySetPrefixLen + sValueLen);
            sState = 1;

            CMI_WOP(sCtx, sPacket.mOpID);
            CMI_WR2(sCtx, &sID);
            CMI_WR4(sCtx, &sValueLen);
            CMI_WR1(sCtx, s8);
            break;

            /*
             * acp_uint32_t
             */
        case ULN_PROPERTY_OPTIMIZER_MODE:
        case ULN_PROPERTY_HEADER_DISPLAY_MODE:
        case ULN_PROPERTY_STACK_SIZE:
        case ULN_PROPERTY_IDLE_TIMEOUT:
        case ULN_PROPERTY_QUERY_TIMEOUT:
        /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case ULN_PROPERTY_DDL_TIMEOUT:
        case ULN_PROPERTY_FETCH_TIMEOUT:
        case ULN_PROPERTY_UTRANS_TIMEOUT:
        case ULN_PROPERTY_NLS_NCHAR_LITERAL_REPLACE: /* PROJ-1579 NCHAR */
        case ULN_PROPERTY_MAX_STATEMENTS_PER_SESSION: /* BUG-31144 */
        case ULN_PROPERTY_LOB_CACHE_THRESHOLD: /* PROJ-2047 Strengthening LOB - LOBCACHE */
        case ULN_PROPERTY_COMMIT_WRITE_WAIT_MODE: // PROJ-2727 add connect attr
        case ULN_PROPERTY_ST_OBJECT_BUFFER_SIZE:
        case ULN_PROPERTY_PARALLEL_DML_MODE:            
        case ULN_PROPERTY_NLS_NCHAR_CONV_EXCP:
        case ULN_PROPERTY_AUTO_REMOTE_EXEC:
        case ULN_PROPERTY_TRCLOG_DETAIL_PREDICATE:
        case ULN_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ:
        case ULN_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ:
        case ULN_PROPERTY_QUERY_REWRITE_ENABLE:
        case ULN_PROPERTY_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT:
        case ULN_PROPERTY_RECYCLEBIN_ENABLE:
        case ULN_PROPERTY___USE_OLD_SORT:
        case ULN_PROPERTY_ARITHMETIC_OPERATION_MODE:
        case ULN_PROPERTY_RESULT_CACHE_ENABLE:
        case ULN_PROPERTY_TOP_RESULT_CACHE_MODE:
        case ULN_PROPERTY_OPTIMIZER_AUTO_STATS:
        case ULN_PROPERTY___OPTIMIZER_TRANSITIVITY_OLD_RULE:
        case ULN_PROPERTY_OPTIMIZER_PERFORMANCE_VIEW:
        case ULN_PROPERTY_REPLICATION_DDL_SYNC:
        case ULN_PROPERTY_REPLICATION_DDL_SYNC_TIMEOUT:
        case ULN_PROPERTY___PRINT_OUT_ENABLE:
        case ULN_PROPERTY_TRCLOG_DETAIL_SHARD:
        case ULN_PROPERTY_SERIAL_EXECUTE_MODE:
        case ULN_PROPERTY_TRCLOG_DETAIL_INFORMATION:
        case ULN_PROPERTY___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE:
        case ULN_PROPERTY_NORMALFORM_MAXIMUM:
        case ULN_PROPERTY___REDUCE_PARTITION_PREPARE_MEMORY:
        case ULN_PROPERTY_TRANSACTIONAL_DDL:
        case ULN_PROPERTY_GLOBAL_DDL:
        case ULN_PROPERTY_INDOUBT_FETCH_TIMEOUT:
        case ULN_PROPERTY___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD: /* BUG-48132 */
        case ULN_PROPERTY___OPTIMIZER_BUCKET_COUNT_MAX        : /* BUG-48161 */
        case ULN_PROPERTY_DDL_LOCK_TIMEOUT:
        case ULN_PROPERTY___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION: /* BUG-48348 */
            s32 = (acp_uint32_t)((acp_ulong_t)aValuePtr);
            sValueLen = 4;

            CMI_WRITE_CHECK(sCtx, sPropertySetPrefixLen + sValueLen);
            sState = 1;

            CMI_WOP(sCtx, sPacket.mOpID);
            CMI_WR2(sCtx, &sID);
            CMI_WR4(sCtx, &sValueLen);
            CMI_WR4(sCtx, &s32);
            break;

        case ULN_PROPERTY_SHARD_PIN: /* PROJ-2660 */
        case ULN_PROPERTY_SHARD_META_NUMBER: /* BUG-46090 Meta Node SMN 전파 */
        case ULN_PROPERTY_REBUILD_SHARD_META_NUMBER:
        case ULN_PROPERTY_TRX_UPDATE_MAX_LOGSIZE: // PROJ-2727
            
            s64 = (acp_uint64_t)((acp_ulong_t)aValuePtr);
            sValueLen = 8;

            CMI_WRITE_CHECK(sCtx, sPropertySetPrefixLen + sValueLen);
            sState = 1;

            CMI_WOP(sCtx, sPacket.mOpID);
            CMI_WR2(sCtx, &sID);
            CMI_WR4(sCtx, &sValueLen);
            CMI_WR8(sCtx, &s64);
            break;

            /* BUG-39817 */
        case ULN_PROPERTY_ISOLATION_LEVEL:
            switch ((acp_uint32_t)((acp_ulong_t)aValuePtr))
            {
                case SQL_TXN_READ_COMMITTED:
                    s32 = ULN_SERVER_ISOLATION_LEVEL_READ_COMMITTED;
                    break;

                case SQL_TXN_REPEATABLE_READ:
                    s32 = ULN_SERVER_ISOLATION_LEVEL_REPEATABLE_READ;
                    break;

                case SQL_TXN_SERIALIZABLE:
                    s32 = ULN_SERVER_ISOLATION_LEVEL_SERIALIZABLE;
                    break;

                default:
                    ACI_RAISE(LABEL_INVALID_ATTR_VALUE);
                    break;
            }

            sValueLen = 4;

            CMI_WRITE_CHECK(sCtx, sPropertySetPrefixLen + sValueLen);
            sState = 1;

            CMI_WOP(sCtx, sPacket.mOpID);
            CMI_WR2(sCtx, &sID);
            CMI_WR4(sCtx, &sValueLen);
            CMI_WR4(sCtx, &s32);
            break;

            /*
             * acp_uint64_t
             */
        case ULN_PROPERTY_CLIENT_PROTOCOL_VERSION:
        case ULN_PROPERTY_CLIENT_PID:
            sValueLen = 8;

            CMI_WRITE_CHECK(sCtx, sPropertySetPrefixLen + sValueLen);
            sState = 1;

            CMI_WOP(sCtx, sPacket.mOpID);
            CMI_WR2(sCtx, &sID);
            CMI_WR4(sCtx, &sValueLen);
            CMI_WR8(sCtx, (acp_ulong_t*)aValuePtr);
            break;

        case ULN_PROPERTY_SHARD_LINKER_TYPE: /* PROJ-2638 shard native linker */
        case ULN_PROPERTY_GLOBAL_TRANSACTION_LEVEL: /* PROJ-2660 */
        case ULN_PROPERTY_SHARD_CLIENT:
        case ULN_PROPERTY_SHARD_SESSION_TYPE:
        case ULN_PROPERTY_SHARD_INTERNAL_LOCAL_OPERATION:
        case ULN_PROPERTY_SHARD_STATEMENT_RETRY:
        case ULN_PROPERTY_INDOUBT_FETCH_METHOD:
            s8 = (acp_uint8_t)((acp_ulong_t)aValuePtr);
            sValueLen = 1;

            CMI_WRITE_CHECK( sCtx, sPropertySetPrefixLen + sValueLen );  /* PROJ-2733-Protocol */
            sState = 1;

            CMI_WOP( sCtx, sPacket.mOpID );
            CMI_WR2( sCtx, &sID );
            CMI_WR4( sCtx, &sValueLen );
            CMI_WR1( sCtx, s8 );
            break;

        case ULN_PROPERTY_SHARD_NODE_NAME: /* PROJ-2638 shard native linker */
            sLen = acpCStrLen( (acp_char_t *)aValuePtr, ULSD_MAX_NODE_NAME_LEN );
            sValueLen = sLen + 4; /* data'length + sizeof int */

            CMI_WRITE_CHECK( sCtx, sPropertySetPrefixLen + sValueLen );  /* PROJ-2733-Protocol */
            sState = 1;

            CMI_WOP( sCtx, sPacket.mOpID );
            CMI_WR2( sCtx, &sID );
            CMI_WR4( sCtx, &sValueLen );
            CMI_WR4( sCtx, &sLen );
            CMI_WCP( sCtx, aValuePtr, sLen );
            break;

        case ULN_PROPERTY_SHARD_CLIENT_CONNECTION_REPORT:   /* BUG-46092 */
            /* BUG-46785 derpecated property */
            ACI_RAISE(LABEL_INVALID_ATTR_OPTION);
            break;

        default:
            /*
             * HY092 : Invalid attribute/option identifier
             */
            ACI_RAISE(LABEL_INVALID_ATTR_OPTION);
            break;
    }

    /*
     * 패킷 쓰기
     */
    ACI_TEST(ulnWriteProtocol(aContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_ATTR_OPTION)
    {
        /*
         * HY092 : Invalid attribute/option identifier
         */
        ulnError(aContext, ulERR_ABORT_INVALID_ATTR_OPTION, aCmPropertyId);
    }
    ACI_EXCEPTION(LABEL_INVALID_ATTR_VALUE);
    {
        ulnError(aContext, ulERR_ABORT_INVALID_ATTRIBUTE_VALUE);
    }
    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

/*
 * ulnWritePropertyGetREQ.
 */
ACI_RC ulnWritePropertyGetREQ(ulnFnContext  *aContext,
                              ulnPtContext  *aPtContext,
                              ulnPropertyId  aCmPropertyId)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sID             = (acp_uint16_t) aCmPropertyId;
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_PropertyGet;

    CMI_WRITE_CHECK(sCtx, 3);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_PropertyGet);
    CMI_WR2(sCtx, &sID);

    /* 1. write request */
    ACI_TEST(ulnWriteProtocol(aContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulnSendConnectAttr(ulnFnContext  *aContext,
                          ulnPropertyId  aCmPropertyID,
                          void          *aValuePtr)
{
    ULN_FLAG(sNeedPtContextFin);

    ulnDbc              *sDbc;
    acp_time_t       sTimeout;

    ULN_FNCONTEXT_GET_DBC( aContext, sDbc );

    cmiProtocolContext *sCtx = NULL;

    /* does not call protocol if no link */
    if (ulnDbcIsConnected(sDbc) == ACP_TRUE)
    {
        /*
         * protocol context 초기화.
         */
        //fix BUG-17722
        ACI_TEST(ulnInitializeProtocolContext(aContext,
                                              &(sDbc->mPtContext),
                                              &(sDbc->mSession))
                 != ACI_SUCCESS);

        ULN_FLAG_UP(sNeedPtContextFin);

        /*
         * 적절한 패킷을 만들어서 protocol context 에 쓴다.
         */
        ACI_TEST(ulnWritePropertySetREQ(aContext, &(sDbc->mPtContext),
                                        aCmPropertyID, aValuePtr)
                 != ACI_SUCCESS);

        /*
         * 실제 전송
         */
        ACI_TEST(ulnFlushProtocol(aContext, &(sDbc->mPtContext) ) != ACI_SUCCESS);

        /*
         * 응답 대기
         */
        sTimeout = acpTimeFrom(ulnDbcGetLoginTimeout(sDbc), 0);

        sCtx = &sDbc->mPtContext.mCmiPtContext;
        if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
        {
            ACI_TEST(ulnReadProtocolIPCDA(aContext, &(sDbc->mPtContext), sTimeout) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST(ulnReadProtocol(aContext, &(sDbc->mPtContext), sTimeout) != ACI_SUCCESS);
        }

        /*
         * protocol context 정리
         */
        ULN_FLAG_DOWN(sNeedPtContextFin);

        ACI_TEST(ulnFinalizeProtocolContext(aContext,&(sDbc->mPtContext)) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedPtContextFin)
    {
        ulnFinalizeProtocolContext(aContext,&(sDbc->mPtContext));
    }

    return ACI_FAILURE;
}

/* PROJ-2177 User Interface - Cancel */
ACI_RC ulnWriteCancelREQ(ulnFnContext  *aFnContext,
                         ulnPtContext  *aPtContext,
                         acp_uint32_t  aStatementID)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_Cancel;

    CMI_WRITE_CHECK(sCtx, 5);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_Cancel);
    CMI_WR4(sCtx, &aStatementID);

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

ACI_RC ulnWritePrepareByCIDREQ(ulnFnContext  *aFnContext,
                               ulnPtContext  *aProtocolContext,
                               acp_char_t    *aString,
                               acp_sint32_t   aLength)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aProtocolContext->mCmiPtContext);
    ulnStmt            *sStmt           = aFnContext->mHandle.mStmt;
    acp_uint8_t        *sRow            = (acp_uint8_t*)aString;
    acp_uint64_t        sRowSize        = aLength;
    acp_uint8_t         sPrepareMode    = ulnStmtGetPrepareMode(sStmt, CMP_DB_PREPARE_MODE_EXEC_DIRECT);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;
    acp_uint64_t        sValue8         = 0;
    acp_uint8_t         sValue1         = 0;

    /* PROJ-1381 FAC, PROJ-1789 Updatable Scrollable Cursor */

    sPacket.mOpID = CMP_OP_DB_PrepareByCIDV3;

    sStmt->mIsSimpleQuery = ACP_FALSE; /*PROJ-2616*/

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    CMI_WRITE_CHECK_WITH_IPCDA(sCtx, 27, 27 + sRowSize);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_PrepareByCIDV3);
    CMI_WR4(sCtx, &(sStmt->mStmtCID));
    CMI_WR1(sCtx, sPrepareMode);
    /* BUG-48775 Reserved 17 bytes */
    CMI_WR8(sCtx, &sValue8);
    CMI_WR8(sCtx, &sValue8);
    CMI_WR1(sCtx, sValue1);
    CMI_WR4(sCtx, (acp_uint32_t*)&aLength);

    ACI_TEST( cmiSplitWrite( sCtx,
                             sRowSize,
                             sRow )
              != ACI_SUCCESS );

    ACI_TEST(ulnWriteProtocol(aFnContext, aProtocolContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulnWriteCancelByCIDREQ(ulnFnContext  *aFnContext,
                              ulnPtContext  *aPtContext,
                              acp_uint32_t  aStmtCID)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx     = &(aPtContext->mCmiPtContext);
    acp_uint8_t        sState = 0;

    sPacket.mOpID = CMP_OP_DB_CancelByCID;

    CMI_WRITE_CHECK(sCtx, 5);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_CancelByCID);
    CMI_WR4(sCtx, &aStmtCID);

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
        (void)cmiFinalizeSendBufferForIPCDA(sCtx);
    }

    return ACI_FAILURE;
}

/* BUG-46785 Shard statement partial rollback */
ACI_RC ulnWriteSetSavepointREQ( ulnFnContext     *aFnContext,
                                ulnPtContext     *aPtContext,
                                const acp_char_t *aSavepointName,
                                acp_uint32_t      aSavepointNameLength )
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_SetSavepoint;

    CMI_WRITE_CHECK( sCtx, 1 +                      /* OP */
                           4 +                      /* length */
                           aSavepointNameLength );  /* savepoint */
    sState = 1;

    CMI_WOP( sCtx, CMP_OP_DB_SetSavepoint );
    CMI_WR4( sCtx, &aSavepointNameLength );
    CMI_WCP( sCtx, aSavepointName, aSavepointNameLength );

    ACI_TEST( ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && ( cmiGetLinkImpl( sCtx ) == CMN_LINK_IMPL_IPCDA ) )
    {
        ulnError( aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE );
    }

    CMI_SET_CURSOR( sCtx, sOrgWriteCursor );

    return ACI_FAILURE;
}

/* BUG-46785 Shard statement partial rollback */
ACI_RC ulnWriteRollbackToSavepointREQ( ulnFnContext     *aFnContext,
                                       ulnPtContext     *aPtContext,
                                       const acp_char_t *aSavepointName,
                                       acp_uint32_t      aSavepointNameLength )
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;

    sPacket.mOpID = CMP_OP_DB_RollbackToSavepoint;

    CMI_WRITE_CHECK( sCtx, 1 +                      /* OP */
                           4 +                      /* length */
                           aSavepointNameLength );  /* savepoint */
    sState = 1;

    CMI_WOP( sCtx, CMP_OP_DB_RollbackToSavepoint );
    CMI_WR4( sCtx, &aSavepointNameLength );
    CMI_WCP( sCtx, aSavepointName, aSavepointNameLength );

    ACI_TEST( ulnWriteProtocol( aFnContext, aPtContext, &sPacket ) != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && ( cmiGetLinkImpl( sCtx ) == CMN_LINK_IMPL_IPCDA ) )
    {
        ulnError( aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE );
    }

    CMI_SET_CURSOR( sCtx, sOrgWriteCursor );

    return ACI_FAILURE;
}
