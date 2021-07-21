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
 * $Id: smpDef.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description:
 *
 * TABLE Layer의 PageList관리는 다음과 같다.
 *
 * TableHeader
 *     ㄴ AllocPageList[] (For Fixed)
 *     ㄴ PageListEntry (For Fixed)
 *                     :
 *             ㄴ FreePagePool
 *             ㄴ FreePageList[] - SizeClassList[]
 *                                        :
 *                     :
 *     ㄴ AllocPageList[] (For Var)
 *     ㄴ PageListEntry[] (For Var)
 *             :
 * 테이블헤더에 Fixed와 Var용의 PageListEntry가 각각 존재하며
 * 각각의 PageListEntry에는 Page의 Alloc정보를 갖는 AllocPageList와
 * Page의 Free정보를 갖는 FreePageList를 유지한다.
 * 각각의 AllocPageList와 FreePageList는 프로퍼티의 PageListGroupCount값으로
 * 다중화되어 트랜잭션이 동시 접근가능하게 하였으며(PROJ-1490),
 * FreePageList에는 Slot의 사용량에 따라 SizeClass별로 그룹핑하였다.
 * 또한 FreePageList간의 밸런싱을 위해 FreePagePool을 유지하였다.
 *
 * Transaction
 *     ㄴ PrivatePageListCachePtr
 *     ㄴ PrivatePageListHashTable
 *
 * 트랜잭션이 DB에서 할당한 Page들은 다른 트랜잭션이 사용하지 않도록
 * PrivatePageList에서 관리하도록 하였으며(PROJ-1464),
 * 최근에 접근했던 PrivatePageList에 대한 Cache포인터를 두어
 * 성능을 높일 수 있도록 하였다.
 *
 **********************************************************************/

#ifndef _O_SMP_DEF_H_
#define _O_SMP_DEF_H_ 1

#include <smDef.h>
#include <idu.h>

/* PROJ-2162 RestartRiskReduction */
// PageType을 정의함
typedef UInt smpPageType;

#define  SMP_PAGETYPE_MASK                  (0x00000003)
#define  SMP_PAGETYPE_NONE                  (0x00000000)
#define  SMP_PAGETYPE_FIX                   (0x00000001)
#define  SMP_PAGETYPE_VAR                   (0x00000002)

#define  SMP_PAGEINCONSISTENCY_MASK         (0x80000000)
#define  SMP_PAGEINCONSISTENCY_FALSE        (0x00000000)
#define  SMP_PAGEINCONSISTENCY_TRUE         (0x80000000)

// alloc/free slot하는 대상 테이블 타입
typedef enum
{
    SMP_TABLE_NORMAL = 0,  // 일반 테이블
    SMP_TABLE_TEMP         // TEMP 테이블
} smpTableType;

/* ----------------------------------------------------------------------------
 *    PersPage Def
 * --------------------------------------------------------------------------*/
// BUGBUG : If smpPersPageHeader is changed,
//          SMM_MEMBASE_OFFSET should be check also
// Critical : smpPersPageHeader의 mPrev/mNext는 PageList를 위한 것이다.
//            이것이 DB와 TABLE이 주고 받을때 임시로 사용할때는 단일한 리스트이지만,
//            테이블에 등록될 때는 AllocPageList가 다중화 되어 있으므로
//            mPrev/mNext를 바로 접근하는 것 보다 smpManager::getPrevAllocPageID()
//            getNextAllocPageID()를 사용해야 테이블에 다중화된 PageList를 올바로
//            접근할 수 있게 된다.
typedef struct smpPersPageHeader
{
    scPageID           mSelfPageID;   // 해당 Page의 PID
    scPageID           mPrevPageID;   // 이전 Page의 PID
    scPageID           mNextPageID;   // 다음 Page의 PID
    smpPageType        mType;         // 해당 Page의 Page Type
    smOID              mTableOID;
    UInt               mAllocListID;  // 해당 Page의 AllocPageList의 ListID
#if defined(COMPILE_64BIT)
    SChar              mDummy[4];
#endif
} smpPersPageHeader;


