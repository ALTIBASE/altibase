/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <sdpjManager.h>
#include "sdpjx.h"

#if defined(BISON_POSTFIX_HPP)
#include "sdpjy.hpp"
#else /* BISON_POSTFIX_CPP_H */
#include "sdpjy.cpp.h"
#endif

#include "sdpjl.h"

extern int sdpjparse(void *param);

IDE_RC sdpjManager::parseIt( SChar        * aParseBuffer,
                             SInt           aParseBufferLength,
                             SChar        * aText,
                             SInt           aTextLength,
                             sdpjObject  ** aObject )
{
    sdpjLexer  s_sdpjLexer(aText, aTextLength);
    sdpjx      s_sdpjx;

    s_sdpjx.mLexer             = &s_sdpjLexer;
    s_sdpjx.mParseBuffer       = aParseBuffer;
    s_sdpjx.mParseBufferLength = aParseBufferLength;
    s_sdpjx.mAllocCount        = 0;
    s_sdpjx.mText              = aText;

    IDE_TEST( sdpjparse(&s_sdpjx) != IDE_SUCCESS );

    *aObject = s_sdpjx.mObject;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SInt sdpjManager::getProgress( void * aLexer )
{
    return ((sdpjLexer*)aLexer)->getProgress();
}
