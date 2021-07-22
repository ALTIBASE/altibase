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
 * 클라이언트 케릭터셋 정보를 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @param[in,out] aCharsetInfo 케릭터셋 정보를 담을 구조체
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_get_charset_info (ALTIBASE               aABConn,
                                       ALTIBASE_CHARSET_INFO *aCharsetInfo)
{
    #define CDBC_FUNC_NAME "altibase_get_charset_info"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HCONN_NOT_VALID(sABConn), InvalidHandle);

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBC_TEST_RAISE(aCharsetInfo == NULL, InvalidNullPtr);

    /* BUGBUG: (CLI) mbmaxlen 정보를 얻을 수 없다 */
    CDBC_RAISE(NotSupported);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(InvalidNullPtr);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(NotSupported);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_UNSUPPORTED_FUNCTION);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 클라이언트 캐릭터셋 이름을 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 클라이언트 캐릭터셋 이름. 에러가 발생하면 NULL
 */
CDBC_EXPORT
const acp_char_t * altibase_get_charset (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_get_charset"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    if (sABConn->mNlsLang[0] == '\0')
    {
        CDBCLOG_CALL("SQLGetConnectAttr");
        sRC = SQLGetConnectAttr(sABConn->mHdbc, ALTIBASE_NLS_USE,
                                sABConn->mNlsLang,
                                ACI_SIZEOF(sABConn->mNlsLang),
                                &(sABConn->mNlsLangLen));
        CDBCLOG_BACK_VAL("SQLGetConnectAttr", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);
    }

    CDBCLOG_OUT_VAL("%s", sABConn->mNlsLang);

    return sABConn->mNlsLang;

    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(sABConn, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 클라이언트 캐릭터셋을 이름으로 설정한다.
 *
 * @param[in] aABConn 연결 핸들
 * @param[in] aCharsetName 케릭터셋 이름
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_set_charset (ALTIBASE aABConn, const acp_char_t *aCharsetName)
{
    #define CDBC_FUNC_NAME "altibase_set_charset"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HCONN_NOT_VALID(sABConn), InvalidHandle);

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBC_TEST_RAISE(aCharsetName == NULL, InvalidNullPtr);

    CDBCLOG_CALL("SQLSetConnectAttr");
    sRC = SQLSetConnectAttr(sABConn->mHdbc, ALTIBASE_NLS_USE,
                            (SQLPOINTER) aCharsetName,
                            acpCStrLen(aCharsetName, ALTIBASE_MAX_NLS_USE_LEN));
    CDBCLOG_BACK_VAL("SQLSetConnectAttr", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    sABConn->mNlsLang[0] = '\0';

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(InvalidNullPtr);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
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

