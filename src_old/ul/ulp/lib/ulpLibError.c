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

#include <ulpLibMultiErrorMgr.h>
#include <ulpLibError.h>

void ulpSetErrorInfo4PCOMP( ulpSqlca          *aSqlca,
                            ulpSqlcode        *aSqlcode,
                            ulpSqlstate       *aSqlstate,
                            const acp_char_t  *aErrMsg,
                            acp_sint32_t       aErrCode,
                            const acp_char_t  *aErrState )
{
/***********************************************************************
 *
 * Description :
 *    내장 SQL문 수행 결과정보를 저장하는 sqlca, sqlcode, sqlstate를
 *    명시적으로 인자로 넘겨준 값으로 설정 해주는 함수. precompiler 내부 에러 설정 담당.
 *
 * Implementation :
 *
 ***********************************************************************/
    if ( aErrMsg != NULL )
    {
        aSqlca->sqlerrm.sqlerrml =
                (short)acpCStrLen(aErrMsg, ACP_SINT16_MAX);

        acpCStrCpy( aSqlca->sqlerrm.sqlerrmc,
                    MAX_ERRMSG_LEN,
                    aErrMsg,
                    acpCStrLen(aErrMsg, ACP_SINT32_MAX));
    }
    else
    {
        aSqlca->sqlerrm.sqlerrmc[0] = '\0';
        aSqlca->sqlerrm.sqlerrml    = 0;
    }

    if ( aErrCode != ERRCODE_NULL )
    {
        switch ( aErrCode )
        {
        /* SQL_ERROR, SQL_SUCCESS, SQL_INVALID_HANDLE ... 에 따라 *
         * SQLCODE, sqlca->sqlcode를 명시된 값으로 설정해줌.         */
            case SQL_SUCCESS:
                aSqlca->sqlcode = SQL_SUCCESS;
                *(aSqlcode) = SQL_SUCCESS;
                break;
            case SQL_SUCCESS_WITH_INFO:
                aSqlca->sqlcode = SQL_SUCCESS_WITH_INFO;
                *(aSqlcode) = SQL_SUCCESS_WITH_INFO;
                break;
            case SQL_NO_DATA:
                aSqlca->sqlcode = SQL_NO_DATA;
                *(aSqlcode) = SQL_NO_DATA;
                break;
            case SQL_ERROR:
                aSqlca->sqlcode = SQL_ERROR;
                *(aSqlcode) = SQL_ERROR;
                break;
            case SQL_INVALID_HANDLE:
                aSqlca->sqlcode = SQL_INVALID_HANDLE;
                *(aSqlcode) = SQL_INVALID_HANDLE;
                break;
            default:
                aSqlca->sqlcode = SQL_ERROR;
                *(aSqlcode) = (acp_sint32_t)( -1 * (aErrCode) );
                break;
        }
    }

    if ( aErrState != NULL )
    {
        acpCStrCpy( *aSqlstate,
                     MAX_ERRSTATE_LEN,
                     aErrState,
                     acpCStrLen(aErrState, ACP_SINT16_MAX) );
    }
    else
    {
        (*aSqlstate)[0] = '\0';
    }
    /* fetch 거나 select일경우 sqlerrd[2]에는 fetch된 row수가 저장돼야한다.*/
    aSqlca->sqlerrd[2] = 0;
    aSqlca->sqlerrd[3] = 0;
}


