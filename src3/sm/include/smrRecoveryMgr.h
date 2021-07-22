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
 * $Id: smrRecoveryMgr.h 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

/***********************************************************************
 *
 * Description :
 *
 * 본 파일은 복구 관리자에 대한 헤더 파일이다.
 *
 * # 기능
 *   1. 로그앵커 관리 (생성/초기화/설정)
 *   2. 로그파일 관리자 쓰레드 및 체크포인트 쓰레드 초기화
 *   3. MRDB의 Dirty-Page 관리
 *   4. 체크포인트 수행
 *   5. Restart Recovery 수행
 *   6. 트랜잭션 rollback 수행
 *   7. **Prj-1149 : media recovery 수행
 **********************************************************************/

#ifndef _O_SMR_RECOVERY_MGR_H_
#define _O_SMR_RECOVERY_MGR_H_ 1

#include <idu.h>
#include <idp.h>
#include <smrDef.h>
#include <smriDef.h>
#include <smrLogAnchorMgr.h>
#include <smrDirtyPageList.h>
#include <smrLogHeadI.h>
#include <smrCompareLSN.h>

// BUG-38503
#define SMR_DLT_REDO_NONE       (0)
#define SMR_DLT_REDO_EXT_DBF    (1)
#define SMR_DLT_REDO_ALL_DLT    (2)

class  idxMarshal;
class  smrLogFile;
class  smrLogAnchorMgr;
class  smrDirtyPageList;

class smrRecoveryMgr
{

public:

    static IDE_RC create();     // 로그파일 및 loganchor 파일 생성
    static IDE_RC initialize(); // 초기화
    static IDE_RC destroy();    // 해제

    static IDE_RC finalize();

    /* 재시작 수행 */
    static IDE_RC restart( UInt aPolicy );

    /* 트랜잭션 수행 */
    static IDE_RC undoTrans( idvSQL* aStatistics,
                             void*   aTrans,
                             smLSN*  aLSN );

    /* Hashing된 Disk Log들을 적용 */
    static IDE_RC applyHashedDiskLogRec( idvSQL* aStatistics );

    // PRJ-1548 User Memory Tablespace

    // 메모리/디스크 TBS 속성을 로그앵커에 갱신한다.
    static IDE_RC updateTBSNodeToAnchor( sctTableSpaceNode*  aSpaceNode );
    // 디스크 데이타파일 속성을 로그앵커에 갱신한다.
    static IDE_RC updateDBFNodeToAnchor( sddDataFileNode*    aFileNode );
    // 메모리 데이타파일 속성을 로그앵커에 갱신한다.
    static IDE_RC updateChkptImageAttrToAnchor(
                                 smmCrtDBFileInfo   * aCrtDBFileInfo,
                                 smmChkptImageAttr  * aChkptImageAttr );

    static IDE_RC updateSBufferNodeToAnchor( sdsFileNode  * aFileNode );
    // Chkpt Path 속성을 Loganchor에 변경한다.
    static IDE_RC updateChkptPathToLogAnchor(
                      smmChkptPathNode * aChkptPathNode );

    // 메모리/디스크 테이블스페이스 속성을 로그앵커에 추가한다.
    static IDE_RC addTBSNodeToAnchor( sctTableSpaceNode*   aSpaceNode );

    // 디스크 데이타파일 속성을 로그앵커에 추가한다.
    static IDE_RC addDBFNodeToAnchor( sddTableSpaceNode*   aSpaceNode,
                                      sddDataFileNode*     aFileNode );

    // 메모리 체크포인트 패스 속성을 로그앵커에 추가한다.
    static IDE_RC addChkptPathNodeToAnchor( smmChkptPathNode * aChkptPathNode );

    // 메모리 데이타파일 속성을 로그앵커에 추가한다.
    static IDE_RC addChkptImageAttrToAnchor(
                                 smmCrtDBFileInfo   * aCrtDBFileInfo,
                                 smmChkptImageAttr  * aChkptImageAttr );


    static IDE_RC addSBufferNodeToAnchor( sdsFileNode*   aFileNode );

    // loganchor에 tablespace 정보 flush
    static IDE_RC updateAnchorAllTBS();


    static IDE_RC updateAnchorAllSB( void );
    
    // loganchor에 archive 로그 모드 flush
    static IDE_RC updateArchiveMode(smiArchiveMode aArchiveMode);

    // loganchor에 TXSEG Entry 개수를 반영한다.
    static IDE_RC updateTXSEGEntryCnt( UInt sEntryCnt );

    /* checkpoint by buffer flush thread */
    static IDE_RC checkpointDRDB(idvSQL* aStatistics);

    /* checkpoint 수행 */
    static IDE_RC checkpoint( idvSQL       * aStatistics,
                              SInt           aCheckpointReason,
                              smrChkptType   aChkptType,
                              idBool         aRemoveLogFile = ID_TRUE,
                              idBool         aFinal         = ID_FALSE );

