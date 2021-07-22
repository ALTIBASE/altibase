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

#include <cmAllClient.h>


ACI_RC cmtAnyInitialize(cmtAny *aAny)
{
    aAny->mType = CMT_ID_NONE;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyInitializeFromBlock(cmtAny *aAny, acp_uint8_t aType)
{
    switch (aType)
    {
        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            ACI_TEST(cmtVariableInitialize(&aAny->mValue.mVariable) != ACI_SUCCESS);
            break;

        case CMT_ID_BIT:
            ACI_TEST(cmtVariableInitialize(&aAny->mValue.mBit.mData) != ACI_SUCCESS);
            aAny->mValue.mBit.mPrecision = ACP_UINT32_MAX;
            break;

        case CMT_ID_IN_BIT:
            aAny->mValue.mInBit.mPrecision = ACP_UINT32_MAX;
            break;

        case CMT_ID_NIBBLE:
            ACI_TEST(cmtVariableInitialize(&aAny->mValue.mNibble.mData) != ACI_SUCCESS);
            aAny->mValue.mNibble.mPrecision = ACP_UINT32_MAX;
            break;

        case CMT_ID_IN_NIBBLE:
            aAny->mValue.mInNibble.mPrecision = ACP_UINT32_MAX;
            break;

        default:
            break;
    }

    aAny->mType = aType;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmtAnyFinalize(cmtAny *aAny)
{
    switch (aAny->mType)
    {
        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            ACI_TEST(cmtVariableFinalize(&aAny->mValue.mVariable) != ACI_SUCCESS);
            break;

        case CMT_ID_BIT:
            ACI_TEST(cmtVariableFinalize(&aAny->mValue.mBit.mData) != ACI_SUCCESS);
            break;

        case CMT_ID_NIBBLE:
            ACI_TEST(cmtVariableFinalize(&aAny->mValue.mNibble.mData) != ACI_SUCCESS);
            break;

        default:
            break;
    }

    aAny->mType = CMT_ID_NONE;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

acp_uint8_t cmtAnyGetType(cmtAny *aAny)
{
    return aAny->mType;
}

acp_bool_t cmtAnyIsNull(cmtAny *aAny)
{
    return (aAny->mType == CMT_ID_NULL) ? ACP_TRUE : ACP_FALSE;
}

ACI_RC cmtAnySetNull(cmtAny *aAny)
{
    aAny->mType = CMT_ID_NULL;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyReadSChar(cmtAny *aAny, acp_sint8_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_sint8_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_sint8_t)aAny->mValue.mUInt8;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadUChar(cmtAny *aAny, acp_uint8_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_uint8_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_uint8_t)aAny->mValue.mUInt8;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadSShort(cmtAny *aAny, acp_sint16_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_sint16_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_sint16_t)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (acp_sint16_t)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (acp_sint16_t)aAny->mValue.mUInt16;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadUShort(cmtAny *aAny, acp_uint16_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_uint16_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_uint16_t)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (acp_uint16_t)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (acp_uint16_t)aAny->mValue.mUInt16;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadSInt(cmtAny *aAny, acp_sint32_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_sint32_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_sint32_t)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (acp_sint32_t)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (acp_sint32_t)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (acp_sint32_t)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (acp_sint32_t)aAny->mValue.mUInt32;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadUInt(cmtAny *aAny, acp_uint32_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_uint32_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_uint32_t)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (acp_uint32_t)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (acp_uint32_t)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (acp_uint32_t)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (acp_uint32_t)aAny->mValue.mUInt32;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadSLong(cmtAny *aAny, acp_sint64_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_sint64_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_sint64_t)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (acp_sint64_t)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (acp_sint64_t)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (acp_sint64_t)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (acp_sint64_t)aAny->mValue.mUInt32;
            break;
        case CMT_ID_SINT64:
            *aValue = (acp_sint64_t)aAny->mValue.mSInt64;
            break;
        case CMT_ID_UINT64:
            *aValue = (acp_sint64_t)aAny->mValue.mUInt64;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadULong(cmtAny *aAny, acp_uint64_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_SINT8:
            *aValue = (acp_uint64_t)aAny->mValue.mSInt8;
            break;
        case CMT_ID_UINT8:
            *aValue = (acp_uint64_t)aAny->mValue.mUInt8;
            break;
        case CMT_ID_SINT16:
            *aValue = (acp_uint64_t)aAny->mValue.mSInt16;
            break;
        case CMT_ID_UINT16:
            *aValue = (acp_uint64_t)aAny->mValue.mUInt16;
            break;
        case CMT_ID_SINT32:
            *aValue = (acp_uint64_t)aAny->mValue.mSInt32;
            break;
        case CMT_ID_UINT32:
            *aValue = (acp_uint64_t)aAny->mValue.mUInt32;
            break;
        case CMT_ID_SINT64:
            *aValue = (acp_uint64_t)aAny->mValue.mSInt64;
            break;
        case CMT_ID_UINT64:
            *aValue = (acp_uint64_t)aAny->mValue.mUInt64;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* BUG-18945 */
ACI_RC cmtAnyReadLobLocator(cmtAny *aAny, acp_uint64_t *aLocator, acp_uint32_t *aSize)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_LOBLOCATOR, InvalidDataType);

    *aLocator = (acp_uint64_t)aAny->mValue.mLobLocator.mLocator;
    *aSize = (acp_uint32_t)aAny->mValue.mLobLocator.mSize;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadSFloat(cmtAny *aAny, acp_float_t *aValue)
{
    switch (aAny->mType)
    {
        case CMT_ID_FLOAT32:
            *aValue = (acp_float_t)aAny->mValue.mFloat32;
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadSDouble(cmtAny *aAny, acp_double_t *aValue)
{
    // BUG-34100 Bus Error
    switch (aAny->mType)
    {
        case CMT_ID_FLOAT32:
	    acpMemCpy((void*)aValue, &aAny->mValue.mFloat32, ACI_SIZEOF(acp_float_t));
            break;
        case CMT_ID_FLOAT64:
            acpMemCpy((void*)aValue, &aAny->mValue.mFloat64, ACI_SIZEOF(acp_double_t));
            break;
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadDateTime(cmtAny *aAny, cmtDateTime **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_DATETIME, InvalidDataType);

    *aValue = &aAny->mValue.mDateTime;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadInterval(cmtAny *aAny, cmtInterval **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_INTERVAL, InvalidDataType);

    *aValue = &aAny->mValue.mInterval;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadNumeric(cmtAny *aAny, cmtNumeric **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_NUMERIC, InvalidDataType);

    *aValue = &aAny->mValue.mNumeric;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadVariable(cmtAny *aAny, cmtVariable **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_VARIABLE, InvalidDataType);

    *aValue = &aAny->mValue.mVariable;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadInVariable(cmtAny *aAny, cmtInVariable **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_IN_VARIABLE, InvalidDataType);

    *aValue = &aAny->mValue.mInVariable;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadBinary(cmtAny *aAny, cmtVariable **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_BINARY, InvalidDataType);

    *aValue = &aAny->mValue.mVariable;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadInBinary(cmtAny *aAny, cmtInVariable **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_IN_BINARY, InvalidDataType);

    *aValue = &aAny->mValue.mInVariable;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadBit(cmtAny *aAny, cmtBit **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_BIT, InvalidDataType);

    *aValue = &aAny->mValue.mBit;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadInBit(cmtAny *aAny, cmtInBit **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_IN_BIT, InvalidDataType);

    *aValue = &aAny->mValue.mInBit;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadNibble(cmtAny *aAny, cmtNibble **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_NIBBLE, InvalidDataType);

    *aValue = &aAny->mValue.mNibble;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyReadInNibble(cmtAny *aAny, cmtInNibble **aValue)
{
    ACI_TEST_RAISE(aAny->mType != CMT_ID_IN_NIBBLE, InvalidDataType);

    *aValue = &aAny->mValue.mInNibble;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyWriteSChar(cmtAny *aAny, acp_sint8_t aValue)
{
    aAny->mType         = CMT_ID_SINT8;
    aAny->mValue.mSInt8 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteUChar(cmtAny *aAny, acp_uint8_t aValue)
{
    aAny->mType         = CMT_ID_UINT8;
    aAny->mValue.mUInt8 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteSShort(cmtAny *aAny, acp_sint16_t aValue)
{
    aAny->mType          = CMT_ID_SINT16;
    aAny->mValue.mSInt16 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteUShort(cmtAny *aAny, acp_uint16_t aValue)
{
    aAny->mType          = CMT_ID_UINT16;
    aAny->mValue.mUInt16 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteSInt(cmtAny *aAny, acp_sint32_t aValue)
{
    aAny->mType          = CMT_ID_SINT32;
    aAny->mValue.mSInt32 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteUInt(cmtAny *aAny, acp_uint32_t aValue)
{
    aAny->mType          = CMT_ID_UINT32;
    aAny->mValue.mUInt32 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteSLong(cmtAny *aAny, acp_sint64_t aValue)
{
    aAny->mType          = CMT_ID_SINT64;
    aAny->mValue.mSInt64 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteULong(cmtAny *aAny, acp_uint64_t aValue)
{
    aAny->mType          = CMT_ID_UINT64;
    aAny->mValue.mUInt64 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteSFloat(cmtAny *aAny, acp_float_t aValue)
{
    aAny->mType           = CMT_ID_FLOAT32;
    aAny->mValue.mFloat32 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteSDouble(cmtAny *aAny, acp_double_t aValue)
{
    aAny->mType           = CMT_ID_FLOAT64;
    aAny->mValue.mFloat64 = aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteDateTime(cmtAny *aAny, cmtDateTime *aValue)
{
    aAny->mType            = CMT_ID_DATETIME;
    aAny->mValue.mDateTime = *aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteInterval(cmtAny *aAny, cmtInterval *aValue)
{
    aAny->mType            = CMT_ID_INTERVAL;
    aAny->mValue.mInterval = *aValue;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteNumeric(cmtAny *aAny, cmtNumeric *aValue)
{
    ACI_TEST_RAISE(aValue->mSize > CMT_NUMERIC_DATA_SIZE, NumericOverflow);

    aAny->mType           = CMT_ID_NUMERIC;
    aAny->mValue.mNumeric = *aValue;

    return ACI_SUCCESS;

    ACI_EXCEPTION(NumericOverflow);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmtAnyWriteVariable(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize)
{
    aAny->mType = CMT_ID_VARIABLE;

    ACI_TEST(cmtVariableInitialize(&aAny->mValue.mVariable) != ACI_SUCCESS);

    ACI_TEST(cmtVariableSetData(&aAny->mValue.mVariable, aBuffer, aBufferSize) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmtAnyWriteBinary(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize)
{
    aAny->mType = CMT_ID_BINARY;

    ACI_TEST(cmtVariableInitialize(&aAny->mValue.mVariable) != ACI_SUCCESS);

    ACI_TEST(cmtVariableSetData(&aAny->mValue.mVariable, aBuffer, aBufferSize) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmtAnyWriteInVariable(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize)
{
    aAny->mType = CMT_ID_IN_VARIABLE;

    aAny->mValue.mInVariable.mSize = aBufferSize;
    aAny->mValue.mInVariable.mData = aBuffer;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteInBinary(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aBufferSize)
{
    aAny->mType = CMT_ID_IN_BINARY;

    aAny->mValue.mInVariable.mSize = aBufferSize;
    aAny->mValue.mInVariable.mData = aBuffer;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteBit(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision)
{
    aAny->mType = CMT_ID_BIT;

    ACI_TEST(cmtVariableInitialize(&aAny->mValue.mBit.mData) != ACI_SUCCESS);

    ACI_TEST(cmtVariableSetData(&aAny->mValue.mBit.mData,
                                aBuffer,
                                (aPrecision + 7) / 8) != ACI_SUCCESS);

    aAny->mValue.mBit.mPrecision = aPrecision;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmtAnyWriteInBit(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision)
{
    cmtInVariable *sInVariable;

    aAny->mType = CMT_ID_IN_BIT;

    sInVariable = &aAny->mValue.mInBit.mData;
    sInVariable->mSize = (aPrecision + 7) / 8;
    sInVariable->mData = aBuffer;

    aAny->mValue.mInBit.mPrecision = aPrecision;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyWriteNibble(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision)
{
    aAny->mType = CMT_ID_NIBBLE;

    ACI_TEST(cmtVariableInitialize(&aAny->mValue.mNibble.mData) != ACI_SUCCESS);

    ACI_TEST(cmtVariableSetData(&aAny->mValue.mNibble.mData,
                                aBuffer,
                                (aPrecision + 1) / 2) != ACI_SUCCESS);

    aAny->mValue.mNibble.mPrecision = aPrecision;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmtAnyWriteInNibble(cmtAny *aAny, acp_uint8_t *aBuffer, acp_uint32_t aPrecision)
{
    cmtInVariable *sInVariable;

    aAny->mType = CMT_ID_IN_NIBBLE;

    sInVariable = &aAny->mValue.mInNibble.mData;
    sInVariable->mSize = (aPrecision + 1) / 2;
    sInVariable->mData = aBuffer;

    aAny->mValue.mInNibble.mPrecision = aPrecision;

    return ACI_SUCCESS;
}

/* BUG-18945 */
ACI_RC cmtAnyWriteLobLocator(cmtAny *aAny, acp_uint64_t aLocator, acp_uint32_t aSize)
{
    aAny->mType = CMT_ID_LOBLOCATOR;

    aAny->mValue.mLobLocator.mLocator = aLocator;
    aAny->mValue.mLobLocator.mSize    = aSize;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyGetDateTimeForWrite(cmtAny *aAny, cmtDateTime **aValue)
{
    aAny->mType = CMT_ID_DATETIME;

    *aValue     = &aAny->mValue.mDateTime;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyGetIntervalForWrite(cmtAny *aAny, cmtInterval **aValue)
{
    aAny->mType = CMT_ID_INTERVAL;

    *aValue     = &aAny->mValue.mInterval;

    return ACI_SUCCESS;
}

ACI_RC cmtAnyGetNumericForWrite(cmtAny *aAny, cmtNumeric **aValue, acp_uint8_t aSize)
{
    ACI_TEST_RAISE(aSize > CMT_NUMERIC_DATA_SIZE, NumericOverflow);

    aAny->mType                 = CMT_ID_NUMERIC;
    aAny->mValue.mNumeric.mSize = aSize;

    *aValue                     = &aAny->mValue.mNumeric;

    return ACI_SUCCESS;

    ACI_EXCEPTION(NumericOverflow);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/***************************************************************
 * 해당 함수는 cmtAny가 연속된 메모리 공간에 저장되었을때의
 * 크기를 구하는 함수이다.
 * 가변길이 타입(CMT_ID_VARIABLE/CMT_ID_BINARY/CMT_ID_BIT/
 * CMT_ID_NIBBLE)은 CMT_ID_IN_*으로 변경되었을때의
 * 크기를 리턴한다.
 ***************************************************************/
acp_uint32_t cmtAnyGetSize( cmtAny *aAny )
{
    acp_uint32_t sSize;

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
        default:
            ACE_DASSERT(0);
            break;
    }

    return sSize;
}

/***************************************************************
 * 해당 함수는 cmtAny를 연속된 메모리 공간에 저장하는 함수이다.
 * 가변길이 타입(CMT_ID_VARIABLE/CMT_ID_BINARY/CMT_ID_BIT/
 * CMT_ID_NIBBLE)은 CMT_ID_IN_*으로 변환시켜 저장한다.
 ***************************************************************/
ACI_RC cmtAnyWriteAnyToBuffer( cmtAny *aAny,
                               acp_uint8_t  *aBuffer )
{
    acp_uint32_t   sCursor;
    acp_uint8_t *sData;
    acp_uint8_t  sInVariableDelimeter = 0;
    acp_uint8_t  sType;

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
            acpMemCpy( sData + sCursor,
                       aAny->mValue.mNumeric.mData,
                       aAny->mValue.mNumeric.mSize );
            sCursor += aAny->mValue.mNumeric.mSize;
            break;
        case CMT_ID_VARIABLE:
        case CMT_ID_BINARY:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mVariable.mTotalSize );
            ACI_TEST( cmtVariableCopy( &aAny->mValue.mVariable,
                                       sData + sCursor,
                                       0,
                                       aAny->mValue.mVariable.mTotalSize )
                      != ACI_SUCCESS );
            sCursor += aAny->mValue.mVariable.mTotalSize;
            /* Make in-variable data null terminated string */
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, sInVariableDelimeter );
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInVariable.mSize );
            acpMemCpy( sData + sCursor,
                       aAny->mValue.mInVariable.mData,
                       aAny->mValue.mInVariable.mSize );
            sCursor += aAny->mValue.mInVariable.mSize;
            /* Make in-variable data null terminated string */
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, sInVariableDelimeter );
            break;
        case CMT_ID_BIT:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mBit.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mBit.mData.mTotalSize );
            ACI_TEST( cmtVariableCopy( &aAny->mValue.mBit.mData,
                                       sData + sCursor,
                                       0,
                                       aAny->mValue.mBit.mData.mTotalSize )
                      != ACI_SUCCESS );
            sCursor += aAny->mValue.mBit.mData.mTotalSize;
            break;
        case CMT_ID_IN_BIT:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mData.mSize );
            acpMemCpy( sData + sCursor,
                       aAny->mValue.mInBit.mData.mData,
                       aAny->mValue.mInBit.mData.mSize );
            sCursor += aAny->mValue.mInBit.mData.mSize;
            break;
        case CMT_ID_NIBBLE:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mNibble.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mNibble.mData.mTotalSize );
            ACI_TEST( cmtVariableCopy( &aAny->mValue.mNibble.mData,
                                       sData + sCursor,
                                       0,
                                       aAny->mValue.mNibble.mData.mTotalSize )
                      != ACI_SUCCESS );
            sCursor += aAny->mValue.mNibble.mData.mTotalSize;
            break;
        case CMT_ID_IN_NIBBLE:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mData.mSize );
            acpMemCpy( sData + sCursor,
                       aAny->mValue.mInNibble.mData.mData,
                       aAny->mValue.mInNibble.mData.mSize );
            sCursor += aAny->mValue.mInNibble.mData.mSize;
            break;
        case CMT_ID_LOBLOCATOR:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mLobLocator.mLocator );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mLobLocator.mSize );
            break;
        default:
            ACE_DASSERT(0);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
