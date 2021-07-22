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
#include <stdarg.h>

/* BUGBUG (2014-11-28) handle이 유효하지 않을때도 에러 정보를 주기위한 것으로
 * 에러 메시지가 바뀌면 여기도 바꿔주는게 좋다. */
#define INVALID_HANDLE_ERRCODE  ACI_E_ERROR_CODE(ulERR_ABORT_INVALID_HANDLE)
#define INVALID_HANDLE_ERRMSG   "Invalid Handle."
#define INVALID_HANDLE_SQLSTATE "IH"



/** is same as gUlnErrorFactory */
static aci_client_error_factory_t gCdbcErrorFactory[] =
{
#include "../../uln/E_UL_US7ASCII.c"
};

static aci_client_error_factory_t *gCdbcClientFactory[] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gCdbcErrorFactory,
    NULL,
    NULL,
    NULL,
    NULL /* gUtErrorFactory, */
};

/**
 * 에러 정보를 초기화한다.
 *
 * @param[in,out] aDiagRec 초기화할 에러 정보 구조체
 */
CDBC_INTERNAL
void altibase_init_errinfo (cdbcABDiagRec *aDiagRec)
{
    #define CDBC_FUNC_NAME "altibase_init_errinfo"

    CDBCLOG_IN();

    CDBC_DASSERT(aDiagRec != NULL);

    if ((aDiagRec->mErrorCode != 0)
     || (aDiagRec->mErrorMessage[0] != '\0')
     || (0 != acpCStrCmp((acp_char_t *)aDiagRec->mErrorState, "00000",
                         ALTIBASE_MAX_SQLSTATE_LEN)))
    {
        acpMemSet(aDiagRec, 0, ACI_SIZEOF(cdbcABDiagRec));
        (void) acpCStrCpy((acp_char_t *)aDiagRec->mErrorState,
                          ALTIBASE_MAX_SQLSTATE_LEN + 1,
                          "00000", ALTIBASE_MAX_SQLSTATE_LEN);
    }

    CDBCLOG_OUT();

    #undef CDBC_FUNC_NAME
}

/**
 * 에러 정보를 설정한다.
 *
 * @param[in,out] aDiagRec 초기화할 에러 정보 구조체
 * @param[in] aErrorCode 설정할 에러 정보의 에러 코드
 * @param[in] aArgs 에러 메시지를 완성하는데 사용되는 값
 */
CDBC_INTERNAL
void altibase_set_errinfo_v (cdbcABDiagRec *aDiagRec, acp_uint32_t aErrorCode, va_list aArgs)
{
    #define CDBC_FUNC_NAME "altibase_set_errinfo_v"

    acp_uint32_t                sSection;
    aci_client_error_factory_t *sCurFactory;

    CDBCLOG_IN();

    CDBC_DASSERT(aDiagRec != NULL);

    sSection = (aErrorCode & ACI_E_MODULE_MASK) >> 28;

    sCurFactory = gCdbcClientFactory[sSection];
    CDBC_DASSERT(sCurFactory != NULL);

    CDBCLOG_CALL("aciSetClientErrorCode");
    aciSetClientErrorCode(aDiagRec, sCurFactory, aErrorCode, aArgs);
    CDBCLOG_BACK("aciSetClientErrorCode");

    /* convert real error code */
    aDiagRec->mErrorCode = ACI_E_ERROR_CODE(aDiagRec->mErrorCode);

    CDBCLOG_OUT();

    #undef CDBC_FUNC_NAME
}

/**
 * 에러 정보를 설정한다.
 *
 * @param[in,out] aDiagRec 초기화할 에러 정보 구조체
 * @param[in] aErrorCode 설정할 에러 정보의 에러 코드
 * @param[in] ... 에러 메시지를 완성하는데 사용되는 값
 */
CDBC_INTERNAL
void altibase_set_errinfo (cdbcABDiagRec *aDiagRec, acp_uint32_t aErrorCode, ...)
{
    #define CDBC_FUNC_NAME "altibase_set_errinfo"

    va_list          sArgs;

    CDBCLOG_IN();

    CDBC_DASSERT(aDiagRec != NULL);

    va_start(sArgs, aErrorCode);
    altibase_set_errinfo_v(aDiagRec, aErrorCode, sArgs);
    va_end(sArgs);

    CDBCLOG_OUT();

    #undef CDBC_FUNC_NAME
}

/**
 * 내부 에러를 가져와 에러 정보를 설정한다.
 *
 * @param[in,out] aDiagRec 에러 정보를 담을 구조체
 * @param[in] aHandleType 내부 핸들 유형
 * @param[in] aHandle 내부 핸들
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
void altibase_set_errinfo_by_sqlhandle (cdbcABDiagRec *aDiagRec,
                                        SQLSMALLINT    aHandleType,
                                        SQLHANDLE      aHandle,
                                        acp_rc_t       aRC)
{
    #define CDBC_FUNC_NAME "altibase_set_errinfo_by_sqlhandle"

    acp_rc_t sRC;

    CDBCLOG_IN();

    CDBC_DASSERT(aDiagRec != NULL);

    CDBC_TEST_RAISE(aRC == ALTIBASE_INVALID_HANDLE, InvalidHandle);

    CDBC_DASSERT(aHandle != NULL);

    CDBCLOG_CALL("SQLGetDiagRec");
    sRC = SQLGetDiagRec(aHandleType, aHandle, 1,
                        (SQLCHAR *) aDiagRec->mErrorState,
                        (SQLINTEGER *) &(aDiagRec->mErrorCode),
                        (SQLCHAR *) aDiagRec->mErrorMessage,
                        ACI_SIZEOF(aDiagRec->mErrorMessage),
                        NULL);
    CDBCLOG_BACK_VAL("SQLGetDiagRec", "%d", sRC);
    CDBCLOG_PRINT_VAL("%s", aDiagRec->mErrorState);
    CDBCLOG_PRINT_VAL("0x%05X", (acp_uint32_t)(aDiagRec->mErrorCode));
    CDBCLOG_PRINT_VAL("%s", aDiagRec->mErrorMessage);
    CDBC_TEST_RAISE(sRC == SQL_INVALID_HANDLE, InvalidHandle);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), FailedToGetDiagRec);

    CDBC_EXCEPTION(InvalidHandle);
    {
        altibase_set_errinfo(aDiagRec, ulERR_ABORT_INVALID_HANDLE);
    }
    CDBC_EXCEPTION(FailedToGetDiagRec);
    {
        altibase_set_errinfo(aDiagRec, ulERR_ABORT_FAIL_OBTAIN_ERROR_INFORMATION);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT();

    #undef CDBC_FUNC_NAME
}

/**
 * 에러 번호를 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 에러 번호. 에러가 없거나 실패하면 0.
 */
