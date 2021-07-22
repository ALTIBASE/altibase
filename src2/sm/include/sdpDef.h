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
 

/*******************************************************************************
 * $Id: sdpDef.h 90259 2021-03-19 01:22:22Z emlee $
 *
 * Description :
 *      본 파일은 page layer의 자료구조를 정의한 헤더파일이다.
 ******************************************************************************/

#ifndef _O_SDP_DEF_H_
#define _O_SDP_DEF_H_ 1

#include <smDef.h>
#include <sdbDef.h>
#include <sdrDef.h>

# define SDP_MIN_EXTENT_PAGE_CNT   (5)
# define SDP_MAX_SEG_PID_CNT       (16)
#if defined(SMALL_FOOTPRINT)
# define SDP_MAX_TSSEG_PID_CNT     (256)
# define SDP_MAX_UDSEG_PID_CNT     (256)
#else
# define SDP_MAX_TSSEG_PID_CNT     (512)
# define SDP_MAX_UDSEG_PID_CNT     (512)
#endif

/*
 * UDS/TSS를 위한 Free Extent Dir. List 타입 정의
 */
typedef enum sdpFreeExtDirType
{
    SDP_TSS_FREE_EXTDIR_LIST,
    SDP_UDS_FREE_EXTDIR_LIST,
    SDP_MAX_FREE_EXTDIR_LIST
} sdpFreeExtDirType;

/* PhyPage가 특정 페이지 리스트에 포함되어 있는지를 가리킨다. */
typedef enum sdpPageListState
{
    SDP_PAGE_LIST_UNLINK = 0,
    SDP_PAGE_LIST_LINK
} sdpPageListState;

typedef struct sdpExtInfo      sdpExtInfo;
typedef struct sdpSegInfo      sdpSegInfo;
typedef struct sdpSegCacheInfo sdpSegCacheInfo;

/* --------------------------------------------------------------------
 * page의 타입
 * 이 타입으로 keymap을 가졌는지 아닌지를 구분할 수 있다.
 * ----------------------------------------------------------------- */
typedef enum
{
    // Free or used
    SDP_PAGE_UNFORMAT = 0,
    SDP_PAGE_FORMAT,
    SDP_PAGE_INDEX_META_BTREE,
    SDP_PAGE_INDEX_META_RTREE,
    SDP_PAGE_INDEX_BTREE,
    SDP_PAGE_INDEX_RTREE,
    SDP_PAGE_DATA,
    SDP_PAGE_TEMP_TABLE_META,   // not used
    SDP_PAGE_TEMP_TABLE_DATA,   // not used
    SDP_PAGE_TSS,
    SDP_PAGE_UNDO,
    SDP_PAGE_LOB_DATA,
    SDP_PAGE_LOB_INDEX,

    /* Freelist Managed Segment의 페이지 타입 정의 */
    SDP_PAGE_FMS_SEGHDR,
    SDP_PAGE_FMS_EXTDIR,

    /* Treelist Managed Segment의 페이지 타입 정의 */
    SDP_PAGE_TMS_SEGHDR,
    SDP_PAGE_TMS_LFBMP,
    SDP_PAGE_TMS_ITBMP,
    SDP_PAGE_TMS_RTBMP,
    SDP_PAGE_TMS_EXTDIR,

    /* Recycled-list Managed Segment의 페이지 타입 정의 */
    SDP_PAGE_CMS_SEGHDR,
    SDP_PAGE_CMS_EXTDIR,

    SDP_PAGE_FEBT_GGHDR,   // File Super Header
    SDP_PAGE_FEBT_LGHDR,   // Extent Group Header

    /* PROJ-1704 MVCC renewal */
    SDP_PAGE_LOB_META,

    SDP_PAGE_HV_NODE,

    SDP_PAGE_TYPE_MAX           // idvTypes.h의 IDV_SM_PAGE_TYPE_MAX도 갱신해야 한다.

} sdpPageType;

/* --------------------------------------------------------------------
 * extent page bitmap의 1번 bit의 상태
 * ----------------------------------------------------------------- */
typedef enum
{
    SDP_PAGE_BIT_INSERT = 0,
    SDP_PAGE_BIT_UPDATE_ONLY = 1
} sdpPageInsertBit;

////PRJ-1497
#define SDP_PAGELIST_ENTRY_RESEV_SIZE  (2)
#define SDP_EXTDIR_PAGEHDR_RESERV_SIZE (3)

/* --------------------------------------------------------------------
 * extent의 상태를 나타낸다.
 * ----------------------------------------------------------------- */
typedef enum
{
    // 아직 tbs의 free list에 달리지 않은 상태
    SDP_EXT_NULL = 0,

    // tbs의 Free list에 달린 상태
    // segment가 최초에 extent를 얻는 상태
    // 모든 page가 free인 상태
    SDP_EXT_FREE,

    // 전부 used, 모두 insert 불가
    SDP_EXT_FULL,

    // ext의 일부 페이지가 free인 상태
    SDP_EXT_INSERTABLE_FREE,

    // 페이지가 free인 것이 없지만 insert가 가능한 것은 있다.
    // free space를 이 상태인 extent에서 찾는다.
    SDP_EXT_INSERTABLE_NO_FREE,

    // insert가능한것이 없으나 free는 있다.
    SDP_EXT_NO_INSERTABLE_FREE

} sdpExtState;

// BUG-45598: checksum 초기값
#define SDP_CHECKSUM_INIT_VAL 0

/* --------------------------------------------------------------------
 * checksum 방법을 표시한다.
 * 만약 LSN값만 사용한다면 checksum값은 page에서 아무 의미가 없다.
 * ----------------------------------------------------------------- */
typedef enum
{
    //  페이지 양끝의 LSN 값을 사용한다.
    SDP_CHECK_LSN = 0,

    // 페이지 가장 처음의 checksum 값을 계산하여 사용한다.
    SDP_CHECK_CHECKSUM

} sdpCheckSumType;

/* --------------------------------------------------------------------
 * Segement Descriptor의 type
 *
 * system 용 혹은 user 용
 * ----------------------------------------------------------------- */
typedef enum
{
    SDP_SEG_TYPE_NONE = 0,
    SDP_SEG_TYPE_SYSTEM,
    SDP_SEG_TYPE_DWB,
    SDP_SEG_TYPE_TSS,
    SDP_SEG_TYPE_UNDO,
    SDP_SEG_TYPE_INDEX,
    SDP_SEG_TYPE_TABLE,
    SDP_SEG_TYPE_LOB,           // PROJ-1362
    SDP_SEG_TYPE_MAX
} sdpSegType;

