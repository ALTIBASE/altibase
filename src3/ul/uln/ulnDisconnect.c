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
#include <ulnDisconnect.h>

/*
 * ULN_SFID_67
 * SQLDisconnect(), DBC 상태전이 함수 : C3, C4, C5
 *
 * 모두 성공시에 C2 로 전이함.
 */
ACI_RC ulnSFID_67(ulnFnContext *aFnContext)
{
    acp_list_node_t *sIterator;
    ulnStmt         *sStmt;
    ulnDbc          *sDbc;

    sDbc = aFnContext->mHandle.mDbc;

    if(aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        /*
         * 들어갈 때 : 일단 하위의 모든 STMT 의 상태도 봐야 한다.
         *             하나라도 통과 못하면 통과 못하는 거다.
         */

        ACP_LIST_ITERATE(&(sDbc->mStmtList), sIterator)
        {
            /*
             * Note : 이 경우는 상당히 특이한 경우로써, 하위의 STMT 들의 상태에 신경을 써야
             *        한다. 왜냐하면 disconnect 하고 나서 stmt 핸들들을 모두 free 할 것이기
             *        때문이다. 그런데, 각각의 stmt 에 대해서 상태머신을 부를 수도 없는 노릇이다.
             *        왜냐하면 지금 현재의 context 는 mHandle 이 DBC 이기 때문이다. context 를
             *        하나 더 생성할 수도 없는 노릇이고... 그렇다고, SQLFreeHandle() 의 경우처럼
             *        간단히 상위 핸들의 상태만 참조하면 되는 것도 아니니 말이다.
             *        일단 일일이 하위 stmt 의 상태를 체크해서 하나라도 아니면 에러 내고
             *        리턴하도록, 즉, 더 이상 함수에 진입하지 못하도록 하였다.
             */
            sStmt = (ulnStmt *)sIterator;
            ACI_TEST_RAISE(ULN_OBJ_GET_STATE(sStmt) >= ULN_S_S8, LABEL_HY010);
        }
    }
    else
    {
        /*
         * 빠져나갈 때 : SQL_SUCCESS 를 리턴할 경우 상태 전이를 한다.
         */
        if(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext)) != 0)
        {
            ULN_OBJ_SET_STATE(sDbc, ULN_S_C2);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_HY010)
    {
        /*
         * HY010
         */
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCallbackDisconnectResult(cmiProtocolContext *aProtocolContext,
                                   cmiProtocol        *aProtocol,
                                   void               *aServiceSession,
                                   void               *aUserContext )
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

static ACI_RC ulnDisconnSendDisconnectReq(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    acp_uint16_t        sState = 0;
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);

    sPacket.mOpID = CMP_OP_DB_Disconnect;

    CMI_WRITE_CHECK(sCtx, 2);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_Disconnect);
    CMI_WR1(sCtx, CMP_DB_DISCONN_CLOSE_SESSION);

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

