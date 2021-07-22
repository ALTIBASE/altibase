/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemMgr.h 85026 2019-03-15 02:16:46Z jayce.park $
 **********************************************************************/

#ifndef _O_IDU_MEM_MGR_H_
#define _O_IDU_MEM_MGR_H_ 1

/**
 * @file
 * @ingroup idu
*/

#include <idl.h>
#include <ide.h>
#include <iduMutex.h>
#include <iduCond.h>
#include <idCore.h>
#include <iduMemDefs.h>

/// 메모리 할당 모드에 대한 옵션 추가
/// - IDU_MEM_FORCE : 할당에 실패하면 무한히 대기하여, 결국 할당에 성공
/// - IDU_MEM_IMMEDIATE : 성공/실패를 즉시 반환한다.
/// 이 외의 경우는 주어진 시간만큼 대기하여 재시도한 후, 그 결과를
/// 반환하도록 한다. 시간 단위는 u-sec이며, 성공/실패 반환
#define IDU_MEM_FORCE     (-1)
#define IDU_MEM_IMMEDIATE (0)

/// 메모리 할당 시도가 실패하였을 경우, 재시도할때까지의 대기시간
/// 시간 단위는 u-sec이며, IDU_MEM_IMMEDIATE일 경우 해당사항 없음
#define IDU_MEM_DEFAULT_RETRY_TIME        (50*1000) // 50 ms

/** 잘못된 메모리 인덱스 */
#define IDU_INVALID_CLIENT_INDEX ID_UINT_MAX

// allocator index for special cases
#define IDU_ALLOC_INDEX_PRIVATE 0xdeaddead
#define IDU_ALLOC_INDEX_LIBC 0xbeefbeef    

typedef enum
{
    IDU_MEMMGR_SINGLE,
    IDU_MEMMGR_CLIENT = IDU_MEMMGR_SINGLE,
    IDU_MEMMGR_LIBC,
    IDU_MEMMGR_TLSF,
    IDU_MEMMGR_INNOCENSE, /*BUG-46568*/
    IDU_MEMMGR_MAX
} iduMemMgrType;

/*
 * Project 2408
 * initializeStatic and destroyStatic for each memmgr type
 */
typedef IDE_RC (*iduMemInitializeStaticFunc)(void);
typedef IDE_RC (*iduMemDestroyStaticFunc)(void);
/*
 * malloc(index, size, pointer)
 * malign(index, size, align, pointer)
 * calloc(index, size, count, pointer)
 * realloc(index, newsize, pointer)
 * free(pointer)
 * shrink(void) - shrinks the memory area of current process
 * buildft(header, dumpobj, table memory)
 *  - export information of all memory blocks
 *  - not usable in single or libc
 */
typedef IDE_RC (*iduMemMallocFunc)(iduMemoryClientIndex, ULong, void **);
typedef IDE_RC (*iduMemMAlignFunc)(iduMemoryClientIndex, ULong, ULong, void **);
typedef IDE_RC (*iduMemCallocFunc)(iduMemoryClientIndex, vSLong, ULong, void **);
typedef IDE_RC (*iduMemReallocFunc)(iduMemoryClientIndex, ULong, void **);
typedef IDE_RC (*iduMemFreeFunc)(void *);
typedef IDE_RC (*iduMemFree4MAlignFunc)(void *, iduMemoryClientIndex aIndex, ULong aSize);

typedef IDE_RC (*iduMemShrinkFunc)(void);

// malloc, calloc, free 함수 포인터를 저장하는 자료구조
typedef struct iduMemFuncType
{
    iduMemInitializeStaticFunc  mInitializeStaticFunc;
    iduMemDestroyStaticFunc     mDestroyStaticFunc;
    iduMemMallocFunc            mMallocFunc;
    iduMemMAlignFunc            mMAlignFunc;
    iduMemCallocFunc            mCallocFunc;
    iduMemReallocFunc           mReallocFunc;
    iduMemFreeFunc              mFreeFunc;
    iduMemFree4MAlignFunc       mFree4MAlignFunc;
    iduMemShrinkFunc            mShrinkFunc;
} iduMemFuncType;

struct idvSQL;

/*
 * TLSF allocator with mmap
 * on expand, shrink, or alloc/free large block,
 * this class will use mmap system call
 */
class iduMemTlsfMmap : public iduMemTlsf
{
public:
    IDE_RC  initialize(SChar*, idBool, ULong);

