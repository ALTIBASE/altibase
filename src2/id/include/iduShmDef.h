/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDU_SHM_DEF_H_)
#define _O_IDU_SHM_DEF_H_ 1

#include <idTypes.h>
#include <iduShmLatch.h>
#include <iduShmSXLatch.h>
#include <iduMemPool.h>
#include <iduShmProcType.h>

#define IDU_SHM_REMOVE_SHM_SEG_RETRY_TIMES           (120)
#define IDU_SHM_SND_LVL_IDX_COUNT_LOG2               (5)

#define IDU_SHM_ADDR_OFFSET_BIT_SIZE                 (iduShmMgr::mOffsetBitSize4ShmAddr)

#define IDU_SHM_CHUNK_MIN_SIZE                       (128*1024*1024)

# define IDU_SHM_NULL_ADDR                           ID_ULONG(0xFFFFFFFFFFFFFFFF)
# define IDU_SHM_PERS_ADDR                           ID_ULONG(0x8000000000000000)

# define IDU_SHM_MAKE_SHM_ADDR( aShmSegID, aOffset )    \
    ( ((ULong)(aShmSegID)) << IDU_SHM_ADDR_OFFSET_BIT_SIZE | (aOffset) )

# define IDU_SHM_MAKE_PERS_ADDR(aShmSegId, aOffset) \
    ( IDU_SHM_PERS_ADDR | ((ULong)(aShmSegId)) << 32 | (aOffset) )

#define IDU_SHM_ADDR_OFFSET_MAX_SIZE                 (ID_ULONG(1) << IDU_SHM_ADDR_OFFSET_BIT_SIZE)

#define IDU_SHM_ADDR_OFFSET_MASK                     (IDU_SHM_ADDR_OFFSET_MAX_SIZE - ID_ULONG(1))
#define IDU_SHM_ADDR_MAX_BLOCK_SIZE                  (IDU_SHM_ADDR_OFFSET_MASK)

#define IDU_SHM_GET_ADDR_SEGID( aShmAddr )           ( aShmAddr >> IDU_SHM_ADDR_OFFSET_BIT_SIZE )
#define IDU_SHM_GET_ADDR_OFFSET( aShmAddr )          ( aShmAddr & IDU_SHM_ADDR_OFFSET_MASK )
#define IDU_SHM_GET_ADDR_PTR( aShmAddr )            (iduShmMgr::getPtrOfAddr( aShmAddr ))
#define IDU_SHM_GET_ADDR_PTR_CHECK( aShmAddr )       ( iduShmMgr::getPtrOfAddrCheck( aShmAddr ) )

/* IDU_SHM_ADDR_FMT and IDU_SHM_ADDR_ARGS are convenience macros that
 * can be used together to output idShmAddr in the "SEGMENT:OFFSET"
 * format.  The output is formatted hexadecimal, to make things like
 * IDU_SHM_NULL_ADDR look more obvious.  (e.g. 0xffff:0xffff) */
#define IDU_SHM_ADDR_FMT  "%#lx:%#lx"
#define IDU_SHM_ADDR_ARGS( aShmAddr ) IDU_SHM_GET_ADDR_SEGID( aShmAddr ), \
        IDU_SHM_GET_ADDR_OFFSET( aShmAddr )

#define IDU_SHM_GET_ADDR_SUBMEMBER( aParentAddr, aStruct, aMember ) \
    ((aParentAddr) + (ULong)offsetof(aStruct, aMember))

#define IDU_SHM_VALIDATE_ADDR_PTR( aAddr, aPtr )      IDE_DASSERT( (SChar*)(aPtr) == (SChar*)IDU_SHM_GET_ADDR_PTR( aAddr ) )

#if defined(COMPILE_64BIT)
#define IDU_SHM_ALIGN_SIZE_LOG2      (3)
#else
#define IDU_SHM_ALIGN_SIZE_LOG2      (2)
#endif

#define IDU_SHM_ALIGN_SIZE           (1 << IDU_SHM_ALIGN_SIZE_LOG2)

#if defined(COMPILE_64BIT)
#define IDU_SHM_FST_LVL_INDEX_MAX    (30)
#else
#define IDU_SHM_FST_LVL_INDEX_MAX    (30)
#endif

