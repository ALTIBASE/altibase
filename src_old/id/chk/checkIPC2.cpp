/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: checkIPC2.cpp 26440 2008-06-10 04:02:48Z jdlee $
 ****************************************************************************/
#include <idl.h>
#include <ide.h>
#include <ida.h>
#include <checkIPC.h>

/* ------------------------------------------------
 *  Single or Double(concurrent)
 * ----------------------------------------------*/
//#define TEST_SINGLE
#define TEST_DOUBLE

/* ------------------------------------------------
 *  세마퍼 테스트
 * ----------------------------------------------*/
#define TEST_SEMA
//#define TEST_MUTEX


struct SharedInfo
{
    ULong        mCount;
    PDL_mutex_t mMutex1;
    PDL_mutex_t mMutex2;
};

class IPC
{
public:
    static SInt   mSemID;
    static SInt   mSemKey;
    static struct sembuf mGet1;
    static struct sembuf mPost1;
    static struct sembuf mGet2;
    static struct sembuf mPost2;

    static PDL_HANDLE mShmID;
    static SInt       mShmKey;
    static SChar     *mShmBuf;

    static SharedInfo  *mInfo;

public:
    IDE_RC initialize(SInt aSemkey, SInt aShmKey);
    IDE_RC lock1();
    IDE_RC lock2();
    IDE_RC unlock1();
    IDE_RC unlock2();
};

SInt   IPC::mSemID;
SInt   IPC::mSemKey;
struct sembuf IPC::mGet1;
struct sembuf IPC::mPost1;
struct sembuf IPC::mGet2;
struct sembuf IPC::mPost2;

PDL_HANDLE IPC::mShmID;
SInt       IPC::mShmKey;
SChar     *IPC::mShmBuf;

SharedInfo *IPC::mInfo;

IDE_RC IPC::initialize(SInt aSemKey, SInt aShmKey)
{
    /* ------------------------------------------------
     *  Semaphore
     * ----------------------------------------------*/

    union semun s_sem_arg;

    mSemKey = aSemKey;
    mSemID  = idlOS::semget(aSemKey, 2, 0666 | IPC_CREAT/* | IPC_EXCL*/);
    assert(mSemID >= 0);

    // 초기화
    s_sem_arg.val = 0;
    assert( idlOS::semctl( mSemID, 0, SETVAL, s_sem_arg) == 0);
    assert( idlOS::semctl( mSemID, 1, SETVAL, s_sem_arg) == 0);

    mGet1.sem_num   =  0;
    mGet1.sem_op    = -1;
    mGet1.sem_flg   =  0;

    mPost1.sem_num   =  0;
    mPost1.sem_op    =  1;
    mPost1.sem_flg   =  0;

    mGet2.sem_num   =  1;
    mGet2.sem_op    = -1;
    mGet2.sem_flg   =  0;

    mPost2.sem_num   =  1;
    mPost2.sem_op    =  1;
    mPost2.sem_flg   =  0;

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
    mInfo->mCount = 0;

    return IDE_SUCCESS;
}

IDE_RC IPC::lock1()
{
    assert(idlOS::semop(mSemID, &mGet1, 1) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::unlock1()
{
    assert(idlOS::semop(mSemID, &mPost1, 1) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::lock2()
{
    assert(idlOS::semop(mSemID, &mGet2, 1) == 0);
    return IDE_SUCCESS;
}

IDE_RC IPC::unlock2()
{
    assert(idlOS::semop(mSemID, &mPost2, 1) == 0);
    return IDE_SUCCESS;
}

#define TEST_COUNT  10000

int main()
{
    ULong i;
    timeval t1, t2;
    pid_t   childPID;
    IPC  myIPC;

    myIPC.initialize(50595, 50596);

    childPID = idlOS::fork();

    if ( childPID == 0) // mutex를 잡고 종료하는 thread
    {
        for (i = 0; i < TEST_COUNT; i++)
        {
            //assert(iduSema::wait(IPC::mSemaA) == IDE_SUCCESS);
            assert(myIPC.lock1() == IDE_SUCCESS);
            //idlOS::fprintf(stderr, "child-1 ");idlOS::fflush(stderr);
            //assert(iduSema::post(IPC::mSemaB) == IDE_SUCCESS);
            assert(myIPC.unlock2() == IDE_SUCCESS);
        }
        idlOS::exit(0);
    }
    else
    {
        idlOS::sleep(1);

        t1 = getCurTime();
        for (i = 0; i < TEST_COUNT; i++)
        {
            assert(myIPC.unlock1() == IDE_SUCCESS);
            //assert(iduSema::post(IPC::mSemaA) == IDE_SUCCESS);
            assert(myIPC.lock2() == IDE_SUCCESS);
            //assert(iduSema::wait(IPC::mSemaB) == IDE_SUCCESS);
            //idlOS::fprintf(stderr, "parent-1 ");idlOS::fflush(stderr);
        }
    }
    t2= getCurTime();
    idlOS::printf("cout=%d time = %ld \n", TEST_COUNT, getTimeGap(t1, t2));

    idlOS::printf("average time = %f \n",
                  (SDouble)getTimeGap(t1, t2) / TEST_COUNT);


}
