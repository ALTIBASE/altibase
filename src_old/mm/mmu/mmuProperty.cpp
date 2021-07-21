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
#include <qci.h>
#include <rpi.h>
#include <mmErrorCode.h>
#include <mmuProperty.h>
#include <mmuAccessList.h>

SChar *mmuProperty::mDbName;
SChar *mmuProperty::mLogDir;
SChar *mmuProperty::mServerMsglogDir;
SChar *mmuProperty::mUnixdomainFilepath;
SChar *mmuProperty::mLogAnchorDir;
SChar *mmuProperty::mDBDir[SM_DB_DIR_MAX_COUNT];
UInt   mmuProperty::mDBDirCount;

UInt   mmuProperty::mShmDbKey;
ULong  mmuProperty::mLogFileSize;
ULong  mmuProperty::mDefaultMemDbFileSize;
UInt   mmuProperty::mPortNo;
UInt   mmuProperty::mMaxListen;
UInt   mmuProperty::mMaxClient;
UInt   mmuProperty::mCmDetectTime;
UInt   mmuProperty::mCmMaxPendingList;
UInt   mmuProperty::mAutoCommit;
UInt   mmuProperty::mTxStartMode;
UInt   mmuProperty::mQueueGlobalHashTableSize;
UInt   mmuProperty::mQueueSessionHashTableSize;
UInt   mmuProperty::mIsolationLevel;
UInt   mmuProperty::mCommitWriteWaitMode;
UInt   mmuProperty::mLoginTimeout;
UInt   mmuProperty::mIdleTimeout;
UInt   mmuProperty::mQueryTimeout;
/* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
UInt   mmuProperty::mDdlTimeout;
UInt   mmuProperty::mFetchTimeout;
UInt   mmuProperty::mUtransTimeout;
UInt   mmuProperty::mIpcChannelCount;

/* PROJ-2616 */
UInt   mmuProperty::mIPCDAChannelCount;
UInt   mmuProperty::mIPCDASleepTime;

UInt   mmuProperty::mAdminMode;
UInt   mmuProperty::mMemoryCompactTime;
UInt   mmuProperty::mShowErrorStack;
UInt   mmuProperty::mTxTabSize;
UInt   mmuProperty::mXaComplete;
UInt   mmuProperty::mXaTimeout;
UInt   mmuProperty::mSelectHeaderDisplay;
/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
UInt   mmuProperty::mStmtpagetablePreallocRatio;
/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
UInt   mmuProperty::mSessionMutexpoolFreelistMaxcnt;
UInt   mmuProperty::mSessionMutexpoolFreelistInitcnt;
UInt   mmuProperty::mMmtSessionListMempoolSize;
UInt   mmuProperty::mMmcMutexpoolMempoolSize;
UInt   mmuProperty::mMmcStmtpagetableMempoolSize;

/* 
 * BUG-28866 : MM logging
 */
UInt   mmuProperty::mMmSessionLogging;

/* 
 * BUG-31144 : Max statements per a session;
 */
UInt   mmuProperty::mMaxStatementsPerSession;

UInt   mmuProperty::mMultiplexingThreadCount;
UInt   mmuProperty::mMultiplexingMaxThreadCount;
UInt   mmuProperty::mMultiplexingPollTimeout;
UInt   mmuProperty::mMultiplexingCheckInterval;

ULong  mmuProperty::mUpdateMaxLogSize;
//PROJ-1436 SQL Plan Cache.
ULong  mmuProperty::mSqlPlanCacheSize;
UInt   mmuProperty::mSqlPlanCacheBucketCnt;
UInt   mmuProperty::mSqlPlanCacheInitPCBCnt;
UInt   mmuProperty::mSqlPlanCacheInitParentPCOCnt;
UInt   mmuProperty::mSqlPlanCacheInitChildPCOCnt;
UInt   mmuProperty::mSqlPlanCacheHotRegionLruRatio;
/* BUG-35521 Add TryLatch in PlanCache. */
UInt   mmuProperty::mSqlPlanCacheParentPCOXLatchTryCnt;
/* BUG-36205 Plan Cache On/Off property for PSM */
UInt   mmuProperty::mSqlPlanCacheUseInPSM;

//fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
//QueryTime Out이 아니라,Property를 제공해야 함.
UInt   mmuProperty::mXaRollbackTimeOut;



UInt   mmuProperty::mQueryLoggingLevel;
// bug-19279 remote sysdba enable + sys can kill session
UInt   mmuProperty::mRemoteSysdbaEnable;
// BUG-24993 네트워크 에러 메시지 log 여부
UInt   mmuProperty::mNetworkErrorLog;

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase  */
UInt   mmuProperty::mServiceThrInitialLifeSpan;
UInt   mmuProperty::mBusyServiceThrPenalty;
UInt   mmuProperty::mMinMigrationTaskRate;
UInt   mmuProperty::mNewServiceThrCreateRate;
UInt   mmuProperty::mNewServiceThrCreateRateGap;
UInt   mmuProperty::mSerivceThrExitRate;
UInt   mmuProperty::mMinTaskCntForThrLive;

// fix BUG-30566
UInt   mmuProperty::mShutdownImmediateTimeout;

// fix BUG-30731
UInt   mmuProperty::mStatementListPartialScanCount;

//fix BUG-30949 A waiting time for enqueue event in transformed dedicated thread should not be infinite.
ULong  mmuProperty::mMaxEnqWaitTime;
//fix BUG-31150, It needs to add  the property for frequency of  hot region LRU  list.
UInt   mmuProperty::mFrequencyForHotLruRegion;

