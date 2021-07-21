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
#include <ulpLibInterFuncB.h>

/* XA관련 핸들이 초기화 됐는지를 판단하기위해 필요함. */
extern acp_bool_t gUlpLibDoInitProc;

/********************************************************************************
 *  Description :
 *     select와 같은 데이터 조회시 데이터를 저장할 host variable를 bind한다.
 *     dynamic,  static 방식에 따라 분기된다.
 *
 *  Parameters : 공통적으로 다음 인자를 갖으며, 함수마다 몇몇 인자가 추가/제거될 수 있다.
 *     ulpLibConnNode *aConnNode   : env핸들, dbc핸들 그리고 에러 처리를 위한 인자.
 *     ulpLibStmtNode *aStmtNode   : statement 관련 정보를 저장한다.
 *     SQLHSTMT    *aHstmt         : CLI호출을 위한 statement 핸들.
 *     acp_bool_t   aIsReBindCheck : rebind 수행 여부.
 *     ulpSqlca    *aSqlca         : 에러 처리를 위한 인자.
 *     ulpSqlcode  *aSqlcode       : 에러 처리를 위한 인자.
 *     ulpSqlstate *aSqlstate      : 에러 처리를 위한 인자.
 * ******************************************************************************/
ACI_RC ulpBindCol(ulpLibConnNode *aConnNode,    
                  ulpLibStmtNode *aStmtNode,  
                  SQLHSTMT    *aHstmt,        
                  ulpSqlstmt  *aSqlstmt,      
                  acp_bool_t   aIsReBindCheck,
                  ulpSqlca    *aSqlca,        
                  ulpSqlcode  *aSqlcode,      
                  ulpSqlstate *aSqlstate )    

