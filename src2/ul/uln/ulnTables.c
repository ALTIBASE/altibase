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

#define ULN_TABLE_TYPE_SYSTEM_TABLE 0x01
#define ULN_TABLE_TYPE_SYSTEM_VIEW  0x02    /* BUG-33915 system view */
#define ULN_TABLE_TYPE_TABLE        0x04
#define ULN_TABLE_TYPE_VIEW         0x08
#define ULN_TABLE_TYPE_QUEUE        0x10
#define ULN_TABLE_TYPE_SYNONYM      0x20
#define ULN_TABLE_TYPE_MVIEW_TABLE  0x40    /* PROJ-2211 Materialized View */
#define ULN_TABLE_TYPE_TEMP_TABLE   0x80    /* BUG-40103 missing 'GLOBAL TEMPORARY' table type in SQLTables */
#define ULN_TABLE_TYPE_ALL          0xff


// add PROJ-1349 a3export -> a4export
// c.tbs_name           : 7
// c.tbs_type           : 8
// b.PCTFREE            : 9
// b.PCTUSED            : 10

typedef struct ulnTablesQuery
{
    acp_uint8_t       mTableTypeMask;
    const acp_char_t *mUserNameConditionFormat;
    const acp_char_t *mTableNameConditionFormat;
    const acp_char_t *mQueryString;
} ulnTablesQuery;

