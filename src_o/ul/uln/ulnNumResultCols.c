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
 * SQLNumResultCols 가 발생시키는 에러들의 정보
 *
 * SQLSTATE  Error
 *           Description
 *
 * O : Implementation Complete
 * X : Do not need to implement
 *   : Need to do
 *
 * [ ] 01000 General warning
 *           Driver-specific informational message. (Function returns SQL_SUCCESS_WITH_INFO.)
 * [ ] 08S01 Communication link failure
 *           The communication link between the driver and the data source to which the driver
 *           was connected failed before the function completed processing.
 * [ ] HY000 General error
 *           An error occurred for which there was no specific SQLSTATE and for which
 *           no implementation-specific SQLSTATE was defined.
 *           The error message returned by SQLGetDiagRec in the *MessageText buffer describes
 *           the error and its cause.
 * [ ] HY001 Memory allocation error
 *           The driver was unable to allocate memory required to support execution or
 *           completion of the function.
 * [ ] HY008 Operation canceled
 *           Asynchronous processing was enabled for the StatementHandle.
 *           The function was called, and before it completed execution,
 *           SQLCancel was called on the StatementHandle; the function was then
 *           called again on the StatementHandle.
 *           The function was called, and before it completed execution,
 *           SQLCancel was called on the StatementHandle from a different thread in
 *           a multithread application.
 * [ ] HY010 Function sequence error
 *           (DM) The function was called prior to calling SQLPrepare or SQLExecDirect
 *           for the StatementHandle.
 *           (DM) An asynchronously executing function (not this one) was
 *           called for the StatementHandle and was still executing
 *           when this function was called.
 *           (DM) SQLExecute, SQLExecDirect, SQLBulkOperations, or SQLSetPos was called for
 *           the StatementHandle and returned SQL_NEED_DATA.
 *           This function was called before data was sent for all data-at-execution parameters
 *           or columns.
 * [ ] HY013 Memory management error
 *           The function call could not be processed because the underlying memory objects
 *           could not be accessed, possibly because of low memory conditions.
 * [ ] HYT01 Connection timeout expired
 *           The connection timeout period expired before the data source responded to
 *           the request. The connection timeout period is set through SQLSetConnectAttr,
 *           SQL_ATTR_CONNECTION_TIMEOUT.
 * [ ] IM001 Driver does not support this function
 *           (DM) The driver associated with the StatementHandle does not support the function.
 *
 * SQLNumResultCols can return any SQLSTATE that can be returned by SQLPrepare or SQLExecute
 * when called after SQLPrepare and before SQLExecute, depending on when the data source
 * evaluates the SQL statement associated with the statement.
 */
static ACI_RC ulnNumResultColsEnter(ulnFnContext *aContext, acp_sint16_t *aColumnCountPtr)
{
    /*
     * HY009 : Invalide Use of Null Pointer
     */
    ACI_TEST_RAISE(aColumnCountPtr == NULL, LABEL_INVALID_NULL);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_NULL);
    {
        ulnError(aContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * ulnNumResultCols.
 *
 * @param [in] aObject
 *  Statement handle.
 * @param [out] aColumnCountPtr
 *  Pointer to a buffer in which to return the number of columns in the result set.
 *  This count does not include a bound bookmark column.
 *
 * SQLNumResultCols can be called successfully only when the statement is in the prepared,
 * executed, or positioned state. If the statement associated with StatementHandle does not
 * return columns, SQLNumResultCols sets *ColumnCountPtr to 0.
 *
 * The number of columns returned by SQLNumResultCols is the same value as
 * the SQL_DESC_COUNT field of the IRD.
 *
 * IPD 의 SQL_DESC_COUNT 를 리턴하는 것과
 * IRD 의 것을 리턴하는 것 빼고는 ulnNumResultCols 와 완전히 identical 한 함수이다.
 */
SQLRETURN ulnNumResultCols(ulnStmt *aStmt, acp_sint16_t *aColumnCountPtr)
{
    acp_bool_t    sNeedExit = ACP_FALSE;
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_NUMRESULTCOLS, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;

    /* PROJ-1891, BUG-46011 If deferred prepare is exists, process it first */
    if (ulnStmtIsSetDeferredQstr(aStmt) == ACP_TRUE)
    {
        ACI_TEST( ulnPrepareDeferComplete(&sFnContext, ACP_TRUE) );
    }

    /*
     * 넘겨진 인자들의 유효성 체크
     */
    ACI_TEST(ulnNumResultColsEnter(&sFnContext, aColumnCountPtr) != ACI_SUCCESS);

    *(acp_uint16_t *)aColumnCountPtr = ulnStmtGetColumnCount(sFnContext.mHandle.mStmt);

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_HIGH, NULL, 0,
            "%-18s| [%2"ACI_INT32_FMT"]",
            "ulnNumResultCols", *(acp_sint16_t*)aColumnCountPtr);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail", "ulnNumResultCols");

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
