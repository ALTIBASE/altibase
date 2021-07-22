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
 * $Id: aclMemTlsfImp.c 4955 2009-03-24 09:31:05Z djin $
 ******************************************************************************/
#include <acp.h>
#include <aclMem.h>
#include <acpMem.h>
#include <acpTest.h>
#include <acpAtomic.h>
#include <aclMemTlsf.h>

    
static const acp_sint32_t gTable[] = {
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
    4, 4,
    4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5,
    5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6,
    6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6,
    6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7
};

/*
 * Function gets least significant bit
 */
ACP_INLINE acp_sint32_t aclMemTlsfLSBit(acp_sint32_t aVal)
{
    acp_uint32_t sA;
    acp_uint32_t sX = aVal & -aVal;

    sA = sX <= 0xffff ? (sX <= 0xff ? 0 : 8) : (sX <= 0xffffff ? 16 : 24);
    return gTable[sX >> sA] + sA;
}

/*
 * Function gets most significant bit
 */
ACP_INLINE acp_sint32_t aclMemTlsfMSBit(acp_sint32_t aVal)
{
    acp_uint32_t sA;
    acp_uint32_t sX = (acp_uint32_t)aVal;

    sA = sX <= 0xffff ? (sX <= 0xff ? 0 : 8) : (sX <= 0xffffff ? 16 : 24);
    return gTable[sX >> sA] + sA;
}

/*
 * Function sets bit in address
 */
ACP_INLINE void aclMemTlsfSetBit(acp_sint32_t aNr, acp_uint32_t *aAddr)
{
    acp_uint32_t sNr = (acp_uint32_t)aNr;

    aAddr[sNr >> 5] |= 1 << (sNr & 0x1f);
}

/*
 * Function clears bit in address
 */
ACP_INLINE void aclMemTlsfClearBit(acp_sint32_t aNr, acp_uint32_t *aAddr)
{
    acp_uint32_t sNr = (acp_uint32_t)aNr;

    aAddr[sNr >> 5] &= ~(1 << (sNr & 0x1f));
}

/*
 * Function searches Sl and Fl indexes according to size
 */
ACP_INLINE void aclMemTlsfMappingSearch(acp_size_t *aSize,
                                        acp_sint32_t *aFl,
                                        acp_sint32_t *aSl)
{
    acp_sint32_t sT;

    if (*aSize < ACL_TLSF_SMALL_B)
    {
        *aFl = 0;
        *aSl = (acp_sint32_t)
               (*aSize / (ACL_TLSF_SMALL_B / ACL_TLSF_MAX_SLI));
    }
    else
    {
        sT = (1 << (acp_uint32_t)(aclMemTlsfMSBit((acp_sint32_t)(*aSize))
                      - ACL_TLSF_MAX_LOG2_SLI)) - 1;
        *aSize = *aSize + sT;
        *aFl = aclMemTlsfMSBit((acp_sint32_t)(*aSize));
        *aSl = (acp_sint32_t)
               ((*aSize >> (acp_uint32_t)(*aFl - ACL_TLSF_MAX_LOG2_SLI))
                     - ACL_TLSF_MAX_SLI);
        *aFl -= ACL_TLSF_FLI_OFFSET;
        *aSize &= ~sT;
    }
}

/*
 * Function sets Sl and Fl indexes according to size
 */
ACP_EXPORT void aclMemTlsfMapSet(acp_size_t aSize,
                                 acp_sint32_t *aFl,
                                 acp_sint32_t *aSl)
{
    if (aSize < ACL_TLSF_SMALL_B)
    {
        *aFl = 0;
        *aSl = (acp_sint32_t)
               (aSize / (ACL_TLSF_SMALL_B / ACL_TLSF_MAX_SLI));
    }
    else
    {
        *aFl = aclMemTlsfMSBit((acp_sint32_t)aSize);
        *aSl = (acp_sint32_t)
               ((aSize >> (acp_uint32_t)(*aFl - ACL_TLSF_MAX_LOG2_SLI))
                      - ACL_TLSF_MAX_SLI);

        *aFl -= ACL_TLSF_FLI_OFFSET;
    }
}

/*
 * Function finds memory block
 */
