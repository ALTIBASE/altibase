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

#ifndef _O_MMT_SERVICE_THREAD_H_
#define _O_MMT_SERVICE_THREAD_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idtBaseThread.h>
#include <cm.h>
#include <qci.h>
#include <mmcDef.h>
#include <mmcStatement.h>
#include <mmuProperty.h>
#include <mmtDef.h>

class mmcTask;
class mmcSession;

class mmtServiceThread : public idtBaseThread
{    
private:
    //PROJ-1677 DEQUEUE
    // shared, dedicated mode service thread의
    // multiplexing function pointer을 다음과 같이 정의한다.
    typedef void (mmtServiceThread::*multiPlexingFunc)();
    mmtServiceThreadInfo              mInfo;

    cmiDispatcher                     *mDispatcher;

    mmcTask                           *mTask;
    mmcStatement                      *mStatement;
    //PROJ-1677 DEQUEUE
    mmtServiceThread::multiPlexingFunc mMultiPlexingFunc;
    

    UInt                               mLoopCounter;
    UInt                               mLoopCheck;

    idBool                             mRun;
    idBool                             mErrorFlag;

    iduListNode                        mThreadListNode;
    iduListNode                        mCheckThreadListNode;

    iduList                            mNewTaskList;
    iduList                            mWaitTaskList;
    iduList                            mReadyTaskList;

    iduMutex                           mMutex;
    iduMutex                           mNewTaskListMutex;

    PDL_Time_Value                     mPollTimeout;
    //fix BUG-19464
    mmtServiceThreadStartFunc          mServiceThreadStartFunc;

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    iduCond         mServiceThreadCV;
    iduMutex        mMutexForServiceThreadSignal;


public:
    mmtServiceThread();
    //fix BUG-19464
    IDE_RC initialize(mmcServiceThreadType      aServiceThreadType,
                      mmtServiceThreadStartFunc aServiceThreadStartFunc);
    IDE_RC finalize();

    /* BUG-38641 Apply PROJ-2379 Thread Renewal on MM */
    IDE_RC initializeThread();
    void   finalizeThread();

    void   run();
    void   stop();

    idBool isRun();
    idBool checkBusy();
    void   addTask(mmcTask *aTask);
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       load balance 이력을 파 악하기 위하여 function
       signature를 변경합니다.
     */
    void   addTasks(iduList *aTaskList,UInt aTaskCnt);
    void   removeAllTasks(iduList *aTaskList, mmtServiceThreadLock aLock);
    void   removeFewTasks(iduList *aTaskList,UInt aTaskCount,UInt *aRemovedTaskCount);
    //PROJ-1677 DEQUEUE
    inline void    moveTaskLists(mmtServiceThread* aTargetServiceThr);
    inline idBool  isRunModeShared();
    //fix PROJ-1749
    void   getNewTask();
    void   executeTask();

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    IDE_RC signalToServiceThread(mmtServiceThread *aThread);

private:
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
     */
    idBool trylockForNewTaskList();
    idBool trylockForThread();
    void   lockForThread();
    void   unlockForThread();

    void   lockForNewTaskList();
    void   unlockForNewTaskList();

    void   removeTask(iduList *aTaskList, mmcTask *aTask, idBool aRemoveFlag);

    //PROJ-1677  shared-mode으로 운영되는 service thread는
    //polling timeout이 의미가 있어서
    //mPollTimeout  pointer가 인자로 넘어온다.
    //그러나  dedicated mode으로 운영되는  service thread는 polling을 하지 않고
    //무한대기를 하기 위하여 NULL이 인자로 넘어온다.
    void   findReadyTask(PDL_Time_Value* aPollTimeout);
    void   multiplexingAsShared();
    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    void   dedicatedThreadMode();
    void   multiplexingAsDedicated();

    //PROJ-1677 DEQUEUE
    IDE_RC waitForEnqueueEvent(mmcTask* aTask);
    void   transformSharedIntoDedicated();
    void   transformDedicatedIntoShared();
    
    void   transitTask_WAITING(mmcTask *aTask);
    void   transitTask_READY(mmcTask *aTask);
    IDE_RC executeTask_READY(mmcTask *aTask, mmcTaskState *aNewState);
    IDE_RC executeTask_QUEUEREADY(mmcTask *aTask, mmcTaskState *aNewState);
    void   terminateTask(mmcTask *aTask);

