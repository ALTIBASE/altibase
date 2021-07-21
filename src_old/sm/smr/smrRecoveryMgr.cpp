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
 * $Id: smrRecoveryMgr.cpp 91224 2021-07-14 05:36:12Z minku.kang $
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <svm.h>
#include <smu.h>
#include <sdb.h>
#include <sct.h>
#include <smr.h>
#include <sds.h>
#include <smx.h>
#include <sdr.h>
#include <smrReq.h>

smrLogAnchorMgr    smrRecoveryMgr::mAnchorMgr;
idBool             smrRecoveryMgr::mFinish;
idBool             smrRecoveryMgr::mABShutDown;
idBool             smrRecoveryMgr::mRestart;
idBool             smrRecoveryMgr::mRedo;
smGetMinSN         smrRecoveryMgr::mGetMinSNFunc;
smLSN              smrRecoveryMgr::mIdxSMOLSN;
iduMutex           smrRecoveryMgr::mDeleteLogFileMutex;
idBool             smrRecoveryMgr::mRestartRecoveryPhase = ID_FALSE;
idBool             smrRecoveryMgr::mMediaRecoveryPhase   = ID_FALSE;
smManageDtxInfo    smrRecoveryMgr::mManageDtxInfoFunc;
smGetDtxMinLSN     smrRecoveryMgr::mGetDtxMinLSNFunc;

// PR-14912
idBool             smrRecoveryMgr::mRefineDRDBIdx;

/* BUG-42785 */
SInt               smrRecoveryMgr::mOnlineDRDBRedoCnt;

/* PROJ-2118 BUG Reporting - Debug Info for Fatal */
smLSN              smrRecoveryMgr::mLstRedoLSN    = { 0,0 };
smLSN              smrRecoveryMgr::mLstUndoLSN    = { 0,0 };
SChar*             smrRecoveryMgr::mCurLogPtr     = NULL;
smrLogHead*        smrRecoveryMgr::mCurLogHeadPtr = NULL;

/* PROJ-2162 RestartRiskReduction */
smrEmergencyRecoveryPolicy smrRecoveryMgr::mEmerRecovPolicy;
idBool                     smrRecoveryMgr::mDurability        = ID_TRUE;
idBool                     smrRecoveryMgr::mDRDBConsistency   = ID_TRUE;
idBool                     smrRecoveryMgr::mMRDBConsistency   = ID_TRUE;
idBool                     smrRecoveryMgr::mLogFileContinuity = ID_TRUE;
SChar                      smrRecoveryMgr::mLostLogFile[SM_MAX_FILE_NAME];
smLSN                      smrRecoveryMgr::mEndChkptLSN;
smLSN                      smrRecoveryMgr::mLstDRDBRedoLSN;
iduMutex                   smrRecoveryMgr::mIOLMutex;
smrRTOI                    smrRecoveryMgr::mIOLHead;
UInt                       smrRecoveryMgr::mIOLCount;

/* PROJ-2133 incremental backup */
UInt        smrRecoveryMgr::mCurrMediaTime;
SChar       smrRecoveryMgr::mLastRestoredTagName[ SMI_MAX_BACKUP_TAG_NAME_LEN ];

smLSN       smrRecoveryMgr::mSkipRedoLSN       = { 0, 0 };

UInt        smrRecoveryMgr::mLastRemovedFileNo;

// * REPLICATION concern function where   * //
// * stubs make not NULL function forever * //

UInt   stubGetMinFileNoFunc() { return ID_UINT_MAX; };
IDE_RC stubIsReplCompleteBeforeCommitFunc( idvSQL *, 
                                           const  smTID, 
                                           const  smSN, 
                                           const  UInt ) 
{ return IDE_SUCCESS; };

void   stubIsReplCompleteAfterCommitFunc( idvSQL *, 
                                          const smTID, 
                                          const smSN,
                                          const smSN, 
                                          const UInt, 
                                          const smiCallOrderInCommitFunc ) 
{ return; };
void   stubCopyToRPLogBufFunc(idvSQL*, UInt, SChar*, smLSN){ return; };
/* PROJ-2453 Eager Replication performance enhancement */
void   stubSendXLogFunc( const SChar* )
{ 
    return; 
};

IDE_RC stubIsReplWaitGlobalTxAfterPrepareFunc( idvSQL       * /* aStatistics */,
                                               idBool         /* aIsRequestNode */,
                                               const smTID    /* aTID */ ,
                                               const smSN     /* aSN */ )
{
    return IDE_SUCCESS;
};

smIsReplCompleteBeforeCommit smrRecoveryMgr::mIsReplCompleteBeforeCommitFunc = stubIsReplCompleteBeforeCommitFunc;
smIsReplCompleteAfterCommit  smrRecoveryMgr::mIsReplCompleteAfterCommitFunc = stubIsReplCompleteAfterCommitFunc;
//PROJ-1670: Log Buffer for Replication
smCopyToRPLogBuf smrRecoveryMgr::mCopyToRPLogBufFunc = stubCopyToRPLogBufFunc;
/* PROJ-2453 Eager Replication performance enhancement */
smSendXLog smrRecoveryMgr::mSendXLogFunc = stubSendXLogFunc;
/* PROJ-2747 Global Tx Consistent */
smIsReplWaitGlobalTxAfterPrepare smrRecoveryMgr::mIsReplWaitGlobalTxAfterPrepareFunc = stubIsReplWaitGlobalTxAfterPrepareFunc;

void smrRecoveryMgr::setCallbackFunction(
                            smGetMinSN                   aGetMinSNFunc,
                            smIsReplCompleteBeforeCommit aIsReplCompleteBeforeCommitFunc,
                            smIsReplCompleteAfterCommit  aIsReplCompleteAfterCommitFunc,
                            smCopyToRPLogBuf             aCopyToRPLogBufFunc,
                            smSendXLog                   aSendXLogFunc,
                            smIsReplWaitGlobalTxAfterPrepare aIsReplWaitGlobalTxAfterPrepareFunc )
{
    mGetMinSNFunc   = aGetMinSNFunc;

    if ( aIsReplCompleteBeforeCommitFunc != NULL)
    {
        mIsReplCompleteBeforeCommitFunc = aIsReplCompleteBeforeCommitFunc;
    }
    else
    {
        mIsReplCompleteBeforeCommitFunc = stubIsReplCompleteBeforeCommitFunc;
    }
    if ( aIsReplCompleteAfterCommitFunc != NULL)
    {
        mIsReplCompleteAfterCommitFunc = aIsReplCompleteAfterCommitFunc;
    }
    else
    {
        mIsReplCompleteAfterCommitFunc = stubIsReplCompleteAfterCommitFunc;
    }
    /*PROJ-1670: Log Buffer for Replication*/
    /* BUG-35392 */
    mCopyToRPLogBufFunc = aCopyToRPLogBufFunc;

    /* PROJ-2453 Eager Replication performance enhancement */
    if ( aSendXLogFunc != NULL )
    {
        mSendXLogFunc = aSendXLogFunc;
    }
    else
    {
        mSendXLogFunc = stubSendXLogFunc;
    }

    if ( aIsReplWaitGlobalTxAfterPrepareFunc != NULL )
    {
        mIsReplWaitGlobalTxAfterPrepareFunc = aIsReplWaitGlobalTxAfterPrepareFunc;
    }
    else
    {
        mIsReplWaitGlobalTxAfterPrepareFunc = stubIsReplWaitGlobalTxAfterPrepareFunc;
    }
}

/***********************************************************************
 * Description : createdb과정에서 로그 관련 작업 수행
 *
 * createdb 과정에서 호출되는 함수로써 로그앵커 생성 및
 * 첫번째 로그파일(logfile0)을 생성하고, 복구 관리자를
 * 초기화하며, 로그파일 관리자 thread를 시작한다.
 **********************************************************************/
IDE_RC smrRecoveryMgr::create()
{
    /* create loganchor file */
    IDE_TEST( mAnchorMgr.create() != IDE_SUCCESS );
    /* create log files */
    IDE_TEST( smrLogMgr::create() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 복구 관리자 초기화
 *
 * A4에서는 다중 로그앵커파일(3개의 복사본유지)을 지원한다.
 * 그러므로, 복구관리자 초기화과정에서 로그앵커 파일을
 * 읽어서 메모리에 적재하는 과정을 처리하기 전에 여러 개의
 * 로그앵커 파일 중 시스템 구동에 사용할 유효한 로그앵커
 * 를 선택하는 과정이 필요하다.
 *
 * - 2nd. code design
 *   + MRDB와 DRDB의 redo/undo 함수 포인터 map을 초기화한다.
 *   + loganchor 관리자를 초기화한다.
 *   + 로그파일을 준비하고, 로그파일 관리자(smrLogFileMgr) thread를
 *     시작한다.
 *   + 로그파일 Sync thread를 초기화하고, dirty page list
 *     (smrDirtyPageList)를 초기화한다.
 **********************************************************************/
IDE_RC smrRecoveryMgr::initialize()
{
    mGetMinSNFunc       = NULL;
    /* mManageDtxInfoFunc and mGetDtxMinLSNFunc are initailized at PRE-PROCESS phase */

    IDE_TEST( mDeleteLogFileMutex.initialize( (SChar*) "Delete LogFile Mutex",
                                             IDU_MUTEX_KIND_POSIX,
                                             IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );

    /* BUG-42785 mOnlineDRDBRedoCnt는 현재 OnlineDRDBRedo를 수행하고 있는
     * 트랜잭션의 수를 나타낸다. */
    mOnlineDRDBRedoCnt = 0;

    /* ------------------------------------------------
     * MRDB와 DRDB의 redo/undo 함수 포인터 map 초기화
     * ----------------------------------------------*/
    smrUpdate::initialize();
    smLayerCallback::initRecFuncMap();

    /* PROJ-2162 RestartRiskReduction */
    mDurability          = ID_TRUE;
    mDRDBConsistency     = ID_TRUE;
    mMRDBConsistency     = ID_TRUE;
    mLogFileContinuity   = ID_TRUE;

    SM_LSN_MAX( mEndChkptLSN ); // BUG-47404 이후 WAL확인을 위해 UINT_MAX 로 설정함.
    SM_LSN_INIT( mLstRedoLSN );

    IDE_TEST( initializeIOL() != IDE_SUCCESS );

    /* ------------------------------------------------
     * loganchor 관리자 초기화과정에서 invalid한
     * loganchor 파일(들)에 대해서 valid한 정보를
     * flush 한다.
     * ----------------------------------------------*/
    IDE_TEST( mAnchorMgr.initialize() != IDE_SUCCESS );

    /* PROJ-2133 incremental backup */
    if ( isCTMgrEnabled() == ID_TRUE )
    {
        IDE_TEST( smriChangeTrackingMgr::begin() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( getBIMgrState() != SMRI_BI_MGR_FILE_REMOVED )
    {
        IDE_TEST( smriBackupInfoMgr::begin() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* PROJ-2133 incremental backup */
    mCurrMediaTime        = ID_UINT_MAX;
    initLastRestoredTagName();

    mFinish               = ID_FALSE;
    mRestart              = ID_FALSE;
    mRedo                 = ID_FALSE;
    mMediaRecoveryPhase   = ID_FALSE; /* 미디어 복구 수행중에만 TRUE */
    mRestartRecoveryPhase = ID_FALSE;

    mRefineDRDBIdx = ID_FALSE;

    mLastRemovedFileNo = 0;

    SM_SET_LSN(mIdxSMOLSN, 0, 0);

    // 각 Tablespace별 dirty page관리자를 관리하는 객체 초기화
    IDE_TEST( smrDPListMgr::initializeStatic() != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::initializeCorruptPageMgr( sdbBufferMgr::getPageCount() )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description : Server Shutdown시 복구관리자 종료작업 수행
 *
 * 서버종료과정에서 진행되던 백업연산이 존재한다면 모두 취소시키고, 재시작 회복을
 * 빠르게 하기 위해서 모든 Dirty 페이지를 모두 내리고, 체크포인트를 수행한다.
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::finalize()
{
    sdbCPListSet * sCPListSet;
    UInt           sBCBCount;
    smLSN          sEndLSN;
    UInt           sLstCreatedLogFile;

    mFinish    = ID_TRUE;

    if ( smrBackupMgr::getBackupState() != SMR_BACKUP_NONE )
    {
        sddDiskMgr::abortBackupAllTableSpace( NULL /* idvSQL* */); 
    }

    if ( smrLogMgr::getLogFileMgr().isStarted() == ID_TRUE )
    {
        /* PROJ-2102 */
        IDE_TEST( sdsFlushMgr::turnOnAllflushers() != IDE_SUCCESS );
        
        IDE_TEST( sdsBufferMgr::flushDirtyPagesInCPList( NULL,
                                                         ID_TRUE )// FLUSH ALL
                  != IDE_SUCCESS );

        /* To fix BUG-22679 */
        IDE_TEST( sdbFlushMgr::turnOnAllflushers() != IDE_SUCCESS );

        // To fix BUG-17953
        // checkpoint -> checkpoint -> flush의 순서를
        // flush -> checkpoint -> checkpoint로 변경
        IDE_TEST( sdbBufferMgr::flushDirtyPagesInCPList(NULL,
                                                       ID_TRUE)// FLUSH ALL
                 != IDE_SUCCESS );

        IDE_TEST( checkpoint( NULL, /* idvSQL* */
                              SMR_CHECKPOINT_BY_SYSTEM,
                              SMR_CHKPT_TYPE_BOTH,
                              ID_FALSE,
                              ID_TRUE )
                  != IDE_SUCCESS );

        IDE_TEST( checkpoint( NULL, /* idvSQL* */
                              SMR_CHECKPOINT_BY_SYSTEM,
                              SMR_CHKPT_TYPE_BOTH,
                              ID_FALSE,
                              ID_TRUE)
                  != IDE_SUCCESS );

        sCPListSet = sdbBufferMgr::getPool()->getCPListSet();
        sBCBCount  = sCPListSet->getTotalBCBCnt();

        /* BUG-40139 The server can`t realize that there is dirty pages in buffer 
         * when the server shutdown in normal 
         * 정상종료중 Checkpoint List에 BCB가 남아있다면(dirty page)가
         * 존재한다면 비정상 종료 하여 restart recovery가 동작하게 한다.*/
        IDE_ASSERT_MSG( sBCBCount == 0,
                        "Forced shutdown due to remaining dirty pages in CPList(%"ID_UINT32_FMT")\n", 
                        sBCBCount);

        smrLogMgr::getLstLSN( &sEndLSN );

        // Loganchor의 EndLSN은 checkpoint시의 Memory RedoLSN 또는
        // Shutdown시의 EndLSN으로 사용되기때문에 Shutdown시에도
        // 체크포인트이미지 메타헤더에 정보를 갱신해주어야 한다.
        sctTableSpaceMgr::setRedoLSN4DBFileMetaHdr( NULL,       // Disk Redo LSN
                                                    &sEndLSN ); // Mem Redo LSN

        // 체크포인트이미지의 메타헤더에 RedoLSN을 기록한다.
        IDE_TEST( smmTBSMediaRecovery::updateDBFileHdr4AllTBS()
                  != IDE_SUCCESS );

        /* ------------------------------------------------
         * loganchor 갱신
         * - 서버 shutdown 상태 변경
         * - last LSN 설정
         * - last create log 번호 설정
         * ----------------------------------------------*/
        smrLogMgr::getLogFileMgr().getLstLogFileNo( &sLstCreatedLogFile );

        IDE_TEST( mAnchorMgr.updateSVRStateAndFlush( SMR_SERVER_SHUTDOWN,
                                                     &sEndLSN,
                                                     &sLstCreatedLogFile )
                  != IDE_SUCCESS );

        IDE_TEST( updateAnchorAllTBS() != IDE_SUCCESS );
        /* PROJ-2102 Fast Secondary Buffer */
        IDE_TEST( updateAnchorAllSB() != IDE_SUCCESS ); 
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 복구 관리자 해제
 *
 * 서버종료과정에서 로그 관련 자원을 해제하고, 로그앵커에 정상종료됨을 설정한다.
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::destroy()
{
    IDE_TEST( smLayerCallback::destroyCorruptPageMgr() != IDE_SUCCESS );

    IDE_TEST( smrDPListMgr::destroyStatic() != IDE_SUCCESS );

    //PROJ-2133 incremental backup
    if ( isCTMgrEnabled() == ID_TRUE )
    {  
        IDE_TEST( smriChangeTrackingMgr::end() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }
 
    if ( getBIMgrState() == SMRI_BI_MGR_INITIALIZED )
    {
        IDE_TEST( smriBackupInfoMgr::end() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    IDE_TEST( mAnchorMgr.destroy() != IDE_SUCCESS );

    IDE_TEST( finalizeIOL() != IDE_SUCCESS );

    IDE_TEST( mDeleteLogFileMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : restart 수행
 *
 * restart 함수 기능 분리 다음과 같이 4가지 함수로 분리한다.
 *
 * - initialize restart 과정
 *   + restart 처리중 표시
 *   + 필요없는 logfile들을 삭제
 *   + stableDB에 대하여 prepareDB->restoreDB
 *   + 이전 shutdown 상태에 따라 recovery가 필요한가
 *     판단
 * - restart normal 과정
 *   + 이전에 정상종료를 하였기때문에, 복구가 필요없는 경우임
 *   + 로그앵커 서버상태 updateFlush
 *   + log 파일 관리자 깨움
 *
 * - restart recovery 과정
 *   + active transaction list
 *   + redoAll 처리
 *   + DRDB table에 대하여 index header 초기화
 *   + undoAll 처리
 *
 * - finalize restart 과정
 *   + flag 설정 및 마지막 생성 dbfile 개수 설정
 **********************************************************************/
IDE_RC smrRecoveryMgr::restart( UInt aPolicy )
{
    idBool                sNeedRecovery;

    // fix BUG-17158
    // offline DBF에 write가 가능하도록 flag 를 설정한다.
    sddDiskMgr::setEnableWriteToOfflineDBF( ID_TRUE );

    /* ------------------------------------------------
     * 1. restart 준비 과정 -> smrRecoveryMgr::initRestart()
     *
     * - restart 처리중 표시
     * - 필요없는 logfile들을 삭제
     * - stableDB에 대하여 prepareDB->restoreDB
     * - 이전 shutdown 상태에 따라 recovery가 필요한가
     *   판단
     * ----------------------------------------------*/
    IDE_TEST( initRestart( &sNeedRecovery ) != IDE_SUCCESS );

    /* ------------------------------------------------
     * BUG-12246 서버 구동시 ARCHIVE할 로그파일을 구해야함
     * 1) 서버가 비정상종료한 경우
     * lst delete logfile no 부터 recovery lsn까지 archive dir을
     * 검사하여 존재하지 않는 로그파일만 다시 archive list에 등록한다.
     * 그 이후는 redoAll 단계에서 등록하도록 한다.
     * 2) 서버가 정상종료한 경우
     * lst delete logfile no부터 end lsn까지 archive dir을 검사하여
     * 존재하지 않는 로그파일만 다시 archive list에 등록한다.
     * ----------------------------------------------*/

    /* PROJ-2162 RestartRiskReduction */
    mEmerRecovPolicy = (smrEmergencyRecoveryPolicy)aPolicy;
    if ( mEmerRecovPolicy == SMR_RECOVERY_SKIP )
    {
        IDE_CALLBACK_SEND_MSG(
            "----------------------------------------"
            "----------------------------------------\n"
            "                            "
            "Emergency - skip restart recovery\n"
            "----------------------------------------"
            "----------------------------------------\n");

        sNeedRecovery    = ID_FALSE; /* Recovery 안함 */

        IDE_TEST( prepare4DBInconsistencySetting() != IDE_SUCCESS );
        mMRDBConsistency = ID_FALSE; /* DB Consistency등이 깨짐 */
        mDRDBConsistency = ID_FALSE;
        mDurability      = ID_FALSE;
        IDE_TEST( finish4DBInconsistencySetting() != IDE_SUCCESS );
    }
    
    /* PROJ-2102 Fast Secondary Buffer 에 따른 BCB 재구축 */    
    IDE_TEST( sdsBufferMgr::restart( sNeedRecovery ) != IDE_SUCCESS );

    if (sNeedRecovery != ID_TRUE) /* 복구가 필요없다면 */
    {
        /* ------------------------------------------------
         * 2-1. 정상 구동 -> smrRecoveryMgr::restartNormal()
         *
         * - 이전에 정상종료를 하였기때문에, 복구가 필요없는 경우임
         * - 로그앵커 서버상태 updateFlush
         * - log 파일 관리자 깨움
         * - DRDB table에 대하여 index header 초기화
         * ----------------------------------------------*/
        IDE_TEST( restartNormal() != IDE_SUCCESS );
    }
    else /* 복구가 필요하다면 */
    {
        /* ------------------------------------------------
         * 2-2. restart recovery ->smrRecoveryMgr::restartRecovery()
         * - active transaction list
         * - redoAll 처리
         * - DRDB table에 대하여 index header 초기화
         * - undoAll 처리
         * ----------------------------------------------*/
        IDE_TEST( restartRecovery() != IDE_SUCCESS );
    }

    // fix BUG-17157 [PROJ-1548] Disk Tablespace Online/Offline 수행시
    // 올바르게 Index Runtime Header 해제하지 않음
    // Restart REDO, UNDO끝난후에 Offline 상태인 Disk Tablespace의
    // Index Runtime Header를 해제한다.
    IDE_TEST( finiOfflineTBS(NULL /* idvSQL* */) != IDE_SUCCESS );

    /* ------------------------------------------------
     * 3. smrRecoveryMgr::finalRestart
     * restart 과정 마무리
     * - flag 설정 및 마지막 생성 dbfile 개수 설정
     * ----------------------------------------------*/
    (void)smrRecoveryMgr::finalRestart();

    // offline DBF에 write가 불가능하도록 flag 를 설정한다.
    sddDiskMgr::setEnableWriteToOfflineDBF( ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    sddDiskMgr::setEnableWriteToOfflineDBF( ID_FALSE );

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : restart 준비 과정 수행
 *
 * 시스템 restart 준비 과정의 초기화 작업을 수행한다.
 * 필요없는 로그파일을 제거하고, checkpoint가 반영된 DB를 로딩한 후,
 * 시스템의 이전 종료상태에 따라 복구과정이 필요한지 결정한다.
 *
 * - 2nd. code design
 *   + restart 처리중 표시
 *   + 필요없는 logfile들을 삭제
 *   + stableDB에 대하여 prepareDB,
 *   +  membase의 timestamp를 갱신
 *   + checkpoint가 반영된 이미지로 restoreDB
 *   + 이전 shutdown 상태에 따라 recovery가 필요한가 결정
 **********************************************************************/
IDE_RC smrRecoveryMgr::initRestart( idBool*  aIsNeedRecovery )
{
    UInt        sFstDeleteLogFile;
    UInt        sLstDeleteLogFile;

    mRestart = ID_TRUE;

    /* remove log file that is not needed for recovery */
    mAnchorMgr.getFstDeleteLogFileNo( &sFstDeleteLogFile );
    mAnchorMgr.getLstDeleteLogFileNo( &sLstDeleteLogFile );

    /* BUG-40323 */
    // 실제로 삭제할 파일이 있을때만 삭제 시도
    if ( sFstDeleteLogFile != sLstDeleteLogFile )
    {
        (void)smrLogMgr::getLogFileMgr().removeLogFile( sFstDeleteLogFile,
                                                        sLstDeleteLogFile,
                                                        ID_FALSE /* Not Checkpoint */ );

        // fix BUG-20241 : 삭제한 LogFileNo와 LogAnchor 동기화
        IDE_TEST( mAnchorMgr.updateFstDeleteFileAndFlush()
                  != IDE_SUCCESS );
    }
    else
    {
        // do nothing
    }

    *aIsNeedRecovery = ( ( mAnchorMgr.getStatus() != SMR_SERVER_SHUTDOWN ) ?
                         ID_TRUE : ID_FALSE );

    IDE_CALLBACK_SEND_MSG("  [SM] Recovery Phase - 1 : Preparing Database");

    IDE_TEST( smmManager::prepareDB( ( *aIsNeedRecovery == ID_TRUE ) ?
                                      SMM_PREPARE_OP_DBIMAGE_NEED_RECOVERY :
                                      SMM_PREPARE_OP_NONE)
             != IDE_SUCCESS );

    /* PROJ-1594 Volatile TBS */
    /* Volatile Tablespace들을 초기화한다. */
    IDE_TEST( svmTBSStartupShutdown::prepareAllTBS() != IDE_SUCCESS );

    IDE_CALLBACK_SEND_MSG("  [SM] Recovery Phase - 2 : Loading Database ");

    /*BUG-BUG-17697 Performance Center P-14 Startup 성능측정 결과가 이상함
     *
     *측정 코드에서 "BEGIN DATABASE RESTORATION" 이 메세지를 사용함.
     *없애면 안됨. */
    ideLog::log(IDE_SERVER_0, "     BEGIN DATABASE RESTORATION\n");

    IDE_TEST( smmManager::restoreDB( ( *aIsNeedRecovery == ID_TRUE ) ?
                                    SMM_RESTORE_OP_DBIMAGE_NEED_RECOVERY :
                                    SMM_RESTORE_OP_NONE ) != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0, "     END DATABASE RESTORATION\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 복구가 필요없는 정상 구동
 *
 * system shutdown 종류는 normal, immediate, abort의 3가지로
 * 나누며, 이 중에서 normal과 immediate shutdown에 해당하는
 * 종료방식으로 이전에 종료된 경우에 정상구동한다.
 * 왜냐하면, 모든 세션의 트랜잭션을 완료(강제 철회나 종료대기든지)
 * 때문에 active 트랜잭션이 존재하지 않으며, 또한, 마지막에 checkpoint를
 * 수행하여 종료하기 때문에 dirty page가 모두 disk에 반영되기 때문이다.
 * 마찬가지로 DRDB 또한 정상종료시에는 세션완료후 버퍼 관리자의
 * flush 대상의 모든 page를 디스크에 반영하여 종료하도록 한다.
 *
 * - 2nd. code design
 *   + 이전에 정상종료를 하였기때문에, 복구가 필요없는 경우임
 *   + 로그앵커 서버상태 updateFlush
 *   + log 파일 관리자 깨움
 *   + DRDB table에 대하여 index header 초기화
 **********************************************************************/
IDE_RC smrRecoveryMgr::restartNormal()
{

    smLSN   sEndLSN;
    UInt    sLstCreatedLogFileNo;

    mAnchorMgr.getEndLSN( &sEndLSN );
    mAnchorMgr.getLstCreatedLogFileNo( &sLstCreatedLogFileNo );

    /* Transaction Free List가 Valid한지 검사한다. */
    smLayerCallback::checkFreeTransList();

    /* Archive 대상 Logfile을  Archive List에 추가한다. */
    if ( getArchiveMode() == SMI_LOG_ARCHIVE )
    {
        IDE_TEST( rebuildArchLogfileList( &sEndLSN )
                  != IDE_SUCCESS );
    }

    IDE_CALLBACK_SEND_MSG("  [SM] Recovery Phase - 3 : Skipping Recovery & Starting Threads...");

    IDE_TEST( mAnchorMgr.updateSVRStateAndFlush( SMR_SERVER_STARTED,
                                                 &sEndLSN,
                                                 &sLstCreatedLogFileNo )
             != IDE_SUCCESS );

    /* ------------------------------------------------
     * page의 index SMO No. 초기화에 사용 [정상구동시]
     * Index SMO는 Disk에 대해서만 로그가 기록된다. 따라서
     * Disk의 Lst LSN을 index SMO LSN으로 설정한다.
     * ----------------------------------------------*/
    mIdxSMOLSN = sEndLSN;

    //star log file mgr thread
    ideLog::log(IDE_SERVER_0,"      [SM-PREPARE] Prepare Logfile Thread Start...");
    IDE_TEST( smrLogMgr::startupLogPrepareThread( &sEndLSN,
                                                  sLstCreatedLogFileNo,
                                                  ID_FALSE /* aIsRecovery */ )
             != IDE_SUCCESS );

    /*
     * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
     * space cache에 정확한 값을  세트한다.
     */
    IDE_TEST( sdpTableSpace::refineDRDBSpaceCache() != IDE_SUCCESS );

    // init Disk Table Header & rebuild Index Runtime Header
    IDE_CALLBACK_SEND_MSG("                            Refining Disk Table ");
    IDE_TEST( smLayerCallback::refineDRDBTables() != IDE_SUCCESS );

    /* BUG-27122 __SM_CHECK_DISK_INDEX_INTEGRITY 프로퍼티 활성화시
     * 디스크 인덱스 Integrity 체크를 수행한다. */
    IDE_TEST( smLayerCallback::verifyIndexIntegrityAtStartUp()
              != IDE_SUCCESS );
    mRefineDRDBIdx = ID_TRUE;

    IDE_TEST( updateAnchorAllTBS() != IDE_SUCCESS );
    IDE_TEST( updateAnchorAllSB() != IDE_SUCCESS ); 

    /*
      PRJ-1548 SM - User Memroy TableSpace 개념도입
      서버구동시 복구이후에 모든 테이블스페이스의
      DataFileCount와 TotalPageCount를 계산하여 설정한다.
    */
    sddDiskMgr::calculateFileSizeOfAllTBS();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : restart recovery 수행
 *
 * 시스템 abort 발생시 restart 과정에서 recovery를 수행하며,
 * 이전 checkpoint 반영시점부터 마지막 기록한 로그LSN까지
 * 트랜잭션 재수행을 하며, 완료되지 못한 트랜잭션 목록을 구하여
 * 트랜잭션 철회를 처리하도록 한다.
 *
 * A3나 A4는 memory table의 index는 restart recovery 후에 index를
 * 재생성하기 때문에 트랜잭션 철회단계에서 접근할 일이 없다.
 * 하지만, disk table의 persistent index에 대한 접근이 트랜잭션
 * 철회단계에서 접근하도록 해주어야 하기 때문에, 철회단계 전에
 * disk index의 runtime index에 대해서만 생성해주어야 한다.
 *
 * - 2nd. code design
 *   + active transaction list
 *   + redoAll 처리
 *   + DRDB table에 대하여 index header 초기화
 *   + undoAll 처리
 **********************************************************************/
IDE_RC smrRecoveryMgr::restartRecovery()
{
    mABShutDown = ID_TRUE;

    IDE_CALLBACK_SEND_MSG("  [SM] Recovery Phase - 3 : Starting Recovery");

    /* PROJ-2162 RestartRiskReduction
     * LogFile이 연속적이지 못하면 LogFile에 손상이 있다는 말로, Durability가
     * 깨졌을 가능성이 있음. */
    IDE_TEST_RAISE( ( mEmerRecovPolicy == SMR_RECOVERY_NORMAL ) &&
                    ( mLogFileContinuity == ID_FALSE ),
                    err_lost_logfile );

    /* BUG-29633 Recovery전 모든 Transaction이 End상태인지 점검이 필요합니다.
     * Recovery시에 다른곳에서 사용되는 Active Transaction이 존재해서는 안된다. */
    IDE_TEST_RAISE( smLayerCallback::existActiveTrans() == ID_TRUE,
                    err_exist_active_trans);

    ideLog::log(IDE_SERVER_0, "     BEGIN DATABASE RECOVERY\n");

    IDE_CALLBACK_SEND_MSG("                            Initializing Active Transaction List ");
    smLayerCallback::initActiveTransLst(); // layer callback

    mRestartRecoveryPhase = ID_TRUE;

    //Redo All
    IDE_CALLBACK_SEND_MSG("                            Redo ");

    mRedo = ID_TRUE;
    IDE_TEST( redoAll( NULL /* idvSQL* */) != IDE_SUCCESS );
    mRedo = ID_FALSE;

    /* PROJ-2162 RestartRiskReduction 검증 */
    IDE_TEST_RAISE( ( mEmerRecovPolicy == SMR_RECOVERY_NORMAL ) &&
                    ( mDurability == ID_FALSE ),
                    err_durability );
    IDE_TEST_RAISE( ( mEmerRecovPolicy == SMR_RECOVERY_NORMAL ) &&
                    ( mDRDBConsistency == ID_FALSE ),
                    err_drdb_consistency );
    IDE_TEST_RAISE( ( mEmerRecovPolicy == SMR_RECOVERY_NORMAL ) &&
                    ( mMRDBConsistency == ID_FALSE ),
                    err_mrdb_consistency );

    /*
     * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
     * space cache에 정확한 값을  세트한다.
     */
    IDE_TEST( sdpTableSpace::refineDRDBSpaceCache() != IDE_SUCCESS );

    // Init Disk Index Runtime Header
    // fix BUG-17157 [PROJ-1548] Disk Tablespace Online/Offline 수행시
    // 올바르게 Index Runtime Header 해제하지 않음
    // Refine DRDB 과정과 Online 과정에서 호출될 수 있는데,
    // INVALID_DISK_TBS( DROPPED/DISCARDED ) 인경우에는
    // Skip 하고 있다.
    IDE_CALLBACK_SEND_MSG("                            Refine Disk Table..");
    IDE_TEST( smLayerCallback::refineDRDBTables() != IDE_SUCCESS );

    /* BUG-27122 __SM_CHECK_DISK_INDEX_INTEGRITY 프로퍼티 활성화시
     * 디스크 인덱스 Integrity 체크를 수행한다. */
    IDE_TEST( smLayerCallback::verifyIndexIntegrityAtStartUp()
              != IDE_SUCCESS );
    mRefineDRDBIdx = ID_TRUE;

    /* 미뤄둔 InconsistentFlag 설정 업무를 수행함 */
    applyIOLAtRedoFinish();

    //Undo All
    IDE_CALLBACK_SEND_MSG("                            Undo ");

    IDE_TEST( undoAll(NULL /* idvSQL* */) != IDE_SUCCESS );

    /* PROJ-2162 RestartRiskReduction
     * Undo 후에 다시 검증 */
    IDE_TEST_RAISE( ( mEmerRecovPolicy == SMR_RECOVERY_NORMAL ) &&
                    ( mDurability == ID_FALSE ),
                    err_durability );

    /* Transaction Free List가 Valid한지 검사한다. */
    smLayerCallback::checkFreeTransList();

     //------------------------------------------------------------------
    // PROJ-1665 : Corrupted Page들의 상태를 검사
    // Corrupted Page가 Redo 수행 후 정상이 된 경우와
    // Corrupted Page가 속한 Extent의 상태가 Free인 경우를 제외하고
    // Corrupted page가 존재하는 경우, fatal error 처리
    //------------------------------------------------------------------
    IDE_TEST( smLayerCallback::checkCorruptedPages() != IDE_SUCCESS );

    // Restart REDO, UNDO끝난후에 Offline 상태인 Memory Tablespace의
    // 메모리를 해제 자세한 이유는 finiOfflineTBS의 주석참고
    IDE_TEST( smLayerCallback::finiOfflineTBS() != IDE_SUCCESS );
    // Disk는 RestartNormal이건 Restart Recovery 이건 모두 처리해야하므로
    // restart()에서 일괄처리한다.

    IDE_TEST( updateAnchorAllTBS() != IDE_SUCCESS );
    IDE_TEST( updateAnchorAllSB() !=  IDE_SUCCESS ); 

    /*
      PRJ-1548 SM - User Memroy TableSpace 개념도입
      서버구동시 복구이후에 모든 테이블스페이스의
      DataFileCount와 TotalPageCount를 계산하여 설정한다.
    */
    sddDiskMgr::calculateFileSizeOfAllTBS();

    mRestartRecoveryPhase = ID_FALSE;


#if defined( DEBUG_PAGE_ALLOC_FREE )
    smmFPLManager::dumpAllFPLs();
#endif

    // 데이터베이스 Page가 Free Page인지 Allocated Page인지를 Free List Info Page를
    // 보고 알 수 있는데, 이 영역이 Restart Recovery가 끝나야 복구가 완료되므로,
    // Restart Recovery가 완료된 후에 Free Page인지 여부를 분간할 수 있다.

    // Restart Recovery전에는 Free Page인지, Allocated Page인지 구분하지 않고
    // 무조건 페이지를 디스크로부터 메모리로 로드한다.
    // 그리고 Restart Recovery가 완료되고 나면, 불필요하게 로드된 Free Page들의
    // Page 메모리를 메모리를 반납한다.

    IDE_TEST( smmManager::freeAllFreePageMemory() != IDE_SUCCESS );


    ideLog::log(IDE_SERVER_0, "     END DATABASE RECOVERY\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_lost_logfile );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_LOG_FILE_MISSING, mLostLogFile ));
    }
    IDE_EXCEPTION( err_durability );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FAILURE_DURABILITY_AT_STARTUP ));
    }
    IDE_EXCEPTION( err_drdb_consistency );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FAILURE_DRDB_WAL_AT_STARTUP ));
    }
    IDE_EXCEPTION( err_mrdb_consistency );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FAILURE_MRDB_WAL_AT_STARTUP ));
    }
    IDE_EXCEPTION( err_exist_active_trans);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_EXIST_ACTIVE_TRANS_IN_RECOV ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : restart 과정 마무리
 * - 2nd. code design
 *   + flag 설정 및 마지막 생성 dbfile 개수 설정
 **********************************************************************/
void smrRecoveryMgr::finalRestart()
{
    smmManager::setLstCreatedDBFileToAllTBS();

    mRestart = ID_FALSE;

    smmDatabase::makeMembaseBackup();

    return;
}

/*
  SKIP할 REDO로그인지 여부를 리턴한다.

  - MEDIA Recovery시
  - DISCARD, DROP된 Tablespace에 대해 Redo Skip실시
  - Restart Recovery시
  - DISCARD, DROP, OFFLINE된 Tablespace에 대해 Redo Skip실시

  단, Tablespace 상태변경에 대한 로그는 Tablespace의 상태와
  관계없이 REDO한다.
*/
idBool smrRecoveryMgr::isSkipRedo( scSpaceID aSpaceID,
                                   idBool    aUsingTBSAttr )
{
    idBool      sRet;
    sctStateSet sSSet4RedoSkip;

    if ( isMediaRecoveryPhase() == ID_TRUE )
    {
        sSSet4RedoSkip = SCT_SS_SKIP_MEDIA_REDO;
    }
    else
    {
        sSSet4RedoSkip = SCT_SS_SKIP_REDO;
    }

    // Redo SKIP할 Tablespace에 관한 로그인지 체크
    if ( sctTableSpaceMgr::hasState( aSpaceID,
                                     sSSet4RedoSkip,
                                     aUsingTBSAttr ) == ID_TRUE )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    //it could be dropped.... check again using tbs node.
    if ( ( sRet == ID_FALSE ) && ( aUsingTBSAttr == ID_TRUE ) )
    {
         sRet = sctTableSpaceMgr::hasState( aSpaceID,
                                            sSSet4RedoSkip );
    }

    return sRet;
}

/***********************************************************************
 * Description : redo a log
 * [IN] aLogSizeAtDisk - 로그파일상의 로그 크기
 **********************************************************************/
IDE_RC smrRecoveryMgr::redo( void       * aCurTrans,
                             smLSN      * aCurLSN,
                             UInt       * aFileCount,
                             smrLogHead * aLogHead,
                             SChar      * aLogPtr,
                             UInt         aLogSizeAtDisk,
                             idBool       aAfterChkpt )
{
    UInt                i;
    SChar             * sRedoBuffer;
    UInt                sRedoSize;
    SChar             * sAfterImage;
    SChar             * sBeforeImage;
    ULong               sLogBuffer[ (SM_PAGE_SIZE * 2) / ID_SIZEOF(ULong) ];
    smmPCH            * sPCH;
    scGRID            * sArrPageGRID;
    scSpaceID           sSpaceID;
    scPageID            sPageID;
    UInt                sDirtyPage;
    smLSN               sBeginLSN;
    smLSN               sEndLSN;
    smLSN               sCurLSN;                            /* PROJ-1923 */
    smrNTALog           sNTALog;
    smrCMPSLog          sCMPSLog;
    smrUpdateLog        sUpdateLog;
    smrXaPrepareLog     sXAPrepareLog;
    smrTBSUptLog        sTBSUptLog;
    smrXaSegsLog        sXaSegsLog;
    smrTransCommitLog   sCommitLog;
    smrTransAbortLog    sAbortLog;
    smOID               sTableOID;
    idBool              sIsDeleteBit;
    void              * sTrans;
    idBool              sIsExistTBS;
    idBool              sIsFailureDBF;
    UInt                sMemRedoSize;
    smrLogType          sLogType;
    void              * sDRDBRedoLogDataList;
    SChar             * sDummyPtr;
    idBool              sIsDiscarded;

    /* BUG-47525 Group Commit */
    void                   * sGCTrans = NULL;
    UInt                     sGCCnt;
    smTID                    sTID;
 
    IDE_DASSERT( aLogHead   != NULL );
    IDE_DASSERT( aLogPtr    != NULL );

    
    sTrans  = NULL;
    SM_GET_LSN(sBeginLSN, *aCurLSN);
    SM_GET_LSN(sEndLSN, *aCurLSN);
    SM_GET_LSN(sCurLSN, *aCurLSN);

    sEndLSN.mOffset += aLogSizeAtDisk;
    sLogType         = smrLogHeadI::getType(aLogHead);

    switch( sLogType )
    {
        /* ------------------------------------------------
         * ## drdb 로그의 parsing 및 hash에 저장
         * disk 로그를 판독한 경우 body에 저장되어 있는
         * disk redo로그들을 파싱하여 (space ID, pageID)에
         * 따라 해시테이블에 저장해 둔다.
         * -> sdrRedoMgr::scanRedoLogRec()
         * ----------------------------------------------*/
        case SMR_DLT_REDOONLY:
        case SMR_DLT_UNDOABLE:
        case SMR_DLT_NTA:
        case SMR_DLT_REF_NTA:
        case SMR_DLT_COMPENSATION:
            smrRecoveryMgr::getDRDBRedoBuffer(
                sLogType,
                smrLogHeadI::getSize( aLogHead ),
                aLogPtr,
                &sRedoSize,
                &sRedoBuffer );

            if ( sRedoSize > 0 )
            {
                IDE_TEST( smLayerCallback::generateRedoLogDataList(
                                                  smrLogHeadI::getTransID( aLogHead ),
                                                  sRedoBuffer,
                                                  sRedoSize,
                                                  &sBeginLSN,
                                                  &sEndLSN,
                                                  &sDRDBRedoLogDataList )
                          != IDE_SUCCESS );
                IDE_TEST( smLayerCallback::addRedoLogToHashTable( sDRDBRedoLogDataList ) 
                          != IDE_SUCCESS );
            }
            break;
        case SMR_LT_TBS_UPDATE:

            idlOS::memcpy(&sTBSUptLog, aLogPtr, SMR_LOGREC_SIZE(smrTBSUptLog));

            // Tablespace 노드 상태 변경 로그는 SKIP하지 않는다.
            // - Drop된 Tablespace의 경우
            //   - Redo Function안에서 SKIP한다.
            // - Offline된 Tablespace의 경우
            //   - Online이나 Dropped로 전이할 수 있으므로 SKIP해서는 안된다.
            // - Discard된 Tablespace의 경우
            //   - Dropped로 전이할 수 있으므로 TBS DROP 로그만 redo한다.
            sRedoBuffer = aLogPtr 
                          + SMR_LOGREC_SIZE(smrTBSUptLog)
                          + sTBSUptLog.mBImgSize;

            sTrans = smLayerCallback::getTransByTID(
                                     smrLogHeadI::getTransID( &sTBSUptLog.mHead ) );

            /* BUG-41689 A discarded tablespace is redone in recovery
             * Discard된 TBS인지 확인한다. */
            sIsDiscarded = sctTableSpaceMgr::hasState( sTBSUptLog.mSpaceID,
                                                       SCT_SS_SKIP_AGING_DISK_TBS,
                                                       ID_FALSE );

            if ( ( sIsDiscarded == ID_FALSE ) ||
                 ( sTBSUptLog.mTBSUptType == SCT_UPDATE_MRDB_DROP_TBS ) ||
                 ( sTBSUptLog.mTBSUptType == SCT_UPDATE_DRDB_DROP_TBS ) ||
                 ( sTBSUptLog.mTBSUptType == SCT_UPDATE_DRDB_DROP_DBF ) )
            {
                /*
                   PRJ-1548 User Memory Tablespace
                   테이블스페이스 UPDATE 로그에 대한 재수행을 수행한다.
                   미디어 복구 중에는 EXTEND_DBF에 대한 redo가 존재하므로
                   Pending Operation이 등록될 수 있다
                   */
                IDE_TEST( gSmrTBSUptRedoFunction[ sTBSUptLog.mTBSUptType ]( NULL,  /* idvSQL* */
                                                                            sTrans,
                                                                            sCurLSN,  // redo 로그의 LSN
                                                                            sTBSUptLog.mSpaceID,
                                                                            sTBSUptLog.mFileID,
                                                                            sTBSUptLog.mAImgSize,
                                                                            sRedoBuffer,
                                                                            mRestart )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do 
                 * discard된 TBS의 redo는 drop tbs를 제외하고 무시한다.*/
            }
            break;

        case SMR_LT_UPDATE:

            idlOS::memcpy(&sUpdateLog, aLogPtr, SMR_LOGREC_SIZE(smrUpdateLog));

            // Redo SKIP할 Tablespace에 관한 로그인지 체크
            if ( isSkipRedo( SC_MAKE_SPACE(sUpdateLog.mGRID) ) == ID_TRUE )
            {
                break;
            }

            sAfterImage = aLogPtr +
                SMR_LOGREC_SIZE(smrUpdateLog) +
                sUpdateLog.mBImgSize;

            if ( isMediaRecoveryPhase() == ID_TRUE )
            {
                // PRJ-1548 User Memory Tablespace
                // Memory 로그중 유일한 Logical Redo Log에 대해서는
                // 미디어복구시에는 특별하게 고려하여야 한다.
                if ( sUpdateLog.mType ==
                     SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK )
                {
                    IDE_TEST( 
                        smmUpdate::redo4MR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK(
                            SC_MAKE_SPACE(sUpdateLog.mGRID),
                            SC_MAKE_PID(sUpdateLog.mGRID),
                            SC_MAKE_OFFSET(sUpdateLog.mGRID),
                            sAfterImage ) != IDE_SUCCESS );
                    break;
                }
                else
                {
                    // update type에 맞게 redo 로그를 호출한다.
                }
            }
            /* MediaRecovery시에만 반영하는 Log인지는 해당 Update함수에서
             * 직접 판단하여 골라냄 */

            if (gSmrRedoFunction[sUpdateLog.mType] != NULL)
            {
                IDE_TEST( gSmrRedoFunction[sUpdateLog.mType](
                             smrLogHeadI::getTransID(&sUpdateLog.mHead),
                             SC_MAKE_SPACE(sUpdateLog.mGRID),
                             SC_MAKE_PID(sUpdateLog.mGRID),
                             SC_MAKE_OFFSET(sUpdateLog.mGRID),
                             sUpdateLog.mData,
                             sAfterImage,
                             sUpdateLog.mAImgSize,
                             smrLogHeadI::getFlag(&sUpdateLog.mHead) )
                         != IDE_SUCCESS );
            }
            break;

        case SMR_LT_DIRTY_PAGE:
            if ( aAfterChkpt == ID_TRUE )
            {
                idlOS::memcpy((SChar*)sLogBuffer,
                              aLogPtr,
                              smrLogHeadI::getSize(aLogHead));

                sDirtyPage = (smrLogHeadI::getSize(aLogHead) -
                              ID_SIZEOF(smrLogHead) - ID_SIZEOF(smrLogTail))
                    / ID_SIZEOF(scGRID);
                sArrPageGRID = (scGRID*)((SChar*)sLogBuffer +
                                         ID_SIZEOF(smrLogHead));

                for(i = 0; i < sDirtyPage; i++)
                {
                    sSpaceID = SC_MAKE_SPACE(sArrPageGRID[i]);


                    // Redo SKIP할 Tablespace에 관한 로그인지 체크
                    /*
                     * BUG-31423 [sm] the server shutdown abnormaly when
                     *           referencing unavalible tablespaces on redoing.
                     *
                     * TBS node의 state값은 처음에 restore가 불가능하더라도
                     * SMM_UPDATE_MRDB_DROP_TBS를 redo중에 만나면  일시적으로
                     * restore된 것으로 판달될 가능성이 있기 때문에 신뢰할 수
                     * 없다. 그러므로 TBS attr의 state를 갖고서 판단한다.
                     */
                    if ( isSkipRedo( sSpaceID,
                                     ID_TRUE ) == ID_TRUE ) //try TBS attribute
                    {
                        continue;
                    }

                    sPageID = SC_MAKE_PID(sArrPageGRID[i]);

                    if ( isMediaRecoveryPhase() == ID_TRUE )
                    {
                        // 미디어 복구가 필요한 데이타파일의
                        // DIRTY Page만 등록하기 위해 확인한다.
                        IDE_TEST( smmTBSMediaRecovery::findMatchFailureDBF(
                                      sSpaceID,
                                      sPageID,
                                      &sIsExistTBS,
                                      &sIsFailureDBF )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Restart Recovery시에는 무조건
                        // DIRTY page로 등록한다.
                        sIsExistTBS   = ID_TRUE;
                        sIsFailureDBF = ID_TRUE;
                    }

                    if ( (sIsExistTBS == ID_TRUE) &&
                         (sIsFailureDBF == ID_TRUE) )
                    {
                        // SpaceID에 해당하는 Tablespace가 Restore되지 않은 경우
                        // SMM_PID_PTR이 해당 Page만 일부 Restore 실시
                        IDE_TEST( smmManager::getPersPagePtr( sSpaceID,
                                                              sPageID,
                                                              (void**)&sDummyPtr )
                                  != IDE_SUCCESS );

                        sPCH = smmManager::getPCH(sSpaceID, sPageID);
                        smrDPListMgr::add(sSpaceID, sPCH, sPageID);
                    }
                }
            }
            break;

        case SMR_LT_NTA:
            idlOS::memcpy(&sNTALog, aLogPtr, SMR_LOGREC_SIZE(smrNTALog));

            // Redo SKIP할 Tablespace에 관한 로그인지 체크
            if ( isSkipRedo( sNTALog.mSpaceID ) == ID_TRUE )
            {
                break;
            }

            if( aCurTrans != NULL )
            {
                if ( smLayerCallback::IsBeginTrans( aCurTrans ) == ID_TRUE )
                {
                    // 미디어 복구시에는 Trans의 상태가 Begin일 수 없다.
                    // Restart Recovery시에만 OID를 등록한다.
                    IDE_ASSERT( isMediaRecoveryPhase() == ID_FALSE );

                }
            }

            sIsDeleteBit = ID_FALSE;

            switch(sNTALog.mOPType)
            {
                case SMR_OP_SMC_TABLEHEADER_ALLOC:
                    /* BUG-19151: Disk Table 생성도중 사망시 Server Startup이
                     *           안됩니다.
                     *
                     * Create Table을 위해서 Catalog Table에서 Slot을 Alloc시에
                     * 이 로그가 기록된다. SlotHeader의 SCN에는 Delete Bit가 없지만
                     * Redo시에 Delete Bit를 Settting하고 smcTable::createTable이
                     * 완료될때 SMR_OP_CREATE_TABLE이라는 NTA로그가 찍히는데 이
                     * 로그의 Redo시에 Delete Bit를 Clear시킨다.
                     *
                     * 이유: smcTable::createTable이 완료되지 못하고 죽는경우
                     * refineDRDBTables시에 smcTableHeader를 참조하는 그 정보들은
                     * 쓰레기값이됩니다. 하여 smcTable::createTable이 SMR_OP_CREATE_TABLE
                     * 이라는 NTA로그를 찍기전에 죽으면 SlotHeader의 SCN이 DeleteBit가
                     * 설정되고 이후 refineDBDBTables시 DeleteBit가 설정되어 있으면
                     * 이 Table은 Skip하도록 하기 위해서 입니다.
                     */
                    sIsDeleteBit = ID_TRUE;
                case SMR_OP_SMC_FIXED_SLOT_ALLOC:

                    IDE_TEST( smLayerCallback::redo_SMP_NTA_ALLOC_FIXED_ROW(
                                                            sNTALog.mSpaceID,
                                                            SM_MAKE_PID(sNTALog.mData1),
                                                            SM_MAKE_OFFSET(sNTALog.mData1),
                                                            sIsDeleteBit )
                              != IDE_SUCCESS );
                     break;
                case SMR_OP_CREATE_TABLE:
                     sTableOID = (smOID)(sNTALog.mData1);
                     IDE_ASSERT( smmManager::getOIDPtr( sNTALog.mSpaceID,
                                                        sTableOID,
                                                        (void**)&sDummyPtr )
                                 == IDE_SUCCESS );
                    IDE_TEST( smLayerCallback::setDeleteBitOnHeader(
                                                              sNTALog.mSpaceID,
                                                              sDummyPtr,
                                                              ID_FALSE /* Clear Delete Bit */)
                              != IDE_SUCCESS );

                    break;
                default :
                    break;
            }
            break;

        case SMR_LT_COMPENSATION:

            idlOS::memcpy(&sCMPSLog, aLogPtr, SMR_LOGREC_SIZE(smrCMPSLog));

            sBeforeImage = aLogPtr + SMR_LOGREC_SIZE(smrCMPSLog);

            if ( (sctUpdateType)sCMPSLog.mTBSUptType != SCT_UPDATE_MAXMAX_TYPE )
            {
                // 미디어 복구시에는 TBS CLR 로그가 들어오지 않는다.
                IDE_DASSERT( isMediaRecoveryPhase() == ID_FALSE );

                // Tablespace 노드 상태 변경 로그는 SKIP하지 않는다.
                // 단, 실제 Undo수행중에 Undo가 완료되었는지 체크하고 SKIP
                //
                // Undo루틴이 CLR Redo에도 쓰이기 때문에,
                // 여러번 재차 수행되어도 결과가 같음을 보장해야 한다.
                //
                // - Create Tablespace의 Undo
                //   - 이미 Undo가 완료되어 DROPPED상태일 경우 SKIP
                //
                // - Alter Tablespace 의 Undo
                //   - 이미 Alter이전의 상태로 Undo된 경우 Skip
                //
                // - Drop Tablespace의 Undo
                //   - 이미 Drop Tablespace의 Undo가 완료되었는지 체크후
                //     SKIP
                IDE_TEST( gSmrTBSUptUndoFunction[ sCMPSLog.mTBSUptType ](
                              NULL, /* idvSQL* */
                              aCurTrans,
                              sCurLSN,
                              SC_MAKE_SPACE(sCMPSLog.mGRID),
                              sCMPSLog.mFileID,
                              sCMPSLog.mBImgSize,
                              sBeforeImage,
                              mRestart ) != IDE_SUCCESS );
            }
            else
            {
                // Redo SKIP할 Tablespace에 관한 로그인지 체크
                if ( isSkipRedo( SC_MAKE_SPACE(sCMPSLog.mGRID) ) == ID_TRUE )
                {
                    break;
                }

                if ( gSmrUndoFunction[sCMPSLog.mUpdateType] != NULL)
                {
                    IDE_TEST( gSmrUndoFunction[sCMPSLog.mUpdateType](
                                  smrLogHeadI::getTransID(&sCMPSLog.mHead),
                                  SC_MAKE_SPACE(sCMPSLog.mGRID),
                                  SC_MAKE_PID(sCMPSLog.mGRID),
                                  SC_MAKE_OFFSET(sCMPSLog.mGRID),
                                  sCMPSLog.mData,
                                  sBeforeImage,
                                  sCMPSLog.mBImgSize,
                                  smrLogHeadI::getFlag(&sCMPSLog.mHead) )
                              != IDE_SUCCESS );
                }
            }
            break;
        case SMR_LT_XA_PREPARE:

            if ( smLayerCallback::IsBeginTrans( aCurTrans ) == ID_TRUE )
            {
                // 미디어 복구시에는 Trans의 상태가 Begin일 수 없다.
                IDE_ASSERT( isMediaRecoveryPhase() == ID_FALSE );
                idlOS::memcpy( &sXAPrepareLog,
                               aLogPtr,
                               SMR_LOGREC_SIZE(smrXaPrepareLog) );

                /* - 트랜잭션 엔트리에 xa 관련 정보 등록
                   - Update Transaction수를 1증가
                   - add trans to Prepared List */
                IDE_TEST( smLayerCallback::setXAInfoAnAddPrepareLst(
                                              aCurTrans,
                                              sXAPrepareLog.mIsGCTx,
                                              sXAPrepareLog.mPreparedTime,
                                              sXAPrepareLog.mXaTransID,
                                              &sXAPrepareLog.mFstDskViewSCN )
                          != IDE_SUCCESS );
            }
            break;

        case SMR_LT_XA_SEGS:

            if ( smLayerCallback::IsBeginTrans( aCurTrans ) == ID_TRUE )
            {
                IDE_ASSERT( isMediaRecoveryPhase() == ID_FALSE );
                idlOS::memcpy( &sXaSegsLog,
                               aLogPtr,
                               SMR_LOGREC_SIZE(smrXaSegsLog) );

                smxTrans::setXaSegsInfo( aCurTrans,
                                         sXaSegsLog.mTxSegEntryIdx,
                                         sXaSegsLog.mExtRID4TSS,
                                         sXaSegsLog.mFstPIDOfLstExt4TSS,
                                         sXaSegsLog.mFstExtRID4UDS,
                                         sXaSegsLog.mLstExtRID4UDS,
                                         sXaSegsLog.mFstPIDOfLstExt4UDS,
                                         sXaSegsLog.mFstUndoPID,
                                         sXaSegsLog.mLstUndoPID );
            }
            break;

        case SMR_LT_MEMTRANS_COMMIT:
        case SMR_LT_DSKTRANS_COMMIT:

            idlOS::memcpy( &sCommitLog,
                           aLogPtr,
                           SMR_LOGREC_SIZE(smrTransCommitLog) );

            sMemRedoSize = smrLogHeadI::getSize(aLogHead) -
                           SMR_LOGREC_SIZE(smrTransCommitLog) -
                           sCommitLog.mDskRedoSize -
                           ID_SIZEOF(smrLogTail);

            // Commit Log의 Body에는 Disk Table의 Record Count가 로깅되어있다.
            // Body Size가 정확히 ( TableOID, Record Count ) 의 배수인지 체크.
            IDE_ASSERT( sMemRedoSize %
                        (ID_SIZEOF(scPageID) +
                         ID_SIZEOF(scOffset) +
                         ID_SIZEOF(ULong)) == 0 );

            if ((sMemRedoSize > 0) && (aCurTrans != NULL))
            {
                sAfterImage = aLogPtr + SMR_LOGREC_SIZE(smrTransCommitLog);

                IDE_TEST( smLayerCallback::redoAllTableInfoToDB( sAfterImage,
                                                                 sMemRedoSize,
                                                                 isMediaRecoveryPhase() )
                          != IDE_SUCCESS );
            }

            if ( sCommitLog.mDskRedoSize > 0 )
            {
                sRedoBuffer = aLogPtr +
                              SMR_LOGREC_SIZE(smrTransCommitLog) +
                              sMemRedoSize;
                IDE_TEST( smLayerCallback::generateRedoLogDataList(
                                              smrLogHeadI::getTransID( &sCommitLog.mHead ),
                                              sRedoBuffer,
                                              sCommitLog.mDskRedoSize,
                                              &sBeginLSN,
                                              &sEndLSN,
                                              &sDRDBRedoLogDataList )
                          != IDE_SUCCESS );
                IDE_TEST( smLayerCallback::addRedoLogToHashTable( sDRDBRedoLogDataList ) 
                          != IDE_SUCCESS );
            }

            if ( aCurTrans != NULL )
            {
                IDE_TEST( smLayerCallback::makeTransEnd( aCurTrans,
                                                         ID_TRUE ) /* COMMIT */
                          != IDE_SUCCESS );
            }
            else
            {
                /* Active Transaction이 아니지만, Disk Redo 범위에
                 * 해당하는 Commit 로그인경우 makeTransEnd할필요없다 */
                IDE_ASSERT( sLogType == SMR_LT_DSKTRANS_COMMIT );
            }

            break;

        case SMR_LT_MEMTRANS_GROUPCOMMIT:
            /* BUG-48543 Recovery Failure in HPUX */
            idlOS::memcpy( &sGCCnt,
                           aLogPtr + SMR_LOG_GROUP_COMMIT_GROUPCNT_OFFSET,
                           ID_SIZEOF( sGCCnt ) );   //BUG-48576: mGroupCnt -> sGCCnt 
                           
            sRedoBuffer = aLogPtr + SMR_LOGREC_SIZE( smrTransGroupCommitLog );

            for ( i = 0 ; i < sGCCnt ; i++ )
            {
                sGCTrans = NULL;

                idlOS::memcpy( &sTID,
                               sRedoBuffer,
                               ID_SIZEOF(smTID) );

                sRedoBuffer += ( ID_SIZEOF(UInt) * 2 );

                sGCTrans = smLayerCallback::getTransByTID( sTID );

                if ( sGCTrans != NULL )
                {
                    IDE_TEST( smLayerCallback::makeTransEnd( sGCTrans,
                                                             ID_TRUE ) /* COMMIT */
                    != IDE_SUCCESS );
                }
            }

            break;

        case SMR_LT_MEMTRANS_ABORT:
        case SMR_LT_DSKTRANS_ABORT:

            idlOS::memcpy( &sAbortLog,
                           aLogPtr,
                           SMR_LOGREC_SIZE(smrTransAbortLog) );

            /* ------------------------------------------------
             * FOR A4 : DRDB 관련 DML(select 제외)인 경우에 대해
             * commit로그 기록후에 과정을 제대로 수행하지 못했던
             * 트랜잭션의 경우 마무리를 해주어야 한다.
             * - 사용하던 tss의 commit SCN을 설정되지 않았다면,
             * tss의 commit list 에 추가해서 제대로 disk G.C가
             * garbage collecting을 할 수 있도록 하여야 한다.
             * ----------------------------------------------*/
            // destroy OID, tx end , remvoe Active Transaction list
            if ( sAbortLog.mDskRedoSize > 0 )
            {
                sRedoBuffer = aLogPtr +
                              SMR_LOGREC_SIZE(smrTransAbortLog);

                IDE_TEST( smLayerCallback::generateRedoLogDataList(
                                          smrLogHeadI::getTransID(&sAbortLog.mHead),
                                          sRedoBuffer,
                                          sAbortLog.mDskRedoSize,
                                          &sBeginLSN,
                                          &sEndLSN,
                                          &sDRDBRedoLogDataList )
                          != IDE_SUCCESS );
                IDE_TEST( smLayerCallback::addRedoLogToHashTable(
                                                           sDRDBRedoLogDataList ) 
                          != IDE_SUCCESS );
            }

            if ( aCurTrans != NULL )
            {
                IDE_TEST( smLayerCallback::makeTransEnd( aCurTrans,
                                                         ID_FALSE ) /* ROLLBACK */ 
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_ASSERT( sLogType == SMR_LT_DSKTRANS_ABORT );
            }

            break;

        case SMR_LT_FILE_BEGIN:
            /* SMR_LT_FILE_BEGIN 로그의 용도에 대해서는 smrDef.h를 참고
             * 이 로그에 대한 체크는 smrLogFile::open에서 수행하기 때문에
             * redo중에는 아무것도 처리하지 않아도 된다.
             * do nothing */
            break;

        case SMR_LT_FILE_END:
            /* To Fix PR-13786 */
            IDE_TEST( redo_FILE_END( &sEndLSN,
                                     aFileCount )
                      != IDE_SUCCESS );
            break;
        default:
            break;
    }

    // 다음 Check할 Log LSN을 설정한다.
    SM_GET_LSN(*aCurLSN, sEndLSN);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 *
 *    To Fix PR-13786 복잡도 개선
 *    smrRecoveryMgr::redo() 함수에서
 *    SMR_LT_FILE_END 로그 처리 부분을 따로 분리해냄.
 *
 * Implementation :
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::redo_FILE_END( smLSN     * aCurLSN,
                                      UInt      * aFileCount )
{
    IDE_DASSERT( aCurLSN    != NULL );
    IDE_DASSERT( aFileCount != NULL );

    SChar       sLogFilename[ SM_MAX_FILE_NAME ];
    iduFile     sFile;
    UInt        sFileCount;
    ULong       sLogFileSize = 0;

    //------------------------------------------
    // Initialize local variables
    //------------------------------------------
    sFileCount = *aFileCount;

    /* ------------------------------------------------
     * for archive log backup
     * ----------------------------------------------*/
    if ( (getArchiveMode() == SMI_LOG_ARCHIVE) &&
         (smLayerCallback::getRecvType() == SMI_RECOVER_RESTART) )
    {
        IDE_TEST( smrLogMgr::getArchiveThread().addArchLogFile( aCurLSN->mFileNo )
                  != IDE_SUCCESS );
    }

    aCurLSN->mFileNo++;
    aCurLSN->mOffset = 0;

    if (getArchiveMode() == SMI_LOG_ARCHIVE &&
        (getLstDeleteLogFileNo() > aCurLSN->mFileNo))
    {
        idlOS::snprintf(sLogFilename,
                        SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT,
                        smrLogMgr::getArchivePath(),
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME,
                        aCurLSN->mFileNo);
    }
    else
    {
        idlOS::snprintf(sLogFilename,
                        SM_MAX_FILE_NAME,
                        "%s%c%s%"ID_UINT32_FMT,
                        smrLogMgr::getLogPath(),
                        IDL_FILE_SEPARATOR,
                        SMR_LOG_FILE_NAME,
                        aCurLSN->mFileNo);
    }

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST( sFile.setFileName(sLogFilename) != IDE_SUCCESS );

    if (idf::access(sLogFilename, F_OK) != 0)
    {
        /* 다음 LogFile이 없으므로 더이상 Redo할 로그가 없다. */
        smrRedoLSNMgr::setRedoLSNToBeInvalid();
    }
    else
    {
        /* Fix PR-2730 */
        if ( smLayerCallback::getRecvType() == SMI_RECOVER_RESTART )
        {
            IDE_TEST( sFile.open() != IDE_SUCCESS );
            IDE_TEST( sFile.getFileSize(&sLogFileSize) != IDE_SUCCESS );
            IDE_TEST( sFile.close() != IDE_SUCCESS );

            if ( sLogFileSize != smuProperty::getLogFileSize() )
            {
                (void)idf::unlink(sLogFilename);
                /* 다음 LogFile이 없으므로 더이상 Redo할 로그가 없다. */
                smrRedoLSNMgr::setRedoLSNToBeInvalid();
            }
        }
    }

    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    (sFileCount)++;
    (((sFileCount) % 5) == 0 ) ? IDE_CALLBACK_SEND_SYM("*") : IDE_CALLBACK_SEND_SYM(".");

    /*
     * BUG-26350 [SD] startup시에 redo의 진행상황을 로그파일명으로 sm log에 출력
     */
    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV, "%s", sLogFilename );

    *aFileCount = sFileCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Active 트랜잭션의 디스크 로그중에 복구를 위한 유지하는 메모리자료구조에
 *               반영할 것을 즉시 반영한다.
 *
 * aLogType [IN]  - 로그타입
 * aLogPtr  [IN]  - SMR_DLT_REF_NTA 로그의 RefOffset + ID_SIZEOF(sdrLogHdr)
 *                  해당하는 LogPtr
 * aContext [OUT] - RollbackContext
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::applyDskLogInstantly4ActiveTrans( void        * aCurTrans,
                                                         SChar       * aLogPtr,
                                                         smrLogHead  * aLogHeadPtr,
                                                         smLSN       * aCurRedoLSNPtr )
{
    scSpaceID           sSpaceID;
    smOID               sTableOID;
    smOID               sIndexOID;
    smrDiskLog          sDiskLogRec;
    smrDiskRefNTALog    sDiskRefNTALogRec;
    SChar            *  sLogBuffer;

    IDE_ASSERT( aCurTrans      != NULL );
    IDE_ASSERT( aLogPtr        != NULL );
    IDE_ASSERT( aLogHeadPtr    != NULL );
    IDE_ASSERT( aCurRedoLSNPtr != NULL );

    IDE_TEST_CONT( smLayerCallback::isTxBeginStatus( aCurTrans )
                   == ID_FALSE, cont_finish );

    switch( smrLogHeadI::getType(aLogHeadPtr) )
    {
        case SMR_DLT_REDOONLY:

            idlOS::memcpy( &sDiskLogRec,
                           aLogPtr,
                           SMR_LOGREC_SIZE(smrDiskLog) );

            // BUG-7983 dml 처리시 undo segment commit후 예외, Shutdown에
            // 대한 undo 처리가 필요합니다.
            if ( sDiskLogRec.mRedoType == SMR_RT_WITHMEM )
            {
                sLogBuffer = aLogPtr +
                             SMR_LOGREC_SIZE(smrDiskLog) +
                             sDiskLogRec.mRefOffset;

                smLayerCallback::redoRuntimeMRDB( aCurTrans,
                                                  sLogBuffer );
            }
            break;

        case SMR_DLT_REF_NTA:

            if ( isVerifyIndexIntegrityLevel2() == ID_TRUE)
            {
                idlOS::memcpy( &sDiskRefNTALogRec,
                               aLogPtr,
                               SMR_LOGREC_SIZE(smrDiskRefNTALog) );

                sLogBuffer = aLogPtr +
                             SMR_LOGREC_SIZE(smrDiskRefNTALog) +
                             sDiskRefNTALogRec.mRefOffset;

                IDE_TEST( smLayerCallback::getIndexInfoToVerify( sLogBuffer,
                                                                 &sTableOID,
                                                                 &sIndexOID,
                                                                 &sSpaceID )
                          != IDE_SUCCESS );

                if ( sIndexOID != SM_NULL_OID )
                {
                    IDE_TEST( smLayerCallback::addOIDToVerify( aCurTrans,
                                                               sTableOID,
                                                               sIndexOID,
                                                               sSpaceID )
                              != IDE_SUCCESS );
                }
            }
            break;

        default:
            break;
    }

    IDE_EXCEPTION_CONT( cont_finish );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrRecoveryMgr::addActiveTrans( smrLogHead    * aLogHeadPtr,
                                       SChar         * aLogPtr,
                                       smTID           aTID,
                                       smLSN         * aCurRedoLSNPtr,
                                       void         ** aCurTrans)
{
    UInt    sTransFlag;
    void  * sCurTrans;

    sCurTrans     = smLayerCallback::getTransByTID( aTID );
    IDE_DASSERT( sCurTrans != NULL );

    switch( smrLogHeadI::getFlag(aLogHeadPtr) & SMR_LOG_TYPE_MASK )
    {
    case SMR_LOG_TYPE_REPLICATED:
        sTransFlag = SMI_TRANSACTION_REPL_NONE;
        break;
    case SMR_LOG_TYPE_REPL_RECOVERY:
        sTransFlag = SMI_TRANSACTION_REPL_RECOVERY;
        break;
    case SMR_LOG_TYPE_NORMAL:
        sTransFlag = SMI_TRANSACTION_REPL_DEFAULT;
        break;
    default:
        IDE_ERROR( 0 );
        break;
    }
    sTransFlag |= SMI_COMMIT_WRITE_NOWAIT;

    /* add active transaction list if tx has beging log. */
    smLayerCallback::addActiveTrans( sCurTrans,
                                     aTID,
                                     sTransFlag,
                                     isBeginLog( aLogHeadPtr ),
                                     aCurRedoLSNPtr );

    IDE_TEST( applyDskLogInstantly4ActiveTrans( sCurTrans,
                                                aLogPtr,
                                                aLogHeadPtr,
                                                aCurRedoLSNPtr ) 
              != IDE_SUCCESS );

    (*aCurTrans) = sCurTrans;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 복구과정에서의 트랜잭션 재수행 (redoall pass)
 *
 * - MRDB와 DRDB의 checkpoint의 통합으로
 *   redo 시작 LSN에 대하여 다음과 같은 경우가 발생
 *   위의 상황에 따라 각기 다르게 처리해야한다.
 *   + CASE 0 - active trans의 recvLSN = DRDB의 RecvLSN
 *     : 지금과 동일하게 처리
 *   + CASE 1 - active trans의 recvLSN > DRDB의 RecvLSN
 *     : DRDB의 RecvLSN부터 active trans의 RecvLSN을 만날때까지
 *       drdb redo 로그만 적용한다.
 *       active trans의 RecvLSN부터는 지금과 동일하게 처리
 *   + CASE 2 - active trans의 recvLSN < DRDB의 RecvLSN
 *     : active trans의 RecvLSN부터 DRDB의 RecvLSN을 만날때까지
 *       drdb redo 로그는 적용하지 않고,
 *       그 다음 지금과 동일하게 처리한다.
 *
 * - Redo 로그 적용 방식
 *   + MRDB의 redo로그를 판독하면 바로 redo 함수를 호출하여
 *     바로 적용한다.
 *   + DRDB의 redo로그를 판독하면 해시테이블에 space ID, page ID
 *     에 따라 redo 로그를 저장한 후, 모든 redo 로그판독이 완료된
 *     후에 해시에 저장된 DRDB의 redo로그를 disk에 적용한다.
 *
 * - 2nd. code design
 *   + 로그앵커로부터 begin Checkpoint LSN을 얻음
 *   + begin checkpoint 로그를 판독하여  memory RecvLSN과
 *     disk RecvLSN을 얻음
 *   + 두 RecvLSN 중에서 작은것으로 RecvLSN을 결정하여, RecvLSN부터
 *     마지막 로그까지 판독하면서 재수행을 처리한다.
 *     : 재수행 처리시에 위에서도 언급했듯이 이미 disk에 반영된
 *       로그에 대해서는 skip 하도록 한다.
 *   + 다음으로 판독할 로그 결정
 *   + !!DRDB의 redo 관리자 초기화
 *   + 각 로그 타입에 따라 redo 수행
 *   + loop - !! 로그 재수행
 *     : !!disk 로그를 판독한 경우 body에 저장되어 있는
 *       disk redo로그들을 파싱하여 (space ID, pageID)에
 *       따라 해시테이블에 저장해 둔다.
 *        -> sdrRedoMgr::scanRedoLogRec()
 *     : mrdb 로그는 기존과 동일하게 처리한다.
 *
 *   + !!해시 테이블에 저장해 둔, DRDB의 redo 로그들을
 *       disk tablespace에 반영
 *       -> sdrRedoMgr::applyHashedLogRec()
 *   + !!DRDB의 redo 관리자 해제
 *   + 모든 로그파일을 close 한다.
 *   + end of log를 이용하여 Log File Manager Thread 실행 및 초기화
 **********************************************************************/
IDE_RC smrRecoveryMgr::redoAll( idvSQL* aStatistics )
{
    UInt                  sState = 0;
    SChar*                sLogPtr;
    void*                 sCurTrans = NULL;
    smTID                 sTID;
    smrLogHead*           sLogHeadPtr;
    smLSN                 sChkptBeginLSN;
    smLSN                 sChkptEndLSN;
    smLSN*                sCurRedoLSNPtr = NULL;
    smLSN                 sDummyRedoLSN4Check;
    smLSN                 sLstRedoLSN;
    smrBeginChkptLog      sBeginChkptLog;
    idBool                sIsValid    = ID_FALSE;
    idBool                sAfterChkpt = ID_FALSE;
    smLSN                 sMemRecvLSNOfDisk;
    smLSN                 sDiskRedoLSN;
    smLSN                 sOldestLSN;
    smLSN                 sMustRedoToLSN;
    UInt                  sCapacityOfBufferMgr;
    smrLogFile*           sLogFile   = NULL;
    UInt                  sFileCount = 1;
    smLSN                 sLstLSN;
    UInt                  sCrtLogFileNo;
    UInt                  sLogSizeAtDisk = 0;
    SChar*                sDBFileName    = NULL;
    smrLogType            sLogType;
    smrRTOI               sRTOI;
    idBool                sRedoSkip          = ID_FALSE;
    idBool                sRedoFail          = ID_FALSE;
    idBool                sObjectConsistency = ID_TRUE;
    smLSN                 sMemBeginLSN; 
    smLSN                 sDiskBeginLSN;

    SM_LSN_MAX( sMemBeginLSN );
    SM_LSN_MAX( sDiskBeginLSN );

    if ( getArchiveMode() == SMI_LOG_ARCHIVE )
    {
        IDE_TEST( smrLogMgr::getArchiveThread().lockThreadMtx()
                  != IDE_SUCCESS );
        sState = 1;
    }

    /* ------------------------------------------------
     * 0. redo 관리자를 초기화한다.
     * ----------------------------------------------*/
    sCapacityOfBufferMgr = sdbBufferMgr::getPageCount();

    IDE_TEST( smLayerCallback::initializeRedoMgr( sCapacityOfBufferMgr,
                                                  SMI_RECOVER_RESTART )
              != IDE_SUCCESS );

    /* -----------------------------------------------------------------
     * 1. begin checkpoint 로그 판독
     *    Checkpoint log는 무조건 Disk에 기록된다.
     * ---------------------------------------------------------------- */
    sChkptBeginLSN  = mAnchorMgr.getBeginChkptLSN();
    sChkptEndLSN    = mAnchorMgr.getEndChkptLSN();

    IDE_TEST( smrLogMgr::readLog( NULL, /* 압축 버퍼 핸들 => NULL
                                          Checkpoint Log의 경우
                                          압축하지 않는다 */
                                 &sChkptBeginLSN,
                                 ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                 &sLogFile,
                                 (smrLogHead*)&sBeginChkptLog,
                                 &sLogPtr,
                                 &sIsValid,
                                 &sLogSizeAtDisk )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsValid == ID_FALSE, err_invalid_log );

    idlOS::memcpy(&sBeginChkptLog, sLogPtr, SMR_LOGREC_SIZE(smrBeginChkptLog));

    /* Checkpoint를 위해서 open된 LogFile을 Close한다.
     */
    IDE_TEST( smrLogMgr::closeLogFile(sLogFile) != IDE_SUCCESS );
    sLogFile = NULL;

    IDE_ASSERT(smrLogHeadI::getType(&sBeginChkptLog.mHead) == SMR_LT_CHKPT_BEGIN);

    /* ------------------------------------------------
     * 2. RecvLSN 결정
     * DRDB의 oldest LSN은 loganchor에 저장된 것과 begin
     * checkpoint 로그에 저장된 것중 가장 큰것으로 결정한 후,
     * Active 트랜잭션의 minimum LSN과 oldest LSN을 비교해서
     * 작은것으로 RecvLSN을 결정
     * ----------------------------------------------*/
    sMemRecvLSNOfDisk = sBeginChkptLog.mEndLSN;
    sDiskRedoLSN      = sBeginChkptLog.mDiskRedoLSN;
    sOldestLSN        = mAnchorMgr.getDiskRedoLSN();
    if (smrCompareLSN::isGT(&sOldestLSN, &sDiskRedoLSN) == ID_TRUE)
    {
        sDiskRedoLSN = sOldestLSN;
    }

    /*
      Disk에 대해서 RecvLSN을 재결정한다. Disk Checkpoint는 MMDB와
      별도로 수행이 되고 그때 RecvLSN은 LogAnchor에 기록되기때문에 LogAnchor의
      oldestLSN와 비교해서 큰 값을 그 recv lsn으로 결정하고 다시 Disk의 RecvLSN과
      비교 해서 작은 값을 recv lsn을 설정한다.
    */
    if ( smrCompareLSN::isGTE(&sMemRecvLSNOfDisk, &sDiskRedoLSN)
         == ID_TRUE )
    {
        // AT recvLSN >= drdb recvLSN
        sBeginChkptLog.mEndLSN = sDiskRedoLSN;
    }
    else
    {
        /* nothing to do */
    }

    /* PROJ-2569 2pc를 위해 필요한 LSN과 recv lsn중 더 작은 값으로 recv lsn을 설정.
     * mSkipRedoLSN에 기존 LSN값을 저장해 둔다. */
    if ( smrCompareLSN::isGT( &sBeginChkptLog.mEndLSN, &sBeginChkptLog.mDtxMinLSN )
         == ID_TRUE )
    {
        mSkipRedoLSN = sBeginChkptLog.mEndLSN;
        sBeginChkptLog.mEndLSN = sBeginChkptLog.mDtxMinLSN;

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_SKIP_REDO_LSN,
                    mSkipRedoLSN.mFileNo,
                    mSkipRedoLSN.mOffset);
    }
    else
    {
        /* Nothing to do */
    }

    /* Archive 대상 Logfile을  Archive List에 추가한다. */
    if ( getArchiveMode() == SMI_LOG_ARCHIVE )
    {
        IDE_TEST( rebuildArchLogfileList( &sBeginChkptLog.mEndLSN )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    SM_LSN_MAX( mIdxSMOLSN );

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_REDOALL_START,
                sBeginChkptLog.mEndLSN.mFileNo,
                sBeginChkptLog.mEndLSN.mOffset);

    /*
      Redo할때마다 RedoLSN이 가리키는 Log중에서 가장
      작은 mLSN값을 가진 로그를 먼저 Redo한다. 이렇게 함으로써 Transaction이
      기록한 로그순으로 Redo를 수행한다. 이를 위해 smrRedoLSNMgr을 이용한다.
    */
    IDE_TEST( smrRedoLSNMgr::initialize( &sBeginChkptLog.mEndLSN )
              != IDE_SUCCESS );

    /* -----------------------------------------------------------------
     * recovery 시작 로그부터 end of log까지 각 로그에 대해 redo 수행
     * ---------------------------------------------------------------- */

    while ( 1 )
    {
        if ( sRedoFail == ID_TRUE ) /* Redo관련 이상 발견 */
        {
            IDE_TEST( startupFailure( &sRTOI, ID_TRUE ) // isRedo
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }

        if ( sRedoSkip == ID_TRUE ) /* 이전 Loop에서 Redo를 안했으면 */
        {
            sCurRedoLSNPtr->mOffset += sLogSizeAtDisk; /* 수동으로 옮겨줌 */
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( smrRedoLSNMgr::readLog( &sCurRedoLSNPtr,
                                          &sLogHeadPtr,
                                          &sLogPtr,
                                          &sLogSizeAtDisk,
                                          &sIsValid) != IDE_SUCCESS );
        SM_GET_LSN( mLstDRDBRedoLSN , *sCurRedoLSNPtr );

        if ( sIsValid == ID_FALSE )
        {
            ideLog::log( IDE_SM_0, 
                         "Restart Recovery Completed "
                         "[ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]",
                         sCurRedoLSNPtr->mFileNo,
                         sCurRedoLSNPtr->mOffset );

            break; /* Invalid한 로그를 판독한 경우 */
        }
        else
        {
            /* nothing to do */
        }

        /* Debuging 정보 저장용으로 Log 위치 백업해둠 */
        SM_GET_LSN( mLstRedoLSN, *sCurRedoLSNPtr );
        mCurLogPtr     = sLogPtr;
        mCurLogHeadPtr = sLogHeadPtr;

        sRedoSkip    = ID_FALSE;
        sRedoFail    = ID_FALSE;
        SM_GET_LSN( sLstRedoLSN, *sCurRedoLSNPtr );
        sLogType     = smrLogHeadI::getType(sLogHeadPtr);
      
        IDE_TEST( processDtxLog( mCurLogPtr,
                                 sLogType,
                                 sCurRedoLSNPtr,
                                 &sRedoSkip )
                  != IDE_SUCCESS );  

        /* 이번 Checkpoint와 짝이 되는 DirtyPageList Log를 찾기 위해,
         * 현 상태가 Checkpoint 직후인지 확인함 */
        if ( smrCompareLSN::isGTE( sCurRedoLSNPtr, &sChkptBeginLSN )
             == ID_TRUE )
        {
            sAfterChkpt = ID_TRUE;
        }
        else
        {
            /* nothing to do ... */
        }

        /* -------------------------------------------------------------
         * [3] 읽은 로그가 트랜잭션이 기록한 로그인 경우 그 트랜잭션을
         * 트랜잭션 Queue에 등록한다.
         * - active 트랜잭션 등록은 active trans recvLSN과 동일한 LSN부터
         *   처리한다.
         * - drdb 트랜잭션의 경우, 할당되었던 tss가 존재할 경우 assign
         *   해주어야 한다.
         * ------------------------------------------------------------ */
        sCurTrans = NULL;
        sTID = smrLogHeadI::getTransID(sLogHeadPtr);

        /* 일반적인 Tx일때만 ActiveTx를 등록함 */
        /* MemRecvLSN은 OldestTxLSN이기에, 이보다 이전이면 무시함  */
        if ( ( sTID != SM_NULL_TID ) &&
             ( smrCompareLSN::isLT( sCurRedoLSNPtr,
                                    &sMemRecvLSNOfDisk) == ID_FALSE ) &&
             ( sRedoSkip == ID_FALSE ) )
        {
            IDE_TEST( addActiveTrans( sLogHeadPtr,
                                      sLogPtr,
                                      sTID,
                                      sCurRedoLSNPtr,
                                      (&sCurTrans) ) != IDE_SUCCESS );
        }

        /*
         * [4] RecvLSN에 따라 선별적으로 로그를 재수행한다.
         * BUG-25014 redo시 서버 비정상종료합니다. (by BUG-23919)
         * 하지만, 다음 로그레코드는 무조건 redo 한다.
         * SMR_LT_FILE_END
         * : 다음 로그파일을 Open해야하기 때문이다.
         * SMR_LT_DSKTRANS_COMMIT와 SMR_LT_DSKTRANS_ABORT
         * : Active Transaction이 아닌 경우에도 Disk LogSlot을 포함하기
         *   때문에 Redo해야 한다.
         *   반대로 Active Transaction이지만, Disk Redo범위에 포함되지
         *   않는다면, Memory Redo와 MakeTransEnd를 수행해야하므로 Redo
         *   되어야한다. 이때 Diskk LogSlot은 Hashing 되어도 반영은 되지
         *   않는다.
         */
        if ( ( sLogType != SMR_LT_DSKTRANS_COMMIT )      &&
             ( sLogType != SMR_LT_DSKTRANS_ABORT )       &&
             ( sLogType != SMR_LT_FILE_END ) )
        {
            if ( smrLogMgr::isDiskLogType(sLogType) == ID_TRUE )
            {
                /* DiskRedoLSN 보다 작은 LSN을 갖는 Disk 로그는 Skip 한다. */
                if ( smrCompareLSN::isLT( sCurRedoLSNPtr, &sDiskRedoLSN ) == ID_TRUE )
                {
                    IDE_ASSERT( SM_IS_LSN_MAX( sDiskBeginLSN ) );
                    sRedoSkip = ID_TRUE;
                }
                else
                {
                    if ( SM_IS_LSN_MAX( sDiskBeginLSN ) )
                    {
                         SM_GET_LSN( sDiskBeginLSN, sLstRedoLSN );
                    }
                }
            }
            else
            {
                /* MemRecvLSN 보다 작은 LSN을 갖는 Mem 로그는 Skip 한다. */
                if ( smrCompareLSN::isLT( sCurRedoLSNPtr, &sMemRecvLSNOfDisk ) == ID_TRUE )
                {
                    IDE_ASSERT( SM_IS_LSN_MAX( sMemBeginLSN ) );
                    sRedoSkip = ID_TRUE;
                }
                else
                {
                    if ( SM_IS_LSN_MAX( sMemBeginLSN ) )
                    {
                        SM_GET_LSN( sMemBeginLSN, sLstRedoLSN );
                    } 
                    else
                    {
                        /* nothing to do */
                    }
                }
            }
        }
        else
        {
            /* nothing to do ... */
        }

        if ( sRedoSkip == ID_TRUE )
        {
            /* PROJ-2133 Incremental Backup */
            if ( isCTMgrEnabled() == ID_TRUE )
            {
                smLSN   sLastFlushLSN;

                sLastFlushLSN = getCTFileLastFlushLSNFromLogAnchor();
                
                if ( smrCompareLSN::isLT( &sLastFlushLSN, sCurRedoLSNPtr )  == ID_TRUE )
                {
                    IDE_TEST( smriChangeTrackingMgr::removeCTFile() 
                              != IDE_SUCCESS );
                    ideLog::log( IDE_SM_0,
                    "====================================================\n"
                    " change tracking is disabled during restart recovery\n"
                    "====================================================\n" );
                }
            }
            continue;
        }

        /*******************************************************************
         * PROJ-2162 RestartRiskReduction
         *******************************************************************/
        if ( smrCompareLSN::isEQ( sCurRedoLSNPtr, &sChkptEndLSN ) == ID_TRUE )
        {
            SM_GET_LSN( mEndChkptLSN, sChkptEndLSN ); /* EndCheckpointLSN 획득 */ 
        }

        /* Redo 진행해도 되는 객체인지 확인 */
        prepareRTOI( sLogPtr,
                     sLogHeadPtr,
                     sCurRedoLSNPtr,
                     NULL, /* aDRDBRedoLogData */
                     NULL, /* aDRDBPagePtr */
                     ID_TRUE, /* aIsRedo */
                     &sRTOI );
        checkObjectConsistency( &sRTOI,
                                &sObjectConsistency );
        if ( ( sLogType == SMR_LT_FILE_END ) &&
             ( sObjectConsistency == ID_FALSE ) )
        {
            // LOGFILE END Log는 절대 실패할 수 없음.
            IDE_DASSERT( 0 );
            sObjectConsistency = ID_TRUE;
        }

        if ( sObjectConsistency == ID_FALSE )
        {
            sRedoSkip = ID_TRUE;
            sRedoFail = ID_TRUE;
            continue; /* Redo 안함 */
        }

        /* Redo하면서 LSN을 이동시키기 때문에, 검증용으로 Redo한번 더하기 위해
         * LSN을 보존해둠 */
        SM_GET_LSN( sDummyRedoLSN4Check, *sCurRedoLSNPtr );

        /* -------------------------------------------------------------
         * [5] 각 로그 타입에 따라 redo 수행
         * ------------------------------------------------------------ */
        if ( redo( sCurTrans,
                   sCurRedoLSNPtr,
                   &sFileCount,
                   sLogHeadPtr,
                   sLogPtr,
                   sLogSizeAtDisk,
                   sAfterChkpt ) == IDE_SUCCESS )
        {
            sRedoSkip = ID_FALSE;
            sRedoFail = ID_FALSE;

            if ( ( smuProperty::getSmEnableStartupBugDetector() == ID_TRUE ) &&
                 ( isIdempotentLog( sLogType ) == ID_TRUE ) )
            {
                /* Idempotent한 Log이기 때문에, 한번 성공했으면 무조건 성공
                 * 해야 함 */
                IDE_ASSERT( redo( sCurTrans,
                                  &sDummyRedoLSN4Check,
                                  &sFileCount,
                                  sLogHeadPtr,
                                  sLogPtr,
                                  sLogSizeAtDisk,
                                  sAfterChkpt )
                            == IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* Redo 실패함 */
            sRedoSkip    = ID_TRUE;
            sRedoFail    = ID_TRUE;
            sRTOI.mCause = SMR_RTOI_CAUSE_REDO;
        }

        // 해싱된 디스크로그들의 크기의 총합이
        // DISK_REDO_LOG_DECOMPRESS_BUFFER_SIZE 를 벗어나면
        // 해싱된 로그들을 모두 버퍼에 적용한다.
        IDE_TEST( checkRedoDecompLogBufferSize() != IDE_SUCCESS );
    }//While

    ideLog::log( IDE_SERVER_0,
                  "MemBeginLSN : [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n"
                  "DskBeginLSN : [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n"
                  "LastRedoLSN : [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n",
                  sMemBeginLSN.mFileNo,
                  sMemBeginLSN.mOffset,
                  sDiskBeginLSN.mFileNo,
                  sDiskBeginLSN.mOffset,
                  sLstRedoLSN.mFileNo,
                  sLstRedoLSN.mOffset );

    IDE_ASSERT( SM_IS_LSN_MAX( sLstRedoLSN ) == ID_FALSE );
    mCurLogPtr     = NULL;
    mCurLogHeadPtr = NULL;
    SM_GET_LSN( mLstRedoLSN, sLstRedoLSN );

    // PROJ-1867
    // 만약 Online Backup받았던 DBFile로 recovery한 경우이면
    // sLstRedoLSN이 endLSN까지 진행 하였는지를 확인한다.
    // Online Backup 하지 않았다면 sMustRedoToLSN는 init상태이다.

    IDE_TEST( getMaxMustRedoToLSN( aStatistics,
                                   &sMustRedoToLSN,
                                   &sDBFileName )
              != IDE_SUCCESS );

    if ( smrCompareLSN::isGT( &sMustRedoToLSN,
                              &mLstDRDBRedoLSN ) == ID_TRUE )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_DRECOVER_NEED_MORE_LOGFILE,
                    mLstDRDBRedoLSN.mFileNo,
                    mLstDRDBRedoLSN.mOffset,
                    sDBFileName,
                    sMustRedoToLSN.mFileNo,
                    sMustRedoToLSN.mOffset );

        IDE_RAISE( err_need_more_log );
    }

    /* redo가 끝나고나면 마지막으로 redo한 로그의 SN을 Repl Recovery LSN으로 설정한다.
     * repl recovery에서 반복적인 장애시에는 서비스 중 제일 처음에 발생한 장애시에만
     * repl recovery LSN을 설정해야 한다. 그러므로, 이전 상태가 서비스 중(SM_SN_NULL)
     * 인 경우에만 설정한다. 그리고, 다시 서비스 들어가기 전(replication 초기화 종료 후)
     * repl recovery LSN을 SM_SN_NULL로 설정한다. proj-1608
     */
    if ( SM_IS_LSN_MAX( getReplRecoveryLSN() ) )
    {
        IDE_TEST( setReplRecoveryLSN( sLstRedoLSN ) != IDE_SUCCESS );
    }

    /* ------------------------------------------------
     * 해시 테이블에 저장해 둔, DRDB의 redo 로그들을
     * disk tablespace에 반영
     * sdrRedoMgr::applyHashedLogRec() 함수 호출
     * ----------------------------------------------*/
    IDE_TEST( applyHashedDiskLogRec( aStatistics) != IDE_SUCCESS );

    /* redo 관리자를 해제한다. */
    IDE_TEST( smLayerCallback::destroyRedoMgr() != IDE_SUCCESS );

    sLstRedoLSN = smrRedoLSNMgr::getNextLogLSNOfLstRedoLog();

    /* Restart 이후에 버퍼상에 Index Page를 적재할때, RedoLSN 보다
     * 큰 LSN을 가진 모든 Index Page의 SMO No를 0으로 초기화한다.
     * 왜냐하면, SMO No는 Logging 대상이 아닌 운영중에 계속 증가하는
     * Runtime 정보이며, 이것은 SMO 진행중임을 판단하여
     * index traverse의 retry 여부를 판단한다. 이는 모두 index 가
     * no-latch traverse scheme 이기 때문이다.
     * 그러므로 여기서는 retry를 오판하지 않도록 LSN을 임시로 저장한다. */
    mIdxSMOLSN = smrRedoLSNMgr::getLstCheckLogLSN();

    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                 SM_TRC_MRECOVERY_RECOVERYMGR_REDOALL_SUCCESS,
                 sLstRedoLSN.mFileNo,
                 sLstRedoLSN.mOffset);

    /* -----------------------------------------------------------------
     * [8] smrLogMgr의 Prepare, Sync Thread를
     *     시작시킨다.
     * ---------------------------------------------------------------- */
    sLstLSN = smrRedoLSNMgr::getLstCheckLogLSN();
    sCrtLogFileNo = sLstLSN.mFileNo;

    /* Redo시 사용한 모든 LogFile을 Close한다. */
    IDE_TEST( smrLogMgr::getLogFileMgr().closeAllLogFile() != IDE_SUCCESS );

    /* ------------------------------------------------
     * [9] 더이상 Redo할 로그가 없기 때문에 smrRedoLSNMgr을
     *     종료시킨다.
     * ----------------------------------------------*/
    IDE_TEST( smrRedoLSNMgr::destroy() != IDE_SUCCESS );

    // BUG-47404 Log Prepare 전에 WAL 검사를 해야 한다.
    // WAL이 깨어졌으면 Consistency를 설정하여
    // 이후 Log Prepare Thread에서 logfile 초기화를 하지 않는다.
    IDE_TEST( checkMemWAL()  != IDE_SUCCESS );
    IDE_TEST( checkDiskWAL() != IDE_SUCCESS );

    ideLog::log(IDE_SERVER_0,"      [SM-PREPARE] Prepare Logfile Thread Start...");
    IDE_TEST( smrLogMgr::startupLogPrepareThread( &sLstLSN,
                                                  sCrtLogFileNo,
                                                  ID_TRUE /* aIsRecovery */ )
             != IDE_SUCCESS );
    ideLog::log(IDE_SERVER_0,"[SUCCESS]\n");

    IDU_FIT_POINT( "BUG-45283@smrRecoveryMgr::redoAll::unlockThreadMtx" );

    if ( getArchiveMode() == SMI_LOG_ARCHIVE )
    {
        sState = 0;
        IDE_TEST( smrLogMgr::getArchiveThread().unlockThreadMtx()
                  != IDE_SUCCESS );
    }

    IDE_CALLBACK_SEND_MSG("");

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_need_more_log );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ErrNeedMoreLog, sDBFileName ) );
    }
    IDE_EXCEPTION( err_invalid_log );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidLog,
                                  sChkptBeginLSN.mFileNo,
                                  sChkptBeginLSN.mOffset ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_DASSERT( getArchiveMode() == SMI_LOG_ARCHIVE );
        IDE_ASSERT( smrLogMgr::getArchiveThread().unlockThreadMtx()
                    == IDE_SUCCESS );
    }
    if ( sLogFile != NULL )
    {
        (void)smrLogMgr::closeLogFile( sLogFile );
        sLogFile = NULL;
    }

    IDE_POP();

    mCurLogPtr     = NULL;
    mCurLogHeadPtr = NULL;

    return IDE_FAILURE;

}

/*
  Decompress Log Buffer 크기가
  프로퍼티에 지정된 값보다 클 경우 Hashing된 Disk Log들을 적용
*/
IDE_RC smrRecoveryMgr::checkRedoDecompLogBufferSize()
{
    if ( smrRedoLSNMgr::getDecompBufferSize() >
         smuProperty::getDiskRedoLogDecompressBufferSize() )
    {
        IDE_TEST( applyHashedDiskLogRec( NULL ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Hashing된 Disk Log들을 적용
 */
IDE_RC smrRecoveryMgr::applyHashedDiskLogRec( idvSQL* aStatistics)
{
    IDE_TEST( smLayerCallback::applyHashedLogRec( aStatistics )
              != IDE_SUCCESS );

    // Decompress Log Buffer가 할당한 모든 메모리를 해제한다.
    IDE_TEST( smrRedoLSNMgr::clearDecompBuffer() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description : 복구과정에서의 active 트랜잭션 철회 (undoall pass)
 *
 * 트래잭션 재수행 과정에서 구성했던 active 트랜잭션 목록에
 * 있는 모든 트랜잭션들에 대해 로그가 생성된 역순으로 가장
 * 큰 LSN을 가지는 로그를 가진 트랜잭션부터 철회를 한다.
 *
 * - 2nd. code design
 *   + 각 active 트랜잭션이 마지막으로 생성한 로그의
 *     LSN으로 queue 구성한다.
 *   + 각 active 트랜잭션이 생성한 각 로그에 대해
 *     트랜잭션 철회를 진행한다.
 *   + 각 active 트랜잭션에 대해 abort log 생성한다.
 *   + prepared list에 있는 각 트랜잭션에 대해 record lock 획득
 **********************************************************************/
IDE_RC smrRecoveryMgr::undoAll( idvSQL* /*aStatistics*/ )
{
    smrUTransQueue      sTransQueue;
    smrLogFile        * sLogFile;
    void              * sActiveTrans;
    smLSN               sCurUndoLSN;
    smrLogFile        * sCurLogFile;
    smrUndoTransInfo  * sTransInfo;

    sLogFile = NULL;

    /* ------------------------------------------------
     * active list에 있는 모든 트랜잭션들에 대해
     * 로그가 생성된 역순으로 가장 큰 LSN을 가지는
     * 로그부터 undo한다.
     * ----------------------------------------------*/

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_UNDOALL_START);

    if ( smLayerCallback::isEmptyActiveTrans() == ID_FALSE )
    {
        IDE_TEST( sTransQueue.initialize(  smLayerCallback::getActiveTransCnt() )
                  != IDE_SUCCESS );

        /* ------------------------------------------------
         * [1] 각 active 트랜잭션이 마지막으로 생성한 로그의
         * LSN으로 queue 구성한다.
         * ----------------------------------------------*/
        IDE_TEST( smLayerCallback::insertUndoLSNs( &sTransQueue )
                  != IDE_SUCCESS );

        /* ------------------------------------------------
         * [2] 각 active 트랜잭션이 생성한 각 로그에 대해 undo 수행
         * - DRDB 로그 타입이든 MRDB 로그 타입이든 여기서는 상관없으며,
         * undo() 함수내부에서 구분하여 처리한다.
         * ----------------------------------------------*/
        sTransInfo = sTransQueue.remove();

        // BUG-27574 klocwork SM
        IDE_ASSERT( sTransInfo != NULL );

        /* For Parallel Logging: Active Transaction의 undoNxtLSN중에서
           가장 큰 LSN값을 가진 Transaction의 undoNextLSN이 가리키는
           Log를 먼저 Undo한다 */
        do
        {
            /*
              BUBBUG:For Replication.
              if ( sTransQueue.m_cUTrans == 1 )
              {
              mSyncReplLSN.mFileNo = sCurUndoLSN.mFileNo;
              mSyncReplLSN.mOffset = sCurUndoLSN.mOffset;
              }
            */

            sActiveTrans = sTransInfo->mTrans;
            sCurUndoLSN  = smLayerCallback::getCurUndoNxtLSN( sActiveTrans );

            if ( undo( NULL, /* idvSQL* */
                       sActiveTrans,
                       &sLogFile )
                != IDE_SUCCESS )
            {
                IDE_TEST( startupFailure( 
                              smLayerCallback::getRTOI4UndoFailure( sActiveTrans ), 
                              ID_FALSE ) // isRedo
                          != IDE_SUCCESS );
            }

            sCurUndoLSN  = smLayerCallback::getCurUndoNxtLSN( sActiveTrans );

            if ( ( sCurUndoLSN.mFileNo == ID_UINT_MAX ) &&
                 ( sCurUndoLSN.mOffset == ID_UINT_MAX ) )
            {
                ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                             SM_TRC_MRECOVERY_RECOVERYMGR_UNDO_TRANS_ID,
                             smLayerCallback::getTransID(sActiveTrans),
                             sCurUndoLSN.mFileNo,
                             sCurUndoLSN.mOffset );
            }
            else
            {
                IDE_TEST( sTransQueue.insert( sTransInfo ) != IDE_SUCCESS );
            }

            sTransInfo = sTransQueue.remove();

            /*BUBBUG:For Replication.
              sCurUndoLSN = sTransQueue.replace(&sPrvUndoLSN);
            */
        }
        while(sTransInfo != NULL);

        sTransQueue.destroy(); //error 체크가 필요없음...

        /* -----------------------------------------------
           [3] 각 active 트랜잭션에 대해 abort log 생성
           ----------------------------------------------- */
        IDE_TEST( smLayerCallback::abortAllActiveTrans() != IDE_SUCCESS );

        /* -----------------------------------------------
           [4] Undo를 위해서 열려진 모든 Log File을 Close한다.
           ----------------------------------------------- */
        if ( sLogFile != NULL )
        {
            sCurLogFile = sLogFile;
            sLogFile = NULL;
            IDE_TEST( smrLogMgr::closeLogFile( sCurLogFile )
                      != IDE_SUCCESS );
            sCurLogFile = NULL;
        }
        else
        {
            /* nothing to do ... */
        }
    }


    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_UNDOALL_SUCCESS );

    /* ------------------------------------------------
     * [5]prepared list에 있는 각 트랜잭션에 대해 record lock 획득
     * - table lock은 refineDB 후에 획득함
     * (왜냐하면 table header의 LockItem을 초기화한 후에
     * table lock을 잡아야 하기 때문임)
     * => prepared transaction이 access한 oid list 정보는
     * redo 과정에서 이미 구성되어 있음)
     * - recovery 이후 트랜잭션 테이블의 모든 엔트리가
     *  transaction free list에 연결되어 있으므로
     *  prepare transaction을 위해 이를 재구성한다.
     * ----------------------------------------------*/
    IDE_TEST( smLayerCallback::setRowSCNForInDoubtTrans() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if ( sLogFile != NULL )
    {
        IDE_ASSERT( smrLogMgr::closeLogFile( sLogFile )
                    == IDE_SUCCESS );
        sLogFile = NULL;
    }
    IDE_POP();

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 트랜잭션 철회
 *
 * 로그를 판독하여 undo가 가능한 로그에 대해서
 * 로그 타입별 physical undo 또는 연산에 대한(NTA)
 * logical undo를 수행하기도 한다.
 **********************************************************************/
IDE_RC smrRecoveryMgr::undo( idvSQL          * aStatistics,
                             void            * aTrans,
                             smrLogFile     ** aLogFilePtr)
{

    smrDiskLog            sDiskLog;        // disk redoonly/undoable 로그
    smrDiskNTALog         sDiskNTALog;     // disk NTA 로그
    smrDiskRefNTALog      sDiskRefNTALog;  // Referenced NTA 로그
    smrTBSUptLog          sTBSUptLog;      // file 연산 로그
    smrUpdateLog          sUpdateLog;
    smrNTALog             sNTALog;
    smrLogHead            sLogHead;
    smOID                 sOID;
    smOID                 sTableOID;
    scPageID              sFirstPID;
    scPageID              sLastPID;
    SChar               * sLogPtr;
    SChar               * sBeforeImage;
    smLSN                 sCurUndoLSN;
    smLSN                 sNTALSN;
    smLSN                 sPrevUndoLSN;
    smLSN                 sCurLSN;      /* PROJ-1923 */
    sctTableSpaceNode   * sSpaceNode;
    smrCompRes          * sCompRes;
    SChar               * sFirstPagePtr;
    SChar               * sLastPagePtr;
    SChar               * sTargetObject;
    smrRTOI               sRTOI;
    idBool                sObjectConsistency = ID_TRUE;
    UInt                  sState   = 0;
    idBool                sIsValid = ID_FALSE;
    UInt                  sLogSizeAtDisk = 0;
    idBool                sIsDiscarded;

    
    IDE_ASSERT(aTrans != NULL);

    SM_LSN_INIT( sCurLSN );     // dummy

    /* ------------------------------------------------
     * [1] 로그를 판독한다.
     * ----------------------------------------------*/
    sCurUndoLSN = smLayerCallback::getCurUndoNxtLSN( aTrans );

    // 트랜잭션의 로그 압축/압축해제 리소스를 가져온다
    IDE_TEST( smLayerCallback::getTransCompRes( aTrans, &sCompRes )
              != IDE_SUCCESS );

    /* BUG-19150: [SKT Rating] 트랜잭션 롤백속도가 느림
     *
     * mmap은 순차방향으로 File을 접근했을때는 속도가 잘나오나
     * 역으로 파일을 접근했을때는 Read할때마다 IO가 발생하여 IO횟수가
     * 많게 되여 성능이 저하된다. 때문에 Rollback시에는 mmap을 이용해서
     * LogFile을 접근하지 않고 Dynamic Memory에 파일을 읽어서 처리한다.
     * */
    IDE_TEST( smrLogMgr::readLog( &sCompRes->mDecompBufferHandle,
                                  &sCurUndoLSN,
                                  ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                  aLogFilePtr,
                                  &sLogHead,
                                  &sLogPtr,
                                  &sIsValid,
                                  &sLogSizeAtDisk ) 
             != IDE_SUCCESS );
  
    IDE_TEST_RAISE( sIsValid == ID_FALSE, err_invalid_log )

    mCurLogPtr     = sLogPtr;
    mCurLogHeadPtr = &sLogHead;
    SM_GET_LSN( mLstUndoLSN, sCurUndoLSN );

    sState = 1;

    /* Undo 진행해도 되는 객체인지 확인 */
    prepareRTOI( (void*)sLogPtr,
                 &sLogHead,
                 &sCurUndoLSN,
                 NULL, /* aDRDBRedoLogData */
                 NULL, /* aDRDBPagePtr */
                 ID_FALSE, /* aIsRedo */
                 &sRTOI );
    checkObjectConsistency( &sRTOI,
                            &sObjectConsistency );

    if ( sObjectConsistency == ID_FALSE )
    {
        IDE_TEST( startupFailure( &sRTOI,
                                  ID_FALSE ) // isRedo
                  != IDE_SUCCESS );
    }
    else
    {
        /* 이후 실패한다면, Undo연산에 문제가 있는 것 */
        sRTOI.mCause = SMR_RTOI_CAUSE_UNDO;

        /* ------------------------------------------------
         * [2] MRDB의 update 타입의 로그에 대한 철회
         * - 해당 undo LSN에 대한 CLR 로그를 기록한다.
         * - 해당 로그 타입에 대한 undo를 수행한다.
         * ----------------------------------------------*/
        if ( smrLogHeadI::getType(&sLogHead) == SMR_LT_UPDATE )
        {
            idlOS::memcpy(&sUpdateLog, sLogPtr, SMR_LOGREC_SIZE(smrUpdateLog));

            // Undo SKIP할 Tablespace에 관한 로그가 아닌 경우
            if ( sctTableSpaceMgr::hasState( SC_MAKE_SPACE(sUpdateLog.mGRID),
                                             SCT_SS_SKIP_UNDO ) == ID_FALSE )
            {
                sBeforeImage = sLogPtr + SMR_LOGREC_SIZE(smrUpdateLog);

                if ( gSmrUndoFunction[sUpdateLog.mType] != NULL)
                {
                    //append compensation log
                    sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sLogHead);

                    IDE_TEST( smrLogMgr::writeCMPSLogRec( aStatistics,
                                                          aTrans,
                                                          SMR_LT_COMPENSATION,
                                                          &sPrevUndoLSN,
                                                          &sUpdateLog,
                                                          sBeforeImage )
                              != IDE_SUCCESS );

                    smrLogHeadI::setPrevLSN(&sLogHead, sPrevUndoLSN);

                    IDE_TEST( gSmrUndoFunction[sUpdateLog.mType](
                                        smrLogHeadI::getTransID(&sUpdateLog.mHead),
                                        SC_MAKE_SPACE(sUpdateLog.mGRID),
                                        SC_MAKE_PID(sUpdateLog.mGRID),
                                        SC_MAKE_OFFSET(sUpdateLog.mGRID),
                                        sUpdateLog.mData,
                                        sBeforeImage,
                                        sUpdateLog.mBImgSize,
                                        smrLogHeadI::getFlag(&sUpdateLog.mHead) )
                              != IDE_SUCCESS );
                }
                else
                {
                    // BUG-15474
                    // undo function이 NULL이라는 의미는 undo 작업을 수행하지
                    // 않는 로그라는 의미이다.
                    // undo 작업이 없다는 것은 페이지 변경 작업이 없기 때문에
                    // 굳이 CLR을 기록할 필요가 없어서 dummy CLR 기록하는 부분을
                    // 삭제한다.
                    // Nothing to do...
                }
            }

        }
        else if ( smrLogHeadI::getType(&sLogHead) == SMR_LT_TBS_UPDATE)
        {
            idlOS::memcpy(&sTBSUptLog, sLogPtr, SMR_LOGREC_SIZE(smrTBSUptLog));

            // Tablespace 노드 상태 변경 로그는 SKIP하지 않는다.
            // - Drop된 Tablespace의 경우
            //   - Undo Function안에서 SKIP한다.
            // - Offline된 Tablespace의 경우
            //   - Online상태로 전이할 수 있으므로 SKIP해서는 안된다.
            // - Discard된 Tablespace의 경우
            //   - Drop Tablespace의 Undo가 올 수 있다.

            sBeforeImage = sLogPtr + SMR_LOGREC_SIZE(smrTBSUptLog);

            //append disk compensation log
            sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sLogHead);

            IDE_TEST( smrLogMgr::writeCMPSLogRec4TBSUpt( aStatistics,
                                                         aTrans,
                                                         &sPrevUndoLSN,
                                                         &sTBSUptLog,
                                                         sBeforeImage )
                      != IDE_SUCCESS );

            smrLogHeadI::setPrevLSN(&sLogHead, sPrevUndoLSN);


            /* BUG-41689 A discarded tablespace is redone in recovery
             * Discard된 TBS인지 확인한다. */
            sIsDiscarded = sctTableSpaceMgr::hasState( sTBSUptLog.mSpaceID,
                                                       SCT_SS_SKIP_AGING_DISK_TBS,
                                                       ID_FALSE );

            if ( ( sIsDiscarded == ID_FALSE ) ||
                 ( sTBSUptLog.mTBSUptType == SCT_UPDATE_MRDB_DROP_TBS ) ||
                 ( sTBSUptLog.mTBSUptType == SCT_UPDATE_DRDB_DROP_TBS ) ||
                 ( sTBSUptLog.mTBSUptType == SCT_UPDATE_DRDB_DROP_DBF ) )
            {
                IDE_TEST( gSmrTBSUptUndoFunction[sTBSUptLog.mTBSUptType](
                                                                aStatistics,
                                                                aTrans,
                                                                sCurLSN,
                                                                sTBSUptLog.mSpaceID,
                                                                sTBSUptLog.mFileID,
                                                                sTBSUptLog.mBImgSize,
                                                                sBeforeImage,
                                                                mRestart )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do 
                 * discard된 TBS의 undo는 drop tbs를 제외하고 무시한다.*/
            }
        }
        else if (smrLogHeadI::getType(&sLogHead) == SMR_DLT_UNDOABLE)
        {
            idlOS::memcpy(&sDiskLog, sLogPtr, SMR_LOGREC_SIZE(smrDiskLog));

            // BUGBUG-1548 Disk Undoable로그 Tablespace별 SKIP여부 결정하여 처리해야함

            /* ------------------------------------------------
             * DRDB의 SMR_DLT_UNDOABLE 타입에 대한 undo
             * - undo하기전에 compensation log를 기록함
             * - DRDB의 undosegment관련 redo 로그타입을 판독한 것이며,
             *   각 layer의 undo log 타입에 따라 정해진 undo 함수를
             *   호출하여 현 로그에 대해서 undo 처리
             * ----------------------------------------------*/
            sBeforeImage = sLogPtr + SMR_LOGREC_SIZE(smrDiskLog) + sDiskLog.mRefOffset;

            sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sDiskLog.mHead);

            IDE_TEST( smLayerCallback::doUndoFunction(
                                              aStatistics,
                                              smrLogHeadI::getTransID( &sDiskLog.mHead ),
                                              sDiskLog.mTableOID,
                                              sBeforeImage,
                                              &sPrevUndoLSN )
                      != IDE_SUCCESS );

            smrLogHeadI::setPrevLSN(&sDiskLog.mHead, sPrevUndoLSN);
        }
        else if ( smrLogHeadI::getType(&sLogHead) == SMR_DLT_NTA)
        {
            idlOS::memcpy(&sDiskNTALog, sLogPtr, SMR_LOGREC_SIZE(smrDiskNTALog));
            // BUGBUG-1548 Disk NTA로그 Tablespace별 SKIP여부 결정하여 처리해야함

            /* ------------------------------------------------
             * DRDB의 SMR_DLT_NTA 타입에 대한 Logical UNDO
             * - DRDB의 redo 로그중에 operation NTA 로그에 대하여
             *   정해진 logical undo 처리
             * - dummy compensation 로그 기록
             * ----------------------------------------------*/
            sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sLogHead);
            IDE_TEST( smLayerCallback::doNTAUndoFunction( aStatistics,
                                                          aTrans,
                                                          sDiskNTALog.mOPType,
                                                          sDiskNTALog.mSpaceID,
                                                          &sPrevUndoLSN,
                                                          sDiskNTALog.mData,
                                                          sDiskNTALog.mDataCount )
                      != IDE_SUCCESS );

            smrLogHeadI::setPrevLSN(&sLogHead, sPrevUndoLSN);
        }
        else if (smrLogHeadI::getType(&sLogHead) == SMR_DLT_REF_NTA )
        {
            idlOS::memcpy(&sDiskRefNTALog, sLogPtr, SMR_LOGREC_SIZE(smrDiskRefNTALog));

            sBeforeImage = sLogPtr +
                           SMR_LOGREC_SIZE(smrDiskRefNTALog) +
                           sDiskRefNTALog.mRefOffset;

            sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sLogHead);

            IDE_TEST( smLayerCallback::doRefNTAUndoFunction( aStatistics,
                                                             aTrans,
                                                             sDiskRefNTALog.mOPType,
                                                             &sPrevUndoLSN,
                                                             sBeforeImage )
                      != IDE_SUCCESS );

            smrLogHeadI::setPrevLSN(&sLogHead, sPrevUndoLSN);
        }
        else if ( smrLogHeadI::getType(&sLogHead) == SMR_LT_NTA )
        {
            idlOS::memcpy(&sNTALog, sLogPtr, SMR_LOGREC_SIZE(smrNTALog));

            if ( sNTALog.mOPType != SMR_OP_NULL )
            {
                switch(sNTALog.mOPType)
                {
                case SMR_OP_SMM_PERS_LIST_ALLOC:
                    // Undo SKIP할 Tablespace에 관한 로그가 아닌 경우
                    if ( sctTableSpaceMgr::hasState( sNTALog.mSpaceID,
                                                     SCT_SS_SKIP_UNDO )
                         == ID_FALSE )
                    {
                        sFirstPID = (scPageID)(sNTALog.mData1);
                        sLastPID  = (scPageID)(sNTALog.mData2);

                        sNTALSN   = smLayerCallback::getLstUndoNxtLSN( aTrans );

                        IDE_ASSERT( smmManager::getPersPagePtr( sNTALog.mSpaceID,
                                                                sFirstPID,
                                                                (void**)&sFirstPagePtr )
                                    == IDE_SUCCESS );
                        IDE_ASSERT( smmManager::getPersPagePtr( sNTALog.mSpaceID,
                                                                sLastPID,
                                                                (void**)&sLastPagePtr )
                                    == IDE_SUCCESS );

                        IDE_TEST( smmManager::freePersPageList( aTrans,
                                                                sNTALog.mSpaceID,
                                                                sFirstPagePtr,
                                                                sLastPagePtr,
                                                                & sNTALSN ) 
                                  != IDE_SUCCESS);
                    }
                    break;

                case SMR_OP_SMM_CREATE_TBS:
                    if ( sctTableSpaceMgr::hasState( sNTALog.mSpaceID,
                                                     SCT_SS_SKIP_UNDO )
                         == ID_FALSE )
                    {
                       sSpaceNode = sctTableSpaceMgr::findSpaceNodeWithoutException( sNTALog.mSpaceID );

                        IDE_TEST( smmTBSDrop::doDropTableSpace( sSpaceNode,
                                                                SMI_ALL_TOUCH )
                                  != IDE_SUCCESS );
                    }
                    break;

                case SMR_OP_SMC_FIXED_SLOT_ALLOC:
                    // Undo SKIP할 Tablespace에 관한 로그가 아닌 경우
                    if ( sctTableSpaceMgr::hasState( sNTALog.mSpaceID,
                                                     SCT_SS_SKIP_UNDO )
                         == ID_FALSE )
                    {
                        // SMR_OP_SMC_LOCK_ROW_ALLOC에 대한 undo시에는
                        // Delete Bit를 세팅하지 않는다.(BUG-14596)
                        sOID = (smOID)(sNTALog.mData1);
                        IDE_ASSERT( smmManager::getOIDPtr( sNTALog.mSpaceID,
                                                           sOID,
                                                           (void**)&sTargetObject )
                                    == IDE_SUCCESS );

                        IDE_TEST( smLayerCallback::setDeleteBit( aTrans,
                                                                 sNTALog.mSpaceID,
                                                                 sTargetObject,
                                                                 SMC_WRITE_LOG_OK) 
                                  != IDE_SUCCESS );
                    }
                    break;

                case SMR_OP_CREATE_TABLE:
                    // BUGBUG-1548 DISCARD/DROP된 Tablespace의 경우 SKIP여부
                    // 결정하고 IF분기해야함
                    sOID = (smOID)(sNTALog.mData1);
                    /*  해당 table이 disk table인 경우에는 생성했던 segment도 free 한다 */
                    IDE_ASSERT( smmManager::getOIDPtr( sNTALog.mSpaceID,
                                                       sOID,
                                                       (void**)&sTargetObject )
                                == IDE_SUCCESS );
                    IDE_TEST( smLayerCallback::setDropTable( aTrans,
                                                             (SChar*)sTargetObject )
                              != IDE_SUCCESS );

                    if ( smrRecoveryMgr ::isRestart() == ID_FALSE )
                    {
                        IDE_TEST( smLayerCallback::doNTADropDiskTable( NULL,
                                                                       aTrans,
                                                                       (SChar*)sTargetObject )
                                  != IDE_SUCCESS );
                    }
                    break;

                case SMR_OP_CREATE_INDEX:
                    //drop index by abort,drop index pending.
                    IDE_TEST( smLayerCallback::dropIndexByAbort( NULL,
                                                                 aTrans,
                                                                (smOID)sNTALog.mData2,
                                                                (smOID)sNTALog.mData1 )
                              != IDE_SUCCESS );
                    break;

                case SMR_OP_DROP_INDEX:
                    // BUGBUG-1548 DISCARD/DROP된 Tablespace의 경우 SKIP여부
                    // 결정하고 IF분기해야함
                    IDE_TEST( smLayerCallback::clearDropIndexList( sNTALog.mData1 )
                              != IDE_SUCCESS );
                    break;

                case SMR_OP_INIT_INDEX:
                    /* PROJ-2184 RP Sync 성능 개선
                     * Index init 후에 abort 된 경우 index runtime header를
                     * drop 한다. */
                    /* BUG-42460 runtime시에만 실행하도록 함. */
                    if ( mRestart == ID_FALSE )
                    {
                        IDE_TEST( smLayerCallback::dropIndexRuntimeByAbort( (smOID)sNTALog.mData1,
                                                                            (smOID)sNTALog.mData2 )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                       /* nothing to do */
                    }
                    break;

                case SMR_OP_ALTER_TABLE:
                    // BUGBUG-1548 DISCARD/DROP된 Tablespace의 경우 SKIP여부
                    // 결정하고 IF분기해야함
                    sOID      = (smOID)(sNTALog.mData1);
                    sTableOID = (smOID)(sNTALog.mData2);
                    IDE_ASSERT( smLayerCallback::restoreTableByAbort( aTrans,
                                                                      sOID,
                                                                      sTableOID,
                                                                      mRestart )
                                == IDE_SUCCESS );
                    break;

                case SMR_OP_INSTANT_AGING_AT_ALTER_TABLE:
                    //BUG-15627 Alter Add Column시 Abort할 경우 Table의 SCN을 변경해야 함.
                    if ( mRestart == ID_FALSE )
                    {
                        sOID = (smOID)(sNTALog.mData1);
                        smLayerCallback::changeTableSCNForRebuild( sOID );
                    }

                    break;

                case SMR_OP_DIRECT_PATH_INSERT:
                    /* Direct-Path INSERT가 rollback 된 경우, table info에
                     * 설정해둔 DPathIns 설정을 해제해주어야 한다. */
                    if ( mRestart == ID_FALSE )
                    {
                        IDE_TEST( smLayerCallback::setExistDPathIns(
                                                aTrans,
                                                (smOID)sNTALog.mData1,
                                                ID_FALSE ) /* aExistDPathIns */
                                  != IDE_SUCCESS );
                    }
                    break;

                default :
                    break;
                }

                //write dummy compensation log
                sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sLogHead);
                IDE_TEST( smrLogMgr::writeCMPSLogRec(aStatistics,
                                                    aTrans,
                                                    SMR_LT_DUMMY_COMPENSATION,
                                                    &sPrevUndoLSN,
                                                    NULL,
                                                    NULL) != IDE_SUCCESS );
                smrLogHeadI::setPrevLSN(&sLogHead, sPrevUndoLSN);

            }
        }
    }

    /*
      Transaction의 Cur Undo Next LSN을 설정한다.
    */

    sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sLogHead);
    smLayerCallback::setCurUndoNxtLSN( aTrans, &sPrevUndoLSN );
    smrLogHeadI::setPrevLSN(&sLogHead, sPrevUndoLSN);
    mCurLogPtr     = NULL;
    mCurLogHeadPtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_log );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidLog,
                                  sCurUndoLSN.mFileNo,
                                  sCurUndoLSN.mOffset ) );
    } 
    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        sPrevUndoLSN = smrLogHeadI::getPrevLSN(&sLogHead);
        smLayerCallback::setCurUndoNxtLSN( aTrans, &sPrevUndoLSN );
        smrLogHeadI::setPrevLSN(&sLogHead, sPrevUndoLSN);
        mCurLogPtr     = NULL;
        mCurLogHeadPtr = NULL;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 트랜잭션 철회
 * aLSN [IN]   - rollback을 수행할 LSN 
                 (여기까지 undo함 특정 LSN , 혹은 init 값이 넘어옴 ) 
 **********************************************************************/
IDE_RC smrRecoveryMgr::undoTrans( idvSQL   * aStatistics,
                                  void     * aTrans,
                                  smLSN    * aLSN )
{
    smLSN         sCurUndoLSN;
    smrLogFile*   sLogFilePtr = NULL;
    SInt          sState      = 1;

    sCurUndoLSN = smLayerCallback::getLstUndoNxtLSN( aTrans );

    smLayerCallback::setCurUndoNxtLSN( aTrans, &sCurUndoLSN );

    /* BUG-21620: PSM내에 Explicit Savepoint가 있으면, PSM실패후 Explicit
     * Savepoint로 Partial Rollback하면 서버가 비정상 종료합니다. */
    while ( 1 )
    {
        if( ( !SM_IS_LSN_MAX( *aLSN ) ) &&
            ( aLSN->mFileNo >= sCurUndoLSN.mFileNo ) &&
            ( aLSN->mOffset >= sCurUndoLSN.mOffset ) )
        {
            break;
        }

        /* Transaction의 첫번째 로그의 Prev LSN과 Transaction의 모든
         * 로그를 Rolback했을때 Transaction의 마지막 CLR로그의 PrevLsN또한
         * < -1, -1>값을 가지게 된다. */
        if ( ( sCurUndoLSN.mFileNo == ID_UINT_MAX  ) ||
             ( sCurUndoLSN.mOffset == ID_UINT_MAX  ) )
        {
            IDE_ASSERT( sCurUndoLSN.mFileNo == ID_UINT_MAX );
            IDE_ASSERT( sCurUndoLSN.mOffset == ID_UINT_MAX );
            break;
        }

        IDE_TEST( undo( aStatistics,
                        aTrans,
                        &( sLogFilePtr )) != IDE_SUCCESS );

        sCurUndoLSN = smLayerCallback::getCurUndoNxtLSN( aTrans );
    }

    if ( sLogFilePtr != NULL)
    {
        sState = 0;
        IDE_TEST( smrLogMgr::closeLogFile( sLogFilePtr ) != IDE_SUCCESS );
        sLogFilePtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( sLogFilePtr != NULL ) && ( sState != 0 ) )
    {
        IDE_ASSERT( smrLogMgr::closeLogFile( sLogFilePtr )
                    == IDE_SUCCESS );
        sLogFilePtr = NULL;
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : prepare 트랜잭션을 위한 table lock 획득
 **********************************************************************/
IDE_RC smrRecoveryMgr::acquireLockForInDoubt()
{

    void*              sTrans;
    smrLogFile*        sLogFile = NULL;
    smLSN              sCurLSN;
    SChar*             sLogPtr;
    smrLogHead         sLogHead;
    smrXaPrepareLog    sXAPrepareLog;
    UInt               sOffset;
    smLSN              sTrxLstUndoNxtLSN;
    SInt               sState = 1;
    smrCompRes       * sCompRes;
    idBool             sIsValid = ID_FALSE;
    UInt               sLogSizeAtDisk = 0;


    if ( smLayerCallback::getPrepareTransCount()  > 0 )
    {
        sTrans = smLayerCallback::getFirstPreparedTrx();
        while( sTrans != NULL )
        {
            sOffset = 0;
            /* ----------------------------------------
               heuristic complete한 트랜잭션에 대해서는
               table lock을 획득하지 않음
               ---------------------------------------- */
            IDE_ASSERT( smLayerCallback::isXAPreparedCommitState( sTrans ) );

            sTrxLstUndoNxtLSN = smLayerCallback::getLstUndoNxtLSN( sTrans );
            SM_SET_LSN( sCurLSN,
                        sTrxLstUndoNxtLSN.mFileNo,
                        sTrxLstUndoNxtLSN.mOffset );

            // 트랜잭션의 로그 압축/압축해제 리소스를 가져온다
            IDE_TEST( smLayerCallback::getTransCompRes( sTrans, &sCompRes )
                      != IDE_SUCCESS );

            IDE_TEST( smrLogMgr::readLog(
                        & sCompRes->mDecompBufferHandle,
                        &sCurLSN,
                        ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                        &sLogFile,
                        &sLogHead,
                        &sLogPtr,
                        &sIsValid,
                        &sLogSizeAtDisk ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sIsValid == ID_FALSE, err_invalid_log )

            /* ----------------------------------------------------------------
               prepared transaction이 마지막으로 기록한 로그는 prepare 로그이며
               여러 개의 prepare log가 생성된 경우 이들은
               순차적으로 생성됨을 보장한다.
               -------------------------------------------------------------- */
            while (smrLogHeadI::getType(&sLogHead) == SMR_LT_XA_PREPARE)
            {
                idlOS::memcpy(&sXAPrepareLog,
                              sLogPtr,
                              SMR_LOGREC_SIZE(smrXaPrepareLog));

                IDE_ASSERT( smrLogHeadI::getTransID( &sXAPrepareLog.mHead ) ==
                            smLayerCallback::getTransID( sTrans ) );

                //lockTabe using preparedLog .
                //BUGBUG s_offset .
                smLayerCallback::lockTableByPreparedLog( sTrans,
                                                         (SChar *)(sLogPtr + SMR_LOGREC_SIZE(smrXaPrepareLog)),
                                                         sXAPrepareLog.mLockCount,&sOffset );

                SM_SET_LSN(sCurLSN,
                           smrLogHeadI::getPrevLSNFileNo(&sLogHead),
                           smrLogHeadI::getPrevLSNOffset(&sLogHead));

                // 트랜잭션의 로그 압축/압축해제 리소스를 가져온다
                IDE_TEST( smLayerCallback::getTransCompRes( sTrans, &sCompRes )
                          != IDE_SUCCESS );

                IDE_TEST( smrLogMgr::readLog(
                                     & sCompRes->mDecompBufferHandle,
                                     &sCurLSN,
                                     ID_TRUE, /* Close Log File When aLogFile doesn't include aLSN */
                                     &sLogFile,
                                     &sLogHead,
                                     &sLogPtr,
                                     &sIsValid,
                                     &sLogSizeAtDisk ) 
                         != IDE_SUCCESS );

                IDE_TEST_RAISE( sIsValid == ID_FALSE, err_invalid_log )
            }
            sTrans = smLayerCallback::getNxtPreparedTrx( sTrans );
        }

        if ( sLogFile != NULL)
        {
            sState = 0;
            IDE_TEST( smrLogMgr::closeLogFile(sLogFile) != IDE_SUCCESS );
            sLogFile = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_log );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidLog,
                                  sCurLSN.mFileNo,
                                  sCurLSN.mOffset ) );
    }
    IDE_EXCEPTION_END;

    if ( (sLogFile != NULL) && (sState != 0) )
    {
        IDE_PUSH();
        IDE_ASSERT(smrLogMgr::closeLogFile(sLogFile)
                   == IDE_SUCCESS );
        sLogFile = NULL;

        IDE_POP();
    }
    return IDE_FAILURE;

}

/***********************************************************************
 * Description : archive 로그 모드 변경 및 로그앵커 flush
 **********************************************************************/
IDE_RC smrRecoveryMgr::updateArchiveMode(smiArchiveMode aArchiveMode)
{
    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_UPDATE_ARCH_MODE,
                (aArchiveMode == SMI_LOG_ARCHIVE) ?
                (SChar*)"ARCHIVELOG" : (SChar*)"NOARCHIVELOG");

    IDE_TEST( mAnchorMgr.updateArchiveAndFlush(aArchiveMode) != IDE_SUCCESS );
    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_UPDATE_ARCH_MODE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_UPDATE_ARCH_MODE_FAILURE);

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : checkpoint for DRDB call by buffer flush thread
 ***********************************************************************/
IDE_RC smrRecoveryMgr::checkpointDRDB(idvSQL* aStatistics)
{
    smLSN  sOldestLSN;
    smLSN  sEndLSN;

    /* PROJ-2162 RestartRiskReduction
     * DB가 Consistent하지 않으면, Checkpoint를 하지 않는다. */
    if ( ( getConsistency() == ID_FALSE ) &&
         ( smuProperty::getCrashTolerance() != 2 ) )
    {
        IDE_CALLBACK_SEND_MSG("[CHECKPOINT-FAILURE] Inconsistent DB.");
    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_DRDB_START);

        smrLogMgr::getLstLSN( &sEndLSN );

        IDE_TEST( makeBufferChkpt( aStatistics,
                                   ID_FALSE,
                                   &sEndLSN,
                                   &sOldestLSN )
                  != IDE_SUCCESS );

        // fix BUG-16467
        IDE_ASSERT( sctTableSpaceMgr::lockForCrtTBS() == IDE_SUCCESS );

        // 디스크 테이블스페이스의 Redo LSN을 DBF 노드의 데이타파일 헤더에
        // 설정하기 위해 TBS 관리자에 임시로 저장한다.
        sctTableSpaceMgr::setRedoLSN4DBFileMetaHdr( &sOldestLSN,  // DiskRedoLSN
                                                    NULL );       // MemRedoLSN 

        // 데이타파일에 헤더를 기록한다.
        IDE_TEST( sddDiskMgr::syncAllTBS( aStatistics,
                                          SDD_SYNC_CHKPT )
                  != IDE_SUCCESS );

    	IDE_TEST( sdsBufferMgr::syncAllSB( aStatistics ) != IDE_SUCCESS );

        // 버퍼체크포인트이기 때문에 Disk Redo LSN만 갱신하면 된다
        // LogAnchor에 Disk Redo LSN을 설정하고 Flush한다.
        IDE_TEST( mAnchorMgr.updateRedoLSN( &sOldestLSN, NULL )
                  != IDE_SUCCESS );

        // 체크포인트 검증
        // Stable은 반드시 있어야 하고 , Unstable은 존재한다면
        // 최소한 유효한 버전이어야 한다.
        IDE_ASSERT( smmTBSMediaRecovery::identifyDBFilesOfAllTBS( ID_TRUE )
                    == IDE_SUCCESS );
        IDE_ASSERT( sddDiskMgr::identifyDBFilesOfAllTBS( aStatistics,
                                                         ID_TRUE )
                    == IDE_SUCCESS );

        //PROJ-2133 incremental backup
        if ( smrRecoveryMgr::isCTMgrEnabled() == ID_TRUE )
        {
            IDE_TEST( smriChangeTrackingMgr::flush() != IDE_SUCCESS );
        }       

        IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_DRDB_SUMMARY,
                    sOldestLSN.mFileNo,
                    sOldestLSN.mOffset);

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_DRDB_END);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV, SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_DRDB_END);

    return IDE_FAILURE;

}


/***********************************************************************
 * Description : buffer checkpoint 수행
 **********************************************************************/
IDE_RC smrRecoveryMgr::makeBufferChkpt( idvSQL      * aStatistics,
                                        idBool        aIsFinal,
                                        smLSN       * aEndLSN,     // in
                                        smLSN       * aOldestLSN)  // out
{
    smLSN  sDiskRedoLSN;
    smLSN  sSBufferRedoLSN;
 

    /* PROJ-2102 buffer pool의 dirtypage가 최신 이므로 2nd->bufferpool의 순서로 수행 */
    IDE_TEST( sdsBufferMgr::flushDirtyPagesInCPList( aStatistics,
                                                     ID_FALSE )// do not flush all
              != IDE_SUCCESS );

    /* BUG-22386 로 internal checkpoint에서도 CPList의 BCB를 flush한다. */
    IDE_TEST( sdbBufferMgr::flushDirtyPagesInCPListByCheckpoint( 
                                                        aStatistics,
                                                        aIsFinal )
              != IDE_SUCCESS );
    /* replacement flush를 통해 secondary buffer에 dirty page가 추가 발생할수 있음. */
    IDE_TEST( sdsBufferMgr::flushDirtyPagesInCPList( aStatistics,
                                                     ID_FALSE )// do not flush all
              != IDE_SUCCESS );

    sdbBufferMgr::getMinRecoveryLSN( aStatistics, &sDiskRedoLSN );

    sdsBufferMgr::getMinRecoveryLSN( aStatistics, &sSBufferRedoLSN );

    if ( smrCompareLSN::isLT( &sDiskRedoLSN, &sSBufferRedoLSN )
            == ID_TRUE )
    {
        /* PBuffer에 있는 RecoveryLSN(sDiskRedoLSN)이 
         * Sscondary Buffer에 있는 RecoveryLSN(sSBufferRedoLSN) 보다 작으면
         * PBuffer에 있는 RecoveryLSN  선택 */
        SM_GET_LSN( *aOldestLSN, sDiskRedoLSN );
    }
    else
    {
        SM_GET_LSN( *aOldestLSN, sSBufferRedoLSN );
    }

    /* DRDB의 DIRTY PAGE가 존재하지 않는다면 현재 EndLSN으로 설정한다. */
    if ( ( aOldestLSN->mFileNo == ID_UINT_MAX ) &&
         ( aOldestLSN->mOffset == ID_UINT_MAX ) )
    {
        *aOldestLSN = *aEndLSN;
    }
    else
    {
        IDE_ASSERT( ( aOldestLSN->mFileNo != ID_UINT_MAX) &&
                    ( aOldestLSN->mOffset != ID_UINT_MAX) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 Description :free space가 체크포인트를 수행할 수 있을 만큼 충분한지
              여부를 리턴함. (BUG-32177)
**********************************************************************/
idBool smrRecoveryMgr::existDiskSpace4LogFile(void)
{

    ULong           sSize;
    const SChar  *  sLogFileDir=NULL;
    idBool          sRet= ID_TRUE;

    sSize = smuProperty::getReservedDiskSizeForLogFile();

    if ( sSize != 0 )
    {
        sLogFileDir = smuProperty::getLogDirPath();

        IDE_ASSERT( sLogFileDir != NULL );

        if ( (ULong)idlVA::getDiskFreeSpace( sLogFileDir ) <= sSize )
        {
            return ID_FALSE;
        }
    }

    return sRet;
}

/***********************************************************************
 Description : checkpoint 수행
    [IN] aTrans        - Checkpoint를 수행하는 Transaction
    [IN] aCheckpointReason - Checkpoint를 하게 된 사유
    [IN] aChkptType    - Checkpoint종류 - Memory 만 ?
                                          Disk 와 Memory 모두 ?
    [IN] aRemoveLogFile - 불필요한 로그파일 제거 여부
    [IN] aIsFinal       - smrRecoveryMgr::finalize() 에서 호출되었는지
**********************************************************************/
IDE_RC smrRecoveryMgr::checkpoint( idvSQL*      aStatistics,
                                   SInt         aCheckpointReason,
                                   smrChkptType aChkptType,
                                   idBool       aRemoveLogFile,
                                   idBool       aIsFinal )
{
    IDE_DASSERT((aChkptType == SMR_CHKPT_TYPE_MRDB) ||
                (aChkptType == SMR_CHKPT_TYPE_BOTH));


    /*
     * BUG-32177  The server might hang when disk is full during checkpoint.
     */
    if ( existDiskSpace4LogFile() == ID_FALSE )
    {
        /*
         * 로그파일을 위한 디스크공간이 부족하여 check point를 실패함.
         * 이때는 메시지만을 뿌리고 다음 checkpoint시 다시시도하게됨.
         * (internal only!)
         */
        IDE_CALLBACK_SEND_MSG("[CHECKPOINT-FAILURE] disk is full.");
        IDE_CONT( SKIP );
    }

    /* PROJ-2162 RestartRiskReduction
     * DB가 Consistent하지 않으면, Checkpoint를 하지 않는다. */
    if ( ( getConsistency() == ID_FALSE ) &&
         ( smuProperty::getCrashTolerance() != 2 ) )
    {
        IDE_CALLBACK_SEND_MSG("[CHECKPOINT-FAILURE] Inconsistent DB.");
        IDE_CONT( SKIP );
    }

    //--------------------------------------------------
    //  Checkpoint를 왜 하게 되었는지를 log에 남긴다.
    //--------------------------------------------------
    IDE_TEST( logCheckpointReason( aCheckpointReason ) != IDE_SUCCESS );

    //---------------------------------
    // 실제 Checkpoint 수행
    //---------------------------------
    IDE_TEST( checkpointInternal( aStatistics,
                                  aChkptType,
                                  aRemoveLogFile,
                                  aIsFinal ) != IDE_SUCCESS );

    // PROJ-2133 incremental backup
    if ( smrRecoveryMgr::isCTMgrEnabled() == ID_TRUE )
    {
        IDE_TEST( smriChangeTrackingMgr::flush() != IDE_SUCCESS );
    }       

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("[CHECKPOINT-FAILURE]");

    idlOS::abort();

    return IDE_FAILURE;
}


/*
  Checkpoint 구현 함수 - 실제 Checkpoint를 수행한다.

  [IN] aChkptType    - Checkpoint종류 - Memory 만 ?
  Disk 와 Memory 모두 ?
  [IN] aRemoveLogFile - 불필요한 로그파일 제거 여부
  [IN] aFinal         -

  checkpoint시 결정되는 DRDB와 MRDB의 RecvLSN이
  다를수 있기 때문에 begin checkpoint log에 모두
  기록해놓아야 한다.

  RecvLSN은 다음기준으로 결정된다.
  - MRDB의 경우 : checkpoint시점의 Active Transaction
  중 가장 작은 transaction의 가장 작은 LSN
  - DRDB의 경우 : 버퍼매니저의 flush-list에 존재하는
  BCB중 가장 오래전에 변경된 BCB의 oldest LSN

  + 2nd. code design
  - logging level에 따라서 모든 트랜잭션이 종료를 대기함
  - MRDB/DRDB의 recovery LSN을 결정하여 시작 checkpoint 로그에 기록
  - 시작 checkpoint 로그 기록한다.
  - phase 1 : dirty-page list를 flush 한다.(smr->disk)
  - dirty-page를 옮겨온다.(smm->smr)
  - 현재 로그관리자의 last LSN까지의 로그를 sync시킨다.
  - phase 2 : dirty-page list를 flush 한다.(smr->disk)
  - DB 파일을 sync시킨다.
  - 완료 checkpoint 로그를 기록한다.
  - 제거할 fst 로그파일번호과 lst 로그파일번호를 구한다.
  - loganchor에 checkpoint 정보를 기록한다.
  - logging level에 따라서 트랜잭션이 발생할 수 있도록 처리한다.

  ============================================================
  CHECKPOINT 과정
  ============================================================
  A. Restart Recovery시에 Redo시작 LSN으로 사용될 Recovery LSN을 계산
  A-1 Memory Recovery LSN 결정
  A-2 Disk Recovery LSN 결정
  - Disk Dirty Page Flush        [step0]
  - Tablespace Log Anchor 동기화 [step1]
  B. Write  Begin_CHKPT                 [step2]
  C. Memory Dirty Page들을 Checkpoint Image에 Flush
  C-1 Flush Dirty Pages              [step3]
  C-2 Sync Log Files
  C-3 Sync Memory Database           [step4]
  D. 모든 테이블스페이스의 데이타파일 메타헤더에 RedoLSN 설정
  E. Write  End_CHKPT                   [step5]
  F. After  End_CHKPT
  E-1. Sync Log Files            [step6]
  E-2. Get Remove Log File #         [step7]
  E-3. Update Log Anchor             [step8]
  E-4. Remove Log Files              [step9]
  G. 체크포인트 검증
  ============================================================
*/
IDE_RC smrRecoveryMgr::checkpointInternal( idvSQL*      aStatistics,
                                           smrChkptType aChkptType,
                                           idBool       aRemoveLogFile,
                                           idBool       aIsFinal )
{
    smLSN   sBeginChkptLSN;
    smLSN   sEndChkptLSN;

    smLSN   sDiskRedoLSN;
    smLSN   sEndLSN;

    smLSN   sRedoLSN;
    smLSN   sSyncLstLSN;

    smLSN   sDtxMinLSN; /* BUG-46754 */

    smLSN * sMediaRecoveryLSN;

    IDU_FIT_POINT( "1.BUG-42785@smrRecoveryMgr::checkpointInternal::sleep" );

    //=============================================
    //  A. Restart Recovery시에 Redo시작 LSN으로 사용될 Recovery LSN을 계산
    //    A-1 Memory Recovery LSN 결정
    //    A-2 Disk Recovery LSN 결정
    //        - Disk Dirty Page Flush        [step0]
    //        - Tablespace Log Anchor 동기화 [step1]
    //=============================================

    IDE_TEST( chkptCalcRedoLSN( aStatistics,
                                aChkptType,
                                aIsFinal,
                                &sRedoLSN,
                                &sDiskRedoLSN,
                                &sEndLSN ) != IDE_SUCCESS );

    //=============================================
    // B. Write Begin_CHKPT
    //=============================================
    IDE_TEST( writeBeginChkptLog( aStatistics,
                                  &sRedoLSN,
                                  sDiskRedoLSN,
                                  sEndLSN,
                                  &sBeginChkptLSN,
                                  &sDtxMinLSN,
                                  aIsFinal ) != IDE_SUCCESS );

    /* FIT/ART/sm/Bugs/BUG-13894/BUG-13894.sql */
    IDU_FIT_POINT( "1.BUG-13894@smrRecoveryMgr::checkpointInternal" );

    //=============================================
    // C. Memory Dirty Page들을 Checkpoint Image에 Flush
    //    C-1 Flush Dirty Pages              [step3]
    //    C-2 Sync Log Files
    //    C-3 Sync Memory Database           [step4]
    //=============================================

    IDE_TEST( chkptFlushMemDirtyPages( &sSyncLstLSN, aIsFinal )
              != IDE_SUCCESS );

    /* FIT/ART/sm/Bugs/BUG-13894/BUG-13894.sql */
    IDU_FIT_POINT( "2.BUG-13894@smrRecoveryMgr::checkpointInternal" );

    /* FIT/ART/sm/Bugs/BUG-23700/BUG-23700.tc */
    IDU_FIT_POINT( "1.BUG-23700@smrRecoveryMgr::checkpointInternal" );

    //=============================================
    // D. 모든 테이블스페이스의 데이타파일 메타헤더에 RedoLSN 설정
    //=============================================
    IDE_TEST( chkptSetRedoLSNOnDBFiles( aStatistics,
                                        &sRedoLSN,
                                        sDiskRedoLSN ) != IDE_SUCCESS );

    /* FIT/ART/sm/Bugs/BUG-13894/BUG-13894.sql */
    IDU_FIT_POINT( "3.BUG-13894@smrRecoveryMgr::checkpointInternal" );

    //=============================================
    // E. Write End_CHKPT
    //=============================================
    IDE_TEST( writeEndChkptLog( aStatistics,
                                & sEndChkptLSN ) != IDE_SUCCESS );


    /* CASE-6985 서버 종료후 startup하면 user와 테이블,
     * 데이터가 모두 사라짐: Checkpoint완료전에 이에대해 검증을 수행한다.
     * 만약 Valid하지 않다면 이전에 완료된 데이타베이스 이미지를 읽어서 원복될 것이다.*/
    smmDatabase::validateCommitSCN();

    //=============================================
    // F. After End_CHKPT
    //    E-1. Sync Log Files                [step6]
    //    E-2. Get Remove Log File #         [step7]
    //    E-3. Update Log Anchor             [step8]
    //    E-4. Remove Log Files              [step9]
    //=============================================
    IDE_TEST( chkptAfterEndChkptLog( aRemoveLogFile,
                                     aIsFinal,
                                     &sBeginChkptLSN,
                                     &sEndChkptLSN,
                                     &sDiskRedoLSN,
                                     &sRedoLSN,
                                     &sSyncLstLSN,
                                     &sDtxMinLSN )
              != IDE_SUCCESS );

    /* BUG-43499
     * Disk와 Memory redo LSN중 작은 LSN을 Media Recovery LSN으로 설정 한다. */
    sMediaRecoveryLSN = (smrCompareLSN::isLTE( &sDiskRedoLSN, &sRedoLSN ) == ID_TRUE ) 
                        ? &sDiskRedoLSN : &sRedoLSN;

    IDE_TEST( mAnchorMgr.updateMediaRecoveryLSN(sMediaRecoveryLSN) != IDE_SUCCESS );

    //=============================================
    // G. 체크포인트 검증
    // 체크포인트 중이면 인자로 ID_TRUE를 넘긴다 .
    //=============================================
    // Stable은 반드시 있어야 하고 , Unstable은 존재한다면
    // 최소한 유효한 버전이어야 한다.
    IDE_ASSERT( smmTBSMediaRecovery::identifyDBFilesOfAllTBS( ID_TRUE )
                 == IDE_SUCCESS );
    IDE_ASSERT( sddDiskMgr::identifyDBFilesOfAllTBS( aStatistics,
                                                     ID_TRUE )
                 == IDE_SUCCESS );

    //---------------------------------
    // Checkpoint Summary Message
    //---------------------------------
    IDE_TEST( logCheckpointSummary( sBeginChkptLSN,
                                    sEndChkptLSN,
                                    &sRedoLSN,
                                    sDiskRedoLSN ) != IDE_SUCCESS );

    IDU_FIT_POINT( "5.BUG-42785@smrRecoveryMgr::checkpointInternal::wakeup" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/*
  Checkpoint를 왜 하게 되었는지를 altibase_sm.log에 남긴다.

  [IN] aCheckpointReason - Checkpoint를 하게 된 이유
*/
IDE_RC smrRecoveryMgr::logCheckpointReason( SInt aCheckpointReason )
{
    switch (aCheckpointReason)
    {
        case SMR_CHECKPOINT_BY_SYSTEM:
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_RECOVERYMGR_CHKP1);
            break;

        case SMR_CHECKPOINT_BY_LOGFILE_SWITCH:
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_RECOVERYMGR_CHKP2,
                        smuProperty::getChkptIntervalInLog());
            break;

        case SMR_CHECKPOINT_BY_TIME:
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_RECOVERYMGR_CHKP3,
                        smuProperty::getChkptIntervalInSec());
            break;

        case SMR_CHECKPOINT_BY_USER:
            ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                        SM_TRC_MRECOVERY_RECOVERYMGR_CHKP4);
            break;
    }

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP5);

    return IDE_SUCCESS;
}


/*
  Begin Checkpoint Log를 기록한다.

  [IN] aRedoLSN - Redo 시작 LSN
  [IN] aDiskRedoLSN - Disk Redo LSN
  [OUT] aBeginChkptLSN - Begin Checkpoint Log의 LSN
*/
IDE_RC smrRecoveryMgr::writeBeginChkptLog( idvSQL* aStatistics,
                                           smLSN * aRedoLSN,
                                           smLSN   aDiskRedoLSN,
                                           smLSN   aEndLSN,
                                           smLSN * aBeginChkptLSN,
                                           smLSN * aDtxMinLSN,
                                           idBool  aIsFinal )
{
    smrBeginChkptLog   sBeginChkptLog;
    smLSN              sBeginChkptLSN;

    //---------------------------------
    // Set Begin CHKPT Log
    //---------------------------------
    smrLogHeadI::setType(&sBeginChkptLog.mHead, SMR_LT_CHKPT_BEGIN);
    smrLogHeadI::setSize(&sBeginChkptLog.mHead,
                         SMR_LOGREC_SIZE(smrBeginChkptLog));
    smrLogHeadI::setTransID(&sBeginChkptLog.mHead, SM_NULL_TID);
    smrLogHeadI::setPrevLSN(&sBeginChkptLog.mHead,
                            ID_UINT_MAX,
                            ID_UINT_MAX);

    smrLogHeadI::setFlag(&sBeginChkptLog.mHead, SMR_LOG_TYPE_NORMAL);
    sBeginChkptLog.mTail = SMR_LT_CHKPT_BEGIN;

    smrLogHeadI::setReplStmtDepth( &sBeginChkptLog.mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    sBeginChkptLog.mEndLSN = *aRedoLSN;

    sBeginChkptLog.mDiskRedoLSN = aDiskRedoLSN;

    /* BUG-48501 
       Shutdown시에 DK모듈이 SM모듈보다 먼저 내려간다.
       Shutdonw시에는 초기값을 설정하도록 수정하였다. */
    if ( ( aIsFinal == ID_TRUE ) ||
         ( mGetDtxMinLSNFunc == NULL ) )
    {
        SMI_LSN_MAX( sBeginChkptLog.mDtxMinLSN );
    }
    else
    {
        sBeginChkptLog.mDtxMinLSN = mGetDtxMinLSNFunc(); /* dktGlobalTxMgr::getDtxMinLSN() */
    }

    IDE_TEST( smrLogMgr::writeLog( aStatistics,
                                   NULL,
                                   (SChar*)& sBeginChkptLog,
                                   NULL,             // Prev LSN Ptr
                                   & sBeginChkptLSN, // Log Begin LSN
                                   NULL,             // END LSN Ptr
                                   SM_NULL_OID )
              != IDE_SUCCESS );


    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP6,
                sBeginChkptLSN.mFileNo,
                sBeginChkptLSN.mOffset,
                aEndLSN.mFileNo,
                aEndLSN.mOffset,
                aDiskRedoLSN.mFileNo,
                aDiskRedoLSN.mOffset);

    * aBeginChkptLSN = sBeginChkptLSN ;
    * aDtxMinLSN     = sBeginChkptLog.mDtxMinLSN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  모든 테이블스페이스의 데이타파일 메타헤더에 RedoLSN 설정

  [IN] aRedoLSN  - Disk/Memory DB의 Redo 시작 LSN
  [IN] aDiskRedoLSN - Disk DB의 Redo 시작 LSN
*/
IDE_RC smrRecoveryMgr::chkptSetRedoLSNOnDBFiles( idvSQL* aStatistics,
                                                 smLSN * aRedoLSN,
                                                 smLSN   aDiskRedoLSN )
{

    //=============================================
    // D. 모든 테이블스페이스의 데이타파일 메타헤더에 RedoLSN 설정
    //=============================================

    // fix BUG-16467
    IDE_ASSERT( sctTableSpaceMgr::lockForCrtTBS() == IDE_SUCCESS );

    // Redo LSN을 모든 테이블스페이스의 데이타파일/체크포인트
    // 이미지의 메타헤더에 기록하기 위해 TBS 관리자에 임시로 저장한다.
    sctTableSpaceMgr::setRedoLSN4DBFileMetaHdr( &aDiskRedoLSN,
                                                aRedoLSN );

    // 디스크 데이타파일과 체크포인트이미지의 메타헤더에 RedoLSN을 기록한다.
    IDE_TEST( smmTBSMediaRecovery::updateDBFileHdr4AllTBS() != IDE_SUCCESS );

    // 디스크 데이타파일과 체크포인트이미지의 메타헤더에 RedoLSN을 기록한다.
    IDE_TEST( sddDiskMgr::syncAllTBS( aStatistics,
                                      SDD_SYNC_CHKPT ) != IDE_SUCCESS );

    IDE_TEST( sdsBufferMgr::syncAllSB( aStatistics) != IDE_SUCCESS );

    // fix BUG-16467
    IDE_ASSERT( sctTableSpaceMgr::unlockForCrtTBS() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*  End Checkpoint Log를 기록한다.

    [OUT] aEndChkptLSN - End Checkpoint Log의 LSN
*/
IDE_RC smrRecoveryMgr::writeEndChkptLog( idvSQL* aStatistics,
                                         smLSN * aEndChkptLSN )
{
    smLSN             sEndChkptLSN;
    smrEndChkptLog    sEndChkptLog;

    //=============================================
    // E. Write End_CHKPT
    //=============================================

    // 5) Write end check point
    smrLogHeadI::setType(&sEndChkptLog.mHead, SMR_LT_CHKPT_END);
    smrLogHeadI::setSize(&sEndChkptLog.mHead, SMR_LOGREC_SIZE(smrEndChkptLog));
    smrLogHeadI::setTransID(&sEndChkptLog.mHead, SM_NULL_TID);

    smrLogHeadI::setPrevLSN(&sEndChkptLog.mHead,
                            ID_UINT_MAX,
                            ID_UINT_MAX);

    smrLogHeadI::setFlag(&sEndChkptLog.mHead, SMR_LOG_TYPE_NORMAL);
    sEndChkptLog.mTail = SMR_LT_CHKPT_END;

    smrLogHeadI::setReplStmtDepth( &sEndChkptLog.mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    IDE_TEST( smrLogMgr::writeLog( aStatistics,
                                   NULL,
                                   (SChar*)&sEndChkptLog,
                                   NULL,            // Prev LSN Ptr
                                   &(sEndChkptLSN), // Log Begin LSN
                                   NULL,            // END LSN Ptr
                                   SM_NULL_OID )
             != IDE_SUCCESS );


    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP7,
                sEndChkptLSN.mFileNo,
                sEndChkptLSN.mOffset);


    * aEndChkptLSN = sEndChkptLSN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  Checkpoint 요약 Message를 로깅한다.
*/
IDE_RC smrRecoveryMgr::logCheckpointSummary( smLSN   aBeginChkptLSN,
                                             smLSN   aEndChkptLSN,
                                             smLSN * aRedoLSN,
                                             smLSN   aDiskRedoLSN )
{
    //=============================================
    // F. 체크포인트 검증
    // 체크포인트 중이면 인자로 ID_TRUE를 넘긴다 .
    //=============================================
    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP8,
                aBeginChkptLSN.mFileNo,
                aBeginChkptLSN.mOffset,
                aEndChkptLSN.mFileNo,
                aEndChkptLSN.mOffset,
                aDiskRedoLSN.mFileNo,
                aDiskRedoLSN.mOffset);

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP9,
                aRedoLSN->mFileNo,
                aRedoLSN->mOffset);

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP10);

    return IDE_SUCCESS;
}

/*
  ALTER TABLESPACE OFFLINE을 위해 Tablespace의 모든 Dirty Page를 Flush

  [IN] aTBSNode - Tablespace의 Node

  PROJ-1548-M5 Alter Tablespace Offline/Online/Discard -----------------

  Alter Tablespace Offline시 호출되어
  Offline으로 전이하려는 Tablespace를 포함한 모든 Tablespace에 대해
  Checkpoint를 수행한다.

  Offline으로 전이하려는 Tablespace의
  Dirty Page들을 Disk의 Checkpoint Image에 Flush하기 위해 사용된다.

  Offline으로 전이하려는 Tablespace의 Dirty Page만 따로
  Checkpoint Image에 Flush하는것이 원칙상 맞지만,
  기존 Checkpoint루틴을 그대로 이용하여 코드변경량을 최소화 하였다.

  [알고리즘]

  (010) Checkpoint Latch 획득
  (020)   Checkpoint를 수행하여 Dirty Page Flush 실시
  ( Unstable DB 부터, 0번 1번 DB 모두 실시 )

  (030) Checkpoint Latch 해제
*/
IDE_RC smrRecoveryMgr::flushDirtyPages4AllTBS()
{
    UInt sStage = 0;
    UInt i;

    //////////////////////////////////////////////////////////////////////
    //  (010) Checkpoint 수행하지 못하도록 막음
    IDE_TEST( smrChkptThread::blockCheckpoint() != IDE_SUCCESS );
    sStage = 1;

    //////////////////////////////////////////////////////////////////////
    //  (020)   Dirty Page Flush 실시
    //          Tablespace의 Unstable Checkpoint Image부터 시작하여,
    //          0번 1번 Checkpoint Image에 대해 다음을 수행

    for (i=1; i<=SMM_PINGPONG_COUNT; i++)
    {
        IDE_TEST( checkpointInternal( NULL, /* idvSQL* */
                                      SMR_CHKPT_TYPE_MRDB,
                                      ID_TRUE, /* Remove Log File */
                                      ID_FALSE /* aFinal */ )
                  != IDE_SUCCESS );
    }

    //////////////////////////////////////////////////////////////////////
    //  (030) Checkpoint 수행 가능하도록 unblock
    sStage = 0;
    IDE_TEST( smrChkptThread::unblockCheckpoint() != IDE_SUCCESS );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 1:
                IDE_ASSERT( smrChkptThread::unblockCheckpoint()
                            == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE ;
}


/*
  특정 Tablespace에 Dirty Page가 없음을 확인한다.

  [IN] aTBSNode - Dirty Page가 없음을 확인할 Tablespace의 Node
*/
IDE_RC smrRecoveryMgr::assertNoDirtyPagesInTBS( smmTBSNode * aTBSNode )
{
    scPageID sDirtyPageCount;

    IDE_TEST( smrDPListMgr::getDirtyPageCountOfTBS( aTBSNode->mHeader.mID,
                                                    & sDirtyPageCount )
              != IDE_SUCCESS );

    // Dirty Page가 없어야 한다.
    IDE_ASSERT( sDirtyPageCount == 0);

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE ;
}

/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   모든 테이블스페이스 정보를 Loganchor에 갱신한다.
   서버구동과정과 서버 종료과정에서만 호출된다.
*/
IDE_RC smrRecoveryMgr::updateAnchorAllTBS()
{
    return mAnchorMgr.updateAllTBSAndFlush();
}
/*
   PROJ-2102 Fast Sencondary Buffer
   모든 테이블스페이스 정보를 Loganchor에 갱신한다.
   서버구동과정과 서버 종료과정에서만 호출된다.
*/
IDE_RC smrRecoveryMgr::updateAnchorAllSB( void )
{
    return mAnchorMgr.updateAllSBAndFlush();
}

/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   변경된 TBS Node 정보를 Loganchor에 갱신한다.

   [IN] aSpaceNode : 테이블스페이스 노드
*/
IDE_RC smrRecoveryMgr::updateTBSNodeToAnchor( sctTableSpaceNode*  aSpaceNode )
{
    return mAnchorMgr.updateTBSNodeAndFlush( aSpaceNode );
}

/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   변경된 DBF Node 정보를 Loganchor에 갱신한다.

   [IN] aFileNode  : 데이타파일 노드
*/
IDE_RC smrRecoveryMgr::updateDBFNodeToAnchor( sddDataFileNode*    aFileNode )
{
    return mAnchorMgr.updateDBFNodeAndFlush( aFileNode );
}

/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   Chkpt Image 속성을 Loganchor에 변경한다.

   [IN] aCrtDBFileInfo   -
   체크포인트 Image 에 대한 생성정보를 가진 Runtime 구조체
   Anchor Offset이 저장되어 있다.
   [IN] aChkptImageAttr  -
   체크포인트 Image 속성
*/
IDE_RC smrRecoveryMgr::updateChkptImageAttrToAnchor( smmCrtDBFileInfo  * aCrtDBFileInfo,
                                                     smmChkptImageAttr * aChkptImageAttr )
{
    IDE_DASSERT( aCrtDBFileInfo  != NULL );
    IDE_DASSERT( aChkptImageAttr != NULL );

    return mAnchorMgr.updateChkptImageAttrAndFlush( aCrtDBFileInfo,
                                                    aChkptImageAttr );
}

/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   Chkpt Path 속성을 Loganchor에 변경한다.

   [IN] aChkptPathNode - 변경할 체크포인트 경로 노드

*/
IDE_RC smrRecoveryMgr::updateChkptPathToLogAnchor( smmChkptPathNode * aChkptPathNode )
{
    IDE_ASSERT( aChkptPathNode != NULL );

    return mAnchorMgr.updateChkptPathAttrAndFlush( aChkptPathNode );
}

/* PDOJ-2102 Fast Secondary Buffer */
IDE_RC smrRecoveryMgr::updateSBufferNodeToAnchor( sdsFileNode  * aFileNode )
{
    return mAnchorMgr.updateSBufferNodeAndFlush( aFileNode );
}

/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   생성된 TBS Node 정보를 Loganchor에 추가한다.

   [IN] aSpaceNode : 테이블스페이스 노드
*/
IDE_RC smrRecoveryMgr::addTBSNodeToAnchor( sctTableSpaceNode*   aSpaceNode )
{
    UInt sAnchorOffset;

    IDE_TEST( mAnchorMgr.addTBSNodeAndFlush( aSpaceNode,
                                             &sAnchorOffset ) != IDE_SUCCESS );
    
    switch( sctTableSpaceMgr::getTBSLocation( aSpaceNode ) )
    {
        case SMI_TBS_MEMORY:
            {
                // 저장되기 전이므로 다음을 만족해야한다.
                IDE_ASSERT( ((smmTBSNode*)aSpaceNode)->mAnchorOffset
                            == SCT_UNSAVED_ATTRIBUTE_OFFSET );

                // 메모리 테이블스페이스
                ((smmTBSNode*)aSpaceNode)->mAnchorOffset = sAnchorOffset;
            }
            break;
        case SMI_TBS_DISK:
            {
                IDE_ASSERT( ((sddTableSpaceNode*)aSpaceNode)->mAnchorOffset
                            == SCT_UNSAVED_ATTRIBUTE_OFFSET );

                // 디스크 테이블스페이스
                ((sddTableSpaceNode*)aSpaceNode)->mAnchorOffset = sAnchorOffset;
            }
            break;
        case SMI_TBS_VOLATILE:
            {
                IDE_ASSERT( ((svmTBSNode*)aSpaceNode)->mAnchorOffset
                            == SCT_UNSAVED_ATTRIBUTE_OFFSET );

                // Volatile tablespace
                ((svmTBSNode*)aSpaceNode)->mAnchorOffset = sAnchorOffset;
            }
            break;
        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   생성된 DBF Node 정보를 Loganchor에 추가한다.

   TBS Node의 NewFileID를 함께 갱신하여야 한다.

   [IN] aSpaceNode : 테이블스페이스 노드
   [IN] aFileNode  : 데이타파일 노드
*/
IDE_RC smrRecoveryMgr::addDBFNodeToAnchor( sddTableSpaceNode*   aSpaceNode,
                                           sddDataFileNode*     aFileNode )
{
    UInt  sAnchorOffset;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aFileNode  != NULL );

    IDE_TEST( mAnchorMgr.addDBFNodeAndFlush( aSpaceNode,
                                             aFileNode,
                                             &sAnchorOffset )
              != IDE_SUCCESS );

    // 저장되기 전이므로 다음을 만족해야한다.
    IDE_ASSERT( aFileNode->mAnchorOffset
                == SCT_UNSAVED_ATTRIBUTE_OFFSET );

    aFileNode->mAnchorOffset = sAnchorOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   생성된 Chkpt Node Path정보를 Loganchor에 추가한다.

   TBS Node의 ChkptNode Count를 함께 갱신하여야 한다.

   [IN] aSpaceNode : 테이블스페이스 노드
   [IN] aChkptPathNode  : 체크포인트Path 노드
*/
IDE_RC smrRecoveryMgr::addChkptPathNodeToAnchor( smmChkptPathNode * aChkptPathNode )
{
    UInt sAnchorOffset;

    IDE_DASSERT( aChkptPathNode != NULL );

    IDE_TEST( mAnchorMgr.addChkptPathNodeAndFlush( aChkptPathNode,
                                                   &sAnchorOffset )
              != IDE_SUCCESS );

    // 저장되기 전이므로 다음을 만족해야한다.
    IDE_ASSERT( aChkptPathNode->mAnchorOffset
                == SCT_UNSAVED_ATTRIBUTE_OFFSET );

    aChkptPathNode->mAnchorOffset = sAnchorOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   PROJ-1548 SM - User Memory TableSpace 개념도입
   생성된 Chkpt Image 속성을 Loganchor에 추가한다.

   [IN] aCrtDBFileInfo   -
   체크포인트 Image 에 대한 생성정보를 가진 Runtime 구조체
   Anchor Offset이 저장되어 있다.
   [IN] aChkptImageAttr  : 체크포인트 Image 속성
*/
IDE_RC smrRecoveryMgr::addChkptImageAttrToAnchor(
    smmCrtDBFileInfo  * aCrtDBFileInfo,
    smmChkptImageAttr * aChkptImageAttr )
{
    UInt   sAnchorOffset;

    IDE_DASSERT( aCrtDBFileInfo  != NULL );
    IDE_DASSERT( aChkptImageAttr != NULL );

    IDE_TEST( mAnchorMgr.addChkptImageAttrAndFlush(
                                      aChkptImageAttr,
                                      &sAnchorOffset )
              != IDE_SUCCESS );

    // 저장되기 전이므로 다음을 만족해야한다.
    IDE_ASSERT( aCrtDBFileInfo->mAnchorOffset
                == SCT_UNSAVED_ATTRIBUTE_OFFSET );

    aCrtDBFileInfo->mAnchorOffset = sAnchorOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   PROJ-2102 Fast Secondary Buffer

   [IN] aFileNode  : Secondary Buffer 파일 노드
*/
IDE_RC smrRecoveryMgr::addSBufferNodeToAnchor( sdsFileNode*   aFileNode )
{
    UInt  sAnchorOffset;

    IDE_DASSERT( aFileNode  != NULL );

    IDE_TEST( mAnchorMgr.addSBufferNodeAndFlush( aFileNode,
                                                 &sAnchorOffset )
              != IDE_SUCCESS );

    // 저장되기 전이므로 다음을 만족해야한다.
    IDE_ASSERT( aFileNode->mAnchorOffset
                == SCT_UNSAVED_ATTRIBUTE_OFFSET );

    aFileNode->mAnchorOffset = sAnchorOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description : loganchor에 TXSEG Entry 개수를 반영한다.
 *******************************************************************/
IDE_RC smrRecoveryMgr::updateTXSEGEntryCnt( UInt sEntryCnt )
{
    IDE_ASSERT( sEntryCnt > 0 );

    IDE_TEST( mAnchorMgr.updateTXSEGEntryCntAndFlush( sEntryCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * DESCRITPION : loganchor로부터 트랜잭션 세그먼트 엔트리 개수를 반환
 *******************************************************************/
UInt smrRecoveryMgr::getTXSEGEntryCnt()
{
    return mAnchorMgr.getTXSEGEntryCnt();
}


/*******************************************************************
 * DESCRITPION : loganchor로부터 binary db version을 반환
 *******************************************************************/
UInt smrRecoveryMgr::getSmVersionIDFromLogAnchor()
{
    return mAnchorMgr.getSmVersionID();
}

/*******************************************************************
 * DESCRITPION : loganchor로부터 DISK REDO LSN을 반환
 *******************************************************************/
void smrRecoveryMgr::getDiskRedoLSNFromLogAnchor( smLSN* aDiskRedoLSN )
{
    *aDiskRedoLSN = mAnchorMgr.getDiskRedoLSN();
    return;
}

/*******************************************************************
 * DESCRITPION : loganchor로부터 last delete 로그파일 no 반환
 *******************************************************************/
UInt smrRecoveryMgr::getLstDeleteLogFileNo()
{

    return mAnchorMgr.getLstDeleteLogFileNo();

}

/*******************************************************************
 * DESCRITPION : loganchor로부터 archive 로그 모드 반환
 *******************************************************************/
smiArchiveMode smrRecoveryMgr::getArchiveMode()
{

    return mAnchorMgr.getArchiveMode();
}

/*******************************************************************
 * DESCRITPION : loganchor로부터 change tracking manager의 상태가 
 *               Enable되었는지 판한하여 결과 반환
 * PROJ-2133 incremental backup
 *******************************************************************/
idBool smrRecoveryMgr::isCTMgrEnabled()
{
    idBool          sResult;
    smriCTMgrState  sCTMgrState;

    sCTMgrState = mAnchorMgr.getCTMgrState();

    if ( sCTMgrState == SMRI_CT_MGR_ENABLED )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }
    return sResult;
}

/*******************************************************************
 * DESCRITPION : loganchor로부터 change tracking manager의 상태가 
 *               Create중인지 판한하여 결과 반환
 * PROJ-2133 incremental backup
 *******************************************************************/
idBool smrRecoveryMgr::isCreatingCTFile()
{
    idBool          sResult;
    smriCTMgrState  sCTMgrState;

    sCTMgrState = mAnchorMgr.getCTMgrState();

    if ( sCTMgrState == SMRI_CT_MGR_FILE_CREATING )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }
    return sResult;
}
/*******************************************************************
 * DESCRITPION :
 * PROJ-2133 incremental backup
 *******************************************************************/
IDE_RC smrRecoveryMgr::getDataFileDescSlotIDFromLogAncho4ChkptImage( 
                                UInt                       aReadOffset,
                                smiDataFileDescSlotID    * aDataFileDescSlotID)
{
    IDE_DASSERT( aDataFileDescSlotID != NULL );
    
    IDE_TEST( mAnchorMgr.getDataFileDescSlotIDFromChkptImageAttr( 
                                                aReadOffset,
                                                aDataFileDescSlotID )
              != IDE_SUCCESS );    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * DESCRITPION :
 * PROJ-2133 incremental backup
 *******************************************************************/
IDE_RC smrRecoveryMgr::getDataFileDescSlotIDFromLogAncho4DBF( 
                                UInt                       aReadOffset,
                                smiDataFileDescSlotID    * aDataFileDescSlotID)
{
    IDE_DASSERT( aDataFileDescSlotID != NULL );
    
    IDE_TEST( mAnchorMgr.getDataFileDescSlotIDFromDBFNodeAttr( 
                                                aReadOffset,
                                                aDataFileDescSlotID )
              != IDE_SUCCESS );    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * DESCRITPION : loganchor로부터 backup info manager의 상태가 
 *               initialize인지 판한하여 결과 반환
 * PROJ-2133 incremental backup
 *******************************************************************/
smriBIMgrState smrRecoveryMgr::getBIMgrState()
{
    return mAnchorMgr.getBIMgrState();
}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 CTFileAttr의 정보를 갱신한다.
 * PROJ-2133 incremental backup
 *******************************************************************/
IDE_RC smrRecoveryMgr::updateCTFileAttrToLogAnchor( 
                                            SChar          * aFileName,
                                            smriCTMgrState * aCTMgrState,
                                            smLSN          * aFlushLSN )
{
    IDE_TEST( mAnchorMgr.updateCTFileAttr( aFileName,
                                           aCTMgrState,
                                           aFlushLSN ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 BIFileAttr의 정보를 갱신한다.
 * PROJ-2133 incremental backup
 *******************************************************************/
IDE_RC smrRecoveryMgr::updateBIFileAttrToLogAnchor( 
                                           SChar          * aFileName,
                                           smriBIMgrState * aBIMgrState,
                                           smLSN          * aBackupLSN,
                                           SChar          * aBackupDir,
                                           UInt           * aDeleteArchLogFile )
{
    IDE_TEST( mAnchorMgr.updateBIFileAttr( aFileName,
                                           aBIMgrState,
                                           aBackupLSN,
                                           aBackupDir,
                                           aDeleteArchLogFile ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 CTFile이름을 가져온다.
 * PROJ-2133 incremental backup
 *******************************************************************/
SChar* smrRecoveryMgr::getCTFileName() 
{

    return mAnchorMgr.getCTFileName();

}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 BIFile이름을 가져온다.
 * PROJ-2133 incremental backup
 *******************************************************************/
SChar* smrRecoveryMgr::getBIFileName() 
{

    return mAnchorMgr.getBIFileName();

}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 마지막 flush LSN을 가져온다.
 * PROJ-2133 incremental backup
 *******************************************************************/
smLSN smrRecoveryMgr::getCTFileLastFlushLSNFromLogAnchor()
{

    return mAnchorMgr.getCTFileLastFlushLSN();

}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 마지막 backup LSN을 가져온다.
 * PROJ-2133 incremental backup
 *******************************************************************/
smLSN smrRecoveryMgr::getBIFileLastBackupLSNFromLogAnchor()
{

    return mAnchorMgr.getBIFileLastBackupLSN();

}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 마지막 이전에 수행된 backup LSN을 가져온다.
 * PROJ-2133 incremental backup
 *******************************************************************/
smLSN smrRecoveryMgr::getBIFileBeforeBackupLSNFromLogAnchor()
{

    return mAnchorMgr.getBIFileBeforeBackupLSN();

}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 incremental backup 디렉토리 경로를
 * 갱신한다.
 * PROJ-2133 incremental backup
 *******************************************************************/
IDE_RC smrRecoveryMgr::changeIncrementalBackupDir( SChar * aNewBackupDir )
{
    UInt    sBackupPathLen;
    SChar   sBackupDir[ SM_MAX_FILE_NAME ];

    idlOS::memset( sBackupDir, 0x00, SM_MAX_FILE_NAME );

    sBackupPathLen = idlOS::strlen(aNewBackupDir);

    if ( aNewBackupDir[ sBackupPathLen -1 ] != IDL_FILE_SEPARATOR)
    {
        idlOS::snprintf( sBackupDir, SM_MAX_FILE_NAME, "%s%c",
                         aNewBackupDir,                 
                         IDL_FILE_SEPARATOR );
    }  
    else
    {
        /* BUG-35113 ALTER DATABASE CHANGE BACKUP DIRECTORY '/backup_dir/'
         * statement is not working */
        idlOS::snprintf( sBackupDir, SM_MAX_FILE_NAME, "%s",
                         aNewBackupDir );
    } 

    IDE_TEST( mAnchorMgr.updateBIFileAttr( NULL,
                                           NULL,
                                           NULL,
                                           sBackupDir,
                                           NULL ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * DESCRITPION : loganchor에 저장된 incremental backup 위치를 반환한다.
 * PROJ-2133 incremental backup
 *******************************************************************/
SChar* smrRecoveryMgr::getIncrementalBackupDirFromLogAnchor()
{

     return mAnchorMgr.getIncrementalBackupDir();

}

/*********************************************************
 * function description: smrRecoveryMgr::resetLogFiles
 * -  incompleted media recovery가 되었을때,
 * 즉 복구가 현재시점까지 되지 않고, 과거의 특정시점까지
 * 되었을때 active  redo log파일을 reset시켜,
 * redo all, undo all을 skip하도록 한다.
 *********************************************************/
IDE_RC smrRecoveryMgr::resetLogFiles()
{
    UInt               j;
    UInt               sBeginLogFileNo;
    UInt               sArchMultiplexDirPathCnt;
    UInt               sLogMultiplexDirPathCnt;
    smLSN              sResetLSN;
    smLSN              sEndLSN;
    SChar              sMsgBuf[SM_MAX_FILE_NAME];
    SChar              sLogFileName[SM_MAX_FILE_NAME];
    const SChar**      sArchMultiplexDirPath;
    const SChar**      sLogMultiplexDirPath;
    smrLogFile*        sLogFile = NULL;

    sArchMultiplexDirPath    = smuProperty::getArchiveMultiplexDirPath();
    sArchMultiplexDirPathCnt = smuProperty::getArchiveMultiplexCount();

    sLogMultiplexDirPath     = smuProperty::getLogMultiplexDirPath();
    sLogMultiplexDirPathCnt  = smuProperty::getLogMultiplexCount();

    IDE_TEST_RAISE( getArchiveMode() != SMI_LOG_ARCHIVE, err_archivelog_mode );

    mAnchorMgr.getResetLogs( &sResetLSN );
    mAnchorMgr.getEndLSN( &sEndLSN );

    IDE_TEST_RAISE( ( sResetLSN.mFileNo == ID_UINT_MAX ) &&
                    ( sResetLSN.mOffset == ID_UINT_MAX ),
                    err_resetlog_no_needed );

    IDE_ASSERT( smrCompareLSN::isEQ( &(sResetLSN),
                                     &(sEndLSN) ) == ID_TRUE );

    IDE_CALLBACK_SEND_SYM("  [SM] Recovery Phase - 0 : Reset logfiles");

    // fix BUG-15840
    // 미디어복구에서 resetlogs 를 수행할때 archive log에 대해서도 고려해야함
    //   Online 로그파일 및 Archive 로그파일도 함께 ResetLogs를 수행한다.

    // [1] Delete Archive Logfiles
    idlOS::snprintf( sMsgBuf,
                     SM_MAX_FILE_NAME,
                     "\n     Archive Deleting logfile%"ID_UINT32_FMT,
                     sResetLSN.mFileNo );

    IDE_CALLBACK_SEND_SYM(sMsgBuf);

    // Delete archive logfiles
    IDE_TEST( deleteLogFiles( (SChar*)smuProperty::getArchiveDirPath(),
                              sResetLSN.mFileNo )
              != IDE_SUCCESS );

    // Delete multiplex archive logfiles
    for ( j = 0; j < sArchMultiplexDirPathCnt; j++ )
    {
        IDE_TEST( deleteLogFiles(  (SChar*)sArchMultiplexDirPath[j],
                                   sResetLSN.mFileNo )
                  != IDE_SUCCESS );
    }

    IDE_CALLBACK_SEND_SYM("\n     [ SUCCESS ]");

    // [2] Delete Online Logfiles
    if ( sResetLSN.mOffset == 0 )
    {
        // 3단계에서 partial clear할 필요없이 파일을 삭제한다.
        sBeginLogFileNo = sResetLSN.mFileNo;
    }
    else
    {
        // 3단계에서 partial clear하기 위해 남겨둔다.
        sBeginLogFileNo = sResetLSN.mFileNo + 1;
    }

    idlOS::snprintf(
            sMsgBuf,
            SM_MAX_FILE_NAME,
            "\n     Online  Deleting logfile%"ID_UINT32_FMT,
            sBeginLogFileNo );

    IDE_CALLBACK_SEND_SYM(sMsgBuf);

    IDE_TEST( deleteLogFiles( (SChar*)smuProperty::getLogDirPath(),
                              sBeginLogFileNo )
              != IDE_SUCCESS );

    // Delete multiplex logfiles
    for ( j = 0; j < sLogMultiplexDirPathCnt; j++ )
    {
        IDE_TEST( deleteLogFiles( (SChar*)sLogMultiplexDirPath[j],
                                  sBeginLogFileNo )
                  != IDE_SUCCESS );
    }

    IDE_CALLBACK_SEND_SYM("\n     [ SUCCESS ]");

    // [3] Clear Online Logfile
    idlOS::snprintf(sMsgBuf,
                    SM_MAX_FILE_NAME,
                    "\n     Reset log sequence number < %"ID_UINT32_FMT",%"ID_UINT32_FMT" >",
                    sResetLSN.mFileNo,
                    sResetLSN.mOffset);

    IDE_CALLBACK_SEND_MSG(sMsgBuf);

    if (sResetLSN.mOffset != 0)
    {
        idlOS::snprintf( sLogFileName,
                         SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT,
                         smuProperty::getLogDirPath(),
                         IDL_FILE_SEPARATOR,
                         SMR_LOG_FILE_NAME,
                         sResetLSN.mFileNo );
        IDE_TEST( idf::access(sLogFileName, F_OK) != 0);
        IDE_TEST( smrLogMgr::openLogFile( sResetLSN.mFileNo,
                                          ID_TRUE,  // Write Mode
                                          &sLogFile)
                 != IDE_SUCCESS );

        IDE_TEST( sLogFile == NULL);
        IDE_TEST( sLogFile->mFileNo != sResetLSN.mFileNo);

        // BUG-14771 Log File을 write mode로 open시
        //           Log Buffer Type이 memory인 경우에는
        //           memset 하므로, 로그파일 새로 읽어온다.
        //           sLogFile->syncToDisk 에서 Direct I/O 단위로
        //           Disk에 로그를 내리기 때문에 로그를 미리 읽어야 함
        if ( smuProperty::getLogBufferType( ) == SMU_LOG_BUFFER_TYPE_MEMORY )
        {
            IDE_TEST( sLogFile->readFromDisk(
                            0,
                            (SChar*)(sLogFile->mBase),
                            /* BUG-15532: 윈도우 플랫폼에서 DirectIO를 사용할경우
                             * 서버 시작실패IO크기가 Sector Size로 Align되어야됨 */
                            idlOS::align(sResetLSN.mOffset,
                                         iduProperty::getDirectIOPageSize()))
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do ... */
        }

        sLogFile->clear(sResetLSN.mOffset);

        // Log Buffer를 Clear했기 때문에 Disk에 변경된 영역을 반영시킨다.
        IDE_TEST( sLogFile->syncToDisk( sResetLSN.mOffset,
                                        smuProperty::getLogFileSize() )
                 != IDE_SUCCESS );

        IDE_TEST( smrLogMultiplexThread::syncToDisk( 
                                                smrLogFileMgr::mSyncThread,
                                                sResetLSN.mOffset,         
                                                smuProperty::getLogFileSize(),
                                                sLogFile )
                  != IDE_SUCCESS );

        IDE_TEST( smrLogMgr::closeLogFile(sLogFile) != IDE_SUCCESS );
        sLogFile = NULL;

        IDE_CALLBACK_SEND_SYM("\n     [ SUCCESS ]");
    }
    else
    {
        /* nothing to do ... */
    }

    SM_LSN_MAX(sResetLSN);

    IDE_TEST( mAnchorMgr.updateResetLSN( &sResetLSN ) != IDE_SUCCESS );

    IDE_CALLBACK_SEND_MSG("[SUCCESS]");

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_archivelog_mode);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrArchiveLogMode));
    }
    IDE_EXCEPTION(err_resetlog_no_needed);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidUseResetLog));
    }
    IDE_EXCEPTION_END;

    IDE_CALLBACK_SEND_MSG("[FAILURE]");

    IDE_PUSH();
    {
        if ( sLogFile != NULL )
        {
            IDE_ASSERT( smrLogMgr::closeLogFile(sLogFile) == IDE_SUCCESS );
            sLogFile = NULL;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*
  입력된 Directory에 입력된 logfile 번호부터 그 이후 모든 로그파일을
  삭제한다.

  [ 인자 ]

  {IN] aDirPath - 로그파일이 존재하는 디렉토리
  {IN] aBeginLogFileNo - 삭제할 로그파일 시작번호
  ( 이후 로그파일은 모두 삭제됨 )

*/
IDE_RC smrRecoveryMgr::deleteLogFiles( SChar   * aDirPath,
                                       UInt      aBeginLogFileNo )
{
    UInt               sBeginLogFileNo;
    UInt               sCurLogFileNo;
    UInt               sDeleteCnt;
    SInt               sRc;
    DIR              * sDIR         = NULL;
    struct  dirent   * sDirEnt      = NULL;
    struct  dirent   * sResDirEnt;
    SChar              sLogFileName[SM_MAX_FILE_NAME];
    SChar              sMsgBuf[SM_MAX_FILE_NAME];

    IDE_DASSERT( aDirPath != NULL );

    sBeginLogFileNo = aBeginLogFileNo;
    sDeleteCnt = 1;

    idlOS::memset(sLogFileName, 0x00, SM_MAX_FILE_NAME);
    idlOS::memset(sMsgBuf, 0x00, SM_MAX_FILE_NAME);

    /* smrRecoveryMgr_deleteLogFiles_malloc_DirEnt.tc */
    /* smrRecoveryMgr_deleteLogFiles.tc */
    IDU_FIT_POINT_RAISE( "smrRecoveryMgr::deleteLogFiles::malloc::DirEnt",
                         insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                       ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                                       (void**)&sDirEnt,
                                       IDU_MEM_FORCE) != IDE_SUCCESS,
                    insufficient_memory );

    /* smrRecoveryMgr_deleteLogFiles.tc */
    IDU_FIT_POINT_RAISE( "smrRecoveryMgr::deleteLogFiles::opendir", err_open_dir );
    sDIR = idf::opendir( aDirPath );
    IDE_TEST_RAISE(sDIR == NULL, err_open_dir);

    sResDirEnt = NULL;
    errno  = 0;

    /* smrRecoveryMgr_deleteLogFiles.tc */
    IDU_FIT_POINT_RAISE( "smrRecoveryMgr::deleteLogFiles::readdir_r", err_read_dir );
    sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
    IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
    errno  = 0;

    while(sResDirEnt != NULL)
    {
        idlOS::strcpy(sLogFileName, sResDirEnt->d_name);

        if (idlOS::strncmp(sLogFileName,
                           SMR_LOG_FILE_NAME,
                           idlOS::strlen(SMR_LOG_FILE_NAME)) == 0)
        {
            sCurLogFileNo = idlOS::strlen(SMR_LOG_FILE_NAME);
            sCurLogFileNo = idlOS::atoi(&sLogFileName[sCurLogFileNo]);

            if (sCurLogFileNo >= sBeginLogFileNo)
            {
                if (sCurLogFileNo > sBeginLogFileNo)
                {
                    (((sDeleteCnt) % 5) == 0 ) ?
                        IDE_CALLBACK_SEND_SYM("*") : IDE_CALLBACK_SEND_SYM(".");
                }
                sDeleteCnt++;

                idlOS::snprintf(sLogFileName,
                                SM_MAX_FILE_NAME,
                                "%s%c%s%"ID_UINT32_FMT,
                                aDirPath,
                                IDL_FILE_SEPARATOR,
                                SMR_LOG_FILE_NAME,
                                sCurLogFileNo);

                if ( idf::unlink(sLogFileName) == 0 )
                {
                    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                                 "[ Reset Log : Success ] Deleted ( %s )", sLogFileName );
                }
                else
                {
                    ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                                 "[ Reset Log : Failure ] Cann't delete ( %s )", sLogFileName );
                }
            }
        }

        sResDirEnt = NULL;
        errno = 0;
        sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
        IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }

    idf::closedir(sDIR);
    sDIR = NULL;

    IDE_TEST( iduMemMgr::free( sDirEnt ) != IDE_SUCCESS );
    sDirEnt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotDir));
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, aDirPath ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sDIR != NULL )
        {
            idf::closedir( sDIR );
            sDIR = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sDirEnt != NULL )
        {
            IDE_ASSERT( iduMemMgr::free( sDirEnt ) == IDE_SUCCESS );
            sDirEnt = NULL;
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 중간에 유실된 로그파일이 없는지 검사한다.
 **********************************************************************/
IDE_RC smrRecoveryMgr::identifyLogFiles()
{
    UInt               sChecked;
    UInt               sState       = 0;
    SInt               sRc;
    DIR              * sDIR         = NULL;
    struct  dirent   * sDirEnt      = NULL;
    struct  dirent   * sResDirEnt;
    iduFile            sFile;
    SChar              sLogFileName[SM_MAX_FILE_NAME];
    UInt               sCurLogFileNo;
    UInt               sTotalLogFileCnt;
    UInt               sMinLogFileNo;
    UInt               sNoPos;
    UInt               sLstDeleteLogFileNo;
    UInt               sPrefixLen;
    ULong              sFileSize    = 0;
    idBool             sIsLogFile;
    idBool             sDeleteFlag;
    ULong              sLogFileSize = smuProperty::getLogFileSize();

    /* BUG-48409 prepare 완료되지 않은 logfile 을 정리하는 방법 개선
     * prepare 중이던 임시 로그 파일 삭제 */
    idlOS::snprintf( sLogFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s%",
                     smuProperty::getLogDirPath(),
                     IDL_FILE_SEPARATOR,
                     SMR_TEMP_LOG_FILE_NAME );
      
    if ( idf::access(sLogFileName, F_OK) == 0 )
    {
        (void)idf::unlink(sLogFileName);
    }

    /* smrRecoveryMgr_identifyLogFiles_malloc_DirEnt.tc */
    /* smrRecoveryMgr_identifyLogFiles.tc */
    IDU_FIT_POINT_RAISE( "smrRecoveryMgr::identifyLogFiles::malloc::DirEnt",
                         insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                       ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                                       (void**)&sDirEnt,
                                       IDU_MEM_FORCE ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    ideLog::log(IDE_SERVER_0,"  \tchecking logfile(s)\n");

    sLstDeleteLogFileNo = mAnchorMgr.getLstDeleteLogFileNo();

    /* smrRecoveryMgr_identifyLogFiles.tc */
    IDU_FIT_POINT_RAISE( "smrRecoveryMgr::identifyLogFiles::opendir", err_open_dir );
    sDIR = idf::opendir( smuProperty::getLogDirPath() );
    IDE_TEST_RAISE(sDIR == NULL, err_open_dir);

    sResDirEnt = NULL;
    errno = 0;

    /* smrRecoveryMgr_identifyLogFiles.tc */
    IDU_FIT_POINT_RAISE( "smrRecoveryMgr::identifyLogFiles::readdir_r", err_read_dir );
    sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
    IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
    errno = 0;

    sMinLogFileNo = ID_UINT_MAX;
    sTotalLogFileCnt = 0;
    sPrefixLen = idlOS::strlen(SMR_LOG_FILE_NAME);

    // BUG-20229
    sDeleteFlag = ID_FALSE;

    while ( sResDirEnt != NULL )
    {
        sNoPos = sPrefixLen;
        idlOS::strcpy(sLogFileName, sResDirEnt->d_name);

        if ( idlOS::strncmp(sLogFileName,
                            SMR_LOG_FILE_NAME,
                            sPrefixLen) == 0 )
        {
            sCurLogFileNo = idlOS::atoi(&sLogFileName[sNoPos]);

            sIsLogFile = ID_TRUE;

            while ( sLogFileName[sNoPos] != '\0' )
            {
                if ( smuUtility::isDigit(sLogFileName[sNoPos])
                     == ID_FALSE )
                {
                    sIsLogFile = ID_FALSE;
                    break;
                }
                else
                {
                    /* nothing to do ... */
                }
                sNoPos++;
            }

            if ( sIsLogFile == ID_TRUE )
            {
                // it is logfile.
                if ( sLstDeleteLogFileNo <= sCurLogFileNo )
                {
                    if (sCurLogFileNo < sMinLogFileNo)
                    {
                        sMinLogFileNo = sCurLogFileNo;
                    }
                    else
                    {
                        /* nothing to do ... */
                    }
                    sTotalLogFileCnt++;
                }
                else
                {
                    /* nothing to do ... */
                }
            }
            else
            {
                /* nothing to do ... */
            }
        }
        else
        {
            /* nothing to do ... */
        }

        sResDirEnt = NULL;
        errno = 0;
        sRc = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
        IDE_TEST_RAISE( (sRc != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }

    idf::closedir(sDIR);
    sDIR = NULL;

    for (sCurLogFileNo = sMinLogFileNo, sChecked = 0;
         sChecked < sTotalLogFileCnt;
         sCurLogFileNo++, sChecked++)
    {
        idlOS::snprintf( sLogFileName,
                         SM_MAX_FILE_NAME,
                         "%s%c%s%"ID_UINT32_FMT,
                         smuProperty::getLogDirPath(),
                         IDL_FILE_SEPARATOR,
                         SMR_LOG_FILE_NAME,
                         sCurLogFileNo );

        /* PROJ-2162 RestartRiskReduction
         * LogFile이 연속적이지 않을때, 바로 서버를 종료하는 대신 이를
         * 체크해둔다.
         * 또한  이 이후의 LogFile을 지워서도 안된다. */
        if ( idf::access(sLogFileName, F_OK) != 0 )
        {
            mLogFileContinuity = ID_FALSE;
            idlOS::strncpy( mLostLogFile,
                            sLogFileName,
                            SM_MAX_FILE_NAME );
            continue;
        }
        else
        {
            /* nothing to do ... */
        }

        IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                   1, /* Max Open FD Count */
                                   IDU_FIO_STAT_OFF,
                                   IDV_WAIT_INDEX_NULL )
                 != IDE_SUCCESS );
        sState = 2;
        IDE_TEST( sFile.setFileName(sLogFileName) != IDE_SUCCESS );
        IDE_TEST( sFile.open() != IDE_SUCCESS );
        sState = 3;

        IDE_TEST( sFile.getFileSize( &sFileSize ) != IDE_SUCCESS );

        sState = 2;
        IDE_TEST( sFile.close() != IDE_SUCCESS );

        sState = 1;
        IDE_TEST( sFile.destroy() != IDE_SUCCESS );
        
        // BUG-20229
        // sDeleteFlag가 ID_TRUE라는 것은
        // 이전 로그파일중에 size가 0인 로그파일이 있었다는 뜻이다.
        // size가 0인 로그파일을 발견했다면
        // 그 이후의 로그파일을 모두 지운다.
        if ( sDeleteFlag == ID_TRUE )
        {
            ideLog::log( IDE_ERR_0,
                         "Remove logfile : %s (%"ID_UINT32_FMT")\n" ,
                         sLogFileName,
                         sFileSize );

            (void)idf::unlink(sLogFileName);
            continue;
        }
        else
        {
            /* nothing to do ... */
        }

        IDE_ASSERT( sFileSize <= sLogFileSize );

        // BUG-20229
        // size가 0인 로그파일을 발견했다면
        // 그 이후의 로그파일을 모두 지운다.
        // prepare logfile시에는 sync를 하지 않기 때문에,
        // 갑작스런 전원 장애로 인하여 시스템이 reboot되는 경우,
        // prepare 중이던 logfile의 size가 0 일 수 있다.
        if ( sFileSize == 0 )
        {
            if ( smuProperty::getZeroSizeLogFileAutoDelete() == 0 )
            {
                /* BUG-42930 OS가 잘못된 size를 반환할수 있습니다.
                 * 로그파일을 무조건 지우는 대신
                 * DBA가 처리할수 있도록 Error 메시지를 출력하도록 하였습니다.
                 * Server Kill 테스트 등에서 Log File Size가 0이 될수 있습니다.
                 * Test용 Property를 두어 natc 도중에는 자동제거 될수 있게 하였습니다. */
                ideLog::log( IDE_ERR_0, "Error zero size logfile : %s\n" , sLogFileName );
                IDE_RAISE( LOG_FILE_SIZE_ZERO );
            }
            else
            {
                ideLog::log( IDE_ERR_0, "Remove zero size logfile : %s\n" , sLogFileName );
                (void)idf::unlink(sLogFileName);
                sDeleteFlag = ID_TRUE;
            }
        }
        else
        {
            if ( sFileSize != sLogFileSize )
            {
                /* BUG-48409 prepare 완료되지 않은 logfile 을 정리하는 방법 개선
                 * use temp logfile에서는 prepare 중에 size가 작은 logfile 이 남지 않는다. */
                if ( smuProperty::getUseTempForPrepareLogFile() == ID_TRUE )
                {
                    ideLog::log( IDE_ERR_0,
                                 "Invalid logfile size : %s (%"ID_UINT32_FMT")\n" ,
                                 sLogFileName,
                                 sFileSize );
                    IDE_RAISE( LOG_FILE_SIZE_ZERO );
                }
                else
                {
                    ideLog::log( IDE_ERR_0,
                                 "Resize logfile : %s (%"ID_UINT32_FMT" -> %"ID_UINT32_FMT")\n" ,
                                 sLogFileName,
                                 sFileSize,
                                 sLogFileSize );
                    // BUG-20229
                    // 로그파일의 크기를 정상적인 로그파일 크기로 늘인다.
                    IDE_TEST( resizeLogFile(sLogFileName,
                                            sLogFileSize)
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* nothing to do ... */
            }
        }
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sDirEnt ) != IDE_SUCCESS );
    sDirEnt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotDir));
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, smuProperty::getLogDirPath() ) );
    }
    IDE_EXCEPTION( LOG_FILE_SIZE_ZERO );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_LogFileSizeIsZero, sLogFileName));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch(sState)
    {
        case 3:
            IDE_ASSERT(sFile.close() == IDE_SUCCESS );

        case 2:
            IDE_ASSERT(sFile.destroy() == IDE_SUCCESS );

        case 1:
            IDE_ASSERT(iduMemMgr::free(sDirEnt) == IDE_SUCCESS );
            sDirEnt = NULL;
            break;

        default:
            break;
    }

    if ( sDIR != NULL )
    {
        idf::closedir( sDIR );
        sDIR = NULL;
    }
    else
    {
        /* Nothing to do */
    }
    IDE_POP();

    return IDE_FAILURE;

}

/*
  incomplete 미디어복구를 수행하여 reset logs를 설정한 경우
  startup meta resetlogs 인지를 검사한다.
*/
IDE_RC smrRecoveryMgr::checkResetLogLSN( UInt aActionFlag )
{
    smLSN               sResetLSN;

    mAnchorMgr.getResetLogs( &sResetLSN );

    if ( (sResetLSN.mFileNo != ID_UINT_MAX) &&
         (sResetLSN.mOffset != ID_UINT_MAX)) 
    {
        IDE_TEST_RAISE( (aActionFlag & SMI_STARTUP_ACTION_MASK) !=
                        SMI_STARTUP_RESETLOGS,
                        err_need_resetlogs);
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_need_resetlogs);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NeedResetLogs));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   PRJ-1149 모든 테이블스페이스의 미디어 오류 검사

   [IN] aActionFlag - 다단계 서버구동과정에서의 플래그
*/
IDE_RC smrRecoveryMgr::identifyDatabase( UInt  aActionFlag )
{
    ideLog::log( IDE_SERVER_0,
                 "  \tchecking file(s) of all memory tablespace\n" );

    // [1] 모든 메모리 테이블스페이스의 Stable한 DBFile들은 반드시
    // 있어야 하고 (loganchor에 저장되어있는한) Unstable DBFiles들은
    // 존재한다면 최소한 유효한 버전이어야 한다.
    // 미디어 복구 필요 여부를 체크한다.
    // 서버구동시에는 ID_FALSE로 넘긴다.
    IDE_TEST( smmTBSMediaRecovery::identifyDBFilesOfAllTBS( ID_FALSE )
              != IDE_SUCCESS );

    ideLog::log( IDE_SERVER_0,
                 "  \tchecking file(s) of all disk tablespace\n" );

    // [2] 모든 디스크 테이블스페이스의 DBFile들의
    // 미디어 복구 필요 여부를 체크한다.
    // 서버구동시에는 ID_FALSE로 넘긴다.
    IDE_TEST( sddDiskMgr::identifyDBFilesOfAllTBS( NULL, /* idvSQL* */
                                                   ID_FALSE )
              != IDE_SUCCESS );

    // [3] 불완전복구를 수행하였다면 alter databae mydb meta resetlogs;
    // 를 수행하였는지 검사한다.
    IDE_TEST( checkResetLogLSN( aActionFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  모든 테이블스페이스를 검사하여 복구대상 파일리스트를 구축한다.

  [IN]  aRecoveryType         - 미디어 복구 타입 (RESTART RECOVERY 제외)
  [OUT] aFailureMediaType     - 미디어 오류 타입
  [OUT] aFromDiskRedoLSN      - 미디어 복구할 시작 Disk RedoLSN
  [OUT] aToDiskRedoLSN        - 미디어 복구할 완료 Disk RedoLSN
  [OUT] aFromMemRedoLSN    - 미디어 복구할 시작 Memory RedoLSN 
  [OUT] aToMemRedoLSN      - 미디어 복구할 완료 Memory RedoLSN 
*/
IDE_RC smrRecoveryMgr::makeMediaRecoveryDBFList4AllTBS(
    smiRecoverType    aRecoveryType,
    UInt            * aFailureMediaType,
    smLSN           * aFromDiskRedoLSN,
    smLSN           * aToDiskRedoLSN,
    smLSN           * aFromMemRedoLSN,
    smLSN           * aToMemRedoLSN )
{
    UInt                sFailureChkptImgCount;
    UInt                sFailureDBFCount;
    sctTableSpaceNode*  sCurrSpaceNode;
    sctTableSpaceNode*  sNextSpaceNode;
    SChar               sMsgBuf[ SM_MAX_FILE_NAME ];
    smLSN               sFromDiskRedoLSN;
    smLSN               sToDiskRedoLSN;
    smLSN               sFromMemRedoLSN;
    smLSN               sToMemRedoLSN;

    IDE_DASSERT( aRecoveryType         != SMI_RECOVER_RESTART );
    IDE_DASSERT( aFailureMediaType     != NULL );
    IDE_DASSERT( aToDiskRedoLSN        != NULL );
    IDE_DASSERT( aFromDiskRedoLSN      != NULL );
    IDE_DASSERT( aToMemRedoLSN         != NULL );
    IDE_DASSERT( aFromMemRedoLSN       != NULL );

    SM_LSN_MAX ( sFromDiskRedoLSN );
    SM_LSN_INIT( sToDiskRedoLSN );

    *aFailureMediaType    = 0;

    // BUG-27616 Klocwork SM (7)

    SM_LSN_MAX ( sFromMemRedoLSN );
    SM_LSN_INIT( sToMemRedoLSN );

    sFailureChkptImgCount = 0;
    sFailureDBFCount      = 0;

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while ( sCurrSpaceNode != NULL )
    {
        sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );

        IDE_ASSERT( (sCurrSpaceNode->mState & SMI_TBS_DROPPED)
                    != SMI_TBS_DROPPED );

        if ( sctTableSpaceMgr::isTempTableSpace( sCurrSpaceNode ) == ID_TRUE )
        {
            // 임시 테이블스페이스의 경우 체크하지 않는다.
            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }

        if ( sctTableSpaceMgr::hasState( sCurrSpaceNode,
                                         SCT_SS_UNABLE_MEDIA_RECOVERY )
             == ID_TRUE )
        {
            // 미디어복구가 필요없는 테이블스페이스
            // DROPPED 이거나 DISCARD 인 경우
            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }
        switch( sctTableSpaceMgr::getTBSLocation( sCurrSpaceNode ) )
        {
            case SMI_TBS_MEMORY:
                // 메모리테이블스페이스의 경우
                IDE_TEST( smmTBSMediaRecovery::makeMediaRecoveryDBFList(
                              sCurrSpaceNode,
                              aRecoveryType,
                              &sFailureChkptImgCount,
                              &sFromMemRedoLSN,
                              &sToMemRedoLSN )
                          != IDE_SUCCESS );
                break;
            case SMI_TBS_DISK:
                IDE_TEST( sddDiskMgr::makeMediaRecoveryDBFList( NULL, /* idvSQL* */
                                                                sCurrSpaceNode,
                                                                aRecoveryType,
                                                                &sFailureDBFCount,
                                                                &sFromDiskRedoLSN,
                                                                &sToDiskRedoLSN )
                          != IDE_SUCCESS );
                break;
            case SMI_TBS_VOLATILE:
                // Nothing to do...
                // volatile tablespace에 대해서는 아무 작업하지 않는다.

                break;
            default:
                IDE_ASSERT(0);
                break;
        }

        sCurrSpaceNode = sNextSpaceNode;
    }

    if ( (sFailureChkptImgCount + sFailureDBFCount) > 0 )
    {
        /*
           복구대상 파일이 존재하면 분석된 재수행구간을
           재수행하여 미디어복구를 완료한다.
        */

        // 무조건 초기화한다.
        SM_GET_LSN( *aFromDiskRedoLSN, sFromDiskRedoLSN );
        SM_GET_LSN( *aToDiskRedoLSN, sToDiskRedoLSN );

        SM_GET_LSN( *aFromMemRedoLSN, sFromMemRedoLSN );
        SM_GET_LSN( *aToMemRedoLSN, sToMemRedoLSN );

        if ( sFailureChkptImgCount > 0)
        {
            *aFailureMediaType |= SMR_FAILURE_MEDIA_MRDB;

            idlOS::snprintf(sMsgBuf,
                            SM_MAX_FILE_NAME,
                            "             Memory Redo LSN <%"ID_UINT32_FMT",%"ID_UINT32_FMT"> - <%"ID_UINT32_FMT",%"ID_UINT32_FMT">",
                            aFromMemRedoLSN->mFileNo,
                            aFromMemRedoLSN->mOffset,
                            aToMemRedoLSN  ->mFileNo,
                            aToMemRedoLSN  ->mOffset );

            IDE_CALLBACK_SEND_MSG( sMsgBuf );
        }

        if ( sFailureDBFCount > 0 )
        {
            *aFailureMediaType |= SMR_FAILURE_MEDIA_DRDB;

            idlOS::snprintf(sMsgBuf,
                            SM_MAX_FILE_NAME,
                            "             Disk Redo LSN <%"ID_UINT32_FMT",%"ID_UINT32_FMT"> - <%"ID_UINT32_FMT",%"ID_UINT32_FMT">",
                            aFromDiskRedoLSN->mFileNo,
                            aFromDiskRedoLSN->mOffset,
                            aToDiskRedoLSN->mFileNo,
                            aToDiskRedoLSN->mOffset );

            IDE_CALLBACK_SEND_MSG( sMsgBuf );
        }

        idlOS::snprintf( sMsgBuf,
                         SM_MAX_FILE_NAME,
                         "  \t     Total %"ID_UINT32_FMT" database file(s)\n  \t     Memory %"ID_UINT32_FMT" Checkpoint Image(s)\n  \t     Disk %"ID_UINT32_FMT" Database File(s)",
                         sFailureChkptImgCount + sFailureDBFCount,
                         sFailureChkptImgCount,
                         sFailureDBFCount );

        IDE_CALLBACK_SEND_MSG(sMsgBuf);
    }
    else
    {
        // 불완전 복구는 모든 데이타파일이 복구대상이다.
        // 만약 사용자가 불완전 복구를 수행하게 되면, 모든 데이타파일이
        // 오류가 없더라도 복구대상이 되기 때문에 else로 들어올수 없다.
        IDE_ASSERT( !((aRecoveryType == SMI_RECOVER_UNTILTIME  ) ||
                      (aRecoveryType == SMI_RECOVER_UNTILCANCEL)) );

        // 미디어 오류가 없는 상황에서 완전복구를 수행하려고 할때
        // 이곳으로 들어온다.

        // 미디어오류난 데이타파일이 없이 완전복구를 수행한 경우 ERROR 처리
        // 불완전 복구 또는 RESTART RECOVERY를 수행해야 한다.
        IDE_TEST_RAISE( aRecoveryType == SMI_RECOVER_COMPLETE,
                        err_media_recovery_type );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_media_recovery_type );
    {
        IDE_SET( ideSetErrorCode(
                     smERR_ABORT_ERROR_MEDIA_RECOVERY_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : DBFileHdr에서 가장 큰 MustRedoToLSN을 가져온다.
 *  DBFile 이 Backup되었던 파일이 아닐 경우 InitLSN을 넘겨준다.
 *
 * aMustRedoToLSN - [OUT] Disk DBFile 중 가장 큰 MustRedoToLSN
 * aDBFileName    - [OUT] 해당 Must Redo to LSN을 가진 DBFile
 **********************************************************************/
IDE_RC smrRecoveryMgr::getMaxMustRedoToLSN( idvSQL   * aStatistics,
                                            smLSN    * aMustRedoToLSN,
                                            SChar   ** aDBFileName )
{
    sctTableSpaceNode*  sCurrSpaceNode;
    sctTableSpaceNode*  sNextSpaceNode;
    smLSN               sMaxMustRedoToLSN;
    smLSN               sMustRedoToLSN;
    SChar*              sDBFileName;
    SChar*              sMaxRedoLSNFileName = NULL;

    IDE_DASSERT( aMustRedoToLSN != NULL );
    IDE_DASSERT( aDBFileName    != NULL );

    *aDBFileName = NULL;
    SM_LSN_INIT( sMaxMustRedoToLSN );

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while ( sCurrSpaceNode != NULL )
    {
        sNextSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );

        if ( sctTableSpaceMgr::isDiskTableSpace( sCurrSpaceNode ) != ID_TRUE )
        {
            // 디스크테이블스페이스의 경우만 확인한다.
            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }

        if ( sctTableSpaceMgr::isTempTableSpace( sCurrSpaceNode ) == ID_TRUE )
        {
            // 임시 테이블스페이스의 경우 체크하지 않는다.
            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }

        if ( sctTableSpaceMgr::hasState( sCurrSpaceNode,
                                         SCT_SS_UNABLE_MEDIA_RECOVERY )
             == ID_TRUE )
        {
            // 미디어복구가 필요없는 테이블스페이스
            // DROPPED 이거나 DISCARD 인 경우
            sCurrSpaceNode = sNextSpaceNode;
            continue;
        }

        IDE_TEST( sddDiskMgr::getMustRedoToLSN(
                                  aStatistics,
                                  sCurrSpaceNode,
                                  &sMustRedoToLSN,
                                  &sDBFileName )
                  != IDE_SUCCESS );

        if ( smrCompareLSN::isGT( &sMustRedoToLSN,
                                 &sMaxMustRedoToLSN ) == ID_TRUE )
        {
            sMaxMustRedoToLSN   = sMustRedoToLSN;
            sMaxRedoLSNFileName = sDBFileName;
        }

        sCurrSpaceNode = sNextSpaceNode;
    }

    *aMustRedoToLSN = sMaxMustRedoToLSN;
    *aDBFileName    = sMaxRedoLSNFileName;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   미디어 복구시 판독할 Log의 Scan 할 구간을 얻는다.

   [IN]  aFromDiskRedoLSN   - Disk 복구구간 최소 LSN
   [IN]  aToDiskRedoLSN     - Disk 복구구간 최대 LSN
   [IN]  aFromMemRedoLSN - Memory 복구구간 최소 LSN
   [IN]  aToMemRedoLSN   - Memory 복구구간 최대 LSN
   [OUT] aMinFromRedoLSN - 미디어 복구시 scan할 로그의 최소 LSN
*/
void smrRecoveryMgr::getRedoLSN4SCAN( smLSN * aFromDiskRedoLSN,
                                      smLSN * aToDiskRedoLSN,
                                      smLSN * aFromMemRedoLSN,
                                      smLSN * aToMemRedoLSN,
                                      smLSN * aMinFromRedoLSN )
{
    smLSN       sInitLSN;
    smLSN       sRestartRedoLSN;

    IDE_ASSERT( aFromDiskRedoLSN   != NULL );
    IDE_ASSERT( aToDiskRedoLSN     != NULL );
    IDE_ASSERT( aFromMemRedoLSN != NULL );
    IDE_ASSERT( aToMemRedoLSN   != NULL );
    IDE_ASSERT( aMinFromRedoLSN != NULL );

    // Disk DBF에만 Media Recovery 수행되어야 하는 경우,
    // Memory의 From, To RedoLSN은 값이 설정되지 않은 상태이다.
    //
    // 마찬가지로, Memory Checkpoint Image에만
    // Media Recovery가 수행되어야 하는 경우 Disk의 From, To RedoLSN은
    // 설정되지 않은 상태이다.
    //
    // => 모든 LFG에 대해 From, To RedoLSN이 설정되지 않은 경우
    //    Log Anchor상의 Redo시작 시점으로 설정한다.
    //
    mAnchorMgr.getEndLSN( &sRestartRedoLSN );

    if ( SM_IS_LSN_MAX( *aFromMemRedoLSN ) )
    {
        SM_GET_LSN( *aFromMemRedoLSN,
                    sRestartRedoLSN );
    }
    else
    {
        /* nothing to do ... */
    }

    if ( SM_IS_LSN_INIT( *aToMemRedoLSN ) )
    {
        SM_GET_LSN( *aToMemRedoLSN,
                    sRestartRedoLSN );
    }
    else
    {
        /* nothing to do ... */
    }

    if ( SM_IS_LSN_MAX( *aFromDiskRedoLSN ) )
    {
        SM_GET_LSN( *aFromDiskRedoLSN,
                    sRestartRedoLSN );
    }
    /*
      getRedoLogLSNRange 구하기

      *[IN ]From Disk Redo LSN
      LFG 0  10

      *[IN ]Array of From Memory Redo LSN
      LFG 0         20
      LFG 1     15

      *[OUT]Array of Mininum From Redo LSN
      LFG 0  10
      LFG 1     15

      위의 그림과 같이 DISK LFG 에 대해서만 비교를 다시하여
      Log Scan 영역을 결정한다.
      일단 Memory 의 From Redo LSN의 기반하에서 Disk 의 FromRedoLSN을
      비교하여 LFG 0번에 대해서만 다시 구한다.

    */
    SM_GET_LSN( *aMinFromRedoLSN, *aFromMemRedoLSN );

    // 디스크 LFG에 대한 재조정
    if ( smrCompareLSN::isGT( aFromMemRedoLSN, aFromDiskRedoLSN ) == ID_TRUE )
    {
        // LFG 0 > Disk Redo LSN
        // set minimum Redo LSN
        SM_GET_LSN( *aMinFromRedoLSN,
                    *aFromDiskRedoLSN );
    }

    SM_LSN_INIT( sInitLSN );

    IDE_ASSERT( !( (smrCompareLSN::isEQ( aFromMemRedoLSN,
                                        &sInitLSN ) == ID_TRUE) &&
                   (smrCompareLSN::isEQ( aToMemRedoLSN,
                                        &sInitLSN) == ID_TRUE)) );

    IDE_ASSERT( smrCompareLSN::isEQ( aMinFromRedoLSN,
                                     &sInitLSN )
                == ID_FALSE );

    return;
}


/*
   PRJ-1149 SM - Backup
   PRJ-1548 User Memory Tablespace 개념도입

   데이타베이스의 미디오 복구 수행

   - 현재 시점까지 media recovery 할때는 aUntilTime이 ULong max로 넘어온다.

   - 과거의 특정시점으로 DB으로 돌릴때는
   date를 idlOS::time(0)으로 변환한 값이 넘어온다.
   alter database recover database until time '2005-01-29:17:55:00'

   복사해온 이전 데이타파일의 OldestLSN부터
   최근 데이타파일의 OldestLSN(by anchor)까지 데이타파일에
   대한 Redo 로그들을 선택적으로 반영하고, 로그앵커와 데이타
   파일의 파일헤더정보를 동기화시킨다.
   입력되는 데이타파일이 다수개가 가능하므로 각각 데이타파일의
   redo 범위를 파악하고,  online 로그와 archive 로그의 정보를 고려하여
   redo를 진행한다.

   [ 알고리즘 ]
   - 재수행관리자를 초기화한다.
   - 입력된 파일이 존재하는지 검사한다.
   - 복구할 파일(들)을 재수행 관리자의 RecvFileHash에 추가한다.
   - 재수행관리자를 해제한다.
   - 로그파일들을 모두 닫는다.

   [IN] aRecoveryType - 완전복구 또는 불완전복구 타입
   [IN] aUntilTIME    - 타임베이스 불완전 복구시에 시간인자

*/
IDE_RC smrRecoveryMgr::recoverDB( idvSQL*           aStatistics,
                                  smiRecoverType    aRecoveryType,
                                  UInt              aUntilTIME )
{

    UInt                sDiskState;
    UInt                sMemState;
    UInt                sCommonState;
    SChar               sMsgBuf[ SM_MAX_FILE_NAME ];
    time_t              sUntilTIME;
    struct tm           sUntil;
    smLSN               sCurRedoLSN;
    UInt                sFailureMediaType;

    smLSN               sMinFromRedoLSN;
    smLSN               sMaxToRedoLSN;

    smLSN               sToDiskRedoLSN;
    smLSN               sFromDiskRedoLSN;
    smLSN               sFromMemRedoLSN;
    smLSN               sToMemRedoLSN;

    smLSN               sResetLSN;
    smLSN              *sMemResetLSNPtr  = NULL;
    smLSN              *sDiskResetLSNPtr = NULL;

    IDE_DASSERT( aRecoveryType != SMI_RECOVER_RESTART );

    /* BUG-18428: Media Recovery중에 Index에 대한 Undo연산을 수행하면 안됩니다.
     *
     * Media Recovery는 Restart Recovery로직을 사용하기때문에
     * Redo시에 Temporal 정보에 대한 접근을 막아야 한다. */
    mRestart = ID_TRUE;
    mMediaRecoveryPhase = ID_TRUE; /* 미디어 복구 수행중에만 TRUE */

    // fix BUG-17158
    // offline DBF에 write가 가능하도록 flag 를 설정한다.
    sddDiskMgr::setEnableWriteToOfflineDBF( ID_TRUE );

    sCommonState = 0;
    sDiskState   = 0;
    sMemState    = 0;

    sFailureMediaType = SMR_FAILURE_MEDIA_NONE;
    idlOS::memset( sMsgBuf, 0x00, SM_MAX_FILE_NAME );

    // 미디어복구는 아카이브로그 모드에서만 지원된다.
    IDE_TEST_RAISE( getArchiveMode() != SMI_LOG_ARCHIVE,
                    err_archivelog_mode );

    // BUG-29633 Recovery전 모든 Transaction이 End상태인지 점검이 필요합니다.
    // Recovery시에 다른곳에서 사용되는 Active Transaction이 존재해서는 안된다.
    IDE_TEST_RAISE( smLayerCallback::existActiveTrans() == ID_TRUE,
                    err_exist_active_trans);

    // Secondary  Buffer 를 사용하면 안된다. 
    IDE_TEST_RAISE( sdsBufferMgr::isServiceable() == ID_TRUE,
                    err_secondary_buffer_service );

    // 불완전 복구가 이미 완료되었는데 다시 미디어 복구를 실행하는
    // 경우를 확인하여 에러를 반환한다.
    IDE_TEST( checkResetLogLSN( SMI_STARTUP_NOACTION )
              != IDE_SUCCESS );

    // [DISK-1] 디스크 redo 관리자 초기화
    IDE_TEST( smLayerCallback::initializeRedoMgr(
                  sdbBufferMgr::getPageCount(),
                  aRecoveryType )
              != IDE_SUCCESS );
    sDiskState = 1;

    IDE_CALLBACK_SEND_MSG("  [ RECMGR ] Checking database consistency..");

    switch ( aRecoveryType )
    {
        case SMI_RECOVER_COMPLETE:
            {
                // 완전복구시 시
                // 로그디렉토리에 로그파일들의 유효성을 검사한다.
                IDE_TEST( identifyLogFiles() != IDE_SUCCESS );
                // 완전복구일경우 로그파일들의 유효성에서 실패하면 restart recovery 에서 에러를 출력하거나 
                // 불완전 복구를 유도 한다. 
                break;
            }
        case SMI_RECOVER_UNTILTIME:
            {
                // 타임베이스 불완전 복구시
                // 로그디렉토리에 로그파일들의 유효성을 검사한다.
                IDE_TEST( identifyLogFiles() != IDE_SUCCESS );

                // 로그가 연속적이지 않을경우 UNTIL TIME 이 의도한대로 동작하지 않을수있다. 
                // recovery 해서 로그 날리기 전에 에러를 발생시킨다. 
                IDE_TEST_RAISE( mLogFileContinuity == ID_FALSE, err_log_consistency );
                break;
            }
        case SMI_RECOVER_UNTILCANCEL:
            {
                // 로그베이스 불완전 복구시에는 로그파일이 연속적으로
                // 존재하지 않을 수 있기 때문에 검증하는 것이 불필요하다.
                break;
            }
        default :
            {
                IDE_ASSERT( 0 );
                break;
            }
    }

    ideLog::log( IDE_SERVER_0,
                 "  \tchecking inconsistency database file(s) count..");

    /*
       [1] 모든 테이블스페이스의 복구할 대상 파일들을 구한다.
       디스크 데이타파일은 재수행관리자의 2차해쉬로
       구축한다.
    */
    IDE_TEST( makeMediaRecoveryDBFList4AllTBS( aRecoveryType,
                                               &sFailureMediaType,
                                               &sFromDiskRedoLSN,
                                               &sToDiskRedoLSN,
                                               &sFromMemRedoLSN,
                                               &sToMemRedoLSN )
              != IDE_SUCCESS );

    // MEDIA_NONE 인 경우는 makeMediaRecoveryDBFList4AllTBS에서
    // exception 처리된다.
    IDE_ASSERT( sFailureMediaType != SMR_FAILURE_MEDIA_NONE );

    /*
       [2] 메모리/디스크 전체를 복구할 수 있는 재수행시작 LSN을 결정한다.
    */

    // [ 중요 ]
    // From Redo LSN 보정하기
    // 보정되지 않은 Redo LSN이 파일헤더에 저장되어 있어
    // 미디어 복구시 From Redo LSN을 보정시켜 주어야 한다.
    // 왜냐하면, 재수행시 중간 LSN이 누락되면 데이타베이스
    // 복구가 제대로 되지 않을 수 있다.

    // get minimum RedoLSN
    getRedoLSN4SCAN( &sFromDiskRedoLSN,
                     &sToDiskRedoLSN,
                     &sFromMemRedoLSN,
                     &sToMemRedoLSN,
                     &sMinFromRedoLSN );

    if ( (sFailureMediaType & SMR_FAILURE_MEDIA_MRDB)
         == SMR_FAILURE_MEDIA_MRDB )
    {
        // [MEMORY-1]
        // 메모리 테이블스페이스의 복구를 수행하기전
        // 복구 대상 테이블스페이스를 Prepare/Restore를 수행한다.
        IDE_TEST( initMediaRecovery4MemTBS() != IDE_SUCCESS );
        sMemState = 1;
    }

    /*
       [3] 메모리/디스크 미디어 복구 수행
    */
    IDE_CALLBACK_SEND_MSG(
        "  [ RECMGR ] Restoring database consistency");

    /*
       !! media recovery 이후에는 server restart recovery를 수행하여
       서버가 구동되어야만 한다.
       상태만 변경한다.
    */
    IDE_TEST( mAnchorMgr.updateSVRStateAndFlush( SMR_SERVER_STARTED )
              != IDE_SUCCESS );

    idlOS::snprintf(sMsgBuf, SM_MAX_FILE_NAME,
                    "             Suggestion Range Of LSN : ");

    IDE_CALLBACK_SEND_MSG( sMsgBuf );

    idlOS::snprintf(sMsgBuf, SM_MAX_FILE_NAME,
                    "              From LSN <%"ID_UINT32_FMT",%"ID_UINT32_FMT"> ~ ",
                    sMinFromRedoLSN.mFileNo,
                    sMinFromRedoLSN.mOffset );

    IDE_CALLBACK_SEND_MSG( sMsgBuf );

    switch ( aRecoveryType )
    {
        case SMI_RECOVER_COMPLETE:
            {
                SM_GET_LSN( sMaxToRedoLSN, sToMemRedoLSN );

                if ( smrCompareLSN::isLT(
                         &sToMemRedoLSN,
                         &sToDiskRedoLSN ) == ID_TRUE )
                {
                    // Disk To Redo LSN이 더 크다면
                    SM_GET_LSN( sMaxToRedoLSN, sToDiskRedoLSN );
                }
                else
                {
                    /* nothing to do ... */
                }

                idlOS::snprintf(sMsgBuf,
                                SM_MAX_FILE_NAME,
                                "             To   LSN <%"ID_UINT32_FMT",%"ID_UINT32_FMT">",
                                sMaxToRedoLSN.mFileNo,
                                sMaxToRedoLSN.mOffset);

                IDE_CALLBACK_SEND_MSG( sMsgBuf );
                break;
            }
        case SMI_RECOVER_UNTILCANCEL:
            {
                idlOS::snprintf(sMsgBuf,
                                SM_MAX_FILE_NAME,
                                "                                       Until CANCEL" );

                IDE_CALLBACK_SEND_MSG( sMsgBuf );
                break;
            }
        case SMI_RECOVER_UNTILTIME:
            {
                sUntilTIME = (time_t)aUntilTIME;

                idlOS::localtime_r((time_t*)&sUntilTIME, &sUntil);

                idlOS::snprintf(sMsgBuf,
                                SM_MAX_FILE_NAME,
                                "                        Until Time "
                                "[%04"ID_UINT32_FMT
                                "/%02"ID_UINT32_FMT
                                "/%02"ID_UINT32_FMT
                                " %02"ID_UINT32_FMT
                                ":%02"ID_UINT32_FMT
                                ":%02"ID_UINT32_FMT" ]",
                                sUntil.tm_year + 1900,
                                sUntil.tm_mon + 1,
                                sUntil.tm_mday,
                                sUntil.tm_hour,
                                sUntil.tm_min,
                                sUntil.tm_sec);

                IDE_CALLBACK_SEND_MSG( sMsgBuf );
                break;
            }
        default:
            {
                IDE_ASSERT( 0 );
                break;
            }
    }


    // [COMMON-1]
    // 결정된 복구구간의 로그레코드를 판독하기 위한 RedoLSN Manager 초기화
    IDE_TEST( smrRedoLSNMgr::initialize( &sMinFromRedoLSN )
              != IDE_SUCCESS );
    sCommonState = 1;

    // flush를 하기 위해 sdbFlushMgr를 초기화한다.
    // 지금은 control 단계이지만 meria recovery를 위해 flush manager가 필요하다.
    IDE_TEST( sdbFlushMgr::initialize( smuProperty::getBufferFlusherCnt() )
              != IDE_SUCCESS );
    sDiskState = 2;

    idlOS::snprintf( sMsgBuf, SM_MAX_FILE_NAME,
                     "             Applying " );

    IDE_CALLBACK_SEND_SYM( sMsgBuf );

    // To Fix PR-13786, PR-14560, PR-14660
    IDE_TEST( recoverAllFailureTBS( aRecoveryType,
                                    sFailureMediaType,
                                    &sUntilTIME,
                                    &sCurRedoLSN,
                                    &sFromDiskRedoLSN,
                                    &sToDiskRedoLSN,
                                    &sFromMemRedoLSN,
                                    &sToMemRedoLSN )
              != IDE_SUCCESS );
    sDiskState = 3;

    IDE_CALLBACK_SEND_SYM("\n");

    switch ( aRecoveryType )
    {
        case SMI_RECOVER_UNTILTIME:
        case SMI_RECOVER_UNTILCANCEL:
            {
                // 불완전 복구 인경우
                idlOS::snprintf( sMsgBuf,
                                 SM_MAX_FILE_NAME,
                                 "\n             recover at < %"ID_UINT32_FMT", %"ID_UINT32_FMT" >",
                                 sCurRedoLSN.mFileNo,
                                 sCurRedoLSN.mOffset );

                IDE_CALLBACK_SEND_SYM(sMsgBuf);

                sResetLSN = smrRedoLSNMgr::getLstCheckLogLSN();

                sMemResetLSNPtr  = &sResetLSN;
                sDiskResetLSNPtr = &sResetLSN;
                break;
            }
        case SMI_RECOVER_COMPLETE:
            {
                // 완전복구시에는 ResetLSN을 파일헤더에
                // 설정할 필요없다.
                sMemResetLSNPtr = NULL;
                sDiskResetLSNPtr = NULL;
                break;
            }
        default:
            break;
    }

    if ( (sFailureMediaType & SMR_FAILURE_MEDIA_MRDB)
         == SMR_FAILURE_MEDIA_MRDB )
    {
        // Stable에 모두 플러쉬하고 DIRTY PAGE를 모두 제거한다.
        IDE_TEST( flushAndRemoveDirtyPagesAllMemTBS() != IDE_SUCCESS );

        IDE_CALLBACK_SEND_MSG("  Restoring unstable checkpoint images...");
        // 복구완료할 메모리 데이타파일 헤더의 REDOLSN을 갱신한다.
        IDE_TEST( repairFailureChkptImageHdr( sMemResetLSNPtr )
                  != IDE_SUCCESS );
    }

    sDiskState = 2;
    if ( (sFailureMediaType & SMR_FAILURE_MEDIA_DRDB)
         == SMR_FAILURE_MEDIA_DRDB )
    {
        // 디스크 로그레코드를 모두 반영한다.
        IDE_TEST( applyHashedDiskLogRec( aStatistics) != IDE_SUCCESS );

        // 복구완료할 디스크 데이타파일 헤더의 REDOLSN을 갱신한다.
        IDE_TEST( smLayerCallback::repairFailureDBFHdr( sDiskResetLSNPtr )
                  != IDE_SUCCESS );
    }
    /* Secondary Buffer 파일을 초기화 한다 */
    IDE_TEST( smLayerCallback::repairFailureSBufferHdr( aStatistics,
                                                        sDiskResetLSNPtr )
              != IDE_SUCCESS );
 
    IDE_CALLBACK_SEND_MSG("\n             Log Applied");

    switch ( aRecoveryType )
    {
        case SMI_RECOVER_UNTILTIME:
        case SMI_RECOVER_UNTILCANCEL:
            {
                IDE_ASSERT( sMemResetLSNPtr != NULL );
                IDE_ASSERT( sDiskResetLSNPtr != NULL );

                // update RESETLOGS to loganchor
                IDE_TEST( mAnchorMgr.updateResetLSN( &sResetLSN )
                          != IDE_SUCCESS );

                IDE_CALLBACK_SEND_MSG("\n             updated resetlogs.");

                // update RedoLSN to loganchor
                IDE_TEST( mAnchorMgr.updateRedoLSN( sDiskResetLSNPtr,
                                                   sMemResetLSNPtr )
                         != IDE_SUCCESS );
                break;
            }
        case SMI_RECOVER_COMPLETE:
        default:
            break;
    }

    IDE_CALLBACK_SEND_MSG(
        "  [ RECMGR ] Database media recovery successful.");

    sCommonState = 0;
    IDE_TEST( smrRedoLSNMgr::destroy() != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::getLogFileMgr().closeAllLogFile() != IDE_SUCCESS );

    if ( (sFailureMediaType & SMR_FAILURE_MEDIA_MRDB)
         == SMR_FAILURE_MEDIA_MRDB )
    {
        // [MEMORY-0]
        // 메모리 테이블스페이스의 복구를 수행하기전
        // 복구 대상 테이블스페이스들을 모두 해제한다..
        sMemState = 0;
        IDE_TEST( finalMediaRecovery4MemTBS() != IDE_SUCCESS );
    }

    if ( (sFailureMediaType & SMR_FAILURE_MEDIA_DRDB)
         == SMR_FAILURE_MEDIA_DRDB )
    {
        // dirty page는 존재하지 않기 때문에 모든 buffer 상의
        // page들을 invalid 시킨다.
        IDE_TEST( sdbBufferMgr::pageOutAll( NULL )
                  != IDE_SUCCESS );
    }

    // flush manager를 종료시킨다. sdbBufferMgr::pageOutAll()을 하기위해서는
    // flush manager가 떠있어야 하기 때문에 pageOutAll()보다 뒤에 둬야한다.
    sDiskState = 1;
    IDE_TEST( sdbFlushMgr::destroy() != IDE_SUCCESS );

    // [DISK-0]
    sDiskState = 0;
    IDE_TEST( smLayerCallback::destroyRedoMgr() != IDE_SUCCESS );

    mRestart = ID_FALSE;
    mMediaRecoveryPhase = ID_FALSE; /* 미디어 복구 수행중에만 TRUE */

    // fix BUG-17158
    // offline DBF에 write가 불가능하도록 flag 를 설정한다.
    sddDiskMgr::setEnableWriteToOfflineDBF( ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_log_consistency )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_ERR_LOG_CONSISTENCY, mLostLogFile ) );
    }
    IDE_EXCEPTION( err_secondary_buffer_service );
    {
        IDE_SET( ideSetErrorCode( 
                    smERR_ABORT_service_secondary_buffer_in_recv ) );
    }
    IDE_EXCEPTION( err_exist_active_trans );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_EXIST_ACTIVE_TRANS_IN_RECOV ));
    }
    IDE_EXCEPTION(err_archivelog_mode);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ErrArchiveLogMode));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch ( sMemState )
        {
            case 1:
                IDE_ASSERT( finalMediaRecovery4MemTBS() == IDE_SUCCESS );
                break;
            default:
                break;
        }

        switch ( sDiskState )
        {
            case 3:
                IDE_ASSERT( smLayerCallback::applyHashedLogRec( aStatistics )
                            == IDE_SUCCESS );

            case 2:
                IDE_ASSERT( sdbFlushMgr::destroy() == IDE_SUCCESS );

            case 1:
                IDE_ASSERT( smLayerCallback::removeAllRecvDBFHashNodes()
                            == IDE_SUCCESS );

                IDE_ASSERT( smLayerCallback::destroyRedoMgr() == IDE_SUCCESS );
                break;
            default:
                break;
        }

        switch ( sCommonState )
        {
            case 1:
                IDE_ASSERT( smrRedoLSNMgr::destroy() == IDE_SUCCESS );
                IDE_ASSERT( smrLogMgr::getLogFileMgr().closeAllLogFile() == IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    IDE_POP();

    mMediaRecoveryPhase = ID_FALSE; /* 미디어 복구 수행중에만 TRUE */
    mRestart = ID_FALSE;
    sddDiskMgr::setEnableWriteToOfflineDBF( ID_FALSE );

    return IDE_FAILURE;

}

/*
   판독된 로그가 common 로그타입인지 확인한다.

   [IN]  aCurLogPtr         - 현재 판독된 로그의 Ptr
   [IN]  aLogType           - 현재 판독된 로그의 Type
   [OUT] aIsApplyLog        - 미디어복구 적용여부

   BUG-31430 - Redo Logs should not be reflected in the complete media recovery
               have been reflected.

   aFailureMediaType을 확인하여 이미 미디어 복구 완료된 로그는 적용하지 않도록
   한다.
*/
IDE_RC smrRecoveryMgr::filterCommonRedoLogType( smrLogType   aLogType,
                                                UInt         aFailureMediaType,
                                                idBool     * aIsApplyLog )
{
    idBool          sIsApplyLog;

    IDE_DASSERT( aIsApplyLog != NULL );

    switch ( aLogType )
    {
        case SMR_LT_FILE_END:
            {
                sIsApplyLog = ID_TRUE;
                break;
            }
        case SMR_LT_MEMTRANS_COMMIT:
        case SMR_LT_MEMTRANS_GROUPCOMMIT:
            {
                if ( (aFailureMediaType & SMR_FAILURE_MEDIA_MRDB) ==
                     SMR_FAILURE_MEDIA_MRDB )
                {
                    sIsApplyLog = ID_TRUE;
                }
                else
                {
                    sIsApplyLog = ID_FALSE;
                }
                break;
            }
        case SMR_LT_DSKTRANS_COMMIT:
            {
                if ( (aFailureMediaType & SMR_FAILURE_MEDIA_DRDB) ==
                     SMR_FAILURE_MEDIA_DRDB )
                {
                    sIsApplyLog = ID_TRUE;
                }
                else
                {
                    sIsApplyLog = ID_FALSE;
                }
                break;
            }
        default:
            {
                sIsApplyLog = ID_FALSE;
                break;
            }
    }

    *aIsApplyLog = sIsApplyLog;

    return IDE_SUCCESS;
}

/*
   판독된 로그가 오류난 메모리 데이타파일의 범위에 포함되는지
   판단하여 적용여부를 반환한다.

   [IN]  aCurLogPtr         - 현재 판독된 로그의 Ptr
   [IN]  aLogType           - 현재 판독된 로그의 Type
   [OUT] aIsApplyLog        - 미디어복구 적용여부
*/
IDE_RC smrRecoveryMgr::filterRedoLog4MemTBS(
    SChar      * aCurLogPtr,
    smrLogType   aLogType,
    idBool     * aIsApplyLog )
{
    scGRID          sGRID;
    scSpaceID       sSpaceID;
    scPageID        sPageID;
    smrUpdateLog    sUpdateLog;
    smrCMPSLog      sCMPSLog;
    smrNTALog       sNTALog;
    idBool          sIsMemTBSLog;
    idBool          sIsExistTBS;
    idBool          sIsApplyLog;

    IDE_DASSERT( aCurLogPtr  != NULL );
    IDE_DASSERT( aIsApplyLog != NULL );

    SC_MAKE_GRID( sGRID, 0, 0, 0 );

    // SMR_FAILURE_MEDIA_MRDB
    // [1] 로그타입확인
    switch ( aLogType )
    {
        case SMR_LT_TBS_UPDATE:
            {
                // MEMORY TABLESPACE UPDATE 로그는 반영하지 않는다.
                // DISK TABLESPACE UPDATE 로그는 선별적으로
                // 반영한다. => filterRedoLogType4DiskTBS에서 처리
                // 일단 본 함수에서는 무조건 aIsApplyLog 를 ID_FALSE로
                // 반환한다.
                // 미디어 복구에서는 Loganchor에 어떠한 TableSpace 상태도
                // 반영하지 않는다. Loganchor를 복구하지 않는다는 의미이다.
                sIsMemTBSLog = ID_FALSE;
                break;
            }
        case SMR_LT_UPDATE:
            {
                // Memory Update Log를 반영한다.
                idlOS::memcpy( &sUpdateLog,
                               aCurLogPtr,
                               ID_SIZEOF(smrUpdateLog) );

                SC_COPY_GRID( sUpdateLog.mGRID, sGRID );
                sIsMemTBSLog = ID_TRUE;
                break;
            }
        case SMR_LT_COMPENSATION:
            {
                idlOS::memcpy( &sCMPSLog,
                               aCurLogPtr,
                               SMR_LOGREC_SIZE(smrCMPSLog) );

                if ( (sctUpdateType)sCMPSLog.mTBSUptType
                     == SCT_UPDATE_MAXMAX_TYPE)
                {
                    sIsMemTBSLog = ID_TRUE;

                    // MEMORY UPDATE CLR 로그는 반영한다.
                    SC_COPY_GRID( sCMPSLog.mGRID, sGRID );
                }
                else
                {
                    // MEMORY TABLESAPCE UPDATE CLR 로그
                    // 반영하지 않는다.
                    sIsMemTBSLog = ID_FALSE;
                }
                break;
            }
        case SMR_LT_DIRTY_PAGE:
            {
                // 일단 Apply 로그로 설정하고
                // redo 할 때 선별한다.
                sIsMemTBSLog = ID_TRUE;
                break;
            }
        case SMR_LT_NTA:
            {
                // BUG-28709 Media recovery시 SMR_LT_NTA에 대해서도
                //           redo를 해야 함
                // BUG-29434 NTA Log가 반영될 Page가 Media Recovery
                //           대상인지 확인합니다.
                idlOS::memcpy( &sNTALog,
                               aCurLogPtr,
                               SMR_LOGREC_SIZE(smrNTALog));

                sIsMemTBSLog   = ID_TRUE;
                sSpaceID = sNTALog.mSpaceID;

                switch(sNTALog.mOPType)
                {
                    case SMR_OP_SMC_TABLEHEADER_ALLOC:
                    case SMR_OP_SMC_FIXED_SLOT_ALLOC:
                    case SMR_OP_CREATE_TABLE:
                        sPageID = SM_MAKE_PID(sNTALog.mData1);
                        SC_MAKE_GRID( sGRID, sSpaceID, sPageID, 0 );
                        break;
                    default :
                        // Media Recovery에서 Redo대상이 아님
                        sIsMemTBSLog = ID_FALSE;
                        break;
                }

                break;
            }
        default:
            {
                sIsMemTBSLog = ID_FALSE;
                break;
            }
    }

    // Memory Log 인 경우
    if ( sIsMemTBSLog == ID_TRUE )
    {
        if ( SC_GRID_IS_NOT_NULL( sGRID ) )
        {
            // [2] 로그의 변경페이지가 오류난 데이타파일에
            // 해당하는지 확인
            IDE_TEST( smmTBSMediaRecovery::findMatchFailureDBF(
                          SC_MAKE_SPACE( sGRID ),
                          SC_MAKE_PID( sGRID ),
                          &sIsExistTBS,
                          &sIsApplyLog )
                      != IDE_SUCCESS );
        }
        else
        {
            // redo 할 때 선별한다.
            sIsApplyLog = ID_TRUE;
        }
    }
    else
    {
        // APPLY할 Memory Log가 아닌 경우
        sIsApplyLog = ID_FALSE;
    }

    *aIsApplyLog = sIsApplyLog;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   판독된 로그가 디스크 로그타입인지 확인한다.
   실제 오류난 데이타파일의 페이지범위 내에 포함되는 로그인지는
   sdrRedoMgr에 의해서 파싱할때 Filtering된다.

   [IN]  aCurLogPtr         - 현재 판독된 로그의 Ptr
   [IN]  aLogType           - 현재 판독된 로그의 Type
   [OUT] aIsApplyLog        - 미디어복구 적용여부
*/
IDE_RC smrRecoveryMgr::filterRedoLogType4DiskTBS( SChar       * aCurLogPtr,
                                                  smrLogType    aLogType,
                                                  UInt          aIsNeedApplyDLT,
                                                  idBool      * aIsApplyLog )
{
    idBool          sIsApplyLog;
    smrTBSUptLog    sTBSUptLog;

    IDE_DASSERT( aCurLogPtr  != NULL );
    IDE_DASSERT( aIsApplyLog != NULL );
    IDE_DASSERT( (aIsNeedApplyDLT == SMR_DLT_REDO_EXT_DBF) ||
                 (aIsNeedApplyDLT == SMR_DLT_REDO_ALL_DLT) );

    // SMR_FAILURE_MEDIA_DRDB
    switch ( aLogType )
    {
        case SMR_LT_TBS_UPDATE:
            {
                // 미디어 복구에서는 Loganchor에 어떠한 TableSpace 상태도
                // 반영하지 않는다. Loganchor를 복구하지 않는다는 의미이다.
                idlOS::memcpy( &sTBSUptLog,
                               aCurLogPtr,
                               ID_SIZEOF(smrTBSUptLog) );

                if ( sTBSUptLog.mTBSUptType ==
                     SCT_UPDATE_DRDB_EXTEND_DBF )
                {
                    // DISK TABLESPACE의 EXTEND DBF 로그는
                    // 반영한다.
                    sIsApplyLog = ID_TRUE;
                }
                else
                {
                    // 그 외 타입은 반영하지 않는다.
                    sIsApplyLog = ID_FALSE;
                }
                break;
            }
        case SMR_DLT_REDOONLY:
        case SMR_DLT_UNDOABLE:
        case SMR_DLT_NTA:
        case SMR_DLT_REF_NTA:
        case SMR_DLT_COMPENSATION:
            {
                // 모두 반영한다.
                if ( aIsNeedApplyDLT == SMR_DLT_REDO_ALL_DLT )
                {
                    sIsApplyLog = ID_TRUE;
                    break;
                }
                else
                {
                    sIsApplyLog = ID_FALSE;
                    break;
                }
            }
        default:
            {
                sIsApplyLog = ID_FALSE;
                break;
            }
    }

    *aIsApplyLog = sIsApplyLog;

    return IDE_SUCCESS;
}

/*
  To Fix PR-13786
  복잡도 개선을 위하여 recoverDB() 함수에서
  하나의 log file에 대한 redo 작업을 분리한다.

*/
IDE_RC smrRecoveryMgr::recoverAllFailureTBS( smiRecoverType        aRecoveryType,
                                             UInt                  aFailureMediaType,
                                             time_t              * aUntilTIME,
                                             smLSN               * aCurRedoLSNPtr,
                                             smLSN               * aFromDiskRedoLSN,
                                             smLSN               * aToDiskRedoLSN,
                                             smLSN               * aFromMemRedoLSN,
                                             smLSN               * aToMemRedoLSN )
{
    smrLogHead        * sLogHeadPtr = NULL;
    SChar*              sLogPtr;
    UInt                sFileCount;
    smrTransCommitLog   sCommitLog;
    time_t              sCommitTIME;
    // To Fix PR-14650, PR-14660
    idBool              sIsValid       = ID_FALSE;
    void              * sCurTrans      = NULL;
    smLSN             * sCurRedoLSNPtr = NULL;
    smrLogType          sLogType;
    UInt                sFailureMediaType;
    idBool              sIsApplyLog;
    UInt                sLogSizeAtDisk = 0;
    // BUG-38503
    UInt                sIsNeedApplyDLT = SMR_DLT_REDO_NONE;

    IDE_DASSERT( aFailureMediaType != SMR_FAILURE_MEDIA_NONE );
    IDE_DASSERT( aRecoveryType     != SMI_RECOVER_RESTART );
    IDE_DASSERT( aFromDiskRedoLSN  != NULL );
    IDE_DASSERT( aToDiskRedoLSN    != NULL );
    IDE_DASSERT( aFromMemRedoLSN!= NULL );
    IDE_DASSERT( aToMemRedoLSN  != NULL );

    sFailureMediaType = aFailureMediaType;

    sFileCount = 1;

    while ( 1 )
    {
        sIsApplyLog     = ID_FALSE;
        sIsNeedApplyDLT = SMR_DLT_REDO_NONE;

        // [1] Log 판독
        IDE_TEST( smrRedoLSNMgr::readLog(&sCurRedoLSNPtr,
                                        &sLogHeadPtr,
                                        &sLogPtr,
                                        &sLogSizeAtDisk,
                                        &sIsValid)
                 != IDE_SUCCESS );

        // [2] 로그레코드가 Invalid 한 경우 완료조건 확인
        // 보통 UNTILCANCEL시 로그파일이 존재하지 않은 경우에
        // 해당한다.
        if ( sIsValid == ID_FALSE )
        {
            if ( sCurRedoLSNPtr != NULL )
            {
                ideLog::log( IDE_SM_0, 
                             "Media Recovery Completed "
                             "[ %"ID_UINT32_FMT", %"ID_UINT32_FMT"]",
                             sCurRedoLSNPtr->mFileNo,
                             sCurRedoLSNPtr->mOffset );
            }

            break; /* invalid한 로그를 판독한 경우 */
        }

        mCurLogPtr     = sLogPtr;
        mCurLogHeadPtr = sLogHeadPtr;
        SM_GET_LSN( mLstRedoLSN, *sCurRedoLSNPtr );

        sLogType = smrLogHeadI::getType(sLogHeadPtr);

        // [3] 미디어 오류가 발생한 테이블스페이스에 따라 Log의
        // 적용여부를 확인한다.
        IDE_ASSERT( sIsApplyLog == ID_FALSE );

        /*
           redo 구간이 결정된 상태에서 관련된 로그만
           redo 를 하기 위한것이다.

           1. From Redo LSN
           RedoLSNMgr가 반환하는 로그중에 Memory From Redo LSN
           보다 크거나같은 LSN의 로그가 나타나면 복구구간에 포함된다.

           2. To Redo LSN
           RedoLSNMgr가 반환하는 로그중에 Memory To Redo LSN
           보다 큰 LSN의 로그가 나타나면 복구구간에 포함된다.  */

        // commit 로그거나 file_end 로그인지 확인한다
        IDE_TEST( filterCommonRedoLogType( sLogType,
                                           sFailureMediaType,
                                           & sIsApplyLog )
                  != IDE_SUCCESS );

        if ( ( sFailureMediaType & SMR_FAILURE_MEDIA_MRDB )
             == SMR_FAILURE_MEDIA_MRDB )
        {
            // 적용할 메모리 로그 Filtering
            if ( sIsApplyLog == ID_FALSE )
            {
                IDE_TEST( filterRedoLog4MemTBS( sLogPtr,
                                                sLogType,
                                                & sIsApplyLog)
                          != IDE_SUCCESS );

                // Memory From Redo LSN 비교하여 적용여부 판단
                if ( smrCompareLSN::isLT( sCurRedoLSNPtr,
                                          aFromMemRedoLSN )
                     == ID_TRUE )
                {
                    // 다음 로그를 판독한다.
                    sIsApplyLog = ID_FALSE;
                }
                else
                {
                    // Memory Tablespace 복구구간에 해당한다.
                    // Failure 데이타파일인지 여부를 검사해서
                    // 최종 적용여부를 검토한다.
                }

                // Memory To Redo LSN 비교하여 적용여부 판단
                if ( smrCompareLSN::isGT( sCurRedoLSNPtr,
                                          aToMemRedoLSN )
                     == ID_TRUE )
                {
                    // 해당로그는 반영하지 않는다.
                    sIsApplyLog = ID_FALSE;

                    // 모든 적용할 로그가 적용되었다.
                    sFailureMediaType &= (~SMR_FAILURE_MEDIA_MRDB);
                }
                else
                {
                    // 현재 판독된 로그가 아직 ToMemReodLSN에
                    // 도달하지 않은 경우
                }
            }
            else
            {
                // 이미 적용판단이 선 경우이다
            }
        }
        else
        {
            // Memory TableSpace가 복구대상이 아닌 경우
        }

        if ( ( sFailureMediaType & SMR_FAILURE_MEDIA_DRDB )
             == SMR_FAILURE_MEDIA_DRDB )
        {
            // DISK 에 대해서만 처리한다.
            if ( sIsApplyLog == ID_FALSE )
            {
                // Disk From Redo LSN 비교하여 적용여부 판단
                if ( smrCompareLSN::isLT( sCurRedoLSNPtr,
                                          aFromDiskRedoLSN ) == ID_TRUE )
                {
                    sIsNeedApplyDLT = SMR_DLT_REDO_NONE;
                }
                else
                {
                    // Memory Tablespace 복구구간에 해당한다.
                    // Failure 데이타파일인지 여부를 검사해서
                    // 최종 적용여부를 검토한다.

                    // Disk To Redo LSN 비교하여 적용여부 판단
                    if ( smrCompareLSN::isGT( sCurRedoLSNPtr, aToDiskRedoLSN )
                         == ID_TRUE )
                    {
                        // 현재 판독된 로그가 aToDiskRedoLSN보다 크다면
                        // 로그 적용을 더이상 진행하지 않는다.

                        // BUG-38503
                        // 완전 복구에 한해서, SMR_LT_TBS_UPDATE 타입 중 
                        // SCT_UPDATE_DRDB_EXTEND_DBF 를 계속 redo 하도록 한다.
                        if ( smrCompareLSN::isGT( sCurRedoLSNPtr,
                                                  aToMemRedoLSN )
                             == ID_TRUE )
                        {
                            // 메모리 TBS 종료 시점이면, Disk도 종료시킨다. 
                            sFailureMediaType  &= (~SMR_FAILURE_MEDIA_DRDB);
                            sIsNeedApplyDLT     = SMR_DLT_REDO_NONE;
                        }
                        else
                        {
                            sIsNeedApplyDLT = SMR_DLT_REDO_EXT_DBF;
                        }
                    }
                    else
                    {
                        // 현재 판독된 로그가 아직 ToMemReodLSN에
                        // 도달하지 않은 경우
                        sIsNeedApplyDLT = SMR_DLT_REDO_ALL_DLT;
                    }
                }

                if ( sIsNeedApplyDLT == SMR_DLT_REDO_NONE )
                {
                    sIsApplyLog = ID_FALSE;
                }
                else
                {
                    // 적용할 디스크 로그 Filtering
                    IDE_TEST( filterRedoLogType4DiskTBS(
                            sLogPtr,
                            sLogType,
                            sIsNeedApplyDLT,
                            &sIsApplyLog ) != IDE_SUCCESS );
                }
            }
            else
            {
                // 이미 적용 판단이 선 경우이다.
            }
        }
        else
        {
            // Disk TableSpace가 복구대상이 아닌 경우
        }

        // [4] 복구 완료 조건 체크
        if ( sFailureMediaType == SMR_FAILURE_MEDIA_NONE )
        {
            // COMPLETE 복구의 경우에 여기서 종료할 수 있다.
            // 복구구간을 모두 scan 한 경우 더이상 redo를
            // 진행하지 않는다.
            break;
        }

        // [5] 적용할 로그인가?
        if ( sIsApplyLog == ID_FALSE )
        {
            // 다음 로그를 판독한다.
            sCurRedoLSNPtr->mOffset += sLogSizeAtDisk;
            continue;
        }

        if ( (sLogType == SMR_LT_MEMTRANS_COMMIT) ||
             (sLogType == SMR_LT_DSKTRANS_COMMIT) ||
             (sLogType == SMR_LT_MEMTRANS_GROUPCOMMIT) )
        {
            // [7] 타임베이스 불완전복구라면 커밋로그에 저장된
            // 시간을 확인하여 재수행 중지 여부를 판단한다.
            if ( aRecoveryType == SMI_RECOVER_UNTILTIME )
            {
                /* BUG-47525 일반 CommitLog와 GroupCommitLog의 생긴건 다르지만
                 * Time 까지는 동일하기 때문에 Time만 확인하는 경우라면
                 * 그냥 CommitLog 형태로 읽어도 된다. */
                idlOS::memcpy(&sCommitLog,
                              sLogPtr,
                              ID_SIZEOF(smrTransCommitLog));

                sCommitTIME = (time_t)sCommitLog.mTxCommitTV;

                if (sCommitTIME > *aUntilTIME)
                {
                    // Time-Based 불완전 복구 완료시점
                    break;
                }
                else
                {
                    // commit log redo 수행
                }
            }
            else
            {
                // 타임베이스 불완전 복구가 아닌 경우
                // commit log에 대해서 체크하지 않는다.
            }
        }

        // Get Transaction Entry
        // 메모리 slot에 TID를 설정해야하는 경우가 있다.
        // 미디어 복구시에도 Transaction 객체를 넘기지만,
        // 트랜잭션 상태를 Begin 시키지 않는다.
        // 왜냐하면 Begin 상태가 아닌 경우에 OID 등록과 Pending 연산
        // 등록이 되지 않아 active tx list와 prepare tx
        // 등도 고려하지 않아도 되기 때문이다.
        sCurTrans = smLayerCallback::getTransByTID( smrLogHeadI::getTransID( sLogHeadPtr ) );

        // [6] 적용해야할 로그 레코드를 재수행한다.
        switch( sLogType )
        {
            case SMR_LT_UPDATE:
            case SMR_LT_DIRTY_PAGE:
            case SMR_LT_TBS_UPDATE:
            case SMR_LT_COMPENSATION:
            case SMR_LT_NTA:
            case SMR_DLT_REDOONLY:
            case SMR_DLT_UNDOABLE:
            case SMR_DLT_NTA:
            case SMR_DLT_REF_NTA:
            case SMR_DLT_COMPENSATION:
            case SMR_LT_FILE_END:
            case SMR_LT_MEMTRANS_COMMIT:
            case SMR_LT_DSKTRANS_COMMIT:
            case SMR_LT_MEMTRANS_GROUPCOMMIT:
                {
                    // 트랜잭션과 상관없이 재수행
                    IDE_TEST( redo( sCurTrans,
                                    sCurRedoLSNPtr,
                                    &sFileCount,
                                    sLogHeadPtr,
                                    sLogPtr,
                                    sLogSizeAtDisk,
                                    ID_TRUE  /* after checkpoint */ )
                              != IDE_SUCCESS );
                    break;
                }
            default :
                {
                    // sIsApplyLog == ID_FALSE
                    // filter 과정에서 이미 걸러졌으므로
                    // assert 검사한다.
                    IDE_ASSERT( 0 );
                    break;
                }
        }

        // 해싱된 디스크로그들의 크기의 총합이
        // DISK_REDO_LOG_DECOMPRESS_BUFFER_SIZE 를 벗어나면
        // 해싱된 로그들을 모두 버퍼에 적용한다.
        IDE_TEST( checkRedoDecompLogBufferSize() != IDE_SUCCESS );
    }
    mCurLogPtr     = NULL;
    mCurLogHeadPtr = NULL;

    // To Fix PR-14560, PR-14660
    if ( sCurRedoLSNPtr != NULL )
    {
        SM_GET_LSN( *aCurRedoLSNPtr, *sCurRedoLSNPtr );
    }
    else
    {
        SM_LSN_INIT( *aCurRedoLSNPtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mCurLogPtr     = NULL;
    mCurLogHeadPtr = NULL;

    return IDE_FAILURE;
}


/*********************************************************
 * Description:rebuildArchLogfileList은 Server Start시 Archive LogFile
 * List를 재구성하는 것이다. checkpoint시 지워진 파일은 archive되었
 * 기때문에 다음파일부터 archive List에 추가한다.
 *
 * aEndLSN - [IN] 마지막 Write된 logfile까지 Archive Logfile List에 추
 *              가한다.
 * BUGBUG: 추가되는 Logfile이 이미 Archive되었을 수 있다. 어디선가 check해야됨.
 *********************************************************/
IDE_RC smrRecoveryMgr::rebuildArchLogfileList( smLSN  *aEndLSN )
{
    UInt sLstDeleteLogFileNo;
    UInt sAddArchLogFileCount = 0;

    sLstDeleteLogFileNo = getLstDeleteLogFileNo();

    if ( sLstDeleteLogFileNo < aEndLSN->mFileNo )
    {
        sAddArchLogFileCount = sAddArchLogFileCount + aEndLSN->mFileNo - sLstDeleteLogFileNo;

        IDE_TEST( smrLogMgr::getArchiveThread().recoverArchiveLogList( sLstDeleteLogFileNo,
                                                                       aEndLSN->mFileNo )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    if ( sAddArchLogFileCount != 0)
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_REBUILD_ARCHLOG_LIST,
                    sAddArchLogFileCount);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * Description: Restart Recovery시에 사용될 Redo LSN을 계산한다.
 *
 *    Checkpoint 과정 중
 *    BEGIN CHECKPOINT LOG 이전에 수행할 일들
 *
 *    A. Restart Recovery시에 Redo시작 LSN으로 사용될 Recovery LSN을 계산
 *        A-1 Memory Recovery LSN 결정
 *        A-2 Disk Recovery LSN 결정
 *            - Disk Dirty Page Flush        [step0]
 *            - Tablespace Log Anchor 동기화 [step1]
 *
 * Implementation
 *
 *********************************************************/
IDE_RC smrRecoveryMgr::chkptCalcRedoLSN( idvSQL       * aStatistics,
                                         smrChkptType   aChkptType,
                                         idBool         aIsFinal,
                                         smLSN        * aRedoLSN,
                                         smLSN        * aDiskRedoLSN,
                                         smLSN        * aEndLSN )
{
    smLSN   sMinLSN;
    smLSN   sDiskRedoLSN;
    smLSN   sSBufferRedoLSN; 

    //---------------------------------
    // A-1. Memory Recovery LSN 의 결정
    //---------------------------------

    //---------------------------------
    // Lst LSN을 가져온다.
    //---------------------------------
    smrLogMgr::getLstLSN(aRedoLSN);

    *aDiskRedoLSN = *aRedoLSN;
    *aEndLSN      = *aDiskRedoLSN;

    /* ------------------------------------------------
     * # MRDB의 recovery LSN을 결정하여 시작 checkpoint
     * 로그에 기록한다.
     * 트랜잭션들중 가장 오래전에 기록한 로그의 LSN으로 설정하며,
     * 없다면, 로그관리자의 End LSN으로 설정한다.
     * ----------------------------------------------*/
    smLayerCallback::getMinLSNOfAllActiveTrans( &sMinLSN );

    if ( sMinLSN.mFileNo != ID_UINT_MAX )
    {

        if ( smrCompareLSN::isGT( aRedoLSN,
                                 &sMinLSN ) == ID_TRUE )

        {
            *aRedoLSN = sMinLSN;
        }
        else
        {
            /* nothing to do ... */
        }
    }
    else
    {
        /* nothing to do ... */
    }

    //---------------------------------
    // A-2. Disk Recovery LSN 의 결정
    //---------------------------------
    /* ------------------------------------------------
     * 시작 checkpoint 로그에 기록할 DRDB의 recovery LSN을 결정한다.
     * 버퍼관리자의 dirty page flush 리스트에서 가장 오래된
     * dirty page의 first modified LSN으로 설정하며, 없다면
     * 로그관리자의 End LSN으로 설정한다.
     * ----------------------------------------------*/
    if ( aChkptType == SMR_CHKPT_TYPE_BOTH )
    {
        IDE_TEST( makeBufferChkpt( aStatistics,
                                   aIsFinal,
                                   aEndLSN,
                                   aDiskRedoLSN )
                  != IDE_SUCCESS );
    }
    else
    {
        // 보통 메모리 DB 체크포인트는 디스크 테이블스페이스 백업이
        // 진행 중인 경우 디스크체크포인트는 하지 않지만, 로그파일이
        // 삭제가 되기 때문에 백업과 관련된 로그파일을 삭제하지 않도록
        // 고려한다.

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP21);


        sdbBufferMgr::getMinRecoveryLSN( aStatistics, 
                                         &sDiskRedoLSN );

        sdsBufferMgr::getMinRecoveryLSN( aStatistics, 
                                         &sSBufferRedoLSN );

        if ( smrCompareLSN::isLT( &sDiskRedoLSN, &sSBufferRedoLSN )
             == ID_TRUE )
        {
            // PBuffer에 있는 RecoveryLSN(sDiskRedoLSN)이 
            // SecondaryBuffer에 있는 RecoveryLSN 보다 작으면
            // PBuffer에 있는 RecoveryLSN  선택
            SM_GET_LSN( *aDiskRedoLSN, sDiskRedoLSN );
        }
        else
        {
            SM_GET_LSN( *aDiskRedoLSN, sSBufferRedoLSN );
        }

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP29,
                    aDiskRedoLSN->mFileNo,
                    aDiskRedoLSN->mOffset );
    }

    if ( (aDiskRedoLSN->mFileNo != ID_UINT_MAX) &&
         (aDiskRedoLSN->mOffset != ID_UINT_MAX))
    {
        // do nothing
    }
    else
    {
        IDE_ASSERT(( aDiskRedoLSN->mFileNo == ID_UINT_MAX) &&
                   ( aDiskRedoLSN->mOffset == ID_UINT_MAX) );
        *aDiskRedoLSN                 = *aEndLSN;
    }

    IDE_DASSERT( (aDiskRedoLSN->mFileNo != ID_UINT_MAX) &&
                 (aDiskRedoLSN->mOffset != ID_UINT_MAX) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************
 * Description: Memory Dirty Page들을 Checkpoint Image에 Flush한다.
 *
 * [OUT] aSyncLstLSN - Sync한 마지막 LSN
 *
 *    Checkpoint 과정 중
 *    END CHECKPOINT LOG 이전에 수행하는 작업
 *
 *    C. Memory Dirty Page들을 Checkpoint Image에 Flush
 *       C-1 Flush Dirty Pages              [step3]
 *       C-2 Sync Log Files
 *       C-3 Sync Memory Database           [step4]
 *
 * Implementation
 *
 *********************************************************/
IDE_RC smrRecoveryMgr::chkptFlushMemDirtyPages( smLSN * aSyncLstLSN,
                                                idBool  aIsFinal )
{
    smrWriteDPOption sWriteOption;
    ULong            sTotalDirtyPageCnt;
    ULong            sNewDirtyPageCnt;
    ULong            sDupDirtyPageCnt;
    ULong            sTotalCnt;
    ULong            sRemoveCnt;
    SLong            sSyncedLogSize;
    smLSN            sSyncedLSN;
    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * Checkpoint과정의 시간을 제어 IO 통계정보를 기록함 */
    ULong            sTotalTime;    /* microsecond (이하 동문) */
    ULong            sLogSyncTime;
    ULong            sDBFlushTime;
    ULong            sDBSyncTime;
    ULong            sDBWaitTime;
    ULong            sDBWriteTime;
    PDL_Time_Value   sTimevalue;
    ULong            sLogSyncBeginTime;
    ULong            sDPFlushBeginTime;
    ULong            sLastSyncTime;
    ULong            sEndTime;
    SDouble          sLogIOPerf;
    SDouble          sDBIOPerf;

    IDE_DASSERT( aSyncLstLSN != NULL );

    //---------------------------------
    // C-1. Flush Dirty Pages
    //---------------------------------

    // 메세지 기록
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP22);

        IDE_TEST( smrDPListMgr::getTotalDirtyPageCnt( & sTotalDirtyPageCnt )
                  != IDE_SUCCESS );

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP23,
                    sTotalDirtyPageCnt );
    }

    // 3) Write dirty pages on backup file
    //     3.1) Write dirty pages of last check point
    //
    // 모든 Tablespace에 대해 SMM => SMR 로 Dirty Page이동
    IDE_TEST( smrDPListMgr::moveDirtyPages4AllTBS(
                                          SCT_SS_SKIP_CHECKPOINT,
                                          & sNewDirtyPageCnt,
                                          & sDupDirtyPageCnt)
              != IDE_SUCCESS );

    // 메세지 기록
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP24,
                    sNewDirtyPageCnt);

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP25,
                    sDupDirtyPageCnt);
    }

    //---------------------------------
    // C-2. Sync Log Files
    //---------------------------------

    // fix PR-2353
    /* LOG File sync수행 */
    smrLogMgr::getLstLSN( aSyncLstLSN );

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_BEGIN_SYNCLSN,
                aSyncLstLSN->mFileNo,
                aSyncLstLSN->mOffset );

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * 이하에서는 gettimeofday로 시간을 젠다. 이 함수는 병목이 꽤 심한
     * 함수이지만, Checkpoint는 혼자 수행하며 그리 자주 일어나지 않기 때문에
     * 사용해도 무방하다. */
    /*==============================LogSync==============================*/
    sTimevalue        = idlOS::gettimeofday();
    sLogSyncBeginTime = 
        (time_t)sTimevalue.sec() * 1000 * 1000 + (time_t)sTimevalue.usec();
    IDE_TEST( smrLogMgr::getLFThread().getSyncedLSN( &sSyncedLSN )
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_CKP, aSyncLstLSN )
             != IDE_SUCCESS );

    sSyncedLogSize = 0;
    if ( SM_IS_LSN_INIT( sSyncedLSN ) )
    {
        /* Synced가 INIT LSN일 경우, 정확한 계산을 할 수 없다. */
        sSyncedLogSize = 0;
    }
    else
    {
        sSyncedLogSize = ( ( (SInt) aSyncLstLSN->mFileNo - sSyncedLSN.mFileNo ) *
                           smuProperty::getLogFileSize() ) + 
                         ( (SInt) aSyncLstLSN->mOffset - sSyncedLSN.mOffset );
    }

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_END_SYNCLSN );

    if ( aIsFinal == ID_TRUE )
    {
        sWriteOption = SMR_WDP_FINAL_WRITE ;
    }
    else
    {
        sWriteOption = SMR_WDP_NONE ;
    }

    /*============================DirtyPageFlush============================*/
    sTimevalue        = idlOS::gettimeofday();
    sDPFlushBeginTime = 
        (time_t)sTimevalue.sec() * 1000 * 1000 + (time_t)sTimevalue.usec();

    // Dirty Page들을 Disk의 Checkpoint Image로 Flush한다.
    IDE_TEST( smrDPListMgr::writeDirtyPages4AllTBS(
                                     SCT_SS_SKIP_CHECKPOINT,
                                     & sTotalCnt,
                                     & sRemoveCnt,
                                     & sDBWaitTime,
                                     & sDBSyncTime,
                                     smmManager::getNxtStableDB, // flush target db
                                     sWriteOption ) /* Final이 True일 경우, 맹렬히 돌아야 함. */
              != IDE_SUCCESS );

    // 메세지 기록
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP26,
                    sTotalCnt);

        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP27,
                    sRemoveCnt);
    }

    //---------------------------------
    // C-3. Sync Memory Database
    //---------------------------------
    // 메세지 기록
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP28);
    }

    /*==========================LastDirtyPageFlush==========================*/
    sTimevalue    = idlOS::gettimeofday();
    sLastSyncTime = (time_t)sTimevalue.sec() * 1000 * 1000 + (time_t)sTimevalue.usec();

    // 4) sync db files
    // 운영중일 경우에는 모든 Online Tablespace에 대해서 Sync를 수행한다.
    // 미디어복구중에는 모든 Online/Offline TableSpace 중 미디어 복구
    // 대상인 것만 Sync를 수행한다.
    IDE_TEST( smmManager::syncDB( SCT_SS_SKIP_CHECKPOINT,
                                  ID_TRUE /* syncLatch 획득 필요 */)
              != IDE_SUCCESS );

    /*==============================End==============================*/
    sTimevalue = idlOS::gettimeofday();
    sEndTime   = (time_t)sTimevalue.sec() * 1000 * 1000 + (time_t)sTimevalue.usec();

    sTotalTime    = sEndTime - sLogSyncBeginTime;

    sLogSyncTime  = sDPFlushBeginTime - sLogSyncBeginTime;
    sDBFlushTime  = sEndTime - sDPFlushBeginTime;
    sDBSyncTime  += sEndTime - sLastSyncTime;
    if ( sDBFlushTime > ( sDBWaitTime + sDBSyncTime ) ) 
    {
        sDBWriteTime = sDBFlushTime - sDBWaitTime - sDBSyncTime;
    }
    else
    {
        sDBWriteTime = 0;
    }


    /* BUG-33142 [sm-mem-recovery] Incorrect IO stat calculation at MMDB
     * Checkpoint
     * 계산시 변수에 저장된 값의 단위는 Byte와 USec입니다. 이를 MB/Sec으로 표현합니다.
     * ( Byte /1024 /1024 ) / ( USec / 1000 / 1000 )
     * 따라서 이 공식이 맞습니다.
     *
     * 다만 곱하기를 앞에서 하고 나누기를 뒤에서 하는 편이 정확성상 좋기 때문에
     *  Byte / ( USec / 1000 / 1000 )  /1024 /1024
     * 로 변경합니다.*/

    if ( sDBFlushTime > 0 )
    {
        /* Byte/Usec 구함 */
        sDBIOPerf = UINT64_TO_DOUBLE( sTotalCnt ) * SM_PAGE_SIZE / sDBFlushTime;
        /* Byte/Usec -> MByte/Sec으로 변형함 */
        sDBIOPerf = sDBIOPerf * 1000 * 1000 / 1024 / 1024 ;
    }
    else
    {
        sDBIOPerf = 0.0f;
    }

    if ( sLogSyncTime > 0 )
    {
        /* Byte/Usec 구함 */
        sLogIOPerf = UINT64_TO_DOUBLE( sSyncedLogSize ) / sLogSyncTime ;
        /* Byte/Usec -> MByte/Sec으로 변형함 */
        sLogIOPerf = sDBIOPerf * 1000 * 1000 / 1024 / 1024 ;
    }
    else
    {
        sLogIOPerf = 0.0f;
    }

    ideLog::log( IDE_SM_0,
                 "==========================================================\n"
                 "SM IO STAT  - Checkpoint \n"
                 "DB SIZE      : %12"ID_UINT64_FMT" Byte "
                 "( %"ID_UINT64_FMT" Page)\n"
                 "LOG SIZE     : %12"ID_UINT64_FMT" Byte\n"
                 "TOTAL TIME   : %6"ID_UINT64_FMT" s %"ID_UINT64_FMT" us\n"
                 "LOG SYNC TIME: %6"ID_UINT64_FMT" s %"ID_UINT64_FMT" us\n"
                 "DB FLUSH TIME: %6"ID_UINT64_FMT" s %"ID_UINT64_FMT"us\n"
                 "      SYNC TIME : %6"ID_UINT64_FMT" s %"ID_UINT64_FMT" us\n"
                 "      WAIT TIME : %6"ID_UINT64_FMT" s %"ID_UINT64_FMT" us\n"
                 "      WRITE TIME: %6"ID_UINT64_FMT" s %"ID_UINT64_FMT" us\n"
                 "LOG IO PERF  : %6"ID_DOUBLE_G_FMT" MB/sec\n"
                 "DB IO PERF   : %6"ID_DOUBLE_G_FMT" MB/sec\n"
                 "=========================================================\n",
                 sTotalCnt * SM_PAGE_SIZE,  sTotalCnt,
                 sSyncedLogSize, 
                 sTotalTime   /1000/1000, sTotalTime,    
                 sLogSyncTime /1000/1000, sLogSyncTime, 
                 sDBFlushTime /1000/1000, sDBFlushTime, 
                 sDBSyncTime  /1000/1000, sDBSyncTime,  
                 sDBWaitTime  /1000/1000, sDBWaitTime,  
                 sDBWriteTime /1000/1000, sDBWriteTime, 
                 sLogIOPerf,
                 sDBIOPerf );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************
 * Description:
 *
 *    Checkpoint 과정 중
 *    END CHECKPOINT LOG 이후에 수행하는 작업
 *
 *    E. After  End_CHKPT
 *        E-1. Sync Log Files            [step6]
 *        E-2. Get Remove Log File #         [step7]
 *        E-3. Update Log Anchor             [step8]
 *        E-4. Remove Log Files              [step9]
 *
 * Implementation
 *
 *********************************************************/
IDE_RC smrRecoveryMgr::chkptAfterEndChkptLog( idBool    aRemoveLogFile,
                                              idBool    aFinal,
                                              smLSN   * aBeginChkptLSN,
                                              smLSN   * aEndChkptLSN,
                                              smLSN   * aDiskRedoLSN,
                                              smLSN   * aRedoLSN,
                                              smLSN   * aSyncLstLSN,
                                              smLSN   * aDtxMinLSN )
{
    UInt        j = 0;

    smSN        sMinReplicationSN       = SM_SN_NULL;
    smLSN       sMinReplicationLSN      = {ID_UINT_MAX,ID_UINT_MAX};
    UInt        sFstFileNo;
    UInt        sEndFileNo = ID_UINT_MAX;
    UInt        sLstFileNo;
    UInt        sLstArchLogFileNo;

    SInt        sState             = 0;
    smGetMinSN sGetMinSNFunc;
    SInt        sOnlineDRDBRedoCnt;

    //---------------------------------
    // E-1. Sync Log Files
    //---------------------------------

    // 6) flush system log buffer and Wait for Log Flush

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP11);

    /* LOG File sync수행*/
    smrLogMgr::getLstLSN( aSyncLstLSN );

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_BEGIN_SYNCLSN,
                aSyncLstLSN->mFileNo,
                aSyncLstLSN->mOffset );

    IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_CKP, aSyncLstLSN )
              != IDE_SUCCESS );

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP_END_SYNCLSN );

    /* BUG-42785 */
    sOnlineDRDBRedoCnt = getOnlineDRDBRedoCnt();
    if ( sOnlineDRDBRedoCnt != 0 )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    "OnlineRedo is performing, Skip Remove Log File(s)\n");

        /* OnlineDRDBRedo가 수행중에는 LogFile을 지워서는 안된다.
         * Checkpoint에 의한 Flush는 모두 수행되었으므로
         * Recovery 수행시점 갱신을 위하여
         * Log Anchor의 Chkpt 정보는 갱신해주도록 한다. */
        mAnchorMgr.getFstDeleteLogFileNo(&sFstFileNo);
        mAnchorMgr.getLstDeleteLogFileNo(&sLstFileNo);

        IDE_TEST( mAnchorMgr.updateChkptAndFlush( aBeginChkptLSN,
                                                  aEndChkptLSN,
                                                  aDiskRedoLSN,
                                                  aRedoLSN,
                                                  &sFstFileNo,
                                                  &sLstFileNo ) != IDE_SUCCESS );

        IDE_RAISE( SKIP_REMOVE_LOGFILE );
    }
    else
    {
        /* 수행중인 OnlineDRDBRedo가 없으므로 Remove Log File을 수행한다. */
    }

    //---------------------------------
    // E-2. Get Remove Log File #
    //---------------------------------
    sGetMinSNFunc    = mGetMinSNFunc;    // bug-17388

    /* BUG-39675 : codesonar에서 sFstFileNo 초기화 warning이 아래의 DR enable
     * 일 때 getFirstNeedLFN 함수 에서 발견되어 이로인한 오류가 예상됨. 
     * 따라서 sFstFileNo의 초기화를 아래 DR에서 삭제할 로그파일 번호를 구하는
     * 코드로 진입하기전에 수행해준다. 또한 기존에 sFstFileNo의 초기화 코드가
     * 존재했던 로그 파일 삭제 코드에서는 삭제한다. 
     */
    mAnchorMgr.getLstDeleteLogFileNo( &sFstFileNo );

    if ( ( sGetMinSNFunc != NULL ) &&
         ( aFinal == ID_FALSE ) &&
         ( aRemoveLogFile == ID_TRUE ) )
    {    
        /*
          For Parallel Logging: Replication은 자신이 Normal Start나
          Abnormal start 시 보내야할 첫번째 로그의 LSN값을 가지고 있다.
          때문에 LSN보다 작은 값을 가지는 로그만을
          가진 LogFile을 찾아서 지워야 한다.
        */
        sEndFileNo = aRedoLSN->mFileNo;
        sEndFileNo =     // BUG-14898 Restart Redo FileNo (Disk)
            ((aDiskRedoLSN->mFileNo > sEndFileNo)
             ? sEndFileNo : aDiskRedoLSN->mFileNo);

        if ( getArchiveMode() == SMI_LOG_ARCHIVE )
        {
            // 주의! 이 안에서 마지막 Arhive된 로그파일 변수에 대해 Mutex를 잡는다 
            IDE_TEST( smrLogMgr::getArchiveThread().getLstArchLogFileNo( &sLstArchLogFileNo )
                      != IDE_SUCCESS );
        }

        /* FIT/ART/rp/Bugs/BUG-17388/BUG-17388.tc */
        IDU_FIT_POINT( "1.BUG-17388@smrRecoveryMgr::chkptAfterEndChkptLog" );

        // replication의 min sn값을 구한다.
        if ( sGetMinSNFunc( &sEndFileNo,        // bug-14898 restart redo FileNo
                            &sLstArchLogFileNo, // bug-29115 last archive log FileNo
                            &sMinReplicationSN )
             != IDE_SUCCESS )
        {
            /* BUG-40294
             * RP의 콜백 함수가 실패하더라도 체크포인트 실패로 서버가 중단되어선 안됨
             * sGetMinSNFunc 함수가 실패할 경우 로그파일은 유지하고 체크포인트는 진행
             */
            IDE_ERRLOG( IDE_ERR_0 );
            ideLog::log( IDE_ERR_0, 
                         "Fail to get minimum LSN for replication.  " \
                         "The reason why sm log file is not removed when checkpoint is executing\n" );

            IDE_ERRLOG( IDE_SM_0 );
            ideLog::log( IDE_SM_0, 
                         "Fail to get minimum LSN for replication.  " \
                         "The reason why sm log file is not removed when checkpoint is executing\n" );

            mAnchorMgr.getFstDeleteLogFileNo( &sFstFileNo );
            mAnchorMgr.getLstDeleteLogFileNo( &sLstFileNo );

            IDE_TEST( mAnchorMgr.updateChkptAndFlush( aBeginChkptLSN,
                                                      aEndChkptLSN,
                                                      aDiskRedoLSN,
                                                      aRedoLSN,
                                                      &sFstFileNo,
                                                      &sLstFileNo ) != IDE_SUCCESS );

            IDE_RAISE( SKIP_REMOVE_LOGFILE );
        }
        else
        {
            /* nothing to do */
        }

        SM_MAKE_LSN( sMinReplicationLSN, sMinReplicationSN);
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP12);

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVERY_RECOVERYMGR_CHKP13,
                     sMinReplicationLSN.mFileNo,
                     sMinReplicationLSN.mOffset );

        /* Replication을 위해서 필요한 파일들의
         * 첫번째 파일의 번호를 구한다. */
        if ( !SM_IS_LSN_MAX( sMinReplicationLSN ) )
        {
            (void)smrLogMgr::getFirstNeedLFN( sMinReplicationLSN,
                                              sFstFileNo,
                                              sEndFileNo,
                                              &sLstFileNo );
        }
        else
        {
            sLstFileNo = aRedoLSN->mFileNo;
        }

        sLstFileNo = ((aDiskRedoLSN->mFileNo > sLstFileNo)  ? sLstFileNo : aDiskRedoLSN->mFileNo);

        /* BUG-46754 */
        sLstFileNo = ( ( aDtxMinLSN->mFileNo > sLstFileNo ) ? sLstFileNo : aDtxMinLSN->mFileNo );

        if ( getArchiveMode() == SMI_LOG_ARCHIVE )
        {
            sLstFileNo = ( sLstArchLogFileNo > sLstFileNo ) ? sLstFileNo : sLstArchLogFileNo;

        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECOVERY_RECOVERYMGR_CHKP14);

        mAnchorMgr.getFstDeleteLogFileNo(&sFstFileNo);
        mAnchorMgr.getLstDeleteLogFileNo(&sLstFileNo);
    }

    //---------------------------------
    // E-3. Update Log Anchor
    //---------------------------------

    ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                SM_TRC_MRECOVERY_RECOVERYMGR_CHKP15);

    IDL_MEM_BARRIER;

    IDE_TEST( mAnchorMgr.updateStableNoOfAllMemTBSAndFlush()
              != IDE_SUCCESS );

    /* 여기서 sEndLSN값은 Normal Shutdown시에만 Start시에
       Last LSN으로 사용되기때문에 여기서는 단순히
       인자를 채우기 위해서 넘긴다..*/
    IDE_TEST( mAnchorMgr.updateChkptAndFlush( aBeginChkptLSN,
                                              aEndChkptLSN,
                                              aDiskRedoLSN,
                                              aRedoLSN,
                                              &sFstFileNo,
                                              &sLstFileNo ) != IDE_SUCCESS );

    /*  PROJ-2742 Support data integrity after fail-back on 1:1 consistent replication  */
   smrRecoveryMgr::updateLastRemovedFileNo( sLstFileNo ); 

    //---------------------------------
    // E-4. Remove Log Files
    //---------------------------------

    // remove log file that is not needed for restart recovery
    // (s_nFstFileNo <= deleted file < s_nLstFileNo)
    // To fix BUG-5071
    if ( aRemoveLogFile == ID_TRUE )
    {
        IDE_TEST( smrRecoveryMgr::lockDeleteLogFileMtx()
                  != IDE_SUCCESS );
        sState = 1;

        if (sFstFileNo != sLstFileNo)
        {
            if (j == 0)
            {
                ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                             SM_TRC_MRECOVERY_RECOVERYMGR_CHKP16);
            }
            else
            {
                /* nothing to do ... */
            }

            j++;
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_MRECOVERY_RECOVERYMGR_CHKP17,
                         sFstFileNo,
                         sLstFileNo - 1 );
            (void)smrLogMgr::getLogFileMgr().removeLogFile( sFstFileNo,
                                                            sLstFileNo,
                                                            ID_TRUE );

            // fix BUG-20241 : 삭제한 LogFileNo와 LogAnchor 동기화
            IDE_TEST( mAnchorMgr.updateFstDeleteFileAndFlush()
                      != IDE_SUCCESS );

        }
        else
        {
            /* nothing to do ... */
        }

        if ( j == 0 )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                         SM_TRC_MRECOVERY_RECOVERYMGR_CHKP18 );
        }

        sState = 0;
        IDE_TEST( smrRecoveryMgr::unlockDeleteLogFileMtx()
                  != IDE_SUCCESS );
    }
    else
    {

        ideLog::log( SM_TRC_LOG_LEVEL_MRECOV,
                     SM_TRC_MRECOVERY_RECOVERYMGR_CHKP19 );
    }

    IDE_EXCEPTION_CONT( SKIP_REMOVE_LOGFILE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sState != 0 )
        {
            IDE_ASSERT( smrRecoveryMgr::unlockDeleteLogFileMtx()
                        == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*
  미디어 오류가 있는 메모리테이블스페이스에 대한
  PREPARE 및 LOADING을 처리한다.

*/
IDE_RC smrRecoveryMgr::initMediaRecovery4MemTBS()
{
    // free page에 대한 page memory 할당을 하지 않기 때문에
    // redo 하다가 smmManager::getPersPagePtr 에서 page에
    // 접근하게 되면 recovery 과정일때
    // 즉시 page memory 를 할당할 수 있게 해주어야 한다.

    IDE_CALLBACK_SEND_MSG("  [ RECMGR ] Restoring corrupted memory tablespace checkpoint images");
    IDE_TEST( smmManager::prepareDB( SMM_PREPARE_OP_DBIMAGE_NEED_MEDIA_RECOVERY )
              != IDE_SUCCESS );

    IDE_CALLBACK_SEND_MSG("  [ RECMGR ] Loading memory tablespace checkpoint images from backup media ");
    IDE_TEST( smmManager::restoreDB( SMM_RESTORE_OP_DBIMAGE_NEED_MEDIA_RECOVERY )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  미디어 오류가 있는 메모리테이블스페이스에 대한
  PREPARE 및 LOADING을 처리한다.
*/
IDE_RC smrRecoveryMgr::finalMediaRecovery4MemTBS()
{
    IDE_CALLBACK_SEND_MSG(
        "  [ RECMGR ] Memory tablespace checkpoint image restoration complete. ");

    //BUG-34530 
    //SYS_TBS_MEM_DIC테이블스페이스 메모리가 해제되더라도
    //DicMemBase포인터가 NULL로 초기화 되지 않습니다.
    smmManager::clearCatalogPointers();

    IDE_TEST( smmTBSMediaRecovery::resetMediaFailureMemTBSNodes() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  미디어복구 완료시 메모리 DirtyPage를 모두 Flush 시킨다.
*/
IDE_RC smrRecoveryMgr::flushAndRemoveDirtyPagesAllMemTBS()
{
    ULong              sNewDirtyPageCnt;
    ULong              sDupDirtyPageCnt;
    ULong              sTotalCnt;
    ULong              sRemoveCnt;
    ULong              sWaitTime;
    ULong              sSyncTime;

    SChar              sMsgBuf[ SM_MAX_FILE_NAME ];

    // 1) 모든 Tablespace에 대해 SMM => SMR 로 Dirty Page이동
    IDE_TEST( smrDPListMgr::moveDirtyPages4AllTBS( SCT_SS_UNABLE_MEDIA_RECOVERY,
                                                   & sNewDirtyPageCnt,
                                                   & sDupDirtyPageCnt )
              != IDE_SUCCESS );

    // 2) Dirty Page들을 Disk의 Checkpoint Image로 Flush한다.
    // 단, Media Recovery 완료후이므로 PID로깅 안함
    IDE_TEST( smrDPListMgr::writeDirtyPages4AllTBS( SCT_SS_UNABLE_MEDIA_RECOVERY,
                                                    & sTotalCnt,
                                                    & sRemoveCnt,
                                                    & sWaitTime,
                                                    & sSyncTime,
                                                    smmManager::getCurrentDB, // flush target db
                                                    SMR_WDP_NO_PID_LOGGING )
              != IDE_SUCCESS );

    IDE_ASSERT( sRemoveCnt == 0 );

    idlOS::snprintf( sMsgBuf, SM_MAX_FILE_NAME,
                     "\n             Flush All Memory ( %d ) Dirty Pages.",
                     sTotalCnt );

    IDE_CALLBACK_SEND_SYM( sMsgBuf );

    // 3) 모든 dirty page들을 discard 시킨다.
    // 생성된 tablespace의 dirty page list는reset tablespace에서 처리한다.
    IDE_TEST( smrDPListMgr::discardDirtyPages4AllTBS() != IDE_SUCCESS );

    // 4) sync db file
    // 운영중일 경우에는 모든 Online Tablespace에 대해서 Sync를 수행한다.
    // 미디어복구중에는 모든 Online/Offline TableSpace 중 미디어 복구
    // 대상인 것만 Sync를 수행한다.
    IDE_TEST( smmManager::syncDB( SCT_SS_UNABLE_MEDIA_RECOVERY,
                                  ID_TRUE /* syncLatch 획득 필요 */ )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
  메모리 데이타파일의 헤더를 복구한다

  [IN] aResetLogsLSN - 불완전복구시 ResetLogsLSN
*/
IDE_RC smrRecoveryMgr::repairFailureChkptImageHdr( smLSN  * aResetLogsLSN )
{
    sctActRepairArgs   sRepairArgs;

    sRepairArgs.mResetLogsLSN = aResetLogsLSN;

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS( NULL, /* idvSQL* */
                                                  smmTBSMediaRecovery::doActRepairDBFHdr,
                                                  &sRepairArgs, /* Action Argument*/
                                                  SCT_ACT_MODE_NONE ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description  : 로그파일의 크기를 인자로 받은 크기만큼 재조정하는 함수
 *
 * aLogFileName - [IN] 크기를 재조정할 로그파일 이름
 * aLogFileSize - [IN] 재조정할 크기
 **********************************************************************/

IDE_RC smrRecoveryMgr::resizeLogFile(SChar    *aLogFileName,
                                     ULong     aLogFileSize)
{
#define                 SMR_NULL_BUFSIZE   (1024*1024)    // 1M

    iduFile             sFile;
    SChar             * sNullBuf      = NULL;
    ULong               sCurFileSize  = 0;
    ULong               sOffset       = 0;
    UInt                sWriteSize    = 0;
    UInt                sState        = 0;

    // Null buffer 할당
    /* smrRecoveryMgr_resizeLogFile_malloc_NullBuf.tc */
    IDU_FIT_POINT("smrRecoveryMgr::resizeLogFile::malloc::NullBuf");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                 SMR_NULL_BUFSIZE,
                                 (void**)&sNullBuf,
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );
    sState = 1;

    idlOS::memset(sNullBuf, 0x00, SMR_NULL_BUFSIZE);

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sFile.setFileName(aLogFileName) != IDE_SUCCESS );
    IDE_TEST( sFile.open() != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( sFile.getFileSize(&sCurFileSize) != IDE_SUCCESS );
    sOffset = sCurFileSize;

    while(sOffset < aLogFileSize)
    {
        if ( (aLogFileSize - sOffset) < SMR_NULL_BUFSIZE )
        {
            sWriteSize = aLogFileSize - sOffset;
        }
        else
        {
            sWriteSize = SMR_NULL_BUFSIZE;
        }

        IDE_TEST( sFile.write( NULL, /* idvSQL* */
                               sOffset,
                               sNullBuf,
                               sWriteSize )
                  != IDE_SUCCESS );

        sOffset += sWriteSize;
    }

    IDE_ASSERT(sOffset == aLogFileSize);

    IDE_TEST( sFile.getFileSize(&sCurFileSize) != IDE_SUCCESS );
    IDE_ASSERT(sCurFileSize == aLogFileSize);

    sState = 2;
    IDE_TEST( sFile.close() != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free((void**)sNullBuf) != IDE_SUCCESS );
    sNullBuf = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 3:
            IDE_ASSERT(sFile.close() == IDE_SUCCESS );

        case 2:
            IDE_ASSERT(sFile.destroy() == IDE_SUCCESS );

        case 1:
            IDE_ASSERT(iduMemMgr::free((void**)sNullBuf) == IDE_SUCCESS );

        default:
            break;
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * 마지막으로 checkpoint 된 지 오래되었다면, restart 시간을 줄이고, 로그를
 * 줄이기 위해 checkpoint를 수행한다.
 * 이 함수는 로그양이 사용자가 지정한 양 보다 많이 남아 있는 경우에 checkpoint
 * ID_TRUE를 리턴함으로 해서 체크포인트가 수행되게끔 한다.
 ******************************************************************************/
idBool smrRecoveryMgr::isCheckpointFlushNeeded(smLSN aLastWrittenLSN)
{
    smLSN  sEndLSN;
    idBool sResult = ID_FALSE;
    
    SM_LSN_MAX ( sEndLSN );

    if ( smrLogMgr::isAvailable() == ID_TRUE )
    {
        smrLogMgr::getLstLSN( &sEndLSN );

        if ( ( sEndLSN.mFileNo - aLastWrittenLSN.mFileNo ) >= 
             smuProperty::getCheckpointFlushMaxGap() )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    else
    {
        /* nothing to do ... */
    }

    return sResult;
}

/*
  fix BUG-17157 [PROJ-1548] Disk Tablespace Online/Offline
  수행시 올바르게 Index Runtime Header 해제하지 않음

  Restart REDO, UNDO끝난 후에 Offline 상태인 Disk Tablespace의
  Index Runtime Header 해제

  Restart REDOAll이 끝나면, Dropped/Discarded TBS에 포함된
  Table의 Runtime Entry와 Table의 Runitm Index Header를 빌드한다.
  그외에 예외적인 경우가 Offline 테이블스페이스인데
  Offline이 완료가 되었다면 Restart Recovery를 수행하여도
  Offline TBS와 관련하여 Undo가 발생하지 않기 때문에
  굳이 Refine DRDB Tables 과정에서 Offline TBS에 포함된
  Table들의 Runtime Index Header들을 빌드하지 않아도 된다.
  ( 그이유는 명백하다. ) 하지만 현재, SKIP_UNDO 에서는
  Offline TBS를 skip 하지 않기 때문에 Refine DRDB 단계에서
  빌드하고 있다.

  그러므로 UndoAll과정이 완료된 이후에 Offline TBS에 포함된
  Table의 Runtime Index Header를 free 시킨다.

 */
IDE_RC smrRecoveryMgr::finiOfflineTBS( idvSQL* aStatistics )
{
    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                                aStatistics,
                                sddDiskMgr::finiOfflineTBSAction,
                                NULL, /* Action Argument*/
                                SCT_ACT_MODE_NONE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2118 BUG Reporting
 *               Server Fatal 시점에 Signal Handler 가 호출할
 *               Debugging 정보 기록함수
 *
 *               이미 altibase_dump.log 에 lock을 잡고 들어오므로
 *               lock을 잡지않는 trace 기록 함수를 사용해야 한다.
 *
 **********************************************************************/
void smrRecoveryMgr::writeDebugInfo()
{
    if ( ( isRestartRecoveryPhase() == ID_TRUE ) ||
        ( isMediaRecoveryPhase()   == ID_TRUE ) )
    {
        ideLog::log( IDE_DUMP_0,
                     "====================================================\n"
                     " Storage Manager Dump Info for Recovery\n"
                     "====================================================\n"
                     "isRestartRecovery : %"ID_UINT32_FMT"\n"
                     "isMediaRecovery   : %"ID_UINT32_FMT"\n"
                     "LstRedoLSN        : [ %"ID_UINT32_FMT" , %"ID_UINT32_FMT" ]\n"
                     "LstUndoLSN        : [ %"ID_UINT32_FMT" , %"ID_UINT32_FMT" ]\n"
                     "====================================================\n",
                     isRestartRecoveryPhase(),
                     isMediaRecoveryPhase(),
                     mLstRedoLSN.mFileNo,
                     mLstRedoLSN.mOffset,
                     mLstUndoLSN.mFileNo,
                     mLstUndoLSN.mOffset );

        if ( ( mCurLogPtr     != NULL ) &&
             ( mCurLogHeadPtr != NULL ) )
        {
            ideLog::log( IDE_DUMP_0,
                         "[ Cur Log Info ]\n"
                         "Log Flag    : %"ID_UINT32_FMT"\n"
                         "Log Type    : %"ID_UINT32_FMT"\n"
                         "Magic Num   : %"ID_UINT32_FMT"\n"
                         "Log Size    : %"ID_UINT32_FMT"\n"
                         "PrevUndoLSN [ %"ID_UINT32_FMT", %"ID_UINT32_FMT" ]\n"
                         "TransID     : %"ID_UINT32_FMT"\n"
                         "ReplSPNum   : %"ID_UINT32_FMT"\n",
                         mCurLogHeadPtr->mFlag,
                         mCurLogHeadPtr->mType,
                         mCurLogHeadPtr->mMagic,
                         mCurLogHeadPtr->mSize,
                         mCurLogHeadPtr->mPrevUndoLSN.mFileNo,
                         mCurLogHeadPtr->mPrevUndoLSN.mOffset,
                         mCurLogHeadPtr->mTransID,
                         mCurLogHeadPtr->mReplSvPNumber );

            ideLog::logMem( IDE_DUMP_0,
                            (UChar*)mCurLogHeadPtr,
                            ID_SIZEOF( smrLogHead ),
                            " Cur Log Header :\n" );

            if ( mCurLogHeadPtr->mSize > 0 )
            {
                ideLog::logMem( IDE_DUMP_0,
                                (UChar*)mCurLogPtr,
                                mCurLogHeadPtr->mSize,
                                " Cur Log Data :\n" );
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    }
}


/**********************************************************************
 * PROJ-2162 RestartRiskReduction
 ***********************************************************************/

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * TOI 객체를 초기화합니다.
 *
 * aObj         - [OUT] 초기화할 객체
 ***********************************************************************/
void smrRecoveryMgr::initRTOI( smrRTOI * aObj )
{
    IDE_DASSERT( aObj != NULL );

    SM_LSN_INIT( aObj->mCauseLSN );
    SC_MAKE_NULL_GRID( aObj->mGRID );
    aObj->mCauseDiskPage  = NULL;
    aObj->mCause          = SMR_RTOI_CAUSE_INIT;
    aObj->mType           = SMR_RTOI_TYPE_INIT;
    aObj->mState          = SMR_RTOI_STATE_INIT;
    aObj->mTableOID       = SM_NULL_OID;
    aObj->mIndexID        = 0;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * Log를 받아 분석하여 이 Log가 Recovery할 대상 객체들을 추출합니다.
 *
 * DML로그는 Page에 대해 Inconsistent함을 설정하고,
 * DDL은 Table/Index에 대해 Inconsistent함을 설정합니다.
 *
 * aLogPtr      - [IN]  추출하게된 Log ( Align 안됨 )
 * aLogHeadPtr  - [IN]  추출하게된 Log의 Head ( Align되어있음 )
 * aLSN         - [IN]  추출하게된 Log의 LSN
 * aRedoLogData - [IN]  DRDB Redo 중이었을 경우, 추출된 redoLog가 날라옴
 * aPagePtr     - [IN]  DRDB Redo 중이었을 경우, 대상 Page가 옴
 * aIsRedo      - [IN]  Redo중인가?
 * aObj         - [OUT] 설정된 결과물
 ***********************************************************************/
void smrRecoveryMgr::prepareRTOI( void                * aLogPtr,
                                  smrLogHead          * aLogHeadPtr,
                                  smLSN               * aLSN,
                                  sdrRedoLogData      * aRedoLogData,
                                  UChar               * aPagePtr,
                                  idBool                aIsRedo,
                                  smrRTOI             * aObj )
{
    smrUpdateType        sType;
    smrOPType            sOPType;
    ULong                sData1;
    UInt                 sRefOffset;

    IDE_DASSERT( aLSN != NULL );
    IDE_DASSERT( aObj != NULL );
    IDE_DASSERT( ( ( aLogHeadPtr == NULL ) && ( aLogPtr == NULL ) ) ||
                 ( ( aLogHeadPtr != NULL ) && ( aLogPtr != NULL ) ) );

    /* 초기화 */
    initRTOI( aObj );

    /* smrRecoveryMgr::redo에서 Log분석시 호출하는 경우 */
    /* LogHeadPtr이 설정되서 온다. */
    if ( ( aLogHeadPtr != NULL ) && ( aLogPtr != NULL ) )
    {
        switch( smrLogHeadI::getType(aLogHeadPtr) )
        {
        case SMR_LT_UPDATE:
            /* MMDB DML, DDL 연산.*/
            idlOS::memcpy(
                &(aObj->mGRID),
                ( (SChar*) aLogPtr) + offsetof( smrUpdateLog, mGRID ),
                ID_SIZEOF( scGRID ) );
            idlOS::memcpy(
                &sType,
                ( (SChar*) aLogPtr) + offsetof( smrUpdateLog, mType ),
                ID_SIZEOF( smrUpdateType ) );


            /* Page 초기화, Table초기화등 Consistency Check를 하면 안되는 경우
             * 가 있다. 이러한 경우는 다음과 같다.
             *
             * Case 1) 초기화 Log( OverwriteLog포함) 등 기존에 해당 객체가
             *    어떤 상태인지 무관한 경우
             * -> Check할 필요도 없고, Recovery실패하더라도 inconsistentFlag
             *  설정 안함. Object가 잘못되었기에 Inconsistent설정 조차
             *  위험할 수 있음. ( DONE으로 바로 감 )
             * Case 2) 1과 비슷하지만, ConsistencyFlag 설정은 괜찮은 경우
             *    ( CHECKED로 바로 감 )
             * Case 3) Inconsistency Flag등을 설정하기 위한 Log일 경우
             *    ( DONE 상태로 감. )
             * Case 4) Log만으로는 TargetObject를 알 수 없는 경우
             *    ( DONE 상태로 감. )
             * 이 경우에 대해서는 체크를 하지 않는다. */
            switch( sType )
            {
            case SMR_SMC_TABLEHEADER_INIT:              /* Case 1 */
            case SMR_SMM_MEMBASE_SET_SYSTEM_SCN:
            case SMR_SMM_MEMBASE_ALLOC_PERS_LIST:
            case SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK:
            case SMR_SMM_PERS_UPDATE_LINK:
            case SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK:
            case SMR_SMM_MEMBASE_INFO:
                aObj->mState = SMR_RTOI_STATE_DONE;
                break;
            case SMR_SMC_PERS_INIT_FIXED_PAGE:          /* Case 2*/
            case SMR_SMC_PERS_INIT_VAR_PAGE:
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
                aObj->mType           = SMR_RTOI_TYPE_MEMPAGE;
                aObj->mState          = SMR_RTOI_STATE_CHECKED;
                break;
            case SMR_SMC_PERS_SET_INCONSISTENCY:
            case SMR_SMC_TABLEHEADER_SET_INCONSISTENCY: /* Case 3 */
                aObj->mState = SMR_RTOI_STATE_DONE;
                break;
            case SMR_PHYSICAL:
            case SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO:
            case SMR_SMC_TABLEHEADER_UPDATE_INDEX:      /* Case 4 */
                /* Index Create/Drop시 수행되는 Physical Log
                 * 연산되는 Index에 대한 정보를 획득할 수 없고,
                 * index 존재 유무와 관련된 연산의 실패다보니
                 * Inconsistent를 설정할 객체가 없음. 따라서 검사 안함*/
                aObj->mState = SMR_RTOI_STATE_DONE;
                break;
            case SMR_SMC_TABLEHEADER_UPDATE_COLUMNS:
            case SMR_SMC_TABLEHEADER_UPDATE_INFO:
            case SMR_SMC_TABLEHEADER_SET_NULLROW:
            case SMR_SMC_TABLEHEADER_UPDATE_ALL:
            case SMR_SMC_TABLEHEADER_UPDATE_FLAG:
            case SMR_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT:
            case SMR_SMC_TABLEHEADER_SET_SEQUENCE:
            case SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT:
            case SMR_SMC_TABLEHEADER_SET_SEGSTOATTR:
            case SMR_SMC_TABLEHEADER_SET_INSERTLIMIT:
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
                aObj->mType  = SMR_RTOI_TYPE_TABLE;
                aObj->mState = SMR_RTOI_STATE_PREPARED;
                break;
            case SMR_SMC_INDEX_SET_FLAG:
            case SMR_SMC_INDEX_SET_SEGATTR:
            case SMR_SMC_INDEX_SET_SEGSTOATTR:
            case SMR_SMC_INDEX_SET_DROP_FLAG:
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
                aObj->mType  = SMR_RTOI_TYPE_INDEX;
                aObj->mState = SMR_RTOI_STATE_PREPARED;
                break;
            case SMR_SMC_PERS_INIT_FIXED_ROW:
            case SMR_SMC_PERS_UPDATE_FIXED_ROW:
            case SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION:
            case SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG:
            case SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT:
            case SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD:
            case SMR_SMC_PERS_UPDATE_VAR_ROW:
            case SMR_SMC_PERS_SET_VAR_ROW_FLAG:
            case SMR_SMC_PERS_SET_VAR_ROW_NXT_OID:
            case SMR_SMC_PERS_WRITE_LOB_PIECE:
            case SMR_SMC_PERS_INSERT_ROW:
            case SMR_SMC_PERS_UPDATE_INPLACE_ROW:
            case SMR_SMC_PERS_UPDATE_VERSION_ROW:
            case SMR_SMC_PERS_DELETE_VERSION_ROW:
            /* PROJ-2429 */
            case SMR_SMC_SET_CREATE_SCN:
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
                aObj->mType  = SMR_RTOI_TYPE_MEMPAGE;
                aObj->mState = SMR_RTOI_STATE_PREPARED;
                break;
            default:
                IDE_DASSERT( 0 );
                break;
            }
            break;
        case SMR_LT_NTA:
            /* MMDB DDL, AllocSlot 연산.*/
            idlOS::memcpy(
                &sData1,
                ( (SChar*) aLogPtr) + offsetof( smrNTALog, mData1 ),
                ID_SIZEOF( ULong ) );
            idlOS::memcpy(
                &sOPType,
                ( (SChar*) aLogPtr) + offsetof( smrNTALog, mOPType ),
                ID_SIZEOF( smrOPType ) );

            switch( sOPType )
            {
            case SMR_OP_NULL:
            case SMR_OP_SMC_TABLEHEADER_ALLOC:           /* Case 2 */
            case SMR_OP_CREATE_TABLE:
                SC_MAKE_GRID( aObj->mGRID,
                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                              SM_MAKE_PID( sData1 ),
                              SM_MAKE_OFFSET( sData1 ) );
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
                aObj->mType  = SMR_RTOI_TYPE_TABLE;
                aObj->mState = SMR_RTOI_STATE_CHECKED;
                break;
            case SMR_OP_SMM_PERS_LIST_ALLOC:
            case SMR_OP_SMC_FIXED_SLOT_ALLOC:
            case SMR_OP_CREATE_INDEX:
            case SMR_OP_DROP_INDEX:
            case SMR_OP_INIT_INDEX:
            case SMR_OP_SMC_FIXED_SLOT_FREE:
            case SMR_OP_SMC_VAR_SLOT_FREE:
            case SMR_OP_ALTER_TABLE:
            case SMR_OP_SMM_CREATE_TBS:
            case SMR_OP_INSTANT_AGING_AT_ALTER_TABLE: /* Case 3 */
            case SMR_OP_DIRECT_PATH_INSERT:
                aObj->mState = SMR_RTOI_STATE_DONE;
                break;
            default:
                IDE_DASSERT( 0 );
                break;
            }
            break;
        case SMR_DLT_UNDOABLE:
            if ( aIsRedo == ID_TRUE )
            {
                /* DRDB Redo에 대해서는 smrRecoveryMgr::redo가 아니라
                 * sdrRedoMgr::applyLogRec에서 수행한다. 여기서 하면
                 * 중복 수행이 되버린다.
                 * 다만 Undo는 sdrRedoMgr등 DRDB전용이 아니라 공통으로
                 * 수행하기 때문에, Undo용 로그는 여기서 분석한다. */
                aObj->mState = SMR_RTOI_STATE_DONE;
            }
            else
            {
                idlOS::memcpy( &sRefOffset,
                               ( (SChar*) aLogPtr) +
                               offsetof( smrDiskLog, mRefOffset ),
                               ID_SIZEOF( UInt ) );
                idlOS::memcpy( &aObj->mGRID,
                               ( (SChar*) aLogPtr) +
                               SMR_LOGREC_SIZE(smrDiskLog) +
                               sRefOffset +
                               offsetof( sdrLogHdr,mGRID ),
                               ID_SIZEOF( scGRID ) );
                aObj->mType  = SMR_RTOI_TYPE_DISKPAGE;
                aObj->mState = SMR_RTOI_STATE_PREPARED;
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
            }
            break;
        case SMR_DLT_REF_NTA:
            if ( aIsRedo == ID_TRUE )
            {
                /* 위 SMR_DLT_UNDOABLE과 같은 이유로 무시함 */
                aObj->mState = SMR_RTOI_STATE_DONE;
            }
            else
            {
                idlOS::memcpy( &sRefOffset,
                               ( (SChar*) aLogPtr) +
                               offsetof( smrDiskRefNTALog, mRefOffset ),
                               ID_SIZEOF( UInt ) );
                idlOS::memcpy( &aObj->mGRID,
                               ( (SChar*) aLogPtr) +
                               SMR_LOGREC_SIZE(smrDiskRefNTALog) +
                               sRefOffset +
                               offsetof( sdrLogHdr,mGRID ),
                               ID_SIZEOF( scGRID ) );
                aObj->mType  = SMR_RTOI_TYPE_DISKPAGE;
                aObj->mState = SMR_RTOI_STATE_PREPARED;
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
            }
            break;
        case SMR_LT_COMPENSATION:
        case SMR_LT_NULL:
        case SMR_LT_DUMMY:
        case SMR_LT_CHKPT_BEGIN:
        case SMR_LT_DIRTY_PAGE:
        case SMR_LT_CHKPT_END:
        case SMR_LT_MEMTRANS_GROUPCOMMIT:
        case SMR_LT_MEMTRANS_COMMIT:
        case SMR_LT_MEMTRANS_ABORT:
        case SMR_LT_DSKTRANS_COMMIT:
        case SMR_LT_DSKTRANS_ABORT:
        case SMR_LT_SAVEPOINT_SET:
        case SMR_LT_SAVEPOINT_ABORT:
        case SMR_LT_XA_PREPARE:
        case SMR_LT_TRANS_PREABORT:
        case SMR_LT_DDL:
        case SMR_LT_XA_SEGS:
        case SMR_LT_LOB_FOR_REPL:
        case SMR_LT_DDL_QUERY_STRING:
        case SMR_LT_DUMMY_COMPENSATION:
        case SMR_LT_FILE_BEGIN:
        case SMR_LT_TBS_UPDATE:
        case SMR_LT_FILE_END:
        case SMR_DLT_REDOONLY:
        case SMR_DLT_NTA:
        case SMR_DLT_COMPENSATION:
        case SMR_LT_TABLE_META:
        case SMR_LT_XA_START_REQ:
        case SMR_LT_XA_PREPARE_REQ:
        case SMR_LT_XA_END:
            aObj->mState = SMR_RTOI_STATE_DONE;
            break;
        default:
            IDE_DASSERT( 0 );
            break;
        }
    }
    else
    {
        /* nothing to do ... */
    }

    /* DRDB에 대해 분석하는 경우 */
    if ( aRedoLogData != NULL )
    {
        IDE_DASSERT( aPagePtr != NULL );

        if ( aPagePtr != NULL )
        {
            aObj->mCauseDiskPage  = aPagePtr;

            IDE_DASSERT( (aRedoLogData->mOffset == SC_NULL_OFFSET) ||
                         (aRedoLogData->mSlotNum == SC_NULL_SLOTNUM) );

            if ( aRedoLogData->mOffset == SC_NULL_OFFSET )
            {
                SC_MAKE_GRID_WITH_SLOTNUM( aObj->mGRID,
                                           aRedoLogData->mSpaceID,
                                           aRedoLogData->mPageID,
                                           aRedoLogData->mSlotNum );
            }
            else
            {
                SC_MAKE_GRID( aObj->mGRID,
                              aRedoLogData->mSpaceID,
                              aRedoLogData->mPageID,
                              aRedoLogData->mOffset );
            }

            /* 과거 sdrRedoMgr::checkByDiskLog와 같은 기능  */
            /* Page단위 Physical Log이나 InitPage Log등은 PageConsistentFlag
             * 와 상관없다. */
            switch( aRedoLogData->mType )
            {
            case SDR_SDP_BINARY:                  /* Case 1*/
            case SDR_SDP_PAGE_CONSISTENT:
            case SDR_SDP_INIT_PHYSICAL_PAGE:
            case SDR_SDP_WRITE_PAGEIMG:
            case SDR_SDP_WRITE_DPATH_INS_PAGE:
                aObj->mState = SMR_RTOI_STATE_DONE;
                break;
            case SDR_SDP_1BYTE:
            case SDR_SDP_2BYTE:
            case SDR_SDP_4BYTE:
            case SDR_SDP_8BYTE:
            case SDR_SDP_INIT_LOGICAL_HDR:
            case SDR_SDP_INIT_SLOT_DIRECTORY:
            case SDR_SDP_FREE_SLOT:
            case SDR_SDP_FREE_SLOT_FOR_SID:
            case SDR_SDP_RESTORE_FREESPACE_CREDIT:
            case SDR_SDP_RESET_PAGE:
            case SDR_SDPST_INIT_SEGHDR:
            case SDR_SDPST_INIT_BMP:
            case SDR_SDPST_INIT_LFBMP:
            case SDR_SDPST_INIT_EXTDIR:
            case SDR_SDPST_ADD_RANGESLOT:
            case SDR_SDPST_ADD_SLOTS:
            case SDR_SDPST_ADD_EXTDESC:
            case SDR_SDPST_ADD_EXT_TO_SEGHDR:
            case SDR_SDPST_UPDATE_WM:
            case SDR_SDPST_UPDATE_MFNL:
            case SDR_SDPST_UPDATE_PBS:
            case SDR_SDPST_UPDATE_LFBMP_4DPATH:
            case SDR_SDPSC_INIT_SEGHDR:
            case SDR_SDPSC_INIT_EXTDIR:
            case SDR_SDPSC_ADD_EXTDESC_TO_EXTDIR:
            case SDR_SDPTB_INIT_LGHDR_PAGE:
            case SDR_SDPTB_ALLOC_IN_LG:
            case SDR_SDPTB_FREE_IN_LG:
            case SDR_SDC_INSERT_ROW_PIECE:
            case SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE:
            case SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO:
            case SDR_SDC_UPDATE_ROW_PIECE:
            case SDR_SDC_OVERWRITE_ROW_PIECE:
            case SDR_SDC_CHANGE_ROW_PIECE_LINK:
            case SDR_SDC_DELETE_FIRST_COLUMN_PIECE:
            case SDR_SDC_ADD_FIRST_COLUMN_PIECE:
            case SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE:
            case SDR_SDC_DELETE_ROW_PIECE:
            case SDR_SDC_LOCK_ROW:
            case SDR_SDC_PK_LOG:
            case SDR_SDC_INIT_CTL:
            case SDR_SDC_EXTEND_CTL:
            case SDR_SDC_BIND_CTS:
            case SDR_SDC_UNBIND_CTS:
            case SDR_SDC_BIND_ROW:
            case SDR_SDC_UNBIND_ROW:
            case SDR_SDC_ROW_TIMESTAMPING:
            case SDR_SDC_DATA_SELFAGING:
            case SDR_SDC_BIND_TSS:
            case SDR_SDC_UNBIND_TSS:
            case SDR_SDC_SET_INITSCN_TO_TSS:
            case SDR_SDC_INIT_TSS_PAGE:
            case SDR_SDC_INIT_UNDO_PAGE:
            case SDR_SDC_INSERT_UNDO_REC:
            case SDR_SDN_INSERT_INDEX_KEY:
            case SDR_SDN_FREE_INDEX_KEY:
            case SDR_SDN_INSERT_UNIQUE_KEY:
            case SDR_SDN_INSERT_DUP_KEY:
            case SDR_SDN_DELETE_KEY_WITH_NTA:
            case SDR_SDN_FREE_KEYS:
            case SDR_SDN_COMPACT_INDEX_PAGE:
            case SDR_SDN_KEY_STAMPING:
            case SDR_SDN_INIT_CTL:
            case SDR_SDN_EXTEND_CTL:
            case SDR_SDN_FREE_CTS:
            case SDR_SDC_LOB_UPDATE_LOBDESC:
            case SDR_SDC_LOB_INSERT_INTERNAL_KEY:
            case SDR_SDC_LOB_INSERT_LEAF_KEY:
            case SDR_SDC_LOB_UPDATE_LEAF_KEY:
            case SDR_SDC_LOB_OVERWRITE_LEAF_KEY:
            case SDR_SDC_LOB_FREE_INTERNAL_KEY:
            case SDR_SDC_LOB_FREE_LEAF_KEY:
            case SDR_SDC_LOB_WRITE_PIECE:
            case SDR_SDC_LOB_WRITE_PIECE4DML:
            case SDR_SDC_LOB_WRITE_PIECE_PREV:
            case SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST:
            case SDR_STNDR_INSERT_INDEX_KEY:
            case SDR_STNDR_UPDATE_INDEX_KEY:
            case SDR_STNDR_FREE_INDEX_KEY:
            case SDR_STNDR_INSERT_KEY:
            case SDR_STNDR_DELETE_KEY_WITH_NTA:
            case SDR_STNDR_FREE_KEYS:
            case SDR_STNDR_COMPACT_INDEX_PAGE:
            case SDR_STNDR_KEY_STAMPING:
                SM_GET_LSN( aObj->mCauseLSN, *aLSN );
                aObj->mType  = SMR_RTOI_TYPE_DISKPAGE;
                aObj->mState = SMR_RTOI_STATE_PREPARED;
                break;
            default:
                IDE_ASSERT( 0 );
                break;
            }
        }
    }

    IDE_DASSERT( aObj->mState != SMR_RTOI_STATE_INIT );
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * TargetObject를 받아 Inconsistent한지 체크.
 *
 * aObj         - [IN] 검사할 대상
 * aConsistency - [OUT] 검사 결과
 ***********************************************************************/
void smrRecoveryMgr::checkObjectConsistency( smrRTOI * aObj,
                                             idBool  * aConsistency )
{
    idBool              sConsistency;

    IDE_DASSERT( aObj         != NULL );
    IDE_DASSERT( aConsistency != NULL );

    sConsistency = ID_TRUE;

    /* 검사할 필요가 있는가? */
    if ( ( aObj->mState == SMR_RTOI_STATE_PREPARED ) &&
         ( isSkipRedo( aObj->mGRID.mSpaceID, ID_TRUE ) == ID_FALSE ) )
    {
        aObj->mCause = SMR_RTOI_CAUSE_INIT;

        /* Property에 의해 해당 Log, Table, Index Page는 제외함 */
        if ( isIgnoreObjectByProperty( aObj ) == ID_TRUE )
        {
            aObj->mCause  = SMR_RTOI_CAUSE_PROPERTY;
            sConsistency  = ID_FALSE;

            if ( findIOL( aObj ) == ID_TRUE )
            {
                /* 이미 설정된 경우 */
                aObj->mState = SMR_RTOI_STATE_DONE;
            }
            else
            {
                /* 아직 등록 안되었음 */
                aObj->mState  = SMR_RTOI_STATE_CHECKED;
            }
        }
        else
        {
            sConsistency = checkObjectConsistencyInternal( aObj );
        }
    }

    (*aConsistency) = sConsistency;

    return;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * Property에서 무시하라고 설정된 객체인지 확인합니다.
 *
 * aObj         - [IN] 검사할 대상
 ***********************************************************************/
idBool smrRecoveryMgr::isIgnoreObjectByProperty( smrRTOI * aObj )
{
    scSpaceID           sSpaceID;
    scPageID            sPageID;
    UInt                i;

    sSpaceID = aObj->mGRID.mSpaceID;
    sPageID  = aObj->mGRID.mPageID;

    if ( !SM_IS_LSN_INIT( aObj->mCauseLSN ) )
    {
        for ( i = 0 ; i < smuProperty::getSmIgnoreLog4EmergencyCount() ; i ++)
        {
            if ( ( aObj->mCauseLSN.mFileNo == smuProperty::getSmIgnoreFileNo4Emergency(i) ) &&
                 ( aObj->mCauseLSN.mOffset == smuProperty::getSmIgnoreOffset4Emergency(i) ) )
            {
                return ID_TRUE;
            }
        }
    }

    if ( aObj->mTableOID != SM_NULL_OID )
    {
        for( i = 0 ; i < smuProperty::getSmIgnoreTable4EmergencyCount() ; i ++)
        {
            if ( aObj->mTableOID == smuProperty::getSmIgnoreTable4Emergency(i) )
            {
                return ID_TRUE;
            }
        }
    }

    if ( aObj->mIndexID != 0 )
    {
        for( i = 0 ; i < smuProperty::getSmIgnoreIndex4EmergencyCount() ; i ++)
        {
            if ( aObj->mIndexID == smuProperty::getSmIgnoreIndex4Emergency(i) )
            {
                return ID_TRUE;
            }
        }
    }
    if ( ( sSpaceID != 0 ) || ( sPageID != 0 ) )
    {
        for( i = 0 ; i < smuProperty::getSmIgnorePage4EmergencyCount() ; i ++)
        {
            if ( ( ( (ULong)sSpaceID << 32 ) + sPageID ==
                  smuProperty::getSmIgnorePage4Emergency( i ) ) )
            {
                return ID_TRUE;
            }
        }
    }

    return ID_FALSE;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * TargetObject가 Inconsistency 한지 체크하는 실제 함수.
 *   Object에 Inconsistent Flag가 설정된 경우에는, 또 설정할 필요가
 * 없기 때문에 Done상태로 간다.
 *   하지만 Object가 이상할 경우, Check해야 한다.
 *
 * aObj        - [IN] 검사할 대상
 ***********************************************************************/
idBool smrRecoveryMgr::checkObjectConsistencyInternal( smrRTOI * aObj )
{
    void              * sTable;
    void              * sIndex;
    UChar             * sDiskPagePtr;
    smpPersPageHeader * sMemPagePtr;
    scSpaceID           sSpaceID;
    scPageID            sPageID;
    smOID               sOID;
    smLSN               sPageLSN;
    idBool              sTrySuccess;
    idBool              sGetPage = ID_FALSE;
    smmTBSNode        * sTBSNode = NULL;
    idBool              sIsFreePage;

    /* 직접 객체에 접근하여 확인함 */
    sSpaceID = aObj->mGRID.mSpaceID;
    sPageID  = aObj->mGRID.mPageID;
    sOID     = SM_MAKE_OID( aObj->mGRID.mPageID,
                            aObj->mGRID.mOffset );

    switch( aObj->mType )
    {
    case SMR_RTOI_TYPE_TABLE:
    case SMR_RTOI_TYPE_INDEX:
    case SMR_RTOI_TYPE_MEMPAGE:
        IDE_TEST_RAISE( sctTableSpaceMgr::findSpaceNodeBySpaceID( sSpaceID,
                                                                  (void**)&sTBSNode ) 
                        != IDE_SUCCESS, err_find_object_inconsistency);
        IDE_TEST_RAISE( smmExpandChunk::isFreePageID( sTBSNode,
                                                      sPageID,
                                                      &sIsFreePage )
                        != IDE_SUCCESS, err_find_object_inconsistency);
        if ( sIsFreePage == ID_TRUE )
        {
            /* Free된 Page. 무조건 수행함 */
            aObj->mState = SMR_RTOI_STATE_CHECKED;
        }
    case SMR_RTOI_TYPE_INIT:
    case SMR_RTOI_TYPE_DISKPAGE:
        break;
    }

    if ( aObj->mState == SMR_RTOI_STATE_CHECKED )
    {
        /* 위에서 이미 검사함 */
    }
    else
    {
        switch( aObj->mType )
        {
        case SMR_RTOI_TYPE_TABLE:
            IDE_TEST_RAISE( smmManager::getOIDPtr( sSpaceID,
                                                   sOID,
                                                   (void**) &sTable )
                            != IDE_SUCCESS,
                            err_find_object_inconsistency);
            aObj->mTableOID = smLayerCallback::getTableOID( sTable );
            /* PROJ-2375 Global Meta BUG-36236, 37915 임시로 막음 */
            //IDE_DASSERT( aObj->mTableOID + SMP_SLOT_HEADER_SIZE  == sOID );

            IDE_TEST_RAISE( smLayerCallback::isTableConsistent( (void*)sTable )
                            == ID_FALSE,
                            err_inconsistent_flag );
            break;
        case SMR_RTOI_TYPE_INDEX:
            IDE_TEST_RAISE( smmManager::getOIDPtr( sSpaceID,
                                                   sOID,
                                                   (void**) &sIndex )
                            != IDE_SUCCESS,
                            err_find_object_inconsistency);
            aObj->mTableOID = smLayerCallback::getTableOIDOfIndexHeader( sIndex );
            aObj->mIndexID  = smLayerCallback::getIndexIDOfIndexHeader( sIndex );
            IDE_TEST_RAISE( smLayerCallback::getIsConsistentOfIndexHeader( sIndex )
                            == ID_FALSE,
                            err_inconsistent_flag );
            break;
        case SMR_RTOI_TYPE_MEMPAGE:
            IDE_TEST_RAISE( smmManager::getPersPagePtr( sSpaceID,
                                                        sPageID,
                                                        (void**) &sMemPagePtr )
                            != IDE_SUCCESS,
                            err_find_object_inconsistency);
            aObj->mTableOID = smLayerCallback::getTableOID4MRDBPage( sMemPagePtr );
            IDE_ASSERT( sMemPagePtr->mSelfPageID == sPageID );
            IDE_TEST_RAISE( SMP_GET_PERS_PAGE_INCONSISTENCY( sMemPagePtr )
                            == SMP_PAGEINCONSISTENCY_TRUE,
                            err_inconsistent_flag );
            break;
        case SMR_RTOI_TYPE_DISKPAGE:
            if ( aObj->mCauseDiskPage == NULL )
            {
                /* Undo 시에는 getPage해야함 */
                IDE_TEST_RAISE( sdbBufferMgr::getPage( NULL, // idvSQL
                                                       sSpaceID,
                                                       sPageID,
                                                       SDB_S_LATCH,
                                                       SDB_WAIT_NORMAL,
                                                       SDB_SINGLE_PAGE_READ,
                                                       &sDiskPagePtr,
                                                       &sTrySuccess )
                                != IDE_SUCCESS,
                                err_find_object_inconsistency);
                IDE_TEST_RAISE( sTrySuccess == ID_FALSE,
                                err_find_object_inconsistency);
                sGetPage = ID_TRUE;
            }
            else
            {
                /* sdrRedoMgr::applyLogRecList에서 들어온 경우로
                 * 이미 getpage되어 있고, 알아서 release함  */
                sDiskPagePtr = aObj->mCauseDiskPage;
            }
            /* DRDB Consistency가 깨진 상황이면 어차피 Logging/Flush 모두 
             * 막힌다.  다만 WAL이 깨진 페이지에 접근하여 Startup도 실패할 수
             * 있다. mLstDRDBRedoLSN 보다 UpdateLSN이 크면 WAL이 깨진 
             * 상황이다. */
            sPageLSN = sdpPhyPage::getPageLSN( sDiskPagePtr );
            IDE_TEST_RAISE( ( mDRDBConsistency == ID_FALSE ) &&
                            ( smrCompareLSN::isLT( &mLstDRDBRedoLSN, 
                                                   &sPageLSN ) ),
                            err_find_object_inconsistency);

            IDE_TEST_RAISE( smLayerCallback::isConsistentPage4DRDB( sDiskPagePtr )
                            == ID_FALSE,
                            err_inconsistent_flag );
            if ( aObj->mCauseDiskPage == NULL )
            {
                sGetPage = ID_FALSE;
                IDE_TEST_RAISE( sdbBufferMgr::releasePage( NULL, // idvSQL
                                                           sDiskPagePtr )
                                != IDE_SUCCESS,
                                err_find_object_inconsistency);
            }
            else
            {
                /* 호출한 sdrRedoMgr에서 알아서 release */
            }
            break;
        default:
            IDE_DASSERT( 0 );
            break;
        }

        /* Check 완료 */
        aObj->mState = SMR_RTOI_STATE_CHECKED;
    }

    return ID_TRUE;

    IDE_EXCEPTION( err_inconsistent_flag );
    {
        /* Inconsistent Flag가 이미 설정된 경우, 또 설정 안해도 됨. */
        aObj->mState = SMR_RTOI_STATE_DONE;
        aObj->mCause = SMR_RTOI_CAUSE_OBJECT;
    }
    IDE_EXCEPTION( err_find_object_inconsistency );
    {
        /* 새롭게 잘못된 점을 찾은 경우. Flag 설정 해야 함*/
        aObj->mState = SMR_RTOI_STATE_CHECKED;
        aObj->mCause = SMR_RTOI_CAUSE_OBJECT;
    }
    IDE_EXCEPTION_END;

    if ( sGetPage == ID_TRUE )
    {
        (void) sdbBufferMgr::releasePage( NULL, // idvSQL
                                          sDiskPagePtr );
    }

    return ID_FALSE;

}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * startup이 실패한 상황에 대해 대처합니다.
 *
 * aObj         - [IN] 검사할 대상
 * aisRedo      - [IN] Redo중인가?
 ***********************************************************************/
IDE_RC smrRecoveryMgr::startupFailure( smrRTOI * aObj,
                                       idBool    aIsRedo )
{
    IDE_DASSERT( aObj->mCause != SMR_RTOI_CAUSE_INIT );

    /* Check되어있어야만, Inconsistent 설정이 의미가 있음. */
    if ( aObj->mState == SMR_RTOI_STATE_CHECKED )
    {
        if ( aObj->mCause == SMR_RTOI_CAUSE_PROPERTY )
        {
            /* Property에 의해 예외처리된 경우는 사용자가 해당 객체의
             * 손실을 감수한다는 것이기에 Durability 손실이 없음 */
        }
        else
        {
            /* Recovery가 실패하니 손실 발생 */
            mDurability = ID_FALSE;
        }

        /* RECOVERY정책이 Normal일 경우, 구동 실패시킴. */
        IDE_TEST_RAISE( ( mEmerRecovPolicy == SMR_RECOVERY_NORMAL ) &&
                        ( mDurability == ID_FALSE ),
                        err_durability );

        setObjectInconsistency( aObj, aIsRedo );
        addIOL( aObj );
    }
    else
    {
        /* Inconsistent 설정 못함  */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_durability );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_FAILURE_DURABILITY_AT_STARTUP ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * TargetObject를 Inconsistent하다고 설정함
 * 참고로 Inconsistent 설정시, 항상 NULL TID, 즉 DUMMY TX를 사용함.
 * CLR을 남기는 Undo중인 TX를 이용할 수 없기 때문.
 *
 * aObj         - [IN] 검사할 대상
 * aisRedo      - [IN] Redo중인가?
 **********************************************************************/
void smrRecoveryMgr::setObjectInconsistency( smrRTOI * aObj,
                                             idBool    aIsRedo )
{
    scSpaceID   sSpaceID;
    scPageID    sPageID;
    SChar       sBuffer[ SMR_MESSAGE_BUFFER_SIZE ];


    /* Checked된 상태에서만 설정할 필요가 있음. */
    if ( aObj->mState == SMR_RTOI_STATE_CHECKED )
    {
        if ( aIsRedo == ID_TRUE )
        {
            /* Redo시점에서는 바로 수행하면 Log가 변경되니 미룬다.
             * 등록해두면 나중에 Redo끝나고 알아서 수행한다.*/
            if ( aObj->mCauseDiskPage != NULL )
            {
                /* 단 DRDB Page일 경우 바로 수행한다. DRDB는 Page Locality가
                 * 중요하기 때문에, 성능상 영향이 있기 때문이다. */
                ((sdpPhyPageHdr*)aObj->mCauseDiskPage)->mIsConsistent = ID_FALSE;
            }

        }
        else
        {
            sSpaceID = aObj->mGRID.mSpaceID;
            sPageID  = aObj->mGRID.mPageID;

            switch( aObj->mType )
            {
            case SMR_RTOI_TYPE_TABLE:
                IDE_TEST( smLayerCallback::setTableHeaderInconsistency( aObj->mTableOID )
                          != IDE_SUCCESS );
                break;
            case SMR_RTOI_TYPE_INDEX:
                IDE_TEST( smLayerCallback::setIndexInconsistency( aObj->mTableOID,
                                                                  aObj->mIndexID )
                          != IDE_SUCCESS );
                break;
            case SMR_RTOI_TYPE_MEMPAGE:
                IDE_TEST( smLayerCallback::setPersPageInconsistency4MRDB( sSpaceID,
                                                                          sPageID )
                          != IDE_SUCCESS );
                break;
            case SMR_RTOI_TYPE_DISKPAGE:
                IDE_TEST( smLayerCallback::setPageInconsistency4DRDB( sSpaceID,
                                                                      sPageID )
                          != IDE_SUCCESS );
                break;
            default:
                IDE_DASSERT( 0 );
                break;
            }

            aObj->mState = SMR_RTOI_STATE_DONE;
        }
    }
    else
    {
        /* 이미 Flag 설정한 경우 */
    }

    return;

    /* Inconsistency 설정에 실패한 경우.
     * 어쨌든 Startup은 해야 하기 때문에, 감수하고 진행한다. */

    IDE_EXCEPTION_END;

    idlOS::snprintf( sBuffer,
                     SMR_MESSAGE_BUFFER_SIZE,
                     SM_TRC_EMERECO_FAILED_SET_INCONSISTENCY_FLAG );

    ideLog::log( IDE_ERR_0, sBuffer );
    IDE_CALLBACK_SEND_MSG( sBuffer );
    displayRTOI( aObj );

    return;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * Undo실패에 대한 RTOI를 준비함
 *
 * aTrans       - [IN] Undo하던 Transaction
 * aType        - [IN] 잘못된 객체의 종류
 * aTableOID    - [IN] RTOI에 기록할 실패할 TableOID
 * aIndexID     - [IN] 실패한 IndexID
 * aSpaceID     - [IN] 대상 페이지의 TablesSpaceID
 * aPageID      - [IN] 대상 페이지의 PageID
 **********************************************************************/
void smrRecoveryMgr::prepareRTOIForUndoFailure( void        * aTrans,
                                                smrRTOIType   aType,
                                                smOID         aTableOID,
                                                UInt          aIndexID,
                                                scSpaceID     aSpaceID,
                                                scPageID      aPageID )
{
    smrRTOI * sObj;

    sObj = smLayerCallback::getRTOI4UndoFailure( aTrans );

    initRTOI( sObj );
    sObj->mCause          = SMR_RTOI_CAUSE_UNDO;
    sObj->mCauseLSN       = smLayerCallback::getCurUndoNxtLSN( aTrans );
    sObj->mCauseDiskPage  = NULL;
    sObj->mType           = aType;
    sObj->mState          = SMR_RTOI_STATE_CHECKED;
    sObj->mTableOID       = aTableOID;
    sObj->mIndexID        = aIndexID;
    sObj->mGRID.mSpaceID  = aSpaceID;
    sObj->mGRID.mPageID   = aPageID;
    sObj->mGRID.mOffset   = 0;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * Refine실패로 해당 Table이 Inconsistent 해짐
 *
 * aTableOID    - [IN] 실패한 Table
 **********************************************************************/
IDE_RC smrRecoveryMgr::refineFailureWithTable( smOID   aTableOID )
{
    smrRTOI   sObj;

    initRTOI( &sObj );
    sObj.mCause          = SMR_RTOI_CAUSE_REFINE;
    SM_SET_LSN( sObj.mCauseLSN, 0, 0 );
    sObj.mCauseDiskPage  = NULL;
    sObj.mType           = SMR_RTOI_TYPE_TABLE;
    sObj.mState          = SMR_RTOI_STATE_CHECKED;
    sObj.mTableOID       = aTableOID;
    sObj.mIndexID        = 0;

    IDE_TEST( smrRecoveryMgr::startupFailure( &sObj,
                                              ID_FALSE ) // isRedo
              != IDE_SUCCESS );

    /* IDE_SUCCESS로 Return하기 때문에, ide error를 청소함 */
    ideClearError();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * Refine실패로 해당 Index가 Inconsistent 해짐
 *
 * aTableOID    - [IN] 실패한 Table
 * aIndex       - [IN] 실패한 Index
 **********************************************************************/
IDE_RC smrRecoveryMgr::refineFailureWithIndex( smOID   aTableOID,
                                               UInt    aIndexID )
{
    smrRTOI   sObj;

    initRTOI( &sObj );
    sObj.mCause          = SMR_RTOI_CAUSE_REFINE;
    SM_SET_LSN( sObj.mCauseLSN, 0, 0 );
    sObj.mCauseDiskPage  = NULL;
    sObj.mType           = SMR_RTOI_TYPE_INDEX;
    sObj.mState          = SMR_RTOI_STATE_CHECKED;
    sObj.mTableOID       = aTableOID;
    sObj.mIndexID        = aIndexID;

    IDE_TEST( smrRecoveryMgr::startupFailure( &sObj,
                                              ID_FALSE ) // isRedo
              != IDE_SUCCESS );

    /* IDE_SUCCESS로 Return하기 때문에, ide error를 청소함 */
    ideClearError();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * IOL을 초기화 합니다.
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::initializeIOL()
{
    mIOLHead.mNext = NULL;
    mIOLCount      = 0;

    IDE_TEST( mIOLMutex.initialize( (SChar*) "SMR_IOL_MUTEX",
                                   IDU_MUTEX_KIND_NATIVE,
                                   IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * IOL을 추가 합니다.
 *
 * aObj         - [IN] 추가할 객체
 **********************************************************************/
void smrRecoveryMgr::addIOL( smrRTOI * aObj )
{
    smrRTOI * sObj;
    SChar     sBuffer[ SMR_MESSAGE_BUFFER_SIZE ];

    if ( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                            ID_SIZEOF( smrRTOI ),
                            (void**)&sObj,
                            IDU_MEM_FORCE)
         == IDE_SUCCESS )
    {
        idlOS::memcpy( sObj, aObj, ID_SIZEOF( smrRTOI ) );

        /* Link Head에 Attach
         * 사실 현재 Mutex를 잡을 필요는 없지만, Parallel Recovery를
         * 대비하여 Mutex를 잡아둔다 */
        (void)mIOLMutex.lock( NULL /* statistics */ );
        sObj->mNext    = mIOLHead.mNext;
        mIOLHead.mNext = sObj;
        mIOLCount ++;
        (void)mIOLMutex.unlock();

        displayRTOI( aObj );
    }
    else
    {
        /* nothing to do ...
         * 메모리 부족으로 실패하는 상황. 어떻게든 서버 구동은 성공해야 하며
         * addIOL이 실패해도 inconsistency 설정만 안될 것이기 때문에
         * 그냥 무시한다. DEBUG모드 빼고 */
        idlOS::snprintf( sBuffer,
                         SMR_MESSAGE_BUFFER_SIZE,
                         SM_TRC_EMERECO_FAILED_SET_INCONSISTENCY_FLAG );

        ideLog::log( IDE_ERR_0, sBuffer );
        IDE_CALLBACK_SEND_MSG( sBuffer );
        displayRTOI( aObj );

        IDE_DASSERT( 0 );
    }
}

/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * IOL이 이미 등록되어있는지 검사함
 *
 * aObj         - [IN] 검사할 객체
 **********************************************************************/
idBool smrRecoveryMgr::findIOL( smrRTOI * aObj )
{
    smrRTOI * sCursor;
    idBool    sRet;

    sRet = ID_FALSE;

    sCursor = mIOLHead.mNext;
    while( (sCursor != NULL ) && ( sRet == ID_FALSE ) )
    {
        if ( aObj->mType == sCursor->mType )
        {
            switch( aObj->mType )
            {
            case SMR_RTOI_TYPE_TABLE:
                if ( aObj->mTableOID == sCursor->mTableOID )
                {
                    sRet = ID_TRUE;
                }
                break;
            case SMR_RTOI_TYPE_INDEX:
                if ( aObj->mIndexID == sCursor->mIndexID )
                {
                    sRet = ID_TRUE;
                }
                break;
            case SMR_RTOI_TYPE_MEMPAGE:
            case SMR_RTOI_TYPE_DISKPAGE:
                if ( ( aObj->mGRID.mSpaceID == sCursor->mGRID.mSpaceID ) &&
                     ( aObj->mGRID.mPageID  == sCursor->mGRID.mPageID  ) )
                {
                    sRet = ID_TRUE;
                }
                break;
            default:
                IDE_DASSERT(0);
                break;
            }
        }
        else
        {
            sRet = ID_FALSE;
        }

        sCursor = sCursor->mNext;
    }

    return sRet;
}


/**********************************************************************
 * Description : PROJ-2162 RestartRiskReduction
 *
 * TOI Message를 TRC/isql에 출력합니다.
 *
 * aObj         - [IN] 출력할 대상
 **********************************************************************/
void smrRecoveryMgr::displayRTOI(  smrRTOI * aObj )
{
    SChar             sBuffer[ SMR_MESSAGE_BUFFER_SIZE ] = {0};

    if ( ( aObj->mState == SMR_RTOI_STATE_CHECKED ) ||
         ( aObj->mState == SMR_RTOI_STATE_DONE ) )
    {
        switch( aObj->mCause )
        {
        case SMR_RTOI_CAUSE_PROPERTY:
            idlOS::snprintf( sBuffer,
                             SMR_MESSAGE_BUFFER_SIZE,
                             "\n          StartupFailure: "
                             "(SRC : PROPERTY )" );
            break;
        case SMR_RTOI_CAUSE_OBJECT:
            idlOS::snprintf( sBuffer,
                             SMR_MESSAGE_BUFFER_SIZE,
                             "\n          StartupFailure: "
                             "(SRC : INCONSISTENT OBJECT )" );
            break;
        case SMR_RTOI_CAUSE_REFINE:
            idlOS::snprintf( sBuffer,
                             SMR_MESSAGE_BUFFER_SIZE,
                             "\n          StartupFailure: "
                             "(SRC : REFINE FAILURE )" );
            break;
        case SMR_RTOI_CAUSE_REDO:
        case SMR_RTOI_CAUSE_UNDO:
            idlOS::snprintf( sBuffer,
                             SMR_MESSAGE_BUFFER_SIZE,
                             "\n          StartupFailure: "
                             "(SRC : LSN=<%"ID_UINT32_FMT","
                             "%"ID_UINT32_FMT">)",
                             aObj->mCauseLSN.mFileNo,
                             aObj->mCauseLSN.mOffset );
            break;
        default:
            IDE_DASSERT(0);
            break;
        }
        switch( aObj->mType )
        {
        case SMR_RTOI_TYPE_TABLE:
            idlVA::appendFormat( sBuffer,
                                 SMR_MESSAGE_BUFFER_SIZE,
                                 "\tTABLE, TableOID=<%"ID_UINT64_FMT">",
                                 aObj->mTableOID );
            break;
        case SMR_RTOI_TYPE_INDEX:
            idlVA::appendFormat( sBuffer,
                                 SMR_MESSAGE_BUFFER_SIZE,
                                 "\tINDEX, IndexID=<%"ID_UINT32_FMT">",
                                 aObj->mIndexID );
            break;
        case SMR_RTOI_TYPE_MEMPAGE:
            idlVA::appendFormat( sBuffer,
                                 SMR_MESSAGE_BUFFER_SIZE,
                                 "\tMRDB PAGE, SPACEID=<%"ID_UINT32_FMT">,"
                                 "PID=<%"ID_UINT32_FMT">",
                                 aObj->mGRID.mSpaceID,
                                 aObj->mGRID.mPageID );
            break;
        case SMR_RTOI_TYPE_DISKPAGE:
            idlVA::appendFormat( sBuffer,
                                 SMR_MESSAGE_BUFFER_SIZE,
                                 "\tDRDB PAGE, SPACEID=<%"ID_UINT32_FMT">,"
                                 "PID=<%"ID_UINT32_FMT">",
                                 aObj->mGRID.mSpaceID,
                                 aObj->mGRID.mPageID );
            break;
        default:
            break;
        }

        ideLog::log( IDE_ERR_0, sBuffer );
        IDE_CALLBACK_SEND_MSG( sBuffer );
    }
}

/* Redo시점에 등록은 되었지만 미뤄둔 Inconsistency를 설정함 */
void smrRecoveryMgr::applyIOLAtRedoFinish()
{
    smrRTOI * sCursor;

    sCursor = mIOLHead.mNext;
    while( sCursor != NULL )
    {
        setObjectInconsistency( sCursor,
                                ID_FALSE ); // isRedo

        sCursor = sCursor->mNext;
    }
}

/* IOL(InconsistentObjectList)를 제거 합니다. */
IDE_RC smrRecoveryMgr::finalizeIOL()
{
    smrRTOI * sCursor;
    smrRTOI * sNextCursor;

    sCursor = mIOLHead.mNext;
    while( sCursor != NULL )
    {
        sNextCursor = sCursor->mNext;
        IDE_TEST( iduMemMgr::free( (void**)sCursor ) != IDE_SUCCESS );
        sCursor = sNextCursor;
    }

    mIOLHead.mNext = NULL;
    mIOLCount      = 0;
    IDE_TEST( mIOLMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* DRDB Wal이 깨졌는지 확인합니다. */
IDE_RC smrRecoveryMgr::checkDiskWAL()
{
    SChar    sBuffer[ SMR_MESSAGE_BUFFER_SIZE ];
    smLSN    sLstUpdatedPageLSN = sdrRedoMgr::getLastUpdatedPageLSN();
    SChar *  sFileName;

    if( smrCompareLSN::isGT( &sLstUpdatedPageLSN,
                             &mLstDRDBRedoLSN ) == ID_TRUE )
    {
        idlOS::snprintf( sBuffer,
                         SMR_MESSAGE_BUFFER_SIZE,
                         SM_TRC_EMERECO_DRDB_WAL_VIOLATION,
                         sLstUpdatedPageLSN.mFileNo,
                         sLstUpdatedPageLSN.mOffset,
                         mLstDRDBRedoLSN.mFileNo,
                         mLstDRDBRedoLSN.mOffset );
        ideLog::log( IDE_ERR_0, sBuffer );
        IDE_CALLBACK_SEND_MSG( sBuffer );

        /* BUG-48275 recovery 에서 잘못된 오류 메시지가 출력 됩니다.
         *           추가 정보 기록 */
        ideLog::log( IDE_ERR_0,
                     "Last Updated SpaceID: %"ID_UINT32_FMT", "
                     "PageID: %" ID_UINT32_FMT ", "
                     "FileID: %" ID_UINT32_FMT ", "
                     "FPageID: %"ID_UINT32_FMT"\n",
                     sdrRedoMgr::getLastUpdatedSpaceID(),
                     sdrRedoMgr::getLastUpdatedPageID(),
                     SD_MAKE_FID(sdrRedoMgr::getLastUpdatedPageID()),
                     SD_MAKE_FPID(sdrRedoMgr::getLastUpdatedPageID()));

        sFileName = sddDiskMgr::getFileName( sdrRedoMgr::getLastUpdatedSpaceID(),
                                             sdrRedoMgr::getLastUpdatedPageID() );

        if ( sFileName != NULL )
        {
            ideLog::log( IDE_ERR_0,
                         "Last Updated File : %s\n",
                         sFileName );
        }

        IDE_TEST( prepare4DBInconsistencySetting() != IDE_SUCCESS );
        mDRDBConsistency = ID_FALSE;
        IDE_TEST( finish4DBInconsistencySetting() != IDE_SUCCESS );
    }
    else
    {
        mDRDBConsistency = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* MRDB Wal이 깨졌는지 확인합니다. */
IDE_RC smrRecoveryMgr::checkMemWAL()
{
    SChar             sBuffer[ SMR_MESSAGE_BUFFER_SIZE ];

    if ( smrCompareLSN::isGT( &mEndChkptLSN, &mLstRedoLSN )
         == ID_TRUE )
    {
        idlOS::snprintf( sBuffer,
                         SMR_MESSAGE_BUFFER_SIZE,
                         SM_TRC_EMERECO_MRDB_WAL_VIOLATION,
                         mEndChkptLSN.mFileNo,
                         mEndChkptLSN.mOffset,
                         mLstRedoLSN.mFileNo,
                         mLstRedoLSN.mOffset );
        ideLog::log( IDE_ERR_0, sBuffer );
        IDE_CALLBACK_SEND_MSG( sBuffer );

        IDE_TEST( prepare4DBInconsistencySetting() != IDE_SUCCESS );
        mMRDBConsistency = ID_FALSE;
        IDE_TEST( finish4DBInconsistencySetting() != IDE_SUCCESS );
    }
    else
    {
        mMRDBConsistency = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* DB Inconsistency 설정을 위한 준비를 합니다.
 * Flusher를 멈춥니다. */
IDE_RC smrRecoveryMgr::prepare4DBInconsistencySetting()
{
    UInt i;

    /* BUG-48275 recovery 에서 잘못된 오류 메시지가 출력 됩니다.
     * EMERGENCY NORMAL에서 다음을 확인하지 않고 바로 오류 출력한다. */
    IDE_TEST( mEmerRecovPolicy == SMR_RECOVERY_NORMAL );

    /* Inconsistent한 DB일 경우 LogFlush를 막는다. 그런데 mmap일 경우, Log가
     * 자동으로 기록되기 때문에 Log의 기록을 막을 수가 없다. */
    IDE_TEST_RAISE( smuProperty::getLogBufferType() == SMU_LOG_BUFFER_TYPE_MMAP,
                    ERR_INCONSISTENT_DB_AND_LOG_BUFFER_TYPE );

    for( i = 0 ; i < sdbFlushMgr::getFlusherCount(); i ++ )
    {
        IDE_TEST( sdbFlushMgr::turnOffFlusher( NULL /*Statistics*/,
                                               i )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INCONSISTENT_DB_AND_LOG_BUFFER_TYPE)
    {
        IDE_SET( ideSetErrorCode(
                 smERR_ABORT_ERR_INCONSISTENT_DB_AND_LOG_BUFFER_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* DB Inconsistency 설정을 마무리 합니다.
 * Flusher를 재구동합니다. */
IDE_RC smrRecoveryMgr::finish4DBInconsistencySetting()
{
    UInt i;

    for( i = 0 ; i < sdbFlushMgr::getFlusherCount(); i ++ )
    {
        IDE_TEST( sdbFlushMgr::turnOnFlusher( i )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 * Level 0 incremental bakcup파일을 이용해 데이터베이스를 복원한다.
 * 
 * aRestoreType     - [IN] 복원 종류
 * aUntilTime       - [IN] 불완전 복원 (시간)
 * aUntilBackupTag  - [IN] 불완전 복원 (backupTag)
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::restoreDB( smiRestoreType    aRestoreType,
                                  UInt              aUntilTime,
                                  SChar           * aUntilBackupTag )
{
    UInt            sUntilTimeBISlotIdx;           
    UInt            sStartScanBISlotIdx;
    UInt            sRestoreBISlotIdx = SMRI_BI_INVALID_SLOT_IDX;
    UInt            sFirstRestoreBISlotIdx;
    smriBIFileHdr * sBIFileHdr;  
    smriBISlot    * sBISlot;
    smriBISlot    * sLastRestoredBISlot;
    idBool          sSearchUntilBackupTag; 
    idBool          sIsNeedRestoreLevel1 = ID_FALSE;
    SChar           sRestoreTag[ SMI_MAX_BACKUP_TAG_NAME_LEN ];
    SChar           sBuffer[SMR_MESSAGE_BUFFER_SIZE];
    scSpaceID       sDummySpaceID = 0;
    UInt            sState = 0;

    IDE_TEST_RAISE( getBIMgrState() != SMRI_BI_MGR_INITIALIZED,
                    err_backup_info_state); 

    IDE_TEST( smriBackupInfoMgr::lock() != IDE_SUCCESS );
    sState = 1;

    /* backup info 파일에서 backupinfo slot들을 읽어들인다. */
    IDE_TEST( smriBackupInfoMgr::loadBISlotArea() != IDE_SUCCESS );
    sState = 2;

    idlOS::memset( sRestoreTag, 0x00, SMI_MAX_BACKUP_TAG_NAME_LEN );

    (void)smriBackupInfoMgr::getBIFileHdr( &sBIFileHdr );

    /* 복원 타입에따라 backupinfo slot의 검색을 시작할 위치를 정한다. */
    switch( aRestoreType )
    {
        case SMI_RESTORE_COMPLETE:
            {
                sStartScanBISlotIdx     = sBIFileHdr->mBISlotCnt - 1;
                sSearchUntilBackupTag   = ID_FALSE;

                break;
            }
        case SMI_RESTORE_UNTILTIME:
            {
                /* untilTime에 해당하는 backupinfo slot의 위치를 구한다. */ 
                IDE_TEST( smriBackupInfoMgr::findBISlotIdxUsingTime( 
                                                             aUntilTime, 
                                                             &sUntilTimeBISlotIdx ) 
                          != IDE_SUCCESS );

                sStartScanBISlotIdx     = sUntilTimeBISlotIdx;
                sSearchUntilBackupTag   = ID_FALSE;

                break;
            }
        case SMI_RESTORE_UNTILTAG:
            {   
                sStartScanBISlotIdx     = sBIFileHdr->mBISlotCnt - 1;
                sSearchUntilBackupTag   = ID_TRUE;

                break;
            }
        default:
            {   
                IDE_ASSERT(0);
                break;
            }
    }

    IDE_TEST_RAISE( sStartScanBISlotIdx == SMRI_BI_INVALID_SLOT_IDX,
                    there_is_no_incremental_backup );

    /* 복원을 시작할 backupinfo slot의 idx값을 구한다. */
    IDE_TEST( smriBackupInfoMgr::getRestoreTargetSlotIdx( aUntilBackupTag,
                                                sStartScanBISlotIdx,
                                                sSearchUntilBackupTag,
                                                SMRI_BI_BACKUP_TARGET_DATABASE,
                                                sDummySpaceID,
                                                &sRestoreBISlotIdx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRestoreBISlotIdx == SMRI_BI_INVALID_SLOT_IDX,
                    there_is_no_database_incremental_backup );

    IDE_TEST( smriBackupInfoMgr::getBISlot( sRestoreBISlotIdx, &sBISlot ) 
              != IDE_SUCCESS );

    /* 복원 대상인 backup tag를 저장한다. */
    idlOS::strncpy( sRestoreTag, 
                    sBISlot->mBackupTag, 
                    SMI_MAX_BACKUP_TAG_NAME_LEN );

    /* 
     * 복원 대상의 backup tag와 다른 backup tag를 가진 backup info slot이
     * 나올때까지 full backup 파일을 복원한다.
     */ 
    
    IDE_CALLBACK_SEND_MSG("  [ RECMGR ] restoring level 0 datafile....");
    sFirstRestoreBISlotIdx = sRestoreBISlotIdx;

    for( ; 
         sRestoreBISlotIdx < sBIFileHdr->mBISlotCnt; 
         sRestoreBISlotIdx++ )
    {
        IDE_TEST( smriBackupInfoMgr::getBISlot( sRestoreBISlotIdx, &sBISlot ) 
                  != IDE_SUCCESS );

        if ( idlOS::strncmp( sRestoreTag, 
                             sBISlot->mBackupTag, 
                             SMI_MAX_BACKUP_TAG_NAME_LEN ) != 0 )
        {
            sIsNeedRestoreLevel1 = ID_TRUE;
            break;
        }
        else
        {
            /* nothung to do */
        }

        IDE_DASSERT( ( sBISlot->mBackupType & SMI_BACKUP_TYPE_FULL ) != 0 );
        IDE_DASSERT( sBISlot->mBackupTarget == SMRI_BI_BACKUP_TARGET_DATABASE );

        IDE_TEST( restoreDataFile4Level0( sBISlot ) != IDE_SUCCESS );
    }

    idlOS::snprintf( sBuffer,
                     SMR_MESSAGE_BUFFER_SIZE,
                     "  \t\t backup info slots <%"ID_UINT32_FMT"> - <%"ID_UINT32_FMT"> restored",
                     sFirstRestoreBISlotIdx,
                     sRestoreBISlotIdx - 1 );

    IDE_CALLBACK_SEND_MSG(sBuffer);

    /* BUG-37371
     * backuptag가 level 0 백업까지 만 복구하도록 지정된경우 level 1복원을 하지
     * 않는다. */
    if ( (aUntilBackupTag != NULL) &&
        (idlOS::strncmp( sRestoreTag, 
                         aUntilBackupTag, 
                         SMI_MAX_BACKUP_TAG_NAME_LEN ) == 0) )
    {
        sIsNeedRestoreLevel1 = ID_FALSE;
    }
    else
    {
        /* nothung to do */
    }
    
    /* 
     * incremental backup 파일을 이용한 복구가 필요한지 확인하고
     * backup파일을 이용해 데이터베이스 복구를 수행한다. 
     */
    if ( sIsNeedRestoreLevel1 == ID_TRUE )
    {
        IDE_DASSERT( sRestoreBISlotIdx < sBIFileHdr->mBISlotCnt );

        IDE_TEST( restoreDB4Level1( sRestoreBISlotIdx,
                                    aRestoreType, 
                                    aUntilTime, 
                                    aUntilBackupTag, 
                                    &sLastRestoredBISlot ) 
                  != IDE_SUCCESS );

    }
    else
    {
        IDE_DASSERT( sRestoreBISlotIdx <= sBIFileHdr->mBISlotCnt );
        sLastRestoredBISlot = sBISlot;
    }

    setCurrMediaTime( sLastRestoredBISlot->mEndBackupTime );
    setLastRestoredTagName( sLastRestoredBISlot->mBackupTag );

    sState = 1;
    IDE_TEST( smriBackupInfoMgr::unloadBISlotArea() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smriBackupInfoMgr::unlock() != IDE_SUCCESS );
   
    return IDE_SUCCESS;

    IDE_EXCEPTION( there_is_no_database_incremental_backup );
    {  
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_ThereIsNoDatabaseIncrementalBackup));
    }
    IDE_EXCEPTION( there_is_no_incremental_backup );
    {  
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_ThereIsNoIncrementalBackup));
    }
    IDE_EXCEPTION( err_backup_info_state ); 
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupInfoState));
    }

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( smriBackupInfoMgr::unloadBISlotArea() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smriBackupInfoMgr::unlock() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::restoreTBS( scSpaceID aSpaceID ) 
{
    smriBIFileHdr * sBIFileHdr;
    smriBISlot    * sBISlot;
    SChar           sBuffer[SMR_MESSAGE_BUFFER_SIZE];
    UInt            sRestoreBISlotIdx;
    UInt            sFirstRestoreBISlotIdx = SMRI_BI_INVALID_SLOT_IDX;
    UInt            sLastRestoreBISlotIdx  = SMRI_BI_INVALID_SLOT_IDX;
    UInt            sState = 0;
    
    IDE_TEST_RAISE( getBIMgrState() != SMRI_BI_MGR_INITIALIZED,
                    err_backup_info_state); 

    IDE_TEST( smriBackupInfoMgr::lock() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smriBackupInfoMgr::loadBISlotArea() != IDE_SUCCESS );
    sState = 2;

    (void)smriBackupInfoMgr::getBIFileHdr( &sBIFileHdr );

    IDE_TEST( smriBackupInfoMgr::getRestoreTargetSlotIdx( NULL,
                                        sBIFileHdr->mBISlotCnt - 1,
                                        ID_FALSE,
                                        SMRI_BI_BACKUP_TARGET_TABLESPACE,
                                        aSpaceID,
                                        &sRestoreBISlotIdx )
              != IDE_SUCCESS );
        
    IDE_TEST_RAISE( sRestoreBISlotIdx == SMRI_BI_INVALID_SLOT_IDX,
                    there_is_no_incremental_backup );

    idlOS::snprintf( sBuffer,
                     SMR_MESSAGE_BUFFER_SIZE,
                     "  [ RECMGR ] restoring datafile.... TBS ID<%"ID_UINT32_FMT"> ",
                     aSpaceID );

    IDE_CALLBACK_SEND_MSG(sBuffer);

    for( ; sRestoreBISlotIdx < sBIFileHdr->mBISlotCnt; sRestoreBISlotIdx++ )
    {
        IDE_TEST( smriBackupInfoMgr::getBISlot( sRestoreBISlotIdx, &sBISlot ) 
                  != IDE_SUCCESS );

        if ( sBISlot->mSpaceID == aSpaceID )
        {
            if ( ( sBISlot->mBackupType & SMI_BACKUP_TYPE_FULL ) != 0 )
            {
                if ( sFirstRestoreBISlotIdx == SMRI_BI_INVALID_SLOT_IDX )
                {
                    sFirstRestoreBISlotIdx = sRestoreBISlotIdx;
                }
                sLastRestoreBISlotIdx = sRestoreBISlotIdx;

                IDE_TEST( restoreDataFile4Level0( sBISlot ) != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } 
    }

    idlOS::snprintf( sBuffer,
                     SMR_MESSAGE_BUFFER_SIZE,
                     "  \t\t backup info slots <%"ID_UINT32_FMT"> - <%"ID_UINT32_FMT"> restored",
                     sFirstRestoreBISlotIdx,
                     sLastRestoreBISlotIdx );

    IDE_CALLBACK_SEND_MSG(sBuffer);

    IDE_TEST( restoreTBS4Level1( aSpaceID, 
                                 sRestoreBISlotIdx, 
                                 sBIFileHdr->mBISlotCnt )
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( smriBackupInfoMgr::unloadBISlotArea() != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( smriBackupInfoMgr::unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( there_is_no_incremental_backup );
    {  
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_ThereIsNoIncrementalBackup));
    }
    IDE_EXCEPTION( err_backup_info_state ); 
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_BackupInfoState));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( smriBackupInfoMgr::unloadBISlotArea() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smriBackupInfoMgr::unlock() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }
    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 * aBISlot  - [IN] 복원할 백업파일의 backupinfo slot
 **********************************************************************/
IDE_RC smrRecoveryMgr::restoreDataFile4Level0( smriBISlot * aBISlot )
{

    SChar               sRestoreFileName[ SMRI_BI_MAX_BACKUP_FILE_NAME_LEN ];
    SChar               sNxtStableDBFileName[ SMRI_BI_MAX_BACKUP_FILE_NAME_LEN ];
    iduFile             sFile;
    smmChkptPathNode  * sCPathNode;
    sctTableSpaceNode * sSpaceNode;
    smmTBSNode        * sMemSpaceNode;
    sddTableSpaceNode * sDiskSpaceNode;
    smmDatabaseFile   * sDatabaseFile;
    smmDatabaseFile   * sNxtStableDatabaseFile;
    sddDataFileNode   * sDataFileNode;
    UInt                sStableDB;
    UInt                sNxtStableDB;
    UInt                sCPathCount;
    idBool              sResult = ID_FALSE;
    UInt                sState = 0;

    IDE_DASSERT( aBISlot != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aBISlot->mSpaceID,
                                                        (void **)&sSpaceNode )
              != IDE_SUCCESS );
    
    idlOS::memset( sRestoreFileName, 0x00, SMRI_BI_MAX_BACKUP_FILE_NAME_LEN );

    if ( sctTableSpaceMgr::isMemTableSpace(aBISlot->mSpaceID) == ID_TRUE )
    {
        sMemSpaceNode = (smmTBSNode *)sSpaceNode;

        sStableDB = smmManager::getCurrentDB( sMemSpaceNode );
        sNxtStableDB = smmManager::getNxtStableDB( sMemSpaceNode );
        
        IDE_TEST( smmManager::getDBFile( sMemSpaceNode,
                                         sStableDB,
                                         aBISlot->mFileNo,
                                         SMM_GETDBFILEOP_NONE,
                                         &sDatabaseFile )
                  != IDE_SUCCESS );
        
        IDE_TEST( smmManager::getDBFile( sMemSpaceNode,
                                         sNxtStableDB,
                                         aBISlot->mFileNo,
                                         SMM_GETDBFILEOP_NONE,
                                         &sNxtStableDatabaseFile )
                  != IDE_SUCCESS );

        if ( (idlOS::strncmp( sDatabaseFile->getFileName(), 
                             "\0", 
                             SMI_MAX_DATAFILE_NAME_LEN ) == 0) ||
            (idlOS::strncmp( sNxtStableDatabaseFile->getFileName(), 
                             "\0", 
                             SMI_MAX_DATAFILE_NAME_LEN ) == 0) )
        {
            IDE_TEST( smmTBSChkptPath::getChkptPathNodeCount( sMemSpaceNode,                    
                                                              &sCPathCount )
                      != IDE_SUCCESS );
 
            IDE_ASSERT( sCPathCount != 0 );
 
            IDE_TEST( smmTBSChkptPath::getChkptPathNodeByIndex( 
                                                    sMemSpaceNode,                    
                                                    aBISlot->mFileNo % sCPathCount,
                                                    &sCPathNode )
                  != IDE_SUCCESS );    
 
            IDE_ASSERT( sCPathNode != NULL );
 
            idlOS::snprintf( sRestoreFileName,
                             SMRI_BI_MAX_BACKUP_FILE_NAME_LEN,
                             "%s%c%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT"",
                             sCPathNode->mChkptPathAttr.mChkptPath,
                             IDL_FILE_SEPARATOR,
                             sMemSpaceNode->mHeader.mName,
                             sStableDB,
                             aBISlot->mFileNo );
            
            IDE_TEST( sDatabaseFile->setFileName( sRestoreFileName ) 
                      != IDE_SUCCESS );
 
            sDatabaseFile->setDir( sCPathNode->mChkptPathAttr.mChkptPath );
 
            idlOS::snprintf( sNxtStableDBFileName,
                             SMRI_BI_MAX_BACKUP_FILE_NAME_LEN,
                             "%s%c%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT"",
                             sCPathNode->mChkptPathAttr.mChkptPath,
                             IDL_FILE_SEPARATOR,
                             sMemSpaceNode->mHeader.mName,
                             sNxtStableDB,
                             aBISlot->mFileNo );
 
            IDE_TEST( sNxtStableDatabaseFile->setFileName( sNxtStableDBFileName ) 
                      != IDE_SUCCESS );
 
            sNxtStableDatabaseFile->setDir( sCPathNode->mChkptPathAttr.mChkptPath );
        }
        else
        {
            idlOS::strncpy( sRestoreFileName, 
                            sDatabaseFile->getFileName(), 
                            SM_MAX_FILE_NAME );

            idlOS::strncpy( sNxtStableDBFileName, 
                            sNxtStableDatabaseFile->getFileName(), 
                            SM_MAX_FILE_NAME );
        }

        IDE_TEST( sDatabaseFile->isNeedCreatePingPongFile( aBISlot,
                                                           &sResult ) 
                  != IDE_SUCCESS ); 
    }
    else if ( sctTableSpaceMgr::isDiskTableSpace(aBISlot->mSpaceID) == ID_TRUE )
    {
        sDiskSpaceNode = (sddTableSpaceNode *)sSpaceNode;

        sDataFileNode = sDiskSpaceNode->mFileNodeArr[ aBISlot->mFileID ];
        
        idlOS::strncpy( sRestoreFileName, 
                        sDataFileNode->mFile.getFileName(),
                        SMI_MAX_DATAFILE_NAME_LEN );
    }
    else
    {
        IDE_DASSERT(0);
    }

    if ( idf::access( sRestoreFileName, F_OK ) == 0 )
    {
        idf::unlink( sRestoreFileName );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SMR,
                                1,                             
                                IDU_FIO_STAT_OFF,              
                                IDV_WAIT_INDEX_NULL )          
              != IDE_SUCCESS );              
    sState = 1;

    IDE_TEST( sFile.setFileName( aBISlot->mBackupFileName ) != IDE_SUCCESS );

    IDE_TEST( sFile.open() != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sFile.copy( NULL, sRestoreFileName ) != IDE_SUCCESS );

    if ( sResult == ID_TRUE )
    {
        IDE_TEST( sFile.copy( NULL, sNxtStableDBFileName ) != IDE_SUCCESS );
    }

    sState = 1;
    IDE_TEST( sFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sFile.destroy() == IDE_SUCCESS );
        case 0:
        default :
            break;
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 * Level 1 incremental bakcup파일을 이용해 데이터베이스를 복원한다.
 * 
 * aRestoreStartSlotIdx     - [IN] 복원를 시작할 backupinfo slot의 idx
 * aRestoreType             - [IN] 복원 타입
 * aUntilTime               - [IN] 불완전 복원 (시간)
 * aUntilBackupTag          - [IN] 불완전 복원 ( backup tag )
 * aLastRestoredBISlot      - [OUT] 마지막으로 복원된 backup info slot
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::restoreDB4Level1( UInt            aRestoreStartSlotIdx,
                                         smiRestoreType  aRestoreType,
                                         UInt            aUntilTime,
                                         SChar         * aUntilBackupTag,
                                         smriBISlot   ** aLastRestoredBISlot )
{
    UInt            sRestoreStartSlotIdx = SMRI_BI_INVALID_SLOT_IDX;
    UInt            sRestoreEndSlotIdx   = SMRI_BI_INVALID_SLOT_IDX;
    UInt            sSlotIdx;
    smriBISlot    * sBISlot = NULL;
    SChar           sBuffer[SMR_MESSAGE_BUFFER_SIZE];

    IDE_DASSERT( aLastRestoredBISlot != NULL );

    /* 복구를 수행할 backup info의 범위 계산 */
    IDE_TEST( smriBackupInfoMgr::calcRestoreBISlotRange4Level1( 
                                              aRestoreStartSlotIdx,
                                              aRestoreType,
                                              aUntilTime,
                                              aUntilBackupTag,
                                              &sRestoreStartSlotIdx,
                                              &sRestoreEndSlotIdx )
              != IDE_SUCCESS );

    IDE_CALLBACK_SEND_MSG("  [ RECMGR ] restoring level 1 datafile....");
    /* 복구 수행 */
    for( sSlotIdx = sRestoreStartSlotIdx; 
         sSlotIdx <= sRestoreEndSlotIdx; 
         sSlotIdx++ )
    {
        IDE_TEST( smriBackupInfoMgr::getBISlot( sSlotIdx, &sBISlot ) 
                  != IDE_SUCCESS );

        if ( sctTableSpaceMgr::isMemTableSpace( sBISlot->mSpaceID ) == ID_TRUE )
        {
            IDE_TEST( restoreMemDataFile4Level1( sBISlot ) != IDE_SUCCESS );
        }
        else if ( sctTableSpaceMgr::isDiskTableSpace( sBISlot->mSpaceID ) == ID_TRUE )
        {
            IDE_TEST( restoreDiskDataFile4Level1( sBISlot ) != IDE_SUCCESS );
        }
        else
        {
            IDE_DASSERT(0);
        }
    }

    IDE_ERROR( sBISlot != NULL );
    
    idlOS::snprintf( sBuffer,
                     SMR_MESSAGE_BUFFER_SIZE,
                     "  \t\t backup info slots <%"ID_UINT32_FMT"> - <%"ID_UINT32_FMT"> restored",
                     sRestoreStartSlotIdx,
                     sRestoreEndSlotIdx );

    IDE_CALLBACK_SEND_MSG(sBuffer);

    *aLastRestoredBISlot = sBISlot;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 * aSpaceID                 - [IN] 복구할 SpaceID
 * aRestoreStartBISlotIdx   - [IN] 복구를 시작할 backup info slot Idx
 * aBISlotCnt               - [IN] backup info 파일에 저장된 slot의 수
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::restoreTBS4Level1( scSpaceID aSpaceID, 
                                          UInt      aRestoreStartBISlotIdx,
                                          UInt      aBISlotCnt ) 
{
    UInt            sBISlotIdx;
    UInt            sRestoreStartSlotIdx = SMRI_BI_INVALID_SLOT_IDX;
    UInt            sRestoreEndSlotIdx = SMRI_BI_INVALID_SLOT_IDX;
    smriBISlot    * sBISlot;
    SChar           sBuffer[SMR_MESSAGE_BUFFER_SIZE];

    IDE_DASSERT( aRestoreStartBISlotIdx != SMRI_BI_INVALID_SLOT_IDX  );
    
    idlOS::snprintf( sBuffer,
                     SMR_MESSAGE_BUFFER_SIZE,
                     "  [ RECMGR ] restoring level 1 datafile.... TBS ID<%"ID_UINT32_FMT"> ",
                     aSpaceID );

    IDE_CALLBACK_SEND_MSG(sBuffer);

    for( sBISlotIdx = aRestoreStartBISlotIdx; 
         sBISlotIdx < aBISlotCnt; 
         sBISlotIdx++ )
    {
        IDE_TEST( smriBackupInfoMgr::getBISlot( sBISlotIdx, &sBISlot ) 
                  != IDE_SUCCESS );

        if ( sBISlot->mSpaceID == aSpaceID )
        {
            if ( sctTableSpaceMgr::isMemTableSpace( sBISlot->mSpaceID ) == ID_TRUE )
            {
                IDE_TEST( restoreMemDataFile4Level1( sBISlot ) != IDE_SUCCESS );
            }
            else if ( sctTableSpaceMgr::isDiskTableSpace( sBISlot->mSpaceID ) == ID_TRUE )
            {
                IDE_TEST( restoreDiskDataFile4Level1( sBISlot ) != IDE_SUCCESS );
            }
            else
            {
                IDE_ASSERT(0);
            }

            if ( sRestoreStartSlotIdx == SMRI_BI_INVALID_SLOT_IDX )
            {
                sRestoreStartSlotIdx = sBISlotIdx;
            }
            sRestoreEndSlotIdx = sBISlotIdx;
        }
    }

    if ( sRestoreStartSlotIdx != SMRI_BI_INVALID_SLOT_IDX )
    {
        idlOS::snprintf( sBuffer,
                         SMR_MESSAGE_BUFFER_SIZE,
                         "  \t\t backup info slots <%"ID_UINT32_FMT"> - <%"ID_UINT32_FMT"> restored",
                         sRestoreStartSlotIdx,
                         sRestoreEndSlotIdx );
    }
    else
    {
        idlOS::snprintf( sBuffer,
                         SMR_MESSAGE_BUFFER_SIZE,
                         "  \t\t No backup info slots restored");
    }

    IDE_CALLBACK_SEND_MSG(sBuffer);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 * aPageID      - [IN] 
 * smriBiSlot   - [IN]
 *
 * 원본함수 - smmManager::getPageNoInFile
 * chkptImage파일내에서 aPageID가 몇 번째에 위치하고있는가를 구한다.
 *
 * DBFilePageCnt는 Membase에 저장되어있으나 startup control단계에서는
 * Membase가 로드되지 않은 상태이므로 데이터베이스 복구중에는 
 * DBFilePageCnt값을 확인 할수 없다. incremental backup 파일을 이용한 복구를
 * 하기위해 DBFilePageCnt값을 backup info slot에 저장하고 이를 이용하여
 * 복구한다
 *
 **********************************************************************/
UInt smrRecoveryMgr::getPageNoInFile( UInt aPageID, smriBISlot * aBISlot)
{
    UInt          sPageNoInFile;   

    IDE_DASSERT( aBISlot != NULL );

    if ( aPageID < SMM_DATABASE_META_PAGE_CNT ) 
    {
         sPageNoInFile = aPageID;   
    }
    else
    {
        IDE_ASSERT( aBISlot->mDBFilePageCnt != 0 ); 

        sPageNoInFile = (UInt)( ( aPageID - SMM_DATABASE_META_PAGE_CNT )
                                    % aBISlot->mDBFilePageCnt );     

        if ( aPageID < (aBISlot->mDBFilePageCnt        
                            + SMM_DATABASE_META_PAGE_CNT) )                                                                                                            
        {
                sPageNoInFile += SMM_DATABASE_META_PAGE_CNT;                                                                                                           
        }
    }

    return sPageNoInFile;
}

/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 * aBISlot  - [IN] 복구하려는 MemDataFile의 backup info slot
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::restoreMemDataFile4Level1( smriBISlot * aBISlot )
{
    iduFile             sBackupFile;
    smmChkptImageHdr    sChkptImageHdr;
    smmTBSNode        * sMemSpaceNode;
    smmDatabaseFile   * sDatabaseFile;
    smmDatabaseFile     sNxtStableDatabaseFile;
    UChar             * sCopyBuffer;
    size_t              sReadOffset;
    size_t              sWriteOffset;
    UInt                sIBChunkCnt;
    UInt                sPageNoInFile;
    UInt                sWhichDB;
    UInt                sNxtStableDB;
    size_t              sCopySize;
    size_t              sReadSize;
    scPageID            sPageID;
    idBool              sIsCreated;
    idBool              sIsMediaFailure;
    UInt                sCPathCount;
    smmChkptPathNode  * sCPathNode;
    SChar               sRestoreFileName[ SMRI_BI_MAX_BACKUP_FILE_NAME_LEN ];
    UInt                sState = 0;
    UInt                sMemAllocState = 0;
    UInt                sDatabaseFileState = 0;
    
    IDE_DASSERT( aBISlot != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aBISlot->mSpaceID,
                                                        (void **)&sMemSpaceNode )
              != IDE_SUCCESS );

    sWhichDB = smmManager::getCurrentDB( sMemSpaceNode );
    
    IDE_TEST( smmManager::getDBFile( sMemSpaceNode,
                                     sWhichDB,
                                     aBISlot->mFileNo,
                                     SMM_GETDBFILEOP_SEARCH_FILE, 
                                     &sDatabaseFile )
                  != IDE_SUCCESS );

    if ( idlOS::strncmp( sDatabaseFile->getFileName(),
                         "\0",
                         SMI_MAX_DATAFILE_NAME_LEN ) == 0 )
    {
        IDE_TEST( smmTBSChkptPath::getChkptPathNodeCount( sMemSpaceNode,
                                                          &sCPathCount )
                  != IDE_SUCCESS );

        IDE_ASSERT( sCPathCount != 0 );

        IDE_TEST( smmTBSChkptPath::getChkptPathNodeByIndex(
                                                sMemSpaceNode,
                                                aBISlot->mFileNo % sCPathCount,
                                                &sCPathNode )
              != IDE_SUCCESS );

        IDE_ASSERT( sCPathNode != NULL );

        idlOS::snprintf( sRestoreFileName,
                         SMRI_BI_MAX_BACKUP_FILE_NAME_LEN,
                         "%s%c%s-%"ID_UINT32_FMT"-%"ID_UINT32_FMT"",
                         sCPathNode->mChkptPathAttr.mChkptPath,
                         IDL_FILE_SEPARATOR,
                         sMemSpaceNode->mHeader.mName,
                         sWhichDB,
                         aBISlot->mFileNo );

        IDE_TEST( sDatabaseFile->setFileName( sRestoreFileName )
                  != IDE_SUCCESS );

        sDatabaseFile->setDir( sCPathNode->mChkptPathAttr.mChkptPath );
    }


    IDE_TEST( sBackupFile.initialize( IDU_MEM_SM_SMR,
                                      1,                             
                                      IDU_FIO_STAT_OFF,              
                                      IDV_WAIT_INDEX_NULL ) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sBackupFile.setFileName( aBISlot->mBackupFileName ) 
              != IDE_SUCCESS );

    IDE_TEST( sBackupFile.open() != IDE_SUCCESS );
    sState = 2;

    if ( ( aBISlot->mBackupType & SMI_BACKUP_TYPE_FULL ) != 0 )
    {
        // copy target 파일이 open된 상태에서 copy하면 안됨
        if ( sDatabaseFile->isOpen() == ID_TRUE )
        {
            IDE_TEST( sDatabaseFile->close() != IDE_SUCCESS );
        }

        IDE_TEST( sBackupFile.copy( NULL, (SChar*)sDatabaseFile->getFileName() )
                  != IDE_SUCCESS );
    }

    if ( sDatabaseFile->isOpen() == ID_FALSE )
    {
        IDE_TEST( sDatabaseFile->open() != IDE_SUCCESS );
        sDatabaseFileState = 1;
    }

    if ( ( aBISlot->mBackupType & SMI_BACKUP_TYPE_FULL ) == 0 )
    {
        IDE_TEST( sBackupFile.read( NULL,
                                    SM_DBFILE_METAHDR_PAGE_OFFSET,
                                    (void *)&sChkptImageHdr,
                                    ID_SIZEOF( smmChkptImageHdr ),
                                    &sReadSize )
                  != IDE_SUCCESS );
 
        sReadOffset = SM_DBFILE_METAHDR_PAGE_SIZE;
 
        smriBackupInfoMgr::clearDataFileHdrBI( &sChkptImageHdr.mBackupInfo );
 
        /* ChkptImageHdr복구 */
        IDE_TEST( sDatabaseFile->mFile.write( NULL,
                                              SM_DBFILE_METAHDR_PAGE_OFFSET,
                                              &sChkptImageHdr,
                                              ID_SIZEOF( smmChkptImageHdr ) )
                  != IDE_SUCCESS );
 
        sCopySize = SM_PAGE_SIZE * aBISlot->mIBChunkSize;
 
        /* smrRecoveryMgr_restoreMemDataFile4Level1_calloc_CopyBuffer.tc */
        IDU_FIT_POINT("smrRecoveryMgr::restoreMemDataFile4Level1::calloc::CopyBuffer");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                     1,
                                     sCopySize,
                                     (void**)&sCopyBuffer )
                  != IDE_SUCCESS );
        sMemAllocState = 1;
 
        /* IBChunk 복원 */
        for( sIBChunkCnt = 0; sIBChunkCnt < aBISlot->mIBChunkCNT; sIBChunkCnt++ )
        {
            IDE_TEST( sBackupFile.read( NULL,
                                        sReadOffset,
                                        (void *)sCopyBuffer,
                                        sCopySize,
                                        &sReadSize )
                      != IDE_SUCCESS );
 
            sPageID = SMP_GET_PERS_PAGE_ID( sCopyBuffer );
 
            sPageNoInFile = getPageNoInFile( sPageID, aBISlot);
 
            sWriteOffset = ( SM_PAGE_SIZE * sPageNoInFile ) 
                                + SM_DBFILE_METAHDR_PAGE_SIZE; 
            
            IDE_TEST( sDatabaseFile->mFile.write( NULL,
                                                  sWriteOffset,
                                                  sCopyBuffer,
                                                  sReadSize )
                      != IDE_SUCCESS );
            sReadOffset += sReadSize;
        }
 
        sMemAllocState = 0;
        IDE_TEST( iduMemMgr::free( sCopyBuffer ) != IDE_SUCCESS );
    }

    IDE_TEST( sDatabaseFile->checkValidationDBFHdr( &sChkptImageHdr,
                                                    &sIsMediaFailure ) 
              != IDE_SUCCESS );

    sNxtStableDB = smmManager::getNxtStableDB( sMemSpaceNode );
    sIsCreated   = smmManager::getCreateDBFileOnDisk( sMemSpaceNode,
                                                      sNxtStableDB,
                                                      aBISlot->mFileNo );

    if ( (sIsCreated == ID_TRUE) && ( sIsMediaFailure == ID_FALSE ) )
    {
       
        IDE_TEST( sNxtStableDatabaseFile.setFileName( 
                                        sDatabaseFile->getFileDir(),
                                        sMemSpaceNode->mHeader.mName,    
                                        sNxtStableDB,
                                        aBISlot->mFileNo )
                != IDE_SUCCESS );

        IDE_TEST( sDatabaseFile->copy( 
                            NULL, 
                            (SChar*)sNxtStableDatabaseFile.getFileName() ) 
                  != IDE_SUCCESS );
    }

    if ( sDatabaseFileState == 1 )
    {
        sDatabaseFileState = 0;
        IDE_TEST( sDatabaseFile->close() != IDE_SUCCESS );
    }

    sState = 1;
    IDE_TEST( sBackupFile.close() != IDE_SUCCESS );
                                    
    sState = 0;
    IDE_TEST( sBackupFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 2:
            IDE_ASSERT( sDatabaseFile->close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sBackupFile.destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    switch( sMemAllocState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sCopyBuffer ) == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    switch( sDatabaseFileState )
    {
        case 1:
            IDE_ASSERT( sDatabaseFile->close() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : PROJ-2133 incremental backup
 *
 * aBISlot  - [IN] 복구하려는 DiskDataFile의 backup info slot
 *
 **********************************************************************/
IDE_RC smrRecoveryMgr::restoreDiskDataFile4Level1( smriBISlot * aBISlot ) 
{
    iduFile             sBackupFile;
    sddDataFileHdr      sDBFileHdr;
    sddTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sDataFileNode = NULL;
    UChar             * sCopyBuffer;
    ULong               sReadOffset;
    ULong               sWriteOffset;
    UInt                sIBChunkCnt;
    size_t              sCopySize;
    size_t              sReadSize;
    ULong               sFileSize = 0;
    size_t              sCurrSize;
    sdpPhyPageHdr     * sPageHdr; 
    UInt                sState         = 0;
    UInt                sMemAllocState = 0;
    UInt                sDataFileState = 0;
    
    IDE_DASSERT( aBISlot != NULL );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aBISlot->mSpaceID,
                                                        (void **)&sSpaceNode )
              != IDE_SUCCESS );
    
    sDataFileNode = sSpaceNode->mFileNodeArr[ aBISlot->mFileID ];
    IDE_DASSERT( sDataFileNode != NULL );

    IDE_TEST( sBackupFile.initialize( IDU_MEM_SM_SMR,    
                                      1,                 
                                      IDU_FIO_STAT_OFF,  
                                      IDV_WAIT_INDEX_NULL ) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sBackupFile.setFileName( aBISlot->mBackupFileName ) 
              != IDE_SUCCESS );

    IDE_TEST( sBackupFile.open() != IDE_SUCCESS );
    sState = 2;

    /* full backup된 incremental backup 복구 */
    if ( ( aBISlot->mBackupType & SMI_BACKUP_TYPE_FULL ) != 0 )
    {
        // copy target 파일이 open된 상태에서 copy하면 안됨
        if ( sDataFileNode->mIsOpened == ID_TRUE )
        {
            IDE_TEST( sddDiskMgr::closeDataFile( NULL,
                                                 sDataFileNode ) != IDE_SUCCESS );
        }
        IDE_ASSERT( sDataFileNode->mIsOpened == ID_FALSE );

        IDE_TEST( sBackupFile.copy( NULL, sDataFileNode->mFile.getFileName() )
                  != IDE_SUCCESS );
    }

    if ( sDataFileNode->mIsOpened == ID_FALSE )
    {
        IDE_TEST( sddDiskMgr::openDataFile( NULL,
                                            sDataFileNode ) != IDE_SUCCESS );
        sDataFileState = 1;
    }

    IDE_TEST( sDataFileNode->mFile.getFileSize(&sFileSize) != IDE_SUCCESS );

    sCurrSize = (sFileSize - SM_DBFILE_METAHDR_PAGE_SIZE) /
                 SD_PAGE_SIZE;
    
    if ( aBISlot->mOriginalFileSize < sCurrSize )
    {
        IDE_TEST( sddDataFile::truncate( sDataFileNode,
                                         aBISlot->mOriginalFileSize ) 
                  != IDE_SUCCESS );

        sddDataFile::setCurrSize( sDataFileNode, sCurrSize );
    
        if ( aBISlot->mOriginalFileSize < sDataFileNode->mInitSize )
        {
            sddDataFile::setInitSize(sDataFileNode,
                                     aBISlot->mOriginalFileSize );
        }
    }
    else
    {
        if ( aBISlot->mOriginalFileSize > sCurrSize )
        {
            sddDataFile::setCurrSize( sDataFileNode, aBISlot->mOriginalFileSize );
        }
        else
        {
            /* 
             * nothing to do 
             * 파일 크기 변화 없음         
             */
        }
    }

    if ( ( aBISlot->mBackupType & SMI_BACKUP_TYPE_FULL ) == 0 )
    {
        IDE_TEST( sBackupFile.read( NULL,
                                    SM_DBFILE_METAHDR_PAGE_OFFSET,
                                    (void *)&sDBFileHdr,
                                    ID_SIZEOF( sddDataFileHdr ),
                                    &sReadSize )
                  != IDE_SUCCESS );
 
        sReadOffset = SM_DBFILE_METAHDR_PAGE_SIZE;
 
        smriBackupInfoMgr::clearDataFileHdrBI( &sDBFileHdr.mBackupInfo );
 
        IDE_TEST( sDataFileNode->mFile.write( NULL,
                                              SM_DBFILE_METAHDR_PAGE_OFFSET,
                                              &sDBFileHdr,
                                              ID_SIZEOF( sddDataFileHdr ) )
                  != IDE_SUCCESS );
 
        sCopySize = SD_PAGE_SIZE * aBISlot->mIBChunkSize;
 
        /* smrRecoveryMgr_restoreDiskDataFile4Level1_calloc_CopyBuffer.tc */
        IDU_FIT_POINT("smrRecoveryMgr::restoreDiskDataFile4Level1::calloc::CopyBuffer");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMR,
                                     1,
                                     sCopySize,
                                     (void**)&sCopyBuffer )
                  != IDE_SUCCESS );
        sMemAllocState = 1;
 
        for( sIBChunkCnt = 0; sIBChunkCnt < aBISlot->mIBChunkCNT; sIBChunkCnt++ )
        {
            IDE_TEST( sBackupFile.read( NULL,
                                        sReadOffset,
                                        (void *)sCopyBuffer,
                                        sCopySize,
                                        &sReadSize )
                    != IDE_SUCCESS );
 
            sPageHdr     = ( sdpPhyPageHdr * )sCopyBuffer;
            sWriteOffset = ( SD_MAKE_FPID(sPageHdr->mPageID) * SD_PAGE_SIZE ) + 
                             SM_DBFILE_METAHDR_PAGE_SIZE ;
 
            IDE_TEST( sDataFileNode->mFile.write( NULL,
                                                  sWriteOffset,
                                                  sCopyBuffer,
                                                  sReadSize )
                      != IDE_SUCCESS );
 
            sReadOffset += sReadSize;
        }
 
        sMemAllocState = 0;
        IDE_TEST( iduMemMgr::free( sCopyBuffer ) != IDE_SUCCESS );
 
    }
    sDataFileState = 0;
    IDE_TEST( sddDiskMgr::closeDataFile( NULL,
                                         sDataFileNode ) != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sBackupFile.close() != IDE_SUCCESS );
                                    
    sState = 0;
    IDE_TEST( sBackupFile.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 2:
            IDE_ASSERT( sBackupFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sBackupFile.destroy() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    switch( sMemAllocState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sCopyBuffer ) == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    switch( sDataFileState )
    {
        case 1:
            IDE_ASSERT( sddDiskMgr::closeDataFile( NULL,
                                                   sDataFileNode ) 
                        == IDE_SUCCESS );
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smrRecoveryMgr::processPrepareReqLog( SChar              * aLogPtr,
                                             smrXaPrepareReqLog * aXaPrepareReqLog,
                                             smLSN              * aLSN )
{
    /* smrRecoveryMgr_processPrepareReqLog_ManageDtxInfoFunc.tc */
    IDU_FIT_POINT( "smrRecoveryMgr::processPrepareReqLog::ManageDtxInfoFunc" );
    IDE_DASSERT( mManageDtxInfoFunc != NULL ); 
    IDE_TEST( mManageDtxInfoFunc( &(aXaPrepareReqLog->mGlobalXID),
                                  smrLogHeadI::getTransID( &(aXaPrepareReqLog->mHead) ),
                                  aXaPrepareReqLog->mGlobalTxId,
                                  aXaPrepareReqLog->mBranchTxSize,
                                  (UChar*)aLogPtr,
                                  aLSN,
                                  NULL, //mCommitSCN
                                  SMI_DTX_PREPARE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrRecoveryMgr::processDtxLog( SChar      * aLogPtr,
                                      smrLogType   aLogType,
                                      smLSN      * aLSN,
                                      idBool     * aRedoSkip )
{
    smrXaPrepareReqLog  sXaPrepareReqLog;
    smrTransCommitLog   sCommitLog;
    smrTransAbortLog    sAbortLog;
    smrXaEndLog         sXaEndLog;

    /* BUG-47525 Group Commit */
    UInt                i;
    UInt                sGCCnt;
    smTID               sTID;
    smTID               sGlobalTID;
    SChar             * sRedoBuffer;
    smrTransGroupCommitLog sGCLog;
    SChar             * sGCCommitSCNBuffer;
    smSCN               sCommitSCN;
    UInt                sLogSize;
    idBool              sIsWithCommitSCN = ID_FALSE;

    /* PROJ-2569 xa_prepare_req로그와 xa_end로그는 sm recovery 과정에 불필요하므로
     * DK에만 넘겨주고 이하 작업은 skip한다. */
    switch ( aLogType )
    {
        case SMR_LT_XA_PREPARE_REQ:

            idlOS::memcpy( &sXaPrepareReqLog,
                           aLogPtr,
                           SMR_LOGREC_SIZE( smrXaPrepareReqLog ) );

            IDE_TEST( processPrepareReqLog( aLogPtr + SMR_LOGREC_SIZE(smrXaPrepareReqLog),
                                            &sXaPrepareReqLog,
                                            aLSN )
                      != IDE_SUCCESS );

            *aRedoSkip = ID_TRUE;

            break;

        case SMR_LT_XA_END:

            idlOS::memcpy( &sXaEndLog,
                           aLogPtr,
                           SMR_LOGREC_SIZE( smrXaEndLog ) );

            IDE_DASSERT( mManageDtxInfoFunc != NULL ); 
            IDE_TEST( mManageDtxInfoFunc( NULL,
                                          smrLogHeadI::getTransID( &(sXaEndLog.mHead) ),
                                          sXaEndLog.mGlobalTxId,
                                          0,
                                          NULL,
                                          NULL,
                                          NULL, //mCommitSCN
                                          SMI_DTX_END )
                      != IDE_SUCCESS );

            *aRedoSkip = ID_TRUE;

            break;

        case SMR_LT_MEMTRANS_COMMIT:
        case SMR_LT_DSKTRANS_COMMIT:

            idlOS::memcpy( &sCommitLog,
                           aLogPtr,
                           SMR_LOGREC_SIZE( smrTransCommitLog ) );

            if ( sCommitLog.mGlobalTxId != SM_INIT_GTX_ID )
            {
                IDE_DASSERT( mManageDtxInfoFunc != NULL ); 
                IDE_TEST( mManageDtxInfoFunc( NULL,
                                              smrLogHeadI::getTransID( &(sCommitLog.mHead) ),
                                              sCommitLog.mGlobalTxId,
                                              0,
                                              NULL,
                                              NULL,
                                              &sCommitLog.mCommitSCN,
                                              SMI_DTX_COMMIT )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            break;

        case SMR_LT_MEMTRANS_GROUPCOMMIT:

            idlOS::memcpy( &sGCLog,  aLogPtr, ID_SIZEOF( smrTransGroupCommitLog ) );
            sGCCnt = sGCLog.mGroupCnt;

            sRedoBuffer = aLogPtr + SMR_LOGREC_SIZE( smrTransGroupCommitLog );

            sLogSize = SMR_LOGREC_SIZE(smrTransGroupCommitLog) + 
                       ( sGCCnt * ID_SIZEOF(smTID) * 2 ) + 
                       ID_SIZEOF(smrLogTail);

            if ( smrLogHeadI::getSize(&sGCLog.mHead) > sLogSize )
            {
                /* CommitSCN 을 기록했으면 당연히 smSCN 크기의 배수 */
                IDE_DASSERT( ( ( smrLogHeadI::getSize(&sGCLog.mHead) - sLogSize ) % ID_SIZEOF(smSCN) ) == 0 );

                /* TID1 | GlobalTID1 | TID2 | GlobalTI2D | CommitSCN1 | CommitSCN2 | 로 생겼음. */
                sGCCommitSCNBuffer = sRedoBuffer + ( sGCCnt * ID_SIZEOF(smTID) * 2 );

                sIsWithCommitSCN = ID_TRUE;
            }
            else
            {
                sIsWithCommitSCN = ID_FALSE;
            }

            for ( i = 0 ; i < sGCCnt ; i++ )
            {
                idlOS::memcpy( &sTID,
                               sRedoBuffer,
                               ID_SIZEOF(smTID) );
                sRedoBuffer += ID_SIZEOF(smTID);

                idlOS::memcpy( &sGlobalTID,
                               sRedoBuffer,
                               ID_SIZEOF(smTID) );
                sRedoBuffer += ID_SIZEOF(smTID);
    
                if ( sIsWithCommitSCN == ID_TRUE )
                {
                    idlOS::memcpy( &sCommitSCN,
                                   sGCCommitSCNBuffer,
                                   ID_SIZEOF(smSCN) );
                    sGCCommitSCNBuffer += ID_SIZEOF(smSCN);
                }
                else
                {
                    SM_INIT_SCN( &sCommitSCN );     
                }

                if ( sGlobalTID != SM_INIT_GTX_ID )
                {
                    IDE_DASSERT( mManageDtxInfoFunc != NULL ); 
                    IDE_TEST( mManageDtxInfoFunc( NULL,
                                                  sTID,
                                                  sGlobalTID,
                                                  0,
                                                  NULL,
                                                  NULL,
                                                  &sCommitSCN,
                                                  SMI_DTX_COMMIT )
                              != IDE_SUCCESS );
                }
            }

            break;

        case SMR_LT_MEMTRANS_ABORT:
        case SMR_LT_DSKTRANS_ABORT:

            idlOS::memcpy( &sAbortLog,
                           aLogPtr,
                           SMR_LOGREC_SIZE( smrTransAbortLog ) );

            if ( sAbortLog.mGlobalTxId != SM_INIT_GTX_ID )
            {
                IDE_DASSERT( mManageDtxInfoFunc != NULL ); 
                IDE_TEST( mManageDtxInfoFunc( NULL,
                                              smrLogHeadI::getTransID( &(sAbortLog.mHead) ),
                                              sAbortLog.mGlobalTxId,
                                              0,
                                              NULL,
                                              NULL,
                                              NULL,  //mCommitSCN
                                              SMI_DTX_ROLLBACK )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }

            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

