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
 *
 * Lock-Free LIFO stack implementation based on:
 *
 * Maged M. Michael
 * Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects.
 * IEEE TPDS, 2004
 *
 * Maged M. Michael
 * Safe Memory Reclamation for Dynamic Lock-Free Objects
 * Using Atomic Read and Writes.
 * ACM PODC, 2002
 *
 * Maged M. Michael, Michael L. Scott
 * Simple, Fast, and Practical Non-Blocking and Blocking
 * Concurrent Queue Algorithms.
 * ACM PODC, 1996
 ******************************************************************************/

#include <acpAtomic.h>
#include <acpSys.h>
#include <acpThr.h>
#include <aceAssert.h>
#include <aclStack.h>


/*
 * PARISC CPU does not support CAS(compare-and-swap) operation
 * so that lockfree algorithm with CAS would be slow.
 * Therefore we use spinlock instead for PARISC CPU.
 */
#if !defined(ALTI_CFG_CPU_PARISC)
#define ACL_STACK_LOCKFREE_ENABLED
#endif


#define ACL_STACK_CAS(aPtr, aWith, aCmp)                                \
    (void *)acpAtomicCas((aPtr), (acp_ulong_t)(aWith), (acp_ulong_t)(aCmp))


/*
 * SMR Functions
 */
