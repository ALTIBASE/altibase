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
 * $Id: $
 **********************************************************************/

#ifndef _O_SDT_SORT_DEF_H_
# define _O_SDT_SORT_DEF_H_ 1

#include <sdtDef.h>

/*****************************************************************************
 * PROJ-2201 Innovation in sorting and hashing(temp)
 *****************************************************************************/

/*********************************************************************
 *
 * - SortTemp의 State Transition Diagram
 *      +------------------------------------------------------+
 *      |                                                      |
 * +------------+                                         +----------+
 * |InsertNSort |----<sort()>--+                          |InsertOnly|
 * +------------+   +----------+-------------------+      +----------+
 *      |           |          |                   |           |
 *      |   +------------+ +-------+ +---------+   |           |
 *   sort() |ExtractNSort|-| Merge |-|MakeIndex| sort()      scan()
 *      |   +------------+ +-------+ +---------+   |           |
 *      |                      |          |        |           |
 * +------------+        +---------+ +---------+   |      +----------+
 * |InMemoryScan|        |MergeScan| |IndexScan|---+      |   Scan   |
 * +------------+        +---------+ +---------+          +----------+
 *
 * 위 상태에 따라 Group들이 구분되며, Group에 따라 아래 Copy 연산이 달라진다.
 *
 *
 *
 * 1. InsertNSort
 *  InsertNSort는 최로로 데이터가 삽입되는 상태이다. 다만 삽입하면서 동시에
 * 정렬도 한다.
 *  그리고 이 상태는 Limit,MinMax등 InMemoryOnly 상태이냐, Range이냐, 그외냐
 *  에 따라 Group의 모양이 달라진다.
 *
 * 1) InsertNSort( InMemoryOnly )
 *  Limit, MinMax등 반드시 페이지 부족이 일어나지 않는다는 보장이 있을 경우
 *  InMemoryOnly로 수행한다.
 * +-------+ +-------+
 * | Sort1 |-| Sort2 |
 * +-------+ +-------+
 *     |         |
 *     +<-SWAP->-+
 * 두개의 SortGroup이 있고, 한 SortGroup이 가득 찰때마다 Compaction용도로
 * SWAP을 수행한다.
 *
 * 2) InsertNSort ( Normal )
 *  일반적인 형태로 Merge등을 하는 형태이다.
 * +-------+ +-------+
 * | Sort  |-| Flush |
 * +-------+ +-------+
 *     |          |
 *     +-NORMAL->-+
 *
 * 3) ExtractNSort ( RangeScan )
 *  RangeScan이 필요해 Index를 만드는 형태이다. Normal과 같지만,
 * Key와 Row를 따로 저장한다. Row에는 모든 칼럼이 들어있고, Key는 IndexColumn
 * 만 들어간다. 이후 Merge등의 과정을 Key를 가지고 처리한다.
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *     |           |        |
 *     +-MAKE_KEY--+        |
 *     |                    |
 *     +-<-<-<-EXTRACT_ROW--+
 *
 *
 *
 *  2. ExtractNSort
 *  InsertNSort의 1.3과 동일하지만, insert는 데이터를 QP로부터 받는데반해
 *  Extract는 1.3 과정에서 SubFlush에 복사해둔 Row를 다시 꺼내어 정렬한다는
 *  점이 특징이다.
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *     |           |        |
 *     +-MAKE_KEY->+        |
 *     |                    |
 *     +-<-<--<-NORMAL------+
 *
 *
 *
 *  3. Merge
 *  SortGroup에 Heaq을 구성한 후 FlushGroup으로 복사하며, FlushGroup으로
 *  Run을 구성한다. 즉 InsertNSort(Normal) 1.2와 동일하다.
 *
 *
 *
 *  4. MakeIndex
 *  Merge와 유사하지만, ExtraPage라는 것으로 Row를 쪼갠다. 왜냐하면 index의
 *  한 Node에는 2개의 Key가 들어가야 안정적이며, 따라서 2개 이상의 Key가
 *  들어갈 수 있도록, Key가 4000Byte를 넘으면 남는 Row가 4000Byte가 되도록
 *  4000Byte 이후의 것들을 ExtraPage에 저장시킨다.
 *
 *  1) LeafNode 생성
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *   |        |              | |
 *   |        +-<-MAKE_LNODE-+ |
 *   |                         |
 *   +-(copyExtraRow())-->->->-+
 *
 *  2) InternalNode 생성
 *  LeafNode는 Depth를 아래에서 위로 올라가며 만드는 방식이다. 현재는
 *  BreadthFirst로 진행하여 속도가 느리니 주의.
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *            |              |
 *            +-<-MAKE_INODE-+
 *
 *
 *
 *  5. InMemoryScan
 *  InsertNSort(Normal) 1.2와 동일하다.
 *
 *
 *
 *  6. MergeScan
 *  Merge와 동일하다.
 *
 *
 *
 *  7.IndexScan
 *  Copy는 없다.
 *  +-------+ +-------+
 *  | INode |-| LNode |
 *  +-------+ +-------+
 *
 * 8. InsertOnly
 *  InsertOnly 최로로 데이터가 삽입되는 상태이다. 다만 삽입하면서
 *   정렬을 하지 않는것이 InsertNSort 와 차이점.
 *
 * 1) InsertOnly( InMemoryOnly )
 *   반드시 페이지 부족이 일어나지 않는다는 보장이 있을 경우
 *  InMemoryOnly로 수행한다.
 *
 * 2) InsertOnly ( Normal )
 *  일반적인 형태로 저장된 런의 순서로 읽음.
 */

