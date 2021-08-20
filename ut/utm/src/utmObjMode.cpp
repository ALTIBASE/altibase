/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id: utmObjMode.cpp $
 **********************************************************************/

#include <smDef.h>
#include <utm.h>
#include <utmExtern.h>
#include <utmDbStats.h>

/* BUG-36367 aexport must consider new objects, 'package' and 'library' */
#define GET_OBJTYPE_QUERY_PACKAGE_LIBRARY                               \
    "UNION "                                                            \
    "SELECT a.user_id, a.package_oid AS obj_id, "                       \
    "CASE2( a.package_type = 6, 7, a.package_type = 7, 8 ) AS type "    \
    "FROM "                                                             \
         "( "                                                           \
            "SELECT user_id, package_oid, package_type "                \
            "FROM system_.sys_packages_ "                               \
            "WHERE package_name = '%s' "                                \
         ") a, system_.sys_users_ b "                                   \
    "WHERE b.user_name = '%s' "                                         \
      "AND b.user_id = a.user_id "                                      \
    "UNION "                                                            \
    "SELECT lib.user_id, lib.library_id AS obj_id, 9 AS type "          \
    "FROM "                                                             \
         "( "                                                           \
            "SELECT user_id, library_id "                               \
            "FROM system_.sys_libraries_ "                              \
            "WHERE library_name = '%s' "                                \
         ") lib, system_.sys_users_ users "                             \
    "WHERE lib.user_id = users.user_id "                                \
          "AND users.user_name = '%s'"

#define GET_OBJTYPE_QUERY                                               \
    "select/*+ NO_PLAN_CACHE */ "                                       \
    "  a.user_id, a.proc_oid as obj_id, '1' as type "                   \
    "from ( "                                                           \
    "select user_id, proc_oid  from system_.sys_procedures_ "           \
    "where proc_name='%s' "                                             \
    ") a, system_.sys_users_ b "                                        \
    "where b.user_name='%s' "                                           \
    "and b.user_id = a.user_id "                                        \
    "union "                                                            \
    "select a.user_id, a.table_id as obj_id, "                          \
    "case2( table_type = 'V', '2', "                                    \
    "       table_type = 'S', '3', "                                    \
    "       table_type = 'T', '4', "                                    \
    "       table_type = 'M', '6', "                                    \
    "       table_type = 'Q', '10' ) "                                  \
    "as type "                                                          \
    "from ( "                                                           \
    "select user_id, table_id,table_type "                              \
    "from system_.sys_tables_ "                                         \
    "where table_name='%s' "                                            \
    ") a, system_.sys_users_ b "                                        \
    "where b.user_name='%s' "                                           \
    "and b.user_id = a.user_id "                                        \
    GET_OBJTYPE_QUERY_PACKAGE_LIBRARY

#define GET_OBJMODE_TABLE_QUERY                                         \
    "select /*+ USE_HASH(B, C) USE_HASH(B, D) NO_PLAN_CACHE */ "        \
    "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT, "           \
    "a.USER_NAME as TABLE_SCHEM, "                                      \
    "b.TABLE_NAME as TABLE_NAME, "                                      \
    "VARCHAR'TABLE' as TABLE_TYPE, "                                    \
    "b.TABLE_NAME as REMARKS, "                                         \
    "b.MAXROW as MAXROW, "                                              \
    "c.NAME as TABLESPACE_NAME, "                                       \
    "c.TYPE as TABLESPACE_TYPE, "                                       \
    "b.PCTFREE as PCTFREE, "                                            \
    "b.PCTUSED as PCTUSED, "                                            \
    "d.TABLE_TYPE as TABLE_TYPE2 "                                      \
    "from "                                                             \
    "SYSTEM_.SYS_USERS_ a, "                                            \
    "SYSTEM_.SYS_TABLES_ b, "                                           \
    "X$TABLESPACES_HEADER c, "                                          \
    "X$TABLE_INFO d "                                                   \
    "where "                                                            \
    "a.USER_ID = b.USER_ID and "                                        \
    "a.USER_NAME <> 'SYSTEM_' and "                                     \
    "b.TBS_ID = c.ID and "                                              \
    "b.TABLE_OID = d.TABLE_OID and "                                    \
    "b.TABLE_TYPE = 'T' "                                               \
    "and a.user_name='%s' "                                             \
    "and b.table_name='%s' "

/* BUG-49242 Queue object mode */
#define GET_OBJMODE_QUEUE_QUERY                                         \
    "select /*+ USE_HASH(B, C) USE_HASH(B, D) NO_PLAN_CACHE */ "        \
    "(SELECT DB_NAME FROM V$DATABASE LIMIT 1) AS TABLE_CAT, "           \
    "a.USER_NAME as TABLE_SCHEM, "                                      \
    "b.TABLE_NAME as TABLE_NAME, "                                      \
    "c.NAME as TABLESPACE_NAME, "                                       \
    "c.TYPE as TABLESPACE_TYPE "                                        \
    "from "                                                             \
    "SYSTEM_.SYS_USERS_ a, "                                            \
    "SYSTEM_.SYS_TABLES_ b, "                                           \
    "X$TABLESPACES_HEADER c "                                           \
    "where "                                                            \
    "a.USER_ID = b.USER_ID and "                                        \
    "a.USER_NAME <> 'SYSTEM_' and "                                     \
    "b.TBS_ID = c.ID and "                                              \
    "b.TABLE_TYPE = 'Q' "                                               \
    "and a.user_name='%s' "                                             \
    "and b.table_name='%s' "