typedef struct smpVarPageHeader
{
    vULong               mIdx;    // BUGBUG: which var page list.
#if !defined(COMPILE_64BIT)
    SChar                mDummy[4];
#endif
} smpVarPageHeader;

struct __smpPersPage__
{
    smpPersPageHeader mHeader;
    SDouble           mBody[1]; // for align calculation
};

#define SMP_PERS_PAGE_BODY_OFFSET (offsetof(__smpPersPage__, mBody))
#define SMP_PERS_PAGE_BODY_SIZE        (SM_PAGE_SIZE - offsetof(__smpPersPage__, mBody))

#define SMP_MAX_FIXED_ROW_SIZE ( SMP_PERS_PAGE_BODY_SIZE - ID_SIZEOF(smpSlotHeader) )

struct smpPersPage
{
    smpPersPageHeader mHeader;
    UChar             mBody[SMP_PERS_PAGE_BODY_SIZE];
};

#define SMP_GET_VC_PIECE_COUNT(length) ((length + SMP_VC_PIECE_MAX_SIZE - 1) / SMP_VC_PIECE_MAX_SIZE)
#define SMP_GET_PERS_PAGE_ID(page) (((smpPersPage*)(page))->mHeader.mSelfPageID)
#define SMP_SET_PERS_PAGE_ID(page, pid) (((smpPersPage*)(page))->mHeader.mSelfPageID = (pid))
#define SMP_SET_PREV_PERS_PAGE_ID(page, pid) (((smpPersPage*)(page))->mHeader.mPrevPageID = (pid))
#define SMP_SET_NEXT_PERS_PAGE_ID(page, pid) (((smpPersPage*)(page))->mHeader.mNextPageID = (pid))

#define SMP_GET_NEXT_PERS_PAGE_ID(page) (((smpPersPage*)(page))->mHeader.mNextPageID)
#define SMP_GET_PREV_PERS_PAGE_ID(page) (((smpPersPage*)(page))->mHeader.mPrevPageID)
#define SMP_SET_PERS_PAGE_TYPE(page, aPageType) (((smpPersPage*)(page))->mHeader.mType = (aPageType))
#define SMP_GET_PERS_PAGE_TYPE( page ) (((smpPersPage*)(page))->mHeader.mType & SMP_PAGETYPE_MASK)
#define SMP_GET_PERS_PAGE_INCONSISTENCY( page ) (((smpPersPage*)(page))->mHeader.mType & SMP_PAGEINCONSISTENCY_MASK)


/* ----------------------------------------------------------------------------
 *    page item(slot) size
 * --------------------------------------------------------------------------*/
/* Max Variable Column Piece Size */
#define SMP_VC_PIECE_MAX_SIZE ( SMP_PERS_PAGE_BODY_SIZE - \
                           ID_SIZEOF(smpVarPageHeader) - ID_SIZEOF(smVCPieceHeader) )

/* BUGBUG: By Newdaily QP에서 32K확장 작업이 끝나면 32K로 바꾸어야한다.  */
//#define SMP_MAX_VAR_COLUMN_SIZE   SMP_VC_PIECE_MAX_SIZE
// PROJ-1583 large geometry
#define SMP_MAX_VAR_COLUMN_SIZE  ID_UINT_MAX // 4G

/* Min Variable Column Piece Size */
#define SMP_VC_PIECE_MIN_SIZE (SM_DVAR(1) << SM_ITEM_MIN_BIT_SIZE)

/* ----------------------------------------------------------------------------
 *    FreePageHeader Def
 * --------------------------------------------------------------------------*/

typedef struct smpFreePageHeader
{
    UInt               mFreeListID;     // 해당 FreePage의 FreePageList ID
    UInt               mSizeClassID;    // 해당 FreePage의 SizeClass ID
    SChar*             mFreeSlotHead;   // 해당 FreePage에서의 FreeSlot Head
    SChar*             mFreeSlotTail;   // 해당 FreePage에서의 FreeSlot Tail
    UInt               mFreeSlotCount;  // 해당 FreePage에서의 FreeSlot 갯수
    smpFreePageHeader* mFreePrev;       // 이전 FreePage의 FreePageHeader
    smpFreePageHeader* mFreeNext;       // 다음 FreePage의 FreePageHeader
    scPageID           mSelfPageID;     // 해당 FreePage의 PID
    iduMutex           mMutex;          // 해당 FreePage의 Mutex
} smpFreePageHeader;