ACP_INLINE void aclStackNullifyNode(aclStackNode  *aNodeToNullify,
                                    aclStackNode **aNodeArray,
                                    acp_sint32_t   aNodeCount)
{
    acp_sint32_t i;

    if (aNodeToNullify != NULL)
    {
        for (i = 0; i < aNodeCount; i++)
        {
            if (aNodeArray[i] == aNodeToNullify)
            {
                aNodeArray[i] = NULL;
                break;
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclStackFreeSafeNodes(acl_stack_t    *aStack)
{
    aclStackNode *sNodesToFree[ACL_STACK_SMR_BATCH_SIZE];
    acp_sint32_t  sCount;
    acp_sint32_t  i;

    /*
     * RetireNodes List를 NodesToFree List로 복사
     */
    acpMemCpy(sNodesToFree,
              aStack->mSpec.mLockFree.mRetireNodes,
              sizeof(sNodesToFree));

    /*
     * NodesToFree List에서 Hazardous Node들을 Nullify
     */
    ACP_MEM_BARRIER();

    for (i = 0; i < ACL_STACK_SMR_HP_COUNT; i++)
    {
        aclStackNullifyNode(aStack->mSpec.mLockFree.mHazardNodes[i].mNode,
                            sNodesToFree,
                            ACL_STACK_SMR_BATCH_SIZE);
    }

    /*
     * NodesToFree List의 Node들을 NodePool로 반환 후 RetireNodes List에서 제거
     */
    sCount = 0;

    for (i = 0; i < aStack->mSpec.mLockFree.mRetireCount; i++)
    {
        if (sNodesToFree[i] != NULL)
        {
            aclMemPoolFree(&aStack->mNodePool, sNodesToFree[i]);
        }
        else
        {
            aStack->mSpec.mLockFree.mRetireNodes[sCount] =
                aStack->mSpec.mLockFree.mRetireNodes[i];
            sCount++;
        }
    }

    aStack->mSpec.mLockFree.mRetireCount = sCount;
}


void aclStackRetireNode(acl_stack_t *aStack, aclStackNode *aNode)
{

    /* 원래는 쓰레드 로컬 스토리지에 리스트를 저장해야하는데
       편의를 위해서 락기반 전역 리스트로 만듬 */
    acpSpinLockLock(&aStack->mSpec.mLockFree.mRetireLock);

    while (aStack->mSpec.mLockFree.mRetireCount == ACL_STACK_SMR_BATCH_SIZE)
    {
        aclStackFreeSafeNodes(aStack);

        /* fail to free unused nodes
           because other threads can hold retired node. */
        if (aStack->mSpec.mLockFree.mRetireCount == ACL_STACK_SMR_BATCH_SIZE)
        {
            acpSpinLockUnlock(&aStack->mSpec.mLockFree.mRetireLock);

            /* let other threads free unused nodes */
            acpThrYield();

            acpSpinLockLock(&aStack->mSpec.mLockFree.mRetireLock);
        }
        else
        {
            /* break while loop */
        }
    }
    
    aStack->mSpec.mLockFree.mRetireNodes
        [aStack->mSpec.mLockFree.mRetireCount] = aNode;
    aStack->mSpec.mLockFree.mRetireCount++;
    
    acpSpinLockUnlock(&aStack->mSpec.mLockFree.mRetireLock);
}

#define ACL_STACK_GET_HP_RETRY 100

ACP_INLINE aclStackSmrRec *aclStackGetSmrRec(acl_stack_t *aStack)
{
    volatile acp_sint32_t sTrue = ACP_TRUE;
    volatile acp_sint32_t sFalse = ACP_FALSE;
    acp_sint32_t i;
    acp_sint32_t sTry;

    for (sTry = ACL_STACK_GET_HP_RETRY; sTry > 0; sTry--)
    {
        for (i = 0; i < ACL_STACK_SMR_HP_COUNT; i++)
        {
            if (aStack->mSpec.mLockFree.mHazardNodes[i].mActive == ACP_FALSE &&
                (acp_sint32_t)ACL_STACK_CAS(
                    &aStack->mSpec.mLockFree.mHazardNodes[i].mActive,
                    sTrue,
                    sFalse) == sFalse)
            {
                return &aStack->mSpec.mLockFree.mHazardNodes[i];
            }
            else
            {
                /* do nothing */
            }
        }

        acpThrYield();
    }

    return NULL;
}

ACP_INLINE void aclStackPutSmrRec(aclStackSmrRec *aSmrRec)
{
    volatile acp_sint32_t sTrue = ACP_TRUE; 
    volatile acp_sint32_t sFalse = ACP_FALSE;

    ACE_DASSERT(aSmrRec != NULL);
    
    aSmrRec->mNode = NULL;
    (void)ACL_STACK_CAS(&aSmrRec->mActive, sFalse, sTrue);
}


static acp_rc_t aclStackPopLockFree(acl_stack_t *aStack, void **aObj)
{
    aclStackSmrRec *sSmrRec = aclStackGetSmrRec(aStack);
    aclStackNode   *sTop = NULL;
    aclStackNode   *sNext = NULL;
    
    if (sSmrRec == NULL)
    {
        return ACP_RC_EAGAIN;
    }
    else
    {
        /* go ahead */
    }
    
    while (1)
    {
        sTop = aStack->mTop;

        if (sTop == NULL)
        {
            aclStackPutSmrRec(sSmrRec);
            return ACP_RC_ENOENT;
        }
        else
        {
            /* do nothing */
        }

        sSmrRec->mNode = sTop;

        ACP_MEM_BARRIER();

        /* top이 다른 쓰레드에의해 바꼈다.
           즉 HP에 top을 등록하기 직전에 sTop을 다른 쓰레드가 바꾸고
           sTop이 가리키는 노드가 해지된 노드일 수 도 있다.
           그러면 top을 HP에 저장해서 보호하면 안된다.
           top은 이미 틀린 정보이기 때문이다.
           따라서 HP에 등록한 직후에 sTop을 다시한번 확인하는 것이다.
         */
           
        if (sTop != aStack->mTop)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sNext = sTop->mNext;

        ACP_MEM_BARRIER();


        if (ACL_STACK_CAS(&aStack->mTop, sNext, sTop) == sTop)
        {
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    *aObj = sTop->mObj;

    /* 이제 노드를 해지해도 된다. */
    aclStackPutSmrRec(sSmrRec);
    aclStackRetireNode(aStack, sTop);

    (void)acpAtomicDec32((void*)&(aStack->mNodeCount));

    return ACP_RC_SUCCESS;
}


acp_rc_t aclStackPushLockFree(acl_stack_t *aStack, void *aObj)
{
    aclStackNode   *sTop = NULL;
    aclStackNode   *sNode = NULL;
    acp_rc_t        sRC;

    sRC = aclMemPoolAlloc(&aStack->mNodePool, (void **)&sNode);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
    }

    while (1)
    {
        sTop = aStack->mTop;

        sNode->mObj  = aObj;
        sNode->mNext = sTop;

        ACP_MEM_BARRIER();

        if (ACL_STACK_CAS(&aStack->mTop, sNode, sTop) == sTop)
        {
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    (void)acpAtomicInc32((void*)&(aStack->mNodeCount));

#if (__STATIC_ANALYSIS_DOING__)
    /*
     * Removing Codesonar warning: Double-Unlock
     */
    aclMemPoolFree(&aStack->mNodePool, sNode);
#endif

    return ACP_RC_SUCCESS;
}


static acp_rc_t aclStackPushSpinLock(acl_stack_t *aStack, void *aObj)
{
    aclStackNode *sNode = NULL;
    acp_rc_t      sRC;

    sRC = aclMemPoolAlloc(&aStack->mNodePool, (void **)&sNode);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        sNode->mObj  = aObj;
    }

    acpSpinLockLock(&aStack->mSpec.mSpinLock.mTopLock);

    sNode->mNext = aStack->mTop;
    aStack->mTop = sNode;

    acpSpinLockUnlock(&aStack->mSpec.mSpinLock.mTopLock);

    (void)acpAtomicInc32((void*)&(aStack->mNodeCount));

    return ACP_RC_SUCCESS;
}

static acp_rc_t aclStackPopSpinLock(acl_stack_t *aStack, void **aObj)
{
    aclStackNode *sNode = NULL;

    acpSpinLockLock(&aStack->mSpec.mSpinLock.mTopLock);

    sNode    = aStack->mTop;

    if (aStack->mTop == NULL)
    {
        acpSpinLockUnlock(&aStack->mSpec.mSpinLock.mTopLock);

        return ACP_RC_ENOENT;
    }
    else
    {
        aStack->mTop  = aStack->mTop->mNext;

        acpSpinLockUnlock(&aStack->mSpec.mSpinLock.mTopLock);

        (void)acpAtomicDec32((void*)&(aStack->mNodeCount));
        *aObj = sNode->mObj;

        aclMemPoolFree(&aStack->mNodePool, sNode);

        return ACP_RC_SUCCESS;
    }
}

static acp_rc_t aclStackPushNoLock(acl_stack_t *aStack, void *aObj)
{
    aclStackNode *sNode = NULL;
    acp_rc_t      sRC;

    sRC = aclMemPoolAlloc(&aStack->mNodePool, (void **)&sNode);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        sNode->mObj  = aObj;
    }

    sNode->mNext = aStack->mTop;
    aStack->mTop = sNode;

    (void)acpAtomicInc32((void*)&(aStack->mNodeCount));

    return ACP_RC_SUCCESS;
}

static acp_rc_t aclStackPopNoLock(acl_stack_t *aStack, void **aObj)
{
    aclStackNode *sNode = aStack->mTop;

    if (aStack->mTop == NULL)
    {
        return ACP_RC_ENOENT;
    }
    else
    {
        *aObj = aStack->mTop->mObj;
        aStack->mTop  = aStack->mTop->mNext;

        (void)acpAtomicDec32((void*)&(aStack->mNodeCount));
        
        aclMemPoolFree(&aStack->mNodePool, sNode);

        return ACP_RC_SUCCESS;
    }
}

static aclStackOp gAclQueueOpLockFree =
{
    aclStackPushLockFree,
    aclStackPopLockFree,
};

static aclStackOp gAclQueueOpSpinLock =
{
    aclStackPushSpinLock,
    aclStackPopSpinLock,
};

static aclStackOp gAclQueueOpNoLock =
{
    aclStackPushNoLock,
    aclStackPopNoLock,
};

ACP_INLINE acp_rc_t aclStackInitLockFree(acl_stack_t *aStack)
{
    acp_sint32_t i;

    for (i = 0; i < ACL_STACK_SMR_HP_COUNT; i++)
    {
        aStack->mSpec.mLockFree.mHazardNodes[i].mActive = ACP_FALSE;
        aStack->mSpec.mLockFree.mHazardNodes[i].mNode = NULL;
    }
            
    acpSpinLockInit(&aStack->mSpec.mLockFree.mRetireLock, 0);
    aStack->mSpec.mLockFree.mRetireCount = 0;
        
    aStack->mOp = &gAclQueueOpLockFree;
    
    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t aclStackInitSpinLock(acl_stack_t *aStack)
{
    aStack->mOp = &gAclQueueOpSpinLock;

    acpSpinLockInit(&aStack->mSpec.mSpinLock.mTopLock, -1);

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t aclStackInitNoLock(acl_stack_t *aStack)
{
    aStack->mOp = &gAclQueueOpNoLock;

    return ACP_RC_SUCCESS;
}


ACP_INLINE void aclStackFinalLockFree(acl_stack_t *aStack)
{
    acp_sint32_t  i;
   
    /* clean retire-nodes*/    
    for (i = 0; i < aStack->mSpec.mLockFree.mRetireCount; i++)
    {
        if (aStack->mSpec.mLockFree.mRetireNodes[i] != NULL)
        {
            aclMemPoolFree(&aStack->mNodePool,
                           aStack->mSpec.mLockFree.mRetireNodes[i]);
        }
        else
        {
            /* do nothing */
        }
    }
}

ACP_INLINE void aclStackFinalSpinLock(acl_stack_t *aStack)
{
    ACP_UNUSED(aStack);
}

ACP_INLINE void aclStackFinalNoLock(acl_stack_t *aStack)
{
    ACP_UNUSED(aStack);
}

/**
 * creates a LIFO stack
 * @param aStack pointer to the stack
 * @param aParallelFactor parallel factor
 * @return result code
 *
 * if @a aParallelFactor is less than zero,
 * it will take system CPU count to determine @a aParallelFactor.
 */
ACP_EXPORT acp_rc_t aclStackCreate(acl_stack_t      *aStack,
                                   acp_sint32_t      aParallelFactor)
{
    acp_rc_t      sRC;

    if (aParallelFactor < 0)
    {
        sRC = acpSysGetCPUCount((acp_uint32_t *)&aStack->mParallelFactor);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        aStack->mParallelFactor = aParallelFactor;
    }

    sRC = aclMemPoolCreate(&aStack->mNodePool,
                           sizeof(aclStackNode),
                           128,
                           aStack->mParallelFactor);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    aStack->mTop = NULL;


    /* BUGBUG: why does the parallel-factor detemine locking method? */
    if (aStack->mParallelFactor > 1)
    {
#if defined(ACL_STACK_LOCKFREE_ENABLED)
        sRC = aclStackInitLockFree(aStack);
#else
        sRC = aclStackInitSpinLock(aStack);
#endif
    }
    else if (aStack->mParallelFactor == 1)
    {
        sRC = aclStackInitSpinLock(aStack);
    }
    else
    {
        sRC = aclStackInitNoLock(aStack);
    }

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        aclMemPoolDestroy(&aStack->mNodePool);
    }
    else
    {
        aStack->mNodeCount = 0;
    }

    return sRC;
}

/**
 * destroys a stack
 * @param aStack pointer to the stack
 *
 * it deallocated all memory allocated by the queue.
 */
ACP_EXPORT void aclStackDestroy(acl_stack_t *aStack)
{
#if defined(ACP_CFG_DEBUG) || defined(ACP_CFG_MEMORY_CHECK)
    void         *sObj = NULL;
    acp_rc_t      sRC;

    do
    {
        sRC = aclStackPop(aStack, &sObj);
    } while (ACP_RC_IS_SUCCESS(sRC));

    ACE_DASSERT(aclStackIsEmpty(aStack) == ACP_TRUE);

#endif

    if (aStack->mParallelFactor > 1)
    {
#if defined(ACL_STACK_LOCKFREE_ENABLED)
        aclStackFinalLockFree(aStack);
#else
        aclStackFinalSpinLock(aStack);
#endif
    }
    else if (aStack->mParallelFactor == 1)
    {
        aclStackFinalSpinLock(aStack);
    }
    else
    {
        aclStackFinalNoLock(aStack);
    }


    aclMemPoolDestroy(&aStack->mNodePool);
}

/**
 * checks whether the stack is empty or not
 * @param aStack pointer to the stack
 * @return #ACP_TRUE if the stack is empty, otherwise #ACP_FALSE.
 */
ACP_EXPORT acp_bool_t aclStackIsEmpty(acl_stack_t *aStack)
{
    return (aStack->mTop == NULL) ? ACP_TRUE : ACP_FALSE;
}