    /* Tablespace에 Checkpoint가 가능한지 검사 */
    static idBool isCheckpointable( sctTableSpaceNode * aSpaceNode );



    /* prepare 트랜잭션에 대한 table lock을 원복 */
    static IDE_RC acquireLockForInDoubt();

    // 로그관리자의 상태 반환
    static inline idBool isFinish()  { return mFinish;  } /* 로그관리자의 상태 반환*/
    static inline idBool isRestart() { return mRestart; } /* restart 진행중 여부 */
    static inline idBool isRedo() { return mRedo; }       /* redo 진행중 여부 */

    // 현재 Recovery중인지 여부를 리턴한다.
    static inline idBool isRestartRecoveryPhase() { return mRestartRecoveryPhase; } /* Restart Recovery 진행중인지 여부 */
    static inline idBool isMediaRecoveryPhase()   { return mMediaRecoveryPhase; }   /* 미디어 복구 수행 중 여부 */

    static inline idBool isVerifyIndexIntegrityLevel2();

    // PR-14912
    static inline idBool isRefineDRDBIdx() { return mRefineDRDBIdx; }

    /* 시작로그 여부 반환 */
    static inline idBool isBeginLog( smrLogHead* aLogHead );

    /* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다. */
    static void setCallbackFunction(
                    smGetMinSN                   aGetMinSN,
                    smIsReplCompleteBeforeCommit aIsReplCompleteBeforeCommitFunc,
                    smIsReplCompleteAfterCommit  aIsReplCompleteAfterCommitFunc,
                    smCopyToRPLogBuf             aCopyToRPLogBufFunc,
                    smSendXLog                   aSendXLogFunc,
                    smIsReplWaitGlobalTxAfterPrepare aIsReplWaitGlobalTxAfterPrepareFunc );

    static inline smLSN  getIdxSMOLSN()       { return mIdxSMOLSN;}
    static inline idBool isABShutDown()       { return mABShutDown; }

    static inline smrLogAnchorMgr *getLogAnchorMgr() { return &mAnchorMgr; }

    static UInt getLstDeleteLogFileNo();

    /* PROJ-2102 */
    static void getEndLSN(smLSN* aLSN)
    { 
        mAnchorMgr.getEndLSN( aLSN );
    }

    static smiArchiveMode getArchiveMode();

    /************************************
    * PROJ-2133 incremental backup begin
    ************************************/
    static idBool isCTMgrEnabled();
 
    static idBool isCreatingCTFile();

    static smriBIMgrState getBIMgrState();

    static SChar* getCTFileName();
    static SChar* getBIFileName();

    static smLSN getCTFileLastFlushLSNFromLogAnchor();
    static smLSN getBIFileLastBackupLSNFromLogAnchor();
    static smLSN getBIFileBeforeBackupLSNFromLogAnchor();

    static IDE_RC changeIncrementalBackupDir( SChar * aNewBackupDir );

    static SChar *  getIncrementalBackupDirFromLogAnchor();

    static IDE_RC updateCTFileAttrToLogAnchor(                        
                                      SChar          * aFileName,    
                                      smriCTMgrState * aCTMgrState,  
                                      smLSN          * aFlushLSN );

    static IDE_RC updateBIFileAttrToLogAnchor(                        
                                      SChar          * aFileName,    
                                      smriBIMgrState * aBIMgrState,  
                                      smLSN          * aBackupLSN,
                                      SChar          * aBackupDir,
                                      UInt           * aDeleteArchLogFile );

    static IDE_RC getDataFileDescSlotIDFromLogAncho4ChkptImage(     
                                UInt                       aReadOffset,
                                smiDataFileDescSlotID    * aDataFileDescSlotID);

    static IDE_RC getDataFileDescSlotIDFromLogAncho4DBF(     
                                UInt                       aReadOffset,
                                smiDataFileDescSlotID    * aDataFileDescSlotID);
    /************************************
    * PROJ-2133 incremental backup end
    ************************************/

    static UInt getTXSEGEntryCnt();
    static UInt getSmVersionIDFromLogAnchor();
    static void getDiskRedoLSNFromLogAnchor(smLSN* aDiskRedoLSN);

    // prj-1149 checkpoint 수행
    static IDE_RC makeBufferChkpt( idvSQL      * aStatistics,
                                   idBool        aIsFinal,
                                   smLSN       * aEndLSN,
                                   smLSN       * aOldestLSN );

    //* Sync Replication extension * //
    /* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다. */
    static smIsReplCompleteBeforeCommit mIsReplCompleteBeforeCommitFunc; // Sync LSN wait for (Before Write CommitLog)
    static smIsReplCompleteAfterCommit  mIsReplCompleteAfterCommitFunc;  // Sync LSN wait for (After Write CommitLog)

    /* PRJ-2747 Global Tx Consistent */
    static smIsReplWaitGlobalTxAfterPrepare mIsReplWaitGlobalTxAfterPrepareFunc;