/* ----------------------------------------------------------------------------
 *                            For Slot Header
 * --------------------------------------------------------------------------*/
//이부분은 smpFreeSlotHeader과 같은 메모리 공간을 사용하기 때문에 수정할때, 변수의 순서등을
//고려해야 한다.

typedef struct smpSlotHeader
{
    smSCN       mCreateSCN;
    smSCN       mLimitSCN;
    ULong       mPosition;
    smOID       mVarOID;
} smpSlotHeader;



/* ----------------------------------------------------------------------------
 *    SCN
 *    1. Commit SCN
 *    2. Infinite SCN + TID
 *    3. Lock Row SCN + TID
 * --------------------------------------------------------------------------*/

#define SMP_GET_TID(aSCN)             SM_GET_TID_FROM_INF_SCN(aSCN)

#define SMP_SLOT_IS_LOCK_TRUE(aHdr)   SM_SCN_IS_LOCK_ROW((aHdr)->mLimitSCN)

#define SMP_SLOT_SET_LOCK(aHdr, aTID) SM_SET_SCN_LOCK_ROW(&((aHdr)->mLimitSCN), aTID)

#define SMP_SLOT_SET_UNLOCK(aHdr)     SM_SET_SCN_FREE_ROW(&((aHdr)->mLimitSCN))

/* ----------------------------------------------------------------------------
 *    Position (OID + OFFSET + FLAG)
 * --------------------------------------------------------------------------*/

/* OID */

#define SMP_SLOT_OID_OFFSET            (16)

#define SMP_SLOT_GET_NEXT_OID(a)       ((smOID)( (((smpSlotHeader*)(a))->mPosition) \
                                                >> SMP_SLOT_OID_OFFSET ))

#define SMP_SLOT_SET_NEXT_OID(a, oid) {                                      \
    IDE_DASSERT( SM_IS_OID( oid ) );                                         \
    IDE_DASSERT( SMP_SLOT_HAS_NULL_NEXT_OID( a ) );                          \
    (a)->mPosition |= ((ULong)(oid) << SMP_SLOT_OID_OFFSET);                 \
}

#define SMP_SLOT_HAS_VALID_NEXT_OID(a) SM_IS_VALID_OID( SMP_SLOT_GET_NEXT_OID( (a) ) )
#define SMP_SLOT_HAS_NULL_NEXT_OID(a)  SM_IS_NULL_OID( SMP_SLOT_GET_NEXT_OID( (a) ) )

/* OFFSET */

#define SMP_SLOT_OFFSET_MASK           (0x000000000000FFF8)
#define SMP_SLOT_GET_OFFSET(a)         ( (a)->mPosition & SMP_SLOT_OFFSET_MASK )
#define SMP_SLOT_SET_OFFSET(a, offset) ( (a)->mPosition = (ULong)offset )

#define SMP_SLOT_INIT_POSITION(a)      ( (a)->mPosition &= SMP_SLOT_OFFSET_MASK )


/* FLAG */

#define SMP_SLOT_USED_MASK             (0x0000000000000001)
#define SMP_SLOT_USED_TRUE             (0x0000000000000001)
#define SMP_SLOT_USED_FALSE            (0x0000000000000000)

#define SMP_SLOT_DROP_MASK             (0x0000000000000002)
#define SMP_SLOT_DROP_TRUE             (0x0000000000000002)
#define SMP_SLOT_DROP_FALSE            (0x0000000000000000)

#define SMP_SLOT_SKIP_REFINE_MASK      (0x0000000000000004)
#define SMP_SLOT_SKIP_REFINE_TRUE      (0x0000000000000004)
#define SMP_SLOT_SKIP_REFINE_FALSE     (0x0000000000000000)

#define SMP_SLOT_FLAG_MASK             (0x0000000000000007)

#define SMP_SLOT_GET_FLAGS(a) (((a)->mPosition) & SMP_SLOT_FLAG_MASK)

