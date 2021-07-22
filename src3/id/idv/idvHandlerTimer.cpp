/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvHandlerTimer.cpp 82277 2018-02-18 22:21:58Z kclee $
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

class idvTimerThread : public idtBaseThread
{
    idBool            mExitFlag;
    PDL_Time_Value    mTV;
    ULong            *mClockArea;
    ULong            *mSecondArea;
public:
    idvTimerThread();

    IDE_RC initialize(ULong *aClockArea, ULong *aSecondArea);
    IDE_RC destroy();

    void   doShutdown();
    void   changeTimerResolution(UInt aTime);
    void   waitServiceAvail();
    
    virtual ~idvTimerThread() {}
    
    virtual void run();
};

idvTimerThread::idvTimerThread() : idtBaseThread()
{
}

IDE_RC idvTimerThread::initialize(ULong *aClockArea, ULong *aSecondArea)
{
    mExitFlag   = ID_FALSE;
    mClockArea  = aClockArea;
    mSecondArea = aSecondArea;
    
    mTV.set(0, 0);
    
    return IDE_SUCCESS;
}

IDE_RC idvTimerThread::destroy()
{
    return IDE_SUCCESS;
}

void idvTimerThread::run()
{
    PDL_Time_Value sCurTime;
    
    while(mExitFlag == ID_FALSE)
    {
        idlOS::sleep(mTV);

        sCurTime = idlOS::gettimeofday();

        // set micro-second
        (void)acpAtomicSet64(mClockArea,
                             sCurTime.usec() + (sCurTime.sec() * 1000000) );
        *mSecondArea = sCurTime.sec();
    }
}



void    idvTimerThread::doShutdown()
{
    mExitFlag = ID_TRUE;
}

void    idvTimerThread::changeTimerResolution(UInt aTime)
{
    mTV.set(0, aTime);
}

void    idvTimerThread::waitServiceAvail()
{
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
    idvTimerThread mTimerThread;
}idvResource;

static idvTimerThread *gTimerThread;

static IDE_RC callbackTimeThreadResolution(
    idvSQL * /*aStatistics*/,
    SChar  * /*aName*/,
    void   * /*aOldValue*/,
    void   * aNewValue,
    void   * /*aArg*/)
{
    gTimerThread->changeTimerResolution(*((UInt *)aNewValue));

    return IDE_SUCCESS;
}

/* ------------------------------------------------
 *  Functions
 * ----------------------------------------------*/

static IDE_RC initializeTimer(idvResource **aRsc,
                              ULong        *aClockArea,
                              ULong        *aSecondArea)
{
    UInt            sTimerValue;
    
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_ID_TIMER_MANAGER,
                               ID_SIZEOF(idvResource),
                               (void**)aRsc,
                               IDU_MEM_IMMEDIATE)
             != IDE_SUCCESS);

    idlOS::memset(*aRsc, 0, ID_SIZEOF(idvResource));

    // Initialize Members

#ifdef __CSURF__
    new (&((*aRsc)->mTimerThread)) idvTimerThread;
    gTimerThread = &((*aRsc)->mTimerThread);
#else
    gTimerThread = new (&((*aRsc)->mTimerThread)) idvTimerThread;
#endif

    IDE_TEST(gTimerThread->initialize(aClockArea, aSecondArea) != IDE_SUCCESS);
    
    /* ------------------------------------------------
     *  Property Callback Registration
     * ----------------------------------------------*/

    IDE_ASSERT(idp::read("TIMER_THREAD_RESOLUTION", &sTimerValue) == IDE_SUCCESS);

    IDE_ASSERT(idp::setupAfterUpdateCallback("TIMER_THREAD_RESOLUTION",
                                             callbackTimeThreadResolution)
               == IDE_SUCCESS);
    
    gTimerThread->changeTimerResolution(sTimerValue);
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC startupTimer(idvResource *aRsc)
{
    IDE_TEST(aRsc->mTimerThread.start() != IDE_SUCCESS);
    IDE_TEST(aRsc->mTimerThread.waitToStart(0) != IDE_SUCCESS);

    // mClock이 초기화될때까지 대기
    aRsc->mTimerThread.waitServiceAvail();
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC shutdownTimer(idvResource *aRsc)
{
    aRsc->mTimerThread.doShutdown();
    IDE_ASSERT(aRsc->mTimerThread.join() == IDE_SUCCESS);
    
    return IDE_SUCCESS;
    /*
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
    */
}

static IDE_RC destroyTimer(idvResource *aRsc)
{
    IDE_ASSERT(iduMemMgr::free(aRsc) == IDE_SUCCESS);

    return IDE_SUCCESS;
}

idvHandler  gTimeThreadHandler =
{
    initializeTimer,
    startupTimer,
    shutdownTimer,
    destroyTimer
};

