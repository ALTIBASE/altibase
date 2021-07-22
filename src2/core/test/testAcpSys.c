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
 * $Id: testAcpSys.c 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <acpSys.h>


#define MAC_ADDR_COUNT 128
#define IP_ADDR_COUNT  128


static void testHardwareID(void)
{
    acp_rc_t sRC;
    acp_char_t sID[ACP_SYS_ID_MAXSIZE] = {0,};

    sRC = acpSysGetHardwareID(sID, ACP_SYS_ID_MAXSIZE);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetHardwareID failed error code=%d", sRC));
    ACT_CHECK_DESC(acpCStrLen(sID, ACP_SYS_ID_MAXSIZE) > 0,
                   ("acpSysGetHardwareID returns empty string\n"));
}

static void testMacAddr(void)
{
    acp_mac_addr_t sMacAddr[MAC_ADDR_COUNT];
    acp_rc_t       sRC;
    acp_uint32_t   sCount;
    acp_uint32_t   sRealCount;
    acp_uint32_t   i;

    ACT_CHECK_DESC(sizeof(acp_mac_addr_t) == 8,
                   ("sizeof acp_mac_addr_t should be 8"));

    /* try to read one
     * If there is many MACs, it try again to read every MAC. */
    sCount = 1;
    sRC = acpSysGetMacAddress(sMacAddr, sCount, &sRealCount);

    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetMacAddress failed error code=%d", sRC));

    ACT_CHECK_DESC(sRealCount <= MAC_ADDR_COUNT,
                    ("System has %d-MAC addresses,"
                     "but test code can read only %d-addresses",
                     sRealCount, MAC_ADDR_COUNT));

    ACT_CHECK_DESC(sRealCount != 0,
                   ("This system has no MAC address, is it right?"));
    
    if (sRealCount == 0)
    {
        /* System has no MAC */
        return;
    }
    else
    {
        /* do nothing */
    }
    
    if (sCount < sRealCount)
    {
        sCount = sRealCount;
        sRC = acpSysGetMacAddress(sMacAddr,
                                  sCount, &sRealCount);
    }
    else
    {
        /* do nothing */
    }

    ACT_CHECK_DESC(sCount >= sRealCount,
                   ("System has %d-MAC addresses,"
                    "but only %d-addresses have been read",
                    sRealCount, sCount));

    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetMacAddress failed error code=%d", sRC));
    ACT_CHECK_DESC(sCount > 0, ("mac address count should be greater than 0"));

    /* If many data structures are passed more than existing in system,
     * empty data structures must be set as invalid.
     */
    sRC = acpSysGetMacAddress(sMacAddr, MAC_ADDR_COUNT, &sRealCount);

    for (i = 0; i < MAC_ADDR_COUNT; i++)
    {
        if (i < sRealCount)
        {
            ACT_CHECK(sMacAddr[i].mValid == ACP_TRUE);

            ACT_CHECK_DESC((sMacAddr[i].mAddr[0] != 0) ||
                           (sMacAddr[i].mAddr[1] != 0) ||
                           (sMacAddr[i].mAddr[2] != 0) ||
                           (sMacAddr[i].mAddr[3] != 0) ||
                           (sMacAddr[i].mAddr[4] != 0) ||
                           (sMacAddr[i].mAddr[5] != 0),
                           ("returned mac address should not be empty"));
        }
        else
        {
            ACT_CHECK(sMacAddr[i].mValid == ACP_FALSE);
        }
    }
}

static void testIPAddr(void)
{
    acp_ip_addr_t sIPAddr[IP_ADDR_COUNT];
    acp_rc_t      sRC;
    acp_uint32_t  sCount;
    acp_uint32_t  sRealCount;
    acp_uint32_t  i;

    /* check how many IP exists */
    sCount = 1;
    sRC = acpSysGetIPAddress(sIPAddr, sCount, &sRealCount);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetIPAddress failed error code=%d", sRC));
    ACT_CHECK_DESC(sCount > 0, ("ip address count should be greater than 0"));

    ACT_CHECK_DESC(sRealCount < IP_ADDR_COUNT,
                    ("System has %d-IP addresses,"
                     "but test code can read only %d-addresses",
                     sRealCount, IP_ADDR_COUNT));

    /* If there are many IP, it read every IPs */
    if (sCount < sRealCount)
    {
        sCount = sRealCount;
        sRC = acpSysGetIPAddress(sIPAddr,
                                  sCount, &sRealCount);
    }
    else
    {
        /* do nothing */
    }

    ACT_CHECK_DESC(sCount == sRealCount,
                   ("System has %d-IP addresses,"
                    "but %d-addresses have been read",
                    sRealCount, sCount));

    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetIPAddress failed error code=%d", sRC));


    sRC = acpSysGetIPAddress(sIPAddr, IP_ADDR_COUNT, &sCount);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetIPAddress failed error code=%d", sRC));
    ACT_CHECK_DESC(sCount > 0, ("ip address count should be greater than 0"));

    for (i = 0; i < IP_ADDR_COUNT; i++)
    {
        if (i < sCount)
        {
            ACT_CHECK(sIPAddr[i].mValid == ACP_TRUE);

            ACT_CHECK_DESC((sizeof(sIPAddr[i].mAddr) > 0),
                           ("returned ip address should not be empty"));
            ACT_CHECK_DESC((sizeof(sIPAddr[i].mBrdAddr) > 0),
                           ("returned broadcast address should not be empty"));
        }
        else
        {
            ACT_CHECK(sIPAddr[i].mValid == ACP_FALSE);
        }
    }
}

