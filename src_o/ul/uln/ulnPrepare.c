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

#include <ulnPrivate.h>
#include <uln.h>
#include <ulnPrepare.h>
#include <ulnCache.h>

#define ULN_PREP_CHKERR_PREFIX_CSTR         "SELECT _PROWID,"
#define ULN_PREP_CHKERR_PREFIX_CLEN         15
#define ULN_PREP_KEYSET_PREFIX_CSTR         "SELECT _PROWID "
#define ULN_PREP_KEYSET_PREFIX_CLEN         15
#define ULN_PREP_ROWSET_PREFIX_CSTR         ULN_PREP_CHKERR_PREFIX_CSTR
#define ULN_PREP_ROWSET_PREFIX_CLEN         ULN_PREP_CHKERR_PREFIX_CLEN
#define ULN_PREP_ROWSET_POSTFIX_B_CSTR      " WHERE _PROWID IN (?"
#define ULN_PREP_ROWSET_POSTFIX_B_CLEN      20
#define ULN_PREP_ROWSET_POSTFIX_P_CSTR      ", ?"
#define ULN_PREP_ROWSET_POSTFIX_P_CLEN      3
#define ULN_PREP_ROWSET_POSTFIX_E_CSTR      ")"
#define ULN_PREP_ROWSET_POSTFIX_E_CLEN      1

#define ULN_PREP_UPDATE_PREFIX_CSTR         "UPDATE "
#define ULN_PREP_UPDATE_PREFIX_CLEN         7
#define ULN_PREP_UPDATE_SET_B_CSTR          " SET \""
#define ULN_PREP_UPDATE_SET_B_CLEN          6
#define ULN_PREP_UPDATE_SET_E_CSTR          "\" = ?"
#define ULN_PREP_UPDATE_SET_E_CLEN          5
#define ULN_PREP_UPDATE_SET_C_CSTR          ", \""
#define ULN_PREP_UPDATE_SET_C_CLEN          3
#define ULN_PREP_UPDATE_SET_MAX_CLEN        (ACP_MAX(ULN_PREP_UPDATE_SET_B_CLEN, ULN_PREP_UPDATE_SET_C_CLEN) + ULN_MAX_NAME_LEN + ULN_PREP_UPDATE_SET_E_CLEN)
#define ULN_PREP_UPDATE_POSTFIX_CSTR        " WHERE _PROWID = ?"
#define ULN_PREP_UPDATE_POSTFIX_CLEN        18

#define ULN_PREP_DELETE_FORM_CSTR           "DELETE FROM %s WHERE _PROWID = ?"
#define ULN_PREP_DELETE_FORM_CLEN           30

#define ULN_PREP_INSERT_PREFIX_CSTR         "INSERT INTO "
#define ULN_PREP_INSERT_PREFIX_CLEN         12
#define ULN_PREP_INSERT_COL_B_CSTR          " (\""
#define ULN_PREP_INSERT_COL_B_CLEN          3
#define ULN_PREP_INSERT_COL_E_CSTR          "\")"
#define ULN_PREP_INSERT_COL_E_CLEN          2
#define ULN_PREP_INSERT_COL_C_CSTR          "\",\""
#define ULN_PREP_INSERT_COL_C_CLEN          3
#define ULN_PREP_INSERT_COL_MAX_CLEN        (ACP_MAX(ULN_PREP_INSERT_COL_B_CSTR, ULN_PREP_INSERT_COL_C_CLEN) + ULN_MAX_NAME_LEN + ULN_PREP_INSERT_PARAM_E_CLEN)
#define ULN_PREP_INSERT_PARAM_B_CSTR        " VALUES (?"
#define ULN_PREP_INSERT_PARAM_B_CLEN        10
#define ULN_PREP_INSERT_PARAM_E_CSTR        ")"
#define ULN_PREP_INSERT_PARAM_E_CLEN        1
#define ULN_PREP_INSERT_PARAM_C_CSTR        ", ?"
#define ULN_PREP_INSERT_PARAM_C_CLEN        3



static ulnPrepareReplaceFunc * const ulnPrepareReplaceMap[ULN_NCHAR_LITERAL_MAX] =
{
    ulnPrepDoText,
    ulnPrepDoTextNchar
};

/*
 * ULN_SFID_43
 * SQLPrepare(), STMT, S1
 *      S2 [s] and [nr]
 *      S3 [s] and [r]
 *      S11 [x]
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [r]  Results. The statement will or did create a (possibly empty) result set.
 *      [nr] No results. The statement will not or did not create a result set.
 */
ACI_RC ulnSFID_43(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (aFnContext->mSqlReturn == SQL_SUCCESS ||
            aFnContext->mSqlReturn == SQL_SUCCESS_WITH_INFO)
        {
            if (ulnStateCheckR(aFnContext) == ACP_TRUE)
            {
                /* [r] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
            }
            else
            {
                /* [nr] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
            }
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_44
 * SQLPrepare(), STMT, S2-S3
 *      -- [s] or ([e] and [1])
 *      S1 [e] and [2]
 *      S11 [x]
 *  where
 *      [1]  The preparation fails for a reason other than validating the statement
 *           (the SQLSTATE was HY009 [Invalid argument value] or
 *           HY090 [Invalid string or buffer length]).
 *      [2]  The preparation fails while validating the statement
 *           (the SQLSTATE was not HY009 [Invalid argument value] or
 *           HY090 [Invalid string or buffer length]).
 *
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [e]  Error. The function returned SQL_ERROR.
 */
ACI_RC ulnSFID_44(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
            ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO)
        {
            if (ulnStmtGetResultSetCount(aFnContext->mHandle.mStmt) == 0)
            {
                /* [nr] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
            }
            else
            {
                /* [r] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
            }
        }
        else
        {
            /*
             * BUGBUG: The preparation fails for a reason other than validating the statement.
             *         State : stays as it was.
             *
             *         The preparation fails while validating the statement.
             *         State : S2orS3 ---> S1
             */
        }
    }

    return ACI_SUCCESS;
}
/*
 * ULN_SFID_45
 * SQLPrepare(), STMT, S4
 *      S1 [e] and [3]
 *      S2 [s], [nr], and [3]
 *      S3 [s], [r], and [3]
 *      S11 [x] and [3]
 *      24000[4]
 *  where
 *      [3] The current result is the last or only result, or there are no current results.
 *      [4] The current result is not the last result.
 *
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [e]  Error. The function returned SQL_ERROR.
 *      [r]  Results. The statement will or did create a (possibly empty) result set.
 *      [nr] No results. The statement will not or did not create a result set.
 */
ACI_RC ulnSFID_45(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        if (0)
        {
            /*
             * BUGBUG: check [4] The current result is not the last result.
             */
            ACI_RAISE(LABEL_24000);
        }
    }
    else
    {
        /*
         * EXIT POINT
         */
        if (1)
        {
            /*
             * BUGBUG: the current result is the last or only result,
             * or there are no current results.
             */
            if (aFnContext->mSqlReturn != SQL_SUCCESS &&
                aFnContext->mSqlReturn != SQL_SUCCESS_WITH_INFO)
            {
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
            }
            else
            {
                if (ulnStateCheckR(aFnContext) == ACP_TRUE)
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
                }
                else
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
                }
            }
        }
        else
        {
            /*
             * BUGBUG: the current result is not the last result.
             */
            ACI_RAISE(LABEL_24000);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_24000)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_CURSOR_STATE);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * An application can retrieve result set metadata at any time after a statement has been
 * prepared or executed
 */

