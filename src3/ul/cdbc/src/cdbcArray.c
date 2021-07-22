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

#include <cdbc.h>



/**
 * Array Binding을 설정한다.
 *
 * @param[in] aABRes 결과 핸들
 * @param[in] aArraySize Array size
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC altibase_result_set_array_bind (cdbcABRes *aABRes, acp_sint32_t aArraySize)
{
    #define CDBC_FUNC_NAME "altibase_result_set_array_bind"

    acp_uint16_t *sArrayStatus = NULL;
    acp_bool_t    sArrayStatusAlloced = ACP_FALSE;
    acp_ulong_t   sArraySize4CLI = aArraySize;
    ALTIBASE_RC   sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HRES_NOT_VALID(aABRes), InvalidHandle);
    CDBC_TEST_RAISE(aArraySize < 1, InvalidParamRange);

    CDBCLOG_CALL("SQLSetStmtAttr");
    sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_PARAMSET_SIZE,
                         (SQLPOINTER) sArraySize4CLI, 0);
    CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    if (aArraySize > 1)
    {
        if (aABRes->mArrayBindSize != aArraySize)
        {
            CDBCLOG_CALL("acpMemCalloc");
            sRC = acpMemCalloc((void **)&sArrayStatus, aArraySize,
                               ACI_SIZEOF(acp_uint16_t));
            CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
            CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);
            sArrayStatusAlloced = ACP_TRUE;
        }
        else
        {
            sArrayStatus = aABRes->mArrayStatusParam;
        }

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_PARAM_BIND_TYPE,
                             SQL_PARAM_BIND_BY_COLUMN, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_PARAM_STATUS_PTR,
                             (SQLPOINTER) sArrayStatus, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_PARAMS_PROCESSED_PTR,
                             (SQLPOINTER) &(aABRes->mArrayProcessed), 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        if (sArrayStatusAlloced == ACP_TRUE)
        {
            SAFE_FREE_AND_CLEAN(aABRes->mArrayStatusParam);
            aABRes->mArrayStatusParam = sArrayStatus;
        }
    }
    else /* if (aArraySize == 1) */
    {
        SAFE_FREE_AND_CLEAN(aABRes->mArrayStatusParam);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_PARAM_STATUS_PTR,
                             (SQLPOINTER) NULL, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_PARAMS_PROCESSED_PTR,
                             (SQLPOINTER) NULL, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
    }

    aABRes->mArrayBindSize = aArraySize;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(InvalidParamRange);
    {
        altibase_set_errinfo(aABRes->mDiagRec,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(aABRes->mDiagRec,
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(aABRes, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    if (sArrayStatusAlloced == ACP_TRUE)
    {
        SAFE_FREE(sArrayStatus);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * Array Binding을 설정한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @param[in] aArraySize Array size
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_set_array_bind (ALTIBASE_STMT aABStmt, acp_sint32_t aArraySize)
{
    #define CDBC_FUNC_NAME "altibase_stmt_set_array_bind"

    cdbcABStmt   *sABStmt = (cdbcABStmt *) aABStmt;
    ALTIBASE_RC   sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    sRC = altibase_result_set_array_bind(sABStmt->mRes, aArraySize);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    if (sABStmt->mArrayBindSize != aArraySize)
    {
        altibase_stmt_parambind_free(sABStmt);
    }

    sABStmt->mArrayBindSize = aArraySize;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * Array Fetch를 설정한다.
 *
 * @param[in] aABRes 결과 핸들
 * @param[in] aArraySize Array size
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC altibase_result_set_array_fetch (cdbcABRes *aABRes, acp_sint32_t aArraySize)
{
    #define CDBC_FUNC_NAME "altibase_result_set_array_fetch"

    acp_uint16_t  *sArrayStatus = NULL;
    acp_bool_t     sArrayStatusAlloced = ACP_FALSE;
    ALTIBASE_RC    sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HRES_NOT_VALID(aABRes), InvalidHandle);
    CDBC_TEST_RAISE(aArraySize < 1, InvalidParamRange);
    CDBCLOG_PRINT_VAL("%d", aArraySize);

    CDBCLOG_CALL("SQLSetStmtAttr");
    sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_ROW_ARRAY_SIZE,
                         (SQLPOINTER)(acp_ulong_t) aArraySize, 0);
    CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    if (aArraySize > 1)
    {
        if (aABRes->mArrayFetchSize != aArraySize)
        {
            CDBCLOG_CALL("acpMemCalloc");
            sRC = acpMemCalloc((void **)&sArrayStatus, aArraySize,
                               ACI_SIZEOF(acp_uint16_t));
            CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
            CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);
            sArrayStatusAlloced = ACP_TRUE;
        }
        else
        {
            sArrayStatus = aABRes->mArrayStatusResult;
        }

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_ROW_BIND_TYPE,
                             SQL_PARAM_BIND_BY_COLUMN, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_ROWS_FETCHED_PTR,
                             (SQLPOINTER) &(aABRes->mArrayFetched), 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_ROW_STATUS_PTR,
                             (SQLPOINTER) sArrayStatus, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        if (sArrayStatusAlloced == ACP_TRUE)
        {
            SAFE_FREE_AND_CLEAN(aABRes->mArrayStatusResult);
            aABRes->mArrayStatusResult = sArrayStatus;
        }
    }
    else /* if (aArraySize == 1) */
    {
        SAFE_FREE_AND_CLEAN(aABRes->mArrayStatusResult);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_ROWS_FETCHED_PTR,
                             (SQLPOINTER) NULL, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

        CDBCLOG_CALL("SQLSetStmtAttr");
        sRC = SQLSetStmtAttr(aABRes->mHstmt, SQL_ATTR_ROW_STATUS_PTR,
                             (SQLPOINTER) NULL, 0);
        CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
    }

    aABRes->mArrayFetchSize = aArraySize;
    CDBCLOG_PRINT_VAL("%d", aABRes->mArrayFetchSize);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(InvalidParamRange);
    {
        altibase_set_errinfo(aABRes->mDiagRec,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(aABRes, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(aABRes->mDiagRec,
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    if (sArrayStatusAlloced == ACP_TRUE)
    {
        SAFE_FREE(sArrayStatus);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * Array Fetch를 설정한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @param[in] aArraySize Array size
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_set_array_fetch (ALTIBASE_STMT aABStmt, acp_sint32_t aArraySize)
{
    #define CDBC_FUNC_NAME "altibase_stmt_set_array_fetch"

    cdbcABStmt    *sABStmt = (cdbcABStmt *) aABStmt;
    ALTIBASE_RC    sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    sRC = altibase_result_set_array_fetch(sABStmt->mRes, aArraySize);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sABStmt->mArrayFetchSize = aArraySize;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * Array Binding/Fetch 수행 결과를 얻는다.
 *
 * @param[in] aABRes 결과 핸들
 * @return Array Binding/Fetch 수행 결과를 담은 배열, 실패하면 NULL
 */
CDBC_INTERNAL
acp_uint16_t * altibase_result_status (cdbcABRes *aABRes)
{
    #define CDBC_FUNC_NAME "altibase_result_status"

    acp_uint16_t *sStatus = NULL;

    CDBCLOG_IN();

    CDBC_TEST(HRES_NOT_VALID(aABRes));
    CDBCLOG_PRINT_VAL("%p", aABRes->mState);
    CDBC_TEST_RAISE(RES_NOT_EXECUTED(aABRes), FuncSeqError);

    if (RES_IS_FETCHED(aABRes))
    {
        CDBC_TEST_RAISE(aABRes->mArrayFetchSize <= 1, FuncSeqError);
        sStatus = aABRes->mArrayStatusResult;
    }
    else /* is array bind */
    {
        CDBC_TEST_RAISE(aABRes->mArrayBindSize <= 1, FuncSeqError);
        sStatus = aABRes->mArrayStatusParam;
    }

    CDBCLOG_OUT_VAL("%p", sStatus);

    return sStatus;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(aABRes->mDiagRec,
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * Array Binding/Fetch 수행 결과를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return Array Binding/Fetch 수행 결과를 담은 배열, 실패하면 NULL
 */
CDBC_EXPORT
acp_uint16_t * altibase_stmt_status (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_status"

    cdbcABStmt   *sABStmt = (cdbcABStmt *) aABStmt;
    acp_uint16_t *sStatus = NULL;
    ALTIBASE_RC   sRC = ALTIBASE_ERROR;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    sStatus = altibase_result_status(sABStmt->mRes);
    CDBC_TEST(sStatus == NULL);

    CDBCLOG_OUT_VAL("%p", sStatus);

    return sStatus;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * Array Binding 수행 후 처리된 행의 개수를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return Array Binding 수행 후 처리된 행의 개수.
 *         실패하면 ALTIBASE_INVALID_PROCESSED
 */
CDBC_EXPORT
ALTIBASE_LONG altibase_stmt_processed (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_processed"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(STMT_NOT_EXECUTED(sABStmt) ||
                    STMT_IS_FETCHED(sABStmt) ||
                    (sABStmt->mArrayBindSize <= 1), FuncSeqError);

    CDBCLOG_OUT_VAL("%d", (acp_sint32_t)(sABStmt->mRes->mArrayProcessed));

    return sABStmt->mRes->mArrayProcessed;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_AFFECTEDROW");

    return ALTIBASE_INVALID_AFFECTEDROW;

    #undef CDBC_FUNC_NAME
}

/**
 * Array Fetch로 가져온 행의 개수를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return Array Fetch로 가져온 행의 개수. 실패하면 ALTIBASE_INVALID_FETCHED
 */
CDBC_EXPORT
ALTIBASE_LONG altibase_stmt_fetched (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_fetched"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(STMT_NOT_EXECUTED(sABStmt) ||
                    STMT_NOT_FETCHED(sABStmt) ||
                    (sABStmt->mArrayFetchSize <= 1), FuncSeqError);

    CDBCLOG_OUT_VAL("%d", (acp_sint32_t)(sABStmt->mRes->mArrayFetched));

    return sABStmt->mRes->mArrayFetched;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_FETCHED");

    return ALTIBASE_INVALID_FETCHED;

    #undef CDBC_FUNC_NAME
}