    /*PROJ-1670 RP LOG Buffer*/
    static smCopyToRPLogBuf   mCopyToRPLogBufFunc;
    /* PROJ-2453 Eager Replication performance enhancement */ 
    static smSendXLog   mSendXLogFunc;
    /* Log File삭제시 mDeleteLogFileMutex를 잡아야 한다. 왜냐하면 Replication의
       LogFile을 Scaning 하면서 자신의 실제로 보내는 위치를 찾을때 log파일이 삭제되는
       것을 방지하기 위해 삭제시 mDeleteLogFileMutex에 대해서 lock을 잡고 또한
       Replication이 보내야 될 첫번째 파일을 찾을 때 잡아야 한다.*/
    static inline IDE_RC lockDeleteLogFileMtx()
                                     { return mDeleteLogFileMutex.lock( NULL  ); }
    static inline IDE_RC unlockDeleteLogFileMtx()  
                                           { return mDeleteLogFileMutex.unlock();}
    static inline void getLstDeleteLogFileNo(UInt *aFileNo) 
                                    { mAnchorMgr.getLstDeleteLogFileNo(aFileNo); }
    /*proj-1608 recovery from replication*/
    static inline smLSN getReplRecoveryLSN() 
                                       { return mAnchorMgr.getReplRecoveryLSN(); }
    static inline IDE_RC setReplRecoveryLSN( smLSN aReplRecoveryLSN )
                  { return mAnchorMgr.updateReplRecoveryLSN( aReplRecoveryLSN ); }
    // Decompress Log Buffer 크기가
    // 프로퍼티에 지정된 값보다 클 경우 Hashing된 Disk Log들을 적용
    static IDE_RC checkRedoDecompLogBufferSize() ;

    /////////////////////////////////////////////////////////////////
    // ALTER TABLESPACE ONLINE/OFFLINE관련 함수

    // ALTER TABLESPACE OFFLINE을 위해
    // 모든 Tablespace의 모든 Dirty Page를 Flush
    static IDE_RC flushDirtyPages4AllTBS();

    // 특정 Tablespace에 Dirty Page가 없음을 확인한다.
    static IDE_RC assertNoDirtyPagesInTBS( smmTBSNode * aTBSNode );

    /////////////////////////////////////////////////////////////////
    // 미디어 복구 관련 함수

    /* 중간에 빠진 로그파일이 있는지 검사 */
    static IDE_RC identifyLogFiles();

    /* 미디어복구가 필요한지 검사 */
    static IDE_RC identifyDatabase( UInt aActionFlag );

    // PRJ-1149
    static IDE_RC recoverDB(idvSQL*           aStatistics,
                            smiRecoverType    aRecvType,
                            UInt              aUntilTIME);

    // incomplete media recovery 이후 로그파일 reset 수행
    static IDE_RC resetLogFiles();

    // resetlogfile 과정에서 로그파일을 삭제한다.
    static IDE_RC deleteLogFiles( SChar   * aDirPath,
                                  UInt      aBeginLogFileNo );

    static idBool isCheckpointFlushNeeded(smLSN aLastWrittenLSN);

    // SKIP할 REDO로그인지 여부를 리턴한다.
    static idBool isSkipRedo( scSpaceID aSpaceID,
                              idBool    aUsingTBSAttr = ID_FALSE);

    static void writeDebugInfo();

    /* DRDB Log를 분석하여 DRDB RedoLog의 시작 위치와 길이를
     * 반환한다. */
    static void getDRDBRedoBuffer( smrLogType    aLogType,
                                   UInt          aLogTotalSize,
                                   SChar       * aLogPtr,
                                   UInt        * aRedoLogSize,
                                   SChar      ** aRedoBuffer );

    /*****************************************************************
     * PROJ-2162 RestartRiskReduction
     *
     * 주요 함수들
     *****************************************************************/
    /* Log를 받아 분석하여 이 Log가 Recovery할 대상 객체들을 추출합니다. */
    static void prepareRTOI( void                * aLogPtr,
                             smrLogHead          * aLogHeadPtr,
                             smLSN               * aLSN,
                             sdrRedoLogData      * aRedoLogData,
                             UChar               * aPagePtr,
                             idBool                aIsRedo,
                             smrRTOI             * aObj );

    /* TargetObject를 받아 Inconsistent한지 체크. */
    static void checkObjectConsistency( smrRTOI * aObj,
                                        idBool  * aConsistency);
    /* Recovery가 실패한 상황입니다. */
    static IDE_RC startupFailure( smrRTOI * aObj,
                                  idBool    aIsRedo );

    /* TargetObject를 Inconsistent하다고 설정함 */
    static void setObjectInconsistency( smrRTOI * aObj,
                                        idBool    aIsRedo );

    /*****************************************************************
     * PROJ-2162 RestartRiskReduction
     *
     * get/set/check 함수들
     *****************************************************************/
    /* RTOI 객체를 초기화한다. */
    static void initRTOI( smrRTOI * aObj );

    /* DRDB Wal이 깨졌는지 확인합니다. */
    static IDE_RC checkDiskWAL();

