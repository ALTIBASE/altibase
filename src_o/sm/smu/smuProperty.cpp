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
 

/*****************************************************************************
 * $Id:
 ****************************************************************************/
#include <smuProperty.h>
#include <smErrorCode.h>
#include <smx.h>
#include <sdt.h>
#include <smiTrans.h>
#include <sma.h>
#include <sdpst.h>

extern smrChkptThread gSmrChkptThread;

idBool smuProperty::mIsLoaded = ID_FALSE;

//sdd
UInt  smuProperty::mOpenDataFileWaitInterval;
UInt  smuProperty::mMaxOpenDataFileCount;
UInt  smuProperty::mBackupDataFileEndWaitInterval;
ULong smuProperty::mDataFileWriteUnitSize;
UInt  smuProperty::mMaxOpenFDCount4File;

//sdb
//UInt smuProperty::mValidateBuffer;
UInt smuProperty::mUseDWBuffer;
UInt smuProperty::mDPathBuffFThreadSyncInterval;
UInt smuProperty::mBulkIOPageCnt4DPInsert;
UInt smuProperty::mDPathBuffPageCnt;
UInt smuProperty::mDBFileMultiReadCnt;
UInt smuProperty::mSmallTblThreshold;
UInt smuProperty::mFlusherBusyConditionCheckInterval;

/* PROJ-2669 */
UInt smuProperty::mDelayedFlushListPct;
UInt smuProperty::mDelayedFlushProtectionTimeMsec;

//PROJ-1568 begin
UInt smuProperty::mHotTouchCnt;
UInt smuProperty::mBufferVictimSearchInterval;
UInt smuProperty::mBufferVictimSearchPct;
UInt smuProperty::mHotListPct;
UInt smuProperty::mBufferHashBucketDensity;
UInt smuProperty::mBufferHashChainLatchDensity;
UInt smuProperty::mBufferHashPermute1;
UInt smuProperty::mBufferHashPermute2;
UInt smuProperty::mBufferLRUListCnt;
UInt smuProperty::mBufferFlushListCnt;
UInt smuProperty::mBufferPrepareListCnt;
UInt smuProperty::mBufferCheckPointListCnt;
UInt smuProperty::mBufferFlusherCnt;
UInt smuProperty::mBufferIOBufferSize;
ULong smuProperty::mBufferAreaSize;
ULong smuProperty::mBufferAreaChunkSize;
//BUG-48042
UInt smuProperty::mBuffAreaCreateParDeg;
//UInt smuProperty::mBufferPinningCount;
//UInt smuProperty::mBufferPinningHistoryCount;
UInt smuProperty::mDefaultFlusherWaitSec;
UInt smuProperty::mMaxFlusherWaitSec;
ULong smuProperty::mCheckpointFlushCount;
ULong smuProperty::mFastStartIoTarget;
UInt smuProperty::mFastStartLogFileTarget;
UInt smuProperty::mLowPreparePCT;
UInt smuProperty::mHighFlushPCT;
UInt smuProperty::mLowFlushPCT;
UInt smuProperty::mTouchTimeInterval;
UInt smuProperty::mCheckpointFlushMaxWaitSec;
UInt smuProperty::mCheckpointFlushMaxGap;
UInt smuProperty::mBlockAllTxTimeOut;
//proj-1568 end

UInt smuProperty::mCheckpointFlushResponsibility;

// PROJ-2068 Direct-Path INSERT 성능 개선
SLong smuProperty::mDPathBuffPageAllocRetryUSec;
idBool smuProperty::mDPathInsertEnable;

ULong smuProperty::mReservedDiskSizeForLogFile;

/*********************************************************************
 * PROJ-2201 Innovation in sorting and hashing(temp)
 *********************************************************************/
ULong   smuProperty::mHashAreaSize;
UInt    smuProperty::mTempSortPartitionSize;
ULong   smuProperty::mInitTotalWASize;
ULong   smuProperty::mMaxTotalWASize;
UInt    smuProperty::mTmpMinInitWAExtCnt;
UInt    smuProperty::mTmpOverInitWAExtCnt;
UInt    smuProperty::mTmpCheckUnique4Update;
UInt    smuProperty::mTmpInitWASegCnt;
UInt    smuProperty::mTempRowSplitThreshold;
UInt    smuProperty::mTempSortGroupRatio;
UInt    smuProperty::mTempHashGroupRatio;
UInt    smuProperty::mTempSubHashGroupRatio;
UInt    smuProperty::mTempSleepInterval;
UInt    smuProperty::mTempAllocTryCount;
UInt    smuProperty::mTempMaxKeySize;
UInt    smuProperty::mTempStatsWatchArraySize;
UInt    smuProperty::mTempStatsWatchTime;
SChar * smuProperty::mTempDumpDirectory;
UInt    smuProperty::mTempDumpLevel;
idBool  smuProperty::mWCBCleanMemset;
UInt    smuProperty::mSmTempOperAbort;
UInt    smuProperty::mTempHashBucketDensity;

UInt    smuProperty::mTmpAllocWAExtCnt;
UInt    smuProperty::mTmpHashFetchSubHashMaxRatio;

//sdp
#if 0
ULong smuProperty::mTransWaitTime4TTS;
#endif
ULong smuProperty::mTransWaitTime;
UInt smuProperty::mTmsIgnoreHintPID;
SInt smuProperty::mTmsManualSlotNoInItBMP;
SInt smuProperty::mTmsManualSlotNoInLfBMP;
UInt smuProperty::mTmsCandidateLfBMPCnt;
UInt smuProperty::mTmsCandidatePageCnt;
UInt smuProperty::mTmsMaxSlotCntPerRtBMP;
UInt smuProperty::mTmsMaxSlotCntPerItBMP;
UInt smuProperty::mTmsMaxSlotCntPerExtDir;

UInt smuProperty::mTmsDelayedAllocHintPageArr;
UInt smuProperty::mTmsHintPageArrSize;

UInt smuProperty::mSegStoInitExtCnt;
UInt smuProperty::mSegStoNextExtCnt;
UInt smuProperty::mSegStoMinExtCnt;
UInt smuProperty::mSegStoMaxExtCnt;

UInt smuProperty::mTSSegExtDescCntPerExtDir;
UInt smuProperty::mUDSegExtDescCntPerExtDir;
UInt smuProperty::mTSSegSizeShrinkThreshold;
UInt smuProperty::mUDSegSizeShrinkThreshold;
UInt smuProperty::mRetryStealCount;

UInt smuProperty::mPctFree;
UInt smuProperty::mPctUsed;
UInt smuProperty::mTableInitTrans;
UInt smuProperty::mTableMaxTrans;
UInt smuProperty::mIndexInitTrans;
UInt smuProperty::mIndexMaxTrans;

UInt smuProperty::mCheckSumMethod;
UInt smuProperty::mExtDescCntInExtDirPage;

// To verify CASE-6829
UInt smuProperty::mSMChecksumDisable;
UInt smuProperty::mCheckDiskAgerStat;

//sdn
UInt  smuProperty::mMaxTempHashTableCount;
ULong smuProperty::mSortAreaSize;
UInt  smuProperty::mMergePageCount;
UInt  smuProperty::mKeyRedistributionLowLimit;
SLong smuProperty::mMaxTraverseLength;
UInt  smuProperty::mUnbalancedSplitRate;

// BUG-29506 TBT가 TBK로 전환시 변경된 offset을 CTS에 반영하지 않습니다.
// 재현하기 위해 CTS 할당 여부를 임의로 제어하기 위한 PROPERTY를 추가
UInt  smuProperty::mDisableTransactionBoundInCTS;

// BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
// 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
UInt  smuProperty::mManualBindingTXSegByEntryID;

// PROJ-1591
UInt  smuProperty::mRTreeSplitMode;
UInt  smuProperty::mRTreeSplitRate;
UInt  smuProperty::mRTreeMaxKeyCount;

#if 0 // not used
UInt smuProperty::mTssCntPctToBufferPool;
#endif
// To verify CASE-6829
UInt smuProperty::mSMAgerDisable;



//smm
UInt   smuProperty::mDirtyPagePool;
SChar* smuProperty::mDBName;
ULong  smuProperty::mMaxDBSize;
ULong  smuProperty::mDefaultMemDBFileSize;
UInt   smuProperty::mMemDPFlushWaitTime;
UInt   smuProperty::mMemDPFlushWaitSID;
UInt   smuProperty::mMemDPFlushWaitPID;
UInt   smuProperty::mMemDPFlushWaitOffset;


UInt   smuProperty::mIOType;
UInt   smuProperty::mSyncCreate;
UInt   smuProperty::mCreateMethod;
UInt   smuProperty::mTempPageChunkCount;
ULong  smuProperty::mSCNSyncInterval;

UInt   smuProperty::mCheckStartupVersion;
UInt   smuProperty::mCheckStartupBitMode;
UInt   smuProperty::mCheckStartupEndian;
UInt   smuProperty::mCheckStartupLogSize;

ULong  smuProperty::mShmChunkSize;

UInt   smuProperty::mRestoreMethod;
UInt   smuProperty::mRestoreThreadCount;
UInt   smuProperty::mRestoreAIOCount;
UInt   smuProperty::mRestoreBufferPageCount;
UInt   smuProperty::mCheckpointAIOCount;

// 하나의 Expand Chunk에 속하는 Page수
UInt  smuProperty::mExpandChunkPageCount;

// 다음과 같은 Page List들을 각각 몇개의 List로 다중화 할 지 결정한다.
//
// 데이터베이스 Free Page List
// 테이블의 Allocated Page List
// 테이블의 Free Page List
UInt   smuProperty::mPageListGroupCount;

// Expand Chunk확장시에 Free Page들이 여러번에 걸쳐서
// 다중화된 Free Page List로 분배된다.
//
// 이 때, 한번에 몇개의 Page를 Free Page List로 분배할지를 설정한다.
UInt   smuProperty::mPerListDistPageCount;

// Free Page List가 분할 되기 위해 가져야 하는 최소한의 Page수
UInt   smuProperty::mMinPagesOnDBFreeList;

UInt   smuProperty::mSeparateDicTBSSizeEnable;

//smr
UInt smuProperty::mChkptBulkWritePageCount;
UInt smuProperty::mChkptBulkWriteSleepSec;
UInt smuProperty::mChkptBulkWriteSleepUSec;
UInt smuProperty::mChkptBulkSyncPageCount;

UInt smuProperty::mSMStartupTest;
UInt smuProperty::mAfterCare;
UInt smuProperty::mCheckLogFileWhenArchThrAbort;

UInt smuProperty::mChkptEnabled;
UInt smuProperty::mChkptIntervalInSec;
UInt smuProperty::mChkptIntervalInLog;

UInt smuProperty::mSyncIntervalSec;
UInt smuProperty::mSyncIntervalMSec;

/* BUG-35392 */
UInt smuProperty::mLFThrSyncWaitMin;
UInt smuProperty::mLFThrSyncWaitMax;
UInt smuProperty::mLFGMgrSyncWaitMin;
UInt smuProperty::mLFGMgrSyncWaitMax;

SChar* smuProperty::mLogAnchorDir[SM_LOGANCHOR_FILE_COUNT];
SChar* smuProperty::mArchiveDirPath;
UInt   smuProperty::mArchiveFullAction;
UInt   smuProperty::mArchiveThreadAutoStart;
UInt   smuProperty::mArchiveMultiplexCount;
UInt   smuProperty::mArchiveMultiplexDirCount;
SChar* smuProperty::mArchiveMultiplexDirPath[ SM_ARCH_MULTIPLEX_PATH_CNT];

SChar* smuProperty::mLogDirPath;
UInt   smuProperty::mLogMultiplexCount;
UInt   smuProperty::mLogMultiplexDirCount;
SChar* smuProperty::mLogMultiplexDirPath[SM_LOG_MULTIPLEX_PATH_MAX_CNT];
UInt   smuProperty::mLogMultiplexThreadSpinCount;
UInt   smuProperty::mFileInitBufferSize;
ULong  smuProperty::mLogFileSize;
UInt   smuProperty::mZeroSizeLogFileAutoDelete;
UInt   smuProperty::mLogFilePrepareCount;
idBool smuProperty::mUseTempForPrepareLogFile;
UInt   smuProperty::mLogFilePreCreateInterval;
//UInt   smuProperty::mMaxKeepLogFile;
UInt   smuProperty::mShmDBKey;
UInt   smuProperty::mShmPageCountPerKey;

UInt   smuProperty::mLogBufferType;

UInt   smuProperty::mMinLogRecordSizeForCompress;

UInt   smuProperty::mLogAllocMutexType;
UInt   smuProperty::mLogReadMethodType;
/* BUG-35392 */
idBool smuProperty::mFastUnlockLogAllocMutex;

UInt   smuProperty::mLogCompAcceleration;

// TASK-2398 Log Compress
// 압축 해제되어 해싱된 Disk Log의 내용을 저장해둘 버퍼의 크기
ULong  smuProperty::mDiskRedoLogDecompressBufferSize;

UInt   smuProperty::mLFGGroupCommitUpdateTxCount;
UInt   smuProperty::mLFGGroupCommitIntervalUSec;
UInt   smuProperty::mLFGGroupCommitRetryUSec;

UInt   smuProperty::mLogIOType;

UInt   smuProperty::mMinCompressionResourceCount;
ULong  smuProperty::mCompressionResourceGCSecond;

/*PROJ-2162 RestartRiskReduction Begin */
UInt   smuProperty::mSmEnableStartupBugDetector;
UInt   smuProperty::mSmMtxRollbackTest;
UInt   smuProperty::mEmergencyStartupPolicy;
UInt   smuProperty::mCrashTolerance;
UInt   smuProperty::mSmIgnoreLog4EmergencyCount = 0;
UInt   smuProperty::mSmIgnoreLFGID4Emergency[SMU_IGNORE4EMERGENCY_COUNT];
UInt   smuProperty::mSmIgnoreFileNo4Emergency[SMU_IGNORE4EMERGENCY_COUNT];
UInt   smuProperty::mSmIgnoreOffset4Emergency[SMU_IGNORE4EMERGENCY_COUNT];
UInt   smuProperty::mSmIgnorePage4EmergencyCount = 0;
ULong  smuProperty::mSmIgnorePage4Emergency[SMU_IGNORE4EMERGENCY_COUNT];
UInt   smuProperty::mSmIgnoreTable4EmergencyCount = 0;
ULong  smuProperty::mSmIgnoreTable4Emergency[SMU_IGNORE4EMERGENCY_COUNT];
UInt   smuProperty::mSmIgnoreIndex4EmergencyCount = 0;
UInt   smuProperty::mSmIgnoreIndex4Emergency[SMU_IGNORE4EMERGENCY_COUNT];
/*PROJ-2162 RestartRiskReduction End */

/* PROJ-2133 Incremental backup Begin */
UInt   smuProperty::mBackupInfoRetentionPeriod;
UInt   smuProperty::mBackupInfoRetentionPeriodForTest;
UInt   smuProperty::mIncrementalBackupChunkSize;
UInt   smuProperty::mBmpBlockBitmapSize;
UInt   smuProperty::mCTBodyExtCnt;
UInt   smuProperty::mIncrementalBackupPathMakeABSPath;
/* PROJ-2133 Incremental backup End */

/* BUG-38515 Add hidden property for skip scn check in startup */
UInt   smuProperty::mSkipCheckSCNInStartup;

//sdr
UInt   smuProperty::mCorruptPageErrPolicy;

//smp
UInt smuProperty::mMinPagesOnTableFreeList;
UInt smuProperty::mAllocPageCount;
UInt smuProperty::mMemSizeClassCount;

//smc
UInt    smuProperty::mTableBackupFileBufferSize;
UInt    smuProperty::mCheckDiskIndexIntegrity;
UInt    smuProperty::mVerifyDiskIndexCount;
SChar * smuProperty::mVerifyDiskIndexName[SMU_MAX_VERIFY_DISK_INDEX_COUNT];
UInt    smuProperty::mDiskIndexNameCntToVerify;
UInt    smuProperty::mIgnoreMemTbsMaxSize;
UInt    smuProperty::mSeqCacheUptTxEnable;

/* BUG-39679 */
UInt    smuProperty::mEnableRowTemplate;

// sma
UInt smuProperty::mDeleteAgerCount;
UInt smuProperty::mAgerListCount;
UInt smuProperty::mLogicalAgerCount;
UInt smuProperty::mMinLogicalAgerCount;
UInt smuProperty::mMaxLogicalAgerCount;
UInt smuProperty::mAgerWaitMin;
UInt smuProperty::mAgerWaitMax;
UInt smuProperty::mAgerCommitInterval;
UInt smuProperty::mRefinePageCount;
UInt smuProperty::mTableCompactAtShutdown;
UInt smuProperty::mParallelLogicalAger;
UInt smuProperty::mParallelDeleteThread;

UInt smuProperty::mCatalogSlotReusable;

//sml
UInt smuProperty::mTableLockEnable;
UInt smuProperty::mTablespaceLockEnable;
SInt smuProperty::mDDLLockTimeOut;
UInt smuProperty::mLockNodeCacheCount; /* BUG-43408 */
UInt smuProperty::mSkipLockedTableAtFixedTable;

/* PROJ-2734 */
idBool smuProperty::mDistributionDeadlockEnable;
ULong  smuProperty::mDistributionDeadlockRiskLowWaitTime;
ULong  smuProperty::mDistributionDeadlockRiskMidWaitTime;
ULong  smuProperty::mDistributionDeadlockRiskHighWaitTime;

//smn
UInt smuProperty::mIndexBuildMinRecordCount;
UInt smuProperty::mIndexBuildThreadCount;
UInt smuProperty::mParallelLoadFactor;
UInt smuProperty::mIteratorMemoryParallelFactor;
//UInt smuProperty::mIndexStatParallelFactor;
UInt smuProperty::mIndexRebuildAtStartup;
UInt smuProperty::mIndexRebuildParallelFactorAtSartup;
UInt smuProperty::mMMDBDefIdxType;
ULong smuProperty::mMemoryIndexBuildRunSize;
ULong smuProperty::mMemoryIndexBuildValueLengthThreshold;
SInt smuProperty::mIndexRebuildParallelMode;
UInt smuProperty::mMemBtreeNodeSize;
UInt smuProperty::mMemBtreeDefaultMaxKeySize;
UInt smuProperty::mMemoryIndexUnbalancedSplitRate;
SInt smuProperty::mMemIndexKeyRedistribution;
SInt smuProperty::mMemIndexKeyRedistributionStandardRate;
idBool smuProperty::mScanlistMoveNonBlock;
idBool smuProperty::mIsCPUAffinity;
idBool smuProperty::mGatherIndexStatOnDDL;
idBool smuProperty::mSkipIdxStatNodeBound;

//smx
UInt  smuProperty::mRebuildMinViewSCNInterval;
UInt  smuProperty::mOIDListCount;
UInt  smuProperty::mTransTouchPageCntByNode;
UInt  smuProperty::mTransTouchPageCacheRatio;
UInt  smuProperty::mTransTblSize;
UInt  smuProperty::mTransTableFullTrcLogCycle;
ULong smuProperty::mLockTimeOut;
UInt  smuProperty::mReplLockTimeOut;
UInt  smuProperty::mTransAllocWaitTime;
UInt  smuProperty::mPrivateBucketCount;
UInt  smuProperty::mTXSEGEntryCnt;
/* BUG-47525 */
UInt  smuProperty::mGroupCommitCnt;
UInt  smuProperty::mGroupCommitListCnt;

UInt  smuProperty::mTXSegAllocWaitTime;
UInt  smuProperty::mTxOIDListMemPoolType;
/* BUG-35392 */
UInt  smuProperty::mUCLSNChkThrInterval;
/* BUG-38515 Add hidden property for skip scn check in startup */
UInt   smuProperty::mTrcLogLegacyTxInfo;
/* BUG-40790 */
UInt   smuProperty::mLobCursorHashBucketCount;
/* BUG-47365 */
UInt   smuProperty::mLogCompResourceReuse;
UInt   smuProperty::mCompResTuneSize;
/* PROJ-2733 */
ULong  smuProperty::mVersioningMinTime;

//smu
UInt   smuProperty::mArtDecreaseVal;
SChar* smuProperty::mDBDir[SM_DB_DIR_MAX_COUNT];
UInt   smuProperty::mDBDirCount;
SChar* smuProperty::mDefaultDiskDBDir;
/* PROJ-2433 */
idBool smuProperty::mForceIndexDirectKey;
/* BUG-41121 */
UInt smuProperty::mForceIndexPersistenceMode;
/* PROJ-2733 */
UInt   smuProperty::mShardEnable;