/* PROJ-2108 Dedicated thread mode which uses less CPU */
UInt   mmuProperty::mIsDedicatedMode;
UInt   mmuProperty::mDedicatedThreadInitCount;
UInt   mmuProperty::mDedicatedThreadMaxCount;
UInt   mmuProperty::mThreadCheckInterval;
UInt   mmuProperty::mIsCPUAffinity;

/* PROJ-2047 Strengthening LOB - LOBCACHE */
UInt   mmuProperty::mLobCacheThreshold;

/* BUG-35332 The socket files can be moved */
SChar *mmuProperty::mIpcFilepath;
SChar *mmuProperty::mIPCDAFilepath;
UInt   mmuProperty::mXAHashSize; // bug-35371
UInt   mmuProperty::mXidMemPoolElementCount; // bug-35381
UInt   mmuProperty::mXidMutexPoolSize;       // bug-35382

/* PROJ-1438 Job Scheduler */
UInt   mmuProperty::mJobThreadCount;
UInt   mmuProperty::mJobThreadQueueSize;
UInt   mmuProperty::mJobSchedulerEnable;

/* PROJ-2474 SSL/TLS */
UInt   mmuProperty::mSslPortNo;
UInt   mmuProperty::mSslEnable;
UInt   mmuProperty::mSslMaxListen;

/* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
SChar *mmuProperty::mIPACLFile;

/* BUG-41168 SSL extension */
UInt   mmuProperty::mTcpEnable;

/* PROJ-2626 Snapshot Export */
UInt   mmuProperty::mSnapshotMemThreshold;
UInt   mmuProperty::mSnapshotDiskUndoThreshold;

/* PROJ-2677 DDL synchronization */
UInt   mmuProperty::mReplicationDDLSync;
UInt   mmuProperty::mReplicationDDLSyncTimeout;

/* PROJ-2681 */
UInt   mmuProperty::mIBEnable;
UInt   mmuProperty::mIBPortNo;
UInt   mmuProperty::mIBMaxListen;
UInt   mmuProperty::mIBListenerDisable;
UInt   mmuProperty::mIBLatency;
UInt   mmuProperty::mIBConChkSpin;

UInt   mmuProperty::mSharedTransHashBucketCount;

/* PROJ-2733 */
UInt   mmuProperty::mIndoubtFetchTimeout;
UInt   mmuProperty::mIndoubtFetchMethod;

