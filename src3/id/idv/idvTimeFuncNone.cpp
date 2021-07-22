/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvTimeFuncNone.cpp 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idp.h>
#include <idu.h>
#include <idv.h>

/* ------------------------------------------------
 *  None Functions
 * ----------------------------------------------*/

static void   gNoneInit(idvTime *)
{
}

static idBool gNoneIsInit(idvTime *)
{
    return ID_TRUE;
}

static idBool gNoneIsOlder(idvTime *, idvTime *)
{
    return ID_TRUE;
}

static void   gNoneGet(idvTime *)
{
}

static ULong  gNoneDiff(idvTime *, idvTime *)
{
    return 0;
}

static ULong  gNoneMicro(idvTime *)
{
    return 0;
}


idvTimeFunc gNoneFunctions =
{
    gNoneInit,
    gNoneIsInit,
    gNoneGet,
    gNoneDiff,
    gNoneMicro,
    gNoneIsOlder
};


