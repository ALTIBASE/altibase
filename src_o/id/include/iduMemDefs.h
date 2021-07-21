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
 
/***********************************************************************
 * $Id:$
 **********************************************************************/

#ifndef _O_IDU_MEM_DEFS_H_
#define _O_IDU_MEM_DEFS_H_ 1

#include <idTypes.h>
#include <ide.h>

#define IDU_MEM_PREPARE_STMT_MUTEX_COUNT    256

/// 메모리 할당을 요청하는 모듈을 구분하기 위한 번호
typedef enum
{
    IDU_MIN_CLIENT_COUNT = 0,
    IDU_MEM_MMA = IDU_MIN_CLIENT_COUNT,
    IDU_MEM_MMC,
    IDU_MEM_MML_MAIN,
    IDU_MEM_MML_QP,
    IDU_MEM_MML_ST,
    IDU_MEM_MML_CD,
    IDU_MEM_MML_CDB,
    IDU_MEM_MMD,
    IDU_MEM_MMT,
    IDU_MEM_MMQ,
    IDU_MEM_MMU,
    IDU_MEM_MMPLAN_CACHE_CONTROL,
    IDU_MEM_ST_STD,
    IDU_MEM_ST_STN, // TODO: 중간에 넣어도 되는지 확인할 것
    IDU_MEM_ST_STF,
    IDU_MEM_ST_TEMP,
    IDU_MEM_QCI,
    IDU_MEM_QCM,
    IDU_MEM_QMC,
    IDU_MEM_QMSEQ,
    IDU_MEM_QSC,
//BUG-22355 
//Replication Module IDU_MEM_QRX --> RPC, RPD, RPN, RPR, RPS, RPX 분류
//RPD, RPX는 기능으로 META, SENDER, RECEIVER, SYNC로 분류
    IDU_MEM_RP,
    IDU_MEM_RP_RPC,
    IDU_MEM_RP_RPD,
    IDU_MEM_RP_RPD_META,
    IDU_MEM_RP_RPN,
    IDU_MEM_RP_RPR,
    IDU_MEM_RP_RPS,
    IDU_MEM_RP_RPX,
    IDU_MEM_RP_RPX_SENDER,
    IDU_MEM_RP_RPX_RECEIVER,
    IDU_MEM_RP_RPX_SYNC,
    IDU_MEM_RP_RPU,
    IDU_MEM_QSN,
    IDU_MEM_QSX,
    IDU_MEM_QMP,
    IDU_MEM_QMX,
    IDU_MEM_QMB,
    IDU_MEM_QMT,
    IDU_MEM_QMBC,
    IDU_MEM_QXC,
    IDU_MEM_QRC,
    IDU_MEM_QIX,
    IDU_MEM_MT,
    IDU_MEM_SM_SDB,
    IDU_MEM_SM_SDC,
    IDU_MEM_SM_SDD,
    IDU_MEM_SM_SDS,
    IDU_MEM_SM_SCT,
    IDU_MEM_SM_SCP,
    IDU_MEM_SM_SDN,
    IDU_MEM_SM_SDP,
    IDU_MEM_SM_SDR,
    IDU_MEM_SM_SGM,
    IDU_MEM_SM_SMA,
    IDU_MEM_SM_SMA_LOGICAL_AGER,
    IDU_MEM_SM_SMC,
    IDU_MEM_SM_SMI,
    IDU_MEM_SM_SML,
    IDU_MEM_SM_SMM,
    IDU_MEM_SM_SMN,
    IDU_MEM_SM_FIXED_TABLE,
    IDU_MEM_SM_SMP,
    IDU_MEM_SM_SMR,
    IDU_MEM_SM_SMR_CHKPT_THREAD,
    IDU_MEM_SM_SMR_LFG_THREAD,
    IDU_MEM_SM_SMR_ARCHIVE_THREAD,
    IDU_MEM_SM_SMU,
    IDU_MEM_SM_SMX,
    IDU_MEM_SM_SVR,
    IDU_MEM_SM_SVM,
    IDU_MEM_SM_SVP,
    IDU_MEM_SM_TEMP,
    IDU_MEM_SM_TRANSACTION_TABLE,
    IDU_MEM_SM_TRANSACTION_LEGACY_TRANS,
    IDU_MEM_SM_TRANSACTION_OID_LIST,
    IDU_MEM_SM_TRANSACTION_PRIVATE_BUFFER,
    IDU_MEM_SM_TRANSACTION_SEGMENT_TABLE,
    IDU_MEM_SM_TRANSACTION_DISKPAGE_TOUCHED_LIST,
    IDU_MEM_SM_TABLE_INFO,
    IDU_MEM_SM_INDEX,
    IDU_MEM_SM_LOG,
    IDU_MEM_SM_IN_MEMORY_RECOVERY,
    IDU_MEM_SM_SCC,
    /* BUG-22352: [ID] Memory 분류가 되지 않고 통으로 관리되고 있습니다. */
    IDU_MEM_ID,
    IDU_MEM_ID_IDU,
    IDU_MEM_ID_ASYNC_IO_MANAGER,
    IDU_MEM_ID_MUTEX_MANAGER,
    IDU_MEM_ID_CLOCK_MANAGER,
    IDU_MEM_ID_TIMER_MANAGER,
    IDU_MEM_ID_PROFILE_MANAGER,
    IDU_MEM_ID_SOCKET_MANAGER,
    IDU_MEM_ID_EXTPROC,       // PROJ-1685
    IDU_MEM_ID_EXTPROC_AGENT, // PROJ-1685
    IDU_MEM_ID_AUDIT_MANAGER,
    IDU_MEM_ID_ALTIWRAP,     /* PROJ-2550 PSM Encryption */
    IDU_MEM_ID_THREAD_INFO,
    IDU_MEM_CMB,
    IDU_MEM_CMN,
    IDU_MEM_CMM,
    IDU_MEM_CMT,
    IDU_MEM_CMI, /* BUG-41909 */
    IDU_MEM_DK,
    IDU_MEM_IDUDM,
    IDU_MEM_SM_TBS_FREE_EXTENT_POOL,
    IDU_MEM_ID_COND_MANAGER,
    IDU_MEM_ID_WATCHDOG,
    IDU_MEM_ID_LATCH,
    IDU_MEM_ID_THREAD_STACK,
    IDU_MEM_RCS,
    IDU_MEM_RCC,
    IDU_MEM_QCR,
    IDU_MEM_SD_STMT,    /* BUG-46092 */
    IDU_MEM_SDI,
    IDU_MEM_OTHER,
    IDU_MEM_MAPPED,
    IDU_MEM_RAW,
    IDU_MEM_SHM_META,
    IDU_MEM_RESERVED,
    /* From here, special purpose indices */
    IDU_MAX_CLIENT_COUNT,
    IDU_MEM_ID_CHUNK,
    IDU_MEM_SENTINEL,
    IDU_MEM_ID_MULTI_ERROR_MANAGER, /* TASK-7218 */
    IDU_MEM_UPPERLIMIT
} iduMemoryClientIndex;

