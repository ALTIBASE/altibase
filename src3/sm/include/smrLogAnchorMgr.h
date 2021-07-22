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
 * $Id: smrLogAnchorMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 * 로그앵커파일 관리자 ( system control 파일 )
 *
 * # 개념
 *
 * 데이타베이스의 구동시에 필요한 정보를 저장하는
 * Loganchor 관리자
 *
 * # 구조
 *
 * 1. Loganchor 구조
 *
 *   다음과 같이 3부분으로 구성된다.
 *
 *   - Header : smrLogAnchor 구조체로 표현되며,
 *              체크섬과 tablespace 개수등을 포함한다
 *   - Body   : 모든 tablespace의 정보를 저장한다 (가변길이)
 *
 * 2. 다수개의 Loganchor 관리
 *
 *   Loganchor 정보를 갱신할때마다 동시에 여러 개의 Loganchor 파일에
 *   대하여 flush를 수행하여, 시스템 예외상황으로 인해서 하나의 Loganchor가
 *   깨지더라도 다른 Loganchor 복사본을 통하여 시스템 복구가 가능하다
 *
 *   - altibase의 경우 : altibase.properties 파일의 CREATE DATABASE 과정에서
 *                       기본 프로퍼티를 사용하여 처리되며, 서버 구동시에
 *                       유효성 검사를 실시한다.
 *
 * 3. log anchor 저장되는 정보
 *
 *   - 최근 checkpoint 시작 LSN
 *   - 최근 checkpoint 완료
 *   - 마지막 LSN
 *   - Stable Database 파일 번호
 *   - 정상종료시 마지막 생성된 로그파일 번호
 *   - 서버종료상태
 *   - 각 database 파일 개수
 *   - 삭제된 로그파일범위 (시작로그번호 ~ 끝로그번호)
 *   - 로깅 레벨
 *   - tablespace 개수
 *
 * 4. flush 시점에 checksum 기록
 *
 *   - 파일 시작 offset에 4bytes값으로 저장
 *
 *
 * # 관련 자료 구조
 *
 *   - smrLogAnchor 구조체
 **********************************************************************/

#ifndef _O_SMR_LOG_ANCHORMGR_H_
#define _O_SMR_LOG_ANCHORMGR_H_ 1

#include <idu.h>
#include <smu.h>
#include <smm.h>
#include <sdd.h>
#include <smrDef.h>
#include <smriDef.h>
#include <smErrorCode.h>

#include <sdsFile.h>

/**
    로그앵커 Attribute에 대한 초기화 처리 옵션
 */
typedef enum smrAnchorAttrOption
{
    SMR_AAO_REWRITE_ATTR,
    SMR_AAO_LOAD_ATTR
} smrAnchorAttrOption ;


class smrLogAnchorMgr
{
public:

    /* loganchor 초기화 */
    IDE_RC initialize();

    /* process단계에서 loganchor초기화 */
    IDE_RC initialize4ProcessPhase();

    /* loganchor 해제 */
    IDE_RC destroy();

    /* loganchor파일 생성 */
    IDE_RC create();

    /* loganchor파일 백업 */
    IDE_RC backup( UInt   aWhich,
                   SChar* aBackupFilePath );
    // PRJ-1149.
    IDE_RC backup( idvSQL*  aStatistics,
                   SChar *  aBackupFilePath );

    /* loganchor flush */
    IDE_RC flushAll();
    /*repl recovery LSN 갱신 proj-1608*/
    IDE_RC updateReplRecoveryLSN( smLSN aReplRecoveryLSN );

    /* loganchor에 checkpoint 정보 flush */
    IDE_RC updateChkptAndFlush( smLSN   * aBeginChkptLSN,
                                smLSN   * aEndChkptLSN,
                                smLSN   * aDiskRedoLSN,
                                smLSN   * aEndLSN,
                                UInt    * aFirstFileNo,
                                UInt    * aLastFileNo );

    /* 서버상태 정보및 로깅정보를 갱신한다 */
    IDE_RC updateSVRStateAndFlush( smrServerStatus  aSvrStatus,
                                   smLSN           * aEndLSN,
                                   UInt            * aLstCrtLog );