/* sdpsf, sdpst 는 Cache의 처음에 공용 Cache정보를 가진다. */
typedef struct sdpSegCCache
{
    /* Temp Table의 Head, Tail */
    scPageID    mTmpSegHead;
    scPageID    mTmpSegTail;

    /* PROJ-1704 MVCC renewal */
    sdpSegType  mSegType;        // Segment Type
    ULong       mSegSizeByBytes; // Segment Size
    UInt        mPageCntInExt;   // ExtDesc 당 Page 개수

    scPageID    mMetaPID;        // Segment Meta PID

    /* BUG-31372: It is needed that the amount of space used in
     * Segment is refered. */
    ULong       mFreeSegSizeByBytes; // Free Segment Size

    /* BUG-32091 add TableOID in PageHeader */
    smOID       mTableOID;
    /* PROJ-2281 add IndexIN in PageHeader */
    UInt        mIndexID;           /* sdpPhyPageHdr::mIndexID 가 제거되면 같이 제거해야함. */
} sdpSegCCache;

/* --------------------------------------------------------------------
 * Segement Descriptor의 type
 *
 * system 용 혹은 user 용
 * ----------------------------------------------------------------- */
typedef enum
{
    SDP_SEG_FREE = 0,
    SDP_SEG_USE
} sdpSegState;

#define SDP_PAGE_CONSISTENT     (1)
#define SDP_PAGE_INCONSISTENT   (0)
/* --------------------------------------------------------------------
 * sdRID list의 노드
 * ----------------------------------------------------------------- */
typedef struct sdpSglRIDListNode
{
    sdRID     mNext; /* 다음 rid */
} sdpSglRIDListNode;

/* --------------------------------------------------------------------
 * sdRID list의 베이스
 * ----------------------------------------------------------------- */
typedef struct sdpSglRIDListBase
{
    ULong     mNodeCnt;  /* RID List의 노드 개수 */

    sdRID     mHead;     /* Head RID */
    sdRID     mTail;     /* Tail RID */
} sdpSglRIDListBase;

/* --------------------------------------------------------------------
 * sdRID list의 노드
 * ----------------------------------------------------------------- */
typedef struct sdpDblRIDListNode
{
    sdRID     mNext; /* 다음 rid */
    sdRID     mPrev; /* 이전 rid */
} sdpDblRIDListNode;

/* --------------------------------------------------------------------
 * sdRID list의 베이스
 * ----------------------------------------------------------------- */
typedef struct sdpDblRIDListBase
{
    ULong             mNodeCnt;  /* rid list의 노드 개수 */

    sdpDblRIDListNode mBase;     /* prev 는 tail을 의미하고,
                                 next 는 head를 의미한다. */
} sdpRIDListBase;

/* --------------------------------------------------------------------
 * page list의 노드
 * ----------------------------------------------------------------- */
typedef struct sdpSglPIDListNode
{
    scPageID     mNext; /* 다음 page ID */
} sdpSglPIDListNode;

/* --------------------------------------------------------------------
 * page list의 베이스
 * ----------------------------------------------------------------- */
typedef struct sdpSglPIDListBase
{
    ULong         mNodeCnt;    /* page list의 노드 개수 */
    scPageID      mHead;       /* page list의 첫번째 */
    scPageID      mTail;       /* page list의 마지막 */
} sdpSglPIDListBase;

/* --------------------------------------------------------------------
 * page list의 노드: mNext가 mPrev 앞에 와야 한다. 왜냐하면 Sigle PID
 * List와 Double PID List가 이 링크 정보를 공유한다. Single은 무조건
 * 이 링크정보의 첫번째 member를 Next 링크정보로 사용하기때문에 mNext
 * 가 항상 먼저 와야 한다.
 *
 * 주의: mNext와 mPrev순서 바꾸지도 또는 앞에 멤버 추가도 말자.
 * ----------------------------------------------------------------- */
typedef struct sdpDblPIDListNode
{
    scPageID     mNext; /* 다음 page ID */
    scPageID     mPrev; /* 이전 page ID */
} sdpDblPIDListNode;

/* --------------------------------------------------------------------
 * page list의 베이스
 * ----------------------------------------------------------------- */
typedef struct sdpDblPIDListBase
{
    ULong             mNodeCnt;    /* page list의 노드 개수 */
    sdpDblPIDListNode mBase;       /* prev 는 tail을 의미하고,
                                      next 는 head를 의미한다. */
} sdpDblPIDListBase;


/* Treelist managed Segment에서 페이지의 상위 노드에
 * 대한 정보를 정의 */
typedef struct sdpParentInfo
{
    scPageID     mParentPID;
    SShort       mIdxInParent;
} sdpParentInfo;


/* --------------------------------------------------------------------
 * Page의 Physical Header 정의
 * ----------------------------------------------------------------- */
typedef struct sdpPhyPageHdr
{
    sdbFrameHdr         mFrameHdr;

    UShort              mTotalFreeSize;
    UShort              mAvailableFreeSize;

    UShort              mLogicalHdrSize;
    UShort              mSizeOfCTL;  /* Change Trans Layer 크기 */

    UShort              mFreeSpaceBeginOffset;
    UShort              mFreeSpaceEndOffset;

    // 이 페이지의 용도를 나타낸다. (sdpPageType)
    UShort              mPageType;

    // 페이지의 상태 ( sdpscPageBitSet : free, insert 가능, insert 불가 )
    UShort              mPageState;

    // 테이블 스페이스에서 페이지의 아이디
    scPageID            mPageID;

    // BUG-17930 : log없이 page를 생성할 때(nologging index build, DPath insert),
    // operation이 시작 후 완료되기 전까지는 page의 physical header에 있는
    // mIsConsistent를 F로 표시하고, operation이 완료된 후 mIsConsistent를
    // T로 표시하여 server failure 후 recovery될 때 redo를 수행하도록 해야 함
    UChar               mIsConsistent;

    // 페이지가 특정 리스트에 포함되어 있는 상태
    UShort              mLinkState;

    // 페이지가 속한 상위 노드의 RID
    // 이 값으로 페이지가 속한 상위노드를 알 수 있다.
    // page를 free할 때는 이 값으로 상위노드를 찾아서 free시킬 수 있다.
    sdpParentInfo       mParentInfo;  // tms

    // page list의 노드
    sdpDblPIDListNode   mListNode;

    /* BUG-32091 [sm_collection] add TableOID in PageHeader */
    smOID               mTableOID;

    /* PROJ-2281 add Index ID in PageHeader */
    UInt                mIndexID;   /* 세팅되지만, 사용하지 않는변수. 추후 다른 용도로 사용할수있도록 남겨둠. */

    // Page를 관리하는 세그먼트에서의 순서 번호
    ULong               mSeqNo;
} sdpPhyPageHdr;

