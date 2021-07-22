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
 * $Id$
 **********************************************************************/

#ifndef _O_SDC_ROW_H_
#define _O_SDC_ROW_H_ 1

#include <sdcDef.h>
#include <smcDef.h>
#include <sdcTableCTL.h>

#define SDC_CD_SETSIZE    (SMI_COLUMN_ID_MAXIMUM)

#define SDC_NCDBITS       (8 * ID_SIZEOF(UChar))

#define SDC_CDSET_LONGS   (SDC_CD_SETSIZE / SDC_NCDBITS)

#define SDC_CDELT(d)      ((d) / SDC_NCDBITS)

#define SDC_CDMASK(d)     (1UL << ((d) % SDC_NCDBITS))

#define SDC_CD_SET(d, set)    \
    ((set)->cds_bits[SDC_CDELT(d)] |= SDC_CDMASK(d))

#define SDC_CD_CLR(d, set)    \
    ((set)->cds_bits[SDC_CDELT(d)] &= ~SDC_CDMASK(d))                          \

#define SDC_CD_ISSET(d, set)  \
    ( (((set)->cds_bits[SDC_CDELT(d)] & SDC_CDMASK(d)) != 0)    \
      ? ID_TRUE : ID_FALSE )

#define SDC_CD_ZERO(set)      \
    ((void) idlOS::memset ((void*)(set), 0x00, ID_SIZEOF(sdcColumnDescSet)))

typedef struct sdcColumnDescSet
{
    UChar cds_bits[SDC_CDSET_LONGS];
} sdcColumnDescSet;


#define SDC_LOG_UNDO_INFO_LAYER_MAX_SIZE    \
    ( SDC_UNDOREC_HDR_MAX_SIZE + ID_SIZEOF(scGRID) )

// UPDATE_ROW_PIECE 로그가 Update Info Layer가 가장 크다.
// opcode(1)
// size(2)
// colcount(2)
// column descset size(1)
// column destset(1~128)
#define SDC_LOG_UPDATE_INFO_LAYER_MAX_SIZE   \
    ( (1) + (2) + (2) + (1) + (128) )

#define SDC_RESERVE_SIZE_FOR_UNDO_RECORD_LOGGING    \
    ( SDC_LOG_UNDO_INFO_LAYER_MAX_SIZE +            \
      SDC_LOG_UPDATE_INFO_LAYER_MAX_SIZE )

#define SDC_EXTRASIZE_FOR_CHAINING                              \
    ( ID_SIZEOF(scPageID) + ID_SIZEOF(scSlotNum) )

#define SDC_MIN_COLPIECE_SIZE    (2)
#define SDC_MIN_ROWPIECE_SIZE                                   \
    ( SDC_ROWHDR_SIZE + SDC_EXTRASIZE_FOR_CHAINING )

#define SDC_MAX_ROWPIECE_SIZE_WITHOUT_CTL                       \
    ( sdpPhyPage::getEmptyPageFreeSize()                        \
      - ID_SIZEOF(sdpSlotDirHdr)                                \
      - ID_SIZEOF(sdpSlotEntry)                                 \
      - SDC_RESERVE_SIZE_FOR_UNDO_RECORD_LOGGING )

#define SDC_MAX_ROWPIECE_SIZE( aCTLSize )                       \
    ( sdpPhyPage::getEmptyPageFreeSize()                        \
      - ID_SIZEOF(sdpSlotDirHdr)                                \
      - ID_SIZEOF(sdpSlotEntry)                                 \
      - (aCTLSize)                                              \
      - SDC_RESERVE_SIZE_FOR_UNDO_RECORD_LOGGING )


#define SDC_COLUMN_LEN_STORE_SIZE_THRESHOLD (250)

#define SDC_COLUMN_PREFIX_COUNT             (3)
#define SDC_NULL_COLUMN_PREFIX              (0xFF)
#define SDC_LARGE_COLUMN_PREFIX             (0xFE)
#define SDC_LOB_DESC_COLUMN_PREFIX          (0xFD)

#define SDC_LARGE_COLUMN_LEN_STORE_SIZE     (3)
#define SDC_SMALL_COLUMN_LEN_STORE_SIZE     (1)
#define SDC_MAX_COLUMN_LEN_STORE_SIZE       SDC_LARGE_COLUMN_LEN_STORE_SIZE

#define SDC_ROWHDR_H_FLAG                   (0x10)  // (H)ead piece of row
#define SDC_ROWHDR_F_FLAG                   (0x08)  // (F)irst data piece
#define SDC_ROWHDR_L_FLAG                   (0x04)  // (L)ast data piece
#define SDC_ROWHDR_P_FLAG                   (0x02)  // first column continues from (P)revious piece
#define SDC_ROWHDR_N_FLAG                   (0x01)  // last column continues in (N)ext piece

#define SDC_ROWHDR_FLAG_ALL    \
    ( SDC_ROWHDR_H_FLAG |      \
      SDC_ROWHDR_F_FLAG |      \
      SDC_ROWHDR_L_FLAG |      \
      SDC_ROWHDR_P_FLAG |      \
      SDC_ROWHDR_N_FLAG )

#define SDC_ROWHDR_FLAG_NO_CHAINING_ROW    \
    ( SDC_ROWHDR_H_FLAG |                  \
      SDC_ROWHDR_F_FLAG |                  \
      SDC_ROWHDR_L_FLAG )

#define SDC_ROWHDR_FLAG_NULLROW    SDC_ROWHDR_FLAG_NO_CHAINING_ROW

/*
 * ###   FSC 플래그   ###
 *
 * DML 연산중에는 당연히 FSC를 reserve 해야 한다.
 * 그러면 redo나 undo시에는 어떻게 해야 하나?
 *
 * redo는 DML 연산을 다시 수행하는 것이므로,
 * DML 연산할때와 동일하게 FSC를 reserve 해야 한다.
 *
 * 반면 undo시에는 FSC를 reserve하면 안된다.
 * 왜나하면 FSC는 DML 연산을 undo시킬때를 대비해서
 * 공간을 예약해두는 것이므로,
 * undo시에는 이전에 reserve해둔 FSC를
 * 페이지에 되돌려(restore)주어야 하고,
 * undo시에 또 다시 FSC를 reserve하려고 해서는 안된다.
 *
 * clr은 undo에 대한 redo이므로 undo때와 동일하게
 * FSC를 reserve하면 안된다.
 *
 * 이 세가지 경우를 구분하여
 * FSC reserve 처리를 해야 하는데,
 * 나(upinel9)는 로그를 기록할때 FSC reserve 여부를 플래그로 남겨서,
 * redo나 undo시에는 이 플래그만 보고
 * reallocSlot()을 하도록 설계하였다.
 *
 * redo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_TRUE
 * undo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
 * clr redo : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
 */
#define SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_MASK   (0x01)
#define SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_TRUE   (0x01)
#define SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE  (0x00)

#define SDC_UPDATE_LOG_FLAG_UPDATE_METHOD_MASK        (0x02)
#define SDC_UPDATE_LOG_FLAG_UPDATE_METHOD_INPLACE     (0x02)
#define SDC_UPDATE_LOG_FLAG_UPDATE_METHOD_OUTPLACE    (0x00)

#define SDC_UPDATE_LOG_FLAG_LOCK_TYPE_MASK        (0x04)
#define SDC_UPDATE_LOG_FLAG_LOCK_TYPE_EXPLICIT    (0x04)
#define SDC_UPDATE_LOG_FLAG_LOCK_TYPE_IMPLICIT    (0x00)

/*
 * PROJ-1704 Disk MVCC 리뉴얼
 *
 * Row-Based Read Consitency
 *
 * Row Piece 변경이후 다른 트랜잭션이 Page에 접근할 때 수행하는
 * Row Time-Stamping 환경하에서 각 Row Piece들에 대해서 CommitSCN
 * 정보를 설정하여 Read-Only 혹은 Update 트랜잭션이 Row Piece의 판독
 * 및 갱신 여부를 최대한 바로 판단할 수 있도록 한다.
 * Row Piece 와 관련된 Undo Record의 SID(UndoSID)를 저장하여
 * Row Piece 기반으로 이전 Row Piece Version을 생성할 수 있도록 한다
 *
 * @ Row Piece Header (+RowHdrEx) 자료구조
 * ______________________________________________________________
 * | CTSLOTIDX | InfiniteSCN | URID.mPageID | URID.mUndoSlotNum |
 * |___________|_____________|______________|___________________|
 *             | FLAG | COLCNT |
 *             |______|________|___________________________
 * (RowHdrEx)  |TSSPID   |TSSlotNum |FSCredit |FSCNOrCSCN |
 *             |_________|__________|_________|___________|
 *
 * Row Piece Header 크기는 34Bytes이다.
 *
 * (1) CTSLOTIDX
 *     갱신을 위한 트랜잭션은 페이지로부터 CTS를 할당 받은 후에 Row를
 *     변경할 수 있다. 할당받은 CTS를 Row Piece에 바인딩할때 CTSlot
 *     Number를 기록한다.
 *
 * (2) InfiniteSCN
 *     Row Piece 변경시점의 InfiniteSCN을 기록한다.
 *
 * (3) UndoSID
 *     Row Piece의 변경에 대한 마지막 Undo Record의 Page와 SlotNum 이다.
 *     Read-Only 트랜잭션은 Row Piece의 UndoSID를 따라서 Undo Record에
 *     접근하여 이전 Row Piece의 Version을 생성할 수 있다.
 *
 * (4) FLAG
 *     Row Piece의 구성정보를 나타낸다. 'H','F','L','P','N'의 비트값으로
 *     구성된다.
 *
 * (5) COLCNT
 *     Row Piece 가 저장하고 있는 Column 개수이다.
 *
 * (9) FSCNOrCSCN은 row binding CTI의 경우 CTS에 해당하는 정보를 기록하고,
 *     commit 이후에는 CommitSCN을 설정한다.
 *
 * (6) ~ (9) 까지는 Row 바인딩된 경우만 유효하다. 즉, RowPiece 내에 CTS
 * 정보를 저장하기 위한 자료구조이며, 모든 RowPiece가 가지고 있다.
 *
 */

