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
#include <ulnConvToFLOAT.h>
#include <ulnConvBit.h>
#include <ulnConvNumeric.h>

ACI_RC ulncCHAR_FLOAT(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_sint32_t sScale = 0;
    acp_double_t sDoubleValue = 0;

    ACI_TEST_RAISE(ulncIsValidNumericLiterals((acp_char_t *)aColumn->mBuffer,
                                              aColumn->mDataLength,
                                              &sScale) != ACP_TRUE,
                   LABEL_INVALID_LITERAL);

    errno = 0;
    /* BUG-33527 The correct size of data should be used when converting data in UL */
    ACI_TEST_RAISE(ACP_RC_IS_ERANGE(
                       acpCStrToDouble((const acp_char_t *)aColumn->mBuffer,
                                       aColumn->mDataLength,
                                       &sDoubleValue,
                                       NULL)),
                   LABEL_OUT_OF_RANGE);

    ACI_TEST_RAISE(sDoubleValue < -(10e38), LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sDoubleValue > (10e38), LABEL_OUT_OF_RANGE);

    /*
     * BUGBUG : FRACTIONAL TRUNCATION 을 디텍트하는 루틴을 만들어 넣어야 한다.
     */
    // *(acp_float_t *)aAppBuffer->mBuffer = sFloatValue;
    *(acp_float_t *)aAppBuffer->mBuffer = sDoubleValue;

#if 0
    acpSnprintf(sDoubleString, 128, "%"ACI_DOUBLE_G_FMT, sDoubleValue);
    acpSnprintf(sFloatString,  128, "%"ACI_FLOAT_G_FMT, sFloatValue);

    if(acpCStrCmp(sDoubleString, sFloatString, 128) != 0)
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
#endif

    aLength->mWritten = ACI_SIZEOF(acp_float_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);

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

ACI_RC ulncVARCHAR_FLOAT(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    return ulncCHAR_FLOAT(aFnContext,
                          aAppBuffer,
                          aColumn,
                          aLength,
                          aRowNumber);
}

ACI_RC ulncVARBIT_FLOAT(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    return ulncBIT_FLOAT(aFnContext,
                         aAppBuffer,
                         aColumn,
                         aLength,
                         aRowNumber);
}

ACI_RC ulncBIT_FLOAT(ulnFnContext  *aFnContext,
                     ulnAppBuffer  *aAppBuffer,
                     ulnColumn     *aColumn,
                     ulnLengthPair *aLength,
                     acp_uint16_t   aRowNumber)
{
    acp_uint8_t sBitValue;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sBitValue = ulnConvBitToUChar(aColumn->mBuffer);

    *(acp_float_t *)aAppBuffer->mBuffer = (acp_float_t)sBitValue;

    aLength->mWritten = ACI_SIZEOF(acp_float_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);

    return ACI_SUCCESS;
}

ACI_RC ulncSMALLINT_FLOAT(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    acp_sint16_t sSmallIntValue;
    acp_sint16_t sSmallIntValue2;
    acp_float_t  sFloatValue;

    sSmallIntValue  = *(acp_sint16_t *)aColumn->mBuffer;
    sFloatValue     = sSmallIntValue;
    sSmallIntValue2 = (acp_sint16_t)sFloatValue;

    *(acp_float_t *)aAppBuffer->mBuffer = (acp_float_t)sSmallIntValue;

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

    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);
    aLength->mWritten = ACI_SIZEOF(acp_float_t);

    return ACI_SUCCESS;
}

ACI_RC ulncINTEGER_FLOAT(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    acp_sint32_t sSInt32Value;
    acp_float_t  sFloatValue;
    acp_sint32_t sSInt32Value2;

    sSInt32Value  = *(acp_sint32_t *)aColumn->mBuffer;
    sFloatValue   = sSInt32Value;
    sSInt32Value2 = (acp_sint32_t)sFloatValue;

    *(acp_float_t *)aAppBuffer->mBuffer = sFloatValue;

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

    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);
    aLength->mWritten = ACI_SIZEOF(acp_float_t);

    return ACI_SUCCESS;
}

ACI_RC ulncBIGINT_FLOAT(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_sint64_t sBigIntValue;
    acp_sint64_t sBigIntValue2;
    acp_float_t  sFloatValue;

    sBigIntValue  = *(acp_sint64_t *)aColumn->mBuffer;
    sFloatValue   = (acp_float_t)sBigIntValue;
    sBigIntValue2 = (acp_sint64_t)sFloatValue;

    *(acp_float_t *)aAppBuffer->mBuffer = (acp_float_t)sBigIntValue;

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

    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);
    aLength->mWritten = ACI_SIZEOF(acp_float_t);

    return ACI_SUCCESS;
}

