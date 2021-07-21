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
#include <idp.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smiDef.h>

#ifndef _O_MMU_PROPERTY_H_
#define _O_MMU_PROPERTY_H_ 1

// bug-19279 remote sysdba enable + sys can kill session
#define REMOTE_SYSDBA_ENABLE_OFF (0)
#define REMOTE_SYSDBA_ENABLE_ON  (1)

struct mmuPropertyArgument;

class mmuProperty
{
public:
    static SChar *mDbName;
    static SChar *mLogDir;
    static SChar *mServerMsglogDir;
    static SChar *mUnixdomainFilepath;
    static SChar* mLogAnchorDir;
    static SChar *mDBDir[SM_DB_DIR_MAX_COUNT];
    static UInt   mDBDirCount;

    static UInt   mShmDbKey;
    static ULong  mLogFileSize;
    static ULong  mDefaultMemDbFileSize;
    static UInt   mPortNo;
    static UInt   mMaxListen;
    static UInt   mMaxClient;
    static UInt   mCmDetectTime;
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* This property indicates the ratio of initially allocated number of StatementPageTables
       to MAX_CLIENT in mmcStatementManager. */
    static UInt   mStmtpagetablePreallocRatio;
    /* Maxinum number of free-list elements in the session mutexpool */
    static UInt   mSessionMutexpoolFreelistMaxcnt;
    /* The number of initially created mutexes in the free-list of the session mutexpool */
    static UInt   mSessionMutexpoolFreelistInitcnt;
    static UInt   mMmtSessionListMempoolSize;
    static UInt   mMmcMutexpoolMempoolSize;
    static UInt   mMmcStmtpagetableMempoolSize;

    // BUG-19465 : CM_Buffer의 pending list를 제한
    static UInt   mCmMaxPendingList;
    static UInt   mAutoCommit;
    static UInt   mTxStartMode;
    static UInt   mQueueGlobalHashTableSize;
    static UInt   mQueueSessionHashTableSize;
    static UInt   mIsolationLevel;
    static UInt   mCommitWriteWaitMode;
    static UInt   mLoginTimeout;
    static UInt   mIdleTimeout;
    static UInt   mQueryTimeout;
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    static UInt   mDdlTimeout;
    static UInt   mFetchTimeout;
    static UInt   mUtransTimeout;
    
    /* BUG-28866 */
    static UInt   mMmSessionLogging;
    
    /* BUG-31144 */
    static UInt   mMaxStatementsPerSession;

    /* PROJ-2616 */
    static UInt   mIPCDAChannelCount;
    static UInt   mIPCDASleepTime;
    static SChar *mIPCDAFilepath;

    static UInt   mIpcChannelCount;
    static UInt   mAdminMode;
    static UInt   mMemoryCompactTime;
    static UInt   mShowErrorStack;
    static UInt   mTxTabSize;
    static UInt   mXaComplete;
    static UInt   mXaTimeout;

    static UInt   mSelectHeaderDisplay;

    static UInt   mMultiplexingThreadCount;
    static UInt   mMultiplexingMaxThreadCount;
    static UInt   mMultiplexingPollTimeout;
    static UInt   mMultiplexingCheckInterval;

    static ULong  mUpdateMaxLogSize;
    //PROJ-1436 SQL Plan Cache.
    static ULong  mSqlPlanCacheSize;
    static UInt   mSqlPlanCacheBucketCnt;
    static UInt   mSqlPlanCacheInitPCBCnt;
    static UInt   mSqlPlanCacheInitParentPCOCnt;
    static UInt   mSqlPlanCacheInitChildPCOCnt;
    static UInt   mSqlPlanCacheHotRegionLruRatio;
    /* BUG-35521 Add TryLatch in PlanCache. */
    static UInt   mSqlPlanCacheParentPCOXLatchTryCnt;
    /* BUG-36205 Plan Cache On/Off property for PSM */
    static UInt   mSqlPlanCacheUseInPSM;

    //fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
    //QueryTime Out이 아니라,Property를 제공해야 함.
    static UInt   mXaRollbackTimeOut;

    static UInt   mQueryLoggingLevel;
    // bug-19279 remote sysdba enable + sys can kill session
    static UInt   mRemoteSysdbaEnable;

