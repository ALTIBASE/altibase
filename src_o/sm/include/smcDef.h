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
 * $Id: smcDef.h 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#ifndef _O_SMC_DEF_H_
#define _O_SMC_DEF_H_ 1

#include <smDef.h>
#include <smmDef.h>
#include <smpDef.h>
#include <sdpDef.h>
#include <sdcDef.h>

/* Memory Table의 State정보 증가시 사용하는 Index */
typedef enum
{
    SMC_INC_UNIQUE_VIOLATION_CNT, /* Unique Index Violation */
    SMC_INC_RETRY_CNT_OF_LOCKROW, /* Lock   Row Retry */
    SMC_INC_RETRY_CNT_OF_UPDATE,  /* Update Row Retry */
    SMC_INC_RETRY_CNT_OF_DELETE   /* Delete Row Retry */
} smcTblStatType;

struct smcTableHeader;

#define SMC_DROP_INDEX_LIST_POOL_SIZE  (128)
#define SMC_RESERV_SIZE (2)

// PROJ-1362 QP - Large Record & Internal LOB 지원.
// 에서 인덱스 제약을  17개에서 64개로 확장하기 위하여
// smcTableHeader의 mIndex를 array로 처리한다.
#define SMC_MAX_INDEX_OID_CNT          (4)

#define SMC_CAT_TABLE ((smcTableHeader *)(smmManager::m_catTableHeader))
#define SMC_CAT_TEMPTABLE ((smcTableHeader *)(smmManager::m_catTempTableHeader))

typedef smiObjectType smcObjectType;

/* ----------------------------------------------------------------------------
 *    validate table
 * --------------------------------------------------------------------------*/
#define SMC_LOCK_MASK     (0x00000010)
#define SMC_LOCK_TABLE    (0x00000000)
#define SMC_LOCK_MUTEX    (0x00000010)

/* PROJ-2399 row template
 * column의 길이를 미리 알 수 없는 데이터타입인경우(ex varchar,nibble등)
 * 해당 column의 길이를 ID_USHORT_MAX로 설정하기 위한 매크로이다. */
#define SMC_UNDEFINED_COLUMN_LEN (ID_USHORT_MAX)

/* ----------------------------------------------------------------------------
 *    Sequence
 * --------------------------------------------------------------------------*/
#define SMC_INIT_SEQUENCE (ID_LONG(0x8000000000000000))

/***********************************************************************
 * Description : PROJ-2419. MVCC로 DML(Insert, Delete, Update) 로그에서 United Variable Column에대한
 *               Log의 Log Size를 구한다. United Variable Log는 다음과 같이 구성된다.
 *
 *  하나로 합쳐진 united variable Column에 대해서 
 *  Var Log : Column Count(UInt), Column ID(1 ... n ) | LENGTH(UInt) | Fst Piece OID, Value 
 *
 *  aLength - [IN] Variable Column 길이
 *  aCount  - [IN] united variable column 에 들어간 column의 숫자
 *  
 * < 로그 구성 >
 *  First Piece OID ( smOID )
 * + Column Count   ( UShort )
 * + Column ID List ( SizeOF(Uint)*aColCount )
 * + pieceCount * (  Next Piece OID ( smOID )
 *                 + Column Count each Piece  ( UShort ))
 * + aLength : Value of All Piece
 ***********************************************************************/
#define SMC_UNITED_VC_LOG_SIZE(aLength, aColCount, aPieceCount)   ( ID_SIZEOF(smOID) \
                                                                  + ID_SIZEOF(UShort) \
                                                                  + (ID_SIZEOF(UInt) * aColCount) \
                                                                  + aPieceCount * ( ID_SIZEOF(smOID) + ID_SIZEOF(UShort)) \
                                                                  + aLength )

/* BUG-43604 여러 Variable Column들을 UnitedVar 형태로  합칠때
 * 각 Column 사이의 Padding 값을 추정한다.
 * NULL 데이터의 경우 Column의 Align이 아닌 MaxAlign 값을 가지고 추정하여야 한다. */
#define SMC_GET_COLUMN_PAD_LENGTH(aValue, aColumn) ( ( aValue.length  > 0 ) ? \
                                                     ( aColumn->align - 1 ) : \
                                                     ( aColumn->maxAlign - 1 ) ) 

