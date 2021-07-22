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
 * $Id$
 *
 * 본 파일은 Treelist Managed Segment 모듈의 공통 자료구조를 정의한
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_DEF_H_
# define _O_SDPST_DEF_H_ 1

# include <iduMutex.h>
# include <sdpDef.h>

/*
 * BMP Type & Stack Depth
 * BMP 들의 type과 stack depth를 정의한다.
 *
 * EMPTY, VIRTBMP 타입의 BMP 는 존재하지 않는다. 이들은 단지 sdpstStackMgr
 * 과 공유하기 위해 존재하는 타입이다.
 *
 * 또한 sdpstBfrAllocExtInfo, sdpstAftAllocExtInfo 에서 사용시
 * VIRTBMP는 EXTDIR로 사용된다.
 *
 * Treelist의 Depth는 4이다.
 */
typedef enum sdpstBMPType
{
    SDPST_EMPTY   = -1,
    SDPST_VIRTBMP = 0,
    SDPST_RTBMP,
    SDPST_ITBMP,
    SDPST_LFBMP,
    SDPST_BMP_TYPE_MAX
} sdpstBMPType;

/* sdpstBfrAllocExtInfo, sdpstAftAllocExtInfo 를 위해서 정의함. */
#define SDPST_EXTDIR    (SDPST_VIRTBMP)

/*
 * 특정 노드의 특정 Slot의 순번을 나타낸다.
 */
typedef struct sdpstPosItem
{
    scPageID         mNodePID;
    SShort           mIndex;
    UChar            mAlign[2];
} sdpstPosItem;

/*
 * Treelist에서의 위치를 표현하는 자료구조를 Stack을 정의한다.
 *
 * 기본적으로 Treelist를 traverse하는데 사용되며,  HWM와
 * Hint It BMP 페이지의 위치를 저장하고 관리하기 위한 자료구조이며,
 * Treelist내에서의 선후위치 관계를 비교하는데 필수적인 자료구조이다.
 */
typedef struct sdpstStack
{
    sdpstPosItem    mPosition[ SDPST_BMP_TYPE_MAX ];
    SInt            mDepth;
    UChar           mAlign[4];
} sdpstStack;

/* 상위 bmp들에 대한 가용도 상태 변경 시작 위치 정의 */
typedef enum sdpstChangeMFNLPhase
{
    SDPST_CHANGEMFNL_RTBMP_PHASE = 0,
    SDPST_CHANGEMFNL_ITBMP_PHASE,
    SDPST_CHANGEMFNL_LFBMP_PHASE,
    SDPST_CHANGEMFNL_MAX_PHASE
} sdpstChangeMFNLPhase;

/* Page의 High/Low Water Mark 정의 */
typedef struct sdpstWM
{
    scPageID        mWMPID;          /* WM PID */
    scPageID        mExtDirPID;      /* ExtDir 페이지의 PID */
    sdpstStack      mStack;          /* WM stack */
    SShort          mSlotNoInExtDir; /* ExtDir 페이지내에서의 ExtDesc 순번 */
    UChar           mAlign[6]; 
} sdpstWM;

/* Bitmap 페이지에서의 유효하지 않은 Slot 순번
 * ( Slot 검색시 유효하지 않는 경우에 반환값으로 사용한다) */
# define SDPST_INVALID_SLOTNO         (-1)

// leaf bitmap 페이지에서의 유효하지 않은 Page 순번 (PBS = PageBitSet)
# define SDPST_INVALID_PBSNO         (-1)

// 동일한 Bitmap 페이지 내에 존재하지 않는다.
# define SDPST_FAR_AWAY_OFF            (ID_SSHORT_MAX)

// leaf bitmap page의 최대 가용공간 레벨 정의 6종류
typedef enum sdpstMFNL
{
    SDPST_MFNL_FUL = 0, /* FULL      */
    SDPST_MFNL_INS,      /* INSERTABLE */
    SDPST_MFNL_FMT,      /* FORMAT    */
    SDPST_MFNL_UNF,      /* UNFORMAT  */
    SDPST_MFNL_MAX       /* 최대개수  */
} sdpstMFNL;


/*
 * Segment 크기의 기준으로 결정되는 leaf bitmap page의
 * range를 정의한다.
 */
