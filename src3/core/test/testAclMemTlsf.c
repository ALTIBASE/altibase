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
 * $Id: testAclMem.c 6225 2009-07-01 01:46:21Z jykim $
 ******************************************************************************/

#include <act.h>
#include <aclMem.h>
#include <aclMemTlsf.h>
#include <acpRand.h>
#include <acpThr.h>

#define TEST_SPIN_COUNT (10000)

/* pool size must be bigger than default size */
#define TEST_TLSF_INIT_POOL_SIZE (ACL_TLSF_DEFAULT_AREA_SIZE * 10)
#define TEST_THREAD_COUNT  16
#define TEST_TLSF_INSTANCE_COUNT 8

#define TEST_ALLOC_COUNT   100
#define TEST_ALLOC_SIZE_COUNT 512
#define TEST_ALLOC_MAX_SIZE 65536
#define TEST_ALLOC_MIN_SIZE 64

acp_sint32_t gAllocSize[TEST_ALLOC_SIZE_COUNT];

acl_mem_alloc_t *gAllocator[TEST_TLSF_INSTANCE_COUNT];
acl_mem_tlsf_init_t gInit[TEST_TLSF_INSTANCE_COUNT];

acl_mem_alloc_t *gAllocatorOnce;
acl_mem_tlsf_init_t gInitOnce = {0};

acp_thr_t gThrHandle[TEST_THREAD_COUNT];
acp_slong_t gThrNum[TEST_THREAD_COUNT];

/* barrier to start threads at once */
static act_barrier_t gBarrier;


/*
 * Basic tests for allocator ACL_MEM_ALLOC_TLSF
 */
void testTlsfOnce(void)
{
    acp_rc_t sRC;
    void  *sPtr[TEST_ALLOC_SIZE_COUNT];
    acp_sint32_t i;
    acp_sint32_t j;
    acp_uint32_t *sPtrTest; 
    acp_bool_t sResult;
    
    for (j = 0; j < TEST_ALLOC_COUNT; j++)
    {
        for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
        {
            sRC = aclMemAlloc(gAllocatorOnce,
                              &sPtr[i],
                              (acp_size_t)gAllocSize[i]);

            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sPtr[i] != NULL);

            /* test validness of pointer */
            sPtrTest = (acp_uint32_t *)sPtr[i];
            *sPtrTest = 0x12345678;
            ACT_CHECK(*sPtrTest == 0x12345678);
        }

        for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
        {
            sRC = aclMemRealloc(gAllocatorOnce,
                                &sPtr[i],
                                (acp_size_t)gAllocSize[i]);

            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sPtr[i] != NULL);

            /* test validness of pointer */
            sPtrTest = (acp_uint32_t *)sPtr[i];
            ACT_CHECK(*sPtrTest == 0x12345678);
        }

        for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
        {
            sRC = aclMemFree(gAllocatorOnce, sPtr[i]);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
    }

    /* Big size test */
    sRC = aclMemAlloc(gAllocatorOnce,
                      &sPtr[0],
                      (acp_size_t)TEST_TLSF_INIT_POOL_SIZE * 2);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr[0] != NULL);

    /* test validness of pointer */
    sPtrTest = (acp_uint32_t *)sPtr[0];
    *sPtrTest = 0x12345678;
    ACT_CHECK(*sPtrTest == 0x12345678);

    sRC = aclMemFree(gAllocatorOnce, sPtr[0]);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* empty test after alloc */
    sRC = aclMemAllocGetAttr(gAllocatorOnce, ACL_MEM_IS_EMPTY, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sResult == ACP_TRUE);
    
    sRC = aclMemAlloc(gAllocatorOnce,
                      &sPtr[0],
                      (acp_size_t)512);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr[0] != NULL);

    sRC = aclMemRealloc(gAllocatorOnce,
                      &sPtr[0],
                      (acp_size_t)1024);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr[0] != NULL);

    sRC = aclMemFree(gAllocatorOnce, sPtr[0]);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* empty test after realloc */
    sRC = aclMemAllocGetAttr(gAllocatorOnce, ACL_MEM_IS_EMPTY, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sResult == ACP_TRUE);

    sRC = aclMemCalloc(gAllocatorOnce,
                      &sPtr[0],
                       1,
                      (acp_size_t)TEST_TLSF_INIT_POOL_SIZE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr[0] != NULL);

    sRC = aclMemFree(gAllocatorOnce, sPtr[0]);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* empty test after calloc */
    sRC = aclMemAllocGetAttr(gAllocatorOnce, ACL_MEM_IS_EMPTY, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sResult == ACP_TRUE);

}

