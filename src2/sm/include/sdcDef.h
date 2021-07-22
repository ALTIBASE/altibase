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
 * $Id: sdcDef.h 88545 2020-09-10 09:14:02Z emlee $
 **********************************************************************/
#ifndef  _O_SDC_DEF_H_
#define  _O_SDC_DEF_H_  1

#include <smDef.h>
#include <sdp.h>
#include <smrDef.h>
#include <sdbDef.h>


#define SDC_MOVE_PTR_TRIPLE(a1stPtr, a2ndPtr, a3rdPtr, aSize)   \
    IDE_DASSERT((a1stPtr) != NULL);                             \
    IDE_DASSERT((a2ndPtr) != NULL);                             \
    IDE_DASSERT((a3rdPtr) != NULL);                             \
    IDE_DASSERT((aSize) > 0);                                   \
    IDE_DASSERT( ID_SIZEOF(*(a1stPtr)) == 1 );                  \
    IDE_DASSERT( ID_SIZEOF(*(a2ndPtr)) == 1 );                  \
    IDE_DASSERT( ID_SIZEOF(*(a3rdPtr)) == 1 );                  \
    (a1stPtr) += (aSize);                                       \
    (a2ndPtr) += (aSize);                                       \
    (a3rdPtr) += (aSize)


typedef enum sdcTSState
{
    SDC_TSS_STATE_ACTIVE,   // TSS가 트랜잭션에 Binding된 상태
    SDC_TSS_STATE_COMMIT,   // 트랜잭션이 커밋한 상태
    SDC_TSS_STATE_ABORT     // 트랜잭션이 롤백한 상태
} sdcTSState;

/**********************************************************************
 * undo record의 타입 정의
 **********************************************************************/
# define SDC_UNDO_INSERT_ROW_PIECE             (0x00)
# define SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE  (0x01)
# define SDC_UNDO_UPDATE_ROW_PIECE             (0x02)
# define SDC_UNDO_OVERWRITE_ROW_PIECE          (0x03)
# define SDC_UNDO_CHANGE_ROW_PIECE_LINK        (0x04)
# define SDC_UNDO_DELETE_FIRST_COLUMN_PIECE    (0x05)
# define SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE  (0x06)
# define SDC_UNDO_DELETE_ROW_PIECE             (0x07)
# define SDC_UNDO_LOCK_ROW                     (0x08)
# define SDC_UNDO_INDEX_CTS                    (0x09) // PROJ-1704 Disk MVCC Renewal
# define SDC_UNDO_UPDATE_LOB_LEAF_KEY          (0x0a) // PROJ-2047 Strengthening LOB

/**********************************************************************
 * Transaction Status Slot 정의
 *
 * TSS를 소유한 트랜잭션의 상태 및 CommitSCN을 다른 트랜잭션이 확인할 수 있다.
 * 다른 트랜잭션이 Row Time-Stamping 수행시 TSS로부터 판독한 CommitSCN을
 * 관련 있는 Row Piece Header들에 설정한다.
 * 트랜잭션이 할당했을 때에는 Infinite SCN(0x8000000000000000)이 설정되며,
 * Commit시에는 CommitSCN, Rollback시에는 Init SCN(0x0000000000000000)이
 * 설정된다.
 * 만약 Commit이후에 CommitSCN을 설정하지 못한 경우에는 서버가 비정상 종료한
 * 경우 서버 Restart Recovery 과정에서 Commit Log에 의해서 Init SCN
 * (0x0000000000000000)을 설정하여 다른 트랜잭션이 갱신하거나 볼수 있게 한다.
 **********************************************************************/
typedef struct sdcTSS
{
    smTID             mTransID;      // 트랜잭션 ID
    sdcTSState        mState;        // Active/Commit/Rollback
    smSCN             mCommitSCN;    // InfiniteSCN 혹은 CommitSCN
} sdcTSS;

/**********************************************************************
 * TSS Page Control Header
 *
 * TSS 페이지를 할당한 혹은 바로 직전에 사용했던 트랜잭션의 정보를 기록하여 TSS의
 * 재사용여부를 판단할 수 있도록 한다.
 *
 * TSSPage.TxBeginSCN > Page의 Commit 되지 않은 CTS.TxBeginSCN
 * 을 만족하면 재사용된 것으로 판단할 수 있다.
 **********************************************************************/
