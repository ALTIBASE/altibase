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

SQLRETURN SQL_API SQLNumRows(SQLHSTMT StatementHandle, SQLLEN *NumRows);



/* BUG-44572 */
static SQLRETURN altibase_affected_rows_core(SQLHSTMT aHstmt, SQLLEN *aRowCount)
{
    #define CDBC_FUNC_NAME "altibase_affected_rows_core"

    SQLLEN    sRowCount;
    SQLRETURN sRC;

    CDBCLOG_IN();

    CDBCLOG_CALL("SQLRowCount");
    sRC = SQLRowCount(aHstmt, &sRowCount);
    CDBCLOG_BACK_VAL("SQLRowCount", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), RETURN);

    if (sRowCount == -1)
    {
        CDBCLOG_CALL("SQLNumRows");
        sRC = SQLNumRows(aHstmt, &sRowCount);
        CDBCLOG_BACK_VAL("SQLNumRows", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), RETURN);

        /* 에러가 났을때만 -1을 줘야한다. */
        if (sRowCount == -1)
        {
            sRowCount = 0;
        }
    }

    *aRowCount = sRowCount;

    CDBC_EXCEPTION_CONT(RETURN);

    CDBCLOG_OUT_VAL("%d", sRC);

    return sRC;

    #undef CDBC_FUNC_NAME
}



/**
 * INSERT, UPDATE, DELETE 쿼리에 영향 받은 행의 개수를 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 영향 받은 행의 개수. 에러가 발생하면 ALTIBASE_INVALID_AFFECTEDROW
 */
