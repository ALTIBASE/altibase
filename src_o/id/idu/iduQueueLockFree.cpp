/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduQueueLockFree.cpp 26440 2008-06-10 04:02:48Z jdlee $
 ****************************************************************************/

/*****************************************************************************
 * This Lock-Free FIFO Queue Implementation is based on
 *
 * "A Simple, Fast and Scalable Non-Blocking Concurrent
 *  FIFO Queue for Shared Memory Multiprocessor Systems"
 * by Philippas Tsigas, Yi Zhang
 * in 2001 ACM ISBN 1-58113-409-6/01/07
 ****************************************************************************/

#include <idu.h>
#include <idkAtomic.h>


#define IDU_QUEUE_NULL0 ((void *)0)
#define IDU_QUEUE_NULL1 ((void *)ID_vULONG_MAX)


typedef struct iduQueueLockFree
{
    UInt   volatile mHead;
    UInt   volatile mTail;

    void * volatile mNull;
    void * volatile mObject[1];
} iduQueueLockFree;


IDE_RC iduQueueInitializeLockFree(iduQueue *aQueue, iduMemoryClientIndex aClientIndex)
{
    iduQueueLockFree *sQueue = NULL;
    UInt              i;

    aQueue->mCapacity += 2;

    IDE_TEST(iduMemMgr::malloc(aClientIndex,
                               ID_SIZEOF(iduQueueLockFree) + ID_SIZEOF(void *) * aQueue->mCapacity,
                               (void **)&sQueue,
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    sQueue->mHead      = 0;
    sQueue->mTail      = 1;
    sQueue->mNull      = IDU_QUEUE_NULL1;
    sQueue->mObject[0] = IDU_QUEUE_NULL1;

    for (i = 1; i < aQueue->mCapacity; i++)
    {
        sQueue->mObject[i] = IDU_QUEUE_NULL0;
    }

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

IDE_RC iduQueueFinalizeLockFree(iduQueue *aQueue)
{
    IDE_TEST(iduMemMgr::free(aQueue->mObj) != IDE_SUCCESS);

    aQueue->mObj = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduQueueEnqueueLockFree(iduQueue *aQueue, void *aObject)
{
    iduQueueLockFree *sQueue = (iduQueueLockFree *)aQueue->mObj;
    UInt              sOldTail;
    UInt              sTail;
    UInt              sNext;
    void             *sObject;

    while (1)
    {
        sOldTail = sQueue->mTail;
        sTail    = sOldTail;
        sNext    = (sTail + 1) % aQueue->mCapacity;
        sObject  = sQueue->mObject[sTail];

        while ((sObject != IDU_QUEUE_NULL0) && (sObject != IDU_QUEUE_NULL1))
        {
            /* check tail consistency */
            if (sOldTail != sQueue->mTail)
            {
                break;
            }

            /* if sNext reaches Head then the queue might be full */
            if (sNext == sQueue->mHead)
            {
                break;
            }

            /* look at the next cell */
            sObject = sQueue->mObject[sNext];
            sTail   = sNext;
            sNext   = (sTail + 1) % aQueue->mCapacity;
        }

        /* check tail consistency */
        if (sOldTail != sQueue->mTail)
        {
            continue;
        }

        /* check whether the queue is full */
        if (sNext == sQueue->mHead)
        {
            sTail   = (sNext + 1) % aQueue->mCapacity;
            sObject = sQueue->mObject[sTail];

            if ((sObject != IDU_QUEUE_NULL0) && (sObject != IDU_QUEUE_NULL1))
            {
                /* The queue is full */
                IDE_RAISE(QueueFull);
            }

            if (sTail == 0)
            {
                /* Head rewind */
                sQueue->mNull = sObject;
            }

            /* help dequeue by updating head */
            idkAtomicCAS4((void *)&sQueue->mHead, sNext, sTail);
        }
        else
        {
            /* check tail consistency */
            if (sOldTail != sQueue->mTail)
            {
                continue;
            }

            /* get the actual tail and attempt to enqueue the data */
            if (idkAtomicCASP((void *)&sQueue->mObject[sTail], sObject, aObject) == sObject)
            {
                /* the enqueue succeeded */
                if ((sNext % 2) == 0)
                {
                    idkAtomicCAS4((void *)&sQueue->mTail, sOldTail, sNext);
                }

                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(QueueFull);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_QUEUE_FULL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduQueueDequeueLockFree(iduQueue *aQueue, void **aObject)
{
    iduQueueLockFree *sQueue = (iduQueueLockFree *)aQueue->mObj;
    UInt              sHead;
    UInt              sNext;
    void             *sObject;
    void             *sNull;

    while (1)
    {
        sHead   = sQueue->mHead;
        sNext   = (sHead + 1) % aQueue->mCapacity;
        sObject = sQueue->mObject[sNext];

        /* find the actual head */
        while ((sObject == IDU_QUEUE_NULL0) || (sObject == IDU_QUEUE_NULL1))
        {
            /* check head consistency */
            if (sHead != sQueue->mHead)
            {
                break;
            }

            /* two consecutive NULL means empty */
            if (sNext == sQueue->mTail)
            {
                IDE_RAISE(QueueEmpty);
            }

            sNext   = (sNext + 1) % aQueue->mCapacity;
            sObject = sQueue->mObject[sNext];
        }

        /* check head consistency */
        if (sHead != sQueue->mHead)
        {
            continue;
        }

        /* check whether the queue is empty */
        if (sNext == sQueue->mTail)
        {
            /* help the dequeue by updating tail */
            idkAtomicCAS4((void *)&sQueue->mTail, sNext, (sNext + 1) % aQueue->mCapacity);
        }
        else
        {
            /* if dequeue rewind to 0, switch NULL to avoid ABA problems */
            if (sNext != 0)
            {
                if (sNext < sHead)
                {
                    sNull = sQueue->mObject[0];
                }
                else
                {
                    sNull = sQueue->mNull;
                }
            }
            else
            {
                sNull = (void *)(~((vULong)(sQueue->mNull)));
            }

            /* check head consistency */
            if (sHead != sQueue->mHead)
            {
                continue;
            }

            /* get the actual head. Null value means empty */
            if (idkAtomicCASP((void *)&sQueue->mObject[sNext], sObject, sNull) == sObject)
            {
                /* if dequeue rewind to 0, switch NULL to avoid ABA problems */
                if (sNext == 0)
                {
                    sQueue->mNull = sNull;
                }

                if ((sNext % 2) == 0)
                {
                    idkAtomicCAS4((void *)&sQueue->mHead, sHead, sNext);
                }

                *aObject = sObject;

                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(QueueEmpty);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_QUEUE_EMPTY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool iduQueueIsEmptyLockFree(iduQueue *aQueue)
{
    iduQueueLockFree *sQueue = (iduQueueLockFree *)aQueue->mObj;
    UInt              sHead;
    UInt              sNext;
    void             *sObject;

    while (1)
    {
        sHead   = sQueue->mHead;
        sNext   = (sHead + 1) % aQueue->mCapacity;
        sObject = sQueue->mObject[sNext];

        /* find the actual head */
        while ((sObject == IDU_QUEUE_NULL0) || (sObject == IDU_QUEUE_NULL1))
        {
            /* check head consistency */
            if (sHead != sQueue->mHead)
            {
                break;
            }

            /* two consecutive NULL means empty */
            if (sNext == sQueue->mTail)
            {
                return ID_TRUE;
            }

            sNext   = (sNext + 1) % aQueue->mCapacity;
            sObject = sQueue->mObject[sNext];
        }

        /* check head consistency */
        if (sHead != sQueue->mHead)
        {
            continue;
        }

        /* check whether the queue is empty */
        if (sNext == sQueue->mTail)
        {
            /* help the dequeue by updating tail */
            idkAtomicCAS4((void *)&sQueue->mTail, sNext, (sNext + 1) % aQueue->mCapacity);
        }
        else
        {
            /* check head consistency */
            if (sHead != sQueue->mHead)
            {
                continue;
            }

            /* check the actual head. Null value means empty.
               But sObject can't be null here, can it? */
            if (sQueue->mObject[sNext] == sObject)
            {
                return ((sObject == IDU_QUEUE_NULL0) || (sObject == IDU_QUEUE_NULL1)) ?
                    ID_TRUE : ID_FALSE;
            }
        }
    }
}


iduQueueOp gIduQueueOpLockFree =
{
    iduQueueInitializeLockFree,
    iduQueueFinalizeLockFree,

    iduQueueEnqueueLockFree,
    iduQueueDequeueLockFree,

    iduQueueIsEmptyLockFree
};
