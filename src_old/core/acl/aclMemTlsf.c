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
 * $Id: aclMemTlsf.c 4955 2009-03-24 09:31:05Z djin $
 ******************************************************************************/
#include <acp.h>
#include <aclMem.h>
#include <acpMem.h>
#include <acpTest.h>
#include <acpAtomic.h>
#include <aclMemTlsf.h>


/* These function are defined in acpMemTlsfImpc.c
   and used only this file. So these are declared here */
ACP_EXPORT void *aclMemTlsfMallocImp(acp_size_t aSize,
                                     void *aPrimaryPool);
static void *aclMemTlsfMallocHuge(acp_size_t aSize,
                                     void *aPrimaryPool);
ACP_EXPORT void *aclMemTlsfReallocImp(void *aPtr,
                                      acp_size_t aNewSize,
                                      void *aPrimaryPool);
ACP_EXPORT void *aclMemTlsfCallocImp(acp_size_t aNelem,
                                     acp_size_t aElemSize,
                                     void *aPrimaryPool);
ACP_EXPORT void aclMemTlsfFreeImp(void *aPtr,
                                  void *aPrimaryPool);
static void aclMemTlsfFreeHuge(void *aPtr,
                                  void *aPrimaryPool);
ACP_EXPORT acp_size_t aclMemTlsfInitMemPool(acp_mmap_t *aMmapArea);
ACP_EXPORT acp_mmap_t *aclMemTlsfGetNewArea(acp_size_t *aSize);
ACP_EXPORT void aclMemTlsfMapSet(acp_size_t aSize,
                                 acp_sint32_t *aFl,
                                 acp_sint32_t *aSl);
ACP_EXPORT void aclMemTlsfExtractBlock(acl_mem_tlsf_bhdr_t *aBlock,
                                       acl_mem_tlsf_t *aTlsf,
                                       acp_sint32_t aFl,
                                       acp_sint32_t aSl);
ACP_EXPORT acl_mem_tlsf_bhdr_t *aclMemTlsfProcessArea(void *aArea, 
                                                      acp_size_t aSize);


/**
 * alloc of tlsf
 * @param aAllocator pointer to the tlsf instance
 * @param aAddr pointer to variable which will hold pointer to 
 * allocated memory block
 * @param aSize the size of memory in bytes to be allocated
 * @return result code
 * #ACP_RC_ENOMEM no memory allocated
 * #ACP_RC_SUCCESS success
 */
