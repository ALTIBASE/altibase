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
 * $Id: sdnbDef.h 88414 2020-08-25 04:45:02Z justin.kwon $
 **********************************************************************/

#ifndef _O_SDNB_DEF_H_
# define _O_SDNB_DEF_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smnDef.h>
#include <sdnDef.h>

/* BUG-32313: The values of DRDB index cardinality converge on 0 */
#define SDNB_FIND_PREV_SAME_VALUE_KEY   (0)
#define SDNB_FIND_NEXT_SAME_VALUE_KEY   (1)

#define SDNB_SPLIT_POINT_NEW_KEY        ID_USHORT_MAX

/* Proj-1872 Disk Index 저장구조 최적화
 * 키 구조 변경을 위해 사용하는 임시 버퍼들의 크기*/
#define SDNB_MAX_LENGTH_KNOWN_COLUMN_SIZE     (SMI_MAX_MINMAX_VALUE_SIZE)
#define SDNB_MAX_KEY_BUFFER_SIZE              (SD_PAGE_SIZE/2)

/* Proj-1872 Disk Index 저장구조 최적화
 * Column을 표현하기 위한 매크로들 
 *
 * Length-Known 칼럼의 경우, Column Header없이 Value만 저장하며
 * Length-Unknown 칼럼의 경우, 250Byte이하의 칼럼에 대해서는 1Byte,
 * 초과에 대해서는 3Byte로 ColumnHeader를 기록하며, 이 ColumnHeader
 * 에 ColumnValue의 길이가 기록된다*/

#define SDNB_SMALL_COLUMN_VALUE_LENGTH_THRESHOLD (0xFA)

#define SDNB_NULL_COLUMN_PREFIX                  (0xFF)
#define SDNB_LARGE_COLUMN_PREFIX                 (0xFE)

#define SDNB_LARGE_COLUMN_HEADER_LENGTH          (3)
#define SDNB_SMALL_COLUMN_HEADER_LENGTH          (1)

#define SDNB_GET_COLUMN_HEADER_LENGTH( aLen )                           \
    ( ((aLen) > SDNB_SMALL_COLUMN_VALUE_LENGTH_THRESHOLD) ?             \
    SDNB_LARGE_COLUMN_HEADER_LENGTH : SDNB_SMALL_COLUMN_HEADER_LENGTH )

/* sdnbIterator stack                */
#define SDNB_STACK_DEPTH               (128)
#define SDNB_CHANGE_CALLBACK_POS        (6)
#define SDNB_ADJUST_SIZE                (8)
#define SDNB_FREE_NODE_LIMIT            (1)

/* compare flag */
#define SDNB_COMPARE_VALUE     (0x00000001)
#define SDNB_COMPARE_PID       (0x00000002)
#define SDNB_COMPARE_OFFSET    (0x00000004)

/* Key propagation mode : PROJ-1628 */
#define SDNB_SMO_MODE_SPLIT_1_2   (0x00000001)
#define SDNB_SMO_MODE_KEY_REDIST  (0x00000002)

/* Key array 구성에서 Page 구분 : PROJ-1628 */
#define SDNB_CURRENT_PAGE                (0)
#define SDNB_NEXT_PAGE                   (1)
#define SDNB_PARENT_PAGE                 (2)
#define SDNB_NO_PAGE                     (3)

#define  SDNB_IS_LEAF_NODE( node )  (                       \
    ((sdnbNodeHdr*)(node))->mLeftmostChild == SD_NULL_PID ? \
    ID_TRUE : ID_FALSE )                                         

/* Proj-1872 Disk index 저장 구조 최적화
 * 로깅을 위해 칼럼 Length측정을 도와주는 ColLen Info의 크기 */
typedef struct sdnbColLenInfoList
{
    UChar          mColumnCount;
    UChar          mColLenInfo[ SMI_MAX_IDX_COLUMNS ];
} sdnbColLenInfoList;

/* Proj-1872 Disk Index 저장구조 최적화
 * ColLenInfoList를 Dump하기 위한 String의 크기
 * ex) CREATE TABLE T1 (I INTEGER,C CHAR(40), D DATE)
 * 4,?,8 */
#define SDNB_MAX_COLLENINFOLIST_STR_SIZE               ( SMI_MAX_IDX_COLUMNS * 2 )

#define SDNB_COLLENINFO_LENGTH_UNKNOWN                 (0)
#define SDNB_COLLENINFO_LIST_SIZE(aCollenInfoList)     ((UInt)( (aCollenInfoList).mColumnCount+1 ))