#define IDU_SHM_SND_LVL_INDEX_COUNT  ( 1 << IDU_SHM_SND_LVL_IDX_COUNT_LOG2 )
#define IDU_SHM_FST_LVL_INDEX_SHIFT  ( IDU_SHM_SND_LVL_IDX_COUNT_LOG2 + IDU_SHM_ALIGN_SIZE_LOG2 )
#define IDU_SHM_FST_LVL_INDEX_COUNT  ( IDU_SHM_FST_LVL_INDEX_MAX - IDU_SHM_FST_LVL_INDEX_SHIFT + 1 )
#define IDU_SHM_SMALL_BLOCK_SIZE     ( 1 << IDU_SHM_FST_LVL_INDEX_SHIFT )

#define IDU_SHM_FST_LVL_BITMAP_SIZE  (24)
#define IDU_SHM_SND_LVL_BITMAP_SIZE  (32)

#define IDU_SHM_MIN_BLOCK_SIZE  (ID_SIZEOF(iduShmFreeBlkList))
#define IDU_SHM_SIGNATURE       ("ALTIBASE XDB $$AA$$B$CC")
#define IDU_SHM_SHM_KEY_NULL    ((SInt)(0xFFFFFFFF))

typedef enum iduShmState
{
    IDU_SHM_STATE_INVALID = 0,
    IDU_SHM_STATE_CREATE,
    IDU_SHM_STATE_INITIALIZE
} iduShmState;

typedef enum iduShmType
{
    IDU_SHM_TYPE_SYSTEM_SEGMENT = 0,
    IDU_SHM_TYPE_DATA_SEGMENT,
    IDU_SHM_TYPE_DATABASE
} iduShmType;

/* Shared Memory Control Header */
typedef struct iduSCH
{
    key_t         mShmKey;
    PDL_HANDLE    mShmID;
    UInt          mSize;
    void        * mChunkPtr;
} iduSCH;

/*
 * Structure describes list of free blocks
 */
typedef struct iduShmFreeBlkList
{
    idShmAddr mAddrPrev;
    idShmAddr mAddrNext;
} iduShmFreeBlkList;

typedef enum iduShmAllocType
{
    PERSISTENT,
    PROVISIONAL
} iduShmAllocType;

/*
** Block header structure.
**
** There are several implementation subtleties involved:
** - The prev_phys_block field is only valid if the previous block is free.
** - The prev_phys_block field is actually stored at the end of the
**   previous block. It appears at the beginning of this structure only to
**   simplify the implementation.
** - The next_free / prev_free fields are only valid if the block is free.
*/
typedef struct iduShmBlockHeader
{
    iduShmAllocType   mType;
    /* The size is stored in bytes */
    /* bit 0 indicates whether the block is used and */
    /* bit 1 allows to know whether the previous block is free */
    UInt              mSize;
    idShmAddr         mAddrSelf;

    iduShmFreeBlkList mFreeList;
} iduShmBlockHeader;

typedef union iduShmBlockFooter
{
    idShmAddr         mHeaderAddr;

#ifdef DEBUG
    /* Used to detect buffer overflow in DEBUG mode. */
    vULong            mFence;
#endif
} iduShmBlockFooter;

#define IDU_SHM_ALLOC_BLOCK_HEADER_SIZE                 \
    ( ULong(offsetof(iduShmBlockHeader, mFreeList)) )

#define IDU_SHM_GET_BLOCK_HEADER_PTR( aAddr )           \
    ( ((aAddr) == IDU_SHM_NULL_ADDR) ? NULL : IDU_SHM_GET_ADDR_PTR((aAddr)-IDU_SHM_ALLOC_BLOCK_HEADER_SIZE) )

/* iduShmBlockHeader의 mSize 하위 2Bit는 각각
 * # 1번째 Bit: 해당 Block의 Free or Used 유무
 * # 2번째 Bit: 해당 Block의 이전 Block의 Free or Used 유무 */
#define IDU_SHM_BLOCK_SIZE_FREE_BIT      (0x00000001)
#define IDU_SHM_BLOCK_SIZE_PREV_FREE_BIT (0x00000002)

