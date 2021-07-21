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
 
/*****************************************************************************
 * $Id:
 ****************************************************************************/

/*****************************************************************************
 *   NAME
 *     idtContainer.cpp - Thread Container class
 *
 *   DESCRIPTION
 *
 *   PUBLIC FUNCTION(S)
 *
 *   PRIVATE FUNCTION(S)
 *
 *   NOTES
 *
 *   MODIFIED   (MM/DD/YY)
 ****************************************************************************/

#include <idtContainer.h>
#include <idtBaseThread.h>
#include <ideLog.h>
#include <idp.h>
#include <iduProperty.h>
#include <iduFixedTable.h>
#include <ideLog.h>
#include <iduVersionDef.h>
#include <iduStack.h>
#include <idtCPUSet.h>

#include <acpSpinWait.h>

/* PROJ-2118 sigaltstack */
#ifdef SA_ONSTACK
#include <signal.h>
#if defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX)
#define SIGALTSTK_SIZE (16384)
#else
#define SIGALTSTK_SIZE SIGSTKSZ
#endif
#endif

/*
 * On SPARC Solaris, I found IDT_SPIN_YIELD worked slightly better for
 * createThread() and join() methods, but ACP_SPIN_WAIT worked order
 * of magitude faster in staticRunner() method and reasonably well in
 * other methods as well.
 */
#define IDT_SPIN_YIELD( aExpr, aSleep )         \
    {                                           \
        PDL_Time_Value sTv;                     \
                                                \
        if ( aSleep > 0 )                       \
        {                                       \
            sTv.msec( aSleep );                 \
        }                                       \
        else                                    \
        {                                       \
            /* do nothing */                    \
        }                                       \
                                                \
        for (;;)                                \
        {                                       \
            if ( aExpr )                        \
            {                                   \
                break;                          \
            }                                   \
            else                                \
            {                                   \
                /* do nothing */                \
            }                                   \
                                                \
            if ( aSleep > 0 )                   \
            {                                   \
                idlOS::sleep( sTv );            \
            }                                   \
            else                                \
            {                                   \
                idlOS::thr_yield();             \
            }                                   \
        }                                       \
    }

class idvSQL;

#define IDT_THREAD_MAX_SLEEP_SEC (10)

IDTHREAD idtContainer * gContainer = NULL;

idBool              idtContainer::mInitialized = ID_FALSE;
iduPeerType         idtContainer::mThreadType  = IDU_CLIENT_TYPE;
idtContainer        idtContainer::mMainThread;
iduMutex            idtContainer::mIdleLock;
iduMutex            idtContainer::mFreeLock;
iduMutex            idtContainer::mInfoLock;
PDL_thread_key_t    idtContainer::mTLS;

SInt                idtContainer::mNoThreads;
SInt                idtContainer::mMaxThreads;
idBool              idtContainer::mIsReuseEnable = ID_FALSE;

SChar* idtContainer::mThreadStatusString[IDT_MAX] = 
{
    (SChar*)"CREATING",
    (SChar*)"IDLE",
    (SChar*)"START",
    (SChar*)"RUNNING",
    (SChar*)"JOIN_WAIT",
    (SChar*)"JOINING",
    (SChar*)"FINALIZING",
    (SChar*)"FINISHING"
};

