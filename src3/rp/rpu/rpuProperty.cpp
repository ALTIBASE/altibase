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
 * $Id: rpuProperty.cpp 90491 2021-04-07 07:02:29Z lswhh $
 **********************************************************************/

#include <idl.h> // to remove win32 compile warning
#include <rp.h>
#include <rpuProperty.h>
#include <sdi.h>

SChar *rpuProperty::mSID;
SChar *rpuProperty::mDbName;
UInt   rpuProperty::mPortNo;
UInt   rpuProperty::mConnectTimeout;
UInt   rpuProperty::mDetectTime;
UInt   rpuProperty::mDetectHighWaterMark;
UInt   rpuProperty::mIsolationLevel;
UInt   rpuProperty::mReceiveTimeout;
UInt   rpuProperty::mPropagation;
UInt   rpuProperty::mSenderSleepTimeout;
UInt   rpuProperty::mSyncLockTimeout;
ULong  rpuProperty::mSyncTupleCount;
UInt   rpuProperty::mUpdateReplace;
UInt   rpuProperty::mInsertReplace;
UInt   rpuProperty::mTrcHBTLog;
UInt   rpuProperty::mTrcConflictLog;
ULong  rpuProperty::mLogFileSize;
UInt   rpuProperty::mTimestampResolution;
UInt   rpuProperty::mConnectReceiveTimeout;
UInt   rpuProperty::mSenderAutoStart;
UInt   rpuProperty::mSenderStartAfterGivingUp;
UInt   rpuProperty::mServiceWaitMaxLimit;
UInt   rpuProperty::mPrefetchLogfileCount;
UInt   rpuProperty::mSenderSleepTime;
UInt   rpuProperty::mKeepAliveCnt;
SInt   rpuProperty::mMaxLogFile;
UInt   rpuProperty::mReceiverXLogQueueSz;
UInt   rpuProperty::mAckXLogCount;
UInt   rpuProperty::mSyncLog;
UInt   rpuProperty::mPerformanceTest;
UInt   rpuProperty::mRPLogBufferSize;
UInt   rpuProperty::mRPRecoveryMaxTime;
UInt   rpuProperty::mRecoveryMaxLogFile;
UInt   rpuProperty::mRecoveryRequestTimeout;
UInt   rpuProperty::mReplicationTXVictimTimeout;
UInt   rpuProperty::mDDLEnable;
UInt   rpuProperty::mDDLEnableLevel;
UInt   rpuProperty::mPoolElementSize;     // RPOJ-1705
UInt   rpuProperty::mPoolElementCount;    // RPOJ-1705
UInt   rpuProperty::mEagerParallelFactor;
UInt   rpuProperty::mCommitWriteWaitMode;
UInt   rpuProperty::mServerFailbackMaxTime;
UInt   rpuProperty::mFailbackPKQueueTimeout;
UInt   rpuProperty::mFailbackIncrementalSync;
UInt   rpuProperty::mMaxListen;
UInt   rpuProperty::mTransPoolSize;
UInt   rpuProperty::mStrictEagerMode;
SInt   rpuProperty::mEagerMaxYieldCount;
UInt   rpuProperty::mEagerReceiverMaxErrorCount;
UInt   rpuProperty::mBeforeImageLogEnable;  // BUG-36555
SInt   rpuProperty::mSenderCompressXLog;
UInt   rpuProperty::mSenderCompressXLogLevel;
UInt   rpuProperty::mMaxReplicationCount;   /* BUG-37482 */
UInt   rpuProperty::mAllowDuplicateHosts;
SInt   rpuProperty::mSenderEncryptXLog;     /* BUG-38102 */
UInt   rpuProperty::mSenderSendTimeout;
UInt   rpuProperty::mUseV6Protocol;
ULong  rpuProperty::mGaplessMaxWaitTime;
ULong  rpuProperty::mGaplessAllowTime;
UInt   rpuProperty::mReceiverApplierQueueSize;
SInt   rpuProperty::mReceiverApplierAssignMode;
UInt   rpuProperty::mForceReceiverApplierCount;
UInt   rpuProperty::mGroupingTransactionMaxCount;
UInt   rpuProperty::mGroupingAheadReadNextLogFile;
UInt   rpuProperty::mReconnectMaxCount;
UInt   rpuProperty::mSyncApplyMethod;
idBool rpuProperty::mIsCPUAffinity;
UInt   rpuProperty::mEagerKeepLogFileCount;
UInt   rpuProperty::mForceSqlApplyEnable;
UInt   rpuProperty::mSqlApplyEnable;
UInt   rpuProperty::mSetRestartSN;
UInt   rpuProperty::mSenderRetryCount;
UInt   rpuProperty::mAllowQueue;
UInt   rpuProperty::mReplicationDDLSync;
UInt   rpuProperty::mReplicationDDLSyncTimeout;
UInt   rpuProperty::mReceiverApplierYieldCount;

UInt   rpuProperty::mIBEnable;
UInt   rpuProperty::mIBPortNo;
UInt   rpuProperty::mIBLatency;

ULong  rpuProperty::mGapUnit;

