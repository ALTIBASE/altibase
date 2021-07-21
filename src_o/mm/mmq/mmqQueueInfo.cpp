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

#include <iduHashUtil.h>
#include <smiMisc.h>
#include <qci.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcTask.h>
#include <mmtThreadManager.h>
#include <mmqQueueInfo.h>
#include <mmuProperty.h>


IDE_RC mmqQueueInfo::initialize(UInt aTableID)
{
    SChar sMutexName[30];

    mTableID        = aTableID;
    mQueueDropFlag  = ID_FALSE;
    SM_INIT_SCN(&mCommitSCN);

    IDU_LIST_INIT(&mSessionList);

    idlOS::snprintf(sMutexName,
                    ID_SIZEOF(sMutexName),
                    "QUEUE_MUTEX_%"ID_UINT32_FMT,
                    mTableID);

    IDE_TEST(mMutex.initialize(sMutexName,
                               IDU_MUTEX_KIND_POSIX,
                               IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    idlOS::snprintf(sMutexName,
                    ID_SIZEOF(sMutexName),
                    "QUEUE_MUTEX_DEQUEUE%"ID_UINT32_FMT,
                    mTableID);


    IDE_TEST_RAISE(mDequeueCond.initialize() != IDE_SUCCESS, ContInitError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ContInitError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_INIT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmqQueueInfo::destroy()
{
    IDE_TEST_RAISE(mDequeueCond.destroy() != IDE_SUCCESS, CondDestroyError);


    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(CondDestroyError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_DESTROY));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void mmqQueueInfo::lock()
{
    IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
}

void mmqQueueInfo::unlock()
{
    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
}


IDE_RC mmqQueueInfo::timedwaitDequeue(ULong aWaitSec, idBool* aTimeOut)
{
    IDE_RC         rc;
    SInt           sWaitSec;
    SInt           sWaitUSec;
    PDL_Time_Value sTimeValue;
    PDL_Time_Value sAddValue;

    //PROJ-1677 DEQ
    *aTimeOut = ID_FALSE;
    if (aWaitSec == 0)
    {
        /* BUG-46173 대기 안함
           DEQUEUE ~ NOWAIT|WAIT 0인 경우에 MMC_TASK_STATE_QUEUEWAIT로 전환되지 않는다.
           그래서 waitForEnqueueEvent()를 통해 본 함수가 호출되지 않는다. */
    }
    else
    {
        if (aWaitSec == ID_ULONG_MAX)
        {
            //fix BUG-30949 A waiting time for enqueue event in transformed dedicated thread should not be infinite. 
            sWaitSec  = mmuProperty::getMaxEnqWaitTime()/1000000;
            sWaitUSec = mmuProperty::getMaxEnqWaitTime() %1000000;
            // BUG-27292 HP-UX에 2038년 문제가 있어서
            // Time값이 넘어서지 않도록 WaitSec를 조정합니다.
            if( sWaitSec > IDV_MAX_TIME_INTERVAL_SEC )
            {
                sWaitSec = IDV_MAX_TIME_INTERVAL_SEC ;
            }
            sAddValue.set( sWaitSec, sWaitUSec );
            sTimeValue = idlOS::gettimeofday();
            sTimeValue += sAddValue;
            rc = mDequeueCond.timedwait(&mMutex, &sTimeValue, IDU_IGNORE_TIMEDOUT);
            //fix BUG-30949 A waiting time for enqueue event in transformed dedicated thread should not be infinite. 
            //infinite를 emulation하기 위하여 timeout에대한 output parameter설정을 하지 않는다.
        }
        else
        {
            // 주어진 시간만큼 대기
            // BUGBUG : session의 mQueueEndTime으로 하는것이 더 정확하지 않을까?

            sWaitSec  = aWaitSec/1000000;
            sWaitUSec = aWaitSec%1000000;

            // BUG-27292 HP-UX에 2038년 문제가 있어서
            // Time값이 넘어서지 않도록 WaitSec를 조정합니다.
            if( sWaitSec > IDV_MAX_TIME_INTERVAL_SEC )
            {
                sWaitSec = IDV_MAX_TIME_INTERVAL_SEC ;
            }

            sAddValue.set( sWaitSec, sWaitUSec );

            sTimeValue = idlOS::gettimeofday();
            sTimeValue += sAddValue;

            rc = mDequeueCond.timedwait(&mMutex, &sTimeValue, IDU_IGNORE_TIMEDOUT);
            *aTimeOut = mDequeueCond.isTimedOut();
        }

        IDE_TEST_RAISE(rc != IDE_SUCCESS, CondWaitError);
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(CondWaitError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_WAIT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmqQueueInfo::broadcastEnqueue()
{
    IDE_TEST_RAISE(mDequeueCond.broadcast() != IDE_SUCCESS, CondBroadcastError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(CondBroadcastError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_BROADCAST));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
//fix BUG-19321
void mmqQueueInfo::addSession(mmcSession *aSession)
{
    //fix BUG-21361 drop queue 동시성 문제로 서버 비정상 종료될수 있음.
    lock();
    IDU_LIST_ADD_LAST(&mSessionList, aSession->getQueueListNode());
    unlock();
}

void mmqQueueInfo::removeSession(mmcSession *aSession)
{
    idBool sFreeFlag = ID_FALSE;
    //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
    // 동시성에 문제가 있음.
    lock();

    aSession->setQueueInfo(NULL);
    aSession->setQueueWaitTime(0);

    if (IDU_LIST_IS_EMPTY(aSession->getQueueListNode()) != ID_TRUE)
    {

        IDU_LIST_REMOVE(aSession->getQueueListNode());

        IDU_LIST_INIT_OBJ(aSession->getQueueListNode(), aSession);

        if ((mQueueDropFlag == ID_TRUE) && (isSessionListEmpty() == ID_TRUE))
        {
            sFreeFlag = ID_TRUE;
        }

    }
    //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
    // 동시성에 문제가 있음.
    unlock();

    if (sFreeFlag == ID_TRUE)
    {
        ideLog::log(IDE_SERVER_0,"DROP QUEQUE HAPPEND!!");
        mmqManager::freeQueueInfo(this);
    }

}
// fix BUG-19320
void mmqQueueInfo::wakeup4DeqRollback()
{
    lock();


    /* fix  BUG-27470 The scn and timestamp in the run time header of queue
       have duplicated objectives.
       increase commit SCN in run time header of queue 
     */
    
    /*fix BUG-31514 While a dequeue rollback ,
      another dequeue statement which currenlty is waiting for enqueue event might lost the  event */

    /* same location: dequeue wait, dequeue rollback  queue table */
    smiGetLastSystemSCN(&mCommitSCN);
    
    SM_INCREASE_SCN(&mCommitSCN);

    (void)broadcastEnqueue();
    
    unlock();
}
//PROJ-1677 DEQUEUE
void mmqQueueInfo::wakeup(smSCN *aCommitSCN)
{   
    lock();

    /* fix  BUG-27470 The scn and timestamp in the run time header of queue
       have duplicated objectives.
     */
    if(SM_SCN_IS_GT(aCommitSCN,&mCommitSCN))
    {       
        SM_SET_SCN(&mCommitSCN,aCommitSCN);
    }    
    
    (void)broadcastEnqueue();
    
    unlock();
}

UInt mmqQueueInfo::hashFunc(void *aKey)
{
    return iduHashUtil::hashUInt(*(UInt *)aKey);
}

SInt mmqQueueInfo::compFunc(void *aLhs, void *aRhs)
{
    if (*(UInt *)aLhs > *(UInt *)aRhs)
    {
        return 1;
    }
    else if (*(UInt *)aLhs < *(UInt *)aRhs)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