    // BUG-24993 네트워크 에러 메시지 log 여부
    static UInt   mNetworkErrorLog;
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase  */
    static UInt   mServiceThrInitialLifeSpan;
    static UInt   mBusyServiceThrPenalty;
    static UInt   mMinMigrationTaskRate;
    static UInt   mNewServiceThrCreateRate;
    static UInt   mNewServiceThrCreateRateGap;
    static UInt   mSerivceThrExitRate;
    static UInt   mMinTaskCntForThrLive;

    // fix BUG-30466
    static UInt   mShutdownImmediateTimeout;

    // fix BUG-30731
    // V$STATEMENT, V$SQLTEXT, V$PLANTEXT 조회시
    // 입력된 값만큼만 한번에 검색
    static UInt   mStatementListPartialScanCount;
    //fix BUG-30949 A waiting time for enqueue event in transformed dedicated thread should not be infinite. 
    static ULong  mMaxEnqWaitTime;
    //fix BUG-31150, It needs to add  the property for frequency of  hot region LRU  list.
    static UInt   mFrequencyForHotLruRegion;
    static UInt   mXAHashSize; // bug-35371
    static UInt   mXidMemPoolElementCount; // bug-35381
    static UInt   mXidMutexPoolSize;       // bug-35382

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    static UInt   mIsDedicatedMode; 
    static UInt   mDedicatedThreadInitCount;
    static UInt   mDedicatedThreadMaxCount;
    static UInt   mThreadCheckInterval; 
    static UInt   mIsCPUAffinity; 

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    static UInt   mLobCacheThreshold;

    /* BUG-35332 The socket files can be moved */
    static SChar *mIpcFilepath;

    /* PROJ-1438 Job Scheduler */
    static UInt  mJobThreadCount;
    static UInt  mJobThreadQueueSize;
    static UInt  mJobSchedulerEnable;

    /* PROJ-2474 SSL/TLS */
    static UInt   mSslPortNo;
    static UInt   mSslMaxListen;
    static UInt   mSslEnable;

    /* BUG-41168 SSL extension*/
    static UInt   mTcpEnable;

    /* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
    static SChar* mIPACLFile;

    /* PROJ-2626 Snapshot Export */
    static UInt   mSnapshotMemThreshold;
    static UInt   mSnapshotDiskUndoThreshold;

    /* PROJ-2677 DDL synchronization */
    static UInt   mReplicationDDLSync;
    static UInt   mReplicationDDLSyncTimeout;

    /* PROJ-2681 */
    static UInt   mIBEnable;
    static UInt   mIBPortNo;
    static UInt   mIBMaxListen;
    static UInt   mIBListenerDisable;
    static UInt   mIBLatency;
    static UInt   mIBConChkSpin;

    static UInt   mSharedTransHashBucketCount;

    /* PROJ-2733 */
    static UInt  mIndoubtFetchTimeout;
    static UInt  mIndoubtFetchMethod;

public:
    static void initialize();
    static void destroy();

    static void load();

    //==================
    static SChar *getDbName()      { return mDbName; }
    static SChar *getLogDir()      { return mLogDir; }
    static SChar *getServerMsglogDir() { return mServerMsglogDir; }
    static SChar *getUnixdomainFilepath() { return mUnixdomainFilepath; }
    static SChar *getLogAnchorDir(){ return mLogAnchorDir; }
    static SChar *getDBDir(UInt sNthValue)
        {
            IDE_ASSERT( sNthValue < getDBDirCount() );
            return mDBDir[sNthValue];
        }

    static UInt   getDBDirCount()  { return mDBDirCount; }

    static UInt   getShmDbKey()    { return mShmDbKey; }
    static ULong  getLogFileSize() { return mLogFileSize; }
    static ULong  getDefaultMemDbFileSize()  { return mDefaultMemDbFileSize; }
    static UInt   getPortNo()      { return mPortNo; }
    static UInt   getMaxListen()   { return mMaxListen; }
    static UInt   getMaxClient()   { return mMaxClient; }
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    static UInt   getStmtpagetablePreallocRatio() { return mStmtpagetablePreallocRatio; }
    static UInt   getSessionMutexpoolFreelistMaxcnt() { return mSessionMutexpoolFreelistMaxcnt; }
    static UInt   getSessionMutexpoolFreelistInitcnt() { return mSessionMutexpoolFreelistInitcnt; }
    static UInt   getMmtSessionListMempoolSize() { return mMmtSessionListMempoolSize; }
    static UInt   getMmcMutexpoolMempoolSize() { return mMmcMutexpoolMempoolSize; }
    static UInt   getMmcStmtpagetableMempoolSize() { return mMmcStmtpagetableMempoolSize; }
    static UInt   getCmDetectTime(){ return mCmDetectTime; }

