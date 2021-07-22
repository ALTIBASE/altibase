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
 * $Id: smmManager.h 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

#ifndef _O_SMM_MANAGER_H_
#define _O_SMM_MANAGER_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <smmDef.h>
#include <smmDatabaseFile.h>
#include <smmFixedMemoryMgr.h>
#include <sctTableSpaceMgr.h>
#include <smu.h>
#include <smmPLoadMgr.h>
/*
    참고 SMM안에서의 File간의 Layer및 역할은 다음과 같다.
    하위 Layer의 코드에서는 상위 Layer의 코드를 사용할 수 없다.

    ----------------
    smmTableSpace    ; Tablespace로의 Operation구현 (create/alter/drop)
    ----------------
    smmTBSMultiPhase ; Tablespace의 다단계 초기화
    ----------------
    smmManager       ; Tablespace의 내부 구현
    smmFPLManager    ; Tablespace Free Page List의 내부 구현
    smmExpandChunk   ; Chunk의 내부구조 구현
    ----------------
 */

class  smmFixedMemoryMgr;

// smmManager를 초기화 하는 모드
typedef enum
{
    // 일반 STARTUP - 디스크상에 데이터베이스 파일 존재
    SMM_MGR_INIT_MODE_NORMAL_STARTUP = 1,
    // CREATEDB도중 - 아직 디스크상에 데이터베이스 파일이 존재하지 않음
    SMM_MGR_INIT_MODE_CREATEDB = 2
} smmMgrInitMode ;

// fillPCHEntry 함수에서 페이지 메모리의 복사여부를 결정
typedef enum smmFillPCHOption {
    SMM_FILL_PCH_OP_NONE = 0 ,
    /** 페이지 메모리의 내용을 PCH 안의 Page에 복사한다 */
    SMM_FILL_PCH_OP_COPY_PAGE = 1,
    /** 페이지 메모리의 포인터를 PCH 안의 Page에 세팅한다 */
    SMM_FILL_PCH_OP_SET_PAGE = 2
} smmFillPCHOption ;


// allocNewExpandChunk 함수의 옵션
typedef enum smmAllocChunkOption
{
    SMM_ALLOC_CHUNK_OP_NONE = 0,              // 옵션없음
    SMM_ALLOC_CHUNK_OP_CREATEDB         = 1   // createdb 도중임을 알림
} smmAllocChunkOption ;



// getDBFile 함수의 옵션
typedef enum smmGetDBFileOption
{
    SMM_GETDBFILEOP_NONE = 0,        // 옵션 없음
    SMM_GETDBFILEOP_SEARCH_FILE = 1  // 모든 MEM_DB_DIR에서 DB파일을 찾아낸다
} smmGetDBFileOption ;

/*********************************************************************
  메모리 데이터베이스의 페이지 사용
 *********************************************************************
  0번째 Page부터 SMM_DATABASE_META_PAGE_CNT개만큼의 Page에는
  데이터베이스를 유지하기 위한 메타정보가 기록된다.

  그리고 그 다음 Page부터 Expand Chunk가 오기 시작한다.

  SMM_DATABASE_META_PAGE_CNT 가 1이고, Expand Chunk당 Page수가 10인 경우의
  Page 사용 예를 도식하면 다음과 같다.
  --------------------------------------------------
   Page#0 : Base Page
            Membase및 Catalog Table정보 저장
  --------------------------------------------------
   Page#1  ~ Page#10  Expand Chunk #0
  --------------------------------------------------
   Page#11 ~ Page#20  Expand Chunk #1
  --------------------------------------------------
 *********************************************************************/

class smmManager
{
private:

    // 각 데이터베이스 Page의 non-durable한 데이터를 지니는 PCH 배열
    /* BUG-48513 Volatile과 Memory 를 같은 array 에 담습니다.*/
    static smPCArr                 mPCArray[SC_MAX_SPACE_ARRAY_SIZE];

    static smmGetPersPagePtrFunc   mGetPersPagePtrFunc;
private :
    // Check Database Dir Exist
    static IDE_RC checkDatabaseDirExist(smmTBSNode * aTBSNode);

    // 데이터베이스 확장에 따라 새로운 Expand Chunk가 할당됨에 따라
    // Membase에 이와 관련 정보를 변경한다.
    static IDE_RC updateMembase4AllocChunk( void     * aTrans,
                                            scPageID   aChunkFirstPID,
                                            scPageID   aChunkLastPID );

    // 공유메모리 풀을 초기화한다.
    static IDE_RC initializeShmMemPool( smmTBSNode *  aTBSNode );


    // 일반 메모리 Page Pool을 초기화한다.
    static IDE_RC initializeDynMemPool(smmTBSNode * aTBSNode);

    // Tablespace에 할당된 Page Pool을 파괴한다.
    static IDE_RC destroyPagePool( smmTBSNode * aTBSNode );


    // 새 데이터베이스 크기가 특정 크기 이상으로
    // 확장되기 전에 Trap을 보낸다.
    static IDE_RC sendDatabaseSizeTrap( UInt aNewPageSize );


    // 데이터베이스 타입에 따라
    // 공유메모리나 일반 메모리를 페이지 메모리로 할당한다
    // BUG-47487: DATA / FLI 페이지 여부 추가
    static IDE_RC allocDynOrShm( smmTBSNode   * aTBSNode,
                                 void        ** aPageHandle,
                                 smmTempPage ** aPage,
                                 idBool         aIsDataPage = ID_TRUE );

    // 데이터베이스타입에따라
    // 하나의 Page메모리를 공유메모리나 일반메모리로 해제한다.
    // BUG-47487: DATA / FLI 페이지 여부 추가
    static IDE_RC freeDynOrShm( smmTBSNode  * aTBSNode,
                                void        * aPageHandle,
                                smmTempPage * aPage,
                                idBool        aIsDataPage = ID_TRUE );


