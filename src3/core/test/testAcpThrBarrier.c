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
 * $Id: testAcpThrBarrier.c 11192 2010-06-08 01:24:21Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpAtomic.h>
#include <acpThr.h>
#include <acpThrBarrier.h>
#include <acpSleep.h>


#define TEST_THREAD_COUNT 10

static acp_thr_barrier_t     gTestBarrier;
static volatile acp_sint32_t gTestCounter;
static acp_thr_barrier_t     gBarrier;


static acp_sint32_t testThreadTouch(void *aArg)
{
    acp_rc_t sRC;

    ACP_UNUSED(aArg);

    acpSleepMsec(100);

    (void)acpAtomicInc32(&gTestCounter);

    sRC = acpThrBarrierTouch(&gTestBarrier);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

static acp_sint32_t testThreadTouchAndWait(void *aArg)
{
    acp_rc_t sRC;

    ACP_UNUSED(aArg);

    acpSleepMsec(100);

    (void)acpAtomicInc32(&gTestCounter);

    sRC = acpThrBarrierTouchAndWait(&gTestBarrier, TEST_THREAD_COUNT);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK_DESC(gTestCounter == TEST_THREAD_COUNT,
                   ("gTestCounter should be %d but %d",
                    TEST_THREAD_COUNT,
                    gTestCounter));

    return 0;
}

static acp_sint32_t testBarrierThread(void* aParam)
{
    acp_rc_t            sRC;
    acp_thr_barrier_t*  sBarrier = (acp_thr_barrier_t*)aParam;

    sRC = acpThrBarrierTouch(sBarrier);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
    return 0;
}

static acp_sint32_t testBarrierInit(void)
{
    acp_thr_t           sThr;
    acp_rc_t            sRC;

    /*
     * Commented on purpose
    sRC = acpThrBarrierCreate(&gBarrier);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
     */

    acpThrBarrierInit(&gBarrier);

    sRC = acpThrCreate(&sThr, NULL, testBarrierThread, &gBarrier);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrBarrierWait(&gBarrier, 1);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

    sRC = acpThrJoin(&sThr, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * Commented on purpose
    sRC = acpThrBarrierDestroy(&gBarrier);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
     */

    return 0;
}

acp_sint32_t main(void)
{
    acp_rc_t     sRC;
    acp_thr_t    sThr[TEST_THREAD_COUNT];
    acp_sint32_t i;

    ACT_TEST_BEGIN();

    sRC = acpThrBarrierCreate(&gTestBarrier);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpThrBarrierTouch
     */
    acpThrBarrierInit(&gTestBarrier);
    gTestCounter = 0;

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testThreadTouch, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpThrBarrierWait(&gTestBarrier, TEST_THREAD_COUNT);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK_DESC(gTestCounter == TEST_THREAD_COUNT,
                   ("gTestCounter should be %d but %d",
                    TEST_THREAD_COUNT,
                    gTestCounter));

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * acpThrBarrierTouchAndWait
     */
    acpThrBarrierInit(&gTestBarrier);
    gTestCounter = 0;

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testThreadTouchAndWait, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpThrBarrierWait(&gTestBarrier, TEST_THREAD_COUNT);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK_DESC(gTestCounter == TEST_THREAD_COUNT,
                   ("gTestCounter should be %d but %d",
                    TEST_THREAD_COUNT,
                    gTestCounter));

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpThrBarrierDestroy(&gTestBarrier);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    (void)testBarrierInit();

    ACT_TEST_END();

    return 0;
}