/* slot directory의 시작 위치에
 * slot directory header가 존재하고
 * 그 다음에 slot entry들이 존재한다. */
typedef struct sdpSlotDirHdr
{
    /* slot entry 갯수 */
    UShort              mSlotEntryCount;
    /* unused slot entry list의 head */
    scSlotNum           mHead;
} sdpSlotDirHdr;

/* ------------------------------------------------
 * ------------ 이 설계는 다른 방법으로 대치됨 --------------
 * insertable page list 관리
 *
 * table을 위한 segment의 0번 페이지에는
 * 모든 insert 가능한 모든 페이지의 list들을 저장한다.
 * 이는 insert slot을 찾기 위한 entry로 사용된다.
 *
 * page list의  개수는 10개로 fix되어 있다.
 * 각 page list는 각 페이지의 big size를 기반으로
 * 분류된다.
 *
 * 리스트는 단일 링크드 리스트로 구성된다.
 *
 *
 * mPageList[0] = 2^5 (base)
 * mPageList[1] = 2^6 (base)
 * mPageList[2] = 2^7 (base)
 * ...
 * mPageList[10] = 2^15 (base)
 * ------------------여기까지 다른 방법으로 대치됨 --------------
 * -----------------더이상 page list로 유지되지 않는다. --------
 *
 * segment dir에 X latch를 잡으면 그 페이지의 모든 segment desc에
 * X latch를 잡은 것과 마찬가지이다.
 * 이는 contention을 유발할 수 있으므로 segment desc의 모든 내용은
 * segment의 0번째 페이지로 옮긴다.
 * 그리고 segment desc는 오직 segment 0번 페이지의 pid를 갖는다.
 * segment의 0번째 페이지를 segment header라고 한다.
 *
 * ----------------------------------------------*/

typedef struct sdpPageFooter
{
    // 마지막으로 페이지가 수정된 시점의 lsn
    // flush 시점에 write 된다.
    smLSN        mPageLSN;
} sdpPageFooter;

typedef UShort    sdpSlotEntry;

/* unused slot entry list에서
 * next ptr이 NULL이라는 것을 표시할때
 * SDP_INVALID_SLOT_ENTRY_NUM을 사용한다. */
#define SDP_INVALID_SLOT_ENTRY_NUM (0x7FFF)

#define SDP_SLOT_ENTRY_MIN_NUM     ((SShort)-1)

#define SDP_SLOT_ENTRY_MAX_NUM     \
    ( (SShort) (SD_PAGE_SIZE / ID_SIZEOF(sdpSlotEntry) ) )

/* --------------------------------------------------------------------
 * page 초기화 value
 * ----------------------------------------------------------------- */
#define SDP_INIT_PAGE_VALUE     (0x00)

/* --------------------------------------------------------------------
 * dump를 위한 flag
 * ----------------------------------------------------------------- */
# define SDP_DUMP_MASK         (0x00000007)
# define SDP_DUMP_DIR_PAGE     (0x00000001)
# define SDP_DUMP_EXT          (0x00000002)
# define SDP_DUMP_SEG          (0x00000004)

/*
 * 테이블스페이스 Online 수행시 관련 Table에 대한
 * Online Action시 넘겨주는 인자
 */
typedef struct sdpActOnlineArgs
{
    void  * mTrans; // 트랜잭션
    ULong   mSmoNo; // DRDB Index Smo No
} sdpActOnlineArgs;

struct sdpExtMgmtOp;

/*
 * PROJ-1671 Bitmap-base Tablespace And Segment Space Management
 *
 * Space Cache 정의
 * 서버 구동시와 디스크 테이블스페이스 생성시에 할당되는
 * Runtime 자료구조로 테이블스페이스의 Extent 할당 및 해제
 * 연산에 필요한 정보를 저장한다.
 *
 */
typedef struct sdpSpaceCacheCommon
{
    /* Tablespace의 ID */
    scSpaceID          mSpaceID;

    /* Extent 관리방식 */
    smiExtMgmtType     mExtMgmtType;

    /* Segment 관리방식 */
    smiSegMgmtType     mSegMgmtType;

    /* Extent의 페이지 개수 */
    UInt               mPagesPerExt;

} sdpSpaceCacheCommon;

/*
 * PROJ-1671 Bitmap-based TableSpace And Segment Space Management
 *
 * Segment 공간관리에 필요한 정보와 Runtime 정보를
 * 관리하는 자료구조
 *
 * Segment 공간관리 방식에 따라 Segment Handle의
 * Runtime Cache를 초기화한다.
 */
typedef struct sdpSegHandle
{
    scSpaceID            mSpaceID;
    /* 테이블 생성시 할당되는 segment desc. 해당 테이블에서 record를
       저장하기 위한 모든 page를 관리한다. */
    scPageID             mSegPID;

    // XXX Meta Offset을 SegDesc로 부터 Offset인지. 아님 페이지 시작위치
    // 부터 Offset인지 결정해야함. 현재 SegDesc의 시작 위치로 부터 Offset임.

    /* Segment의 Storage 속성 */
    smiSegStorageAttr    mSegStoAttr;
    /* Segment 속성 */
    smiSegAttr           mSegAttr;

    /*
     * runtime시 구성하는 temporary 정보
     * 아래의 내용들은 모두 disk에 존재하며, runtime시에
     * 디스크 페이지로부터 읽어서 설정한다.
     */

    /* Segment 공간 관리에 필요한 Runtime 정보 */
    void                * mCache;
} sdpSegHandle;


/*
 * Segment Cache내의 Hint Position 정보
 */
typedef struct sdpHintPosInfo
{
    scPageID      mSPosVtPID;
    SShort        mSRtBMPIdx;
    scPageID      mSPosRtPID;
    SShort        mSItBMPIdx;
    scPageID      mSPosItPID;
    SShort        mSLfBMPIdx;
    UInt          mSRsFlag;
    UInt          mSStFlag;
    scPageID      mPPosVtPID;
    SShort        mPRtBMPIdx;
    scPageID      mPPosRtPID;
    SShort        mPItBMPIdx;
    scPageID      mPPosItPID;
    SShort        mPLfBMPIdx;
    UInt          mPRsFlag;
    UInt          mPStFlag;
} sdpHintPosInfo;


/* PROJ-1705 */
typedef UShort (*sdpGetSlotSizeFunc)( const UChar    *aSlotPtr );


/*
 * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
 *
 * 테이블스이스의 공간관리 인터페이스들의 프로토타입 정의
 */