#define GET_OBJMODE_VIEW_QUERY1                                         \
    "select/*+ NO_PLAN_CACHE */ "                                       \
    "  c.USER_NAME, a.VIEW_ID, a.SEQ_NO, a.PARSE,"                      \
    "  b.table_name, d.status "                                         \
    "from system_.SYS_VIEW_PARSE_ a, system_.sys_tables_ b,"            \
    "  system_.SYS_USERS_ c,system_.sys_views_ d "                      \
    "where a.view_id=b.table_id and b.USER_ID=c.USER_ID "               \
    "  and b.table_type='V' and a.view_id=d.view_id "                   \
    "  and a.view_id in (select table_id "                              \
    "    from system_.sys_view_related_ a,system_.sys_tables_ b "       \
    "    where a.related_user_id=b.user_id "                            \
    "      and a.related_object_name=b.table_name "                     \
    "      and a.related_object_type=2 "                                \
    "      and b.table_type='V') "

#define GET_OBJMODE_VIEW_QUERY2                                         \
    "select/*+ NO_PLAN_CACHE */ "                                       \
    "  c.USER_NAME, a.VIEW_ID, a.SEQ_NO, a.PARSE,"                      \
    "  b.table_name, d.status "                                         \
    "from system_.SYS_VIEW_PARSE_ a, system_.sys_tables_ b,"            \
    "  system_.SYS_USERS_ c,system_.sys_views_ d "                      \
    "where a.view_id=b.table_id and b.USER_ID=c.USER_ID "               \
    "  and b.table_type='V' and a.view_id=d.view_id "                   \
    "  and a.view_id not in (select table_id "                          \
    "    from system_.sys_view_related_ a,system_.sys_tables_ b "       \
    "    where a.related_user_id=b.user_id "                            \
    "      and a.related_object_name=b.table_name "                     \
    "      and a.related_object_type=2 "                                \
    "      and b.table_type='V') "    

/* PROJ-2211 Materialized View */
#define GET_OBJMODE_MVIEW_QUERY                                         \
    "select /*+ NO_PLAN_CACHE */ "                                      \
    "  c.USER_NAME, a.VIEW_ID, a.SEQ_NO, a.PARSE,"                      \
    "  b.table_name, d.status "                                         \
    "from system_.SYS_VIEW_PARSE_ a, system_.sys_tables_ b,"            \
    "  system_.SYS_USERS_ c,system_.sys_views_ d "                      \
    "where a.view_id=b.table_id and b.USER_ID=c.USER_ID "               \
    "  and b.table_type='A' and a.view_id=d.view_id "

#define GET_OBJMODE_PROC_QUERY1                                         \
    "select/*+ NO_PLAN_CACHE */ "                                       \
    "  c.USER_NAME,a.PROC_OID,a.SEQ_NO,a.PARSE,"                        \
    "  b.proc_name,b.object_type "                                      \
    "from system_.SYS_PROC_PARSE_ a, system_.sys_procedures_ b,"        \
    "  system_.SYS_USERS_ c "                                           \
    "where a.proc_oid=b.proc_oid and b.USER_ID=c.USER_ID "              \
    "  and a.proc_oid not in (select distinct proc_oid "                \
    "    from system_.sys_proc_related_ "                               \
    "    where related_object_type=0)" 

#define GET_OBJMODE_PROC_QUERY2                                         \
    "select/*+ NO_PLAN_CACHE */ "                                       \
    "  c.USER_NAME, a.PROC_OID,a.SEQ_NO,a.PARSE,"                       \
    "  b.proc_name,b.object_type "                                      \
    "from system_.SYS_PROC_PARSE_ a, system_.sys_procedures_ b,"        \
    "  system_.SYS_USERS_ c "                                           \
    "where a.proc_oid=b.proc_oid and b.USER_ID=c.USER_ID "              \
    "  and a.proc_oid in (select distinct proc_oid "                    \
    "    from system_.sys_proc_related_ "                               \
    "    where related_object_type=0)"

/* BUG-36367 aexport must consider new objects, 'package' and 'library' */
#define GET_OBJMODE_PKG_QUERY                                                  \
    "SELECT /*+ NO_PLAN_CACHE */ users.user_name, parse.package_oid, "         \
           "parse.seq_no, parse.parse, pkg.package_name, "                     \
           "CASE2(pkg.package_type = 6, 7, pkg.package_type = 7, 8) AS type "  \
    "FROM system_.sys_package_parse_ parse, system_.sys_packages_ pkg, "       \
         "system_.sys_users_ users "                                           \
    "WHERE parse.package_oid = pkg.package_oid "                               \
      "AND pkg.user_id = users.user_id "                                       \

/* Object Mode 인 경우에는 Connect 구문이 추가 되지 않는다.
 * 단, 해당 object 의 권한은 connect 구문이 추가 된다.
 * objectModeInfoQuery -> getSeqQuery(), getObjModeTableQuery(),
 * getObjModeProcQuery(),getObjModeViewQuery() -> searchObjPrivQuery()
 * */