/* Proj-1872 Disk index 저장 구조 최적화
 * 함수들간에 Key나 Row의 정보를 전달하기위한 임시 구조체
 *
 * KeyInfo, ConvertedKeyInfo 셋으로 분리
 * Proj-1872 Disk index 저장구조 최적화를 통해, 디스크 인덱스에서 사용하는
 * 키의 표현 구조는 위 세가지이다.
 * 
 *   ConvertedKeyInfo는 KeyInfo의 데이터를 smiValueList형태로 변환한 형태이다.
 * 또한 KeyInfo의 정보들을 모두 포함한다. 일대다 Compare가 필요할 경우, 변환
 * 비용을 줄이기위해 사용된다.
 *   KeyInfo에 저장되는 Key는 Length-Unknonwn타입이냐, Length-Known타입이냐에 
 * 따라 키 크기를 최소화시킨 디스크 인덱스 키 구조이다. 
 */
typedef struct sdnbKeyInfo
{
    UChar         *mKeyValue;   // KeyValue Pointer
    scPageID       mRowPID;     // Row PID
    scSlotNum      mRowSlotNum;
    UShort         mKeyState;   // Key에 설정되어야할 상태
} sdnbKeyInfo;

typedef struct sdnbConvertedKeyInfo
{
    sdnbKeyInfo    mKeyInfo;
    smiValue      *mSmiValueList;
} sdnbConvertedKeyInfo;

#define SDNB_LKEY_TO_CONVERTED_KEYINFO( aLKey, aConvertedKeyInfo, aColLenInfoList        )        \
        ID_4_BYTE_ASSIGN( &((aConvertedKeyInfo).mKeyInfo.mRowPID),     &((aLKey)->mRowPID)     ); \
        ID_2_BYTE_ASSIGN( &((aConvertedKeyInfo).mKeyInfo.mRowSlotNum), &((aLKey)->mRowSlotNum) ); \
        (aConvertedKeyInfo).mKeyInfo.mKeyState = SDNB_GET_STATE( aLKey );                         \
        (aConvertedKeyInfo).mKeyInfo.mKeyValue = SDNB_LKEY_KEY_PTR( aLKey );                      \
        sdnbBTree::makeSmiValueListFromKeyValue(                                                  \
            aColLenInfoList,                                                                      \
            (aConvertedKeyInfo).mKeyInfo.mKeyValue, (aConvertedKeyInfo).mSmiValueList );

#define SDNB_KEYINFO_TO_CONVERTED_KEYINFO( aKeyInfo, aConvertedKeyInfo, aColLenInfoList )           \
        (aConvertedKeyInfo).mKeyInfo    = aKeyInfo;                                                 \
        sdnbBTree::makeSmiValueListFromKeyValue(                                                    \
            aColLenInfoList, (aKeyInfo).mKeyValue, (aConvertedKeyInfo).mSmiValueList );

#define SDNB_GET_COLLENINFO( aKeyColumn )   \
        ( ( ( (aKeyColumn)->flag & SMI_COLUMN_LENGTH_TYPE_MASK) == SMI_COLUMN_LENGTH_TYPE_KNOWN ) ? \
          ( ( aKeyColumn )->size ) : /*LENGTH_KNOWN   */                                            \
          0 )                        /*LENGTH_UNKNOWN */
                         

#define SDNB_KEY_TO_SMIVALUEINFO( aKeyColumn, aKeyPtr, aColumnHeaderLength, aSmiValueInfo)     \
        IDE_DASSERT( ID_SIZEOF( *(aKeyPtr) ) == 1 );                                           \
        (aSmiValueInfo)->column = aKeyColumn;                                                  \
        aKeyPtr += sdnbBTree::getColumnLength(                                                 \
                                    SDNB_GET_COLLENINFO( aKeyColumn ),                         \
                                    aKeyPtr,                                                   \
                                    &sColumnHeaderLength,                                      \
                                    &(aSmiValueInfo)->length,                                  \
                                    &(aSmiValueInfo)->value);

// index segment 첫번째 페이지에 있는 persistent information
// runtime header 생성시에 이것을 참조해서 min, max값 세팅
typedef struct sdnbMeta
{
    scPageID        mRootNode;
    scPageID        mEmptyNodeHead;
    scPageID        mEmptyNodeTail;

    scPageID        mMinNode;
    scPageID        mMaxNode;

    // FOR PROJ-1469
    // mIsCreatedWithLogging : index build시 logging mode, FALSE : Nologging
    // mNologgingCompletionLSN : index build시점에서의 LSN
    // mIsConsistent : nologging index build에서 consistent 표시를 위해
    // mIsCreatedWithForce : nologging force mode, TRUE : Force
    idBool          mIsCreatedWithLogging;
    smLSN           mNologgingCompletionLSN;
    idBool          mIsConsistent;
    idBool          mIsCreatedWithForce;

    ULong           mFreeNodeCnt;
    scPageID        mFreeNodeHead;
}sdnbMeta;

