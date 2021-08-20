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
 * $Id: aexport.cpp 91504 2021-08-19 23:38:48Z chkim $
 **********************************************************************/

#include <idl.h>
#include <iduVersion.h>
#include <utm.h>
#include <utmExtern.h>
#include <utmDbStats.h>
/* priv : ALL:1 , CREATE TABLE :217 , CREATE ANY TABLE :218 */
#define GET_CREATE_TABLE_PRIV_COUNT_QUERY                               \
    "select COUNT(*) "                                                  \
    "from system_.sys_privileges_ a,system_.sys_grant_system_ b,"       \
    "system_.sys_users_ c "                                             \
    "where a.priv_id=b.priv_id and "                                    \
    "b.grantee_id=c.user_id and a.priv_type=2 "                         \
    "and b.priv_id in (1, 217, 218) "                                   \
    "and c.user_name=? "

/* BUG-34502: handling quoted identifiers */
extern const SChar *gUserOptionFormatStr[];
extern const SChar *gPwdOptionFormatStr[];
const SChar *gOptionSqlFile[] = {
    " -f %s_%s.sql",
    " -f "UTM_FILE_QUOT"%s_%s.sql"UTM_FILE_QUOT
};

SInt           m_user_cnt;
UserInfo      *m_UserInfo = NULL;
utmProgOption  gProgOption;
uteErrorMgr    gErrorMgr;
FileStream     gFileStream;

/* BUG-47159 Using DBMS_METADATA package in aexport */
utmDbmsMeta       *gMeta;

extern ObjectModeInfo *gObjectModeInfo;

static void show_copyright();
static void show_runmsg();
static void exec_script();
static IDE_RC createFile( SChar *aUserName );
static IDE_RC destroyFile( SChar *aUserName );
static IDE_RC doImport();
static IDE_RC doExport();
static IDE_RC doObjModeExport();
static IDE_RC doMakeRunScript();
static IDE_RC hasEnoughPriv4Analysis( SChar *UserName );

int main(int argc, char** argv)
{
    gMeta = new utmDbmsMeta();

    IDE_TEST(gProgOption.ParsingCommandLine(argc, argv) != IDE_SUCCESS);

    show_copyright();

    /* BUG-47652 Set file permission */
    // BUG-26287: 옵션 처리방법 통일
    IDE_TEST_RAISE( gProgOption.ReadEnvironment() != IDE_SUCCESS, env_var_error );

    // BUG-26287: 있으면 altibase.properties도 참조 (for server)
    gProgOption.ReadServerProperties();

    IDE_TEST(gProgOption.getProperties() != IDE_SUCCESS);
   
    if ( idlOS::strcasecmp( gProgOption.mOper, "OUT" ) == 0 )
    {
        gProgOption.ReadProgOptionInteractive();

        // BUG-40271 Replace the default character set from predefined value (US7ASCII) to DB character set.
        IDE_TEST_RAISE(gProgOption.setNls() != SQL_SUCCESS,
                       nls_error);

        IDE_TEST_RAISE(init_handle() != SQL_SUCCESS,
                       init_error);

        /* BUG-47159 Using DBMS_METADATA package in aexport */
        IDE_TEST_RAISE(gMeta->initialize(m_hdbc), init_error);

        if ( gProgOption.m_bExist_OBJECT == ID_TRUE )
        {
            doObjModeExport();
        }
        else
        {
            doExport();
        }

        IDE_TEST_RAISE( fini_handle() != SQL_SUCCESS,
                        fini_error );
    }
    else if ( idlOS::strcasecmp( gProgOption.mOper, "IN" ) == 0 )
    {
        doImport();
    }

    exit(0);

    IDE_EXCEPTION(init_error);
    {
        printError();
    }
    IDE_EXCEPTION(nls_error);
    {
        printError();
    }
    IDE_EXCEPTION(fini_error);
    {
        printError();
    }
    /* BUG-47652 Set file permission */
    IDE_EXCEPTION(env_var_error);
    {
        printError();
    }
    IDE_EXCEPTION_END;

    exit(1);
}

