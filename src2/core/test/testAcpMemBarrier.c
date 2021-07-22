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
 * $Id: testAcpMemBarrier.c 10912 2010-04-23 01:44:15Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpStr.h>
#include <acpMemBarrier.h>
#include <acpTime.h>
#include <acpPrintf.h>
#include <acpProc.h>
#include <acpRand.h>

#define TESTACPMEMARRAYSIZE (3145728)
acp_sint32_t gLargeArray[TESTACPMEMARRAYSIZE];
acp_uint32_t gSeed = 0;

void testMemPrefetch(acp_std_file_t* aFile, acp_uint32_t aSize)
{
    acp_time_t sBefore;
    acp_time_t sAfter;

    acp_uint32_t i;
    acp_uint32_t sSize = (acp_uint32_t)(aSize * sizeof(acp_sint32_t));

    acp_time_t sInit;
    acp_time_t sSeqP;
    acp_time_t sSeqW;
    acp_time_t sRandP;
    acp_time_t sRandW;

    /* Initialize Large Array */
    sBefore = acpTimeNow();
    for(i=0; i < aSize; i++)
    {
        gLargeArray[i] = 0;
    }
    sAfter = acpTimeNow();
    sInit = sAfter - sBefore;

    /* Without Prefetch - Sequential */
    sBefore = acpTimeNow();
    for(i=0; i < aSize; i++)
    {
        gLargeArray[i]++;
    }
    sAfter = acpTimeNow();
    sSeqW = sAfter - sBefore;

    /* With Prefetch */
    sBefore = acpTimeNow();
    ACP_MEM_PREFETCH(gLargeArray);
    for(i=0; i < aSize; i++)
    {
        gLargeArray[i]++;
    }
    sAfter = acpTimeNow();
    sSeqP = sAfter - sBefore;

    /* Without Prefetch - Random2 */
    sBefore = acpTimeNow();
    for(i=0; i < aSize; i++)
    {
        gLargeArray[acpRand(&gSeed) % aSize]++;
    }
    sAfter = acpTimeNow();
    sRandW = sAfter - sBefore;

    /* With Prefetch */
    sBefore = acpTimeNow();
    ACP_MEM_PREFETCH(gLargeArray);
    for(i=0; i < aSize; i++)
    {
        gLargeArray[acpRand(&gSeed) % aSize]++;
    }
    sAfter = acpTimeNow();
    sRandP = sAfter - sBefore;

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFprintf(
                aFile,
                "[%10u] [%6llu] [%8llu/%8llu] [%8llu/%8llu]\n",
                sSize, sInit,
                sSeqW, sSeqP,
                sRandW, sRandP
        )));
}

acp_sint32_t main(void)
{
    acp_uint32_t sTestSize[] = 
    {
        16, 32, 64, 128, 256, 512, 768, 1024
    };

    acp_sint32_t i;
    acp_sint32_t j;
    acp_rc_t sRC;
    acp_std_file_t sFile;
    acp_char_t sBuffer[128];
    acp_str_t sFilename = ACP_STR_CONST(sBuffer);

    ACT_TEST_BEGIN();


    gSeed = acpRandSeedAuto();

    for(j=0; j<10; j++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(acpSnprintf(
                    sBuffer, 128, "testAcpMemBarrier%d.log", j)));
        ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, sBuffer, "w")));

        ACT_CHECK(ACP_RC_IS_SUCCESS(acpFprintf(
            &sFile,
            "[   Size   ] [ Init ] [   Sequential    ] [     Random      ]\n"
            "                      [Non-Pref/Prefetch] [Non-Pref/Prefetch]\n"
            )));
        for(i=0; i<8; i++)
        {
            testMemPrefetch(&sFile, sTestSize[i] * 1024);
        }

        sRC = acpStdClose(&sFile);
    }

    ACP_STR_FINAL(sFilename);

    /* Just call barrier functions to check they exist */
    acpMemBarrier();
    acpMemRBarrier();
    acpMemWBarrier();
    
    ACT_TEST_END();

    return 0;
}