ACP_INLINE acl_mem_tlsf_bhdr_t *aclMemTlsfFindBlock(acl_mem_tlsf_t *aTlsf,
                                                    acp_sint32_t *aFl,
                                                    acp_sint32_t *aSl)
{
    acp_uint32_t sTmp = aTlsf->mSlBitmap[*aFl] &
             ((acp_uint32_t)~0 << (acp_uint32_t)(*aSl));

    acl_mem_tlsf_bhdr_t *sBlock = NULL;

    if (sTmp != 0)
    {
        *aSl = aclMemTlsfLSBit(sTmp);
        sBlock = aTlsf->mMatrix[*aFl][*aSl];
    }
    else
    {
        *aFl = aclMemTlsfLSBit(aTlsf->mFlBitmap & 
                  ((acp_uint32_t)~0 << (acp_uint32_t)(*aFl + 1)));

        if (*aFl > 0)
        {
            *aSl = aclMemTlsfLSBit(aTlsf->mSlBitmap[*aFl]);
            sBlock = aTlsf->mMatrix[*aFl][*aSl];
        }
        else
        {
            /* do nothing */
        }
    }

    return sBlock;
}

/*
 * Function gets header block (the first block) from block list
 * aBlock must be the first block of the block list
 * that is found by aclMemTlsfFindBlock.
 */
ACP_INLINE void aclMemTlsfExtractBlockHdr(acl_mem_tlsf_bhdr_t *aBlock, 
                                          acl_mem_tlsf_t *aTlsf, 
                                          acp_sint32_t aFl, 
                                          acp_sint32_t aSl)
{
    aTlsf -> mMatrix [aFl] [aSl] = aBlock -> mPtr.mFreePtr.mNext;

    if (aTlsf -> mMatrix[aFl][aSl] != NULL)
    {
        aTlsf -> mMatrix[aFl][aSl] -> mPtr.mFreePtr.mPrev = NULL;
    }
    else
    {
        aclMemTlsfClearBit (aSl, &aTlsf -> mSlBitmap [aFl]);

        if (aTlsf -> mSlBitmap[aFl] == 0)
        {
            aclMemTlsfClearBit (aFl, &aTlsf -> mFlBitmap);
        }
        else
        {
        }
    }
    aBlock -> mPtr.mFreePtr.mPrev =  NULL;
    aBlock -> mPtr.mFreePtr.mNext =  NULL;
}

/*
 * Function extract arbitrary block from list of free block list
 * aBlock can be any block in the free block list.
 */
ACP_EXPORT void aclMemTlsfExtractBlock(acl_mem_tlsf_bhdr_t *aBlock,
                                       acl_mem_tlsf_t *aTlsf,
                                       acp_sint32_t aFl,
                                       acp_sint32_t aSl)
{
    if (aBlock -> mPtr.mFreePtr.mNext != NULL)
    {
        aBlock -> mPtr.mFreePtr.mNext->mPtr.mFreePtr.mPrev =
                       aBlock->mPtr.mFreePtr.mPrev;
    }
    else
    {
    }

    if (aBlock -> mPtr.mFreePtr.mPrev != NULL)
    {
        aBlock -> mPtr.mFreePtr.mPrev->mPtr.mFreePtr.mNext =
                      aBlock->mPtr.mFreePtr.mNext;
    }
    else
    {
    }

    if (aTlsf -> mMatrix [aFl][aSl] == aBlock)
    {
        aTlsf -> mMatrix [aFl][aSl] = aBlock -> mPtr.mFreePtr.mNext;

        if (aTlsf -> mMatrix [aFl][aSl] == NULL)
        {
            aclMemTlsfClearBit (aSl, &aTlsf -> mSlBitmap[aFl]);

            if (aTlsf->mSlBitmap[aFl] == 0)
            {
                aclMemTlsfClearBit(aFl, &aTlsf -> mFlBitmap);
            }
            else
            {
            }
        }
        else
        {
        }
    }
    else
    {
    }

    aBlock -> mPtr.mFreePtr.mPrev = NULL;
    aBlock -> mPtr.mFreePtr.mNext = NULL;
}

/*
 * Refer BUG-32954
 * An wrong pointer can be passed into tlsf matrix.
 * If the pointer is in the user space it can be accepted
 * becuase block address is passed as void pointer.
 * Only aclMemTlsfInsertBlock can insert block into tlsf matrix
 * so that checking function is added into aclMemTlsfInsertBlock.
 */
static acp_bool_t aclMemTlsfCheckBlockPointer(acl_mem_tlsf_t *aTlsf,
                                       acl_mem_tlsf_bhdr_t *aBlock)
{
    acl_mem_tlsf_area_info_t *sCurArea = aTlsf->mAreaHead;
    acp_ulong_t sAreaSize;
    acp_ulong_t sStartAddr;
    acp_ulong_t sBlockPtr = (acp_ulong_t)aBlock;

    while(sCurArea != NULL)
    {
        sStartAddr = (acp_ulong_t)acpMmapGetAddress(sCurArea->mMapArea);
        sAreaSize = (acp_ulong_t)acpMmapGetSize(sCurArea->mMapArea);

        if (sBlockPtr > sStartAddr && sBlockPtr < sStartAddr+sAreaSize)
        {
            return ACP_TRUE;
        }
        else
        {
            sCurArea = sCurArea->mNext;
        }
    }

    return ACP_FALSE;
}


