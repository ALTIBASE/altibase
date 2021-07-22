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
 * $Id: stndrFT.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Disk RTree Index의 DUMP를 위한 함수
 *
 **********************************************************************/

#ifndef _O_STNDR_FT_H_
#define _O_STNDR_FT_H_  (1)



# include <idu.h>
# include <smDef.h>
# include <stndrDef.h>


//-------------------------------
// D$DISK_INDEX_RTREE_STRUCTURE 의 구조
//-------------------------------

typedef struct stndrDumpTreePage       // for stndrNodeHdr
{
    UShort         mHeight;           // HEIGHT
    SChar          mIsLeaf;           // IS_LEAF
    UInt           mMyPID;            // MY_PAGEID
    UInt           mNextPID;          // NEXT_PAGEID
    ULong          mPageSeq;          // PAGE_SEQ
    SShort         mSlotCount;        // SLOT_CNT
    ULong          mSmoNo;            // SMO_NO
    SChar          mNodeMBRString[SMN_DUMP_VALUE_LENGTH];  // NODE_MBR
    stdMBR         mNodeMBR;          // MIN_X, MIN_Y, MAX_X, MAX_Y
    UShort         mTotalFreeSize;    // TOTAL_FREE_SIZE
    UShort         mUnlimitedKeyCount;// UNLIMITED_KEY_COUNT
    UShort         mTotalDeadKeySize; // TOTAL_DEAD_KEY_SIZE
    UShort         mTBKCount;         // TBK_COUNT
    UChar          mPageState;        // PAGE_STATE
    UShort         mCTLayerSize;      // CTL_SIZE
    UShort         mCTSlotUsedCount;  // CTS_USED_COUNT
    SChar          mIsConsistent;     // IS_CONSISTENT
} stndrDumpTreePage;

//-------------------------------
// D$DISK_INDEX_RTREE_KEY 의 구조
//-------------------------------

typedef struct stndrDumpKey            // for stndrIKey, stndrLKey
{
    UInt           mMyPID;            // MY_PAGEID
    ULong          mPageSeq;          // PAGE_SEQ
    UShort         mHeight;           // HEIGHT
    SChar          mIsLeaf;           // IS_LEAF
    SShort         mNthSlot;          // NTH_SLOT
    UShort         mColumnLength;     // COLUMN_LENGTH    : PROJ-1872
    SChar          mValue[SMN_DUMP_VALUE_LENGTH];  // VALUE24B
    stdMBR         mMBR;              // MIN_X, MIN_Y, MAX_X, MAX_Y
    UInt           mChildPID;         // CHILD_PAGEID
    UInt           mRowPID;           // ROW_PAGEID
    UInt           mRowSlotNum;       // ROW_SLOTNUM
    UChar          mState;            // STATE
    UShort         mCreateCTS;        // CREATE_CTS_NO
    UChar          mCreateChained;    // CREATE_CHAINED
    UShort         mLimitCTS;         // LIMIT_CTS_NO
    UChar          mLimitChained;     // LIMIT_CHAINED
    UChar          mTxBoundType;      // TB_TYPE
    ULong          mCreateTSS;        // CREATE_TSS_RID
    ULong          mLimitTSS;         // LIMIT_TSS_RID
    SChar*         mCreateCSCN;       // CREATE_COMMIT_SCN
    SChar*         mLimitCSCN;        // LIMIT_COMMIT_SCN
    SChar*         mCreateSCN;        // CREATE_SCN
    SChar*         mLimitSCN;         // LIMIT_SCN
} stndrDumpKey;



