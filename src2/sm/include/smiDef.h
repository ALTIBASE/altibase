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
 * $Id: smiDef.h 90899 2021-05-27 08:55:20Z jiwon.kim $
 **********************************************************************/

#ifndef _O_SMI_DEF_H_
# define _O_SMI_DEF_H_ 1

# include <idTypes.h>
# include <iduLatch.h>
# include <iduFixedTable.h>
# include <idv.h>
# include <idnCharSet.h>
# include <smuQueueMgr.h>

/* Class 선언                                        */
class smiTable;
class smiTrans;
class smiStatement;
class smiTableCursor;

#define SMI_MINIMUM_TABLE_CTL_SIZE (0)
#define SMI_MAXIMUM_TABLE_CTL_SIZE (120)
#define SMI_MINIMUM_INDEX_CTL_SIZE (0)
#define SMI_MAXIMUM_INDEX_CTL_SIZE (50) /* <BUG-48064>
                                           sdnbLKey::mTxInfo[2]의 createCTS/limitCTS index의 저장bit가 5->6bit로 증가함.
                                           따라서, CTS의 MAX갯수를 30->50개로 증가시킴.
                                           (MAX를 62개까지  증가시킬수 있으나, sdnbBTree::getMaxKeySize()에서 CTS MAX값을 고려해
                                           MAX KEY SIZE를 제한하기 때문에 기존과 동일한 크기가 되도록 50으로 맞추었음) */

/* TASK-4990 changing the method of collecting index statistics
 * 수동 통계정보 수집 기능 */
/*  MIN MAX Value 최대 길이 */
#define SMI_MAX_MINMAX_VALUE_SIZE (40) // 반드시 8byte align을 맞춰야 함.

/* 만약 전체 Page가 4인데 Sampling Percentage가 10%다, 그러면 어떤 Page도
 * Samplig 돼지 않을 수 있다. 따라서 첫 페이지는 거의 무조건 Sampling되도록
 * 초기값을 높게 잡는다. */
#define SMI_STAT_SAMPLING_INITVAL  (0.99f)

#define SMI_STAT_NULL              (ID_SLONG_MAX)
#define SMI_STAT_READTIME_NULL     (-1.0)

typedef struct smiSystemStat
{
    UInt     mCreateTV;                 /*TimeValue */
    SDouble  mSReadTime;                /*Single block read time */
    SDouble  mMReadTime;                /*Milti block read time */
    SLong    mDBFileMultiPageReadCount; /*DB_FILE_MULTIPAGE_READ_COUNT */

    SDouble  mHashTime;          /* 평균 hashFunc() 수행 시간 */
    SDouble  mCompareTime;       /* 평균 compare() 수행 시간 */
    SDouble  mStoreTime;         /* 평균 mem temp table store 수행 시간 */
} smiSystemStat;

typedef struct smiTableStat
{
    UInt     mCreateTV;         /*TimeValue */
    SLong    mNumRowChange;     /*통계정보 수집 이후 Row개수 변화량(I/D)*/
    SFloat   mSampleSize;       /*1~100     */
    SLong    mNumRow;           /*TableRowCount     */
    SLong    mNumPage;          /*Page개수          */
    SLong    mAverageRowLen;    /*Record 길이       */
    SDouble  mOneRowReadTime;   /*Row 하나를 읽는 평균 시간 */

    SLong    mMetaSpace;     /* PageHeader, ExtDir등 Meta 공간 */
    SLong    mUsedSpace;     /* 현재 사용중인 공간 */
    SLong    mAgableSpace;   /* 나중에 Aging가능한 공간 */
    SLong    mFreeSpace;     /* Data삽입이 가능한 빈 공간 */

    /* 버퍼풀에 올라온 테이블 페이지 숫자.
     * BUG-42095 : 사용 안 함*/
    SLong    mNumCachedPage;
} smiTableStat;

 /* 테이블 헤더와 인덱스 헤더에 삽입될 구조체
  * 실시간으로 테이블(또는 인덱스) 단위로 통계를 수집하기 때문에 헤더에 들어가며
  * 런타임에만 유의미한 정보이므로 구조체로 두고  동적할당을 통해 접근한다.
  * 변수가 하나뿐임에도 불구하고 이렇게 하는 이유는
  * FSB 및 향후 다른 프로젝트로 실시간 통계정보와 수동 정보를 분리해서 저장할 때
  * 유연하게 해결할 수 있도록 하기 위함이다.
  *
  * BUG-42095 : PROJ-2281 "buffer pool에 load된 page 통계 정보 제공" 기능을 제거한다.
  * 구조체 및 관련 변수는 삭제 하지 않고 통계 정보 수집및 업데이트 부분은 삭제한다.
  */
typedef struct smiCachedPageStat
{
    SLong  mNumCachedPage; /* 버퍼풀에 올라온 테이블 페이지 숫자 */
} smiCachedPageStat;


typedef struct smiIndexStat
{
    UInt   mCreateTV;      /*TimeValue */
    SFloat mSampleSize;    /*1~100     */
    SLong  mNumPage;       /*Page개수  */
    SLong  mAvgSlotCnt;    /*Leaf node당 평균 slot 개수 */
    SLong  mClusteringFactor;
    SLong  mIndexHeight;
    SLong  mNumDist;       /*Distinct Value */
    SLong  mKeyCount;      /*Key Count      */

    SLong  mMetaSpace;     /* PageHeader, ExtDir등 Meta 공간 */
    SLong  mUsedSpace;     /* 현재 사용중인 공간 */
    SLong  mAgableSpace;   /* 나중에 Aging가능한 공간 */
    SLong  mFreeSpace;     /* Data삽입이 가능한 빈 공간 */

    /* mNumCachedPage: 버퍼풀에 올라온 테이블 페이지 숫자.
     * BUG-42095 : 사용 안하도록 변경,  
     * BUG-47885 : 인덱스 아이디 + dummy로 변경 */ 
    UInt   mId;
    UInt   mDummy;

    /* 항상 MinMax는 맨 뒤에 있어야 함.
     * smiStatistics::setIndexStatWithoutMinMax 참조 */
    /* BUG-33548   [sm_transaction] The gather table statistics function
     * doesn't consider SigBus error */
    ULong  mMinValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Min값 */
    ULong  mMaxValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Max값 */
} smiIndexStat;

typedef struct smiColumnStat
{
    UInt   mCreateTV;         /*TimeValue */
    SFloat mSampleSize;       /*1~100     */
    SLong  mNumDist;          /*Distinct Value  */
    SLong  mNumNull;          /*NullValue Count */
    SLong  mAverageColumnLen; /*평균 길이       */
    /* BUG-33548   [sm_transaction] The gather table statistics function
     * doesn't consider SigBus error */
    ULong  mMinValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Min값 */
    ULong  mMaxValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Max값 */
} smiColumnStat;

// PROJ-2492 Dynamic sample selection
// 통계정보를 모두 저장 할 수 있는 자료구조
typedef struct smiAllStat
{
    smiTableStat    mTableStat;

    UInt            mColumnCount;
    UInt            mIndexCount;
    smiColumnStat * mColumnStat;
    smiIndexStat  * mIndexStat;
} smiAllStat;

/*
 * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
 */
typedef enum smiExtMgmtType
{
    SMI_EXTENT_MGMT_BITMAP_TYPE = 0,
    SMI_EXTENT_MGMT_NULL_TYPE     = 2,
    SMI_EXTENT_MGMT_MAX
} smiExtMgmtType;

/*
 * 세그먼트의 공간관리 방식
 */
typedef enum smiSegMgmtType
{
    SMI_SEGMENT_MGMT_FREELIST_TYPE     = 0,
    SMI_SEGMENT_MGMT_TREELIST_TYPE     = 1,
    SMI_SEGMENT_MGMT_CIRCULARLIST_TYPE = 2,
    SMI_SEGMENT_MGMT_NULL_TYPE         = 3,
    SMI_SEGMENT_MGMT_MAX
} smiSegMgmtType;

/* Segment 속성을 정의한다. */
typedef struct smiSegAttr
{
    /* PCTFREE reserves space in the data block for updates to existing rows.
       It represents a percentage of the block size. Before reaching PCTFREE,
       the free space is used both for insertion of new rows
       and by the growth of the data block header. */
    UShort   mPctFree;

    /* PCTUSED determines when a block is available for inserting new rows
       after it has become full(reached PCTFREE).
       Until the space used is less then PCTUSED,
       the block is not considered for insertion. */
    UShort   mPctUsed;

    /* 한 페이지당 초기 CTS 개수 */
    UShort   mInitTrans;
    /* 한 페이지당 최대 CTS 개수 */
    UShort   mMaxTrans;
} smiSegAttr;


/*
 * Segment의 STORAGE 속성을 정의한다.
 * 현재는 Treelist Managed Segment에서만 지원함.
 */
typedef struct smiSegStorageAttr
{
    /* Segment 생성시 Extent 개수 */
    UInt     mInitExtCnt;
    /* Segment 확장시 Extent 개수 */
    UInt     mNextExtCnt;
    /* Segment의 최소 Extent 개수 */
    UInt     mMinExtCnt;
    /* Segment의 최대 Extent 개수 */
    UInt     mMaxExtCnt;
} smiSegStorageAttr;

/* BUG-17033: 최상위 Statement가 아닌 Statment에 대해서도
 * Partial Rollback을 지원해야 합니다. */

/* Statment의 Depth가 가질수 있는 최대값 */
#define SMI_STATEMENT_DEPTH_MAX  (255)
/* Statment의 Depth가 이값을 Depth로 가져서는 안된다 */
#define SMI_STATEMENT_DEPTH_NULL (0)

/* FOR A4 : 테이블 스페이스의 타입을 정의함 */
typedef enum
{
    SMI_MEMORY_SYSTEM_DICTIONARY = 0,
    SMI_MEMORY_SYSTEM_DATA,
    SMI_MEMORY_USER_DATA,
    SMI_DISK_SYSTEM_DATA,
    SMI_DISK_USER_DATA,
    SMI_DISK_SYSTEM_TEMP,
    SMI_DISK_USER_TEMP,
    SMI_DISK_SYSTEM_UNDO,
    SMI_VOLATILE_USER_DATA,
    SMI_TABLESPACE_TYPE_MAX  //  for function array
} smiTableSpaceType;

// TBS가 속한 관리 영역을 나타냄
// XXX 이후 smiTableSpaceType의 상위 bitset으로
// 관리 영역을 표시하도록 변경되면 제거 되어야 함
typedef enum
{
    SMI_TBS_DISK = 0,
    SMI_TBS_MEMORY,
    SMI_TBS_VOLATILE,
    SMI_TBS_NONE
} smiTBSLocation;

typedef enum
{
    SMI_OFFLINE_NONE = 0,
    SMI_OFFLINE_NORMAL,
    SMI_OFFLINE_IMMEDIATE
} smiOfflineType;

/* --------------------------------------------------------------------
 * Description :
 * [주의] removeTableSpace와 removeDataFile은 each 모드로 수행하지 못하며,
 *  createTableSpace 및 createDataFiles는 each 모드로 수행가능하다.
 *
 * + 생성시의 touch 모드
 *    EACH_BYMODE, ALL_NOTOUCH
 *
 * +  제거시의 touch 모드
 *    ALL_TOUCH, ALL_NOTOUCH
 * ----------------------------------------------------------------- */
typedef enum
{
    SMI_ALL_TOUCH = 0, /* tablespace의 모든 datafile 노드의
                          datafile을 건드린다. */
    SMI_ALL_NOTOUCH,   /* tablespace의 모든 datafile 노드의
                          datafile을 건드리지 않는다. */
    SMI_EACH_BYMODE    // 각 datafile 노드의 create 모드에 따른다.

} smiTouchMode;

/* ------------------------------------------------
 * media recovery 및 restart시 재수행관리자 초기화 flag
 * ----------------------------------------------*/
typedef enum smiRecoverType
{
    SMI_RECOVER_RESTART = 0,
    SMI_RECOVER_COMPLETE,
    SMI_RECOVER_UNTILTIME,
    SMI_RECOVER_UNTILCANCEL,
    SMI_RECOVER_UNTILTAG
} smiRecoverType;

/* ------------------------------------------------
 * PROJ-2133 Incremental backup
 * media restore
 * ----------------------------------------------*/
typedef enum smiRestoreType
{
    SMI_RESTORE_COMPLETE = 0,
    SMI_RESTORE_UNTILTIME,
    SMI_RESTORE_UNTILTAG
} smiRestoreType;

/*-------------------------------------------------------------
 * PROJ-2133 Incremental backup
 * Description :
 * 수행된 backup level
 *
 * backup level과 backup type과의 관계
 * + level0 backup의 backup type:
 *     1) SMI_BACKUP_TYPE_FULL
 *          level0인경우 backup type은 SMI_BACKUP_TYPE_FULL만 가능하다.
 *
 * + level1 backup의 backup type:
 *     1) SMI_BACKUP_TYPE_DIFFERENTIAL
 *          과거에 해당 데이터파일이 level0으로 backup된적이 <있고>
 *          DIFFERENTIAL로 backup을 수행한 경우
 *
 *     2) SMI_BACKUP_TYPE_CUMULATIVE
 *          과거에 해당 데이터파일이 level0으로 backup된적이 <있고>
 *          CUMULATIVE로 backup을 수행한 경우
 *
 *     3) SMI_BACKUP_TYPE_DIFFERENTIAL & SMI_BACKUP_TYPE_FULL
 *          과거에 해당 데이터파일이 level0으로 backup된적이 <없고>
 *          DIFFERENTIAL로 backup을 수행한 경우
 *
 *     4) SMI_BACKUP_TYPE_CUMULATIVE & SMI_BACKUP_TYPE_FULL
 *          과거에 해당 데이터파일이 level0으로 backup된적이 <없고>
 *          CUMULATIVE로 backup을 수행한 경우
 *
 *  3번과 4번의 backup type은 level 0백업이 수행된 이후 create TBS나 create
 *  dataFile이 수행되어 새로운 데이터파일이 데이터베이스에 추가된경우이다.
 *  이 경우 level1 backup이 수행된경우 새롭게 추가된 데이터파일은 변화추적에
 *  대한 base가 없기 때문에 full backup이 수행되고 changeTracking이 시작되게
 *  된다.
 *-----------------------------------------------------------*/
typedef enum smiBackupLevel
{
    SMI_BACKUP_LEVEL_NONE,
    SMI_BACKUP_LEVEL_0,
    SMI_BACKUP_LEVEL_1
}smiBackuplevel;

#define SMI_BACKUP_TYPE_FULL            ((UShort)(0x01))
#define SMI_BACKUP_TYPE_DIFFERENTIAL    ((UShort)(0X02))
#define SMI_BACKUP_TYPE_CUMULATIVE      ((UShort)(0X04))
#define SMI_MAX_BACKUP_TAG_NAME_LEN     (128)

/* ------------------------------------------------
 * archive 로그 모드 타입
 * ------------------------------------------------ */
typedef enum
{
    SMI_LOG_NOARCHIVE = 0,
    SMI_LOG_ARCHIVE
} smiArchiveMode;

/* ------------------------------------------------
 * TASK-1842 lock type
 * ------------------------------------------------ */
typedef enum
{
    SMI_LOCK_ITEM_NONE = 0,
    SMI_LOCK_ITEM_TABLESPACE,
    SMI_LOCK_ITEM_TABLE
} smiLockItemType;

/*
  Tablespace의 Lock Mode

  Tablespace에 직접 Lock을 획득할때 사용한다.
 */
typedef enum smiTBSLockMode
{
    SMI_TBS_LOCK_NONE=0,
    SMI_TBS_LOCK_EXCLUSIVE,
    SMI_TBS_LOCK_SHARED
} smiTBSLockMode ;

/* ----------------------------------------------------------------------------
 *   smOID = ( PageID << SM_OFFSET_BIT_SIZE ) | Offset
 * --------------------------------------------------------------------------*/
typedef  vULong  smOID;
typedef  UInt    smTID;


// PRJ-1548 User Memory Tablespace
typedef  UShort     scSpaceID;
typedef  UInt       scPageID;
typedef  UShort     scOffset;
typedef  scOffset   scSlotNum;  /* PROJ-1705 */

typedef enum
{
    SMI_DUMP_TBL = 0,
    SMI_DUMP_IDX,
    SMI_DUMP_LOB,
    SMI_DUMP_TSS,
    SMI_DUMP_UDS
} smiDumpType;

#if defined(SMALL_FOOTPRINT)
#define  SC_MAX_SPACE_COUNT  (32)
#else
/* PROJ-2201
 * SpaceID의 마지막 둘은 WorkArea용으로 예약한다.
 * sdtDef.h의 SDT_SPACEID_WORKAREA, SDT_SPACEID_WAMAP 참조
 * BUG-48513 ushort max -2 가 들어가려면 array size 는 ushort max -1 */
#define  SC_MAX_SPACE_COUNT  (ID_USHORT_MAX - 1)
/* BUG-48513 3개 더 추가해서 scSpaceID(short)가 잘못 들어와도
 * 비정상 종료를 하지 않도록 한다.*/
#define  SC_MAX_SPACE_ARRAY_SIZE  (ID_USHORT_MAX + (UInt)1)
#endif
#define  SC_MAX_PAGE_COUNT   (ID_UINT_MAX)
#define  SC_MAX_OFFSET       (ID_USHORT_MAX)

#define  SC_NULL_SPACEID     ((scSpaceID)(0))
#define  SC_NULL_PID         ((scPageID) (0))
#define  SC_NULL_OFFSET      ((scOffset) (0))
#define  SC_NULL_SLOTNUM     ((scSlotNum)(0))

#define  SC_VIRTUAL_NULL_OFFSET ((scOffset) (0xFFFF))

// PROJ-1705
typedef struct scGRID
{
    scSpaceID  mSpaceID;
    scOffset   mOffset;
    scPageID   mPageID;
} scGRID;

/* PROJ-1789 */
#define SC_GRID_OFFSET_FLAG_MASK      (0x8000)
#define SC_GRID_OFFSET_FLAG_OFFSET    (0x0000)
#define SC_GRID_OFFSET_FLAG_SLOTNUM   (0x8000)

#define SC_GRID_SLOTNUM_MASK      (0x7FFF)

// PRJ-1548 User Memory Tablespace
#define SC_MAKE_GRID(grid, spaceid, pid, offset)     \
    {  (grid).mSpaceID = (spaceid);                  \
       (grid).mPageID = (pid);                       \
       (grid).mOffset = (offset); }

#define SC_MAKE_GRID_WITH_SLOTNUM(grid, spaceid, pid, slotnum)     \
    {  (grid).mSpaceID = (spaceid);                  \
       (grid).mPageID = (pid);                       \
       (grid).mOffset = ((slotnum) | SC_GRID_OFFSET_FLAG_SLOTNUM); }

#define SC_MAKE_NULL_GRID(grid)                       \
    {   (grid).mSpaceID = SC_NULL_SPACEID;            \
        (grid).mPageID  = SC_NULL_PID;                \
        (grid).mOffset  = SC_NULL_OFFSET; }

#define SC_MAKE_VIRTUAL_NULL_GRID_FOR_LOB(grid)      \
    {  (grid).mSpaceID = SC_NULL_SPACEID;            \
        (grid).mPageID  = SC_NULL_PID;               \
       (grid).mOffset  = SC_VIRTUAL_NULL_OFFSET; }

#define SC_COPY_GRID(src_grid, dst_grid)            \
    {   (dst_grid).mSpaceID = (src_grid).mSpaceID;  \
        (dst_grid).mPageID  = (src_grid).mPageID;   \
        (dst_grid).mOffset  = (src_grid).mOffset; }

#define SC_MAKE_SPACE(grid)    ( (grid).mSpaceID )
#define SC_MAKE_PID(grid)      ( (grid).mPageID )

inline static scSlotNum SC_MAKE_OFFSET(scGRID aGRID)
{
    IDE_DASSERT( ((aGRID.mOffset) & SC_GRID_OFFSET_FLAG_MASK) ==
                 (SC_GRID_OFFSET_FLAG_OFFSET) );

    return aGRID.mOffset;
}

inline static scSlotNum SC_MAKE_SLOTNUM(scGRID aGRID)
{
    IDE_DASSERT( ((aGRID.mOffset) & SC_GRID_OFFSET_FLAG_MASK) ==
                 (SC_GRID_OFFSET_FLAG_SLOTNUM) );

    return (aGRID.mOffset) & SC_GRID_SLOTNUM_MASK;
}

#define SC_GRID_IS_EQUAL(grid1, grid2)            \
    (((grid1).mSpaceID == (grid2).mSpaceID) &&    \
     ((grid1).mPageID  == (grid2).mPageID)  &&    \
     ((grid1).mOffset  == (grid2).mOffset))

