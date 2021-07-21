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

#include <ulpLibInterFuncA.h>
#include <ulnParse.h>

/* Connection hash table을 관리하기 위한 extern 변수. */
extern ulpLibConnMgr gUlpConnMgr;

/* BUG-28209 : AIX 에서 c compiler로 컴파일하면 생성자 호출안됨. */
ACI_RC ulpInitializeConnMgr( void )
{
/***********************************************************************
 *
 * Description :
 *    Initialize the ulpConnMgr class.
 *
 ***********************************************************************/
    return ulpLibConInitConnMgr();
}


/* =========================================================
 *  << Library internal functions >>
 *
 *  Description :
 *     Interface ulpDoEmsql 에서 호출되는 함수들로, 함수 하나당 하나의 내장구문 처리를 맡는다.
 *
 *  Parameters : 모든 함수들이 동일한 인자type을 갖는다.
 *     acp_char_t *aConnName   : AT절에 명시된 connection 이름.
 *     ulpSqlstmt *aSqlstmt    : SQL내장구문에 대한 정보.
 *     void *aReserved         : 예약인자.
 *
 * ========================================================*/

ACI_RC ulpSetOptionThread( acp_char_t *aConnName , ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    library option인 multithread를 설정 한다.
 *
 *    처리구문> EXEC SQL OPTION ( THREADS={TRUE|FALSE} );
 *
 * Implementation :
 *
 ***********************************************************************/
    ACP_UNUSED(aConnName);
    ACP_UNUSED(reserved);
    gUlpLibOption.mIsMultiThread = (acp_bool_t) aSqlstmt->sqlinfo;
    return ACI_SUCCESS;
}

ACI_RC ulpConnect( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    DB에 연결을 맺는다.
 *
 *    처리구문> EXEC SQL [ AT <conn_name | :conn_name> ] CONNECT <:user>
 *            IDENTIFIED BY <:passwd> [ USING <:conn_opt1> [ , <:conn_opt2> ] ];
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_sint32_t       sI;
    acp_sint32_t       sLen;
    acp_char_t         sConnStr[MAX_CONN_STR + 1] = {'\0', };
    ulpLibConnNode    *sConnNode;
    acp_char_t         sErrMsg[MAX_ERRMSG_LEN];
    SQLRETURN          sSqlRes;
    acp_bool_t         sIsFirstFail;
    acp_bool_t         sIsAllocNew;
    acp_bool_t         sIsLatched;
    ulnConnStrAttrType sConnStrAttrType;
    acp_rc_t           sRC;

    ACP_UNUSED(reserved);

    sIsLatched   = ACP_FALSE;
    sIsFirstFail = ACP_FALSE;
    sIsAllocNew  = ACP_FALSE;
    sErrMsg[0]   = '\0';

    // BUG-39202 [ux-apre] The concurrency problem is occurred when the multi-thread use same connection name.
    if ( gUlpLibOption.mIsMultiThread == ACP_TRUE )
    {
        /* get write lock */
        ACI_TEST_RAISE( acpThrRwlockLockWrite( &gUlpConnMgr.mConnLatch )
                        != ACP_RC_SUCCESS, ERR_LATCH_WRITE );

        sIsLatched = ACP_TRUE;
    }

    if ( aConnName != NULL )
    {
        /*aConnName으로 Connection node(ConnNode)를 찾는다*/
        sConnNode = ulpLibConLookupConn( aConnName );

        if ( sConnNode == NULL )
        {
            /*aConnName의 ConnNode를 새로 생성한다*/
            sConnNode = ulpLibConNewNode( aConnName, ACP_FALSE );
            /*memory alloc error*/
            ACI_TEST_RAISE( sConnNode == NULL, ERR_CONN_NAME_OVERFLOW );
            sIsAllocNew  = ACP_TRUE;
        }
        else
        {
            /*이미 Connect 되어 있다면 error발생함*/
            ACI_RAISE( ERR_ALREADY_EXIST );
        }
    }
    else
    {
        sConnNode = ulpLibConGetDefaultConn();
        ACI_TEST_RAISE( sConnNode->mHdbc != SQL_NULL_HDBC, ERR_ALREADY_EXIST );
    }

    ACI_TEST_RAISE( sConnNode->mIsXa == ACP_TRUE, ERR_ALREADY_EXIST );

    for ( sI = 0 ; sI < aSqlstmt->numofhostvar ; sI++)
    {
        switch(sI)
        {
            case 0:
                sConnNode->mUser = (acp_char_t *)aSqlstmt->hostvalue[0].mHostVar;
                sConnStrAttrType = ulnConnStrGetAttrType(sConnNode->mUser);
                ACI_TEST_RAISE( sConnStrAttrType == ULN_CONNSTR_ATTR_TYPE_NONE ||
                                sConnStrAttrType == ULN_CONNSTR_ATTR_TYPE_INVALID, ERR_INVALID_USER );
                break;
            case 1:
                sConnNode->mPasswd = (acp_char_t *)aSqlstmt->hostvalue[1].mHostVar;
                sConnStrAttrType = ulnConnStrGetAttrType(sConnNode->mPasswd);
                ACI_TEST_RAISE( sConnStrAttrType == ULN_CONNSTR_ATTR_TYPE_NONE ||
                                sConnStrAttrType == ULN_CONNSTR_ATTR_TYPE_INVALID, ERR_INVALID_PASSWD );
                break;
            case 2:
                sConnNode->mConnOpt1 = (acp_char_t *)aSqlstmt->hostvalue[2].mHostVar;
                break;
            case 3:
                sConnNode->mConnOpt2 = (acp_char_t *)aSqlstmt->hostvalue[3].mHostVar;
                break;
            default:
                ACE_ASSERT(0);
        }
    }

    sSqlRes = SQLAllocEnv ( &(sConnNode->mHenv) );

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR || sSqlRes == SQL_INVALID_HANDLE),
                    ERR_CLI_ALLOC_ENV );
    sSqlRes = SQLAllocConnect ( sConnNode->mHenv, &(sConnNode->mHdbc) );

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR || sSqlRes == SQL_INVALID_HANDLE),
                    ERR_CLI_ALLOC_CONN );

    sRC = ulnConnStrAppendStrAttr( sConnStr, MAX_CONN_STR + 1, "UID", sConnNode->mUser );
    ACE_ASSERT( sRC != ACP_RC_EINVAL );
    ACI_TEST_RAISE( sRC != ACP_RC_SUCCESS, ERR_CONNSTR_OVERFLOW );
    sRC = ulnConnStrAppendStrAttr( sConnStr, MAX_CONN_STR + 1, "PWD", sConnNode->mPasswd );
    ACE_ASSERT( sRC != ACP_RC_EINVAL );
    ACI_TEST_RAISE( sRC != ACP_RC_SUCCESS, ERR_CONNSTR_OVERFLOW );
    sLen = acpCStrLen( sConnStr, ACP_SINT32_MAX );
    if( sConnNode->mConnOpt1 != NULL )
    {
        ACI_TEST_RAISE( acpCStrCat( sConnStr, MAX_CONN_STR + 1,
                                    sConnNode->mConnOpt1,
                                    acpCStrLen( sConnNode->mConnOpt1, ACP_SINT32_MAX ) )
                        != ACP_RC_SUCCESS, ERR_CONNSTR_OVERFLOW );
    }
    else
    {
        ACI_TEST_RAISE( acpCStrCat( sConnStr, MAX_CONN_STR + 1,
                                    "DSN=127.0.0.1;CONNTYPE=1", 24 )
                        != ACP_RC_SUCCESS, ERR_CONNSTR_OVERFLOW );
    }

    /*BUG-44651 멀티쓰레드에서 apre수행시 동시성문제로 순차적으로 대기하는 문제를 수정한다.*/
    if ( sIsAllocNew == ACP_TRUE )
    {
        /*connection hash table에 추가.연결이 완료되었다는 의미는 아님.*/
        ACI_TEST_RAISE( ulpLibConAddConn( sConnNode ) == NULL,
                        ERR_ALREADY_EXIST );
    }

    if ( sIsLatched == ACP_TRUE )
    {
        /* release write lock */
        ACI_TEST_RAISE( acpThrRwlockUnlock( &gUlpConnMgr.mConnLatch )
                        != ACP_RC_SUCCESS, ERR_LATCH_RELEASE );

        sIsLatched = ACP_FALSE;
    }

    sSqlRes = SQLDriverConnect ( sConnNode->mHdbc,
                                 NULL,
                                 (SQLCHAR *)sConnStr,
                                 SQL_NTS,
                                 NULL,
                                 0,
                                 NULL,
                                 SQL_DRIVER_NOPROMPT );

    ACI_TEST_RAISE ( sSqlRes == SQL_INVALID_HANDLE, ERR_CLI_CONNECT );

    if ( sSqlRes == SQL_SUCCESS || sSqlRes == SQL_SUCCESS_WITH_INFO )
    {
        ulpSetDateFmtCore( sConnNode );
    }

    if ( sSqlRes == SQL_ERROR )
    {
        if ( sConnNode->mConnOpt2 != NULL )
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr, utERR_ABORT_Conn_First_Trial_Failed );
            acpCStrCat( sErrMsg, MAX_ERRMSG_LEN-1,
                        ulpGetErrorMSG(&sErrorMgr),
                        acpCStrLen(ulpGetErrorMSG(&sErrorMgr), ACP_SINT32_MAX )
                      );

            /* 첫번째 접속 시도에 대한 에러 정보를 얻어온다. */
            ulpGetConnErrorMsg( sConnNode, sErrMsg );
            sIsFirstFail = ACP_TRUE;
            ACI_TEST_RAISE( acpSnprintf( sConnStr + sLen, MAX_CONN_STR + 1 - sLen, sConnNode->mConnOpt2 )
                            != ACP_RC_SUCCESS, ERR_CONNSTR_OVERFLOW );
            sSqlRes = SQLDriverConnect ( sConnNode->mHdbc,
                                         NULL,
                                         (SQLCHAR *)sConnStr,
                                         SQL_NTS,
                                         NULL,
                                         0,
                                         NULL,
                                         SQL_DRIVER_NOPROMPT );

            ACI_TEST_RAISE ( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE),
                             ERR_CLI_CONNECT );

            if ( sSqlRes == SQL_SUCCESS || sSqlRes == SQL_SUCCESS_WITH_INFO )
            {
                ulpSetDateFmtCore( sConnNode );
            }
        }
        else
        {
            ACI_RAISE ( ERR_CLI_CONNECT );
        }
    }

    /* ConnNode에서 관리되는 공유되는 statement를 할당한다.*/
    sSqlRes = SQLAllocStmt ( sConnNode->mHdbc, &(sConnNode->mHstmt) );

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) ||
                    (sSqlRes == SQL_INVALID_HANDLE), ERR_CLI_ALLOC_STMT );

    /*현재의 apre사용패턴(쓰레드당 connect -> embedded sql.. -> disconnect)으로 보면 
      베리어가 필요없지만 차후에 코드 변경시 필요하게 될 수가 있으므로
      만약을 위해 넣어 두기로 한다.*/
    ACP_MEM_RBARRIER();

    if ( sIsAllocNew  == ACP_TRUE )
    {
        sConnNode->mValid = ACP_TRUE; /*연결완료*/
    }

    /* Set nchar utf16*/
    if ( aSqlstmt -> sqlinfo == 1 )
    {
        gUlpLibOption.mIsNCharUTF16 = ACP_TRUE;
    }

    if ( sIsFirstFail == ACP_TRUE )
    {
        /* 첫번째 접속실패 결과 정보 저장*/
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               sErrMsg,
                               SQL_SUCCESS_WITH_INFO,
                               NULL );
    }
    else
    {
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               SQLMSG_SUCCESS,
                               SQL_SUCCESS,
                               SQLSTATE_SUCCESS );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_INVALID_USER );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Invalid_User_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR/* sqlcode */,
                               NULL );

    }
    ACI_EXCEPTION ( ERR_INVALID_PASSWD );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Invalid_Passwd_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR/* sqlcode */,
                               NULL );

    }
    ACI_EXCEPTION ( ERR_ALREADY_EXIST );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Aleady_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_ALREADY_CONNECTED/* sqlcode */,
                               ulpGetErrorSTATE(&sErrorMgr) );

    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_ENV );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE3 );

        sConnNode->mHenv = SQL_NULL_HENV;

    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_CONN );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE3 );

        SQLFreeEnv( sConnNode->mHenv );

        sConnNode->mHenv = SQL_NULL_HENV;
        sConnNode->mHdbc = SQL_NULL_HDBC;

    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );

        SQLFreeConnect( sConnNode->mHdbc );
        SQLFreeEnv( sConnNode->mHenv );

        sConnNode->mHenv = SQL_NULL_HENV;
        sConnNode->mHdbc = SQL_NULL_HDBC;
        sConnNode->mHstmt = SQL_NULL_HSTMT;

    }
    ACI_EXCEPTION ( ERR_CLI_CONNECT );
    {
        acp_char_t *sErrMsg2;
        ulpErrorMgr sErrorMgr;

        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             sSqlRes,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE1 );

        if( sIsFirstFail == ACP_TRUE )
        {
            ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Conn_Second_Trial_Failed );
            sErrMsg2 = ulpGetErrorMSG(&sErrorMgr);
            acpCStrCat ( sErrMsg,
                         MAX_ERRMSG_LEN - 1,
                         sErrMsg2,
                         MAX_ERRMSG_LEN - 1 - acpCStrLen( sErrMsg, MAX_ERRMSG_LEN )
                       );
            acpCStrCat ( sErrMsg,
                         MAX_ERRMSG_LEN - 1,
                         aSqlstmt->sqlcaerr->sqlerrm.sqlerrmc,
                         MAX_ERRMSG_LEN - 1 - acpCStrLen( sErrMsg, MAX_ERRMSG_LEN )
                       );
            acpCStrCpy ( aSqlstmt->sqlcaerr->sqlerrm.sqlerrmc,
                         MAX_ERRMSG_LEN,
                         sErrMsg,
                         acpCStrLen(sErrMsg, ACP_SINT32_MAX)
                       );
            aSqlstmt->sqlcaerr->sqlerrm.sqlerrml = acpCStrLen( sErrMsg, MAX_ERRMSG_LEN );
        }

        SQLFreeConnect( sConnNode->mHdbc );
        SQLFreeEnv( sConnNode->mHenv );

        sConnNode->mHenv = SQL_NULL_HENV;
        sConnNode->mHdbc = SQL_NULL_HDBC;

    }
    ACI_EXCEPTION ( ERR_CONNSTR_OVERFLOW );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Exceed_Max_Connname_Error,
                         sConnStr );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );

        SQLFreeConnect( sConnNode->mHdbc );
        SQLFreeEnv( sConnNode->mHenv );

        sConnNode->mHenv = SQL_NULL_HENV;
        sConnNode->mHdbc = SQL_NULL_HDBC;

    }
    ACI_EXCEPTION ( ERR_CONN_NAME_OVERFLOW );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Exceed_Max_Connname_Error,
                         aConnName );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_CONNAME_OVERFLOW/* sqlcode */,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION (ERR_LATCH_WRITE);
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Latch_Write_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION (ERR_LATCH_RELEASE);
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Latch_Release_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION_END;

    if ( sIsAllocNew == ACP_TRUE )
    {
        ulpLibConDelConn( aConnName );
    }

    if ( sIsLatched == ACP_TRUE )
    {
        /* release write lock */
        ACE_ASSERT( acpThrRwlockUnlock( &gUlpConnMgr.mConnLatch )
                    == ACP_RC_SUCCESS );
    }

     return ACI_FAILURE;
}

