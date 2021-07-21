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
 
/***********************************************************************
 * $Id: 
 *
 **********************************************************************/

#ifndef O_IDU_PROPERTY_H
#define O_IDU_PROPERTY_H 1

#include <acpPath.h>
#include <idl.h>
#include <idp.h>

//===================================================================
// To Fix PR-13963
//===================================================================
#define INSPECTION_LARGE_HEAP_THRESHOLD_INITIALIZED             \
    (iduProperty::getInspectionLargeHeapThresholdInitialized())
#define INSPECTION_LARGE_HEAP_THRESHOLD                 \
    (iduProperty::getInspectionLargeHeapThreshold())

// fix BUG-21266
#define IDU_DEFAULT_THREAD_STACK_SIZE           \
    (iduProperty::getDefaultThreadStackSize())

// fix BUG-21547
#define IDU_USE_MEMORY_POOL                     \
    (iduProperty::getUseMemoryPool())

// PROJ-1685
#define IDU_EXTPROC_AGENT_CONNECT_TIMEOUT               \
    (iduProperty::getExtprocAgentConnectTimeout())

#define IDU_EXTPROC_AGENT_IDLE_TIMEOUT          \
    (iduProperty::getExtprocAgentIdleTimeout())

#define IDV_TIMED_STATISTICS_OFF (0)
#define IDV_TIMED_STATISTICS_ON  (1)

struct idvSQL;

class iduProperty
{
private:
    static IDE_RC checkConstraints();
    
public:
    static IDE_RC registCallbacks();

    static IDE_RC callbackXLatchUseSignal( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);

    static IDE_RC callbackProtocolDump( idvSQL * /*aStatistics*/,
                                        SChar * /*aName*/,
                                        void  * /*aOldValue*/,
                                        void  *aNewValue,
                                        void  * /*aArg*/);

    // To Fix PR-13963
    static IDE_RC callbackInspectionLargeHeapThreshold( idvSQL * /*aStatistics*/,
                                                        SChar * /*aName*/,
                                                        void  * /*aOldValue*/,
                                                        void  *aNewValue,
                                                        void  * /*aArg*/);

    /* PROJ-2473 SNMP 지원 */
    static IDE_RC callbackSNMPTrcFlag(idvSQL * /*aStatistics*/,
                                      SChar * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/);