    /* MRDB Wal이 깨졌는지 확인합니다. */
    static IDE_RC checkMemWAL();

    /* DB Inconsistency 설정을 위한 준비를 합니다.
     * Flusher를 멈춥니다. */
    static IDE_RC prepare4DBInconsistencySetting();

    /* DB Inconsistency 설정을 마무리 합니다.
     * Flusher를 재구동합니다. */
    static IDE_RC finish4DBInconsistencySetting();

    /* Idempotent한 Log인지 확인합니다. */
    static idBool isIdempotentLog( smrLogType sLogType );

    /* Property에서 무시하라고 한 객체인지 조사함 */
    static idBool isIgnoreObjectByProperty( smrRTOI * aObj);

    /* RTOI가 Inconsistency 한지 체크하는 실제 함수 */
    static idBool checkObjectConsistencyInternal( smrRTOI * aObj );

    /* Emergency Startup 과정 후 조회를 위한 get 함수 */
    static smrRTOI * getIOLHead()            { return &mIOLHead; }
    static idBool    getDBDurability()       { return mDurability; }
    static idBool    getDRDBConsistency()    { return mDRDBConsistency; }
    static idBool    getMRDBConsistency()    { return mMRDBConsistency; }
    static idBool    getConsistency()
    {
        if ( ( mDRDBConsistency == ID_TRUE ) &&
             ( mMRDBConsistency == ID_TRUE ) )
        {
            return ID_TRUE;
        }
        return ID_FALSE;
    }
    static smrEmergencyRecoveryPolicy getEmerRecovPolicy()
        { return mEmerRecovPolicy; }

    /*****************************************************************
     * PROJ-2162 RestartRiskReduction
     *
     * Undo/Refine 등 실제 연산 함수등에서 실패를 처리하는 경우
     *****************************************************************/

    /* Undo실패에 대한 RTOI를 준비함 */
    static void prepareRTOIForUndoFailure( void        * aTrans,
                                           smrRTOIType   aType,
                                           smOID         aTableOID,
                                           UInt          aIndexID,
                                           scSpaceID     aSpaceID,
                                           scPageID      aPageID );

    /* Refine실패로 해당 Table이 Inconsistent 해짐 */
    static IDE_RC refineFailureWithTable( smOID   aTableOID );

    /* Refine실패로 해당 Index가 Inconsistent 해짐 */
    static IDE_RC refineFailureWithIndex( smOID   aTableOID,
                                          UInt    aIndexID );

    /*****************************************************************
     * PROJ-2162 RestartRiskReduction
     *
     * IOL(InconsistentObjectList)을 다루는 함수들
     *****************************************************************/
    /* IOL를 초기화 합니다. */
    static IDE_RC initializeIOL();

    /* IOL를 추가 합니다. */
    static void addIOL( smrRTOI * aObj );

    /* 이미 등록된 객체가 있는지, IOL에서 찾습니다. */
    static idBool findIOL( smrRTOI * aObj);

    /* RTOI Message를 TRC/isql에 출력합니다. */
    static void displayRTOI( smrRTOI * aObj );

    /* Redo시점에 등록은 되었지만 미뤄둔 Inconsistency를 설정함 */
    static void applyIOLAtRedoFinish();

    /* IOL를 제거 합니다. */
    static IDE_RC finalizeIOL();
    /*****************************************************************
     * PROJ-2162 RestartRiskReduction End
     *****************************************************************/

    
    /************************************
    * PROJ-2133 incremental backup begin
    ************************************/
    static IDE_RC restoreDB( smiRestoreType    aRestoreType,
                             UInt              aUntilTIME,  
                             SChar           * aUntilBackupTag );

    static IDE_RC restoreTBS( scSpaceID aSpaceID );


    static IDE_RC restoreDataFile4Level0( smriBISlot * aBISlot );

    static IDE_RC restoreDB4Level1( UInt            aRestoreStartSlotIdx, 
                                    smiRestoreType  aRestoreType,
                                    UInt            aUntilTime,
                                    SChar         * aUntilBackupTag,
                                    smriBISlot   ** aLastRestoredBISlot );

    static IDE_RC restoreTBS4Level1( scSpaceID aSpaceID,
                                     UInt      aRestoreStartBISlotIdx,
                                     UInt      aBISlotCnt );

    static UInt getPageNoInFile( UInt aPageID, smriBISlot * aBISlot);

    static IDE_RC restoreMemDataFile4Level1( smriBISlot * aBISlot );

    static IDE_RC restoreDiskDataFile4Level1( smriBISlot * aBISlot );

    static UInt getCurrMediaTime()
        { return mCurrMediaTime; }

    static void setCurrMediaTime ( UInt aRestoreCompleteTime )
        { mCurrMediaTime = aRestoreCompleteTime; }

    static SChar * getLastRestoredTagName()
        { return mLastRestoredTagName; }
    
    static void setLastRestoredTagName( SChar * aTagName )
        { idlOS::strcpy( mLastRestoredTagName, aTagName ); }

