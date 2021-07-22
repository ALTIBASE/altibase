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

#define TEST_ALLOC_SMALL_MEM_SIZE 1
#define TEST_ALLOC_BASIC_MEM_SIZE 32
#define TEST_ALLOC_BIG_MEM_SIZE  (10 * 1024 * 1024)

#define TEST_TLSF_INIT_POOL_SIZE (1024 * 1024)

#define TEST_ALLOC_BLOCK_NUM 10
#define TEST_STRING          "hello, world"
#define TEST_STRING_SIZE     14

#define TEST_THREAD_COUNT  10

#define TEST_ALLOC_COUNT   1000

#define TEST_REALLOC_COUNT 1000

/* Structure to test allocations */
typedef struct testMemStruct
{
    struct y
    {
       acp_uint32_t mA;
       acp_char_t   mB[TEST_STRING_SIZE + 1];

    } mY;

    acp_char_t mC;

} testMemStruct;

/*
 * Sanity test for  alloc/calloc/realloc/free functions
 */
static void testAllocBasic(acl_mem_alloc_t *aAlloc)
{
    acp_rc_t    sRC;
    void        *sPtr = NULL;

    acp_uint8_t sZeroBuf[TEST_ALLOC_BASIC_MEM_SIZE] = {0};
    acp_uint8_t sTestBuf[] = {'a','l','l','l','o','c','a','t','o','r'};

    /* Test aclMemAlloc / aclMemFree */
    sRC = aclMemAlloc(aAlloc, &sPtr, TEST_ALLOC_BASIC_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    sRC = aclMemFree(aAlloc, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));


    /* Test aclMemCalloc / aclMemFree */
    sPtr = NULL;

    sRC = aclMemCalloc(aAlloc, &sPtr, 1, TEST_ALLOC_BASIC_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    ACT_CHECK(acpMemCmp(sPtr, sZeroBuf, TEST_ALLOC_BASIC_MEM_SIZE) == 0);

    sRC = aclMemFree(aAlloc, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Test aclMemAlloc / aclMemRealloc /aclMemFree */
    sPtr = NULL;

    sRC = aclMemAlloc(aAlloc, &sPtr, TEST_ALLOC_BASIC_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    acpMemCpy(sPtr, sTestBuf, sizeof (sTestBuf) / sizeof(acp_uint8_t));

    sRC = aclMemRealloc(aAlloc, &sPtr, 2 * TEST_ALLOC_BASIC_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    ACT_CHECK(acpMemCmp(sPtr,
              sTestBuf,
              sizeof (sTestBuf) / sizeof(acp_uint8_t))  == 0);

    sRC = aclMemFree(aAlloc, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

/*
 *  Basic tests for allocator ACL_MEM_ALLOC_LIBC
 */
static void testLibcAllocatorBasic(void)
{
    acp_rc_t sRC;
    acl_mem_alloc_t *sAllocator = NULL;
    acp_bool_t      sThreadSafe = ACP_FALSE;
    void            *sStat = NULL;

    /* Create instance of ACL_MEM_ALLOC_LIBC allocator */
    sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_LIBC, 
                                 NULL, 
                                 &sAllocator);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sAllocator != NULL);

    /* Get statistics */
    sRC = aclMemAllocStatistic(sAllocator, sStat);
    /* ACL_MEM_ALLOC_LIBC does not support this operation */
    ACT_CHECK(ACP_RC_IS_ENOTSUP(sRC));

    /* Get attribute ACL_MEM_ALLOC_THREAD_SAFE_ATTR */
    sRC =  aclMemAllocGetAttr(sAllocator,
                              ACL_MEM_THREAD_SAFE_ATTR,
                              (void *)&sThreadSafe);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sThreadSafe == ACP_TRUE);

    /* Set attribute ACL_MEM_ALLOC_THREAD_SAFE_ATTR */
    sThreadSafe = ACP_FALSE;

    sRC =  aclMemAllocSetAttr(sAllocator,
                              ACL_MEM_THREAD_SAFE_ATTR,
                              (void *)&sThreadSafe);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Get attribute again to check that it was set */
    sRC =  aclMemAllocGetAttr(sAllocator,
                              ACL_MEM_THREAD_SAFE_ATTR,
                              (void *)&sThreadSafe);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sThreadSafe == ACP_FALSE);

    /* Check basic operations with allocator */
    testAllocBasic(sAllocator);

    /* Set it as default */
    aclMemAllocSetDefault(sAllocator);

    /* To check that allocator was set as default get  
     * ACL_MEM_ALLOC_THREAD_SAFE_ATTR it should be
     * false
     */
    sThreadSafe = ACP_TRUE;
    sRC =  aclMemAllocGetAttr(NULL,
                              ACL_MEM_THREAD_SAFE_ATTR,
                              (void *)&sThreadSafe);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sThreadSafe == ACP_FALSE);

    /* Check basic operations with default allocator */
    testAllocBasic(NULL);

    /* Free allocator instance */
    sRC = aclMemAllocFreeInstance(sAllocator); 
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

/*
 * Test invalid cases for allocator API
 */
static void testAllocatorInvalidCases(void)
{
    acp_rc_t sRC;
    acl_mem_alloc_t *sAllocator = NULL;
    acp_bool_t      sAttrValue;

    /* Try to create instance of invalid allocator */
    sRC = aclMemAllocGetInstance(34,
                                 NULL,
                                 &sAllocator);

    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));
    ACT_CHECK(sAllocator == NULL);

    /* Create instance of allocator */
    sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_LIBC,
                                 NULL,
                                 &sAllocator);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sAllocator != NULL);


    /* Try to set invalid attribute */
    sRC =  aclMemAllocSetAttr(sAllocator,
                              34,
                              (void *)&sAttrValue);

    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    /*  Try to get invalid attribute */
    sRC =  aclMemAllocGetAttr(sAllocator,
                              34,
                              (void *)&sAttrValue);

    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    /* Free allocator instance  */
    sRC = aclMemAllocFreeInstance(sAllocator);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));


    /* Try to free NULL allocator */
    sRC = aclMemAllocFreeInstance(NULL);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));
}