UInt   rpuProperty::mCheckSRIDInGeometryEnable;
UInt   rpuProperty::mXLogfilePrepareCount;
ULong  rpuProperty::mXLogfileSize;
UInt   rpuProperty::mXLogfileRemoveInterval;
UInt   rpuProperty::mXLogfileRemoveIntervalByFileCreate;
SChar *rpuProperty::mXLogDirPath;

IDE_RC rpuProperty::initProperty()
{
    IDE_TEST( load() != IDE_SUCCESS );
    if (RPU_REPLICATION_PORT_NO != 0)
    {
        /*******************************************************************
         * 프라퍼티 사이의 충돌을 검사한다.
         *******************************************************************/
        IDE_TEST(checkConflict() != IDE_SUCCESS);
    }

    /* 하위 모듈에게 이중화 관련 정보를 사용할 때 참고하기 위한 이중화 속성을 설정해 준다. */
    setLowerModuleStaticProperty();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpuProperty::finalProperty()
{
    return IDE_SUCCESS;
}

UInt   rpuProperty::mMetaItemCountDiffEnable;

IDE_RC
rpuProperty::load()
{
    /*******************************************************************
     * 프라퍼티를 읽는다.
     *******************************************************************/
    IDE_ASSERT(idp::readPtr((SChar *)"SID",
                            (void **)&mSID)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::readPtr((SChar *)"DB_NAME",
                            (void **)&mDbName)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_PORT_NO",
                         &mPortNo)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_CONNECT_TIMEOUT",
                         &mConnectTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_HBT_DETECT_TIME",
                         &mDetectTime )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_HBT_DETECT_HIGHWATER_MARK",
                         &mDetectHighWaterMark )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"ISOLATION_LEVEL",
                         &mIsolationLevel)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_RECEIVE_TIMEOUT",
                         &mReceiveTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_PROPAGATION",
                         &mPropagation)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SENDER_SLEEP_TIMEOUT",
                         &mSenderSleepTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"LOG_FILE_SIZE",
                         &mLogFileSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SYNC_LOCK_TIMEOUT",
                         &mSyncLockTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SYNC_TUPLE_COUNT",
                         &mSyncTupleCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_UPDATE_REPLACE",
                         &mUpdateReplace)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_INSERT_REPLACE",
                         &mInsertReplace)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_TIMESTAMP_RESOLUTION",
                         &mTimestampResolution)
               == IDE_SUCCESS);

    //fix BUG-9343
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SENDER_AUTO_START",
                         &mSenderAutoStart)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_SENDER_START_AFTER_GIVING_UP",
                           &mSenderStartAfterGivingUp )
                == IDE_SUCCESS );

    //fix BUG-9894
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_CONNECT_RECEIVE_TIMEOUT",
                         &mConnectReceiveTimeout)
               == IDE_SUCCESS);
    
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SERVICE_WAIT_MAX_LIMIT",
                         &mServiceWaitMaxLimit)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_PREFETCH_LOGFILE_COUNT",
                         &mPrefetchLogfileCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SENDER_SLEEP_TIME",
                         &mSenderSleepTime)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_KEEP_ALIVE_CNT",
                         &mKeepAliveCnt)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_RECEIVER_XLOG_QUEUE_SIZE",
                         &mReceiverXLogQueueSz)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_ACK_XLOG_COUNT",
                         &mAckXLogCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_MAX_LOGFILE",
                         &mMaxLogFile)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SYNC_LOG",
                         &mSyncLog)
               == IDE_SUCCESS);
    // TASK-2359
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_PERFORMANCE_TEST",
                         &mPerformanceTest)
               == IDE_SUCCESS);
    /*PROJ-1670 replication Log Buffer*/
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_LOG_BUFFER_SIZE",
                         &mRPLogBufferSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_RECOVERY_MAX_TIME",
                         &mRPRecoveryMaxTime)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_RECOVERY_MAX_LOGFILE",
                         &mRecoveryMaxLogFile)
               == IDE_SUCCESS);
    IDE_ASSERT(idp::read((SChar *)"__REPLICATION_RECOVERY_REQUEST_TIMEOUT",
                         &mRecoveryRequestTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"__REPLICATION_TX_VICTIM_TIMEOUT",
                         &mReplicationTXVictimTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_DDL_ENABLE",
                         &mDDLEnable)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_DDL_ENABLE_LEVEL",
                           &mDDLEnableLevel )
                == IDE_SUCCESS );

    // PROJ-1705
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_POOL_ELEMENT_SIZE",
                         &mPoolElementSize)
               == IDE_SUCCESS);
    // PROJ-1705
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_POOL_ELEMENT_COUNT",
                         &mPoolElementCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_EAGER_PARALLEL_FACTOR",
                         &mEagerParallelFactor)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_COMMIT_WRITE_WAIT_MODE",
                         &mCommitWriteWaitMode)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SERVER_FAILBACK_MAX_TIME",
                         &mServerFailbackMaxTime)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"__REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT",
                         &mFailbackPKQueueTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_FAILBACK_INCREMENTAL_SYNC",
                         &mFailbackIncrementalSync)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_MAX_LISTEN",
                         &mMaxListen)
               == IDE_SUCCESS);
    
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_STRICT_EAGER_MODE",
                         &mStrictEagerMode)
               == IDE_SUCCESS);
    
    IDE_ASSERT(idp::read((SChar *)"REPLICATION_EAGER_MAX_YIELD_COUNT",
                         &mEagerMaxYieldCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT",
                         &mEagerReceiverMaxErrorCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_BEFORE_IMAGE_LOG_ENABLE",
                         &mBeforeImageLogEnable)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SENDER_COMPRESS_XLOG",
                         &mSenderCompressXLog)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SENDER_COMPRESS_XLOG_LEVEL",
                         &mSenderCompressXLogLevel)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_MAX_COUNT",
                         &mMaxReplicationCount )
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_ALLOW_DUPLICATE_HOSTS",
                         &mAllowDuplicateHosts)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SENDER_ENCRYPT_XLOG",
                         &mSenderEncryptXLog)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SENDER_SEND_TIMEOUT",
                         &mSenderSendTimeout)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"__REPLICATION_USE_V6_PROTOCOL",
                           &mUseV6Protocol )
                == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_GAPLESS_MAX_WAIT_TIME",
                         &mGaplessMaxWaitTime)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_GAPLESS_ALLOW_TIME",
                         &mGaplessAllowTime)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar*)"REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE",
                         &mReceiverApplierQueueSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar*)"REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE",
                         &mReceiverApplierAssignMode)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar*)"REPLICATION_FORCE_RECEIVER_PARALLEL_APPLY_COUNT",
                         &mForceReceiverApplierCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar*)"REPLICATION_GROUPING_TRANSACTION_MAX_COUNT",
                         &mGroupingTransactionMaxCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar*)"REPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE",
                         &mGroupingAheadReadNextLogFile)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_RECONNECT_MAX_COUNT",
                         &mReconnectMaxCount)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_TRANSACTION_POOL_SIZE",
                           &mTransPoolSize )
                == IDE_SUCCESS );

    IDE_ASSERT(idp::read((SChar *)"REPLICATION_SYNC_APPLY_METHOD",
                         &mSyncApplyMethod)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_EAGER_KEEP_LOGFILE_COUNT",
                           &mEagerKeepLogFileCount )
                == IDE_SUCCESS );
    
    /* BUG-43393 */
    IDE_ASSERT( idp::read( (SChar *)"THREAD_CPU_AFFINITY",
                           &mIsCPUAffinity ) 
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_FORCE_SQL_APPLY_ENABLE",
                           &mForceSqlApplyEnable )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_SQL_APPLY_ENABLE",
                           &mSqlApplyEnable )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"__REPLICATION_SET_RESTARTSN",
                           &mSetRestartSN )
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_SENDER_RETRY_COUNT",
                           &mSenderRetryCount )
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_ALLOW_QUEUE",
                           &mAllowQueue )
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_DDL_SYNC", 
                           &mReplicationDDLSync ) 
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_DDL_SYNC_TIMEOUT",
                           &mReplicationDDLSyncTimeout ) 
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_RECEIVER_APPLIER_YIELD_COUNT",
                           &mReceiverApplierYieldCount )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( (SChar *)"IB_ENABLE", 
                           &mIBEnable ) 
                == IDE_SUCCESS );
    
    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_IB_PORT_NO",
                           &mIBPortNo )
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read( (SChar *)"REPLICATION_IB_LATENCY",
                           &mIBLatency )
                == IDE_SUCCESS);

    IDE_ASSERT(idp::read( (SChar *)"REPLICATION_GAP_UNIT",
                          &mGapUnit )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( (SChar *)"REPLICATION_CHECK_SRID_IN_GEOMETRY_ENABLE",
                          &mCheckSRIDInGeometryEnable )
               == IDE_SUCCESS);
               
    IDE_ASSERT(idp::read( (SChar *)"REPLICATION_META_ITEM_COUNT_DIFF_ENABLE",
                          &mMetaItemCountDiffEnable )
               == IDE_SUCCESS);    
    
    IDE_ASSERT(idp::read( (SChar *)"XLOGFILE_PREPARE_COUNT",
                          &mXLogfilePrepareCount )
               == IDE_SUCCESS);
 
    IDE_ASSERT(idp::read( (SChar *)"XLOGFILE_SIZE",
                          &mXLogfileSize )
               == IDE_SUCCESS);
 
    IDE_ASSERT(idp::read( (SChar *)"XLOGFILE_REMOVE_INTERVAL",
                          &mXLogfileRemoveInterval )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( (SChar *)"XLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE",
                          &mXLogfileRemoveIntervalByFileCreate )
               == IDE_SUCCESS);
    IDE_TEST( idp::readPtr( "XLOGFILE_DIR", (void**)&mXLogDirPath ) != IDE_SUCCESS );
    /*******************************************************************
     * Update Callback을 설정한다.
     *******************************************************************/
    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SYNC_LOCK_TIMEOUT",
                  rpuProperty::notifyREPLICATION_SYNC_LOCK_TIMEOUT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_TIMESTAMP_RESOLUTION",
                  rpuProperty::notifyREPLICATION_TIMESTAMP_RESOLUTION)
             != IDE_SUCCESS);

    //fix BUG-9788
    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_UPDATE_REPLACE",
                  rpuProperty::notifyREPLICATION_UPDATE_REPLACE)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_INSERT_REPLACE",
                  rpuProperty::notifyREPLICATION_INSERT_REPLACE)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_CONNECT_TIMEOUT",
                  rpuProperty::notifyREPLICATION_CONNECT_TIMEOUT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_RECEIVE_TIMEOUT",
                  rpuProperty::notifyREPLICATION_RECEIVE_TIMEOUT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SENDER_SLEEP_TIMEOUT",
                  rpuProperty::notifyREPLICATION_SENDER_SLEEP_TIMEOUT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_HBT_DETECT_TIME",
                  rpuProperty::notifyREPLICATION_HBT_DETECT_TIME)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_HBT_DETECT_HIGHWATER_MARK",
                  rpuProperty::notifyREPLICATION_HBT_DETECT_HIGHWATER_MARK)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SYNC_TUPLE_COUNT",
                  rpuProperty::notifyREPLICATION_SYNC_TUPLE_COUNT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_CONNECT_RECEIVE_TIMEOUT",
                  rpuProperty::notifyREPLICATION_CONNECT_RECEIVE_TIMEOUT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_MAX_LOGFILE",
                  rpuProperty::notifyREPLICATION_MAX_LOGFILE)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_PREFETCH_LOGFILE_COUNT",
                  rpuProperty::notifyREPLICATION_PREFETCH_LOGFILE_COUNT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_DDL_ENABLE",
                  rpuProperty::notifyREPLICATION_DDL_ENABLE)
             != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback( 
                    (const SChar*)"REPLICATION_DDL_ENABLE_LEVEL",
                    rpuProperty::notifyREPLICATION_DDL_ENABLE_LEVEL )
              != IDE_SUCCESS );

    // PROJ-1705
    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_POOL_ELEMENT_SIZE",
                  rpuProperty::notifyREPLICATION_POOL_ELEMENT_SIZE)
             != IDE_SUCCESS);
    // PROJ-1705
    IDE_TEST(idp::setupAfterUpdateCallback(
                 (const SChar*)"REPLICATION_POOL_ELEMENT_COUNT",
                 rpuProperty::notifyREPLICATION_POOL_ELEMENT_COUNT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"__REPLICATION_TX_VICTIM_TIMEOUT",
                  rpuProperty::notifyREPLICATION_TX_VICTIM_TIMEOUT)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_COMMIT_WRITE_WAIT_MODE",
                  rpuProperty::notifyREPLICATION_COMMIT_WRITE_WAIT_MODE)
             != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback(
                  (const SChar*)"__REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT",
                  rpuProperty::notifyREPLICATION_FAILBACK_PK_QUEUE_TIMEOUT)
             != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SENDER_START_AFTER_GIVING_UP",
                  rpuProperty::notifyREPLICATION_SENDER_START_AFTER_GIVING_UP )
              != IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_TRANSACTION_POOL_SIZE",
                  rpuProperty::notifyREPLICATION_TRANSACTION_POOL_SIZE)
              != IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_STRICT_EAGER_MODE",
                  rpuProperty::notifyREPLICATION_STRICT_EAGER_MODE)
              != IDE_SUCCESS );
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_EAGER_MAX_YIELD_COUNT",
                  rpuProperty::notifyREPLICATION_EAGER_MAX_YIELD_COUNT)
              != IDE_SUCCESS );
    
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_BEFORE_IMAGE_LOG_ENABLE",
                  rpuProperty::notifyREPLICATION_BEFORE_IMAGE_LOG_ENABLE)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                   (const SChar*)"REPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT",
                   rpuProperty::notifyREPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SENDER_COMPRESS_XLOG",
                  rpuProperty::notifyREPLICATION_SENDER_COMPRESS_XLOG)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SENDER_COMPRESS_XLOG_LEVEL",
                  rpuProperty::notifyREPLICATION_SENDER_COMPRESS_XLOG_LEVEL)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_ALLOW_DUPLICATE_HOSTS",
                  rpuProperty::notifyREPLICATION_ALLOW_DUPLICATE_HOSTS)
              != IDE_SUCCESS );

    /* BUG-38102 */
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SENDER_ENCRYPT_XLOG",
                  rpuProperty::notifyREPLICATION_SENDER_ENCRYPT_XLOG)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( 
                  (const SChar*)"REPLICATION_SENDER_SEND_TIMEOUT",
                  rpuProperty::notifyREPLICATION_SENDER_SEND_TIMEOUT)
              != IDE_SUCCESS);

    /* BUG-40301 */
    IDE_TEST( idp::setupAfterUpdateCallback( 
                  (const SChar*)"REPLICATION_SENDER_SLEEP_TIME",
                  rpuProperty::notifyREPLICATION_SENDER_SLEEP_TIME)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback( 
                  (const SChar*)"REPLICATION_GAPLESS_MAX_WAIT_TIME",
                  rpuProperty::notifyREPLICATION_GAPLESS_MAX_WAIT_TIME)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback( 
                  (const SChar*)"REPLICATION_GAPLESS_ALLOW_TIME",
                  rpuProperty::notifyREPLICATION_GAPLESS_ALLOW_TIME)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback( 
                  (const SChar*)"REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE",
                  rpuProperty::notifyREPLICATION_REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE",
                  rpuProperty::notifyREPLICATION_REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback(
                    (const SChar*)"REPLICATION_FORCE_RECEIVER_PARALLEL_APPLY_COUNT",
                    rpuProperty::notifyREPLICATION_FORCE_RECEIVER_PARALLEL_APPLY_COUNT)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                    (const SChar*)"REPLICATION_GROUPING_TRANSACTION_MAX_COUNT",
                    rpuProperty::notifyREPLICATION_GROUPING_TRANSACTION_MAX_COUNT)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                    (const SChar*)"REPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE",
                    rpuProperty::notifyREPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE)
              != IDE_SUCCESS );

    /* BUG-41246 */
    IDE_TEST( idp::setupAfterUpdateCallback( 
                  (const SChar*)"REPLICATION_RECONNECT_MAX_COUNT",
                  rpuProperty::notifyREPLICATION_RECONNECT_MAX_COUNT)
              != IDE_SUCCESS);
    /* BUG-42563 */
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SYNC_APPLY_METHOD",
                  rpuProperty::notifyREPLICATION_SYNC_APPLY_METHOD)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_EAGER_KEEP_LOGFILE_COUNT",
                  rpuProperty::notifyREPLICATION_EAGER_KEEP_LOGFILE_COUNT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                    (const SChar*)"REPLICATION_FORCE_SQL_APPLY_ENABLE",
                    rpuProperty::notifyREPLICATION_FORCE_SQL_APPLY_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                    (const SChar*)"REPLICATION_SQL_APPLY_ENABLE",
                    rpuProperty::notifyREPLICATION_SQL_APPLY_ENABLE )
              != IDE_SUCCESS );
    
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"__REPLICATION_SET_RESTARTSN",
                  rpuProperty::notifyREPLICATION_SET_RESTARTSN )
              != IDE_SUCCESS);
 
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_SENDER_RETRY_COUNT",
                  rpuProperty::notifyREPLICATION_SENDER_RETRY_COUNT )
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_ALLOW_QUEUE",
                  rpuProperty::notifyREPLICATION_ALLOW_QUEUE )
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_RECEIVER_APPLIER_YIELD_COUNT",
                  rpuProperty::notifyREPLICATION_RECEIVER_APPLIER_YIELD_COUNT )
              != IDE_SUCCESS ); 

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_GAP_UNIT",
                  rpuProperty::notifyREPLICATION_GAP_UNIT )
              != IDE_SUCCESS ); 

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_CHECK_SRID_IN_GEOMETRY_ENABLE",
                  rpuProperty::notifyREPLICATION_CHECK_SRID_IN_GEOMETRY_ENABLE )
              != IDE_SUCCESS ); 

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"REPLICATION_META_ITEM_COUNT_DIFF_ENABLE",
                  rpuProperty::notifyREPLICATION_META_ITEM_COUNT_DIFF_ENABLE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar*)"XLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE",
                  rpuProperty::notifyXLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE )
              != IDE_SUCCESS ); 
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 프라퍼티 사이의 충돌을 검사한다.
 *
 ***********************************************************************/
