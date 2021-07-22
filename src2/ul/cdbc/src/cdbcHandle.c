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


CDBC_INTERNAL SQLHSTMT altibase_hstmt_init (cdbcABConn *aABConn);
CDBC_INTERNAL ALTIBASE_RC altibase_ensure_hstmt (cdbcABConn *aABConn);



/**
 * 핸들이 유효한지 확인한다.
 *
 * @param[in] aABHandle 유효한지 확인할 핸들.
 *                      ALTIBASE, ALTIBASE_STMT, ALTIBASE_RES 중 하나.
 * @return 핸들이 유효하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC cdbcCheckHandle (cdbcABHandleType aType, void *aABHandle)
{
    #define CDBC_FUNC_NAME "cdbcCheck"

    cdbcABHandle  *sABHandle = (cdbcABHandle *) aABHandle;
    cdbcABHandle  *sABParent;
    cdbcABConn    *sABConn;
    cdbcABStmt    *sABStmt;
    cdbcABRes     *sABRes;

    CDBCLOG_IN();

    CDBCLOG_PRINT_VAL("%p", sABHandle);
    CDBC_TEST_RAISE(sABHandle == NULL, InvalidHandle);
    sABParent = sABHandle->mParentHandle;

    CDBCLOG_PRINT_VAL("%d", aType);
    CDBCLOG_PRINT_VAL("%d", sABHandle->mType);
    switch (aType)
    {
        case ALTIBASE_HANDLE_CONN:
            CDBC_TEST_RAISE(sABHandle->mStSize != ACI_SIZEOF(cdbcABConn) ||
                            sABHandle->mType != aType,
                            InvalidHandle);
            sABConn = (cdbcABConn *) sABHandle;
            CDBC_TEST_RAISE((sABParent != NULL) ||
                            (sABConn->mHenv == NULL) || (sABConn->mHdbc == NULL),
                            InvalidDBC);
            break;

        case ALTIBASE_HANDLE_STMT:
            CDBC_TEST_RAISE(sABHandle->mStSize != ACI_SIZEOF(cdbcABStmt) ||
                            sABHandle->mType != aType,
                            InvalidHandle);
            sABStmt = (cdbcABStmt *) sABHandle;
            CDBC_TEST_RAISE(sABParent == NULL ||
                            cdbcCheckHandle(ALTIBASE_HANDLE_CONN, sABParent) != ALTIBASE_SUCCESS ||
                            sABStmt->mHstmt == NULL,
                            InvalidSTMT);
            break;

        case ALTIBASE_HANDLE_RES:
            CDBC_TEST_RAISE(sABHandle->mStSize != ACI_SIZEOF(cdbcABRes) ||
                            sABHandle->mType != aType,
                            InvalidHandle);
            sABRes = (cdbcABRes *) sABHandle;
            CDBC_TEST_RAISE(sABParent == NULL ||
                            cdbcCheckHandle(sABParent->mType, sABParent) != ALTIBASE_SUCCESS ||
                            sABRes->mHstmt == NULL,
                            InvalidRES);
            break;

        case ALTIBASE_HANDLE_RES_META:
            CDBC_TEST_RAISE(sABHandle->mStSize != ACI_SIZEOF(cdbcABRes), InvalidHandle);
            sABRes = (cdbcABRes *) sABHandle;
            CDBC_TEST_RAISE(sABParent == NULL ||
                            cdbcCheckHandle(sABParent->mType, sABParent) != ALTIBASE_SUCCESS,
                            InvalidRES);
            /* ALTIBASE_HANDLE_RES도 메타 정보를 가지고 있으므로 VALID로 간주 */
            if (sABHandle->mType == ALTIBASE_HANDLE_RES)
            {
                CDBC_TEST_RAISE(sABRes->mHstmt == NULL, InvalidRES);
            }
            else if (sABHandle->mType == ALTIBASE_HANDLE_RES_META)
            {
                CDBC_TEST_RAISE(sABRes->mFieldInfos == NULL, InvalidRES);
            }
            else
            {
                CDBC_RAISE(InvalidHandle);
            }
            break;

        default:
            CDBC_RAISE(InvalidHandle);
            break;
    }

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle);
    {
        /* 에러 정보를 설정할 수 없다; */
    }
    CDBC_EXCEPTION(InvalidDBC);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec), ulERR_ABORT_INVALID_HANDLE);
    }
    CDBC_EXCEPTION(InvalidSTMT);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec), ulERR_ABORT_INVALID_HANDLE);
    }
    CDBC_EXCEPTION(InvalidRES);
    {
        altibase_set_errinfo(sABRes->mDiagRec, ulERR_ABORT_INVALID_HANDLE);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_INVALID_HANDLE");

    return ALTIBASE_INVALID_HANDLE;

    #undef CDBC_FUNC_NAME
}