#define SC_GRID_IS_NOT_EQUAL(grid1, grid2)     \
    (((grid1).mSpaceID != (grid2).mSpaceID) || \
     ((grid1).mPageID  != (grid2).mPageID)  || \
     ((grid1).mOffset  != (grid2).mOffset))

#define SC_GRID_IS_NULL(grid1)                  \
    (((grid1).mSpaceID == SC_NULL_SPACEID)  && \
     ((grid1).mPageID  == SC_NULL_PID)      && \
     ((grid1).mOffset  == SC_NULL_OFFSET))

#define SC_GRID_IS_NOT_NULL(grid1)            \
    (((grid1).mSpaceID != SC_NULL_SPACEID) || \
     ((grid1).mPageID  != SC_NULL_PID)     || \
     ((grid1).mOffset  != SC_NULL_OFFSET))

#define SC_GRID_IS_VIRTUAL_NULL_GRID_FOR_LOB(grid)       \
    ( ((grid).mSpaceID == SC_NULL_SPACEID)  &&           \
      ((grid).mPageID  == SC_NULL_PID    )  &&    \
      ((grid).mOffset  == SC_VIRTUAL_NULL_OFFSET ) )

#define SC_GRID_IS_WITH_SLOTNUM(grid)                        \
    ( ((grid).mOffset & SC_GRID_OFFSET_FLAG_MASK) ==         \
      SC_GRID_OFFSET_FLAG_SLOTNUM )

#define SC_GRID_IS_NOT_WITH_SLOTNUM(grid)                    \
    ( ((grid).mOffset & SC_GRID_OFFSET_FLAG_MASK) !=         \
      SC_GRID_OFFSET_FLAG_SLOTNUM )

// PROJ-1705
#define SMI_MAKE_GRID              SC_MAKE_GRID
#define SMI_MAKE_NULL_GRID         SC_MAKE_NULL_GRID
#define SMI_GRID_IS_NULL           SC_GRID_IS_NULL
#define SMI_MAKE_VIRTUAL_NULL_GRID SC_MAKE_VIRTUAL_NULL_GRID_FOR_LOB
#define SMI_GRID_IS_VIRTUAL_NULL   SC_GRID_IS_VIRTUAL_NULL_GRID_FOR_LOB

extern scGRID gScNullGRID;
#define SC_NULL_GRID gScNullGRID

#define SMI_MAKE_NULL_GRID   SC_MAKE_NULL_GRID

// PROJ-2264
#define SMI_CHANGE_GRID_TO_OID(GRID) SM_MAKE_OID( (GRID).mPageID, (GRID).mOffset )
#define SMI_NULL_OID SM_NULL_OID
#define SMI_INIT_SCN( SCN ) SM_INIT_SCN( SCN )

// PRJ-1671
typedef  UShort   smFileID;  /* SM 내부에서는 sdFileID로 구현되었음 */

// PROJ-1362.
typedef  ULong  smLobLocator;
typedef  UInt   smLobCursorID;

typedef  struct  smiSegIDInfo
{
    scSpaceID      mSpaceID;
    void          *mTable;
    smFileID       mFID;
    UInt           mFPID;
    smiDumpType    mType;
} smiSegIDInfo;


/* ----------------------------------------------------------------------------
 *   SCN: (S)YSTEM (C)OMMIT (N)UMBER
 * ---------------------------------------------------------------------------*/
#ifdef COMPILE_64BIT
typedef ULong  smSCN;
#else
typedef struct smSCN
{
    UInt mHigh;
    UInt mLow;
} smSCN;

#endif

typedef struct smLSN
{
    /* 로그가 위치한 File No */
    UInt  mFileNo;
    /* 로그가 위치한 File에서의 위치 */
    UInt  mOffset;
} smLSN;

/* ------------------------------------------------
 * Description : tablespace와 datafile의 속성 자료구조
 *
 * - Log Anchor에 테이블스페이스와 데이타파일의
 *   정보를 write하거나 read할 경우에 사용
 *
 *   # 테이블스페이스의 정보와 데이파일의 정보가
 *     Log Anchor에 저장된 그림
 *   __________________________
 *   |      ........          |
 *   |________________________| ___
 *   |@ sddTableSpaceAttr     |   |
 *   |____________________+2개|   |
 *   |+ sddDataFileAttr       |   |-- 테이블스페이스 ID 1
 *   |________________________|   |
 *   |+ sddDataFileAttr       |   |
 *   |________________________| __|
 *   |@                       |   |
 *   |____________________+3개|   |
 *   |+_______________________|   |-- 테이블스페이스 ID 2
 *   |+_______________________|   |
 *   |+_______________________| __|
 *               .
 *               .
 * ----------------------------------------------*/

#define SMI_MAX_TABLESPACE_NAME_LEN    (512)
#define SMI_MAX_DATAFILE_NAME_LEN      (512)
#define SMI_MAX_DWFILE_NAME_LEN        (512)
#define SMI_MAX_CHKPT_PATH_NAME_LEN    (512)
#define SMI_MAX_SBUFFER_FILE_NAME_LEN  (512)
/* --------------------------------------------------------------------
 * Description : create 모드 for reuse 구문
 * ----------------------------------------------------------------- */
typedef enum
{
    SMI_DATAFILE_REUSE = 0,  // datafile을 재사용한다.
    SMI_DATAFILE_CREATE,     // datafile을 생성한다.
    SMI_DATAFILE_CREATE_MODE_MAX // smiDataFileMode 가 가질수 있는 최대값
} smiDataFileMode;

/* ------------------------------------------------
 * PRJ-1149 Data File Node의 상태
 * ----------------------------------------------*/
typedef enum smiDataFileState
{
    SMI_FILE_NULL           =0x00000000,
    SMI_FILE_OFFLINE        =0x00000001,
    SMI_FILE_ONLINE         =0x00000002,
    SMI_FILE_BACKUP         =0x00000004,
    SMI_FILE_CREATING       =0x00000010,
    SMI_FILE_DROPPING       =0x00000020,
    SMI_FILE_RESIZING       =0x00000040,
    SMI_FILE_DROPPED        =0x00000080
} smiDataFileState;

#define SMI_FILE_STATE_IS_OFFLINE( aFlag )              \
    (((aFlag) & SMI_FILE_OFFLINE ) == SMI_FILE_OFFLINE )

#define SMI_FILE_STATE_IS_ONLINE( aFlag )               \
    (((aFlag) & SMI_FILE_ONLINE ) == SMI_FILE_ONLINE )

#define SMI_FILE_STATE_IS_BACKUP( aFlag )         \
    (((aFlag) & SMI_FILE_BACKUP ) == SMI_FILE_BACKUP )

#define SMI_FILE_STATE_IS_NOT_BACKUP( aFlag )         \
    (((aFlag) & SMI_FILE_BACKUP ) != SMI_FILE_BACKUP )

#define SMI_FILE_STATE_IS_CREATING( aFlag )             \
    (((aFlag) & SMI_FILE_CREATING ) == SMI_FILE_CREATING )

#define SMI_FILE_STATE_IS_NOT_CREATING( aFlag )             \
    (((aFlag) & SMI_FILE_CREATING ) != SMI_FILE_CREATING )

#define SMI_FILE_STATE_IS_DROPPING( aFlag )             \
    (((aFlag) & SMI_FILE_DROPPING ) == SMI_FILE_DROPPING )

#define SMI_FILE_STATE_IS_NOT_DROPPING( aFlag )             \
    (((aFlag) & SMI_FILE_DROPPING ) != SMI_FILE_DROPPING )

#define SMI_FILE_STATE_IS_RESIZING( aFlag )             \
    (((aFlag) & SMI_FILE_RESIZING ) == SMI_FILE_RESIZING )

#define SMI_FILE_STATE_IS_NOT_RESIZING( aFlag )             \
    (((aFlag) & SMI_FILE_RESIZING ) != SMI_FILE_RESIZING )

#define SMI_FILE_STATE_IS_DROPPED( aFlag )              \
    (((aFlag) & SMI_FILE_DROPPED ) == SMI_FILE_DROPPED )

#define SMI_FILE_STATE_IS_NOT_DROPPED( aFlag )              \
    (((aFlag) & SMI_FILE_DROPPED ) != SMI_FILE_DROPPED )

/*
   fix PR-15004

   1. STATE OF DATAFILE

   OFFLINE              0x01        = 1
   ONLINE               0x02        = 2
   ONLINE|BACKUP        0x02 | 0x04 = 6
   CREATING             0x10        = 16
   ONLINE|DROPPING      0x02 | 0x20 = 34
   ONLINE|RESIZEING     0x02 | 0x40 = 66
   DROPPED              0x40        = 128
*/

// Data File Node state가 가질 수 있는 최대값
#define SMI_DATAFILE_STATE_MAX      (0x0000000FF)

// OFFLINE | ONLINE
#define SMI_ONLINE_OFFLINE_MASK     (0x000000003)

/* ------------------------------------------------
 * PRJ-1149, table space의 상태
 * ----------------------------------------------*/
/* 이 Enumeration이 변경되면 smmTBSFixedTable.cpp의
   X$MEM_TABLESPACE_STATUS_DESC 구축 코드도 변경되어야 함 */
typedef enum smiTableSpaceState
{
    /* 백업할 수 없다. 대기 해야함. 생성, 삭제, OFFLINE 중일수 있다. */
    SMI_TBS_BLOCK_BACKUP          = 0x10000000,
    SMI_TBS_OFFLINE               = 0x00000001,
    SMI_TBS_ONLINE                = 0x00000002,
    SMI_TBS_BACKUP                = 0x00000004,
    // Tablespace 생성도중에 일시적으로 INCONSISTENT한 상태가 될 수 있다.
    SMI_TBS_INCONSISTENT          = 0x00000008,
    SMI_TBS_CREATING              = 0x00000010 | SMI_TBS_BLOCK_BACKUP ,
    SMI_TBS_DROPPING              = 0x00000020 | SMI_TBS_BLOCK_BACKUP ,
    // Drop Tablespace Transaction의 Commit도중 Pending Action수행중
    SMI_TBS_DROP_PENDING          = 0x00000040,
    SMI_TBS_DROPPED               = 0x00000080,
    // Online->Offline으로 진행중
    SMI_TBS_SWITCHING_TO_OFFLINE  = 0x00000100 | SMI_TBS_BLOCK_BACKUP ,
    // Offline->Online으로 진행중
    SMI_TBS_SWITCHING_TO_ONLINE   = 0x00000200 | SMI_TBS_BLOCK_BACKUP ,
    SMI_TBS_DISCARDED             = 0x00000400
} smiTableSpaceState;

# define SMI_TBS_IS_OFFLINE(state) (((state) & SMI_ONLINE_OFFLINE_MASK ) == SMI_TBS_OFFLINE )
# define SMI_TBS_IS_ONLINE(state) (((state) & SMI_ONLINE_OFFLINE_MASK ) == SMI_TBS_ONLINE )
# define SMI_TBS_IS_BACKUP(state) (((state) & SMI_TBS_BACKUP ) == SMI_TBS_BACKUP )
# define SMI_TBS_IS_NOT_BACKUP(state) (((state) & SMI_TBS_BACKUP ) != SMI_TBS_BACKUP )
# define SMI_TBS_IS_CREATING(state) (((state) & SMI_TBS_CREATING ) == SMI_TBS_CREATING )
# define SMI_TBS_IS_DROPPING(state) (((state) & SMI_TBS_DROPPING ) == SMI_TBS_DROPPING )
# define SMI_TBS_IS_DROP_PENDING(state) (((state) & SMI_TBS_DROP_PENDING) == SMI_TBS_DROP_PENDING )
# define SMI_TBS_IS_DROPPED(state) (((state) & SMI_TBS_DROPPED) == SMI_TBS_DROPPED )
# define SMI_TBS_IS_NOT_DROPPED(state) (((state) & SMI_TBS_DROPPED) != SMI_TBS_DROPPED )
# define SMI_TBS_IS_DISCARDED(state) (((state) & SMI_TBS_DISCARDED) == SMI_TBS_DISCARDED )
# define SMI_TBS_IS_NOT_DISCARDED(state) (((state) & SMI_TBS_DISCARDED) != SMI_TBS_DISCARDED )


/*ONLINE, BACKUP 이외의 Flag가 있으면 생성중, offline, dropping 등등이다.*/
# define SMI_TBS_IS_INCOMPLETE(state) ((state) & ~(SMI_TBS_ONLINE|SMI_TBS_BACKUP)) 
# define SMI_TBS_IS_COMPLETE(state)   (!SMI_TBS_IS_INCOMPLETE(state))


/* 테이블스페이스의 속성 플래그

   기본값으로 사용할 값에 대해 Mask안의 모든 Bit를 0으로 설정한다.

 */
// V$TABLESPACES에서 사용자에게 보이는 Tablespace의 상태들
// XXX 이후 최적화 해서 모두 보이도록 수정해야 한다.
// OFFLINE(0x001) | ONLINE(0x002) | DISCARD(0x400) | DROPPED(0x080)
#define SMI_TBS_STATE_USER_MASK         (0x000000483)
// 테이블스페이스안의 데이터에 대한 Log를 Compress할지의 여부
#define SMI_TBS_ATTR_LOG_COMPRESS_MASK  ( 0x00000001 )
// 기본적으로 "로그 압축"을 사용하도록 한다. (0으로 설정)
#define SMI_TBS_ATTR_LOG_COMPRESS_TRUE  ( 0x00000000 )
#define SMI_TBS_ATTR_LOG_COMPRESS_FALSE ( 0x00000001 )

/* --------------------------------------------------------------------
 * Description : tablespace 정보
 *
 * ID는 입력값으로는 어떤 의미를 지니지 않고 오직 출력값으로만 사용된다.
 * tablespace create 시 부여된 아이디가 리턴된다.
 * ----------------------------------------------------------------- */
typedef struct smiDiskTableSpaceAttr
{
    smFileID              mNewFileID;      /* 다음에 생성될 datafile ID */
    UInt                  mExtPageCount;   /* extent의 크기(page count) */
    smiExtMgmtType        mExtMgmtType;    /* Extent 관리방식 */
    smiSegMgmtType        mSegMgmtType;    /* Segment 관리방식 */
} smiDiskTableSpaceAttr;

typedef struct smiMemTableSpaceAttr
{
    key_t              mShmKey;
    SInt               mCurrentDB;
    scPageID           mMaxPageCount;
    scPageID           mNextPageCount;
    scPageID           mInitPageCount;
    idBool             mIsAutoExtend;
    UInt               mChkptPathCount;
    scPageID           mSplitFilePageCount;
} smiMemTableSpaceAttr;

typedef struct smiVolTableSpaceAttr
{
    scPageID           mMaxPageCount;
    scPageID           mNextPageCount;
    scPageID           mInitPageCount;
    idBool             mIsAutoExtend;
} smiVolTableSpaceAttr;

/*
  PRJ-1548 User Memory Tablespace

  노드 속성 타입 정의

  Loganchor에 가변길이영역을 로드할때, smiNodeAttrHead를
  먼저 읽어서 노드속성을 알아낸 후, 다음 노드가 저장된 오프셋을
  알아낸다.
*/
typedef enum smiNodeAttrType
{
    SMI_TBS_ATTR        = 1,  // 테이블스페이스 노드속성
    SMI_CHKPTPATH_ATTR  = 2,  // CHECKPOINT PATH 노드속성
    SMI_DBF_ATTR        = 3,  // 데이타파일 노드속성
    SMI_CHKPTIMG_ATTR   = 4,  // CHECKPOINT IMAGE 노드속성
    SMI_SBUFFER_ATTR    = 5   // Second Buffer File 노드 속성
} smiNodeAttrType;

/* --------------------------------------------------------------------
 * Description : tablespace 정보
 *
 * ID는 입력값으로는 어떤 의미를 지니지 않고 오직 출력값으로만 사용된다.
 * tablespace create 시 부여된 아이디가 리턴된다.
 * ----------------------------------------------------------------- */
typedef struct smiTableSpaceAttr
{
    smiNodeAttrType       mAttrType; // PRJ-1548 반드시 처음에 저장
    scSpaceID             mID;    // 테이블스페이스 아이디
    // NULL로 끝나는 문자열
    SChar                 mName[SMI_MAX_TABLESPACE_NAME_LEN + 1];
    UInt                  mNameLength;

    // 테이블 스페이스 속성 FLAG
    UInt                  mAttrFlag;

    // 테이블스페이스 상태(Creating, Droppping, Online, Offline...)
    // 이 상태는 Log Anchor에 저장되는 상태이다.
    // sctTableSpaceNode.mState와 혼동하여 쓰는 것을 막기 위해
    // 변수이름에 LA(Log Anchor)에 저장되는 State임을 명시하였다.
    UInt                  mTBSStateOnLA;
    // 테이블스페이스 종류
    // User || System, Disk || Memory, Data || Temp || Undo
    smiTableSpaceType     mType;
    // 오프라인 타입(None, Normal, Immediate)
    smiDiskTableSpaceAttr mDiskAttr;
    smiMemTableSpaceAttr  mMemAttr;
    smiVolTableSpaceAttr  mVolAttr;
} smiTableSpaceAttr;

/* --------------------------------------------------------------------
 * Description : data file의 정보
 *
 * 각 size는 tbs 확장 크기의 배수로 align 된다.
 * tbs 확장 크기는 tablespace가 한번에 할당하는 extent의 개수 *
 * extent의 크기이다.
 * next size는 tbs확장 크기보다는 커야 한다.
 * createTableSpace시에 init size와 curr size의 값은 반드시
 * 같아야 한다. extend가 발생하면 이 두 값은 달라진다.
 *
 * ----------------------------------------------------------------- */
//incremental backup
typedef struct smiDataFileDescSlotID
{
    /*datafile descriptor slot이 위치한 block ID*/
    UInt             mBlockID;

    /*datafile descriptor block내에서의 slot index*/
    UInt             mSlotIdx;
}smiDataFileDescSlotID;

typedef struct smiDataFileAttr
{
    smiNodeAttrType       mAttrType;  // PRJ-1548 반드시 처음에 저장
    scSpaceID             mSpaceID;
    SChar                 mName[SMI_MAX_DATAFILE_NAME_LEN];
    UInt                  mNameLength;
    smFileID              mID;

    // unlimited일 경우 0이 set되며
    // system max size로 set된다.
    ULong                 mMaxSize;      /* datafile의 최대 page 개수 */
    ULong                 mNextSize;     /* datafile의 확장 page 개수 */
    ULong                 mCurrSize;     /* datafile의 총 page 개수 */
    ULong                 mInitSize;     /* datafile의 초기 page 개수 */
    idBool                mIsAutoExtend; /* 데이타파일의 자동확장 여부 */
    UInt                  mState;        /* 데이타 파일의상태       */
    smiDataFileMode       mCreateMode;   /* datafile 생성? reuse ? */
    smLSN                 mCreateLSN;    /* 데이타파일 생성 LSN */
    //PROJ-2133 incremental backup
    smiDataFileDescSlotID  mDataFileDescSlotID;
} smiDataFileAttr;

/* --------------------------------------------------------------------
 * Description : checkpoint path 정보
 * ----------------------------------------------------------------- */
typedef struct smiChkptPathAttr
{
    smiNodeAttrType       mAttrType;
    scSpaceID             mSpaceID; // PRJ-1548 반드시 처음에 저장
    // NULL 로 끝나는 문자열
    SChar                 mChkptPath[SMI_MAX_CHKPT_PATH_NAME_LEN+1];
} smiChkptPathAttr;


/* 여러 개의 smiChkptPathAttr을 단방향 Linked List로 연결한 자료구조 */
typedef struct smiChkptPathAttrList
{
    smiChkptPathAttr       mCPathAttr;
    smiChkptPathAttrList * mNext;
} smiChkptPathAttrList ;


/**********************************************************************
 * Log anchor file에 저장되는 Secondaty Buffer file 의 정보
 **********************************************************************/
typedef struct smiSBufferFileAttr
{
    smiNodeAttrType       mAttrType;     // PRJ-1548 반드시 처음에 저장.
    SChar                 mName[SMI_MAX_SBUFFER_FILE_NAME_LEN];  // File이 저장된 위치
    UInt                  mNameLength;   // 이름의 길이
    ULong                 mPageCnt;      // Secondaty Buffer file의 최대 page 개수
    smiDataFileState      mState;        // File의 상태 (online, offline)
    smLSN                 mCreateLSN;    // 생성 LSN
} smiSBufferFileAttr;


/* ------------------------------------------------
 * Description : Reserved ID of tablespace
 *
 * 아래의 기본적으로 생성되는 tablespace를 위해 0부터 3번 ID까지
 * 예약하며, 그 이후 ID는 user tablespace에 할당 가능하다.
 * ----------------------------------------------*/