/*
 * Function inserts memory block into list of free blocks
 */
ACP_INLINE void aclMemTlsfInsertBlock(acl_mem_tlsf_bhdr_t *aBlock,
                                      acl_mem_tlsf_t *aTlsf,
                                      acp_sint32_t aFl,
                                      acp_sint32_t aSl)
{

    ACE_DASSERT(aclMemTlsfCheckBlockPointer(aTlsf, aBlock) == ACP_TRUE);
        
    aBlock -> mPtr.mFreePtr.mPrev = NULL;
    aBlock -> mPtr.mFreePtr.mNext = aTlsf -> mMatrix [aFl][aSl];

    if (aTlsf -> mMatrix [aFl][aSl] != NULL)
    {
        aTlsf -> mMatrix [aFl][aSl] -> mPtr.mFreePtr.mPrev = aBlock;
    }
    else
    {
    }

    aTlsf -> mMatrix [aFl][aSl] = aBlock;
    aclMemTlsfSetBit (aSl, &aTlsf -> mSlBitmap [aFl]);
    aclMemTlsfSetBit (aFl, &aTlsf -> mFlBitmap);
}

/*
 * Function gets memory area from OS
 */
ACP_EXPORT acp_mmap_t* aclMemTlsfGetNewArea(acp_size_t *aSize)
{
    acp_mmap_t  *sMmap = NULL;
    acp_rc_t    sRC;
    acp_size_t  sPageSize;

    sRC = acpSysGetPageSize(&sPageSize);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    *aSize = ACL_TLSF_ROUNDUP(*aSize, sPageSize);

    sRC = acpMemAlloc((void **)&sMmap,
                      sizeof(acp_mmap_t));
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    sRC = acpMmap(sMmap,
                  (acp_size_t)(*aSize),
                  ACP_MMAP_READ | ACP_MMAP_WRITE,
                  ACP_MMAP_PRIVATE);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_BAD_MMAP);

    return sMmap;

    ACP_EXCEPTION(E_BAD_MMAP)
    {
        acpMemFree(sMmap);
    }

    ACP_EXCEPTION_END;

    return NULL;
}

/*
 * Function processes memory area
 */
ACP_EXPORT acl_mem_tlsf_bhdr_t *aclMemTlsfProcessArea(void *aArea, 
                                                      acp_size_t aSize)
{
    acl_mem_tlsf_bhdr_t      *sBlock = NULL;
    acl_mem_tlsf_bhdr_t      *sLastBlock = NULL;
    acl_mem_tlsf_bhdr_t      *sInitBlock = NULL;
    acl_mem_tlsf_area_info_t *sAreaInfo = NULL;

    sInitBlock = (acl_mem_tlsf_bhdr_t *) aArea;

    if (sizeof(acl_mem_tlsf_area_info_t) < ACL_TLSF_MIN_B_SIZE)
    {
        sInitBlock->mSize = ACL_TLSF_MIN_B_SIZE;
    }
    else
    {
        sInitBlock->mSize =
            ACL_TLSF_UP_SIZE(sizeof(acl_mem_tlsf_area_info_t)) | 
            ACL_TLSF_USED_B | ACL_TLSF_PREV_USED;
    }

    sBlock = (acl_mem_tlsf_bhdr_t *) ACL_TLSF_NEXT_B(sInitBlock->mPtr.mBuffer, 
                                         sInitBlock->mSize & ACL_TLSF_B_SIZE);

    sBlock->mSize = ACL_TLSF_DOWN_SIZE(aSize - 3 * ACL_TLSF_BHDR_OVRHD - 
                           (sInitBlock->mSize & ACL_TLSF_B_SIZE))
        | ACL_TLSF_USED_B | ACL_TLSF_PREV_USED;

    sBlock->mPtr.mFreePtr.mPrev = sBlock->mPtr.mFreePtr.mNext = 0;

    sLastBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer,
            sBlock->mSize & ACL_TLSF_B_SIZE);

    sLastBlock->mPrevHdr = sBlock;

    sLastBlock->mSize = 0 | ACL_TLSF_USED_B | ACL_TLSF_PREV_FREE;

    sAreaInfo = (acl_mem_tlsf_area_info_t *) sInitBlock->mPtr.mBuffer;

    sAreaInfo->mNext = 0;

    sAreaInfo->mEnd = sLastBlock;

    return sInitBlock;
}

ACP_EXPORT void aclMemTlsfFreeImp(void *aPtr, void *aPrimaryPool);

/*
 * Function inits memory area
 */