/* iduShmBlockHeader의 mSize의 하위 2Bit를 뺀 나머지 값으로 부터
 * 실제 Block의 Size값을 가져온다. */
#define IDU_SHM_GET_BLOCK_SIZE(aBlock)                                  \
    (((iduShmBlockHeader*)(aBlock))->mSize &                            \
     ~( IDU_SHM_BLOCK_SIZE_FREE_BIT | IDU_SHM_BLOCK_SIZE_PREV_FREE_BIT ))

/* iduShmBlockHeader의 mSize에 Block의 크기를 설정한다. */
#define IDU_SHM_SET_BLOCK_SIZE(aBlock, aSize)                                          \
do                                                                                     \
{                                                                                      \
    const UInt sOldSize = ((iduShmBlockHeader*)(aBlock))->mSize;                       \
    ((iduShmBlockHeader*)(aBlock))->mSize = (aSize) |                                  \
        (sOldSize & (IDU_SHM_BLOCK_SIZE_FREE_BIT | IDU_SHM_BLOCK_SIZE_PREV_FREE_BIT)); \
}while(0)

/* 각 Free Block List의 Block들은 Null Block을 마지막으로 가리키고 있다.
 * Null Block의 크기는 0이다. */
#define IDU_SHM_IS_BLOCK_LAST(aBlock)         (IDU_SHM_GET_BLOCK_SIZE(aBlock) == 0)

/* Shared Memory Block이 할당시 Meta정보를 위해 사용되는 Memory 크기 */
#define IDU_SHM_BLOCK_OVERHEAD                (ID_SIZEOF(UInt)     /*mSize*/   + \
                                               ID_SIZEOF(UInt)     /*Align*/   + \
                                               ID_SIZEOF(idShmAddr)/*mAddrSelf*/ + \
                                               ID_SIZEOF(iduShmBlockFooter))
/* Block의 시작 Offset */
#define IDU_SHM_BLOCK_START_OFFSET            (offsetof(iduShmBlockHeader, mAddrSelf) + ID_SIZEOF(idShmAddr))

/* Shared Memory Block의 최소 크기 */
#define IDU_SHM_BLOCK_SIZE_MIN                (ID_SIZEOF(iduShmBlockHeader) - ID_SIZEOF(idShmAddr) \
                                               + ID_SIZEOF(iduShmBlockFooter))
/* Shared Memory Block의 최대 크기 */
#define IDU_SHM_BLOCK_SIZE_MAX                (1 << IDU_SHM_FST_LVL_INDEX_MAX)

/* 모든 Shared Memory Chunk가 가지는 Header */
typedef struct iduShmHeader
{
    /* Altibase가 생성한 Chunk인지를 구분하기 위해 생성 */
    SChar             mSigniture[128];
    /* Segment ID */
    UInt              mSegID;
    /* Segment의 크기 */
    UInt              mSize;
    /* Segment에 할당된 Shared Memory Key */
    key_t             mShmKey;
    /* Segment의 상태 */
    iduShmState       mState;
    /* Segment의 종류 */
    UInt              mType;
    /* Segment를 생성한 XDB Daemon Process가 Starup된 시간 */
    struct timeval    mStartUpTime;

    /* Segment가 생성된 시간 */
    struct timeval    mCreateTime;

    /* Shared Memory를 생성한 Binary의 Version */
    SChar             mVersion[24];
    UInt              mVersionID;
} iduShmHeader;

#define IDU_SHM_DATA_SEG_OVERHEAD  ( IDU_SHM_BLOCK_START_OFFSET + IDU_SHM_BLOCK_OVERHEAD * 2 + \
                                     alignUp( ID_SIZEOF(iduShmHeader), IDU_SHM_ALIGN_SIZE ) )

/* 현재 System에 할당된 Segment의 정보로 Shared Memory에 저장되어 있다. */
typedef struct iduStShmSegInfo
{
    /* Shared Memory Segment의 Shared Memory Key */
    key_t           mShmKey;
    /* Segment의 크기 */
    UInt            mSize;
} iduStShmSegInfo;

