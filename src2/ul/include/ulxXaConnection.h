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

#ifndef _O_ULX_XA_CONNECTION_H_
#define _O_ULX_XA_CONNECTION_H_ 1

#include <ulo.h>
#include <uluLock.h>
#include <xa.h>
#include <ulxMsgLog.h>

typedef struct ulxXaConnection ulxXaConnection;
typedef struct ulxXaRmid       ulxXaRmid;

typedef enum ulxXaStatus
{
    ULX_XA_DISCONN, /* before XA_OPEN, after xa_close */
    ULX_XA_CONN,    /* after XA_OPEN, before XA_START
                       after XA_END */
    ULX_XA_ACTIVE   /* after XA_START */
} ulxXaStatus;


struct ulxXaConnection
{
    acp_sint32_t         mRmid;
    ulxXaStatus          mStatus;
    ulxMsgLog            mLogObj;
    ulnEnv              *mEnv;
    ulnDbc              *mDbc;
    acp_thr_mutex_t     *mLock;
    acp_sint32_t         mRecoverCnt;
    acp_sint32_t         mRecoverPos;
    XID                 *mRecoverXid;
    ulxXaConnection     *next;
    /* PROJ-1645 UL-FailOver */
    long                 mOpenFlag;
};

ulxXaConnection* ulxGetConnectionHeader();
  /* PROJ-1645 UL-FailOver */
ACI_RC ulxAddConnection(acp_sint32_t rmid, ulxXaConnection **aConnection,long aOpenFlag);

ACI_RC ulxDeleteConnection(acp_sint32_t rmid);

ACI_RC ulxFindConnection(acp_sint32_t rmid, ulxXaConnection **aConnection);

void ulxConnSetDisconn(ulxXaConnection *aConn);

void ulxConnSetConn(ulxXaConnection *aConn);

void ulxConnSetActive(ulxXaConnection *aConn);

ulxXaStatus ulxConnGetStatus(ulxXaConnection *aConn);

ACI_RC ulxGetFirstConnection(ulxXaConnection **aConnection);

void ulxConnInitRecover(ulxXaConnection *aConn);

//fix BUG-25597 APRE에서 AIX플랫폼 턱시도 연동문제를 해결해야 합니다.
// APRE의 ulConnMgr 초기화전에 이미 생성된 CLI 의 XA Connection들을
// Loading하는 함수이다.

void  ulxXaRegisterOpenedConnections2APRE();
extern ulxCallbackForSesConn    gCallbackForSesConn;

#endif /* _O_ULX_XA_CONNECTION_H_ */