typedef enum
{
    SDPST_PAGE_RANGE_NULL = 0,     /* NULL */
    SDPST_PAGE_RANGE_16   = 16,    /*        Segment Size < 1m  */
    SDPST_PAGE_RANGE_64   = 64,    /*  1M <= Segment Size < 64M */
    SDPST_PAGE_RANGE_256  = 256,   /* 64M <= Segment Size < 1G  */
    SDPST_PAGE_RANGE_1024 = 1024   /*  1G <= Segment Size       */
} sdpstPageRange;

/* 탐색 시작 Internal BMP Hint */
typedef struct sdpstSearchHint
{
    /* Hint 재설정 여부 */
    idBool           mUpdateHintItBMP;

    /* Hint Stack 갱신 동시성을 위한 Latch */
    iduLatch      mLatch4Hint;

    /* 위치이력 */
    sdpstStack       mHintItStack;
} sdpstSearchHint;

/*
 * Segment Cache 자료구조 (Runtime 정보)
 */
typedef struct sdpstSegCache
{
    sdpSegCCache     mCommon;

    iduLatch         mLatch4WM;          /* WM 갱신 동시성을 위한 Mutex */
    iduMutex         mExtendExt;         /* Segment 공간 확장을 위한 Mutex */
    idBool           mOnExtend;          /* Segment 확장 진행 여부 */
    iduCond          mCondVar;           /* Condition Variable */
    UInt             mWaitThrCnt4Extend; /* Waiter */

    sdpSegType       mSegType;

    /* Slot 할당을 위한 가용공간탐색 시작
     * internal bitmap page의 Hint*/
    sdpstSearchHint  mHint4Slot;

    /* Page 할당을 위한 가용공간탐색 시작
     * internal bitmap page의 Hint*/
    sdpstSearchHint  mHint4Page;

    /* Candidate Child Set을 만들기 위한 Hint */
    UShort           mHint4CandidateChild;

    /* Format Page count */
    ULong            mFmtPageCnt;

    /* HWM */
    sdpstWM          mHWM;

    /* PRJ-1671 Bitmap Tablespace, Load HWM
     * BUG-29005 Fullscan 성능 문제
     * 마지막으로 할당된 페이지까지만 Fullscan한다. */
    iduMutex         mMutex4LstAllocPage;

    /* FullScan Hint 사용 여부 */
    idBool           mUseLstAllocPageHint;

    /* 마지막으로 할당된 페이지 정보 */
    scPageID         mLstAllocPID;
    ULong            mLstAllocSeqNo;

    /* hint insertable page hint array
     * BUG-28935에서 처음 사용 시 alloc받도록 수정*/
    scPageID       * mHint4DataPage;
} sdpstSegCache;

// Segment에 Extent 할당연산 중에  참고해야할
// Bitmap page들에 대한 정보를 정의한 자료구조
typedef struct sdpstBfrAllocExtInfo
{
    /* 마지막 rt, it, lf-bmp, ExtDir의 PID */
    scPageID        mLstPID[SDPST_BMP_TYPE_MAX];

    UShort          mFreeSlotCnt[SDPST_BMP_TYPE_MAX];
    UShort          mMaxSlotCnt[SDPST_BMP_TYPE_MAX];

    /* Segment의 총 페이지 개수  */
    ULong           mTotPageCnt;

    /* 현재 Page Range */
    sdpstPageRange  mPageRange;
    UShort          mFreePageRangeCnt;

    SShort          mSlotNoInExtDir;
    SShort          mLstPBSNo;

    ULong           mNxtSeqNo;

} sdpstBfrAllocExtInfo;

typedef struct sdpstAftAllocExtInfo
{
    /* 확장이후 새로운 첫번째 bmp, ExtDir 페이지의 PID */
    scPageID        mFstPID[SDPST_BMP_TYPE_MAX];
    /* 확장이후 새로운 마지막 bmp, ExtDir 페이지의 PID */
    scPageID        mLstPID[SDPST_BMP_TYPE_MAX];


    /* 이번에 새롭게 생성할 Meta 페이지의 개수 및
     * 생성할 페이지의 최대 slot 개수 */
    UShort          mPageCnt[SDPST_BMP_TYPE_MAX];
    UShort          mFullBMPCnt[SDPST_BMP_TYPE_MAX];
//    UShort          mMaxSlotCnt[SDPST_BMP_TYPE_MAX];

    /* 새로운 Segment의 총 페이지 개수  */
    ULong           mTotPageCnt;

    /* ExtDir을 생성해야할 Leaf BMP 페이지 */
    scPageID        mLfBMP4ExtDir;

    /* 현재 Page Range */
    sdpstPageRange  mPageRange;

    /* 이번에 새롭게 생성할 Segment Meta Header 페이지의 개수
     * 중요: 0 또는 1 값만 갖는다. */
    UShort          mSegHdrCnt;

    SShort          mSlotNoInExtDir;
} sdpstAftAllocExtInfo;