// To Fix BUG-17681
// SQLTables()함수는 Catalog Function으로 인자로 패턴 문자를 사용할 수 있음.
static ulnTablesQuery gTablesQuery[] =
{
    {
        ULN_TABLE_TYPE_SYSTEM_TABLE,
        " and a.USER_NAME like '%s'",
        " and b.TABLE_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," // BUG-17786
        "a.USER_NAME as TABLE_SCHEM,"
        "b.TABLE_NAME as TABLE_NAME,"
        "VARCHAR'SYSTEM TABLE' as TABLE_TYPE,"
        "b.TABLE_NAME as REMARKS,"
        "b.MAXROW as MAXROW,"
        "c.NAME as TABLESPACE_NAME,"
        "c.TYPE as TABLESPACE_TYPE,"
        "b.PCTFREE as PCTFREE,"                                  // BUG-23652
        "b.PCTUSED as PCTUSED "
        "from "
        "SYSTEM_.SYS_USERS_ a,"
        "SYSTEM_.SYS_TABLES_ b,"
        "X$TABLESPACES_HEADER c "
        "where "
        "a.USER_ID = b.USER_ID and "
        "a.USER_NAME = 'SYSTEM_' and "
        "b.TBS_ID = c.ID and "
        "b.TABLE_TYPE = 'T' and "
        "b.HIDDEN ='N'"                                           // PROJ-1624 Global Non-partitioned Index
    }, 
    // BUG-33915 system view  
    {
        ULN_TABLE_TYPE_SYSTEM_VIEW,
        " and a.USER_NAME like '%s'",
        " and b.TABLE_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," // BUG-17786
        "a.USER_NAME as TABLE_SCHEM,"
        "b.TABLE_NAME as TABLE_NAME,"
        "VARCHAR'SYSTEM VIEW' as TABLE_TYPE,"
        "b.TABLE_NAME as REMARKS,"
        "b.MAXROW as MAXROW,"
        "c.NAME as TABLESPACE_NAME,"
        "c.TYPE as TABLESPACE_TYPE,"
        "b.PCTFREE as PCTFREE,"                                  // BUG-23652
        "b.PCTUSED as PCTUSED "
        "from "
        "SYSTEM_.SYS_USERS_ a,"
        "SYSTEM_.SYS_TABLES_ b,"
        "X$TABLESPACES_HEADER c "
        "where "
        "a.USER_ID = b.USER_ID and "
        "a.USER_NAME = 'SYSTEM_' and "
        "b.TBS_ID = c.ID and "
        "b.TABLE_TYPE = 'V' and "
        "b.HIDDEN = 'N'"
    },
    {
        ULN_TABLE_TYPE_TABLE,
        " and a.USER_NAME like '%s'",
        " and b.TABLE_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," // BUG-17786
        "a.USER_NAME as TABLE_SCHEM,"
        "b.TABLE_NAME as TABLE_NAME,"
        "VARCHAR'TABLE' as TABLE_TYPE,"
        "b.TABLE_NAME as REMARKS,"
        "b.MAXROW as MAXROW,"
        "c.NAME as TABLESPACE_NAME,"
        "c.TYPE as TABLESPACE_TYPE,"
        "b.PCTFREE as PCTFREE,"                                  // BUG-23652
        "b.PCTUSED as PCTUSED "
        "from "
        "SYSTEM_.SYS_USERS_ a,"
        "SYSTEM_.SYS_TABLES_ b,"
        "X$TABLESPACES_HEADER c "
        "where "
        "a.USER_ID = b.USER_ID and "
        "a.USER_NAME <> 'SYSTEM_' and "
        "b.TBS_ID = c.ID and "
        "b.TABLE_TYPE = 'T' and "
        "b.HIDDEN = 'N' and "
        "b.TEMPORARY = 'N'"                                      // BUG-40103
    },
    {
        ULN_TABLE_TYPE_VIEW,
        " and a.USER_NAME like '%s'",
        " and b.TABLE_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," // BUG-17786
        "a.USER_NAME as TABLE_SCHEM,"
        "b.TABLE_NAME as TABLE_NAME,"
        "VARCHAR'VIEW' as TABLE_TYPE,"
        "b.TABLE_NAME as REMARKS,"
        "b.MAXROW as MAXROW,"
        "c.NAME as TABLESPACE_NAME,"
        "c.TYPE as TABLESPACE_TYPE,"
        "b.PCTFREE as PCTFREE,"                                  // BUG-23652
        "b.PCTUSED as PCTUSED "
        "from "
        "SYSTEM_.SYS_USERS_ a,"
        "SYSTEM_.SYS_TABLES_ b,"
        "X$TABLESPACES_HEADER c "
        "where "
        "a.USER_ID = b.USER_ID and "
        "a.USER_NAME <> 'SYSTEM_' and "                           /* PROJ-2207 Password policy support */
        "b.TBS_ID = c.ID and "
        "b.TABLE_TYPE = 'V' and "
        "b.HIDDEN = 'N'"
    },
    {
        ULN_TABLE_TYPE_QUEUE,
        " and a.USER_NAME like '%s'",
        " and b.TABLE_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," // BUG-17786
        "a.USER_NAME as TABLE_SCHEM,"
        "b.TABLE_NAME as TABLE_NAME,"
        "VARCHAR'QUEUE' as TABLE_TYPE,"
        "b.TABLE_NAME as REMARKS,"
        "b.MAXROW as MAXROW,"
        "c.NAME as TABLESPACE_NAME,"
        "c.TYPE as TABLESPACE_TYPE,"
        "b.PCTFREE as PCTFREE,"                                  // BUG-23652
        "b.PCTUSED as PCTUSED "
        "from "
        "SYSTEM_.SYS_USERS_ a,"
        "SYSTEM_.SYS_TABLES_ b,"
        "X$TABLESPACES_HEADER c "
        "where "
        "a.USER_ID = b.USER_ID and "
        "b.TBS_ID = c.ID and "
        "b.TABLE_TYPE = 'Q'"
    },
    {
        ULN_TABLE_TYPE_SYNONYM,
        " and (a.USER_NAME like '%s' or b.SYNONYM_OWNER_ID IS NULL)",
        " and b.SYNONYM_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," // BUG-17786
        "a.USER_NAME as TABLE_SCHEM,"
        "b.SYNONYM_NAME as TABLE_NAME,"
        "VARCHAR'SYNONYM' as TABLE_TYPE,"
        "b.OBJECT_NAME as REMARKS,"
        "0 as MAXROW,"
        "NULL as TABLESPACE_NAME,"
        "NULL as TABLESPACE_TYPE,"
        "0 as PCTFREE,"                                          // BUG-23652
        "0 as PCTUSED "
        "from "
        "SYSTEM_.SYS_SYNONYMS_ b left outer join "
        "SYSTEM_.SYS_USERS_ a on a.USER_ID = b.SYNONYM_OWNER_ID "
        "where "
        "b.SYNONYM_NAME is not null"
    },
    /* PROJ-2211 Materialized View */
    {
        ULN_TABLE_TYPE_MVIEW_TABLE,
        " and a.USER_NAME like '%s'",
        " and b.TABLE_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," /* BUG-17786 */
        "a.USER_NAME as TABLE_SCHEM,"
        "b.TABLE_NAME as TABLE_NAME,"
        "VARCHAR'MATERIALIZED VIEW' as TABLE_TYPE,"
        "b.TABLE_NAME as REMARKS,"
        "b.MAXROW as MAXROW,"
        "c.NAME as TABLESPACE_NAME,"
        "c.TYPE as TABLESPACE_TYPE,"
        "b.PCTFREE as PCTFREE,"                                  /* BUG-23652 */
        "b.PCTUSED as PCTUSED "
        "from "
        "SYSTEM_.SYS_USERS_ a,"
        "SYSTEM_.SYS_TABLES_ b,"
        "X$TABLESPACES_HEADER c "
        "where "
        "a.USER_ID = b.USER_ID and "
        "a.USER_NAME <> 'SYSTEM_' and "
        "b.TBS_ID = c.ID and "
        "b.TABLE_TYPE = 'M' and "
        "b.HIDDEN = 'N'"
    },
    /* BUG-40103 missing 'GLOBAL TEMPORARY' table type in SQLTables */
    {
        ULN_TABLE_TYPE_TEMP_TABLE,
        " and a.USER_NAME like '%s'",
        " and b.TABLE_NAME like '%s'",
        "select "
        "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT," // BUG-17786
        "a.USER_NAME as TABLE_SCHEM,"
        "b.TABLE_NAME as TABLE_NAME,"
        "VARCHAR'GLOBAL TEMPORARY' as TABLE_TYPE,"
        "b.TABLE_NAME as REMARKS,"
        "b.MAXROW as MAXROW,"
        "c.NAME as TABLESPACE_NAME,"
        "c.TYPE as TABLESPACE_TYPE,"
        "b.PCTFREE as PCTFREE,"                                  // BUG-23652
        "b.PCTUSED as PCTUSED "
        "from "
        "SYSTEM_.SYS_USERS_ a,"
        "SYSTEM_.SYS_TABLES_ b,"
        "X$TABLESPACES_HEADER c "
        "where "
        "a.USER_ID = b.USER_ID and "
        "a.USER_NAME <> 'SYSTEM_' and "
        "b.TBS_ID = c.ID and "
        "b.TABLE_TYPE = 'T' and "
        "b.HIDDEN = 'N' and "
        "b.TEMPORARY IN ('D', 'P')"
    },
    {
        0,
        NULL,
        NULL,
        NULL
    }
};

