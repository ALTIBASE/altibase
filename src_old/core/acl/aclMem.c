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
 * $Id: aclMem.c 4955 2009-03-24 09:31:05Z djin $
 ******************************************************************************/

#include <aclMem.h>
#include <acpMem.h>
#include <acpTest.h>
#include <acpAtomic.h>
#include <aclMemTlsf.h>

/*
 * Prototype declaration for ACL_MEM_ALLOC_LIBC allocator
 */
static acp_rc_t aclMemAllocAcp(acl_mem_alloc_t *aHandle,
                            void **aAddr,
                            acp_size_t aSize);

static acp_rc_t aclMemCallocAcp(acl_mem_alloc_t *aHandle,
                             void  **aAddr,
                             acp_size_t   aElements,
                             acp_size_t   aSize);

static acp_rc_t aclMemReallocAcp(acl_mem_alloc_t *aHandle,
                              void **aAddr,
                              acp_size_t aSize);

static acp_rc_t aclMemFreeAcp(acl_mem_alloc_t *aHandle, void *aAddr);

static acp_rc_t aclMemControlAcp(acl_mem_alloc_t *aHandle,
                                 acl_mem_alloc_attr_t aAttrName,
                                 void *aAttrValue);


/* Static table with memory allocators */
static acl_mem_alloc_t gAclMem[ACL_MEM_ALLOC_TABLE_SIZE] = 
                                                   {
                    /* ACL_MEM_ALLOC_LIBC allocator */  {NULL,
                                                         ACP_TRUE,
                                                         NULL, 
                                                         NULL,
                                                         NULL,
                                                         &aclMemAllocAcp,
                                                         &aclMemCallocAcp,
                                                         &aclMemReallocAcp,
                                                         &aclMemFreeAcp,
                                                         &aclMemControlAcp
                                                        },

                    /* ACL_MEM_ALLOC_TLSF allocator */  {NULL,
                                                         ACP_FALSE,
                                                         &aclMemInitTlsf,
                                                         &aclMemFinitTlsf,
                                                         &aclMemStatisticTlsf,
                                                         &aclMemAllocTlsf,
                                                         &aclMemCallocTlsf,
                                                         &aclMemReallocTlsf,
                                                         &aclMemFreeTlsf,
                                                         &aclMemControlTlsf
                                                        }
                                                   };

/*
 * Default memory allocator ACL_MEM_ALLOC_LIBC which uses 
 * Libc malloc via ACP acpMemAlloc functions 
 */
static volatile acl_mem_alloc_t *gAclMemDefault = &gAclMem[ACL_MEM_ALLOC_LIBC];


/**
 * Creates new instance of given memory allocator type
 *
 * @param aAllocatorType type of allocator
 * @param aArgs pointer to allocator specific init params
 * @param aAllocator pointer to variable which points to memory allocator
 * instance structure
 * @return result code
 *
 * It returns allocator init function specific return code
 * It returns ACP_RC_EINVAL if allocator type name is invalid
 * It returns ACP_RC_ENOMEM if there is not enough memory to create 
 * new allocator instance
 */
ACP_EXPORT acp_rc_t aclMemAllocGetInstance(acl_mem_alloc_type_t  aAllocatorType,
                                           void                 *aArgs,
                                           acl_mem_alloc_t      **aAllocator)
{
    acp_rc_t sRC;

    ACP_TEST_RAISE(aAllocatorType >= ACL_MEM_ALLOC_TABLE_SIZE, E_BAD_ALLOCATOR);

    sRC = acpMemAlloc((void **)aAllocator,
                      sizeof(acl_mem_alloc_t));

    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_BAD_MEM);

    /* Assign allocator functions to this allocator */
    (*aAllocator)->mAclMemInit    = gAclMem[aAllocatorType].mAclMemInit;
    (*aAllocator)->mAclMemFinit   = gAclMem[aAllocatorType].mAclMemFinit;
    (*aAllocator)->mAclMemStat    = gAclMem[aAllocatorType].mAclMemStat;
    (*aAllocator)->mAclMemAlloc   = gAclMem[aAllocatorType].mAclMemAlloc;
    (*aAllocator)->mAclMemCalloc  = gAclMem[aAllocatorType].mAclMemCalloc;
    (*aAllocator)->mAclMemRealloc = gAclMem[aAllocatorType].mAclMemRealloc;
    (*aAllocator)->mAclMemFree    = gAclMem[aAllocatorType].mAclMemFree;
    (*aAllocator)->mAclMemControl = gAclMem[aAllocatorType].mAclMemControl;

    /* Call allocator specific init function */
    if ((*aAllocator)->mAclMemInit != NULL)
    {
        sRC = (*aAllocator)->mAclMemInit(*aAllocator,
                                          aArgs);

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_BAD_INIT);
    }
    else
    {
        (*aAllocator)->mHandle = gAclMem[aAllocatorType].mHandle;

        sRC = ACP_RC_SUCCESS;
    }

    /* Assign allocator attributes */
    (*aAllocator)->mIsThreadSafe = gAclMem[aAllocatorType].mIsThreadSafe;

    return sRC;

    ACP_EXCEPTION(E_BAD_ALLOCATOR)
    { 
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_BAD_MEM)
    {
        sRC = ACP_RC_ENOMEM;
    }

    ACP_EXCEPTION(E_BAD_INIT)
    {
        /* Free memory sRC will have result from allocator init function */
        acpMemFree(aAllocator);
    }

    ACP_EXCEPTION_END;

    return sRC;
}

