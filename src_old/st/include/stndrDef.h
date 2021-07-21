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
 * $Id: stndrDef.h 89278 2020-11-19 01:41:49Z justin.kwon $
 ***********************************************************************/

#ifndef _O_STNDR_DEF_H_
#define _O_STNDR_DEF_H_ 1

#include <idl.h>
#include <smnDef.h>
#include <sdnDef.h>
#include <stdTypes.h>



#define STNDR_MAX_PATH_STACK_DEPTH  (256)

// Key Buffer는 stdGeometryHeader와 KeySize보다 커야 한다.
//  - makeKeyValueFromRow: stdGeometryHeader를 읽기 위한 크기로 사용
//  - nodeAging: 임시 Leaf Key를 담기 위한 크기로 사용
#define STNDR_MAX_KEY_BUFFER_SIZE   ( ID_SIZEOF(stdGeometryHeader) +    \
                                      ID_SIZEOF(stndrLKeyEx) +          \
                                      ID_SIZEOF(stndrIKey) +            \
                                      ID_SIZEOF(stdMBR) )

#define STNDR_MAX_KEY_COUNT         (SD_PAGE_SIZE / ID_SIZEOF(stdMBR))

#define STNDR_INVALID_KEY_SEQ       ((SShort)-1)

#define STNDR_MAX(a, b)             (((a) > (b)) ? (a) : (b))
#define STNDR_MIN(a, b)             (((a) < (b)) ? (a) : (b))

#define STNDR_INIT_MBR( aMBR ) \
    {                          \
        aMBR.mMinX = 0.0;      \
        aMBR.mMinY = 0.0;      \
        aMBR.mMaxX = 0.0;      \
        aMBR.mMaxY = 0.0;      \
    }

#define  STNDR_IS_LEAF_NODE( node ) \
    ( ((stndrNodeHdr*)(node))->mHeight == 0 ? ID_TRUE : ID_FALSE )


// split mode
typedef enum {
    STNDR_SPLIT_MODE_RTREE = 0,
    STNDR_SPLIT_MODE_RSTAR
} stndrSplitMode;


// traverse mode
typedef enum {
    STNDR_TRAVERSE_DELETE_KEY = 0,
    STNDR_TRAVERSE_FREE_NODE,
    STNDR_TRAVERSE_INSERTKEY_ROLLBACK,
    STNDR_TRAVERSE_DELETEKEY_ROLLBACK
} stndrTraverseMode;


typedef struct stndrKeyArray
{
    SShort  mKeySeq;            // Key Seq
    stdMBR  mMBR;               // Key's MBR
} stndrKeyArray;


// Key Info
typedef struct stndrKeyInfo
{
    UChar       * mKeyValue;    // KeyValue Pointer
    scPageID      mRowPID;
    scSlotNum     mRowSlotNum;
    UShort        mKeyState;
} stndrKeyInfo;


// Meta Page
typedef struct stndrMeta 
{
    scPageID    mRootNode;
    scPageID    mEmptyNodeHead;
    scPageID    mEmptyNodeTail;

    idBool      mIsCreatedWithLogging;
    smLSN       mNologgingCompletionLSN;
    idBool      mIsConsistent;
    idBool      mIsCreatedWithForce;

    ULong       mFreeNodeCnt;
    scPageID    mFreeNodeHead;
    smSCN       mFreeNodeSCN;

    UShort      mConvexhullPointNum;
} stndrMeta;


// Virtual Root Node
typedef struct stndrVirtualRootNode
{
    scPageID    mChildPID;      // Root Node PID
    ULong       mChildSmoNo;    // Root Node Smo No
} stndrVirtualRootNode;


// Internal Key
typedef struct stndrIKey
{
    scPageID    mChildPID;
} stndrIKey;


#define STNDR_IKEY_HEADER_LEN   ( ID_SIZEOF(scPageID) )

#define STNDR_IKEY_LEN( aKeyValueLength ) \
    ( STNDR_IKEY_HEADER_LEN + (aKeyValueLength) )

#define STNDR_IKEY_KEYVALUE_PTR( aKeyPtr )  \
    ( (UChar*)aKeyPtr + STNDR_IKEY_HEADER_LEN )

