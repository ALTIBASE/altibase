/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvHandlerClock.cpp 82277 2018-02-18 22:21:58Z kclee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idp.h>
#include <idu.h>
#include <idtBaseThread.h>

/* ------------------------------------------------
 *  Property : TIMER_RUNING_TYPE : time thread, clock
 * ----------------------------------------------*/

class idvClockThread : public idtBaseThread
{
    idBool            mExitFlag;
    PDL_Time_Value    mTV;
    ULong            *mClockArea;
    ULong            *mSecondArea;
    ULong             mGapOfTickAvg;
    ULong             mSleepMicro;

public:
    idvClockThread();

    IDE_RC initialize(ULong *aClockArea, ULong *aSecondArea);
    IDE_RC destroy();

    void   doShutdown();
    void   changeClockResolution(UInt aTime);
    void   waitServiceAvail();

    virtual ~idvClockThread() {}

    virtual void run();
};

idvClockThread::idvClockThread() : idtBaseThread()
{
}

IDE_RC idvClockThread::initialize(ULong *aClockArea, ULong *aSecondArea)
{
    mExitFlag   = ID_FALSE;
    mClockArea  = aClockArea;/*no need to use atomic because nobody knows it.*/
    mSecondArea = aSecondArea;
    mGapOfTickAvg   = 0;
    mTV.set(0, 0);
    mSleepMicro = 0;
    return IDE_SUCCESS;
}

IDE_RC idvClockThread::destroy()
{
    return IDE_SUCCESS;
}

void idvClockThread::run()
{
    ULong          sBeforeTick;
    ULong          sAfterTick;
    PDL_Time_Value sCurTime;

    while(mExitFlag == ID_FALSE)
    {
        sBeforeTick = idvGetClockTickFromSystem();
        idlOS::sleep(mTV);
        sAfterTick  = idvGetClockTickFromSystem();
        
        sCurTime     = idlOS::gettimeofday();
        *mSecondArea = sCurTime.sec();

        if (mGapOfTickAvg == 0) // 초기 상태
        {
            mGapOfTickAvg = sAfterTick - sBeforeTick;
        }
        else
        {
            ULong sTmpCur;

            sTmpCur       = sAfterTick - sBeforeTick;
            mGapOfTickAvg = (mGapOfTickAvg + sTmpCur) / 2;
        }

	
        // set micro-second
	(void)acpAtomicSet64(mClockArea, mGapOfTickAvg / mSleepMicro); /*BUG-45547*/

#ifdef NOTDEF
        ideLog::log(IDE_SERVER_0, " Clock Value = %llu"
                    " sleepMicro = %llu : %lluusec calc\n",
                    acpAtomicGet64(mClockArea),
                    mSleepMicro,
                    ((sAfterTick - sBeforeTick) / acpAtomicGet64(mClockArea)) );
#endif
    }
}

void    idvClockThread::doShutdown()
{
    mExitFlag = ID_TRUE;
}

void    idvClockThread::changeClockResolution(UInt aTime)
{
    mSleepMicro = aTime;

    mTV.set(0, mSleepMicro);
}

void   idvClockThread::waitServiceAvail() 
{
    // thread가 실행되어서 clock 초기화가 될때까지 
    // 대기하다 리턴한다. 
    while( acpAtomicGet64(mClockArea) == 0 )
    {
        idlOS::sleep(1);
    }
}


/* ------------------------------------------------
 *  Property : USE_TIMER_THREAD = 1
 * ----------------------------------------------*/
typedef struct idvResource
{
    idvClockThread mClockThread;
}idvResource;

static idvClockThread *gClockThread;

static IDE_RC callbackTimeThreadResolution(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/)
{
    gClockThread->changeClockResolution(*((UInt *)aNewValue));

    return IDE_SUCCESS;
}

/* ------------------------------------------------
 *  Functions
 * ----------------------------------------------*/

static IDE_RC initializeClock(idvResource **aRsc,
                              ULong *aClockArea,
                              ULong *aSecondArea)
{
    UInt            sTime = 1000000; /* 1 sec */
    UInt            sClockValue;

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_ID_CLOCK_MANAGER,
                               ID_SIZEOF(idvResource),
                               (void**)aRsc,
                               IDU_MEM_IMMEDIATE)
             != IDE_SUCCESS);

    idlOS::memset(*aRsc, 0, ID_SIZEOF(idvResource));

    // Initialize Members

#ifdef __CSURF__
    new (&((*aRsc)->mClockThread)) idvClockThread;
    gClockThread = &((*aRsc)->mClockThread);
#else
    gClockThread = new (&((*aRsc)->mClockThread)) idvClockThread;
#endif
    IDE_TEST(gClockThread->initialize(aClockArea, aSecondArea) != IDE_SUCCESS);

    /* ------------------------------------------------
     *  Property Callback Registration
     * ----------------------------------------------*/

    IDE_ASSERT(idp::update(NULL/* aStatistics */,
                           (SChar*)"TIMER_THREAD_RESOLUTION",
                           sTime) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("TIMER_THREAD_RESOLUTION", &sClockValue) == IDE_SUCCESS);

    IDE_ASSERT(idp::setupAfterUpdateCallback("TIMER_THREAD_RESOLUTION",
                                             callbackTimeThreadResolution)
               == IDE_SUCCESS);

    gClockThread->changeClockResolution(sClockValue);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC startupClock(idvResource *aRsc)
{
    IDE_TEST(aRsc->mClockThread.start() != IDE_SUCCESS);
    IDE_TEST(aRsc->mClockThread.waitToStart(0) != IDE_SUCCESS);

    // thread available 할때까지 대기한다. 
    aRsc->mClockThread.waitServiceAvail();

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC shutdownClock(idvResource *aRsc)
{
    aRsc->mClockThread.doShutdown();
    IDE_ASSERT(aRsc->mClockThread.join() == IDE_SUCCESS);

    return IDE_SUCCESS;
    /*
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
    */
}

static IDE_RC destroyClock(idvResource *aRsc)
{
    IDE_ASSERT(iduMemMgr::free(aRsc) == IDE_SUCCESS);

    return IDE_SUCCESS;
}

idvHandler  gClockThreadHandler =
{
    initializeClock,
    startupClock,
    shutdownClock,
    destroyClock
};

