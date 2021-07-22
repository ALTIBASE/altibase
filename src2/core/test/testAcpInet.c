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
 * $Id: testAcpInet.c 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpByteOrder.h>
#include <acpInet.h>
#include <acpMem.h>
#include <acpSock.h>

/* Service port number for test acpInetGetServInfo */
#define TEST_INET_SERV_PORT 22
#define TEST_INET_SERV_PORT_STR "22"

void testAcpInetGetNameInfo(void)
{
    acp_rc_t sRC;
    acp_char_t sNameBuf[ACP_INET_NI_MAXHOST];
    acp_sock_addr_in_t sSockAddr;
#if defined(ACP_AF_INET6)
    acp_sock_addr_in6_t sSockAddr6;
#endif
    
    /*
     * acpInetGetNameInfo
     */
    
    acpMemSet(&sSockAddr, 0, sizeof(acp_sock_addr_t));
    sSockAddr.sin_family = AF_INET;
    sRC = acpInetStrToAddr(ACP_AF_INET, "127.0.0.1", &sSockAddr.sin_addr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpInetGetNameInfo((acp_sock_addr_t *)&sSockAddr,
                             sizeof(acp_sock_addr_t),
                             sNameBuf, ACP_INET_NI_MAXHOST,
                             ACP_INET_NI_NUMERICHOST);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    
#if defined(AF_INET6)
    acpMemSet(&sSockAddr6, 0, sizeof(struct sockaddr_in6));
    sSockAddr6.sin6_family = ACP_AF_INET6;
    sRC = acpInetStrToAddr(ACP_AF_INET6, "::1", &sSockAddr6.sin6_addr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpInetGetNameInfo((acp_sock_addr_t *)&sSockAddr6,
                             sizeof(struct sockaddr_in6),
                             sNameBuf, ACP_INET_NI_MAXHOST,
                             ACP_INET_NI_NUMERICHOST);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
#endif
}


void testAcpInetGetAddrInfo(void)
{
    acp_rc_t sRC;
    acp_inet_addr_info_t *sInfo;
    acp_uint32_t sAddr = 0;
    acp_uint32_t sPort = 0;

    /*
     * acpInetGetAddrInfo
     */
    sRC = acpInetGetAddrInfo(&sInfo, "127.0.0.1", NULL,
                             0,
                             ACP_INET_AI_PASSIVE,
                             ACP_AF_INET);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acp_sock_addr_in_t *sAddr4 = (acp_sock_addr_in_t *)sInfo->ai_addr;
        acp_bool_t sResult = ACP_FALSE;
            
        while (sInfo != NULL)
        {
            if ((acp_uint32_t)(sAddr4->sin_addr.s_addr) ==
                ACP_TON_BYTE4_ARG(INADDR_LOOPBACK))
            {
                sResult = ACP_TRUE;
                break;
            }
            else
            {
                sInfo = sInfo->ai_next;
            }
        }
        ACT_CHECK_DESC(sResult == ACP_TRUE,
                       ("Getting address fails"));

        acpInetFreeAddrInfo(sInfo);
    }
    else
    {
        /* do nothing */
    }

    /*
     * acpInetGetAddrInfo
     */

    sRC = acpInetGetAddrInfo(&sInfo, "127.0.0.1", NULL,
                             0,
                             ACP_INET_AI_CANONNAME,
                             ACP_AF_INET);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acp_bool_t sResult = ACP_FALSE;
            
        while (sInfo != NULL)
        {
            if (acpCStrLen(sInfo->ai_canonname, 128) > 0)
            {
                sResult = ACP_TRUE;
                break;
            }
            else
            {
                sInfo = sInfo->ai_next;
            }
        }
        ACT_CHECK_DESC(sResult == ACP_TRUE,
                       ("Getting address fails"));

        acpInetFreeAddrInfo(sInfo);
    }
    else
    {
        /* do nothing */
    }

    sRC = acpInetGetAddrInfo(&sInfo, "127.0.0.1", NULL,
                             0,
                             ACP_INET_AI_PASSIVE,
                             0xFF); /* incorrect */
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    /*
     * BUG-30078
     * Return value differs from system to system. Not always EAI_NONAME
     * Anyway, NOT SUCCESS. Disabling this test
     */
#if 0
    sRC = acpInetGetAddrInfo(&sInfo, "127.0.0.", NULL, /* incorrect */
                             ACP_INET_AI_PASSIVE | ACP_INET_AI_NUMERICHOST,
                             ACP_AF_INET);
    ACT_CHECK(ACP_RC_IS_EAI_NONAME(sRC));
#endif

    /*
     * acpInetGetAddrInfo
     */
    sRC = acpInetGetAddrInfo(&sInfo, "127.0.0.1", TEST_INET_SERV_PORT_STR,
                             ACP_SOCK_STREAM,
                             ACP_INET_AI_PASSIVE,
                             ACP_AF_INET);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acp_sock_addr_in_t *sAddr4 = (acp_sock_addr_in_t *)sInfo->ai_addr;
        acp_bool_t sResult = ACP_FALSE;

        while (sInfo != NULL)
        {
            sAddr = (acp_uint32_t)sAddr4->sin_addr.s_addr;
            sPort = (acp_uint32_t)sAddr4->sin_port;

            if (sAddr == ACP_TON_BYTE4_ARG(INADDR_LOOPBACK) &&
                sPort == ACP_TON_BYTE2_ARG(TEST_INET_SERV_PORT))
            {
                sResult = ACP_TRUE;
                break;
            }
            else
            {
                sInfo = sInfo->ai_next;
            }
        }
        ACT_CHECK_DESC(sResult == ACP_TRUE,
                       ("Getting address with service fails"));

        acpInetFreeAddrInfo(sInfo);
    }
    else
    {
        /* do nothing */
    }

    return;
}