void testTlsfHuge(void)
{
    acp_rc_t sRC;
    acp_size_t sBigSize;
    acp_uint32_t *sBigPtr;
    acp_uint32_t i;
    
    /* test 1G allocation */
    sBigSize = ACL_TLSF_HUGE_MEMORY_SIZE;

    /* Big size test */
    sRC = aclMemAlloc(gAllocatorOnce,
                      (void **)&sBigPtr,
                      sBigSize);
    
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sBigPtr != NULL);
    
    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return;
    }
    else
    {
        /* go ahead */
    }

    /* test validness of pointer */
    for (i = 0; i < sBigSize/4;i++)
    {
        sBigPtr[i] = 0x12345678;
        ACT_CHECK(sBigPtr[i] == 0x12345678);
    }

    sRC = aclMemRealloc(gAllocatorOnce,
                        (void **)&sBigPtr,
                        sBigSize / 2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    
    sRC = aclMemRealloc(gAllocatorOnce,
                        (void **)&sBigPtr,
                        sBigSize * 2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    
    sRC = aclMemFree(gAllocatorOnce, sBigPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclMemCalloc(gAllocatorOnce,
                       (void **)&sBigPtr,
                       sBigSize,
                       1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sBigPtr != NULL);

    sRC = aclMemFree(gAllocatorOnce, sBigPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

acp_sint32_t testTlsfThread(void *aThrNum)
{
    acp_ulong_t sInstanceNum =
        (acp_slong_t)aThrNum % TEST_TLSF_INSTANCE_COUNT;
    acp_rc_t             sRC;
    void  *sPtr[TEST_ALLOC_SIZE_COUNT];
    acp_sint32_t i;
    acp_sint32_t j;
    acp_uint32_t *sPtrTest;
    acp_sint32_t sCount;

    ACT_BARRIER_TOUCH_AND_WAIT(&gBarrier, TEST_THREAD_COUNT + 1);

    for (j = 0; j < TEST_ALLOC_COUNT; j++)
    {
        for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
        {
            sRC = aclMemAlloc(gAllocator[sInstanceNum],
                              &sPtr[i],
                              (acp_size_t)gAllocSize[i]);

            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sPtr[i] != NULL);

            /* test validness of pointer */
            sPtrTest = (acp_uint32_t *)sPtr[i];
            *sPtrTest = 0x12345678;
            ACT_CHECK(*sPtrTest == 0x12345678);
        }

        for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
        {
            sRC = aclMemRealloc(gAllocator[sInstanceNum],
                              &sPtr[i],
                              (acp_size_t)gAllocSize[i] * 2);

            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sPtr[i] != NULL);

            /* test validness of pointer */
            sPtrTest = (acp_uint32_t *)sPtr[i];
            ACT_CHECK(*sPtrTest == 0x12345678);
        }

        for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
        {
            sRC = aclMemFree(gAllocator[sInstanceNum], sPtr[i]);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }

        /* SHRINK can work for multi-thread? */
        sRC = aclMemAllocSetAttr(gAllocator[sInstanceNum],
                                 ACL_MEM_SHRINK,
                                 &sCount);
        ACT_CHECK((ACP_RC_IS_SUCCESS(sRC) && sCount > 0)
                  || ACP_RC_IS_ENOENT(sRC));
    }

    return 0;
}

acp_sint32_t testInitTlsfInstances(void)
{
    acp_sint32_t    i;
    acp_rc_t        sRC;
    acp_bool_t      sThreadSafe = ACP_TRUE;
    acp_bool_t      sResult;
    acp_sint32_t    sSpinCountSet;
    acp_sint32_t    sSpinCountGot;
    acl_mem_tlsf_stat_t sStat;
    acp_size_t sUsedOld;
    void *sPtr[3];
    acp_sint32_t sCount;
    
    /*
     * Test init params and statistics 
     */

    gInitOnce.mPoolSize = TEST_TLSF_INIT_POOL_SIZE;
    
    sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                 &gInitOnce,
                                 &gAllocatorOnce);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(gAllocatorOnce != NULL);

    if (gAllocatorOnce == NULL)
    {
        return -1;
    }
    else
    {
        /* nothing */
    }

    /* check attributes, IS_EMPTY and IS_FULL */
    sRC = aclMemAllocGetAttr(gAllocatorOnce, ACL_MEM_IS_EMPTY, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sResult == ACP_TRUE);

    sRC = aclMemAllocGetAttr(gAllocatorOnce, ACL_MEM_IS_FULL, &sResult);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sResult == ACP_FALSE);

    /* check pool size setting */
    sRC = aclMemAllocStatistic(gAllocatorOnce, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sStat.mPoolSize == TEST_TLSF_INIT_POOL_SIZE);
    sUsedOld = sStat.mUsedSize;

    /* check pool size increasing */
    sRC = aclMemAlloc(gAllocatorOnce,
                      &sPtr[0],
                      (acp_size_t)TEST_TLSF_INIT_POOL_SIZE/2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);
    
    sRC = aclMemAllocStatistic(gAllocatorOnce, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* check used size increaseing */
    /* increased size is bigger than requested memory size,
       becauseof overhead of memory management */
    ACT_CHECK((sStat.mUsedSize >
              sUsedOld + (acp_size_t)TEST_TLSF_INIT_POOL_SIZE/2) &&
              (sStat.mUsedSize < (acp_size_t)TEST_TLSF_INIT_POOL_SIZE));

    sRC = aclMemAlloc(gAllocatorOnce,
                      &sPtr[1],
                      (acp_size_t)TEST_TLSF_INIT_POOL_SIZE/2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    sRC = aclMemAlloc(gAllocatorOnce,
                      &sPtr[2],
                      (acp_size_t)TEST_TLSF_INIT_POOL_SIZE/2);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    /* Allocating the 1/2 size of INIT_POOL_SIZE will make allocator
       create one more area */
    sRC = aclMemAllocStatistic(gAllocatorOnce, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sStat.mPoolSize == TEST_TLSF_INIT_POOL_SIZE * 3);
    
    (void)aclMemFree(gAllocatorOnce, sPtr[0]);
    (void)aclMemFree(gAllocatorOnce, sPtr[1]);
    (void)aclMemFree(gAllocatorOnce, sPtr[2]);

    /* One area among three areas can be shrinked */
    sRC = aclMemAllocSetAttr(gAllocatorOnce, ACL_MEM_SHRINK, &sCount);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sCount == 1);
    
    sRC = aclMemAllocStatistic(gAllocatorOnce, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sStat.mPoolSize == TEST_TLSF_INIT_POOL_SIZE * 2);

    /* test spinlock setting */
    sSpinCountSet = TEST_SPIN_COUNT;
    sRC = aclMemAllocSetAttr(gAllocatorOnce, ACL_MEM_SET_SPIN_COUNT,
                             &sSpinCountSet);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = aclMemAllocGetAttr(gAllocatorOnce, ACL_MEM_GET_SPIN_COUNT,
                             &sSpinCountGot);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sSpinCountGot == TEST_SPIN_COUNT);

    for (i = 0; i < TEST_TLSF_INSTANCE_COUNT; i++)
    {
        gInit[i].mPoolSize = TEST_TLSF_INIT_POOL_SIZE;
        
        sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                     &gInit[i],
                                     &gAllocator[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(gAllocator[i] != NULL);

        if (gAllocator[i] == NULL || ACP_RC_NOT_SUCCESS(sRC))
        {
            return -1;
        }
        else
        {
            /* nothing */
        }
        
        /* must activate lock */
        sRC = aclMemAllocSetAttr(gAllocator[i],
                                 ACL_MEM_THREAD_SAFE_ATTR,
                                 &sThreadSafe);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return -1;
        }
        else
        {
            /* nothing */
        }
    }

    return 0;
}

