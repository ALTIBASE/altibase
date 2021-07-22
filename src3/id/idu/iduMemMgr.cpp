/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemMgr.cpp 84564 2018-12-10 05:40:03Z kclee $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduMemList.h>
#include <iduMemMgr.h>
#include <idp.h>
#include <iduMemPoolMgr.h>
#include <idCore.h>
#include <iduVarMemList.h>
#include <iduMemory.h>

#define IDU_INVALID_CLIENT_INDEX ID_UINT_MAX

/*
 * mem pool mgr를 만들어야하는가?
 */
#define IDU_MAKE_MEMPOOL_MGR ( (IDU_USE_MEMORY_POOL == 1)                 &&\
                              (iduMemMgr::isServerMemType() == ID_TRUE) )

iduMemMgrType    iduMemMgr::mMemType        = IDU_MEMMGR_SINGLE;

iduMemFuncType   iduMemMgr::mMemFunc[IDU_MEMMGR_MAX] =
{
    /* Single */
    {
        iduMemMgr::single_initializeStatic,
        iduMemMgr::single_destroyStatic,
        iduMemMgr::single_malloc,
        iduMemMgr::single_malign,
        iduMemMgr::single_calloc,
        iduMemMgr::single_realloc,
        iduMemMgr::single_free,
        iduMemMgr::single_free4malign,
        iduMemMgr::single_shrink
    },
    /* LIBC */
    {
        iduMemMgr::libc_initializeStatic,
        iduMemMgr::libc_destroyStatic,
        iduMemMgr::libc_malloc,
        iduMemMgr::libc_malign,
        iduMemMgr::libc_calloc,
        iduMemMgr::libc_realloc,
        iduMemMgr::libc_free,
        iduMemMgr::libc_free4malign,
        iduMemMgr::libc_shrink
    },
    /* TLSF */
    {
        iduMemMgr::tlsf_initializeStatic,
        iduMemMgr::tlsf_destroyStatic,
        iduMemMgr::tlsf_malloc,
        iduMemMgr::tlsf_malign,
        iduMemMgr::tlsf_calloc,
        iduMemMgr::tlsf_realloc,
        iduMemMgr::tlsf_free,
        iduMemMgr::tlsf_free4malign,
        iduMemMgr::tlsf_shrink
    },
    /* INNOCENSE */
    {
        iduMemMgr::innocense_initializeStatic,
        iduMemMgr::innocense_destroyStatic,
        iduMemMgr::innocense_malloc,
        iduMemMgr::innocense_malign,
        iduMemMgr::innocense_calloc,
        iduMemMgr::innocense_realloc,
        iduMemMgr::innocense_free,
        iduMemMgr::innocense_free4malign,
        iduMemMgr::innocense_shrink
    }
};

SLong           iduMemMgr::mAllocRetryTime = IDU_MEM_DEFAULT_RETRY_TIME;

UInt            iduMemMgr::mLogLevel       = 0;
ULong           iduMemMgr::mLogLowerSize   = 0;
ULong           iduMemMgr::mLogUpperSize   = 0;

// BUG-20129
ULong           iduMemMgr::mHeapMemMaxSize = ID_ULONG_MAX;

UInt            iduMemMgr::mNoAllocators = 0;
idBool          iduMemMgr::mIsCPU = ID_FALSE;

UInt            iduMemMgr::mUsePrivateAlloc = 0;
ULong           iduMemMgr::mPrivatePoolSize = 0;
idBool          iduMemMgr::mAutoShrink;
UInt            iduMemMgr::mAllocatorIndex = 0;

PDL_thread_mutex_t  iduMemMgr::mAllocListLock;
iduMemAllocCore     iduMemMgr::mAllocList;

iduMemTlsf**         iduMemMgr::mTLSFLocal;
ULong                iduMemMgr::mTLSFLocalChunkSize;
SInt                 iduMemMgr::mTLSFLocalInstances;
SInt                 iduMemMgr::mSpinCount = 0;

//BUG-21080
static pthread_once_t     gMemMgrInitOnce  = PTHREAD_ONCE_INIT;
static PDL_thread_mutex_t gMemMgrInitMutex;
static SInt               gMemMgrInitCount;

static void iduMemMgrInitializeOnce( void )
{
    IDE_ASSERT(idlOS::thread_mutex_init(&gMemMgrInitMutex) == 0);

    gMemMgrInitCount = 0;
}