ACP_EXPORT  acp_rc_t aclMemAllocTlsf(acl_mem_alloc_t *aAllocator,
                                     void **aAddr,
                                     acp_size_t aSize)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                   aAllocator->mHandle;
    acp_size_t sPoolSize = sTlsf->mPoolSize;


    if (aAllocator -> mIsThreadSafe == ACP_TRUE)
    {
        acpSpinLockLock(&sTlsf->mLock);


        if (aSize >= (sPoolSize - ACL_TLSF_BHDR_OVRHD))
        {
            *aAddr = aclMemTlsfMallocHuge(aSize, sTlsf->mPrimaryArea);
        }
        else
        {
            *aAddr = aclMemTlsfMallocImp(aSize, sTlsf->mPrimaryArea);
        }
        
        acpSpinLockUnlock(&sTlsf->mLock);
    }
    else
    {
        if (aSize >= (sPoolSize - ACL_TLSF_BHDR_OVRHD))
        {
            *aAddr = aclMemTlsfMallocHuge(aSize, sTlsf->mPrimaryArea);
        }
        else
        {
            *aAddr = aclMemTlsfMallocImp(aSize, sTlsf->mPrimaryArea);
        }
    }

    if (*aAddr == NULL)
    {
        return ACP_RC_ENOMEM;
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

static void *aclMemTlsfMallocHuge(acp_size_t aSize, void *aPrimaryPool)
{
    acl_mem_tlsf_t *sTlsf = (acl_mem_tlsf_t *)aPrimaryPool;
    acp_size_t sAreaSize;
    acp_mmap_t *sMmapArea = NULL;
    acl_mem_tlsf_area_info_t *sAreaInfo = NULL;
    acl_mem_tlsf_bhdr_t *sInitBlock = NULL;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    acl_mem_tlsf_bhdr_t *sLastBlock = NULL;
        
    sAreaSize = aSize + ACL_TLSF_BHDR_OVRHD * 10;

    sMmapArea = aclMemTlsfGetNewArea(&sAreaSize);
    ACP_TEST(sMmapArea == NULL);

    sInitBlock = aclMemTlsfProcessArea(acpMmapGetAddress(sMmapArea),
                                       sAreaSize);
    
    sBlock = (acl_mem_tlsf_bhdr_t *) ACL_TLSF_NEXT_B(sInitBlock->mPtr.mBuffer, 
                                         sInitBlock->mSize & ACL_TLSF_B_SIZE);
    sLastBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer, 
                                  sBlock->mSize & ACL_TLSF_B_SIZE);

    /* Inserting the area in the list of linked areas */
    sAreaInfo = (acl_mem_tlsf_area_info_t *) sInitBlock->mPtr.mBuffer;
    sAreaInfo->mNext = sTlsf->mHugeAreaHead;
    sAreaInfo->mMapArea = sMmapArea;
    sAreaInfo->mEnd = sLastBlock;
    
    /* put new area into huge area list */
    sTlsf->mHugeAreaHead = sAreaInfo;

    /* set statistics info */
    sTlsf->mUsedSize += sAreaSize;
    return (void *)(sBlock->mPtr.mBuffer);

    ACP_EXCEPTION_END;

    return NULL;
}

/**
 * calloc of tlsf
 * @param aAllocator pointer to the tlsf instance
 * @param aAddr pointer to variable which will hold pointer 
 * to allocated memory block
 * @param aElements number of elements to be allocated 
 * @param aSize size of element in bytes
 * @return result code
 * #ACP_RC_ENOMEM no memory allocated
 * #ACP_RC_SUCCESS success
 */
ACP_EXPORT acp_rc_t aclMemCallocTlsf(acl_mem_alloc_t *aAllocator,
                                     void  **aAddr,
                                     acp_size_t   aElements,
                                     acp_size_t   aSize)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                   aAllocator->mHandle;
    acp_size_t sPoolSize = sTlsf->mPoolSize;

    /* calloc for huge memory is meaningless */
    if (aElements * aSize >= (sPoolSize - ACL_TLSF_BHDR_OVRHD))
    {
        *aAddr = aclMemTlsfMallocHuge(aElements * aSize, sTlsf->mPrimaryArea);
        ACP_TEST(*aAddr == NULL);
        acpMemSet(*aAddr, 0x00, aElements*aSize);
    }
    else
    {
        /* go ahead */

        if (aAllocator->mIsThreadSafe == ACP_TRUE)
        {
            acpSpinLockLock(&sTlsf->mLock);

            *aAddr = aclMemTlsfCallocImp(aElements, aSize, sTlsf->mPrimaryArea);

            acpSpinLockUnlock(&sTlsf->mLock);
        }
        else
        {
            *aAddr = aclMemTlsfCallocImp(aElements, aSize, sTlsf->mPrimaryArea);
        }
        ACP_TEST(*aAddr == NULL);
    }
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_ENOMEM;
}

/**
 * realloc of tlsf
 * @param aAllocator pointer to the tlsf instance
 * @param aAddr pointer to memory block to be reallocated
 * @param aSize size of memory block
 * @return result code
 * #ACP_RC_ENOMEM no memory allocated
 * #ACP_RC_SUCCESS success
 */