static IDE_RC createFile( SChar *aUserName)
{
    SChar sFileName[UTM_NAME_LEN+50]; /* BUG-39622: file_name="[user_name]_CRT_XXXX.sql" */
    SChar *sUserName = NULL;

    if ( gProgOption.mbExistTwoPhaseScript == ID_FALSE )
    {
        if (idlOS::strcasecmp(aUserName, (SChar*)UTM_STR_SYS) == 0)
        {
            sUserName = (SChar *) UTM_STR_ALL;

            IDE_TEST_RAISE( open_file((SChar *)"ALL_CRT_TBS.sql", &(gFileStream.mTbsFp)) 
                                       != SQL_SUCCESS, openError);

            IDE_TEST_RAISE( open_file((SChar *)"ALL_CRT_REP.sql", &(gFileStream.mReplFp)) 
                                       != SQL_SUCCESS, openError);

            /* PROJ-1438 Job Scheduler */
            IDE_TEST_RAISE( open_file((SChar *)"ALL_CRT_JOB.sql", &(gFileStream.mJobFp))
                                       != SQL_SUCCESS, openError);
        }
        else
        {
            sUserName = aUserName;

            /* BUG-40469 output tablespaces info. in user mode */
            if ( gProgOption.mbCrtTbs4UserMode == ID_TRUE )
            {
                idlOS::sprintf( sFileName,"%s_CRT_TBS.sql", sUserName );
                IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mTbsFp)) 
                                != SQL_SUCCESS, openError );
            }
        }

        idlOS::sprintf( sFileName,"%s_CRT_USER.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mUserFp)) 
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_SYN.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mSynFp))
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_DIR.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mDirFp))
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_TBL.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mTblFp)) 
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_SEQ.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mSeqFp)) 
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_VIEW_PROC.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mViewProcFp)) 
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_TRIG.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mTrigFp)) 
                                   != SQL_SUCCESS, openError);

        /* BUG-36367 aexport must consider new objects, 'package' and 'library' */
        idlOS::sprintf( sFileName,"%s_CRT_LIB.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mLibFp)) 
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_INDEX.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mIndexFp))
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_FK.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mFkFp)) 
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName,"%s_CRT_LINK.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mLinkFp)) 
                                   != SQL_SUCCESS, openError);

        idlOS::sprintf( sFileName, "%s_REFRESH_MVIEW.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mRefreshMViewFp) )
                                   != SQL_SUCCESS, openError );

        idlOS::sprintf( sFileName, "%s_ALT_TBL.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mAltTblFp) )
                                   != SQL_SUCCESS, openError );

        idlOS::sprintf( sFileName, "%s_EXE_STATS.sql", sUserName );
        IDE_TEST_RAISE( open_file( sFileName, &(gFileStream.mDbStatsFp) )
                        != SQL_SUCCESS, openError );

    }
    else
    {
        if (idlOS::strcasecmp(aUserName, (SChar*)UTM_STR_SYS) == 0)
        {
           IDE_TEST_RAISE(open_file((SChar*)"ALL_OBJECT.sql", &(gFileStream.mObjFp)) 
                                    != SQL_SUCCESS, openError);

           IDE_TEST_RAISE(open_file((SChar*)"ALL_OBJECT_CONSTRAINTS.sql", &(gFileStream.mObjConFp)) 
                                    != SQL_SUCCESS, openError);
        }
        else
        {
            idlOS::sprintf( sFileName,"%s_OBJECT.sql", aUserName );
            IDE_TEST_RAISE(open_file( sFileName, &gFileStream.mObjFp )
                                      != SQL_SUCCESS, openError);

            idlOS::sprintf( sFileName,"%s_OBJECT_CONSTRAINTS.sql", aUserName );
            IDE_TEST_RAISE(open_file( sFileName, &gFileStream.mObjConFp )
                                      != SQL_SUCCESS, openError);
        }
        
        gFileStream.mTbsFp          = gFileStream.mObjFp;
        gFileStream.mUserFp         = gFileStream.mObjFp;
        gFileStream.mSynFp          = gFileStream.mObjFp;
        gFileStream.mDirFp          = gFileStream.mObjFp;
        gFileStream.mTblFp          = gFileStream.mObjFp;
        gFileStream.mSeqFp          = gFileStream.mObjFp;
        gFileStream.mViewProcFp     = gFileStream.mObjFp;
        gFileStream.mLibFp          = gFileStream.mObjFp;
        gFileStream.mLinkFp         = gFileStream.mObjFp;
        gFileStream.mDbStatsFp      = gFileStream.mObjFp;
        gFileStream.mTrigFp         = gFileStream.mObjConFp;
        gFileStream.mIndexFp        = gFileStream.mObjConFp;
        gFileStream.mFkFp           = gFileStream.mObjConFp;
        gFileStream.mReplFp         = gFileStream.mObjConFp;
        gFileStream.mRefreshMViewFp = gFileStream.mObjConFp;
        gFileStream.mJobFp          = gFileStream.mObjConFp;
        gFileStream.mAltTblFp       = gFileStream.mObjConFp;
    }
    
    if ( gProgOption.mbExistInvalidScript == ID_TRUE )
    {
        IDE_TEST_RAISE( open_file((SChar*)"INVALID.sql", &(gFileStream.mInvalidFp)) 
                                  != SQL_SUCCESS, openError );
    }
    
    /* import 를 위한 실행 스크립트 생성 run_is_*.sh */
    IDE_TEST_RAISE( open_file( gProgOption.mScriptIsql, &gFileStream.mIsFp )
                               != SQL_SUCCESS, openError);

    if ( gProgOption.mbExistTwoPhaseScript == ID_FALSE )
    {
        if ( gProgOption.mbExistScriptIsqlIndex == ID_TRUE )
        {
            IDE_TEST_RAISE( open_file( gProgOption.mScriptIsqlIndex, &gFileStream.mIsIndexFp )
                    != SQL_SUCCESS, openError);
        }

        if ( gProgOption.mbExistScriptIsqlFK == ID_TRUE )
        {
            IDE_TEST_RAISE( open_file( gProgOption.mScriptIsqlFK, &gFileStream.mIsFkFp ) 
                    != SQL_SUCCESS, openError);
        }

        if ( gProgOption.mbExistScriptRepl == ID_TRUE )
        {
            IDE_TEST_RAISE( open_file( gProgOption.mScriptIsqlRepl, &gFileStream.mIsReplFp )
                    != SQL_SUCCESS, openError);
        }

        if ( gProgOption.mbExistScriptRefreshMView == ID_TRUE )
        {
            IDE_TEST_RAISE( open_file( gProgOption.mScriptIsqlRefreshMView,
                                       &gFileStream.mIsRefreshMViewFp )
                            != SQL_SUCCESS, openError );
        }

        if ( gProgOption.mbExistScriptJob == ID_TRUE )
        {
            IDE_TEST_RAISE( open_file( gProgOption.mScriptIsqlJob, &gFileStream.mIsJobFp )
                    != SQL_SUCCESS, openError);
        }
        else
        {
            /* Nothing to do */
        }

        if ( gProgOption.mbExistScriptAlterTable == ID_TRUE )
        {
            IDE_TEST_RAISE( open_file( gProgOption.mScriptIsqlAlterTable,
                                       &gFileStream.mIsAltTblFp )
                            != SQL_SUCCESS, openError );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        IDE_TEST_RAISE( open_file( gProgOption.mScriptIsqlCon, &gFileStream.mIsConFp ) 
                != SQL_SUCCESS, openError);

        gFileStream.mIsIndexFp        = gFileStream.mIsConFp;
        gFileStream.mIsFkFp           = gFileStream.mIsConFp;
        gFileStream.mIsReplFp         = gFileStream.mIsConFp;
        gFileStream.mIsRefreshMViewFp = gFileStream.mIsConFp;
        gFileStream.mIsJobFp          = gFileStream.mIsConFp;
        gFileStream.mIsAltTblFp       = gFileStream.mIsConFp;
    }

    IDE_TEST_RAISE( open_file( gProgOption.mScriptIloOut, &gFileStream.mIlOutFp)
                               != SQL_SUCCESS, openError);

    IDE_TEST_RAISE( open_file( gProgOption.mScriptIloIn, &gFileStream.mIlInFp) 
                               != SQL_SUCCESS, openError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(openError);
    {
        printError();
    }
    IDE_EXCEPTION_END;

    destroyFile( aUserName );

    return IDE_FAILURE;
}

static IDE_RC destroyFile( SChar *aUserName )
{
    if ( gProgOption.mbExistTwoPhaseScript == ID_FALSE )
    {
        if (idlOS::strcasecmp(aUserName, (SChar*)UTM_STR_SYS) == 0)
        {
            IDE_TEST_RAISE( close_file(gFileStream.mTbsFp) 
                                    != SQL_SUCCESS, closeError );

            IDE_TEST_RAISE( close_file(gFileStream.mReplFp) 
                                    != SQL_SUCCESS, closeError );

            /* PROJ-1438 Job Scheduler */
            IDE_TEST_RAISE( close_file( gFileStream.mJobFp )
                            != SQL_SUCCESS, closeError );
        }

        IDE_TEST_RAISE( close_file(gFileStream.mUserFp) 
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mSynFp)
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mDirFp)
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mTblFp) 
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mSeqFp) 
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mViewProcFp) 
                                    != SQL_SUCCESS, closeError );

        /* BUG-36367 aexport must consider new objects, 'package' and 'library' */
        IDE_TEST_RAISE( close_file(gFileStream.mLibFp) 
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mTrigFp) 
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mIndexFp)
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mFkFp) 
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file(gFileStream.mLinkFp) 
                                    != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file( gFileStream.mRefreshMViewFp )
                        != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file( gFileStream.mAltTblFp )
                        != SQL_SUCCESS, closeError );

        IDE_TEST_RAISE( close_file( gFileStream.mDbStatsFp )
                        != SQL_SUCCESS, closeError );
    }
    else
    {
           IDE_TEST_RAISE( close_file(gFileStream.mObjFp) 
                                      != SQL_SUCCESS, closeError );

           IDE_TEST_RAISE( close_file(gFileStream.mObjConFp) 
                                      != SQL_SUCCESS, closeError );

    }
    
    if ( gProgOption.mbExistInvalidScript == ID_TRUE )
    {
        IDE_TEST_RAISE( close_file(gFileStream.mInvalidFp) 
                                    != SQL_SUCCESS, closeError );
    }
    
    IDE_TEST_RAISE( close_file(gFileStream.mIsFp )
                                    != SQL_SUCCESS, closeError );

    if ( gProgOption.mbExistTwoPhaseScript == ID_FALSE )
    {
        if ( gProgOption.mbExistScriptIsqlIndex == ID_TRUE )
        {
            IDE_TEST_RAISE( close_file(gFileStream.mIsIndexFp )
                                    != SQL_SUCCESS, closeError );
        }

        if ( gProgOption.mbExistScriptIsqlFK == ID_TRUE )
        {
            IDE_TEST_RAISE( close_file( gFileStream.mIsFkFp ) 
                                    != SQL_SUCCESS, closeError );
        }

        if( gProgOption.mbExistScriptRepl == ID_TRUE )
        {
            IDE_TEST_RAISE( close_file( gFileStream.mIsReplFp)
                                    != SQL_SUCCESS, closeError );
        }

        if ( gProgOption.mbExistScriptRefreshMView == ID_TRUE )
        {
            IDE_TEST_RAISE( close_file( gFileStream.mIsRefreshMViewFp )
                            != SQL_SUCCESS, closeError );
        }

        if ( gProgOption.mbExistScriptJob == ID_TRUE )
        {
            IDE_TEST_RAISE( close_file( gFileStream.mIsJobFp )
                            != SQL_SUCCESS, closeError );
        }
        else
        {
            /* Nothing to do */
        }

        if ( gProgOption.mbExistScriptAlterTable == ID_TRUE )
        {
            IDE_TEST_RAISE( close_file( gFileStream.mIsAltTblFp )
                            != SQL_SUCCESS, closeError );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        IDE_TEST_RAISE( close_file(gFileStream.mIsConFp ) 
                                    != SQL_SUCCESS, closeError );
    }

    IDE_TEST_RAISE( close_file(gFileStream.mIlOutFp)
                                    != SQL_SUCCESS, closeError );

    IDE_TEST_RAISE( close_file(gFileStream.mIlInFp) 
                               != SQL_SUCCESS, closeError );


    return IDE_SUCCESS;

    IDE_EXCEPTION(closeError);
    {
        uteSetErrorCode(&gErrorMgr,
                        utERR_ABORT_openFileError,
                        "sql file");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-39895 Need to handle aexport out fails with non-table create privileged user.
static IDE_RC hasEnoughPriv4Analysis(SChar* aUserName)
{
    SQLHSTMT   s_userStmt = SQL_NULL_HSTMT;
    SChar      s_query[QUERY_LEN / 2];
    SInt       s_Count = 0;
    SQLRETURN  sRet;

    if ( idlOS::strcasecmp( aUserName, (SChar*)UTM_STR_SYS ) != 0 )
    {
        IDE_TEST_RAISE(SQLAllocStmt(m_hdbc, &s_userStmt) != SQL_SUCCESS, DBCError);

        idlOS::sprintf( s_query, GET_CREATE_TABLE_PRIV_COUNT_QUERY );
        IDE_TEST_RAISE(
                (Prepare(s_query, s_userStmt) != SQL_SUCCESS), UserStmtError);

        IDE_TEST_RAISE(
                SQLBindParameter(s_userStmt, 1, SQL_PARAM_INPUT,
                    SQL_C_CHAR, SQL_VARCHAR, UTM_NAME_LEN, 0,
                    aUserName, UTM_NAME_LEN+1, NULL)
                != SQL_SUCCESS, UserStmtError);

        IDE_TEST_RAISE(
                SQLBindCol(s_userStmt, 1, SQL_C_SLONG, (SQLPOINTER)&s_Count,
                    0, NULL)
                != SQL_SUCCESS, UserStmtError);

        IDE_TEST_RAISE(Execute(s_userStmt) != SQL_SUCCESS, UserStmtError);
        sRet = SQLFetch(s_userStmt);
        IDE_TEST_RAISE(sRet != SQL_SUCCESS, UserStmtError);
        FreeStmt( &s_userStmt );
        // CHECK USER HAS ENOUGH PRIVILEGE TO CREATE TABLE ( ALL, CREATE_TABLE, CREATE_ANY_TABLE )
        IDE_TEST_RAISE(s_Count < 1, InsufficientPrivError);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(DBCError);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_DBC, (SQLHANDLE)m_hdbc);
    }
    IDE_EXCEPTION(UserStmtError);
    {
        utmSetErrorMsgWithHandle(SQL_HANDLE_STMT, (SQLHANDLE)s_userStmt);
    }
    IDE_EXCEPTION(InsufficientPrivError);
    {
        uteSetErrorCode(&gErrorMgr, utERR_ABORT_Insufficient_Priv_Error);
    }
    IDE_EXCEPTION_END;

    printError();

    if ( s_userStmt != SQL_NULL_HSTMT )
    {
        FreeStmt( &s_userStmt );
    }

    return IDE_FAILURE;
}

static IDE_RC doExport()
{
    SChar      sUserName[UTM_NAME_LEN+1];
    SChar      sPasswd[50];
    SChar     *sObjName = NULL;

    /* BUG-36593: like BUG-17563(iloader 에서 큰따옴표 이용한 Naming Rule 제약 제거) */
    idlOS::strcpy( sUserName, gProgOption.GetUserNameInSQL() );
    idlOS::strcpy( sPasswd, gProgOption.GetPassword() );

    // BUG-39895 Need to handle aexport out fails with non-table create privileged user.
    IDE_TEST( hasEnoughPriv4Analysis(sUserName) != SQL_SUCCESS );

    IDE_TEST( createFile( sUserName ) != SQL_SUCCESS );

    /* BUG-40174 Support export and import DBMS Stats */
    if ( gProgOption.mbCollectDbStats == ID_TRUE )
    {
        IDE_TEST( getSystemStats(gFileStream.mDbStatsFp) != SQL_SUCCESS );
    }

    if ( idlOS::strcasecmp( sUserName, (SChar*)UTM_STR_SYS ) == 0 )
    {
        IDE_TEST_RAISE( getTBSQuery( gFileStream.mTbsFp)
                                   != SQL_SUCCESS, init_error );

        IDE_TEST_RAISE( getUserQuery( gFileStream.mUserFp,
                                      &m_user_cnt,
                                      sUserName,
                                      sPasswd ) != SQL_SUCCESS,
                                      table_error);
        IDE_TEST_RAISE( getSynonymAll( gFileStream.mSynFp)
                                       != SQL_SUCCESS,
                                       table_error );
    
        //BUG-22708 directory의 소유자는 SYS이므로, 이 경우에만 처리되어야 함..
        IDE_TEST_RAISE( getDirectoryAll( gFileStream.mDirFp )
                                         != SQL_SUCCESS, table_error );
    }
    else
    {
        m_user_cnt = 1;
        
        m_UserInfo = (UserInfo *) idlOS::malloc(sizeof(UserInfo) * m_user_cnt);
        
        IDE_TEST(m_UserInfo == NULL);
        
        idlOS::strcpy(m_UserInfo[0].m_user, sUserName);
        
        idlOS::strcpy(m_UserInfo[0].m_passwd, sPasswd );
        
        IDE_TEST_RAISE( getUserQuery_user( gFileStream.mUserFp,
                                           sUserName,
                                           sPasswd ) != SQL_SUCCESS,
                                           table_error);
        
        IDE_TEST_RAISE(getSynonymUser( sUserName,
                                       gFileStream.mSynFp )
                                       != SQL_SUCCESS,
                                       table_error);

    }

    IDE_TEST_RAISE( getTableInfo( sUserName,
                                  gFileStream.mIlOutFp,
                                  gFileStream.mIlInFp,
                                  gFileStream.mTblFp,
                                  gFileStream.mIndexFp,
                                  gFileStream.mAltTblFp, /* PROJ-2359 Table/Partition Access Option */
                                  gFileStream.mDbStatsFp
                                )
                    != SQL_SUCCESS, table_error );

    IDE_TEST_RAISE( getForeignKeys( sUserName,
                                    gFileStream.mFkFp ) != SQL_SUCCESS,
                                    table_error );

    IDE_TEST_RAISE( getQueueInfo( sUserName,
                                  gFileStream.mTblFp,
                                  gFileStream.mIlOutFp,
                                  gFileStream.mIlInFp)
                                  != SQL_SUCCESS, table_error );

    IDE_TEST_RAISE( getSeqQuery( sUserName, sObjName,
                                 gFileStream.mSeqFp )
                                 != SQL_SUCCESS, table_error);

    IDE_TEST_RAISE( getDBLinkQuery( sUserName, 
                                    gFileStream.mLinkFp )
                    != SQL_SUCCESS, table_error);

    IDE_TEST_RAISE( getViewProcQuery( sUserName,
                                      gFileStream.mViewProcFp,
                                      gFileStream.mRefreshMViewFp ) /* PROJ-2211 Materialized View */
                    != SQL_SUCCESS, table_error );

    IDE_TEST_RAISE( getTrigQuery( sUserName, gFileStream.mTrigFp )
                                 != SQL_SUCCESS, table_error );

    /* BUG-36367 aexport must consider new objects, 'package' and 'library' */
    IDE_TEST_RAISE( getLibQuery( gFileStream.mLibFp,
                                 sUserName,                 
                                 sObjName) != SQL_SUCCESS, table_error);
 
    if ( idlOS::strcasecmp( sUserName, (SChar*)UTM_STR_SYS ) == 0 )
    {
        IDE_TEST_RAISE(getReplQuery( gFileStream.mReplFp ) != SQL_SUCCESS,
                       table_error);

        IDE_TEST_RAISE( getJobQuery( gFileStream.mJobFp ) != SQL_SUCCESS,
                        table_error);
    }
    
    doMakeRunScript();

    IDE_TEST( destroyFile( sUserName ) != SQL_SUCCESS );

    if ( idlOS::strcmp( gProgOption.mExec, "ON" ) == 0 )
    {
        exec_script();
    }
    else
    {
        show_runmsg();
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(init_error);
    {
        printError();
    }
    IDE_EXCEPTION(table_error);
    {
        printError();

        destroyFile ( sUserName );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC doObjModeExport()
{
    SInt   i;
    /* BUG-39622: file_name="[user_name]_[object_name]_CRT_XXXX.sql" */
    SChar  sCreateFileName[UTM_NAME_LEN*2+20];
    SChar  sExecStatsFileName[UTM_NAME_LEN*2+20];
    FILE  *sCreateFp  = NULL;
    FILE  *sExecStatsFp  = NULL;

    IDE_TEST( objectModeInfoQuery() != SQL_SUCCESS ); 

    for ( i = 0 ; i < gProgOption.mObjModeOptCount; i++ )
    {
        switch( gObjectModeInfo[i].mObjObjectType )
        {
            case UTM_TABLE:
                idlOS::sprintf( sCreateFileName,"%s_%s_CRT.sql",  
                                          gObjectModeInfo[i].mObjUserName,
                                          gObjectModeInfo[i].mObjObjectName );

                IDE_TEST_RAISE( open_file( sCreateFileName, &sCreateFp ) 
                                           != SQL_SUCCESS,openError);

                /* BUG-40174 Support export and import DBMS Stats */
                if ( gProgOption.mbCollectDbStats == ID_TRUE )
                {
                    idlOS::sprintf( sExecStatsFileName,"%s_%s_STATS.sql",  
                            gObjectModeInfo[i].mObjUserName,
                            gObjectModeInfo[i].mObjObjectName );

                    IDE_TEST_RAISE( open_file( sExecStatsFileName, &sExecStatsFp ) 
                            != SQL_SUCCESS,openError);

                    IDE_TEST( getSystemStats(sExecStatsFp) != SQL_SUCCESS );
                }

                IDE_TEST_RAISE( getObjModeTableQuery( sCreateFp,
                                                      sExecStatsFp,
                                                      gObjectModeInfo[i].mObjUserName,
                                                      gObjectModeInfo[i].mObjObjectName )
                                                      != SQL_SUCCESS, table_error);

                IDE_TEST_RAISE( close_file( sCreateFp ) != SQL_SUCCESS, closeError);

                if ( gProgOption.mbCollectDbStats == ID_TRUE )
                {
                    IDE_TEST_RAISE( close_file( sExecStatsFp ) != SQL_SUCCESS, closeError);
                }

                break;

            /* BUG-49242 Queue object mode */
            case UTM_QUEUE:
                idlOS::sprintf( sCreateFileName,"%s_%s_CRT.sql",  
                                          gObjectModeInfo[i].mObjUserName,
                                          gObjectModeInfo[i].mObjObjectName );

                IDE_TEST_RAISE( open_file( sCreateFileName, &sCreateFp ) 
                                           != SQL_SUCCESS,openError );

                IDE_TEST_RAISE( getObjModeQueueQuery( sCreateFp,
                                                      gObjectModeInfo[i].mObjUserName,
                                                      gObjectModeInfo[i].mObjObjectName )
                                                      != SQL_SUCCESS, init_error );

                IDE_TEST_RAISE( close_file( sCreateFp ) != SQL_SUCCESS, closeError );

                break;

            case UTM_SEQUENCE:
                idlOS::sprintf( sCreateFileName,"%s_%s_CRT.sql",  
                                          gObjectModeInfo[i].mObjUserName,
                                          gObjectModeInfo[i].mObjObjectName );

               
                IDE_TEST_RAISE(open_file( sCreateFileName, &sCreateFp ) 
                                          != SQL_SUCCESS,openError);
                
                IDE_TEST_RAISE( getSeqQuery( gObjectModeInfo[i].mObjUserName,
                                             gObjectModeInfo[i].mObjObjectName,
                                             sCreateFp )
                                             != SQL_SUCCESS, init_error);

                IDE_TEST_RAISE( close_file( sCreateFp ) != SQL_SUCCESS, closeError);

                break;
            
            case UTM_VIEW:
                idlOS::sprintf( sCreateFileName,"%s_%s_CRT.sql",  
                                          gObjectModeInfo[i].mObjUserName,
                                          gObjectModeInfo[i].mObjObjectName );


                IDE_TEST_RAISE( open_file( sCreateFileName, &sCreateFp ) 
                                           != SQL_SUCCESS,openError);
                         
                IDE_TEST_RAISE( getObjModeViewQuery( sCreateFp,
                                                      gObjectModeInfo[i].mObjUserName,
                                                      gObjectModeInfo[i].mObjObjectName ) 
                                                      != SQL_SUCCESS, init_error );

                IDE_TEST_RAISE( close_file( sCreateFp ) != SQL_SUCCESS, closeError);

                break;
            
            case UTM_PROCEDURE:
            case UTM_FUNCTION_PROCEDURE:
                idlOS::sprintf( sCreateFileName,"%s_%s_CRT.sql",  
                                          gObjectModeInfo[i].mObjUserName,
                                          gObjectModeInfo[i].mObjObjectName );

               
                IDE_TEST_RAISE(open_file( sCreateFileName, &sCreateFp ) 
                                          != SQL_SUCCESS,openError);
                         
                IDE_TEST_RAISE( getObjModeProcQuery( sCreateFp,
                                                      gObjectModeInfo[i].mObjUserName,
                                                      gObjectModeInfo[i].mObjObjectName ) 
                                                      != SQL_SUCCESS, init_error );

                IDE_TEST_RAISE( close_file( sCreateFp ) != SQL_SUCCESS, closeError);

                break;
            
                /* BUG-36367 aexport must consider new objects, 'package' and 'library' */
            case UTM_PACKAGE_SPEC:
            case UTM_PACKAGE_BODY:
                idlOS::sprintf( sCreateFileName,
                                "%s_%s_CRT.sql",
                                gObjectModeInfo[i].mObjUserName,
                                gObjectModeInfo[i].mObjObjectName);

                IDE_TEST_RAISE( open_file( sCreateFileName, &sCreateFp) != SQL_SUCCESS, openError);
                
                IDE_TEST_RAISE( getObjModePkgQuery( sCreateFp,
                                                    gObjectModeInfo[i].mObjUserName,
                                                    gObjectModeInfo[i].mObjObjectName)
                                                    != SQL_SUCCESS, init_error);
                
                IDE_TEST_RAISE( close_file( sCreateFp) != SQL_SUCCESS, closeError);

                break;
            
            case UTM_MVIEW : /* PROJ-2211 Materialized View */
                idlOS::sprintf( sCreateFileName, "%s_%s_CRT.sql",
                                           gObjectModeInfo[i].mObjUserName,
                                           gObjectModeInfo[i].mObjObjectName );

                IDE_TEST_RAISE( open_file( sCreateFileName, &sCreateFp )
                                != SQL_SUCCESS, openError );

                IDE_TEST_RAISE( getObjModeMViewQuery( sCreateFp,
                                                      gObjectModeInfo[i].mObjUserName,
                                                      gObjectModeInfo[i].mObjObjectName )
                                != SQL_SUCCESS, table_error );

                IDE_TEST_RAISE( close_file( sCreateFp ) != SQL_SUCCESS, closeError );
                break;

            case UTM_LIBRARY : /* BUG-36367 aexport must consider new objects, 'package' and 'library' */
                idlOS::sprintf( sCreateFileName, "%s_%s_CRT.sql",
                                           gObjectModeInfo[i].mObjUserName,
                                           gObjectModeInfo[i].mObjObjectName);

                IDE_TEST_RAISE( open_file( sCreateFileName, &sCreateFp) != SQL_SUCCESS, openError);

                IDE_TEST_RAISE( getLibQuery( sCreateFp,
                                             gObjectModeInfo[i].mObjUserName,
                                             gObjectModeInfo[i].mObjObjectName)
                                != SQL_SUCCESS, table_error);

                IDE_TEST_RAISE( close_file( sCreateFp) != SQL_SUCCESS, closeError);

                break;
             default:
     
                idlOS::fprintf( stderr,"[%s.%s is an unknown or non existent object type.]\n",
                                        gObjectModeInfo[i].mObjUserName, gObjectModeInfo[i].mObjObjectName );
               break;
        }
    }

    idlOS::free( gObjectModeInfo );
    gObjectModeInfo = NULL;

   return IDE_SUCCESS;
    
    IDE_EXCEPTION(openError);
    {
        printError();
    }
    IDE_EXCEPTION(closeError);
    {
        printError();
    }
    IDE_EXCEPTION(init_error);
    {
        printError();
    }
    IDE_EXCEPTION(table_error);
    {
        printError();
    }
    IDE_EXCEPTION_END;
    
    // BUG-36779: [ux] Codesonar warning UX part - codesonar warning 158523.2579804
    if( sCreateFp != NULL )
    {
        (void)close_file( sCreateFp );
    }

    if( sExecStatsFp != NULL )
    {
        (void)close_file( sExecStatsFp );
    }

    idlOS::free( gObjectModeInfo );
    gObjectModeInfo = NULL;

    return IDE_FAILURE;
}


static IDE_RC doImport()
{
    exec_script();

    return IDE_SUCCESS;
}

static IDE_RC doMakeRunScript()
{
    SChar  sUserName[UTM_NAME_LEN+1];
    SChar  sPasswd[50];
    SChar *sScriptHeadName = NULL;    

    /* BUG-34502: handling quoted identifiers */
    idBool sNeedQuote4User   = ID_FALSE;
    idBool sNeedQuote4Pwd = ID_FALSE;
    idBool sNeedQuote4File   = ID_FALSE;

    /* BUG-36593: like BUG-17563(iloader 에서 큰따옴표 이용한 Naming Rule 제약 제거) */
    idlOS::strcpy(sUserName, gProgOption.GetUserNameInSQL());
    idlOS::strcpy(sPasswd, gProgOption.GetPassword());

    if ( idlOS::strcasecmp( sUserName, (SChar*)UTM_STR_SYS ) == 0 )
    {
        sScriptHeadName = (SChar *) UTM_STR_ALL;

        sNeedQuote4User = ID_FALSE;
        sNeedQuote4Pwd = utString::needQuotationMarksForFile(sPasswd);
        sNeedQuote4File = ID_FALSE;
    }
    else
    {
        sScriptHeadName = sUserName;        

        sNeedQuote4User = utString::needQuotationMarksForObject(sUserName);
        sNeedQuote4Pwd = utString::needQuotationMarksForFile(sPasswd);
        sNeedQuote4File = utString::needQuotationMarksForFile(sScriptHeadName);
    }

    if ( gProgOption.mbExistTwoPhaseScript == ID_FALSE )
    {
        if ( idlOS::strcasecmp( sUserName, (SChar*)UTM_STR_SYS ) == 0 )
        {
            /* ..._CRT_TBS.sql */
            idlOS::fprintf( gFileStream.mIsFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_TBS" );
            idlOS::fprintf( gFileStream.mIsFp, "\n");
 
            /* ..._CRT_USER.sql */
            idlOS::fprintf( gFileStream.mIsFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_USER" );
            idlOS::fprintf( gFileStream.mIsFp, "\n");
        }
        else
        {
            /* BUG-40469 output tablespaces info. in user mode */
            /* ..._CRT_TBS.sql */
            if ( gProgOption.mbCrtTbs4UserMode == ID_TRUE )
            {
                idlOS::fprintf( gFileStream.mIsFp,
                                ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
                idlOS::fprintf( gFileStream.mIsFp,
                                gUserOptionFormatStr[0], UTM_STR_SYS );
                idlOS::fprintf( gFileStream.mIsFp,
                                gPwdOptionFormatStr[0], UTM_STR_MANAGER );
                idlOS::fprintf( gFileStream.mIsFp,
                                gOptionSqlFile[sNeedQuote4File],
                                sScriptHeadName, "CRT_TBS" );
                idlOS::fprintf( gFileStream.mIsFp, "\n");
            }
 
            /* ..._CRT_USER.sql */
            idlOS::fprintf( gFileStream.mIsFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsFp,
                            gUserOptionFormatStr[0], UTM_STR_SYS );
            idlOS::fprintf( gFileStream.mIsFp,
                            gPwdOptionFormatStr[0], UTM_STR_MANAGER );
            idlOS::fprintf( gFileStream.mIsFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_USER" );
            idlOS::fprintf( gFileStream.mIsFp, "\n");
        }

        /* ..._CRT_SYN.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_SYN" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");

        /* ..._CRT_DIR.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_DIR" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");

        /* ..._CRT_SEQ.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_SEQ" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");
       
        /* ..._CRT_TBL.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_TBL" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");

        /* BUG-36367 aexport must consider new objects, 'package' and 'library' */
        /* ..._CRT_LIB.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_LIB" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");

        /* ..._CRT_VIEW_PROC.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_VIEW_PROC" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");

        /* ..._CRT_LINK.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_LINK" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");

        if ( gProgOption.mbExistScriptIsqlIndex == ID_TRUE )
        {
            /* ..._CRT_INDEX.sql */
            idlOS::fprintf( gFileStream.mIsIndexFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsIndexFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsIndexFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsIndexFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_INDEX" );
            idlOS::fprintf( gFileStream.mIsIndexFp, "\n");
        }
        
        if ( gProgOption.mbExistScriptIsqlFK == ID_TRUE )
        {
            /* ..._CRT_FK.sql */
            idlOS::fprintf( gFileStream.mIsFkFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsFkFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsFkFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsFkFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_FK" );
            idlOS::fprintf( gFileStream.mIsFkFp, "\n");

            /* ..._CRT_TRIG.sql */
            idlOS::fprintf( gFileStream.mIsFkFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsFkFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsFkFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsFkFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_TRIG" );
            idlOS::fprintf( gFileStream.mIsFkFp, "\n");
        }

        if( gProgOption.mbExistScriptRepl == ID_TRUE )
        {
            /* ..._CRT_REP.sql */
            idlOS::fprintf( gFileStream.mIsReplFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsReplFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsReplFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsReplFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_REP" );
            idlOS::fprintf( gFileStream.mIsReplFp, "\n");
        }

        if ( gProgOption.mbExistScriptRefreshMView == ID_TRUE )
        {
            /* ..._REFRESH_MVIEW.sql */
            idlOS::fprintf( gFileStream.mIsRefreshMViewFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsRefreshMViewFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsRefreshMViewFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsRefreshMViewFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "REFRESH_MVIEW" );
            idlOS::fprintf( gFileStream.mIsRefreshMViewFp, "\n");
        }

        if ( gProgOption.mbExistScriptJob == ID_TRUE )
        {
            /* ..._CRT_JOB.sql */
            idlOS::fprintf( gFileStream.mIsJobFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsJobFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsJobFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsJobFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "CRT_JOB" );
            idlOS::fprintf( gFileStream.mIsJobFp, "\n");
        }
        else
        {
            /* Nothing to do */
        }

        if ( gProgOption.mbExistScriptAlterTable == ID_TRUE )
        {
            /* ..._ALT_TBL.sql */
            idlOS::fprintf( gFileStream.mIsAltTblFp,
                            ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
            idlOS::fprintf( gFileStream.mIsAltTblFp,
                            gUserOptionFormatStr[sNeedQuote4User], sUserName );
            idlOS::fprintf( gFileStream.mIsAltTblFp,
                            gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
            idlOS::fprintf( gFileStream.mIsAltTblFp,
                            gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "ALT_TBL" );
            idlOS::fprintf( gFileStream.mIsAltTblFp, "\n");
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* ..._OBJECT.sql */
        idlOS::fprintf( gFileStream.mIsFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "OBJECT" );
        idlOS::fprintf( gFileStream.mIsFp, "\n");

        /* ..._OBJECT_CONSTRAINTS.sql */
        idlOS::fprintf( gFileStream.mIsConFp,
                        ISQL_PRODUCT_NAME" %s", gProgOption.mInConnectStr );
        idlOS::fprintf( gFileStream.mIsConFp,
                        gUserOptionFormatStr[sNeedQuote4User], sUserName );
        idlOS::fprintf( gFileStream.mIsConFp,
                        gPwdOptionFormatStr[sNeedQuote4Pwd], sPasswd );
        idlOS::fprintf( gFileStream.mIsConFp,
                        gOptionSqlFile[sNeedQuote4File], sScriptHeadName, "OBJECT_CONSTRAINTS" );
        idlOS::fprintf( gFileStream.mIsConFp, "\n");
    }

    return IDE_SUCCESS;
}

static void show_copyright()
{
    idlOS::fprintf(stdout, "-----------------------------------------------------------------\n");
    idlOS::fprintf(stdout, "     Altibase Export Script Utility.\n");
    idlOS::fprintf(stdout, "     Release Version %s\n", iduVersionString);
    idlOS::fprintf(stdout, "     Copyright 2000, ALTIBASE Corporation or its subsidiaries.\n");
    idlOS::fprintf(stdout, "     All Rights Reserved.\n");
    idlOS::fprintf(stdout, "-----------------------------------------------------------------\n");
//    idlOS::fprintf(stdout, "\n");
    idlOS::fflush(stdout);
}

static void show_runmsg()
{
    SInt sIdx = 1;
    
    idlOS::fprintf(stdout, "-------------------------------------------------------\n");
    idlOS::fprintf(stdout, "  ##### The following script files were generated. #####\n");
    idlOS::fprintf(stdout, "  %d. %-24s : [ %s formout, data-out script ]\n",
                           sIdx++, gProgOption.mScriptIloOut, ILO_PRODUCT_NAME);
    idlOS::fprintf(stdout, "  %d. %-24s : [ %s table-schema script ]\n",
                           sIdx++, gProgOption.mScriptIsql, ISQL_PRODUCT_NAME);
    idlOS::fprintf(stdout, "  %d. %-24s : [ %s data-in script ]\n",
                           sIdx++, gProgOption.mScriptIloIn, ILO_PRODUCT_NAME);

    if ( gProgOption.mbExistTwoPhaseScript == ID_TRUE )
    {
        idlOS::fprintf(stdout, "  %d. %-24s \n", sIdx++, gProgOption.mScriptIsqlCon);
        if ( gProgOption.mbExistScriptRefreshMView == ID_TRUE )
        {
            idlOS::fprintf(stdout, "           - [ %s materialized view refresh script ]\n", ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistIndex == ID_TRUE )
        {
            idlOS::fprintf(stdout, "           - [ %s table-index script ]\n", ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistScriptIsqlFK == ID_TRUE )
        {
            idlOS::fprintf(stdout, "           - [ %s table-foreign key script ]\n", ISQL_PRODUCT_NAME);
        }
        if( gProgOption.mbExistScriptRepl == ID_TRUE )
        {
            idlOS::fprintf(stdout, "           - [ %s replication script ]\n", ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistScriptJob == ID_TRUE )
        {
            idlOS::fprintf(stdout, "           - [ %s job script ]\n", ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistScriptAlterTable == ID_TRUE )
        {
            idlOS::fprintf(stdout, "           - [ %s table-alter script ]\n", ISQL_PRODUCT_NAME);
        }

    }
    else
    {
        if ( gProgOption.mbExistScriptRefreshMView == ID_TRUE )
        {
            idlOS::fprintf(stdout, "  %d. %-24s : [ %s materialized view refresh script ]\n",
                                   sIdx++, gProgOption.mScriptIsqlRefreshMView, ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistIndex == ID_TRUE )
        {
            idlOS::fprintf(stdout, "  %d. %-24s : [ %s table-index script ]\n",
                                   sIdx++, gProgOption.mScriptIsqlIndex, ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistScriptIsqlFK == ID_TRUE )
        {
            idlOS::fprintf(stdout, "  %d. %-24s : [ %s table-foreign key script ]\n",
                                   sIdx++, gProgOption.mScriptIsqlFK, ISQL_PRODUCT_NAME);
        }
        if( gProgOption.mbExistScriptRepl == ID_TRUE )
        {
            idlOS::fprintf(stdout, "  %d. %-24s : [ %s replication script ]\n",
                                   sIdx++, gProgOption.mScriptIsqlRepl, ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistScriptJob == ID_TRUE )
        {
            idlOS::fprintf(stdout, "  %d. %-24s : [ %s job script ]\n",
                                   sIdx++, gProgOption.mScriptIsqlJob, ISQL_PRODUCT_NAME);
        }
        if ( gProgOption.mbExistScriptAlterTable == ID_TRUE )
        {
            idlOS::fprintf(stdout, "  %d. %-24s : [ %s table-alter script ]\n",
                                   sIdx++, gProgOption.mScriptIsqlAlterTable, ISQL_PRODUCT_NAME);
        }
    }

    idlOS::fprintf(stdout, "-------------------------------------------------------\n");
    idlOS::fprintf(stdout, "\n");
    idlOS::fflush(stdout);
}

static void exec_script()
{
    SInt  sIdx = 1;
    SChar sCommand[50];

    show_runmsg();

#if defined(VC_WIN32)
#define COMMAND_FORMAT "type %s | cmd"
#else
#define COMMAND_FORMAT "sh %s"
#endif

    if ( idlOS::strcmp(gProgOption.mOper, "OUT") == 0 )
    {
        idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
        idlOS::fprintf(stdout, "  ##### The following script file is running. #####\n");
        idlOS::fprintf(stdout, "  %d. %-24s : [ %s formout, data-out script ]\n",
                               sIdx++, gProgOption.mScriptIloOut, ILO_PRODUCT_NAME);
        idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIloOut);
        idlOS::system(sCommand);
        idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
    }
    else if ( idlOS::strcmp(gProgOption.mOper, "IN") == 0 )
    {
        idlOS::fprintf(stdout, "\n");
        idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
        idlOS::fprintf(stdout, "  %d. %-24s : [ %s table-schema script ]\n",
                               sIdx++, gProgOption.mScriptIsql, ISQL_PRODUCT_NAME);
        idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
        idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIsql);
        idlOS::system(sCommand);

        idlOS::fprintf(stdout, "\n");
        idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
        idlOS::fprintf(stdout, "  %d. %-24s : [ %s data-in script ]\n",
                               sIdx++, gProgOption.mScriptIloIn, ILO_PRODUCT_NAME);
        idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
        idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIloIn);
        idlOS::system(sCommand);

        if ( gProgOption.mbExistTwoPhaseScript == ID_FALSE )
        {
            if ( gProgOption.mbExistScriptRefreshMView == ID_TRUE )
            {
                idlOS::fprintf( stdout, "\n" );
                idlOS::fprintf( stdout, "-----------------------------------------------------------\n" );
                idlOS::fprintf( stdout, "  %d. %-24s : [ %s materialized view refresh script ]\n",
                                        sIdx++, gProgOption.mScriptIsqlRefreshMView, ISQL_PRODUCT_NAME );
                idlOS::fprintf( stdout, "-----------------------------------------------------------\n" );
                idlOS::sprintf( sCommand, COMMAND_FORMAT, gProgOption.mScriptIsqlRefreshMView );
                idlOS::system( sCommand );
            }

            if ( gProgOption.mbExistIndex == ID_TRUE )
            {
                idlOS::fprintf(stdout, "\n");
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::fprintf(stdout, "  %d. %-24s : [ %s table-index script ]\n",
                                       sIdx++, gProgOption.mScriptIsqlIndex, ISQL_PRODUCT_NAME);
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIsqlIndex);
                idlOS::system(sCommand);
            }

            if ( gProgOption.mbExistScriptIsqlFK == ID_TRUE )
            {
                idlOS::fprintf(stdout, "\n");
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::fprintf(stdout, "  %d. %-24s : [ %s table-foreign key script ]\n",
                                       sIdx++, gProgOption.mScriptIsqlFK, ISQL_PRODUCT_NAME);
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIsqlFK);
                idlOS::system(sCommand);
            }

            if( gProgOption.mbExistScriptRepl == ID_TRUE )
            {
                idlOS::fprintf(stdout, "\n");
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::fprintf(stdout, "  %d. %-24s : [ %s replication script ]\n",
                                       sIdx++, gProgOption.mScriptIsqlRepl, ISQL_PRODUCT_NAME);
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIsqlRepl);
                idlOS::system(sCommand);
            }

            if ( gProgOption.mbExistScriptJob == ID_TRUE )
            {
                idlOS::fprintf(stdout, "\n");
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::fprintf(stdout, "  %d. %-24s : [ %s Job script ]\n",
                                       sIdx++, gProgOption.mScriptIsqlJob, ISQL_PRODUCT_NAME);
                idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
                idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIsqlJob);
                ( void )idlOS::system(sCommand);
            }
            else
            {
                /* Nothing to do */
            }
            
            if ( gProgOption.mbExistScriptAlterTable == ID_TRUE )
            {
                idlOS::fprintf( stdout, "\n" );
                idlOS::fprintf( stdout, "-----------------------------------------------------------\n" );
                idlOS::fprintf( stdout, "  %d. %-24s : [ %s table-alter script ]\n",
                                        sIdx++, gProgOption.mScriptIsqlAlterTable, ISQL_PRODUCT_NAME );
                idlOS::fprintf( stdout, "-----------------------------------------------------------\n" );
                idlOS::sprintf( sCommand, COMMAND_FORMAT, gProgOption.mScriptIsqlAlterTable );
                idlOS::system( sCommand );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
            idlOS::fprintf(stdout, "  %d. %-24s : [ %s constraints, replication script ]\n",
                                   sIdx++, gProgOption.mScriptIsqlCon, ISQL_PRODUCT_NAME);
            idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
            idlOS::sprintf(sCommand, COMMAND_FORMAT, gProgOption.mScriptIsqlCon);
            idlOS::system(sCommand);
        }
        idlOS::fprintf(stdout, "-----------------------------------------------------------\n");
    }
}
