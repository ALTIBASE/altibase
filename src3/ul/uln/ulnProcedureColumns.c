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



static ACI_RC ulnProcColCreateQueryString(ulnFnContext *aFnContext,
                                          acp_char_t   *aSchemaName,
                                          acp_sint16_t  aNameLength2,
                                          acp_char_t   *aProcName,
                                          acp_sint16_t  aNameLength3,
                                          acp_char_t   *aColumnName,
                                          acp_sint16_t  aNameLength4,
                                          acp_char_t   *aQueryStringBuffer,
                                          acp_uint32_t  aQueryStringBufferSize,
                                          acp_bool_t    aOrderByPos)
{
    acp_sint32_t   sSize;
    acp_rc_t       sRet;

    const acp_char_t* sColNames[6][2]=
        {
            /*ODBC 2.0 column         ODBC 3.x column */
            { "PROCEDURE_QUALIFIER", "PROCEDURE_CAT"   },
            { "PROCEDURE_OWNER"    , "PROCEDURE_SCHEM" },
            { "PRECISION"          , "COLUMN_SIZE"     },
            { "LENGTH"             , "BUFFER_LENGTH"   },
            { "SCALE"              , "DECIMAL_DIGITS"  },
            { "RADIX"              , "NUM_PREC_RADIX"  }
        };

    acp_bool_t    sIsODBC3 = ACP_FALSE;
    ACP_UNUSED(aOrderByPos);

    if(aFnContext->mHandle.mStmt->mParentDbc->mOdbcVersion ==  SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    /*
     * BUGBUG - there are many cases for special function
     * for calculation of BUFFER_LENGTH, and CHAR_OCTET_LENGTH
     * like x$CHAR_OCTET_LENGTH(type, precision, scale);
     */

    /******************************************************
       bug-36655: the params order of a PSM is strange.
       변경사항
       1. union을 추가하여 return_value 조회도 추가
          (jdbc getProcedureColumns()를 참고함)
       2. order by를 inout_type에서 파라미터 순서로 변경
          (inout_type으로 되어있는 msdn odbc spec이 이상함)
    ******************************************************/
    sRet = acpSnprintf(aQueryStringBuffer, aQueryStringBufferSize,
            "SELECT " "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," 
            "d.user_name"       " as %s," // PROCEDURE_SCHEM
            "b.proc_name"       " as PROCEDURE_NAME,"
            "VARCHAR'RETURN_VALUE' as COLUMN_NAME,"
            "cast( 5 as SMALLINT )"
            " as COLUMN_TYPE,"
            "cast( b.return_data_type as SMALLINT ) as  DATA_TYPE,"
            "t.type_name"       " as  TYPE_NAME,"
            "cast( decode(b.return_precision,0, "
            "             decode(b.return_data_type, "
            "                    1 , 0, "
            "                    12, 0, "
            "                    -8, 0, "
            "                    -9, 0, "
            "                    60, 0, "
            "                    61, 0, "
            "                    t.COLUMN_SIZE), "
            "             b.return_precision) as INTEGER )"
            " as %s," // COLUMN_SIZE
            "cast( decode(b.return_size,0,b.return_precision,b.return_size) as INTEGER )"
            " as %s," // BUFFER_LENGTH
            "cast( b.return_scale as SMALLINT ) as %s," // DECIMAL_DIGITS
            "cast( t.NUM_PREC_RADIX as SMALLINT ) as %s,"
            "2"                 " as NULLABLE,"
            "VARCHAR'RETURN_VALUE' as REMARKS,"        
            "VARCHAR''"         " as COLUMN_DEF,"
            "t.SQL_DATA_TYPE"   " as SQL_DATA_TYPE,"
            "t.SQL_DATETIME_SUB"" as SQL_DATETIME_SUB,"
            "cast( decode(b.return_size,0,b.return_precision, b.return_size) as INTEGER )"
            " as CHAR_OCTET_LENGTH,"
            "0"              " as ORDINAL_POSITION,"
            "VARCHAR''"      " as IS_NULLABLE"          
            " FROM  "
            "X$DATATYPE"           " t,"
            "system_.sys_procedures_ b,"
            "system_.sys_users_ d"
            " WHERE "
            "b.object_type = 1 AND "
            "b.user_id=d.user_id AND "
            "t.data_type=b.return_data_type",

        sColNames[0][sIsODBC3],
        sColNames[1][sIsODBC3],
        sColNames[2][sIsODBC3],
        sColNames[3][sIsODBC3],
        sColNames[4][sIsODBC3],
        sColNames[5][sIsODBC3]);

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);

    sSize = acpCStrLen(aQueryStringBuffer, aQueryStringBufferSize);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryStringBuffer,
                                     aQueryStringBufferSize,
                                     sSize,
                                     " AND d.user_name  LIKE '%s'", aSchemaName, aNameLength2);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryStringBuffer,
                                     aQueryStringBufferSize,
                                     sSize,
                                     " AND  b.proc_name LIKE '%s'", aProcName, aNameLength3);
    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);

    sSize = aciVaAppendFormat(aQueryStringBuffer, aQueryStringBufferSize,
            "UNION "
            "SELECT " "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // PROCEDURE_CAT BUG-17786
            "d.user_name"       " as %s,"                               // PROCEDURE_SCHEM
            "b.proc_name"       " as PROCEDURE_NAME,"
            "a.para_name"       " as COLUMN_NAME,"
            "cast( decode(a.data_type,1000004,3,decode(a.inout_type,0,1,2,2,1,4,0)) as SMALLINT )"
            " as COLUMN_TYPE,"
            "cast( decode(a.data_type,1000004,-404,a.data_type) as SMALLINT ) as DATA_TYPE,"
            "NVL(t.TYPE_NAME,decode(a.data_type,1000004,'REF CURSOR','OTHER'))"
            " as TYPE_NAME,"
            "NVL2(t.TYPE_NAME,"
                 "cast( decode(a.precision,0,"
                              "decode(a.data_type," // fix BUG-26817 문자열의 pricision이 0이면 0 반환
                                      "1, a.precision,"
                                     "12, a.precision,"
                                     "-8, a.precision,"
                                     "-9, a.precision,"
                                     "60, a.precision,"
                                     "61, a.precision,"
                                     "t.COLUMN_SIZE),"
                              "a.precision)"
                      " as INTEGER)"
                 ", NULL)"
            " as %s,"                                                   // COLUMN_SIZE
            "NVL2(t.TYPE_NAME, cast(decode(a.size,0,a.precision,a.size) as INTEGER), NULL)"
            " as %s,"                                                   // BUFFER_LENGTH - should be function fo calculate it ??
            "NVL2(t.TYPE_NAME, cast(a.scale as SMALLINT), NULL) as %s," // DECIMAL_DIGITS
            "cast( t.NUM_PREC_RADIX as SMALLINT ) as %s,"               // NUM_PREC_RADIX
            "2"                 " as NULLABLE,"
            "VARCHAR''"         " as REMARKS,"                          // fix BUG-18222
            "a.default_val"     " as COLUMN_DEF,"
            "t.SQL_DATA_TYPE"   " as SQL_DATA_TYPE,"
            "t.SQL_DATETIME_SUB"" as SQL_DATETIME_SUB,"
            "NVL2(t.TYPE_NAME, cast(decode(a.size,0,a.precision, a.size) as INTEGER), NULL)"
            " as CHAR_OCTET_LENGTH,"
            "a.para_order"      " as ORDINAL_POSITION,"
            "VARCHAR''"      " as IS_NULLABLE"                          // fix BUG-18222
            " FROM  "
            "system_.sys_proc_paras_ a"
            " LEFT JOIN X$DATATYPE t ON t.data_type=a.data_type,"
            "system_.sys_procedures_ b,"
            "system_.sys_users_ d"
            " WHERE "
            "a.proc_oid=b.proc_oid AND "
            "b.user_id=d.user_id",
        sColNames[0][sIsODBC3],
        sColNames[1][sIsODBC3],
        sColNames[2][sIsODBC3],
        sColNames[3][sIsODBC3],
        sColNames[4][sIsODBC3],
        sColNames[5][sIsODBC3]);

    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);

    sSize = acpCStrLen(aQueryStringBuffer, aQueryStringBufferSize);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryStringBuffer,
                                     aQueryStringBufferSize,
                                     sSize,
                                     " AND d.user_name  LIKE '%s'", aSchemaName, aNameLength2);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryStringBuffer,
                                     aQueryStringBufferSize,
                                     sSize,
                                     " AND  b.proc_name LIKE '%s'", aProcName, aNameLength3);
    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryStringBuffer,
                                     aQueryStringBufferSize,
                                     sSize,
                                     " AND a.para_name LIKE '%s'", aColumnName, aNameLength4);
    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN4);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringBufferSize, ERR_HY001);

    /* bug-36655: the params order of a PSM is strange */
    sSize = aciVaAppendFormat(aQueryStringBuffer, aQueryStringBufferSize,
            " ORDER BY 2,3,18");
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

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN4)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength4);
    }

    ACI_EXCEPTION(ERR_HY001);
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "Not enougth buffer's space for query.");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnProcedureColumns(ulnStmt      *aStmt,
                              acp_char_t   *aCatalogName,
                              acp_sint16_t  aNameLength1,
                              acp_char_t   *aSchemaName,
                              acp_sint16_t  aNameLength2,
                              acp_char_t   *aProcName,
                              acp_sint16_t  aNameLength3,
                              acp_char_t   *aColumnName,
                              acp_sint16_t  aNameLength4,
                              acp_bool_t    aOrderByPos)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext sFnContext;
    acp_char_t   sQueryString[ULN_CATALOG_QUERY_STR_BUF_SIZE];
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

    ACI_TEST(ulnProcColCreateQueryString(&sFnContext,
                                         aSchemaName,
                                         aNameLength2,
                                         aProcName,
                                         aNameLength3,
                                         aColumnName,
                                         aNameLength4,
                                         sQueryString,
                                         ACI_SIZEOF(sQueryString),
                                         aOrderByPos) != ACI_SUCCESS);

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
     *          stmt->bindings[ 5-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[ 6-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[ 8-1].forced_type = SQL_C_SLONG;
     *          stmt->bindings[ 9-1].forced_type = SQL_C_SLONG;
     *          stmt->bindings[10-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[11-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[12-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[15-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[16-1].forced_type = SQL_C_SSHORT;
     *          stmt->bindings[17-1].forced_type = SQL_C_SLONG;
     *          stmt->bindings[18-1].forced_type = SQL_C_SLONG;
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
