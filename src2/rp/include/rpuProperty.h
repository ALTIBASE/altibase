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
 

/***********************************************************************
 * $Id: rpuProperty.h 90491 2021-04-07 07:02:29Z lswhh $
 **********************************************************************/

#ifndef _O_RPU_PROPERTY_H_
#define _O_RPU_PROPERTY_H_ 1

#include <idl.h>
#include <ideErrorMgr.h>
#include <idp.h>
#include <idErrorCode.h>

#define RPU_REPLICATION_MAX_EAGER_PARALLEL_FACTOR IDP_REPLICATION_MAX_EAGER_PARALLEL_FACTOR

#define RPU_SID                                   (rpuProperty::mSID)
#define RPU_DB_NAME                               (rpuProperty::mDbName)
#define RPU_REPLICATION_PORT_NO                   (rpuProperty::mPortNo)
#define RPU_REPLICATION_CONNECT_TIMEOUT           (rpuProperty::mConnectTimeout)
#define RPU_REPLICATION_HBT_DETECT_TIME           (rpuProperty::mDetectTime)
#define RPU_REPLICATION_HBT_DETECT_HIGHWATER_MARK (rpuProperty::mDetectHighWaterMark)
#define RPU_ISOLATION_LEVEL                       (rpuProperty::mIsolationLevel)
#define RPU_REPLICATION_RECEIVE_TIMEOUT           (rpuProperty::mReceiveTimeout)