#if defined(ACP_AF_INET6)
void testAcpInetGetAddrInfo6(void)
{
    acp_rc_t sRC;
    acp_inet_addr_info_t *sInfo;

    sRC = acpInetGetAddrInfo(&sInfo, "::1", NULL,
                             0,
                             ACP_INET_AI_PASSIVE,
                             ACP_AF_INET6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acp_sock_addr_in6_t *sAddr6 = (acp_sock_addr_in6_t *)sInfo->ai_addr;
        acp_bool_t sResult = ACP_FALSE;
        struct in6_addr sAddr6Loopback;

        sRC = acpInetStrToAddr(ACP_AF_INET6, "::1", &sAddr6Loopback);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpInetStrToAddr should return SUCCESS but %d", sRC));

        while (sInfo != NULL)
        {
            if (acpMemCmp(&(sAddr6->sin6_addr), &sAddr6Loopback,
                          sizeof(struct in6_addr)) == 0)
            {
                sResult = ACP_TRUE;
                break;
            }
            else
            {
                sInfo = sInfo->ai_next;
            }
        }
        ACT_CHECK_DESC(sResult == ACP_TRUE,
                       ("Getting address fails"));
        
        acpInetFreeAddrInfo(sInfo);
    }
    else
    {
        /* do nothing */
    }


    sRC = acpInetGetAddrInfo(&sInfo, "::1", NULL,
                             0,
                             ACP_INET_AI_CANONNAME,
                             ACP_AF_INET6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acp_bool_t sResult = ACP_FALSE;

        while (sInfo != NULL)
        {
            if (acpCStrLen(sInfo->ai_canonname, 128) > 0)
            {
                sResult = ACP_TRUE;
                break;
            }
            else
            {
                sInfo = sInfo->ai_next;
            }
        }
        ACT_CHECK_DESC(sResult == ACP_TRUE,
                       ("Getting address fails"));
        
        acpInetFreeAddrInfo(sInfo);
    }
    else
    {
        /* do nothing */
    }

    sRC = acpInetGetAddrInfo(&sInfo, "::1", NULL,
                             0,
                             ACP_INET_AI_PASSIVE,
                             0xFF);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
    
    sRC = acpInetGetAddrInfo(&sInfo, "::1", NULL,
                             0,
                             0xFFFF, /* incorrect */
                             ACP_AF_INET);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
    
    sRC = acpInetGetAddrInfo(&sInfo, "::::1", NULL,
                             0,
                             0xFFFF, /* incorrect */
                             ACP_AF_INET);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));


    return;
}
#endif

void testAcpInetGetServInfo(void)
{
    acp_rc_t sRC;
    acp_char_t sServBuf[ACP_INET_NI_MAXSERV] = {0};
    acp_sock_addr_in_t sSockAddr;

    acpMemSet(&sSockAddr, 0, sizeof(acp_sock_addr_t));

    sSockAddr.sin_family = ACP_AF_INET;
    sSockAddr.sin_port = ACP_TON_BYTE2_ARG(TEST_INET_SERV_PORT);
    sRC = acpInetStrToAddr(ACP_AF_INET, "127.0.0.1", &sSockAddr.sin_addr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));


    sRC = acpInetGetServInfo((acp_sock_addr_t *)&sSockAddr,
                             sizeof(acp_sock_addr_t),
                             sServBuf, ACP_INET_NI_MAXSERV,
                             ACP_INET_NI_NUMERICSERV);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

