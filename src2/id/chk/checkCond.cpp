/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: checkCond.cpp 26440 2008-06-10 04:02:48Z jdlee $
 ****************************************************************************/
#include <idl.h>
#include <ide.h>
#include <ida.h>
#include <checkIPC.h>

struct SharedInfo
{
    ULong        mCount;
    PDL_mutex_t  mMutex1;
    PDL_mutex_t  mMutex2;
    PDL_cond_t   mCond;
};

class IPC
{
public:
    static PDL_HANDLE mShmID;
    static SInt       mShmKey;
    static SChar     *mShmBuf;

    static SharedInfo  *mInfo;

public:
    IDE_RC initialize(SInt aSemkey, SInt aShmKey);
    IDE_RC lock1();
    IDE_RC wait1();
    IDE_RC signal1();
    IDE_RC destroy1();
    IDE_RC unlock1();

    IDE_RC lock2();
    IDE_RC unlock2();
    IDE_RC wait2();
    IDE_RC signal2();
    IDE_RC destroy2();
};


PDL_HANDLE IPC::mShmID;
SInt       IPC::mShmKey;
SChar     *IPC::mShmBuf;

SharedInfo *IPC::mInfo;

IDE_RC IPC::initialize(SInt aSemKey, SInt aShmKey)
{
    /* ------------------------------------------------
     *  Shared memory
     * ----------------------------------------------*/

    mShmKey = aShmKey;
    mShmID  = idlOS::shmget(aShmKey, sizeof(SharedInfo) + 128,
                                            0666 | IPC_CREAT/* | IPC_EXCL*/);
    assert(mShmID != PDL_INVALID_HANDLE);

    mShmBuf = (SChar *)idlOS::shmat(mShmID, NULL, 0);
    assert(mShmBuf != (void *)-1);

    mInfo = (SharedInfo *)mShmBuf;

    assert(idlOS::mutex_init(&(mInfo->mMutex1), USYNC_PROCESS) == 0);
    assert(idlOS::mutex_init(&(mInfo->mMutex2), USYNC_PROCESS) == 0);
    assert(idlOS::cond_init(&(mInfo->mCond), USYNC_PROCESS) == 0);
    mInfo->mCount = 0;

    return IDE_SUCCESS;
}


IDE_RC IPC::lock1()
{
    assert(idlOS::mutex_lock(&mInfo->mMutex1) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::unlock1()
{
    assert(idlOS::mutex_unlock(&mInfo->mMutex1) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::wait1()
{
    SInt rc;
    rc = idlOS::cond_wait(&(mInfo->mCond), &(mInfo->mMutex1));
    printf("cond wait rc = %d \n", rc);
    return IDE_SUCCESS;

}

IDE_RC IPC::signal1()
{
    assert(idlOS::cond_signal(&(mInfo->mCond)) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::destroy1()
{
    assert(idlOS::mutex_destroy(&(mInfo->mMutex1)) == 0);
    assert(idlOS::cond_destroy(&(mInfo->mCond)) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::lock2()
{
    assert(idlOS::mutex_lock(&mInfo->mMutex2) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::unlock2()
{
    assert(idlOS::mutex_unlock(&mInfo->mMutex2) == 0);
    return IDE_SUCCESS;
}

IPC  myIPC;



//#define  CHILD_LOCK_HANG  1
#define  CHILD_WAIT_HANG  1

#define TEST_COUNT  1000000

void sigint(SInt num)
{
    signal(SIGINT, sigint);
    idlOS::printf("SIGINT!!! \n");
}

void sigdestroy(SInt num)
{
    myIPC.signal1();
    myIPC.unlock1();
    idlOS::printf("destroy!!! \n");
    signal(SIGUSR1, sigdestroy);
}

int main()
{
    ULong i;
    timeval t1, t2;
    pid_t   childPID;
    pid_t   childPID2;

    myIPC.initialize(50505, 50506);
    signal(SIGINT, sigint);

    childPID = idlOS::fork();
    if ( childPID == 0) // child
    {
        assert(PDL_OS::signal (SIGHUP, SIG_IGN) == 0);
        t1 = getCurTime();
        for (i = 0; i < TEST_COUNT; i++)
        {
#ifdef CHILD_LOCK_HANG
            idlOS::sleep(3);
#endif
            idlOS::printf("[FIRST CHILD] try lock \n");
            myIPC.lock1();
            idlOS::printf("[FIRST CHILD] lock & sleep \n");
            myIPC.wait1();
            idlOS::printf("[FIRST CHILD] wake up \n");
            myIPC.unlock1();
            if (IPC::mInfo->mCount != 0)
            {
                idlOS::printf("[FIRST CHILD] die... \n");
                idlOS::exit(0);
            }
        }
        t2 = getCurTime();

        idlOS::printf("child : time = %ld \n", getTimeGap(t1, t2));
        idlOS::printf("child : average time = %f \n\n",
                      (SDouble)getTimeGap(t1, t2) / TEST_COUNT);

    }
    else
    {
        idlOS::printf("[FIRST CHILD] pid=%d \n", childPID);
        childPID2 = idlOS::fork();
        if ( childPID2 == 0) // mutex를 잡고 종료하는 thread
        {
#ifdef CHILD_WAIT_HANG
            idlOS::sleep(3);
#endif
            myIPC.lock1();
            idlOS::printf("[SECOND] child get Lock() & exit.=%d \n", childPID2);
            idlOS::exit(0);
           //signal(SIGUSR1, sigdestroy);

        }
        idlOS::printf("second child pid=%d \n", childPID2);

        while(1)
        {
            idlOS::printf("[PARENT] sleep 10 \n");
            idlOS::sleep(10);
            IPC::mInfo->mCount = 1;
            idlOS::printf("[PARENT] mutex signal & unlock. \n");
            myIPC.signal1();
            myIPC.unlock1();
#ifdef CHILD_LOCK_HANG
            idlOS::printf("[PARENT] SIGINT signal send. PID=%d \n", childPID);
            idlOS::sleep(1);
            idlOS::kill(childPID, SIGINT);
            myIPC.signal1();
            idlOS::sleep(1);
            myIPC.signal1();
#endif

            assert(idlOS::wait(childPID, NULL) == childPID);
            idlOS::exit(0);
        }
    }
}
