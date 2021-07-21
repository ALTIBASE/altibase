/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduTraceCode.cpp 68698 2015-01-28 02:32:20Z djin $
 **********************************************************************/

/***********************************************************************
 * NAME
 *  iduTraceCode.cpp
 *
 * DESCRIPTION
 *  This file defines various Trace Message.
 *
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduVersion.h>
#include <iduStack.h>
#include <idp.h>


const SChar *gIDTraceCode[] =
{
#include "ID_TRC_CODE.ic"
    NULL
};