// Altibase내의 각 모듈별로 할당받은 메모리의 크기를 관리하는 자료구조
typedef struct iduMemClientInfo
{
    // To Fix BUG-16821 select name from v$memstat 시 공백 행이 나옴
    //   => iduMemoryClientIndex 만 추가하고 mClientInfo를 추가하지 않은 경우를
    //      IDE_DASSERT로 Detect하기 위함
    iduMemoryClientIndex    mClientIndex;
    SLong                   mAllocSize;
    SLong                   mAllocCount;
    SLong                   mMaxTotSize;
    SChar                   mOwner[64];
    SChar                   mModule[64];
    SChar                   mName[64];
#if defined(BUILD_MODE_DEBUG)
    ULong mAccAllocCount[33];
    ULong mAccFreedCount[33];
#endif
} iduMemClientInfo;

#if !defined(BUILD_MODE_DEBUG)

# define IDU_MEM_CLIENTINFO_STRUCT(aIndex, aModule, aName)               \
    { aIndex, ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), "", aModule, aName }

#else

# define IDU_MEM_CLIENTINFO_STRUCT(aIndex, aModule, aName)              \
    {                                                                   \
        aIndex, ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), "", aModule, aName, \
        {                                                               \
            ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),         \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0)      \
                },                                                      \
        {                                                               \
            ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),         \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0),     \
                ID_ULONG(0), ID_ULONG(0), ID_ULONG(0), ID_ULONG(0)      \
                }                                                       \
    }