IDE_RC iduMemAlloc::initialize(SChar* aName)
{
    SInt i;

    idlOS::strcpy(mName, aName);
    /* initialize meminfo */
    idlOS::memcpy((void*)mMemInfo,
                  (void*)iduMemMgr::mClientInfo,
                  sizeof(iduMemClientInfo)*IDU_MEM_UPPERLIMIT);
    idlOS::snprintf(mAddr, sizeof(mAddr), "%p", this);

    for(i = (SInt)IDU_MIN_CLIENT_COUNT; i < (SInt)IDU_MEM_UPPERLIMIT; i++)
    {
        idlOS::strcpy(mMemInfo[i].mOwner, mName);
        mMemInfo[i].mClientIndex = (iduMemoryClientIndex)i;
    }

    mPoolSize       = 0;
    mUsedSize       = 0;

    IDE_TEST(idlOS::thread_mutex_lock(&(iduMemMgr::mAllocListLock)) != 0);
    mNext = &(iduMemMgr::mAllocList);
    mPrev = iduMemMgr::mAllocList.mPrev;
    mPrev->mNext = this;
    iduMemMgr::mAllocList.mPrev = this;
    IDE_TEST(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) != 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC iduMemAlloc::destroy(void)
{
    IDE_TEST(idlOS::thread_mutex_lock(&(iduMemMgr::mAllocListLock)) != 0);
    mNext->mPrev = mPrev;
    mPrev->mNext = mNext;
    IDE_TEST(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) != 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// iduMemMgr를 초기화.
// IDU_SERVER_TYPE일 경우 mutex를 초기화한다.
IDE_RC iduMemMgr::initializeStatic(iduPeerType aType)
{
#define IDE_FN "iduMemMgr::initializeStatic()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool  sIsLock = ID_FALSE;
    UInt    sAutoShrink;
    UInt    sInstanceMax;
    UInt    sAllocType;

    //BUG-21080   
    idlOS::pthread_once(&gMemMgrInitOnce, iduMemMgrInitializeOnce);

    IDE_ASSERT(idlOS::thread_mutex_lock(&gMemMgrInitMutex) == 0);

    sIsLock = ID_TRUE;
    
    IDE_TEST(gMemMgrInitCount < 0);

    if (gMemMgrInitCount == 0)
    {
        switch(aType)
        {
        case IDU_SINGLE_TYPE:
            mMemType = IDU_MEMMGR_SINGLE;
            break;
        case IDU_CLIENT_TYPE:
            mMemType = IDU_MEMMGR_CLIENT;
            break;
        case IDU_SERVER_TYPE:
            mMemType = IDU_MEMMGR_LIBC;
            break;
        default:
            IDE_ASSERT(0);
            break;
        }
    
        if (aType == IDU_SERVER_TYPE)
        {
            IDE_ASSERT(idp::read("ALLOCATION_RETRY_TIME", &mAllocRetryTime) == IDE_SUCCESS);
    
            IDE_ASSERT(idp::read("MEMMGR_LOG_LEVEL", &mLogLevel) == IDE_SUCCESS);
            IDE_ASSERT(idp::read("MEMMGR_LOG_LOWERSIZE", &mLogLowerSize) == IDE_SUCCESS);
            IDE_ASSERT(idp::read("MEMMGR_LOG_UPPERSIZE", &mLogUpperSize) == IDE_SUCCESS);
    
            // BUG-20129
            IDE_ASSERT(idp::read("__HEAP_MEM_MAX_SIZE", &mHeapMemMaxSize) == IDE_SUCCESS);
    
            IDE_ASSERT(idp::setupAfterUpdateCallback("MEMMGR_LOG_LEVEL",
                                                    callbackLogLevel) == IDE_SUCCESS);
            IDE_ASSERT(idp::setupAfterUpdateCallback("MEMMGR_LOG_LOWERSIZE",
                                                    callbackLogLowerSize) == IDE_SUCCESS);
            IDE_ASSERT(idp::setupAfterUpdateCallback("MEMMGR_LOG_UPPERSIZE",
                                                    callbackLogUpperSize) == IDE_SUCCESS);

            IDE_ASSERT(idp::read("MEMORY_ALLOCATOR_TYPE", &sAllocType) == IDE_SUCCESS);
            IDE_ASSERT(idp::read("MEMORY_ALLOCATOR_DEFAULT_SPINLOCK_COUNT",
                                 &mSpinCount) == IDE_SUCCESS);

#if defined(ALTIBASE_MEMORY_CHECK) || defined(ALTIBASE_USE_VALGRIND)
            mMemType = IDU_MEMMGR_INNOCENSE;
            mUsePrivateAlloc = 0;
#else
            switch(sAllocType)
            {
            case 0:
                mMemType = IDU_MEMMGR_LIBC;
                break;
            case 1:
                mMemType = IDU_MEMMGR_TLSF;
                break;
            default:
                IDE_RAISE(ENOSUCHMEMMGR);
                break;
            }
            // configuration for private allocator
            IDE_TEST(idp::read("MEMORY_ALLOCATOR_AUTO_SHRINK", &sAutoShrink)
                     != IDE_SUCCESS);
            IDE_TEST(idp::read("MEMORY_ALLOCATOR_MAX_INSTANCES", &sInstanceMax)
                     != IDE_SUCCESS);
            IDE_ASSERT(idp::read("MEMORY_ALLOCATOR_USE_PRIVATE",
                                 &mUsePrivateAlloc) == IDE_SUCCESS);
            iduMemMgr::mPrivatePoolSize   = iduProperty::getMemoryPrivatePoolSize();
            iduMemMgr::mAutoShrink        = (sAutoShrink != 0)? ID_TRUE : ID_FALSE;
            iduMemMgr::mNoAllocators      = sInstanceMax;
#endif
            IDE_ASSERT(iduMemory::initializeStatic() == IDE_SUCCESS);
            IDE_ASSERT(iduVarMemList::initializeStatic() == IDE_SUCCESS);

            idlOS::strcpy(mAllocList.mAddr, "LIBC");
            idlOS::strcpy(mAllocList.mName, "LIBC");
            idlOS::strcpy(mAllocList.mType, "LIBC");
            mAllocList.mPoolSize = 0;
            mAllocList.mUsedSize = 0;

            mAllocList.mNext = &mAllocList;
            mAllocList.mPrev = &mAllocList;

            IDE_TEST(idlOS::thread_mutex_init(&(mAllocListLock)) != 0);
        }
        else
        {
        }

        IDE_TEST(mMemFunc[mMemType].mInitializeStaticFunc() != IDE_SUCCESS);
    }

    gMemMgrInitCount++;

    IDE_ASSERT(idlOS::thread_mutex_unlock(&gMemMgrInitMutex) == 0);

    sIsLock = ID_FALSE;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ENOSUCHMEMMGR)
    {
        gMemMgrInitCount = -1;

        if (sIsLock == ID_TRUE)
        {
            IDE_ASSERT(idlOS::thread_mutex_unlock(&gMemMgrInitMutex) == 0);
        }
        else
        {
            /* do nothing */
        }
    }

    IDE_EXCEPTION_END;
    {
        gMemMgrInitCount = -1;

        if (sIsLock == ID_TRUE)
        {
            IDE_ASSERT(idlOS::thread_mutex_unlock(&gMemMgrInitMutex) == 0);
        }
        else
        {
            /* do nothing */
        }
    }         

    return IDE_FAILURE;
#undef IDE_FN
}

// iduMemMgr를 종료.
// IDU_SERVER_TYPE일 경우 mutex를 해제하고 IDU_SINGLE_TYPE으로 전환한다.
IDE_RC iduMemMgr::destroyStatic()
{
    idBool sState = ID_FALSE; //if lock is held.
#define IDE_FN "iduMemMgr::destroyStatic()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //BUG-21080 
    IDE_ASSERT(idlOS::thread_mutex_lock(&gMemMgrInitMutex) == 0);
    sState = ID_TRUE;    

    IDE_TEST(gMemMgrInitCount < 0);

    // call destructor for each algorithm
    IDE_TEST(mMemFunc[mMemType].mDestroyStaticFunc() != IDE_SUCCESS);

    gMemMgrInitCount--;

    if (gMemMgrInitCount == 0)
    {
        if (mMemType != IDU_MEMMGR_SINGLE)
        {
            IDE_TEST(idlOS::thread_mutex_destroy(&(mAllocListLock)) != 0);
            mMemType = IDU_MEMMGR_SINGLE;
        }
    }
    
    sState = ID_FALSE;
    IDE_ASSERT(idlOS::thread_mutex_unlock(&gMemMgrInitMutex) == 0);

    if( IDU_MAKE_MEMPOOL_MGR == ID_TRUE ) 
    {
        IDE_TEST( iduMemPoolMgr::destroy() != IDE_SUCCESS );
    }
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        gMemMgrInitCount = -1;

        if( sState == ID_TRUE )
        {
            IDE_ASSERT(idlOS::thread_mutex_unlock(&gMemMgrInitMutex) == 0);
        }
    }

    return IDE_FAILURE;
#undef IDE_FN
}