#define RPU_REPLICATION_SENDER_SLEEP_TIMEOUT      (rpuProperty::mSenderSleepTimeout)
#define RPU_REPLICATION_SYNC_LOCK_TIMEOUT         (rpuProperty::mSyncLockTimeout)
#define RPU_REPLICATION_SYNC_TUPLE_COUNT          (rpuProperty::mSyncTupleCount)
#define RPU_REPLICATION_UPDATE_REPLACE            (rpuProperty::mUpdateReplace)
#define RPU_REPLICATION_INSERT_REPLACE            (rpuProperty::mInsertReplace)
#define RPU_TRCLOG_SET_INSERT_SM_LOG              (rpuProperty::mTrcInsertSMLog)
#define RPU_LOG_FILE_SIZE                         (rpuProperty::mLogFileSize)
#define RPU_REPLICATION_TIMESTAMP_RESOLUTION      (rpuProperty::mTimestampResolution)
#define RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT   (rpuProperty::mConnectReceiveTimeout)
#define RPU_REPLICATION_SENDER_AUTO_START         (rpuProperty::mSenderAutoStart)
#define RPU_REPLICATION_SERVICE_WAIT_MAX_LIMIT    (rpuProperty::mServiceWaitMaxLimit)
#define RPU_PREFETCH_LOGFILE_COUNT                (rpuProperty::mPrefetchLogfileCount)
#define RPU_SENDER_SLEEP_TIME                     (rpuProperty::mSenderSleepTime)
#define RPU_KEEP_ALIVE_CNT                        (rpuProperty::mKeepAliveCnt)
#define RPU_REPLICATION_MAX_LOGFILE               (rpuProperty::mMaxLogFile)
#define RPU_SENDER_START_AFTER_GIVING_UP          (rpuProperty::mSenderStartAfterGivingUp)
#define RPU_REPLICATION_RECEIVER_XLOG_QUEUE_SIZE  (rpuProperty::mReceiverXLogQueueSz)
#define RPU_REPLICATION_ACK_XLOG_COUNT            (rpuProperty::mAckXLogCount)
#define RPU_REPLICATION_SYNC_LOG                  (rpuProperty::mSyncLog)
#define RPU_REPLICATION_PERFORMANCE_TEST          (rpuProperty::mPerformanceTest)
#define RPU_REPLICATION_LOG_BUFFER_SIZE           (rpuProperty::mRPLogBufferSize)
#define RPU_REPLICATION_RECOVERY_MAX_TIME         (rpuProperty::mRPRecoveryMaxTime)
#define RPU_REPLICATION_RECOVERY_MAX_LOGFILE      (rpuProperty::mRecoveryMaxLogFile)
#define RPU_REPLICATION_RECOVERY_REQUEST_TIMEOUT  (rpuProperty::mRecoveryRequestTimeout)
#define RPU_REPLICATION_TX_VICTIM_TIMEOUT         (rpuProperty::mReplicationTXVictimTimeout)
#define RPU_REPLICATION_DDL_ENABLE                (rpuProperty::mDDLEnable)
#define RPU_REPLICATION_DDL_ENABLE_LEVEL          (rpuProperty::mDDLEnableLevel)
#define RPU_REPLICATION_POOL_ELEMENT_SIZE         (rpuProperty::mPoolElementSize)
#define RPU_REPLICATION_POOL_ELEMENT_COUNT        (rpuProperty::mPoolElementCount)
#define RPU_REPLICATION_EAGER_PARALLEL_FACTOR     (rpuProperty::mEagerParallelFactor)
#define RPU_REPLICATION_COMMIT_WRITE_WAIT_MODE    (rpuProperty::mCommitWriteWaitMode)
#define RPU_REPLICATION_SERVER_FAILBACK_MAX_TIME  (rpuProperty::mServerFailbackMaxTime)
#define RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT (rpuProperty::mFailbackPKQueueTimeout)
#define RPU_REPLICATION_FAILBACK_INCREMENTAL_SYNC (rpuProperty::mFailbackIncrementalSync)
#define RPU_REPLICATION_MAX_LISTEN                (rpuProperty::mMaxListen)
#define RPU_REPLICATION_TRANSACTION_POOL_SIZE     (rpuProperty::mTransPoolSize)
#define RPU_REPLICATION_STRICT_EAGER_MODE         (rpuProperty::mStrictEagerMode)
#define RPU_REPLICATION_EAGER_MAX_YIELD_COUNT     (rpuProperty::mEagerMaxYieldCount)
#define RPU_REPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT      (rpuProperty::mEagerReceiverMaxErrorCount)
#define RPU_REPLICATION_BEFORE_IMAGE_LOG_ENABLE   (rpuProperty::mBeforeImageLogEnable)
#define RPU_REPLICATION_SENDER_COMPRESS_XLOG      (rpuProperty::mSenderCompressXLog)
#define RPU_REPLICATION_SENDER_COMPRESS_XLOG_LEVEL          (rpuProperty::mSenderCompressXLogLevel)
#define RPU_REPLICATION_MAX_COUNT                 (rpuProperty::mMaxReplicationCount)
#define RPU_REPLICATION_ALLOW_DUPLICATE_HOSTS     (rpuProperty::mAllowDuplicateHosts)
#define RPU_REPLICATION_SENDER_ENCRYPT_XLOG       (rpuProperty::mSenderEncryptXLog)
#define RPU_REPLICATION_SENDER_SEND_TIMEOUT       (rpuProperty::mSenderSendTimeout)
#define RPU_REPLICATION_USE_V6_PROTOCOL           (rpuProperty::mUseV6Protocol)
#define RPU_REPLICATION_GAPLESS_MAX_WAIT_TIME     (rpuProperty::mGaplessMaxWaitTime)
#define RPU_REPLICATION_GAPLESS_ALLOW_TIME        (rpuProperty::mGaplessAllowTime)
#define RPU_REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE         (rpuProperty::mReceiverApplierQueueSize)
#define RPU_REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE        (rpuProperty::mReceiverApplierAssignMode)
#define RPU_REPLICATION_FORCE_RECIEVER_APPLIER_COUNT        (rpuProperty::mForceReceiverApplierCount)
#define RPU_REPLICATION_GROUPING_TRANSACTION_MAX_COUNT (rpuProperty::mGroupingTransactionMaxCount)
#define RPU_REPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE (rpuProperty::mGroupingAheadReadNextLogFile)
#define RPU_REPLICATION_RECONNECT_MAX_COUNT       (rpuProperty::mReconnectMaxCount)
#define RPU_REPLICATION_SYNC_APPLY_METHOD         (rpuProperty::mSyncApplyMethod)
#define RPU_IS_CPU_AFFINITY                       (rpuProperty::mIsCPUAffinity)
#define RPU_REPLICATION_EAGER_KEEP_LOGFILE_COUNT  (rpuProperty::mEagerKeepLogFileCount)
#define RPU_REPLICATION_FORCE_SQL_APPLY_ENABLE    (rpuProperty::mForceSqlApplyEnable)
#define RPU_REPLICATION_SQL_APPLY_ENABLE          (rpuProperty::mSqlApplyEnable)
#define RPU_REPLICATION_SET_RESTARTSN             (rpuProperty::mSetRestartSN)
#define RPU_REPLICATION_SENDER_RETRY_COUNT        (rpuProperty::mSenderRetryCount)
#define RPU_REPLICATION_ALLOW_QUEUE               (rpuProperty::mAllowQueue)
#define RPU_REPLICATION_DDL_SYNC                  (rpuProperty::mReplicationDDLSync)
#define RPU_REPLICATION_DDL_SYNC_TIMEOUT          (rpuProperty::mReplicationDDLSyncTimeout)
#define RPU_REPLICATION_RECEIVER_APPLIER_YIELD_COUNT        (rpuProperty::mReceiverApplierYieldCount)

