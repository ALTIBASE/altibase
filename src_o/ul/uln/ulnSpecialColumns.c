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

static ACI_RC ulnSpColCreateQueryString(ulnFnContext *aFnContext,
                                        acp_uint16_t  aIdentifierType,
                                        acp_char_t   *aSchemaName,
                                        acp_sint16_t  aNameLength2,
                                        acp_char_t   *aTableName,
                                        acp_sint16_t  aNameLength3,
                                        acp_char_t   *aQueryBuffer,
                                        acp_uint32_t  aQueryBufferSize)
{
    acp_sint32_t sSize;
    acp_char_t   sUserName[MAX_NAME_LEN];
    acp_char_t   sTableName[MAX_NAME_LEN];
    acp_rc_t     sRet;

    /* Default ODBC2 naming */
    const acp_char_t* sColumnName[2][3] =
        {
            { "PRECISION"  , "LENGTH",        "SCALE"          },
            { "COLUMN_SIZE", "BUFFER_LENGTH", "DECIMAL_DIGITS" }
        };
    acp_bool_t   sIsODBC3 = ACP_FALSE;
    // bug-25905: conn nls not applied to client lang module
    ulnDbc* sDbc = aFnContext->mHandle.mStmt->mParentDbc;

    if(aFnContext->mHandle.mStmt->mParentDbc->mOdbcVersion ==  SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    // ACI_TEST_RAISE(aSchemaName == NULL, ERR_HY009);
    // ACI_TEST_RAISE(aTableName  == NULL, ERR_HY009);

    // bug-25905: conn nls not applied to client lang module
    // aMtlModule 인자 추가
    ACI_TEST_RAISE(ulnMakeNullTermNameInSQL(sDbc->mClientCharsetLangModule,
                                            sUserName,
                                            ACI_SIZEOF(sUserName),
                                            aSchemaName,
                                            aNameLength2,
                                            NULL)
                   != ACI_SUCCESS, LABEL_INVALID_BUF_LEN2);

    // bug-25905: conn nls not applied to client lang module
    ACI_TEST_RAISE(ulnMakeNullTermNameInSQL(sDbc->mClientCharsetLangModule,
                                            sTableName,
                                            ACI_SIZEOF(sTableName),
                                            aTableName,
                                            aNameLength3,
                                            NULL)
                   != ACI_SUCCESS, LABEL_INVALID_BUF_LEN3);

    sRet = acpSnprintf(aQueryBuffer, aQueryBufferSize,
                       "SELECT 2 "     " as SCOPE,"
                       "c.column_name"" as COLUMN_NAME,"
                       "t.sql_data_type as DATA_TYPE,"
                       "t.type_name as TYPE_NAME,"
                       "cast( decode(c.precision, 0, "
                       "             decode(c.data_type, "     // fix BUG-26817 문자열의 pricision이 0이면 0 반환
                       "                    1, c.precision, "
                       "                    12, c.precision, "
                       "                    -8, c.precision, "
                       "                    -9, c.precision, "
                       "                    60, c.precision, "
                       "                    61, c.precision, "
                       "                    t.COLUMN_SIZE), "
                       "             c.precision) as INTEGER )"
                       " as %s,"
                       "c.precision"   " as %s,"
                       "cast( c.scale as SMALLINT) as %s,"
                       "1 as PSEUDO_COLUMN"
                       " from "
                       "system_.SYS_CONSTRAINT_COLUMNS_ a,"
                       "system_.SYS_CONSTRAINTS_ b,system_.SYS_COLUMNS_ c,"
                       "system_.SYS_TABLES_ d,system_.SYS_USERS_ e,"
                       "X$DATATYPE t"
                       " WHERE  1 = %d" // SQL_BEST_ROWID = aIdentifierType
                       " and a.CONSTRAINT_ID=b.CONSTRAINT_ID"
                       " and b.constraint_type=3"
                       " and a.COLUMN_ID=c.COLUMN_ID"
                       " and b.table_id=d.table_id"
                       " and d.user_id=e.user_id",
                       sColumnName[sIsODBC3][0],
                       sColumnName[sIsODBC3][1],
                       sColumnName[sIsODBC3][2], aIdentifierType);

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);

    sSize = acpCStrLen(aQueryBuffer, aQueryBufferSize);

    if (acpCStrLen(sUserName, MAX_NAME_LEN) > 0)
    {
        // bug-25905: conn nls not applied to client lang module
        // aFnContext 인자 추가
        sSize = ulnAppendFormatParameter(aFnContext,
                                         aQueryBuffer,
                                         aQueryBufferSize,
                                         sSize,
                                         " AND e.user_name  LIKE '%s'", aSchemaName, aNameLength2);
        ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
    }

    // bug-25905: conn nls not applied to client lang module
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryBuffer,
                                     aQueryBufferSize,
                                     sSize,
                                     " AND d.table_name  LIKE '%s'"
                                     " and c.data_type=t.data_type"
                                     " ORDER BY 4",
                                     aTableName, aNameLength3);
    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN2)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength2);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN3)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength3);
    }

    ACI_EXCEPTION(ERR_HY001);
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "Not enougth buffer's space for query.");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnSpecialColumns(ulnStmt      *aStmt,
                            acp_uint16_t  aIdentifierType,
                            acp_char_t   *aCatalogName,
                            acp_sint16_t  aNameLength1,
                            acp_char_t   *aSchemaName,
                            acp_sint16_t  aNameLength2,
                            acp_char_t   *aTableName,
                            acp_sint16_t  aNameLength3,
                            acp_uint16_t  aScope,
                            acp_uint16_t  aNullable)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext sFnContext;
    acp_char_t   sQueryStringBuffer[ULN_CATALOG_QUERY_STR_BUF_SIZE];
    ulnDbc      *sParentDbc = (aStmt != NULL) ? aStmt->mParentDbc : NULL;  /* BUG-46885 */

    ACP_UNUSED(aCatalogName);
    ACP_UNUSED(aNameLength1);
    ACP_UNUSED(aScope);
    ACP_UNUSED(aNullable);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SPECIALCOLUMNS, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)sParentDbc) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * BUGBUG : Argument validity checking 을 수행해야 한다.
     */

    ACI_TEST(ulnSpColCreateQueryString(&sFnContext,
                                       aIdentifierType,
                                       aSchemaName,
                                       aNameLength2,
                                       aTableName,
                                       aNameLength3,
                                       sQueryStringBuffer,
                                       ACI_SIZEOF(sQueryStringBuffer)) != ACI_SUCCESS);

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
                            sQueryStringBuffer,
                            acpCStrLen(sQueryStringBuffer, ACI_SIZEOF(sQueryStringBuffer)),
                            CMP_DB_PREPARE_MODE_EXEC_DIRECT) != ACI_SUCCESS);

    /*
     * Execute Phase
     */
    //fix BUG-17722
    ACI_TEST(ulnExecuteCore(&sFnContext, &(aStmt->mParentDbc->mPtContext))
              != ACI_SUCCESS);
    //fix BUG-17722
    ACI_TEST(ulnFlushAndReadProtocol(&sFnContext,
                                     &(aStmt->mParentDbc->mPtContext),
                                     aStmt->mParentDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /*
     * Protocol Context 정리
     */
    ULN_FLAG_DOWN(sNeedFinPtContext);
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * BUGBUG : 컬럼의 타입을 강제로 지정해 주는 코드가 cli2 에는 있었다.
     *          stmt->bindings[1-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[3-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[5-1].forced_type = SQL_C_SLONG;
     *          stmt->bindings[6-1].forced_type = SQL_C_SLONG;
     *          stmt->bindings[7-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[8-1].forced_type = SQL_C_SSHORT;
     */

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