{
    SQLDA *sBinda = NULL;
    /* BUG-41010 dynamic binding */
    if ( IS_DYNAMIC_VARIABLE(aSqlstmt) )
    {
        sBinda = (SQLDA*) aSqlstmt->hostvalue[0].mHostVar;

        ACI_TEST( ulpDynamicBindCol( aConnNode,
                                     aStmtNode,
                                     aHstmt,
                                     aSqlstmt,
                                     sBinda,
                                     aSqlca,
                                     aSqlcode,
                                     aSqlstate )
                  == ACI_FAILURE );

        ACI_TEST( ulpSetStmtAttrDynamicRowCore( aConnNode,
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
            if ( ulpCheckNeedReBindColCore( aStmtNode, aSqlstmt ) == ACP_TRUE )
            {
                ACI_TEST( ulpBindHostVarCore( aConnNode,
                                              aStmtNode,
                                              aHstmt,
                                              aSqlstmt,
                                              aSqlca,
                                              aSqlcode,
                                              aSqlstate )
                          == ACI_FAILURE );

                if ( ulpCheckNeedReSetStmtAttrCore( aStmtNode, aSqlstmt ) == ACP_TRUE )
                {
                    ACI_TEST( ulpSetStmtAttrRowCore( aConnNode,
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

            ACI_TEST( ulpSetStmtAttrRowCore( aConnNode,
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

ACI_RC ulpOpen ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    선언된 커서 open 처리 담당함.
 *
 *    처리구문> EXEC SQL OPEN <cursor_name> [ USING <in_host_var_list> ];
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;

    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    sStmtNode = ulpLibStLookupCur( &( sConnNode->mCursorHashT )
                                   , aSqlstmt->curname );

    ACI_TEST_RAISE( sStmtNode == NULL, ERR_NO_CURSOR );

    ulpSetColRowSizeCore( aSqlstmt );

    if( sStmtNode->mCurState >= C_DECLARE )
    {
        switch ( sStmtNode->mStmtState )
        {
            case S_PREPARE:
            case S_CLOSE:
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
            case S_BINDING:
            case S_SETSTMTATTR:
            case S_EXECUTE:
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
                else
                {
                    /* BUG-43716 편의를 위해 close 없이 다시 open하는걸 허용 */
                    (void) ulpCloseStmtCore(sConnNode,
                                            sStmtNode,
                                            &(sStmtNode->mHstmt),
                                            aSqlstmt->sqlcaerr,
                                            aSqlstmt->sqlcodeerr,
                                            aSqlstmt->sqlstateerr);
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
            default: /*S_INITIAL*/
                /* PREPARE를 먼저 해야한다.*/
                ACI_RAISE( ERR_STMT_NEED_PREPARE_4EXEC );
                break;
        }
    }
    else
    {
        ACI_RAISE( ERR_CUR_NEED_DECL_4OPEN );
    }

    ACI_TEST( ulpExecuteCore( sConnNode,
                              sStmtNode,
                              aSqlstmt,
                              &(sStmtNode->mHstmt) ) == ACI_FAILURE );

    sStmtNode -> mStmtState = S_EXECUTE;
    sStmtNode -> mCurState  = C_OPEN;

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
    ACI_EXCEPTION ( ERR_NO_CURSOR );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Cursor_Not_Exist_Error,
                         aSqlstmt->curname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_CURSOR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_STMT_NEED_PREPARE_4EXEC );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Need_Prepare_4Execute_Error,
                         sStmtNode->mStmtName );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_CUR_NEED_DECL_4OPEN );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Cursor_Need_Declare_4Open_Error,
                         aSqlstmt->curname );

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

ACI_RC ulpFetch ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    Open된 커서에 대한 fetch 처리 담당함.
 *
 *    처리구문> FETCH [ FIRST| PRIOR|NEXT|LAST|CURRENT | RELATIVE fetch_offset
 *            | ABSOLUTE fetch_offset ] <cursor_name> INTO <host_variable_list>;
 *
 * Implementation :
 *
 ***********************************************************************/
    acp_sint32_t    i;
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    SQLRETURN       sSqlRes = SQL_ERROR;
    SQLUINTEGER     sNumFetched;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    sStmtNode = ulpLibStLookupCur( &( sConnNode->mCursorHashT )
                                   , aSqlstmt->curname );

    ACI_TEST_RAISE( sStmtNode == NULL, ERR_NO_CURSOR );

    ulpSetColRowSizeCore( aSqlstmt );

    /* FOR절 처리*/
    ACI_TEST( ulpAdjustArraySize(aSqlstmt) == ACI_FAILURE );

    if( (sStmtNode->mCurState == C_OPEN) || (sStmtNode->mCurState == C_FETCH) )
    {
        switch ( sStmtNode->mStmtState )
        {
            case S_EXECUTE:
            case S_SETSTMTATTR:
            case S_BINDING:
                /*host 변수가 있으면 binding & setstmt해준다.*/
                if( aSqlstmt->numofhostvar > 0 )
                {
                    /* BUG-41010 dynamic binding */
                    ACI_TEST( ulpBindCol(sConnNode,    
                                         sStmtNode,  
                                         &(sStmtNode->mHstmt),        
                                         aSqlstmt,      
                                         ACP_TRUE,
                                         aSqlstmt->sqlcaerr,        
                                         aSqlstmt->sqlcodeerr,      
                                         aSqlstmt->sqlstateerr )
                              == ACI_FAILURE );
                }
                break;
            default: 
                /* S_INITIAL, S_PREPARE, S_INITIAL*/
                /* EXECUTE를 먼저 해야한다.*/
                ACI_RAISE( ERR_STMT_NEED_EXECUTE_4FETCH );
                break;
        }
    }
    else
    {
        ACI_RAISE( ERR_CUR_NEED_OPEN_4FETCH );
    }

    if ( sStmtNode -> mCurState  != C_FETCH )
    {
        sSqlRes = SQLSetStmtAttr( sStmtNode->mHstmt,
                                  SQL_ATTR_ROWS_FETCHED_PTR,
                                  (SQLPOINTER) &(sStmtNode->mRowsFetched),
                                  0 );
        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_SETSTMT );
    }

    /* BUG-45448 FETCH에서도 FOR구문이 적용되도록 SQL_ATTR_ROW_ARRAY_SIZE 매번 셋팅한다 */
    if(aSqlstmt->arrsize > 0)
    {
        sSqlRes = SQLSetStmtAttr( sStmtNode->mHstmt,
                                  SQL_ATTR_ROW_ARRAY_SIZE,
                                  (SQLPOINTER)(acp_slong_t)aSqlstmt->arrsize,
                                  0 );
    }

    switch( aSqlstmt -> scrollcur )
    {
        case F_NONE:
            sSqlRes = SQLFetch( sStmtNode->mHstmt );
            break;
        case F_FIRST:
            sSqlRes = SQLFetchScroll( sStmtNode->mHstmt, SQL_FETCH_FIRST, 0 );
            break;
        case F_PRIOR:
            sSqlRes = SQLFetchScroll( sStmtNode->mHstmt, SQL_FETCH_PRIOR, 0 );
            break;
        case F_NEXT:
            sSqlRes = SQLFetchScroll( sStmtNode->mHstmt, SQL_FETCH_NEXT, 0 );
            break;
        case F_LAST:
            sSqlRes = SQLFetchScroll( sStmtNode->mHstmt, SQL_FETCH_LAST, 0 );
            break;
        case F_CURRENT:
            sSqlRes = SQLFetchScroll( sStmtNode->mHstmt, SQL_FETCH_RELATIVE, 0 );
            break;
        case F_RELATIVE:
            sSqlRes = SQLFetchScroll( sStmtNode->mHstmt, SQL_FETCH_RELATIVE
                                      , aSqlstmt -> sqlinfo );
            break;
        case F_ABSOLUTE:
            sSqlRes = SQLFetchScroll( sStmtNode->mHstmt, SQL_FETCH_ABSOLUTE
                                      , aSqlstmt -> sqlinfo );
            break;
        default:
            ACE_ASSERT(0);
            break;
    }

    /* varchar 를 호스트 변수로 사용하며 사용자가 indicator를 명시 해줬을경우,*/
    /* fetch 수행후 indicator결과 값을 varchar.len 에 복사해 줘야함.*/
    for( i = 0 ; aSqlstmt->numofhostvar > i ; i++ )
    {
        if( ((aSqlstmt->hostvalue[i].mType == H_VARCHAR) ||
            (aSqlstmt->hostvalue[i].mType  == H_NVARCHAR)) &&
            (aSqlstmt->hostvalue[i].mVcInd != NULL) )
        {
            *(aSqlstmt->hostvalue[i].mVcInd) = *(aSqlstmt->hostvalue[i].mHostInd);
        }
    }

    ulpSetErrorInfo4CLI( sConnNode,
                         sStmtNode->mHstmt,
                         sSqlRes,
                         aSqlstmt->sqlcaerr,
                         aSqlstmt->sqlcodeerr,
                         aSqlstmt->sqlstateerr,
                         ERR_TYPE1 );

    /*---------------------------------------------------------------*/
    /* if "22002" error(null fetched without indicator) occured and unsafe_null is true,*/
    /*  we allow null fetch without indicator and return SQL_SUCCESS*/
    /*---------------------------------------------------------------*/
    if( (acpCStrCmp( *(aSqlstmt->sqlstateerr), SQLSTATE_UNSAFE_NULL,
                     MAX_ERRSTATE_LEN-1 ) == 0) &&
        (ulpGetStmtOption( aSqlstmt,
                           ULP_OPT_UNSAFE_NULL ) == ACP_TRUE) )
    {
        sSqlRes = SQL_SUCCESS;
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               SQLMSG_SUCCESS,
                               sSqlRes,
                               SQLSTATE_SUCCESS );
    }

    sNumFetched = sStmtNode->mRowsFetched;
    aSqlstmt->sqlcaerr->sqlerrd[2] = sNumFetched;

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_FETCH );

    /* 커서 상태를 FETCH로 바꿈;*/
    sStmtNode -> mCurState  = C_FETCH;

    return ACI_SUCCESS;

    /* fetch된 row수를 sqlca->sqlerrd[2] 에 저장해야하는데 어디서 처리해야하나? */

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
    ACI_EXCEPTION ( ERR_NO_CURSOR );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Cursor_Not_Exist_Error,
                         aSqlstmt->curname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_CURSOR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_STMT_NEED_EXECUTE_4FETCH );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Stmt_Need_Execute_4Fetch_Error,
                         sStmtNode->mStmtName );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_CUR_NEED_OPEN_4FETCH );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Cursor_Need_Open_4Fetch_Error,
                         aSqlstmt->curname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( ERR_CLI_SETSTMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             sStmtNode->mHstmt,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION ( ERR_CLI_FETCH );
    {

    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpClose ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    open된 커서에 대한 close처리 담당함. statement를 drop하지 않고 close한다. 재활용 가능.
 *
 *    처리구문> EXEC SQL CLOSE <cursor_name>;
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    sStmtNode = ulpLibStLookupCur( &( sConnNode->mCursorHashT )
                                   , aSqlstmt->curname );

    ACI_TEST_RAISE( sStmtNode == NULL, ERR_NO_CURSOR );

    ACI_TEST( ulpCloseStmtCore(    sConnNode,
                                   sStmtNode,
                                   &(sStmtNode-> mHstmt),
                                   aSqlstmt -> sqlcaerr,
                                   aSqlstmt -> sqlcodeerr,
                                   aSqlstmt -> sqlstateerr )
              == ACI_FAILURE );

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
    ACI_EXCEPTION ( ERR_NO_CURSOR );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Cursor_Not_Exist_Error,
                         aSqlstmt->curname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_CURSOR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpCloseRelease ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    open된 커서에 대한 release처리 담당함. statement를 drop하며, 해당 cursor hash node를
 *    제거한다.
 *
 *    처리구문> EXEC SQL CLOSE RELEASE <cursor_name>;
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    SQLRETURN       sSqlRes;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    sStmtNode = ulpLibStLookupCur( &( sConnNode->mCursorHashT )
                                   , aSqlstmt->curname );

    ACI_TEST_RAISE( sStmtNode == NULL, ERR_NO_CURSOR );

    if( sStmtNode->mStmtName[0] == '\0' )
    {
        sSqlRes = SQLFreeStmt( sStmtNode->mHstmt, SQL_DROP );
        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_FREE_STMT );
    }
    else
    {
        /* stmt 이름이 명시 되어 있으면 나중에 사용될 수 있기때문에 close만 한다.*/
        ACI_TEST( ulpCloseStmtCore( sConnNode,
                                    sStmtNode,
                                    &(sStmtNode-> mHstmt),
                                    aSqlstmt -> sqlcaerr,
                                    aSqlstmt -> sqlcodeerr,
                                    aSqlstmt -> sqlstateerr )
                  == ACI_FAILURE );
    }

    /* cursor hash table 에서 해당 cursor node를 제거한다. ( link만 제거 or node자체를 제거 )*/
    ulpLibStDeleteCur( &(sConnNode->mCursorHashT), aSqlstmt->curname );

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
    ACI_EXCEPTION ( ERR_NO_CURSOR );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Cursor_Not_Exist_Error,
                         aSqlstmt->curname );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_NO_CURSOR,
                               ulpGetErrorSTATE(&sErrorMgr) );
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

    return ACI_FAILURE;
}