    // BUG-19465 : CM_Buffer의 pending list를 제한
    static UInt   getCmMaxPendingList(){ return mCmMaxPendingList; }
    static UInt   getAutoCommit()     { return mAutoCommit; }
    static UInt   getTxStartMode()    { return mTxStartMode; }
    static IDE_RC callbackAutoCommit(idvSQL * /*aStatistics*/,
                                     SChar *, void *, void *, void*);

    static UInt   getQueueGlobalHashTableSize() { return mQueueGlobalHashTableSize; }
    static UInt   getQueueSessionHashTableSize() { return mQueueSessionHashTableSize; }

    static UInt   getIsolationLevel() { return mIsolationLevel; }
    
    static UInt   getCommitWriteWaitMode() { return mCommitWriteWaitMode; }

    static UInt   getLoginTimeout()   { return mLoginTimeout; }
    static IDE_RC callbackLoginTimeout(idvSQL * /*aStatistics*/,
                                       SChar *, void *, void *, void *);

    static UInt   getIdleTimeout()    { return mIdleTimeout; }
    static IDE_RC callbackIdleTimeout(idvSQL * /*aStatistics*/,
                                      SChar *, void *, void *, void*);
    
    /* BUG-28866 */
    static UInt   getMmSessionLogging()    { return mMmSessionLogging; }
    static IDE_RC callbackMmSessionLogging(idvSQL * /*aStatistics*/,
                                           SChar *, void *, void *, void*);

    /* BUG-31144 */
    static UInt   getMaxStatementsPerSession()    { return mMaxStatementsPerSession; }
    static IDE_RC callbackMaxStatementsPerSession(idvSQL * /*aStatistics*/,
                                                  SChar *, void *, void *, void*);

    static UInt   getQueryTimeout()   { return mQueryTimeout; }
    static IDE_RC callbackQueryTimeout(idvSQL * /*aStatistics*/,
                                       SChar *, void *, void *, void*);

    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    static UInt   getDdlTimeout()   { return mDdlTimeout; }
    static IDE_RC callbackDdlTimeout(idvSQL * /*aStatistics*/,
                                     SChar *, void *, void *, void*);

    static UInt   getFetchTimeout()   { return mFetchTimeout; }
    static IDE_RC callbackFetchTimeout(idvSQL * /*aStatistics*/,
                                       SChar *, void *, void *, void*);

    static UInt   getUtransTimeout()  { return mUtransTimeout; }
    static IDE_RC callbackUransTimeout(idvSQL * /*aStatistics*/,
                                       SChar *, void *, void *, void*);

    static UInt   getIpcChannelCount()        { return mIpcChannelCount; }

    static UInt   getAdminMode()              { return mAdminMode; }
    static IDE_RC callbackAdminMode(idvSQL * /*aStatistics*/,
                                    SChar *, void *, void *, void *);

    static UInt   getMemoryCompactTime()      { return mMemoryCompactTime; }
    static UInt   getShowErrorStack()         { return mShowErrorStack; }
    static IDE_RC callbackShowErrorStack(idvSQL * /*aStatistics*/,
                                         SChar *, void *, void *, void *);

    static UInt   getTransactionTableSize()   { return mTxTabSize;      }
    static UInt   getXaComplete() { return mXaComplete; }
    static UInt   getXaTimeout()  { return mXaTimeout;  }

    static UInt   getSelectHeaderDisplay() { return mSelectHeaderDisplay;  }
    static IDE_RC callbackSelectHeaderDisplay(idvSQL * /*aStatistics*/,
                                              SChar *, void *, void *, void *);

    static UInt   getMultiplexingThreadCount()      { return mMultiplexingThreadCount;    }
    static UInt   getMultiplexingMaxThreadCount()   { return mMultiplexingMaxThreadCount; }
    static UInt   getMultiplexingPollTimeout()      { return mMultiplexingPollTimeout;    }
    static UInt   getMultiplexingCheckInterval()    { return mMultiplexingCheckInterval;  }

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    static UInt   getIsDedicatedMode()              { return mIsDedicatedMode;         }
    static UInt   getDedicatedThreadInitCount()     { return mDedicatedThreadInitCount;}
    static UInt   getDedicatedThreadMaxCount()      { return mDedicatedThreadMaxCount; }
    static UInt   getThreadCheckInterval()          { return mThreadCheckInterval;     } 
    static UInt   getIsCPUAffinity()                { return mIsCPUAffinity;           } 