    // 새로 할당된 Expand Chunk안에 속하는 Page들의 PCH Entry를 할당한다.
    // Chunk안의 Free List Info Page의 Page Memory도 할당한다.
    static IDE_RC fillPCHEntry4AllocChunk(smmTBSNode * aTBSNode,
                                          scPageID     aNewChunkFirstPID,
                                          scPageID     aNewChunkLastPID );


    // Tablespace의 Meta Page ( 0번 Page)를 초기화한다.
    static IDE_RC createTBSMetaPage( smmTBSNode   * aTBSNode,
                                     void         * aTrans,
                                     SChar        * aDBName,
                                     scPageID       aDBFilePageCount,
                                     SChar        * aDBCharSet,
                                     SChar        * aNationalCharSet,
                                     idBool         aIsNeedLogging = ID_TRUE );

public:
    // 일반 테이블을 저장할 카탈로그 테이블의 헤더
    static void               *m_catTableHeader;
    // Temp 테이블을 저장할 카탈로그 테이블의 헤더
    static void               *m_catTempTableHeader;

    /*****************************************************************
       Tablespace의 단계별 초기화 루틴
     */

    // Memory Tablespace Node의 필드를 초기화한다.
    static IDE_RC initMemTBSNode( smmTBSNode * aTBSNode,
                                  smiTableSpaceAttr * aTBSAttr );

    // Tablespace를 위한 매체(DB File) 관리 시스템 초기화
    static IDE_RC initMediaSystem( smmTBSNode * aTBSNode );


    // Tablespace를 위한 Page관리 시스템 초기화
    static IDE_RC initPageSystem( smmTBSNode  * aTBSNode );
    static IDE_RC allocPCHArray( scSpaceID aSpaceID,
                                 UInt      aMaxPageCount );
    static IDE_RC freePCHArray( scSpaceID aSpaceID );
    /*****************************************************************
       Tablespace의 단계별 해제 루틴
     */
    // Tablespace Node를 파괴한다.
    static IDE_RC finiMemTBSNode(smmTBSNode * aTBSNode);


    // Tablespace를 위한 매체(DB File) 관리 시스템 해제
    static IDE_RC finiMediaSystem(smmTBSNode * aTBSNode);

    // Tablespace를 위한 Page관리 시스템 해제
    static IDE_RC finiPageSystem(smmTBSNode * aTBSNode);

    /*****************************************************************
     */
    // DB파일 크기가 OS의 파일크기 제한에 걸리지는 않는지 체크한다.
    static IDE_RC checkOSFileSize( vULong aDBFileSize );

    // DB File 객체들과 관련 데이터 구조들을 초기화한다.
    static IDE_RC initDBFileObjects(smmTBSNode *       aTBSNode,
                                    UInt               aDBFilePageCount);

    // DB File 객체를 생성한다.
    static IDE_RC createDBFileObject( smmTBSNode       * aTBSNode,
                                      UInt               aPingPongNum,
                                      UInt               aFileNum,
                                      scPageID           aFstPageID,
                                      scPageID           aLstPageID,
                                      smmDatabaseFile  * aDBFileObj );

    // DB File객체와 관련 자료구조를 해제한다.
    static IDE_RC finiDBFileObjects( smmTBSNode * aTBSNode );


    // Tablespace의 모든 Checkpoint Image File들을
    // Close하고 (선택적으로) 지워준다.
    static IDE_RC closeAndRemoveChkptImages(smmTBSNode * aTBSNode,
                                            idBool       aRemoveImageFiles );


    static IDE_RC removeAllChkptImages(smmTBSNode *   aTBSNode);

    // Membase가 들어있는 Meta Page(0번)을 Flush한다.
    static IDE_RC flushTBSMetaPage(smmTBSNode *   aTBSNode,
                            UInt           aWhichDB);

    // 0번 Page를 디스크로부터 읽어서
    // MemBase로부터 데이터베이스관련 정보를 읽어온다.
    static IDE_RC readMemBaseInfo(smmTBSNode * aTBSNode,
                                  UInt       * aDbFilePageCount,
                                  UInt       * aChunkPageCount );

    // 0번 Page를 디스크로부터 읽어서 MemBase를 복사한다.
    static IDE_RC readMemBaseFromFile(smmTBSNode *   aTBSNode,
                                      smmMemBase *   aMemBase );

    // PROJ 2281 storing bufferpool stat persistantly
    static IDE_RC setSystemStatToMemBase( smiSystemStat      * aSystemStat );
    static IDE_RC getSystemStatFromMemBase( smiSystemStat     * aSystemStat );

    // 데이터베이스 크기를 이용하여 데이터베이스 Page수를 구한다.
    static ULong calculateDbPageCount( ULong aDbSize, ULong aChunkPageCount );

    // 하나의 Page의 데이터가 저장되는 메모리 공간을 PCH Entry에서 가져온다.
    static inline IDE_RC getPersPagePtr( scSpaceID    aSpaceID, 
                                         scPageID     aPID, 
                                         void      ** aPagePtr );
    static IDE_RC getPersPagePtrErr( scSpaceID    aSpaceID, 
                                     scPageID     aPID, 
                                     void      ** aPagePtr );
    static UInt   getPageNoInFile(smmTBSNode * aTBSNode, scPageID aPageID );
    // Page ID가 Valid한 값인지 체크한다.
    static idBool isValidPageID( scSpaceID aSpaceID, scPageID aPageID );

    // 테이블에 할당된 페이지이면서 메모리가 없는 경우 있는지 체크
    // Free Page이면서 페이지 메모리 설정된 경우 있는지 체크
    static idBool isAllPageMemoryValid(smmTBSNode * aTBSNode);

    /***********************************************************
     * smmTableSpace 에서 호출하는 함수들
     ***********************************************************/
    // Tablespace에서 사용할 Page 메모리 풀을 초기화한다.
    static IDE_RC initializePagePool( smmTBSNode * aTBSNode );

