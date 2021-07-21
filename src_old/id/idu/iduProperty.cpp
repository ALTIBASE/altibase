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
 
/*****************************************************************************
 * $Id:
 ****************************************************************************/
#include <idl.h>
#include <idp.h>
#include <ide.h>
#include <iduProperty.h>
#include <idvProfile.h>
#include <idvAudit.h>
#include <iduMutexMgr.h>

iduProperty::iduPropertyStore *iduProperty::mProperties = &iduProperty::mStaticProperties;
iduProperty::iduPropertyStore  iduProperty::mStaticProperties =
{
    ID_SCALABILITY_CLIENT_CPU,  // mScalabilityPerCPU
    ID_SCALABILITY_CLIENT_MAX,  // mMaxScalability
    IDL_NET_CONN_IP_STACK_V4,   // mNetConnIpStack
    IDL_NET_CONN_IP_STACK_V4,   // mRpNetConnIpStack
    0,                          // mErrorValidationLevel
    1,                          // mWriteErrorTrace
    0,                          // mWritePstack
    1,                          // mUseSigAltStack
    10,                         // mLogCollectCount
    1,                          // mCollectDumpInfo
    1,                          // mWriteWindowsMinidump
    3 * 1048576,                // mDefaultThreadStackSize
    1,                          // mUseMemoryPool
    ID_FALSE,                   // mInspectionLargeHeapThresholdInitialized
};