/*PROJ-2399 row template */
typedef struct smcColTemplate                                                                             
{        
    /* 저장되는 column의 길이, variable column인 경우
     * SMC_UNDEFINED_COLUMN_LEN이 저장된다. */ 
    UShort   mStoredSize;
    /* column의 길이 정보를 저장하는 공간 */
    UShort   mColLenStoreSize;
    /* 해당 column 앞에 존재하는 가장가까운 variable column의
     * mColStartOffset 값을 저장하고있는 위치를 가리킨다.*/
    UShort   mVariableColIdx; 
    /* 한 row에서 column이 위치하는 offset 
     * row가 한 rowpiece에 저장되었을 경우의 offset이다.
     * row가 여러 row piece로 나누어졌을경우 보정 된다. */
    SShort   mColStartOffset;
}smcColTemplate;

typedef struct smcRowTemplate
{            
    /* row를 구성하는 column들의 정보 */
    smcColTemplate * mColTemplate;
    /* row를 구성하는 variable column들의 mColStartOffset
     * 정보이다. fetch하려는 column의 앞에 존재하는 non variable column들
     * 길이의 합 정보로 보면 된다. */
    UShort         * mVariableColOffset; 
    /* table의 총 column 수 */
    UShort           mTotalColCount;
    /* row를 구성하는 variable column의 수 */
    UShort           mVariableColCount; 
}smcRowTemplate; 

/* BUG-31206 improve usability of DUMPCI and DUMPDDF
 * smcTable::dumpTableHeaderByBuffer와 같이 고쳐야 함 */
typedef enum
{
    SMC_TABLE_NORMAL = 0,
    SMC_TABLE_TEMP,
    SMC_TABLE_CATALOG,
    SMC_TABLE_SEQUENCE,
    SMC_TABLE_OBJECT
} smcTableType;


typedef struct smcSequenceInfo
{
    SLong   mCurSequence;
    SLong   mStartSequence;
    SLong   mIncSequence;
    SLong   mSyncInterval;
    SLong   mMaxSequence;
    SLong   mMinSequence;
    SLong   mLstSyncSequence;
    UInt    mFlag;
    UInt    mAlign;
} smcSequenceInfo;

# define SMC_INDEX_TYPE_COUNT       (1024)

/* ------------------------------------------------
 * For A4 :
 * disk/memory 테이블의 공통 header
 *
 * disk table의 경우 fixed/variable column을 구분하지 않고
 * 저장하지 않고, memory table과는 달리 disk table은 page list
 * entry를 fixed와 variable로 구분할 필요가 없다.
 * ----------------------------------------------*/