ACI_RC ulpDeclareStmt ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    새로운 statement를 선언처리 담당함. stmt hash table에 새 node 추가됨.
 *    statement가 이미 존재하면 아무런 처리를 하지 않는다.
 *
 *    처리구문> EXEC SQL DECLARE <statement_name> STATEMENT;
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    ulpLibStmtNode *sStmtNode;
    SQLRETURN       sSqlRes;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    /* 이미 해당 stmtname으로 stmt node가 존재하는지 찾음*/
    sStmtNode = ulpLibStLookupStmt( &( sConnNode->mStmtHashT ),
                                    aSqlstmt->stmtname );
    /* 존재하면 do nothing*/
    if( sStmtNode != NULL )
    {
        /* do nothing */
    }
    else
    {
        /* 새 stmt 노드 할당*/
        sStmtNode = ulpLibStNewNode(aSqlstmt, aSqlstmt->stmtname );
        ACI_TEST( sStmtNode == NULL);

        /* AllocStmt 수행.*/
        sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, &( sStmtNode -> mHstmt ) );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_ALLOC_STMT);

        /* stmt node를 hash table에 추가.*/
        ACI_TEST_RAISE ( ulpLibStAddStmt( &(sConnNode->mStmtHashT),
                                          sStmtNode ) == NULL,
                         ERR_STMT_ADDED_JUST_BEFORE );
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
    ACI_EXCEPTION ( ERR_STMT_ADDED_JUST_BEFORE );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Conflict_Two_Emsqls_Error );

        SQLFreeStmt( sStmtNode -> mHstmt, SQL_DROP );
        acpMemFree( sStmtNode );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpAutoCommit ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *
 *    처리구문> EXEC SQL AUTOCOMMIT { ON | OFF };
 *
 * Implementation :
 *
 ***********************************************************************/

    ulpLibConnNode *sConnNode;
    SQLRETURN       sSqlRes;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    if ( aSqlstmt->sqlinfo == 0 ) /* AUTOCOMMIT OFF*/
    {
        sSqlRes = SQLSetConnectAttr ( sConnNode->mHdbc, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0 );
    }
    else                          /* AUTOCOMMIT ON*/
    {
        sSqlRes = SQLSetConnectAttr ( sConnNode->mHdbc, SQL_ATTR_AUTOCOMMIT,
                                      (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0 );
    }

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_SETCONNATTR_4AUTOCOMMIT );

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
    ACI_EXCEPTION ( ERR_CLI_SETCONNATTR_4AUTOCOMMIT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             sSqlRes,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpCommit ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *
 *    처리구문> EXEC SQL <COMMIT|ROLLBACK> [WORK] [RELEASE];
 *
 * Implementation :
 *
 ***********************************************************************/

    ulpLibConnNode *sConnNode;
    SQLRETURN    sSqlRes;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    if ( aSqlstmt -> sqlinfo < 2 )
    {
        sSqlRes = SQLEndTran ( SQL_HANDLE_DBC, sConnNode->mHdbc, SQL_COMMIT );
        if ( aSqlstmt -> sqlinfo == 1 )
        {
            ulpDisconnect( aConnName, aSqlstmt, NULL );
        }
    }
    else
    {
        sSqlRes = SQLEndTran ( SQL_HANDLE_DBC, sConnNode->mHdbc, SQL_ROLLBACK );
        if ( aSqlstmt -> sqlinfo == 3 )
        {
            ulpDisconnect( aConnName, aSqlstmt, NULL );
        }
    }

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_ENDTRANS_4COMMIT );

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
    ACI_EXCEPTION ( ERR_CLI_ENDTRANS_4COMMIT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             sSqlRes,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpBatch ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    ODBC CLI함수 SQLSetConnectAttr 에서 SQL_ATTR_BATCH 를 지원하지 않고 있으다.
 *    호성을 위해 에러를 발생시키지 않고 경고 메세지만 보여준다.
 *
 *    처리구문> EXEC SQL BATCH { ON | OFF };
 *
 * Implementation :
 *
 ***********************************************************************/
    ACP_UNUSED(aConnName);
    ACP_UNUSED(reserved);
    /*경고 메세지를 sqlca에 세팅한다.*/
    ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                           aSqlstmt->sqlcodeerr,
                           aSqlstmt->sqlstateerr,
                           SQLMSG_BATCH_NOT_SUPPORTED,
                           SQL_SUCCESS_WITH_INFO,
                           SQLSTATE_SUCCESS );

    return ACI_SUCCESS;

}

