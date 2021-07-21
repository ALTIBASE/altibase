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

static ACI_RC ulnFKCreateQueryString(ulnFnContext *aFnContext,
                                     acp_char_t   *aPKSchemaName,
                                     acp_sint16_t  aNameLength2,
                                     acp_char_t   *aPKTableName,
                                     acp_sint16_t  aNameLength3,
                                     acp_char_t   *aFKSchemaName,
                                     acp_sint16_t  aNameLength5,
                                     acp_char_t   *aFKTableName,
                                     acp_sint16_t  aNameLength6,
                                     acp_char_t   *aQueryBuffer,
                                     acp_uint32_t  aQueryBufferSize)
{
    acp_sint32_t   sSize;
    acp_rc_t       sRet;

    const acp_char_t* sColNames[4][2]=
    {
        /* ODBC 2.0 column     ODBC 3.x column */
        { "PKTABLE_QUALIFIER"   , "PKTABLE_CAT"    },
        { "PKTABLE_OWNER"       , "PKTABLE_SCHEM"  },
        { "FKTABLE_QUALIFIER"   , "FKTABLE_CAT"    },
        { "FKTABLE_OWNER"       , "FKTABLE_SCHEM"  }
    };
    acp_bool_t sIsODBC3 = ACP_FALSE;

    if(aFnContext->mHandle.mStmt->mParentDbc->mOdbcVersion ==  SQL_OV_ODBC3)
    {
        sIsODBC3 = ACP_TRUE;
    }

    /* By ODBC3 that PKTable and FKTable
     * list MAST be defined           */
    ACI_TEST_RAISE(aPKTableName == NULL && aFKTableName == NULL, ERR_HY009);

    sRet = acpSnprintf(aQueryBuffer, aQueryBufferSize,
                       "SELECT "
                       "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // PKTABLE_CAT BUG-17786
                       "P.PKTABLE_SCHEM  as %s,"                         // PKTABLE_SCHEM
                       "P.PKTABLE_NAME"" as PKTABLE_NAME,"
                       "P.PK_COLUMN_NAME as PKCOLUMN_NAME,"
                       "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS %s," // FKTABLE_CAT BUG-17786
                       "F.FKTABLE_SCHEM  as %s,"                         // FKTABLE_SCHEM
                       "F.FKTABLE_NAME   as FKTABLE_NAME,"
                       "F.FK_COLUMN_NAME as FKCOLUMN_NAME,"
                       "cast( F.KEY_SEQ as SMALLINT ) as KEY_SEQ,"
                       "3"             " as UPDATE_RULE,"
                       // BUG-25175 delete cascade 를 처리하지 못합니다.
                       // delete cascade 일 경우 서버에서 1을 보내줌
                       // SQL_CASCADE     0
                       // SQL_RESTRICT    1
                       // SQL_SET_NULL    2
                       // SQL_NO_ACTION   3
                       // SQL_SET_DEFAULT 4
                       "cast( decode(F.DELETE_RULE, 1, 0, 2, 2, 0, 3) as SMALLINT) as DELETE_RULE,"
                       "F.FK_NAME"     " as FK_NAME,"
                       "P.PK_NAME"     " as PK_NAME,"
                       "6"             " as DEFERRABILITY " // #define SQL_INITIALLY_IMMEDIATE  6
                       " FROM  "
                       "(SELECT "
                       "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS PKTABLE_CAT," // BUG-17786
                       "pe.user_name"          " as PKTABLE_SCHEM,"
                       "pd.table_name"         " as PKTABLE_NAME,"
                       "pc.column_name"        " as PK_COLUMN_NAME,"
                       "pa.index_name"         " as PK_NAME,"
                       "pb.index_col_order+1"  " as KEY_SEQ,"
                       "pd.table_id"           " as CID,"
                       "pa.INDEX_ID"           " as idxid"
                       " FROM "
                       "system_.sys_indices_ "    " pa,"
                       "system_.sys_index_columns_  pb,"
                       "system_.sys_columns_"     " pc,"
                       "system_.sys_tables_"      " pd,"
                       "system_.sys_users_"       " pe "
                       " WHERE ",
                       sColNames[0][sIsODBC3],
                       sColNames[1][sIsODBC3],
                       sColNames[2][sIsODBC3],
                       sColNames[3][sIsODBC3] );

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), ERR_HY001);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ERR_HY001);

    sSize = acpCStrLen(aQueryBuffer, aQueryBufferSize);

    /* 1. PK_SCHEMA */
    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryBuffer,
                                     aQueryBufferSize,
                                     sSize,
                                     "pe.user_name=\'%s\' and ",
                                     aPKSchemaName,
                                     aNameLength2);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN2);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    /* 1.2 link PK_SCHEMA & PK_TABLE */
    sSize = aciVaAppendFormat(aQueryBuffer,
                              aQueryBufferSize,
                              "pe.user_id = pd.user_id ");

    ACI_TEST_RAISE(0 > sSize || sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    /* 2. PK_TABLE */
    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryBuffer,
                                     aQueryBufferSize,
                                     sSize,
                                     "and pd.table_name=\'%s\' ",
                                     aPKTableName,
                                     aNameLength3);
    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN3);
    ACI_TEST_RAISE( sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    sSize = aciVaAppendFormat(aQueryBuffer,
                              aQueryBufferSize,
                              "and pe.user_id=pa.user_id "
                              "and pa.table_id=pd.table_id "
                              "and pa.is_unique='T' "
                              "and pa.index_id=pb.index_id "
                              "and pb.column_id=pc.column_id) AS P "
                              "INNER JOIN "
                              "(select "
                              "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS FKTABLE_CAT," // BUG-17786
                              "fe.user_name"            " as  FKTABLE_SCHEM,"
                              "fd.table_name"           " as  FKTABLE_NAME,"
                              "fc.column_name"          " as  FK_COLUMN_NAME,"
                              "fa.constraint_name"      " as  FK_NAME,"
                              "fb.constraint_col_order+1  as  KEY_SEQ,"
                              "fa.referenced_table_id"  " as  CID,"
                              "fa.REFERENCED_INDEX_ID"  " as  idxid, "
                              "fa.DELETE_RULE        "  " as  DELETE_RULE "
                              " FROM "
                              "system_.sys_constraints_"     " fa,"
                              "system_.sys_constraint_columns_ fb,"
                              "system_.sys_columns_"         " fc,"
                              "system_.sys_tables_"          " fd,"
                              "system_.sys_users_"           " fe "
                              " WHERE ");

    ACI_TEST_RAISE(0 > sSize || sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    /* 3. FK_SCHEMA */
    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryBuffer,
                                     aQueryBufferSize,
                                     sSize,
                                     "fe.user_name=\'%s\' and ",
                                     aFKSchemaName,
                                     aNameLength5);

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN5);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    /* 3.1 Link */
    sSize = aciVaAppendFormat(aQueryBuffer,
                              aQueryBufferSize,
                              "fe.user_id=fd.user_id ");

    ACI_TEST_RAISE(0 > sSize || sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    /* 4. FK_TABLE */
    // bug-25905: conn nls not applied to client lang module
    // aFnContext 인자 추가
    sSize = ulnAppendFormatParameter(aFnContext,
                                     aQueryBuffer,
                                     aQueryBufferSize,
                                     sSize,
                                     "and fd.table_name=\'%s\' ",
                                     aFKTableName,
                                     aNameLength6 );

    ACI_TEST_RAISE(0 > sSize, LABEL_INVALID_BUF_LEN6);
    ACI_TEST_RAISE(sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    sSize = aciVaAppendFormat(aQueryBuffer, aQueryBufferSize,
                              "and fe.user_id = fa.user_id  "
                              "and fa.table_id = fd.table_id "
                              "and fa.constraint_type=0 "
                              "and fa.constraint_id=fb.constraint_id "
                              "and fb.column_id=fc.column_id) AS F "
                              "ON (P.CID=F.CID and P.IDXID=F.IDXID and P.KEY_SEQ=F.KEY_SEQ) ");

    ACI_TEST_RAISE(0 > sSize || sSize >= (acp_sint32_t)aQueryBufferSize, ERR_HY001);

    if( aFKTableName != NULL)
    {
        // To Fix BUG-17907
        // 문제 1 )
        //    ODBC 표준에 의거 해당 테이블이 참조하고 있는
        //    Foreign Key 정보가 요청될 경우,
        //    다음 순서로 ORDER BY 됨 :
        //    PKTABLE_CAT(1), PKTABLE_SCHEM(2), PKTABLE_NAME(3), KEY_SEQ(9)
        // 문제 2 )
        //    ODBC 표준에는 Foreign Key Name 에 대한
        //    Sorting 여부가 존재하지 않으며,
        //    Vendor 에 따라 NULL값이 올수 있음을 허용하고 있다.
        //    그러나, 동일한 Primary Key를 참조하고 있는
        //    Foreign Key Constraint가 여러개일 경우
        //    Foreign Key Name으로 Ordering하지 않으면
        //    KEY_SEQ 의 연관 관계를 파악할 수 없게 된다.
        // 표준 ORDER BY 1, 2, 3, 9 => ORDER BY 1, 2, 3, 12(FK_NAME), 9
        sSize = aciVaAppendFormat(aQueryBuffer,
                                  aQueryBufferSize,
                                  "ORDER BY 1,2,3,12,9");
    }
    else
    {
        // To Fix BUG-17907
        // 문제 1 )
        //    ODBC 표준에 의거 해당 테이블의 Primary Key와
        //    연관이 있는 다른 테이블의 Foreign Key 정보가 요청될 경우
        //    다음 순서로 ORDER BY 됨 :
        //     FKTABLE_CAT(5), FKTABLE_SCHEM(6), FKTABLE_NAME(7), KEY_SEQ(9)
        // 문제 2 )
        //    ODBC 표준에는 Foreign Key Name 에 대한 Sorting 여부가 존재하지 않으며,
        //    Vendor 에 따라 NULL값이 올수 있음을 허용하고 있다.
        //    그러나, 다른 테이블에서 동일한 나의 Primary Key를 참조하고 있는
        //    Foreign Key Constraint가 여러개일 경우
        //    Foreign Key Name으로 Ordering하지 않으면
        //    KEY_SEQ 의 연관 관계를 파악할 수 없게 된다.
        // 표준 ORDER BY 5, 6, 7, 9 => ORDER BY 5, 6, 7, 12(FK_NAME), 9
        sSize = aciVaAppendFormat(aQueryBuffer,
                                  aQueryBufferSize,
                                  "ORDER BY 5,6,7,12,9");
    }

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

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN5)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength5);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN6)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength6);
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