    IDE_RC execute(cmiProtocolContext *aProtocolContext,
                   mmcStatement       *aStatement,
                   idBool              aDoAnswer,
                   idBool             *aSuspended,
                   UInt               *aResultSetCount,
                   SLong              *aAffectedRowCount,
                   SLong              *aFetchedRowCount);
    
    IDE_RC executeIPCDASimpleQuery(cmiProtocolContext *aProtocolContext,
                                   mmcStatement       *aStatement,
                                   UShort             *aBindColInfo,
                                   idBool              aDoAnswer,
                                   idBool             *aSuspended,
                                   UInt               *aResultSetCount,
                                   SLong              *aAffectedRowCount,
                                   SLong              *aFetchedRowCount,
                                   UChar              *aBindBuffer);

    /* BUG-440705 IPCDA fetch */
    IDE_RC fetchIPCDA(cmiProtocolContext *aProtocolContext,
                      mmcSession         *aSession,
                      mmcStatement       *aStatement,
                      UShort              aResultSetID,
                      UShort              aColumnFrom,
                      UShort              aColumnTo,
                      UInt                aRecordCount);

    /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
    IDE_RC fetch(cmiProtocolContext *aProtocolContext,
                 mmcSession         *aSession,
                 mmcStatement       *aStatement,
                 UShort              aResultSetID,
                 UShort              aColumnFrom,
                 UShort              aColumnTo,
                 UInt                aRecordCount);

    /* proj_2160 cm_type removal */
    IDE_RC executeA5(cmiProtocolContext *aProtocolContext,
                   mmcStatement       *aStatement,
                   idBool              aDoAnswer,
                   idBool             *aSuspended,
                   UInt               *aResultSetCount,
                   ULong              *aAffectedRowCount);
    
    IDE_RC fetchA5(cmiProtocolContext *aProtocolContext,
                 mmcSession         *aSession,
                 mmcStatement       *aStatement,
                 UShort              aResultSetID,
                 UShort              aColumnFrom,
                 UShort              aColumnTo,
                 UShort              aFetchCount);

    // PROJ-1518
    IDE_RC atomicCheck(mmcStatement * aStatement, UChar *aOption);

    void atomicInit(mmcStatement * aStatement);

    IDE_RC atomicBegin(mmcStatement * aStatement);

    IDE_RC atomicExecute(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext);

    IDE_RC atomicEnd(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext);

    IDE_RC atomicExecuteA5(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext);

    IDE_RC atomicEndA5(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext);

public:
    /*
     * Accessor
     */

    mmtServiceThreadInfo  *getInfo();
    iduListNode           *getThreadListNode();
    iduListNode           *getCheckThreadListNode();

    UInt                   getServiceThreadID();
    //fix BUG-19464
    void                   setServiceThreadID(UInt aServiceThrID);
    
    void                   setIpcID(UInt aIpcID);
    UInt                   getIpcID();

    mmcServiceThreadType   getServiceThreadType();

    mmcStatement          *getStatement();

    mmtServiceThreadState  getState();
    UInt                   getTaskCount();
    UInt                   getReadyTaskCount();
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       load balance 이력을 파 악하기 위하여 function을 추가합니다.
     */

    void                   increaseInTaskCount(mmtServiceThreadRunStatus  aRunStatus,
                                               UInt                       aCount);
    void                   increaseOutTaskCount(mmtServiceThreadRunStatus  aRunStatus,
                                                UInt                       aCount);
    void                   increaseBusyExperienceCnt();
    

    inline UInt            getLifeSpan();
    inline void            incLifeSpan(UInt aLifeSpan);
    inline void            decLifeSpan();
    inline UInt            getAssignedTasks() ;
    
    ULong                  getBusyDegree(idBool aIncludeToCurrentBusy);
    idBool                 needToRemove();

    /* BUG-38384 A task in a service thread can be a orphan */
    inline idBool          getRemoveAllTasks();
    inline void            setRemoveAllTasks(idBool aFlag);
    
    /* BUG-38496 Notify users when their password expiry date is approaching */
    inline idBool          getErrorFlag();
    inline void            setErrorFlag( idBool aErrorFlag );

private:
    void                   setState(mmtServiceThreadState aState);

    void                   addTaskCount(SInt aDelta);
    void                   addReadyTaskCount(SInt aDelta);

    void                   setSessionID(mmcSessID aSessionID);
    void                   setTask(mmcTask *aTask);

    void                   setStatement(mmcStatement *aStatement);
    idBool                 need2Move(UInt aTaskCount2Move,UInt aMinTaskCount);
    /*fix BUG-29717 When a task is terminated or migrated ,
      a busy degree of service thread which the task was assigned
      to should be decreased.*/
    void                   decreaseBusyExperienceCnt(UInt aDelta);
    void                   decreaseBusyExperienceCnt4Task(mmcTask * aTask);
    