// Row Piece Header의 각 구성요소에 대한 Offset 정의
#define SDC_ROWHDR_CTSLOTIDX_SIZE      ( ID_SIZEOF(UChar) )
#define SDC_ROWHDR_INFINITESCN_SIZE    ( ID_SIZEOF(smSCN) )
#define SDC_ROWHDR_UNDOPAGEID_SIZE     ( ID_SIZEOF(scPageID) )
#define SDC_ROWHDR_UNDOSLOTNUM_SIZE    ( ID_SIZEOF(scSlotNum) )
#define SDC_ROWHDR_COLCOUNT_SIZE       ( ID_SIZEOF(UShort) )
#define SDC_ROWHDR_FLAG_SIZE           ( ID_SIZEOF(UChar) )
#define SDC_ROWHDR_TSSLOTPID_SIZE      ( ID_SIZEOF(scPageID) )
#define SDC_ROWHDR_TSSLOTNUM_SIZE      ( ID_SIZEOF(scSlotNum) )
#define SDC_ROWHDR_FSCREDIT_SIZE       ( ID_SIZEOF(UShort) )
#define SDC_ROWHDR_FSCNORCSCN_SIZE     ( ID_SIZEOF(smSCN) )

#define SDC_ROWHDR_SIZE                ( SDC_ROWHDR_CTSLOTIDX_SIZE  + \
                                         SDC_ROWHDR_INFINITESCN_SIZE  + \
                                         SDC_ROWHDR_UNDOPAGEID_SIZE + \
                                         SDC_ROWHDR_UNDOSLOTNUM_SIZE + \
                                         SDC_ROWHDR_COLCOUNT_SIZE   + \
                                         SDC_ROWHDR_FLAG_SIZE       + \
                                         SDC_ROWHDR_TSSLOTPID_SIZE  + \
                                         SDC_ROWHDR_TSSLOTNUM_SIZE  + \
                                         SDC_ROWHDR_FSCREDIT_SIZE   + \
                                         SDC_ROWHDR_FSCNORCSCN_SIZE )

// Row Piece Header의 각 구성요소에 대한 Size 정의
#define SDC_ROWHDR_CTSLOTIDX_OFFSET    (0)
#define SDC_ROWHDR_INFINITESCN_OFFSET  ( SDC_ROWHDR_CTSLOTIDX_OFFSET +  \
                                         SDC_ROWHDR_CTSLOTIDX_SIZE )
#define SDC_ROWHDR_UNDOPAGEID_OFFSET   ( SDC_ROWHDR_INFINITESCN_OFFSET +  \
                                         SDC_ROWHDR_INFINITESCN_SIZE )
#define SDC_ROWHDR_UNDOSLOTNUM_OFFSET   ( SDC_ROWHDR_UNDOPAGEID_OFFSET + \
                                         SDC_ROWHDR_UNDOPAGEID_SIZE )
#define SDC_ROWHDR_COLCOUNT_OFFSET     ( SDC_ROWHDR_UNDOSLOTNUM_OFFSET + \
                                         SDC_ROWHDR_UNDOSLOTNUM_SIZE )
#define SDC_ROWHDR_FLAG_OFFSET         ( SDC_ROWHDR_COLCOUNT_OFFSET +   \
                                         SDC_ROWHDR_COLCOUNT_SIZE )

#define SDC_ROWHDR_TSSLOTPID_OFFSET     ( SDC_ROWHDR_FLAG_OFFSET      +  \
                                          SDC_ROWHDR_FLAG_SIZE )
#define SDC_ROWHDR_TSSLOTNUM_OFFSET     ( SDC_ROWHDR_TSSLOTPID_OFFSET +  \
                                          SDC_ROWHDR_TSSLOTPID_SIZE )
#define SDC_ROWHDR_FSCREDIT_OFFSET      ( SDC_ROWHDR_TSSLOTNUM_OFFSET +  \
                                          SDC_ROWHDR_TSSLOTNUM_SIZE )
#define SDC_ROWHDR_FSCNORCSCN_OFFSET    ( SDC_ROWHDR_FSCREDIT_OFFSET +  \
                                          SDC_ROWHDR_FSCREDIT_SIZE )

#define SDC_ROW_NEXT_PID_OFFSET     ( SDC_ROWHDR_FSCNORCSCN_OFFSET   +  \
                                      SDC_ROWHDR_FSCNORCSCN_SIZE )

#define SDC_ROW_NEXT_SNUM_OFFSET    ( SDC_ROW_NEXT_PID_OFFSET + \
                                      ID_SIZEOF(scPageID) )

#define SDC_ROWHDR_FIELD_OFFSET(aField)    \
    (aField ## _OFFSET)

#define SDC_ROWHDR_FIELD_SIZE(aField)      \
    (aField ## _SIZE)

#define SDC_GET_ROWHDR_FIELD_PTR(aHdr, aField)   \
    ( (aHdr) + SDC_ROWHDR_FIELD_OFFSET(aField) )

#define SDC_GET_ROWHDR_FIELD(aHdr, aField, aRet)                   \
    IDE_DASSERT( SDC_ROWHDR_FIELD_SIZE(aField) != 1 );             \
    idlOS::memcpy( (void*)(aRet),                                  \
                   (void*)SDC_GET_ROWHDR_FIELD_PTR(aHdr, aField),  \
                   SDC_ROWHDR_FIELD_SIZE(aField) )

#define SDC_SET_ROWHDR_FIELD(aHdr, aField, aVal)                   \
    IDE_DASSERT( SDC_ROWHDR_FIELD_SIZE(aField) != 1 );             \
    idlOS::memcpy( (void*)SDC_GET_ROWHDR_FIELD_PTR(aHdr, aField),  \
                   (void*)(aVal),                                  \
                   SDC_ROWHDR_FIELD_SIZE(aField) )

#define SDC_GET_ROWHDR_1B_FIELD(aHdr, aField, aRet)              \
    IDE_DASSERT( ID_SIZEOF(aRet) == 1 );                         \
    IDE_DASSERT( SDC_ROWHDR_FIELD_SIZE(aField) == 1 );           \
    (aRet) = *(UChar*)SDC_GET_ROWHDR_FIELD_PTR(aHdr, aField);

#define SDC_SET_ROWHDR_1B_FIELD(aHdr, aField, aVal)              \
    IDE_DASSERT( ID_SIZEOF(aVal) == 1 );                         \
    IDE_DASSERT( SDC_ROWHDR_FIELD_SIZE(aField) == 1 );           \
    *(UChar*)SDC_GET_ROWHDR_FIELD_PTR(aHdr, aField) = (aVal)

#define SDC_IS_HEAD_ROWPIECE(aFlag)                                \
    ( (SM_IS_FLAG_ON((aFlag), SDC_ROWHDR_H_FLAG) == ID_TRUE) ? \
      ID_TRUE : ID_FALSE )

#define SDC_IS_HEAD_ONLY_ROWPIECE(aFlag)                           \
    ( ( (aFlag) == SDC_ROWHDR_H_FLAG ) ? ID_TRUE: ID_FALSE )

#define SDC_IS_LAST_ROWPIECE(aFlag)                                \
    ( (SM_IS_FLAG_ON((aFlag), SDC_ROWHDR_L_FLAG) == ID_TRUE) ? \
      ID_TRUE : ID_FALSE )

#define SDC_IS_FIRST_PIECE_IN_INSERTINFO(aInsertInfo)      \
    ( ( ((aInsertInfo)->mStartColSeq    == 0) &&           \
        ((aInsertInfo)->mStartColOffset == 0) )            \
      ? ID_TRUE : ID_FALSE )

#define SDC_GET_COLLEN_STORE_SIZE(aLen)                                    \
    ( ((aLen) > SDC_COLUMN_LEN_STORE_SIZE_THRESHOLD) ?                     \
      SDC_LARGE_COLUMN_LEN_STORE_SIZE : SDC_SMALL_COLUMN_LEN_STORE_SIZE )

#define SDC_GET_COLPIECE_SIZE(aLen)                                        \
    ( SDC_GET_COLLEN_STORE_SIZE(aLen) + (aLen) )

#define SDC_GET_COLUMN_SEQ(aColumn)    \
    ( (aColumn)->id % SMI_COLUMN_ID_MAXIMUM )

#define SDC_IS_NULL(aValue)                                     \
    ( (((aValue)->value == NULL) && ((aValue)->length == 0)) ?  \
    ID_TRUE : ID_FALSE )

#define SDC_IS_EMPTY(aValue)                                    \
    ( (((aValue)->value != NULL) && ((aValue)->length == 0)) ?  \
    ID_TRUE : ID_FALSE )

#define SDC_IS_LOB_COLUMN(aColumn)                              \
    ( ( ( (aColumn)->flag & SMI_COLUMN_TYPE_MASK )              \
        == SMI_COLUMN_TYPE_LOB )                                \
      ? ID_TRUE : ID_FALSE )

// BUG-31134 Insert Undo Record log에 추가되는 RP Info는
//           Before Image를 기준으로 작성되어야 합니다.
// ( Is Update ) && (( Redo && New is In Mode ) || ( Undo && Old is In Mode ))
#define SDC_IS_IN_MODE_UPDATE_COLUMN( aColumnInfo, aIsUndoRec )               \
    ((((aColumnInfo)->mColumn != NULL ) &&                                    \
      ((((aIsUndoRec) == ID_FALSE ) &&                                        \
        ((aColumnInfo)->mNewValueInfo.mInOutMode == SDC_COLUMN_IN_MODE ))         \
       ||                                                                     \
       (((aIsUndoRec) == ID_TRUE ) &&                                         \
        ((aColumnInfo)->mOldValueInfo.mInOutMode == SDC_COLUMN_IN_MODE ))         \
     )) ? ID_TRUE : ID_FALSE )

#define SDC_IS_IN_MODE_COLUMN( aValue )                         \
    (((aValue).mInOutMode == SDC_COLUMN_IN_MODE ) ? ID_TRUE : ID_FALSE )

#define SDC_GET_COLUMN_INOUT_MODE( aColumn, aLength )   \
   ((( SDC_IS_LOB_COLUMN( aColumn ) == ID_TRUE ) &&     \
     ( (aLength) > (aColumn)->vcInOutBaseSize )) ?      \
    SDC_COLUMN_OUT_MODE_LOB : SDC_COLUMN_IN_MODE )

#define SDC_IS_UPDATE_COLUMN(aColumn)                           \
    ( ( (aColumn) != NULL ) ? ID_TRUE : ID_FALSE )

#define SDC_INIT_ROWHDR_INFO(aRowHdrInfo,                   \
                             aCTSlotIdx,                    \
                             aInfiniteSCN,                  \
                             aUndoSID,                      \
                             aColCount,                     \
                             aRowFlag,                      \
                             aFstDiskViewSCN )              \
    (aRowHdrInfo)->mCTSlotIdx    = (aCTSlotIdx);            \
    (aRowHdrInfo)->mInfiniteSCN  = (aInfiniteSCN);          \
    (aRowHdrInfo)->mUndoSID      = (aUndoSID);              \
    (aRowHdrInfo)->mColCount     = (aColCount);             \
    (aRowHdrInfo)->mRowFlag      = (aRowFlag);              \
    (idlOS::memset( &(aRowHdrInfo)->mExInfo, 0x00, ID_SIZEOF(sdcRowHdrExInfo))); \
    SM_SET_SCN( &((aRowHdrInfo)->mExInfo.mFSCNOrCSCN), &aFstDiskViewSCN );

#define SDC_INIT_ROWHDREX_INFO( aRowHdrExInfo,                   \
                                aTSSlotSID,                      \
                                aFSCreditSize,                   \
                                aFSCNOrCSCN )                    \
    (aRowHdrExInfo)->mTSSPageID  = SD_MAKE_PID(aTSSlotSID);      \
    (aRowHdrExInfo)->mTSSlotNum  = SD_MAKE_SLOTNUM(aTSSlotSID);  \
    (aRowHdrExInfo)->mFSCredit   = (aFSCreditSize);              \
    SM_SET_SCN( &((aRowHdrExInfo)->mFSCNOrCSCN), &(aFSCNOrCSCN) );

