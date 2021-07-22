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
 
/******************************************************************************
 * $Id: testAclStack.c 1878 2008-03-03 05:34:42Z shsuh $
 ******************************************************************************/

#include <act.h>
#include <acpSys.h>
#include <aclStack.h>


#if !defined(__INSURE__)
#define RESULT_ARRAY_SIZE 1024
#define THREAD_COUNT      4
#define SAMPLE_COUNT      100
#else
#define RESULT_ARRAY_SIZE 512
#define THREAD_COUNT      2
#define SAMPLE_COUNT      10
#endif

#define LOOP_COUNT        (SAMPLE_COUNT * RESULT_ARRAY_SIZE)


static acl_stack_t   gStack;
static acp_uint32_t  gResult[THREAD_COUNT][RESULT_ARRAY_SIZE];

static act_barrier_t gBarrier;
static acp_bool_t    gStopFlag;

static acp_bool_t    gTimeCheck;


static acp_sint32_t testProducer(void *aArgs)
{
    acp_rc_t     sRC;
    acp_uint32_t sThrId = (acp_uint32_t)((acp_ulong_t)aArgs);
    acp_ulong_t  i;

    ACT_BARRIER_TOUCH_AND_WAIT(&gBarrier, THREAD_COUNT * 2 + 1);

    for (i = sThrId * LOOP_COUNT; i < (sThrId + 1) * LOOP_COUNT; i++)
    {
        sRC = aclStackPush(&gStack, (void *)i);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("aclStackPush returned error %d (%u)", sRC, sThrId));
    }

    return 0;
}

static acp_sint32_t testConsumer(void *aArgs)
{
    acp_rc_t     sRC;
    acp_uint32_t sThrId      = (acp_uint32_t)((acp_ulong_t)aArgs);
    acp_ulong_t  sValue      = 0;

    ACT_BARRIER_TOUCH_AND_WAIT(&gBarrier, THREAD_COUNT * 2 + 1);

    while (1)
    {
        sRC = aclStackPop(&gStack, (void **)&sValue);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            gResult[sThrId][sValue % RESULT_ARRAY_SIZE]++;
        }
        else if (ACP_RC_IS_ENOENT(sRC))
        {
            if (gStopFlag == ACP_TRUE)
            {
                break;
            }
            else
            {
                /* continue */
            }
        }
        else
        {
            ACT_ERROR(("aclStackPop returned error %d (%u)", sRC, sThrId));
        }
    }

    return 0;
}

static acp_sint32_t testRacer(void *aArgs)
{
    acp_rc_t     sRC;
    acp_uint32_t sThrId      = (acp_uint32_t)((acp_ulong_t)aArgs);
    acp_ulong_t  sValue      = 0;
    acp_ulong_t  i;

    ACT_BARRIER_TOUCH_AND_WAIT(&gBarrier, THREAD_COUNT + 1);

    for (i = sThrId * LOOP_COUNT; i < (sThrId + 1) * LOOP_COUNT; i++)
    {
        sRC = aclStackPush(&gStack, (void *)i);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("aclStackPush returned error %d (%u)", sRC, sThrId));

        sRC = aclStackPop(&gStack, (void **)&sValue);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            gResult[sThrId][sValue % RESULT_ARRAY_SIZE]++;
        }
        else if (ACP_RC_IS_ENOENT(sRC))
        {
            /* continue */
        }
        else
        {
            ACT_ERROR(("aclStackPop returned error %d (%u)", sRC, sThrId));
        }
    }

    ACT_BARRIER_TOUCH(&gBarrier);

    while (1)
    {
        sRC = aclStackPop(&gStack, (void **)&sValue);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            gResult[sThrId][sValue % RESULT_ARRAY_SIZE]++;
        }
        else if (ACP_RC_IS_ENOENT(sRC))
        {
            if (gStopFlag == ACP_TRUE)
            {
                break;
            }
            else
            {
                /* continue */
            }
        }
        else
        {
            ACT_ERROR(("aclStackPop returned error %d (%u)", sRC, sThrId));
        }
    }

    return 0;
}

static void testInit(void)
{
    ACT_BARRIER_INIT(&gBarrier);

    acpMemSet(gResult, 0, sizeof(gResult));

    gStopFlag = ACP_FALSE;
}

