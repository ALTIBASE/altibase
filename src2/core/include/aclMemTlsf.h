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
 * $Id: aclMemTlsf.h 4955 2009-03-24 09:31:05Z djin $
 ******************************************************************************/

#if !defined(_O_ACL_MEM_TLSF_H_)
#define _O_ACL_MEM_TLSF_H_

/**
 * @file
 * @ingroup CoreMem
 *
 * Memory allocator API 
 */

#include <acpMem.h>
#include <acpMmap.h>
#include <acpSpinLock.h>

ACP_EXTERN_C_BEGIN

/*
 *  TLSF implementation macros and constants
 */

#define ACL_TLSF_ADD_SIZE(tlsf, b)                          \
    do                                                      \
    {                                                       \
        tlsf->mUsedSize += ((b->mSize & ACL_TLSF_B_SIZE)    \
                            + ACL_TLSF_BHDR_OVRHD);         \
    } while(0)


#define  ACL_TLSF_REMOVE_SIZE(tlsf, b)                      \
    do                                                      \
    {                                                       \
        tlsf->mUsedSize -= ((b->mSize & ACL_TLSF_B_SIZE)    \
                + ACL_TLSF_BHDR_OVRHD);                     \
    } while(0)


#define ACL_TLSF_B_ALIGN (sizeof(void *) * 2)

/* Maximum memory size that tlsf can support is 2^30 */
#define ACL_TLSF_MAX_FLI      (30)
#define ACL_TLSF_MAX_LOG2_SLI (5) 

/**
 * TLSF can support 2^30 bytes at most. A memory reguest bigger than 2^30-1MB is
 * handled by mmap system-call(1MB is for block header).
 * When free the huge memory, we call unmap and
 * VSZ is reduced immediatly.
 */
#define ACL_TLSF_HUGE_MEMORY_SIZE ((1<<ACL_TLSF_MAX_FLI) - 1024*1024)

/* Number of columns in matrix */
/* ACL_TLSF_MAX_SLI = 2^ACL_TLSF_MAX_LOG2_SLI */
#define ACL_TLSF_MAX_SLI      (1 << ACL_TLSF_MAX_LOG2_SLI)

/*
 * Blocks smaller thatn ACL_TLSF_SMALL_B-bytes are stored at Matrix[0]
 */
#define ACL_TLSF_SMALL_B    (128)

/*
 * ACL_TLSF_SMALL_B is 128 and it means 2^6.
 * If the request size is smaller than 128,
 * it search block at Matrix[0].
 * So FLI_OFFSET is 6.
 * For example, if request size is 200, MS-bit is 7.
 * And FL is (7 - ACL_TLSF_FLI_OFFSET) = 1.
 * Therefore it search block at Matrix[1][sl].
 */
#define ACL_TLSF_FLI_OFFSET   (6)
#define ACL_TLSF_REAL_FLI   (ACL_TLSF_MAX_FLI - ACL_TLSF_FLI_OFFSET)
#define ACL_TLSF_MIN_B_SIZE (sizeof (acl_mem_tlsf_free_ptr_t))

#define ACL_TLSF_BHDR_OVRHD                                      \
     (sizeof (acl_mem_tlsf_bhdr_t) - ACL_TLSF_MIN_B_SIZE)

#define ACL_TLSF_SIGNATURE (0x2A59FA59)

#define ACL_TLSF_PTR_MASK   (sizeof(void *) - 1)
#define ACL_TLSF_B_SIZE     (0xFFFFFFFF - ACL_TLSF_PTR_MASK)

#define ACL_TLSF_NEXT_B(_addr, _r)                      \
    ((acl_mem_tlsf_bhdr_t *) ((char *) (_addr) + (_r)))

#define ACL_TLSF_ALIGN       ((ACL_TLSF_B_ALIGN) - 1)

#define ACL_TLSF_UP_SIZE(_r) (((_r) + ACL_TLSF_ALIGN) & ~ACL_TLSF_ALIGN)

#define ACL_TLSF_DOWN_SIZE(_r)    ((_r) & ~ACL_TLSF_ALIGN)

#define ACL_TLSF_ROUNDUP(_x, _v)  ((((~(_x)) + 1) & ((_v)-1)) + (_x))

#define ACL_TLSF_B_STATE     (0x1)
#define ACL_TLSF_PREV_STATE  (0x2)

/* bit 0 of the block size */
#define ACL_TLSF_FREE_B  (0x1)
#define ACL_TLSF_USED_B  (0x0)

/* bit 1 of the block size */
#define ACL_TLSF_PREV_FREE   (0x2)
#define ACL_TLSF_PREV_USED   (0x0)

/**
 * The size of memory area maintained by TLSF allocator.
 * Default size is 256k which may enough for one thread
 * but small if area is shared by many threads.
 * You should initialize area size with acl_mem_tlsf_init_t parameter.
 * It must be bigger than 8K and multiple of page size.
 * @see acl_mem_tlsf_init_t
 */
#define ACL_TLSF_DEFAULT_AREA_SIZE (1024 * 256)

/**
 * Structure for TLSF allocator instance
 */