#define SDC_STAMP_ROWHDREX_INFO( aRowHdrExInfo,                  \
                                 aCommitSCN )                    \
    (aRowHdrExInfo)->mTSSPageID  = SC_NULL_PID;                  \
    (aRowHdrExInfo)->mTSSlotNum  = SC_NULL_SLOTNUM;              \
    (aRowHdrExInfo)->mFSCredit   = 0;                            \
    SM_SET_SCN( &((aRowHdrExInfo)->mFSCNOrCSCN), &(aCommitSCN) );

#define SDC_INIT_INSERT_INFO(aInsertInfo)           \
    (aInsertInfo)->mStartColSeq       = 0;          \
    (aInsertInfo)->mStartColOffset    = 0;          \
    (aInsertInfo)->mEndColSeq         = 0;          \
    (aInsertInfo)->mEndColOffset      = 0;          \
    (aInsertInfo)->mRowPieceSize      = 0;          \
    (aInsertInfo)->mColCount          = 0;          \
    (aInsertInfo)->mLobDescCnt        = 0;          \
    (aInsertInfo)->mIsUptLobByAPI     = ID_FALSE;   \
    (aInsertInfo)->mIsInsert4Upt      = ID_FALSE


#define SDC_INIT_OVERWRITE_INFO(aOverwriteInfo, aUpdateInfo)                           \
    IDE_DASSERT( (aOverwriteInfo) != NULL );                                           \
    IDE_DASSERT( (aUpdateInfo)    != NULL );                                           \
    (aOverwriteInfo)->mColInfoList             = (aUpdateInfo)->mColInfoList;          \
    (aOverwriteInfo)->mOldRowHdrInfo           = (aUpdateInfo)->mOldRowHdrInfo;        \
    (aOverwriteInfo)->mOldRowPieceSize         = (aUpdateInfo)->mOldRowPieceSize;      \
    (aOverwriteInfo)->mNewRowHdrInfo           = (aUpdateInfo)->mNewRowHdrInfo;        \
    (aOverwriteInfo)->mNewRowPieceSize         = (aUpdateInfo)->mNewRowPieceSize;      \
    (aOverwriteInfo)->mUptAftInModeColCnt      = 0;                                    \
    (aOverwriteInfo)->mUptAftLobDescCnt        = 0;                                    \
    (aOverwriteInfo)->mIsUptLobByAPI           = (aUpdateInfo)->mIsUptLobByAPI;        \
    (aOverwriteInfo)->mTrailingNullUptCount    = (aUpdateInfo)->mTrailingNullUptCount; \
    (aOverwriteInfo)->mLstColumnOverwriteSize  = 0;                                    \
    (aOverwriteInfo)->mUptInModeColCntBfrSplit = (aUpdateInfo)->mUptBfrInModeColCnt;   \
    (aOverwriteInfo)->mRowPieceSID             = (aUpdateInfo)->mRowPieceSID;

#define SDC_INIT_SUPPLEMENT_JOB_INFO(aSupplementJobInfo)                  \
    (aSupplementJobInfo).mJobType         = SDC_SUPPLEMENT_JOB_NONE;      \
    (aSupplementJobInfo).mNextRowPieceSID = SD_NULL_SID;

#define SDC_ADD_SUPPLEMENT_JOB(aJobInfo, aUptJobType)                 \
    (aJobInfo)->mJobType |= (aUptJobType) ;

#define SDC_EXIST_SUPPLEMENT_JOB( aSupplementJobInfo )                    \
    (( (aSupplementJobInfo)->mJobType != SDC_SUPPLEMENT_JOB_NONE )        \
    ? ID_TRUE : ID_FALSE )

#define SDC_NEED_RETRY_CHECK( aRetryInfo )                     \
    ( ( aRetryInfo->mStmtRetryColLst.mCurColumn != NULL ) ||   \
      ( aRetryInfo->mRowRetryColLst.mCurColumn != NULL ) )

/* PROJ-1784 DML without retry
 * Disk table용 Retry Info
 * 하나의 column list만 확인한다
 * 여러 page에 걸친 row를 비교하기 위해 사용 */
typedef struct sdcRetryCompColumns
{
    const smiColumnList * mCurColumn;
    const smiValue      * mCurValue;
    UInt                  mCurOffset;

}sdcRetryColumns;

/* PROJ-1784 DML without retry
 * Disk table용 Retry Info */
typedef struct sdcRetryInfo
{
    const smiDMLRetryInfo * mRetryInfo;
    idBool                  mIsAlreadyModified;
    void                  * mISavepoint;
    sdcRetryCompColumns     mStmtRetryColLst;
    sdcRetryCompColumns     mRowRetryColLst;
    UShort                  mColSeqInRow;

} sdcRetryInfo;

static UShort gColumnHeadSizeTbl[] = {
    SDC_SMALL_COLUMN_LEN_STORE_SIZE,                         /* SDC_NULL_COLUMN_PREFIX */
    0,                                                       /* SDC_LARGE_COLUMN_PREFIX */
    SDC_SMALL_COLUMN_LEN_STORE_SIZE + ID_SIZEOF(sdcLobDesc)  /* SDC_LOB_DESC_COLUMN_PREFIX */
};

class sdcRow
{
public:

    static IDE_RC insert( idvSQL           * aStatistics,
                          void             * aTrans,
                          void             * aTableInfoPtr,
                          void             * aTableHeader,
                          smSCN              aCSInfiniteSCN,
                          SChar            **, //aRetRow
                          scGRID           * aRowGRID,
                          const smiValue   * aValueList,
                          UInt               aFlag );

   // PROJ-1566
    static IDE_RC insertAppend( idvSQL            * aStatistics,
                                void              * aTrans,
                                void              * aDPathSegInfo,
                                void              * aTableHeader,
                                smSCN               aCSInfiniteSCN,
                                const smiValue    * aValueList,
                                scGRID            * aRowGRID );

    static IDE_RC update( idvSQL                * aStatistics,
                          void                  * aTrans,
                          smSCN                   aStmtViewSCN,
                          void                  * aTableInfoPtr,
                          void                  * aTableHeader,
                          SChar                 *,//aOldRow,
                          scGRID                  aSlotGRID,
                          SChar                **,//aRetRow,
                          scGRID                * aRetUpdateSlotGRID,
                          const smiColumnList   * aColumnList,
                          const smiValue        * aValueList,
                          const smiDMLRetryInfo * aDMLRetryInfo,
                          smSCN                   aCSInfiniteSCN,
                          sdcColInOutMode       * aValueModeList,
                          ULong                 * aModifyIdxBit,
                          idBool                  aForbiddenToRetry );

    static IDE_RC remove( idvSQL               * aStatistics,
                          void                 * aTrans,
                          smSCN                  aStmtViewSCN,
                          void                 * aTableInfoPtr,
                          void                 * aTableHeader,
                          SChar                * /* aRow */,
                          scGRID                 aSlotGRID,
                          smSCN                  aCSInfiniteSCN,
                          const smiDMLRetryInfo *aDMLRetryInfo,
                          idBool                 aIsDequeue, 
                          idBool                 aForbiddenToRetry );

    static IDE_RC fetch( idvSQL                      *aStatistics,
                         sdrMtx                      *aMtx,
                         sdrSavePoint                *aSP,
                         void                        *aTrans,
                         scSpaceID                    aTableSpaceID,
                         UChar                       *aSlotPtr,
                         idBool                       aIsPersSlot,
                         sdbPageReadMode              aPageReadMode,
                         const smiFetchColumnList    *aFetchColumnList,
                         smFetchVersion               aFetchVersion,
                         sdSID                        aMyTSSlotSID,
                         const smSCN                 *aMyStmtSCN,
                         const smSCN                 *aInfiniteSCN,
                         sdcIndexInfo4Fetch          *aIndexInfo4Fetch,
                         sdcLobInfo4Fetch            *aLobInfo4Fetch,
                         smcRowTemplate              *aRowTemplate,
                         UChar                       *aDestRowBuf,
                         idBool                      *aIsRowDeleted,
                         idBool                      *aIsPageLatchReleased,
                         idBool                       aIsSkipAssert = ID_FALSE );


    static IDE_RC lock( idvSQL       *aStatistics,
                        UChar        *aSlotPtr,
                        sdSID         aSlotSID,
                        smSCN        *aInfiniteSCN,
                        sdrMtx       *aMtx,
                        UInt          aCTSlotIdx,
                        idBool*       aSkipLockRec);

    static IDE_RC free( idvSQL      *aStatistics,
                        sdrMtx      *aMtx,
                        void        *aTableHeader,
                        scGRID       aSlotGRID,
                        UChar       *aSlotPtr );

    static IDE_RC restoreFreeSpaceCredit( sdrMtx      *aMtx,
                                          UChar       *aSlotPtr,
                                          UShort       aRestoreSize );

    static IDE_RC prepareUpdatePageBySID( idvSQL           * aStatistics,
                                          sdrMtx           * aMtx,
                                          scSpaceID          aSpaceID,
                                          sdSID              aRowPieceSID,
                                          sdbPageReadMode    aPageReadMode,
                                          UChar           ** aPagePtr,
                                          UChar            * aCTSlotIdx );

    static IDE_RC prepareUpdatePageByPID( idvSQL           * aStatistics,
                                          sdrMtx           * aMtx,
                                          scSpaceID          aSpaceID,
                                          scPageID           aPageID,
                                          sdbPageReadMode    aPageReadMode,
                                          UChar           ** aSlotPtr,
                                          UChar            * aCTSlotIdx );

