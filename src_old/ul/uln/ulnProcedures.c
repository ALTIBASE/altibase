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

static ACI_RC ulnProcCreateQueryString(ulnFnContext *aFnContext,
                                       acp_char_t   *aSchemaName,
                                       acp_sint16_t  aNameLength2,
                                       acp_char_t   *aProcName,
                                       acp_sint16_t  aNameLength3,
                                       acp_char_t   *aQueryBuffer,
                                       acp_uint32_t  aQueryBufferSize)
{
    acp_sint32_t   sSize;
    acp_rc_t       sRet;

    const acp_char_t* sColNames[2][2]=
        {
            /*ODBC 2.0 column         ODBC 3.x column */
            { "PROCEDURE_QUALIFIER", "PROCEDURE_CAT"   },
            { "PROCEDURE_OWNER"    , "PROCEDURE_SCHEM" }
        };

    acp_bool_t     sIsODBC3 = ACP_FALSE;

    if(aFnContext->mHandle.mStmt->mParentDbc->mOdbcVersion ==  SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    /* ODBC3 spec says that one of them mast have value */
    // ACI_TEST_RAISE(aSchemaName == NULL && aProcName == NULL, ERR_HY009);

    sRet = acpSnprintf(aQueryBuffer, aQueryBufferSize,
                       "select "
                       "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // PROCEDURE_CAT BUG-17786
                       "d.user_name as %s,"                              // PROCEDURE_SCHEM
                       "a.proc_name as PROCEDURE_NAME,"

                       /* calculate count of INPUT */
                       "(select count(*) "
                       " from system_.sys_proc_paras_ p"
                       " where ( p.INOUT_TYPE = 0 or p.INOUT_TYPE = 2)"
                       " AND p.proc_oid = a.proc_oid )"
                       " as NUM_INPUT_PARAMS,"

                       /* calculate count of OUTPUT */
                       "(select count(*) "
                       " from system_.sys_proc_paras_ p"
                       " where ( p.INOUT_TYPE = 1 or p.INOUT_TYPE = 2)"
                       " AND p.proc_oid = a.proc_oid )"
                       " as NUM_OUTPUT_PARAMS,"

                       "0  as NUM_RESULT_SETS,"
                       "VARCHAR'' as REMARKS,"                           // To Fix BUG-18222
                       "cast( decode(a.object_type,0,1,1,2,0) as SMALLINT ) as PROCEDURE_TYPE"
                       " from "
                       "system_.sys_procedures_ a,"
                       "system_.sys_users_ d "
                       "where a.user_id=d.user_id ",
                       sColNames[0][sIsODBC3],
                       sColNames[1][sIsODBC3] );

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);

    sSize = acpCStrLen(aQueryBuffer, aQueryBufferSize);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryBuffer,
                                     aQueryBufferSize,
                                     sSize,
                                     " AND d.user_name  LIKE '%s'", aSchemaName, aNameLength2);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryBuffer,
                                     aQueryBufferSize,
                                     sSize,
                                     " AND  a.proc_name LIKE '%s'", aProcName, aNameLength3);
    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    sSize = aciVaAppendFormat(aQueryBuffer,
                              aQueryBufferSize,
                              " order by 2,3");

    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

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

SQLRETURN ulnProcedures(ulnStmt      *aStmt,
                        acp_char_t   *aCatalogName,
                        acp_sint16_t  aNameLength1,
                        acp_char_t   *aSchemaName,
                        acp_sint16_t  aNameLength2,
                        acp_char_t   *aProcName,
                        acp_sint16_t  aNameLength3)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext sFnContext;
    acp_char_t   sQueryStringBuffer[ULN_CATALOG_QUERY_STR_BUF_SIZE];
    ulnDbc      *sParentDbc = (aStmt != NULL) ? aStmt->mParentDbc : NULL;  /* BUG-46885 */

    ACP_UNUSED(aCatalogName);
    ACP_UNUSED(aNameLength1);
    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PROCEDURECOLUMNS, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)sParentDbc) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * BUGBUG : Argument validity checking 을 수행해야 한다.
     */

    ACI_TEST(ulnProcCreateQueryString(&sFnContext,
                                      aSchemaName,
                                      aNameLength2,
                                      aProcName,
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
    ACI_TEST(ulnExecuteCore(&sFnContext,&(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);
    //fix BUG-17722
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
        ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

