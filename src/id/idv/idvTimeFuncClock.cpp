/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvTimeFuncClock.cpp 91519 2021-08-24 05:09:06Z kclee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idp.h>
#include <idu.h>
#include <idv.h>

/* ------------------------------------------------
 * Clock Tick
 * ----------------------------------------------*/

static void   gClockInit(idvTime *aValue)
{
    aValue->iTime.mClock = 0;
}

static idBool gClockIsInit(idvTime *aValue)
{
    return (aValue->iTime.mClock == 0) ? ID_TRUE : ID_FALSE;
}

static void   gClockGet(idvTime *aValue)
{
    aValue->iTime.mClock = idvGetClockTickFromSystem();
}

// call by callback. convert to micro second.
static ULong  gClockDiff(idvTime *aBefore, idvTime *aAfter)
{
    ULong  sClock = idvManager::getClock();

    //BUG-49253  : fix divide by zero
    if(sClock == 0) 
    {
        sClock = 1;
    }
    
    return (aAfter->iTime.mClock - aBefore->iTime.mClock) / sClock;
}

static ULong  gClockMicro(idvTime *aValue)
{
    ULong  sClock = idvManager::getClock();

    //BUG-49253  : fix divide by zero
    if(sClock == 0) 
    {
        sClock = 1;
    }

    return (aValue->iTime.mClock / sClock);
}

static idBool gClockIsOlder(idvTime *aBefore, idvTime *aAfter)
{
    return ( aBefore->iTime.mClock > aAfter->iTime.mClock ) ? ID_TRUE : ID_FALSE;
}


idvTimeFunc gClockFunctions =
{
    gClockInit,
    gClockIsInit,
    gClockGet,
    gClockDiff,
    gClockMicro,
    gClockIsOlder
};