    static IDE_RC writeInsertRowPieceRedoUndoLog(
                      UChar                       *aSlotPtr,
                      scGRID                       aSlotGRID,
                      sdrMtx                      *aMtx,
                      const sdcRowPieceInsertInfo *aInsertInfo,
                      idBool                       aReplicate );

    static IDE_RC writeInsertRowPieceLog4RP( const sdcRowPieceInsertInfo *aInsertInfo,
                                             sdrMtx                      *aMtx        );

    static IDE_RC writeUpdateRowPieceRedoUndoLog(
                      const UChar                 *aSlotPtr,
                      scGRID                       aSlotGRID,
                      const sdcRowPieceUpdateInfo *aUpdateInfo,
                      idBool                       aReplicate,
                      sdrMtx                      *aMtx );

    static UChar* writeUpdateRowPieceUndoRecRedoLog(
        UChar                       *aWritePtr,
        const UChar                 *aOldSlotPtr,
        const sdcRowPieceUpdateInfo *aUpdateInfo );

    static IDE_RC writeUpdateRowPieceCLR( const UChar    *aUndoRecHdr,
                                          scGRID          aSlotGRID,
                                          sdSID           aUndoSID,
                                          sdrMtx         *aMtx );

    static IDE_RC writeUpdateRowPieceLog4RP(
        const sdcRowPieceUpdateInfo *aUpdateInfo,
        idBool                       aIsUndoRec,
        sdrMtx                      *aMtx        );

    static IDE_RC writeOverwriteRowPieceRedoUndoLog(
        UChar                          *aSlotPtr,
        scGRID                          aSlotGRID,
        const sdcRowPieceOverwriteInfo *aOverwriteInfo,
        idBool                          aReplicate,
        sdrMtx                         *aMtx );

    static UChar* writeOverwriteRowPieceUndoRecRedoLog(
        UChar                          *aWritePtr,
        const UChar                    *aOldSlotPtr,
        const sdcRowPieceOverwriteInfo *aOverwriteInfo );

    static IDE_RC writeOverwriteRowPieceCLR(
        const UChar             *aUndoRecHdr,
        scGRID                   aSlotGRID,
        sdSID                    aUndoSID,
        sdrMtx                  *aMtx );

    static IDE_RC writeOverwriteRowPieceLog4RP(
        const sdcRowPieceOverwriteInfo *aOverwriteInfo,
        idBool                          aIsUndoRec,
        sdrMtx                         *aMtx        );

    static IDE_RC writeDeleteFstColumnPieceRedoUndoLog(
        const UChar                 *aSlotPtr,
        scGRID                       aSlotGRID,
        const sdcRowPieceUpdateInfo *aUpdateInfo,
        sdrMtx                      *aMtx );

    static UChar* writeDeleteFstColumnPieceRedoLog(
        UChar                       *aWritePtr,
        const UChar                 *aOldSlotPtr,
        const sdcRowPieceUpdateInfo *aUpdateInfo );

    static IDE_RC writeDeleteFstColumnPieceCLR(
        const UChar             *aUndoRecHdr,
        scGRID                   aSlotGRID,
        sdSID                    aUndoSID,
        sdrMtx                  *aMtx );

    static IDE_RC writeDeleteFstColumnPieceLog4RP(
        const sdcRowPieceUpdateInfo *aUpdateInfo,
        sdrMtx                      *aMtx );

    static IDE_RC writeDeleteRowPieceRedoUndoLog( UChar     *aSlotPtr,
                                                  scGRID     aSlotGRID,
                                                  idBool     aIsDelete4Upt,
                                                  SShort     aChangeSize,
                                                  sdrMtx    *aMtx );

    static IDE_RC writeDeleteRowPieceCLR(
                                UChar     *aSlotPtr,
                                scGRID     aSlotGRID,
                                sdrMtx    *aMtx );

    static IDE_RC writeDeleteRowPieceLog4RP(
        void                           *aTableHeader,
        const UChar                    *aSlotPtr,
        const sdcRowPieceUpdateInfo    *aUpdateInfo,
        sdrMtx                         *aMtx );

    static IDE_RC writeLockRowRedoUndoLog( UChar     *aSlotPtr,
                                           scGRID     aSlotGRID,
                                           sdrMtx    *aMtx,
                                           idBool     aIsExplicitLock );

    static IDE_RC writeLockRowCLR( const UChar    *aUndoRecHdr,
                                   scGRID          aSlotGRID,
                                   sdSID           aUndoSID,
                                   sdrMtx         *aMtx );

    static IDE_RC writePKLog( idvSQL             *aStatistics,
                              void               *aTrans,
                              void               *aTableHeader,
                              scGRID              aSlotGRID,
                              const sdcPKInfo    *aPKInfo );

    static IDE_RC canUpdateRowPiece(
                   idvSQL                *aStatistics,
                   sdrMtx                *aMtx,
                   sdrSavePoint          *aMtxSavePoint,
                   scSpaceID              aSpaceID,
                   sdSID                  aSlotSID,
                   sdbPageReadMode        aPageReadMode,
                   smSCN                 *aStmtViewSCN,
                   smSCN                 *aCSInfiniteSCN,
                   idBool                 aIsUptLobByAPI,
                   UChar                **aTargetRow,
                   sdcUpdateState        *aUptState,
                   UChar                 *aNewCTSlotIdx,
                   /* BUG-31359 
                    * SELECT ... FOR UPDATE NOWAIT command on disk table
                    * waits for commit of a transaction on other session.
                    *
                    * Add aLockWaitMicroSec argument from cursor property. */
                   ULong                  aLockWaitMicroSec );

    static IDE_RC canUpdateRowPieceInternal(
                              idvSQL            * aStatistics,
                              sdrMtxStartInfo   * aStartInfo,
                              UChar             * aTargetRow,
                              sdRID               aMyTSSlotRID,
                              sdbPageReadMode     aPageReadMode,
                              smSCN             * aStmtViewSCN,
                              smSCN             * aCSInfiniteSCN,
                              idBool              aIsUptLobByAPI,
                              sdcUpdateState    * aRetFlag,
                              smTID             * aWaitTransID );

    static IDE_RC releaseLatchForAlreadyModify( sdrMtx       * aMtx,
                                                sdrSavePoint * aMtxSavePoint );

    static IDE_RC checkRetryRemainedRowPiece( idvSQL         * aStatistics,
                                              void           * aTrans,
                                              scSpaceID        aSpaceID,
                                              sdSID            aNextRowPieceSID,
                                              idBool           aForbiddenToRetry,
                                              smSCN            aStmtViewSCN,
                                              sdcRetryInfo   * aRetryInfo );

    static IDE_RC checkRetry( void         * aTrans,
                              sdrMtx       * aMtx,
                              sdrSavePoint * aSP,
                              UChar        * aRowSlotPtr,
                              UInt           aRowFlag,
                              idBool         aForbiddenToRetry,
                              smSCN          aStmtViewSCN,
                              sdcRetryInfo * aRetryInfo,
                              UShort         aColCount );

    static idBool isSameColumnValue( UChar                * aSlotPtr,
                                     UInt                   aRowFlag,
                                     sdcRetryCompColumns  * aCompColumns,
                                     UShort                 aFstColSeq,
                                     UShort                 aColCount );

    static void setRetryInfo( const smiDMLRetryInfo * aDMLRetryInfo,
                              sdcRetryInfo          * aRetryInfo )
    {
        aRetryInfo->mRetryInfo                  = aDMLRetryInfo;
        aRetryInfo->mISavepoint                 = NULL;
        aRetryInfo->mIsAlreadyModified          = ID_FALSE;
        aRetryInfo->mStmtRetryColLst.mCurOffset = 0;
        aRetryInfo->mRowRetryColLst.mCurOffset  = 0;
        aRetryInfo->mColSeqInRow                = 0;

        if( aDMLRetryInfo != NULL )
        {
            aRetryInfo->mStmtRetryColLst.mCurColumn
                = aDMLRetryInfo->mStmtRetryColLst;
            aRetryInfo->mStmtRetryColLst.mCurValue
                = aDMLRetryInfo->mStmtRetryValLst;
            aRetryInfo->mRowRetryColLst.mCurColumn
                = aDMLRetryInfo->mRowRetryColLst;
            aRetryInfo->mRowRetryColLst.mCurValue
                = aDMLRetryInfo->mRowRetryValLst;
        }
        else
        {
            aRetryInfo->mStmtRetryColLst.mCurColumn = NULL;
            aRetryInfo->mStmtRetryColLst.mCurValue  = NULL;
            aRetryInfo->mRowRetryColLst.mCurColumn  = NULL;
            aRetryInfo->mRowRetryColLst.mCurValue   = NULL;
        }
    };

    static idBool isHeadRowPiece( const UChar    *aSlotPtr );

    static idBool isDeleted( const UChar    *aSlotPtr );

    static void getRowHdrInfo( const UChar      *aSlotPtr,
                               sdcRowHdrInfo    *aRowHdrInfo );

    static void getRowHdrExInfo( const UChar      *aSlotPtr,
                                 sdcRowHdrExInfo  *aRowHdrExInfo );

    static UShort getRowPieceSize( const UChar    *aSlotPtr );
    static UShort getRowPieceBodySize( const UChar    *aSlotPtr );

    static UShort getMinRowPieceSize();

    static sdSID getNextRowPieceSID( const UChar    *aSlotPtr );

    inline static UChar* getDataArea( const UChar    *aSlotPtr )
    {
        UChar     *sDataPtr;
        UChar      sRowFlag;
 
        IDE_DASSERT( aSlotPtr != NULL );
 
        SDC_GET_ROWHDR_1B_FIELD(aSlotPtr, SDC_ROWHDR_FLAG, sRowFlag);
 
        if ( SDC_IS_LAST_ROWPIECE(sRowFlag) == ID_TRUE )
        {
            sDataPtr = (UChar*)aSlotPtr + SDC_ROWHDR_SIZE;
        }
        else
        {
            sDataPtr = (UChar*)aSlotPtr +
                        SDC_ROWHDR_SIZE  +
                        SDC_EXTRASIZE_FOR_CHAINING;
        }
 
        return sDataPtr;
    }

    inline static UChar* getColPiece( const UChar      * aColPtr,
                                      UInt             * aColLen,
                                      UChar           ** aColVal, 
                                      sdcColInOutMode  * aColumnMode );

    static UInt getColPieceLen( const UChar    *aColPtr );

    inline static void getColPieceInfo( const UChar     * aColPtr,
                                        UChar           * aPrefix,
                                        UInt            * aColLen,
                                        UChar          ** aColVal,
                                        UShort          * aColStoreSize,
                                        sdcColInOutMode * aInOutMode );