    /* 서버상태 정보만을 갱신한다 */
    IDE_RC updateSVRStateAndFlush( smrServerStatus  aSvrStatus );

    /* 아카이브모드를 갱신한다  */
    IDE_RC updateArchiveAndFlush( smiArchiveMode   aArchiveMode );

    /* 서버가 사용하는 트랜잭션 엔트리 개수를 로그앵커에 저장한다. */
    IDE_RC updateTXSEGEntryCntAndFlush( UInt aEntryCnt );

    /* 디스크/메모리 Redo LSN을 갱신한다. */
    IDE_RC updateRedoLSN( smLSN * aDiskRedoLSN,
                          smLSN * aMemRedoLSN );

    /* incomplete media recovery를 수행하는 경우에
       reset시킬 log lsn flush */
    IDE_RC updateResetLSN(smLSN *aResetLSN);

    /* 모든 Online Tablespace에 대해
       Stable DB를 Switch하고 Log Anchor에 Flush */
    IDE_RC switchAndUpdateStableDB4AllTBS();

    /*  loganchor의 TableSpace 정보를 갱신하는 함수 */
    IDE_RC updateAllTBSAndFlush();

    IDE_RC updateAllSBAndFlush( void );
    
    /*  loganchor의 Memory TableSpace의 다음 Stable No를 갱신하는 함수 */
    IDE_RC updateStableNoOfAllMemTBSAndFlush();

    /*  loganchor의 TBS Node 정보를 갱신하는 함수 */
    IDE_RC updateTBSNodeAndFlush( sctTableSpaceNode  * aSpaceNode );

    /*  loganchor의 DBF Node 정보를 갱신하는 함수 */
    IDE_RC updateDBFNodeAndFlush( sddDataFileNode    * aFileNode );

    /*  loganchor의 Chkpt Inage Node 정보를 갱신하는 함수 */
    IDE_RC updateChkptImageAttrAndFlush( smmCrtDBFileInfo  * aCrtDBFileInfo,
                                         smmChkptImageAttr * aChkptImageAttr );

    /* 변경된 하나의 Checkpint Path Node를 Loganchor에 반영한다. */
    IDE_RC updateChkptPathAttrAndFlush( smmChkptPathNode * aChkptPathNode );
    
    IDE_RC updateSBufferNodeAndFlush( sdsFileNode    * aFileNode );

    // fix BUG-20241
    /*  loganchor의 FstDeleteFileNo 정보를 갱신하는 함수 */
    IDE_RC updateFstDeleteFileAndFlush();

    /*  loganchor의 TBS Node 정보를 추가하는 함수 */
    IDE_RC addTBSNodeAndFlush( sctTableSpaceNode*  aSpaceNode,
                               UInt*               aAnchorOffset );

    /*  loganchor의 DBF Node 정보를 추가하는 함수 */
    IDE_RC addDBFNodeAndFlush( sddTableSpaceNode* aSpaceNode,
                               sddDataFileNode*   aFileNode,
                               UInt*              aAnchorOffset );

    /*  loganchor의 ChkptPath Node 정보를 추가하는 함수 */
    IDE_RC addChkptPathNodeAndFlush( smmChkptPathNode*  aChkptPathNode,
                                     UInt*              aAnchorOffset );

    /*  loganchor의 Chkpt Inage Node 정보를 추가하는 함수 */
    IDE_RC addChkptImageAttrAndFlush( smmChkptImageAttr * aChkptImageAttr,
                                      UInt              * aAnchorOffset );

    IDE_RC addSBufferNodeAndFlush( sdsFileNode  * aFileNode,
                                   UInt         * aAnchorOffset );
    
    /* BUG-39764 : loganchor의 Last Created Logfile Num을 갱신하는 함수 */
    IDE_RC updateLastCreatedLogFileNumAndFlush( UInt aLstCrtFileNo );

    //PROJ-2133 incremental backup
    //logAnchor Buffer에 저장된 DataFileDescSlotID를 반환한다.
    IDE_RC getDataFileDescSlotIDFromChkptImageAttr( 
                            UInt                       aReadOffset,                   
                            smiDataFileDescSlotID    * aDataFileDescSlotID );

