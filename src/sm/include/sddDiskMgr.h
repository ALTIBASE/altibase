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
 * $Id: sddDiskMgr.h 91197 2021-07-12 01:15:29Z emlee $
 *
 * Description :
 *
 * 본 파일은 disk 관리자에 대한 헤더 파일이다.
 *
 * # 개념
 *
 * tablespace의 리스트와 오픈된 datafile 등 I/O에 필요한 전체 정보를 관리
 * 이 클래스는 시스템 전역에 하나 존재한다.
 *
 **********************************************************************/

#ifndef _O_SDD_DISK_MGR_H_
#define _O_SDD_DISK_MGR_H_ 1

#include <idu.h>
#include <smu.h>
#include <smDef.h>
#include <sdpDef.h>
#include <sddDef.h>
#include <sctDef.h>
#include <smriDef.h>
#include <idCore.h>
#include <sdpDef.h>
#include <sdptbDef.h>

struct sdptbSpaceCache;
class sddDiskMgr
{

public:

    /* 디스크관리자 초기화  */
    static IDE_RC initialize( UInt aMaxFilePageCnt );
    /* 디스크관리자 해제 */
    static IDE_RC destroy();

    /* Space Cache 설정 */
    static void  setSpaceCache( scSpaceID  aSpaceID,
                                void     * aSpaceCache );
    static void  setSpaceCache( sddTableSpaceNode * aSpaceNode ,
                                void              * aSpaceCache );
    /* Space Cache 반환 */
    static sdptbSpaceCache* getSpaceCache( scSpaceID  aSpaceID );
    static sdptbSpaceCache* getSpaceCache( void*  aSpaceNode )
    {   return (sdptbSpaceCache*)(((sddTableSpaceNode*)aSpaceNode)->mSpaceCache); };

    /* 로그앵커에 의한 tablespace 노드 생성 및 초기화 */
    static IDE_RC loadTableSpaceNode(
                           idvSQL*            aStatistics,
                           smiTableSpaceAttr* aTableSpaceAttr,
                           UInt               aAnchorOffset );

    /* 로그앵커에 의한 datafile 노드생성 */
    static IDE_RC loadDataFileNode( idvSQL*           aStatistics,
                                    smiDataFileAttr*  aDataFileAttr,
                                    UInt              aAnchorOffset );

    /* tablespace 노드 생성 및 초기화, datafile 노드 생성 */
    static IDE_RC createTableSpace(
                        idvSQL             * aStatistics,
                        void               * aTrans,
                        smiTableSpaceAttr  * aTableSpaceAttr,
                        smiDataFileAttr   ** aDataFileAttr,
                        UInt                 aDataFileAttrCount,
                        smiTouchMode         aTouchMode );

    /* PROJ-1923
     * tablespace 노드 생성 및 초기화, redo 과정에서만 사용 됨 */
    static IDE_RC createTableSpace4Redo( void               * aTrans,
                                         smiTableSpaceAttr  * aTableSpaceAttr );

    /* PROJ-1923
     * DBF 노드 생성 및 초기화, redo 과정에서만 사용 됨 */
    static IDE_RC createDataFile4Redo( void               * aTrans,
                                       smLSN                aCurLSN,
                                       scSpaceID            aSpaceID,
                                       smiDataFileAttr    * aDataFileAttr );

    /* tablespace 제거 (노드 제거만 혹은 노드 및 파일 제거) */
    static IDE_RC removeTableSpace( idvSQL*       aStatistics,
                                    void *        aTrans,
                                    scSpaceID     aTableSpaceID,
                                    smiTouchMode  aTouchMode );

    // 모든 디스크 테이블스페이스의 DBFile들의
    // 미디어 복구 필요 여부를 체크한다.
    static IDE_RC identifyDBFilesOfAllTBS( idvSQL * aStatistics,
                                           idBool   aIsOnCheckPoint );

    // Sync타입에 따라서 모든 테이블스페이스를 Sync한다.
    static IDE_RC syncAllTBS( idvSQL    * aStatistics,
                              sddSyncType aSyncType );