ACP_EXPORT acp_size_t aclMemTlsfInitMemPool(acp_mmap_t *aMmapArea)
{
    acl_mem_tlsf_t *sTlsf = NULL;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    acl_mem_tlsf_bhdr_t *sInitBlock = NULL;

    acp_size_t sMemPoolSize = acpMmapGetSize(aMmapArea);
    void *sMemPool = acpMmapGetAddress(aMmapArea);

    if (sMemPool == NULL ||
        sMemPoolSize == 0 ||
        sMemPoolSize < sizeof(acl_mem_tlsf_t) + ACL_TLSF_BHDR_OVRHD * 8)
    {
        return 0;
    }
    else
    {
        /* continue */
    }

    if (((acp_ulong_t) sMemPool & ACL_TLSF_PTR_MASK))
    {
        return 0;
    }
    else
    {
    }

    sTlsf = (acl_mem_tlsf_t *) sMemPool;

    /* Check if already initialised */
    if (sTlsf->mTlsfSignature == ACL_TLSF_SIGNATURE)
    {

        sBlock = ACL_TLSF_NEXT_B(sMemPool,
                 ACL_TLSF_UP_SIZE(sizeof(acl_mem_tlsf_t)));

        return sBlock->mSize & ACL_TLSF_B_SIZE;
    }
    else
    {
        /* continue */
    }

#if 0 /* PRJ-2152 */    
    /* Zeroing the memory pool */
    acpMemSet(sMemPool, 0, sizeof(acl_mem_tlsf_t));
#endif
    
    sTlsf->mTlsfSignature = ACL_TLSF_SIGNATURE;

    sInitBlock = aclMemTlsfProcessArea(ACL_TLSF_NEXT_B
                 (sMemPool, ACL_TLSF_UP_SIZE(sizeof(acl_mem_tlsf_t))), 
                  ACL_TLSF_DOWN_SIZE(sMemPoolSize - sizeof(acl_mem_tlsf_t)));

#if defined(__STATIC_ANALYSIS_DOING__)
    /*
     * Workaround for code sonar warning:
     * Null pointer dereference of sInitBlock and sInitBlock->mPtr.mBuffer
     */
    if (sInitBlock == NULL || sInitBlock->mPtr.mBuffer == NULL)
    {
        return 0;
    }
    else
    {
    }
#endif

    sBlock = ACL_TLSF_NEXT_B(sInitBlock->mPtr.mBuffer, 
                             sInitBlock->mSize & ACL_TLSF_B_SIZE);

#if defined(__STATIC_ANALYSIS_DOING__)
    /*
     * Workaround for code sonar warning:
     * Null pointer dereference of sBlock and sBlock->mPtr.mBuffer
     */
    if (sBlock == NULL || sBlock->mPtr.mBuffer == NULL)
    {
        return 0;
    }
    else
    {
    }
#endif

    sTlsf->mAreaHead = (acl_mem_tlsf_area_info_t *) sInitBlock->mPtr.mBuffer;
    sTlsf->mAreaHead->mMapArea = aMmapArea;

    /* attach sBlock into matrix */
    aclMemTlsfFreeImp(sBlock->mPtr.mBuffer, sTlsf);

    /* Initial allocator has only one area in the pool */
    /* sBlock->mSize is initlaized at aclMemTlsfFreeImp
       so these code must be below than aclMemTlsfFreeImp */
    sTlsf->mUsedSize = sMemPoolSize - (sBlock->mSize & ACL_TLSF_B_SIZE);
    sTlsf->mPoolSize = sMemPoolSize;
    sTlsf->mAreaSize = sMemPoolSize;

    /* proj-2152: huge size (>=1G) memory request */
    sTlsf->mHugeAreaHead = NULL;

    return (sBlock->mSize & ACL_TLSF_B_SIZE);
}

/*
 * Function adds new memory area
 */
acp_size_t aclMemTlsfAddNewArea(acp_mmap_t *aMmapArea,
                                void *aPrimaryPool)
{
    acl_mem_tlsf_t *sTlsf = (acl_mem_tlsf_t *) aPrimaryPool;

    /* travel areas */ 
#if 0 /* PRJ-2152 */   
    acl_mem_tlsf_area_info_t *sPtr = NULL;
    acl_mem_tlsf_area_info_t *sPtrPrev = NULL;
#endif
    acl_mem_tlsf_area_info_t *sAreaInfo = NULL;

    acl_mem_tlsf_bhdr_t *sInitBlock0 = NULL;
    acl_mem_tlsf_bhdr_t *sBlock0 = NULL;
    acl_mem_tlsf_bhdr_t *sLastBlock0 = NULL;
#if 0 /* PRJ-2152 */
    acl_mem_tlsf_bhdr_t *sInitBlock1 = NULL;
    acl_mem_tlsf_bhdr_t *sBlock1 = NULL;
    acl_mem_tlsf_bhdr_t *sLastBlock1 = NULL;
    acl_mem_tlsf_bhdr_t *sNextBlock = NULL;
#endif

    void      *sAreaAddr     = acpMmapGetAddress(aMmapArea);
    acp_size_t sAreaSize = acpMmapGetSize(aMmapArea);

#if 0 /* PRJ-2152 */
    acpMemSet(sAreaAddr, 0, sAreaSize));

    sPtr = sTlsf->mAreaHead;
    sPtrPrev = 0;
