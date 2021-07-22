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
 * $Id: sddDef.h 88191 2020-07-27 03:08:54Z mason.lee $
 *
 * Description :
 *
 * 본 파일은 Resource Layer 자료구조의 헤더 파일이다.
 *
 **********************************************************************/

#ifndef _O_SDD_DEF_H_
#define _O_SDD_DEF_H_ 1

#include <idu.h>
#include <smu.h>
#include <sctDef.h>
#include <smriDef.h>

/* --------------------------------------------------------------------
 * Description : tablespace에 대한 해쉬 테이블의 크기
 * ----------------------------------------------------------------- */
#define SDD_HASH_TABLE_SIZE  (128)

/* ------------------------------------------------
 * IO 모드 : sddDiskMgr::completeIO에서 처리함
 * ----------------------------------------------*/
typedef enum
{
    SDD_IO_READ  = 0,
    SDD_IO_WRITE
} sddIOMode;

#define SDD_CALC_PAGEOFFSET(aPageCnt) \
        (SM_DBFILE_METAHDR_PAGE_OFFSET + SM_DBFILE_METAHDR_PAGE_SIZE + \
        (aPageCnt * SD_PAGE_SIZE))

typedef enum sddSyncType
{
    /* 디스크 공간 할당/해제 및 변경에 대한 타입 */
    SDD_SYNC_NORMAL = 0,
    SDD_SYNC_CHKPT
} sddSyncType;

// 디스크 데이타파일의 메타헤더
typedef struct sddDataFileHdr
{
    UInt    mSmVersion;

     // 미디어복구를 위한 RedoLSN
    smLSN   mRedoLSN;

     // 미디어복구를 위한 CreateLSN
    smLSN   mCreateLSN;

     // 미디어복구를 위한 DiskLstLSN
    smLSN   mMustRedoToLSN;

    // PROJ-2133 incremental backup
    smiDataFileDescSlotID    mDataFileDescSlotID;
    
    // PROJ-2133 incremental backup
    // incremental backup된 파일에만 존재하는 정보
    smriBISlot  mBackupInfo;

} sddDataFileHdr;

/* --------------------------------------------------------------------
 * Description : 데이타 화일에 대한 정보
 * ----------------------------------------------------------------- */
typedef struct sddDataFileNode
{
    // tablespace별로 유일하게 식별되는 데이타 화일의 ID
    scSpaceID        mSpaceID;
    sdFileID         mID;
    // state of the data file node(not used, but will be used by msjung)
    UInt             mState;
    sddDataFileHdr   mDBFileHdr;
    /* ------------------------------------------------
     * 다음의 mNextSize 부터 mIsAutoExtend 속성이 sddDataFileAttr에도
     * 정의되어 있다. 왜냐하면, 로그앵커에 저장된 sddDataFileAttr를
     * 읽어서 datafile 노드를 초기화하도록 하기 위함이다.
     * ------------------------------------------------ */
    ULong            mNextSize;       // 확장될 페이지 개수
    ULong            mMaxSize;        // 최대 페이지 개수
    ULong            mInitSize;       // 초기 페이지 개수

    /* ------------------------------------------------
     * - 데이타파일의 현재 페이지 개수
     * 처음에 INIT SIZE가 할당되고 화일이 autoextend, resize될때
     * 확장된다.
     * startup시 이 크기와 실제 화일의 크기를 비교할 필요가 없다.
     * 화일 확장 후, 로그앵커에 쓰기 전에 시스템이 abort가 발생하더라도
     * 확장된 부분은 초기화가 완료되지 못한 상황이라 사용하지 못한다.
     * 물론 restart recovery시에 undo 처리과정에서 다시 확장에 대한
     * 요구사항이 발생하거나 서비스 시작후에 재확장 및 초기화를 시도될
     * 수 있다.
     * ----------------------------------------------*/
    ULong             mCurrSize;
    smiDataFileMode   mCreateMode;     // datafile 생성 모드
    SChar*            mName;           // 화일 이름
    idBool            mIsAutoExtend;   // 자동확장 여부
    UInt              mIOCount;        // IO가 현재 몇개가 진행 중인가
    idBool            mIsOpened;       // Open 된 상태여부
    idBool            mIsModified;     // 파일의 수정여부 및 flush할때 필요
    smuList          mNode4LRUList;   // open된 datafile 리스트
    iduFile           mFile;           // 데이타 화일

    UInt              mAnchorOffset;    // Loganchor 메모리 버퍼내의 DBF 속성 위치

    UChar*            mPageBuffPtr;
    UChar*            mAlignedPageBuff;

    iduMutex          mMutex;           // File의 Open과 Close그리고 IOCount IsOpened 를 보호한다.
} sddDataFileNode;

/* ------------------------------------------------
 * Description : tablespace 정보를 저장하는 자료구조
 *
 * 물리적인 tablespace 에 대한 메모리 노드를 표현하는
 * 자료구조이다.
 * ----------------------------------------------*/
typedef struct sddTableSpaceNode
{
    sctTableSpaceNode  mHeader;

    /* PROJ-1671 Bitmap-based Tablespace And Segment
     * Space Management */
    void              * mSpaceCache;     /* Space Cache */

    /* loganchor로부터 초기화되거나, 테이블스페이스 생성시 설정됨 */
    smiExtMgmtType     mExtMgmtType;     /* Extemt의 공간관리 방식 */
    smiSegMgmtType     mSegMgmtType;     /* Segment의 공간관리 방식 */
    UInt               mExtPageCount;    /* Extent 페이지 개수 */

    // tablespace 속성 Flag
    // ( ex> Tablespace안의 데이터 변경에 대해 Log Compress여부 )
    UInt               mAttrFlag; 

    sdFileID           mNewFileID; // tablespace에 속한 데이타화일에 id부여

    UInt               mDataFileCount; // 테이블스페이스의 소속된 파일개수

    //PRJ-1671  Bitmap-based Tablespace And Segment Space Management
    //data file node를 요소로 갖는 배열
    sddDataFileNode  * mFileNodeArr[ SD_MAX_FID_COUNT] ; 

    ULong              mTotalPageCount;     // TBS의 포함된 총 페이지 개수

    /* fix BUG-17456 Disk Tablespace online이후 update 발생시 index 무한루프
     * 초기값은 (0,0)이며, Online 시에는 Online TBS LSN이 기록된다. */
    smLSN              mOnlineTBSLSN4Idx;
    
    /* fix BUG-24403 Disk Tablespace online이후 hang 발생 방지를 위해
     * offline 할때 SMO 값을 저장한다. */
    ULong              mMaxSmoNoForOffline;

    UInt               mAnchorOffset;       // Loganchor 메모리 버퍼내의 TBS 속성 위치
} sddTableSpaceNode;

typedef IDE_RC (*sddReadPageFunc)(idvSQL          * aStatistics,
                                  sddDataFileNode * aFileNode,
                                  scPageID          aFstPID,
                                  ULong             aPageCnt,
                                  UChar           * aBuffer,
                                  UInt            * aState );

typedef IDE_RC (*sddWritePageFunc)(idvSQL          * aStatistics,
                                   sddDataFileNode * aFileNode,
                                   scPageID          aPageID,
                                   UChar           * aBuffer,
                                   UInt            * aState );

/* DoubleWrite File Prefix 정의 */
#define   SDD_DWFILE_NAME_PREFIX     "dwfile"

/* DoubleWrite File Prefix 정의2 */
#define   SDD_SBUFFER_DWFILE_NAME_PREFIX    "sdwfile"


#endif // _O_SDD_DEF_H_