static ACI_RC ulnDisconnReceiveDisconnectRes(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnDbc *sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * 타임아웃 세팅
     */
    if (cmiGetLinkImpl(&(aPtContext->mCmiPtContext)) == CMI_LINK_IMPL_IPCDA)
    {
        ACI_TEST(ulnReadProtocolIPCDA(aFnContext,
                                      aPtContext,
                                      sDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnReadProtocol(aFnContext,
                                 aPtContext,
                                 sDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnDisconnDoLogicalDisconnect(ulnFnContext *aFnContext)
{
    acp_bool_t  sNeedPtFin = ACP_FALSE;
    ulnDbc     *sDbc;

    sDbc = aFnContext->mHandle.mDbc;

    if(ulnDbcIsConnected(sDbc) == ACP_TRUE)
    {
        /*
         * protocol context 를 초기화한다.
         */
        //fix BUG-17722
        ACI_TEST(ulnInitializeProtocolContext(aFnContext,
                                              &(sDbc->mPtContext),
                                              &(sDbc->mSession))
                 != ACI_SUCCESS);
        sNeedPtFin = ACP_TRUE;

        /*
         * disconnect req 전송
         *
         * BUG-16724 Disconnect 시에 서버가 죽어 있어도 성공해야 한다.
         *           --> ACI_TEST() 를 없애서 에러가 나도 계속 진행.
         */
        //fix BUG-17722
        if (ulnDisconnSendDisconnectReq(aFnContext,&(sDbc->mPtContext)) == ACI_SUCCESS)
        {
            /*
             * BUGBUG : ulnDisconnSendDisconnectReq() 함수의 에러가 반드시
             *          communication link failure 라는 법은 없다.
             *          일단, 릴리즈에 바쁘므로 이와같이 처리하고 넘어가자.
             */

            /*
             * disconnect result 수신
             */
            if (ulnDisconnReceiveDisconnectRes(aFnContext,&(sDbc->mPtContext))
                     != ACI_SUCCESS)
            {
                ACI_TEST_RAISE(ulnClearDiagnosticInfoFromObject(aFnContext->mHandle.mObj)
                               != ACI_SUCCESS,
                               LABEL_MEM_MAN_ERR);
                ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS);
            }
        }
        else
        {
            ACI_TEST_RAISE(ulnClearDiagnosticInfoFromObject(aFnContext->mHandle.mObj)
                           != ACI_SUCCESS,
                           LABEL_MEM_MAN_ERR);

            // BUG-24817
            // windows 에서 ulnDisconnSendDisconnectReq 함수가 실패합니다.
            // 보내는것이 실패했기 때문에 받을것도 없다.
            sDbc->mPtContext.mNeedReadProtocol = 0;

            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS);
        }

        /*
         * protocol context 정리
         */
        sNeedPtFin = ACP_FALSE;
        //fix BUG-17722
        ACI_TEST(ulnFinalizeProtocolContext(aFnContext,
                                           &(sDbc->mPtContext)) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnDisconnDoLogicalDisconnect");
    }

    ACI_EXCEPTION_END;

    if(sNeedPtFin == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(aFnContext, &(sDbc->mPtContext));
    }

    return ACI_FAILURE;
}

static ACI_RC ulnDisconnDoPhysicalDisconnect(ulnFnContext *aFnContext)
{
    ulnDbc *sDbc;

    sDbc = aFnContext->mHandle.mDbc;


    if(sDbc->mLink != NULL)
    {
        ACI_TEST_RAISE(ulnDbcFreeLink(sDbc) != ACI_SUCCESS, LABEL_CM_ERR);
    }

    ulnDbcSetIsConnected(sDbc, ACP_FALSE);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CM_ERR)
    {
        ulnErrHandleCmError(aFnContext, NULL);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * BUGBUG : 아래 두 함수가 있어야 할 파일이 어디인지 좀 더 생각해 보자.
 */
static ACI_RC ulnDisconnFreeAllDesc(ulnFnContext *aFnContext)
{
    ulnDbc          *sDbc;
    ulnDesc         *sDesc;
    acp_list_node_t *sIterator1;
    acp_list_node_t *sIterator2;

    sDbc = aFnContext->mHandle.mDbc;

    ACP_LIST_ITERATE_SAFE(&(sDbc->mDescList), sIterator1, sIterator2)
    {
        sDesc = (ulnDesc *)sIterator1;

        ACI_TEST(ulnFreeHandleDescBody(aFnContext, sDbc, sDesc) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnDisconnFreeAllStmt(ulnFnContext *aFnContext)
{
    ulnDbc          *sDbc;
    ulnStmt         *sStmt;
    acp_list_node_t *sIterator1;
    acp_list_node_t *sIterator2;

    sDbc = aFnContext->mHandle.mDbc;

    ACP_LIST_ITERATE_SAFE(&(sDbc->mStmtList), sIterator1, sIterator2)
    {
        sStmt = (ulnStmt *)sIterator1;

        /*
         * Note : disconnect 시에 서버로 stmt free request 을 보내지 않는다.
         *        disconnect 를 하면 서버에서는 자동으로 해당 연결에 속한 stmt 들을
         *        해제하기 때문에 이중 해제가 된다.
         *        큰 문제는 없지만 에러가 보내어져 온다.
         */
        ACI_TEST(ulnFreeHandleStmtBody(aFnContext, sDbc, sStmt, ACP_FALSE) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnDisconnect(ulnDbc *aDbc)
{
    acp_bool_t   sNeedExit = ACP_FALSE;
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_DISCONNECT, aDbc, ULN_OBJ_TYPE_DBC);

    /*
     * 함수 진입
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    ACI_TEST_RAISE(aDbc->mXaEnlist == ACP_TRUE, LABEL_SUCCESS_RETURN);

    /*
     * 인자 체크 : ulnDbc 는 ulnEnter() 함수에서 체크됨. 따로 수행할 필요 없음.
     */

    /*
     * 남아있는 STMT 핸들 해제
     * Note : 진입시 상태머신에서 stmt 상태 체크 다 했으므로 마음놓고 해제해도 된다.
     *        모든 하위 stmt 는 SQLFreeHandle() 이 호출 될 수 있는 상태에 있다.
     */
    ACI_TEST(ulnDisconnFreeAllStmt(&sFnContext) != ACI_SUCCESS);

    /*
     * 남아있는 DESC 해제
     */
    ACI_TEST(ulnDisconnFreeAllDesc(&sFnContext) != ACI_SUCCESS);

    /*
     * Note : disconnect 를 stmt 해제보다 늦게 하는 이유는 stmt 해제하면서 서버로 stmt free 를
     *        날리기 때문이다.
     *
     * 접속 끊기 (DB Session)
     */
    ACI_TEST(ulnDisconnDoLogicalDisconnect(&sFnContext) != ACI_SUCCESS);

    /*
     * 링크 끊기
     */
    ACI_TEST(ulnDisconnDoPhysicalDisconnect(&sFnContext) != ACI_SUCCESS);

    /*
     * 함수 탈출
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s|", "ulnDisconnect");

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_SUCCESS_RETURN);
    {
        ulnExit(&sFnContext);
        return ACI_SUCCESS;
    }

    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail", "ulnDisconnect");

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

}

/* ulnDisconnectLocal
 *
 * 설명:
 *  REQ_SendDisconnect을 전송하고 이것에 대한 리턴 결과는 고려하지 않는다.
 *  샤드에서 코디네이터에 하나의 클라이언트와의 세션에 여러 개의 데이터 노드와의
 *  연결이 존재할 수 있습니다. 이 데이터 노드 연결 중에는 정상적인 것과 비정상적인
 *  것이 존재할 수 있습니다. 이것에 대해서 메타에서는 데이터노드에서의 응답에
 *  관계없이 로컬 연결을 무조건 끊습니다.
 *
 *  Return : SQL_SUCCESS or Error
 */
SQLRETURN ulnDisconnectLocal(ulnDbc *aDbc)
{
    acp_bool_t   sNeedExit = ACP_FALSE;
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_DISCONNECT, aDbc, ULN_OBJ_TYPE_DBC);

    /*
     * 함수 진입
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    ACI_TEST_RAISE(aDbc->mXaEnlist == ACP_TRUE, LABEL_SUCCESS_RETURN);

    /*
     * 인자 체크 : ulnDbc 는 ulnEnter() 함수에서 체크됨. 따로 수행할 필요 없음.
     */

    /*
     * 남아있는 STMT 핸들 해제
     * Note : 진입시 상태머신에서 stmt 상태 체크 다 했으므로 마음놓고 해제해도 된다.
     *        모든 하위 stmt 는 SQLFreeHandle() 이 호출 될 수 있는 상태에 있다.
     */
    ACI_TEST(ulnDisconnFreeAllStmt(&sFnContext) != ACI_SUCCESS);

    /*
     * 남아있는 DESC 해제
     */
    ACI_TEST(ulnDisconnFreeAllDesc(&sFnContext) != ACI_SUCCESS);

    /*
     * Note : disconnect 를 stmt 해제보다 늦게 하는 이유는 stmt 해제하면서 서버로 stmt free 를
     *        날리기 때문이다.
     *
     * 접속 끊기 (DB Session)
     */
    (void)ulnDisconnDoLogicalDisconnect(&sFnContext);

    /*
     * 링크 끊기
     */
    ACI_TEST(ulnDisconnDoPhysicalDisconnect(&sFnContext) != ACI_SUCCESS);

    /*
     * 함수 탈출
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s|", "ulnDisconnect");

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_SUCCESS_RETURN);
    {
        (void)ulnExit(&sFnContext);
        return ACI_SUCCESS;
    }

    ACI_EXCEPTION_END;

    if ( sNeedExit == ACP_TRUE )
    {
        (void)ulnExit(&sFnContext);
    }
    else
    {
        /* Nothing to do */
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail", "ulnDisconnect");

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

}