#endif
    sTlsf->mPoolSize += sAreaSize;

    sInitBlock0 = aclMemTlsfProcessArea(sAreaAddr, sAreaSize);
    sBlock0 = ACL_TLSF_NEXT_B(sInitBlock0->mPtr.mBuffer, 
                              sInitBlock0->mSize & ACL_TLSF_B_SIZE);
    sLastBlock0 = ACL_TLSF_NEXT_B(sBlock0->mPtr.mBuffer, 
                                  sBlock0->mSize & ACL_TLSF_B_SIZE);

    /* Before inserting the new area, we have to merge this area with the
       already existing ones */

#if 0
    /*
     * PRJ-2152
     * Original source code merge area if they are adjusted.
     * But if the areas are merged, shrinking an area is impossible.
     * Therefore we remove this code to make shrink possible.
     */

    while (sPtr) /* travel all areas */
    {
        /* init-block is the first block of the area */
        sInitBlock1 = (acl_mem_tlsf_bhdr_t *) 
             ((char *) sPtr - ACL_TLSF_BHDR_OVRHD);
        sBlock1 = ACL_TLSF_NEXT_B(sInitBlock1->mPtr.mBuffer, 
                                  sInitBlock1->mSize & ACL_TLSF_B_SIZE);
        sLastBlock1 = sPtr->mEnd;

        /* Merging the new area with the next physically contigous one */
        if ((acp_ulong_t) sInitBlock1 == 
                (acp_ulong_t) sLastBlock0 + ACL_TLSF_BHDR_OVRHD)
        {
            if (sTlsf->mAreaHead == sPtr)
            {
                sTlsf->mAreaHead = sPtr->mNext;
                sPtr = sPtr->mNext;
            }
            else
            {
                sPtrPrev->mNext = sPtr->mNext;
                sPtr = sPtr->mNext;
            }

            sBlock0->mSize =
                ACL_TLSF_DOWN_SIZE((sBlock0->mSize & ACL_TLSF_B_SIZE) +
                               (sInitBlock1->mSize & ACL_TLSF_B_SIZE) +
                               2 * ACL_TLSF_BHDR_OVRHD)
                | ACL_TLSF_USED_B | ACL_TLSF_PREV_USED;

            sBlock1->mPrevHdr = sBlock0;
            sLastBlock0 = sLastBlock1;

            continue;
        }
        else
        {
        }

        /* Merging the new area with the previous physically contigous
           one */
        if ((acp_ulong_t) sLastBlock1->mPtr.mBuffer == 
                (acp_ulong_t) sInitBlock0)
        {
            if (sTlsf->mAreaHead == sPtr)
            {
                sTlsf->mAreaHead = sPtr->mNext;
                sPtr = sPtr->mNext;
            }
            else
            {
                sPtrPrev->mNext = sPtr->mNext;
                sPtr = sPtr->mNext;
            }

            sLastBlock1->mSize =
            ACL_TLSF_DOWN_SIZE((sBlock0->mSize & ACL_TLSF_B_SIZE) +
            (sInitBlock0->mSize & ACL_TLSF_B_SIZE) + 2 * ACL_TLSF_BHDR_OVRHD) 
            | ACL_TLSF_USED_B |
            (sLastBlock1->mSize & ACL_TLSF_PREV_STATE);


            sNextBlock = ACL_TLSF_NEXT_B(sLastBlock1->mPtr.mBuffer, 
                                         sLastBlock1->mSize & ACL_TLSF_B_SIZE);
            sNextBlock->mPrevHdr = sLastBlock1;
            sBlock0 = sLastBlock1;
            sInitBlock0 = sInitBlock1;

            continue;
        }
        else
        {
        }

        sPtrPrev = sPtr;
        sPtr = sPtr->mNext;
    }
#endif
    
    /* Inserting the area in the list of linked areas */
    sAreaInfo = (acl_mem_tlsf_area_info_t *) sInitBlock0->mPtr.mBuffer;
    sAreaInfo->mNext = sTlsf->mAreaHead;
    sAreaInfo->mEnd = sLastBlock0;

    sAreaInfo->mMapArea = aMmapArea;

    sTlsf->mAreaHead = sAreaInfo;

    /* The aclMemTlsfFreeImp function will decrease the statistic infomation
     *  with the size of sBlock0, so the size of sBlock0 should be added before
     * aclMemTlsfFreeImp function.
     */
    ACL_TLSF_ADD_SIZE(sTlsf, sBlock0);
    aclMemTlsfFreeImp(sBlock0->mPtr.mBuffer, aPrimaryPool);

    return (sBlock0->mSize & ACL_TLSF_B_SIZE);
}

