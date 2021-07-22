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
#include <ulnGetTypeInfo.h>

/*
 * ULN_SFID_65
 *  related functions :
 *      SQLColumnPrivileges, SQLColumns, SQLForeignKeys, SQLGetTypeInfo, SQLPrimaryKeys,
 *      SQLProcedureColumns, SQLProcedures, SQLSpecialColumns, SQLStatistics,
 *      SQLTablePrivileges, SQLTables, SQLGetTypeInfo
 *      >> functions that take STMT.
 *  what this SF does :
 *      --[1]
 *      C6[2]
 *  where
 *      [1] The connection was in auto-commit mode,
 *          or the data source did not begin a transaction.
 *      [2] The connection was in manual-commit mode,
 *          and the data source began a transaction.
 */
ACI_RC ulnSFID_65(ulnFnContext *aFnContext)
{
    ulnDbc *sDbc;

    if(aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        sDbc = (ulnDbc *)(aFnContext->mArgs);

        ACE_ASSERT(sDbc != NULL);

        if(sDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF)
        {
            if(ACP_TRUE)
            {
                /* [2] */
                /*
                 * BUGBUG: Should provide a method to find out whether the data source have begun
                 *         a transaction.
                 */
                ULN_OBJ_SET_STATE(sDbc, ULN_S_C6);
            }
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_08
 *  related functions :
 *      SQLColumnPrivileges, SQLColumns, SQLForeignKeys, SQLGetTypeInfo, SQLPrimaryKeys,
 *      SQLProcedureColumns, SQLProcedures, SQLSpecialColumns, SQLStatistics,
 *      SQLTablePrivileges, SQLTables, SQLGetTypeInfo
 *  related state : S1 (allocated)
 *
 * Table Entry
 *      S5 [s]
 *      S11 [x]
 * where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 */
ACI_RC ulnSFID_08(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    if(aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        /*
         * BUGBUG: still executing state
         */
        if(ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO)
        {
            /* [s] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
            ulnCursorOpen(&sStmt->mCursor);
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_09
 *  related functions :
 *      SQLColumnPrivileges, SQLColumns, SQLForeignKeys, SQLGetTypeInfo, SQLPrimaryKeys,
 *      SQLProcedureColumns, SQLProcedures, SQLSpecialColumns, SQLStatistics,
 *      SQLTablePrivileges, SQLTables, SQLGetTypeInfo
 *  related state :
 *      S2-S3 (prepared)
 *
 *      S1 [e]
 *      S5 [s]
 *      S11 [x]
 */
ACI_RC ulnSFID_09(ulnFnContext *aFnContext)
{
    if(aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        /*
         * BUGBUG: still executing state
         */
    }
    else
    {
        /*
         * Exit point
         */
        switch(ULN_FNCONTEXT_GET_RC(aFnContext))
        {
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
                /* [s] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                ulnCursorOpen(&aFnContext->mHandle.mStmt->mCursor);
                break;

            default:
                /* [e] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                break;
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_10 :
 *  related functions :
 *      SQLColumnPrivileges, SQLColumns, SQLForeignKeys, SQLGetTypeInfo, SQLPrimaryKeys,
 *      SQLProcedureColumns, SQLProcedures, SQLSpecialColumns, SQLStatistics,
 *      SQLTablePrivileges, SQLTables, SQLGetTypeInfo
 *  related state :
 *      S4 (executed)
 *
 *      S1 [e] and [1]
 *      S5 [s] and [1]
 *      S11 [x] and [1]
 *      24000[2]
 *  where
 *      [1] The current result is the last or only result, or there are no current results.
 *      [2] The current result is not the last result
 */
ACI_RC ulnSFID_10(ulnFnContext *aFnContext)
{
    if(aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        /*
         * BUGBUG: still executing state
         */

        /*
         * BUGBUG: Should check if current result is NOT the last result
         */
        if(0)
        {
            /* [2] : 24000 */

            ACI_TEST(ulnError(aFnContext, ulERR_ABORT_INVALID_CURSOR_STATE) != ACI_SUCCESS);
        }
    }
    else
    {
        /*
         * BUGBUG: To check the current result is not the last result.
         *         For now, just assume that the current result is the last or only result.
         *         Or there is no current result.
         */
        if(1)
        {
            /* [1] */
            switch(ULN_FNCONTEXT_GET_RC(aFnContext))
            {
                case SQL_SUCCESS:
                case SQL_SUCCESS_WITH_INFO:
                    /* [s] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                    ulnCursorOpen(&aFnContext->mHandle.mStmt->mCursor);
                    break;

                default:
                    /* [e] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                    break;
            }
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


static ACI_RC ulnGetTypeInfoCheckArgs(ulnFnContext *aFnContext, acp_sint16_t aDataType)
{
    switch(aDataType)
    {
        case SQL_ALL_TYPES:
        case SQL_VARBIT:
        case SQL_NIBBLE:
        case SQL_CHAR:
        case SQL_BLOB:
        case SQL_CLOB:
        case SQL_BYTE:
        case SQL_DATE:
        case SQL_DOUBLE:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR: // fix BUG-19865
        case SQL_GEOMETRY:
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_BIT:
        case SQL_TINYINT:
        case SQL_BIGINT:
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
        case SQL_WCHAR:
        case SQL_WVARCHAR:

        // fix BUG-19865
        case SQL_TIME:
        case SQL_TIMESTAMP:

        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP:

        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_YEAR_TO_MONTH:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND:
            break;

        default:
            ACI_RAISE(LABEL_INVALID_SQL_TYPE);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_SQL_TYPE)
    {
        /*
         * HY004 : Invalid SQL data type
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_SQL_TYPE, aDataType);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnGetTypeInfo(ulnStmt *aStmt, acp_sint16_t aDataType)
{
    acp_bool_t   sNeedExit = ACP_FALSE;
    acp_bool_t   sNeedFinPtContext = ACP_FALSE;

    ulnFnContext sFnContext;
    acp_char_t   sQueryString[2048];

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETTYPEINFO, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    sQueryString[0] = '\0';
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;

    /*
     * Check if the arguments are valid
     */
    ACI_TEST(ulnGetTypeInfoCheckArgs(&sFnContext, aDataType) != ACI_SUCCESS);

    /*
     * create query string
     */

    // fix BUG-30312
    // SQL_ATTR_LONGDATA_COMPAT 속성 사용시
    // SQL_LONGVARBINARY 및 SQL_LONGVARCHAR의 정보는
    // X$DATATYPE의 SQL_BLOB, SQL_CLOB의 정보를 갖고오도록 한다.

    if (ulnDbcGetLongDataCompat(aStmt->mParentDbc) == ACP_TRUE)
    {
        switch(aDataType)
        {
            case SQL_LONGVARBINARY :
                aDataType = SQL_BLOB;
                break;
            case SQL_LONGVARCHAR :
                aDataType = SQL_CLOB;
                break;
        }
    }

    // BUG-17202
    // X$DATATYPE에서
    // ODBC_DATA_TYPE과 ODBC_SQL_DATA_TYPE을
    // DATA_TYPE과 SQL_DATA_TYPE으로 변환

    acpSnprintf(sQueryString,
                ACI_SIZEOF(sQueryString),
                "SELECT "
                "  TYPE_NAME,"
                "  ODBC_DATA_TYPE AS DATA_TYPE,"
                "  COLUMN_SIZE,"
                "  LITERAL_PREFIX,"
                "  LITERAL_SUFFIX,"
                "  CREATE_PARAM AS CREATE_PARAMS,"
                "  NULLABLE,"
                "  CASE_SENSITIVE,"
                "  SEARCHABLE,"
                "  UNSIGNED_ATTRIBUTE,"
                "  FIXED_PREC_SCALE,"
                "  AUTO_UNIQUE_VALUE,"
                "  LOCAL_TYPE_NAME,"
                "  MINIMUM_SCALE,"
                "  MAXIMUM_SCALE,"
                "  ODBC_SQL_DATA_TYPE AS SQL_DATA_TYPE,"
                "  SQL_DATETIME_SUB,"
                "  NUM_PREC_RADIX,"
                "  INTERVAL_PRECISION "
                "FROM X$DATATYPE");

    if (aDataType != SQL_ALL_TYPES)
    {
        // fix BUG-31061
        // DATE 관련 TYPE은 모두 DATE로 서버로 전송한다.
        switch(aDataType)
        {
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
            case SQL_TYPE_TIMESTAMP:
                aDataType = SQL_DATE;
                break;
        }

        aciVaAppendFormat(sQueryString,
                          ACI_SIZEOF(sQueryString),
                          " WHERE DATA_TYPE=%"ACI_INT32_FMT,
                          aDataType);
    }
    else
    {
        aciVaAppendFormat(sQueryString,
                          ACI_SIZEOF(sQueryString),
                          " ORDER BY ODBC_DATA_TYPE, TYPE_NAME");
    }

    /*
     * Protocol Context 초기화
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession))
             != ACI_SUCCESS);

    sNeedFinPtContext = ACP_TRUE;

    /*
     * Prepare Phase
     */
    //fix BUG-17722
    ACI_TEST(ulnPrepareCore(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext),
                            sQueryString,
                            SQL_NTS,
                            CMP_DB_PREPARE_MODE_EXEC_DIRECT) != ACI_SUCCESS);

    /*
     * Execute Phase
     */
    //fix BUG-17722
    ACI_TEST(ulnExecuteCore(&sFnContext,&(aStmt->mParentDbc->mPtContext))
             != ACI_SUCCESS);

    ACI_TEST(ulnFlushAndReadProtocol(&sFnContext,
                                     &(aStmt->mParentDbc->mPtContext),
                                     aStmt->mParentDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /*
     * Protocol Context 정리
     */
    sNeedFinPtContext = ACP_FALSE;
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                                        &(aStmt->mParentDbc->mPtContext))
             != ACI_SUCCESS);

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| [%2"ACI_INT32_FMT"]\n [%s]",
            "ulnGetTypeInfo", aDataType, sQueryString);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if(sNeedFinPtContext == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [%2"ACI_INT32_FMT"] fail\n [%s]",
            "ulnGetTypeInfo", aDataType, sQueryString);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

