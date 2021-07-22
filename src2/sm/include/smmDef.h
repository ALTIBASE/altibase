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
 * $Id: smmDef.h 90083 2021-02-26 00:58:48Z et16 $
 **********************************************************************/

#ifndef _O_SMM_DEF_H_
#define _O_SMM_DEF_H_ 1

#include <idu.h>
#include <idnCharSet.h>
#include <iduMutex.h>
#include <smDef.h>
#include <smuList.h>
#include <sctDef.h>
#include <smriDef.h>


#define SMM_PINGPONG_COUNT   (2)  // ping & pong

#define SMM_MAX_AIO_COUNT_PER_FILE  (8192)  // 한 화일에 대한 AIO 최대 갯수
#define SMM_MIN_AIO_FILE_SIZE       (1 * 1024 * 1024) // 최소 AIO 수행 화일 크기 

// BUG-29607 Checkpoint Image File Name의 형식
// TBSName "-" PingpongNumber "-"
// TBSName "-" PingpongNumber "-" FileNumber
// DirPath "//" TBSName "-" PingpongNumber "-" FileNumber
#define SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG          \
                           "%s-%"ID_UINT32_FMT"-"
#define SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG_FILENUM  \
                           SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG"%"ID_UINT32_FMT
#define SMM_CHKPT_IMAGE_NAME_WITH_PATH             \
                           "%s"IDL_FILE_SEPARATORS""SMM_CHKPT_IMAGE_NAME_TBS_PINGPONG_FILENUM
#define SMM_CHKPT_IMAGE_NAMES  "%s-.."

/* PROJ-1490 DB Free Page List
 */
typedef struct smmDBFreePageList
{
    // Free Page List ID
    // smmMemBase.mFreePageLists내에서의 index
    scPageID   mFirstFreePageID ; // 첫번째 Free Page 의 ID
    vULong     mFreePageCount ;   // Free Page 수
} smmDBFreePageList ;

// 최대 Free Page List의 수 
#define SMM_MAX_FPL_COUNT (SM_MAX_PAGELIST_COUNT)


typedef struct smmMemBase
{
    // fix when createdb
    SChar           mDBname[SM_MAX_DB_NAME]; // DB Name
    SChar           mProductSignature[IDU_SYSTEM_INFO_LENGTH];
    SChar           mDBFileSignature[IDU_SYSTEM_INFO_LENGTH];
    UInt            mVersionID;              // Altibase Version
    UInt            mCompileBit;             // Support Mode "32" or '"64"
    idBool          mBigEndian;              // big = ID_TRUE 
    vULong          mLogSize;                // Logfile Size
    vULong          mDBFilePageCount;        // one dbFile Page Count
    UInt            mTxTBLSize;    // Transaction Table Size;
    UInt            mDBFileCount[ SMM_PINGPONG_COUNT ];

    // PROJ-1579 NCHAR
    SChar           mDBCharSet[IDN_MAX_CHAR_SET_LEN];
    SChar           mNationalCharSet[IDN_MAX_CHAR_SET_LEN];

    // change in run-time
    struct timeval  mTimestamp;
    vULong          mAllocPersPageCount;

    // PROJ 2281
    // Storing SystemStat persistantly 
    smiSystemStat   mSystemStat;

    // PROJ-1490 페이지 리스트 다중화및 메모리 해제 관련 멤버들
    // Expand Chunk는 데이터베이스를 확장하는 최소 단위이다.
    // Exapnd Chunk의 크기는 데이터베이스 생성시에 결정되며, 변경이 불가능한다.
    vULong          mExpandChunkPageCnt;    // ExpandChunk 당 Page수
    vULong          mCurrentExpandChunkCnt; // 현재 할당한 ExpandChunk의 수
    
    // 여러개로 다중화된 DB Free Page List및 그 갯수
    UInt                mFreePageListCount;
    smmDBFreePageList   mFreePageLists[ SMM_MAX_FPL_COUNT ];
    // system SCN
    smSCN           mSystemSCN;
} smmMemBase;

/* ----------------------------------------------------------------------------
 *    TempPage Def
 * --------------------------------------------------------------------------*/
#if defined(WRS_VXWORKS)
#undef m_next
#endif 
struct smmTempPage;
typedef struct smmTempPageHeader
{
    smmTempPage * m_self;
    smmTempPage * m_prev;
    smmTempPage * m_next;
} smmTempPageHeader;