/**
 * Destroys instance of memory allocator
 *
 * @param aAllocator pointer to the allocator instance
 * @return result code
 *
 * It returns allocator specific return code
 * It returns ACP_RC_EINVAL if pointer to allocator instance 
 * is NULL 
 */
ACP_EXPORT acp_rc_t aclMemAllocFreeInstance(acl_mem_alloc_t *aAllocator)
{
    acp_rc_t sRC;

    if (aAllocator != NULL)
    {
        if (aAllocator->mAclMemFinit != NULL)
        {
            /* Call finit function to destroy memory alllocator structure
             * mHandle should be freed by finit function
             */
            sRC = aAllocator->mAclMemFinit(aAllocator);
        }
        else
        {
            /* If there is no finalize function then consider this  
             * as no finalization is needed and return success
             */
            sRC =  ACP_RC_SUCCESS;
        }

        /* Free allocator */
        acpMemFree(aAllocator);
    }
    else
    {
        sRC = ACP_RC_EINVAL;
    }

    return sRC;
}

/**
 * Sets allocator instance as default memory allocator
 *
 * @param aAllocator pointer to the allocator instance
 *
 * If pointer to allocator instance is NULL then ACL_MEM_ALLOC_LIBC
 * allocator will be set as default
 */
ACP_EXPORT void  aclMemAllocSetDefault(acl_mem_alloc_t *aAllocator)
{
    /* Set default allocator ACL_MEM_ALLOC_LIBC */
    if (aAllocator == NULL)
    {
        (void)acpAtomicSet(&gAclMemDefault, 
                          (acp_slong_t)&gAclMem[ACL_MEM_ALLOC_LIBC]); 
    }
    else
    {
        (void)acpAtomicSet(&gAclMemDefault, 
                          (acp_slong_t)aAllocator);
    }
}

/**
 * Sets allocator instance attribute
 *
 * @param aAllocator pointer to the allocator instance
 * @param aAttrName attribute name
 * @param aAttrValue pointer to  attribute value
 * @return result code
 *
 * If allocator does not support attribute then it will be 
 * ignored
 *
 * It returns ACP_RC_EINVAL if attribute name is invalid 
 */
ACP_EXPORT acp_rc_t  aclMemAllocSetAttr(acl_mem_alloc_t *aAllocator, 
                                        acl_mem_alloc_attr_t aAttrName, 
                                        void *aAttrValue)
{
    acp_rc_t        sRC = ACP_RC_SUCCESS;
    acl_mem_alloc_t *sAllocator = NULL;

    ACP_TEST_RAISE(aAttrName >= ACL_MEM_ALLOC_ATTR_SIZE, E_BAD_ATTRNAME);
        
    if (aAllocator == NULL)
    {
        sAllocator = (acl_mem_alloc_t *)gAclMemDefault;
    }
    else
    {
        sAllocator = aAllocator;
    }

    ACP_TEST_RAISE(sAllocator->mAclMemControl == NULL, E_NO_CONTROLFUNC);

    switch (aAttrName)
    {
        /* Some attr can be set here
           and some can be set via allocator controler function */
        case ACL_MEM_THREAD_SAFE_ATTR:
            {
                sAllocator->mIsThreadSafe = *((acp_bool_t *)aAttrValue);
            }
            break;

        default:
            sRC = sAllocator->mAclMemControl(sAllocator,
                                             aAttrName,
                                             aAttrValue);
            break;
    }
    
    return sRC;

    ACP_EXCEPTION(E_BAD_ATTRNAME)
    { 
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_NO_CONTROLFUNC)
    {
        sRC = ACP_RC_ENOTSUP;
    }

    ACP_EXCEPTION_END;
    
    return sRC;
}

/**
 * Gets allocator instance attribute
 *
 * @param aAllocator pointer to the allocator instance
 * @param aAttrName attribute name
 * @param aAttrValue pointer to variable to hold attribute value
 * @return result code
 *
 * It returns ACP_RC_EINVAL if attribute name is invalid 
 */
