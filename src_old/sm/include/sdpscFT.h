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
 *
 * $Id: sdpscFT.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment의 Fixed Table에 대한 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPSC_FT_H_
# define _O_SDPSC_FT_H_ 1

#include <sdpscDef.h>
#include <sdcTXSegFreeList.h>


/***********************************************************************
 * X$DISK_UNDO_EXTDIRHDR Dump Table의 레코드
 * Dump Extent Dir Control Header 정의
 ***********************************************************************/
typedef struct sdpscDumpExtDirCntlHdr
{
    UChar        mSegType;       // Dump한 Segment의 타입
    scSpaceID    mSegSpaceID;    // Segment의 TbsID
    scPageID     mSegPageID;     // Segment의 PID
    scPageID     mMyPageID;      // 현재 ExtDirCtlHeader의 PID
    UShort       mExtCnt;        // 페이지내의 Extent 개수
    scPageID     mNxtExtDir;     // 다음 Extent Map 페이지의 PID
    UShort       mMaxExtCnt;     // 최대 Extent개수 저장
    scOffset     mMapOffset;     // 페이지 내의 Extent Map의 Offset
    smSCN        mLstCommitSCN;  // 마지막 사용한 커밋한 트랜잭션의 CommitSCN
    smSCN        mFstDskViewSCN; // 사용했던 혹은 사용중인 트랜잭션의 Fst Disk ViewSCN

    UInt          mTxExtCnt;        // TSS Segment에 할당된 Extent 총 개수
    UInt          mExpiredExtCnt;   // Segment에 할당된 Expire상태의 Extent 개수
    UInt          mUnExpiredExtCnt; // Segment에 할당된 UnExpire 상태의 Extent 수
    UInt          mUnStealExtCnt;   // SegHdr와 지금 보고 있는 Ext
    smSCN         mSysMinViewSCN;   // Segment 조회할 당시의 minViewSCN
    UChar         mIsOnline;        // binding 되어 있는지 여부
} sdpscDumpExtDirCntlHdr;

/***********************************************************************
 * X$DISK_UNDO_SEGHDR Dump Table의 레코드
 * Dump Segment Header
 ***********************************************************************/
typedef struct sdpscDumpSegHdrInfo
{
    UChar         mSegType;       // Dump한 Segment의 타입
    scPageID      mSegPID;        // PID
    sdpSegState   mSegState;      // SEG_STATE
    UInt          mTotExtCnt;     // Segment에 할당된 Extent 총 개수
    scPageID      mLstExtDir;     // 마지막 ExtDir 페이지의 PID
    UInt          mTotExtDirCnt;  // Extent Map의 총 개수 (SegHdr 제외한개수)
    UInt          mPageCntInExt;  // Extent의 페이지 개수
    UShort        mExtCntInPage;  // 페이지내의 Extent 개수
    scPageID      mNxtExtDir;     // 다음 Extent Map 페이지의 PID
    UShort        mMaxExtCnt;     // 최대 Extent개수 저장
    scOffset      mMapOffset;     // 페이지 내의 Extent Map의 Offset
    smSCN         mLstCommitSCN;  // 마지막 사용한 커밋한 트랜잭션의 CommitSCN
    smSCN         mFstDskViewSCN; // 사용했던 혹은 사용중인 트랜잭션의 Fst Disk ViewSCN

    UInt          mTxExtCnt;        // TSS Segment에 할당된 Extent 총 개수
    UInt          mExpiredExtCnt;   // Segment에 할당된 Expire상태의 Extent 개수
    UInt          mUnExpiredExtCnt; // Segment에 할당된 UnExpire 상태의 Extent 수
    UInt          mUnStealExtCnt;   // SegHdr와 지금 보고 있는 Ext
    smSCN         mSysMinViewSCN;   // Segment 조회할 당시의 minViewSCN
    UChar         mIsOnline;        // binding 되어 있는지 여부 
} sdpscDumpSegHdrInfo;

class sdpscFT
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    // 세그먼트 헤더 Dump
    static IDE_RC dumpSegHdr( scSpaceID             aSpaceID,
                              scPageID              aPageID,
                              sdpSegType            aSegType,
                              sdcTXSegEntry       * aEntry,
                              sdpscDumpSegHdrInfo * aDumpSegHdr,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory );

    // ExtentDirectory헤더 Dump
    static IDE_RC dumpExtDirHdr( scSpaceID             aSpaceID,
                                 scPageID              aPageID,
                                 sdpSegType            aSegType,
                                 void                * aHeader,
                                 iduFixedTableMemory * aMemory );
    

    /* X$DISK_UNDO_SEGHDR */
    static IDE_RC buildRecord4SegHdr(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

    /* X$DISK_UNDO_EXTDIRHDR */
    static IDE_RC buildRecord4ExtDirHdr(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );
private:
    static SChar convertSegTypeToChar( sdpSegType aSegType );
    static SChar convertToChar( idBool aIsTrue );
};


#endif // _O_SDPSC_FT_H_