/* Extent Slot 정의 */
typedef struct sdpstExtDesc
{
    scPageID       mExtFstPID;     /* Extent 첫번째 페이지의 PID */
    UInt           mLength;        /* Extent의 페이지 개수 */
    scPageID       mExtMgmtLfBMP;  /* ExtDir 페이지를 관리하는 lf-bmp
                                      페이지의 PID */
    scPageID       mExtFstDataPID; /* Extent의 첫번째 Data 페이지의 PID */
} sdpstExtDesc;

// ExtDir Control Header 정의
typedef struct sdpstExtDirHdr
{
    UShort       mExtCnt;          // 페이지내의 Extent 개수
    UShort       mMaxExtCnt;       // 페이지내의 최대 ExtDesc 개수
    scOffset     mBodyOffset;       // 페이지 내의 ExtDir의 Offset
    UChar        mAlign[2];
} sdpstExtDirHdr;


/* BMP Slot 정의 */
typedef struct sdpstBMPSlot
{
    scPageID      mBMP;   // rt/it-BMP 페이지의 PID
    sdpstMFNL     mMFNL;  // rt/it-BMP 페이지의 Maximum Freeness Level
} sdpstBMPSlot;

/* Leaf Bitmap 페이지의 Page RaOBnge Slot 개수 */
# define SDPST_MAX_RANGE_SLOT_COUNT   (16)

/* 탐색과정에서 Unformat 페이지를 만났을 경우 */
# define SDPST_MAX_FORMAT_PAGES_AT_ONCE (16)

/* 하나의 Page를 표현하는 bitset의 길이 */
# define SDPST_PAGE_BITSET_SIZE  (8)

/*
 * lf-BMP의 Page Bitset Table 크기
 * SDPST_PAGE_BITSET_TABLE_SIZE / SDPST_PAGE_BITSET_SIZE = 1024 페이지 표현
 */
# define SDPST_PAGE_BITSET_TABLE_SIZE ( 1024 )

/*
 * lf-bmp에서 관리할 Data Page를 range를 나누어 관리하기 위한 range slot
 */
typedef struct sdpstRangeSlot
{
    scPageID    mFstPID;            // Range slot의 첫번째 페이지의 PID
    UShort      mLength;            // Range slot의 페이지 개수
    SShort      mFstPBSNo;          // Range내에서의 Range slot의 첫번째
                                    // 페이지의 순번
    scPageID    mExtDirPID;         // ExtDir Page의 PID
    SShort      mSlotNoInExtDir;    // ExtDir에서 Extent Slot No
    UChar       mAlign[2];
} sdpstRangeSlot;

/* Page BitSet을 1바이트로 정의한다. */
typedef UChar sdpstPBS;

/*
 * lf-BMP 에서 Page range를 관리하기 위한 map
 */
typedef struct sdpstRangeMap
{
    sdpstRangeSlot  mRangeSlot[ SDPST_MAX_RANGE_SLOT_COUNT ];
    sdpstPBS        mPBSTbl[ SDPST_PAGE_BITSET_TABLE_SIZE ];
}sdpstRangeMap;

/*
 * Bitmap 페이지의 Header
 */
typedef struct sdpstBMPHdr
{
    sdpstBMPType    mType;
    sdpstMFNL       mMFNL;              /* 페이지의 MFNL */
    UShort          mMFNLTbl[SDPST_MFNL_MAX];   /* slot의 MFNL별 개수 */
    UShort          mSlotCnt;           /* 페이지내의 BMP 페이지의 개수 */
    UShort          mFreeSlotCnt;       /* 페이지 내의 free slot 개수 */
    UShort          mMaxSlotCnt;        /* 페이지 내의 최대 BMP 개수 */
    SShort          mFstFreeSlotNo;     /* 첫번째 free slot 순번 */
    sdpParentInfo   mParentInfo;        /* 상위 BMP 페이지 PID, slot no */
    scPageID        mNxtRtBMP;          /* 다음 RtBMP PID. RtBMP만 사용 */
    scOffset        mBodyOffset;        /* 페이지내의 BMP map의  Offset */
    UChar           mAlign[2];
} sdpstBMPHdr;

