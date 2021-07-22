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
 * $Id: testAclMemPool.c 6225 2009-07-01 01:46:21Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpMem.h>
#include <aclMemPool.h>


#define TEST_ALLOC_COUNT   100
#define TEST_ELEMENT_COUNT 10
#define TEST_THREAD_COUNT  10


static acl_mem_pool_t  gPool;
static void           *gAddr[TEST_THREAD_COUNT][TEST_ALLOC_COUNT];


static acp_sint32_t testThreadAllocFree(void *aArg)
{
    acp_rc_t     sRC;
    acp_sint32_t sID = (acp_sint32_t)(acp_ulong_t)aArg;
    acp_sint32_t i;

    ACP_UNUSED(aArg);

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        sRC = aclMemPoolCalloc(&gPool, (void **)&gAddr[sID][i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        aclMemPoolFree(&gPool, gAddr[sID][i]);
    }

    return 0;
}

static acp_sint32_t testThreadAlloc(void *aArg)
{
    acp_rc_t     sRC;
    acp_sint32_t sID = (acp_sint32_t)(acp_ulong_t)aArg;
    acp_sint32_t i;

    ACP_UNUSED(aArg);

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        sRC = aclMemPoolCalloc(&gPool, (void **)&gAddr[sID][i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return 0;
}

static acp_sint32_t testThreadFree(void *aArg)
{
    acp_sint32_t sID = (acp_sint32_t)(acp_ulong_t)aArg;
    acp_sint32_t i;

    ACP_UNUSED(aArg);

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        aclMemPoolFree(&gPool, gAddr[sID][i]);
    }

    return 0;
}

static void runThread(void)
{
    acp_thr_t    sThr[TEST_THREAD_COUNT];
    acp_rc_t     sRC;
    acp_sint32_t i;

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i],
                           NULL,
                           testThreadAllocFree,
                           (void *)(acp_ulong_t)i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i],
                           NULL,
                           testThreadAlloc,
                           (void *)(acp_ulong_t)i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThr[i],
                           NULL,
                           testThreadFree,
                           (void *)(acp_ulong_t)i);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThr[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
}

static void testPoolMultiThread(void)
{
    acp_rc_t     sRC;

    sRC = aclMemPoolCreate(&gPool, 10, TEST_ELEMENT_COUNT, 1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    runThread();

    aclMemPoolDestroy(&gPool);

    sRC = aclMemPoolCreate(&gPool, 10, TEST_ELEMENT_COUNT, -1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    runThread();

    aclMemPoolDestroy(&gPool);
}

#if !defined(ACP_CFG_MEMORY_CHECK)
static void testPoolSingleThread(void)
{
    acl_mem_pool_t  sPool;
    void           *sAddr[TEST_ALLOC_COUNT];
    void           *sTempAddr;
    acp_rc_t        sRC;
    acp_sint32_t    i;

    sRC = aclMemPoolCreate(&sPool, 10, TEST_ELEMENT_COUNT, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        sRC = aclMemPoolCalloc(&sPool, (void **)&sAddr[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for (i = 0; i < TEST_ELEMENT_COUNT; i++)
    {
        aclMemPoolFree(&sPool, sAddr[i]);
    }

    for (i = 0; i < TEST_ELEMENT_COUNT; i++)
    {
        sRC = aclMemPoolCalloc(&sPool, (void **)&sTempAddr);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC(sTempAddr == sAddr[TEST_ELEMENT_COUNT - i - 1],
                       ("returned address from free list mismatch at %d", i));
    }

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        aclMemPoolFree(&sPool, sAddr[i]);
    }

    aclMemPoolDestroy(&sPool);
}
#endif

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

#if !defined(ACP_CFG_MEMORY_CHECK)
    testPoolSingleThread();
#endif
    testPoolMultiThread();

    ACT_TEST_END();

    return 0;
}