    static IDE_RC callbackServerTrcFlag( idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/);
    static IDE_RC callbackSmTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);
    static IDE_RC callbackQpTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);

    /* BUG-49108 */
    static IDE_RC callbackJobTrcFlag( idvSQL * /*aStatistics*/,
                                      SChar  * /*aName*/,
                                      void   * /*aOldValue*/,
                                      void   * aNewValue,
                                      void   * /*aArg*/ );

    /* BUG-46138 */
    static IDE_RC callbackSdTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);

    static IDE_RC callbackRpTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);
    static IDE_RC callbackRpConflictTrcFlag( idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /*aArg*/);
    static IDE_RC callbackDkTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);

    // bug-24840 divide xa log
    static IDE_RC callbackXaTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);
    /* BUG-41909 Add dump CM block when a packet error occurs */
    static IDE_RC callbackCmTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);
    /* bug-45274 */
    static IDE_RC callbackLbTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);
                                     
    static IDE_RC callbackMmTrcFlag( idvSQL * /*aStatistics*/,
                                     SChar * /*aName*/,
                                     void  * /*aOldValue*/,
                                     void  *aNewValue,
                                     void  * /*aArg*/);
    static IDE_RC callbackSourceInfo( idvSQL * /*aStatistics*/,
                                      SChar * /*aName*/,
                                      void  * /*aOldValue*/,
                                      void  *aNewValue,
                                      void  * /*aArg*/);
    static IDE_RC callbackAllMsglogFlush( idvSQL * /*aStatistics*/,
                                          SChar * /*aName*/,
                                          void  * /*aOldValue*/,
                                          void  *aNewValue,
                                          void  * /*aArg*/);

    static IDE_RC callbackPrepareMemoryMax( idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);
    
    static IDE_RC callbackExecuteMemoryMax( idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);

    static IDE_RC callbackQueryProfFlag( idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/);
    
    static IDE_RC callbackQueryProfBufSize( idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);
    

    static IDE_RC callbackQueryProfBufFlushSize( idvSQL * /*aStatistics*/,
                                                 SChar * /*aName*/,
                                                 void  * /*aOldValue*/,
                                                 void  *aNewValue,
                                                 void  * /*aArg*/);
    

    static IDE_RC callbackQueryProfBufFullSkip( idvSQL * /*aStatistics*/,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  *aNewValue,
                                                void  * /*aArg*/);
    
    static IDE_RC callbackQueryProfFileSize( idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/,
                                             void  * /*aOldValue*/,
                                             void  *aNewValue,
                                             void  * /*aArg*/);

    static IDE_RC callbackQueryProfLogDir( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);

    /* BUG-37706 */
    static IDE_RC callbackAuditBufferSize( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);

    static IDE_RC callbackAuditBufferFlushSize( idvSQL * /*aStatistics*/,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  *aNewValue,
                                                void  * /*aArg*/);
    
    static IDE_RC callbackAuditBufferFullSkip( idvSQL * /*aStatistics*/,
                                               SChar * /*aName*/,
                                               void  * /*aOldValue*/,
                                               void  *aNewValue,
                                               void  * /*aArg*/);
    
    static IDE_RC callbackAuditFileSize( idvSQL * /*aStatistics*/,
                                         SChar * /*aName*/,
                                         void  * /*aOldValue*/,
                                         void  *aNewValue,
                                         void  * /*aArg*/);

    static IDE_RC callbackAuditLogDir( idvSQL * /*aStatistics*/,
                                       SChar * /*aName*/,
                                       void  * /*aOldValue*/,
                                       void  *aNewValue,
                                       void  * /*aArg*/);

    static IDE_RC callbackCheckMutexDurationTimeEnable( idvSQL * /*aStatistics*/,
                                                        SChar * /*aName*/,
                                                        void  * /*aOldValue*/,
                                                        void  *aNewValue,
                                                        void  * /*aArg*/);

    static IDE_RC callbackMutexPoolMaxSize( idvSQL * /*aStatistics*/,
                                            SChar * /*aName*/,
                                            void  * /*aOldValue*/,
                                            void  *aNewValue,
                                            void  * /*aArg*/);

    static IDE_RC callbackTimedStatistics( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);

    /*
     * PROJ-2118 Bug Reporting
     */
    static IDE_RC callbackErrorValidationLevel( idvSQL * /*aStatistics*/,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  *aNewValue,
                                                void  * /*aArg*/);
    
    static IDE_RC callbackWriteErrorTrace( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);

    static IDE_RC callbackWritePstack( idvSQL * /*aStatistics*/,
                                       SChar * /*aName*/,
                                       void  * /*aOldValue*/,
                                       void  *aNewValue,
                                       void  * /*aArg*/);

    static IDE_RC callbackUseSigAltStack( idvSQL * /*aStatistics*/,
                                          SChar * /*aName*/,
                                          void  * /*aOldValue*/,
                                          void  *aNewValue,
                                          void  * /*aArg*/);

    static IDE_RC callbackLogCollectCount( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);
    
    static IDE_RC callbackCollectDumpInfo( idvSQL * /*aStatistics*/,
                                           SChar * /*aName*/,
                                           void  * /*aOldValue*/,
                                           void  *aNewValue,
                                           void  * /*aArg*/);
    
    static IDE_RC callbackWriteWindowsMinidump( idvSQL * /*aStatistics*/,
                                                SChar * /*aName*/,
                                                void  * /*aOldValue*/,
                                                void  *aNewValue,
                                                void  * /*aArg*/);

    static IDE_RC callbackShmLogging( idvSQL * /*aStatistics*/,
                                      SChar  * /*aName*/,
                                      void   * /*aOldValue*/,
                                      void   * aNewValue,
                                      void   * /*aArg*/);

    static IDE_RC callbackUseDumpCallstacks( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * /*aArg*/);

    static IDE_RC callbackShmLatchSpinLockCount( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/);

    // TASK-6327
    static IDE_RC callbackDiskMaxDBSize( idvSQL * /*aStatistics*/,
                                         SChar  * /*aName*/,
                                         void   * /*aOldValue*/,
                                         void   * aNewValue,
                                         void   * /*aArg*/ );

    // BUG-40819
    static IDE_RC callbackExtprocAgentCallRetryCount( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ );

    /* BUG-46892 */
    static IDE_RC callbackMathTempMemMax( idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ );

    /*
     * Loads the properties into the statically allocated memory and
     * sets up iduProperty for accessing the properties.
     */
    static IDE_RC load();

