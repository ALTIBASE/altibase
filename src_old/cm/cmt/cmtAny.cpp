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

#include <cmAll.h>


IDE_RC cmtAnyInitialize(cmtAny *aAny)
{
    aAny->mType = CMT_ID_NONE;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyInitializeFromBlock(cmtAny *aAny, UChar aType)
{
    switch (aType)
    {
        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            IDE_TEST(cmtVariableInitialize(&aAny->mValue.mVariable) != IDE_SUCCESS);
            break;

        case CMT_ID_BIT:
            IDE_TEST(cmtVariableInitialize(&aAny->mValue.mBit.mData) != IDE_SUCCESS);
            aAny->mValue.mBit.mPrecision = ID_UINT_MAX;
            break;

        case CMT_ID_IN_BIT:
            aAny->mValue.mInBit.mPrecision = ID_UINT_MAX;
            break;

        case CMT_ID_NIBBLE:
            IDE_TEST(cmtVariableInitialize(&aAny->mValue.mNibble.mData) != IDE_SUCCESS);
            aAny->mValue.mNibble.mPrecision = ID_UINT_MAX;
            break;

        case CMT_ID_IN_NIBBLE:
            aAny->mValue.mInNibble.mPrecision = ID_UINT_MAX;
            break;

        default:
            break;
    }

    aAny->mType = aType;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmtAnyFinalize(cmtAny *aAny)
{
    switch (aAny->mType)
    {
        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            IDE_TEST(cmtVariableFinalize(&aAny->mValue.mVariable) != IDE_SUCCESS);
            break;

        case CMT_ID_BIT:
            IDE_TEST(cmtVariableFinalize(&aAny->mValue.mBit.mData) != IDE_SUCCESS);
            break;

        case CMT_ID_NIBBLE:
            IDE_TEST(cmtVariableFinalize(&aAny->mValue.mNibble.mData) != IDE_SUCCESS);
            break;

        default:
            break;
    }

    aAny->mType = CMT_ID_NONE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UChar cmtAnyGetType(cmtAny *aAny)
{
    return aAny->mType;
}

idBool cmtAnyIsNull(cmtAny *aAny)
{
    return (aAny->mType == CMT_ID_NULL) ? ID_TRUE : ID_FALSE;
}

IDE_RC cmtAnySetNull(cmtAny *aAny)
{
    aAny->mType = CMT_ID_NULL;

    return IDE_SUCCESS;
}

// PROJ-2256 Communication protocol for efficient query result transmission
IDE_RC cmtAnySetRedundancy( cmtAny *aAny )
{
    aAny->mType = CMT_ID_REDUNDANCY;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyReadSChar(cmtAny *aAny, SChar *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (SChar)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (SChar)aAny->mValue.mUInt8;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadUChar(cmtAny *aAny, UChar *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (UChar)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (UChar)aAny->mValue.mUInt8;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadSShort(cmtAny *aAny, SShort *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (SShort)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (SShort)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (SShort)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (SShort)aAny->mValue.mUInt16;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadUShort(cmtAny *aAny, UShort *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (UShort)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (UShort)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (UShort)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (UShort)aAny->mValue.mUInt16;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadSInt(cmtAny *aAny, SInt *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (SInt)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (SInt)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (SInt)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (SInt)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (SInt)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (SInt)aAny->mValue.mUInt32;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadUInt(cmtAny *aAny, UInt *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (UInt)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (UInt)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (UInt)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (UInt)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (UInt)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (UInt)aAny->mValue.mUInt32;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadSLong(cmtAny *aAny, SLong *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (SLong)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (SLong)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (SLong)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (SLong)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (SLong)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (SLong)aAny->mValue.mUInt32;
            break;
        case CMT_ID_SINT64:
            *aValue = (SLong)aAny->mValue.mSInt64;
            break;
        case CMT_ID_UINT64:
            *aValue = (SLong)aAny->mValue.mUInt64;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadULong(cmtAny *aAny, ULong *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (ULong)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (ULong)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (ULong)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (ULong)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (ULong)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (ULong)aAny->mValue.mUInt32;
            break;
        case CMT_ID_SINT64:
            *aValue = (ULong)aAny->mValue.mSInt64;
            break;
        case CMT_ID_UINT64:
            *aValue = (ULong)aAny->mValue.mUInt64;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-18945 */
IDE_RC cmtAnyReadLobLocator(cmtAny *aAny, ULong *aLocator, UInt *aSize)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_LOBLOCATOR, InvalidDataType);

    *aLocator = (ULong)aAny->mValue.mLobLocator.mLocator;
    *aSize = (UInt)aAny->mValue.mLobLocator.mSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadSFloat(cmtAny *aAny, SFloat *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_FLOAT32:
            *aValue = (SFloat)aAny->mValue.mFloat32;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadSDouble(cmtAny *aAny, SDouble *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_FLOAT32:
            *aValue = (SDouble)aAny->mValue.mFloat32;
            break;
        case CMT_ID_FLOAT64:
            *aValue = (SDouble)aAny->mValue.mFloat64;
            break;
        default:
            IDE_RAISE(InvalidDataType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadDateTime(cmtAny *aAny, cmtDateTime **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_DATETIME, InvalidDataType);

    *aValue = &aAny->mValue.mDateTime;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadInterval(cmtAny *aAny, cmtInterval **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_INTERVAL, InvalidDataType);

    *aValue = &aAny->mValue.mInterval;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadNumeric(cmtAny *aAny, cmtNumeric **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_NUMERIC, InvalidDataType);

    *aValue = &aAny->mValue.mNumeric;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadVariable(cmtAny *aAny, cmtVariable **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_VARIABLE, InvalidDataType);

    *aValue = &aAny->mValue.mVariable;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1920 */
IDE_RC cmtAnyReadPtr(cmtAny *aAny, cmtInVariable **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_PTR, InvalidDataType);

    *aValue = &aAny->mValue.mInVariable;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadInVariable(cmtAny *aAny, cmtInVariable **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_IN_VARIABLE, InvalidDataType);

    *aValue = &aAny->mValue.mInVariable;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadBinary(cmtAny *aAny, cmtVariable **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_BINARY, InvalidDataType);

    *aValue = &aAny->mValue.mVariable;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadInBinary(cmtAny *aAny, cmtInVariable **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_IN_BINARY, InvalidDataType);

    *aValue = &aAny->mValue.mInVariable;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadBit(cmtAny *aAny, cmtBit **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_BIT, InvalidDataType);

    *aValue = &aAny->mValue.mBit;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadInBit(cmtAny *aAny, cmtInBit **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_IN_BIT, InvalidDataType);

    *aValue = &aAny->mValue.mInBit;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadNibble(cmtAny *aAny, cmtNibble **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_NIBBLE, InvalidDataType);

    *aValue = &aAny->mValue.mNibble;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyReadInNibble(cmtAny *aAny, cmtInNibble **aValue)
{
    IDE_TEST_RAISE(aAny->mType != CMT_ID_IN_NIBBLE, InvalidDataType);

    *aValue = &aAny->mValue.mInNibble;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDataType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyWriteSChar(cmtAny *aAny, SChar aValue)
{
    aAny->mType         = CMT_ID_SINT8;
    aAny->mValue.mSInt8 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteUChar(cmtAny *aAny, UChar aValue)
{
    aAny->mType         = CMT_ID_UINT8;
    aAny->mValue.mUInt8 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteSShort(cmtAny *aAny, SShort aValue)
{
    aAny->mType          = CMT_ID_SINT16;
    aAny->mValue.mSInt16 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteUShort(cmtAny *aAny, UShort aValue)
{
    aAny->mType          = CMT_ID_UINT16;
    aAny->mValue.mUInt16 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteSInt(cmtAny *aAny, SInt aValue)
{
    aAny->mType          = CMT_ID_SINT32;
    aAny->mValue.mSInt32 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteUInt(cmtAny *aAny, UInt aValue)
{
    aAny->mType          = CMT_ID_UINT32;
    aAny->mValue.mUInt32 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteSLong(cmtAny *aAny, SLong aValue)
{
    aAny->mType          = CMT_ID_SINT64;
    aAny->mValue.mSInt64 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteULong(cmtAny *aAny, ULong aValue)
{
    aAny->mType          = CMT_ID_UINT64;
    aAny->mValue.mUInt64 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteSFloat(cmtAny *aAny, SFloat aValue)
{
    aAny->mType           = CMT_ID_FLOAT32;
    aAny->mValue.mFloat32 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteSDouble(cmtAny *aAny, SDouble aValue)
{
    aAny->mType           = CMT_ID_FLOAT64;
    aAny->mValue.mFloat64 = aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteDateTime(cmtAny *aAny, cmtDateTime *aValue)
{
    aAny->mType            = CMT_ID_DATETIME;
    aAny->mValue.mDateTime = *aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteInterval(cmtAny *aAny, cmtInterval *aValue)
{
    aAny->mType            = CMT_ID_INTERVAL;
    aAny->mValue.mInterval = *aValue;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteNumeric(cmtAny *aAny, cmtNumeric *aValue)
{
    IDE_TEST_RAISE(aValue->mSize > CMT_NUMERIC_DATA_SIZE, NumericOverflow);

    aAny->mType           = CMT_ID_NUMERIC;
    aAny->mValue.mNumeric = *aValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION(NumericOverflow);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmtAnyWriteVariable(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize)
{
    aAny->mType = CMT_ID_VARIABLE;

    IDE_TEST(cmtVariableInitialize(&aAny->mValue.mVariable) != IDE_SUCCESS);

    IDE_TEST(cmtVariableSetData(&aAny->mValue.mVariable, aBuffer, aBufferSize) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmtAnyWriteBinary(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize)
{
    aAny->mType = CMT_ID_BINARY;

    IDE_TEST(cmtVariableInitialize(&aAny->mValue.mVariable) != IDE_SUCCESS);

    IDE_TEST(cmtVariableSetData(&aAny->mValue.mVariable, aBuffer, aBufferSize) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-1920 */
IDE_RC cmtAnyWritePtr(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize)
{
    aAny->mType = CMT_ID_PTR;

    aAny->mValue.mInVariable.mSize = aBufferSize;
    aAny->mValue.mInVariable.mData = aBuffer;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteInVariable(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize)
{
    aAny->mType = CMT_ID_IN_VARIABLE;

    aAny->mValue.mInVariable.mSize = aBufferSize;
    aAny->mValue.mInVariable.mData = aBuffer;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteInBinary(cmtAny *aAny, UChar *aBuffer, UInt aBufferSize)
{
    aAny->mType = CMT_ID_IN_BINARY;

    aAny->mValue.mInVariable.mSize = aBufferSize;
    aAny->mValue.mInVariable.mData = aBuffer;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteBit(cmtAny *aAny, UChar *aBuffer, UInt aPrecision)
{
    aAny->mType = CMT_ID_BIT;

    IDE_TEST(cmtVariableInitialize(&aAny->mValue.mBit.mData) != IDE_SUCCESS);

    IDE_TEST(cmtVariableSetData(&aAny->mValue.mBit.mData,
                                aBuffer,
                                (aPrecision + 7) / 8) != IDE_SUCCESS);

    aAny->mValue.mBit.mPrecision = aPrecision;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmtAnyWriteInBit(cmtAny *aAny, UChar *aBuffer, UInt aPrecision)
{
    cmtInVariable *sInVariable;
    
    aAny->mType = CMT_ID_IN_BIT;

    sInVariable = &aAny->mValue.mInBit.mData;
    sInVariable->mSize = (aPrecision + 7) / 8;
    sInVariable->mData = aBuffer;

    aAny->mValue.mInBit.mPrecision = aPrecision;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyWriteNibble(cmtAny *aAny, UChar *aBuffer, UInt aPrecision)
{
    aAny->mType = CMT_ID_NIBBLE;

    IDE_TEST(cmtVariableInitialize(&aAny->mValue.mNibble.mData) != IDE_SUCCESS);

    IDE_TEST(cmtVariableSetData(&aAny->mValue.mNibble.mData,
                                aBuffer,
                                (aPrecision + 1) / 2) != IDE_SUCCESS);

    aAny->mValue.mNibble.mPrecision = aPrecision;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmtAnyWriteInNibble(cmtAny *aAny, UChar *aBuffer, UInt aPrecision)
{
    cmtInVariable *sInVariable;

    aAny->mType = CMT_ID_IN_NIBBLE;

    sInVariable = &aAny->mValue.mInNibble.mData;
    sInVariable->mSize = (aPrecision + 1) / 2;
    sInVariable->mData = aBuffer;
    
    aAny->mValue.mInNibble.mPrecision = aPrecision;

    return IDE_SUCCESS;
}

/* BUG-18945 */
IDE_RC cmtAnyWriteLobLocator(cmtAny *aAny, ULong aLocator, UInt aSize)
{
    aAny->mType = CMT_ID_LOBLOCATOR;

    aAny->mValue.mLobLocator.mLocator = aLocator;
    aAny->mValue.mLobLocator.mSize    = aSize;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyGetDateTimeForWrite(cmtAny *aAny, cmtDateTime **aValue)
{
    aAny->mType = CMT_ID_DATETIME;

    *aValue     = &aAny->mValue.mDateTime;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyGetIntervalForWrite(cmtAny *aAny, cmtInterval **aValue)
{
    aAny->mType = CMT_ID_INTERVAL;

    *aValue     = &aAny->mValue.mInterval;

    return IDE_SUCCESS;
}

IDE_RC cmtAnyGetNumericForWrite(cmtAny *aAny, cmtNumeric **aValue, UChar aSize)
{
    IDE_TEST_RAISE(aSize > CMT_NUMERIC_DATA_SIZE, NumericOverflow);

    aAny->mType                 = CMT_ID_NUMERIC;
    aAny->mValue.mNumeric.mSize = aSize;

    *aValue                     = &aAny->mValue.mNumeric;

    return IDE_SUCCESS;

    IDE_EXCEPTION(NumericOverflow);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * 해당 함수는 cmtAny가 연속된 메모리 공간에 저장되었을때의
 * 크기를 구하는 함수이다.
 * 가변길이 타입(CMT_ID_VARIABLE/CMT_ID_BINARY/CMT_ID_BIT/
 * CMT_ID_NIBBLE)은 CMT_ID_IN_*으로 변경되었을때의
 * 크기를 리턴한다.
 ***************************************************************/
UInt cmtAnyGetSize( cmtAny *aAny )
{
    UInt sSize;
    
    /* TYPE(1) */
    sSize = 1;
    
    switch( aAny->mType )
    {
        case CMT_ID_SINT8:
        case CMT_ID_UINT8:
            sSize += 1;
            break;
        case CMT_ID_SINT16:
        case CMT_ID_UINT16:
            sSize += 2;
            break;
        case CMT_ID_SINT32:
        case CMT_ID_UINT32:
        case CMT_ID_FLOAT32:
            sSize += 4;
            break;
        case CMT_ID_SINT64:
        case CMT_ID_UINT64:
        case CMT_ID_FLOAT64:
            sSize += 8;
            break;
        case CMT_ID_DATETIME:
            sSize += 13;
            break;
        case CMT_ID_INTERVAL:
            sSize += 16;
            break;
        case CMT_ID_NUMERIC:
            /* NUMERIC_HEADER_SIZE(5) */
            sSize += 5;
            /* NUMERIC SIZE(x) */
            sSize += aAny->mValue.mNumeric.mSize;
            break;
        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            /* IN HEADER(4) */
            sSize += 4;
            /* IN SIZE(x) */
            sSize += aAny->mValue.mVariable.mTotalSize;
            /* IN delimeter */
            sSize += 1;
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            /* IN HEADER(4) */
            sSize += 4;
            /* IN SIZE(x) */
            sSize += aAny->mValue.mInVariable.mSize;
            /* IN delimeter */
            sSize += 1;
            break;
        case CMT_ID_BIT:
            /* IN_BIT_HEADER(4) */
            sSize += 4;
            /* IN_VARIABLE_HEADER(4) */
            sSize += 4;
            /* BIT SIZE(x) */
            sSize += aAny->mValue.mBit.mData.mTotalSize;
            break;
        case CMT_ID_IN_BIT:
            /* IN_BIT_HEADER(4) */
            sSize += 4;
            /* IN_VARIABLE_HEADER(4) */
            sSize += 4;
            /* BIT SIZE(x) */
            sSize += aAny->mValue.mInBit.mData.mSize;
            break;
        case CMT_ID_NIBBLE:
            /* IN_NIBBLE_HEADER(4) */
            sSize += 4;
            /* IN_VARIABLE_HEADER(4) */
            sSize += 4;
            /* NIBBLE SIZE(x) */
            sSize += aAny->mValue.mNibble.mData.mTotalSize;
            break;
        case CMT_ID_IN_NIBBLE:
            /* IN_NIBBLE_HEADER(4) */
            sSize += 4;
            /* IN_VARIABLE_HEADER(4) */
            sSize += 4;
            /* NIBBLE SIZE(x) */
            sSize += aAny->mValue.mInNibble.mData.mSize;
            break;
        case CMT_ID_LOBLOCATOR:
            sSize += 12;
            break;
        case CMT_ID_PTR:
            /* PROJ-1920 */
            sSize += ID_SIZEOF(UInt) + ID_SIZEOF(vULong);
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return sSize;
}

/***************************************************************
 * 해당 함수는 cmtAny를 연속된 메모리 공간에 저장하는 함수이다.
 * 가변길이 타입(CMT_ID_VARIABLE/CMT_ID_BINARY/CMT_ID_BIT/
 * CMT_ID_NIBBLE)은 CMT_ID_IN_*으로 변환시켜 저장한다.
 ***************************************************************/
IDE_RC cmtAnyWriteAnyToBuffer( cmtAny *aAny,
                               UChar  *aBuffer )
{
    UInt   sCursor;
    UChar *sData;
    UChar  sInVariableDelimeter = 0;
    UChar  sType;

    sCursor = 0;
    sData   = aBuffer;

    switch (aAny->mType)
    {
        case CMT_ID_VARIABLE:
            sType = CMT_ID_IN_VARIABLE;
            break;
        case CMT_ID_BINARY:
            sType = CMT_ID_IN_BINARY;
            break;
        case CMT_ID_BIT:
            sType = CMT_ID_IN_BIT;
            break;
        case CMT_ID_NIBBLE:
            sType = CMT_ID_IN_NIBBLE;
            break;
        default:
            sType = aAny->mType;
            break;
    }
        
    /*
     * Type ID 씀
     */
    CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, sType)

    /*
     * Type 별로 Data 씀
     */
    switch (aAny->mType)
    {
        case CMT_ID_NULL:
            break;
        case CMT_ID_SINT8:
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mSInt8)
            break;
        case CMT_ID_UINT8:
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mUInt8)
            break;
        case CMT_ID_SINT16:
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mSInt16)
            break;
        case CMT_ID_UINT16:
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mUInt16)
            break;
        case CMT_ID_SINT32:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mSInt32)
            break;
        case CMT_ID_UINT32:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mUInt32)
            break;
        case CMT_ID_SINT64:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mSInt64)
            break;
        case CMT_ID_UINT64:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mUInt64)
            break;
        case CMT_ID_FLOAT32:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mFloat32)
            break;
        case CMT_ID_FLOAT64:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mFloat64)
            break;
        case CMT_ID_DATETIME:
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mDateTime.mYear );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mMonth );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mDay );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mHour );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mMinute );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mSecond );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mDateTime.mMicroSecond );
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mDateTime.mTimeZone );
            break;
        case CMT_ID_INTERVAL:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mInterval.mSecond);
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mInterval.mMicroSecond);
            break;
        case CMT_ID_NUMERIC:
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mNumeric.mSize );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mNumeric.mPrecision );
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mNumeric.mScale );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mNumeric.mSign );
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mNumeric.mData,
                           aAny->mValue.mNumeric.mSize );
            sCursor += aAny->mValue.mNumeric.mSize;
            break;
        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mVariable.mTotalSize );
            IDE_TEST( cmtVariableCopy( &aAny->mValue.mVariable,
                                       sData + sCursor,
                                       0,
                                       aAny->mValue.mVariable.mTotalSize )
                      != IDE_SUCCESS );
            sCursor += aAny->mValue.mVariable.mTotalSize;
            /* Make in-variable data null terminated string */
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, sInVariableDelimeter );
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInVariable.mSize );
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mInVariable.mData,
                           aAny->mValue.mInVariable.mSize );
            sCursor += aAny->mValue.mInVariable.mSize;
            /* Make in-variable data null terminated string */
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, sInVariableDelimeter );
            break;
        case CMT_ID_BIT:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mBit.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mBit.mData.mTotalSize );
            IDE_TEST( cmtVariableCopy( &aAny->mValue.mBit.mData,
                                       sData + sCursor,
                                       0,
                                       aAny->mValue.mBit.mData.mTotalSize )
                      != IDE_SUCCESS );
            sCursor += aAny->mValue.mBit.mData.mTotalSize;
            break;
        case CMT_ID_IN_BIT:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mData.mSize );
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mInBit.mData.mData,
                           aAny->mValue.mInBit.mData.mSize );
            sCursor += aAny->mValue.mInBit.mData.mSize;
            break;
        case CMT_ID_NIBBLE:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mNibble.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mNibble.mData.mTotalSize );
            IDE_TEST( cmtVariableCopy( &aAny->mValue.mNibble.mData,
                                       sData + sCursor,
                                       0,
                                       aAny->mValue.mNibble.mData.mTotalSize )
                      != IDE_SUCCESS );
            sCursor += aAny->mValue.mNibble.mData.mTotalSize;
            break;
        case CMT_ID_IN_NIBBLE:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mData.mSize );
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mInNibble.mData.mData,
                           aAny->mValue.mInNibble.mData.mSize );
            sCursor += aAny->mValue.mInNibble.mData.mSize;
            break;
        case CMT_ID_LOBLOCATOR:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mLobLocator.mLocator );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mLobLocator.mSize );
            break;
        case CMT_ID_PTR:
            /* PROJ-1920 */
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInVariable.mSize );
            CMT_COLLECTION_WRITE_PTR( sData, sCursor, aAny->mValue.mInVariable.mData );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
