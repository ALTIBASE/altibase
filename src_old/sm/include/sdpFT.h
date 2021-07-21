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
 * $Id: sdpFT.h 19550 2010-01-07
 **********************************************************************/

#ifndef _O_SDP_FT_H_
#define _O_SDP_FT_H_  (1)

# include <idu.h>
# include <smDef.h>
# include <smpFT.h>
# include <sdcDef.h>

/* TASK-4007 [SM] PBT를 위한 기능 추가 
 *
 * Page dump를 위한 fixed table 정의*/

//----------------------------------------
// D$DISK_DB_PAGE
//-----------------------------------------
typedef struct sdpDiskDBPageDump
{
    scSpaceID mTBSID;
    scPageID  mPID;
    SChar     mPageDump[SMP_DUMP_COLUMN_SIZE];
} sdpDiskDBPageDump;

//----------------------------------------
// D$DISK_DB_PHYPAGEHDR
//----------------------------------------- 
typedef struct sdpDiskDBPhyPageHdrDump
{
    UInt     mCheckSum;
    UInt     mPageLSNFILENo;
    UInt     mPageLSNOffset;
    ULong    mIndexSMONo;
    UShort   mSpaceID;
    UShort   mTotalFreeSize;
    UShort   mAvailableFreeSize;
    UShort   mLogicalHdrSize;
    UShort   mSizeOfCTL;
    UShort   mFreeSpaceBeginOffset;
    UShort   mFreeSpaceEndOffset;
    UShort   mPageType;
    UShort   mPageState;
    scPageID mPageID;
    UChar    mIsConsistent;
    UInt     mIndexID;
    UShort   mLinkState;
    scPageID mParentPID;
    SShort   mIdxInParent;
    scPageID mPrevPID;
    scPageID mNextPID;
    smOID    mTableOID;
} sdpDiskDBPhyPageHdrDump;

//----------------------------------------- 
// X$SEGMENT
//----------------------------------------- 
typedef struct sdpSegmentPerfV
{
    UInt         mSpaceID;             /* TBS ID */
    ULong        mTableOID;            /* Table OID */
    ULong        mObjectID;            /* Object ID */
    scPageID     mSegPID;              /* 세그먼트 페이지 PID */
    sdpSegType   mSegmentType;         /* Segment Type */
    UInt         mState;               /* Segment 할당여부 */
    ULong        mExtentTotalCnt;      /* Extent 총 개수 */

    /* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다. */
    ULong        mUsedTotalSize;       /* 세그먼트 사용양 */
} sdpSegmentPerfV;

class sdpFT
{
public:

    //------------------------------------------
    // D$DISK_DB_PAGE
    // ReadPage( Fix + latch )한 후 Binary를 바로 Dump한다.
    //------------------------------------------
    static IDE_RC buildRecordDiskDBPageDump(idvSQL              * /*aStatistics*/,
                                            void                *aHeader,
                                            void                *aDumpObj,
                                            iduFixedTableMemory *aMemory);

    //------------------------------------------
    // D$DISK_DB_PHYPAGEHDR,
    // PhysicalPageHdr를 Dump한다.
    //------------------------------------------
    static IDE_RC buildRecordDiskDBPhyPageHdrDump(idvSQL              * /*aStatistics*/,
                                                  void                *aHeader,
                                                  void                *aDumpObj,
                                                  iduFixedTableMemory *aMemory);

    //------------------------------------------
    // X$SEGMENT            
    //------------------------------------------
    static IDE_RC buildRecordForSegment(idvSQL              * /*aStatistics*/,
                                        void        *aHeader,
                                        void        *aDumpObj,
                                        iduFixedTableMemory *aMemory);

    static IDE_RC getSegmentInfo( UInt               aSpaceID,
                                  scPageID           aSegPID,
                                  ULong              aTableOID,
                                  sdpSegCCache     * aSegCache,
                                  sdpSegmentPerfV  * aSegmentInfo );
};


#endif /* _O_SDP_FT_H_ */
