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
 * $Id: sdpsfDef.h 27220 2008-07-23 14:56:22Z newdaily $
 ***********************************************************************/

# ifndef _O_SDPSF_DEF_H_
# define _O_SDPSF_DEF_H_ 1

#define SDPSF_SEGHDR_OFFSET (idlOS::align8((UInt)ID_SIZEOF(sdpPhyPageHdr)))

/* --------------------------------------------------------------------
 * 페이지의 physical header에 저장된다.
 * 페이지의 상태를 표시한다.
 * extent desc의 표시되는 페이지의 상태와 physical header에
 * 표시되는 페이지의 상태가 같은 상태로 유지될 것이다.
 * 이것이 다르다면 중간에 서버가 죽었던 상태일 수 있으므로,
 * 적절히 처리해야 한다.
 * ----------------------------------------------------------------- */
typedef enum
{
    // page가 used이고 insert 가능한 상태
    // insertable 여부를 사용하지 않는 페이지라면 used를 의미
    SDPSF_PAGE_USED_INSERTABLE = 0,

    // page가 used이고 insert 불가능한 상태
    // 오직 update만 가능하다.
    SDPSF_PAGE_USED_UPDATE_ONLY,

    // page가 free인 상태
    SDPSF_PAGE_FREE

} sdpsfPageState;

/* Segment Cache 자료구조 (Runtime 정보) */
typedef struct sdpsfSegCache
{
    sdpSegCCache  mCommon;
} sdpsfSegCache;

//XXX Name을 수정해야 됨.
#define SDP_SF_EXTDESC_FST_IS_EXTDIRPAGE_MASK  (0x00000001)
#define SDP_SF_EXTDESC_FST_IS_EXTDIRPAGE_TRUE  (0x00000001)
#define SDP_SF_EXTDESC_FST_IS_EXTDIRPAGE_FALSE (0x00000000)

#define SDP_SF_IS_FST_EXTDIRPAGE_AT_EXT( aFlag ) \
    ( ( aFlag & SDP_SF_EXTDESC_FST_IS_EXTDIRPAGE_MASK ) == SDP_SF_EXTDESC_FST_IS_EXTDIRPAGE_TRUE )

/* --------------------------------------------------------------------
 * Extent Descriptor의 정보
 * ----------------------------------------------------------------- */
typedef struct sdpsfExtDesc
{
    // 이 extent에 속한 첫번째 페이지의 ID
    scPageID           mFstPID;
    // Extent의 상태정보: ex) 첫번째 페이지가 ExtDirPage인가?
    UInt               mFlag;
} sdpsfExtDesc;

// Extent Direct Control Header 정의
typedef struct sdpsfExtDirCntlHdr
{
    UShort       mExtDescCnt;         // 페이지내의 ExtentDesc 개수
    UShort       mMaxExtDescCnt;      // 페이지내의 최대 ExtentDesc 개수
    scOffset     mFstExtDescOffset;   // 페이지 내의 ExtentDesc의 첫번째 Offset

    /* PROJ-1497 DB Migration을 방지하기 위해 Reserved 영역을 추가 */
    ULong        mReserveArea[SDP_EXTDIR_PAGEHDR_RESERV_SIZE];
} sdpsfExtDirCntlHdr;

/* --------------------------------------------------------------------
 * Segement Descriptor의 정보
 *
 * extent의 리스트는 항상 seg desc에 존재 해야 한다.
 * table seg hdr에 존재 할 수 없다.
 * 왜냐하면 table seg hdr를 최초에 할당받기 위해서는
 * 먼저 extent를 할당받을 수 있어야 하는데 table seg hdr에
 * extent list를 두었을 경우 애초에 extent를 할당받을 수
 * 있는 방법이 없다.
 * ----------------------------------------------------------------- */
typedef struct sdpsfSegHdr
{
    /* Segment Hdr PageID */
    scPageID           mSegHdrPID;

    /* table or index or undo or tss or temp */
    UShort             mType;

    /* 할당 상태 */
    UShort             mState;

    /* PageCnt In Ext */
    UInt               mPageCntInExt;

    /* Max Extent Cnt In Segment Header */
    UShort             mMaxExtCntInSegHdrPage;
    /* Max Extent Cnt In Extent Directory Page */
    UShort             mMaxExtCntInExtDirPage;

    /* Direct Path Insert나 Segment Merge시 HWM이 뒤로 이동하는데
     * 이때 중간에 있는 Extent에 있는 UFmt Page를 이 리스트에 추가한다.
     * 왜냐하면 DPath Insert나 Merge가 Rollback시 Add된 Free Page를
     * Free해야 하는데 Ager가 동시에 돌 경우 UFmtPageList에 대한 변경이
     * 발생하여 Add된 페이지를 rollback시 제거할 수 없다. Commit전까지
     * 다른 Trasnsaction이 접근하지 않는 Private FreePage List를 둔다.*/
    sdpSglPIDListBase  mPvtFreePIDList;

    /* 완전히 빈 Page List( External, LOB, Multiple Column의 Page가
     * 한번할당 되고 Row가 Free되어 완전히 비면 FreePage List가 아닌
     * Unformated Page List에 반환된다. */
    sdpSglPIDListBase  mUFmtPIDList;

    /* Insert할 공간이 있는 PageList */
    sdpSglPIDListBase  mFreePIDList;

    /* Segment에서 한번이라도 할당된 Page의 갯수 */
    ULong              mFmtPageCnt;

    /* Segment에서 Format된 마지막 페이지를 가리킨다. */
    scPageID           mHWMPID;

    /* Meta Page ID Array: Index Seg는 이 PageID Array 에 Root
     * Node PID를 기록한다. */
    scPageID           mArrMetaPID[ SDP_MAX_SEG_PID_CNT ];

    /* Segment에서 현재 Page Alloc을 위해 사용중인 Ext RID */
    sdRID              mAllocExtRID;

    /* Segment에서 현재 Page Alloc을 위해 사용중인 Extent의
     * 첫번째 PID */
    scPageID           mFstPIDOfAllocExt;


    /** Extent Management에 관련된 정보를 기록한다. **/
    /* Ext Desc의 Free 리스트 */
    sdpDblPIDListBase  mExtDirPIDList;

    /* Segment가 가지고 있는 Extent의 Total Count */
    ULong              mTotExtCnt;

    /* Segment Header의 남는 공간을 ExtDesc를 기록하기
     * 위해 사용한다. */
    sdpsfExtDirCntlHdr mExtDirCntlHdr;
} sdpsfSegHdr;

#endif // _O_SDPSF_DEF_H_
