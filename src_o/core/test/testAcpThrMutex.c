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
 * $Id: testAcpThrMutex.c 2355 2008-05-20 07:34:00Z shsuh $
 ******************************************************************************/

#include <act.h>
#include <acpThr.h>
#include <acpThrMutex.h>


#define TEST_THREAD_COUNT 10
#define TEST_LOOP         10000


static acp_thr_mutex_t gMutex = ACP_THR_MUTEX_INITIALIZER;
static acp_uint32_t    gValue = 0;


static acp_sint32_t testMutexDefault(void *aArg)
{
    acp_rc_t     sRC;
    acp_uint32_t i;

    ACP_UNUSED(aArg);

    for (i = 0; i < TEST_LOOP; i++)
    {
        sRC = acpThrMutexLock(&gMutex);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        gValue++;

        sRC = acpThrMutexUnlock(&gMutex);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return 0;
}

static acp_sint32_t testMutexErrorCheck(void *aArg)
{
    acp_rc_t     sRC;
    acp_uint32_t i;

    ACP_UNUSED(aArg);

    for (i = 0; i < TEST_LOOP; i++)
    {
        sRC = acpThrMutexLock(&gMutex);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("should return ACP_RC_SUCCESS, but %d", sRC));

        sRC = acpThrMutexLock(&gMutex);
#if !defined(ALTI_CFG_OS_WINDOWS)
        ACT_CHECK_DESC(ACP_RC_IS_EDEADLK(sRC),
                       ("should return ACP_RC_EDEADLK, but %d", sRC));
#else
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("should return ACP_RC_SUCCESS, but %d", sRC));
#endif

        gValue++;

        sRC = acpThrMutexUnlock(&gMutex);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("should return ACP_RC_SUCCESS, but %d", sRC));

        sRC = acpThrMutexUnlock(&gMutex);
#if !defined(ALTI_CFG_OS_WINDOWS)
        ACT_CHECK_DESC(ACP_RC_IS_EPERM(sRC),
                       ("should return ACP_RC_EPERM, but %d", sRC));
#else
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("should return ACP_RC_SUCCESS, but %d", sRC));
#endif

    }

    return 0;
}

static acp_sint32_t testMutexRecursive(void *aArg)
{
    acp_rc_t     sRC;
    acp_uint32_t i;

    ACP_UNUSED(aArg);

    for (i = 0; i < TEST_LOOP; i++)
    {
        sRC = acpThrMutexTryLock(&gMutex);

        if (ACP_RC_IS_EBUSY(sRC))
        {
            sRC = acpThrMutexLock(&gMutex);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
        }

        sRC = acpThrMutexLock(&gMutex);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        gValue++;

        sRC = acpThrMutexUnlock(&gMutex);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpThrMutexUnlock(&gMutex);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("should return SUCCESS, but %d", sRC));

#if !defined (ALTI_CFG_OS_WINDOWS)
        sRC = acpThrMutexUnlock(&gMutex);
        ACT_CHECK_DESC(ACP_RC_IS_EPERM(sRC),
                       ("should return EPERM, but %d", sRC));
#endif
    }

    return 0;
}


acp_sint32_t main(void)
{
    acp_sint32_t    sMutexFlag[3] =
        {
            ACP_THR_MUTEX_DEFAULT,
            ACP_THR_MUTEX_ERRORCHECK,
            ACP_THR_MUTEX_RECURSIVE
        };
    acp_thr_func_t *sThreadFunc[3] =
        {
            testMutexDefault,
            testMutexErrorCheck,
            testMutexRecursive
        };

    acp_thr_t       sThr[TEST_THREAD_COUNT];
    acp_rc_t        sRC;
    acp_sint32_t    sThrRet;
    acp_sint32_t    i;
    acp_sint32_t    j;

    ACT_TEST_BEGIN();

    /*
     * mutex initialized with ACP_THR_MUTEX_INITIALIZER
     */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testMutexDefault, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], &sThrRet);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sThrRet == 0);
    }

    sRC = acpThrMutexDestroy(&gMutex);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK_DESC(gValue == (TEST_THREAD_COUNT * TEST_LOOP),
                   ("the value should be %u but %u",
                    TEST_THREAD_COUNT * TEST_LOOP,
                    gValue));

    /*
     * mutex initialized with acpThrMutexCreate
     */
    for (j = 0; j < 3; j++)
    {
        gValue = 0;

        sRC = acpThrMutexCreate(&gMutex, sMutexFlag[j]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        for (i = 0; i < TEST_THREAD_COUNT; i++)
        {
            sRC = acpThrCreate(&sThr[i], NULL, sThreadFunc[j], NULL);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }

        for (i = 0; i < TEST_THREAD_COUNT; i++)
        {
            sRC = acpThrJoin(&sThr[i], &sThrRet);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sThrRet == 0);
        }

        sRC = acpThrMutexDestroy(&gMutex);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        ACT_CHECK_DESC(gValue == (TEST_THREAD_COUNT * TEST_LOOP),
                       ("the value should be %u but %u",
                        TEST_THREAD_COUNT * TEST_LOOP,
                        gValue));
    }

    ACT_TEST_END();

    return 0;
}