/*
 * Implementation of the malloc function
 */
ACP_EXPORT void *aclMemTlsfMallocImp(acp_size_t aSize, void *aPrimaryPool)
{
    acl_mem_tlsf_t *sTlsf = (acl_mem_tlsf_t *)aPrimaryPool;

    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    acl_mem_tlsf_bhdr_t *sBlock2 = NULL;
    acl_mem_tlsf_bhdr_t *sNextBlock = NULL;

    acp_sint32_t    sFl;
    acp_sint32_t    sSl;
    acp_size_t sTmpSize;

    aSize = (aSize < ACL_TLSF_MIN_B_SIZE) ?
        ACL_TLSF_MIN_B_SIZE : ACL_TLSF_UP_SIZE(aSize);

    /* Rounding up the requested size and calculating fl and sl */
    aclMemTlsfMappingSearch(&aSize, &sFl, &sSl);

    /* Searching a free block, recall that this function 
       changes the values of fl and sl,
       so they are not longer valid when the function fails */
    sBlock = aclMemTlsfFindBlock(sTlsf, &sFl, &sSl);

    if (sBlock == NULL)
    {
        acp_size_t sAreaSize;
        acp_mmap_t *sMmapArea = NULL;

        /* Growing the pool size when needed */
        /* size plus enough room for the requered headers. */

        sAreaSize = aSize + ACL_TLSF_BHDR_OVRHD * 8;

        /* BUG-32757: The original source makes a area as only */
        /* default area size. But we need to configure increasing size */
        /* sAreaSize = (sAreaSize > ACL_TLSF_DEFAULT_AREA_SIZE) ?  */
        /* sAreaSize : ACL_TLSF_DEFAULT_AREA_SIZE; */

        /* Total pool size grow as multiple of area size */
        sAreaSize = sTlsf->mAreaSize * ((sAreaSize / sTlsf->mAreaSize) + 1);

        sMmapArea = aclMemTlsfGetNewArea(&sAreaSize);

        if (sMmapArea == NULL)
        {
            return NULL;  /* Not enough system memory */
        }
        else
        {
            /* do nothing */
        }

        (void)aclMemTlsfAddNewArea(sMmapArea, aPrimaryPool);

        /* Rounding up the requested size and calculating fl and sl */
        aclMemTlsfMappingSearch(&aSize, &sFl, &sSl);

        /* Searching a free block */
        sBlock = aclMemTlsfFindBlock(sTlsf, &sFl, &sSl);
    }
    else
    {
        /* continue */
    }

    if (sBlock == NULL)
    {
        return NULL; /* Not found */
    }
    else
    {
        /* continue */
    }

    aclMemTlsfExtractBlockHdr(sBlock, sTlsf, sFl, sSl);

    /*-- found: */
    sNextBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer,
                                 sBlock->mSize & ACL_TLSF_B_SIZE);
    /* Should the block be split? */
    sTmpSize = (sBlock->mSize & ACL_TLSF_B_SIZE) - aSize;

    if (sTmpSize >= sizeof(acl_mem_tlsf_bhdr_t))
    {
        sTmpSize -= ACL_TLSF_BHDR_OVRHD;
        sBlock2 = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer, aSize);
        sBlock2->mSize = sTmpSize | ACL_TLSF_FREE_B | ACL_TLSF_PREV_USED;
        sNextBlock->mPrevHdr = sBlock2;
        aclMemTlsfMapSet(sTmpSize, &sFl, &sSl);
        aclMemTlsfInsertBlock(sBlock2, sTlsf, sFl, sSl);

        sBlock->mSize = aSize | (sBlock->mSize & ACL_TLSF_PREV_STATE);
    }
    else
    {
        sNextBlock->mSize &= (~ACL_TLSF_PREV_FREE);
        sBlock->mSize &= (~ACL_TLSF_FREE_B);       /* Now it's used */
    }

    ACL_TLSF_ADD_SIZE(sTlsf, sBlock);

    return (void *) sBlock->mPtr.mBuffer;
}

/*
 * Implementation of the free function
 */