    static UChar* getNxtColPiece( const UChar    *aColPtr );

    static void getColumnPiece( const UChar    *aSlotPtr,
                                UShort          aColumnSeq,
                                UChar          *aColumnValueBuf,
                                UShort          aColumnValueBufLen,
                                UShort         *aColumnLen );

    static UShort calcInsertRowPieceLogSize4RP(
        const sdcRowPieceInsertInfo *aInsertInfo );

    static UShort calcUpdateRowPieceLogSize(
        const sdcRowPieceUpdateInfo *aUpdateInfo,
        idBool                       aIsUndoRec,
        idBool                       aIsReplicate );

    static UShort calcUpdateRowPieceLogSize4RP(
        const sdcRowPieceUpdateInfo *aUpdateInfo,
        idBool                       aIsUndoRec );

    static UShort calcOverwriteRowPieceLogSize(
        const sdcRowPieceOverwriteInfo  *aOverwriteInfo,
        idBool                           aIsUndoRec,
        idBool                           aIsReplicate );

    static UShort calcOverwriteRowPieceLogSize4RP(
        const sdcRowPieceOverwriteInfo *aOverwriteInfo,
        idBool                          aIsUndoRec );

    static UShort calcDeleteFstColumnPieceLogSize(
        const sdcRowPieceUpdateInfo *aUpdateInfo,
        idBool                       aIsReplicate );

    static UShort calcDeleteRowPieceLogSize4RP(
        const UChar                    *aSlotPtr,
        const sdcRowPieceUpdateInfo    *aUpdateInfo );

    static idBool isMyTransUpdating( UChar    * aPagePtr,
                                     UChar    * aSlotPtr,
                                     smSCN      aMyFstDskViewSCN,
                                     sdRID      aMyTSSlotRID,
                                     smSCN    * aFstDskViewSCN );

    static idBool isMyLegacyTransUpdated( void    * aTrans,
                                          smSCN     aLegacyCSInfSCN,
                                          smSCN     aRowInfSCN,
                                          smSCN     aRowCommitSCN );

    static IDE_RC insertRowPiece( idvSQL                      *aStatistics,
                                  void                        *aTrans,
                                  void                        *aTableInfoPtr,
                                  void                        *aTableHeader,
                                  const sdcRowPieceInsertInfo *aInsertInfo,
                                  UChar                        aRowFlag,
                                  sdSID                        aNextRowPieceSID,
                                  smSCN                       *aCSInfiniteSCN,
                                  idBool                       aIsNeedUndoRec,
                                  idBool                       aIsUndoLogging,
                                  idBool                       aReplicate,
                                  sdSID                       *aInsertRowPieceSID );

    static IDE_RC insertRowPiece4Append( idvSQL                      *aStatistics,
                                         void                        *aTrans,
                                         void                        *aTableHeader,
                                         sdpDPathSegInfo             *aDPathSegInfo,
                                         const sdcRowPieceInsertInfo *aInsertInfo,
                                         UChar                        aRowFlag,
                                         sdSID                        aNextRowPieceSID,
                                         smSCN                       *aCSInfiniteSCN,
                                         sdSID                       *aInsertRowPieceSID );

    /* BUG-39507 */
    static IDE_RC isUpdatedRowBySameStmt( idvSQL           * aStatistics,
                                          void             * aTrans,
                                          smSCN              aStmtViewSCN,
                                          void             * aTableHeader,
                                          scGRID             aSlotGRID,
                                          smSCN              aCSInfiniteSCN,
                                          idBool           * aIsUpdatedRowBySameStmt );

    /* BUG-39507 */
    static IDE_RC isUpdatedRowPieceBySameStmt( idvSQL          *aStatistics,
                                               void            *aTrans,
                                               void            *aTableHeader,
                                               smSCN           *aStmtViewSCN,
                                               smSCN           *aCSInfiniteSCN,
                                               sdSID            aCurrRowPieceSID,
                                               idBool          *aIsUpdatedRowBySameStmt );

    static IDE_RC updateRowPiece( idvSQL              *aStatistics,
                                  void                *aTrans,
                                  void                *aTableInfoPtr,
                                  void                *aTableHeader,
                                  smSCN               *aStmtViewSCN,
                                  smSCN               *aCSInfiniteSCN,
                                  const smiColumnList *aColList,
                                  const smiValue      *aValueList,
                                  sdcRetryInfo        *aRetryInfo,
                                  const sdSID          aPrevRowPieceSID,
                                  sdSID                aCurrRowPieceSID,
                                  idBool               aReplicate,
                                  sdcColInOutMode     *aValueModeList,
                                  idBool               aForbiddenToRetry,
                                  sdcPKInfo           *aPKInfo,
                                  sdcRowUpdateStatus  *aUpdateStatus,
                                  sdSID               *aNextRowPieceSID,
                                  sdSID               *aLstInsertRowPieceSID,
                                  idBool              *aIsRowPieceDeletedByItSelf );
    
    static IDE_RC doUpdate( idvSQL                      *aStatistics,
                            void                        *aTrans,
                            void                        *aTableInfoPtr,
                            void                        *aTableHeader,
                            sdrMtx                      *aMtx,
                            sdrMtxStartInfo             *aStartInfo,
                            UChar                       *aSlotPtr,
                            scGRID                       aSlotGRID,
                            const sdcRowPieceUpdateInfo *aUpdateInfo,
                            UShort                       aFstColumnSeq,
                            idBool                       aReplicate,
                            sdSID                        aNextRowPieceSID,
                            sdcRowPieceOverwriteInfo    *aOverwriteInfo,
                            sdcSupplementJobInfo        *aSupplementJobInfo,
                            sdSID                       *aLstInsertRowPieceSID );

    static IDE_RC
        doSupplementJob( idvSQL                            *aStatistics,
                         sdrMtxStartInfo                   *aStartInfo,
                         void                              *aTableHeader,
                         scGRID                             aSlotGRID,
                         smSCN                              aCSInfiniteSCN,
                         sdSID                              aPrevRowPieceSID,
                         const sdcSupplementJobInfo        *aSupplementJobInfo,
                         const sdcRowPieceUpdateInfo       *aUpdateInfo,
                         const sdcRowPieceOverwriteInfo    *aOverwriteInfo );

    static IDE_RC updateInplace( idvSQL                      *aStatistics,
                                 void                        *aTableHeader,
                                 sdrMtx                      *aMtx,
                                 sdrMtxStartInfo             *aStartInfo,
                                 UChar                       *aSlotPtr,
                                 scGRID                       aSlotGRID,
                                 const sdcRowPieceUpdateInfo *aUpdateInfo,
                                 idBool                       aReplicate );

    static IDE_RC updateOutplace( idvSQL                      *aStatistics,
                                  void                        *aTableHeader,
                                  sdrMtx                      *aMtx,
                                  sdrMtxStartInfo             *aStartInfo,
                                  UChar                       *aOldSlotPtr,
                                  scGRID                       aSlotGRID,
                                  const sdcRowPieceUpdateInfo *aUpdateInfo,
                                  sdSID                        aNextRowPieceSID,
                                  idBool                       aReplicate );

    static IDE_RC redo_undo_UPDATE_INPLACE_ROW_PIECE(
                      sdrMtx              *aMtx,
                      UChar               *aLogPtr,
                      UChar               *aSlotPtr,
                      sdcOperToMakeRowVer  aOper4RowPiece );

    static IDE_RC redo_undo_UPDATE_OUTPLACE_ROW_PIECE(
                                 sdrMtx             *aMtx,
                                 UChar              *aLogPtr,
                                 UChar              *aSlotPtr,
                                 sdSID               aSlotSID,
                                 sdcOperToMakeRowVer aOper2MakeRowVer,
                                 UChar              *aRowBuf4MVCC,
                                 UChar             **aNewSlotPtr4Undo,
                                 SShort             *aFSCreditSize );

    static IDE_RC overwriteRowPiece( idvSQL                         *aStatistics,
                                     void                           *aTableHeader,
                                     sdrMtx                         *aMtx,
                                     sdrMtxStartInfo                *aStartInfo,
                                     UChar                          *aOldSlotPtr,
                                     scGRID                          aSlotGRID,
                                     const sdcRowPieceOverwriteInfo *aOverwriteInfo,
                                     sdSID                           aNextRowPieceSID,
                                     idBool                          aReplicate );

    static IDE_RC redo_undo_OVERWRITE_ROW_PIECE(
                                     sdrMtx             *aMtx,
                                     UChar              *aLogPtr,
                                     UChar              *aSlotPtr,
                                     sdSID               aSlotSID,
                                     sdcOperToMakeRowVer aOper2MakeRowVer,
                                     UChar              *aRowBuf4MVCC,
                                     UChar             **aNewSlotPtr4Undo,
                                     SShort             *aFSCreditSize );

    static IDE_RC processOverflowData( idvSQL                      *aStatistics,
                                       void                        *aTrans,
                                       void                        *aTableInfoPtr,
                                       void                        *aTableHeader,
                                       sdrMtx                      *aMtx,
                                       sdrMtxStartInfo             *aStartInfo,
                                       UChar                       *aSlotPtr,
                                       scGRID                       aSlotGRID,
                                       const sdcRowPieceUpdateInfo *aUpdateInfo,
                                       UShort                       aFstColumnSeq,
                                       idBool                       aReplicate,
                                       sdSID                        aNextRowPieceSID,
                                       sdcRowPieceOverwriteInfo    *aOverwriteInfo,
                                       sdcSupplementJobInfo        *aSupplementJobInfo,
                                       sdSID                       *aLstInsertRowPieceSID );

    static IDE_RC processRemainData( idvSQL                      *aStatistics,
                                     void                        *aTrans,
                                     void                        *aTableInfoPtr,
                                     void                        *aTableHeader,
                                     sdrMtx                      *aMtx,
                                     sdrMtxStartInfo             *aStartInfo,
                                     UChar                       *aSlotPtr,
                                     scGRID                       aSlotGRID,
                                     const sdcRowPieceInsertInfo *aInsertInfo,
                                     const sdcRowPieceUpdateInfo *aUpdateInfo,
                                     idBool                       aReplicate,
                                     sdSID                        aNextRowPieceSID,
                                     UChar                        aNextRowFlag,
                                     sdcRowPieceOverwriteInfo    *aOverwriteInfo,
                                     sdcSupplementJobInfo        *aSupplementJobInfo,
                                     sdSID                       *aInsertRowPieceSID );