#endif

#define IDU_MEM_REPEAT_POINTER(aSentence, aFuncName)                        \
    PDL_Time_Value  sTimeOut;                                               \
    PDL_Time_Value  sInterval;                                              \
    IDE_RC          sRC;                                                    \
    IDE_TEST_RAISE(                                                         \
            aSentence != IDE_SUCCESS,                                       \
            EALLOCRETRY);                                                   \
    return sMemPtr;                                                         \
    IDE_EXCEPTION_CONT( EALLOCRETRY );                                      \
    IDE_TEST( aTimeOut == IDU_MEM_IMMEDIATE );                              \
    (void)ideLog::log(IDE_ERR_0, ID_TRC_MEMMGR_RETRY_ALLOC, aFuncName);     \
    if ( aTimeOut > 0 )                                                     \
    {                                                                       \
        sTimeOut.initialize(0, aTimeOut);                                   \
        sInterval.initialize(0, mAllocRetryTime);                           \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        sTimeOut.initialize(1, 0);                                          \
        sInterval.initialize(0, mAllocRetryTime);                           \
    }                                                                       \
    do {                                                                    \
        idlOS::sleep(sInterval);                                            \
        sRC = aSentence;                                                    \
        if ( (sRC == IDE_FAILURE) && (sTimeOut > 0) ) {                     \
            sTimeOut -= sInterval; }                                        \
        else { }                                                            \
    } while( (sRC == IDE_FAILURE) && (sTimeOut < sInterval) );              \
    IDE_TEST( sRC == IDE_FAILURE );                                         \
    return sMemPtr;                                                         \
    IDE_EXCEPTION_END;                                                      \
    IDE_SET( ideSetErrorCode(idERR_ABORT_IDU_MEMORY_ALLOCATION,             \
                aFuncName) );                                               \
    return NULL;


#define IDU_MEM_REPEAT_PHRASE(aSentence, aFuncName)                         \
    PDL_Time_Value  sTimeOut;                                               \
    PDL_Time_Value  sInterval;                                              \
    IDE_RC          sRC;                                                    \
    IDE_TEST_RAISE(                                                         \
            aSentence != IDE_SUCCESS,                                       \
            EALLOCRETRY);                                                   \
    return IDE_SUCCESS;                                                     \
    IDE_EXCEPTION_CONT( EALLOCRETRY );                                      \
    IDE_TEST( aTimeOut == IDU_MEM_IMMEDIATE );                              \
    (void)ideLog::log(IDE_ERR_0, ID_TRC_MEMMGR_RETRY_ALLOC, aFuncName);     \
    if ( aTimeOut > 0 )                                                     \
    {                                                                       \
        sTimeOut.initialize(0, aTimeOut);                                   \
        sInterval.initialize(0, mAllocRetryTime);                           \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        sTimeOut.initialize(1, 0);                                          \
        sInterval.initialize(0, mAllocRetryTime);                           \
    }                                                                       \
    do {                                                                    \
        idlOS::sleep(sInterval);                                            \
        sRC = aSentence;                                                    \
        if ( (sRC == IDE_FAILURE) && (sTimeOut > 0) ) {                     \
            sTimeOut -= sInterval; }                                        \
        else { }                                                            \
    } while( (sRC == IDE_FAILURE) && (sTimeOut < sInterval) );              \
    IDE_TEST( sRC == IDE_FAILURE );                                         \
    return IDE_SUCCESS;                                                     \
    IDE_EXCEPTION_END;                                                      \
    IDE_SET( ideSetErrorCode(idERR_ABORT_IDU_MEMORY_ALLOCATION,             \
                aFuncName) );                                               \
    return IDE_FAILURE;