typedef struct sdnbIKey
{
    scPageID       mRightChild;
    scPageID       mRowPID;     // right child의 첫번째 키의 mRowPID
    scSlotNum      mRowSlotNum; // right child의 첫번째 키의 mRowSlotNum
} sdnbIKey;

#define SDNB_IKEY_HEADER_LEN ( ID_SIZEOF(scPageID) + ID_SIZEOF(scPageID) + ID_SIZEOF(scSlotNum) ) 

#define SDNB_IKEY_LEN( aKeyLen )                                            \
    (SDNB_IKEY_HEADER_LEN + (aKeyLen))

#define SDNB_IKEY_KEY_PTR( aSlotPtr )                                       \
    ((UChar*)aSlotPtr + SDNB_IKEY_HEADER_LEN)

#define SDNB_KEYINFO_TO_IKEY( aKeyInfo, aRightChild, aKeyLength, aIKey )                      \
        ID_4_BYTE_ASSIGN( &((aIKey)->mRightChild),          &aRightChild );                   \
        ID_4_BYTE_ASSIGN( &((aIKey)->mRowPID),              &((aKeyInfo).mRowPID) );          \
        ID_2_BYTE_ASSIGN( &((aIKey)->mRowSlotNum),          &((aKeyInfo).mRowSlotNum) );      \
        idlOS::memcpy( SDNB_IKEY_KEY_PTR( aIKey ), (aKeyInfo).mKeyValue, aKeyLength   );           

#define SDNB_IKEY_TO_KEYINFO( aIKey, aKeyInfo )                                 \
        ID_4_BYTE_ASSIGN( &((aKeyInfo).mRowPID),     &((aIKey)->mRowPID) );     \
        ID_2_BYTE_ASSIGN( &((aKeyInfo).mRowSlotNum), &((aIKey)->mRowSlotNum) ); \
        (aKeyInfo).mKeyValue = SDNB_IKEY_KEY_PTR( aIKey );

#define SDNB_GET_RIGHT_CHILD_PID( aChildPID, aIKey )                         \
        ID_4_BYTE_ASSIGN( aChildPID, &((aIKey)->mRightChild) ); 



typedef struct sdnbLKey
{
    // 같은 값을 가지는 key들은 mRowPID, mRowSlotNum순이 됨
    scPageID    mRowPID;
    scSlotNum   mRowSlotNum;
    UChar       mTxInfo[2]; /* = { createCTS   (6bit),  // Create CTS No.
                             *     state       (2bit),  // Key 상태
                             *     limitCTS    (6bit),  // Limit CTS No.
                             *     duplicated  (1bit),  // Duplicated Key 인지 여부.
                             *     txBoundType (1bit) } // Key 타입( SDNB_KEY_TB_CTS, SDNB_KEY_TB_KEY )
                             */
} sdnbLKey;

typedef struct sdnbLTxInfo
{
    smSCN mCreateCSCN;     // Commit SCN or Begin SCN
    sdSID mCreateTSS;
    smSCN mLimitCSCN;
    sdSID mLimitTSS;
} sdnbLTxInfo;

#define SDNB_DUPKEY_NO        (0)  // 'N'
#define SDNB_DUPKEY_YES       (1)  // 'Y'

#define SDNB_CACHE_VISIBLE_UNKNOWN    (0)
#define SDNB_CACHE_VISIBLE_YES        (1)

#define SDNB_KEY_UNSTABLE     (0)  // 'U'
#define SDNB_KEY_STABLE       (1)  // 'S'
#define SDNB_KEY_DELETED      (2)  // 'd'
#define SDNB_KEY_DEAD         (3)  // 'D'

#define SDNB_KEY_TB_CTS       (0)  // 'T'
#define SDNB_KEY_TB_KEY       (1)  // 'K'

// PROJ-1872 DiskIndex 저장구조 최적화
// Bottom-up Build시 LKey 형태로 키를 저장한다.
#define SDNB_WRITE_LKEY( aRowPID, aRowSlotNum, aKey, aKeyLength, aLKey )      \
        ID_4_BYTE_ASSIGN( &((aLKey)->mRowPID),              &(aRowPID) );     \
        ID_2_BYTE_ASSIGN( &((aLKey)->mRowSlotNum),          &(aRowSlotNum) ); \
        SDNB_SET_TB_TYPE( (aLKey),                          SDNB_KEY_TB_CTS );\
        idlOS::memcpy( SDNB_LKEY_KEY_PTR( aLKey ), aKey, aKeyLength  ); 

