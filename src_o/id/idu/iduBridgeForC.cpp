/***********************************************************************
 * Copyright 1999-2000, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduBridgeForC.cpp 74787 2016-03-21 06:45:48Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <ideCallback.h>
#include <ideLog.h>
#include <iduProperty.h>
#include <idErrorCode.h>
#include <iduBridgeTime.h>
#include <idv.h>

IDL_EXTERN_C_BEGIN

ULong idlOS_getThreadID()
{
    return idlOS::getThreadID();
}

SInt idlOS_getProcessorCount()
{
    return idlVA::getProcessorCount();
}

void idlOS_sleep(ULong aSec, ULong aMicroSec)
{
    PDL_Time_Value      sTimeOut;

    sTimeOut.set(aSec, aMicroSec);
    idlOS::sleep(sTimeOut);
}

void idlOS_thryield(void)
{
    idlOS::thr_yield();
}

SInt  idlOS_rand()
{
    return idlOS::rand();
}

void assertForC(SChar *aCond, SChar *aFile, UInt aLine)
{
    ideLog::log(IDE_SERVER_0,ID_TRC_ASSERT_BEGIN,
                aCond, aFile, aLine);

    ideLog::logCallStack(IDE_SERVER_0);
    ideLog::log(IDE_SERVER_0, ID_TRC_ASSERT_END);
    (void)gCallbackForAssert(aFile, aLine, ID_FALSE);
    
    assert(0);
    idlOS::exit(-1);
}


/* idvTime related functions */
// return ID_SIZEOF(idvTime)
UInt idv_SizeOf_IdvTime()
{
    return ID_SIZEOF(idvTime);
}

// wrapper for IDV_TIME_AVAILABLE() macro
idBool idv_TIME_AVAILABLE()
{
    return (IDV_TIME_AVAILABLE()) ? (ID_TRUE):(ID_FALSE) ;
}


// wrapper for IDV_TIME_GET()
void idv_TIME_GET( iduBridgeTime * aTime )
{
    IDE_ASSERT( ID_SIZEOF(iduBridgeTime) >= ID_SIZEOF(idvTime) );
        
    IDV_TIME_GET( (idvTime*) aTime );
}

// wrapper for IDV_TIME_DIFF_MICRO()
ULong  idv_TIME_DIFF_MICRO(iduBridgeTime * aBeforeTime,
                           iduBridgeTime * aAfterTime)
{
    IDE_ASSERT( ID_SIZEOF(iduBridgeTime) >= ID_SIZEOF(idvTime) );
    
    return IDV_TIME_DIFF_MICRO( (idvTime*) aBeforeTime,
                                (idvTime*) aAfterTime );
}


/* ------------------------------------------------
 *  Read Property
 * ----------------------------------------------*/
UInt iduBridge_getMutexSpinCount()
{
    return iduProperty::getMutexSpinCount();
}

// wrapper for iduProperty::getCheckMutexDurationTimeEnable()
UInt iduBridge_getCheckMutexDurationTimeEnable()
{
    return iduProperty::getCheckMutexDurationTimeEnable();
}

// wrapper for iduProperty::getTimedStatistics()
UInt iduBridge_getTimedStatistcis()
{
    return iduProperty::getTimedStatistics();
}

void idv_BEGIN_WAIT_EVENT( void * aStatSQL,
                           void * aWeArgs )
{
    IDV_BEGIN_WAIT_EVENT( (idvSQL*)aStatSQL,
                          (idvWeArgs*)aWeArgs );
}

void idv_END_WAIT_EVENT( void  * aStatSQL,
                         void  * aWeArgs )
{
    IDV_END_WAIT_EVENT( (idvSQL*)aStatSQL, (idvWeArgs*)aWeArgs );
}

UInt iduLatchMinSleep(void)
{
    return iduProperty::getLatchMinSleep();
}

UInt iduLatchMaxSleep(void)
{
    return iduProperty::getLatchMaxSleep();
}

UInt iduMutexSleepType(void)
{
    return iduProperty::getMutexSleepType();
}

IDL_EXTERN_C_END