SQLRETURN objectModeInfoQuery()
{
#define IDE_FN "objectModeInfoQuery()"
    SQLHSTMT  sStmt = SQL_NULL_HSTMT;
    SQLRETURN sRet  = 0;
    SChar     sQuery[QUERY_LEN*2];
    SInt      sUserId   = 0;
    SLong     sObjId    = 0;
    SInt      sObjType  = 0;
    SInt      i;

    /* BUG-46292 */
    SQLBIGINT sObjId2Bind  = SQLBIGINT_INIT_TO_ZERO;

    for ( i = 0 ; i < gProgOption.mObjModeOptCount; i++ )
    {
        IDE_TEST_RAISE( SQLAllocStmt( m_hdbc, &sStmt )
                                      != SQL_SUCCESS, alloc_error );
        
        /* BUG-36367 aexport must consider new objects, 'package' and 'library' */
        idlOS::sprintf( sQuery,
                        GET_OBJTYPE_QUERY,
                        gObjectModeInfo[i].mObjObjectName, gObjectModeInfo[i].mObjUserName,
                        gObjectModeInfo[i].mObjObjectName, gObjectModeInfo[i].mObjUserName,
                        gObjectModeInfo[i].mObjObjectName, gObjectModeInfo[i].mObjUserName,
                        gObjectModeInfo[i].mObjObjectName, gObjectModeInfo[i].mObjUserName );

        IDE_TEST_RAISE( SQLExecDirect( sStmt, (SQLCHAR *)sQuery, SQL_NTS)
                != SQL_SUCCESS, notExist );
        
        IDE_TEST_RAISE(
            SQLBindCol(sStmt, 1, SQL_C_SLONG, (SQLPOINTER)&sUserId, 0, NULL)
            != SQL_SUCCESS, StmtError);
        IDE_TEST_RAISE(        
            SQLBindCol(sStmt, 2, SQL_C_SBIGINT, (SQLPOINTER)&sObjId2Bind, 0, NULL)
            != SQL_SUCCESS, StmtError);
        IDE_TEST_RAISE(        
            SQLBindCol(sStmt, 3, SQL_C_SLONG, (SQLPOINTER)&sObjType, 0, NULL)
            != SQL_SUCCESS, StmtError);

        sRet = SQLFetch(sStmt);

        if ( sRet != SQL_NO_DATA )
        {
            IDE_TEST_RAISE( sRet != SQL_SUCCESS, notExist );

            /* BUG-46292 */
            sObjId = SQLBIGINT_TO_SLONG( sObjId2Bind );

            gObjectModeInfo[i].mObjUserId    = sUserId;
            gObjectModeInfo[i].mObjObjectId   = sObjId;
            gObjectModeInfo[i].mObjObjectType = sObjType;
        }
        else
        {
            gObjectModeInfo[i].mObjObjectType = UTM_NONE;
        }

        // BUG-33995 aexport have wrong free handle code
        FreeStmt( &sStmt );
    }
    return SQL_SUCCESS;

    IDE_EXCEPTION(alloc_error);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc);
    }
    IDE_EXCEPTION( notExist );    
    {
        fprintf(stderr,"The %s.%s object not exist\n",  gObjectModeInfo[i].mObjUserName,
                        gObjectModeInfo[i].mObjObjectName );
    }
    IDE_EXCEPTION(StmtError);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_STMT, (SQLHANDLE)sStmt);
    }
    
    IDE_EXCEPTION_END;

    if ( sStmt != SQL_NULL_HSTMT )
    {
        // BUG-33995 aexport have wrong free handle code
        FreeStmt( &sStmt );
    }

    return SQL_ERROR;
#undef IDE_FN
}

