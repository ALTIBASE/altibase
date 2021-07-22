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
#include <ulnExtendedFetch.h>

/*
 * ULN_SFID_52
 * SQLExtendedFetch, STMT, S5
 *      S7 [s] or [nf]
 *      S11 [x]
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [nf] Not found. The function returned SQL_NO_DATA.
 *           This does not apply when SQLExecDirect, SQLExecute, or SQLParamData returns
 *           SQL_NO_DATA after executing a searched update or delete statement.
 */
ACI_RC ulnSFID_52(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
        {
            /* [s] or [nf] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S7);
        }
    }

    return ACI_SUCCESS;
}

/*
 * ===========================================
 * SQLExtendedFetch()
 * ===========================================
 */

/*
 * Note : 64bit odbc 에서 SQLROWSETSIZE 는 SQLUINTEGER 이다. 즉 32비트 정수이다.
 *        ExtendedFetch 의 4번째 parameter 는 64비트가 아니라 32비트이다.
 */

SQLRETURN ulnExtendedFetch(ulnStmt      *aStmt,
                           acp_uint16_t  aOrientation,
                           ulvSLen       aOffset,
                           acp_uint32_t *aRowCountPtr,
                           acp_uint16_t *aRowStatusArray)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);
    ULN_FLAG(sNeedRecoverRowStatusPtr);

    ulnFnContext  sFnContext;
    acp_uint32_t  sNumberOfRowsFetched = 0;
    acp_uint16_t *sOriginalRowStatusArray;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_EXTENDEDFETCH, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * ===========================================
     * INITIALIZE
     * ===========================================
     */

    /*
     * Note : 아래와 같은 바꿔치기가 가능한 이유는 stmt 를 공유하지 않기 때문이다.
     *        dbc 레벨에서 exclusive 한 lock 을 잡기 때문에 절대로 한 stmt 에 두개의 함수가
     *        실행되지 않는다.
     *        따라서 아래처럼 바꿔치기 해서 안전하게 사용할 수 있다.
     */

    /*
     * 바꿔치기 1 : SQL_ATTR_ROW_STATUS_PTR
     * ------------------------------------
     * Row status array 4 extended fetch 를 세팅한다.
     * stmt 의 속성으로써, SQLFetchScroll() 함수가 사용하는 SQL_ATTR_ROW_STATUS_PTR 과는
     * 다른 버퍼를 써야 한다고 ODBC 에서 이야기하고 있다.
     *
     * 일단 바꿔치기를 한 후, 나중에 복구시켜 두면 된다.
     */

    sOriginalRowStatusArray = ulnStmtGetAttrRowStatusPtr(aStmt);
    ulnStmtSetAttrRowStatusPtr(aStmt, aRowStatusArray);

    ULN_FLAG_UP(sNeedRecoverRowStatusPtr);

    // fix BUG-20745
    // http://msdn.microsoft.com/en-us/library/ms713591.aspx
    // SQLExtendedFetch에서는 SQL_ATTR_ROW_BIND_OFFSET_PTR를 사용할 수 없지만
    // MSDASQL에서 이렇게 호출하고 있음.
    // 따라서 SQLExtendedFetch에서도 SQL_ATTR_ROW_BIND_OFFSET_PTR를 사용할 수 있게 변경함

    /*
     * ===========================================
     * MAIN
     * ===========================================
     */

    ACI_TEST(ulnCheckFetchOrientation(&sFnContext, aOrientation) != ACI_SUCCESS);

    /*
     * Protocol Context 초기화
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    if (aRowCountPtr != NULL)
    {
        *aRowCountPtr = sNumberOfRowsFetched;
    }
    //fix BUG-17722
    ACI_TEST(ulnFetchCore(&sFnContext,
                          &(aStmt->mParentDbc->mPtContext),
                          aOrientation,
                          aOffset,
                          &sNumberOfRowsFetched) != ACI_SUCCESS);

    /*
     * 사용자에게 몇개의 row 를 페치해 왔는지 리턴해 준다.
     */
    if (aRowCountPtr != NULL)
    {
        *aRowCountPtr = sNumberOfRowsFetched;
    }

    /*
     * 데이터를 fetch 해 주지 않은 row 들의 status array 를 SQL_ROW_NOROW 로 만들어 준다.
     *
     * block cursor 를 사용할 때,
     * 가령 10 줄을 사용자가 페치했는데, 데이터는 5줄만 들어갔을 경우,
     * 나머지 5 줄의 row status ptr 에는 SQL_ROW_NOROW 를 넣어 줘야 한다.
     */
    ulnStmtInitRowStatusArrayValue(aStmt,
                                   sNumberOfRowsFetched,
                                   ulnStmtGetAttrRowArraySize(aStmt),
                                   SQL_ROW_NOROW);

    /*
     * Note : 에러 리턴에 있어서 모든 row 가 error 이면 extended fetch 는 SQL_SUCCESS_WITH_INFO 를
     *        리턴하고, SQL_ERROR 를 리턴하지 않는다.
     *
     *        그런데, 모든 row 가 error 인 상황은 row 하나가 fetch 되었는데, 그 row 가 error 인
     *        상황밖에 없다.
     *        이 경우, SQLFetchScroll() 은 SQL_ERROR 를 리턴하게 되어 있긴 한데,...
     *
     * BUGBUG : 이러한 경우를 처리하도록 해야 한다.
     */

    /*
     * Protocol Context 정리
     */
    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * ===========================================
     * FINALIZE
     * ===========================================
     */

    /*
     * 바꿔치기 해 두었던 SQL_ATTR_ROW_STATUS_PTR 을 원상복구한다.
     */
    ulnStmtSetAttrRowStatusPtr(aStmt, sOriginalRowStatusArray);

    /*
     * Exit
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_HIGH, NULL, 0,
            "%-18s| [orient: %"ACI_UINT32_FMT"] %"ACI_INT32_FMT,
            "ulnExtendedFetch", aOrientation, sFnContext.mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedRecoverRowStatusPtr)
    {
        /*
         * 바꿔치기 해 두었던 SQL_ATTR_ROW_STATUS_PTR 을 원상복구한다.
         */
        ulnStmtSetAttrRowStatusPtr(aStmt, sOriginalRowStatusArray);
    }

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
            "%-18s| [orient: %"ACI_UINT32_FMT"] fail",
            "ulnExtendedFetch", aOrientation);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

