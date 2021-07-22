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
 * $Id: aclQueue.h 3773 2008-11-28 09:05:43Z djin $
 ******************************************************************************/

#if !defined(_O_ACL_QUEUE_H_)
#define _O_ACL_QUEUE_H_

/**
 * @file
 * @ingroup CoreCollection
 *
 * FIFO(first-in first-out) Queue
 *
 * aclQueue is unbounded (no limitation of the number of queued object) queue.
 *
 * queued object의 갯수에 대한 제한을 줄 수 있고 현재 queued object의 갯수를
 * 얻는 함수를 만들 계획이어으나 lockfree queue 는 queue data structure전체에
 * 대한 lock을 잡지 않기 때문에 counter의 증감을 atomic operation으로
 * 처리하더라도 일시적으로 잘못된 값이 들어갈 수 있다.
 *
 * 예를 들면, enqueue와 dequeue가 순서대로 처리되고 counter값은
 * dequeue, enqueue순서로 처리되면 일시적으로 counter는 음수가 될 수 있다.
 *
 * 또한, atomic increase/decrease가 atomic operation이 성공할 때까지 반복하는
 * 성격의 operation이므로 성능에도 영향을 미칠 수 있다.
 *
 * 그러므로, aclQueue는 이에 대한 처리를 하지 않도록 하였다.
 */

#include <acpSpinLock.h>
#include <aclMemPool.h>


ACP_EXTERN_C_BEGIN


/**
 * queue object
 */
typedef struct acl_queue_t acl_queue_t;


typedef struct aclQueueNode   aclQueueNode;
typedef struct aclQueueSmrRec aclQueueSmrRec;

typedef struct aclQueueOp
{
    acp_rc_t (*mEnqueue)(acl_queue_t *aQueue, void *aObj);
    acp_rc_t (*mDequeue)(acl_queue_t *aQueue, void **aObj);
} aclQueueOp;

typedef struct aclQueueLockFree
{
    aclQueueSmrRec *mSmrRec;
} aclQueueLockFree;

typedef struct aclQueueSpinLock
{
    acp_spin_lock_t mHeadLock;
    acp_spin_lock_t mTailLock;
} aclQueueSpinLock;

struct acl_queue_t
{
    union
    {
        aclQueueLockFree  mLockFree;
        aclQueueSpinLock  mSpinLock;
    } mSpec;

    acl_mem_pool_t        mNodePool;
    acp_sint32_t          mParallelFactor;
    aclQueueOp           *mOp;
    aclQueueNode         *mHead;
    aclQueueNode         *mTail;

    volatile acp_sint32_t          mNodeCount;
};


ACP_EXPORT acp_rc_t   aclQueueCreate(acl_queue_t  *aQueue,
                                     acp_sint32_t  aParallelFactor);
ACP_EXPORT void       aclQueueDestroy(acl_queue_t *aQueue);

ACP_EXPORT acp_bool_t aclQueueIsEmpty(acl_queue_t *aQueue);

/**
 * enqueues an object to the queue
 * @param aQueue pointer to the queue
 * @param aObj pointer to the object to enqueue
 * @return result code
 */
ACP_INLINE acp_rc_t aclQueueEnqueue(acl_queue_t *aQueue, void *aObj)
{
    return aQueue->mOp->mEnqueue(aQueue, aObj);
}

/**
 * dequeues an object from the queue
 * @param aQueue pointer to the queue
 * @param aObj pointer to the variable to store dequeued object
 * @return result code
 *
 * it returns #ACP_RC_ENOENT if the queue is empty
 */
ACP_INLINE acp_rc_t aclQueueDequeue(acl_queue_t *aQueue, void **aObj)
{
    return aQueue->mOp->mDequeue(aQueue, aObj);
}

ACP_INLINE acp_sint32_t aclQueueGetCount(acl_queue_t* aQueue)
{
    acp_sint32_t sCount = aQueue->mNodeCount;
    return (sCount < 0)? 0 : sCount;
}

ACP_EXTERN_C_END


#endif
