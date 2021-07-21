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


static ACI_RC ulnColumnsCreateQueryString(ulnFnContext *aFnContext,
                                          acp_char_t   *aSchemaName,
                                          acp_sint16_t  aNameLength2,
                                          acp_char_t   *aTableName,
                                          acp_sint16_t  aNameLength3,
                                          acp_char_t   *aColumnName,
                                          acp_sint16_t  aNameLength4,
                                          acp_char_t   *aQueryString,
                                          acp_uint32_t  aQueryStringLength)
{
    acp_sint32_t sSize;
    acp_rc_t     sRet;
    const acp_char_t* sColNames[6][2] =
        {
            /* ODBC 2.0 column  ODBC 3.x column */
            { "TABLE_QUALIFIER" , "TABLE_CAT"       },
            { "TABLE_OWNER"     , "TABLE_SCHEM"     },
            { "PRECISION"       , "COLUMN_SIZE"     },
            { "LENGTH"          , "BUFFER_LENGTH"   },
            { "SCALE"           , "DECIMAL_DIGITS"  },
            { "RADIX"           , "NUM_PREC_RADIX"  }
        };

    acp_bool_t sIsODBC3 = ACP_FALSE;

    if(aFnContext->mHandle.mStmt->mParentDbc->mOdbcVersion ==  SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    /* ODBC3 spec says that one of them mast have value */
    ACI_TEST_RAISE(aSchemaName == NULL && aTableName == NULL && aColumnName == NULL, ERR_HY009);

    // BUG-17202]
    sRet = acpSnprintf(aQueryString, aQueryStringLength,
                            "SELECT "
                            "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // BUG-17786
                            "c.user_name"           " as %s,"            // TABLE_SCHEM
                            "b.table_name"          " as TABLE_NAME,"
                            "a.column_name"         " as COLUMN_NAME,"
                            "cast( nvl2(f.column_id,3010,t.ODBC_SQL_DATA_TYPE) as SMALLINT )"
                                                    " as DATA_TYPE,"       // BUG-21594
                            "nvl2(f.column_id,'TIMESTAMP',"
                                 "decode(a.data_type, 60, VARCHAR'CHAR', 61, VARCHAR'VARCHAR', t.type_name))"
                                                    " as TYPE_NAME,"       // PROJ-2002 Column Security
                            "cast( decode(a.precision,0, "
                            "             decode(a.data_type, "            // fix BUG-26817 문자열의 pricision이 0이면 0 반환
                            "                    1, a.precision, "
                            "                    12, a.precision, "
                            "                    -8, a.precision, "
                            "                    -9, a.precision, "
                            "                    60, a.precision, "
                            "                    61, a.precision, "
                            "                    t.column_size), "
                            "             a.precision) as INTEGER )"
                                                    " as %s,"              // COLUMN_SIZE
                            /* bug-21202:
                             * wrong buffer_length returned from SQLColumns.
                             * before: return the internal size used in server.
                             * after : return the maximum transfer size in bytes. */
                            "cast( decode(a.data_type, "
                            "             5, 2, "              /* smallint */
                            "             4, 4, "              /* integer  */
                            "             -5, 20*2, "          /* bigint   */
                            "             7, 4, "              /* real     */
                            "             8, 8, "              /* double   */
                            "             2, a.precision+2, "  /* numeric  */
                            "             6, a.precision+2, "  /* float    */
                            "             -8, a.precision*3, " /* nchar    */
                            "             -9, a.precision*3, " /* nvarchar */
                            "             30, t.column_size, " /* blob     */
                            "             40, t.column_size, " /* clob     */
                            "             20001, a.precision*2, "  /* byte */
                            "             9, 16, "             /* date     */
                            "             a.precision) as INTEGER )"
                            " as %s,"
                            "cast( a.scale as SMALLINT ) as %s,"           // DECIMAL_DIGITS
                            "cast( t.NUM_PREC_RADIX as SMALLINT ) as %s,"  // NUM_PREC_RADIX,"
                            "cast( decode(a.IS_NULLABLE,'F',0,1) as SMALLINT ) as NULLABLE,"
                            "VARCHAR'' as REMARKS,"   // To Fix BUG-18222
                            "a.DEFAULT_VAL"         " as COLUMN_DEF,"
                            "t.SQL_DATA_TYPE"       " as SQL_DATA_TYPE,"
                            "t.SQL_DATETIME_SUB"    " as SQL_DATETIME_SUB,"

                            /* bug-21202:
                             * wrong buffer_length returned from SQLColumns.
                             * char_octet_length: transfer size in bytes */
                            "cast( decode(a.data_type, "
                            "             1, a.precision, "    /* char     */
                            "             12, a.precision, "   /* varchar  */
                            "             -8, a.precision*3, " /* nchar    */
                            "             -9, a.precision*3, " /* nvarchar */
                            "             60, a.precision, "   /* echar    */
                            "             61, a.precision, "   /* evarchar */
                            "             30, t.column_size, " /* blob     */
                            "             40, t.column_size, " /* clob     */
                            "             20001, a.precision*2, " /* byte   */
                            "             20002, a.precision, "   /* nibble */
                            "             -7, a.precision, "      /* bit    */
                            "             -100, a.precision, "    /* varbit */
                            "             0) as INTEGER )"
                            " as CHAR_OCTET_LENGTH,"

                            "a.column_order+1"      " as ORDINAL_POSITION,"

                            "decode(a.IS_NULLABLE,'F','NO','YES')"
                            " as IS_NULLABLE,"

                            /* Altibase Specific Columns */
                            "a.store_type"          " as STORE_TYPE, "

                            /* PROJ-2002 Column Security */
                            "cast( decode(a.data_type, 60, 1, 61, 1, 0) as SMALLINT )"
                                                    " as ENCRYPT, "

                            /* PROJ-2422 srid */
                            "cast( decode(a.data_type, 10003, 1, 0) as SMALLINT )"
                                                    " as GEOMETRY "

                            "FROM "
                            "system_.sys_tables_  b,"
                            "system_.sys_users_"" c,"
                            "X$DATATYPE"        " t,"
                            "system_.sys_columns_ a left outer join "
                            "(select d.table_id table_id,d.column_id column_id "
                            "from system_.SYS_CONSTRAINT_COLUMNS_ d,"
                            "system_.SYS_CONSTRAINTS_ e,system_.sys_tables_ f "
                            "where d.constraint_id=e.constraint_id "
                            "and e.constraint_type=5 and e.table_id=f.table_id ",
                            sColNames[0][sIsODBC3],
                            sColNames[1][sIsODBC3],
                            sColNames[2][sIsODBC3],
                            sColNames[3][sIsODBC3],
                            sColNames[4][sIsODBC3],
                            sColNames[5][sIsODBC3] );

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);

    sSize = acpCStrLen(aQueryString, aQueryStringLength);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
            aQueryString,
            aQueryStringLength,
            sSize,
            "and f.table_name LIKE '%s'",
            aTableName,
            aNameLength3 );

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringLength, ERR_HY001);

    sRet = acpSnprintf(aQueryString + sSize, aQueryStringLength - sSize,
                       ") f on f.column_id=a.column_id "
                       "WHERE "
                       "b.TABLE_TYPE in('T','V','Q','M','A','G','E','R') and "  /* BUG-35460 Add TABLE_TYPE G in SYS_TABLES_ */
                       "a.table_id=b.table_id and "
                       "a.data_type=t.data_type and "
                       "a.IS_HIDDEN = 'F' " ); /* PROJ-1090 Function-based Index */

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);


    sSize += acpCStrLen(aQueryString + sSize, aQueryStringLength - sSize);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize =  ulnAppendFormatParameter(aFnContext,
            aQueryString,
            aQueryStringLength,
            sSize,
            "and c.user_name LIKE '%s' "
            "and b.user_id=c.user_id ",
            aSchemaName,
            aNameLength2 );

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringLength, ERR_HY001);

    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
            aQueryString,
            aQueryStringLength,
            sSize,
            "and b.table_name LIKE '%s' ",
            aTableName,
            aNameLength3 );

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringLength, ERR_HY001);

    // bug-25905: conn nls not applied to client lang module
    sSize = ulnAppendFormatParameter(aFnContext,
            aQueryString,
            aQueryStringLength,
            sSize,
            "and a.column_name LIKE '%s' ",
            aColumnName,
            aNameLength4);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN4);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringLength, ERR_HY001);

    // BUG-20889
    // SQLColumns returns the results as a standard result set
    // ordered by TABLE_CAT, TABLE_SCHEM, TABLE_NAME, and ORDINAL_POSITION
    sSize = aciVaAppendFormat(aQueryString,
                              aQueryStringLength,
                              "order by c.user_name, b.table_name, a.column_order");

    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryStringLength, ERR_HY001);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN2);
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength2);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN3);
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength3);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN4);
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength4);
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