    //logAnchor Buffer에 저장된 DataFileDescSlotID를 반환한다.
    IDE_RC getDataFileDescSlotIDFromDBFNodeAttr( 
                            UInt                       aReadOffset,                   
                            smiDataFileDescSlotID    * aDataFileDescSlotID );

    //PROJ-2133 incremental backup
    IDE_RC updateCTFileAttr( SChar          * aFileName,
                             smriCTMgrState * aCTMgrState,
                             smLSN          * aFlushLSN );

    /* BUG-43499  online backup시 restore에 필요한 최소(최초)의 로그를 확인
     * 가능해야 합니다. */
    IDE_RC updateMediaRecoveryLSN( smLSN * aMediaRecoveryLSN );

    inline SChar * getCTFileName()
        { return mLogAnchor->mCTFileAttr.mCTFileName; }

    inline smriCTMgrState getCTMgrState()
        { return mLogAnchor->mCTFileAttr.mCTMgrState; }

    inline smLSN getCTFileLastFlushLSN()
        { return mLogAnchor->mCTFileAttr.mLastFlushLSN; }

    //PROJ-2133 incremental backup
     IDE_RC updateBIFileAttr( SChar          * aFileName,
                              smriBIMgrState * aBIMgrState, 
                              smLSN          * aBackupLSN,
                              SChar          * aBackupDir,
                              UInt           * aDeleteArchLogFileNo );

    inline SChar * getBIFileName()
        { return mLogAnchor->mBIFileAttr.mBIFileName; }

    inline smriBIMgrState  getBIMgrState()
        { return mLogAnchor->mBIFileAttr.mBIMgrState; }

    inline smLSN getBIFileLastBackupLSN()
        { return mLogAnchor->mBIFileAttr.mLastBackupLSN; }

    inline smLSN getBIFileBeforeBackupLSN()
        { return mLogAnchor->mBIFileAttr.mBeforeBackupLSN; }

    inline SChar * getIncrementalBackupDir()
        { return mLogAnchor->mBIFileAttr.mBackupDir; }

    // PRJ-1548 User Memory Tablespace
    // interfaces scan loganchor
    // file must be opened
    static IDE_RC readLogAnchorHeader( iduFile *      aLogAnchorFile,
                                       UInt *         aCurOffset,
                                       smrLogAnchor * aHeader );

    // Loganchor로부터 가변영역의 첫번째 노드 속성 타입 반환
    static IDE_RC getFstNodeAttrType( iduFile *          aLogAnchorFile,
                                      UInt            *  aBeginOffset,
                                      smiNodeAttrType *  aAttrType );

    // Loganchor로부터 다음 노드 속성 타입 반환
    static IDE_RC getNxtNodeAttrType( iduFile *          aLogAnchorFile,
                                      UInt               aNextOffset,
                                      smiNodeAttrType *  aNextAttrType );

    // Loganchor로부터 TBS 노드 속성 반환
    static IDE_RC readTBSNodeAttr( iduFile *           aLogAnchorFile,
                                   UInt *              aCurOffset,
                                   smiTableSpaceAttr * aTBSAttr );

    // Loganchor로부터 DBF 노드 속성 반환
    static IDE_RC readDBFNodeAttr( iduFile *           aLogAnchorFile,
                                    UInt *              aCurOffset,
                                    smiDataFileAttr *   aFileAttr );

    // Loganchor로부터 CPATH 노드 속성 반환
    static IDE_RC readChkptPathNodeAttr(
                           iduFile *          aLogAnchorFile,
                           UInt *             aCurOffset,
                           smiChkptPathAttr * aChkptPathAttr );

    // Loganchor로부터 Checkpoint Image 속성 반환
    static IDE_RC readChkptImageAttr(
                           iduFile *           aLogAnchorFile,
                           UInt *              aCurOffset,
                           smmChkptImageAttr * aChkptImageAttr );

    // PROJ-2102 Loganchor로부터 Secondary Buffer Image 속성 반환
    static IDE_RC readSBufferFileAttr(
                               iduFile               * aLogAnchorFile,
                               UInt                  * aCurOffset,
                               smiSBufferFileAttr    * aFileAttr );

