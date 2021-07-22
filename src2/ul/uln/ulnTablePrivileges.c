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


static ACI_RC ulnTblPrivCreateQueryString(ulnFnContext *aFnContext,
                                          acp_char_t   *aSchemaName,
                                          acp_sint16_t  aNameLength2,
                                          acp_char_t   *aTableName,
                                          acp_sint16_t  aNameLength3,
                                          acp_char_t   *aQueryBuffer,
                                          acp_uint32_t  aQueryBufferSize)
{
    acp_char_t       *sTempStr = NULL;
    const acp_char_t *gColumnName[2][2] =
        {
            { "TABLE_QUALIFIER", "TABLE_OWNER" }, /* ODBC 2.0 naming */
            { "TABLE_CAT"      , "TABLE_SCHEM" }  /* ODBC 3.0 naming */
        };
    acp_bool_t  sIsODBC3 = ACP_FALSE;

    acp_char_t  sUserName[MAX_NAME_LEN];
    acp_char_t  sTableName[MAX_NAME_LEN];
    // bug-25905: conn nls not applied to client lang module
    ulnDbc     *sDbc = aFnContext->mHandle.mStmt->mParentDbc;

    ACI_TEST_RAISE(aTableName == NULL, ERR_HY009);

    /* set index of ODBC x.0 */
    if( aFnContext->mHandle.mStmt->mParentDbc->mOdbcVersion == SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    // bug-25905: conn nls not applied to client lang module
    // aMtlModule 인자 추가
    ACI_TEST_RAISE(ulnMakeNullTermNameInSQL(sDbc->mClientCharsetLangModule,
                                            sUserName,
                                            ACI_SIZEOF(sUserName),
                                            aSchemaName,
                                            aNameLength2,
                                            NULL) != ACI_SUCCESS,
                   LABEL_INVALID_BUF_LEN2);

    // bug-25905: conn nls not applied to client lang module
    ACI_TEST_RAISE(ulnMakeNullTermNameInSQL(sDbc->mClientCharsetLangModule,
                                            sTableName,
                                            ACI_SIZEOF(sTableName),
                                            aTableName,
                                            aNameLength3,
                                            NULL) != ACI_SUCCESS,
                   LABEL_INVALID_BUF_LEN3);

    acpSnprintf(aQueryBuffer, aQueryBufferSize,
                "select "
                "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // BUG-17786
                "u_t.user_name %s,"
                "u_t.table_name TABLE_NAME,"
                "u.user_name GRANTOR,"
                "u2.user_name GRANTEE,"
                "priv.priv_name PRIVILEGE,"
                "decode(obj.with_grant_option,1,'YES',0,'NO') IS_GRANTABLE "
                "from "
                "(select t.table_id table_id, t.table_name table_name, "
                "u.user_name user_name  "
                "from system_.sys_users_ u, system_.sys_tables_ t "
                "where u.user_id = t.user_id and t.table_type in ( 'T', 'M' )",
                gColumnName[sIsODBC3][0],
                gColumnName[sIsODBC3][1]
        );

    if (sUserName[0] != 0)
    {
        /* BUGBUG just for emulate REGEX '*' by LIKE */
        for( sTempStr = sUserName;  *sTempStr != 0; sTempStr++)
        {
            if( *sTempStr == '*' )
            {
                *sTempStr = '%';
            }
        }

        aciVaAppendFormat(aQueryBuffer,
                          aQueryBufferSize,
                          " and u.user_name LIKE \'%s\'",
                          sUserName);

    }

    /* BUGBUG just for emulate REGEX '*' by LIKE */
    for( sTempStr = sTableName; *sTempStr != 0; sTempStr++)
    {
        if( *sTempStr == '*' )
        {
            *sTempStr = '%';
        }
    }
    aciVaAppendFormat(aQueryBuffer,
                      aQueryBufferSize,
                      " and t.table_name LIKE \'%s\'",
                      sTableName);

    aciVaAppendFormat(aQueryBuffer,
                      aQueryBufferSize,
                      ") U_T,"
                      "system_.sys_users_ u,"
                      "system_.sys_users_ u2,"
                      "system_.sys_privileges_ priv,"
                      "system_.sys_grant_object_ obj "
                      "where obj.obj_id = u_t.table_id "
                      "and u.user_id = obj.grantor_id "
                      "and u2.user_id = obj.grantee_id "
                      "and priv.priv_id = obj.priv_id "
                      "order by 2,3,6,5");

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_HY009)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN2)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength2);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN3)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength3);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnTablePrivileges(ulnStmt      *aStmt,
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
    acp_char_t   sQueryStringBuffer[ULN_CATALOG_QUERY_STR_BUF_SIZE];
    ulnDbc      *sParentDbc = (aStmt != NULL) ? aStmt->mParentDbc : NULL;  /* BUG-46885 */

    ACP_UNUSED(aCatalogName);
    ACP_UNUSED(aNameLength1);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_TABLEPRIVILEGES, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)sParentDbc) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * BUGBUG : Argument validity checking 을 수행해야 한다.
     */

    ACI_TEST(ulnTblPrivCreateQueryString(&sFnContext,
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
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * BUGBUG : 컬럼의 타입을 강제로 지정해 주는 코드가 cli2 에는 있었다.
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
        ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