    virtual IDE_RC allocChunk(iduMemTlsfChunk**);
    virtual IDE_RC freeChunk(iduMemTlsfChunk*);

    virtual IDE_RC mallocLarge(ULong, void**);
    virtual IDE_RC freeLarge(void*, ULong);

    virtual ~iduMemTlsfMmap() {};
};

/*
 * TLSF local allocator
 * on expand, shrink, or alloc/free large block,
 * this class will get the memory block from global TLSF instance
 */
class iduMemTlsfLocal : public iduMemTlsf
{
public:
    IDE_RC  initialize(SChar*, idBool, ULong);

    virtual IDE_RC malloc(iduMemoryClientIndex, ULong, void**);
    virtual IDE_RC malign(iduMemoryClientIndex, ULong, ULong, void**);

    virtual IDE_RC allocChunk(iduMemTlsfChunk**);
    virtual IDE_RC freeChunk(iduMemTlsfChunk*);

    virtual IDE_RC mallocLarge(ULong, void**) {return IDE_FAILURE;}
    virtual IDE_RC freeLarge(void*, ULong)    {return IDE_FAILURE;}

    virtual ~iduMemTlsfLocal() {};
};

/*
 * TLSF global allocator
 * on expand, shrink, or alloc/free large block,
 * this class will allocate chunk only once.
 */
class iduMemTlsfGlobal : public iduMemTlsf
{
public:
    IDE_RC  initialize(SChar*, ULong);
    virtual IDE_RC destroy(void);

    virtual IDE_RC allocChunk(iduMemTlsfChunk**){return IDE_FAILURE;}
    virtual IDE_RC freeChunk(iduMemTlsfChunk*)  {return IDE_FAILURE;}

    virtual IDE_RC mallocLarge(ULong, void**) {return IDE_FAILURE;}
    virtual IDE_RC freeLarge(void*, ULong)    {return IDE_FAILURE;}

    virtual ~iduMemTlsfGlobal() {};
};

///
/// @class iduMemMgr
/// Memory allocators: malloc, calloc, realloc and free.
/// initializeStatic() should be called first to initialize memory allocators.
///
class iduMemMgr
{
public:
    static iduMemClientInfo mClientInfo[IDU_MEM_UPPERLIMIT];

    static idBool               mAutoShrink;
    static PDL_thread_mutex_t   mAllocListLock;
    static iduMemAllocCore      mAllocList;
    static SInt                 mSpinCount;

    static SInt getSpinCount(void) {return mSpinCount;}

    /// check allocator is set as server type
    /// @return ID_TRUE or ID_FALSE
    static idBool isServerMemType(void)
    {
       idBool sRet;

       if( mMemType !=  IDU_MEMMGR_CLIENT )
       {
            sRet = ID_TRUE;
       }
       else
       {
            sRet = ID_FALSE;
       }
       
       return sRet;
    }

    /* Statistics update function */
    static void server_statupdate(iduMemoryClientIndex  aIndex,
                                  SLong                 aSize,
                                  SLong                 aCount);
    static void single_statupdate(iduMemoryClientIndex  aIndex,
                                  SLong                 aSize,
                                  SLong                 aCount);

    static inline idBool isPrivateAvailable(void)
    {
        idBool sRet;

        if(isServerMemType() == ID_TRUE)
        {
            if(mMemType == IDU_MEMMGR_TLSF)
            {
                sRet = ID_FALSE;
            }
            else
            {
                sRet = (mUsePrivateAlloc != 0)? ID_TRUE:ID_FALSE;
            }
        }
        else
        {
            sRet = ID_FALSE;
        }

        return sRet;
    }

    static inline idBool isBlockTracable(void)
    {
        idBool sRet;

        if(isServerMemType() == ID_TRUE)
        {
            if(mMemType == IDU_MEMMGR_TLSF)
            {
                sRet = ID_TRUE;
            }
            else
            {
                sRet = isPrivateAvailable();
            }
        }
        else
        {
            sRet = ID_FALSE;
        }

        return sRet;
    }

    static inline idBool isUseResourcePool(void)
    {
        idBool sRet;

        if(isServerMemType() == ID_TRUE)
        {
            if(mUsePrivateAlloc != 0)
            {
                sRet = ID_TRUE;
            }
            else
            {
                sRet = ID_FALSE;
            }
        }
        else
        {
            sRet = ID_FALSE;
        }

        return sRet;

    }

private:
    static iduMemMgrType    mMemType;
    static iduMemFuncType   mMemFunc[IDU_MEMMGR_MAX];