/*
 * SQLPrepare 가 발생시킬 수 있는 에러들의 정보
 *
 * O : Implementation Complete
 * X : Do not need to implement
 *   : Need to do
 *
 * SQLSTATE  Error
 *           Description
 *
 * [ ] 01000 General warning
 *           Driver-specific informational message.
 *           (Function returns SQL_SUCCESS_WITH_INFO.)
 * [ ] 01S02 Option value changed
 *           A specified statement attribute was invalid because of implementation working
 *           conditions, so a similar value was temporarily substituted.
 *           (SQLGetStmtAttr can be called to determine what the temporarily substituted
 *           value is.) The substitute value is valid for the StatementHandle
 *           until the cursor is closed.
 *           The statement attributes that can be changed are:
 *              SQL_ATTR_CONCURRENCY
 *              SQL_ATTR_CURSOR_TYPE
 *              SQL_ATTR_KEYSET_SIZE
 *              SQL_ATTR_MAX_LENGTH
 *              SQL_ATTR_MAX_ROWS
 *              SQL_ATTR_QUERY_TIMEOUT
 *              SQL_ATTR_SIMULATE_CURSOR
 *           (Function returns SQL_SUCCESS_WITH_INFO.)
 * [O] 08S01 Communication link failure
 *           The communication link between the driver and the data source
 *           to which the driver was connected failed before the function completed processing.
 * [ ] 21S01 Insert value list does not match column list
 *           *StatementText contained an INSERT statement, and the number of values
 *           to be inserted did not match the degree of the derived table.
 * [ ] 21S02 Degree of derived table does not match column list
 *           *StatementText contained a CREATE VIEW statement,
 *           and the number of names specified is not the same degree as the derived table
 *           defined by the query specification.
 * [ ] 22018 Invalid character value for cast specification
 *           *StatementText contained an SQL statement that contained a literal or parameter,
 *           and the value was incompatible with the data type of the associated table column.
 * [ ] 22019 Invalid escape character
 *           The argument StatementText contained a LIKE predicate with an ESCAPE
 *           in the WHERE clause, and the length of the escape character following ESCAPE
 *           was not equal to 1.
 * [ ] 22025 Invalid escape sequence
 *           The argument StatementText contained "LIKE pattern value ESCAPE escape character"
 *           in the WHERE clause, and the character following the escape character
 *           in the pattern value was neither "%" nor "_".
 * [ ] 24000 Invalid cursor state
 *           (DM) A cursor was open on the StatementHandle,
 *           and SQLFetch or SQLFetchScroll had been called.
 *           A cursor was open on the StatementHandle,
 *           but SQLFetch or SQLFetchScroll had not been called.
 * [ ] 34000 Invalid cursor name
 *           *StatementText contained a positioned DELETE or a positioned UPDATE,
 *           and the cursor referenced by the statement being prepared was not open.
 * [ ] 3D000 Invalid catalog name
 *           The catalog name specified in StatementText was invalid.
 * [ ] 3F000 Invalid schema name
 *           The schema name specified in StatementText was invalid.
 * [ ] 42000 Syntax error or access violation
 *           *StatementText contained an SQL statement that was not preparable
 *           or contained a syntax error.
 *           *StatementText contained a statement for which the user did not have the required
 *           privileges.
 * [ ] 42S01 Base table or view already exists
 *           *StatementText contained a CREATE TABLE or CREATE VIEW statement,
 *           and the table name or view name specified already exists.
 * [ ] 42S02 Base table or view not found
 *           *StatementText contained a DROP TABLE or a DROP VIEW statement,
 *           and the specified table name or view name did not exist.
 *           *StatementText contained an ALTER TABLE statement,
 *           and the specified table name did not exist.
 *           *StatementText contained a CREATE VIEW statement,
 *           and a table name or view name defined by the query specification did not exist.
 *           *StatementText contained a CREATE INDEX statement,
 *           and the specified table name did not exist.
 *           *StatementText contained a GRANT or REVOKE statement,
 *           and the specified table name or view name did not exist.
 *           *StatementText contained a SELECT statement,
 *           and a specified table name or view name did not exist.
 *           *StatementText contained a DELETE, INSERT, or UPDATE statement,
 *           and the specified table name did not exist.
 *           *StatementText contained a CREATE TABLE statement,
 *           and a table specified in a constraint (referencing a table other than
 *                   the one being created) did not exist.
 * [ ] 42S11 Index already exists
 *           *StatementText contained a CREATE INDEX statement,
 *           and the specified index name already existed.
 * [ ] 42S12 Index not found
 *           *StatementText contained a DROP INDEX statement,
 *           and the specified index name did not exist.
 * [ ] 42S21 Column already exists
 *           *StatementText contained an ALTER TABLE statement,
 *           and the column specified in the ADD clause is not unique or identifies
 *           an existing column in the base table.
 * [ ] 42S22 Column not found
 *           *StatementText contained a CREATE INDEX statement,
 *           and one or more of the column names specified in the column list did not exist.
 *           *StatementText contained a GRANT or REVOKE statement,
 *           and a specified column name did not exist.
 *           *StatementText contained a SELECT, DELETE, INSERT, or UPDATE statement,
 *           and a specified column name did not exist.
 *           *StatementText contained a CREATE TABLE statement, and a column specified in
 *           a constraint (referencing a table other than the one being created) did not exist.
 * [ ] HY000 General error
 *           An error occurred for which there was no specific SQLSTATE and
 *           for which no implementation-specific SQLSTATE was defined.
 *           The error message returned by SQLGetDiagRec in the *MessageText buffer describes
 *           the error and its cause.
 * [X] HY001 Memory allocation error
 *           The driver was unable to allocate memory required to support execution or
 *           completion of the function.
 * [ ] HY008 Operation canceled
 *           Asynchronous processing was enabled for the StatementHandle.
 *           The function was called, and before it completed execution, SQLCancel was called on
 *           the StatementHandle, and then the function was called again on the StatementHandle.
 *           The function was called, and before it completed execution, SQLCancel was called on
 *           the StatementHandle from a different thread in a multithread application.
 * [O] HY009 Invalid use of null pointer
 *           (DM) StatementText was a null pointer.
 * [ ] HY010 Function sequence error
 *           (DM) An asynchronously executing function (not this one) was called for
 *           the StatementHandle and was still executing when this function was called.
 *           (DM) SQLExecute, SQLExecDirect, SQLBulkOperations, or SQLSetPos was called for
 *           the StatementHandle and returned SQL_NEED_DATA. This function was called
 *           before data was sent for all data-at-execution parameters or columns.
 * [X] HY013 Memory management error
 *           The function call could not be processed because the underlying memory objects
 *           could not be accessed, possibly because of low memory conditions.
 * [O] HY090 Invalid string or buffer length
 *           (DM) The argument TextLength was less than or equal to 0 but not equal to SQL_NTS.
 * [ ] HYC00 Optional feature not implemented
 *           The concurrency setting was invalid for the type of cursor defined.
 *           The SQL_ATTR_USE_BOOKMARKS statement attribute was set to SQL_UB_VARIABLE,
 *           and the SQL_ATTR_CURSOR_TYPE statement attribute was set to a cursor type
 *           for which the driver does not support bookmarks.
 * [ ] HYT00 Timeout expired
 *           The timeout period expired before the data source returned the result set.
 *           The timeout period is set through SQLSetStmtAttr, SQL_ATTR_QUERY_TIMEOUT.
 * [ ] HYT01 Connection timeout expired
 *           The connection timeout period expired before the data source responded to
 *           the request. The connection timeout period is set through SQLSetConnectAttr,
 *           SQL_ATTR_CONNECTION_TIMEOUT.
 * [ ] IM001 Driver does not support this function
 *           (DM) The driver associated with the StatementHandle does not support the function.
 */

ACI_RC ulnCallbackPrepareResult(cmiProtocolContext *aProtocolContext,
                                cmiProtocol        *aProtocol,
                                void               *aServiceSession,
                                void               *aUserContext)
{
    ulnFnContext          *sFnContext  = (ulnFnContext *)aUserContext;
    ulnStmt               *sStmt       = sFnContext->mHandle.mStmt;

    acp_uint32_t           sStatementID;
    acp_uint32_t           sStatementType;
    acp_uint16_t           sParamCount;
    acp_uint16_t           sResultSetCount;

    acp_uint16_t           sStatePoint; 
    acp_uint8_t            sSimpleQuery = 0;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD4(aProtocolContext, &sStatementType);
    CMI_RD2(aProtocolContext, &sParamCount);
    CMI_RD2(aProtocolContext, &sResultSetCount);
    CMI_SKIP_READ_BLOCK(aProtocolContext, 8);  /* BUG-48775 Reserved 8 bytes */

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        CMI_RD1(aProtocolContext, sSimpleQuery);
        sStmt->mIsSimpleQuery = (sSimpleQuery == 0 ? ACP_FALSE : ACP_TRUE);
    }
    else
    {
        /* do nothing. */
    }

    ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sStmt) != ULN_OBJ_TYPE_STMT, LABEL_MEM_MANAGE_ERR);

    ulnStmtFreeAllResult(sStmt);

    sStmt->mIsPrepared  = ACP_TRUE;
    ulnStmtSetStatementID(sStmt, sStatementID);
    ulnStmtSetStatementType(sStmt, sStatementType);

    ulnStmtSetParamCount(sStmt, sParamCount);
    ulnStmtSetResultSetCount(sStmt, sResultSetCount);

    // BUG-17592
    if (sResultSetCount > 0)
    {
        sStmt->mResultType = ULN_RESULT_TYPE_RESULTSET;
    }
    else
    {
        sStmt->mResultType = ULN_RESULT_TYPE_ROWCOUNT;
    }

    if (ulnStmtGetAttrDeferredPrepare(sStmt) == ULN_CONN_DEFERRED_PREPARE_ON)
    {
        /* In case of DirectExecute, sStmt cannot have mDeferredPrepareStateFunc */
        if(sStmt->mDeferredPrepareStateFunc != NULL)
        {
            sStatePoint = sFnContext->mWhere;
            sFnContext->mWhere = ULN_STATE_EXIT_POINT;
            ACI_TEST_RAISE(sStmt->mDeferredPrepareStateFunc(sFnContext) != ACI_SUCCESS, LABEL_DEFERRED_PREPARE);

            sFnContext->mWhere = sStatePoint;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "CallbackPrepareResult, Object is not a STMT handle.");
    }
    ACI_EXCEPTION(LABEL_DEFERRED_PREPARE)
    {
        sFnContext->mWhere = sStatePoint;
    }
    ACI_EXCEPTION_END;

    /*
     * Note : ACI_SUCCESS 를 리턴하는 것은 버그가 아니다.
     *        cm 의 콜백함수가 ACI_FAILURE 를 리턴하면 communication error 로 취급되어 버리기
     *        때문이다.
     *
     *        어찌되었던 간에, Function Context 의 멤버인 mSqlReturn 에 함수 리턴값이
     *        저장되게 될 것이며, uln 의 cmi 매핑 함수인 ulnReadProtocol() 함수 안에서
     *        Function Context 의 mSqlReturn 을 체크해서 적절한 조치를 취하게 될 것이다.
     */
    return ACI_SUCCESS;
}