void *iduMemMgr::mallocRaw(ULong aSize,
                           SLong aTimeOut)
{
#define IDE_FN "iduMemMgr::mallocRaw()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void* sMemPtr;
    IDU_MEM_REPEAT_POINTER(libc_malloc(IDU_MEM_RAW, aSize, &sMemPtr),
            "iduMemMgr::mallocRaw");
#undef IDE_FN
}

void *iduMemMgr::callocRaw(vSLong   aCount,
                           ULong    aSize,
                           SLong    aTimeOut)
{
#define IDE_FN "iduMemMgr::callocRaw()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void *sMemPtr;
    IDU_MEM_REPEAT_POINTER(
            libc_calloc(IDU_MEM_RAW, aCount, aSize, &sMemPtr),
            "iduMemMgr::callocRaw");
#undef IDE_FN
}

void *iduMemMgr::reallocRaw(void *aMemPtr,
                            ULong aSize,
                            SLong aTimeOut)
{
#define IDE_FN "iduMemMgr::reallocRaw()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void *sMemPtr;
    sMemPtr = aMemPtr;

    IDU_MEM_REPEAT_POINTER(
            libc_realloc(IDU_MEM_RAW, aSize, &sMemPtr),
            "iduMemMgr::reallocRaw");
#undef IDE_FN
}