#define SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC      ((scSpaceID)0)
#define SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA     ((scSpaceID)1)
#define SMI_ID_TABLESPACE_SYSTEM_DISK_DATA       ((scSpaceID)2)
#define SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO       ((scSpaceID)3)
#define SMI_ID_TABLESPACE_SYSTEM_DISK_TEMP       ((scSpaceID)4)

#define SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DIC    "SYS_TBS_MEM_DIC"
#define SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DATA   "SYS_TBS_MEM_DATA"
#define SMI_TABLESPACE_NAME_SYSTEM_DISK_DATA     "SYS_TBS_DISK_DATA"
#define SMI_TABLESPACE_NAME_SYSTEM_DISK_UNDO     "SYS_TBS_DISK_UNDO"
#define SMI_TABLESPACE_NAME_SYSTEM_DISK_TEMP     "SYS_TBS_DISK_TEMP"

/* System Tablespace들의 Attribute Flag */
// 안정성을 위해 Dictionary Tablespace에 대한 Log를 압축하지 않는다.
#define SMI_TABLESPACE_ATTRFLAG_SYSTEM_MEMORY_DIC \
        ( SMI_TBS_ATTR_LOG_COMPRESS_FALSE )

#define SMI_TABLESPACE_ATTRFLAG_SYSTEM_MEMORY_DATA \
        ( SMI_TBS_ATTR_LOG_COMPRESS_TRUE )

#define SMI_TABLESPACE_ATTRFLAG_SYSTEM_DISK_DATA \
        ( SMI_TBS_ATTR_LOG_COMPRESS_TRUE )

#define SMI_TABLESPACE_ATTRFLAG_SYSTEM_DISK_UNDO \
        ( SMI_TBS_ATTR_LOG_COMPRESS_TRUE )

#define SMI_TABLESPACE_ATTRFLAG_SYSTEM_DISK_TEMP \
        ( SMI_TBS_ATTR_LOG_COMPRESS_TRUE )


/* ----------------------------------------------------------------------------
 *   for index
 * --------------------------------------------------------------------------*/
#if defined(SMALL_FOOTPRINT)
#define SMI_MAX_IDX_COLUMNS       (8) // -> from smn (20040715 for QP)
#else
#define SMI_MAX_IDX_COLUMNS       (32) // -> from smn (20040715 for QP)
#endif

/* ----------------------------------------------------------------------------
 *   Global Transaction ID
 * --------------------------------------------------------------------------*/

// For Global Transaction Commit State
typedef enum
{
    SMX_XA_START,
    SMX_XA_PREPARED,
    SMX_XA_COMPLETE
} smiCommitState;

typedef enum
{
    SMI_OBJECT_NONE = 0,
    SMI_OBJECT_PROCEDURE,
    SMI_OBJECT_PACKAGE,
    SMI_OBJECT_VIEW,
    SMI_OBJECT_DATABASE_LINK,
    SMI_OBJECT_TRIGGER
} smiObjectType;

typedef enum
{
    SMI_SELECT_CURSOR = 0,
    SMI_INSERT_CURSOR,
    SMI_UPDATE_CURSOR,
    SMI_DELETE_CURSOR,
    SMI_LOCKROW_CURSOR,  // for Referential constraint check
    // PROJ-1502 PARTITIONED DISK TABLE
    SMI_COMPOSITE_CURSOR
} smiCursorType;


/* ----------------------------------------------------------------------------
 *   PROJ-1362 Large Record & Internal LOB support
 *   LobLocator는 다음과 같이 정의된다.
 *   [ Shard flag | TransactionID | LobCursorID ] =  smLobLocator
 *      1bit      |  31bit        |  32bit        =  64bit.
 * --------------------------------------------------------------------------*/
#define  SMI_LOB_LOCATOR_SHARD_MASK  (0x80000000)
#define  SMI_LOB_SHARD_FLAG_BIT_SIZE (ID_ULONG(1))
#define  SMI_LOB_SHARD_FLAG_MASK     ((ID_ULONG(1) << SMI_LOB_SHARD_FLAG_BIT_SIZE) - ID_ULONG(1))
#define  SMI_LOB_TRANSID_BIT_SIZE    (ID_ULONG(31))
#define  SMI_LOB_TRANSID_MASK        ((ID_ULONG(1) << SMI_LOB_TRANSID_BIT_SIZE) - ID_ULONG(1))
#define  SMI_LOB_CURSORID_BIT_SIZE   (ID_ULONG(32))
#define  SMI_LOB_CURSORID_MASK       ((ID_ULONG(1) << SMI_LOB_CURSORID_BIT_SIZE) - ID_ULONG(1))

#define  SMI_MAKE_LOB_SHARD_FLAG(locator)                                      \
    ( ((locator) >> (SMI_LOB_CURSORID_BIT_SIZE + SMI_LOB_TRANSID_BIT_SIZE) ) & \
      SMI_LOB_SHARD_FLAG_MASK )

#define  SMI_MAKE_LOB_TRANSID(locator)                                         \
    ( ((locator) >> SMI_LOB_CURSORID_BIT_SIZE) & SMI_LOB_TRANSID_MASK )

#define  SMI_MAKE_LOB_CURSORID(locator) ( (locator) & SMI_LOB_CURSORID_MASK)
#define  SMI_MAKE_LOB_LOCATOR(tid,cursorid) ( ((smLobLocator) (tid) << SMI_LOB_CURSORID_BIT_SIZE) | (cursorid))

#define  SMI_MAKE_SHARD_LOB_LOCATOR(tid,cursorid)                              \
    ( ((smLobLocator) ((tid) | SMI_LOB_LOCATOR_SHARD_MASK)                     \
                       << SMI_LOB_CURSORID_BIT_SIZE) | (cursorid) )

#define  SMI_IS_SHARD_LOB_LOCATOR(locator)                                     \
    (idBool)( ( SMI_MAKE_LOB_SHARD_FLAG((locator)) == 1 ) &&                   \
              ( SMI_IS_NULL_LOCATOR((locator)) == 0 ) )

#define  SMI_NULL_LOCATOR  SMI_MAKE_LOB_LOCATOR(SM_NULL_TID, 0);

#define  SMI_IS_NULL_LOCATOR(locator)                                          \
    ( (SMI_MAKE_LOB_TRANSID((locator)) == SM_NULL_TID) &&                      \
      (((locator) & SMI_LOB_CURSORID_MASK) == 0) )

#define SMI_GET_SESSION_STATISTICS( aTrans ) (((smxTrans*)aTrans)->mStatistics)

typedef enum
{
    SMI_LOB_READ_MODE = 0,
    SMI_LOB_READ_WRITE_MODE,
    SMI_LOB_TABLE_CURSOR_MODE,
    SMI_LOB_READ_LAST_VERSION_MODE  /* not used */
} smiLobCursorMode;

typedef enum
{
    SMI_LOB_COLUMN_TYPE_BLOB = 0,
    SMI_LOB_COLUMN_TYPE_CLOB
} smiLobColumnType;

# define SMI_MAX_NAME_LEN                  (SM_MAX_NAME_LEN)
# define SMI_MAX_PARTKEY_COND_VALUE_LEN    (SM_MAX_PARTKEY_COND_VALUE_LEN)

/* smiColumn.id                                      */
# define SMI_COLUMN_ID_MASK                (0x000003FF)
# define SMI_COLUMN_ID_MAXIMUM                   (1024)
# define SMI_COLUMN_ID_INCREMENT           (0x00000400)

/* smiColumn.flag                                    */
# define SMI_COLUMN_TYPE_MASK              (0x000F0003)
# define SMI_COLUMN_TYPE_FIXED             (0x00000000)
# define SMI_COLUMN_TYPE_VARIABLE          (0x00000001)
# define SMI_COLUMN_TYPE_LOB               (0x00000002)
/* BUG-43840
 * PROJ-2419 UnitedVar 적용되기 이전의 Variable 타입인 LargeVar를
 * 사용 하기 위해 Column Type에 LargeVar를 추가하였다.
 * Geometry Type 은 항상 LargeVar 형태로 저장된다. */
# define SMI_COLUMN_TYPE_VARIABLE_LARGE      (0x00000003)

// PROJ-2362 memory temp 저장 효율성 개선
// memory temp에만 사용되는 column type
# define SMI_COLUMN_TYPE_TEMP_1B           (0x00010000)
# define SMI_COLUMN_TYPE_TEMP_2B           (0x00020000)
# define SMI_COLUMN_TYPE_TEMP_4B           (0x00030000)

/* smiColumn.flag                                    */
// Variable Column의 IN/OUT MODE
# define SMI_COLUMN_MODE_MASK              (0x00000004)
# define SMI_COLUMN_MODE_IN                (0x00000000)
# define SMI_COLUMN_MODE_OUT               (0x00000004)

/* smiColumn.flag - Use for CreateIndex              */
# define SMI_COLUMN_ORDER_MASK             (0x00000008)
# define SMI_COLUMN_ORDER_ASCENDING        (0x00000000)
# define SMI_COLUMN_ORDER_DESCENDING       (0x00000008)

/* smiColumn.flag                                     */
// Column의 저장 매체에 대한 정보
# define SMI_COLUMN_STORAGE_MASK            (0x00000010)
# define SMI_COLUMN_STORAGE_MEMORY          (0x00000000)
# define SMI_COLUMN_STORAGE_DISK            (0x00000010)

// PROJ-1705
/* smiColumn.flag - table col or index key            */
# define SMI_COLUMN_USAGE_MASK              (0x00000020)
# define SMI_COLUMN_USAGE_TABLE             (0x00000000)
# define SMI_COLUMN_USAGE_INDEX             (0x00000020)

/* smiColumn.flag-for PROJ-1362 LOB logging or nologging*/
# define SMI_COLUMN_LOGGING_MASK            (0x00000040)
# define SMI_COLUMN_LOGGING                 (0x00000000)
# define SMI_COLUMN_NOLOGGING               (0x00000040)

/* smiColumn.flag-for PROJ-1362 LOB buffer or no buffer*/
# define SMI_COLUMN_USE_BUFFER_MASK         (0x00000080)
# define SMI_COLUMN_USE_BUFFER              (0x00000000)
# define SMI_COLUMN_USE_NOBUFFER            (0x00000080)

/* smiColumn.flag */
/* SM이 컬럼을 여러 row piece에
 * 나누어 저장해도 되는지 여부를 나타낸다. */
# define SMI_COLUMN_DATA_STORE_DIVISIBLE_MASK  (0x00000100)
# define SMI_COLUMN_DATA_STORE_DIVISIBLE_FALSE (0x00000000)
# define SMI_COLUMN_DATA_STORE_DIVISIBLE_TRUE  (0x00000100)

// PROJ-1872 Disk index 저장 구조 최적화
/* smiColumn.flag */
/* Length가 알려진 Column인지, 다루 붙는지를 나타낸다 */
# define SMI_COLUMN_LENGTH_TYPE_MASK           (0x00000200)
# define SMI_COLUMN_LENGTH_TYPE_KNOWN          (0x00000000)
# define SMI_COLUMN_LENGTH_TYPE_UNKNOWN        (0x00000200)

/* findCompare 함수에서 필요한 flag 정보 */
# define SMI_COLUMN_COMPARE_TYPE_MASK          (0x00000C00)
# define SMI_COLUMN_COMPARE_NORMAL             (0x00000000)
# define SMI_COLUMN_COMPARE_KEY_AND_VROW       (0x00000400)
# define SMI_COLUMN_COMPARE_KEY_AND_KEY        (0x00000800)
# define SMI_COLUMN_COMPARE_DIRECT_KEY         (0x00000C00) /* PROJ-2433 */

/* smiColumn.flag */
/* PROJ-1597 Temp record size 제약제거
   length-unknown type이지만 최대 column precision만큼
   공간을 확보해놔야 하는 컬럼임을 표시
   이 속성은 temp table의 aggregation column처럼
   update가 필요한 컬럼들에 대해 필요하다.
   SMI_COLUMN_LENGTH_TYPE_UNKNOWN 이면서 update되는 컬럼이면
   이 속성을 켜야 한다. (temp table에만 적용됨) */
# define SMI_COLUMN_ALLOC_FIXED_SIZE_MASK      (0x00001000)
# define SMI_COLUMN_ALLOC_FIXED_SIZE_FALSE     (0x00000000)
# define SMI_COLUMN_ALLOC_FIXED_SIZE_TRUE      (0x00001000)

// PROJ-2264
// compression column 여부를 판단하데 필요한 flag 정보
# define SMI_COLUMN_COMPRESSION_MASK           (0x00002000)
# define SMI_COLUMN_COMPRESSION_FALSE          (0x00000000)
# define SMI_COLUMN_COMPRESSION_TRUE           (0x00002000)

// PROJ-2429 Dictionary based data compress for on-disk DB
// Dictionary Table의 column이 어느 타입의 table column의
// 실제 데이터를 가지고있는지를 나타낸다.
# define SMI_COLUMN_COMPRESSION_TARGET_MASK    (0x00008000)
# define SMI_COLUMN_COMPRESSION_TARGET_MEMORY  (0x00000000)
# define SMI_COLUMN_COMPRESSION_TARGET_DISK    (0x00008000)

// PROJ-2362 memory temp 저장 효율성 개선
// 주의 SMI_COLUMN_TYPE_MASK에서 (0x000F0000)를 사용하고 있다.

/* PROJ-2435 ORDER BY NULLS OPTION
 * Sort Temp 에서만 사용.
 */
# define SMI_COLUMN_NULLS_ORDER_MASK           (0x00300000)
# define SMI_COLUMN_NULLS_ORDER_NONE           (0x00000000)
# define SMI_COLUMN_NULLS_ORDER_FIRST          (0x00100000)
# define SMI_COLUMN_NULLS_ORDER_LAST           (0x00200000)

/* For table flag */
/* aFlag VON smiTable::createTable                   */
/* aFlag VON smiTable::modifyTableInfo               */
# define SMI_TABLE_FLAG_UNCHANGE           (ID_UINT_MAX)
# define SMI_TABLE_REPLICATION_MASK        (0x00000001)
# define SMI_TABLE_REPLICATION_DISABLE     (0x00000000)
# define SMI_TABLE_REPLICATION_ENABLE      (0x00000001)

# define SMI_TABLE_REPLICATION_TRANS_WAIT_MASK        (0x00000002)
# define SMI_TABLE_REPLICATION_TRANS_WAIT_DISABLE     (0x00000000)
# define SMI_TABLE_REPLICATION_TRANS_WAIT_ENABLE      (0x00000002)

# define SMI_TABLE_LOCK_ESCALATION_MASK    (0x00000010)
# define SMI_TABLE_LOCK_ESCALATION_DISABLE (0x00000000)
# define SMI_TABLE_LOCK_ESCALATION_ENABLE  (0x00000010)

#if 0 // not used
typedef enum smiDMLType
{
    SMI_DML_INSERT,
    SMI_DML_UPDATE,
    SMI_DML_DELETE
} smiDMLType;
#endif

# define SMI_TABLE_TYPE_COUNT              (7)
# define SMI_TABLE_TYPE_TO_ID( aType )     (((aType) & SMI_TABLE_TYPE_MASK) >> 12)
# define SMI_TABLE_TYPE_MASK               (0x0000F000)
# define SMI_TABLE_META                    (0x00000000) // Catalog Tables
# define SMI_TABLE_TEMP_LEGACY             (0x00001000) // Temporary Tables
# define SMI_TABLE_MEMORY                  (0x00002000) // Memory Tables
# define SMI_TABLE_DISK                    (0x00003000) // Disk Tables
# define SMI_TABLE_FIXED                   (0x00004000) // Fixed Tables
# define SMI_TABLE_VOLATILE                (0x00005000) // Volatile Tables
# define SMI_TABLE_REMOTE                  (0x00006000) // Remote Query

/* PROJ-2083 */
/* Dual Table 여부 */
# define SMI_TABLE_DUAL_MASK               (0x00000100)
# define SMI_TABLE_DUAL_TRUE               (0x00000000)
# define SMI_TABLE_DUAL_FALSE              (0x00000100)

/* PROJ-1407 Temporary Table */
#define SMI_TABLE_PRIVATE_VOLATILE_MASK    (0x00000200)
#define SMI_TABLE_PRIVATE_VOLATILE_TRUE    (0x00000200)
#define SMI_TABLE_PRIVATE_VOLATILE_FALSE   (0x00000000)

# define SMI_COLUMN_TYPE_IS_TEMP(flag) (((((flag) & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_TEMP_1B) || \
                                         (((flag) & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_TEMP_2B) || \
                                         (((flag) & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_TEMP_4B)) \
                                         ? ID_TRUE : ID_FALSE)

# define SMI_GET_TABLE_TYPE(a)         (((a)->mFlag) & SMI_TABLE_TYPE_MASK)

# define SMI_TABLE_TYPE_IS_META(a)     (((((a)->mFlag) & SMI_TABLE_TYPE_MASK) \
                                       == SMI_TABLE_META) ? ID_TRUE : ID_FALSE)
/* not used
# define SMI_TABLE_TYPE_IS_TEMP(a)     (((((a)->mFlag) & SMI_TABLE_TYPE_MASK) \
                                       == SMI_TABLE_TEMP) ? ID_TRUE : ID_FALSE)
*/
# define SMI_TABLE_TYPE_IS_MEMORY(a)   (((((a)->mFlag) & SMI_TABLE_TYPE_MASK) \
                                       == SMI_TABLE_MEMORY)  ? ID_TRUE : ID_FALSE)

# define SMI_TABLE_TYPE_IS_DISK(a)     (((((a)->mFlag) & SMI_TABLE_TYPE_MASK) \
                                       == SMI_TABLE_DISK) ? ID_TRUE : ID_FALSE)
/* not used
# define SMI_TABLE_TYPE_IS_FIXED(a)    (((((a)->mFlag) & SMI_TABLE_TYPE_MASK) \
                                       == SMI_TABLE_FIXED) ? ID_TRUE : ID_FALSE)
*/
# define SMI_TABLE_TYPE_IS_VOLATILE(a) (((((a)->mFlag) & SMI_TABLE_TYPE_MASK) \
                                       == SMI_TABLE_VOLATILE) ? ID_TRUE : ID_FALSE)
/* not used
# define SMI_TABLE_TYPE_IS_REMOTE(a)   (((((a)->mFlag) & SMI_TABLE_TYPE_MASK) \
                                       == SMI_TABLE_REMOTE) ? ID_TRUE : ID_FALSE)
*/
/* PROJ-1665 */
/* Table상태가 Consistent 한지 여부 정보 */
/*
# define SMI_TABLE_CONSISTENT_MASK         (0x00010000)
# define SMI_TABLE_CONSISTENT              (0x00000000)
# define SMI_TABLE_INCONSISTENT            (0x00010000)
 * PROJ-2162 Suspended ( => Flag대신 IsConsistent로 독립적으로 사용됨 )
 */

/* PROJ-1665 */
/* Table Logging 여부 ( Direct-Path INSERT 시에만 유효 ) */
# define SMI_TABLE_LOGGING_MASK            (0x00020000)
# define SMI_TABLE_LOGGING                 (0x00000000)
# define SMI_TABLE_NOLOGGING               (0x00020000)

/*  Table Type Flag calculation shift right 12 bit   */
# define SMI_SHIFT_TO_TABLE_TYPE           (12)

/* TASK-2398 Log Compress */
# define SMI_TABLE_LOG_COMPRESS_MASK       (0x00040000)
// Mask에 해당하는 모든 Bit가 0인 플래그를 Default로 사용한다.
// Default => Table의 로그를 압축 ( 값이 0 )
# define SMI_TABLE_LOG_COMPRESS_TRUE       (0x00000000)
# define SMI_TABLE_LOG_COMPRESS_FALSE      (0x00040000)


/* TASK-2401 Disk/Memory 테이블의 Log분리
   Meta Table에 대해 Log Flush여부 결정

   LFG=2로 설정된 경우 Hybrid Transaction이 Commit할 경우
   Dependent한 LFG에 대해 Flush를 하여야 한다.

   이때 Disk Table의 Validation과정에서 Meta Table을 접근하게 되어
   항상 Hybrid Transaction으로 분류되는 문제를 해결하기 위한 Flag이다.

   => 문제점 :
      Disk Table에 대한 DML시 Validation과정에서
      Table의 Meta Table을 참조하게 된다
      그런데 Meta Table은 Memory Table이기 때문에,
      Memory Table(Meta)을 읽고 Disk Table에 DML을 하게 되면
      Hybrid Transaction으로 분류가 되어
      Memory 로그가 항상 Flush되는 문제가 발생한다.

      => 해결책 :
        Meta Table을 읽는 경우 Hybrid Transaction으로 분류하지 않는 대신
        Meta Table의 변경시에 Memory Log를 Flush하도록 한다.

   => 또다른 문제점 :
      Meta Table중 Replication의 XSN과 같이 DML과 관계없이 빈번하게
      수정되는 Meta Table의 경우 매번 Log를 Flush할 경우 성능저하발생

      => 해결책 :
         Meta Table마다 변경이 되었을 때 Log를 Flush할지 여부를
         Flag로 둔다.
         이 Flag가 켜져 있는 경우에만 Meta Table변경한 Transaction의
         Commit시에 로그를 Flush하도록 한다.
 */
