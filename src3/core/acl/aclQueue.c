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
 * $Id: aclQueue.c 9884 2010-02-03 06:50:04Z gurugio $
 *
 * Lock-Free FIFO Queue implementation based on:
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
#include <aclQueue.h>


/*
 * PARISC CPU는 spinlock으로 사용하는 atomic operation만 제공되고
 * compare and swap과 같은 atomic operation을 지원하지 않기 때문에
 * compare and swap을 이용하는 lockfree queue가 더 느릴 수 밖에 없다.
 * 그러므로, PARISC CPU에서는 lockfree queue를 사용하지 않도록 한다.
 */
#if !defined(ALTI_CFG_CPU_PARISC)
#define ACL_QUEUE_LOCKFREE_ENABLED
#endif


/*
 * SMR (Safe-Memory Reclamation) Constants
 *
 * P: number of participating threads       = aQueue->mParallelFactor
 * K: number of harzard pointers per thread = ACL_QUEUE_SMR_HP_COUNT
 * R: batch size                            = ACL_QUEUE_SMR_BATCH_SIZE
 */
#define ACL_QUEUE_SMR_HP_COUNT    2
#define ACL_QUEUE_SMR_BATCH_SIZE  128


#define ACL_QUEUE_CAS(aPtr, aWith, aCmp)                                \
    (void *)acpAtomicCas((aPtr), (acp_ulong_t)(aWith), (acp_ulong_t)(aCmp))


struct aclQueueNode
{
    void         *mObj;
    aclQueueNode *mNext;
};

struct aclQueueSmrRec
{
    acp_spin_lock_t  mLock;
    acp_sint32_t     mRetiredCount;
    aclQueueNode    *mRetiredNodes[ACL_QUEUE_SMR_BATCH_SIZE];
    aclQueueNode    *mHazardNodes[ACL_QUEUE_SMR_HP_COUNT];
};


/*
 * SMR Functions
 */