SQLRETURN ulnForeignKeys(ulnStmt      *aStmt,
                         acp_char_t   *aPKCatalogName,
                         acp_sint16_t  aNameLength1,
                         acp_char_t   *aPKSchemaName,
                         acp_sint16_t  aNameLength2,
                         acp_char_t   *aPKTableName,
                         acp_sint16_t  aNameLength3,
                         acp_char_t   *aFKCatalogName,
                         acp_sint16_t  aNameLength4,
                         acp_char_t   *aFKSchemaName,
                         acp_sint16_t  aNameLength5,
                         acp_char_t   *aFKTableName,
                         acp_sint16_t  aNameLength6)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext sFnContext;
    acp_char_t sQueryStringBuffer[10240];   /* BUGBUG : 크기는 충분한가? */

    ACP_UNUSED(aPKCatalogName);
    ACP_UNUSED(aNameLength1);
    ACP_UNUSED(aFKCatalogName);
    ACP_UNUSED(aNameLength4);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FOREIGNKEYS, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST( aStmt == NULL );       //BUG-28623 [CodeSonar]Null Pointer Dereference

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(aStmt->mParentDbc)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * BUGBUG : Argument validity checking 을 수행해야 한다.
     */

    ACI_TEST(ulnFKCreateQueryString(&sFnContext,
                                    aPKSchemaName,
                                    aNameLength2,
                                    aPKTableName,
                                    aNameLength3,
                                    aFKSchemaName,
                                    aNameLength5,
                                    aFKTableName,
                                    aNameLength6,
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
    ACI_TEST(ulnExecuteCore(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);
    //fix BUG-17722
    ACI_TEST(ulnFlushAndReadProtocol(&sFnContext,
                                     &(aStmt->mParentDbc->mPtContext),
                                     aStmt->mParentDbc->mConnTimeoutValue) != ACI_SUCCESS);

    /*
     * Protocol Context 정리
     */
    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                                  &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

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