//smi
UInt   smuProperty::mDBMSStatMethod;
UInt   smuProperty::mDBMSStatMethod4MRDB;
UInt   smuProperty::mDBMSStatMethod4VRDB;
UInt   smuProperty::mDBMSStatMethod4DRDB;

UInt   smuProperty::mDBMSStatSamplingBaseCnt;
UInt   smuProperty::mDBMSStatParallelDegree;
UInt   smuProperty::mDBMSStatGatherInternalStat;
UInt   smuProperty::mDBMSStatAutoInterval;
UInt   smuProperty::mDBMSStatAutoPercentage;

UInt   smuProperty::mDefaultSegMgmtType;
UInt   smuProperty::mDefaultExtCntForExtentGroup;
ULong  smuProperty::mSysDataTBSExtentSize;
ULong  smuProperty::mSysDataFileInitSize;
ULong  smuProperty::mSysDataFileMaxSize;
ULong  smuProperty::mSysDataFileNextSize;

ULong  smuProperty::mSysUndoTBSExtentSize;
ULong  smuProperty::mSysUndoFileInitSize;
ULong  smuProperty::mSysUndoFileMaxSize;
ULong  smuProperty::mSysUndoFileNextSize;

ULong  smuProperty::mSysTempTBSExtentSize;
ULong  smuProperty::mSysTempFileInitSize;
ULong  smuProperty::mSysTempFileMaxSize;
ULong  smuProperty::mSysTempFileNextSize;

ULong  smuProperty::mUserDataTBSExtentSize;
ULong  smuProperty::mUserDataFileInitSize;
ULong  smuProperty::mUserDataFileMaxSize;
ULong  smuProperty::mUserDataFileNextSize;

ULong  smuProperty::mUserTempTBSExtentSize;
ULong  smuProperty::mUserTempFileInitSize;
ULong  smuProperty::mUserTempFileMaxSize;
ULong  smuProperty::mUserTempFileNextSize;

UInt   smuProperty::mLockEscMemSize;
UInt   smuProperty::mIndexBuildBottomUpThreshold;
UInt   smuProperty::mTableBackupTimeOut;
/* BUG-38621 */
UInt   smuProperty::mRELPathInLog;

//svm
ULong  smuProperty::mVolMaxDBSize;

//scp
/* Proj-2059 DB Upgrade 기능 */
UInt   smuProperty::mDataPortFileBlockSize;
UInt   smuProperty::mExportColumnChainingThreshold;
UInt   smuProperty::mDataPortDirectIOEnable;

//smu
UInt   smuProperty::mPortNo;

//-------------------------------------
// To Fix PR-14783
// System Thread의 동작을 제어함.
//-------------------------------------

UInt   smuProperty::mRunMemDeleteThread;
UInt   smuProperty::mRunMemGCThread;
UInt   smuProperty::mRunBufferFlushThread;
UInt   smuProperty::mRunArchiveThread;
UInt   smuProperty::mRunCheckpointThread;
UInt   smuProperty::mRunLogFlushThread;
UInt   smuProperty::mRunLogPrepareThread;
UInt   smuProperty::mRunLogPreReadThread;

/*********************************************************************
 * TASK-6198 Samsung Smart SSD GC control
 *********************************************************************/
#ifdef ALTIBASE_ENABLE_SMARTSSD
UInt   smuProperty::mSmartSSDLogRunGCEnable;
SChar* smuProperty::mSmartSSDLogDevice;
UInt   smuProperty::mSmartSSDGCTimeMilliSec;
#endif

/* PROJ-2102 Fast Secondary Buffer */
//sds
UInt   smuProperty::mSBufferEnable;
UInt   smuProperty::mSBufferFlusherCount;
UInt   smuProperty::mMaxSBufferCPFlusherCnt;
UInt   smuProperty::mSBufferType;
ULong  smuProperty::mSBufferSize;
SChar* smuProperty::mSBufferFileDirectory;

/////////////////////////////////////////////////
// PROJ-1568 Buffer Manager Renewal
/////////////////////////////////////////////////
UInt   smuProperty::mDWDirCount;
SChar* smuProperty::mDWDir[SM_MAX_DWDIR_COUNT];

/* PROJ-2620 */
SInt   smuProperty::mLockMgrCacheNode;

void smuProperty::initialize()
{
}

void smuProperty::destroy()
{
}


IDE_RC smuProperty::load()
{

    if ( mIsLoaded == ID_FALSE )
    {
        loadForSDD();
        loadForSDB();
        loadForSDP();
        loadForSDC(); // PROJ-1665
        loadForSDN();
        loadForSDR();

        loadForSMM();
        loadForSMR();
        loadForSMP();
        loadForSMC();
        loadForSMA();
        loadForSML();

        loadForSMN();
        loadForSMX();
        loadForSMU();
        loadForSMI();
        loadForSCP(); // Proj-2059
        loadForSDS(); // PROJ-2102
        loadForUTIL();
        loadForSystemThread();
#ifdef ALTIBASE_ENABLE_SMARTSSD
        loadForSmartSSD();
#endif
        loadForLockMgr();

        registCallbacks();

        IDE_TEST( checkConstraints()
                  != IDE_SUCCESS );

        mIsLoaded = ID_TRUE;

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

void smuProperty::init4Util()
{
    /* Load된 후 호출되어야 함 */
    IDE_DASSERT( mIsLoaded == ID_TRUE );

    mBufferAreaSize = 1024*1024; // 1M
}

void smuProperty::loadForSDD()
{
    UInt i;

    IDE_ASSERT( idp::read((SChar*)"OPEN_DATAFILE_WAIT_INTERVAL",
                          &mOpenDataFileWaitInterval)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"MAX_OPEN_DATAFILE_COUNT",
                          &mMaxOpenDataFileCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"BACKUP_DATAFILE_END_WAIT_INTERVAL",
                          &mBackupDataFileEndWaitInterval)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"DATAFILE_WRITE_UNIT_SIZE",
                          &mDataFileWriteUnitSize)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"DRDB_FD_MAX_COUNT_PER_DATAFILE",
                          &mMaxOpenFDCount4File)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"DOUBLE_WRITE_DIRECTORY_COUNT",
                          &mDWDirCount)
                == IDE_SUCCESS );

    for(i = 0; i < mDWDirCount; i++)
    {
        if ( idp::readPtr("DOUBLE_WRITE_DIRECTORY",
                          (void**)&mDWDir[i],
                          i)
            != IDE_SUCCESS )
        {
            break;
        }
    }
}

void smuProperty::loadForSDB()
{

//    IDE_ASSERT( idp::read( "VALIDATE_BUFFER",
//                           &mValidateBuffer )
//                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "USE_DW_BUFFER",
                           &mUseDWBuffer )
                == IDE_SUCCESS );

    // PROJ-1566
    IDE_ASSERT( idp::read( "__DIRECT_BUFFER_FLUSH_THREAD_SYNC_INTERVAL",
                           &mDPathBuffFThreadSyncInterval )
                == IDE_SUCCESS );

    // PROJ-1568 begin
    IDE_ASSERT( idp::read( "HOT_TOUCH_CNT",
                           &mHotTouchCnt )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_VICTIM_SEARCH_INTERVAL",
                           &mBufferVictimSearchInterval )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "BUFFER_VICTIM_SEARCH_PCT",
                           &mBufferVictimSearchPct )
                == IDE_SUCCESS );

    /* PROJ-2669 begin */
    IDE_ASSERT( idp::read( "DELAYED_FLUSH_LIST_PCT",
                           &mDelayedFlushListPct )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DELAYED_FLUSH_PROTECTION_TIME_MSEC",
                           &mDelayedFlushProtectionTimeMsec )
                == IDE_SUCCESS );
    /* PROJ-2669 end */

    IDE_ASSERT( idp::read( "HOT_LIST_PCT",
                           &mHotListPct )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "BUFFER_HASH_BUCKET_DENSITY",
                           &mBufferHashBucketDensity )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_HASH_CHAIN_LATCH_DENSITY",
                           &mBufferHashChainLatchDensity )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "__BUFFER_HASH_PERMUTE1",
                           &mBufferHashPermute1 )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__BUFFER_HASH_PERMUTE2",
                           &mBufferHashPermute2 )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "BUFFER_LRU_LIST_CNT",
                           &mBufferLRUListCnt )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_FLUSH_LIST_CNT",
                           &mBufferFlushListCnt )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_PREPARE_LIST_CNT",
                           &mBufferPrepareListCnt )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_CHECKPOINT_LIST_CNT",
                           &mBufferCheckPointListCnt )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_FLUSHER_CNT",
                           &mBufferFlusherCnt )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_IO_BUFFER_SIZE",
                           &mBufferIOBufferSize )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "BUFFER_AREA_SIZE",
                           &mBufferAreaSize )
                == IDE_SUCCESS );


    IDE_ASSERT( idp::read( "BUFFER_AREA_CHUNK_SIZE",
                           &mBufferAreaChunkSize )
                == IDE_SUCCESS );
    //BUG-48042
    IDE_ASSERT( idp::read( "__BUFFAREA_CREATE_PARALLEL_DEGREE",
                           &mBuffAreaCreateParDeg )
                == IDE_SUCCESS );

//    IDE_ASSERT( idp::read( "BUFFER_PINNING_COUNT",
//                           &mBufferPinningCount )
//                == IDE_SUCCESS );

//    IDE_ASSERT( idp::read( "BUFFER_PINNING_HISTORY_COUNT",
//                           &mBufferPinningHistoryCount )
//                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DEFAULT_FLUSHER_WAIT_SEC",
                           &mDefaultFlusherWaitSec )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "MAX_FLUSHER_WAIT_SEC",
                           &mMaxFlusherWaitSec )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "CHECKPOINT_FLUSH_COUNT",
                           &mCheckpointFlushCount )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "FAST_START_IO_TARGET",
                           &mFastStartIoTarget )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "FAST_START_LOGFILE_TARGET",
                           &mFastStartLogFileTarget )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "LOW_PREPARE_PCT",
                           &mLowPreparePCT )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "HIGH_FLUSH_PCT",
                           &mHighFlushPCT )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "LOW_FLUSH_PCT",
                           &mLowFlushPCT )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "TOUCH_TIME_INTERVAL",
                           &mTouchTimeInterval )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "CHECKPOINT_FLUSH_MAX_WAIT_SEC",
                           &mCheckpointFlushMaxWaitSec )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "CHECKPOINT_FLUSH_MAX_GAP",
                           &mCheckpointFlushMaxGap )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__CHECKPOINT_FLUSH_JOB_RESPONSIBILITY",
                           &mCheckpointFlushResponsibility )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "BLOCK_ALL_TX_TIME_OUT",
                           &mBlockAllTxTimeOut)
                == IDE_SUCCESS );

    //proj-1568 end

    /* BUG-18646: Direct Path Insert시 연속된 Page에 대한 Insert 연산시 IO를
       Page단위가 아닌 여러개의 페이지를 묶어서 한번의 IO로 수행하여야 합니다. */
    IDE_ASSERT( idp::read( "BULKIO_PAGE_COUNT_FOR_DIRECT_PATH_INSERT",
                           &mBulkIOPageCnt4DPInsert )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DIRECT_PATH_BUFFER_PAGE_COUNT",
                           &mDPathBuffPageCnt ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "DB_FILE_MULTIPAGE_READ_COUNT",
                           &mDBFileMultiReadCnt ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SMALL_TABLE_THRESHOLD",
                           &mSmallTblThreshold ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__FLUSHER_BUSY_CONDITION_CHECK_INTERVAL",
                           &mFlusherBusyConditionCheckInterval ) == IDE_SUCCESS );

    // PROJ-2068 Direct-Path INSERT 성능 개선
    IDE_ASSERT( idp::read( "__DPATH_BUFF_PAGE_ALLOC_RETRY_USEC",
                           &mDPathBuffPageAllocRetryUSec )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__DPATH_INSERT_ENABLE",
                           &mDPathInsertEnable ) == IDE_SUCCESS );
}

void smuProperty::loadForSDP()
{
#ifdef DEBUG
    UInt sSlotCnt;
#endif

#if 0
    IDE_ASSERT( idp::read((SChar*)"TRANS_WAIT_TIME_FOR_TTS",
                          &mTransWaitTime4TTS  )
                == IDE_SUCCESS );
#endif
    IDE_ASSERT( idp::read((SChar*)"TRANS_WAIT_TIME",
                          &mTransWaitTime)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"DEFAULT_SEGMENT_STORAGE_INITEXTENTS",
                          &mSegStoInitExtCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"DEFAULT_SEGMENT_STORAGE_NEXTEXTENTS",
                          &mSegStoNextExtCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"DEFAULT_SEGMENT_STORAGE_MINEXTENTS",
                          &mSegStoMinExtCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"DEFAULT_SEGMENT_STORAGE_MAXEXTENTS",
                          &mSegStoMaxExtCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"TSSEG_EXTDESC_COUNT_PER_EXTDIR",
                          &mTSSegExtDescCntPerExtDir )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"UDSEG_EXTDESC_COUNT_PER_EXTDIR",
                          &mUDSegExtDescCntPerExtDir )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"TSSEG_SIZE_SHRINK_THRESHOLD",
                          &mTSSegSizeShrinkThreshold )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"UDSEG_SIZE_SHRINK_THRESHOLD",
                          &mUDSegSizeShrinkThreshold )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"RETRY_STEAL_COUNT_",
                          &mRetryStealCount )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"PCTFREE",
                          &mPctFree)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"PCTUSED",
                          &mPctUsed)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"TABLE_INITRANS",
                          &mTableInitTrans)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"TABLE_MAXTRANS",
                          &mTableMaxTrans)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"INDEX_INITRANS",
                          &mIndexInitTrans)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"INDEX_MAXTRANS",
                          &mIndexMaxTrans)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "CHECKSUM_METHOD",
                           &mCheckSumMethod )
                == IDE_SUCCESS );

    // To verify CASE-6829
    IDE_ASSERT( idp::read( "__SM_CHECKSUM_DISABLE",
                           &mSMChecksumDisable )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__FMS_EXTDESC_CNT_IN_EXTDIRPAGE",
                           &mExtDescCntInExtDirPage )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_IGNORE_HINT_PID",
                          &mTmsIgnoreHintPID)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_MANUAL_SLOT_NO_IN_ITBMP",
                          &mTmsManualSlotNoInItBMP )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_MANUAL_SLOT_NO_IN_LFBMP",
                          &mTmsManualSlotNoInLfBMP )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_CANDIDATE_LFBMP_CNT",
                          &mTmsCandidateLfBMPCnt)
                == IDE_SUCCESS );
    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_CANDIDATE_PAGE_CNT",
                          &mTmsCandidatePageCnt)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_MAX_SLOT_CNT_PER_RTBMP",
                          &mTmsMaxSlotCntPerRtBMP)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_MAX_SLOT_CNT_PER_ITBMP",
                          &mTmsMaxSlotCntPerItBMP)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_MAX_SLOT_CNT_PER_EXTDIR",
                          &mTmsMaxSlotCntPerExtDir)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_DELAYED_ALLOC_HINT_PAGE_ARRAY",
                          &mTmsDelayedAllocHintPageArr)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read((SChar*)"__DISK_TMS_HINT_PAGE_ARRAY_SIZE",
                          &mTmsHintPageArrSize)
                == IDE_SUCCESS );

    /* PROJ-2037 Treelist Segment Management */
    /* RtBMP, ItBMP, ExtDir 의 Slot Count는 페이지 크기에 따라 최대값이
     * 정해딘다. 프로퍼티에 이런 최대값보다 더 큰 값이 설정된 경우 최대값으로
     * 조정해준다. */
#ifdef DEBUG
    sSlotCnt = mTmsMaxSlotCntPerRtBMP;
    IDE_DASSERT( callbackTmsMaxSlotCntPerRtBMP( NULL,
                                                NULL,
                                                NULL,
                                                &sSlotCnt,
                                                NULL ) == IDE_SUCCESS );


    sSlotCnt = mTmsMaxSlotCntPerItBMP;
    IDE_DASSERT( callbackTmsMaxSlotCntPerItBMP( NULL,
                                                NULL,
                                                NULL,
                                                &sSlotCnt,
                                                NULL ) == IDE_SUCCESS );

    sSlotCnt = mTmsMaxSlotCntPerExtDir;
    IDE_DASSERT( callbackTmsMaxSlotCntPerExtDir( NULL,
                                                 NULL,
                                                 NULL,
                                                 &sSlotCnt,
                                                 NULL ) == IDE_SUCCESS );
#endif
}

void smuProperty::loadForSDC()
{
    ;
}

void smuProperty::loadForSDN()
{

    IDE_ASSERT(idp::read("MAX_TEMP_HASHTABLE_COUNT",
                         &mMaxTempHashTableCount)
               == IDE_SUCCESS);

    // PROJ-1595
    IDE_ASSERT(idp::read("SORT_AREA_SIZE",
                         &mSortAreaSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("DISK_INDEX_BUILD_MERGE_PAGE_COUNT",
                         &mMergePageCount)
               == IDE_SUCCESS);

    // PROJ-1628
    IDE_ASSERT(idp::read("__DISK_INDEX_KEY_REDISTRIBUTION_LOW_LIMIT",
                         &mKeyRedistributionLowLimit)
               == IDE_SUCCESS);

    // BUG-43046
    IDE_ASSERT(idp::read("__DISK_INDEX_MAX_TRAVERSE_LENGTH",
                         &mMaxTraverseLength)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("DISK_INDEX_UNBALANCED_SPLIT_RATE",
                         &mUnbalancedSplitRate)
               == IDE_SUCCESS);

    // BUG-29506 TBT가 TBK로 전환시 변경된 offset을 CTS에 반영하지 않습니다.
    // 재현하기 위해 CTS 할당 여부를 임의로 제어하기 위한 PROPERTY를 추가
    IDE_ASSERT(idp::read("__DISABLE_TRANSACTION_BOUND_IN_CTS",
                         &mDisableTransactionBoundInCTS)
               == IDE_SUCCESS);

    // BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
    // 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
    IDE_ASSERT(idp::read("__MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID",
                         &mManualBindingTXSegByEntryID)
               == IDE_SUCCESS);

    // PROJ-1591
    IDE_ASSERT(idp::read("__DISK_INDEX_RTREE_SPLIT_MODE",
                         &mRTreeSplitMode)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DISK_INDEX_RTREE_SPLIT_RATE",
                         &mRTreeSplitRate)
               == IDE_SUCCESS);
    IDE_ASSERT(idp::read("__DISK_INDEX_RTREE_MAX_KEY_COUNT",
                         &mRTreeMaxKeyCount)
               == IDE_SUCCESS);
}

void smuProperty::loadForSDR()
{
    IDE_ASSERT( idp::read("CORRUPT_PAGE_ERR_POLICY",
                          &mCorruptPageErrPolicy )
                == IDE_SUCCESS );
}

