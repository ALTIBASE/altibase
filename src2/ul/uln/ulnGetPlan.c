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
#include <ulnGetPlan.h>

ACI_RC ulnCallbackPlanGetResult(cmiProtocolContext *aPtContext,
                                cmiProtocol        *aProtocol,
                                void               *aServiceSession,
                                void               *aUserContext)
{
    ulnFnContext            *sFnContext = (ulnFnContext *)aUserContext;
    ulnDbc                  *sDbc;
    acp_uint8_t             *sPlan;
    acp_uint8_t             *sRow = NULL;
    acp_uint32_t             sRowSize;

    acp_uint32_t             sStatementID;
    acp_uint32_t             sLen;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    CMI_RD4(aPtContext, &sStatementID);
    CMI_RD4(aPtContext, &sLen);

    sRowSize = sLen;

    ACI_TEST_RAISE(sFnContext->mHandle.mStmt->mStatementID != sStatementID,
                   LABEL_STMT_ID_MISMATCH);

    if(sFnContext->mHandle.mStmt->mPlanTree != NULL)
    {
        ulnStmtFreePlanTree(sFnContext->mHandle.mStmt);
    }

    /*
     * Null Termination character 까지 포함
     */
    ACI_TEST_RAISE(ulnStmtAllocPlanTree(sFnContext->mHandle.mStmt, sLen + 1)
                   != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    sPlan = (acp_uint8_t*)sFnContext->mHandle.mStmt->mPlanTree;

    
    if ( cmiGetLinkImpl(aPtContext) == CMI_LINK_IMPL_IPCDA )
    {
        /* PROJ-2616 메모리에 바로 접근하여 데이터를 읽도록 한다. */
        ACI_TEST_RAISE( cmiSplitReadIPCDA(aPtContext,
                                          sLen,
                                          &sRow,
                                          NULL) != ACI_SUCCESS,
                        cm_error );
        acpMemCpy(sPlan, sRow, sLen);
    }
    else
    {
        ACI_TEST_RAISE( cmiSplitRead( aPtContext,
                                      sRowSize,
                                      sPlan,
                                      sDbc->mConnTimeoutValue )
                        != ACI_SUCCESS, cm_error );
    }

    sRowSize = 0;

    /*
     * Null termination
     */
    *(sPlan + sLen) = 0;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "CallbackPlanGetResult");
    }
    ACI_EXCEPTION(LABEL_STMT_ID_MISMATCH)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "Statement IDs are not match in CallbackPlanGetResult.");
    }
    ACI_EXCEPTION(cm_error)
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    if( sRowSize != 0 )
    {
        ACI_TEST_RAISE( cmiSplitSkipRead( aPtContext,
                                          sRowSize,
                                          sDbc->mConnTimeoutValue )
                        != ACI_SUCCESS, cm_error );
    }

    return ACI_SUCCESS;
}

SQLRETURN ulnGetPlan(ulnStmt *aStmt, acp_char_t **aPlan)
{
    acp_bool_t    sNeedExit = ACP_FALSE;
    acp_bool_t    sNeedFinPtContext = ACP_FALSE;
    ulnFnContext  sFnContext;
    ulnDbc       *sDbc;

    /*PROJ-2616*/
    cmiProtocolContext *sCtx = NULL;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETPLAN, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /* PROJ-1381, BUG-32902 Explain Plan OFF 일 때는 에러를 내는게 낫다.
     * Prepare가 되지 않았을 때도 굳이 서버에 갔다 올 필요 없다. */
    ACI_TEST_RAISE(sDbc->mAttrExplainPlan == SQL_FALSE, FuncSeqError);
    ACI_TEST_RAISE(ulnStmtIsPrepared(aStmt) != ACP_TRUE, FuncSeqError);

    /*
     * Protocol Context 초기화
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);

    sNeedFinPtContext = ACP_TRUE;
    //fix BUG-17722
    ACI_TEST(ulnWritePlanGetREQ(&sFnContext,
                                &(aStmt->mParentDbc->mPtContext),
                                aStmt->mStatementID) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(&sFnContext,
                             &(aStmt->mParentDbc->mPtContext))
                != ACI_SUCCESS);

    sCtx = &aStmt->mParentDbc->mPtContext.mCmiPtContext;
    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        /* PROJ-2616 */
        ACI_TEST(ulnReadProtocolIPCDA(&sFnContext,
                                      &(aStmt->mParentDbc->mPtContext),
                                      sDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnReadProtocol(&sFnContext,
                                 &(aStmt->mParentDbc->mPtContext),
                                 sDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }


    *aPlan = sFnContext.mHandle.mStmt->mPlanTree;

    /*
     * Protocol Context 정리
     */
    sNeedFinPtContext = ACP_FALSE;
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                             &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(FuncSeqError);
    {
        ulnError(&sFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION_END;

    if(sNeedFinPtContext == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
