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
 * $Id: aclMem.h 4955 2009-03-24 09:31:05Z djin $
 ******************************************************************************/

#if !defined(_O_ACL_MEM_H_)
#define _O_ACL_MEM_H_

/**
 * @file
 * @ingroup CoreMem
 *
 * Memory allocator API 
 */

#include <acpMem.h>


ACP_EXTERN_C_BEGIN

/**
 * Allocator types
 */
typedef enum acl_mem_alloc_type_t
{
    ACL_MEM_ALLOC_LIBC = 0,  /**< system default library in LIBC */
    ACL_MEM_ALLOC_TLSF,      /**< real-time algorithm */

    /* Add new allocator type here */

    /* This is sentinel */
    ACL_MEM_ALLOC_TABLE_SIZE

} acl_mem_alloc_type_t;

/**
 * Allocator attributes
 */
typedef enum acl_mem_alloc_attr_t
{
    ACL_MEM_THREAD_SAFE_ATTR = 0, /**< can be shared by multiple threads */
    ACL_MEM_IS_FULL,              /**< is full? */
    ACL_MEM_IS_EMPTY,             /**< is empty? */
    ACL_MEM_SET_SPIN_COUNT,       /**< change spin loop count
                                     if allocator use spin-lock
                                     for synchronization */
    ACL_MEM_GET_SPIN_COUNT,       /**< get spin loop count
                                     if allocator use spin-lock
                                     for synchronization */
    ACL_MEM_SHRINK,               /**< shrink empty memory occupied
                                     by allocator */
    
    /* sentinel */
    ACL_MEM_ALLOC_ATTR_SIZE

} acl_mem_alloc_attr_t;


/*
 *  Allocator structure
 */
typedef struct acl_mem_alloc_t
{
    /* Allocator implementation specific control structure */
    void *mHandle;

    /* Thread safe attribute */
    acp_bool_t mIsThreadSafe;

    /* Memory allocator initialize function */
    acp_rc_t (*mAclMemInit)(struct acl_mem_alloc_t *aHandle,
                            void *aArgs);

    /* Memory allocator finalize function */
    acp_rc_t (*mAclMemFinit)(struct acl_mem_alloc_t *aHandle);

    /* Memory allocator statistic function */
    acp_rc_t (*mAclMemStat)(struct acl_mem_alloc_t *aHandle, 
                            void *aStat);

    /* Memory allocator alloc function */
    acp_rc_t (*mAclMemAlloc)(struct acl_mem_alloc_t *aHandle,
                             void **aAddr,
                             acp_size_t aSize);

    /* Memory allocator calloc function */
    acp_rc_t (*mAclMemCalloc)(struct acl_mem_alloc_t *aHandle,
                              void  **aAddr,
                              acp_size_t   aElements,
                              acp_size_t   aSize);

    /* Memory allocator realloc function */
    acp_rc_t (*mAclMemRealloc)(struct acl_mem_alloc_t *aHandle,
                               void **aAddr,
                               acp_size_t aSize);

    /* Memory alloc free function */
    acp_rc_t (*mAclMemFree)(struct acl_mem_alloc_t *aHandle,
                         void *aAddr);

    acp_rc_t (*mAclMemControl)(struct acl_mem_alloc_t *aHandle,
                               acl_mem_alloc_attr_t aAttr,
                               void *aArg);
} acl_mem_alloc_t;


ACP_EXPORT acp_rc_t aclMemAllocGetInstance(acl_mem_alloc_type_t  aAllocatorType,
                                           void                 *aArgs,
                                           acl_mem_alloc_t      **aAllocator);

ACP_EXPORT acp_rc_t aclMemAllocFreeInstance (acl_mem_alloc_t *aAllocator);


ACP_EXPORT void  aclMemAllocSetDefault(acl_mem_alloc_t *aAllocator);


ACP_EXPORT acp_rc_t  aclMemAllocSetAttr(acl_mem_alloc_t *aAllocator,
                                        acl_mem_alloc_attr_t aAttrName,
                                        void *aAttrValue);

ACP_EXPORT acp_rc_t  aclMemAllocGetAttr(acl_mem_alloc_t      *aAllocator,
                                        acl_mem_alloc_attr_t  aAttrName,
                                        void                 *aAttrValue);


ACP_EXPORT acp_rc_t aclMemAllocStatistic(acl_mem_alloc_t *aAllocator, 
                                         void *aStat);


ACP_EXPORT acp_rc_t aclMemAlloc(acl_mem_alloc_t *aAllocator,
                                void **aAddr,
                                acp_size_t aSize);


ACP_EXPORT acp_rc_t aclMemCalloc(acl_mem_alloc_t *aAllocator,
                                 void  **aAddr,
                                 acp_size_t   aElements,
                                 acp_size_t   aSize);


ACP_EXPORT acp_rc_t aclMemRealloc(acl_mem_alloc_t *aAllocator,
                                  void **aAddr,
                                  acp_size_t aSize);


ACP_EXPORT acp_rc_t aclMemFree(acl_mem_alloc_t *aAllocator, void *aAddr);



ACP_EXTERN_C_END


#endif
