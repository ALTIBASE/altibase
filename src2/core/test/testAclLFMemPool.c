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
 * $Id: $
 ******************************************************************************/

#include <act.h>
#include <aclLFMemPool.h>

#define ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE      1024
#define ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH     10
#define ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW      3
#define ACL_TEST_LOCKFREE_MEMPOOL_CHUNKS_DEFAULT 0

#define ACL_TEST_LOCKFREE_MEMPOOL_ALLOC_COUNT  1024
#define ACL_TEST_LOCKFREE_MEMPOOL_THREAD_COUNT 100

typedef struct testMTArgs {
    acp_sint32_t             mContent;
    acl_lockfree_mempool_t*  mMemPool;
    void*                    mMem;
} testMTArgs;

static testMTArgs gTestArgs[ACL_TEST_LOCKFREE_MEMPOOL_THREAD_COUNT];

void aclTestLFMemPoolCreation()
{
    acp_rc_t               sRC;
    acl_lockfree_mempool_t sMemPool;

    /* Should be success */
    sRC = aclLFMemPoolCreate(&sMemPool,
                             ACL_LOCKFREE_MEMPOOL_HEAP,
                             ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                             ACL_TEST_LOCKFREE_MEMPOOL_CHUNKS_DEFAULT);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC), ("Heap MemPool creation failed."));

    ACT_CHECK_DESC(sMemPool.mType == ACL_LOCKFREE_MEMPOOL_HEAP,
                   ("Wrong MemPool type."));

    ACT_CHECK_DESC(sMemPool.mBlockSize == ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                   ("Wrong MemPool block size."));

    ACT_CHECK_DESC(sMemPool.mEmptyHighLimit ==
                   ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                   ("Wrong MemPool chunk count high limit."));

    ACT_CHECK_DESC(sMemPool.mEmptyLowLimit ==
                   ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                   ("Wrong MemPool chunk count low limit."));

    ACT_CHECK_DESC(sMemPool.mEmptyCount == ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                   ("After initialization it should be %d (low limit) empty " \
                    "chunks in list. But %d empty chunks in list",
                    ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW, sMemPool.mEmptyCount));

    sRC = aclLFMemPoolDestroy(&sMemPool);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclLFMemPoolCreate(&sMemPool,
                             ACL_LOCKFREE_MEMPOOL_SHMEM,
                             ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                             ACL_TEST_LOCKFREE_MEMPOOL_CHUNKS_DEFAULT);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("Shared memory MemPool creation failed."));

    ACT_CHECK_DESC(sMemPool.mType == ACL_LOCKFREE_MEMPOOL_SHMEM,
                   ("Wrong MemPool type."));

    ACT_CHECK_DESC(sMemPool.mBlockSize == ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                   ("Wrong MemPool block size."));

    ACT_CHECK_DESC(sMemPool.mEmptyHighLimit ==
                   ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                   ("Wrong MemPool chunk count high limit."));

    ACT_CHECK_DESC(sMemPool.mEmptyLowLimit ==
                   ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                   ("Wrong MemPool chunk count low limit."));

    ACT_CHECK_DESC(sMemPool.mEmptyCount == ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                   ("After initialization it should be %d (low limit) empty " \
                    "chunks in list. But %d empty chunks in list",
                    ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW, sMemPool.mEmptyCount));

    sRC = aclLFMemPoolDestroy(&sMemPool);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclLFMemPoolCreate(&sMemPool,
                             ACL_LOCKFREE_MEMPOOL_MMAP,
                             ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                             ACL_TEST_LOCKFREE_MEMPOOL_CHUNKS_DEFAULT);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("Mmaped memory MemPool creation failed."));

    ACT_CHECK_DESC(sMemPool.mType == ACL_LOCKFREE_MEMPOOL_MMAP,
                   ("Wrong MemPool type."));

    ACT_CHECK_DESC(sMemPool.mBlockSize == ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                   ("Wrong MemPool block size."));

    ACT_CHECK_DESC(sMemPool.mEmptyHighLimit ==
                   ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                   ("Wrong MemPool chunk count high limit."));

    ACT_CHECK_DESC(sMemPool.mEmptyLowLimit ==
                   ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                   ("Wrong MemPool chunk count low limit."));

    ACT_CHECK_DESC(sMemPool.mEmptyCount == ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                   ("After initialization it should be %d (low limit) empty " \
                    "chunks in list. But %d empty chunks in list",
                    ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW, sMemPool.mEmptyCount));

    sRC = aclLFMemPoolDestroy(&sMemPool);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Should fail. High and low limits exchanged. */
    sRC = aclLFMemPoolCreate(&sMemPool,
                             ACL_LOCKFREE_MEMPOOL_HEAP,
                             ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                             ACL_TEST_LOCKFREE_MEMPOOL_CHUNKS_DEFAULT);
    ACT_CHECK_DESC(ACP_RC_IS_EINVAL(sRC),
                   ("Low limit is greater then high limit. " \
                    "Return value should be ACP_RC_EINVAL."));

}