ACP_INLINE void aclQueueNullifyNode(aclQueueNode  *aNodeToNullify,
                                    aclQueueNode **aNodeArray,
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

ACP_INLINE void aclQueueFreeSafeNodes(acl_queue_t    *aQueue,
                                      aclQueueSmrRec *aSmrRec)
{
    aclQueueNode *sNodesToFree[ACL_QUEUE_SMR_BATCH_SIZE];
    acp_sint32_t  sCount;
    acp_sint32_t  i;
    acp_sint32_t  j;

    /*
     * RetiredNodes List를 NodesToFree List로 복사
     */
    acpMemCpy(sNodesToFree, aSmrRec->mRetiredNodes, sizeof(sNodesToFree));

    /*
     * NodesToFree List에서 Hazardous Node들을 Nullify
     */
    ACP_MEM_BARRIER();

    for (i = 0; i < aQueue->mParallelFactor; i++)
    {
        for (j = 0; j < ACL_QUEUE_SMR_HP_COUNT; j++)
        {
            aclQueueNullifyNode(
                aQueue->mSpec.mLockFree.mSmrRec[i].mHazardNodes[j],
                sNodesToFree,
                aSmrRec->mRetiredCount);
        }
    }

    /*
     * NodesToFree List의 Node들을 NodePool로 반환 후 RetiredNodes List에서 제거
     */
    sCount = 0;

    for (i = 0; i < aSmrRec->mRetiredCount; i++)
    {
        if (sNodesToFree[i] != NULL)
        {
            aclMemPoolFree(&aQueue->mNodePool, sNodesToFree[i]);
        }
        else
        {
            aSmrRec->mRetiredNodes[sCount] = aSmrRec->mRetiredNodes[i];
            sCount++;
        }
    }

    aSmrRec->mRetiredCount = sCount;
}

ACP_INLINE void aclQueueRetireNode(acl_queue_t    *aQueue,
                                   aclQueueSmrRec *aSmrRec,
                                   aclQueueNode   *aNode)
{
    if (aSmrRec->mRetiredCount == ACL_QUEUE_SMR_BATCH_SIZE)
    {
        while (1)
        {
            aclQueueFreeSafeNodes(aQueue, aSmrRec);

            if (aSmrRec->mRetiredCount == ACL_QUEUE_SMR_BATCH_SIZE)
            {
                acpThrYield();
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        /* do nothing */
    }

    aSmrRec->mRetiredNodes[aSmrRec->mRetiredCount] = aNode;
    aSmrRec->mRetiredCount++;
}

/*
 * SMR Record List
 */
ACP_INLINE aclQueueSmrRec *aclQueueSmrRecTryLock(acl_queue_t  *aQueue,
                                                 acp_uint32_t  aIndex)
{
    acp_uint32_t i = aIndex;

    do
    {
        if (acpSpinLockTryLock(&aQueue->mSpec.mLockFree.mSmrRec[i].mLock)
            == ACP_TRUE)
        {
            return &aQueue->mSpec.mLockFree.mSmrRec[i];
        }
        else
        {
            /* do nothing */
        }

        i = (i + 1) % aQueue->mParallelFactor;
    } while (i != aIndex);

    return NULL;
}

ACP_INLINE aclQueueSmrRec *aclQueueSmrRecLock(acl_queue_t *aQueue)
{
    aclQueueSmrRec *sSmrRec = NULL;
    acp_uint32_t    sIndex = acpThrGetParallelIndex() % aQueue->mParallelFactor;

    ACP_SPIN_WAIT((sSmrRec = aclQueueSmrRecTryLock(aQueue, sIndex)) != NULL, 0);

    return sSmrRec;
}

ACP_INLINE void aclQueueSmrRecUnlock(aclQueueSmrRec *aSmrRec)
{
    acpSpinLockUnlock(&aSmrRec->mLock);
}


static acp_rc_t aclQueueEnqueueLockFree(acl_queue_t *aQueue, void *aObj)
{
    aclQueueSmrRec *sSmrRec = NULL;
    aclQueueNode   *sTail = NULL;
    aclQueueNode   *sNext = NULL;
    aclQueueNode   *sNode = NULL;
    acp_rc_t        sRC;

    sRC = aclMemPoolAlloc(&aQueue->mNodePool, (void **)&sNode);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        sNode->mObj  = aObj;
        sNode->mNext = NULL;
    }

    sSmrRec = aclQueueSmrRecLock(aQueue);

    while (1)
    {
        sTail = aQueue->mTail;

        sSmrRec->mHazardNodes[0] = sTail;

        ACP_MEM_BARRIER();

        if (sTail != aQueue->mTail)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sNext = sTail->mNext;

        if (sTail == aQueue->mTail)
        {
            if (sNext == NULL)
            {
                if (ACL_QUEUE_CAS(&sTail->mNext, sNode, NULL) == NULL)
                {
                    break;
                }
                else
                {
                    /* do nothing */
                }
            }
            else
            {
                (void)ACL_QUEUE_CAS(&aQueue->mTail, sNext, sTail);
            }
        }
        else
        {
            /* do nothing */
        }
    }

    (void)ACL_QUEUE_CAS(&aQueue->mTail, sNode, sTail);

    sSmrRec->mHazardNodes[0] = NULL;

    aclQueueSmrRecUnlock(sSmrRec);

    (void)acpAtomicInc32((void*)&(aQueue->mNodeCount));

#if (__STATIC_ANALYSIS_DOING__)
    /*
     * Removing Codesonar warning: Double-Unlock
     */
    aclMemPoolFree(&aQueue->mNodePool, sNode);
#endif

    return ACP_RC_SUCCESS;
}

static acp_rc_t aclQueueDequeueLockFree(acl_queue_t *aQueue, void **aObj)
{
    aclQueueSmrRec *sSmrRec = NULL;
    aclQueueNode   *sHead = NULL;
    aclQueueNode   *sTail = NULL;
    aclQueueNode   *sNext = NULL;

    /*
     * BUGBUG: This is LOCK!
     * We should change SMR data into lock-free linked list
     * as implementation of paper: "Hazard Pointers:
     * Safe Memory Reclamation for Lock-Free Objects."
     */
    sSmrRec = aclQueueSmrRecLock(aQueue);

    while (1)
    {
        sHead = aQueue->mHead;

        sSmrRec->mHazardNodes[0] = sHead;

        ACP_MEM_BARRIER();

        if (sHead != aQueue->mHead)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sTail = aQueue->mTail;
        sNext = sHead->mNext;

        sSmrRec->mHazardNodes[1] = sNext;

        ACP_MEM_BARRIER();

        if (sHead == aQueue->mHead)
        {
            if (sNext == NULL)
            {
                sSmrRec->mHazardNodes[0] = NULL;
                aclQueueSmrRecUnlock(sSmrRec);
                return ACP_RC_ENOENT;
            }
            else
            {
                if (sHead != sTail)
                {
                    *aObj = sNext->mObj;

                    if (ACL_QUEUE_CAS(&aQueue->mHead, sNext, sHead) == sHead)
                    {
                        break;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    (void)ACL_QUEUE_CAS(&aQueue->mTail, sNext, sTail);
                }
            }
        }
        else
        {
            /* do nothing */
        }
    }

    sSmrRec->mHazardNodes[0] = NULL;
    sSmrRec->mHazardNodes[1] = NULL;

    aclQueueRetireNode(aQueue, sSmrRec, sHead);

    aclQueueSmrRecUnlock(sSmrRec);
    (void)acpAtomicDec32((void*)&(aQueue->mNodeCount));

    return ACP_RC_SUCCESS;
}

static acp_rc_t aclQueueEnqueueSpinLock(acl_queue_t *aQueue, void *aObj)
{
    aclQueueNode *sNode = NULL;
    acp_rc_t      sRC;

    sRC = aclMemPoolAlloc(&aQueue->mNodePool, (void **)&sNode);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        sNode->mObj  = aObj;
        sNode->mNext = NULL;
    }

    acpSpinLockLock(&aQueue->mSpec.mSpinLock.mTailLock);

    aQueue->mTail->mNext = sNode;
    aQueue->mTail        = sNode;
    aQueue->mNodeCount++;

    acpSpinLockUnlock(&aQueue->mSpec.mSpinLock.mTailLock);

    return ACP_RC_SUCCESS;
}

static acp_rc_t aclQueueDequeueSpinLock(acl_queue_t *aQueue, void **aObj)
{
    aclQueueNode *sNode = NULL;
    aclQueueNode *sNewHead = NULL;

    acpSpinLockLock(&aQueue->mSpec.mSpinLock.mHeadLock);

    sNode    = aQueue->mHead;
    sNewHead = sNode->mNext;

    if (sNewHead == NULL)
    {
        acpSpinLockUnlock(&aQueue->mSpec.mSpinLock.mHeadLock);

        return ACP_RC_ENOENT;
    }
    else
    {
        *aObj          = sNewHead->mObj;
        aQueue->mHead  = sNewHead;
        aQueue->mNodeCount--;

        acpSpinLockUnlock(&aQueue->mSpec.mSpinLock.mHeadLock);

        aclMemPoolFree(&aQueue->mNodePool, sNode);

        return ACP_RC_SUCCESS;
    }
}

static acp_rc_t aclQueueEnqueueNoLock(acl_queue_t *aQueue, void *aObj)
{
    aclQueueNode *sNode = NULL;
    acp_rc_t      sRC;

    sRC = aclMemPoolAlloc(&aQueue->mNodePool, (void **)&sNode);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        sNode->mObj  = aObj;
        sNode->mNext = NULL;
    }

    aQueue->mTail->mNext = sNode;
    aQueue->mTail        = sNode;
    (void)acpAtomicInc32((void*)&(aQueue->mNodeCount));

    return ACP_RC_SUCCESS;
}

static acp_rc_t aclQueueDequeueNoLock(acl_queue_t *aQueue, void **aObj)
{
    aclQueueNode *sNode    = aQueue->mHead;
    aclQueueNode *sNewHead = sNode->mNext;

    if (sNewHead == NULL)
    {
        return ACP_RC_ENOENT;
    }
    else
    {
        *aObj          = sNewHead->mObj;
        aQueue->mHead  = sNewHead;

        (void)acpAtomicDec32((void*)&(aQueue->mNodeCount));
        aclMemPoolFree(&aQueue->mNodePool, sNode);

        return ACP_RC_SUCCESS;
    }
}

static aclQueueOp gAclQueueOpLockFree =
{
    aclQueueEnqueueLockFree,
    aclQueueDequeueLockFree,
};

static aclQueueOp gAclQueueOpSpinLock =
{
    aclQueueEnqueueSpinLock,
    aclQueueDequeueSpinLock,
};

static aclQueueOp gAclQueueOpNoLock =
{
    aclQueueEnqueueNoLock,
    aclQueueDequeueNoLock,
};


ACP_INLINE acp_rc_t aclQueueInitLockFree(acl_queue_t *aQueue)
{
    acp_rc_t     sRC;
    acp_sint32_t i;

    aQueue->mOp = &gAclQueueOpLockFree;

    sRC = acpMemCalloc((void **)&aQueue->mSpec.mLockFree.mSmrRec,
                       aQueue->mParallelFactor,
                       sizeof(aclQueueSmrRec));

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    for (i = 0; i < aQueue->mParallelFactor; i++)
    {
        acpSpinLockInit(&aQueue->mSpec.mLockFree.mSmrRec[i].mLock, 0);
    }

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t aclQueueInitSpinLock(acl_queue_t *aQueue)
{
    aQueue->mOp = &gAclQueueOpSpinLock;

    acpSpinLockInit(&aQueue->mSpec.mSpinLock.mHeadLock, -1);
    acpSpinLockInit(&aQueue->mSpec.mSpinLock.mTailLock, -1);

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t aclQueueInitNoLock(acl_queue_t *aQueue)
{
    aQueue->mOp = &gAclQueueOpNoLock;

    return ACP_RC_SUCCESS;
}

ACP_INLINE void aclQueueFinalLockFree(acl_queue_t *aQueue)
{
    acp_sint32_t  i;
    acp_sint32_t  j;

    for (i = 0; i < aQueue->mParallelFactor; i++)
    {
        for (j = 0;
             j < aQueue->mSpec.mLockFree.mSmrRec[i].mRetiredCount;
             j++)
        {
            aclMemPoolFree(
                &aQueue->mNodePool,
                aQueue->mSpec.mLockFree.mSmrRec[i].mRetiredNodes[j]);
        }
    }

    acpMemFree(aQueue->mSpec.mLockFree.mSmrRec);
}

ACP_INLINE void aclQueueFinalSpinLock(acl_queue_t *aQueue)
{
    ACP_UNUSED(aQueue);
}

ACP_INLINE void aclQueueFinalNoLock(acl_queue_t *aQueue)
{
    ACP_UNUSED(aQueue);
}


/**
 * creates a FIFO queue
 * @param aQueue pointer to the queue
 * @param aParallelFactor parallel factor
 * @return result code
 *
 * <table border="1" cellspacing="1" cellpadding="4">
 * <tr><th>@a aParallelFactor</th><th>MT-Safe</th><th>Queue Algorithm</th></tr>
 * <tr><td>0</td><td>NO</td><td>No Lock</td></tr>
 * <tr><td>1</td><td>YES</td><td>Spin Lock</td></tr>
 * <tr><td>&gt;1</td><td>YES</td><td>Lock Free (Spin Lock in PARISC)</td></tr>
 * </table>
 *
 * if @a aParallelFactor is less than zero,
 * it will take system CPU count to determine @a aParallelFactor.
 */
ACP_EXPORT acp_rc_t aclQueueCreate(acl_queue_t      *aQueue,
                                   acp_sint32_t      aParallelFactor)
{
    aclQueueNode *sNode = NULL;
    acp_rc_t      sRC;

    if (aParallelFactor < 0)
    {
        sRC = acpSysGetCPUCount((acp_uint32_t *)&aQueue->mParallelFactor);

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
        aQueue->mParallelFactor = aParallelFactor;
    }

    sRC = aclMemPoolCreate(&aQueue->mNodePool,
                           sizeof(aclQueueNode),
                           128,
                           aQueue->mParallelFactor);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRC = aclMemPoolAlloc(&aQueue->mNodePool, (void **)&sNode);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        aclMemPoolDestroy(&aQueue->mNodePool);
        return sRC;
    }
    else
    {
        sNode->mObj  = NULL;
        sNode->mNext = NULL;

        aQueue->mHead = sNode;
        aQueue->mTail = sNode;
    }

    if (aQueue->mParallelFactor > 1)
    {
#if defined(ACL_QUEUE_LOCKFREE_ENABLED)
        sRC = aclQueueInitLockFree(aQueue);
#else
        sRC = aclQueueInitSpinLock(aQueue);
#endif
    }
    else if (aQueue->mParallelFactor == 1)
    {
        sRC = aclQueueInitSpinLock(aQueue);
    }
    else
    {
        sRC = aclQueueInitNoLock(aQueue);
    }

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        aclMemPoolFree(&aQueue->mNodePool, sNode);
        aclMemPoolDestroy(&aQueue->mNodePool);
    }
    else
    {
        aQueue->mNodeCount = 0;
    }

    return sRC;
}

/**
 * destroys a queue
 * @param aQueue pointer to the queue
 *
 * it deallocated all memory allocated by the queue.
 */
ACP_EXPORT void aclQueueDestroy(acl_queue_t *aQueue)
{
#if defined(ACP_CFG_DEBUG) || defined(ACP_CFG_MEMORY_CHECK)
    void         *sObj = NULL;
    acp_rc_t      sRC;

    do
    {
        sRC = aclQueueDequeue(aQueue, &sObj);
    } while (ACP_RC_IS_SUCCESS(sRC));

    ACE_DASSERT(aclQueueIsEmpty(aQueue) == ACP_TRUE);
#endif

    if (aQueue->mParallelFactor > 1)
    {
#if defined(ACL_QUEUE_LOCKFREE_ENABLED)
        aclQueueFinalLockFree(aQueue);
#else
        aclQueueFinalSpinLock(aQueue);
#endif
    }
    else if (aQueue->mParallelFactor == 1)
    {
        aclQueueFinalSpinLock(aQueue);
    }
    else
    {
        aclQueueFinalNoLock(aQueue);
    }

    aclMemPoolFree(&aQueue->mNodePool, aQueue->mHead);
    aclMemPoolDestroy(&aQueue->mNodePool);
}

/**
 * checks whether the queue is empty or not
 * @param aQueue pointer to the queue
 * @return #ACP_TRUE if the queue is empty, otherwise #ACP_FALSE.
 */
ACP_EXPORT acp_bool_t aclQueueIsEmpty(acl_queue_t *aQueue)
{
    return (aQueue->mHead->mNext == NULL) ? ACP_TRUE : ACP_FALSE;
}