    void                   decreaseCurAddedTaskCnt();
    IDE_RC addTaskScheduleTime(mmcSession* aSession);

    /* TASK-7218 Handling Multi-Error for SD */
    IDE_RC answerNormalErrorResult(cmiProtocolContext *aProtocolContext,
                                   UChar               aOperationID,
                                   UInt                aErrorIndex,
                                   mmcSession         *aSession);
    IDE_RC answerShardErrorResult(cmiProtocolContext *aProtocolContext,
                                  UChar               aOperationID,
                                  UInt                aErrorIndex,
                                  mmcSession         *aSession,
                                  UChar               aExecuteOption);
    IDE_RC answerMultiErrorResult(cmiProtocolContext *aProtocolContext,
                                  UChar               aOperationID,
                                  UInt                aErrorIndex,
                                  smSCN               aSCN,
                                  mmcSession         *aSession);

public:
    static IDE_RC getUserInfoFromDB(idvSQL *aStatistics, qciUserInfo *aUserInfo);
    static IDE_RC getUserInfoFromFile(qciUserInfo *aUserInfo);
    IDE_RC answerErrorResult(cmiProtocolContext *aProtocolContext,
                             UChar               aOperationID,
                             UInt                aErrorIndex,
                             mmcSession         *aSession = NULL,
                             UChar               aExecuteOption = CMP_DB_EXECUTE_NONE);

private:
    static IDE_RC findSession(mmcTask *aTask, mmcSession **aSession, mmtServiceThread *aThread);
    static IDE_RC checkSessionState(mmcSession *aSession, mmcSessionState aSessionState);

    static IDE_RC findStatement(mmcStatement **aStatement, mmcSession *aSession, mmcStmtID *aStmtID, mmtServiceThread *aThread);
    static IDE_RC checkStatementState(mmcStatement *aStatement, mmcStmtState aStmtState);

    // To Fix BUG-15361
    static IDE_RC validateDBName( SChar * aDBName, UInt aDBNameLen );

    /* proj_2160 cm_type removal */
    IDE_RC answerErrorResultA5(cmiProtocolContext *aProtocolContext, UChar aOperationID, UShort aErrorIndex);

public:
    /*
     * QP Database Callbacks
     */

    static IDE_RC createDatabase(idvSQL * /*aStatistics*/, void *aArg);
    static IDE_RC dropDatabase(idvSQL * /*aStatistics*/, void *aArg);
    static IDE_RC startupDatabase(idvSQL * /*aStatistics*/, void *aArg);
    static IDE_RC shutdownDatabase(idvSQL * /*aStatistics*/, void *aArg);
    static IDE_RC commitDTX(idvSQL * /*aStatistics*/, void *aArg);
    static IDE_RC rollbackDTX(idvSQL * /*aStatistics*/, void *aArg);
    static IDE_RC closeSession(idvSQL * /*aStatistics*/, void *aArg);

    /*
     * QP Out-Bind Lob Callback
     */

    static IDE_RC outBindLobCallback(void         *aMmSession,
                                     smLobLocator *aOutLocator,
                                     smLobLocator *aOutFirstLocator,
                                     UShort        aOutCount);
    static IDE_RC closeOutBindLobCallback(void         *aMmSession,
                                          smLobLocator *aOutFirstLocator,
                                          UShort        aOutCount);

    static IDE_RC setParamData4RebuildCallback( idvSQL * aStatistics,
                                                void   * aBindParam,
                                                void   * aTarget,
                                                UInt     aTargetSize,
                                                void   * aTemplate,
                                                void   * aSession4Rebuild,
                                                void   * aBindData);
    
    static IDE_RC setExecuteCallback(void* aThread, void* aTask);

public:
    /*
     * Protocol Callbacks
     *
     * static IDE_RC callbackFunction(cmiProtocolContext *aProtocolContext,
     *                                cmiProtocol        *aProtocol,
     *                                void               *aSessionOwner,
     *                                void               *aUserContext);
     */

