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
 * 쿼리문의 유형을 확인한다.
 *
 * 쿼리문이 어떻게 시작하는지로 어떤 유형인지 판단하므로
 * 단순한 쿼리에 대해서만 가능하다.
 * procedure나 function에 대해서는 확인할 수 없다.
 *
 * @param[in] aQstr 종류를 확인할 쿼리문
 * @return 쿼리문의 종류
 */
CDBC_INTERNAL
cdbcABQueryType altibase_query_type (const acp_char_t *aQstr)
{
    #define CDBC_FUNC_NAME "altibase_query_type"

    cdbcABQueryType  sType   = ALTIBASE_QUERY_NONE;
    acp_char_t      *sCurPtr = (acp_char_t *) aQstr;
    acp_sint32_t     sLen;

    CDBCLOG_IN();

    CDBC_DASSERT(sCurPtr != NULL);

    while (acpCharIsSpace(*sCurPtr) == ACP_TRUE)
    {
        sCurPtr++;
    }

    sLen = acpCStrLen(sCurPtr, ACP_SINT32_MAX);
    if (sLen < 9)
    {
        sType = ALTIBASE_QUERY_UNKNOWN;
    }
    else if (acpCStrCaseCmp("SELECT ", sCurPtr, 7) == 0)
    {
        sType = ALTIBASE_QUERY_SELECT;
    }
    else if (acpCStrCaseCmp("INSERT ", sCurPtr, 7) == 0)
    {
        sType = ALTIBASE_QUERY_INSERT;
    }
    else if (acpCStrCaseCmp("UPDATE ", sCurPtr, 7) == 0)
    {
        sType = ALTIBASE_QUERY_UPDATE;
    }
    else if (acpCStrCaseCmp("DELETE ", sCurPtr, 7) == 0)
    {
        sType = ALTIBASE_QUERY_DELETE;
    }
    else if ((acpCStrCaseCmp("CREATE "  , sCurPtr, 7) == 0)
          || (acpCStrCaseCmp("DROP "    , sCurPtr, 5) == 0)
          || (acpCStrCaseCmp("ALTER "   , sCurPtr, 6) == 0)
          || (acpCStrCaseCmp("RENAME "  , sCurPtr, 7) == 0)
          || (acpCStrCaseCmp("TRUNCATE ", sCurPtr, 9) == 0))
    {
        sType = ALTIBASE_QUERY_DDL;
    }
    else
    {
        sType = ALTIBASE_QUERY_UNKNOWN;
    }

    CDBCLOG_OUT_VAL("%d", sType);

    return sType;

    #undef CDBC_FUNC_NAME
}

/**
 * SQL 타입에 해당하는 ALTIBASE_FIELD_TYPE을 얻는다.
 *
 * @param[in] aSqlType SQL 타입
 * @return SQL 타입에 해당하는 ALTIBASE_FIELD_TYPE
 */
CDBC_INTERNAL
ALTIBASE_FIELD_TYPE altibase_typeconv_sql2alti (SQLSMALLINT aSqlType)
{
    #define CDBC_FUNC_NAME "altibase_typeconv_sql2alti"

    ALTIBASE_FIELD_TYPE sType;

    CDBCLOG_IN();

    switch (aSqlType)
    {
        case SQL_CHAR:           sType = ALTIBASE_TYPE_CHAR; break;
        case SQL_VARCHAR:        sType = ALTIBASE_TYPE_VARCHAR; break;
        case SQL_WCHAR:          sType = ALTIBASE_TYPE_NCHAR; break;
        case SQL_WVARCHAR:       sType = ALTIBASE_TYPE_NVARCHAR; break;
        case SQL_NUMERIC:        sType = ALTIBASE_TYPE_NUMERIC; break;
        case SQL_FLOAT:          sType = ALTIBASE_TYPE_FLOAT; break;
        case SQL_DOUBLE:         sType = ALTIBASE_TYPE_DOUBLE; break;
        case SQL_REAL:           sType = ALTIBASE_TYPE_REAL; break;
        case SQL_BIGINT:         sType = ALTIBASE_TYPE_BIGINT; break;
        case SQL_INTEGER:        sType = ALTIBASE_TYPE_INTEGER; break;
        case SQL_SMALLINT:       sType = ALTIBASE_TYPE_SMALLINT; break;
        case SQL_TYPE_TIMESTAMP: sType = ALTIBASE_TYPE_DATE; break;
        case SQL_BLOB:           sType = ALTIBASE_TYPE_BLOB; break;
        case SQL_CLOB:           sType = ALTIBASE_TYPE_CLOB; break;
        case SQL_BYTE:           sType = ALTIBASE_TYPE_BYTE; break;
        case SQL_NIBBLE:         sType = ALTIBASE_TYPE_NIBBLE; break;
        case SQL_BIT:            sType = ALTIBASE_TYPE_BIT; break;
        case SQL_VARBIT:         sType = ALTIBASE_TYPE_VARBIT; break;
        case SQL_BINARY:         /* GEOMETRY 컬럼을 desc하면 SQL_BINARY을 줌 */
        case SQL_GEOMETRY:       sType = ALTIBASE_TYPE_GEOMETRY; break;
        default:                 sType = ALTIBASE_TYPE_UNKNOWN; break;
    }

    CDBCLOG_OUT_VAL("%d", sType);

    return sType;

    #undef CDBC_FUNC_NAME
}

