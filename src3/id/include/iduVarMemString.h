/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*****************************************************************************
 * $Id$
 ****************************************************************************/

#ifndef _O_IDU_VAR_MEM_STRING_H_
#define _O_IDU_VAR_MEM_STRING_H_

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <iduVarMemList.h>


struct iduVarMemString;
struct iduVarMemStringPiece;


typedef struct iduVarMemString
{
    iduVarMemList *mVarMem;

    SChar         *mBuffer;

    iduList        mPieceList;
    UInt           mPieceSize;

    UInt           mLength;
} iduVarMemString;

typedef struct iduVarMemStringPiece
{
    iduListNode mPieceListNode;
    UInt        mLength;

    SChar       mData[1];
} iduVarMemStringPiece;


IDE_RC iduVarMemStringInitialize(iduVarMemString *aString,
                                 iduVarMemList   *aVarMem,
                                 UInt             aPieceSize);
IDE_RC iduVarMemStringFinalize(iduVarMemString *aString);

UInt   iduVarMemStringGetLength(iduVarMemString *aString);

IDE_RC iduVarMemStringTruncate(iduVarMemString *aString, idBool aReserveFlag);

IDE_RC iduVarMemStringPrint(iduVarMemString *aString, const SChar *aCString);
IDE_RC iduVarMemStringPrintLength(iduVarMemString *aString, const SChar *aCString, UInt aLength);
IDE_RC iduVarMemStringPrintFormat(iduVarMemString *aString, const SChar *aFormat, ...);
IDE_RC iduVarMemStringPrintFormatV(iduVarMemString *aString, const SChar *aFormat, va_list aArg);

IDE_RC iduVarMemStringAppend(iduVarMemString *aString, const SChar *aCString);
IDE_RC iduVarMemStringAppendLength(iduVarMemString *aString, const SChar *aCString, UInt aLength);
IDE_RC iduVarMemStringAppendFormat(iduVarMemString *aString, const SChar *aFormat, ...);
IDE_RC iduVarMemStringAppendFormatV(iduVarMemString *aString, const SChar *aFormat, va_list aArg);
IDE_RC iduVarMemStringExtractString(iduVarMemString *aString, SChar *aBuffer, UInt aBufferSize );


#endif
