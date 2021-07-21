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
 * $Id: sdtDef.h 79989 2017-05-15 09:58:46Z et16 $
 * Description :
 **********************************************************************/

#ifndef _O_SDT_DEF_H_
#define _O_SDT_DEF_H_ 1

#include <smDef.h>
#include <iduLatch.h>
#include <smuUtility.h>
#include <iduMutex.h>

typedef enum
{
    SDT_WA_PAGESTATE_INIT,          /* NPage가 할당되어 있지 않음. */
    SDT_WA_PAGESTATE_CLEAN,         /* Assign돼었으며 내용이 동일함 */
    SDT_WA_PAGESTATE_DIRTY          /* 내용이 변경되었고, 아직 Write되지 않았음*/
} sdtWAPageState;

typedef enum
{
    SDT_WORK_TYPE_NONE,
    SDT_WORK_TYPE_HASH,
    SDT_WORK_TYPE_SORT,
    SDT_WORK_TYPE_MAX
} sdtWorkType;

typedef struct sdtWCB
{
    sdtWAPageState mWPState;
    SInt           mFix;

    /* 자신의 Page 포인터 */
    UChar        * mWAPagePtr;
    idBool         mBookedFree; /* Free가 예약됨 */
    /* 이 WAPage와 연결된 NPage : 디스크에 존재한다.  */
    scPageID       mNPageID;

    /* 페이지 탐색용 Hash를 위한 LinkPID */
    sdtWCB       * mNextWCB4Hash;
    sdtWCB       * mNxtUsedWCBPtr;

    /* LRUList관리용 */
    scPageID       mLRUPrevPID;
    scPageID       mLRUNextPID;
} sdtWCB;

/* INMEMORY -> DiscardPage를 허용하지 않는다. 또한 뒤쪽부터 페이지를 할당
 *             해준다. WAMap과의 적합성을 올리기 위함이다.
 * FIFO -> 앞쪽부터 순서대로 할당해준다. 전부 사용하면, 앞쪽부터 재사용한다.
 * LRU  -> 사용한지 오래된 페이지를 할당한다. getWAPage를 하면 Top으로 올린다.
 * */
typedef enum
{
    SDT_WA_REUSE_NONE,
    SDT_WA_REUSE_INMEMORY, /* REUSE를 허용하지 않으며,뒤에서부터 페이지 할당*/
    SDT_WA_REUSE_FIFO,     /* 순차적으로 페이지를 사용함 */
    SDT_WA_REUSE_LRU       /* LRU에 따라 페이지를 할당해줌 */
} sdtWAReusePolicy;

#define SDT_MAX_WAFLUSHER_COUNT (64)

#define SDT_USED_PAGE_PTR_TERMINATED ((sdtWCB*)1)

/* Extent 하나당 Page개수 64개로 설정 */
#define SDT_WAEXTENT_PAGECOUNT      (64)
#define SDT_WAEXTENT_PAGECOUNT_MASK (SDT_WAEXTENT_PAGECOUNT-1)

/* WGRID, NGRID 등을 구별하기 위해, SpaceID를 사용함 */
#define SDT_SPACEID_WORKAREA  ( ID_USHORT_MAX )     /*InMemory,WGRID */
#define SDT_SPACEID_WAMAP     ( ID_USHORT_MAX - 1 ) /*InMemory,WAMap Slot*/
/* Slot을 아직 사용하지 않은 상태 */
#define SDT_WASLOT_UNUSED     ( ID_UINT_MAX )

#define SDT_WAEXTENT_SIZE     (SD_PAGE_SIZE * SDT_WAEXTENT_PAGECOUNT)


typedef struct sdtWAExtent
{
    // Page를 Flush 하기 위해선 align을 맞춰야 한다.
    UChar         mPage[SDT_WAEXTENT_PAGECOUNT][SD_PAGE_SIZE];
    // align을 쉽게 맞추기 위해 Next Extent는 Page 뒤에 위치
    sdtWAExtent * mNextExtent;
}sdtWAExtent;

#define SDT_NEXTARR_EXTCOUNT (128)

typedef struct sdtNExtentArr
{
    scPageID        mMap[SDT_NEXTARR_EXTCOUNT];
    sdtNExtentArr * mNextArr;
} sdtNExtentArr;

typedef struct sdtNExtFstPIDList
{
    ULong      mCount;
    UInt       mPageSeqInLFE;   /*LFE(LastFreeExtent)내에서 할당해서 건내준페이지 번호 */
    scPageID   mLastFreeExtFstPID; /*NExtentArray의 마지막의 Extent*/
    sdtNExtentArr  * mHead;
    sdtNExtentArr  * mTail;
} sdtNExtFstPIDList;

typedef struct sdtWAExtentInfo
{
    sdtWAExtent   * mHead;
    sdtWAExtent   * mTail;
    UInt            mCount;
} sdtWAExtentInfo;

#define SDT_WORKAREA_IN_MEMORY      (1)
#define SDT_WORKAREA_OUT_MEMORY     (0)

#define SDT_TEMP_FREEOFFSET_BITSIZE (13)
#define SDT_TEMP_FREEOFFSET_BITMASK (0x1FFF)
#define SDT_TEMP_TYPE_SHIFT         (SDT_TEMP_FREEOFFSET_BITSIZE)
#define SDT_TEMP_TYPE_BITMASK       (0xE000)

#define SDT_TEMP_SLOT_UNUSED      (ID_USHORT_MAX)

typedef enum
{
    SDT_TEMP_PAGETYPE_INIT,
    SDT_TEMP_PAGETYPE_INMEMORYGROUP,
    SDT_TEMP_PAGETYPE_SORTEDRUN,
    SDT_TEMP_PAGETYPE_LNODE,
    SDT_TEMP_PAGETYPE_INODE,
    SDT_TEMP_PAGETYPE_INDEX_EXTRA,
    SDT_TEMP_PAGETYPE_HASHROWS,
    SDT_TEMP_PAGETYPE_SUBHASH,
    SDT_TEMP_PAGETYPE_MAX
} sdtTempPageType;


/*****************************************************************************
 * PROJ-2201 Innovation in sorting and hashing(temp)
 *****************************************************************************/

#define SDT_TRFLAG_NULL              (0x00)
#define SDT_TRFLAG_HEAD              (0x01) /*Head RowPiece인지 여부.*/
#define SDT_TRFLAG_NEXTGRID          (0x02) /*NextGRID를 사용하는가?*/
#define SDT_TRFLAG_CHILDGRID         (0x04) /*ChildGRID를 사용하는가?*/
#define SDT_TRFLAG_UNSPLIT           (0x10) /*쪼개지지 않도록 설정함*/
/* GRID가 설정되어 있는가? */
#define SDT_TRFLAG_GRID     ( SDT_TRFLAG_NEXTGRID | SDT_TRFLAG_CHILDGRID )

// 부분 %로 전체 100%의 크기를 계산한다. 1보다 작으면 1로 가정
#define SDT_GET_FULL_SIZE( aPartSize, aPartRatio )                 \
    (( (SInt)(aPartRatio) < 1 ) ?                                  \
     ( (aPartSize) * 100 ) :                                       \
     ( ((aPartSize) * 100 + (aPartRatio-1)) / (aPartRatio) ))

#endif  // _O_SDT_DEF_H_
