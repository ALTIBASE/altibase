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

#include <xa.h>
#include <uln.h>
#include <ulnPrivate.h>
#include <ulxXaFunction.h>
#include <ulxXaConnection.h>
#include <sqlcli.h>
#include <ulxXaProtocol.h>
#include <ulxMsgLog.h>

#define XA_RETURNCODE_STR(d) XA_RETURNSTR[(d)+9]

const acp_char_t *XA_RETURNSTR[] =
{
    "XAER_OUTSIDE", // -9
    "XAER_DUPID",   // -8
    "XAER_RMFAIL",  // -7
    "XAER_PROTO",   // -6
    "XAER_INVAL",   // -5
    "XAER_NOTA",    // -4
    "XAER_RMERR",   // -3
    "XAER_AYNC",    // -2
    "NONE",         // -1
    "XA_OK",        //  0
    "NONE",         //  1
    "NONE",         //  2
    "XA_RDONLY",    //  3
    "XA_RETRY",     //  4
    "XA_HEURMIX",   //  5
    "XA_HEURRB",    //  6
    "XA_HEURCOM",   //  7
    "XA_HEURHAZ",   //  8
    "XA_NOMIGRATE"  //  9
};
void traceErrorMessage(ulxXaConnection  *aConn,
                       const acp_char_t *aFuncName,
                       int               aRmid,
                       acp_sint32_t      aResult);

void ulxNullCallbackForSesConn(int     aRmid,
                               SQLHENV aEnv,
                               SQLHDBC aDbc)
{
    ACP_UNUSED(aRmid);
    ACP_UNUSED(aEnv);
    ACP_UNUSED(aDbc);

    // Nothing to do.
}

//BUG-26374 XA_CLOSE시 Client 자원 정리가 되지 않습니다.
void ulxNullCallbackForSesDisConn( )
{
    // Nothing to do.
}

ulxCallbackForSesConn    gCallbackForSesConn    = ulxNullCallbackForSesConn;
ulxCallbackForSesDisConn gCallbackForSesDisConn = ulxNullCallbackForSesDisConn;

void ulxSetCallbackSesConn( ulxCallbackForSesConn aFuncPtr )
{
    gCallbackForSesConn = aFuncPtr;
}

void ulxSetCallbackSesDisConn( ulxCallbackForSesDisConn aFuncPtr )
{
    gCallbackForSesDisConn = aFuncPtr;
}