ACI_RC ulpFree ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    데이터베이스 서버와의 연결 및 내장 SQL문 수행 시 할당받았던 자원을 모두 해제한다.
 *
 *    처리구문> EXEC SQL FREE;
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    SQLRETURN       sSqlRes;
    acp_bool_t      sIsDefaultConn;
    ACP_UNUSED(reserved);

    sIsDefaultConn = ACP_FALSE;

    if ( aConnName != NULL )
    {
        /* aConnName으로 Connection node(ConnNode)를 찾는다;*/
        sConnNode = ulpLibConLookupConn( aConnName );

        ACI_TEST_RAISE( sConnNode == NULL, ERR_NO_CONN );
    }
    else
    {
        sConnNode = ulpLibConGetDefaultConn();
        /* BUG-26359 valgrind bug */
        ACI_TEST_RAISE( sConnNode->mHenv == SQL_NULL_HENV, ERR_NO_CONN );
        sIsDefaultConn = ACP_TRUE;
    }

    sSqlRes = SQLFreeConnect( sConnNode->mHdbc );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_FREE_CONN );

    sConnNode->mHdbc = SQL_NULL_HDBC;

    sSqlRes = SQLFreeEnv( sConnNode->mHenv );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_FREE_ENV );

    sConnNode->mHenv = SQL_NULL_HENV;

    if( sIsDefaultConn == ACP_TRUE )
    {
        (void) ulpLibConInitDefaultConn();
    }
    else
    {
        ulpLibConDelConn( aConnName );
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
    ACI_EXCEPTION ( ERR_CLI_FREE_CONN );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );

        if( sIsDefaultConn != ACP_TRUE )
        {
            ulpLibConDelConn( aConnName );
        }
    }
    ACI_EXCEPTION ( ERR_CLI_FREE_ENV );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );

        if( sIsDefaultConn != ACP_TRUE )
        {
            ulpLibConDelConn( aConnName );
        }
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulpAlterSession ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    DATA FORMAT 관련 session property 설정함.
 *
 *    처리구문> EXEC SQL ALTER SESSION SET DATE_FORMAT = '...';
 *            EXEC SQL ALTER SESSION SET DEFAULT_DATE_FORMAT = '...';
 *
 * Implementation :
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    SQLPOINTER      sValue;
    SQLINTEGER      sLen;
    SQLRETURN       sSqlRes;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    sValue = (SQLPOINTER)( aSqlstmt->extrastr + 1 );
    sLen   = acpCStrLen( aSqlstmt->extrastr + 1, ACP_SINT32_MAX ) - 1;

    ACI_TEST_RAISE( (sSqlRes = SQLSetConnectAttr( sConnNode->mHdbc,
                                                  ALTIBASE_DATE_FORMAT,
                                                  sValue,
                                                  sLen ))
                    != SQL_SUCCESS, ERR_CLI_SETCONN );

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
    ACI_EXCEPTION( ERR_CLI_SETCONN );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             sSqlRes,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}