    // 테이블스페이스의 Dirty된 데이타파일을 Sync한다. (WRAPPER)
    static IDE_RC syncTBSInNormal( idvSQL    * aStatistics,
                                   scSpaceID   aSpaceID );

    // 체크포인트시 모든 데이타파일에 체크포인트 정보를 갱신한다.
    static IDE_RC doActUpdateAllDBFHdrInChkpt(
                       idvSQL*             aStatistics,
                       sctTableSpaceNode * aSpaceNode,
                       void              * aActionArg );

    // 모든 데이타파일의 메타헤더를 판독하여
    // 미디어 오류여부를 확인한다.
    static IDE_RC doActIdentifyAllDBFiles(
                       idvSQL*              aStatistics,
                       sctTableSpaceNode  * aSpaceNode,
                       void               * aActionArg );

    /* datafile 생성 (노드생성포함) */
    static IDE_RC createDataFiles( idvSQL          * aStatistics,
                                   void*             aTrans,
                                   scSpaceID         aTableSpaceID,
                                   smiDataFileAttr** aDataFileAttr,
                                   UInt              aDataFileAttrCount,
                                   smiTouchMode      aTouchMode);

    static inline IDE_RC validateDataFileName(
                                     sddTableSpaceNode *  aSpaceNode,
                                     smiDataFileAttr   ** aDataFileAttr,
                                     UInt                 aDataFileAttrCount,
                                     SChar             ** sExistFileName,
                                     idBool            *  aNameExist);

    /* datafile 제거 (노드제거포함) 또는 datafile 노드만 제거 */
    static IDE_RC removeDataFileFEBT( idvSQL*             aStatistics,
                                      void*               aTrans,
                                      sddTableSpaceNode * sSpaceNode,
                                      sddDataFileNode   * aFileNode,
                                      smiTouchMode        aTouchMode);
    /*
       PROJ-1548
       DROP DBF에 대한 Pending 연산 함수
    */
    static IDE_RC removeFilePending( idvSQL            * aStatistics,
                                     sctTableSpaceNode * aSpaceNode,
                                     sctPendingOp      * aPendingOp  );

    /* tablespace의 datafile 크기 확장 */
    //PROJ-1671 Bitmap-based Tablespace And Segment Space Management
    static IDE_RC extendDataFileFEBT(
                       idvSQL      *         aStatistics,
                       void        *         aTrans,
                       scSpaceID             aTableSpaceID,
                       sddDataFileNode     * aFileNode );

    /* datafile 노드의 autoextend 속성 변경 */
    static IDE_RC alterAutoExtendFEBT( idvSQL            * aStatistics,
                                       void              * aTrans,
                                       sddTableSpaceNode * aSpaceNode,
                                       SChar             * aDataFileName,
                                       sddDataFileNode   * aFileNode,
                                       idBool              aAutoExtendMode,
                                       ULong               aNextSize,
                                       ULong               aMaxSize );

    /* tablespace의 datafile 크기 resize */
    static IDE_RC alterResizeFEBT( idvSQL          * aStatistics,
                                   void             * aTrans,
                                   scSpaceID          aTableSpaceID,
                                   SChar            * aDataFileName,
                                   scPageID           aHWM,
                                   ULong              aSizeWanted,
                                   sddDataFileNode  * aFileNode);

    /* datafile 노드의 datafile 명 속성 변경 */
    static IDE_RC alterDataFileName( idvSQL    * aStatistics,
                                     scSpaceID   aTableSpaceID,
                                     SChar*      aOldFileName,
                                     SChar*      aNewFileName );

    // 테이블스페이스와 데이타파일의
    // 삭제로 인한 페이지의 유효성 여부를 반환한다.
    static idBool isValidPageID( idvSQL*    aStatistics,
                                 scSpaceID  aTableSpaceID,
                                 scPageID   aPageID );

    static IDE_RC existDataFile( idvSQL *  aStatistics,
                                 scSpaceID aID,
                                 SChar*    aName,
                                 idBool*   aExist);