SQLRETURN getObjModeTableQuery( FILE   *aTblFp,
                                FILE   *aDbStatsFp,
                                SChar  *aUserName,
                                SChar  *aObjName )
{
#define IDE_FN "getObjModeTableQuery()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    SQLHSTMT    sStmt = SQL_NULL_HSTMT;
    SQLRETURN   sRet;
    SChar       sQuery[QUERY_LEN];
    SChar       sUserName[UTM_NAME_LEN+1];
    SChar       sTableName[UTM_NAME_LEN+1];
    SChar       s_table_type[STR_LEN];
    SQLLEN      s_user_ind;
    SQLLEN      s_table_ind;
    SQLLEN      sMaxRowInd;
#if (SIZEOF_LONG == 8) || defined(HAVE_LONG_LONG) || defined(VC_WIN32)
    SQLBIGINT   sMaxRow   = (SQLBIGINT)ID_LONG(0);
#else
    SQLBIGINT   sMaxRow   = {0, 0};
#endif
    SChar       sTbsName[41];
    SQLLEN      sTbsName_ind;
    SInt        sTbsType  = 0;
    SQLLEN      sTbsType_ind;
    SInt        sPctFree = 0;
    SQLLEN      sPctFree_ind;
    SInt        sPctUsed = 0;
    SQLLEN      sPctUsed_ind;
    SInt        sTableType2 = 0;
    SQLLEN      sTableType2_ind;
    SChar      *sPuserName = NULL;

    idlOS::fprintf(stdout, "\n##### TABLE #####\n");

    IDE_TEST_RAISE(SQLAllocStmt(m_hdbc, &sStmt) != SQL_SUCCESS,
            alloc_error);

    idlOS::sprintf( sQuery, GET_OBJMODE_TABLE_QUERY,
                    aUserName, aObjName );

    IDE_TEST_RAISE( SQLExecDirect( sStmt, (SQLCHAR *)sQuery, SQL_NTS )
                                   != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(
        SQLBindCol( sStmt, 2, SQL_C_CHAR, (SQLPOINTER)sUserName,
                    (SQLLEN)ID_SIZEOF(sUserName), &s_user_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 3, SQL_C_CHAR, (SQLPOINTER)sTableName,
                    (SQLLEN)ID_SIZEOF(sTableName), &s_table_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 4, SQL_C_CHAR, (SQLPOINTER)s_table_type,
                    (SQLLEN)ID_SIZEOF(s_table_type), NULL)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 6, SQL_C_SBIGINT, (SQLPOINTER)&sMaxRow, 
                    0, &sMaxRowInd)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 7, SQL_C_CHAR, (SQLPOINTER)sTbsName,
                    (SQLLEN)ID_SIZEOF(sTbsName), &sTbsName_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 8, SQL_C_SLONG, (SQLPOINTER)&sTbsType,
                    0, &sTbsType_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 9, SQL_C_SLONG, (SQLPOINTER)&sPctFree,
                    0, &sPctFree_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 10, SQL_C_SLONG, (SQLPOINTER)&sPctUsed, 
                    0, &sPctUsed_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 11, SQL_C_SLONG, (SQLPOINTER)&sTableType2,
                    0, &sTableType2_ind)
        != SQL_SUCCESS, sStmtError );

    sRet = SQLFetch(sStmt);

    if ( sRet != SQL_NO_DATA )
    {
        IDE_TEST_RAISE( sRet != SQL_SUCCESS, sStmtError );
    }

    sPuserName = sUserName;

    /* BUG-40174 Support export and import DBMS Stats */
    if ( gProgOption.mbCollectDbStats == ID_TRUE )
    {
        IDE_TEST( getTableStats( sUserName, 
                  sTableName, 
                  aDbStatsFp ) 
                  != SQL_SUCCESS );
    }

    IDE_TEST( getTableQuery( sUserName,
                             sTableName,
                             aTblFp,
                             aTblFp )    /* PROJ-2359 Table/Partition Access Option */
              != SQL_SUCCESS );

    idlOS::fprintf(aTblFp,"\n");

    IDE_TEST( getIndexQuery( sUserName, sPuserName, sTableName, aTblFp )
            != SQL_SUCCESS );

    IDE_TEST( resultForeignKeys( sUserName, sPuserName, sTableName, aTblFp )
            != SQL_SUCCESS);

    IDE_TEST( getComment( sUserName, sTableName, aTblFp )
            != SQL_SUCCESS );

    // BUG-33995 aexport have wrong free handle code
    FreeStmt( &sStmt );

    return SQL_SUCCESS;

    IDE_EXCEPTION(alloc_error);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc);
    }
    IDE_EXCEPTION(sStmtError);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_STMT, (SQLHANDLE)sStmt);
    }
    IDE_EXCEPTION_END;

    if ( sStmt != SQL_NULL_HSTMT )
    {
        // BUG-33995 aexport have wrong free handle code
        FreeStmt( &sStmt );
    }

    return SQL_ERROR;
#undef IDE_FN
}

/* BUG-49242 Queue object mode */
SQLRETURN getObjModeQueueQuery( FILE   *aQueueFp,
                                SChar  *aUserName,
                                SChar  *aObjName )
{
#define IDE_FN "getObjModeQueueeQuery()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    SQLHSTMT    sStmt = SQL_NULL_HSTMT;
    SQLRETURN   sRet;
    SChar       sQuery[QUERY_LEN];
    SChar       sUserName[UTM_NAME_LEN+1];
    SQLLEN      sUserName_ind;
    SChar       sTableName[UTM_NAME_LEN+1];
    SQLLEN      sTableName_ind;
    SChar       sTbsName[UTM_NAME_LEN+1];
    SQLLEN      sTbsName_ind;
    SInt        sTbsType  = 0;
    SQLLEN      sTbsType_ind;

    idlOS::fprintf( stdout, "\n##### QUEUE #####\n" );

    IDE_TEST_RAISE( SQLAllocStmt(m_hdbc, &sStmt) != SQL_SUCCESS,
            alloc_error );

    idlOS::sprintf( sQuery, GET_OBJMODE_QUEUE_QUERY,
                    aUserName, aObjName );

    IDE_TEST_RAISE( SQLExecDirect( sStmt, (SQLCHAR *)sQuery, SQL_NTS )
                                   != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(
        SQLBindCol( sStmt, 2, SQL_C_CHAR, (SQLPOINTER)sUserName,
                    (SQLLEN)ID_SIZEOF(sUserName), &sUserName_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 3, SQL_C_CHAR, (SQLPOINTER)sTableName,
                    (SQLLEN)ID_SIZEOF(sTableName), &sTableName_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 4, SQL_C_CHAR, (SQLPOINTER)sTbsName,
                    (SQLLEN)ID_SIZEOF(sTbsName), &sTbsName_ind)
        != SQL_SUCCESS, sStmtError );
    IDE_TEST_RAISE(    
        SQLBindCol( sStmt, 5, SQL_C_SLONG, (SQLPOINTER)&sTbsType,
                    0, &sTbsType_ind)
        != SQL_SUCCESS, sStmtError );

    sRet = SQLFetch(sStmt);

    if ( sRet != SQL_NO_DATA )
    {
        IDE_TEST_RAISE( sRet != SQL_SUCCESS, sStmtError );
    }

    IDE_TEST( getQueueQuery( sUserName,
                             sTableName,
                             aQueueFp )
              != SQL_SUCCESS );

    idlOS::fprintf(aQueueFp,"\n");

    FreeStmt( &sStmt );

    return SQL_SUCCESS;

    IDE_EXCEPTION( alloc_error );
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc );
    }
    IDE_EXCEPTION( sStmtError );
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_STMT, (SQLHANDLE)sStmt );
    }
    IDE_EXCEPTION_END;

    if ( sStmt != SQL_NULL_HSTMT )
    {
        FreeStmt( &sStmt );
    }

    return SQL_ERROR;
