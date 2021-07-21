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
 
/* **********************************************************************
 *   $Id: 
 *   NAME
 *     idtContainer.h - 스레드 컨테이너
 *
 *   DESCRIPTION
 *     프로젝트 2379로 새로 생성되는 스레드 컨테이너
 *     스레드 컨테이너 
 *
 *   PUBLIC FUNCTION(S)
 *
 *   PRIVATE FUNCTION(S)
 *
 *   NOTES
 *
 *   MODIFIED   (MM/DD/YY)
 ********************************************************************** */

#ifndef O_IDT_CONTAINER_H
#define O_IDT_CONTAINER_H   1

#include <acp.h>
#include <idl.h>
#include <ideErrorMgr.h>
#include <iduMemDefs.h>
#include <iduMutex.h>
#include <iduCond.h>
#include <iduFitManager.h>

/* PROJ-2118 sigaltstack */
#ifdef SA_ONSTACK
#include <signal.h>
#if defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX)
#define SIGALTSTK_SIZE (16384)
#else
#define SIGALTSTK_SIZE SIGSTKSZ
#endif
#endif

typedef enum
{
    IDT_CREATING = 0,
    IDT_IDLE,
    IDT_START,
    IDT_RUNNING,
    IDT_JOIN_WAIT,
    IDT_JOINING,
    IDT_FINALIZING,
    IDT_FINISHING,
    IDT_MAX
} idtThreadStatus;

#define IDT_STRLEN 16
#define IDT_MAX_MSG_SIZE (65536)

IDL_EXTERN_C void initTempContainer( SChar       * aObjectString,
                                     UInt          aChkFlag,
                                     ideLogModule  aModule,
                                     UInt          aLevel );

IDL_EXTERN_C void destroyTempContainer();

class idtBaseThread;

class idtContainer
{
public:
    /*
     * For Fixed Table
     */
    SChar               mIDString[IDT_STRLEN];
    SChar               mIDParent[IDT_STRLEN];
    SChar               mObjectString[IDT_STRLEN];
    SChar*              mStatusString;
    SChar               mAffinityString[128];

    /*
     * Thread handles
     */
    PDL_thread_t        mThreadID;
    PDL_hthread_t       mHandle;
    PDL_thread_t        mParentID;
    idtBaseThread*      mThread;

    /*
     * Thread Specific information
     */
    iduMemClientInfo    mMemInfo[IDU_MEM_UPPERLIMIT];
    ideErrorMgr         mErrorStruct;

    size_t              mStackSize;

    iduMutex            mContainerMutex;
    iduCond             mContainerCV;
    IDE_RC              mStartRC;

    /*
     * CPU and NUMA node number
     */
    SInt                mCPUNo;
    idBool              mIsCPUSet;
    SInt                mNUMANo;
    idBool              mIsNUMASet;

    /*
     * Status
     */
    volatile idtThreadStatus     mThreadStatus;

    /*
     * Information, allocation link, and thread key
     */
    idtContainer*       mIdleLink;
    idtContainer*       mFreeLink;
    idtContainer*       mInfoPrev;
    idtContainer*       mInfoNext;
    idtContainer*       mInfoTail;
    
    static idtContainer mMainThread;
    static iduPeerType  mThreadType;

    /*
     * Static Information -
     * Mutexes and Thread Local Key
     */
    static iduMutex             mIdleLock;
    static iduMutex             mFreeLock;
    static iduMutex             mInfoLock;
    static SChar*               mThreadStatusString[IDT_MAX];
    static idBool               mUsePrivateAllocator;
    static idBool               mInitialized;
    static PDL_thread_key_t     mTLS;
    static SInt                 mNoThreads;
    static SInt                 mMaxThreads;
    static idBool               mIsReuseEnable;

    static void*                staticRunner(void*);
    static void*                clientRunner(void*);
    /*
     * Small blocks will be allocated from thread specific allocator
     */
    iduMemSmall*                mSmallAlloc;
    iduMemTlsf*                 mTlsfAlloc;

#ifdef ALTI_CFG_OS_LINUX
    pid_t                       mLWPID;
    inline  pid_t getSysLWPNo(void){ return mLWPID ;}
    static inline pid_t getSysLWPNumber(void)
    {
        return getThreadContainer()->getSysLWPNo();
    }
#endif