CDBC_EXPORT
ALTIBASE_LONG altibase_affected_rows (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_affected_rows"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    SQLLEN      sRowCount;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    sRC = altibase_affected_rows_core(sABConn->mHstmt, &sRowCount);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    CDBCLOG_OUT_VAL("%d", (acp_sint32_t) sRowCount);

    return (ALTIBASE_LONG) sRowCount;

    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_connstmt(sABConn, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_AFFECTEDROW");

    return ALTIBASE_INVALID_AFFECTEDROW;

    #undef CDBC_FUNC_NAME
}

/**
 * INSERT, UPDATE, DELETE 쿼리에 영향 받은 행의 개수를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 영향 받은 행의 개수. 에러가 발생하면 ALTIBASE_INVALID_AFFECTEDROW
 */
CDBC_EXPORT
ALTIBASE_LONG altibase_stmt_affected_rows (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_affected_rows"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;
    SQLLEN      sRowCount;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    sRC = altibase_affected_rows_core(sABStmt->mHstmt, &sRowCount);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    CDBCLOG_OUT_VAL("%d", (acp_sint32_t) sRowCount);

    return (ALTIBASE_LONG) sRowCount;

    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_stmt(sABStmt, sRC);
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_AFFECTEDROW");

    return ALTIBASE_INVALID_AFFECTEDROW;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋의 컬럼 수를 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 결과셋의 컬럼 수, 실패하면 ALTIBASE_INVALID_FIELDCOUNT
 */
CDBC_EXPORT
acp_sint32_t altibase_field_count (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_field_count"

    cdbcABConn  *sABConn = (cdbcABConn *) aABConn;
    SQLSMALLINT  sColCount;
    acp_rc_t     sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBCLOG_CALL("SQLNumResultCols");
    sRC = SQLNumResultCols(sABConn->mHstmt, &sColCount);
    CDBCLOG_BACK_VAL("SQLNumResultCols", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    CDBCLOG_OUT_VAL("%d", sColCount);

    return sColCount;

    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_connstmt(sABConn, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_FIELDCOUNT");

    return ALTIBASE_INVALID_FIELDCOUNT;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋의 컬럼 수를 얻는다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 결과셋의 컬럼 수, 실패하면 ALTIBASE_INVALID_FIELDCOUNT
 */
CDBC_EXPORT
acp_sint32_t altibase_num_fields (ALTIBASE_RES aABRes)
{
    #define CDBC_FUNC_NAME "altibase_num_fields"

    cdbcABRes   *sABRes = (cdbcABRes *) aABRes;
    acp_rc_t     sRC;

    CDBCLOG_IN();

    CDBC_TEST(HROM_NOT_VALID(sABRes));

    altibase_init_errinfo(sABRes->mDiagRec);

    if (sABRes->mFieldCount == ALTIBASE_INVALID_FIELDCOUNT)
    {
        CDBCLOG_CALL("SQLNumResultCols");
        sRC = SQLNumResultCols(sABRes->mHstmt, &(sABRes->mFieldCount));
        CDBCLOG_BACK_VAL("SQLNumResultCols", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
        CDBCLOG_PRINT_VAL("%d", sABRes->mFieldCount);
    }

    CDBCLOG_OUT_VAL("%d", sABRes->mFieldCount);

    return sABRes->mFieldCount;

    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(sABRes, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_FIELDCOUNT");

    return ALTIBASE_INVALID_FIELDCOUNT;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋의 컬럼 수를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 결과셋의 컬럼 수, 실패하면 ALTIBASE_INVALID_FIELDCOUNT
 */
CDBC_EXPORT
acp_sint32_t altibase_stmt_field_count (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_field_count"

    cdbcABStmt     *sABStmt = (cdbcABStmt *) aABStmt;
    acp_sint32_t    sColCount;
    ALTIBASE_RC     sRC = ALTIBASE_ERROR;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    sColCount = altibase_num_fields(sABStmt->mRes);
    CDBC_TEST(sColCount == ALTIBASE_INVALID_FIELDCOUNT);

    CDBCLOG_OUT_VAL("%d", sColCount);

    return sColCount;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION_END;

    if (sRC != ALTIBASE_INVALID_HANDLE)
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_FIELDCOUNT");

    return ALTIBASE_INVALID_FIELDCOUNT;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋의 필드 정보를 얻는다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @param[in] aIdx 컬럼 인덱스 (0 부터 시작)
 * @return 필드 정보, 실패하면 NULL
 */
CDBC_EXPORT
ALTIBASE_FIELD * altibase_field (ALTIBASE_RES aABRes, acp_sint32_t aIdx)
{
    #define CDBC_FUNC_NAME "altibase_field"

    cdbcABRes      *sABRes = (cdbcABRes *) aABRes;
    ALTIBASE_FIELD *sField;
    acp_rc_t        sRC;

    CDBCLOG_IN();

    CDBC_TEST(HROM_NOT_VALID(sABRes));

    altibase_init_errinfo(sABRes->mDiagRec);

    sRC = altibase_ensure_full_fieldinfos(sABRes);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    CDBC_TEST_RAISE(INDEX_NOT_VALID(aIdx, 0, sABRes->mFieldCount),
                    InvalidParamRange);

    sField = &(sABRes->mFieldInfos[aIdx]);

    CDBCLOG_OUT_VAL("%p", sField);

    return sField;

    CDBC_EXCEPTION(InvalidParamRange);
    {
        altibase_set_errinfo(sABRes->mDiagRec,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋의 필드 정보를 모두 얻는다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 결과셋의 모든 필드 정보, 실패하면 NULL
 */
CDBC_EXPORT
ALTIBASE_FIELD * altibase_fields (ALTIBASE_RES aABRes)
{
    #define CDBC_FUNC_NAME "altibase_fields"

    cdbcABRes *sABRes = (cdbcABRes *) aABRes;
    acp_rc_t   sRC;

    CDBCLOG_IN();

    CDBC_TEST(HROM_NOT_VALID(sABRes));

    altibase_init_errinfo(sABRes->mDiagRec);

    sRC = altibase_ensure_full_fieldinfos(sABRes);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    CDBCLOG_OUT_VAL("%p", sABRes->mFieldInfos);

    return sABRes->mFieldInfos;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 필드의 기본 정보가 없으면 얻어와 채운다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC altibase_ensure_basic_fieldinfos (cdbcABRes *aABRes)
{
    #define CDBC_FUNC_NAME "altibase_ensure_basic_fieldinfos"

    ALTIBASE_FIELD *sColInfos = NULL;
    acp_sint32_t    sColCount = 0;
    SQLULEN         sColSize  = 0;
    SQLSMALLINT     sColScale = 0;
    SQLSMALLINT     sDataType = SQL_UNKNOWN_TYPE;
    SQLSMALLINT     sNameLen  = 0;
    acp_rc_t        sRC;
    acp_sint32_t    i;

    CDBCLOG_IN();

    sColCount = altibase_num_fields(aABRes);
    CDBC_TEST(sColCount == ALTIBASE_INVALID_FIELDCOUNT);
    CDBCLOG_PRINT_VAL("%d", sColCount);

    if (aABRes->mFieldInfos == NULL)
    {
        CDBCLOG_CALL("acpMemCalloc");
        sRC = acpMemCalloc((void **)&sColInfos, sColCount,
                           ACI_SIZEOF(ALTIBASE_FIELD));
        CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
        CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);

        for (i = 0; i < sColCount; i++)
        {
            CDBCLOG_CALL("SQLDescribeCol");
            sRC = SQLDescribeCol(aABRes->mHstmt,
                                 i+1,
                                 (SQLCHAR *) &(sColInfos[i].name),
                                 ALTIBASE_MAX_FIELD_NAME_LEN,
                                 &sNameLen,
                                 &sDataType,
                                 &sColSize,
                                 &sColScale,
                                 NULL);
            CDBCLOG_BACK_VAL("SQLDescribeCol", "%d", sRC);
            CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
            sColInfos[i].name_length = sNameLen;
            sColInfos[i].size = (acp_sint32_t) sColSize;
            sColInfos[i].scale = (acp_sint32_t) sColScale;
            sColInfos[i].type = altibase_typeconv_sql2alti(sDataType);
        }

        aABRes->mFieldInfos = sColInfos;
    }

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(aABRes->mDiagRec,
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(aABRes, sRC);
    }
    CDBC_EXCEPTION_END;

    SAFE_FREE(sColInfos);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_ERROR");

    return ALTIBASE_ERROR;

    #undef CDBC_FUNC_NAME
}

/**
 * 얻어오지 않은 필드 정보가 없으면 얻어와 채운다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC altibase_ensure_full_fieldinfos (cdbcABRes *aABRes)
{
    #define CDBC_FUNC_NAME "altibase_ensure_full_fieldinfos"

    ALTIBASE_FIELD *sColInfos = NULL;
    SQLSMALLINT     sNameLen;
    acp_rc_t        sRC;
    acp_sint32_t    i;

    CDBCLOG_IN();

    sRC = altibase_ensure_basic_fieldinfos(aABRes);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    if (aABRes->mFieldInfoEx == ACP_FALSE)
    {
        sColInfos = aABRes->mFieldInfos;

        for (i = 0; i < aABRes->mFieldCount; i++)
        {
            /* org_name */
            CDBCLOG_CALL("SQLColAttribute");
            sRC = SQLColAttribute(aABRes->mHstmt, i+1, SQL_DESC_BASE_COLUMN_NAME,
                                  &(sColInfos[i].org_name), ALTIBASE_MAX_FIELD_NAME_LEN,
                                  &sNameLen, NULL);
            CDBCLOG_BACK_VAL("SQLColAttribute", "%d", sRC);
            CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
            sColInfos[i].org_name_length = sNameLen;

            /* table */
            CDBCLOG_CALL("SQLColAttribute");
            sRC = SQLColAttribute(aABRes->mHstmt, i+1, SQL_DESC_TABLE_NAME,
                                  &(sColInfos[i].table), ALTIBASE_MAX_TABLE_NAME_LEN,
                                  &sNameLen, NULL);
            CDBCLOG_BACK_VAL("SQLColAttribute", "%d", sRC);
            CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
            sColInfos[i].table_length = sNameLen;

            /* org_table */
            CDBCLOG_CALL("SQLColAttribute");
            sRC = SQLColAttribute(aABRes->mHstmt, i+1, SQL_DESC_BASE_TABLE_NAME,
                                  &(sColInfos[i].org_table), ALTIBASE_MAX_TABLE_NAME_LEN,
                                  &sNameLen, NULL);
            CDBCLOG_BACK_VAL("SQLColAttribute", "%d", sRC);
            CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
            sColInfos[i].org_table_length = sNameLen;
        }
        aABRes->mFieldInfoEx = ACP_TRUE;
    }

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_res(aABRes, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_ERROR");

    return ALTIBASE_ERROR;

    #undef CDBC_FUNC_NAME
}

/**
 * 준비된 문장의 파라미터 개수를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 성공했으면 파라미터 개수, 그렇지 않으면 ALTIBASE_INVALID_PARAMCOUNT
 */
CDBC_EXPORT
acp_sint32_t altibase_stmt_param_count (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_param_count"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(STMT_NOT_PREPARED(sABStmt), FuncSeqError);

    CDBCLOG_OUT_VAL("%d", sABStmt->mParamCount);

    return sABStmt->mParamCount;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_PARAMCOUNT");

    return ALTIBASE_INVALID_PARAMCOUNT;

    #undef CDBC_FUNC_NAME
}

/**
 * 준비된 문장의 메타 정보를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 성공했으면 메타 정보를 담은 결과셋, 그렇지 않으면 NULL
 * @see altibase_stmt_prepare
 */
CDBC_EXPORT
ALTIBASE_RES altibase_stmt_result_metadata (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_result_metadata"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;
    cdbcABRes  *sABRes = NULL;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(STMT_NOT_PREPARED(sABStmt), FuncSeqError);
    CDBC_DASSERT(sABStmt->mRes != NULL);

    sRC = altibase_ensure_full_fieldinfos(sABStmt->mRes);
    CDBC_TEST(ALTIBASE_NOT_SUCCEEDED(sRC));

    sABRes = altibase_stmt_result_init(sABStmt, ALTIBASE_HANDLE_RES_META);
    CDBC_TEST(sABRes == NULL);

    sABRes->mFieldCount  = sABStmt->mRes->mFieldCount;
    sABRes->mFieldInfos  = sABStmt->mRes->mFieldInfos;
    sABRes->mFieldInfoEx = sABStmt->mRes->mFieldInfoEx;

    CDBCLOG_OUT_VAL("%p", sABRes);

    return sABRes;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION_END;

    if (HSTMT_IS_VALID(sABStmt))
    {
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }

    CDBC_DASSERT(sABRes == NULL);

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