ACI_RC ulnPrepCheckArgs(ulnFnContext *aFnContext, acp_char_t *aStatementText, acp_sint32_t aTextLength)
{
    /*
     * HY009 : Invalide Use of Null Pointer
     */
    ACI_TEST_RAISE(aStatementText == NULL, LABEL_HY009);

    /*
     * HY090 : Invalid string of buffer length
     */
    ACI_TEST_RAISE(aTextLength <= 0 && aTextLength != SQL_NTS, LABEL_HY090);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_HY009)
    {
        /*
         * HY009
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION(LABEL_HY090)
    {
        /*
         * HY090
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aTextLength);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnPrepDoText(ulnFnContext *aFnContext,
                     ulnEscape    *aEsc,
                     ulnCharSet   *aCharSet,
                     acp_char_t   *aStatementText,
                     acp_sint32_t  aTextLength)
{
    ulnDbc    *sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST( aTextLength <= 0 );

    // PROJ-1579 NCHAR
    // 클라이언트 캐릭터 셋 => 데이터베이스 캐릭터 셋으로 변환한다.
    ACI_TEST(ulnCharSetConvert(aCharSet,
                               aFnContext,
                               NULL,
                               sDbc->mClientCharsetLangModule,    //BUG-22684
                               (const mtlModule*)sDbc->mCharsetLangModule,
                               (void*)aStatementText,
                               aTextLength,
                               CONV_DATA_IN)
             != ACI_SUCCESS);

    /*
     * Statement Text 를 unescape 한다.
     */
    ACI_TEST_RAISE(ulnEscapeUnescapeByLen(aEsc,
                                          (acp_char_t*)ulnCharSetGetConvertedText(aCharSet),
                                          ulnCharSetGetConvertedTextLen(aCharSet)) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnPrepDoText");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnPrepDoTextNchar(ulnFnContext *aFnContext,
                          ulnEscape    *aEsc,
                          ulnCharSet   *aCharSet,
                          acp_char_t   *aStatementText,
                          acp_sint32_t  aTextLength)
{
    ulnDbc    *sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST( aTextLength <= 0 );

    // PROJ-1579 NCHAR
    // 클라이언트 캐릭터 셋 => 데이터베이스 캐릭터 셋으로 변환한다.
    ACI_TEST(ulnCharSetConvertNLiteral(aCharSet,
                                       aFnContext,
                                       sDbc->mClientCharsetLangModule,    //BUG-22684
                                       (const mtlModule*)sDbc->mCharsetLangModule,
                                       (void*)aStatementText,
                                       aTextLength)
             != ACI_SUCCESS);

    /*
     * Statement Text 를 unescape 한다.
     */
    ACI_TEST_RAISE(ulnEscapeUnescapeByLen(aEsc,
                                          (acp_char_t*)ulnCharSetGetConvertedText(aCharSet),
                                          ulnCharSetGetConvertedTextLen(aCharSet)) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnPrepDoTextNchar");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-1789 Updatabel Scrollable Cursor */

/**
 * pre-prepare cursor attributes downgrade for scrollable/updatable cursor
 *
 * @param[in] aFnContext function context
 * @param[in] aStmt      statement handle
 */
void ulnPrepPreDowngrade(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    if (ulnStmtGetAttrCursorScrollable(sStmt) == SQL_NONSCROLLABLE)
    {
        if (ulnStmtGetAttrCursorType(sStmt) != SQL_CURSOR_FORWARD_ONLY)
        {
            ulnStmtSetAttrCursorType(sStmt, SQL_CURSOR_FORWARD_ONLY);

            ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_CURSOR_TYPE downgraded to SQL_CURSOR_FORWARD_ONLY");
        }
    }
    else /* if SQL_SCROLLABLE */
    {
        if (ulnStmtGetAttrCursorSensitivity(sStmt) == SQL_INSENSITIVE)
        {
            if (ulnStmtGetAttrCursorType(sStmt) != SQL_CURSOR_STATIC)
            {
                ulnStmtSetAttrCursorType(sStmt, SQL_CURSOR_STATIC);
                ulnStmtSetAttrConcurrency(sStmt, SQL_CONCUR_READ_ONLY);
 
                ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                         "SQL_ATTR_CURSOR_TYPE downgraded to SQL_CURSOR_STATIC");
            }
        }
    }

    switch (ulnStmtGetAttrCursorType(sStmt))
    {
        /* FORWARD_ONLY, STATIC은 INSENSITIVE + READ_ONLY만 지원 */
        case SQL_CURSOR_FORWARD_ONLY:
        case SQL_CURSOR_STATIC:
            if (ulnStmtGetAttrCursorSensitivity(sStmt) != SQL_INSENSITIVE)
            {
                ulnStmtSetAttrCursorSensitivity(sStmt, SQL_INSENSITIVE);

                ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                         "SQL_ATTR_CURSOR_SENSITIVITY changed to SQL_INSENSITIVE");
            }

            if (ulnStmtGetAttrConcurrency(sStmt) != SQL_CONCUR_READ_ONLY)
            {
                ulnStmtSetAttrConcurrency(sStmt, SQL_CONCUR_READ_ONLY);

                ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                         "SQL_ATTR_CONCURRENCY changed to SQL_CONCUR_READ_ONLY");
            }
            break;

        /* KEYSET_DRIVEN은 SQL_CONCUR_READ_ONLY, SQL_CONCUR_ROWVER만 지원 */
        case SQL_CURSOR_KEYSET_DRIVEN:
            if ((ulnStmtGetAttrConcurrency(sStmt) != SQL_CONCUR_READ_ONLY)
             && (ulnStmtGetAttrConcurrency(sStmt) != SQL_CONCUR_ROWVER))
            {
                ulnStmtSetAttrConcurrency(sStmt, SQL_CONCUR_ROWVER);

                ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                         "SQL_ATTR_CONCURRENCY changed to SQL_CONCUR_ROWVER");
            }
            break;

        default:
            ACE_ASSERT(ACP_FALSE);
            break;
    }
}

/**
 * post-prepare cursor attributes downgrade for scrollable/updatable cursor
 *
 * @param[in] aFnContext function context
 */
void ulnPrepPostDowngrade(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    /* insensitive keyset-driven은 지원하지 않으므로 항상 static으로 떨군다.
     * static은 forward-only를 이용하므로 실패할 일이 없다. */

    ulnStmtSetAttrCursorType(sStmt, SQL_CURSOR_STATIC);
    ulnStmtSetAttrCursorSensitivity(sStmt, SQL_INSENSITIVE);
    ulnStmtSetAttrConcurrency(sStmt, SQL_CONCUR_READ_ONLY);

    ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
             "SQL_ATTR_CURSOR_TYPE downgraded to SQL_CURSOR_STATIC");
}

/**
 * 공백과 주석을 제외한 구문의 시작 위치를 찾는다.
 *
 * @param[in] aFnContext   function context
 * @param[in] aSrcQstr     원본 쿼리문
 * @param[in] aSrcQstrSize 쿼리문의 길이(octet length)
 * @param[in] aStartPos    검색을 시작할 인덱스(octet base)
 *
 * @return 0부터 시작하는 octet 단위 index
 */
acp_sint32_t ulnPrepIndexOfNonWhitespaceAndComment(ulnFnContext     *aFnContext,
                                                   const acp_char_t *aSrcQstr,
                                                   acp_sint32_t      aSrcQstrSize,
                                                   acp_sint32_t      aStartPos)
{
    acp_sint32_t  i;

    ACP_UNUSED(aFnContext);

    /* skip starting white-space and comments */
    for (i = aStartPos; i < aSrcQstrSize; i++)
    {
        switch (aSrcQstr[i])
        {
            /* ' ~ ' */
            case '\'':
                for (i++; (i < aSrcQstrSize) && (aSrcQstr[i] != '\''); i++)
                    ;
                break;

            /* " ~ " */
            case '"':
                for (i++; (i < aSrcQstrSize) && (aSrcQstr[i] != '"'); i++)
                    ;
                break;

            /* ( ~ ) */
            case '(':
                for (i++; (i < aSrcQstrSize) && (aSrcQstr[i] != ')'); i++)
                    ;
                break;

            /* C & C++ style comments */
            case '/':
                /* block comment */
                if (aSrcQstr[i + 1] == '*')
                {
                    for (i += 2; i < aSrcQstrSize; i++)
                    {
                        if ((aSrcQstr[i] == '*') && (aSrcQstr[i + 1] == '/'))
                        {
                            i++;
                            break;
                        }
                    }
                }
                /* line comment */
                else if (aSrcQstr[i + 1] == '/')
                {
                    for (i += 2; (i < aSrcQstrSize) && (aSrcQstr[i] != '\n' && aSrcQstr[i] != '\r'); i++)
                        ; /* skip to end-of-line */
                }
                break;

            /* line comment */
            case '-':
                if (aSrcQstr[i + 1] == '-')
                {
                    for (i += 2; (i < aSrcQstrSize) && (aSrcQstr[i] != '\n' && aSrcQstr[i] != '\r'); i++)
                        ; /* skip to end-of-line */
                }
                break;

            /* Skip white-space */
            case ' ' :
            case '\t':
            case '\n':
            case '\r':
                break;

            default:
                ACI_RAISE(RETURN_INDEX);
                break;
        }
    }

    ACI_EXCEPTION_CONT(RETURN_INDEX);
    return i;
}

/**
 * SELECT문의 시작 위치를 찾는다.
 *
 * @param[in] aFnContext   function context
 * @param[in] aSrcQstr     원본 쿼리문
 * @param[in] aSrcQstrSize 쿼리문의 길이(octet length)
 * @param[in] aStartPos    검색을 시작할 인덱스(octet base)
 *
 * @return 0부터 시작하는 octet 단위 index. "SELECT"로 시작하지 않으면 -1
 */
acp_sint32_t ulnPrepIndexOfSelect(ulnFnContext     *aFnContext,
                                  const acp_char_t *aSrcQstr,
                                  acp_sint32_t      aSrcQstrSize,
                                  acp_sint32_t      aStartPos)
{
    acp_sint32_t i;

    ACP_UNUSED(aFnContext);

    i = ulnPrepIndexOfNonWhitespaceAndComment(aFnContext, aSrcQstr, aSrcQstrSize, aStartPos);
    if ((i + 6 >= aSrcQstrSize) || (acpCStrCaseCmp("SELECT", aSrcQstr + i, 6) != 0))
    {
        i = -1;
    }

    return i;
}

ACP_INLINE acp_bool_t ulnPrepIsValidFromPrevChar(acp_char_t aCh)
{
    acp_bool_t sResult = ACP_FALSE;
    if ((acpCharIsSpace(aCh) == ACP_TRUE)
        || (aCh == '\'')
        || (aCh == '"')
        || (aCh == '/')
        || (aCh == ')'))
    {
        sResult = ACP_TRUE;
    }
    return sResult;
}

ACP_INLINE acp_bool_t ulnPrepIsValidFromNextChar(acp_char_t aCh)
{
    acp_bool_t sResult = ACP_FALSE;
    if ((acpCharIsSpace(aCh) == ACP_TRUE)
        || (aCh == '"')
        || (aCh == '/')
        || (aCh == '(')
        || (aCh == '-'))
    {
        sResult = ACP_TRUE;
    }
    return sResult;
}

/**
 * FROM의 시작 위치를 찾는다.
 *
 * @param[in] aFnContext   function context
 * @param[in] aSrcQstr     원본 쿼리문
 * @param[in] aSrcQstrSize 쿼리문의 길이(octet length)
 * @param[in] aStartPos    검색을 시작할 인덱스(octet base)
 *
 * @return FROM 절이 있으면 FROM의 시작 위치(0 base), 아니면 -1
 */