static void testIPv6Addr(void)
{
    acp_ipv6_addr_t sIPv6Addr[IP_ADDR_COUNT];
    acp_rc_t      sRC;
    acp_uint32_t  sCount;
    acp_uint32_t  sRealCount;
    acp_uint32_t  i;

    /* check how many IP exists */
    sCount = 1;
    sRC = acpSysGetIPv6Address(sIPv6Addr, sCount, &sRealCount);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetIPAddress failed error code=%d", sRC));
    /*
     * BUG-30078
     * System may not be configured with IPv6
    ACT_CHECK_DESC(sCount > 0, ("ip address count should be greater than 0"));
     */

    ACT_CHECK_DESC(sRealCount < IP_ADDR_COUNT,
                    ("System has %d-IP addresses,"
                     "but test code can read only %d-addresses",
                     sRealCount, IP_ADDR_COUNT));


    /* If there are not one IP, it read every IPs */
    if (sCount != sRealCount)
    {
        sCount = sRealCount;
        sRC = acpSysGetIPv6Address(sIPv6Addr,
                                   sCount, &sRealCount);
    }
    else
    {
        /* do nothing */
    }

    ACT_CHECK_DESC(sCount == sRealCount,
                   ("System has %d-IP addresses,"
                    "but %d-addresses have been read",
                    sRealCount, sCount));

    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetIPv6Address failed error code=%d", sRC));


    sRC = acpSysGetIPv6Address(sIPv6Addr, IP_ADDR_COUNT, &sCount);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetIPv6Address failed error code=%d", sRC));
    /*
     * BUG-30078
     * System may not be configured with IPv6
    ACT_CHECK_DESC(sCount > 0, ("ip address count should be greater than 0"));
     */

    for (i = 0; i < IP_ADDR_COUNT; i++)
    {
        if (i < sCount)
        {
            ACT_CHECK(sIPv6Addr[i].mValid == ACP_TRUE);

            ACT_CHECK_DESC((sizeof(sIPv6Addr[i].mAddr6) > 0),
                           ("returned ip address should not be empty"));
        }
        else
        {
            ACT_CHECK(sIPv6Addr[i].mValid == ACP_FALSE);
        }
    }

}

static void testCPUCount(void)
{
    acp_rc_t     sRC;
    acp_uint32_t sCPUCount;

    sRC = acpSysGetCPUCount(&sCPUCount);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetCPUCount failed error code=%d", sRC));
    ACT_CHECK_DESC(sCPUCount > 0, ("cpu count should be greater than 0"));
}

static void testPageSize(void)
{
    acp_rc_t   sRC;
    acp_size_t sPageSize;

    sRC = acpSysGetPageSize(&sPageSize);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetPageSize failed error code=%d", sRC));
    ACT_CHECK_DESC((sPageSize > 0) && ((sPageSize % 8) == 0),
                   ("page size should be greater than 0 and aligned to 8"));
}

static void testBlockSize(void)
{
    acp_rc_t   sRC;
    acp_size_t sBlockSize;

    sRC = acpSysGetBlockSize(&sBlockSize);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysBlockSize failed error code=%d", sRC));
    ACT_CHECK_DESC((sBlockSize > 0) && ((sBlockSize % 8) == 0),
                   ("block(cluster) size should be greater than 0 "
                    "and alinged to 8"));
}

static void testHostName(void)
{
    acp_char_t   sHostName[128];
    acp_rc_t     sRC;

    sRC = acpSysGetHostName(sHostName, sizeof(sHostName));
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSysGetHostName failed error code=%d", sRC));
    ACT_CHECK_DESC(acpCStrLen(sHostName, sizeof(sHostName)) > 0,
                   ("host name should not be empty"));
}

acp_sint32_t main(void)
{
    acp_cpu_times_t sCPUTimes;
    acp_time_t      sCPUTime;
    acp_rc_t        sRC;
    acp_uint32_t    i;

    ACT_TEST_BEGIN();

    testIPv6Addr();
    testCPUCount();
    testMacAddr();
    testIPAddr();
    testPageSize();
    testBlockSize();
    testHostName();

    testHardwareID();

    for (i = 0; i < ACP_SINT64_LITERAL(10000000); i++)
    {
        (void)acpProcGetSelfID();
    }

    sRC = acpSysGetCPUTimes(&sCPUTimes);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sCPUTime = sCPUTimes.mUserTime + sCPUTimes.mSystemTime;
    ACT_CHECK_DESC(sCPUTime > 0,
                   ("sCPUTime should be larger than 0, but %d",
                    sCPUTime));

    ACT_TEST_END();

    return 0;
}