#define STNDR_KEYINFO_TO_IKEY( aKeyInfo, aChildPID, aKeyValueLength, aIKey )                    \
    {                                                                                           \
        ID_4_BYTE_ASSIGN( &((aIKey)->mChildPID), &aChildPID );                                  \
        idlOS::memcpy( STNDR_IKEY_KEYVALUE_PTR(aIKey), (aKeyInfo).mKeyValue, aKeyValueLength ); \
    }
        
#define STNDR_IKEY_TO_KEYINFO( aIKey, aKeyInfo ) \
    (aKeyInfo).mKeyValue = STNDR_IKEY_KEYVALUE_PTR( aIKey );

#define STNDR_GET_CHILD_PID( aChildPID, aIKey ) \
    ID_4_BYTE_ASSIGN( aChildPID, &((aIKey)->mChildPID) );

#define STNDR_GET_MBR_FROM_IKEY( aMBR, aIKey ) \
    idlOS::memcpy( &aMBR, STNDR_IKEY_KEYVALUE_PTR(aIKey), ID_SIZEOF(aMBR) );    

#define STNDR_GET_MBR_FROM_KEYINFO( aMBR, aKeyInfo ) \
    idlOS::memcpy( &aMBR, ((aKeyInfo)->mKeyValue), ID_SIZEOF(aMBR) );    

#define STNDR_SET_KEYVALUE_TO_IKEY( aKeyValue, aKeyValueLength, aIKey ) \
    idlOS::memcpy( STNDR_IKEY_KEYVALUE_PTR(aIKey), &aKeyValue, aKeyValueLength );


// Leaf Key
typedef struct stndrLKey
{
    scPageID    mRowPID;
    scSlotNum   mRowSlotNum;
    UChar       mTxInfo[2];    /* = { createCTS   (6bit),
                                *     state       (2bit),
                                *     limitCTS    (6bit),
                                *     duplicated  (1bit), - unused
                                *     txBoundType (1bit) }
                                */
    smSCN       mCreateSCN; 
    smSCN       mLimitSCN;
} stndrLKey;

typedef struct stndrLTxInfo
{
    smSCN mCreateCSCN;
    sdSID mCreateTSS;
    smSCN mLimitCSCN;
    sdSID mLimitTSS;
} stndrLTxInfo;

typedef struct stndrLKeyEx
{
    stndrLKey       mSlot;
    stndrLTxInfo    mTxInfoEx;
} stndrLKeyEx;



/* For Bottom-up Build */
typedef enum stndrBottomUpBuildPhase
{
    STNDR_EXTRACT_AND_SORT = 0,
    STNDR_MERGE_RUN
} stndrBottomUpBuildPhase;

typedef struct stndrCenterPoint
{
    SDouble    mPointX;
    SDouble    mPointY;
} stndrCenterPoint;

typedef struct stndrLBuildKey
{
    stndrLKey           mLeafKey;
    stndrCenterPoint    mCenterPoint;
} stndrLBuildKey;

typedef struct stndrIBuildKey
{
    stndrIKey           mInternalKey;
    SChar               mAlign[4];      /* BUG-31024: AIX에서 Padding이 안되는 문제, 첫 멤버가
                                         * Double이 아니면 4byte 정렬됨 */
    stndrCenterPoint    mCenterPoint;
} stndrIBuildKey;

#define STNDR_WRITE_LBUILDKEY( aRowPID, aRowSlotNum, aBuildKey, aBuildKeyLength, aLBuildKey )     \
{                                                                                                 \
    ID_4_BYTE_ASSIGN( &(((stndrLBuildKey*)(aLBuildKey))->mLeafKey.mRowPID),     &(aRowPID) );     \
    ID_2_BYTE_ASSIGN( &(((stndrLBuildKey*)(aLBuildKey))->mLeafKey.mRowSlotNum), &(aRowSlotNum) ); \
    STNDR_SET_TB_TYPE( ((stndrLKey*)(aLBuildKey)), STNDR_KEY_TB_CTS );                            \
    STNDR_SET_STATE  ( ((stndrLKey*)(aLBuildKey)), STNDR_KEY_STABLE );                            \
    idlOS::memcpy( STNDR_LBUILDKEY_KEYVALUE_PTR( aLBuildKey ), aBuildKey, aBuildKeyLength );      \
    ((stndrLBuildKey*)aLBuildKey)->mCenterPoint.mPointX = (((stdMBR*)(aBuildKey))->mMinX + ((stdMBR*)(aBuildKey))->mMaxX) / 2; \
    ((stndrLBuildKey*)aLBuildKey)->mCenterPoint.mPointY = (((stdMBR*)(aBuildKey))->mMinY + ((stdMBR*)(aBuildKey))->mMaxY) / 2; \
}