void iduMemMgr::freeRaw(void* aMemPtr)
{
#define IDE_FN "iduMemMgr::freeRaw()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    IDE_ASSERT(libc_free(aMemPtr) == IDE_SUCCESS);
#undef IDE_FN
}

IDE_RC iduMemMgr::malloc(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         void                 **aMemPtr,
                         SLong                  aTimeOut)
{
    logAllocRequest(aIndex, aSize, aTimeOut);
 
    IDU_MEM_REPEAT_PHRASE(
            mMemFunc[mMemType].mMallocFunc(aIndex, aSize, aMemPtr),
            "iduMemMgr::malloc");
}

IDE_RC iduMemMgr::malign(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         ULong                  aAlign,
                         void                 **aMemPtr,
                         SLong                  aTimeOut)
{
    logAllocRequest(aIndex, aSize, aTimeOut);

    IDU_MEM_REPEAT_PHRASE(
            mMemFunc[mMemType].mMAlignFunc(aIndex, aSize, aAlign, aMemPtr),
            "iduMemMgr::malloc");
}

IDE_RC iduMemMgr::calloc(iduMemoryClientIndex   aIndex,
                         vSLong                 aCount,
                         ULong                  aSize,
                         void                 **aMemPtr,
                         SLong                  aTimeOut)
{
    logAllocRequest(aIndex, aCount * aSize, aTimeOut);

    IDU_MEM_REPEAT_PHRASE(
            mMemFunc[mMemType].mCallocFunc(aIndex, aCount, aSize, aMemPtr),
            "iduMemMgr::calloc");
}

IDE_RC iduMemMgr::realloc(iduMemoryClientIndex  aIndex,
                          ULong                 aSize,
                          void                **aMemPtr,
                          SLong                 aTimeOut)
{
    logAllocRequest(aIndex, aSize, aTimeOut);

    IDU_MEM_REPEAT_PHRASE(
            mMemFunc[mMemType].mReallocFunc(aIndex, aSize, aMemPtr),
            "iduMemMgr::realloc");
}

