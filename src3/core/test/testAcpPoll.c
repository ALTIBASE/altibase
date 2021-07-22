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
 * $Id: testAcpPoll.c 9236 2009-12-17 02:57:25Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpMem.h>
#include <acpPoll.h>
#include <acpSock.h>
#include <acpThr.h>
#include <acpByteOrder.h>


#define SOCK_TCP_LISTEN ((void *)0)
#define SOCK_TCP_PEER   ((void *)1)
#define SOCK_UDP_PEER   ((void *)2)


#define SOCK_COUNT    10
#define BUFFER_SIZE   50

#define SOCK_NUM      5

static acp_bool_t   gTcpClientRun;
static acp_bool_t   gUdpClientRun;

static acp_size_t   gTcpRecvSize  = 0;
static acp_size_t   gTcpSendSize  = 0;
static acp_sint32_t gUdpRecvCount = 0;
static acp_sint32_t gUdpSendCount = 0;

static acp_sint32_t callbackTestRemovePoll(acp_poll_set_t       *aPollSet,
                                           const acp_poll_obj_t *aPollObj,
                                           void                 *aData)
{
    ACP_UNUSED(aPollSet);
    ACP_UNUSED(aPollObj);
    ACP_UNUSED(aData);

    /* Do nothing, only return non 0 value to stop dispatching */
    return 1;
}