    //CreateDB시 데이터베이스의 파일을 생성한다.
    static IDE_RC createDBFile( void          * aTrans,
                                smmTBSNode    * aTBSNode,
                                SChar         * aTBSName,
                                vULong          aDBPageCnt,
                                idBool          aIsNeedLogging );

    // PROJ-1923 ALTIBASE HDB Disaster Recovery
    // Tablespace의 Meta Page를 초기화하고 Free Page들을 생성한다.
    static IDE_RC createTBSPages4Redo( smmTBSNode * aTBSNode,
                                       void       * aTrans,
                                       SChar      * aDBName,
                                       UInt         aDBFilePageCount,
                                       UInt         aCreatePageCount,
                                       SChar      * aDBCharSet,
                                       SChar      * aNationalCharSet );

    // Tablespace의 Meta Page를 초기화하고 Free Page들을 생성한다.
    static IDE_RC createTBSPages( smmTBSNode      * aTBSNode,
                                  void            * aTrans,
                                  SChar           * aDBName,
                                  UInt              aDBFilePageCount,
                                  UInt              aCreatePageCount,
                                  SChar           * aDBCharSet,
                                  SChar           * aNationalCharSet );

    // 페이지 메모리를 할당하고, 해당 Page를 초기화한다.
    // 필요한 경우, 페이지 초기화에 대한 로깅을 실시한다
    static IDE_RC allocAndLinkPageMemory( smmTBSNode * aTBSNode,
                                          void     *   aTrans,
                                          scPageID     aPID,
                                          scPageID     aPrevPID,
                                          scPageID     aNextPID );

    // Dirty Page의 PCH에 기록된 Dirty Flag를 모두 초기화한다.
    static IDE_RC clearDirtyFlag4AllPages(smmTBSNode * aTBSNode );

    /////////////////////////////////////////////////////////////////////
    // fix BUG-17343 loganchor에 Stable/Unstable Chkpt Image에 대한 생성
    //               정보를 저장

    static void initCrtDBFileInfo( smmTBSNode * aTBSNode );

    // 주어진 파일번호에 해당하는 DBF가 하나라도 생성이 되었는지 여부반환
    static idBool isCreateDBFileAtLeastOne( idBool     * aCreateDBFileOnDisk );

    // create dbfile 플래그를 설정한다.
    static void   setCreateDBFileOnDisk( smmTBSNode    * aTBSNode,
                                         UInt            aPingPongNum,
                                         UInt            aFileNum,
                                         idBool          aFlag )
    { (aTBSNode->mCrtDBFileInfo[aFileNum]).mCreateDBFileOnDisk[aPingPongNum]
                 = aFlag; }

    // create dbfile 플래그를 반환한다.
    static idBool getCreateDBFileOnDisk( smmTBSNode    * aTBSNode,
                                         UInt            aPingPongNum,
                                         UInt            aFileNum )
   { return
     (aTBSNode->mCrtDBFileInfo[aFileNum]).mCreateDBFileOnDisk[aPingPongNum ]; }

    // ChkptImg Attribute 의 Loganchor 오프셋을 설정한다.
    static void setAnchorOffsetToCrtDBFileInfo(
                            smmTBSNode * aTBSNode,
                            UInt         aFileNum,
                            UInt         aAnchorOffset )
    { (aTBSNode->mCrtDBFileInfo[aFileNum]).mAnchorOffset = aAnchorOffset; }

private:

    // 특정 Page의 PCH안의 Page Memory를 할당한다.
    static IDE_RC allocPageMemory( smmTBSNode * aTBSNode,
                                   scPageID     aPID );

    // 특정 Page의 PCH안의 Page Memory를 해제한다.
    static IDE_RC freePageMemory( smmTBSNode * aTBSNode, scPageID aPID );


    // FLI Page에 Next Free Page ID로 링크된 페이지들에 대해
    // PCH안의 Page 메모리를 할당하고 Page Header의 Prev/Next포인터를 연결한다.
    static IDE_RC allocFreePageMemoryList( smmTBSNode * aTBSNode,
                                           void       * aTrans,
                                           scPageID     aHeadPID,
                                           scPageID     aTailPID,
                                           vULong     * aPageCount );

    // PCH의 Page속의 Page Header의 Prev/Next포인터를 기반으로
    // PCH 의 Page 메모리를 반납한다.
    static IDE_RC freeFreePageMemoryList( smmTBSNode * aTBSNode,
                                          void       * aHeadPage,
                                          void       * aTailPage,
                                          vULong     * aPageCount );

    // PCH의 Page속의 Page Header의 Prev/Next포인터를 기반으로
    // FLI Page에 Next Free Page ID를 설정한다.
    static IDE_RC linkFreePageList( smmTBSNode * aTBSNode,
                                    void       * aTrans,
                                    void       * aHeadPage,
                                    void       * aTailPage,
                                    vULong     * aPageCount );


    static IDE_RC reverseMap(); // bugbug : implement this.

    /*
     * Page의 PCH(Page Control Header)정보를 구성한다.
     */
    // PCH 할당및 초기화
    static IDE_RC allocPCHEntry(smmTBSNode *  aTBSNode,
                                scPageID      a_pid);

    // PCH 해제
    static IDE_RC freePCHEntry(smmTBSNode *  aTBSNode,
                                scPageID     a_pid,
                               idBool       aPageFree = ID_TRUE);

    // Page의 PCH(Page Control Header)정보를 구성한다.
    // 필요에 따라서, Page Memory의 내용을 구성하기도 한다.
    static IDE_RC fillPCHEntry(smmTBSNode *        aTBSNode,
                               scPageID            aPID,
                               smmFillPCHOption    aFillOption
                                                   = SMM_FILL_PCH_OP_NONE,
                               void              * aPage = NULL);