#define RPU_IB_ENABLE                             (rpuProperty::mIBEnable)
#define RPU_REPLICATION_IB_PORT_NO                (rpuProperty::mIBPortNo)
#define RPU_REPLICATION_IB_LATENCY                (rpuProperty::mIBLatency)

#define RPU_REPLICATION_GAP_UNIT                  (rpuProperty::mGapUnit)

#define RPU_REPLICATION_CHECK_SRID_IN_GEOMETRY_ENABLE    (rpuProperty::mCheckSRIDInGeometryEnable)
#define RPU_REPLICATION_XLOGFILE_PREPARE_COUNT           (rpuProperty::mXLogfilePrepareCount)
#define RPU_REPLICATION_XLOGFILE_SIZE                    (rpuProperty::mXLogfileSize)
#define RPU_REPLICATION_XLOGFILE_REMOVE_INTERVAL         (rpuProperty::mXLogfileRemoveInterval)
#define RPU_REPLICATION_XLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE (rpuProperty::mXLogfileRemoveIntervalByFileCreate)
#define RPU_REPLICATION_XLOGFILE_DIR_PATH                (rpuProperty::mXLogDirPath)

#define RPU_REPLICATION_META_ITEM_COUNT_DIFF_ENABLE     (rpuProperty::mMetaItemCountDiffEnable)

class rpuProperty
{
public:
    static IDE_RC initProperty();
    static IDE_RC finalProperty();
    static IDE_RC load();
    static IDE_RC checkConflict();
    static void setLowerModuleStaticProperty();

