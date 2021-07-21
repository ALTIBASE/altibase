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
 * $Id: testAcpSock.c 11359 2010-06-25 08:40:49Z vsebelev $
 ******************************************************************************/

#include <act.h>
#include <acpByteOrder.h>
#include <acpPoll.h>
#include <acpSock.h>
#include <acpInet.h>
#include <acpThr.h>

#define TEST_TIMED_CONNECT_SOCK_MAX 100

static acp_bool_t gIsUnreachable = ACP_FALSE;


static acp_sint32_t testTimedConnect(void *aArg)
{
    struct sockaddr_in  sAddrClient;
    struct sockaddr_in *sAddrServer;
    acp_sock_t          sSock[TEST_TIMED_CONNECT_SOCK_MAX];
    acp_rc_t            sRC;
    acp_sint32_t        i;
    acp_sint32_t        j;

    sAddrServer = (struct sockaddr_in *)aArg;

    /* Clearing sAddrClient */
    acpMemSet(&sAddrClient, 0, sizeof(sAddrClient));

    sAddrClient.sin_family      = ACP_AF_INET;
    sAddrClient.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddrClient.sin_port        = sAddrServer->sin_port;

    for (i = 0; i < TEST_TIMED_CONNECT_SOCK_MAX; i++)
    {
        sRC = acpSockOpen(&sSock[i], ACP_AF_INET, SOCK_STREAM, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockTimedConnect(&sSock[i],
                                  (acp_sock_addr_t *)&sAddrClient,
                                  (acp_sock_len_t)sizeof(sAddrClient),
                                  acpTimeFromMsec(100));

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            /* continue, normal case */
        }
        else if (ACP_RC_IS_ETIMEDOUT(sRC))
        {
            /* continue, normal case */
        }
#if defined(ALTI_CFG_OS_FREEBSD)
        else if (ACP_RC_IS_ECONNRESET(sRC))
        {
           /* Continue, normal case for FreeBSD */
        }
#endif
        else
        {
            ACT_ERROR(("acpSockTimedConnect should return SUCCESS or ETIMEDOUT "
                       "but %d", sRC));
        }
    }

    for (j = 0; j <= ACP_MIN(i, TEST_TIMED_CONNECT_SOCK_MAX - 1); j++)
    {
        sRC = acpSockClose(&sSock[j]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return 0;
}

static acp_sint32_t testConnect(void *aArg)
{
    struct sockaddr_in  sAddrClient;
    struct sockaddr_in *sAddrServer;
    acp_sock_t          sSock;
    acp_rc_t            sRC;

    sAddrServer = (struct sockaddr_in *)aArg;

    /* Clearing sAddrClient */
    acpMemSet(&sAddrClient, 0, sizeof(sAddrClient));

    sAddrClient.sin_family      = ACP_AF_INET;
    sAddrClient.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddrClient.sin_port        = sAddrServer->sin_port;

    sRC = acpSockOpen(&sSock, ACP_AF_INET, SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockConnect(&sSock,
                         (acp_sock_addr_t *)&sAddrClient,
                         (acp_sock_len_t)sizeof(sAddrClient));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockClose(&sSock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}


#define TEST_IPV4_PORT 45768


static void testMain(void)
{
    acp_char_t         sBuffer[10];
    acp_thr_t          sThread;
    struct sockaddr_in sAddr;
    acp_sock_len_t     sAddrLen;
    acp_sock_t         sSockListen;
    acp_sock_t         sSockPeer;
    acp_sint32_t       sOpt;
    acp_sock_len_t     sOptSize;
    acp_rc_t           sRC;



    /*
     * acpSockOpen
     */
    sRC = acpSockOpen(&sSockListen, ACP_AF_INET, SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Clearing sAddrClient */
    acpMemSet(&sAddr, 0, sizeof(sAddr));

    /*
     * acpSockBind
     */
    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);
    sAddr.sin_port        = ACP_TON_BYTE2_ARG(TEST_IPV4_PORT);

    sRC = acpSockBind(&sSockListen,
                      (acp_sock_addr_t *)&sAddr,
                      (acp_sock_len_t)sizeof(sAddr),
                      ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * Test SetOpt and GetOpt
     */
    sOptSize = (acp_sock_len_t)sizeof(sOpt);
    sRC = acpSockGetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, &sOptSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_FALSE != sOpt);

    sOpt = ACP_FALSE;
    sRC = acpSockSetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, (acp_sock_len_t)sizeof(sOpt));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sOptSize = (acp_sock_len_t)sizeof(sOpt);
    sRC = acpSockGetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, &sOptSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_FALSE == sOpt);
     
    sOpt = ACP_TRUE;
    sRC = acpSockSetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, (acp_sock_len_t)sizeof(sOpt));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpSockGetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, &sOptSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_FALSE != sOpt);

    /*
     * acpSockListen
     */
    sRC = acpSockListen(&sSockListen, 1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpSockSetBlockMode <- ACP_FALSE
     */
    sRC = acpSockSetBlockMode(&sSockListen, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpSockTimedConnect
     */
    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSockListen,
                         (acp_sock_addr_t *)&sAddr,
                         &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCreate(&sThread, NULL, testTimedConnect, &sAddr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sThread, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    while (1)
    {
        sRC = acpSockAccept(&sSockPeer,
                            &sSockListen,
                            (acp_sock_addr_t *)&sAddr,
                            &sAddrLen);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            /*
             * Check if sAddrLen is truncated
             */
            acp_uint32_t sLenCheck = (acp_uint32_t)sAddrLen;
            sRC = acpSockClose(&sSockPeer);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sAddrLen == sLenCheck);
        }
        else if (ACP_RC_IS_EAGAIN(sRC))
        {
            break;
        }
        else
        {
            ACT_ERROR(("acpSockAccept should return SUCCESS or EAGAIN but %d",
                       sRC));
            break;
        }
    }

    /*
     * acpSockSetBlockMode <- ACP_TRUE
     */
    sRC = acpSockSetBlockMode(&sSockListen, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpSockConnect
     */
    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSockListen,
                         (acp_sock_addr_t *)&sAddr,
                         &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCreate(&sThread, NULL, testConnect, &sAddr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockAccept(&sSockPeer,
                        &sSockListen,
                        (acp_sock_addr_t *)&sAddr,
                        &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sThread, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpSockPoll
     */
    sRC = acpSockPoll(&sSockPeer, ACP_POLL_IN, ACP_TIME_INFINITE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockRecv(&sSockPeer, sBuffer, sizeof(sBuffer), NULL, 0);
    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    sRC = acpSockClose(&sSockPeer);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockClose(&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}



static acp_sint32_t testTimedConnectIPv6(void *aArg)
{
    acp_sock_addr_in6_t *sAddrClient;
    acp_sock_addr_in6_t *sAddrServer;

    acp_inet_addr_info_t *sInfo;
    
    acp_sock_t          sSock[TEST_TIMED_CONNECT_SOCK_MAX];
    acp_rc_t            sRC;
    acp_sint32_t        i;
    acp_sint32_t        j;

    sAddrServer = (acp_sock_addr_in6_t *)aArg;

    /* Clearing sAddrClient */
    acpMemSet(&sAddrClient, 0, sizeof(sAddrClient));

    /* client should set address and family via getaddrinfo */
    sRC = acpInetGetAddrInfo(&sInfo, "::1", NULL, 0,
                             ACP_INET_AI_PASSIVE, ACP_AF_INET6);

    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FAIL_GETADDRINFO);

    sAddrClient = (acp_sock_addr_in6_t *)(sInfo->ai_addr);
    sAddrClient->sin6_port = sAddrServer->sin6_port;
    
    for (i = 0; i < TEST_TIMED_CONNECT_SOCK_MAX; i++)
    {
        sRC = acpSockOpen(&sSock[i], ACP_AF_INET6, SOCK_STREAM, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockTimedConnect(&sSock[i],
                                  (acp_sock_addr_t *)sAddrClient,
                                  (acp_sock_len_t)sizeof(acp_sock_addr_in6_t),
                                  acpTimeFromMsec(100));

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            /* continue, normal case */
        }
        else if (ACP_RC_IS_ETIMEDOUT(sRC))
        {
            /* continue, normal case */
        }
        else if (ACP_RC_IS_ENETUNREACH(sRC))
        {
           /*
            * continue,
            * BUG-30078. on some platforms
            * in which IPv6 function is available but not configured,
            * network unreachable occurs as error code
            */
           gIsUnreachable = ACP_TRUE;
        }
#if defined(ALTI_CFG_OS_FREEBSD)
        else if (ACP_RC_IS_ECONNRESET(sRC))
        {
           /* Continue, normal case for FreeBSD */
        }
#endif
        else
        {
            ACT_ERROR(("acpSockTimedConnect should return SUCCESS or ETIMEDOUT "
                       "but %d", sRC));
            /* connect failed, return error code */
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FAIL_CONNECT);
        }
    }

    for (j = 0; j <= ACP_MIN(i, TEST_TIMED_CONNECT_SOCK_MAX - 1); j++)
    {
        sRC = acpSockClose(&sSock[j]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(FAIL_GETADDRINFO)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_ERROR(("acpInetGetAddrInfo failed"));
    }

    ACP_EXCEPTION(FAIL_CONNECT)
    {
        for (j = 0; j < i; j++)
        {
            /* close opend sockets */
            (void)acpSockClose(&sSock[j]);
        }
    }

    ACP_EXCEPTION_END;
    return sRC;
}



static acp_sint32_t testConnectIPv6(void *aArg)
{
    acp_sock_addr_in6_t *sAddrClient;
    acp_sock_addr_in6_t *sAddrServer;
    acp_sock_t          sSock;
    acp_rc_t            sRC;
    acp_inet_addr_info_t *sInfo;

    sAddrServer = (acp_sock_addr_in6_t *)aArg;

    /* Clearing sAddrClient */
    acpMemSet(&sAddrClient, 0, sizeof(sAddrClient));

    /* client should set address and family via getaddrinfo */
    sRC = acpInetGetAddrInfo(&sInfo, "::1", NULL, 0,
                             ACP_INET_AI_PASSIVE, ACP_AF_INET6);

    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FAIL_GETADDRINFO);

    sAddrClient = (acp_sock_addr_in6_t *)(sInfo->ai_addr);
    sAddrClient->sin6_port = sAddrServer->sin6_port;
    
    sRC = acpSockOpen(&sSock, ACP_AF_INET6, SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockConnect(&sSock,
                         (acp_sock_addr_t *)sAddrClient,
                         (acp_sock_len_t)sizeof(acp_sock_addr_in6_t));
    /*   
     * BUG-30078. on some platforms
     * in which IPv6 function is available but not configured,
     * network unreachable occurs as error code
     */
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ENETUNREACH(sRC));

    /* connect failed, return error code */
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FAIL_CONNECT);
    
    sRC = acpSockClose(&sSock);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return  0;

    ACP_EXCEPTION(FAIL_GETADDRINFO)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_ERROR(("acpInetGetAddrInfo failed"));
    }

    ACP_EXCEPTION(FAIL_CONNECT)
    {
        (void)acpSockClose(&sSock);
    }
    
    ACP_EXCEPTION_END;
    return sRC;
}


#define TEST_PORT_IPV6 45768


static void testIPv6Main(void)
{
    acp_char_t         sBuffer[10];
    acp_thr_t          sThread;
    acp_sock_addr_in6_t sAddr6;
    acp_sock_len_t     sAddrLen;
    acp_sock_t         sSockListen;
    acp_sock_t         sSockPeer;
    acp_sint32_t       sOpt;
    acp_sock_len_t     sOptSize;
    acp_rc_t           sRC;
    acp_rc_t           sThrRC;


    /*
     * acpSockOpen
     */
    sRC = acpSockOpen(&sSockListen, ACP_AF_INET6, SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Clearing sAddrClient */
    acpMemSet(&sAddr6, 0, sizeof(sAddr6));

    /*
     * acpSockBind
     */
    /* For server address, each field can be initialized directly, */
    /* without getaddrinfo. */
    /* sin6_flowinfo, sin6_scope_id fields can be uninitialized */
    sAddr6.sin6_family      = ACP_AF_INET6;
    sAddr6.sin6_addr        = in6addr_any;
    sAddr6.sin6_port        = TEST_PORT_IPV6;

    sRC = acpSockBind(&sSockListen,
                      (acp_sock_addr_t *)&sAddr6,
                      (acp_sock_len_t)sizeof(sAddr6),
                      ACP_TRUE);
 
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACP_TEST_RAISE(ACP_RC_IS_EINVAL(sRC), IPV6_ISNOTACTIVE);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), IPV6_ERROR);

    /*
     * Test SetOpt and GetOpt
     */
    sOptSize = (acp_sock_len_t)sizeof(sOpt);
    sRC = acpSockGetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, &sOptSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_FALSE != sOpt);

    sOpt = ACP_FALSE;
    sRC = acpSockSetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, (acp_sock_len_t)sizeof(sOpt));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sOptSize = (acp_sock_len_t)sizeof(sOpt);
    sRC = acpSockGetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, &sOptSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_FALSE == sOpt);
     
    sOpt = ACP_TRUE;
    sRC = acpSockSetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, (acp_sock_len_t)sizeof(sOpt));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpSockGetOpt(&sSockListen, SOL_SOCKET, SO_REUSEADDR,
                        &sOpt, &sOptSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_FALSE != sOpt);

    /*
     * acpSockListen
     */
    sRC = acpSockListen(&sSockListen, 1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpSockSetBlockMode <- ACP_FALSE
     */
    sRC = acpSockSetBlockMode(&sSockListen, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpSockTimedConnect
     */
    sAddrLen = (acp_sock_len_t)sizeof(sAddr6);
    sRC = acpSockGetName(&sSockListen,
                         (acp_sock_addr_t *)&sAddr6,
                         &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCreate(&sThread, NULL, testTimedConnectIPv6, &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sThread, &sThrRC);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    /* check thread success */
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sThrRC), IPV6_TIMEDCONNECT);

    while (1)
    {
        sRC = acpSockAccept(&sSockPeer,
                            &sSockListen,
                            (acp_sock_addr_t *)&sAddr6,
                            &sAddrLen);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            /*
             * Check if sAddrLen is truncated
             */
            acp_uint32_t sLenCheck = (acp_uint32_t)sAddrLen;
            sRC = acpSockClose(&sSockPeer);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sAddrLen == sLenCheck);
        }
        else if (ACP_RC_IS_EAGAIN(sRC))
        {
            break;
        }
        else
        {
            ACT_ERROR(("acpSockAccept should return SUCCESS or EAGAIN but %d",
                       sRC));
            break;
        }
    }

    /*
     * BUG-30078
     * Skip the tests if IPv6 is not configured
     */
    ACP_TEST_RAISE(gIsUnreachable == ACP_TRUE, IPV6_UNREACH);
    /*
     * acpSockSetBlockMode <- ACP_TRUE
     */
    sRC = acpSockSetBlockMode(&sSockListen, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpSockConnect
     */
    sAddrLen = (acp_sock_len_t)sizeof(sAddr6);
    sRC = acpSockGetName(&sSockListen,
                         (acp_sock_addr_t *)&sAddr6,
                         &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCreate(&sThread, NULL, testConnectIPv6, &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockAccept(&sSockPeer,
                        &sSockListen,
                        (acp_sock_addr_t *)&sAddr6,
                        &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrJoin(&sThread, &sThrRC);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    /* check thread success */
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sThrRC), IPV6_CONNECT);
    
    /*
     * acpSockPoll
     */
    sRC = acpSockPoll(&sSockPeer, ACP_POLL_IN, ACP_TIME_INFINITE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockRecv(&sSockPeer, sBuffer, sizeof(sBuffer), NULL, 0);
    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    sRC = acpSockClose(&sSockPeer);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockClose(&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return;

    ACP_EXCEPTION(IPV6_ISNOTACTIVE)
    {
        sRC = acpSockClose(&sSockListen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_ERROR(("IPv6 support isn't active. Skip IPv6 test.\n"));
    }
    ACP_EXCEPTION(IPV6_UNREACH)
    {
        sRC = acpSockClose(&sSockListen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_ERROR(("IPv6 addresses are unreachable. "
                   "System is not configured with IPv6.\n"));
    }
    ACP_EXCEPTION(IPV6_ERROR)
    {
        sRC = acpSockClose(&sSockListen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_ERROR(("Error initializing IPv6. Skip IPv6 test.\n"));
    }

    ACP_EXCEPTION(IPV6_TIMEDCONNECT)
    {
        sRC = acpSockClose(&sSockListen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_ERROR(("Error of timed connect. Skip IPv6 test.\n"));
    }        

    ACP_EXCEPTION(IPV6_CONNECT)
    {
        sRC = acpSockClose(&sSockListen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_ERROR(("Error of connect. Skip IPv6 test.\n"));
    }        

    ACP_EXCEPTION_END;
}


acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testMain();
    testIPv6Main();
    
    ACT_TEST_END();

    return 0;
}