#define STNDR_WRITE_IBUILDKEY( aChildPID, aBuildKey, aBuildKeyLength, aIBuildKey )                \
{                                                                                                 \
    ID_4_BYTE_ASSIGN( &(((stndrIBuildKey*)(aIBuildKey))->mInternalKey.mChildPID), &(aChildPID) ); \
    idlOS::memcpy( STNDR_IBUILDKEY_KEYVALUE_PTR( aIBuildKey ), aBuildKey, aBuildKeyLength );      \
    ((stndrIBuildKey*)aIBuildKey)->mCenterPoint.mPointX = (((stdMBR*)(aBuildKey))->mMinX + ((stdMBR*)(aBuildKey))->mMaxX) / 2; \
    ((stndrIBuildKey*)aIBuildKey)->mCenterPoint.mPointY = (((stdMBR*)(aBuildKey))->mMinY + ((stdMBR*)(aBuildKey))->mMaxY) / 2; \
}

#define STNDR_LBUILDKEY_KEYVALUE_PTR( aSlotPtr ) \
    ( ((UChar*)aSlotPtr) + ID_SIZEOF(stndrLBuildKey) )

#define STNDR_IBUILDKEY_KEYVALUE_PTR( aSlotPtr ) \
    ( ((UChar*)aSlotPtr) + ID_SIZEOF(stndrIBuildKey) )

#define STNDR_LBUILDKEY_TO_NODEKEYINFO( aLBuildKey, aNodeKeyInfo )                              \
{                                                                                               \
    (aNodeKeyInfo).mRowPID     = ((stndrLBuildKey*)aLBuildKey)->mLeafKey.mRowPID;               \
    (aNodeKeyInfo).mRowSlotNum = ((stndrLBuildKey*)aLBuildKey)->mLeafKey.mRowSlotNum;           \
    (aNodeKeyInfo).mKeyState   = STNDR_GET_STATE( &(((stndrLBuildKey*)aLBuildKey)->mLeafKey) ); \
    (aNodeKeyInfo).mKeyValue   = STNDR_LBUILDKEY_KEYVALUE_PTR( aLBuildKey );                    \
}

#define STNDR_IBUILDKEY_TO_NODEKEYINFO( aIBuildKey, aNodeKeyInfo )         \
{                                                                          \
    (aNodeKeyInfo).mKeyValue = STNDR_IBUILDKEY_KEYVALUE_PTR( aIBuildKey ); \
}

typedef struct stndrSortedBlk
{
    scPageID           mStartPID;
} stndrSortedBlk;

typedef struct stndrRunInfo
{
    scPageID             mHeadPID;
    UInt                 mSlotCnt;
    UInt                 mSlotSeq;
    sdpPhyPageHdr      * mPageHdr;
    void               * mKey;
} stndrRunInfo;
/* For Bottom-up Build */


#define STNDR_KEY_UNSTABLE     0  // 'U'
#define STNDR_KEY_STABLE       1  // 'S'
#define STNDR_KEY_DELETED      2  // 'd'
#define STNDR_KEY_DEAD         3  // 'D'

#define STNDR_KEY_TB_CTS       0  // 'T'
#define STNDR_KEY_TB_KEY       1  // 'K'

#define STNDR_WRITE_LKEY( aRowPID, aRowSlotNum, aKeyValue, aKeyValueLength, aLKey )     \
    {                                                                                   \
        ID_4_BYTE_ASSIGN( &((aLKey)->mRowPID),      &(aRowPID) );                       \
        ID_2_BYTE_ASSIGN( &((aLKey)->mRowSlotNum),  &(aRowSlotNum) );                   \
        STNDR_SET_TB_TYPE( (aLKey),                 STNDR_KEY_TB_CTS );                 \
        idlOS::memcpy( STNDR_LKEY_KEYVALUE_PTR(aLKey), aKeyValue, aKeyValueLength );    \
    }