struct __smmTempPage__
{
    smmTempPageHeader m_header;
    vULong            m_body[1]; // for align calculation 
};

#define SMM_TEMP_PAGE_BODY_SIZE (SM_PAGE_SIZE - offsetof(__smmTempPage__, m_body))

struct smmTempPage
{
    smmTempPageHeader m_header;
#if !defined(COMPILE_64BIT)
    UInt              m_align;
#endif
    UChar             m_body[SMM_TEMP_PAGE_BODY_SIZE];
};
/* ========================================================================*/

typedef struct smmPCH : public smPCHBase // Page Control Header : PCH
{
    void               *mPageMemHandle;

    // BUG-17770: DML Transactino은 Page에 대한 갱신 또는 Read전에
    //            Page에 대해서 각기 X, S Latch를 잡는다. 이때 사용하는
    //            Mutex가 mPageMemMutex이다. 때문에 Checkpoint시에는 별도의
    //            Mutex를 사용하여야 한다.

    // m_page의 Page Memory를 Free하려는 Thread와 m_page를 Disk에 내리려는
    // Checkpoint Thread간의 동시성 제어를 위한 Mutex.
    // mPageMemMutex를 사용하게 되면, 일반 트랜잭션들과
    // Checkpoint Thread가 불필요한 Contension에 걸리게 된다.
    iduMutex            mMutex;

    // DML(insert, update, delete)연산을 수행하는 Transaction이
    // 잡는 Mutex

    idBool              m_dirty;
    UInt                m_dirtyStat; // CASE-3768 

    //added by newdaily because dirty page list
    smmPCH             *m_pnxtDirtyPCH; 
    smmPCH             *m_pprvDirtyPCH; // CASE-3768

    // for smrDirtyPageMgr
    scSpaceID           mSpaceID;
} smmPCH;


// BUG-43463 에서 사용 scanlist link/unlink 시에
// page가 List에서 제거되거나 연결 될 때 갱신한다.
// 홀수이면 수정중, 짝수이면 수정 완료
#define SMM_PCH_SET_MODIFYING( aPCH ) \
IDE_ASSERT( aPCH->mModifySeqForScan % 2 == 0); \
idCore::acpAtomicInc64( &(aPCH->mModifySeqForScan) );

#define SMM_PCH_SET_MODIFIED( aPCH ) \
IDE_ASSERT( aPCH->mModifySeqForScan % 2 != 0); \
idCore::acpAtomicInc64( &(aPCH->mModifySeqForScan) );

// BUG-43463 smnnSeq::moveNext/Prev등의 함수에서 사용
// 현재 page가 link/unlink작업 중인지 확인한다.
#define SMM_PAGE_IS_MODIFYING( aScanModifySeq ) \
( (( aScanModifySeq % 2 ) == 1) ? ID_TRUE : ID_FALSE )

typedef struct smmTempMemBase
{
    // for Temp Page
    iduMutex            m_mutex;
    smmTempPage        *m_first_free_temp_page;
    vULong              m_alloc_temp_page_count;
    vULong              m_free_temp_page_count;
} smmTempMemBase;

typedef struct smmDirtyPage
{
    smmDirtyPage *m_next;
    smPCSlot     *m_pch;  // BUG-48513
} smmDirtyPage;

    
struct smmShmHeader;

typedef struct smmSCH  // Shared memory Control Header
{
    smmSCH       *m_next;
    SInt          m_shm_id;
    smmShmHeader *m_header;
} smmSCH;


/* ------------------------------------------------
 *  CPU CACHE Set : BUGBUG : ID Layer로 옮길수 있음!
 * ----------------------------------------------*/
#if defined(SPARC_SOLARIS)
#define SMM_CPU_CACHE_LINE      (64)
#else
#define SMM_CPU_CACHE_LINE      (32)
#endif
#define SMM_CACHE_ALIGN(data)   ( ((data) + (SMM_CPU_CACHE_LINE - 1)) & ~(SMM_CPU_CACHE_LINE - 1))

/* ------------------------------------------------
 *  OFFSET OF MEMBASE PAGE
 * ----------------------------------------------*/
