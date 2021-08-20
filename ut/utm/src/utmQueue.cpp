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
 * $Id$
 **********************************************************************/

#include <smDef.h>
#include <utm.h>
#include <utmExtern.h>
#include <utmTable.h>
#include <utmScript4Ilo.h>

extern const SChar *gOptionUser[2];
extern const SChar *gOptionPasswd[2];
extern const SChar *gOptionTable[2];
extern const SChar *gOptionFmt[2];
extern const SChar *gOptionDat[2];
extern const SChar *gOptionLog[2];
extern const SChar *gOptionBad[2];

SQLRETURN getQueueInfo( SChar *a_user,
                        FILE  *aTblFp,
                        FILE  *aIlOutFp,
                        FILE  *aIlInFp )
{
#define IDE_FN "getQueueInfo()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    SQLHSTMT  s_tblStmt = SQL_NULL_HSTMT;
    SQLRETURN sRet;

    SInt      i = 0;
    SChar     sQuotedUserName[UTM_NAME_LEN+3];
    SChar     s_user_name[UTM_NAME_LEN+1];
    SChar     s_puser_name[UTM_NAME_LEN+1];
    SChar     s_table_name[UTM_NAME_LEN+1];
    SChar     s_table_type[STR_LEN];
    SChar     s_passwd[STR_LEN];
    SQLLEN    s_user_ind;
    SQLLEN    s_table_ind;

    idBool    sNeedQuote4User  = ID_FALSE;
    idBool    sNeedQuote4Pwd   = ID_FALSE;
    idBool    sNeedQuote4Tbl   = ID_FALSE;
    SInt      sNeedQuote4File  = 0;

    SChar     sTBSName[UTM_NAME_LEN+1];
    SQLLEN    sTBSName_ind;
    SInt      sTbsType  = 0;
    SQLLEN    sTbsType_ind;

    s_puser_name[0] = '\0';
    idlOS::fprintf(stdout, "\n##### QUEUE #####\n");

    IDE_TEST_RAISE(SQLAllocStmt(m_hdbc, &s_tblStmt) != SQL_SUCCESS,
                   alloc_error);

    if (idlOS::strcasecmp(a_user, "SYS") == 0)
    {
        IDE_TEST_RAISE(SQLTables(s_tblStmt,
                              NULL, 0,
                              NULL, 0,
                              NULL, 0,
                              (SQLCHAR *)"QUEUE", SQL_NTS) != SQL_SUCCESS,
                       tbl_error);
    }
    else
    {
        /* BUG-36593 소문자,공백이 포함된 이름(quoted identifier) 처리 */
        utString::makeQuotedName(sQuotedUserName, a_user, idlOS::strlen(a_user));

        IDE_TEST_RAISE(SQLTables(s_tblStmt,
                              NULL, 0,
                              (SQLCHAR *)sQuotedUserName, SQL_NTS,
                              NULL, 0,
                              (SQLCHAR *)"QUEUE", SQL_NTS)
                       != SQL_SUCCESS, tbl_error);
    }

    IDE_TEST_RAISE(
        SQLBindCol(s_tblStmt, 2, SQL_C_CHAR,
                   (SQLPOINTER)s_user_name, (SQLLEN)ID_SIZEOF(s_user_name),
                   &s_user_ind)
        != SQL_SUCCESS, tbl_error);        
    IDE_TEST_RAISE(
        SQLBindCol(s_tblStmt, 3, SQL_C_CHAR,
                   (SQLPOINTER)s_table_name, (SQLLEN)ID_SIZEOF(s_table_name),
                   &s_table_ind)
        != SQL_SUCCESS, tbl_error);    
    IDE_TEST_RAISE(
        SQLBindCol(s_tblStmt, 4, SQL_C_CHAR,
                   (SQLPOINTER)s_table_type, (SQLLEN)ID_SIZEOF(s_table_type),
                   NULL)
        != SQL_SUCCESS, tbl_error);    

    IDE_TEST_RAISE(
        SQLBindCol(s_tblStmt, 7, SQL_C_CHAR, (SQLPOINTER)sTBSName,
                   (SQLLEN)ID_SIZEOF(sTBSName), &sTBSName_ind)
        != SQL_SUCCESS, tbl_error);
    
    IDE_TEST_RAISE(
        SQLBindCol(s_tblStmt, 8, SQL_C_SLONG, (SQLPOINTER)&sTbsType, 0,
                   &sTbsType_ind)
        != SQL_SUCCESS, tbl_error);

    while ((sRet = SQLFetch(s_tblStmt)) != SQL_NO_DATA)
    {
        IDE_TEST_RAISE( sRet != SQL_SUCCESS, tbl_error );

        if ( idlOS::strncmp(s_user_name, "SYSTEM_", 7) == 0 ||
             idlOS::strcmp(s_table_type, "QUEUE") != 0 )
        {
            continue;
        }

        idlOS::fprintf(stdout, "** \"%s\".\"%s\"\n", s_user_name, s_table_name);

        if (i == 0 || idlOS::strcmp(s_user_name, s_puser_name) != 0)
        {

            IDE_TEST(getPasswd(s_user_name, s_passwd) != SQL_SUCCESS);

            idlOS::fprintf(aTblFp, "connect \"%s\" / \"%s\"\n",s_user_name, s_passwd);
        }
        
        i++;

        /* BUG-34502: handling quoted identifiers */
        sNeedQuote4User = utString::needQuotationMarksForObject(s_user_name);
        sNeedQuote4Pwd  = utString::needQuotationMarksForFile(s_passwd);
        sNeedQuote4Tbl  = utString::needQuotationMarksForObject(s_table_name, ID_TRUE);
        sNeedQuote4File = utString::needQuotationMarksForFile(s_user_name) ||
                                  utString::needQuotationMarksForFile(s_table_name);

        /* formout in run_il_out.sh */
        printFormOutScript(aIlOutFp,
                           sNeedQuote4User,
                           sNeedQuote4Pwd,
                           sNeedQuote4Tbl,
                           sNeedQuote4File,
                           s_user_name,
                           s_passwd,
                           s_table_name,
                           ID_FALSE);

        /* data out in run_il_out.sh*/
        printOutScript(aIlOutFp,
                       sNeedQuote4User,
                       sNeedQuote4Pwd,
                       sNeedQuote4File,
                       s_user_name,
                       s_passwd,
                       s_table_name,
                       NULL );
        
        /* data in in run_il_in.sh*/
        printInScript(aIlInFp,
                      sNeedQuote4User,
                      sNeedQuote4Pwd,
                      sNeedQuote4File,
                      s_user_name,
                      s_passwd,
                      s_table_name,
                      NULL );

        IDE_TEST( getQueueQuery( s_user_name, s_table_name, aTblFp )
                                 != SQL_SUCCESS);

        idlOS::fprintf( stdout, "\n" );

        idlOS::snprintf( s_puser_name, ID_SIZEOF(s_puser_name), "%s",
                         s_user_name );
    } // end while

    // BUG-33995 aexport have wrong free handle code
    FreeStmt( &s_tblStmt );

    return SQL_SUCCESS;

    IDE_EXCEPTION(alloc_error);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc);
    }
    IDE_EXCEPTION(tbl_error);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_STMT, (SQLHANDLE)s_tblStmt);
    }
    IDE_EXCEPTION_END;

    if ( s_tblStmt != SQL_NULL_HSTMT )
    {
        // BUG-33995 aexport have wrong free handle code
        FreeStmt( &s_tblStmt );
    }

    return SQL_ERROR;