    static IDE_RC changeRowPiece( idvSQL                      *aStatistics,
                                  void                        *aTrans,
                                  void                        *aTableInfoPtr,
                                  void                        *aTableHeader,
                                  sdrMtx                      *aMtx,
                                  sdrMtxStartInfo             *aStartInfo,
                                  UChar                       *aSlotPtr,
                                  scGRID                       aSlotGRID,
                                  const sdcRowPieceUpdateInfo *aUpdateInfo,
                                  const sdcRowPieceInsertInfo *aInsertInfo,
                                  idBool                       aReplicate,
                                  sdSID                        aNextRowPieceSID,
                                  UChar                        aNextRowFlag, 
                                  sdcSupplementJobInfo        *aSupplementJobInfo,
                                  sdSID                       *aInsertRowPieceSID );

    static IDE_RC migrateRowPieceData( idvSQL                      *aStatistics,
                                       void                        *aTrans,
                                       void                        *aTableInfoPtr,
                                       void                        *aTableHeader,
                                       sdrMtx                      *aMtx,
                                       sdrMtxStartInfo             *aStartInfo,
                                       UChar                       *aSlotPtr,
                                       scGRID                       aSlotGRID,
                                       const sdcRowPieceInsertInfo *aInsertInfo,
                                       const sdcRowPieceUpdateInfo *aUpdateInfo,
                                       idBool                       aReplicate,
                                       sdSID                        aNextRowPieceSID,
                                       UChar                        aNextRowFlag,
                                       sdcRowPieceOverwriteInfo    *aOverwriteInfo,
                                       sdSID                       *aInsertRowPieceSID );

    static IDE_RC truncateRowPieceData(
                                  idvSQL                         *aStatistics,
                                  void                           *aTableHeader,
                                  sdrMtx                         *aMtx,
                                  sdrMtxStartInfo                *aStartInfo,
                                  UChar                          *aSlotPtr,
                                  scGRID                          aSlotGRID,
                                  const sdcRowPieceUpdateInfo    *aUpdateInfo,
                                  sdSID                           aNextRowPieceSID,
                                  idBool                          aReplicate,
                                  sdcRowPieceOverwriteInfo       *aOverwriteInfo );

    static IDE_RC changeRowPieceLink( idvSQL               * aStatistics,
                                      void                 * aTrans,
                                      void                 * aTableHeader,
                                      smSCN                * aCSInfiniteSCN,
                                      sdSID                  aSlotSID,
                                      sdSID                  aNextRowPieceSID );

    static IDE_RC truncateNextLink( sdSID                   aSlotSID,
                                    UChar                  *aOldSlotPtr,
                                    const sdcRowHdrInfo    *aNewRowHdrInfo,
                                    UChar                 **aNewSlotPtr );

    static IDE_RC writeChangeRowPieceLinkRedoUndoLog(
                                    UChar          *aSlotPtr,
                                    scGRID          aSlotGRID,
                                    sdrMtx         *aMtx,
                                    sdSID           aNextRowPieceSID );

    static IDE_RC undo_CHANGE_ROW_PIECE_LINK( 
                                   sdrMtx            *aMtx,
                                   UChar             *aLogPtr,
                                   UChar             *aSlotPtr,
                                   sdSID              aSlotSID,
                                   sdcOperToMakeRowVer aOper4RowPiece,
                                   UChar             *aRowBuf4MVCC,
                                   UChar            **aNewSlotPtr4Undo,
                                   SShort            *aFSCreditSize );

    static IDE_RC writeChangeRowPieceLinkCLR(
                             const UChar             *aUndoRecHdr,
                             scGRID                   aSlotGRID,
                             sdSID                    aUndoSID,
                             sdrMtx                  *aMtx );

    static void initRowUpdateStatus( const smiColumnList     *aColumnList,
                                     sdcRowUpdateStatus      *aUpdateStatus );

    static void resetRowUpdateStatus( const sdcRowHdrInfo          *aRowHdrInfo,
                                      const sdcRowPieceUpdateInfo  *aUpdateInfo,
                                      sdcRowUpdateStatus           *aUpdateStatus );

    static idBool isUpdateEnd( sdSID                       aNextRowPieceSID,
                               const sdcRowUpdateStatus   *aUpdateStatus,
                               idBool                      aReplicate,
                               const sdcPKInfo            *aPKInfo );

    static void calcColumnDescSet( const sdcRowPieceUpdateInfo *aUpdateInfo,
                                   UShort                       aColCount,
                                   sdcColumnDescSet            *aColDescset);

    static UChar calcColumnDescSetSize( UShort                  aUptColCount );

    static IDE_RC deleteFstColumnPiece(
                            idvSQL                       *aStatistics,
                            void                         *aTrans,
                            void                         *aTableHeader,
                            smSCN                        *aCSInfiniteSCN,
                            sdrMtx                       *aMtx,
                            sdrMtxStartInfo              *aStartInfo,
                            UChar                        *aOldSlotPtr,
                            sdSID                         aCurrRowPieceSID,
                            const sdcRowPieceUpdateInfo  *aUpdateInfo,
                            sdSID                         aNextRowPieceSID,
                            idBool                        aReplicate );

    static IDE_RC undo_DELETE_FIRST_COLUMN_PIECE(
                                        sdrMtx               *aMtx,
                                        UChar                *aLogPtr,
                                        UChar                *aSlotPtr,
                                        sdSID                 aSlotSID,
                                        sdcOperToMakeRowVer   aOper4RowPiece,
                                        UChar                *aRowBuf4MVCC,
                                        UChar              **aNewSlotPtr4Undo );

    static IDE_RC removeRowPiece( idvSQL                *aStatistics,
                                  void                  *aTrans,
                                  void                  *aTableHeader,
                                  sdSID                  aSlotSID,
                                  smSCN                 *aStmtViewSCN,
                                  smSCN                 *aCSInfiniteSCN,
                                  sdcRetryInfo          *aDMLRetryInfo,
                                  idBool                 aReplicate,
                                  idBool                 aForbiddenToRetry,
                                  sdcPKInfo             *aPKInfo,
                                  UShort                *aFstColumnSeq,
                                  idBool                *aSkipFlag,
                                  sdSID                 *aNextRowPieceSID);
    
    static IDE_RC removeRowPiece4Upt(
                      idvSQL                         *aStatistics,
                      void                           *aTableHeader,
                      sdrMtx                         *aMtx,
                      sdrMtxStartInfo                *aStartInfo,
                      UChar                          *aDeleteSlot,
                      const sdcRowPieceUpdateInfo    *aUpdateInfo,
                      idBool                          aReplicate );

    static UShort calcPKLogSize( const sdcPKInfo    *aPKInfo );

    static idBool existLOBCol( sdcRetryInfo  * aRetryInfo );

    static IDE_RC fetchRowPiece(
                            idvSQL                     * aStatistics,
                            void                       * aTrans,
                            scSpaceID                    aTableSpaceID,
                            UChar                      * aCurrSlotPtr,
                            idBool                       aIsPersSlot,
                            const smiFetchColumnList   * aFetchColumnList,
                            smFetchVersion               aFetchVersion,
                            sdSID                        aMyTSSlotSID,
                            sdbPageReadMode              aPageReadMode,
                            const smSCN                * aMyStmtSCN,
                            const smSCN                * aInfiniteSCN,
                            sdcIndexInfo4Fetch         * aIndexInfo4Fetch,
                            sdcLobInfo4Fetch           * aLobInfo4Fetch,
                            smcRowTemplate             * aRowTemplate,
                            sdcRowFetchStatus          * aFetchStatus,
                            UChar                      * aDestRowBuf,
                            idBool                     * aIsRowDeleted,
                            sdSID                      * aNextRowPieceSID,
                            idBool                       aIsSkipAssert = ID_FALSE );

    static IDE_RC doFetch( UChar               * aColPiecePtr,
                           smcRowTemplate      * aRowTemplate,
                           sdcRowFetchStatus   * aFetchStatus,
                           sdcLobInfo4Fetch    * aLobInfo4Fetch,
                           sdcIndexInfo4Fetch  * aIndexInfo4Fetch,
                           sdSID                 aUndoSID,
                           sdcRowHdrInfo       * aRowHdrInfo,
                           UChar               * aDestRowBuf,
                           UShort              * aFetchColumnCount,
                           UShort              * aLstFetchedColumnLen,
                           smiFetchColumnList ** aLstFetchedColumn,
                           idBool                aIsSkipAssert = ID_FALSE );

    static IDE_RC doFetchTrailingNull( const smiFetchColumnList * aFetchColumnList,
                                       sdcIndexInfo4Fetch       * aIndexInfo4Fetch,
                                       sdcLobInfo4Fetch         * aLobInfo4Fetch,
                                       sdcRowFetchStatus        * aFetchStatus,
                                       UChar                    * aDestRowBuf );

    static void initRowFetchStatus( const smiFetchColumnList * aFetchColumnList,
                                    sdcRowFetchStatus        * aFetchStatus );

    static void resetRowFetchStatus( const sdcRowHdrInfo  * aRowHdrInfo,
                                     UShort                 aFetchColumnCnt,
                                     smiFetchColumnList   * aLstFetchedColumn,
                                     UShort                 aLstFetchedColumnLen,
                                     sdcRowFetchStatus    * aFetchStatus );

    static IDE_RC lockRow( idvSQL                      *aStatistics,
                           sdrMtx                      *aMtx,
                           sdrMtxStartInfo             *aStartInfo,
                           smSCN                       *aCSInfiniteSCN,
                           smOID                        aTableOID,
                           UChar                       *aSlotPtr,
                           sdSID                        aSlotSID,
                           UChar                        aCTSlotIdxBfrLock,
                           UChar                        aCTSlotIdxAftLock,
                           idBool                       aIsExplicitLock );

    static IDE_RC redo_undo_LOCK_ROW( sdrMtx              *aMtx,
                                      UChar               *aLogPtr,
                                      UChar               *aSlotPtr,
                                      sdcOperToMakeRowVer  aOper4RowPiece );

    static IDE_RC isAlreadyMyLockExistOnRow( sdSID     * aMyTSSlotSID,
                                             smSCN     * aMyFstDskViewSCN,
                                             UChar     * aSlotPtr,
                                             idBool    * aRetExist);

    static IDE_RC removeOldLobPage4Upt( idvSQL                       *aStatistics,
                                        void                         *aTrans,
                                        const sdcRowPieceUpdateInfo  *aUpdateInfo );

    static IDE_RC removeAllLobCols( idvSQL                         *aStatistics,
                                    void                           *aTrans,
                                    const sdcLobInfoInRowPiece     *aLobInfo );

    static UChar* writeRowPiece4IRP(
                      UChar                        *aWritePtr,
                      const sdcRowHdrInfo          *aRowHdrInfo,
                      const sdcRowPieceInsertInfo  *aInsertInfo,
                      sdSID                         aNextRowPieceSID );

