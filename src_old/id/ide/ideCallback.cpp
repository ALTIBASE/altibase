/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: ideCallback.cpp 74787 2016-03-21 06:45:48Z jake.jang $
 **********************************************************************/

/***********************************************************************
 * NAME
 *  ideCallback.cpp
 *
 * DESCRIPTION
 *  This file defines various Trace Message Callbacks
 *
 **********************************************************************/

#include <idl.h>
#include <ideCallback.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <iduVersion.h>
#include <iduStack.h>
#include <idp.h>



/* ----------------------------------------------------------------------
 *  Message Callback Function
 * ----------------------------------------------------------------------*/
IDE_RC ideNullCallbackFuncForMessage(
    const SChar * /*_ msg  _*/,
    SInt          /*_ flag _*/,
    idBool        /*_ aLogMsg _*/)
{
    return IDE_SUCCESS;
}

void ideNullCallbackFuncForFatal(
    SChar  * /*_ file _*/,
    SInt     /*_ linenum _*/,
    SChar  * /*_ msg _*/ )
{
}

void ideNullCallbackFuncForWarning(
    SChar  * /*_ file _*/,
    SInt     /*_ linenum _*/,
    SChar  * /*_ msg _*/ )
{
}

void ideNullCallbackFuncForErrlog(
    SChar  * /*_ file _*/,
    SInt     /*_ linenum _*/)
{
}

idBool ideNullCallbackFuncForAssert(
    SChar  * /*_ file _*/,
    SInt     /*_ linenum _*/,
    idBool   /*_ aAcceptFaultTolerance _*/)
{
    return ID_TRUE;
}

//fix PROJ-1749
idBool ideNullCallbackFuncForDumpStack()
{
    return ID_FALSE;
}

IDE_RC ideNullCallbackFuncForNChar()
{
    return IDE_SUCCESS;
}

// Message Callback Function
// Assert를 위한 callback 함수 추가.    by kumdory, 2004-03-11

ideCallbackForMessage   gCallbackForMessage   = ideNullCallbackFuncForMessage;
ideCallbackForFatal     gCallbackForFatal     = ideNullCallbackFuncForFatal;
ideCallbackForWarning   gCallbackForWarning   = ideNullCallbackFuncForWarning;
ideCallbackForErrlog    gCallbackForErrlog    = ideNullCallbackFuncForErrlog;
ideCallbackForAssert    gCallbackForAssert    = ideNullCallbackFuncForAssert;
ideCallbackForDumpStack gCallbackForDumpStack = ideNullCallbackFuncForDumpStack;
ideCallbackForNChar     gCallbackForNChar     = ideNullCallbackFuncForNChar;

void ideFinalizeCallbackFunctions( void )
{
    /* BUG-34359 */
    gCallbackForMessage   = ideNullCallbackFuncForMessage;
    gCallbackForFatal     = ideNullCallbackFuncForFatal;
    gCallbackForWarning   = ideNullCallbackFuncForWarning;
    gCallbackForErrlog    = ideNullCallbackFuncForErrlog;
    gCallbackForAssert    = ideNullCallbackFuncForAssert;
    gCallbackForDumpStack = ideNullCallbackFuncForDumpStack;
    gCallbackForNChar     = ideNullCallbackFuncForNChar;
}

void ideSetCallbackFunctions(
    ideCallbackForMessage aFuncPtrForMessage,
    ideCallbackForFatal   aFuncPtrForFatal,
    ideCallbackForWarning aFuncPtrForWarning,
    ideCallbackForErrlog  aFuncPtrForErrlog)
{
    gCallbackForMessage = aFuncPtrForMessage;
    gCallbackForFatal   = aFuncPtrForFatal;
    gCallbackForWarning = aFuncPtrForWarning;
    gCallbackForErrlog  = aFuncPtrForErrlog;
}

void ideSetCallbackMessage( ideCallbackForMessage aFuncPtrForMessage)
{
    gCallbackForMessage = aFuncPtrForMessage;
}

void ideSetCallbackFatal  ( ideCallbackForFatal   aFuncPtrForFatal)
{
    gCallbackForFatal   = aFuncPtrForFatal;
}

void ideSetCallbackWarning( ideCallbackForWarning aFuncPtrForWarning)
{
    gCallbackForWarning = aFuncPtrForWarning;
}

void ideSetCallbackErrlog ( ideCallbackForErrlog  aFuncPtrForErrlog)
{
    gCallbackForErrlog  = aFuncPtrForErrlog;
}

void ideSetCallbackAssert(  ideCallbackForAssert  aFuncPtrForAssert)
{
    gCallbackForAssert  = aFuncPtrForAssert;
}

//fix PROJ-1749
void ideSetCallbackDumpStack( ideCallbackForDumpStack aFuncPtrForDumpStack)
{
    gCallbackForDumpStack = aFuncPtrForDumpStack;
}

void ideSetCallbackNChar(  ideCallbackForNChar  aFuncPtrForNChar)
{
    gCallbackForNChar  = aFuncPtrForNChar;
}

void ideAssert( const SChar * aSource,
                idBool        aAcceptFaultTolerance, /* PROJ-2617 */
                const SChar * aFile,
                SInt          aLine )
{
    ideLogEntry sLog(IDE_ERR_0);

    sLog.appendFormat("%s[%s:%u], errno=[%u]\n",
                      aSource,
                      (SChar *)idlVA::basename(aFile),
                      aLine,
                      errno);
    sLog.write();

    ideLog::logCallStackInternal();

    if (gCallbackForAssert((SChar *)idlVA::basename(aFile),
                           aLine, aAcceptFaultTolerance) == ID_TRUE)
    {
        os_assert(0);
        idlOS::exit(-1);
    }
    else
    {
        /* PROJ-2617 */
        ideNonLocalJumpForFaultTolerance();
    }
}

/* PROJ-2118 BUG Reporting
 * Message 기록 Assert 매크로 추가 */
void ideAssertMsg( const SChar * aSource,
                   idBool        aAcceptFaultTolerance, /* PROJ-2617 */
                   const SChar * aFile,
                   SInt          aLine,
                   const SChar * aFormat,
                   ... )
{
    va_list ap;

    ideLogEntry sLog(IDE_ERR_0);

    sLog.appendFormat("%s[%s:%u], errno=[%u]\n",
                      aSource,
                      (SChar *)idlVA::basename(aFile),
                      aLine,
                      errno );

    va_start( ap, aFormat );

    sLog.appendArgs( aFormat,
                     ap );
    va_end( ap );

    sLog.append( "\n" );
    sLog.write();

    ideLog::logCallStackInternal();

    if (gCallbackForAssert((SChar *)idlVA::basename(aFile),
                           aLine, aAcceptFaultTolerance) == ID_TRUE)
    {
        os_assert(0);
        idlOS::exit(-1);
    }
    else
    {
        /* PROJ-2617 */
        ideNonLocalJumpForFaultTolerance();
    }
}