#undef IDE_FN
}

SQLRETURN getQueueQuery( SChar *a_user,
                         SChar *a_table,
                         FILE  *a_crt_fp)
{
#define IDE_FN "getQueueQuery()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idlOS::fprintf(a_crt_fp, "\n--############################\n");
    idlOS::fprintf(a_crt_fp, "--  \"%s\".\"%s\"  \n", a_user, a_table);
    idlOS::fprintf(a_crt_fp, "--############################\n");
    if ( gProgOption.mbExistDrop == ID_TRUE )
    {
        // BUG-20943 drop 구문에서 user 가 명시되지 않아 drop 이 실패합니다.
        idlOS::fprintf(a_crt_fp, "drop queue \"%s\".\"%s\";\n", a_user, a_table);
    }

    /* BUG-47159 Using DBMS_METADATA package in aexport */
    IDE_TEST(gMeta->getDdl(DDL, UTM_OBJ_TYPE_QUEUE, a_table, a_user)
             != IDE_SUCCESS);

#ifdef DEBUG
    idlOS::fprintf( stderr, "%s\n", gMeta->getDdlStr() );
#endif
    idlOS::fprintf( a_crt_fp, "%s\n", gMeta->getDdlStr() );

    idlOS::fflush(a_crt_fp);

    return SQL_SUCCESS;

    IDE_EXCEPTION_END;

    return SQL_ERROR;
#undef IDE_FN
}