void testFinalTlsfInstances(void)
{
    acp_sint32_t i;
    acp_rc_t             sRC;
    
    sRC = aclMemAllocFreeInstance(gAllocatorOnce);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    
    for (i = 0; i < TEST_TLSF_INSTANCE_COUNT; i++)
    {
        sRC = aclMemAllocFreeInstance(gAllocator[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
}

acp_sint32_t main(void)
{
    acp_uint32_t sSeed;
    acp_slong_t i;

    ACT_TEST_BEGIN();

    /* create sizes */
    sSeed = acpRandSeedAuto();
    
    for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
    {
        gAllocSize[i] =
            (acpRand(&sSeed) % TEST_ALLOC_MAX_SIZE) + TEST_ALLOC_MIN_SIZE;
    }

    i = testInitTlsfInstances();

    if (i < 0)
    {
        return -1;
    }
    else
    {
        /* do nothing */
    }

    /*
     * test for one thread allocation
     */

    /* If you check the execution time of testTlsfOnce,
       it will be performance test of tlsf for one thread */
    testTlsfOnce();
    testTlsfHuge();

    /*
     * test for multi-thread allocation
     */

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        gThrNum[i] = i;
    }

    ACT_BARRIER_CREATE(&gBarrier);
    ACT_BARRIER_INIT(&gBarrier);

    /* If you check the execution time of all thread,
       it will be performance test of tlsf for multi-thread */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        (void)acpThrCreate(&gThrHandle[i], NULL,
                           testTlsfThread, (void *)gThrNum[i]);
    }

    /* wait all threads created and then start all threads at once */
    ACT_BARRIER_WAIT(&gBarrier, TEST_THREAD_COUNT);
    ACT_BARRIER_TOUCH(&gBarrier);

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        (void)acpThrJoin(&gThrHandle[i], NULL);
    }

    ACT_BARRIER_DESTROY(&gBarrier);

    /*
     * end of test
     */
    testFinalTlsfInstances();
    
    ACT_TEST_END();

    return 0;
}


