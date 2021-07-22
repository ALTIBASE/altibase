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
 * $Id:
 ******************************************************************************/

#include <act.h>
#include <acpMem.h>
#include <aclQueue.h>

/* ------------------------------------------------
 * Main Body
 * ----------------------------------------------*/

typedef struct testQueueStruct
{
    acp_uint64_t    mPushCount;
    acp_uint64_t    mPullCount;
    acl_queue_t*    mQueue;
} testQueueStruct;

/* ------------------------------------------------
 *  Sample
 * ----------------------------------------------*/

acp_rc_t testQueuePerfInit(actPerfFrmThrObj *aThrObj)
{
    testQueueStruct*    sQueueStruct;
    acp_rc_t            sRC;
    acp_uint64_t        sCount;
    acp_uint32_t        i;

    sCount = aThrObj->mObj.mFrm->mLoopCount;

    sRC = acpMemAlloc((void*)(&sQueueStruct), sizeof(testQueueStruct));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FREE_NOTHING);

    sRC = acpMemAlloc((void*)(&sQueueStruct->mQueue), sizeof(acl_queue_t));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FREE_STRUCT);

    sRC = aclQueueCreate(sQueueStruct->mQueue, aThrObj->mObj.mFrm->mParallel);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), FREE_QUEUE);

    sQueueStruct->mPushCount = sQueueStruct->mPullCount = 0;

    for (i = 0; i < aThrObj->mObj.mFrm->mParallel; i++)
    {
        aThrObj[i].mObj.mData = (void*)sQueueStruct;
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(FREE_NOTHING)
    {
        /* Do nothing */
    }

    ACP_EXCEPTION(FREE_STRUCT)
    {
        acpMemFree(sQueueStruct);
    }

    ACP_EXCEPTION(FREE_QUEUE)
    {
        acpMemFree(sQueueStruct->mQueue);
        acpMemFree(sQueueStruct);
    }

    ACP_EXCEPTION_END;

    return sRC;

}

acp_rc_t testQueuePerfBody(actPerfFrmThrObj *aThrObj)
{
    testQueueStruct*    sQueueStruct = (testQueueStruct*)aThrObj->mObj.mData;
    acl_queue_t*        sQueue;

    acp_uint64_t        sCount;
    acp_uint64_t        sCount1;
    acp_uint64_t        sCount2;
    void*               sTemp;

    acp_rc_t            sRC;
    acp_ulong_t        i;

    ACP_TEST_RAISE(NULL == sQueueStruct, NO_QUEUE);

    sQueue  = sQueueStruct->mQueue;
    sCount  = aThrObj->mObj.mFrm->mLoopCount;
    sCount1 = sCount / 2;
    sCount2 = sCount - sCount1;

    /* Push random generated Numbers into queue */
    for(i = 0; i < sCount1; i++)
    {
        sRC = aclQueueEnqueue(sQueue, (void*)i);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENQUEUE_FAILED);
        (void)acpAtomicInc64(&(sQueueStruct->mPushCount));
    }

    /* Push and pull simultaneously */
    for(i = 0; i < sCount2; i++)
    {
        sRC = aclQueueEnqueue(sQueue, (void*)i);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENQUEUE_FAILED);
        (void)acpAtomicInc64(&(sQueueStruct->mPushCount));

        sRC = aclQueueDequeue(sQueue, &sTemp);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpAtomicInc64(&(sQueueStruct->mPullCount));
        }
        else
        {
            /* ACP_RC_ENOENT */
        }
    }

    /* Pull Everything */
    while(ACP_RC_IS_SUCCESS(aclQueueDequeue(sQueue, &sTemp)))
    {
        (void)acpAtomicInc64(&(sQueueStruct->mPullCount));
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(NO_QUEUE)
    {
        sRC = ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(ENQUEUE_FAILED)
    {
        ACT_ERROR(("Enqueue Failed! : %d\n", sRC));
    }

    ACP_EXCEPTION_END;
    return sRC;
}

acp_rc_t testQueuePerfFinal(actPerfFrmThrObj *aThrObj)
{
    testQueueStruct*    sQueueStruct = (testQueueStruct*)aThrObj->mObj.mData;
    acp_uint64_t        sCount;

    ACT_CHECK(NULL != sQueueStruct);

    if(NULL == sQueueStruct)
    {
        /* Do nothing */
    }
    else
    {
        /* Push and pull number must be same with loopcount * parallels */
        sCount  = aThrObj->mObj.mFrm->mLoopCount;
        sCount *= aThrObj->mObj.mFrm->mParallel;

        ACT_CHECK(sCount == sQueueStruct->mPushCount);
        ACT_CHECK(sCount == sQueueStruct->mPullCount);

        /* Free queue */
        aclQueueDestroy(sQueueStruct->mQueue);
        acpMemFree(sQueueStruct->mQueue);
        acpMemFree(sQueueStruct);
    }

    return ACP_RC_SUCCESS;
    
}

/* ------------------------------------------------
 *  Data Descriptor 
 * ----------------------------------------------*/

acp_char_t /*@unused@*/ *gPerName = "QueuePerf";

actPerfFrm /*@unused@*/ gPerfDesc[] =
{
    /* aclQueue */
    {
        "aclQueue",
        1,
        ACP_UINT64_LITERAL(1000000),
        testQueuePerfInit,
        testQueuePerfBody,
        testQueuePerfFinal
    } ,
    {
        "aclQueue",
        2,
        ACP_UINT64_LITERAL(1000000),
        testQueuePerfInit,
        testQueuePerfBody,
        testQueuePerfFinal
    } ,
    {
        "aclQueue",
        4,
        ACP_UINT64_LITERAL(1000000),
        testQueuePerfInit,
        testQueuePerfBody,
        testQueuePerfFinal
    } ,
    {
        "aclQueue",
        8,
        ACP_UINT64_LITERAL(1000000),
        testQueuePerfInit,
        testQueuePerfBody,
        testQueuePerfFinal
    } ,
    {
        "aclQueue",
        16,
        ACP_UINT64_LITERAL(1000000),
        testQueuePerfInit,
        testQueuePerfBody,
        testQueuePerfFinal
    } ,
    ACT_PERF_FRM_SENTINEL
};