#undef IDE_FN
}

SQLRETURN getObjModeMViewQuery( FILE  * aMViewFp,
                                SChar * aUserName,
                                SChar * aObjectName )
{
    SQLHSTMT    sViewStmt = SQL_NULL_HSTMT;
    SQLRETURN   sRet;
    SInt        sPos = 0;
    SChar       sQuery[QUERY_LEN];
    SChar       sUserName[UTM_NAME_LEN+1];
    SChar       sMVIewName[UTM_NAME_LEN+1];
    SChar       sViewName[UTM_NAME_LEN+1];
    SChar       sViewParse[STR_LEN * 3];
    SInt        sViewId;
    SInt        sSeqNo;
    SInt        sStatus;
    SQLLEN      sUserInd;
    SQLLEN      sParseInd;
    idBool      sFirstFlag = ID_TRUE;
    SChar     * sPuserName = NULL;

    idlOS::fprintf( stdout, "\n##### MATERIALIZED VIEW #####\n" );

    IDE_TEST_RAISE( SQLAllocStmt( m_hdbc, &sViewStmt ) != SQL_SUCCESS,
                    allocError );

    sPos = idlOS::sprintf( sQuery, GET_OBJMODE_MVIEW_QUERY );

    sPos += idlOS::sprintf( sQuery + sPos,
                            "and c.USER_NAME='%s' "
                            "and b.TABLE_NAME='%s"UTM_MVIEW_VIEW_SUFFIX"' ",
                            aUserName, aObjectName );

    idlOS::sprintf( sQuery + sPos, "order by 1,5,3 " );

    IDE_TEST_RAISE( SQLExecDirect( sViewStmt, (SQLCHAR *)sQuery, SQL_NTS )
                    != SQL_SUCCESS, viewError );
    IDE_TEST_RAISE( SQLBindCol( sViewStmt, 1, SQL_C_CHAR, (SQLPOINTER)sUserName,
                                (SQLLEN)ID_SIZEOF(sUserName), &sUserInd )
                    != SQL_SUCCESS, viewError );
    IDE_TEST_RAISE( SQLBindCol( sViewStmt, 2, SQL_C_SLONG, (SQLPOINTER)&sViewId,
                                0, NULL )
                    != SQL_SUCCESS, viewError );
    IDE_TEST_RAISE( SQLBindCol( sViewStmt, 3, SQL_C_SLONG, (SQLPOINTER)&sSeqNo,
                                0, NULL )
                    != SQL_SUCCESS, viewError );
    IDE_TEST_RAISE( SQLBindCol( sViewStmt, 4, SQL_C_CHAR, (SQLPOINTER)sViewParse,
                                (SQLLEN)ID_SIZEOF(sViewParse), &sParseInd )
                    != SQL_SUCCESS, viewError );
    IDE_TEST_RAISE( SQLBindCol( sViewStmt, 5, SQL_C_CHAR, (SQLPOINTER)sViewName,
                                (SQLLEN)ID_SIZEOF(sViewName), NULL )
                    != SQL_SUCCESS, viewError );
    IDE_TEST_RAISE( SQLBindCol( sViewStmt, 6, SQL_C_SLONG, (SQLPOINTER)&sStatus,
                                0, NULL )
                    != SQL_SUCCESS, viewError );

    while ( (sRet = SQLFetch( sViewStmt )) != SQL_NO_DATA )
    {
        IDE_TEST_RAISE( sRet != SQL_SUCCESS, viewError );

        idlOS::fprintf( aMViewFp, "%s", sViewParse );
        idlOS::fflush( aMViewFp );

        sFirstFlag = ID_FALSE;
    }

    if ( sFirstFlag == ID_FALSE )
    {
        idlOS::fprintf( aMViewFp, ";\n\n" );
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-33995 aexport have wrong free handle code
    FreeStmt( &sViewStmt );

    sPuserName = sUserName;

    /* View Name에서 $VIEW 제거 */
    idlOS::memset( sMVIewName, 0x00, ID_SIZEOF( sMVIewName ) );
    idlOS::memcpy( sMVIewName, sViewName, idlOS::strlen( sViewName ) -
                                          UTM_MVIEW_VIEW_SUFFIX_SIZE );

    IDE_TEST( getObjPrivQuery( aMViewFp, UTM_MVIEW, sUserName, sMVIewName )
              != SQL_SUCCESS );

    IDE_TEST( getIndexQuery( sUserName, sPuserName, sMVIewName, aMViewFp )
              != SQL_SUCCESS );

    IDE_TEST( resultForeignKeys( sUserName, sPuserName, sMVIewName, aMViewFp )
              != SQL_SUCCESS );

    idlOS::fflush( aMViewFp );

    return SQL_SUCCESS;

    IDE_EXCEPTION( allocError );
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc );
    }
    IDE_EXCEPTION( viewError );
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_STMT, (SQLHANDLE)sViewStmt );
    }
    IDE_EXCEPTION_END;

    idlOS::fflush( aMViewFp );

    if ( sViewStmt != SQL_NULL_HSTMT )
    {
        // BUG-33995 aexport have wrong free handle code
        FreeStmt( &sViewStmt );
    }
    else
    {
        /* Nothing to do */
    }

    return SQL_ERROR;
}