ACP_EXPORT void aclMemTlsfFreeImp(void *aPtr, void *aPrimaryPool)
{
    acl_mem_tlsf_t *sTlsf = (acl_mem_tlsf_t *) aPrimaryPool;
    acl_mem_tlsf_bhdr_t *sBlock = NULL;
    acl_mem_tlsf_bhdr_t *sTmpBlock = NULL;

    acp_sint32_t sFl = 0;
    acp_sint32_t sSl = 0;

    if (aPtr == NULL)
    {
        return;
    }
    else
    {
        /* continue */
    }

    sBlock = (acl_mem_tlsf_bhdr_t *) ((char *) aPtr - ACL_TLSF_BHDR_OVRHD);
    sBlock->mSize |= ACL_TLSF_FREE_B;

    ACL_TLSF_REMOVE_SIZE(sTlsf, sBlock);

    sBlock->mPtr.mFreePtr.mPrev = NULL;
    sBlock->mPtr.mFreePtr.mNext = NULL;

    sTmpBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer, 
                                sBlock->mSize & ACL_TLSF_B_SIZE);

    if (sTmpBlock->mSize & ACL_TLSF_FREE_B)
    {
        aclMemTlsfMapSet(sTmpBlock->mSize & ACL_TLSF_B_SIZE, &sFl, &sSl);
        aclMemTlsfExtractBlock(sTmpBlock, sTlsf, sFl, sSl);
        sBlock->mSize += (sTmpBlock->mSize & ACL_TLSF_B_SIZE) 
            + ACL_TLSF_BHDR_OVRHD;
    }
    else
    {
    }

    if (sBlock->mSize & ACL_TLSF_PREV_FREE)
    {
        sTmpBlock = sBlock->mPrevHdr;
        aclMemTlsfMapSet(sTmpBlock->mSize & ACL_TLSF_B_SIZE, &sFl, &sSl);
        aclMemTlsfExtractBlock(sTmpBlock, sTlsf, sFl, sSl);
        sTmpBlock->mSize += (sBlock->mSize & ACL_TLSF_B_SIZE) 
            + ACL_TLSF_BHDR_OVRHD;
        sBlock = sTmpBlock;
    }
    else
    {
    }

    aclMemTlsfMapSet(sBlock->mSize & ACL_TLSF_B_SIZE, &sFl, &sSl);
    aclMemTlsfInsertBlock(sBlock, sTlsf, sFl, sSl);

    sTmpBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer, 
                       sBlock->mSize & ACL_TLSF_B_SIZE);
    sTmpBlock->mSize |= ACL_TLSF_PREV_FREE;
    sTmpBlock->mPrevHdr = sBlock;
}

/*
 * Implementation of the realloc function
 */