    // checkpoint image attribute size
    static UInt   getChkptImageAttrSize()
        { return ID_SIZEOF( smmChkptImageAttr ); }


    // interfaces for utils to scan loganchor

    /* loganchor에서 checkpoint begin 로그의 LSN 반환 */
    inline smLSN           getBeginChkptLSN()
        {return mLogAnchor->mBeginChkptLSN;}

    /* loganchor에서 checkpoint end 로그의 LSN 반환 */
    inline smLSN           getEndChkptLSN()
        {return mLogAnchor->mEndChkptLSN;}

    /* loganchor의 바이너리 버전 ID 반환 */
    inline UInt            getSmVersionID()
        { return mLogAnchor->mSmVersionID; }

    /* loganchor의 Disk Redo LSN 반환 */
    inline smLSN           getDiskRedoLSN()
        { return mLogAnchor->mDiskRedoLSN; }

    /* loganchor의 reset logs 반환 */
    inline void          getResetLogs(smLSN *aLSN)
        {
            SM_GET_LSN( *aLSN, mLogAnchor->mResetLSN );
        }

    /* loganchor에서 end LSN 반환 */
    inline void          getEndLSN(smLSN* aLSN)
        {
            SM_GET_LSN( *aLSN, mLogAnchor->mMemEndLSN );
        }

    /* loganchor에서 서버상태값 반환 */
    inline smrServerStatus getStatus()
        { return mLogAnchor->mServerStatus; }

    /* loganchor에서 서버상태값 반환 */
    inline UInt getTXSEGEntryCnt()
        { return mLogAnchor->mTXSEGEntryCnt; }

    /* loganchor에서 사용한 마지막 로그번호 반환 */
    inline UInt            getLstLogFileNo()
        { return mLogAnchor->mMemEndLSN.mFileNo; }

    /* loganchor에서 마지막 생성한 로그파일 번호 반환 */
    inline void            getLstCreatedLogFileNo(UInt *aFileNo)
        {
            *aFileNo = mLogAnchor->mLstCreatedLogFileNo;
        }
    /* loganchor에서 파일 개수 반환 */
    inline UInt getLogAnchorFileCount()
        { return (SMR_LOGANCHOR_FILE_COUNT); }

    /* loganchor에서 파일 path 반환 */
    inline SChar* getLogAnchorFilePath(UInt aWhichAnchor);
    /* loganchor에서 해당 database 파일 개수 반환 */
    inline UInt getDBFileCount(UInt aWhichDB);
    /* loganchor에서 지난번에 제거하기 시작한 로그파일번호 반환 */
    inline void getFstDeleteLogFileNo(UInt* aFileNo);
    /* loganchor에서 지난번에 제거하기 끝 로그파일번호 반환 */
    inline void getLstDeleteLogFileNo(UInt* aFileNo);

    /* loganchor에서 지난번에 제거하기 시작한 로그파일번호 반환 */
    inline UInt  getFstDeleteLogFileNo();
    /* loganchor에서 지난번에 제거하기 끝 로그파일번호 반환 */
    inline UInt  getLstDeleteLogFileNo();

    /* loganchor에서 archive 로그 모드 반환 */
    inline smiArchiveMode getArchiveMode() {
                          return mLogAnchor->mArchiveMode; }

    /* proj-1608 recovery from replication repl recovery LSN값 리턴 */
    inline smLSN  getReplRecoveryLSN() { return mLogAnchor->mReplRecoveryLSN; }

    /*  여러 loganchor파일 중에서 유효한 loganchor파일번호 반환 */
    static IDE_RC checkAndGetValidAnchorNo(UInt* aWhich);

    /* Check Log Anchor Dir Exist */
    static IDE_RC checkLogAnchorDirExist();

    smrLogAnchorMgr();
    virtual ~smrLogAnchorMgr();

private:
    // 로그앵커로부터 Tablespace의 특정 타입의 Attribute들을 로드한다.
    IDE_RC readAllTBSAttrs( UInt aWhichAnchor,
                            smiNodeAttrType  aAttrTypeToLoad );

