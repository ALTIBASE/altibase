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

#ifndef _O_MMT_SESSION_MANAGER_H_
#define _O_MMT_SESSION_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <cm.h>
#include <mmcDef.h>
#include <iduLatch.h>
#include <qci.h>

class mmcTask;
class mmcSession;
class mmcStatement;
class mmtLoadBalancer;
struct mmcTransObj;

typedef struct mmtTimeoutInfo
{
    void  (*mLogFunc)(mmcTask      *aTask,
                      mmcStatement *aStatement,
                      UInt          aTimeGap,
                      UInt          aTimeout);

    idBool  mShouldClose;
    UInt   *mCount;
    UInt    mStatIndex;
} mmtTimeoutInfo;


typedef struct mmtSessionManagerInfo
{
    UInt  mTaskCount;
    UInt  mSessionCount;

    UInt  mBaseTime;
    UInt  mLoginTimeoutCount;
    UInt  mIdleTimeoutCount;
    UInt  mQueryTimeoutCount;
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    UInt  mDdlTimeoutCount;
    UInt  mFetchTimeoutCount;
    UInt  mUTransTimeoutCount;
    UInt  mTerminatedCount;
    ULong mUpdateMaxLogSize;
    UInt  mDdlSyncTimeoutCount;
} mmtSessionManagerInfo;


class mmtSessionManager
{
public:
    static PDL_Time_Value         mCurrentTime;

    static mmtSessionManagerInfo  mInfo;
private:
    static idBool                 mRun;
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       BUG-29171 Load Balancing module must be separated from the session manager thread.
    */

    static mmtLoadBalancer        *mLoadBalancer;
    static mmcSessID              mNextSessionID;

    static iduMemPool             mTaskPool;
    static iduMemPool             mSessionPool;

    static iduList                mTaskList;
    /* PROJ-2451 Concurrent Execute Package */
    static iduList                mInternalSessionList;

    /*
     * BUG-42434 session manager lock should be modified 
     *           in order to reduce lock contention.
     */
    static iduLatch               mLatch;
    static iduMutex               mMutexSessionIDList;

    static mmtTimeoutInfo         mTimeoutInfo[];

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* A List which contains sessionIDs of finalized sessions. */
    static iduList                mFreeSessionIDList;
    static iduMemPool             mListNodePool;
    static iduMemPool             mSessionIDPool;


public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC allocTask(mmcTask **aTask);
    static IDE_RC freeTask(mmcTask *aTask);

    /* BUG-48515 */
    static iduList* getTaskList();

    static IDE_RC allocInternalTask( mmcTask ** aTask,
                                     cmiLink  * aLink );

    static IDE_RC freeInternalTask(mmcTask *aTask);
    
    static IDE_RC allocSession(mmcTask *aTask,
                               idBool aIsSysdba);
    static IDE_RC freeSession(mmcTask *aTask);

    static IDE_RC shutdown();
    static IDE_RC terminate( mmcSession *aWorkerSession,
                             mmcSessID   aTargetSessID,
                             SChar      *aUserName,
                             UInt        aUserNameLen,
                             idBool      aCloseAll,
                             SInt       *aResultCount );

    static void   run();
    /* BUG-48515 */
    static idBool isRun();
    static void   stop();

    static void   logSessionOverview(ideLogEntry &aLog);
    static idBool logTaskOfThread(ideLogEntry &aLog, ULong aThreadID);

    /* fix BUG-25020, BUG-25323 */
    static void   setQueryTimeoutEvent(mmcSession *aSession, mmcStmtID aStmtID);

    /* PROJ-2177 User Interface - Cancel */
    static IDE_RC setCancelEvent(mmcStmtID aStmtID);
    static IDE_RC findSession(mmcSession **aSession, mmcSessID aSessionID);

    static idBool existSessionByXID( ID_XID *aXID );

    /* PROJ-2451 Concurrent Execute Package */
    static IDE_RC freeInternalSession( mmcSession * aSession );
    static IDE_RC allocInternalSession( mmcSession  ** aSession,
                                        qciUserInfo  * aUserInfo );

    /* PROJ-2379 Is sysdba? */
    static idBool isSysdba(void);

