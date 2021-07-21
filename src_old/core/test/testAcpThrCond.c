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
 * $Id: testAcpThrCond.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpMem.h>
#include <acpSleep.h>
#include <acpThr.h>
#include <acpThrCond.h>


#define TEST_THREAD_COUNT 10
#define TEST_WAIT_TIMEOUT 1000
#define TEST_RETRY_COUNT  100

#define TEST_COND_SIGNAL()                              \
    do                                                  \
    {                                                   \
        sRC = acpThrMutexLock(&gMutex);                 \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),          \
                       ("mutex lock error=%d", sRC));   \
                                                        \
        sRC = acpThrCondSignal(&gCond);                 \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),          \
                       ("signal error=%d", sRC));       \
                                                        \
        sRC = acpThrMutexUnlock(&gMutex);               \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),          \
                       ("mutex unlock error=%d", sRC)); \
    } while (0)

#define TEST_COND_BROADCAST()                           \
    do                                                  \
    {                                                   \
        sRC = acpThrMutexLock(&gMutex);                 \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),          \
                       ("mutex lock error=%d", sRC));   \
                                                        \
        sRC = acpThrCondBroadcast(&gCond);              \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),          \
                       ("broadcast error=%d", sRC));    \
                                                        \
        sRC = acpThrMutexUnlock(&gMutex);               \
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),          \
                       ("mutex unlock error=%d", sRC)); \
    } while (0)

#define TEST_WAIT_FOR_SIGNALED(aValue)                          \
    do                                                          \
    {                                                           \
        for (sRetry = 0; sRetry < TEST_RETRY_COUNT; sRetry++)   \
        {                                                       \
            acp_uint32_t sValue;                                \
                                                                \
            acpSleepUsec(TEST_WAIT_TIMEOUT);                    \
                                                                \
            sRC = acpThrMutexLock(&gMutex);                     \
            ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),              \
                           ("mutex lock error=%d", sRC));       \
                                                                \
            sValue = gSignaled;                                 \
                                                                \
            sRC = acpThrMutexUnlock(&gMutex);                   \
            ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),              \
                           ("mutex unlock error=%d", sRC));     \
                                                                \
            if (sValue == (aValue))                             \
            {                                                   \
                break;                                          \
            }                                                   \
            else                                                \
            {                                                   \
            }                                                   \
        }                                                       \
                                                                \
        ACT_CHECK_DESC(gSignaled == (aValue),                   \
                       ("signaled thread should be %u but %u",  \
                        (aValue),                               \
                        gSignaled));                            \
                                                                \
        gSignaled = 0;                                          \
    } while (0)


static acp_thr_cond_t  gCond;
static acp_thr_mutex_t gMutex;

static acp_uint32_t    gSignaled;
static acp_bool_t      gStop;

static acp_sint32_t    gSignaledThread[TEST_THREAD_COUNT];


static acp_sint32_t testCondWait(void *aArg)
{
    acp_sint32_t sID = (acp_sint32_t)(acp_slong_t)aArg;
    acp_rc_t     sRC;

    sRC = acpThrMutexLock(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    gSignaled++;

    do
    {
        sRC = acpThrCondWait(&gCond, &gMutex);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        ACT_CHECK(gSignaledThread[sID] == 0);
        gSignaledThread[sID] = 1;

        gSignaled++;
    } while (gStop != ACP_TRUE);

    sRC = acpThrMutexUnlock(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}


acp_sint32_t main(void)
{
    acp_thr_t       sThr[TEST_THREAD_COUNT];
    acp_rc_t        sRC;
    acp_sint32_t    sRetry;
    acp_sint32_t    i;

    ACT_TEST_BEGIN();

    /*
     * create cond/mutex
     */
    sRC = acpThrCondCreate(&gCond);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrMutexCreate(&gMutex, ACP_THR_MUTEX_DEFAULT);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * condition wakeup
     */
    gSignaled = 0;
    gStop     = ACP_FALSE;

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i],
                           NULL,
                           testCondWait,
                           (void *)(acp_slong_t)i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    TEST_WAIT_FOR_SIGNALED(TEST_THREAD_COUNT);

    acpMemSet(gSignaledThread, 0, sizeof(gSignaledThread));
    TEST_COND_SIGNAL();

    TEST_WAIT_FOR_SIGNALED(1);

    acpMemSet(gSignaledThread, 0, sizeof(gSignaledThread));
    TEST_COND_BROADCAST();

    TEST_WAIT_FOR_SIGNALED(TEST_THREAD_COUNT);

    acpMemSet(gSignaledThread, 0, sizeof(gSignaledThread));
    TEST_COND_SIGNAL();

    TEST_WAIT_FOR_SIGNALED(1);

    gStop = ACP_TRUE;

    acpMemSet(gSignaledThread, 0, sizeof(gSignaledThread));
    TEST_COND_BROADCAST();

    TEST_WAIT_FOR_SIGNALED(TEST_THREAD_COUNT);

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * timed wait
     */
    sRC = acpThrMutexLock(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCondTimedWait(&gCond,
                              &gMutex,
                              acpTimeNow() + TEST_WAIT_TIMEOUT,
                              ACP_TIME_ABS);
    ACT_CHECK(ACP_RC_IS_ETIMEDOUT(sRC));

    sRC = acpThrMutexUnlock(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrMutexLock(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCondTimedWait(&gCond,
                              &gMutex,
                              acpTimeFromUsec(TEST_WAIT_TIMEOUT),
                              ACP_TIME_REL);
    ACT_CHECK(ACP_RC_IS_ETIMEDOUT(sRC));

    sRC = acpThrMutexUnlock(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * destroy cond/mutex
     */
    sRC = acpThrMutexDestroy(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrCondDestroy(&gCond);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_TEST_END();

    return 0;
}