ACP_EXPORT acp_rc_t aclMemReallocTlsf(acl_mem_alloc_t *aAllocator,
                                      void **aAddr,
                                      acp_size_t aSize)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                  aAllocator->mHandle;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    acp_size_t sOldSize;
    void* sAddr = NULL;
    acp_rc_t sRC;

    if(*aAddr != NULL)
    {
        sBlock =   (acl_mem_tlsf_bhdr_t *)
            ((char *)*aAddr - ACL_TLSF_BHDR_OVRHD);
        sOldSize = sBlock->mSize;
    }
    else
    {
        sBlock = NULL;
        sOldSize = 0;
    }

    if(sOldSize < aSize)
    {
        sRC = aclMemAllocTlsf(aAllocator, &sAddr, aSize);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        if(*aAddr != NULL)
        {
            (void)acpMemCpy(sAddr, *aAddr, sOldSize);
            (void)aclMemFreeTlsf(aAllocator, *aAddr);
        }
    
        *aAddr = sAddr;
    }
    else
    {
        /* Do nothing */
    }
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * free of tlsf
 * @param aAllocator pointer to the tlsf instance
 * @param aAddr pointer to memory to be freed
 * @return always return #ACP_RC_SUCCESS
 */
ACP_EXPORT acp_rc_t aclMemFreeTlsf(acl_mem_alloc_t *aAllocator, void *aAddr)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                   aAllocator->mHandle;
    acp_size_t sPoolSize = sTlsf->mPoolSize;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    
    if (aAddr == NULL)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        sBlock = (acl_mem_tlsf_bhdr_t *)((char *)aAddr - ACL_TLSF_BHDR_OVRHD);
        
        if (aAllocator->mIsThreadSafe == ACP_TRUE)
        {
            acpSpinLockLock(&sTlsf->mLock);

            if (sBlock->mSize >= sPoolSize)
            {
                aclMemTlsfFreeHuge(aAddr, sTlsf->mPrimaryArea);
            }
            else
            {
                aclMemTlsfFreeImp(aAddr, sTlsf->mPrimaryArea);
            }

            acpSpinLockUnlock(&sTlsf->mLock);
        }
        else
        {
            if (sBlock->mSize >= sPoolSize)
            {
                aclMemTlsfFreeHuge(aAddr, sTlsf->mPrimaryArea);
            }
            else
            {
                aclMemTlsfFreeImp(aAddr, sTlsf->mPrimaryArea);
            }
        }
    }
    
    return ACP_RC_SUCCESS;
}

static void aclMemTlsfFreeHuge(void *aPtr, void *aPrimaryPool)
{
    acl_mem_tlsf_t *sTlsf = (acl_mem_tlsf_t *) aPrimaryPool;
    acl_mem_tlsf_area_info_t *sAreaInfo = NULL;
    acl_mem_tlsf_area_info_t *sCurr = NULL;
    acp_mmap_t *sMmapArea = NULL;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    
    if (aPtr == NULL)
    {
        return;
    }
    else
    {
        /* continue */
    }

    sBlock = (acl_mem_tlsf_bhdr_t *)((char *)aPtr - ACL_TLSF_BHDR_OVRHD);

    sAreaInfo = (acl_mem_tlsf_area_info_t *)
        ((char *)sBlock - ACL_TLSF_UP_SIZE(sizeof(acl_mem_tlsf_area_info_t)));

    sMmapArea = sAreaInfo->mMapArea;

    /* check area infomation correction */
    ACE_DASSERT(acpMmapGetAddress(sMmapArea) ==
                (void *)((char *)sAreaInfo - ACL_TLSF_BHDR_OVRHD));
    ACE_DASSERT(sBlock->mSize < acpMmapGetSize(sMmapArea));

    /* remove area from the list */
    if (sTlsf->mHugeAreaHead == sAreaInfo)
    {
        sTlsf->mHugeAreaHead = sTlsf->mHugeAreaHead->mNext;
    }
    else
    {
        sCurr = sTlsf->mHugeAreaHead;
        
        while (sCurr->mNext != sAreaInfo && sCurr->mNext != NULL)
        {
            sCurr = sCurr->mNext;
        }

        if (sCurr->mNext != NULL)
        {
            sCurr->mNext = sCurr->mNext->mNext;
        }
        else
        {
            /* do not exist in the list */
            ACE_DASSERT(0);
            return;
        }
    }

    /* set statistics info */
    sTlsf->mUsedSize -= acpMmapGetSize(sMmapArea);
    (void)acpMmapDetach(sMmapArea);
    acpMemFree(sMmapArea);
}



