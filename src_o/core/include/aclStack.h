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
 * $Id: aclStack.h 3773 2008-11-28 09:05:43Z djin $
 ******************************************************************************/

#if !defined(_O_ACL_STACK_H_)
#define _O_ACL_STACK_H_

/**
 * @file
 * @ingroup CoreCollection
 *
 * LIFO(first-in first-out) Stack
 *
 * aclStack is unbounded (no limitation of the number of stacked object) stack.
 *
 */

#include <acpSpinLock.h>
#include <aclMemPool.h>


ACP_EXTERN_C_BEGIN

/*
 * SMR (Safe-Memory Reclamation) Constants
 * (configuration for hazard pointers)
 *
 * P: number of participating threads       = aStack->mParallelFactor
 * K: number of harzard pointers per thread = ACL_STACK_SMR_HP_COUNT
 * R: batch size                            = ACL_STACK_SMR_BATCH_SIZE
 */
#define ACL_STACK_SMR_HP_COUNT    256
#define ACL_STACK_SMR_BATCH_SIZE  256



/**
 * stack object
 */
typedef struct aclStackNode
{
    void         *mObj;
    struct aclStackNode *mNext;
} aclStackNode;

typedef struct aclStackSmrRec
{
    volatile acp_sint32_t mActive;
    aclStackNode *mNode;
} aclStackSmrRec;

typedef struct aclStackLockFree
{
    aclStackSmrRec mHazardNodes[ACL_STACK_SMR_HP_COUNT];
    acp_spin_lock_t mRetireLock;
    aclStackNode *mRetireNodes[ACL_STACK_SMR_BATCH_SIZE];
    acp_sint32_t mRetireCount;
} aclStackLockFree;

typedef struct aclStackSpinLock
{
    acp_spin_lock_t mTopLock;
} aclStackSpinLock;

typedef struct acl_stack_t
{
    union
    {
        aclStackLockFree  mLockFree;
        aclStackSpinLock  mSpinLock;
    } mSpec;

    acl_mem_pool_t        mNodePool;
    acp_sint32_t          mParallelFactor;
    struct aclStackOp    *mOp;
    aclStackNode         *mTop;

    volatile acp_sint32_t          mNodeCount;
} acl_stack_t;


typedef struct aclStackOp
{
    acp_rc_t (*mPush)(struct acl_stack_t *aStack, void *aObj);
    acp_rc_t (*mPop)(struct acl_stack_t *aStack, void **aObj);
} aclStackOp;


ACP_EXPORT acp_rc_t   aclStackCreate(acl_stack_t  *aStack,
                                     acp_sint32_t  aParallelFactor);
ACP_EXPORT void       aclStackDestroy(acl_stack_t *aStack);

ACP_EXPORT acp_bool_t aclStackIsEmpty(acl_stack_t *aStack);

/**
 * push an object to the stack
 * @param aStack pointer to the stack
 * @param aObj pointer to the object to push
 * @return result code
 */
ACP_INLINE acp_rc_t aclStackPush(acl_stack_t *aStack, void *aObj)
{
    return aStack->mOp->mPush(aStack, aObj);
}

/**
 * pop an object from the stack
 * @param aStack pointer to the stack
 * @param aObj pointer to the variable to store poped object
 * @return result code
 *
 * it returns #ACP_RC_ENOENT if the stack is empty
 */
ACP_INLINE acp_rc_t aclStackPop(acl_stack_t *aStack, void **aObj)
{
    return aStack->mOp->mPop(aStack, aObj);
}

ACP_INLINE acp_sint32_t aclStackGetCount(acl_stack_t* aStack)
{
    acp_sint32_t sCount = aStack->mNodeCount;
    return (sCount < 0)? 0 : sCount;
}

ACP_EXTERN_C_END


#endif