    static IDE_RC existDataFile( SChar*    aName,
                                 idBool*   aExist);

    /* page 판독 #1 */
    static IDE_RC read( idvSQL      * aStatistics,
                        scSpaceID     aTableSpaceID,
                        scPageID      aPageID,
                        UChar       * aBuffer );
    // PRJ-1149.
    static IDE_RC readPageFromDataFile( idvSQL           * aStatistics,
                                        sddDataFileNode  * aFileNode,
                                        scPageID           aPageID,
                                        ULong              aPageCnt,
                                        UChar            * aBuffer );

    static IDE_RC readvPageFromDataFile( idvSQL           * aStatistics,
                                         sddDataFileNode  * aFileNode,
                                         scPageID           aPageID,
                                         iduFileIOVec     & aVec );
    /* page 판독 #2 */
    static IDE_RC read( idvSQL      * aStatistics,
                        scSpaceID     aTableSpaceID,
                        scPageID      aPageID,
                        ULong         aPageCount,
                        UChar       * aBuffer,
                        idBool        aFatal );
    /* page 판독 #2 */
    static IDE_RC readv( idvSQL      * aStatistics,
                         scSpaceID      aTableSpaceID,
                         scPageID       aFstPageID,
                         iduFileIOVec & aVec );

    /* page 기록 #1 */
    static IDE_RC write( idvSQL*       aStatistics,
                         scSpaceID     aTableSpaceID,
                         scPageID      aPageID,
                         UChar*        aBuffer );

    /* page 기록 #2 */
    static IDE_RC writeMultiPage( idvSQL*       aStatistics,
                                  scSpaceID     aTableSpaceID,
                                  scPageID      aFstPID,
                                  ULong         aPageCount,
                                  UChar*        aBuffer );
    /* page 기록 #2 */
    static IDE_RC writevMultiPage( idvSQL*        aStatistics,
                                   scSpaceID      aTableSpaceID,
                                   scPageID       aFstPID,
                                   iduFileIOVec & aVec );
    static IDE_RC addPageToVec( iduFileIOVec & aVec,
                              UChar        * aPagePtr,
                              UInt           aPageCount )
    {
        return aVec.add( aPagePtr, aPageCount * SD_PAGE_SIZE );
    };
    // PR-15004

    static IDE_RC writePage2DataFile( idvSQL          * aStatistics,
                                      sddDataFileNode * aFileNode,
                                      scPageID          aPageID,
                                      UChar           * aBuffer );
    static IDE_RC writeMultiPage2DataFile( idvSQL          * aStatistics,
                                           sddDataFileNode * aFileNode,
                                           scPageID          aFstPID,
                                           ULong             aPageCnt,
                                           UChar           * aBuffer );
    static IDE_RC writevMultiPage2DataFile( idvSQL          * aStatistics,
                                            sddDataFileNode * aFileNode,
                                            scPageID          aFstPID,
                                            iduFileIOVec    & aVec );

    static IDE_RC syncFile( idvSQL *   aStatistics,
                            scSpaceID  aSpaceID,
                            sdFileID   aFileID );

    /* 해당 tablespace 노드의 총 page 개수 반환 */
    static IDE_RC getTotalPageCountOfTBS(
                     idvSQL*          aStatistics,
                     scSpaceID        aTableSpaceID,
                     ULong*           aTotalPageCount );

    static IDE_RC getExtentAnTotalPageCnt(
                     idvSQL*    aStatistics,
                     scSpaceID  aTableSpaceID,
                     UInt*      aExtentPageCout,
                     ULong*     aTotalPageCount );

    /* tablespace 노드를 출력*/
    static IDE_RC dumpTableSpaceNode( scSpaceID aTableSpaceID );

    /* 디스크관리자의 오픈 datafile LRU 리스트를 출력 */
    static IDE_RC dumpOpenDataFileLRUList();

    static UInt     getMaxDataFileSize()
                    { return (mMaxDataFilePageCount); }