ACI_RC ulpDisconnect( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    DB에 연결을 끊는다.
 *
 *    처리구문> EXEC SQL [ AT <conn_name | :conn_name> ] DISCONNECT;
 *
 * Implementation :
 *
 ***********************************************************************/
    acp_bool_t      sIsDefaultConn;
    acp_bool_t      sIsLatched;
    ulpLibConnNode *sConnNode;
    SQLRETURN       sSqlRes;
    ACP_UNUSED(reserved);

    sIsDefaultConn = ACP_FALSE;
    sIsLatched     = ACP_FALSE;

    // BUG-39202 [ux-apre] The concurrency problem is occurred when the multi-thread use same connection name.
    if ( gUlpLibOption.mIsMultiThread == ACP_TRUE )
    {
        /* get write lock */
        ACI_TEST_RAISE( acpThrRwlockLockWrite( &gUlpConnMgr.mConnLatch )
                        != ACP_RC_SUCCESS, ERR_LATCH_WRITE );

        sIsLatched = ACP_TRUE;
    }

    if ( aConnName != NULL )
    {
        /*aConnName으로 Connection node를 찾는다*/
        sConnNode = ulpLibConLookupConn( aConnName );
        /*disconnect할 해당 connection이 없으면 에러*/
        ACI_TEST_RAISE( sConnNode == NULL, ERR_NO_CONN );
    }
    else
    {
        sConnNode = ulpLibConGetDefaultConn();
        /* BUG-26359 valgrind bug */
        ACI_TEST_RAISE( sConnNode->mHenv == SQL_NULL_HENV, ERR_NO_CONN );
        sIsDefaultConn = ACP_TRUE;
    }

    ACI_TEST_RAISE ( sConnNode->mIsXa == ACP_TRUE, ERR_XA_DISCONN  );

    /* stmt handle을 해제한다*/
    SQLFreeHandle ( SQL_HANDLE_STMT, sConnNode->mHstmt );

    sSqlRes = SQLDisconnect( sConnNode->mHdbc );

    ulpSetErrorInfo4CLI( sConnNode,
                         SQL_NULL_HSTMT,
                         sSqlRes,
                         aSqlstmt->sqlcaerr,
                         aSqlstmt->sqlcodeerr,
                         aSqlstmt->sqlstateerr,
                         ERR_TYPE2 );

    if( sSqlRes == SQL_ERROR )
    {
        if( *(aSqlstmt->sqlcodeerr) == SQL_INVALID_HANDLE
            && acpCStrCmp( *(aSqlstmt->sqlstateerr), "08003",
                           MAX_ERRSTATE_LEN-1 ) == 0 )
        {
            /* Connection does not exist*/
            /* : The connection specified in the argument ConnectionHandle was not open.*/
            /* 이 경우 성공으로 간주한다*/
            /* do nothing */
        }
        else
        {
            ACI_RAISE( ERR_CLI_DISCONNECT );
        }
    }

    /* dbc handle을 해제한다*/
    sSqlRes = SQLFreeHandle ( SQL_HANDLE_DBC, sConnNode->mHdbc );

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR || sSqlRes == SQL_INVALID_HANDLE),
                     ERR_CLI_FREE_HANDLE );

    /* env handle을 해제한다*/
    sSqlRes = SQLFreeHandle ( SQL_HANDLE_ENV, sConnNode->mHenv );

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR || sSqlRes == SQL_INVALID_HANDLE),
                     ERR_CLI_FREE_HANDLE );

    if ( sIsDefaultConn == ACP_TRUE )
    {
        /* default connection node 초기화*/
        (void) ulpLibConInitDefaultConn();
    }
    else
    {
        /* connection hash table에서 해당 sConnNode를 제거한다*/
        ulpLibConDelConn( aConnName );
    }

    // BUG-39202 [ux-apre] The concurrency problem is occurred when the multi-thread use same connection name.
    if ( sIsLatched == ACP_TRUE )
    {
        /* release write lock */
        ACI_TEST_RAISE( acpThrRwlockUnlock( &gUlpConnMgr.mConnLatch )
                        != ACP_RC_SUCCESS, ERR_LATCH_RELEASE );

        sIsLatched = ACP_FALSE;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );

    }
    ACI_EXCEPTION ( ERR_XA_DISCONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulERR_ABORT_Disconn_Xa_Error );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );

    }
    ACI_EXCEPTION ( ERR_CLI_FREE_HANDLE );
    {
      ulpSetErrorInfo4CLI( sConnNode,
                           SQL_NULL_HSTMT,
                           SQL_ERROR,
                           aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           ERR_TYPE3 );

        /* ConnNode의 Statement/Cursor hash table을 해제한다.;*/
        ulpLibStDelAllStmtCur( &(sConnNode->mStmtHashT),
                               &(sConnNode->mCursorHashT) );
        ulpLibStDelAllUnnamedStmt( &(sConnNode->mUnnamedStmtCacheList) );
    }
    ACI_EXCEPTION ( ERR_CLI_DISCONNECT );
    {
        /* dbc handle을 해제한다*/
        SQLFreeHandle ( SQL_HANDLE_DBC, sConnNode->mHdbc );

        /* env handle을 해제한다*/
        SQLFreeHandle ( SQL_HANDLE_ENV, sConnNode->mHenv );

        /* BUG-42166
           When the disconnection fail is occurred in APRE, the related connection structure should be removed.
           (related to INC-30510) */
        if ( sIsDefaultConn == ACP_TRUE )
        {
            /* default connection node 초기화*/
            (void) ulpLibConInitDefaultConn();
        }
        else
        {
            /* ConnNode의 Statement/Cursor hash table을 해제한다.;*/
            ulpLibStDelAllStmtCur( &(sConnNode->mStmtHashT),
                                   &(sConnNode->mCursorHashT) );
            ulpLibStDelAllUnnamedStmt( &(sConnNode->mUnnamedStmtCacheList) );

            /* connection hash table에서 해당 sConnNode를 제거한다*/
            (void) ulpLibConDelConn( aConnName );
        }
    }
    ACI_EXCEPTION (ERR_LATCH_WRITE);
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Latch_Write_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION (ERR_LATCH_RELEASE);
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Latch_Release_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &mErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION_END;

    // BUG-39202 [ux-apre] The concurrency problem is occurred when the multi-thread use same connection name.
    if ( sIsLatched == ACP_TRUE )
    {
        /* release write lock */
        ACE_ASSERT( acpThrRwlockUnlock( &gUlpConnMgr.mConnLatch )
                    == ACP_RC_SUCCESS );
    }

    return ACI_FAILURE;
}