    static SChar          *mSID;
    static SChar          *mDbName;
    static UInt            mPortNo;
    static UInt            mConnectTimeout;
    static UInt            mDetectTime;
    static UInt            mDetectHighWaterMark;
    static UInt            mIsolationLevel;
    static UInt            mReceiveTimeout;
    static UInt            mPropagation;
    static UInt            mSenderSleepTimeout;
    static UInt            mSyncLockTimeout;
    static ULong           mSyncTupleCount;
    static UInt            mUpdateReplace;
    static UInt            mInsertReplace;
    static UInt            mTrcHBTLog;
    static UInt            mTrcConflictLog;
    static UInt            mTrcInsertSMLog;
    static ULong           mLogFileSize;
    static UInt            mTimestampResolution;
    static UInt            mConnectReceiveTimeout;
    static UInt            mSenderAutoStart;
    static UInt            mSenderStartAfterGivingUp;
    static UInt            mServiceWaitMaxLimit;
    static UInt            mTransDurabilityLevel;
    static UInt            mPrefetchLogfileCount;
    static UInt            mSenderSleepTime;
    static UInt            mKeepAliveCnt;
    static SInt            mMaxLogFile;
    static UInt            mReceiverXLogQueueSz;
    static UInt            mAckXLogCount;
    static UInt            mSyncLog;
    static UInt            mPerformanceTest;
    static UInt            mRPLogBufferSize;
    static UInt            mRPRecoveryMaxTime;
    static UInt            mRecoveryMaxLogFile;
    static UInt            mRecoveryRequestTimeout;
    static UInt            mReplicationTXVictimTimeout;
    static UInt            mDDLEnable;
    static UInt            mDDLEnableLevel;
    static UInt            mPoolElementSize;    // PROJ-1705
    static UInt            mPoolElementCount;   // PROJ-1705
    static UInt            mEagerParallelFactor;
    static UInt            mCommitWriteWaitMode;
    static UInt            mServerFailbackMaxTime;
    static UInt            mFailbackPKQueueTimeout;
    static UInt            mFailbackIncrementalSync;
    static UInt            mMaxListen;
    static UInt            mTransPoolSize;
    static UInt            mStrictEagerMode;
    static SInt            mEagerMaxYieldCount;
    static UInt            mEagerReceiverMaxErrorCount;
    static UInt            mBeforeImageLogEnable;
    static SInt            mSenderCompressXLog;
    static UInt            mSenderCompressXLogLevel;
    static UInt            mMaxReplicationCount;     /* BUG-37482 */
    static UInt            mAllowDuplicateHosts;
    static SInt            mSenderEncryptXLog;  /* BUG-38102 */
    static UInt            mSenderSendTimeout;
    static UInt            mUseV6Protocol;
    static ULong           mGaplessMaxWaitTime;
    static ULong           mGaplessAllowTime;
    static UInt            mReceiverApplierQueueSize;
    static SInt            mReceiverApplierAssignMode;
    static UInt            mForceReceiverApplierCount;
    static UInt            mGroupingTransactionMaxCount;
    static UInt            mGroupingAheadReadNextLogFile;
    static UInt            mReconnectMaxCount;
    static UInt            mSyncApplyMethod;
    static idBool          mIsCPUAffinity;
    static UInt            mEagerKeepLogFileCount;
    static UInt            mForceSqlApplyEnable;
    static UInt            mSqlApplyEnable;
    static UInt            mSetRestartSN;
    static UInt            mSenderRetryCount;
    static UInt            mAllowQueue;
    static UInt            mReplicationDDLSync;
    static UInt            mReplicationDDLSyncTimeout;
    static UInt            mReceiverApplierYieldCount;
    
    static UInt            mIBEnable;
    static UInt            mIBPortNo;
    static UInt            mIBLatency;
    
    static ULong           mGapUnit;
    
    static UInt            mCheckSRIDInGeometryEnable;
    static UInt            mXLogfilePrepareCount;
    static ULong           mXLogfileSize;
    static UInt            mXLogfileRemoveInterval;
    static UInt            mXLogfileRemoveIntervalByFileCreate;

    static UInt            mMetaItemCountDiffEnable;
    static SChar          *mXLogDirPath;

    static IDE_RC notifyREPLICATION_SYNC_LOCK_TIMEOUT(idvSQL* /* aStatistics */,
                                                      SChar *aName,
                                                      void  *aOldValue,
                                                      void  *aNewValue,
                                                      void  *aArg);