    static void initLastRestoredTagName()
        { 
            idlOS::memset( mLastRestoredTagName, 
                           0x00, 
                           SMI_MAX_BACKUP_TAG_NAME_LEN ); 
        }
    /************************************
    * PROJ-2133 incremental backup end
    ************************************/

    // Memory Dirty Page들을 Checkpoint Image에 Flush한다.
    // END CHKPT LOG 이전에 수행하는 작업
    // PROJ-1923 private -> public 으로 변경
    static IDE_RC chkptFlushMemDirtyPages( smLSN          * aSyncLstLSN,
                                           idBool           aIsFinal );
    /* PROJ-2569 */
    static inline void set2PCCallback( smGetDtxMinLSN aGetDtxMinLSN,
                                       smManageDtxInfo aManageDtxInfo );

    static smGetDtxMinLSN    mGetDtxMinLSNFunc;
    static smManageDtxInfo   mManageDtxInfoFunc;
    static IDE_RC processPrepareReqLog( SChar * aLogPtr,
                                        smrXaPrepareReqLog * aXaPrepareReqLog,
                                        smLSN * aLSN );
    static IDE_RC processDtxLog( SChar      * aLogPtr,
                                 smrLogType   aLogType,
                                 smLSN      * aLSN,
                                 idBool     * aRedoSkip );

    /* BUG-42785 OnlineDRDBRedo와 LogFileDeleteThread의 동시성 제어
     * mOnlineDRDBRedoCnt 변수에대한 제어 */
    static SInt getOnlineDRDBRedoCnt()
    {
        return idCore::acpAtomicGet32( &mOnlineDRDBRedoCnt );
    }
    static IDE_RC incOnlineDRDBRedoCnt()
    {
        SInt sOnlineDRDBRedoCnt;

        sOnlineDRDBRedoCnt = idCore::acpAtomicInc32( &mOnlineDRDBRedoCnt );

        IDE_TEST( sOnlineDRDBRedoCnt > ID_SINT_MAX );

        return IDE_SUCCESS;

        IDE_EXCEPTION_END;
        
        return IDE_FAILURE;
    }
    static IDE_RC decOnlineDRDBRedoCnt()
    {
        SInt sOnlineDRDBRedoCnt;

        sOnlineDRDBRedoCnt = idCore::acpAtomicDec32( &mOnlineDRDBRedoCnt );

        IDE_TEST( sOnlineDRDBRedoCnt < 0 );
        
        return IDE_SUCCESS;
        
        IDE_EXCEPTION_END;

        return IDE_FAILURE;
    }

    static void getLastRemovedFileNo( UInt *aFailNo )
    {
        *aFailNo = mLastRemovedFileNo;
    }

private:

    static IDE_RC applyDskLogInstantly4ActiveTrans( void        * aCurTrans,
                                                    SChar       * aLogPtr,
                                                    smrLogHead  * aLogHeadPtr,
                                                    smLSN       * aCurRedoLSNPtr );

    // 로그앵커에 resetloglsn이 설정되어 있는지 확인한다.
    static IDE_RC checkResetLogLSN( UInt aActionFlag );

    /* 복구대상 파일목록 생성 */
    static IDE_RC makeMediaRecoveryDBFList4AllTBS(
                                smiRecoverType    aRecoveryType,
                                UInt            * aFailureMediaType,
                                smLSN           * aFromDiskRedoLSN,
                                smLSN           * aToDiskRedoLSN,
                                smLSN           * aFromMemRedoLSN,
                                smLSN           * aToMemRedoLSN );

    // PROJ-1867 백업 되었던 DBFile에서 MustRedoToLSN값을 가져온다.
    static IDE_RC getMaxMustRedoToLSN( idvSQL     * aStatistics,
                                       smLSN      * aMustRedoToLSN,
                                       SChar     ** sDBFileName );

    // 메모리 데이타파일의 헤더를 복구한다
    static IDE_RC repairFailureChkptImageHdr( smLSN  * aResetLogsLSN );

    // 메모리 테이블스페이스의 미디어복구를 위해
    // 테이블스페이스를 로딩한다.
    static IDE_RC initMediaRecovery4MemTBS();

    // 메모리 테이블스페이스의 미디어복구를 위해
    // 테이블스페이스를 메모리해제한다.
    static IDE_RC finalMediaRecovery4MemTBS();

    // 미디어복구 완료시 메모리 Dirty Pages를 모두 Flush 시킨다.
    static IDE_RC flushAndRemoveDirtyPagesAllMemTBS();

    // To Fix PR-13786
    static IDE_RC recoverAllFailureTBS( smiRecoverType       aRecoveryType,
                                        UInt                 aFailureMediaType,
                                        time_t             * aUntilTIME,
                                        smLSN              * aCurRedoLSNPtr,
                                        smLSN              * aFromDiskRedoLSN,
                                        smLSN              * aToDiskRedoLSN,
                                        smLSN              * aFromMemRedoLSN,
                                        smLSN              * aToMemRedoLSN );

