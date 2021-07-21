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
 * $Id: aclMemPool.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACL_MEM_POOL_H_)
#define _O_ACL_MEM_POOL_H_

/**
 * @file
 * @ingroup CoreMem
 *
 * dynamic memory allocator for fixed size memory blocks
 */

#include <acpMem.h>


ACP_EXTERN_C_BEGIN


/**
 * memory pool
 */
typedef struct acl_mem_pool_t
{
    acp_size_t              mElementSize;    /* size of element              */
    acp_uint32_t            mElementCount;   /* number of element in a chunk */
    acp_sint32_t            mParallelFactor; /* number of chunk stores       */

    struct aclMemPoolStore *mStore;          /* chunk store array            */
} acl_mem_pool_t;


ACP_EXPORT acp_rc_t aclMemPoolCreate(acl_mem_pool_t *aPool,
                                     acp_size_t      aElementSize,
                                     acp_uint32_t    aElementCount,
                                     acp_sint32_t    aParallelFactor);
ACP_EXPORT void     aclMemPoolDestroy(acl_mem_pool_t *aPool);

ACP_EXPORT acp_rc_t aclMemPoolAlloc(acl_mem_pool_t *aPool, void **aAddr);

/**
 * allocates a zero-initialized memory block from the pool
 * @param aPool pointer to the memory pool
 * @param aAddr pointer to a variable to store allocated memory address
 * @return result code
 */
ACP_INLINE acp_rc_t aclMemPoolCalloc(acl_mem_pool_t *aPool, void **aAddr)
{
    acp_rc_t sRC;

#if defined(ACP_CFG_MEMORY_CHECK)
    sRC = acpMemCalloc(aAddr, 1, aPool->mElementSize);
#else
    sRC = aclMemPoolAlloc(aPool, aAddr);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acpMemSet(*aAddr, 0, aPool->mElementSize);
    }
    else
    {
        /* do nothing */
    }
#endif

    return sRC;
}

ACP_EXPORT void aclMemPoolFree(acl_mem_pool_t *aPool, void *aAddr);


ACP_EXTERN_C_END


#endif