    static IDE_RC notifyREPLICATION_TIMESTAMP_RESOLUTION(idvSQL* /* aStatistics */,
                                                         SChar *aName,
                                                         void  *aOldValue,
                                                         void  *aNewValue,
                                                         void  *aArg);
    //fix BUG-9788
    static IDE_RC notifyREPLICATION_UPDATE_REPLACE(idvSQL* /* aStatistics */,
                                                   SChar *aName,
                                                   void  *aOldValue,
                                                   void  *aNewValue,
                                                   void  *aArg);
    static IDE_RC notifyREPLICATION_INSERT_REPLACE(idvSQL* /* aStatistics */,
                                                   SChar *aName,
                                                   void  *aOldValue,
                                                   void  *aNewValue,
                                                   void  *aArg);
    static IDE_RC notifyREPLICATION_CONNECT_TIMEOUT(idvSQL* /* aStatistics */,
                                                    SChar *aName,
                                                    void  *aOldValue,
                                                    void  *aNewValue,
                                                    void  *aArg);
    static IDE_RC notifyREPLICATION_RECEIVE_TIMEOUT(idvSQL* /* aStatistics */,
                                                    SChar *aName,
                                                    void  *aOldValue,
                                                    void  *aNewValue,
                                                    void  *aArg);
    static IDE_RC notifyREPLICATION_SENDER_SLEEP_TIMEOUT(idvSQL* /* aStatistics */,
                                                         SChar *aName,
                                                         void  *aOldValue,
                                                         void  *aNewValue,
                                                         void  *aArg);
    static IDE_RC notifyREPLICATION_HBT_DETECT_TIME(idvSQL* /* aStatistics */,
                                                    SChar *aName,
                                                    void  *aOldValue,
                                                    void  *aNewValue,
                                                    void  *aArg);
    static IDE_RC notifyREPLICATION_HBT_DETECT_HIGHWATER_MARK(idvSQL* /* aStatistics */,
                                                              SChar *aName,
                                                              void  *aOldValue,
                                                              void  *aNewValue,
                                                              void  *aArg);
    static IDE_RC notifyREPLICATION_SYNC_TUPLE_COUNT(idvSQL* /* aStatistics */,
                                                     SChar *aName,
                                                     void  *aOldValue,
                                                     void  *aNewValue,
                                                     void  *aArg);
    static IDE_RC notifyREPLICATION_CONNECT_RECEIVE_TIMEOUT(idvSQL* /* aStatistics */,
                                                            SChar *aName,
                                                            void  *aOldValue,
                                                            void  *aNewValue,
                                                            void  *aArg);
    static IDE_RC notifyREPLICATION_MAX_LOGFILE(idvSQL* /* aStatistics */,
                                                SChar *aName,
                                                void  *aOldValue,
                                                void  *aNewValue,
                                                void  *aArg);

    static IDE_RC notifyREPLICATION_PREFETCH_LOGFILE_COUNT(idvSQL* /* aStatistics */,
                                                           SChar *Name,
                                                           void  *aOldValue,
                                                           void  *aNewValue,
                                                           void  *aArg);

    static IDE_RC notifyREPLICATION_DDL_ENABLE(idvSQL* /* aStatistics */,
                                               SChar *Name,
                                               void  *aOldValue,
                                               void  *aNewValue,
                                               void  *aArg);

    static IDE_RC notifyREPLICATION_DDL_ENABLE_LEVEL( idvSQL* /* aStatistics */,
                                                      SChar *Name,
                                                      void  *aOldValue,
                                                      void  *aNewValue,
                                                      void  *aArg );

    // PROJ-1705
    static IDE_RC notifyREPLICATION_POOL_ELEMENT_SIZE(idvSQL* /* aStatistics */,
                                                      SChar *Name,
                                                      void  *aOldValue,
                                                      void  *aNewValue,
                                                      void  *aArg);
    // PROJ-1705
    static IDE_RC notifyREPLICATION_POOL_ELEMENT_COUNT(idvSQL* /* aStatistics */,
                                                       SChar *Name,
                                                       void  *aOldValue,
                                                       void  *aNewValue,
                                                       void  *aArg);

    static IDE_RC notifyREPLICATION_TX_VICTIM_TIMEOUT(idvSQL* /* aStatistics */,
                                                      SChar *Name,
                                                      void  *aOldValue,
                                                      void  *aNewValue,
                                                      void  *aArg);

    static IDE_RC notifyREPLICATION_COMMIT_WRITE_WAIT_MODE(idvSQL* /* aStatistics */,
                                                           SChar *Name,
                                                           void  *aOldValue,
                                                           void  *aNewValue,
                                                           void  *aArg);

    static IDE_RC notifyREPLICATION_FAILBACK_PK_QUEUE_TIMEOUT(idvSQL* /* aStatistics */,
                                                              SChar *Name,
                                                              void  *aOldValue,
                                                              void  *aNewValue,
                                                              void  *aArg);

    static IDE_RC notifyREPLICATION_SENDER_START_AFTER_GIVING_UP( idvSQL* /* aStatistics */,
                                                                  SChar * Name,
                                                                  void  * aOldValue,
                                                                  void  * aNewValue,
                                                                  void  * aArg );
    static IDE_RC notifyREPLICATION_TRANSACTION_POOL_SIZE( idvSQL* /* aStatistics */,
                                                           SChar * Name,
                                                           void  * aOldValue,
                                                           void  * aNewValue,
                                                           void  * aArg );
    static IDE_RC notifyREPLICATION_STRICT_EAGER_MODE( idvSQL* /* aStatistics */,
                                                       SChar * Name,
                                                       void  * aOldValue,
                                                       void  * aNewValue,
                                                       void  * aArg );
    static IDE_RC notifyREPLICATION_EAGER_MAX_YIELD_COUNT( idvSQL* /* aStatistics */,
                                                           SChar * Name,
                                                           void  * aOldValue,
                                                           void  * aNewValue,
                                                           void  * aArg );