    // 미디어복구시 판독할 Log LSN의 scan할 구간을 얻는다
    static void getRedoLSN4SCAN( smLSN * aFromDiskRedoLSN,
                                 smLSN * aToDiskRedoLSN,
                                 smLSN * aFromMemRedoLSN,
                                 smLSN * aToMemRedoLSN,
                                 smLSN * aMinFromRedoLSN );

    // 판독된 로그가 common 로그타입인지 확인한다.
    static IDE_RC filterCommonRedoLogType( smrLogType   aLogType,
                                           UInt         aFailureMediaType,
                                           idBool     * aIsApplyLog );

    // 판독된 로그가 오류난 메모리 데이타파일의 범위에 포함되는지
    // 판단하여 적용여부를 반환한다.
    static IDE_RC filterRedoLog4MemTBS( SChar      * aCurLogPtr,
                                        smrLogType   aLogType,
                                        idBool     * aIsApplyLog );

    // 판독된 로그가 디스크 로그타입인지를 우선확인하고 적용여부를
    // 판단하여 반환한다.
    static IDE_RC filterRedoLogType4DiskTBS( SChar      * aCurLogPtr,
                                             smrLogType   aLogType,
                                             UInt         aIsNeedApplyDLT,
                                             idBool     * aIsApplyLog );

    //   fix BUG-17157 [PROJ-1548] Disk Tablespace Online/Offline
    static IDE_RC finiOfflineTBS( idvSQL * aStatistics );

    /* ------------------------------------------------
     * !!] system restart 관련 함수
     * ----------------------------------------------*/
    static IDE_RC initRestart(idBool* aIsNeedRecovery);
    static IDE_RC restartNormal();   /* 복구가 필요없는 restart */
    static IDE_RC restartRecovery(); /* 복구가 필요한 restart */
    static void   finalRestart();    /* restart 과정 마무리 */

    static IDE_RC redo( void        * aCurTrans,
                        smLSN       * aCurLSN,
                        UInt        * aFileCount,
                        smrLogHead  * aLogHead,
                        SChar       * aLogPtr,
                        UInt          aLogSizeAtDisk,
                        idBool        aAfterChkpt );

    static IDE_RC addActiveTrans( smrLogHead    * aLogHeadPtr,
                                  SChar         * aLogPtr,
                                  smTID           aTID,
                                  smLSN         * aCurRedoLSNPtr,
                                  void         ** aCurTrans );

    // To Fix PR-13786

    static IDE_RC redo_FILE_END( smLSN      * aCurLSN,
                                 UInt       * aFileCount );


    /* restart recovery 과정의 재수행 과정 수행 */
    static IDE_RC redoAll(idvSQL* aStatistics);

    /* restart recovery 과정의 undoall pass 수행 */
    static IDE_RC undoAll(idvSQL* aStatistics);

    /* 트랜잭션 철회 */
    static IDE_RC undo( idvSQL*       aStatistics,
                        void*         aTrans,
                        smrLogFile**  aLogFilePtr);

    static IDE_RC rebuildArchLogfileList( smLSN  *aEndLSN );

    /* ------------------------------------------------
     * To Fix PR-13786 Cyclomatic Number
     * CHECKPOINT 관련 함수
     * ----------------------------------------------*/
    // Checkpoint 구현 함수 - 실제 Checkpoint를 수행한다.
    static IDE_RC checkpointInternal( idvSQL*      aStatistics,
                                      smrChkptType aChkptType,
                                      idBool       aRemoveLogFile,
                                      idBool       aFinal );

    // Checkpoint를 왜 하게 되었는지를 altibase_sm.log에 남긴다.
    static IDE_RC logCheckpointReason( SInt aCheckpointReason );

    // Checkpoint 요약 Message를 로깅한다.
    static IDE_RC logCheckpointSummary( smLSN   aBeginChkptLSN,
                                        smLSN   aEndChkptLSN,
                                        smLSN * aRedoLSN,
                                        smLSN   aDiskRedoLSN );

    // Begin Checkpoint Log를 기록한다.
    static IDE_RC writeBeginChkptLog( idvSQL* aStatistics,
                                      smLSN * aRedoLSN,
                                      smLSN   aDiskRedoLSN,
                                      smLSN   aEndLSN,
                                      smLSN * aBeginChkptLSN,
                                      smLSN * aDtxMinLSN,
                                      idBool  aFinal );


    // End Checkpoint Log를 기록한다.
    static IDE_RC writeEndChkptLog( idvSQL* aStatistics,
                                    smLSN * aEndChkptLSN );

    // Restart Recovery시에 Redo시작 LSN으로 사용될 Recovery LSN을 계산
    // BEGIN CHKPT LOG 이전에 수행하는 작업
    static IDE_RC chkptCalcRedoLSN( idvSQL*            aStatistics,
                                    smrChkptType       aChkptType,
                                    idBool             aIsFinal,
                                    smLSN            * aRedoLSN,
                                    smLSN            * aDiskRedoLSN,
                                    smLSN            * aEndLSN );

