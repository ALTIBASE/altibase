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

#ifndef _O_CMT_ANY_CLIENT_H_
#define _O_CMT_ANY_CLIENT_H_ 1


typedef struct cmtAny
{
    acp_uint8_t mType;

    union
    {
        acp_sint8_t   mSInt8;
        acp_uint8_t   mUInt8;
        acp_sint16_t  mSInt16;
        acp_uint16_t  mUInt16;
        acp_sint32_t  mSInt32;
        acp_uint32_t  mUInt32;
        acp_sint64_t  mSInt64;
        acp_uint64_t  mUInt64;
        acp_float_t   mFloat32;
        acp_double_t  mFloat64;
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


ACI_RC cmtAnyInitialize(cmtAny *aAny);
ACI_RC cmtAnyFinalize(cmtAny *aAny);

acp_uint8_t cmtAnyGetType(cmtAny *aAny);

acp_bool_t cmtAnyIsNull(cmtAny *aAny);
ACI_RC     cmtAnySetNull(cmtAny *aAny);

ACI_RC cmtAnyReadSChar(cmtAny *aAny, acp_sint8_t *aValue);
ACI_RC cmtAnyReadUChar(cmtAny *aAny, acp_uint8_t *aValue);
ACI_RC cmtAnyReadSShort(cmtAny *aAny, acp_sint16_t *aValue);
ACI_RC cmtAnyReadUShort(cmtAny *aAny, acp_uint16_t *aValue);
ACI_RC cmtAnyReadSInt(cmtAny *aAny, acp_sint32_t *aValue);
ACI_RC cmtAnyReadUInt(cmtAny *aAny, acp_uint32_t *aValue);
ACI_RC cmtAnyReadSLong(cmtAny *aAny, acp_sint64_t *aValue);
ACI_RC cmtAnyReadULong(cmtAny *aAny, acp_uint64_t *aValue);
ACI_RC cmtAnyReadSFloat(cmtAny *aAny, acp_float_t *aValue);
ACI_RC cmtAnyReadSDouble(cmtAny *aAny, acp_double_t *aValue);
ACI_RC cmtAnyReadDateTime(cmtAny *aAny, cmtDateTime **aValue);
ACI_RC cmtAnyReadInterval(cmtAny *aAny, cmtInterval **aValue);
ACI_RC cmtAnyReadNumeric(cmtAny *aAny, cmtNumeric **aValue);
ACI_RC cmtAnyReadVariable(cmtAny *aAny, cmtVariable **aValue);
ACI_RC cmtAnyReadInVariable(cmtAny *aAny, cmtInVariable **aValue);
ACI_RC cmtAnyReadBinary(cmtAny *aAny, cmtVariable **aValue);
ACI_RC cmtAnyReadInBinary(cmtAny *aAny, cmtInVariable **aValue);
ACI_RC cmtAnyReadBit(cmtAny *aAny, cmtBit **aValue);
ACI_RC cmtAnyReadInBit(cmtAny *aAny, cmtInBit **aValue);
ACI_RC cmtAnyReadNibble(cmtAny *aAny, cmtNibble **aValue);
ACI_RC cmtAnyReadInNibble(cmtAny *aAny, cmtInNibble **aValue);
ACI_RC cmtAnyReadLobLocator(cmtAny *aAny, acp_uint64_t *aLocator, acp_uint32_t *aSize);

ACI_RC cmtAnyWriteSChar(cmtAny *aAny, acp_sint8_t aValue);
ACI_RC cmtAnyWriteUChar(cmtAny *aAny, acp_uint8_t aValue);
ACI_RC cmtAnyWriteSShort(cmtAny *aAny, acp_sint16_t aValue);
ACI_RC cmtAnyWriteUShort(cmtAny *aAny, acp_uint16_t aValue);
ACI_RC cmtAnyWriteSInt(cmtAny *aAny, acp_sint32_t aValue);
ACI_RC cmtAnyWriteUInt(cmtAny *aAny, acp_uint32_t aValue);
ACI_RC cmtAnyWriteSLong(cmtAny *aAny, acp_sint64_t aValue);
ACI_RC cmtAnyWriteULong(cmtAny *aAny, acp_uint64_t aValue);
ACI_RC cmtAnyWriteSFloat(cmtAny *aAny, acp_float_t aValue);
ACI_RC cmtAnyWriteSDouble(cmtAny *aAny, acp_double_t aValue);
ACI_RC cmtAnyWriteDateTime(cmtAny *aAny, cmtDateTime *aValue);
ACI_RC cmtAnyWriteInterval(cmtAny *aAny, cmtInterval *aValue);
ACI_RC cmtAnyWriteNumeric(cmtAny *aAny, cmtNumeric *aValue);
ACI_RC cmtAnyWriteVariable(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize);
ACI_RC cmtAnyWriteInVariable(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize);
ACI_RC cmtAnyWriteBinary(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize);
ACI_RC cmtAnyWriteInBinary(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize);
ACI_RC cmtAnyWriteBit(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision);
ACI_RC cmtAnyWriteInBit(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision);
ACI_RC cmtAnyWriteNibble(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision);
ACI_RC cmtAnyWriteInNibble(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision);
ACI_RC cmtAnyWriteLobLocator(cmtAny *aAny, acp_uint64_t aLocator, acp_uint32_t aSize);

ACI_RC cmtAnyGetDateTimeForWrite(cmtAny *aAny, cmtDateTime **aValue);
ACI_RC cmtAnyGetIntervalForWrite(cmtAny *aAny, cmtInterval **aValue);
ACI_RC cmtAnyGetNumericForWrite(cmtAny *aAny, cmtNumeric **aValue, acp_uint8_t aSize);

/*
 * To fix BUG-20195
 */
acp_uint32_t cmtAnyGetSize( cmtAny *aAny );
ACI_RC       cmtAnyWriteAnyToBuffer( cmtAny *aAny, acp_uint8_t *aBuffer );

#endif