static acp_sint32_t testRemoveThread(void *aArg)
{
    acp_rc_t         sRC;
    acp_poll_set_t   sPollSet;
    acp_sock_t      *sSockListen = (acp_sock_t *)aArg;
    acp_sock_t       sSock[SOCK_NUM];
    acp_uint16_t     i;

    /* Create poll set */
    sRC = acpPollCreate(&sPollSet, SOCK_NUM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Accept connections */
    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpSockAccept(&sSock[i], sSockListen, NULL, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpPollAddSock(&sPollSet, &sSock[i], ACP_POLL_IN, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /* Start dispatching events from sockets */
    sRC = acpPollDispatch(&sPollSet,
                          acpTimeFromMsec(1000),
                          callbackTestRemovePoll,
                          NULL);

    /* Check for ECANCELLED */
    ACT_CHECK(ACP_RC_IS_ECANCELED(sRC));

#if defined(ALTI_CFG_OS_TRU64)
    /* To be sure not closing socket before send */
    acpSleepSec(1);
#endif

    /* Remove sockets from poll set  */
    for (i = 0; i < SOCK_NUM; i++)
    { 
        sRC = acpPollRemoveSock(&sPollSet, &sSock[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC), 
                    ("Failed to remove sock from poll sRC=%d\n", sRC));

        sRC = acpSockClose(&sSock[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpPollDestroy(&sPollSet);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

static acp_sint32_t callbackPoll(acp_poll_set_t       *aPollSet,
                                 const acp_poll_obj_t *aPollObj,
                                 void                 *aData)
{
    struct sockaddr_in  sAddr;
    acp_sock_len_t      sAddrLen;
    acp_char_t          sBuffer[BUFFER_SIZE];
    acp_size_t          sRecvSize;
    acp_sock_t         *sSock;
    acp_rc_t            sRC = ACP_RC_EINVAL;

    ACP_UNUSED(aData);

    if (aPollObj->mUserData == SOCK_TCP_LISTEN)
    {
        /*
         * accept new tcp client connection
         */
        sRC = acpMemAlloc((void **)&sSock, sizeof(acp_sock_t));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockAccept(sSock, aPollObj->mSock, NULL, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpPollAddSock(aPollSet, sSock, ACP_POLL_IN, SOCK_TCP_PEER);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
    else if (aPollObj->mUserData == SOCK_TCP_PEER)
    {
        /*
         * receive data from tcp client
         */
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
        }
        else if (ACP_RC_IS_SUCCESS(sRC))
        {
            gTcpRecvSize += sRecvSize;
        }
        else
        {
            ACT_ERROR(("acpSockRecv returned error %d", sRC));
        }
    }
    else if (aPollObj->mUserData == SOCK_UDP_PEER)
    {
        /*
         * receive data from udp client
         */
        sAddrLen = (acp_sock_len_t)sizeof(sAddr);
        sRC = acpSockRecvFrom(aPollObj->mSock,
                              sBuffer,
                              sizeof(sBuffer),
                              &sRecvSize,
                              0,
                              (acp_sock_addr_t *)&sAddr,
                              &sAddrLen);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC(sRecvSize ==
                       (acp_size_t)
                       ((ACP_TOH_BYTE2_ARG(sAddr.sin_port) % BUFFER_SIZE) + 1),
                       ("acpSockRecvFrom should receive %zd bytes "
                        "but received %zd bytes",
                        ((ACP_TOH_BYTE2_ARG(sAddr.sin_port) % BUFFER_SIZE) + 1),
                        sRecvSize));

        gUdpRecvCount++;
    }
    else
    {
        ACT_ERROR(("unknown type of socket polled"));
    }

    return sRC;
}

static acp_sint32_t testTcpClient(void *aArg)
{
    acp_char_t          sBuffer[SOCK_COUNT * 10];
    struct sockaddr_in  sAddrClient;
    acp_sock_t          sSock[SOCK_COUNT];
    acp_size_t          sSendSize;
    acp_rc_t            sRC;
    acp_sint32_t        sOption = 1;
    acp_sint32_t        i;

    acpMemSet(sBuffer, 1, sizeof(sBuffer));

    /*
     * connect to server
     */
    sAddrClient.sin_family      = ACP_AF_INET;
    sAddrClient.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddrClient.sin_port        = (acp_uint16_t)((acp_ulong_t)aArg);

    for (i = 0; i < SOCK_COUNT; i++)
    {
        sRC = acpSockOpen(&sSock[i], ACP_AF_INET, ACP_SOCK_STREAM, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockConnect(&sSock[i],
                             (acp_sock_addr_t *)&sAddrClient,
                             (acp_sock_len_t)sizeof(sAddrClient));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockSetOpt(&sSock[i],
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            &sOption,
                            (acp_sock_len_t)sizeof(sOption));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * send data to server
     */
    for (i = 0; i < SOCK_COUNT; i++)
    {
        sRC = acpSockSend(&sSock[i],
                          sBuffer,
                          (acp_size_t)i * 10,
                          &sSendSize,
                          0);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            gTcpSendSize += sSendSize;
        }
        else
        {
            ACT_ERROR(("acpSockSend returned error %d in test case (%d)",
                       sRC,
                       i));
        }
    }

    /*
     * send data to server
     */
    for (i = SOCK_COUNT - 1; i >= 0; i--)
    {
        sRC = acpSockSend(&sSock[i],
                          sBuffer,
                          (acp_size_t)i * 10,
                          &sSendSize,
                          0);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            gTcpSendSize += sSendSize;
        }
        else
        {
            ACT_ERROR(("acpSockSend returned error %d in test case (%d)",
                       sRC,
                       i));
        }
    }

    /*
     * disconnect from server
     */
    for (i = 0; i < SOCK_COUNT; i++)
    {
        sRC = acpSockClose(&sSock[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    gTcpClientRun = ACP_FALSE;

    return 0;
}

static acp_sint32_t testUdpClient(void *aArg)
{
    acp_char_t          sBuffer[BUFFER_SIZE];
    struct sockaddr_in  sAddrClient;
    struct sockaddr_in  sAddrServer;
    acp_sock_len_t      sAddrLen;
    acp_sock_t          sSock[SOCK_COUNT];
    acp_size_t          sSendSize;
    acp_rc_t            sRC;
    acp_sint32_t        i;

    acpMemSet(sBuffer, 1, sizeof(sBuffer));

    /*
     * address
     */
    sAddrClient.sin_family      = ACP_AF_INET;
    sAddrClient.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);

    sAddrServer.sin_family      = ACP_AF_INET;
    sAddrServer.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddrServer.sin_port        = (acp_uint16_t)((acp_ulong_t)aArg);

    /*
     * open & bind udp socket
     */
    for (i = 0; i < SOCK_COUNT; i++)
    {
        sAddrClient.sin_port        = ACP_TON_BYTE2_ARG(45771 + i);
        sRC = acpSockOpen(&sSock[i], ACP_AF_INET, ACP_SOCK_DGRAM, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockBind(&sSock[i],
                          (acp_sock_addr_t *)&sAddrClient,
                          (acp_sock_len_t)sizeof(sAddrClient),
                          ACP_FALSE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * send data to server
     */
    for (i = 0; i < SOCK_COUNT; i++)
    {
        sAddrLen = (acp_sock_len_t)sizeof(sAddrClient);
        sRC = acpSockGetName(&sSock[i],
                             (acp_sock_addr_t *)&sAddrClient,
                             &sAddrLen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sSendSize = (ACP_TOH_BYTE2_ARG(sAddrClient.sin_port) % BUFFER_SIZE) + 1;

        sRC = acpSockSendTo(&sSock[i],
                            sBuffer,
                            sSendSize,
                            &sSendSize,
                            0,
                            (acp_sock_addr_t *)&sAddrServer,
                            (acp_sock_len_t)sizeof(sAddrServer));

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            gUdpSendCount++;
        }
        else
        {
            ACT_ERROR(("acpSockSend returned error %d in test case (%d)",
                       sRC,
                       i));
        }
    }

    /*
     * send data to server
     */
    for (i = SOCK_COUNT - 1; i >= 0; i--)
    {
        sAddrLen = (acp_sock_len_t)sizeof(sAddrClient);
        sRC = acpSockGetName(&sSock[i],
                             (acp_sock_addr_t *)&sAddrClient,
                             &sAddrLen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sSendSize = (ACP_TOH_BYTE2_ARG(sAddrClient.sin_port) % BUFFER_SIZE) + 1;

        sRC = acpSockSendTo(&sSock[i],
                            sBuffer,
                            sSendSize,
                            &sSendSize,
                            0,
                            (acp_sock_addr_t *)&sAddrServer,
                            (acp_sock_len_t)sizeof(sAddrServer));

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            gUdpSendCount++;
        }
        else
        {
            ACT_ERROR(("acpSockSend returned error %d in test case (%d)",
                       sRC,
                       i));
        }
    }

    /*
     * close udp socket
     */
    for (i = 0; i < SOCK_COUNT; i++)
    {
        sRC = acpSockClose(&sSock[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    gUdpClientRun = ACP_FALSE;

    return 0;
}

static void testPoll(void)
{
    acp_time_t         sTime1;
    acp_time_t         sTime2;
    acp_thr_t          sTcpThread;
    acp_thr_t          sUdpThread;
    acp_poll_set_t     sPollSet;
    struct sockaddr_in sAddr;
    acp_sock_len_t     sAddrLen;
    acp_sock_t         sSockListen;
    acp_sock_t         sSockUdp;
    acp_rc_t           sRC;

    /*
     * open tcp server socket
     */
    sRC = acpSockOpen(&sSockListen, ACP_AF_INET, ACP_SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * bind tcp server socket
     */
    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);
    sAddr.sin_port        = ACP_TON_BYTE2_ARG(45768);

    sRC = acpSockBind(&sSockListen,
                      (acp_sock_addr_t *)&sAddr,
                      (acp_sock_len_t)sizeof(sAddr),
                      ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * listen tcp server socket
     */
    sRC = acpSockListen(&sSockListen, 10);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * open udp server socket
     */
    sRC = acpSockOpen(&sSockUdp, ACP_AF_INET, ACP_SOCK_DGRAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * bind udp server socket
     */
    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);
    sAddr.sin_port        = ACP_TON_BYTE2_ARG(45769);

    sRC = acpSockBind(&sSockUdp,
                      (acp_sock_addr_t *)&sAddr,
                      (acp_sock_len_t)sizeof(sAddr),
                      ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * create poll set
     */
    sRC = acpPollCreate(&sPollSet, SOCK_COUNT + 2);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpPollCreate returned error %d", sRC));

    /*
     * add server socket to poll set
     */
    sRC = acpPollAddSock(&sPollSet, &sSockListen, ACP_POLL_IN, SOCK_TCP_LISTEN);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpPollAddSock returned error %d", sRC));

    sRC = acpPollAddSock(&sPollSet, &sSockUdp, ACP_POLL_IN, SOCK_UDP_PEER);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpPollAddSock returned error %d", sRC));

    /*
     * poll -> ETIMEDOUT
     */
    sTime1 = acpTimeNow();
    sRC = acpPollDispatch(&sPollSet, ACP_TIME_IMMEDIATE, NULL, NULL);
    sTime2 = acpTimeNow();
    ACT_CHECK_DESC(ACP_RC_IS_ETIMEDOUT(sRC),
                   ("acpPollDispatch should return ETIMEDOUT but %d", sRC));
    ACT_CHECK_DESC((sTime2 - sTime1) < 1000,
                   ("acpPollDispatch should return immediately "
                    "but taked %lldus",
                    (sTime2 - sTime1)));

    sTime1 = acpTimeNow();
    sRC = acpPollDispatch(&sPollSet, acpTimeFromMsec(100), NULL, NULL);
    sTime2 = acpTimeNow();
    ACT_CHECK_DESC(ACP_RC_IS_ETIMEDOUT(sRC),
                   ("acpPollDispatch should return ETIMEDOUT but %d", sRC));
    ACT_CHECK_DESC(((sTime2 - sTime1) >= acpTimeFromMsec(50)) &&
                   ((sTime2 - sTime1) <= acpTimeFromMsec(999)),
                   ("acpPollDispatch timeout is not acceptable(%lld)",
                    (sTime2 - sTime1)));

    /*
     * get address of tcp server socket
     */
    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSockListen, (acp_sock_addr_t *)&sAddr, &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * create tcp client thread
     */
    sRC = acpThrCreate(&sTcpThread,
                       NULL,
                       testTcpClient,
                       (void *)((acp_ulong_t)sAddr.sin_port));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * get address of udp server socket
     */
    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSockUdp, (acp_sock_addr_t *)&sAddr, &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * create udp client thread
     */
    sRC = acpThrCreate(&sUdpThread,
                       NULL,
                       testUdpClient,
                       (void *)((acp_ulong_t)sAddr.sin_port));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * poll loop
     */
    while (1)
    {
        sRC = acpPollDispatch(&sPollSet,
                              acpTimeFromMsec(200),
                              callbackPoll,
                              NULL);

        if (ACP_RC_IS_ETIMEDOUT(sRC))
        {
            if ((gTcpClientRun == ACP_FALSE) && (gUdpClientRun == ACP_FALSE))
            {
                break;
            }
            else
            {
                /* continue */
            }
        }
        else if (ACP_RC_IS_SUCCESS(sRC))
        {
            /* continue */
        }
        else
        {
            ACT_ERROR(("acpPollDispatch returned error %d", sRC));
            break;
        }
    }

    /*
     * destroy poll set
     */
    sRC = acpPollRemoveSock(&sPollSet, &sSockListen);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpPollRemoveSock returned error %d", sRC));

    sRC = acpPollRemoveSock(&sPollSet, &sSockUdp);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpPollRemoveSock returned error %d", sRC));

    ACT_CHECK_DESC(sPollSet.mCurCount == 0,
                   ("number of socket in the poll set should be 0 but %d",
                    sPollSet.mCurCount));

    sRC = acpPollDestroy(&sPollSet);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * join client thread
     */
    sRC = acpThrJoin(&sTcpThread, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sUdpThread, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * close sockets
     */
    sRC = acpSockClose(&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockClose(&sSockUdp);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}


/*
 *  Test case for BUG-27551 acpPollRemoveSock functionality
 *  Tests that sockets are successfully removed from poll
 *  after dispatch is cancelled
 */
static void testPollRemove(void)
{
    acp_rc_t        sRC;
    acp_thr_t       sServerThread;

    acp_sock_t      sSock[SOCK_NUM];
    acp_sock_t      sSockListen;
    acp_uint16_t    i;
    acp_sock_len_t  sAddrLen;

    acp_char_t      sBuffer[] = "Test";
    acp_size_t      sSendSize;

    struct sockaddr_in  sAddrClient;
    struct sockaddr_in  sAddr;


    /* open tcp server socket */
    sRC = acpSockOpen(&sSockListen, ACP_AF_INET, ACP_SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* bind tcp server socket */
    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);
    sAddr.sin_port        = ACP_TON_BYTE2_ARG(45770);

    sRC = acpSockBind(&sSockListen,
                      (acp_sock_addr_t *)&sAddr,
                      (acp_sock_len_t)sizeof(sAddr),
                      ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* listen tcp server socket */
    sRC = acpSockListen(&sSockListen, SOCK_NUM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSockListen, (acp_sock_addr_t *)&sAddr, &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* start server thread */
    sRC = acpThrCreate(&sServerThread,
                      NULL,
                      testRemoveThread,
                      (void *)&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));


    /* Create client sockets */
    sAddrClient.sin_family      = ACP_AF_INET;
    sAddrClient.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddrClient.sin_port        = sAddr.sin_port;

    /* Connect sockets and send data */
    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpSockOpen(&sSock[i], ACP_AF_INET, ACP_SOCK_STREAM, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockConnect(&sSock[i],
                             (acp_sock_addr_t *)&sAddrClient,
                             (acp_sock_len_t)sizeof(sAddrClient));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockSend(&sSock[i],
                          sBuffer,
                          (acp_size_t)sizeof(sBuffer),
                          &sSendSize,
                          0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    }

    /* Join server thread */
    sRC = acpThrJoin(&sServerThread, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Close sockets */
    sRC = acpSockClose(&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpSockClose(&sSock[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    gTcpClientRun = ACP_TRUE;
    gUdpClientRun = ACP_TRUE;

    testPoll();

    ACT_CHECK_DESC(gTcpRecvSize == gTcpSendSize,
                   ("tcp recv size = %zu, tcp send size = %zu mismatch",
                    gTcpRecvSize, gTcpSendSize));

    testPollRemove();

    ACT_TEST_END();

    return 0;
}