ACI_RC ulpRunDirectQuery ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    EXEC SQL 다음에 SQL 구문이 오는 경우 (unnamed stmt.) 에 대해 처리한다.
 *
 *    처리구문> EXEC SQL [ AT <conn_name | :conn_name> ] <sql_query_str>;
 *
 * Implementation :
 *
 ***********************************************************************/
    ACP_UNUSED(reserved);

    ulpSetColRowSizeCore( aSqlstmt );

    /* FOR절 처리*/
    ACI_TEST( ulpAdjustArraySize(aSqlstmt) == ACI_FAILURE );

    if ( ( ulpGetStmtOption( aSqlstmt, ULP_OPT_STMT_CACHE ) == ACP_TRUE ) &&
           (  (aSqlstmt->stmttype == S_DirectDML)     /*DML*/
           || (aSqlstmt->stmttype == S_DirectSEL)     /*SELECT*/
           || (aSqlstmt->stmttype == S_DirectPSM)     /*BUG-35519 PSM*/
           || (aSqlstmt->stmttype == S_DirectANONPSM) /*BUG-46824 anonymous block */) )
    {
        /* BUG-45779 stmt cache를 사용할 경우 */
        ACI_TEST( ulpRunDirectQueryCache(aConnName, aSqlstmt) != ACI_SUCCESS );
    }
    else
    {
        /* BUG-45779 stmt cache를 사용하지 않을 경우 */
        ACI_TEST( ulpRunDirectQueryUnCache(aConnName, aSqlstmt) != ACI_SUCCESS );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* direct Query에서 stmtcache={on|off} 옵션이 on 일경우 동작한다.
 * 현재 default on 으로 되어 있음.*/
ACI_RC ulpRunDirectQueryCache(acp_char_t *aConnName, ulpSqlstmt *aSqlstmt)
{
    acp_sint32_t     i;
    ulpLibConnNode  *sConnNode;
    ulpLibStmtNode  *sStmtNode = NULL;
    SQLRETURN        sSqlRes;
    SQLHSTMT        *sHstmtPtr;
    acp_bool_t       sIsSharedSTMT;
    /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
    /* SQLAllocStmt 여부에대한 정보저장.*/
    acp_bool_t       sIsAllocNode;
    acp_bool_t       sIsAllocStmt;
    acp_bool_t       sIsLatched;

    acp_list_node_t     *sPSMMetaIterator;
    acp_list_node_t     *sPSMMetaNodeNext;
    ulpPSMArrDataInfo   *sUlpPSMArrDataInfo;

    sIsSharedSTMT           = ACP_FALSE;
    sIsAllocNode            = ACP_FALSE;
    sIsAllocStmt            = ACP_FALSE;
    sIsLatched              = ACP_FALSE;

    ULPGETCONNECTION(aConnName, sConnNode);

    /** unnamed stmt. cache를 하겠다. **/
    /* 1. unnamed cache list에 구문이 이미 존재하는지 검색한다.*/
    sStmtNode = ulpLibStLookupUnnamedStmt( &(sConnNode->mUnnamedStmtCacheList) ,aSqlstmt->stmt );
    if ( sStmtNode == NULL  )
    {
        /* 2.1 존재하지 않으면 stmt node를 새로 생성/설정 해준다.*/
        sStmtNode = ulpLibStNewNode(aSqlstmt, NULL );
        ACI_TEST(sStmtNode == NULL);
        sIsAllocNode = ACP_TRUE;

        /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
        sHstmtPtr = &( sStmtNode -> mHstmt );

        /* 새로 생성할 경우 AllocStmt, Prepare 수행.*/
        sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, sHstmtPtr );

        /* Do we have to check whether AllocStmt error is caused by
        * "limit on the number of handles exceeded" ??? I dont think so. */

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE) , ERR_CLI_ALLOC_STMT);

        /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
        sIsAllocStmt = ACP_TRUE;

        /* ulpPrepareCore가 실패하면 이미 stmt는 drop된 상태다.*/
        ACI_TEST_RAISE( ulpPrepareCore  ( sConnNode,
                                          sStmtNode,
                                          aSqlstmt -> stmt,
                                          aSqlstmt -> sqlcaerr,
                                          aSqlstmt -> sqlcodeerr,
                                          aSqlstmt -> sqlstateerr )
                        == ACI_FAILURE, ERR_PREPARE_CORE );

        /* BUG-45779 prepare뒤에 psm array type진행 여부를 체크한다. */
        if( aSqlstmt->stmttype == S_DirectPSM )
        {
            ACI_TEST( ulpCheckPSMArray( sConnNode,
                                        aSqlstmt,
                                        &sStmtNode->mUlpPSMArrInfo.mIsPSMArray )
                              == ACI_FAILURE );
        }

        /* binding이나 setstmtattr 처리함.*/
        if ( aSqlstmt -> numofhostvar > 0 )
        {
            ACI_TEST( ulpBindHostVarCore( sConnNode,
                                          sStmtNode,
                                          sHstmtPtr,
                                          aSqlstmt,
                                          aSqlstmt -> sqlcaerr,
                                          aSqlstmt -> sqlcodeerr,
                                          aSqlstmt -> sqlstateerr)
                      == ACI_FAILURE );
            if( aSqlstmt->stmttype != S_DirectSEL )
            {
                ACI_TEST( ulpSetStmtAttrParamCore( sConnNode,
                                                   sStmtNode,
                                                   sHstmtPtr,
                                                   aSqlstmt,
                                                   aSqlstmt -> sqlcaerr,
                                                   aSqlstmt -> sqlcodeerr,
                                                   aSqlstmt -> sqlstateerr )
                          == ACI_FAILURE );
            }
        }

        /* 3. SQLExecute 수행.*/
        ACI_TEST( ulpExecuteCore( sConnNode,
                                  sStmtNode,
                                  aSqlstmt,
                                  sHstmtPtr )
                  == ACI_FAILURE );

        /* 4. SELECT일 경우 fetch 수행함.*/
        if( aSqlstmt->stmttype == S_DirectSEL/*SELECT*/  )
        {
            ACI_TEST( ulpSetStmtAttrRowCore( sConnNode,
                                             sStmtNode,
                                             sHstmtPtr,
                                             aSqlstmt,
                                             aSqlstmt -> sqlcaerr,
                                             aSqlstmt -> sqlcodeerr,
                                             aSqlstmt -> sqlstateerr )
                       == ACI_FAILURE );

            /* BUG-28588 : 반복해서 null fetch 시 에러 메세지가 달라지는 문제. */
            ACI_TEST_RAISE( ulpFetchCore( sConnNode,
                                          sStmtNode,
                                          sHstmtPtr,
                                          aSqlstmt,
                                          aSqlstmt -> sqlcaerr,
                                          aSqlstmt -> sqlcodeerr,
                                          aSqlstmt -> sqlstateerr ) == ACI_FAILURE,
                            ERR_FETCH_CORE );

            sSqlRes = SQLFreeStmt( *sHstmtPtr, SQL_CLOSE );

            ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE), ERR_CLI_FREE_STMT );

            for( i = 0 ; aSqlstmt->numofhostvar > i ; i++ )
            {
                if( ( (aSqlstmt->hostvalue[i].mType == H_VARCHAR) ||
                      (aSqlstmt->hostvalue[i].mType == H_NVARCHAR)) &&
                      (aSqlstmt->hostvalue[i].mVcInd != NULL) )
                {
                    *(aSqlstmt->hostvalue[i].mVcInd) = *(aSqlstmt->hostvalue[i].mHostInd);
                }
            }
        }
        else if( aSqlstmt->stmttype == S_DirectPSM )
        {
            /* BUG-45779 out paramter인 경우에만 Server로 부터 받은 데이터를 복사해준다. */
            ACP_LIST_ITERATE_SAFE(&sStmtNode->mUlpPSMArrInfo.mPSMArrDataInfoList, sPSMMetaIterator, sPSMMetaNodeNext)
            {
                sUlpPSMArrDataInfo = (ulpPSMArrDataInfo *)sPSMMetaIterator->mObj;

                if( sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT_4PSM ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_INOUT )
                {
                    ACI_TEST( ulpPSMArrayRead(aSqlstmt, sUlpPSMArrDataInfo->mUlpHostVar, sUlpPSMArrDataInfo->mData) != ACI_SUCCESS );
                }
            }

            /* BUG-45779 out parameter인 경우에 데이터를 전부 copy한뒤에 null이 포함되어 있는지 확인한다. */
            ACP_LIST_ITERATE_SAFE(&sStmtNode->mUlpPSMArrInfo.mPSMArrDataInfoList, sPSMMetaIterator, sPSMMetaNodeNext)
            {
                sUlpPSMArrDataInfo = (ulpPSMArrDataInfo *)sPSMMetaIterator->mObj;

                if( sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT_4PSM ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_INOUT )
                {
                    ACI_TEST( ulpPSMArrayHasNullCheck(aSqlstmt, sUlpPSMArrDataInfo->mData) != ACI_SUCCESS );
                }
            }
        }

        /*5. add unnamed stmt list*/
        ulpLibStAddUnnamedStmt( &(sConnNode->mUnnamedStmtCacheList),
                                sStmtNode );
    }
    else
    {
        sHstmtPtr = &(sStmtNode->mHstmt);

        /* BUG-31467 : APRE should consider the thread safety of statement */
        /* unnamed statement 끼리의 thread safety 고려 */
        /* get write lock */
        ACI_TEST_RAISE ( acpThrRwlockLockWrite( &(sStmtNode->mLatch) ) != ACP_RC_SUCCESS, ERR_LATCH_WRITE );
        sIsLatched = ACP_TRUE;

        /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
        if ( sStmtNode->mNeedReprepare == ACP_TRUE ) /* BUG-47239  mFailoveredJustnow 삭제 */
        {
            // do prepare
            ACI_TEST_RAISE( ulpPrepareCore  ( sConnNode,
                            sStmtNode,
                            aSqlstmt -> stmt,
                            aSqlstmt -> sqlcaerr,
                            aSqlstmt -> sqlcodeerr,
                            aSqlstmt -> sqlstateerr )
                    == ACI_FAILURE, ERR_PREPARE_CORE );

            sStmtNode->mNeedReprepare = ACP_FALSE;

            /* BUG-45779 prepare뒤에 psm array type진행 여부를 체크한다. */
            if( aSqlstmt->stmttype == S_DirectPSM )
            {
                ACI_TEST( ulpCheckPSMArray( sConnNode,
                                            aSqlstmt,
                                            &sStmtNode->mUlpPSMArrInfo.mIsPSMArray )
                                  == ACI_FAILURE );
            }
        }

        /* 2.2 존재하면 재활용, binding이나 setstmtattr이 필요할 경우 처리함.*/
        if( ulpCheckNeedReBindAllCore( sStmtNode, aSqlstmt ) == ACP_TRUE )
        {

            ACI_TEST( ulpBindHostVarCore( sConnNode,
                                          sStmtNode,
                                          sHstmtPtr,
                                          aSqlstmt,
                                          aSqlstmt -> sqlcaerr,
                                          aSqlstmt -> sqlcodeerr,
                                          aSqlstmt -> sqlstateerr )
                      == ACI_FAILURE );

            if( (aSqlstmt->stmttype != S_DirectSEL) &&
                (ulpCheckNeedReSetStmtAttrCore( sStmtNode, aSqlstmt ) == ACP_TRUE) )
            {

                ACI_TEST( ulpSetStmtAttrParamCore( sConnNode,
                                                   sStmtNode,
                                                   sHstmtPtr,
                                                   aSqlstmt,
                                                   aSqlstmt -> sqlcaerr,
                                                   aSqlstmt -> sqlcodeerr,
                                                   aSqlstmt -> sqlstateerr )
                          == ACI_FAILURE );

            }
        }
        else
        {
            /* BUG-29825 : The execution mode of a statement has been changed
               from non-array to array when record count is changed. */
            /* Select 구문에서 SQL_ATTR_PARAM 속성을 변경할일은 없다.*/
            if( aSqlstmt->stmttype != S_DirectSEL )
            {
                /* FOR절, ATOMIC절 처리 고려.*/
                /* array size가 변경되거나 atomic 속성이 변경되었을경우 SetStmtAttr 재호출.*/
                if ( (aSqlstmt -> isarr   != sStmtNode -> mBindInfo.mIsArray)  ||
                     (aSqlstmt -> arrsize != sStmtNode -> mBindInfo.mArraySize)||
                     (aSqlstmt -> isatomic!= (acp_sint32_t)sStmtNode->mBindInfo.mIsAtomic)
                   )
                {
                    ACI_TEST( ulpSetStmtAttrParamCore( sConnNode,
                                                       sStmtNode,
                                                       sHstmtPtr,
                                                       aSqlstmt,
                                                       aSqlstmt -> sqlcaerr,
                                                       aSqlstmt -> sqlcodeerr,
                                                       aSqlstmt -> sqlstateerr )
                               == ACI_FAILURE );
                }
            }
        }

        /* BUG-45779 User Data Copy & Endian */

        /* 3. SQLExecute 수행.*/
        ACI_TEST( ulpExecuteCore( sConnNode,
                                  sStmtNode,
                                  aSqlstmt,
                                  sHstmtPtr )
                  == ACI_FAILURE );

        /* 4. SELECT일 경우 fetch 수행함.*/
        if( aSqlstmt->stmttype == S_DirectSEL/*SELECT*/  )
        {
            if (ulpCheckNeedReSetStmtAttrCore( sStmtNode, aSqlstmt ) == ACP_TRUE)
            {

                ACI_TEST( ulpSetStmtAttrRowCore( sConnNode,
                                             sStmtNode,
                                             sHstmtPtr,
                                             aSqlstmt,
                                             aSqlstmt -> sqlcaerr,
                                             aSqlstmt -> sqlcodeerr,
                                             aSqlstmt -> sqlstateerr )
                           == ACI_FAILURE );
            }

            /* BUG-28588 : 반복해서 null fetch 시 에러 메세지가 달라지는 문제. */
            ACI_TEST_RAISE( ulpFetchCore( sConnNode,
                                          sStmtNode,
                                          sHstmtPtr,
                                          aSqlstmt,
                                          aSqlstmt -> sqlcaerr,
                                          aSqlstmt -> sqlcodeerr,
                                          aSqlstmt -> sqlstateerr ) == ACI_FAILURE,
                            ERR_FETCH_CORE );

            sSqlRes = SQLFreeStmt( *sHstmtPtr, SQL_CLOSE );

            ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE), ERR_CLI_FREE_STMT );

            for( i = 0 ; aSqlstmt->numofhostvar > i ; i++ )
            {
                if( ((aSqlstmt->hostvalue[i].mType == H_VARCHAR) ||
                     (aSqlstmt->hostvalue[i].mType == H_NVARCHAR)) &&
                     (aSqlstmt->hostvalue[i].mVcInd != NULL) )
                {
                    *(aSqlstmt->hostvalue[i].mVcInd) = *(aSqlstmt->hostvalue[i].mHostInd);
                }
            }
        }
        else if( aSqlstmt->stmttype == S_DirectPSM )
        {
            /* BUG-45779 out paramter인 경우에만 Server로 부터 받은 데이터를 복사해준다. */
            ACP_LIST_ITERATE_SAFE(&sStmtNode->mUlpPSMArrInfo.mPSMArrDataInfoList, sPSMMetaIterator, sPSMMetaNodeNext)
            {
                sUlpPSMArrDataInfo = (ulpPSMArrDataInfo *)sPSMMetaIterator->mObj;

                if( sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT_4PSM ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_INOUT )
                {
                    ACI_TEST( ulpPSMArrayRead(aSqlstmt, sUlpPSMArrDataInfo->mUlpHostVar, sUlpPSMArrDataInfo->mData) != ACI_SUCCESS );
                }
            }

            /* BUG-45779 out parameter인 경우에 데이터를 전부 copy한뒤에 null이 포함되어 있는지 확인한다. */
            ACP_LIST_ITERATE_SAFE(&sStmtNode->mUlpPSMArrInfo.mPSMArrDataInfoList, sPSMMetaIterator, sPSMMetaNodeNext)
            {
                sUlpPSMArrDataInfo = (ulpPSMArrDataInfo *)sPSMMetaIterator->mObj;

                if( sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_OUT_4PSM ||
                    sUlpPSMArrDataInfo->mUlpHostVar->mInOut == H_INOUT )
                {
                    ACI_TEST( ulpPSMArrayHasNullCheck(aSqlstmt, sUlpPSMArrDataInfo->mData) != ACI_SUCCESS );
                }
            }
        }

        /* BUG-31467 : APRE should consider the thread safety of statement */
        /* release write lock */
        if( sIsLatched == ACP_TRUE )
        {
            ACI_TEST_RAISE ( acpThrRwlockUnlock( &(sStmtNode->mLatch) )
                             != ACP_RC_SUCCESS, ERR_LATCH_RELEASE );
            sIsLatched = ACP_FALSE;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );


        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );

    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION ( ERR_CLI_FREE_STMT );
    {
      ulpSetErrorInfo4CLI( sConnNode,
                           *sHstmtPtr,
                           SQL_ERROR,
                           aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           ERR_TYPE2 );
    }
    /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
    ACI_EXCEPTION ( ERR_PREPARE_CORE );
    {
        /* BUG-29903 : prepare 실패를 반복하면 메모리증가. */
        /* 재활용이아닌 새로 할당받은 statment handle을 사용하여*/
        /* prepare하다가 실패하면 SQL_DROP하여 해제해주어야함.*/
    }
    /* BUG-28588 : 반복해서 null fetch 시 에러 메세지가 달라지는 문제. */
    ACI_EXCEPTION ( ERR_FETCH_CORE );
    {
        SQLFreeStmt( *sHstmtPtr, SQL_CLOSE );
    }
    /* BUG-31467 : APRE should consider the thread safety of statement */
    ACI_EXCEPTION (ERR_LATCH_WRITE);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Latch_Write_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &sErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION (ERR_LATCH_RELEASE);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Latch_Release_Error,
                         __LINE__,
                         __FILE__ );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &sErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION_END;

    /* BUG-31467 : APRE should consider the thread safety of statement */
    if( sIsLatched == ACP_TRUE )
    {
        /* release write lock */
        ACI_TEST_RAISE ( acpThrRwlockUnlock( &(sStmtNode->mLatch) )
                         != ACP_RC_SUCCESS, ERR_LATCH_RELEASE );
        sIsLatched = ACP_FALSE;
    }

    /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
    if ( sIsAllocStmt == ACP_TRUE || sIsSharedSTMT == ACP_TRUE )
    {
        SQLFreeStmt( *sHstmtPtr, SQL_DROP );
        if( sIsSharedSTMT == ACP_TRUE )
        {   /*connection node의 공유 statement를 사용하는 경우라면 stmt 핸들을 DROP후 다시 할당받는다.*/
            SQLAllocStmt( sConnNode -> mHdbc, sHstmtPtr );
        }
    }

    if ( sIsAllocNode == ACP_TRUE )
    {
        /* BUG-30789: Memory usage per prepared statement is too BIG. (>300k) */
        if ( sStmtNode -> mQueryStr != NULL )
        {
            acpMemFree( sStmtNode -> mQueryStr );
        }
        if ( sStmtNode -> mInHostVarPtr != NULL )
        {
            acpMemFree( sStmtNode -> mInHostVarPtr );
        }
        if ( sStmtNode -> mOutHostVarPtr != NULL )
        {
            acpMemFree( sStmtNode -> mOutHostVarPtr );
        }
        if ( sStmtNode -> mExtraHostVarPtr != NULL )
        {
            acpMemFree( sStmtNode -> mExtraHostVarPtr );
        }
        acpMemFree( sStmtNode );
    }

    return ACI_FAILURE;
}


