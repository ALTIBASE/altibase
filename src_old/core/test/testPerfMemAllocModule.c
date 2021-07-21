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
 * $Id:
 ******************************************************************************/

#include <act.h>
#include <acpMem.h>
#include <aclMem.h>
#include <acpRand.h>

/* Memory size classes */
#define TEST_BLOCK_SIZE_TINY   (1  * 64)
#define TEST_BLOCK_SIZE_SMALL  (1  * 512)
#define TEST_BLOCK_SIZE_MEDIUM (4  * 1024)
#define TEST_BLOCK_SIZE_BIG    (64 * 1024)

#define TEST_SIZE_ARRAY_LEN    (64)

#define TEST_LOOP_COUNT ((acp_uint64_t)100000)

static acp_bool_t gInitFlag = ACP_TRUE;
static acp_size_t gSizeArray[TEST_SIZE_ARRAY_LEN] = {0};

/* Allocator test structure */
typedef struct testAllocStruct
{
    /* Pointer to allocator instance */
    acl_mem_alloc_t      *mAllocator;
    acl_mem_alloc_type_t  mType;
    acp_bool_t            mShared;

} testAllocStruct;

/*
 * Init function for test case
 */
ACP_INLINE acp_rc_t testAllocInit(actPerfFrmThrObj     *aThrObj,
                                  acl_mem_alloc_type_t aAllocatorType,
                                  acp_bool_t           aShared,
                                  acp_size_t           aBlockSize)
{
    testAllocStruct *sAllocStruct;
    acp_rc_t         sRC;
    acp_uint32_t     i          = 0;
    acp_sint32_t     sRandValue = 0;
    acp_uint32_t     sSeed      = 0;

    sRC = acpMemAlloc((void*)(&sAllocStruct), sizeof(testAllocStruct));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FREE_NOTHING);

    /*
     * Test scenario will use shared mmeory manager instance between threads
     */
    if (aShared == ACP_TRUE)
    {
        /* Create instance */
        sRC = aclMemAllocGetInstance(aAllocatorType,
                                     NULL,
                                     &sAllocStruct->mAllocator);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ALLOCATOR_ERROR);

         /* Set thread safe attr */
        sRC =  aclMemAllocSetAttr(sAllocStruct->mAllocator,
                                  ACL_MEM_THREAD_SAFE_ATTR,
                                  (void *)&aShared);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
    else
    {
        sAllocStruct->mAllocator = NULL;
    }

    /* Generate test block sizes */
    if (gInitFlag == ACP_TRUE)
    {
        for (i = 0; i < TEST_SIZE_ARRAY_LEN; i++)
        {
            sSeed = acpRandSeedAuto();
            sRandValue = acpRand(&sSeed);

            gSizeArray[i] = sRandValue % aBlockSize;

            if (gSizeArray[i] == 0)
            {
                gSizeArray[i] = aBlockSize;
            }
            else
            {
                /* Do nothing */
            }
        }

        gInitFlag = ACP_FALSE;
    }
    else
    {
        /* Initialize only once */
    }

    sAllocStruct->mType  = aAllocatorType;
    sAllocStruct->mShared = aShared;

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = (void *)sAllocStruct;
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(FREE_NOTHING)
    {
        /* Do nothing */
    }

    ACP_EXCEPTION(ALLOCATOR_ERROR)
    {
        acpMemFree(sAllocStruct);
    }

    ACP_EXCEPTION_END;

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = NULL;
    }

    return sRC;
}

/*
 * LIBC allocator thread private instance test init functions
 */
acp_rc_t testAllocPerfInitAllocatorLibcPrivTiny(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC,
                         ACP_FALSE,
                         TEST_BLOCK_SIZE_TINY);
}

acp_rc_t testAllocPerfInitAllocatorLibcPrivSmall(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC,
                         ACP_FALSE,
                         TEST_BLOCK_SIZE_SMALL);
}

acp_rc_t testAllocPerfInitAllocatorLibcPrivMedium(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC,
                         ACP_FALSE,
                         TEST_BLOCK_SIZE_MEDIUM);
}

acp_rc_t testAllocPerfInitAllocatorLibcPrivBig(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC, 
                         ACP_FALSE, 
                         TEST_BLOCK_SIZE_BIG);
}

/*
 * TLSF allocator thread private instance test init functions
 */
acp_rc_t testAllocPerfInitAllocatorTlsfPrivTiny(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF, 
                         ACP_FALSE, 
                         TEST_BLOCK_SIZE_TINY);
}

acp_rc_t testAllocPerfInitAllocatorTlsfPrivSmall(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF,
                         ACP_FALSE,
                         TEST_BLOCK_SIZE_SMALL);
}

acp_rc_t testAllocPerfInitAllocatorTlsfPrivMedium(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF,
                         ACP_FALSE, TEST_BLOCK_SIZE_MEDIUM);
}

acp_rc_t testAllocPerfInitAllocatorTlsfPrivBig(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF, 
                         ACP_FALSE, 
                         TEST_BLOCK_SIZE_BIG);
}

/*
 * LIBC allocator shared instance test init functions
 */
acp_rc_t testAllocPerfInitAllocatorLibcSharedTiny(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC,
                         ACP_TRUE,
                         TEST_BLOCK_SIZE_TINY);
}

acp_rc_t testAllocPerfInitAllocatorLibcSharedSmall(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC,
                         ACP_TRUE,
                         TEST_BLOCK_SIZE_SMALL);
}


acp_rc_t testAllocPerfInitAllocatorLibcSharedMedium(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC,
                         ACP_TRUE,
                         TEST_BLOCK_SIZE_MEDIUM);
}