    // 로그앵커 버퍼에 로그앵커 Attribute들중
    // Valid한 Attribute들 만드로 다시 기록한다.
    IDE_RC readLogAnchorToBuffer(UInt aWhichAnchor);

   // 각 Attribute들을 초기화 한다.
    IDE_RC readAttrFromLogAnchor( smrAnchorAttrOption aAttrOp,
                                  smiNodeAttrType     aAttrType,
                                  UInt                aWhichAnchor,
                                  UInt              * aReadOffset );

    // 각 attr의 size를 구하는 함수
    UInt getAttrSize( smiNodeAttrType aAttrType );

    // TBS SpaceNode에서 TBS Attr와 anchorOffset을 반환한다.
    IDE_RC getTBSAttrAndAnchorOffset(
        scSpaceID          aSpaceID,
        smiTableSpaceAttr* aSpaceAttr,
        UInt             * aAnchorOffset );

    // log anchor buffer와 각 node들을 비교 검사한다.
    idBool checkLogAnchorBuffer();

    // log anchor buffer를 기준으로 Space Node를 검사
    idBool checkTBSAttr(  smiTableSpaceAttr* aSpaceAttrByAnchor,
                          UInt               aOffsetByAnchor );

    // log anchor buffer를 기준으로 DBFile Node를 검사
    idBool checkDBFAttr( smiDataFileAttr*   aFileAttrByAnchor,
                         UInt               aOffsetByAnchor );

    // log anchor buffer를 기준으로 Checkpoint Path Node를 검사
    idBool checkChkptPathAttr( smiChkptPathAttr*  aCPPathAttrByAnchor,
                               UInt               aOffsetByAnchor );

    // log anchor buffer를 기준으로 Checkpoint Image Node를 검사
    idBool checkChkptImageAttr( smmChkptImageAttr* aCPImgAttrByAnchor,
                                UInt               aOffsetByAnchor );

    // log anchor buffer를 기준으로 SeondaryBufferFile Node를 검사
    idBool checkSBufferFileAttr( smiSBufferFileAttr* aFileAttrByAnchor,
                                 UInt                aOffsetByAnchor );

    // Tablespace attritube가 동일한지 검사
    idBool cmpTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                              smiTableSpaceAttr* aSpaceAttrByAnchor );

    // Disk Tablespace attritube가 동일한지 검사
    void cmpDiskTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                                smiTableSpaceAttr* aSpaceAttrByAnchor,
                                idBool             sIsEqual,
                                idBool           * sIsValid );

    // Memory Tablespace attritube가 동일한지 검사
    void cmpMemTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                               smiTableSpaceAttr* aSpaceAttrByAnchor,
                               idBool             sIsEqual,
                               idBool           * sIsValid );

    // Volatile Tablespace attritube가 동일한지 검사
    void cmpVolTableSpaceAttr( smiTableSpaceAttr* aSpaceAttrByNode,
                               smiTableSpaceAttr* aSpaceAttrByAnchor,
                               idBool             sIsEqual,
                               idBool           * sIsValid );

    // DBFile attritube가 동일한지 검사
    idBool cmpDataFileAttr( smiDataFileAttr*   aFileAttrByNode,
                            smiDataFileAttr*   aFileAttrByAnchor );

    // Checkpoint Image attritube가 동일한지 검사
    idBool cmpChkptImageAttr( smmChkptImageAttr*   aImageAttrByNode,
                              smmChkptImageAttr*   aImageAttrByAnchor );

    // DBFile attritube가 동일한지 검사
    idBool cmpSBufferFileAttr( smiSBufferFileAttr*   aFileAttrByNode,
                               smiSBufferFileAttr*   aFileAttrByAnchor );


    // 메모리/디스크 테이블스페이스 초기화
    IDE_RC initTableSpaceAttr( UInt                  aWhich,
                               smrAnchorAttrOption   aAttrOp,
                               UInt                * aReadOffset );

    // 디스크 데이타파일 메타헤더 초기화
    IDE_RC initDataFileAttr( UInt                  aWhich,
                             smrAnchorAttrOption   aAttrOp,
                             UInt                * aReadOffset );

    // 메모리 체크포인트 PATH 초기화
    IDE_RC initChkptPathAttr( UInt                  aWhich,
                             smrAnchorAttrOption    aAttrOp,
                              UInt                * aReadOffset );

    // 메모리 데이타파일 메타헤더 초기화
    IDE_RC initChkptImageAttr( UInt                  aWhich,
                               smrAnchorAttrOption   aAttrOp,
                               UInt                * aReadOffset );

    IDE_RC initSBufferFileAttr( UInt                aWhich,
                                smrAnchorAttrOption aAttrOp,
                                UInt              * aReadOffset );

    /* FOR A4 */
    inline IDE_RC lock();
    inline IDE_RC unlock();

    /* loganchor 버퍼 초기화 */
    inline IDE_RC allocBuffer(UInt  aBufferSize);

    /* loganchor 버퍼 offset 초기화 */
    inline void   initBuffer();

    /* loganchor 버퍼 resize  */
    IDE_RC resizeBuffer( UInt aBufferSize );

    /* loganchor 버퍼에 기록 */
    IDE_RC writeToBuffer( void* aBuffer,
                          UInt  aWriteSize );

    /* loganchor 버퍼에 기록 */
    IDE_RC updateToBuffer( void *aBuffer,
                           UInt  aOffset,
                           UInt  aWriteSize );

    /* loganchor 버퍼 해제 */
    IDE_RC freeBuffer();

    /* TBS Attr을 기록할 offset */
    UInt getTBSAttrStartOffset();

    /* checksum 계산 함수 */
    static inline UInt   makeCheckSum(UChar*  aBuffer,
                                      UInt    aBufferSize);