/*
 * Leaf Bitmap 페이지의 Header
 */
typedef struct sdpstLfBMPHdr
{
    sdpstBMPHdr     mBMPHdr;

    sdpstPageRange  mPageRange;       // Leaf BMP의 결정된 Page Range
    UShort          mTotPageCnt;

    // 최초 사용가능했던 Data 페이지의 순번
    SShort          mFstDataPagePBSNo;  // Leaf BMP에서 첫번째 Data 페이지 Index
} sdpstLfBMPHdr;

/* Segment Size를 Page 개수로 나타냄 */
# define SDPST_PAGE_CNT_1M     (    1 * 1024 * 1024 / SD_PAGE_SIZE )
# define SDPST_PAGE_CNT_64M    (   64 * 1024 * 1024 / SD_PAGE_SIZE )
# define SDPST_PAGE_CNT_1024M  ( 1024 * 1024 * 1024 / SD_PAGE_SIZE )

/* leaf bitmap 페이지 Page BitSet의 페이지 종류(TyPe)를 정의한다. */
# define SDPST_BITSET_PAGETP_MASK   (0x80)
# define SDPST_BITSET_PAGETP_META   (0x80)  /* Meta Page */
# define SDPST_BITSET_PAGETP_DATA   (0x00)  /* Data Page (Meta를 제외한 나머지) */

/* leaf bitmap 페이지 Page BitSet의 페이지 가용도(FreeNess)를 정의한다. */
# define SDPST_BITSET_PAGEFN_MASK   (0x7F)
# define SDPST_BITSET_PAGEFN_UNF    (0x00)  /* Unformat */
# define SDPST_BITSET_PAGEFN_FMT    (0x01)  /* Format */
# define SDPST_BITSET_PAGEFN_INS    (0x02)  /* Insertable */
# define SDPST_BITSET_PAGEFN_FUL    (0x04)  /* Full */

/* Free Data 페이지 후보 작성 비트셋 */
# define SDPST_BITSET_CANDIDATE_ALLOCPAGE ( SDPST_BITSET_PAGEFN_UNF  | \
                                            SDPST_BITSET_PAGEFN_FMT  )

/* Insertable Data 페이지 후보 작성 비트셋 정의 */
# define SDPST_BITSET_CANDIDATE_ALLOCSLOT ( SDPST_BITSET_PAGEFN_INS  | \
                                            SDPST_BITSET_PAGEFN_UNF  | \
                                            SDPST_BITSET_PAGEFN_FMT  )

/* formate된 페이지중 Insertable한 페이지 비트셋  정의 */
# define SDPST_BITSET_CHECK_ALLOCSLOT ( SDPST_BITSET_PAGEFN_INS  | \
                                        SDPST_BITSET_PAGEFN_FMT  )

/*
 * Segment Header 페이지 정의
 * Segment의 Treelist와 Extent 정보 BMP 정보등을 관리한다.
 */
typedef struct sdpstSegHdr
{
    /* Segment Type과 State */
    sdpSegType          mSegType;
    sdpSegState         mSegState;

    /* Segment Header가 존재하는 PID */
    scPageID            mSegHdrPID;

    /* 마지막 BMP의 PID & Page SeqNo */
    scPageID            mLstLfBMP;
    scPageID            mLstItBMP;
    scPageID            mLstRtBMP;
    ULong               mLstSeqNo;

    /* Segment에 할당된 총 페이지 개수 */
    ULong               mTotPageCnt;
    /* Segment에 할당된 총 RtBMP 페이지 개수 */
    ULong               mTotRtBMPCnt;
    /* Free 페이지 개수 */
    ULong               mFreeIndexPageCnt;
    /* Segment에 할당된 Extent 총 개수 */
    ULong               mTotExtCnt;
    /* Extent당 페이지 개수 */
    UInt                mPageCntInExt;
    UChar               mAlign[4];
    /* 마지막으로 할당된 Extent 의 RID */
    sdRID               mLstExtRID;

    /* HWM */
    sdpstWM             mHWM;

    /* Meta Page ID Array: Index Seg는 이 PageID Array에 Root Node PID를
     * 기록한다. */
    scPageID            mArrMetaPID[ SDP_MAX_SEG_PID_CNT ];

    /* ExtDir PID List */
    sdpDblPIDListBase   mExtDirBase;

    /* Segment Header의 남는 공간을 RtBMP와 ExtDir로 사용한다. */
    sdpstExtDirHdr      mExtDirHdr;
    sdpstBMPHdr         mRtBMPHdr;
} sdpstSegHdr;