int ulxXaOpen(char *aXa_info, int aRmid, long aFlags)
{
    //ulxConnection *sConnHead;
    ulxXaConnection *sConn;
    acp_sint32_t     sResult = XAER_RMERR;
    acp_char_t       sAppInfo[30];
    acp_sint32_t     sLen   = 0;
    acp_sint32_t     sState = 0;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);
    //PROJ-1645 UL,FailOver.
    ACI_TEST_RAISE(ulxAddConnection(aRmid, &sConn,aFlags) != ACI_SUCCESS,
                   parsingErr);
    sState = 1;

    ulxMsgLogParseOpenStr(&sConn->mLogObj,aXa_info);

    // get the env/dbc
    ACI_TEST_RAISE(ulnAllocHandle(SQL_HANDLE_ENV,
                                  NULL,
                                  (void**)&(sConn->mEnv))
                   != ACI_SUCCESS, err_alloc_env);
    sState = 2;

    ACI_TEST_RAISE(ulnAllocHandle(SQL_HANDLE_DBC,
                                  sConn->mEnv,
                                  (void**)&(sConn->mDbc))
                   != ACI_SUCCESS, err_alloc_dbc);
    sState = 3;
    //PROJ-1645 UL-FailOver에서 STF시에 필요하다.
    sConn->mDbc->mXaConnection = sConn;

    ACI_TEST(acpSnprintf(sAppInfo, 30, "RMID: %"ACI_INT32_FMT"", aRmid)
             != ACP_RC_SUCCESS);
    sLen = acpCStrLen(sAppInfo, 30);

    ACI_TEST_RAISE( ulnDbcSetAppInfo( sConn->mDbc, sAppInfo, sLen )
                    != ACI_SUCCESS, err_connect );

    // connect to server
    ACI_TEST_RAISE(ulnDriverConnect(sConn->mDbc,
                                    aXa_info,
                                    SQL_NTS,
                                    NULL,
                                    0,
                                    0)
             != ACI_SUCCESS, err_connect);
    sState = 4;

    ulxMsgLogSetXaName(&sConn->mLogObj, sConn->mDbc->mXaName);

    ACI_TEST_RAISE(ulxConnectionProtocol(sConn->mDbc,
                                         CMP_DB_XA_OPEN,
                                         aRmid,
                                         (acp_slong_t)aFlags,
                                         &sResult)
                   != ACI_SUCCESS, err_protocol);
    sConn->mDbc->mAttrAutoCommit = (acp_uint8_t)SQL_AUTOCOMMIT_OFF;
    sState = 5;

    if (sResult == XA_OK)
    {
        ulxConnSetConn(sConn);
        gCallbackForSesConn(aRmid, (SQLHENV)(sConn->mEnv), (SQLHDBC)(sConn->mDbc));
    }

    return sResult;

    ACI_EXCEPTION(err_alloc_env);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaOpen", "XAER_RMERR",
                    0, "memory alloc error");
        sResult = XAER_RMERR;
    }
    ACI_EXCEPTION(err_alloc_dbc);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaOpen", "XAER_RMERR",
                    0, "memory alloc error");
        sResult = XAER_RMERR;
    }
    ACI_EXCEPTION(err_connect);
    {
        ulxMsgLogSetXaName(&sConn->mLogObj, sConn->mDbc->mXaName);
        sResult = XAER_RMERR;
        traceErrorMessage(sConn,
                          "ulxXaOpen",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaOpen",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(parsingErr);
    {
        sResult = XAER_INVAL;
    }
    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;

    switch ( sState )
    {
        case 4:
            ulnDisconnect(sConn->mDbc);
        case 3:
            ulnFreeHandle(SQL_HANDLE_DBC, sConn->mDbc);
        case 2:
            ulnFreeHandle(SQL_HANDLE_ENV, sConn->mEnv);
        case 1:
            ulxDeleteConnection(aRmid);
        default:
            break;
    }

    return sResult;

}

int ulxXaClose(char *aXa_info, int aRmid, long aFlags)
{
    ulxXaConnection *sConn = NULL;
    acp_sint32_t     sResult = XAER_RMERR;

    ACP_UNUSED(aXa_info);

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, errAsync);

    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);

    ACI_TEST_RAISE(ulxConnGetStatus(sConn)
                   == ULX_XA_ACTIVE, xaer_proto);

    ACI_TEST_RAISE(ulxConnectionProtocol(sConn->mDbc,
                                         CMP_DB_XA_CLOSE,
                                         aRmid,
                                         (acp_slong_t)aFlags,
                                         &sResult)
             != ACI_SUCCESS, err_protocol);

    //BUG-26374 XA_CLOSE시 Client 자원 정리가 되지 않습니다.
    gCallbackForSesDisConn();

    ulxConnSetDisconn(sConn);

    ACI_TEST_RAISE(ulnDisconnect(sConn->mDbc)
                   != ACI_SUCCESS, err_disconn);
    ACI_TEST_RAISE(ulnFreeHandle(SQL_HANDLE_DBC, sConn->mDbc)
                   != ACI_SUCCESS, err_freeDbc);
    ACI_TEST_RAISE(ulnFreeHandle(SQL_HANDLE_ENV, sConn->mEnv)
                   != ACI_SUCCESS, err_freeEnv);
    ACI_TEST_RAISE(ulxDeleteConnection(aRmid)
                   != ACI_SUCCESS, err_freecon);
    sConn = NULL;

    return sResult;

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(errAsync);
    {
        sResult = XAER_ASYNC;
    }
    ACI_EXCEPTION(err_disconn);
    {
        sResult = XAER_RMERR;
        traceErrorMessage(sConn,
                          "ulxXaClose",
                          aRmid,
                          sResult);
        ulxConnSetDisconn(sConn);
    }
    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaClose",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(xaer_proto);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaClose", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_freeEnv);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaClose", "XAER_RMERR",
                    0, "memory free error");
        sResult = XAER_RMERR;
    }
    ACI_EXCEPTION(err_freeDbc);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaClose", "XAER_RMERR",
                    0, "memory free error");
        sResult = XAER_RMERR;
    }
    ACI_EXCEPTION(err_freecon);
    {
/*
        이미 sConn 이 삭제되었을 수도 있음
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaClose", "XAER_RMERR",
                    "memory free error");
*/
        sResult = XAER_RMERR;
    }
    ACI_EXCEPTION_END;

    if (sConn != NULL)
    {
        ulxConnSetDisconn(sConn);
    }

    return sResult;
}