public:

    /* loganchor파일에 대한 I/O 처리 */
    iduFile            mFile[SMR_LOGANCHOR_FILE_COUNT];
    /*  mBuffer에 저장된 loganchor 자료구조에 대한 포인터 */
    smrLogAnchor*      mLogAnchor;

    /* ------------------------------------------------
     * 로그앵커에 저장하기 위한 자료구조
     * ----------------------------------------------*/
    // 메모리/디스크 테이블스페이스 속성
    smiTableSpaceAttr  mTableSpaceAttr;
    // 디스크 데이타파일 속성
    smiDataFileAttr    mDataFileAttr;
    // 메모리 체크포인트 Path 속성
    smiChkptPathAttr   mChkptPathAttr;
    // 메모리 데이타파일 속성
    smmChkptImageAttr  mChkptImageAttr;
    // Secondary Buffer 파일 속성
    smiSBufferFileAttr mSBufferFileAttr;
    /* ------------------------------------------------
     * 가변길이의 loganchor 파일에 write하기 위한 버퍼
     * ----------------------------------------------*/
    UChar*            mBuffer;
    UInt              mBufferSize;
    UInt              mWriteOffset;

    /* ------------------------------------------------
     * A3에서는 경합이 일어나는 상황이 없어서 mutex가
     * 없어도 되지만, A4에서는 tablespace관련 DDL이 생기면서
     * loganchor의 갱신에 동시성을 지원하여야 함
     * ----------------------------------------------*/
    iduMutex           mMutex;

    /* PROJ-2133 incremental backup */
    idBool             mIsProcessPhase;

    /* for property */
    static SChar*      mAnchorDir[SMR_LOGANCHOR_FILE_COUNT];

};

/***********************************************************************
 * Description : loganchor관리자의 mutex 획득
 **********************************************************************/
inline IDE_RC smrLogAnchorMgr::lock()
{

    return mMutex.lock( NULL );

}

/***********************************************************************
 * Description : loganchor관리자의 mutex 해제
 **********************************************************************/
inline IDE_RC smrLogAnchorMgr::unlock()
{

    return mMutex.unlock();

}

/***********************************************************************
 * Description : loganchor에서 지난번에 제거하기 시작한 로그파일번호 반환
 **********************************************************************/
