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
 
#ifndef _O_SVM_DEF_H_
#define _O_SVM_DEF_H_ 1

#include <idu.h>
#include <iduMutex.h>
#include <smDef.h>
#include <smuList.h>
#include <sctDef.h>


// Tablespace에서 첫 데이터 페이지 아이디
// 0번 페이지 아이디는 SM_NULL_PID이므로 사용을 피한다.
// PCHArray[0]은 m_page가 NULL이다.
// 즉 기존의 meta page(membase)는 없어졌다.
#define SVM_TBS_FIRST_PAGE_ID      (1)

/* PROJ-1490 DB Free Page List
 */
typedef struct svmDBFreePageList
{
    // Free Page List ID
    // svmMemBase.mFreePageLists내에서의 index
    scPageID   mFirstFreePageID ; // 첫번째 Free Page 의 ID
    vULong     mFreePageCount ;   // Free Page 수
} svmDBFreePageList ;

// 최대 Free Page List의 수 
#define SVM_MAX_FPL_COUNT (SM_MAX_PAGELIST_COUNT)

typedef struct svmMemBase
{
    SChar              mDBname[SM_MAX_DB_NAME]; // DB Name
    vULong             mAllocPersPageCount;
    vULong             mExpandChunkPageCnt;    // ExpandChunk 당 Page수
    vULong             mCurrentExpandChunkCnt; // 현재 할당한 ExpandChunk의 수
    UInt               mFreePageListCount;
    svmDBFreePageList  mFreePageLists[SVM_MAX_FPL_COUNT];
} svmMemBase;

/* ----------------------------------------------------------------------------
 *    TempPage Def
 * --------------------------------------------------------------------------*/
#if defined(WRS_VXWORKS)
#undef m_next
#endif
struct svmTempPage;
typedef struct svmTempPageHeader
{
    svmTempPage * m_self;
    svmTempPage * m_prev;
    svmTempPage * m_next;
} svmTempPageHeader;

struct __svmTempPage__
{
    svmTempPageHeader m_header;
    vULong            m_body[1]; // for align calculation 
};

#define SVM_TEMP_PAGE_BODY_SIZE (SM_PAGE_SIZE - offsetof(__svmTempPage__, m_body))

struct svmTempPage
{
    svmTempPageHeader m_header;
#if !defined(COMPILE_64BIT)
    UInt              m_align;
#endif
    UChar             m_body[SVM_TEMP_PAGE_BODY_SIZE];
};
/* ========================================================================*/

typedef  smPCHBase svmPCH;  // BUG-48513

// BUG-43463 에서 사용 scanlist link/unlink 시에
// page가 List에서 제거되거나 연결 될 때 갱신한다.
// 홀수이면 수정중, 짝수이면 수정 완료
#define SVM_PCH_SET_MODIFYING( aPCH ) \
    SMM_PCH_SET_MODIFYING( aPCH );

#define SVM_PCH_SET_MODIFIED( aPCH ) \
    SMM_PCH_SET_MODIFIED( aPCH );              \

// BUG-43463 smnnSeq::moveNext/Prev등의 함수에서 사용
// 현재 page가 link/unlink작업 중인지 확인한다.
#define SVM_PAGE_IS_MODIFYING( aScanModifySeq ) \
    SMM_PAGE_IS_MODIFYING( aScanModifySeq )


/* ------------------------------------------------
 *  CPU CACHE Set : BUGBUG : ID Layer로 옮길수 있음!
 * ----------------------------------------------*/
#if defined(SPARC_SOLARIS)
#define SVM_CPU_CACHE_LINE      (64)
#else
#define SVM_CPU_CACHE_LINE      (32)
#endif
#define SVM_CACHE_ALIGN(data)   ( ((data) + (SVM_CPU_CACHE_LINE - 1)) & ~(SVM_CPU_CACHE_LINE - 1))

/* ------------------------------------------------
 *  OFFSET OF MEMBASE PAGE
 * ----------------------------------------------*/
// BUGBUG!!! : SVM Cannot Refer to SMC. Original Definition is like below.
//
// #define SVM_MEMBASE_OFFSET      SVM_CACHE_ALIGN(offsetof(smcPersPage, m_body))
//
// Currently, we Fix 'offsetof(smcPersPage, m_body)' to 32.
// If smcPersPage is modified, this definition should be checked also.
// Ask to xcom73.
#define SVM_MEMBASE_OFFSET       SVM_CACHE_ALIGN(32)
#define SVM_CAT_TABLE_OFFSET     SVM_CACHE_ALIGN(SVM_MEMBASE_OFFSET + ID_SIZEOF(struct svmMemBase))

/* ------------------------------------------------
   PROJ-1490 페이지리스트 다중화및 메모리 반납 관련
 * ----------------------------------------------*/