typedef enum sdtCopyPurpose
{
    SDT_COPY_NORMAL,
    SDT_COPY_SWAP,
    SDT_COPY_MAKE_KEY,
    SDT_COPY_MAKE_LNODE,
    SDT_COPY_MAKE_INODE,
    SDT_COPY_EXTRACT_ROW,
} sdtCopyPurpose;

/* make MergeRun
 * +------+------+------+------+------+
 * | Size |0 PID |0 Slot|1 PID |1 Slot|
 * +------+------+------+------+------+
 * MergeRun은 위와같은 모양의 Array로, MergeRunCount * 2 + 1 개로 구성된다.
 */
typedef struct sdtTempMergeRunInfo
{
    /* Run번호.
     * Run은 Last부터 왼쪽으로 배치되며, 하나의 Run은 MaxRowPageCount의 크기를
     * 갖기 때문에, LastWPID - MaxRowPageCnt * No 하면 Run의 WPID를 알 수
     * 있다. */
    UInt    mRunNo;
    UInt    mPIDSeq;  /*Run내 PageSequence */
    SShort  mSlotNo;  /*Page의 Slot번호 */
} sdtTempMergeRunInfo;

#define SDT_TEMP_RUNINFO_NULL ( ID_UINT_MAX )

#define SDT_TEMP_MERGEPOS_SIZEIDX     ( 0 )
#define SDT_TEMP_MERGEPOS_PIDIDX(a)   ( (a) * 2 + 1 )
#define SDT_TEMP_MERGEPOS_SLOTIDX(a)  ( (a) * 2 + 2 )

#define SDT_TEMP_MERGEPOS_SIZE(a)     ( (a) * 2 + 1 )

typedef scPageID sdtTempMergePos;


/* make ScanRun
 * +------+-----+-------+-------+------+
 * | Size | pin | 0 PID | 1 PID |
 * +------+-----+-------+-------+------+
 *  ScanRund 은  위와 같은 모양의 array로 RunCount +2  로 구성된다.
 */
#define SDT_TEMP_SCANPOS_SIZEIDX     ( 0 )
#define SDT_TEMP_SCANPOS_PINIDX      ( 1 )
#define SDT_TEMP_SCANPOS_HEADERIDX   ( 2 )

#define SDT_TEMP_SCANPOS_PIDIDX(a)   ( (a) + 2 )
#define SDT_TEMP_SCANPOS_SIZE(a)     ( (a) + 2 )

typedef scPageID sdtTempScanPos;


/****************************************************************************
 * PROJ-2201 Innovation in sorting and hashing(temp)
 ****************************************************************************/
#define SDT_WAGROUPID_MAX        (4)    /* 최대 그룹 갯수 */

/*******************************
 * Group
 *****************************/
/* Common */
/* sdtTempRow::fetch시 SDT_WAGROUPID_NONE을 사용한다는 것은,
 * BufferMiss시 해당 WAGroup에 ReadPage하여 올리지 않겠다는 것 */
#define SDT_WAGROUPID_NONE     ID_UINT_MAX
#define SDT_WAGROUPID_INIT     (0)
/* Sort */
#define SDT_WAGROUPID_SORT     (1)
#define SDT_WAGROUPID_FLUSH    (2)
#define SDT_WAGROUPID_SUBFLUSH (3)
/* Sort IndexScan */
#define SDT_WAGROUPID_INODE    (1)
#define SDT_WAGROUPID_LNODE    (2)
/* Scan*/
#define SDT_WAGROUPID_SCAN     (1)
#define SDT_WAGROUPID_MAX      (4)
#define SDT_HASH_GROUP_MAX     (3)

typedef UInt sdtGroupID;

#define SDT_WAGROUP_MIN_PAGECOUNT (2)

