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
 * $Id: stnmrFT.h 19550 2007-02-07 03:09:40Z kimmkeun $
 *
 * Memory RTree Index의 DUMP를 위한 함수
 *
 **********************************************************************/

#ifndef _O_STNMR_FT_H_
#define _O_STNMR_FT_H_  (1)

# include <idu.h>
# include <smDef.h>
# include <stnmrDef.h>

//-------------------------------
// D$MEM_INDEX_RTREE_STRUCTURE의 구조
//-------------------------------

typedef struct stnmrDumpTreePage
{
    UShort         mDepth;            // DEPTH
    SChar          mIsLeaf;           // IS_LEAF
    SChar        * mMyPagePtr;        // MY_PAGE_PTR
    SShort         mSlotCount;        // SLOT_CNT
    SChar        * mParentPagePtr;    // PARENT_PAGE_PTR
} stnmrDumpTreePage;

//-------------------------------
// D$MEM_INDEX_RTREE_KEY 의 구조
//-------------------------------

typedef struct stnmrDumpKey  
{
    SChar        * mMyPagePtr;        // MY_PAGE_PTR
    UShort         mDepth;            // DEPTH
    SChar          mIsLeaf;           // IS_LEAF
    SChar        * mParentPagePtr;    // PARENT_PAGE_PTR
    SShort         mNthSlot;          // NTH_SLOT
    SDouble        mMinX;             // MIN_X
    SDouble        mMinY;             // MIN_Y
    SDouble        mMaxX;             // MAX_X 
    SDouble        mMaxY;             // MAX_Y
    SChar        * mChildPagePtr;     // CHILD_PAGE_PTR
    SChar        * mRowPtr;           // ROW_PTR
} stnmrDumpKey;

class stnmrFT 
{
public:

    //------------------------------------------
    // For D$MEM_INDEX_RTREE_STRUCTURE
    //------------------------------------------
    static IDE_RC buildRecordTreeStructure( idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildTreePage( void                * aHeader,
                                         iduFixedTableMemory * aMemory,
                                         SInt                  aDepth,
                                         stnmrNode           * aCurNode,
                                         stnmrNode           * aParentNode );

    //------------------------------------------
    // For D$MEM_INDEX_RTREE_KEY
    //------------------------------------------
    static IDE_RC buildRecordKey( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * aDumpObj,
                                  iduFixedTableMemory * aMemory );
    
    static IDE_RC traverseBuildKey( void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    stnmrHeader         * aIdxHdr,
                                    SInt                  aDepth,
                                    stnmrNode           * aCurNode,
                                    stnmrNode           * aParentNode );
};


#endif /* _O_SMNB_FT_H_ */