/**
 * get statistic information of tlsf
 * @param aAllocator pointer to the tlsf instance
 * @param aStat pointer to the tlsf specific structure to hold 
 * statistic data
 * @return always return #ACP_RC_SUCCESS
 */
ACP_EXPORT acp_rc_t aclMemStatisticTlsf(acl_mem_alloc_t *aAllocator,
                                        void *aStat)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                   aAllocator->mHandle;
    acl_mem_tlsf_stat_t  *sStat = (acl_mem_tlsf_stat_t *) aStat;

    if (aAllocator->mIsThreadSafe == ACP_TRUE)
    {
        acpSpinLockLock(&sTlsf->mLock);

        sStat->mPoolSize = ((acl_mem_tlsf_t *) sTlsf->mPrimaryArea)->mPoolSize;
        sStat->mUsedSize = ((acl_mem_tlsf_t *) sTlsf->mPrimaryArea)->mUsedSize;

        acpSpinLockUnlock(&sTlsf->mLock);
    }
    else
    {
        sStat->mPoolSize = ((acl_mem_tlsf_t *) sTlsf->mPrimaryArea)->mPoolSize;
        sStat->mUsedSize = ((acl_mem_tlsf_t *) sTlsf->mPrimaryArea)->mUsedSize;
    }

    return ACP_RC_SUCCESS;
}

/**
 * initialze tlsf allocator
 * @param aAllocator tlsf instance to be initialized
 * @param aArgs pointer to tlsf specific init params, for example area size
 * @return result code
 * #ACP_RC_SUCCESS success to initialize
 * #ACP_RC_ENOMEM fail to initialize becauseof lack of system memory
 */
ACP_EXPORT acp_rc_t aclMemInitTlsf(acl_mem_alloc_t *aAllocator, void *aArgs)
{
    acp_rc_t   sRC;
    acp_size_t sSize = 0;

    acp_size_t sAreaSize  = ACL_TLSF_DEFAULT_AREA_SIZE; 
    acp_mmap_t *sMmapArea = NULL;

    acl_mem_alloc_tlsf_t *sTlsf = NULL;
    acl_mem_tlsf_init_t  *sInit = (acl_mem_tlsf_init_t *)aArgs;

    if (sInit != NULL)
    {
        sAreaSize = sInit->mPoolSize;
    }
    else
    {
        /* Use default init params */
    }

    sAreaSize = (sAreaSize > ACL_TLSF_DEFAULT_AREA_SIZE) ?
        sAreaSize : ACL_TLSF_DEFAULT_AREA_SIZE;

    sRC = acpMemAlloc((void **)&sTlsf,
                       sizeof(acl_mem_alloc_tlsf_t));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_BAD_MEM);
    sTlsf->mPoolSize   = sAreaSize;

    /* Mmap memory area */
    sMmapArea = aclMemTlsfGetNewArea(&sAreaSize);
    ACP_TEST_RAISE(sMmapArea == NULL, E_BAD_AREA);

    sTlsf->mPrimaryArea = acpMmapGetAddress(sMmapArea);

    sSize = aclMemTlsfInitMemPool(sMmapArea);
    ACP_TEST_RAISE(sSize == 0, E_BAD_AREA);

    aAllocator->mHandle     = sTlsf;

    acpSpinLockInit(&sTlsf->mLock, -1);