    static ULong   getUpdateMaxLogSize() { return mUpdateMaxLogSize; }
    //PROJ-1436 SQL Plan Cache.
    static ULong  getSqlPlanCacheSize() { return mSqlPlanCacheSize; }
    static UInt   getSqlPlanCacheBucketCnt() {return mSqlPlanCacheBucketCnt;}
    static UInt   getSqlPlanCacheInitPCBCnt() {return mSqlPlanCacheInitPCBCnt;}
    static UInt   getSqlPlanCacheInitParentPCOCnt() {return mSqlPlanCacheInitParentPCOCnt;}
    static UInt   getSqlPlanCacheInitChildPCOCnt() {return mSqlPlanCacheInitChildPCOCnt;}
    static UInt   getSqlPlanCacheHotRegionRatio()  {return mSqlPlanCacheHotRegionLruRatio;}
    /* BUG-35521 Add TryLatch in PlanCache. */
    static UInt   getSqlPlanCacheParentPCOXLatchTryCnt()
    {
        return mSqlPlanCacheParentPCOXLatchTryCnt;
    }

    /* BUG-36205 Plan Cache On/Off property for PSM */
    static UInt   getSqlPlanCacheUseInPSM() { return mSqlPlanCacheUseInPSM; }

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase  */
    static UInt   getServiceThrInitialLifeSpan()    {return mServiceThrInitialLifeSpan;}
    static UInt   getBusyServiceThrPenalty()        {return mBusyServiceThrPenalty;}
    static UInt   getMinMigrationTaskRate()         {return mMinMigrationTaskRate;}
    static UInt   getNewServiceThrCreateRate()      {return mNewServiceThrCreateRate;}
    static UInt   getNewServiceThrCreateRateGap()   {return mNewServiceThrCreateRateGap;}
    static UInt   getSerivceThrExitRate()           {return mSerivceThrExitRate;}
    static UInt   getMinTaskCntForThrLive()         {return mMinTaskCntForThrLive; }
   
    /* PROJ-2474 SSL/TLS */
    static UInt   getSslPortNo()      { return mSslPortNo; }
    static UInt   getSslMaxListen()   { return mSslMaxListen; }
    static UInt   getSslEnable()      { return mSslEnable; }
    
    /* BUG-41168 SSL extension */
    static UInt   getTcpEnable()      { return mTcpEnable; }
            
    //fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
    //QueryTime Out이 아니라,Property를 제공해야 함.
    static UInt   getXaRollbackTimeOut()       {  return mXaRollbackTimeOut; }
            
    // bug-19279 remote sysdba enable + sys can kill session
    static UInt getRemoteSysdbaEnable() { return mRemoteSysdbaEnable; }

    //fix BUG-30949 A waiting time for enqueue event in transformed dedicated thread should not be infinite.
    static ULong  getMaxEnqWaitTime()  { return mMaxEnqWaitTime; }
    static IDE_RC callbackMaxEnqWaitTime(idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/);
    //fix BUG-31150, It needs to add  the property for frequency of  hot region LRU  list.
    static UInt  getFrequencyForHotLruRegion()  {  return mFrequencyForHotLruRegion;  }

    /* PROJ-1438 Job Scheduler */
    static UInt  getJobThreadCount()        { return mJobThreadCount; }
    static UInt  getJobThreadQueueSize()    { return mJobThreadQueueSize; }
    static UInt  getJobSchedulerEnable()    { return mJobSchedulerEnable; }

    /* PROJ-2626 Snapshot Export */
    static UInt  getSnapshotMemThreshold() { return mSnapshotMemThreshold; }
    static UInt  getSnapshotDiskUndoThreshold() { return mSnapshotDiskUndoThreshold; }

    /* PROJ-2681 */
    static UInt   getIBEnable()                { return mIBEnable; }
    static UInt   getIBPortNo()                { return mIBPortNo; }
    static UInt   getIBMaxListen()             { return mIBMaxListen; }
    static UInt   getIBListenerDisable()       { return mIBListenerDisable; }
    static UInt   getIBLatency()               { return mIBLatency; }
    static UInt   getIBConChkSpin()            { return mIBConChkSpin; }