    // 데이타파일의 페이지 구간 반환
    static IDE_RC  getPageRangeInFileByID(idvSQL*            aStatistics,
                                          scSpaceID          aSpaceID,
                                          UInt               aFileID,
                                          scPageID         * aFstPageID,
                                          scPageID         * aLstPageID );

    static IDE_RC prepareFileBackup( idvSQL*            aStatistics,
                                     sddTableSpaceNode* aSpaceNode,
                                     sddDataFileNode*   aDataFileNode );
    static void completeFileBackup( idvSQL*            aStatistics,
                                    sddTableSpaceNode* aSpaceNode,
                                    sddDataFileNode*   aDataFileNode );

    // update tablespace info to loganchor
    static IDE_RC updateTBSInfoForChkpt();

    /* ------------------------------------------------
     * PRJ-1149 미디어복구관련
     * ----------------------------------------------*/
    static IDE_RC checkValidationDBFHdr(
                       idvSQL*           aStatistics,
                       sddDataFileNode*  aFileNode,
                       sddDataFileHdr*   aDBFileHdr,
                       idBool*           aIsMediaFailure );

    static IDE_RC readDBFHdr(
                       idvSQL*           aStatistics,
                       sddDataFileNode*  aFileNode,
                       sddDataFileHdr*   aDBFileHdr );

    static IDE_RC getMustRedoToLSN(
                       idvSQL            * aStatistics,
                       sctTableSpaceNode * aSpaceNode,
                       smLSN             * aMustRedoToLSN,
                       SChar            ** aDBFileName );

    /* 파일헤더정보를 파일에 기록 */
    static IDE_RC writeDBFHdr( idvSQL*          aStatistics,
                               sddDataFileNode* aDataFile );

    /* call by recovery manager */
    static void abortBackupAllTableSpace( idvSQL*  aStatistics );

    /* begin backup */
    static void abortBackupTableSpace( idvSQL*            aStatistics,
                                       sddTableSpaceNode* aSpaceNode );

    static IDE_RC copyDataFile( idvSQL            * aStatistics,
                                sddTableSpaceNode * aSpaceNode,
                                sddDataFileNode   * aDataFileNode,
                                SChar             * aBackupFilePath );

    //PROJ-2133 incremental backup
    static IDE_RC incrementalBackup(idvSQL                 * aStatistics,
                                    sddTableSpaceNode      * aSpaceNode,
                                    smriCTDataFileDescSlot * aDataFileDescSlot,
                                    sddDataFileNode        * aDataFileNode,
                                    smriBISlot             * sBackupInfo );

    // PRJ-1548 User Memory Tablespace
    // 서버구동시 복구이후에 모든 테이블스페이스의
    // DataFileCount와 TotalPageCount를 계산하여 설정한다.
    static void calculateFileSizeOfAllTBS();

    // PRJ-1548 User Memory Tablespace
    // 디스크 테이블스페이스를 백업한다.
    static IDE_RC backupAllDiskTBS( idvSQL  * aStatistics,
                                    SChar   * aBackupDir );

    //PROJ-2133 incremental backup
    //디스크 테이블스페이스를 incremental 백업한다.
    static IDE_RC incrementalBackupAllDiskTBS( idvSQL     * aStatistics,
                                               smriBISlot * aCommonBackupInfo,
                                               SChar      * aBackupDir );

    // 테이블스페이스의 DBF 노드들의 백업완료작업을 수행한다.
    static IDE_RC endBackupDiskTBS( idvSQL            * aStatistics,
                                    sddTableSpaceNode * aSpaceNode );

    // 테이블스페이스의 미디어오류가 있는 데이타파일 목록을 만든다.
    static IDE_RC makeMediaRecoveryDBFList(
                           idvSQL            * aStatistics,
                           sctTableSpaceNode * sSpaceNode,
                           smiRecoverType      aRecoveryType,
                           UInt              * aDiskDBFCount,
                           smLSN             * aFromRedoLSN,
                           smLSN             * aToRedoLSN );

