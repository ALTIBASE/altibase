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
 * $Id: sdnbFT.h 19550 2007-02-07 03:09:40Z leekmo $
 *
 * Disk BTree Index의 DUMP를 위한 함수
 *
 **********************************************************************/

#ifndef _O_SDNB_FT_H_
#define _O_SDNB_FT_H_  (1)

# include <idu.h>
# include <smDef.h>
# include <sdnbDef.h>

//-------------------------------
// D$DISK_INDEX_BTREE_STRUCTURE 의 구조
//-------------------------------

typedef struct sdnbDumpTreePage       // for sdnbNodeHdr
{
    UShort         mDepth;            // DEPTH
    UShort         mHeight;           // HEIGHT
    SShort         mNthSibling;       // NTH_SIBLING
    SChar          mIsLeaf;           // IS_LEAF
    UInt           mMyPID;            // MY_PAGEID
    UInt           mNextPID;          // NEXT_PAGEID
    UInt           mPrevPID;          // PREV_PAGEID
    ULong          mPageSeq;          // PAGE_SEQ
    SShort         mSlotCount;        // SLOT_CNT
    ULong          mSmoNo;            // SMO_NO
    UShort         mTotalFreeSize;    // TOTAL_FREE_SIZE
    UInt           mParentPID;        // PARENT_PAGEID
    UInt           mLeftmostChildPID; // LMOST_CHILD_PAGEID
    UShort         mUnlimitedKeyCount;// UNLIMITED_KEY_COUNT
    UShort         mTotalDeadKeySize; // TOTAL_DEAD_KEY_SIZE
    UShort         mTBKCount;         // TBK_COUNT
    UChar          mPageState;        // PAGE_STATE
    UShort         mCTLayerSize;      // CTL_SIZE
    UShort         mCTSlotUsedCount;  // CTS_USED_COUNT
    SChar          mIsConsistent;     // IS_CONSISTENT
} sdnbDumpTreePage;

//-------------------------------
// D$DISK_INDEX_BTREE_KEY 의 구조
//-------------------------------

typedef struct sdnbDumpKey            // for sdnbIKey, sdnbLKey
{
    UInt           mMyPID;            // MY_PAGEID
    ULong          mPageSeq;          // PAGE_SEQ
    UShort         mDepth;            // DEPTH
    SChar          mIsLeaf;           // IS_LEAF
    UInt           mParentPID;        // PARENT_PAGEID
    SShort         mNthSibling;       // NTH_SIBLING
    SShort         mNthSlot;          // NTH_SLOT
    SShort         mNthColumn;        // NTH_COLUMN  : BUG-16805
    UShort         mColumnLength;     // COLUMN_LENGTH    : PROJ-1872
    SChar          mValue[SMN_DUMP_VALUE_LENGTH];  // VALUE24B
    UInt           mChildPID;         // CHILD_PAGEID
    UInt           mRowPID;           // ROW_PAGEID
    UInt           mRowSlotNum;       // ROW_SLOTNUM
    UChar          mState;            // STATE
    UChar          mDuplicated;       // DUPLICATED
    UShort         mCreateCTS;        // CREATE_CTS_NO
    UChar          mCreateChained;    // CREATE_CHAINED - unused
    UShort         mLimitCTS;         // LIMIT_CTS_NO
    UChar          mLimitChained;     // LIMIT_CHAINED - unused
    UChar          mTxBoundType;      // TB_TYPE
    ULong          mCreateTSS;        // CREATE_TSS_RID
    ULong          mLimitTSS;         // LIMIT_TSS_RID
    SChar         *mCreateCSCN;       // CREATE_COMMIT_SCN
    SChar         *mLimitCSCN;        // LIMIT_COMMIT_SCN
} sdnbDumpKey;

//-------------------------------
// X$DISK_BTREE_HEADER 의 구조
//-------------------------------

