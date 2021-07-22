/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvHandlerDefault.cpp 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idp.h>

/* ------------------------------------------------
 *  Property : TIMER_RUNING_TYPE : none, library
 * ----------------------------------------------*/

static IDE_RC initializeDefault(idvResource ** /*aRsc*/,
                                ULong * /*aClockArea*/,
                                ULong * /*aSecond */)
{
    /* Noting to do */
    return IDE_SUCCESS;
}

static IDE_RC startupDefault(idvResource * /*aRsc*/)
{
    /* Noting to do */
    return IDE_SUCCESS;
}

static IDE_RC shutdownDefault(idvResource * /*aRsc*/)
{
    /* Noting to do */
    return IDE_SUCCESS;
}

static IDE_RC destroyDefault(idvResource * /*aRsc*/)
{
    /* Noting to do */
    return IDE_SUCCESS;
}

idvHandler  gDefaultHandler =
{
    initializeDefault,
    startupDefault,
    shutdownDefault,
    destroyDefault
};