void smuProperty::loadForSMM()
{
    IDE_ASSERT(idp::read("DIRTY_PAGE_POOL",
                         &mDirtyPagePool)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::readPtr("DB_NAME",
                            (void **)&mDBName)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MEM_MAX_DB_SIZE",
                         &mMaxDBSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("VOLATILE_MAX_DB_SIZE",
                         &mVolMaxDBSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("DEFAULT_MEM_DB_FILE_SIZE",
                         &mDefaultMemDBFileSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__MEM_DPFLUSH_WAIT_TIME",
                         &mMemDPFlushWaitTime)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__MEM_DPFLUSH_WAIT_SPACEID",
                         &mMemDPFlushWaitSID)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__MEM_DPFLUSH_WAIT_PAGEID",
                         &mMemDPFlushWaitPID)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__MEM_DPFLUSH_WAIT_OFFSET",
                         &mMemDPFlushWaitOffset)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("DATABASE_IO_TYPE",
                         &mIOType)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYNC_CREATE_",
                         &mSyncCreate)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LOG_CREATE_METHOD",
                         &mCreateMethod)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("TEMP_PAGE_CHUNK_COUNT",
                         &mTempPageChunkCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SCN_SYNC_INTERVAL",
                         &mSCNSyncInterval)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("CHECK_STARTUP_VERSION",
                         &mCheckStartupVersion)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("CHECK_STARTUP_BITMODE",
                         &mCheckStartupBitMode)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("CHECK_STARTUP_ENDIAN",
                         &mCheckStartupEndian)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("CHECK_STARTUP_LOGSIZE",
                         &mCheckStartupLogSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("STARTUP_SHM_CHUNK_SIZE",
                         &mShmChunkSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("RESTORE_METHOD_",
                         &mRestoreMethod)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("RESTORE_THREAD_COUNT",
                         &mRestoreThreadCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("RESTORE_AIO_COUNT",
                         &mRestoreAIOCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("RESTORE_BUFFER_PAGE_COUNT",
                         &mRestoreBufferPageCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("CHECKPOINT_AIO_COUNT",
                         &mCheckpointAIOCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("EXPAND_CHUNK_PAGE_COUNT",
                         &mExpandChunkPageCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("PAGE_LIST_GROUP_COUNT",
                         &mPageListGroupCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("PER_LIST_DIST_PAGE_COUNT",
                         &mPerListDistPageCount)
               == IDE_SUCCESS);


    IDE_ASSERT(idp::read("MIN_PAGES_ON_DB_FREE_LIST",
                         &mMinPagesOnDBFreeList)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__SEPARATE_DIC_TBS_SIZE_ENABLE",
                         &mSeparateDicTBSSizeEnable)
               == IDE_SUCCESS);
}


void smuProperty::loadForSMR()
{
    UInt    i;
    UInt    sTempValue;

    // To Fix BUG-9366
    IDE_ASSERT( idp::read("CHECKPOINT_BULK_WRITE_PAGE_COUNT",
                          &mChkptBulkWritePageCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("CHECKPOINT_BULK_WRITE_SLEEP_SEC",
                          &mChkptBulkWriteSleepSec)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("CHECKPOINT_BULK_WRITE_SLEEP_USEC",
                          &mChkptBulkWriteSleepUSec)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("CHECKPOINT_BULK_SYNC_PAGE_COUNT",
                          &mChkptBulkSyncPageCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("__AFTER_CARE",
                          &mAfterCare )
                == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "__SM_STARTUP_TEST",
                           &mSMStartupTest )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("CHECKPOINT_ENABLED",
                          &mChkptEnabled)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("CHECKPOINT_INTERVAL_IN_SEC",
                          &mChkptIntervalInSec)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("CHECKPOINT_INTERVAL_IN_LOG",
                          &mChkptIntervalInLog)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SYNC_INTERVAL_SEC_",
                          &mSyncIntervalSec)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SYNC_INTERVAL_MSEC_",
                          &mSyncIntervalMSec)
                == IDE_SUCCESS );

    /* BUG-35392 */
    IDE_ASSERT( idp::read("LFTHREAD_SYNC_WAIT_MIN",
                          &mLFThrSyncWaitMin)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LFTHREAD_SYNC_WAIT_MAX",
                          &mLFThrSyncWaitMax)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LFG_MANAGER_SYNC_WAIT_MIN",
                          &mLFGMgrSyncWaitMin)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LFG_MANAGER_SYNC_WAIT_MAX",
                          &mLFGMgrSyncWaitMax)
                == IDE_SUCCESS );

    for(i = 0; i < 3; i++)
    {
        IDE_ASSERT( idp::readPtr("LOGANCHOR_DIR",
                                 (void**)&mLogAnchorDir[i],
                                 i)
                    == IDE_SUCCESS );
    }

    IDE_ASSERT( idp::read("ARCHIVE_FULL_ACTION",
                          &mArchiveFullAction)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("ARCHIVE_THREAD_AUTOSTART",
                          &mArchiveThreadAutoStart)
                == IDE_SUCCESS );

    // To Fix PR-13786
    loadForSMR_LogFile();

#ifndef VC_WIN32
    IDE_ASSERT( idp::read("SHM_DB_KEY",
                          &mShmDBKey)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SHM_PAGE_COUNT_PER_KEY",
                          &mShmPageCountPerKey)
                == IDE_SUCCESS );
#else
    mShmDBKey           = (UInt) 0;
    mShmPageCountPerKey = (UInt) 0;
#endif

    IDE_ASSERT( idp::read("MIN_LOG_RECORD_SIZE_FOR_COMPRESS",
                          &mMinLogRecordSizeForCompress )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("DISK_REDO_LOG_DECOMPRESS_BUFFER_SIZE",
                          &mDiskRedoLogDecompressBufferSize )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LFG_GROUP_COMMIT_UPDATE_TX_COUNT",
                          &mLFGGroupCommitUpdateTxCount )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LFG_GROUP_COMMIT_INTERVAL_USEC",
                          &mLFGGroupCommitIntervalUSec )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LFG_GROUP_COMMIT_RETRY_USEC",
                          &mLFGGroupCommitRetryUSec )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LOG_IO_TYPE",
                          &mLogIOType )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("MIN_COMPRESSION_RESOURCE_COUNT",
                          &mMinCompressionResourceCount )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("COMPRESSION_RESOURCE_GC_SECOND",
                          &mCompressionResourceGCSecond )
                == IDE_SUCCESS );

    /*PROJ-2162 RestartRiskReduction Begin */
    IDE_ASSERT( idp::read("__SM_ENABLE_STARTUP_BUG_DETECTOR", 
                          &mSmEnableStartupBugDetector )
                == IDE_SUCCESS );
    IDE_ASSERT( idp::read("__SM_MTX_ROLLBACK_TEST", 
                          &mSmMtxRollbackTest )
                == IDE_SUCCESS );
    IDE_ASSERT( idp::read("__EMERGENCY_STARTUP_POLICY", 
                          &mEmergencyStartupPolicy )
                == IDE_SUCCESS );
    IDE_ASSERT( idp::read("__CRASH_TOLERANCE", 
                          &mCrashTolerance )
                == IDE_SUCCESS );

    for(i = 0; i < SMU_IGNORE4EMERGENCY_COUNT; i++)
    {
        if ( idp::read( "__SM_IGNORE_LFGID_IN_STARTUP",  
                        &mSmIgnoreLFGID4Emergency[i],
                        i )
            != IDE_SUCCESS )
        {
            break;
        }
        if ( idp::read( "__SM_IGNORE_FILENO_IN_STARTUP",  
                        &mSmIgnoreFileNo4Emergency[i],
                        i )
            != IDE_SUCCESS )
        {
            break;
        }
        if ( idp::read( "__SM_IGNORE_OFFSET_IN_STARTUP",  
                        &mSmIgnoreOffset4Emergency[i],
                        i )
            != IDE_SUCCESS )
        {
            break;
        }
    }
    mSmIgnoreLog4EmergencyCount = i;

    for(i = 0; i < SMU_IGNORE4EMERGENCY_COUNT; i++)
    {
        if ( idp::read( "__SM_IGNORE_PAGE_IN_STARTUP",  
                        &mSmIgnorePage4Emergency[i],
                        i )
            != IDE_SUCCESS )
        {
            break;
        }
    }
    mSmIgnorePage4EmergencyCount = i;

    for(i = 0; i < SMU_IGNORE4EMERGENCY_COUNT; i++)
    {
        if ( idp::read( "__SM_IGNORE_TABLE_IN_STARTUP", 
                        &mSmIgnoreTable4Emergency[i],
                        i )
            != IDE_SUCCESS )
        {
            break;
        }
    }
    mSmIgnoreTable4EmergencyCount = i;

    for(i = 0; i < SMU_IGNORE4EMERGENCY_COUNT; i++)
    {
        if ( idp::read( "__SM_IGNORE_INDEX_IN_STARTUP", 
                        &mSmIgnoreIndex4Emergency[i],
                        i )
            != IDE_SUCCESS )
        {
            break;
        }
    }
    mSmIgnoreIndex4EmergencyCount = i;
    /*PROJ-2162 RestartRiskReduction End */

    /*PROJ-2133 Incremental backup begin*/
    IDE_ASSERT( idp::read( "INCREMENTAL_BACKUP_INFO_RETENTION_PERIOD",
                           &mBackupInfoRetentionPeriod )
                == IDE_SUCCESS );
    
    IDE_ASSERT( idp::read( "__INCREMENTAL_BACKUP_INFO_RETENTION_PERIOD_FOR_TEST",
                           &mBackupInfoRetentionPeriodForTest )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "INCREMENTAL_BACKUP_CHUNK_SIZE",
                           &mIncrementalBackupChunkSize )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__INCREMENTAL_BACKUP_BMP_BLOCK_BITMAP_SIZE",
                           &mBmpBlockBitmapSize )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__INCREMENTAL_BACKUP_CTBODY_EXTENT_CNT",
                           &mCTBodyExtCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__INCREMENTAL_BACKUP_PATH_MAKE_ABS_PATH",
                           &mIncrementalBackupPathMakeABSPath )
                == IDE_SUCCESS );
    /*PROJ-2133 Incremental backup end*/

    IDE_ASSERT( idp::read( "LOG_ALLOC_MUTEX_TYPE",
                           &mLogAllocMutexType )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__LOG_READ_METHOD_TYPE",
                           &mLogReadMethodType )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__RESERVED_DISK_SIZE_FOR_LOGFILE",
                           &mReservedDiskSizeForLogFile )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__CHECK_LOGFILE_WHEN_ARCH_THR_ABORT",
                           &mCheckLogFileWhenArchThrAbort )
                == IDE_SUCCESS );

    /* BUG-35392 */
    IDE_ASSERT( idp::read( "FAST_UNLOCK_LOG_ALLOC_MUTEX",
                           &sTempValue )
                == IDE_SUCCESS );

    mFastUnlockLogAllocMutex = ( sTempValue == 1 ) ? ID_TRUE : ID_FALSE;

    /* BUG-45744 */
    IDE_ASSERT( idp::read( "__LOG_COMPRESSION_ACCELERATION",
                           &mLogCompAcceleration )
                == IDE_SUCCESS );

    /*******************************************************************
     * PROJ-2201 Innovation in sorting and hashing(temp)
     *******************************************************************/
    SMU_PROPERTY_WRITABLE_REGIST( "HASH_AREA_SIZE",
                                  HashAreaSize );
    SMU_PROPERTY_WRITABLE_REGIST( "TEMP_SORT_PARTITION_SIZE",
                                  TempSortPartitionSize );
    SMU_PROPERTY_WRITABLE_REGIST( "TEMP_SORT_GROUP_RATIO", 
                                  TempSortGroupRatio );
    SMU_PROPERTY_WRITABLE_REGIST( "INIT_TOTAL_WA_SIZE",
                                  InitTotalWASize );
    SMU_PROPERTY_WRITABLE_REGIST( "TOTAL_WA_SIZE",
                                  MaxTotalWASize );

    SMU_PROPERTY_WRITABLE_REGIST( "__TEMP_CHECK_UNIQUE_FOR_UPDATE",
                                  TmpCheckUnique4Update );
    SMU_PROPERTY_WRITABLE_REGIST( "__TEMP_MIN_INIT_WAEXTENT_COUNT",
                                  TmpMinInitWAExtCnt );
    SMU_PROPERTY_WRITABLE_REGIST( "__TEMP_OVER_INIT_WAEXTENT_COUNT",
                                  TmpOverInitWAExtCnt );
    SMU_PROPERTY_WRITABLE_REGIST( "__TEMP_INIT_WASEGMENT_COUNT",
                                  TmpInitWASegCnt );

    SMU_PROPERTY_WRITABLE_REGIST( "__TEMP_ALLOC_WAEXTENT_COUNT",
                                  TmpAllocWAExtCnt );
    SMU_PROPERTY_WRITABLE_REGIST( "__TEMP_HASH_FETCH_SUBHASH_MAX_RATIO",
                                  TmpHashFetchSubHashMaxRatio );

    SMU_PROPERTY_WRITABLE_REGIST( "TEMP_ALLOC_TRY_COUNT",
                                  TempAllocTryCount );
    SMU_PROPERTY_READONLY_REGIST( "TEMP_ROW_SPLIT_THRESHOLD",
                                  TempRowSplitThreshold );
    SMU_PROPERTY_WRITABLE_REGIST( "TEMP_HASH_GROUP_RATIO", 
                                  TempHashGroupRatio );
    SMU_PROPERTY_WRITABLE_REGIST( "__TEMP_SUBHASH_GROUP_RATIO", 
                                  TempSubHashGroupRatio );
    SMU_PROPERTY_WRITABLE_REGIST( "TEMP_SLEEP_INTERVAL", 
                                  TempSleepInterval );
    SMU_PROPERTY_READONLY_REGIST( "TEMP_MAX_KEY_SIZE",
                                  TempMaxKeySize );
    SMU_PROPERTY_READONLY_REGIST( "TEMP_STATS_WATCH_ARRAY_SIZE",
                                  TempStatsWatchArraySize );
    SMU_PROPERTY_WRITABLE_REGIST( "TEMP_STATS_WATCH_TIME",
                                  TempStatsWatchTime );
    SMU_PROPERTY_READONLY_PTR_REGIST( "TEMPDUMP_DIRECTORY",
                                      TempDumpDirectory );
    SMU_PROPERTY_WRITABLE_REGIST( "__TEMPDUMP_LEVEL",
                                  TempDumpLevel );
    SMU_PROPERTY_WRITABLE_REGIST( "__WCB_CLEAN_MEMSET",
                                  WCBCleanMemset );
    SMU_PROPERTY_WRITABLE_REGIST( "__SM_TEMP_OPER_ABORT",
                                  SmTempOperAbort );
    SMU_PROPERTY_READONLY_REGIST( "TEMP_HASH_BUCKET_DENSITY",
                                  TempHashBucketDensity );

    /* BUG-38621 */
    IDE_ASSERT( idp::read( "__RELATIVE_PATH_IN_LOG",
                           &sTempValue )
                == IDE_SUCCESS);

    mRELPathInLog = ( ( sTempValue == 1 ) ? ID_TRUE : ID_FALSE );

    /* BUG-38515 Add hidden property for skip scn check in startup */    
    IDE_ASSERT( idp::read("__SM_SKIP_CHECKSCN_IN_STARTUP",
                          &mSkipCheckSCNInStartup )
                == IDE_SUCCESS );


    
}

// To Fix PR-13786 복잡도 개선
void smuProperty::loadForSMR_LogFile()
{
    UInt i;
    UInt sValue;

    /* BUGBUG: 동일한 Log Dir이 존재하면 않된다. */
    IDE_ASSERT( idp::readPtr( "LOG_DIR",
                              (void**)&mLogDirPath ) 
                == IDE_SUCCESS );

    for(i = 0; i < SM_LOG_MULTIPLEX_PATH_MAX_CNT; i++)
    {
        if ( idp::readPtr("LOG_MULTIPLEX_DIR",
                         (void**)&mLogMultiplexDirPath[i],
                         i) != IDE_SUCCESS )
        {
            break;
        }

        if ( idlOS::strncmp( mLogMultiplexDirPath[i], "", 1) == 0 )
        {
            break;
        }
    }

    mLogMultiplexDirCount = i;

    IDE_ASSERT( idp::read("LOG_MULTIPLEX_COUNT",
                          &mLogMultiplexCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("__LOG_MULTIPLEX_THREAD_SPIN_COUNT",
                          &mLogMultiplexThreadSpinCount)
                == IDE_SUCCESS );


    /* For Parallel Logging:로그 Dir이 여러개가 됨에 따라
       각각의 로그 Dir마다
       Archive Dir이 따로 존재해야 한다. */
    /* BUGBUG: 동일한 Archive Dir이 존재하면 않된다. */
    idp::readPtr( "ARCHIVE_DIR",
                  (void**)&mArchiveDirPath ); 

    for( i = 0; i < SM_ARCH_MULTIPLEX_PATH_CNT; i++ )
    {
        if ( idp::readPtr("ARCHIVE_MULTIPLEX_DIR",
                         (void**)&mArchiveMultiplexDirPath[i],
                         i) != IDE_SUCCESS )
        {
            break;
        }

        if ( idlOS::strncmp( mArchiveMultiplexDirPath[i], "", 1) == 0 )
        {
            break;
        }
    }

    mArchiveMultiplexDirCount = i;

    IDE_ASSERT( idp::read("ARCHIVE_MULTIPLEX_COUNT",
                          &mArchiveMultiplexCount)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("LOG_BUFFER_TYPE",
                          &mLogBufferType )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("FILE_INIT_BUFFER_SIZE",
                          &mFileInitBufferSize)
                == IDE_SUCCESS );

    IDE_ASSERT(idp::read("LOG_FILE_SIZE",
                         &mLogFileSize)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read("__ZERO_SIZE_LOG_FILE_AUTO_DELETE",
                          &mZeroSizeLogFileAutoDelete)
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("PREPARE_LOG_FILE_COUNT",
                          &mLogFilePrepareCount)
                == IDE_SUCCESS );
    
    /* BUG-48409 temp를 이용한 Logfile생성 방법 추가 */
    IDE_ASSERT( idp::read("__USE_TEMP_FOR_PREPARE_LOGFILE",
                          &sValue )
                == IDE_SUCCESS );
    
    mUseTempForPrepareLogFile = ( sValue == 0 )? ID_FALSE : ID_TRUE;
    
    IDE_ASSERT( idp::read("LOGFILE_PRECREATE_INTERVAL",
                          &mLogFilePreCreateInterval)
                == IDE_SUCCESS );

    if ( idf::isVFS() == ID_TRUE )
    {
        mLogBufferType = SMU_LOG_BUFFER_TYPE_MEMORY;
    }
}


void smuProperty::loadForSMP()
{
    IDE_ASSERT(idp::read("MIN_PAGES_ON_TABLE_FREE_LIST",
                         &mMinPagesOnTableFreeList)
               == IDE_SUCCESS);
    
    /*
     * BUG-25327 : [MDB] Free Page Size Class 개수를 Property화 해야 합니다.
     */
    IDE_ASSERT(idp::read("MEM_SIZE_CLASS_COUNT",
                         &mMemSizeClassCount)
               == IDE_SUCCESS);
    
    if ( mMemSizeClassCount > SMP_MAX_SIZE_CLASS_COUNT )
    {
        mMemSizeClassCount = SMP_MAX_SIZE_CLASS_COUNT;
    }
    
    IDE_ASSERT(idp::read("TABLE_ALLOC_PAGE_COUNT",
                         &mAllocPageCount)
               == IDE_SUCCESS);

    if ( mAllocPageCount == 0 )
    {
        // TABLE_ALLOC_PAGE_COUNT 값이 0이면
        // List에 유지하고 싶은 Page 갯수만큼 DB에서 가져온다.
        mAllocPageCount = mMinPagesOnTableFreeList;
    }
}

void smuProperty::loadForSMC()
{
    UInt i;

    IDE_ASSERT(idp::read("TABLE_BACKUP_FILE_BUFFER_SIZE",
                         &mTableBackupFileBufferSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__SM_CHECK_DISK_INDEX_INTEGRITY",
                         &mCheckDiskIndexIntegrity)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__SM_VERIFY_DISK_INDEX_COUNT",
                         &mVerifyDiskIndexCount )
               == IDE_SUCCESS);

    for (i = 0; i < mVerifyDiskIndexCount; i++ )
    {
        if ( idp::readPtr( "__SM_VERIFY_DISK_INDEX_NAME",
                           (void**)&mVerifyDiskIndexName[i],
                           i )
             != IDE_SUCCESS )
        {
            break;
        }

        IDE_ASSERT( mVerifyDiskIndexName[i] != NULL );
    }

    mDiskIndexNameCntToVerify = i;

    IDE_ASSERT(idp::read("__EMERGENCY_IGNORE_MEM_TBS_MAXSIZE",
                         &mIgnoreMemTbsMaxSize )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__ENABLE_ROW_TEMPLATE",
                         &mEnableRowTemplate )
               == IDE_SUCCESS);

    //BUG-49062
    IDE_ASSERT(idp::read("__SEQ_CACHE_UPT_TX_ENABLE",
                         &mSeqCacheUptTxEnable)
               == IDE_SUCCESS);
}

