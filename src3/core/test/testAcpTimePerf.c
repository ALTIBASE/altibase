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
 * $Id: testAcpTimePerf.c 4486 2009-02-10 07:37:20Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpSpinLock.h>
#include <acpSleep.h>
#include <acpThr.h>
#include <acpTime.h>
#include <acpTimePrivate.h>

#define LOOP_COUNT          100000
#define CLIENT_THREAD_COUNT 10

#define TEST_TIME_NOW       1
#define TEST_HIRES_NOW      2

static acp_spin_lock_t gSpinLock = ACP_SPIN_LOCK_INITIALIZER(-1);
static acp_time_t      gTime     = 0;
static acp_uint64_t    gResult   = 0;

static void testAcpTimeNow(acp_sint32_t aType)
{
    acp_time_t   sTime;
    acp_uint64_t sTicks;
    acp_sint32_t i;

    /* calls */
    (void)acpPrintf("thread  %d, loop %d of", 1, LOOP_COUNT);

    if (aType == (acp_sint32_t)TEST_TIME_NOW)
    {
        (void)acpPrintf("      acpTimeNow() : ");
    }
    else
    {
        (void)acpPrintf(" acpTimeHiResNow() : ");
    }

    sTime = acpTimeNow();
    for (i = 0; i < LOOP_COUNT; i++)
    {
        if (aType == (acp_sint32_t)TEST_TIME_NOW)
        {
            (void)acpTimeNow();
        }
        else
        {
            (void)acpTimeHiResNow();
        }
    }
    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("%7lld (calls/sec),",
                    ACP_SINT64_LITERAL(LOOP_COUNT*1000000)/sTime);

    /* cycles */
    sTicks = acpTimeTicks();
    for (i = 0; i < LOOP_COUNT; i++)
    {
        if (aType == (acp_sint32_t)TEST_TIME_NOW)
        {
            (void)acpTimeNow();
        }
        else
        {
            (void)acpTimeHiResNow();
        }
    }
    sTicks = acpTimeTicks() - sTicks;

    (void)acpPrintf("%5lld (cycles/call)\n", sTicks/LOOP_COUNT);
}

static acp_sint32_t testClientThread(void *aArg)
{
    acp_time_t   sTime;
    acp_uint64_t sTicks;
    acp_sint32_t i;
    acp_sint32_t sType = *(acp_sint32_t *)aArg;

    sTime = acpTimeNow();
    for (i = 0; i < LOOP_COUNT; i++)
    {
        if (sType == (acp_sint32_t)TEST_TIME_NOW)
        {
            (void)acpTimeNow();
        }
        else
        {
            (void)acpTimeHiResNow();
        }
    }
    sTime = acpTimeNow() - sTime;

    acpSpinLockLock(&gSpinLock);

    gTime += sTime;

    acpSpinLockUnlock(&gSpinLock);

    sTicks = acpTimeTicks();
    for (i = 0; i < LOOP_COUNT; i++)
    {
        if (sType == (acp_sint32_t)TEST_TIME_NOW)
        {
            (void)acpTimeNow();
        }
        else
        {
            (void)acpTimeHiResNow();
        }
    }
    sTicks = acpTimeTicks() - sTicks;

    acpSpinLockLock(&gSpinLock);

    gResult += sTicks;

    acpSpinLockUnlock(&gSpinLock);

    return 0;
}

static void testMultipleClient(acp_sint32_t aType)
{
    acp_thr_t    sClient[CLIENT_THREAD_COUNT];
    acp_rc_t     sRC;
    acp_sint32_t i;
    acp_sint32_t sType = aType;

    for (i = 0; i < CLIENT_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sClient[i], NULL, testClientThread, &sType);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < CLIENT_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sClient[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    (void)acpPrintf("thread %2d, loop %d of      acpTimeNow() : "
                    "%7lld (calls/sec), %5lld (cycles/call)\n",
                    CLIENT_THREAD_COUNT,
                    LOOP_COUNT,
                    (ACP_SINT64_LITERAL(LOOP_COUNT*1000000)/gTime)/
                    CLIENT_THREAD_COUNT,
                    gResult/(CLIENT_THREAD_COUNT*LOOP_COUNT));
}

static void testTimeTicks(void)
{
    acp_time_t   sTime;
    acp_sint32_t i;

    sTime = acpTimeNow();

    for (i = 0; i < LOOP_COUNT; i++)
    {
        (void)acpTimeTicks();
    }

    sTime = acpTimeNow() - sTime;

    (void)acpPrintf("loop %d, acpTimeTicks uses %lld usecs\n",
                    LOOP_COUNT,
                    acpTimeGetUsec(sTime));
}

acp_sint32_t main(void)
{
    /* single client */
    testAcpTimeNow(TEST_TIME_NOW);
    testAcpTimeNow(TEST_HIRES_NOW);

    /* multiple client */
    testMultipleClient(TEST_TIME_NOW);
    testMultipleClient(TEST_HIRES_NOW);

    /* test acpTimeTicks Performance */
    testTimeTicks();

    return 0;
}
