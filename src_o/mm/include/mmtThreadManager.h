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

#ifndef _O_MMT_THREAD_MANAGER_H_
#define _O_MMT_THREAD_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idt.h>
#include <cm.h>
#include <mmcDef.h>
#include <mmtDef.h>

#define MM_IP_VER_STRLEN_ON_BOOT  80  /* proj-1538 ipv6 + bug-30541 */

class mmcTask;
class mmtDispatcher;
class mmtServiceThread;

typedef struct mmtDBProtocol4PerfV
{
    UChar   * mOpName;
    UInt      mOpID;
    ULong     mCount;
} mmtDBProtocol4PerfV;

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   BUG-29335A performance-view about service thread should be strengthened for problem tracking
*/
typedef struct mmtServiceThreadMgrInfo
{
    UInt                  mAddThrCount;
    UInt                  mRemoveThrCount;
    UInt                  mReservedThrCnt;
} mmtServiceThreadMgrInfo;

/* BUG-35356 Memory violation may occur when calculating CPU busy count */
typedef struct mmtCoreInfo
{
    UInt mCoreNumber;
    UInt mCoreBusyCount;
} mmtCoreInfo;

class mmtThreadManager
{
private:
    static cmiLink           *mListenLink[CMI_LINK_IMPL_MAX];
    static mmtDispatcher     *mDispatcher[CMI_DISPATCHER_IMPL_MAX];

    static idBool             mDispatcherRun;

    static iduMutex           mMutex;

    static iduMemPool         mServiceThreadPool;
    static UInt               mServiceThreadLastID;

    static UInt               mServiceThreadSocketCount;
    static iduList            mServiceThreadSocketList;

    //PROJ-1677 DEQUEUE
    static iduList            mTempDedicateServiceThrList;
    static UInt               mServiceThreadIPCCount;
    static iduList            mServiceThreadIPCList;
    static iduList            mServiceThreadFreeList;

    /*PROJ-2616*/
    static UInt               mServiceThreadIPCDACount;
    static iduList            mServiceThreadIPCDAList;

    static mmtServiceThread **mIpcServiceThreadArray;

    static iduMutex          *mIpcServiceThreadMutex;
    static iduCond           *mIpcServiceThreadCV;
    static mmtServiceThreadMgrInfo mServiceThreadMgrInfo;

    /*PROJ-2616*/
    static mmtServiceThread **mIPCDAServiceThreadArray;
    static iduMutex          *mIPCDAServiceThreadMutex;
    static iduCond           *mIPCDAServiceThreadCV;

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    static iduCond            mThreadManagerCV;
    static iduMutex           mMutexForThreadManagerSignal;
    static UInt               mCoreCount;
    static idtCPUSet          mThreadManagerPset;
    /* BUG-35356 Memory violation may occur when calculating CPU busy count */
    static mmtCoreInfo       *mCoreInfoArray;

public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC setupProtocolCallback();

    /*PROJ-2616*/
    static mmtServiceThread *getIPCDAServiceThread(UInt aChannelID);

    static IDE_RC addListener(cmiLinkImpl aLinkImpl, SChar *aMsgBuffer, UInt aMsgBufferSize);

    static IDE_RC startListener();
    static IDE_RC stopListener();

    static IDE_RC startServiceThreads();
    static IDE_RC stopServiceThreads();
    //PROJ-1677 DEQUEUE
    static idBool try2AddServiceThread(mmtServiceThread*  aServiceThread);
    static IDE_RC removeFromServiceThrList(mmtServiceThread*  aServiceThread);
    static UInt   getServiceThreadSocketCount();
    static UInt   getServiceThreadIPCCount();

    /*PROJ-2616*/
    static UInt   getServiceThreadIPCDACount();

    //fix PROJ-1749
    static IDE_RC allocServiceThread(mmcServiceThreadType       aServiceThreadType,
                                     //fix BUG-19464
                                     mmtServiceThreadStartFunc  aServiceThreadStartFunc,
                                     mmtServiceThread          **aServiceThread);

    static IDE_RC freeServiceThread(mmtServiceThread *aServiceThread);


private:
    /*
     * Internal Functions
     */

    static void   lock();
    static void   unlock();

    static IDE_RC makeListenArg(cmiListenArg *aListenArg,
                                cmiLinkImpl   aImpl,
                                SChar        *aMsgBuffer,
                                UInt          aMsgBufferSize);

    static IDE_RC allocDispatcher(cmiDispatcherImpl aImpl);
    static IDE_RC freeDispatcher(cmiDispatcherImpl aImpl);

    //fix BUG-19464
    static void   addServiceThread(mmtServiceThread    *aServiceThread);
    
    static IDE_RC startServiceThread(mmcServiceThreadType aServiceThreadType,UInt aNewThrCnt, UInt *aAllocateThrCnt);
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       fix BUG-29083 When a task is assigned to service-thread, dispatcher make effort to
       choose a proper service-thread.
    */
    static mmtServiceThread* chooseServiceThread();

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    static mmtServiceThread* chooseDedicatedServiceThread();
    static void   freeServiceThreads(iduList *aList);