/* 테이블스페이스 모듈 초기화 */
typedef IDE_RC (*sdptInitializeFunc)( scSpaceID          aSpaceID,
                                      smiExtMgmtType     aExtMgmtType,
                                      smiSegMgmtType     aSegMgmtType,
                                      UInt               aExtPageCount );

/* Free Extent */
typedef IDE_RC (*sdptFreeExtentFunc) ( idvSQL        * aStatistics,
                                       sdrMtx        * aMtx,
                                       scSpaceID       aSpaceID,
                                       scPageID        aExtFstPID,
                                       UInt        *   aNrDone );

/* 데이타파일 Autoextend 모드 변경 */
typedef IDE_RC (*sdptAlterFileAutoExtendFunc)(
                           idvSQL     *aStatistics,
                           void*       aTrans,
                           scSpaceID   aSpaceID,
                           SChar      *aFileName,
                           idBool      aAutoExtend,
                           ULong       aNextSize,
                           ULong       aMaxSize,
                           SChar      *aValidDataFileName );

/* 데이타파일 Rename  */
typedef IDE_RC (*sdptAlterFileNameFunc)(
                           idvSQL*      aStatistics,
                           scSpaceID    aSpaceID,
                           SChar       *aOldName,
                           SChar       *aNewName );

/* 데이타파일 Resize 모드 변경 */
typedef  IDE_RC (*sdptAlterFileResizeFunc)(
                          idvSQL       *aStatistics,
                          void         *aTrans,
                          scSpaceID     aSpaceID,
                          SChar        *aFileName,
                          ULong         aSizeWanted,
                          ULong        *aSizeChanged,
                          SChar        *aValidDataFileName );

/*
 * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
 *
 * Segment 공간관리 인터페이스들의 프로토타입 정의
 */

/* Segment 공간관리 모듈 초기화 */
typedef IDE_RC (*sdpsInitializeFunc)( sdpSegHandle * aSegHandle,
                                      scSpaceID      aSpaceID,
                                      sdpSegType     aSegType,
                                      smOID          aObjectID,
                                      UInt           aIndexID );

/* Segment 공간관리 모듈 해제 */
typedef IDE_RC (*sdpsDestroyFunc)( sdpSegHandle * aSegHandle );

/* Table Segment 할당 및 Segment Meta Header 초기화 */
typedef IDE_RC (*sdpsCreateSegmentFunc)(
                       idvSQL                * aStatistics,
                       sdrMtx                * aMtx,
                       scSpaceID               aSpaceID,
                       sdpSegType              aSegType,
                       sdpSegHandle          * aSegHandle );

/* Segment 해제 */
typedef IDE_RC (*sdpsDropSegmentFunc)( idvSQL             * aStatistics,
                                       sdrMtx             * aMtx,
                                       scSpaceID            aSpaceID,
                                       scPageID             aSegPID );

/* Segment 리셋 */
typedef IDE_RC (*sdpsResetSegmentFunc)( idvSQL           * aStatistics,
                                        sdrMtx           * aMtx,
                                        scSpaceID          aSpaceID,
                                        sdpSegHandle     * aSegHandle,
                                        sdpSegType         aSegType );

typedef IDE_RC (*sdpsExtendSegmentFunc)( idvSQL           * aStatistics,
                                         sdrMtxStartInfo  * aStartInfo,
                                         scSpaceID          aSpaceID,
                                         sdpSegHandle     * aSegHandle,
                                         UInt               aExtCount );


typedef IDE_RC (*sdpsSetMetaPIDFunc)( idvSQL           * aStatistics,
                                      sdrMtx           * aMtx,
                                      scSpaceID          aSpaceID,
                                      scPageID           aSegPID,
                                      UInt               aIndex,
                                      scPageID           aMetaPID );

typedef IDE_RC (*sdpsGetMetaPIDFunc)( idvSQL           * aStatistics,
                                      scSpaceID          aSpaceID,
                                      scPageID           aSegPID,
                                      UInt               aIndex,
                                      scPageID         * aMetaPID );

/* 새로운 페이지 할당 */
typedef IDE_RC (*sdpsAllocNewPageFunc)( idvSQL           * aStatistics,
                                        sdrMtx           * aMtx,
                                        scSpaceID          aSpaceID,
                                        sdpSegHandle     * aSegHandle,
                                        sdpPageType        aPageType,
                                        UChar           ** aAllocPagePtr );

/* 가용공간 할당이후 페이지 가용도 및 Segment 가용도 변경 */
typedef IDE_RC (*sdpsUpdatePageState)( idvSQL           * aStatistics,
                                       sdrMtx           * aMtx,
                                       scSpaceID          aSpaceID,
                                       sdpSegHandle     * aSegHandle,
                                       UChar            * aDataPagePtr );


/* 요청된 개수만큼 Free 페이지를 Segment에 확보 */
typedef IDE_RC (*sdpsPrepareNewPagesFunc)( idvSQL            * aStatistics,
                                           sdrMtx            * aMtxForSMO,
                                           scSpaceID           aSpaceID,
                                           sdpSegHandle      * aSegHandle,
                                           UInt                aCountWanted );


/* DPath-Insert와 TSS/UDS 페이지 할당 연산을 위한 페이지 할당 */
typedef IDE_RC (*sdpsAllocNewPage4AppendFunc) ( idvSQL               *aStatistics,
                                                sdrMtx               *aMtx,
                                                scSpaceID             aSpaceID,
                                                sdpSegHandle         *aSegHandle,
                                                sdRID                 aPrvAllocExtRID,
                                                scPageID              aFstPIDOfPrvAllocExt,
                                                scPageID              aPrvAllocPageID,
                                                idBool                aIsLogging,
                                                sdpPageType           aPageType,
                                                sdRID                *aAllocExtRID,
                                                scPageID             *aFstPIDOfAllocExt,
                                                scPageID             *aAllocPID,
                                                UChar               **aAllocPagePtr );


/* DPath-Insert와 TSS/UDS를 위한 페이지 확보 */
typedef IDE_RC (*sdpsPrepareNewPage4AppendFunc)( idvSQL               *aStatistics,
                                                 sdrMtx               *aMtx,
                                                 scSpaceID             aSpaceID,
                                                 sdpSegHandle         *aSegHandle,
                                                 sdRID                 aPrvAllocExtRID,
                                                 scPageID              aFstPIDOfPrvAllocExt,
                                                 scPageID              aPrvAllocPageID );

/* 페이지 Free*/
typedef IDE_RC (*sdpsFreePageFunc)( idvSQL            * aStatistics,
                                    sdrMtx            * aMtx,
                                    scSpaceID           aSpaceID,
                                    sdpSegHandle      * aSegHandle,
                                    UChar             * aPagePtr );