/*******************************************************************************
 * Project 2408
 * Memory manager block headers and extra definition
 ******************************************************************************/

/*
 * Signature for memory allocator classes
 */
struct iduMemAllocCore
{
    SChar               mName[64];
    SChar               mAddr[64];
    SChar               mType[64];
    ULong               mUsedSize;
    ULong               mPoolSize;
    iduMemClientInfo    mMemInfo[IDU_MEM_UPPERLIMIT];

    iduMemAllocCore*    mPrev;
    iduMemAllocCore*    mNext;
};

class iduMemAlloc : public iduMemAllocCore
{
public:
    /* Initialization functions */
    virtual IDE_RC initialize(SChar* aName);
    virtual IDE_RC destroy(void);

    /* for fixed-size allocators */
    virtual IDE_RC malloc(void**) = 0;
    virtual IDE_RC calloc(void**) = 0;

    /* for variable-size allocators */
    virtual IDE_RC malloc(iduMemoryClientIndex, ULong, void**) = 0;
    virtual IDE_RC malign(iduMemoryClientIndex, ULong, ULong, void**) = 0;
    virtual IDE_RC calloc(iduMemoryClientIndex, vSLong, ULong, void**) = 0;
    virtual IDE_RC realloc(iduMemoryClientIndex, ULong, void**) = 0;
    virtual IDE_RC free(void*) = 0;

    /* for expanding and shrinking memory allocator chunks */
    virtual IDE_RC expand(void) = 0;
    virtual IDE_RC shrink(void) = 0;

    /* for locking/unlocking */
    virtual IDE_RC  lock(void) = 0;
    virtual IDE_RC  unlock(void) = 0;

    virtual ~iduMemAlloc() {};

    /* for memory statistics */
    inline void statupdate(iduMemoryClientIndex  aIndex,
                           SLong                 aSize, 
                           SLong                 aCount)
    {
        ULong sMaxTotalSize;
        ULong sCurTotalSize;

        mUsedSize += aSize;

        mMemInfo[aIndex].mAllocSize     += aSize;
        sCurTotalSize                    = mMemInfo[aIndex].mAllocSize;
        mMemInfo[aIndex].mAllocCount    += aCount;

        //===================================================================
        // To Fix PR-13959
        // 현재까지 사용한 최대 메모리 사용량
        //===================================================================
        sMaxTotalSize = mMemInfo[aIndex].mMaxTotSize;
        if ( sCurTotalSize > sMaxTotalSize )
        {
            mMemInfo[aIndex].mMaxTotSize = sCurTotalSize;
        }
        else
        {
            // Nothing To Do
        }
    }

    inline void updateReserved(void)
    {
        mMemInfo[IDU_MEM_RESERVED].mAllocSize = mPoolSize - mUsedSize;
    }

    /* dump memory status */
    virtual IDE_RC dumpMemory(PDL_HANDLE, UInt) = 0;
};

/*
 * LIBC header
 */
typedef struct iduMemLibcHeader
{
    /* Allocation size of this block */
    ULong mAllocSize;
    /* Memory client index of this block */
    UInt  mClientIndex;
    /* Dummy */
    UInt  mDummy;
} iduMemLibcHeader;

#define IDU_SMALL_MINSIZE   (16)
#define IDU_SMALL_MAXSIZE   (2048)
#define IDU_SMALL_SIZES     (8)

/*
 * TLSF header and definitions
 */
/* Free block */
#define IDU_TLSF_FREE   (0)
#define IDU_TLSF_USED   ((ULong)(0x0001))

#define IDU_TLSF_IS_BLOCK_USED(aBlock)  \
    (((aBlock->mAllocSize & IDU_TLSF_USED) == IDU_TLSF_USED)? ID_TRUE:ID_FALSE)
