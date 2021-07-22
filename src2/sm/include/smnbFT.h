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
 * $Id: smnbFT.h 19550 2007-02-07 03:09:40Z kimmkeun $
 *
 * Memory BTree Index의 FT를 위한 함수
 *
 **********************************************************************/

#ifndef _O_SMNB_FT_H_
#define _O_SMNB_FT_H_  (1)

# include <idu.h>
# include <smDef.h>

//-------------------------------
// D$MEM_INDEX_BTREE_STRUCTURE의 구조
//-------------------------------

typedef struct smnbDumpTreePage
{
    UShort         mDepth;            // DEPTH
    SChar          mIsLeaf;           // IS_LEAF
    SChar        * mMyPagePtr;        // MY_PAGE_PTR
    SShort         mSlotCount;        // SLOT_CNT
    SChar        * mParentPagePtr;    // PARENT_PAGE_PTR
} smnbDumpTreePage;

//-------------------------------
// D$MEM_INDEX_BTREE_KEY 의 구조
//-------------------------------

typedef struct smnbDumpKey
{
    SChar        * mMyPagePtr;        // MY_PAGE_PTR
    UShort         mDepth;            // DEPTH
    SChar          mIsLeaf;           // IS_LEAF
    SChar        * mParentPagePtr;    // PARENT_PAGE_PTR
    SShort         mNthSlot;          // NTH_SLOT
    SShort         mNthColumn;        // NTH_COLUMN
    SChar          mValue[SMN_DUMP_VALUE_LENGTH];  // VALUE24B
    SChar        * mChildPagePtr;     // CHILD_PAGE_PTR
    SChar        * mRowPtr;           // ROW_PTR
} smnbDumpKey;

class smnbFT
{
public:

    //------------------------------------------
    // For D$MEM_INDEX_BTREE_STRUCTURE
    //------------------------------------------

    static IDE_RC buildRecordTreeStructure( idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildTreePage( void                * aHeader,
                                         iduFixedTableMemory * aMemory,
                                         SInt                  aDepth,
                                         smnbNode            * aCurNode,
                                         smnbNode            * aParentNode );


    //------------------------------------------
    // For D$MEM_INDEX_BTREE_KEY
    //------------------------------------------
    static IDE_RC buildRecordKey( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * aDumpObj,
                                  iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildKey( void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    smnbHeader          * aIdxHdr,
                                    SInt                  aDepth,
                                    smnbNode            * aCurNode,
                                    smnbNode            * aParentNode );

    //-----------------------------------------
    // For X$MEM_BTREE_HEADER
    //-----------------------------------------
    static IDE_RC buildRecordForMemBTreeHeader(idvSQL              * /*aStatistics*/,
                                               void                * aHeader,
                                               void                * /* aDumpObj */,
                                               iduFixedTableMemory * aMemory);

    //-----------------------------------------
    // For X$VOL_BTREE_HEADER
    //-----------------------------------------
    static IDE_RC buildRecordForVolBTreeHeader(idvSQL              * /*aStatistics*/,
                                               void                * aHeader,
                                               void                * /* aDumpObj */,
                                               iduFixedTableMemory * aMemory);

    static IDE_RC buildRecordForBTreeHeader(void                * aHeader,
                                            iduFixedTableMemory * aMemory,
                                            UInt                  aTableType,
                                            smcTableHeader      * aCatTblHdr );

    //-----------------------------------------
    // For X$TEMP_BTREE_HEADER
    //-----------------------------------------
    static IDE_RC buildRecordForTempBTreeHeader(idvSQL              * /*aStatistics*/,
                                                void                * aHeader,
                                                void                * /* aDumpObj */,
                                                iduFixedTableMemory * aMemory);

    //-----------------------------------------
    // For X$MEM_BTREE_STAT
    //-----------------------------------------
    static IDE_RC buildRecordForMemBTreeStat(idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory);

    //-----------------------------------------
    // For X$VOL_BTREE_STAT
    //-----------------------------------------
    static IDE_RC buildRecordForVolBTreeStat(idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory);

    static IDE_RC buildRecordForBTreeStat(void                * aHeader,
                                          iduFixedTableMemory * aMemory,
                                          UInt                  aTableType,
                                          smcTableHeader      * aCatTblHdr);

    //-----------------------------------------
    // For X$TEMP_BTREE_STAT
    //-----------------------------------------
    static IDE_RC buildRecordForTempBTreeStat(idvSQL              * /*aStatistics*/,
                                              void                * aHeader,
                                              void                * /* aDumpObj */,
                                              iduFixedTableMemory * aMemory);

    //-----------------------------------------
    // For X$MEM_BTREE_NODEPOOL
    //-----------------------------------------
    // BUG-18122 : MEM_BTREE_NODEPOOL performance view 추가
    static IDE_RC buildRecordForMemBTreeNodePool(idvSQL              * /*aStatistics*/,
                                                 void                * aHeader,
                                                 void                * /* aDumpObj */,
                                                 iduFixedTableMemory * aMemory);

};


#endif /* _O_SMNB_FT_H_ */