    // Tablespace안의 모든 Page Memory/PCH Entry를 해제한다.
    static IDE_RC freeAllPageMemAndPCH(smmTBSNode * aTBSNode );

    // 모든 PCH 메모리를 해제한다.
    // aPageFree == ID_TRUE이면 페이지 메모리도 해제한다.
    static IDE_RC freeAll(smmTBSNode * aTBSNode, idBool aPageFree);

    // 데이터 베이스의 Base Page (Page#0)를 가리키는 포인터들을 세팅한다.
    static IDE_RC setupCatalogPointers( smmTBSNode * aTBSNode,
                                        UChar *      aBasePage );
    static IDE_RC setupMemBasePointer( smmTBSNode * aTBSNode,
                                       UChar *      aBasePage );

    // 데이터 베이스의 메타정보를 설정한다.
    // 현재 알티베이스의 메타정보로는 MemBase와 Catalog Table정보가 있다.
    static IDE_RC setupDatabaseMetaInfo(
                      UChar *a_base_page,
                      idBool a_check_version = ID_TRUE);

    // 일반 메모리로 데이타파일 이미지의 내용을 읽어들인다.
    static IDE_RC restoreDynamicDBFile( smmTBSNode      * aTBSNode,
                                        smmDatabaseFile * aDatabaseFile );

    // 일반 메모리로 데이터베이스 이미지의 내용을 읽어들인다.
    static IDE_RC restoreDynamicDB(smmTBSNode *     aTBSNode);

    // 공유 메모리로 데이터베이스 이미지의 내용을 읽어들인다.
    static IDE_RC restoreCreateSharedDB( smmTBSNode * aTBSNode,
                                         smmRestoreOption aOp );

    // 시스템의 공유메모리에 존재하는 데이터베이스 페이지들을 ATTACH한다.
    static IDE_RC restoreAttachSharedDB(smmTBSNode *        aTBSNode,
                                        smmShmHeader      * aShmHeader,
                                        smmPrepareOption    aOp );

    // DB File Loading Message출력
    static void printLoadingMessage( smmDBRestoreType aRestoreType );

public:

    /* --------------------
     * [1] Class Control
     * -------------------*/
    smmManager();
    // smmManager를 초기화한다.
    static IDE_RC initializeStatic(  );
    static IDE_RC destroyStatic();

    // Base Page ( 0번 Page ) 에 Latch를 건다
    static IDE_RC lockBasePage(smmTBSNode * aTBSNode);
    // Base Page ( 0번 Page ) 에서 Latch를 푼다.
    static IDE_RC unlockBasePage(smmTBSNode * aTBSNode);

    static IDE_RC initSCN();

    static IDE_RC getDBFileCountFromDisk(UInt  a_stableDB,
                                         UInt *a_cDBFile);

    /* -----------------------
     * [2] Create & Restore DB
     * ----------------------*/

    // Tablespace Node를 초기화한다.
    static IDE_RC initializeTBSNode( smmTBSNode        * aTBSNode );

    // 미디어복구시의 복구가 필요한 테이블스페이스의 여부를 반환
    static idBool isMediaFailureTBS( smmTBSNode * aTBSNode );

    // 데이터베이스 restore(Disk -> 메모리로 로드 )를 위해 준비한다.
    static IDE_RC prepareDB (smmPrepareOption  aOp);

    static IDE_RC prepareTBS (smmTBSNode *      aTBSNode,
                              smmPrepareOption  aOp );

    // prepareDB를 위한 Action함수
    static IDE_RC prepareTBSAction( idvSQL            * aStatistics,
                                    sctTableSpaceNode * aTBSNode,
                                    void              * aActionArg );

    // Alter TBS Online을 위해 Tablespace를 Prepare / Restore 한다.
    static IDE_RC prepareAndRestore( smmTBSNode * aTBSNode );

    // 디스크 이미지로부터 데이터베이스 페이지를 로드한다.
    static IDE_RC restoreDB ( smmRestoreOption aOp = SMM_RESTORE_OP_NONE );

    // loganchor의 checkpoint image attribute수를 토대로 계산된
    //  dbfile 갯수 반환 => restore db시에 사용됨
    static UInt   getRestoreDBFileCount( smmTBSNode      * aTBSNode);

    static IDE_RC restoreTBS ( smmTBSNode * aTBSNode, smmRestoreOption aOp );

    // restoreDB를 위한 Action함수
    static IDE_RC restoreTBSAction( idvSQL            * aStatistics,
                                    sctTableSpaceNode * aTBSNode,
                                    void              * aActionArg );

    // 첫번째 데이타파일을 오픈하고 Membase를 설정한다.
    static IDE_RC openFstDBFilesAndSetupMembase( smmTBSNode * aTBSNode,
                                                 smmRestoreOption aOp,
                                                 UChar      * aReadBuffer );

    /* ------------------------------------------------
     *              *** DB Loading 방법 선택 ***
     *
     *  1. loadParallel ()
     *     ==> Thread를 이용해 각 Table 단위로 동시에 로딩
     *         system call이 Page당 1회씩 불리는 단점과
     *         HP, AIX, DEC의 경우에는 쓰레드가 1개로 로딩
     *         pread가 thread safe하지 않기 때문임.
     *
     *  2. loadSerial ()
     *     ==> 하나의 쓰레드가 DB 화일의 Chunk( ~M) 단위로
     *         로딩한다. system call 횟수가 적음.
     *         HP, AIX, DEC의 경우에 잇점이 있음.
     * ----------------------------------------------*/
    // moved to smc
    static IDE_RC loadParallel(smmTBSNode *     aTBSNode);

    // 디스크상의 데이터베이스 이미지를 메모리로 로드한다
    // ( 1개의 Thread만 사용 )
    static IDE_RC loadSerial2( smmTBSNode *     aTBSNode);