int ulxXaFree(char *aXa_info, int aRmid, long aFlags)
{
    ulxXaConnection *sConn = NULL;
    acp_sint32_t     sResult = XAER_RMERR;

    ACP_UNUSED(aXa_info);

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, errAsync);

    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
                   != ACI_SUCCESS, err_find_conn);

    //BUG-26374 XA_CLOSE시 Client 자원 정리가 되지 않습니다.
    gCallbackForSesDisConn();

    ulxConnSetDisconn(sConn);

    (void) ulnDisconnectLocal(sConn->mDbc);
    (void) ulnFreeHandle(SQL_HANDLE_DBC, sConn->mDbc);
    (void) ulnFreeHandle(SQL_HANDLE_ENV, sConn->mEnv);
    (void) ulxDeleteConnection(aRmid);
    sConn = NULL;

    sResult = XA_OK;

    return sResult;

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(errAsync);
    {
        sResult = XAER_ASYNC;
    }
    ACI_EXCEPTION_END;

    if (sConn != NULL)
    {
        ulxConnSetDisconn(sConn);
    }
    else
    {
        /* Nothing to do */
    }

    return sResult;
}

ACI_RC checkXID(XID *aXid)
{
    acp_sint64_t sLen;

    sLen = aXid->gtrid_length + aXid->bqual_length;

    ACI_TEST(sLen > XIDDATASIZE);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

int ulxXaCommit(XID *aXid, int aRmid, long aFlags)
{
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);
    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);
    ACI_TEST_RAISE(checkXID(aXid) != ACI_SUCCESS, err_xid);

    ACI_TEST_RAISE(ulxConnGetStatus(sConn) == ULX_XA_DISCONN,
                   err_state);

    ACI_TEST_RAISE(ulxTransactionProtocol(sConn->mDbc,
                                          CMP_DB_XA_COMMIT,
                                          aRmid,
                                          (acp_slong_t)aFlags,
                                          (XID*)aXid,
                                          &sResult)
                   != ACI_SUCCESS, err_protocol);
    return sResult;

    ACI_EXCEPTION(err_xid);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaCommit", "XAER_NOTA",
                    0, "the specified XID does not exist");
        sResult = XAER_NOTA;
    }

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaCommit", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }

    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaCommit",
                          aRmid,
                          sResult);
    }

    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;
    return sResult;

}

acp_sint32_t ulxXaComplete(int  *aHandle,
                           int  *retval,
                           int   rmid,
                           long  aFlags)
{
    ACP_UNUSED(aHandle);
    ACP_UNUSED(retval);
    ACP_UNUSED(rmid);
    ACP_UNUSED(aFlags);

/*
    ulxLogTrace(&(sConn->mLogObj), ULX_TRC_I_XA_COMPLETE, aRmid, "XAER_ASYNC",
                "Altibase does not support TMUSEASYNC");
*/
    return XAER_ASYNC;
}

int ulxXaEnd(XID *aXid, int aRmid, long aFlags)
{
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);

    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);
    //fix BUG-22561.
    ACI_TEST_RAISE(ulxConnGetStatus(sConn) < ULX_XA_CONN,
                   err_state);

    ACI_TEST_RAISE(ulxTransactionProtocol(sConn->mDbc,
                                          CMP_DB_XA_END,
                                          aRmid,
                                          (acp_slong_t)aFlags,
                                          (XID*)aXid,
                                          &sResult)
                   != ACI_SUCCESS, err_protocol);

    if (sResult == XA_OK)
    {
        ulxConnSetConn(sConn);
    }

    return sResult;

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaEnd", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }

    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaEnd",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;
    return sResult;
}