#if defined(__STATIC_ANALYSIS_DOING__)
    /*
     * Workaround for code sonar
     * This pointer will be freed when allocator instance 
     * is destroyed in aclMemFinitTlsf function
     */
    acpMemFree(sMmapArea);
#endif

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_BAD_MEM)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_BAD_AREA)
    {
        acpMemFree(sTlsf);
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

/**
 * finalize tlsf instance.
 * This function does not care the instance has busy memory.
 * All memory included in the instance will be freed.
 * @param aAllocator tlsf instance
 * @return system error code
 * #ACP_RC_SUCCESS success
 */
ACP_EXPORT acp_rc_t aclMemFinitTlsf(acl_mem_alloc_t *aAllocator)
{
    acp_rc_t    sRC     = ACP_RC_SUCCESS;
    void       *sTmpPtr = NULL;
    acp_mmap_t *sArea   = NULL;
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                  aAllocator->mHandle;
    acl_mem_tlsf_t *sTlsfPtr = (acl_mem_tlsf_t *) sTlsf -> mPrimaryArea;
    acl_mem_tlsf_area_info_t *sNext = sTlsfPtr->mAreaHead;

    /* Unmap all mmaped memory areas */
    while(sNext)
    {
        sArea = sNext->mMapArea;

        sTmpPtr = sNext->mNext;

        /* Unmap mmaped area */
        sRC = acpMmapDetach(sArea);

        acpMemFree(sArea);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            /* Continue */
        }
        else
        {
            break;
        }

        sNext = sTmpPtr;
    }

    acpMemFree(aAllocator->mHandle);

    return sRC;
}

/* check if the area is empty */
/* travel all blocks in the area and
   return true if all blocks are free except initial block and sentinel block
   because initial-block and sentinel-block are always busy. */
static acp_bool_t aclMemTlsfIsEmptyArea(acl_mem_tlsf_area_info_t *aArea)
{
    acl_mem_tlsf_bhdr_t *sInitBlock = NULL;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    acp_bool_t sResult = ACP_TRUE;

    sInitBlock = (acl_mem_tlsf_bhdr_t *)((char *)aArea - ACL_TLSF_BHDR_OVRHD);
    sBlock = ACL_TLSF_NEXT_B(sInitBlock->mPtr.mBuffer,
                             sInitBlock->mSize & ACL_TLSF_B_SIZE);

    while (sBlock != aArea->mEnd)
    {
        if ((sBlock->mSize & ACL_TLSF_B_STATE) == ACL_TLSF_FREE_B)
        {
            /* find free block and go next block */
        }
        else
        {
            sResult = ACP_FALSE;
            break;
        }

        sBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer,
                                 sBlock->mSize & ACL_TLSF_B_SIZE);
    }

    return sResult;
}

/* Remove blocks of area from free block list */
/* and unmap entire area */
static acp_rc_t aclMemTlsfDelArea(acl_mem_tlsf_t *aTlsfPtr,
                                      acl_mem_tlsf_area_info_t *aArea,
                                      acl_mem_tlsf_area_info_t *aPrevArea)
{
    acp_sint32_t sFl;
    acp_sint32_t sSl;
    acl_mem_tlsf_bhdr_t *sInitBlock = NULL;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    acp_rc_t sRC;
    acp_mmap_t *sMmapPtr = NULL;

    /* remove blocks from free list */
    sInitBlock = (acl_mem_tlsf_bhdr_t *)((char *)aArea - ACL_TLSF_BHDR_OVRHD);
    
    sBlock = ACL_TLSF_NEXT_B(sInitBlock->mPtr.mBuffer,
                             sInitBlock->mSize & ACL_TLSF_B_SIZE);

    while (sBlock != aArea->mEnd)
    {
        /* All blocks must be free in this area */
        ACP_TEST_RAISE((sBlock->mSize & ACL_TLSF_B_STATE) != ACL_TLSF_FREE_B,
                       E_NOT_FREE_BLOCK_EXIST);
                       
        aclMemTlsfMapSet(sBlock->mSize & ACL_TLSF_B_SIZE, &sFl, &sSl);
        aclMemTlsfExtractBlock(sBlock, aTlsfPtr, sFl, sSl);

        sBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer,
                                 sBlock->mSize & ACL_TLSF_B_SIZE);
    }

    /* remove area from area list */
    aPrevArea->mNext = aArea->mNext;

    /* decrease mPoolSize */
    aTlsfPtr->mPoolSize -= acpMmapGetSize(aArea->mMapArea);

    /* must copy the pointer of mmap data structure
       because aArea will be removed and
       aArea->mMapArea pointer will be not accessible
    */
    sMmapPtr = aArea->mMapArea;
    sRC = acpMmapDetach(sMmapPtr);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    
    acpMemFree(sMmapPtr);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NOT_FREE_BLOCK_EXIST)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

