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
 * $Id:
 ******************************************************************************/

/* A mimic of ifconfig */

#define SAMPLE_MAX_IP (16)

#include <acpInet.h>
#include <acpMem.h>
#include <acpSys.h>
#include <acpPrintf.h>
#include <acpInit.h>

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_rc_t        sRC;
    acp_uint32_t    sCount;
    acp_sint32_t    i;
    acp_ip_addr_t*  sIP = NULL;
    acp_char_t      sName[100];

    ACP_UNUSED(aArgc);
    
    sRC = acpInitialize();
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    sRC = acpSysGetIPAddress(NULL, 0, &sCount);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), IP_FAIL);

    (void)acpPrintf("Number of NICs : %u\n", sCount);
    sRC = acpMemAlloc((void*)(&sIP), sizeof(acp_ip_addr_t) * sCount);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), IP_FAIL);

    sRC = acpSysGetIPAddress(sIP, sCount, &sCount);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), IP_FAIL);

    for(i = 0; i < (acp_sint32_t)sCount; i++)
    {
        sRC = acpInetAddrToStr(
            ACP_AF_INET,
            &(sIP[i].mAddr),
            sName, sizeof(sName)
            );
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpPrintf("IP Address [%3d] : %s\n", i, sName);
        }
        else
        {
            (void)acpPrintf("Cannot convert IP address to string! [%3d]\n", i);
        }
    }
    acpMemFree(sIP);

    (void)acpFinalize();

    return 0;


    ACP_EXCEPTION(PROG_FAIL)
    {
        /* Nothing to handle
         * Print error message */
        (void)acpPrintf("Cannot initialize application %s\n", aArgv[0]);
    }

    ACP_EXCEPTION(IP_FAIL)
    {
        /* Nothing to handle
         * Print error message */
        (void)acpPrintf("Cannot get ip addresses : %d\n", (acp_sint32_t)sRC);

        if(NULL != sIP)
        {
            acpMemFree(sIP);
        }
        else
        {
            /* Do nothing */
        }
    }

    ACP_EXCEPTION_END;
        (void)acpFinalize();

    return 1;
}