int ulxXaForget(XID *aXid, int aRmid, long aFlags)
{
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);
    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);
    ACI_TEST_RAISE(checkXID(aXid) != ACI_SUCCESS, err_xid);

    ACI_TEST_RAISE(ulxConnGetStatus(sConn) == ULX_XA_DISCONN,
                   err_state);

    ACI_TEST_RAISE(ulxTransactionProtocol(sConn->mDbc,
                                          CMP_DB_XA_FORGET,
                                          aRmid,
                                          (acp_slong_t)aFlags,
                                          (XID*)aXid,
                                          &sResult)
                   != ACI_SUCCESS, err_protocol);
    return sResult;

    ACI_EXCEPTION(err_xid);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaForget", "XAER_NOTA",
                    0, "the specified XID does not exist");
        sResult = XAER_NOTA;
    }

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaForget", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }

    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaForget",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;
    return sResult;
}

int ulxXaPrepare(XID *aXid, int aRmid, long aFlags)
{
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);
    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);

    ACI_TEST_RAISE(ulxConnGetStatus(sConn) == ULX_XA_DISCONN,
                   err_state);
    ACI_TEST_RAISE(ulxTransactionProtocol(sConn->mDbc,
                                          CMP_DB_XA_PREPARE,
                                          aRmid,
                                          (acp_slong_t)aFlags,
                                          (XID*)aXid,
                                          &sResult)
                   != ACI_SUCCESS, err_protocol);
    return sResult;


    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaPrepare", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }

    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaPrepare",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;
    return sResult;
}

int ulxXaRecover(XID *aXid, long aCount, int aRmid, long aFlags)
{
    acp_sint32_t     i;
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);
    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);

    ACI_TEST_RAISE( (aXid == NULL) && (aCount > 0), err_invalid );
    ACI_TEST_RAISE( aCount < 0, err_invalid );

    ACI_TEST_RAISE(ulxConnGetStatus(sConn) == ULX_XA_DISCONN,
                   err_state);

    // START 를 하기 전에는 TMNOFLAGS 를 사용할 수 없다.
    ACI_TEST_RAISE( (aFlags == TMNOFLAGS) && (sConn->mRecoverPos < 0),
                    err_invalid_start );

    /* TMSTARTRSCAN 일 때, 서버로부터 모두 받아와서 sConn->mRecoverXid 에
       저장한다. 이 후 TMNOFLAGS 로 xa_recover 가 호출되면 현재 위치(mRecoverPos)
       로부터 aCount 만큼 반환한다.  */
    if ( (aFlags & TMSTARTRSCAN) != 0 )
    {
        ulxConnInitRecover(sConn);
        ACI_TEST_RAISE(ulxRecoverProtocol(sConn->mDbc,
                                          CMP_DB_XA_RECOVER,
                                          aRmid,
                                          (acp_slong_t)aFlags,
                                          (XID**)&(sConn->mRecoverXid),
                                          aCount,
                                          &(sConn->mRecoverCnt))
                       != ACI_SUCCESS, err_protocol);
        sConn->mRecoverPos = 0;
    }

    for (i=0; (sConn->mRecoverPos<sConn->mRecoverCnt) && (i<aCount); sConn->mRecoverPos++,i++)
    {
        acpMemCpy(&aXid[i], &(sConn->mRecoverXid[sConn->mRecoverPos]), ACI_SIZEOF(XID));
    }
    sResult = i;

    if ( (aFlags & TMENDRSCAN) != 0 )  /* BUG-45777 */
    {
        ulxConnInitRecover(sConn);
    }
    else
    {
        /* Nothing to do */
    }
    return sResult;

    ACI_EXCEPTION(err_invalid);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaRecover", "XAER_INVAL",
                    0, "invalid options");
        sResult = XAER_INVAL;
    }
    ACI_EXCEPTION(err_invalid_start);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaRecover", "XAER_INVAL",
                    0, "TMSTARTRSCAN not set");
        sResult = XAER_INVAL;
    }

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaRecover", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }

    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaRecover",
                          aRmid,
                          sResult);
//        sResult = XAER_RMERR;
        ulxConnInitRecover(sConn);
//        ulxConnSetDisconn(sConn);
    }

    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;
    return sResult;

}

int ulxXaRollback(XID *aXid, int aRmid, long aFlags)
{
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);
    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);
    ACI_TEST_RAISE(checkXID(aXid) != ACI_SUCCESS, err_xid);

    ACI_TEST_RAISE(ulxConnGetStatus(sConn) == ULX_XA_DISCONN,
                   err_state);
    ACI_TEST_RAISE(ulxTransactionProtocol(sConn->mDbc,
                                         CMP_DB_XA_ROLLBACK,
                                         aRmid,
                                         (acp_slong_t)aFlags,
                                         (XID*)aXid,
                                         &sResult)
                   != ACI_SUCCESS, err_protocol);
    return sResult;

    ACI_EXCEPTION(err_xid);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaRollback", "XAER_NOTA",
                    0, "the specified XID does not exist");
        sResult = XAER_NOTA;
    }

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaRollback", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }

    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaRollback",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;
    return sResult;
}

