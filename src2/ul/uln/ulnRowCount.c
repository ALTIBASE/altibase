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

/*
 * SQLRowCount returns the number of rows affected by an UPDATE, INSERT, or DELETE statement;
 * an SQL_ADD, SQL_UPDATE_BY_BOOKMARK, or SQL_DELETE_BY_BOOKMARK operation
 * in SQLBulkOperations; or an SQL_UPDATE or SQL_DELETE operation in SQLSetPos.
 *
 * Note : For other statements and functions, the driver may define the value
 *        returned in *RowCountPtr.
 *        For example, some data sources may be able to return the number of rows
 *        returned by a SELECT statement or a catalog function before fetching the rows.
 *
 *        Note Many data sources cannot return the number of rows in a result set
 *             before fetching them; for maximum interoperability,
 *             applications should not rely on this behavior.
 */

SQLRETURN ulnRowCount(ulnStmt *aStmt, ulvSLen *aRowCountPtr)
{
    ULN_FLAG(sNeedExit);

    acp_sint64_t  sRowCount = 0;
    ulnResult    *sResult;
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_ROWCOUNT, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * ===========================================
     * Function BEGIN
     * ===========================================
     */

    ACI_TEST_RAISE(aRowCountPtr == NULL, LABEL_INVALID_NULL);

    sResult   = ulnStmtGetCurrentResult(aStmt);
    sRowCount = ulnResultGetAffectedRowCount(sResult);

    if (sRowCount > ACP_SLONG_MAX)
    {
        ulnError(&sFnContext, ulERR_IGNORE_VALUE_TOO_BIG);
    }
    else
    {
    	/* Do nothing */
    }

    *aRowCountPtr = (ulvSLen)sRowCount;

    /*
     * ===========================================
     * Function END
     * ===========================================
     */

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| [cnt: %llu]", "ulnRowCount", sRowCount);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_NULL)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [cnt: %llu] fail", "ulnRowCount", sRowCount);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

/* BUG-44572
 * BUGBUG (2017-02-28) 호출 시점에 알 수 있는 값이 아니라 execute 결과로 받은 값을 준다.
 * TODO (2017-02-28) 추후 cached rows까지 포함해 호출 시점에 알 수 있는 값을 주도록 바꿔야 한다. */
/**
 * Fetched Row Count를 얻는다.
 *
 * @param[in]  aStmt       Statement 핸들
 * @param[out] aNumRowsPtr Fetched Row Count를 담을 포인터
 *
 * @return 함수 수행에 성공했다면 SQL_SUCCESS, 실패하면 SQL_FAILURE, 핸들이 올바르지 않으면 SQL_INVALID_HANDLE
 */
SQLRETURN ulnNumRows(ulnStmt *aStmt, ulvSLen *aNumRowsPtr)
{
    ULN_FLAG(sNeedExit);

    acp_sint64_t  sNumRows = 0;
    ulnResult    *sResult  = NULL;
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_ROWCOUNT, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST_RAISE(aNumRowsPtr == NULL, LABEL_INVALID_NULL);

    sResult  = ulnStmtGetCurrentResult(aStmt);
    sNumRows = ulnResultGetFetchedRowCount(sResult);

    if (sNumRows > ACP_SLONG_MAX)
    {
        ulnError(&sFnContext, ulERR_IGNORE_VALUE_TOO_BIG);
    }
    else
    {
    	/* do nothing */
    }

    *aNumRowsPtr = (ulvSLen)sNumRows;

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_MID, NULL, 0,
                  "%-18s| [cnt: %llu]", "ulnNumRows", sNumRows);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_NULL)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| [cnt: %llu]", "ulnNumRows", sNumRows);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

