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
#include <ulnConvToSLONG.h>
#include <ulnConvToULONG.h>
#include <ulnConvNumeric.h>

ACI_RC ulncCHAR_ULONG(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_sint32_t sScale = 0;
    acp_sint64_t sBigIntValue;

    ACI_TEST_RAISE(ulncIsValidNumericLiterals((acp_char_t *)aColumn->mBuffer,
                                              aColumn->mDataLength,
                                              &sScale) != ACP_TRUE,
                   LABEL_INVALID_LITERAL);

    errno = 0;
    sBigIntValue = ulncStrToSLong((const acp_char_t *)aColumn->mBuffer, (acp_char_t **)NULL, 10);
    ACI_TEST_RAISE(errno == ERANGE, LABEL_OUT_OF_RANGE);

    ACI_TEST_RAISE(sBigIntValue < 0, LABEL_OUT_OF_RANGE_MINUS);
    ACI_TEST_RAISE(sBigIntValue > ACP_UINT32_MAX, LABEL_OUT_OF_RANGE);

    if(sScale != 0)
    {
        /*
         * 01s07
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    *(acp_uint32_t *)aAppBuffer->mBuffer = sBigIntValue;

    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LITERAL)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_INVALID_NUMERIC_LITERAL);
    }

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE_MINUS)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
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

ACI_RC ulncVARCHAR_ULONG(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    return ulncCHAR_ULONG(aFnContext,
                          aAppBuffer,
                          aColumn,
                          aLength,
                          aRowNumber);
}

ACI_RC ulncVARBIT_ULONG(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    return ulncBIT_SLONG(aFnContext,
                         aAppBuffer,
                         aColumn,
                         aLength,
                         aRowNumber);
}

ACI_RC ulncBIT_ULONG(ulnFnContext  *aFnContext,
                     ulnAppBuffer  *aAppBuffer,
                     ulnColumn     *aColumn,
                     ulnLengthPair *aLength,
                     acp_uint16_t   aRowNumber)
{
    return ulncBIT_SLONG(aFnContext,
                         aAppBuffer,
                         aColumn,
                         aLength,
                         aRowNumber);
}

ACI_RC ulncSMALLINT_ULONG(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    acp_sint16_t sSmallIntValue;
    acp_uint32_t sIntValue;

    sSmallIntValue = *(acp_sint16_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sSmallIntValue < 0, LABEL_OUT_OF_RANGE);

    sIntValue      = (acp_uint32_t)sSmallIntValue;

//     idlOS::printf("#### sSmallIntValue = %d\n", sSmallIntValue);
//     idlOS::printf("#### sIntValue      = %d\n", sIntValue);

    *(acp_uint32_t *)aAppBuffer->mBuffer = sIntValue;

//     idlOS::printf("#### *aAppBuffer->mBuffer   = %d\n", *(acp_sint32_t *)aAppBuffer->mBuffer);

    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);
    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncINTEGER_ULONG(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    ACI_TEST_RAISE(*(acp_sint32_t *)aColumn->mBuffer < 0, LABEL_OUT_OF_RANGE);

    *(acp_uint32_t *)aAppBuffer->mBuffer = *(acp_uint32_t *)aColumn->mBuffer;

    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);
    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncBIGINT_ULONG(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_sint64_t sBigIntValue;

    sBigIntValue = *(acp_sint64_t *)aColumn->mBuffer;

    // idlOS::printf("##### sBigIntValue = %"ACI_INT64_FMT"\n", sBigIntValue);

    ACI_TEST_RAISE(sBigIntValue < ACP_SINT64_LITERAL(0), LABEL_OUT_OF_RANGE_MINUS);
    ACI_TEST_RAISE(sBigIntValue > ACP_UINT32_MAX, LABEL_OUT_OF_RANGE);

    *(acp_uint32_t *)aAppBuffer->mBuffer = sBigIntValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);
    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE_MINUS)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
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

ACI_RC ulncREAL_ULONG(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_sint64_t sBigIntValue;
    acp_float_t  sFloatValue;

    sFloatValue = *(acp_float_t *)aColumn->mBuffer;

    ACI_TEST_RAISE(sFloatValue < 0, LABEL_OUT_OF_RANGE_MINUS);
    ACI_TEST_RAISE(sFloatValue > ACP_UINT32_MAX, LABEL_OUT_OF_RANGE);

    sBigIntValue = (acp_sint64_t)sFloatValue;

    if(sBigIntValue != sFloatValue)
    {
        /*
         * 01s07
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    *(acp_uint32_t *)aAppBuffer->mBuffer = sBigIntValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);
    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE_MINUS)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
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

ACI_RC ulncDOUBLE_ULONG(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_sint64_t sBigIntValue;
    acp_double_t sDoubleValue;


    sDoubleValue = *(acp_double_t *)aColumn->mBuffer;

    // idlOS::printf("### sDoubleVal = %"ACI_DOUBLE_G_FMT"\n", sDoubleValue);

    ACI_TEST_RAISE(sDoubleValue < 0, LABEL_OUT_OF_RANGE_MINUS);
    ACI_TEST_RAISE(sDoubleValue > ACP_UINT32_MAX, LABEL_OUT_OF_RANGE);

    sBigIntValue = (acp_sint64_t)sDoubleValue;

    // idlOS::printf("### sBigintval = %"ACI_INT64_FMT"\n", sBigIntValue);

    if(sBigIntValue != sDoubleValue)
    {
        /*
         * 01s07
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    *(acp_uint32_t *)aAppBuffer->mBuffer = sBigIntValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);
    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE_MINUS)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
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

ACI_RC ulncINTERVAL_ULONG(ulnFnContext  *aFnContext,
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

ACI_RC ulncNUMERIC_ULONG(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    cmtNumeric  *sCmNumeric;
    ulncNumeric  sDecimal;
    acp_uint8_t  sBuffer[ULNC_NUMERIC_ALLOCSIZE];

    acp_uint64_t sBigIntValue = 0;
    acp_bool_t   sIsOverflow  = ACP_FALSE;

    sCmNumeric = (cmtNumeric *)aColumn->mBuffer;

    ACI_TEST_RAISE(sCmNumeric->mSign == 0, LABEL_OUT_OF_RANGE_MINUS);

    /*
     * BUGBUG : ULong 을 중간 타입으로 쓰는데, 곧장 하도록 수정해야 한다.
     *
     * cmtNumeric 에서 곧장 ULong 으로 가니까 overflow 디텍트가 너무 머리아파서
     * 일단 ulncDecimal 의 10진수로 변환 한 후 ULong 으로 가도록 한다.
     */
    ulncNumericInitialize(&sDecimal, 10, ULNC_ENDIAN_BIG, sBuffer, ULNC_NUMERIC_ALLOCSIZE);
    ulncCmtNumericToDecimal((cmtNumeric *)aColumn->mBuffer, &sDecimal);

    sBigIntValue = ulncDecimalToULong(&sDecimal, &sIsOverflow);

    ACI_TEST_RAISE(sIsOverflow == ACP_TRUE, LABEL_OUT_OF_RANGE);
    ACI_TEST_RAISE(sBigIntValue > ACP_UINT32_MAX, LABEL_OUT_OF_RANGE);

    if (sDecimal.mScale > 0)
    {
        /*
         * 01S07 : fractional truncation
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    *(acp_uint32_t *)aAppBuffer->mBuffer = (acp_uint32_t)sBigIntValue;

    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);
    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);

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

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE_MINUS)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncNCHAR_ULONG(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    acp_sint32_t  sScale = 0;
    acp_sint64_t  sBigIntValue;
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

    errno = 0;
    sBigIntValue = ulncStrToSLong((const acp_char_t *)ulnCharSetGetConvertedText(&sCharSet), (acp_char_t **)NULL, 10);
    ACI_TEST_RAISE(errno == ERANGE, LABEL_OUT_OF_RANGE);

    ACI_TEST_RAISE(sBigIntValue < 0, LABEL_OUT_OF_RANGE_MINUS);
    ACI_TEST_RAISE(sBigIntValue > ACP_UINT32_MAX, LABEL_OUT_OF_RANGE);

    if(sScale != 0)
    {
        /*
         * 01s07
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_IGNORE_FRACTIONAL_TRUNCATION);
    }

    *(acp_uint32_t *)aAppBuffer->mBuffer = sBigIntValue;

    aLength->mWritten = ACI_SIZEOF(acp_uint32_t);
    aLength->mNeeded  = ACI_SIZEOF(acp_uint32_t);

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

    ACI_EXCEPTION(LABEL_OUT_OF_RANGE_MINUS)
    {
        /*
         * 22003
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE_MINUS);
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

ACI_RC ulncNVARCHAR_ULONG(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    return ulncNCHAR_ULONG(aFnContext,
                           aAppBuffer,
                           aColumn,
                           aLength,
                           aRowNumber);
}