    // DB File로부터 테이블에 할당된 Page들을 메모리로 로드한다.
    static IDE_RC loadDbFile(smmTBSNode *     aTBSNode,
                             UInt             aFileNumber,
                             scPageID         aFileMinPID,
                             scPageID         aFileMaxPID,
                             UInt             aLoadPageCount );

    // 하나의 DB파일에 속한 모든 Page를 메모리 Page로 로드한다.
    static IDE_RC loadDbPagesFromFile( smmTBSNode *     aTBSNode,
                                       UInt             aFileNumber,
                                       scPageID         aFileMinPID,
                                       ULong            aLoadPageCount );

    // 데이터베이스 파일의 일부 조각(Chunk) 하나를
    // 메모리 페이지로 로드한다.
    static IDE_RC loadDbFileChunk( smmTBSNode *     aTBSNode,
                                   smmDatabaseFile * aDbFile,
                                   void            * aAlignedBuffer,
                                   scPageID          aFileMinPID,
                                   scPageID          aChunkStartPID,
                                   ULong             aChunkPageCount );

    static void setLstCreatedDBFileToAllTBS ( );

    static IDE_RC syncDB( sctStateSet aSkipStateSet,
                          idBool aSyncLatch );
    static IDE_RC syncTBS(smmTBSNode * aTBSNode);

    static void setStartupPID(smmTBSNode *     aTBSNode,
                              scPageID         aPid)
    {
        aTBSNode->mStartupPID = aPid;
    }

    static IDE_RC checkExpandChunkProps(smmMemBase * aMemBase);



    // 데이터베이스 안의 Page중 Free Page에 할당된 메모리를 해제한다
    static IDE_RC freeAllFreePageMemory(void);
    static IDE_RC freeTBSFreePageMemory(smmTBSNode * aTBSNode);

    /* -----------------------
     * [3] persistent  memory
     *     manipulations
     * ----------------------*/

    // 여러개의 Expand Chunk를 추가하여 데이터베이스를 확장한다.
    static IDE_RC allocNewExpandChunks( smmTBSNode *  aTBSNode,
                                        void       *  aTrans,
                                        UInt          aExpandChunkCount );


    // 특정 페이지 범위만큼 데이터베이스를 확장한다.
    static IDE_RC allocNewExpandChunk( smmTBSNode  * aTBSNode,
                                       void        * aTrans,
                                       scPageID      aNewChunkFirstPID,
                                       scPageID      aNewChunkLastPID );

    // 미디어 복구의 AllocNewExpandChunk에 대한 Logical Redo를 수행한다.
    static IDE_RC allocNewExpandChunk4MR( smmTBSNode *  aTBSNode,
                                          scPageID      aNewChunkFirstPID,
                                          scPageID      aNewChunkLastPID,
                                          idBool        aSetFreeListOfMembase,
                                          idBool        aSetNextFreePageOfFPL );
#if 0 // not used
    // DB로부터 하나의 Page를 할당받는다.
    static IDE_RC allocatePersPage (void       *  aTrans,
                                    scSpaceID     aSpaceID,
                                    void      **  aAllocatedPage);
#endif

    // DB로부터 Page를 여러개 할당받아 트랜잭션에게 Free Page를 제공한다.
    // aHeadPage부터 aTailPage까지
    // Page Header의 Prev/Next포인터로 연결해준다.
    static IDE_RC allocatePersPageList (void        *aTrans,
                                        scSpaceID    aSpaceID,
                                        UInt         aPageCount,
                                        void       **aHeadPage,
                                        void       **aTailPage,
                                        UInt        *aAllocPageCnt);
#if 0
    // 하나의 Page를 데이터베이스로 반납한다
    static IDE_RC freePersPage (void     *   aTrans,
                                scSpaceID    aSpaceID,
                                void     *   aToBeFreePage );
#endif
    // 여러개의 Page를 한꺼번에 데이터베이스로 반납한다.
    // aHeadPage부터 aTailPage까지
    // Page Header의 Prev/Next포인터로 연결되어 있어야 한다.
    static IDE_RC freePersPageList (void        *aTrans,
                                    scSpaceID    aSpaceID,
                                    void        *aHeadPage,
                                    void        *aTailPage,
                                    smLSN       *aNTALSN );

    /* -----------------------
     * [*] retrieval of
     *     the memory manager
     *     informations
     * ----------------------*/
    // 특정 Page의 PCH를 가져온다.
    static smPCHBase  * getPCHBase(scSpaceID aSpaceID, scPageID aPID );
    static smmPCH      * getPCH(scSpaceID aSpaceID, scPageID aPID );
    static smPCSlot  * getPCHSlot(scSpaceID aSpaceID, scPageID aPID );
    static inline idBool isPageExist( scSpaceID aSpaceID, scPageID aPID );
    static inline void * getPagePtr(scSpaceID aSpaceID, scPageID aPID );
    // OID를 메모리 주소값으로 변환한다.
    static inline IDE_RC getOIDPtr( scSpaceID      aSpaceID,
                                    smOID          aOID,
                                    void        ** aPtr );

    // 특정 Page에 S래치를 획득한다. ( 현재는 X래치로 구현되어 있다 )
    static IDE_RC            holdPageSLatch(scSpaceID aSpaceID,
                                            scPageID  aPageID);
    // 특정 Page에 X래치를 획득한다.
    static IDE_RC            holdPageXLatch(scSpaceID aSpaceID,
                                            scPageID  aPageID);
    // 특정 Page에서 래치를 풀어준다.
    static IDE_RC            releasePageLatch(scSpaceID aSpaceID,
                                              scPageID  aPageID);
    // Membase에 설정된 DB File수를 가져온다.
    static UInt              getDbFileCount(smmTBSNode * aTBSNode,
                                            SInt         aCurrentDB);