//-------------------------------
// X$DISK_RTREE_HEADER 의 구조
//-------------------------------
typedef struct stndrHeader4PerfV
{
    UChar             mName[SMN_MAX_INDEX_NAME_SIZE+8]; // INDEX_NAME
    UInt              mIndexID;             // INDEX_ID
    
    UInt              mIndexTSID;           // INDEX_TBS_ID
    UInt              mTableTSID;           // TABLE_TBS_ID
    UInt              mSegHdrPID;           // SEG_HDR_PAGEID
    
    UInt              mRootNode;            // ROOT_PAGEID
    UInt              mEmptyNodeHead;       // EMPTY_HEAD_PAGEID
    UInt              mEmptyNodeTail;       // EMPTY_TAIL_PAGEID
    ULong             mSmoNo;               // SMO_NO

    UInt              mFreeNodeHead;        // FREE_NODE_HEAD
    ULong             mFreeNodeCnt;         // FREE_NODE_CNT
    SChar*            mFreeNodeSCN;         // FREE_NODE_SCN
    
    SChar             mIsConsistent;        // IS_CONSISTENT
    SChar             mIsCreatedWithLogging;// IS_CREATED_WITH_LOGGING
    SChar             mIsCreatedWithForce;  // IS_CREATED_WITH_FORCE
    smLSN             mCompletionLSN;       // COMPLETION_LSN_FILE_NO
                                            // COMPLETION_LSN_FILE_OFFSET
    stdMBR            mTreeMBR;
    
    smiSegAttr        mSegAttr;             // Segment Attribute
    smiSegStorageAttr mSegStoAttr;          // Segment Storage Attribute
} stndrHeader4PerfV;



//-------------------------------
// X$DISK_RTREE_STAT 의 구조
//-------------------------------
typedef struct stndrStat4PerfV
{
    UChar           mName[SMN_MAX_INDEX_NAME_SIZE+8];
    UInt            mIndexID;
    ULong           mTreeLatchReadCount;
    ULong           mTreeLatchWriteCount;
    ULong           mTreeLatchReadMissCount;
    ULong           mTreeLatchWriteMissCount;
    ULong           mKeyCount;
    stndrStatistic  mDMLStat;
    stndrStatistic  mQueryStat;
    ULong           mCardinality;
    stdMBR          mTreeMBR;
} stndrStat4PerfV;




class stndrFT
{
public:
    //------------------------------------------
    // For D$DISK_INDEX_RTREE_CTS
    //------------------------------------------
    static IDE_RC buildRecordCTS( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * aDumpObj,
                                  iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildCTS( idvSQL              * aStatistics,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    stndrHeader         * aIdxHdr,
                                    scSpaceID             aSpaceID,
                                    stndrStack          * aTraverseStack,
                                    ULong               * aPageSeq );

    //------------------------------------------
    // For D$DISK_INDEX_RTREE_STRUCTURE
    //------------------------------------------
    static IDE_RC buildRecordTreeStructure( idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildTreePage( idvSQL              * aStatistics,
                                         void                * aHeader,
                                         iduFixedTableMemory * aMemory,
                                         stndrHeader         * aIdxHdr,
                                         scSpaceID             aSpaceID,
                                         stndrStack          * aTraverseStack,
                                         ULong               * aPageSeq );

    //------------------------------------------
    // For D$DISK_INDEX_RTREE_KEY
    //------------------------------------------
    static IDE_RC buildRecordKey( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * aDumpObj,
                                  iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildKey( idvSQL              * aStatistics,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    stndrHeader         * aIdxHdr,
                                    scSpaceID             aSpaceID,
                                    stndrStack          * aTraverseStack,
                                    ULong               * aPageSeq );
    
    static IDE_RC convertMBR2String( UChar  * sSrcKeyPtr,
                                     UChar  * aDestString );
    
    //------------------------------------------
    // For X$DISK_RTREE_STAT
    //------------------------------------------
    static IDE_RC buildRecordForDiskRTreeHeader(idvSQL              * /*aStatistics*/,
                                                void                * aHeader,
                                                void                * aDumpObj,
                                                iduFixedTableMemory * aMemory);

    //------------------------------------------
    // For X$DISK_RTREE_HEADER
    //------------------------------------------
    static IDE_RC buildRecordForDiskRTreeStat(idvSQL              * /*aStatistics*/,
                                              void                *aHeader,
                                              void                */*aDumpObj*/,
                                              iduFixedTableMemory *aMemory);

    
    //------------------------------------------
    // For X$DISK_TEMP_RTREE_STAT
    //------------------------------------------
    static IDE_RC buildRecordForDiskTempRTreeStat(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * /* aDumpObj */,
        iduFixedTableMemory * aMemory);


};



#endif /* _O_STNDR_FT_H_ */
