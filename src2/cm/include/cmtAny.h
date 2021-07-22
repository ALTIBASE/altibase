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

#ifndef _O_CMT_ANY_H_
#define _O_CMT_ANY_H_ 1


typedef struct cmtAny
{
    UChar mType;

    union
    {
        SChar         mSInt8;
        UChar         mUInt8;
        SShort        mSInt16;
        UShort        mUInt16;
        SInt          mSInt32;
        UInt          mUInt32;
        SLong         mSInt64;
        ULong         mUInt64;
        SFloat        mFloat32;
        SDouble       mFloat64;
        cmtDateTime   mDateTime;
        cmtInterval   mInterval;
        cmtNumeric    mNumeric;
        cmtVariable   mVariable;
        cmtInVariable mInVariable;
        cmtBit        mBit;
        cmtInBit      mInBit;
        cmtNibble     mNibble;
        cmtInNibble   mInNibble;
        cmtLobLocator mLobLocator; /* BUG-18945 */
    } mValue;

} cmtAny;


IDE_RC cmtAnyInitialize(cmtAny *aAny);
IDE_RC cmtAnyFinalize(cmtAny *aAny);

UChar  cmtAnyGetType(cmtAny *aAny);

idBool cmtAnyIsNull(cmtAny *aAny);
IDE_RC cmtAnySetNull(cmtAny *aAny);
IDE_RC cmtAnySetRedundancy( cmtAny *aAny ); // PROJ-2256

IDE_RC cmtAnyReadSChar(cmtAny *aAny, SChar *aValue);
IDE_RC cmtAnyReadUChar(cmtAny *aAny, UChar *aValue);
IDE_RC cmtAnyReadSShort(cmtAny *aAny, SShort *aValue);
IDE_RC cmtAnyReadUShort(cmtAny *aAny, UShort *aValue);
IDE_RC cmtAnyReadSInt(cmtAny *aAny, SInt *aValue);
IDE_RC cmtAnyReadUInt(cmtAny *aAny, UInt *aValue);
IDE_RC cmtAnyReadSLong(cmtAny *aAny, SLong *aValue);
IDE_RC cmtAnyReadULong(cmtAny *aAny, ULong *aValue);
IDE_RC cmtAnyReadSFloat(cmtAny *aAny, SFloat *aValue);
IDE_RC cmtAnyReadSDouble(cmtAny *aAny, SDouble *aValue);
IDE_RC cmtAnyReadDateTime(cmtAny *aAny, cmtDateTime **aValue);
IDE_RC cmtAnyReadInterval(cmtAny *aAny, cmtInterval **aValue);
IDE_RC cmtAnyReadNumeric(cmtAny *aAny, cmtNumeric **aValue);
IDE_RC cmtAnyReadVariable(cmtAny *aAny, cmtVariable **aValue);
IDE_RC cmtAnyReadInVariable(cmtAny *aAny, cmtInVariable **aValue);
IDE_RC cmtAnyReadBinary(cmtAny *aAny, cmtVariable **aValue);
IDE_RC cmtAnyReadInBinary(cmtAny *aAny, cmtInVariable **aValue);
IDE_RC cmtAnyReadBit(cmtAny *aAny, cmtBit **aValue);
IDE_RC cmtAnyReadInBit(cmtAny *aAny, cmtInBit **aValue);
IDE_RC cmtAnyReadNibble(cmtAny *aAny, cmtNibble **aValue);
IDE_RC cmtAnyReadInNibble(cmtAny *aAny, cmtInNibble **aValue);
IDE_RC cmtAnyReadLobLocator(cmtAny *aAny, ULong *aLocator, UInt *aSize);
/* PROJ-1920 */
IDE_RC cmtAnyReadPtr(cmtAny *aAny, cmtInVariable **aValue);

IDE_RC cmtAnyWriteSChar(cmtAny *aAny, SChar aValue);
IDE_RC cmtAnyWriteUChar(cmtAny *aAny, UChar aValue);
IDE_RC cmtAnyWriteSShort(cmtAny *aAny, SShort aValue);
IDE_RC cmtAnyWriteUShort(cmtAny *aAny, UShort aValue);
IDE_RC cmtAnyWriteSInt(cmtAny *aAny, SInt aValue);
IDE_RC cmtAnyWriteUInt(cmtAny *aAny, UInt aValue);
IDE_RC cmtAnyWriteSLong(cmtAny *aAny, SLong aValue);
IDE_RC cmtAnyWriteULong(cmtAny *aAny, ULong aValue);
IDE_RC cmtAnyWriteSFloat(cmtAny *aAny, SFloat aValue);
IDE_RC cmtAnyWriteSDouble(cmtAny *aAny, SDouble aValue);
IDE_RC cmtAnyWriteDateTime(cmtAny *aAny, cmtDateTime *aValue);
IDE_RC cmtAnyWriteInterval(cmtAny *aAny, cmtInterval *aValue);
IDE_RC cmtAnyWriteNumeric(cmtAny *aAny, cmtNumeric *aValue);
IDE_RC cmtAnyWriteVariable(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize);
IDE_RC cmtAnyWriteInVariable(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize);
IDE_RC cmtAnyWriteBinary(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize);
IDE_RC cmtAnyWriteInBinary(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize);
IDE_RC cmtAnyWriteBit(cmtAny *aAny, UChar *aBuffer, UInt aPrecision);
IDE_RC cmtAnyWriteInBit(cmtAny *aAny, UChar *aBuffer, UInt aPrecision);
IDE_RC cmtAnyWriteNibble(cmtAny *aAny, UChar *aBuffer, UInt aPrecision);
IDE_RC cmtAnyWriteInNibble(cmtAny *aAny, UChar *aBuffer, UInt aPrecision);
IDE_RC cmtAnyWriteLobLocator(cmtAny *aAny, ULong aLocator, UInt aSize);
/* PROJ-1920 */
IDE_RC cmtAnyWritePtr(cmtAny *aAny, UChar *aValue, UInt aSize);

IDE_RC cmtAnyGetDateTimeForWrite(cmtAny *aAny, cmtDateTime **aValue);
IDE_RC cmtAnyGetIntervalForWrite(cmtAny *aAny, cmtInterval **aValue);
IDE_RC cmtAnyGetNumericForWrite(cmtAny *aAny, cmtNumeric **aValue, UChar aSize);

// To fix BUG-20195
UInt   cmtAnyGetSize( cmtAny *aAny );
IDE_RC cmtAnyWriteAnyToBuffer( cmtAny *aAny, UChar *aBuffer );

#endif
