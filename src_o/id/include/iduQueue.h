/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduQueue.h 26440 2008-06-10 04:02:48Z jdlee $
 ****************************************************************************/

#ifndef _O_IDU_QUEUE_H_
#define _O_IDU_QUEUE_H_ 1

#include <idl.h>
#include <ide.h>


typedef enum iduQueueType
{
    IDU_QUEUE_TYPE_DUALLOCK,
/*
    IDU_QUEUE_TYPE_LOCKFREE,
*/
    IDU_QUEUE_TYPE_MAX
} iduQueueType;


typedef struct iduQueue
{
    iduQueueType        mType;
    UInt                mCapacity;

    struct iduQueueObj *mObj;
    struct iduQueueOp  *mOp;
} iduQueue;

typedef struct iduQueueOp
{
    IDE_RC (*mInitialize)(iduQueue *aQueue, iduMemoryClientIndex aClientIndex);
    IDE_RC (*mFinalize)(iduQueue *aQueue);

    IDE_RC (*mEnqueue)(iduQueue *aQueue, void *aObject);
    IDE_RC (*mDequeue)(iduQueue *aQueue, void **aObject);

    idBool (*mIsEmpty)(iduQueue *aQueue);
} iduQueueOp;


IDE_RC iduQueueInitialize(iduQueue             *aQueue,
                          UInt                  aCapacity,
                          iduQueueType          aType,
                          iduMemoryClientIndex  aClientIndex);

IDE_RC iduQueueFinalize(iduQueue *aQueue);

IDE_RC iduQueueEnqueue(iduQueue *aQueue, void *aObject);
IDE_RC iduQueueDequeue(iduQueue *aQueue, void **aObject);

idBool iduQueueIsEmpty(iduQueue *aQueue);


#endif