    // 데이터베이스 확장에 따라 새로운 Expand Chunk가 할당됨에 따라
    // 새로 생겨나게 되는 DB파일의 수를 계산한다.
    static IDE_RC            calcNewDBFileCount( smmTBSNode * aTBSNode,
                                                 scPageID     aChunkFirstPID,
                                                 scPageID     aChunkLastPID,
                                                 UInt     *   aNewDBFileCount);

    // 특정 Page가 속하는 데이터베이스 파일 번호를 알아낸다.
    static SInt              getDbFileNo(smmTBSNode * aTBSNode,
                                         scPageID     aPageID);

    // 특정 DB파일이 몇개의 Page를 저장하여야 하는지를 리턴한다.
    static scPageID      getPageCountPerFile(smmTBSNode * aTBSNode,
                                             UInt         aDBFileNo );

    static IDE_RC        openOrCreateDBFileinRecovery( smmTBSNode * aTBSNode,
                                                       SInt         aDBFileNo, 
                                                       idBool     * aIsCreated );

    // Disk에 존재하는 데이터베이스 Page의 수를 계산한다
    static IDE_RC        calculatePageCountInDisk(smmTBSNode *  aTBSNode);
    static SInt          getCurrentDB( smmTBSNode * aTBSNode );
    static SInt          getNxtStableDB( smmTBSNode * aTBSNode );

    // 데이터베이스 파일 객체를 리턴한다.
    // 필요하다면 모든 DB 디렉토리에서 DB파일을 찾는다 )
    static IDE_RC getDBFile( smmTBSNode *          aTBSNode,
                             UInt                  aStableDB,
                             UInt                  aDBFileNo,
                             smmGetDBFileOption    aOp,
                             smmDatabaseFile    ** aDBFile );


    // 데이터베이스 File을 Open하고, 데이터베이스 파일 객체를 리턴한다
    static IDE_RC openAndGetDBFile( smmTBSNode *      aTBSNode,
                                    SInt              aStableDB,
                                    UInt              aDBFileNo,
                                    smmDatabaseFile **aDBFile );

    // BUG-34530 
    // SYS_TBS_MEM_DIC테이블스페이스 메모리가 해제되더라도
    // DicMemBase포인터가 NULL로 초기화 되지 않습니다.
    static void clearCatalogPointers();

    /* ------------------------------------------------
     * [*] Database Validation for Shared Memory
     * ----------------------------------------------*/
    static void validate(smmTBSNode * aTBSNode)
    {
        if (aTBSNode->mRestoreType != SMM_DB_RESTORE_TYPE_DYNAMIC)
        {
            smmFixedMemoryMgr::validate(aTBSNode);
        }
    }
    static void invalidate(smmTBSNode * aTBSNode)
    {
        if (aTBSNode->mRestoreType != SMM_DB_RESTORE_TYPE_DYNAMIC)
        {
            smmFixedMemoryMgr::invalidate(aTBSNode);
        }
    }

    static void   getTableSpaceAttr(smmTBSNode * aTBSNode,
                                    smiTableSpaceAttr * aTBSAttr)
    {
        IDE_DASSERT( aTBSNode != NULL );
        IDE_DASSERT( aTBSAttr != NULL );

        idlOS::memcpy(aTBSAttr,
                      &aTBSNode->mTBSAttr,
                      ID_SIZEOF(smiTableSpaceAttr));
        aTBSAttr->mType = aTBSNode->mHeader.mType;
        aTBSAttr->mTBSStateOnLA = aTBSNode->mHeader.mState;
    };

    static UInt getTBSAttrFlag( smmTBSNode    * aTBSNode )
    {
        IDE_DASSERT( aTBSNode != NULL );

        return aTBSNode->mTBSAttr.mAttrFlag;
    };
    static void setTBSAttrFlag(smmTBSNode    * aTBSNode,
                               UInt            aAttrFlag )
    {
        IDE_DASSERT( aTBSNode != NULL );

        aTBSNode->mTBSAttr.mAttrFlag = aAttrFlag;
    };

    static void switchCurrentDB(smmTBSNode * aTBSNode)
    {
        aTBSNode->mTBSAttr.mMemAttr.mCurrentDB =
            (aTBSNode->mTBSAttr.mMemAttr.mCurrentDB + 1) % SM_PINGPONG_COUNT;
    }

    // 데이터 베이스의 Base Page (Page#0) 와 관련한 정보를 설정한다.
    // 이와 관련된 정보로는  MemBase와 Catalog Table정보가 있다.
    static IDE_RC setupBasePageInfo( smmTBSNode * aTBSNode,
                                     UChar *      aBasePage );

    // ChkptImageNode로 부터 AnchorOffset을 구한다.
    static inline UInt getAnchorOffsetFromCrtDBFileInfo( smmTBSNode* aSpaceNode,
                                                         UInt        aFileNum )
    {
        IDE_ASSERT( aSpaceNode != NULL );
        IDE_ASSERT( aFileNum < aSpaceNode->mHighLimitFile );
        return aSpaceNode->mCrtDBFileInfo[ aFileNum ].mAnchorOffset;
    }

    /* in order to dumpci utility uses server modules */
    static void setCallback4Util( smmGetPersPagePtrFunc aFunc )
    {
        mGetPersPagePtrFunc = aFunc;
    }
private:
    static void dump(FILE     *a_fp,
                     scPageID  a_no);
};

/*
 * 특정 Page의 PCH를 가져온다
 *
 * aPID [IN] PCH를 가져올 페이지의 ID
 */
inline smPCHBase * smmManager::getPCHBase(scSpaceID aSpaceID, scPageID aPID )
{
    return mPCArray[aSpaceID].mPC[ aPID ].mPCH;
}

inline smmPCH *
smmManager::getPCH(scSpaceID aSpaceID, scPageID aPID )
{
    IDE_DASSERT( isValidPageID( aSpaceID, aPID ) == ID_TRUE );
    return (smmPCH*)getPCHBase( aSpaceID, aPID );
}

