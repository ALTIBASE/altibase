/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduQueueDualLock.cpp 67616 2014-11-19 05:59:37Z djin $
 ****************************************************************************/

#include <idu.h>


typedef struct iduQueueDualLock
{
    UInt   volatile mHead;
    UInt   volatile mTail;

    iduMutex        mHeadMutex;
    iduMutex        mTailMutex;

    void * volatile mObject[1];
} iduQueueDualLock;


IDE_RC iduQueueInitializeDualLock(iduQueue *aQueue, iduMemoryClientIndex aClientIndex)
{
    iduQueueDualLock *sQueue = NULL;

    aQueue->mCapacity += 1;

    IDE_TEST(iduMemMgr::malloc(aClientIndex,
                               ID_SIZEOF(iduQueueDualLock) + aQueue->mCapacity * ID_SIZEOF(void *),
                               (void **)&sQueue,
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    sQueue->mHead = 0;
    sQueue->mTail = 0;

    IDE_TEST(sQueue->mHeadMutex.initialize((SChar *)"IDU_QUEUE_HEAD_MUTEX",
                                           IDU_MUTEX_KIND_NATIVE2,
                                           IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);
    IDE_TEST(sQueue->mTailMutex.initialize((SChar *)"IDU_QUEUE_TAIL_MUTEX",
                                           IDU_MUTEX_KIND_NATIVE2,
                                           IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    aQueue->mObj = (struct iduQueueObj *)sQueue;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sQueue != NULL)
        {
            IDE_ASSERT(iduMemMgr::free(sQueue) == IDE_SUCCESS);
        }
    }

    return IDE_FAILURE;
}

IDE_RC iduQueueFinalizeDualLock(iduQueue *aQueue)
{
    iduQueueDualLock *sQueue = (iduQueueDualLock *)aQueue->mObj;

    IDE_TEST(sQueue->mHeadMutex.destroy() != IDE_SUCCESS);
    IDE_TEST(sQueue->mTailMutex.destroy() != IDE_SUCCESS);

    IDE_TEST(iduMemMgr::free(aQueue->mObj) != IDE_SUCCESS);

    aQueue->mObj = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduQueueEnqueueDualLock(iduQueue *aQueue, void *aObject)
{
    iduQueueDualLock *sQueue = (iduQueueDualLock *)aQueue->mObj;
    UInt              sTail;

    IDE_ASSERT(sQueue->mTailMutex.lock( NULL )
               == IDE_SUCCESS);

    sTail = (sQueue->mTail + 1) % aQueue->mCapacity;

    IDE_TEST_RAISE(sTail == sQueue->mHead, QueueFull);

    sQueue->mObject[sQueue->mTail] = aObject;
    sQueue->mTail = sTail;

    IDE_ASSERT(sQueue->mTailMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(QueueFull);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_QUEUE_FULL));
    }
    IDE_EXCEPTION_END;
    {
        IDE_ASSERT(sQueue->mTailMutex.unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

IDE_RC iduQueueDequeueDualLock(iduQueue *aQueue, void **aObject)
{
    iduQueueDualLock *sQueue = (iduQueueDualLock *)aQueue->mObj;

    IDE_ASSERT( sQueue->mHeadMutex.lock( NULL )
                == IDE_SUCCESS);

    IDE_TEST_CONT(sQueue->mHead == sQueue->mTail, QueueEmpty);

    *aObject = sQueue->mObject[sQueue->mHead];
    sQueue->mHead = (sQueue->mHead + 1) % aQueue->mCapacity;

    IDE_ASSERT(sQueue->mHeadMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(QueueEmpty);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_QUEUE_EMPTY));
    }
    IDE_EXCEPTION_END;
    {
        IDE_ASSERT(sQueue->mHeadMutex.unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

idBool iduQueueIsEmptyDualLock(iduQueue *aQueue)
{
    iduQueueDualLock *sQueue = (iduQueueDualLock *)aQueue->mObj;

    return (sQueue->mHead == sQueue->mTail) ? ID_TRUE : ID_FALSE;
}


iduQueueOp gIduQueueOpDualLock =
{
    iduQueueInitializeDualLock,
    iduQueueFinalizeDualLock,

    iduQueueEnqueueDualLock,
    iduQueueDequeueDualLock,

    iduQueueIsEmptyDualLock
};
