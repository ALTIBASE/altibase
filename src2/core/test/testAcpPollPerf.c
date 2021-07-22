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
 
/*******************************************************************************
 * $Id: testAcpPollPerf.c 7329 2009-09-01 00:05:05Z djin $
 ******************************************************************************/

#include <acp.h>
#include <act.h>


#define BUFFER_SIZE         100

#define CLIENT_THREAD_COUNT 10
#define CLIENT_THREAD_LOOP  100000

#define SOCK_TYPE_LISTEN    ((void *)1)
#define SOCK_TYPE_PEER      ((void *)2)


static act_barrier_t gBarrier;

static acp_sint16_t  gPort;
static acp_sint32_t  gConnectionCount = 0;


static acp_sint32_t processAccept(acp_poll_set_t       *aPollSet,
                                  const acp_poll_obj_t *aPollObj)
{
    acp_sock_t   *sSock;
    acp_sint32_t  sOption;
    acp_rc_t      sRC;

    sRC = acpMemAlloc((void **)&sSock, sizeof(acp_sock_t));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockAccept(sSock, aPollObj->mSock, NULL, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sOption = 1;
    sRC = acpSockSetOpt(sSock,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        &sOption,
                        (acp_sock_len_t)sizeof(sOption));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpPollAddSock(aPollSet, sSock, ACP_POLL_IN, SOCK_TYPE_PEER);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    gConnectionCount++;

    return 0;
}

static acp_sint32_t processPeer(acp_poll_set_t       *aPollSet,
                                const acp_poll_obj_t *aPollObj)
{
    acp_char_t sBuffer[BUFFER_SIZE];
    acp_size_t sRecvSize;
    acp_rc_t   sRC;

    sRC = acpSockRecv(aPollObj->mSock,
                      sBuffer,
                      sizeof(sBuffer),
                      &sRecvSize,
                      0);

    if (ACP_RC_IS_EOF(sRC))
    {
        sRC = acpPollRemoveSock(aPollSet, aPollObj->mSock);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockClose(aPollObj->mSock);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        acpMemFree(aPollObj->mSock);

        gConnectionCount--;

        if (gConnectionCount == 0)
        {
            return 1;
        }
        else
        {
            /* do nothing */
        }
    }
    else if (ACP_RC_IS_SUCCESS(sRC))
    {
        sRC = acpSockSend(aPollObj->mSock, sBuffer, sRecvSize, NULL, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
    else
    {
        ACT_ERROR(("acpSockRecv returned error = %d", sRC));
    }

    return 0;
}

static acp_sint32_t callbackPoll(acp_poll_set_t       *aPollSet,
                                 const acp_poll_obj_t *aPollObj,
                                 void                 *aData)
{
    ACP_UNUSED(aData);

    if (aPollObj->mUserData == SOCK_TYPE_LISTEN)
    {
        return processAccept(aPollSet, aPollObj);
    }
    else if (aPollObj->mUserData == SOCK_TYPE_PEER)
    {
        return processPeer(aPollSet, aPollObj);
    }
    else
    {
        ACT_ERROR(("unknown type of socket dispatched"));
        return 0;
    }
}

static acp_sint32_t clientThread(void *aArg)
{
    acp_char_t         sBuffer[BUFFER_SIZE];
    struct sockaddr_in sAddr;
    acp_sock_t         sSock;
    acp_size_t         sSendSize;
    acp_size_t         sRecvSize;
    acp_sint32_t       sOption;
    acp_rc_t           sRC;
    acp_sint32_t       i;

    ACP_UNUSED(aArg);

    acpMemSet(sBuffer, 1, sizeof(sBuffer));

    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddr.sin_port        = gPort;

    sRC = acpSockOpen(&sSock, ACP_AF_INET, ACP_SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockConnect(&sSock,
                         (acp_sock_addr_t *)&sAddr,
                         (acp_sock_len_t)sizeof(sAddr));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sOption = 1;
    sRC = acpSockSetOpt(&sSock,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        &sOption,
                        (acp_sock_len_t)sizeof(sOption));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_BARRIER_TOUCH_AND_WAIT(&gBarrier, CLIENT_THREAD_COUNT + 2);

    for (i = 0; i < CLIENT_THREAD_LOOP; i++)
    {
        sRC = acpSockSend(&sSock, sBuffer, sizeof(sBuffer), &sSendSize, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockRecv(&sSock, sBuffer, sizeof(sBuffer), &sRecvSize, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        ACT_CHECK(sSendSize == sRecvSize);
    }

    sRC = acpSockClose(&sSock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

static acp_sint32_t serverThread(void *aArg)
{
    struct sockaddr_in sAddr;
    acp_sock_len_t     sAddrLen;
    acp_sock_t         sSock;
    acp_poll_set_t     sPollSet;
    acp_rc_t           sRC;

    ACP_UNUSED(aArg);

    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);
    sAddr.sin_port        = 0;

    sRC = acpSockOpen(&sSock, ACP_AF_INET, ACP_SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockBind(&sSock,
                      (acp_sock_addr_t *)&sAddr,
                      (acp_sock_len_t)sizeof(sAddr),
                      ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockListen(&sSock, 100);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSock, (acp_sock_addr_t *)&sAddr, &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    gPort = (acp_sint16_t)sAddr.sin_port;

    ACT_BARRIER_TOUCH(&gBarrier);

    sRC = acpPollCreate(&sPollSet, CLIENT_THREAD_COUNT + 1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpPollAddSock(&sPollSet, &sSock, ACP_POLL_IN, SOCK_TYPE_LISTEN);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    while (1)
    {
        sRC = acpPollDispatch(&sPollSet, ACP_TIME_INFINITE, callbackPoll, NULL);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            break;
        }
        else
        {
            /* continue */
        }
    }

    sRC = acpPollDestroy(&sPollSet);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockClose(&sSock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

acp_sint32_t main(void)
{
    acp_thr_t    sServerThr;
    acp_thr_t    sClientThr[CLIENT_THREAD_COUNT];
    acp_time_t   sTime;
    acp_rc_t     sRC;
    acp_sint32_t i;

    ACT_TEST_BEGIN();

    ACT_BARRIER_CREATE(&gBarrier);

    sRC = acpThrCreate(&sServerThr, NULL, serverThread, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_BARRIER_WAIT(&gBarrier, 1);

    for (i = 0; i < CLIENT_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sClientThr[i], NULL, clientThread, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    ACT_BARRIER_WAIT(&gBarrier, CLIENT_THREAD_COUNT + 1);

    sTime = acpTimeNow();

    ACT_BARRIER_TOUCH(&gBarrier);

    sRC = acpThrJoin(&sServerThr, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < CLIENT_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sClientThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sTime = acpTimeNow() - sTime;

    ACT_BARRIER_DESTROY(&gBarrier);

    (void)acpPrintf("thread %d, loop %d time: %5lld msec\n",
                    CLIENT_THREAD_COUNT,
                    CLIENT_THREAD_LOOP,
                    acpTimeToMsec(sTime));

    ACT_TEST_END();

    return 0;
}
