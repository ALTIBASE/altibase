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


#define NUMERIC_HEADER_SIZE  5
#define VARIABLE_HEADER_SIZE 7

acp_uint32_t        gIPCDASimpleQueryDataBlockSize = (32 * 1024);

ACI_RC cmbBlockMove(cmbBlock *aTargetBlock, cmbBlock *aSourceBlock, acp_uint32_t aOffset)
{
    /*
     * offset 범위 검사
     */
    ACE_ASSERT(aOffset < aSourceBlock->mDataSize);

    /*
     * Target Block Size 세팅
     */
    aTargetBlock->mDataSize = aSourceBlock->mDataSize - aOffset;

    /*
     * Source Block Size 세팅
     */
    aSourceBlock->mDataSize = aOffset;

    /*
     * Source Block에서 Target Block으로 복사
     */
    acpMemCpy(aTargetBlock->mData, aSourceBlock->mData + aOffset, aTargetBlock->mDataSize);

    return ACI_SUCCESS;
}

ACI_RC cmbBlockReadSChar(cmbBlock *aBlock, acp_sint8_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 1);

    CMB_BLOCK_READ_BYTE1(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadUChar(cmbBlock *aBlock, acp_uint8_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 1);

    CMB_BLOCK_READ_BYTE1(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadSShort(cmbBlock *aBlock, acp_sint16_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 2);

    CMB_BLOCK_READ_BYTE2(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadUShort(cmbBlock *aBlock, acp_uint16_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 2);

    CMB_BLOCK_READ_BYTE2(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadSInt(cmbBlock *aBlock, acp_sint32_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 4);

    CMB_BLOCK_READ_BYTE4(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadUInt(cmbBlock *aBlock, acp_uint32_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 4);

    CMB_BLOCK_READ_BYTE4(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadSLong(cmbBlock *aBlock, acp_sint64_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 8);

    CMB_BLOCK_READ_BYTE8(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadULong(cmbBlock *aBlock, acp_uint64_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 8);

    CMB_BLOCK_READ_BYTE8(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* BUG-18945 */
ACI_RC cmbBlockReadLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 12);

    CMB_BLOCK_READ_BYTE8(aBlock, &(aLobLocator->mLocator));

    CMB_BLOCK_READ_BYTE4(aBlock, &(aLobLocator->mSize));

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadSFloat(cmbBlock *aBlock, acp_float_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 4);

    CMB_BLOCK_READ_BYTE4(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadSDouble(cmbBlock *aBlock, acp_double_t *aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 8);

    CMB_BLOCK_READ_BYTE8(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 13);

    /*
     * DateTime 읽음
     */
    CMB_BLOCK_READ_BYTE2(aBlock, &aDateTime->mYear);
    CMB_BLOCK_READ_BYTE1(aBlock, &aDateTime->mMonth);
    CMB_BLOCK_READ_BYTE1(aBlock, &aDateTime->mDay);
    CMB_BLOCK_READ_BYTE1(aBlock, &aDateTime->mHour);
    CMB_BLOCK_READ_BYTE1(aBlock, &aDateTime->mMinute);
    CMB_BLOCK_READ_BYTE1(aBlock, &aDateTime->mSecond);
    CMB_BLOCK_READ_BYTE4(aBlock, &aDateTime->mMicroSecond);
    CMB_BLOCK_READ_BYTE2(aBlock, &aDateTime->mTimeZone);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadInterval(cmbBlock *aBlock, cmtInterval *aInterval, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 16);

    /*
     * Interval 읽음
     */
    CMB_BLOCK_READ_BYTE8(aBlock, &aInterval->mSecond);
    CMB_BLOCK_READ_BYTE8(aBlock, &aInterval->mMicroSecond);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, acp_bool_t *aSuccess)
{
    acp_uint16_t sCursor;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Numeric Array Size 읽음
     */
    ACI_TEST(cmbBlockReadUChar(aBlock, &aNumeric->mSize, aSuccess) != ACI_SUCCESS);

    ACI_TEST_RAISE(*aSuccess == ACP_FALSE, EndOfBlock);

    ACI_TEST_RAISE(aNumeric->mSize > CMT_NUMERIC_DATA_SIZE, SizeOverflow);

    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, aNumeric->mSize + NUMERIC_HEADER_SIZE - 1);

    /*
     * Numeric 속성 읽음
     */
    CMB_BLOCK_READ_BYTE1(aBlock, &aNumeric->mPrecision);
    CMB_BLOCK_READ_BYTE2(aBlock, &aNumeric->mScale);
    CMB_BLOCK_READ_BYTE1(aBlock, &aNumeric->mSign);

    /*
     * Numeric Array 복사
     */
    acpMemCpy(aNumeric->mData, aBlock->mData + aBlock->mCursor, aNumeric->mSize);

    CMB_BLOCK_MOVE_READ_CURSOR(aBlock, aNumeric->mSize);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));
            return ACI_FAILURE;
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION(SizeOverflow);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    ACI_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadVariable(cmbBlock *aBlock, cmtVariable *aVariable, acp_bool_t *aSuccess)
{
    acp_uint32_t sOffset;
    acp_uint16_t sPieceSize;
    acp_uint16_t sCursor;
    acp_uint8_t  sIsEnd;
    acp_bool_t   sHeaderReadFlag = ACP_FALSE;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Variable Header 읽음
     */
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, VARIABLE_HEADER_SIZE);

    CMB_BLOCK_READ_BYTE4(aBlock, &sOffset);
    CMB_BLOCK_READ_BYTE2(aBlock, &sPieceSize);
    CMB_BLOCK_READ_BYTE1(aBlock, &sIsEnd);

    sHeaderReadFlag = ACP_TRUE;

    /*
     * Variable에 Piece 추가
     */
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, sPieceSize);

    ACI_TEST(cmtVariableAddPiece(aVariable,
                                 sOffset,
                                 sPieceSize,
                                 aBlock->mData + aBlock->mCursor) != ACI_SUCCESS);

    CMB_BLOCK_MOVE_READ_CURSOR(aBlock, sPieceSize);

    *aSuccess = sIsEnd ? ACP_TRUE : ACP_FALSE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return ACI_FAILURE;
        }
        else
        {
            if (sHeaderReadFlag == ACP_TRUE)
            {
                ACI_SET(aciSetErrorCode(cmERR_ABORT_VARIABLE_SEQUENCE_SIZE_MISMATCH));
                return ACI_FAILURE;
            }
        }

        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, acp_bool_t *aSuccess)
{
    acp_uint16_t sCursor;
    acp_uint8_t  sInVariableDelimeter;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * IN BLOCK Variable Header 읽음
     */
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 4);

    CMB_BLOCK_READ_BYTE4(aBlock, &aInVariable->mSize);

    /*
     * IN BLOCK Variable data 읽음
     */
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, aInVariable->mSize);

    aInVariable->mData = aBlock->mData + aBlock->mCursor;

    CMB_BLOCK_MOVE_READ_CURSOR(aBlock, aInVariable->mSize);

    /*
     * IN BLOCK Variable delimeter skip
     */
    CMB_BLOCK_READ_BYTE1(aBlock, &sInVariableDelimeter);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return ACI_FAILURE;
        }

        ACE_DASSERT(0);
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadBit(cmbBlock *aBlock, cmtBit *aBit, acp_bool_t *aSuccess)
{
    if (aBit->mPrecision == ACP_UINT32_MAX)
    {
        ACI_TEST(cmbBlockReadUInt(aBlock, &aBit->mPrecision, aSuccess) != ACI_SUCCESS);

        if (*aSuccess == ACP_TRUE)
        {
            ACI_TEST(cmbBlockReadVariable(aBlock, &aBit->mData, aSuccess) != ACI_SUCCESS);
        }
    }
    else
    {
        ACI_TEST(cmbBlockReadVariable(aBlock, &aBit->mData, aSuccess) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbBlockReadInBit(cmbBlock *aBlock, cmtInBit *aInBit, acp_bool_t *aSuccess)
{
    if (aInBit->mPrecision == ACP_UINT32_MAX)
    {
        ACI_TEST(cmbBlockReadUInt(aBlock, &aInBit->mPrecision, aSuccess) != ACI_SUCCESS);

        if (*aSuccess == ACP_TRUE)
        {
            ACI_TEST(cmbBlockReadInVariable(aBlock, &aInBit->mData, aSuccess) != ACI_SUCCESS);
        }
    }
    else
    {
        ACI_TEST(cmbBlockReadInVariable(aBlock, &aInBit->mData, aSuccess) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbBlockReadNibble(cmbBlock *aBlock, cmtNibble *aNibble, acp_bool_t *aSuccess)
{
    if (aNibble->mPrecision == ACP_UINT32_MAX)
    {
        ACI_TEST(cmbBlockReadUInt(aBlock, &aNibble->mPrecision, aSuccess) != ACI_SUCCESS);

        if (*aSuccess == ACP_TRUE)
        {
            ACI_TEST(cmbBlockReadVariable(aBlock, &aNibble->mData, aSuccess) != ACI_SUCCESS);
        }
    }
    else
    {
        ACI_TEST(cmbBlockReadVariable(aBlock, &aNibble->mData, aSuccess) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbBlockReadInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, acp_bool_t *aSuccess)
{
    if (aInNibble->mPrecision == ACP_UINT32_MAX)
    {
        ACI_TEST(cmbBlockReadUInt(aBlock, &aInNibble->mPrecision, aSuccess) != ACI_SUCCESS);

        if (*aSuccess == ACP_TRUE)
        {
            ACI_TEST(cmbBlockReadInVariable(aBlock, &aInNibble->mData, aSuccess) != ACI_SUCCESS);
        }
    }
    else
    {
        ACI_TEST(cmbBlockReadInVariable(aBlock, &aInNibble->mData, aSuccess) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbBlockReadAny(cmbBlock *aBlock, cmtAny *aAny, acp_bool_t *aSuccess)
{
    acp_uint16_t sCursor;
    acp_uint8_t  sType;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 읽음
     */
    ACI_TEST(cmbBlockReadUChar(aBlock, &sType, aSuccess) != ACI_SUCCESS);

    if (*aSuccess == ACP_TRUE)
    {
        /*
         * 초기화
         */
        if (aAny->mType == CMT_ID_NONE)
        {
            ACI_TEST(cmtAnyInitializeFromBlock(aAny, sType) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST_RAISE(aAny->mType != sType, TypeMismatch);
        }

        /*
         * Type 별로 Data 읽음
         */
        switch (aAny->mType)
        {
            case CMT_ID_NULL:
                break;

            case CMT_ID_SINT8:
                ACI_TEST(cmbBlockReadSChar(aBlock, &aAny->mValue.mSInt8, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT8:
                ACI_TEST(cmbBlockReadUChar(aBlock, &aAny->mValue.mUInt8, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_SINT16:
                ACI_TEST(cmbBlockReadSShort(aBlock, &aAny->mValue.mSInt16, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT16:
                ACI_TEST(cmbBlockReadUShort(aBlock, &aAny->mValue.mUInt16, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_SINT32:
                ACI_TEST(cmbBlockReadSInt(aBlock, &aAny->mValue.mSInt32, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT32:
                ACI_TEST(cmbBlockReadUInt(aBlock, &aAny->mValue.mUInt32, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_SINT64:
                ACI_TEST(cmbBlockReadSLong(aBlock, &aAny->mValue.mSInt64, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT64:
                ACI_TEST(cmbBlockReadULong(aBlock, &aAny->mValue.mUInt64, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_FLOAT32:
                ACI_TEST(cmbBlockReadSFloat(aBlock, &aAny->mValue.mFloat32, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_FLOAT64:
                ACI_TEST(cmbBlockReadSDouble(aBlock, &aAny->mValue.mFloat64, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_DATETIME:
                ACI_TEST(cmbBlockReadDateTime(aBlock, &aAny->mValue.mDateTime, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_INTERVAL:
                ACI_TEST(cmbBlockReadInterval(aBlock, &aAny->mValue.mInterval, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_NUMERIC:
                ACI_TEST(cmbBlockReadNumeric(aBlock, &aAny->mValue.mNumeric, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_VARIABLE:
            case CMT_ID_BINARY:
                ACI_TEST(cmbBlockReadVariable(aBlock, &aAny->mValue.mVariable, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_VARIABLE:
            case CMT_ID_IN_BINARY:
                ACI_TEST(cmbBlockReadInVariable(aBlock, &aAny->mValue.mInVariable, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_BIT:
                ACI_TEST(cmbBlockReadBit(aBlock, &aAny->mValue.mBit, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_BIT:
                ACI_TEST(cmbBlockReadInBit(aBlock, &aAny->mValue.mInBit, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_NIBBLE:
                ACI_TEST(cmbBlockReadNibble(aBlock, &aAny->mValue.mNibble, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_NIBBLE:
                ACI_TEST(cmbBlockReadInNibble(aBlock, &aAny->mValue.mInNibble, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_LOBLOCATOR:
                /* BUG-18945 */
                ACI_TEST(cmbBlockReadLobLocator(aBlock, &aAny->mValue.mLobLocator, aSuccess) != ACI_SUCCESS);
                break;

            default:
                ACI_RAISE(InvalidType);
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(TypeMismatch);
    {
         ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION(InvalidType);
    {
        aAny->mType = CMT_ID_NONE;
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return ACI_FAILURE;
}

ACI_RC cmbBlockReadCollection(cmbBlock *aBlock, cmtCollection *aCollection, acp_bool_t *aSuccess)
{
    acp_uint16_t sCursor;
    acp_uint8_t  sType;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 읽음
     */
    ACI_TEST(cmbBlockReadUChar(aBlock, &sType, aSuccess) != ACI_SUCCESS);

    if (*aSuccess == ACP_TRUE)
    {
        /*
         * 초기화
         */
        if (aCollection->mType == CMT_ID_NONE)
        {
            ACI_TEST(cmtCollectionInitializeFromBlock(aCollection, sType) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST_RAISE(aCollection->mType != sType, TypeMismatch);
        }

        /*
         * Type 별로 Data 읽음
         */
        switch (aCollection->mType)
        {
            case CMT_ID_VARIABLE:
                ACI_TEST(cmbBlockReadVariable(aBlock, &aCollection->mValue.mVariable, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_VARIABLE:
                ACI_TEST(cmbBlockReadInVariable(aBlock, &aCollection->mValue.mInVariable, aSuccess) != ACI_SUCCESS);
                break;

            default:
                ACE_ASSERT(0);
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(TypeMismatch);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteSChar(cmbBlock *aBlock, acp_sint8_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 1);

    CMB_BLOCK_WRITE_BYTE1(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteUChar(cmbBlock *aBlock, acp_uint8_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 1);

    CMB_BLOCK_WRITE_BYTE1(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteSShort(cmbBlock *aBlock, acp_sint16_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 2);

    CMB_BLOCK_WRITE_BYTE2(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteUShort(cmbBlock *aBlock, acp_uint16_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 2);

    CMB_BLOCK_WRITE_BYTE2(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteSInt(cmbBlock *aBlock, acp_sint32_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteUInt(cmbBlock *aBlock, acp_uint32_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteSLong(cmbBlock *aBlock, acp_sint64_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 8);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteULong(cmbBlock *aBlock, acp_uint64_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 8);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteSFloat(cmbBlock *aBlock, acp_float_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteSDouble(cmbBlock *aBlock, acp_double_t aValue, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 8);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aValue);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

/* BUG-18945 */
ACI_RC cmbBlockWriteLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 12);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aLobLocator->mLocator);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aLobLocator->mSize);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 13);

    /*
     * DataTime 씀
     */
    CMB_BLOCK_WRITE_BYTE2(aBlock, aDateTime->mYear);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aDateTime->mMonth);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aDateTime->mDay);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aDateTime->mHour);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aDateTime->mMinute);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aDateTime->mSecond);
    CMB_BLOCK_WRITE_BYTE4(aBlock, aDateTime->mMicroSecond);
    CMB_BLOCK_WRITE_BYTE2(aBlock, aDateTime->mTimeZone);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteInterval(cmbBlock *aBlock, cmtInterval *aInterval, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 16);

    /*
     * Interval 씀
     */
    CMB_BLOCK_WRITE_BYTE8(aBlock, aInterval->mSecond);
    CMB_BLOCK_WRITE_BYTE8(aBlock, aInterval->mMicroSecond);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC cmbBlockWriteNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, acp_bool_t *aSuccess)
{
    ACI_TEST_RAISE(aNumeric->mSize > CMT_NUMERIC_DATA_SIZE, SizeOverflow);

    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, aNumeric->mSize + NUMERIC_HEADER_SIZE);

    /*
     * Numeric 속성 씀
     */
    CMB_BLOCK_WRITE_BYTE1(aBlock, aNumeric->mSize);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aNumeric->mPrecision);
    CMB_BLOCK_WRITE_BYTE2(aBlock, aNumeric->mScale);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aNumeric->mSign);

    /*
     * Numeric Array 씀
     */
    acpMemCpy(aBlock->mData + aBlock->mCursor, aNumeric->mData, aNumeric->mSize);

    CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, aNumeric->mSize);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION(SizeOverflow);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteVariable(cmbBlock *aBlock, cmtVariable *aVariable, acp_uint32_t *aSizeLeft, acp_bool_t *aSuccess)
{
    acp_uint32_t sSize;
    acp_uint32_t sOffset;
    acp_uint16_t sPieceSize;

    /*
     * Variable Size 획득 및 검사
     */
    sSize = cmtVariableGetSize(aVariable);

    ACI_TEST_RAISE((sSize > 0) && (aVariable->mPieceCount == 0), InvalidVariableType);

    /*
     * SizeLeft = 0 인 경우 SizeLeft <- Variable Size로 세팅
     */
    if (*aSizeLeft == 0)
    {
        *aSizeLeft = sSize;
    }

    /*
     * Variable 최소 사이즈 확보여부 검사
     */
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, VARIABLE_HEADER_SIZE + (*aSizeLeft ? 1 : 0));

    /*
     * Variable Write Offset 씀
     */
    sOffset = sSize - *aSizeLeft;

    CMB_BLOCK_WRITE_BYTE4(aBlock, sOffset);

    /*
     * Variable Write Size 계산
     */
    sPieceSize = ACP_MIN(*aSizeLeft,
                         (acp_uint16_t)(aBlock->mBlockSize -
                                        aBlock->mCursor -
                                        (VARIABLE_HEADER_SIZE - 4)));

    /*
     * Variable Write Size 씀
     */
    CMB_BLOCK_WRITE_BYTE2(aBlock, sPieceSize);

    /*
     * 마지막 Piece인지 검사 후 Flag 씀
     */
    *aSizeLeft -= sPieceSize;

    CMB_BLOCK_WRITE_BYTE1(aBlock, (*aSizeLeft ? 0 : 1));

    /*
     * Variable Data 씀
     */
    ACI_TEST(cmtVariableCopy(aVariable,
                             aBlock->mData + aBlock->mCursor,
                             sOffset,
                             sPieceSize) != ACI_SUCCESS);

    CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, sPieceSize);

    /*
     * 마지막 Piece이면 끝
     */
    *aSuccess = *aSizeLeft ? ACP_FALSE : ACP_TRUE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidVariableType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_VARIABLE_TYPE));
    }
    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, acp_bool_t *aSuccess)
{
    acp_uint8_t sInVariableDelimeter = 0;

    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, aInVariable->mSize + 5);

    /*
     * IN BLOCK Variable Write Size 씀
     */
    CMB_BLOCK_WRITE_BYTE4(aBlock, aInVariable->mSize);

    /*
     * IN BLOCK Variable Data 씀
     */
    acpMemCpy(aBlock->mData + aBlock->mCursor, aInVariable->mData, aInVariable->mSize);

    CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, aInVariable->mSize);

    /*
     * IN BLOCK Variable Delimeter 씀
     * Make in-variable data null terminated string
     */
    CMB_BLOCK_WRITE_BYTE1(aBlock, sInVariableDelimeter);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        ACE_DASSERT(0);
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteBit(cmbBlock *aBlock, cmtBit *aBit, acp_uint32_t *aSizeLeft, acp_bool_t *aSuccess)
{
    if (*aSizeLeft == 0)
    {
        /*
         * Data Size가 0인건지 처음보내는 건지 판단할 수 없으므로
         * 처음 보낼 때 Data Size가 0인 경우 쪼개지지 않고
         * 한번에 보내지도록 버퍼 포인터를 검사한다.
         */
        CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, VARIABLE_HEADER_SIZE + 4);

        CMB_BLOCK_WRITE_BYTE4(aBlock, aBit->mPrecision);

        ACI_TEST(cmbBlockWriteVariable(aBlock, &aBit->mData, aSizeLeft, aSuccess) != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(cmbBlockWriteVariable(aBlock, &aBit->mData, aSizeLeft, aSuccess) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteInBit(cmbBlock *aBlock, cmtInBit *aInBit, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4 + 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aInBit->mPrecision);

    ACI_TEST(cmbBlockWriteInVariable(aBlock, &aInBit->mData, aSuccess) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteNibble(cmbBlock *aBlock, cmtNibble *aNibble, acp_uint32_t *aSizeLeft, acp_bool_t *aSuccess)
{
    if (*aSizeLeft == 0)
    {
        /*
         * Data Size가 0인건지 처음보내는 건지 판단할 수 없으므로
         * 처음 보낼 때 Data Size가 0인 경우 쪼개지지 않고
         * 한번에 보내지도록 버퍼 포인터를 검사한다.
         */
        CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, VARIABLE_HEADER_SIZE + 4);

        CMB_BLOCK_WRITE_BYTE4(aBlock, aNibble->mPrecision);

        ACI_TEST(cmbBlockWriteVariable(aBlock, &aNibble->mData, aSizeLeft, aSuccess) != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(cmbBlockWriteVariable(aBlock, &aNibble->mData, aSizeLeft, aSuccess) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, acp_bool_t *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4 + 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aInNibble->mPrecision);

    ACI_TEST(cmbBlockWriteInVariable(aBlock, &aInNibble->mData, aSuccess) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(EndOfBlock);
    {
        return ACI_SUCCESS;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteAny(cmbBlock *aBlock, cmtAny *aAny, acp_uint32_t *aSizeLeft, acp_bool_t *aSuccess)
{
    acp_uint16_t sCursor;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 범위 검사
     */
    ACE_ASSERT((aAny->mType > CMT_ID_NONE) && (aAny->mType < CMT_ID_MAX));

    /*
     * Type ID 씀
     */
    ACI_TEST(cmbBlockWriteUChar(aBlock, aAny->mType, aSuccess) != ACI_SUCCESS);

    if (*aSuccess == ACP_TRUE)
    {
        /*
         * Type 별로 Data 씀
         */
        switch (aAny->mType)
        {
            case CMT_ID_NULL:
                break;

            case CMT_ID_SINT8:
                ACI_TEST(cmbBlockWriteSChar(aBlock, aAny->mValue.mSInt8, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT8:
                ACI_TEST(cmbBlockWriteUChar(aBlock, aAny->mValue.mUInt8, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_SINT16:
                ACI_TEST(cmbBlockWriteSShort(aBlock, aAny->mValue.mSInt16, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT16:
                ACI_TEST(cmbBlockWriteUShort(aBlock, aAny->mValue.mUInt16, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_SINT32:
                ACI_TEST(cmbBlockWriteSInt(aBlock, aAny->mValue.mSInt32, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT32:
                ACI_TEST(cmbBlockWriteUInt(aBlock, aAny->mValue.mUInt32, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_SINT64:
                ACI_TEST(cmbBlockWriteSLong(aBlock, aAny->mValue.mSInt64, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_UINT64:
                ACI_TEST(cmbBlockWriteULong(aBlock, aAny->mValue.mUInt64, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_FLOAT32:
                ACI_TEST(cmbBlockWriteSFloat(aBlock, aAny->mValue.mFloat32, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_FLOAT64:
                ACI_TEST(cmbBlockWriteSDouble(aBlock, aAny->mValue.mFloat64, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_DATETIME:
                ACI_TEST(cmbBlockWriteDateTime(aBlock, &aAny->mValue.mDateTime, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_INTERVAL:
                ACI_TEST(cmbBlockWriteInterval(aBlock, &aAny->mValue.mInterval, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_NUMERIC:
                ACI_TEST(cmbBlockWriteNumeric(aBlock, &aAny->mValue.mNumeric, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_VARIABLE:
            case CMT_ID_BINARY:
                ACI_TEST(cmbBlockWriteVariable(aBlock, &aAny->mValue.mVariable, aSizeLeft, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_VARIABLE:
            case CMT_ID_IN_BINARY:
                ACI_TEST(cmbBlockWriteInVariable(aBlock, &aAny->mValue.mInVariable, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_BIT:
                ACI_TEST(cmbBlockWriteBit(aBlock, &aAny->mValue.mBit, aSizeLeft, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_BIT:
                ACI_TEST(cmbBlockWriteInBit(aBlock, &aAny->mValue.mInBit, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_NIBBLE:
                ACI_TEST(cmbBlockWriteNibble(aBlock, &aAny->mValue.mNibble, aSizeLeft, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_NIBBLE:
                ACI_TEST(cmbBlockWriteInNibble(aBlock, &aAny->mValue.mInNibble, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_LOBLOCATOR:
                /* BUG-18945 */
                ACI_TEST(cmbBlockWriteLobLocator(aBlock, &aAny->mValue.mLobLocator, aSuccess) != ACI_SUCCESS);
                break;

            default:
                ACI_RAISE(InvalidType);
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidType);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    ACI_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor   = sCursor;
        aBlock->mDataSize = sCursor;
    }

    return ACI_FAILURE;
}

ACI_RC cmbBlockWriteCollection(cmbBlock *aBlock, cmtCollection *aCollection, acp_uint32_t *aSizeLeft, acp_bool_t *aSuccess)
{
    acp_uint16_t sCursor;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 범위 검사
     */
    ACE_ASSERT((aCollection->mType == CMT_ID_VARIABLE) ||
               (aCollection->mType == CMT_ID_IN_VARIABLE));

    /*
     * Type ID 씀
     */
    ACI_TEST(cmbBlockWriteUChar(aBlock, aCollection->mType, aSuccess) != ACI_SUCCESS);

    if (*aSuccess == ACP_TRUE)
    {
        /*
         * Type 별로 Data 씀
         */
        switch (aCollection->mType)
        {
            case CMT_ID_VARIABLE:
                ACI_TEST(cmbBlockWriteVariable(aBlock, &aCollection->mValue.mVariable, aSizeLeft, aSuccess) != ACI_SUCCESS);
                break;

            case CMT_ID_IN_VARIABLE:
                ACI_TEST(cmbBlockWriteInVariable(aBlock, &aCollection->mValue.mInVariable, aSuccess) != ACI_SUCCESS);
                break;

            default:
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor   = sCursor;
        aBlock->mDataSize = sCursor;
    }

    return ACI_FAILURE;
}

acp_uint32_t cmbBlockGetIPCDASimpleQueryDataBlockSize()
{
#if defined(SMALL_FOOTPRINT)
    return 4 * 1024;
#else
    return gIPCDASimpleQueryDataBlockSize * 1024;
#endif
}

