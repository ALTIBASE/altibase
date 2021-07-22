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
  
/***********************************************************************
 * $Id: aciVarString.c 11106 2010-05-20 11:06:29Z gurugio $
 **********************************************************************/
#include <aciVarString.h>

#define IDU_VAR_STRING_PIECE_HEADER_SIZE                                \
        ((acp_uint32_t) ((acp_ulong_t) (((aci_var_string_piece_t *)0)->mData) & 0xffffffff))

#define IDU_VAR_STRING_PIECE_BUFFER_SIZE(aString)               \
        (aString->mPieceSize - IDU_VAR_STRING_PIECE_HEADER_SIZE)


static ace_rc_t aciVarStringPieceAlloc(aci_var_string_t *aString, aci_var_string_piece_t **aPiece)
{
    ACP_TEST(ACP_RC_NOT_SUCCESS(aclMemPoolAlloc(aString->mPiecePool,
                                                (void **)aPiece)));

    acpListInitObj(&(*aPiece)->mPieceListNode, *aPiece);

    (*aPiece)->mLength = 0;

    acpListAppendNode(&aString->mPieceList, &(*aPiece)->mPieceListNode);

    return ACE_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return ACE_RC_FAILURE;
}

static ace_rc_t aciVarStringPieceFree(aci_var_string_t *aString, aci_var_string_piece_t *aPiece)
{
    acpListDeleteNode(&aPiece->mPieceListNode);

    aclMemPoolFree(aString->mPiecePool, aPiece);

    return ACE_RC_SUCCESS;
}


ACP_EXPORT
ace_rc_t aciVarStringInitialize(aci_var_string_t   *aString,
                                acl_mem_pool_t *aPool,
                                acp_uint32_t    aPieceSize)
{
    aString->mPiecePool = aPool;
    aString->mPieceSize = aPieceSize;
    aString->mLength    = 0;
    aString->mBuffer    = NULL;

    acpListInit(&aString->mPieceList);

    return ACE_RC_SUCCESS;
}

ACP_EXPORT
ace_rc_t aciVarStringFinalize(aci_var_string_t *aString)
{
    ACP_TEST(aciVarStringTruncate(aString, ACP_FALSE) != ACE_RC_SUCCESS);

    if (aString->mBuffer != NULL)
    {
        aclMemPoolFree(aString->mPiecePool, aString->mBuffer);
    }

    return ACE_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return ACE_RC_FAILURE;
}

ACP_EXPORT
acp_uint32_t aciVarStringGetLength(aci_var_string_t *aString)
{
    return aString->mLength;
}