    inline PDL_thread_t getTid()
    {
        return mThreadID;
    }

    inline PDL_hthread_t getHandle()
    {
        return mHandle;
    }

    inline void* getThread()
    {
        return mThread;
    }


    inline idBool isStarted(void)
    {
        return (mThreadStatus != IDT_IDLE)? ID_TRUE:ID_FALSE;
    }

    /*
     * Create, start and join
     */
    IDE_RC          createThread(void);
    IDE_RC          createClient(void);
    IDE_RC          start(void* aThread);
    IDE_RC          startServer(void* aThread);
    IDE_RC          startClient(void* aThread);
    IDE_RC          join(void);

    /*
     * Thread affinities
     */
    IDE_RC          setCPUAffinity (SInt aCPUNo);
    IDE_RC          setNUMAAffinity(SInt aNUMANo);

    /*
     * Initialization, destruction
     * Thread specific information
     */
    static void                 initializeBucket(void);
    static IDE_RC               initializeStatic(iduPeerType aType);
    static IDE_RC               initializeStaticContainer(void);
    static IDE_RC               destroyStatic(void);
    static IDE_RC               pop(idtContainer**);
    static idtContainer*        getThreadContainer(void);
    static ideErrorMgr*         getErrorStruct(void);
    static iduMemClientInfo*    getMemClientInfo(void);
    static IDE_RC               expand(SInt aNewMax);

    void                        wakeupStatus( idtThreadStatus aStatus );
    void                        waitStatus( idtThreadStatus aStatus );
    void                        addIdleList( idtContainer* aContainer );
    static IDE_RC               cleanIdleList();
    void                        addFreeList( idtContainer* aContainer );
    static IDE_RC               cleanFreeList();
    static void                 removeFromInfoList( idtContainer* aThread );
    void                        destroyContainer( void );

    static idtContainer* getFirstIdle( void )
    {
        return mMainThread.mIdleLink;
    }
    static idtContainer* getFirstFree( void )
    {
        return mMainThread.mFreeLink;
    }

    static idtContainer* getFirstInfo( void )
    {
        return &(idtContainer::mMainThread);
    }
    static idtContainer* getTailInfo( void )
    {
        return mMainThread.mInfoTail;
    }

    static void setTailInfo( idtContainer* aContainer )
    {
        mMainThread.mInfoTail = aContainer;
    }

    inline idtContainer* getPrevInfo( void )
    {
        return mInfoPrev;
    }

    inline idtContainer* getNextInfo( void )
    {
        return mInfoNext;
    }

    inline void setStatusString(void)
    {
        mStatusString = idtContainer::mThreadStatusString[(SInt)mThreadStatus];
    }

    void makeAffinityString(void);

#if defined(WRS_VXWORKS) 
    static void*    getTaskEntry() {return (void*)staticRunner;}
#endif 

    /*
     * Project-2595
     * Thread specific data
     */
    UInt        mThreadNo;
    UInt        mParentNo;
    size_t      mBucketMsgLength[IDE_LOG_MAX_MODULE];
    SChar       mBucketMsg[IDE_LOG_MAX_MODULE][IDT_MAX_MSG_SIZE];

    inline UInt getThreadNo(void) {return mThreadNo;}
    inline PDL_thread_t getSysThreadNo(void) {return mThreadID;}
    static inline UInt getThreadNumber(void)
    {
        return getThreadContainer()->getThreadNo();
    }
    static inline PDL_thread_t getSysThreadNumber(void)
    {
        return getThreadContainer()->getSysThreadNo();
    }

    static inline idBool getThreadReuseEnable( void )
    {
        return idtContainer::mIsReuseEnable;
    }

    static inline void* getBaseThread( void )
    {
        return getThreadContainer()->getThread();
    }


    void makeBucket(void);
};

#endif 