#define IDU_TLSF_IS_BLOCK_FREE(aBlock)  \
    (((aBlock->mAllocSize & IDU_TLSF_USED) != IDU_TLSF_USED)? ID_TRUE:ID_FALSE)

/* block at the end - cannot coalesce beyond this block */
#define IDU_TLSF_PREVUSED  (ID_ULONG(0x0002))
/* Sentinel block - cannot coalesce beyond this block */
#define IDU_TLSF_SENTINEL  (ID_ULONG(0x0004))
/* Small block - this block is allocated from small allocator */
#define IDU_TLSF_SMALL     (ID_ULONG(0x0008))
/* Allocation Size - with all flags removed */
#define IDU_TLSF_ALLOCSIZE(aBlock) ((aBlock->mAllocSize) & ~(0x0F))

typedef struct iduMemTlsfChunk iduMemTlsfChunk;
class iduMemSmall;
class iduMemTlsf;

typedef struct iduMemSmallHeader
{
    union {
        struct iduMemSmallHeader*   mNext;
        struct {
            UInt    mClientIndex;
            UInt    mDummy;
        } a; /* alloced */
    } h; /* header */
    /* 8bytes */

    /* Allocation size of this block */
    ULong           mAllocSize;
    /* 16bytes so far */
} iduMemSmallHeader;

typedef struct iduMemSmallChunk
{
    iduMemSmall*                mParent;
    struct iduMemSmallChunk*    mNext;
    /* 16bytes so far */
    UInt                        mChunkSize;
    UInt                        mBlockSize;
    /* 24bytes so far */
    iduMemSmallHeader           mFirst;
    /* 32bytes so far */
    /*
     * Memory blocks will start here.
     * All blocks will be aligned by 16bytes
     */
} iduMemSmallChunk;

/* Allocator algorithm for blocks <= 2048bytes */
class iduMemSmall : public iduMemAlloc
{
public:
    /* these functions are not supported
     * for these are fixed-size allocators */
    virtual inline IDE_RC malloc(void**) {return IDE_FAILURE;}
    virtual inline IDE_RC calloc(void**) {return IDE_FAILURE;}
    /* Do not support align for small blocks */
    virtual IDE_RC  malign(iduMemoryClientIndex, ULong, ULong, void**)
    { return IDE_FAILURE; }
    /* Do not support calloc for small blocks */
    virtual IDE_RC  calloc(iduMemoryClientIndex, vSLong, ULong, void**)
    { return IDE_FAILURE; }
    /* Do not support realloc for small blocks */
    virtual IDE_RC  realloc(iduMemoryClientIndex, ULong, void**)
    { return IDE_FAILURE; }
    virtual ~iduMemSmall() {};

public:
    virtual IDE_RC  initialize(SChar*, UInt);
    virtual IDE_RC  destroy();
    virtual IDE_RC  malloc(iduMemoryClientIndex, ULong, void**);
    virtual IDE_RC  free(void*);
    static  IDE_RC  freeStatic(void*);

    virtual IDE_RC  lock(void);
    virtual IDE_RC  unlock(void);

    /* Do not support expand or shrink */
    virtual IDE_RC  expand(void) {return IDE_SUCCESS;}
    virtual IDE_RC  shrink(void) {return IDE_SUCCESS;}

    /* Calculate allocation size */
    UInt            calcAllocSize(ULong, SInt*);
    SInt            calcFL(ULong);
    /* Allocate a chunk by its size*/
    IDE_RC          allocChunk(UInt);
    /* Dump memory status */
    virtual IDE_RC  dumpMemory(PDL_HANDLE, UInt);

    /* Memory chunks */
    iduMemSmallChunk*   mChunks;
    iduMemSmallHeader*  mFrees[IDU_SMALL_SIZES];
    UInt                mChunkSize;

    /* Locks */
#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    PDL_thread_mutex_t	mLock;
#endif
    ULong               mOwner;
    SInt                mLocked;
    SInt                mSpinCount;
};