# define SMI_TABLE_META_LOG_FLUSH_MASK     (0x00080000)
// 기본값 ( Flush 실시 )을 0으로 설정
# define SMI_TABLE_META_LOG_FLUSH_TRUE     (0x00000000)
# define SMI_TABLE_META_LOG_FLUSH_FALSE    (0x00080000)

/* For Disable All Index */
# define SMI_TABLE_DISABLE_ALL_INDEX_MASK  (0x00100000) // Fixed Tables
# define SMI_TABLE_ENABLE_ALL_INDEX        (0x00000000) // Fixed Tables
# define SMI_TABLE_DISABLE_ALL_INDEX       (0x00100000) // Fixed Tables

// PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
// SUPPLEMENTAL LOGGING해야하는지 여부 플래그
# define SMI_TABLE_SUPPLEMENTAL_LOGGING_MASK    (0x00200000)
// Mask에 해당하는 모든 Bit가 0인 플래그를 Default로 사용한다.
// Default => Table에 대해 SUPPLEMENTAL LOGGING하지 않음
# define SMI_TABLE_SUPPLEMENTAL_LOGGING_FALSE   (0x00000000)
# define SMI_TABLE_SUPPLEMENTAL_LOGGING_TRUE    (0x00200000)

// PROJ-2264
// dictionary table 여부를 판단하데 필요한 flag 정보
// debug 용도로 사용하는 flag
// altibase.properties 에서 __FORCE_COMPRESSION_COLUMN = 1 일때 작동하는 flag
# define SMI_TABLE_DICTIONARY_MASK         (0x00800000)
# define SMI_TABLE_DICTIONARY_FALSE        (0x00000000)
# define SMI_TABLE_DICTIONARY_TRUE         (0x00800000)

/* aFlag VON smiTable::createIndex                   */
# define SMI_INDEX_UNIQUE_MASK             (0x00000001)
# define SMI_INDEX_UNIQUE_DISABLE          (0x00000000)
# define SMI_INDEX_UNIQUE_ENABLE           (0x00000001)

/* aFlag VON smiTable::createIndex                   */
# define SMI_INDEX_TYPE_MASK               (0x00000002)
# define SMI_INDEX_TYPE_NORMAL             (0x00000000)
# define SMI_INDEX_TYPE_PRIMARY_KEY        (0x00000002)

/* aFlag VON smiTable::createIndex                   */
# define SMI_INDEX_PERSISTENT_MASK         (0x00000004)
# define SMI_INDEX_PERSISTENT_DISABLE      (0x00000000)
# define SMI_INDEX_PERSISTENT_ENABLE       (0x00000004)

/* aFlag VON smiTable::createIndex                   */
# define SMI_INDEX_USE_MASK                (0x00000008)
# define SMI_INDEX_USE_ENABLE              (0x00000000)
# define SMI_INDEX_USE_DISABLE             (0x00000008)

/* aFlag VON smiTable::createIndex                   */
// PROJ-1502 PARTITIONED DISK TABLE
# define SMI_INDEX_LOCAL_UNIQUE_MASK       (0x00000020)
# define SMI_INDEX_LOCAL_UNIQUE_DISABLE    (0x00000000)
# define SMI_INDEX_LOCAL_UNIQUE_ENABLE     (0x00000020)

/* PROJ-2433 direct key index 사용 flag
 * aFlag VON smiTable::createIndex */
# define SMI_INDEX_DIRECTKEY_MASK          (0x00000040)
# define SMI_INDEX_DIRECTKEY_FALSE         (0x00000000)
# define SMI_INDEX_DIRECTKEY_TRUE          (0x00000040)

/*******************************************************************************
 * Index Bulid Flag
 *
 * - LOGGING, FORCE
 *  disk index의 경우, logging, force에 대한 옵션을 지정해서 생성할 수 있는데,
 * mem, vol index의 경우 logging이나 force에 대한 옵션을 사용하지 않기 때문에
 * CREATE INDEX 구문에서 logging이나 force 옵션을 입력하였을 경우 구문 오류로
 * 판단하여 에러 메세지를 반환한다.
 * 따라서 build flag에서 logging, force 옵션은 아무것도 설정하지 않은 경우와
 * logging, nologging을 설정한 경우, force, noforce를 설정한 경우가 구분이
 * 가능해야 한다. 따라서 logging 옵션과 nologging 옵션을 동시에 설정할 수
 * 없음에도 불구하고 별도의 bit로 값을 설정한다.
 ******************************************************************************/

/* aBuildFlag VON smiTable::createIndex              */
// PROJ-1469 NOLOGGING INDEX BUILD
# define SMI_INDEX_BUILD_DEFAULT                 (0x00000000)

/* aBuildFlag VON smiTable::createIndex              */
// PROJ-1502 PARTITIONED DISK TABLE
# define SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK    (0x00000001) /* 00000001 */
# define SMI_INDEX_BUILD_UNCOMMITTED_ROW_DISABLE (0x00000000) /* 00000000 */
# define SMI_INDEX_BUILD_UNCOMMITTED_ROW_ENABLE  (0x00000001) /* 00000001 */

/* aBuildFlag VON smiTable::createIndex              */
// PROJ-1469 NOLOGGING INDEX BUILD
# define SMI_INDEX_BUILD_FORCE_MASK              (0x00000006) /* 00000110 */
# define SMI_INDEX_BUILD_NOFORCE                 (0x00000002) /* 00000010 */
# define SMI_INDEX_BUILD_FORCE                   (0x00000004) /* 00000100 */

/* aBuildFlag VON smiTable::createIndex               */
// PROJ-1469 NOLOGGING INDEX BUILD
# define SMI_INDEX_BUILD_LOGGING_MASK            (0x00000018) /* 00011000 */
# define SMI_INDEX_BUILD_LOGGING                 (0x00000008) /* 00001000 */
# define SMI_INDEX_BUILD_NOLOGGING               (0x00000018) /* 00010000 */

/* aBuildFlag VON smiTable::createIndex               */
// TOP-DOWN, BOTTOM-UP MASK
# define SMI_INDEX_BUILD_FASHION_MASK            (0x00000020) /* 00100000 */
# define SMI_INDEX_BUILD_BOTTOMUP                (0x00000000) /* 00000000 */
# define SMI_INDEX_BUILD_TOPDOWN                 (0x00000020) /* 00100000 */

/* aBuildFlag VON smnManager::enableAllIndex          */
// PROJ-2184 RP Sync 성능 향상
# define SMI_INDEX_BUILD_DISK_DEFAULT (                                 \
                    SMI_INDEX_BUILD_DEFAULT                 |           \
                    SMI_INDEX_BUILD_NOFORCE                 |           \
                    SMI_INDEX_BUILD_LOGGING )

/* BUG-44794 인덱스 빌드시 인덱스 통계 정보를 수집하지 않는 히든 프로퍼티 추가
 * smuProperty::getGatherIndexStatOnDDL()
 *  - ID_TRUE: DDL 수행 시 Index runtime 통계 저장
 *  - ID_FALSE: DDL 수행 시 Index runtime 통계 저장 안함
 * ENUM
 *  - SMI_INDEX_BUILD_RT_STAT_UPDATE: Index runtime 통계 저장
 *  - SMI_INDEX_BUILD_RT_STAT_NO_UPDATE: Index runtime 통계 저장 안함
 * SMI_INDEX_BUILD_NEED_RT_STAT :
 *  - Property 및 Transaction의 DDL 여부에 따라
 *    Index runtime 통계를 저장하는지 여부 결정 후 그에 맞는 __FLAG 값 설정
 *  - smuProperty::getGatherIndexStatOnDDL() == ID_TRUE 인 경우
 *   + runtime 통계 저장
 *  - smuProperty::getGatherIndexStatOnDDL() == ID_FALSE 인 경우
 *   + __TX == NULL 인 경우 OR __TX->mIsDDL == ID_TRUE 인 경우는 runtime 통계 저장 안함
 *   + 그 외의 경우 ( __TX != NULL AND __TX->mIsDDL == ID_FALSE ) runtime 통계 저장
 */
# define SMI_INDEX_BUILD_RT_STAT_MASK            (0x00000001) /* 00000001 */
# define SMI_INDEX_BUILD_RT_STAT_NO_UPDATE       (0x00000000) /* 00000000 */
# define SMI_INDEX_BUILD_RT_STAT_UPDATE          (0x00000001) /* 00000001 */

# define SMI_INDEX_BUILD_NEED_RT_STAT( __FLAG, __TX ) \
                    ( ( smuProperty::getGatherIndexStatOnDDL() == ID_TRUE ) \
                      ? ( ( __FLAG ) |= SMI_INDEX_BUILD_RT_STAT_UPDATE ) \
                      : SMI_INDEX_BUILD_RT_STAT_WITHOUT_DDL( ( __FLAG ), ( __TX ) ) )
# define SMI_INDEX_BUILD_RT_STAT_WITHOUT_DDL( __FLAG, __TX ) \
                    ( ( ( __TX ) == NULL || ( __TX )->mIsDDL == ID_TRUE ) \
                        ? ( ( __FLAG ) &= ~SMI_INDEX_BUILD_RT_STAT_UPDATE ) \
                        : ( ( __FLAG ) |= SMI_INDEX_BUILD_RT_STAT_UPDATE ) )


/* aFlag VON smiTrans::begin                         */
# define SMI_ISOLATION_MASK                (0x00000003)
# define SMI_ISOLATION_CONSISTENT          (0x00000000)
# define SMI_ISOLATION_REPEATABLE          (0x00000001)
# define SMI_ISOLATION_NO_PHANTOM          (0x00000002)

/* aFlag VON smiTrans::begin                         */
# define SMI_TRANSACTION_MASK              (0x00000004)
# define SMI_TRANSACTION_NORMAL            (0x00000000)
# define SMI_TRANSACTION_UNTOUCHABLE       (0x00000004)

/* PROJ-1541 smiTrans::begin
 * transaction의 Flag에 Set되며, MASK가 가리키는 3 비트는
 * 아래의 값 중 하나의 값만을 가질 수 있음
 * None이외의 모드는 모두 REPICATION대상 트랜잭션임
 *+----------------------------------------------+
 *|TxMode / ReplMode| Lazy    |  Acked |  Eager  |
 *|----------------------------------------------|
 *|Default          | Lazy    |  Acked |  Eager  |
 *|----------------------------------------------|
 *|None(REPLICATED) | N/A     |  N/A   |  N/A    |
 *+----------------------------------------------+
 */
# define SMI_TRANSACTION_REPL_MASK         (0x00000070) //00000001110000
# define SMI_TRANSACTION_REPL_DEFAULT      (0x00000000) //00000000000000
# define SMI_TRANSACTION_REPL_NONE         (0x00000010) //00000000010000
# define SMI_TRANSACTION_REPL_RECOVERY     (0x00000020) //00000000100000
# define SMI_TRANSACTION_REPL_NOT_SUPPORT  (0x00000040) //00000001000000
# define SMI_TRANSACTION_REPL_REPLICATED   SMI_TRANSACTION_REPL_NONE
# define SMI_TRANSACTION_REPL_PROPAGATION  SMI_TRANSACTION_REPL_RECOVERY

/* TASK-6548 duplicated unique key */
# define SMI_TRANSACTION_REPL_CONFLICT_RESOLUTION_MASK (0x00000080) //00000010000000
# define SMI_TRANSACTION_REPL_CONFLICT_RESOLUTION      (0x00000080) //00000010000000

/* PROJ-1381 Fetch Across Commits
 * aFlag for Transaction Reuse */
# define SMI_RELEASE_TRANSACTION           (0x00000000)
# define SMI_DO_NOT_RELEASE_TRANSACTION    (0x00000001)


/* aFlag VON smiTrans::begin                         */
/* BUG-15396 : commit시 log를 disk에 sync하는 것을 기다릴지에 대한 flag */
# define SMI_COMMIT_WRITE_MASK             (0x00000100)
# define SMI_COMMIT_WRITE_NOWAIT           (0x00000000)
# define SMI_COMMIT_WRITE_WAIT             (0x00000100)

/* aFlag VON smiTrans::begin
 * BUG-33539 : In-place update 를 수행할지, 안 할지 결정 */
# define SMI_TRANS_INPLACE_UPDATE_MASK     (0x00000600) //00011000000000
# define SMI_TRANS_INPLACE_UPDATE_DISABLE  (0x00000200) //00001000000000
# define SMI_TRANS_INPLACE_UPDATE_TRY      (0x00000400) //00010000000000

/* BUG-47472 DBHang 현상 관련 디버그 코드 */
# define SMI_TRANS_LOCK_DEBUG_INFO_MASK         (0x00000800)
# define SMI_TRANS_LOCK_DEBUG_INFO_ENABLE       (0x00000800)
# define SMI_TRANS_LOCK_DEBUG_INFO_DISABLE      (0x00000000)

# define SMI_IS_LOCK_DEBUG_INFO_ENABLE(aFlag) ( ((aFlag) & SMI_TRANS_LOCK_DEBUG_INFO_MASK)    \
                                                == SMI_TRANS_LOCK_DEBUG_INFO_ENABLE ? ID_TRUE : ID_FALSE )

/* PROJ-2733 GlobalConsistentTx : GLOBAL_TRANSACTION_LEVEL=3 으로 설정된 Tx */
# define SMI_TRANS_GCTX_MASK (0x00001000)
# define SMI_TRANS_GCTX_OFF  (0x00000000)
# define SMI_TRANS_GCTX_ON   (0x00001000)

#define SMI_IS_GCTX_ON(aFlag) ( ((aFlag) & SMI_TRANS_GCTX_MASK)    \
                                 == SMI_TRANS_GCTX_OFF ? ID_FALSE : ID_TRUE )

/* aFlag VON smiStatement::begin                     */
// PROJ-2199 SELECT func() FOR UPDATE 지원
// SMI_STATEMENT_FORUPDATE 추가
# define SMI_STATEMENT_MASK                (0x0000000C)
# define SMI_STATEMENT_NORMAL              (0x00000000)
# define SMI_STATEMENT_UNTOUCHABLE         (0x00000004)
# define SMI_STATEMENT_FORUPDATE           (0x00000008)

/* aFlag VON smiStatement::begin                     */
/* FOR A4 : Memory or Disk or Either                 */
# define SMI_STATEMENT_CURSOR_MASK         (0x00000003)
# define SMI_STATEMENT_CURSOR_NONE         (0x00000000)
# define SMI_STATEMENT_MEMORY_CURSOR       (0x00000001)
# define SMI_STATEMENT_DISK_CURSOR         (0x00000002)
# define SMI_STATEMENT_ALL_CURSOR          (SMI_STATEMENT_MEMORY_CURSOR |\
                                            SMI_STATEMENT_DISK_CURSOR)

/* foreign key용 statement::begin */
# define SMI_STATEMENT_SELF_MASK           (0x00000020)
# define SMI_STATEMENT_SELF_FALSE          (0x00000000)
# define SMI_STATEMENT_SELF_TRUE           (0x00000020)

/* PROJ-2733 분산 트랜잭션 정합성
 * aFlag smiStatement::begin */ 
# define SMI_STATEMENT_VIEWSCN_MASK        (0x00000040)
# define SMI_STATEMENT_VIEWSCN_LASTEST     (0x00000000)  // 최신뷰를 본다.
# define SMI_STATEMENT_VIEWSCN_REQUESTED   (0x00000040)  // 요구자 SCN 을 이용한다. 

# define SMI_STATEMENT_VIEWSCN_IS_REQUESTED(aFlag) ( ((aFlag) & SMI_STATEMENT_VIEWSCN_MASK) \
                                                     == SMI_STATEMENT_VIEWSCN_LASTEST ? ID_FALSE : ID_TRUE )

/* aFlag VON smiTrans::commit or smiStatement::open  */
# define SMI_STATEMENT_LEGACY_MASK         (0x00000010)
# define SMI_STATEMENT_LEGACY_NONE         (0x00000000)
# define SMI_STATEMENT_LEGACY_TRUE         (0x00000010)

/* aFlag VON smiStatement::end                       */
# define SMI_STATEMENT_RESULT_MASK         (0x00000001)
# define SMI_STATEMENT_RESULT_SUCCESS      (0x00000000)
# define SMI_STATEMENT_RESULT_FAILURE      (0x00000001)

/* aFlag VON smiTableCursor::open                    */
# define SMI_LOCK_MASK                     (0x00000007)
# define SMI_LOCK_READ                     (0x00000000)
# define SMI_LOCK_WRITE                    (0x00000001)
# define SMI_LOCK_REPEATABLE               (0x00000002)
# define SMI_LOCK_TABLE_SHARED             (0x00000003)
# define SMI_LOCK_TABLE_EXCLUSIVE          (0x00000004)

/* aFlag VON smiTableCursor::open                    */
# define SMI_PREVIOUS_MASK                 (0x00000008)
# define SMI_PREVIOUS_DISABLE              (0x00000000)
# define SMI_PREVIOUS_ENABLE               (0x00000008)

/* aFlag VON smiTableCursor::open                    */
# define SMI_TRAVERSE_MASK                 (0x00000010)
# define SMI_TRAVERSE_FORWARD              (0x00000000)
# define SMI_TRAVERSE_BACKWARD             (0x00000010)

/* aFlag VON smiTableCursor::open                    */
# define SMI_INPLACE_UPDATE_MASK           (0x00000020)
# define SMI_INPLACE_UPDATE_ENABLE         (0x00000000)
# define SMI_INPLACE_UPDATE_DISABLE        (0x00000020)

/* aFlag VON smiTableCursor::open    ( BUG-47758 )   */
# define SMI_TRANS_ISOLATION_IGNORE        (0x00000040)

/* aFlag VON smiTableCursor::readOldRow/readNewRow   */
# define SMI_FIND_MODIFIED_MASK            (0x00000300)
# define SMI_FIND_MODIFIED_NONE            (0x00000000)
# define SMI_FIND_MODIFIED_OLD             (0x00000100)
# define SMI_FIND_MODIFIED_NEW             (0x00000200)

/* aFlag VON smiTableCursor::readOldRow/readNewRow   */
/* readOldRow()/readNewRow() 수행 시, 현재 읽고 있는 undo page list */
# define SMI_READ_UNDO_PAGE_LIST_MASK      (0x00000C00)
# define SMI_READ_UNDO_PAGE_LIST_NONE      (0x00000000)
# define SMI_READ_UNDO_PAGE_LIST_INSERT    (0x00000400)
# define SMI_READ_UNDO_PAGE_LIST_UPDATE    (0x00000800)

/* PROJ-1566 */
/* smiTableCursor::mFlag */
# define SMI_INSERT_METHOD_MASK            (0x00001000)
# define SMI_INSERT_METHOD_NORMAL          (0x00000000)
# define SMI_INSERT_METHOD_APPEND          (0x00001000)

#if 0
/* Proj-2059
 * Lob Nologging 지원 */
/* smiTableCursor::mFlag */
# define SMI_INSERT_LOBLOGGING_MASK        (0x00002000)
# define SMI_INSERT_LOBLOGGING_ENABLE      (0x00000000)
# define SMI_INSERT_LOBLOGGING_DISABLE     (0x00002000)
#endif

/* aFlag VON smiTableCursor::readRow                 */
# define SMI_FIND_MASK                     (0x00000003)
# define SMI_FIND_NEXT                     (0x00000000)
# define SMI_FIND_PREV                     (0x00000001)
# define SMI_FIND_CURR                     (0x00000002)
# define SMI_FIND_BEFORE                   (0x00000002)
# define SMI_FIND_AFTER                    (0x00000003)
# define SMI_FIND_RETRAVERSE_NEXT          (0x00000004)
# define SMI_FIND_RETRAVERSE_BEFORE        (0x00000005)

/* aFlag VON Emergency State                 */
# define SMI_EMERGENCY_MASK                (0x00000003)
# define SMI_EMERGENCY_DB_MASK             (0x00000001)
# define SMI_EMERGENCY_LOG_MASK            (0x00000002)
# define SMI_EMERGENCY_DB_SET              (0x00000001)
# define SMI_EMERGENCY_LOG_SET             (0x00000002)
# define SMI_EMERGENCY_DB_CLR              (0xFFFFFFFE)
# define SMI_EMERGENCY_LOG_CLR             (0xFFFFFFFD)

/* iterator state flag */
# define SMI_RETRAVERSE_MASK               (0x00000003)
# define SMI_RETRAVERSE_BEFORE             (0x00000000)
# define SMI_RETRAVERSE_NEXT               (0x00000001)
# define SMI_RETRAVERSE_AFTER              (0x00000002)