    static IDE_RC callbackFrequencyForHotLruRegion(idvSQL * /*aStatistics*/,
                                                   SChar * /*aName*/,
                                                   void  * /*aOldValue*/,
                                                   void  *aNewValue,
                                                   void  * /*aArg*/);

    static IDE_RC callbackNetworkErrorLog(idvSQL * /*aStatistics*/,
                                          SChar *, void *, void *, void*);
    
    static IDE_RC callbackMultiplexingMaxThreadCount(idvSQL * /*aStatistics*/,
                                                     SChar *, void *, void *, void *);
    static IDE_RC callbackMultiplexingPollTimeout(idvSQL * /*aStatistics*/,
                                                  SChar *, void *, void *, void *);
    static IDE_RC callbackMultiplexingCheckInterval(idvSQL * /*aStatistics*/,
                                                    SChar *, void *, void *, void *);
    static IDE_RC callbackSqlPlanCacheSize( idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);
    static IDE_RC callbackSqlPlanCacheHotRegionLruRatio(idvSQL * /*aStatistics*/,
                                                        SChar * /*aName*/,
                                                        void  * /*aOldValue*/,
                                                        void  *aNewValue,
                                                        void  * /*aArg*/);

    /* BUG-35521 Add TryLatch in PlanCache. */
    static IDE_RC callbackSqlPlanCacheParentPCOXLatchTryCnt(idvSQL * /*aStatistics*/,
                                                            SChar * /*aName*/,
                                                            void  * /*aOldValue*/,
                                                            void  *aNewValue,
                                                            void  * /*aArg*/);

    /* BUG-36205 Plan Cache On/Off property for PSM */
    static IDE_RC callbackSqlPlanCacheUseInPSM(idvSQL * /*aStatistics*/,
                                               SChar * /*aName*/,
                                               void  * /*aOldValue*/,
                                               void  *aNewValue,
                                               void  * /*aArg*/);
                                                

    //fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
    //QueryTime Out이 아니라,Property를 제공해야 함.
    static IDE_RC callbackXaRollbackTimeOut(idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);
    
    // To fix BUG-21134
    static UInt   getQueryLoggingLevel() { return mQueryLoggingLevel; }
    static IDE_RC callbackQueryLoggingLevel(idvSQL * /*aStatistics*/,
                                            SChar *, void *, void *, void *);

    // BUG-24993 네트워크 에러 메시지 log 여부
    static UInt   getNetworkErrorLog() { return mNetworkErrorLog; }
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase  */
    static IDE_RC callbackServiceThrInitialLifeSpan(idvSQL * /*aStatistics*/,
                                                    SChar * /*aName*/,
                                                    void  * /*aOldValue*/,
                                                    void  *aNewValue,
                                                    void  * /*aArg*/);

    static IDE_RC callbackBusyServiceThrPenalty(idvSQL * /*aStatistics*/,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  *aNewValue,
                                                void  * /*aArg*/);
   
    static IDE_RC callbackMinMigrationTaskRate(idvSQL * /*aStatistics*/,
                                               SChar * /*aName*/,
                                               void  * /*aOldValue*/,
                                               void  *aNewValue,
                                               void  * /*aArg*/);
    
    static IDE_RC callbackNewServiceThrCreateRate(idvSQL * /*aStatistics*/,
                                                  SChar * /*aName*/,
                                                  void  * /*aOldValue*/,
                                                  void  *aNewValue,
                                                  void  * /*aArg*/);

    static IDE_RC callbackNewServiceThrCreateRateGap(idvSQL * /*aStatistics*/,
                                                     SChar * /*aName*/,
                                                     void  * /*aOldValue*/,
                                                     void  *aNewValue,
                                                     void  * /*aArg*/);
    
    static IDE_RC callbackSerivceThrExitRate(idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /*aArg*/);
    
    static IDE_RC callbackMinTaskCntForThrLive(idvSQL * /*aStatistics*/,
                                               SChar * /*aName*/,
                                               void  * /*aOldValue*/,
                                               void  *aNewValue,
                                               void  * /*aArg*/);

    // fix BUG-30566
    static UInt   getShutdownImmediateTimeout() { return mShutdownImmediateTimeout;  }
    static IDE_RC callbackShutdownImmediateTimeout(idvSQL * /*aStatistics*/,
                                                   SChar * /*aName*/,
                                                   void  * /*aOldValue*/,
                                                   void  *aNewValue,
                                                   void  * /*aArg*/);