    static IDE_RC notifyREPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT( idvSQL* /* aStatistics */,
                                                                    SChar * Name,
                                                                    void  * aOldValue,
                                                                    void  * aNewValue,
                                                                    void  * aArg );
	static IDE_RC notifyREPLICATION_BEFORE_IMAGE_LOG_ENABLE( idvSQL* /* aStatistics */,
                                                             SChar * Name, 
                                                             void  * aOldValue, 
                                                             void  * aNewValue, 
                                                             void  * aArg );

    static IDE_RC notifyREPLICATION_SENDER_COMPRESS_XLOG( idvSQL* /* aStatistics */,
                                                          SChar * Name,
                                                          void  * aOldValue,
                                                          void  * aNewValue,
                                                          void  * aArg );

    static IDE_RC notifyREPLICATION_SENDER_COMPRESS_XLOG_LEVEL( idvSQL* /* aStatistics */,
                                                                SChar * Name,
                                                                void  * aOldValue,
                                                                void  * aNewValue,
                                                                void  * aArg );

    static IDE_RC notifyREPLICATION_ALLOW_DUPLICATE_HOSTS( idvSQL* /* aStatistics */,
                                                           SChar * Name,
                                                           void  * aOldValue,
                                                           void  * aNewValue,
                                                           void  * aArg );

    /* BUG-38102 */
    static IDE_RC notifyREPLICATION_SENDER_ENCRYPT_XLOG( idvSQL* /* aStatistics */,
                                                         SChar * Name, 
                                                         void  * aOldValue, 
                                                         void  * aNewValue, 
                                                         void  * aArg );

    static IDE_RC notifyREPLICATION_SENDER_SEND_TIMEOUT( idvSQL* /* aStatistics */,
                                                         SChar * aName,
                                                         void  * aOldValue,
                                                         void  * aNewValue,
                                                         void  * aArg );

    static IDE_RC notifyREPLICATION_SENDER_SLEEP_TIME( idvSQL* /* aStatistics */,
                                                       SChar  * aName,
                                                       void   * aOldValue,
                                                       void   * aNewValue,
                                                       void   * aArg );

    static idBool isUseV6Protocol( void );

    static IDE_RC notifyREPLICATION_GAPLESS_MAX_WAIT_TIME( idvSQL* /* aStatistics */,
                                                           SChar * aName,
                                                           void  * aOldValue,
                                                           void  * aNewValue,
                                                           void  * aArg );

    static IDE_RC notifyREPLICATION_GAPLESS_ALLOW_TIME( idvSQL* /* aStatistics */,
                                                        SChar * aName,
                                                        void  * aOldValue,
                                                        void  * aNewValue,
                                                        void  * aArg );

    static IDE_RC notifyREPLICATION_REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE( idvSQL* /* aStatistics */,
                                                                             SChar * aName,
                                                                             void  * aOldValue,
                                                                             void  * aNewValue,
                                                                             void  * aArg );

    static IDE_RC notifyREPLICATION_REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE( idvSQL* /* aStatistics */,
                                                                              SChar * aName,
                                                                              void  * aOldValue,
                                                                              void  * aNewValue,
                                                                              void  * aArg );

    static IDE_RC notifyREPLICATION_FORCE_RECEIVER_PARALLEL_APPLY_COUNT( idvSQL* /* aStatistics */,
                                                                         SChar * aName,
                                                                         void  * aOldValue,
                                                                         void  * aNewValue,
                                                                         void  * aArg  );

    static IDE_RC notifyREPLICATION_GROUPING_TRANSACTION_MAX_COUNT( idvSQL* /* aStatistics */,
                                                                    SChar * aName,
                                                                    void  * aOldValue,
                                                                    void  * aNewValue,
                                                                    void  * aArg );