/** unnamed stmt. cache를 하지 않겠다. **/
/* direct Query에서 stmtcache={on|off} 옵션이 off 일경우 동작한다.
 * 현재 default on(하드코딩) 이므로 특별한 옵션 변경이 없다면 해당 로직은 돌아가지 않음.*/
ACI_RC ulpRunDirectQueryUnCache(acp_char_t *aConnName, ulpSqlstmt *aSqlstmt)
{
    acp_sint32_t     i;
    ulpLibConnNode  *sConnNode;
    SQLRETURN        sSqlRes;
    SQLHSTMT        *sHstmtPtr;
    SQLHSTMT         sHstmt;
    acp_bool_t       sIsSharedSTMT;
    /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
    /* SQLAllocStmt 여부에대한 정보저장.*/
    acp_bool_t       sIsAllocStmt;

    sIsSharedSTMT           = ACP_FALSE;
    sIsAllocStmt            = ACP_FALSE;
    sHstmt                  = SQL_NULL_HSTMT;

    ULPGETCONNECTION(aConnName,sConnNode);

    if( gUlpLibOption.mIsMultiThread == ACP_TRUE )
    {
        /* 1.1 mutithread 일경우 stmt 핸들 새로 할당.*/
        sHstmtPtr = &( sHstmt );
        sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, sHstmtPtr );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_ALLOC_STMT);

        /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
        sIsAllocStmt = ACP_TRUE;
    }
    else
    {
        /* 1.2 mutithread 가 아닐경우 연결노드의 stmt 핸들 사용.*/
        sHstmtPtr = &( sConnNode -> mHstmt );
        sIsSharedSTMT= ACP_TRUE;
    }

    /* 2. host변수가 존재할 경우, binding & setstmtattr 처리함.*/
    if ( aSqlstmt -> numofhostvar > 0 )
    {
        ACI_TEST( ulpBindHostVarCore( sConnNode,
                                      NULL,
                                      sHstmtPtr,
                                      aSqlstmt,
                                      aSqlstmt -> sqlcaerr,
                                      aSqlstmt -> sqlcodeerr,
                                      aSqlstmt -> sqlstateerr )
                == ACI_FAILURE );

        /* BUG-29825 : The execution mode of a statement has been changed
           from non-array to array when record count is changed. */
        if( aSqlstmt->stmttype != S_DirectSEL )
        {
            ACI_TEST( ulpSetStmtAttrParamCore( sConnNode,
                                               NULL,
                                               sHstmtPtr,
                                               aSqlstmt,
                                               aSqlstmt -> sqlcaerr,
                                               aSqlstmt -> sqlcodeerr,
                                               aSqlstmt -> sqlstateerr )
                    == ACI_FAILURE );
        }
    }

    /* 4. SQLExecDirect 수행.*/
    ACI_TEST( ulpExecDirectCore( sConnNode,
                                 NULL,
                                 aSqlstmt,
                                 sHstmtPtr,
                                 aSqlstmt -> stmt )
              == ACI_FAILURE );

    /* 5. SELECT일 경우 fetch 수행함.*/
    if( aSqlstmt->stmttype == S_DirectSEL/*SELECT*/  )
    {
        ACI_TEST( ulpSetStmtAttrRowCore( sConnNode,
                                         NULL,
                                         sHstmtPtr,
                                         aSqlstmt,
                                         aSqlstmt -> sqlcaerr,
                                         aSqlstmt -> sqlcodeerr,
                                         aSqlstmt -> sqlstateerr )
                  == ACI_FAILURE );

        ACI_TEST_RAISE( ulpFetchCore( sConnNode,
                                      NULL,
                                      sHstmtPtr,
                                      aSqlstmt,
                                      aSqlstmt -> sqlcaerr,
                                      aSqlstmt -> sqlcodeerr,
                                      aSqlstmt -> sqlstateerr ) == ACI_FAILURE,
                        ERR_FETCH_CORE);

        sSqlRes = SQLFreeStmt( *sHstmtPtr, SQL_CLOSE );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_FREE_STMT );

        /* varchar 를 호스트 변수로 사용하며 사용자가 indicator를 명시 해줬을경우,
           fetch 수행후 indicator결과 값을 varchar.len 에 복사해 줘야함. */
        for( i = 0 ; aSqlstmt->numofhostvar > i ; i++ )
        {
            if( ((aSqlstmt->hostvalue[i].mType == H_VARCHAR) ||
                 (aSqlstmt->hostvalue[i].mType == H_NVARCHAR)) &&
                 (aSqlstmt->hostvalue[i].mVcInd != NULL) )
            {
                *(aSqlstmt->hostvalue[i].mVcInd) = *(aSqlstmt->hostvalue[i].mHostInd);
            }
        }
    }

    /* 6. stmt 핸들 해제 */
    sSqlRes = SQLFreeStmt( *sHstmtPtr, SQL_DROP );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_FREE_STMT );
    /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
    sIsAllocStmt = ACP_FALSE;

    if( sIsSharedSTMT == ACP_TRUE )
    {   /*connection node의 공유 statement를 사용하는 경우라면 stmt 핸들을 DROP후 다시 할당받는다.*/
        sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, sHstmtPtr );
        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_ALLOC_STMT);
    }

    /* 7. ROLLBACK인 경우 추가 처리.*/
    if( (aSqlstmt->stmttype == S_DirectRB) &&
        (aSqlstmt->sqlinfo  == STMT_ROLLBACK_RELEASE /* ROLLBACK REALEASE 일경우 */) )
    {
        ulpDisconnect( aConnName, aSqlstmt, NULL );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );


        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );

    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION ( ERR_CLI_FREE_STMT );
    {
      ulpSetErrorInfo4CLI( sConnNode,
                           *sHstmtPtr,
                           SQL_ERROR,
                           aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           ERR_TYPE2 );
    }
    /* BUG-28588 : 반복해서 null fetch 시 에러 메세지가 달라지는 문제. */
    ACI_EXCEPTION ( ERR_FETCH_CORE );
    {
        SQLFreeStmt( *sHstmtPtr, SQL_CLOSE );
    }
    ACI_EXCEPTION_END;

    /* BUG-28392 : direct 내장sql dml구문 반복 실패시 메모리 누수발생. */
    if ( sIsAllocStmt == ACP_TRUE || sIsSharedSTMT == ACP_TRUE )
    {
        SQLFreeStmt( *sHstmtPtr, SQL_DROP );
        if( sIsSharedSTMT == ACP_TRUE )
        {   /*connection node의 공유 statement를 사용하는 경우라면 stmt 핸들을 DROP후 다시 할당받는다.*/
            SQLAllocStmt( sConnNode -> mHdbc, sHstmtPtr );
        }
    }

    return ACI_FAILURE;
}