struct sdtWASortMapHdr;

typedef struct sdtSortGroup
{
    sdtWAReusePolicy mPolicy;

    /* WAGroup의 범위.
     * 여기서 End는 마지막 사용가능한 PageID + 1 이다. */
    scPageID         mBeginWPID;
    scPageID         mEndWPID;
    /* Fifo 또는 LRU 정책을 통한 페이지 재활용을 위한 변수.
     * 다음번에 할당할 WPID를 가진다.
     *이 값 이후의 페이지들은 한번도 재활용되지 않은 페이지이기 때문에,
     * 이 페이지를 재활용하면 된다. */
    scPageID         mReuseWPIDSeq;

    /* LRU일때 LRU List로 사용된다 */
    scPageID         mReuseWPIDTop; /* 최근에 사용한 페이지 */
    scPageID         mReuseWPIDBot; /* 사용한지 오래된 재활용 대상 */

    /* 직전에 삽입을 시도한 페이지.
     * WPID로 지칭되며, 따라서 Fix되어 고정된다. unassign되어 다른 페이지
     * 로 대체되면, 삽입하던 내용이 중간에 끊기기 때문이다. */
    sdtWCB     *     mHintWCBPtr;

    /* WAGroup에 Map이 있으면, 그것은 Group전체를 하나의 큰 페이지로 보아
     * InMemory연산을 수행한다는 의미이다. */
    sdtWASortMapHdr *mSortMapHdr;
} sdtSortGroup;

/* WAMap의 Slot은 커봤자 16Byte */
#define SDT_WAMAP_SLOT_MAX_SIZE  (16)

typedef enum
{
    SDT_WM_TYPE_RUNINFO,
    SDT_WM_TYPE_POINTER,
    SDT_WM_TYPE_MAX
} sdtWMType;

struct sdtSortSegHdr;

typedef struct sdtWMTypeDesc
{
    sdtWMType     mWMType;
    const SChar * mName;
    UInt          mSlotSize;
    smuDumpFunc   mDumpFunc;
} sdtWMTypeDesc;

typedef struct sdtWASortMapHdr
{
    sdtSortSegHdr* mWASegment;
    sdtGroupID     mWAGID;          /* Map을 가진 Group */
    scPageID       mBeginWPID;      /* WAMap이 시작되는 PID */
    UInt           mSlotCount;      /* Slot 개수 */
    UInt           mSlotSize;       /* Slot 하나의 크기 */
    sdtWMType      mWMType;         /* Map Slot의 종류(Pointer, GRID 등 )*/
    UInt           mVersionCount;   /* Slot의 Versioning 개수 */
    UInt           mVersionIdx;     /* 현재의 Version Index */
} sdtWASortMapHdr;

typedef struct sdtSortSegHdr
{
    UChar                mWorkType;
    UChar                mIsInMemory;    /*쫓겨난 Page가 있는가?*/
    scSpaceID            mSpaceID;

/* 마지막으로 접근한 WAPage. Hint 및 Dump하기 위해 사용됨 */
    /* getNPageToWCB, getWAPagePtr, getWCB 용 Hint */
    sdtWCB             * mHintWCBPtr;

    UInt                 mNPageHashBucketCnt;
    sdtWCB            ** mNPageHashPtr;
    sdtWCB            *  mUsedWCBPtr;

    sdtWASortMapHdr      mSortMapHdr; /*KeyMap, Heap, PIDList등의 Map */

    sdtWAExtentInfo      mWAExtentInfo;
    UInt                 mMaxWAExtentCount;
    sdtWAExtent        * mCurFreeWAExtent;
    UInt                 mCurrFreePageIdx;
    UInt                 mAllocWAPageCount;

    sdtNExtFstPIDList    mNExtFstPIDList;  /*NormalExtent들을 보관하는 Map*/
    UInt                 mNPageCount;     /*할당한 NPage의 개수 */

    sdtSortGroup         mGroup[ SDT_WAGROUPID_MAX ];  /*WAGroup에 대한 정보*/
    iduStackMgr          mFreeNPageStack; /*FreenPage를 재활용용 */

    idvSQL             * mStatistics;
    smiTempTableStats  * mStatsPtr;

    /* 페이지 탐색용 Hash*/
    sdtWCB               mWCBMap[1];

} sdtSortSegHdr;

#define SDT_SORT_SEGMENT_HEADER_SIZE (ID_SIZEOF(sdtSortSegHdr) - ID_SIZEOF(sdtWCB))