UInt smrLogAnchorMgr::getFstDeleteLogFileNo()
{
    /* BUG-39289 : FST가 LST 보다 크거나 같은 경우는 Lst값으로 리턴한다. 
       FST가 LST + Log Delete Count이기 때문이다. */
    
    if ( mLogAnchor->mLstDeleteFileNo >= mLogAnchor->mFstDeleteFileNo )
    {
        return mLogAnchor->mLstDeleteFileNo;
    }
    else
    {
        return mLogAnchor->mFstDeleteFileNo;
    }

}

/***********************************************************************
 * Description : loganchor에서 지난번에 제거한 끝 로그파일번호 반환
 **********************************************************************/
UInt smrLogAnchorMgr::getLstDeleteLogFileNo()
{
    return mLogAnchor->mLstDeleteFileNo;
}

/***********************************************************************
 * Description : loganchor에서 지난번에 제거하기 시작한 로그파일번호 반환
 **********************************************************************/
void smrLogAnchorMgr::getFstDeleteLogFileNo(UInt* aFileNo)
{
    /* BUG-39289  : FST가 LST 보다 크거나 같은 경우는 LST값으로 리턴한다. 
       FST가 LST + Log Delete Count이기 때문이다. */ 
    if ( mLogAnchor->mFstDeleteFileNo >= mLogAnchor->mLstDeleteFileNo )
    {
        *aFileNo = mLogAnchor->mLstDeleteFileNo;
    }
    else
    {
        *aFileNo = mLogAnchor->mFstDeleteFileNo;
    }

}

/***********************************************************************
 * Description : loganchor에서 지난번에 제거한 끝 로그파일번호 반환
 **********************************************************************/
void smrLogAnchorMgr::getLstDeleteLogFileNo(UInt* aFileNo)
{
    *aFileNo = mLogAnchor->mLstDeleteFileNo;
}


/***********************************************************************
 * Description : loganchor에서 파일 path 반환
 **********************************************************************/
SChar* smrLogAnchorMgr::getLogAnchorFilePath(UInt aWhichAnchor)
{

    IDE_DASSERT( aWhichAnchor < SMR_LOGANCHOR_FILE_COUNT );

    return mFile[aWhichAnchor].getFileName();

}

/***********************************************************************
 * Description : loganchor 버퍼를 할당함
 **********************************************************************/
IDE_RC smrLogAnchorMgr::allocBuffer(UInt   aBufferSize)
{
    IDE_DASSERT( aBufferSize >= idlOS::align(ID_SIZEOF(smrLogAnchorMgr)));

    mBufferSize = aBufferSize;
    mBuffer     = NULL;

    /* TC/FIT/Limit/sm/smr/smrLogAnchorMgr_allocBuffer_malloc.sql */
    IDU_FIT_POINT_RAISE( "smrLogAnchorMgr::allocBuffer::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SMR,
                                      mBufferSize,
                                      (void**)&mBuffer) != IDE_SUCCESS ,
                    insufficient_memory );

    idlOS::memset(mBuffer, 0x00, mBufferSize);

    mLogAnchor   = (smrLogAnchor*)mBuffer;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : loganchor offset을 초기화
 **********************************************************************/
void smrLogAnchorMgr::initBuffer()
{
    mWriteOffset = 0;
}

/***********************************************************************
 * Description : mBuffer에 대한 checksum을 계산하여 반환
 * !!] 32bits or 64bits 하에서도 동일한 값을 생성해야한다.
 **********************************************************************/
UInt smrLogAnchorMgr::makeCheckSum(UChar* aBuffer,
                                   UInt   aBufferSize)
{

    SInt sLength;
    UInt sCheckSum;

    IDE_DASSERT( aBuffer != NULL );

    sCheckSum = 0;
    sLength   = (SInt)(aBufferSize - (UInt)ID_SIZEOF(smrLogAnchor));

    IDE_ASSERT( sLength >= 0 );

    sCheckSum = smuUtility::foldBinary((UChar*)(aBuffer + ID_SIZEOF(UInt)),
                                       aBufferSize - ID_SIZEOF(UInt));

    sCheckSum = sCheckSum & 0xFFFFFFFF;

    return sCheckSum;

}


#endif /* _O_SMR_LOG_ANCHORMGR_H_ */
