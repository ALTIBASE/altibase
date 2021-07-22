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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnConv.h>
#include <ulnConvToBIT.h>
#include <ulnConvBit.h>
#include <ulnConvNumeric.h>

#define ULN_RETURN_BIT_TO_USER(aValueFromServer, aUserBuffer) do    \
{                                                                   \
    if((aValueFromServer) == 0)                                     \
    {                                                               \
        *(acp_uint8_t *)(aUserBuffer) = 0;                          \
    }                                                               \
    else                                                            \
    {                                                               \
        if((aValueFromServer) == 1)                                 \
        {                                                           \
            *(acp_uint8_t *)(aUserBuffer) = 1;                      \
        }                                                           \
        else                                                        \
        {                                                           \
            *(acp_uint8_t *)(aUserBuffer) = 1;                      \
                                                                    \
            ulnErrorExtended(aFnContext,                            \
                             aRowNumber,                            \
                             aColumn->mColumnNumber,                \
                             ulERR_IGNORE_FRACTIONAL_TRUNCATION);   \
        }                                                           \
    }                                                               \
                                                                    \
    aLength->mWritten = ACI_SIZEOF(acp_uint8_t);                        \
    aLength->mNeeded  = ACI_SIZEOF(acp_uint8_t);                        \
} while(0)

