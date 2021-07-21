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
 * $Id: sdnFT.h 19550 2007-02-07 03:09:40Z leekmo $
 *
 * Disk Index의 DUMP를 위한 함수
 * Btree와 RTree 공통적인 것들을 Dump한다.
 *
 **********************************************************************/

#ifndef _O_SDN_DUMP_H_
#define _O_SDN_DUMP_H_  (1)

# include <idu.h>
# include <smDef.h>
# include <sdnbDef.h>

//-------------------------------
// D$DISK_INDEX_CTS 의 구조
//-------------------------------

typedef struct sdnDumpCTS            
{
    UInt           mMyPID;            // MY_PAGEID
    ULong          mPageSeq;          // PAGE_SEQ
    SShort         mNthSlot;          // NTH_SLOT
    SChar         *mCommitSCN;        // COMMIT_SCN
    SChar         *mNxtCommitSCN;     // NEXT_COMMIT_SCN - unused
    UChar          mState;            // STATE
    UChar          mChained;          // CHAINED - unused
    UInt           mTSSlotPID;        // TSS_PAGEID
    UShort         mTSSlotNum;        // TSS_SLOTNUM
    UInt           mUndoPID;          // UNDO_PAGEID  - unused
    UShort         mUndoSlotNum;      // UNDO_SLOTNUM - unused
    UShort         mRefCnt;           // REF_CNT
    UShort         mRefKey1;          // REF_KEY1
    UShort         mRefKey2;          // REF_KEY2
    UShort         mRefKey3;          // REF_KEY3
} sdnDumpCTS;

class sdnFT
{
public:

    //------------------------------------------
    // For D$DISK_INDEX_CTS
    //------------------------------------------

    static IDE_RC buildRecordCTS( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * aDumpObj,
                                  iduFixedTableMemory * aMemory );

    static IDE_RC traverseBuildCTS( idvSQL*               aStatistics,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    sdnbHeader          * aIdxHdr,
                                    sdrMtx              * aMiniTx,
                                    UShort                aDepth,
                                    SShort                aNthSibling,
                                    scSpaceID             aSpaceID,
                                    scPageID              aMyPID,
                                    ULong               * aPageSeq );
};


#endif /* _O_SDN_DUMP_H_ */


        