#define STNDR_KEYINFO_TO_LKEY( aKeyInfo, aKeyValueLength, aLKey,            \
                               aCCTS, aCSCN,                                \
                               aLCTS, aLSCN, aState, aTbType )              \
    {                                                                           \
        ID_4_BYTE_ASSIGN( &((aLKey)->mRowPID),      &((aKeyInfo).mRowPID) );    \
        ID_2_BYTE_ASSIGN( &((aLKey)->mRowSlotNum),  &((aKeyInfo).mRowSlotNum) );\
        STNDR_SET_CCTS_NO( aLKey, aCCTS );                                      \
        STNDR_SET_LCTS_NO( aLKey, aLCTS );                                      \
        STNDR_SET_CSCN( aLKey, &aCSCN );                                        \
        STNDR_SET_LSCN( aLKey, &aLSCN );                                        \
        STNDR_SET_STATE( aLKey, aState );                                       \
        STNDR_SET_TB_TYPE( aLKey, aTbType );                                    \
        idlOS::memcpy( STNDR_LKEY_KEYVALUE_PTR(aLKey), (aKeyInfo).mKeyValue, aKeyValueLength ); \
    }

#define STNDR_LKEY_TO_KEYINFO( aLKey, aKeyInfo ) \
    {                                            \
        ID_4_BYTE_ASSIGN( &((aKeyInfo).mRowPID),     &((aLKey)->mRowPID)     ); \
        ID_2_BYTE_ASSIGN( &((aKeyInfo).mRowSlotNum), &((aLKey)->mRowSlotNum) ); \
        (aKeyInfo).mKeyState = STNDR_GET_STATE( aLKey );                        \
        (aKeyInfo).mKeyValue = STNDR_LKEY_KEYVALUE_PTR( aLKey );                \
    }
    
#define STNDR_EQUAL_PID_AND_SLOTNUM( aLKey, aKeyInfo )  \
    ( (idlOS::memcmp( &((aLKey)->mRowSlotNum),          \
                      &((aKeyInfo)->mRowSlotNum),       \
                      ID_SIZEOF(scSlotNum) ) == 0)      \
      &&                                                \
      (idlOS::memcmp( &((aLKey)->mRowPID),              \
                      &((aKeyInfo)->mRowPID),           \
                      ID_SIZEOF(scPageID)  ) == 0) )

#define STNDR_GET_CCTS_NO( aKey )   ((0xFC & (aKey)->mTxInfo[0]) >> 2)
#define STNDR_GET_STATE( aKey )     ((0x03 & (aKey)->mTxInfo[0]))
#define STNDR_GET_LCTS_NO( aKey )   ((0xFC & (aKey)->mTxInfo[1]) >> 2)

#define STNDR_GET_TB_TYPE( aKey )   ((0x01 & (aKey)->mTxInfo[1]))

#define STNDR_GET_TBK_CSCN( aKey, aValue )       \
    ( idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mCreateCSCN), ID_SIZEOF(smSCN) ) )

#define STNDR_GET_TBK_CTSS( aKey, aValue )       \
    ( idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mCreateTSS), ID_SIZEOF(sdSID) ) )

#define STNDR_GET_TBK_LSCN( aKey, aValue )       \
    ( idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mLimitCSCN), ID_SIZEOF(smSCN) ) )

#define STNDR_GET_TBK_LTSS( aKey, aValue )       \
    ( idlOS::memcpy( (aValue), &((aKey)->mTxInfoEx.mLimitTSS), ID_SIZEOF(sdSID) ) )

#define STNDR_GET_CSCN( aKey, aValue ) \
    ( idlOS::memcpy( (aValue), &((aKey)->mCreateSCN), ID_SIZEOF(smSCN) ) )
    
#define STNDR_GET_LSCN( aKey, aValue ) \
    ( idlOS::memcpy( (aValue), &((aKey)->mLimitSCN), ID_SIZEOF(smSCN) ) )


#define STNDR_GET_MBR_FROM_LKEY( aMBR, aLKey )                          \
    memcpy( &aMBR, STNDR_LKEY_KEYVALUE_PTR(aLKey), ID_SIZEOF(stdMBR) );    

#define STNDR_LKEY_LEN( aKeyValueLength, aTxBoundType ) \
    ( ( (aTxBoundType) == STNDR_KEY_TB_CTS ) ?  \
      ( (aKeyValueLength) + ID_SIZEOF(stndrLKey) ) :    \
      ( (aKeyValueLength) + ID_SIZEOF(stndrLKeyEx) ) )

