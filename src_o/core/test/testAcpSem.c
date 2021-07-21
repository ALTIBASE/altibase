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
 * $Id: testAcpSem.c 5854 2009-06-02 07:55:10Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpStr.h>
#include <acpSem.h>

#define TEST_SEMAPHORE_COUNT1 1
#define TEST_SEMAPHORE_COUNT2 5
#define TEST_SEMAPHORE_LOOP   10

acp_key_t gTestSemaphoreKey;

static void testChild(acp_key_t aSemKey, acp_sint32_t aCase)
{
    acp_bool_t sLoopCondition = ACP_TRUE;
    acp_bool_t sIsWaited      = ACP_FALSE;
    acp_rc_t   sRC;
    acp_sem_t  sSem;

    sRC = acpSemOpen(&sSem, aSemKey);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpSemOpen should return SUCCESS, but (%d)", sRC));

    switch (aCase)
    {
        case 1:
            sRC = acpSemAcquire(&sSem);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            sRC = acpSemTryAcquire(&sSem);
            ACT_CHECK(ACP_RC_IS_EBUSY(sRC));

            sRC = acpSemRelease(&sSem);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            break;
        case 2:
            while (sLoopCondition)
            {
                sRC = acpSemTryAcquire(&sSem);

                if (ACP_RC_IS_SUCCESS(sRC))
                {
                    if (sIsWaited == ACP_FALSE)
                    {
                        acpSleepSec(1);
                    }
                    else
                    {
                        /* do nothing */
                    }

                    sLoopCondition = ACP_FALSE;
                }
                else if (ACP_RC_IS_EBUSY(sRC))
                {
                    sIsWaited = ACP_TRUE;
                    continue;
                }
                else
                {
                    ACT_ASSERT_DESC(ACP_FALSE,
                                    ("acpSemTryAcquire should return SUCCESS "
                                     "or EBUSY but (%d)",
                                     sRC));
                }
            }

            sRC = acpSemRelease(&sSem);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            break;
        default:
            ACT_ERROR(("This is not a test case (%d)", aCase));
            break;
    }

    sRC = acpSemClose(&sSem);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testSemaphoreWithOneSema(acp_char_t *aProcPath)
{
    acp_char_t        sSemKey[16];
    acp_char_t       *sArgs[4];
    acp_proc_t        sProc;
    acp_proc_wait_t   sWaitResult;
    acp_sem_t         sSem;
    acp_rc_t          sRC;
    acp_sint32_t      i;

    do {
        sRC = acpSemCreate(gTestSemaphoreKey, TEST_SEMAPHORE_COUNT1);
        if(ACP_RC_IS_EEXIST(sRC))
        {
            /* Loop until available key found */
            gTestSemaphoreKey++;
        }
        else
        {
            break;
        }
    } while(ACP_RC_IS_EEXIST(sRC));

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        ACT_ERROR(("acpSemCreate should return SUCCESS, but %d", sRC));
    }
    else
    {
        /* do nothing */
    }

    (void)acpSnprintf(sSemKey,
                      sizeof(sSemKey),
                      "%u",
                      (acp_uint32_t)gTestSemaphoreKey);

    sArgs[0] = sSemKey;
    sArgs[2] = NULL;

    /* open semaphore */
    sRC = acpSemOpen(&sSem, gTestSemaphoreKey);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* simple check to acquire/release */
    for (i = 0; i < TEST_SEMAPHORE_COUNT1; i++)
    {
        sRC = acpSemAcquire(&sSem);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpSemTryAcquire(&sSem);
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));

    for (i = 0; i < TEST_SEMAPHORE_COUNT1; i++)
    {
        sRC = acpSemRelease(&sSem);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sArgs[1] = "1";

    for (i = 0; i < TEST_SEMAPHORE_LOOP; i++)
    {
        sRC = acpProcForkExec(&sProc, aProcPath, sArgs, ACP_FALSE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_SEMAPHORE_LOOP; i++)
    {
        sRC = acpProcWait(NULL, &sWaitResult, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpProcWait(NULL, &sWaitResult, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_ECHILD(sRC));

    sRC = acpSemClose(&sSem);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSemDestroy(gTestSemaphoreKey);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* re-open destroyed semaphore - Must fail! */
    sRC = acpSemOpen(&sSem, gTestSemaphoreKey);
    ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
}

static void testSemaphoreWithSeveralSema(acp_char_t *aProcPath)
{
    acp_char_t        sSemKey[16];
    acp_char_t       *sArgs[3];
    acp_sem_t         sSem;
    acp_proc_t        sProc;
    acp_proc_wait_t   sWaitResult;
    acp_rc_t          sRC;
    acp_sint32_t      i;

    do {
        sRC = acpSemCreate(gTestSemaphoreKey, TEST_SEMAPHORE_COUNT2);
        if(ACP_RC_IS_EEXIST(sRC))
        {
            /* Loop until available key found */
            gTestSemaphoreKey++;
        }
        else
        {
            break;
        }
    } while(ACP_RC_IS_EEXIST(sRC));

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        ACT_ERROR(("acpSemCreate should return SUCCESS, but %d", sRC));
    }
    else
    {
        /* do nothing */
    }

    (void)acpSnprintf(sSemKey,
                      sizeof(sSemKey),
                      "%u",
                      (acp_uint32_t)gTestSemaphoreKey);

    sArgs[0] = sSemKey;
    sArgs[2] = NULL;

    /* open semaphore */
    sRC = acpSemOpen(&sSem, gTestSemaphoreKey);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* simple check to acquire/release */
    for (i = 0; i < TEST_SEMAPHORE_COUNT2; i++)
    {
        sRC = acpSemAcquire(&sSem);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpSemTryAcquire(&sSem);
    ACT_CHECK(ACP_RC_IS_EBUSY(sRC));

    for (i = 0; i < TEST_SEMAPHORE_COUNT2; i++)
    {
        sRC = acpSemRelease(&sSem);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sArgs[1] = "2";

    for (i = 0; i < TEST_SEMAPHORE_LOOP; i++)
    {
        sRC = acpProcForkExec(&sProc, aProcPath, sArgs, ACP_FALSE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_SEMAPHORE_LOOP; i++)
    {
        sRC = acpProcWait(NULL, &sWaitResult, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpProcWait(NULL, &sWaitResult, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_ECHILD(sRC));

    sRC = acpSemClose(&sSem);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSemDestroy(gTestSemaphoreKey);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACT_TEST_BEGIN();

    if (aArgc == 3)
    {
        acp_str_t    sStr1 = ACP_STR_CONST(aArgv[1]);
        acp_str_t    sStr2 = ACP_STR_CONST(aArgv[2]);
        acp_sint32_t sSign;
        acp_uint64_t sNum;
        acp_key_t    sSemKey;
        acp_sint32_t sCase;
        acp_rc_t     sRC;

        sRC = acpStrToInteger(&sStr1, &sSign, &sNum, NULL, 0, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sSemKey = (acp_key_t)((acp_sint32_t)sNum * sSign);

        sRC = acpStrToInteger(&sStr2, &sSign, &sNum, NULL, 0, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sCase = (acp_sint32_t)sNum * sSign;

        testChild(sSemKey, sCase);
    }
    else
    {
        gTestSemaphoreKey = (acp_key_t)acpProcGetSelfID();

        /* binary semaphore */
        testSemaphoreWithOneSema(aArgv[0]);

        /* counting semaphore */
        testSemaphoreWithSeveralSema(aArgv[0]);
    }

    ACT_TEST_END();

    return 0;
}