// bug-25043 SQLTables()에서 SQL_ALL_CATALOGS("%")를 지원해야 한다
// catalog list를 select하는 sql: mydb 하나밖에 없다
// catalog 외의 다른 column은 모두 NULL을 반환하도록 한다
static const acp_char_t gSelectAllCatalogsSQL[] =
"SELECT "
"(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"NULL AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL";

// schema(user) list를 select하는 sql
// select 결과는 다음과 같다
// ex) sys, system_, user1
static const acp_char_t gSelectAllSchemasSQL[] =
"SELECT "
"NULL AS TABLE_CAT,"
"USER_NAME AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"NULL AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM SYSTEM_.SYS_USERS_ "
"ORDER BY 2";

// table type list를 select하는 sql
// select 결과는 다음과 같다
// queue, synonym, system table, system view, table, view, materialized view
static const acp_char_t gSelectAllTableTypesSQL[] =
// system table
"SELECT "
"NULL AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"VARCHAR'SYSTEM TABLE' AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL "
// BUG-33915 system view     
"UNION ALL "
"SELECT "
"NULL AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"VARCHAR'SYSTEM VIEW' AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL "
// table
"UNION ALL "
"SELECT "
"NULL AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"VARCHAR'TABLE' AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL "
// view
"UNION ALL "
"SELECT "
"NULL AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"VARCHAR'VIEW' AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL "
// queue
"UNION ALL "
"SELECT "
"NULL AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"VARCHAR'QUEUE' AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL "
// synonym
"UNION ALL "
"SELECT "
"NULL AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"VARCHAR'SYNONYM' AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL "
/* PROJ-2211 Materialized View */
"UNION ALL "
"SELECT "
"NULL AS TABLE_CAT,"
"NULL AS TABLE_SCHEM,"
"NULL AS TABLE_NAME,"
"VARCHAR'MATERIALIZED VIEW' AS TABLE_TYPE,"
"NULL AS REMARKS "
"FROM DUAL "
"ORDER BY 4";