void smuProperty::loadForSMA()
{


    IDE_ASSERT(idp::read("DELETE_AGER_COUNT_",
                         &mDeleteAgerCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__AGER_LIST_COUNT",
                         &mAgerListCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MAX_LOGICAL_AGER_COUNT",
                         &mMaxLogicalAgerCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("MIN_LOGICAL_AGER_COUNT",
                         &mMinLogicalAgerCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("LOGICAL_AGER_COUNT_",
                         &mLogicalAgerCount)
               == IDE_SUCCESS);


    IDE_ASSERT(idp::read("AGER_WAIT_MINIMUM",
                         &mAgerWaitMin)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("AGER_WAIT_MAXIMUM",
                         &mAgerWaitMax)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("DELETE_AGER_COMMIT_INTERVAL",
                         &mAgerCommitInterval)
               == IDE_SUCCESS);


    IDE_ASSERT( idp::read("REFINE_PAGE_COUNT",
                          &mRefinePageCount)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("TABLE_COMPACT_AT_SHUTDOWN",
                          &mTableCompactAtShutdown)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("__PARALLEL_LOGICAL_AGER",
                          &mParallelLogicalAger)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("__PARALLEL_DELETE_THREAD",
                          &mParallelDeleteThread)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read( "__CATALOG_SLOT_REUSABLE",
                           &mCatalogSlotReusable )
                == IDE_SUCCESS );

}

void smuProperty::loadForSML()
{
    IDE_ASSERT( idp::read("TABLE_LOCK_ENABLE",
                          &mTableLockEnable)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("TABLESPACE_LOCK_ENABLE",
                          &mTablespaceLockEnable)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("DDL_LOCK_TIMEOUT",
                          &mDDLLockTimeOut)
                == IDE_SUCCESS);
    IDE_ASSERT( idp::read("LOCK_NODE_CACHE_COUNT",
                          &mLockNodeCacheCount)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("__SKIP_LOCKED_TABLE_AT_FIXED_TABLE",
                          &mSkipLockedTableAtFixedTable)
                == IDE_SUCCESS);

    /* PROJ-2734 */
    IDE_ASSERT( idp::read("DISTRIBUTION_DEADLOCK_ENABLE",
                          &mDistributionDeadlockEnable)
                == IDE_SUCCESS);
    IDE_ASSERT( idp::read("DISTRIBUTION_DEADLOCK_RISK_LOW_WAIT_TIME",
                          &mDistributionDeadlockRiskLowWaitTime)
                == IDE_SUCCESS);
    IDE_ASSERT( idp::read("DISTRIBUTION_DEADLOCK_RISK_MID_WAIT_TIME",
                          &mDistributionDeadlockRiskMidWaitTime)
                == IDE_SUCCESS);
    IDE_ASSERT( idp::read("DISTRIBUTION_DEADLOCK_RISK_HIGH_WAIT_TIME",
                          &mDistributionDeadlockRiskHighWaitTime)
                == IDE_SUCCESS);
}


void smuProperty::loadForSMN()
{
    UInt  sTempValue;

    IDE_ASSERT(idp::read("INDEX_BUILD_MIN_RECORD_COUNT",
                         &mIndexBuildMinRecordCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("INDEX_BUILD_THREAD_COUNT",
                         &mIndexBuildThreadCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("PARALLEL_LOAD_FACTOR",
                         &mParallelLoadFactor)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("INDEX_REBUILD_AT_STARTUP",
                         &mIndexRebuildAtStartup)
               == IDE_SUCCESS);
    IDE_ASSERT(idp::read("INDEX_REBUILD_PARALLEL_FACTOR_AT_STARTUP",
                         &mIndexRebuildParallelFactorAtSartup)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("ITERATOR_MEMORY_PARALLEL_FACTOR",
                         &mIteratorMemoryParallelFactor)
               == IDE_SUCCESS);

//    IDE_ASSERT(idp::read("__INDEX_STAT_PARALLEL_FACTOR",
//                         &mIndexStatParallelFactor)
//               == IDE_SUCCESS);

    // PROJ-1629 Memory Index Build
    IDE_ASSERT(idp::read("MEMORY_INDEX_BUILD_RUN_SIZE",
                         &mMemoryIndexBuildRunSize)
               == IDE_SUCCESS);

    // PROJ-1629 Memory Index Build
    IDE_ASSERT(idp::read("MEMORY_INDEX_BUILD_VALUE_LENGTH_THRESHOLD",
                         &mMemoryIndexBuildValueLengthThreshold)
               == IDE_SUCCESS);

    // TASK-6006 Index Level Parallel Index Rebuilding
    IDE_ASSERT(idp::read("REBUILD_INDEX_PARALLEL_MODE",
                         &mIndexRebuildParallelMode )
               == IDE_SUCCESS);

    /* PROJ-2433 */
    IDE_ASSERT( idp::read( "__MEM_BTREE_NODE_SIZE",
                           &mMemBtreeNodeSize )
                == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "__MEM_BTREE_DEFAULT_MAX_KEY_SIZE",
                           &mMemBtreeDefaultMaxKeySize )
                == IDE_SUCCESS );

    /* BUG-40509 Change Memory Index Node Split Rate  */
    IDE_ASSERT( idp::read( "MEMORY_INDEX_UNBALANCED_SPLIT_RATE",
                           &mMemoryIndexUnbalancedSplitRate )
                == IDE_SUCCESS );

    /* PROJ-2613 Key Redistribution In MRDB Index */
    IDE_ASSERT( idp::read( "MEM_INDEX_KEY_REDISTRIBUTION",
                           &mMemIndexKeyRedistribution )
                == IDE_SUCCESS );

    /* PROJ-2613 Key Redistribution In MRDB Index */
    IDE_ASSERT( idp::read( "MEM_INDEX_KEY_REDISTRIBUTION_STANDARD_RATE",
                           &mMemIndexKeyRedistributionStandardRate )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__SCANLIST_MOVE_NONBLOCK",
                           &sTempValue )
                == IDE_SUCCESS );

    mScanlistMoveNonBlock = ( sTempValue == 1 ) ? ID_TRUE : ID_FALSE;

    IDE_ASSERT(idp::read("THREAD_CPU_AFFINITY",
                         &sTempValue)
               == IDE_SUCCESS);

    mIsCPUAffinity = ( sTempValue == 1 ) ? ID_TRUE : ID_FALSE;

    /* BUG-44794 인덱스 빌드시 인덱스 통계 정보를 수집하지 않는 히든 프로퍼티 추가 */
    IDE_ASSERT( idp::read( "__GATHER_INDEX_STAT_ON_DDL",
                           &sTempValue )
                == IDE_SUCCESS );

    mGatherIndexStatOnDDL = ( sTempValue == 1 ) ? ID_TRUE : ID_FALSE;

    // BUG-47540 __SKIP_IDX_STAT_BOUND_NODE
    IDE_ASSERT(idp::read("__SKIP_IDX_STAT_NODE_BOUND",
                         &sTempValue)
               == IDE_SUCCESS);
    mSkipIdxStatNodeBound = (sTempValue == 1) ? ID_TRUE : ID_FALSE;
}
void smuProperty::loadForSMX()
{
    IDE_ASSERT(idp::read("REBUILD_MIN_VIEWSCN_INTERVAL_",
                         &mRebuildMinViewSCNInterval)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("OID_COUNT_IN_LIST_", &mOIDListCount)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("TRANSACTION_TOUCH_PAGE_COUNT_BY_NODE_", 
                &mTransTouchPageCntByNode)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("TRANSACTION_TOUCH_PAGE_CACHE_RATIO_", 
                &mTransTouchPageCacheRatio )
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("TRANSACTION_TABLE_SIZE", &mTransTblSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__TRANSACTION_TABLE_FULL_TRCLOG_CYCLE",
                         &mTransTableFullTrcLogCycle)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read("LOCK_TIME_OUT", &mLockTimeOut)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("REPLICATION_LOCK_TIMEOUT", &mReplLockTimeOut)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("TRANS_ALLOC_WAIT_TIME", &mTransAllocWaitTime)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read("TX_PRIVATE_BUCKET_COUNT", &mPrivateBucketCount)
                == IDE_SUCCESS);

    IDE_ASSERT( idp::read( "TRANSACTION_SEGMENT_COUNT", &mTXSEGEntryCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__GROUP_COMMIT_COUNT", &mGroupCommitCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__GROUP_COMMIT_LIST_COUNT", &mGroupCommitListCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "TRANSACTION_SEGMENT_ALLOC_WAIT_TIME_", 
                           &mTXSegAllocWaitTime ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__TX_OIDLIST_MEMPOOL_TYPE", 
                           &mTxOIDListMemPoolType) == IDE_SUCCESS );

    /* BUG-35392 */
    IDE_ASSERT( idp::read("UNCOMPLETED_LSN_CHECK_THREAD_INTERVAL",
                          &mUCLSNChkThrInterval )
                == IDE_SUCCESS );

    /* BUG-38515 Add hidden property for skip scn check in startup */    
    IDE_ASSERT( idp::read("__TRCLOG_LEGACY_TX_INFO",
                          &mTrcLogLegacyTxInfo )
                == IDE_SUCCESS );

    /* BUG-40790 */
    IDE_ASSERT( idp::read("__LOB_CURSOR_HASH_BUCKET_COUNT",
                          &mLobCursorHashBucketCount )
                == IDE_SUCCESS );

    /* BUG-47365 */
    IDE_ASSERT( idp::read("LOG_COMP_RESOURCE_REUSE",
                          &mLogCompResourceReuse )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("COMP_RES_TUNE_SIZE",
                          &mCompResTuneSize )
                == IDE_SUCCESS );

    /* PROJ-2733 */
    IDE_ASSERT( idp::read("VERSIONING_MIN_TIME",
                          &mVersioningMinTime )
                == IDE_SUCCESS );
}


void smuProperty::loadForSMU()
{

    UInt sLoop      = 0;
    UInt sTempValue = 0;

    IDE_ASSERT(idp::getMemValueCount("MEM_DB_DIR",
                                     &mDBDirCount)
               == IDE_SUCCESS);

    for( sLoop = 0; sLoop != mDBDirCount; ++sLoop )
    {
        IDE_ASSERT(idp::readPtr("MEM_DB_DIR",
                                (void**)&mDBDir[sLoop],
                                sLoop)
                   == IDE_SUCCESS);
    }

    IDE_ASSERT(idp::readPtr("DEFAULT_DISK_DB_DIR",
                            (void**)&mDefaultDiskDBDir)
               == IDE_SUCCESS);

    // PRJ-1552
    IDE_ASSERT(idp::read("ART_DECREASE_VAL",
                         (void**)&mArtDecreaseVal)
               == IDE_SUCCESS);

    /* PROJ-2433 */
    IDE_ASSERT( idp::read("__FORCE_INDEX_DIRECTKEY",
                          &sTempValue )
                == IDE_SUCCESS );

    mForceIndexDirectKey = ( sTempValue == 1 ) ? ID_TRUE : ID_FALSE;

    /* BUG-41121 */
    IDE_ASSERT( idp::read("__FORCE_INDEX_PERSISTENCE_MODE",
                          &mForceIndexPersistenceMode )
                == IDE_SUCCESS );

    /* PROJ-2733 */
    IDE_ASSERT( idp::read("SHARD_ENABLE",
                         (void*)&mShardEnable ) 
                == IDE_SUCCESS );

}

void smuProperty::loadForSMI()
{
    IDE_ASSERT(idp::read("__DBMS_STAT_METHOD",
                         (void*)&mDBMSStatMethod)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_METHOD_FOR_MRDB",
                         (void*)&mDBMSStatMethod4MRDB)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_METHOD_FOR_VRDB",
                         (void*)&mDBMSStatMethod4VRDB)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_METHOD_FOR_DRDB",
                         (void*)&mDBMSStatMethod4DRDB)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_SAMPLING_BASE_PAGE_COUNT",
                         (void*)&mDBMSStatSamplingBaseCnt)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_PARALLEL_DEGREE",
                         (void*)&mDBMSStatParallelDegree)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_GATHER_INTERNALSTAT",
                         (void*)&mDBMSStatGatherInternalStat)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_AUTO_PERCENTAGE",
                         (void*)&mDBMSStatAutoPercentage)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DBMS_STAT_AUTO_INTERVAL",
                         (void*)&mDBMSStatAutoInterval)
               == IDE_SUCCESS);


    IDE_ASSERT(idp::read("DEFAULT_SEGMENT_MANAGEMENT_TYPE",
                         (void*)&mDefaultSegMgmtType)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("DEFAULT_EXTENT_CNT_FOR_EXTENT_GROUP",
                         (void*)&mDefaultExtCntForExtentGroup)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_DATA_TBS_EXTENT_SIZE",
                         (void*)&mSysDataTBSExtentSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_DATA_FILE_INIT_SIZE",
                         (void*)&mSysDataFileInitSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_DATA_FILE_MAX_SIZE",
                         (void*)&mSysDataFileMaxSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_DATA_FILE_NEXT_SIZE",
                         (void*)&mSysDataFileNextSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_UNDO_TBS_EXTENT_SIZE",
                         (void*)&mSysUndoTBSExtentSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_UNDO_FILE_INIT_SIZE",
                         (void*)&mSysUndoFileInitSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_UNDO_FILE_MAX_SIZE",
                         (void*)&mSysUndoFileMaxSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_UNDO_FILE_NEXT_SIZE",
                         (void*)&mSysUndoFileNextSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_TEMP_TBS_EXTENT_SIZE",
                         (void*)&mSysTempTBSExtentSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_TEMP_FILE_INIT_SIZE",
                         (void*)&mSysTempFileInitSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_TEMP_FILE_MAX_SIZE",
                         (void*)&mSysTempFileMaxSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SYS_TEMP_FILE_NEXT_SIZE",
                         (void*)&mSysTempFileNextSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_DATA_TBS_EXTENT_SIZE",
                         (void*)&mUserDataTBSExtentSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_DATA_FILE_INIT_SIZE",
                         (void*)&mUserDataFileInitSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_DATA_FILE_MAX_SIZE",
                         (void*)&mUserDataFileMaxSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_DATA_FILE_NEXT_SIZE",
                         (void*)&mUserDataFileNextSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_TEMP_TBS_EXTENT_SIZE",
                         (void*)&mUserTempTBSExtentSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_TEMP_FILE_INIT_SIZE",
                         (void*)&mUserTempFileInitSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_TEMP_FILE_MAX_SIZE",
                         (void*)&mUserTempFileMaxSize)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("USER_TEMP_FILE_NEXT_SIZE",
                         (void*)&mUserTempFileNextSize)
               == IDE_SUCCESS);

    IDE_ASSERT( idp::read("LOCK_ESCALATION_MEMORY_SIZE",
                          &mLockEscMemSize)
                == IDE_SUCCESS );

    IDE_ASSERT(idp::read("__DISK_INDEX_BOTTOM_UP_BUILD_THRESHOLD",
                         &mIndexBuildBottomUpThreshold)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__TABLE_BACKUP_TIMEOUT",
                         &mTableBackupTimeOut)
               == IDE_SUCCESS);

}

/*
 * Proj-2059 DB Upgrade 기능
 * SCPModule에서 사용하는 Properties를 읽어온다.  */
void smuProperty::loadForSCP()
{
    IDE_ASSERT(idp::read("__DATAPORT_FILE_BLOCK_SIZE",
                         (void*)&mDataPortFileBlockSize)
               == IDE_SUCCESS);

    // DirectIO를 위해 Align을 맞춰줘야 합니다.
    mDataPortFileBlockSize = idlOS::align( mDataPortFileBlockSize,
                                           ID_MAX_DIO_PAGE_SIZE );

    IDE_ASSERT(idp::read("__EXPORT_COLUMN_CHAINING_THRESHOLD",
                         (void*)&mExportColumnChainingThreshold)
               == IDE_SUCCESS);

    IDE_ASSERT(idp::read("__DATAPORT_DIRECT_IO_ENABLE",
                         (void*)&mDataPortDirectIOEnable)
               == IDE_SUCCESS);
}

/* PROJ-2102 Fast Secondary Buffer */
void smuProperty::loadForSDS()
{
    IDE_ASSERT( idp::read("SECONDARY_BUFFER_ENABLE",
                         (void*)&mSBufferEnable ) 
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SECONDARY_BUFFER_FLUSHER_CNT",
                         (void*)&mSBufferFlusherCount ) 
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__MAX_SECONDARY_CHECKPOINT_FLUSHER_CNT",
                           &mMaxSBufferCPFlusherCnt )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SECONDARY_BUFFER_TYPE",
                         (void*)&mSBufferType ) 
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read("SECONDARY_BUFFER_SIZE",
                         (void*)&mSBufferSize )
                == IDE_SUCCESS );

    IDE_ASSERT(idp::readPtr("SECONDARY_BUFFER_FILE_DIRECTORY",
                            (void **)&mSBufferFileDirectory )
               == IDE_SUCCESS);
}

void smuProperty::loadForUTIL()
{
    IDE_ASSERT(idp::read("PORT_NO",
                         (void**)&mPortNo)
               == IDE_SUCCESS);

}