typedef struct iduMemTlsfHeader
{
    /* Previous block : Just left to this */
    struct iduMemTlsfHeader*        mPrevBlock;
    union {
        struct {
            /* Link for free blocks */
            struct iduMemTlsfHeader* mPrev;
            struct iduMemTlsfHeader* mNext;
        } f; /* used when free */
        struct {
            /* Information for allocated blocks */
            UInt                mClientIndex;
            UInt                mDummy;
            iduMemTlsf*         mParent;
        } a; /* used when alloc */
    } h;
    /* Allocation size of this block */
    ULong                           mAllocSize;
} iduMemTlsfHeader;

typedef struct iduMemTlsfChunk
{
    /* Size of this chunk */
    ULong               mChunkSize;
    ULong               mDummy;
    /* Link for chunks */
    iduMemTlsfChunk*    mPrev;
    iduMemTlsfChunk*    mNext;
    iduMemTlsfHeader    mSentinel;
    iduMemTlsfHeader    mFirst;
} iduMemTlsfChunk;

#define IDU_TLSF_SIZEGAP    ID_ULONG(32)
#define IDU_TLSF_MINBLOCK   ID_ULONG(64)
#define IDU_TLSF_MAXSMALL   ID_ULONG(8192)
#define IDU_TLSF_MAXNEXT    (ID_ULONG(8192)+ID_ULONG(128))

#define IDU_TLSF_FL         (64)
#define IDU_TLSF_SL         (64)

class iduMemTlsf : public iduMemAlloc
{
public:
    /* these functions are not supported
     * for these are fixed-size allocators */
    virtual inline IDE_RC malloc(void**) {return IDE_FAILURE;}
    virtual inline IDE_RC calloc(void**) {return IDE_FAILURE;}
    /* do not support calloc, realloc by this class itself */
    virtual IDE_RC  calloc(iduMemoryClientIndex, vSLong, ULong, void**)
    { return IDE_FAILURE; }
    virtual IDE_RC  realloc(iduMemoryClientIndex, ULong, void**)
    { return IDE_FAILURE; }
    virtual ~iduMemTlsf() {};

public:
    virtual IDE_RC  destroy();
    virtual IDE_RC  malloc(iduMemoryClientIndex, ULong, void**);
    virtual IDE_RC  malign(iduMemoryClientIndex, ULong, ULong, void**);
    virtual IDE_RC  free(void*);
    static  IDE_RC  freeStatic(void*);

    virtual IDE_RC  lock(void);
    virtual IDE_RC  unlock(void);

    virtual IDE_RC  initialize(SChar*, idBool, ULong);

    virtual IDE_RC  expand(void);
    virtual IDE_RC  shrink(void) {return IDE_SUCCESS;}

    /* TLSF algorithms */
    iduMemTlsfHeader*   popBlock(ULong);
    void                insertBlock(iduMemTlsfHeader*, idBool);
    void                removeBlock(iduMemTlsfHeader*, SInt = -1, SInt = -1);
    void                calcFLSL(SInt*, SInt*, ULong);
    ULong               calcAllocSize(ULong);

    static iduMemTlsfHeader*   getHeader(void*);
    iduMemTlsfHeader*   getLeftBlock(iduMemTlsfHeader*);
    iduMemTlsfHeader*   getRightBlock(iduMemTlsfHeader*);

    void                split(iduMemTlsfHeader*,
                              ULong,
                              iduMemTlsfHeader**);
    idBool              mergeLeft(iduMemTlsfHeader*,
                                  iduMemTlsfHeader**);
    idBool              mergeRight(iduMemTlsfHeader* aHeader);

    /* Large blocks */
    virtual IDE_RC      mallocLarge(ULong, void**) = 0;
    virtual IDE_RC      freeLarge(void*, ULong) = 0;

    /* Chunks */
    virtual IDE_RC      allocChunk(iduMemTlsfChunk**) = 0;
    virtual IDE_RC      freeChunk(iduMemTlsfChunk*) = 0;
    /* Dump memory status */
    virtual IDE_RC      dumpMemory(PDL_HANDLE, UInt);