    /* TableSapce의 모든 DataFile의 Max Open Count를 변경한다. */
    static IDE_RC  setMaxFDCnt4AllDFileOfTBS( sctTableSpaceNode* aSpaceNode,
                                              UInt               aMaxFDCnt4File );

    // Restart 단계에서 Offline TBS에 대한 Runtime 객체들을 Free
    static IDE_RC finiOfflineTBSAction(
                      idvSQL*             aStatistics,
                      sctTableSpaceNode * aSpaceNode,
                      void              * /* aActionArg */ );

    // TBS의 한 Extent의 페이지 갯수를 리턴한다.
    static inline UInt getPageCntInExt( scSpaceID aSpaceID );

    /* datafile 열기 및 LRU 리스트에 datafile 노드 추가 */
    static IDE_RC openDataFile( idvSQL          * aStatistics,
                                sddDataFileNode * aDataFileNode );

    /* datafile 닫기 및 LRU 리스트로부터 datafile 노드 제거 */
    static IDE_RC closeDataFile( idvSQL          * aStatistics,
                                 sddDataFileNode * aDataFileNode );

    static IDE_RC closeDataFileInternal( idvSQL          * aStatistics,
                                         sddDataFileNode * aDataFileNode  );

    static inline void removeNodeList( idvSQL          * aStatistics,
                                       sddDataFileNode * aDataFileNode );
    static inline void addNodeList(  idvSQL          * aStatistics,
                                     sddDataFileNode * aDataFileNode );
    static IDE_RC checkAndOpenDataFile( sddDataFileNode*  aDataFileNode );

public: // for unit test-driver

    static IDE_RC openDataFile( idvSQL   * aStatistics,
                                scSpaceID  aTableSpaceID,
                                scPageID   aPageID );

    static IDE_RC closeDataFile( scSpaceID  aTableSpaceID,
                                 scPageID   aPageID );

    static IDE_RC completeIO( idvSQL*   aStatistics,
                              scSpaceID aTableSpaceID,
                              scPageID  aPageID );

    /* datafile에 대한 I/O를 하기 전에 datafile 노드를 오픈 */
    static IDE_RC prepareIO( idvSQL          * aStatistics,
                             sddDataFileNode * aDataFileNode );

    /* datafile에 대한 I/O 완료후 datafile 노드 설정 */
    static IDE_RC completeIO( idvSQL          * aStatistics,
                              sddDataFileNode * aDataFileNode,
                              sddIOMode         aIOMode );

    // Offline DBF에 대한 Write 연산 가능여부 설정
    static void   setEnableWriteToOfflineDBF( idBool aOnOff )
                  { mEnableWriteToOfflineDBF = aOnOff; }

     //PROJ-1671 Bitmap-based Tablespace And Segment Space Management
    static void getExtendableSmallestFileNode( sddTableSpaceNode *sSpaceNode,
                                               sddDataFileNode  **sFileNode );

    static IDE_RC tracePageInFile( UInt            aChkFlag,
                                   ideLogModule    aModule,
                                   UInt            aLevel,
                                   scSpaceID       aSpaceID,
                                   scPageID        aPageID,
                                   const SChar   * aTitle );

    /* PROJ-1923
     * private -> public 전환 */
    static IDE_RC shrinkFilePending( idvSQL            * aStatistics,
                                     sctTableSpaceNode * aSpaceNode,
                                     sctPendingOp      * aPendingOp  );

    static void lockCloseVictimList( idvSQL * aStatistics ) { (void)mMutexVFL.lock( aStatistics ); };
    static void unlockCloseVictimList() { (void)mMutexVFL.unlock(); };

    static void lockIncOpenFileCount( idvSQL * aStatistics ) { (void)mMutexOPC.lock( aStatistics ); };
    static void unlockIncOpenFileCount() { (void)mMutexOPC.unlock(); };

    static void lockGlobalPageCountCheckMutex( idvSQL * aStatistics )
    { (void)mGlobalPageCountCheckMutex.lock( aStatistics );   }
    static void unlockGlobalPageCountCheckMutex( )
    { (void)mGlobalPageCountCheckMutex.unlock(); }

