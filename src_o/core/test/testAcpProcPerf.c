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
 * $Id: testAcpProcPerf.c 6718 2009-07-30 08:54:34Z djin $
 ******************************************************************************/

/*******************************************************************************
 * acpProc Performance Test Source
 * Test 1. generates aProcesses processes and waits for all children.
 *  Children shall generate a random number K from 10 to 60
 *  and sleeps for K seconds.
 * Test 2. generates aProcesses processes and kills them all.
 *  Children shall wait for 60seconds waiting on the guillotin.
 ******************************************************************************/

#include <act.h>
#include <acpCStr.h>
#include <acpProc.h>
#include <acpRand.h>
#include <acpPrintf.h>
#include <acpSleep.h>
#include <acpStr.h>
#include <acpTime.h>

#define TEST_LOOP       10
#define TEST_PROC_COUNT 32

/* A large number of children to be created */
void testAcpProcChildren(acp_char_t *aCommand)
{
    acp_proc_t      sProc;
    acp_proc_wait_t sWR;
    acp_rc_t        sRC;

    acp_char_t    sParam[10];
    acp_char_t   *sArgvs[4] = {"0", sParam, NULL};
    acp_sint32_t  i;

    for (i = 0; i < TEST_PROC_COUNT; i++)
    {
        /* Generates children */
        (void)acpSnprintf(sParam, 10, "%d", i);

        sRC = acpProcForkExec(&sProc, aCommand, sArgvs, ACP_FALSE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_PROC_COUNT; i++)
    {
        /* Wait for children */
        sRC = acpProcWait(NULL, &sWR, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpProcWait(NULL, &sWR, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_ECHILD(sRC),
                   ("All Children exited. "
                    "acpProcWait should be ACP_RC_ECHILD"));
}

void testAcpProcChildrenWithGuillotin(acp_char_t *aCommand)
{
    acp_proc_t      sProc[TEST_PROC_COUNT];
    acp_proc_wait_t sWR;
    acp_rc_t        sRC;

    acp_char_t    sParam[10];
    acp_char_t   *sArgvs[4] = {"1", sParam, NULL};
    acp_sint32_t  i;

    for (i = 0; i < TEST_PROC_COUNT; i++)
    {
        /* Generates children */
        (void)acpSnprintf(sParam, 10, "%d", i);

        sRC = acpProcForkExec(&sProc[i], aCommand, sArgvs, ACP_FALSE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    acpSleepMsec(500);

    for (i = 0; i < TEST_PROC_COUNT; i++)
    {
        /* Killing children */
        sRC = acpProcKill(&sProc[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcKill have to return ACP_RC_SUCCESS but "
                        "%d returned with child process %d.", sRC, i));
    }

    for (i = 0; i < TEST_PROC_COUNT; i++)
    {
        /* Wait for children */
        sRC = acpProcWait(NULL, &sWR, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC((SIGKILL == ACP_PROC_WR_RETURNCODE(sWR)),
                       ("Exitcode must be SIGKILL but "
                        "%X with child process %d.",
                        ACP_PROC_WR_RETURNCODE(sWR), i));
    }

    sRC = acpProcWait(NULL, &sWR, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_ECHILD(sRC),
                   ("No children running. "
                    "acpProcWait should be ACP_RC_ECHILD"));
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_sint32_t sRC = 0;
    acp_sint32_t sSign;
    acp_uint32_t sSeed = 0;

    ACT_TEST_BEGIN();

    if (3 == aArgc)
    {
        acp_sint32_t sToSleep;
        acp_sint32_t i;
        (void)acpCStrToInt32(aArgv[2], 10, &sSign,
                             (acp_uint32_t*)&sRC, 10, NULL);

        switch(aArgv[1][0])
        {
            case '0':
                /* $ testAcpProcPerf 0 N
                 * This child process exits with exitcode N % 256
                 */
                sSeed = acpRandSeedAuto();
                sToSleep = acpRand(&sSeed) % 50 + 10;

                for(i=0; i<sToSleep; i++)
                {
                    acpSleepMsec(1);
                }
                /* (void)acpProcExit(sRC); */
                break;

            case '1':
                /* $ testAcpProcPerf 1 N
                 * This child process never exits. Must be killed.
                 */

                /* Intentional Inifnite-Loop */
                while(ACP_TRUE)
                {
                    acpSleepSec(1);
                }
        }
    }
    else
    {
        acp_sint32_t i;

        for (i = 0; i < TEST_LOOP; i++)
        {
            testAcpProcChildren(aArgv[0]);
        }

        for (i = 0; i < TEST_LOOP; i++)
        {
            testAcpProcChildrenWithGuillotin(aArgv[0]);
        }
    }

    ACT_TEST_END();

    return sRC;
}