    static IDE_RC addSocketTask(mmcTask *aTask);
    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    static IDE_RC addDedicatedTask(mmcTask *aTask);
    static IDE_RC addIpcTask(mmcTask *aTask);
    /* PROJ-2616 */
    static IDE_RC addIPCDATask(mmcTask *aTask);

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       The list of idle thread should be sorted by a degree of busy
    */

    static void  insert2IdleThrList(iduList           *aIdleThreadList,
                                    mmtServiceThread  *aIdleThread);

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       load balance function의 가독성을 높이기 위하여
       추가된 함수들입니다.
    */

    static idBool  needToCreateThr(UInt  aSumOfTaskCnt,
                                   UInt  aThrCnt,
                                   UInt  aAvgTaskCntPerIdleThr,
                                   UInt aAvgTaskCntPerThr);
    
        
    static void    assignSocketTask(mmcTask *aTask);
    

    static void    tryToStopIdleThreads(UInt aThrCnt,
                                        iduList *aIdleThreadList);
    
    static void    distributeTasksAmongIdleThreads(iduList *aIdleThreadList);
    
    static IDE_RC  adjustCountForNewThr(UInt aThrCnt,
                                        UInt aSumOfTaskCnt,
                                        UInt *aCntForNewThr);
    
    static IDE_RC  createNewThr(UInt aThrCnt,
                                SInt aCntForNewThr);
    

public:
    /*
     * Service Thread Multiplexing
     */

    static void   checkServiceThreads();

public:
    /* 
     * PROJ-2108 Dedicated thread mode which uses less CPU
     * Function to remove expanded idle service threads in dedicated thread mode
     */
    static void   stopExpandedIdleServiceThreads();

public:
    /*
     * Dispatcher Callback
     */

    static IDE_RC dispatchCallback(cmiLink *aLink, cmiDispatcherImpl aDispatcherImpl);

public:
    /*
     * Service Thread Callback
     */

    static void   serviceThreadStarted(mmtServiceThread    *aServiceThread,
                                       mmcServiceThreadType aServiceThreadType,
                                       UInt                *aServiceThreadID);
    //fix BUG-19464
    static void   noServiceThreadStarted(mmtServiceThread    *aServiceThread,
                                         mmcServiceThreadType aServiceThreadType,
                                         UInt                *aServiceThreadID);
    
    static void   serviceThreadStopped(mmtServiceThread *aServiceThread);

public:
    /*
     * Thread Callback from SM
     */

    static IDE_RC threadSleepCallback(ULong aMicroSec, idBool *aWaited);
    static IDE_RC threadWakeupCallback();

public:
    /*
     * Functions for Thread
     */

    static IDE_RC setupDefaultThreadSignal();

public:
    /*
     * Logging
     */

    static void   logError(const SChar *aMessage);
    static void   logNetworkError(const SChar *aMessage);
    static void   logMemoryError(const SChar *aMessage);
    static void   logDisconnectError(mmcTask *aTask, UInt aErrorCode);

    static idBool logCurrentSessionInfo(ideLogEntry &aLog);

public:
    /*
     * Fixed Table Builder
     */

    static IDE_RC buildRecordForServiceThread(idvSQL              * /*aStatistics*/,
                                              void *aHeader, 
                                              void *aDumpObj,
                                              iduFixedTableMemory *aMemory);
    static IDE_RC buildRecordForServiceThreadMgr(idvSQL              * /*aStatistics*/,
                                                 void *aHeader,
                                                 void * /* aDumpObj */,
                                                 iduFixedTableMemory *aMemory);
    

    static IDE_RC buildRecordForDBProtocol(idvSQL              * /*aStatistics*/,
                                           void * aHeader,
                                           void * /* aDumpObj */,
                                           iduFixedTableMemory * aMemory);
    
    /*
     * IPC Service Thread Mutex
     */

    static iduMutex*   getIpcServiceThreadMutex(SInt aIpcServiceThreadID);
    static iduCond*    getIpcServiceThreadCV(SInt aIpcServiceThreadID);

    /*PROJ-2616*/
    static iduMutex*   getIPCDAServiceThreadMutex( SInt aIPCDAServiceThreadID );
    static iduCond*    getIPCDAServiceThreadCV( SInt aIPCDAServiceThreadID );

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    static IDE_RC      signalToThreadManager(void);
    static UInt        getCoreIdxToAssign(void);
    static void        decreaseCoreBusyCount(UInt aSelectedCore);
    /* BUG-35356 Memory violation may occur when calculating CPU busy count */
    static UInt        getCoreNumberFromIdx(UInt aSelectedCoreIdx);

};

inline UInt mmtThreadManager::getServiceThreadSocketCount()
{
    return mServiceThreadSocketCount;
}

inline UInt mmtThreadManager::getServiceThreadIPCCount()
{
    return mServiceThreadIPCCount;
}

/*PROJ-2616*/
inline UInt mmtThreadManager::getServiceThreadIPCDACount()
{
    return mServiceThreadIPCDACount;
}

inline void mmtThreadManager::lock()
{
// do TASK-3873 code-sonar
    IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
}

inline void mmtThreadManager::unlock()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
}

/*PROJ-2616*/
inline mmtServiceThread *mmtThreadManager::getIPCDAServiceThread(UInt aChannelID)
{
    return mIPCDAServiceThreadArray[aChannelID];
}

#endif