#define STNDR_LKEY_KEYVALUE_PTR( aKeyPtr )                                  \
    ( ( STNDR_GET_TB_TYPE( ((stndrLKey*)aKeyPtr) ) == STNDR_KEY_TB_CTS ) ?  \
      ( ((UChar*)aKeyPtr) + ID_SIZEOF(stndrLKey) ) :                        \
      ( ((UChar*)aKeyPtr) + ID_SIZEOF(stndrLKeyEx) ) )

#define STNDR_LKEY_HEADER_LEN( aKeyPtr )                                    \
    ( ( STNDR_GET_TB_TYPE( ((stndrLKey*)aKeyPtr) ) == STNDR_KEY_TB_CTS ) ?  \
      ( ID_SIZEOF(stndrLKey) ) :                                            \
      ( ID_SIZEOF(stndrLKeyEx) ) )

#define STNDR_LKEY_HEADER_LEN_EX    ( ID_SIZEOF(stndrLKeyEx) )

#define STNDR_CREATE_CTS_UNMASK     ((UChar)(0x03)) // [1111 1100] [0000 0000]
#define STNDR_STATE_UNMASK          ((UChar)(0xFC)) // [0000 0011] [0000 0000]
#define STNDR_LIMIT_CTS_UNMASK      ((UChar)(0x03)) // [0000 0000] [1111 1100]
#define STNDR_TB_UNMASK             ((UChar)(0xFE)) // [0000 0000] [0000 0001]

#define STNDR_SET_CSCN( aKey, aValue ) \
    idlOS::memcpy( &((aKey)->mCreateSCN), (aValue), ID_SIZEOF(smSCN) );
    
#define STNDR_SET_LSCN( aKey, aValue ) \
    idlOS::memcpy( &((aKey)->mLimitSCN), (aValue), ID_SIZEOF(smSCN) );

#define STNDR_SET_CCTS_NO( aKey, aValue )               \
    {                                                   \
        (aKey)->mTxInfo[0] &= STNDR_CREATE_CTS_UNMASK;  \
        (aKey)->mTxInfo[0] |= ((UChar)aValue << 2);     \
    }
#define STNDR_SET_STATE( aKey, aValue )             \
    {                                               \
        (aKey)->mTxInfo[0] &= STNDR_STATE_UNMASK;   \
        (aKey)->mTxInfo[0] |= ((UChar)aValue); \
    }
#define STNDR_SET_LCTS_NO( aKey, aValue )               \
    {                                                   \
        (aKey)->mTxInfo[1] &= STNDR_LIMIT_CTS_UNMASK;   \
        (aKey)->mTxInfo[1] |= ((UChar)aValue << 2);     \
    }
#define STNDR_SET_TB_TYPE( aKey, aValue )       \
    {                                           \
        (aKey)->mTxInfo[1] &= STNDR_TB_UNMASK;  \
        (aKey)->mTxInfo[1] |= (UChar)aValue;    \
    }

#define STNDR_SET_TBK_CSCN( aKey, aValue )                              \
    ( idlOS::memcpy( &((aKey)->mTxInfoEx.mCreateCSCN), (aValue), ID_SIZEOF(smSCN) ) )

#define STNDR_SET_TBK_CTSS( aKey, aValue )                              \
    ( idlOS::memcpy( &((aKey)->mTxInfoEx.mCreateTSS), (aValue), ID_SIZEOF(sdSID) ) )

#define STNDR_SET_TBK_LSCN( aKey, aValue )                              \
    ( idlOS::memcpy( &((aKey)->mTxInfoEx.mLimitCSCN), (aValue), ID_SIZEOF(smSCN) ) )

#define STNDR_SET_TBK_LTSS( aKey, aValue )                              \
    ( idlOS::memcpy( &((aKey)->mTxInfoEx.mLimitTSS), (aValue), ID_SIZEOF(sdSID) ) )


// rollback info
typedef struct stndrRollbackContext
{
    smOID   mTableOID;
    UInt    mIndexID;
    smSCN   mCreateSCN;
    smSCN   mLimitSCN;
    smSCN   mFstDiskViewSCN;
    UChar   mTxInfo[2];
} stndrRollbackContext;

typedef struct stndrRollbackContextEx
{
    stndrRollbackContext    mRollbackContext;
    stndrLTxInfo            mTxInfoEx;
} stndrRollbackContextEx;