    static idBool isDirectIO(){ return mIsDirectIO; };

    static SChar* getFileName( scSpaceID aSpaceID,
                               scPageID  aPageID );

private:

    // BUG-17158
    // Offline DBF에 대한 Write 연산 가능여부 반환
    static idBool isEnableWriteToOfflineDBF()
                  { return mEnableWriteToOfflineDBF; }


    /* 오픈된 datafile에서 사용하지 않는 datafile을 검색 */
    static IDE_RC closeDataFileInList( idvSQL          * aStatistics,
                                       sddDataFileNode * aDataFileNode  );

public:

    static UInt   mInitialCheckSum;  // 최초 page write시에 저장되는 checksum

private:

    // Close를 위한 Victim FileNode List Mutex
    static iduMutex    mMutexVFL;

    // OpenFileCount 증가시 Max값을 넘지 않도록 보호하는 Mutex
    static iduMutex    mMutexOPC;

    // Drop DBF와 AllocPageCount 연산간의 동시성 제어
    static iduMutex    mGlobalPageCountCheckMutex;

    // 오픈된 datafile 노드의 LRU 리스트
    static smuList     mVictimFileList;
    // 오픈된 datafile 노드 개수
    static UInt        mOpenFileCount;
    static UInt        mVictimListNodeCount;

    /* ------------------------------------------------
     * 모든 datafile의 크기는 maxsize(페이지개수)초과할 수 없다.
     * 이 값은 설정파일에 정의되어 있으며,
     * 디스크관리자의 초기화시에 할당된다.
     * ----------------------------------------------*/

    // datafile이 가질수 있는 max page 개수
    static UInt        mMaxDataFilePageCount;

    /* ------------------------------------------------
     * 현재 오픈 할 수 있는 화일이 생기길 기다리는 스레드가 있다는 표시
     * ----------------------------------------------*/
    static UInt              mWaitThr4Open;

    // BUG-17158
    // Offline DBF에 대해 Write를 해야하는 경우
    static idBool            mEnableWriteToOfflineDBF;

    static idBool            mIsDirectIO;
};

inline UInt sddDiskMgr::getPageCntInExt( scSpaceID aSpaceID )
{
    sdpSpaceCacheCommon *sTbsCache =
        (sdpSpaceCacheCommon*)getSpaceCache( aSpaceID );

    return sTbsCache->mPagesPerExt;
}

/* -----------------------------------------------------------------------
 * IOCount가 0인 FileNode를 Close Victim Filt List 에 연결하거나 제거한다.
 * IOCount를 확인하는 것과 List에 추가하는 것이 같은 lock이 아니므로
 * IOCount가 0 이상인 Node도 있을 수 있다.
 * 다만 IOCount가 0이면 반드시 List에 연결 되어 있어야 한다.
 * ----------------------------------------------------------------------*/
inline void sddDiskMgr::addNodeList( idvSQL          * aStatistics,
                                     sddDataFileNode * aDataFileNode )
{
    lockCloseVictimList( aStatistics );
    if ( aDataFileNode->mNode4LRUList.mNext == NULL )
    {
        SMU_LIST_ADD_FIRST( &mVictimFileList,
                            &aDataFileNode->mNode4LRUList );
        mVictimListNodeCount++;
    }
    unlockCloseVictimList();
}

inline void sddDiskMgr::removeNodeList( idvSQL          * aStatistics,
                                        sddDataFileNode * aDataFileNode )
{
    lockCloseVictimList( aStatistics );
    if ( aDataFileNode->mNode4LRUList.mNext != NULL )
    {
        SMU_LIST_DELETE( &aDataFileNode->mNode4LRUList );
        aDataFileNode->mNode4LRUList.mNext = NULL;
        aDataFileNode->mNode4LRUList.mPrev = NULL;
        mVictimListNodeCount--;
    }
    unlockCloseVictimList();
}

#endif // _O_SDD_DISK_MGR_H_