SQLRETURN getObjModeViewQuery( FILE  *aViewFp,
                               SChar *aUserName,
                               SChar *aObjectName )
{
#define IDE_FN "getObjModeViewQuery()"
    SQLHSTMT  sViewStmt = SQL_NULL_HSTMT;
    SQLRETURN sRet;
    SInt      i    = 0;
    SInt      sPos = 0;
    SChar     sQuery[QUERY_LEN];
    SChar     sUserName[UTM_NAME_LEN+1];
    SChar     sViewName[UTM_NAME_LEN+1];
    SChar     sViewParse[STR_LEN*3];
    SInt      sViewId   = 0;
    SInt      sSeqNo    = 0;
    SInt      sStatus   = 0;
    SQLLEN    sUserInd  = 0;
    SQLLEN    sParseInd = 0;
    idBool    sFirstFlag = ID_TRUE;

    idlOS::fprintf(stdout, "\n##### VIEW #####\n");

    IDE_TEST_RAISE(SQLAllocStmt(m_hdbc, &sViewStmt) != SQL_SUCCESS,
                   allocError);

    for ( i = 0; i < 2; i++)
    {
        if ( i == 0 )
        {
            sPos = idlOS::sprintf(sQuery,
                                  GET_OBJMODE_VIEW_QUERY1);
        }
        else
        {
            sPos = idlOS::sprintf( sQuery,
                                   GET_OBJMODE_VIEW_QUERY2);
        }

        sPos += idlOS::sprintf( sQuery + sPos,                     
                                 "and c.USER_NAME='%s' "
                                 "and b.TABLE_NAME='%s' ",
                                 aUserName, aObjectName );

        idlOS::sprintf(sQuery + sPos, "order by 1,5,3 ");

        IDE_TEST_RAISE(SQLExecDirect(sViewStmt, (SQLCHAR *)sQuery, SQL_NTS)
                       != SQL_SUCCESS, viewError);
        IDE_TEST_RAISE(
            SQLBindCol( sViewStmt, 1, SQL_C_CHAR, (SQLPOINTER)sUserName,
                        (SQLLEN)ID_SIZEOF(sUserName), &sUserInd)
            != SQL_SUCCESS, viewError);
        IDE_TEST_RAISE(        
            SQLBindCol( sViewStmt, 2, SQL_C_SLONG, (SQLPOINTER)&sViewId,
                        0, NULL)
            != SQL_SUCCESS, viewError);
        IDE_TEST_RAISE(        
            SQLBindCol( sViewStmt, 3, SQL_C_SLONG, (SQLPOINTER)&sSeqNo,
                        0, NULL)
            != SQL_SUCCESS, viewError);
        IDE_TEST_RAISE(        
            SQLBindCol( sViewStmt, 4, SQL_C_CHAR, (SQLPOINTER)sViewParse,
                        (SQLLEN)ID_SIZEOF(sViewParse), &sParseInd)
            != SQL_SUCCESS, viewError);
        IDE_TEST_RAISE(        
            SQLBindCol( sViewStmt, 5, SQL_C_CHAR, (SQLPOINTER)sViewName,
                        (SQLLEN)ID_SIZEOF(sViewName), NULL)
            != SQL_SUCCESS, viewError);
        IDE_TEST_RAISE(        
            SQLBindCol( sViewStmt, 6, SQL_C_SLONG, (SQLPOINTER)&sStatus,
                        0, NULL)
            != SQL_SUCCESS, viewError);        

        while ((sRet = SQLFetch(sViewStmt)) != SQL_NO_DATA)
        {
            IDE_TEST_RAISE( sRet != SQL_SUCCESS, viewError );

            if ( sSeqNo == 1 )
            {
                if ( gProgOption.mbExistViewForce == ID_TRUE )
                {
                    SChar *sTmp;
                    SChar *sOrg = sViewParse;
                    sTmp = idlOS::strtok( sOrg, " \t\n" );
                 
                    while ( sTmp != NULL )
                    {
                        if ( idlOS::strcasecmp( sTmp, "FORCE" ) == 0 )
                        {
                            idlOS::fprintf( aViewFp, "%s ", sTmp );
                            break;
                        }
                        else if ( idlOS::strcasecmp( sTmp, "VIEW" ) == 0 )
                        {
                            idlOS::fprintf( aViewFp, "force %s ", sTmp );
                            break;
                        }
                        else
                        {
                            idlOS::fprintf( aViewFp, "%s ", sTmp );
                        }
                      
                        sTmp = idlOS::strtok( NULL, " \t\n" );
                    }
                      
                    // BUG-29971 codesonar null pointer dereference
                    IDE_ASSERT(sTmp != NULL);

                    idlOS::fprintf( aViewFp, sTmp + idlOS::strlen(sTmp) + 1 );
                    idlOS::fflush( aViewFp );
                }
                else
                {
                    idlOS::fprintf( aViewFp, "%s", sViewParse );
                    idlOS::fflush( aViewFp );
                }
            }
            else
            {
                idlOS::fprintf( aViewFp, "%s", sViewParse);
                idlOS::fflush( aViewFp );
            }
            sFirstFlag = ID_FALSE;
        } // end while

        SQLFreeStmt( sViewStmt, SQL_CLOSE );
        SQLFreeStmt( sViewStmt, SQL_UNBIND );
    } // end for

    // BUG-33995 aexport have wrong free handle code
    FreeStmt( &sViewStmt );

    if ( sFirstFlag == ID_FALSE )
    {
        idlOS::fprintf( aViewFp, ";\n\n");
    }

    IDE_TEST( getObjPrivQuery( aViewFp, UTM_VIEW,
                               sUserName, sViewName )
                               != SQL_SUCCESS );

    idlOS::fflush( aViewFp );
    
    return SQL_SUCCESS;

    IDE_EXCEPTION(allocError);
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc );
    }
    IDE_EXCEPTION( viewError );
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_STMT, (SQLHANDLE)sViewStmt );
    }
    IDE_EXCEPTION_END;

    idlOS::fflush( aViewFp );

    if ( sViewStmt != SQL_NULL_HSTMT )
    {
        // BUG-33995 aexport have wrong free handle code
        FreeStmt( &sViewStmt );
    }

    return SQL_ERROR;