int ulxXaStart(XID *aXid, int aRmid, long aFlags)
{
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);
    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
             != ACI_SUCCESS, err_find_conn);

    ACI_TEST_RAISE(ulxConnGetStatus(sConn) == ULX_XA_DISCONN,
                   err_state);

    ACI_TEST_RAISE(ulxTransactionProtocol(sConn->mDbc,
                                         CMP_DB_XA_START,
                                         aRmid,
                                         (acp_slong_t)aFlags,
                                         (XID*)aXid,
                                         &sResult)
                   != ACI_SUCCESS, err_protocol);

    if (sResult == XA_OK)
    {
        ulxConnSetActive(sConn);
    }

    return sResult;


    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        ulxLogTrace(&(sConn->mLogObj), aRmid, "ulxXaStart", "XAER_PROTO",
                    0, "state transition error");
        sResult = XAER_PROTO;
    }

    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaStart",
                          aRmid,
                          sResult);
    }

    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;
    return sResult;

}

acp_sint32_t ulxXaHeuristicCompleted(XID         *aXid,
                                    acp_sint32_t  aRmid,
                                    acp_slong_t   aFlags)
{
    acp_sint32_t     sResult = XAER_RMERR;
    ulxXaConnection *sConn = NULL;

    ACI_TEST_RAISE((aFlags & TMASYNC) != 0, asyncError);

    ACI_TEST_RAISE(ulxFindConnection(aRmid, &sConn)
                   != ACI_SUCCESS, err_find_conn);

    ACI_TEST_RAISE(ulxConnGetStatus(sConn) == ULX_XA_DISCONN, err_state);

    ACI_TEST_RAISE(ulxTransactionProtocol(sConn->mDbc,
                                          CMP_DB_XA_HEURISTIC_COMPLETED,
                                          aRmid,
                                          aFlags,
                                          (XID*)aXid,
                                          &sResult)
                   != ACI_SUCCESS, err_protocol);

    return sResult;

    ACI_EXCEPTION(err_find_conn);
    {
        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_state);
    {
        (void)ulxLogTrace(&(sConn->mLogObj),
                          aRmid,
                          "ulxHeuristicCompleted",
                          "XAER_PROTO",
                          0,
                          "state transition error");

        sResult = XAER_PROTO;
    }
    ACI_EXCEPTION(err_protocol);
    {
        traceErrorMessage(sConn,
                          "ulxXaHeuristicCompleted",
                          aRmid,
                          sResult);
    }
    ACI_EXCEPTION(asyncError);
    {
        sResult = XAER_ASYNC;
    }

    ACI_EXCEPTION_END;

    return sResult;
}

void traceErrorMessage(ulxXaConnection  *aConn,
                       const acp_char_t *aFuncName,
                       int               aRmid,
                       acp_sint32_t      aResult)
{
    SQLRETURN    sSqlRC;
    SQLSMALLINT  sTextLength;
    acp_sint32_t sErrorCode;
    acp_char_t   sErrorMessage[2048+256];

    sSqlRC = ulnGetDiagRec(SQL_HANDLE_DBC,
                           (ulnObject *)aConn->mDbc,
                           1,
                           NULL,
                           (acp_sint32_t *)(&sErrorCode),
                           (acp_char_t *)(sErrorMessage),
                           (acp_sint16_t)ACI_SIZEOF(sErrorMessage),
                           &sTextLength,
                           ACP_FALSE);

    if (sSqlRC == SQL_SUCCESS || sSqlRC == SQL_SUCCESS_WITH_INFO )
    {
        ulxLogTrace(&(aConn->mLogObj), aRmid, aFuncName, XA_RETURNCODE_STR(aResult),
                    sErrorCode, sErrorMessage);
    }
    else
    {
        ulxLogTrace(&(aConn->mLogObj), aRmid, aFuncName, XA_RETURNCODE_STR(aResult),
                    0, "protocol error");
    }
}