# define SMI_ITERATOR_TYPE_MASK            (0x00000010)
# define SMI_ITERATOR_WRITE                (0x00000000)
# define SMI_ITERATOR_READ                 (0x00000010)

/* sequence read flag */
# define SMI_SEQUENCE_MASK                 (0x00000010)
# define SMI_SEQUENCE_CURR                 (0x00000000)
# define SMI_SEQUENCE_NEXT                 (0x00000010)

/* sequence circular flag */
# define SMI_SEQUENCE_CIRCULAR_MASK        (0x00000010)
# define SMI_SEQUENCE_CIRCULAR_DISABLE     (0x00000000)
# define SMI_SEQUENCE_CIRCULAR_ENABLE      (0x00000010)

/* PROJ-2365 sequence table */
/* sequence table flag */
# define SMI_SEQUENCE_TABLE_MASK           (0x00000020)
# define SMI_SEQUENCE_TABLE_FALSE          (0x00000000)
# define SMI_SEQUENCE_TABLE_TRUE           (0x00000020)

/* TASK-7217 Sharded sequence */
/* sequence locality flag */
# define SMI_SEQUENCE_LOCALITY_MASK        (0x00000700)
# define SMI_SEQUENCE_LOCALITY_LOCAL       (0x00000100)
# define SMI_SEQUENCE_LOCALITY_SHARD       (0x00000200)
# define SMI_SEQUENCE_LOCALITY_GLOBAL      (0x00000400)  // UNUSED

/* TASK-7217 Sharded sequence */
/* sequence scale of maxvalue for sharded sequence */
# define SMI_SEQUENCE_SCALE_MASK           (0x0000000F)

/* TASK-7217 Sharded sequence */
# define SMI_SEQUENCE_SCALE_FIXED_MASK     (0x00001000)
# define SMI_SEQUENCE_SCALE_FIXED_FALSE    (0x00000000)  // VARIABLE
# define SMI_SEQUENCE_SCALE_FIXED_TRUE     (0x00001000)

/* smiTimeStamp flag */
# define SMI_TIMESTAMP_RETRAVERSE_MASK     (0x00000001)
# define SMI_TIMESTAMP_RETRAVERSE_DISABLE  (0x00000000)
# define SMI_TIMESTAMP_RETRAVERSE_ENABLE   (0x00000001)

/* FOR A4 : DROP Table Space flag */
# define SMI_DROP_TBLSPACE_MASK            (0x00000111)
# define SMI_DROP_TBLSPACE_ONLY            (0x00000000)
# define SMI_DROP_TBLSPACE_CONTENTS        (0x00000001)
# define SMI_DROP_TBLSPACE_DATAFILES       (0x00000010)
# define SMI_DROP_TBLSPACE_CONSTRAINT      (0x00000100)


/* FOR A4 : Startup Phase로 전이할때 ACTION  flag */
# define SMI_STARTUP_ACTION_MASK            (0x00001111)
# define SMI_STARTUP_NOACTION               (0x00000000)
# define SMI_STARTUP_NORESETLOGS            (0x00000000)
# define SMI_STARTUP_RESETLOGS              (0x00000001)
# define SMI_STARTUP_NORESETUNDO            (0x00000000)
# define SMI_STARTUP_RESETUNDO              (0x00000010)


/* RP2 smiLogRecord usage defined */
#define SMI_LOG_TYPE_MASK           SMR_LOG_TYPE_MASK
#define SMI_LOG_TYPE_NORMAL         SMR_LOG_TYPE_NORMAL
#define SMI_LOG_TYPE_REPLICATED      SMR_LOG_TYPE_REPLICATED
#define SMI_LOG_TYPE_REPL_RECOVERY  SMR_LOG_TYPE_REPL_RECOVERY

#define SMI_LOG_SAVEPOINT_MASK      SMR_LOG_SAVEPOINT_MASK
#define SMI_LOG_SAVEPOINT_NO        SMR_LOG_SAVEPOINT_NO
#define SMI_LOG_SAVEPOINT_OK        SMR_LOG_SAVEPOINT_OK

#define SMI_LOG_BEGINTRANS_MASK     SMR_LOG_BEGINTRANS_MASK
#define SMI_LOG_BEGINTRANS_NO       SMR_LOG_BEGINTRANS_NO
#define SMI_LOG_BEGINTRANS_OK       SMR_LOG_BEGINTRANS_OK

#define SMI_LOG_CONTINUE_MASK       (0x000000200)
#define SMI_LOG_COMMIT_MASK         (0x000000100)

// BUGBUG mtcDef.h와 동일하게 하게 한다.
// MTD_OFFSET_USELESS,MTD_OFFSET_USE
# define SMI_OFFSET_USELESS                (0x00000001)
# define SMI_OFFSET_USE                    (0x00000000)

/* PROJ-2433 Direct Key Index
 * partail direct key 인경우 세팅된다.
 * MTD_PARTIAL_KEY_ON, MTD_PARTIAL_KEY_OFF와 동일하게 해야 한다. */
# define SMI_PARTIAL_KEY_MASK              (0x00000002)
# define SMI_PARTIAL_KEY_ON                (0x00000002)
# define SMI_PARTIAL_KEY_OFF               (0x00000000)

/*
 * BUG-17123 [PRJ-1548] offline된 TableSpace에 대해서 데이타파일을
 *           삭제하다가 Error 발생하여 diff
 *
 * DML DDL의 Validation, Execution시에
 * 테이블스페이스에 대한 lock validation을 하기 위해서
 * 다음과 같은 LV Option Type을 입력해야한다.
 */
typedef enum smiTBSLockValidType
{
    SMI_TBSLV_NONE = 0,
    SMI_TBSLV_DDL_DML,  // OLINE TBS만 Lock 획득 가능
    SMI_TBSLV_DROP_TBS, // ONLINE/OFFLINE/DISCARDED TBS Lock 획득 가능
    SMI_TBSLV_OPER_MAXMAX
} smiTBSLockValidType;

typedef struct smiValue
{
    UInt        length;
    const void* value;
} smiValue;


typedef struct smiColumn
{
    UInt         id;
    UInt         flag;
    UInt         offset;
    UInt         varOrder;      /* Column 중 variable 컬럼들 간의 순서 */
    /* Variable Column데이타가 In, Out으로 저장될지 결정하는 길이
       로서 if Variable column length <= vcInOutBaseSize, in-mode,
       else out-mode */
    UInt         vcInOutBaseSize;
    UInt         size;
    UShort       align;         /* BUG-43117 */
    UShort       maxAlign;      /* BUG-43287 smiColumn List의 Variable Column 중 가장 큰 align 값 */
    void       * value;

    /*
     * PROJ-1362 LOB, LOB column에서만 의미있다.
     *
     * Column이 저장된 Tablespace의 ID
     * Memory Table : Table이 속한 SpaceID와 항상 동일
     * Disk Table   : LOB의 경우 Table이 속한 SpaceID와
     *                다른 Tablespace에 저장될 수 있으므로
     *                Table 의 Space ID와 다른 값이 될수있다
     */
    scSpaceID     colSpace;
    scGRID        colSeg;
    UInt          colType; /* PROJ-2047 Strengthening LOB (CLOB or BLOB) */
    void        * descSeg; /* Disk Lob Segment에 대한 기술자 */

    smiColumnStat mStat;
    // PROJ-2264
    smOID         mDictionaryTableOID;
} smiColumn;

// BUG-30711
// ALTER TABLE ... MODIFY COLUMN ... 수행 시에
// 바뀌지 않는 정보를 원복해줄때 사용함
#define SMI_COLUMN_LOB_INFO_COPY(  _dst_, _src_ )\
{                                                \
    _dst_->colSpace  = _src_->colSpace;          \
    _dst_->colSeg    = _src_->colSeg;            \
    _dst_->descSeg   = _src_->descSeg;           \
}

typedef struct smiColumnList
{
    smiColumnList*   next;
    const smiColumn* column;
}
smiColumnList;

// PROJ-1705
typedef struct smiFetchColumnList
{
    const smiColumn    *column;
    UShort              columnSeq;
    void               *copyDiskColumn;
    smiFetchColumnList *next;
} smiFetchColumnList;

#define SMI_GET_COLUMN_SEQ(aColumn) \
            SDC_GET_COLUMN_SEQ(aColumn)

/* TASK-5030 Full XLog
 * MRDB DML에서 column list를 sort하기 위한 구조체 */
typedef struct smiUpdateColumnList
{
    const smiColumn   * column;
    UInt                position;
} smiUpdateColumnList;

typedef IDE_RC (*smiCallBackFunc)( idBool     * aResult,
                                   const void * aRow,
                                   void       * aDirectKey,             /* PROJ-2433 direct key index */
                                   UInt         aDirectKeyPartialSize,  /* PROJ-2433 */
                                   const scGRID aGRID,
                                   void       * aData );

typedef struct smiCallBack
{
    // For A4 : Hash Index의 경우 Hash Value로 사용
    UInt            mHashVal; // Hash Value
    smiCallBackFunc callback;
    void*           data;
}
smiCallBack;

typedef struct smiRange
{
    idvSQL  *   statistics;
    smiRange*   prev;
    smiRange*   next;

    // For A4 : Hash Index의 경우 hash value를 포함함.
    smiCallBack minimum;
    smiCallBack maximum;
}
smiRange;


/*
 * for database link
 */
typedef struct smiRemoteTableColumn
{
    SChar * mName;
    UInt mTypeId;       /* MTD_xxx_ID */
    SInt mPrecision;
    SInt mScale;
} smiRemoteTableColumn;

typedef struct smiRemoteTable
{
    idBool  mIsStore;
    SChar * mLinkName;
    SChar * mRemoteQuery;

    SInt mColumnCount;
    smiRemoteTableColumn * mColumns;
} smiRemoteTable;

typedef struct smiRemoteTableParam
{
    void * mQcStatement;
    void * mDkiSession;
} smiRemoteTableParam;


/*
 * PROJ-1784 DML without retry
 * fetch시 어떤 버전의 row를 구성 할 지 여부
 */
typedef enum
{
    SMI_FETCH_VERSION_CONSISTENT,// 현재 view의 row를 읽어온다.
    SMI_FETCH_VERSION_LAST,      // 최신 row를 읽어온다.
    SMI_FETCH_VERSION_LASTPREV   // 최신 row의 바로 앞 버전을 읽는다.
                                 //   index old key를 제거하기 위해 필요
} smFetchVersion;

/* PROJ-1784 DML without retry
 *  retry 유무를 판단하기 위한 정보
 *  QP에서 설정하고 SM에서 사용 */
typedef struct smiDMLRetryInfo
{
    idBool                mIsWithoutRetry;   // QP용 retry info on/off flag
    idBool                mIsRowRetry;       // row retry인지 여부
    const smiColumnList * mStmtRetryColLst;  // statement retry 판단을 위한 column list
    const smiValue      * mStmtRetryValLst;  // statement retry 판단을 위한 value list
    const smiColumnList * mRowRetryColLst;   // row retry 판단을 위한 column list
    const smiValue      * mRowRetryValLst;   // row retry 판단을 위한 value list

}smiDMLRetryInfo;

// PROJ-2402
typedef struct smiParallelReadProperties
{
    ULong      mThreadCnt;
    ULong      mThreadID;
    UInt       mParallelReadGroupID;
} smiParallelReadProperties;

/* FOR A4 : smiCursorProperties
   smiCursor::open 함수의 인자를 줄이기위해 추가됨.
   나중에 Cursor관련 기능 추가시에 이 구조체에 멤버로 추가
*/
typedef struct smiCursorProperties
{
    ULong                  mLockWaitMicroSec;
    ULong                  mFirstReadRecordPos;
    ULong                  mReadRecordCount;
    // PROJ-1502 PARTITIONED DISK TABLE
    idBool                 mIsUndoLogging;
    idvSQL                *mStatistics;

    /* for remote table */
    smiRemoteTable       * mRemoteTable;
    smiRemoteTableParam    mRemoteTableParam;

    // PROJ-1665
    UInt                   mHintParallelDegree;

    // PROJ-1705
    smiFetchColumnList    *mFetchColumnList;   // 패치시 복사가 필요한 컬럼리스트정보
    UChar                 *mLockRowBuffer;
    UInt                   mLockRowBufferSize;

    // PROJ-2113
    UChar                  mIndexTypeID;

    // PROJ-2402
    smiParallelReadProperties mParallelReadProperties;

} smiCursorProperties;

// smiCursorProperties를 초기화한다.
#define SMI_CURSOR_PROP_INIT_WITH_TYPE(aProp, aStat, aIndexType) \
    (aProp)->mLockWaitMicroSec = ID_ULONG_MAX;     \
    (aProp)->mFirstReadRecordPos = 0;              \
    (aProp)->mReadRecordCount = ID_ULONG_MAX;      \
    (aProp)->mIsUndoLogging = ID_TRUE;             \
    (aProp)->mStatistics = aStat;                  \
    (aProp)->mRemoteTable = NULL;                  \
    (aProp)->mHintParallelDegree = 0;              \
    (aProp)->mFetchColumnList = NULL;              \
    (aProp)->mLockRowBuffer = NULL;                \
    (aProp)->mLockRowBufferSize = 0;               \
    (aProp)->mIndexTypeID = aIndexType;            \
    (aProp)->mParallelReadProperties.mThreadCnt = 1; \
    (aProp)->mParallelReadProperties.mThreadID  = 1; \
    (aProp)->mParallelReadProperties.mParallelReadGroupID = 0;

// qp meta table의 full scan을 위하여 smiCursorProperties를 초기화한다.
#define SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN(aProp, aStat) \
    SMI_CURSOR_PROP_INIT_WITH_TYPE(aProp, aStat, SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID)

// qp meta table의 index scan을 위하여 smiCursorProperties를 초기화한다.
#define SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(aProp, aStat) \
    SMI_CURSOR_PROP_INIT_WITH_TYPE(aProp, aStat, SMI_BUILTIN_B_TREE_INDEXTYPE_ID)

// qp table의 full scan을 위하여 smiCursorProperties를 초기화한다.
#define SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN(aProp, aStat) \
    SMI_CURSOR_PROP_INIT_WITH_TYPE(aProp, aStat, SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID)

// qp table의 index scan을 위하여 smiCursorProperties를 초기화한다.
#define SMI_CURSOR_PROP_INIT_FOR_INDEX_SCAN(aProp, aStat, aIndexType) \
    SMI_CURSOR_PROP_INIT_WITH_TYPE(aProp, aStat, aIndexType)

// index handle을 이용하여 smiCursorProperties를 초기화한다.
#define SMI_CURSOR_PROP_INIT(aProp, aStat, aIndex) \
    SMI_CURSOR_PROP_INIT_WITH_TYPE(aProp, aStat, smiGetIndexType(aIndex))

typedef struct smiIterator
{
    smSCN       SCN;
    smSCN       infinite;
    void*       trans;
    void*       table;
    SChar*      curRecPtr;
    SChar*      lstFetchRecPtr;
    scGRID      mRowGRID;
    smTID       tid;
    UInt        flag;

    smiCursorProperties  * properties;
    smiStatement         * mStatement;
} smiIterator;

//----------------------------
// PROJ-1872
// compare 할때 필요한 정보
//----------------------------
typedef struct smiValueInfo
{
    const smiColumn * column;
    const void      * value;
    UInt              length;
    UInt              flag;
} smiValueInfo;

#define SMI_SET_VALUEINFO( aValueInfo1, aColumn1, aValue1, aLength1, aFlag1, \
                           aValueInfo2, aColumn2, aValue2, aLength2, aFlag2) \
    (aValueInfo1)->column = (aColumn1);                                      \
    (aValueInfo1)->value  = (aValue1);                                       \
    (aValueInfo1)->length = (aLength1);                             \
    (aValueInfo1)->flag   = (aFlag1);                               \
    (aValueInfo2)->column = (aColumn2);                             \
    (aValueInfo2)->value  = (aValue2);                              \
    (aValueInfo2)->length = (aLength2);                             \
    (aValueInfo2)->flag   = (aFlag2);

/* BUG-42639 Monitoring query */
typedef struct smiFixedTableProperties
{
    ULong         mFirstReadRecordPos;
    ULong         mReadRecordCount;
    ULong         mReadRecordPos;
    idvSQL      * mStatistics;
    smiCallBack * mFilter;
    void        * mTrans;
    smiRange    * mKeyRange;
    smiColumn   * mMinColumn;
    smiColumn   * mMaxColumn;
} smiFixedTableProperties;

#define SMI_FIXED_TABLE_PROPERTIES_INIT( aProp ) \
        ( aProp )->mFirstReadRecordPos = 0;      \
        ( aProp )->mReadRecordCount    = 0;      \
        ( aProp )->mReadRecordPos      = 0;      \
        ( aProp )->mStatistics         = NULL;   \
        ( aProp )->mFilter             = NULL;   \
        ( aProp )->mTrans              = NULL;   \
        ( aProp )->mKeyRange           = NULL;   \
        ( aProp )->mMinColumn          = NULL;   \
        ( aProp )->mMaxColumn          = NULL;


typedef SInt (*smiCompareFunc)( smiValueInfo * aValueInfo1,
                                smiValueInfo * aValueInfo2 );

// PROJ-1618
typedef IDE_RC (*smiKey2StringFunc)( smiColumn  * aColumn,
                                     void       * aValue,
                                     UInt         aValueSize,
                                     UChar      * aCompileFmt,
                                     UInt         aCompileFmtLen,
                                     UChar      * aText,
                                     UInt       * aTextLen,
                                     IDE_RC     * aRet );

// PROJ-1629
typedef void (*smiNullFunc)( const smiColumn* aColumn,
                             const void*      aRow );

typedef void (*smiPartialKeyFunc)( ULong*           aPartialKey,
                                   const smiColumn* aColumn,
                                   const void*      aRow);

typedef UInt (*smiHashKeyFunc) (UInt              aHash,
                                const smiColumn * aColumn,
                                const void      * aRow );

typedef idBool (*smiIsNullFunc)( const smiColumn* aColumn,
                                 const void*      aRow );

// PROJ-1705
typedef IDE_RC (*smiCopyDiskColumnValueFunc)( UInt              aColumnSize,
                                              void            * aDestValue,
                                              UInt              aDestValueOffset,
                                              UInt              aLength,
                                              const void      * aValue );

// aColumn은 반드시 NULL로 넘기고,
// aRow는 해당 컬럼의 value pointer를 가리키고,
// aFlag은 1 ( MTD_OFFSET_USELESS ) 을 넘기도록 한다.
typedef  UInt (*smiActualSizeFunc)( const smiColumn* aColumn,
                                    const void*      aRow );

typedef IDE_RC (*smiFindCompareFunc)( const smiColumn* aColumn,
                                      UInt             aFlag,
                                      smiCompareFunc*  aCompare );

// PROJ-1618
typedef IDE_RC (*smiFindKey2StringFunc)( const smiColumn  * aColumn,
                                         UInt               aFlag,
                                         smiKey2StringFunc* aKey2String );

// PROJ-1629
typedef IDE_RC (*smiFindNullFunc)( const smiColumn*   aColumn,
                                   UInt               aFlag,
                                   smiNullFunc*       aNull );

// PROJ-1705
typedef IDE_RC (*smiFindCopyDiskColumnValueFunc)(
    const smiColumn            * aColumn,
    smiCopyDiskColumnValueFunc * aCopyDiskColumnValueFunc );

// PROJ-2429
// smiStatistics.cpp 에서만 사용된다.
typedef IDE_RC (*smiFindCopyDiskColumnValue4DataTypeFunc)(
    const smiColumn            * aColumn,
    smiCopyDiskColumnValueFunc * aCopyDiskColumnValueFunc );

typedef IDE_RC (*smiFindActualSizeFunc)( const smiColumn   * aColumn,
                                         smiActualSizeFunc * aActualSizeFunc );

typedef IDE_RC (*smiFindPartialKeyFunc)( const smiColumn*   aColumn,
                                         UInt               aFlag,
                                         smiPartialKeyFunc* aPartialKey );

typedef IDE_RC (*smiFindHashKeyFunc)( const smiColumn* aColumn,
                                     smiHashKeyFunc * aHashKeyFunc );

typedef IDE_RC (*smiFindIsNullFunc)( const smiColumn*   aColumn,
                                     UInt               aFlag,
                                     smiIsNullFunc*     aIsNull );

typedef IDE_RC (*smiGetAlignValueFunc)( const smiColumn*   aColumn,
                                        UInt *             aAlignValue );

// PROJ-1705
typedef IDE_RC (*smiGetValueLengthFromFetchBuffer)( idvSQL          * aStatistics,
                                                    const smiColumn * aColumn,
                                                    const void      * aRow,
                                                    UInt            * aColumnSize,
                                                    idBool          * aIsNullValue );