// BUGBUG!!! : SMM Cannot Refer to SMC. Original Definition is like below.
//
// #define SMM_MEMBASE_OFFSET      SMM_CACHE_ALIGN(offsetof(smcPersPage, m_body))
//
// Currently, we Fix 'offsetof(smcPersPage, m_body)' to 32.
// If smcPersPage is modified, this definition should be checked also.
// Ask to xcom73.
#define SMM_MEMBASE_PAGEID       ((scPageID)0)
#define SMM_MEMBASE_PAGE_SIZE    (SM_PAGE_SIZE)
#define SMM_MEMBASE_OFFSET       SMM_CACHE_ALIGN(32)
#define SMM_CAT_TABLE_OFFSET     SMM_CACHE_ALIGN(SMM_MEMBASE_OFFSET + ID_SIZEOF(struct smmMemBase))


/* ------------------------------------------------
 *  Shared Memory DB Control
 * ----------------------------------------------*/

/*
    Unaligned version of Shared Memory Header
 */
typedef struct smmShmHeader
{
    UInt          m_versionID;
    idBool        m_valid_state;   // operation 중에 죽으면 ID_FALSE
    scPageID      m_page_count;    // 할당된 page 갯수
    // PROJ-1548 Memory Table Space
    // 사용자가 SHM_DB_KEY_TBS_INTERVAL를 변경하여 
    // 공유메모리 Chunk가 다른 테이블 스페이스로
    // attach가 되는 경우를 체크하기 위함
    scSpaceID     mTBSID;          // 공유메모리 Chunk가 속한 TBS의 ID
    key_t         m_next_key;      // == 0 이면  next key가 없음
} smmShmHeader;

/*
    Aligned Version of Shared memory header

   공유메모리 Header는 공유메모리 영역안에서
   Cache Line크기로 Align한 크기만큼을 차지한다.
   이를 통해 Header바로 다음에 오는 공유메모리 Page들의 시작주소가
   Cache Line에 Align되도록 한다
 */

// 공유메모리 Header의 크기를 Cache Align하여
// 공유메모리 Page들의 시작주소가 Cache Align되도록 한다
#define SMM_CACHE_ALIGNED_SHM_HEADER_SIZE \
            ( SMM_CACHE_ALIGN( ID_SIZEOF(smmShmHeader) ) )
#define SMM_SHM_DB_SIZE(a_use_page)       \
            ( SMM_CACHE_ALIGNED_SHM_HEADER_SIZE + SM_PAGE_SIZE * a_use_page)
/* ------------------------------------------------
 *  [] Fixed Memory의 링크에 위치할 경우
 *     포인터를 아래의 값(0xFF..)으로 설정.
 *     attach()를 수행할 때 이 값을 이용함.
 * ----------------------------------------------*/
#ifdef COMPILE_64BIT
#define SMM_SHM_LOCATION_FREE     (ID_ULONG(0xFFFFFFFFFFFFFFFF))
#else
#define SMM_SHM_LOCATION_FREE     (0xFFFFFFFF)
#endif
/* ------------------------------------------------
 *  SLOT List
 * ----------------------------------------------*/
/* aMaximum VON smmSlotList::initialize         */
# define SMM_SLOT_LIST_MAXIMUM_DEFAULT       (100)

/* IN smmSlotList::initialize                   */
# define SMM_SLOT_LIST_MAXIMUM_MINIMUM       ( 10)

/* aCache VON smmSlotList::initialize           */
# define SMM_SLOT_LIST_CACHE_DEFAULT         ( 10)

/* IN smmSlotList::initialize                   */
# define SMM_SLOT_LIST_CACHE_MINIMUM         (  5)

/* aFlag    VOM smmSlotList::allocateSlots      *
 * aFlag    VOM smmSlotList::releaseSlots       */
# define SMM_SLOT_LIST_MUTEX_MASK     (0x00000001)
# define SMM_SLOT_LIST_MUTEX_NEEDLESS (0x00000000)
# define SMM_SLOT_LIST_MUTEX_ACQUIRE  (0x00000001)

/* CASE-3768, DOC-28381
 * 1. INIT->FLUSH->REMOVE->INIT
 * 2. INIT->FLUSH->REMOVE->[FLUSHDUP->REMOVE]*->INIT
 */