typedef struct sdnbHeader4PerfV
{
    UChar             mName[SM_MAX_NAME_LEN+8]; // INDEX_NAME
    UInt              mIndexID;          // INDEX_ID
    
    UInt              mIndexTSID;        // INDEX_TBS_ID
    UInt              mTableTSID;        // TABLE_TBS_ID
    UInt              mSegHdrPID;        // SEG_HDR_PAGEID
    
    UInt              mRootNode;         // ROOT_PAGEID
    UInt              mEmptyNodeHead;    // EMPTY_HEAD_PAGEID
    UInt              mEmptyNodeTail;    // EMPTY_TAIL_PAGEID
    ULong             mSmoNo;            // SMO_NO

    UInt              mFreeNodeHead;     // FREE_NODE_HEAD
    ULong             mFreeNodeCnt;      // FREE_NODE_CNT
    
    SChar             mIsUnique;         // IS_UNIQUE
    UChar             mColLenInfoStr[SDNB_MAX_COLLENINFOLIST_STR_SIZE+8];
    
    UInt              mMinNode;          // STAT_MIN_PAGEID
    UInt              mMaxNode;          // STAT_MAX_PAGEID
    SChar             mIsConsistent;     // IS_CONSISTENT

    // BUG-17957
    // X$DISK_BTREE_HEADER에 creation option(logging, force) 추가
    SChar             mIsCreatedWithLogging;  // IS_CREATED_WITH_LOGGING
    SChar             mIsCreatedWithForce;    // IS_CREATED_WITH_FORCE

    smLSN             mCompletionLSN;    // COMPLETION_LSN_FILE_NO
                                         // COMPLETION_LSN_FILE_OFFSET
    smiSegAttr        mSegAttr;          // Segment Attribute
    smiSegStorageAttr mSegStoAttr;       // Segment Storage Attribute
} sdnbHeader4PerfV;

//-------------------------------
// X$DISK_BTREE_STAT 의 구조
//-------------------------------

typedef struct sdnbStat4PerfV
{
    UChar         mName[SM_MAX_NAME_LEN+8];
    UInt          mIndexID;
    ULong         mTreeLatchReadCount;
    ULong         mTreeLatchWriteCount;
    ULong         mTreeLatchReadMissCount;
    ULong         mTreeLatchWriteMissCount;
    ULong         mKeyCount;
    sdnbStatistic mDMLStat;
    sdnbStatistic mQueryStat;

    ULong         mNumDist;
    // BUG-18188
    UChar         mMinValue[MAX_MINMAX_VALUE_SIZE];   // MIN_VALUE
    UChar         mMaxValue[MAX_MINMAX_VALUE_SIZE];   // MAX_VALUE
} sdnbStat4PerfV;


class sdnbFT
{
public:

    //------------------------------------------
    // For D$DISK_INDEX_BTREE_STRUCTURE
    //------------------------------------------
    static IDE_RC buildRecordTreeStructure( idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildTreePage( idvSQL*               aStatistics,
                                         void                * aHeader,
                                         iduFixedTableMemory * aMemory,
                                         UShort                aDepth,
                                         SShort                aNthSibling,
                                         scSpaceID             aSpaceID,
                                         scPageID              aMyPID,
                                         ULong               * aPageSeq,
                                         scPageID              aParentPID );

    //------------------------------------------
    // For D$DISK_INDEX_BTREE_KEY
    //------------------------------------------
    static IDE_RC buildRecordKey( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * aDumpObj,
                                  iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildKey( idvSQL*               aStatistics,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    sdnbHeader          * aIdxHdr,
                                    sdrMtx              * aMiniTx,
                                    UShort                aDepth,
                                    SShort                aNthSibling,
                                    scSpaceID             aSpaceID,
                                    scPageID              aMyPID,
                                    ULong               * aPageSeq,
                                    scPageID              aParentPID );
    
    //------------------------------------------
    // For X$DISK_BTREE_STAT
    //------------------------------------------
    static IDE_RC buildRecordForDiskBTreeHeader(idvSQL              * /*aStatistics*/,
                                                void                * aHeader,
                                                void                * aDumpObj,
                                                iduFixedTableMemory * aMemory);

    //------------------------------------------
    // For X$DISK_BTREE_HEADER
    //------------------------------------------
    static IDE_RC buildRecordForDiskBTreeStat(idvSQL              * /*aStatistics*/,
                                              void                *aHeader,
                                              void                */*aDumpObj*/,
                                              iduFixedTableMemory *aMemory);

    
    //------------------------------------------
    // For X$DISK_TEMP_BTREE_STAT
    //------------------------------------------
    static IDE_RC buildRecordForDiskTempBTreeStat(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * /* aDumpObj */,
        iduFixedTableMemory * aMemory);


};


#endif /* _O_SDNB_FT_H_ */


        