static acp_rc_t aclMemTlsfShrinkBody(acl_mem_alloc_t *aAllocator,
                                     acp_sint32_t *aCount)
{
    acp_rc_t sRC = ACP_RC_ENOENT;
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                  aAllocator->mHandle;
    acl_mem_tlsf_t *sTlsfPtr = (acl_mem_tlsf_t *) sTlsf -> mPrimaryArea;
    acl_mem_tlsf_area_info_t *sCurr = NULL;
    acl_mem_tlsf_area_info_t *sPrev = NULL;
    acl_mem_tlsf_area_info_t *sNext = NULL;
    acp_sint32_t sCount = 0;

    ACP_TEST_RAISE(sTlsfPtr->mAreaHead->mNext == NULL, E_ONLY_ONE_AREA);
    
    /* do not shrink the first area because it is the recently used area.
     * The last used area probably has cached-data/address.
     */
    sCurr = sTlsfPtr->mAreaHead->mNext;
    sPrev = sTlsfPtr->mAreaHead;
    sNext = sCurr->mNext;

    /* The primary area which has tlsf structure are the last area in the list.
       It cannot be deleted */
    while(sCurr->mNext != NULL)
    {
        sNext = sCurr->mNext;

        if (aclMemTlsfIsEmptyArea(sCurr) == ACP_TRUE)
        {
            /* If current area is deleted,
               sCurr pointer cannot be access and
               prev area is remained as prev again. */
            sRC = aclMemTlsfDelArea(sTlsfPtr, sCurr, sPrev);
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

            sRC = ACP_RC_SUCCESS;
            sCount++;
        }
        else
        {
            /* Current area is no deleted,
               current area becomes prev area */
            sPrev = sCurr;
        }

        sCurr = sNext;
    }

    if (aCount != NULL)
    {
        *aCount = sCount;
    }
    else
    {
        /* do nothing */
    }

    return sRC;

    ACP_EXCEPTION(E_ONLY_ONE_AREA)
    {
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION_END;

    return sRC;
}

/**
 * try to shrink memory allocator occupy
 * Shrinking happens only if the allocator has at least 3 areas.
 * The first area in the area list of allocator is not shrinked
 * because it is the recently used area.
 * The last area is the primary area that has the extra information
 * in the head of area, so it cannot be shrinked.
 * Therefore other areas in the middle of the list are candidates.
 * @param aAllocator allocator instance
 * @return #ACP_RC_SUCCESS some areas are shrinked
 * #ACP_RC_ENOENT no area can be shrinked
 * #ACP_RC_EINVAL an busy block found in empty area
 */
ACP_EXPORT acp_rc_t aclMemTlsfShrink(acl_mem_alloc_t *aAllocator,
                                     acp_sint32_t *aCount)
{
    acp_rc_t sRC;
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                   aAllocator->mHandle;
    
    if (aAllocator->mIsThreadSafe == ACP_TRUE)
    {
        acpSpinLockLock(&sTlsf->mLock);

        sRC = aclMemTlsfShrinkBody(aAllocator, aCount);

        acpSpinLockUnlock(&sTlsf->mLock);
    }
    else
    {
        sRC = aclMemTlsfShrinkBody(aAllocator, aCount);
    }

    return sRC;
}


/* check allocator has only no busy memory */
static acp_bool_t aclMemIsEmptyTlsf(acl_mem_alloc_t *aAllocator)
{
    acp_bool_t sResult = ACP_TRUE;
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                  aAllocator->mHandle;
    acl_mem_tlsf_t *sTlsfPtr = (acl_mem_tlsf_t *) sTlsf -> mPrimaryArea;
    acl_mem_tlsf_area_info_t *sCurr = sTlsfPtr->mAreaHead;
    
    /* scan all areas */
    while(sCurr)
    {
        if (aclMemTlsfIsEmptyArea(sCurr) == ACP_TRUE)
        {
            /* go next area */
        }
        else
        {
            sResult = ACP_FALSE;
            break;
        }

        sCurr = sCurr->mNext;
    }

    return sResult;
}

/**
 * control tlsf attribute or behavior
 * @param aAllocator allocator instance
 * @param aAttrName attribute name or control command
 * @param aArg control data input or output according to the command
 * @return #ACP_RC_EINVAL unidentified command
 * The return value is determined by each command
 */
ACP_EXPORT acp_rc_t aclMemControlTlsf(acl_mem_alloc_t      *aAllocator,
                                      acl_mem_alloc_attr_t  aAttrName,
                                      void                 *aArg)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    
    switch (aAttrName)
    {
        case ACL_MEM_IS_FULL:
            /* Because tlsf can make pool again, instance cannot be full */
            *(acp_bool_t *)aArg = ACP_FALSE;
            break;
        case ACL_MEM_IS_EMPTY:
            *(acp_bool_t *)aArg = aclMemIsEmptyTlsf(aAllocator);;
            break;
        case ACL_MEM_SET_SPIN_COUNT:
            acpSpinLockSetCount(
                &(((acl_mem_alloc_tlsf_t *)aAllocator->mHandle)->mLock),
                *(acp_sint32_t*)aArg);
            break;
        case ACL_MEM_GET_SPIN_COUNT:
            *(acp_sint32_t*)aArg = acpSpinLockGetCount(
                &(((acl_mem_alloc_tlsf_t *)aAllocator->mHandle)->mLock));
            break;
        case ACL_MEM_SHRINK:
            sRC = aclMemTlsfShrink(aAllocator, (acp_sint32_t *)aArg);
            break;
        default:
            sRC = ACP_RC_EINVAL;
    }
    return sRC;
}