    // 모든 테이블스페이스의 데이타파일 메타헤더에 RedoLSN 설정
    static IDE_RC chkptSetRedoLSNOnDBFiles( idvSQL* aStatistics,
                                            smLSN * aRedoLSN,
                                            smLSN   aDiskRedoLSN );

    // END CHKPT LOG 이후에 수행하는 작업
    static IDE_RC chkptAfterEndChkptLog( idBool             aRemoveLogFile,
                                         idBool             aFinal,
                                         smLSN            * aBeginChkptLSN,
                                         smLSN            * aEndChkptLSN,
                                         smLSN            * aDiskRedoLSN,
                                         smLSN            * aRedoLSN,
                                         smLSN            * aSyncLstLSN,
                                         smLSN            * aDtxMinLSN );

    // BUG-20229
    static IDE_RC resizeLogFile(SChar    *aLogFileName,
                                ULong     aLogFileSize);

    static idBool existDiskSpace4LogFile(void);

    static void updateLastRemovedFileNo( UInt aFileNo )
    {
        mLastRemovedFileNo = aFileNo ;
    }

private:

    static smrLogAnchorMgr    mAnchorMgr;  /* loganchor 관리자 */
    static smrDirtyPageList   mDPPList;    /* dirty page list */
    static sdbFlusher         mFlusher;
    static idBool             mFinish;     /* 로그관리자의 해제여부 */
    static idBool             mABShutDown; /* 비정상 종료의 여부 */

    /* restart 진행중 여부 */
    static idBool             mRestart;

    /* redo 진행중 여부 */
    static idBool             mRedo;

    /* 미디어 복구 수행 중 여부 */
    static idBool             mMediaRecoveryPhase;

    /* Restart Recovery 진행중인지 여부 */
    static idBool             mRestartRecoveryPhase;

    // PR-14912
    static idBool             mRefineDRDBIdx;

    static UInt               mLogSwitch;
    /* Replication을 위해서 필요한 로그 레코드들의 가장
       작은 SN값을 가져온다. */
    static smGetMinSN         mGetMinSNFunc;

    /* for index smo */
    static smLSN              mIdxSMOLSN;

    /* Log File Delete시 잡는 Mutex */
    static iduMutex           mDeleteLogFileMutex;

    /* BUG-42785 OnlineDRDBRedo와 LogFileDeleteThread의 동시성 제어
     * mOnlineDRDBRedoCnt는 현재 OnlineDRDBRedo를 수행하고 있는 
     * 트랜잭션의 수를 나타내며
     * OnlineDRDBRedo가 수행되는 동안은 LogFile이 지워지면 안된다.
     * 따라서 mOnlineDRDBRedoCnt가 0일때만 
     * LogFileDeleteThread가 동작하여야 한다. */
    static SInt               mOnlineDRDBRedoCnt;

    // PROJ-2118 BUG Reporting - Debug Info for Fatal
    static smLSN         mLstRedoLSN;
    static smLSN         mLstUndoLSN;
    static SChar      *  mCurLogPtr;
    static smrLogHead *  mCurLogHeadPtr;


    /*****************************************************************
     * PROJ-2162 RestartRiskReduction Begin
     *
     * EmergencyStartup을 위한 정책 및 변수들입니다.
     *****************************************************************/

    /* 긴급 구동 정책 ( 0 ~ 2 ) */
    static smrEmergencyRecoveryPolicy mEmerRecovPolicy;

    static idBool mDurability;             // Durability가 바른가?
    static idBool mDRDBConsistency;        // DRDB가 Consistent한가?
    static idBool mMRDBConsistency;        // MRDB가 Consistent한가?
    static idBool mLogFileContinuity;      // LogFile이 연속적인가?
    static SChar  mLostLogFile[SM_MAX_FILE_NAME]; // 없는 LogFile이름

    /* EndCheckpoint Log의 위치 */
    static smLSN  mEndChkptLSN;           // MMDB WAL Check에 사용됨

    static smLSN mLstDRDBRedoLSN;         // DRDB WAL Check에 사용됨

    /* Inconsistent한 Object들을 모아두는 곳. MemMge를 이용해 할당함 */
    static smrRTOI  mIOLHead; /* Inconsistent Object List */
    static iduMutex mIOLMutex;/* IOL Attach용 Mutex */
    static UInt     mIOLCount;/* 현재까지 추가된 IOL Node 개수 */
    /*****************************************************************
     * PROJ-2162 RestartRiskReduction End
     *****************************************************************/

    /* PROJ-2133 incremental backup */
    static SChar  mLastRestoredTagName[ SMI_MAX_BACKUP_TAG_NAME_LEN ];
    static UInt   mCurrMediaTime;

    static smLSN    mSkipRedoLSN;
  
    /* PROJ-2742 Support data integrity after fail-back on 1:1 consistent replication */
    static UInt     mLastRemovedFileNo;
};

/***********************************************************************
 * Description : 트랜잭션의 첫번째 로그 여부 반환
 **********************************************************************/