void testAcpInetGetInfo(void)
{
    testAcpInetGetNameInfo();
    testAcpInetGetAddrInfo();
    testAcpInetGetServInfo();
#if defined(ACP_AF_INET6)
    testAcpInetGetAddrInfo6();
#endif
}


void testAcpInetGetHost(void)
{
    acp_rc_t sRC;
    acp_inet_hostent_t *sHostEnt;
#if defined(ACP_AF_INET6)
    struct in6_addr sAddr6;
#endif
    struct in_addr  sAddr4 = ACP_SOCK_INADDR_INITIALIZER;
    
    /*
     * acpInetGetHostByName
     */
    sRC = acpInetGetHostByName(&sHostEnt, "localhost");
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK_DESC(
        *(acp_uint32_t *)(sHostEnt->h_addr_list[0]) ==
                          ACP_TON_BYTE4_ARG(INADDR_LOOPBACK),
        ("acpInetGetHostByName result should be LOOPBACK(0x7f000001) "
         "but %#08x", *(acp_uint32_t *)(sHostEnt->h_addr_list[0]))
        );

    sRC = acpInetGetHostByName(&sHostEnt, "127.0.0.1");
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK_DESC(
        *(acp_uint32_t *)(sHostEnt->h_addr_list[0]) ==
                          ACP_TON_BYTE4_ARG(INADDR_LOOPBACK),
        ("acpInetGetHostByName result should be LOOPBACK(0x7f000001) "
         "but %#08x", *(acp_uint32_t *)(sHostEnt->h_addr_list[0]))
        );

    sRC = acpInetGetHostByName(&sHostEnt, "name_error");
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

#if defined(ACP_AF_INET6)
# if defined(ALTI_CFG_OS_AIX)
    sRC = acpInetGetHostByName(&sHostEnt, "::1");
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
# else
    /* acpInetGetHostByName does not support IPv6 */
    sRC = acpInetGetHostByName(&sHostEnt, "::1");
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
# endif
#endif

    
    /*
     * acpInetGetHostByAddr
     */

    sRC = acpInetStrToAddr(ACP_AF_INET, "127.0.0.1", &sAddr4);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetStrToAddr should return SUCCESS but %d", sRC));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "::1", &sAddr6);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetStrToAddr should return SUCCESS but %d", sRC));
    
    sHostEnt = NULL;
    sRC = acpInetGetHostByAddr(&sHostEnt, (acp_sock_addr_in_t *)&sAddr4,
                               4, AF_INET);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetStrToAddr should return SUCCESS but %d", sRC));
    ACT_CHECK_DESC(
        *(acp_uint32_t *)(sHostEnt->h_addr_list[0]) ==
                          ACP_TON_BYTE4_ARG(INADDR_LOOPBACK),
        ("acpInetGetHostByName result should be LOOPBACK(0x7f000001) "
         "but %#08x", *(acp_uint32_t *)(sHostEnt->h_addr_list[0]))
        );
    /*
     * BUG-30255
     * This is not guaranteed.
     * Some machines return "localhost",
     * but other machines return their own hostname
     */
#if 0
    ACT_CHECK_DESC(acpCStrCmp(sHostEnt->h_name,
                              "localhost",
                              acpCStrLen("localhost", 128)) == 0,
                   ("acpInetGetHostByAddr should return \"localhost\""
                    " but %s was returned", sHostEnt->h_name));
#endif

#if defined(AF_INET6)
    sRC = acpInetStrToAddr(ACP_AF_INET6, "::1", &sAddr6);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetStrToAddr should return SUCCESS but %d", sRC));
    sRC = acpInetGetHostByAddr(&sHostEnt, (acp_sock_addr_in_t *)&sAddr6,
                               16, AF_INET6);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
    
    sRC = acpInetStrToAddr(ACP_AF_INET, "127.0.0.1", &sAddr4);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetStrToAddr should return SUCCESS but %d", sRC));
    sRC = acpInetGetHostByAddr(&sHostEnt, (acp_sock_addr_in_t *)&sAddr4,
                               16, AF_INET6);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
#endif

    return;
}