acp_sint32_t ulnPrepIndexOfFrom(ulnFnContext     *aFnContext,
                                const acp_char_t *aSrcQstr,
                                acp_sint32_t      aSrcQstrSize,
                                acp_sint32_t      aStartPos)
{
    acp_sint32_t  i;

    ACP_UNUSED(aFnContext);

    for (i = aStartPos; i < aSrcQstrSize; i++)
    {
        switch (aSrcQstr[i])
        {
            /* ' ~ ' */
            case '\'':
                for (i++; (i < aSrcQstrSize) && (aSrcQstr[i] != '\''); i++)
                    ;
                break;

            /* " ~ " */
            case '"':
                for (i++; (i < aSrcQstrSize) && (aSrcQstr[i] != '"'); i++)
                    ;
                break;

            /* ( ~ ) */
            case '(':
                for (i++; (i < aSrcQstrSize) && (aSrcQstr[i] != ')'); i++)
                    ;
                break;

            /* C & C++ style comments */
            case '/':
                /* block comment */
                if (aSrcQstr[i+1] == '*')
                {
                    for (i+=2; i < aSrcQstrSize; i++)
                    {
                        if ((aSrcQstr[i] == '*')
                         && (aSrcQstr[i+1] == '/'))
                        {
                            i++;
                            break;
                        }
                    }
                }
                /* line comment */
                else if (aSrcQstr[i+1] == '/')
                {
                    for (i+=2; (i < aSrcQstrSize) && (aSrcQstr[i] != '\n'); i++)
                        ; /* skip to end-of-line */
                }
                break;

            /* line comment */
            case '-':
                if (aSrcQstr[i+1] == '-')
                {
                    for (i+=2; (i < aSrcQstrSize) && (aSrcQstr[i] != '\n'); i++)
                        ; /* skip to end-of-line */
                }
                break;

            case 'F':
            case 'f':
                /* {공백/블럭}FROM{공백/블럭} 인지 확인 */
                if ((i == 0)
                 || (i+5 >= aSrcQstrSize)
                 || (ulnPrepIsValidFromPrevChar(aSrcQstr[i-1]) == ACP_FALSE))
                {
                    break;
                }

                if ((acpCStrCaseCmp(aSrcQstr + i, "FROM", 4) == 0)
                 && (ulnPrepIsValidFromNextChar(aSrcQstr[i+4]) == ACP_TRUE))
                {
                    ACI_RAISE(LABEL_FROM_FOUND);
                }
                break;

            default:
                break;
        }
    }
    i = -1; /* not found */

    ACE_EXCEPTION_CONT(LABEL_FROM_FOUND);

    return i;
}

/**
 * 다음에 나오는 첫번째 non-space 문자 위치를 얻는다.
 *
 * @param[in] aFnContext    function context
 * @param[in] aSrcStr       string
 * @param[in] aSrcStrSize   string length
 * @param[in] aStartPos     start position
 *
 * @return 처음 나오는 공백(white-space) 아닌 문자 위치(0 base),
 *         공백밖에 없으면 -1
 */
acp_sint32_t ulnPrepIndexOfNonSpace(ulnFnContext     *aFnContext,
                                    const acp_char_t *aSrcStr,
                                    acp_sint32_t      aSrcStrSize,
                                    acp_sint32_t      aStartPos)
{
    acp_sint32_t i;

    ACP_UNUSED(aFnContext);

    /* skip white-space */
    for (i = aStartPos; i < aSrcStrSize; i++)
    {
        if (acpCharIsSpace(aSrcStr[i]) != ACP_TRUE)
        {
            break;
        }
    }

    /* not found */
    if (i == aSrcStrSize)
    {
        i = -1;
    }

    return i;
}

/**
 * 다음에 나오는 ID의 끝 위치를 얻는다.
 *
 * @param[in] aFnContext    function context
 * @param[in] aSrcStr       string
 * @param[in] aSrcStrSize   string length
 * @param[in] aStartPos     start position
 *
 * @return 처음 나오는 ID의 끝 위치(0 base), ID가 없으면 -1
 */
acp_sint32_t ulnPrepIndexOfIdEnd(ulnFnContext     *aFnContext,
                                 const acp_char_t *aSrcStr,
                                 acp_sint32_t      aSrcStrSize,
                                 acp_sint32_t      aStartPos)
{
    acp_sint32_t i;

    i = ulnPrepIndexOfNonSpace(aFnContext, aSrcStr, aSrcStrSize, aStartPos);
    if (i != -1)
    {
        for (; i < aSrcStrSize; i++)
        {
            switch (aSrcStr[i])
            {
                /* " ~ " */
                case '"':
                    for (i++; (i < aSrcStrSize) && (aSrcStr[i] != '"'); i++)
                        ;
                    break;

                case '_':
                case '.':
                    break;

                default:
                    ACI_TEST_RAISE( acpCharIsAlnum(aSrcStr[i]) != ACP_TRUE,
                                    LABEL_ID_END_FOUND );
                    break;
            }
        }
        if (i > aSrcStrSize)
        {
            i = aSrcStrSize;
        }
        ACE_EXCEPTION_CONT(LABEL_ID_END_FOUND);
    }

    return i;
}

/**
 * FROM의 끝 위치를 찾는다.
 *
 * @param[in] aFnContext     function context
 * @param[in] aSrcQstr       원본 쿼리문
 * @param[in] aSrcQstrSize   쿼리문의 길이(octet length)
 * @param[in] aStartPos      검색을 시작할 인덱스(octet base)
 *
 * @return FROM 절이 있으면 FROM의 끝 위치(0 base), 아니면 -1
 */
acp_sint32_t ulnPrepIndexOfFromEnd(ulnFnContext     *aFnContext,
                                   const acp_char_t *aSrcQstr,
                                   acp_sint32_t      aSrcQstrSize,
                                   acp_sint32_t      aStartPos)
{
    acp_sint32_t i;
    acp_sint32_t sTokB;
    acp_sint32_t sTokE;
    acp_sint32_t sTokLen;

    i = ulnPrepIndexOfFrom(aFnContext, aSrcQstr, aSrcQstrSize, aStartPos);
    ACI_TEST_RAISE(i == -1, LABEL_FROM_NOT_FOUND);
    i += 4; /* skip "FROM" */

    /* tableName */
    i = ulnPrepIndexOfIdEnd(aFnContext, aSrcQstr, aSrcQstrSize, i);
    ACE_DASSERT(i != -1);

    /* if is not the end of qstr */
    if ((aSrcQstr[i] != '\0') && (aSrcQstr[i] != ';'))
    {
        /* aliasName or next clause */
        sTokB = ulnPrepIndexOfNonSpace(aFnContext, aSrcQstr, aSrcQstrSize, i);
        sTokE = ulnPrepIndexOfIdEnd(aFnContext, aSrcQstr, aSrcQstrSize, sTokB);

        sTokLen = sTokE - sTokB;

        /* FROM 절 다음에 "AS"가 나오거나,
         * FROM 절 다음에 나올 수 있는 예약어가 아니면 AS 없이 alias를 한거다. */
        switch (sTokLen)
        {
            case 2:
                if (acpCStrCaseCmp("AS", aSrcQstr + sTokB, 2) == 0)
                {
                    /* aliasName */
                    sTokB = ulnPrepIndexOfNonSpace(aFnContext, aSrcQstr, aSrcQstrSize, sTokE);
                    sTokE = ulnPrepIndexOfIdEnd(aFnContext, aSrcQstr, aSrcQstrSize, sTokB);

                    i = sTokE;
                }
                break;

            case 3:
                if (acpCStrCaseCmp("FOR", aSrcQstr + sTokB, 3) != 0)
                {
                    i = sTokE;
                }
                break;

            case 5:
                if ((acpCStrCaseCmp("WHERE", aSrcQstr + sTokB, 5) != 0)
                 && (acpCStrCaseCmp("GROUP", aSrcQstr + sTokB, 5) != 0)
                 && (acpCStrCaseCmp("UNION", aSrcQstr + sTokB, 5) != 0)
                 && (acpCStrCaseCmp("MINUS", aSrcQstr + sTokB, 5) != 0)
                 && (acpCStrCaseCmp("START", aSrcQstr + sTokB, 5) != 0)
                 && (acpCStrCaseCmp("ORDER", aSrcQstr + sTokB, 5) != 0)
                 && (acpCStrCaseCmp("LIMIT", aSrcQstr + sTokB, 5) != 0))
                {
                    i = sTokE;
                }
                break;

            case 6:
                if (acpCStrCaseCmp("HAVING", aSrcQstr + sTokB, 6) != 0)
                {
                    i = sTokE;
                }
                break;

            case 7:
                if (acpCStrCaseCmp("CONNECT", aSrcQstr + sTokB, 7) != 0)
                {
                    i = sTokE;
                }
                break;
            case 9:
                if (acpCStrCaseCmp("INTERSECT", aSrcQstr + sTokB, 9) != 0)
                {
                    i = sTokE;
                }
                break;

            default:
                i = sTokE;
                break;
        }
    }

    ACE_EXCEPTION_CONT(LABEL_FROM_NOT_FOUND);

    return i;
}

/**
 * SELECT 쿼리인지 확인.
 *
 * 단순히 쿼리문이 SELECT로 시작하는지만 보고 판단한다.
 *
 * @param[in] aFnContext     function context
 * @param[in] aStatementText query string
 * @param[in] aTextLength    octet length of query string
 *
 * @return SELECT 쿼리면 ACI_TRUE, 아니면 ACI_FALSE
 */
acp_bool_t ulnPrepIsSelect(ulnFnContext *aFnContext,
                           acp_char_t   *aStatementText,
                           acp_sint32_t  aTextLength)
{
    acp_sint32_t sPosStartOfSelect;

    sPosStartOfSelect = ulnPrepIndexOfSelect(aFnContext, aStatementText, aTextLength, 0);
    return (sPosStartOfSelect == -1) ? ACP_FALSE : ACP_TRUE;
}

