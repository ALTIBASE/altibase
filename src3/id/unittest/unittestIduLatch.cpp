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
#include <idp.h>
#include <idtBaseThread.h>
#include <iduCond.h>
#include <iduLatch.h>

class testReadThread : public idtBaseThread
{
public:
    testReadThread() : idtBaseThread(IDT_JOINABLE)
    {
        mCont = ID_TRUE;
    }

    void initialize(iduLatch* aLatch, SInt* aBeforeCount, SInt* aAfterCount)
    {
        mLatch = aLatch;
        mBeforeCount = aBeforeCount;
        mAfterCount = aAfterCount;
    }

    virtual IDE_RC initializeThread(void)
    {
        return IDE_SUCCESS;
    }

    virtual void run(void)
    {
        acpAtomicInc32(mBeforeCount);
        IDE_ASSERT(mLatch->lockRead(NULL, NULL) == IDE_SUCCESS);
        while(mCont == ID_TRUE)
        {
            idlOS::thr_yield();
        }
    }

    virtual void finalizeThread(void)
    {
        acpAtomicInc32(mAfterCount);
        IDE_ASSERT(mLatch->unlock() == IDE_SUCCESS);
    }

    iduLatch*   mLatch;
    SInt*       mBeforeCount;
    SInt*       mAfterCount;
    idBool      mCont;
};

class testWriteThread : public idtBaseThread
{
public:
    testWriteThread() : idtBaseThread(IDT_JOINABLE)
    {
        mCont = ID_TRUE;
    }

    void initialize(iduLatch* aLatch, SInt* aCount)
    {
        mLatch = aLatch;
        mCount = aCount;
    }

    virtual IDE_RC initializeThread(void)
    {
        return mLatch->lockWrite(NULL, NULL);
    }

    virtual void run(void)
    {
        while(mCont == ID_TRUE)
        {
            idlOS::thr_yield();
        }
    }

    virtual void finalizeThread(void)
    {
        IDE_ASSERT(mLatch->unlock() == IDE_SUCCESS);
    }

    iduLatch*   mLatch;
    SInt*       mCount;
    idBool      mCont;
};

IDE_RC testFunc(iduLatchType aType)
{
    idBool      sSuccess;
    iduLatch    sLatch;
    SInt        sBeforeCount;
    SInt        sAfterCount;
    SInt        i;

    testReadThread  sReads[4];
    testWriteThread sWrite;

    sBeforeCount = 0;
    sAfterCount = 0;

    IDE_TEST( sLatch.initialize("TESTLATCH", aType) != IDE_SUCCESS ); 

    sWrite.initialize(&sLatch, &sBeforeCount);
    IDE_TEST( sWrite.start() != IDE_SUCCESS );

    IDE_TEST( sLatch.tryLockRead(&sSuccess) != IDE_SUCCESS );
    IDE_ASSERT( sSuccess == ID_FALSE );

    for(i = 0; i < 4; i++)
    {
        sReads[i].initialize(&sLatch, &sBeforeCount, &sAfterCount);
        IDE_TEST( sReads[i].start() != IDE_SUCCESS );
    }

    while(sBeforeCount < 4)
    {
        idlOS::thr_yield();
    }

    sWrite.mCont = ID_FALSE;
    IDE_TEST( sWrite.join() != IDE_SUCCESS );

    for(i = 0; i < 4; i++)
    {
        sReads[i].mCont = ID_FALSE;
        IDE_TEST( sReads[i].join() != IDE_SUCCESS );
    }

    IDE_TEST( sLatch.destroy() != IDE_SUCCESS ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

SInt main( void )
{
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) == IDE_SUCCESS);    
    ACT_ASSERT(iduLatch::initializeStatic(IDU_CLIENT_TYPE) == IDE_SUCCESS);

    ACT_ASSERT(testFunc(IDU_LATCH_TYPE_POSIX) == IDE_SUCCESS);
    ACT_ASSERT(testFunc(IDU_LATCH_TYPE_POSIX2) == IDE_SUCCESS);
    ACT_ASSERT(testFunc(IDU_LATCH_TYPE_NATIVE) == IDE_SUCCESS);
    ACT_ASSERT(testFunc(IDU_LATCH_TYPE_NATIVE2) == IDE_SUCCESS);

    ACT_ASSERT(iduLatch::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::destroyStatic() == IDE_SUCCESS);

    return IDE_SUCCESS;
}