typedef idBool (*sdpsIsFreePageFunc)( UChar * aPagePtr );

/* 가용공간 할당을 위한 탐색 */
typedef IDE_RC (*sdpsFindInsertablePageFunc)( idvSQL           * aStatistics,
                                              sdrMtx           * aMtx,
                                              scSpaceID          aSpaceID,
                                              sdpSegHandle     * aSegHandle,
                                              void             * aTableInfo,
                                              sdpPageType        aPageType,
                                              UInt               aRecordSize,
                                              idBool             aNeedKeySlot,
                                              UChar           ** aPagePtr,
                                              UChar            * aCTSlotIdx );

/* Sequential Scan할 Segment 정보를 반환한다 */
typedef IDE_RC (*sdpsGetFmtPageCntFunc)( idvSQL       *aStatistics,
                                         scSpaceID     aSpaceID,
                                         sdpSegHandle *aSegHandle,
                                         ULong        *aFmtPageCnt );

/* Sequential Scan할 Segment 정보를 반환한다 */
typedef IDE_RC (*sdpsGetSegInfoFunc)( idvSQL       *aStatistics,
                                      scSpaceID     aSpaceID,
                                      scPageID      aSegPID,
                                      void         *aTableHeader,
                                      sdpSegInfo   *aSegInfo );

/* Sequential Scan할 Segment 정보를 반환한다 */
typedef IDE_RC (*sdpsGetSegCacheInfoFunc)( idvSQL            *aStatistics,
                                           sdpSegHandle      *aSegHandle,
                                           sdpSegCacheInfo   *aSegCacheInfo );

/* Sequential Scan할 Extent 정보를 반환한다 */
typedef IDE_RC (*sdpsGetExtInfoFunc)( idvSQL       *aStatistics,
                                      scSpaceID     aSpaceID,
                                      sdRID         aExtRID,
                                      sdpExtInfo   *aExtInfo );

/* 다음 Extent의 정보를 반환한다 */
typedef IDE_RC (*sdpsGetNxtExtInfoFunc)( idvSQL       *aStatistics,
                                         scSpaceID     aSpaceID,
                                         scPageID      aSegHdrPID,
                                         sdRID         aCurExtRID,
                                         sdRID        *aNxtExtRID );

/* Sequential Scan할 다음 페이지를 반환한다 */
typedef IDE_RC (*sdpsGetNxtAllocPageFunc)( idvSQL           * aStatistics,
                                           scSpaceID          aSpaceID,
                                           sdpSegInfo       * aSegInfo,
                                           sdpSegCacheInfo  * aSegCacheInfo,
                                           sdRID            * aExtRID,
                                           sdpExtInfo       * aExtInfo,
                                           scPageID         * aPageID );

/* Full Scan 완료후 Last Alloc Page를 갱신한다. */
typedef IDE_RC (*sdpsSetLstAllocPageFunc)( idvSQL       *aStatistics,
                                           sdpSegHandle *aSegHandle,
                                           scPageID      aLstAllocPID,
                                           ULong         aLstAllocSeqNo );

/* segment merge완료과정에서 HWM 갱신한다. */
typedef IDE_RC (*sdpsUpdateHWMInfo4DPath)( idvSQL           *aStatistics,
                                           sdrMtxStartInfo  *aStartInfo,
                                           scSpaceID         aSpaceID,
                                           sdpSegHandle     *aSegHandle,
                                           scPageID          aPrvLstAllocPID,
                                           sdRID             aLstAllocExtRID,
                                           scPageID          aFstPIDOfLstAllocExt,
                                           scPageID          aLstAllocIPID,
                                           ULong             aAllocPageCnt,
                                           idBool            aMegeMultiSeg );

/* segment merge완료과정에서 Dpath시 format된 페이지를 다시 포맷한다. */
typedef IDE_RC (*sdpsReformatPage4DPath)( idvSQL           *aStatistics,
                                          sdrMtxStartInfo  *aStartInfo,
                                          scSpaceID         aSpaceID,
                                          sdpSegHandle     *aSegHandle,
                                          sdRID             aLstAllocExtRID,
                                          scPageID          aLstPID );

/* Segment의 상태를 리턴한다. */
typedef IDE_RC (*sdpsGetSegStateFunc) ( idvSQL        *aStatistics,
                                        scSpaceID      aSpaceID,
                                        scPageID       aSegPID,
                                        sdpSegState   *aSegState );

/* Segment Cache에 저장된 탐색Hint정보를 반환한다. */
typedef void (*sdpsGetHintPosInfoFunc) ( idvSQL          * aStatistics,
                                         void            * aSegCache,
                                         sdpHintPosInfo  * aHintPosInfo );

/* Segment 자료구조에 대한 Verify */
typedef IDE_RC (*sdpsVerifyFunc)( idvSQL     * aStatistics,
                                  scSpaceID    aSpaceID,
                                  void       * aSegDesc,
                                  UInt         aFlag,
                                  idBool       aAllUsed,
                                  scPageID     aUsedLimit );

/* Segment 자료구조를 출력 */
typedef IDE_RC (*sdpsDumpFunc) ( scSpaceID    aSpaceID,
                                 void        *aSegDesc,
                                 idBool       aDisplayAll );


typedef IDE_RC (*sdpsMarkSCN4ReCycleFunc)
                                  ( idvSQL        * aStatistics,
                                    scSpaceID       aSpaceID,
                                    scPageID        aSegPID,
                                    sdpSegHandle  * aSegHandle,
                                    sdRID           aFstExtRID,
                                    sdRID           aLstExtRID,
                                    smSCN         * aCommitSCN );

typedef IDE_RC (*sdpsSetSCNAtAllocFunc)
                                  ( idvSQL        * aStatistics,
                                    scSpaceID       aSpaceID,
                                    sdRID           aExtRID,
                                    smSCN         * aTransBSCN );

typedef IDE_RC (*sdpsTryStealExtsFunc) ( idvSQL           * aStatistics,
                                         sdrMtxStartInfo  * aStartInfo,
                                         scSpaceID          aSpaceID,
                                         sdpSegHandle     * aFrSegHandle,
                                         scPageID           aFrSegPID,
                                         scPageID           aFrCurExtDir,
                                         sdpSegHandle     * aToSegHandle,
                                         scPageID           aToSegPID,
                                         scPageID           aToCurExtDir,
                                         idBool           * aTrySuccess );
/* BUG-31055 Can not reuse undo pages immediately after it is used to 
 *           aborted transaction */