void smuProperty::loadForSystemThread()
{
    //===================================================================
    // To Fix PR-14783 System Thread Control
    //===================================================================

    IDE_ASSERT( idp::read( (const SChar*) "MEM_DELETE_THREAD",
                           & mRunMemDeleteThread ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( (const SChar*) "MEM_GC_THREAD",
                           & mRunMemGCThread ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( (const SChar*) "BUFFER_FLUSH_THREAD",
                           & mRunBufferFlushThread ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( (const SChar*) "ARCHIVE_THREAD",
                           & mRunArchiveThread ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( (const SChar*) "CHECKPOINT_THREAD",
                           & mRunCheckpointThread ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( (const SChar*) "LOG_FLUSH_THREAD",
                           & mRunLogFlushThread ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( (const SChar*) "LOG_PREPARE_THREAD",
                           & mRunLogPrepareThread ) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( (const SChar*) "LOG_PREREAD_THREAD",
                           & mRunLogPreReadThread ) == IDE_SUCCESS );
}

#ifdef ALTIBASE_ENABLE_SMARTSSD
void smuProperty::loadForSmartSSD()
{
    IDE_ASSERT( idp::read( "SMART_SSD_LOG_RUN_GC_ENABLE",
                           &mSmartSSDLogRunGCEnable) == IDE_SUCCESS );
    IDE_ASSERT( idp::readPtr( "SMART_SSD_LOG_DEVICE",
                              (void **)&mSmartSSDLogDevice) == IDE_SUCCESS );
    IDE_ASSERT( idp::read( "SMART_SSD_GC_TIME_MILLISEC",
                           &mSmartSSDGCTimeMilliSec) == IDE_SUCCESS );
}
#endif

void smuProperty::loadForLockMgr(void)
{
    SMU_PROPERTY_READONLY_REGIST("LOCK_MGR_CACHE_NODE", LockMgrCacheNode);
}

void smuProperty::registCallbacks()
{
    // a3
    idp::setupAfterUpdateCallback("CHECKPOINT_INTERVAL_IN_SEC",
                                  callbackChkptIntervalInSec);

    idp::setupAfterUpdateCallback("CHECKPOINT_INTERVAL_IN_LOG",
                                  callbackChkptIntervalInLog);

    idp::setupBeforeUpdateCallback("CHECKPOINT_BULK_SYNC_PAGE_COUNT",
                                   callbackChkptBulkSyncPageCount);

    idp::setupAfterUpdateCallback("LOCK_ESCALATION_MEMORY_SIZE",
                                  callbackLockEscMemSize);

    idp::setupAfterUpdateCallback("LOCK_TIME_OUT",
                                  callbackLockTimeOut);

    idp::setupAfterUpdateCallback("DDL_LOCK_TIMEOUT",
                                  callbackDDLLockTimeOut);

    idp::setupAfterUpdateCallback("__USE_TEMP_FOR_PREPARE_LOGFILE",
                                  callbackUseTempForPrepareLogFile);
    
    idp::setupAfterUpdateCallback("LOGFILE_PRECREATE_INTERVAL",
                                  callbackLogFilePreCreateInterval);
    
    idp::setupAfterUpdateCallback("REPLICATION_LOCK_TIMEOUT",
                                  callbackReplLockTimeOut);

    // a4

    idp::setupAfterUpdateCallback("OPEN_DATAFILE_WAIT_INTERVAL",
                                  callbackOpenDataFileWaitInterval);

    idp::setupAfterUpdateCallback("BULKIO_PAGE_COUNT_FOR_DIRECT_PATH_INSERT",
                                  callbackBulkIOPageCnt4DPInsert);

    idp::setupAfterUpdateCallback("DIRECT_PATH_BUFFER_PAGE_COUNT",
                                  callbackDPathBuffPageCnt);

    idp::setupAfterUpdateCallback("DB_FILE_MULTIPAGE_READ_COUNT",
                                  callbackDBFileMultiReadCnt);

    idp::setupAfterUpdateCallback("SMALL_TABLE_THRESHOLD",
                                  callbackSmallTblThreshold);

    idp::setupAfterUpdateCallback("__FLUSHER_BUSY_CONDITION_CHECK_INTERVAL",
                                  callbackFlusherBusyConditionCheckInterval);

    // PROJ-1566
    idp::setupAfterUpdateCallback("__DIRECT_BUFFER_FLUSH_THREAD_SYNC_INTERVAL",
                                  callbackDPathBufferFlushThreadInterval);

    idp::setupAfterUpdateCallback("LOGICAL_AGER_COUNT_",
                                  callbackSetAgerCount );

    idp::setupBeforeUpdateCallback("TABLE_LOCK_ENABLE",
                                   callbackTableLockEnable);

    idp::setupBeforeUpdateCallback("TABLESPACE_LOCK_ENABLE",
                                   callbackTablespaceLockEnable);

    idp::setupBeforeUpdateCallback("__SKIP_LOCKED_TABLE_AT_FIXED_TABLE",
                                   callbackSkipLockedTableAtFixedTable);

    idp::setupBeforeUpdateCallback("CHECKPOINT_BULK_WRITE_PAGE_COUNT",
                                   callbackChkptBulkWritePageCount);

    idp::setupBeforeUpdateCallback("CHECKPOINT_BULK_WRITE_SLEEP_SEC",
                                   callbackChkptBulkWriteSleepSec);

    idp::setupBeforeUpdateCallback("CHECKPOINT_BULK_WRITE_SLEEP_USEC",
                                   callbackChkptBulkWriteSleepUSec);

    idp::setupBeforeUpdateCallback("TRANS_ALLOC_WAIT_TIME",
                                   callbackTransAllocWaitTime);

    // BUG-47655
    idp::setupBeforeUpdateCallback("__TRANSACTION_TABLE_FULL_TRCLOG_CYCLE",
                                   callbackTransTableFullTrcLogCycle);

    idp::setupAfterUpdateCallback("DATAFILE_WRITE_UNIT_SIZE",
                                  callbackDataFileWriteUnitSize);

    idp::setupAfterUpdateCallback("DRDB_FD_MAX_COUNT_PER_DATAFILE",
                                  callbackMaxOpenFDCount4File);

    idp::setupAfterUpdateCallback("__DISK_INDEX_BOTTOM_UP_BUILD_THRESHOLD",
                                  callbackDiskIndexBottomUpBuildThreshold);

    idp::setupAfterUpdateCallback("DISK_INDEX_BUILD_MERGE_PAGE_COUNT",
                                  callbackDiskIndexBuildMergePageCount);

    idp::setupAfterUpdateCallback("SORT_AREA_SIZE",
                                  callbackSortAreaSize);

    // BUG-29506 TBT가 TBK로 전환시 변경된 offset을 CTS에 반영하지 않습니다.
    // 재현하기 위해 CTS 할당 여부를 임의로 제어하기 위한 PROPERTY를 추가
    idp::setupAfterUpdateCallback("__DISABLE_TRANSACTION_BOUND_IN_CTS",
                                  callbackDisableTransactionBoundInCTS);

    // BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
    // 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
    idp::setupAfterUpdateCallback("__MANUAL_BINDING_TRANSACTION_SEGMENT_BY_ENTRY_ID",
                                  callbackManualBindingTXSegByEntryID);

    // PROJ-1628
    idp::setupAfterUpdateCallback("__DISK_INDEX_KEY_REDISTRIBUTION_LOW_LIMIT",
                                  callbackDiskIndexKeyRedistributionLowLimit);

    // BUG-43046
    idp::setupAfterUpdateCallback("__DISK_INDEX_MAX_TRAVERSE_LENGTH",
                                  callbackDiskIndexMaxTraverseLength);

    idp::setupAfterUpdateCallback("DISK_INDEX_UNBALANCED_SPLIT_RATE",
                                  callbackDiskIndexUnbalancedSplitRate);

    idp::setupAfterUpdateCallback("__DISK_INDEX_RTREE_MAX_KEY_COUNT",
                                  callbackDiskIndexRTreeMaxKeyCount);

    idp::setupAfterUpdateCallback("__DISK_INDEX_RTREE_SPLIT_MODE",
                                  callbackDiskIndexRTreeSplitMode);

    /* BUG-18725: Writable Property에 Update Callback Function이 설정되지
     * 않은 것이 있습니다. */
#if 0 //not used
    idp::setupAfterUpdateCallback("TSS_CNT_PCT_TO_BUFFER_POOL",
                                  callbackTssCntPctToBufferPool);
#endif
    idp::setupAfterUpdateCallback("CHECKPOINT_FLUSH_MAX_GAP",
                                  callbackCheckpointFlushMaxGap);

    /* BUG-40137 Can`t modify __CHECKPOINT_FLUSH_JOB_RESPONSIBILITY property value 
     * by alter system set */
    idp::setupAfterUpdateCallback("__CHECKPOINT_FLUSH_JOB_RESPONSIBILITY",
                                  callbackCheckpointFlushResponsibility);

    // PROJ-1629 : Memory Index Build
    idp::setupAfterUpdateCallback("MEMORY_INDEX_BUILD_RUN_SIZE",
                                  callbackMemoryIndexBuildRunSize);

    idp::setupAfterUpdateCallback("SHM_PAGE_COUNT_PER_KEY",
                                  callbackShmPageCountPerKey );

    idp::setupAfterUpdateCallback("MIN_PAGES_ON_TABLE_FREE_LIST",
                                  callbackMinPagesOnTableFreeList );

    idp::setupAfterUpdateCallback("TABLE_ALLOC_PAGE_COUNT",
                                  callbackAllocPageCount );

    idp::setupAfterUpdateCallback("SYNC_INTERVAL_SEC_",
                                  callbackSyncIntervalSec );

    idp::setupAfterUpdateCallback("SYNC_INTERVAL_MSEC_",
                                  callbackSyncIntervalMSec );

    // BUG-45598: CHECKSUM_METHOD 수정 
    idp::setupAfterUpdateCallback("CHECKSUM_METHOD",
                                  callbackSMChecksumMethod );

    idp::setupAfterUpdateCallback("__SM_CHECKSUM_DISABLE",
                                  callbackSMChecksumDisable );

    idp::setupAfterUpdateCallback("__SM_AGER_DISABLE",
                                  callbackSMAgerDisable );

    idp::setupAfterUpdateCallback("__SM_CHECK_DISK_INDEX_INTEGRITY",
                                  callbackCheckDiskIndexIntegrity );

    idp::setupAfterUpdateCallback("MIN_LOG_RECORD_SIZE_FOR_COMPRESS",
                                  callbackMinLogRecordSizeForCompress );

    idp::setupAfterUpdateCallback("__AFTER_CARE",
                                  callbackAfterCare );

    idp::setupAfterUpdateCallback("CHECKPOINT_ENABLED",
                                  callbackChkptEnabled );

    idp::setupAfterUpdateCallback("__SEPARATE_DIC_TBS_SIZE_ENABLE",
                                  callbackSeparateDicTBSSizeEnable );

    /*PROJ-2162 RestartRiskReduction Begin */
    idp::setupAfterUpdateCallback("__SM_ENABLE_STARTUP_BUG_DETECTOR", 
                                  callbackSmEnableStartupBugDetector );
    idp::setupAfterUpdateCallback("__SM_MTX_ROLLBACK_TEST", 
                                  callbackSmMtxRollbackTest );
    idp::setupAfterUpdateCallback("__CRASH_TOLERANCE",                
                                  callbackCrashTolerance );
    /*PROJ-2162 RestartRiskReduction End */

    // PROJ-1629 : Memory Index Build
    idp::setupAfterUpdateCallback("MEMORY_INDEX_BUILD_VALUE_LENGTH_THRESHOLD",
                                  callbackMemoryIndexBuildValueLengthThreshold);

    //===================================================================
    // To Fix PR-14783 System Thread Control
    //===================================================================
    idp::setupAfterUpdateCallback("MEM_DELETE_THREAD",
                                  callbackMemDeleteThread);
    idp::setupAfterUpdateCallback("MEM_GC_THREAD",
                                  callbackMemGCThread);
    idp::setupAfterUpdateCallback("BUFFER_FLUSH_THREAD",
                                  callbackBufferFlushThread);
    idp::setupAfterUpdateCallback("ARCHIVE_THREAD",
                                  callbackArchiveThread);
    idp::setupAfterUpdateCallback("CHECKPOINT_THREAD",
                                  callbackCheckpointThread);
    idp::setupAfterUpdateCallback("LOG_FLUSH_THREAD",
                                  callbackLogFlushThread);
    idp::setupAfterUpdateCallback("LOG_PREPARE_THREAD",
                                  callbackLogPrepareThread);
    idp::setupAfterUpdateCallback("LOG_PREREAD_THREAD",
                                  callbackLogPreReadThread);
    idp::setupAfterUpdateCallback("BUFFER_VICTIM_SEARCH_INTERVAL",
                                  callbackBufferVictimSearchInterval);

    // PROJ-1568 Buffer Manager Renewal begin
    idp::setupAfterUpdateCallback("BUFFER_VICTIM_SEARCH_PCT",
                                  callbackBufferVictimSearchPct);
    idp::setupAfterUpdateCallback("HOT_TOUCH_CNT",
                                  callbackHotTouchCnt);

    /* PROJ-2669 */
    idp::setupAfterUpdateCallback("DELAYED_FLUSH_LIST_PCT",
                                  callbackDelayedFlushListPct);
    idp::setupAfterUpdateCallback("DELAYED_FLUSH_PROTECTION_TIME_MSEC",
                                  callbackDelayedFlushProtectionTimeMsec);

    idp::setupAfterUpdateCallback("HOT_LIST_PCT",
                                  callbackHotListPct);
    idp::setupAfterUpdateCallback("BUFFER_AREA_SIZE",
                                  callbackBufferAreaSize);
    idp::setupAfterUpdateCallback("DEFAULT_FLUSHER_WAIT_SEC",
                                  callbackDefaultFlusherWaitSec);
    idp::setupAfterUpdateCallback("MAX_FLUSHER_WAIT_SEC",
                                  callbackMaxFlusherWaitSec);
    idp::setupAfterUpdateCallback("CHECKPOINT_FLUSH_COUNT",
                                  callbackCheckpointFlushCount);
    idp::setupAfterUpdateCallback("FAST_START_IO_TARGET",
                                  callbackFastStartIoTarget);
    idp::setupAfterUpdateCallback("FAST_START_LOGFILE_TARGET",
                                  callbackFastStartLogFileTarget);
    idp::setupAfterUpdateCallback("LOW_PREPARE_PCT",
                                  callbackLowPreparePCT);
    idp::setupAfterUpdateCallback("HIGH_FLUSH_PCT",
                                  callbackHighFlushPCT);
    idp::setupAfterUpdateCallback("LOW_FLUSH_PCT",
                                  callbackLowFlushPCT);
    idp::setupAfterUpdateCallback("TOUCH_TIME_INTERVAL",
                                  callbackTouchTimeInterval);
    idp::setupAfterUpdateCallback("CHECKPOINT_FLUSH_MAX_WAIT_SEC",
                                  callbackCheckpointFlushMaxWaitSec);
#if 0
    idp::setupAfterUpdateCallback( "TRANS_WAIT_TIME_FOR_TTS",
                                   callbackTransWaitTime4TTS );
#endif
    idp::setupAfterUpdateCallback( "TRANS_WAIT_TIME",
                                   callbackTransWaitTime );
    idp::setupAfterUpdateCallback("BLOCK_ALL_TX_TIME_OUT",
                                  callbackBlockAllTxTimeOut);
    // PROJ-1568 Buffer Manager Renewal end
    // PROJ-1671 Bitmap Tablespace And Segment Space Management
    idp::setupAfterUpdateCallback( "__DISK_TMS_IGNORE_HINT_PID",
                                   callbackTmsIgnoreHintPID);
    idp::setupAfterUpdateCallback( "__DISK_TMS_MANUAL_SLOT_NO_IN_ITBMP",
                                   callbackTmsManualSlotNoInItBMP);
    idp::setupAfterUpdateCallback( "__DISK_TMS_MANUAL_SLOT_NO_IN_LFBMP",
                                   callbackTmsManualSlotNoInLfBMP);

    idp::setupAfterUpdateCallback( "__DISK_TMS_CANDIDATE_LFBMP_CNT",
                                   callbackTmsCandidateLfBMPCnt);
    idp::setupAfterUpdateCallback( "__DISK_TMS_CANDIDATE_PAGE_CNT",
                                   callbackTmsCandidatePageCnt);
    idp::setupAfterUpdateCallback( "__DISK_TMS_MAX_SLOT_CNT_PER_RTBMP",
                                   callbackTmsMaxSlotCntPerRtBMP);
    idp::setupAfterUpdateCallback( "__DISK_TMS_MAX_SLOT_CNT_PER_ITBMP",
                                   callbackTmsMaxSlotCntPerItBMP);
    idp::setupAfterUpdateCallback( "__DISK_TMS_MAX_SLOT_CNT_PER_EXTDIR",
                                   callbackTmsMaxSlotCntPerExtDir);

    /* PROJ-2037 TMS 안정화 */
    idp::setupAfterUpdateCallback( "DEFAULT_EXTENT_CNT_FOR_EXTENT_GROUP",
                                   callbackDefaulExtCntForExtentGroup );

    idp::setupAfterUpdateCallback( "__FMS_EXTDESC_CNT_IN_EXTDIRPAGE",
                                   callbackExtDescCntInExtDirPage );

    // PROJ-1704 Disk MVCC Renewal
    idp::setupAfterUpdateCallback( "TRANSACTION_TOUCH_PAGE_COUNT_BY_NODE_",
                                   callbackTransTouchPageCntByNode);
    idp::setupAfterUpdateCallback( "TRANSACTION_TOUCH_PAGE_CACHE_RATIO_",
                                   callbackTransTouchPageCacheRatio);
    idp::setupAfterUpdateCallback( "REBUILD_MIN_VIEWSCN_INTERVAL_",
                                   callbackRebuildMinViewSCNInterval);

    idp::setupAfterUpdateCallback( "RETRY_STEAL_COUNT_",
                                   callbackRetryStealCount );

    idp::setupAfterUpdateCallback( "TSSEG_SIZE_SHRINK_THRESHOLD",
                                   callbackTSSegSizeShrinkThreshold );

    idp::setupAfterUpdateCallback( "UDSEG_SIZE_SHRINK_THRESHOLD",
                                   callbackUDSegSizeShrinkThreshold );

    idp::setupAfterUpdateCallback( "TABLE_COMPACT_AT_SHUTDOWN",
                                   callbackTableCompactAtShutdown);

    // BUG-27126 INDEX_BUILD_THREAD_COUNT를 alter system 으로 변경가능 해야...
    idp::setupAfterUpdateCallback( "INDEX_BUILD_THREAD_COUNT",
                                   callbackIndexBuildThreadCount);

    /* BUG-40509 Change Memory Index Node Split Rate */
    idp::setupAfterUpdateCallback( "MEMORY_INDEX_UNBALANCED_SPLIT_RATE",
                                   callbackMemoryIndexUnbalancedSplitRate );

    /* Proj-2059 DB Upgrade 기능 */ 
    idp::setupAfterUpdateCallback( "__DATAPORT_FILE_BLOCK_SIZE",
                                   callbackDataPortFileBlockSize);

    idp::setupAfterUpdateCallback( "__EXPORT_COLUMN_CHAINING_THRESHOLD",
                                   callbackExportColumnChainingThreshold);

    idp::setupAfterUpdateCallback( "__DATAPORT_DIRECT_IO_ENABLE",
                                   callbackDataPortDirectIOEnable);

    idp::setupAfterUpdateCallback( "__DPATH_BUFF_PAGE_ALLOC_RETRY_USEC",
                                   callbackDPathBuffPageAllocRetryUSec );

    idp::setupAfterUpdateCallback( "__DPATH_INSERT_ENABLE",
                                   callbackDPathInsertEnable );

    idp::setupAfterUpdateCallback("__RESERVED_DISK_SIZE_FOR_LOGFILE",
                                   callbackReservedDiskSizeForLogFile );

    idp::setupAfterUpdateCallback("__MEM_DPFLUSH_WAIT_TIME",
                                   callbackMemDPFlushWaitTime );

    idp::setupAfterUpdateCallback("__MEM_DPFLUSH_WAIT_SPACEID",
                                   callbackMemDPFlushWaitSID );

    idp::setupAfterUpdateCallback("__MEM_DPFLUSH_WAIT_PAGEID",
                                   callbackMemDPFlushWaitPID );

    idp::setupAfterUpdateCallback("__MEM_DPFLUSH_WAIT_OFFSET",
                                   callbackMemDPFlushWaitOffset );

    idp::setupAfterUpdateCallback("__LOG_READ_METHOD_TYPE",
                                  callbackLogReadMethodType );

    /* TASK-4990 changing the method of collecting index statistics */
    idp::setupBeforeUpdateCallback("__DBMS_STAT_METHOD",
                                   callbackDBMSStatMethod);

    idp::setupBeforeUpdateCallback("__DBMS_STAT_METHOD_FOR_MRDB",
                                   callbackDBMSStatMethod4MRDB);

    idp::setupBeforeUpdateCallback("__DBMS_STAT_METHOD_FOR_VRDB",
                                   callbackDBMSStatMethod4VRDB);

    idp::setupBeforeUpdateCallback("__DBMS_STAT_METHOD_FOR_DRDB",
                                   callbackDBMSStatMethod4DRDB);

    idp::setupBeforeUpdateCallback("__DBMS_STAT_SAMPLING_BASE_PAGE_COUNT",
                                   callbackDBMSStatSamplingBaseCnt);
    idp::setupBeforeUpdateCallback("__DBMS_STAT_PARALLEL_DEGREE",
                                   callbackDBMSStatParallelDegree );
    idp::setupBeforeUpdateCallback("__DBMS_STAT_GATHER_INTERNALSTAT",
                                   callbackDBMSStatGatherInternalStat );
    idp::setupBeforeUpdateCallback("__DBMS_STAT_AUTO_INTERVAL",
                                   callbackDBMSStatAutoInterval );
    idp::setupBeforeUpdateCallback("__DBMS_STAT_AUTO_PERCENTAGE",
                                   callbackDBMSStatAutoPercentage );

    idp::setupBeforeUpdateCallback("__TABLE_BACKUP_TIMEOUT",
                                    callbackTableBackupTimeOut );

    /* BUG-38621 */
    idp::setupBeforeUpdateCallback("__RELATIVE_PATH_IN_LOG",
                                    callbackRELPathInLog );

    /* BUG-38515 Add hidden property for skip scn check in startup */
    idp::setupBeforeUpdateCallback("__TRCLOG_LEGACY_TX_INFO",
                                    callbackTRCLogLegacyTxInfo );

    /* BUG-40790 */
    idp::setupBeforeUpdateCallback("__LOB_CURSOR_HASH_BUCKET_COUNT",
                                   callbackLobCursorHashBucketCount);

    /* BUG-41550 */
    idp::setupBeforeUpdateCallback("__INCREMENTAL_BACKUP_INFO_RETENTION_PERIOD_FOR_TEST",
                                   callbackBackupInfoRetentionPeriodForTest);

    /* PROJ-2613 Key Redistribution in MRDB Index */
    idp::setupBeforeUpdateCallback("MEM_INDEX_KEY_REDISTRIBUTION",
                                   callbackMemIndexKeyRedistribution);

    /* PROJ-2613 Key Redistribution in MRDB Index */
    idp::setupBeforeUpdateCallback("MEM_INDEX_KEY_REDISTRIBUTION_STANDARD_RATE",
                                   callbackMemIndexKeyRedistributionStandardRate);

    idp::setupBeforeUpdateCallback("__SCANLIST_MOVE_NONBLOCK",
                                   callbackScanlistMoveNonBlock );

    idp::setupAfterUpdateCallback( "__GATHER_INDEX_STAT_ON_DDL",
                                   callbackGatherIndexStatOnDDL);
    /* BUG-45744 */
    idp::setupAfterUpdateCallback( "__LOG_COMPRESSION_ACCELERATION",
                                   callbackLogCompAcceleration );

    /* PROJ-2734 */
    idp::setupAfterUpdateCallback( "DISTRIBUTION_DEADLOCK_ENABLE",
                                   callbackDistributionDeadlockEnable );
    idp::setupAfterUpdateCallback( "DISTRIBUTION_DEADLOCK_RISK_LOW_WAIT_TIME",
                                   callbackDistributionDeadlockRiskLowWaitTime );
    idp::setupAfterUpdateCallback( "DISTRIBUTION_DEADLOCK_RISK_MID_WAIT_TIME",
                                   callbackDistributionDeadlockRiskMidWaitTime );
    idp::setupAfterUpdateCallback( "DISTRIBUTION_DEADLOCK_RISK_HIGH_WAIT_TIME",
                                   callbackDistributionDeadlockRiskHighWaitTime );
    /* PROJ-2733 */
    idp::setupAfterUpdateCallback( "VERSIONING_MIN_TIME",
                                   callbackVersioningMinTime );
}

IDE_RC smuProperty::checkDuplecateMultiplexDirPath(
                                    SChar  ** aArchMultiplexDirPath,
                                    SChar  ** aLogMultiplexDirPath,
                                    UInt     aArchMultiplexDirCnt,
                                    UInt     aLogMultiplexDirCnt )
{
    UInt i;
    UInt j;

    IDE_DASSERT( aArchMultiplexDirPath != NULL );
    IDE_DASSERT( aLogMultiplexDirPath  != NULL );

    /* 다중화 ARCH PATH와 중복되는 LOG, ARCH LOG DIR이 있는지 검사 */
    for( i = 0; i < aArchMultiplexDirCnt; i++)
    {
        IDE_TEST_RAISE( idlOS::strncmp( aArchMultiplexDirPath[i],
                                        mArchiveDirPath,
                                        IDP_MAX_VALUE_LEN )
                        == 0, error_duplicate_arch_multiplex_dir_path )

        IDE_TEST_RAISE( idlOS::strncmp( aArchMultiplexDirPath[i],
                                        mLogDirPath,
                                        IDP_MAX_VALUE_LEN )
                        == 0, error_duplicate_arch_multiplex_dir_path )

        /*ArchLog 다중화 디렉토중 중복되는 디렉토리가 있는지 검사 */
        for( j = 0; j < aArchMultiplexDirCnt; j++ )
        {
            if ( i != j )
            {
                IDE_TEST_RAISE( idlOS::strncmp( aArchMultiplexDirPath[i],
                                                aArchMultiplexDirPath[j],
                                                IDP_MAX_VALUE_LEN )
                                == 0, error_duplicate_arch_multiplex_dir_path )
            }
        }
    }

    /* 다중화 LOG PATH와 중복되는 LOG, ARCH LOG DIR이 있는지 검사 */
    for( i = 0; i < aLogMultiplexDirCnt; i++ )
    {
        IDE_TEST_RAISE( idlOS::strncmp( aLogMultiplexDirPath[i],
                                        mArchiveDirPath,
                                        IDP_MAX_VALUE_LEN )
                        == 0, error_duplicate_log_multiplex_dir_path )

        IDE_TEST_RAISE( idlOS::strncmp( aLogMultiplexDirPath[i],
                                        mLogDirPath,
                                        IDP_MAX_VALUE_LEN )
                        == 0, error_duplicate_log_multiplex_dir_path )

        /* Log 다중화 디렉토중 중복되는 디렉토리가 있는지 검사 */
        for( j = 0; j < aLogMultiplexDirCnt; j++ )
        {
            if ( i != j )
            {
                IDE_TEST_RAISE( idlOS::strncmp( aLogMultiplexDirPath[i],
                                                aLogMultiplexDirPath[j],
                                                IDP_MAX_VALUE_LEN )
                                == 0, error_duplicate_log_multiplex_dir_path )
            }
        }
    }

    /* Log와 ArchLog 다중화 디렉토중 중복되는 디렉토리가 있는지 검사 */
    for( i = 0; i < aLogMultiplexDirCnt; i++ )
    {
        for( j = 0; j < aArchMultiplexDirCnt; j++ )
        {
            IDE_TEST_RAISE( idlOS::strncmp( aLogMultiplexDirPath[i],
                                            aArchMultiplexDirPath[j],
                                            IDP_MAX_VALUE_LEN )
                            == 0, error_duplicate_log_multiplex_dir_path )
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_duplicate_arch_multiplex_dir_path );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_DuplicateMultiplexDirPath,
                                aArchMultiplexDirPath[i]));
    }
    IDE_EXCEPTION( error_duplicate_log_multiplex_dir_path );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_DuplicateMultiplexDirPath,
                                aLogMultiplexDirPath[i]));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smuProperty::checkConstraints()
{
    UInt sLogFileAlignSize ;

#if defined( NTO_QNX )
    /* You need to pass in the flag MAP_NOSYNCFILE.
       This is due to the fact that the current VM on Neutrino
       does not support keeping a memory mapped file in sync with the disk
       if you use read/write to modify the file. */

    /* QNX와 WinCE는 mmap을 지원하지 않는다. 때문에 다음과 같이 처리한다.*/
    IDE_TEST_RAISE( mLogBufferType == SMU_LOG_BUFFER_TYPE_MMAP , err_not_support_mmap );
#endif

    IDE_TEST_RAISE( mArchiveMultiplexCount != mArchiveMultiplexDirCount,
                    error_wrong_arch_multiplex_dir_count );

    IDE_TEST_RAISE( mLogMultiplexCount != mLogMultiplexDirCount,
                    error_wrong_multiplex_log_dir_count );

    IDE_TEST( checkDuplecateMultiplexDirPath( mArchiveMultiplexDirPath,
                                              mLogMultiplexDirPath,
                                              mArchiveMultiplexCount,
                                              mLogMultiplexCount )

              != IDE_SUCCESS );

    if ( mPerListDistPageCount == 0)
    {   /* BUG-42816 */
        mPerListDistPageCount = mExpandChunkPageCount / (mPageListGroupCount * 2);
    }

    /* LogFile 크기는 DIRECT_IO_PAGE_SIZE의 배수이어야 한다. */
    IDE_TEST_RAISE( mLogFileSize % iduProperty::getDirectIOPageSize()
                    != 0, err_invalid_logfile_size );

    /* BUG-31170 [sm-util] [SM] need that check the direct i/o limitation. */
    IDE_TEST_RAISE( mLogFileSize > IDU_FILE_DIRECTIO_MAX_SIZE, 
                    err_too_large_logfile_size );

    /*
      Direct IO Page크기체크.

      추후 file offset과 data size를 Page단위로 Align 할때
      비용이 비싼 곱셈/나눗셈 대신 비용이 저렴한
      Bit Mask And연산을 사용하기 위함
    */

    // PR-14475 Group Commit
    // 로그파일 크기가 Direct I/O Page 최대크기인 8K로 Align되어있는지
    // 검사한다. idp::read 시에 idlOS::getpagesize()나
    // Direct I/O최대 Page크기 중 큰 값으로 Align하도록 되어있다.
    // align하는 이유에 대해서는 idpDescResource.cpp의 LOG_FILE_SIZE 참고
    sLogFileAlignSize = (UInt) ((idlOS::getpagesize() > ID_MAX_DIO_PAGE_SIZE) ?
                                idlOS::getpagesize() : ID_MAX_DIO_PAGE_SIZE );

    IDE_ASSERT( ( mLogFileSize % sLogFileAlignSize ) == 0 );

    IDE_TEST_RAISE ( ( mDefaultMemDBFileSize %
                       (mExpandChunkPageCount * SM_PAGE_SIZE) ) != 0,
                     error_db_file_size_not_aligned_to_chunk_size );

    IDE_TEST( checkFileSizeLimit( "DEFAULT_MEM_DB_FILE_SIZE property",
                                  mDefaultMemDBFileSize )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( mMaxLogicalAgerCount < mMinLogicalAgerCount,
                    err_max_ager_count_lt_min_ager_count );

    IDE_TEST_RAISE( mLogicalAgerCount > mMaxLogicalAgerCount,
                    err_ager_count_out_of_max_count );

    IDE_TEST_RAISE( mLogicalAgerCount < mMinLogicalAgerCount,
                    err_ager_count_out_of_min_count );

    IDE_TEST_RAISE( mDiskIndexNameCntToVerify != mVerifyDiskIndexCount,
                    err_verify_disk_index_count );

    IDE_TEST_RAISE( mExportColumnChainingThreshold*2  >  mDataPortFileBlockSize,
                    ERR_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE );

    // BUG-29566 테이터 파일의 크기를 32G 를 초과하여 지정해도 에러를 출력하지 않습니다.
    // 32G뿐만 아니라 OS Limit File Size도 검사합니다.

    IDE_TEST( checkFileSizeProperty( "SYS_DATA_FILE_INIT_SIZE",
                                     "SYS_DATA_FILE_MAX_SIZE",
                                     mSysDataFileInitSize,
                                     mSysDataFileMaxSize )
              != IDE_SUCCESS );

    IDE_TEST( checkFileSizeProperty( "SYS_UNDO_FILE_INIT_SIZE",
                                     "SYS_UNDO_FILE_MAX_SIZE",
                                     mSysUndoFileInitSize,
                                     mSysUndoFileMaxSize )
              != IDE_SUCCESS );

    IDE_TEST( checkFileSizeProperty( "SYS_TEMP_FILE_INIT_SIZE",
                                     "SYS_TEMP_FILE_MAX_SIZE",
                                     mSysTempFileInitSize,
                                     mSysTempFileMaxSize )
              != IDE_SUCCESS );

    IDE_TEST( checkFileSizeProperty( "USER_DATA_FILE_INIT_SIZE",
                                     "USER_DATA_FILE_MAX_SIZE",
                                     mUserDataFileInitSize,
                                     mUserDataFileMaxSize )
              != IDE_SUCCESS );

    IDE_TEST( checkFileSizeProperty( "USER_TEMP_FILE_INIT_SIZE",
                                     "USER_TEMP_FILE_MAX_SIZE",
                                     mUserTempFileInitSize,
                                     mUserTempFileMaxSize )
              != IDE_SUCCESS );

    /* BUG-31862 resize transaction table without db migration
     * TRANSACTION_TABLE_SIZE 값은 16 ~ 16384(2^14) 까지의 2^n 값만 가능하다
     */
    IDE_TEST_RAISE( (mTransTblSize & (mTransTblSize -1)) != 0,
                    invalid_trans_tbl_size);

    /*******************************************************************
     * PROJ-2102 Fast Secondary Buffer 
     *******************************************************************/
    IDE_TEST( checkSBufferPropery() != IDE_SUCCESS );


    /*******************************************************************
     * PROJ-2433 Direct Key Index Max Size
     *******************************************************************/
    IDE_TEST_RAISE( ( mMemBtreeNodeSize / 3 ) < mMemBtreeDefaultMaxKeySize,
                    ERR_INVALID_DIRECTKEY_MAXSIZE );


    /*******************************************************************
     * BUG-45292 fallocate
     *******************************************************************/
# if !defined (PDL_HAS_FALLOCATE)
    IDE_TEST_RAISE( mCreateMethod == SMU_LOG_FILE_CREATE_FALLOCATE,
                    ERR_NOT_SUPPORT_FALLOCATE );
#endif

    return IDE_SUCCESS;

#if defined( NTO_QNX ) || defined ( VC_WINCE)
    IDE_EXCEPTION( err_not_support_mmap );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_Not_Support_MMap ));
    }
#endif
    IDE_EXCEPTION( error_wrong_arch_multiplex_dir_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongArchMultiplexDirCount,
                                mArchiveMultiplexDirCount,
                                mArchiveMultiplexCount));
    }
    IDE_EXCEPTION( error_wrong_multiplex_log_dir_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_WrongLogMultiplexDirCount,
                                mLogMultiplexDirCount,
                                mLogMultiplexCount));
    }
    IDE_EXCEPTION( error_db_file_size_not_aligned_to_chunk_size );
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_DefaultDBFileSizeNotAlignedToChunkSize,
                    // KB 단위의 Expand Chunk Page 크기
                    ((ULong)mExpandChunkPageCount * SM_PAGE_SIZE ) / 1024 ));
    }
    IDE_EXCEPTION(err_max_ager_count_lt_min_ager_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_MAX_AGER_COUNT_LT_MIN_AGER_COUNT,
                                mMaxLogicalAgerCount, mMinLogicalAgerCount ));
    }
    IDE_EXCEPTION(err_ager_count_out_of_max_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AGER_COUNT_OUT_OF_MAX_COUNT,
                                mLogicalAgerCount, mMaxLogicalAgerCount ));
    }
    IDE_EXCEPTION(err_ager_count_out_of_min_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AGER_COUNT_OUT_OF_MIN_COUNT,
                                mLogicalAgerCount, mMinLogicalAgerCount ));
    }
    IDE_EXCEPTION( err_invalid_logfile_size );
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_LogFileSizeNotAlignedToDirectIOPageSize,
                    mLogFileSize,
                    iduProperty::getDirectIOPageSize() ));
    }
    IDE_EXCEPTION( err_too_large_logfile_size );
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_LOGFILE_TOO_BIG_WITH_DIRECT_IO,
                    mLogFileSize,
                    IDU_FILE_DIRECTIO_MAX_SIZE ));
    }
    IDE_EXCEPTION( err_verify_disk_index_count )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_WrongVerifyDiskIndexCount,
                                  mDiskIndexNameCntToVerify,
                                  mVerifyDiskIndexCount ) );
    }
    IDE_EXCEPTION( ERR_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE );
    {
        IDE_SET( ideSetErrorCode(
                 smERR_ABORT_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE,
                 mExportColumnChainingThreshold, 
                 mDataPortFileBlockSize ) );
    }
    IDE_EXCEPTION(invalid_trans_tbl_size);
    {
        IDE_SET( ideSetErrorCode(
                    smERR_ABORT_TRANSACTION_TABLE_SIZE_IS_NOT_POWER_OF_TWO,
                    mTransTblSize ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DIRECTKEY_MAXSIZE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDirectKeyMaxSize ) );
    }