# define SMM_PCH_DIRTY_STAT_MASK      (0x00000003)
# define SMM_PCH_DIRTY_STAT_INIT      (0x00000000) 
# define SMM_PCH_DIRTY_STAT_FLUSH     (0x00000001) 
# define SMM_PCH_DIRTY_STAT_FLUSHDUP  (0x00000002) 
# define SMM_PCH_DIRTY_STAT_REMOVE    (0x00000003) 

typedef struct smmSlot
{
    smmSlot* prev;
    smmSlot* next;
}
smmSlot;

/* ------------------------------------------------
   db dir이 여러개 있을때 첫번째 db dir에 memory db를
   create한다. 
 * ----------------------------------------------*/
#define SMM_CREATE_DB_DIR_INDEX (0)



/* ------------------------------------------------
   PROJ-1490 페이지리스트 다중화및 메모리 반납 관련
 * ----------------------------------------------*/

// Free Page List 의 수
#define SMM_FREE_PAGE_LIST_COUNT (smuProperty::getPageListGroupCount())

// 최소 몇개의 Page를 가진 Free Page List를 분할 할 것인가?
#define SMM_FREE_PAGE_SPLIT_THRESHOLD                       \
            ( smuProperty::getMinPagesDBTableFreeList() )


// Expand Chunk할당시 각 Free Page에 몇개의 Page씩을 연결할 것인지 결정.
#define SMM_PER_LIST_DIST_PAGE_COUNT                       \
            ( smuProperty::getPerListDistPageCount() )

//// 페이지가 부족할 때 한번에 몇개의 Expand Chunk를 할당받을 것인가?
//#define SMM_EXPAND_CHUNK_COUNT (1)

// 데이터베이스 메타 페이지의 수
// 메타 페이지는 데이터베이스의 맨 처음에 위치하며
// Membase와 카탈로그 테이블 정보를 지닌다.
#define SMM_DATABASE_META_PAGE_CNT ((vULong)1)

// PROJ-1490 페이지리스트 다중화및 메모리반납
// Page가 테이블로 할당될 때 해당 Page의 Free List Info Page에 설정될 값
// 서버 기동시 Free Page와 Allocated Page를 구분하기 위한 용도로 사용된다.
#define SMM_FLI_ALLOCATED_PID ( SM_SPECIAL_PID )


// prepareDB 관련 함수들에 전달되는 옵션
typedef enum smmPrepareOption
{
    SMM_PREPARE_OP_NONE = 0,                    // 옵션없음 
    SMM_PREPARE_OP_DBIMAGE_NEED_RECOVERY = 1,   // DB IMAGE가 Recovery될 예정
    SMM_PREPARE_OP_DONT_CHECK_RESTORE_TYPE = 2,  // RestoreType 체크 안함
    SMM_PREPARE_OP_DBIMAGE_NEED_MEDIA_RECOVERY = 3,  // DB IMAGE가 Media Recovery될 예정
    SMM_PREPARE_OP_DONT_CHECK_DB_SIGNATURE_4SHMUTIL = 4  // shmutil이 사용 
} smmPrepareOption ;

// restoreDB 관련 함수들에 전달되는 옵션
typedef enum smmRestoreOption
{
    SMM_RESTORE_OP_NONE = 0,                        // 옵션없음
    // Restart Recovery가 필요한지 여부
    // DB IMAGE가 Recovery될 예정
    SMM_RESTORE_OP_DBIMAGE_NEED_RECOVERY = 1, 
    // DB IMAGE가 Media Recovery될 예정
    SMM_RESTORE_OP_DBIMAGE_NEED_MEDIA_RECOVERY = 2  
} smmRestoreOption ;

// PRJ-1548 User Memory Tablespace
// 데이터베이스의 종류
typedef enum
{
    // 일반메모리
    SMM_DB_RESTORE_TYPE_DYNAMIC    = 0, 
    // DISK상에 CREATE된 데이터베이스를 공유메모리로 RESTORE
    SMM_DB_RESTORE_TYPE_SHM_CREATE = 1,
    // 공유메모리상에 존재하는 데이터베이스를 ATTACH
    SMM_DB_RESTORE_TYPE_SHM_ATTACH = 2,
    // 아직 PREPARE/RESTORE가 되지 않았음
    SMM_DB_RESTORE_TYPE_NOT_RESTORED_YET = 3,
    // TYPE없음 ( 이 값을 가지는 경우가 있어서는 안됨 )
    SMM_DB_RESTORE_TYPE_NONE       = 4
} smmDBRestoreType;