    // Allocation 실패시, 재시도를 수행할 때까지의 대기시간(u-sec)
    static SLong            mAllocRetryTime;

    // 범위내의 사이즈에 대한 메모리 할당 요청이 들어오면 콜스택을 남김
    static UInt             mLogLevel;
    static ULong            mLogLowerSize;
    static ULong            mLogUpperSize;

    // BUG-20129
    static ULong            mHeapMemMaxSize;

    static UInt             mNoAllocators;
    static idBool           mIsCPU;

    static UInt             mUsePrivateAlloc;
    static ULong            mPrivatePoolSize;

    static UInt             mAllocatorIndex;

public:
    /// initialize class
    /// @param aType execution of application,
    /// @see iduMemMgrType
    static IDE_RC initializeStatic(iduPeerType aType);

    /// destruct class
    static IDE_RC destroyStatic();

    /// get the amount of allocated memory
    /// @return memory size
    static ULong  getTotalMemory();

    /// 모든 메모리를 해지한 후에도 해지가 안된 메모리가 있는지 확인한다.
    /// 만약 해지가 되지 않은 메모리가 존재한다면 메모리 릭이 발생한 것이다.
    /// 릭이 존재한다면 로그에 어떤 클라이언트 인덱스에 어느 크기의
    /// 메모리가 해지되지 않았는지 메시지를 출력한다.
    static IDE_RC logMemoryLeak();

    /// 운영체체로부터 직접 메모리를 할당 및 해제하지만,
    /// time out 기능을 필요로 할때 사용한다.
    /// @param aSize memory size
    /// @param aTimeOut amount of wait time
    static void * mallocRaw(ULong aSize,
                            SLong aTimeOut = IDU_MEM_IMMEDIATE);
    /// 운영체제의 calloc을 호출함
    static void * callocRaw(vSLong aCount,
                            ULong aSize,
                            SLong aTimeOut = IDU_MEM_IMMEDIATE);
    /// 운영체제의 realloc을 호출함
    static void * reallocRaw(void *aMemPtr,
                             ULong aSize,
                             SLong aTimeOut = IDU_MEM_IMMEDIATE);
    /// 운영체제의 free를 호출함
    /// mallocRaw(), callocRaw(), reallocRaw()로 할당받은 메모리는 반드시
    /// freeRaw로 해지해야함
    static void   freeRaw(void*  aMemPtr);

    // iduMemMgr의 메모리 할당 해제 함수들

    /// malloc과 동일한 역할을 하며 통계정보를 갱신한다.
    /// 다음과 같이 클래스의 초기화 타입에 따라 통계 정보 갱신에 차이가 있다.
    /// - IDU_MEMMGR_CLIENT: 통계 정보 갱신을 하지 않는다.
    /// - IDU_MEMMGR_SERVER: 여러개의 쓰레드가 동시에 실행될 수 있도록 동기화된 통계 정보 갱신을 한다.
    /// @param aIndex 메모리를 요청하는 모듈의 인덱스
    /// @param aSize 메모리 크기
    /// @param aMemPtr 할당된 메모리의 포인터를 저장할 변수의 포인터
    /// @param aTimeOut 대기 시간
    /// @param aAlloc 메모리 관리자
    /// @see iduMemMgrType
    static IDE_RC malloc(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         void                 **aMemPtr,
                         SLong                  aTimeOut = IDU_MEM_IMMEDIATE);

    /** malloc과 동일한 역할을 하되
     * aAlign에 위치를 맞춘 메모리 포인터를 리턴한다.
     * @param aIndex 메모리를 요청하는 모듈의 인덱스
     * @param aAlign 메모리 정렬 기준. 2의 거듭제곱값(=2^n)이고 sizeof(void*)의 배수여야 함.
     * @param aSize 메모리 크기
     * @param aMemPtr 할당된 메모리의 포인터를 저장할 변수의 포인터
     * @param aTimeOut 대기 시간
     * @param aAlloc 메모리 관리자
     * @see iduMemMgrType
     */
    static IDE_RC malign(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         ULong                  aAlign,
                         void                 **aMemPtr,
                         SLong                  aTimeOut = IDU_MEM_IMMEDIATE);

    /// calloc과 같음
    /// @see malloc()
    static IDE_RC calloc(iduMemoryClientIndex   aIndex,
                         vSLong                 aCount,
                         ULong                  aSize,
                         void                 **aMemPtr,
                         SLong                  aTimeOut = IDU_MEM_IMMEDIATE);

