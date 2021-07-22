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
#include <ulnCancel.h>

/**
 * ULN_SFID_03
 *
 * SQLCancel(), STMT, S8-S10
 *
 * http://nok.altibase.com/x/jqCM
 *
 * related state :
 *      S1 [1]
 *      S2 [nr] and [2]
 *      S3 [r] and [2]
 *      S5 [3] and [5]
 *      S6 ([3] or [4]) and [6]
 *      S7 [4] and [7]
 *
 * where
 *      [1] SQLExecDirect returned SQL_NEED_DATA.
 *      [2] SQLExecute returned SQL_NEED_DATA.
 *      [3] SQLBulkOperations returned SQL_NEED_DATA.
 *      [4] SQLSetPos returned SQL_NEED_DATA.
 *      [5] SQLFetch, SQLFetchScroll, or SQLExtendedFetch had not been called.
 *      [6] SQLFetch or SQLFetchScroll had been called.
 *      [7] SQLExtendedFetch had been called.
 *
 * @param[in] aFnContext funnction context
 *
 * @return always ACI_SUCCESS
 */
ACI_RC ulnSFID_03(ulnFnContext *aFnContext)
{
    ulnStmt         *sStmt = aFnContext->mHandle.mStmt;
    ulnFunctionId   sNDFuncID;

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sNDFuncID = ulnStmtGetNeedDataFuncID(sStmt);
        ACP_TEST_RAISE(sNDFuncID == ULN_FID_NONE, NO_EFFECT);

        switch (sNDFuncID)
        {
            case ULN_FID_EXECDIRECT:
                ULN_OBJ_SET_STATE(sStmt, ULN_S_S1);
                break;

            case ULN_FID_EXECUTE:
                if (ulnStateCheckR(aFnContext) == ACP_TRUE)
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(sStmt, ULN_S_S3);
                }
                else
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(sStmt, ULN_S_S2);
                }
                break;

            case ULN_FID_BULKOPERATIONS:
                switch (ulnStmtGetLastFetchFuncID(sStmt))
                {
                    case ULN_FID_NONE:
                        ULN_OBJ_SET_STATE(sStmt, ULN_S_S5);
                        break;
                    case ULN_FID_FETCH:
                    case ULN_FID_FETCHSCROLL:
                        ULN_OBJ_SET_STATE(sStmt, ULN_S_S6);
                        break;
                    case ULN_FID_EXTENDEDFETCH:
                        ULN_OBJ_SET_STATE(sStmt, ULN_S_S7);
                        break;
                    default:
                        /* WTH! */
                        ACE_ASSERT(ACP_FALSE);
                        break;
                }
                break;

            case ULN_FID_SETPOS:
                switch (ulnStmtGetLastFetchFuncID(sStmt))
                {
                    case ULN_FID_FETCH:
                    case ULN_FID_FETCHSCROLL:
                        ULN_OBJ_SET_STATE(sStmt, ULN_S_S6);
                        break;
                    case ULN_FID_EXTENDEDFETCH:
                        ULN_OBJ_SET_STATE(sStmt, ULN_S_S7);
                        break;
                    default:
                        /* crazy! */
                        ACE_ASSERT(ACP_FALSE);
                        break;
                }
                break;

            default:
                /* crazy! */
                ACE_ASSERT(ACP_FALSE);
                break;
        }

        ulnStmtResetNeedDataFuncID(sStmt);
    }

    ACE_EXCEPTION_CONT(NO_EFFECT);

    return ACI_SUCCESS;
}

/**
 * CancelResult 프로토콜 처리를 위한 콜백 함수
 *
 * @param[in] aProtocolContext unsued
 * @param[in] aProtocol        unsued
 * @param[in] aServiceSession  unsued
 * @param[in] aUserContext     unsued
 *
 * @return 항상 ACI_SUCCESS
 */