#define SDNB_KEYINFO_TO_LKEY( aKeyInfo, aKeyLength, aLKey,                              \
                              aCCTS, aDup, aLCTS, aState, aTbType )                     \
        ID_4_BYTE_ASSIGN( &((aLKey)->mRowPID),              &((aKeyInfo).mRowPID) );    \
        ID_2_BYTE_ASSIGN( &((aLKey)->mRowSlotNum),          &((aKeyInfo).mRowSlotNum) );\
        SDNB_SET_CCTS_NO( aLKey, aCCTS );                                               \
        SDNB_SET_DUPLICATED( aLKey, aDup );                                             \
        SDNB_SET_LCTS_NO( aLKey, aLCTS );                                               \
        SDNB_SET_STATE( aLKey, aState );                                                \
        SDNB_SET_TB_TYPE( aLKey, aTbType );                                             \
        idlOS::memcpy( SDNB_LKEY_KEY_PTR( aLKey ), (aKeyInfo).mKeyValue, aKeyLength ); 

#define SDNB_LKEY_TO_KEYINFO( aLKey, aKeyInfo )                                 \
        ID_4_BYTE_ASSIGN( &((aKeyInfo).mRowPID),     &((aLKey)->mRowPID)     ); \
        ID_2_BYTE_ASSIGN( &((aKeyInfo).mRowSlotNum), &((aLKey)->mRowSlotNum) ); \
        (aKeyInfo).mKeyState = SDNB_GET_STATE( aLKey );                         \
        (aKeyInfo).mKeyValue = SDNB_LKEY_KEY_PTR( aLKey );

#define SDNB_EQUAL_PID_AND_SLOTNUM( aLKey, aKeyInfo )                                                        \
        (idlOS::memcmp( &((aLKey)->mRowSlotNum),  &((aKeyInfo)->mRowSlotNum), ID_SIZEOF(scSlotNum) )==0 &&   \
         idlOS::memcmp( &((aLKey)->mRowPID),      &((aKeyInfo)->mRowPID),     ID_SIZEOF(scPageID)  )==0) 

/*
 * BUG-25226: [5.3.1 SN/SD] 30개 이상의 트랜잭션에서 각각 미반영
 *            레코드가 1개 이상인 경우, allocCTS를 실패합니다.
 */
typedef struct sdnbLKeyEx
{
    sdnbLKey    mSlot;
    sdnbLTxInfo mTxInfoEx;
} sdnbLKeyEx;

// PROJ-1704 MVCC Renewal
#define SDNB_GET_CCTS_NO( aKey )       \
    ((0xFC & (aKey)->mTxInfo[0]) >> 2)

#define SDNB_GET_STATE( aKey )         \
    ((0x03 & (aKey)->mTxInfo[0]))

#define SDNB_GET_LCTS_NO( aKey )       \
    ((0xFC & (aKey)->mTxInfo[1]) >> 2)

#define SDNB_GET_DUPLICATED( aKey )    \
    ((0x02 & (aKey)->mTxInfo[1]) >> 1)

#define SDNB_GET_TB_TYPE( aKey )       \
    ((0x01 & (aKey)->mTxInfo[1]))

#define SDNB_GET_TBK_CSCN( aKey, aValue )       \
    idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mCreateCSCN), ID_SIZEOF( smSCN ) )
#define SDNB_GET_TBK_CTSS( aKey, aValue )       \
    idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mCreateTSS), ID_SIZEOF( sdSID ) )
#define SDNB_GET_TBK_LSCN( aKey, aValue )       \
    idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mLimitCSCN), ID_SIZEOF( smSCN ) )
#define SDNB_GET_TBK_LTSS( aKey, aValue )       \
    idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mLimitTSS), ID_SIZEOF( sdSID ) )

#define SDNB_LKEY_LEN( aKeyLen, aTxBoundType ) \
    ( ( (aTxBoundType) == SDNB_KEY_TB_CTS ) ?   \
      ( (aKeyLen) + ID_SIZEOF(sdnbLKey) ) :    \
      ( (aKeyLen) + ID_SIZEOF(sdnbLKeyEx) ) )

#define SDNB_LKEY_KEY_PTR( aSlotPtr )                                          \
    ( ( SDNB_GET_TB_TYPE( ((sdnbLKey*)aSlotPtr) ) == SDNB_KEY_TB_CTS ) ?       \
      ( ((UChar*)aSlotPtr) + ID_SIZEOF(sdnbLKey) ) :                           \
      ( ((UChar*)aSlotPtr) + ID_SIZEOF(sdnbLKeyEx) ) )