IDE_RC mmuProperty::callbackLoginTimeout(idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/)
{
    mLoginTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackIdleTimeout(idvSQL * /*aStatistics*/,
                                        SChar * /*aName*/,
                                        void  * /*aOldValue*/,
                                        void  *aNewValue,
                                        void  * /*aArg*/)
{
    mIdleTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackQueryTimeout(idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/)
{
    mQueryTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
IDE_RC mmuProperty::callbackDdlTimeout(idvSQL * /*aStatistics*/,
                                       SChar * /*aName*/,
                                       void  * /*aOldValue*/,
                                       void  *aNewValue,
                                       void  * /*aArg*/)
{
    mDdlTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-28866 */
IDE_RC mmuProperty::callbackMmSessionLogging(idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /*aArg*/)
{
    mMmSessionLogging = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-31144 */
IDE_RC mmuProperty::callbackMaxStatementsPerSession(idvSQL * /*aStatistics*/,
                                                    SChar * /*aName*/,
                                                    void  * /*aOldValue*/,
                                                    void  *aNewValue,
                                                    void  * /*aArg*/)
{
    mMaxStatementsPerSession = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackFetchTimeout(idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/)
{
    mFetchTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackUransTimeout(idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/)
{
    mUtransTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackAutoCommit(idvSQL * /*aStatistics*/,
                                       SChar * /*aName*/,
                                       void  * /*aOldValue*/,
                                       void  *aNewValue,
                                       void  * /*aArg*/)
{
    mAutoCommit = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackShowErrorStack(idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/)
{
    mShowErrorStack = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackSelectHeaderDisplay(idvSQL * /*aStatistics*/,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  *aNewValue,
                                                void  * /*aArg*/)
{
    mSelectHeaderDisplay = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackMultiplexingMaxThreadCount(idvSQL * /*aStatistics*/,
                                                       SChar * /*aName*/,
                                                       void  * /*aOldValue*/,
                                                       void  *aNewValue,
                                                       void  * /*aArg*/)
{
    mMultiplexingMaxThreadCount = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackMultiplexingPollTimeout(idvSQL * /*aStatistics*/,
                                                    SChar * /*aName*/,
                                                    void  * /*aOldValue*/,
                                                    void  *aNewValue,
                                                    void  * /*aArg*/)
{
    mMultiplexingPollTimeout = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackMultiplexingCheckInterval(idvSQL * /*aStatistics*/,
                                                      SChar * /*aName*/,
                                                      void  * /*aOldValue*/,
                                                      void  *aNewValue,
                                                      void  * /*aArg*/)
{
    mMultiplexingCheckInterval = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

// BUG-26280 쓰기가 가능하지만 콜백이 등록안되어 있는 프로퍼티
IDE_RC mmuProperty::callbackNetworkErrorLog(idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/)
{
    mNetworkErrorLog = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackAdminMode(idvSQL * aStatistics,
                                      SChar * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  *aArg)
{
    UInt sUserID;
    idpArgument *sArgObj = (idpArgument *)aArg;

    sUserID = *(UInt *)(sArgObj->getArgValue(aStatistics, sArgObj, IDP_ARG_USERID));

    IDE_TEST_RAISE( (sUserID != QC_SYS_USER_ID) &&
                    (sUserID != QC_SYSTEM_USER_ID),  user_error);

    mAdminMode = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION(user_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ADMIN_MODE_PRIV));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmuProperty::callbackUpdateMaxLogSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mUpdateMaxLogSize = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackSqlPlanCacheSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mSqlPlanCacheSize = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackSqlPlanCacheHotRegionLruRatio(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mSqlPlanCacheHotRegionLruRatio = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-35521 Add TryLatch in PlanCache. */
IDE_RC mmuProperty::callbackSqlPlanCacheParentPCOXLatchTryCnt(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mSqlPlanCacheParentPCOXLatchTryCnt = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

//fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
//QueryTime Out이 아니라,Property를 제공해야 함.
IDE_RC mmuProperty::callbackXaRollbackTimeOut(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mXaRollbackTimeOut  = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}


/* BUG-18660: COMMIT_WRITE_WAIT_MODE을 Alter System으로 변경해도 MM property에
 *            반영되지 않습니다. Property변경시 mmuProperty에 있는 값을 변경한는
 *            callback function이 등록되지 않았습니다. */
IDE_RC mmuProperty::callbackCommitWriteWaitMode(idvSQL * /*aStatistics*/,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  * aNewValue,
                                                void  * /*aArg*/)
{
    mCommitWriteWaitMode = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// To fix BUG-21134
// query logging level setting
IDE_RC mmuProperty::callbackQueryLoggingLevel(idvSQL * /*aStatistics*/,
                                              SChar * /*aName*/,
                                              void  * /*aOldValue*/,
                                              void  * aNewValue,
                                              void  * /*aArg*/)
{
    mQueryLoggingLevel = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// bug-19279 remote sysdba enable + sys can kill session
IDE_RC mmuProperty::callbackRemoteSysdbaEnable(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mRemoteSysdbaEnable = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
 */
IDE_RC mmuProperty::callbackServiceThrInitialLifeSpan(idvSQL * /*aStatistics*/,
                                                      SChar * /*aName*/,
                                                      void  * /*aOldValue*/,
                                                      void  *aNewValue,
                                                      void  * /*aArg*/)
{
    mServiceThrInitialLifeSpan = *((UInt*) aNewValue);
    return IDE_SUCCESS;
    
}

IDE_RC mmuProperty::callbackBusyServiceThrPenalty(idvSQL * /*aStatistics*/,
                                                  SChar * /*aName*/,
                                                  void  * /*aOldValue*/,
                                                  void  *aNewValue,
                                                  void  * /*aArg*/)
{
    mBusyServiceThrPenalty  = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}
   
IDE_RC mmuProperty::callbackMinMigrationTaskRate(idvSQL * /*aStatistics*/,
                                                 SChar * /*aName*/,
                                                 void  * /*aOldValue*/,
                                                 void  *aNewValue,
                                                 void  * /*aArg*/)

{
    mMinMigrationTaskRate = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackNewServiceThrCreateRate(idvSQL * /*aStatistics*/,
                                                    SChar * /*aName*/,
                                                    void  * /*aOldValue*/,
                                                    void  *aNewValue,
                                                    void  * /*aArg*/)
{
    mNewServiceThrCreateRate  = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

    
IDE_RC mmuProperty::callbackNewServiceThrCreateRateGap(idvSQL * /*aStatistics*/,
                                                       SChar * /*aName*/,
                                                       void  * /*aOldValue*/,
                                                       void  *aNewValue,
                                                       void  * /*aArg*/)
{
    mNewServiceThrCreateRateGap  = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

    
IDE_RC mmuProperty::callbackSerivceThrExitRate(idvSQL * /*aStatistics*/,
                                               SChar * /*aName*/,
                                               void  * /*aOldValue*/,
                                               void  *aNewValue,
                                               void  * /*aArg*/)
{
    mSerivceThrExitRate = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackMinTaskCntForThrLive(idvSQL * /*aStatistics*/,
                                                 SChar * /*aName*/,
                                                 void  * /*aOldValue*/,
                                                 void  *aNewValue,
                                                 void  * /*aArg*/)
{
    mMinTaskCntForThrLive = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackShutdownImmediateTimeout(idvSQL * /*aStatistics*/,
                                                     SChar * /*aName*/,
                                                     void  * /*aOldValue*/,
                                                     void  *aNewValue,
                                                     void  * /*aArg*/)
{
    mShutdownImmediateTimeout = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackStatementPartialScanCount(idvSQL * /*aStatistics*/,
                                                      SChar * /*aName*/,
                                                      void  * /*aOldValue*/,
                                                      void  *aNewValue,
                                                      void  * /*aArg*/)
{
    mStatementListPartialScanCount = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

//fix BUG-30949 A waiting time for enqueue event in transformed dedicated thread should not be infinite.
IDE_RC mmuProperty::callbackMaxEnqWaitTime(idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/)
{
    mMaxEnqWaitTime = *((ULong*) aNewValue);
    return IDE_SUCCESS;
}
//fix BUG-31150, It needs to add  the property for frequency of  hot region LRU  list.
IDE_RC mmuProperty::callbackFrequencyForHotLruRegion(idvSQL * /*aStatistics*/,
                                                     SChar * /*aName*/,
                                                     void  * /*aOldValue*/,
                                                     void  *aNewValue,
                                                     void  * /*aArg*/)
{
    mFrequencyForHotLruRegion = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

/* BUG-36205 Plan Cache On/Off property for PSM */
IDE_RC mmuProperty::callbackSqlPlanCacheUseInPSM(idvSQL * /*aStatistics*/,
                                                 SChar * /*aName*/,
                                                 void  * /*aOldValue*/,
                                                 void  *aNewValue,
                                                 void  * /*aArg*/)
{
    mSqlPlanCacheUseInPSM = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

/* PROJ-2047 Strengthening LOB - LOBCACHE */
IDE_RC mmuProperty::callbackLobCacheThreshold(idvSQL * /*aStatistics*/,
                                              SChar * /*aName*/,
                                              void  * /*aOldValue*/,
                                              void  *aNewValue,
                                              void  * /*aArg*/)
{
    mLobCacheThreshold = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// bug-35371
IDE_RC mmuProperty::callbackXAHashSize(idvSQL * /*aStatistics*/,
                                       SChar * /*aName*/,
                                       void  * /*aOldValue*/,
                                       void  *aNewValue,
                                       void  * /*aArg*/)
{
    mXAHashSize = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}


IDE_RC mmuProperty::callbackXidMemPoolElementCount(idvSQL * /*aStatistics*/,
                                                   SChar * /*aName*/,
                                                   void  * /*aOldValue*/,
                                                   void  *aNewValue,
                                                   void  * /*aArg*/)
{
    mXidMemPoolElementCount = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackXidMutexPoolSize(idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /*aArg*/)
{
    mXidMutexPoolSize = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackJobSchedulerEnable(idvSQL * /*aStatistics*/,
                                               SChar * /*aName*/,
                                               void  * /*aOldValue*/,
                                               void  *aNewValue,
                                               void  * /*aArg*/)
{
    mJobSchedulerEnable = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackIPCDASleepTime(idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/)
{
    mIPCDASleepTime = *((UInt*) aNewValue);
    return IDE_SUCCESS;
}

/* PROJ-2626 Snapshot Export */
IDE_RC mmuProperty::callbackSnapshotMemThreshold(idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/)
{
    mSnapshotMemThreshold = *( (UInt *)aNewValue );
    return IDE_SUCCESS;
}

/* PROJ-2626 Snapshot Export */
IDE_RC mmuProperty::callbackSnapshotDiskUndoThreshold(idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/)
{
    mSnapshotDiskUndoThreshold = *( (UInt *)aNewValue );
    return IDE_SUCCESS;
}

/* PROJ-2677 */
IDE_RC mmuProperty::callbackReplicationDDLSync( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mReplicationDDLSync = *( (UInt *)aNewValue );

    rpi::setReplicationDDLSync( mReplicationDDLSync );

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackReplicationDDLSyncTimeout( idvSQL * /*aStatistics*/,
                                                       SChar  * /*aName*/,
                                                       void   * /*aOldValue*/,
                                                       void   * aNewValue,
                                                       void   * /*aArg*/ )
{
    mReplicationDDLSyncTimeout = *( (UInt *)aNewValue );

    rpi::setReplicationDDLSyncTimeout( mReplicationDDLSyncTimeout );

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackIndoubtFetchTimeout( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mIndoubtFetchTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC mmuProperty::callbackIndoubtFetchMethod( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mIndoubtFetchMethod = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

void mmuProperty::initialize()
{
}

void mmuProperty::destroy()
{

}

void mmuProperty::load()
{
    UInt                 sLoop = 0;
    UInt                 sIPACLCount = 0;
    SChar               *sIPACL = NULL;
    SChar               *sTk = NULL;
    idBool               sIPACLPermit = ID_TRUE;
    struct in6_addr      sIPACLAddr;
    SChar                sIPACLAddrStr[MM_IP_ACL_MAX_ADDR_STR];
    UInt                 sIPACLAddrFamily = 0;
    UInt                 sIPACLMask = 0;
    /* BUG-48515 */
    UInt                 sIPACLLimitSize = 0;

    IDE_ASSERT(idp::readPtr("DB_NAME", (void **)&mDbName) == IDE_SUCCESS);

    IDE_ASSERT(idp::readPtr("LOG_DIR", (void **)&mLogDir) == IDE_SUCCESS);

    IDE_ASSERT(idp::readPtr("SERVER_MSGLOG_DIR", (void **)&mServerMsglogDir)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::readPtr("UNIXDOMAIN_FILEPATH", (void **)&mUnixdomainFilepath)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::readPtr("LOGANCHOR_DIR", (void **)&mLogAnchorDir, 0)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::getMemValueCount("MEM_DB_DIR", &mDBDirCount) == IDE_SUCCESS);
    // bug-34051: must check limit count
    IDE_ASSERT(mDBDirCount <= SM_DB_DIR_MAX_COUNT);

    for( sLoop = 0; sLoop != mDBDirCount; ++sLoop )
    {
        IDE_ASSERT(idp::readPtr("MEM_DB_DIR", (void**)&mDBDir[sLoop], sLoop) == IDE_SUCCESS);
    }


    IDE_ASSERT(idp::read("SHM_DB_KEY", &mShmDbKey) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LOG_FILE_SIZE", &mLogFileSize) == IDE_SUCCESS);
    
    IDE_ASSERT(idp::read("DEFAULT_MEM_DB_FILE_SIZE",
                         &mDefaultMemDbFileSize) == IDE_SUCCESS);
        
    IDE_ASSERT(idp::read("PORT_NO", &mPortNo) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MAX_LISTEN", &mMaxListen) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MAX_CLIENT", &mMaxClient) == IDE_SUCCESS);

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_ASSERT(idp::read("STMTPAGETABLE_PREALLOC_RATIO", &mStmtpagetablePreallocRatio) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SESSION_MUTEXPOOL_FREE_LIST_MAXCNT", &mSessionMutexpoolFreelistMaxcnt) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SESSION_MUTEXPOOL_FREE_LIST_INITCNT", &mSessionMutexpoolFreelistInitcnt) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MMT_SESSION_LIST_MEMPOOL_SIZE", &mMmtSessionListMempoolSize) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MMC_MUTEXPOOL_MEMPOOL_SIZE", &mMmcMutexpoolMempoolSize) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MMC_STMTPAGETABLE_MEMPOOL_SIZE", &mMmcStmtpagetableMempoolSize) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("CM_DISCONN_DETECT_TIME",&mCmDetectTime) == IDE_SUCCESS);

    // BUG-19465 : CM_Buffer의 pending list를 제한
    IDE_ASSERT(idp::read("CM_BUFFER_MAX_PENDING_LIST", &mCmMaxPendingList)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("AUTO_COMMIT", &mAutoCommit) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("TRANSACTION_START_MODE", &mTxStartMode) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("QUEUE_GLOBAL_HASHTABLE_SIZE", &mQueueGlobalHashTableSize) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("QUEUE_SESSION_HASHTABLE_SIZE", &mQueueSessionHashTableSize) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("QUEUE_MAX_ENQ_WAIT_TIME", &mMaxEnqWaitTime) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("ISOLATION_LEVEL",&mIsolationLevel) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LOGIN_TIMEOUT", &mLoginTimeout) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("IDLE_TIMEOUT",&mIdleTimeout) == IDE_SUCCESS);

    /* BUG-28866 */
    IDE_ASSERT(idp::read("MM_SESSION_LOGGING",&mMmSessionLogging) == IDE_SUCCESS);

    /* BUG-31144 */
    IDE_ASSERT(idp::read("MAX_STATEMENTS_PER_SESSION",&mMaxStatementsPerSession) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("QUERY_TIMEOUT",&mQueryTimeout) == IDE_SUCCESS);

    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    IDE_ASSERT(idp::read("DDL_TIMEOUT",&mDdlTimeout) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("FETCH_TIMEOUT",&mFetchTimeout) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("UTRANS_TIMEOUT",&mUtransTimeout) == IDE_SUCCESS);
    
    IDE_ASSERT(idp::read("IPC_CHANNEL_COUNT",&mIpcChannelCount) == IDE_SUCCESS);
    
    /* PROJ-2616 */
#if defined(ALTI_CFG_OS_LINUX)
    IDE_ASSERT(idp::read("IPCDA_CHANNEL_COUNT",&mIPCDAChannelCount) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("IPCDA_SERVER_SLEEP_TIME",&mIPCDASleepTime) == IDE_SUCCESS);

    IDE_ASSERT(idp::readPtr("IPCDA_FILEPATH", (void **)&mIPCDAFilepath)
               == IDE_SUCCESS);
#endif

    /*
     * TASK-5894 Permit sysdba via IPC
     *
     * SYSDBA를 위한 채널을 하나 추가한다.
     */
    mIpcChannelCount += 1;

    IDE_ASSERT(idp::read("ADMIN_MODE",&mAdminMode) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MEMORY_COMPACT_TIME",&mMemoryCompactTime) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__SHOW_ERROR_STACK", &mShowErrorStack) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("TRANSACTION_TABLE_SIZE", &mTxTabSize) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("XA_HEURISTIC_COMPLETE", &mXaComplete) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("XA_INDOUBT_TX_TIMEOUT", &mXaTimeout) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SELECT_HEADER_DISPLAY", &mSelectHeaderDisplay) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MULTIPLEXING_THREAD_COUNT", &mMultiplexingThreadCount) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MULTIPLEXING_MAX_THREAD_COUNT", &mMultiplexingMaxThreadCount) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MULTIPLEXING_POLL_TIMEOUT", &mMultiplexingPollTimeout) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MULTIPLEXING_CHECK_INTERVAL", &mMultiplexingCheckInterval) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("COMMIT_WRITE_WAIT_MODE", &mCommitWriteWaitMode) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("TRX_UPDATE_MAX_LOGSIZE", &mUpdateMaxLogSize)
               == IDE_SUCCESS );

    //PROJ-1436 SQL Plan Cache.
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_SIZE",&mSqlPlanCacheSize) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_BUCKET_CNT",&mSqlPlanCacheBucketCnt) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_INIT_PCB_CNT",&mSqlPlanCacheInitPCBCnt) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_INIT_PARENT_PCO_CNT",&mSqlPlanCacheInitParentPCOCnt) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_INIT_CHILD_PCO_CNT",&mSqlPlanCacheInitChildPCOCnt) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_HOT_REGION_LRU_RATIO",&mSqlPlanCacheHotRegionLruRatio) == IDE_SUCCESS);
    //fix BUG-31150, It needs to add  the property for frequency of  hot region LRU  list.
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_HOT_REGION_FREQUENCY",&mFrequencyForHotLruRegion) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_USE_IN_PSM",&mSqlPlanCacheUseInPSM) == IDE_SUCCESS);
    /* BUG-35521 Add TryLatch in PlanCache. */
    IDE_ASSERT(idp::read("SQL_PLAN_CACHE_PARENT_PCO_XLATCH_TRY_CNT",
                         &mSqlPlanCacheParentPCOXLatchTryCnt)
               == IDE_SUCCESS);
    //fix BUG-23776, XA ROLLBACK시 XID가 ACTIVE일때 대기시간을
    //QueryTime Out이 아니라,Property를 제공해야 함.
    IDE_ASSERT(idp::read("XA_ROLLBACK_TIMEOUT",&mXaRollbackTimeOut) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__QUERY_LOGGING_LEVEL", &mQueryLoggingLevel)
               == IDE_SUCCESS );
    
    // bug-19279 remote sysdba enable + sys can kill session
    IDE_ASSERT(idp::read("REMOTE_SYSDBA_ENABLE", &mRemoteSysdbaEnable)
               == IDE_SUCCESS);

    // BUG-24993 네트워크 에러 메시지 log 여부
    IDE_ASSERT(idp::read("NETWORK_ERROR_LOG",&mNetworkErrorLog) == IDE_SUCCESS);
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
     */
    
    IDE_ASSERT(idp::read("SERVICE_THREAD_INITIAL_LIFESPAN",&mServiceThrInitialLifeSpan) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("BUSY_SERVICE_THREAD_PENALTY",&mBusyServiceThrPenalty) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MIN_MIGRATION_TASK_RATE",&mMinMigrationTaskRate) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("NEW_SERVICE_CREATE_RATE",&mNewServiceThrCreateRate) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("NEW_SERVICE_CREATE_RATE_GAP",&mNewServiceThrCreateRateGap) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SERVICE_THREAD_EXIT_RATE",&mSerivceThrExitRate) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("MIN_TASK_COUNT_FOR_THREAD_LIVE",&mMinTaskCntForThrLive) == IDE_SUCCESS);

    // fix BUG-30566
    IDE_ASSERT(idp::read("SHUTDOWN_IMMEDIATE_TIMEOUT",&mShutdownImmediateTimeout) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("STATEMENT_LIST_PARTIAL_SCAN_COUNT",&mStatementListPartialScanCount) == IDE_SUCCESS);


    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    IDE_ASSERT(idp::read("DEDICATED_THREAD_MODE", &mIsDedicatedMode) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("DEDICATED_THREAD_INIT_COUNT", &mDedicatedThreadInitCount) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("DEDICATED_THREAD_MAX_COUNT", &mDedicatedThreadMaxCount) == IDE_SUCCESS);
    /* Dedicated thread count should not exceed max client limitation */
    if ( mDedicatedThreadInitCount > mMaxClient )
    {
        mDedicatedThreadInitCount = mMaxClient;
    }
    if ( mDedicatedThreadMaxCount > mMaxClient )
    {
        mDedicatedThreadMaxCount = mMaxClient;
    }
    if ( mDedicatedThreadInitCount > mDedicatedThreadMaxCount )
    {
        mDedicatedThreadInitCount = mDedicatedThreadMaxCount;
    }
    IDE_ASSERT(idp::read("DEDICATED_THREAD_CHECK_INTERVAL", &mThreadCheckInterval) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("THREAD_CPU_AFFINITY", &mIsCPUAffinity) == IDE_SUCCESS);

    /* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
    IDE_ASSERT( idp::readPtr( "ACCESS_LIST_FILE", (void**)&mIPACLFile ) == IDE_SUCCESS );
    IDE_DASSERT( mmuProperty::mIPACLFile != NULL );

    if ( mIPACLFile[0] == '\0' )
    {
        IDE_ASSERT ( idp::getMemValueCount( "ACCESS_LIST",
                                            &sIPACLCount )
                     == IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    IDE_ASSERT(idp::read("LOB_CACHE_THRESHOLD", &mLobCacheThreshold)
               == IDE_SUCCESS);

    /* BUG-35332 The socket files can be moved */
    IDE_ASSERT(idp::readPtr("IPC_FILEPATH", (void **)&mIpcFilepath)
               == IDE_SUCCESS);

    // bug-35371
    IDE_ASSERT(idp::read("XA_HASH_SIZE",&mXAHashSize) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("XID_MEMPOOL_ELEMENT_COUNT",&mXidMemPoolElementCount) == IDE_SUCCESS);

    // bug-35382
    IDE_ASSERT(idp::read("XID_MUTEX_POOL_SIZE", &mXidMutexPoolSize)
            == IDE_SUCCESS);

    /* PROJ-1438 Job Scheduler */
    IDE_ASSERT(idp::read("JOB_THREAD_COUNT", &mJobThreadCount) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("JOB_THREAD_QUEUE_SIZE", &mJobThreadQueueSize) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("JOB_SCHEDULER_ENABLE", &mJobSchedulerEnable) == IDE_SUCCESS);

    /* PROJ-2474 SSL/TLS */
    IDE_ASSERT(idp::read("SSL_PORT_NO", &mSslPortNo) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SSL_ENABLE", &mSslEnable) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SSL_MAX_LISTEN", &mSslMaxListen) == IDE_SUCCESS);

    /* BUG-41168 SSL extension*/
    IDE_ASSERT(idp::read("TCP_ENABLE", &mTcpEnable) == IDE_SUCCESS);

    /* PROJ-2626 Snapshot Export */
    IDE_ASSERT(idp::read("SNAPSHOT_MEM_THRESHOLD", &mSnapshotMemThreshold ) == IDE_SUCCESS);
    /* PROJ-2626 Snapshot Export */
    IDE_ASSERT(idp::read("SNAPSHOT_DISK_UNDO_THRESHOLD", &mSnapshotDiskUndoThreshold ) == IDE_SUCCESS);

    /* PROJ-2677 */
    IDE_ASSERT( idp::read( "REPLICATION_DDL_SYNC", &mReplicationDDLSync ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "REPLICATION_DDL_SYNC_TIMEOUT", &mReplicationDDLSyncTimeout ) == IDE_SUCCESS );

    /* PROJ-2681 */
    IDE_ASSERT(idp::read("IB_ENABLE", &mIBEnable) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("IB_PORT_NO", &mIBPortNo) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("IB_MAX_LISTEN", &mIBMaxListen) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("IB_LISTENER_DISABLE", &mIBListenerDisable) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("IB_LATENCY", &mIBLatency) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("IB_CONCHKSPIN", &mIBConChkSpin) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SHARED_TRANS_HASH_BUCKET_COUNT", &mSharedTransHashBucketCount) == IDE_SUCCESS);

    /* PROJ-2733 */
    IDE_ASSERT( idp::read("INDOUBT_FETCH_TIMEOUT", &mIndoubtFetchTimeout ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read("INDOUBT_FETCH_METHOD", &mIndoubtFetchMethod ) == IDE_SUCCESS );
    

    /* proj-1538 ipv6: initialize all entries. */
    mmuAccessList::clear();

    /* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
    if ( mIPACLFile[0] == '\0' )
    {
        /* fix BUG-28834 IP Access Control List 잘못되었습니다. */
        /* proj-1538 ipv6: handle ipv6 addr entries */
        for ( sLoop = 0; sLoop < sIPACLCount; ++sLoop )
        {
            IDE_ASSERT(idp::readPtr("ACCESS_LIST",
                                    (void**)&sIPACL,
                                    sLoop)
                       == IDE_SUCCESS);

            /* First Value, "Permit" or "Deny" */
            sTk = idlOS::strtok( sIPACL, " \t," );
            /* if no entry in the file, sTk: NULL,  mIPACLCount: 1. */
            if (sTk == NULL)
            {
                break;
            }
            else
            {
                /* Nothing To Do */
            }

            if (idlOS::strcasecmp(sTk, "DENY") == 0)
            {
                sIPACLPermit = ID_FALSE;
            }
            else
            {
                sIPACLPermit = ID_TRUE;
            }

            /* Second Value, IP Address */
            sTk = idlOS::strtok(NULL, " \t,");
            if (sTk == NULL)
            {
                break;
            }
            else
            {
                /* Nothing To Do */
            }

            /* ipv6 addr includes colons */
            if (idlOS::strchr(sTk, ':'))
            {

                /* inet_pton returns a negative value or 0 if error */
                if (idlOS::inet_pton(AF_INET6, sTk, &sIPACLAddr) <= 0)
                {
                    break;
                }
                else
                {
                    sIPACLAddrFamily = AF_INET6;
                    idlOS::snprintf( sIPACLAddrStr,
                                     MM_IP_ACL_MAX_ADDR_STR,
                                     "%s",
                                     sTk );
                }
            }
            /* ipv4 addr */
            else
            {
                /* inet_pton returns a negative value or 0 if error */
               if (idlOS::inet_pton(AF_INET, sTk, &sIPACLAddr) <= 0)
                {
                    break;
                }
                else
                {
                    sIPACLAddrFamily = AF_INET;
                    idlOS::snprintf( sIPACLAddrStr,
                                     MM_IP_ACL_MAX_ADDR_STR,
                                     "%s",
                                     sTk );
                }
            }

            /* Third Value, IP mask
             * ipv4: d.d.d.d mask form
             * ipv6: prefix bit mask length by which to compare */
            sTk = idlOS::strtok(NULL, " \t,");
            if (sTk == NULL)
            {
                break;
            }
            else
            {
                /* Nothing To Do */
            }

            if (sIPACLAddrFamily == AF_INET)
            {
                sIPACLMask = idlOS::inet_addr(sTk);
            }
            else
            {
                sIPACLMask = idlOS::atoi(sTk);
                if (sIPACLMask > 128)
                {
                    sIPACLMask = 128; /* max ipv6 addr bits: 128 */
                }
                else
                {
                    /* Nothing To Do */
                }
            }

            sTk = idlOS::strtok(NULL, " \t,");
            if ( sTk != NULL && sIPACLPermit == ID_TRUE )
            {
                /* ip 최대 허용 size */
                sIPACLLimitSize = idlOS::atoi(sTk);
            }
            else
            {
                /* ip 최대 허용 size가 셋팅하지 않았으므로 0(무한대) */
                sIPACLLimitSize = 0;
            }

            /* access list에 추가 */
            if ( mmuAccessList::add( sIPACLPermit,
                                     &sIPACLAddr,
                                     sIPACLAddrStr,
                                     sIPACLAddrFamily,
                                     sIPACLMask,
                                     sIPACLLimitSize ) != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0, "[ACCESS LIST] fail to loading" );
                IDE_ASSERT( 0 );
            }
            else
            {
                /* Nothing to do */
            }
         }
    }
    else
    {
        if ( mmuAccessList::loadAccessList() != IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "[ACCESS LIST] fail to loading" );
            IDE_ASSERT( 0 );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /*
     * Regist Callback Functions
     */
    idp::setupAfterUpdateCallback("LOGIN_TIMEOUT",  callbackLoginTimeout);
    idp::setupAfterUpdateCallback("IDLE_TIMEOUT",   callbackIdleTimeout);
    /* BUG-28866 */
    idp::setupAfterUpdateCallback("MM_SESSION_LOGGING",   callbackMmSessionLogging);
    
    /* BUG-31144 */
    idp::setupAfterUpdateCallback("MAX_STATEMENTS_PER_SESSION", callbackMaxStatementsPerSession);

    idp::setupAfterUpdateCallback("QUERY_TIMEOUT",  callbackQueryTimeout);
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    idp::setupAfterUpdateCallback("DDL_TIMEOUT",  callbackDdlTimeout);
    idp::setupAfterUpdateCallback("FETCH_TIMEOUT",  callbackFetchTimeout);
    idp::setupAfterUpdateCallback("UTRANS_TIMEOUT", callbackUransTimeout);

    idp::setupBeforeUpdateCallback("ADMIN_MODE",     callbackAdminMode);
    idp::setupBeforeUpdateCallback("AUTO_COMMIT",    callbackAutoCommit);
    idp::setupBeforeUpdateCallback("__SHOW_ERROR_STACK", callbackShowErrorStack);

    idp::setupBeforeUpdateCallback("SELECT_HEADER_DISPLAY", callbackSelectHeaderDisplay);

    idp::setupAfterUpdateCallback("MULTIPLEXING_MAX_THREAD_COUNT",
                                  callbackMultiplexingMaxThreadCount);
    
    idp::setupAfterUpdateCallback("MULTIPLEXING_POLL_TIMEOUT",
                                  callbackMultiplexingPollTimeout);
    
    idp::setupAfterUpdateCallback("MULTIPLEXING_CHECK_INTERVAL",
                                  callbackMultiplexingCheckInterval);
    idp::setupAfterUpdateCallback("COMMIT_WRITE_WAIT_MODE",
                                  callbackCommitWriteWaitMode);

    idp::setupAfterUpdateCallback("TRX_UPDATE_MAX_LOGSIZE", callbackUpdateMaxLogSize);
    //PROJ-1436 SQL-Plan Cache.
    idp::setupAfterUpdateCallback("SQL_PLAN_CACHE_SIZE",callbackSqlPlanCacheSize);
    idp::setupAfterUpdateCallback("SQL_PLAN_CACHE_HOT_REGION_LRU_RATIO",callbackSqlPlanCacheHotRegionLruRatio);
    /* BUG-35521 Add TryLatch in PlanCache. */
    idp::setupAfterUpdateCallback("SQL_PLAN_CACHE_PARENT_PCO_XLATCH_TRY_CNT",
                                  callbackSqlPlanCacheParentPCOXLatchTryCnt);

    idp::setupAfterUpdateCallback("XA_ROLLBACK_TIMEOUT",callbackXaRollbackTimeOut);
    
    idp::setupAfterUpdateCallback("__QUERY_LOGGING_LEVEL", callbackQueryLoggingLevel);

    // bug-19279 remote sysdba enable + sys can kill session
    idp::setupAfterUpdateCallback("REMOTE_SYSDBA_ENABLE",  callbackRemoteSysdbaEnable);
    // BUG-26280 
    idp::setupAfterUpdateCallback("NETWORK_ERROR_LOG", callbackNetworkErrorLog);
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase  */
    idp::setupAfterUpdateCallback("SERVICE_THREAD_INITIAL_LIFESPAN",callbackServiceThrInitialLifeSpan);
    idp::setupAfterUpdateCallback("BUSY_SERVICE_THREAD_PENALTY",callbackBusyServiceThrPenalty);
    idp::setupAfterUpdateCallback("MIN_MIGRATION_TASK_RATE",callbackMinMigrationTaskRate);
    idp::setupAfterUpdateCallback("NEW_SERVICE_CREATE_RATE",callbackNewServiceThrCreateRate);
    idp::setupAfterUpdateCallback("NEW_SERVICE_CREATE_RATE_GAP",callbackNewServiceThrCreateRateGap);
    idp::setupAfterUpdateCallback("SERVICE_THREAD_EXIT_RATE",callbackSerivceThrExitRate);
    idp::setupAfterUpdateCallback("MIN_TASK_COUNT_FOR_THREAD_LIVE",callbackMinTaskCntForThrLive);

    // fix BUG-30566
    idp::setupAfterUpdateCallback("SHUTDOWN_IMMEDIATE_TIMEOUT",callbackShutdownImmediateTimeout);
    idp::setupAfterUpdateCallback("STATEMENT_LIST_PARTIAL_SCAN_COUNT",callbackStatementPartialScanCount);
    idp::setupAfterUpdateCallback("QUEUE_MAX_ENQ_WAIT_TIME",callbackMaxEnqWaitTime);
    //fix BUG-31150, It needs to add  the property for frequency of  hot region LRU  list.
    idp::setupAfterUpdateCallback("SQL_PLAN_CACHE_HOT_REGION_FREQUENCY",callbackFrequencyForHotLruRegion);
    idp::setupAfterUpdateCallback("SQL_PLAN_CACHE_USE_IN_PSM",callbackSqlPlanCacheUseInPSM);
    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    idp::setupAfterUpdateCallback("LOB_CACHE_THRESHOLD", callbackLobCacheThreshold);

    // bug-35371
    idp::setupAfterUpdateCallback("XA_HASH_SIZE",callbackXAHashSize);

    // bug-35381
    idp::setupAfterUpdateCallback("XID_MEMPOOL_ELEMENT_COUNT",callbackXidMemPoolElementCount);

    // bug-35382
    idp::setupAfterUpdateCallback("XID_MUTEX_POOL_SIZE", callbackXidMutexPoolSize);

    /* PROJ-1438 Job Scheduler */
    ( void )idp::setupAfterUpdateCallback("JOB_SCHEDULER_ENABLE", callbackJobSchedulerEnable);
    
    /* PROJ-2616 */
#if defined(ALTI_CFG_OS_LINUX)
    idp::setupAfterUpdateCallback("IPCDA_SERVER_SLEEP_TIME", callbackIPCDASleepTime);
#endif

    /* PROJ-2626 Snapshot Export */
    ( void )idp::setupAfterUpdateCallback("SNAPSHOT_MEM_THRESHOLD", callbackSnapshotMemThreshold );
    ( void )idp::setupAfterUpdateCallback("SNAPSHOT_DISK_UNDO_THRESHOLD", callbackSnapshotDiskUndoThreshold );

    ( void )idp::setupAfterUpdateCallback( "REPLICATION_DDL_SYNC", callbackReplicationDDLSync );
    ( void )idp::setupAfterUpdateCallback( "REPLICATION_DDL_SYNC_TIMEOUT", callbackReplicationDDLSyncTimeout );

    /* PROJ-2733 */
    idp::setupAfterUpdateCallback( "INDOUBT_FETCH_TIMEOUT", callbackIndoubtFetchTimeout );
    idp::setupAfterUpdateCallback( "INDOUBT_FETCH_METHOD",  callbackIndoubtFetchMethod );
}

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

void*  mmuProperty::callbackForGettingArgument(mmuPropertyArgument *aArg,
                                              idpArgumentID        aID)
{
    void *sValue = NULL;

    switch(aID)
    {
        case IDP_ARG_USERID :
            sValue = (void *)&(aArg->mUserID);
            break;
        case IDP_ARG_TRANSID:
            sValue = (void *)(aArg->mTrans);
            break;
        default:
            IDE_CALLBACK_FATAL("unknown argument id");
            break;
    }

    return sValue;
}