#ifdef ALTIBASE_PRODUCT_XDB
    static IDE_RC loadShm( idvSQL *aStatistics );
    static IDE_RC unloadShm( idvSQL *aStatistics );
#endif

    static UInt getXLatchUseSignal()
    {
        return mProperties->mXLatchUseSignal;
    }
    
    static UInt getMutexType()
    {
        return mProperties->mMutexType;
    }

    static UInt getCheckMutexDurationTimeEnable()
    {
        return mProperties->mCheckMutexDurationTimeEnable;
    }

    static UInt getTimedStatistics()
    {
        return mProperties->mTimedStatistics;
    }
    
    static UInt getLatchType()
    {
        return mProperties->mLatchType;
    }
    
    static UInt getMutexSpinCount()
    {
        return mProperties->mMutexSpinCount;
    }

    static UInt getNativeMutexSpinCount()
    {
        return mProperties->mNativeMutexSpinCount;
    }

    static UInt getNativeMutexSpinCount4Logging()
    {
        return mProperties->mNativeMutexSpinCount4Logging;
    }

    static UInt getLatchSpinCount()
    {
        return mProperties->mLatchSpinCount;
    }

    static UInt getLatchMinSleep()
    {
        return mProperties->mLatchMinSleep;
    }
    static UInt getLatchMaxSleep()
    {
        return mProperties->mLatchMaxSleep;
    }
    static UInt getMutexSleepType()
    {
        return mProperties->mLatchSleepType;
    }

    static UInt getMutexPoolMaxSize()
    {
        return mProperties->mMutexPoolMaxSize;
    }

    static UInt getProtocolDump()
    {
        return mProperties->mProtocolDump;
    }
    
    static UInt getServerTrcFlag()
    {
        return mProperties->mServerTrcFlag;
    }

    static inline UInt getSmTrcFlag()
    {
        return mProperties->mSmTrcFlag;
    }

    static UInt getQpTrcFlag()
    {
        return mProperties->mQpTrcFlag;
    }

    /* BUG-49108 */
    static UInt getJobTrcFlag()
    {
        return mProperties->mJobTrcFlag;
    }

    /* BUG-46138 */
    static UInt getSdTrcFlag()
    {
        return mProperties->mSdTrcFlag;
    }

    static UInt getSdTrcEnable()
    {
        return mProperties->mSdTrcEnable;
    }

    static UInt getRpTrcFlag()
    {
        return mProperties->mRpTrcFlag;
    }

    static UInt getDkTrcFlag()
    {
        return mProperties->mDkTrcFlag;
    }
    static UInt getRpConflictTrcFlag()
    {
        return mProperties->mRpConflictTrcFlag;
    }
    static UInt getRpConflictTrcEnable()
    {
        return mProperties->mRpConflictTrcEnable;
    }

    // bug-24840 divide xa log
    static UInt getXaTrcFlag()
    {
        return mProperties->mXaTrcFlag;
    }

    /* BUG-41909 Add dump CM block when a packet error occurs */
    static UInt getCmTrcFlag()
    {
        return mProperties->mCmTrcFlag;
    }

    /* bug-45274 */
    static UInt getLbTrcFlag()
    {
        return mProperties->mLbTrcFlag;
    }

    static UInt getMmTrcFlag()
    {
        return mProperties->mMmTrcFlag;
    }

    static UInt getSourceInfo()
    {
        return mProperties->mSourceInfo;
    }

    static UInt getAllMsglogFlush()
    {
        return mProperties->mAllMsglogFlush;
    }

    static ULong getPrepareMemoryMax()
    {
        return mProperties->mQMPMemMaxSize;
    }

    static ULong getExecuteMemoryMax()
    {
        return mProperties->mQMXMemMaxSize;
    }

    static UInt getDirectIOEnabled()
    {
        return mProperties->mDirectIOEnabled;
    }

    static UInt   getQueryProfFlag()
    {
        return mProperties->mQueryProfFlag;
    }
    
    static UInt   getQueryProfBufSize()
    {
        return mProperties->mQueryProfBufSize;
    }
    
    static UInt   getQueryProfBufFullSkip()
    {
        return mProperties->mQueryProfBufFullSkip;
    }
    
    static UInt   getQueryProfBufFlushSize()
    {
        return mProperties->mQueryProfBufFlushSize;
    }
    
    static UInt   getQueryProfFileSize()
    {
        return mProperties->mQueryProfFileSize;
    }
    
    /* BUG-36806 */
    static SChar *getQueryProfLogDir()
    {
        return mProperties->mQueryProfLogDir;
    }

    // Direct I/O시 file offset및 data size를 Align할 Page크기
    static UInt getDirectIOPageSize()
    {
        return mProperties->mDirectIOPageSize;
    }

    /* PROJ-2223 Altibase Auditing */
    static UInt getAuditBufferSize()
    {
        return mProperties->mAuditBufferSize;
    }
    
    static UInt getAuditBufferFullSkip()
    {
        return mProperties->mAuditBufferFullSkip;
    }
    
    static UInt getAuditBufferFlushSize()
    {
        return mProperties->mAuditBufferFlushSize;
    }
    
    static UInt getAuditFileSize()
    {
        return mProperties->mAuditFileSize;
    }

    static SChar *getAuditLogDir()
    {
        return mProperties->mAuditLogDir;
    }

    static UInt getAuditOutputMethod()
    {
        return mProperties->mAuditOutputMethod;
    }

    static SChar *getAuditTagNameInSyslog()
    {
        return mProperties->mAuditTagNameInSyslog;
    }

    static UInt getEnableRecTest()
    {
        return mProperties->mEnableRecoveryTest;
    }

    static UInt getShowMutexLeakList()
    {
        return mProperties->mShowMutexLeakList;
    }

    static UInt getScalability4CPU()
    {
        return mProperties->mScalabilityPerCPU;
    }

    static UInt getMaxScalability()
    {
        return mProperties->mMaxScalability;
    }

    static UInt getNetConnIpStack()
    {
        return mProperties->mNetConnIpStack;
    }
    
    static UInt getRpNetConnIpStack()
    {
        return mProperties->mRpNetConnIpStack;
    }

    /*
     * PROJ-2118 Bug Reporting
     */
    static UInt getErrorValidationLevel()
    {
        return mProperties->mErrorValidationLevel;
    }

    static idBool getWriteErrorTrace()
    {
        if( mProperties->mWriteErrorTrace == 1 )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    static UInt getWritePstack()
    {
        if ( mProperties->mWritePstack == 1 )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    static UInt getUseSigAltStack()
    {
        if ( mProperties->mUseSigAltStack == 1 )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    static UInt getLogCollectCount()
    {
        return mProperties->mLogCollectCount;
    }

    static idBool getCollectDumpInfo()
    {
        if( mProperties->mCollectDumpInfo == 1 )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    static idBool getWriteWindowsMinidump()
    {
        if( mProperties->mWriteWindowsMinidump == 1 )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    static SChar *getMemDBDir()
    {
        return mProperties->mMemDBDir;
    }

    static SChar *getDefaultDiskDBDir()
    {
        return mProperties->mDefaultDiskDBDir;
    }

    static SChar *getLogDir()
    {
        return mProperties->mLogDir;
    }

    static SChar *getArchiveDir()
    {
        return mProperties->mArchiveDir;
    }

    static SChar *getServerMsglogDir()
    {
        return mProperties->mServerMsglogDir;
    }

    static SChar *getLogAnchorDir()
    {
        return mProperties->mLogAnchorDir;
    }

    static UInt getShmMemoryPolicy()
    {
        return mProperties->mShmMemoryPolicy;
    }

    static ULong getShmMaxSize()
    {
        return mProperties->mShmMaxSize;
    }

    static ULong getShmLock()
    {
        return mProperties->mShmLock;
    }

    static ULong getShmAlignSize()
    {
        return mProperties->mShmAlignSize;
    }

    static UInt getShmChunkSize()
    {
        return mProperties->mShmChunkSize;
    }

    static UInt getStartUpShmSize()
    {
        return mProperties->mShmStartUpSize;
    }

    static UInt getShmDBKey()
    {
        return mProperties->mShmDBKey;
    }

    static UInt getShmLogging()
    {
        return mProperties->mShmLogging;
    }

    static UInt getShmLatchSpinLockCount()
    {
        return mProperties->mShmLatchSpinLockCount;
    }

    static UInt getShmLatchYieldCount()
    {
        return mProperties->mShmLatchYieldCount;
    }

    static UInt getShmLatchMaxYieldCount()
    {
        return mProperties->mShmLatchMaxYieldCount;
    }

    static UInt getShmLatchSleepDuration()
    {
        return mProperties->mShmLatchSleepDuration;
    }

    static idBool getUserProcessCpuAffinity()
    {
        if( mProperties->mUserProcessCpuAffinity == 1 )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    
    // BUG-32293
    static ULong getQpMemChunkSize()
    {
        return mProperties->mQpMemChunkSize;
    }

    static UInt getMemoryAllocatorUsePrivate()
    {
        return mProperties->mMemAllocatorUsePrivate;
    }

    static ULong getMemoryPrivatePoolSize()
    {
        return mProperties->mMemPrivatePoolSize;
    }

    // BUG-36203
    static UInt getQueryHashStringLengthMax()
    {
        return mProperties->mQueryHashStringLengthMax;
    }

    /* PROJ-2473 SNMP 지원 */
    static UInt getPortNo()
    {
        return mProperties->mPortNo;
    }

    static UInt getSNMPEnable()
    {
        return mProperties->mSNMPEnable;
    }
    static UInt getSNMPPortNo()
    {
        return mProperties->mSNMPPortNo;
    }
    static UInt getSNMPTrapPortNo()
    {
        return mProperties->mSNMPTrapPortNo;
    }
    static UInt getSNMPRecvTimeout()
    {
        return mProperties->mSNMPRecvTimeout;
    }
    static UInt getSNMPSendTimeout()
    {
        return mProperties->mSNMPSendTimeout;
    }
    static UInt getSNMPTrcFlag()
    {
        return mProperties->mSNMPTrcFlag;
    }

    static UInt getSNMPAlarmQueryTimeout()
    {
        return mProperties->mSNMPAlarmQueryTimeout;
    }
    static UInt getSNMPAlarmUtransTimeout()
    {
        return mProperties->mSNMPAlarmUtransTimeout;
    }
    static UInt getSNMPAlarmFetchTimeout()
    {
        return mProperties->mSNMPAlarmFetchTimeout;
    }
    static UInt getSNMPAlarmSessionFailureCount()
    {
        return mProperties->mSNMPAlarmSessionFailureCount;
    }
    static void setSNMPAlarmQueryTimeout(UInt aValue)
    {
        mProperties->mSNMPAlarmQueryTimeout = aValue;
    }
    static void setSNMPAlarmUtransTimeout(UInt aValue)
    {
        mProperties->mSNMPAlarmUtransTimeout = aValue;
    }
    static void setSNMPAlarmFetchTimeout(UInt aValue)
    {
        mProperties->mSNMPAlarmFetchTimeout = aValue;
    }
    static void setSNMPAlarmSessionFailureCount(UInt aValue)
    {
        mProperties->mSNMPAlarmSessionFailureCount = aValue;
    }

    // TASK-6327
    static ULong getDiskMaxDBSize()
    {
        return mProperties->mDiskMaxDBSize;
    }

    // BUG-40492
    static UInt   getMinMemPoolSlotCount()
    {
        return mProperties->mMinMemPoolChunkSlotCnt;
    }

    // BUG-40819
    static UInt getMaxClient()
    {
        return mProperties->mMaxClient;
    }

    /*
     * BUG-40838 
     * it's only called by mmuSharedProperty to adjust max client value.
     * just work around. 
     * sooner or later this function will be deleted.
     */
    static void setMaxClient( UInt aValue )
    {
        mProperties->mMaxClient = aValue;
    }

    // BUG-40819
    static UInt getJobThreadCount()
    {
        return mProperties->mJobThreadCount;
    }

    // BUG-40819
    static UInt getConcExecDegreeMax()
    {
        return mProperties->mConcExecDegreeMax;
    }

    // BUG-40819
    static UInt getExtprocAgentCallRetryCount()
    {
        return mProperties->mExtprocAgentCallRetryCount;
    }

    static UInt getInspectionLargeHeapThresholdInitialized()
    {
        return mProperties->mInspectionLargeHeapThresholdInitialized;
    }

    static UInt getInspectionLargeHeapThreshold()
    {
        return mProperties->mInspectionLargeHeapThreshold;
    }

    // fix BUG-21266
    static UInt getDefaultThreadStackSize()
    {
        return mProperties->mDefaultThreadStackSize;
    }

    // fix BUG-21547
    static UInt getUseMemoryPool()
    {
        return mProperties->mUseMemoryPool;
    }

    static idBool getUseDumpCallstacks()
    {
        return ( mProperties->mUseDumpCallstacks == 1 ) ? ID_TRUE : ID_FALSE;
    }

    // PROJ-1685
    static UInt getExtprocAgentConnectTimeout()
    {
        return mProperties->mExtprocAgentConnectTimeout;
    }

    static UInt getExtprocAgentIdleTimeout()
    {
        return mProperties->mExtprocAgentIdleTimeout;
    }

    /* PROJ-2617 */
    static idBool getFaultToleranceEnable()
    {
        return (mProperties->mFaultToleranceEnable == 1) ? ID_TRUE : ID_FALSE;
    }

    // BUG-44652 Socket file path of EXTPROC AGENT could be set by property.
    static SChar * getExtprocAgentSocketFilepath()
    {
        return mProperties->mExtprocAgentSocketFilepath;
    }

    static idBool getThreadReuseEnable()
    {
        return ( mProperties->mThreadReuseEnable == 1 ) ? ID_TRUE : ID_FALSE;
    }

    /* BUG-46892 */
    static ULong getMathTempMemMax()
    {
        return mProperties->mMathTempMemMax;
    }

private:
    struct iduPropertyStore
    {
        /* BUG-20789
         * SM 소스에서 contention이 발생할 수 있는 부분에
         * 다중화를 할 경우 CPU하나당 몇 클라이언트를 예상할 지를 나타내는 상수
         * SM_SCALABILITY는 CPU개수 곱하기 이값이다. */
        UInt    mScalabilityPerCPU;

        UInt    mMaxScalability;

        UInt    mNetConnIpStack;

        UInt    mRpNetConnIpStack;

        /*
         * PROJ-2118 Bug Reporting
         */
        UInt    mErrorValidationLevel;
        UInt    mWriteErrorTrace;
        UInt    mWritePstack;
        UInt    mUseSigAltStack;
        UInt    mLogCollectCount;
        UInt    mCollectDumpInfo;
        UInt    mWriteWindowsMinidump;

        // fix BUG-21266
        UInt    mDefaultThreadStackSize;

        // fix BUG-21547
        UInt    mUseMemoryPool;

    	UInt    mUseDumpCallstacks; //BUG-45182

        idBool  mInspectionLargeHeapThresholdInitialized;

        UInt    mXLatchUseSignal;
        UInt    mMutexType;
        UInt    mCheckMutexDurationTimeEnable;
        UInt    mTimedStatistics;
        UInt    mLatchType;
        UInt    mMutexSpinCount;
        UInt    mNativeMutexSpinCount;
        UInt    mNativeMutexSpinCount4Logging;
        UInt    mLatchSpinCount;
        UInt    mProtocolDump;
        UInt    mServerTrcFlag;
        UInt    mSmTrcFlag;
        UInt    mQpTrcFlag;
        UInt    mJobTrcFlag;    /* BUG-49108 */
        UInt    mSdTrcFlag;     /* BUG-46138 */
        UInt    mSdTrcEnable;   /* BUG-46138 */
        UInt    mRpTrcFlag;
        UInt    mRpConflictTrcFlag;
        UInt    mRpConflictTrcEnable;
        UInt    mDkTrcFlag;
        UInt    mXaTrcFlag;     // bug-24840 divide xa log
        UInt    mCmTrcFlag;     /* BUG-41909 */
        UInt    mLbTrcFlag;     /* BUG-45274 */
        UInt    mMmTrcFlag;     /* BUG-45369 */
        UInt    mSourceInfo;
        UInt    mAllMsglogFlush;
        // direct io enabled
        UInt    mDirectIOEnabled;

        // Query Prepare Memory Maxmimum Limit Size.
        ULong   mQMPMemMaxSize;

        // Query Execute Memory Maximum Limit Size.
        ULong   mQMXMemMaxSize;

        // Query Profile
        UInt    mQueryProfFlag;
        UInt    mQueryProfBufSize;
        UInt    mQueryProfBufFullSkip;
        UInt    mQueryProfBufFlushSize;
        UInt    mQueryProfFileSize;
        /* BUG-36806 */
        SChar   mQueryProfLogDir[ ID_MAX_FILE_NAME ];

        /* PROJ-2223 Altibase Auditing */
        UInt    mAuditBufferSize;
        UInt    mAuditBufferFullSkip;
        UInt    mAuditBufferFlushSize;
        UInt    mAuditFileSize;
        SChar   mAuditLogDir[ ID_MAX_FILE_NAME ];

        /* BUG-39760 Enable AltiAudit to write log into syslog */
        UInt    mAuditOutputMethod;
        SChar   mAuditTagNameInSyslog[ IDP_MAX_PROP_STRING_LEN ];

        // Direct I/O시 file offset및 data size를 Align할 Page크기
        UInt    mDirectIOPageSize;
        UInt    mEnableRecoveryTest;

        /*
         * BUG-21487    Mutex Leak List출력을 property화 해야합니다.
         */
        UInt    mShowMutexLeakList;

        UInt    mMutexPoolMaxSize;

        SChar   mMemDBDir[ ID_MAX_FILE_NAME ];
        SChar   mDefaultDiskDBDir[ ID_MAX_FILE_NAME ];
        SChar   mLogDir[ ID_MAX_FILE_NAME ];
        SChar   mArchiveDir[ ID_MAX_FILE_NAME ];
        SChar   mServerMsglogDir[ ID_MAX_FILE_NAME ];
        SChar   mLogAnchorDir[ ID_MAX_FILE_NAME ];
        SChar   mXLogDir[ ID_MAX_FILE_NAME ];

        /* PROJ-2473 SNMP 지원 */
        UInt    mPortNo;

        UInt    mSNMPEnable;
        UInt    mSNMPPortNo;
        UInt    mSNMPTrapPortNo;
        UInt    mSNMPRecvTimeout;
        UInt    mSNMPSendTimeout;
        UInt    mSNMPTrcFlag;

        UInt    mSNMPAlarmQueryTimeout;
        UInt    mSNMPAlarmUtransTimeout;
        UInt    mSNMPAlarmFetchTimeout;
        UInt    mSNMPAlarmSessionFailureCount;

        ULong   mDiskMaxDBSize;

        // BUG-40492 iduMemPool의 Chunk 당 Slot Size 최소값
        UInt    mMinMemPoolChunkSlotCnt;

        UInt    mShmMemoryPolicy;
        ULong   mShmMaxSize;
        UInt    mShmLock;
        UInt    mShmAlignSize;
        UInt    mShmChunkSize;
        UInt    mShmStartUpSize;
        UInt    mShmDBKey;

        UInt    mShmLogging;
        UInt    mShmLatchSpinLockCount;
        UInt    mShmLatchYieldCount;
        UInt    mShmLatchMaxYieldCount;
        UInt    mShmLatchSleepDuration;
        UInt    mUserProcessCpuAffinity;

        // BUG-40819
        UInt    mMaxClient;
        UInt    mJobThreadCount;
        UInt    mConcExecDegreeMax;
        UInt    mExtprocAgentCallRetryCount;

        UInt    mLatchMinSleep;
        UInt    mLatchMaxSleep;
        UInt    mLatchSleepType;
        // To Fix PR-13963
        UInt    mInspectionLargeHeapThreshold;

        // PROJ-1685
        UInt    mExtprocAgentConnectTimeout;

        UInt    mExtprocAgentIdleTimeout;

        // BUG-32293
        ULong   mQpMemChunkSize;

        UInt    mMemAllocatorUsePrivate;
        ULong   mMemPrivatePoolSize;

        // BUG-36203
        UInt    mQueryHashStringLengthMax;

        /* PROJ-2617 */
        UInt    mFaultToleranceEnable;
        UInt    mFaultToleranceTrace;

        // BUG-44652 Socket file path of EXTPROC AGENT could be set by property.
        SChar * mExtprocAgentSocketFilepath;

        UInt    mThreadReuseEnable;

        /* BUG-46892 */
        ULong   mMathTempMemMax;
    };

    static iduPropertyStore *mProperties;
    static iduPropertyStore  mStaticProperties;
};
#endif /*  O_IDU_PROPERTY_H */