ACI_RC ulncREAL_FLOAT(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_float_t sFloatValue;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sFloatValue = *(acp_float_t *)aColumn->mBuffer;

    *(acp_float_t *)aAppBuffer->mBuffer = sFloatValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);
    aLength->mWritten = ACI_SIZEOF(acp_float_t);

    return ACI_SUCCESS;
}

ACI_RC ulncDOUBLE_FLOAT(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_double_t sDoubleValue;
    acp_char_t   sDoubleString[128];
    acp_char_t   sFloatString[128];


    sDoubleValue = *(acp_double_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sDoubleValue < -(10e38), LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sDoubleValue > (10e38), LABEL_OUT_OF_RANGE);

    *(acp_float_t *)aAppBuffer->mBuffer = (acp_float_t)sDoubleValue;

    acpSnprintf(sDoubleString, 128, "%"ACI_DOUBLE_G_FMT, sDoubleValue);
    acpSnprintf(sFloatString,  128, "%"ACI_FLOAT_G_FMT, (acp_float_t)sDoubleValue);

    if(acpCStrCmp(sDoubleString, sFloatString, 128) != 0)
    // if(sDoubleValue != (acp_float_t)sDoubleValue)
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

    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);
    aLength->mWritten = ACI_SIZEOF(acp_float_t);

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

ACI_RC ulncINTERVAL_FLOAT(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    // BUG-21348
    // INTERVAL 타입을 DOUBLE, FLOAT로 컨버전하는 함수가 구현되어 있지 않았음
    cmtInterval  *sCmInterval;
    acp_double_t  sDoubleValue;

    sCmInterval = (cmtInterval *)aColumn->mBuffer;

    sDoubleValue = ((acp_double_t)sCmInterval->mSecond / (3600 * 24)) +
                   (((acp_double_t)sCmInterval->mMicroSecond * 0.000001) / (3600 * 24));

    ACI_TEST_RAISE(sDoubleValue < -(10e38), LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sDoubleValue > (10e38), LABEL_OUT_OF_RANGE);

    *(acp_float_t *)aAppBuffer->mBuffer = (acp_float_t)sDoubleValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);
    aLength->mWritten = ACI_SIZEOF(acp_float_t);

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

ACI_RC ulncNUMERIC_FLOAT(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    cmtNumeric  *sCmNumeric;
    acp_float_t  sFloatValue = 0;

    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aRowNumber);

    sCmNumeric = (cmtNumeric *)aColumn->mBuffer;

    sFloatValue = ulncCmtNumericToFloat(sCmNumeric);

    *(acp_float_t *)aAppBuffer->mBuffer = (acp_float_t)sFloatValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);
    aLength->mWritten = ACI_SIZEOF(acp_float_t);

    return ACI_SUCCESS;
}

ACI_RC ulncNCHAR_FLOAT(ulnFnContext  *aFnContext,
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

    errno = 0;
    ACI_TEST_RAISE(ACP_RC_IS_ERANGE(
                       acpCStrToDouble((const acp_char_t *)ulnCharSetGetConvertedText(&sCharSet),
                                       ulnCharSetGetConvertedTextLen(&sCharSet),
                                       &sDoubleValue,
                                       NULL)),
                   LABEL_OUT_OF_RANGE);

    ACI_TEST_RAISE(sDoubleValue < -(10e38), LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sDoubleValue > (10e38), LABEL_OUT_OF_RANGE);

    /*
     * BUGBUG : FRACTIONAL TRUNCATION 을 디텍트하는 루틴을 만들어 넣어야 한다.
     */
    // *(acp_float_t *)aAppBuffer->mBuffer = sFloatValue;
    *(acp_float_t *)aAppBuffer->mBuffer = sDoubleValue;

#if 0
    acpSnprintf(sDoubleString, 128, "%"ACI_DOUBLE_G_FMT, sDoubleValue);
    acpSnprintf(sFloatString,  128, "%"ACI_FLOAT_G_FMT, sFloatValue);

    if(acpCStrCmp(sDoubleString, sFloatString, 128) != 0)
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
#endif

    aLength->mWritten = ACI_SIZEOF(acp_float_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_float_t);

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

ACI_RC ulncNVARCHAR_FLOAT(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    return ulncNCHAR_FLOAT(aFnContext,
                           aAppBuffer,
                           aColumn,
                           aLength,
                           aRowNumber);
}
