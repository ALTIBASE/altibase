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

#include <aclLFMemPool.h>
#include <aclSafeList.h>

#define ACL_LFMEMPOOL_BLOCK_HEADER_SIZE ACP_CFG_COMPILE_BIT
#define ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES ACL_LFMEMPOOL_BLOCK_HEADER_SIZE/8
#define ACL_LFMEMPOOL_CHUNK_HEADER_SIZE 64
#define ACL_LFMEMPOOL_CHUNK_HEADER_SIZE_BYTES ACL_LFMEMPOOL_CHUNK_HEADER_SIZE/8
#define ACL_LFMEMPOOL_BLOCK_COUNT ACL_LFMEMPOOL_CHUNK_HEADER_SIZE - 1

#define ACL_LFMEMPOOL_DEFAULT_SEGMENT_CHUNKS 100

static acp_key_t gSegmentCounter = 0;

ACP_INLINE void aclLFMemPoolChunkInit(void*      aChunk,
                                      acp_size_t aBlockSize,
                                      acp_bool_t aActive)
{
    acp_uint32_t i;
    acp_char_t   *sPosition = (acp_char_t *)aChunk;

    /* Set chunk header */
    if (aActive == ACP_TRUE)
    {
        acpAtomicSet64(aChunk, ACL_LOCKFREE_MEMPOOL_EMPTYCHUNK);
    }
    else
    {
        acpAtomicSet64(aChunk, ACL_LOCKFREE_MEMPOOL_INACTIVECHUNK);
    }

    sPosition += ACL_LFMEMPOOL_CHUNK_HEADER_SIZE_BYTES;

    for (i = 0; i < ACL_LFMEMPOOL_BLOCK_COUNT; i++)
    {
#if defined(ACP_CFG_COMPILE_64BIT)
        acpAtomicSet64(sPosition, (acp_size_t)aChunk);
#else
        acpAtomicSet32(sPosition, (acp_size_t)aChunk);
#endif
        sPosition += aBlockSize + ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES;
    }
}

