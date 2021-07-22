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
#include <ulnFetch.h>

/*
 * ULN_SFID_24
 * SQLFetch(), SQLFetchScroll(), STMT, S5
 *      S6 [s] or [nf]
 *      S11 [x]
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [nf] Not found. The function returned SQL_NO_DATA.
 *           This does not apply when SQLExecDirect, SQLExecute, or SQLParamData returns
 *           SQL_NO_DATA after executing a searched update or delete statement.
 */
ACI_RC ulnSFID_24(ulnFnContext *aFnContext)
{
    if(aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if(ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
        {
            /* [s] or [nf] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S6);
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_25
 * SQLFetch(), SQLFetchScroll(), STMT 상태전이함수 : S6 상태
 *      -- [s] or [nf]
 *      S11 [x]
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [nf] Not found. The function returned SQL_NO_DATA.
 *           This does not apply when SQLExecDirect, SQLExecute, or SQLParamData returns
 *           SQL_NO_DATA after executing a searched update or delete statement.
 */
ACI_RC ulnSFID_25(ulnFnContext *aFnContext)
{
    if(aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if(ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
        {
            /* [s] or [nf] */
        }
    }

    return ACI_SUCCESS;
}

/*
 * ===========================================
 * SQLFetch()
 * ===========================================
 */

SQLRETURN ulnFetch(ulnStmt *aStmt)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;

    acp_uint32_t  sNumberOfRowsFetched;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FETCH, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedExit);

    /*
     * =============================================
     * Function body BEGIN
     * =============================================
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &aStmt->mParentDbc->mSession) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    if (aStmt->mIsSimpleQuerySelectExecuted == ACP_TRUE)
    {
        ACI_TEST(ulnFetchCoreForIPCDASimpleQuery(&sFnContext,
                                                 aStmt,
                                                 &sNumberOfRowsFetched)
                 != ACI_SUCCESS);
        if (sNumberOfRowsFetched == 0)
        {
            /*
             * BUGBUG : ulnFetchFromCache() 함수 주석을 보면 아래와 같이 적혀 있다 :
             *
             * SQL_NO_DATA 를 리턴해 주기 위해서 세팅을 해야 하지만,
             * 이미 ulnFetchUpdateAfterFetch() 함수에서 세팅해버린 경우이다.
             */
            ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_NO_DATA);
        }
    }
    else
    {
        /*
         * --------------------------------
         * Protocol context
         */

        ulnDescSetRowsProcessedPtrValue(aStmt->mAttrIrd, 0);

        //fix BUG-17722
        ACI_TEST(ulnFetchCore(&sFnContext,
                              &(aStmt->mParentDbc->mPtContext),
                              SQL_FETCH_NEXT,
                              ULN_vLEN(0),
                              &sNumberOfRowsFetched) != ACI_SUCCESS);
    }

    /*
     * 사용자에게 페치한 Row 의 갯수 리턴.
     */
    ulnDescSetRowsProcessedPtrValue(aStmt->mAttrIrd, sNumberOfRowsFetched);

    /*
     * 데이터를 fetch 해 주지 않은 row 들의 status array 를 SQL_ROW_NOROW 로 만들어 준다.
     *
     * block cursor 를 사용할 때,
     * 가령 10 줄을 사용자가 페치했는데, 데이터는 5줄만 들어갔을 경우,
     * 나머지 5 줄의 row status ptr 에는 SQL_ROW_NOROW 를 넣어 줘야 한다.
     */
    ulnDescInitStatusArrayValues(aStmt->mAttrIrd,
                                 sNumberOfRowsFetched,
                                 ulnStmtGetAttrRowArraySize(aStmt),
                                 SQL_ROW_NOROW);

    /*
     * Protocol context
     * --------------------------------
     */

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                                        &(aStmt->mParentDbc->mPtContext) )
               != ACI_SUCCESS);

    /*
     * =============================================
     * Function body END
     * =============================================
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_HIGH, NULL, 0,
            "%-18s| %"ACI_INT32_FMT, "ulnFetch", sFnContext.mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail", "ulnFetch");

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