IDE_RC iduMemMgr::free(void                  * aMemPtr)
{
    IDE_TEST(mMemFunc[mMemType].mFreeFunc(aMemPtr) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::free4malign(void                  * aMemPtr,
                              iduMemoryClientIndex    aIndex,
                              ULong                   aSize)
{
    IDE_TEST(mMemFunc[mMemType].mFree4MAlignFunc(aMemPtr,aIndex,aSize) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* Memory management functions with specific allocator */
IDE_RC iduMemMgr::malloc(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc)
{
    if(aAlloc == NULL || aAlloc->mAlloc == NULL)
    {
        return iduMemMgr::malloc(aIndex, aSize, aMemPtr, aTimeOut);
    }
    else
    {
        logAllocRequest(aIndex, aSize, aTimeOut);
        IDU_MEM_REPEAT_PHRASE(aAlloc->malloc(aIndex, aSize, aMemPtr),
                "iduMemMgr::malloc");
    }
}

IDE_RC iduMemMgr::malign(iduMemoryClientIndex   aIndex,
                         ULong                  aSize,
                         ULong                  aAlign,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc)
{
    if(aAlloc == NULL || aAlloc->mAlloc == NULL)
    {
        return iduMemMgr::malign(aIndex, aSize, aAlign, aMemPtr, aTimeOut);
    }
    else
    {
        logAllocRequest(aIndex, aSize, aTimeOut);
        IDU_MEM_REPEAT_PHRASE(aAlloc->malign(aIndex, aSize, aAlign, aMemPtr),
                "iduMemMgr::malign");
    }
}

IDE_RC iduMemMgr::calloc(iduMemoryClientIndex   aIndex,
                         vSLong                 aCount,
                         ULong                  aSize,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc)
{
    if(aAlloc == NULL || aAlloc->mAlloc == NULL)
    {
        return iduMemMgr::calloc(aIndex, aCount, aSize, aMemPtr, aTimeOut);
    }
    else
    {
        logAllocRequest(aIndex, aCount * aSize, aTimeOut);
        IDU_MEM_REPEAT_PHRASE(aAlloc->calloc(aIndex, aCount, aSize, aMemPtr),
                "iduMemMgr::calloc");
    }
}

IDE_RC iduMemMgr::realloc(iduMemoryClientIndex  aIndex,
                          ULong                 aSize,
                         void**                 aMemPtr,
                         SLong                  aTimeOut,
                         iduMemAllocator*       aAlloc)
{
    if(aAlloc == NULL || aAlloc->mAlloc == NULL)
    {
        return iduMemMgr::realloc(aIndex, aSize, aMemPtr, aTimeOut);
    }
    else
    {
        logAllocRequest(aIndex, aSize, aTimeOut);
        IDU_MEM_REPEAT_PHRASE(aAlloc->realloc(aIndex, aSize, aMemPtr),
                "iduMemMgr::realloc");
    }
}

IDE_RC iduMemMgr::free(void*                    aMemPtr,
                       iduMemAllocator*         aAlloc)
{
    if(aAlloc == NULL || aAlloc->mAlloc == NULL)
    {
        IDE_TEST(iduMemMgr::free(aMemPtr) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(aAlloc->free(aMemPtr) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::callbackLogLevel(
    idvSQL *, SChar  *, void   *, void   * aNewValue, void   * )
{
#define IDE_FN "iduMemMgr::callbackLogLevel()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mLogLevel = *(UInt *)aNewValue;

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC iduMemMgr::callbackLogLowerSize(
    idvSQL *, SChar *, void *, void *aNewValue, void * )
{
#define IDE_FN "iduMemMgr::callbackLogLowerSize()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mLogLowerSize = *(ULong *)aNewValue;

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC iduMemMgr::callbackLogUpperSize(
    idvSQL *, SChar *, void *, void *aNewValue, void * )
{
#define IDE_FN "iduMemMgr::callbackLogUpperSize()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mLogUpperSize = *(ULong *)aNewValue;

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC iduMemMgr::getSmallAlloc(iduMemSmall** aAlloc)
{
    iduMemSmall*    sSmall;
    SChar           sName[64];

    if((mMemType == IDU_MEMMGR_LIBC) || (mUsePrivateAlloc == 0))
    {
        *aAlloc = NULL;
    }
    else
    {
        sSmall = (iduMemSmall*)idlOS::malloc(sizeof(iduMemSmall));
        IDE_TEST(sSmall == NULL);

        sSmall = new(sSmall) iduMemSmall;
        idlOS::snprintf(sName, sizeof(sName),
                        "THREAD_%08X_ALLOC", idlOS::thr_self());
        IDE_TEST_RAISE(sSmall->initialize(sName, mPrivatePoolSize)
                       != IDE_SUCCESS,
                       EINITFAILED);
        *aAlloc = sSmall;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(EINITFAILED)
    {
        idlOS::free(sSmall);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::getTlsfAlloc(iduMemTlsf** aAlloc)
{
#if defined(ALTIBASE_MEMORY_CHECK) || defined(ALTIBASE_USE_VALGRIND)
    *aAlloc = NULL;
#else
    if(mMemType == IDU_MEMMGR_LIBC || mMemType == IDU_MEMMGR_INNOCENSE) //what the XXXX.
    {
        *aAlloc = NULL;
    }
    else
    {
        *aAlloc = mTLSFLocal[getAllocIndex()];
    }
#endif

    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::destroySmallAlloc(iduMemSmall* aAlloc)
{
    if(aAlloc == NULL)
    {
        /* Do nothing and return */
    }
    else
    {
        IDE_TEST(aAlloc->destroy() != IDE_SUCCESS);
        idlOS::free(aAlloc);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::dumpAllMemory(SChar* aDumpTarget, SInt aDumpLevel)
{
    PDL_HANDLE          sFile;
    iduMemAlloc*        sAlloc;
    idBool              sDumpAll;
    SChar*              sDir;
    SChar               sFilename[1024];
    SChar               sLine[1024];
    SInt                sDumped;

    IDE_TEST_RAISE(idp::readPtr( "SERVER_MSGLOG_DIR",
                                 (void **)&sDir)
                   != IDE_SUCCESS, EPROPERTY);

    sDumpAll = (idlOS::strcmp(aDumpTarget, "ALL") == 0)?
        ID_TRUE : ID_FALSE;

    IDE_TEST(idlOS::thread_mutex_lock(&(iduMemMgr::mAllocListLock)) != 0);

    sAlloc  = (iduMemAlloc*)iduMemMgr::mAllocList.mNext;
    sDumped = 0;

    while(sAlloc != &(iduMemMgr::mAllocList))
    {
        if((sDumpAll == ID_TRUE) ||
           (idlOS::strcmp(sAlloc->mName, aDumpTarget) == 0))
        {
            idlOS::snprintf(sFilename,
                            ID_SIZEOF(sFilename),
                            "%s%s%s.htm",
                            sDir,
                            IDL_FILE_SEPARATORS,
                            sAlloc->mName);
            sFile = idlOS::open(sFilename, O_CREAT | O_TRUNC | O_RDWR, 0644);
            IDE_TEST_RAISE(sFile == PDL_INVALID_HANDLE, EBUILDERROR);

            sAlloc->dumpMemory(sFile, aDumpLevel);
            (void)idlOS::close(sFile);

            sDumped++;
        }
        else
        {
            /* dump target only */
        }
            
        sAlloc = (iduMemAlloc*)sAlloc->mNext;
    }

    IDE_ASSERT(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) == 0);

    if(sDumped == 0)
    {
        (void)idlOS::snprintf(sLine, sizeof(sLine),
                              "No such allocator \"%s\"\n",
                              aDumpTarget);
        IDE_CALLBACK_SEND_SYM_NOLOG(sLine);
    }
    else
    {
        /* fall through */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(EPROPERTY)
    {
    }

    IDE_EXCEPTION(EBUILDERROR)
    {
        IDE_ASSERT(idlOS::thread_mutex_unlock(&(iduMemMgr::mAllocListLock)) == 0);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::createAllocator(iduMemAllocator *aAlloc,
                                  idCoreAclMemAllocType,
                                  void*)
{
    iduMemAlloc*    sAlloc;

    if(mMemType == IDU_MEMMGR_TLSF)
    {
        sAlloc = mTLSFLocal[getAllocIndex()];
    }
    else
    {
        sAlloc = NULL;
    }

    IDE_TEST(aAlloc->initialize(sAlloc) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::freeAllocator(iduMemAllocator *aAlloc,
                                idBool aCheckEmpty)
{
    PDL_UNUSED_ARG(aCheckEmpty);
    aAlloc->mAlloc = NULL;
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::shrinkAllocator(iduMemAllocator *aAlloc)
{
    return aAlloc->shrink();
}

IDE_RC iduMemMgr::shrinkAllAllocators(void)
{
    return IDE_SUCCESS;
}

UInt iduMemMgr::getAllocIndex(void)
{
    UInt sRet;
#if defined(OS_LINUX_KERNEL)
# if(__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 9)
    if(mIsCPU == ID_TRUE)
    {
        sRet = sched_getcpu();
    }
    else
    {
        sRet = acpAtomicInc32(&mAllocatorIndex) % mNoAllocators;
    }
# else
    sRet = acpAtomicInc32(&mAllocatorIndex) % mNoAllocators;
# endif
#else
    sRet = acpAtomicInc32(&mAllocatorIndex) % mNoAllocators;
#endif

    return sRet;
}

#ifdef ALTIBASE_PRODUCT_XDB

/*
 * C Interface for XDB DA
 */
ACP_EXTERN_C_BEGIN

IDE_RC iduMemMgrClientFree( void *aMemPtr )
{
    return ( IDE_RC )iduMemMgr::free( aMemPtr );
}

ACP_EXTERN_C_END

#endif /* ALTIBASE_PRODUCT_XDB */
