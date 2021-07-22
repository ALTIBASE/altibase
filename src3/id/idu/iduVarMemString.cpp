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

#include <iduVarMemString.h>


#define IDU_VAR_MEM_STRING_PIECE_HEADER_SIZE                                \
    ((UInt) ((vULong) (((iduVarMemStringPiece *)0)->mData) & 0xffffffff))

#define IDU_VAR_MEM_STRING_PIECE_BUFFER_SIZE(aString)               \
    (aString->mPieceSize - IDU_VAR_MEM_STRING_PIECE_HEADER_SIZE)


static IDE_RC iduVarMemStringPieceAlloc(iduVarMemString *aString, iduVarMemStringPiece **aPiece)
{
    IDE_TEST(aString->mVarMem->alloc(aString->mPieceSize, (void **)aPiece) != IDE_SUCCESS);

    IDU_LIST_INIT_OBJ(&(*aPiece)->mPieceListNode, *aPiece);

    (*aPiece)->mLength = 0;

    IDU_LIST_ADD_LAST(&aString->mPieceList, &(*aPiece)->mPieceListNode);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC iduVarMemStringPieceFree(iduVarMemString *aString, iduVarMemStringPiece *aPiece)
{
    IDU_LIST_REMOVE(&aPiece->mPieceListNode);

    IDE_TEST(aString->mVarMem->free(aPiece) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarMemStringInitialize(iduVarMemString *aString,
                                 iduVarMemList   *aVarMem,
                                 UInt             aPieceSize)
{
    aString->mVarMem    = aVarMem;
    aString->mPieceSize = aPieceSize;
    aString->mLength    = 0;
    aString->mBuffer    = NULL;

    IDU_LIST_INIT(&aString->mPieceList);

    return IDE_SUCCESS;
}

IDE_RC iduVarMemStringFinalize(iduVarMemString *aString)
{
    IDE_TEST(iduVarMemStringTruncate(aString, ID_FALSE) != IDE_SUCCESS);

    if (aString->mBuffer != NULL)
    {
        IDE_TEST(aString->mVarMem->free(aString->mBuffer) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt iduVarMemStringGetLength(iduVarMemString *aString)
{
    return aString->mLength;
}

IDE_RC iduVarMemStringTruncate(iduVarMemString *aString, idBool aReserveFlag)
{
    iduListNode          *sIterator;
    iduListNode          *sNextNode;
    iduVarMemStringPiece *sPiece;

    IDU_LIST_ITERATE_SAFE(&aString->mPieceList, sIterator, sNextNode)
    {
        sPiece = (iduVarMemStringPiece *)sIterator->mObj;

        if (aReserveFlag == ID_FALSE)
        {
            IDE_TEST(iduVarMemStringPieceFree(aString, sPiece) != IDE_SUCCESS);
        }
        else
        {
            sPiece->mLength = 0;
            aReserveFlag    = ID_FALSE;
        }
    }

    aString->mLength = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarMemStringPrint(iduVarMemString *aString, const SChar *aCString)
{
    return iduVarMemStringPrintLength(aString, aCString, idlOS::strlen(aCString));
}

IDE_RC iduVarMemStringPrintLength(iduVarMemString *aString, const SChar *aCString, UInt aLength)
{
    IDE_TEST(iduVarMemStringTruncate(aString, ID_TRUE) != IDE_SUCCESS);

    IDE_TEST(iduVarMemStringAppendLength(aString, aCString, aLength) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarMemStringPrintFormat(iduVarMemString *aString, const SChar *aFormat, ...)
{
    va_list sArg;
    IDE_RC  sRet;

    va_start(sArg, aFormat);
    sRet = iduVarMemStringPrintFormatV(aString, aFormat, sArg);
    va_end(sArg);

    return sRet;
}

IDE_RC iduVarMemStringPrintFormatV(iduVarMemString *aString, const SChar *aFormat, va_list aArgs)
{
    IDE_TEST(iduVarMemStringTruncate(aString, ID_TRUE) != IDE_SUCCESS);

    IDE_TEST(iduVarMemStringAppendFormatV(aString, aFormat, aArgs) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarMemStringAppend(iduVarMemString *aString, const SChar *aCString)
{
    return iduVarMemStringAppendLength(aString, aCString, idlOS::strlen(aCString));
}

IDE_RC iduVarMemStringAppendLength(iduVarMemString *aString, const SChar *aCString, UInt aLength)
{
    iduVarMemStringPiece *sPiece;
    iduListNode          *sIterator;
    UInt                  sLenLeft; /* 복사해야할 문자열의 남은 길이 */
    UInt                  sLenCopy; /* Piece에 복사할(한) 길이       */

    sIterator = aString->mPieceList.mPrev;

    for (sLenLeft = aLength; sLenLeft > 0; sIterator = sPiece->mPieceListNode.mNext)
    {
        sPiece = (iduVarMemStringPiece *)sIterator->mObj;

        if (sPiece == NULL)
        {
            IDE_TEST(iduVarMemStringPieceAlloc(aString, &sPiece) != IDE_SUCCESS);
        }

        sLenCopy = IDL_MIN(sLenLeft,
                           IDU_VAR_MEM_STRING_PIECE_BUFFER_SIZE(aString) - sPiece->mLength - 1);

        idlOS::memcpy(sPiece->mData + sPiece->mLength,
                      aCString + aLength - sLenLeft,
                      sLenCopy);

        sPiece->mLength  += sLenCopy;
        aString->mLength += sLenCopy;
        sLenLeft         -= sLenCopy;

        sPiece->mData[sPiece->mLength] = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarMemStringAppendFormat(iduVarMemString *aString, const SChar *aFormat, ...)
{
    va_list sArg;
    IDE_RC  sRet;

    va_start(sArg, aFormat);
    sRet = iduVarMemStringAppendFormatV(aString, aFormat, sArg);
    va_end(sArg);

    return sRet;
}

IDE_RC iduVarMemStringAppendFormatV(iduVarMemString *aString, const SChar *aFormat, va_list aArgs)
{
    UInt sLength;

    if (aString->mBuffer == NULL)
    {
        IDE_TEST(aString->mVarMem->alloc(aString->mPieceSize, (void **)&aString->mBuffer) != IDE_SUCCESS);
    }

    sLength = idlOS::vsnprintf(aString->mBuffer, aString->mPieceSize, aFormat, aArgs);

    IDE_TEST_RAISE(sLength >= aString->mPieceSize, TooLong);

    IDE_TEST(iduVarMemStringAppendLength(aString, aString->mBuffer, sLength) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(TooLong);
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_VAR_STRING_APPEND_FORMAT_TOO_LONG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarMemStringExtractString(iduVarMemString *aString, SChar *aBuffer, UInt aBufferSize)
{   
    iduListNode          *sIterator;
    iduVarMemStringPiece *sPiece;
    UInt                  sSize;
    UInt                  sTotalSize;

    sTotalSize = aString->mLength;

    if( aBufferSize < sTotalSize )
    {
        IDE_ERROR( 0 );
    }
    else
    {
        IDU_LIST_ITERATE(&aString->mPieceList, sIterator)
        {
            sPiece = (iduVarMemStringPiece *)sIterator->mObj;
            sSize = ( sPiece->mLength < sTotalSize ) ? sPiece->mLength : sTotalSize;

            idlOS::memcpy( aBuffer, sPiece->mData, sSize );

            aBuffer    += sSize;
            sTotalSize -= sSize;
        }
    }

    IDE_DASSERT( sTotalSize == 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
