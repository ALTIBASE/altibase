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
 * $Id: testAcpSpinLock.c 8779 2009-11-20 11:06:58Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpSpinLock.h>
#include <acpThr.h>


#define THREAD_COUNT 10
#if !defined(__INSURE__)
#define LOOP_COUNT   1000000
#else
#define LOOP_COUNT   1000
#endif


static acp_spin_lock_t gTestLock   = ACP_SPIN_LOCK_INITIALIZER(-1);

static acp_uint32_t    gTestCount1 = 0;
static acp_uint32_t    gTestCount2 = 0;
static acp_uint32_t    gTestCount3 = 0;

static act_barrier_t   gBarrier;


static acp_sint32_t testThread(void *aArg)
{
    acp_sint32_t i;

    ACP_UNUSED(aArg);

    ACT_BARRIER_TOUCH_AND_WAIT(&gBarrier, THREAD_COUNT);

    for (i = 0; i < LOOP_COUNT; i++)
    {
        acpSpinLockLock(&gTestLock);

        if ((gTestCount1 % 2) == 0)
        {
            (void)acpAtomicAdd32(&gTestCount2, 2);
        }
        else
        {
            (void)acpAtomicAdd32(&gTestCount3, 2);
        }

        (void)acpAtomicInc32(&gTestCount1);

        acpSpinLockUnlock(&gTestLock);
    }

    return 0;
}

static void testMT(void)
{
    acp_thr_t    sThr[THREAD_COUNT];
    acp_rc_t     sRC;
    acp_sint32_t i;

    ACT_BARRIER_CREATE(&gBarrier);

    ACT_BARRIER_INIT(&gBarrier);

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testThread, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    ACT_CHECK_DESC(gTestCount1 == THREAD_COUNT * LOOP_COUNT,
                   ("gTestCount1 should be %u but %u",
                    THREAD_COUNT * LOOP_COUNT,
                    gTestCount1));
    ACT_CHECK_DESC(gTestCount2 == THREAD_COUNT * LOOP_COUNT,
                   ("gTestCount2 should be %u but %u",
                    THREAD_COUNT * LOOP_COUNT,
                    gTestCount2));
    ACT_CHECK_DESC(gTestCount3 == THREAD_COUNT * LOOP_COUNT,
                   ("gTestCount3 should be %u but %u",
                    THREAD_COUNT * LOOP_COUNT,
                    gTestCount3));

    ACT_BARRIER_DESTROY(&gBarrier);
}

static void testSimple(void)
{
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_FALSE);

    /*
     * 1
     */
    ACT_CHECK(acpSpinLockTryLock(&gTestLock) == ACP_TRUE);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_TRUE);

    acpSpinLockUnlock(&gTestLock);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_FALSE);

    /*
     * 2
     */
    ACT_CHECK(acpSpinLockTryLock(&gTestLock) == ACP_TRUE);
    ACT_CHECK(acpSpinLockTryLock(&gTestLock) == ACP_FALSE);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_TRUE);

    acpSpinLockUnlock(&gTestLock);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_FALSE);

    /*
     * 3
     */
    ACT_CHECK(acpSpinLockTryLock(&gTestLock) == ACP_TRUE);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_TRUE);

    acpSpinLockUnlock(&gTestLock);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_FALSE);

    /*
     * 4
     */
    ACT_CHECK(acpSpinLockTryLock(&gTestLock) == ACP_TRUE);
    ACT_CHECK(acpSpinLockTryLock(&gTestLock) == ACP_FALSE);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_TRUE);

    acpSpinLockUnlock(&gTestLock);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_FALSE);

    /*
     * 5
     */
    acpSpinLockLock(&gTestLock);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_TRUE);

    acpSpinLockUnlock(&gTestLock);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_FALSE);

    /*
     * 6
     */
    acpSpinLockLock(&gTestLock);
    ACT_CHECK(acpSpinLockTryLock(&gTestLock) == ACP_FALSE);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_TRUE);

    acpSpinLockUnlock(&gTestLock);
    ACT_CHECK(acpSpinLockIsLocked(&gTestLock) == ACP_FALSE);
}


static void testSMP(void)
{

    /* simulation for Uni-Processor system,
     * busy-waiting loop counter becomes non-zero.
     */

    acpSpinWaitSetDefaultSpinCount(
        ACP_SPIN_WAIT_DEFAULT_SPIN_COUNT);

    gTestCount1 = 0;
    gTestCount2 = 0;
    gTestCount3 = 0;

    testSimple();
    testMT();

}

static void testUP(void)
{

    /* simulation for Uni-Processor system,
     * busy-waiting loop counter becomes zero.
     */
    acpSpinWaitSetDefaultSpinCount(0);

    gTestCount1 = 0;
    gTestCount2 = 0;
    gTestCount3 = 0;

    testSimple();
    testMT();

}

#define TEST_SPINCOUNT_1 (100)
#define TEST_SPINCOUNT_2 (200)

void testBasic(void)
{
    acp_spin_lock_t sSpinLock;
    acp_sint32_t    sSpinCount = TEST_SPINCOUNT_1;

    acpSpinLockInit(&sSpinLock, sSpinCount);

    sSpinCount = acpSpinLockGetCount(&sSpinLock);
    ACT_CHECK(sSpinCount == TEST_SPINCOUNT_1);

    acpSpinLockSetCount(&sSpinLock, TEST_SPINCOUNT_2);

    sSpinCount = acpSpinLockGetCount(&sSpinLock);
    ACT_CHECK(sSpinCount == TEST_SPINCOUNT_2);
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testBasic();
    testSMP();
    testUP();

    ACT_TEST_END();

    return 0;
}