typedef IDE_RC (*smiLockWaitFunc)(ULong aMicroSec, idBool *aWaited);
typedef IDE_RC (*smiLockWakeupFunc)();
typedef void   (*smiSetEmergencyFunc)(UInt);
typedef UInt   (*smiGetCurrTimeFunc)();
typedef void   (*smiDDLSwitchFunc)(SInt aFlag);

/*
    Disk Tablespace를 생성하는 함수 타입
 */
typedef IDE_RC (*smiCreateDiskTBSFunc)(idvSQL             *aStatistics,
                                       smiTableSpaceAttr*  aTableSpaceAttr,
                                       smiDataFileAttr**   aDataFileAttr,
                                       UInt                aDataFileAttrCount,
                                       void*               aTransForMtx);

typedef void (*smiTryUptTransViewSCNFunc)( smiStatement* aStmt );

/* PROJ-1594 Volatile TBS */
typedef IDE_RC (*smiMakeNullRowFunc)(idvSQL        *aStatistics,
                                     smiColumnList *aColumnList,
                                     smiValue      *aNullRow,
                                     SChar         *aValueBuffer);

// BUG-21895
typedef IDE_RC (*smiCheckNeedUndoRecord)(smiStatement * aSmiStmt,
                                         void         * aTableHandle,
                                         idBool       * aIsNeed);

/* BUG-19080: Old Version의 양이 일정이상 만들면 해당 Transaction을
 * Abort하는 기능이 필요합니다.*/
typedef ULong (*smiGetUpdateMaxLogSize)( idvSQL* aStatistics );

/* PROJ-2201
 * Session으로부터 SQL을 얻어오는 기능이 필요합니다. */
typedef IDE_RC (*smiGetSQLText)( idvSQL * aStatistics,
                                 UChar  * aStrBuffer,
                                 UInt     aStrBufferSize);


// TASK-3171
typedef IDE_RC (*smiGetNonStoringSizeFunc)( const smiColumn *aColumn,
                                            UInt * aOutSize );

// PROJ-2059 DB Upgrade 기능
typedef void *(*smiGetColumnHeaderDescFunc)();
typedef void *(*smiGetTableHeaderDescFunc)();
typedef void *(*smiGetPartitionHeaderDescFunc)();
typedef UInt  (*smiGetColumnMapFromColumnHeader)( void * aColumnHeader,
                                                  UInt   aColumnIdx );

// BUG-37484
typedef idBool (*smiNeedMinMaxStatistics)( const smiColumn * aColumn );

typedef IDE_RC (*smiGetColumnStoreLen)( const smiColumn * aColumn,
                                        UInt            * aActualColen );

typedef idBool (*smiIsUsablePartialDirectKey)( void *aColumn );

typedef SInt (*smiGetDDLLockTimeout)(void * aMmSession);

typedef struct smiGlobalCallBackList
{
    smiFindCompareFunc                      findCompare;
    smiFindKey2StringFunc                   findKey2String;  // PROJ-1618
    smiFindNullFunc                         findNull;        // PROJ-1629
    smiFindCopyDiskColumnValueFunc          findCopyDiskColumnValue; // PROJ-1705
    smiFindCopyDiskColumnValue4DataTypeFunc findCopyDiskColumnValue4DataType; // PROJ-2429
    smiFindActualSizeFunc                   findActualSize;
    smiFindHashKeyFunc                      findHash;      // Hash Key 생성 함수를 찾는 함수
    smiFindIsNullFunc                       findIsNull;
    smiGetAlignValueFunc                    getAlignValue;
    smiGetValueLengthFromFetchBuffer        getValueLengthFromFetchBuffer; // PROJ-1705
    smiLockWaitFunc                         waitLockFunc;
    smiLockWakeupFunc                       wakeupLockFunc;
    smiSetEmergencyFunc                     setEmergencyFunc; // 복구 가능한 에러 발생시 호출.
    smiSetEmergencyFunc                     clrEmergencyFunc; // 복구 가능한 에러 해결시 호출.
    smiGetCurrTimeFunc                      getCurrTimeFunc;
    smiDDLSwitchFunc                        switchDDLFunc;
    smiMakeNullRowFunc                      makeNullRow;
    smiCheckNeedUndoRecord                  checkNeedUndoRecord; // BUG-21895
    smiGetUpdateMaxLogSize                  getUpdateMaxLogSize;
    smiGetSQLText                           getSQLText;
    smiGetNonStoringSizeFunc                getNonStoringSize;
    smiGetColumnHeaderDescFunc              getColumnHeaderDescFunc;
    smiGetTableHeaderDescFunc               getTableHeaderDescFunc;
    smiGetPartitionHeaderDescFunc           getPartitionHeaderDescFunc;
    smiGetColumnMapFromColumnHeader         getColumnMapFromColumnHeaderFunc;
    smiNeedMinMaxStatistics                 needMinMaxStatistics; // BUG-37484
    smiGetColumnStoreLen                    getColumnStoreLen; // PROJ-2399
    smiIsUsablePartialDirectKey             isUsablePartialDirectKey; /* PROJ-2433 */
    smiGetDDLLockTimeout                    getDDLLockTimeout;
} smiGlobalCallBackList;


/* not used
// TASK-1421 SM UNIT enhancement , BUG-13124
typedef IDE_RC (*smiCallbackRunSQLFunc)(smiStatement * aSmiStmt,
                                        SChar        * aSqlStr,
                                        vSLong       * aRowCnt );

typedef IDE_RC (*smiCallbackGetTableHandleByNameFunc)(
                                        smiStatement * aSmiStmt,
                                        UInt           aUserID,
                                        UChar        * aTableName,
                                        SInt           aTableNameSize,
                                        void        ** aTableHandle,
                                        smSCN        * aSCN,
                                        idBool       * aIsSequence );

typedef struct smiUnitCallBackList
{
    smiCallbackRunSQLFunc mRunSQLFunc;
    smiCallbackGetTableHandleByNameFunc mGetTableHandleByNameFunc;
}smiUnitCallBackList;
*/

/* ------------------------------------------------
 *  smiInit()을 사용하기 위한 Macros
 *  SM을 사용하는 유틸리티 (altibase 포함)마다
 *  smiInit()이 동작하는 방식이 다르게 결정됨
 * ----------------------------------------------*/

/* not used
typedef enum
{
    SMI_INIT_ACTION_MAKE_DB_NAME     = 0x00000001, // db name 생성
    SMI_INIT_ACTION_MANAGER_INIT     = 0x00000002, // 각종 manager 초기화
    SMI_INIT_ACTION_RESTART_RECOVERY = 0x00000004, // restart recovery 수행
    SMI_INIT_ACTION_REFINE_DB        = 0x00000008, // db refine 수행
    SMI_INIT_ACTION_INDEX_REBUILDING = 0x00000010, // index 재구성
    SMI_INIT_ACTION_USE_SM_THREAD    = 0x00000020, // sm Thread startup
    SMI_INIT_ACTION_CREATE_LOG_FILE  = 0x00000040, // create log file
    SMI_INTI_ACTION_PRINT_INFO       = 0x00000080, // out manager info
    SMI_INIT_ACTION_SHMUTIL_INIT     = 0x00000100,
    SMI_INIT_ACTION_LOAD_ONLY_META   = 0x00000200, // Load Only Meta

    SMI_INIT_ACTION_MAX_END          = 0xFFFFFFFF
} smiInitAction;

// smiInit()의 인자로 입력됨
#define SMI_INIT_CREATE_DB  (SMI_INIT_ACTION_MAKE_DB_NAME |\
                             SMI_INIT_ACTION_MANAGER_INIT |\
                             SMI_INIT_ACTION_CREATE_LOG_FILE)

#define SMI_INIT_META_DB    (SMI_INIT_ACTION_INDEX_REBUILDING|\
                             SMI_INIT_ACTION_USE_SM_THREAD)

#define SMI_INIT_RESTORE_DB (SMI_INIT_ACTION_MAKE_DB_NAME|\
                             SMI_INIT_ACTION_MANAGER_INIT|\
                             SMI_INIT_ACTION_RESTART_RECOVERY|\
                             SMI_INIT_ACTION_REFINE_DB|\
                             SMI_INIT_ACTION_INDEX_REBUILDING|\
                             SMI_INIT_ACTION_USE_SM_THREAD|\
                             SMI_INTI_ACTION_PRINT_INFO)

#define SMI_INIT_SHMUTIL_DB (SMI_INIT_ACTION_MAKE_DB_NAME |\
                             SMI_INIT_ACTION_SHMUTIL_INIT)
*/

/* ------------------------------------------------
 *  A4를 위한 Startup Flags
 * ----------------------------------------------*/
typedef enum
{
    SMI_STARTUP_INIT        = IDU_STARTUP_INIT,
    SMI_STARTUP_PRE_PROCESS = IDU_STARTUP_PRE_PROCESS,  /* for DB Creation    */
    SMI_STARTUP_PROCESS     = IDU_STARTUP_PROCESS,
    SMI_STARTUP_CONTROL     = IDU_STARTUP_CONTROL,
    SMI_STARTUP_META        = IDU_STARTUP_META,
    SMI_STARTUP_SERVICE     = IDU_STARTUP_SERVICE,
    SMI_STARTUP_SHUTDOWN    = IDU_STARTUP_SHUTDOWN,

    SMI_STARTUP_MAX         = IDU_STARTUP_MAX,    // 7
} smiStartupPhase;

//lock Mode의 순서는 smlLockMode와 동일하게 한다.
typedef enum
{
    SMI_TABLE_NLOCK       = 0x00000000,
    SMI_TABLE_LOCK_S      = 0x00000001,
    SMI_TABLE_LOCK_X      = 0x00000002,
    SMI_TABLE_LOCK_IS     = 0x00000003,
    SMI_TABLE_LOCK_IX     = 0x00000004,
    SMI_TABLE_LOCK_SIX    = 0x00000005
} smiTableLockMode;


typedef struct smiCursorPosInfo
{
    union
    {
        struct
        {
            scGRID     mLeafNode;
            smLSN      mLSN;
            scGRID     mRowGRID;
            ULong      mSmoNo;
            SInt       mSlotSeq;
            smiRange * mKeyRange; /* BUG-43913 */
        } mDRPos;
        struct
        {
            void     * mLeafNode;
            void     * mPrevNode;
            void     * mNextNode;
            IDU_LATCH  mVersion;
            void     * mRowPtr;
            smiRange * mKeyRange; /* BUG-43913 */
        } mMRPos;
        struct
        {
            SLong    mIndexPos;
            void   * mRowPtr;
        } mTRPos;  // Memory Temp Table을 위한 Cursor Information
        struct
        {
            void            * mPos;
        } mDTPos;  // DiskTempTable을 위한 CursorInformation
    } mCursor;
} smiCursorPosInfo;

//-----------------------------------------------
// INDEX TYPE의 개수
//    - 최대 인덱스 종류 : 128 개
// BUILT-IN INDEX의 ID
//    - 하위 호환성을 위하여 값을 그대로 유지한다.
//-----------------------------------------------

#define SMI_MAX_INDEXTYPE_ID                    (128)
#define SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID       (0)
#define SMI_BUILTIN_B_TREE_INDEXTYPE_ID           (1)
#define SMI_BUILTIN_HV_B_TREE_INDEXTYPE_ID_LEGACY (2)    /* not used */
#define SMI_BUILTIN_B_TREE2_INDEXTYPE_ID          (3)    /* Deprecated */
#define SMI_BUILTIN_GRID_INDEXTYPE_ID             (4)
#define SMI_ADDITIONAL_RTREE_INDEXTYPE_ID         (6)    /* TASK-3171 */
#define SMI_INDEXTYPE_COUNT                       (7)

#define SMI_INDEXIBLE_TABLE_TYPES    (SMI_TABLE_TYPE_COUNT) // meta,temp,memory,disk,fixed,volatile,remote

/* ----------------------------------------------------------------------------
 *  db dir의 최대 개수
 * --------------------------------------------------------------------------*/
#define SM_DB_DIR_MAX_COUNT       (8)

/* ----------------------------------------------------------------------------
 * verify option
 * 현재는 TBS verify만 지원한다.
 * --------------------------------------------------------------------------*/
// tbs, seg, ext verify
#define SMI_VERIFY_TBS        (0x0001)
// page verify
#define SMI_VERIFY_PAGE       (0x0002)
// sm log에 write
#define SMI_VERIFY_WRITE_LOG  (0x0004)
// dbf verify
#define SMI_VERIFY_DBF        (0x0008)

#define SMI_IS_FIXED_COLUMN(aFlag) ( ((aFlag) & SMI_COLUMN_TYPE_MASK) \
                                     == SMI_COLUMN_TYPE_FIXED ? ID_TRUE : ID_FALSE )
#define SMI_IS_VARIABLE_COLUMN(aFlag) ( ((aFlag) & SMI_COLUMN_TYPE_MASK) \
                                        == SMI_COLUMN_TYPE_VARIABLE ? ID_TRUE : ID_FALSE )
#define SMI_IS_VARIABLE_LARGE_COLUMN(aFlag) ( ((aFlag) & SMI_COLUMN_TYPE_MASK) \
                                              == SMI_COLUMN_TYPE_VARIABLE_LARGE ? ID_TRUE : ID_FALSE )
#define SMI_IS_LOB_COLUMN(aFlag) ( ((aFlag) & SMI_COLUMN_TYPE_MASK)  \
                                   == SMI_COLUMN_TYPE_LOB ? ID_TRUE : ID_FALSE )
#define SMI_IS_OUT_MODE(aFlag) ( ((aFlag) & SMI_COLUMN_MODE_MASK)    \
                                 == SMI_COLUMN_MODE_OUT ? ID_TRUE : ID_FALSE )

//-----------------------------------------------
// PROJ-1877
// alter table modify column
//
// modify column 기능추가로 memory table의 backup & restore시
// 다른 type으로 변환하여 restore하는 기능이 필요하게 되었다.
//
// initilize, finalize는 record 단위로 호출되는 함수이며
// convertValue는 column 마다 원하는 type으로 변환하는 함수이다.
//-----------------------------------------------

typedef IDE_RC (*smiConvertInitializeForRestore)( void * aInfo );

typedef IDE_RC (*smiConvertFinalizeForRestore)( void * aInfo );

typedef IDE_RC (*smiConvertValueFuncForRestore)( idvSQL          * aStatistics,
                                                 const smiColumn * aSrcColumn,
                                                 const smiColumn * aDestColumn,
                                                 smiValue        * aValue,
                                                 void            * aInfo );

/* PROJ-1090 Function-based Index */
typedef IDE_RC (*smiCalculateValueFuncForRestore)( smiValue * aValueArr,
                                                   void     * aInfo );

// BUG-42920 DDL display data move progress
typedef IDE_RC (*smiPrintProgressLogFuncForRestore)( void  * aInfo,
                                                     idBool  aIsProgressComplete );

struct smiAlterTableCallBack {
    void                              * info;   // qdbCallBackInfo

    smiConvertInitializeForRestore      initializeConvert;
    smiConvertFinalizeForRestore        finalizeConvert;
    smiConvertValueFuncForRestore       convertValue;
    smiCalculateValueFuncForRestore     calculateValue;
    smiPrintProgressLogFuncForRestore   printProgressLog;
};




/* Proj-2059 DB Upgrade 기능
 * Server 중심적으로 데이터를 가져오고 넣는 기능 */

// Header Structure를 Endian상관없이 읽고 쓰는 그닝
typedef UInt   smiHeaderType;

#define SMI_DATAPORT_HEADER_OFFSETOF(s,m) (IDU_FT_OFFSETOF(s, m))
#define SMI_DATAPORT_HEADER_SIZEOF(s,m)   (IDU_FT_SIZEOF(s, m))

#define SMI_DATAPORT_HEADER_TYPE_CHAR       (0x0001)
#define SMI_DATAPORT_HEADER_TYPE_LONG       (0x0002)
#define SMI_DATAPORT_HEADER_TYPE_INTEGER    (0x0003)

#define SMI_DATAPORT_HEADER_FLAG_DESCNAME_MASK   (0x0001)
#define SMI_DATAPORT_HEADER_FLAG_DESCNAME_YES    (0x0001)
#define SMI_DATAPORT_HEADER_FLAG_DESCNAME_NO     (0x0000)

#define SMI_DATAPORT_HEADER_FLAG_COLNAME_MASK    (0x0002)
#define SMI_DATAPORT_HEADER_FLAG_COLNAME_YES     (0x0002)
#define SMI_DATAPORT_HEADER_FLAG_COLNAME_NO      (0x0000)

#define SMI_DATAPORT_HEADER_FLAG_TYPE_MASK       (0x0004)
#define SMI_DATAPORT_HEADER_FLAG_TYPE_YES        (0x0004)
#define SMI_DATAPORT_HEADER_FLAG_TYPE_NO         (0x0000)

#define SMI_DATAPORT_HEADER_FLAG_SIZE_MASK       (0x0008)
#define SMI_DATAPORT_HEADER_FLAG_SIZE_YES        (0x0008)
#define SMI_DATAPORT_HEADER_FLAG_SIZE_NO         (0x0000)

#define SMI_DATAPORT_HEADER_FLAG_FULL            \
        ( SMI_DATAPORT_HEADER_FLAG_DESCNAME_YES |\
          SMI_DATAPORT_HEADER_FLAG_COLNAME_YES  |\
          SMI_DATAPORT_HEADER_FLAG_TYPE_YES     |\
          SMI_DATAPORT_HEADER_FLAG_SIZE_YES )

//Validation용 함수
typedef IDE_RC (*smiDataPortHeaderValidateFunc)( void * aDesc,
                                                  UInt   aVersion,
                                                  void * aHeader  );

typedef struct smiDataPortHeaderColDesc
{
    SChar         * mName;       // 이름
    UInt            mOffset;     // Offset
    UInt            mSize;       // Size
    ULong           mDefaultNum; // 하위버전일 경우, 이 값을 대신 설정한다
    SChar         * mDefaultStr; // 하위버전일 경우, 이 값을 대신 설정한다
    smiHeaderType   mType;       // DataType
} smiDataPortHeaderColDesc;

typedef struct smiDataPortHeaderDesc
{
    SChar                          * mName;          // 이름
    UInt                             mSize;          // 원본 헤더의 크기
    smiDataPortHeaderColDesc       * mColumnDesc;    // column정보
    smiDataPortHeaderValidateFunc    mValidateFunc;  // Validation용 함수
} smiDataPortHeaderDesc;

/* ULong을 BigEndian으로 Write한다. */
#define SMI_WRITE_ULONG(src, dst){                      \
     ((UChar*)(dst))[0] = (*((ULong*)(src)))>>(56) & 255; \
     ((UChar*)(dst))[1] = (*((ULong*)(src)))>>(48) & 255; \
     ((UChar*)(dst))[2] = (*((ULong*)(src)))>>(40) & 255; \
     ((UChar*)(dst))[3] = (*((ULong*)(src)))>>(32) & 255; \
     ((UChar*)(dst))[4] = (*((ULong*)(src)))>>(24) & 255; \
     ((UChar*)(dst))[5] = (*((ULong*)(src)))>>(16) & 255; \
     ((UChar*)(dst))[6] = (*((ULong*)(src)))>>( 8) & 255; \
     ((UChar*)(dst))[7] = (*((ULong*)(src)))>>( 0) & 255; \
}

/* UInt을 BigEndian으로 Write한다. */
#define  SMI_WRITE_UINT(src, dst){                     \
     ((UChar*)(dst))[0] = (*(UInt*)(src))>>(24) & 255; \
     ((UChar*)(dst))[1] = (*(UInt*)(src))>>(16) & 255; \
     ((UChar*)(dst))[2] = (*(UInt*)(src))>>( 8) & 255; \
     ((UChar*)(dst))[3] = (*(UInt*)(src))>>( 0) & 255; \
}

/* UShort을 BigEndian으로 Write한다. */
#define  SMI_WRITE_USHORT(src, dst){                    \
    ((UChar*)(dst))[0] = (*(UShort*)(src))>>( 8) & 255; \
    ((UChar*)(dst))[1] = (*(UShort*)(src))>>( 0) & 255; \
}

/* BigEndian으로 기록된 ULong 을 Read한다. */
#define  SMI_READ_ULONG(src, dst){                         \
    *((ULong*)(dst)) = ((ULong)((UChar*)src)[0]<<56)       \
                   | ((ULong)((UChar*)src)[1]<<48)         \
                   | ((ULong)((UChar*)src)[2]<<40)         \
                   | ((ULong)((UChar*)src)[3]<<32)         \
                   | ((ULong)((UChar*)src)[4]<<24)         \
                   | ((ULong)((UChar*)src)[5]<<16)         \
                   | ((ULong)((UChar*)src)[6]<<8 )         \
                   | ((ULong)((UChar*)src)[7]);            \
}