typedef struct acl_mem_alloc_tlsf_t
{
    /* acl_mem_tlsf_t data structure exists at the head of primary area */
    acp_uint8_t *mPrimaryArea;
    acp_ssize_t  mPoolSize;

    acp_spin_lock_t mLock;

} acl_mem_alloc_tlsf_t;

/**
 * parameters to initialize allocator instance
 */
typedef struct acl_mem_tlsf_init_t
{
    acp_size_t mPoolSize;  /**< Initial memory pool size
                              If allocator consumes all memory in the pool,
                              it will allocate another pool from system.
                              Therefore memory size allocator consumes is
                              increases as multiple of initial pool size. */
} acl_mem_tlsf_init_t;

/**
 * Structure describes memory statistics
 */
typedef struct acl_mem_tlsf_stat_t
{
    acp_size_t mUsedSize;    /**< Allocated memory size */
    acp_size_t mPoolSize;    /**< Total memory pool size, mmaped from OS */
} acl_mem_tlsf_stat_t;

/*
 * Structure describes list of free blocks
 */
typedef struct free_ptr_struct
{
    struct bhdr_struct *mPrev;
    struct bhdr_struct *mNext;

} acl_mem_tlsf_free_ptr_t;

/*
 * Structure describes TLSF block
 * refer to boundary tag mechanism
 */
typedef struct bhdr_struct
{
    /* This pointer is just valid if the first bit of size is set */
    struct bhdr_struct *mPrevHdr;

    /* The size is stored in bytes */
    /* bit 0 indicates whether the block is used and */
    /* bit 1 allows to know whether the previous block is free */
    acp_size_t mSize;

    union
    {
        struct free_ptr_struct mFreePtr;

        acp_uint8_t mBuffer[1];         /* sizeof(struct free_ptr_struct)]; */
    } mPtr;

} acl_mem_tlsf_bhdr_t;

/*
 * This structure is embedded at the beginning of each area, giving us
 * enough information to cope with a set of areas 
 */
typedef struct area_info_struct
{
    acl_mem_tlsf_bhdr_t *mEnd;

    /* Pointer to the next area */
    struct area_info_struct *mNext;

    /* Pointer to mmaped memory area */
    acp_mmap_t *mMapArea;

} acl_mem_tlsf_area_info_t;

/*
 * Structure describes TLSF allocator
 */
typedef struct TLSF_struct
{
    /* the TLSF's structure signature */
    acp_uint32_t mTlsfSignature;

    /* size of one area
     * The area is an unit of memory pool and maintained as area list.
     * The pool means a set of area included in one tlsf instance.
     */
    acp_size_t mAreaSize;

    /* Statistic member allocated size */
    acp_size_t mUsedSize;

    /* Statistic member overall pool size */
    acp_size_t mPoolSize;

    /* A linked list holding all the existing areas */
    acl_mem_tlsf_area_info_t *mAreaHead;

    /* the first-level bitmap */
    /* This array should have a size of ACL_TLSF_REAL_FLI bits */
    acp_uint32_t mFlBitmap;

    /* the second-level bitmap */
    acp_uint32_t mSlBitmap[ACL_TLSF_REAL_FLI];

    /* Matrix of pointers to blocks */
    acl_mem_tlsf_bhdr_t *mMatrix[ACL_TLSF_REAL_FLI][ACL_TLSF_MAX_SLI];

    /* A special linked list for huge size block */
    acl_mem_tlsf_area_info_t *mHugeAreaHead;
    
} acl_mem_tlsf_t;


ACP_EXPORT acp_rc_t aclMemFreeTlsf(acl_mem_alloc_t *aAllocator, void *aAddr);

ACP_EXPORT  acp_rc_t aclMemAllocTlsf(acl_mem_alloc_t *aAllocator,
                                     void **aAddr,
                                     acp_size_t aSize);


ACP_EXPORT acp_rc_t aclMemReallocTlsf(acl_mem_alloc_t *aAllocator,
                                      void **aAddr,
                                      acp_size_t aSize);

ACP_EXPORT acp_rc_t aclMemCallocTlsf(acl_mem_alloc_t *aAllocator,
                                     void  **aAddr,
                                     acp_size_t   aElements,
                                     acp_size_t   aSize);

ACP_EXPORT acp_rc_t aclMemInitTlsf(acl_mem_alloc_t *aAllocator, 
                                   void *aArgs);

ACP_EXPORT acp_rc_t aclMemStatisticTlsf(acl_mem_alloc_t *aAllocator,
                                        void *aStat);

ACP_EXPORT acp_rc_t aclMemFinitTlsf(acl_mem_alloc_t *aAllocator);

ACP_EXPORT acp_rc_t aclMemControlTlsf(acl_mem_alloc_t      *aAllocator,
                                      acl_mem_alloc_attr_t  aAttrName,
                                      void                 *aArg);

ACP_EXPORT acp_rc_t aclMemTlsfShrink(acl_mem_alloc_t *aAllocator,
                                     acp_sint32_t *aCount);

    
ACP_EXTERN_C_END

#endif