// slot use flag setting
#define SMP_SLOT_SET_USED(a)    ( (a)->mPosition |= SMP_SLOT_USED_TRUE )
#define SMP_SLOT_UNSET_USED(a)  ( (a)->mPosition &= ~(SMP_SLOT_USED_TRUE) )
#define SMP_SLOT_IS_USED(a)     ( ( (a)->mPosition & SMP_SLOT_USED_MASK )    \
                                  == SMP_SLOT_USED_TRUE )
#define SMP_SLOT_IS_NOT_USED(a) ( ( (a)->mPosition & SMP_SLOT_USED_MASK )    \
                                  == SMP_SLOT_USED_FALSE )
// table drop flag setting
#define SMP_SLOT_SET_DROP(a)    ( (a)->mPosition |= SMP_SLOT_DROP_TRUE )
#define SMP_SLOT_UNSET_DROP(a)  ( (a)->mPosition &= ~(SMP_SLOT_DROP_TRUE) )
#define SMP_SLOT_IS_DROP(a)     ( ( (a)->mPosition & SMP_SLOT_DROP_MASK )    \
                                  == SMP_SLOT_DROP_TRUE )
#define SMP_SLOT_IS_NOT_DROP(a) ( ( (a)->mPosition & SMP_SLOT_DROP_MASK )    \
                                  == SMP_SLOT_DROP_FALSE )

// skip refine flag setting
#define SMP_SLOT_SET_SKIP_REFINE(a)   ( (a)->mPosition |= SMP_SLOT_SKIP_REFINE_TRUE )
#define SMP_SLOT_UNSET_SKIP_REFINE(a) ( (a)->mPosition &= ~(SMP_SLOT_SKIP_REFINE_TRUE) )
#define SMP_SLOT_IS_SKIP_REFINE(a)    ( ( (a)->mPosition & SMP_SLOT_SKIP_REFINE_MASK ) \
                                        == SMP_SLOT_SKIP_REFINE_TRUE )
/* not yet used
#define SMP_SLOT_IS_NOT_SKIP_REFINE(a) ( ( (a)->mPosition & SMP_SLOT_SKIP_REFINE_MASK )\
                                         == SMP_SLOT_SKIP_REFINE_FALSE )
*/













// record pointer <-> PID
#define SMP_SLOT_GET_PID(pRow)                                           \
     (( (smpPersPageHeader *)( (SChar *)(pRow) -                          \
       SMP_SLOT_GET_OFFSET( (smpSlotHeader*)(pRow) ) ) )->mSelfPageID)

#define SMP_SLOT_GET_OID(pRow)                                           \
     SM_MAKE_OID( SMP_SLOT_GET_PID( (smpSlotHeader*)(pRow) ),            \
                  SMP_SLOT_GET_OFFSET( (smpSlotHeader *)(pRow) ) )

#define SMP_GET_PERS_PAGE_HEADER(pRow)                                   \
     ( (smpPersPageHeader *)( (SChar *)(pRow) -                          \
       SMP_SLOT_GET_OFFSET( (smpSlotHeader*)(pRow) ) ) )


#define SMP_SLOT_HEADER_SIZE (idlOS::align((UInt)ID_SIZEOF(smpSlotHeader)))

typedef struct smpFreeSlotHeader
{
    smSCN mCreateSCN;
    smSCN mLimitSCN;
    ULong mPosition;
//    smOID mVarOID;

    smpFreeSlotHeader* mNextFreeSlot;
} smpFreeSlotHeader;













/*
 * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
 */
typedef struct smpScanPageListEntry
{
    scPageID              mHeadPageID; // Next Scan Page ID
    scPageID              mTailPageID; // Prev Scan Page ID
    iduMutex             *mMutex;      // List의 Mutex
} smpScanPageListEntry;

typedef struct smpAllocPageListEntry
{
    vULong                mPageCount;  // AllocPage 갯수
    scPageID              mHeadPageID; // List의 Head
    scPageID              mTailPageID; // List의 Tail
    iduMutex             *mMutex;      // List의 Mutex
                                       // (AllocList가 Durable한 정보이고,
                                       //  Mutex는 Runtime정보이기 때문에
                                       //  포인터로 갖는다.)
} smpAllocPageListEntry;

