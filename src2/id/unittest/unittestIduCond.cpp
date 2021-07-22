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
 
#include <idl.h>
#include <act.h>
#include <idu.h>
#include <ide.h>
#include <idtBaseThread.h>

#define TESTINIT    0
#define TESTWAIT    1


class testThread : public idtBaseThread
{
public:
    iduMutex*   mMutex;
    iduCond*    mCond;
    SInt        mStatus;

    testThread(iduMutex* aMutex, iduCond* aCond)
        : idtBaseThread(), mMutex(aMutex), mCond(aCond)
    {
        mStatus = TESTINIT;
    }
    virtual ~testThread() {};

    virtual void wait() = 0;
    virtual void signal() = 0;

    virtual void test()
    {
        ACT_ASSERT(start() == IDE_SUCCESS);
        while(mStatus == TESTINIT)
        {
            idlOS::thr_yield();
        }
        
        signal();
        join();
    }

    virtual void run()
    {
        ACT_ASSERT(mMutex->lock(NULL) == IDE_SUCCESS);
        mStatus = TESTWAIT;
        wait();
        ACT_ASSERT(mMutex->unlock() == IDE_SUCCESS);
    }
};

class testWait : public testThread
{
public:
    testWait(iduMutex* aMutex, iduCond* aCond) : testThread(aMutex, aCond) {}
    virtual void signal()
    {
        ACT_ASSERT(mMutex->lock(NULL) == IDE_SUCCESS);
        ACT_ASSERT(mCond->signal() == IDE_SUCCESS);
        ACT_ASSERT(mMutex->unlock() == IDE_SUCCESS);
    }

    virtual void wait()
    {
        ACT_ASSERT(mCond->wait(mMutex) == IDE_SUCCESS);
    }
};

class testTimedWait : public testThread
{
public:
    testTimedWait(iduMutex* aMutex, iduCond* aCond) : testThread(aMutex, aCond) {}
    virtual void signal()
    {
        ACT_ASSERT(mMutex->lock(NULL) == IDE_SUCCESS);
        ACT_ASSERT(mCond->signal() == IDE_SUCCESS);
        ACT_ASSERT(mMutex->unlock() == IDE_SUCCESS);
    }

    virtual void wait()
    {
        PDL_Time_Value sTime;

        sTime.initialize(1);
        ACT_ASSERT(mCond->timedwait(mMutex, &sTime, IDU_CATCH_TIMEDOUT) == IDE_SUCCESS);
    }
};

class testTimedOut : public testThread
{
public:
    testTimedOut(iduMutex* aMutex, iduCond* aCond) : testThread(aMutex, aCond) {}
    virtual void signal()
    {
    }

    virtual void wait()
    {
        PDL_Time_Value sTime;

        sTime.initialize(10);
        ACT_ASSERT(mCond->timedwait(mMutex, &sTime) == IDE_FAILURE);
        ACT_ASSERT(mCond->isTimedOut() == ID_TRUE);
    }
};

class testTimedIgnore : public testThread
{
public:
    testTimedIgnore(iduMutex* aMutex, iduCond* aCond) : testThread(aMutex, aCond) {}
    virtual void signal()
    {
    }

    virtual void wait()
    {
        PDL_Time_Value sTime;

        sTime.initialize(1);
        ACT_ASSERT(mCond->timedwait(mMutex, &sTime, IDU_IGNORE_TIMEDOUT) == IDE_SUCCESS);
        ACT_ASSERT(mCond->isTimedOut() == ID_TRUE);
    }
};

SInt main( void )
{
    iduMutex    sMutex;
    iduCond     sCond;

    testWait        sWait(&sMutex, &sCond);
    testTimedWait   sTimedWait(&sMutex, &sCond);
    testTimedOut    sTimedOut(&sMutex, &sCond);
    testTimedIgnore sTimedIgnore(&sMutex, &sCond);

    ACT_ASSERT(iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) == IDE_SUCCESS);    
    ACT_ASSERT(iduCond::initializeStatic(IDU_CLIENT_TYPE) == IDE_SUCCESS);    

    ACT_ASSERT(sMutex.initialize("TEST",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_THREAD_SYNC) == IDE_SUCCESS);
    ACT_ASSERT(sCond.initialize("TEST") == IDE_SUCCESS);

    idlOS::printf("Wait test\n");
    sWait.test();

    idlOS::printf("TimedWait test\n");
    sTimedWait.test();

    idlOS::printf("TimedOut test\n");
    sTimedOut.test();

    idlOS::printf("TimedOut Ignore test\n");
    sTimedIgnore.test();

    ACT_ASSERT(sCond.destroy() == IDE_SUCCESS);
    ACT_ASSERT(sMutex.destroy() == IDE_SUCCESS);

    ACT_ASSERT(iduCond::destroyStatic() == IDE_SUCCESS);    
    ACT_ASSERT(iduMutexMgr::destroyStatic() == IDE_SUCCESS);    
    ACT_ASSERT(iduMemMgr::destroyStatic() == IDE_SUCCESS);

    return IDE_SUCCESS;
}