#define SDNB_LKEY_HEADER_LEN( aSlotPtr )                                       \
    ( ( SDNB_GET_TB_TYPE( ((sdnbLKey*)aSlotPtr) ) == SDNB_KEY_TB_CTS ) ?       \
      ( ID_SIZEOF(sdnbLKey) ) :                                                \
      ( ID_SIZEOF(sdnbLKeyEx) ) )

#define SDNB_LKEY_HEADER_LEN_EX ( ID_SIZEOF(sdnbLKeyEx) )

#define SDNB_CREATE_CTS_UNMASK   ((UChar)(0x03)) // [1111 1100] [0000 0000]
#define SDNB_STATE_UNMASK        ((UChar)(0xFC)) // [0000 0011] [0000 0000]
#define SDNB_LIMIT_CTS_UNMASK    ((UChar)(0x03)) // [0000 0000] [1111 1100]
#define SDNB_DUPLICATED_UNMASK   ((UChar)(0xFD)) // [0000 0000] [0000 0010]
#define SDNB_TB_UNMASK           ((UChar)(0xFE)) // [0000 0000] [0000 0001]

#define SDNB_SET_CCTS_NO( aKey, aValue )          \
{                                                 \
    (aKey)->mTxInfo[0] &= SDNB_CREATE_CTS_UNMASK; \
    (aKey)->mTxInfo[0] |= ((UChar)aValue << 2);   \
}
#define SDNB_SET_STATE( aKey, aValue )          \
{                                               \
    (aKey)->mTxInfo[0] &= SDNB_STATE_UNMASK;    \
    (aKey)->mTxInfo[0] |= (UChar)aValue; \
}
#define SDNB_SET_LCTS_NO( aKey, aValue )          \
{                                                 \
    (aKey)->mTxInfo[1] &= SDNB_LIMIT_CTS_UNMASK;  \
    (aKey)->mTxInfo[1] |= ((UChar)aValue << 2);   \
}
#define SDNB_SET_DUPLICATED( aKey, aValue )       \
{                                                 \
    (aKey)->mTxInfo[1] &= SDNB_DUPLICATED_UNMASK; \
    (aKey)->mTxInfo[1] |= ((UChar)aValue << 1);   \
}
#define SDNB_SET_TB_TYPE( aKey, aValue )    \
{                                           \
    (aKey)->mTxInfo[1] &= SDNB_TB_UNMASK;   \
    (aKey)->mTxInfo[1] |= (UChar)aValue;    \
}

#define SDNB_SET_TBK_CSCN( aKey, aValue )       \
    idlOS::memcpy( &((aKey)->mTxInfoEx.mCreateCSCN), (aValue), ID_SIZEOF( smSCN ) )
#define SDNB_SET_TBK_CTSS( aKey, aValue )       \
    idlOS::memcpy( &((aKey)->mTxInfoEx.mCreateTSS), (aValue), ID_SIZEOF( sdSID ) )
#define SDNB_SET_TBK_LSCN( aKey, aValue )       \
    idlOS::memcpy( &((aKey)->mTxInfoEx.mLimitCSCN), (aValue), ID_SIZEOF( smSCN ) )
#define SDNB_SET_TBK_LTSS( aKey, aValue )       \
    idlOS::memcpy( &((aKey)->mTxInfoEx.mLimitTSS), (aValue), ID_SIZEOF( sdSID ) )

typedef struct sdnbRollbackContext
{
    smOID mTableOID;
    UInt  mIndexID;
    UChar mTxInfo[2];
} sdnbRollbackContext;

typedef struct sdnbRollbackContextEx
{
    sdnbRollbackContext mRollbackContext;
    sdnbLTxInfo         mTxInfoEx;
} sdnbRollbackContextEx;

// PROJ-1595
// for merge sorted block
typedef struct sdnbMergeBlkInfo
{
    sdnbKeyInfo   *mMinKeyInfo;
    sdnbKeyInfo   *mMaxKeyInfo;
} sdnbMergeBlkInfo;

typedef struct sdnbFreePageLst
{
    scPageID                  mPageID;
    struct sdnbFreePageLst *  mNext;
} sdnbFreePageLst;

typedef struct sdnbSortedBlk
{
    scPageID           mStartPID;
} sdnbSortedBlk;

typedef struct sdnbLKeyHeapNode
{
    sdRID              mExtRID;
    scPageID           mHeadPID;
    scPageID           mCurrPID;
    scPageID           mNextPID;
    UInt               mSlotSeq;
    UInt               mSlotCount;
} sdnbLKeyHeapNode;

typedef struct sdnbMergeRunInfo
{
    scPageID           mHeadPID;
    UInt               mSlotCnt;
    UInt               mSlotSeq;
    sdpPhyPageHdr    * mPageHdr;
    sdnbKeyInfo        mRunKey;
} sdnbMergeRunInfo;