    // fix BUG-30731
    static IDE_RC callbackStatementPartialScanCount(idvSQL * /*aStatistics*/,
                                                    SChar * /*aName*/,
                                                    void  * /*aOldValue*/,
                                                    void  *aNewValue,
                                                    void  * /*aArg*/);

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    static UInt getLobCacheThreshold()
        {
            return mLobCacheThreshold;
        }
    static IDE_RC callbackLobCacheThreshold(idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);

    /* BUG-35332 The socket files can be moved */
    static SChar *getIpcFilepath() { return mIpcFilepath; }

    /* PROJ-2616 */
    static SChar *getIPCDAFilepath()        { return mIPCDAFilepath; }
    static UInt   getIPCDAChannelCount()    { return mIPCDAChannelCount; }
    static ULong  getIPCDASleepTime()       { return mIPCDASleepTime;}

    /* PROJ-2677 */
    static UInt   getReplicationDDLSync()   { return mReplicationDDLSync; }
    static IDE_RC callbackReplicationDDLSync( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ );

    static UInt   getReplicationDDLSyncTimeout()   { return mReplicationDDLSyncTimeout; }
    static IDE_RC callbackReplicationDDLSyncTimeout( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ );

    static UInt   getSharedTransHashBucketCount() { return mSharedTransHashBucketCount;  }

private:
    static IDE_RC callbackUpdateMaxLogSize( idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);


public:
    static void*  callbackForGettingArgument(mmuPropertyArgument *aArg,
                                            idpArgumentID        aID);

    static IDE_RC callbackCommitWriteWaitMode(idvSQL * /*aStatistics*/,
                                              SChar * /*aName*/,
                                              void  * /*aOldValue*/,
                                              void  *aNewValue,
                                              void  * /*aArg*/);

    // bug-19279 remote sysdba enable + sys can kill session
    static IDE_RC callbackRemoteSysdbaEnable( idvSQL * /*aStatistics*/,
                                              SChar * /*aName*/,
                                              void  * /*aOldValue*/,
                                              void  *aNewValue,
                                              void  * /*aArg*/);

    // bug-35371
    static UInt   getXAHashSize() { return mXAHashSize;  }
    static IDE_RC callbackXAHashSize(idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);

    static UInt   getXidMemPoolElementCount() { return mXidMemPoolElementCount;  }
    static IDE_RC callbackXidMemPoolElementCount(idvSQL * /*aStatistics*/,
                                                 SChar * /*aName*/,
                                                 void  * /*aOldValue*/,
                                                 void  *aNewValue,
                                                 void  * /*aArg*/);

    static UInt   getXidMutexPoolSize() { return mXidMutexPoolSize;  }
    static IDE_RC callbackXidMutexPoolSize(idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);

    static IDE_RC callbackJobSchedulerEnable(idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /*aArg*/);

    /* Project 2379 - Set max thread count callback */
    static IDE_RC callbackBeforeSetMaxThread(idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /*aArg*/);
    static IDE_RC callbackAfterSetMaxThread(idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  *aOldValue,
                                            void  *aNewValue,
                                            void  * /*aArg*/);

    /*PROJ-2616*/
    static IDE_RC callbackIPCDASleepTime(idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/);

    /* PROJ-2626 Snapshot Export */
    static IDE_RC callbackSnapshotMemThreshold(idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/);
    /* PROJ-2626 Snapshot Export */
    static IDE_RC callbackSnapshotDiskUndoThreshold(idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/);
    /* PROJ-2733 분산 트랜잭션 정합성 */
    static UInt getIndoubtFetchTimeout() { return mIndoubtFetchTimeout; }
    static IDE_RC callbackIndoubtFetchTimeout( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ );
    static UInt getIndoubtFetchMethod() { return mIndoubtFetchMethod; }
    static IDE_RC callbackIndoubtFetchMethod( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ );
 
};

/* ------------------------------------------------
 *  Argument Passing for Property : BUG-12719
 *  How to Added..
 *
 *  1. add  ArgumentID  to  idpBase.h => enum idpArgumentID
 *  2. add  Argument Object ot mmuProperty.h => struct mmuPropertyArgument
 *  3. add  switch/case in mmuProperty.cpp => callbackForGetingArgument()
 *
 *  Example property : check this => mmuProperty::callbackAdminMode()
 * ----------------------------------------------*/

typedef struct mmuPropertyArgument
{
    idpArgument mArg;

    /* define here argument value */

    UInt        mUserID;
    smiTrans   *mTrans;


}mmuPropertyArgument;


#endif