static acp_uint8_t ulnGetTableTypeMask(acp_char_t *aTableType)
{
    acp_uint8_t   sTableTypeMask = 0;
    acp_char_t   *sTempPtr1;
    acp_sint32_t  sTempIndex = 0;
    acp_char_t   *sTempPtr2;

    sTempPtr1 = aTableType;
    sTempPtr2 = aTableType;

    while(sTempPtr1 != NULL)
    {
        if (acpCStrFindChar(sTempPtr2, ',', &sTempIndex, 0, 0) == ACP_RC_SUCCESS)
        {
            sTempPtr1 = sTempPtr2 + sTempIndex;
            *sTempPtr1 = 0;
        }
        else
        {
            sTempPtr1 = NULL;
        }

        /*
         * Ignore Whitespace
         */
        while((*sTempPtr2 != 0) && (acpCharIsSpace(*sTempPtr2) != ACP_FALSE))
        {
            sTempPtr2++;
        }

        if(acpCStrCmp(sTempPtr2, SQL_ALL_TABLE_TYPES, acpCStrLen(SQL_ALL_TABLE_TYPES, ACP_SINT32_MAX)) == 0)
        {
            sTableTypeMask = ULN_TABLE_TYPE_ALL;
        }
        else if(acpCStrCmp(sTempPtr2, "SYSTEM TABLE", 12) == 0 ||
                acpCStrCmp(sTempPtr2, "'SYSTEM TABLE'", 14) == 0)
        {
            sTableTypeMask |= ULN_TABLE_TYPE_SYSTEM_TABLE;
        }
        // BUG-33915 system view     
        else if(acpCStrCmp(sTempPtr2, "SYSTEM VIEW", 11) == 0 ||
                acpCStrCmp(sTempPtr2, "'SYSTEM VIEW'", 13) == 0)
        {
            sTableTypeMask |= ULN_TABLE_TYPE_SYSTEM_VIEW;
        }
        else if(acpCStrCmp(sTempPtr2, "TABLE", 5) == 0 ||
                acpCStrCmp(sTempPtr2, "'TABLE'", 7) == 0)
        {
            sTableTypeMask |= ULN_TABLE_TYPE_TABLE;
        }
        else if(acpCStrCmp(sTempPtr2, "VIEW", 4) == 0 ||
                acpCStrCmp(sTempPtr2, "'VIEW'", 6) == 0)
        {
            sTableTypeMask |= ULN_TABLE_TYPE_VIEW;
        }
        else if(acpCStrCmp(sTempPtr2, "QUEUE", 5) == 0 ||
                acpCStrCmp(sTempPtr2, "'QUEUE'", 7) == 0)
        {
            sTableTypeMask |= ULN_TABLE_TYPE_QUEUE;
        }
        else if(acpCStrCmp(sTempPtr2, "SYNONYM", 7) == 0 ||
                 acpCStrCmp(sTempPtr2, "'SYNONYM'", 9) == 0)
        {
            sTableTypeMask |= ULN_TABLE_TYPE_SYNONYM;
        }
        /* PROJ-2211 Materialized View */
        else if ( ( acpCStrCmp( sTempPtr2, "MATERIALIZED VIEW", 17 ) == 0 ) ||
                  ( acpCStrCmp( sTempPtr2, "'MATERIALIZED VIEW'", 19 ) == 0 ) )
        {
            sTableTypeMask |= ULN_TABLE_TYPE_MVIEW_TABLE;
        }
        /* BUG-40103 missing 'GLOBAL TEMPORARY' table type in SQLTables */
        else if ( ( acpCStrCmp( sTempPtr2, "GLOBAL TEMPORARY", 16 ) == 0 ) ||
                  ( acpCStrCmp( sTempPtr2, "'GLOBAL TEMPORARY'", 18 ) == 0 ) )
        {
            sTableTypeMask |= ULN_TABLE_TYPE_TEMP_TABLE;
        }

        if(sTempPtr1 == NULL)
        {
            break;
        }

        sTempPtr1++;
        sTempPtr2 = sTempPtr1;
    }

    return sTableTypeMask;
}

