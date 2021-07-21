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
#include <ulnCatalogFunctions.h>


static ACI_RC ulnStatisticsCreateQueryString(ulnFnContext *aFnContext,
                                             acp_char_t   *aUserName,
                                             acp_sint16_t  aUserNameLength,
                                             acp_char_t   *aTableName,
                                             acp_sint16_t  aTableNameLength,
                                             acp_uint16_t  aUnique,
                                             acp_char_t   *aQueryString,
                                             acp_uint32_t  aQueryStringLength)
{
   const acp_char_t *gColumnName[2][4] =
   {
       { "TABLE_QUALIFIER", "TABLE_OWNER", "SEQ_IN_INDEX"    ,  "COLLATION"   }, /* ODBC 2.0 naming */
       { "TABLE_CAT"      , "TABLE_SCHEM", "ORDINAL_POSITION",  "ASC_OR_DESC" }  /* ODBC 3.0 naming */
   };


    acp_sint32_t sSize;
    acp_bool_t   sIsODBC3 = ACP_FALSE;
    acp_rc_t     sRet;

    /* set index of ODBC x.0 */
    if( aFnContext->mHandle.mStmt->mParentDbc->mOdbcVersion == SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    /* By ODBC3 that TableName is not NULL */
    ACI_TEST_RAISE(aTableName == NULL, ERR_HY009);

    sRet = acpSnprintf(aQueryString, aQueryStringLength,
                       "select "
                       "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // BUG-17786
                       "e.user_name as %s,"
                       "d.table_name as TABLE_NAME,"
                       "cast( decode(b.IS_UNIQUE,'T',0,1) as SMALLINT) as NON_UNIQUE,"
                       "VARCHAR'' as INDEX_QUALIFIER,"   // To fix BUG-18222
                       "b.INDEX_NAME as INDEX_NAME,"
                       "3  as TYPE,"
                       "cast( a.INDEX_COL_ORDER+1 as SMALLINT ) as %s,"
                       "decode( c.IS_HIDDEN, 'T', case2( octet_length( c.DEFAULT_VAL ) > %"ACI_INT32_FMT","
                       "                                     VARCHAR'...', c.DEFAULT_VAL ),"
                       "        c.COLUMN_NAME ) as COLUMN_NAME," /* PROJ-1090 Function-based Index */
                       "a.SORT_ORDER as %s,"
                       "INTEGER'' as CARDINALITY,"       // To fix BUG-18222
                       "INTEGER'' as PAGES,"             // To fix BUG-18222
                       "VARCHAR'' as FILTER_CONDITION,"  // To fix BUG-18222
                       "b.index_type as INDEX_TYPE,"
                       "b.index_id "
                       "from system_.SYS_INDEX_COLUMNS_ a,system_.SYS_INDICES_ b,"
                       "system_.SYS_COLUMNS_ c,system_.SYS_TABLES_ d,"
                       "system_.SYS_USERS_ e "
                       "where a.INDEX_ID = b.INDEX_ID %s "
                       "and a.COLUMN_ID = c.COLUMN_ID "
                       "and b.table_id = d.table_id "
                       "and d.user_id = e.user_id ",
                       gColumnName[sIsODBC3][0],
                       gColumnName[sIsODBC3][1],
                       gColumnName[sIsODBC3][2],
                       MAX_NAME_LEN,                     /* PROJ-1090 Function-based Index */
                       gColumnName[sIsODBC3][3],
                       (aUnique == SQL_INDEX_UNIQUE) ? "and b.IS_UNIQUE='T'" : "" );

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);

    sSize = acpCStrLen(aQueryString, aQueryStringLength);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryString,
                                     aQueryStringLength,
                                     sSize,
                                     "  and e.user_name LIKE \'%s\'",
                                     aUserName,
                                     aUserNameLength);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringLength, ERR_HY001);


    // bug-25905: conn nls not applied to client lang module
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryString,
                                     aQueryStringLength,
                                     sSize,
                                     " and d.table_name=\'%s\' ORDER BY 4,6,8",
                                     aTableName,
                                     aTableNameLength);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringLength, ERR_HY001);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN2)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aUserNameLength);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN3)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aTableNameLength);
    }

    ACI_EXCEPTION(ERR_HY009);
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }


    ACI_EXCEPTION(ERR_HY001);
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "Not enougth buffer's space for query.");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnStatistics(ulnStmt      *aStmt,
                        acp_char_t   *aCatalogName,
                        acp_sint16_t  aNameLength1,
                        acp_char_t   *aSchemaName,
                        acp_sint16_t  aNameLength2,
                        acp_char_t   *aTableName,
                        acp_sint16_t  aNameLength3,
                        acp_uint16_t  aUnique,
                        acp_uint16_t  aReserved)
{
    acp_bool_t sNeedExit = ACP_FALSE;
    acp_bool_t sNeedFinPtContext = ACP_FALSE;

    ulnFnContext sFnContext;
    acp_char_t   sQueryString[ULN_CATALOG_QUERY_STR_BUF_SIZE];
    ulnDbc      *sParentDbc = (aStmt != NULL) ? aStmt->mParentDbc : NULL;  /* BUG-46885 */

    ACP_UNUSED(aCatalogName);
    ACP_UNUSED(aNameLength1);
    ACP_UNUSED(aReserved);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_STATISTICS, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)sParentDbc) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;

    /*
     * BUGBUG : 넘어온 인자들이 적법한지 체크해야 함.
     */

    ACI_TEST(ulnStatisticsCreateQueryString(&sFnContext,
                                            aSchemaName,
                                            aNameLength2,
                                            aTableName,
                                            aNameLength3,
                                            aUnique,
                                            sQueryString,
                                            ACI_SIZEOF(sQueryString)) != ACI_SUCCESS);

    /*
     * Protocol Context 초기화
     */
    // fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);

    sNeedFinPtContext = ACP_TRUE;

    /*
     * Prepare Phase
     */
    // fix BUG-17722
    ACI_TEST(ulnPrepareCore(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext),
                            sQueryString,
                            acpCStrLen(sQueryString, ULN_CATALOG_QUERY_STR_BUF_SIZE),
                            CMP_DB_PREPARE_MODE_EXEC_DIRECT) != ACI_SUCCESS);

    /*
     * Execute Phase
     */
    // fix BUG-17722
    ACI_TEST(ulnExecuteCore(&sFnContext,&(aStmt->mParentDbc->mPtContext))
               != ACI_SUCCESS);
    // fix BUG-17722
    ACI_TEST(ulnFlushAndReadProtocol(&sFnContext,
                                     &(aStmt->mParentDbc->mPtContext),
                                     aStmt->mParentDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /*
     * Protocol Context 정리
     */
    sNeedFinPtContext = ACP_FALSE;
    // fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * BUGBUG : 각 컬럼의 타입을 강제로 지정해 주는 코드가 cli2 에는 있었다.
     *
     *          stmt->bindings[ 4-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[ 7-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[ 8-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[11-1].forced_type = SQL_C_SLONG;
     *          stmt->bindings[12-1].forced_type = SQL_C_SLONG;
     */

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if(sNeedFinPtContext == ACP_TRUE)
    {
        // fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
