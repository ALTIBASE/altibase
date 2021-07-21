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

#include <idtCPUSet.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcTask.h>
#include <mmtServiceThread.h>
#include <mmtSessionManager.h>
#include <mmtThreadManager.h>
#include <mmuProperty.h>
#include <mmm.h>
#include <mmuAccessList.h>

#if defined(WRS_VXWORKS) 
# define MMT_SERVICE_THREAD_MIN_POLL_TIMEOUT 100000
#else
# define MMT_SERVICE_THREAD_MIN_POLL_TIMEOUT 50
#endif
/* PROJ-2108 Dedicated thread mode which uses less CPU
 * This timeout(765432) is a magic number to notify
 * dispatcher that the server is in dedicated thread mode
 */
# define MMT_SERVICE_THREAD_DEDICATED_POLL_TIMEOUT 765432

mmtServiceThread::mmtServiceThread() : idtBaseThread()
{
}

//fix BUG-19464
IDE_RC mmtServiceThread::initialize(mmcServiceThreadType aServiceThreadType,
                                    mmtServiceThreadStartFunc aServiceThreadStartFunc)
{
    this->setIsServiceThread( ID_TRUE );
    mInfo.mServiceThreadType = aServiceThreadType;
    mInfo.mServiceThreadID   = 0;
    mInfo.mIpcID             = 0;
    mInfo.mState             = MMT_SERVICE_THREAD_STATE_NONE;
    mInfo.mStartTime         = 0;
    mInfo.mSessionID         = 0;
    mInfo.mStmtID            = 0;
    mInfo.mTaskCount         = 0;
    mInfo.mReadyTaskCount    = 0;
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       Load balance history관련 된 필드추가.
    */

    mInfo.mInTaskCntFromIdle    = 0;
    mInfo.mOutTaskCntInIdle     = 0;
    mInfo.mInTaskCntFromBusy    = 0;
    mInfo.mOutTaskCntInBusy     = 0;
    mInfo.mBusyExperienceCnt    = 0;
    mInfo.mLifeSpan = mmuProperty::getServiceThrInitialLifeSpan();
    mInfo.mCurAddedTasks        = 0;
    IDV_TIME_INIT(&mInfo.mExecuteBegin);
    IDV_TIME_INIT(&mInfo.mExecuteEnd);

    /* BUG-38384 A task in a service thread can be a orphan */
    mInfo.mRemoveAllTasks = ID_FALSE;

    mStatement   = NULL;

    mLoopCounter = 1;
    mLoopCheck   = 0;

    mRun         = ID_FALSE;
    mErrorFlag   = ID_FALSE;

    IDU_LIST_INIT_OBJ(&mThreadListNode, this);
    IDU_LIST_INIT_OBJ(&mCheckThreadListNode, this);

    IDU_LIST_INIT(&mNewTaskList);
    IDU_LIST_INIT(&mWaitTaskList);
    IDU_LIST_INIT(&mReadyTaskList);
    //fix BUG-19464
    mServiceThreadStartFunc = aServiceThreadStartFunc;

    IDE_TEST(mMutex.initialize((SChar *)"MMT_SERVICE_THREAD_MUTEX",
                               IDU_MUTEX_KIND_POSIX,
                               IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    IDE_TEST(mNewTaskListMutex.initialize((SChar *)"MMT_SERVICE_THREAD_TASK_LIST_MUTEX",
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    /* BUG-37038 DEDICATED TYPE인 경우에만 initialize 한다. */
    if (mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_DEDICATED)
    {
        IDE_TEST(mMutexForServiceThreadSignal.initialize(
                                            (SChar *)"MMT_SERVICE_THREAD_MUTEX_FOR_SIGNAL",
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
        IDE_TEST(mServiceThreadCV.initialize() != IDE_SUCCESS);
    }
    else
    {
        /* Nothing */
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::finalize()
{
    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);
    IDE_TEST(mNewTaskListMutex.destroy() != IDE_SUCCESS);

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    /* BUG-37038 DEDICATED TYPE인 경우에만 finalize 한다. */
    if (mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_DEDICATED)
    {
        IDE_TEST(mMutexForServiceThreadSignal.destroy() != IDE_SUCCESS);
        IDE_TEST(mServiceThreadCV.destroy() != IDE_SUCCESS);
    }
    else
    {
        /* Nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::initializeThread()
{
    mDispatcher = NULL;

    switch (mInfo.mServiceThreadType)
    {
        case MMC_SERVICE_THREAD_TYPE_SOCKET:
            // BUG-24318 IPC 일경우 세션 정보가 잘못 나옵니다.
            mInfo.mRunMode           = MMT_SERVICE_THREAD_RUN_SHARED;
            mMultiPlexingFunc = &mmtServiceThread::multiplexingAsShared;

            /* PROJ-2681 */
            if ((mmuProperty::getIBEnable() == ID_TRUE) && (mmuProperty::getIBListenerDisable() != ID_TRUE))
            {
                IDE_TEST(cmiAllocDispatcher(&mDispatcher,
                                            CMI_DISPATCHER_IMPL_IB,
                                            mmuProperty::getMaxClient()) != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(cmiAllocDispatcher(&mDispatcher,
                                            CMI_DISPATCHER_IMPL_SOCK,
                                            mmuProperty::getMaxClient()) != IDE_SUCCESS);
            }
            break;

        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        case MMC_SERVICE_THREAD_TYPE_DEDICATED:
            mInfo.mRunMode           = MMT_SERVICE_THREAD_RUN_DEDICATED;
            mMultiPlexingFunc = &mmtServiceThread::dedicatedThreadMode;

            /* PROJ-2681 */
            if ((mmuProperty::getIBEnable() == ID_TRUE) && (mmuProperty::getIBListenerDisable() != ID_TRUE))
            {
                IDE_TEST(cmiAllocDispatcher(&mDispatcher,
                                            CMI_DISPATCHER_IMPL_IB,
                                            mmuProperty::getMaxClient()) != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(cmiAllocDispatcher(&mDispatcher,
                                            CMI_DISPATCHER_IMPL_SOCK,
                                            mmuProperty::getMaxClient()) != IDE_SUCCESS);
            }
            break;

        case MMC_SERVICE_THREAD_TYPE_IPC:
            // BUG-24318 IPC 일경우 세션 정보가 잘못 나옵니다.
            mInfo.mRunMode           = MMT_SERVICE_THREAD_RUN_DEDICATED;
            mMultiPlexingFunc = &mmtServiceThread::multiplexingAsDedicated;

            IDE_TEST(cmiAllocDispatcher(&mDispatcher,
                                        CMI_DISPATCHER_IMPL_IPC,
                                        1) != IDE_SUCCESS);
            break;

        case MMC_SERVICE_THREAD_TYPE_IPCDA:
            // BUG-24318 IPC 일경우 세션 정보가 잘못 나옵니다.
            mInfo.mRunMode           = MMT_SERVICE_THREAD_RUN_DEDICATED;
            mMultiPlexingFunc = &mmtServiceThread::multiplexingAsDedicated;

            IDE_TEST(cmiAllocDispatcher(&mDispatcher,
                                        CMI_DISPATCHER_IMPL_IPCDA,
                                        1) != IDE_SUCCESS);
            break;

        //fix PROJ-1749
        case MMC_SERVICE_THREAD_TYPE_DA:
            break;

        default:
            IDE_ASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (mDispatcher != NULL)
    {
        /* BUG-45240 Non-reachable */
        cmiFreeDispatcher(mDispatcher);
        mDispatcher = NULL;
    }

    return IDE_FAILURE;
}

void   mmtServiceThread::finalizeThread()
{
    //fix PROJ-1749
    if (mInfo.mServiceThreadType != MMC_SERVICE_THREAD_TYPE_DA)
    {
        IDE_DASSERT(cmiFreeDispatcher(mDispatcher) == IDE_SUCCESS);
    }
}

/* PROJ-2108 Dedicated thread mode which uses less CPU */
void mmtServiceThread::run()
{
    mmcServiceThreadType sServiceThreadType;
    UInt                 sIpcServiceThreadID;
    iduMutex*            sIpcServiceThreadMutex;
    iduCond*             sIpcServiceThreadCV;

    /*PROJ-2616*/
    UInt                 sIPCDAServiceThreadID    = 0;
    iduMutex*            sIPCDAServiceThreadMutex = NULL;
    iduCond*             sIPCDAServiceThreadCV    = NULL;

    UInt                 sState=0;        

    IDE_RC               sRC;
    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    /* BUG-35356 Memory violation may occur when calculating CPU busy count */
    UInt                 sSelectedCoreIdx = 0;
    UInt                 sSelectedCoreNumber = 0;
    UInt                 sIsBound = 0;

    IDE_ASSERT(mmtThreadManager::setupDefaultThreadSignal() == IDE_SUCCESS);

    /* PROJ-2617 */
    IDE_ASSERT(ideEnableFaultMgr(ID_TRUE) == IDE_SUCCESS);

    mInfo.mThreadID  = getTid();
    mInfo.mStartTime = mmtSessionManager::getBaseTime();
    mRun             = ID_TRUE;

    mPollTimeout.set(0, mmuProperty::getMultiplexingPollTimeout());

    lockForThread();
    sState=1;	

    setState(MMT_SERVICE_THREAD_STATE_POLL);

    //fix BUG-19464
    mServiceThreadStartFunc(this,
                            mInfo.mServiceThreadType,
                            &mInfo.mServiceThreadID);

    /*
     * BUG-37038
     *
     * DEDICATED TYPE인 경우에만 Lock을 획득한다.
     * 그렇지 않으면 finalize()에서 mutex destroy시 실패(EBUSY)한다.
     * IPC Service Thread에서는 DEDICATED TYPE에서만 사용하는 Mutex를
     * Unlock() 하는 부분이 없기 때문이다.
     */ 
    if (mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_DEDICATED)
    {
        IDE_ASSERT(mMutexForServiceThreadSignal.lock(NULL) == IDE_SUCCESS);
        mmtThreadManager::signalToThreadManager();
    }
    else
    {
        /* do nothing */
    }
    sServiceThreadType = mInfo.mServiceThreadType;

    /* PROJ-2108 Dedicated thread mode which uses less CPU
     * Bind thread to a specific CPU 
     */
    if ( mmuProperty::getIsCPUAffinity() == 1 )
    {
        sSelectedCoreIdx = mmtThreadManager::getCoreIdxToAssign();
        sSelectedCoreNumber = mmtThreadManager::getCoreNumberFromIdx( sSelectedCoreIdx );
        /* BUG-35356 Memory violation may occur when calculating CPU busy count */
        if ( this->setAffinity( sSelectedCoreNumber ) != IDE_SUCCESS )
        {
            mmtThreadManager::decreaseCoreBusyCount( sSelectedCoreIdx );
        }
        else
        {
            sIsBound = 1;
        }
    }
    else
    {
        /* do nothing */
    }

    switch (sServiceThreadType)
    {
        case MMC_SERVICE_THREAD_TYPE_SOCKET:
            while (mRun == ID_TRUE)
            {
                //PROJ-1677 아래와 같이 run-mode가 전환될수 있기 때문에
                //shared thread run mode < ---> dedicated thread run mode 
                //아래와 같이  function pointer call으로 대치 한다.
                (this->*mMultiPlexingFunc)();
            }//while
            break;
            /* PROJ-2108 Dedicated thread mode which uses less CPU */
        case MMC_SERVICE_THREAD_TYPE_DEDICATED:
            sRC = mServiceThreadCV.wait(&mMutexForServiceThreadSignal);
            if (sRC != IDE_SUCCESS) 
            {
                IDE_ASSERT(mMutexForServiceThreadSignal.unlock() == IDE_SUCCESS);
                IDE_RAISE(CondWaitError);
            }
            IDE_ASSERT(mMutexForServiceThreadSignal.unlock() == IDE_SUCCESS);
            while (mRun == ID_TRUE)
            {
                (this->*mMultiPlexingFunc)();                
                if( getAssignedTasks() == 0)
                {

                    IDE_ASSERT(mMutexForServiceThreadSignal.lock(NULL) == IDE_SUCCESS);

                    sRC = mServiceThreadCV.wait(&mMutexForServiceThreadSignal);
                    if(sRC != IDE_SUCCESS) 
                    {
                        IDE_ASSERT(mMutexForServiceThreadSignal.unlock() == IDE_SUCCESS);
                        IDE_RAISE(CondWaitError);
                    }
                    IDE_ASSERT(mMutexForServiceThreadSignal.unlock() == IDE_SUCCESS);
                }
            }//while
            break;

        case MMC_SERVICE_THREAD_TYPE_IPC:
            sIpcServiceThreadID    = getIpcID();
            sIpcServiceThreadMutex = mmtThreadManager::getIpcServiceThreadMutex(sIpcServiceThreadID);
            sIpcServiceThreadCV    = mmtThreadManager::getIpcServiceThreadCV(sIpcServiceThreadID);

            IDE_ASSERT(sIpcServiceThreadMutex->lock(NULL /* idvSQL* */) == IDE_SUCCESS);

            while (mRun == ID_TRUE)
            {
                IDE_CLEAR();

                mLoopCounter++;

                if (mRun == ID_FALSE)
                {
                    break;
                }

                sRC = sIpcServiceThreadCV->wait(sIpcServiceThreadMutex);
                if(sRC != IDE_SUCCESS) 
                {
                    IDE_ASSERT(sIpcServiceThreadMutex->unlock() == IDE_SUCCESS);
                    IDE_RAISE(CondWaitError);
                }
                
                if (mRun == ID_FALSE)
                {
                    break;
                }

                getNewTask();

                if (getReadyTaskCount() > 0)
                {
                    do
                    {
                        mLoopCounter++;

                        executeTask();

                        IDV_TIME_GET(&mInfo.mExecuteEnd);

                        setState(MMT_SERVICE_THREAD_STATE_POLL);

                    } while (getReadyTaskCount() > 0);
                }
            }

            IDE_ASSERT(sIpcServiceThreadMutex->unlock() == IDE_SUCCESS);
            break;
            /*PROJ-2616*/
        case MMC_SERVICE_THREAD_TYPE_IPCDA:
            sIPCDAServiceThreadID    = getIpcID();
            sIPCDAServiceThreadMutex = mmtThreadManager::getIPCDAServiceThreadMutex(sIPCDAServiceThreadID);
            sIPCDAServiceThreadCV    = mmtThreadManager::getIPCDAServiceThreadCV(sIPCDAServiceThreadID);

            IDE_ASSERT(sIPCDAServiceThreadMutex->lock(NULL /* idvSQL* */) == IDE_SUCCESS);

            while (mRun == ID_TRUE)
            {
                IDE_CLEAR();

                mLoopCounter++;

                if (mRun == ID_FALSE)
                {
                    break;
                }

                sRC = sIPCDAServiceThreadCV->wait(sIPCDAServiceThreadMutex);
                if(sRC != IDE_SUCCESS)
                {
                    IDE_ASSERT(sIPCDAServiceThreadMutex->unlock() == IDE_SUCCESS);
                    IDE_RAISE(CondWaitError);
                }

                if (mRun == ID_FALSE)
                {
                    break;
                }

                if (getReadyTaskCount() > 0)
                {
                    do
                    {
                        mLoopCounter++;

                        /* BUG-46163 */
                        executeTask();

                        IDV_TIME_GET(&mInfo.mExecuteEnd);

                        setState(MMT_SERVICE_THREAD_STATE_POLL);

                    } while (getReadyTaskCount() > 0);
                }
            }

            IDE_ASSERT(sIPCDAServiceThreadMutex->unlock() == IDE_SUCCESS);
            break;

        default:
            IDE_ASSERT(0);
    }

    /* PROJ-2108 Dedicated thread mode which uses less CPU
     * Unbind thread from a specific CPU 
     */
    if ( sIsBound == 1 )
    {
        mmtThreadManager::decreaseCoreBusyCount( sSelectedCoreIdx );
    }
    else
    {
        /* do nothing */
    }

    mmtThreadManager::serviceThreadStopped(this);

    sState=0;	
    unlockForThread();

    return;

    IDE_EXCEPTION(CondWaitError);
    {
        /*
         * BUG-35298    [mm] hang occurs because ipc count is incorrect.
         */
    	mmtThreadManager::serviceThreadStopped(this);
        if( sState == 1 )
        {
    	    unlockForThread();
        }
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_WAIT));
    }
    IDE_EXCEPTION_END;
}

void mmtServiceThread::stop()
{
    mRun = ID_FALSE;
}

void mmtServiceThread::addTask(mmcTask *aTask)
{
    cmiProtocolContext *sCtx = aTask->getProtocolContext();
    if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
    {
        /* PROJ-2616 */
        IDU_LIST_ADD_FIRST(&mReadyTaskList, aTask->getThreadListNode());
        addReadyTaskCount(1);
        aTask->setTaskState(MMC_TASK_STATE_READY);
    }
    else
    {
        lockForNewTaskList();
        /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
           assign된 task 개수 증가.
           왜나하면 해당 service thread가 OS에서 scheduling되어
           getNewTask function을 수행되야 ,
           task 갯수가 증가되기 때문...
        */

        mInfo.mCurAddedTasks++;
        mInfo.mLifeSpan += 1;
        IDU_LIST_ADD_LAST(&mNewTaskList, aTask->getThreadListNode());

        unlockForNewTaskList();
    }
}

void mmtServiceThread::addTasks(iduList *aTaskList,UInt aTaskCnt)
{
    lockForNewTaskList();
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       assign된 task 개수 증가.
       왜나하면 해당 service thread가 OS에서 scheduling되어
       getNewTask function을 수행되야 ,
       task 갯수가 증가되기 때문...
    */

    mInfo.mCurAddedTasks += aTaskCnt;
    mInfo.mLifeSpan += aTaskCnt;
    IDU_LIST_JOIN_LIST(&mNewTaskList, aTaskList);

    unlockForNewTaskList();
}

void mmtServiceThread::removeAllTasks(iduList *aTaskList, mmtServiceThreadLock aLock)
{
    idBool       sIsLocked = ID_TRUE;
    mmcTask     *sTask;

    /* BUG-45274 */
    UInt              sPreRemoveTaskCount = 0;

    sPreRemoveTaskCount = getAssignedTasks();

    if (aLock == MMT_SERVICE_THREAD_LOCK)
    {
        sIsLocked = trylockForThread();
    }

    if (sIsLocked == ID_TRUE)
    {
        /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
           task 개수 1 이하이면 아래의 루틴을 타는 것은
           CPU clock 낭비이다.
        */

        /*
         * BUG-38384 A task in a service thread can be a orphan
         *
         * mInfo.mRemoveAllTasks를 두어 모든 Task를 제거해야 경우를 판단한다.
         * 현재 DEDICATED THREAD 방식에서 모든 Task를 제거하는 코드가 수행되면
         * 이상한 현상이 발생하는데 정확하게 원인을 파악하지는 못했다.
         * 현재 의심이 가는 상황에서만 아래 코드가 반드시 수행되도록 수정했다.
         */
        if( getAssignedTasks() > 1 || getRemoveAllTasks() == ID_TRUE )
        {
            /*fix BUG-29717 When a task is terminated or migrated ,
              a busy degree of service thread which the task was assigned
              to should be decreased.*/
            for (;  (IDU_LIST_IS_EMPTY(&mReadyTaskList) != ID_TRUE); )       
            {
                sTask = (mmcTask *)mReadyTaskList.mNext->mObj;
                removeTask(aTaskList, sTask, ID_TRUE);
            }

            lockForNewTaskList();
            for (;  (IDU_LIST_IS_EMPTY(&mNewTaskList) != ID_TRUE); )    
            {
                sTask = (mmcTask *)mNewTaskList.mNext->mObj;
                removeTask(aTaskList, sTask, ID_FALSE);
            }
            increaseOutTaskCount(MMT_SERVICE_THREAD_RUN_IN_BUSY,(getTaskCount() -1));

            unlockForNewTaskList();
            for (;  (IDU_LIST_IS_EMPTY(&mWaitTaskList) != ID_TRUE); )
            {
                sTask = (mmcTask *)mWaitTaskList.mNext->mObj;
                removeTask(aTaskList, sTask, ID_TRUE);
            }
            mInfo.mCurAddedTasks = 0;
        
            mInfo.mTaskCount = 1;
        
            mInfo.mReadyTaskCount = 0;
        }
        else
        {
            //nothing to do.
        }
        if (aLock == MMT_SERVICE_THREAD_LOCK)
        {
            unlockForThread();
        }
    }
    else
    {
        /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
           new tasks들이라도  migration하련는 노력을 해야 한다.
        */
        if(trylockForNewTaskList() == ID_TRUE)
        {
            for (;  (IDU_LIST_IS_EMPTY(&mNewTaskList) != ID_TRUE); )
            {
                sTask = (mmcTask *)mNewTaskList.mNext->mObj;
                increaseOutTaskCount(MMT_SERVICE_THREAD_RUN_IN_BUSY,1);
                removeTask(aTaskList, sTask, ID_FALSE);
                decreaseCurAddedTaskCnt();
            }
            unlockForNewTaskList();
        }
        else
        {
            //nothing to do
        }
        
    }//else
    
    /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
    if( getAssignedTasks() < sPreRemoveTaskCount)
    {
        ideLog::logLine(IDE_LB_2, "decrease Task count(%"ID_UINT32_FMT" --> %"ID_UINT32_FMT") in ServiceThread(%"ID_UINT32_FMT")",
                        sPreRemoveTaskCount,
                        getAssignedTasks(),
                        getServiceThreadID());
    }
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   load balance 과정에서 idle max thread에서 idle min thread으로
   task 분배시 불리는 함수이다.
 */

idBool mmtServiceThread::need2Move(UInt aTaskCount2Move,UInt aMinTaskCount )
{

    UInt  sMoveRate;
    
    if(aTaskCount2Move == 0)
    {
        return ID_FALSE;
    }
    else
    {
        if(aMinTaskCount == 0)
        {
            /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
            ideLog::logLine(IDE_LB_2, "L7 Min Task Count is zero.");

            return ID_TRUE;
            
        }
        else
        {
            sMoveRate = (( aTaskCount2Move * 100) / aMinTaskCount);
            /* task 수신하는 thread의 task 변동폭 이 MIN_MIGRATION_TASK_RATE
               을 넘는 경우에만 task를 수신한다.*/
            if(sMoveRate >  mmuProperty::getMinMigrationTaskRate())
            {
                /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
                ideLog::logLine(IDE_LB_2, "L7 Task Move Rate(%"ID_UINT32_FMT") > MIN_MIGRATION_TASK_RATE(%"ID_UINT32_FMT")",
                                sMoveRate, mmuProperty::getMinMigrationTaskRate());
                return ID_TRUE;
            }
            else
            {
                return ID_FALSE;
            }
        }//else
    }//else
}



idBool mmtServiceThread::needToRemove()
{
    idBool               sRetVal;
    
    UInt    sTaskCount = getAssignedTasks();
    
    if( sTaskCount >= 2)
    {
        sRetVal = ID_TRUE;
    }
    else
    {
        //  <= 1
        sRetVal = ID_FALSE;
    }//else
    return sRetVal;
}


/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   load balance 과정에서 idle max thread에서 idle min thread으로
   task 분배하는 함수이다.
 */
void mmtServiceThread::removeFewTasks(iduList *aTaskList, UInt aMinTaskCount,UInt *aRemovedTaskCount)
{
    mmcTask *sTask;
    UInt sMaxTaskCount = getAssignedTasks();
    UInt sTaskCount;
    

    *aRemovedTaskCount = 0;
    if(sMaxTaskCount <= aMinTaskCount) /* BUG-45549 */
    {
        sTaskCount = 0;
    }
    else
    {
         sTaskCount = (sMaxTaskCount  - aMinTaskCount - 1) / 2;
    }
    // task 분배를 할 필요가 있는지를 판단.
    if (need2Move(sTaskCount,aMinTaskCount ) == ID_TRUE )
    {
        lockForNewTaskList();

        for (; (sTaskCount > 0) && (IDU_LIST_IS_EMPTY(&mNewTaskList) != ID_TRUE); sTaskCount--)
        {
            sTask = (mmcTask *)mNewTaskList.mNext->mObj;
            increaseOutTaskCount(MMT_SERVICE_THREAD_RUN_IN_IDLE,1);
            *aRemovedTaskCount +=1;
            decreaseCurAddedTaskCnt();
            removeTask(aTaskList, sTask, ID_FALSE);
        }

        unlockForNewTaskList();

        if (sTaskCount > 0)
        {
            if (trylockForThread() == ID_TRUE)
            {
                for (; (sTaskCount > 0) && (IDU_LIST_IS_EMPTY(&mWaitTaskList) != ID_TRUE); sTaskCount--)
                {
                    sTask = (mmcTask *)mWaitTaskList.mNext->mObj;
                    *aRemovedTaskCount +=1;
                    removeTask(aTaskList, sTask, ID_TRUE);
                    increaseOutTaskCount(MMT_SERVICE_THREAD_RUN_IN_IDLE,1);
                    addTaskCount(-1);

                }
                
                for (; (sTaskCount > 0) && (IDU_LIST_IS_EMPTY(&mReadyTaskList) != ID_TRUE); sTaskCount--)

                {
                    sTask = (mmcTask *)mReadyTaskList.mNext->mObj;
                    removeTask(aTaskList, sTask, ID_TRUE);
                    *aRemovedTaskCount +=1;
                    addTaskCount(-1);
                    addReadyTaskCount(-1);
                    increaseOutTaskCount(MMT_SERVICE_THREAD_RUN_IN_IDLE,1);
                }

                unlockForThread();

            }

        }//if
    }//if

}

void mmtServiceThread::removeTask(iduList *aTaskList, mmcTask *aTask, idBool aRemoveFlag)
{
    IDU_LIST_REMOVE(aTask->getThreadListNode());

    IDU_LIST_ADD_LAST(aTaskList, aTask->getThreadListNode());
    /*fix BUG-29717 When a task is terminated or migrated ,
      a busy degree of service thread which the task was assigned
      to should be decreased.*/
    decreaseBusyExperienceCnt4Task(aTask);

    if (aRemoveFlag == ID_TRUE)
    {
        IDE_ASSERT(cmiRemoveLinkFromDispatcher(mDispatcher,
                                               aTask->getLink()) == IDE_SUCCESS);

    }
}

void mmtServiceThread::getNewTask()
{
    mmcTask     *sTask;
    iduListNode *sIterator;
    iduListNode *sNodeNext;


    lockForNewTaskList();

    IDU_LIST_ITERATE_SAFE(&mNewTaskList, sIterator, sNodeNext)
    {
        sTask = (mmcTask *)sIterator->mObj;

        IDU_LIST_REMOVE(sTask->getThreadListNode());

        switch (mInfo.mServiceThreadType)
        {
            case MMC_SERVICE_THREAD_TYPE_SOCKET:
            /* PROJ-2108 Dedicated thread mode which uses less CPU */
            case MMC_SERVICE_THREAD_TYPE_DEDICATED:
                switch (sTask->getTaskState())
                {
                    case MMC_TASK_STATE_WAITING:
                        IDE_ASSERT(cmiAddLinkToDispatcher(mDispatcher, sTask->getLink()) == IDE_SUCCESS);

                        IDU_LIST_ADD_LAST(&mWaitTaskList, sTask->getThreadListNode());

                        break;

                    case MMC_TASK_STATE_READY:
                        IDE_ASSERT(cmiAddLinkToDispatcher(mDispatcher, sTask->getLink()) == IDE_SUCCESS);

                        IDU_LIST_ADD_FIRST(&mReadyTaskList, sTask->getThreadListNode());

                        addReadyTaskCount(1);
                        break;

                    default:
                        IDE_CALLBACK_FATAL("invalid task state in mmtServiceThread::getNewTask()");
                        break;
                }
                break;
            case MMC_SERVICE_THREAD_TYPE_IPC:
                    IDU_LIST_ADD_FIRST(&mReadyTaskList, sTask->getThreadListNode());
                    addReadyTaskCount(1);
                    sTask->setTaskState(MMC_TASK_STATE_READY);
                    break;
            /*PROJ-2616*/
            case MMC_SERVICE_THREAD_TYPE_IPCDA:
                    IDU_LIST_ADD_FIRST(&mReadyTaskList, sTask->getThreadListNode());
                    addReadyTaskCount(1);
                    sTask->setTaskState(MMC_TASK_STATE_READY);
                    break;
            //fix PROJ-1749
            case MMC_SERVICE_THREAD_TYPE_DA:
                    IDU_LIST_ADD_FIRST(&mReadyTaskList, sTask->getThreadListNode());
                    addReadyTaskCount(1);
                    sTask->setTaskState(MMC_TASK_STATE_READY);
                    break;
            default:
                IDE_ASSERT(0);
        }
        /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
           해당 service thread에 task가 counting되었기때문에
           add된 task갯수를 1감소 시킨다.
        */
        //fix BUG-29526 service thread의 getNewTask Function에서
        //decreaseCurAddedTaskCnt의 호출 위치가 잘못되었습니다.
        decreaseCurAddedTaskCnt();
        addTaskCount(1);
    }

    
    unlockForNewTaskList();
}

//  PROJ-1677 DEQUEUE
//  shared-mode으로 운영되는 service thread는
//  polling timeout이 의미가 있어서
//  mPollTimeout  pointer가 인자로 넘어온다 .
//  그러나  dedicated mode으로 운영되는  service thread는 polling을 하지 않고
// 무한대기를 하기 위하여 NULL이 인자로 넘어온다.
void mmtServiceThread::findReadyTask(PDL_Time_Value* aPollTimeout)
{
    iduList      sReadyList;
    iduListNode *sIterator;
    cmiLink     *sLink;
    mmcTask     *sTask;

    /*
     * Dispatcher를 이용하여 Waiting Task -> Ready Task 검색
     */


    IDE_ASSERT(cmiSelectDispatcher(mDispatcher,
                                   &sReadyList,
                                   NULL,
                                   aPollTimeout) == IDE_SUCCESS);



    IDU_LIST_ITERATE(&sReadyList, sIterator)
    {
        sLink = (cmiLink *)sIterator->mObj;
        sTask = (mmcTask *)cmiGetLinkUserPtr(sLink);
        /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
           task schedule관련된 정보 setting한다.
        */
        transitTask_READY(sTask);
    }

}

//PROJ-1677 DEQ
// lockForThread는  wait,ready task list 의 동시성제어를 위하여
// 사용된다.
void mmtServiceThread::executeTask()
{
    mmcTask      *sTask;
    mmcTaskState  sTaskState;
    IDE_RC        sExecuteResult = IDE_SUCCESS;
    mmqQueueInfo *sQueueInfo;
    //fix BUG-19647
    idBool        sQueueReady;

    IDE_CLEAR();

    sTask      = (mmcTask *)mReadyTaskList.mNext->mObj;
    /* bug-36875: null-checking for task object is necessary */
    IDE_TEST_RAISE(sTask == NULL, TaskNull);
    sTaskState = sTask->getTaskState();

    setTask(sTask);

    switch (mInfo.mServiceThreadType)
    {
        case MMC_SERVICE_THREAD_TYPE_SOCKET:
        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        case MMC_SERVICE_THREAD_TYPE_DEDICATED:

            /* bug-35395: task schedule time 을 sysstat에 추가
               패킷수신 감지직후부터 패킷해석 직전까지 시간(usec)을 계산 */
            if (iduProperty::getTimedStatistics() == IDV_TIMED_STATISTICS_ON)
            {
                addTaskScheduleTime(sTask->getSession());
            }

            IDU_LIST_REMOVE(sTask->getThreadListNode());
            addReadyTaskCount(-1);
            unlockForThread();
            sTask->setThread(this);
QUEUE_READY:
            switch (sTaskState)
            {
                case MMC_TASK_STATE_READY:
                    sExecuteResult = executeTask_READY(sTask, &sTaskState);
                    break;

                case MMC_TASK_STATE_QUEUEREADY:
                    sExecuteResult = executeTask_QUEUEREADY(sTask, &sTaskState);
                    break;

                default:
                    IDE_CALLBACK_FATAL("invalid task state in mmtServiceThread::executeTask()");
                    break;
            }
            sTask->setThread(NULL);
            setSessionID(0);
            setStatement(NULL);
            IDE_TEST_RAISE(sExecuteResult != IDE_SUCCESS,ExecuteFail);

            switch (sTaskState)
            {
                case MMC_TASK_STATE_WAITING:
                    //PROJ-1677 DEQ
                    lockForThread();
                    transitTask_WAITING(sTask);
                    break;
                case MMC_TASK_STATE_QUEUEWAIT:
                    //PROJ-1677 DEQUEUE
                    if ( isRunModeShared() == ID_TRUE )
                    {
                        //service thread run as shared mode.
                        // need to transform  shared-run mode into dedicated-run mode.
                        // MVCC로 인하여 Queue에 데이타 있음에도 불구하고 ,
                        //못읽는 경우가 있으니 이를 변신하기 전에  체크한다.
                        //fix BUG-19647
                        sQueueInfo = sTask->getSession()->getQueueInfo();
                        IDE_DASSERT(sQueueInfo != NULL);
                        sQueueInfo->lock();
                        sQueueReady = sTask->getSession()->isQueueReady();
                        sQueueInfo->unlock();
                        if(sQueueReady == ID_FALSE)
                        {    
                            transformSharedIntoDedicated();
                        }
                        else
                        {
                            sTaskState = MMC_TASK_STATE_QUEUEREADY;
                            sTask->setTaskState(MMC_TASK_STATE_EXECUTING);
                            IDE_RAISE(QUEUE_READY);
                        }
                    }
                    else
                    {
                        //service thread run as dedicated mode,nothing to do.
                    }
                    //fix BUG-30952 A session and statement of a transformed dedicated thread should be showed
                    //through V$SERVICE_THRED, V$SESSION.
                    sTask->setThread(this);
                    setSessionID(sTask->getSession()->getSessionID());
                    setStatement(sTask->getSession()->getExecutingStatement());
                    IDE_TEST_RAISE(waitForEnqueueEvent(sTask) != IDE_SUCCESS,ExecuteFail);
                    //enqueue event가 아니라 shutdown event를 받을수 있다.
                    if(mRun == ID_TRUE)
                    {
                        sTaskState = sTask->getTaskState();
                        sTask->setTaskState(MMC_TASK_STATE_EXECUTING);
                        IDE_RAISE(QUEUE_READY);
                    }
                    else
                    {
                        // thread 종료시 unlockForThread를 호출하기때문에 여기서 lock을 잡는다.
                        lockForThread();
                    }
                    break;

                default:
                    IDE_CALLBACK_FATAL("invalid task state after mmtServiceThread::executeTask()");
                    break;
            }//switch
            break;
        case MMC_SERVICE_THREAD_TYPE_IPC:
        case MMC_SERVICE_THREAD_TYPE_IPCDA: /* BUG-46163 */
        case MMC_SERVICE_THREAD_TYPE_DA: //fix PROJ-1749
            unlockForThread();
            sTask->setThread(this);

            switch (sTaskState)
            {
                case MMC_TASK_STATE_READY:
                    sExecuteResult = executeTask_READY(sTask, &sTaskState);
                    break;
                case MMC_TASK_STATE_QUEUEREADY:
                    sExecuteResult = executeTask_QUEUEREADY(sTask, &sTaskState);
                    break;
                default:
                    IDE_CALLBACK_FATAL("invalid task state in mmtServiceThread::executeTask()");
                    break;
            }
            IDE_TEST_RAISE(sExecuteResult != IDE_SUCCESS, ExecuteFail);
            switch (sTaskState)
            {
                case MMC_TASK_STATE_WAITING:
                    sTask->setTaskState(MMC_TASK_STATE_READY);
                    break;

                case MMC_TASK_STATE_QUEUEWAIT:
                    IDE_TEST_RAISE(waitForEnqueueEvent(sTask) != IDE_SUCCESS,
                                   ExecuteFail);
                    break;

                default:
                    IDE_CALLBACK_FATAL("invalid task state after mmtServiceThread::executeTask()");
                    break;
            }
            //PROJ-1677 DEQUEUE
            lockForThread();
            break;
        default:
            IDE_ASSERT(0);
    }

    return;

    IDE_EXCEPTION(ExecuteFail);
    {

        lockForThread();
        //fix PROJ-1749
        if( (MMC_SERVICE_THREAD_TYPE_IPC == mInfo.mServiceThreadType) ||
            (MMC_SERVICE_THREAD_TYPE_DA == mInfo.mServiceThreadType) ||
            (MMC_SERVICE_THREAD_TYPE_IPCDA == mInfo.mServiceThreadType) )
        {
            IDU_LIST_REMOVE(sTask->getThreadListNode());
            addReadyTaskCount(-1);
        }
        else
        {
            //fix BUG-30952 A session and statement of a transformed dedicated thread should be showed
            //through V$SERVICE_THRED, V$SESSION.
            sTask->setThread(NULL);
            setSessionID(0);
            setStatement(NULL);
        }
        terminateTask(sTask);

    }
    /* bug-36875: null-checking for task object is necessary.
       client 잘못으로 프로토콜이 async로 날라오는 경우
       task가 null일 수 있다. 방어하자 */
    IDE_EXCEPTION(TaskNull);
    {
        ideLog::log(IDE_SERVER_0,
                "executeTask: NULL task ignored. mReadyTaskCount: %u",
                mInfo.mReadyTaskCount);
        mInfo.mReadyTaskCount    = 0;
    }
    IDE_EXCEPTION_END;
}

/*
 * transitTask_XXXX 함수들은 모두 lockForThread가 획득된 상태에서만 호출되어야 한다.
 */

void mmtServiceThread::transitTask_READY(mmcTask *aTask)
{
    mmcSession  *sSession;
    idvSQL      *sStatSQL;

    /* bug-35395: task schedule time 을 sysstat에 추가
       패킷수신 감지직후부터 패킷해석 직전까지 시간(usec)을 계산 */
    sSession = aTask->getSession();
    if (sSession != NULL)
    {
        sStatSQL = sSession->getStatSQL();
        IDV_SQL_OPTIME_BEGIN(sStatSQL, IDV_OPTM_INDEX_TASK_SCHEDULE);
    }

    IDU_LIST_REMOVE(aTask->getThreadListNode());
    IDU_LIST_ADD_LAST(&mReadyTaskList, aTask->getThreadListNode());

    addReadyTaskCount(1);

    aTask->setTaskState(MMC_TASK_STATE_READY);
}


void mmtServiceThread::transitTask_WAITING(mmcTask *aTask)
{
    if (cmiProtocolContextHasPendingRequest(aTask->getProtocolContext()) == ID_TRUE)
    {
        IDU_LIST_ADD_LAST(&mReadyTaskList, aTask->getThreadListNode());

        addReadyTaskCount(1);

        aTask->setTaskState(MMC_TASK_STATE_READY);
    }
    else
    {
        IDU_LIST_ADD_LAST(&mWaitTaskList, aTask->getThreadListNode());

        aTask->setTaskState(MMC_TASK_STATE_WAITING);
    }
}

/*
 * executeTask_XXXX 함수들은 lockForThread가 획득되지 않은 상태에서 호출되므로
 * Dispatcher, Task List, Task Count, Task State 등
 * lockForThread로 보호되어야 하는 것들을 사용하거나 변경하면 안된다.
 */

IDE_RC mmtServiceThread::executeTask_READY(mmcTask *aTask, mmcTaskState *aNewState)
{
    IDE_RC      sRet;
    mmcSession *sSession;
    cmiProtocolContext* sCtx;

    mErrorFlag = ID_FALSE;


    sSession = aTask->getSession();
    if (sSession != NULL)
    {
        sSession->setIdleStartTime(mmtSessionManager::getBaseTime());
    }


    // fix BUG-29073 task executing 상태 로갱신하는 시점이
    //잘못되었다.
    // BUG-24318 IPC 일경우 세션 정보가 잘못 나옵니다.
    if ((mInfo.mServiceThreadType != MMC_SERVICE_THREAD_TYPE_IPC) &&
        (mInfo.mServiceThreadType != MMC_SERVICE_THREAD_TYPE_IPCDA))
    {
        aTask->setTaskState(MMC_TASK_STATE_EXECUTING);
    }
    
    /* proj_2160 cm_type removal */
    sCtx = aTask->getProtocolContext();
    if (cmiGetPacketType(sCtx) != CMP_PACKET_TYPE_A5)
    {
        if ( cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA )
        {
            sRet = cmiRecvIPCDA(sCtx,
                                this,
                                mmuProperty::getIPCDASleepTime());
        }
        else
        {
            sRet = cmiRecv(sCtx, this, NULL, aTask);
        }
    }
    else
    {
        sRet = cmiReadProtocolAndCallback(sCtx, this, NULL, aTask);
    }

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       task schedule  정보를  setting한다.
    */

    switch (sRet)
    {
        case IDE_CM_STOP:
            // bug-26975: codesonar: session null ref
            // 세션이 null이 가능한지는 모르지만, 방어용으로 검사추가.
            sSession = aTask->getSession();
            IDE_TEST(sSession == NULL);
            IDE_ASSERT(sSession->getQueueInfo() != NULL);

            *aNewState = MMC_TASK_STATE_QUEUEWAIT;
            break;

        case IDE_SUCCESS:
            if ( aTask->isShutdownTask() != ID_TRUE )
            {
                /* proj_2160 cm_type removal */
                if (cmiGetPacketType(sCtx) != CMP_PACKET_TYPE_A5)
                {
                    if ( cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA )
                    {
                        IDE_TEST_RAISE( cmiLinkPeerFinalizeSvrForIPCDA(sCtx) != IDE_SUCCESS, CommunicationFail);
                    }
                    else
                    {
                        IDE_TEST_RAISE(cmiSend(sCtx, ID_TRUE)
                                       != IDE_SUCCESS, FlushFail);
                    }
                }
                else
                {
                    IDE_TEST_RAISE(cmiFlushProtocol(sCtx, ID_TRUE)
                                   != IDE_SUCCESS, FlushFail);
                }
            }

            *aNewState = MMC_TASK_STATE_WAITING;
            break;

        case IDE_FAILURE:
        default:
        {
            /* BUG-46390 cmiLinkPeerFinalizeSvrForIPCDA는 반드시 호출되어야 함 */
            if ( cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA )
            {
                cmiLinkPeerFinalizeSvrForIPCDA(sCtx);
            }
            IDE_RAISE(CommunicationFail);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(FlushFail);
    {
        if (ideGetErrorCode() != cmERR_ABORT_CONNECTION_CLOSED)
        {
            mmtThreadManager::logNetworkError(MM_TRC_FLUSH_PROTOCOL_FAILED);
        }
    }
    IDE_EXCEPTION(CommunicationFail);
    {
        switch (ideGetErrorCode())
        {
            case cmERR_ABORT_CONNECTION_CLOSED:
            case idERR_ABORT_Session_Closed:
                break;

            case idERR_ABORT_Session_Disconnected:
                mmtThreadManager::logDisconnectError(aTask, ideGetErrorCode());
                break;

            default:
                mmtThreadManager::logNetworkError(MM_TRC_PROTOCOL_PROCESSING_FAILED);
                break;
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::executeTask_QUEUEREADY(mmcTask *aTask, mmcTaskState *aNewState)
{
    idBool sSuspended = ID_FALSE;
    cmiProtocolContext* sCtx;
    UChar               sPacketType;

    setSessionID(aTask->getSession()->getSessionID());
    setStatement(aTask->getSession()->getExecutingStatement());
    sCtx = aTask->getProtocolContext();

    sPacketType = cmiGetPacketType(sCtx);
    // proj_2160 cm_type removal: for A7 client
    if (sPacketType != CMP_PACKET_TYPE_A5)
    {
        if (execute(sCtx,
                    aTask->getSession()->getExecutingStatement(),
                    ID_TRUE,
                    &sSuspended,
                    NULL, NULL, NULL) != IDE_SUCCESS)
        {
            IDE_TEST_RAISE(answerErrorResult(sCtx,
                                             CMI_PROTOCOL_OPERATION(DB, Execute),
                                             0) != IDE_SUCCESS,
                           CommunicationFail);
        }
    }
    // for A5 client
    else
    {
        if (executeA5(sCtx,
                    aTask->getSession()->getExecutingStatement(),
                    ID_TRUE,
                    &sSuspended,
                    NULL, NULL) != IDE_SUCCESS)
        {
            IDE_TEST_RAISE(answerErrorResultA5(sCtx,
                                             CMI_PROTOCOL_OPERATION(DB, Execute),
                                             0) != IDE_SUCCESS,
                           CommunicationFail);
        }

    }

    if (sSuspended == ID_TRUE)
    {
        *aNewState = MMC_TASK_STATE_QUEUEWAIT;
    }
    else
    {
        /* BUG-46163 IPCDA의 경우 Pending은 사용되지 않음 (무조건 FALSE) */
        if (cmiProtocolContextHasPendingRequest(sCtx) == ID_TRUE)
        {
            IDE_TEST(executeTask_READY(aTask, aNewState) != IDE_SUCCESS);
        }
        else
        {
            // proj_2160 cm_type removal
            if (sPacketType != CMP_PACKET_TYPE_A5)
            {
                if ( cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA )
                {
                    IDE_TEST_RAISE( cmiLinkPeerFinalizeSvrForIPCDA(sCtx) != IDE_SUCCESS, CommunicationFail);
                }
                else
                {
                    IDE_TEST_RAISE(cmiSend(sCtx, ID_TRUE)
                                   != IDE_SUCCESS, FlushFail);
                }
            }
            else
            {
                IDE_TEST_RAISE(cmiFlushProtocol(sCtx, ID_TRUE)
                               != IDE_SUCCESS, FlushFail);
            }

            *aNewState = MMC_TASK_STATE_WAITING;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(FlushFail);
    {
        if (ideGetErrorCode() != cmERR_ABORT_CONNECTION_CLOSED)
        {
            mmtThreadManager::logNetworkError(MM_TRC_FLUSH_PROTOCOL_FAILED);
        }
    }
    IDE_EXCEPTION(CommunicationFail);
    {
        switch (ideGetErrorCode())
        {
            case cmERR_ABORT_CONNECTION_CLOSED:
            case idERR_ABORT_Session_Closed:
                break;

            case idERR_ABORT_Session_Disconnected:
                mmtThreadManager::logDisconnectError(aTask, ideGetErrorCode());
                break;

            default:
                mmtThreadManager::logNetworkError(MM_TRC_PROTOCOL_PROCESSING_FAILED);
                break;
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//PROJ-1677
void mmtServiceThread::terminateTask(mmcTask *aTask)
{
    IDE_RC sRC;

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       Load balance와 동시성이 관련되어 있어서,
       instruction pipe line에 대하여 대비한다.
    */

    // fix BUG-25158
    // Thread Manager가 removeAllTasks 진행 시
    // Service Thread의 Dispatcher에서 뺀 link를 다시 넣지 않도록 하기 위해
    // 현재 rollback중인 task를 NULL로 설정한다.
    ID_SERIAL_BEGIN(setTask(NULL));

    /* BUG-48515 최대 접속 사이즈가 설정된 access_list 가 있을경우 현재 접속 개수를 차감한다. */
    (void) mmuAccessList::disconnect(aTask);

    //PROJ-1677
    ID_SERIAL_EXEC(IDE_ASSERT(cmiRemoveLinkFromDispatcher(mDispatcher,
                                                          aTask->getLink()) == IDE_SUCCESS),1);

    /*fix BUG-29717 When a task is terminated or migrated ,
      a busy degree of service thread which the task was assigned
      to should be decreased.*/
    ID_SERIAL_EXEC(decreaseBusyExperienceCnt4Task(aTask),2);
    // fix BUG-25158
    // rollback시에는 Service Thread의 lock을 풀고 수행한다.
    // service thread lock 를 풀고나서 execute 를 반드시보장하기위하여...
    ID_SERIAL_EXEC(unlockForThread(),3);
    if (aTask->isShutdownTask() == ID_TRUE)
    {
        // service thread lock 를 풀고나서 execute 를 반드시보장하기위하여...
        ID_SERIAL_EXEC(sRC = mmtSessionManager::freeSession(aTask),4);
        if (sRC != IDE_SUCCESS)
        {
            mmtThreadManager::logMemoryError(MM_TRC_SESSION_FREE_FAILED);
        }
    }
    else
    {
        // service thread lock 를 풀고나서 execute 를 반드시보장하기위하여...
        ID_SERIAL_EXEC(sRC = mmtSessionManager::freeTask(aTask),4);
        if (sRC  != IDE_SUCCESS)
        {
            mmtThreadManager::logMemoryError(MM_TRC_TASK_FREE_FAILED);
        }
    }
    // service thread lock 를 풀고나서 execute 를 반드시보장하기위하여...
    ID_SERIAL_END(lockForThread());

    //PROJ-1677 DEQUEUE
    addTaskCount(-1);

    //service thread type이 IPC이 아니 socket type이고 ,
    //dedicated mode으로 running 되었다면
    //다시 shread mode  service thread으로 transform하거나 ,
    //소멸하여야 한다.
    if (mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_SOCKET)
    {
        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        if ( ( mmuProperty::getIsDedicatedMode() == 0 ) && ( isRunModeShared() == ID_FALSE ) )
        {
            transformDedicatedIntoShared();
        }//if
    }//if mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_SOCKET
}

IDE_RC mmtServiceThread::getUserInfoFromDB( idvSQL      *aStatistics, 
                                            qciUserInfo *aUserInfo )
{
    smiTrans      sTrans;
    smiStatement  sSmiStmt;
    smiStatement *sDummySmiStmt;
    qciStatement  sQciStmt;

    UInt          sStage = 0;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS ); /* PROJ-2446 */
    sStage++;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            aStatistics, /* PROJ-2446 */
                            ( SMI_TRANSACTION_UNTOUCHABLE | 
                              SMI_ISOLATION_CONSISTENT    | 
                              SMI_COMMIT_WRITE_NOWAIT ) ) 
              != IDE_SUCCESS );
    sStage++;

    IDE_TEST( sSmiStmt.begin ( sTrans.getStatistics(), /* PROJ-2446 */
                               sDummySmiStmt, 
                               SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR) != IDE_SUCCESS );
    sStage++;

    idlOS::memset(&sQciStmt, 0, ID_SIZEOF(sQciStmt));
    IDE_TEST(qci::initializeStatement(&sQciStmt, NULL, NULL, NULL) != IDE_SUCCESS);
    sStage++;

    IDE_TEST(qci::getUserInfo(&sQciStmt, &sSmiStmt, aUserInfo) != IDE_SUCCESS);
    IDE_TEST(qci::finalizeStatement(&sQciStmt) != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.destroy(NULL) != IDE_SUCCESS);
    sStage--;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch (sStage)
        {
            case 4:
                qci::finalizeStatement(&sQciStmt);
            case 3:
                IDE_ASSERT(sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);
            case 2:
                IDE_ASSERT(sTrans.commit() == IDE_SUCCESS);
            case 1:
                IDE_ASSERT(sTrans.destroy(NULL) == IDE_SUCCESS);
            default:
                break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::getUserInfoFromFile(qciUserInfo *aUserInfo)
{
    PDL_HANDLE  sPasswdFD;
    SChar       sPasswdFile[256];
    SInt        sPasswdLen;

    if( aUserInfo->mUsrDN != NULL )
    {
        SInt    sSvrDNLen = idlOS::strlen(aUserInfo->mSvrDN);
        SInt    sUsrDNLen = idlOS::strlen(aUserInfo->mUsrDN);
        IDE_TEST_RAISE( sSvrDNLen != sUsrDNLen, MMT_ERR_DNMismatched );
        IDE_TEST_RAISE(idlOS::memcmp(aUserInfo->mSvrDN, aUserInfo->mUsrDN, sSvrDNLen) != 0, MMT_ERR_DNMismatched);
        idlOS::snprintf(aUserInfo->loginID,
                        ID_SIZEOF(aUserInfo->loginID),
                        "%s",
                        "SYS");
    }

    IDE_TEST_RAISE(idlOS::strcasecmp(aUserInfo->loginID, IDP_SYSUSER_NAME) != 0, IncorrectUser);

    aUserInfo->loginUserID = QC_SYS_USER_ID;   /* BUG-41561 */
    aUserInfo->userID      = QC_SYS_USER_ID;

    idlOS::snprintf(sPasswdFile,
                    ID_SIZEOF(sPasswdFile),
                    "%s" IDL_FILE_SEPARATORS "%s",
                    idp::getHomeDir(),
                    IDP_SYSPASSWORD_FILE);

    sPasswdFD = idf::open(sPasswdFile, O_RDONLY);

    IDE_TEST_RAISE(sPasswdFD == PDL_INVALID_HANDLE, PasswdFileOpenFail);

    sPasswdLen = idf::read(sPasswdFD,
                              aUserInfo->userPassword,
                              ID_SIZEOF(aUserInfo->userPassword) - 1);

    IDE_TEST_RAISE(sPasswdLen <= 0, PasswdFileReadFail);

    aUserInfo->userPassword[sPasswdLen] = 0;
    idf::close(sPasswdFD);

    return IDE_SUCCESS;

    IDE_EXCEPTION(MMT_ERR_DNMismatched);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DNMismatched,aUserInfo->mSvrDN,aUserInfo->mUsrDN));
    }
    IDE_EXCEPTION(IncorrectUser);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NO_USER_ERROR));
    }
    IDE_EXCEPTION(PasswdFileOpenFail);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_PASSWORD_FILE_ERROR));
        IDE_CALLBACK_FATAL("password file open failed");
    }
    IDE_EXCEPTION(PasswdFileReadFail);
    {
        idf::close(sPasswdFD);

        IDE_SET(ideSetErrorCode(mmERR_FATAL_PASSWORD_FILE_ERROR));
        IDE_CALLBACK_FATAL("password file read failed");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::findSession(mmcTask           *aTask,
                                     mmcSession       **aSession,
                                     mmtServiceThread  *aThread)
{
    IDE_TEST_RAISE(aTask == NULL, NoTask);

    *aSession = aTask->getSession();

    IDE_TEST_RAISE(*aSession == NULL, NoSession);

    aThread->setSessionID((*aSession)->getSessionID());

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoTask);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SESSION_NOT_SPECIFIED));
    }
    IDE_EXCEPTION(NoSession);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SESSION_NOT_CONNECTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::checkSessionState(mmcSession *aSession, mmcSessionState aSessionState)
{
    switch (aSession->getSessionState())
    {
        case MMC_SESSION_STATE_AUTH:
            IDE_TEST_RAISE(aSessionState != MMC_SESSION_STATE_AUTH, InvalidState);
            break;

        case MMC_SESSION_STATE_READY:
            IDE_TEST_RAISE(aSessionState == MMC_SESSION_STATE_SERVICE, InvalidState);
            break;

        case MMC_SESSION_STATE_SERVICE:
            aSession->setIdleStartTime(0);
            break;

        default:
            IDE_RAISE(InvalidState);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_SESSION_STATE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::findStatement(mmcStatement     **aStatement,
                                       mmcSession        *aSession,
                                       mmcStmtID         *aStmtID,
                                       mmtServiceThread  *aThread)
{
    *aStatement = NULL;

    if (aThread->getStatement() != NULL)
    {
        if (*aStmtID == 0)
        {
            *aStatement = aThread->getStatement();
            *aStmtID    = aThread->getStatement()->getStmtID();
        }
        else if (*aStmtID == aThread->getStatement()->getStmtID())
        {
            *aStatement = aThread->getStatement();
        }
        else
        {
            IDE_TEST(mmcStatementManager::findStatement(aStatement,
                                                        aSession,
                                                        *aStmtID) != IDE_SUCCESS);

            aThread->setStatement(*aStatement);

            /* PROJ-2177 User Interface - Cancel */
            IDU_SESSION_CLR_CANCELED(*aSession->getEventFlag());
        }
    }
    else
    {
        IDE_TEST(mmcStatementManager::findStatement(aStatement,
                                                    aSession,
                                                    *aStmtID) != IDE_SUCCESS);

        aThread->setStatement(*aStatement);

        /* PROJ-2177 User Interface - Cancel */
        IDU_SESSION_CLR_CANCELED(*aSession->getEventFlag());
    }

    // BUG-33544
    // 위 조건에 관계없이 current_statement_id가 갱신되어야 함.
    aSession->getInfo()->mCurrStmtID = *aStmtID;

    /* BUG-38472 Query timeout applies to one statement. */
    if ( (*aStatement)->getTimeoutEventOccured() == ID_TRUE )
    {
        IDU_SESSION_SET_TIMEOUT( *aSession->getEventFlag(), *aStmtID );
    }
    else
    {
        IDU_SESSION_CLR_TIMEOUT( *aSession->getEventFlag() );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::checkStatementState(mmcStatement *aStatement, mmcStmtState aStmtState)
{
    IDE_TEST_RAISE(aStatement->getStmtState() < aStmtState, InvalidState);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1677 shared mode thread multiplexing function.
void mmtServiceThread::multiplexingAsShared()
{
    ULong sExecDuration;
    // fix BUG-32534 [mm-queue] The polling time of a shared service thread could be 0 in case that the service thread which had been transformed to dedicated service thread is reused
    // and that the ready task count of the service thread eventually becomes 0.
    UInt  sPollTimeOutUsec;
    IDE_CLEAR();


    mLoopCounter++;
    getNewTask();
    if (getReadyTaskCount() > 0)
    {
        setState(MMT_SERVICE_THREAD_STATE_EXECUTE);
        do
        {
            IDV_TIME_GET(&mInfo.mExecuteBegin);
            //execute완료후에 LoopCounter를 증가시킨다.
            ID_SERIAL_BEGIN(executeTask());
            ID_SERIAL_END(mLoopCounter++);
            IDV_TIME_GET(&mInfo.mExecuteEnd);
            /* fix BUG-29592 A responsibility for updating a global session time have better to be assigned
               to session manager rather than load balancer
               time resultion이 1초단위가 되어 execution time을 다음과 같이 구해야 함.
             */
            if(mInfo.mBusyExperienceCnt > 0)
            {   
                sExecDuration = IDV_TIME_DIFF_MICRO(&mInfo.mExecuteBegin,&mInfo.mExecuteEnd);
                if(sExecDuration < mmuProperty::getMultiplexingCheckInterval())
                {
                    /*fix BUG-29717 When a task is terminated or migrated ,
                    a busy degree of service thread which the task was assigned
                    to should be decreased.*/
                    decreaseBusyExperienceCnt(1);
                }
            }
        } while (getReadyTaskCount() > 0);
        mPollTimeout.set(0, MMT_SERVICE_THREAD_MIN_POLL_TIMEOUT);
    }
    else
    {

        // fix BUG-32534 [mm-queue] The polling time of a shared service thread could be 0 in case that the service thread which had been transformed to dedicated service thread is reused
        // and that the ready task count of the service thread eventually becomes 0.
        sPollTimeOutUsec = (UInt)mPollTimeout.usec() * 2;
        if(sPollTimeOutUsec > 0)
        {
            sPollTimeOutUsec = IDL_MIN(sPollTimeOutUsec,mmuProperty::getMultiplexingPollTimeout());
        }
        else
        {
            sPollTimeOutUsec = mmuProperty::getMultiplexingPollTimeout();
        }
        
        mPollTimeout.set(0, sPollTimeOutUsec);
    }

    setState(MMT_SERVICE_THREAD_STATE_POLL);
    findReadyTask(&mPollTimeout);
}

/* PROJ-2108 Dedicated thread mode which uses less CPU */
void mmtServiceThread::dedicatedThreadMode()
{
    IDE_CLEAR();
    getNewTask();

    if (getReadyTaskCount() > 0)
    {
        setState(MMT_SERVICE_THREAD_STATE_EXECUTE);
        do
        {
            IDV_TIME_GET(&mInfo.mExecuteBegin);
            executeTask();
            IDV_TIME_GET(&mInfo.mExecuteEnd);
        }while (getReadyTaskCount() > 0);
    }
    mPollTimeout.set(MMT_SERVICE_THREAD_DEDICATED_POLL_TIMEOUT, 0);
    setState(MMT_SERVICE_THREAD_STATE_POLL);
    findReadyTask(&mPollTimeout);
}
// PROJ-1677 dedicated  mode thread multiplexing function.
void mmtServiceThread::multiplexingAsDedicated()
{
    /* 
     * BUG-30656 When server stop while some service thread runs as dedicated , hang may happen.
     */
    SInt           sWaitSec;
    SInt           sWaitUSec;
    ULong sExecDuration;
    
    IDE_CLEAR();
    if (getReadyTaskCount() > 0)
    {
        setState(MMT_SERVICE_THREAD_STATE_EXECUTE);
        IDV_TIME_GET(&mInfo.mExecuteBegin);
        executeTask();
        IDV_TIME_GET(&mInfo.mExecuteEnd);
        sExecDuration = IDV_TIME_DIFF_MICRO(&mInfo.mExecuteBegin,&mInfo.mExecuteEnd);
        if( sExecDuration <= mmuProperty::getMultiplexingCheckInterval())
        {
            /*fix BUG-29717 When a task is terminated or migrated ,
              a busy degree of service thread which the task was assigned
              to should be decreased.*/
            decreaseBusyExperienceCnt(1);
        }
        else
        {
            increaseBusyExperienceCnt();
        }
        
    }
    // executeTask에서  shared에서 dedicated으로 transformation되는 경우가 있다.
    // 따라서  transformation되는 경우를 고려해야 한다.
    if(mMultiPlexingFunc != &mmtServiceThread::multiplexingAsShared)
    {
        setState(MMT_SERVICE_THREAD_STATE_POLL);
        /* 
         * BUG-30656 When server stop while some service thread runs as dedicated , hang may happen.
         */
        sWaitSec  = mmuProperty::getMaxEnqWaitTime()/1000000;
        sWaitUSec = mmuProperty::getMaxEnqWaitTime() %1000000;
        mPollTimeout.set(sWaitSec, sWaitUSec);
        findReadyTask(&mPollTimeout);
    }//if
}

// PROJ-1677 wait util enqueue event .
IDE_RC  mmtServiceThread::waitForEnqueueEvent(mmcTask* aTask)
{
    mmqQueueInfo *sQueueInfo;
    IDE_RC        sQueueResult;
    idBool        sTimeOut;
    idBool        sQueueReady;
    
    
    sQueueInfo = aTask->getSession()->getQueueInfo();
    IDE_ASSERT(sQueueInfo != NULL);

  RETRY_QWAIT:
    sQueueInfo->lock();
    // condition wait하기전에 이미 enqueue event가 발생하였는지 검사한다.
    // signal을 잃어버리지 않기 위해 ....
    if(aTask->getSession()->isQueueReady() == ID_FALSE)
    {
        //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
        // 동시성에 문제가 있음.
        // -- 진짜 enqueue evnet대기하는 경우만  task 의 상태를 Queue Wait을 변경한다.
        aTask->setTaskState(MMC_TASK_STATE_QUEUEWAIT);
        sQueueResult = sQueueInfo->timedwaitDequeue(aTask->getSession()->getQueueWaitTime(),&sTimeOut);
        sQueueReady = aTask->getSession()->isQueueReady();
        //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
        // 동시성에 문제가 있음.
        // -- 대기 상태가 풀려서 상태 갱신.
        aTask->setTaskState(MMC_TASK_STATE_QUEUEREADY);
        sQueueInfo->unlock();
        
        IDE_TEST_RAISE(sQueueResult != IDE_SUCCESS, ExecuteFail);
        // fileter broadcast event.
        if( sQueueReady == ID_FALSE)
        {
            if( IDU_SESSION_CHK_CLOSED(*aTask->getSession()->getEventFlag()))
            {
                // for terminate task.
                IDE_RAISE(ExecuteFail);
            }
            if(sTimeOut == ID_FALSE)
            {
                IDE_RAISE(RETRY_QWAIT);
                
            }//if sTimeOut
        }//if
        else
        {
            //enqueue event happend!!
        }
    }//if aTask 
    else
    {

        aTask->setTaskState(MMC_TASK_STATE_QUEUEREADY);
        sQueueInfo->unlock();
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ExecuteFail);
    {
        //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
        // 동시성에 문제가 있음.
        aTask->getSession()->endQueueWait();
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

//PROJ-1677,need to transform  shared-run mode into dedicated-run mode
//1.shared mode에서 dedicate mode으로 running하기 위하여 service list에서 나간다.
// - 나가는 과정에서 가지고 있는 task list 상의 task들을 정리한다.
//2. multiplexing function 을  mtServiceThread::multiplexingAsShared으로 변경한다.
void  mmtServiceThread::transformSharedIntoDedicated()
{
    // service thread list 나오는 방법을  확실히 한다.
    if( mmtThreadManager::removeFromServiceThrList(this) == IDE_SUCCESS)
    {
        mMultiPlexingFunc = &mmtServiceThread::multiplexingAsDedicated;
        IDE_DASSERT(IDU_LIST_IS_EMPTY(&mNewTaskList) == ID_TRUE);
        IDE_DASSERT(IDU_LIST_IS_EMPTY(&mWaitTaskList) == ID_TRUE);
        IDE_DASSERT(IDU_LIST_IS_EMPTY(&mReadyTaskList) == ID_TRUE);
        //fix BUG-19323    
        mInfo.mRunMode   = MMT_SERVICE_THREAD_RUN_DEDICATED;
    }
    else
    {
        // task양도하는 과정에서 새로운 서비스 쓰레드를 생성하다가 ,
        // 실패하였다. 따라서 shared mode를 유지 한다.
    }

}

//PROJ-1677,need to transform dedicated-run mode into  shared-run mode.
//1. multiplexing function  as mtServiceThread::multiplexingAsShared
//2. dedicated service thread를 shared service thread list에
//반납시도를 한다.
// 만약  현재 service list에 있는 서비스 쓰레드 갯수가
// MULTIPLEXING_MAX_THREAD_COUNT보다 크거나 같으면
// 반납을 하지않고 스스로 소멸을 한다.

void mmtServiceThread::transformDedicatedIntoShared()
{
    mLoopCounter = 1;
    mLoopCheck   = 0;
    mMultiPlexingFunc = &mmtServiceThread::multiplexingAsShared;
    //fix BUG-19323    
    mInfo.mRunMode           = MMT_SERVICE_THREAD_RUN_SHARED;
    //escape dead lock 
    unlockForThread();
    // try to add this service thread in service thread list .
    if(mmtThreadManager::try2AddServiceThread(this) == ID_FALSE)
    {
        // MULTIPLEXING_MAX_THREAD_COUNT제약 조건에 걸려 소멸하여야 한다.
        mRun  = ID_FALSE;
        /* fix BUG-30322 In case of terminating a transformed dedicated service thread,
           the count of service can be negative . */
        /* dedicated service thread에서 shared service thread로 transform이 실패
           하였기때문에  multiplexing func, run mode를 원복해야 한다 */
        mMultiPlexingFunc = &mmtServiceThread::multiplexingAsDedicated;
        mInfo.mRunMode    =  MMT_SERVICE_THREAD_RUN_DEDICATED;

    }
    lockForThread();
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   a service thread가 busy 인지 여부를 판단하는 함수
*/

idBool mmtServiceThread::checkBusy()
{
    idBool sBusy; 
    
    if (trylockForThread() == ID_TRUE)
    {
        if (mLoopCheck == mLoopCounter)
        {
            sBusy  = ID_TRUE;
            increaseBusyExperienceCnt();
        }
        else
        {
            mLoopCheck = mLoopCounter;
            sBusy = ID_FALSE;
        }

        unlockForThread();
    }
    else
    {
        if (mLoopCheck == mLoopCounter)
        {
            sBusy  = ID_TRUE;
        }
        else
        {
            sBusy = ID_FALSE;
        }
    }
    return sBusy;
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
*/

idBool mmtServiceThread::trylockForNewTaskList()
{
    //fix PROJ-1749
    if (mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_DA)
    {
        return ID_TRUE;
    }

    idBool sSuccess;

    mNewTaskListMutex.trylock(sSuccess);

    return sSuccess;
}

idBool mmtServiceThread::trylockForThread()
{
    //fix PROJ-1749
    if (mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_DA)
    {
        return ID_TRUE;
    }

    idBool sSuccess;

    mMutex.trylock(sSuccess);

    return sSuccess;
}
void mmtServiceThread::lockForThread()
{
    //fix PROJ-1749
    if (mInfo.mServiceThreadType != MMC_SERVICE_THREAD_TYPE_DA)
    {
        //fix BUG-29514
        IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    }
}

void mmtServiceThread::unlockForThread()
{
    //fix PROJ-1749
    if (mInfo.mServiceThreadType != MMC_SERVICE_THREAD_TYPE_DA)
    {
        //fix BUG-29514.
        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    }
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   load balance과정에서 task in/out를 기록하는 함수.
*/
void mmtServiceThread::increaseInTaskCount(mmtServiceThreadRunStatus  aRunStatus,
                                           UInt                       aCount)
{
    if(aRunStatus == MMT_SERVICE_THREAD_RUN_IN_IDLE)
    {
        mInfo.mInTaskCntFromIdle  +=  aCount;
    }
    else
    {
        mInfo.mInTaskCntFromBusy  += aCount;
    }
}

void mmtServiceThread::increaseOutTaskCount(mmtServiceThreadRunStatus  aRunStatus,
                                            UInt                       aCount)
{
    if(aRunStatus == MMT_SERVICE_THREAD_RUN_IN_IDLE)
    {
        mInfo.mOutTaskCntInIdle  +=  aCount;
    }
    else
    {
        mInfo.mOutTaskCntInBusy  += aCount;
    }
}


void  mmtServiceThread::increaseBusyExperienceCnt()
{
    /*fix BUG-29599 while altibase startup,
      a load balancer should not increase a busy experice count of a service thread.*/
    
    if (mmm::getCurrentPhase() == MMM_STARTUP_SERVICE)
    {
        mInfo.mBusyExperienceCnt += 1;
        /*fix BUG-29717 When a task is terminated or migrated ,
          a busy degree of service thread which the task was assigned
          to should be decreased.*/
        if(mTask != NULL)
        {
            mTask->incBusyExprienceCnt();
        }   
    }
}

void  mmtServiceThread::decreaseBusyExperienceCnt(UInt aDelta)
{
    /*fix BUG-29717 When a task is terminated or migrated ,
      a busy degree of service thread which the task was assigned
      to should be decreased.*/
    if(mTask != NULL)
    {
        mTask->decBusyExprienceCnt(aDelta);
    }
    if(mInfo.mBusyExperienceCnt >= aDelta)
    {    
        mInfo.mBusyExperienceCnt -= aDelta;
    }
    else
    {
        mInfo.mBusyExperienceCnt = 0;
    }

}

void  mmtServiceThread::decreaseBusyExperienceCnt4Task(mmcTask * aTask)
{
    UInt sBusyExprienceCnt;
    
    if(aTask != NULL)
    {
        sBusyExprienceCnt = aTask->getBusyExprienceCnt();
        if(mInfo.mBusyExperienceCnt >= sBusyExprienceCnt)
        {    
            mInfo.mBusyExperienceCnt -= sBusyExprienceCnt;
        }
        else
        {
            mInfo.mBusyExperienceCnt = 0;
        }
        aTask->decBusyExprienceCnt(sBusyExprienceCnt);
    }
}

/* PROJ-2108 Dedicated thread mode which uses less CPU */
IDE_RC mmtServiceThread::signalToServiceThread(mmtServiceThread *aThread)
{
    IDE_ASSERT(aThread->mMutexForServiceThreadSignal.lock(NULL) == IDE_SUCCESS);
    IDE_TEST(aThread->mServiceThreadCV.signal() != IDE_SUCCESS);
    IDE_ASSERT(aThread->mMutexForServiceThreadSignal.unlock() == IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* bug-35395: task schedule time 을 sysstat에 추가
   세션의 스케줄 시간을 세션 통계정보에 반영(누적, 최대).
   sStatSQL : 현재의 시간측정값(begin, end)이 mOpTime 에 있음
   sStatSess: 누적치, 최대치를 저장할 세션 stat 정보
   */
IDE_RC mmtServiceThread::addTaskScheduleTime(mmcSession* aSession)
{
    idvSQL           *sStatSQL;
    idvSession       *sStatSess;
    idvOperTimeIndex  sIndexTaskSched = IDV_OPTM_INDEX_TASK_SCHEDULE;
    ULong             sElaTime;
    ULong            *sSchedMaxTime;

    if (aSession != NULL)
    {
        sStatSQL  = aSession->getStatSQL();
        sStatSess = aSession->getStatistics();
        IDV_SQL_OPTIME_END(sStatSQL, sIndexTaskSched);

        if ( IDV_TIME_AVAILABLE() )
        {
            sElaTime = IDV_TIMEBOX_GET_ELA_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL, sIndexTaskSched));

            /* 경과시간이 스케줄 단위시간 이상인 경우만 통계에 반영 */
            if (sElaTime >= (ULong) mmuProperty::getMultiplexingCheckInterval())
            {
                /* 세션의 스케줄 경과시간을 누적시킨다 */
                IDV_SESS_ADD(sStatSess, IDV_STAT_INDEX_OPTM_TASK_SCHEDULE, sElaTime);

                /* 세션의 스케줄 최대 경과시간을 갱신한다 */
                sSchedMaxTime =
                    &(sStatSess->mStatEvent[IDV_STAT_INDEX_OPTM_TASK_SCHEDULE_MAX].mValue);
                if (sElaTime > *sSchedMaxTime)
                {
                    *sSchedMaxTime = sElaTime;
                }
            }

            /* idvTimeBox.mATD.mAccumTime (누적시간)는
               OPTIME_END시에 누적시간 계산되고,
               endStmt시에 addOpTimeToSession를 통해
               통계정보에 반영된다.
               하지만 stmt 단위이고, 무조건 누적이므로
               여기서는 사용하지 않고 대신 GET_ELA_TIME을 사용.
               따라서, 통계정보에 반영되지 않도록 clear 해 준다 */
            IDV_TIMEBOX_INIT_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(sStatSQL, sIndexTaskSched ) );
        }

    }

    return IDE_SUCCESS;
}

/* bug-33841: ipc thread's state is wrongly displayed
   IPC 서비스 쓰레드의 상태값을 정확하게 세팅하기 위해
   패킷 수신 바로 후에 호출되는 callback 함수이다
   cm에서 호출되는 mm 함수이므로 콜백처리하였다 */
IDE_RC mmtServiceThread::setExecuteCallback(void* aThread, void* aTask)
{
    mmtServiceThread* sThread = (mmtServiceThread*) aThread;
    mmcTask*          sTask   = (mmcTask*) aTask;

    /* IPC만 해당된다. 이 코드의 원래 위치는
       executeTask_READY에서 cmiRead...호출 바로 전이었다 */
    if ((sThread != NULL) && (sTask != NULL))
    {
        /*PROJ-2616*/
        if( (sThread->mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_IPC) ||
            (sThread->mInfo.mServiceThreadType == MMC_SERVICE_THREAD_TYPE_IPCDA))
        {
            sThread->setState(MMT_SERVICE_THREAD_STATE_EXECUTE);
            IDV_TIME_GET(&(sThread->mInfo.mExecuteBegin));
            sTask->setTaskState(MMC_TASK_STATE_EXECUTING);
        }
        else
        {
            /* nothing */
        }
    }
    else
    {
        /* nothing */
    }
    return IDE_SUCCESS;
}