    /* segrational-fit bitmap */
    ULong               mFLBitmap;
    ULong               mSLBitmap[IDU_TLSF_FL];
    /**********************************************************************
     * Segrational-fit free list
     * Row 0~15 contains exact-fit free blocks, with 32bytes of size gap
     * [ 0] :   32,   64,   96, ....,  512
     * [ 1] :  576,  608,  640, ...., 1024
     * [ 2] : 1056, ....
     * ...
     * [15] : 7712, 7744, 7776, ...., 8192
     * Row 16~63 contains segrational-fit free blocks.
     * [16] : 2**13 + 2** 7 + (i + 1) =    8k + 128 ~   16k
     * [17] : 2**14 + 2** 8 + (i + 1) =   16k + 256 ~   32k
     * ...
     * [22] : 2**19 + 2**13 + (i + 1) =  512k +  8k ~ 1024k = 1M
     * [23] : 2**20 + 2**14 + (i + 1) =    1M + 16k ~    2M
     * ...
     * [32] : 2**29 + 2**23 + (i + 1) =  512M +  8M ~ 1024M = 1G
     * [33] : 2**30 + 2**24 + (i + 1) =    1G + 16M ~    2G
     * ...
     * [62] : 2**59 + 2**53 + (i + 1) =  512P +  8P ~ 1024P = 1E
     * [63] : 2**60 + 2**54 + (i + 1) =    1E + 16P ~    2E
     *********************************************************************/
    iduMemTlsfHeader    mFreeList[IDU_TLSF_FL][IDU_TLSF_SL];

    /* Lock */
    PDL_thread_mutex_t  mLock;
    ULong               mOwner;
    SInt                mLocked;
    SInt                mSpinCount;

    /* Stats */
    /* A linked list holding all the existing areas */
    iduMemTlsfChunk*    mChunk;

    /* Chunk size for small blocks */
    ULong               mChunkSize;
    /* Linked list for large blocks */
    iduMemTlsfChunk*    mLargeBlocks;

    idBool              mAutoShrink;
};

/*
 * Allocator class for user created allocator
 */

class iduMemAllocator
{
public:
    iduMemAlloc* mAlloc;

    inline IDE_RC initialize(iduMemAlloc* aAlloc)
    {
        mAlloc = aAlloc;
        return IDE_SUCCESS;
    }
    inline IDE_RC destroy()
    {
        mAlloc = NULL;
        return IDE_SUCCESS;
    }
    inline IDE_RC malloc(void** aPtr)
    {
        return mAlloc->malloc(aPtr);
    }

    inline IDE_RC calloc(void** aPtr)
    {
        return mAlloc->calloc(aPtr);
    }

    inline IDE_RC malloc(iduMemoryClientIndex aIndex, ULong aSize, void** aPtr)
    {
        return mAlloc->malloc(aIndex, aSize, aPtr);
    }

    inline IDE_RC malign(iduMemoryClientIndex aIndex, ULong aSize,
                  ULong aAlign, void** aPtr)
    {
        return mAlloc->malign(aIndex, aSize, aAlign, aPtr);
    }

    inline IDE_RC calloc(iduMemoryClientIndex aIndex, vSLong aCount,
                  ULong aSize, void** aPtr)
    {
        return mAlloc->calloc(aIndex, aCount, aSize, aPtr);
    }

    inline IDE_RC realloc(iduMemoryClientIndex aIndex, ULong aSize, void** aPtr)
    {
        return mAlloc->realloc(aIndex, aSize, aPtr);
    }
    inline IDE_RC free(void*& aPtr)
    {
         IDE_TEST(mAlloc->free(aPtr) != IDE_SUCCESS);
         aPtr = NULL;

         return IDE_SUCCESS;

         IDE_EXCEPTION_END;
         return IDE_FAILURE;
    }
    inline IDE_RC shrink(void)
    {
        return mAlloc->shrink();
    }
};

#endif  // _O_IDU_MEM_DEFS_H_

