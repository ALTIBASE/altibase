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
#include <ulnConvToDOUBLE.h>
#include <ulnConvBit.h>
#include <ulnConvNumeric.h>

ACI_RC ulncCHAR_DOUBLE(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    acp_sint32_t sScale = 0;
    acp_double_t sDoubleValue = 0;


    ACI_TEST_RAISE(ulncIsValidNumericLiterals((acp_char_t *)aColumn->mBuffer, aColumn->mDataLength, &sScale)
                   != ACP_TRUE,
                   LABEL_INVALID_LITERAL);

    /* BUG-33527 The correct size of data should be used when converting data in UL */
    ACI_TEST_RAISE(ACP_RC_IS_ERANGE(
                       acpCStrToDouble((const acp_char_t *)aColumn->mBuffer,
                                       aColumn->mDataLength,
                                       &sDoubleValue,
                                       NULL)),
                   LABEL_OUT_OF_RANGE);

    /*
     * BUGBUG : fractional truncation 을 체크하려면 numeric 등으로 변환을 해야 하는데,
     *          일단은 포기하자. 다음에 시간이 되면
     *          strtod 함수를 직접 만들어서 그 함수에서 체크하도록 해야 하겠다.
     */
    *(acp_double_t *)aAppBuffer->mBuffer = sDoubleValue;

    aLength->mWritten = ACI_SIZEOF(acp_double_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }

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

ACI_RC ulncVARCHAR_DOUBLE(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    return ulncCHAR_DOUBLE(aFnContext,
                           aAppBuffer,
                           aColumn,
                           aLength,
                           aRowNumber);
}

ACI_RC ulncVARBIT_DOUBLE(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    return ulncBIT_DOUBLE(aFnContext,
                          aAppBuffer,
                          aColumn,
                          aLength,
                          aRowNumber);
}

ACI_RC ulncBIT_DOUBLE(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_uint8_t sBitValue;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sBitValue = ulnConvBitToUChar(aColumn->mBuffer);

    *(acp_double_t *)aAppBuffer->mBuffer = (acp_double_t)sBitValue;

    aLength->mWritten = ACI_SIZEOF(acp_double_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncSMALLINT_DOUBLE(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber)
{
    acp_sint16_t sSmallIntValue;
    acp_sint16_t sSmallIntValue2;
    acp_double_t sDoubleValue;

    sSmallIntValue  = *(acp_sint16_t *)aColumn->mBuffer;
    sDoubleValue    = sSmallIntValue;
    sSmallIntValue2 = (acp_sint16_t)sDoubleValue;

    *(acp_double_t *)aAppBuffer->mBuffer = (acp_double_t)sSmallIntValue;

    if(sSmallIntValue != sSmallIntValue2)
    {
        /*
         * 01s07
         *
         * BUGBUG : out of range 를 내어 줘야 할지, fractional truncation 을 줘야 할지..
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);
    aLength->mWritten = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncINTEGER_DOUBLE(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    acp_sint32_t sSInt32Value;
    acp_sint32_t sSInt32Value2;
    acp_double_t sDoubleValue;


    sSInt32Value  = *(acp_sint32_t *)aColumn->mBuffer;
    sDoubleValue  = sSInt32Value;
    sSInt32Value2 = (acp_sint32_t)sDoubleValue;

    *(acp_double_t *)aAppBuffer->mBuffer = sDoubleValue;

    if(sSInt32Value2 != sSInt32Value)
    {
        /*
         * 01s07
         *
         * BUGBUG : out of range 를 내어 줘야 할지, fractional truncation 을 줘야 할지..
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);
    aLength->mWritten = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncBIGINT_DOUBLE(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    acp_sint64_t sBigIntValue;
    acp_sint64_t sBigIntValue2;
    acp_double_t sDoubleValue;


    sBigIntValue  = *(acp_sint64_t *)aColumn->mBuffer;
    sDoubleValue  = (acp_double_t)sBigIntValue;
    sBigIntValue2 = (acp_sint64_t)sDoubleValue;

    *(acp_double_t *)aAppBuffer->mBuffer = (acp_double_t)sBigIntValue;

    if(sBigIntValue != sBigIntValue2)
    {
        /*
         * 01s07
         *
         * BUGBUG : out of range 를 내어 줘야 할지, fractional truncation 을 줘야 할지..
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);
    aLength->mWritten = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncREAL_DOUBLE(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    acp_double_t sDoubleValue;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sDoubleValue = *(acp_float_t *)aColumn->mBuffer;

    *(acp_double_t *)aAppBuffer->mBuffer = sDoubleValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);
    aLength->mWritten = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncDOUBLE_DOUBLE(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    acp_double_t sDoubleValue;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sDoubleValue = *(acp_double_t *)aColumn->mBuffer;

    *(acp_double_t *)aAppBuffer->mBuffer = (acp_double_t)sDoubleValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);
    aLength->mWritten = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncINTERVAL_DOUBLE(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber)
{
    // BUG-21348
    // INTERVAL 타입을 DOUBLE로 컨버전하는 함수가 구현되어 있지 않았음
    cmtInterval  *sCmInterval;
    acp_double_t  sDoubleValue;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sCmInterval = (cmtInterval *)aColumn->mBuffer;

    sDoubleValue = ((acp_double_t)sCmInterval->mSecond / (3600 * 24)) +
                   (((acp_double_t)sCmInterval->mMicroSecond * 0.000001) / (3600 * 24));

    *(acp_double_t *)aAppBuffer->mBuffer = (acp_double_t)sDoubleValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);
    aLength->mWritten = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncNUMERIC_DOUBLE(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    cmtNumeric   *sCmNumeric;
    acp_double_t  sDoubleValue = 0;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sCmNumeric = (cmtNumeric *)aColumn->mBuffer;

    sDoubleValue = ulncCmtNumericToDouble(sCmNumeric);

    *(acp_double_t *)aAppBuffer->mBuffer = (acp_double_t)sDoubleValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);
    aLength->mWritten = ACI_SIZEOF(acp_double_t);

    return ACI_SUCCESS;
}

ACI_RC ulncNCHAR_DOUBLE(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_sint32_t  sScale = 0;
    acp_double_t  sDoubleValue = 0;
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

    ACI_TEST_RAISE(ulncIsValidNumericLiterals(
                       (acp_char_t*)ulnCharSetGetConvertedText(&sCharSet),
                       ulnCharSetGetConvertedTextLen(&sCharSet),
                       &sScale) != ACP_TRUE,
                   LABEL_INVALID_LITERAL);

    ACI_TEST_RAISE(ACP_RC_IS_ERANGE(
                       acpCStrToDouble((const acp_char_t *)ulnCharSetGetConvertedText(&sCharSet),
                                       ulnCharSetGetConvertedTextLen(&sCharSet),
                                       &sDoubleValue,
                                       NULL)),
                   LABEL_OUT_OF_RANGE);

    /*
     * BUGBUG : fractional truncation 을 체크하려면 numeric 등으로 변환을 해야 하는데,
     *          일단은 포기하자. 다음에 시간이 되면
     *          strtod 함수를 직접 만들어서 그 함수에서 체크하도록 해야 하겠다.
     */
    *(acp_double_t *)aAppBuffer->mBuffer = sDoubleValue;

    aLength->mWritten = ACI_SIZEOF(acp_double_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_double_t);

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
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
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

ACI_RC ulncNVARCHAR_DOUBLE(ulnFnContext  *aFnContext,
                           ulnAppBuffer  *aAppBuffer,
                           ulnColumn     *aColumn,
                           ulnLengthPair *aLength,
                           acp_uint16_t   aRowNumber)
{
    return ulncNCHAR_DOUBLE(aFnContext,
                            aAppBuffer,
                            aColumn,
                            aLength,
                            aRowNumber);
}
