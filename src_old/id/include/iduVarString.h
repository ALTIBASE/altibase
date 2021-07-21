/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduVarString.h 68602 2015-01-23 00:13:11Z sbjang $
 ****************************************************************************/

#ifndef _O_IDU_VAR_STRING_H_
#define _O_IDU_VAR_STRING_H_

#include <idl.h>
#include <ide.h>
#include <idu.h>


struct iduVarString;
struct iduVarStringPiece;


typedef struct iduVarString
{
    iduMemPool *mPiecePool;

    SChar      *mBuffer;

    iduList     mPieceList;
    UInt        mPieceSize;

    UInt        mLength;
} iduVarString;

typedef struct iduVarStringPiece
{
    iduListNode mPieceListNode;
    UInt        mLength;

    SChar       mData[1];
} iduVarStringPiece;


IDE_RC iduVarStringInitialize(iduVarString *aString,
                              iduMemPool   *aPool,
                              UInt          aPieceSize);
IDE_RC iduVarStringFinalize(iduVarString *aString);

UInt   iduVarStringGetLength(iduVarString *aString);

IDE_RC iduVarStringTruncate(iduVarString *aString, idBool aReserveFlag);

IDE_RC iduVarStringPrint(iduVarString *aString, const SChar *aCString);
IDE_RC iduVarStringPrintLength(iduVarString *aString, const SChar *aCString, UInt aLength);
IDE_RC iduVarStringPrintFormat(iduVarString *aString, const SChar *aFormat, ...);
IDE_RC iduVarStringPrintFormatV(iduVarString *aString, const SChar *aFormat, va_list aArg);

IDE_RC iduVarStringAppend(iduVarString *aString, const SChar *aCString);
IDE_RC iduVarStringAppendLength(iduVarString *aString, const SChar *aCString, UInt aLength);
IDE_RC iduVarStringAppendFormat(iduVarString *aString, const SChar *aFormat, ...);
IDE_RC iduVarStringAppendFormatV(iduVarString *aString, const SChar *aFormat, va_list aArg);

IDE_RC iduVarStringConvToCString( iduVarString *aSrc, UInt aDstBufLen, SChar *aDstBuf );

#endif