#undef IDE_FN
}

SQLRETURN getObjModeProcQuery( FILE  *aProcFp,
                               SChar *aUserName,
                               SChar *aObjectName )
{
#define IDE_FN "getObjModeProcQuery()"
    SQLHSTMT   sStmt = SQL_NULL_HSTMT;
    SQLRETURN  sRet;
    SInt       i     = 0;
    SInt       sPos  = 0;
    SChar      sQuery[QUERY_LEN];
    SChar      sUserName[UTM_NAME_LEN+1];
    SChar      sProcName[UTM_NAME_LEN+1];
    SChar      sProcParse[STR_LEN*3];
    SQLBIGINT  sProcId;
    SInt       sSeqNo;
    SInt       sObjType;
    SQLLEN     sUserInd;
    SQLLEN     sParseInd;
    idBool     sFirstFlag = ID_TRUE;

    idlOS::fprintf(stdout, "\n##### STORED PROCEDURE #####\n");

    IDE_TEST_RAISE(SQLAllocStmt(m_hdbc, &sStmt) != SQL_SUCCESS,
                   allocError);

    for ( i = 0; i < 2; i++ )
    {
        if ( i == 0 )
        {
            sPos = idlOS::sprintf( sQuery, GET_OBJMODE_PROC_QUERY1 );
        }
        else
        {
            sPos = idlOS::sprintf( sQuery, GET_OBJMODE_PROC_QUERY2);
        }

        sPos += idlOS::sprintf( sQuery + sPos,
                                 " and c.USER_NAME='%s'"
                                 " and b.PROC_NAME='%s'",
                                  aUserName, aObjectName );

        idlOS::sprintf( sQuery + sPos, " order by 1,5,3" );

        IDE_TEST_RAISE( SQLExecDirect( sStmt, (SQLCHAR *)sQuery, SQL_NTS )
                                       != SQL_SUCCESS, procError);
        IDE_TEST_RAISE(
            SQLBindCol( sStmt, 1, SQL_C_CHAR, (SQLPOINTER)sUserName,
                        (SQLLEN)ID_SIZEOF(sUserName), &sUserInd)
            != SQL_SUCCESS, procError);
        IDE_TEST_RAISE(        
            SQLBindCol( sStmt, 2, SQL_C_SBIGINT, (SQLPOINTER)&sProcId,
                        0, NULL)
            != SQL_SUCCESS, procError);
        IDE_TEST_RAISE(        
            SQLBindCol( sStmt, 3, SQL_C_SLONG, (SQLPOINTER)&sSeqNo,
                        0, NULL)
            != SQL_SUCCESS, procError);
        IDE_TEST_RAISE(        
            SQLBindCol( sStmt, 4, SQL_C_CHAR, (SQLPOINTER)sProcParse,
                        (SQLLEN)ID_SIZEOF(sProcParse), &sParseInd)
            != SQL_SUCCESS, procError);
        IDE_TEST_RAISE(        
            SQLBindCol( sStmt, 5, SQL_C_CHAR, (SQLPOINTER)sProcName,
                        (SQLLEN)ID_SIZEOF(sProcName), NULL)
            != SQL_SUCCESS, procError);
        IDE_TEST_RAISE(        
            SQLBindCol( sStmt, 6, SQL_C_SLONG, (SQLPOINTER)&sObjType,
                        0, NULL)
            != SQL_SUCCESS, procError);        

        while ( (sRet = SQLFetch(sStmt)) != SQL_NO_DATA )
        {
            IDE_TEST_RAISE( sRet != SQL_SUCCESS, procError );

            if ( sSeqNo == 1 )
            {
                if ( gProgOption.mbExistDrop == ID_TRUE )
                {
                    if ( sObjType == 0 )
                    {
                        idlOS::fprintf( aProcFp, "drop procedure \"%s\".\"%s\";\n", 
                                        sUserName, sProcName);
                    }
                    else if ( sObjType == 1 )
                    {
                        idlOS::fprintf( aProcFp, "drop function \"%s\".\"%s\";\n", 
                                        sUserName, sProcName);
                    }
                }
            }

            sFirstFlag = ID_FALSE;

            idlOS::fprintf( aProcFp, "%s", sProcParse );
        
       } // end while

        SQLFreeStmt( sStmt, SQL_CLOSE );
        SQLFreeStmt( sStmt, SQL_UNBIND );
    } // end for

    // BUG-33995 aexport have wrong free handle code
    FreeStmt( &sStmt );

    if ( sFirstFlag == ID_FALSE )
    {
        idlOS::fprintf( aProcFp, ";\n/\n\n" );
    }

    IDE_TEST( getObjPrivQuery( aProcFp, UTM_PROCEDURE,
                               sUserName, sProcName )
                               != SQL_SUCCESS );

    idlOS::fflush( aProcFp);
 
    return SQL_SUCCESS;

    IDE_EXCEPTION(allocError);
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc );
    }
    IDE_EXCEPTION( procError );
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_STMT, (SQLHANDLE)sStmt );
    }
    IDE_EXCEPTION_END;

    idlOS::fflush( aProcFp );

    if ( sStmt != SQL_NULL_HSTMT )
    {
        // BUG-33995 aexport have wrong free handle code
        FreeStmt( &sStmt );
    }

    return SQL_ERROR;