// Free Page List 의 수
#define SVM_FREE_PAGE_LIST_COUNT (smuProperty::getPageListGroupCount())

// 최소 몇개의 Page를 가진 Free Page List를 분할 할 것인가?
#define SVM_FREE_PAGE_SPLIT_THRESHOLD                       \
            ( smuProperty::getMinPagesDBTableFreeList() )


// Expand Chunk할당시 각 Free Page에 몇개의 Page씩을 연결할 것인지 결정.
#define SVM_PER_LIST_DIST_PAGE_COUNT                       \
            ( smuProperty::getPerListDistPageCount() )

// 페이지가 부족할 때 한번에 몇개의 Expand Chunk를 할당받을 것인가?
#define SVM_EXPAND_CHUNK_COUNT (1)

// Volatile TBS의 메타 페이지 개수를 지정한다.
#define SVM_TBS_META_PAGE_CNT ((vULong)1)

// PROJ-1490 페이지리스트 다중화및 메모리반납
// Page가 테이블로 할당될 때 해당 Page의 Free List Info Page에 설정될 값
// 서버 기동시 Free Page와 Allocated Page를 구분하기 위한 용도로 사용된다.
#define SVM_FLI_ALLOCATED_PID ( SM_SPECIAL_PID )

/* BUG-31881  smmDef.h의 smmPageReservation와 같은 역할.*/

#define SVM_PAGE_RESERVATION_MAX  (64)
#define SVM_PAGE_RESERVATION_NULL ID_UINT_MAX

typedef struct svmPageReservation
{
    void     * mTrans     [ SVM_PAGE_RESERVATION_MAX ];
    SInt       mPageCount [ SVM_PAGE_RESERVATION_MAX ];
    UInt       mSize;
} svmPageReservation;


// Memory Tablespace Node
// 하나의 Memory Tablespace에 대한 모든 Runtime정보를 지닌다.
typedef struct svmTBSNode
{
    /******** FROM svmTableSpace ********************/
    // Memory / Disk TBS common informations
    sctTableSpaceNode mHeader;

    // Tablespace Attrubute
    smiTableSpaceAttr mTBSAttr;

    /******** FROM svmManager ********************/
    ULong             mDBMaxPageCount;
    
    // BUG-47487: FLI 페이지 memPool ( Volatile )
    iduMemPool        mFLIMemPagePool;
    
    iduMemPool        mMemPagePool;
    iduMemPool        mPCHMemPool;

    // BUG-17216
    // membase를 0번 페이지가 아닌 TBSNode 안에 두도록 수정한다.
    svmMemBase        mMemBase;
    
    /******** FROM svmFPLManager ********************/

    // 각 Free Page List별 Mutex 배열
    // Mutex는 Durable한 정보가 아니기 때문에,
    // membase에 Free Page List의 Mutex를 함께 두지 않는다.
    iduMutex   * mArrFPLMutex;

    // 하나의 트랜잭션이 Expand Chunk를 할당하고 있을 때,
    // 다른 트랜잭션이 Page부족시 Expand Chunk를 또 할당하지 않고,
    // Expand Chunk할당 작업이 종료되기를 기다리도록 하기 위한 Mutex.
    iduMutex     mAllocChunkMutex;
    
    /* BUG-31881 [sm-mem-resource] When executing alter table in MRDB and 
     * using space by other transaction,
     * The server can not do restart recovery. 
     * FreePageListCount만큼 다중화되어 mArrFPLMutex를 통해 제어된다. */
    svmPageReservation * mArrPageReservation;

    /******** FROM svmExpandChunk ********************/

    // Free List Info Page내의 Slot기록을 시작할 위치 
    UInt              mFLISlotBase;
    // Free List Info Page의 수이다.
    UInt              mChunkFLIPageCnt;
    // 하나의 Expand Chunk가 지니는 Page의 수
    // Free List Info Page의 수 까지 포함한 전체 Page 수 이다.
    vULong            mChunkPageCnt;
    
    // PRJ-1548 User Memory Tablespace
    // Loganchor 메모리버퍼상의 TBS Attribute 저장오프셋
    UInt              mAnchorOffset;
    
} svmTBSNode;

/* BUG-32461 [sm-mem-resource] add getPageState functions to smmExpandChunk
 * module */
typedef enum svmPageState
{
    SVM_PAGE_STATE_NONE = 0,
    SVM_PAGE_STATE_META,
    SVM_PAGE_STATE_FLI,
    SVM_PAGE_STATE_ALLOC, // isDataPage = True
    SVM_PAGE_STATE_FREE   // isDataPage = True
} svmPageState;

#endif // _O_SVM_DEF_H_