/* Shared Memory의 통계 정보 */
typedef struct iduShmStatistics
{
    /* 총 크기(Byte) */
    UInt      mTotalSize;
    /* 현재 할당된 Memory 크기(Byte) */
    UInt      mAllocSize;
    /* Free상태인 Memory 크기(Byte) */
    UInt      mFreeSize;

    /* Free상태의 Block의 갯수 */
    UInt      mFreeBlkCnt;
    /* 할당된 Block의 갯수 */
    UInt      mAllocBlkCnt;

    /* Memory Allocation Request의 총횟수 */
    ULong     mAllocReqCnt;
    /* Memory Free Request의 총횟수 */
    ULong     mFreeReqCnt;
} iduShmStatistics;

#define IDU_SHM_FREE_SEGINFO_NULL (0xFFFFFFFFFFFFFFFF)
#define IDU_SHM_SYS_SEGMENT_ID    (0)

typedef enum iduShmMetaBlockType
{
    IDU_SHM_META_PERSISTENT,
    IDU_SHM_META_TRANSMGR_BLOCK,
    IDU_SHM_META_TRANS_BLOCK,
    IDU_SHM_META_LOCKMGR_BLOCK,
    IDU_SHM_META_RECVMGR_BLOCK,
    IDU_SHM_META_LFGMGR_BLOCK,
    IDU_SHM_META_DPMGR_BLOCK,
    IDU_SHM_META_CTMGR_BLOCK, // PROJ-2488
    IDU_SHM_META_BIMGR_BLOCK, // PROJ-2488
    IDU_SHM_META_BACKUPMGR_BLOCK,
    IDU_SHM_META_CHKPTMGR_BLOCK,
    IDU_SHM_META_SMCLOB_STATISTICS_BLOCK,
    IDU_SHM_META_ARCHIVE_THREAD_BLOCK,
    IDU_SHM_META_BTREE_BLOCK,
    IDU_SHM_META_LOGICAL_AGER_BLOCK,
    IDU_SHM_META_DELETE_THREAD_BLOCK,
    IDU_SHM_META_STATISTICS_THREAD_BLOCK,
    IDU_SHM_META_SMM_FPLMGR_BLOCK,
    IDU_SHM_META_SMPTABLE_BLOCK,
    IDU_SHM_META_SVM_MGR_BLOCK,
    IDU_SHM_META_SVM_FPLMGR_BLOCK,
    IDU_SHM_META_MM_SESSINFO_BLOCK,
    IDU_SHM_META_MM_STMTINFOMGR_BLOCK, // PROJ-2348
    IDU_SHM_META_MM_PROPERTIES_BLOCK,
    IDU_SHM_META_MM_SHARED_FLAGS_BLOCK,
    IDU_SHM_META_RP_PROPERTIES_BLOCK,
    IDU_SHM_META_SM_SHM_PROPERTY_BLOCK,
    IDU_SHM_META_WATCHDOG_BLOCK,
    IDU_SHM_META_TSM_TEST,
    IDU_SHM_META_ID_PROPERTIES_BLOCK,
    IDU_SHM_META_DK_PROPERTIES_BLOCK,
    IDU_SHM_META_QP_PROPERTIES_BLOCK,
    IDU_SHM_META_MT_PROPERTIES_BLOCK,
 // persistent shared memory를 이용하는 meta block들..
    IDU_SHM_META_SM_CATALOGCACHE_BLOCK,
    IDU_SHM_META_TBSMGR_BLOCK,
    IDU_SHM_META_SM_LOB_BLOCK,
    IDU_SHM_META_IDXMGR_BLOCK,
    IDU_SHM_META_DATABASE_BLOCK,
    IDU_SHM_META_FIXEDMEMMGR_BLOCK,
    IDU_SHM_META_SMMMGR_BLOCK,
    
    IDU_SHM_META_MAX_COUNT
} iduShmMetaBlockType;

#define IDU_LOG_BUFFER_SIZE     (32*1024)
#define IDU_MAX_PROCESS_COUNT   (1024)