    /// realloc과 같음
    /// @see malloc()
    static IDE_RC realloc(iduMemoryClientIndex  aIndex,
                          ULong                 aSize,
                          void                **aMemPtr,
                          SLong                 aTimeOut = IDU_MEM_IMMEDIATE);

    /// free와 같음
    /// malloc(), calloc(), realloc()으로 할당받은 메모리는 반드시 free()로 해지되야 한다.
    /// @param aMemPtr 해지할 메모리의 포인터
    /// @param aAlloc 메모리 관리자
    static IDE_RC free(void                 * aMemPtr);

    //BUG-46165
    //this function must be called by the address getting from malign()   not malloc()
    static IDE_RC free4malign(void                  * aMemPtr,
                              iduMemoryClientIndex    aIndex,
                              ULong                   aSize);

    /* Memory management functions with specific allocator */
    static IDE_RC malloc(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc);
    static IDE_RC malign(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         ULong                  aAlign,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc);
    static IDE_RC calloc(iduMemoryClientIndex   aIndex,
                         vSLong                 aCount,
                         ULong                  aSize,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc);
    static IDE_RC realloc(iduMemoryClientIndex  aIndex,
                          ULong                 aSize,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc);
    static IDE_RC free(void*                    aMemPtr,
                       iduMemAllocator*         aAlloc);

    /* Project 2408 */
    static iduMemAlloc* getGlobalAlloc(void) {return mTLSFGlobal;}
    static IDE_RC getSmallAlloc(iduMemSmall**);
    static IDE_RC destroySmallAlloc(iduMemSmall*);

    static IDE_RC getTlsfAlloc(iduMemTlsf**);
    static IDE_RC dumpAllMemory(SChar*, SInt);


    ///
    static IDE_RC createAllocator(iduMemAllocator *,
                                  idCoreAclMemAllocType = ACL_MEM_ALLOC_TLSF,
                                  void* = NULL); // common for all kinds of allocator

    static IDE_RC freeAllocator(iduMemAllocator *aAlloc,
                                idBool aCheckEmpty = ID_TRUE);

    static IDE_RC shrinkAllocator(iduMemAllocator *aAlloc);
    static IDE_RC shrinkAllAllocators(void); // fix BUG-37960

    // BUG-39198
    static UInt   getAllocatorType( void )
    {
        return (UInt)mMemType;
    }

private:
    /*
     * IDU_MEMMGR_SINGLE = IDU_MEMMGR_CLIENT
     * initializeStatic 이전의 single-threaded server 상황,
     * 혹은 클라이언트에서 사용되는 함수들
     * mutex lock을 잡지 않고 모듈별 통계 정보를 기록하지 않는다.
     */
    static IDE_RC single_initializeStatic(void);
    static IDE_RC single_destroyStatic(void);
    static IDE_RC single_malloc(iduMemoryClientIndex   aIndex,
                                ULong                  aSize,
                                void                 **aMemPtr);
    static IDE_RC single_malign(iduMemoryClientIndex   aIndex,
                                ULong                  aSize,
                                ULong                  aAlign,
                                void                 **aMemPtr);
    static IDE_RC single_calloc(iduMemoryClientIndex   aIndex,
                                vSLong                 aCount,
                                ULong                  aSize,
                                void                 **aMemPtr);
    static IDE_RC single_realloc(iduMemoryClientIndex  aIndex,
                                 ULong                 aSize,
                                 void                **aMemPtr);
    static IDE_RC single_free(void                   *aMemPtr);
    static IDE_RC single_free4malign(void                   *aMemPtr,
                                     iduMemoryClientIndex    aIndex,
                                     ULong                   aSize);
    static IDE_RC single_shrink(void);


private:
    /*
     * IDU_MEMMGR_LIBC
     * multi-threaded server에서 사용되는 함수들
     * malloc을 직접 호출하고 모듈별 통계 정보를 기록한다.
     * 메모리 블럭 고정테이블을 조회할 수 없다.
     */
    static IDE_RC libc_initializeStatic(void);
    static IDE_RC libc_destroyStatic(void);
    static IDE_RC libc_malloc(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              void                 **aMemPtr);
    static IDE_RC libc_malign(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              ULong                  aAlign,
                              void                 **aMemPtr);
    static IDE_RC libc_calloc(iduMemoryClientIndex   aIndex,
                              vSLong                 aCount,
                              ULong                  aSize,
                              void                 **aMemPtr);
    static IDE_RC libc_realloc(iduMemoryClientIndex  aIndex,
                               ULong                 aSize,
                               void                **aMemPtr);
    static IDE_RC libc_free(void                   *aMemPtr);
    static IDE_RC libc_free4malign(void                   *aMemPtr,
                                   iduMemoryClientIndex    aIndex,
                                   ULong                   aSize);

