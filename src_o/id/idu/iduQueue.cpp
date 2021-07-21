/*******************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ******************************************************************************/

/*******************************************************************************
 * $Id: iduQueue.cpp 67616 2014-11-19 05:59:37Z djin $
 ******************************************************************************/

#include <idu.h>


extern iduQueueOp gIduQueueOpDualLock;

/*
extern iduQueueOp gIduQueueOpLockFree;
*/


IDE_RC iduQueueInitialize(iduQueue            *aQueue,
                          UInt                 aCapacity,
                          iduQueueType         aType,
                          iduMemoryClientIndex aClientIndex)
{
    aQueue->mType     = aType;
    aQueue->mCapacity = aCapacity;

    switch (aType)
    {
        case IDU_QUEUE_TYPE_DUALLOCK:
            aQueue->mOp = &gIduQueueOpDualLock;
            break;
/*
        case IDU_QUEUE_TYPE_LOCKFREE:
            aQueue->mOp = &gIduQueueOpLockFree;
            break;
*/
        default:
            IDE_ASSERT(0);
    }

    return aQueue->mOp->mInitialize(aQueue, aClientIndex);
}

IDE_RC iduQueueFinalize(iduQueue *aQueue)
{
    return aQueue->mOp->mFinalize(aQueue);
}

IDE_RC iduQueueEnqueue(iduQueue *aQueue, void *aObject)
{
    /*
     * BUGBUG: IDE_ASSERT or IDE_TEST ?
     */
    IDE_ASSERT(aObject != NULL);

    return aQueue->mOp->mEnqueue(aQueue, aObject);
}

IDE_RC iduQueueDequeue(iduQueue *aQueue, void **aObject)
{
    return aQueue->mOp->mDequeue(aQueue, aObject);
}

idBool iduQueueIsEmpty(iduQueue *aQueue)
{
    return aQueue->mOp->mIsEmpty(aQueue);
}
