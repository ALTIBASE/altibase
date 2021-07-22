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
 * 연결 옵션을 설정한다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_set_option (ALTIBASE aABConn, ALTIBASE_OPTION aOption, const void *aValue)
{
    #define CDBC_FUNC_NAME "altibase_set_option"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    SQLINTEGER  sLen;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HCONN_NOT_VALID(sABConn), InvalidHandle);

    altibase_init_errinfo(&(sABConn->mDiagRec));

    switch (aOption)
    {
        /* 수치 옵션 */
        case ALTIBASE_AUTOCOMMIT:
        case ALTIBASE_TXN_ISOLATION:
        case ALTIBASE_CONNECTION_TIMEOUT:
        case ALTIBASE_PORT:
        case ALTIBASE_NLS_NCHAR_LITERAL_REPLACE:
            sLen = 0;
            break;

        /* 문자열 옵션 */
        case ALTIBASE_DATE_FORMAT:
        case ALTIBASE_NLS_USE:
        case ALTIBASE_APP_INFO:
        case ALTIBASE_IPC_FILEPATH:
        case ALTIBASE_CLIENT_IP:
            sLen = SQL_NTS;
            break;

        /* 지원하지 않는 옵션 */
        default:
            CDBC_RAISE(NotSupported);
            break;
    }

    CDBCLOG_CALL("SQLSetConnectAttr");
    sRC = SQLSetConnectAttr(sABConn->mHdbc, (SQLINTEGER) aOption,
                            (SQLPOINTER) aValue, sLen);
    CDBCLOG_BACK_VAL("SQLSetConnectAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(sABConn, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(NotSupported);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_INVALID_ATTR_OPTION,
                             aOption);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 명령문 속성을 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @param[in] aOption 명령문 속성 유형
 * @param[in] aBuffer 명령문 속성 값을 담을 버퍼
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_get_attr (ALTIBASE_STMT            aABStmt,
                                    ALTIBASE_STMT_ATTR_TYPE  aOption,
                                    void                    *aBuffer)
{
    #define CDBC_FUNC_NAME "altibase_stmt_get_attr"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;
    SQLINTEGER  sLen;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    switch (aOption)
    {
        case ALTIBASE_STMT_ATTR_ATOMIC_ARRAY:
            sLen = ACI_SIZEOF(acp_uint32_t);
            break;

        /* 지원하지 않는 옵션 */
        default:
            CDBC_RAISE(NotSupported);
            break;
    }

    CDBCLOG_CALL("SQLGetStmtAttr");
    sRC = SQLGetStmtAttr(sABStmt->mHstmt, (SQLINTEGER) aOption, aBuffer, sLen, NULL);
    CDBCLOG_BACK_VAL("SQLGetStmtAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_stmt(sABStmt, sRC);
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }
    CDBC_EXCEPTION(NotSupported);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_INVALID_ATTR_OPTION,
                             aOption);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 명령문 속성을 설정한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @param[in] aOption 명령문 속성 유형
 * @param[in] aValue  명령문 속성 값
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_set_attr (ALTIBASE_STMT            aABStmt,
                                    ALTIBASE_STMT_ATTR_TYPE  aOption,
                                    const void              *aValue)
{
    #define CDBC_FUNC_NAME "altibase_stmt_set_attr"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;
    SQLINTEGER  sLen;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    switch (aOption)
    {
        case ALTIBASE_STMT_ATTR_ATOMIC_ARRAY:
            sLen = 0;
            break;

        /* 지원하지 않는 옵션 */
        default:
            CDBC_RAISE(NotSupported);
            break;
    }

    CDBCLOG_CALL("SQLSetStmtAttr");
    sRC = SQLSetStmtAttr(sABStmt->mHstmt, (SQLINTEGER) aOption,
                         (SQLPOINTER) aValue, sLen);
    CDBCLOG_BACK_VAL("SQLSetStmtAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(STMTError);
    {
        altibase_set_errinfo_by_stmt(sABStmt, sRC);
        SAFE_FAILOVER_POST_PROC(sABStmt);
    }
    CDBC_EXCEPTION(NotSupported);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_INVALID_ATTR_OPTION,
                             aOption);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

#if defined(SUPPORT_GET_AUTOCOMMIT)
/**
 * 설정된 오토커밋 모드를 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 오토커밋 모드(ALTIBASE_AUTOCOMMIT_ON 또는 ALTIBASE_AUTOCOMMIT_OFF),
 *         실패하면 ALTIBASE_ERROR
 */
CDBC_EXPORT
acp_sint32_t altibase_get_autocommit (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_get_autocommit"

    cdbcABConn   *sABConn = (cdbcABConn *) aABConn;
    acp_uint32_t  sMode;
    acp_rc_t      sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBCLOG_CALL("SQLGetConnectAttr");
    sRC = SQLGetConnectAttr(sABConn->mHdbc, SQL_ATTR_AUTOCOMMIT, (void *) sMode, 0, 0);
    CDBCLOG_BACK_VAL("SQLGetConnectAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    CDBCLOG_OUT_VAL("%d", sMode);

    return (acp_sint32_t) sMode;

    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(sABConn, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_ERROR");

    return ALTIBASE_ERROR;

    #undef CDBC_FUNC_NAME
}
#endif

/**
 * 오토커밋 모드를 설정한다.
 *
 * @param[in] aABConn 연결 핸들
 * @param[in] aMode 오토커밋 모드 여부.
 *                  ALTIBASE_AUTOCOMMIT_ON 또는 ALTIBASE_AUTOCOMMIT_OFF
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_set_autocommit (ALTIBASE aABConn, acp_sint32_t aMode)
{
    #define CDBC_FUNC_NAME "altibase_set_autocommit"

    cdbcABConn     *sABConn = (cdbcABConn *) aABConn;
    acp_slong_t     sCliMode;
    ALTIBASE_RC     sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HCONN_NOT_VALID(sABConn), InvalidHandle);

    altibase_init_errinfo(&(sABConn->mDiagRec));

    switch (aMode)
    {
        case ALTIBASE_AUTOCOMMIT_ON:
            sCliMode = SQL_AUTOCOMMIT_ON;
            break;
        case ALTIBASE_AUTOCOMMIT_OFF:
            sCliMode = SQL_AUTOCOMMIT_OFF;
            break;
        default:
            CDBC_RAISE(InvalidParamRange);
            break;
    }

    CDBCLOG_CALL("SQLSetConnectAttr");
    sRC = SQLSetConnectAttr(sABConn->mHdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) sCliMode, 0);
    CDBCLOG_BACK_VAL("SQLSetConnectAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(InvalidParamRange);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(sABConn, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