static ACI_RC ulnColumnsCheckArgs(ulnFnContext *aFnContext,
                                  acp_char_t   *aCatalogName,
                                  acp_sint16_t  aNameLength1,
                                  acp_char_t   *aSchemaName,
                                  acp_sint16_t  aNameLength2,
                                  acp_char_t   *aTableName,
                                  acp_sint16_t  aNameLength3,
                                  acp_char_t   *aColumnName,
                                  acp_sint16_t  aNameLength4)
{
    ACP_UNUSED(aCatalogName);
    ACP_UNUSED(aSchemaName);
    ACP_UNUSED(aTableName);
    ACP_UNUSED(aColumnName);

    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength1, ULN_CATALOG_MAX_NAME_LEN) != ACI_SUCCESS, LABEL_INVALID_BUF_LEN1 );

    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength2, ULN_CATALOG_MAX_NAME_LEN) != ACI_SUCCESS, LABEL_INVALID_BUF_LEN2 );

    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength3, ULN_CATALOG_MAX_NAME_LEN) != ACI_SUCCESS, LABEL_INVALID_BUF_LEN3 );

    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength4, ULN_CATALOG_MAX_NAME_LEN) != ACI_SUCCESS, LABEL_INVALID_BUF_LEN4 );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN1)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength1);
    }
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
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnColumns(ulnStmt      *aStmt,
                     acp_char_t   *aCatalogName,
                     acp_sint16_t  aNameLength1,
                     acp_char_t   *aSchemaName,
                     acp_sint16_t  aNameLength2,
                     acp_char_t   *aTableName,
                     acp_sint16_t  aNameLength3,
                     acp_char_t   *aColumnName,
                     acp_sint16_t  aNameLength4)
{
    acp_bool_t             sNeedExit = ACP_FALSE;
    acp_bool_t             sNeedFinPtContext = ACP_FALSE;

    acp_char_t              sQueryString[ULN_CATALOG_QUERY_STR_BUF_SIZE];

    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_COLUMNS, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST( aStmt == NULL );  //BUG-28184 : [CodeSonar] Null Pointer Dereference
    ACI_TEST(ulnEnter(&sFnContext, (void *)(aStmt->mParentDbc)) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;

    /*
     * Check if arguments are valid
     */
    ACI_TEST(ulnColumnsCheckArgs(&sFnContext,
                                 aCatalogName,
                                 aNameLength1,
                                 aSchemaName,
                                 aNameLength2,
                                 aTableName,
                                 aNameLength3,
                                 aColumnName,
                                 aNameLength4) != ACI_SUCCESS);

    /*
     * Create query string for geting column information from DB
     */
    ACI_TEST(ulnColumnsCreateQueryString(&sFnContext,
                                         aSchemaName,
                                         aNameLength2,
                                         aTableName,
                                         aNameLength3,
                                         aColumnName,
                                         aNameLength4,
                                         sQueryString,
                                         ACI_SIZEOF(sQueryString)) != ACI_SUCCESS);

    /*
     * Protocol Context 초기화
     */
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          //fix BUG-17722
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);

    sNeedFinPtContext = ACP_TRUE;

    /*
     * Prepare Phase
     */
    ACI_TEST(ulnPrepareCore(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext),
                            sQueryString,
                            acpCStrLen(sQueryString, ULN_CATALOG_QUERY_STR_BUF_SIZE),
                            CMP_DB_PREPARE_MODE_EXEC_DIRECT) != ACI_SUCCESS);

    /*
     * Execute Phase
     */
    ACI_TEST(ulnExecuteCore(&sFnContext,&(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    ACI_TEST(ulnFlushAndReadProtocol(&sFnContext,
                                     &(aStmt->mParentDbc->mPtContext),
                                     aStmt->mParentDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /*
     * BUGBUG : 각 컬럼의 타입을 강제로 지정해 주는 코드가 cli2 에는 있었다.
     */

    /*
     * Protocol Context 정리
     */
    sNeedFinPtContext = ACP_FALSE;

    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if(sNeedFinPtContext == ACP_TRUE)
    {
        ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