ACI_RC ulpSetErrorInfo4CLI( ulpLibConnNode    *aConnNode,
                            SQLHSTMT           aHstmt,
                            const SQLRETURN    aSqlRes,
                            ulpSqlca          *aSqlca,
                            ulpSqlcode        *aSqlcode,
                            ulpSqlstate       *aSqlstate,
                            ulpLibErrType      aCLIType )
{
/***********************************************************************
 *
 * Description :
 *     내장 SQL문 수행 결과정보를 ODBC CLI 함수를 이용해 설정 해주는 함수.
 *    CLI에서 발생한 에러 설정 담당.
 *     ulpSetErrorInfo4CLI 함수 내에서 CLI호출 중 에러가 발생하면 SQL_FAILURE 리턴,
 *    그외의 경우 SQL_SUCCESS 리턴.
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint64_t sRowCnt;
    SQLRETURN    sRc;
    SQLHENV     *sHenv;
    SQLHDBC     *sHdbc;
    /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
    SQLSMALLINT sRecordNo;
    /* sqlcli수행결과정보를 임시로 저장해두기위한 변수들 */
    ulpSqlcode   sSqlcode;
    ulpSqlstate  sSqlstate;
    acp_char_t   sErrMsg[MAX_ERRMSG_LEN];
    acp_sint16_t sErrMsgLen;

    acp_bool_t        sExistFailOverSuccess;
    ulpMultiErrorMgr *sMultiErrMgr; /* TASK-7218 */

    sHenv = &(aConnNode->mHenv);
    sHdbc = &(aConnNode->mHdbc);
    sMultiErrMgr = &(aConnNode->mMultiErrorMgr);

    ulpLibInitMultiErrorMgr(sMultiErrMgr);
    ulpLibSetMultiErrorMgr(NULL);

    switch ( aSqlca->sqlcode = aSqlRes )
    {
        case SQL_SUCCESS :
            *aSqlcode = aSqlRes;
            acpCStrCpy( *aSqlstate,
                         MAX_ERRSTATE_LEN,
                         (acp_char_t *)SQLSTATE_SUCCESS,
                         MAX_ERRSTATE_LEN-1);

            acpCStrCpy( aSqlca->sqlerrm.sqlerrmc,
                        MAX_ERRMSG_LEN,
                        (acp_char_t *)SQLMSG_SUCCESS,
                        acpCStrLen((acp_char_t *)SQLMSG_SUCCESS, ACP_SINT16_MAX));

            aSqlca->sqlerrm.sqlerrml = (short)acpCStrLen(aSqlca->sqlerrm.sqlerrmc,
                                                         ACP_SINT16_MAX);
            sRowCnt = 0;
            switch ( aCLIType )
            {
                case ERR_TYPE1_DML :
                    sRc = SQLGetStmtAttr( aHstmt,
                                          ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT,
                                          &sRowCnt, 0, NULL);
                    ACI_TEST_RAISE ( sRc != SQL_SUCCESS, ERR_GETERRINFO );
                    aSqlca->sqlerrd[2] = (acp_sint32_t) sRowCnt;
                    break;
                default :
                    aSqlca->sqlerrd[2] = (acp_sint32_t) sRowCnt;
            }
            /*aSqlca->sqlerrd[3] = 0;*/
            break;
        case SQL_NO_DATA :
            switch ( aCLIType )
            {
                case ERR_TYPE1:
                case ERR_TYPE1_DML :
                    *(aSqlcode) = SQLCODE_SQL_NO_DATA;

                    acpCStrCpy( *aSqlstate,
                                 MAX_ERRSTATE_LEN,
                                 (acp_char_t *)SQLSTATE_SQL_NO_DATA,
                                 MAX_ERRSTATE_LEN-1);

                    acpCStrCpy( aSqlca->sqlerrm.sqlerrmc,
                                MAX_ERRMSG_LEN,
                                (acp_char_t *)SQLMSG_SQL_NO_DATA,
                                acpCStrLen((acp_char_t *)SQLMSG_SQL_NO_DATA,
                                           ACP_SINT16_MAX)
                              );

                    aSqlca->sqlerrm.sqlerrml = (short)acpCStrLen(aSqlca->sqlerrm.sqlerrmc,
                                                                 ACP_SINT16_MAX);

                    sRowCnt = 0;
                    if ( aCLIType == ERR_TYPE1_DML )
                    {
                        sRc = SQLGetStmtAttr( aHstmt,
                                              ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT,
                                              &sRowCnt, 0, NULL);
                        ACI_TEST_RAISE ( sRc != SQL_SUCCESS, ERR_GETERRINFO );
                    }
                    aSqlca->sqlerrd[2] = (acp_sint32_t) sRowCnt;
                    /*aSqlca->sqlerrd[3] = 0;*/
                    break;
                default :
                    goto GOTO_DEFAULT;
            }
            break;
        case SQL_INVALID_HANDLE :
            ACI_RAISE ( ERR_INVALID_HANDLE );
            break;
        case SQL_SUCCESS_WITH_INFO :
            switch ( aCLIType )
            {
                case ERR_TYPE1 :
                case ERR_TYPE1_DML :
                case ERR_TYPE2 :
                    /* SQLError 함수를 통해 SQLSTATE, SQLCODE, 에러 메시지 얻어옴. */
                    sRc = SQLError(*sHenv,
                                   *sHdbc,
                                   aHstmt,
                                   (SQLCHAR *)aSqlstate,
                                   (SQLINTEGER *)aSqlcode,
                                   (SQLCHAR *)aSqlca->sqlerrm.sqlerrmc,
                                   MAX_ERRMSG_LEN,
                                   (SQLSMALLINT *)&(aSqlca->sqlerrm.sqlerrml));

                    if ( sRc == SQL_SUCCESS || sRc == SQL_SUCCESS_WITH_INFO)
                    {
                        *(aSqlcode) = SQL_SUCCESS_WITH_INFO;
                        sRowCnt = 0;
                        if ( aCLIType == ERR_TYPE1_DML )
                        {
                            sRc = SQLGetStmtAttr( aHstmt,
                                                  ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT,
                                                  &sRowCnt, 0, NULL);
                            ACI_TEST_RAISE ( sRc != SQL_SUCCESS, ERR_GETERRINFO );
                        }
                        aSqlca->sqlerrd[2] = (acp_sint32_t) sRowCnt;
                        /*aSqlca->sqlerrd[3] = 0;*/
                    }
                    else
                    {
                        ACI_TEST_RAISE ( sRc == SQL_INVALID_HANDLE, ERR_INVALID_HANDLE );
                    }
                    break;
                default :
                    goto GOTO_DEFAULT;
            }
            break;

        default :
/* goto */
GOTO_DEFAULT:
        sRecordNo = 1;
        /* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
        /* SQLGetDiagRec 함수를 통해 SQLSTATE, SQLCODE, 에러 메시지 얻어옴. */
        /* failover성공후 또다른 SQLCLI함수가 호출되어 다른에러로 에러값을 덮어 씌울 수 있기때문에
           SQLGetDiagRec 함수로 이전 상황의 에러값도 확인함. */
        if( aHstmt == SQL_NULL_HSTMT )
        {
            sRc = SQLGetDiagRec(SQL_HANDLE_DBC,
                               *sHdbc,
                                sRecordNo,
                                (SQLCHAR *)aSqlstate,
                                (SQLINTEGER *)aSqlcode,
                                (SQLCHAR *)aSqlca->sqlerrm.sqlerrmc,
                                MAX_ERRMSG_LEN,
                                (SQLSMALLINT *)&(aSqlca->sqlerrm.sqlerrml));
        }
        else
        {
            /* TASK-7218 Handling Multiple Errors */
            ACI_TEST( ulpLibMultiErrorMgrSetNumber( aHstmt, sMultiErrMgr )
                      != ACI_SUCCESS );

            /* TASK-7218 Handling Multiple Errors 
               FAILOVER_SUCCESS가 리턴되더라도 Multi-Error를 모두 셋팅하기 위해
               break 대신에 flag를 두어 계속 반복하도록 함 */
            /* 에러 or SQL_NO_DATA가 리턴될때까지 반복해서 호출하며, FAILOVER_SUCCESS가 리턴되면
               내장 구문 재수행시 reprepare를 하기위한 처리를 하며, 그외의 경우에는 가장 최근의 error정보를
               저장함. */
            /* 반복해서 SQL_ERROR가 리턴되지는 않는다고 가정함.
               가끔이라도 정상처리가 되면 언젠가는 SQL_NO_DATA가 리턴됨. */
            sExistFailOverSuccess = ACP_FALSE;
            while ((sRc = SQLGetDiagRec(SQL_HANDLE_STMT,
                                        aHstmt,
                                        sRecordNo,
                                        (SQLCHAR *)sSqlstate,
                                        (SQLINTEGER *)&sSqlcode,
                                        (SQLCHAR *)sErrMsg,
                                        MAX_ERRMSG_LEN,
                                        (SQLSMALLINT *)&sErrMsgLen) != SQL_NO_DATA))
            {
                if(sRc == SQL_INVALID_HANDLE )
                {
                    break;
                }

                if ( sSqlcode == ALTIBASE_FAILOVER_SUCCESS &&
                     sExistFailOverSuccess == ACP_FALSE )
                {
                    *aSqlcode = sSqlcode;
                    acpSnprintf(*aSqlstate, MAX_SQLSTATE_LEN, "%s", sSqlstate);
                    acpSnprintf(aSqlca->sqlerrm.sqlerrmc, MAX_ERRMSG_LEN, "%s", sErrMsg);
                    aSqlca->sqlerrm.sqlerrml = sErrMsgLen;
                    sExistFailOverSuccess = ACP_TRUE;
                }
                else
                {
                    /* 가장 최근의 error정보를 저장해둔다. */
                    if(sRecordNo==1)
                    {
                        *aSqlcode = sSqlcode;
                        acpSnprintf(*aSqlstate, MAX_SQLSTATE_LEN, "%s", sSqlstate);
                        acpSnprintf(aSqlca->sqlerrm.sqlerrmc, MAX_ERRMSG_LEN, "%s", sErrMsg);
                        aSqlca->sqlerrm.sqlerrml = sErrMsgLen;
                    }
                }

                /* TASK-7218 Handling Multiple Errors */
                ulpLibMultiErrorMgrSetDiagRec( aHstmt,
                                         sMultiErrMgr,
                                         sRecordNo,
                                         sSqlcode,
                                         sSqlstate,
                                         sErrMsg );

                sRecordNo++;
            }

            /* TASK-7218 Handling Multiple Errors */
            ulpLibSetMultiErrorMgr(sMultiErrMgr);
        }

        ACI_TEST_RAISE ( sRc == SQL_INVALID_HANDLE, ERR_INVALID_HANDLE );

        switch(*(aSqlcode))
        {
            case SQL_SUCCESS:
                *(aSqlcode) = SQL_ERROR;
                break;
            case ALTIBASE_FAILOVER_SUCCESS:
                /* BUG-47239 mFailoveredJustnow를 삭제하고 FAILOVER_SUCCESS가 
                 * 감지되는대로 ulpSetFailoverFlag을 통해서 mNeedReprepare를 바로 셋팅해준다. */
                (void) ulpSetFailoverFlag( aConnNode );
                *(aSqlcode) *= -1;
                break;
            default:
                *(aSqlcode) *= -1;
                break;
        }

        sRowCnt = 0;
        if ( aCLIType == ERR_TYPE1_DML )
        {
            sRc = SQLGetStmtAttr( aHstmt,
                                  ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT,
                                  &sRowCnt, 0, NULL);
            ACI_TEST_RAISE ( sRc != SQL_SUCCESS, ERR_GETERRINFO );
        }
        aSqlca->sqlerrd[2] = (acp_sint32_t) sRowCnt;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION (ERR_GETERRINFO);
    {
        aSqlca->sqlcode = SQL_ERROR;
        aSqlca->sqlerrm.sqlerrml = (short)acpCStrLen(aSqlca->sqlerrm.sqlerrmc,
                                                     ACP_SINT16_MAX);
        aSqlca->sqlerrd[2] = 0;
       /* aSqlca->sqlerrd[3] = 0;*/
    }
    ACI_EXCEPTION (ERR_INVALID_HANDLE);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Not_Exist_Error,
                         aConnNode->mConnName );

        aSqlca->sqlcode = SQL_INVALID_HANDLE;
        *(aSqlcode) = SQL_INVALID_HANDLE;

        acpCStrCpy( *aSqlstate,
                    MAX_ERRSTATE_LEN,
                    ulpGetErrorSTATE( &sErrorMgr ),
                    MAX_ERRSTATE_LEN-1 );

        acpCStrCpy( aSqlca->sqlerrm.sqlerrmc,
                    MAX_ERRMSG_LEN,
                    ulpGetErrorMSG(&sErrorMgr),
                    MAX_ERRMSG_LEN-1 );

        aSqlca->sqlerrm.sqlerrml = (short)acpCStrLen( aSqlca->sqlerrm.sqlerrmc,
                                                      ACP_SINT16_MAX );

        /*idlOS::strcpy(ses_sqlerrmsg, aSqlstmt->mSqlca->sqlerrm.sqlerrmc);*/
        aSqlca->sqlerrd[2] = 0;
        /*aSqlca->sqlerrd[3] = 0;*/
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulpGetConnErrorMsg( ulpLibConnNode *aConnNode, acp_char_t *aMsg )
{
/***********************************************************************
 *
 * Description :
 *    Connection 중 에러가 발생한 경우, 이 함수를 이용해 에러 메세지를 얻어온다.
 * 
 * Implementation :
 *
 ***********************************************************************/
    acp_char_t  sMsg[MAX_ERRMSG_LEN];
    SQLCHAR     sState[MAX_ERRSTATE_LEN];
    SQLINTEGER  sCode;
    SQLRETURN   sRes;
    SQLSMALLINT sMsglen;

    sRes = SQLError( aConnNode->mHenv,
                     aConnNode->mHdbc,
                     aConnNode->mHstmt,
                     sState,
                     &sCode,
                     (SQLCHAR *) sMsg,
                     MAX_ERRMSG_LEN,
                     &sMsglen );

    ACI_TEST( (sRes==SQL_ERROR) || (sRes==SQL_INVALID_HANDLE) );

    acpCStrCat( aMsg,
                MAX_ERRMSG_LEN-1,
                sMsg,
                acpCStrLen(sMsg, MAX_ERRMSG_LEN) );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