/*
 * 가용공간 탐색 대상
 */
typedef enum sdpstSearchType
{
    SDPST_SEARCH_NEWSLOT = 0,
    SDPST_SEARCH_NEWPAGE
} sdpstSearchType;

/*
 * 탐색시 임시로 필요한 후보 Data 페이지 정보를 정의한다.
 */
typedef struct sdpstCandidatePage
{
    scPageID        mPageID;
    SShort          mPBSNo;
    sdpstPBS        mPBS;
    UShort          mRangeSlotNo;
} sdpstCandidatePage;

/*
 * BMP별 Fix and GetPage 함수 정의
 */
typedef IDE_RC (*sdpstFixAndGetHdr4UpdateFunc)( idvSQL       * aStatistics,
                                                sdrMtx       * aMtx,
                                                scSpaceID      aSpaceID,
                                                scPageID       aPageID,
                                                sdpstBMPHdr ** aBMPHdr );

typedef IDE_RC (*sdpstFixAndGetHdr4ReadFunc)( idvSQL       * aStatistics,
                                              sdrMtx       * aMtx,
                                              scSpaceID      aSpaceID,
                                              scPageID       aPageID,
                                              sdpstBMPHdr ** aBMPHdr );

typedef IDE_RC (*sdpstFixAndGetHdrFunc)( idvSQL       * aStatistics,
                                         scSpaceID      aSpaceID,
                                         scPageID       aPageID,
                                         sdpstBMPHdr ** aBMPHdr );

/* BMP 헤더를 가져온다. */
typedef sdpstBMPHdr * (*sdpstGetBMPHdrFunc)( UChar * aPagePtr );

/* SlotCnt 또는 PageCnt를 가져온다. */
typedef UShort (*sdpstGetSlotOrPageCntFunc)( UChar * aPagePtr );

/* depth별로 stack간의 거리차를 구한다. */
typedef SShort (*sdpstGetDistInDepthFunc)( sdpstPosItem * aLHS,
                                           sdpstPosItem * aRHS );

/*
 * for makeCandidateChild
 */
/* Candidate Child 여부를 확인한다. */
typedef idBool (*sdpstIsCandidateChildFunc)( UChar     * aPagePtr,
                                             SShort      aSlotNo,
                                             sdpstMFNL   aTargetMFNL );

/* Candidate Child 선정시 탐색 시작 위치를 찾는다. */
typedef UShort (*sdpstGetStartSlotOrPBSNoFunc)( UChar  * aPagePtr,
                                                SShort   aTransID );

/* Candidate Child 선정시 탐색 대상 개수를 구한다. */
typedef SShort (*sdpstGetChildCountFunc)( UChar         * aPagePtr,
                                          sdpstWM       * aHWM );

/* 후보를 저장하는 배열에 후보를 저장한다. */
typedef void (*sdpstSetCandidateArrayFunc)( UChar     * aPagePtr,
                                            UShort      aSlotNo,
                                            UShort      aArrIdx,
                                            void      * aCandidateArray );

/* 첫번째 free slot 또는 free PBS */
typedef SShort (*sdpstGetFstFreeSlotOrPBSNoFunc)( UChar     * aPagePtr );

/* 최대 후보 선택 개수를 구한다. */
typedef UInt (*sdpstGetMaxCandidateCntFunc)();

/*
 * updateBMPuntilHWM
 */
typedef IDE_RC (*sdpstUpdateBMPUntilHWMFunc)( idvSQL            * aStatistics,
                                              sdrMtxStartInfo   * aStartInfo,
                                              scSpaceID           aSpaceID,
                                              scPageID            aSegPID,
                                              sdpstStack        * aHWMStack,
                                              sdpstStack        * aCurStack,
                                              SShort            * aFmSlotNo,
                                              SShort            * aFmItSlotNo,
                                              sdpstBMPType      * aPrvDepth,
                                              idBool            * aIsFinish,
                                              sdpstMFNL         * aNewMFNL );

/* MFNL이 동일한지 확인한다. */
typedef IDE_RC (*sdpstIsEqUnfFunc)( sdpstBMPHdr * aBMPHdr,
                                    UShort        aSlotNo );