CDBC_EXPORT
acp_uint32_t altibase_errno (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_errno"

    cdbcABConn  *sABConn = (cdbcABConn *) aABConn;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    CDBCLOG_OUT_VAL("0x%05X", (acp_uint32_t) (sABConn->mDiagRec).mErrorCode);

    return (sABConn->mDiagRec).mErrorCode;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("0x%05X", (acp_uint32_t) INVALID_HANDLE_ERRCODE);

    return INVALID_HANDLE_ERRCODE;

    #undef CDBC_FUNC_NAME
}

/**
 * 에러 번호를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 에러 번호. 에러가 없거나 실패하면 0.
 */
CDBC_EXPORT
acp_uint32_t altibase_stmt_errno (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_errno"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    CDBCLOG_OUT_VAL("0x%05X", (acp_uint32_t) (sABStmt->mDiagRec).mErrorCode);

    return (sABStmt->mDiagRec).mErrorCode;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("0x%05X", (acp_uint32_t) INVALID_HANDLE_ERRCODE);

    return INVALID_HANDLE_ERRCODE;

    #undef CDBC_FUNC_NAME
}

/**
 * 에러 번호를 얻는다.
 *
 * @param[in] aABRes 결과 핸들
 * @return 에러 번호. 에러가 없거나 실패하면 0.
 */
CDBC_INTERNAL
acp_uint32_t altibase_result_errno (ALTIBASE_RES aABRes)
{
    #define CDBC_FUNC_NAME "altibase_result_errno"

    cdbcABRes *sABRes = (cdbcABRes *) aABRes;

    CDBCLOG_IN();

    CDBC_TEST(HROM_NOT_VALID(sABRes));

    CDBC_DASSERT(sABRes->mDiagRec != NULL);

    CDBCLOG_OUT_VAL("0x%05X", (acp_uint32_t) sABRes->mDiagRec->mErrorCode);

    return (sABRes->mDiagRec)->mErrorCode;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("0x%05X", (acp_uint32_t) INVALID_HANDLE_ERRCODE);

    return INVALID_HANDLE_ERRCODE;

    #undef CDBC_FUNC_NAME
}

/**
 * 에러 메시지를 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 에러 메시지. 에러가 없으면 빈 문자열, 실패하면 NULL
 */
CDBC_EXPORT
const acp_char_t * altibase_error (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_error"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    CDBCLOG_OUT_VAL("%s", (sABConn->mDiagRec).mErrorMessage);

    return (const acp_char_t *) (sABConn->mDiagRec).mErrorMessage;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", INVALID_HANDLE_ERRMSG);

    return INVALID_HANDLE_ERRMSG;

    #undef CDBC_FUNC_NAME
}

/**
 * 에러 메시지를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 에러 메시지. 에러가 없으면 빈 문자열, 실패하면 NULL
 */
CDBC_EXPORT
const acp_char_t * altibase_stmt_error (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_error"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    CDBCLOG_OUT_VAL("%s", (sABStmt->mDiagRec).mErrorMessage);

    return (const acp_char_t *)((sABStmt->mDiagRec).mErrorMessage);

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", INVALID_HANDLE_ERRMSG);

    return INVALID_HANDLE_ERRMSG;

    #undef CDBC_FUNC_NAME
}

/**
 * SQLSTATE를 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return SQLSTATE 값. 에러가 없으면 "00000", 실패하면 NULL.
 */
CDBC_EXPORT
const acp_char_t * altibase_sqlstate (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_sqlstate"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    CDBCLOG_OUT_VAL("%s", (sABConn->mDiagRec).mErrorState);

    return (const acp_char_t *) (sABConn->mDiagRec).mErrorState;

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", INVALID_HANDLE_SQLSTATE);

    return INVALID_HANDLE_SQLSTATE;

    #undef CDBC_FUNC_NAME
}

/**
 * SQLSTATE를 얻는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return SQLSTATE 값. 에러가 없으면 "00000", 실패하면 NULL.
 */
CDBC_EXPORT
const acp_char_t * altibase_stmt_sqlstate (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_sqlstate"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;

    CDBCLOG_IN();

    CDBC_TEST(HSTMT_NOT_VALID(sABStmt));

    CDBCLOG_OUT_VAL("%s", (sABStmt->mDiagRec).mErrorState);

    return (const acp_char_t *)((sABStmt->mDiagRec).mErrorState);

    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", INVALID_HANDLE_SQLSTATE);

    return INVALID_HANDLE_SQLSTATE;

    #undef CDBC_FUNC_NAME
}