typedef struct iduShmMemPoolState
{
    iduMemoryClientIndex mWhere;
    UInt                 mMemListCnt;

    vULong               mFreeCnt;
    vULong               mFullCnt;
    vULong               mPartialCnt;
    vULong               mChunkLimit;
    vULong               mChunkSize;
    vULong               mElemCnt;
    vULong               mElemSize;
    SChar                mName[IDU_MEM_POOL_NAME_LEN];
} iduShmMemPoolState;

typedef struct iduShmListNode
{
    idShmAddr mAddrSelf;
    idShmAddr mAddrPrev;
    idShmAddr mAddrNext;
    idShmAddr mAddrData;
} iduShmListNode;

#define IDU_SHM_THR_LOGGING_MASK (0x00000001)
#define IDU_SHM_THR_LOGGING_ON   (0x00000001)
#define IDU_SHM_THR_LOGGING_OFF  (0x00000000)

#define IDU_SHM_THR_DEFAULT_ATTR (IDU_SHM_THR_LOGGING_ON)

typedef struct iduShmLatchInfo
{
    void              * mObject;
    idShmAddr           mAddr4Latch;
    iduShmSXLatchMode   mMode;
    UInt                mLSN4Latch;
    iduShmLatchState    mLatchOPState;
    SInt                mOldSLatchCnt;
} iduShmLatchInfo;

#define IDU_MAX_SHM_LATCH_STACK_SIZE   (128)
#define IDU_LATCH_STACK_NULL_IDX       (ID_UINT_MAX)

#define IDU_SHM_LATCH_STACK_TOP( aStack ) ( (aStack)->mArray[ (aStack)->mCurSize - 1 ] )
#define IDU_SHM_LATCH_STACK_POP( aStack ) ( (aStack)->mCurSize--, (aStack)->mArray[ (aStack)->mCurSize ] )
#define IDU_SHM_LATCH_STACK_POP_DISCARD( aStack ) { (aStack)->mCurSize--; }
#define IDU_SHM_LATCH_STACK_GET( aStack, i ) ( (aStack)->mArray[ i ] )
#define IDU_SHM_LATCH_STACK_IS_EMPTY( aStack ) ( (aStack)->mCurSize == 0 )

typedef struct iduShmLatchStack
{
    UInt              mCurSize;
    iduShmLatchInfo   mArray[IDU_MAX_SHM_LATCH_STACK_SIZE];
} iduShmLatchStack;

typedef enum iduShmThrState
{
    IDU_SHM_THR_STATE_RUN,
    IDU_SHM_THR_STATE_WAIT
} iduShmThrState;

typedef struct iduShmTxInfo
{
    idShmAddr           mSelfAddr;
    UInt                mFlag;
    iduShmListNode      mNode;

    // ShmTx에 PendingOperation 수행을 위한 List
    iduShmListNode      mPendingOpList;
    /* PendingOperation의 제거를 위한 FreeList로 Node를 remove하면
     * 이 List에 매달렸다가 free 또는 commit 시점시 제거된다. */
    iduShmListNode      mPendingOpFreeList;
    idLPID              mLPID;
    idGblThrID          mThrID;
    iduShmLatchStack    mLatchStack;
    SChar               mLogBuffer[IDU_LOG_BUFFER_SIZE];
    UInt                mLogOffset;
    iduShmThrState      mState;
    iduShmLatchStack    mSetLatchStack;
} iduShmTxInfo;

typedef enum iduShmProcState
{
    IDU_SHM_PROC_STATE_NULL,
    IDU_SHM_PROC_STATE_INIT,
    IDU_SHM_PROC_STATE_RUN,
    IDU_SHM_PROC_STATE_DEAD,
    IDU_SHM_PROC_STATE_RECOVERY
} iduShmProcState;

typedef IDE_RC (*iduProcRestartFunc)( idvSQL *aStatistics );

typedef struct iduShmProcInfo
{
    idShmAddr             mSelfAddr;
    ULong                 mPID;
    iduShmProcState       mState;
    UInt                  mLPID;
    iduShmProcType        mType;
    UInt                  mThreadCnt;
    iduShmTxInfo          mMainTxInfo;
    key_t                 mSemKey;
    SInt                  mSemID;
    iduShmLatch           mLatch;
    iduProcRestartFunc    mRestartProcFunc;
    iduShmListNode        mStStatementList;
} iduShmProcInfo;

