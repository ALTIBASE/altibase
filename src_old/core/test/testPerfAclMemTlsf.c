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
 * $Id: testPerfAclMemTlsf.c 439 2012-07-30 06:38:25Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <aclMem.h>
#include <aclMemTlsf.h>
#include <acpRand.h>
#include <acpThr.h>

/* ------------------------------------------------
 * Main Body
 * ----------------------------------------------*/

#define TEST_TLSF_INSTANCE_COUNT 8
#define TEST_TLSF_INIT_POOL_SIZE (1024 * 1024 * 16)

#define TEST_ALLOC_SIZE_COUNT 10000
#define TEST_ALLOC_MAX_SIZE 1024
#define TEST_ALLOC_MIN_SIZE 5*1024

acp_sint32_t gAllocSize[TEST_ALLOC_SIZE_COUNT];

acl_mem_alloc_t *gAllocator[TEST_TLSF_INSTANCE_COUNT];
acl_mem_tlsf_init_t gInit[TEST_TLSF_INSTANCE_COUNT];

acp_thr_t gThrHandle[TEST_TLSF_INSTANCE_COUNT];
acp_sint32_t gThrNum[TEST_TLSF_INSTANCE_COUNT];


acp_rc_t testAclMemTlsfPerfInit(actPerfFrmThrObj *aThrObj)
{
    acp_sint32_t i;
    acp_rc_t             sRC;
    acp_bool_t sThreadSafe = ACP_TRUE;
    acp_uint32_t sSeed;
    
    /*
     * Test init params and statistics 
     */

    for (i = 0; i < TEST_TLSF_INSTANCE_COUNT; i++)
    {
        gInit[i].mPoolSize = TEST_TLSF_INIT_POOL_SIZE;
        
        sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                     &gInit[i],
                                     &gAllocator[i]);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC) || gAllocator[i] == NULL,
                       INSTANCE_FAIL);
        
        /* must activate lock */
        sRC = aclMemAllocSetAttr(gAllocator[i],
                                 ACL_MEM_THREAD_SAFE_ATTR,
                                 &sThreadSafe);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ATTR_FAIL);
    }

    /* create sizes */
    sSeed = acpRandSeedAuto();
    
    for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
    {
        gAllocSize[i] = (acpRand(&sSeed) % TEST_ALLOC_MAX_SIZE) + TEST_ALLOC_MIN_SIZE;
    }


    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(INSTANCE_FAIL)
    {
        ACT_ERROR(("Cannot get allocator instance!\n"));
    }

    ACP_EXCEPTION(ATTR_FAIL)
    {
        ACT_ERROR(("Cannot set attribute!\n"));
    }
    
    ACP_EXCEPTION_END;
    return sRC;
}

/*
 * test performance of TLSF in case of big memory 5K~6K
 */
acp_rc_t testAclMemTlsfPerfTest(actPerfFrmThrObj *aThrObj)
{
    acp_sint32_t sInstanceNum =
        aThrObj->mNumber % TEST_TLSF_INSTANCE_COUNT;
    acp_rc_t             sRC;
    char  *sPtr[TEST_ALLOC_SIZE_COUNT];
    acp_sint32_t i;
    acp_sint32_t j;
    
    for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
    {
        sRC = aclMemAlloc(gAllocator[sInstanceNum],
                          (void **)&sPtr[i],
                          (acp_size_t)gAllocSize[i]);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sPtr[i] != NULL);

        for (j = 0; j < gAllocSize[i]; j++)
        {
            sPtr[i][j] = (char)j;
        }
    }

    for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
    {
        sRC = aclMemFree(gAllocator[sInstanceNum], sPtr[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return ACP_RC_SUCCESS;
}
acp_rc_t testAclMemTlsfPerfFinal(actPerfFrmThrObj *aThrObj)
{
    acp_sint32_t i;
    acp_rc_t             sRC;
    
    for (i = 0; i < TEST_TLSF_INSTANCE_COUNT; i++)
    {
        sRC = aclMemAllocFreeInstance(gAllocator[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
    
    return ACP_RC_SUCCESS;
}

acp_rc_t testAcpMemPerfTestInit(actPerfFrmThrObj *aThrObj)
{
    return ACP_RC_SUCCESS;
}
acp_rc_t testAcpMemPerfTestFinal(actPerfFrmThrObj *aThrObj)
{
    return ACP_RC_SUCCESS;
}

/*
 * test performance of LIBC in case of big memory 5K~6K
 */
acp_rc_t testAcpMemPerfTest(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t             sRC;
    char  *sPtr[TEST_ALLOC_SIZE_COUNT];
    acp_sint32_t i;
    acp_sint32_t j;

    for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
    {
        sRC = aclMemAlloc(NULL,
                          (void **)&sPtr[i],
                          (acp_size_t)gAllocSize[i]);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sPtr[i] != NULL);

        for (j = 0; j < gAllocSize[i]; j++)
        {
            sPtr[i][j] = (char)j;
        }
    }

    for (i = 0; i < TEST_ALLOC_SIZE_COUNT; i++)
    {
        sRC = aclMemFree(NULL, sPtr[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return ACP_RC_SUCCESS;
}



/* ------------------------------------------------
 *  Data Descriptor 
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "aclMemPerf";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    {
        "TLSF_16-THREADs",
        16,
        TEST_ALLOC_SIZE_COUNT,
        testAclMemTlsfPerfInit,
        testAclMemTlsfPerfTest,
        testAclMemTlsfPerfFinal
    } ,
    {
        "LIBC_16-THREADs",
        16,
        TEST_ALLOC_SIZE_COUNT,
        testAcpMemPerfTestInit,
        testAcpMemPerfTest,
        testAcpMemPerfTestFinal
    } ,
    {
        "TLSF_32-THREADs",
        32,
        TEST_ALLOC_SIZE_COUNT,
        testAclMemTlsfPerfInit,
        testAclMemTlsfPerfTest,
        testAclMemTlsfPerfFinal
    } ,
    {
        "LIBC_32-THREADs",
        32,
        TEST_ALLOC_SIZE_COUNT,
        testAcpMemPerfTestInit,
        testAcpMemPerfTest,
        testAcpMemPerfTestFinal
    } ,
    {
        "TLSF_64-THREADs",
        64,
        TEST_ALLOC_SIZE_COUNT,
        testAclMemTlsfPerfInit,
        testAclMemTlsfPerfTest,
        testAclMemTlsfPerfFinal
    } ,
    {
        "LIBC_64-THREADs",
        64,
        TEST_ALLOC_SIZE_COUNT,
        testAcpMemPerfTestInit,
        testAcpMemPerfTest,
        testAcpMemPerfTestFinal
    } ,
    ACT_PERF_FRM_SENTINEL 
};