inline smPCSlot *
smmManager::getPCHSlot(scSpaceID aSpaceID, scPageID aPID )
{
    IDE_DASSERT( isValidPageID( aSpaceID, aPID ) == ID_TRUE );

    return &mPCArray[ aSpaceID ].mPC[ aPID ];
}

inline void * smmManager::getPagePtr( scSpaceID aSpaceID, scPageID aPID )
{
    if ( isValidPageID( aSpaceID, aPID ) == ID_TRUE )
    {
        return mPCArray[ aSpaceID ].mPC[ aPID ].mPagePtr ;
    }
    return NULL ;
}

inline idBool smmManager::isPageExist( scSpaceID aSpaceID, scPageID aPID )
{
    if( isValidPageID( aSpaceID, aPID ) == ID_TRUE )
    {
        if ((mPCArray[aSpaceID].mPC[ aPID ].mPCH != NULL ) &&
            (mPCArray[aSpaceID].mPC[ aPID ].mPagePtr != NULL ))
        {
            return ID_TRUE;
        }
    }
    return ID_FALSE;
}


/*
 * OID를 메모리 주소값으로 변환한다.
 *
 * aOID [IN] 메모리 주소값으로 변환할 Object ID
 */
inline IDE_RC smmManager::getOIDPtr( scSpaceID     aSpaceID,
                                     smOID         aOID,
                                     void       ** aPtr )
{
    scOffset sOffset;
    scPageID sPageID;

    sPageID = (scPageID)(aOID >> SM_OFFSET_BIT_SIZE);
    sOffset = (scOffset)(aOID & SM_OFFSET_MASK);

    IDE_TEST( getPersPagePtr( aSpaceID, sPageID, aPtr ) != IDE_SUCCESS );

    (*aPtr) = (void *)( ((UChar *)(*aPtr)) + sOffset  );

    return IDE_SUCCESS ;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

///*
// * GRID를 메모리 주소값으로 변환한다.
// *
// * aGRID [IN] 메모리 주소값으로 변환할 GRID
// */
//inline IDE_RC smmManager::getGRIDPtr( scGRID aGRID, void ** aPtr )
//{
//    IDE_TEST( getPersPagePtr( SC_MAKE_SPACE( aGRID ),
//                              SC_MAKE_PID( aGRID ),
//                              aPtr )
//              != IDE_SUCCESS );
//
//    (*aPtr) = (void *)( ((UChar *)(*aPtr)) + SC_MAKE_OFFSET( aGRID ) );
//
//    return IDE_SUCCESS ;
//
//    IDE_EXCEPTION_END;
//
//    return IDE_FAILURE;
//}

/*
 * Membase에 설정된 DB File수를 가져온다.
 *
 * aCurrentDB [IN] Ping/Pong 데이터베이스 번호 ( 0 혹은 1 )
 */

inline UInt
smmManager::getDbFileCount(smmTBSNode * aTBSNode,
                           SInt         aCurrentDB)
{
    IDE_DASSERT( (aCurrentDB == 0) || (aCurrentDB == 1) );
    IDE_DASSERT( aTBSNode->mRestoreType !=
                 SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET );



    return smmDatabase::getDBFileCount(aTBSNode->mMemBase, aCurrentDB);
}

/* Page ID를 근거로, 하나의 데이터파일 내에서의 Page 번호를 구한다.
 * Page번호는 0부터 시작하여 1씩 순차적으로 증가하는 값이다.
 *
 * 하나의 데이터파일은 여러개의 Expand Chunk를 가지며,
 * 하나의 Expand Chunk는 여러개의 데이터파일이 걸쳐 있을 수 없으므로,
 * 데이터파일은 Expand Chunk Page수로 정확히 나누어 떨어지는 일정한
 * 페이지 수를 가지게 된다.
 *
 * 바로 이 하나의 데이터파일이 가지는 Page수가 바로 mPageCountPerFile이다.
 *
 * 예외적으로 첫번째 데이터파일의 경우 Membase및 Catalog Table을 지니는
 * Page인 Database metapage를 지니게 되어 SMM_DATABASE_META_PAGES 만큼
 * 더 많은 페이지를 가지게 된다 ( 현재 1 개의 Page를 더 가짐. )
 *
 */
inline UInt smmManager::getPageNoInFile( smmTBSNode * aTBSNode,
                                         scPageID     aPageID )
{
    UInt          sPageNoInFile;
    // Primitive Operation이어서 aPageID에 대한 Validation을 할 수 없음

    // 데이터베이스 Meta Page를 읽어올 때는 m_membase조차 읽어오지 않은 상황.
    // 이 때 getPageCountPerFile에서 mPageCountPerFile 이 초기화가
    // 안된 상태이다. ( mPageCountPerFile은 m_membase의 ExpandChunk당 Page수
    // 를 이용하여 계산되기 때문.
    if ( aPageID < SMM_DATABASE_META_PAGE_CNT )
    {
        sPageNoInFile = aPageID;
    }
    else
    {
        IDE_ASSERT( smmDatabase::getDBFilePageCount(aTBSNode->mMemBase) != 0 );

        sPageNoInFile = (UInt)( ( aPageID - SMM_DATABASE_META_PAGE_CNT )
                                % smmDatabase::getDBFilePageCount(aTBSNode->mMemBase) );

        // 만약 첫번째 데이터파일에 속한 Page라면
        if ( aPageID < (smmDatabase::getDBFilePageCount(aTBSNode->mMemBase)
                        + SMM_DATABASE_META_PAGE_CNT) )
        {
            // 맨 앞에 Meta Page가 있기 때문에 이만큼을 보정한다.
            sPageNoInFile += SMM_DATABASE_META_PAGE_CNT;
        }
    }

    return sPageNoInFile ;
}

/*
 * 특정 Page가 속하는 데이터베이스 파일 번호를 알아낸다.
 * ( 데이터베이스 파일의 번호는 0부터 시작한다 )
 *
 * aPageID [IN]  어느 데이터베이스 파일에 속한 것인지 알고싶은 페이지의 ID
 * return  [OUT] aPageID가 속한 데이터베이스 파일의 번호
 */
inline SInt
smmManager::getDbFileNo(smmTBSNode * aTBSNode, scPageID aPageID)
{
    vULong   sDbFilePageCount;

    IDE_DASSERT( isValidPageID(aTBSNode->mTBSAttr.mID, aPageID) == ID_TRUE );

    // 데이터베이스 메타 페이지는 항상 0번 파일에 존재한다.
    // 이러한 이유로 0번 파일은 크기가 다른 파일보다
    // SMM_DATABASE_META_PAGE_CNT만큼 크다.
    if ( aPageID < SMM_DATABASE_META_PAGE_CNT )
    {
        return 0;
    }
    else
    {
        /*
          IDE_ASSERT( smmDatabase::getDBFilePageCount(aTBSNode->mMemBase) != 0 );

          return (SInt)( ( aPageID - SMM_DATABASE_META_PAGE_CNT )
          / smmDatabase::getDBFilePageCount(aTBSNode->mMemBase) );
        */
        sDbFilePageCount = aTBSNode->mTBSAttr.mMemAttr.mSplitFilePageCount;
        IDE_ASSERT( sDbFilePageCount != 0 );

        return (SInt)( ( aPageID - SMM_DATABASE_META_PAGE_CNT )
                       / sDbFilePageCount  );
    }
}

/*
 * 특정 DB파일이 몇개의 Page를 저장하여야 하는지를 리턴한다.
 *
 * PROJ-1490 페이지 리스트 다중화및 메모리 반납
 *
 * DB파일안의 Free Page는 Disk로 내려가지도 않고
 * 메모리로 올라가지도 않는다.
 * 그러므로, DB파일의 크기와 DB파일에 저장되어야 할 Page수와는
 * 아무런 관계가 없다.
 *
 * 하나의 DB파일에 기록되는 Page의 수를 계산한다.
 *
 */
inline scPageID
smmManager::getPageCountPerFile(smmTBSNode * aTBSNode, UInt aDBFileNo )
{
    scPageID sDBFilePageCount;

    sDBFilePageCount = aTBSNode->mTBSAttr.mMemAttr.mSplitFilePageCount;
    IDE_DASSERT( sDBFilePageCount != 0 );

#ifdef DEBUG
    // aTBSNode->mMembase == NULL일때에도 이 함수가 호출될 수 있다.
    if (aTBSNode->mMemBase != NULL)
    {
        IDE_DASSERT( sDBFilePageCount ==
                     smmDatabase::getDBFilePageCount(aTBSNode->mMemBase) );
    }
#endif

    // 데이터베이스 메타 페이지는 항상 0번 파일에 존재한다.
    // 이러한 이유로 0번 파일은 크기가 다른 파일보다
    // SMM_DATABASE_META_PAGE_CNT만큼 크다.
    if ( aDBFileNo == 0 )
    {
        sDBFilePageCount += SMM_DATABASE_META_PAGE_CNT ;
    }

    return sDBFilePageCount ;
}

#if 0
/* TRUE만 리턴하고 있어서 함수삭제함. */

// Page 수가 Valid한 값인지 체크한다.
inline idBool
smmManager::isValidPageCount( smmTBSNode * /* aTBSNode*/,
                              vULong       /*aPageCount*/ )
{
/*
  IDE_ASSERT( mHighLimitPAGE > 0 );
  return ( aPageCount <= mHighLimitPAGE ) ?
  ID_TRUE : ID_FALSE ;
*/
    return ID_TRUE;
}
#endif

// Page ID가 Valid한 값인지 체크한다.
// 이함수는 Validation작업이 없습니다. 사용하는 곳에서 aSpaceID 가 정상인지 확인이 필요합니다. 
inline idBool smmManager::isValidPageID( scSpaceID  aSpaceID ,
                                         scPageID   aPageID )
{
    return ( aPageID < mPCArray[aSpaceID].mMaxPageCount ) ? ID_TRUE : ID_FALSE;
}

#if 0 // not used
/***********************************************************************
 * Description : 찾는 PCH가 메모리에 존재하고 있는지 확인한다.
 *
 * [IN] aSpaceID    - SpaceID
 * [IN] aPageID     - PageID
 **********************************************************************/
inline idBool
smmManager::isExistPCH( scSpaceID aSpaceID, scPageID aPageID )
{
    if ( isValidPageID(aSpaceID, aPageID) == ID_TRUE )
    {
        if ( ( mPCHArray[aSpaceID][aPageID] != NULL ) &&
             ( mPCHArray[aSpaceID][aPageID]->m_page != NULL ) )
        {
            return ID_TRUE;
        }
    }

    return ID_FALSE;
}
#endif 
/*
 * 하나의 Page의 데이터가 저장되는 메모리 공간을 PCH Entry에서 가져온다.
 *
 * aPID [IN] 페이지의 ID
 * return Page의 데이터가 저장되는 메모리 공간
 */
/* BUG-32479 [sm-mem-resource] refactoring for handling exceptional case about
 * the SMM_OID_PTR and SMM_PID_PTR macro. */
IDE_RC smmManager::getPersPagePtr( scSpaceID    aSpaceID, 
                                   scPageID     aPID,
                                   void      ** aPersPagePtr )
{
    IDE_ERROR( aPersPagePtr != NULL );

    (*aPersPagePtr) = getPagePtr( aSpaceID, aPID );
    
    if( (*aPersPagePtr) == NULL )
    {
        return getPersPagePtrErr( aSpaceID, 
                                  aPID,
                                  aPersPagePtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif // _O_SMM_MANAGER_H_