// Node Page
#define STNDR_IN_USED            0   // 'U'
#define STNDR_IN_EMPTY_LIST      1   // 'E'
#define STNDR_IN_FREE_LIST       2   // 'F'

#define STNDR_TBK_MAX_CACHE      2
#define STNDR_TBK_CACHE_NULL     0xFFFF


typedef struct stndrNodeHdr
{
    stdMBR      mMBR;
    scPageID    mNextEmptyNode;
    scPageID    mNextFreeNode;
    smSCN       mFreeNodeSCN;
    smSCN       mNextFreeNodeSCN;
    UShort      mHeight;
    UShort      mUnlimitedKeyCount;
    UShort      mTotalDeadKeySize;
    UShort      mTBKCount;
    UShort      mRefTBK[STNDR_TBK_MAX_CACHE];
    UChar       mState;
    UChar       mPadding[3];
} stndrNodeHdr;


// Runtime Header
typedef idBool  (*stndrIntersectFunc)       ( stdMBR *a1,    stdMBR *a2 );
typedef idBool  (*stndrContainsFunc)        ( stdMBR *a1,    stdMBR *a2 );
typedef void    (*stndrCopyMBRFunc)         ( stdMBR *aMbr1, stdMBR *aMbr2 );
typedef stdMBR* (*stndrGetExtendedMBRFunc)  ( stdMBR *aMbr1, stdMBR *aMbr2 );
typedef SDouble (*stndrGetAreaFunc)         ( stdMBR *aMbr );
typedef SDouble (*stndrGetDeltaFunc)        ( stdMBR *aMbr1, stdMBR *aMbr2 );
typedef SDouble (*stndrGetOverlapFunc)      ( stdMBR *aMbr1, stdMBR *aMbr2 );
typedef idBool  (*stndrIsEqualFunc)         ( stdMBR *aMbr1, stdMBR *aMbr2 );

typedef struct stndrColumn
{
    smiKey2StringFunc             mKey2String;
    
    smiCopyDiskColumnValueFunc    mCopyDiskColumnFunc;
    smiIsNullFunc                 mIsNull;
    
    smiNullFunc                   mNull;

    UInt                          mMtdHeaderLength;

    smiColumn                     mKeyColumn;       // Key에서의 column info
    smiColumn                     mVRowColumn;      // fetch된 Row의 column info
}stndrColumn;

typedef struct stndrPageStat
{
    ULong   mGetPageCount;
    ULong   mReadPageCount;
} stndrPageStat;

typedef struct stndrStatistic
{
    ULong         mKeyCompareCount;
    ULong         mKeyPropagateCount;
    ULong         mKeyRangeCount;
    ULong         mRowFilterCount;
    ULong         mFollowRightLinkCount;
    ULong         mOpRetryCount;
    ULong         mNodeSplitCount;
    ULong         mNodeDeleteCount;
    stndrPageStat mIndexPage;
    stndrPageStat mMetaPage;
} stndrStatistic;

#define STNDR_ADD_PAGE_STATISTIC( dest, src )               \
    {                                                       \
        (dest)->mGetPageCount  += (src)->mGetPageCount;     \
        (dest)->mReadPageCount += (src)->mReadPageCount;    \
    }

#define STNDR_ADD_STATISTIC( dest, src )                                \
    {                                                                   \
        (dest)->mKeyCompareCount        += (src)->mKeyCompareCount;         \
        (dest)->mKeyPropagateCount      += (src)->mKeyPropagateCount;       \
        (dest)->mKeyRangeCount          += (src)->mKeyRangeCount;           \
        (dest)->mRowFilterCount         += (src)->mRowFilterCount;          \
        (dest)->mFollowRightLinkCount   += (src)->mFollowRightLinkCount;    \
        (dest)->mOpRetryCount           += (src)->mOpRetryCount;            \
        (dest)->mNodeSplitCount         += (src)->mNodeSplitCount;          \
        (dest)->mNodeDeleteCount        += (src)->mNodeDeleteCount;         \
        STNDR_ADD_PAGE_STATISTIC( &((dest)->mIndexPage), &((src)->mIndexPage) );\
        STNDR_ADD_PAGE_STATISTIC( &((dest)->mMetaPage), &((src)->mMetaPage) );  \
    }