void testIPv6Macros(void)
{
    acp_sock_in6_addr_t sAddr6;
    acp_rc_t sRC;
    
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_UNSPECIFIED(&in6addr_any));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_LOOPBACK(&in6addr_loopback));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "FFFF:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_MULTICAST(&sAddr6));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "FE80:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_LINKLOCAL(&sAddr6));
    
    sRC = acpInetStrToAddr(ACP_AF_INET6, "FEC0:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_SITELOCAL(&sAddr6));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "::FFFF:192.1.2.3", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_V4MAPPED(&sAddr6));
    
    sRC = acpInetStrToAddr(ACP_AF_INET6, "::192.1.2.3", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_V4COMPAT(&sAddr6));
    
    sRC = acpInetStrToAddr(ACP_AF_INET6, "FF01:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_MC_NODELOCAL(&sAddr6));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "FF02:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_MC_LINKLOCAL(&sAddr6));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "FF05:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_MC_SITELOCAL(&sAddr6));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "FF08:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_MC_ORGLOCAL(&sAddr6));

    sRC = acpInetStrToAddr(ACP_AF_INET6, "FF0E:1:2:3:4:5:6:7", &sAddr6);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(ACP_INET_IN6_IS_ADDR_MC_GLOBAL(&sAddr6));
}


void testAddrConv(void)
{
#if defined(AF_INET6)
    struct in6_addr sAddr6;
#endif
    struct in_addr  sAddr4 = ACP_SOCK_INADDR_INITIALIZER;
    acp_rc_t        sRC;
    acp_char_t      sName[100];

    ACT_CHECK(0 == ACP_SOCK_INADDR_GET(sAddr4));
    ACP_SOCK_INADDR_SET(sAddr4, 5);
    ACT_CHECK(5 == ACP_SOCK_INADDR_GET(sAddr4));

    /*
     * acpInetStrToAddr
     */
    sRC = acpInetStrToAddr(ACP_AF_UNIX, "localhost", &sAddr4);
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));

    sRC = acpInetStrToAddr(ACP_AF_INET, "localhost", &sAddr4);
    ACT_CHECK_DESC(ACP_RC_IS_EINVAL(sRC),
                   ("acpInetStrToAddr should return EINVAL but %d", sRC));

    sRC = acpInetStrToAddr(ACP_AF_INET, "127.0.0.1", &sAddr4);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetStrToAddr should return SUCCESS but %d", sRC));
    ACT_CHECK_DESC(
        ACP_SOCK_INADDR_GET(sAddr4) == ACP_TON_BYTE4_ARG(INADDR_LOOPBACK),
        ("acpInetStrToAddr result should be LOOPBACK(0x7f000001) "
         "but %#08x", ACP_TOH_BYTE4_ARG(ACP_SOCK_INADDR_GET(sAddr4)))
        );

#if defined(AF_INET6)
    sRC = acpInetStrToAddr(ACP_AF_INET6, "::1", &sAddr6);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetStrToAddr should return SUCCESS but %d", sRC));
    ACT_CHECK_DESC(acpMemCmp(&sAddr6,
                             &in6addr_loopback,
                             sizeof(struct in6_addr)) == 0,
                   ("acpInetStrToAddr result should be INET6 LOOPBACK"));
#else
    sRC = acpInetStrToAddr(ACP_AF_INET6, "::1", &sAddr4);
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));
#endif

    /*
     * acpInetAddrToStr
     */
    sRC = acpInetAddrToStr(ACP_AF_UNIX, &sAddr4, sName, sizeof(sName));
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));

    sRC = acpInetAddrToStr(ACP_AF_INET, &sAddr4, sName, sizeof(sName));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sName, "127.0.0.1", sizeof(sName)) == 0);

#if defined(AF_INET6)
    sRC = acpInetAddrToStr(ACP_AF_INET6, &sAddr6, sName, sizeof(sName));
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpInetAddrToStr should return SUCCESS but %d", sRC));
    ACT_CHECK(acpCStrCmp(sName, "::1", sizeof(sName)) == 0);
#else
    sRC = acpInetAddrToStr(ACP_AF_INET6, &sAddr4, sName, sizeof(sName));
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));
#endif
}

void testGetStrError(void)
{

    acp_rc_t    sRC;
    acp_char_t *sErrorStr = NULL;

    sRC = acpInetGetStrError(ACP_RC_EAI_AGAIN, NULL);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpInetGetStrError(ACP_RC_EAI_AGAIN, &sErrorStr);

#if defined(AF_INET6)
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sErrorStr != NULL);
#else
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));
    ACT_CHECK(sErrorStr == NULL);
#endif

}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testAddrConv();
    testAcpInetGetHost();
    testAcpInetGetInfo();
    testIPv6Macros();
    testGetStrError();

    ACT_TEST_END();

    return 0;
}
