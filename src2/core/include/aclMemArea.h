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
 * $Id: aclMemArea.h 4955 2009-03-24 09:31:05Z djin $
 ******************************************************************************/

#if !defined(_O_ACL_MEM_AREA_H_)
#define _O_ACL_MEM_AREA_H_

/**
 * @file
 * @ingroup CoreMem
 *
 * stack like dynamic memory allocator
 */

#include <acpMem.h>
#include <acpList.h>


ACP_EXTERN_C_BEGIN


/**
 * memory area
 */
typedef struct acl_mem_area_t
{
#if defined(ACP_CFG_DEBUG)
    acp_uint32_t     mChunkCount;           /* number of current chunks */
#endif
    acp_size_t       mChunkSize;            /* minimum chunk size       */
    acp_list_t       mChunkList;            /* chunk list               */
    acp_list_node_t *mCurrentChunkListNode; /* current chunk node       */
} acl_mem_area_t;

/**
 * memory area snapshot
 */
typedef struct acl_mem_area_snapshot_t
{
#if defined(ACP_CFG_DEBUG)
    acl_mem_area_t  *mMemArea;              /* pointer to the parent mem area */
    acp_uint32_t     mChunkCount;           /* number of current chunks       */
#endif
    acp_size_t       mAllocSize;            /* allocated size of current node */
    acp_list_node_t *mCurrentChunkListNode; /* current chunk node             */
} acl_mem_area_snapshot_t;

/*
 * memory area statistics information
 */
typedef struct acl_mem_area_stat_t
{
    acp_uint32_t mChunkCount; /* number of chunks         */
    acp_size_t   mTotalSize;  /* total size of chunks     */
    acp_size_t   mAllocSize;  /* allocated size of chunks */
} acl_mem_area_stat_t;


ACP_EXPORT void aclMemAreaCreate(acl_mem_area_t *aArea, acp_size_t aChunkSize);
ACP_EXPORT void aclMemAreaDestroy(acl_mem_area_t *aArea);

ACP_EXPORT acp_rc_t aclMemAreaAlloc(acl_mem_area_t  *aArea,
                                    void           **aAddr,
                                    acp_size_t       aSize);

/**
 * allocates a zero-initialized memory block from the memory area
 * @param aArea pointer to the memory area
 * @param aAddr pointer to a variable to store allocated memory address
 * @param aSize size of the memory block to allocate
 * @return result code
 */
ACP_INLINE acp_rc_t aclMemAreaCalloc(acl_mem_area_t  *aArea,
                                     void           **aAddr,
                                     acp_size_t       aSize)
{
    acp_rc_t sRC;

    sRC = aclMemAreaAlloc(aArea, aAddr, aSize);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acpMemSet(*(void **)aAddr, 0, aSize);
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

ACP_EXPORT void aclMemAreaGetSnapshot(acl_mem_area_t     *aArea,
                                      acl_mem_area_snapshot_t *aSnapshot);
ACP_EXPORT void aclMemAreaFreeToSnapshot(acl_mem_area_t     *aArea,
                                         acl_mem_area_snapshot_t *aSnapshot);
ACP_EXPORT void aclMemAreaFreeAll(acl_mem_area_t *aArea);

ACP_EXPORT void aclMemAreaShrink(acl_mem_area_t *aArea);

ACP_EXPORT void aclMemAreaStat(acl_mem_area_t      *aArea,
                               acl_mem_area_stat_t *aStat);


ACP_EXTERN_C_END


#endif