    static IDE_RC notifyREPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE( idvSQL* /* aStatistics */,
                                                                       SChar * aName,
                                                                       void  * aOldValue,
                                                                       void  * aNewValue,
                                                                       void  * aArg );

    static IDE_RC notifyREPLICATION_RECONNECT_MAX_COUNT( idvSQL* /* aStatistics */,
                                                         SChar  * aName,
                                                         void   * aOldValue,
                                                         void   * aNewValue,
                                                         void   * aArg );

    static IDE_RC notifyREPLICATION_SYNC_APPLY_METHOD( idvSQL* /* aStatistics */,
                                                       SChar * aName,
                                                       void  * aOldValue,
                                                       void  * aNewValue,
                                                       void  * aArg );
    static IDE_RC notifyREPLICATION_EAGER_UPDATED_RESTARTSN_GAP( idvSQL* /* aStatistics */,
                                                                 SChar * aName,
                                                                 void  * aOldValue,
                                                                 void  * aNewValue,
                                                                 void  * aArg );

    static IDE_RC notifyREPLICATION_EAGER_KEEP_LOGFILE_COUNT( idvSQL* /* aStatistics */,
                                                              SChar * aName,
                                                              void  * aOldValue,
                                                              void  * aNewValue,
                                                              void  * aArg );

    static IDE_RC notifyREPLICATION_FORCE_SQL_APPLY_ENABLE( idvSQL* /* aStatistics */,
                                                            SChar * /* aName */,
                                                            void  * /* aOldValue */,
                                                            void  * aNewValue,
                                                            void  * /* aArg */ );

    static IDE_RC notifyREPLICATION_SQL_APPLY_ENABLE( idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  * aNewValue,
                                                      void  * /* aArg */ );

    static IDE_RC notifyREPLICATION_SET_RESTARTSN( idvSQL* /* aStatistics */,
                                                   SChar * aName,
                                                   void  * aOldValue,
                                                   void  * aNewValue,
                                                   void  * aArg );
    
    static IDE_RC notifyREPLICATION_SENDER_RETRY_COUNT( idvSQL* /* aStatistics */,
                                                          SChar * aName,
                                                          void  * aOldValue,
                                                          void  * aNewValue,
                                                          void  * aArg );

    static IDE_RC notifyREPLICATION_ALLOW_QUEUE( idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */ );

    static inline void setReplicationDDLSync( UInt aValue )
    {
        mReplicationDDLSync = aValue;
    }

    static inline void setReplicationDDLSyncTimeout( UInt aValue )
    {        
        mReplicationDDLSyncTimeout = aValue;
    }

    static IDE_RC notifyREPLICATION_RECEIVER_APPLIER_YIELD_COUNT( idvSQL* /* aStatistics */,
                                                                  SChar * /* aName */,
                                                                  void  * /* aOldValue */,
                                                                  void  * aNewValue,
                                                                  void  * /* aArg */ );

    static IDE_RC notifyREPLICATION_GAP_UNIT( idvSQL* /* aStatistics */,
                                              SChar * /* aName */,
                                              void  * /* aOldValue */,
                                              void  * aNewValue,
                                              void  * /* aArg */ );
    
    static IDE_RC notifyREPLICATION_CHECK_SRID_IN_GEOMETRY_ENABLE( idvSQL* /* aStatistics */,
                                                                   SChar * /* aName */,
                                                                   void  * /* aOldValue */,
                                                                   void  * aNewValue,
                                                                   void  * /* aArg */ );

    static IDE_RC notifyREPLICATION_META_ITEM_COUNT_DIFF_ENABLE( idvSQL* /* aStatistics */,
                                                                 SChar * /* aName */,
                                                                 void  * /* aOldValue */,
                                                                 void  * aNewValue,
                                                                 void  * /* aArg */ );
                                                                   
    static IDE_RC notifyXLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE( idvSQL* /* aStatistics */,
                                                                 SChar * /* aName */,
                                                                 void  * /* aOldValue */,
                                                                 void  * aNewValue,
                                                                 void  * /* aArg */ );
};

#endif