typedef struct smcTableHeader
{
    void             * mLock;    /* for normal table lock,
                                    smlLockItem */

    scSpaceID         mSpaceID;                                         

    /* ------------------------------------------------
     * FOR A4 : table type에 따른 main entry
     * sdpPageListEntry를 별도로 정의
     *
     * 공통 table header에서는 page list entry를 테이블 type에
     * 따라 union으로 처리하고, disk type의 table entry인
     * sdpPageListEntry 정의
     * ----------------------------------------------*/
    union
    {
        sdpPageListEntry  mDRDB;   /* disk page list entry */ 
        smpPageListEntry  mMRDB;   /* memory page list entry */
        smpPageListEntry  mVRDB;   /* volatile page list entry */
    } mFixed;

    // fixed memory page list에 해당하는 AllocPageList
    union
    {
        smpAllocPageListEntry mMRDB[SM_MAX_PAGELIST_COUNT];
        smpAllocPageListEntry mVRDB[SM_MAX_PAGELIST_COUNT];
    } mFixedAllocList;

    /* ------------------------------------------------
     * FOR A4 : disk table에서는 사용하지 않음
     * 왜냐하면 variable column도 fixed page list에서
     * 모두 저장하기 때문에 memory table에서 사용하는
     * variable column을 위한 page list는 필요없다.
     * ----------------------------------------------*/
    union
    {
        smpPageListEntry  mMRDB[SM_VAR_PAGE_LIST_COUNT];
        smpPageListEntry  mVRDB[SM_VAR_PAGE_LIST_COUNT];
    } mVar;

    /* BUG-13850 : var page list에서 AllocPageList가 다중화되어
                   table header 크기가 커져서 한 page를 넘게 되어
                   AllocPageList를 page size에 상관없이 공유하여 사용한다.*/
    union
    {
        smpAllocPageListEntry mMRDB[SM_MAX_PAGELIST_COUNT];
        smpAllocPageListEntry mVRDB[SM_MAX_PAGELIST_COUNT];
    } mVarAllocList;

    smcTableType       mType;    /* table 타입  */
    smcObjectType      mObjType; /* object 타입 */                        
    smOID              mSelfOID; /* table header가 기록된 slot header를 가리킴 */

    smOID              mNullOID;

    /* Table Column정보 */
    UInt               mColumnSize;     /* column 크기 */
    UInt               mColumnCount;    /* column 개수 */
    UInt               mLobColumnCount; /* lob column 갯수 */
    UInt               mUnitedVarColumnCount; /* United Variable column 갯수 */
    smVCDesc           mColumns;        /* column이 저장된 Variable Column */

    smVCDesc           mInfo;           /* info가 저장된 Varaiable Column */

    /* for sequence */
    smcSequenceInfo    mSequence;

    /* index management */
    smVCDesc           mIndexes[SMC_MAX_INDEX_OID_CNT]; /* list of created index */
    void             * mDropIndexLst; /* smnIndexHeader */
    UInt               mDropIndex;    /* dropindex list에 추가된 index 개수 */

    /* PROJ-1665
       - mFlag : table type 정보와 logging 여부 정보
                 table type 정보 ( SMI_TABLE_TYPE_MASK )
                 logging/nologging 정보 ( SMI_INSERT_APPEND_LOGGING_MASK )
       - mParallelDegree  : parallel degree */
    UInt               mFlag;
    UInt               mParallelDegree;

    idBool             mIsConsistent; /* PROJ-2162 */

    /* To Fix BUG-17371  Aging이 밀릴경우 System에 과부하 및 Aging이
                         밀리는 현상이 더 심화됨.

       Ager와 DDL Transaction이 각각 1개씩만 존재했던 상황에서는
       iduSyncWord 만으로도 동시성 제어가 가능했었다.

       이제는 여러 개의 Ager가 동시에 수행될 수 있는 상황이다.
       iduSyncWord대신 iduLatch를 사용한다.
    */
    /* N개의 Ager와 1개의 DDL이 경합을 벌이는 RW Latch */
    iduLatch         * mIndexLatch ;

    ULong              mMaxRow;           /* max row */
    void             * mRuntimeInfo;      /* PROJ-1597 Temp record size 제약 제거
                                             예전 mTempInfo도 이 멤버를 사용한다
                                             각종 runtime 정보를 위해 이 맴버를 사용한다. */

    /* TASK-4990 changing the method of collecting index statistics 
     * 여기서 저장되는 RowCount 등은 통계정보 수집시 기록된 것으로 실제 값과는
     * 차이가 있다. */
    smiTableStat       mStat;

    /* BUG-48588  table drop 확인용도. 테이블 생성 후 갱신없음 */
    smSCN              mTableCreateSCN;

    smcRowTemplate   * mRowTemplate;                   

    /* BUG-42928 No Partition Lock */
    void             * mReplacementLock;

    /* PROJ-1497 DB Migration을 방지하기 위해 Reserved 영역을 추가 */
    ULong              mReserveArea[SMC_RESERV_SIZE];
} smcTableHeader;

/* BUG-42928 No Partition Lock */
#define SMC_TABLE_LOCK( tableHeader ) ( ( (tableHeader)->mReplacementLock != NULL ) ?   \
                                          (tableHeader)->mReplacementLock :             \
                                          (tableHeader)->mLock )

/* ------------------------------------------------
 * temp table을 위한 catalog table의 offset을 나타낸다.
 * system 0번 page의 구성은 다음과 같다.
 *   ____________
 *   | membase  |
 *   |__________|
 *   |#         |--> catalog table for normal table
 *   |__________|
 *   |#         |--> catalog table for temp table
 *   |__________|
 *    ....
 * ----------------------------------------------*/
#define SMC_CAT_TEMPTABLE_OFFSET SMM_CACHE_ALIGN(SMM_CAT_TABLE_OFFSET              \
                                                 + ID_SIZEOF(struct smpSlotHeader) \
                                                 + ID_SIZEOF(struct smcTableHeader))

typedef struct smcTBSOnlineActionArgs
{
    void  * mTrans;
    ULong   mMaxSmoNo;
} smcTBSOnlineActionArgs;

