/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvTimeFuncThread.cpp 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idp.h>
#include <idu.h>
#include <idv.h>

/* ------------------------------------------------
 *  Time Thread  Functions
 * ----------------------------------------------*/

static void   gTimeThreadInit(idvTime *aValue)
{
    aValue->iTime.mClock = 0;
}

static idBool gTimeThreadIsInit(idvTime *aValue)
{
    return (aValue->iTime.mClock == 0) ? ID_TRUE : ID_FALSE;
}

static void   gTimeThreadGet(idvTime *aValue)
{
    aValue->iTime.mClock = idvManager::getClock();
}

static ULong  gTimeThreadDiff(idvTime *aBefore, idvTime *aAfter)
{
    return (aAfter->iTime.mClock - aBefore->iTime.mClock);
}

static ULong  gTimeThreadMicro(idvTime *aValue)
{
    return aValue->iTime.mClock;
}

static idBool  gTimeThreadIsOlder(idvTime *aBefore, idvTime *aAfter)
{
    
    return (gTimeThreadMicro(aBefore) > gTimeThreadMicro(aAfter)) ?
        ID_TRUE : ID_FALSE;
}

idvTimeFunc gTimeThreadFunctions =
{
    gTimeThreadInit,
    gTimeThreadIsInit,
    gTimeThreadGet,
    gTimeThreadDiff,
    gTimeThreadMicro,
    gTimeThreadIsOlder
};