/* 상위 BMP로 이동한다. */
typedef void (*sdpstGoParentBMPFunc)( sdpstBMPHdr   * aBMPHdr,
                                      sdpstStack    * aStack );

/* 하위 BMP로 이동한다. */
typedef void (*sdpstGoChildBMPFunc)( sdpstStack    * aStack,
                                     sdpstBMPHdr   * aBMPHdr,
                                     UShort          aSlotNo );

typedef scPageID (*sdpstGetLstChildBMPFunc)( sdpstBMPHdr * aBMPHdr );

typedef struct sdpstBMPOps
{
    sdpstGetBMPHdrFunc                 mGetBMPHdr;

    sdpstGetSlotOrPageCntFunc          mGetSlotOrPageCnt;
    sdpstGetDistInDepthFunc            mGetDistInDepth;

    /* 후보 탐색 관련 함수 */
    sdpstIsCandidateChildFunc          mIsCandidateChild;
    sdpstGetStartSlotOrPBSNoFunc       mGetStartSlotOrPBSNo;
    sdpstGetChildCountFunc             mGetChildCount;
    sdpstSetCandidateArrayFunc         mSetCandidateArray;
    sdpstGetFstFreeSlotOrPBSNoFunc     mGetFstFreeSlotOrPBSNo;
    sdpstGetMaxCandidateCntFunc        mGetMaxCandidateCnt;

    /* updateBMPUntilHWM */
    sdpstUpdateBMPUntilHWMFunc         mUpdateBMPUntilHWM;
} sdpstBMPOps;

# define SDPST_SEARCH_QUOTA_IN_RTBMP        (3)

# define SDPST_MAX_CANDIDATE_LFBMP_CNT      (1024)
# define SDPST_MAX_CANDIDATE_PAGE_CNT       (1024)

/* Search 타입에 따른 최소 만족해야하는 MFNL 정의 */
# define SDPST_SEARCH_TARGET_MIN_MFNL( aSearchType ) \
    (aSearchType == SDPST_SEARCH_NEWPAGE ? SDPST_MFNL_FMT : SDPST_MFNL_INS )

/* TMS에서 사용하는 프로퍼티 */
# define SDPST_SEARCH_WITHOUT_HASHING()     \
    ( smuProperty::getTmsSearchWithoutHashing() )
# define SDPST_CANDIDATE_LFBMP_CNT()        \
    ( smuProperty::getTmsCandidateLfBMPCnt() )
# define SDPST_CANDIDATE_PAGE_CNT()         \
    ( smuProperty::getTmsCandidatePageCnt() )

# define SDPST_MAX_SLOT_CNT_PER_RTBMP()     \
    ( smuProperty::getTmsMaxSlotCntPerRtBMP() )
# define SDPST_MAX_SLOT_CNT_PER_ITBMP()     \
    ( smuProperty::getTmsMaxSlotCntPerItBMP() )
# define SDPST_MAX_SLOT_CNT_PER_LFBMP()     \
    ( SDPST_MAX_RANGE_SLOT_COUNT )
# define SDPST_MAX_SLOT_CNT_PER_EXTDIR()    \
    ( smuProperty::getTmsMaxSlotCntPerExtDir() )

# define SDPST_MAX_SLOT_CNT_PER_RTBMP_IN_SEGHDR()        \
    ( (SDPST_MAX_SLOT_CNT_PER_RTBMP() <=                 \
            sdpstBMP::getMaxSlotCnt4Property() / 2) ?    \
      SDPST_MAX_SLOT_CNT_PER_RTBMP() :                   \
      SDPST_MAX_SLOT_CNT_PER_RTBMP() / 2 )
# define SDPST_MAX_SLOT_CNT_PER_EXTDIR_IN_SEGHDR()       \
    ( (SDPST_MAX_SLOT_CNT_PER_EXTDIR() <=                \
            sdpstExtDir::getMaxSlotCnt4Property() / 2) ? \
      SDPST_MAX_SLOT_CNT_PER_EXTDIR() :                  \
      SDPST_MAX_SLOT_CNT_PER_EXTDIR() / 2 )

/*
 * SDR_SDPST_INIT_SEGHDR
 */
typedef struct sdpstRedoInitSegHdr
{
    sdpSegType      mSegType;
    scPageID        mSegPID;
    UInt            mPageCntInExt;
    UShort          mMaxExtDescCntInExtDir;
    UShort          mMaxSlotCntInRtBMP;
} sdpstRedoInitSegHdr;