// 현재 데이터베이스 상태가 서비스 중인지 아닌지를 지니는 FLAG
typedef enum
{
    SMM_STARTUP_PHASE_NON_SERVICE = 0, // 서비스중이 아님
    SMM_STARTUP_PHASE_SERVICE     = 1  // 서비스중임.
} smmStartupPhase;

/* 
   메모리 데이타파일에 대한 생성여부 및 로그앵커상의 Offset 등.. 
   Runtime 정보를 저장하는 구조체
*/
typedef struct smmCrtDBFileInfo
{
    // Stable/Unstable의 파일 생성여부를 저장한다.
    idBool    mCreateDBFileOnDisk[ SMM_PINGPONG_COUNT ];

    // Loganchor상에 저장된 Chkpt Image Attribute의 AnchorOffset를 저장한다. 
    UInt      mAnchorOffset;

} smmCrtDBFileInfo;

class smmDirtyPageMgr;


/* BUG-31881 [sm-mem-resource] When executing alter table in MRDB and 
 * using space by other transaction,
 * The server can not do restart recovery. 
 * AlterTable 중 RestoreTable 하기 위해 확보하는 Page들.
 * 동시에 SMM_PAGE_RESERVATION_MAX만큼의 Tx가 예약해둘 수 있다.*/

#define SMM_PAGE_RESERVATION_MAX  (64)
#define SMM_PAGE_RESERVATION_NULL ID_UINT_MAX

typedef struct smmPageReservation
{
    void     * mTrans     [ SMM_PAGE_RESERVATION_MAX ];
    SInt       mPageCount [ SMM_PAGE_RESERVATION_MAX ];
    UInt       mSize;
} smmPageReservation;

// Memory Tablespace Node
// 하나의 Memory Tablespace에 대한 모든 Runtime정보를 지닌다.
typedef struct smmTBSNode
{
    /******** FROM smmTableSpace ********************/
    // Memory / Disk TBS common informations
    sctTableSpaceNode mHeader;

    // Tablespace Attrubute
    smiTableSpaceAttr mTBSAttr;
    
    // Base node of Checkpoint Path attribute list
    smuList           mChkptPathBase;

    smmDirtyPageMgr * mDirtyPageMgr;
    
    /******** FROM smmManager ********************/
    smmSCH            mBaseSCH;
    smmSCH *          mTailSCH;
    smmDBRestoreType  mRestoreType;

    // Loading 시의 최대 PID : 메모리 해제방법이 틀려짐
    // PID가 아래의 값보다 작을 경우 mDataArea에 의해 해제되어야 하고,
    // 클 경우 TempMemPool에 의해 해제 되어야 함.
    scPageID          mStartupPID;

    ULong             mDBMaxPageCount;
    ULong             mHighLimitFile;
    UInt              mDBPageCountInDisk;
    UInt              mLstCreatedDBFile;
    void **           mDBFile[ SM_DB_DIR_MAX_COUNT ];

    // BUG-17343 loganchor에 Stable/Unstable Chkpt Image에 대한 생성 
    //           정보를 저장 
    // 메모리 데이타파일의 생성에 대한 Runtime 정보 
    smmCrtDBFileInfo* mCrtDBFileInfo; 

    // BUG-47487: FLI페이지 memPool 
    iduMemPool2       mFLIMemPagePool;
    
    iduMemPool2       mDynamicMemPagePool;
    iduMemPool        mPCHMemPool;
    smmTempMemBase    mTempMemBase;    // 공유메모리 페이지 리스트

    smmMemBase *      mMemBase;
    
    /******** FROM smmFPLManager ********************/

    // 각 Free Page List별 Mutex 배열
    // Mutex는 Durable한 정보가 아니기 때문에,
    // membase에 Free Page List의 Mutex를 함께 두지 않는다.
    iduMutex   * mArrFPLMutex;

    // 하나의 트랜잭션이 Expand Chunk를 할당하고 있을 때,
    // 다른 트랜잭션이 Page부족시 Expand Chunk를 또 할당하지 않고,
    // Expand Chunk할당 작업이 종료되기를 기다리도록 하기 위한 Mutex.
    iduMutex     mAllocChunkMutex;
    
    /* BUG-31881 [sm-mem-resource] When executing alter table in MRDB and 
     * using space by other transaction,
     * The server can not do restart recovery. 
     * FreePageListCount만큼 다중화되어 mArrFPLMutex를 통해 제어된다. */
    smmPageReservation * mArrPageReservation;

    /******** FROM smmExpandChunk ********************/

    // Free List Info Page내의 Slot기록을 시작할 위치 
    UInt              mFLISlotBase;
    // Free List Info Page의 수이다.
    UInt              mChunkFLIPageCnt;
    // 하나의 Expand Chunk가 지니는 Page의 수
    // Free List Info Page의 수 까지 포함한 전체 Page 수 이다.
    vULong            mChunkPageCnt;
    
    // PRJ-1548 User Memory Tablespace
    // Loganchor 메모리버퍼상의 TBS Attribute 저장오프셋
    UInt              mAnchorOffset;
    
} smmTBSNode;