ACI_RC ulncCHAR_BIT(ulnFnContext  *aFnContext,
                    ulnAppBuffer  *aAppBuffer,
                    ulnColumn     *aColumn,
                    ulnLengthPair *aLength,
                    acp_uint16_t   aRowNumber)
{
    acp_sint32_t sScale = 0;
    acp_sint64_t sBigIntValue;
    acp_double_t sDoubleValue;

    ACI_TEST_RAISE(ulncIsValidNumericLiterals((acp_char_t *)aColumn->mBuffer,
                                              aColumn->mDataLength,
                                              &sScale) != ACP_TRUE,
                   LABEL_INVALID_LITERAL);

    sBigIntValue = ACP_SINT64_MAX;
    sDoubleValue = sBigIntValue;

    /* BUG-33527 The correct size of data should be used when converting data in UL */
    ACI_TEST_RAISE(acpCStrToDouble((const acp_char_t *)aColumn->mBuffer,
                                   aColumn->mDataLength,
                                   &sDoubleValue,
                                   (acp_char_t **)NULL) != ACP_RC_SUCCESS,
                   LABEL_INVALID_LITERAL);

    ACI_TEST_RAISE(sDoubleValue < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sDoubleValue >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sDoubleValue, aAppBuffer->mBuffer);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
        {
            /*
             * 22018
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_INVALID_NUMERIC_LITERAL);
        }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            /*
             * 22003
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncVARCHAR_BIT(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    return ulncCHAR_BIT(aFnContext,
                        aAppBuffer,
                        aColumn,
                        aLength,
                        aRowNumber);
}

ACI_RC ulncVARBIT_BIT(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    return ulncBIT_BIT(aFnContext,
                       aAppBuffer,
                       aColumn,
                       aLength,
                       aRowNumber);
}

ACI_RC ulncBIT_BIT(ulnFnContext  *aFnContext,
                   ulnAppBuffer  *aAppBuffer,
                   ulnColumn     *aColumn,
                   ulnLengthPair *aLength,
                   acp_uint16_t   aRowNumber)
{
    acp_uint8_t sBitValue;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sBitValue = ulnConvBitToUChar(aColumn->mBuffer);

    *(acp_uint8_t *)aAppBuffer->mBuffer = (acp_uint8_t)sBitValue;

    aLength->mWritten = ACI_SIZEOF(acp_uint8_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_uint8_t);

    return ACI_SUCCESS;
}

ACI_RC ulncSMALLINT_BIT(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_sint16_t sSmallIntValue;

    sSmallIntValue = *(acp_sint16_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sSmallIntValue < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sSmallIntValue >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sSmallIntValue, aAppBuffer->mBuffer);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncINTEGER_BIT(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    acp_sint32_t sSInt32Value;

    sSInt32Value = *(acp_sint32_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sSInt32Value < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sSInt32Value >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sSInt32Value, aAppBuffer->mBuffer);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncBIGINT_BIT(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_sint64_t sBigIntValue;

    sBigIntValue = *(acp_sint64_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sBigIntValue < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sBigIntValue >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sBigIntValue, aAppBuffer->mBuffer);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncREAL_BIT(ulnFnContext  *aFnContext,
                    ulnAppBuffer  *aAppBuffer,
                    ulnColumn     *aColumn,
                    ulnLengthPair *aLength,
                    acp_uint16_t   aRowNumber)
{
    acp_float_t sFloatValue;

    sFloatValue = *(acp_float_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sFloatValue < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sFloatValue >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sFloatValue, aAppBuffer->mBuffer);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            /*
             * 22003
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncDOUBLE_BIT(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_double_t sDoubleValue;

    sDoubleValue = *(acp_double_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sDoubleValue < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sDoubleValue >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sDoubleValue, aAppBuffer->mBuffer);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            /*
             * 22003
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncINTERVAL_BIT(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    ACP_UNUSED(aAppBuffer);
    ACP_UNUSED(aLength);

    /*
     * BUGBUG : 원래는 inteval 타입에 single field 만 있으면 가능하도록 해야 하는데,
     *          일단 이렇게 두자.
     */

    /*
     * 07006
     */
    return ulnErrorExtended(aFnContext,
                            aRowNumber,
                            aColumn->mColumnNumber,
                            ulERR_ABORT_INVALID_CONVERSION);
}

ACI_RC ulncNUMERIC_BIT(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    cmtNumeric  *sCmNumeric;
    acp_float_t  sFloatValue = 0;

    sCmNumeric = (cmtNumeric *)aColumn->mBuffer;

    sFloatValue = ulncCmtNumericToFloat(sCmNumeric);

    ACI_TEST_RAISE(sFloatValue < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sFloatValue >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sFloatValue, aAppBuffer->mBuffer);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            /*
             * 22003
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncNCHAR_BIT(ulnFnContext  *aFnContext,
                     ulnAppBuffer  *aAppBuffer,
                     ulnColumn     *aColumn,
                     ulnLengthPair *aLength,
                     acp_uint16_t   aRowNumber)
{
    acp_sint32_t  sScale = 0;
    acp_sint64_t  sBigIntValue;
    acp_double_t  sDoubleValue;
    ulnDbc       *sDbc;
    ulnCharSet    sCharSet;
    acp_sint32_t  sState = 0;

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnCharSetConvert(&sCharSet,
                               aFnContext,
                               NULL,
                               (const mtlModule*)sDbc->mNcharCharsetLangModule,
                               sDbc->mClientCharsetLangModule,    //BUG-22684
                               (void*)aColumn->mBuffer,
                               aColumn->mDataLength,
                               CONV_DATA_OUT)
             != ACI_SUCCESS);

    ACI_TEST_RAISE(ulncIsValidNumericLiterals((acp_char_t *)ulnCharSetGetConvertedText(&sCharSet),
                                              ulnCharSetGetConvertedTextLen(&sCharSet),
                                              &sScale) != ACP_TRUE,
                   LABEL_INVALID_LITERAL);

    sBigIntValue = ACP_SINT64_MAX;
    sDoubleValue = sBigIntValue;

    ACI_TEST_RAISE(
        acpCStrToDouble((const acp_char_t *)ulnCharSetGetConvertedText(&sCharSet),
                        ulnCharSetGetConvertedTextLen(&sCharSet),
                        &sDoubleValue,
                        (acp_char_t **)NULL) != ACP_RC_SUCCESS,
        LABEL_INVALID_LITERAL);

    ACI_TEST_RAISE(sDoubleValue < 0, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sDoubleValue >= 2, LABEL_OUT_OF_RANGE);

    ULN_RETURN_BIT_TO_USER(sDoubleValue, aAppBuffer->mBuffer);

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
        {
            /*
             * 22018
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_INVALID_NUMERIC_LITERAL);
        }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
        {
            /*
             * 22003
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumn->mColumnNumber,
                             ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE);
        }

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return ACI_FAILURE;
}

ACI_RC ulncNVARCHAR_BIT(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    return ulncNCHAR_BIT(aFnContext,
                         aAppBuffer,
                         aColumn,
                         aLength,
                         aRowNumber);
}
