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
#include <iduVersionDef.h>



CDBC_INTERNAL acp_sint32_t altibase_parse_verstr (
    acp_sint32_t       *aDestVerArr,
    acp_sint32_t        aDestVerArrCnt,
    const acp_char_t   *aSrcVerStr,
    acp_sint32_t        aSrcVerStrLen
);



static acp_char_t g_ClientVerStr[ALTIBASE_MAX_VERSTR_LEN + 1] = {'\0', };



/**
 * 클라이언트 라이브러리 버전 문자열을 얻는다.
 *
 * @return "x.x.x.x.x" 형식의 클라이언트 라이브러리 버전 문자열, 실패하면 NULL
 */
CDBC_EXPORT
const acp_char_t * altibase_client_verstr (void)
{
    #define CDBC_FUNC_NAME "altibase_client_verstr"

    acp_rc_t sRC;

    CDBCLOG_IN();

    if (g_ClientVerStr[0] == '\0')
    {
        sRC = acpSnprintf(g_ClientVerStr, ACI_SIZEOF(g_ClientVerStr),
                          ALTIBASE_VERSTR_FORM,
                          IDU_ALTIBASE_MAJOR_VERSION,
                          IDU_ALTIBASE_MINOR_VERSION,
                          IDU_ALTIBASE_DEV_VERSION,
                          IDU_ALTIBASE_PATCHSET_LEVEL,
                          IDU_ALTIBASE_PATCH_LEVEL);
        CDBC_TEST(ACP_RC_NOT_SUCCESS(sRC));
    }

    CDBCLOG_OUT_VAL("%s", g_ClientVerStr);

    return g_ClientVerStr;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 클라이언트 라이브러리 버전 값을 얻는다.
 *
 * @return MMmmttSSpp 형식의 클라이언트 라이브러리 버전
 */
CDBC_EXPORT
acp_sint32_t altibase_client_version (void)
{
    #define CDBC_FUNC_NAME "altibase_client_version"

    acp_sint32_t sVer;

    CDBCLOG_IN();

    sVer = (IDU_ALTIBASE_MAJOR_VERSION  * ALTIBASE_MAJOR_VER_UNIT)
         + (IDU_ALTIBASE_MINOR_VERSION  * ALTIBASE_MINOR_VER_UNIT)
         + (IDU_ALTIBASE_DEV_VERSION    * ALTIBASE_TERM_VER_UNIT)
         + (IDU_ALTIBASE_PATCHSET_LEVEL * ALTIBASE_PATCHSET_LEVEL_UNIT)
         + (IDU_ALTIBASE_PATCH_LEVEL    * ALTIBASE_PATCH_LEVEL_UNIT);

    CDBCLOG_OUT_VAL("%d", sVer);

    return sVer;

    #undef CDBC_FUNC_NAME
}

/**
 * 통신 프로토콜 버전 문자열을 얻는다.
 *
 * @return "x.x.0.0.x" 형식의 통신 프로토콜 버전 문자열, 실패하면 NULL
 */
CDBC_EXPORT
const acp_char_t * altibase_proto_verstr (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_proto_verstr"

    cdbcABConn     *sABConn = (cdbcABConn *) aABConn;
    acp_sint32_t    sVer;
    acp_rc_t        sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    if (sABConn->mProtoVerStr[0] == '\0')
    {
        sVer = altibase_proto_version(aABConn);
        CDBC_TEST(sVer == ALTIBASE_INVALID_VERSION);

        sRC = acpSnprintf(sABConn->mProtoVerStr, ACI_SIZEOF(sABConn->mProtoVerStr),
                          ALTIBASE_VERSTR_FORM,
                          GET_MAJOR_VERSION(sVer),
                          GET_MINOR_VERSION(sVer),
                          GET_TERM_VERSION(sVer),
                          GET_PATCHSET_VERSION(sVer),
                          GET_PATCH_VERSION(sVer));
        CDBC_TEST(ACP_RC_NOT_SUCCESS(sRC));
    }

    CDBCLOG_OUT_VAL("%s", sABConn->mProtoVerStr);

    return sABConn->mProtoVerStr;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 통신 프로토콜 버전 값을 얻는다.
 *
 * @return MMmm0000pp 형식의 통신 프로토콜 버전
 */
CDBC_EXPORT
acp_sint32_t altibase_proto_version (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_proto_version"

    cdbcABConn     *sABConn = (cdbcABConn *) aABConn;
    acp_sint32_t    sVer[3] = { 0, };
    acp_rc_t        sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    if (sABConn->mProtoVer == ALTIBASE_INVALID_VERSION)
    {
        CDBCLOG_CALL("SQLGetInfo");
        sRC = SQLGetInfo(sABConn->mHdbc, ALTIBASE_PROTO_VER,
                         sABConn->mProtoVerStr,
                         ACI_SIZEOF(sABConn->mProtoVerStr),
                         &(sABConn->mProtoVerStrLen));
        CDBCLOG_BACK_VAL("SQLGetInfo", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

        sRC = altibase_parse_verstr(sVer, 3,
                                    sABConn->mProtoVerStr,
                                    sABConn->mProtoVerStrLen);
        CDBC_TEST_RAISE(sRC != 3, VerStrParseError);

        sABConn->mProtoVer = (sVer[0] * ALTIBASE_MAJOR_VER_UNIT)
                           + (sVer[1] * ALTIBASE_MINOR_VER_UNIT)
                           + (sVer[2] * ALTIBASE_PATCH_LEVEL_UNIT);
    }

    CDBCLOG_OUT_VAL("%d", sABConn->mProtoVer);

    return sABConn->mProtoVer;

    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(sABConn, sRC);
    }
    CDBC_EXCEPTION(VerStrParseError);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_VERSION");

    return ALTIBASE_INVALID_VERSION;

    #undef CDBC_FUNC_NAME
}

/**
 * 서버 버전 문자열을 얻는다.
 *
 * @return "x.x.x.x.x" 형식의 서버 버전 문자열, 실패하면 NULL
 */
CDBC_EXPORT
const acp_char_t * altibase_server_verstr (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_server_verstr"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    if (sABConn->mServerVerStr[0] == '\0')
    {
        CDBCLOG_CALL("SQLGetInfo");
        sRC = SQLGetInfo(sABConn->mHdbc, SQL_DBMS_VER,
                         sABConn->mServerVerStr,
                         ACI_SIZEOF(sABConn->mServerVerStr),
                         &(sABConn->mServerVerStrLen));
        CDBCLOG_BACK_VAL("SQLGetInfo", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);
    }

    CDBCLOG_OUT_VAL("%s", sABConn->mServerVerStr);

    return sABConn->mServerVerStr;

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
 * 서버 버전 값을 얻는다.
 *
 * @return MMmmttSSpp 형식의 서버 버전, 실패하면 ALTIBASE_INVALID_VERSION
 */
CDBC_EXPORT
acp_sint32_t altibase_server_version (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_server_version"

    cdbcABConn         *sABConn = (cdbcABConn *) aABConn;
    const acp_char_t   *sServerVerstr;
    acp_sint32_t        sVer[ALTIBASE_VER_PARTS_CNT] = { 0, };
    acp_rc_t            sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    if (sABConn->mServerVer == ALTIBASE_INVALID_VERSION)
    {
        sServerVerstr = altibase_server_verstr(aABConn);
        CDBC_TEST(sServerVerstr == NULL);

        sRC = altibase_parse_verstr(sVer, ALTIBASE_VER_PARTS_CNT,
                                    sABConn->mServerVerStr,
                                    sABConn->mServerVerStrLen);
        CDBC_TEST_RAISE(sRC != ALTIBASE_VER_PARTS_CNT, VerStrParseError);

        sABConn->mServerVer = (sVer[0] * ALTIBASE_MAJOR_VER_UNIT)
                            + (sVer[1] * ALTIBASE_MINOR_VER_UNIT)
                            + (sVer[2] * ALTIBASE_TERM_VER_UNIT)
                            + (sVer[3] * ALTIBASE_PATCHSET_LEVEL_UNIT)
                            + (sVer[4] * ALTIBASE_PATCH_LEVEL_UNIT);
    }

    CDBCLOG_OUT_VAL("%d", sABConn->mServerVer);

    return sABConn->mServerVer;

    CDBC_EXCEPTION(VerStrParseError);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_VERSION");

    return ALTIBASE_INVALID_VERSION;

    #undef CDBC_FUNC_NAME
}

/**
 * 버전 문자열을 숫자 배열로 파싱한다.
 *
 * @param[in,out] aDestVerArr 파싱된 버전 요소 값을 담을 배열
 * @param[in] aDestVerArrCnt 파싱된 버전 요소 값을 담을 배열의 요소 개수
 * @param[in] aSrcVerStr 파싱할 버전 문자열
 * @param[in] aSrcVerStrLen 파싱할 버전 문자열의 길이
 * @return 파싱된 요소 개수. 파싱에 실패했으면 0보다 작은 수.
 *         CDBC_EXPNO_INVALID_ARGS : 잘못된 인자 사용
 *         CDBC_EXPNO_BUF_NOT_ENOUGH : Dest 인자가 버전 요소를 담기에 충분하지 않음
 *         CDBC_EXPNO_INVALID_VERFORM : 잘못된 버전 문자열 형식
 */
CDBC_INTERNAL
acp_sint32_t altibase_parse_verstr (acp_sint32_t *aDestVerArr, acp_sint32_t aDestVerArrCnt, const acp_char_t *aSrcVerStr, acp_sint32_t aSrcVerStrLen)
{
    #define CDBC_FUNC_NAME "altibase_parse_verstr"

    acp_sint32_t    sVerIdx;
    acp_rc_t        sRC;
    acp_sint32_t    i;

    CDBCLOG_IN();

    CDBC_TEST_RAISE((aDestVerArr == NULL) || (aSrcVerStr == NULL), InvalidNullPtr);
    CDBC_TEST_RAISE((aDestVerArrCnt < 1) || (aSrcVerStrLen < 1), InvalidParamRange);
    CDBCLOG_PRINT_VAL("%s", aSrcVerStr);
    CDBCLOG_PRINT_VAL("%d", aSrcVerStrLen);

    sVerIdx = 0;
    aDestVerArr[0] = 0;
    for (i = 0; i < aSrcVerStrLen; i++)
    {
        if (aSrcVerStr[i] == ALTIBASE_VERSTR_DELIM_CHAR)
        {
            sVerIdx++;
            CDBC_TEST_RAISE(sVerIdx >= aDestVerArrCnt, ArrayNotEnough);

            aDestVerArr[sVerIdx] = 0;
        }
        else
        {
            CDBC_TEST_RAISE(acpCharIsDigit(aSrcVerStr[i]) == ACP_FALSE,
                            InvalidNumericLiteral);
            aDestVerArr[sVerIdx] *= 10;
            aDestVerArr[sVerIdx] += (aSrcVerStr[i] - '0');
        }
    }

    CDBCLOG_OUT_VAL("%d", (sVerIdx + 1));

    return (sVerIdx + 1);

    /* 에러 메시지는 상위 인터페이스에서 처리 */

    CDBC_EXCEPTION(InvalidNullPtr);
    {
        sRC = CDBC_EXPNO_INVALID_ARGS;
    }
    CDBC_EXCEPTION(InvalidParamRange);
    {
        sRC = CDBC_EXPNO_INVALID_ARGS;
    }
    CDBC_EXCEPTION(ArrayNotEnough);
    {
        sRC = CDBC_EXPNO_BUF_NOT_ENOUGH;
    }
    CDBC_EXCEPTION(InvalidNumericLiteral);
    {
        sRC = CDBC_EXPNO_INVALID_VERFORM;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%d", sRC);

    return sRC;

    #undef CDBC_FUNC_NAME
}