ACI_RC ulnCallbackCancelResult(cmiProtocolContext *aProtocolContext,
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

/**
 * Cancel 프로토콜을 전송할 CM 초기화
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aCmiSession cmiSession struct
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnInitializeCancelContext(ulnFnContext *aFnContext,
                                  ulnPtContext *aPtContext,
                                  cmiSession   *aCmiSession)
{
    cmiLink        *sCmiLink = NULL;
    ulnDbc         *sDbc = NULL;
    acp_time_t      sTimeout;
    acp_uint32_t    sStep = 0;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    // proj_2160 cm_type removal: set cmbBlock-ptr NULL
    ACI_TEST(cmiMakeCmBlockNull( &(aPtContext->mCmiPtContext) ) != ACI_SUCCESS);

    ACI_TEST_RAISE(cmiAddSession(aCmiSession, sDbc, CMI_PROTOCOL_MODULE(DB), NULL)
                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);
    sStep = 1;
    
    /* PROJ-2474 SSL/TLS */
    ACI_TEST_RAISE(cmiAllocLink(&sCmiLink, CMI_LINK_TYPE_PEER_CLIENT, sDbc->mCmiLinkImpl)
                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    sStep = 2;
    // proj_2160 cm_type removal: allocate cmbBlock
    ACI_TEST_RAISE(cmiAllocCmBlock( &(aPtContext->mCmiPtContext),
                                    CMP_MODULE_DB,
                                    sCmiLink,
                                    sDbc )
                   != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    sStep = 3;

    ACI_TEST_RAISE(cmiSetLinkForSession(aCmiSession, sCmiLink)
                   != ACI_SUCCESS, LABEL_CM_ERR);

    sTimeout = acpTimeFrom(ulnDbcGetLoginTimeout(sDbc), 0);
    ACI_TEST_RAISE(cmiConnect(&(aPtContext->mCmiPtContext), &(sDbc->mConnectArg), sTimeout, SO_LINGER)
                   != ACI_SUCCESS, LABEL_CM_ERR);

    ACI_TEST(ulnInitializeProtocolContext(aFnContext, aPtContext, aCmiSession)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnInitializeCancelContext");
    }
    ACI_EXCEPTION(LABEL_CM_ERR)
    {
        ulnErrHandleCmError(aFnContext, NULL);
    }
    ACI_EXCEPTION_END;

    switch (sStep)
    {
        case 3:
            cmiFreeCmBlock(&(aPtContext->mCmiPtContext));
        case 2:
            ACE_ASSERT(cmiFreeLink(sCmiLink) == ACI_SUCCESS);
            aCmiSession->mLink = NULL;
        case 1:
            ACE_ASSERT(cmiRemoveSession(aCmiSession) == ACI_SUCCESS);
            break;
    }

    return ACI_FAILURE;
}

/**
 * Cancel 프로토콜을 위한 CM 정리
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aCmiSession cmiSession struct
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnFinalizeCancelContext(ulnFnContext *aFnContext,
                                ulnPtContext *aPtContext,
                                cmiSession   *aCmiSession)
{
    acp_uint32_t sStep = 3;

    ACI_TEST(ulnFinalizeProtocolContext(aFnContext, aPtContext)
             != ACI_SUCCESS);
    sStep = 2;

    ACI_TEST(cmiFreeCmBlock(&(aPtContext->mCmiPtContext)) != ACI_SUCCESS);
    sStep = 1;

    ACI_TEST(cmiFreeLink((cmiLink *)aCmiSession->mLink) != ACI_SUCCESS);
    aCmiSession->mLink = NULL;
    sStep = 0;

    ACI_TEST(cmiRemoveSession(aCmiSession) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    switch (sStep)
    {
        case 3:
            (void)cmiFreeCmBlock(&(aPtContext->mCmiPtContext));
        case 2:
            (void) cmiFreeLink((cmiLink *)aCmiSession->mLink);
            aCmiSession->mLink = NULL;
        case 1:
            (void) cmiRemoveSession(aCmiSession);
            break;
    }

    return ACI_FAILURE;
}

/**
 * Execute를 취소하기 위한 Cancel 명령을 서버로 날린다.
 *
 * @param[in] aFnContext   function context
 * @param[in] aPtContext   protocol context
 * @param[in] aStmt        statement handle
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnCancelExecute(ulnFnContext *aFnContext,
                        ulnPtContext *aPtContext,
                        ulnStmt      *aStmt)
{
    ulnDbc          *sDbc;
    acp_uint32_t    sStmtID   = ulnStmtGetStatementID(aStmt);
    acp_uint32_t    sStmtCID;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /* StatementID가 없으면 CID로 시도한다. */
    if (sStmtID == ULN_STMT_ID_NONE)
    {
        sStmtCID = ulnStmtGetCID(aStmt);
        ACE_ASSERT(sStmtCID != ULN_STMT_CID_NONE);

        ACI_TEST(ulnWriteCancelByCIDREQ(aFnContext, aPtContext, sStmtCID)
                 != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnWriteCancelREQ(aFnContext, aPtContext, sStmtID)
                 != ACI_SUCCESS);
    }

    ACI_TEST(ulnFlushAndReadProtocol(aFnContext, aPtContext, sDbc->mConnTimeoutValue)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * NeedData 상태에서 처리된 Param 정보를 처리한다.
 *
 * @param[in] aStmt   statement handle
 */
void ulnCancelNeedDataParam(ulnStmt *aStmt)
{
    ulnDescRec   *sDescRecApd;
    ulnPDContext *sPDContext;
    int           i;

    for (i = 1; i <= aStmt->mProcessingParamNumber; i++)
    {
        sDescRecApd = ulnStmtGetApdRec(aStmt, i);
        if (sDescRecApd == NULL)
        {
            continue;
        }

        sPDContext = &(sDescRecApd->mPDContext);
        if (sPDContext->mOp != NULL)
        {
            sPDContext->mOp->mFinalize(sPDContext);
        }
        else
        {
            /* BUG-32616: 없을거 같지만, PDContext->mOp가 NULL일 때도 있다.
             * 이 때도 PDContext의 상태는 바꿔줘야한다. */
            ulnPDContextSetState(sPDContext, ULN_PD_ST_CREATED);
        }
        ulnDescRemovePDContext(sDescRecApd->mParentDesc, sPDContext);
    }

    /* Execute와 관련된 변수 초기화 */
    ulnStmtResetPD(aStmt);
    ulnDescSetRowsProcessedPtrValue(aStmt->mAttrIpd, 0);
}

/**
 * Cancel 수행
 *
 * @param[in] aFnContext   function context
 * @param[in] aStmt        statement handle
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnCancelCore(ulnFnContext *aFnContext,
                     ulnStmt      *aStmt)
{
    ULN_FLAG(sNeedFinalize);

    ulnPtContext  sPtContext;
    cmiSession    sCmiSession;

    /* Cancel NEED DATA */
    if (ulnStmtGetNeedDataFuncID(aStmt) != ULN_FID_NONE)
    {
        ulnCancelNeedDataParam(aStmt);
    }
    /* Cancel EXECUTE */
    else
    {
        ACI_TEST(ulnInitializeCancelContext(aFnContext, &sPtContext, &sCmiSession) != ACI_SUCCESS);
        ULN_FLAG_UP(sNeedFinalize);

        ACI_TEST(ulnCancelExecute(aFnContext, &sPtContext, aStmt) != ACI_SUCCESS);

        ULN_FLAG_DOWN(sNeedFinalize);
        ACE_ASSERT(ulnFinalizeCancelContext(aFnContext, &sPtContext, &sCmiSession) == ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinalize)
    {
        ACE_ASSERT(ulnFinalizeCancelContext(aFnContext, &sPtContext, &sCmiSession) == ACI_SUCCESS);
    }

    return ACI_FAILURE;
}

/**
 * EXECUTE 명령을 취소한다.
 *
 * EXECUTE 수행중이 아닐때는 조용히 넘어간다.
 *
 * @param[in] aStmt statement handle
 *
 * @return Cancel잘 수행했으면 SQL_SUCCESS, 아니면 에러 코드
 */
SQLRETURN ulnCancel(ulnStmt *aStmt)
{
    ulnFnContext  sFnContext;
    ulnFnContext  sTmpFnContext;
    ulnStmt      *sRowsetStmt;

    ULN_FLAG(sNeedExit);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_CANCEL, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    sRowsetStmt = aStmt->mRowsetStmt;  /* BUG-46885 */

    if (sRowsetStmt != SQL_NULL_HSTMT)
    {
        ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_CANCEL, sRowsetStmt, ULN_OBJ_TYPE_STMT);

        ACI_TEST_RAISE(ulnCancelCore(&sTmpFnContext, sRowsetStmt)
                       != ACI_SUCCESS, FailToCanelRowsetStmtException);
    }
    else
    {
        ACI_TEST(ulnCancelCore(&sFnContext, aStmt) != ACI_SUCCESS);
    }

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(FailToCanelRowsetStmtException)
    {
        if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
        {
            ulnDiagRecMoveAll( &(aStmt->mObj), &(sRowsetStmt->mObj) );
            ULN_FNCONTEXT_SET_RC(&sFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
        }
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