ACI_RC ulpFreeLob ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    해당 LOB locator와 관련된 자원들을 모두 해제해준다.
 *
 *    처리구문> EXEC SQL FREE LOB <hostvalue_name_list>;
 *
 * Implementation :
 *    SQLAllocStmt(...) -> SQLFreeLob(...)
 *
 ***********************************************************************/
    ulpLibConnNode *sConnNode;
    SQLHSTMT        sHstmt;
    SQLRETURN       sSqlRes;
    acp_uint32_t            sI;
    acp_uint32_t            sJ;
    acp_uint32_t            sArrSize;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    /* 1. alloc statement.*/
    sSqlRes = SQLAllocStmt( sConnNode -> mHdbc, &( sHstmt ) );

    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_ALLOC_STMT);

    /* 2. host value 각각에 대한 SQLFreeLob 호출.*/
    for (sI = 0; sI < aSqlstmt -> numofhostvar ; sI++)
    {
        if ( aSqlstmt -> hostvalue[ sI ].isarr == 0 )
        {
            sArrSize = 1;
        }
        else
        {
            /* FOR 절 고려*/
            if ( (aSqlstmt -> iters > 0) &&
                 (aSqlstmt -> iters < aSqlstmt -> hostvalue[ sI ].arrsize) )
            {
                sArrSize = aSqlstmt -> iters;
            }
            else
            {
                sArrSize = aSqlstmt -> hostvalue[ sI ].arrsize;
            }
        }

        for (sJ = 0; sJ < sArrSize; sJ++)
        {
            sSqlRes = SQLFreeLob( sHstmt,
                                  ((SQLUBIGINT *)aSqlstmt -> hostvalue[ sI ].mHostVar)[sJ] );

            ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                            , ERR_CLI_FREE_LOB );
        }
    }

    /* 3. statement free*/
    sSqlRes = SQLFreeStmt( sHstmt, SQL_DROP );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                    , ERR_CLI_FREE_STMT );


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
    ACI_EXCEPTION( ERR_CLI_FREE_LOB );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             sHstmt,
                             sSqlRes,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE3 );
    }
    ACI_EXCEPTION ( ERR_CLI_FREE_STMT );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             sHstmt,
                             SQL_ERROR,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;

}


