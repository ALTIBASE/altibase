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
 * $Id: aclMemArea.c 4955 2009-03-24 09:31:05Z djin $
 ******************************************************************************/

#include <acpAlign.h>
#include <aceAssert.h>
#include <aclMemArea.h>


#define ACL_MEM_AREA_COULD_ALLOC(aChunk, aSize)                 \
    (((aChunk) != NULL) && ((aChunk)->mChunkSize >= (aSize)))

#define ACL_MEM_AREA_USER_ADDR_FROM_CHUNK(aChunk)               \
    (void *)((acp_char_t *)sChunk +                             \
             sChunk->mAllocSize +                               \
             ACP_ALIGN8(sizeof(aclMemAreaChunk)))

typedef struct aclMemAreaChunk
{
    acp_size_t      mChunkSize;     /* total size of chunk                 */
    acp_size_t      mAllocSize;     /* allocated size of chunk             */
    acp_list_node_t mChunkListNode; /* node of chunk list                  */
                                    /* and, followed by allocatable blocks */
} aclMemAreaChunk;


ACP_INLINE acp_rc_t aclMemAreaAllocChunk(acl_mem_area_t   *aArea,
                                         aclMemAreaChunk **aChunk,
                                         acp_size_t        aSize)
{
    aclMemAreaChunk *sChunk = NULL;
    acp_rc_t sRC;

    aSize = ACP_MAX(aArea->mChunkSize, aSize);

    sRC = acpMemAlloc((void **)&sChunk,
                      ACP_ALIGN8(sizeof(aclMemAreaChunk)) + aSize);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        *aChunk = sChunk;
    }

    sChunk->mChunkSize = aSize;
    sChunk->mAllocSize = 0;

    acpListInitObj(&sChunk->mChunkListNode, sChunk);

    acpListInsertAfter(aArea->mCurrentChunkListNode, &sChunk->mChunkListNode);

    aArea->mCurrentChunkListNode = &sChunk->mChunkListNode;

    return ACP_RC_SUCCESS;
}

ACP_INLINE void aclMemAreaFreeChunk(acl_mem_area_t  *aArea,
                                    aclMemAreaChunk *aChunk)
{
    if (aArea->mCurrentChunkListNode == &aChunk->mChunkListNode)
    {
        aArea->mCurrentChunkListNode = aChunk->mChunkListNode.mPrev;
    }
    else
    {
        /* do nothing */
    }

    acpListDeleteNode(&aChunk->mChunkListNode);

    acpMemFree(aChunk);
}


/**
 * creates a memory area
 * @param aArea pointer to the memory area
 * @param aChunkSize size of a memory chunk
 */
ACP_EXPORT void aclMemAreaCreate(acl_mem_area_t *aArea, acp_size_t aChunkSize)
{
#if defined(ACP_CFG_MEMORY_CHECK)
    ACP_UNUSED(aChunkSize);

    aArea->mChunkSize = 0;
#else
    aArea->mChunkSize = ACP_ALIGN8(aChunkSize);
#endif

#if defined(ACP_CFG_DEBUG)
    aArea->mChunkCount = 0;
#endif

    acpListInit(&aArea->mChunkList);

    aArea->mCurrentChunkListNode = &aArea->mChunkList;
}

/**
 * destroys a memory area
 * @param aArea pointer to the memory area
 *
 * it deallocates all memory allocated by the memory area
 */
ACP_EXPORT void aclMemAreaDestroy(acl_mem_area_t *aArea)
{
    acp_list_node_t *sIterator = NULL;
    acp_list_node_t *sNextNode = NULL;
    aclMemAreaChunk *sChunk = NULL;

#if defined(ACP_CFG_DEBUG)
    sChunk = aArea->mCurrentChunkListNode->mObj;

    if (sChunk != NULL)
    {
        ACE_DASSERT(aArea->mChunkCount == 1);
        ACE_DASSERT(sChunk->mAllocSize == 0);
    }
    else
    {
        ACE_DASSERT(aArea->mChunkCount == 0);
    }
#endif

    ACP_LIST_ITERATE_SAFE(&aArea->mChunkList, sIterator, sNextNode)
    {
        sChunk = sIterator->mObj;

        aclMemAreaFreeChunk(aArea, sChunk);
    }
}

/**
 * allocates a memory block from the memory area
 * @param aArea pointer to the memory area
 * @param aAddr pointer to a variable to store allocated memory address
 * @param aSize size of the memory block to allocate
 * @return result code
 */
ACP_EXPORT acp_rc_t aclMemAreaAlloc(acl_mem_area_t  *aArea,
                                    void           **aAddr,
                                    acp_size_t       aSize)
{
    aclMemAreaChunk *sChunk = NULL;
    acp_rc_t         sRC;

    aSize = ACP_ALIGN8(aSize);

    sChunk = aArea->mCurrentChunkListNode->mObj;

    if (ACL_MEM_AREA_COULD_ALLOC(sChunk, sChunk->mAllocSize + aSize))
    {
        /* do nothing */
    }
    else
    {
        sChunk = aArea->mCurrentChunkListNode->mNext->mObj;

        if (ACL_MEM_AREA_COULD_ALLOC(sChunk, aSize))
        {
#if defined(ACP_CFG_DEBUG)
            aArea->mChunkCount++;
#endif
            aArea->mCurrentChunkListNode = &sChunk->mChunkListNode;
            sChunk->mAllocSize           = 0;
        }
        else
        {
            sRC = aclMemAreaAllocChunk(aArea, &sChunk, aSize);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                return sRC;
            }
            else
            {
                /* do nothing */
            }
        }
    }

    *aAddr = ACL_MEM_AREA_USER_ADDR_FROM_CHUNK(sChunk);

    sChunk->mAllocSize += aSize;

    return ACP_RC_SUCCESS;
}