// TX's Private Free Page List
typedef struct smpPrivatePageListEntry
{
    smOID              mTableOID;           // PageList의 테이블OID
    smpFreePageHeader* mFixedFreePageHead;  // Fixed영역을 위한 FreePageList
    smpFreePageHeader* mFixedFreePageTail;  // Fixed영역을 위한 FreePageList
    smpFreePageHeader* mVarFreePageHead[SM_VAR_PAGE_LIST_COUNT];
                                            // Var영역을 위한 FreePageList
    // Fixed영역은 MVCC때문에 예비 Slot을 남겨야 하기 때문에 중간 FreePage가
    // 먼저 제거될 수 있어서 양방향 리스트이나,
    // Var영역은 무조건 사용한 순서로 제거되므로 단방향 리스트로 구성한다.
} smpPrivatePageListEntry;

/*
 * BUG-25327 : [MDB] Free Page Size Class 개수를 Property화 해야 합니다.
 * FreePageList의 SizeClass는 4개가 최대이고,
 * Property MEM_SIZE_CLASS_COUNT에 의해서 제어된다.
 */
#define SMP_MAX_SIZE_CLASS_COUNT (4)
#define SMP_SIZE_CLASS_COUNT(x)  (((smpRuntimeEntry*)(x))->mSizeClassCount)

typedef struct smpFreePageListEntry
{
    /* ---------------------------------------------------------------------
     * Free Page List는 Size Class별로 관리된다.
     * Size Class는 Pagge내의 Free Slot비율에 따라 결정된다.
     * Simple Logic을 위해 Free Page List는 Size Class 최대 갯수로 초기화된다.
     * --------------------------------------------------------------------- */
    vULong                mPageCount[SMP_MAX_SIZE_CLASS_COUNT];  // FreePage갯수
    smpFreePageHeader*    mHead[SMP_MAX_SIZE_CLASS_COUNT];       // List의 Head
    smpFreePageHeader*    mTail[SMP_MAX_SIZE_CLASS_COUNT];       // List의 Tail
    iduMutex              mMutex;  // for Alloc/Free Page
} smpFreePageListEntry;

// FreePagePool은 EmptyPage만을 가진다.
// FreePagePool에서는 단방향리스트로 구성할 수 있지만
// FreePageList와 smpFreePageHeader를 공유하고
// FreePageList로 보내줄때 다시 양방향리스트를 구축해야 하므로
// FreePageList에서 받을때도 굳이 양방향리스트를 제거할 필요가 없다.
typedef struct smpFreePagePoolEntry
{
    vULong                mPageCount;   // EmptyPage갯수
    smpFreePageHeader*    mHead;        // List의 Head
    smpFreePageHeader*    mTail;        // List의 Tail
    iduMutex              mMutex;       // FreePagePool의 Mutex
} smpFreePagePoolEntry;

// FreePagePool과 FreePageList는 restart시에 재구축하는 영역으로
// Durable하지 않다.
typedef struct smpRuntimeEntry
{
    // TableHeader에 있는 AllocPageList에 대한 포인터를 갖는다.
    smpAllocPageListEntry* mAllocPageList;

    // Full Scan 전용 Page List
    smpScanPageListEntry   mScanPageList;

    // FreePagePool은 EmptyPage만 있으므로 하나의 리스트다
    smpFreePagePoolEntry  mFreePagePool;                        // FreePagePool
    smpFreePageListEntry  mFreePageList[SM_MAX_PAGELIST_COUNT]; // FreePageList

    /*
     * BUG-25327 : [MDB] Free Page Size Class 개수를 Property화 해야 합니다.
     */
    UInt                  mSizeClassCount;

    // Record Count 관련 멤버
    ULong                 mInsRecCnt;  // Inserted Record Count
    ULong                 mDelRecCnt;  // Deleted Record Count

    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부하 및
    //Aging이 밀리는 현상이 더 심화됨.
    //테이블이 가지는 old version 의 개수
    ULong                 mOldVersionCount;
    //중복키를 입력하려 하려하다 abort된 횟수
    ULong                 mUniqueViolationCount;
    //update할때 이미 다른 트랜잭션이 새로운 version을 생성하였다면,
    //그 Tx는 retry하여 새로운 scn을 따야 하는데...  이러한 사건의 발생 횟수.
    ULong                 mUpdateRetryCount;
    //remove할때 이미 다른 트랜잭션이 새로운 version을 생성하였다면,
    //그 Tx는 retry하여 새로운 scn을 따야 하는데...  이러한 사건의 발생 횟수.
    ULong                 mDeleteRetryCount;

    //LockRow시 이미 다른 트랜잭션이 Row를 갱신, 삭제후 Commit했다면
    //그 Tx는 retry하여 새로운 scn을 따야 하는데...  이러한 사건의 발생 횟수.
    ULong                 mLockRowRetryCount;

    iduMutex              mMutex;      // Count Mutex

} smpRuntimeEntry;