typedef IDE_RC (*sdpsShrinkExtsFunc) ( idvSQL            * aStatistics,
                                       scSpaceID           aSpaceID,
                                       scPageID            aSegPID,
                                       sdpSegHandle      * aSegHandle,
                                       sdrMtxStartInfo   * aStartInfo,
                                       sdpFreeExtDirType   aFreeListIdx,
                                       sdRID               aFstExtRID,
                                       sdRID               aLstExtRID);

/*
 * Segment 공간관리 방식에 따른 모듈들의 인터페이스
 * 집합을 정의한다.
 */
typedef struct sdpSegMgmtOp
{
    /* 모듈 초기화 및 해제 */
    sdpsInitializeFunc                mInitialize;
    sdpsDestroyFunc                   mDestroy;

    /* Segment 생성 및 해제 */
    sdpsCreateSegmentFunc             mCreateSegment;
    sdpsDropSegmentFunc               mDropSegment;
    sdpsResetSegmentFunc              mResetSegment;
    sdpsExtendSegmentFunc             mExtendSegment;

    /* 페이지 할당 및 해제 */
    sdpsAllocNewPageFunc              mAllocNewPage;
    sdpsPrepareNewPagesFunc           mPrepareNewPages;
    sdpsAllocNewPage4AppendFunc       mAllocNewPage4Append;
    sdpsPrepareNewPage4AppendFunc     mPrepareNewPage4Append;
    sdpsUpdatePageState               mUpdatePageState;
    sdpsFreePageFunc                  mFreePage;

    /* Page State */
    sdpsIsFreePageFunc                mIsFreePage;

    /* DPath가 Commit시 Temp Seg의 페이지를 Target Seg에
     * Add한다. */
    sdpsUpdateHWMInfo4DPath           mUpdateHWMInfo4DPath;
    sdpsReformatPage4DPath            mReformatPage4DPath;

    /* 가용공간 페이지 탐색 */
    sdpsFindInsertablePageFunc        mFindInsertablePage;

    /* Format Page Count */
    sdpsGetFmtPageCntFunc             mGetFmtPageCnt;
    /* Sequential Scan */
    sdpsGetSegInfoFunc                mGetSegInfo;
    sdpsGetExtInfoFunc                mGetExtInfo;
    sdpsGetNxtExtInfoFunc             mGetNxtExtInfo;
    sdpsGetNxtAllocPageFunc           mGetNxtAllocPage;
    sdpsGetSegCacheInfoFunc           mGetSegCacheInfo;
    sdpsSetLstAllocPageFunc           mSetLstAllocPage;

    /* Segment의 상태 */
    sdpsGetSegStateFunc               mGetSegState;
    sdpsGetHintPosInfoFunc            mGetHintPosInfo;

    /* Segment Meta PID관리 */
    sdpsSetMetaPIDFunc                mSetMetaPID;
    sdpsGetMetaPIDFunc                mGetMetaPID;

    sdpsMarkSCN4ReCycleFunc           mMarkSCN4ReCycle;
    sdpsSetSCNAtAllocFunc             mSetSCNAtAlloc;
    sdpsTryStealExtsFunc              mTryStealExts;
    sdpsShrinkExtsFunc                mShrinkExts;

    /* Segment 자료구조의 Verify 및 Dump */
    sdpsDumpFunc                      mDump;
    sdpsVerifyFunc                    mVerify;

} sdpSegMgmtOp;

/* PROJ-1671 Segment의 공간관리 방식
 * Segment 공간관리에 필요한 정보를 저장한다. */
typedef struct sdpSegmentDesc
{
    /* PROJ-1671 Bitmap-based Tablespace And Segment Space Management */
    sdpSegHandle     mSegHandle;
    /* Segment Space Management Type */
    smiSegMgmtType   mSegMgmtType;
    /* Segment 연산의 인터페이스 모듈 */
    sdpSegMgmtOp   * mSegMgmtOp;

} sdpSegmentDesc;

/* ------------------------------------------------
 * disk table을 위한 page list entry
 * - memory table의 page list entry와는 별도로
 *   정의되어 있음.
 * ----------------------------------------------*/
typedef struct sdpPageListEntry
{
    /* mItemSize의 align된 길이 fixed 영역의 크기가 page의
     * 데이타 가용공간크기보다 큰 테이블은 생성할 수 없다 */
    UInt                  mSlotSize;

    /* Segment 기술자 */
    sdpSegmentDesc        mSegDesc;

    /* insert된 record의 개수 */
    ULong                 mRecCnt;

    /* 다수의 transaction이 동시에 page list entry에
     * 접근할 수 있는 동시성 제공 */
    iduMutex              *mMutex;

    /* PROJ-1497 DB Migration을 방지하기 위해 Reserved 영역을 추가 */
    ULong                  mReserveArea[SDP_PAGELIST_ENTRY_RESEV_SIZE];
} sdpPageListEntry;

#define SDP_SEG_SPECIFIC_DATA_SIZE  (64)

typedef struct sdpSegInfo
{
    /* Common */
    scPageID          mSegHdrPID;
    sdpSegType        mType;
    sdpSegState       mState;

    /* Extent의 페이지 갯수 */
    UInt              mPageCntInExt;

    /* Format된 페이지 갯수 */
    ULong             mFmtPageCnt;

    /* Extent갯수 */
    ULong             mExtCnt;

    /* Extent Dir Page 개수 */
    ULong              mExtDirCnt;

    /* 첫번째 Extent RID */
    sdRID             mFstExtRID;

    /* 마지막 Extent RID */
    sdRID             mLstExtRID;

    /* High Water Mark */
    scPageID          mHWMPID;

    /* for FMS */
    sdRID             mLstAllocExtRID;
    scPageID          mFstPIDOfLstAllocExt;

    /* HWM의 Extent RID */
    sdRID             mExtRIDHWM;

    /* For TempSegInfo */
    ULong             mSpecData4Type[ SDP_SEG_SPECIFIC_DATA_SIZE / ID_SIZEOF( ULong ) ];
} sdpSegInfo;

typedef struct sdpSegCacheInfo
{
    /* 필요한 정보를 추가하면 된다.
     * 현재는 TMS에서 마지막 할당된 페이지를 가져오기 위해서만 사용한다. */

    /* for TMS */
    idBool        mUseLstAllocPageHint; // 마지막 할당된 Page Hint 사용 여부
    scPageID      mLstAllocPID;         // 마지막 할당된 Page ID
    ULong         mLstAllocSeqNo;       // 마지막 할당된 Page의 SeqNo
} sdpSegCacheInfo;

