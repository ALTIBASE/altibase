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
 * $Id: testAcpThr.c 8996 2009-12-03 06:30:46Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpSleep.h>
#include <acpThr.h>


#define TEST_THREAD_COUNT 10

static acp_sint32_t   gTestData[TEST_THREAD_COUNT];

static acp_thr_once_t gTestOnceControl = ACP_THR_ONCE_INITIALIZER;
static acp_sint32_t   gTestOnceValue   = 0;


static void testOnceFunc(void)
{
    acpSleepMsec(100);
    gTestOnceValue++;
}

static acp_sint32_t testThread(void *aArg)
{
    acp_uint32_t *sDataPtr = (acp_uint32_t *)aArg;
    acp_uint32_t  sDataVal = *sDataPtr;

    acpThrYield();

    *sDataPtr *= 10;

    if ((sDataVal % 3) == 0)
    {
        acpThrExit((acp_rc_t)sDataVal);
    }
    else
    {
        return sDataVal;
    }

    return 0;
}

static acp_sint32_t testTempThread(void *aArg)
{
    aArg = NULL;

    /* time consuming */
    acpSleepMsec(1);

    return 0;
}

static acp_sint32_t testThreadOnce(void *aArg)
{
    ACP_UNUSED(aArg);

    acpThrOnce(&gTestOnceControl, testOnceFunc);

    ACT_CHECK_DESC(gTestOnceValue == 1,
                   ("gTestOnceValue should be 1, but %d", gTestOnceValue));

    return 0;
}

static void testThreadStruct(void)
{
    acp_rc_t sRC;
    acp_thr_t sTempThr[100];
    acp_sint32_t i;

    /* These threads will run after testThreadStruct function exited,
     * so acp_thr_t can be declared in stack
     */
    for (i = 0; i < 100; i++)
    {
        sRC = acpThrCreate(&sTempThr[i], NULL, testTempThread, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        sRC = acpThrDetach(&sTempThr[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
}

static acp_sint32_t testThrCheckID(void* aParam)
{
    acp_thr_t*      sThr = (acp_thr_t*)aParam;
    acp_uint64_t    sCurrentID;
    acp_uint64_t    sThrID;
    acp_rc_t        sRC;

    /*
     * ID got with acpThrGetSelfID and get with acpThrGetID(sThr)
     * should be the same.
     * Main thread passes the thread ID as aParam
     */
    /* Sleep to wait for initializing aParam */
    acpThrYield();
    sCurrentID = acpThrGetSelfID();
    sRC = acpThrGetID(sThr, &sThrID);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK_DESC(sCurrentID == sThrID,
                   ("acpThrGetSelfID and acpThrGetID should return same value, "
                    "but %lld != %lld", sCurrentID, sThrID));

    return 0;
}

acp_sint32_t main(void)
{
    acp_rc_t       sRC;
    acp_sint32_t   sThrRet;
    acp_thr_t      sThr[TEST_THREAD_COUNT];
    acp_thr_t      sThrCheckID;
    acp_thr_attr_t sAttr;
    acp_sint32_t   i;

    ACT_TEST_BEGIN();

    
    /* BUG-27358 test acp_thr_t structure */
    testThreadStruct();

    /*
     * create thread attribute
     */
    sRC = acpThrAttrCreate(&sAttr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * test error code for invalid attribute value
     */
    sRC = acpThrAttrSetStackSize(&sAttr, 1);
#if defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
#else
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));
#endif

    /*
     * set other attribute value
     */
    sRC = acpThrAttrSetBound(&sAttr, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrAttrSetBound(&sAttr, ACP_FALSE);
#if defined(ALTI_CFG_OS_HPUX) &&                                         \
    ((ALTI_CFG_OS_MAJOR < 10) ||                                     \
     (ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR <= 11))
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));
#elif defined(ALTI_CFG_OS_LINUX)
    /*
     * acpThrGetID() does use gettid syscall.
     * This assumes all threads in linux will be assigned as kernel process.
     *
     * If the linux supports m:n thread model (process-scope or unbound thread),
     * acpThrGetID() implementation should be verified.
     */
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));
#elif defined(ALTI_CFG_OS_WINDOWS)
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));
#endif

    sRC = acpThrAttrSetDetach(&sAttr, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpThrAttrSetDetach(&sAttr, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * thread create w or w/o attribute
     */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        gTestData[i] = i + 1;

        if (i == 7)
        {
            sRC = acpThrAttrSetDetach(&sAttr, ACP_TRUE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
            /* do nothing */
        }

        sRC = acpThrCreate(&sThr[i],
                           (i % 2) ? &sAttr : NULL,
                           testThread,
                           &gTestData[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        if (i == 7)
        {
            sRC = acpThrAttrSetDetach(&sAttr, ACP_FALSE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            sRC = acpThrJoin(&sThr[i], &sThrRet);
            ACT_CHECK(ACP_RC_IS_EINVAL(sRC) || ACP_RC_IS_ESRCH(sRC));
        }
        else
        {
            /* do nothing */
        }
    }

    /*
     * destroy thread attribute
     */
    sRC = acpThrAttrDestroy(&sAttr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * thread yield
     */
    acpThrYield();

    /*
     * thread detach/join
     */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        if (i == 7)
        {
            /* do nothing */
        }
        else if ((i % 5) == 0)
        {
            sRC = acpThrDetach(&sThr[i]);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            sRC = acpThrJoin(&sThr[i], &sThrRet);
            ACT_CHECK(ACP_RC_IS_EINVAL(sRC) || ACP_RC_IS_ESRCH(sRC));
        }
        else
        {
            sRC = acpThrJoin(&sThr[i], &sThrRet);
            ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                           ("acpThrJoin returned %d for thread(%d)", sRC, i));
            ACT_CHECK_DESC(sThrRet == (i + 1),
                           ("thread(%d) return value should be %d but %d",
                            i,
                            i + 1,
                            sThrRet));
            ACT_CHECK_DESC(gTestData[i] == ((i + 1) * 10),
                           ("thread(%d) data should be %d but %d",
                            i,
                            (i + 1) * 10,
                            gTestData[i]));
        }
    }

    /*
     * thread once
     */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i], NULL, testThreadOnce, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * Get Thread ID functions
     */
    sRC = acpThrCreate(&sThrCheckID, NULL, testThrCheckID, &sThrCheckID);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpThrJoin(&sThrCheckID, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    
    ACT_TEST_END();

    return 0;
}