typedef struct smpPageListEntry
{
    vULong                mSlotSize;   // Slot의 Size(SlotHeader 포함)
    UInt                  mSlotCount;  // 이 페이지에 할당가능한 slot 갯수
    smOID                 mTableOID;   // PageList가 속한 테이블 OID

    // Runtime시에 재구성해야하는 정보를 PTR로 관리한다.
    smpRuntimeEntry       *mRuntimeEntry;
} smpPageListEntry;

// PageList를 다중화한다.
#define SMP_PAGE_LIST_COUNT     (smuProperty::getPageListGroupCount())
// PageListID가 NULL이라는 것은 어떤 PageList에도 달리지 않은 상황이다.
#define SMP_PAGELISTID_NULL     ID_UINT_MAX
// 다중화된 PageList의 마지막 PageListID
#define SMP_LAST_PAGELISTID     (SMP_PAGE_LIST_COUNT - 1)
// FreePagePool에 해당하는 PageListID
#define SMP_POOL_PAGELISTID     (ID_UINT_MAX - 2)
// Tx's Private Free Page List에 해당하는 PageListID
#define SMP_PRIVATE_PAGELISTID  (ID_UINT_MAX - 1)

// SizeClassID가 NULL이라는 것은 어떤 SizeClass의 FreePageList에도 달리지 않았다.
#define SMP_SIZECLASSID_NULL     ID_UINT_MAX
// 다중화된 SizeClass FreePageList의 마지막 SizeClassID
#define SMP_LAST_SIZECLASSID(x)  (SMP_SIZE_CLASS_COUNT(x) - 1)
// 마지막 SizeClass는 EmptyPage에 대한 SizeClass이다.
#define SMP_EMPTYPAGE_CLASSID(x) SMP_LAST_SIZECLASSID(x)
// Tx's Private Free Page List에 해당하는 PageListID
#define SMP_PRIVATE_SIZECLASSID (ID_UINT_MAX - 1)

// 프로퍼티에 정의한 MIN_PAGES_ON_TABLE_FREE_LIST만큼 항상 PageList에 남기게 된다.
#define SMP_FREEPAGELIST_MINPAGECOUNT (smuProperty::getMinPagesOnTableFreeList())
// PageList에 남기고 싶은만큼씩 FreePagePool에서 FreePageList로 가져온다.
#define SMP_MOVEPAGECOUNT_POOL2LIST   (smuProperty::getMinPagesOnTableFreeList())
// DB에서 Page를 할당받을때 FreePageList에 필요한 만큼씩 가져온다.
#define SMP_ALLOCPAGECOUNT_FROMDB     (smuProperty::getAllocPageCount())

/* smpFixedPageList::allocSlot의 Argument */
#define  SMP_ALLOC_FIXEDSLOT_NONE           (0x00000000)
// Allocate를 요청하는 Table 의 Record Count를 증가시킨다.
#define  SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT  (0x00000001)
// 할당받은 Slot의 Header를 Update하고 Logging하라.
#define  SMP_ALLOC_FIXEDSLOT_SET_SLOTHEADER (0x00000002)

#endif /* _O_SMP_DEF_H_ */