typedef struct sdpExtInfo
{
    /* Extent의 첫번째 Extent PID */
    scPageID     mFstPID;

    /* Extent내의 첫번째 Data PID */
    scPageID     mFstDataPID;

    /* Extent를 관리하는 LF BMP PID */
    scPageID     mExtMgmtLfBMP;

    /* Extent의 마지막 PID */
    scPageID     mLstPID;

} sdpExtInfo;

typedef struct sdpTBSInfo
{
    /* High High Water Mark */
    scPageID           mHWM;
    /* Low  High Water Mark */
    scPageID           mLHWM;

    /* Total Ext Count */
    ULong              mTotExtCnt;

    /* Format Ext Count */
    ULong              mFmtExtCnt;

    /* Page Count In Ext */
    UInt               mPageCntInExt;

    /* 확장시 생성되는 Extent의 갯수 */
    UInt               mExtCntOfExpand;

    /* Free Extent RID List */
    sdRID              mFstFreeExtRID;
} sdpTBSInfo;

typedef struct sdpDumpTBSInfo
{
    /* TBS의 속한 Extent RID */
    sdRID              mExtRID;

    /* Extent RID값의 PageID, mOffset */
    scPageID           mPID;
    scOffset           mOffset;

    /* Extent의 첫번째 PID */
    scPageID           mFstPID;
    /* Extent의 첫번째 Data PID */
    scPageID           mFstDataPID;
    /* Extent의 페이지 갯수 */
    UInt               mPageCntInExt;
} sdpDumpTBSInfo;


/*
 * Extent Desc 정의
 */
typedef struct sdpExtDesc
{
    scPageID   mExtFstPID;  // Extent의 첫번째 PageID
    UInt       mLength;     // Extent의 페이지 개수
} sdpExtDesc;

# define SDP_1BYTE_ALIGN_SIZE   (1)
# define SDP_8BYTE_ALIGN_SIZE   (8)

/**********************************************************************
 * SelfAging Check Flag
 * 데이타 페이지에 대해서 SelfAging의 수행하기 이전에 앞서 확인단계가 있는데,
 * 확인과정에서 반환하는 flag 값이다.
 *
 * (1) CANNOT_AGING
 *     AGING 대상 Row Piece가 존재하지만, 아직 볼수 있는 트랜잭션이 Active하기
 *     때문에 Aging을 할 수 없는 경우이다. AGING가능한 SCN이 기록되어 있기 때문에
 *     바로 판단할 수 있다.
 * (2) NOTHING_TO_AGING
 *     AGING 대상 Row Piece가 존재하지 않는다.
 * (3) CHECK_AND_AGING
 *     AGING 대상이 존재하지만, AGING 가능한 SCN을 FullScan 해보고 구해야하므로
 *     바로 판단할 수 없다. 최초 한번은 FullScan 한다.
 * (4) AGING_DEAD_ROWS
 *     Row Piece를 볼수 있는 다른 트랜잭션이 없기 때문에 AGING 할 수 있는
 *     Row Piece가 존재한다. 바로 판단한다.
 **********************************************************************/
typedef enum sdpSelfAgingFlag
{
    SDP_SA_FLAG_CANNOT_AGING     = 0,
    SDP_SA_FLAG_NOTHING_TO_AGING,
    SDP_SA_FLAG_CHECK_AND_AGING,
    SDP_SA_FLAG_AGING_DEAD_ROWS
} sdpSelfAgingFlag;


/**********************************************************************
 *
 * PROJ-1705 Disk MVCC 리뉴얼
 *
 * 디스크 테이블의 데이타 페이지의 Touched Transaction Layer 헤더 정의
 *
 **********************************************************************/
typedef struct sdpCTL
{
    smSCN     mSCN4Aging;    // Self-Aging할 수 있는 기준 SCN
    UShort    mCandAgingCnt; // Self-Aging할 수 있는 Dead Deleted Row
    UShort    mDelRowCnt;    // 총 Deleted Or Deleting 인 RowPiece 개수
    UShort    mTotCTSCnt;    // 총 CTS 개수
    UShort    mBindCTSCnt;   // 총 Bind된 CTS 개수
    UShort    mMaxCTSCnt;    // 최대 CTS 개수
    UShort    mRowBindCTSCnt;
    UChar     mAlign[4];     // Dummy Align
} sdpCTL;


# define SDP_CACHE_SLOT_CNT    (2)


/**********************************************************************
 * 데이타 페이지의 Touched Transaction Slot 정의
 *
 * 동일한 트랜잭션이 갱신한 Row들은 할당받은 CTS를 통해서 연결관계를 맺게되며,
 * Commit 이후 다른 트랜잭션이 수행한 Row Time-Stamping에 의해서 연결관계는
 * 정리된다.
 *
 *   __________________________________________________________________
 *   |TSSlotSID.mPageID |TSSlotSID.mSlotNum| FSCredit | Stat |Align 1B |
 *   |__________________|__________________|__________|______|_________|
 *      | RefCount |  RefRowSlotNum[0] | RefRowSlotNum[1] |
 *      |__________|__________________|_______________|
 *      |Trans Begin SCN or CommitSCN |
 *      |_____________________________|
 *
 *      Touched Transaction Slot의 Size는 24바이트이며,
 *      8Byte에 align되어 있다.
 *
 * (1) TSSlotSID
 *     갱신 트랜잭션마다 할당되는 TSS의 SID이다. TSS 페이지도 재사용될 수 있으며,
 *     재사용이후에도 트랜잭션들이 TSSlotSID를 따라서 TSS에 방문할 수 있다.
 *
 *     만약 Fast CTS Stamping이 실패했을때 이후 트랜잭션에 의해서 Delayed CTS
 *     Stamping 혹은 Hard Row Time-Stamping시에 TSSlotSID를 따라서 재방문하게
 *     되는데 이때 재사용되었다고 판단되면 정확한 CommitSCN을 구할 수가 없지만,
 *     대신에 INITSCN(0x0000000000000000)을 설정하므로 해서 이후 트랜잭션이
 *     모두 갱신하거나 볼수 있게 한다. 이것이 가능한 이유는 TSS 페이지가 재사용
 *     되는 기준이 되는 시점에 있다. 즉, 그 TSS와 관련한 모든 Old Row Version을
 *     판독할 트랜잭션이 존재하지 않는 경우 TSS를 재사용할 수 있기 때문이다.
 *
 * (2) FSC ( Free Space Credit )
 *     트랜잭션이 Row를 Update하거나 Delete연산시 Rollback 할 수 있는 가용공간을
 *     Commit할때까지 다른 트랜잭션이 할당하지 못하게 해야한다. 이처럼 Commit 이후
 *     에 해제되는 공간의 크기를 FSC라고 하고 이것을 CTS에 기록해둔다.
 *
 * (3) FLAG
 *     CTS를 할당받은 트랜잭션의 Active/Rollback/Time-Stamping 여부를
 *     판단할 수 있는 상태값이다.
 *
 *     다음과 같은 4가지 상태값을 가진다.
 *
 *     - 'N' (0x01)
 *     - 'A' (0x02)
 *       CTS를 할당한 트랜잭션이 아직 Active인 경우 혹은 이미 Commit 되었지만,
 *       Time-Stamping이 필요한 경우이다.
 *     - 'T' (0x04)
 *       할당한 트랜잭션이 Commit연산이나 SCAN 연산시 CTS Time-Stamping된 CTS
 *       이며, 정확한  CommitSCN이 설정된다. 이후 Row Time-Stamping이 필요하다.
 *     - 'R' (0x08)
 *       Row Time-Stamping이 완료되었음을 의미한다. CommitSCN을 정확한 SCN 값
 *       이거나 INITSCN(0x0000000000000000)이 설정된다.
 *     - 'O' (0x10)
 *       관련 Row가 모두 Rollback된 상태이다.
 *
 * (4) RefCount
 *     CTS와 관련된 Row의 개수이다. 주의점은 중복 갱신된 Row에 대해서는 1만 증가
 *     한다.
 *
 * (5) RefRowSlotNum
 *     CTS와 관련된 Row Piece Header의 Cache 정보를 최대 2개까지 Cache한다.
 *     왜냐하면, 한페이지 내에서 2개이하로 갱신될 경우에는 Slot Dir. 를 FullScan하지
 *     않도록 하기 위한 것이다.
 *
 * (6) 트랜잭션 FstDskViewSCN 혹은 Commit SCN
 *     갱신 트랜잭션의 FstDskViewSCN을 설정해 놓고, Commit시에는 CTS
 *     Time-Stamping을 CommitSCN으로 설정하게 된다.
 *     FstDskViewSCN은 만약 CTS Page가 재사용된 경우를 체크하기 위해서이다.
 *
 **********************************************************************/