ACP_EXPORT void *aclMemTlsfReallocImp(void *aPtr, 
                                      acp_size_t aNewSize, 
                                      void *aPrimaryPool)
{
    acl_mem_tlsf_t *sTlsf = (acl_mem_tlsf_t *) aPrimaryPool;
    void           *sPtrAux = NULL;
    acp_uint32_t   sCpSize;
    acl_mem_tlsf_bhdr_t    *sBlock = NULL;
    acl_mem_tlsf_bhdr_t    *sTmpBlock = NULL;
    acl_mem_tlsf_bhdr_t    *sNextBlock = NULL;

    acp_sint32_t sFl;
    acp_sint32_t sSl;

    acp_size_t sTmpSize;

    if (aPtr == NULL)
    {
        if (aNewSize > 0)
        {
            return (void *) aclMemTlsfMallocImp(aNewSize, aPrimaryPool);
        }
        else
        {
            return NULL;
        }
    }
    else if (aNewSize == 0)
    {
        aclMemTlsfFreeImp(aPtr, aPrimaryPool);

        return NULL;
    }
    else
    {
        /* Continue with reallocation */
    }

    sBlock = (acl_mem_tlsf_bhdr_t *) ((char *) aPtr - ACL_TLSF_BHDR_OVRHD);
    sNextBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer, 
                                 sBlock->mSize & ACL_TLSF_B_SIZE);
    aNewSize = (aNewSize < ACL_TLSF_MIN_B_SIZE) ? 
          ACL_TLSF_MIN_B_SIZE : ACL_TLSF_UP_SIZE(aNewSize);
    sTmpSize = (sBlock->mSize & ACL_TLSF_B_SIZE);

    if (aNewSize <= sTmpSize)
    {
        ACL_TLSF_REMOVE_SIZE(sTlsf, sBlock);

        if (sNextBlock->mSize & ACL_TLSF_FREE_B)
        {
            aclMemTlsfMapSet(sNextBlock->mSize & ACL_TLSF_B_SIZE, &sFl, &sSl);
            aclMemTlsfExtractBlock(sNextBlock, sTlsf, sFl, sSl);
            sTmpSize += (sNextBlock->mSize & ACL_TLSF_B_SIZE) 
                + ACL_TLSF_BHDR_OVRHD;
            sNextBlock = ACL_TLSF_NEXT_B(sNextBlock->mPtr.mBuffer, 
                              sNextBlock->mSize & ACL_TLSF_B_SIZE);
            /* We allways reenter this free block because sTmpSize will
               be greater then sizeof (bhdr_t) */
        }
        else
        {

        }

        sTmpSize -= aNewSize;

        if (sTmpSize >= sizeof(acl_mem_tlsf_bhdr_t))
        {
            sTmpSize -= ACL_TLSF_BHDR_OVRHD;
            sTmpBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer, aNewSize);
            sTmpBlock->mSize = sTmpSize | ACL_TLSF_FREE_B | ACL_TLSF_PREV_USED;
            sNextBlock->mPrevHdr = sTmpBlock;
            sNextBlock->mSize |= ACL_TLSF_PREV_FREE;
            aclMemTlsfMapSet(sTmpSize, &sFl, &sSl);
            aclMemTlsfInsertBlock(sTmpBlock, sTlsf, sFl, sSl);
            sBlock->mSize = aNewSize | (sBlock->mSize & ACL_TLSF_PREV_STATE);
        }
        else
        {
           /* do nothing */
        }

        ACL_TLSF_ADD_SIZE(sTlsf, sBlock);

        return (void *) sBlock->mPtr.mBuffer;
    }
    else
    {
        /* continue */
    }

    if ((sNextBlock->mSize & ACL_TLSF_FREE_B))
    {
        if (aNewSize <= (sTmpSize + (sNextBlock->mSize & ACL_TLSF_B_SIZE)))
        {
            ACL_TLSF_REMOVE_SIZE(sTlsf, sBlock);

            aclMemTlsfMapSet(sNextBlock->mSize & ACL_TLSF_B_SIZE, 
                                    &sFl, &sSl);
            aclMemTlsfExtractBlock(sNextBlock, sTlsf, sFl, sSl);
            sBlock->mSize += (sNextBlock->mSize & ACL_TLSF_B_SIZE) 
                          + ACL_TLSF_BHDR_OVRHD;
            sNextBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer, 
                                         sBlock->mSize & ACL_TLSF_B_SIZE);
            sNextBlock->mPrevHdr = sBlock;
            sNextBlock->mSize &= ~ACL_TLSF_PREV_FREE;
            sTmpSize = (sBlock->mSize & ACL_TLSF_B_SIZE) - aNewSize;

            if (sTmpSize >= sizeof(acl_mem_tlsf_bhdr_t))
            {
                sTmpSize -= ACL_TLSF_BHDR_OVRHD;
                sTmpBlock = ACL_TLSF_NEXT_B(sBlock->mPtr.mBuffer,
                                            aNewSize);
                sTmpBlock->mSize = sTmpSize |
                                   ACL_TLSF_FREE_B |
                                   ACL_TLSF_PREV_USED;
                sNextBlock->mPrevHdr = sTmpBlock;
                sNextBlock->mSize |= ACL_TLSF_PREV_FREE;
                aclMemTlsfMapSet(sTmpSize, &sFl, &sSl);
                aclMemTlsfInsertBlock(sTmpBlock, sTlsf, sFl, sSl);
                sBlock->mSize = aNewSize |
                                (sBlock->mSize & ACL_TLSF_PREV_STATE);
            }
            else
            {
                /* do nothing */
            }

            ACL_TLSF_ADD_SIZE(sTlsf, sBlock);

            return (void *) sBlock->mPtr.mBuffer;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    sPtrAux = aclMemTlsfMallocImp(aNewSize, aPrimaryPool);

    if (sPtrAux == NULL)
    {
        return NULL;
    }
    else
    {
        /* continue */
    }

    sCpSize = (acp_uint32_t)(((sBlock->mSize & ACL_TLSF_B_SIZE) > aNewSize) ? 
              aNewSize : (sBlock->mSize & ACL_TLSF_B_SIZE));

    acpMemCpy(sPtrAux, aPtr, sCpSize);

    aclMemTlsfFreeImp(aPtr, aPrimaryPool);

    return sPtrAux;
}

/*
 * Implementation of the calloc function
 */
ACP_EXPORT void *aclMemTlsfCallocImp(acp_size_t aNelem, 
                                     acp_size_t aElemSize, 
                                     void *aPrimaryPool)
{
    void *sPtr = NULL;

    if (aNelem <= 0 || aElemSize <= 0)
    {
        return NULL;
    }
    else
    {
        /* continue */
    }

    sPtr = aclMemTlsfMallocImp(aNelem * aElemSize, aPrimaryPool);

    if (sPtr != NULL)
    {
        acpMemSet(sPtr, 0, aNelem * aElemSize);
    }
    else
    {
        /* do nothing, return pointer even if it is NULL */
    }

    return sPtr;
}
