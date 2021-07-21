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

static ACI_RC ulnPKCreateQueryString(ulnFnContext *aFnContext,
                                     acp_char_t   *aSchemaName,
                                     acp_sint16_t  aNameLength2,
                                     acp_char_t   *aTableName,
                                     acp_sint16_t  aNameLength3,
                                     acp_char_t   *aQueryStringBuffer,
                                     acp_uint32_t  aQueryStringBufferSize)
{
    ulnDbc       *sDbc = aFnContext->mHandle.mStmt->mParentDbc;
    acp_sint32_t  sSize;
    acp_rc_t      sRet;

    const acp_char_t *sPkCatalogName[2] [2] = /* 2.0, 3.0 */
    {
        { "TABLE_QUALIFIER", "TABLE_ONWER" },
        { "TABLE_CAT",       "TABLE_SCHEM" }
    };

    acp_bool_t    sIsODBC3 = ACP_FALSE;

    if(sDbc->mOdbcVersion ==  SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    /* (DM) This argument cannot be a null pointer.*/
    ACI_TEST_RAISE( aTableName == NULL, ERR_HY009);

    /* ODBC2/3 spec Named */

    sRet = acpSnprintf(aQueryStringBuffer, aQueryStringBufferSize,
                       "SELECT "
                       "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // BUG-17786
                       "e.user_name   as %s,"
                       "d.table_name  as TABLE_NAME,"
                       "c.column_name as COLUMN_NAME,"
                       "cast( a.CONSTRAINT_COL_ORDER+1 as SMALLINT ) as KEY_SEQ,"
                       "b.CONSTRAINT_NAME as PK_NAME,"
                       "b.index_id "
                       "FROM "
                       "system_.SYS_CONSTRAINT_COLUMNS_ a,"
                       "system_.SYS_CONSTRAINTS_        b,"
                       "system_.SYS_COLUMNS_            c,"
                       "system_.SYS_TABLES_             d,"
                       "system_.SYS_USERS_              e "
                       "WHERE a.CONSTRAINT_ID   = b.CONSTRAINT_ID "
                       "  and b.constraint_type = 3 "
                       "  and a.COLUMN_ID       = c.COLUMN_ID "
                       "  and b.table_id        = d.table_id "
                       "  and d.user_id         = e.user_id ",
                       sPkCatalogName[0][sIsODBC3],
                       sPkCatalogName[0][sIsODBC3]);

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);

    sSize = acpCStrLen(aQueryStringBuffer, aQueryStringBufferSize);

    /* Is There Schema setted */
    if( aSchemaName != NULL )
    {
        // bug-25905: conn nls not applied to client lang module
        // aFnContext 인자 추가
        sSize = ulnAppendFormatParameter(aFnContext,
                                         aQueryStringBuffer,
                                         aQueryStringBufferSize,
                                         sSize,
                                         " and e.user_name='%s' ",
                                         aSchemaName,
                                         aNameLength2 );
        ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
        ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);
    }

    /* compulsary TableName parameter append */
    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryStringBuffer,
                                     aQueryStringBufferSize,
                                     sSize,
                                     " and d.table_name='%s' order by 2,3,5",
                                     aTableName,
                                     aNameLength3);
    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN2)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength2);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN3)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength3);
    }

    ACI_EXCEPTION(ERR_HY009)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION(ERR_HY001)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "Not enougth buffer's space for query.");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnPrimaryKeys(ulnStmt      *aStmt,
                         acp_char_t   *aCatalogName,
                         acp_sint16_t  aNameLength1,
                         acp_char_t   *aSchemaName,
                         acp_sint16_t  aNameLength2,
                         acp_char_t   *aTableName,
                         acp_sint16_t  aNameLength3)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext sFnContext;

    acp_char_t sQueryString[ULN_CATALOG_QUERY_STR_BUF_SIZE];

    ACP_UNUSED(aCatalogName);
    ACP_UNUSED(aNameLength1);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PRIMARYKEYS, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST( aStmt == NULL );          //BUG-28623 [CodeSonar]Null Pointer Dereference

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(aStmt->mParentDbc)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * BUGBUG : Argument validity checking 을 수행해야 한다.
     */

    ACI_TEST(ulnPKCreateQueryString(&sFnContext,
                                    aSchemaName,
                                    aNameLength2,
                                    aTableName,
                                    aNameLength3,
                                    sQueryString,
                                    ACI_SIZEOF(sQueryString)) != ACI_SUCCESS);

    /*
     * Protocol Context 초기화
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * Prepare Phase
     */
    //fix BUG-17722
    ACI_TEST(ulnPrepareCore(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext),
                            sQueryString,
                            acpCStrLen(sQueryString, ACI_SIZEOF(sQueryString)),
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
    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext))
               != ACI_SUCCESS);

    /*
     * Exit
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if(sNeedFinPtContext == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