void initTempContainer( SChar       * aObjectString,
                        UInt          aChkFlag,
                        ideLogModule  aModule,
                        UInt          aLevel )
{
    SInt i;

    if ( gContainer == NULL )
    {
        gContainer = (idtContainer*)iduMemMgr::mallocRaw(ID_SIZEOF(idtContainer));
    }

    /* mallocRaw가 실패한 경우 */
    if( gContainer == NULL )
    {
        ideLog::log( aChkFlag, aModule, aLevel, "%s Thread Container Initialize failed", aObjectString );
    }
    else
    {
        gContainer->mThreadID   = idlOS::thr_self();
#ifdef ALTI_CFG_OS_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)   /* due to glibc bug */
        gContainer->mLWPID      = syscall(SYS_gettid);
#endif
        gContainer->mThread     = NULL;
        gContainer->mThreadNo   = 0;

        (void)idlOS::snprintf(gContainer->mIDString,
                              16,
                              "%"ID_XPOINTER_FMT,
                              gContainer->mThreadID);
        idlOS::strcpy(gContainer->mObjectString, aObjectString );
        idlOS::strcpy(gContainer->mIDParent, "N/A");

        idlOS::memcpy((void*)gContainer->mMemInfo,
                      (void*)iduMemMgr::mClientInfo,
                      ID_SIZEOF(iduMemClientInfo)*IDU_MEM_UPPERLIMIT);

        idlOS::memset( (void*)&(gContainer->mErrorStruct),
                       0,
                       ID_SIZEOF( ideErrorMgr ) );

        for(i = (SInt)IDU_MIN_CLIENT_COUNT; i < (SInt)IDU_MEM_UPPERLIMIT; i++)
        {
            idlOS::snprintf(gContainer->mMemInfo[i].mOwner,
                            ID_SIZEOF(gContainer->mMemInfo[i].mOwner),
                            "LIBC_THREAD_%s", gContainer->mIDString);
            gContainer->mMemInfo[i].mClientIndex = (iduMemoryClientIndex)i;
        }
        idlOS::strcpy(gContainer->mMemInfo[IDU_MEM_RESERVED].mName, "RESERVED");

        gContainer->mCPUNo      = 0;
        gContainer->mIsCPUSet   = ID_FALSE;
        gContainer->mNUMANo     = 0;
        gContainer->mIsNUMASet  = ID_FALSE;

        gContainer->mThreadStatus     = IDT_RUNNING;

        gContainer->mIdleLink  = NULL;
        gContainer->mFreeLink  = NULL;
        gContainer->mInfoPrev   = NULL;
        gContainer->mInfoNext   = NULL;

        gContainer->mInitialized = ID_TRUE;
        gContainer->mThreadType = IDU_SERVER_TYPE;
        gContainer->mNoThreads = 1;
        gContainer->mStackSize  = IDU_DEFAULT_THREAD_STACK_SIZE;

        gContainer->mSmallAlloc = NULL;
        gContainer->mTlsfAlloc  = NULL;

        IDE_ASSERT( gContainer->mContainerMutex.initialize( "THREAD_SYNC",
                                                            IDU_MUTEX_KIND_POSIX,
                                                            IDV_WAIT_ID_SYSTEM ) == IDE_SUCCESS );

        IDE_ASSERT( gContainer->mContainerCV.initialize() == IDE_SUCCESS );

        ideLog::log( aChkFlag, aModule, aLevel, "%s Thread Container Initialized", aObjectString );
    }
}

void destroyTempContainer()
{
    if ( gContainer != NULL )
    {
        gContainer->destroyContainer();

        iduMemMgr::freeRaw( gContainer );
        gContainer = NULL;
    }
}