ACI_RC ulpExecuteImmediate ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    Dynamic SQL method1 에 대한 처리를 담당함.
 *
 *    처리구문> EXEC SQL EXECUTE IMMEDIATE <:host_var | string_literal>;
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    SQLRETURN       sSqlRes;
    SQLHSTMT       *sHstmtPtr;
    SQLHSTMT        sHstmt;
    acp_bool_t      sIsMulti;
    ACP_UNUSED(reserved);

    sIsMulti = ACP_FALSE;
    sHstmt   = SQL_NULL_HSTMT;

    ULPGETCONNECTION(aConnName,sConnNode);

    if( gUlpLibOption.mIsMultiThread == ACP_TRUE )
    {
        /* mutithread 일경우 stmt 핸들 새로 할당.*/
        sHstmtPtr = &( sHstmt );
        sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, sHstmtPtr );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_ALLOC_STMT);
        sIsMulti = ACP_TRUE;
    }
    else
    {
        /* mutithread 가 아닐경우 연결노드의 stmt 핸들 사용.*/
        sHstmtPtr = &( sConnNode -> mHstmt );
    }

    /* SQLExecDirect 수행.*/
    /* BUG-37815 temp stmt have to free when error occurred */
    ACI_TEST_RAISE( ulpExecDirectCore( sConnNode,
                                       NULL,
                                       aSqlstmt,
                                       sHstmtPtr,
                                       aSqlstmt->stmt ) == ACI_FAILURE,
                    ERR_EXEC_DIRECT_CORE )

   /* stmt 핸들 해제 & stmt node 해제.*/
    if( sIsMulti == ACP_TRUE )
    {
        sSqlRes = SQLFreeStmt( *sHstmtPtr, SQL_DROP );
        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_FREE_STMT );
    }
    else
    {
        ACI_TEST( ulpCloseStmtCore( sConnNode,
                                    NULL,
                                    sHstmtPtr,
                                    aSqlstmt -> sqlcaerr,
                                    aSqlstmt -> sqlcodeerr,
                                    aSqlstmt -> sqlstateerr )
                  == ACI_FAILURE );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );

    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    // BUG-37815
    ACI_EXCEPTION( ERR_EXEC_DIRECT_CORE );
    {
        if ( sIsMulti == ACP_TRUE )
        {
            ( void )SQLFreeStmt( *sHstmtPtr, SQL_DROP );
        }
    }
    ACI_EXCEPTION ( ERR_CLI_FREE_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             *sHstmtPtr,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_bool_t ulpIsNeedReprepare( ulpLibStmtNode *aStmtNode,
                               ulpSqlstmt     *aSqlstmt )
{
    acp_uint32_t    sCursorAttr;
    acp_bool_t      sNeedReprepare = ACP_TRUE;
    SQLRETURN       sRC;

    /* 쿼리문이 없을때는 prepare(또는 re-prepare)가 필요 없다. */
    ACI_TEST_RAISE( (aStmtNode->mQueryStr == NULL) && (aSqlstmt->stmt == NULL),
                    NoNeedPrepareCont );

    ACI_TEST_RAISE( (aStmtNode->mQueryStr == NULL) ||
                    ( (aSqlstmt->stmt != NULL) &&
                      (acpCStrCmp(aStmtNode->mQueryStr, aSqlstmt->stmt,
                                  aStmtNode->mQueryStrLen) != 0) ),
                    NeedReprepareCont );


    sRC = SQLGetStmtAttr( aStmtNode->mHstmt, SQL_ATTR_CURSOR_SCROLLABLE,
                          (SQLPOINTER) &sCursorAttr, ACI_SIZEOF(sCursorAttr),
                          NULL );
    ACE_ASSERT( SQL_SUCCEEDED(sRC) );
    ACI_TEST_RAISE( sCursorAttr != aSqlstmt->cursorscrollable,
                    NeedReprepareCont );

    sRC = SQLGetStmtAttr( aStmtNode->mHstmt, SQL_ATTR_CURSOR_SENSITIVITY,
                          (SQLPOINTER) &sCursorAttr, ACI_SIZEOF(sCursorAttr),
                          NULL );
    ACE_ASSERT( SQL_SUCCEEDED(sRC) );
    ACI_TEST_RAISE( sCursorAttr != aSqlstmt->cursorsensitivity,
                    NeedReprepareCont );

    sRC = SQLGetStmtAttr( aStmtNode->mHstmt, SQL_ATTR_CURSOR_HOLD,
                          (SQLPOINTER) &sCursorAttr, ACI_SIZEOF(sCursorAttr),
                            NULL );
    ACE_ASSERT( SQL_SUCCEEDED(sRC) );
    ACI_TEST_RAISE( sCursorAttr != aSqlstmt->cursorwithhold,
                    NeedReprepareCont );

    ACI_EXCEPTION_CONT(NoNeedPrepareCont);

    sNeedReprepare = ACP_FALSE;

    ACI_EXCEPTION_CONT(NeedReprepareCont);

    return sNeedReprepare;
}

ACI_RC ulpPrepare ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    SQL statement에대한 prepare 처리 담당함.
 *
 *    처리구문> EXEC SQL PREPARE <statement_name> FROM <:host_var | string_literal>;
 *
 * Implementation :
 *    statement가 이미 존재 하면, prepare를 다시하여 덮어씌움.
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    SQLRETURN       sSqlRes;
    acp_bool_t      sIsAllocNode;

    ACP_UNUSED(reserved);

    sIsAllocNode  = ACP_FALSE;

    ULPGETCONNECTION(aConnName,sConnNode);

    sStmtNode = ulpLibStLookupStmt( &( sConnNode->mStmtHashT ),
                                    aSqlstmt->stmtname );

    if ( sStmtNode == NULL )
    {
        /* stmt hash table에 존재하지 않으면 stmt node를 새로 생성/설정 해준다.*/
        sStmtNode = ulpLibStNewNode(aSqlstmt, aSqlstmt->stmtname );
        ACI_TEST( sStmtNode == NULL);
        sIsAllocNode = ACP_TRUE;

        /* AllocStmt, Prepare 수행.*/
        sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, &( sStmtNode -> mHstmt ) );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_ALLOC_STMT);

        ACI_TEST( ulpPrepareCore( sConnNode,
                                  sStmtNode,
                                  aSqlstmt -> stmt,
                                  aSqlstmt -> sqlcaerr,
                                  aSqlstmt -> sqlcodeerr,
                                  aSqlstmt -> sqlstateerr )
                  == ACI_FAILURE );

        /* add stmt node to hash table.*/
        ACI_TEST_RAISE( ulpLibStAddStmt( &(sConnNode->mStmtHashT), sStmtNode ) == NULL,
                        ERR_STMT_ADDED_JUST_BEFORE );
    }
    else
    {
        /* hash table에 이미 존재한다.*/

        if ( ulpIsNeedReprepare(sStmtNode, aSqlstmt) == ACP_TRUE )
        {
            /*query 가 다르다면, prepare를 다시한다.*/
            switch(sStmtNode -> mStmtState)
            {
                case S_BINDING:
                case S_SETSTMTATTR:
                case S_EXECUTE:
                    sSqlRes = SQLFreeStmt( sStmtNode->mHstmt, SQL_UNBIND );
                    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                                    , ERR_CLI_FREE_STMT );
                    sSqlRes = SQLFreeStmt( sStmtNode->mHstmt, SQL_RESET_PARAMS );
                    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                                    , ERR_CLI_FREE_STMT );
                    sSqlRes = SQLFreeStmt( sStmtNode->mHstmt, SQL_CLOSE );
                    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                                    , ERR_CLI_FREE_STMT );

                    break;
                case S_INITIAL:
                    break;
                default:
                    break;
            }

            ACI_TEST( ulpPrepareCore( sConnNode,
                                      sStmtNode,
                                      aSqlstmt -> stmt,
                                      aSqlstmt -> sqlcaerr,
                                      aSqlstmt -> sqlcodeerr,
                                      aSqlstmt -> sqlstateerr )
                      == ACI_FAILURE );
        }
        else
        {
            /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
            if ( sStmtNode->mNeedReprepare == ACP_TRUE ) /* BUG-47239  mFailoveredJustnow 삭제 */
            {
                // do reprepare
                ACI_TEST( ulpPrepareCore  ( sConnNode,
                                            sStmtNode,
                                            aSqlstmt -> stmt,
                                            aSqlstmt -> sqlcaerr,
                                            aSqlstmt -> sqlcodeerr,
                                            aSqlstmt -> sqlstateerr )
                          == ACI_FAILURE );

                sStmtNode->mNeedReprepare = ACP_FALSE;
            }
        }
    }

    ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           SQLMSG_SUCCESS,
                           SQL_SUCCESS,
                           SQLSTATE_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_STMT_ADDED_JUST_BEFORE );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Conflict_Two_Emsqls_Error );

        SQLFreeStmt( sStmtNode -> mHstmt, SQL_DROP );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION ( ERR_CLI_FREE_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             sStmtNode->mHstmt,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    if ( sIsAllocNode == ACP_TRUE )
    {
        /* BUG-30789: Memory usage per prepared statement is too BIG. (>300k) */
        if( sStmtNode->mQueryStr != NULL )
        {
            acpMemFree( sStmtNode->mQueryStr );
        }
        acpMemFree( sStmtNode );
    }

    return ACI_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    SQL prepare 된 Query의 Bind 정보를 제공한다.
 *
 *    처리구문> EXEC SQL DESCRIBE BIND VARIABLES FOR <stmt_name> INTO <bind_variable>
 *
 * Implementation :
 *    statement는 무조건 존재해야 하며 prepare가 끝난 상태여야 한다. (없으면 에러)
 *
 *  struct SQLDA {
 *    int        N;     Descriptor size in number of entries        
 *    char     **V;     Ptr to Arr of addresses of main variables   
 *    int       *L;     Ptr to Arr of lengths of buffers            
 *    short     *T;     Ptr to Arr of types of buffers              
 *    short    **I;     Ptr to Arr of addresses of indicator vars   
 *    int        F;     Number of variables found by DESCRIBE       
 *  };
 *  typedef struct SQLDA SQLDA
 *
 ***********************************************************************/
ACI_RC ulpBindVariable( acp_char_t *aConnName,
                        ulpSqlstmt *aSqlstmt,
                        void       *aReserved )
{
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    
    SQLSMALLINT     sNumParams = 0;
    SQLSMALLINT     sParamType = 0;

    SQLDA          *sSqlda = NULL;
    acp_sint32_t    sI = 0;

    ACP_UNUSED(aReserved);

    ULPGETCONNECTION(aConnName, sConnNode);

    sStmtNode = ulpLibStLookupStmt( &( sConnNode->mStmtHashT ),
                                    aSqlstmt->stmtname );
    ACI_TEST_RAISE ( sStmtNode == NULL, ERR_NO_STMT );
    
    sSqlda = (SQLDA*) aSqlstmt->hostvalue[0].mHostVar;
    ACI_TEST( sSqlda == NULL );

    ACI_TEST( SQLNumParams( sStmtNode->mHstmt, &sNumParams ) != SQL_SUCCESS );

    for ( sI = 1; sI <= sNumParams; sI++ )
    {
        (void) SQLDescribeParam( sStmtNode->mHstmt,
                                 (SQLUSMALLINT) sI,
                                 &sParamType,
                                 NULL,
                                 NULL,
                                 NULL );
        sSqlda->T[sI - 1] = sParamType;
    }

    ACI_TEST_RAISE( sNumParams > sSqlda->N, ERR_MAX_BINDSIZE );
    sSqlda->F = sNumParams;

    ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           SQLMSG_SUCCESS,
                           SQL_SUCCESS,
                           SQLSTATE_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_NO_STMT );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Not_Exist_Error,
                         aSqlstmt->stmtname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_STMTNAME,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_MAX_BINDSIZE );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_DynamicSql_Max_BindSize_Small,
                         aSqlstmt->stmtname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_STMTNAME,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpSetArraySize( acp_char_t *aConnName,
                        ulpSqlstmt *aSqlstmt,
                        void       *aReserved )
{
    ulpLibConnNode   *sConnNode;
    ulpLibStmtNode   *sStmtNode;

    acp_uint32_t      sArraySize;

    SQLRETURN sSqlRes;

    ACP_UNUSED(aReserved);

    ULPGETCONNECTION( aConnName, sConnNode );

    sStmtNode = ulpLibStLookupStmt( &(sConnNode->mStmtHashT), aSqlstmt->stmtname );
    ACI_TEST_RAISE( sStmtNode == NULL, ERR_NO_STMT );

    sArraySize = *(acp_uint32_t*)(aSqlstmt->hostvalue[0].mHostVar);
    ACI_TEST_RAISE( sArraySize < 1, ERR_INVALID_ARRAYSIZE );
                    
    sSqlRes = SQLSetStmtAttr( sStmtNode->mHstmt,
                              SQL_ATTR_PARAMSET_SIZE,
                              (SQLPOINTER)(acp_ulong_t)sArraySize,
                              0 );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE), ERR_CLI_SETSTMT );

    sStmtNode->mBindInfo.mIsArray = (acp_bool_t)(sArraySize > 1 ? ACP_TRUE : ACP_FALSE); 
    sStmtNode->mBindInfo.mArraySize = sArraySize; 
    sStmtNode->mStmtState = S_SETSTMTATTR;

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_NO_CONN )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                              aSqlstmt->sqlcodeerr,
                              aSqlstmt->sqlstateerr,
                              ulpGetErrorMSG(&sErrorMgr),
                              SQL_INVALID_HANDLE,
                              ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION( ERR_NO_STMT )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Not_Exist_Error,
                         aSqlstmt->stmtname );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                              aSqlstmt->sqlcodeerr,
                              aSqlstmt->sqlstateerr,
                              ulpGetErrorMSG(&sErrorMgr),
                              SQLCODE_NO_STMTNAME,
                              ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION( ERR_INVALID_ARRAYSIZE )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Invalid_Array_Size_Error,
                         aSqlstmt->stmtname );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                              aSqlstmt->sqlcodeerr,
                              aSqlstmt->sqlstateerr,
                              ulpGetErrorMSG(&sErrorMgr),
                              SQLCODE_NO_STMTNAME,
                              ulpGetErrorSTATE(&sErrorMgr) );

    }
    ACI_EXCEPTION( ERR_CLI_SETSTMT )
    {
        (void) ulpSetErrorInfo4CLI( sConnNode,
                                    sStmtNode->mHstmt,
                                    (SQLRETURN) SQL_ERROR,
                                    aSqlstmt->sqlcaerr,
                                    aSqlstmt->sqlcodeerr,
                                    aSqlstmt->sqlstateerr,
                                    ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpBindParam( ulpLibConnNode *aConnNode,
                     ulpLibStmtNode *aStmtNode,
                     SQLHSTMT       *aHstmt,
                     ulpSqlstmt     *aSqlstmt,
                     acp_bool_t      aIsReBindCheck,
                     ulpSqlca       *aSqlca,
                     ulpSqlcode     *aSqlcode,
                     ulpSqlstate    *aSqlstate )
{

    SQLDA *sBinda = NULL;

    /* BUG-41010 dynamic binding */
    if ( IS_DYNAMIC_VARIABLE(aSqlstmt) )
    {
        sBinda = (SQLDA*) aSqlstmt->hostvalue[0].mHostVar;

        ACI_TEST( ulpDynamicBindParameter( aConnNode,
                                           aStmtNode,
                                           aHstmt,
                                           aSqlstmt,
                                           sBinda,
                                           aSqlca,
                                           aSqlcode,
                                           aSqlstate )
                  == ACI_FAILURE );

        ACI_TEST( ulpSetStmtAttrDynamicParamCore( aConnNode,
                                                  aStmtNode,
                                                  aHstmt,
                                                  aSqlca,
                                                  aSqlcode,
                                                  aSqlstate )
                  == ACI_FAILURE );
    }
    else
    {
        if ( aIsReBindCheck == ACP_TRUE )
        {
            if ( ulpCheckNeedReBindParamCore( aStmtNode, aSqlstmt ) == ACP_TRUE )
            {
                ACI_TEST( ulpBindHostVarCore( aConnNode,
                                              aStmtNode,
                                              aHstmt,
                                              aSqlstmt,
                                              aSqlca,
                                              aSqlcode,
                                              aSqlstate )
                          == ACI_FAILURE );
            }
            else
            {
                /* do nothing */
            }

            /* BUG-32051 : If the number of FOR loop is changed,
             *        SQL_ATTR_ROW_ARRAY_SIZE should be updated.*/
            if ( ulpCheckNeedReSetStmtAttrCore( aStmtNode, aSqlstmt ) == ACP_TRUE )
            {
                ACI_TEST( ulpSetStmtAttrParamCore( aConnNode,
                                                   aStmtNode,
                                                   aHstmt,
                                                   aSqlstmt,
                                                   aSqlca,
                                                   aSqlcode,
                                                   aSqlstate )
                          == ACI_FAILURE );
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            ACI_TEST( ulpBindHostVarCore( aConnNode,
                                          aStmtNode,
                                          aHstmt,
                                          aSqlstmt,
                                          aSqlca,
                                          aSqlcode,
                                          aSqlstate )
                      == ACI_FAILURE );
        
            ACI_TEST( ulpSetStmtAttrParamCore( aConnNode,
                                               aStmtNode,
                                               aHstmt,
                                               aSqlstmt,
                                               aSqlca,
                                               aSqlcode,
                                               aSqlstate )
                      == ACI_FAILURE );
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulpExecute ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    Prepare된 statement대한 execute 처리 담당함.
 *
 *    처리구문> EXEC SQL EXECUTE <statement_name> [ USING <host_var_list> ];
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;

    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    sStmtNode = ulpLibStLookupStmt( &( sConnNode->mStmtHashT ),
                                    aSqlstmt->stmtname );

    ACI_TEST_RAISE( sStmtNode == NULL, ERR_NO_STMT );

    ulpSetColRowSizeCore( aSqlstmt );

    /* FOR절 처리*/
    ACI_TEST( ulpAdjustArraySize(aSqlstmt) == ACI_FAILURE );

    switch ( sStmtNode->mStmtState )
    {
        case S_INITIAL:
            /* PREPARE를 먼저 해야한다.*/
            ACI_RAISE( ERR_STMT_NEED_PREPARE_4EXEC );
            break;
        case S_PREPARE:
            /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
            if ( sStmtNode->mNeedReprepare == ACP_TRUE ) /* BUG-47239  mFailoveredJustnow 삭제 */
            {
                // do prepare
                ACI_TEST_RAISE( ulpPrepareCore ( sConnNode,
                                                 sStmtNode,
                                                 sStmtNode->mQueryStr,
                                                 aSqlstmt->sqlcaerr,
                                                 aSqlstmt->sqlcodeerr,
                                                 aSqlstmt->sqlstateerr )
                                == ACI_FAILURE, ERR_PREPARE_CORE );

                sStmtNode->mNeedReprepare = ACP_FALSE;
            }

            /*host 변수가 있으면 binding & setstmt해준다.*/
            if( aSqlstmt->numofhostvar > 0 )
            {
                /* BUG-41010 dynamic binding */
                ACI_TEST( ulpBindParam ( sConnNode,
                                         sStmtNode,
                                         &(sStmtNode->mHstmt),
                                         aSqlstmt,
                                         ACP_FALSE,
                                         aSqlstmt->sqlcaerr,
                                         aSqlstmt->sqlcodeerr,
                                         aSqlstmt->sqlstateerr ) == ACI_FAILURE );
            }
            break;
        default:
            /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
            if ( sStmtNode->mNeedReprepare == ACP_TRUE ) /* BUG-47239  mFailoveredJustnow 삭제 */
            {
                // do prepare
                ACI_TEST_RAISE( ulpPrepareCore  ( sConnNode,
                                                  sStmtNode,
                                                  sStmtNode-> mQueryStr,
                                                  aSqlstmt -> sqlcaerr,
                                                  aSqlstmt -> sqlcodeerr,
                                                  aSqlstmt -> sqlstateerr )
                                == ACI_FAILURE, ERR_PREPARE_CORE );

                sStmtNode->mNeedReprepare = ACP_FALSE;
            }

            /* 필요하다면 binding & setstmt 를 다시해준다.*/
            if( aSqlstmt->numofhostvar > 0 )
            {
                /* BUG-41010 dynamic binding */
                ACI_TEST( ulpBindParam ( sConnNode,
                                         sStmtNode,
                                         &(sStmtNode->mHstmt),
                                         aSqlstmt,
                                         ACP_TRUE,
                                         aSqlstmt->sqlcaerr,
                                         aSqlstmt->sqlcodeerr,
                                         aSqlstmt->sqlstateerr ) == ACI_FAILURE );
            }
            break;
    }

    ACI_TEST( ulpExecuteCore( sConnNode,
                              sStmtNode,
                              aSqlstmt,
                              &(sStmtNode->mHstmt) ) == ACI_FAILURE );

    sStmtNode -> mStmtState = S_EXECUTE;

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_NO_STMT );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Not_Exist_Error,
                         aSqlstmt->stmtname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_STMTNAME,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_STMT_NEED_PREPARE_4EXEC );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Need_Prepare_4Execute_Error,
                         aSqlstmt->stmtname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_PREPARE_CORE );
    {
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpSetStmtAttr4Prepare( ulpLibConnNode *aConnNode,
                               ulpLibStmtNode *aStmtNode,
                               ulpSqlstmt     *aSqlstmt )
{
    SQLRETURN sRC;

    if (aSqlstmt->cursorscrollable == SQL_SCROLLABLE)
    {
        /* scrollable + sensitive == keyset-driven */
        if (aSqlstmt->cursorsensitivity == SQL_SENSITIVE)
        {
            sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                                  SQL_ATTR_CURSOR_TYPE,
                                  (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN,
                                  0 );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );

            sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                                  SQL_ATTR_CONCURRENCY,
                                  (SQLPOINTER)SQL_CONCUR_VALUES,
                                  0 );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );
        }
        /* scrollable + insensitive == static */
        else
        {
            sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                                  SQL_ATTR_CURSOR_TYPE,
                                  (SQLPOINTER)SQL_CURSOR_STATIC,
                                  0 );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );

            sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                                  SQL_ATTR_CONCURRENCY,
                                  (SQLPOINTER)SQL_CONCUR_READ_ONLY,
                                  0 );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );
        }

        aStmtNode->mBindInfo.mIsScrollCursor = ACP_TRUE;
    }
    else /* non-scrollable */
    {
        if (aStmtNode->mBindInfo.mIsScrollCursor == ACP_FALSE)
        {
            /* 굳이 다시 설정할 필요 없다.
             * scrollable이 아니면 항상 forward-only이기 때문. */
        }
        else
        {
            sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                                  SQL_ATTR_CURSOR_TYPE,
                                  SQL_CURSOR_FORWARD_ONLY,
                                  0 );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );

            /* forward-only는 insensitive만 지원 */
            sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                                  SQL_ATTR_CURSOR_SENSITIVITY,
                                  (SQLPOINTER)SQL_INSENSITIVE,
                                  0 );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );

            aStmtNode->mBindInfo.mIsScrollCursor = ACP_FALSE;
        }
    }

    /* cursor WITH HOLD */
    if (aSqlstmt->cursorwithhold == SQL_CURSOR_HOLD_ON)
    {
        sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                              SQL_ATTR_CURSOR_HOLD,
                              (SQLPOINTER)SQL_CURSOR_HOLD_ON,
                              0 );
        ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );
    }
    else
    {
        sRC = SQLSetStmtAttr( aStmtNode->mHstmt,
                              SQL_ATTR_CURSOR_HOLD,
                              (SQLPOINTER)SQL_CURSOR_HOLD_OFF,
                              0 );
        ACI_TEST_RAISE( ! SQL_SUCCEEDED(sRC), ERR_CLI_SETSTMT_4SCROLL );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CLI_SETSTMT_4SCROLL )
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             aStmtNode->mHstmt,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpSelectList( acp_char_t *aConnName,
                      ulpSqlstmt *aSqlstmt,
                      void       *aReserved )
{
/***********************************************************************
 *
 * Description :
 *    SQL prepare 된 Select Query의 결과집합 정보를 제공한다.
 *
 *    처리구문> EXEC SQL DESCRIBE SELECT LIST FOR <stmt_name> INTO <bind_variable>
 *
 * Implementation :
 *    statement는 무조건 존재해야 하며 prepare가 끝난 상태여야 한다. (없으면 에러)
 *
 *  struct SQLDA {
 *    int        N;     Descriptor size in number of entries        
 *    char     **V;     Ptr to Arr of addresses of main variables   
 *    int       *L;     Ptr to Arr of lengths of buffers            
 *    short     *T;     Ptr to Arr of types of buffers              
 *    short    **I;     Ptr to Arr of addresses of indicator vars   
 *    int        F;     Number of variables found by DESCRIBE       
 *  };
 *  typedef struct SQLDA SQLDA
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    
    SQLSMALLINT     sNumResultCols = 0;
    SQLSMALLINT     sType = 0;

    SQLDA          *sSqlda = NULL;
    SQLRETURN       sSqlRes;
    acp_sint32_t    sI = 0;

    ACP_UNUSED(aReserved);

    ULPGETCONNECTION(aConnName, sConnNode);

    sStmtNode = ulpLibStLookupStmt( &( sConnNode->mStmtHashT ),
                                    aSqlstmt->stmtname );
    ACI_TEST_RAISE ( sStmtNode == NULL, ERR_NO_STMT );
    
    sSqlda = (SQLDA*) aSqlstmt->hostvalue[0].mHostVar;
    ACI_TEST( sSqlda == NULL );

    ACI_TEST( SQLNumResultCols( sStmtNode->mHstmt, &sNumResultCols  ) != SQL_SUCCESS );

    for ( sI = 0; sI < sNumResultCols; sI++ )
    {
        sSqlRes = SQLDescribeCol( sStmtNode->mHstmt,
                                  (SQLUSMALLINT)(sI + 1),
                                  NULL,
                                  0,
                                  NULL,
                                  &sType,
                                  NULL,
                                  NULL,
                                  NULL );
        ACI_TEST( sSqlRes != SQL_SUCCESS );
        sSqlda->T[sI] = sType;
    }

    ACI_TEST_RAISE( sNumResultCols > sSqlda->N, ERR_MAX_COLUMNSIZE ); 
    sSqlda->F = sNumResultCols;

    ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           SQLMSG_SUCCESS,
                           SQL_SUCCESS,
                           SQLSTATE_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_NO_STMT );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Not_Exist_Error,
                         aSqlstmt->stmtname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_STMTNAME,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_MAX_COLUMNSIZE );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_DynamicSql_Max_ColumnSize_Small,
                         aSqlstmt->stmtname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_STMTNAME,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpDeclareCursor ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    커서 선언 처리 담당함.
 *
 *    처리구문> EXEC SQL DECLARE <cursor_name> [SCROLL] CURSOR
 *            FOR <stmt_id | string_literal>;
 *
 * Implementation :
 *
 * WARNING : Multi threaded 프로그램의 경우, 내장구문 처리시 동기화 시켜주지 않으면 비정상 처리될 수 있음.
 * 
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    ulpLibStmtNode *sCurNode;
    acp_char_t     *sQuery;
    SQLRETURN       sSqlRes;
    acp_bool_t      sDoNothing;

    ACP_UNUSED(reserved);

    sDoNothing    = ACP_FALSE;

    ULPGETCONNECTION(aConnName,sConnNode);

    if( aSqlstmt->stmtname != NULL )
    {
        /* FOR절에 stmt_id가 올경우.*/
        /* 1. stmt hash table에서 해당 stmt 이름을 찾아본다.*/
        sStmtNode = ulpLibStLookupStmt( &( sConnNode->mStmtHashT ),
                                        aSqlstmt->stmtname );
        ACI_TEST_RAISE ( sStmtNode == NULL, ERR_NO_STMT );

        /* 2. cursor hash table에서 해당 cursor 이름을 찾아본다.*/
        sCurNode = ulpLibStLookupCur( &( sConnNode->mCursorHashT ),
                                      aSqlstmt->curname );

        if( sCurNode == NULL )
        {
            /* 해당 커서이름이 존재하지 않는다면, cursor hash table에 해당 stmt노드에 대한
             * 링크를 추가해준다.
             */
            if( sStmtNode->mCursorName[0] != '\0' )
            {
                ulpLibStDeleteCur( &( sConnNode->mCursorHashT ),
                                   sStmtNode->mCursorName );
            }
            ulpLibStAddCurLink( &( sConnNode->mCursorHashT ),
                                sStmtNode,
                                aSqlstmt->curname );
        }
        else
        {
            /* 해당 커서이름이 이미 존재*/
            if( sStmtNode == sCurNode )
            {
                /* 둘다 동일한 stmt node라면 ok*/
                /* do nothing */
                sDoNothing = ACP_TRUE;
            }
            else
            {
                /* 서로 다른 stmt node라면 기존의 cursor node에 덮어 씌움.*/
                if( sCurNode -> mStmtName[0] != '\0' )
                {
                    ( void ) ulpLibStDeleteCur( &( sConnNode->mCursorHashT ),
                                                aSqlstmt->curname );
                    /* ulpCloseStmtCore(...) ?*/
                    sCurNode -> mCursorName[0] = '\0';
                    sCurNode -> mCurState = C_INITIAL;
                }
                else
                {
                    sSqlRes = SQLFreeStmt( sCurNode->mHstmt, SQL_DROP );
                    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                                    , ERR_CLI_FREE_STMT );
                    ( void ) ulpLibStDeleteCur( &( sConnNode->mCursorHashT ),
                                                aSqlstmt->curname );
                }

                if( sStmtNode->mCursorName[0] != '\0' )
                {
                    ulpLibStDeleteCur( &( sConnNode->mCursorHashT ),
                                       sStmtNode->mCursorName );
                }
                ulpLibStAddCurLink( &( sConnNode->mCursorHashT ),
                                    sStmtNode,
                                    aSqlstmt->curname );
            }
        }

        if ( ulpIsNeedReprepare(sStmtNode, aSqlstmt) == ACP_TRUE )
        {
            /* 이미 Prepare되어있다면 커서 속성을 바꿀 수 없다.
             * 다시 Prepare하려면 Stmt를 다시 생성해야한다. */
            sSqlRes = SQLFreeStmt( sStmtNode->mHstmt, SQL_DROP );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sSqlRes), ERR_CLI_FREE_STMT );

            /* alloc stmt, prepare stmt*/
            sSqlRes = SQLAllocStmt( sConnNode->mHdbc, &(sStmtNode->mHstmt) );
            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sSqlRes), ERR_CLI_ALLOC_STMT );

            ACI_TEST( ulpSetStmtAttr4Prepare(sConnNode,
                                             sStmtNode,
                                             aSqlstmt)
                      != ACI_SUCCESS );

            /* DECLARE 때 쿼리문을 지정하지 않았다면, 기존 쿼리문 사용 */
            sQuery = (aSqlstmt->stmt != NULL)
                   ? aSqlstmt->stmt : sStmtNode->mQueryStr;

            ACI_TEST( ulpPrepareCore( sConnNode,
                                      sStmtNode,
                                      sQuery,
                                      aSqlstmt->sqlcaerr,
                                      aSqlstmt->sqlcodeerr,
                                      aSqlstmt->sqlstateerr )
                      == ACI_FAILURE );

            ACI_TEST_RAISE( ! SQL_SUCCEEDED(sSqlRes), ERR_CLI_FREE_STMT );

            sDoNothing = ACP_FALSE;
        }
    }
    else
    {
        /* FOR절에 string_literal이 올경우.*/
        /* cursor hash table에서 cursor 이름으로 찾아본다.*/
        sCurNode = ulpLibStLookupCur( &( sConnNode->mCursorHashT ),
                                      aSqlstmt->curname );
        if( sCurNode != NULL )
        {
            /* same query */
            if ( ulpIsNeedReprepare(sCurNode, aSqlstmt) != ACP_TRUE )
            {
                /* INITIAL상태일 경우에는 무조건 다시 prepare함. */
                if( sCurNode -> mCurState == C_INITIAL )
                {

                    ACI_TEST( ulpSetStmtAttr4Prepare(sConnNode,
                                                     sCurNode,
                                                     aSqlstmt)
                              != ACI_SUCCESS );

                    ACI_TEST( ulpPrepareCore  ( sConnNode,
                                                sCurNode,
                                                aSqlstmt -> stmt,
                                                aSqlstmt -> sqlcaerr,
                                                aSqlstmt -> sqlcodeerr,
                                                aSqlstmt -> sqlstateerr )
                              == ACI_FAILURE );
                }
                else
                {
                    /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
                    if ( sCurNode ->mNeedReprepare == ACP_TRUE ) /* BUG-47239  mFailoveredJustnow 삭제 */
                    {

                        ACI_TEST( ulpSetStmtAttr4Prepare(sConnNode,
                                                         sCurNode,
                                                         aSqlstmt)
                                  != ACI_SUCCESS );

                        /* do reprepare */
                        ACI_TEST( ulpPrepareCore  ( sConnNode,
                                                    sCurNode,
                                                    aSqlstmt -> stmt,
                                                    aSqlstmt -> sqlcaerr,
                                                    aSqlstmt -> sqlcodeerr,
                                                    aSqlstmt -> sqlstateerr )
                                  == ACI_FAILURE );

                        sCurNode->mNeedReprepare = ACP_FALSE;
                    }
                    /* do nothing */
                    sDoNothing = ACP_TRUE;
                }
                sStmtNode = sCurNode;
            }
            /* different query */
            else
            {
                if( sCurNode -> mStmtName[0] != '\0' )
                {
                    ( void ) ulpLibStDeleteCur( &( sConnNode->mCursorHashT ),
                                                aSqlstmt->curname );

                    sCurNode -> mCursorName[0] = '\0';
                    sCurNode -> mCurState = C_INITIAL;

                    /* stmt node생성후 hash table에 추가.*/
                    sStmtNode = ulpLibStNewNode(aSqlstmt, NULL );
                    /* bug-36991: assertion in ulpLibStNewNode is not good */
                    ACI_TEST(sStmtNode == NULL);

                    ulpLibStAddCurLink( &( sConnNode->mCursorHashT ),
                                        sStmtNode,
                                        aSqlstmt->curname );

                    /* alloc stmt, prepare stmt*/
                    sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, &( sStmtNode -> mHstmt ) );
                    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                                    , ERR_CLI_ALLOC_STMT);


                    ACI_TEST( ulpSetStmtAttr4Prepare(sConnNode,
                                                     sStmtNode,
                                                     aSqlstmt)
                              != ACI_SUCCESS );
                    
                    ACI_TEST( ulpPrepareCore  ( sConnNode,
                                                sStmtNode,
                                                aSqlstmt -> stmt,
                                                aSqlstmt -> sqlcaerr,
                                                aSqlstmt -> sqlcodeerr,
                                                aSqlstmt -> sqlstateerr )
                            == ACI_FAILURE );
                }
                else
                {
                    ACI_TEST( ulpCloseStmtCore( sConnNode,
                                                sCurNode,
                                                &(sCurNode-> mHstmt),
                                                aSqlstmt -> sqlcaerr,
                                                aSqlstmt -> sqlcodeerr,
                                                aSqlstmt -> sqlstateerr )
                             == ACI_FAILURE );
                   
                    ACI_TEST( ulpSetStmtAttr4Prepare(sConnNode,
                                                     sCurNode,
                                                     aSqlstmt)
                              != ACI_SUCCESS );
                    
                    ACI_TEST( ulpPrepareCore  ( sConnNode,
                                                sCurNode,
                                                aSqlstmt -> stmt,
                                                aSqlstmt -> sqlcaerr,
                                                aSqlstmt -> sqlcodeerr,
                                                aSqlstmt -> sqlstateerr )
                            == ACI_FAILURE );

                    sStmtNode = sCurNode;
                }
            }
        }
        else
        {
            /* unexist*/
            /* stmt node생성후 hash table에 추가.*/
            sStmtNode = ulpLibStNewNode(aSqlstmt, NULL );
            ACI_TEST(sStmtNode == NULL);

            ulpLibStAddCurLink( &( sConnNode->mCursorHashT ),
                                sStmtNode,
                                aSqlstmt->curname );

            /* alloc stmt, prepare stmt*/
            sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, &( sStmtNode -> mHstmt ) );
            ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                            , ERR_CLI_ALLOC_STMT);


            ACI_TEST( ulpSetStmtAttr4Prepare(sConnNode,
                                             sStmtNode,
                                             aSqlstmt)
                      != ACI_SUCCESS );

            ACI_TEST( ulpPrepareCore  ( sConnNode,
                                        sStmtNode,
                                        aSqlstmt -> stmt,
                                        aSqlstmt -> sqlcaerr,
                                        aSqlstmt -> sqlcodeerr,
                                        aSqlstmt -> sqlstateerr )
                     == ACI_FAILURE );
        }

        /* comment */
        if ( aSqlstmt -> numofhostvar > 0 )
        {
            ulpSetColRowSizeCore( aSqlstmt );

            ACI_TEST( ulpBindHostVarCore( sConnNode,
                                          sStmtNode,
                                          &(sStmtNode->mHstmt),
                                          aSqlstmt,
                                          aSqlstmt -> sqlcaerr,
                                          aSqlstmt -> sqlcodeerr,
                                          aSqlstmt -> sqlstateerr )
                      == ACI_FAILURE );
            ACI_TEST( ulpSetStmtAttrParamCore( sConnNode,
                                               sStmtNode,
                                               &(sStmtNode->mHstmt),
                                               aSqlstmt,
                                               aSqlstmt -> sqlcaerr,
                                               aSqlstmt -> sqlcodeerr,
                                               aSqlstmt -> sqlstateerr )
                      == ACI_FAILURE );
        }
    }

    if ( sDoNothing != ACP_TRUE )
    {
        sStmtNode -> mCurState = C_DECLARE;
    }

    ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           SQLMSG_SUCCESS,
                           SQL_SUCCESS,
                           SQLSTATE_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_NO_CONN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         ULPCHECKCONNECTIONNAME(aConnName) );
                         

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_INVALID_HANDLE,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_NO_STMT );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Not_Exist_Error,
                         aSqlstmt->stmtname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_STMTNAME,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_CLI_FREE_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             sCurNode->mHstmt,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION ( ERR_CLI_ALLOC_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