typedef struct stndrHeader
{
    sdnRuntimeHeader        mSdnHeader;

    // R-Tree Specific Information
    scPageID                mRootNode;
    scPageID                mEmptyNodeHead;
    scPageID                mEmptyNodeTail;

    ULong                   mFreeNodeCnt;
    scPageID                mFreeNodeHead;
    smSCN                   mFreeNodeSCN;

    ULong                   mKeyCount;
    stndrStatistic          mDMLStat;
    stndrStatistic          mQueryStat;
    stdMBR                  mTreeMBR;
    idBool                  mInitTreeMBR;
    UInt                    mMaxKeyCount;
    
    UShort                  mConvexhullPointNum;

    stndrColumn             mColumn;
    smiFetchColumnList      mFetchColumnListToMakeKey;

    iduMutex                mSmoNoMutex;
    UInt                    mSmoNoAtomicA;
    UInt                    mSmoNoAtomicB;

    // BUG-29743: Root Node의 Split 여부를 잘못 판단합니다.
    stndrVirtualRootNode    mVirtualRootNode;
    UInt                    mVirtualRootNodeAtomicA;
    UInt                    mVirtualRootNodeAtomicB;

    /* 위 값들에 대해 Mtx Rollback이 일어났을때 복구하기 위한 백업본 */
    scPageID                mRootNode4MtxRollback;
    scPageID                mEmptyNodeHead4MtxRollback;
    scPageID                mEmptyNodeTail4MtxRollback;
    ULong                   mFreeNodeCnt4MtxRollback;
    scPageID                mFreeNodeHead4MtxRollback;
    smSCN                   mFreeNodeSCN4MtxRollback;
    ULong                   mKeyCount4MtxRollback;
    stdMBR                  mTreeMBR4MtxRollback;
    idBool                  mInitTreeMBR4MtxRollback;
    stndrVirtualRootNode    mVirtualRootNode4MtxRollback;
} stndrHeader;

typedef struct stndrCallbackContext
{
    stndrStatistic  * mStatistics;
    stndrHeader     * mIndex;
    stndrLKey       * mLeafKey;
} stndrCallbackContext;


// stack for sort
#define STNDR_MAX_SORT_STACK_SIZE   (STNDR_MAX_KEY_COUNT * 2)

typedef struct stdnrSortStackSlot
{
    stndrKeyArray   * mArray;
    SInt              mArraySize;
} stndrSortStackSlot;


// traveree stack
typedef struct stndrStackSlot 
{
    scPageID    mNodePID;
    SShort      mKeySeq;
    SShort      mHeight;
    ULong       mSmoNo;
} stndrStackSlot;

typedef struct stndrStack
{
    SInt              mDepth;
    SInt              mStackSize;
    stndrStackSlot  * mStack;
} stndrStack;

typedef struct stndrPathStack
{
    SInt            mDepth;
    stndrStackSlot  mStack[STNDR_MAX_PATH_STACK_DEPTH];
} stndrPathStack;


// Iterator
#define STNDR_MIN_KEY_VALUE_LENGTH  (ID_SIZEOF(stdMBR))
#define STNDR_MAX_ROW_CACHE \
    ( (SD_PAGE_SIZE - \
       ID_SIZEOF(sdpPhyPageHdr) - \
       ID_SIZEOF(stndrNodeHdr)  - \
       ID_SIZEOF(sdpPageFooter) - \
       ID_SIZEOF(sdpSlotEntry)) \
       / (ID_SIZEOF(stndrLKey) + STNDR_MIN_KEY_VALUE_LENGTH) )

typedef struct stndrRowCache
{
    scPageID   mRowPID;
    scSlotNum  mRowSlotNum;
    scOffset   mOffset;
} stndrRowCache;

typedef struct stndrIterator
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

    void                * mIndex;
    const smiRange      * mKeyRange;
    const smiCallBack   * mRowFilter;

    idBool                mIsLastNodeInRange;

    stndrRowCache       * mCurRowPtr;
    stndrRowCache       * mCacheFence;
    stndrRowCache         mRowCache[STNDR_MAX_ROW_CACHE];

    stndrStack            mStack; // stack for traverse

    ULong                 mAlignedPage[SD_PAGE_SIZE/ID_SIZEOF(ULong)];
    UChar               * mPage;
} stndrIterator;



#endif /* _O_STNDR_DEF_H_ */