#if defined(ACP_CFG_DEBUG) || defined (ACP_CFG_DOXYGEN)

static void aclMemTlsfPrintBlock(acl_mem_tlsf_bhdr_t *aBlock)
{
    if (aBlock == NULL)
    {
        return;
    }
    else
    {
        (void)acpPrintf(">> [%p] (", aBlock);
        if (aBlock->mSize & ACL_TLSF_B_SIZE)
        {
            (void)acpPrintf("%lu bytes, ",
                            (acp_ulong_t)(aBlock->mSize & ACL_TLSF_B_SIZE));
        }
        else
        {
            (void)acpPrintf("sentinel, ");
        }

        if ((aBlock->mSize & ACL_TLSF_B_STATE) == ACL_TLSF_FREE_B)
        {
            (void)acpPrintf("free [%p, %p], ", aBlock->mPtr.mFreePtr.mPrev,
                            aBlock->mPtr.mFreePtr.mNext);
        }
        else
        {
            (void)acpPrintf("used, ");
        }

        if ((aBlock->mSize & ACL_TLSF_PREV_STATE) == ACL_TLSF_PREV_FREE)
        {
            (void)acpPrintf("prev. free [%p])\n", aBlock->mPrevHdr);
        }
        else
        {
            (void)acpPrintf("prev used)\n");
        }
    }
}