    static UChar* writeRowPiece4URP(
                      UChar                        *aWritePtr,
                      const sdcRowHdrInfo          *aRowHdrInfo,
                      const sdcRowPieceUpdateInfo  *aUpdateInfo,
                      sdSID                         aNextRowPieceSID );

    static UChar* writeRowPiece4ORP(
                      UChar                           *aWritePtr,
                      const sdcRowHdrInfo             *aRowHdrInfo,
                      const sdcRowPieceOverwriteInfo  *aOverwriteInfo,
                      sdSID                            aNextRowPieceSID );

    static UChar* writeRowHdr( UChar                *aWritePtr,
                               const sdcRowHdrInfo  *aRowHdrInfo );

    static UChar* writeRowHdrExInfo( UChar                  *aDestPtr,
                                     const sdcRowHdrExInfo  *aRowHdrExInfo );

    static UChar* writeNextLink( UChar    *aWritePtr,
                                 sdSID     aNextRowPieceSID );

    static UChar* writeColPiece4Ins( UChar                       *aWritePtr,
                                     const sdcRowPieceInsertInfo *aInsertInfo,
                                     UShort                       aColSeq );

    static UChar* writeColPiece( UChar                *aWritePtr,
                                 const UChar          *aColValuePtr,
                                 UShort                aColLen,
                                 sdcColInOutMode       aInOutMode );

    static IDE_RC writeColPieceLog( sdrMtx            *aMtx,
                                    UChar             *aColValuePtr,
                                    UShort             aColLen,
                                    sdcColInOutMode    aInOutMode );

    static IDE_RC analyzeRowForInsert( sdpPageListEntry          *aPageListEntry,
                                       UShort                     aCurrColSeq,
                                       UInt                       aCurrOffset,
                                       sdSID                      aNextRowPieceSID,
                                       smOID                      aTableOID,
                                       sdcRowPieceInsertInfo     *aInsertInfo );

    static IDE_RC analyzeRowForUpdate( UChar                     *aSlotPtr,
                                       const smiColumnList       *aColList,
                                       const smiValue            *aValueList,
                                       UShort                     aColCount,
                                       UShort                     aFstColumnSeq,
                                       UShort                     aLstUptColumnSeq,
                                       sdcRowPieceUpdateInfo     *aUpdateInfo );

    static IDE_RC analyzeRowForLobRemove( void                    *aTableHeader,
                                          UChar                   *aSlotPtr,
                                          UShort                   aFstColumnSeq,
                                          UShort                   aColCount,
                                          sdcLobInfoInRowPiece    *aLobInfo );

    /* TASK-5030 */
    static IDE_RC analyzeRowForDelete4RP( void                  * aTableHeader,
                                          UShort                  aColCount,
                                          UShort                  aFstColumnSeq,
                                          sdcRowPieceUpdateInfo * aUpdateInfo );

    static void getRowUndoSID( const UChar    * aSlotPtr,
                               sdSID          * aUndoSID );

    static UInt getColDataSize2Store(
                     const sdcRowPieceInsertInfo  *aInsertInfo,
                     UShort                        aColSeq );

    static UShort getTrailingNullCount(
                     const sdcRowPieceInsertInfo *aInsertInfo,
                     UShort                       aTotalColCount );

    static idBool isDivisibleColumn(const sdcColumnInfo4Insert    *aColumn);

    static void makeInsertInfo( void                   *aTableHeader,
                                const smiValue         *aValueList,
                                UShort                  aTotalColCount,
                                sdcRowPieceInsertInfo  *aInsertInfo );

    static void makeInsertInfoFromUptInfo(
                    void                          *aTableHeader,
                    const sdcRowPieceUpdateInfo   *aUpdateInfo,
                    UShort                         aColCount,
                    UShort                         aFstColumnSeq,
                    sdcRowPieceInsertInfo         *aInsertInfo );

    static IDE_RC makeUpdateInfo( UChar                   * aSlotPtr,
                                  const smiColumnList     * aColList,
                                  const smiValue          * aValueList,
                                  sdSID                     aSlotSID,
                                  UShort                    aColCount,
                                  UShort                    aFstColumnSeq,
                                  sdcColInOutMode         * aValueModeList,
                                  sdcRowPieceUpdateInfo   * aUpdateInfo );

    static void makeOverwriteInfo(
                    const sdcRowPieceInsertInfo    *aInsertInfo,
                    const sdcRowPieceUpdateInfo    *aUpdateInfo,
                    sdSID                           aNextRowPieceSID,
                    sdcRowPieceOverwriteInfo       *aOverwriteInfo );

    static void makePKInfo( void         *aTableHeader,
                            sdcPKInfo    *aPKInfo );

    static void copyPKInfo( const UChar                   *aSlotPtr,
                            const sdcRowPieceUpdateInfo   *aUpdateInfo,
                            UShort                         aColCount,
                            sdcPKInfo                     *aPKInfo );

    static IDE_RC makeTrailingNullUpdateInfo(
        void                  * aTableHeader,
        sdcRowHdrInfo         * aRowHdrInfo,
        sdcRowPieceUpdateInfo * aUpdateInfo,
        UShort                  aFstColumnSeq,
        const smiColumnList   * aColList,
        const smiValue        * aValueList,
        const sdcColInOutMode * aValueModeList );

    static UChar calcRowFlag4Insert( const sdcRowPieceInsertInfo *aInsertInfo,
                                     sdSID                        aNextRowPieceSID );

    static UChar calcRowFlag4Update( UChar                        aInheritRowFlag,
                                     const sdcRowPieceInsertInfo *aInsertInfo,
                                     sdSID                        aNextRowPieceSID );

    static idBool canReallocSlot( UChar    *aSlotPtr,
                                  UInt      aNewSlotSize );

    static IDE_RC reallocSlot( sdSID           aSlotSID,
                               UChar          *aOldSlotPtr,
                               UShort          aNewSlotSize,
                               idBool          aReserveFreeSpaceCredit,
                               UChar         **aNewSlotPtr );

    static SShort calcFSCreditSize( UShort    aOldRowPieceSize,
                                    UShort    aNewRowPieceSize );

#ifdef DEBUG
    static idBool validateSmiValue( const smiValue  *aValueList,
                                    UInt             aCount );

    static idBool validateSmiColumnList( const smiColumnList   *aColumnList );

    static idBool validateSmiFetchColumnList( const smiFetchColumnList   *aFetchColumnList );
#endif

    static idBool validateRowFlag( UChar    aRowFlag,
                                   UChar    aNextRowFlag );

    static idBool validateRowFlagForward( UChar    aRowFlag,
                                          UChar    aNextRowFlag );

    static idBool validateRowFlagBackward( UChar   aRowFlag,
                                           UChar   aNextRowFlag );

    static idBool validateInsertInfo( sdcRowPieceInsertInfo * aInsertInfo,
                                      UShort                  aColCount,
                                      UShort                  aFstColumnSeq );

    static IDE_RC getValidVersion( idvSQL              * aStatistics,
                                   UChar               * aSlotPtr,
                                   idBool                aIsPersSlot,
                                   smFetchVersion        aFetchVersion,
                                   sdSID                 aMyTSSlotSID,
                                   sdbPageReadMode       aPageReadMode,
                                   const smSCN         * aMyViewSCN,
                                   const smSCN         * aCSInfiniteSCN,
                                   void                * aTrans,
                                   sdcLobInfo4Fetch    * aLobInfo4Fetch,
                                   idBool              * aDoMakeOldVersion,
                                   sdSID               * aUndoSID,
                                   UChar               * aRowBuf);

    static IDE_RC makeOldVersionWithFix( idvSQL         *aStatistics,
                                         sdSID           aUndoSID,
                                         smOID           aTableOID,
                                         UChar          *aDestSlotPtr,
                                         idBool         *aContinue );

    static IDE_RC makeOldVersion( UChar          *aUndoRecHdr,
                                  smOID           aTableOID,
                                  UChar          *aDestSlotPtr,
                                  idBool         *aContinue );

    static IDE_RC getPKInfo( idvSQL       *aStatistics,
                             void         *aTrans,
                             scSpaceID     aTableSpaceID,
                             void         *aTableHeader,
                             UChar        *aSlotPtr,
                             sdSID         aSlotSID,
                             sdcPKInfo    *aPKInfo );

    static IDE_RC getPKInfoInRowPiece( UChar         *aSlotPtr,
                                       sdSID          aSlotSID,
                                       sdcPKInfo     *aPKInfo,
                                       sdSID         *aNextRowPieceSID );

    /*
     * PROJ-2047 Strengthening LOB
     */
    
    static IDE_RC writeAllLobCols( idvSQL                      * aStatistics,
                                   void                        * aTrans,
                                   void                        * aTableHeader,
                                   const sdcRowPieceInsertInfo * aInsertInfo,
                                   scGRID                        aRowGRID );

    static IDE_RC updateAllLobCols( idvSQL                      * aStatistics,
                                    sdrMtxStartInfo             * aStartInfo,
                                    void                        * aTableHeader,
                                    const sdcRowPieceInsertInfo * aInsertInfo,
                                    scGRID                        aRowGRID );

    static IDE_RC updateAllLobCols( idvSQL                      * aStatistics,
                                    sdrMtxStartInfo             * aStartInfo,
                                    void                        * aTableHeader,
                                    const sdcRowPieceUpdateInfo * aUpdateInfo,
                                    UShort                        aColCount,
                                    scGRID                        aRowGRID );

    static IDE_RC updateAllLobCols( idvSQL                         * aStatistics,
                                    sdrMtxStartInfo                * aStartInfo,
                                    void                           * aTableHeader,
                                    const sdcRowPieceOverwriteInfo * aOverwriteInfo,
                                    UShort                           aColCount,
                                    scGRID                           aRowGRID );
    
    static IDE_RC writeLobColUsingSQL( idvSQL          * aStatistics,
                                       void            * aTrans,
                                       smLobViewEnv    * aLobViewEnv,
                                       UInt              aOffset,
                                       UInt              aPieceLen,
                                       UChar           * aPieceVal,
                                       smrContType       aContType );
    
    static IDE_RC getLobDesc4Write( idvSQL             * aStatistics,
                                    const smLobViewEnv * aLobViewEnv,
                                    sdrMtx             * aMtx,
                                    UChar             ** aRow,
                                    UChar             ** aLobDesc,
                                    UShort             * aLobColSeqInRowPiece,
                                    UChar              * aCTSlotIdx );
    
    static void getLobDescInRowPiece( UChar    * aRow,
                                      UShort     aColSeqInRowPiece,
                                      UChar   ** aLobDesc );