typedef struct sdcTSSPageCntlHdr
{
    smTID       mTransID;       // Undo Record를 기록한 트랜잭션 ID
    smSCN       mFstDskViewSCN; // TSS를 할당해간 트랜잭션의 Begin SCN
} sdcTSSPageCntlHdr;

/* ------------------------------------------------
 * undo record header 정의
 * ----------------------------------------------*/

typedef UChar sdcUndoRecType;
typedef UChar sdcUndoRecFlag;

#define SDC_UNDOREC_HDR_FIELD_OFFSET(aField)    \
    (aField ## _OFFSET)

#define SDC_UNDOREC_HDR_FIELD_SIZE(aField)      \
    (aField ## _SIZE)

#define SDC_UNDOREC_HDR_TYPE_SIZE       (1)
#define SDC_UNDOREC_HDR_FLAG_SIZE       (1)
#define SDC_UNDOREC_HDR_TABLEOID_SIZE   (ID_SIZEOF(smOID))
#define SDC_UNDOREC_HDR_SIZE            \
    ( SDC_UNDOREC_HDR_TYPE_SIZE +       \
      SDC_UNDOREC_HDR_FLAG_SIZE +       \
      SDC_UNDOREC_HDR_TABLEOID_SIZE )

/*
 * undo record header에는 tableoid 필드(smOID type)가 있는데
 * smOID(vULong) type은 몇 bit 장비인지에 따라 크기가 가변적이다.
 * (32bit 장비 : 4byte,  64bit 장비 : 8byte)
 * 이로 인해 SDC_MAX_ROWPIECE_SIZE의 크기도
 * 몇 비트 장비인지에 따라 가변적이 된다.
 * 이렇게 되면 PROJ-1705에서 추가한 테스트케이스에서
 * diff가 발생하는데(dump를 찍기 때문), lst를 두개(32bit, 64bit) 만드는 것은
 * 너무 번거롭다고 생각했다. 그래서 SDC_UNDOREC_HDR_MAX_SIZE 매크로를
 * 정의하고 tableoid의 크기를 8byte로 계산하도록 하였다.
 * */
#define SDC_UNDOREC_HDR_MAX_SIZE        \
    ( SDC_UNDOREC_HDR_TYPE_SIZE +       \
      SDC_UNDOREC_HDR_FLAG_SIZE +       \
      ID_SIZEOF(ULong) )

#define SDC_UNDOREC_HDR_TYPE_OFFSET     (0)
#define SDC_UNDOREC_HDR_FLAG_OFFSET     \
    ( SDC_UNDOREC_HDR_TYPE_OFFSET +     \
      SDC_UNDOREC_HDR_TYPE_SIZE )
#define SDC_UNDOREC_HDR_TABLEOID_OFFSET \
    ( SDC_UNDOREC_HDR_FLAG_OFFSET +     \
      SDC_UNDOREC_HDR_FLAG_SIZE )

#define SDC_GET_UNDOREC_HDR_FIELD_PTR(aHdr, aField)   \
    ( (aHdr) + SDC_UNDOREC_HDR_FIELD_OFFSET(aField) )

#define SDC_GET_UNDOREC_HDR_FIELD(aHdr, aField, aRetPtr)                \
    (idlOS::memcpy( (void*)(aRetPtr),                                   \
                    (void*)SDC_GET_UNDOREC_HDR_FIELD_PTR(aHdr, aField), \
                    SDC_UNDOREC_HDR_FIELD_SIZE(aField) ))

#define SDC_SET_UNDOREC_HDR_FIELD(aHdr, aField, aValPtr)                \
    (idlOS::memcpy( (void*)SDC_GET_UNDOREC_HDR_FIELD_PTR(aHdr, aField), \
                    (void*)(aValPtr),                                   \
                    SDC_UNDOREC_HDR_FIELD_SIZE(aField) ))

#define SDC_GET_UNDOREC_HDR_1B_FIELD(aHdr, aField, aRet)              \
    (*aRet = *(UChar*)SDC_GET_UNDOREC_HDR_FIELD_PTR(aHdr, aField))

#define SDC_SET_UNDOREC_HDR_1B_FIELD(aHdr, aField, aVal)              \
    (*(UChar*)SDC_GET_UNDOREC_HDR_FIELD_PTR(aHdr, aField) = (aVal))


#define SDC_SET_UNDOREC_FLAG_ON(set, f) \
    ((set) |= (f))

#define SDC_SET_UNDOREC_FLAG_OFF(set, f)     \
    ((set) &= ~(f))


/* Undo Record Header의 Flag정보 */
#define SDC_UNDOREC_FLAG_IS_VALID_MASK  (0x01)
#define SDC_UNDOREC_FLAG_IS_VALID_TRUE  (0x01)
#define SDC_UNDOREC_FLAG_IS_VALID_FALSE (0x00)

#define SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_MASK  (0x02)
#define SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE  (0x02)
#define SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_FALSE (0x00)

#define SDC_UNDOREC_FLAG_UNDO_FOR_LOB_UPDATE_MASK  (0x04)
#define SDC_UNDOREC_FLAG_UNDO_FOR_LOB_UPDATE_TRUE  (0x04)
#define SDC_UNDOREC_FLAG_UNDO_FOR_LOB_UPDATE_FALSE (0x00)

#define SDC_UNDOREC_FLAG_IS_UNDO_FOR_LOB_UPDATE(aFlag)               \
    ( ( ( (aFlag) & SDC_UNDOREC_FLAG_UNDO_FOR_LOB_UPDATE_MASK )      \
        == SDC_UNDOREC_FLAG_UNDO_FOR_LOB_UPDATE_TRUE ) ?             \
      ID_TRUE : ID_FALSE )

#define SDC_UNDOREC_FLAG_IS_UNDO_FOR_HEAD_ROWPIECE(aFlag)               \
    ( ( ( (aFlag) & SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_MASK )      \
        == SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE ) ?             \
      ID_TRUE : ID_FALSE )

#define SDC_UNDOREC_FLAG_IS_VALID(aFlag)                                \
    ( ( ( (aFlag) & SDC_UNDOREC_FLAG_IS_VALID_MASK )                    \
        == SDC_UNDOREC_FLAG_IS_VALID_TRUE ) ?                           \
      ID_TRUE : ID_FALSE )

typedef struct sdcVarColHdr
{
    UShort    offset;
    UShort    length;
} sdcVarColHdr;

/* BUG-25624 디스크테이블에서 CTS할당 실패로 인해 이중화 ADI의
 *           Resource Deadlock제거
 * RowHdr의 확장영역에 기록하는 트랜잭션 정보 정의
 * 디스크 테이블 페이지에서 갱신트랜잭션이 CTS를
 * 할당못한 경우 RowPiece 헤더의 확장영역에 트랜잭션의
 * 갱신정보를 기록한다. */
typedef struct sdcRowHdrExInfo
{
    scPageID   mTSSPageID;
    scSlotNum  mTSSlotNum;
    UShort     mFSCredit;
    smSCN      mFSCNOrCSCN;
} sdcRowHdrExInfo;

typedef struct sdcRowHdrInfo
{
    UChar            mCTSlotIdx;
    smSCN            mInfiniteSCN;
    sdSID            mUndoSID;
    UShort           mColCount;
    UChar            mRowFlag;
    sdcRowHdrExInfo  mExInfo;
} sdcRowHdrInfo;


typedef struct sdcUndoRecHdrInfo
{
    UChar        mType;
    UChar        mFlag;
    smOID        mTableOID;
} sdcUndoRecHdrInfo;

typedef enum sdcColInOutMode
{
    SDC_COLUMN_IN_MODE      = SMI_COLUMN_MODE_IN,
    SDC_COLUMN_OUT_MODE_LOB = SMI_COLUMN_MODE_OUT
} sdcColInOutMode;

// Value를 가리키는 Pointer
// 직접 가리켜야 할 때 사용
typedef struct sdcValue
{
    smiValue          mValue;
    smiValue          mOutValue;
    sdcColInOutMode   mInOutMode; // Value의 In Out Mode
} sdcValue;

// PROJ-2399 rowTemplate fetch시 사용할 colum value 구조체
typedef struct sdcValue4Fetch
{
    sdcColInOutMode mInOutMode; // Value의 In Out Mode
    UShort          mColLenStoreSize;
    smiValue        mValue;
} sdcValue4Fetch;

typedef struct sdcColumnInfo4Insert
{
    const smiColumn * mColumn;
    idBool            mIsUptCol;    // Insert Row Piece For Update에서
                                    // 사용하며 Update Column인지
                                    // 아닌지를 나타낸다.
    sdcValue          mValueInfo;
} sdcColumnInfo4Insert;


/* insert rowpiece 연산시에 사용하는 자료구조이다. */
typedef struct sdcRowPieceInsertInfo
{
    UInt                   mStartColOffset;
    UShort                 mStartColSeq;

    UShort                 mEndColSeq;
    UInt                   mEndColOffset;

    UShort                 mRowPieceSize;   /* 저장하려는 rowpiece의 크기 */
    UShort                 mColCount;       /* total column count in rowpiece */
    
    idBool                 mIsInsert4Upt;   /* insert rowpiece for update 연산인지 여부 */
    idBool                 mIsUptLobByAPI;  /* API에 의한 LOB Update인지의 여부 */

    UShort                 mLobDescCnt;

    /* 컬럼정보 배열 */
    sdcColumnInfo4Insert   mColInfoList[SMI_COLUMN_ID_MAXIMUM];

} sdcRowPieceInsertInfo;

typedef struct sdcColumnInfo4Update
{
    /* mColumn의 값을 보고 update 컬럼인지 여부를 판단한다.
     * mColumn == NULL : update 컬럼 X
     * mColumn != NULL : update 컬럼 O */
    const smiColumn * mColumn;
    sdcValue          mNewValueInfo; // Update하려는 New Value
    sdcValue          mOldValueInfo; // 저장되어있던 Old Value

} sdcColumnInfo4Update;

typedef struct sdcRowPieceUpdateInfo
{
    sdcRowHdrInfo         *mNewRowHdrInfo;
    const sdcRowHdrInfo   *mOldRowHdrInfo;

    UInt                   mNewRowPieceSize;
    UShort                 mOldRowPieceSize;

    /* In Mode LOB을 포함하여 In Mode인 Update Column의 Cnt */
    UShort                 mUptBfrInModeColCnt;
    UShort                 mUptAftInModeColCnt;
    UShort                 mUptAftLobDescCnt;
    UShort                 mUptBfrLobDescCnt;
    
    UShort                 mTrailingNullUptCount;

    /* delete first column piece 연산을 수행할지 여부 */
    idBool                 mIsDeleteFstColumnPiece;
    /* update inplace 연산이 가능한지 여부 */
    idBool                 mIsUpdateInplace;
    /* trailing null을 update 하려는지 여부 */
    idBool                 mIsTrailingNullUpdate;
    /* API에 의한 LOB Update인지의 여부 */
    idBool                 mIsUptLobByAPI; 
    
    sdSID                  mRowPieceSID;

    /* 컬럼정보 배열 */
    sdcColumnInfo4Update   mColInfoList[SMI_COLUMN_ID_MAXIMUM];
    /* rowpiece에 저장된 컬럼들의 old value를 복사하기 위한 8K buffer */
    UChar                  mSpace4CopyOldValue[SD_PAGE_SIZE];

} sdcRowPieceUpdateInfo;

typedef struct sdcRowPieceOverwriteInfo
{
    /* sdcRowPieceUpdateInfo 자료구조의
     * mColInfoList 배열을 가리키는 pointer이다. */
    const sdcColumnInfo4Update  *mColInfoList;

    sdcRowHdrInfo               *mNewRowHdrInfo;
    const sdcRowHdrInfo         *mOldRowHdrInfo;

    UInt                         mNewRowPieceSize;
    UShort                       mOldRowPieceSize;

    UShort                       mUptAftInModeColCnt;
    UShort                       mUptAftLobDescCnt;

    /* only use for rp logging */
    UShort                       mUptInModeColCntBfrSplit;
    
    UShort                       mTrailingNullUptCount;

    /* overwrite rowpiece 연산의 경우
     * split이 발생했을 수 있기 때문에, last column을 저장할때
     * 잘려진 크기(mLstColumnOverwriteSize)만큼만 저장해야 한다. */
    UShort                       mLstColumnOverwriteSize;

    /* API에 의한 LOB Update인지의 여부 */
    idBool                       mIsUptLobByAPI;

    sdSID                        mRowPieceSID;
} sdcRowPieceOverwriteInfo;

/* update 연산의 진행상태에 관한 정보를 저장하는 자료구조이다. */
typedef struct sdcRowUpdateStatus
{
    UShort    mTotalUpdateColCount;    /* update를 수행해야 할 column의 갯수 */
    UShort    mUpdateDoneColCount;     /* update 수행을 완료한 column의 갯수 */

    UShort    mFstColumnSeq;           /* row piece에서 첫번째 column piece의
                                        * sequence */

    UShort    mLstUptColumnSeq;        /* 마지막 update column의 sequence
                                        * trailing null update 처리시에
                                        * 이 정보를 이용한다. */

    UChar     mPrevRowPieceRowFlag;    /* BUG-32278: 이전 row piece의 row flag
                                        * 를 저장한다. row flag의 검증용으로
                                        * 사용된다. */ 
} sdcRowUpdateStatus;

#define SDC_SUPPLEMENT_JOB_NONE                             (0x00000000)
#define SDC_SUPPLEMENT_JOB_UPDATE_LOB_COLS_AFTER_UPDATE     (0x00000001)
#define SDC_SUPPLEMENT_JOB_UPDATE_LOB_COLS_AFTER_OVERWRITE  (0x00000002)
#define SDC_SUPPLEMENT_JOB_CHANGE_ROWPIECE_LINK             (0x00000004)
#define SDC_SUPPLEMENT_JOB_REMOVE_OLD_OUT_MODE_LOB          (0x00000008)

typedef struct sdcSupplementJobInfo
{
    // Supplement Job의 종류가 저장되어 있다.
    UInt                    mJobType;

    /* change rowpiece link 연산시에,
     * 아래 변수에 저장해둔 next rowpiece sid 값을 이용한다. */
    sdSID                   mNextRowPieceSID;
} sdcSupplementJobInfo;

typedef struct sdcColumnInfo4Fetch
{
    /* mColumn의 값을 보고 fetch 컬럼인지 여부를 판단한다.
     * mColumn == NULL : fetch 컬럼 X
     * mColumn != NULL : fetch 컬럼 O */
    const smiColumn              *mColumn;
    sdcValue                      mValueInfo;

    /* 데이터를 저장할때, MT datatype format으로 저장하지 않고
     * raw value만 저장한다.(저장 공간을 줄이기 위해서이다.)
     * 그래서 저장된 data를 QP에게 보내줄 때,
     * memcpy를 하면 안되고, QP가 내려준 callback 함수를 호출해야 한다.
     * QP가 내려준 callback function pointer를 mCallback에 저장한다. */
    smiCopyDiskColumnValueFunc    mCopyDiskColumn;

    /* MT datatype format의 size를 구할때 사용하는 callback 함수 */
    smiActualSizeFunc             mActualSize;

    UShort                        mColSeqInRowPiece;
} sdcColumnInfo4Fetch;

typedef struct sdcRowPieceFetchInfo
{
    /* 컬럼정보 배열 */
    sdcColumnInfo4Fetch   mColInfoList[SMI_COLUMN_ID_MAXIMUM];

    UShort                mFetchColCount;
} sdcRowPieceFetchInfo;

typedef struct sdcRowFetchStatus
{
    UShort    mTotalFetchColCount;    /* fetch를 수행해야 할 column의 갯수 */
    UShort    mFetchDoneColCount;     /* fetch 수행을 완료한 column의 갯수 */

    UShort    mFstColumnSeq;          /* row piece에서 첫번째 column piece의
                                       * sequence */

    UInt      mAlreadyCopyedSize;     /* 여러 rowpiece에 나누어 저장된 컬럼을
                                       * fetch하는 경우, copy offset 정보를
                                       * 저장한다. */
    const smiFetchColumnList * mFstFetchConlumn; /* row piece에서 찾아야할 첫번째
                                                  * fetch 대상 column */
} sdcRowFetchStatus;

/* BUG-22943 index bottom up build 성능개선 */
typedef IDE_RC (*sdcCallbackFunc4Index)( const smiColumn * aIndexVRowColumn,
                                         UInt              aCopyOffset,
                                         const smiValue  * aColumnValue,
                                         void            * aIndexInfo );

/* BUG-22943 index bottom up build 성능개선 */
typedef struct sdcIndexInfo4Fetch
{
    const void                *mTableHeader;
    UShort                     mVarOffset;
    sdcCallbackFunc4Index      mCallbackFunc4Index;
    smiValue                   mValueList[ SMI_COLUMN_ID_MAXIMUM ];
    UChar                     *mBuffer;
    UChar                     *mBufferCursor;

    /* BUG-24091
     * [SD-기능추가] vrow column 만들때 원하는 크기만큼만 복사하는 기능 추가 */
    /* vrow column 만들때 fetchSize 크기 이상은 복사하지 않는다. */
    UInt                      mFetchSize;
} sdcIndexInfo4Fetch;

typedef struct sdcColumnInfo4PK
{
    const smiColumn    *mColumn;
    smiValue            mValue;
    sdcColInOutMode     mInOutMode;
} sdcColumnInfo4PK;

/* pk 정보를 저장하는 자료구조이다. */
typedef struct sdcPKInfo
{
    /* 컬럼정보 배열 */
    sdcColumnInfo4PK    mColInfoList[SMI_MAX_IDX_COLUMNS];
    /* PK value를 복사하기 위한 4K buffer */
    UChar               mSpace4CopyPKValue[SD_PAGE_SIZE/2];

    /* primary key index에 지정되어 있는 column의 갯수  */
    UShort              mTotalPKColCount;
    /* 복사를 완료한 pk column의 갯수 */
    UShort              mCopyDonePKColCount;

    /* row piece에서 첫번째 column piece의 sequence */
    UShort              mFstColumnSeq;
} sdcPKInfo;


/* ------------------------------------------------
 * updatable check 상태값
 * ----------------------------------------------*/
typedef enum sdcUpdateState
{
    SDC_UPTSTATE_NULL,

    // update 가능 상태
    SDC_UPTSTATE_UPDATABLE,

    // delete 된 상태
    SDC_UPTSTATE_ALREADY_DELETED,

    // 이미 자신의 statement에 의해 갱신됨
    SDC_UPTSTATE_INVISIBLE_MYUPTVERSION,

    // 다른 Tx가 변경했으나 아직 commit되지 않았을때
    // commit되기를 기다려 retry한다.
    SDC_UPTSTATE_UPDATE_BYOTHER,

    // 이미 다른 트랜잭션이 내 StmtSCN보다 큰 CSCN
    // 으로 갱신한 경우
    SDC_UPTSTATE_REBUILD_ALREADY_MODIFIED,

    // 인덱스에서 Unique Vilolation 난 경우
    SDC_UPTSTATE_UNIQUE_VIOLATION,

    // Row Retry 가 필요한 경우
    SDC_UPTSTATE_ROW_RETRY,
} sdcUpdateState;

/* X$TSSEG 정보출력 자료구조 */
typedef struct sdcTSSegInfo
{
    UInt          mSpaceID;        // TBSID
    scPageID      mSegPID;         // 세그먼트 PID
    UShort        mType;           // 세그먼트 타입
    UShort        mState;          // 세그먼트 상태
    UInt          mTXSegID;        // 트랜잭션 세그먼트 ID
    ULong         mTotExtCnt;      // 총 ExtDesc 개수
    ULong         mTotExtDirCnt;   // 총 ExtDir 개수
    sdRID         mCurAllocExtRID; // 현재 사용중인 ExtDesc RID
    scPageID      mCurAllocPID;    // 현재 사용중인 페이지의 PID
    UInt          mPageCntInExt;   // ExtDesc 당 페이지 개수
} sdcTSSegInfo;

typedef sdcTSSegInfo sdcUDSegInfo;

/*
 * X$DISK_UNDO_RECORDS 구조체
 */
typedef struct sdcUndoRec4FT
{
    UInt       mSegSeq;                        // SEG_SEQ
    scPageID   mSegPID;                        // SEG_PID
    scPageID   mPageID;                        // PAGE_ID
    scOffset   mOffset;                        // OFFSET
    UShort     mNthSlot;                       // NTH_SLOT
    UInt       mSize;                          // SIZE
    UInt       mType;                          // TYPE
    UInt       mFlag;                          // FLAG
    ULong      mTableOID;                      // TABLE_OID
} sdcUndoRec4FT;

/*
 * X$DISK_TSS_RECORDS 구조체
 */
typedef struct sdcTSS4FT
{
    UInt       mSegSeq;          // SEG_SEQ
    scPageID   mSegPID;          // SEG_PID
    scPageID   mPageID;          // PAGE_ID
    scOffset   mOffset;          // OFFSET
    SShort     mNthSlot;         // NTH_SLOT
    smTID      mTransID;         // TID
    SChar    * mCSCN;            // CSCN
    sdcTSState mState;           // STATE
} sdcTSS4FT;

/*
 * ROJ-1704 Disk MVCC Renewal
 *
 * 트랜잭션 세그먼트 엔트리의 상태값을 정의한다.
 *
 * OFFLINE - 트랜잭션 세그먼트 엔트리가 할당되지 않은 상태
 * ONLINE  - 트랜잭션 세그먼트 엔트리가 트랜잭션에 할당된 상태
 */
typedef enum sdcTXSegStatus
{
    SDC_TXSEG_OFFLINE = 0,
    SDC_TXSEG_ONLINE
} sdcTXSegStatus;

/* Row Version에 대한 연산 종류 정의 */
typedef enum sdcOperToMakeRowVer
{
    SDC_MVCC_MAKE_VALROW,
    SDC_REDO_MAKE_NEWROW,
    SDC_UNDO_MAKE_OLDROW
} sdcOperToMakeRowVer;

/* CTS의 FSCOrCSCN에 대한 COMMIT SCN 여부만을 확인한다 */
#ifdef COMPILE_64BIT
#define SDC_CTS_SCN_IS_COMMITTED( SCN )        \
    ( ( ( SCN ) & SM_SCN_COMMIT_PARITY_BIT ) != SM_SCN_COMMIT_PARITY_BIT )
#define SDC_CTS_SCN_IS_NOT_COMMITTED( SCN )    \
    ( ( ( SCN ) & SM_SCN_COMMIT_PARITY_BIT ) == SM_SCN_COMMIT_PARITY_BIT )
#define SDC_CTS_SCN_IS_LEGACY( SCN )        \
    ( ( ( SCN ) & SM_SCN_COMMIT_LEGACY_BIT ) == SM_SCN_COMMIT_LEGACY_BIT )
#define SDC_CTS_SCN_IS_NOT_LEGACY( SCN )    \
    ( ( ( SCN ) & SM_SCN_COMMIT_LEGACY_BIT ) != SM_SCN_COMMIT_LEGACY_BIT )
#else
# define SDC_CTS_SCN_IS_COMMITTED( SCN )        \
    ( ((SCN).mLow & SM_SCN_COMMIT_PARITY_BIT ) != SM_SCN_COMMIT_PARITY_BIT )
# define SDC_CTS_SCN_IS_NOT_COMMITTED( SCN )    \
    ( ((SCN).mLow & SM_SCN_COMMIT_PARITY_BIT ) == SM_SCN_COMMIT_PARITY_BIT )
# define SDC_CTS_SCN_IS_LEGACY( SCN )        \
    ( ((SCN).mLow & SM_SCN_COMMIT_LEGACY_BIT ) == SM_SCN_COMMIT_LEGACY_BIT )
# define SDC_CTS_SCN_IS_NOT_LEGACY( SCN )    \
    ( ((SCN).mLow & SM_SCN_COMMIT_LEGACY_BIT ) != SM_SCN_COMMIT_LEGACY_BIT )
#endif

// Direct-Path INSERT를 위한 자료 구조
// Transaction 단위로 생성된다.
typedef struct sdcDPathEntry
{
    sdbDPathBuffInfo    mDPathBuffInfo;
    sdpDPathInfo        mDPathInfo;
} sdcDPathEntry;

// X$DIRECT_PATH_INSERT의 출력을 위한 통계 값을 저장
typedef struct sdcDPathStat
{
    ULong   mCommitTXCnt;
    ULong   mAbortTXCnt;
    ULong   mInsRowCnt;
    ULong   mAllocBuffPageTryCnt;
    ULong   mAllocBuffPageFailCnt;
    ULong   mBulkIOCnt;
} sdcDPathStat;



/**********************************************************************
 * Disk LOB 자료 구조
 **********************************************************************/

#define SDC_LOB_INVALID_KEY_SEQ     (-1)
#define SDC_LOB_INVALID_ENTRY_SEQ   (-1)
#define SDC_LOB_MAX_IN_MODE_SIZE    SM_LOB_MAX_IN_ROW_SIZE   // Max In Mode LOB Size

/*
 * LOB Descriptor
 */

#define SDC_LOB_DESC_NULL_MASK   (0x0001)
#define SDC_LOB_DESC_NULL_FALSE  (0x0000)
#define SDC_LOB_DESC_NULL_TRUE   (0x0001)

#define SDC_LOB_MAX_DIRECT_PAGE_CNT (4)

typedef struct sdcLobDesc
{
    ULong       mLobVersion;
    UShort      mLobDescFlag;
    UShort      mLastPageSize;
    UInt        mLastPageSeq;
    UInt        mDirectCnt;
    scPageID    mRootNodePID;
    scPageID    mDirect[SDC_LOB_MAX_DIRECT_PAGE_CNT];
} sdcLobDesc;

/*
 * LOB Meta Page
 */

typedef struct sdcLobMeta
{
    idBool              mLogging;       // Reserved
    idBool              mBuffer;        // Reserved
    UInt                mColumnID;
    sdpDblPIDListBase   mAgingListBase;
} sdcLobMeta;

/*
 * LOB Index & Data Page
 */

typedef struct sdcLobNodeHdr
{
    UShort  mHeight;        // Internal, Leaf Page
    UShort  mKeyCnt;        // Internal, Leaf Page
    UInt    mNodeSeq;       // Internal, Leaf Page
    UShort  mStoreSize;     // Data Page
    UShort  mAlign;         // All
    UInt    mLobPageState;  // All
    ULong   mLobVersion;    // All
    sdSID   mTSSlotSID;     // All
    smSCN   mFstDskViewSCN; // All
} sdcLobNodeHdr;

/* sdcLOBIKey must be aligned. */
typedef struct sdcLobIKey
{
    scPageID    mChild;
} sdcLobIKey;

#define SDC_LOB_MAX_ENTRY_CNT   (8)
 
/* sdcLOBLKey must be aligned. */
typedef struct sdcLobLKey
{
    UShort      mLobVersion;
    UShort      mEntryCnt;
    UInt        mReserved;
    sdSID       mUndoSID;
    scPageID    mEntry[SDC_LOB_MAX_ENTRY_CNT];
} sdcLobLKey;

typedef enum sdcLobPageState
{
    SDC_LOB_UNUSED  = 0,
    SDC_LOB_USED,
    SDC_LOB_AGING_LIST
} sdcLobPageState;

#define SDC_LOB_PAGE_BODY_SIZE  ( sdpPhyPage::getEmptyPageFreeSize() -          \
                                  idlOS::align8(ID_SIZEOF(sdcLobNodeHdr)) )

/*
 * Stuff
 */

typedef struct sdcLobInfo4Fetch
{
    /* LobCursor open mode(read or read write) */
    smiLobCursorMode    mOpenMode;  // LOB Cursor Open Mode
    sdcColInOutMode     mInOutMode; // LOB Data의 In Out 유무
} sdcLobInfo4Fetch;

typedef struct sdcColumnInfo4Lob
{
    const smiColumn * mColumn;
    sdcLobDesc      * mLobDesc;
} sdcColumnInfo4Lob;

typedef struct sdcLobInfoInRowPiece
{
    sdcColumnInfo4Lob   mColInfoList[SD_PAGE_SIZE/ID_SIZEOF(sdcLobDesc)];
    UChar               mSpace4CopyLobDesc[SD_PAGE_SIZE];
    UShort              mLobDescCnt;
} sdcLobInfoInRowPiece;

typedef struct sdcLobColBuffer
{   
    UChar           * mBuffer;
    UInt              mLength;
    sdcColInOutMode   mInOutMode;
    idBool            mIsNullLob;
} sdcLobColBuffer;

typedef enum sdcLobWriteType
{
    SDC_LOB_WTYPE_WRITE = 0,
    SDC_LOB_WTYPE_ERASE,
    SDC_LOB_WTYPE_TRIM
} sdcLobWriteType;

typedef enum sdcLobChangeType
{
    SDC_LOB_IN_TO_IN = 0,
    SDC_LOB_IN_TO_OUT,
    SDC_LOB_OUT_TO_OUT,
} sdcLobChangeType;

#define SDC_LOB_STACK_SIZE  (6)

typedef struct sdcLobStack
{
    SInt        mPos;
    scPageID    mStack[SDC_LOB_STACK_SIZE];
} sdcLobStack;

#define SDC_LOB_SET_IKEY( aKey, aChild )     \
{                                            \
    (aKey)->mChild = aChild; \
}

#endif