#define SDNB_IN_INIT            (0)   // 'I' /* 페이지 init 상태 */
#define SDNB_IN_USED            (1)   // 'U' /* 키가 삽입되어 있는 상태 */
#define SDNB_IN_EMPTY_LIST      (2)   // 'E' /* 삭제된 키만 존재하는 상태 */
#define SDNB_IN_FREE_LIST       (3)   // 'F' /* DEAD 키만 존재하는 상태 */

#define SDNB_TBK_MAX_CACHE      (2)
#define SDNB_TBK_CACHE_NULL     (0xFFFF)

typedef struct sdnbNodeHdr
{
    scPageID    mLeftmostChild;     /* 첫번째 자식Node의 PID. Leaf Node는 항상 0. (이값으로 Internal/Leaf 구분함) */
    scPageID    mNextEmptyNode;     /* Node가 empty node list에 달렸을때 사용 */
    UShort      mHeight;            /* Leaf Node이면 0, 윗 node로 갈수록 1,2,3,... 증가 */
    UShort      mUnlimitedKeyCount; /* key 상태가 SDNB_KEY_UNSTABLE 또는 SDNB_KEY_STABLE 인 key의 갯수 */
    UShort      mTotalDeadKeySize;  /* key 상태가 SDNB_KEY_DEAD 인 key의 사이즈 합계 */
    UShort      mTBKCount;          /* Node에 포함된 TBK 갯수 */
    UShort      mDummy[SDNB_TBK_MAX_CACHE]; /* BUG-44973 사용하지 않음 */
    UChar       mState;             /* Node의 상태 ( SDNB_IN_USED, SDNB_IN_EMPTY_LIST, SDNB_IN_FREE_LIST ) */
    UChar       mPadding[3];
} sdnbNodeHdr;

typedef struct sdnbColumn
{
    smiCompareFunc                mCompareKeyAndKey;
    smiCompareFunc                mCompareKeyAndVRow;
    smiKey2StringFunc             mKey2String;      // PROJ-1618

    smiCopyDiskColumnValueFunc    mCopyDiskColumnFunc;
    smiIsNullFunc                 mIsNull;
    /* PROJ-1872 Disk Index 저장구조 최적화
     * MakeKeyFromValueRow시, Row는 Length-Known타입의 Null을 1Byte로 압축
     * 하여 표현하기 때문에 NullValue를 알지 못한다. 따라서 이 Null을 가져
     * 오기 위해 mNull 함수를 설정한다. */
    smiNullFunc                   mNull;

    /*BUG-24449 키마다 Header길이가 다를 수 있음 */
    UInt                          mMtdHeaderLength;

    smiColumn                     mKeyColumn;       // Key에서의 column info
    smiColumn                     mVRowColumn;      // fetch된 Row의 column info
}sdnbColumn;

#define SDNB_MIN_KEY_VALUE_LENGTH    (1)
#define SDNB_MAX_ROW_CACHE  \
   ( (SD_PAGE_SIZE - ID_SIZEOF(sdpPhyPageHdr) - ID_SIZEOF(sdnbNodeHdr) - ID_SIZEOF(sdpPageFooter) - ID_SIZEOF(sdpSlotEntry)) / \
    (ID_SIZEOF(sdnbLKey) + SDNB_MIN_KEY_VALUE_LENGTH)) 

typedef struct sdnbPageStat
{
    ULong   mGetPageCount;
    ULong   mReadPageCount;
} sdnbPageStat;

// PROJ-1617
typedef struct sdnbStatistic
{
    ULong        mKeyCompareCount;
    ULong        mKeyValidationCount;
    ULong        mNodeRetryCount;
    ULong        mRetraverseCount;
    ULong        mOpRetryCount;
    ULong        mPessimisticCount;
    ULong        mNodeSplitCount;
    ULong        mNodeDeleteCount;
    ULong        mBackwardScanCount;
    sdnbPageStat mIndexPage;
    sdnbPageStat mMetaPage;
} sdnbStatistic;

#define SDNB_ADD_PAGE_STATISTIC( dest, src ) \
{                                                      \
    (dest)->mGetPageCount  += (src)->mGetPageCount;    \
    (dest)->mReadPageCount += (src)->mReadPageCount;   \
}