/*
 * SDR_SDPST_UPDATE_WM
 */
typedef struct sdpstRedoUpdateWM
{
    scPageID        mWMPID;
    scPageID        mExtDirPID;
    sdpstStack      mStack;
    SShort          mSlotNoInExtDir;
    UChar           mAligh[6];
} sdpstRedoUpdateWM;

/*
 * SDR_SDPST_INIT_BMP
 */
typedef struct sdpstRedoInitBMP
{
    sdpstBMPType    mType;
    sdpParentInfo   mParentInfo;
    scPageID        mFstChildBMP;
    scPageID        mLstChildBMP;
    UShort          mFullBMPCnt;
    UShort          mMaxSlotCnt;
} sdpstRedoInitBMP;

/*
 * SDR_SDPST_INIT_LFBMP
 */
typedef struct sdpstRedoInitLfBMP
{
    sdpstPageRange  mPageRange;
    sdpParentInfo   mParentInfo;
    scPageID        mRangeFstPID;
    scPageID        mExtDirPID;
    UShort          mNewPageCnt;
    UShort          mMetaPageCnt;
    SShort          mSlotNoInExtDir;
    UShort          mMaxSlotCnt;
} sdpstRedoInitLfBMP;

/*
 * SDR_SDPST_INIT_EXTDIR
 */
typedef struct sdpstRedoInitExtDir
{
    UShort          mMaxExtCnt;
    scOffset        mBodyOffset;
} sdpstRedoInitExtDir;

/*
 * SDR_SDPST_ADD_EXTDESC
 */
typedef struct sdpstRedoAddExtDesc
{
    scPageID       mExtFstPID;
    UInt           mLength;
    scPageID       mExtMgmtLfBMP;
    scPageID       mExtFstDataPID;
    UShort         mLstSlotNo;
} sdpstRedoAddExtDesc;

/*
 * SDR_SDPST_ADD_SLOT
 */
typedef struct sdpstRedoAddSlots
{
    scPageID        mFstChildBMP;
    scPageID        mLstChildBMP;
    UShort          mLstSlotNo;
    UShort          mFullBMPCnt;
} sdpstRedoAddSlots;

/*
 * SDR_SDPST_ADD_RANGESLOT
 */
typedef struct sdpstRedoAddRangeSlot
{
    scPageID        mFstPID;
    scPageID        mExtDirPID;
    UShort          mLength;
    UShort          mMetaPageCnt;
    SShort          mSlotNoInExtDir;
} sdpstRedoAddRangeSlot;

/*
 * SDR_SDPST_ADD_EXT_TO_SEGHDR
 */
typedef struct sdpstRedoAddExtToSegHdr
{
    ULong           mAllocPageCnt;
    ULong           mMetaPageCnt;
    scPageID        mNewLstLfBMP;
    scPageID        mNewLstItBMP;
    scPageID        mNewLstRtBMP;
    UShort          mNewRtBMPCnt;
} sdpstRedoAddExtToSegHdr;

/*
 * SDR_SDPST_UPDATE_PBS
 */
typedef struct sdpstRedoUpdatePBS
{
    SShort          mFstPBSNo;
    UShort          mPageCnt;
    sdpstPBS        mPBS;
} sdpstRedoUpdatePBS;

/*
 * SDR_SDPST_UPDATE_MFNL
 */
typedef struct sdpstRedoUpdateMFNL
{
    sdpstMFNL       mFmMFNL;
    sdpstMFNL       mToMFNL;
    UShort          mPageCnt;
    SShort          mFmSlotNo;
    SShort          mToSlotNo;
} sdpstRedoUpdateMFNL;

/*
 * SDR_SDPST_UPDATE_LFBMP_4DPATH
 */
typedef struct sdpstRedoUpdateLfBMP4DPath
{
    SShort          mFmPBSNo;
    UShort          mPageCnt;
} sdpstRedoUpdateLfBMP4DPath;

/*
 * SDR_SDPST_UPDATE_BMP_4DPATH
 */
typedef struct sdpstRedoUpdateBMP4DPath
{
    SShort          mFmSlotNo;
    UShort          mSlotCnt;
    sdpstMFNL       mToMFNL;
    sdpstMFNL       mMFNLOfLstSlot;
} sdpstRedoUpdateBMP4DPath;

# endif // _O_SDPST_DEF_H_