/**
 * Updatable/Scrollable을 위한 SELECT 쿼리문에서
 * 에러 확인을 위한 쿼리문을 만든다.
 *
 * @param[in]  aFnContext       function context
 * @param[out] aDestQstr        변환된 쿼리문을 담을 버퍼
 * @param[in]  aDestQstrSize    버퍼 크기(octet length)
 * @param[out] aDestQstrOutSize 변환된 쿼리문의 길이(octet length)를 담을 변수
 * @param[in]  aSrcQstr         원본 쿼리문
 * @param[in]  aSrcQstrSize     쿼리문의 길이(octet length)
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBuildSelectForChkErr(ulnFnContext     *aFnContext,
                                   acp_char_t       *aDestQstr,
                                   acp_sint32_t      aDestQstrSize,
                                   acp_sint32_t     *aDestQstrOutSize,
                                   const acp_char_t *aSrcQstr,
                                   acp_sint32_t      aSrcQstrSize)
{
    acp_sint32_t  sCopyLen;
    acp_sint32_t  sOutLen;
    acp_sint32_t  i;

    ACP_UNUSED(aFnContext);

    i = ulnPrepIndexOfSelect(aFnContext, aSrcQstr, aSrcQstrSize, 0);
    ACI_TEST(i == -1);

    i += 6; /* skip "SELECT" */
    sCopyLen = aSrcQstrSize - i;
    sOutLen = ULN_PREP_CHKERR_PREFIX_CLEN + sCopyLen;
    ACI_TEST((sOutLen + 1) > aDestQstrSize);

    acpMemCpy(aDestQstr, ULN_PREP_CHKERR_PREFIX_CSTR,
              ULN_PREP_CHKERR_PREFIX_CLEN);
    acpMemCpy(aDestQstr + ULN_PREP_CHKERR_PREFIX_CLEN,
              aSrcQstr + i, sCopyLen);
    aDestQstr[sOutLen] = '\0';

    /*  Note. 내부에서만 쓰니까 NULL 체크는 굳이 하지 않는다. */
    *aDestQstrOutSize = sOutLen;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Updatable/Scrollable을 위한 SELECT 쿼리문에서
 * Keyset을 쌓기위한 쿼리문을 만든다.
 *
 * @param[in]  aFnContext       function context
 * @param[out] aDestQstr        변환된 쿼리문을 담을 버퍼
 * @param[in]  aDestQstrSize    버퍼 크기(octet length)
 * @param[out] aDestQstrOutSize 변환된 쿼리문의 길이(octet length)를 담을 변수
 * @param[in]  aSrcQstr         원본 쿼리문
 * @param[in]  aSrcQstrSize     쿼리문의 길이(octet length)
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBuildSelectForKeyset(ulnFnContext     *aFnContext,
                                   acp_char_t       *aDestQstr,
                                   acp_sint32_t      aDestQstrSize,
                                   acp_sint32_t     *aDestQstrOutSize,
                                   const acp_char_t *aSrcQstr,
                                   acp_sint32_t      aSrcQstrSize)
{
    acp_sint32_t  sOutLen;
    acp_sint32_t  i;

    ACP_UNUSED(aFnContext);

    i = ulnPrepIndexOfFrom(aFnContext, aSrcQstr, aSrcQstrSize, 0);
    ACI_TEST(i == -1);

    sOutLen = aSrcQstrSize - i + ULN_PREP_KEYSET_PREFIX_CLEN;
    ACI_TEST((sOutLen + 1) > aDestQstrSize);

    acpMemCpy(aDestQstr, ULN_PREP_KEYSET_PREFIX_CSTR,
              ULN_PREP_KEYSET_PREFIX_CLEN);
    acpMemCpy(aDestQstr + ULN_PREP_KEYSET_PREFIX_CLEN,
              aSrcQstr + i, aSrcQstrSize - i);
    aDestQstr[sOutLen] = '\0';

    /*  Note. 내부에서만 쓰니까 NULL 체크는 굳이 하지 않는다. */
    *aDestQstrOutSize = sOutLen;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Updatable/Scrollable을 위한 SELECT 쿼리문에서
 * Rowset을 쌓기위한 쿼리문의 기초를 만든다.
 *
 * @param[in] aFnContext   function context
 * @param[in] aSrcQstr     원본 쿼리문
 * @param[in] aSrcQstrSize 쿼리문의 길이(octet length)
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBuildSelectBaseForRowset(ulnFnContext      *aFnContext,
                                       const acp_char_t  *aSrcQstr,
                                       acp_sint32_t       aSrcQstrSize)
{
    ulnStmt        *sStmt;
    acp_sint32_t    i;
    acp_sint32_t    sPosStartOfTarget;
    acp_uint32_t    sCopyLen;
    acp_sint32_t    sQstrLen;
    acp_sint32_t    sPrefetchSize;

    sStmt = aFnContext->mHandle.mStmt;
    sPrefetchSize = ulnCacheCalcBlockSizeOfOneFetch(ulnStmtGetCache(sStmt),
                                                    ulnStmtGetCursor(sStmt));

    i = ulnPrepIndexOfSelect(aFnContext, aSrcQstr, aSrcQstrSize, 0);
    ACI_TEST(i == -1);
    sPosStartOfTarget = i + 6; /* skip "SELECT" */

    i = ulnPrepIndexOfFromEnd(aFnContext, aSrcQstr, aSrcQstrSize,
                              sPosStartOfTarget);
    ACI_TEST(i == -1);

    /* keyset-driven으로 열었을 때만 이 함수를 사용한다.
     * 그러므로, 원본 쿼리는 단일 테이블에 대한 단순 쿼리라고 간주할 수 있다.
     * (keyset-driven 제약조건으로 인함.)
     * 즉, FROM 절 이하는 싹 날려버려도 괜찮다는거. */
    sCopyLen = i - sPosStartOfTarget;

    sQstrLen = sCopyLen
             + ULN_PREP_ROWSET_PREFIX_CLEN + ULN_PREP_ROWSET_POSTFIX_B_CLEN
             + (ULN_PREP_ROWSET_POSTFIX_P_CLEN * (sPrefetchSize - 1))
             + ULN_PREP_ROWSET_POSTFIX_E_CLEN;
    ACI_TEST(ulnStmtEnsureReallocQstrForRowset(sStmt, sQstrLen + 1)
             != ACI_SUCCESS);

    acpMemCpy(sStmt->mQstrForRowset, ULN_PREP_ROWSET_PREFIX_CSTR,
              ULN_PREP_ROWSET_PREFIX_CLEN);
    acpMemCpy(sStmt->mQstrForRowset + ULN_PREP_ROWSET_PREFIX_CLEN,
              aSrcQstr + sPosStartOfTarget, sCopyLen);
    acpMemCpy(sStmt->mQstrForRowset + ULN_PREP_ROWSET_PREFIX_CLEN + sCopyLen,
              ULN_PREP_ROWSET_POSTFIX_B_CSTR, ULN_PREP_ROWSET_POSTFIX_B_CLEN);

    sStmt->mQstrForRowsetParamPos = ULN_PREP_ROWSET_PREFIX_CLEN
                                  + sCopyLen
                                  + ULN_PREP_ROWSET_POSTFIX_B_CLEN;

    sStmt->mQstrForRowsetParamCnt = 0;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Rowset을 쌓기위한 쿼리문을 만든다.
 *
 * @param[in] aFnContext function context
 * @param[in] aRowCount  가져올 Row 갯수
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBuildSelectForRowset(ulnFnContext      *aFnContext,
                                   acp_sint32_t       aRowCount)
{
    ulnStmt      *sStmt;
    acp_char_t   *sQstrPtr;
    acp_sint32_t  sQstrLen;
    acp_sint32_t  i;

    sStmt = aFnContext->mHandle.mStmt;

    ACI_TEST_RAISE(sStmt->mQstrForRowsetParamCnt == aRowCount,
                   LABEL_CONT_ALREADY_BUILD);

    sQstrPtr = sStmt->mQstrForRowset + sStmt->mQstrForRowsetParamPos;

    sQstrLen = sStmt->mQstrForRowsetParamPos
             + (ULN_PREP_ROWSET_POSTFIX_P_CLEN * (aRowCount - 1))
             + ULN_PREP_ROWSET_POSTFIX_E_CLEN;
    ACI_TEST(ulnStmtEnsureReallocQstrForRowset(sStmt, sQstrLen + 1)
             != ACI_SUCCESS);

    for (i = 0; i < aRowCount - 1; i++)
    {
        acpMemCpy(sQstrPtr, ULN_PREP_ROWSET_POSTFIX_P_CSTR,
                  ULN_PREP_ROWSET_POSTFIX_P_CLEN);
        sQstrPtr += ULN_PREP_ROWSET_POSTFIX_P_CLEN;
    }

    acpMemCpy(sQstrPtr, ULN_PREP_ROWSET_POSTFIX_E_CSTR,
              ULN_PREP_ROWSET_POSTFIX_E_CLEN);
    sQstrPtr[ULN_PREP_ROWSET_POSTFIX_E_CLEN] = '\0';

    sStmt->mQstrForRowsetLen = sQstrLen;

    sStmt->mQstrForRowsetParamCnt = aRowCount;

    ACI_EXCEPTION_CONT(LABEL_CONT_ALREADY_BUILD);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * UPDATE를 위한 쿼리문을 만든다.
 *
 * @param[in] aFnContext function context
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBuildUpdateQstr(ulnFnContext        *aFnContext,
                              ulnColumnIgnoreFlag *aColumnIgnoreFlags)
{
    ulnStmt      *sStmt;
    acp_char_t   *sQstrPtr;
    acp_sint32_t  sQstrMaxLen;
    acp_sint32_t  sColTotCount;
    ulnDescRec   *sDescRecIrd;
    acp_char_t   *sBaseColName;
    acp_sint32_t  sBaseColNameLen;
    acp_bool_t    sParamStarted;
    acp_sint32_t  i;

    sStmt = aFnContext->mHandle.mStmt;

    ACI_TEST_RAISE(ulnStmtBuildTableNameForUpdate(sStmt)
                   != ACI_SUCCESS, FUNC_SEQ_EXCEPTION);

    sColTotCount = ulnStmtGetColumnCount(sStmt);

    sQstrMaxLen = ULN_PREP_UPDATE_PREFIX_CLEN
                + ulnStmtGetTableNameForUpdateLen(sStmt)
                + (ULN_PREP_UPDATE_SET_MAX_CLEN * sColTotCount)
                + ULN_PREP_UPDATE_POSTFIX_CLEN;

    ACI_TEST_RAISE(ulnStmtEnsureAllocQstrForUpdate(sStmt, sQstrMaxLen + 1)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sQstrPtr = sStmt->mQstrForInsUpd;

    acpMemCpy(sQstrPtr, ULN_PREP_UPDATE_PREFIX_CSTR,
              ULN_PREP_UPDATE_PREFIX_CLEN);
    sQstrPtr += ULN_PREP_UPDATE_PREFIX_CLEN;

    acpMemCpy(sQstrPtr, ulnStmtGetTableNameForUpdate(sStmt),
              ulnStmtGetTableNameForUpdateLen(sStmt));
    sQstrPtr += ulnStmtGetTableNameForUpdateLen(sStmt);

    sParamStarted = ACP_TRUE;
    for (i = 1; i <= sColTotCount; i++)
    {
        if ((aColumnIgnoreFlags != NULL)
         && (aColumnIgnoreFlags[i-1] == ULN_COLUMN_IGNORE))
        {
            continue;
        }

        if (sParamStarted == ACP_TRUE)
        {
            acpMemCpy(sQstrPtr, ULN_PREP_UPDATE_SET_B_CSTR,
                      ULN_PREP_UPDATE_SET_B_CLEN);
            sQstrPtr += ULN_PREP_UPDATE_SET_B_CLEN;
            sParamStarted = ACP_FALSE;
        }
        else
        {
            acpMemCpy(sQstrPtr, ULN_PREP_UPDATE_SET_C_CSTR,
                      ULN_PREP_UPDATE_SET_C_CLEN);
            sQstrPtr += ULN_PREP_UPDATE_SET_C_CLEN;
        }

        sDescRecIrd = ulnStmtGetIrdRec(sStmt, i);
        ACI_TEST_RAISE(sDescRecIrd == NULL, FUNC_SEQ_EXCEPTION);

        sBaseColName = ulnDescRecGetBaseColumnName(sDescRecIrd);
        ACI_TEST_RAISE(sBaseColName == NULL, FUNC_SEQ_EXCEPTION);
        sBaseColNameLen = acpCStrLen(sBaseColName, ACP_SINT32_MAX);
        ACI_TEST_RAISE(sBaseColNameLen == 0, FUNC_SEQ_EXCEPTION);
        acpMemCpy(sQstrPtr, sBaseColName, sBaseColNameLen);
        sQstrPtr += sBaseColNameLen;

        acpMemCpy(sQstrPtr, ULN_PREP_UPDATE_SET_E_CSTR,
                  ULN_PREP_UPDATE_SET_E_CLEN);
        sQstrPtr += ULN_PREP_UPDATE_SET_E_CLEN;
    }

    acpMemCpy(sQstrPtr, ULN_PREP_UPDATE_POSTFIX_CSTR,
              ULN_PREP_UPDATE_POSTFIX_CLEN);
    sQstrPtr += ULN_PREP_UPDATE_POSTFIX_CLEN;
    *sQstrPtr = '\0';

    sStmt->mQstrForInsUpdLen = (sQstrPtr - sStmt->mQstrForInsUpd);

    return ACI_SUCCESS;

    ACI_EXCEPTION(FUNC_SEQ_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION(NOT_ENOUGH_MEM_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnPrepBuildUpdateQstr");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * DELETE를 위한 쿼리문을 만든다.
 *
 * @param[in] aFnContext function context
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBuildDeleteQstr(ulnFnContext *aFnContext)
{
    ulnStmt      *sStmt;
    acp_sint32_t  sQstrLen;

    sStmt = aFnContext->mHandle.mStmt;

    ACP_TEST_RAISE(ulnStmtIsQstrForDeleteBuilt(sStmt) == ACP_TRUE,
                   ALREADY_DELQSTR_BUILT_CONT);

    ACI_TEST_RAISE(ulnStmtBuildTableNameForUpdate(sStmt)
                   != ACI_SUCCESS, FUNC_SEQ_EXCEPTION);

    sQstrLen = ULN_PREP_DELETE_FORM_CLEN
             + ulnStmtGetTableNameForUpdateLen(sStmt);
    ACI_TEST_RAISE(ulnStmtEnsureAllocQstrForDelete(sStmt, sQstrLen + 1)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);

    (void) acpSnprintf(sStmt->mQstrForDelete, sQstrLen + 1,
                       ULN_PREP_DELETE_FORM_CSTR,
                       ulnStmtGetTableNameForUpdate(sStmt));

    sStmt->mQstrForDeleteLen = sQstrLen;

    ACI_EXCEPTION_CONT(ALREADY_DELQSTR_BUILT_CONT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(FUNC_SEQ_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION(NOT_ENOUGH_MEM_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnPrepBuildDeleteQstr");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * INSERT를 위한 쿼리문을 만든다.
 *
 * @param[in] aFnContext function context
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBuildInsertQstr(ulnFnContext        *aFnContext,
                              ulnColumnIgnoreFlag *aColumnIgnoreFlags)
{
    ulnStmt      *sStmt;
    acp_char_t   *sQstrPtr;
    acp_sint32_t  sQstrMaxLen;
    acp_sint32_t  sColTotCount;
    acp_sint32_t  sColActCount;
    ulnDescRec   *sDescRecIrd;
    acp_char_t   *sBaseColName;
    acp_sint32_t  sBaseColNameLen;
    acp_bool_t    sParamStarted;
    acp_sint32_t  i;

    sStmt = aFnContext->mHandle.mStmt;

    ACI_TEST_RAISE(ulnStmtBuildTableNameForUpdate(sStmt)
                   != ACI_SUCCESS, FUNC_SEQ_EXCEPTION);

    sColTotCount = ulnStmtGetColumnCount(sStmt);

    sQstrMaxLen = ULN_PREP_INSERT_PREFIX_CLEN
                + ulnStmtGetTableNameForUpdateLen(sStmt)
                + ULN_PREP_INSERT_COL_B_CLEN
                + ((ULN_PREP_INSERT_COL_C_CLEN + ULN_MAX_NAME_LEN) * (sColTotCount - 1))
                + ULN_PREP_INSERT_COL_E_CLEN
                + ULN_PREP_INSERT_PARAM_B_CLEN
                + (ULN_PREP_INSERT_PARAM_C_CLEN * (sColTotCount - 1))
                + ULN_PREP_INSERT_PARAM_E_CLEN;

    ACI_TEST_RAISE(ulnStmtEnsureAllocQstrForUpdate(sStmt, sQstrMaxLen + 1)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sQstrPtr = sStmt->mQstrForInsUpd;

    acpMemCpy(sQstrPtr, ULN_PREP_INSERT_PREFIX_CSTR,
              ULN_PREP_INSERT_PREFIX_CLEN);
    sQstrPtr += ULN_PREP_INSERT_PREFIX_CLEN;

    acpMemCpy(sQstrPtr, ulnStmtGetTableNameForUpdate(sStmt),
              ulnStmtGetTableNameForUpdateLen(sStmt));
    sQstrPtr += ulnStmtGetTableNameForUpdateLen(sStmt);

    sParamStarted = ACP_TRUE;
    sColActCount = 0;
    for (i = 1; i <= sColTotCount; i++)
    {
        if ((aColumnIgnoreFlags != NULL)
         && (aColumnIgnoreFlags[i-1] == ULN_COLUMN_IGNORE))
        {
            continue;
        }

        if (sParamStarted == ACP_TRUE)
        {
            acpMemCpy(sQstrPtr, ULN_PREP_INSERT_COL_B_CSTR,
                      ULN_PREP_INSERT_COL_B_CLEN);
            sQstrPtr += ULN_PREP_INSERT_COL_B_CLEN;
            sParamStarted = ACP_FALSE;
        }
        else
        {
            acpMemCpy(sQstrPtr, ULN_PREP_INSERT_COL_C_CSTR,
                      ULN_PREP_INSERT_COL_C_CLEN);
            sQstrPtr += ULN_PREP_INSERT_COL_C_CLEN;
        }

        sDescRecIrd = ulnStmtGetIrdRec(sStmt, i);
        ACI_TEST_RAISE(sDescRecIrd == NULL, FUNC_SEQ_EXCEPTION);

        sBaseColName = ulnDescRecGetBaseColumnName(sDescRecIrd);
        ACI_TEST_RAISE(sBaseColName == NULL, FUNC_SEQ_EXCEPTION);
        sBaseColNameLen = acpCStrLen(sBaseColName, ACP_SINT32_MAX);
        ACI_TEST_RAISE(sBaseColNameLen == 0, FUNC_SEQ_EXCEPTION);
        acpMemCpy(sQstrPtr, sBaseColName, sBaseColNameLen);
        sQstrPtr += sBaseColNameLen;

        sColActCount++;
    }
    acpMemCpy(sQstrPtr, ULN_PREP_INSERT_COL_E_CSTR,
              ULN_PREP_INSERT_COL_E_CLEN);
    sQstrPtr += ULN_PREP_INSERT_COL_E_CLEN;

    acpMemCpy(sQstrPtr, ULN_PREP_INSERT_PARAM_B_CSTR,
              ULN_PREP_INSERT_PARAM_B_CLEN);
    sQstrPtr += ULN_PREP_INSERT_PARAM_B_CLEN;
    for (i = 2; i <= sColActCount; i++)
    {
        acpMemCpy(sQstrPtr, ULN_PREP_INSERT_PARAM_C_CSTR,
                  ULN_PREP_INSERT_PARAM_C_CLEN);
        sQstrPtr += ULN_PREP_INSERT_PARAM_C_CLEN;
    }
    acpMemCpy(sQstrPtr, ULN_PREP_INSERT_PARAM_E_CSTR,
              ULN_PREP_INSERT_PARAM_E_CLEN);
    sQstrPtr += ULN_PREP_INSERT_PARAM_E_CLEN;
    *sQstrPtr = '\0';

    sStmt->mQstrForInsUpdLen = (sQstrPtr - sStmt->mQstrForInsUpd);

    return ACI_SUCCESS;

    ACI_EXCEPTION(FUNC_SEQ_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION(NOT_ENOUGH_MEM_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnPrepBuildUpdateQstr");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * 원본 ResultSet의 ColumnInfo를 저장해둔다.
 *
 * @param[in] aFnContext function context
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepBackupColumnInfo(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    /* Error 확인을 위한 쿼리로부터 얻은 컬럼 정보를 저장해둔다.
     * 이 때, 필요한 정보만 복사해두는게 더 번거로우니 Ird를 통째로 뗀다.
     * 여기에는 _PROWID 정보도 포함되어있으므로 이를 빼두면 좋지만,
     * Ird는 realloc으로 할당하기 때문에 걍 놔두는게 더 낫다. */

    if (sStmt->mIrd4KeysetDriven != NULL)
    {
        ACI_TEST_RAISE(ulnDescDestroy(sStmt->mIrd4KeysetDriven) != ACI_SUCCESS,
                       LABEL_MEM_MAN_ERR);
    }

    sStmt->mIrd4KeysetDriven = sStmt->mAttrIrd;

    sStmt->mAttrIrd = NULL;
    ACI_TEST(ulnDescCreate((ulnObject *)sStmt,
                           &sStmt->mAttrIrd,
                           ULN_DESC_TYPE_IRD,
                           ULN_DESC_ALLOCTYPE_IMPLICIT) != ACI_SUCCESS);

    ulnDescInitialize(sStmt->mAttrIrd, (ulnObject *)sStmt);
    ulnDescInitializeUserPart(sStmt->mAttrIrd);
    acpMemCpy(&(sStmt->mAttrIrd->mHeader), &(sStmt->mIrd4KeysetDriven->mHeader), ACI_SIZEOF(ulnDescHeader));

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnPrepBuildColNames");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * APD가 없는(SQLBindParameter를 하지 않은것으로 추정되는) 파라미터의 IPD를 제거한다.
 * 핸들을 재사용 할 경우, 새로 Prepare 한 쿼리에 맞는 IPD 정보를 생성하기 위한 준비작업.
 *
 * @param[in] aStmt Statement Handle
 */
static void ulnStmtRemoveAllUnboundIpd(ulnStmt *aStmt)
{
    ulnDescRec   *sDescRecIpd = NULL;
    ulnDescRec   *sDescRecApd = NULL;
    acp_uint32_t  sHighestBoundIndex = ulnDescGetHighestBoundIndex(aStmt->mAttrIpd);
    acp_uint32_t  i = 0;
    ACI_RC        sRC = ACI_SUCCESS;

    for (i = 1; i <= sHighestBoundIndex; i++)
    {
        sDescRecIpd = ulnStmtGetIpdRec(aStmt, i);
        sDescRecApd = ulnStmtGetApdRec(aStmt, i);
        if ( (sDescRecApd == NULL) && (sDescRecIpd != NULL) )
        {
            /* BUG-44858 메모리 재활용을 위해 ACP_TRUE로 설정 */
            sRC = ulnDescRemoveDescRec(aStmt->mAttrIpd, sDescRecIpd, ACP_TRUE);
            ACE_ASSERT(sRC == ACI_SUCCESS);
        }
    }
}

/**
 * 실제 Prepare를 수행한다.
 *
 * @param[in] aFnContext     function context
 * @param[in] aPtContext     protocol context
 * @param[in] aStatementText statement text
 * @param[in] aTextLength    length of statement text (octet length)
 * @param[in] aPrepareMode   prepare mode. CMP_DB_PREPARE_MODE_EXEC_DIRECT or CMP_DB_PREPARE_MODE_EXEC_PREPARE
 *
 * @return ACI_SUCCESS if successful, or ACI_FAILURE otherwise
 */
ACI_RC ulnPrepareCore(ulnFnContext *aFnContext,
                      ulnPtContext *aPtContext,
                      acp_char_t   *aStatementText,
                      acp_sint32_t  aTextLength,
                      acp_uint8_t   aPrepareMode)
{
    ulnDbc       *sDbc;
    ulnStmt      *sStmt;
    ulnDesc      *sDescIrd;
    ulnEscape     sEscape;
    ulnCharSet    sCharSet;
    acp_sint32_t  sState = 0;

    /* PROJ-1789 Updatable Scrollable Cursor */
    acp_char_t            *sPrepareText;
    acp_sint32_t           sPrepareTextLen;

    acp_bool_t             sCanRetry = ACP_TRUE;
    ulnPrepareReplaceFunc *sPrepareReplaceFunc;
    ACI_RC                 sRC;
    acp_uint8_t            sPrepareModeExec;
    acp_uint8_t            sPrepareModeAnalyze;

    /* PROJ-1721 Name-based Binding */
    sPrepareModeExec    = aPrepareMode & CMP_DB_PREPARE_MODE_EXEC_MASK;
    sPrepareModeAnalyze = aPrepareMode & CMP_DB_PREPARE_MODE_ANALYZE_MASK;

    ulnEscapeInitialize(&sEscape);
    sState = 1;

    ulnCharSetInitialize(&sCharSet);
    sState = 2;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sStmt = aFnContext->mHandle.mStmt;

    // BUG-17592
    sStmt->mResultType = ULN_RESULT_TYPE_UNKNOWN;

    // BUG-17807
    ulnStmtSetStatementType(sStmt, 0);

    ulnStmtResetTableNameForUpdate(sStmt);

    /* Statement Text 의 길이를 계산한다. */
    if (aTextLength == SQL_NTS)
    {
        aTextLength = acpCStrLen(aStatementText, ACP_SINT32_MAX);
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    ulnStmtResetQstrForDelete(sStmt);
    sStmt->mIsSelect = ulnPrepIsSelect(aFnContext, aStatementText, aTextLength);
    if (sStmt->mIsSelect== ACP_TRUE)
    {
        /* PROJ-1381 Fetch Across Commit
         * SELECT이면 commit mode와 Holdability를 확인한다.
         * HOLD ON은 FETCH가 아니면 무시하고, autocommit일 때는 쓸 수 없다. */
        if ((sDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_ON)
         && (ulnCursorGetHoldability(&sStmt->mCursor) == SQL_CURSOR_HOLD_ON))
        {
            ulnCursorSetHoldability(&sStmt->mCursor, SQL_CURSOR_HOLD_OFF);

            ulnError(aFnContext, ulERR_IGNORE_OPTION_VALUE_CHANGED,
                     "SQL_ATTR_CURSOR_HOLD changed to SQL_CURSOR_HOLD_OFF");
        }

        ulnPrepPreDowngrade(aFnContext);

        if (ulnStmtGetAttrCursorType(sStmt) == SQL_CURSOR_KEYSET_DRIVEN)
        {
            sRC = ulnStmtEnsureAllocPrepareTextBuf(sStmt, aTextLength * 2 + 2);
            ACI_TEST_RAISE(sRC != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

            ACI_TEST_RAISE(ulnPrepBuildSelectForChkErr(aFnContext,
                                                       sStmt->mPrepareTextBuf,
                                                       sStmt->mPrepareTextBufMaxLen,
                                                       &sPrepareTextLen,
                                                       aStatementText,
                                                       aTextLength)
                           != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
            sPrepareText = sStmt->mPrepareTextBuf;
        }
        else
        {
            sPrepareText = aStatementText;
            sPrepareTextLen = aTextLength;
        }
    }
    else
    {
        sPrepareText = aStatementText;
        sPrepareTextLen = aTextLength;
    }

    ACI_EXCEPTION_CONT(RetryForPostDowngrade);

    /* PROJ-1721 Name-based Binding */
    if (sPrepareModeAnalyze == CMP_DB_PREPARE_MODE_ANALYZE_ON)
    {
        if (sStmt->mAnalyzeStmt == NULL)
        {
            ACI_TEST_RAISE(ulnAnalyzeStmtCreate(&sStmt->mAnalyzeStmt,
                                                aStatementText,
                                                aTextLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEM);
        }
        else
        {
            ACI_TEST_RAISE(ulnAnalyzeStmtReInit(&sStmt->mAnalyzeStmt,
                                                aStatementText,
                                                aTextLength) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEM);
        }
    }
    else
    {
        /* Nothing */
    }

    /*
     * statement text 관련 처리
     */
    sPrepareReplaceFunc = ulnPrepareReplaceMap[sDbc->mNlsNcharLiteralReplace];
    ACI_TEST(sPrepareReplaceFunc(aFnContext,
                                 &sEscape,
                                 &sCharSet,
                                 sPrepareText,
                                 sPrepareTextLen) != ACI_SUCCESS);

    /*
     * BUGBUG : 아래의 것들을 Prepare result 를 받았을 때 하면 보다 깔끔할텐데...
     * stmt 의 IRD 를 초기화한다.
     */
    sDescIrd = ulnStmtGetIrd(sStmt);
    ACI_TEST_RAISE(sDescIrd == NULL, LABEL_MEM_MAN_ERR);
    ACI_TEST_RAISE(ulnDescRollBackToInitial(sDescIrd) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
    ACI_TEST_RAISE(ulnDescInitialize(sDescIrd, (ulnObject *)sStmt) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    /*
     * -----------------------
     * 캐시 초기화
     * -----------------------
     */

    if (sStmt->mCache == NULL)
    {
        ACI_TEST_RAISE(ulnStmtCreateCache(sStmt) != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);
    }

    ACI_TEST( ulnCacheInitialize(sStmt->mCache) != ACI_SUCCESS );

    /* 
     * PROJ-2047 Strengthening LOB - LOBCACHE
     */
    if (sStmt->mLobCache == NULL)
    {
        ACI_TEST_RAISE(ulnLobCacheCreate(&sStmt->mLobCache) != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);
    }
    else
    {
        /* Nothing */
    }

    /*
     * stmt 의 mParameterCount 와 mResultSetCount 를 초기화한다.
     */
    ulnStmtSetParamCount(sStmt, 0);
    ulnStmtSetResultSetCount(sStmt, 0);

    ulnStmtSetCurrentResultSetID(sStmt, 0);

    /*
     * prepare 를 하게 되면, 서버에 있던 바인드 정보가 싸그리 다 날아가 버리므로
     * stmt 에 딸려 있는 모든 descriptor 의 mBindInfo->mIsSent 를 ACP_FALSE 로 잡아 주어서
     * 다시 한번 바인드 정보가 전송되도록 해야 한다.
     */
    ulnStmtClearBindInfoSentFlagAll(sStmt);
    ulnStmtSetBuildBindInfo(sStmt, ACP_TRUE);
    ulnStmtSetExecutedParamSetMaxSize(sStmt, 0);  /* BUG-42096 */

    ulnStmtResetPD(sStmt);

    /*
     * PREPARE REQ 쓰기
     */
    /* PROJ-2177 User Interface - Cancel
     * ExecDirect때는 StmtID를 모르면 Cancel 할 수 없다.
     * 이 때는 CID로 Prepare해서, CID로 Cancel 할 수 있도록 한다. */
    if ((sPrepareModeExec == CMP_DB_PREPARE_MODE_EXEC_DIRECT)
     && (sStmt->mStatementID == ULN_STMT_ID_NONE))
    {
        /* StmtCID 생성에 실패해서는 안된다. */
        ACE_ASSERT(sStmt->mStmtCID != ULN_STMT_CID_NONE);

        ACI_TEST(ulnWritePrepareByCIDREQ(aFnContext,
                                         aPtContext,
                                         ulnEscapeUnescapedSql(&sEscape),
                                         ulnEscapeUnescapedLen(&sEscape)) != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnWritePrepareREQ(aFnContext,
                                    aPtContext,
                                    ulnEscapeUnescapedSql(&sEscape),
                                    ulnEscapeUnescapedLen(&sEscape),
                                    sPrepareModeExec) != ACI_SUCCESS);
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    if ( (sCanRetry == ACP_TRUE)
      && (sStmt->mIsSelect == ACP_TRUE)
      && (ulnStmtGetAttrCursorType(sStmt) == SQL_CURSOR_KEYSET_DRIVEN) )
    {
        ACI_TEST(ulnWriteColumnInfoGetREQ(aFnContext, aPtContext, 0) != ACI_SUCCESS);

        /* flush and read for check error */
        sRC = ulnFlushAndReadProtocol(aFnContext, aPtContext,
                                      sDbc->mConnTimeoutValue);
        if (sRC != ACI_SUCCESS)
        {
            /* 변환된 쿼리를 수행했을 때 받았던 에러를 날린다. */
            ulnDiagRecRemoveAll(&sStmt->mObj);

            ulnPrepPostDowngrade(aFnContext);

            /* Prepare할 쿼리문을 downgrade된 커서 속성에 맞게 다시 설정.
             * 현재는 늘 static으로 바뀌므로, 늘 원본 쿼리문을 설정한다. */
            sPrepareText = aStatementText;
            sPrepareTextLen = aTextLength;

            sCanRetry = ACP_FALSE;
            ACI_RAISE(RetryForPostDowngrade);
        }
        else
        {
            /* keyset-driven이면 컬럼 정보를 미리 받아둬야 한다.
             * 쿼리문이 바뀌므로 나중에는 정보를 제대로 얻을 수 없다. */
            ACI_TEST(ulnPrepBackupColumnInfo(aFnContext) != ACI_SUCCESS);

            ACI_TEST_RAISE(ulnPrepBuildSelectBaseForRowset(aFnContext,
                                                           aStatementText,
                                                           aTextLength)
                           != ACI_SUCCESS, LABEL_MEM_MAN_ERR);

            ACI_TEST_RAISE(ulnPrepBuildSelectForKeyset(aFnContext,
                                                       sStmt->mPrepareTextBuf,
                                                       sStmt->mPrepareTextBufMaxLen,
                                                       &sPrepareTextLen,
                                                       aStatementText,
                                                       aTextLength)
                           != ACI_SUCCESS, LABEL_MEM_MAN_ERR);

            ACI_TEST(sPrepareReplaceFunc(aFnContext, &sEscape, &sCharSet,
                                         sStmt->mPrepareTextBuf,
                                         sPrepareTextLen)
                     != ACI_SUCCESS);

            ACI_TEST(ulnWritePrepareREQ(aFnContext, aPtContext,
                                        ulnEscapeUnescapedSql(&sEscape),
                                        ulnEscapeUnescapedLen(&sEscape),
                                        sPrepareModeExec) != ACI_SUCCESS);

            if (sStmt->mKeyset == NULL)
            {
                ACI_TEST_RAISE(ulnStmtCreateKeyset(sStmt) != ACI_SUCCESS,
                               LABEL_NOT_ENOUGH_MEM);
            }
            ACI_TEST_RAISE(ulnKeysetInitialize(sStmt->mKeyset) != ACI_SUCCESS,
                           LABEL_MEM_MAN_ERR);

            /* target 절만 줄어드는거기 때문에 성공 여부를 확인할 필요 없다.
             * FlushAndRead는 밑에서 Prepare Mode에 따라 선택적으로 한다. */
        }
    }

    /*
     * BINDINFO GET REQ
     *
     * Note : prepare 할 때마다 BINDINFO GET REQ 해야 한다.
     *
     * Note : result set 의 column 갯수가 0 이상인 것을 체크하지 않고
     *        0 이라도 보내는 이유는, io transaction 을 줄이기 위한 편법 중의 하나이다.
     */
    ACI_TEST(ulnWriteColumnInfoGetREQ(aFnContext, aPtContext, 0) != ACI_SUCCESS);

    /* BUG-44858 */
    ulnStmtRemoveAllUnboundIpd(sStmt);
    if (ulnStmtGetAttrPrepareWithDescribeParam(sStmt) == ACP_TRUE)
    {
        ACI_TEST(ulnWriteParamInfoGetREQ(aFnContext, aPtContext, 0) != ACI_SUCCESS);
    }
    else
    {
        /* A obsolete convention */
    }

    /*
     * 패킷 전송 및 RES 기다리기
     *
     * PROJ-1891 Deferred Prepare
     * It will be sent just in case DeferredPrepare is disabled. 
     */
    if (sPrepareModeExec == CMP_DB_PREPARE_MODE_EXEC_PREPARE)
    {
        if (sStmt->mAttrDeferredPrepare == ULN_CONN_DEFERRED_PREPARE_OFF)
        {
            ACI_TEST(ulnFlushAndReadProtocol(aFnContext,
                                         aPtContext,
                                         sDbc->mConnTimeoutValue)
                 != ACI_SUCCESS);
        }
        else
        {
            /* Although the prepare mode is CMP_DB_PREPARE_MODE_EXEC_PREPARE, 
             * when it comes to the bulk operations, they act like DirectExec function.
             * Therefore, the function id must be checked before setting a state function */
            if (sStmt->mObj.mExecFuncID == ULN_FID_PREPARE)
            {
                sStmt->mDeferredPrepareStateFunc = *aFnContext->mStateFunc;
            }
        }
    }

    sState = 1;
    ulnCharSetFinalize(&sCharSet);

    sState = 0;
    ulnEscapeFinalize(&sEscape);

    if (ulnStmtIsSetDeferredQstr(sStmt) == ACP_TRUE)
    {
        ulnStmtClearDeferredQstr(sStmt);
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "PrepareCore");
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "PrepareCore");
    }

    ACI_EXCEPTION_END;

    switch(sState)
    {
        case 2:
            ulnCharSetFinalize(&sCharSet);
        case 1:
            ulnEscapeFinalize(&sEscape);
        default:
            break;
    }

    return ACI_FAILURE;
}


ACI_RC ulnPrepareDefer(ulnFnContext *aFnContext,
                       ulnStmt      *aStmt,
                       acp_char_t   *aStatementText,
                       acp_sint32_t  aTextLength,
                       acp_uint8_t   aPrepareMode)
{
    if (aTextLength == SQL_NTS)
    {
        aTextLength = acpCStrLen(aStatementText, ACP_SINT32_MAX);
    }

    ACI_TEST_RAISE( ulnStmtEnsureAllocDeferredQstr(aStmt, aTextLength + 1)
                    != ACI_SUCCESS,
                    LABEL_NOT_ENOUGH_MEM );

    acpMemCpy(aStmt->mDeferredQstr, aStatementText, aTextLength);
    aStmt->mDeferredQstr[aTextLength] = '\0';
    aStmt->mDeferredQstrLen = aTextLength;
    aStmt->mDeferredPrepareMode = aPrepareMode;

    aStmt->mDeferredPrepareStateFunc = *aFnContext->mStateFunc;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnPrepareDefer");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnPrepareDeferComplete(ulnFnContext *aFnContext,
                               acp_bool_t    aFlush)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    ACI_TEST( ulnPrepareCore(aFnContext,
                             &(sStmt->mParentDbc->mPtContext),
                             sStmt->mDeferredQstr,
                             sStmt->mDeferredQstrLen,
                             sStmt->mDeferredPrepareMode)
              != ACI_SUCCESS );

    if (aFlush == ACP_TRUE)
    {
        ACI_TEST( ulnFinalizeProtocolContext(aFnContext,
                                             &(sStmt->mParentDbc->mPtContext))
                  != ACI_SUCCESS );

        ulnUpdateDeferredState(aFnContext, sStmt);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* 
 * PROJ-1721 Name-based Binding
 *
 * @aAnalyzeText : NULL이 아니면 aStatementText를 분석한다.
 */
SQLRETURN ulnPrepare(ulnStmt      *aStmt,
                     acp_char_t   *aStatementText,
                     acp_sint32_t  aTextLength,
                     acp_char_t   *aAnalyzeText)
{
    acp_bool_t    sNeedExit = ACP_FALSE;
    acp_bool_t    sNeedFinPtContext = ACP_FALSE;
    ulnFnContext  sFnContext;

    acp_uint8_t   sPrepareMode = CMP_DB_PREPARE_MODE_EXEC_PREPARE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PREPARE, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    /* 넘겨진 객체의 validity 체크를 포함한 ODBC 3.0 에서 정의하는 각종 Error 체크 */
    ACI_TEST(ulnPrepCheckArgs(&sFnContext, aStatementText, aTextLength) != ACI_SUCCESS);

    //fix BUG-17722
    ACI_TEST( ulnInitializeProtocolContext(&sFnContext,
                                           &(aStmt->mParentDbc->mPtContext),
                                           &(aStmt->mParentDbc->mSession))
              != ACI_SUCCESS );

    sNeedFinPtContext = ACP_TRUE;

    /* PROJ-1721 Name-based Binding */
    if (aAnalyzeText != NULL)
    {
        sPrepareMode |= CMP_DB_PREPARE_MODE_ANALYZE_ON;
    }
    else
    {
        /* Nothing */
    }

    if (aStmt->mAttrDeferredPrepare == ULN_CONN_DEFERRED_PREPARE_ON)
    {
        ACI_TEST( ulnPrepareDefer(&sFnContext,
                                  aStmt,
                                  aStatementText,
                                  aTextLength,
                                  sPrepareMode)
                  != ACI_SUCCESS );

        sNeedFinPtContext = ACP_FALSE;
    }
    else
    {
        ACI_TEST( ulnPrepareCore(&sFnContext,
                                 &(aStmt->mParentDbc->mPtContext),
                                 aStatementText,
                                 aTextLength,
                                 sPrepareMode)
                  != ACI_SUCCESS );

        sNeedFinPtContext = ACP_FALSE;
        ACI_TEST( ulnFinalizeProtocolContext(&sFnContext,
                                             &(aStmt->mParentDbc->mPtContext))
                  != ACI_SUCCESS );
    }

    sNeedExit = ACP_FALSE;
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s|\n [%s]", "ulnPrepare", aStatementText);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if (sNeedFinPtContext == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext));
    }

    if (sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail\n [%s]", "ulnPrepare", aStatementText);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