IDE_RC iduProperty::load()
{
    SChar *sValue = NULL;

    IDE_ASSERT(idp::read("DIRECT_IO_ENABLED", &mProperties->mDirectIOEnabled)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SM_XLATCH_USE_SIGNAL",
                         &mProperties->mXLatchUseSignal)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MUTEX_TYPE", &mProperties->mMutexType)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("CHECK_MUTEX_DURATION_TIME_ENABLE",
                         &mProperties->mCheckMutexDurationTimeEnable)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("TIMED_STATISTICS",
                         &mProperties->mTimedStatistics)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LATCH_TYPE", &mProperties->mLatchType)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LATCH_MINSLEEP", &mProperties->mLatchMinSleep)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LATCH_MAXSLEEP", &mProperties->mLatchMaxSleep)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MUTEX_SLEEPTYPE", &mProperties->mLatchSleepType)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MUTEX_SPIN_COUNT", &mProperties->mMutexSpinCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("NATIVE_MUTEX_SPIN_COUNT", &mProperties->mNativeMutexSpinCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("NATIVE_MUTEX_SPIN_COUNT_FOR_LOGGING",
                         &mProperties->mNativeMutexSpinCount4Logging)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LATCH_SPIN_COUNT", &mProperties->mLatchSpinCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("PROTOCOL_DUMP", &mProperties->mProtocolDump)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SERVER_MSGLOG_FLAG", &mProperties->mServerTrcFlag)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SM_MSGLOG_FLAG", &mProperties->mSmTrcFlag)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("QP_MSGLOG_FLAG", &mProperties->mQpTrcFlag)
               == IDE_SUCCESS);

    /* BUG-49108 */
    IDE_ASSERT(idp::read("JOB_MSGLOG_FLAG", &mProperties->mJobTrcFlag)
               == IDE_SUCCESS);

    /* BUG-46138 */
    IDE_ASSERT(idp::read("SD_MSGLOG_FLAG", &mProperties->mSdTrcFlag)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SHARD_ENABLE", &mProperties->mSdTrcEnable)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("RP_MSGLOG_FLAG", &mProperties->mRpTrcFlag)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("RP_CONFLICT_MSGLOG_FLAG", &mProperties->mRpConflictTrcFlag)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("RP_CONFLICT_MSGLOG_ENABLE", &mProperties->mRpConflictTrcEnable)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("DK_MSGLOG_FLAG", &mProperties->mDkTrcFlag)
               == IDE_SUCCESS);

    // bug-24840 divide xa log
    IDE_ASSERT(idp::read("XA_MSGLOG_FLAG", &mProperties->mXaTrcFlag)
               == IDE_SUCCESS);

    /* BUG-41909 Add dump CM block when a packet error occurs */
    IDE_ASSERT(idp::read("CM_MSGLOG_FLAG", &mProperties->mCmTrcFlag)
               == IDE_SUCCESS);

    /* BUG-45274 */
    IDE_ASSERT(idp::read("LB_MSGLOG_FLAG", &mProperties->mLbTrcFlag)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MM_MSGLOG_FLAG", &mProperties->mMmTrcFlag)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SOURCE_INFO", &mProperties->mSourceInfo)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("ALL_MSGLOG_FLUSH", &mProperties->mAllMsglogFlush)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("PREPARE_STMT_MEMORY_MAXIMUM", &mProperties->mQMPMemMaxSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("EXECUTE_STMT_MEMORY_MAXIMUM", &mProperties->mQMXMemMaxSize)
               == IDE_SUCCESS);

    //===================================================================
    // To Fix PR-13963
    //===================================================================
    IDE_ASSERT( idp::read( (const SChar*) "INSPECTION_LARGE_HEAP_THRESHOLD",
                           &mProperties->mInspectionLargeHeapThreshold ) == IDE_SUCCESS );

    mProperties->mInspectionLargeHeapThresholdInitialized = ID_TRUE;

    //===================================================================
    // PROJ-1598
    //===================================================================

    IDE_ASSERT( idp::read( (const SChar*) "QUERY_PROF_FLAG",
                           &mProperties->mQueryProfFlag ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (const SChar*) "QUERY_PROF_BUF_SIZE",
                           &mProperties->mQueryProfBufSize ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (const SChar*) "QUERY_PROF_BUF_FLUSH_SIZE",
                           &mProperties->mQueryProfBufFlushSize ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (const SChar*) "QUERY_PROF_BUF_FULL_SKIP",
                           &mProperties->mQueryProfBufFullSkip ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (const SChar*) "QUERY_PROF_FILE_SIZE",
                           &mProperties->mQueryProfFileSize ) == IDE_SUCCESS );

    /* BUG-36806 */
    IDE_ASSERT( idp::readPtr( (const SChar*) "QUERY_PROF_LOG_DIR", 
                              (void **)&sValue ) == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mQueryProfLogDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mQueryProfLogDir[ ID_MAX_FILE_NAME - 1 ] = '\0';

    IDE_ASSERT( idp::read("DIRECT_IO_PAGE_SIZE",
                          &mProperties->mDirectIOPageSize )
                == IDE_SUCCESS );

    //===================================================================
    // PROJ-2223 Altibase Auditing
    //===================================================================

    IDE_ASSERT( idp::read( (const SChar*) "AUDIT_BUFFER_SIZE",
                           &mProperties->mAuditBufferSize ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (const SChar*) "AUDIT_BUFFER_FLUSH_SIZE",
                           &mProperties->mAuditBufferFlushSize ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (const SChar*) "AUDIT_BUFFER_FULL_SKIP",
                           &mProperties->mAuditBufferFullSkip ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (const SChar*) "AUDIT_FILE_SIZE",
                           &mProperties->mAuditFileSize ) == IDE_SUCCESS );
    
    /* BUG-36807 */
    IDE_ASSERT(idp::readPtr( (const SChar *)"AUDIT_LOG_DIR",
                             (void **)&sValue) == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mAuditLogDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mAuditLogDir[ ID_MAX_FILE_NAME - 1 ] = '\0';

    /* BUG-39760 Enable AltiAudit to write log into syslog */
    IDE_ASSERT( idp::read( (const SChar *)"AUDIT_OUTPUT_METHOD",
                           &mProperties->mAuditOutputMethod ) == IDE_SUCCESS );

    IDE_ASSERT(idp::readPtr( (const SChar *)"AUDIT_TAG_NAME_IN_SYSLOG",
                             (void **)&sValue) == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mAuditTagNameInSyslog,
                    sValue,
                    IDP_MAX_PROP_STRING_LEN );




    // PRJ-1552
    IDE_ASSERT(idp::read("ENABLE_RECOVERY_TEST",
                         (void**)&mProperties->mEnableRecoveryTest)
               == IDE_SUCCESS);

    // fix BUG-21266
    IDE_ASSERT(idp::read("DEFAULT_THREAD_STACK_SIZE", &mProperties->mDefaultThreadStackSize)
               == IDE_SUCCESS);

    // fix BUG-21547
    IDE_ASSERT(idp::read("USE_MEMORY_POOL", &mProperties->mUseMemoryPool)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__USE_DUMP_CALLSTACKS", &mProperties->mUseDumpCallstacks)
               == IDE_SUCCESS);

    /*
     * BUG-21487     Mutex Leak List출력을 property화 해야합니다.
     */
    IDE_ASSERT(idp::read("SHOW_MUTEX_LEAK_LIST",
                         (void*)&mProperties->mShowMutexLeakList)
               == IDE_SUCCESS);

    // BUG-20789
    IDE_ASSERT(idp::read("SCALABILITY_PER_CPU",
                         (void*)&mProperties->mScalabilityPerCPU)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MAX_SCALABILITY",
                         (void*)&mProperties->mMaxScalability)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("NET_CONN_IP_STACK", &mProperties->mNetConnIpStack)
                               == IDE_SUCCESS);

    /*
     * PROJ-2118 Bug Reporting
     */
    IDE_ASSERT(idp::read( "__ERROR_VALIDATION_LEVEL",
                          (void*)&mProperties->mErrorValidationLevel )
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "__WRITE_ERROR_TRACE",
                          (void*)&mProperties->mWriteErrorTrace )
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "__WRITE_PSTACK",
                          (void*)&mProperties->mWritePstack )
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "__USE_SIGALTSTACK",
                          (void*)&mProperties->mUseSigAltStack )
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "__LOGFILE_COLLECT_COUNT_IN_DUMP",
                          (void*)&mProperties->mLogCollectCount )
               == IDE_SUCCESS );
    
    IDE_ASSERT(idp::read( "COLLECT_DUMP_INFO",
                          (void*)&mProperties->mCollectDumpInfo )
               == IDE_SUCCESS );
    
    IDE_ASSERT(idp::read( "__WRITE_WINDOWS_MINIDUMP",
                          (void*)&mProperties->mWriteWindowsMinidump )
               == IDE_SUCCESS );

    mProperties->mRpNetConnIpStack = mProperties->mNetConnIpStack;

    IDE_ASSERT(idp::readPtr( "DEFAULT_DISK_DB_DIR",
                             (void **)&sValue)
               == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mDefaultDiskDBDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mDefaultDiskDBDir[ ID_MAX_FILE_NAME - 1 ] = '\0';
    
    IDE_ASSERT(idp::readPtr( "MEM_DB_DIR",
                             (void **)&sValue)
               == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mMemDBDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mMemDBDir[ ID_MAX_FILE_NAME - 1 ] = '\0';

    IDE_ASSERT(idp::readPtr( "LOG_DIR",
                             (void **)&sValue)
               == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mLogDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mLogDir[ ID_MAX_FILE_NAME - 1 ] = '\0';
    
    IDE_ASSERT(idp::readPtr( "ARCHIVE_DIR",
                             (void **)&sValue)
               == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mArchiveDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mArchiveDir[ ID_MAX_FILE_NAME - 1 ] = '\0';
    
    IDE_ASSERT(idp::readPtr( "SERVER_MSGLOG_DIR",
                             (void **)&sValue)
               == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mServerMsglogDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mServerMsglogDir[ ID_MAX_FILE_NAME - 1 ] = '\0';

    IDE_ASSERT(idp::readPtr( "LOGANCHOR_DIR",
                             (void **)&sValue, 0)
               == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mLogAnchorDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mLogAnchorDir[ ID_MAX_FILE_NAME - 1 ] = '\0';

    IDE_ASSERT(idp::readPtr( "XLOGFILE_DIR",
                             (void **)&sValue)
               == IDE_SUCCESS );
    idlOS::strncpy( mProperties->mXLogDir, sValue, ID_MAX_FILE_NAME );
    mProperties->mXLogDir[ ID_MAX_FILE_NAME - 1 ] = '\0';

    IDE_ASSERT(idp::read( "SHM_POLICY",
                          (void **)&mProperties->mShmMemoryPolicy, 0)
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "SHM_MAX_SIZE",
                          (void **)&mProperties->mShmMaxSize, 0)
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "SHM_LOCK",
                          (void **)&mProperties->mShmLock, 0)
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "SHM_CHUNK_ALIGN_SIZE",
                          (void **)&mProperties->mShmAlignSize, 0)
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "SHM_CHUNK_SIZE",
                          &mProperties->mShmChunkSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( "SHM_STARTUP_SIZE",
                          &mProperties->mShmStartUpSize)
               == IDE_SUCCESS);

#ifndef VC_WIN32
    IDE_ASSERT( idp::read("SHM_DB_KEY",
                          &mProperties->mShmDBKey)
                == IDE_SUCCESS );
#else
    mProperties->mShmDBKey = (UInt) 0;
#endif

    IDE_ASSERT( idp::read("SHM_LOGGING",
                          &mProperties->mShmLogging)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SHM_LATCH_SPIN_LOCK_COUNT",
                          &mProperties->mShmLatchSpinLockCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SHM_LATCH_YIELD_COUNT",
                          &mProperties->mShmLatchYieldCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SHM_LATCH_MAX_YIELD_COUNT",
                          &mProperties->mShmLatchMaxYieldCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SHM_LATCH_SLEEP_DURATION",
                          &mProperties->mShmLatchSleepDuration)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("USER_PROCESS_CPU_AFFINITY",
                          &mProperties->mUserProcessCpuAffinity)
                == IDE_SUCCESS );

    // PROJ-1685
    IDE_ASSERT(idp::read( "EXTPROC_AGENT_CONNECT_TIMEOUT", 
                          &mProperties->mExtprocAgentConnectTimeout )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( "EXTPROC_AGENT_IDLE_TIMEOUT", 
                          &mProperties->mExtprocAgentIdleTimeout )
               == IDE_SUCCESS);

    // BUG-32293
    IDE_ASSERT(idp::read( "QP_MEMORY_CHUNK_SIZE",
                          &mProperties->mQpMemChunkSize )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( "MEMORY_ALLOCATOR_USE_PRIVATE",
                          &mProperties->mMemAllocatorUsePrivate )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( "MEMORY_ALLOCATOR_POOLSIZE_PRIVATE",
                          &mProperties->mMemPrivatePoolSize )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( "__QUERY_HASH_STRING_LENGTH_MAX",
                          &mProperties->mQueryHashStringLengthMax )
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read ( "__MUTEX_POOL_MAX_SIZE",
                            &mProperties->mMutexPoolMaxSize )
                == IDE_SUCCESS );

    /* PROJ-2473 SNMP 지원 */
    IDE_ASSERT(idp::read("PORT_NO",           &mProperties->mPortNo)          == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SNMP_ENABLE",       &mProperties->mSNMPEnable)      == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_PORT_NO",      &mProperties->mSNMPPortNo)      == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_TRAP_PORT_NO", &mProperties->mSNMPTrapPortNo)  == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_RECV_TIMEOUT", &mProperties->mSNMPRecvTimeout) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_SEND_TIMEOUT", &mProperties->mSNMPSendTimeout) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_MSGLOG_FLAG",  &mProperties->mSNMPTrcFlag)     == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SNMP_ALARM_QUERY_TIMEOUT",
                         &mProperties->mSNMPAlarmQueryTimeout)        == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_ALARM_UTRANS_TIMEOUT",
                         &mProperties->mSNMPAlarmUtransTimeout)       == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_ALARM_FETCH_TIMEOUT",
                         &mProperties->mSNMPAlarmFetchTimeout)        == IDE_SUCCESS);
    IDE_ASSERT(idp::read("SNMP_ALARM_SESSION_FAILURE_COUNT",
                         &mProperties->mSNMPAlarmSessionFailureCount) == IDE_SUCCESS);

    // TASK=6327
    IDE_ASSERT(idp::read( "DISK_MAX_DB_SIZE",
                          &mProperties->mDiskMaxDBSize )
               == IDE_SUCCESS);

    // BUG-40492
    IDE_ASSERT(idp::read( "__MEMPOOL_MINIMUM_SLOT_COUNT",
                          &mProperties->mMinMemPoolChunkSlotCnt )
               == IDE_SUCCESS);

    // BUG-40819
    IDE_ASSERT(idp::read( "MAX_CLIENT",
                          &mProperties->mMaxClient )
               == IDE_SUCCESS);

    // BUG-40819
    IDE_ASSERT(idp::read( "JOB_THREAD_COUNT",
                          &mProperties->mJobThreadCount )
               == IDE_SUCCESS);

    // BUG-40819
    IDE_ASSERT(idp::read( "CONCURRENT_EXEC_DEGREE_MAX",
                          &mProperties->mConcExecDegreeMax )
               == IDE_SUCCESS);

    // BUG-40819
    IDE_ASSERT(idp::read( "EXTPROC_AGENT_CALL_RETRY_COUNT",
                          &mProperties->mExtprocAgentCallRetryCount )
               == IDE_SUCCESS);

    /* PROJ-2617 */
    IDE_ASSERT(idp::read( "__FAULT_TOLERANCE_ENABLE",
                          &mProperties->mFaultToleranceEnable )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read( "__FAULT_TOLERANCE_TRACE",
                          (void*)&mProperties->mFaultToleranceTrace )
               == IDE_SUCCESS );

    // BUG-44652 Socket file path of EXTPROC AGENT could be set by property.
    IDE_ASSERT(idp::readPtr( "EXTPROC_AGENT_SOCKET_FILEPATH",
                             (void**)&mProperties->mExtprocAgentSocketFilepath )
               == IDE_SUCCESS );

    IDE_ASSERT(idp::read( "THREAD_REUSE_ENABLE",
                          &mProperties->mThreadReuseEnable )
               == IDE_SUCCESS );

    /* BUG-46892 */
    IDE_ASSERT( idp::read( "MATHEMATICS_TEMP_MEMORY_MAXIMUM",
                           & mProperties->mMathTempMemMax )
                == IDE_SUCCESS );