// BUG-18201 : Memory/Disk Index 통계치
#define SDNB_ADD_STATISTIC( dest, src ) \
{                                                                           \
    (dest)->mKeyCompareCount    += (src)->mKeyCompareCount;                 \
    (dest)->mKeyValidationCount += (src)->mKeyValidationCount;              \
    (dest)->mNodeRetryCount     += (src)->mNodeRetryCount;                  \
    (dest)->mRetraverseCount    += (src)->mRetraverseCount;                 \
    (dest)->mOpRetryCount       += (src)->mOpRetryCount;                    \
    (dest)->mPessimisticCount   += (src)->mPessimisticCount;                \
    (dest)->mNodeSplitCount     += (src)->mNodeSplitCount;                  \
    (dest)->mNodeDeleteCount    += (src)->mNodeDeleteCount;                 \
    (dest)->mBackwardScanCount  += (src)->mBackwardScanCount;               \
    SDNB_ADD_PAGE_STATISTIC( &((dest)->mIndexPage), &((src)->mIndexPage) ); \
    SDNB_ADD_PAGE_STATISTIC( &((dest)->mMetaPage), &((src)->mMetaPage) );   \
}

/* BUG-31845 [sm-disk-index] Debugging information is needed for 
 * PBT when fail to check visibility using DRDB Index.
 * 이 자료구조가 변경될경우, sdnbBTree::dumpRuntimeHeader
 * 역시 수정되어야 합니다. */
typedef struct sdnbHeader
{
    SDN_RUNTIME_PARAMETERS

    scPageID             mRootNode;
    scPageID             mEmptyNodeHead;
    scPageID             mEmptyNodeTail;
    scPageID             mMinNode;
    scPageID             mMaxNode;

    ULong                mFreeNodeCnt;
    scPageID             mFreeNodeHead;

    /* 위 값들에 대해 Mtx Rollback이 일어났을때 복구하기 위한 백업본 */
    scPageID             mRootNode4MtxRollback;
    scPageID             mEmptyNodeHead4MtxRollback;
    scPageID             mEmptyNodeTail4MtxRollback;
    scPageID             mMinNode4MtxRollback;
    scPageID             mMaxNode4MtxRollback;
    ULong                mFreeNodeCnt4MtxRollback;
    scPageID             mFreeNodeHead4MtxRollback;


    // PROJ-1617 STMT및 AGER로 인한 통계정보 구축
    sdnbStatistic        mDMLStat;
    sdnbStatistic        mQueryStat;

    /* Proj-1827
     * 현재 인덱스 저장 구조는 최대한 저장효율을 높이기 위해 
     * 인덱스 런타임 헤더의 정보를 이용하도록 설계되어 있다.
     * 인덱스 로깅 과정에서는 런타임 헤더 정보를 알수 없기 
     * 때문에 키 길이를 계산할 수 없다.따라서 인덱스 키 길이
     * 를 계산하기 위한 ColLenInfo를 제공해야 한다. */
    sdnbColLenInfoList   mColLenInfoList;
    
    sdnbColumn         * mColumnFence;
    sdnbColumn         * mColumns; //fix BUG-22840

    /* BUG-22946 
     * index key를 insert하려면 row로부터 index key column value를
     * fetch해와야 한다.
     *   row로부터 column 정보를 fetch하려면 fetch column list를 내려
     * 주어야 하는데, insert key insert 할때마다 fetch column list를
     * 구성하면 성능저하가 발생한다.
     *   그래서 create index시에 fetch column list를 구성하고 sdnb-
     * Header에 매달아두도록 수정한다. */
    smiFetchColumnList * mFetchColumnListToMakeKey;

    /* CommonPersistentHeader는 Alter문에 따라 계속 바뀌기 때문에,
     * Pointer로 링크를 걸어둘 수 없다. */

    // TODO: 추후 SdnHeader에 넣기
    UInt                 mSmoNoAtomicA;
    UInt                 mSmoNoAtomicB;
} sdnbHeader;

typedef struct sdnbCallbackContext
{
    sdnbStatistic * mStatistics;
    sdnbHeader    * mIndex;
    sdnbLKey      * mLeafKey;
} sdnbCallbackContext;

/* PROJ-1628 */
typedef struct sdnbKeyArray
{
    UShort           mLength;
    UShort           mPage;
    UShort           mSeq;
} sdnbKeyArray;

/* PROJ-1628에서 mNextNode와 mSlotSize가 추가됨. 설정은 항상 하지만
   Optimistic에서는 사용되지 않으며, Pessimistic에서만 사용됨.
*/
typedef struct sdnbStackSlot
{
    scPageID         mNode;
    scPageID         mNextNode;
    ULong            mSmoNo;
    SShort           mKeyMapSeq;
    UShort           mNextSlotSize;
} sdnbStackSlot;

typedef struct sdnbStack
{
    SInt            mIndexDepth;
    SInt            mCurrentDepth;
    idBool          mIsSameValueInSiblingNodes;
    sdnbStackSlot   mStack[SDNB_STACK_DEPTH];
} sdnbStack;