    /* proj_2160 cm_type removal
     * functions below (xxxxProtocolA5) are used only for A5 */
    static IDE_RC errorInfoProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC connectProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC disconnectProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC propertyGetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC propertySetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC prepareProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC planGetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC columnInfoGetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC columnInfoSetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramInfoGetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramInfoSetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramInfoSetListProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramDataInProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramDataInListProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC executeProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC fetchMoveProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC fetchProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC freeProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC transactionProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetSizeProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetBytePosCharLenProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetCharPosCharLenProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobBytePosProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobCharLengthProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobPutBeginProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobPutProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobPutEndProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobFreeProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobFreeAllProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC xaOperationProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *); 
    static IDE_RC xaTransactionProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);   
    static IDE_RC invalidProtocolA5(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* proj_2160 cm_type removal
     * functions below are used for A7 or higher */
    static IDE_RC connectProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC disconnectProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC propertyGetProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC propertySetProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC prepareProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC planGetProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC columnInfoGetProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramInfoGetProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramInfoSetListProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramDataInProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC paramDataInListProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC executeProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC fetchMoveProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC fetchProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC freeProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC cancelProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC transactionProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetSizeProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetSizeV2Protocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetBytePosCharLenProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobGetCharPosCharLenProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobBytePosProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobCharLengthProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobPutBeginProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobPutProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobPutEndProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobFreeProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC lobFreeAllProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    static IDE_RC lobTrimProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    static IDE_RC xaOperationProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *); 
    static IDE_RC xaTransactionProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    static IDE_RC invalidProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    static IDE_RC handshakeProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* PROJ-2177 User Interface - Cancel */
    static IDE_RC prepareByCIDProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);
    static IDE_RC cancelByCIDProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* BUG-38496 Notify users when their password expiry date is approaching */
    static IDE_RC connectExProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* PROJ-2598 Shard */
    static IDE_RC shardNodeGetListProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* PROJ-2622 Shard Retry Execution */
    static IDE_RC shardNodeUpdateListProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* PROJ-2598 Shard pilot(shard analyze) */
    static IDE_RC shardAnalyzeProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* PROJ-2658 altibase sharding */
    static IDE_RC shardHandshakeProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* BUG-45411 client-side global transaction */
    static IDE_RC shardTransactionProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    static IDE_RC shardPrepareProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    static IDE_RC shardEndPendingTxProtocol(cmiProtocolContext *, cmiProtocol *, void *, void *);

    /* BUG-46785 Shard statement partial rollback */
    static IDE_RC setSavepointProtocol( cmiProtocolContext *, cmiProtocol *, void *, void * );
    static IDE_RC rollbackToSavepointProtocol( cmiProtocolContext *, cmiProtocol *, void *, void * );
    static IDE_RC shardStmtPartialRollback( cmiProtocolContext *, cmiProtocol *, void *, void * );
    static IDE_RC shardNodeReport( cmiProtocolContext *, cmiProtocol *, void *, void * );

    static IDE_RC sendShardRebuildNoti( cmiProtocolContext *aProtocolContext );
};




inline void mmtServiceThread::lockForNewTaskList()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT(mNewTaskListMutex.lock(NULL /* idvSQL* */)  == IDE_SUCCESS);
}

inline void mmtServiceThread::unlockForNewTaskList()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT( mNewTaskListMutex.unlock() == IDE_SUCCESS);
}

inline idBool mmtServiceThread::isRun()
{
    return mRun;
}

inline mmtServiceThreadInfo *mmtServiceThread::getInfo()
{
    return &mInfo;
}

inline iduListNode *mmtServiceThread::getThreadListNode()
{
    return &mThreadListNode;
}

inline iduListNode *mmtServiceThread::getCheckThreadListNode()
{
    return &mCheckThreadListNode;
}

inline UInt mmtServiceThread::getServiceThreadID()
{
    return mInfo.mServiceThreadID;
}

inline void mmtServiceThread::setIpcID(UInt aIpcID)
{
     mInfo.mIpcID = aIpcID;
}

inline UInt mmtServiceThread::getIpcID()
{
    return mInfo.mIpcID;
}

inline mmcServiceThreadType mmtServiceThread::getServiceThreadType()
{
    return mInfo.mServiceThreadType;
}

inline mmtServiceThreadState mmtServiceThread::getState()
{
    return mInfo.mState;
}

inline UInt mmtServiceThread::getTaskCount()
{
    return mInfo.mTaskCount;
}

inline UInt mmtServiceThread::getReadyTaskCount()
{
    return mInfo.mReadyTaskCount;
}

inline void mmtServiceThread::setState(mmtServiceThreadState aState)
{
    mInfo.mState = aState;
}

