/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_SDPJ_MANAGER_H_
#define _O_SDPJ_MANAGER_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <sdi.h>

typedef enum
{
    SDPJ_OBJECT = 0,
    SDPJ_ARRAY,
    SDPJ_STRING,
    SDPJ_LONG,
    SDPJ_DOUBLE,
    SDPJ_TRUE,
    SDPJ_FALSE,
    SDPJ_NULL
} sdpjValueType;

typedef struct sdpjString
{
    SInt         mOffset;  /* jString 크기를 줄이기위해 pointer가 아니라 offset을 사용한다. */
    SInt         mSize;
} sdpjString;

typedef struct sdpjValue
{
    sdpjValueType   mType;
    union
    {
        sdpjString          mString;
        struct sdpjObject * mObject;
        struct sdpjArray  * mArray;
        SLong               mLong;
        SDouble             mDouble;
    } mValue;
} sdpjValue;

typedef struct sdpjArray
{
    sdpjValue       mValue;
    sdpjArray     * mNext;
} sdpjArray;

typedef struct sdpjKeyValue
{
    sdpjString      mKey;
    sdpjValue       mValue;
} sdpjKeyValue;

typedef struct sdpjObject
{
    sdpjKeyValue    mKeyValue;
    sdpjObject    * mNext;
} sdpjObject;

class sdpjManager
{
public:
    static IDE_RC parseIt( SChar        * aParseBuffer,
                           SInt           aParseBufferLength,
                           SChar        * aText,
                           SInt           aTextLength,
                           sdpjObject  ** aObject );

    static SInt getProgress( void * aLexer );
};

#endif  /* _O_SDPJ_MANAGER_H_ */