inline idBool smrRecoveryMgr::isBeginLog(smrLogHead * aLogHead)
{

    IDE_DASSERT( aLogHead != NULL );

    if ( ( smrLogHeadI::getFlag(aLogHead) & SMR_LOG_BEGINTRANS_MASK ) ==
         SMR_LOG_BEGINTRANS_OK)
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }

}

/***********************************************************************
 *
 * Description : Restart Recovery 과정에서 Active트랜잭션과 연관된 인덱스
 *               무결성검증 여부를 반환한다.
 *
 **********************************************************************/
idBool smrRecoveryMgr::isVerifyIndexIntegrityLevel2()
{
    if ( ( isRestartRecoveryPhase() == ID_TRUE ) &&
         ( smuProperty::getCheckDiskIndexIntegrity() 
           == SMU_CHECK_DSKINDEX_INTEGRITY_LEVEL2 ) )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/***********************************************************************
 * Description : Idempotent한 성질을 가져야 하는 Log인가?
 **********************************************************************/
inline idBool smrRecoveryMgr::isIdempotentLog( smrLogType sLogType )
{
    idBool sRet = ID_FALSE;
    switch( sLogType )
    {
    case SMR_LT_UPDATE:
    case SMR_LT_NTA:
    case SMR_LT_COMPENSATION:
        sRet = ID_TRUE;
        break;
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
    case SMR_DLT_UNDOABLE:
    case SMR_DLT_NTA:
    case SMR_DLT_COMPENSATION:
    case SMR_DLT_REF_NTA:
    case SMR_LT_TABLE_META:
        sRet = ID_FALSE;
        break;
    default:
        IDE_DASSERT(0);
        break;
    }

    return sRet;
}

/***********************************************************************
 * Description : DRDB Log를 분석하여 DRDB RedoLog의 시작 위치와 길이를
 *               반환한다.
 **********************************************************************/
inline void smrRecoveryMgr::getDRDBRedoBuffer( smrLogType    aLogType,
                                               UInt          aLogTotalSize,
                                               SChar       * aLogPtr,
                                               UInt        * aRedoLogSize,
                                               SChar      ** aRedoBuffer )
{
    switch( aLogType )
    {
    case SMR_DLT_REDOONLY:
    case SMR_DLT_UNDOABLE:
        (*aRedoBuffer) = aLogPtr + SMR_LOGREC_SIZE( smrDiskLog );
        idlOS::memcpy( aRedoLogSize,
                       aLogPtr + offsetof( smrDiskLog, mRedoLogSize ),
                       ID_SIZEOF( UInt ) );
        break;
    case SMR_DLT_NTA:
        (*aRedoBuffer) = aLogPtr + SMR_LOGREC_SIZE( smrDiskNTALog );
        idlOS::memcpy( aRedoLogSize,
                       aLogPtr + offsetof( smrDiskNTALog, mRedoLogSize ),
                       ID_SIZEOF( UInt ) );
        break;
    case SMR_DLT_REF_NTA:
        (*aRedoBuffer) = aLogPtr + SMR_LOGREC_SIZE( smrDiskRefNTALog );
        idlOS::memcpy( aRedoLogSize,
                       aLogPtr + offsetof( smrDiskRefNTALog, mRedoLogSize ),
                       ID_SIZEOF( UInt ) );
        break;
    case SMR_DLT_COMPENSATION:
        (*aRedoBuffer) = aLogPtr + SMR_LOGREC_SIZE( smrDiskCMPSLog );
        idlOS::memcpy( aRedoLogSize,
                       aLogPtr + offsetof( smrDiskCMPSLog, mRedoLogSize ),
                       ID_SIZEOF( UInt ) );
        break;
    case SMR_LT_DSKTRANS_COMMIT:
        idlOS::memcpy( aRedoLogSize,
                       aLogPtr + offsetof( smrTransCommitLog, mDskRedoSize ),
                       ID_SIZEOF( UInt ) );
        (*aRedoBuffer) = aLogPtr + aLogTotalSize
                         - (*aRedoLogSize) - ID_SIZEOF(smrLogTail);
        break;
    case SMR_LT_DSKTRANS_ABORT:
        (*aRedoBuffer) = aLogPtr + SMR_LOGREC_SIZE( smrTransAbortLog );
        idlOS::memcpy( aRedoLogSize,
                       aLogPtr + offsetof( smrTransAbortLog, mDskRedoSize ),
                       ID_SIZEOF( UInt ) );
        break;
    default:
        /* DRDB가 아니거나, 할게 없음 */
        (*aRedoLogSize) = 0;
        break;
    }
}

inline void smrRecoveryMgr::set2PCCallback( smGetDtxMinLSN aGetDtxMinLSN,
                                            smManageDtxInfo aManageDtxInfo )
{
    mGetDtxMinLSNFunc  = aGetDtxMinLSN;
    mManageDtxInfoFunc = aManageDtxInfo;
}

#endif /* _O_SMR_RECOVERY_MGR_H_ */
