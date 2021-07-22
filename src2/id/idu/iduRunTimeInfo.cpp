/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduRunTimeInfo.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <iduRunTimeInfo.h>

iduRunTimeInfo::iduRunTimeInfo()
{

#define IDE_FN "iduRunTimeInfo::iduRunTimeInfo()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#undef IDE_FN
}

iduRunTimeInfo::~iduRunTimeInfo(void)
{
#define IDE_FN "iduRunTimeInfo::~iduRunTimeInfo(void)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#undef IDE_FN
}

IDE_RC iduRunTimeInfo::initialize()
{
#define IDE_FN "SInt iduRunTimeInfo::initialize()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC iduRunTimeInfo::destroy()
{
#define IDE_FN "SInt iduRunTimeInfo::destroy()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC iduRunTimeInfo::add(SChar * /*strName*/,
                           iduStatusObject * /*statusObjectFunc*/)
{
#define IDE_FN "SInt iduRunTimeInfo::add()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;
    
#undef IDE_FN
}