/*
 * Basic tests for allocator ACL_MEM_ALLOC_TLSF
 */
void testTlsfAllocatorBasic(void)
{
    acp_rc_t             sRC;
    acl_mem_alloc_t      *sAllocator = NULL;
    acl_mem_tlsf_stat_t  sStat = {0, 0};
    acl_mem_tlsf_init_t  sInit = {0};
    acp_size_t           sPrevSize = 0;
    void  *sPtr = NULL;

    /* Create instance of ACL_MEM_ALLOC_TLSF allocator */
    sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                 NULL,
                                 &sAllocator);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sAllocator != NULL);

    /* Test aclMemAlloc / aclMemFree  small block size */
    sRC = aclMemAlloc(sAllocator, &sPtr, TEST_ALLOC_BASIC_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    sRC = aclMemFree(sAllocator, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Test aclMemAlloc / aclMemFree big block size */
    sRC = aclMemAlloc(sAllocator, &sPtr, TEST_ALLOC_BIG_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    acpMemSet(sPtr, 0x0, TEST_ALLOC_BIG_MEM_SIZE);

    sRC = aclMemFree(sAllocator, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    testAllocBasic(sAllocator);

    /* Test allocating 0 byte block */
    sRC = aclMemAlloc(sAllocator, &sPtr, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    sRC = aclMemFree(sAllocator, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));


    /* Free allocator instance  */
    sRC = aclMemAllocFreeInstance(sAllocator);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sAllocator = NULL;
    sPtr       = NULL;

    /*
     * Test init params and statistics 
     */
    sInit.mPoolSize = TEST_TLSF_INIT_POOL_SIZE; 

    sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                 &sInit,
                                 &sAllocator);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sAllocator != NULL);

    sRC = aclMemAllocStatistic(sAllocator, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(sStat.mPoolSize == TEST_TLSF_INIT_POOL_SIZE);
    ACT_CHECK_DESC((sStat.mUsedSize > 0) &&
                   (sStat.mUsedSize < TEST_TLSF_INIT_POOL_SIZE),
                   ("sStat.mUsedSize is %ld\n", (acp_ulong_t)sStat.mUsedSize));

    sPrevSize = sStat.mUsedSize;

    sRC = aclMemAlloc(sAllocator, &sPtr, TEST_ALLOC_SMALL_MEM_SIZE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    sRC = aclMemAllocStatistic(sAllocator, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(sPrevSize < sStat.mUsedSize);

    sPrevSize = sStat.mUsedSize;

    sRC = aclMemFree(sAllocator, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclMemAllocStatistic(sAllocator, &sStat);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(sStat.mUsedSize <= sPrevSize);

    /* Free allocator instance  */
    sRC = aclMemAllocFreeInstance(sAllocator);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

/*
 * Extended tests for allocator ACL_MEM_ALLOC_TLSF
 */
void  testTlsfAllocatorEx(void)
{
    acp_rc_t             sRC;
    acl_mem_alloc_t      *sAllocator = NULL;

    void  *sPtr  = NULL;
    void  *sPtrTable[TEST_ALLOC_BLOCK_NUM] = {NULL};

    testMemStruct *sStructPtr = NULL;

    acp_uint32_t i = 0;

    /* Create instance of ACL_MEM_ALLOC_TLSF allocator */
    sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                 NULL,
                                 &sAllocator);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sAllocator != NULL);

    /* Allocate mmeory for structure */
    sRC = aclMemAlloc(sAllocator, 
                      (void **)&sStructPtr, 
                      sizeof(testMemStruct));

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sStructPtr != NULL);

    /* Fill the structure */
    sStructPtr->mY.mA = 1234;
 
    sRC = acpCStrCpy(sStructPtr->mY.mB, TEST_STRING_SIZE,
                     TEST_STRING, TEST_STRING_SIZE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sStructPtr->mC = 'A';

    /* Malloc a lot of small chunks */
    for(i = 0; i < TEST_ALLOC_BLOCK_NUM; i++)
    {
        sRC = aclMemAlloc(sAllocator,
                          &sPtrTable[i],
                          TEST_ALLOC_SMALL_MEM_SIZE);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sPtrTable[i] != NULL);

        acpMemSet(sPtrTable[i], 0xFF, TEST_ALLOC_SMALL_MEM_SIZE);
    }

    /* Realloc a lot of small chunks */
    for(i = 0; i < TEST_ALLOC_BLOCK_NUM; i++)
    {
        sRC = aclMemRealloc(sAllocator,
                            &sPtrTable[i],
                            2 * TEST_ALLOC_SMALL_MEM_SIZE);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sPtrTable[i] != NULL);
    }

    /* Alloc the big chunk */ 
    sRC = aclMemAlloc(sAllocator,
                      &sPtr,
                      TEST_ALLOC_BIG_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    acpMemSet(sPtr, 0x0, TEST_ALLOC_BIG_MEM_SIZE);

    /* Check that memory blocks has the right values */
    for(i = 0; i < TEST_ALLOC_BLOCK_NUM; i++)
    {
        ACT_CHECK(((acp_uint8_t *)sPtrTable[i])[0] == 0xFF);
    }

    /* Free big block */
    sRC = aclMemFree(sAllocator, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Free memory blocks */
    for(i = 0; i < TEST_ALLOC_BLOCK_NUM; i++)
    {
        sRC = aclMemFree(sAllocator, sPtrTable[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /* Check that structure has right values */
    ACT_CHECK(sStructPtr->mY.mA == 1234);

    ACT_CHECK(acpCStrCmp(sStructPtr->mY.mB,
                         TEST_STRING, 
                         TEST_STRING_SIZE) == 0);

    ACT_CHECK(sStructPtr->mC == 'A');

    /* Free structure */
    sRC = aclMemFree(sAllocator, sStructPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Free allocator instance  */
    sRC = aclMemAllocFreeInstance(sAllocator);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

/*
 * Thread function for alloc/free operations
 */
static acp_sint32_t testThreadAllocFree(void *aArg)
{
    void      *sPtr  = NULL;
    acp_rc_t  sRC;

    acl_mem_alloc_t *sAllocator  = (acl_mem_alloc_t *)aArg;
    acp_sint32_t i;

    acp_sint32_t sRandValue = 0;
    acp_uint32_t sSeed = 0;

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        sSeed = acpRandSeedAuto();
        sRandValue = acpRand(&sSeed);

        sRC = aclMemAlloc(sAllocator,
                          &sPtr,
                          sRandValue % (128 * 1024));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sPtr != NULL);

        sRC = aclMemFree(sAllocator, sPtr);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sPtr = NULL;
    }

    return 0;
}

/*
 * Thread function for realloc operations
 */
static acp_sint32_t testThreadRealloc(void *aArg)
{
    void      *sPtr  = NULL;
    acp_rc_t  sRC;

    acl_mem_alloc_t *sAllocator  = (acl_mem_alloc_t *)aArg;
    acp_sint32_t i;

    acp_sint32_t sRandValue = 0;
    acp_uint32_t sSeed = 0;

    sRC = aclMemAlloc(sAllocator,
                      &sPtr,
                     TEST_ALLOC_BIG_MEM_SIZE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sPtr != NULL);

    for (i = 0; i < TEST_REALLOC_COUNT; i++)
    {
        sSeed = acpRandSeedAuto();
        sRandValue = acpRand(&sSeed);

        sRC = aclMemRealloc(sAllocator,
                            &sPtr,
                            sRandValue % (1024 * 1024));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        /* Do not check for sPtr != NULL since
         * if size is 0 then realloc works as free
         * and returns NULL
         */
    }

    sRC = aclMemFree(sAllocator, sPtr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

/*
 * Multithreaded tests for allocator ACL_MEM_ALLOC_TLSF
 */
void testTlsfAllocatorMT(void)
{

    acp_thr_t        sThrAllocFree[TEST_THREAD_COUNT];
    acp_thr_t        sThrRealloc[TEST_THREAD_COUNT];

    acl_mem_alloc_t  *sAllocator = NULL;
    acp_bool_t       sThreadSafe = ACP_TRUE;

    acp_rc_t     sRC;
    acp_sint32_t i;

    /* Create instance of ACL_MEM_ALLOC_TLSF allocator */
    sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                 NULL,
                                 &sAllocator);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(sAllocator != NULL);

    /* Set it to be thread safe */
    sRC =  aclMemAllocSetAttr(sAllocator,
                              ACL_MEM_THREAD_SAFE_ATTR,
                              (void *)&sThreadSafe);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * Run threads doing alloc/free with shared allocator
     * instance
     */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThrAllocFree[i],
                           NULL,
                           testThreadAllocFree,
                           (void *)(sAllocator));

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * Run threads doing realloc with shared allocator
     * instance
     */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThrRealloc[i],
                           NULL,
                           testThreadRealloc,
                           (void *)(sAllocator));

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /* Wait for threads doing alloc/free */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThrAllocFree[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /* Wait for threads doing realloc */
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThrRealloc[i], NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /* Free allocator instance  */
    sRC = aclMemAllocFreeInstance(sAllocator);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

/*
 * Unit test for ACL memory allocators
 */
acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    /* Test alloc functions with default allocator */
    testAllocBasic(NULL);

    /* Mem allocator ACL_MEM_ALLOC_LIBC basic test */
    testLibcAllocatorBasic();

    /* Mem allocator ACL_MEM_ALLOC_TLSF basic test */
    testTlsfAllocatorBasic();

    /* Mem allocator ACL_MEM_ALLOC_TLSF extended test */
    testTlsfAllocatorEx();

    /* Mem allocator ACL_MEM_ALLOC_TLSF multithreaded test */
    testTlsfAllocatorMT();

    /* Test invalid cases */
    testAllocatorInvalidCases();

    ACT_TEST_END();

    return 0;
}