# if !defined (PDL_HAS_FALLOCATE)
    IDE_EXCEPTION( ERR_NOT_SUPPORT_FALLOCATE )
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_NOT_SUPPORT_FALLOCATE ));
    }    
#endif

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  File Size이 OS의  Limit보다 큰지 체크한다.

  [IN] aUserKeyword - 사용자가 파일의 크기를 지정할 때 사용한 키워드
  에러메세지와 함께 출력한다.
  ex> DEFAULT_MEM_MAX_DB_FILE_SIZE property
  ex> SPLIT EACH clause
  [IN] aFileSize - 사용자가 지정한 파일의 크기
*/
IDE_RC smuProperty::checkFileSizeLimit( const SChar  * aUserKeyword, ULong aFileSize )
{

#if defined(CYGWIN32) || defined(WRS_VXWORKS)
    return IDE_SUCCESS ;
#else
    struct rlimit     limit;

    IDE_TEST_RAISE(idlOS::getrlimit(RLIMIT_FSIZE, &limit) != 0,
                   getrlimit_error);

    IDE_TEST_RAISE( limit.rlim_cur  < aFileSize-1,
                    check_OSLimit_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION(getrlimit_error);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_GETLIMIT_ERROR ));
    }

    IDE_EXCEPTION(check_OSLimit_error);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_OSFileSizeLimit_ERROR,
                                (ULong) (limit.rlim_cur+1) / 1024,
                                (ULong) aFileSize,
                                aUserKeyword ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#endif
}

/**********************************************************************
 * Description: Data File관련 Property를 검사해서 OS Limit을 넘어가는지,
 *              init Size가 Max Size보다 크지는 않은지 검사한다.
 *              32G제한에 대해서는 idpProperty에서 검증된다.
 *              해당 TBS들은 모두 Autoextend on상태이다.
 *              ( BUG-29599 데이터 파일의 크기를 32G 를 초과하여 지정해도
 *                          에러를 출력하지 않습니다. )
 *
 *     [ 검사순서 ]
 *      1. autoextend on 에서 init size < max size인지
 *      2. max size 가 32G or OS Limit을 넘지 않는지
 *
 * aPropertyName - [IN] 오류 발생시 사용자가 확인해야 할 Property의 Name
 * aInitFileSize - [IN] Init File Size
 * aMaxFileSize  - [IN] Max File SIze
 **********************************************************************/
IDE_RC smuProperty::checkFileSizeProperty( const SChar  * aInitSizePropName,
                                           const SChar  * aMaxSizePropName,
                                           ULong         aInitFileSize ,
                                           ULong         aMaxFileSize )
{
#if !defined(CYGWIN32) && !defined(WRS_VXWORKS)
    struct rlimit     limit;
    ULong             sOSLimit;
#endif

    IDE_TEST_RAISE( aInitFileSize > aMaxFileSize,
                    err_init_exceed_maxfilesize );

#if !defined(CYGWIN32) && !defined(WRS_VXWORKS)
    IDE_TEST_RAISE(idlOS::getrlimit(RLIMIT_FSIZE, &limit) != 0,
                   getrlimit_error);

    sOSLimit = limit.rlim_cur;
    IDE_TEST_RAISE( sOSLimit < aMaxFileSize-1,
                    check_OSLimit_error );
#endif

    return IDE_SUCCESS;

#if !defined(CYGWIN32) && !defined(WRS_VXWORKS)
    IDE_EXCEPTION(getrlimit_error);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_GETLIMIT_ERROR ));
    }
    IDE_EXCEPTION(check_OSLimit_error);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_MaxSizePropExceedOSLimit,
                                 aMaxSizePropName,
                                 aMaxFileSize,
                                 sOSLimit ));
    }
#endif
    IDE_EXCEPTION( err_init_exceed_maxfilesize )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InitSizePropExceedMaxSizeProp,
                                  aInitSizePropName,
                                  aMaxSizePropName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