acp_rc_t testAllocPerfInitAllocatorLibcSharedBig(actPerfFrmThrObj *aThrObj)
{
    gInitFlag = ACP_TRUE;

    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_LIBC, 
                         ACP_TRUE, 
                         TEST_BLOCK_SIZE_BIG);
}

/*
 * TLSF allocator shared instance test init functions
 */
acp_rc_t testAllocPerfInitAllocatorTlsfSharedTiny(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF,
                         ACP_TRUE,
                         TEST_BLOCK_SIZE_TINY);
}

acp_rc_t testAllocPerfInitAllocatorTlsfSharedSmall(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF,
                         ACP_TRUE,
                         TEST_BLOCK_SIZE_SMALL);
}

acp_rc_t testAllocPerfInitAllocatorTlsfSharedMedium(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF,
                         ACP_TRUE,
                         TEST_BLOCK_SIZE_MEDIUM);
}

acp_rc_t testAllocPerfInitAllocatorTlsfSharedBig(actPerfFrmThrObj *aThrObj)
{
    return testAllocInit(aThrObj,
                         ACL_MEM_ALLOC_TLSF, 
                         ACP_TRUE, 
                         TEST_BLOCK_SIZE_BIG);
}

/*
 * Test case body
 */
acp_rc_t testAllocPerfBody(actPerfFrmThrObj *aThrObj)
{
    testAllocStruct *sAllocStruct = (testAllocStruct*)aThrObj->mObj.mData;
    acl_mem_alloc_t *sAllocator   = NULL;

    acp_rc_t      sRC;
    acp_uint64_t  i = 0;
    acp_uint32_t  k = 0;
    void         *sPtr[TEST_SIZE_ARRAY_LEN] = {NULL};

    if (sAllocStruct->mShared == ACP_TRUE)
    {
        /* Use shared allocator instance */
        sAllocator = sAllocStruct->mAllocator;
    }
    else
    {
        /* Create thread private allocator instance */
        sRC = aclMemAllocGetInstance(sAllocStruct->mType,
                                     NULL,
                                     &sAllocator);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }


    /* Test loop */
    for(i = 0; i < aThrObj->mObj.mFrm->mLoopCount; i++)
    {
        if (sPtr[k] == NULL)
        {
            sRC = aclMemAlloc(sAllocator,
                          &sPtr[k],
                          gSizeArray[k]);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACT_CHECK(sPtr[k] != NULL);

            if ((k % 5) == 0)
            {
                sRC = aclMemFree(sAllocator, sPtr[k]);
                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

                sPtr[k] = NULL;
            }
            else
            {
               /* Do nothing */
            }
        }
        else
        {

            sRC = aclMemFree(sAllocator, sPtr[k]);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            sPtr[k] = NULL;

            if ((k % 10) == 0)
            {
                sRC = aclMemAlloc(sAllocator,
                                  &sPtr[k],
                                  gSizeArray[(k + 1) % TEST_SIZE_ARRAY_LEN]);

                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
                ACT_CHECK(sPtr[k] != NULL);
            }
            else
            {
                /* Do nothing */
            }
        }

        k++;

        k = k % TEST_SIZE_ARRAY_LEN;
    }


    for(k = 0; k < TEST_SIZE_ARRAY_LEN; k++)
    {
        sRC = aclMemFree(sAllocator, sPtr[k]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sPtr[k] = NULL;
    }

    if (sAllocStruct->mShared == ACP_TRUE)
    {
        /* Do nothing */
    }
    else
    {
        sRC = aclMemAllocFreeInstance(sAllocator);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return sRC;
}

/*
 * Test case finalize function
 */
acp_rc_t testAllocPerfFinal(actPerfFrmThrObj *aThrObj)
{
    acp_rc_t         sRC;
    testAllocStruct* sAllocStruct = (testAllocStruct*)aThrObj->mObj.mData;

    ACT_CHECK(NULL != sAllocStruct);

    if (sAllocStruct != NULL)
    {
        if (sAllocStruct->mShared == ACP_TRUE)
        {
            /* Free allocator instance  */
            sRC = aclMemAllocFreeInstance(sAllocStruct->mAllocator);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
            /* Do nothing */
        }

        acpMemFree(sAllocStruct);
    }
    else
    {
        /* Do nothing */
    }

    return ACP_RC_SUCCESS;
}

acp_char_t /*@unused@*/ *gPerName = "Memory allocator perf";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /*
     * Each thread has thread-private instance.
     * Thread creates instance and try allocation with own instance.
     */
    
    {  /* Test Tiny class size */
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {  /* Test Small class size */
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        32,
        1000000,
        testAllocPerfInitAllocatorLibcPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~512",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {  /* Test Medium class size */
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~4Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {  /* Test Big class size */
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=per-thread-own-instance ALLOC-SIZE=0~64Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfPrivBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },

    /*
     * Multi-thread Shared instance
     * Many thread try allocation with shared instance.
     */
    
    {  /* Test Tiny class size */
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedTiny,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {  /* Test Small class size */
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~512",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedSmall,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {  /* Test Medium class size */
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~4Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedMedium,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {  /* Test Big class size */
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        32,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        64,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        128,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=LIBC ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorLibcSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },
    {
        "TYPE=TLSF ATTR=shared-by-multithread-instance ALLOC-SIZE=0~64Kb",
        256,
        TEST_LOOP_COUNT,
        testAllocPerfInitAllocatorTlsfSharedBig,
        testAllocPerfBody,
        testAllocPerfFinal
    },

    ACT_PERF_FRM_SENTINEL
};