    static IDE_RC updateLobDesc( idvSQL        * aStatistics,
                                 void          * aTrans,
                                 smLobViewEnv  * aLobViewEnv,
                                 smrContType     aContType );
    

    //TASK-4007 [SM]PBT를 위한 기능 추가
    //Page로부터 Row를 Dump하여 보여준다.
    static IDE_RC dump( UChar *aPage ,
                        SChar *aOutBuf ,
                        UInt   aOutSize );

    static void makeLobColumnInfo( sdcLobInfo4Fetch           * aLobInfo4Fetch,
                                   sdcLobDesc                 * aLobDesc,
                                   UInt                       * aCopyOffset,
                                   sdcValue4Fetch             * aFetchColumnValue );

    static void dumpErrorDiskColumnSize( UChar           * aColPiecePtr,
                                         const smiColumn * aFetchColumn,
                                         sdSID             aUndoSID,
                                         UChar           * aDestRowBuf,
                                         sdcValue4Fetch  * aFetchColumnValue,
                                         UInt              aCopyOffset,
                                         UShort            aColSeqInRowPiece,
                                         sdcRowHdrInfo   * aRowHdrInfo );

    static void dumpErrorCopyDiskColumn( UChar           * aColPiecePtr,
                                         const smiColumn * aFetchColumn,
                                         sdSID             aUndoSID,
                                         sdcValue4Fetch  * aFetchColumnValue,
                                         UInt              aCopyOffset,
                                         UShort            aColSeqInRowPiece,
                                         sdcRowHdrInfo   * aRowHdrInfo );

    /* 
     * BUG-37529 [sm-disk-collection] [DRDB] The change row piece logic
     * generates invalid undo record.
     * GRID를 이용해 slot의 포인터를 구한다.
     */
    static IDE_RC getSlotPtr( sdrMtx  * aMtx,
                              scGRID    aSlotGRID,
                              UChar  ** aSlotPtr )
    {
        UChar * sSlotDirPtr;
        UChar * sPagePtr;

        IDE_ASSERT( aMtx != NULL );

        sPagePtr = sdrMiniTrans::getPagePtrFromPageID( 
                                                aMtx,
                                                SC_MAKE_SPACE( aSlotGRID ),
                                                SC_MAKE_PID( aSlotGRID ) );
        /* 위쪽 함수에서 이미 GetPage한 Page여야 함 */
        IDE_ASSERT( sPagePtr != NULL );

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sPagePtr );
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                    sSlotDirPtr,
                                                    SC_MAKE_SLOTNUM( aSlotGRID ),
                                                    aSlotPtr )
                  != IDE_SUCCESS );

        return IDE_SUCCESS;
        
        IDE_EXCEPTION_END;

        return IDE_FAILURE;
    }

    static void getColumnValue( UChar           * aColPiecePtr,
                                UShort            aAdjustOffet,
                                idBool            aIsVariableColumn,
                                smcColTemplate  * aColTemplate,
                                sdcValue4Fetch  * aFetchColumnValue )
    {
        const UChar * sColStartPtr = aColPiecePtr + aColTemplate->mColStartOffset + aAdjustOffet;
        
        IDE_DASSERT( aColPiecePtr      != NULL );
        IDE_DASSERT( aColTemplate      != NULL );
        IDE_DASSERT( aFetchColumnValue != NULL );

        if ( aIsVariableColumn == ID_TRUE )
        {
            getColPieceInfo( sColStartPtr,
                             NULL, /* aPrefix */
                             &aFetchColumnValue->mValue.length,
                             (UChar**)&aFetchColumnValue->mValue.value,
                             &aFetchColumnValue->mColLenStoreSize,
                             &aFetchColumnValue->mInOutMode );
        }
        else
        {
#ifdef DEBUG
            UShort          sColLenSoreSize;
            UInt            sColLen;
            sdcColInOutMode sInOutMode;

            getColPieceInfo( sColStartPtr,
                             NULL, // aPrefix
                             &sColLen,
                             NULL,
                             &sColLenSoreSize,
                             &sInOutMode );

            IDE_ASSERT( sInOutMode      == SDC_COLUMN_IN_MODE );
            IDE_ASSERT( sColLen         == aColTemplate->mStoredSize );
            IDE_ASSERT( sColLenSoreSize == aColTemplate->mColLenStoreSize );
#endif
            aFetchColumnValue->mValue.length    = aColTemplate->mStoredSize;
            aFetchColumnValue->mValue.value     = sColStartPtr + aColTemplate->mColLenStoreSize; 
            aFetchColumnValue->mColLenStoreSize = aColTemplate->mColLenStoreSize;
        }
    }

    static UShort getColumnStoredLen( UChar * aColStartPtr )
    {
        UShort sColLen;

        IDE_DASSERT( aColStartPtr != NULL );

        sColLen = ((UShort)*aColStartPtr + SDC_SMALL_COLUMN_LEN_STORE_SIZE);

        /* sPrefix의 값이 SDC_LOB_DESC_COLUMN_PREFIX <253> 보다 작으면 일반 
           small column이다 */
        if ( (UShort)*aColStartPtr < SDC_LOB_DESC_COLUMN_PREFIX )
        {
            return sColLen;
        }

        if ( *aColStartPtr == SDC_LARGE_COLUMN_PREFIX )
        {
            ID_READ_VALUE(aColStartPtr+1, &sColLen, ID_SIZEOF(UShort));

            sColLen += SDC_LARGE_COLUMN_LEN_STORE_SIZE;
        }
        else
        {
            IDE_DASSERT( (SDC_NULL_COLUMN_PREFIX - *aColStartPtr) < SDC_COLUMN_PREFIX_COUNT );
            sColLen = gColumnHeadSizeTbl[ SDC_NULL_COLUMN_PREFIX - *aColStartPtr ];
        }

        return sColLen;
    }
};


/***********************************************************************
 * Description : Column Piece를 읽어온다.
 *
 *   aColPtr    - [IN]  읽을 Column Piece의 Ptr
 *   aColLen    - [OUT] Column Piece의 크기
 *   aIsLobDesc - [OUT] LOB Descriptor인지 여부를 반환
 **********************************************************************/
inline UChar* sdcRow::getColPiece( const UChar      * aColPtr,
                                   UInt             * aColLen,
                                   UChar           ** aColVal,
                                   sdcColInOutMode  * aInOutMode )
{
    UChar   * sColPiecePtr = (UChar*)aColPtr;
    UShort    sColStoreSize;

    (void)getColPieceInfo( sColPiecePtr,
                           NULL, /* aPrefix */
                           aColLen,
                           aColVal,
                           &sColStoreSize,
                           aInOutMode );
    
    IDE_DASSERT( SDC_GET_COLLEN_STORE_SIZE(*aColLen) == sColStoreSize );
    sColPiecePtr += sColStoreSize;

    return sColPiecePtr;
}

/***********************************************************************
 * Description : get ColPieceLen And Column Prefix
 *
 *   aColPtr - [IN]  길이를 확인할 Column Piece의 Ptr
 *   aPrefix - [OUT] Column Prefix
 **********************************************************************/
inline void sdcRow::getColPieceInfo( const UChar     * aColPtr,
                                     UChar           * aPrefix,
                                     UInt            * aColLen,
                                     UChar          ** aColVal,
                                     UShort          * aColStoreSize,
                                     sdcColInOutMode * aInOutMode )
{
    UChar             sPrefix;
    UChar           * sColVal;
    UShort            sColLen;
    UShort            sColStoreSize;
    sdcColInOutMode   sInOutMode = SDC_COLUMN_IN_MODE;

    IDE_DASSERT( aColPtr != NULL );

    ID_READ_1B_VALUE( aColPtr, &sPrefix );
    
    if ( (sPrefix != SDC_LARGE_COLUMN_PREFIX) && 
         (sPrefix != SDC_LOB_DESC_COLUMN_PREFIX) &&
         (sPrefix != SDC_NULL_COLUMN_PREFIX) )
    {
        sColLen = (UShort)sPrefix;
        sColVal = (UChar*)(aColPtr + SDC_SMALL_COLUMN_LEN_STORE_SIZE);

        IDE_DASSERT( SDC_GET_COLLEN_STORE_SIZE(sColLen) == 
                     SDC_SMALL_COLUMN_LEN_STORE_SIZE );
        sColStoreSize = SDC_SMALL_COLUMN_LEN_STORE_SIZE;
    }
    else
    {
        if ( sPrefix == SDC_LARGE_COLUMN_PREFIX )
        {
            ID_READ_VALUE( aColPtr+1, &sColLen, ID_SIZEOF(sColLen) );
            sColVal = (UChar*)(aColPtr + SDC_LARGE_COLUMN_LEN_STORE_SIZE);

            IDE_DASSERT( SDC_GET_COLLEN_STORE_SIZE(sColLen) == 
                         SDC_LARGE_COLUMN_LEN_STORE_SIZE );
            sColStoreSize = SDC_LARGE_COLUMN_LEN_STORE_SIZE;
        }
        else
        {
            if ( sPrefix == SDC_LOB_DESC_COLUMN_PREFIX )
            {
                sColLen = ID_SIZEOF(sdcLobDesc);
                sColVal = (UChar*)(aColPtr + SDC_SMALL_COLUMN_LEN_STORE_SIZE);
                sInOutMode = SDC_COLUMN_OUT_MODE_LOB;

                IDE_DASSERT( SDC_GET_COLLEN_STORE_SIZE(sColLen) == 
                             SDC_SMALL_COLUMN_LEN_STORE_SIZE );
                sColStoreSize = SDC_SMALL_COLUMN_LEN_STORE_SIZE;
            }
            else
            {
                IDE_ASSERT( sPrefix == SDC_NULL_COLUMN_PREFIX );
                sColLen = 0;
                sColVal = NULL;

                IDE_DASSERT( SDC_GET_COLLEN_STORE_SIZE(sColLen) == 
                             SDC_SMALL_COLUMN_LEN_STORE_SIZE );
                sColStoreSize = SDC_SMALL_COLUMN_LEN_STORE_SIZE;
            }
        }
    }


    if ( aPrefix != NULL )
    {
        *aPrefix = sPrefix;
    }

    if ( aColLen != NULL )
    {
        *aColLen = (UInt)sColLen;
    }

    if ( aColVal != NULL )
    {
        *aColVal = sColVal;
    }

    if ( aColStoreSize != NULL )
    {
        *aColStoreSize = sColStoreSize;
    }

    if ( aInOutMode != NULL )
    {
        *aInOutMode = sInOutMode;
    }

    return;
}

#endif /* _O_SDC_ROW_H_ */