ACP_INLINE acp_rc_t aclLFMemPoolChunkAllocHeap(acl_lockfree_mempool_t* aMemPool,
                                               void**                  aChunk)
{
    acp_rc_t       sRC;
    acp_size_t     sChunkSize;

    sChunkSize = ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES;
    sChunkSize += aMemPool->mBlockSize;
    sChunkSize *= ACL_LFMEMPOOL_BLOCK_COUNT;
    sChunkSize += ACL_LFMEMPOOL_CHUNK_HEADER_SIZE_BYTES;
    sChunkSize = ACP_ALIGN8(sChunkSize);

    sRC = acpMemAlloc(aChunk, sChunkSize);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MALLOC);

    acpMemSet(*aChunk, 0x00, sChunkSize);

    aclLFMemPoolChunkInit(*aChunk, aMemPool->mBlockSize, ACP_TRUE);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_MALLOC)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_INLINE acp_rc_t aclLFMemPoolChunkFreeHeap(void* aChunk)
{
    acpMemFree(aChunk);

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t aclLFMemPoolSegmentInit(void*                   aAddress,
                                            acp_size_t              aChunkSize,
                                            acp_uint32_t            aChunkCount,
                                            acp_size_t              aBlockSize)
{
    acp_rc_t       sRC;
    acp_uint32_t   i;

    ACP_TEST_RAISE(aAddress == NULL, E_NULL);

    for (i = 0; i < aChunkCount; i++)
    {
        aclLFMemPoolChunkInit((acp_char_t *)aAddress + (i * aChunkSize),
                              aBlockSize,
                              ACP_FALSE);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

#define ACL_LFMEMPOOL_SHM_MAX_PERM    (   \
              ACP_S_IRUSR | ACP_S_IWUSR | \
              ACP_S_IRGRP | ACP_S_IWGRP | \
              ACP_S_IROTH | ACP_S_IWOTH)

ACP_INLINE acp_rc_t aclLFMemPoolDeleteSegment(acl_lockfree_mempool_t* aMemPool,
                                              acl_lockfree_segment_t* aSegment);

ACP_INLINE acp_rc_t aclLFMemPoolAddSegment(acl_lockfree_mempool_t*  aMemPool,
                                           acl_lockfree_segment_t*  aSegment)
{
    acp_rc_t       sRC;
    acp_size_t     sChunkSize;
    acp_key_t      sKey;
    acp_sint32_t   sOldKey;

    ACP_TEST_RAISE(aMemPool == NULL || aSegment == NULL, E_NULL);

    sChunkSize = ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES;
    sChunkSize += aMemPool->mBlockSize;
    sChunkSize *= ACL_LFMEMPOOL_BLOCK_COUNT;
    sChunkSize += ACL_LFMEMPOOL_CHUNK_HEADER_SIZE_BYTES;
    sChunkSize = ACP_ALIGN8(sChunkSize);

    switch (aMemPool->mType)
    {
        case ACL_LOCKFREE_MEMPOOL_SHMEM:
            sKey = gSegmentCounter;
            aSegment->mKey = (acp_key_t)acpProcGetSelfID() + sKey;
            sRC = acpShmCreate(aSegment->mKey,
                               sChunkSize * aMemPool->mSegmentChunks,
                               ACL_LFMEMPOOL_SHM_MAX_PERM);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_SHM_CREATE);
            sRC = acpShmAttach(&(aSegment->mShm), aSegment->mKey);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_SHM_ATTACH);
            sOldKey = acpAtomicCas32(&gSegmentCounter, sKey + 1, sKey);
            if (sOldKey == sKey)
            {
                aclLFMemPoolSegmentInit(ACP_ALIGN8_PTR(acpShmGetAddress(&(aSegment->mShm))),
                                        sChunkSize,
                                        aMemPool->mSegmentChunks,
                                        aMemPool->mBlockSize);
            }
            else
            {
                (void)aclLFMemPoolDeleteSegment(aMemPool, aSegment);
                ACP_RAISE(E_CHANGED);
            }
            break;
        case ACL_LOCKFREE_MEMPOOL_MMAP:
            sKey = gSegmentCounter;
            sRC = acpMmap(&(aSegment->mMmap),
                          sChunkSize * aMemPool->mSegmentChunks,
                          ACP_MMAP_READ | ACP_MMAP_WRITE,
                          ACP_MMAP_SHARED);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MMAP_CREATE);
            sOldKey = acpAtomicCas32(&gSegmentCounter, sKey + 1, sKey);
            if (sOldKey == sKey)
            {
                aclLFMemPoolSegmentInit(ACP_ALIGN8_PTR(acpMmapGetAddress(&(aSegment->mMmap))),
                                        sChunkSize,
                                        aMemPool->mSegmentChunks,
                                        aMemPool->mBlockSize);
            }
            else
            {
                (void)aclLFMemPoolDeleteSegment(aMemPool, aSegment);
                ACP_RAISE(E_CHANGED);
            }
            break;
        default:
            ACP_RAISE(E_WRONG_TYPE);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_WRONG_TYPE)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_SHM_CREATE)
    {
    }

    ACP_EXCEPTION(E_SHM_ATTACH)
    {
    }

    ACP_EXCEPTION(E_MMAP_CREATE)
    {
    }

    ACP_EXCEPTION(E_CHANGED)
    {
        sRC = ACP_RC_EAGAIN;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_INLINE acp_rc_t aclLFMemPoolDeleteSegment(acl_lockfree_mempool_t* aMemPool,
                                              acl_lockfree_segment_t* aSegment)
{
    acp_rc_t       sRC;

    ACP_TEST_RAISE(aMemPool == NULL || aSegment == NULL, E_NULL);

    switch (aMemPool->mType)
    {
        case ACL_LOCKFREE_MEMPOOL_SHMEM:
            sRC = acpShmDetach(&(aSegment->mShm));
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

            sRC = acpShmDestroy(aSegment->mKey);
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
            break;
        case ACL_LOCKFREE_MEMPOOL_MMAP:
            sRC = acpMmapDetach(&(aSegment->mMmap));
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
            break;
        default:
            ACP_RAISE(E_WRONG_TYPE);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_WRONG_TYPE)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_INLINE acp_rc_t aclLFMemPoolFindChunkInSegment(
                            void*                   aAddress,
                            acp_size_t              aChunkSize,
                            acp_uint32_t            aChunkCount,
                            void**                  aChunk)
{
    acp_rc_t                 sRC;
    acp_uint32_t             i;
    void*                    sAddress = NULL;

    ACP_TEST_RAISE(aAddress == NULL, E_NULL);

    for (i = 0; i < aChunkCount; i++)
    {
        sAddress = (acp_char_t *)aAddress + (i * aChunkSize);
        if (acpAtomicCas64(sAddress,
                           ACL_LOCKFREE_MEMPOOL_EMPTYCHUNK,
                           ACL_LOCKFREE_MEMPOOL_INACTIVECHUNK)
                == ACL_LOCKFREE_MEMPOOL_INACTIVECHUNK)
        {
            *aChunk = sAddress;
            break;
        }
        else
        {
            /* Do nothing */
        }
    }

    ACP_TEST_RAISE(*aChunk == NULL, E_NOTFOUND);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_NOTFOUND)
    {
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_INLINE acp_rc_t aclLFMemPoolChunkAllocSegment(
                        acl_lockfree_mempool_t*                       aMemPool,
                        void**                                        aChunk)
{
    acp_rc_t                 sRC;
    acp_size_t               sChunkSize;
    acl_safelist_node_t*     sNode = NULL;
    acl_lockfree_segment_t*  sSegment = NULL;
    void*                    sAddress = NULL;

    ACP_TEST_RAISE(aMemPool == NULL || aChunk == NULL, E_NULL);

    *aChunk = NULL;

    sChunkSize = ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES;
    sChunkSize += aMemPool->mBlockSize;
    sChunkSize *= ACL_LFMEMPOOL_BLOCK_COUNT;
    sChunkSize += ACL_LFMEMPOOL_CHUNK_HEADER_SIZE_BYTES;
    sChunkSize = ACP_ALIGN8(sChunkSize);

    do
    {
        sRC = aclSafeListFirst(&(aMemPool->mSegmentList), &sNode);
        while (ACP_RC_NOT_ENOENT(sRC))
        {
            sSegment = (acl_lockfree_segment_t *)sNode->mData;

            switch (aMemPool->mType)
            {
                case ACL_LOCKFREE_MEMPOOL_SHMEM:
                    sAddress = ACP_ALIGN8_PTR(acpShmGetAddress(&(sSegment->mShm)));
                    break;
                case ACL_LOCKFREE_MEMPOOL_MMAP:
                    sAddress = ACP_ALIGN8_PTR(acpMmapGetAddress(&(sSegment->mMmap)));
                    break;
                default:
                    ACP_RAISE(E_WRONG_TYPE);
            }

            /* Find free chunk inside segment */
            sRC = aclLFMemPoolFindChunkInSegment(sAddress,
                                                 sChunkSize,
                                                 aMemPool->mSegmentChunks,
                                                 aChunk);
            if (ACP_RC_IS_SUCCESS(sRC))
            {
                break;
            }
            else if (ACP_RC_IS_ENOENT(sRC))
            {
                sRC = aclSafeListNext(&(aMemPool->mSegmentList),sNode, &sNode);
            }
            else
            {
                ACP_RAISE(E_NULL);
            }
        }
        if (ACP_RC_IS_ENOENT(sRC))
        {
            sRC = acpMemAlloc((void **)&sSegment,
                              sizeof(acl_lockfree_segment_t));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MALLOC);

            sRC = aclLFMemPoolAddSegment(aMemPool, sSegment);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC) && ACP_RC_NOT_EAGAIN(sRC),
                           E_ADD_SEGMENT_FAILED);
            if (ACP_RC_IS_SUCCESS(sRC))
            {
                sRC = acpMemAlloc((void **)&sNode, sizeof(acl_safelist_node_t));
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_NODE_MALLOC);

                sNode->mData = (void *)sSegment;

                sRC = aclSafeListPushBack(&(aMemPool->mSegmentList), sNode);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_PUSH_FAILED);
            }
            else
            {
                acpMemFree(sSegment);
            }
        }
    } while (*aChunk == NULL);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_WRONG_TYPE)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_MALLOC)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_NODE_MALLOC)
    {
        acpMemFree(sSegment);
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_LIST_PUSH_FAILED)
    {
        acpMemFree(sSegment);
        acpMemFree(sNode);
    }

    ACP_EXCEPTION(E_ADD_SEGMENT_FAILED)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_INLINE acp_rc_t aclLFMemPoolChunkFreeSegment(void* aChunk)
{
    acpAtomicSet64(aChunk, ACL_LOCKFREE_MEMPOOL_INACTIVECHUNK);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t aclLFMemPoolCreate(acl_lockfree_mempool_t *aMemPool,
                                       aclLFMemPoolTypes      aType,
                                       acp_uint32_t           aBlockSize,
                                       acp_uint32_t           aEmptyHighLimit,
                                       acp_uint32_t           aEmptyLowLimit,
                                       acp_uint32_t           aSegmentChunks)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNode = NULL;
    acl_lockfree_segment_t* sSegment = NULL;

    /* Check parameters */
    ACP_TEST_RAISE(aEmptyHighLimit < aEmptyLowLimit, E_WRONG_PARAM);

    aMemPool->mType           = aType;
    aMemPool->mBlockSize      = ACP_ALIGN8(aBlockSize);
    aMemPool->mEmptyHighLimit = aEmptyHighLimit;
    aMemPool->mEmptyLowLimit  = aEmptyLowLimit;

    if (aSegmentChunks > 0)
    {
        aMemPool->mSegmentChunks  = aSegmentChunks;
    }
    else
    {
        aMemPool->mSegmentChunks = ACL_LFMEMPOOL_DEFAULT_SEGMENT_CHUNKS;
    }

    sRC = aclSafeListCreate(&(aMemPool->mList));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_CREATE_FAILED);

    switch (aMemPool->mType)
    {
        case ACL_LOCKFREE_MEMPOOL_HEAP:
            aMemPool->mFuncs.mChunkAlloc = *aclLFMemPoolChunkAllocHeap;
            aMemPool->mFuncs.mChunkFree = *aclLFMemPoolChunkFreeHeap;
            break;
        case ACL_LOCKFREE_MEMPOOL_SHMEM:
        case ACL_LOCKFREE_MEMPOOL_MMAP:
            aMemPool->mFuncs.mChunkAlloc = *aclLFMemPoolChunkAllocSegment;
            aMemPool->mFuncs.mChunkFree = *aclLFMemPoolChunkFreeSegment;
            sRC = aclSafeListCreate(&(aMemPool->mSegmentList));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_CREATE_FAILED);

            sRC = acpMemAlloc((void **)&sSegment,
                              sizeof(acl_lockfree_segment_t));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MALLOC);

            if (ACP_RC_IS_SUCCESS(aclLFMemPoolAddSegment(aMemPool, sSegment)))
            {
                sRC = acpMemAlloc((void **)&sNode, sizeof(acl_safelist_node_t));
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_NODE_MALLOC);

                sNode->mData = (void *)sSegment;

                sRC = aclSafeListPushBack(&(aMemPool->mSegmentList), sNode);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_PUSH_FAILED);
            }
            else
            {
                acpMemFree(sSegment);
            }
            break;
        case ACL_LOCKFREE_MEMPOOL_UNINITIALIZED:
        default:
            ACP_RAISE(E_WRONG_PARAM);
            break;
    }

    for (aMemPool->mEmptyCount = 0;
         aMemPool->mEmptyCount < aEmptyLowLimit;
         aMemPool->mEmptyCount++)
    {
        sRC = acpMemAlloc((void **)&sNode, sizeof(acl_safelist_node_t));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MALLOC);

        sRC = aMemPool->mFuncs.mChunkAlloc(aMemPool, &(sNode->mData));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_CHUNK_ALLOC_FAILED);

        sRC = aclSafeListPushBack(&(aMemPool->mList), sNode);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_PUSH_FAILED);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_LIST_CREATE_FAILED)
    {
        /* Passthroufgh */
    }

    ACP_EXCEPTION(E_WRONG_PARAM)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_MALLOC)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_NODE_MALLOC)
    {
        acpMemFree(sSegment);
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_SEGMENT_LIST_PUSH_FAILED)
    {
        acpMemFree(sSegment);
        acpMemFree(sNode);
    }

    ACP_EXCEPTION(E_CHUNK_ALLOC_FAILED)
    {
        acpMemFree(sNode);
        while (ACP_RC_NOT_ENOENT(aclSafeListPopHead(&(aMemPool->mList),
                                                    &sNode)))
        {
            (void)aMemPool->mFuncs.mChunkFree(sNode->mData);
            acpMemFree(sNode);
        }
        aMemPool->mEmptyCount = 0;
    }

    ACP_EXCEPTION(E_LIST_PUSH_FAILED)
    {
        (void)aMemPool->mFuncs.mChunkFree(sNode->mData);
        acpMemFree(sNode);
        while (ACP_RC_NOT_ENOENT(aclSafeListPopHead(&(aMemPool->mList),
                                                    &sNode)))
        {
            (void)aMemPool->mFuncs.mChunkFree(sNode->mData);
            acpMemFree(sNode);
        }
        aMemPool->mEmptyCount = 0;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_EXPORT acp_rc_t aclLFMemPoolDestroy(acl_lockfree_mempool_t *aMemPool)
{
    acp_rc_t              sRC;
    acl_safelist_node_t*  sAnchor = NULL;

    sRC = aclSafeListPopHead(&(aMemPool->mList), &sAnchor);
    while (sRC != ACP_RC_ENOENT)
    {
        sRC = aMemPool->mFuncs.mChunkFree(sAnchor->mData);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_CHUNK_FREE_FAILED);
        acpMemFree(sAnchor);
        sRC = aclSafeListPopHead(&(aMemPool->mList), &sAnchor);
    }

    sRC = aclSafeListDestroy(&(aMemPool->mList));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_DEL_FAILED);

    switch (aMemPool->mType)
    {
        case ACL_LOCKFREE_MEMPOOL_HEAP:
            break;
        case ACL_LOCKFREE_MEMPOOL_SHMEM:
        case ACL_LOCKFREE_MEMPOOL_MMAP:
	    sRC = aclSafeListPopHead(&(aMemPool->mSegmentList),
                                     &sAnchor);
            while (ACP_RC_NOT_ENOENT(sRC))
            {
                sRC = aclLFMemPoolDeleteSegment(
                          aMemPool,
                          (acl_lockfree_segment_t *)sAnchor->mData);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_SEGMENT_DEL_FAILED);
                acpMemFree(sAnchor->mData);
                acpMemFree(sAnchor);
	        sRC = aclSafeListPopHead(&(aMemPool->mSegmentList),
                                         &sAnchor);
            }
            sRC = aclSafeListDestroy(&(aMemPool->mSegmentList));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_DEL_FAILED);
            break;
        default:
            break;
    }

    aMemPool->mType       = ACL_LOCKFREE_MEMPOOL_UNINITIALIZED;
    aMemPool->mBlockSize  = 0;
    aMemPool->mEmptyCount = 0;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_CHUNK_FREE_FAILED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_NODE_DEL_FAILED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_LIST_DEL_FAILED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_SEGMENT_DEL_FAILED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_INLINE acp_rc_t