#define IDU_SHM_PROC_REGISTER_MASK (0x00000001)
#define IDU_SHM_PROC_REGISTER_ON   (0x00000001)
#define IDU_SHM_PROC_REGISTER_OFF  (0x00000000)

typedef struct iduShmProcMgrInfo
{
    idShmAddr       mAddrSelf;
    UInt            mCurProcCnt;
    UInt            mFlag;
    iduShmProcInfo  mPRTable[IDU_MAX_PROCESS_COUNT];
} iduShmProcMgrInfo;

/* System Segment의 Header */
typedef struct iduShmSSegment
{
    /* Segment Header */
    iduShmHeader         mHeader;

    /* 특정 정보를 가지고 있는 Meta Block의 ShmAddr의 Array */
    idShmAddr            mArrMetaBlockAddr[IDU_SHM_META_MAX_COUNT];

    /* Shared Memory 통계 정보 */
    iduShmStatistics     mStatistics;

    /* Block할당시 잡게되는 Latch이다. */
    iduShmLatch          mLatch4AllocBlk;

    /* First Level Bitmap: 각 Bit는 해당 열에 Free Block이 있고 없고를 나타낸다. */
    UInt                 mFstLvlBitmap;

    /* Second Level Bitmap: 2차원 Bit 배열의 Bit는 Mapping되는 mFBMatrix에 배열의
     * Slot에 Free Block이 있는지 없는지를 나타낸다. */
    UInt                 mSndLvlBitmap[IDU_SHM_SND_LVL_INDEX_COUNT];
    /* Free Block Matrix: Free Block Linked List를 가지고 있다. */
    idShmAddr            mFBMatrix[IDU_SHM_FST_LVL_INDEX_COUNT][IDU_SHM_SND_LVL_INDEX_COUNT];

    /* First Level Bitmap: 각 Bit는 해당 열에 Free Block이 있고 없고를 나타낸다. */
    UInt                 mPersFstLvlBitmap;

    /* Second Level Bitmap: 2차원 Bit 배열의 Bit는 Mapping되는 mFBMatrix에 배열의
     * Slot에 Free Block이 있는지 없는지를 나타낸다. */
    UInt                 mPersSndLvlBitmap[IDU_SHM_SND_LVL_INDEX_COUNT];
    /* Free Block Matrix: Free Block Linked List를 가지고 있다. */
    idShmAddr            mPersFBMatrix[IDU_SHM_FST_LVL_INDEX_COUNT][IDU_SHM_SND_LVL_INDEX_COUNT];

    /* Daemon이 Shm에 Attach하려면, 정상적으로 Shutdown되었어야 함. */
    idBool               mIsNormalShutdown;

    /* Segment Size */
    UInt                 mSegSize;
    /* 최대 Segment의 갯수 */
    ULong                mMaxSegCount;
    /* 현재 Segment의 갯수 */
    ULong                mSegCount;
    /* 다음으로 할당될 Shared Memory Key */
    key_t                mNxtShmKey;

    /* BUG-40895 Cannot destroy the shared memory segments which is created as other user */
    uid_t                mUID;

    iduShmBlockHeader    mNullBlock;

    /* Process 정보가 할당되어야만 Shared Memory Manager접근이 가능하기때문에
     * System Segment에 미리 넣어둔다. */
    iduShmProcMgrInfo    mProcMgrInfo;

    /* 새로운 Segment할당된 잡게되는 Latch이다. */
    iduShmLatch          mLatch4AllocSeg;
    /* ArrSegInfo에서 첫번째 Free Slot의 Index */
    ULong                mFstFreeSegInfoIdx;
    /* System Segment가 처음 생성될때 Segment Info Array가 생성된다. */
    iduStShmSegInfo      mArrSegInfo[1];
} iduShmSSegment;

#define IDU_SPIN_WAIT_DEFAULT_SPIN_COUNT 100000

#define IDU_SPIN_WAIT_SLEEP_TIME_MIN     200
#define IDU_SPIN_WAIT_SLEEP_TIME_MAX     100000

#endif /* _O_IDU_SHM_DEF_H_ */
