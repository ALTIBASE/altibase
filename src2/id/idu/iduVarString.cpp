/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduVarString.cpp 68602 2015-01-23 00:13:11Z sbjang $
 ****************************************************************************/

#include <iduVarString.h>


#define IDU_VAR_STRING_PIECE_HEADER_SIZE                                \
    ((UInt) ((vULong) (((iduVarStringPiece *)0)->mData) & 0xffffffff))

#define IDU_VAR_STRING_PIECE_BUFFER_SIZE(aString)               \
    (aString->mPieceSize - IDU_VAR_STRING_PIECE_HEADER_SIZE)


static IDE_RC iduVarStringPieceAlloc(iduVarString *aString, iduVarStringPiece **aPiece)
{
    IDE_TEST(aString->mPiecePool->alloc((void **)aPiece) != IDE_SUCCESS);

    IDU_LIST_INIT_OBJ(&(*aPiece)->mPieceListNode, *aPiece);

    (*aPiece)->mLength = 0;

    IDU_LIST_ADD_LAST(&aString->mPieceList, &(*aPiece)->mPieceListNode);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC iduVarStringPieceFree(iduVarString *aString, iduVarStringPiece *aPiece)
{
    IDU_LIST_REMOVE(&aPiece->mPieceListNode);

    IDE_TEST(aString->mPiecePool->memfree(aPiece) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarStringInitialize(iduVarString *aString,
                              iduMemPool   *aPool,
                              UInt          aPieceSize)
{
    aString->mPiecePool = aPool;
    aString->mPieceSize = aPieceSize;
    aString->mLength    = 0;
    aString->mBuffer    = NULL;

    IDU_LIST_INIT(&aString->mPieceList);

    return IDE_SUCCESS;
}

IDE_RC iduVarStringFinalize(iduVarString *aString)
{
    IDE_TEST(iduVarStringTruncate(aString, ID_FALSE) != IDE_SUCCESS);

    if (aString->mBuffer != NULL)
    {
        IDE_TEST(aString->mPiecePool->memfree(aString->mBuffer) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt iduVarStringGetLength(iduVarString *aString)
{
    return aString->mLength;
}

IDE_RC iduVarStringTruncate(iduVarString *aString, idBool aReserveFlag)
{
    iduListNode       *sIterator;
    iduListNode       *sNextNode;
    iduVarStringPiece *sPiece;

    IDU_LIST_ITERATE_SAFE(&aString->mPieceList, sIterator, sNextNode)
    {
        sPiece = (iduVarStringPiece *)sIterator->mObj;

        if (aReserveFlag == ID_FALSE)
        {
            IDE_TEST(iduVarStringPieceFree(aString, sPiece) != IDE_SUCCESS);
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

IDE_RC iduVarStringPrint(iduVarString *aString, const SChar *aCString)
{
    return iduVarStringPrintLength(aString, aCString, idlOS::strlen(aCString));
}

IDE_RC iduVarStringPrintLength(iduVarString *aString, const SChar *aCString, UInt aLength)
{
    IDE_TEST(iduVarStringTruncate(aString, ID_TRUE) != IDE_SUCCESS);

    IDE_TEST(iduVarStringAppendLength(aString, aCString, aLength) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarStringPrintFormat(iduVarString *aString, const SChar *aFormat, ...)
{
    va_list sArg;
    IDE_RC  sRet;

    va_start(sArg, aFormat);
    sRet = iduVarStringPrintFormatV(aString, aFormat, sArg);
    va_end(sArg);

    return sRet;
}

IDE_RC iduVarStringPrintFormatV(iduVarString *aString, const SChar *aFormat, va_list aArgs)
{
    IDE_TEST(iduVarStringTruncate(aString, ID_TRUE) != IDE_SUCCESS);

    IDE_TEST(iduVarStringAppendFormatV(aString, aFormat, aArgs) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduVarStringAppend(iduVarString *aString, const SChar *aCString)
{
    return iduVarStringAppendLength(aString, aCString, idlOS::strlen(aCString));
}

IDE_RC iduVarStringAppendLength(iduVarString *aString, const SChar *aCString, UInt aLength)
{
    iduVarStringPiece *sPiece;
    iduListNode       *sIterator;
    UInt               sLenLeft; /* 복사해야할 문자열의 남은 길이 */
    UInt               sLenCopy; /* Piece에 복사할(한) 길이       */

    sIterator = aString->mPieceList.mPrev;

    for (sLenLeft = aLength; sLenLeft > 0; sIterator = sPiece->mPieceListNode.mNext)
    {
        sPiece = (iduVarStringPiece *)sIterator->mObj;

        if (sPiece == NULL)
        {
            IDE_TEST(iduVarStringPieceAlloc(aString, &sPiece) != IDE_SUCCESS);
        }

        sLenCopy = IDL_MIN(sLenLeft,
                           IDU_VAR_STRING_PIECE_BUFFER_SIZE(aString) - sPiece->mLength - 1);

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

IDE_RC iduVarStringAppendFormat(iduVarString *aString, const SChar *aFormat, ...)
{
    va_list sArg;
    IDE_RC  sRet;

    va_start(sArg, aFormat);
    sRet = iduVarStringAppendFormatV(aString, aFormat, sArg);
    va_end(sArg);

    return sRet;
}

IDE_RC iduVarStringAppendFormatV(iduVarString *aString, const SChar *aFormat, va_list aArgs)
{
    UInt sLength;

    if (aString->mBuffer == NULL)
    {
        IDE_TEST(aString->mPiecePool->alloc((void **)&aString->mBuffer) != IDE_SUCCESS);
    }

    sLength = idlOS::vsnprintf(aString->mBuffer, aString->mPieceSize, aFormat, aArgs);

    IDE_TEST_RAISE(sLength >= aString->mPieceSize, TooLong);

    IDE_TEST(iduVarStringAppendLength(aString, aString->mBuffer, sLength) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(TooLong);
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_VAR_STRING_APPEND_FORMAT_TOO_LONG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description:
 * iduVarString을 SChar로 변환
 *
 * aSrc       - [in]  원본 iduVarString
 * aDstBufLen - [in]  복사 버퍼의 길이
 * aDstBuf    - [out] 복사 버퍼
 ******************************************************************************/
IDE_RC iduVarStringConvToCString( iduVarString *aSrc, UInt aDstBufLen, SChar *aDstBuf )
{
    iduListNode       *sIterator;
    iduVarStringPiece *sPiece;
    UInt               sOffset = 0;

    IDE_TEST_RAISE( aDstBufLen <= iduVarStringGetLength( aSrc ),
                    err_small_buffer );

    IDU_LIST_ITERATE( &aSrc->mPieceList, sIterator )
    {
        sPiece = ( iduVarStringPiece * )sIterator->mObj;

        IDE_TEST_RAISE(sPiece->mLength > ID_USHORT_MAX, err_small_buffer);

       (void)idlOS::memcpy( ( void * )( aDstBuf + sOffset ),
                            ( void * )sPiece->mData,
                            sPiece->mLength);

       sOffset += sPiece->mLength;
    }

    aDstBuf[sOffset] = '\0';


    return IDE_SUCCESS;

    IDE_EXCEPTION( err_small_buffer )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_BUFFER_SIZE_TOO_SMALL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