static ACI_RC ulnTablesCreateQueryString(ulnFnContext *aFnContext,
                                         acp_char_t   *aCatalogName,
                                         acp_sint16_t  aNameLength1,
                                         acp_char_t   *aSchemaName,
                                         acp_sint16_t  aNameLength2,
                                         acp_char_t   *aTableName,
                                         acp_sint16_t  aNameLength3,
                                         acp_char_t   *aTableType,
                                         acp_sint16_t  aNameLength4,
                                         acp_char_t   *aQueryString,
                                         acp_uint32_t  aQueryStringSize)
{

    acp_uint8_t   sTableTypeMask;

    acp_char_t    sUserName[ULN_CATALOG_MAX_NAME_LEN];
    acp_char_t    sTableName[ULN_CATALOG_MAX_NAME_LEN];
    acp_char_t    sTableType[ULN_CATALOG_MAX_TYPE_LEN]; // To Fix BUG-17785
    acp_sint32_t  i;

    ulnDbc       *sDbc = NULL;
    ACP_UNUSED(aNameLength1);

    // bug-25905: conn nls not applied to client lang module
    ACI_TEST_RAISE(aFnContext->mHandle.mStmt == NULL, ERR_HY009);
    sDbc = aFnContext->mHandle.mStmt->mParentDbc;

    // bug-25043 SQLTables()에서 SQL_ALL_CATALOGS("%")를 지원해야 한다
    // catalog 목록을 조회하는 경우 excel에서 다음과 같이 호출한다
    // SQLTables(stmt, "%", 1, "", 0, "", 0, NULL, 0)
    *aQueryString = 0;
    // catalog 목록을 조회하는 경우
    if ((aCatalogName != NULL) &&
        (acpCStrCmp(aCatalogName, SQL_ALL_CATALOGS,
                    acpCStrLen(SQL_ALL_CATALOGS, ACP_SINT32_MAX)) == 0))
    {
        aciVaAppendFormat(aQueryString,
                          aQueryStringSize,
                          "%s",
                          gSelectAllCatalogsSQL);
        ACI_RAISE(label_success);
    }
    // schema 목록을 조회하는 경우
    else if ((aSchemaName != NULL) &&
             (acpCStrCmp(aSchemaName, SQL_ALL_SCHEMAS,
                         acpCStrLen(SQL_ALL_SCHEMAS, ACP_SINT32_MAX)) == 0))
    {
        aciVaAppendFormat(aQueryString,
                          aQueryStringSize,
                          "%s",
                          gSelectAllSchemasSQL);
        ACI_RAISE(label_success);
    }
    // table type 목록을 조회하는 경우
    else if ((aTableType != NULL) &&
             (acpCStrCmp(aTableType, SQL_ALL_TABLE_TYPES,
                         acpCStrLen(SQL_ALL_TABLE_TYPES, ACP_SINT32_MAX)) == 0))
    {
        aciVaAppendFormat(aQueryString,
                          aQueryStringSize,
                          "%s",
                          gSelectAllTableTypesSQL);
        ACI_RAISE(label_success);
    }
    else
    {
        // normal case
    }

    /*
     * Copy User Name
     */

    // bug-25905: conn nls not applied to client lang module
    // aMtlModule 인자 추가
    ACI_TEST_RAISE(ulnMakeNullTermNameInSQL(sDbc->mClientCharsetLangModule,
                                            sUserName,
                                            ACI_SIZEOF(sUserName),
                                            aSchemaName,
                                            aNameLength2,
                                            NULL )
                   != ACI_SUCCESS, LABEL_INVALID_BUF_LEN2);

    /*
     * Copy Table Name
     */

    // bug-25905: conn nls not applied to client lang module
    ACI_TEST_RAISE(ulnMakeNullTermNameInSQL(sDbc->mClientCharsetLangModule,
                                            sTableName,
                                            ACI_SIZEOF(sTableName),
                                            aTableName,
                                            aNameLength3,
                                            NULL)
                   != ACI_SUCCESS, LABEL_INVALID_BUF_LEN3);


    /*
     * Copy Table Type
     */

    // bug-25905: conn nls not applied to client lang module
    ACI_TEST_RAISE(ulnMakeNullTermNameInSQL(sDbc->mClientCharsetLangModule,
                                            sTableType,
                                            ACI_SIZEOF(sTableType),
                                            aTableType,
                                            aNameLength4,
                                            NULL)
                   != ACI_SUCCESS, LABEL_INVALID_BUF_LEN4);

    /*
     * Get Table Type
     */

    sTableTypeMask = ulnGetTableTypeMask(sTableType);

    if(sTableTypeMask == 0)
    {
        // To Fix BUG-17681
        // 가능한 모든 TYPE을 의미함
        sTableTypeMask = ULN_TABLE_TYPE_ALL;
    }

    /*
     * Generate Query String
     */

    for(i = 0; gTablesQuery[i].mTableTypeMask != 0; i++)
    {
        if(sTableTypeMask & gTablesQuery[i].mTableTypeMask)
        {
            /*
             * Append 'union all' When the Query already has one or more query
             */
            if(*aQueryString != 0)
            {
                aciVaAppendFormat(aQueryString,
                                  aQueryStringSize,
                                  " union all ");
            }

            /*
             * Append the Query
             */
            aciVaAppendFormat(aQueryString,
                              aQueryStringSize,
                              "%s",
                              gTablesQuery[i].mQueryString);

            /*
             * Append User Name Condition
             */
            if(sUserName[0] != 0)
            {
                aciVaAppendFormat(aQueryString,
                                  aQueryStringSize,
                                  gTablesQuery[i].mUserNameConditionFormat,
                                  sUserName);
            }

            /*
             * Append Table Name Condition
             */
            if(sTableName[0] != 0)
            {
                aciVaAppendFormat(aQueryString,
                                  aQueryStringSize,
                                  gTablesQuery[i].mTableNameConditionFormat,
                                  sTableName);
            }
        }
    }

    /*
     * Append 'order by' Clause
     */
    aciVaAppendFormat(aQueryString,
                      aQueryStringSize,
                      " order by 4,1,2,3");

    ACI_EXCEPTION_CONT(label_success);

    return ACI_SUCCESS;

    // bug-25905: conn nls not applied to client lang module
    // error 처리 if -> ACI_TEST_RAISE 변경
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
    ACI_EXCEPTION(LABEL_INVALID_BUF_LEN4)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength4);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnTablesCheckArgs(ulnFnContext *aFnContext,
                                 acp_char_t   *aCatalogName,
                                 acp_sint16_t  aNameLength1,
                                 acp_char_t   *aSchemaName,
                                 acp_sint16_t  aNameLength2,
                                 acp_char_t   *aTableName,
                                 acp_sint16_t  aNameLength3,
                                 acp_char_t   *aTableType,
                                 acp_sint16_t  aNameLength4)
{
    ACP_UNUSED(aCatalogName);
    ACP_UNUSED(aSchemaName);
    ACP_UNUSED(aTableName);
    ACP_UNUSED(aTableType);

    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength1, ULN_CATALOG_MAX_NAME_LEN) != ACI_SUCCESS, LABEL_INVALID_BUFFER_LEN1 );

    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength2, ULN_CATALOG_MAX_NAME_LEN) != ACI_SUCCESS, LABEL_INVALID_BUFFER_LEN2 );

    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength3, ULN_CATALOG_MAX_NAME_LEN) != ACI_SUCCESS, LABEL_INVALID_BUFFER_LEN3 );

    // To Fix BUG-17785
    // Value List 가 올 수 있으므로 보다 큰 Size의 허용치가 필요함.
    ACI_TEST_RAISE( ulnCheckStringLength(aNameLength4, ULN_CATALOG_MAX_TYPE_LEN) != ACI_SUCCESS, LABEL_INVALID_BUFFER_LEN4 );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN1)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength1);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN2)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength2);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN3)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength3);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN4)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aNameLength4);
    }        
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnTables(ulnStmt      *aStmt,
                    acp_char_t   *aCatalogName,
                    acp_sint16_t  aNameLength1,
                    acp_char_t   *aSchemaName,
                    acp_sint16_t  aNameLength2,
                    acp_char_t   *aTableName,
                    acp_sint16_t  aNameLength3,
                    acp_char_t   *aTableType,
                    acp_sint16_t  aNameLength4)
{
    acp_bool_t   sNeedExit = ACP_FALSE;
    acp_bool_t   sNeedFinPtContext = ACP_FALSE;

    ulnFnContext sFnContext;

    acp_char_t   sQueryString[ULN_CATALOG_QUERY_STR_BUF_SIZE];


    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_TABLES, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    /*
     * Check if the arguments are valid
     */
    ACI_TEST(ulnTablesCheckArgs(&sFnContext,
                                aCatalogName,
                                aNameLength1,
                                aSchemaName,
                                aNameLength2,
                                aTableName,
                                aNameLength3,
                                aTableType,
                                aNameLength4) != ACI_SUCCESS);

    /*
     * Create query string for getting table information from DB
     */
    ACI_TEST(ulnTablesCreateQueryString(&sFnContext,
                                        aCatalogName,
                                        aNameLength1,
                                        aSchemaName,
                                        aNameLength2,
                                        aTableName,
                                        aNameLength3,
                                        aTableType,
                                        aNameLength4,
                                        sQueryString,
                                        ACI_SIZEOF(sQueryString)) != ACI_SUCCESS);

    /*
     * Protocol Context 초기화
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);

    sNeedFinPtContext = ACP_TRUE;

    /*
     * Prepare Phase
     */
    //fix BUG-17722
    ACI_TEST(ulnPrepareCore(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext),
                            sQueryString,
                            acpCStrLen(sQueryString, ULN_CATALOG_QUERY_STR_BUF_SIZE),
                            CMP_DB_PREPARE_MODE_EXEC_DIRECT) != ACI_SUCCESS);

    /*
     * Execute Phase
     */
    //fix BUG-17722
    ACI_TEST(ulnExecuteCore(&sFnContext,&(aStmt->mParentDbc->mPtContext) )
                 != ACI_SUCCESS);
    //fix BUG-17722
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