// PRJ-1548 User Memory Tablespace
// 메모리테이블스페이스의 CHECKPOINT PATH 정보를 저장하는 노드
// 서버구동시에 Loganchor로부터 초기화되거나  DDL을 통해서 
// 생성된다.
//
// 이 구조체에 멤버가 추가되면
// smmTBSChkptPath::initializeChkptPathNode 도 함께 변경되어야 한다.
typedef struct smmChkptPathNode
{
    smiChkptPathAttr       mChkptPathAttr; // Chkpt Path 속성
    UInt                   mAnchorOffset;  // Loganchor상의 저장된 오프셋
} smmChkptPathNode;

// 메모리 체크포인트이미지(데이타파일)의 메타헤더
typedef struct smmChkptImageHdr
{
    // To Fix BUG-17143 TBS CREATE/DROP/CREATE시 DBF NOT FOUND에러
    scSpaceID mSpaceID;  // Tablespace의 ID

    UInt    mSmVersion;  // 바이너리 버전 저장 

    // 미디어복구를 위한 RedoLSN
    smLSN   mMemRedoLSN;

    // 미디어복구를 위한 CreateLSN 
    smLSN   mMemCreateLSN;

    // PROJ-2133 incremental backup
    smiDataFileDescSlotID  mDataFileDescSlotID;
    
    // PROJ-2133 incremental backup
    // incremental backup된 파일에만 존재하는 정보
    smriBISlot  mBackupInfo;
} smmChkptImageHdr;

/* 
   PRJ-1548 User Memory TableSpace 

   메모리 데이타파일의 메타헤더를 LogAnchor에 저장하기 위한 구조체
   smmDatabaseFile 객체에 멤버변수로 정의되어 있다. 
*/
typedef struct smmChkptImageAttr
{
    smiNodeAttrType  mAttrType;         // PRJ-1548 반드시 처음에 저장
    scSpaceID        mSpaceID;          // 메모리 테이블스페이스 ID
    UInt             mFileNum;          // 데이타파일의 No. 
    // 데이타파일의 CreateLSN
    smLSN            mMemCreateLSN;
    // Stable/Unstable 파일 생성 여부 
    idBool           mCreateDBFileOnDisk[ SMM_PINGPONG_COUNT ];
    //PROJ-2133 incremental backup
    smiDataFileDescSlotID mDataFileDescSlotID;

} smmChkptImageAttr;

// PRJ-1548 User Memory Tablespace 개념 도입 
// dirty page flush할 때 taget 데이타베이스의 pingpong 번호를 반환 
// 미디어복구연산에서는 현재 stable한 데이타베이스에 flush해야 하고,
// 운영중 체크포인트시에는 다음 stable한 데이타베이스에 flush해야 한다. 
typedef SInt (*smmGetFlushTargetDBNoFunc) ( smmTBSNode * aTBSNode );

/* BUG-32461 [sm-mem-resource] add getPageState functions to smmExpandChunk
 * module */
typedef enum smmPageState
{
    SMM_PAGE_STATE_NONE = 0,
    SMM_PAGE_STATE_META,
    SMM_PAGE_STATE_FLI,
    SMM_PAGE_STATE_ALLOC, // isDataPage = True
    SMM_PAGE_STATE_FREE   // isDataPage = True
} smmPageState;


typedef IDE_RC (*smmGetPersPagePtrFunc)( scSpaceID     aSpaceID,
                                         scPageID      aPageID,
                                         void       ** aPersPagePtr );

#endif // _O_SMM_DEF_H_