/**
 * gets the current snapshot of the memory area
 * @param aArea pointer to the memory area
 * @param aSnapshot pointer to the memory area snapshot
 */
ACP_EXPORT void aclMemAreaGetSnapshot(acl_mem_area_t          *aArea,
                                      acl_mem_area_snapshot_t *aSnapshot)
{
    aclMemAreaChunk *sChunk = aArea->mCurrentChunkListNode->mObj;

#if defined(ACP_CFG_DEBUG)
    aSnapshot->mMemArea    = aArea;
    aSnapshot->mChunkCount = aArea->mChunkCount;
#endif

    if (sChunk == NULL)
    {
        aSnapshot->mAllocSize = 0;
    }
    else
    {
        aSnapshot->mAllocSize = sChunk->mAllocSize;
    }

    aSnapshot->mCurrentChunkListNode = aArea->mCurrentChunkListNode;
}

/**
 * frees memory blocks allocated after the specified snapshot of the memory area
 * @param aArea pointer to the memory area
 * @param aSnapshot pointer to the memory area snapshot
 */
ACP_EXPORT void aclMemAreaFreeToSnapshot(acl_mem_area_t          *aArea,
                                         acl_mem_area_snapshot_t *aSnapshot)
{
    aclMemAreaChunk *sChunk = aSnapshot->mCurrentChunkListNode->mObj;

#if defined(ACP_CFG_DEBUG)
    ACE_DASSERT(aSnapshot->mMemArea    == aArea);
    ACE_DASSERT(aSnapshot->mChunkCount <= aArea->mChunkCount);

    aArea->mChunkCount = aArea->mChunkCount;
#endif

    if (sChunk == NULL)
    {
        aArea->mCurrentChunkListNode = aSnapshot->mCurrentChunkListNode->mNext;
    }
    else
    {
        ACE_DASSERT(sChunk->mChunkSize >= aSnapshot->mAllocSize);

        sChunk->mAllocSize = aSnapshot->mAllocSize;

        aArea->mCurrentChunkListNode = aSnapshot->mCurrentChunkListNode;
    }
}

/**
 * frees all memory blocks allocated from the memory area
 * @param aArea pointer to the memory area
 */
ACP_EXPORT void aclMemAreaFreeAll(acl_mem_area_t *aArea)
{
    aclMemAreaChunk *sChunk = NULL;

    aArea->mCurrentChunkListNode = aArea->mChunkList.mNext;

    sChunk = aArea->mCurrentChunkListNode->mObj;

    if (sChunk != NULL)
    {
#if defined(ACP_CFG_DEBUG)
        aArea->mChunkCount = 1;
#endif
        sChunk->mAllocSize = 0;
    }
    else
    {
#if defined(ACP_CFG_DEBUG)
        aArea->mChunkCount = 0;
#endif
    }
}

/**
 * reclaims the unused memory chunks to the system
 * @param aArea pointer to the memory area
 */
ACP_EXPORT void aclMemAreaShrink(acl_mem_area_t *aArea)
{
    aclMemAreaChunk *sChunk = NULL;
    acp_list_node_t *sIterator = NULL;
    acp_list_node_t *sNextNode = NULL;

    ACP_LIST_ITERATE_SAFE_FROM(&aArea->mChunkList,
                               aArea->mCurrentChunkListNode->mNext,
                               sIterator,
                               sNextNode)
    {
        sChunk = sIterator->mObj;

        aclMemAreaFreeChunk(aArea, sChunk);
    }
}

/*
 * gets statistics information of a mem area
 * @param aArea pointer to the memory area
 * @param aStat pointer to the memory area statistics
 */
ACP_EXPORT void aclMemAreaStat(acl_mem_area_t      *aArea,
                               acl_mem_area_stat_t *aStat)
{
    aclMemAreaChunk *sChunk = NULL;
    acp_list_node_t *sIterator = NULL;
    acp_bool_t       sIsAlloced = ACP_TRUE;

    aStat->mChunkCount = 0;
    aStat->mTotalSize  = 0;
    aStat->mAllocSize  = 0;

    ACP_LIST_ITERATE(&aArea->mChunkList, sIterator)
    {
        sChunk = sIterator->mObj;

        aStat->mChunkCount++;
        aStat->mTotalSize += sChunk->mChunkSize;

        if (sIsAlloced == ACP_TRUE)
        {
            aStat->mAllocSize += sChunk->mAllocSize;
        }
        else
        {
            /* do nothing */
        }

        if (sIterator == aArea->mCurrentChunkListNode)
        {
            sIsAlloced = ACP_FALSE;
        }
        else
        {
            /* do nothing */
        }
    }
}
