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
#include <ulxXaConnection.h>
#include <ulxDef.h>


ulxXaConnection *gUlxConnectionHeader = NULL;
acp_thr_once_t   once_control         = ACP_THR_ONCE_INIT;

// callback functions for global variables;
void ulxDestroyConnHeader(void)
{
    if (gUlxConnectionHeader != NULL)
    {
        /* BUG-46052 codesonar null test after dereference */
        acpThrMutexDestroy(gUlxConnectionHeader->mLock);
        uluLockDestroy(gUlxConnectionHeader->mLock);
        acpMemFree(gUlxConnectionHeader);
        gUlxConnectionHeader = NULL;
    }
}

void ulxInitConnHeader(void)
{
    acp_thr_mutex_t *sLockEnv;

    //BUG-28168 [CodeSonar] Null Point Dereference
    if (acpMemAlloc((void**)&gUlxConnectionHeader, ACI_SIZEOF(ulxXaConnection))
        == ACP_RC_SUCCESS)
    {
        ulxMsgLogInitialize(&(gUlxConnectionHeader->mLogObj));
        gUlxConnectionHeader->mEnv = NULL;
        gUlxConnectionHeader->mDbc = NULL;
        gUlxConnectionHeader->next = NULL;
        uluLockCreate(&(gUlxConnectionHeader->mLock));
        sLockEnv = gUlxConnectionHeader->mLock;
        acpThrMutexCreate(sLockEnv, ACP_THR_MUTEX_DEFAULT);
    }
}

ulxXaConnection* ulxGetConnectionHeader()
{
    acpThrOnce(&once_control, ulxInitConnHeader);
    return gUlxConnectionHeader;
}