* Secondary Buffer의 Propery를 확인  
*******************************************************************/
IDE_RC smuProperty::checkSBufferPropery()
{
    if ( mSBufferEnable != SMU_SECONDARY_BUFFER_ENABLE )
    {
        if ( mSBufferSize != 0 )
        {
            mSBufferSize = 0;
            ideLog::log(IDE_SERVER_0,
                    "The value of the SECONDARY_BUFFER_ENABLE property is 0\n"
                    "Before Secondary Buffer is executed, please set the value of  property to 1.");
        }
        if ( idlOS::strcmp( mSBufferFileDirectory, "") != 0 )
        {
            idlOS::strcpy( mSBufferFileDirectory, "");
            ideLog::log(IDE_SERVER_0,
                    "The value of the SECONDARY_BUFFER_ENABLE property is 0\n"
                    "Before Secondary Buffer is executed, please set the value of  property to 1.");
        }
    }
    else 
    {
        IDE_TEST( mSBufferSize == 0 );
        IDE_TEST( idlOS::strcmp( mSBufferFileDirectory, "" )== 0 );

        /* Os level 의 limit 검사만 수행 기타는 sdsFile에서 함 */
        IDE_TEST( checkFileSizeLimit( "SECONDARY_BUFFER_SIZE property",
                                      mSBufferSize )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( 
                smERR_ABORT_invalid_secondary_buffer_propperty,
                mSBufferSize,
                mSBufferFileDirectory ) );

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackInitTotalWASize( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * /*aArg*/ )
{
    ULong  sNewInitSize = *((ULong *)aNewValue);
    ULong  sMaxSize;
    UInt   sState = 0;

    sdtWAExtentMgr::lock();
    sState = 1;
    sMaxSize = getMaxTotalWASize();

    // 새로운 Init Size와 Max Total WA Size 중 작은값으로 구성 변경
    if ( sNewInitSize < sMaxSize )
    {
        IDE_TEST( sdtWAExtentMgr::resizeWAExtentPool( sNewInitSize ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdtWAExtentMgr::resizeWAExtentPool( sMaxSize ) != IDE_SUCCESS );
    }

    mInitTotalWASize = sNewInitSize;

    sState = 0;
    sdtWAExtentMgr::unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sState = 0;
        sdtWAExtentMgr::unlock();
    }

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackMaxTotalWASize( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * /*aArg*/ )
{
    ULong  sNewMaxSize = *((ULong *)aNewValue);
    ULong  sInitSize;
    UInt   sState   = 0;

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    IDE_TEST_RAISE( sNewMaxSize >
                    ( idlOS::sysconf(_SC_PHYS_PAGES) * idlOS::sysconf(_SC_PAGESIZE) ),
                    ERR_ABORT_INTERNAL_MAXSIZE );
#endif

    sdtWAExtentMgr::lock();
    sState = 1;

    sInitSize = mInitTotalWASize;

    // Init Size와 새로운 Max Total WA Size 중 작은값으로 구성 변경
    if ( sInitSize < sNewMaxSize )
    {
        // 만약 Old max Total WA Size가 작아서 Total WA Size값으로 설정 되어 있었다면
        // init size 만큼 Total WA Size를 늘릴 것이다.
        IDE_TEST( sdtWAExtentMgr::resizeWAExtentPool( sInitSize ) != IDE_SUCCESS );
    }
    else // sNewMaxSize < sInitSize
    {
        // 새로운 New Max Size가 Init Size보다 작은 경우로 New max Size에 맞춘다.
        IDE_TEST( sdtWAExtentMgr::resizeWAExtentPool( sNewMaxSize ) != IDE_SUCCESS );
    }

    mMaxTotalWASize = sNewMaxSize;
    sState = 0;
    sdtWAExtentMgr::unlock();

    return IDE_SUCCESS;

#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    IDE_EXCEPTION( ERR_ABORT_INTERNAL_MAXSIZE )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InternalServerErrorWithString, 
                                  " The supplied value was beyond the bounds of MaxTotalWASize" ) );
    }
#endif
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sState = 0;
        sdtWAExtentMgr::unlock();
    }

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackSortAreaSize( idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ )
{
    ULong   sSortAreaSize = *((ULong *)aNewValue);
    UInt    sState = 0;

    if( mSortAreaSize != sSortAreaSize )
    {
        sdtSortSegment::lock();
        sState = 1;

        IDE_TEST( sdtSortSegment::resetWASegmentSize( sSortAreaSize ) != IDE_SUCCESS );
        mSortAreaSize = sSortAreaSize;

        sState = 0;
        sdtSortSegment::unlock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sState = 0;
        sdtSortSegment::unlock();
    }

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackHashAreaSize( idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ )
{
    ULong   sHashAreaSize = *((ULong *)aNewValue);
    UInt    sState = 0;

    if( mHashAreaSize != sHashAreaSize )
    {
        sdtHashModule::lock();
        sState = 1;

        IDE_TEST( sdtHashModule::resetWASegmentSize( sHashAreaSize ) != IDE_SUCCESS );
        mHashAreaSize = sHashAreaSize;

        sState = 0;
        sdtHashModule::unlock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        sState = 0;
        sdtHashModule::unlock();
    }

    return IDE_FAILURE;
}

/* TEMP_HASH_SLOT_RATIO 프로퍼티 값을 변경 할 때,
 * TEMP_HASH_GROUP_RATIO 등을 고려하여 변경 가능한 값인지
 * 확인 하도록 한다.
 */
IDE_RC smuProperty::callbackTempSubHashGroupRatio( idvSQL * /*aStatistics*/,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * /*aArg*/ )
{
    UInt   sTempSubHashGroupRatio = *((UInt *)aNewValue);

    if( mTempSubHashGroupRatio != sTempSubHashGroupRatio )
    {
        sdtHashModule::lock();

        mTempSubHashGroupRatio = sTempSubHashGroupRatio;

        sdtHashModule::resetTempSubHashGroupPageCount();

        sdtHashModule::unlock();
    }

    return IDE_SUCCESS;
}


/* TEMP_HASH_SLOT_RATIO 프로퍼티 값을 변경 할 때,
 * TEMP_HASH_GROUP_RATIO 등을 고려하여 변경 가능한 값인지
 * 확인 하도록 한다.
 */
IDE_RC smuProperty::callbackTempHashGroupRatio( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    UInt   sTempHashGroupRatio = *((UInt *)aNewValue);

    if( mTempHashGroupRatio != sTempHashGroupRatio )
    {
        sdtHashModule::lock();

        mTempHashGroupRatio = sTempHashGroupRatio;

        sdtHashModule::resetTempHashGroupPageCount();
        sdtHashModule::resetTempSubHashGroupPageCount();

        sdtHashModule::unlock();
    }

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackTmpInitWASegCnt( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,    
                                             void   * /*aArg*/ )
{
    UInt  sNewCount = *((UInt *)aNewValue);

    IDE_TEST( sdtHashModule::resizeWASegmentPool( sNewCount ) != IDE_SUCCESS );
    IDE_TEST( sdtSortSegment::resizeWASegmentPool( sNewCount ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//smi
IDE_RC smuProperty::callbackDBMSStatMethod( idvSQL * /*aStatistics*/,
                                            SChar  * /*aName*/,
                                            void   * /*aOldValue*/,
                                            void   * aNewValue,
                                            void   * /*aArg*/ )
{
    mDBMSStatMethod =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDBMSStatMethod4MRDB( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mDBMSStatMethod4MRDB =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDBMSStatMethod4VRDB( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mDBMSStatMethod4VRDB =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDBMSStatMethod4DRDB( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mDBMSStatMethod4DRDB =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackDBMSStatSamplingBaseCnt( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mDBMSStatSamplingBaseCnt =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}
IDE_RC smuProperty::callbackDBMSStatParallelDegree( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mDBMSStatParallelDegree =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}
IDE_RC smuProperty::callbackDBMSStatGatherInternalStat( idvSQL * /*aStatistics*/,
                                                        SChar  * /*aName*/,
                                                        void   * /*aOldValue*/,
                                                        void   * aNewValue,
                                                        void   * /*aArg*/ )
{
    mDBMSStatGatherInternalStat =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDBMSStatAutoPercentage( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mDBMSStatAutoPercentage =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDBMSStatAutoInterval( idvSQL * /*aStatistics*/,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * /*aArg*/ )
{
    mDBMSStatAutoInterval =  *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTableBackupTimeOut( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mTableBackupTimeOut   = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* BUG-38621 */
IDE_RC smuProperty::callbackRELPathInLog( idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ )
{
    mRELPathInLog         = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// To Fix BUG-9366
IDE_RC smuProperty::callbackChkptBulkWritePageCount( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mChkptBulkWritePageCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackChkptBulkWriteSleepSec( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mChkptBulkWriteSleepSec = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackChkptBulkWriteSleepUSec( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mChkptBulkWriteSleepUSec = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackChkptBulkSyncPageCount( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mChkptBulkSyncPageCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackChkptIntervalInSec( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mChkptIntervalInSec = *((UInt *)aNewValue);

    gSmrChkptThread.setCheckPTTimeInterval();

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackChkptIntervalInLog( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mChkptIntervalInLog = *((UInt *)aNewValue);

    gSmrChkptThread.setCheckPTLSwitchInterval();

    return IDE_SUCCESS;
}



IDE_RC smuProperty::callbackLockEscMemSize( idvSQL * /*aStatistics*/,
                                            SChar  * /*aName*/,
                                            void   * /*aOldValue*/,
                                            void   * aNewValue,
                                            void   * /*aArg*/ )
{
    mLockEscMemSize = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackLockTimeOut( idvSQL * /*aStatistics*/,
                                         SChar  * /*aName*/,
                                         void   * /*aOldValue*/,
                                         void   * aNewValue,
                                         void   * /*aArg*/ )
{
    mLockTimeOut = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackUseTempForPrepareLogFile( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mUseTempForPrepareLogFile = (*((UInt *)aNewValue) == 0 ) ? ID_FALSE : ID_TRUE;

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLogFilePreCreateInterval( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mLogFilePreCreateInterval = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackReplLockTimeOut( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * /*aArg*/ )
{
    mReplLockTimeOut = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackOpenDataFileWaitInterval( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mOpenDataFileWaitInterval = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackBulkIOPageCnt4DPInsert( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mBulkIOPageCnt4DPInsert = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDPathBuffPageCnt( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ )
{
    mDPathBuffPageCnt = *((UInt *)aNewValue);

    sdbDPathBufferMgr::setMaxDPathBuffPageCnt( mDPathBuffPageCnt );

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDBFileMultiReadCnt( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mDBFileMultiReadCnt = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSmallTblThreshold( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mSmallTblThreshold = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackFlusherBusyConditionCheckInterval( idvSQL * /*aStatistics*/,
                                                               SChar  * /*aName*/,
                                                               void   * /*aOldValue*/,
                                                               void   * aNewValue,
                                                               void   * /*aArg*/ )
{
    mFlusherBusyConditionCheckInterval = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDPathBufferFlushThreadInterval( idvSQL * /*aStatistics*/,
                                                            SChar  * /*aName*/,
                                                            void   * /*aOldValue*/,
                                                            void   * aNewValue,
                                                            void   * /*aArg*/ )
{
    mDPathBuffFThreadSyncInterval = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDPathBuffPageAllocRetryUSec( idvSQL * /*aStatistics*/,
                                                         SChar  * /*aName*/,
                                                         void   * /*aOldValue*/,
                                                         void   * aNewValue,
                                                         void   * /*aArg*/ )
{
    mDPathBuffPageAllocRetryUSec = *((SLong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDPathInsertEnable(
    idvSQL * /*aStatistics*/,
    SChar * /*aName*/,
    void  * /*aOldValue*/,
    void  *aNewValue,
    void  * /*aArg*/)
{
    mDPathInsertEnable = ( *((UInt *)aNewValue) == 1 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackReservedDiskSizeForLogFile( idvSQL * /*aStatistics*/,
                                                        SChar  * /*aName*/,
                                                        void   * /*aOldValue*/,
                                                        void   * aNewValue,
                                                        void   * /*aArg*/ )

{
    mReservedDiskSizeForLogFile = *((ULong *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMemDPFlushWaitTime( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mMemDPFlushWaitTime = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMemDPFlushWaitSID( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mMemDPFlushWaitSID = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMemDPFlushWaitPID( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mMemDPFlushWaitPID = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMemDPFlushWaitOffset( idvSQL * /*aStatistics*/,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * /*aArg*/ )
{
    mMemDPFlushWaitOffset = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLogReadMethodType ( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mLogReadMethodType = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTableLockEnable( idvSQL * aStatistics,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * aArg )
{
    smiTrans* sTrans;
    UInt sState =0;
    idBool sIsBlockedAllTx = ID_FALSE;

    idpArgument *sArgObj = (idpArgument *)aArg;

    sTrans=  (smiTrans *)(sArgObj->getArgValue(aStatistics, sArgObj,IDP_ARG_TRANSID));
    IDE_ASSERT( sTrans != NULL);

    /*
     * BUG-42927
     * BLOCK_ALL_TIME_OUT(default 3초)동안
     * 새로운 active transaction을 BLOCK 하고 active transaciont이 모두 종료되었는지 확인한다.
     * BLOCK된 시간안에 active transaction이 모두 종료되었다면, TABLE_LOCK_ENABLE 값을 변경한다. 
     */
    if ( mTableLockEnable != *((UInt *)aNewValue) )
    {
        smxTransMgr::block( sTrans->getTrans(),
                            ( smuProperty::getBlockAllTxTimeOut() * 1000 * 1000 ), /* sec => micro-sec */ 
                            &sIsBlockedAllTx );
        sState = 1;
        IDL_MEM_BARRIER;

        IDE_TEST_CONT( sIsBlockedAllTx != ID_TRUE, error_active_trans_exits );

        mTableLockEnable = *((UInt *)aNewValue);
        sState = 0;
        smxTransMgr::unblock();
    }
    else
    {
        /* unchanged value */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_active_trans_exits);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ActiveTransExits));
    }

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        smxTransMgr::unblock();
    }
    return IDE_FAILURE;
}


IDE_RC smuProperty::callbackTablespaceLockEnable( idvSQL * aStatistics,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * aArg )
{
    smiTrans* sTrans;
    UInt sState =0;

    idpArgument *sArgObj = (idpArgument *)aArg;

    sTrans=  (smiTrans *)(sArgObj->getArgValue(aStatistics, sArgObj,IDP_ARG_TRANSID));
    IDE_ASSERT( sTrans != NULL);

    //1.먼저 active transaction이 있는지 먼저 검사한다.
    IDE_TEST_RAISE(smxTransMgr::existActiveTrans((smxTrans*) sTrans->getTrans()) == ID_TRUE,
                   error_active_trans_exits);

    // 2. active 트랜잭션이 없는 것을확인하고 나서
    //트랜잭션 begin을 못하게 한다.
    smxTransMgr::disableTransBegin();
    sState =1;
    IDL_MEM_BARRIER;

    // 3. 다시 확인. 1~2사이에 새로운 트랜잭션이 begin될수 있기 때문이다.
    IDE_TEST_RAISE(smxTransMgr::existActiveTrans((smxTrans*)sTrans->getTrans()) == ID_TRUE,
                   error_active_trans_exits);

    mTablespaceLockEnable =  *((UInt *)aNewValue);
    sState =0;
    smxTransMgr::enableTransBegin();

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_active_trans_exits);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ActiveTransExits));
    }

    IDE_EXCEPTION_END;
    if ( sState != 0 )
    {
        smxTransMgr::enableTransBegin();
    }
    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackSkipLockedTableAtFixedTable( idvSQL * /*aStatistics*/,
                                                         SChar  * /*aName*/,
                                                         void   * /*aOldValue*/,
                                                         void   * aNewValue,
                                                         void   * /*aArg*/ )
{
    mSkipLockedTableAtFixedTable  = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTransAllocWaitTime( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mTransAllocWaitTime   = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// BUG-47655
IDE_RC smuProperty::callbackTransTableFullTrcLogCycle( idvSQL * /*aStatistics*/,
                                                       SChar  * /*aName*/,
                                                       void   * /*aOldValue*/,
                                                       void   * aNewValue,
                                                       void   * /*aArg*/ )
{
    mTransTableFullTrcLogCycle = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// PROJ-1568
IDE_RC smuProperty::callbackHotTouchCnt( idvSQL * /*aStatistics*/,
                                         SChar  * /*aName*/,
                                         void   * /*aOldValue*/,
                                         void   * aNewValue,
                                         void   * /*aArg*/ )

{
    sdbBufferPool *sPool;
    sPool = sdbBufferMgr::getPool();

    sdbBufferMgr::lockBufferMgrMutex(NULL);
    mHotTouchCnt = *((UInt *) aNewValue);
    sPool->mHotTouchCnt = mHotTouchCnt;
    sdbBufferMgr::unlockBufferMgrMutex();

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackBufferVictimSearchInterval( idvSQL * /*aStatistics*/,
                                                        SChar  * /*aName*/,
                                                        void   * /*aOldValue*/,
                                                        void   * aNewValue,
                                                        void   * /*aArg*/ )
{
    mBufferVictimSearchInterval = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackBufferVictimSearchPct( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    sdbBufferPool *sPool;
    sPool = sdbBufferMgr::getPool();

    sdbBufferMgr::lockBufferMgrMutex(NULL);
    mBufferVictimSearchPct = *((UInt *) aNewValue);

    sPool->setLRUSearchCnt( mBufferVictimSearchPct );
    sdbBufferMgr::unlockBufferMgrMutex();

    return IDE_SUCCESS;
}

/* PROJ-2669 begin */
IDE_RC smuProperty::callbackDelayedFlushListPct( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )

{
    sdbBufferPool *sPool;
    sPool = sdbBufferMgr::getPool();

    mDelayedFlushListPct = *((UInt *) aNewValue);

    sdbBufferMgr::lockBufferMgrMutex(NULL);
    sPool->setMaxDelayedFlushListPct( mDelayedFlushListPct );
    sdbBufferMgr::unlockBufferMgrMutex();

    sdbFlushMgr::setDelayedFlushProperty( mDelayedFlushListPct,
                                          mDelayedFlushProtectionTimeMsec );
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDelayedFlushProtectionTimeMsec( idvSQL * /*aStatistics*/,
                                                            SChar  * /*aName*/,
                                                            void   * /*aOldValue*/,
                                                            void   * aNewValue,
                                                            void   * /*aArg*/ )

{
    mDelayedFlushProtectionTimeMsec = *((UInt *) aNewValue);
    sdbFlushMgr::setDelayedFlushProperty( mDelayedFlushListPct,
                                      mDelayedFlushProtectionTimeMsec );
    return IDE_SUCCESS;
}
/* PROJ-2669 end */

IDE_RC smuProperty::callbackHotListPct( idvSQL * /*aStatistics*/,
                                        SChar  * /*aName*/,
                                        void   * /*aOldValue*/,
                                        void   * aNewValue,
                                        void   * /*aArg*/ )

{
    sdbBufferPool *sPool;
    sPool = sdbBufferMgr::getPool();

    sdbBufferMgr::lockBufferMgrMutex(NULL);
    mHotListPct = *((UInt *) aNewValue);
    sPool->setHotMax( NULL, mHotListPct );
    sdbBufferMgr::unlockBufferMgrMutex();

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackBufferAreaSize( idvSQL * aStatistics,
                                            SChar  * /*aName*/,
                                            void   * /*aOldValue*/,
                                            void   * aNewValue,
                                            void   * /*aArg*/ )

{
    ULong      sNewSize;
    idBool     sLocked = ID_FALSE;
    smxTrans * sTrans;

    IDE_TEST( smxTransMgr::alloc( &sTrans,
                                  aStatistics ) !=  IDE_SUCCESS );

    IDE_ASSERT( sTrans->begin( NULL,
                               (SMI_TRANSACTION_REPL_NONE |
                                SMI_COMMIT_WRITE_NOWAIT),
                               SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    sdbBufferMgr::lockBufferMgrMutex(NULL);
    sLocked = ID_TRUE;
    IDE_TEST( sdbBufferMgr::resize( NULL,
                                    *(ULong*)aNewValue,
                                    (void*)sTrans,
                                    &sNewSize)
              != IDE_SUCCESS );

    IDE_TEST( sTrans->commit() != IDE_SUCCESS );
    IDE_TEST( smxTransMgr::freeTrans( sTrans) != IDE_SUCCESS );

    mBufferAreaSize = sNewSize;
    sdbBufferMgr::unlockBufferMgrMutex();
    sLocked = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLocked == ID_TRUE )
    {
        sdbBufferMgr::unlockBufferMgrMutex();
        sLocked = ID_FALSE;
    }
    return IDE_FAILURE;

}

IDE_RC smuProperty::callbackDefaultFlusherWaitSec( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    mDefaultFlusherWaitSec = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMaxFlusherWaitSec( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mMaxFlusherWaitSec = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackCheckpointFlushCount( idvSQL * /*aStatistics*/,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * /*aArg*/ )
{
    mCheckpointFlushCount = *((ULong *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackFastStartIoTarget( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mFastStartIoTarget = *((ULong *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackFastStartLogFileTarget( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mFastStartLogFileTarget = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLowPreparePCT(idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ )
{
    mLowPreparePCT = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackHighFlushPCT( idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ )
{
    mHighFlushPCT = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLowFlushPCT( idvSQL * /*aStatistics*/,
                                         SChar  * /*aName*/,
                                         void   * /*aOldValue*/,
                                         void   * aNewValue,
                                         void   * /*aArg*/ )
{
    mLowFlushPCT = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTouchTimeInterval( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    sdbBufferMgr::lockBufferMgrMutex(NULL);
    //BUG-21621 [PRJ-1568] LRU-touch_time_interval.sql에서 diff발생
    mTouchTimeInterval = *((UInt *) aNewValue);
    sdbBCB::mTouchUSecInterval = mTouchTimeInterval * 1000000;
    sdbBufferMgr::unlockBufferMgrMutex();

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackCheckpointFlushMaxWaitSec( idvSQL * /*aStatistics*/,
                                                       SChar  * /*aName*/,
                                                       void   * /*aOldValue*/,
                                                       void   * aNewValue,
                                                       void   * /*aArg*/ )
{
    mCheckpointFlushMaxWaitSec = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackBlockAllTxTimeOut( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mBlockAllTxTimeOut = *((UInt *) aNewValue);

    return IDE_SUCCESS;
}

//proj-1568 end


//===================================================================
// To Fix PR-14783 System Thread Control
//===================================================================


IDE_RC smuProperty::callbackMemDeleteThread( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * /*aArg*/ )
{
    mRunMemDeleteThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMemGCThread( idvSQL * /*aStatistics*/,
                                         SChar  * /*aName*/,
                                         void   * /*aOldValue*/,
                                         void   * aNewValue,
                                         void   * /*aArg*/ )
{
    mRunMemGCThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackBufferFlushThread( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mRunBufferFlushThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackArchiveThread( idvSQL * /*aStatistics*/,
                                           SChar  * /*aName*/,
                                           void   * /*aOldValue*/,
                                           void   * aNewValue,
                                           void   * /*aArg*/ )
{
    mRunArchiveThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackCheckpointThread( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ )
{
    mRunCheckpointThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLogFlushThread( idvSQL * /*aStatistics*/,
                                            SChar  * /*aName*/,
                                            void   * /*aOldValue*/,
                                            void   * aNewValue,
                                            void   * /*aArg*/ )
{
    mRunLogFlushThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLogPrepareThread( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ )
{
    mRunLogPrepareThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLogPreReadThread( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ )
{
    mRunLogPreReadThread = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/*
  PROJ-1548

  기능 : DDL_LOCK_TIMEOUT 을 위한 Callback 함수

*/
IDE_RC smuProperty::callbackDDLLockTimeOut( idvSQL * /*aStatistics*/,
                                            SChar  * /*aName*/,
                                            void   * /*aOldValue*/,
                                            void   * aNewValue,
                                            void   * /*aArg*/ )
{
    idlOS::memcpy( &mDDLLockTimeOut,
                   aNewValue,
                   ID_SIZEOF(SInt) );

    return IDE_SUCCESS;
}

// BUG-17226
// 데이터 파일 생성시 쓰기 단위 사용
IDE_RC smuProperty::callbackDataFileWriteUnitSize( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    mDataFileWriteUnitSize = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMaxOpenFDCount4File( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mMaxOpenFDCount4File = *((UInt *)aNewValue);

    IDE_TEST( sctTableSpaceMgr::setMaxFDCntAllDFileOfAllDiskTBS(
                                                     mMaxOpenFDCount4File )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackDiskIndexBottomUpBuildThreshold( idvSQL * /*aStatistics*/,
                                                             SChar  * /*aName*/,
                                                             void   * /*aOldValue*/,
                                                             void   * aNewValue,
                                                             void   * /*aArg*/ )
{
    mIndexBuildBottomUpThreshold = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDiskIndexKeyRedistributionLowLimit( idvSQL * /*aStatistics*/,
                                                                SChar  * /*aName*/,
                                                                void   * /*aOldValue*/,
                                                                void   * aNewValue,
                                                                void   * /*aArg*/ )
{
    mKeyRedistributionLowLimit = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDiskIndexMaxTraverseLength( idvSQL * /*aStatistics*/,
                                                        SChar  * /*aName*/,
                                                        void   * /*aOldValue*/,
                                                        void   * aNewValue,
                                                        void   * /*aArg*/ )
{
    mMaxTraverseLength = *((SLong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDiskIndexUnbalancedSplitRate( idvSQL * /*aStatistics*/,
                                                          SChar  * /*aName*/,
                                                          void   * /*aOldValue*/,
                                                          void   * aNewValue,
                                                          void   * /*aArg*/ )
{
    mUnbalancedSplitRate = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDiskIndexRTreeMaxKeyCount( idvSQL * /*aStatistics*/,
                                                       SChar  * /*aName*/,
                                                       void   * /*aOldValue*/,
                                                       void   * aNewValue,
                                                       void   * /*aArg*/ )
{
    mRTreeMaxKeyCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDiskIndexRTreeSplitMode( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mRTreeSplitMode = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDiskIndexBuildMergePageCount( idvSQL * /*aStatistics*/,
                                                          SChar  * /*aName*/,
                                                          void   * /*aOldValue*/,
                                                          void   * aNewValue,
                                                          void   * /*aArg*/ )
{
    mMergePageCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}



// BUG-29506 TBT가 TBK로 전환시 변경된 offset을 CTS에 반영하지 않습니다.
// 재현하기 위해 CTS 할당 여부를 임의로 제어하기 위한 PROPERTY를 추가
IDE_RC smuProperty::callbackDisableTransactionBoundInCTS( idvSQL * /*aStatistics*/,
                                                          SChar  * /*aName*/,
                                                          void   * /*aOldValue*/,
                                                          void   * aNewValue,
                                                          void   * /*aArg*/ )
{
    mDisableTransactionBoundInCTS = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
// 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
IDE_RC smuProperty::callbackManualBindingTXSegByEntryID( idvSQL * /*aStatistics*/,
                                                         SChar  * /*aName*/,
                                                         void   * /*aOldValue*/,
                                                         void   * aNewValue,
                                                         void   * /*aArg*/ )
{
    mManualBindingTXSegByEntryID = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

#if 0 // not used
IDE_RC smuProperty::callbackTssCntPctToBufferPool( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    mTssCntPctToBufferPool = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}
#endif

IDE_RC smuProperty::callbackCheckpointFlushMaxGap( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    mCheckpointFlushMaxGap = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

/* BUG-40137 Can`t modify __CHECKPOINT_FLUSH_JOB_RESPONSIBILITY property value 
 * by alter system set */
IDE_RC smuProperty::callbackCheckpointFlushResponsibility( idvSQL * /*aStatistics*/,
                                                           SChar  * /*aName*/,
                                                           void   * /*aOldValue*/,
                                                           void   * aNewValue,
                                                           void   * /*aArg*/ )
{
    mCheckpointFlushResponsibility = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

// PROJ-1629 : Memory Index Build
IDE_RC smuProperty::callbackMemoryIndexBuildRunSize(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/ )
{
    mMemoryIndexBuildRunSize = *((ULong *)aNewValue);
    return IDE_SUCCESS;
}

// PROJ-1629 : Memory Index Build
IDE_RC smuProperty::callbackMemoryIndexBuildValueLengthThreshold(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/ )
{
    mMemoryIndexBuildValueLengthThreshold = *((ULong *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackShmPageCountPerKey( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mShmPageCountPerKey = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMinPagesOnTableFreeList( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mMinPagesOnTableFreeList = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackAllocPageCount( idvSQL * /*aStatistics*/,
                                            SChar  * /*aName*/,
                                            void   * /*aOldValue*/,
                                            void   * aNewValue,
                                            void   * /*aArg*/ )
{
    mAllocPageCount = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSyncIntervalSec( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * /*aArg*/ )
{
    mSyncIntervalSec = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSyncIntervalMSec( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ )
{
    mSyncIntervalMSec = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}
// BUG-45598: CHECKSUM_METHOD 콜백 
IDE_RC smuProperty::callbackSMChecksumMethod( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ )
{
    mCheckSumMethod  = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}
IDE_RC smuProperty::callbackSMChecksumDisable( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mSMChecksumDisable = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSMAgerDisable( idvSQL * /*aStatistics*/,
                                           SChar  * /*aName*/,
                                           void   * /*aOldValue*/,
                                           void   * aNewValue,
                                           void   * /*aArg*/ )
{
    mSMAgerDisable = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackCheckDiskIndexIntegrity( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mCheckDiskIndexIntegrity = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackMinLogRecordSizeForCompress( idvSQL * /*aStatistics*/,
                                                         SChar  * /*aName*/,
                                                         void   * /*aOldValue*/,
                                                         void   * aNewValue,
                                                         void   * /*aArg*/ )
{
    mMinLogRecordSizeForCompress = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackAfterCare( idvSQL * /*aStatistics*/,
                                       SChar  * /*aName*/,
                                       void   * /*aOldValue*/,
                                       void   * aNewValue,
                                       void   * /*aArg*/ )
{
    mAfterCare = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackChkptEnabled( idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ )
{
    mChkptEnabled = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSeparateDicTBSSizeEnable( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mSeparateDicTBSSizeEnable  = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSmEnableStartupBugDetector( idvSQL * /*aStatistics*/,
                                                        SChar  * /*aName*/,
                                                        void   * /*aOldValue*/,
                                                        void   * aNewValue,
                                                        void   * /*aArg*/ )
{
    mSmEnableStartupBugDetector = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSmMtxRollbackTest( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mSmMtxRollbackTest = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackCrashTolerance( idvSQL * /*aStatistics*/,
                                            SChar  * /*aName*/,
                                            void   * /*aOldValue*/,
                                            void   * aNewValue,
                                            void   * /*aArg*/ )
{
    mCrashTolerance = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

#if 0
IDE_RC smuProperty::callbackTransWaitTime4TTS( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mTransWaitTime4TTS = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}
#endif

IDE_RC smuProperty::callbackTransWaitTime( idvSQL * /*aStatistics*/,
                                           SChar  * /*aName*/,
                                           void   * /*aOldValue*/,
                                           void   * aNewValue,
                                           void   * /*aArg*/ )
{
    mTransWaitTime = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTmsIgnoreHintPID( idvSQL * /*aStatistics*/,
                                              SChar  * /*aName*/,
                                              void   * /*aOldValue*/,
                                              void   * aNewValue,
                                              void   * /*aArg*/ )
{
    mTmsIgnoreHintPID = *((UInt *)aNewValue);
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTmsManualSlotNoInItBMP( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mTmsManualSlotNoInItBMP = *(SInt*)aNewValue;
    return IDE_SUCCESS;
}


IDE_RC smuProperty::callbackTmsManualSlotNoInLfBMP( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mTmsManualSlotNoInLfBMP = *(SInt*)aNewValue;
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTmsCandidateLfBMPCnt( idvSQL * /*aStatistics*/,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * /*aArg*/ )
{
    mTmsCandidateLfBMPCnt = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTmsCandidatePageCnt( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mTmsCandidatePageCnt= *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTmsMaxSlotCntPerRtBMP( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    UInt    sNewValue;
    UInt    sMaxValue;

    sNewValue   = *((UInt *)aNewValue);
    sMaxValue   = sdpstBMP::getMaxSlotCnt4Property();

    IDE_DASSERT( sMaxValue > 0 );

    if ( sNewValue > sMaxValue )
    {
        mTmsMaxSlotCntPerRtBMP = sMaxValue;
    }
    else
    {
        mTmsMaxSlotCntPerRtBMP = sNewValue;
    }

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTmsMaxSlotCntPerItBMP( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    UInt    sNewValue;
    UInt    sMaxValue;

    sNewValue   = *((UInt *)aNewValue);
    sMaxValue   = sdpstBMP::getMaxSlotCnt4Property();

    IDE_DASSERT( sMaxValue > 0 );

    if ( sNewValue > sMaxValue )
    {
        mTmsMaxSlotCntPerItBMP = sMaxValue;
    }
    else
    {
        mTmsMaxSlotCntPerItBMP = sNewValue;
    }

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTmsMaxSlotCntPerExtDir( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    UInt    sNewValue;
    UInt    sMaxValue;

    sNewValue   = *((UInt *)aNewValue);
    sMaxValue   = sdpstExtDir::getMaxSlotCnt4Property();

    IDE_DASSERT( sMaxValue > 0 );

    if ( sNewValue > sMaxValue )
    {
        mTmsMaxSlotCntPerExtDir = sMaxValue;
    }
    else
    {
        mTmsMaxSlotCntPerExtDir = sNewValue;
    }

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackExtDescCntInExtDirPage( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mExtDescCntInExtDirPage = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackDefaulExtCntForExtentGroup( idvSQL * /*aStatistics*/,
                                                        SChar  * /*aName*/,
                                                        void   * /*aOldValue*/,
                                                        void   * aNewValue,
                                                        void   * /*aArg*/ )
{
    mDefaultExtCntForExtentGroup = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackSetAgerCount( idvSQL * /*aStatistics*/,
                                          SChar  * /*aName*/,
                                          void   * /*aOldValue*/,
                                          void   * aNewValue,
                                          void   * /*aArg*/ )
{
    UInt sAgerCount ;

    sAgerCount = *((UInt*)aNewValue);


    IDE_TEST_RAISE( sAgerCount > smuProperty::getMaxLogicalAgerCount(),
                    err_ager_count_out_of_max_count );
    IDE_TEST_RAISE( sAgerCount < smuProperty::getMinLogicalAgerCount(),
                    err_ager_count_out_of_min_count );

    IDE_TEST( smaLogicalAger::changeAgerCount( sAgerCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_ager_count_out_of_max_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AGER_COUNT_OUT_OF_MAX_COUNT,
                                sAgerCount,
                                smuProperty::getMaxLogicalAgerCount() ));
    }
    IDE_EXCEPTION(err_ager_count_out_of_min_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AGER_COUNT_OUT_OF_MIN_COUNT,
                                sAgerCount,
                                smuProperty::getMinLogicalAgerCount() ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackTransTouchPageCntByNode( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mTransTouchPageCntByNode = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTransTouchPageCacheRatio( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mTransTouchPageCacheRatio = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTableCompactAtShutdown( idvSQL * /*aStatistics*/,
                                                    SChar  * /*aName*/,
                                                    void   * /*aOldValue*/,
                                                    void   * aNewValue,
                                                    void   * /*aArg*/ )
{
    mTableCompactAtShutdown = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// PROJ-1704 Disk MVCC 리뉴얼
IDE_RC smuProperty::callbackTSSegSizeShrinkThreshold( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mTSSegSizeShrinkThreshold = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackUDSegSizeShrinkThreshold( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mUDSegSizeShrinkThreshold = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackRebuildMinViewSCNInterval( idvSQL * /*aStatistics*/,
                                                       SChar  * /*aName*/,
                                                       void   * /*aOldValue*/,
                                                       void   * aNewValue,
                                                       void   * /*aArg*/ )
{
    mRebuildMinViewSCNInterval = *((UInt *)aNewValue);

    smxTransMgr::resetBuilderInterval();

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackRetryStealCount( idvSQL * /*aStatistics*/,
                                             SChar  * /*aName*/,
                                             void   * /*aOldValue*/,
                                             void   * aNewValue,
                                             void   * /*aArg*/ )
{
    mRetryStealCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

// BUG-27126 INDEX_BUILD_THREAD_COUNT를 alter system 으로 변경가능 해야...
IDE_RC smuProperty::callbackIndexBuildThreadCount( idvSQL * /*aStatistics*/,
                                                   SChar  * /*aName*/,
                                                   void   * /*aOldValue*/,
                                                   void   * aNewValue,
                                                   void   * /*aArg*/ )
{
    mIndexBuildThreadCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* PROJ-40509 */
IDE_RC smuProperty::callbackMemoryIndexUnbalancedSplitRate( idvSQL  * /*aStatistics*/,
                                                            SChar   * /*aName*/,
                                                            void    * /*aOldValue*/,
                                                            void    * aNewValue,
                                                            void    * /*aArg*/ )
{
    mMemoryIndexUnbalancedSplitRate = *((UInt *)aNewValue);
    smnbBTree::setNodeSplitRate();

    return IDE_SUCCESS;
}

/* Proj-2059 DB Upgrade 기능 */
IDE_RC smuProperty::callbackDataPortFileBlockSize  ( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    UInt sNewValue;

    sNewValue = *((UInt *)aNewValue);

    IDE_TEST_RAISE( smuProperty::getExportColumnChainingThreshold() * 2 
                    > sNewValue,
                    ERR_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE );

    // DirectIO를 위해 Align을 맞춰줘야 합니다.
    mDataPortFileBlockSize = idlOS::align( *((UInt *)aNewValue),
                                           ID_MAX_DIO_PAGE_SIZE );
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE );
    {
        IDE_SET(ideSetErrorCode(
                smERR_ABORT_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE,
                smuProperty::getExportColumnChainingThreshold(),
                sNewValue ) ) ;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackExportColumnChainingThreshold  ( idvSQL * /*aStatistics*/,
                                                             SChar  * /*aName*/,
                                                             void   * /*aOldValue*/,
                                                             void   * aNewValue,
                                                             void   * /*aArg*/ )
{
    UInt sNewValue;

    sNewValue = *((UInt *)aNewValue);

    IDE_TEST_RAISE( sNewValue * 2 > smuProperty::getDataPortFileBlockSize(),
                    ERR_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE );

    mExportColumnChainingThreshold = sNewValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_COLUMN_CHAINING_THRESHOLD_LAGER_THAN_BLOCK_SIZE,
                                sNewValue, smuProperty::getDataPortFileBlockSize() ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smuProperty::callbackDataPortDirectIOEnable ( idvSQL * /*aStatistics*/,
                                                     SChar  * /*aName*/,
                                                     void   * /*aOldValue*/,
                                                     void   * aNewValue,
                                                     void   * /*aArg*/ )
{
    mDataPortDirectIOEnable = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackTRCLogLegacyTxInfo( idvSQL * /*aStatistics*/,
                                                SChar  * /*aName*/,
                                                void   * /*aOldValue*/,
                                                void   * aNewValue,
                                                void   * /*aArg*/ )
{
    mTrcLogLegacyTxInfo = *((UInt *)aNewValue);
 
    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLobCursorHashBucketCount( idvSQL * /*aStatistics*/,
                                                      SChar  * /*aName*/,
                                                      void   * /*aOldValue*/,
                                                      void   * aNewValue,
                                                      void   * /*aArg*/ )
{
    mLobCursorHashBucketCount = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackBackupInfoRetentionPeriodForTest( idvSQL * /*aStatistics*/,
                                                              SChar  * /*aName*/,
                                                              void   * /*aOldValue*/,
                                                              void   * aNewValue,
                                                              void   * /*aArg*/ )
{
    mBackupInfoRetentionPeriodForTest = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* PROJ-2613 Key Redistribution in MRDB Index */
IDE_RC smuProperty::callbackMemIndexKeyRedistribution( idvSQL * /*aStatistics*/,
                                                       SChar  * /*aName*/,
                                                       void   * /*aOldValue*/,
                                                       void   * aNewValue,
                                                       void   * /*aArg*/ )
{
    mMemIndexKeyRedistribution = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* PROJ-2613 Key Redistribution in MRDB Index */
IDE_RC smuProperty::callbackMemIndexKeyRedistributionStandardRate( idvSQL * /*aStatistics*/,
                                                                   SChar  * /*aName*/,
                                                                   void   * /*aOldValue*/,
                                                                   void   * aNewValue,
                                                                   void   * /*aArg*/ )
{
    mMemIndexKeyRedistributionStandardRate = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackScanlistMoveNonBlock( idvSQL * /*aStatistics*/,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * /*aArg*/ )
{
    mScanlistMoveNonBlock= ( *((UInt *)aNewValue) == 1 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackGatherIndexStatOnDDL( idvSQL * /*aStatistics*/,
                                                  SChar  * /*aName*/,
                                                  void   * /*aOldValue*/,
                                                  void   * aNewValue,
                                                  void   * /*aArg*/ )
{
    mGatherIndexStatOnDDL = ( *((UInt *)aNewValue) == 1 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackLogCompAcceleration( idvSQL * /*aStatistics*/,
                                                 SChar  * /*aName*/,
                                                 void   * /*aOldValue*/,
                                                 void   * aNewValue,
                                                 void   * /*aArg*/ )
{
    mLogCompAcceleration = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

/* PROJ-2734 */
IDE_RC smuProperty::callbackDistributionDeadlockEnable( idvSQL * /*aStatistics*/,
                                                        SChar  * /*aName*/,
                                                        void   * /*aOldValue*/,
                                                        void   * aNewValue,
                                                        void   * /*aArg*/ )
{
    mDistributionDeadlockEnable = ( *((UInt *)aNewValue) == 1 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;
}
IDE_RC smuProperty::callbackDistributionDeadlockRiskLowWaitTime( idvSQL * /*aStatistics*/,
                                                                 SChar  * /*aName*/,
                                                                 void   * /*aOldValue*/,
                                                                 void   * aNewValue,
                                                                 void   * /*aArg*/ )
{
    mDistributionDeadlockRiskLowWaitTime = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}
IDE_RC smuProperty::callbackDistributionDeadlockRiskMidWaitTime( idvSQL * /*aStatistics*/,
                                                                 SChar  * /*aName*/,
                                                                 void   * /*aOldValue*/,
                                                                 void   * aNewValue,
                                                                 void   * /*aArg*/ )
{
    mDistributionDeadlockRiskMidWaitTime = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}
IDE_RC smuProperty::callbackDistributionDeadlockRiskHighWaitTime( idvSQL * /*aStatistics*/,
                                                                  SChar  * /*aName*/,
                                                                  void   * /*aOldValue*/,
                                                                  void   * aNewValue,
                                                                  void   * /*aArg*/ )
{
    mDistributionDeadlockRiskHighWaitTime = *((ULong *)aNewValue);

    return IDE_SUCCESS;
}

IDE_RC smuProperty::callbackVersioningMinTime( idvSQL * /*aStatistics*/,
                                               SChar  * /*aName*/,
                                               void   * /*aOldValue*/,
                                               void   * aNewValue,
                                               void   * /*aArg*/ )
{
    mVersioningMinTime = *((ULong *)aNewValue);

    smxTransMgr::resetVersioningMinTime();

    return IDE_SUCCESS;
}