IDE_RC rpuProperty::checkConflict()
{
    /* BUG-30694 RP Log Buffer에서 LSN을 얻을 수 없으므로, 관련 프라퍼티 막음 */
    IDE_TEST_RAISE((mRPLogBufferSize > 0) && (mSyncLog == 1),
                   ERR_RP_LOG_BUFFER_PROPERTY_CONFLICT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_RP_LOG_BUFFER_PROPERTY_CONFLICT);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_LOG_BUFFER_PROPERTY_CONFLICT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpuProperty::setLowerModuleStaticProperty()
{
    /* 하위 모듈에서 사용하는 이중화 관련 속성 설정 */
    sdi::setReplicationMaxParallelCount(RP_PARALLEL_APPLIER_MAX_COUNT);
}

IDE_RC
rpuProperty::notifyREPLICATION_SYNC_LOCK_TIMEOUT( idvSQL* /* aStatistics */,
                                                  SChar * /* aName */,
                                                  void  * /* aOldValue */,
                                                  void  * aNewValue,
                                                  void  * /* aArg */)
{
    idlOS::memcpy( &mSyncLockTimeout,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_TIMESTAMP_RESOLUTION( idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */)
{
    idlOS::memcpy( &mTimestampResolution,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

//fix BUG-9788
IDE_RC 
rpuProperty::notifyREPLICATION_UPDATE_REPLACE( idvSQL* /* aStatistics */,
                                               SChar * /*aName*/,
                                               void  * /*aOldValue*/,
                                               void  * aNewValue,
                                               void  * /*aArg*/)
{
    idlOS::memcpy( &mUpdateReplace,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_INSERT_REPLACE(idvSQL* /* aStatistics */,
                                              SChar * /*aName*/,
                                              void  * /*aOldValue*/,
                                              void  * aNewValue,
                                              void  * /*aArg*/)
{
    idlOS::memcpy(&mInsertReplace,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_CONNECT_TIMEOUT( idvSQL* /* aStatistics */,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  * aNewValue,
                                                void  * /*aArg*/)
{
    idlOS::memcpy( &mConnectTimeout,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_RECEIVE_TIMEOUT( idvSQL* /* aStatistics */,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  * aNewValue,
                                                void  * /*aArg*/)
{
    idlOS::memcpy( &mReceiveTimeout,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_SENDER_SLEEP_TIMEOUT( idvSQL* /* aStatistics */,
                                                     SChar * /*aName*/,
                                                     void  * /*aOldValue*/,
                                                     void  * aNewValue,
                                                     void  * /*aArg*/)
{
    idlOS::memcpy( &mSenderSleepTimeout,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_HBT_DETECT_TIME( idvSQL* /* aStatistics */,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  * aNewValue,
                                                void  * /*aArg*/)
{
    idlOS::memcpy( &mDetectTime,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_HBT_DETECT_HIGHWATER_MARK( idvSQL* /* aStatistics */,
                                                          SChar * /*aName*/,
                                                          void  * /*aOldValue*/,
                                                          void  * aNewValue,
                                                          void  * /*aArg*/)
{
    idlOS::memcpy( &mDetectHighWaterMark,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_SYNC_TUPLE_COUNT( idvSQL* /* aStatistics */,
                                                 SChar * /*aName*/,
                                                 void  * /*aOldValue*/,
                                                 void  * aNewValue,
                                                 void  * /*aArg*/)
{
    idlOS::memcpy( &mSyncTupleCount,
                   aNewValue,
                   ID_SIZEOF(ULong) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_CONNECT_RECEIVE_TIMEOUT( idvSQL* /* aStatistics */,
                                                        SChar * /*aName*/,
                                                        void  * /*aOldValue*/,
                                                        void  * aNewValue,
                                                        void  * /*aArg*/)
{
    idlOS::memcpy( &mConnectReceiveTimeout,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_MAX_LOGFILE(idvSQL* /* aStatistics */,
                                           SChar * /* aName */,
                                           void  * /* aOldValue */,
                                           void  * aNewValue,
                                           void  * /* aArg */)
{
    idlOS::memcpy(&mMaxLogFile,
                  aNewValue,
                  ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_PREFETCH_LOGFILE_COUNT(idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  *aNewValue,
                                                      void  * /* aArg */)
{
    idlOS::memcpy(&mPrefetchLogfileCount,
                  aNewValue,
                  ID_SIZEOF(SInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_DDL_ENABLE(idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */)
{
    idlOS::memcpy(&mDDLEnable,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_DDL_ENABLE_LEVEL( idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */ )
{
    idlOS::memcpy( &mDDLEnableLevel,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

// PROJ-1705
IDE_RC
rpuProperty::notifyREPLICATION_POOL_ELEMENT_SIZE(idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */)
{
    idlOS::memcpy(&mPoolElementSize,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}
// PROJ-1705
IDE_RC
rpuProperty::notifyREPLICATION_POOL_ELEMENT_COUNT(idvSQL* /* aStatistics */,
                                                  SChar * /* aName */,
                                                  void  * /* aOldValue */,
                                                  void  * aNewValue,
                                                  void  * /* aArg */)
{
    idlOS::memcpy(&mPoolElementCount,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_TX_VICTIM_TIMEOUT(idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */)
{
    idlOS::memcpy(&mReplicationTXVictimTimeout,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_COMMIT_WRITE_WAIT_MODE(idvSQL* /* aStatistics */,
                                                      SChar * /*aName*/,
                                                      void  * /*aOldValue*/,
                                                      void  * aNewValue,
                                                      void  * /*aArg*/)
{
    idlOS::memcpy(&mCommitWriteWaitMode,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_FAILBACK_PK_QUEUE_TIMEOUT(idvSQL* /* aStatistics */,
                                                         SChar * /*aName*/,
                                                         void  * /*aOldValue*/,
                                                         void  * aNewValue,
                                                         void  * /*aArg*/)
{
    idlOS::memcpy(&mFailbackPKQueueTimeout,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_SENDER_START_AFTER_GIVING_UP( idvSQL* /* aStatistics */,
                                                             SChar * /*aName*/,
                                                             void  * /*aOldValue*/,
                                                             void  * aNewValue,
                                                             void  * /*aArg*/ )
{
    idlOS::memcpy( &mSenderStartAfterGivingUp,
                   aNewValue,
                   ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_TRANSACTION_POOL_SIZE(idvSQL* /* aStatistics */,
                                                     SChar * /*aName*/,
                                                     void  * /*aOldValue*/,
                                                     void  * aNewValue,
                                                     void  * /*aArg*/)
{
    idlOS::memcpy(&mTransPoolSize,
                  aNewValue,
                  ID_SIZEOF(UInt));

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_STRICT_EAGER_MODE(idvSQL* /* aStatistics */,
                                                 SChar * /*aName*/,
                                                 void  * /*aOldValue*/,
                                                 void  * aNewValue,
                                                 void  * /*aArg*/)
{
    idlOS::memcpy(&mStrictEagerMode,
                  aNewValue,
                  ID_SIZEOF(UInt));
    
    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_EAGER_MAX_YIELD_COUNT(idvSQL* /* aStatistics */,
                                                     SChar * /*aName*/,
                                                     void  * /*aOldValue*/,
                                                     void  * aNewValue,
                                                     void  * /*aArg*/)
{
    idlOS::memcpy(&mEagerMaxYieldCount,
                  aNewValue,
                  ID_SIZEOF(SInt));
    
    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_BEFORE_IMAGE_LOG_ENABLE(idvSQL* /* aStatistics */,
                                                       SChar * /*aName*/, 
                                                       void  * /*aOldValue*/, 
                                                       void  * aNewValue, 
                                                       void  * /*aArg*/)
{
    idlOS::memcpy(&mBeforeImageLogEnable,
                  aNewValue,
                  ID_SIZEOF(UInt));
    
    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT(idvSQL* /* aStatistics */,
                                                              SChar * /*aName*/,
                                                              void  * /*aOldValue*/,
                                                              void  * aNewValue,
                                                              void  * /*aArg*/)
{
    idlOS::memcpy(&mEagerReceiverMaxErrorCount,
                  aNewValue,                                              
                  ID_SIZEOF(UInt));                                       
                                                                          
    return IDE_SUCCESS;                                                   
}  

IDE_RC
rpuProperty::notifyREPLICATION_SENDER_COMPRESS_XLOG( idvSQL* /* aStatistics */,
                                                     SChar * /* Name */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */ )
{
    idlOS::memcpy( &mSenderCompressXLog,
                   aNewValue,
                   ID_SIZEOF( SInt ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_SENDER_COMPRESS_XLOG_LEVEL( idvSQL* /* aStatistics */,
                                                           SChar * /* Name */,
                                                           void  * /* aOldValue */,
                                                           void  * aNewValue,
                                                           void  * /* aArg */ )
{
    idlOS::memcpy( &mSenderCompressXLogLevel,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC rpuProperty::notifyREPLICATION_ALLOW_DUPLICATE_HOSTS( idvSQL* /* aStatistics */,
                                                             SChar * /* Name */,
                                                             void  * /* aOldValue */,
                                                             void  * aNewValue,
                                                             void  * /* aArg */ )
{
    idlOS::memcpy( &mAllowDuplicateHosts,
                   aNewValue,
                   ID_SIZEOF( SInt ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_SENDER_ENCRYPT_XLOG( idvSQL* /* aStatistics */,
                                                    SChar * /* Name */, 
                                                    void  * /* aOldValue */, 
                                                    void  * aNewValue, 
                                                    void  * /* aArg */ )
{
    idlOS::memcpy( &mSenderEncryptXLog,
                   aNewValue,
                   ID_SIZEOF( SInt ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_SENDER_SEND_TIMEOUT( idvSQL* /* aStatistics */,
                                                    SChar * /* aName */,
                                                    void  * /* aOldValue */,
                                                    void  * aNewValue,
                                                    void  * /* aArg */)
{
    idlOS::memcpy( &mSenderSendTimeout,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_SENDER_SLEEP_TIME( idvSQL* /* aStatistics */,
                                                  SChar * /* aName */,
                                                  void  * /* aOldValue */,
                                                  void  * aNewValue,
                                                  void  * /* aArg */ )
{
    idlOS::memcpy( &mSenderSleepTime,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

idBool rpuProperty::isUseV6Protocol( void )
{
    idBool sRetValue = ID_FALSE;

    if( mUseV6Protocol == 1 )
    {
        sRetValue = ID_TRUE;
    }
    else
    {
        sRetValue = ID_FALSE;
    }

    return sRetValue;
}

IDE_RC rpuProperty::notifyREPLICATION_GAPLESS_MAX_WAIT_TIME( idvSQL* /* aStatistics */,
                                                             SChar * /* aName */,
                                                             void  * /* aOldValue */,
                                                             void  * aNewValue,
                                                             void  * /* aArg */ )
{
    idlOS::memcpy( &mGaplessMaxWaitTime,
                   aNewValue,
                   ID_SIZEOF( ULong ) );

    return IDE_SUCCESS;
}

IDE_RC rpuProperty::notifyREPLICATION_GAPLESS_ALLOW_TIME( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */ )
{
    idlOS::memcpy( &mGaplessAllowTime,
                   aNewValue,
                   ID_SIZEOF( ULong ) );

    return IDE_SUCCESS;
}

IDE_RC rpuProperty::notifyREPLICATION_REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE( idvSQL* /* aStatistics */,
                                                                               SChar * /* aName */,
                                                                               void  * /* aOldValue */,
                                                                               void  * aNewValue,
                                                                               void  * /* aArg */ )
{
    idlOS::memcpy( &mReceiverApplierQueueSize,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}
IDE_RC rpuProperty::notifyREPLICATION_REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE( idvSQL* /* aStatistics */,
                                                                                SChar * /* aName */,
                                                                                void  * /* aOldValue */,
                                                                                void  * aNewValue,
                                                                                void  * /* aArg */ )
{
    idlOS::memcpy( &mReceiverApplierAssignMode,
                   aNewValue,
                   ID_SIZEOF( SInt ) );

    return IDE_SUCCESS;
}

IDE_RC rpuProperty::notifyREPLICATION_FORCE_RECEIVER_PARALLEL_APPLY_COUNT( idvSQL* /* aStatistics */,
                                                                           SChar * /* aName */,
                                                                           void  * /* aOldValue */,
                                                                           void  * aNewValue,
                                                                           void  * /* aArg */ )
{
    idlOS::memcpy( &mForceReceiverApplierCount,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC rpuProperty::notifyREPLICATION_GROUPING_TRANSACTION_MAX_COUNT( idvSQL* /* aStatistics */,
                                                                      SChar * /* aName */,
                                                                      void  * /* aOldValue */,
                                                                      void  * aNewValue,
                                                                      void  * /* aArg */ )
{
    idlOS::memcpy( &mGroupingTransactionMaxCount,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC rpuProperty::notifyREPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE( idvSQL* /* aStatistics */,
                                                                         SChar * /* aName */,
                                                                         void  * /* aOldValue */,
                                                                         void  * aNewValue,
                                                                         void  * /* aArg */ )
{
    idlOS::memcpy( &mGroupingAheadReadNextLogFile,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_RECONNECT_MAX_COUNT( idvSQL* /* aStatistics */,
                                                    SChar * /* aName */,
                                                    void  * /* aOldValue */,
                                                    void  * aNewValue,
                                                    void  * /* aArg */ )
{
    idlOS::memcpy( &mReconnectMaxCount,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_SYNC_APPLY_METHOD( idvSQL* /* aStatistics */,
                                                  SChar * /* aName */,
                                                  void  * /* aOldValue */,
                                                  void  * aNewValue,
                                                  void  * /* aArg */ )
{
    idlOS::memcpy( &mSyncApplyMethod,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_EAGER_KEEP_LOGFILE_COUNT( idvSQL* /* aStatistics */,
                                                         SChar * /* aName */,
                                                         void  * /* aOldValue */,
                                                         void  * aNewValue,
                                                         void  * /* aArg */ )
{
    idlOS::memcpy( &mEagerKeepLogFileCount,
                   aNewValue,
                   ID_SIZEOF( UInt ) );
    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_FORCE_SQL_APPLY_ENABLE( idvSQL* /* aStatistics */,
                                                       SChar * /* aName */,
                                                       void  * /* aOldValue */,
                                                       void  * aNewValue,
                                                       void  * /* aArg */ )
{
    idlOS::memcpy( &mForceSqlApplyEnable,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC rpuProperty::notifyREPLICATION_SQL_APPLY_ENABLE( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */ )
{
    idlOS::memcpy( &mSqlApplyEnable,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_SET_RESTARTSN( idvSQL* /* aStatistics */,
                                              SChar * /* aName */,
                                              void  * /* aOldValue */,
                                              void  * aNewValue,
                                              void  * /* aArg */ )
{
    idlOS::memcpy( &mSetRestartSN,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}
    
IDE_RC 
rpuProperty::notifyREPLICATION_SENDER_RETRY_COUNT( idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */ )
{
    idlOS::memcpy( &mSenderRetryCount,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_ALLOW_QUEUE( idvSQL* /* aStatistics */,
                                            SChar * /* aName */,
                                            void  * /* aOldValue */,
                                            void  * aNewValue,
                                            void  * /* aArg */ )
{
    idlOS::memcpy( &mAllowQueue,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_RECEIVER_APPLIER_YIELD_COUNT( idvSQL* /* aStatistics */,
                                                             SChar * /* aName */,
                                                             void  * /* aOldValue */,
                                                             void  * aNewValue,
                                                             void  * /* aArg */ )
{
    idlOS::memcpy( &mReceiverApplierYieldCount,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC 
rpuProperty::notifyREPLICATION_GAP_UNIT( idvSQL* /* aStatistics */,
                                         SChar * /* aName */,
                                         void  * /* aOldValue */,
                                         void  * aNewValue,
                                         void  * /* aArg */ )
{
    idlOS::memcpy( &mGapUnit,
                   aNewValue,
                   ID_SIZEOF( ULong ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_CHECK_SRID_IN_GEOMETRY_ENABLE( idvSQL* /* aStatistics */,
                                                              SChar * /* aName */,
                                                              void  * /* aOldValue */,
                                                              void  * aNewValue,
                                                              void  * /* aArg */ )
{
    idlOS::memcpy( &mCheckSRIDInGeometryEnable,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyREPLICATION_META_ITEM_COUNT_DIFF_ENABLE( idvSQL* /* aStatistics */,
                                                            SChar * /* aName */,
                                                            void  * /* aOldValue */,
                                                            void  * aNewValue,
                                                            void  * /* aArg */ )
{
    idlOS::memcpy( &mMetaItemCountDiffEnable,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC
rpuProperty::notifyXLOGFILE_REMOVE_INTERVAL_BY_FILE_CREATE( idvSQL* /* aStatistics */,
                                                            SChar * /* aName */,
                                                            void  * /* aOldValue */,
                                                            void  * aNewValue,
                                                            void  * /* aArg */ )
{
    idlOS::memcpy( &mXLogfileRemoveIntervalByFileCreate,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

