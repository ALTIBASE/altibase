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
 * $Id: testAcpAtomicPerf.c 5558 2009-05-11 04:29:01Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpAtomic.h>
#include <acpTime.h>

#define TEST_VAL16_0   0
#define TEST_VAL16_1   1
#define TEST_VAL16_2   2

#define TEST_VAL32_0   0
#define TEST_VAL32_1   1
#define TEST_VAL32_2   2

#define TEST_VAL64_0   ACP_SINT64_LITERAL(0)
#define TEST_VAL64_1   ACP_SINT64_LITERAL(1)
#define TEST_VAL64_2   ACP_SINT64_LITERAL(2)

#define TEST_COUNT ACP_SINT16_MAX

static void testAtomic16(volatile acp_uint16_t *aAtomic)
{
    acp_sint16_t i = 0;
    acp_time_t   sTime;

    *aAtomic = TEST_VAL16_0;

    (void)acpPrintf("16 bit atomic operations\n");

    /*
     * set
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicSet16(aAtomic, TEST_VAL16_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tSet16 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * increase
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicInc16(aAtomic);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tInc16 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * decrease
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicDec16(aAtomic);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tDec16 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * add
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicAdd16(aAtomic, TEST_VAL16_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tAdd16 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * subtract
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicSub16(aAtomic, TEST_VAL16_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tSub16 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * compare and swap
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicCas16(aAtomic,
                       TEST_VAL16_1,
                       TEST_VAL16_2);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tCas16 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);
}

static void testAtomic32(volatile acp_uint32_t *aAtomic)
{
    acp_sint32_t i = 0;
    acp_time_t   sTime;

    *aAtomic = TEST_VAL32_0;

    (void)acpPrintf("32 bit atomic operations\n");

    /*
     * set
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicSet32(aAtomic, TEST_VAL32_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tSet32 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * increase
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicInc32(aAtomic);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tInc32 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * decrease
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicDec32(aAtomic);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tDec32 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * add
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicAdd32(aAtomic, TEST_VAL32_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tAdd32 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * subtract
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicSub32(aAtomic, TEST_VAL32_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tSub32 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * compare and swap
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicCas32(aAtomic,
                       TEST_VAL32_1,
                       TEST_VAL32_2);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tCas32 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);
}

static void testAtomic64(volatile acp_uint64_t *aAtomic)
{
    acp_sint64_t i = 0;
    acp_time_t   sTime;

    *aAtomic = TEST_VAL64_0;

    (void)acpPrintf("64 bit atomic operations\n");

    /*
     * set
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)(void)acpAtomicSet64(aAtomic, TEST_VAL64_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tSet64 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * increase
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicInc64(aAtomic);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tInc64 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * decrease
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicDec64(aAtomic);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tDec64 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * add
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicAdd64(aAtomic, TEST_VAL64_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tAdd64 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * subtract
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicSub64(aAtomic, TEST_VAL64_1);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tSub64 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);

    /*
     * compare and swap
     */
    sTime = acpTimeNow();
    for (i = 0; i < TEST_COUNT; i++)
    {
        (void)acpAtomicCas64(aAtomic,
                       TEST_VAL64_1,
                       TEST_VAL64_2);
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("\tCas64 : %lld usecs for %d loops\n",
              sTime, TEST_COUNT);
}

acp_sint32_t main(void)
{
    volatile acp_uint16_t sAtomic16;
    volatile acp_uint32_t sAtomic32;
    volatile acp_uint64_t sAtomic64;

    /* NOTICE
     *  we dont need to test acp_ulong_t based tests
     *  because they are also based on 32bit or 64bit atomic operations */

    ACT_TEST_BEGIN();

    testAtomic16(&sAtomic16);
    testAtomic32(&sAtomic32);
    testAtomic64(&sAtomic64);

    ACT_TEST_END();

    return 0;
}
