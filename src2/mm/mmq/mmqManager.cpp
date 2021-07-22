/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <idl.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmqManager.h>
#include <mmqQueueInfo.h>
#include <mmuProperty.h>

iduMemPool  mmqManager::mQueueInfoPool;
smuHashBase mmqManager::mQueueInfoHash;


IDE_RC mmqManager::initialize()
{
    IDE_TEST(mQueueInfoPool.initialize(IDU_MEM_MMQ,
                                       (SChar *)"Queue Info Memory Pool",
                                       ID_SCALABILITY_SYS,
                                       sizeof(mmqQueueInfo),
                                       4,
                                       IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                       ID_TRUE,							/* UseMutex */
                                       IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                       ID_FALSE,						/* ForcePooling */
                                       ID_TRUE,							/* GarbageCollection */
                                       ID_TRUE,                          /* HWCacheLine */
                                       IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			

    IDE_TEST(smuHash::initialize(&mQueueInfoHash,
                                 ID_SCALABILITY_CPU,
                                 mmuProperty::getQueueGlobalHashTableSize(),
                                 sizeof(UInt),
                                 ID_TRUE,
                                 mmqQueueInfo::hashFunc,
                                 mmqQueueInfo::compFunc) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmqManager::finalize()
{
    mmqQueueInfo *sQueueInfo;

    IDE_TEST(smuHash::open(&mQueueInfoHash) != IDE_SUCCESS);

    while (1)
    {
        IDE_TEST(smuHash::cutNode(&mQueueInfoHash, (void **)&sQueueInfo) != IDE_SUCCESS);

        if (sQueueInfo == NULL)
        {
            break;
        }

        IDE_TEST(sQueueInfo->destroy() != IDE_SUCCESS);

        IDE_TEST(mQueueInfoPool.memfree(sQueueInfo) != IDE_SUCCESS);
    }

    IDE_TEST(smuHash::close(&mQueueInfoHash) != IDE_SUCCESS);

    IDE_TEST(smuHash::destroy(&mQueueInfoHash) != IDE_SUCCESS);

    IDE_TEST(mQueueInfoPool.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmqManager::findQueue(UInt aTableID, mmqQueueInfo **aQueueInfo)
{
    IDE_TEST(smuHash::findNode(&mQueueInfoHash, &aTableID, (void **)aQueueInfo) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmqManager::freeQueue(mmqQueueInfo *aQueueInfo)
{
    UInt          sTableID   = aQueueInfo->getTableID();
    idBool        sFreeFlag  = ID_FALSE;
    mmqQueueInfo *sQueueInfo = NULL;
    
    IDE_TEST(smuHash::deleteNode(&mQueueInfoHash, &sTableID, (void **)&sQueueInfo) != IDE_SUCCESS);

    IDE_ASSERT(sQueueInfo == aQueueInfo);

    sQueueInfo->lock();
    
    sQueueInfo->setQueueDrop();

    if (sQueueInfo->isSessionListEmpty() == ID_TRUE)
    {
        sFreeFlag = ID_TRUE;
    }

    sQueueInfo->unlock();

    if (sFreeFlag == ID_TRUE)
    {
        IDE_TEST(mmqManager::freeQueueInfo(sQueueInfo) != IDE_SUCCESS);
    }
    else
    {
        //fix BUG-19320
        sQueueInfo->wakeup4DeqRollback();
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmqManager::freeQueueInfo(mmqQueueInfo *aQueueInfo)
{
    IDE_TEST(aQueueInfo->destroy() != IDE_SUCCESS);

    IDE_TEST(mQueueInfoPool.memfree(aQueueInfo) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmqManager::createQueue(void *aArg)
{
    qciArgCreateQueue *sArg       = (qciArgCreateQueue *)aArg;
    mmqQueueInfo      *sQueueInfo = NULL;

    IDE_TEST(smuHash::findNode(&mQueueInfoHash, &sArg->mTableID, (void **)&sQueueInfo) != IDE_SUCCESS);

    if (sQueueInfo == NULL)
    {
        IDU_FIT_POINT( "mmqManager::createQueue::alloc::QueueInfo" );

        IDE_TEST(mQueueInfoPool.alloc((void **)&sQueueInfo) != IDE_SUCCESS);

        IDE_TEST(sQueueInfo->initialize(sArg->mTableID) != IDE_SUCCESS);

        IDE_TEST(smuHash::insertNode(&mQueueInfoHash, &sArg->mTableID, sQueueInfo) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmqManager::dropQueue(void *aArg)
{
    qciArgDropQueue *sArg       = (qciArgDropQueue *)aArg;
    mmcSession      *sSession   = (mmcSession *)sArg->mMmSession;
    mmqQueueInfo    *sQueueInfo = NULL;
    IDE_ASSERT(findQueue(sArg->mTableID, &sQueueInfo) == IDE_SUCCESS);

    if (sQueueInfo != NULL)
    {
        sSession->setQueueInfo(sQueueInfo);
    }
    return IDE_SUCCESS;
}

IDE_RC mmqManager::enqueue(void *aArg)
{
    qciArgEnqueue *sArg     = (qciArgEnqueue *)aArg;
    mmcSession    *sSession = (mmcSession *)sArg->mMmSession;

    IDE_TEST(sSession->bookEnqueue(sArg->mTableID) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmqManager::dequeue(void *aArg,idBool aBookDeq)
{
    qciArgDequeue *sArg       = (qciArgDequeue *)aArg;
    mmcSession    *sSession   = (mmcSession *)sArg->mMmSession;
    mmqQueueInfo  *sQueueInfo = NULL;
    //PROJ-1677 DEQUEUE
    if(aBookDeq == ID_TRUE)
    { 
        IDE_TEST(sSession->bookDequeue(sArg->mTableID) != IDE_SUCCESS);
    }
    else
    {
        if (sSession->getQueueInfo() == NULL)
        {
            IDE_ASSERT(findQueue(sArg->mTableID, &sQueueInfo) == IDE_SUCCESS);
            IDE_TEST_RAISE(sQueueInfo == NULL, QueueNotFound);
            sSession->setQueueInfo(sQueueInfo);
            sSession->setQueueWaitTime(sArg->mWaitSec);
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(QueueNotFound);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_QUEUE_NOT_FOUND));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmqManager::setQueueStamp(void* aArg)
{
    qciArgDequeue *sArg       = (qciArgDequeue *)aArg;
    mmcSession    *sSession   = (mmcSession *)sArg->mMmSession;
    mmqQueueInfo  *sQueueInfo = NULL;
    IDE_ASSERT(findQueue(sArg->mTableID, &sQueueInfo) == IDE_SUCCESS);
    IDE_TEST_RAISE(sQueueInfo == NULL, QueueNotFound);
    /* fix  BUG-27470 The scn and timestamp in the run time header of queue have duplicated objectives. */
    sSession->setQueueSCN(&(sArg->mViewSCN));
    return IDE_SUCCESS;

    IDE_EXCEPTION(QueueNotFound);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_QUEUE_NOT_FOUND));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