typedef struct sdtSortPageHdr
{
    /* 하위 13bit를 FreeOffset, 상위 3bit를 Type으로 사용함 */
    scPageID mSelfPID;
    UShort   mTypeAndFreeOffset;
    UShort   mSlotCount;
    scPageID mPrevPID;
    scPageID mNextPID;
    scOffset mSlotDir[2]; // padding으로 인한 계산 때문에 2개로 넣는다.
} sdtSortPageHdr;

#define SDT_SORT_PAGE_HEADER_SIZE (ID_SIZEOF(sdtSortPageHdr) - (ID_SIZEOF(scOffset)*2) )


/**************************************************************************
 * TempRowPiece는 다음과 같이 구성된다.
 *
 * +-------------------------------+.+---------+--------+.-------------------------+
 * + RowPieceHeader                |.|GRID HEADER       |. ColumnValues.(mtdValues)|
 * +----+-----+------+---------+---+.+---------+--------+.-------------------------+
 * |flag|dummy|ValLen|HashValue|hit|.|ChildGRID|NextGRID|.ColumnValue|...
 * +----+-----+------+---------+---+.+---------+--------+.-------------------------+
 * <----------   BASE    ----------> <------Option------>
 *
 * Base는 모든 rowPiece가 가진다.
 * NextGRID와 ChildGRID는 필요에 따라 가질 수 있다.
 * (하지만 논리적인 이유로, ChildGRID는 FirstRowPiece만 소유한다. *
 **************************************************************************/

/* TRPInfo(TempRowPieceInfo)는 Runtime구조체로 Runtime상황에서 사용되며
 * 실제 Page상에는 sdtSortTRPHdr(TempRowPiece)와 Value가 기록된다.  */
typedef struct sdtSortTRPHdr
{
    /* Row에 대해 설명해주는 TR(TempRow)Flag.
     * (HashValue사용 여부, 이전번 삽입시 성공 여부, Column쪼개짐 여부 ) */
    UChar       mTRFlag;
    UChar       mDummy;
    /* RowHeader부분을 제외한. Value부분의 길이 */
    UShort      mValueLength;

    /* hitSequence값 */
    UInt        mHitSequence;

    /******************************* Optional ****************************/
    /* IndexInternalNode나 UniqueTempHash용으로 해당 Slot의 ChildRID가
     * 저장된다. */
    scGRID         mChildGRID;
    /* 이전번에 삽입된 RowPiece의 RID이다. 그런데 삽입은 역순으로 진행되기
     * 때문에 공간적으로는 다음번 RowPiece의 RID이다.
     * 즉 FirstRowPiece 후 뒤쪽 RowPiece들이 이 NextRID로 연결되어있다. */
    scGRID         mNextGRID;
} sdtSortTRPHdr;

typedef struct sdtSortInsertInfo
{
    sdtSortTRPHdr  mTRPHeader;

    UInt            mColumnCount;  /* Column개수 */
    smiTempColumn * mColumns;      /* Column정보 */
    UInt            mValueLength;  /* Value총 길이 */
    smiValue      * mValueList;    /* Value들 */
} sdtSortInsertInfo;

typedef struct sdtSortInsertResult
{
    scGRID          mHeadRowpieceGRID; /* 머리 Rowpiece의 GRID */
    sdtSortTRPHdr * mHeadRowpiecePtr;  /* 머리 Rowpiece의 Pointer */
    UInt            mRowPageCount;     /* Row삽입하는데 사용한 page개수*/
    idBool          mComplete;         /* 삽입 성공하였는가 */
} sdtSortInsertResult;

typedef struct sdtSortScanInfo
{
    sdtSortTRPHdr * mTRPHeader;

    UInt            mValueLength;    /* Value총 길이 */
    UInt            mFetchEndOffset; /* 여기 까지 fetch */
    UChar         * mValuePtr;       /* Value의 첫 위치*/
} sdtSortScanInfo;

/* 반드시 기록되야 하는 항목들. ( sdtSortTRPHdr참조 )
 * mTRFlag, mValueLength, mHashValue, mHitSequence,
 * (1) + (1) + (2) + (4) = 8 */
#define SDT_TR_HEADER_SIZE_BASE ( 8 )

/* 추가로 옵셔널한 항목들 ( sdtSortTRPHdr참조 )
 * Base + mNextGRID + mChildGRID
 * (8) + ( 8 + 8 ) */
#define SDT_TR_HEADER_SIZE_FULL ( SDT_TR_HEADER_SIZE_BASE + 16 )

/* RID가 있으면 24 Byte */
#define SDT_TR_HEADER_SIZE(aFlag)  ( ( (aFlag) & SDT_TRFLAG_GRID )  ?                           \
                                     SDT_TR_HEADER_SIZE_FULL : SDT_TR_HEADER_SIZE_BASE )


#endif /* _O_SDT_SORT_DEF_H_ */
