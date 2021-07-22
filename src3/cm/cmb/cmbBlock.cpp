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


#define NUMERIC_HEADER_SIZE  5
#define VARIABLE_HEADER_SIZE 7

IDE_RC cmbBlockMove(cmbBlock *aTargetBlock, cmbBlock *aSourceBlock, UInt aOffset)
{
    /*
     * offset 범위 검사
     */
    IDE_ASSERT(aOffset < aSourceBlock->mDataSize);

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
    idlOS::memcpy(aTargetBlock->mData, aSourceBlock->mData + aOffset, aTargetBlock->mDataSize);

    return IDE_SUCCESS;
}

IDE_RC cmbBlockReadSChar(cmbBlock *aBlock, SChar *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 1);

    CMB_BLOCK_READ_BYTE1(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadUChar(cmbBlock *aBlock, UChar *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 1);

    CMB_BLOCK_READ_BYTE1(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadSShort(cmbBlock *aBlock, SShort *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 2);

    CMB_BLOCK_READ_BYTE2(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadUShort(cmbBlock *aBlock, UShort *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 2);

    CMB_BLOCK_READ_BYTE2(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadSInt(cmbBlock *aBlock, SInt *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 4);

    CMB_BLOCK_READ_BYTE4(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadUInt(cmbBlock *aBlock, UInt *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 4);

    CMB_BLOCK_READ_BYTE4(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadSLong(cmbBlock *aBlock, SLong *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 8);

    CMB_BLOCK_READ_BYTE8(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadULong(cmbBlock *aBlock, ULong *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 8);

    CMB_BLOCK_READ_BYTE8(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-18945 */
IDE_RC cmbBlockReadLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 12);

    CMB_BLOCK_READ_BYTE8(aBlock, &(aLobLocator->mLocator));

    CMB_BLOCK_READ_BYTE4(aBlock, &(aLobLocator->mSize));

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadSFloat(cmbBlock *aBlock, SFloat *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 4);

    CMB_BLOCK_READ_BYTE4(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadSDouble(cmbBlock *aBlock, SDouble *aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 8);

    CMB_BLOCK_READ_BYTE8(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, idBool *aSuccess)
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

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadInterval(cmbBlock *aBlock, cmtInterval *aInterval, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, 16);

    /*
     * Interval 읽음
     */
    CMB_BLOCK_READ_BYTE8(aBlock, &aInterval->mSecond);
    CMB_BLOCK_READ_BYTE8(aBlock, &aInterval->mMicroSecond);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, idBool *aSuccess)
{
    UShort sCursor;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Numeric Array Size 읽음
     */
    IDE_TEST(cmbBlockReadUChar(aBlock, &aNumeric->mSize, aSuccess) != IDE_SUCCESS);

    IDE_TEST_RAISE(*aSuccess == ID_FALSE, EndOfBlock);

    IDE_TEST_RAISE(aNumeric->mSize > CMT_NUMERIC_DATA_SIZE, SizeOverflow);

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
    idlOS::memcpy(aNumeric->mData, aBlock->mData + aBlock->mCursor, aNumeric->mSize);

    CMB_BLOCK_MOVE_READ_CURSOR(aBlock, aNumeric->mSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(SizeOverflow);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadVariable(cmbBlock *aBlock, cmtVariable *aVariable, idBool *aSuccess)
{
    UInt   sOffset;
    UShort sPieceSize;
    UShort sCursor;
    UChar  sIsEnd;
    idBool sHeaderReadFlag = ID_FALSE;

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

    sHeaderReadFlag = ID_TRUE;

    /*
     * Variable에 Piece 추가
     */
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, sPieceSize);

    IDE_TEST(cmtVariableAddPiece(aVariable,
                                 sOffset,
                                 sPieceSize,
                                 aBlock->mData + aBlock->mCursor) != IDE_SUCCESS);

    CMB_BLOCK_MOVE_READ_CURSOR(aBlock, sPieceSize);

    *aSuccess = sIsEnd ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }
        else
        {
            if (sHeaderReadFlag == ID_TRUE)
            {
                IDE_SET(ideSetErrorCode(cmERR_ABORT_VARIABLE_SEQUENCE_SIZE_MISMATCH));

                return IDE_FAILURE;
            }
        }

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return IDE_FAILURE;
}

/* PROJ-1920 */
IDE_RC cmbBlockReadPtr(cmbBlock *aBlock, cmtInVariable*aInVariable, idBool *aSuccess)
{
    UShort sCursor;
    UInt   sPtrSize;

    sPtrSize = ID_SIZEOF(vULong);

    /*   
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;
    
    CMB_BLOCK_CHECK_READ_CURSOR(aBlock, sPtrSize + 4);

    /*   
     * IN BLOCK Ptr Header 읽음
     */
    CMB_BLOCK_READ_BYTE4(aBlock, &aInVariable->mSize);

    /*   
     * IN BLOCK Ptr 읽음
     */
    CMB_BLOCK_READ_PTR(aBlock, aInVariable->mData, sPtrSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        IDE_DASSERT(0);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, idBool *aSuccess)
{
    UShort sCursor;
    UChar  sInVariableDelimeter;

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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        if (aBlock->mCursor != aBlock->mDataSize)
        {
            IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

            return IDE_FAILURE;
        }

        IDE_DASSERT(0);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadBit(cmbBlock *aBlock, cmtBit *aBit, idBool *aSuccess)
{
    if (aBit->mPrecision == ID_UINT_MAX)
    {
        IDE_TEST(cmbBlockReadUInt(aBlock, &aBit->mPrecision, aSuccess) != IDE_SUCCESS);

        if (*aSuccess == ID_TRUE)
        {
            IDE_TEST(cmbBlockReadVariable(aBlock, &aBit->mData, aSuccess) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST(cmbBlockReadVariable(aBlock, &aBit->mData, aSuccess) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbBlockReadInBit(cmbBlock *aBlock, cmtInBit *aInBit, idBool *aSuccess)
{
    if (aInBit->mPrecision == ID_UINT_MAX)
    {
        IDE_TEST(cmbBlockReadUInt(aBlock, &aInBit->mPrecision, aSuccess) != IDE_SUCCESS);

        if (*aSuccess == ID_TRUE)
        {
            IDE_TEST(cmbBlockReadInVariable(aBlock, &aInBit->mData, aSuccess) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST(cmbBlockReadInVariable(aBlock, &aInBit->mData, aSuccess) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbBlockReadNibble(cmbBlock *aBlock, cmtNibble *aNibble, idBool *aSuccess)
{
    if (aNibble->mPrecision == ID_UINT_MAX)
    {
        IDE_TEST(cmbBlockReadUInt(aBlock, &aNibble->mPrecision, aSuccess) != IDE_SUCCESS);

        if (*aSuccess == ID_TRUE)
        {
            IDE_TEST(cmbBlockReadVariable(aBlock, &aNibble->mData, aSuccess) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST(cmbBlockReadVariable(aBlock, &aNibble->mData, aSuccess) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbBlockReadInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, idBool *aSuccess)
{
    if (aInNibble->mPrecision == ID_UINT_MAX)
    {
        IDE_TEST(cmbBlockReadUInt(aBlock, &aInNibble->mPrecision, aSuccess) != IDE_SUCCESS);

        if (*aSuccess == ID_TRUE)
        {
            IDE_TEST(cmbBlockReadInVariable(aBlock, &aInNibble->mData, aSuccess) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST(cmbBlockReadInVariable(aBlock, &aInNibble->mData, aSuccess) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbBlockReadAny(cmbBlock *aBlock, cmtAny *aAny, idBool *aSuccess)
{
    UShort sCursor;
    UChar  sType;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 읽음
     */
    IDE_TEST(cmbBlockReadUChar(aBlock, &sType, aSuccess) != IDE_SUCCESS);

    if (*aSuccess == ID_TRUE)
    {
        /*
         * 초기화
         */
        if (aAny->mType == CMT_ID_NONE)
        {
            IDE_TEST(cmtAnyInitializeFromBlock(aAny, sType) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST_RAISE(aAny->mType != sType, TypeMismatch);
        }

        /*
         * Type 별로 Data 읽음
         */
        switch (aAny->mType)
        {
            case CMT_ID_NULL:
                break;

            case CMT_ID_SINT8:
                IDE_TEST(cmbBlockReadSChar(aBlock, &aAny->mValue.mSInt8, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT8:
                IDE_TEST(cmbBlockReadUChar(aBlock, &aAny->mValue.mUInt8, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_SINT16:
                IDE_TEST(cmbBlockReadSShort(aBlock, &aAny->mValue.mSInt16, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT16:
                IDE_TEST(cmbBlockReadUShort(aBlock, &aAny->mValue.mUInt16, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_SINT32:
                IDE_TEST(cmbBlockReadSInt(aBlock, &aAny->mValue.mSInt32, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT32:
                IDE_TEST(cmbBlockReadUInt(aBlock, &aAny->mValue.mUInt32, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_SINT64:
                IDE_TEST(cmbBlockReadSLong(aBlock, &aAny->mValue.mSInt64, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT64:
                IDE_TEST(cmbBlockReadULong(aBlock, &aAny->mValue.mUInt64, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_FLOAT32:
                IDE_TEST(cmbBlockReadSFloat(aBlock, &aAny->mValue.mFloat32, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_FLOAT64:
                IDE_TEST(cmbBlockReadSDouble(aBlock, &aAny->mValue.mFloat64, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_DATETIME:
                IDE_TEST(cmbBlockReadDateTime(aBlock, &aAny->mValue.mDateTime, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_INTERVAL:
                IDE_TEST(cmbBlockReadInterval(aBlock, &aAny->mValue.mInterval, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_NUMERIC:
                IDE_TEST(cmbBlockReadNumeric(aBlock, &aAny->mValue.mNumeric, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_VARIABLE:
            case CMT_ID_BINARY:
                IDE_TEST(cmbBlockReadVariable(aBlock, &aAny->mValue.mVariable, aSuccess) != IDE_SUCCESS);
                break;
                
            case CMT_ID_IN_VARIABLE:
            case CMT_ID_IN_BINARY:
                IDE_TEST(cmbBlockReadInVariable(aBlock, &aAny->mValue.mInVariable, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_BIT:
                IDE_TEST(cmbBlockReadBit(aBlock, &aAny->mValue.mBit, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_IN_BIT:
                IDE_TEST(cmbBlockReadInBit(aBlock, &aAny->mValue.mInBit, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_NIBBLE:
                IDE_TEST(cmbBlockReadNibble(aBlock, &aAny->mValue.mNibble, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_IN_NIBBLE:
                IDE_TEST(cmbBlockReadInNibble(aBlock, &aAny->mValue.mInNibble, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_LOBLOCATOR:
                /* BUG-18945 */
                IDE_TEST(cmbBlockReadLobLocator(aBlock, &aAny->mValue.mLobLocator, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_PTR:
                /* PROJ-1920 */
                IDE_TEST(cmbBlockReadPtr(aBlock, &aAny->mValue.mInVariable, aSuccess) != IDE_SUCCESS);
                break;

            default:
                IDE_RAISE(InvalidType);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(TypeMismatch);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION(InvalidType);
    {
        aAny->mType = CMT_ID_NONE;

        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return IDE_FAILURE;
}

IDE_RC cmbBlockReadCollection(cmbBlock *aBlock, cmtCollection *aCollection, idBool *aSuccess)
{
    UShort sCursor;
    UChar  sType;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 읽음
     */
    IDE_TEST(cmbBlockReadUChar(aBlock, &sType, aSuccess) != IDE_SUCCESS);

    if (*aSuccess == ID_TRUE)
    {
        /*
         * 초기화
         */
        if (aCollection->mType == CMT_ID_NONE)
        {
            IDE_TEST(cmtCollectionInitializeFromBlock(aCollection, sType) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST_RAISE(aCollection->mType != sType, TypeMismatch);
        }
        
        /*
         * Type 별로 Data 읽음
         */
        switch (aCollection->mType)
        {
            case CMT_ID_VARIABLE:
                IDE_TEST(cmbBlockReadVariable(aBlock, &aCollection->mValue.mVariable, aSuccess) != IDE_SUCCESS);
                break;
                
            case CMT_ID_IN_VARIABLE:
                IDE_TEST(cmbBlockReadInVariable(aBlock, &aCollection->mValue.mInVariable, aSuccess) != IDE_SUCCESS);
                break;

            default:
                IDE_ASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(TypeMismatch);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor = sCursor;
    }

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteSChar(cmbBlock *aBlock, SChar aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 1);

    CMB_BLOCK_WRITE_BYTE1(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteUChar(cmbBlock *aBlock, UChar aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 1);

    CMB_BLOCK_WRITE_BYTE1(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteSShort(cmbBlock *aBlock, SShort aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 2);

    CMB_BLOCK_WRITE_BYTE2(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteUShort(cmbBlock *aBlock, UShort aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 2);

    CMB_BLOCK_WRITE_BYTE2(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteSInt(cmbBlock *aBlock, SInt aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteUInt(cmbBlock *aBlock, UInt aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteSLong(cmbBlock *aBlock, SLong aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 8);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteULong(cmbBlock *aBlock, ULong aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 8);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteSFloat(cmbBlock *aBlock, SFloat aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteSDouble(cmbBlock *aBlock, SDouble aValue, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 8);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aValue);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

/* BUG-18945 */
IDE_RC cmbBlockWriteLobLocator(cmbBlock *aBlock, cmtLobLocator *aLobLocator, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 12);

    CMB_BLOCK_WRITE_BYTE8(aBlock, aLobLocator->mLocator);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aLobLocator->mSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteDateTime(cmbBlock *aBlock, cmtDateTime *aDateTime, idBool *aSuccess)
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

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteInterval(cmbBlock *aBlock, cmtInterval *aInterval, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 16);

    /*
     * Interval 씀
     */
    CMB_BLOCK_WRITE_BYTE8(aBlock, aInterval->mSecond);
    CMB_BLOCK_WRITE_BYTE8(aBlock, aInterval->mMicroSecond);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);

    IDE_EXCEPTION_END;

    return IDE_SUCCESS;
}

IDE_RC cmbBlockWriteNumeric(cmbBlock *aBlock, cmtNumeric *aNumeric, idBool *aSuccess)
{
    IDE_TEST_RAISE(aNumeric->mSize > CMT_NUMERIC_DATA_SIZE, SizeOverflow);

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
    idlOS::memcpy(aBlock->mData + aBlock->mCursor, aNumeric->mData, aNumeric->mSize);

    CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, aNumeric->mSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(SizeOverflow);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_NUMERIC_SIZE_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteVariable(cmbBlock *aBlock, cmtVariable *aVariable, UInt *aSizeLeft, idBool *aSuccess)
{
    UInt   sSize;
    UInt   sOffset;
    UShort sPieceSize;

    /*
     * Variable Size 획득 및 검사
     */
    sSize = cmtVariableGetSize(aVariable);

    IDE_TEST_RAISE((sSize > 0) && (aVariable->mPieceCount == 0), InvalidVariableType);

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
    sPieceSize = IDL_MIN(*aSizeLeft,
                         (UShort)(aBlock->mBlockSize -
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
    IDE_TEST(cmtVariableCopy(aVariable,
                             aBlock->mData + aBlock->mCursor,
                             sOffset,
                             sPieceSize) != IDE_SUCCESS);

    CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, sPieceSize);

    /*
     * 마지막 Piece이면 끝
     */
    *aSuccess = *aSizeLeft ? ID_FALSE : ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidVariableType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_VARIABLE_TYPE));
    }
    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1920 */
IDE_RC cmbBlockWritePtr(cmbBlock *aBlock, cmtInVariable *aInVariable, idBool *aSuccess)
{
    UInt sPtrSize;

    sPtrSize = ID_SIZEOF(vULong);

    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, sPtrSize + 4);

    /*
     * IN BLOCK Ptr이 가리키는 데이터의 Size 씀
     */
    CMB_BLOCK_WRITE_BYTE4(aBlock, aInVariable->mSize);

    /*
     * IN BLOCK Ptr 씀
     */
    CMB_BLOCK_WRITE_PTR(aBlock, aInVariable->mData, sPtrSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        IDE_DASSERT(0);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteInVariable(cmbBlock *aBlock, cmtInVariable *aInVariable, idBool *aSuccess)
{
    UChar sInVariableDelimeter = 0;
    
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, aInVariable->mSize + 5);

    /*
     * IN BLOCK Variable Write Size 씀
     */
    CMB_BLOCK_WRITE_BYTE4(aBlock, aInVariable->mSize);

    /*
     * IN BLOCK Variable Data 씀
     */
    idlOS::memcpy(aBlock->mData + aBlock->mCursor, aInVariable->mData, aInVariable->mSize);

    CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, aInVariable->mSize);

    /*
     * IN BLOCK Variable Delimeter 씀
     * Make in-variable data null terminated string
     */
    CMB_BLOCK_WRITE_BYTE1(aBlock, sInVariableDelimeter);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        IDE_DASSERT(0);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteBit(cmbBlock *aBlock, cmtBit *aBit, UInt *aSizeLeft, idBool *aSuccess)
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

        IDE_TEST(cmbBlockWriteVariable(aBlock, &aBit->mData, aSizeLeft, aSuccess) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(cmbBlockWriteVariable(aBlock, &aBit->mData, aSizeLeft, aSuccess) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteInBit(cmbBlock *aBlock, cmtInBit *aInBit, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4 + 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aInBit->mPrecision);

    IDE_TEST(cmbBlockWriteInVariable(aBlock, &aInBit->mData, aSuccess) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteNibble(cmbBlock *aBlock, cmtNibble *aNibble, UInt *aSizeLeft, idBool *aSuccess)
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

        IDE_TEST(cmbBlockWriteVariable(aBlock, &aNibble->mData, aSizeLeft, aSuccess) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(cmbBlockWriteVariable(aBlock, &aNibble->mData, aSizeLeft, aSuccess) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteInNibble(cmbBlock *aBlock, cmtInNibble *aInNibble, idBool *aSuccess)
{
    CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, 4 + 4);

    CMB_BLOCK_WRITE_BYTE4(aBlock, aInNibble->mPrecision);

    IDE_TEST(cmbBlockWriteInVariable(aBlock, &aInNibble->mData, aSuccess) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EndOfBlock);
    {
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteAny(cmbBlock *aBlock, cmtAny *aAny, UInt *aSizeLeft, idBool *aSuccess)
{
    UShort sCursor;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 범위 검사
     */
    IDE_ASSERT((aAny->mType > CMT_ID_NONE) && (aAny->mType < CMT_ID_MAX));

    /*
     * Type ID 씀
     */
    IDE_TEST(cmbBlockWriteUChar(aBlock, aAny->mType, aSuccess) != IDE_SUCCESS);

    if (*aSuccess == ID_TRUE)
    {
        /*
         * Type 별로 Data 씀
         */
        switch (aAny->mType)
        {
            case CMT_ID_NULL:
                break;

            case CMT_ID_SINT8:
                IDE_TEST(cmbBlockWriteSChar(aBlock, aAny->mValue.mSInt8, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT8:
                IDE_TEST(cmbBlockWriteUChar(aBlock, aAny->mValue.mUInt8, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_SINT16:
                IDE_TEST(cmbBlockWriteSShort(aBlock, aAny->mValue.mSInt16, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT16:
                IDE_TEST(cmbBlockWriteUShort(aBlock, aAny->mValue.mUInt16, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_SINT32:
                IDE_TEST(cmbBlockWriteSInt(aBlock, aAny->mValue.mSInt32, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT32:
                IDE_TEST(cmbBlockWriteUInt(aBlock, aAny->mValue.mUInt32, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_SINT64:
                IDE_TEST(cmbBlockWriteSLong(aBlock, aAny->mValue.mSInt64, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_UINT64:
                IDE_TEST(cmbBlockWriteULong(aBlock, aAny->mValue.mUInt64, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_FLOAT32:
                IDE_TEST(cmbBlockWriteSFloat(aBlock, aAny->mValue.mFloat32, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_FLOAT64:
                IDE_TEST(cmbBlockWriteSDouble(aBlock, aAny->mValue.mFloat64, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_DATETIME:
                IDE_TEST(cmbBlockWriteDateTime(aBlock, &aAny->mValue.mDateTime, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_INTERVAL:
                IDE_TEST(cmbBlockWriteInterval(aBlock, &aAny->mValue.mInterval, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_NUMERIC:
                IDE_TEST(cmbBlockWriteNumeric(aBlock, &aAny->mValue.mNumeric, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_VARIABLE:
            case CMT_ID_BINARY:
                IDE_TEST(cmbBlockWriteVariable(aBlock, &aAny->mValue.mVariable, aSizeLeft, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_IN_VARIABLE:
            case CMT_ID_IN_BINARY:
                IDE_TEST(cmbBlockWriteInVariable(aBlock, &aAny->mValue.mInVariable, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_BIT:
                IDE_TEST(cmbBlockWriteBit(aBlock, &aAny->mValue.mBit, aSizeLeft, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_IN_BIT:
                IDE_TEST(cmbBlockWriteInBit(aBlock, &aAny->mValue.mInBit, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_NIBBLE:
                IDE_TEST(cmbBlockWriteNibble(aBlock, &aAny->mValue.mNibble, aSizeLeft, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_IN_NIBBLE:
                IDE_TEST(cmbBlockWriteInNibble(aBlock, &aAny->mValue.mInNibble, aSuccess) != IDE_SUCCESS);
                break;
                
            case CMT_ID_LOBLOCATOR:
                /* BUG-18945 */
                IDE_TEST(cmbBlockWriteLobLocator(aBlock, &aAny->mValue.mLobLocator, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_PTR:
                /* PROJ-1920 */
                IDE_TEST(cmbBlockWritePtr(aBlock, &aAny->mValue.mInVariable, aSuccess) != IDE_SUCCESS);
                break;

            default:
                IDE_RAISE(InvalidType);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_DATATYPE));
    }
    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor   = sCursor;
        aBlock->mDataSize = sCursor;
    }

    return IDE_FAILURE;
}

IDE_RC cmbBlockWriteCollection(cmbBlock *aBlock, cmtCollection *aCollection, UInt *aSizeLeft, idBool *aSuccess)
{
    UShort sCursor;

    /*
     * Cursor 위치 복사
     */
    sCursor = aBlock->mCursor;

    /*
     * Type ID 범위 검사
     */
    IDE_ASSERT((aCollection->mType == CMT_ID_VARIABLE) || 
               (aCollection->mType == CMT_ID_IN_VARIABLE));

    /*
     * Type ID 씀
     */
    IDE_TEST(cmbBlockWriteUChar(aBlock, aCollection->mType, aSuccess) != IDE_SUCCESS);

    if (*aSuccess == ID_TRUE)
    {
        /*
         * Type 별로 Data 씀
         */
        switch (aCollection->mType)
        {
            case CMT_ID_VARIABLE:
                IDE_TEST(cmbBlockWriteVariable(aBlock, &aCollection->mValue.mVariable, aSizeLeft, aSuccess) != IDE_SUCCESS);
                break;

            case CMT_ID_IN_VARIABLE:
                IDE_TEST(cmbBlockWriteInVariable(aBlock, &aCollection->mValue.mInVariable, aSuccess) != IDE_SUCCESS);
                break;

            default:
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        /*
         * Cursor 위치 복구
         */
        aBlock->mCursor   = sCursor;
        aBlock->mDataSize = sCursor;
    }

    return IDE_FAILURE;
}

UInt cmbBlockGetIPCDASimpleQueryDataBlockSize()
{
#if defined(SMALL_FOOTPRINT)
    return 4 * 1024;
#else
    return cmuProperty::getIPCDASimpleQueryDataBlockSize() * 1024;
#endif
}
