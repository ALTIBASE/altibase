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

static ACI_RC ulnNumParamsCheckArgs(ulnFnContext *aContext, acp_sint16_t *aParamCountPtr)
{
    /*
     * HY009 : Invalide Use of Null Pointer
     */
    ACI_TEST_RAISE(aParamCountPtr == NULL, LABEL_INVALID_NULL);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_NULL);
    {
        ulnError(aContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * ulnNumParams.
 *
 * IPD 의 SQL_DESC_COUNT 를 리턴하는 것과
 * IRD 의 것을 리턴하는 것 빼고는 ulnNumResultCols 와 완전히 identical 한 함수이다.
 */
SQLRETURN ulnNumParams(ulnStmt *aStmt, acp_sint16_t *aParamCountPtr)
{
    acp_bool_t    sNeedExit   = ACP_FALSE;
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_NUMPARAMS, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    /* PROJ-1891, BUG-46011 If deferred prepare is exists, process it first */
    if (ulnStmtIsSetDeferredQstr(aStmt) == ACP_TRUE)
    {
        ACI_TEST( ulnPrepareDeferComplete(&sFnContext, ACP_TRUE) );
    }

    ACI_TEST(ulnNumParamsCheckArgs(&sFnContext, aParamCountPtr) != ACI_SUCCESS);

    *aParamCountPtr = (acp_sint16_t)ulnStmtGetParamCount( aStmt );

    sNeedExit = ACP_FALSE;
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_HIGH, NULL, 0,
                  "%-18s| [%2"ACI_INT32_FMT"]",
                  "ulnNumParams", *aParamCountPtr);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail", "ulnNumParams");

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