/**
 * ALTIBASE_FIELD_TYPE에 해당하는 SQL 타입을 얻는다.
 *
 * @param[in] aAbType ALTIBASE_FIELD_TYPE 값
 * @return ALTIBASE_FIELD_TYPE에 해당하는 SQL 타입
 */
CDBC_INTERNAL
SQLSMALLINT altibase_typeconv_alti2sql (ALTIBASE_FIELD_TYPE aAbType)
{
    #define CDBC_FUNC_NAME "altibase_typeconv_alti2sql"

    SQLSMALLINT sType;

    CDBCLOG_IN();

    switch (aAbType)
    {
        case ALTIBASE_TYPE_CHAR:     sType = SQL_CHAR; break;
        case ALTIBASE_TYPE_VARCHAR:  sType = SQL_VARCHAR; break;
        case ALTIBASE_TYPE_NCHAR:    sType = SQL_WCHAR; break;
        case ALTIBASE_TYPE_NVARCHAR: sType = SQL_WVARCHAR; break;
        case ALTIBASE_TYPE_NUMERIC:  sType = SQL_NUMERIC; break;
        case ALTIBASE_TYPE_FLOAT:    sType = SQL_FLOAT; break;
        case ALTIBASE_TYPE_DOUBLE:   sType = SQL_DOUBLE; break;
        case ALTIBASE_TYPE_REAL:     sType = SQL_REAL; break;
        case ALTIBASE_TYPE_BIGINT:   sType = SQL_BIGINT; break;
        case ALTIBASE_TYPE_INTEGER:  sType = SQL_INTEGER; break;
        case ALTIBASE_TYPE_SMALLINT: sType = SQL_SMALLINT; break;
        case ALTIBASE_TYPE_DATE:     sType = SQL_TYPE_TIMESTAMP; break;
        case ALTIBASE_TYPE_BLOB:     sType = SQL_BLOB; break;
        case ALTIBASE_TYPE_CLOB:     sType = SQL_CLOB; break;
        case ALTIBASE_TYPE_BYTE:     sType = SQL_BYTE; break;
        case ALTIBASE_TYPE_NIBBLE:   sType = SQL_NIBBLE; break;
        case ALTIBASE_TYPE_BIT:      sType = SQL_BIT; break;
        case ALTIBASE_TYPE_VARBIT:   sType = SQL_VARBIT; break;
        case ALTIBASE_TYPE_GEOMETRY: sType = SQL_GEOMETRY; break;
        default:                     sType = SQL_BINARY; break;
    }

    CDBCLOG_OUT_VAL("%d", sType);

    return sType;

    #undef CDBC_FUNC_NAME
}

/**
 * 연결 정보 문자열을 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return "type/host:port/app_info" 형식의 연결 정보 문자열, 실패하면 NULL
 */
CDBC_EXPORT
const acp_char_t * altibase_host_info (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_host_info"

    cdbcABConn     *sABConn = (cdbcABConn *) aABConn;
    acp_char_t     *sConnTypeStr;
    acp_char_t     *sHost;
    acp_sint32_t    sPortNo;
    acp_char_t      sAppInfo[ALTIBASE_MAX_APP_INFO_LEN] = { '\0', };
    acp_rc_t        sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    /* BUGBUG: (CLI) type, host 정보를 얻을 수 없다 */
    CDBC_RAISE(NotSupported);

    sRC = SQLGetConnectAttr(sABConn->mHdbc, SQL_ATTR_PORT,
                            &sPortNo, 0, NULL);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    sRC = SQLGetConnectAttr(sABConn->mHdbc, ALTIBASE_APP_INFO,
                            sAppInfo, ACI_SIZEOF(sAppInfo), NULL);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    if (sAppInfo != NULL)
    {
        sRC = acpSnprintf(sABConn->mHostInfo, ACI_SIZEOF(sABConn->mHostInfo),
                          "%s/%s:%d/%s",
                          sConnTypeStr, sHost, sPortNo, sAppInfo);
    }
    else
    {
        sRC = acpSnprintf(sABConn->mHostInfo, ACI_SIZEOF(sABConn->mHostInfo),
                          "%s/%s:%d",
                          sConnTypeStr, sHost, sPortNo);
    }
    CDBC_TEST(ACP_RC_NOT_SUCCESS(sRC));

    CDBCLOG_OUT_VAL("%s", sABConn->mHostInfo);

    return sABConn->mHostInfo;

    CDBC_EXCEPTION(NotSupported);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_UNSUPPORTED_FUNCTION);
    }
    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(sABConn, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