ACI_RC ulxFindConnection(acp_sint32_t rmid, ulxXaConnection **aConnection)
{
    ulxXaConnection *sConnHeader;
    ulxXaConnection *sConn;
    acp_thr_mutex_t *sLockEnv;

    sConnHeader = ulxGetConnectionHeader();
    sLockEnv = sConnHeader->mLock;

    ACI_TEST(acpThrMutexLock(sLockEnv) != ACP_RC_SUCCESS);

    for (sConn = sConnHeader->next;
         sConn != NULL; sConn = sConn->next)
    {
        if (sConn->mRmid == rmid)
        {
            *aConnection = sConn;
            break;
        }
    }

    ACI_TEST(acpThrMutexUnlock(sLockEnv) != ACP_RC_SUCCESS);

    ACI_TEST_RAISE(sConn == NULL, LABEL_ERR_NO_CONNECTION);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ERR_NO_CONNECTION);
    {
        *aConnection = NULL;
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulxGetFirstConnection(ulxXaConnection **aConnection)
{
    ulxXaConnection* sConnHeader;
    sConnHeader = ulxGetConnectionHeader();
    ACI_TEST_RAISE(sConnHeader->next == NULL,LABEL_ERR_NO_CONNECTION);

    *aConnection = sConnHeader->next;
    return ACI_SUCCESS;


    ACI_EXCEPTION(LABEL_ERR_NO_CONNECTION);
    {
        *aConnection = NULL;
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulxDeleteConnection(acp_sint32_t rmid)
{
    ulxXaConnection *sConnHeader;
    ulxXaConnection *sConn;
    ulxXaConnection *sPrevConn;
    acp_thr_mutex_t *sLockEnv;

    sConnHeader = ulxGetConnectionHeader();
    sPrevConn = sConnHeader;

    sLockEnv = sConnHeader->mLock;

    ACI_TEST(acpThrMutexLock(sLockEnv) != ACP_RC_SUCCESS);

    for (sConn = sConnHeader->next;
         sConn != NULL; sConn = sConn->next)
    {
        if (sConn->mRmid == rmid)
        {
            break;
        }
        sPrevConn = sConn;
    }

    if (sConn != NULL)
    {
        if ( sConn->mRecoverXid != NULL )
        {
            acpMemFree(sConn->mRecoverXid);
            sConn->mRecoverXid = NULL;
        }
        sPrevConn->next = sConn->next;
        acpMemFree(sConn);
    }

    ACI_TEST(acpThrMutexUnlock(sLockEnv) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static void ulxInitConnection(ulxXaConnection *aConn)
{
    ACE_ASSERT(aConn != NULL);

    ulxMsgLogInitialize(&(aConn->mLogObj));

    aConn->mRmid        = ULX_INIT_RMID;
    aConn->mOpenFlag    = TMNOFLAGS;
    aConn->mEnv         = NULL;
    aConn->mDbc         = NULL;
    aConn->mLock        = NULL;
    aConn->next         = NULL;
    aConn->mStatus      = ULX_XA_DISCONN;

    aConn->mRecoverCnt  = 0;
    aConn->mRecoverPos  = -1;
    aConn->mRecoverXid  = NULL;
}

//PROJ-1645 UL-FailOver
ACI_RC ulxAddConnection(acp_sint32_t      rmid,
                        ulxXaConnection **aConnection,
                        long              aOpenFlag)
{
    ulxXaConnection *sConnHeader;
    ulxXaConnection *sConn;
    acp_thr_mutex_t *sLockEnv;
    acp_sint32_t     sState = 0;

    sConnHeader = ulxGetConnectionHeader();

    sLockEnv = sConnHeader->mLock;

    ACI_TEST(acpThrMutexLock(sLockEnv) != ACP_RC_SUCCESS);

    sState = 1;

    for (sConn = sConnHeader->next; sConn != NULL; sConn = sConn->next)
    {
        if (sConn->mRmid == rmid)
        {
            break;
        }
    }

    ACI_TEST(sConn != NULL);

    ACI_TEST(acpMemAlloc((void**)&sConn, ACI_SIZEOF(ulxXaConnection)) != ACP_RC_SUCCESS);

    sState = 2;

    ulxInitConnection(sConn);
    sConn->mRmid        = rmid;
    //PROJ-1645 UL-FailOver
    sConn->mOpenFlag    = aOpenFlag;
    sConn->next         = sConnHeader->next;

    *aConnection = sConn;

    sConnHeader->next   = *aConnection;

    ACI_TEST(acpThrMutexUnlock(sLockEnv) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            acpMemFree(sConn);
        case 1:
            (void)acpThrMutexUnlock(sLockEnv);
        default:
            break;
    }

    return ACI_FAILURE;
}

void ulxConnSetDisconn(ulxXaConnection *aConn)
{
    aConn->mStatus = ULX_XA_DISCONN;
}

void ulxConnSetConn(ulxXaConnection *aConn)
{
    aConn->mStatus = ULX_XA_CONN;
}

void ulxConnSetActive(ulxXaConnection *aConn)
{
    aConn->mStatus = ULX_XA_ACTIVE;
}

ulxXaStatus ulxConnGetStatus(ulxXaConnection *aConn)
{
    return aConn->mStatus;
}

void ulxConnInitRecover(ulxXaConnection *aConn)
{
    aConn->mRecoverCnt = 0;
    aConn->mRecoverPos = -1;
    if ( aConn->mRecoverXid != NULL )
    {
        acpMemFree(aConn->mRecoverXid);
        aConn->mRecoverXid = NULL;
    }
}

//fix BUG-25597 APRE에서 AIX플랫폼 턱시도 연동문제를 해결해야 합니다.
// APRE의 ulConnMgr 초기화전에 이미 생성된 CLI 의 XA Connection들을
// Loading하는 함수이다.
void  ulxXaRegisterOpenedConnections2APRE()
{
    ulxXaConnection *sConnHeader;
    ulxXaConnection *sConn;
    acp_thr_mutex_t *sLockEnv;

    sConnHeader = ulxGetConnectionHeader();

    sLockEnv = sConnHeader->mLock;

    acpThrMutexLock(sLockEnv);

    for (sConn = sConnHeader->next; sConn != NULL; sConn = sConn->next)
    {
        gCallbackForSesConn(sConn->mRmid, (SQLHENV)(sConn->mEnv), (SQLHDBC)(sConn->mDbc));
    }

    acpThrMutexUnlock(sLockEnv);
}