// added for performance view.
typedef struct  smcTableInfoPerfV
{
    scSpaceID    mSpaceID;
    UInt         mType;
    //for memory  table.
    smOID        mTableOID;
    vULong       mMemPageCnt;
    ULong        mMemVarPageCnt;
    UInt         mMemSlotCnt;
    vULong       mMemSlotSize;
    vULong       mFixedUsedMem;
    vULong       mVarUsedMem;
    scPageID     mMemPageHead;

    //for disk  table
    ULong        mDiskTotalPageCnt;
    ULong        mDiskPageCnt;
    scPageID     mTableSegPID;

    scPageID     mTableMetaPage;
    sdRID        mFstExtRID;
    sdRID        mLstExtRID;
    UShort       mDiskPctFree;           // PCTFREE
    UShort       mDiskPctUsed;           // PCTUSED
    UShort       mDiskInitTrans;         // INITRANS ..
    UShort       mDiskMaxTrans;          // MAXTRANS ..
    UInt         mDiskInitExtents;       // Storage InitExtents 
    UInt         mDiskNextExtents;       // Storage NextExtents 
    UInt         mDiskMinExtents;        // Storage MinExtents 
    UInt         mDiskMaxExtents;        // Storage MaxExtents 

    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부하 및 Aging이
    //밀리는 현상이 더 심화됨.
    ULong       mOldVersionCount;       // Old Version개수
    ULong       mStatementRebuildCount; // Statement Rebuild Count
    ULong       mUniqueViolationCount;  // Unique Violation Count
    ULong       mUpdateRetryCount;      // UpdateRow시 AlreadyModifiedError로 인한 Retry횟수
    ULong       mDeleteRetryCount;      // DeleteRow시 AlreadyModifiedError로 인한 Retry횟수
    ULong       mLockRowRetryCount;     // LockRow시 AlreadyModifiedError로 인한 Retry횟수

    // TASK-2398 Log Compress
    //   로그 압축 여부 ( 1: 압축, 0: 압축안함 )
    UInt        mCompressedLogging;

    idBool      mIsConsistent; /* PROJ-2162 */
}smcTableInfoPerfV;

typedef struct  smcSequence4PerfV
{
    smOID        mSeqOID;
    smcSequenceInfo   mSequence;
}smcSequence4PerfV;

// PROJ-1362 QP - Large Record & Internal LOB 지원.
// drop table pending시 사용하게 될 mColumns OID stack.
typedef struct smcOIDStack
{
    UInt   mLength;
    smOID* mArr;
}smcOIDStack;

/* Variable Column Log기록시 After인지 Before 인지?*/
typedef enum
{
    SMC_VC_LOG_WRITE_TYPE_NONE = 0,
    SMC_VC_LOG_WRITE_TYPE_AFTERIMG,
    SMC_VC_LOG_WRITE_TYPE_BEFORIMG
} smcVCLogWrtOpt;

/* Update Inplace이 Log기록시 After인지 Before 인지?*/
typedef enum
{
    SMC_UI_LOG_WRITE_TYPE_NONE = 0,
    SMC_UI_LOG_WRITE_TYPE_AFTERIMG,
    SMC_UI_LOG_WRITE_TYPE_BEFORIMG
} smcUILogWrtOpt;

/* smcRecordUpdate::makeLogFlag의 인자 flag */
/* BUG-14513 */
typedef enum
{
    SMC_MKLOGFLAG_NONE = 0,
    // Log Flag에 SMR_LOG_ALLOC_FIXEDSLOT_OK을 원하면.
    SMC_MKLOGFLAG_SET_ALLOC_FIXED_OK   = 0x00000001,
    // Log Flag에 SMR_LOG_ALLOC_FIXEDSLOT_NO을 원하면.
    SMC_MKLOGFLAG_SET_ALLOC_FIXED_NO   = 0x00000002,
    // Replication Sender가 LOG를 Skip하도록한다.
    SMC_MKLOGFLAG_REPL_SKIP_LOG        = 0x00000004
} smcMakeLogFlagOpt;

/* 현재 Log를 Replication Sender가 읽을것인지? */
typedef enum
{
    SMC_LOG_REPL_SENDER_SEND = 0,
    SMC_LOG_REPL_SENDER_SEND_OK = 1,
    SMC_LOG_REPL_SENDER_SEND_NO = 2
} smcLogReplOpt;

/* smc 관련 Function Argument */
#define SMC_WRITE_LOG_OK  (0x00000001)
#define SMC_WRITE_LOG_NO  (0x00000000)

typedef struct smcCatalogInfoPerfV
{
    smOID        mTableOID;
    UInt         mColumnCnt;
    UInt         mColumnVarSlotCnt;
    UInt         mIndexCnt;
    UInt         mIndexVarSlotCnt;
}smcCatalogInfoPerfV;

/******************************************************************
 *
 * PRJ-1362 MEMORY LOB
 *               ------------------------------
 * FixedRow         ... | smcLobDesc | ...
 *               ------------------------------
 * LobPieceControlHeader   | ㄴ[smcLPCH|smcLPCH|smcLPCH|smcLPCH]
 *                         |       |       |       |       |
 * LobPiece                ㄴ [varslot][varslot][varslot][varslot]
 *
 ******************************************************************/

 // Max In Mode LOB Size
#define  SMC_LOB_MAX_IN_ROW_STORE_SIZE   ( SM_LOB_MAX_IN_ROW_SIZE + ID_SIZEOF(smVCDescInMode) )