aclLFMemPoolAllocFromChunk(void*                     aChunk,
                           acp_size_t                aBlockSize,
                           void**                    aPtr,
                           aclLFMemPoolAllocResults* aResult)
{
    acp_rc_t             sRC;
    acp_sint64_t         sChunkHeader;
    acp_sint64_t         sEmptyMask;
    acp_sint64_t         sNewChunkHeader;
    acp_sint64_t         sAllocMask;
    acp_sint32_t         sBlockNumber;

    *aResult = ACL_LOCKFREE_MEMPOOL_NONE;

    ACP_TEST_RAISE(aChunk == NULL, E_NULL);

    do
    {
        sChunkHeader = acpAtomicGet64(aChunk);

        ACP_TEST_RAISE(sChunkHeader == ACL_LOCKFREE_MEMPOOL_FULLCHUNK, E_FULL);

        if (sChunkHeader == ACL_LOCKFREE_MEMPOOL_EMPTYCHUNK)
        {
            *aResult = ACL_LOCKFREE_MEMPOOL_WAS_EMPTY;
        }
        else
        {
            /* Do nothing */
        }

        sEmptyMask = sChunkHeader ^ ACL_LOCKFREE_MEMPOOL_FULLCHUNK;

        sEmptyMask = acpByteOrderTOH8(sEmptyMask);
        sBlockNumber = acpBitFls64((acp_uint64_t)sEmptyMask);
        sEmptyMask = acpByteOrderTON8(sEmptyMask);

        sAllocMask = ACL_LOCKFREE_MEMPOOL_ONELSB;
        sAllocMask = acpByteOrderTOH8(sAllocMask);
        sAllocMask <<= sBlockNumber;
        sAllocMask = acpByteOrderTON8(sAllocMask);
        sNewChunkHeader = sChunkHeader | sAllocMask;

        if (sNewChunkHeader == ACL_LOCKFREE_MEMPOOL_FULLCHUNK)
        {
            *aResult = ACL_LOCKFREE_MEMPOOL_FULL;
        }
        else
        {
            /* Do nothing */
        }
    } while (acpAtomicCas64(aChunk,
                            sNewChunkHeader,
                            sChunkHeader) != sChunkHeader);

    *aPtr = (acp_char_t *)aChunk + ACL_LFMEMPOOL_CHUNK_HEADER_SIZE_BYTES +
            ((aBlockSize + ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES) *
            (62 - sBlockNumber)) + ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_FULL)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_EXPORT acp_rc_t aclLFMemPoolAlloc(acl_lockfree_mempool_t* aMemPool,
                                      void**                  aPtr)
{
    acp_rc_t                 sRC;
    acl_safelist_node_t*     sNode = NULL;
    aclLFMemPoolAllocResults sResult;
    acp_uint32_t             sEmptyCount = 0;
    acp_bool_t               sAllocated = ACP_FALSE;

    ACP_TEST_RAISE(aMemPool == NULL, E_NULL);


    do
    {
        while (ACP_RC_NOT_SUCCESS(aclSafeListFirst(&(aMemPool->mList),
                                                   &sNode)));
    }
    while(ACP_RC_NOT_SUCCESS(aclLFMemPoolAllocFromChunk(sNode->mData,
                                                        aMemPool->mBlockSize,
                                                        aPtr,
                                                        &sResult)));

    if (sResult == ACL_LOCKFREE_MEMPOOL_FULL)
    {
        sRC = aclSafeListPopHead(&(aMemPool->mList), &sNode);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_REMOVE_CHUNK);

        if (aMemPool->mEmptyCount < aMemPool->mEmptyLowLimit)
        {
            sRC = aMemPool->mFuncs.mChunkAlloc(aMemPool, &(sNode->mData));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_CHUNK_ALLOC_FAILED);

            sRC = aclSafeListPushBack(&(aMemPool->mList), sNode);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_PUSH_FAILED);
        }
        else
        {
            sNode->mData = NULL;
            acpMemFree(sNode);
        }
    }
    else if (sResult == ACL_LOCKFREE_MEMPOOL_WAS_EMPTY)
    {
        sRC = acpMemAlloc((void **)&sNode, sizeof(acl_safelist_node_t));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MALLOC);

        sRC = aMemPool->mFuncs.mChunkAlloc(aMemPool, &(sNode->mData));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_CHUNK_ALLOC_FAILED);

        sRC = aclSafeListPushBack(&(aMemPool->mList), sNode);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_PUSH_FAILED);
    }
    else
    {
        /* Do nothing */
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_MALLOC)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_CHUNK_ALLOC_FAILED)
    {
        acpMemFree(sNode);
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_LIST_PUSH_FAILED)
    {
        aMemPool->mFuncs.mChunkFree(sNode->mData);
        acpMemFree(sNode);
        (void)acpAtomicDec32(&(aMemPool->mEmptyCount));
    }

    ACP_EXCEPTION(E_GET_CHUNK)
    {
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION(E_REMOVE_CHUNK)
    {
        /* Passthrough */
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_INLINE acp_rc_t aclLFMemPoolShrink(acl_lockfree_mempool_t* aMemPool)
{
    acp_rc_t             sRC;
    acl_safelist_node_t* sNode = NULL;
    acp_sint64_t         sChunkHeader;

    while (aMemPool->mEmptyCount >= aMemPool->mEmptyHighLimit)
    {
        (void)acpAtomicDec32(&(aMemPool->mEmptyCount));

        sRC = aclSafeListLast(&(aMemPool->mList), &sNode);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        while (acpAtomicGet64(sNode->mData) != ACL_LOCKFREE_MEMPOOL_EMPTYCHUNK)
        {
            sRC = aclSafeListPrev(&(aMemPool->mList), sNode, &sNode);
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
            ACP_TEST(sNode->mData == NULL);
        }

        sRC = aclSafeListDeleteNode(&(aMemPool->mList), sNode, ACP_TRUE);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        (void)aMemPool->mFuncs.mChunkFree(sNode->mData);
        acpMemFree(sNode);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;

    (void)acpAtomicInc32(&(aMemPool->mEmptyCount));

    return sRC;
}

ACP_INLINE acp_rc_t
aclLFMemPoolFreeInChunk(void*                     aChunk,
                        acp_size_t                aBlockSize,
                        void*                     aPtr,
                        aclLFMemPoolAllocResults* aResult)
{
    acp_rc_t             sRC;
    acp_sint64_t         sChunkHeader;
    acp_sint64_t         sNewChunkHeader;
    acp_sint64_t         sFreeMask;
    acp_sint32_t         sBlockNumber;

    ACP_TEST_RAISE((aChunk == NULL || aPtr == NULL), E_NULL);

    sBlockNumber = ((acp_sint32_t)(((acp_char_t *)aPtr - (acp_char_t *)aChunk) -
                                   ACL_LFMEMPOOL_CHUNK_HEADER_SIZE_BYTES) /
                                  (aBlockSize +
                                   ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES)) + 1;

    do
    {
        *aResult = ACL_LOCKFREE_MEMPOOL_NONE;

        sChunkHeader = acpAtomicGet64(aChunk);

        if (sChunkHeader == ACL_LOCKFREE_MEMPOOL_FULLCHUNK)
        {
            *aResult = ACL_LOCKFREE_MEMPOOL_WAS_FULL;
        }
        else
        {
            /* Do nothing */
        }

        sFreeMask = ACL_LOCKFREE_MEMPOOL_ONELSB;
        sFreeMask = acpByteOrderTOH8(sFreeMask);
        sFreeMask <<= 63 - sBlockNumber;
        sFreeMask = acpByteOrderTON8(sFreeMask);

        sNewChunkHeader = sChunkHeader ^ sFreeMask;

        /* Check if it empty now */
        if (sNewChunkHeader == ACL_LOCKFREE_MEMPOOL_EMPTYCHUNK)
        {
            *aResult = ACL_LOCKFREE_MEMPOOL_EMPTY;
        }
        else
        {
            /* Do nothing */
        }
    } while (acpAtomicCas64(aChunk,
                            sNewChunkHeader,
                            sChunkHeader) != sChunkHeader);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

ACP_EXPORT acp_rc_t aclLFMemPoolFree(acl_lockfree_mempool_t* aMemPool,
                                     void*                   aPtr)
{
    acp_rc_t                 sRC;
    acp_sint64_t             sChunkHeader;
    acp_uint8_t*             sChunk = NULL;
    aclLFMemPoolAllocResults sResult;
    acl_safelist_node_t*     sNode = NULL;

    ACP_TEST_RAISE((aMemPool == NULL || aPtr == NULL), E_NULL);

#if defined(ACP_CFG_COMPILE_64BIT)
    sChunk = (acp_uint8_t *)acpAtomicGet64(
                                (acp_uint8_t *)aPtr -
                                ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES);
#else
    sChunk = (acp_uint8_t *)acpAtomicGet32(
                                (acp_uint8_t *)aPtr -
                                ACL_LFMEMPOOL_BLOCK_HEADER_SIZE_BYTES);
#endif

    sRC = aclLFMemPoolFreeInChunk((void*)sChunk,
                                  aMemPool->mBlockSize,
                                  aPtr,
                                  &sResult);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_FREE_CHUNK);

    /* If chunk was full we need to add it to list */
    if (sResult == ACL_LOCKFREE_MEMPOOL_WAS_FULL)
    {
        sRC = acpMemAlloc((void **)&sNode, sizeof(acl_safelist_node_t));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MALLOC);

        sNode->mData = (void *)sChunk;

        sRC = aclSafeListPushBack(&(aMemPool->mList), sNode);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_LIST_PUSH_FAILED);
    }

    /* If chunk is empty after free run shrink procedure */
    if (sResult == ACL_LOCKFREE_MEMPOOL_EMPTY)
    {
        (void)acpAtomicInc32(&(aMemPool->mEmptyCount));

        sRC = aclLFMemPoolShrink(aMemPool);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NULL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(E_FREE_CHUNK)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(E_MALLOC)
    {
        sRC = ACP_RC_ENOMEM;
    }
    ACP_EXCEPTION(E_LIST_PUSH_FAILED)
    {
        acpMemFree(sNode);
    }

    ACP_EXCEPTION_END;

    return sRC;
}