/**
 * Print all blocks in each area
 * This function does not access matrix info in tlsf structure.
 * Even if matrix has error, this function can print all blocks.
 */
ACP_EXPORT void aclMemTlsfPrintAllBlocks(acl_mem_alloc_t *aAllocator)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                  aAllocator->mHandle;
    acl_mem_tlsf_t *sTlsfPtr = (acl_mem_tlsf_t *) sTlsf -> mPrimaryArea;
    acl_mem_tlsf_area_info_t *sAreaInfo = sTlsfPtr->mAreaHead;
    acl_mem_tlsf_bhdr_t *sNext = NULL;

    (void)acpPrintf("\nTLSF at %p\nALL BLOCKS\n\n", sTlsfPtr);

    while (sAreaInfo)
    {
        sNext = (acl_mem_tlsf_bhdr_t *)((char *)sAreaInfo -
                                        ACL_TLSF_BHDR_OVRHD);

        while (sNext)
        {
            aclMemTlsfPrintBlock(sNext);
            if (sNext->mSize & ACL_TLSF_B_SIZE)
            {
                sNext = ACL_TLSF_NEXT_B(sNext->mPtr.mBuffer,
                                        sNext->mSize & ACL_TLSF_B_SIZE);
            }
            else
            {
                sNext = NULL;
            }
        }
        (void)acpPrintf("\n");
        sAreaInfo = sAreaInfo->mNext;
    }
}


/**
 * Print matrix info in tlsf structure
 *
 */
ACP_EXPORT void aclMemTlsfPrintMatrix(acl_mem_alloc_t *aAllocator)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                  aAllocator->mHandle;
    acl_mem_tlsf_t *sTlsfPtr = (acl_mem_tlsf_t *) sTlsf -> mPrimaryArea;
    acp_sint32_t i;
    acp_sint32_t j;

    for (i = 0; i < ACL_TLSF_REAL_FLI; i++)
    {
        for (j = 0; j < ACL_TLSF_MAX_SLI; j++)
        {
                (void)acpPrintf("%p  ", sTlsfPtr->mMatrix[i][j]);
        }
        (void)acpPrintf("\n");
    }
}


/**
 * Print all blocks in matrix
 * This function travels in free list of each cell of matrix.
 * If matrix has error, this function occurs error.
 */
ACP_EXPORT void aclMemTlsfPrintTlsf(acl_mem_alloc_t *aAllocator)
{
    acl_mem_alloc_tlsf_t *sTlsf = (acl_mem_alloc_tlsf_t *)
                                  aAllocator->mHandle;
    acl_mem_tlsf_t *sTlsfPtr = (acl_mem_tlsf_t *) sTlsf -> mPrimaryArea;
    acl_mem_tlsf_bhdr_t *sNext = NULL;
    acp_sint32_t i;
    acp_sint32_t j;

    (void)acpPrintf("\nTLSF at %p\n", sTlsfPtr);
    (void)acpPrintf("FL bitmap:0x%X\n\n", sTlsfPtr->mFlBitmap);

    for (i = 0; i < ACL_TLSF_REAL_FLI; i++)
    {
        if (sTlsfPtr->mSlBitmap[i])
        {
            (void)acpPrintf("SL bitmap 0x%X\n", sTlsfPtr->mSlBitmap[i]);
        }
        else
        {
        }

        for (j = 0; j < ACL_TLSF_MAX_SLI; j++)
        {
            sNext = sTlsfPtr->mMatrix[i][j];

            if (sNext)
            {
                (void)acpPrintf("-> [%d][%d]\n", i, j);
            }
            else
            {
            }

            while (sNext)
            {
                aclMemTlsfPrintBlock(sNext);
                sNext = sNext->mPtr.mFreePtr.mNext;
            }
        }
    }
}

#endif