ACP_EXPORT acp_rc_t  aclMemAllocGetAttr(acl_mem_alloc_t      *aAllocator,
                                        acl_mem_alloc_attr_t  aAttrName, 
                                        void                 *aAttrValue)
{
    acp_rc_t        sRC = ACP_RC_SUCCESS;
    acl_mem_alloc_t *sAllocator = NULL;
 
    ACP_TEST_RAISE(aAttrName >= ACL_MEM_ALLOC_ATTR_SIZE, E_BAD_ATTRNAME);
        
    if (aAllocator == NULL)
    {
        sAllocator = (acl_mem_alloc_t *)gAclMemDefault;
    }
    else
    {
        sAllocator = aAllocator;
    }
    
    ACP_TEST_RAISE(sAllocator->mAclMemControl == NULL, E_NO_CONTROLFUNC);
    
    switch (aAttrName)
    {
        /* Some attr can be read here
           and some can be read via allocator controler function */
        case ACL_MEM_THREAD_SAFE_ATTR:
            {
                *((acp_bool_t *)aAttrValue) = sAllocator->mIsThreadSafe;
            }
            break;

        default:
            sRC = sAllocator->mAclMemControl(sAllocator,
                                             aAttrName,
                                             aAttrValue);
            break;
    }

    return sRC;

    ACP_EXCEPTION(E_BAD_ATTRNAME)
    { 
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(E_NO_CONTROLFUNC)
    {
        sRC = ACP_RC_ENOTSUP;
    }

    ACP_EXCEPTION_END;

    return sRC;
}


/**
 * Gets allocator statistics
 * 
 * @param aAllocator pointer to the allocator instance
 * @param aStat pointer to the allocator specific structure to hold 
 * statistic data
 * @return result code
 *
 * It returns allocator specific return code
 * It returns ACP_RC_ENOTSUP if allocator does not support this function
 */
ACP_EXPORT acp_rc_t aclMemAllocStatistic(acl_mem_alloc_t *aAllocator, 
                                         void *aStat)
{
    acl_mem_alloc_t *sAllocator = NULL;

    if (aAllocator == NULL)
    {
        sAllocator = (acl_mem_alloc_t *)gAclMemDefault;
    }
    else
    {
        sAllocator = aAllocator;
    }

    if (sAllocator-> mAclMemStat != NULL)
    {
        return  sAllocator->mAclMemStat(sAllocator, aStat);
    }
    else
    {
        return  ACP_RC_ENOTSUP;
    }
}


/**
 * Allocates memory using provided memory allocator instance
 *
 * @param aAllocator pointer to the allocator instance
 * @param aAddr pointer to variable which will hold pointer to 
 * allocated memory block
 * @param aSize the size of memory in bytes to be allocated
 * @return result code
 *
 * If pointer to allocator instance is NULL then default allocator will be used
 *
 * It returns allocator specific return code
 * It returns ACP_RC_ENOTSUP if allocator does not support this function
 */
ACP_EXPORT acp_rc_t aclMemAlloc(acl_mem_alloc_t *aAllocator,
                                void **aAddr,
                                acp_size_t aSize)
{
#if defined(__STATIC_ANALYSIS_DOING__)

    ACP_UNUSED(aAllocator);

    return acpMemAlloc(aAddr, aSize);

#else

    acl_mem_alloc_t *sAllocator = NULL; 

    if (aAllocator == NULL)
    {
        sAllocator = (acl_mem_alloc_t *)gAclMemDefault;
    }
    else
    {
        sAllocator = aAllocator;
    }
 
    if (sAllocator->mAclMemAlloc != NULL)
    {
        return  sAllocator->mAclMemAlloc(sAllocator, aAddr, aSize);
    }
    else
    {
        return  ACP_RC_ENOTSUP;
    }

#endif
}

/**
 * Allocates zero-initialized memory using provided memory allocator instance
 * 
 * @param aAllocator pointer to the allocator instance
 * @param aAddr pointer to variable which will hold pointer 
 * to allocated memory block
 * @param aElements number of elements to be allocated 
 * @param aSize size of element in bytes
 * @return result code
 *
 * If pointer to allocator instance is NULL then default allocator will be used
 *
 * It returns allocator specific return code
 * It returns ACP_RC_ENOTSUP if allocator does not support this function
 */
ACP_EXPORT acp_rc_t aclMemCalloc(acl_mem_alloc_t *aAllocator,
                                 void  **aAddr,
                                 acp_size_t   aElements,
                                 acp_size_t   aSize)
{

#if defined(__STATIC_ANALYSIS_DOING__)

    ACP_UNUSED(aAllocator);

    return acpMemCalloc(aAddr, aElements, aSize);

#else

    acl_mem_alloc_t *sAllocator = NULL; 

    if (aAllocator == NULL)
    {
        sAllocator = (acl_mem_alloc_t *)gAclMemDefault;
    }
    else
    {
        sAllocator = aAllocator;
    }

    if (sAllocator->mAclMemCalloc != NULL)
    {
        return  sAllocator->mAclMemCalloc(sAllocator, aAddr, aElements, aSize);
    }
    else
    {
        return ACP_RC_ENOTSUP;
    }
#endif
}

/**
 * Changes the size of the memory block using provided memory allocator instance
 * 
 * @param aAllocator pointer to the allocator instance
 * @param aAddr pointer to memory block to be reallocated
 * @param aSize size of memory block
 * @return result code
 *
 * If pointer to allocator instance is NULL then default allocator will be used
 *
 * It returns allocator specific return code
 * It returns ACP_RC_ENOTSUP if allocator does not support this function
 */
ACP_EXPORT acp_rc_t aclMemRealloc(acl_mem_alloc_t *aAllocator,
                              void **aAddr,
                              acp_size_t aSize)
{
#if defined(__STATIC_ANALYSIS_DOING__)

    ACP_UNUSED(aAllocator);

    return acpMemRealloc(aAddr, aSize);

#else

    acl_mem_alloc_t *sAllocator = NULL; 

    if (aAllocator == NULL)
    {
        sAllocator = (acl_mem_alloc_t *)gAclMemDefault;
    }
    else
    {
        sAllocator = aAllocator;
    }


    if (sAllocator->mAclMemRealloc != NULL)
    {
        return  sAllocator->mAclMemRealloc(sAllocator, aAddr, aSize);
    }
    else
    {
        return ACP_RC_ENOTSUP;
    }

#endif
}

/**
 * Frees memory using provided memory allocator instance
 * 
 * @param aAllocator pointer to the allocator instance
 * @param aAddr pointer to memory to be freed
 * @return result code
 *
 * If pointer to allocator instance is NULL then default allocator will be used
 *
 * It returns allocator specific return code
 * It returns ACP_RC_ENOTSUP if allocator does not support this function
 */
ACP_EXPORT acp_rc_t aclMemFree(acl_mem_alloc_t *aAllocator, void *aAddr)
{

#if defined(__STATIC_ANALYSIS_DOING__)
    ACP_UNUSED(aAllocator);

    acpMemFree(aAddr);

    return ACP_RC_SUCCESS;

#else

    acl_mem_alloc_t *sAllocator = NULL;

    if (aAllocator == NULL)
    {
        sAllocator = (acl_mem_alloc_t *)gAclMemDefault;
    }
    else
    {
        sAllocator = aAllocator;
    }

    if (sAllocator->mAclMemFree!= NULL)
    {
        return sAllocator->mAclMemFree(sAllocator, aAddr);
    }
    else
    {
        return ACP_RC_ENOTSUP;
    }
#endif
}

/*
 * Implementation of alloc function for ACL_MEM_ALLOC_LIBC allocator
 */
static acp_rc_t aclMemAllocAcp(acl_mem_alloc_t *aHandle,
                               void **aAddr,
                               acp_size_t aSize)
{
    ACP_UNUSED(aHandle);

    return acpMemAlloc(aAddr, aSize);
}

/*
 * Implementation of calloc function for ACL_MEM_ALLOC_LIBC allocator
 */
static acp_rc_t aclMemCallocAcp(acl_mem_alloc_t *aHandle,
                                void  **aAddr,
                                acp_size_t   aElements,
                                acp_size_t   aSize)
{
    ACP_UNUSED(aHandle);

    return acpMemCalloc(aAddr, aElements, aSize);
}

/*
 * Implementation of realloc function for ACL_MEM_ALLOC_LIBC allocator
 */
static acp_rc_t aclMemReallocAcp(acl_mem_alloc_t *aHandle,
                                 void **aAddr,
                                 acp_size_t aSize)
{
    ACP_UNUSED(aHandle);

    return acpMemRealloc(aAddr, aSize);
}

/*
 * Implementation of free function for ACL_MEM_ALLOC_LIBC allocator
 */
static acp_rc_t aclMemFreeAcp(acl_mem_alloc_t *aHandle, void *aAddr)
{
    ACP_UNUSED(aHandle);

    acpMemFree(aAddr);

    return ACP_RC_SUCCESS;
}

static acp_rc_t aclMemControlAcp(acl_mem_alloc_t *aHandle,
                                 acl_mem_alloc_attr_t aAttrName,
                                 void *aAttrValue)
{
    ACP_UNUSED(aHandle);
    ACP_UNUSED(aAttrName);
    ACP_UNUSED(aAttrValue);
    return ACP_RC_ENOTSUP;
}

