/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _SDPJX_H_
#define _SDPJX_H_ 1

#include <iduMemory.h>
#include <qc.h>
#include <sdi.h>
#include <sdpjManager.h>

class sdpjLexer;

typedef struct sdpjx
{
    sdpjLexer    * mLexer;
    SChar        * mParseBuffer;
    UInt           mParseBufferLength;
    UInt           mAllocCount;
    SChar        * mText;
    sdpjObject   * mObject;  /* out */
} sdpjx;

#endif /* _SDPJX_H_ */