    static IDE_RC libc_shrink(void);
    static void   libc_getHeader(void* aMemPtr,
                                 void** aHeader1,
                                 void** aHeader2);

private:
    /*
     * IDU_MEMMGR_TLSF
     * multi-threaded server에서 사용되는 함수들
     * TLSF 메모리 관리자를 생성해 사용하고 모듈별 통계 정보를 기록한다.
     * 메모리 블럭 고정테이블을 조회할 수 있다.
     */
    static IDE_RC tlsf_initializeStatic(void);
    static IDE_RC tlsf_destroyStatic(void);
    static IDE_RC tlsf_malloc(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              void                 **aMemPtr);
    static IDE_RC tlsf_malign(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              ULong                  aAlign,
                              void                 **aMemPtr);
    static IDE_RC tlsf_calloc(iduMemoryClientIndex   aIndex,
                              vSLong                 aCount,
                              ULong                  aSize,
                              void                 **aMemPtr);
    static IDE_RC tlsf_realloc(iduMemoryClientIndex  aIndex,
                               ULong                 aSize,
                               void                **aMemPtr);
    static IDE_RC tlsf_free(void *aMemPtr);

    static IDE_RC tlsf_free4malign(void *aMemPtr,
                                   iduMemoryClientIndex aIndex,
                                   ULong                aSize);
    static IDE_RC tlsf_shrink(void);
    static void   tlsf_getHeader(void* aMemPtr,
                                 void** aHeader1,
                                 void** aHeader2,
                                 void** aTailer);

    /* TLSF Allocators */
    static iduMemTlsfGlobal*    mTLSFGlobal;
    static ULong                mTLSFGlobalChunkSize;

    static iduMemTlsf**         mTLSFLocal;
    static ULong                mTLSFLocalChunkSize;
    static SInt                 mTLSFLocalInstances;

private:
    /*
     * IDU_MEMMGR_INNOCENSE
     */
    static IDE_RC innocense_initializeStatic(void);
    static IDE_RC innocense_destroyStatic(void);
    static IDE_RC innocense_malloc(iduMemoryClientIndex   aIndex,
                                   ULong                  aSize,
                                   void                 **aMemPtr);
    static IDE_RC innocense_malign(iduMemoryClientIndex   aIndex,
                                   ULong                  aSize,
                                   ULong                  aAlign,
                                   void                 **aMemPtr);
    static IDE_RC innocense_calloc(iduMemoryClientIndex   aIndex,
                                   vSLong                 aCount,
                                   ULong                  aSize,
                                   void                 **aMemPtr);
    static IDE_RC innocense_realloc(iduMemoryClientIndex  aIndex,
                                    ULong                 aSize,
                                    void                **aMemPtr);
    static IDE_RC innocense_free(void                   *aMemPtr);
    static IDE_RC innocense_free4malign(void                   *aMemPtr,
                                        iduMemoryClientIndex    aIndex,
                                        ULong                   aSize);

    static IDE_RC innocense_shrink(void);
    static void   innocense_getHeader(void* aMemPtr,
                                      void** aHeader1,
                                      void** aHeader2,
                                      void** aTailer);

    /*
     * BUG-32751
     * Distributing memory allocation into several allocators
     * so that the locks can be dispersed
     */
    static idCoreAclMemAlloc *getAllocator(void **aMemPtr,
                                           UInt aType,
                                           iduMemAllocator *aAlloc);

    static void   logAllocRequest(iduMemoryClientIndex aIndex, ULong aSize, SLong aTimeOut);

    static inline IDE_RC callbackLogLevel(idvSQL*, SChar*, void*, void*, void*);
    static inline IDE_RC callbackLogLowerSize(idvSQL*, SChar*, void*, void*, void*);
    static inline IDE_RC callbackLogUpperSize(idvSQL*, SChar*, void*, void*, void*);

    static UInt getAllocIndex(void);
};

#endif  // _O_IDU_MEM_MGR_H_