ACI_RC ulpFailOver ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    fail over callback 설정함.
 *
 *    처리구문> EXEC SQL REGISTER [AT <conn_name>] FAIL_OVER_CALLBACK <:host_variable>;
 *            EXEC SQL UNREGISTER [AT <conn_name>] FAIL_OVER_CALLBACK ;
 *
 * Implementation :
 *       SQLSetConnectAttr(...ALTIBASE_FAILOVER_CALLBACK...)
 *
 ***********************************************************************/

    ulpLibConnNode *sConnNode;
    SQLRETURN       sSqlRes;
    ACP_UNUSED(reserved);

    ULPGETCONNECTION(aConnName,sConnNode);

    if ( aSqlstmt->sqlinfo != 0 )
    {
        ACI_TEST_RAISE( (sSqlRes = SQLSetConnectAttr(sConnNode->mHdbc,
                                        ALTIBASE_FAILOVER_CALLBACK,
                                        (SQLPOINTER)aSqlstmt->hostvalue[0].mHostVar,
                                        0))
                        != SQL_SUCCESS, ERR_CLI_SETCONN );
    }
    else
    {
        ACI_TEST_RAISE( (sSqlRes = SQLSetConnectAttr(sConnNode->mHdbc,
                                        ALTIBASE_FAILOVER_CALLBACK,
                                        (SQLPOINTER)NULL,
                                        0))
                        != SQL_SUCCESS, ERR_CLI_SETCONN );
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
    ACI_EXCEPTION( ERR_CLI_SETCONN );
    {
        ulpSetErrorInfo4CLI( sConnNode,
                             SQL_NULL_HSTMT,
                             sSqlRes,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulpAfterXAOpen ( acp_sint32_t aRmid,
                      SQLHENV      aEnv,
                      SQLHDBC      aDbc )
{
/***********************************************************************
 *
 * Description :
 *      XA 관련 처리함수.
 *
 * Implementation :
 *
 ***********************************************************************/

    ulpLibConnNode *sNode;
    ulpSqlca       *sSqlca;
    ulpSqlcode     *sSqlcode;
    ulpSqlstate    *sSqlstate;
    SQLRETURN       sSqlres;
    acp_char_t      sDBName[MAX_CONN_NAME_LEN + 1];

    SQLINTEGER      sInd;
    acp_bool_t      sAllocStmt;
    acp_bool_t      sIsLatched;

    /*fix BUG-25597 APRE에서 AIX플랫폼 턱시도 연동문제를 해결해야 합니다.
     APRE의 ulpConnMgr초기이전에 CLI의 XaOpen함수에 본 함수가 불린경우
     무시한다.
     나중에  APRE의 ulpConnMgr 초기화된  CLI로 부터  기 Open된 Xa Connection을
     로딩하도록 한다.
   */
    /* gUlpLibDoInitProc가 true라면 아직 XA 관련 핸들을 얻어오지 못한 상태이다.*/
    ACI_TEST(gUlpLibDoInitProc == ACP_TRUE);

    sSqlca    = ulpGetSqlca();
    sSqlcode  = ulpGetSqlcode();
    sSqlstate = ulpGetSqlstate();

    sSqlres = SQLGetConnectAttr( aDbc,
                                 ALTIBASE_XA_NAME,
                                 sDBName,
                                 MAX_CONN_NAME_LEN + 1,
                                 &sInd);

    ACI_TEST_RAISE ( (sSqlres == SQL_ERROR) || (sSqlres == SQL_INVALID_HANDLE),
                     ERR_CLI_GETCONNECT_ATTR );

    sAllocStmt = sIsLatched = ACP_FALSE;

    if ( sDBName[0] == '\0' )
    {
        /* set default connection*/
        sNode = ulpLibConGetDefaultConn();
        sAllocStmt = ACP_TRUE;
    }
    else
    {
        /* set connection with name*/
        sNode = ulpLibConLookupConn( sDBName );
        if (sNode == NULL)
        {
            sNode = ulpLibConNewNode( sDBName, ACP_TRUE );

            ACI_TEST_RAISE( sNode == NULL, ERR_CONN_NAME_OVERFLOW );

            ACI_TEST_RAISE( ulpLibConAddConn( sNode ) == NULL,
                            ERR_ALREADY_CONNECTED );

            sAllocStmt = ACP_TRUE;
        }
    }

    sNode->mHenv     = aEnv;
    sNode->mHdbc     = aDbc;
    sNode->mUser     = NULL;
    sNode->mPasswd   = NULL;
    sNode->mConnOpt1 = NULL;
    sNode->mConnOpt2 = NULL;
    sNode->mIsXa     = ACP_TRUE;
    sNode->mXaRMID   = aRmid;

    if( gUlpLibOption.mIsMultiThread == ACP_TRUE )
    {
        ACI_TEST_RAISE ( acpThrRwlockLockWrite( &(sNode->mLatch4Xa) )
                         != ACP_RC_SUCCESS, ERR_LATCH_WRITE );

        sIsLatched = ACP_TRUE;
    }

    ulpSetDateFmtCore( sNode );

    if( sIsLatched == ACP_TRUE )
    {
        ACI_TEST_RAISE ( acpThrRwlockUnlock ( &(sNode->mLatch4Xa) )
                         != ACP_RC_SUCCESS, ERR_LATCH_RELEASE );
    }

    if ( sAllocStmt == ACP_TRUE )
    {
        sSqlres = SQLAllocStmt( sNode->mHdbc,
                                &(sNode->mHstmt) );

        ulpSetErrorInfo4CLI( sNode,
                             sNode->mHstmt,
                             sSqlres,
                             sSqlca,
                             sSqlcode,
                             sSqlstate,
                             ERR_TYPE2 );

        if (sSqlres == SQL_ERROR)
        {
            sNode->mHenv  = SQL_NULL_HENV;
            sNode->mHdbc  = SQL_NULL_HDBC;
            sNode->mHstmt = SQL_NULL_HSTMT;
        }

    }

    ACI_EXCEPTION(ERR_CLI_GETCONNECT_ATTR);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_XA_GetConnectAttr_Error );
        ulpSetErrorInfo4PCOMP( sSqlca,
                               sSqlcode,
                               sSqlstate,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION(ERR_CONN_NAME_OVERFLOW);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Exceed_Max_Connname_Error,
                         sDBName );
        ulpSetErrorInfo4PCOMP( sSqlca,
                               sSqlcode,
                               sSqlstate,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_CONNAME_OVERFLOW,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION(ERR_ALREADY_CONNECTED);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Conn_Aleady_Exist_Error,
                         sDBName );

        ulpSetErrorInfo4PCOMP( sSqlca,
                               sSqlcode,
                               sSqlstate,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQLCODE_ALREADY_CONNECTED,
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION(ERR_LATCH_WRITE);
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
    ACI_EXCEPTION(ERR_LATCH_RELEASE);
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
}

void ulpAfterXAClose ( void )
{
    /* default connection node 초기화*/
    (void) ulpLibConInitDefaultConn();
}

/* TASK-7218 Handling Multiple Errors */
ACI_RC ulpGetStmtDiag ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    diagnostics information 설정함.
 *
 *    처리구문 : 
 *    >>---EXEC SQL---GET-.---------.-DIAGNOSTICS-->
 *                        +-CURRENT-+
 *
 *            +-----------,--------------+
 *            V                          |
 *    >-----.-.- :hvar1 = --.-ROW_COUNT--.->
 *                          |            |
 *                          +-NUMBER-----+
 *
 * Implementation :
 *       ulpSetErrorInfo4CLI()에서 구해둔 MultiError 정보를 반환한다.
 *       그러나 ulpSetErrorInfo4CLI()에서 에러 정보를 셋팅할 때 statement 핸들이
 *       NULL이었다면, 즉 dbc 에서 에러가 발생한 경우라면 MultiError는 셋팅되지
 *       않는다.
 *       하지만 sqlca, SQLCODE, SQLSTATE는 셋팅되므로 이 정보를 이용해서
 *       반환한다.
 *       ...
 *       dbc에서 에러가 발생했다면 ConnNode가 바로 삭제될 수 있기 때문에
 *       MultiError를 셋팅하는 것이 무의미하므로 셋팅하지 않도록 구현함.
 *
 ***********************************************************************/
    acp_sint16_t      i;
    ulpMultiErrorMgr *sMultiErrMgr;
    ulpHostVar       *sHostValue = NULL;

    ACP_UNUSED(aConnName);
    ACP_UNUSED(reserved);

    sHostValue = aSqlstmt->hostvalue;

    ACI_TEST( sqlca.sqlcode == SQL_SUCCESS );

    sMultiErrMgr = ulpLibGetMultiErrorMgr();

    /* multi-error가 셋팅되어 있지 않으면 sqlca, SQLCODE, SQLSTATE 값을 이용 */
    for (i = 0; i < aSqlstmt->numofhostvar; i++)
    {
        switch ( sHostValue[i].mDiagType )
        {
        case H_STMT_DIAG_NUMBER:
            *(SQLINTEGER *)(sHostValue[i].mHostVar) = ulpLibMultiErrorMgrGetNumber( sMultiErrMgr );
            break;
        case H_STMT_DIAG_ROW_COUNT:
            *(SQLINTEGER *)(sHostValue[i].mHostVar) = sqlca.sqlerrd[2];
            break;
        }
    }
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    for (i = 0; i < aSqlstmt->numofhostvar; i++)
    {
        *(SQLINTEGER *)(sHostValue[i].mHostVar) = 0;
    }

    return ACI_SUCCESS;
}

ACI_RC ulpGetConditionDiag ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved )
{
/***********************************************************************
 *
 * Description :
 *    diagnostics information 설정함.
 *
 *    처리구문:
 *    >>---EXEC SQL---GET-.---------.-DIAGNOSTICS-->            
 *                        +-CURRENT-+
 *
 *                                  +-----------------,------------------+
 *                                  V                                    |  
 *    >----CONDITION--.--:hvar2--.--.-:hvar3 = --.---RETURNED_SQLCODE----.->
 *                    +--integer-+               |                       |
 *                                               +---RETURNED_SQLSTATE---+
 *                                               |                       |
 *                                               +---MESSAGE_TEXT--------+
 *                                               |                       |
 *                                               +---ROW_NUMBER----------+
 *                                               |                       |
 *                                               +---COLUMN_NUMBER-------+
 *
 * Implementation :
 *       ulpSetErrorInfo4CLI()에서 SQLGetDiagRec()를 반복하면서 셋팅해둔
 *       MultiError 정보를 반환한다.
 *       그러나 ulpSetErrorInfo4CLI()에서 에러 정보를 셋팅할 때 statement 핸들이
 *       NULL이었다면, 즉 dbc 에서 에러가 발생한 경우라면 MultiError는 셋팅되지
 *       않는다.
 *       하지만 sqlca, SQLCODE, SQLSTATE는 셋팅되므로 이 정보를 이용해서
 *       반환한다.
 *       ...
 *       dbc에서 에러가 발생했다면 ConnNode가 바로 삭제될 수 있기 때문에
 *       MultiError를 셋팅하는 것이 무의미하므로 셋팅하지 않도록 구현함.
 *
 ***********************************************************************/
    acp_sint16_t        i;
    ulpMultiErrorMgr   *sMultiErrorMgr;
    ulpMultiErrorStack *sError     = NULL;
    ulpHostVar         *sHostValue = NULL;
    acp_sint32_t        sRecNum    = aSqlstmt->iters;

    ACP_UNUSED(aConnName);
    ACP_UNUSED(reserved);

    sHostValue = aSqlstmt->hostvalue;

    ACI_TEST( sqlca.sqlcode == SQL_SUCCESS );

    sMultiErrorMgr = ulpLibGetMultiErrorMgr();

    if ( sMultiErrorMgr == NULL ||
         sMultiErrorMgr->mErrorNum == 0 )
    {
        /* multi-error가 셋팅되어 있지 않다면 sqlca, SQLCODE, SQLSTATE 값을 이용할 것이다.
         * 따라서 sRecNum은 반드시 1이어야 한다. */
        ACI_TEST( sRecNum != 1 );
    }
    else
    {
        ACI_TEST( sRecNum < 1 || sRecNum > sMultiErrorMgr->mErrorNum );
        sError = &(ulpLibGetMultiErrorMgr()->mErrorStack[sRecNum-1]);
    }
     
    for (i = 0; i < aSqlstmt->numofhostvar; i++)
    {
        switch ( sHostValue[i].mDiagType )
        {
        case H_COND_DIAG_RETURNED_SQLCODE:
            *(SQLINTEGER *)(sHostValue[i].mHostVar) = ulpLibMultiErrorMgrGetSqlcode( sError );
            break;
        case H_COND_DIAG_RETURNED_SQLSTATE:
            ulpLibMultiErrorMgrGetSqlstate( sError,
                                         sHostValue[i].mHostVar,
                                         sHostValue[i].mMaxlen,
                                         sHostValue[i].mHostInd );
            break;
        case H_COND_DIAG_MESSAGE_TEXT:
            ulpLibMultiErrorMgrGetMessageText( sError,
                                            sHostValue[i].mHostVar,
                                            sHostValue[i].mMaxlen,
                                            sHostValue[i].mHostInd );
            break;
        case H_COND_DIAG_ROW_NUMBER:
            *(SQLINTEGER *)(sHostValue[i].mHostVar) = ulpLibMultiErrorMgrGetRowNumber( sError );
            break;
        case H_COND_DIAG_COLUMN_NUMBER:
            *(SQLINTEGER *)(sHostValue[i].mHostVar) = ulpLibMultiErrorMgrGetColumnNumber( sError );
            break;
        }
    }
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    for (i = 0; i < aSqlstmt->numofhostvar; i++)
    {
        switch ( sHostValue[i].mDiagType )
        {
        case H_COND_DIAG_RETURNED_SQLCODE:
        case H_COND_DIAG_ROW_NUMBER:
        case H_COND_DIAG_COLUMN_NUMBER:
            *(SQLINTEGER *)(sHostValue[i].mHostVar) = 0;
            break;
        case H_COND_DIAG_RETURNED_SQLSTATE:
        case H_COND_DIAG_MESSAGE_TEXT:
            ((acp_char_t *)(sHostValue[i].mHostVar))[0] = '\0';
            if  (sHostValue[i].mHostInd != NULL )
            {
                *(ulvSLen *)(sHostValue[i].mHostInd) = 0;
            }
            break;
        }
    }
    return ACI_SUCCESS;
}