void* idtContainer::staticRunner(void* aParam)
{
    idtContainer*   sContainer = (idtContainer*)aParam;

    idtBaseThread*  sThread;

#ifdef ALTI_CFG_OS_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)   /* due to glibc bug */
    sContainer->mLWPID      = syscall(SYS_gettid);
#endif

    /*
     * Set thread container as thread specific data
     */
    IDE_ASSERT(mThreadType == IDU_SERVER_TYPE);
    IDE_ASSERT(idlOS::thr_setspecific(mTLS, (void*)sContainer) == 0);
    IDE_ASSERT(iduStack::setSigStack() == IDE_SUCCESS);

    iduMemMgr::server_statupdate( IDU_MEM_ID_THREAD_STACK,
                                  sContainer->mStackSize,
                                  1 );


    /*
     * Initialize resources here
     * to get the nearst memory area from CPU
     */

    if(iduMemMgr::getSmallAlloc(&(sContainer->mSmallAlloc))
       != IDE_SUCCESS)
    {
        sContainer->mSmallAlloc = NULL;
    }
    else
    {
        /* Do nothing */
    }

    if(iduMemMgr::getTlsfAlloc(&(sContainer->mTlsfAlloc))
       != IDE_SUCCESS)
    {
        sContainer->mTlsfAlloc = NULL;
    }
    else
    {
        /* Do nothing */
    }
    ideErrorCollectionInit();

    while( sContainer->mThreadStatus < IDT_FINALIZING )
    {
        /*
         * PROJ-2595
         * Clear bucket messages
         */
        sContainer->makeBucket();

        /*
         * BUG-44105
         * Unbind thread affinity when a threads starts
         */
        if(idtCPUSet::unbindThread() != ACP_RC_SUCCESS)
        {
            ideLog::log( IDE_SERVER_0,
                         "Warning : Unbinding thread %d(%p) failed.",
                         sContainer->mThreadNo,
                         sContainer );
        }

        sContainer->wakeupStatus( IDT_IDLE );
        sContainer->waitStatus( IDT_START );

        IDE_TEST_CONT( sContainer->mThreadStatus == IDT_FINALIZING,
                       NORMAL_EXIT );

        sThread = (idtBaseThread*)sContainer->mThread;

        sContainer->mStartRC = sThread->initializeThread();

        sContainer->wakeupStatus( IDT_RUNNING );

        if( sContainer->mStartRC == IDE_SUCCESS )
        {
            sThread->run();
            sThread->finalizeThread();

        }
        else
        {
            /* do nothing */
        }

        if( sThread->mIsJoin == ID_TRUE )
        {
            sContainer->wakeupStatus( IDT_JOIN_WAIT );
            sContainer->waitStatus( IDT_JOINING );
        }
        else
        {
            /* Do nothing */
        }

        sContainer->mThread = NULL;
        sContainer->mParentNo = 0;
        sContainer->mParentID = 0;

        (void)idlOS::strcpy( sContainer->mObjectString, "NULL" );
        (void)idlOS::memset( sContainer->mIDString, 0, IDT_STRLEN );
        (void)idlOS::memset( sContainer->mIDParent, 0, IDT_STRLEN );
        (void)idlOS::strcpy( sContainer->mIDParent, "N/A" );

        if ( sContainer->mIsReuseEnable == ID_TRUE )
        {
            IDE_ASSERT( sContainer->mContainerMutex.lock( NULL ) == IDE_SUCCESS );
            sContainer->mThreadStatus = IDT_IDLE;
            IDE_ASSERT( sContainer->mContainerMutex.unlock() == IDE_SUCCESS );

            sContainer->addIdleList( sContainer );
        }
        else
        {
            IDE_ASSERT( sContainer->mContainerMutex.lock( NULL ) == IDE_SUCCESS );
            sContainer->mThreadStatus = IDT_FINISHING;
            IDE_ASSERT( sContainer->mContainerMutex.unlock() == IDE_SUCCESS );

            sContainer->addFreeList( sContainer );
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    ideErrorCollectionDestroy();
    sContainer->mThreadStatus = IDT_FINISHING;

    return NULL;
}

void* idtContainer::clientRunner(void* aParam)
{
    idtContainer*   sContainer = (idtContainer*)aParam;
    idtBaseThread*  sThread = NULL;

#ifdef ALTI_CFG_OS_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)   /* due to glibc bug */
    sContainer->mLWPID      = syscall(SYS_gettid);
#endif

    IDE_ASSERT(mThreadType == IDU_CLIENT_TYPE);
    IDE_ASSERT(idlOS::thr_setspecific(mTLS, (void*)sContainer) == 0);

    idlOS::memset((void*)&(sContainer->mErrorStruct), 0,
                  ID_SIZEOF(ideErrorMgr));
    ACP_SPIN_WAIT( NULL != sContainer->mThread, -1 );
    sThread = (idtBaseThread*)sContainer->mThread;
    IDE_ASSERT( sThread != NULL );

    sContainer->mStartRC = sThread->initializeThread();

    IDE_ASSERT( sContainer->mContainerMutex.lock( NULL ) == IDE_SUCCESS );
    sContainer->mThreadStatus = IDT_IDLE;
    IDE_ASSERT( sContainer->mContainerCV.wait( &(sContainer->mContainerMutex) ) == IDE_SUCCESS );
    IDE_ASSERT( sContainer->mContainerMutex.unlock() == IDE_SUCCESS );

    sContainer->mThreadStatus = IDT_RUNNING;
    if(sContainer->mStartRC == IDE_SUCCESS)
    {
        sThread->run();
        sThread->finalizeThread();
    }

    (void)sContainer->mContainerMutex.destroy();
    (void)sContainer->mContainerCV.destroy();
    sContainer->mThreadStatus = IDT_JOINING;

    return NULL;
}

IDE_RC idtContainer::initializeStatic(iduPeerType aType)
{
    SInt i;

    IDE_ASSERT(mInitialized == ID_FALSE);
    mMainThread.mThreadID   = idlOS::thr_self();
#ifdef ALTI_CFG_OS_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)   /* due to glibc bug */
    mMainThread.mLWPID      = syscall(SYS_gettid);
#endif
    mMainThread.mThread     = NULL;
    mMainThread.mThreadNo   = 0;

    (void)idlOS::snprintf(mMainThread.mIDString,
                          16,
                          "%"ID_XPOINTER_FMT,
                          mMainThread.mThreadID);
    idlOS::strcpy(mMainThread.mObjectString, "MAIN");
    idlOS::strcpy(mMainThread.mIDParent, "N/A");

    idlOS::memcpy((void*)mMainThread.mMemInfo,
                  (void*)iduMemMgr::mClientInfo,
                  ID_SIZEOF(iduMemClientInfo)*IDU_MEM_UPPERLIMIT);
#if 0
    idlOS::memset((void*)&(mMainThread.mErrorStruct), 0,
                  ID_SIZEOF(ideErrorMgr));
#endif

    for(i = (SInt)IDU_MIN_CLIENT_COUNT; i < (SInt)IDU_MEM_UPPERLIMIT; i++)
    {
        idlOS::snprintf(mMainThread.mMemInfo[i].mOwner,
                        ID_SIZEOF(mMainThread.mMemInfo[i].mOwner),
                        "LIBC_THREAD_%s", mMainThread.mIDString);
        mMainThread.mMemInfo[i].mClientIndex = (iduMemoryClientIndex)i;
    }
    idlOS::strcpy(mMainThread.mMemInfo[IDU_MEM_RESERVED].mName, "RESERVED");

    mMainThread.mCPUNo      = 0;
    mMainThread.mIsCPUSet   = ID_FALSE;
    mMainThread.mNUMANo     = 0;
    mMainThread.mIsNUMASet  = ID_FALSE;

    mMainThread.mThreadStatus     = IDT_RUNNING;

    mMainThread.mIdleLink  = NULL;
    mMainThread.mFreeLink  = NULL;
    mMainThread.mInfoPrev   = NULL;
    mMainThread.mInfoNext   = NULL;
    setTailInfo( &mMainThread );

    IDE_TEST(idlOS::thr_keycreate(&mTLS, NULL) != 0);
    IDE_TEST(idlOS::thr_setspecific(mTLS, (void*)&mMainThread) != 0);

    mInitialized = ID_TRUE;
    mThreadType = aType;
    mNoThreads = 1;

    initializeBucket();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idtContainer::initializeStaticContainer(void)
{
    IDE_DASSERT( mThreadType == IDU_SERVER_TYPE );

    IDE_TEST_RAISE( mIdleLock.initialize( "THREAD_IDLE_MUTEX",
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_ID_SYSTEM ) != IDE_SUCCESS,
                    EMUTEXERROR );
    IDE_TEST_RAISE( mInfoLock.initialize( "THREAD_INFO_MUTEX",
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_ID_SYSTEM ) != IDE_SUCCESS,
                    EMUTEXERROR );
    IDE_TEST_RAISE( mFreeLock.initialize( "THREAD_FREE_MUTEX",
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_ID_SYSTEM ) != IDE_SUCCESS,
                    EMUTEXERROR );

    /* BUG-45426 */
    mIsReuseEnable = iduProperty::getThreadReuseEnable();
    mMaxThreads = ID_SINT_MAX;

    return IDE_SUCCESS;

    IDE_EXCEPTION(EMUTEXERROR)
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idc_MUTEX_LOCK));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idtContainer::destroyStatic(void)
{
    while ( idtContainer::mNoThreads > 1 )
    {
        (void)cleanIdleList();
        (void)cleanFreeList();
    }

    (void)mIdleLock.destroy();
    (void)mInfoLock.destroy();
    (void)mFreeLock.destroy();

    mInitialized = ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC idtContainer::pop( idtContainer** aNewThread )
{
    idtContainer*   sNewThread = NULL;
    idtContainer*   sCurrent;

    /*
     * initializeStatic 이전에 서버 스레드를 생성할 수 없음
     */
    IDE_ASSERT(mInitialized == ID_TRUE || mThreadType == IDU_CLIENT_TYPE);

    if(mInitialized == ID_FALSE)
    {
        /*
         * Enable launch from utility
         */
        IDE_TEST(iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS);
        IDE_TEST(initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS);
    }

    if(mThreadType == IDU_SERVER_TYPE)
    {
        if ( mIsReuseEnable == ID_TRUE )
        {
            IDE_ASSERT( mMainThread.mIdleLock.lock( NULL ) == IDE_SUCCESS );
            sNewThread = mMainThread.mIdleLink;
            if ( sNewThread != NULL )
            {
                mMainThread.mIdleLink = sNewThread->mIdleLink;
            }
            else
            {
                /* Do nothing */
            }
            IDE_ASSERT( mMainThread.mIdleLock.unlock() == IDE_SUCCESS );
        }
        else
        {
            /* Do nothing */
        }

        sCurrent = idtContainer::getThreadContainer();

        if ( sNewThread == NULL )
        {
            IDE_TEST_RAISE(idtContainer::mNoThreads >=
                           idtContainer::mMaxThreads,
                           ETHREADEXCEED);

            sNewThread = (idtContainer*)iduMemMgr::mallocRaw(ID_SIZEOF(idtContainer));
            IDE_TEST(sNewThread == NULL);

       
            IDE_TEST_RAISE( sNewThread->createThread() != IDE_SUCCESS,
                            ECREATEFAILEDSERVER);

        }
        else
        {
            /* do nothing */
        }

        if ( sCurrent != NULL )
        { 
            sNewThread->mParentNo = sCurrent->getThreadNo();
            sNewThread->mParentID = sCurrent->getTid();
        }
        else
        {
            sNewThread->mParentNo = 0;
            sNewThread->mParentID = 0;
        }

        (void)idlOS::snprintf( sNewThread->mIDParent,
                               IDT_STRLEN,
                               "%"ID_XPOINTER_FMT,
                               sNewThread->mParentID );
    }
    else
    {
        sNewThread = (idtContainer*)idlOS::malloc(ID_SIZEOF(idtContainer));
        /* BUG-39551 */
        IDE_TEST(sNewThread == NULL);
        sNewThread->mThread = NULL;
        IDE_TEST_RAISE(sNewThread->createClient() != IDE_SUCCESS, ECREATEFAILEDCLIENT);
    }

    *aNewThread = sNewThread;
    return IDE_SUCCESS;

    IDE_EXCEPTION( ECREATEFAILEDSERVER )
    {
        iduMemMgr::freeRaw( sNewThread );
        IDE_SET( ideSetErrorCode( idERR_IGNORE_THREAD_CREATEFAIL ) );
    }

    IDE_EXCEPTION( ECREATEFAILEDCLIENT )
    {
        idlOS::free( sNewThread );
        IDE_SET( ideSetErrorCode( idERR_IGNORE_THREAD_CREATEFAIL ) );
    }

    IDE_EXCEPTION(ETHREADEXCEED)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_THREADCOUNT_EXCEEDED,
                                idtContainer::mMaxThreads));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idtContainer::createThread( void )
{
    SInt i;

    mThreadStatus   = IDT_CREATING;
    /*
     * BUGBUG : Replace these codes with
     * CPU or NUMA affinity functions
     */
    mCPUNo      = 0;
    mIsCPUSet   = ID_FALSE;
    mNUMANo     = 0;
    mIsNUMASet  = ID_FALSE;

    mStackSize  = IDU_DEFAULT_THREAD_STACK_SIZE;
   /*
     * Initialize resources here
     * to get the nearst memory area from CPU
     */
    mThread = NULL;
    mThreadNo   = 0;

    mSmallAlloc = NULL;
    mTlsfAlloc  = NULL;

    idlOS::memcpy( (void*)mMemInfo,
                   (void*)iduMemMgr::mClientInfo,
                   ID_SIZEOF(iduMemClientInfo) * IDU_MEM_UPPERLIMIT );
    idlOS::memset( (void*)&(mErrorStruct),
                   0,
                   ID_SIZEOF( ideErrorMgr ) );

    IDE_ASSERT( mContainerMutex.initialize( "THREAD_SYNC",
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_ID_SYSTEM ) == IDE_SUCCESS );

    IDE_ASSERT( mContainerCV.initialize() == IDE_SUCCESS );


    IDE_TEST( idlOS::thr_create( staticRunner,
                                 this,
                                 THR_JOINABLE,
                                 &mThreadID,
                                 &mHandle,
                                 PDL_DEFAULT_THREAD_PRIORITY,
                                 NULL,
                                 mStackSize ) != 0 );

    // Below seems to work slightly faster on SPARC Solaris.
    // IDT_SPIN_YIELD( mThreadStatus != IDT_CREATING, -1 );

    (void)idlOS::snprintf( mIDString,
                           IDT_STRLEN,
                           "%"ID_XPOINTER_FMT,
                           mThreadID );

    (void)idlOS::strcpy( mObjectString,
                         "NEW" );

    for ( i = (SInt)IDU_MIN_CLIENT_COUNT; i < (SInt)IDU_MEM_UPPERLIMIT ; i++ )
    {
        idlOS::snprintf( mMemInfo[i].mOwner,
                         ID_SIZEOF( mMemInfo[i].mOwner ),
                         "LIBC_THREAD_%s",
                         mIDString );
        mMemInfo[i].mClientIndex = (iduMemoryClientIndex)i;
    }
    idlOS::strcpy( mMemInfo[IDU_MEM_RESERVED].mName,
                   "RESERVED" );

    IDE_ASSERT( mMainThread.mInfoLock.lock( NULL ) == IDE_SUCCESS );

    mInfoPrev = mMainThread.mInfoTail;
    mInfoNext = NULL;

    mMainThread.mInfoTail->mInfoNext = this;
    setTailInfo( this );

    mThreadNo = idtContainer::mNoThreads;
    idtContainer::mNoThreads++;

    IDE_ASSERT( mMainThread.mInfoLock.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idtContainer::createClient(void)
{
    mThreadStatus   = IDT_CREATING;
    /*
     * BUGBUG : Replace these codes with
     * CPU or NUMA affinity functions
     */
    mCPUNo      = 0;
    mIsCPUSet   = ID_FALSE;
    mNUMANo     = 0;
    mIsNUMASet  = ID_FALSE;

    mStackSize  = IDU_DEFAULT_THREAD_STACK_SIZE;

    IDE_ASSERT( mContainerMutex.initialize( "THREAD_SYNC",
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_ID_SYSTEM) == IDE_SUCCESS );
    IDE_ASSERT( mContainerCV.initialize() == IDE_SUCCESS );

    IDE_TEST(idlOS::thr_create(clientRunner,
                               this,
                               THR_JOINABLE,
                               &mThreadID,
                               &mHandle,
                               PDL_DEFAULT_THREAD_PRIORITY,
                               NULL,
                               mStackSize) != 0);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
IDE_RC idtContainer::start( void* aThread )
{
    return (mThreadType == IDU_CLIENT_TYPE)?
        startClient( aThread ):
        startServer( aThread );
}

IDE_RC idtContainer::startClient( void* aThread )
{
    idtBaseThread* sThread = (idtBaseThread*)aThread;

    mThread = sThread;
    sThread->mContainer = this;

    /*
     * Wait idle status
     */
    ACP_SPIN_WAIT( mThreadStatus == IDT_IDLE, -1 );

    IDE_TEST( mContainerMutex.lock( NULL ) != IDE_SUCCESS );
    IDE_TEST_RAISE( mContainerCV.signal() != IDE_SUCCESS, ESTARTSIGNAL );
    IDE_TEST( mContainerMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ESTARTSIGNAL)
    {
        IDE_ASSERT( mContainerMutex.unlock() == IDE_SUCCESS );
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idtContainer::startServer( void* aThread )
{
    idtBaseThread* sThread = (idtBaseThread*)aThread;

    mThread = sThread;
    sThread->mContainer = this;

    (void)idlOS::snprintf( mObjectString,
                           IDT_STRLEN,
                           "%"ID_XPOINTER_FMT,
                           mThread );

    waitStatus( IDT_IDLE );
    wakeupStatus( IDT_START );
    waitStatus( IDT_RUNNING );

    IDE_TEST_RAISE( mStartRC != IDE_SUCCESS, ERR_START_FAIL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_START_FAIL )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InternalServerErrorWithString, 
                                  "Thread Start Fail" ) );

    }
    IDE_EXCEPTION_END;

    if ( mThread != NULL )
    {
        if ( mThread->mIsJoin == ID_TRUE )
        {
            wakeupStatus( IDT_JOINING );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

IDE_RC idtContainer::join(void)
{
    if(mThreadType == IDU_CLIENT_TYPE)
    {
        IDE_TEST( idlOS::thr_join(mHandle, NULL) != 0 );
        idlOS::free( this );
    }
    else
    {
        // Below seems to work slightly faster on SPARC Solaris.
        // IDT_SPIN_YIELD( mThreadStatus == IDT_JOINING, -1 );

        waitStatus( IDT_JOIN_WAIT );
        wakeupStatus( IDT_JOINING );

        if ( mIsReuseEnable == ID_FALSE )
        {
            if ( idtContainer::cleanFreeList() != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0,
                             "[Warning] : %s",
                             ideGetErrorMsg( ideGetErrorCode() ) );
            }
            else
            {
                /* Do nothing */
            }
        }
        else
        {
            /* Do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void idtContainer::destroyContainer(void)
{
    (void)mContainerMutex.destroy();
    (void)mContainerCV.destroy();
}

void idtContainer::addIdleList( idtContainer* aContainer )
{
    IDE_ASSERT( mMainThread.mIdleLock.lock( NULL ) == IDE_SUCCESS );

    aContainer->mIdleLink = mMainThread.mIdleLink;
    mMainThread.mIdleLink = aContainer;

    IDE_ASSERT( mMainThread.mIdleLock.unlock() == IDE_SUCCESS );
}
IDE_RC idtContainer::cleanIdleList()
{
    /* IdleList의 container들을 join시키고 메모리를 정리한다. */
    idtContainer* sContainer = NULL;
    idtContainer* sNextContainer = NULL;
    idBool        sIsLocked = ID_FALSE;

    IDE_ASSERT( mMainThread.mIdleLock.lock( NULL ) == IDE_SUCCESS );
    sIsLocked = ID_TRUE;
    sContainer = idtContainer::getFirstIdle();

    while ( sContainer != NULL )
    {
        sNextContainer = sContainer->mIdleLink;

        sContainer->wakeupStatus( IDT_FINALIZING );
        IDE_TEST_RAISE( idlOS::thr_join( sContainer->mHandle, NULL ) != 0,
                        JOIN_FAIL );

        removeFromInfoList( sContainer );
        /* remove from Idlelist*/
        mMainThread.mIdleLink = sNextContainer;

        sContainer->destroyContainer();
        iduMemMgr::freeRaw( sContainer );

        sContainer = sNextContainer;
    }

    sIsLocked = ID_FALSE;
    IDE_ASSERT( mMainThread.mIdleLock.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( JOIN_FAIL )
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_Systhrjoin ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mMainThread.mIdleLock.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* Do nothing */
    }

    return IDE_FAILURE;
}


void idtContainer::addFreeList( idtContainer* aContainer )
{
    IDE_ASSERT( mMainThread.mFreeLock.lock( NULL ) == IDE_SUCCESS );

    aContainer->mFreeLink = mMainThread.mFreeLink;
    mMainThread.mFreeLink = aContainer;

    IDE_ASSERT( mMainThread.mFreeLock.unlock() == IDE_SUCCESS );
}

IDE_RC idtContainer::cleanFreeList()
{
    /* FreeList의 container들을 join시키고 메모리를 정리한다. */
    idtContainer* sContainer = NULL;
    idtContainer* sNextContainer = NULL;
    idBool        sIsLocked = ID_FALSE;

    IDE_ASSERT( mMainThread.mFreeLock.lock( NULL ) == IDE_SUCCESS );
    sIsLocked = ID_TRUE;
    sContainer = idtContainer::getFirstFree();

    while ( sContainer != NULL )
    {
        sNextContainer = sContainer->mFreeLink;

        IDU_FIT_POINT_RAISE( "idtContainer::cleanFreeList::thr_join",
                             JOIN_FAIL );
        IDE_TEST_RAISE( idlOS::thr_join( sContainer->mHandle, NULL ) != 0,
                        JOIN_FAIL );

        removeFromInfoList( sContainer );
        /* remove from FreeList*/
        mMainThread.mFreeLink = sNextContainer;

        sContainer->destroyContainer();
        iduMemMgr::freeRaw( sContainer );

        sContainer = sNextContainer;
    }

    sIsLocked = ID_FALSE;
    IDE_ASSERT( mMainThread.mFreeLock.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( JOIN_FAIL )
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_Systhrjoin ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mMainThread.mFreeLock.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* Do nothing */
    }

    return IDE_FAILURE;
}


void idtContainer::removeFromInfoList( idtContainer* aThread )
{
    idtContainer*  sPrev = NULL;
    idtContainer*  sNext = NULL;

    IDE_DASSERT( aThread != NULL );

    IDE_ASSERT( mMainThread.mInfoLock.lock( NULL ) == IDE_SUCCESS );

    sPrev = aThread->mInfoPrev;
    sNext = aThread->mInfoNext;

    if ( sNext != NULL )
    {
        sPrev->mInfoNext = sNext;
        sNext->mInfoPrev = sPrev;
    }
    else
    {
        sPrev->mInfoNext = NULL;
        setTailInfo( sPrev );
    }
    idtContainer::mNoThreads--;
    IDE_ASSERT( mMainThread.mInfoLock.unlock() == IDE_SUCCESS );
}

void idtContainer::wakeupStatus( idtThreadStatus aStatus )
{
    IDE_ASSERT( mContainerMutex.lock( NULL ) == IDE_SUCCESS );

    if ( mThreadStatus < aStatus )
    {
        mThreadStatus = aStatus;
    }
    else
    {
        /* Do nothing */
    }

    (void)mContainerCV.signal();

    IDE_ASSERT( mContainerMutex.unlock() == IDE_SUCCESS );

}
void idtContainer::waitStatus( idtThreadStatus aStatus )
{
    IDE_RC sRc = IDE_FAILURE;
    PDL_Time_Value     sCheckTime;

    sCheckTime.initialize();

    IDE_ASSERT( mContainerMutex.lock( NULL ) == IDE_SUCCESS );

    while ( mThreadStatus < aStatus )
    {
        sCheckTime.set( idlOS::time() + IDT_THREAD_MAX_SLEEP_SEC );
        sRc = mContainerCV.timedwait( &(mContainerMutex),
                                      &(sCheckTime),
                                      IDU_IGNORE_TIMEDOUT );

        if ( sRc == IDE_FAILURE )
        {
            ideLog::log( IDE_SERVER_0,
                         "[Warning] : %s", ideGetErrorMsg( ideGetErrorCode() ) );
            idlOS::sleep(1);
        }
        else
        {
            /* Do nothing */
        }
    }
    IDE_ASSERT( mContainerMutex.unlock() == IDE_SUCCESS );

}

IDE_RC idtContainer::setCPUAffinity (SInt aCPUNo)
{
    PDL_UNUSED_ARG(aCPUNo);
    return IDE_SUCCESS;
}

IDE_RC idtContainer::setNUMAAffinity(SInt aNUMANo)
{
    PDL_UNUSED_ARG(aNUMANo);
    return IDE_SUCCESS;
}

idtContainer* idtContainer::getThreadContainer(void)
{
    idtContainer* sContainer;

    if(idtContainer::mInitialized == ID_TRUE)
    {
        IDE_ASSERT(idlOS::thr_getspecific(idtContainer::mTLS, (void**)&sContainer) == 0);
    }
    else
    {
        sContainer = &(mMainThread);
    }

    if ( sContainer == NULL )
    {
        sContainer = gContainer;
    }

    return sContainer;
}


ideErrorMgr* idtContainer::getErrorStruct(void)
{
    idtContainer* sContainer;
    sContainer = idtContainer::getThreadContainer();
    return (sContainer==NULL)? NULL:&(sContainer->mErrorStruct);
}

iduMemClientInfo* idtContainer::getMemClientInfo(void)
{
    idtContainer* sContainer;
    sContainer = idtContainer::getThreadContainer();
    return (sContainer==NULL)? NULL:&(sContainer->mMemInfo[0]);
}

IDE_RC idtContainer::expand(SInt aNewMax)
{
    IDE_TEST_RAISE(aNewMax <= mMaxThreads, ECANNOTSHRINK);
    mMaxThreads = aNewMax;
    return IDE_SUCCESS;

    IDE_EXCEPTION(ECANNOTSHRINK)
    {
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void idtContainer::makeBucket(void)
{
    /*
     * Prepare for bucket list
     */
    SInt i;

    for(i = IDE_ERR; i < IDE_LOG_MAX_MODULE; i++)
    {
        mBucketMsgLength[i] = 0;
    }

    mBucketMsgLength[IDE_SERVER] = idlOS::snprintf(mBucketMsg[IDE_SERVER],
                                                   IDT_MAX_MSG_SIZE,
                                                   "ALTIBASE %s\n"
                                                   "\tProduct version  : %s\n"
                                                   "\tCPU              : %s\n"
                                                   "\tOperating System : %s\n"
                                                   "\tProcess ID       : %d\n"
                                                   "\tThread No        : %d\n",
                                                   ALTIBASE_PRODUCT,
                                                   IDU_ALTIBASE_VERSION_STRING,
                                                   ALTI_CFG_CPU,
                                                   OS_SYSTEM_TYPE,
                                                   idlOS::getpid(),
                                                   getThreadNo());

    mStartRC = IDE_FAILURE;
}

void idtContainer::initializeBucket()
{
    mMainThread.makeBucket();
}

void idtContainer::makeAffinityString(void)
{
    idtBaseThread* sThread;

    sThread = (idtBaseThread*)mThread;

    if(mThread == NULL)
    {
        idlOS::strcpy(mAffinityString, "N/A");
    }
    else
    {
        sThread->mAffinity.dumpCPUsToString(mAffinityString, 128);
    }
}

#if !defined(SMALL_FOOTPRINT) || defined(WRS_VXWORKS)

/*
 * Project 2379
 * Fixed Table to Retrieve Thread Container Information
 */

static iduFixedTableColDesc gThreadContainerColDesc[] =
{
    {
        (SChar *)"THREAD_NO",
        offsetof(idtContainer, mThreadNo),
        IDU_FT_SIZEOF(idtContainer, mThreadNo),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PARENT_NO",
        offsetof(idtContainer, mParentNo),
        IDU_FT_SIZEOF(idtContainer, mParentNo),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"THREAD_ID",
        offsetof(idtContainer, mIDString),
        IDU_FT_SIZEOF(idtContainer, mIDString),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"PARENT_ID",
        offsetof(idtContainer, mIDParent),
        IDU_FT_SIZEOF(idtContainer, mIDParent),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"THREAD_OBJECT",
        offsetof(idtContainer, mObjectString),
        IDU_FT_SIZEOF(idtContainer, mObjectString),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"STATUS",
        offsetof(idtContainer, mStatusString),
        IDT_STRLEN,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"AFFINITY",
        offsetof(idtContainer, mAffinityString),
        128,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

static IDE_RC buildRecordForThreadContainer(idvSQL      *,
                                            void        *aHeader,
                                            void        * /* aDumpObj */,
                                            iduFixedTableMemory *aMemory)
{
    idtContainer* sBase = NULL;
    idBool        sIsLocked = ID_FALSE;

    if ( idtContainer::getThreadReuseEnable() == ID_FALSE )
    {
        if ( idtContainer::cleanFreeList() != IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0,
                         "[Warning] : %s", ideGetErrorMsg( ideGetErrorCode() ) );
        }
        else
        {
            /* Do nothing */
        }
        IDE_ASSERT( idtContainer::mMainThread.mInfoLock.lock( NULL ) == IDE_SUCCESS );
        sIsLocked = ID_TRUE;

    }
    else
    {
        /* Do nothing */
    }

    sBase = idtContainer::getFirstInfo();
    do
    {
        sBase->setStatusString();
        sBase->makeAffinityString();

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void*)sBase)
                 != IDE_SUCCESS);

        sBase = sBase->getNextInfo();
    } while(sBase != NULL);

    if ( idtContainer::getThreadReuseEnable() == ID_FALSE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( idtContainer::mMainThread.mInfoLock.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* Do nothing */
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( idtContainer::mMainThread.mInfoLock.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* Do nothing */
    }
    return IDE_FAILURE;
}


iduFixedTableDesc gThreadContainerTableDesc =
{
    (SChar *)"X$THREADS",
    buildRecordForThreadContainer,
    gThreadContainerColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

#endif