/**
 * 초기화된 연결 핸들을 얻는다.
 *
 * @return 초기화된 연결 핸들. 실패하면 NULL
 */
CDBC_EXPORT
ALTIBASE altibase_init (void)
{
    #define CDBC_FUNC_NAME "altibase_init"

    cdbcABConn *sABConn = NULL;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBCLOG_CALL("acpMemCalloc");
    sRC = acpMemCalloc((void **)&sABConn, 1, ACI_SIZEOF(cdbcABConn));
    CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
    CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);

    CDBCLOG_CALL("SQLAllocHandle");
    sRC = SQLAllocHandle(SQL_HANDLE_ENV, NULL, &(sABConn->mHenv));
    CDBCLOG_BACK_VAL("SQLAllocHandle", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), HAllocError);

    CDBCLOG_CALL("SQLSetEnvAttr");
    sRC = SQLSetEnvAttr(sABConn->mHenv, SQL_ATTR_ODBC_VERSION,
                        (SQLPOINTER) SQL_OV_ODBC3, 0);
    CDBCLOG_BACK_VAL("SQLSetEnvAttr", "%d", sRC);
    CDBC_ASSERT(sRC != SQL_INVALID_HANDLE);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), ENVError);

    CDBCLOG_CALL("SQLAllocHandle");
    sRC = SQLAllocHandle(SQL_HANDLE_DBC, sABConn->mHenv, &(sABConn->mHdbc));
    CDBCLOG_BACK_VAL("SQLAllocHandle", "%d", sRC);
    CDBC_ASSERT(sRC != SQL_INVALID_HANDLE);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), ENVError);

    (sABConn->mBaseHandle).mStSize       = ACI_SIZEOF(cdbcABConn);
    (sABConn->mBaseHandle).mType         = ALTIBASE_HANDLE_CONN;
    (sABConn->mBaseHandle).mParentHandle = NULL;
    sABConn->mState                      = ALTIBASE_STATE_INIT;
    sABConn->mServerVer                  = ALTIBASE_INVALID_VERSION;
    sABConn->mProtoVer                   = ALTIBASE_INVALID_VERSION;

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBCLOG_OUT_VAL("%p", sABConn);

    return (ALTIBASE) sABConn;

    CDBC_EXCEPTION(MAllocError);
    {
        /* 에러 정보를 설정할 수 없다; */
    }
    CDBC_EXCEPTION(HAllocError);
    {
        /* 에러 정보를 설정할 수 없다; */
    }
    CDBC_EXCEPTION(ENVError);
    {
        /* 에러 정보를 설정할 수 없다; */
    }
    CDBC_EXCEPTION_END;

    if (sABConn != NULL)
    {
        altibase_close(sABConn);
    }

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 연결 핸들을 닫는다.
 * 연결 핸들에 관계된 명령문 핸들과 결과 핸들은 모두 무효화된다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_close (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_close"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HCONN_NOT_VALID(sABConn), InvalidHandle);
    CDBC_TEST_RAISE(CONN_IS_RESRETURNED(sABConn), FuncSeqError);

    if (sABConn->mHstmt != NULL)
    {
        sRC = SQLFreeHandle(SQL_HANDLE_STMT, sABConn->mHstmt);
        CDBC_DASSERT( CDBC_CLI_SUCCEEDED(sRC) );
        sABConn->mHstmt = NULL;
    }

    if (sABConn->mHdbc != NULL)
    {
        if (CONN_IS_CONNECTED(sABConn))
        {
            sRC = SQLDisconnect(sABConn->mHdbc);
            CDBC_DASSERT( CDBC_CLI_SUCCEEDED(sRC) );
            CONN_UNSET_CONNECTED(sABConn);
        }
        sRC = SQLFreeHandle(SQL_HANDLE_DBC, sABConn->mHdbc);
        CDBC_DASSERT( CDBC_CLI_SUCCEEDED(sRC) );
        sABConn->mHdbc = NULL;
    }

    if (sABConn->mHenv != NULL)
    {
        sRC = SQLFreeHandle(SQL_HANDLE_ENV, sABConn->mHenv);
        CDBC_DASSERT( CDBC_CLI_SUCCEEDED(sRC) );
        sABConn->mHenv = NULL;
    }

    altibase_invalidate_handle(&sABConn->mBaseHandle);

    acpMemFree(aABConn);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(FuncSeqError)
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 초기화된 명령문 핸들을 얻는다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 초기화된 명령문 핸들. 실패하면 NULL
 */
CDBC_EXPORT
ALTIBASE_STMT altibase_stmt_init (ALTIBASE aABConn)
{
    #define CDBC_FUNC_NAME "altibase_stmt_init"

    cdbcABConn *sABConn = (cdbcABConn *) aABConn;
    cdbcABStmt *sABStmt = NULL;
    SQLHSTMT    sHstmt;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBC_TEST(HCONN_NOT_VALID(sABConn));

    altibase_init_errinfo(&(sABConn->mDiagRec));

    CDBCLOG_CALL("acpMemCalloc");
    sRC = acpMemCalloc((void **)&sABStmt, 1, ACI_SIZEOF(cdbcABStmt));
    CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
    CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);

    sHstmt = altibase_hstmt_init(sABConn);
    CDBC_TEST(sHstmt == NULL);

    (sABStmt->mBaseHandle).mStSize       = ACI_SIZEOF(cdbcABStmt);
    (sABStmt->mBaseHandle).mType         = ALTIBASE_HANDLE_STMT;
    (sABStmt->mBaseHandle).mParentHandle = (cdbcABHandle *)sABConn;
    sABStmt->mHstmt                      = sHstmt;
    sABStmt->mState                      = ALTIBASE_STATE_INIT;
    sABStmt->mArrayBindSize              = 1;
    sABStmt->mArrayFetchSize             = 1;
    sABStmt->mParamCount                 = ALTIBASE_INVALID_PARAMCOUNT;

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBCLOG_OUT_VAL("%p", sABStmt);

    return (ALTIBASE_STMT) sABStmt;

    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(&(sABConn->mDiagRec),
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
    }
    CDBC_EXCEPTION_END;

    if (sABStmt != NULL)
    {
        altibase_stmt_close(sABStmt);
    }

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 명령문 핸들을 닫는다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_close (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_close"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    SAFE_FREE_AND_CLEAN(sABStmt->mQstr);
    sABStmt->mQstrMaxLen = 0;

    altibase_stmt_parambind_free(sABStmt);

    if (sABStmt->mRes != NULL)
    {
        sRC = altibase_free_result(sABStmt->mRes);
        CDBC_DASSERT( ALTIBASE_SUCCEEDED(sRC) );
        sABStmt->mRes = NULL;
    }

    if (sABStmt->mHstmt != NULL)
    {
        sRC = SQLFreeHandle(SQL_HANDLE_STMT, sABStmt->mHstmt);
        CDBC_DASSERT( CDBC_CLI_SUCCEEDED(sRC) );
        sABStmt->mHstmt = NULL;
    }

    altibase_invalidate_handle(&sABStmt->mBaseHandle);

    acpMemFree(sABStmt);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과 핸들을 생성한다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 성공했으면 결과 핸들, 그렇지 않으면 NULL
 */
CDBC_INTERNAL
cdbcABRes * altibase_result_init (cdbcABConn *aABConn)
{
    #define CDBC_FUNC_NAME "altibase_result_init"

    cdbcABRes  *sABRes = NULL;
    acp_rc_t    sRC;

    CDBCLOG_IN();

    CDBC_DASSERT(HCONN_IS_VALID(aABConn));
    CDBC_TEST_RAISE(CONN_NOT_EXECUTED(aABConn) || CONN_IS_RESRETURNED(aABConn),
                    FuncSeqError);

    CDBCLOG_CALL("acpMemCalloc");
    sRC = acpMemCalloc((void **)&sABRes, 1, ACI_SIZEOF(cdbcABRes));
    CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
    CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);
    CDBCLOG_PRINT_VAL("%p", sABRes);
    CDBCLOG_PRINT_VAL("%p", aABConn->mHstmt);

    (sABRes->mBaseHandle).mStSize       = ACI_SIZEOF(cdbcABRes);
    (sABRes->mBaseHandle).mType         = ALTIBASE_HANDLE_RES;
    (sABRes->mBaseHandle).mParentHandle = (cdbcABHandle *)aABConn;
    sABRes->mDiagRec                    = &(aABConn->mDiagRec);
    sABRes->mHstmt                      = aABConn->mHstmt;
    sABRes->mState                      = &(aABConn->mState);
    sABRes->mFieldCount                 = ALTIBASE_INVALID_FIELDCOUNT;
    sABRes->mFieldInfoEx                = ACP_FALSE;
    sABRes->mArrayBindSize              = 1;
    sABRes->mArrayFetchSize             = 1;

    CDBCLOG_OUT_VAL("%p", sABRes);

    return sABRes;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(aABConn->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(&(aABConn->mDiagRec),
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
    }
    CDBC_EXCEPTION_END;

    SAFE_FREE(sABRes);

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과 핸들을 생성한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 성공했으면 결과 핸들, 그렇지 않으면 NULL
 */
CDBC_INTERNAL
cdbcABRes * altibase_stmt_result_init (cdbcABStmt *aABStmt, cdbcABHandleType aType)
{
    #define CDBC_FUNC_NAME "altibase_stmt_result_init"

    cdbcABRes      *sABRes = NULL;
    acp_rc_t        sRC;

    CDBCLOG_IN();

    CDBC_DASSERT(HSTMT_IS_VALID(aABStmt));
    CDBCLOG_PRINT_VAL("%d", aABStmt->mState);
    CDBCLOG_PRINT_VAL("%d", STMT_NOT_PREPARED(aABStmt));
    CDBCLOG_PRINT_VAL("%d", STMT_IS_RESRETURNED(aABStmt));
    CDBC_TEST_RAISE(STMT_NOT_PREPARED(aABStmt), FuncSeqError);
    CDBC_TEST_RAISE((aType != ALTIBASE_HANDLE_RES) &&
                    (aType != ALTIBASE_HANDLE_RES_META), InvalidParamRange);

    CDBCLOG_CALL("acpMemCalloc");
    sRC = acpMemCalloc((void **)&sABRes, 1, ACI_SIZEOF(cdbcABRes));
    CDBCLOG_BACK_VAL("acpMemCalloc", "%d", sRC);
    CDBC_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAllocError);
    CDBCLOG_PRINT_VAL("%p", sABRes);

    CDBCLOG_PRINT_VAL("%d", aType);
    (sABRes->mBaseHandle).mStSize       = ACI_SIZEOF(cdbcABRes);
    (sABRes->mBaseHandle).mType         = aType;
    (sABRes->mBaseHandle).mParentHandle = (cdbcABHandle *)aABStmt;
    sABRes->mDiagRec                    = &(aABStmt->mDiagRec);
    sABRes->mFieldCount                 = ALTIBASE_INVALID_FIELDCOUNT;
    sABRes->mFieldInfoEx                = ACP_FALSE;

    if (aType == ALTIBASE_HANDLE_RES)
    {
        CDBCLOG_PRINT_VAL("%p", aABStmt->mHstmt);

        /* META는 생성될 때 모든 정보를 미리 받아두고 이를 사용하므로
           CLI 함수 호출을 위한 변수 설정은 필요 없다. */
        sABRes->mHstmt          = aABStmt->mHstmt;
        sABRes->mState          = &(aABStmt->mState);
        sABRes->mArrayBindSize  = 1;
        sABRes->mArrayFetchSize = 1;
    }

    CDBCLOG_OUT_VAL("%p", sABRes);

    return sABRes;

    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(aABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    CDBC_EXCEPTION(InvalidParamRange);
    {
        altibase_set_errinfo(&(aABStmt->mDiagRec),
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
    }
    CDBC_EXCEPTION(MAllocError);
    {
        altibase_set_errinfo(&(aABStmt->mDiagRec),
                             ulERR_FATAL_MEMORY_ALLOC_ERROR,
                             CDBC_FUNC_NAME);
    }
    CDBC_EXCEPTION_END;

    SAFE_FREE(sABRes);

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋 핸들을 해제한다.
 *
 * @param[in] aABRes 결과셋 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_free_result (ALTIBASE_RES aABRes)
{
    #define CDBC_FUNC_NAME "altibase_free_result"

    cdbcABRes     *sABRes = (cdbcABRes *) aABRes;
    ALTIBASE_RC    sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HROM_NOT_VALID(sABRes), InvalidHandle);

    if ((sABRes->mBaseHandle).mType == ALTIBASE_HANDLE_RES)
    {
        /* 필드 정보 */
        SAFE_FREE_AND_CLEAN(sABRes->mFieldInfos);

        /* free binding info */
        altibase_result_bind_free(sABRes);
        SAFE_FREE_AND_CLEAN(sABRes->mArrayStatusParam);
        sABRes->mArrayBindSize = 1;
        SAFE_FREE_AND_CLEAN(sABRes->mArrayStatusResult);
        sABRes->mArrayFetchSize = 1;

        /* free result */
        if (RES_IS_STORED(sABRes))
        {
            altibase_clean_stored_result(sABRes);

            SAFE_FREE_AND_CLEAN(sABRes->mFetchedColOffset);
        }
        else
        {
            /* use result일 때만 alloc한 공간. */
            SAFE_FREE_AND_CLEAN(sABRes->mFetchedRow); /* free mFetchedRow, mLengths */

            altibase_clean_buffer(&(sABRes->mDatBuffer));

            CDBC_DASSERT(sABRes->mFetchedColOffset == NULL);
            CDBC_DASSERT(sABRes->mFetchedColOffsetMaxCount == 0);
        }

        /* 상태 변경 */
        RES_UNSET_EXECUTED(sABRes);
        RES_UNSET_FETCHED(sABRes);
        RES_UNSET_RESRETURNED(sABRes);
        RES_UNSET_STORED(sABRes);
    }

    altibase_invalidate_handle(&sABRes->mBaseHandle);

    acpMemFree(sABRes);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * 결과셋 핸들을 해제한다.
 *
 * @param[in] aABStmt 명령문 핸들
 * @return 성공하면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_EXPORT
ALTIBASE_RC altibase_stmt_free_result (ALTIBASE_STMT aABStmt)
{
    #define CDBC_FUNC_NAME "altibase_stmt_free_result"

    cdbcABStmt *sABStmt = (cdbcABStmt *) aABStmt;
    cdbcABRes  *sABRes;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_TEST_RAISE(HSTMT_NOT_VALID(sABStmt), InvalidHandle);

    altibase_init_errinfo(&(sABStmt->mDiagRec));

    CDBC_TEST_RAISE(HRES_NOT_VALID(sABStmt->mRes), FuncSeqError);
    sABRes = sABStmt->mRes;

    if (sABRes->mHstmt != NULL)
    {
        CDBCLOG_CALL("SQLFreeStmt : SQL_CLOSE");
        sRC = SQLFreeStmt(sABRes->mHstmt, SQL_CLOSE);
        CDBCLOG_BACK_VAL("SQLFreeStmt", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
    }

    /* 필드 정보, 바인드 정보는 유지 */

    /* free result */
    if (RES_IS_STORED(sABRes))
    {
        altibase_clean_stored_result(sABRes);
        SAFE_FREE_AND_CLEAN(sABRes->mFetchedColOffset);
        sABRes->mFetchedColOffsetMaxCount = 0;
    }
    else
    {
        /* use result일 때만 alloc한 공간. */
        SAFE_FREE_AND_CLEAN(sABRes->mFetchedRow); /* free mFetchedRow, mLengths */

        altibase_clean_buffer(&(sABRes->mDatBuffer));

        CDBC_DASSERT(sABRes->mFetchedColOffset == NULL);
        CDBC_DASSERT(sABRes->mFetchedColOffsetMaxCount == 0);
    }
    CDBC_DASSERT(sABRes->mRowMap == NULL);
    CDBC_DASSERT(sABRes->mFetchedRow == NULL);
    CDBC_DASSERT(sABRes->mDatBuffer.mHead == NULL);
    CDBC_DASSERT(sABRes->mDatBuffer.mTail == NULL);

    /* 상태 변경 */
    RES_UNSET_EXECUTED(sABRes);
    RES_UNSET_FETCHED(sABRes);
    RES_UNSET_RESRETURNED(sABRes);
    RES_UNSET_STORED(sABRes);

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(InvalidHandle)
    {
        sRC = ALTIBASE_INVALID_HANDLE;
    }
    CDBC_EXCEPTION(FuncSeqError);
    {
        altibase_set_errinfo(&(sABStmt->mDiagRec),
                             ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION(STMTError);
    {   
        altibase_set_errinfo_by_res(sABRes, sRC);
        sRC = ALTIBASE_ERROR;
    }
    CDBC_EXCEPTION_END;

    /* mRes는 altibase_stmt_close() 할 때 닫는다. */

    CDBCLOG_OUT_VAL("%s", altibase_rc_string(sRC));

    return sRC;

    #undef CDBC_FUNC_NAME
}

/**
 * stmt 핸들을 생성한다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 성공했으면 stmt 핸들, 그렇지 않으면 NULL
 */
CDBC_INTERNAL
SQLHSTMT altibase_hstmt_init (cdbcABConn *aABConn)
{
    #define CDBC_FUNC_NAME "altibase_hstmt_init"

    SQLHSTMT sHstmt;
    acp_rc_t sRC;

    CDBCLOG_IN();

    CDBC_DASSERT(HCONN_IS_VALID(aABConn));

    CDBCLOG_CALL("SQLAllocHandle");
    sRC = SQLAllocHandle(SQL_HANDLE_STMT, aABConn->mHdbc, &sHstmt);
    CDBCLOG_BACK_VAL("SQLAllocHandle", "%d", sRC);
    CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), DBCError);

    CDBCLOG_OUT_VAL("%p", sHstmt);

    return sHstmt;

    CDBC_EXCEPTION(DBCError);
    {
        altibase_set_errinfo_by_conndbc(aABConn, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "{null}");

    return NULL;

    #undef CDBC_FUNC_NAME
}

/**
 * stmt 핸들이 없으면 생성한다.
 * 이미 stmt 핸들이 있으면, 기존 상태를 유지한다.
 *
 * @param[in] aABConn 연결 핸들
 * @return 성공했으면 ALTIBASE_SUCCESS, 그렇지 않으면 ALTIBASE_ERROR
 */
CDBC_INTERNAL
ALTIBASE_RC altibase_ensure_hstmt (cdbcABConn *aABConn)
{
    #define CDBC_FUNC_NAME "altibase_ensure_hstmt"

    SQLHSTMT    sHstmt;
    ALTIBASE_RC sRC;

    CDBCLOG_IN();

    CDBC_DASSERT(HCONN_IS_VALID(aABConn));

    if (aABConn->mHstmt == NULL)
    {
        sHstmt = altibase_hstmt_init(aABConn);
        CDBC_TEST(sHstmt == NULL);

        aABConn->mHstmt = sHstmt;
    }
    else
    {
        CDBCLOG_CALL("SQLFreeStmt : SQL_CLOSE");
        sRC = SQLFreeStmt(aABConn->mHstmt, SQL_CLOSE);
        CDBCLOG_BACK_VAL("SQLFreeStmt", "%d", sRC);
        CDBC_TEST_RAISE(CDBC_CLI_NOT_SUCCEEDED(sRC), STMTError);
    }

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_SUCCESS");

    return ALTIBASE_SUCCESS;

    CDBC_EXCEPTION(STMTError);
    {   
        altibase_set_errinfo_by_connstmt(aABConn, sRC);
    }
    CDBC_EXCEPTION_END;

    CDBCLOG_OUT_VAL("%s", "ALTIBASE_ERROR");

    return ALTIBASE_ERROR;

    #undef CDBC_FUNC_NAME
}