typedef struct sdnbRowCache
{
    scPageID   mRowPID;
    scSlotNum  mRowSlotNum;
    scOffset   mOffset;  // leaf node내에서의 slot offset
} sdnbRowCache;

/* BUG-31845 [sm-disk-index] Debugging information is needed for 
 * PBT when fail to check visibility using DRDB Index.
 * 이 자료구조가 변경될경우, sdnbBTree::dumpIterator
 * 역시 수정되어야 합니다. */
typedef struct sdnbIterator
{
    smSCN       mSCN;
    smSCN       mInfinite;
    void*       mTrans;
    void*       mTable;
    SChar*      mCurRecPtr;  // MRDB scan module에서만 정의해서 쓰도록 수정?
    SChar*      mLstFetchRecPtr;
    scGRID      mRowGRID;
    smTID       mTID;
    UInt        mFlag;

    smiCursorProperties  * mProperties;
    smiStatement         * mStatement;
    /* smiIterator 공통 변수 끝 */

    void*               mIndex;
    const smiRange *    mKeyRange;
    const smiRange *    mKeyFilter;
    const smiCallBack * mRowFilter;

    /* BUG-32017 [sm-disk-index] Full index scan in DRDB */
    idBool              mFullIndexScan;
    scPageID            mCurLeafNode;
    scPageID            mNextLeafNode;
    scPageID            mScanBackNode;
    ULong               mCurNodeSmoNo;
    ULong               mIndexSmoNo;
    idBool              mIsLastNodeInRange;
    smLSN               mCurNodeLSN;

    sdnbRowCache *      mCurRowPtr;
    sdnbRowCache *      mCacheFence;
    sdnbRowCache        mRowCache[SDNB_MAX_ROW_CACHE];
    sdnbLKey *          mPrevKey;
    // BUG-18557
    ULong               mAlignedPage[SD_PAGE_SIZE/ID_SIZEOF(ULong)];
    UChar *             mPage;
} sdnbIterator;

/*  TASK-4990 changing the method of collecting index statistics
 * 수동 통계정보 수집 기능 */
typedef struct sdnbStatArgument
{
    idvSQL                        * mStatistics;
    sdbMPRFilter4SamplingAndPScan   mFilter4Scan;
    sdnbHeader                    * mRuntimeIndexHeader;

    /* OutParameter */
    IDE_RC  mResult;
    SLong   mClusteringFactor;  /* ClusteringFactor */
    SLong   mNumDist;           /* NumberOfDistinctValue(Cardinality) */
    SLong   mKeyCount;          /* 분석한 키 개수 */
    UInt    mAnalyzedPageCount; /* 실제로 분석한 페이지 개수 */
    UInt    mSampledPageCount;  /* Sample로 선정된 페이지 개수 */

    SLong   mMetaSpace;     /* PageHeader, ExtDir등 Meta 공간 */
    SLong   mUsedSpace;     /* 현재 사용중인 공간 */
    SLong   mAgableSpace;   /* 나중에 Aging가능한 공간 */
    SLong   mFreeSpace;     /* Data삽입이 가능한 빈 공간 */
} sdnbStatArgument;


/* BUG-44973 TBK STAMPING LOGGING용 */
typedef struct sdnbTBKStamping
{
    scOffset mSlotOffset; /* TBK Stamping된 key의 slot offset */
    UChar    mCSCN;       /* CreateCSCN에 의해 TBK Stamping 됨(1) */
    UChar    mLSCN;       /* LimitCSCN에 의해 TBK Stamping 됨(1) */
} sdnbTBKStamping;

#define SDNB_IS_TBK_STAMPING_WITH_CSCN( aTBKStamping )  ( ( (UChar)((aTBKStamping)->mCSCN) == ((UChar)1) ) ? ID_TRUE : ID_FALSE )
#define SDNB_IS_TBK_STAMPING_WITH_LSCN( aTBKStamping )  ( ( (UChar)((aTBKStamping)->mLSCN) == ((UChar)1) ) ? ID_TRUE : ID_FALSE )
#define SDNB_SET_TBK_STAMPING( aTBKStamping, aSlotOffset, aIsCSCN, aIsLSCN )            \
{                                                                                       \
    (aTBKStamping)->mSlotOffset = (scOffset)aSlotOffset;                                \
    (aTBKStamping)->mCSCN       = ( ( aIsCSCN == ID_TRUE ) ? ((UChar)1) : ((UChar)0) ); \
    (aTBKStamping)->mLSCN       = ( ( aIsLSCN == ID_TRUE ) ? ((UChar)1) : ((UChar)0) ); \
}

#endif /* _O_SDNB_DEF_H_ */