/* MEMORY LOB PIECE CONTROL HEADER */
typedef struct smcLPCH
{
    ULong        mLobVersion;
    smOID        mOID;         // Lob 데이타 piece의 OID
    void        *mPtr;         // Lob 데이타 piece의 실제 pointer (varslot)
}smcLPCH;

/* MEMORY LOB COLUMN DESCRIPTOR */
typedef struct smcLobDesc
{
    /* smVCDesc 구조체와 공유한다. --------------------------------- */
    UInt           flag;        // variable column의 속성 (In, Out)
    UInt           length;      // variable column의 데이타 길이
    smOID          fstPieceOID; // variable column의 첫번째 piece oid

    /* ------------------------------------------------------------- */
    UInt           mLPCHCount;  // Lob 데이타 piece 개수
    ULong          mLobVersion; // Lob Version
    smcLPCH       *mFirstLPCH;  // 첫번째 LPCH(Lob Piece Control Header)
}smcLobDesc;

#define SMC_LOB_DESC_INIT( aLobDesc ) \
{   \
        aLobDesc->flag        = SM_VCDESC_MODE_IN; \
        aLobDesc->length      = 0; \
        aLobDesc->fstPieceOID = SM_NULL_OID; \
        aLobDesc->mLPCHCount  = 0; \
        aLobDesc->mLobVersion = 0; \
        aLobDesc->mFirstLPCH  = NULL; \
}

// lob column이 table에서 몇번째 column인지를 나타냄.
typedef struct smcLobColIdxNode
{
    UInt       mColIdx;
    smuList    mListNode;
}smcLobColIdxNode;

typedef IDE_RC  (*smcFreeLobSegFunc)( idvSQL           *aStatistics,
                                      void*             aTrans,
                                      scSpaceID         aColSlotSpaceID,
                                      smOID             aColSlotOID,
                                      UInt              aColumnSize,
                                      sdrMtxLogMode     aLoggingMode );
typedef enum
{
    SMC_UPDATE_BY_TABLECURSOR,
    SMC_UPDATE_BY_LOBCURSOR
} smcUpdateOpt;


// BUG-20048
#define SM_TABLE_BACKUP_STR            "SM_TABLE_BACKUP"
#define SM_VOLTABLE_BACKUP_STR         "SM_VOLTABLE_BACKUP"
#define SM_TABLE_BACKUP_TYPE_SIZE      (40)

/* Table Backup File Header */
typedef struct smcBackupFileHeader
{
    // BUG-20048
    SChar           mType[SM_TABLE_BACKUP_TYPE_SIZE];
    SChar           mProductSignature[IDU_SYSTEM_INFO_LENGTH];
    SChar           mDbname[SM_MAX_DB_NAME]; // DB Name
    UInt            mVersionID;              // Altibase Version
    UInt            mCompileBit;             // Support Mode "32" or '"64"
    vULong          mLogSize;                // Logfile Size
    smOID           mTableOID;               // Table OID
    idBool          mBigEndian;              // big = ID_TRUE

    // PROJ-1579 NCHAR
    SChar           mDBCharSet[IDN_MAX_CHAR_SET_LEN];
    SChar           mNationalCharSet[IDN_MAX_CHAR_SET_LEN];
} smcBackupFileHeader;

typedef struct smcBackupFileTail
{
    smOID           mTableOID;               // Table OID
    ULong           mSize;                   // File Size
    UInt            mMaxRowSize;             // Max Row Size
} smcBackupFileTail;

typedef struct smcTmsCacheInfoPerfV
{
    scSpaceID      mSpaceID;           // TBS_ID
    scPageID       mSegPID;
    sdpHintPosInfo mHintPosInfo;
} smcTmsCacheInfoPerfV;

/* 
 * BUG-22677 DRDB에서create table중 죽을때 recovery가 안되는 경우가 있음. 
 */
#define SMC_IS_VALID_COLUMNINFO(fstPieceOID)  ( fstPieceOID  != SM_NULL_OID )

typedef IDE_RC (*smcAction4TBL)( idvSQL         * aStatistics,
                                 smcTableHeader * aHeader,
                                 void           * aActionArg );
typedef struct smcLobStatistics
{
    ULong  mOpen;
    ULong  mRead;
    ULong  mWrite;
    ULong  mErase;
    ULong  mTrim;
    ULong  mPrepareForWrite;
    ULong  mFinishWrite;
    ULong  mGetLobInfo;
    ULong  mClose;
} smcLobStatistics;

#endif /* _O_SMC_DEF_H_ */