inline void mmtServiceThread::addTaskCount(SInt aDelta)
{
    mInfo.mTaskCount += aDelta;
}

inline void mmtServiceThread::addReadyTaskCount(SInt aDelta)
{
    mInfo.mReadyTaskCount += aDelta;
}

inline void mmtServiceThread::setSessionID(mmcSessID aSessionID)
{
    mInfo.mSessionID = aSessionID;
}

inline void mmtServiceThread::setTask(mmcTask *aTask)
{
    mTask = aTask;
}

inline mmcStatement *mmtServiceThread::getStatement()
{
    return mStatement;
}

inline void mmtServiceThread::setStatement(mmcStatement *aStatement)
{
    mStatement    = aStatement;
    /* fix BUG-31002, Remove unnecessary code dependency from the inline function of mm moudle. */
    mInfo.mStmtID = (aStatement != NULL) ? aStatement->getStmtID() : 0;
}

//PROJ-1677 현재 서비스 쓰레드가 shared mode 또는 dedicated mode으로 running되고 있는지 여부를
//return 한다.
inline idBool mmtServiceThread::isRunModeShared()
{
    return (mMultiPlexingFunc == &mmtServiceThread::multiplexingAsShared) ? ID_TRUE:ID_FALSE;
}

// PROJ-1677 새로 생성한 target service thread에게 task list를
// 양도 한다.
inline void  mmtServiceThread::moveTaskLists(mmtServiceThread* aTargetServiceThr)
{
    iduList           sTaskList;

    /* BUG-31316 */
    iduListNode *sIterator;
    iduListNode *sNodeNext;
    UInt sTaskCount = 0;

    IDU_LIST_INIT(&sTaskList);

    removeAllTasks(&sTaskList, MMT_SERVICE_THREAD_NO_LOCK);

    IDU_LIST_ITERATE_SAFE(&sTaskList, sIterator, sNodeNext)
    {
        sTaskCount++;
    }

    if (sTaskCount > 0)
    {
        aTargetServiceThr->addTasks(&sTaskList, sTaskCount);
    }
}

//fix BUG-19464
inline void  mmtServiceThread::setServiceThreadID(UInt aServiceThrID)
{
    mInfo.mServiceThreadID = aServiceThrID;
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
*/

inline UInt  mmtServiceThread::getLifeSpan()
{
    return mInfo.mLifeSpan;
}

inline void  mmtServiceThread::incLifeSpan(UInt aLifeSpan)
{
    mInfo.mLifeSpan +=  aLifeSpan;
}
inline void  mmtServiceThread::decLifeSpan()
{
    if(mInfo.mLifeSpan  > 0)
    {
        
        mInfo.mLifeSpan--;
    }
    
}
/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   the definition of busy degree
   busy experice count + a count of current assigned tasks 
*/
inline ULong mmtServiceThread::getBusyDegree(idBool aIncludeToCurrentBusy)
{
    
    ULong                  sBusyDegree;
    
    sBusyDegree =  mInfo.mBusyExperienceCnt + getAssignedTasks();
    

    if(aIncludeToCurrentBusy == ID_TRUE)
    {
        // 현재 busy이면 SERVICE_THREAD_INITIAL_LIFESPAN개의 task가 있는것으로 간주한다.
        sBusyDegree += ((mLoopCounter ==  mLoopCheck ) ? mmuProperty::getBusyServiceThrPenalty() : 0 );
    }
    return sBusyDegree;
}


inline void  mmtServiceThread::decreaseCurAddedTaskCnt()
{
    if(mInfo.mCurAddedTasks > 0)
    {
        mInfo.mCurAddedTasks -= 1;
    }
    
}

inline UInt  mmtServiceThread::getAssignedTasks()
{
    return mInfo.mCurAddedTasks +mInfo.mTaskCount;
}

/* BUG-38384 A task in a service thread can be a orphan */
inline idBool mmtServiceThread::getRemoveAllTasks()
{
    return mInfo.mRemoveAllTasks;
}

inline void   mmtServiceThread::setRemoveAllTasks(idBool aFlag)
{
    mInfo.mRemoveAllTasks = aFlag;
}

inline idBool mmtServiceThread::getErrorFlag()
{
    return mErrorFlag;
}

inline void mmtServiceThread::setErrorFlag( idBool aErrorFlag )
{
    mErrorFlag = aErrorFlag;
}

void addIPCDATask(UInt aChannelID, mmcTask *aTask);
void executeIPCDATask(UInt aChannelID);
void terminateIPCDATask(mmcTask *aTask);

#endif