static acp_time_t testUnidirectional(void)
{
    acp_thr_t      sProducerThr[THREAD_COUNT];
    acp_thr_t      sConsumerThr[THREAD_COUNT];
    acp_thr_attr_t sAttr;
    acp_time_t     sTime;
    acp_rc_t       sRC;
    acp_ulong_t    i;

    sRC = acpThrAttrCreate(&sAttr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrAttrSetBound(&sAttr, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sProducerThr[i], &sAttr, testProducer, (void *)i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sConsumerThr[i], &sAttr, testConsumer, (void *)i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    ACT_BARRIER_WAIT(&gBarrier, THREAD_COUNT * 2);

    sTime = acpTimeNow();

    ACT_BARRIER_TOUCH(&gBarrier);

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sProducerThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    gStopFlag = ACP_TRUE;

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sConsumerThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return acpTimeNow() - sTime;
}

static acp_time_t testBidirectional(void)
{
    acp_thr_t      sThr[THREAD_COUNT];
    acp_thr_attr_t sAttr;
    acp_time_t     sTime;
    acp_rc_t       sRC;
    acp_ulong_t    i;

    sRC = acpThrAttrCreate(&sAttr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrAttrSetBound(&sAttr, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], &sAttr, testRacer, (void *)i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    ACT_BARRIER_WAIT(&gBarrier, THREAD_COUNT);

    sTime = acpTimeNow();

    ACT_BARRIER_TOUCH(&gBarrier);

    ACT_BARRIER_WAIT(&gBarrier, THREAD_COUNT * 2 + 1);

    gStopFlag = ACP_TRUE;

    for (i = 0; i < THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return acpTimeNow() - sTime;
}

static void testCheckResult(const acp_char_t *aTestName)
{
    acp_bool_t   sIsAllOk = ACP_TRUE;
    acp_uint32_t sResult;
    acp_uint32_t i;
    acp_uint32_t j;

    ACT_CHECK_DESC(aclStackIsEmpty(&gStack) == ACP_TRUE,
                   ("Stack is not empty in %s", aTestName));

    for (i = 0; i < RESULT_ARRAY_SIZE; i++)
    {
        sResult = 0;

        for (j = 0; j < THREAD_COUNT; j++)
        {
            sResult += gResult[j][i];
        }

        if (sResult != SAMPLE_COUNT * THREAD_COUNT)
        {
            sIsAllOk = ACP_FALSE;
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    if (sIsAllOk == ACP_FALSE)
    {
        ACT_ERROR(("Incorrect Result in %s", aTestName));

        for (i = 0; i < RESULT_ARRAY_SIZE; i++)
        {
            sResult = 0;

            for (j = 0; j < THREAD_COUNT; j++)
            {
                sResult += gResult[j][i];
            }

            (void)acpPrintf(" %d", sResult);

            if ((i + 1) % 16 == 0)
            {
                (void)acpPrintf("\n");
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        /* do nothing */
    }
}


static void testSpinLockStack(void)
{
    acp_time_t sTime = 0;
    acp_rc_t   sRC;

    sRC = aclStackCreate(&gStack, 1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * MultiThread Unidirectional Test
     */
    testInit();
    sTime += testUnidirectional();
    testCheckResult("SpinLock Stack Unidirectional Test");

    /*
     * MultiThread Bidirectional Test
     */
    testInit();
    sTime += testBidirectional();
    testCheckResult("SpinLock Stack Bidirectional Test");

    aclStackDestroy(&gStack);

    if (gTimeCheck == ACP_TRUE)
    {
        (void)acpPrintf("thread %d, loop %d of spinlock stack: %5lld msec\n",
                        THREAD_COUNT,
                        LOOP_COUNT,
                        acpTimeToMsec(sTime));
    }
    else
    {
        /* do nothing */
    }

}

#if !defined(ALTI_CFG_CPU_PARISC)

acp_rc_t aclStackPushLockFree(acl_stack_t *, void *);

static void testLockFreeStack(void)
{
    acp_time_t   sTime = 0;
    acp_uint32_t sCPUCount;
    acp_rc_t     sRC;

    sRC = acpSysGetCPUCount(&sCPUCount);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclStackCreate(&gStack, ((sCPUCount > 1) ? -1 : 2));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * MultiThread Unidirectional Test
     */
    testInit();
    
    sTime += testUnidirectional();
    testCheckResult("LockFree Stack Unidirectional Test");
    
    /*
     * MultiThread Bidirectional Test
     */
    testInit();
    sTime += testBidirectional();
    testCheckResult("LockFree Stack Bidirectional Test");

    aclStackDestroy(&gStack);

    if (gTimeCheck == ACP_TRUE)
    {
        (void)acpPrintf("thread %d, loop %d of lockfree stack: %5lld msec\n",
                        THREAD_COUNT,
                        LOOP_COUNT,
                        acpTimeToMsec(sTime));
    }
    else
    {
        /* do nothing */
    }
}

#endif


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    if (aArgc > 1)
    {
        gTimeCheck = ACP_TRUE;
    }
    else
    {
        gTimeCheck = ACP_FALSE;
    }

    ACT_BARRIER_CREATE(&gBarrier);

    testSpinLockStack();
#if !defined(ALTI_CFG_CPU_PARISC)
    testLockFreeStack();
#endif

    ACT_BARRIER_DESTROY(&gBarrier);

    ACT_TEST_END();

    return 0;
}