ACP_EXPORT
ace_rc_t aciVarStringTruncate(aci_var_string_t *aString, acp_bool_t aReserveFlag)
{
    acp_list_node_t   *sIterator = NULL;
    acp_list_node_t   *sNextNode = NULL;
    aci_var_string_piece_t *sPiece = NULL;


    ACP_LIST_ITERATE_SAFE(&aString->mPieceList, sIterator, sNextNode)
    {
        sPiece = (aci_var_string_piece_t *)sIterator->mObj;

        if (aReserveFlag == ACP_FALSE)
        {
            ACP_TEST(aciVarStringPieceFree(aString, sPiece) != ACE_RC_SUCCESS);
        }
        else
        {
            sPiece->mLength = 0;
            aReserveFlag    = ACP_FALSE;
        }
    }

    aString->mLength = 0;

    return ACE_RC_SUCCESS;

    ACP_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

ACP_EXPORT
ace_rc_t aciVarStringPrint(aci_var_string_t *aString, const acp_char_t *aCString)
{
    return aciVarStringPrintLength(aString, aCString, acpCStrLen(aCString, ACP_UINT32_MAX));
}

ACP_EXPORT
ace_rc_t aciVarStringPrintLength(aci_var_string_t *aString, const acp_char_t *aCString, acp_uint32_t aLength)
{
    ACP_TEST(aciVarStringTruncate(aString, ACP_TRUE) != ACE_RC_SUCCESS);

    ACP_TEST(aciVarStringAppendLength(aString, aCString, aLength) != ACE_RC_SUCCESS);

    return ACE_RC_SUCCESS;

    ACP_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

ACP_EXPORT
ace_rc_t aciVarStringPrintFormat(aci_var_string_t *aString, const acp_char_t *aFormat, ...)
{
    va_list sArg;
    ace_rc_t  sRet;

    va_start(sArg, aFormat);
    sRet = aciVarStringPrintFormatV(aString, aFormat, sArg);
    va_end(sArg);

    return sRet;
}

ACP_EXPORT
ace_rc_t aciVarStringPrintFormatV(aci_var_string_t *aString, const acp_char_t *aFormat, va_list aArgs)
{
    ACP_TEST(aciVarStringTruncate(aString, ACP_TRUE) != ACE_RC_SUCCESS);

    ACP_TEST(aciVarStringAppendFormatV(aString, aFormat, aArgs) != ACE_RC_SUCCESS);

    return ACE_RC_SUCCESS;

    ACP_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

ACP_EXPORT
ace_rc_t aciVarStringAppend(aci_var_string_t *aString, const acp_char_t *aCString)
{
    return aciVarStringAppendLength(aString, aCString, acpCStrLen(aCString, ACP_UINT32_MAX));
}

ACP_EXPORT
ace_rc_t aciVarStringAppendLength(aci_var_string_t *aString, const acp_char_t *aCString, acp_uint32_t aLength)
{
    aci_var_string_piece_t *sPiece = NULL;
    acp_list_node_t   *sIterator = aString->mPieceList.mPrev;
    acp_uint32_t       sLenLeft; /* 복사해야할 문자열의 남은 길이 */
    acp_uint32_t       sLenCopy; /* Piece에 복사할(한) 길이       */

    for (sLenLeft = aLength; sLenLeft > 0; sIterator = sPiece->mPieceListNode.mNext)
    {
        sPiece = (aci_var_string_piece_t *)sIterator->mObj;

        if (sPiece == NULL)
        {
            ACP_TEST(aciVarStringPieceAlloc(aString, &sPiece) != ACE_RC_SUCCESS);
        }

        sLenCopy = ACP_MIN(sLenLeft,
                           IDU_VAR_STRING_PIECE_BUFFER_SIZE(aString) - sPiece->mLength - 1);

        acpMemCpy(sPiece->mData + sPiece->mLength,
                  aCString + aLength - sLenLeft,
                  sLenCopy);

        sPiece->mLength  += sLenCopy;
        aString->mLength += sLenCopy;
        sLenLeft         -= sLenCopy;

        sPiece->mData[sPiece->mLength] = 0;
    }

    return ACE_RC_SUCCESS;
    ACP_EXCEPTION_END;
    return ACE_RC_FAILURE;
}

ACP_EXPORT
ace_rc_t aciVarStringAppendFormat(aci_var_string_t *aString, const acp_char_t *aFormat, ...)
{
    va_list  sArg;
    ace_rc_t sRet;

    va_start(sArg, aFormat);
    sRet = aciVarStringAppendFormatV(aString, aFormat, sArg);
    va_end(sArg);

    return sRet;
}

ACP_EXPORT
ace_rc_t aciVarStringAppendFormatV(aci_var_string_t *aString, const acp_char_t *aFormat, va_list aArgs)
{
    acp_rc_t sRet;

    if (aString->mBuffer == NULL)
    {
        ACP_TEST(ACP_RC_NOT_SUCCESS(aclMemPoolAlloc(aString->mPiecePool,
                                                    (void **)&aString->mBuffer)));
    }

    sRet = acpVsnprintf(aString->mBuffer, aString->mPieceSize, aFormat, aArgs);

    ACP_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), TooLong);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRet));

    ACP_TEST(aciVarStringAppendLength(aString,
                                      aString->mBuffer,
                                      acpCStrLen(aString->mBuffer, aString->mPieceSize))
             != ACE_RC_SUCCESS);

    return ACE_RC_SUCCESS;

    ACP_EXCEPTION(TooLong);
    {
        /*
         * BUGBUG
         * IDE_SET(ideSetErrorCode(idERR_IGNORE_VAR_STRING_APPEND_FORMAT_TOO_LONG));
         */
    }
    ACP_EXCEPTION_END;

    return ACE_RC_FAILURE;
}