typedef struct sdpCTS
{
    scPageID    mTSSPageID;          // TSS 의 PID
    scSlotNum   mTSSlotNum;          // TSS 의 SlotNum
    UShort      mFSCredit;           // 페이지에 반환하지 않은 가용공간크기
    UChar       mStat;               // CTS의 상태
    UChar       mAlign;              // Align Dummy 바이트
    UShort      mRefCnt;             // Cache된 Row Piece의 개수
    scSlotNum   mRefRowSlotNum[ SDP_CACHE_SLOT_CNT ];
                                     // Cache된 Row Piece의 Offset
    smSCN       mFSCNOrCSCN;         // CTS를 할당한 트랜잭션 BSCN 혹은 CSCN
} sdpCTS;

# define SDP_CTS_STAT_NUL    (0x01)  // 'N' CTS가 한번도 바인딩된적 없는 상태
# define SDP_CTS_STAT_ACT    (0x02)  // 'A' CTS가 트랜잭션에 바인딩된 상태
# define SDP_CTS_STAT_CTS    (0x08)  // 'T' CTS TimeStamping이 된 상태
# define SDP_CTS_STAT_RTS    (0x10)  // 'R' Row TimeStamping이 된 상태
# define SDP_CTS_STAT_ROL    (0x20)  // 'O' Rollback된 상태

# define SDP_CTS_SS_FREE     ( SDP_CTS_STAT_NUL | \
                               SDP_CTS_STAT_RTS | \
                               SDP_CTS_STAT_ROL )

/**********************************************************************
 * Table Change Transaction Slot Idx 정의
 * Page Layer에서 다음 값들을 참조함.
 **********************************************************************/

# define SDP_CTS_MAX_IDX     (0x78)  // 0 ~ 120 01111000
# define SDP_CTS_MAX_CNT     (SDP_CTS_MAX_IDX + 1)

/* CTS를 할당받지 못하고 BoundRow한 경우 */
# define SDP_CTS_IDX_NULL    (0x7C)  // 124     01111100

/* Stamping 이후에 아무것도 가리키지 않은 경우 */
# define SDP_CTS_IDX_UNLK    (0x7E)  // 126     01111110
# define SDP_CTS_IDX_MASK    (0x7F)  // 127     01111111
# define SDP_CTS_LOCK_BIT    (0x80)  // 128     10000000

// PROJ-2068 Direct-Path INSERT 성능 개선
typedef struct sdpDPathSegInfo
{
    // Linked List의 Next Pointer
    smuList     mNode;

    // Undo시 DPathSegInfo를 식별하기 위한 번호
    UInt        mSeqNo;

    // Insert 대상 Table의 SpaceID
    scSpaceID   mSpaceID;
    // insert 대상 table 또는 partition의 TableOID
    smOID       mTableOID;

    // BUG-29032 - DPath Insert merge 시 TMS에서 assert
    // 처음 할당했던 PageID
    scPageID    mFstAllocPID;

    // 마지막 할당했던 ExtRID, PageID
    scPageID    mLstAllocPID;
    sdRID       mLstAllocExtRID;
    scPageID    mFstPIDOfLstAllocExt;

    // 마지막 page를 가리키는 포인터
    UChar     * mLstAllocPagePtr;
    // Table, External, Multiple page까지 합한 Page 개수
    UInt        mTotalPageCount;

    // Record Count
    UInt        mRecCount;

    // PROJ-1671 Bitmap-based Tablespace And Segment Space Management
    // Segment 공간관리에 필요한 정보 및 연산 정의
    // Segment Handle : Segment RID 및 Semgnet Cache
    sdpSegmentDesc    * mSegDesc;

    // 마지막으로 Insert된 SegInfo 일때 TRUE
    idBool          mIsLastSeg;
} sdpDPathSegInfo;

typedef struct sdpDPathInfo
{
    // List Of Segment Info for DPath Insert
    smuList mSegInfoList;

    //  sdpDPathSegInfo의 mSeqNo에 할당하기 위한 다음 SeqNo
    //
    //  SeqNo는 Direct-Path INSERT의 rollback에 사용되는 값이다.
    // Direct-Path INSERT의 undo 시에 sdpDPathSegInfo에 달린 mSeqNo를 보고
    // NTA 로그에 기록된 SeqNo와 동일한 sdpDPathSegInfo를 파괴하는 방식으로
    // undo를 수행한다.
    UInt    mNxtSeqNo;

    // X$DIRECT_PATH_INSERT 통계를 위한 값
    ULong   mInsRowCnt;
} sdpDPathInfo;


#endif // _O_SDP_DEF_H_