    // BUG-42464 dbms_alert package
    static IDE_RC applySignal( mmcSession * aSession );

    /* PROJ-2626 Snapshot Export */
    static idBool existClientAppInfoType( mmcClientAppInfoType );
    static void  terminateAllClientAppInoType( mmcClientAppInfoType );

public:
    static UInt            getTaskCount();
    static UInt            getSessionCount();

    static UInt            getBaseTime();
    static PDL_Time_Value *getCurrentTime();

    static void lock();     /* r/w */
    static void lockRead(); /* r   */
    static void unlock();

private:
    static void changeTaskCount(SInt aValue);
    static void changeSessionCount(SInt aValue);

private:
    static void   checkAllTask();

    static void   checkSession(mmcTask *aTask);

    static void   checkSessionTimeout(mmcTask *aTask);

    static idBool checkTimeoutEvent(mmcTask      *aTask,
                                    mmcStatement *aStatement,
                                    UInt          sStartTime,
                                    UInt          aTimeout,
                                    UInt          aTimeoutIndex);
    //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
    // 동시성에 문제가 있음.
    static void wakeupEnqueWaitIfNessary(mmcTask*  aTask,
                                         mmcSession * aSession);

public:
    /*
     * Fixed Table
     */

    static IDE_RC buildRecordForSESSION(idvSQL              * /*aStatistics*/,
                                        void *aHeader,
                                        void *aDumpObj,
                                        iduFixedTableMemory *aMemory);
    
    /* PROJ-2451 Concurrent Execute Package */
    static IDE_RC buildRecordForINTERNALSESSION(idvSQL * /*aStatistics*/,
                                                void *aHeader,
                                                void *aDumpObj,
                                                iduFixedTableMemory *aMemory);
    
    static IDE_RC buildRecordForSESSIONMGR(idvSQL              * /*aStatistics*/,
                                           void *aHeader,
                                           void *aDumpObj,
                                           iduFixedTableMemory *aMemory);
    
    static IDE_RC buildRecordForSESSTAT(idvSQL              * /*aStatistics*/,
                                        void *aHeader,
                                        void *aDumpObj,
                                        iduFixedTableMemory *aMemory);
    
    static IDE_RC buildRecordForSYSSTAT(idvSQL              * /*aStatistics*/,
                                        void *aHeader,
                                        void *aDumpObj,
                                        iduFixedTableMemory *aMemory);
    
    static IDE_RC buildRecordForSessionEvent(
        idvSQL              * /*aStatistics*/,
        void *aHeader,
        void *aDumpObj,
        iduFixedTableMemory *aMemory);
    
    static IDE_RC buildRecordForSystemEvent(
        idvSQL              * /*aStatistics*/,
        void *aHeader,
        void *aDumpObj,
        iduFixedTableMemory *aMemory);

    static IDE_RC buildRecordForSysConflictPage(
        idvSQL              * /*aStatistics*/,
        void *aHeader,
        void * /* aDumpObj */,
        iduFixedTableMemory *aMemory);
};


inline UInt mmtSessionManager::getTaskCount()
{
    return mInfo.mTaskCount;
}

inline UInt mmtSessionManager::getSessionCount()
{
    return mInfo.mSessionCount;
}

inline UInt mmtSessionManager::getBaseTime()
{
    return mInfo.mBaseTime;
}


inline PDL_Time_Value *mmtSessionManager::getCurrentTime()
{
    return &mCurrentTime;
}

inline void mmtSessionManager::lock()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT( mLatch.lockWrite( NULL,/* idvSQL* */
                                  NULL ) /* sWeArgs*/
               == IDE_SUCCESS);
}

inline void mmtSessionManager::lockRead()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT( mLatch.lockRead( NULL,/* idvSQL* */
                                 NULL ) /* sWeArgs*/
               == IDE_SUCCESS);
}

inline void mmtSessionManager::unlock()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT(mLatch.unlock()  == IDE_SUCCESS);  
}

inline void mmtSessionManager::changeTaskCount(SInt aValue)
{
    mInfo.mTaskCount += aValue;
}

inline void mmtSessionManager::changeSessionCount(SInt aValue)
{
    mInfo.mSessionCount += aValue;
}


#endif