#ifdef DEBUG
    gIdeFTTrace = mProperties->mFaultToleranceTrace;
#endif

    IDE_TEST(registCallbacks() != IDE_SUCCESS);

    IDE_TEST( checkConstraints() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#ifdef ALTIBASE_PRODUCT_XDB
/*
 * Loads or attaches to the ID properties in a shared memory data
 * segment, depending on the calling process' type.  The method
 * prepares iduProperty class for accessing the properties in HDB DA
 * Edition.
 *
 * The method behaves differently for the daemon process from the
 * other processes.  The daemon process allocates memory and loads the
 * properties in it, whereas other processes simply attach to the
 * properties already prepared by the daemon process.
 *
 * We assume the properties have already been loaded and set up
 * locally i.e. in the statically allocated buffer.  Once the shared
 * memory is allocated, the method simply copies the properties from
 * the buffer to the shared memory.  We do this due to the circular
 * dependency between iduShmMgr and iduProperty.
 */
IDE_RC iduProperty::loadShm( idvSQL *aStatistics )
{
    if ( iduGetShmProcType() == IDU_PROC_TYPE_DAEMON )
    {
        iduPropertyStore *sStore     = NULL;
        iduShmTxInfo     *sShmTxInfo = NULL;

        sShmTxInfo = IDV_WP_GET_THR_INFO( aStatistics );
        IDE_TEST( iduShmMgr::allocMemWithoutUndo( aStatistics,
                                                  sShmTxInfo,
                                                  IDU_MEM_ID_IDU,
                                                  ID_SIZEOF( iduPropertyStore ),
                                                  (void **)&sStore ) != IDE_SUCCESS );
        iduShmMgr::setMetaBlockAddr( IDU_SHM_META_ID_PROPERTIES_BLOCK,
                                     iduShmMgr::getAddr( sStore ) );

        /* Copy the properties into the shared memory storage and
         * switch to it. */
        (void)idlOS::memcpy( sStore,
                             &mStaticProperties,
                             sizeof( iduPropertyStore ) );
        mProperties = sStore;
    }
    else
    {
        idShmAddr sPropAddr;
        sPropAddr   = iduShmMgr::getMetaBlockAddr( IDU_SHM_META_ID_PROPERTIES_BLOCK );
        mProperties = (iduPropertyStore *)IDU_SHM_GET_ADDR_PTR_CHECK( sPropAddr );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::unloadShm( idvSQL *aStatistics )
{
    iduPropertyStore *sStore = NULL;

    /* Copy the properties back in the static storage for any changes
     * and use the static storage to serve them. */
    (void)idlOS::memcpy( &mStaticProperties,
                         mProperties,
                         sizeof( iduPropertyStore ) );
    sStore      = mProperties;
    mProperties = &mStaticProperties;

    if ( iduGetShmProcType() == IDU_PROC_TYPE_DAEMON )
    {
        iduShmTxInfo *sShmTxInfo = IDV_WP_GET_THR_INFO( aStatistics );

        IDE_TEST( iduShmMgr::freeMem( aStatistics,
                                      sShmTxInfo,
                                      NULL,
                                      sStore ) != IDE_SUCCESS );
    }
    else
    {
        /* Only allow the daemon process to do the final clean-up. */
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

#endif /* ALTIBASE_PRODUCT_XDB */

IDE_RC iduProperty::registCallbacks()
{
#define IDE_FN "iduProperty::registCallbacks()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( idp::setupAfterUpdateCallback(
        (const SChar*) "INSPECTION_LARGE_HEAP_THRESHOLD",
        callbackInspectionLargeHeapThreshold)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SM_XLATCH_USE_SIGNAL",
                                  callbackXLatchUseSignal)
              != IDE_SUCCESS);

    IDE_TEST(idp::setupAfterUpdateCallback("PROTOCOL_DUMP",
                                           callbackProtocolDump)
             != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("SOURCE_INFO", callbackSourceInfo)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("ALL_MSGLOG_FLUSH", callbackAllMsglogFlush)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("SERVER_MSGLOG_FLAG",
                                            callbackServerTrcFlag)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("SM_MSGLOG_FLAG",
                                            callbackSmTrcFlag)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("QP_MSGLOG_FLAG",
                                            callbackQpTrcFlag)
              != IDE_SUCCESS);

    /* BUG-49108 */
    IDE_TEST( idp::setupAfterUpdateCallback("JOB_MSGLOG_FLAG", callbackJobTrcFlag)
              != IDE_SUCCESS );

    /* BUG-46138 */
    IDE_TEST( idp::setupAfterUpdateCallback("SD_MSGLOG_FLAG", callbackSdTrcFlag)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("RP_MSGLOG_FLAG",
                                            callbackRpTrcFlag)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("RP_CONFLICT_MSGLOG_FLAG",
                                            callbackRpConflictTrcFlag)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("DK_MSGLOG_FLAG",
                                            callbackDkTrcFlag)
              != IDE_SUCCESS);

    // bug-24840 divide xa log
    IDE_TEST( idp::setupAfterUpdateCallback("XA_MSGLOG_FLAG",
                                            callbackXaTrcFlag)
              != IDE_SUCCESS);

    /* BUG-41909 Add dump CM block when a packet error occurs */
    IDE_TEST( idp::setupAfterUpdateCallback("CM_MSGLOG_FLAG",
                                            callbackCmTrcFlag)
              != IDE_SUCCESS);

    /* BUG-45274 */
    IDE_TEST( idp::setupAfterUpdateCallback("LB_MSGLOG_FLAG",
                                            callbackLbTrcFlag)
              != IDE_SUCCESS);

    /* BUG-45369 */
    IDE_TEST( idp::setupAfterUpdateCallback("MM_MSGLOG_FLAG",
                                            callbackMmTrcFlag)
              != IDE_SUCCESS);

    /* PROJ-2473 SNMP 지원 */
    IDE_TEST( idp::setupAfterUpdateCallback("SNMP_MSGLOG_FLAG",
                                            callbackSNMPTrcFlag)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("PREPARE_STMT_MEMORY_MAXIMUM",
                                            callbackPrepareMemoryMax)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("EXECUTE_STMT_MEMORY_MAXIMUM",
                                            callbackExecuteMemoryMax)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("CHECK_MUTEX_DURATION_TIME_ENABLE",
                                            callbackCheckMutexDurationTimeEnable)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("TIMED_STATISTICS",
                                            callbackTimedStatistics)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback ( "__MUTEX_POOL_MAX_SIZE",
                                              callbackMutexPoolMaxSize )
              != IDE_SUCCESS);

    /* BUG-46892 */
    IDE_TEST( idp::setupAfterUpdateCallback( "MATHEMATICS_TEMP_MEMORY_MAXIMUM",
                                             callbackMathTempMemMax )
              != IDE_SUCCESS );

#if !defined(LIB_BUILD)

    IDE_TEST( idp::setupBeforeUpdateCallback("QUERY_PROF_FLAG",
                                            callbackQueryProfFlag)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("QUERY_PROF_BUF_FLUSH_SIZE",
                                            callbackQueryProfBufFlushSize)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("QUERY_PROF_BUF_FULL_SKIP",
                                            callbackQueryProfBufFullSkip)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("QUERY_PROF_BUF_SIZE",
                                            callbackQueryProfBufSize)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("QUERY_PROF_FILE_SIZE",
                                            callbackQueryProfFileSize)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("QUERY_PROF_LOG_DIR",
                                            callbackQueryProfLogDir)
              != IDE_SUCCESS);

    /* BUG-37706 */
    IDE_TEST( idp::setupBeforeUpdateCallback("AUDIT_BUFFER_FLUSH_SIZE",
                                            callbackAuditBufferFlushSize)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("AUDIT_BUFFER_FULL_SKIP",
                                            callbackAuditBufferFullSkip)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("AUDIT_BUFFER_SIZE",
                                            callbackAuditBufferSize)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("AUDIT_FILE_SIZE",
                                            callbackAuditFileSize)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupBeforeUpdateCallback("AUDIT_LOG_DIR",
                                            callbackAuditLogDir)
              != IDE_SUCCESS);
#endif



    /*
     * PROJ-2118 Bug Reporting
     */
    IDE_TEST( idp::setupAfterUpdateCallback("__ERROR_VALIDATION_LEVEL",
                                            callbackErrorValidationLevel)
              != IDE_SUCCESS);
    
    IDE_TEST( idp::setupAfterUpdateCallback("__WRITE_ERROR_TRACE",
                                            callbackWriteErrorTrace)
              != IDE_SUCCESS);
    
    IDE_TEST( idp::setupAfterUpdateCallback("__WRITE_PSTACK",
                                            callbackWritePstack)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("__USE_SIGALTSTACK",
                                            callbackUseSigAltStack)
              != IDE_SUCCESS);


    IDE_TEST( idp::setupAfterUpdateCallback("__LOGFILE_COLLECT_COUNT_IN_DUMP",
                                            callbackLogCollectCount)
              != IDE_SUCCESS);
    
    IDE_TEST( idp::setupAfterUpdateCallback("COLLECT_DUMP_INFO",
                                            callbackCollectDumpInfo)
              != IDE_SUCCESS);
    
    IDE_TEST( idp::setupAfterUpdateCallback("__WRITE_WINDOWS_MINIDUMP",
                                            callbackWriteWindowsMinidump)
              != IDE_SUCCESS);
    
    IDE_TEST( idp::setupAfterUpdateCallback("DISK_MAX_DB_SIZE",
                                            callbackDiskMaxDBSize)
              != IDE_SUCCESS);
    
    IDE_TEST( idp::setupAfterUpdateCallback("SHM_LOGGING",
                                            callbackShmLogging)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("__USE_DUMP_CALLSTACKS",
                                            callbackUseDumpCallstacks)
              != IDE_SUCCESS);

    IDE_TEST( idp::setupAfterUpdateCallback("SHM_LATCH_SPIN_LOCK_COUNT",
                                            callbackShmLatchSpinLockCount)
              != IDE_SUCCESS);

    // BUG-40819
    IDE_TEST( idp::setupAfterUpdateCallback("EXTPROC_AGENT_CALL_RETRY_COUNT",
                                            callbackExtprocAgentCallRetryCount)
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC iduProperty::callbackXLatchUseSignal(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mXLatchUseSignal = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackProtocolDump(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mProtocolDump = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackServerTrcFlag(idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void  * /*aOldValue*/,
                                          void  *aNewValue,
                                          void  * /*aArg*/)
{
    mProperties->mServerTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackSmTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar  * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mSmTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackQpTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar  * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mQpTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-49108 */
IDE_RC iduProperty::callbackJobTrcFlag(idvSQL * /*aStatistics*/,
                                       SChar  * /*aName*/,
                                       void   * /*aOldValue*/,
                                       void   * aNewValue,
                                       void   * /*aArg*/)
{
    mProperties->mJobTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-46138 */
IDE_RC iduProperty::callbackSdTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar  * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mSdTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackRpTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar  * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mRpTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackRpConflictTrcFlag(idvSQL * /*aStatistics*/,
                                              SChar * /*aName*/,
                                              void  * /*aOldValue*/,
                                              void  *aNewValue,
                                              void  * /*aArg*/)
{
    mProperties->mRpConflictTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackDkTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mDkTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// bug-24840 divide xa log
IDE_RC iduProperty::callbackXaTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mXaTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-41909 Add dump CM block when a packet error occurs */
IDE_RC iduProperty::callbackCmTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mCmTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* bug-45274 */
IDE_RC iduProperty::callbackLbTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar * /*aNam*/,
                                      void  * /* aOldValue */,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mLbTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackMmTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/)
{
    mProperties->mMmTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* PROJ-2473 SNMP 지원 */
IDE_RC iduProperty::callbackSNMPTrcFlag(idvSQL * /*aStatistics*/,
                                        SChar * /*aName*/,
                                        void  * /*aOldValue*/,
                                        void  *aNewValue,
                                        void  * /*aArg*/)
{
    mProperties->mSNMPTrcFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackSourceInfo(idvSQL * /*aStatistics*/,
                                       SChar  * /*aName*/,
                                       void  * /*aOldValue*/,
                                       void  *aNewValue,
                                       void  * /*aArg*/)
{
    mProperties->mSourceInfo = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackAllMsglogFlush(idvSQL * /*aStatistics*/,
                                           SChar  * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/)
{
    mProperties->mAllMsglogFlush = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackPrepareMemoryMax(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mQMPMemMaxSize = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackExecuteMemoryMax(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mQMXMemMaxSize = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

/* ------------------------------------------------
 *   PROJ-1598
 *   Clinet Library의 경우에는 idvProfile 관련된
 *   프로퍼티 및 관련 코드가 링크될 필요가 없다.
 * ----------------------------------------------*/
#if !defined(LIB_BUILD)

IDE_RC iduProperty::callbackQueryProfFlag(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvProfile::setProfFlag(*((UInt *)aNewValue))
             != IDE_SUCCESS);
    mProperties->mQueryProfFlag = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::callbackQueryProfBufSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvProfile::setBufSize(*((UInt *)aNewValue))
             != IDE_SUCCESS);

    mProperties->mQueryProfBufSize = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC iduProperty::callbackQueryProfBufFlushSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvProfile::setBufFlushSize(*((UInt *)aNewValue))
             != IDE_SUCCESS);

    mProperties->mQueryProfBufFlushSize = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::callbackQueryProfBufFullSkip(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvProfile::setBufFullSkipFlag(*((UInt *)aNewValue))
             != IDE_SUCCESS);

    mProperties->mQueryProfBufFullSkip = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::callbackQueryProfFileSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvProfile::setFileSize(*((UInt *)aNewValue))
             != IDE_SUCCESS);
    mProperties->mQueryProfFileSize = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::callbackQueryProfLogDir(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue, 
    void  * /*aArg*/)
{
    IDE_TEST(idvProfile::setLogDir(((SChar *)aNewValue))
             != IDE_SUCCESS);

    idlOS::strncpy( mProperties->mQueryProfLogDir,
		    (SChar *)aNewValue,
		    ID_MAX_FILE_NAME );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* BUG-37706 */
IDE_RC iduProperty::callbackAuditBufferSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvAudit::setBufferSize(*((UInt *)aNewValue))
             != IDE_SUCCESS);

    mProperties->mAuditBufferSize = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC iduProperty::callbackAuditBufferFlushSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvAudit::setBufferFlushSize(*((UInt *)aNewValue))
             != IDE_SUCCESS);

    mProperties->mAuditBufferFlushSize = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::callbackAuditBufferFullSkip(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvAudit::setBufferFullSkipFlag(*((UInt *)aNewValue))
             != IDE_SUCCESS);

    mProperties->mAuditBufferFullSkip = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::callbackAuditFileSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    IDE_TEST(idvAudit::setFileSize(*((UInt *)aNewValue))
             != IDE_SUCCESS);
    mProperties->mAuditFileSize = *((UInt *)aNewValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduProperty::callbackAuditLogDir(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue, 
    void  * /*aArg*/)
{
    IDE_TEST(idvAudit::setAuditLogDir(((SChar *)aNewValue))
             != IDE_SUCCESS);

    idlOS::strncpy( mProperties->mAuditLogDir,
		    (SChar *)aNewValue,
		    ID_MAX_FILE_NAME );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

#endif

//===================================================================
// To Fix PR-13963
//===================================================================
IDE_RC iduProperty::callbackInspectionLargeHeapThreshold(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    ideLog::log(IDE_SERVER_0, ID_TRC_HEAP_INSPECTION_ACK,
                mProperties->mInspectionLargeHeapThreshold, *((UInt*)aNewValue) );
    mProperties->mInspectionLargeHeapThreshold = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}


//===================================================================
// BUG-17371
//===================================================================
IDE_RC iduProperty::callbackCheckMutexDurationTimeEnable(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mCheckMutexDurationTimeEnable = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

/*
 * TASK-2356 [제품문제분석] DRDB의 DML문제파악
 * altibase wait interface 시간통계정보 수집
 */
IDE_RC iduProperty::callbackTimedStatistics(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mTimedStatistics = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackMutexPoolMaxSize(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mMutexPoolMaxSize = *((UInt*) aNewValue);
    iduMutexMgr::setPoolMaxCount(mProperties->mMutexPoolMaxSize);
    IDE_TEST( iduMutexMgr::freeIdles() != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//===================================================================
// PROJ-2118 BUG Reporting __ERROR_VALIDATION_LEVEL Property
//===================================================================
IDE_RC iduProperty::callbackErrorValidationLevel(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mErrorValidationLevel = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

//===================================================================
// PROJ-2118 BUG Reporting __WRITE_ERROR_TRACE Property
//===================================================================
IDE_RC iduProperty::callbackWriteErrorTrace(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mWriteErrorTrace = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

//===================================================================
// PROJ-2118 BUG Reporting __WRITE_PSTACK Property
//===================================================================
IDE_RC iduProperty::callbackWritePstack(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mWritePstack = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

//===================================================================
// PROJ-2118 BUG Reporting __USE_SIGALTSTACK Property
//===================================================================
IDE_RC iduProperty::callbackUseSigAltStack(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mUseSigAltStack = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}


//===================================================================
// PROJ-2118 BUG Reporting __LOGFILE_COLLECT_COUNT_IN_DUMP Property
//===================================================================
IDE_RC iduProperty::callbackLogCollectCount(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mLogCollectCount = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

//===================================================================
// PROJ-2118 BUG Reporting COLLECT_DUMP_INFO Property
//===================================================================
IDE_RC iduProperty::callbackCollectDumpInfo(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mCollectDumpInfo = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

//===================================================================
// PROJ-2118 BUG Reporting __WRITE_WINDOWS_MINIDUMP Property
//===================================================================
IDE_RC iduProperty::callbackWriteWindowsMinidump(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mProperties->mWriteWindowsMinidump = *((UInt*) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackDiskMaxDBSize( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/ )
{
    mProperties->mDiskMaxDBSize = *((ULong*) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::checkConstraints()
{
    // 다음 IF문은 Direct I/O최대 크기가 8192라는 가정하에 쓰여졌다.
    // 이 Direct I/O최대 크기가 바뀌면
    // - 이 if조건도 함께 바뀌어야 한다.
    // - idERR_ABORT_WrongDirectIOPageSize
    IDE_ASSERT( ID_MAX_DIO_PAGE_SIZE == 8192 );

    if ( mProperties->mDirectIOPageSize == 512  ||
         mProperties->mDirectIOPageSize == 1024 ||
         mProperties->mDirectIOPageSize == 2048 ||
         mProperties->mDirectIOPageSize == 4096 ||
         mProperties->mDirectIOPageSize == 8192 )
    {
        // 정상상황. Do Nothing.
    }
    else
    {
        IDE_RAISE( err_invalid_dio_page_size );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_dio_page_size );
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_WrongDirectIOPageSize));
        /* BUG-17621:
         * DIRECT_IO_PAGE_SIZE 속성 값이 부적절한 값일 때 server start를 하면
         * altibase_boot.log에 기록되는 오류 메시지가
         * 속성 값이 잘못되었다는 내용이 아니라
         * "No Error Message Loaded"라는 엉뚱한 오류 메시지가 출력되는 버그가 있었다.
         * 버그의 원인은 오류 메시지 로딩 전에 현재 라인이 수행되기 때문이었다.
         * 하지만 선후 의존관계 때문에
         * 오류 메시지를 먼저 로딩하고 현재 라인을 수행하는 것은 곤란하다.
         * 따라서, 부득이 아래와 같이 하드 코딩으로
         * 오류 메시지를 설정하는 코드를 추가하여 버그를 수정한다. */
        if (idlOS::strcmp(ideGetErrorMgr()->Stack.LastErrorMsg,
                          "No Error Message Loaded")
            == 0)
        {
            SChar sLastErrorMsg[MAX_ERROR_MSG_LEN];
            idlOS::snprintf(sLastErrorMsg,
                            MAX_ERROR_MSG_LEN,
                            "The page size for Direct I/O should be exactly "
                            "either 512, 1024, 2048, 4096, or 8192.");
            ideSetErrorCodeAndMsg(idERR_FATAL_WrongDirectIOPageSize,
                                  sLastErrorMsg);
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC iduProperty::callbackShmLogging(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/)
{
    mProperties->mShmLogging = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackUseDumpCallstacks(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/)
{
    mProperties->mUseDumpCallstacks = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC iduProperty::callbackShmLatchSpinLockCount(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/)
{
    mProperties->mShmLatchSpinLockCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// BUG-40819
IDE_RC iduProperty::callbackExtprocAgentCallRetryCount(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/)
{
    mProperties->mExtprocAgentCallRetryCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-46892 */
IDE_RC iduProperty::callbackMathTempMemMax(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/ )
{
    mProperties->mMathTempMemMax = *( (ULong *) aNewValue );

    return IDE_SUCCESS;
}