#undef IDE_FN
}

/* BUG-36367 aexport must consider new objects, 'package' and 'library' */
SQLRETURN getObjModePkgQuery( FILE  *aPkgFp,
                              SChar *aUserName,
                              SChar *aObjectName)
{
#define IDE_FN "getObjModePkgQuery()"
    SQLHSTMT   sStmt = SQL_NULL_HSTMT;
    SQLRETURN  sRet;
    SInt       sPos  = 0;
    SChar      sQuery[QUERY_LEN];
    SChar      sUserName[UTM_NAME_LEN+1];
    SChar      sPkgName[UTM_NAME_LEN+1];
    SChar      sPkgParse[STR_LEN*3];
    SQLBIGINT  sPkgId;
    SInt       sSeqNo;
    SInt       sObjType;
    SQLLEN     sUserInd;
    SQLLEN     sParseInd;
    idBool     sHasWritten = ID_FALSE;

    idlOS::fprintf( stdout, "\n##### STORED PACKAGE #####\n");

    IDE_TEST_RAISE( SQLAllocStmt( m_hdbc, &sStmt) != SQL_SUCCESS, allocError);

    sPos = idlOS::sprintf( sQuery, GET_OBJMODE_PKG_QUERY);

    sPos += idlOS::sprintf( sQuery + sPos,
                            " AND users.user_name = '%s'"
                            " AND pkg.package_name = '%s'",
                            aUserName,
                            aObjectName );

    idlOS::sprintf( sQuery + sPos, " ORDER BY 1,5,6,3" );

    IDE_TEST_RAISE( SQLExecDirect( sStmt, (SQLCHAR *)sQuery, SQL_NTS) != SQL_SUCCESS,
                    pkgError);

    IDE_TEST_RAISE(
            SQLBindCol( sStmt,
                        1,
                        SQL_C_CHAR,
                        (SQLPOINTER)sUserName,
                        (SQLLEN)ID_SIZEOF(sUserName),
                        &sUserInd)
            != SQL_SUCCESS, pkgError);

    IDE_TEST_RAISE(        
            SQLBindCol( sStmt,
                        2,
                        SQL_C_SBIGINT,
                        (SQLPOINTER)&sPkgId,
                        0,
                        NULL)
            != SQL_SUCCESS, pkgError);

    IDE_TEST_RAISE(        
            SQLBindCol( sStmt,
                        3,
                        SQL_C_SLONG,
                        (SQLPOINTER)&sSeqNo,
                        0,
                        NULL)
            != SQL_SUCCESS, pkgError);

    IDE_TEST_RAISE(        
            SQLBindCol( sStmt,
                        4,
                        SQL_C_CHAR,
                        (SQLPOINTER)sPkgParse,
                        (SQLLEN)ID_SIZEOF(sPkgParse), 
                        &sParseInd)
            != SQL_SUCCESS, pkgError);

    IDE_TEST_RAISE(        
            SQLBindCol( sStmt,
                        5,
                        SQL_C_CHAR,
                        (SQLPOINTER)sPkgName,
                        (SQLLEN)ID_SIZEOF(sPkgName), 
                        NULL)
            != SQL_SUCCESS, pkgError);

    IDE_TEST_RAISE(        
            SQLBindCol( sStmt,
                        6,
                        SQL_C_SLONG,
                        (SQLPOINTER)&sObjType,
                        0,
                        NULL)
            != SQL_SUCCESS, pkgError);        


    while ( (sRet = SQLFetch( sStmt)) != SQL_NO_DATA)
    {
        IDE_TEST_RAISE( sRet != SQL_SUCCESS, pkgError);

        if ( gProgOption.mbExistDrop == ID_TRUE )
        {
            // To write DROP DDL only once
            if ( sSeqNo == 1 && sObjType == UTM_PACKAGE_SPEC)
            {
                idlOS::fprintf( aPkgFp, "drop package \"%s\".\"%s\";\n",
                        sUserName, sPkgName);
            }
        }

        // To add psm terminator(;/) at the end of package spec.
        if ( sSeqNo == 1 && sObjType == UTM_PACKAGE_BODY )
        {
            idlOS::fprintf( aPkgFp, ";\n/\n\n");
        }

        idlOS::fprintf( aPkgFp, "%s", sPkgParse);
        sHasWritten = ID_TRUE;
    }

    if ( sHasWritten == ID_TRUE )
    {
        idlOS::fprintf( aPkgFp, ";\n/\n\n" );
    }

    IDE_TEST( getObjPrivQuery( aPkgFp,
                               UTM_PACKAGE_SPEC,
                               sUserName,
                               sPkgName) != SQL_SUCCESS);

    FreeStmt( &sStmt );

    idlOS::fflush( aPkgFp);
 
    return SQL_SUCCESS;

    IDE_EXCEPTION( allocError);
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc);
    }
    IDE_EXCEPTION( pkgError);
    {
        utmSetErrorMsgWithHandle( SQL_HANDLE_STMT, (SQLHANDLE)sStmt);
    }
    IDE_EXCEPTION_END;

    idlOS::fflush( aPkgFp);

    if ( sStmt != SQL_NULL_HSTMT)
    {
        FreeStmt( &sStmt);
    }

    return SQL_ERROR;
#undef IDE_FN
}
