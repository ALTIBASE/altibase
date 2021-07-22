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
 * ===========================================
 * SQLFetchScroll()
 * ===========================================
 */

SQLRETURN ulnFetchScroll(ulnStmt *aStmt, acp_sint16_t aFetchOrientation, ulvSLen aFetchOffset)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;

    acp_uint32_t  sNumberOfRowsFetched;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FETCHSCROLL, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedExit);

    /*
     * =============================================
     * Function Body BEGIN
     * =============================================
     */

    ACI_TEST(ulnCheckFetchOrientation(&sFnContext, aFetchOrientation) != ACI_SUCCESS);

    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * ---------------------------------------
     * Protocol context
     */

    // ulnDescSetRowsProcessedPtrValue(aStmt->mAttrIrd, 0);

    //fix BUG-17722
    ACI_TEST(ulnFetchCore(&sFnContext,
                          &(aStmt->mParentDbc->mPtContext),
                          aFetchOrientation,
                          aFetchOffset,
                          &sNumberOfRowsFetched) != ACI_SUCCESS);

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
    ulnStmtInitRowStatusArrayValue(aStmt,
                                   sNumberOfRowsFetched,
                                   ulnStmtGetAttrRowArraySize(aStmt),
                                   SQL_ROW_NOROW);

    /*
     * Protocol context
     * ---------------------------------------
     */

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                      &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * =============================================
     * Function Body END
     * =============================================
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_HIGH, NULL, 0,
            "%-18s| [orient: %"ACI_INT32_FMT"] %"ACI_INT32_FMT,
            "ulnFetchScroll", aFetchOrientation, sFnContext.mSqlReturn);

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
            "%-18s| [orient: %"ACI_INT32_FMT"] fail",
            "ulnFetchScroll", aFetchOrientation);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