void aclTestLFMemPoolAllocation(aclLFMemPoolTypes aType)
{
    acp_rc_t               sRC;
    acl_lockfree_mempool_t sMemPool;
    acp_sint32_t           i;
    acp_sint32_t           sReadValue;
    acp_sint32_t           j;
    void                   *sPtr[ACL_TEST_LOCKFREE_MEMPOOL_ALLOC_COUNT];

    sRC = aclLFMemPoolCreate(&sMemPool,
                             aType,
                             ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                             ACL_TEST_LOCKFREE_MEMPOOL_CHUNKS_DEFAULT);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("MemPool creation failed. Type: %d", aType));

    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_ALLOC_COUNT; i++)
    {
        sRC = aclLFMemPoolAlloc(&sMemPool, &sPtr[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Counldn't allocate %d elem.", i));
    }

    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_ALLOC_COUNT; i++)
    {
        for (j = 0; j < sMemPool.mBlockSize / sizeof(i); j++)
        {
            acpAtomicSet32((acp_char_t *)sPtr[i] + (j * sizeof(i)), i);
        }
    }

    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_ALLOC_COUNT; i++)
    {
        for (j = 0; j < sMemPool.mBlockSize / sizeof(i); j++)
        {
            sReadValue = acpAtomicGet32((acp_char_t *)sPtr[i] + (j * sizeof(i)));
            ACT_CHECK_DESC(sReadValue == i,
                           ("Value should be %d but %d for elem %d.",
                            i,
                            sReadValue,
                            i));
        }
    }

    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_ALLOC_COUNT; i++)
    {
        sRC = aclLFMemPoolFree(&sMemPool, sPtr[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Counldn't free %d elem.", i));
    }

    sRC = aclLFMemPoolDestroy(&sMemPool);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static acp_sint32_t aclTestLFMemPoolWriteThr(void *aArg)
{
    acp_rc_t     sRC;
    testMTArgs   *sArg = (testMTArgs *)aArg;
    acp_sint32_t i;

    sRC = aclLFMemPoolAlloc(sArg->mMemPool, &sArg->mMem);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("Counldn't allocate %d elem.", sArg->mContent));

    for (i = 0; i < sArg->mMemPool->mBlockSize / sizeof(sArg->mContent); i++)
    {
        acpAtomicSet32((acp_char_t *)sArg->mMem + (i * sizeof(sArg->mContent)),
                       sArg->mContent);
    }

    return 0;
}

static acp_sint32_t aclTestLFMemPoolReadThr(void *aArg)
{
    acp_rc_t               sRC;
    testMTArgs             *sArg = (testMTArgs *)aArg;
    acp_sint32_t           i;
    acp_sint32_t           sReadValue;

    for (i = 0; i < sArg->mMemPool->mBlockSize / sizeof(sArg->mContent); i++)
    {
        sReadValue = acpAtomicGet32((acp_char_t *)sArg->mMem + (i * sizeof(sArg->mContent)));
        ACT_CHECK_DESC(sReadValue == sArg->mContent,
                       ("Value should be %d but %d for elem %d.",
                        sArg->mContent,
                        sReadValue,
                        sArg->mContent));
    }

    sRC = aclLFMemPoolFree(sArg->mMemPool, sArg->mMem);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("Counldn't free %d elem.", sArg->mContent));

    return 0;
}

void aclTestLFMemPoolMTAllocation(aclLFMemPoolTypes aType)
{
    acp_rc_t               sRC;
    acl_lockfree_mempool_t sMemPool;
    acp_thr_t              sThrID[ACL_TEST_LOCKFREE_MEMPOOL_THREAD_COUNT];
    acp_sint32_t           i;

    sRC = aclLFMemPoolCreate(&sMemPool,
                             aType,
                             ACL_TEST_LOCKFREE_MEMPOOL_BLOCKSIZE,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_HIGH,
                             ACL_TEST_LOCKFREE_MEMPOOL_LIMIT_LOW,
                             ACL_TEST_LOCKFREE_MEMPOOL_CHUNKS_DEFAULT);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("MemPool creation failed. Type: %d", aType));

    /* Run writing threads */
    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_THREAD_COUNT; i++)
    {
        /* Init thread function arguments */
        gTestArgs[i].mContent = i;
        gTestArgs[i].mMemPool = &sMemPool;

        sRC = acpThrCreate(&sThrID[i],
                           NULL,
                           aclTestLFMemPoolWriteThr,
                           &gTestArgs[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Couldn't create write thread"));
    }

    /* Join writing threads */
    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThrID[i], NULL);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC), ("Couldn't join thread"));
    }

    /* Run reading threads */
    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_THREAD_COUNT; i++)
    {
        sRC = acpThrCreate(&sThrID[i],
                           NULL,
                           aclTestLFMemPoolReadThr,
                           &gTestArgs[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC), ("Couldn't create read thread"));
    }

    /* Join reading threads */
    for (i = 0; i < ACL_TEST_LOCKFREE_MEMPOOL_THREAD_COUNT; i++)
    {
        sRC = acpThrJoin(&sThrID[i], NULL);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC), ("Couldn't join thread"));
    }

    sRC = aclLFMemPoolDestroy(&sMemPool);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

acp_sint32_t main()
{
    ACT_TEST_BEGIN();

    aclTestLFMemPoolCreation();
    aclTestLFMemPoolAllocation(ACL_LOCKFREE_MEMPOOL_HEAP);
    aclTestLFMemPoolAllocation(ACL_LOCKFREE_MEMPOOL_SHMEM);
    aclTestLFMemPoolAllocation(ACL_LOCKFREE_MEMPOOL_MMAP);
    aclTestLFMemPoolMTAllocation(ACL_LOCKFREE_MEMPOOL_HEAP);
    aclTestLFMemPoolMTAllocation(ACL_LOCKFREE_MEMPOOL_SHMEM);
    aclTestLFMemPoolMTAllocation(ACL_LOCKFREE_MEMPOOL_MMAP);

    ACT_TEST_END();

    return 0;
}
