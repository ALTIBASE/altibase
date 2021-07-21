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

#include <act.h>
#include <acp.h>

#define THR_COUNT  32
#define DATA_VALUE 16000

acp_uint16_t gData16[THR_COUNT];
acp_uint32_t gData32[THR_COUNT];

acp_uint32_t gState = 0;

static acp_sint32_t testThreadInc32(acp_uint32_t *aArg)
{
    acp_sint32_t i;

    while (acpAtomicGet32(&gState) == 0)
    {
        acpThrYield();
    }

    for (i = 0; i < DATA_VALUE; i++)
    {
        (void)acpAtomicInc32(aArg);
    }

    return 0;
}

static acp_sint32_t testThreadSet32(acp_uint32_t *aArg)
{
    acp_sint32_t i;

    while (acpAtomicGet32(&gState) == 0)
    {
        acpThrYield();
    }

    for (i = 0; i < DATA_VALUE; i++)
    {
        (void)acpAtomicSet32(aArg, i);
    }

    return 0;
}

void testThreadsFor32bits(void)
{
    acp_rc_t sRC;
    acp_thr_t sThrH[THR_COUNT];
    acp_uint32_t i;

    (void)acpAtomicSet32(&gState, 0);
    for (i = 0; i < THR_COUNT; i++)
    {
        sRC = acpThrCreate(sThrH + i,
                           NULL,
                           (acp_thr_func_t *)testThreadInc32,
                           (void *) (gData32 + (i % (THR_COUNT / 2))));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /* Wait until all threads are ready to start simultaneously */
    acpSleepSec(1);
    (void)acpAtomicSet32(&gState, 1);
    for (i = 0; i < THR_COUNT; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(acpThrJoin(sThrH + i, NULL)));
    }
    for (i = 0; i < THR_COUNT; i++)
    {
        ACT_CHECK_DESC(
            acpAtomicGet32(gData32 + (i % (THR_COUNT / 2))) == (DATA_VALUE * 2),
            ("%d %p: %d %d\n", i, gData32 + (i % (THR_COUNT / 2)),
             gData32[i % (THR_COUNT / 2)], DATA_VALUE * 2
             )
            );
    }

    (void)acpAtomicSet32(&gState, 0);
    for (i = 0; i < THR_COUNT; i++)
    {
        sRC = acpThrCreate(sThrH + i,
                           NULL,
                           (acp_thr_func_t *)testThreadSet32,
                           (void *) (gData32 + (i % (THR_COUNT / 2))));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /* Wait until all threads are ready to start simultaneously */
    acpSleepSec(1);
    (void)acpAtomicSet32(&gState, 1);
    for (i = 0; i < THR_COUNT; i++)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(acpThrJoin(sThrH + i, NULL)));
    }
    for (i = 0; i < THR_COUNT; i++)
    {
        ACT_CHECK_DESC(
            acpAtomicGet32(gData32 + (i % (THR_COUNT / 2))) == DATA_VALUE - 1,
            ("%d %p: %d %d\n", i, gData32 + (i % (THR_COUNT / 2)),
             gData32[i % (THR_COUNT / 2)], DATA_VALUE * 2
             )
            );
    }
}


acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testThreadsFor32bits();

    ACT_TEST_END();

    return 0;
}