#define  SMI_READ_UINT(src, dst){                     \
    *((UInt*)(dst)) = ((UInt)((UChar*)src)[0]<<24)    \
                   | ((UInt)((UChar*)src)[1]<<16)     \
                   | ((UInt)((UChar*)src)[2]<<8)      \
                   | ((UInt)((UChar*)src)[3]);        \
}


/* BigEndian으로 기록된 UShort을 Read한다. */
#define SMI_READ_USHORT(src, dst){                 \
    *((UShort*)(dst)) = (((UChar*)src)[0]<<8)      \
    | (((UChar*)src)[1]);                          \
}

/* Proj-2059 DB Upgrade 기능
 * Server 중심적으로 데이터를 가져오고 넣는 기능 */

// DataPort기능을 제어하는 Handle

// 아직 초기화되지 않은 RowSeq
#define SMI_DATAPORT_NULL_ROWSEQ      (ID_SLONG_MAX)

// DataPort Object의 종류
#define SMI_DATAPORT_TYPE_FILE        (0)
#define SMI_DATAPORT_TYPE_MAX         (1)

//DataPortHeader의 Version
#define SMI_DATAPORT_VERSION_1        (1)

#define SMI_DATAPORT_VERSION_BEGIN    (1)
#define SMI_DATAPORT_VERSION_LATEST   (SMI_DATAPORT_VERSION_1)
#define SMI_DATAPORT_VERSION_COUNT    (SMI_DATAPORT_VERSION_LATEST + 1)

/* BUG-30503  [PROJ-2059] SYS_DATA_PORTS_ 테이블 컬럼 길이와 qcmDataPortInfo
 * 구조체 및 로컬 배열 길이 불일치
 * QC_MAX_NAME_LEN와 일치해야 함. 너무 클 필요 없음. */
#define SMI_DATAPORT_JOBNAME_SIZE     (40)

// LobColumn 여부
#define SMI_DATAPORT_LOB_COLUMN_TRUE  (true)
#define SMI_DATAPORT_LOB_COLUMN_FALSE (false)

// Object에 저장되는 Header
typedef struct smiDataPortHeader
{
    UInt               mVersion; // Object의 Version.
                                 // Version만은 이하 다른 값과 달리
                                 // 따로 4Byte 선행되어 읽는다.
                                 // 왜냐하면 mVersion에 따라 Header의
                                 // 모양이 달라질 수 있기 때문이다.

    idBool             mIsBigEndian;                // BigEndian여부
    UInt               mCompileBit;                 // 32Bit인가 64Bit인가
    SChar              mDBCharSet[ IDN_MAX_CHAR_SET_LEN ];
    SChar              mNationalCharSet[ IDN_MAX_CHAR_SET_LEN ];

    UInt               mPartitionCount;   // Partition의 개수
    UInt               mColumnCount;      // Column의 개수
    UInt               mBasicColumnCount; // 일반Column의 개수
    UInt               mLobColumnCount;   // LobColumn의 개수

    // 이하 변수들은 세부 Header들로, Encoder의 도움을 받아
    // 기록된다.
    void             * mObjectHeader;     // scp?Header
    void             * mTableHeader;      // qsfTableInfo
    void             * mColumnHeader;     // qsfColumnInfo
    void             * mPartitionHeader;  // qsfPartitionInfo
} smiDataPortHeader;

extern smiDataPortHeaderDesc gSmiDataPortHeaderDesc[];

typedef struct smiRow4DP
{
    smiValue * mValueList;        // 삽입할 Row내의 Value들.
    idBool     mHasSupplementLob; // Lob을 따로 추가적으로 읽어야 하는지 여부
} smiRow4DP;

/* TASK-4990 changing the method of collecting index statistics
 * 수동 통계정보 수집 기능 */

/* 1Byte에 몇Bit인가? */
#define SMI_STAT_BYTE_BIT_COUNT  (8)

/* 1Byte를 구하기 위한 BitMask는? */
#define SMI_STAT_BYTE_BIT_MASK   (SMI_STAT_BYTE_BIT_COUNT-1)

/* HashTable에 저장되는 Bit개수는? */
#define SMI_STAT_HASH_TBL_BIT_COUNT  (24)

/* HashTable의 Byte크기는? */
#define SMI_STAT_HASH_TBL_SIZE   ( 1<<SMI_STAT_HASH_TBL_BIT_COUNT )

/* HashTable의 Byte크기의 mask는? */
#define SMI_STAT_HASH_TBL_MASK   ( SMI_STAT_HASH_TBL_SIZE - 1 )

/* LocalHash가 초기화되는 크기 */
#define SMI_STAT_HASH_THRESHOLD  ( SMI_STAT_HASH_TBL_SIZE / 8 )

/* mtdHash에 의해 4Byte(32bit)인 hash값을 SMI_STAT_HASH_TBL_BIT_COUNT Size로
 * 축소함 */
#define SMI_STAT_HASH_COMPACT(i) ( ( ((UInt)i) & SMI_STAT_HASH_TBL_MASK ) ^ \
                                   ( ((UInt)i) >> SMI_STAT_HASH_TBL_BIT_COUNT ) )

/* HashTable에서 몇번째 Byte인지 찾음 */
#define SMI_STAT_GET_HASH_IDX(i)     ( SMI_STAT_HASH_COMPACT(i) /  \
                                       SMI_STAT_BYTE_BIT_COUNT )

/* HashTable의 해당 Byte에 설정할 Bit를 얻음 */
#define SMI_STAT_GET_HASH_BIT(i)     ( 1 << ( SMI_STAT_HASH_COMPACT(i) \
                                              & SMI_STAT_BYTE_BIT_MASK ) )

/* HashTable에 해당 값의 Bit를 설정함 */
#define SMI_STAT_SET_HASH_VALUE(h,i) ( h[ SMI_STAT_GET_HASH_IDX(i) ] |= \
                                        SMI_STAT_GET_HASH_BIT(i) )

/* HashTable에 해당 값의 Bit를 가져옴함 */
#define SMI_STAT_GET_HASH_VALUE(h,i) ( h[ SMI_STAT_GET_HASH_IDX(i) ] & \
                                        SMI_STAT_GET_HASH_BIT(i) )

/* Column에 대한 분석용 자료구조 */
typedef struct smiStatSystemArgument
{
    SLong  mHashTime;          /* 누적 hash 시간 */
    SLong  mHashCnt;           /* 누적 hash 횟수 */
    SLong  mCompareTime;       /* 누적 compare 시간 */
    SLong  mCompareCnt;        /* 누적 compare 횟수 */
} smiStatSystemArgument;

/* Column에 대한 분석용 자료구조 */
typedef struct smiStatColumnArgument
{
    /* BUG-33548   [sm_transaction] The gather table statistics function
     * doesn't consider SigBus error */
    /* Min max */
    ULong  mMinValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Min값 */
    ULong  mMaxValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Max값 */
    UInt   mMinLength;
    UInt   mMaxLength;

    /* Column Info */
    smiColumn         *        mColumn;
    smiHashKeyFunc             mHashFunc;
    smiActualSizeFunc          mActualSizeFunc;
    smiCompareFunc             mCompare;
    smiCopyDiskColumnValueFunc mCopyDiskColumnFunc;
    smiIsNullFunc              mIsNull;
    smiKey2StringFunc          mKey2String;
    UInt                       mMtdHeaderLength;

    /* NumDist */
    UChar mLocalHashTable[ SMI_STAT_HASH_TBL_SIZE / SMI_STAT_BYTE_BIT_COUNT ];
    UChar mGlobalHashTable[ SMI_STAT_HASH_TBL_SIZE / SMI_STAT_BYTE_BIT_COUNT ];
    SLong mLocalNumDistPerGroup;
    SLong mLocalNumDist;
    SLong mGlobalNumDist;
    UInt  mLocalGroupCount;

    /* Average Column Length */
    SLong mAccumulatedSize;

    /* Result */
    SLong mNumDist;         /* NumberOfDinstinctValue(Cardinality) */
    SLong mNullCount;       /* NullValue Count   */
} smiStatColumnArgument;

/* Table에 대한 분석용 자료구조 */
typedef struct smiStatTableArgument
{
    SFloat mSampleSize;        /* 1~100     */

    SLong  mAnalyzedRowCount;  /* 분석한 로우 개수    */
    SLong  mAccumulatedSize;   /* 누적 로우 크기    */

    SLong  mReadRowTime;       /* 누적 one row read time */
    SLong  mReadRowCnt;        /* 누적 one row read count */

    SLong  mAnalyzedPageCount;
    SLong  mMetaSpace;         /* PageHeader, ExtDir등 Meta 공간 */
    SLong  mUsedSpace;         /* 현재 사용중인 공간 */
    SLong  mAgableSpace;       /* 나중에 Aging가능한 공간 */
    SLong  mFreeSpace;         /* Data삽입이 가능한 빈 공간 */

    smiStatColumnArgument * mColumnArgument;

    /* PROJ-2180 valueForModule
       SMI_OFFSET_USELESS 용 컬럼 */
    smiColumn             * mBlankColumn;
} smiStatTableArgument;

// BUG-40217
// float 타입의 경우 최대길이가 47이다.
// 이를 고려하여 48까지 늘린다.
#define SMI_DBMSSTAT_STRING_VALUE_SIZE (48)

/* X$DBMS_STAT 을 구현하기 위함 */
typedef struct smiDBMSStat4Perf
{
    SChar   mCreateTime[SMI_DBMSSTAT_STRING_VALUE_SIZE];  /* TimeValue */
    SDouble mSampleSize;
    SLong   mNumRowChange;

    SChar   mType;            /* S(System), T(Table), I(Index), C(Column) */

    ULong   mTargetID;
    UInt    mColumnID;

    SDouble mSReadTime;
    SDouble mMReadTime;
    SLong   mDBFileMultiPageReadCount;

    SDouble mHashTime;
    SDouble mCompareTime;
    SDouble mStoreTime;

    SLong   mNumRow;
    SLong   mNumPage;
    SLong   mNumDist;
    SLong   mNumNull;
    SLong   mAvgLen;
    SDouble mOneRowReadTime;
    SLong   mAvgSlotCnt;
    SLong   mIndexHeight;
    SLong   mClusteringFactor;

    SLong   mMetaSpace;
    SLong   mUsedSpace;
    SLong   mAgableSpace;
    SLong   mFreeSpace;

    /* BUG-33548   [sm_transaction] The gather table statistics function
     * doesn't consider SigBus error */
    ULong   mMinValue[SMI_DBMSSTAT_STRING_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Min값 */
    ULong   mMaxValue[SMI_DBMSSTAT_STRING_VALUE_SIZE/ID_SIZEOF(ULong)]; /*Max값 */
} smiDBMSStat4Perf;

/*****************************************************************************
 * PROJ-2201 Innovation in sorting and hashing(temp)
 *****************************************************************************/

/************************** TT Flag (TempTable) *****************************/

#define SMI_TTFLAG_NONE                 (0x00000000)

/* Table의 종류 */
#define SMI_TTFLAG_TYPE_MASK            (0x00000003)
#define SMI_TTFLAG_TYPE_SORT            (0x00000001) /* SortTemp로 */
#define SMI_TTFLAG_TYPE_HASH            (0x00000002) /* 일반 Hash로 */

/* 사용자가 RangeScan을 사용할 경우(ex:SortJoin)
 * 역순위 탐색 및 MultipleIndex도 가능해짐 */
#define SMI_TTFLAG_RANGESCAN            (0x00000010)
/* UniquenessViolation을 일으킴 */
#define SMI_TTFLAG_UNIQUE               (0x00000020)

/************************** TC Flag (TempCursor) ****************************/
/* Default값 */
#define SMI_TCFLAG_INIT           ( SMI_TCFLAG_FORWARD | \
                                    SMI_TCFLAG_ORDEREDSCAN )

#define SMI_TCFLAG_DIRECTION_MASK (0x00000003)/*Scan 방향  */
#define SMI_TCFLAG_FORWARD        (0x00000001)/*정방향(LeftToRight) */
#define SMI_TCFLAG_BACKWARD       (0x00000002)/*역방향(RightToLeft) */

#define SMI_TCFLAG_HIT_MASK       (0x0000000C)/*HitFlag체크 여부 */
#define SMI_TCFLAG_IGNOREHIT      (0x00000000)/*상관없이 탐색 */
#define SMI_TCFLAG_HIT            (0x00000004)/*Hit된 것만 탐색 */
#define SMI_TCFLAG_NOHIT          (0x00000008)/*Hit안된 것은 제외 */

#define SMI_TCFLAG_SCAN_MASK      (0x00000070)/*Scan하는 방법 */
#define SMI_TCFLAG_FULLSCAN       (0x00000000)/*일반적인 Scan */
#define SMI_TCFLAG_ORDEREDSCAN    (0x00000010)/*정렬된 Scan */
#define SMI_TCFLAG_RANGESCAN      (0x00000020)/*Range(또는 HashValue)*/
#define SMI_TCFLAG_HASHSCAN       (0x00000040)/*Hash값으로 탐색)*/

#define SMI_TCFLAG_FILTER_MASK    (0x00000700)/*Filter를 사용여부*/
#define SMI_TCFLAG_FILTER_RANGE   (0x00000100)/*RangeFilter를 사용함*/
#define SMI_TCFLAG_FILTER_KEY     (0x00000200)/*KeyFilter를 사용함*/
#define SMI_TCFLAG_FILTER_ROW     (0x00000400)/*RowFilter를 사용함*/

#define SMI_HASH_CURSOR_NONE         (0x00000000) /* Hash가 cursor를 열 수 없는 상태이다 */
#define SMI_HASH_CURSOR_INIT         (0x00000001) /* Hash가 초기화 되어서 cursor를 열 수 있는 상태 */
#define SMI_HASH_CURSOR_FULL_SCAN    (0x00000002)
#define SMI_HASH_CURSOR_HASH_SCAN    (0x00000003)
#define SMI_HASH_CURSOR_HASH_UPDATE  (0x00000004)

typedef enum
{
    SMI_TTSTATE_INIT,
    SMI_TTSTATE_SORT_INSERTNSORT,
    SMI_TTSTATE_SORT_INSERTONLY,
    SMI_TTSTATE_SORT_EXTRACTNSORT,
    SMI_TTSTATE_SORT_MERGE,
    SMI_TTSTATE_SORT_MAKETREE,
    SMI_TTSTATE_SORT_INMEMORYSCAN,
    SMI_TTSTATE_SORT_MERGESCAN,
    SMI_TTSTATE_SORT_INDEXSCAN,
    SMI_TTSTATE_SORT_SCAN,
    SMI_TTSTATE_HASH_INSERT,
    SMI_TTSTATE_HASH_FETCH_FULLSCAN,
    SMI_TTSTATE_HASH_FETCH_HASHSCAN,
    SMI_TTSTATE_HASH_FETCH_UPDATE
} smiTempState;

struct smiSortTempCursor;
struct smiTempTableHeader;
struct smiTempPosition;

/* TempTable용 Module.
 * Fetch, Store/Restore Cursor등은 Cursor에 Module이 있음 */
typedef IDE_RC (*smiTempOpenCursor)( smiTempTableHeader * aHeader,
                                     smiSortTempCursor  * aCursor );

typedef IDE_RC (*smiTempFetch)( smiSortTempCursor  * aTempCursor,
                                UChar  ** aRow,
                                scGRID  * aRowGRID );
typedef IDE_RC (*smiTempStoreCursor)( smiSortTempCursor * aCursor,
                                      smiTempPosition   * aPosition );
typedef IDE_RC (*smiTempRestoreCursor)( smiSortTempCursor * aCursor,
                                        smiTempPosition   * aPosition );

typedef struct smiTempColumn
{
    UInt                        mIdx;
    smiColumn                   mColumn;
    smiCopyDiskColumnValueFunc  mConvertToCalcForm;
    smiNullFunc                 mNull;
    smiIsNullFunc               mIsNull; /* PROJ-2435 order by nulls first/last */
    smiCompareFunc              mCompare;
    UInt                        mStoringSize;
    smiTempColumn             * mNextKeyColumn;
} smiTempColumn;

/* TempTable에서 수행하는 연산들
 * TTOPR(TempTableOperation) */
typedef enum
{
    SMI_TTOPR_NONE,
    SMI_TTOPR_CREATE,
    SMI_TTOPR_DROP,
    SMI_TTOPR_SORT, 
    SMI_TTOPR_OPENCURSOR,
    SMI_TTOPR_OPENCURSOR_HASH,
    SMI_TTOPR_OPENCURSOR_FULL,
    SMI_TTOPR_OPENCURSOR_UPDATE,
    SMI_TTOPR_RESTARTCURSOR,
    SMI_TTOPR_FETCH,
    SMI_TTOPR_FETCH_HASH,
    SMI_TTOPR_FETCH_FULL,
    SMI_TTOPR_FETCH_FROMGRID,
    SMI_TTOPR_STORECURSOR,
    SMI_TTOPR_RESTORECURSOR,
    SMI_TTOPR_CLEAR,
    SMI_TTOPR_CLEARHITFLAG,
    SMI_TTOPR_INSERT,
    SMI_TTOPR_UPDATE,
    SMI_TTOPR_SETHITFLAG,
    SMI_TTOPR_RESET_KEY_COLUMN,
    SMI_TTOPR_MAX
} smiTempTableOpr;

/* 해당 연산이 SMI_TT_STATS_INTERVAL회 누적되면, 통계정보를 갱신함 */
#define SMI_TT_STATS_INTERVAL (65536)
/* 이 TempTable을 수행한 SQL을 저장해둘 버퍼의 크기 */
#define SMI_TT_SQLSTRING_SIZE (16384)
/* 기타 TempTable용 String의 버퍼 크기 */
#define SMI_TT_STR_SIZE          (36)

/* TempTable 하나에 대한 통계 정보 */
typedef struct smiTempTableStats
{

    ULong             mCount; /* mGlobalStat용. 누적한 통계정보 갯수 */
    ULong             mTime;  /* mGlobalStat용. 누적 시간 */

    UInt              mCreateTV;
    UInt              mDropTV;
    UInt              mSpaceID;
    UInt              mTransID;

    smiTempTableOpr   mTTLastOpr;   /* 마지막으로 실행한 Operation */
    smiTempState      mTTState;

    ULong             mReadCount;       /* Read 회수 */
    ULong             mWriteCount;      /* Write회수 */
    ULong             mWritePageCount;  /* Write한 Page개수  */
    ULong             mOverAllocCount;  /* Extent를 초과 할당한 수   */
    ULong             mAllocWaitCount;  /* Extent 할당 대기한 수   */

    SLong             mExtraStat1;     /* (Sort시, 정렬을 위한 비교회수),
                                        * (Hash시 HashBucket 개수) */
    SLong             mExtraStat2;     /* (Sort시, 마지막 Run의 Slot개수),
                                          (HashCollision 발생 회수 ) */

    SLong             mMaxWorkAreaSize;/*Byte */
    SLong             mUsedWorkAreaSize;
    SLong             mNormalAreaSize; /*Byte*/
    SLong             mRuntimeMemSize; /*Byte*/
    SLong             mRecordCount;
    UInt              mRecordLength;

    UInt              mMergeRunCount;  /* SortTemp의 MergeRun의 개수 */
    UInt              mHeight;         /* SortTemp의 Index의 높이 */

    UInt              mIOPassNo;    /* 0:InMemory, 1:OnePass, 2:.. */
    ULong             mEstimatedOptimalSize; /* InMemorySort 크기 예측 */
    ULong             mEstimatedSubOptimalSize; /* OnePassHash 크기 예측 */

    UChar             mSQLText[ SMI_TT_SQLSTRING_SIZE ];

} smiTempTableStats;

typedef struct smiTempTableIOStats
{
    SLong     mMaxUsedWorkAreaSize;
    ULong     mReadCount;       /* Read 회수 */
    ULong     mWriteCount;      /* Write회수 */
    ULong     mWritePageCount;  /* Write한 Page개수  */

    ULong     mEstimatedOptimalSize; /* InMemoryHash 크기 예측 */
    ULong     mEstimatedSubOptimalSize; /* OnePassHash 크기 예측 */

    UInt      mIOPassNo;    /* 0:InMemory, 1:OnePass, 2:.. */

}smiTempTableIOStats;

/* X$TEMPTABLE_STATS */
/* PerformanceView를 위한 보기 좋은 view로 수정함 */
/* 개괄적인 통계정보 대부분을 출력해줌.
 * 칼럼의 의미는 위 smiTempTableStats와 동일*/
typedef struct smiTempTableStats4Perf
{
    UInt    mSlotIdx;
    UChar   mSQLText[ SMI_TT_SQLSTRING_SIZE ];
    SChar   mCreateTime[SMI_TT_STR_SIZE];
    SChar   mDropTime[SMI_TT_STR_SIZE];
    UInt    mConsumeTime;

    UInt    mSpaceID;
    UInt    mTransID;
    SChar   mLastOpr[SMI_TT_STR_SIZE];

    SChar   mTTState[SMI_TT_STR_SIZE];
    UInt    mIOPassNo;
    ULong   mEstimatedOptimalSize;
    ULong   mEstimatedSubOptimalSize;
    ULong   mReadCount;
    ULong   mWriteCount;
    ULong   mWritePageCount;
    ULong   mOverAllocCount;
    ULong   mAllocWaitCount;

    SLong   mMaxWorkAreaSize;   /*Byte */
    SLong   mUsedWorkAreaSize;   /*Byte */
    SLong   mNormalAreaSize; /*Byte*/
    SLong   mRuntimeMemSize; /*Byte*/
    SLong   mRecordCount;
    UInt    mRecordLength;

    UInt    mMergeRunCount;
    UInt    mHeight;
    SLong   mExtraStat1;
    SLong   mExtraStat2;
} smiTempTableStats4Perf;

/* X$TEMPINFO */
/* Temp관련 전반적인 정보 모두에 대한 PerformanceView */
typedef struct smiTempInfo4Perf
{
    SChar   mName[SMI_TT_STR_SIZE];  /* 정보의 이름 */
    SChar   mValue[SMI_TT_STR_SIZE]; /* 정보의 값 */
    SChar   mUnit[SMI_TT_STR_SIZE];  /* 정보의 단위 */
} smiTempInfo4Perf;

/* X$TEMPINFO에 Name, Value, Unit으로 Record를 등록하는 Macro들 */
#define SMI_TT_SET_TEMPINFO_UINT( name, value, unit )                       \
 idlOS::snprintf( sInfo.mName,  SMI_TT_STR_SIZE, name  );                   \
 idlOS::snprintf( sInfo.mValue, SMI_TT_STR_SIZE, "%"ID_UINT32_FMT, value ); \
 idlOS::snprintf( sInfo.mUnit,  SMI_TT_STR_SIZE, unit  );                   \
 IDE_TEST(iduFixedTable::buildRecord( aHeader, aMemory, (void *)&sInfo )    \
                  != IDE_SUCCESS );

#define SMI_TT_SET_TEMPINFO_ULONG( name, value, unit )                      \
 idlOS::snprintf( sInfo.mName,  SMI_TT_STR_SIZE, name  );                   \
 idlOS::snprintf( sInfo.mValue, SMI_TT_STR_SIZE, "%"ID_UINT64_FMT, value ); \
 idlOS::snprintf( sInfo.mUnit,  SMI_TT_STR_SIZE, unit  );                   \
 IDE_TEST(iduFixedTable::buildRecord( aHeader, aMemory, (void *)&sInfo )    \
                  != IDE_SUCCESS );

#define SMI_TT_SET_TEMPINFO_STR( name, value, unit )                         \
 idlOS::snprintf( sInfo.mName,  SMI_TT_STR_SIZE, name  );                    \
 idlOS::snprintf( sInfo.mValue, SMI_TT_STR_SIZE, value );                    \
 idlOS::snprintf( sInfo.mUnit,  SMI_TT_STR_SIZE, unit  );                    \
 IDE_TEST(iduFixedTable::buildRecord( aHeader, aMemory, (void *)&sInfo )     \
                  != IDE_SUCCESS );

typedef struct smiTempTableHeader
{
    smiTempColumn   * mColumns;
    smiTempColumn   * mKeyColumnList;  /* mColumns를 바탕으로한 KeyColumnList*/

    void            * mWASegment;
    UInt              mColumnCount;
    UInt              mKeyEndOffset;
    smiTempState      mTTState;        /* TempTable의 상태값 */
    UInt              mTTFlag;         /* 이 TempTable을 정의하는 Flag */
    scSpaceID         mSpaceID;        /* NormalExtent를 가져올 TablespaceID*/
    UInt              mHitSequence;    /* clearHitFlag를 위한 HitSequence값*/
    UInt              mWorkGroupRatio; /* 연산 Group의 크기 */
    void            * mTempCursorList; /* TempCursor 목록 */

    /**************************************************************
     * Row관련 정보
     ***************************************************************/
    UInt           mRowSize;
    UInt           mMaxRowPageCount; /*Row하나가 사용한 최대 페이지 건수 */
    UInt           mFetchGroupID;    /*FetchFromGRID에서 사용할 GroupID */
    SLong          mRowCount;

    /* RowPiece 하나를 RowInfo로 가져올때 Row가 쪼개진 경우를 커버하기 위해
     * 있는 버퍼. 기본적으로 RowSize만큼 할당됨 */
    UChar        * mNullRow;
    UChar        * mRowBuffer4Fetch;
    UChar        * mRowBuffer4Compare;
    UChar        * mRowBuffer4CompareSub;


    /**************************************************************
     * 실제 연산시 개별 객체에서 사용하는 자료구조
     ***************************************************************/
    /**************** insert(extract)NSort용 **************/
    /* extractNSort, insrtNSort시 사용되는, 현재 삽입되는 Sort공간*/
    UChar         mSortGroupID;
    smiTempOpenCursor    mOpenCursor;   /* Sort에서만 사용됨, open cursor func */
    /**************** Merge용 **************/
    smuQueueMgr   mRunQueue;      /* Run들의 FirstPageID를 담아두는 Queue*/
    UInt          mMergeRunSize;  /* Run의 크기 */
    UInt          mMergeRunCount; /* Run의 개수 */
    UInt          mLeftBottomPos; /* Heap의 가장 아래 왼쪽 Slot의 위치 */
    iduStackMgr   mSortStack;     /* Sort하는데 사용되는 Stack */
    void        * mInitMergePosition;
    scGRID        mGRID;          /* 마지막으로 접근한 GRID를 저장해둠 */

    /**************** Index용 **************/
    scPageID      mRootWPID;    /* Index의 RootNode, */
    UInt          mHeight;      /* Index의 높이 */
    scPageID      mRowHeadNPID; /* Resort를 대비해, LeafNode의 HeadPage의
                                 * 위치를 저장해둠 */

    /**************** Scan용 **************/
    scPageID    * mScanPosition; /* Run의 FirstPageID를 담아두는 배열 */

    /**************** 통계용 **************/
    UInt                 mCheckCnt;    /* 통계정보 갱신을 자주하지 않기 위한
                                        * 누적값 */
    smiTempTableStats  * mStatsPtr;
    smiTempTableStats    mStatsBuffer; /* 일단 여기에 갱신하다가,
                                        * TEMP_TABLE)WATCH_TIME을 넘어가면
                                        * Array에 등록한다. */

    idvSQL             * mStatistics;
} smiTempTableHeader;

// Temp Table Cursor, 64Byte안에 필요한 정보부터 모은다.
typedef struct smiHashTempCursor
{
    UInt                 mTCFlag;
    UInt                 mHashValue;
    void               * mWASegment;
    UInt                 mHashSlotCount;
    UInt                 mSeq;
    /* 이전에 Fetch한 Row의 ChildGRID임 */
    UChar              * mChildRowPtr;
    scPageID             mChildPageID;
    scOffset             mChildOffset;

    /* Cache. 마지막으로 접근한 페이지에 대한 정보.
     * 같은 페이지의 다음 Slot에 접근할 경우, 이미 WPID, WAPagePtr,
     * SlotCount를 알고 있기 때문에 빠르게 접근 가능함. */
    UChar                mIsInMemory;
    UChar                mSubHashIdx;

    /* WAMap의 Sequence */
    void               * mSubHashPtr;
    void               * mSubHashWCBPtr;
    void               * mSubHashWCBPtr4Fetch;
    UChar              * mRowPtr;
    void               * mHashSlot;
    smiTempTableHeader * mTTHeader;
    UChar              * mWAPagePtr;

    void               * mEndWCBPtr;
    void               * mWCBPtr;

    /* 탐색할 대상 정보 */
    const smiCallBack  * mRowFilter;
    const smiColumnList* mUpdateColumns;
    UInt                 mUpdateEndOffset;

    scPageID             mNExtLstPID;
    void               * mCurNExtentArr;

    smiHashTempCursor  * mNext;         /* CursorList */
} smiHashTempCursor;

typedef struct smiSortTempCursor
{
    void               * mWASegment;
    smiTempTableHeader * mTTHeader;

    /* Row의 위치. Update/SetHitFlag시 이 GRID의 Row를 즉시 갱신함*/
    scGRID               mGRID;

    UInt                 mTCFlag;
    UInt                 mWAGroupID;  /* Fetch시 사용하는 GroupID */
    /* Cache. 마지막으로 접근한 페이지에 대한 정보.
     * 같은 페이지의 다음 Slot에 접근할 경우, 이미 WPID, WAPagePtr,
     * SlotCount를 알고 있기 때문에 빠르게 접근 가능함. */
    UInt                 mSlotCount;
    scPageID             mWPID;
    UChar              * mRowPtr;
    UChar              * mWAPagePtr;
    /* WAMap의 Sequence */
    SInt                 mSeq;

    /* (Scan 할때) row가 위치한 run의 인덱스 */
    UInt                 mPinIdx;
    SInt                 mLastSeq;

    /* 탐색할 대상 정보 */
    UInt                 mUpdateEndOffset;
    const smiColumnList* mUpdateColumns;
    const smiRange     * mRange;
    const smiRange     * mKeyFilter;
    const smiCallBack  * mRowFilter;

    smiTempPosition    * mPositionList; /* 이 Cursor의 PositionList */
    /* run 정보를 저장 */
    void               * mMergePosition;


    /* Module */
    smiTempFetch         mFetch;
    smiTempStoreCursor   mStoreCursor;
    smiTempRestoreCursor mRestoreCursor;
    smiSortTempCursor  * mNext;         /* CursorList */

} smiSortTempCursor;

typedef struct smiTempPosition
{
    smiSortTempCursor  * mOwner;
    smiTempPosition    * mNext;

    /* MergeScan시 Position을 저장하기 위해서는, 각 MergeRun의 정보를
     * 모두 저장해야 함. 하지만 MergeRun이 몇개인지 모르기 때문에
     * 해당 메모리를 malloc하여 여기에 달아둠 */
    void               * mExtraInfo;

        /* Row의 위치. Update/SetHitFlag시 이 GRID의 Row를 즉시 갱신함*/
    scGRID               mGRID;
    /* Cache. 마지막으로 접근한 페이지에 대한 정보.
     * 같은 페이지의 다음 Slot에 접근할 경우, 이미 WPID, WAPagePtr,
     * SlotCount를 알고 있기 때문에 빠르게 접근 가능함. */
    UInt                 mSlotCount;
    scPageID             mWPID;
    /* WAMap의 Sequence */
    UInt                 mSeq;
    smiTempState         mTTState;

    UChar              * mRowPtr;
    UChar              * mWAPagePtr;

    /* (Scan 할때) row가 위치한 run의 인덱스 */
    UInt                 mPinIdx;

} smiTempPosition;

typedef enum
{
    SMI_BEFORE_LOCK_RELEASE,
    SMI_AFTER_LOCK_RELEASE
} smiCallOrderInCommitFunc;

/* PROJ-2365 sequence table
 * replication이 가능하도록 sequence를 table로 부터 얻는다.
 * sequence.nextval을 위한 callback function
 */
typedef IDE_RC (*smiSelectCurrValFunc)( SLong * aCurrVal,
                                        void  * aInfo );
typedef IDE_RC (*smiUpdateLastValFunc)( SLong   aLastVal,
                                        void  * aInfo );

struct smiSeqTableCallBack {
    void                 * info;           /* qdsCallBackInfo */
    smiSelectCurrValFunc   selectCurrVal;  /* open cursor, select row */
    smiUpdateLastValFunc   updateLastVal;  /* update row, close cursor */
    UInt                   scale;          /* for Sharded sequence */
};

typedef enum
{
    SMI_DTX_NONE = 0,
    SMI_DTX_PREPARE,
    SMI_DTX_COMMIT,
    SMI_DTX_ROLLBACK,
    SMI_DTX_END
} smiDtxLogType;

/* dk 에서 MinLSN 을 구하기 위해 사용한다 */
#define SMI_LSN_MAX(aLSN)                           \
{                                                   \
    (aLSN).mFileNo = (aLSN).mOffset = ID_UINT_MAX;  \
}
#define SMI_LSN_INIT(aLSN)                          \
{                                                   \
   (aLSN).mFileNo = (aLSN).mOffset = 0;             \
}

#define SMI_IS_LSN_INIT(aLSN)                       \
    (((aLSN).mFileNo == 0) && ((aLSN).mOffset == 0))

/* BUG-37778 qp 에서 disk hash temp table 사이즈를 예측하기 위해서 사용한다 */
/* XXX PROJ-2647 로 plan이 변경되어서 임시로 이전값을 넣었다.
 * 이후에 PLAN을 수정하면서 같이 수정 하여야 한다. */
#define SMI_TR_HEADER_SIZE_FULL     (32)  //  SDT_TR_HEADER_SIZE_FULL
#define SMI_WAEXTENT_PAGECOUNT      SDT_WAEXTENT_PAGECOUNT
#define SMI_WAMAP_SLOT_MAX_SIZE     SDT_WAMAP_SLOT_MAX_SIZE

/* BUG-41765 Remove warining in smiLob.cpp */
#define SMI_LOB_LOCATOR_CLIENT_MASK        (0x00000004)
#define SMI_LOB_LOCATOR_CLIENT_TRUE        (0x00000004)
#define SMI_LOB_LOCATOR_CLIENT_FALSE       (0x00000000)

/* BUG-43408, BUG-45368 */
#define SMI_ITERATOR_SIZE                  (50 * 1024)


/* session정보를 얻기 위한 mmcSession의 callback 함수들의 집합.
 * BUG-47655 session의 transaction 할당 재시도 횟수를 설정하기 위한 Callback 함수 */
typedef struct smiSessionCallback
{
    void (*mSetAllocTransRetryCount)( void   * aSession,
                                      ULong    aRetryCount );
  
    UInt (*mGetIndoubtFetchTimeout)( void * aMmSession ); 
    UInt (*mGetIndoubtFetchMethod)( void * aMmSession );

} smiSessionCallback;

/* PROJ-2735 DDL Transaction */
typedef struct smiDDLTargetTableInfo
{
    UInt           mTableID;
    void         * mOldTableInfo;
    void         * mNewTableInfo;
    idBool         mIsReCreated;
    iduList        mPartInfoList;
    iduListNode    mNode;
} smiDDLTargetTableInfo;

typedef struct  smiTransactionalDDLCallback
{
    IDE_RC (*backupDDLTargetOldTableInfo)( smiTrans               * aTrans, 
                                           smOID                    aTableOID,
                                           UInt                     aPartOIDCount,
                                           smOID                  * aPartOIDArray,
                                           smiDDLTargetTableInfo ** aDDLTargetTableInfo );

    IDE_RC (*backupDDLTargetNewTableInfo)( smiTrans               * aTrans, 
                                           smOID                    aTableOID,
                                           UInt                     aPartOIDCount,
                                           smOID                  * aPartOIDArray,
                                           smiDDLTargetTableInfo ** aDDLTargetTableInfo );

    void   (*removeDDLTargetTableInfo)( smiTrans * aTrans, smiDDLTargetTableInfo * aDDLTargetTableInfo );

    void   (*restoreDDLTargetOldTableInfo)( smiDDLTargetTableInfo * aDDLTargetTableInfo );

    void   (*destroyDDLTargetNewTableInfo)( smiDDLTargetTableInfo * aDDLTargetTableInfo );
} smiTrasactionalDDLCallback;

/*
   PROJ-2734
   샤드핀 관련 정의

   주의!!!
   아래 샤드핀 관련 정의는sdi.h 파일의 sdiShardPin 관련 정의와 일치하여야 한다.
 */
typedef ULong  smiShardPin;

#define SMI_SHARD_PIN_INVALID (0)

typedef enum
{
    SMI_OFFSET_VERSION      = 56,   /* << 7 byte, sMmSharMPinInfo.mVersion                  */
    SMI_OFFSET_RESERVED     = 48,   /* << 6 byte, sMmSharMPinInfo.mReserveM                 */
    SMI_OFFSET_META_NODE_ID = 32,   /* << 4 byte, sMmSharMPinInfo.mMetaNoMeInfo.mMetaNoMeIM */
    SMI_OFFSET_SEQEUNCE     = 0,    /* << 0 byte, sMmSharMPinInfo.mSeq                      */
} smihardPinFactorOffset;

#define SMI_MAX_SHARD_PIN_STR_LEN              (20) /* Without Null terminated */
#define SMI_SHARD_PIN_FORMAT_STR   "%"ID_UINT32_FMT"-%"ID_UINT32_FMT"-%"ID_UINT32_FMT
#define SMI_SHARD_PIN_FORMAT_ARG( __SHARD_PIN ) \
    ( __SHARD_PIN & ( (smiShardPin)0xff       << SMI_OFFSET_VERSION      ) ) >> SMI_OFFSET_VERSION, \
    ( __SHARD_PIN & ( (smiShardPin)0xffff     << SMI_OFFSET_META_NODE_ID ) ) >> SMI_OFFSET_META_NODE_ID, \
    ( __SHARD_PIN & ( (smiShardPin)0xffffffff << SMI_OFFSET_SEQEUNCE     ) ) >> SMI_OFFSET_SEQEUNCE

inline void SMI_DIVIDE_SHARD_PIN( smiShardPin aShardPIN,
                                  UChar *aVersion,
                                  UShort *aNodeId,
                                  UInt *aSequence )
{
    if ( aVersion != NULL )
    {
        *aVersion  = ( aShardPIN & ( (smiShardPin)0xff       << SMI_OFFSET_VERSION      ) ) >> SMI_OFFSET_VERSION;
    }

    if ( aNodeId != NULL )
    {
        *aNodeId   = ( aShardPIN & ( (smiShardPin)0xffff     << SMI_OFFSET_META_NODE_ID ) ) >> SMI_OFFSET_META_NODE_ID;
    }

    if ( aSequence != NULL )
    {
        *aSequence = ( aShardPIN & ( (smiShardPin)0xffffffff << SMI_OFFSET_SEQEUNCE     ) ) >> SMI_OFFSET_SEQEUNCE;
    }
}

/* PROJ-2734 
   분산레벨
   레벨 SINGLE   : 하나의 노드에서만 Statement가 수행되었음
   레벨 MULTI    : 여러 노드에서 Statement가 수행되었으나, 동시간에 하나의 노드에서만 수행.
   레벨 PARALLEL : 동시에 여러 노드에서 Statement가 수행되었음 */
typedef enum
{
    SMI_DIST_LEVEL_INIT = 0,
    SMI_DIST_LEVEL_SINGLE,
    SMI_DIST_LEVEL_MULTI,
    SMI_DIST_LEVEL_PARALLEL,
    SMI_DIST_LEVEL_MAX
} smiDistLevel;

/* 유효한 분산레벨인지 확인한다.
   (분산레벨이 유효한 경우, 분산정보가 있음을 나타낸다.) */
#define SMI_DIST_LEVEL_IS_VALID( aLevel )     \
    ( ( aLevel == SMI_DIST_LEVEL_SINGLE ) ||  \
      ( aLevel == SMI_DIST_LEVEL_MULTI )  ||  \
      ( aLevel == SMI_DIST_LEVEL_PARALLEL ) )
#define SMI_DIST_LEVEL_IS_NOT_VALID( aLevel ) \
    ( ( aLevel != SMI_DIST_LEVEL_SINGLE ) &&  \
      ( aLevel != SMI_DIST_LEVEL_MULTI )  &&  \
      ( aLevel != SMI_DIST_LEVEL_PARALLEL ) )

typedef struct smiDistTxInfo
{
    smSCN              mFirstStmtViewSCN;
    PDL_Time_Value     mFirstStmtTime;
    smiShardPin        mShardPin;
    smiDistLevel       mDistLevel;
} smiDistTxInfo;

#define SMI_SET_SMI_DIST_TX_INFO( aTargetDistTxInfo, aFirstStmtViewSCN, aFirstStmtTime, aShardPin, aDistLevel ) \
{ \
    (aTargetDistTxInfo)->mFirstStmtViewSCN = aFirstStmtViewSCN; \
    (aTargetDistTxInfo)->mFirstStmtTime    = aFirstStmtTime; \
    (aTargetDistTxInfo)->mShardPin         = aShardPin; \
    (aTargetDistTxInfo)->mDistLevel        = aDistLevel; \
}

#define SMI_MAX_ERR_MSG_LEN  (256)
#define SMI_XID_STRING_LEN   (256)  /* XID_DATA_MAX_LEN 참조 */

#endif /* _O_SMI_DEF_H_ */
