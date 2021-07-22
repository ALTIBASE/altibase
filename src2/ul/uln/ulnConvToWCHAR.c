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
#include <ulnConvNumeric.h>
#include <ulnConvBit.h>
#include <ulnConvChar.h>
#include <ulnConvWChar.h>

#include <ulnConvToWCHAR.h>

static ACI_RC ulnConvWCharNullTerminate(ulnFnContext *aFnContext,
                                        ulnAppBuffer *aAppBuffer,
                                        acp_sint32_t  aSourceSize,
                                        acp_uint16_t  aColumnNumber,
                                        acp_uint16_t  aRowNumber)
{
    /*
     * Note : SQL_C_WCHAR 로 변환시 null terminate 시켜 주는 함수.
     *        만약, 소스 사이즈가 SQL_NULL_DATA 이면 그냥 성공을 리턴하고, is truncated 는
     *        ID_FALSE 로 세팅한다.
     */

    acp_sint32_t sLengthNeeded;
    acp_uint32_t sOffset;

    /*
     * SourceSize 는 복사할 데이터가 있는 버퍼의 크기 되겠다.
     *
     * Note : aSourceSize 가 0 인 경우도 있다.
     *        LOB 이 NULL 일 경우에는 cmtNull 이 오는것이 아니고, 사이즈 0 인 lob 이 오는데,
     *        이 경우가 되겠다.
     *
     *        게다가, aSourceSize 는 그다지, 체크를 안해도 되겠다.
     *        단지, 음수인 것만 체크하자.
     */

    // BUG-27515: SQL_NO_TOTAL 처리
    if (aSourceSize == SQL_NO_TOTAL)
    {
        sLengthNeeded = SQL_NO_TOTAL;
    }
    else
    {
        /*
         * Note : SQL_NULL_DATA 일 때에는 NULL conversion 으로 가지 이곳으로 안온다.
         */
        ACI_TEST_RAISE(aSourceSize < 0, LABEL_INVALID_DATA_SIZE);

        sLengthNeeded = aSourceSize + ACI_SIZEOF(ulWChar);
    }

    /*
     * GetData() 를 할 때 버퍼 사이즈에 0 을 주기도 한다. -_-;
     */
    if (aAppBuffer->mBufferSize > ULN_vULEN(0))
    {
        ACI_TEST( aAppBuffer->mBuffer == NULL );    //BUG-28561 [CodeSonar] Null Pointer Dereference

        // BUG-27515: SQL_NO_TOTAL 처리
        if ((sLengthNeeded == SQL_NO_TOTAL) || ((acp_uint32_t)sLengthNeeded > aAppBuffer->mBufferSize))
        {
            /* 
             * PROJ-2047 Strengthening LOB - Partial Converting
             *
             * WCHAR에서 Null Char를 처리할 때는 버퍼 사이즈가 짝수여야 한다.
             * 버퍼 사이즈가 홀수인 경우에는 BUG-28110 정책을 따라 버퍼 마지막 바이트를 무시한다.
             * rx6600 장비에서 BUS Error가 발생해 수정.
             */
            if (aAppBuffer->mBufferSize % 2 == 1)
            {
                sOffset = aAppBuffer->mBufferSize - ACI_SIZEOF(ulWChar) - 1;
            }
            else
            {
                sOffset = aAppBuffer->mBufferSize - ACI_SIZEOF(ulWChar);
            }

            *(ulWChar*)(aAppBuffer->mBuffer + sOffset) = '\0';

            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aColumnNumber,
                             ulERR_IGNORE_RIGHT_TRUNCATED);

            aAppBuffer->mColumnStatus = ULN_ROW_SUCCESS_WITH_INFO;
        }
        else
        {
            /* 
             * PROJ-2047 Strengthening LOB - Partial Converting
             *
             * WCHAR에서 Null Char를 처리할 때는 버퍼 사이즈가 짝수여야 한다.
             * 버퍼 사이즈가 홀수인 경우에는 BUG-28110 정책을 따라 버퍼 마지막 바이트를 무시한다.
             * rx6600 장비에서 BUS Error가 발생해 수정.
             */
            if (sLengthNeeded % 2 == 1)
            {
                sOffset = sLengthNeeded - ACI_SIZEOF(ulWChar) - 1;
            }
            else
            {
                sOffset = sLengthNeeded - ACI_SIZEOF(ulWChar);
            }

            *(ulWChar*)(aAppBuffer->mBuffer + sOffset) = '\0';
        }
    }
    else
    {
        /*
         * sLengthNeeded 는 0 일 수 없다. 즉 이곳은
         * sLengthNeeded > 0 and aAppBuffer->mBufferSize == 0
         * 인 경우이다.
         * 주저할 것 없이 01004 발생
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumnNumber,
                         ulERR_IGNORE_RIGHT_TRUNCATED);

        aAppBuffer->mColumnStatus = ULN_ROW_SUCCESS_WITH_INFO;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_DATA_SIZE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumnNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "Data size incorrect. Possible memory corruption");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ulWChar ulnConvNibbleToWCharTable[16] =
{
    0x0030 /*'0'*/,
    0x0031 /*'1'*/,
    0x0032 /*'2'*/,
    0x0033 /*'3'*/,
    0x0034 /*'4'*/,
    0x0035 /*'5'*/,
    0x0036 /*'6'*/,
    0x0037 /*'7'*/,
    0x0038 /*'8'*/,
    0x0039 /*'9'*/,
    0x0041 /*'A'*/,
    0x0042 /*'B'*/,
    0x0043 /*'C'*/,
    0x0044 /*'D'*/,
    0x0045 /*'E'*/,
    0x0046 /*'F'*/
};

ACI_RC ulncCHAR_WCHAR(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    ulnDbc* sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    aLength->mNeeded = aColumn->mDataLength - aColumn->mGDPosition;

    ACI_TEST(ulnConvCopyStr(aFnContext,
                            sDbc->mCharsetLangModule,
                            sDbc->mWcharCharsetLangModule,
                            aAppBuffer,
                            aColumn,
                            (acp_char_t *)aColumn->mBuffer,
                            aColumn->mDataLength,
                            aLength) != ACI_SUCCESS);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);
    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncVARCHAR_WCHAR(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    return ulncCHAR_WCHAR(aFnContext,
                          aAppBuffer,
                          aColumn,
                          aLength,
                          aRowNumber);
}

ACI_RC ulncNUMERIC_WCHAR(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    ulncNumeric sDecimal;
    acp_uint8_t sNumericBuffer[ULNC_DECIMAL_ALLOCSIZE];

    ulncNumericInitialize(&sDecimal, 10, ULNC_ENDIAN_BIG, sNumericBuffer, ULNC_DECIMAL_ALLOCSIZE);
    ulncCmtNumericToDecimal((cmtNumeric *)aColumn->mBuffer, &sDecimal);

    ulncDecimalPrintW(&sDecimal,
                     (ulWChar *)aAppBuffer->mBuffer,
                     aAppBuffer->mBufferSize / 2,
                     aColumn->mGDPosition,
                     aLength);

    if (aLength->mWritten > 0)
    {
		aColumn->mGDPosition += (aLength->mWritten / ACI_SIZEOF(ulWChar));
    }

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncVARBIT_WCHAR(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_uint32_t i;
    acp_uint32_t j;
    acp_uint32_t sStartingBit     = 0;
    acp_uint32_t sWritingPosition = 0;
    acp_uint32_t sBitLength;
    acp_uint32_t sFence;
    acp_uint8_t  sBitValue = 0;

    // BUG-23630 VARCHAR가 WCHAR로 컨버젼이 안됩니다.
    // 컨버젼 함수를 사용하지 않기 때문에 알아서 엔디안을 고려해야 한다.
#ifndef ENDIAN_IS_BIG_ENDIAN
    ulWChar sOneValue   = 0x0031; /*'1'*/
    ulWChar sZeroValue  = 0x0030; /*'0'*/
#else
    ulWChar sOneValue   = 0x3100; /*'1'*/
    ulWChar sZeroValue  = 0x3000; /*'0'*/
#endif

    /*
     * VARBIT 의 경우 GDPosition 의 단위는 비트단위로 한다.
     * 즉, 0x34 0xAB 의 VARBIT 데이터에서 GDPosition 이 11 이라면,
     *
     *      11번 인덱스의 비트 -------+
     *                                |
     *                                V
     *      0 0 1 1   0 1 0 0   1 0 1 0   1 0 1 1
     *
     * 위의 그림과 같은 부분부터 읽기를 시작해야 한다.
     *
     * 이는 2번째 바이트의 3번 비트이며, sStartingBit 의 값은 3 으로 세팅된다.
     */

    sBitLength    = aColumn->mDataLength - aColumn->mGDPosition / 8;

    // BUG-23630 VARCHAR가 WCHAR로 컨버젼이 안됩니다.
    // Precision *2 만큼만 사용자 버퍼에 써야 한다.
    sFence  = (aColumn->mPrecision * ACI_SIZEOF(ulWChar));

    // mNeeded 는 사용자가 모든 데이타를 담기에 충분한 버퍼의 크기를 말한다.
    aLength->mNeeded = (aColumn->mPrecision - aColumn->mGDPosition) * ACI_SIZEOF(ulWChar);

    if (aAppBuffer->mBuffer == NULL)
    {
        aLength->mWritten = 0;
    }
    else
    {
        sStartingBit = aColumn->mGDPosition % 8;

        for (i = 0; i < sBitLength; i++)
        {
            sBitValue = *(aColumn->mBuffer + (aColumn->mGDPosition / 8) + i);

            for (j = sStartingBit; j < 8; j++)
            {
                // BUG-23630 VARCHAR가 WCHAR로 컨버젼이 안됩니다.
                // 2byte씩 쓰므로 다음 바이트로 체크해야 한다.
                if ((sWritingPosition + 1) < sFence)
                {
                    if ((sWritingPosition + 1) < aAppBuffer->mBufferSize)
                    {
                        if (sBitValue & 0x01 << (7 - j))
                        {
                            *(ulWChar*)(aAppBuffer->mBuffer + sWritingPosition) = sOneValue;
                        }
                        else
                        {
                            *(ulWChar*)(aAppBuffer->mBuffer + sWritingPosition) = sZeroValue;
                        }
                        sWritingPosition = sWritingPosition + ACI_SIZEOF(ulWChar);
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            sStartingBit = 0;
        }

        aLength->mWritten = sWritingPosition;
    }

    if (sWritingPosition > 0)
    {
        aColumn->mGDPosition += (sWritingPosition / ACI_SIZEOF(ulWChar)) - 1;
    }

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncBIT_WCHAR(ulnFnContext  *aFnContext,
                     ulnAppBuffer  *aAppBuffer,
                     ulnColumn     *aColumn,
                     ulnLengthPair *aLength,
                     acp_uint16_t   aRowNumber)
{
    acp_uint8_t sBitValue = 0;

    sBitValue = ulnConvBitToUChar(aColumn->mBuffer);

    if (aAppBuffer->mBuffer == NULL)
    {
        aLength->mWritten = 0;
    }
    else
    {
        if (aAppBuffer->mBufferSize > 0)
        {
            *(ulWChar*)(aAppBuffer->mBuffer) = sBitValue + 0x0030 /*'0'*/;
            aLength->mWritten = ACI_SIZEOF(ulWChar);
        }
        else
        {
            aLength->mWritten = 0;
        }
    }

    aLength->mNeeded = ACI_SIZEOF(ulWChar);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncSMALLINT_WCHAR(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    ulnDbc     *sDbc;
    acp_char_t  sTempBuffer[ULN_TRACE_LOG_MAX_DATA_LEN];
    acp_char_t *sValue;
    acp_size_t  sLen;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sValue = ulnItoA( *(acp_sint16_t *)aColumn->mBuffer, sTempBuffer );
    sLen   = acpCStrLen( sValue, ACI_SIZEOF(sTempBuffer));

    acpMemMove( sTempBuffer, sValue, sLen+1);

    ACI_TEST(ulnConvCopyStr(aFnContext,
                            sDbc->mCharsetLangModule,
                            sDbc->mWcharCharsetLangModule,
                            aAppBuffer,
                            aColumn,
                            sTempBuffer,
                            acpCStrLen(sTempBuffer, ACI_SIZEOF(sTempBuffer)),
                            aLength) != ACI_SUCCESS);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncINTEGER_WCHAR(ulnFnContext  *aFnContext,
                         ulnAppBuffer  *aAppBuffer,
                         ulnColumn     *aColumn,
                         ulnLengthPair *aLength,
                         acp_uint16_t   aRowNumber)
{
    ulnDbc     *sDbc;
    acp_char_t  sTempBuffer[ULN_TRACE_LOG_MAX_DATA_LEN];
    acp_char_t *sValue;
    acp_size_t  sLen;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sValue = ulnItoA( *(acp_sint32_t *)aColumn->mBuffer, sTempBuffer );
    sLen   = acpCStrLen( sValue, ACI_SIZEOF(sTempBuffer));

    acpMemMove( sTempBuffer, sValue, sLen+1);

    ACI_TEST(ulnConvCopyStr(aFnContext,
                            sDbc->mCharsetLangModule,
                            sDbc->mWcharCharsetLangModule,
                            aAppBuffer,
                            aColumn,
                            sTempBuffer,
                            acpCStrLen(sTempBuffer, ACI_SIZEOF(sTempBuffer)),
                            aLength) != ACI_SUCCESS);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncBIGINT_WCHAR(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    ulnDbc     *sDbc;
    acp_char_t  sTempBuffer[ULN_TRACE_LOG_MAX_DATA_LEN];
    acp_char_t *sValue;
    acp_size_t  sLen;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sValue = ulnItoA( *(acp_sint64_t *)aColumn->mBuffer, sTempBuffer );
    sLen   = acpCStrLen( sValue, ACI_SIZEOF(sTempBuffer));

    acpMemMove( sTempBuffer, sValue, sLen+1);

    ACI_TEST(ulnConvCopyStr(aFnContext,
                            sDbc->mCharsetLangModule,
                            sDbc->mWcharCharsetLangModule,
                            aAppBuffer,
                            aColumn,
                            sTempBuffer,
                            acpCStrLen(sTempBuffer, ACI_SIZEOF(sTempBuffer)),
                            aLength) != ACI_SUCCESS);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncREAL_WCHAR(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    ulnDbc     *sDbc;
    acp_char_t  sTempBuffer[50];

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    acpSnprintf(sTempBuffer,
                ACI_SIZEOF(sTempBuffer),
                "%"ACI_FLOAT_G_FMT,
                *(acp_float_t *)aColumn->mBuffer);

    ACI_TEST(ulnConvCopyStr(aFnContext,
                            sDbc->mCharsetLangModule,
                            sDbc->mWcharCharsetLangModule,
                            aAppBuffer,
                            aColumn,
                            sTempBuffer,
                            acpCStrLen(sTempBuffer, ACI_SIZEOF(sTempBuffer)),
                            aLength) != ACI_SUCCESS);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncDOUBLE_WCHAR(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    ulnDbc     *sDbc;
    acp_char_t  sTempBuffer[50];

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    ACI_TEST( sDbc == NULL );     //BUG-28561 [CodeSonar] Null Pointer Dereference

    acpSnprintf(sTempBuffer,
                ACI_SIZEOF(sTempBuffer),
                "%"ACI_DOUBLE_G_FMT,
                *(acp_double_t *)aColumn->mBuffer);

    ACI_TEST(ulnConvCopyStr(aFnContext,
                            sDbc->mCharsetLangModule,
                            sDbc->mWcharCharsetLangModule,
                            aAppBuffer,
                            aColumn,
                            sTempBuffer,
                            acpCStrLen(sTempBuffer, ACI_SIZEOF(sTempBuffer)),
                            aLength) != ACI_SUCCESS);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncBINARY_WCHAR(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    ACP_UNUSED(aAppBuffer);
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aLength);
    ACP_UNUSED(aRowNumber);

    ulnError(aFnContext,
             ulERR_ABORT_INVALID_APP_BUFFER_TYPE,
             SQL_C_WCHAR);

    return ACI_FAILURE;
}

ACI_RC ulncNIBBLE_WCHAR(ulnFnContext  *aFnContext,
                        ulnAppBuffer  *aAppBuffer,
                        ulnColumn     *aColumn,
                        ulnLengthPair *aLength,
                        acp_uint16_t   aRowNumber)
{
    acp_uint32_t i;
    acp_uint8_t  sNibbleData;
    acp_uint8_t  sNibbleLength;
    acp_uint32_t sPosition = 0;
    acp_uint32_t sStartingPosition;

    /*
     * NIBBLE 의 경우 GDPosition 의 단위는 니블단위로 한다.
     * 즉, 0x34 0xAB 0x1D 0xF 의 NIBBLE 데이터에서 GDPosition 이 5 라면,
     *
     *      5번 인덱스의 니블 --+
     *                          |
     *                          V
     *              3 4  A B  1 D  F
     *
     * 위의 그림과 같은 부분부터 읽기를 시작해야 한다.
     *
     * 이는 3번째 바이트의 하위 니블이며, sStartingBit 의 값은 3 으로 세팅된다.
     */

    sNibbleLength     = aColumn->mPrecision;
    aLength->mNeeded  = (sNibbleLength - aColumn->mGDPosition) * ACI_SIZEOF(ulWChar);

    if (aAppBuffer->mBuffer == NULL)
    {
        aLength->mWritten = 0;
    }
    else
    {
        /*
         * 시작으로 읽을 차례의 니블의 니블인덱스
         */

        sStartingPosition = aColumn->mGDPosition;

        /*
         * 변환 시작
         */

        for (i = sStartingPosition; i < sNibbleLength; i++)
        {
            if((i & 0x01) == 0x01)
            {
                sNibbleData = aColumn->mBuffer[i / 2] & 0x0f;
            }
            else
            {
                sNibbleData = aColumn->mBuffer[i / 2] >> 4;
            }

            if (sPosition < aAppBuffer->mBufferSize - 1)
            {
                *(ulWChar*)(aAppBuffer->mBuffer + sPosition) = ulnConvNibbleToWCharTable[sNibbleData];
                sPosition = sPosition + ACI_SIZEOF(ulWChar);
            }
            else
            {
                break;
            }
        }

        aLength->mWritten = sPosition;
    }

    if (aLength->mWritten > 0)
    {
        aColumn->mGDPosition += (aLength->mWritten / ACI_SIZEOF(ulWChar));
    }

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncBYTE_WCHAR(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_uint32_t sNumberOfBytesWritten = 0;

    aLength->mNeeded  = (aColumn->mDataLength - aColumn->mGDPosition) * ACI_SIZEOF(ulWChar) * 2;

    if (aAppBuffer->mBuffer == NULL)
    {
        sNumberOfBytesWritten = 0;
    }
    else
    {
        sNumberOfBytesWritten = ulnConvDumpAsWChar(aAppBuffer->mBuffer,
                                                   aAppBuffer->mBufferSize,
                                                   aColumn->mBuffer + aColumn->mGDPosition,
                                                   aColumn->mDataLength - aColumn->mGDPosition);

        /*
         * Null termination.
         *
         * ulnConvDumpAsChar() 함수는 안전하게 null 터미네이팅을 할 곳을 리턴한다.
         */

        *(ulWChar*)(aAppBuffer->mBuffer + sNumberOfBytesWritten) = 0;
    }

    aLength->mWritten = sNumberOfBytesWritten;    // null termination 포함 안함.

    if (sNumberOfBytesWritten > 0)
    {
        aColumn->mGDPosition += ((sNumberOfBytesWritten / 2) / ACI_SIZEOF(ulWChar));
    }

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncDATE_WCHAR(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    acp_char_t   *sDateFormat;
    mtdDateType  *sMtdDate;

    acp_char_t    sDateString[128];
    acp_uint32_t  sDateStringLength = 0;
    ulnDbc       *sDbc;
    ulnCharSet    sCharSet;
    acp_sint32_t  sState = 0;

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * Date 를 스트링으로 변환
     */

    sDateFormat = aFnContext->mHandle.mStmt->mParentDbc->mDateFormat;

    sMtdDate = (mtdDateType *)aColumn->mBuffer;

    ACI_TEST_RAISE( mtdDateInterfaceToChar(sMtdDate,
                                           (acp_uint8_t *)sDateString,
                                           &sDateStringLength,
                                           ACI_SIZEOF(sDateString),
                                           (acp_uint8_t *)sDateFormat,
                                           acpCStrLen(sDateFormat, ACP_SINT32_MAX))
                    != ACI_SUCCESS, INVALID_DATETIME_FORMAT_ERROR );

    ACI_TEST(ulnCharSetConvert(&sCharSet,
                               aFnContext,
                               NULL,
                               sDbc->mClientCharsetLangModule,    //BUG-22684
                               (const mtlModule*)sDbc->mWcharCharsetLangModule,
                               (void*)sDateString,
                               sDateStringLength,
                               CONV_DATA_OUT)
             != ACI_SUCCESS);

    aLength->mNeeded  = ulnCharSetGetConvertedTextLen(&sCharSet) -
                       (aColumn->mGDPosition * ACI_SIZEOF(ulWChar));

    /*
     * 사용자 버퍼에 복사
     */

    aLength->mWritten = ulnConvCopy(aAppBuffer->mBuffer,
                                    aAppBuffer->mBufferSize,
                                    ulnCharSetGetConvertedText(&sCharSet) + (aColumn->mGDPosition * ACI_SIZEOF(ulWChar)),
                                    ulnCharSetGetConvertedTextLen(&sCharSet) - (aColumn->mGDPosition * ACI_SIZEOF(ulWChar)));

    if (aLength->mWritten > 0)
    {
        aColumn->mGDPosition += (aLength->mWritten / ACI_SIZEOF(ulWChar)) - 1;
    }

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION( INVALID_DATETIME_FORMAT_ERROR )
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_ABORT_Invalid_Datatime_Format_Error);
    }

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return ACI_FAILURE;
}

ACI_RC ulncINTERVAL_WCHAR(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    /*
     * BUGBUG : ODBC 에 interval literal 에 대한 문법이 있음에도 불구하고
     *          발생할 수많은 diff 로 인해 Double 로 찍어서 준다.
     *
     *          다음번에 표준대로 하도록 하자.
     */
    cmtInterval  *sCmInterval;
    acp_double_t  sDoubleValue;
    acp_char_t    sTempBuffer[50];
    ulnDbc       *sDbc;
    ulnCharSet    sCharSet;
    acp_sint32_t  sState = 0;

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    sCmInterval = (cmtInterval *)aColumn->mBuffer;

    sDoubleValue = ((acp_double_t)sCmInterval->mSecond / (3600 * 24)) +
                   (((acp_double_t)sCmInterval->mMicroSecond * 0.000001) / (3600 * 24));

    acpSnprintf(sTempBuffer, ACI_SIZEOF(sTempBuffer), "%"ACI_DOUBLE_G_FMT, sDoubleValue);

    ACI_TEST(ulnCharSetConvert(&sCharSet,
                               aFnContext,
                               NULL,
                               sDbc->mClientCharsetLangModule,    //BUG-22684
                               (const mtlModule*)sDbc->mWcharCharsetLangModule,
                               (void*)sTempBuffer,
                               acpCStrLen(sTempBuffer, ACI_SIZEOF(sTempBuffer)),
                               CONV_DATA_OUT)
             != ACI_SUCCESS);

    aLength->mWritten = ulnConvCopy(aAppBuffer->mBuffer,
                                    aAppBuffer->mBufferSize,
                                    ulnCharSetGetConvertedText(&sCharSet) + (aColumn->mGDPosition * ACI_SIZEOF(ulWChar)),
                                    ulnCharSetGetConvertedTextLen(&sCharSet) - (aColumn->mGDPosition * ACI_SIZEOF(ulWChar)));


    aLength->mNeeded  = ulnCharSetGetConvertedTextLen(&sCharSet) -
                       (aColumn->mGDPosition * ACI_SIZEOF(ulWChar));

    if (aLength->mWritten > 0)
    {
        aColumn->mGDPosition += (aLength->mWritten / ACI_SIZEOF(ulWChar)) - 1;
    }

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return ACI_FAILURE;
}

ACI_RC ulncBLOB_WCHAR(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    ulnPtContext *sPtContext  = (ulnPtContext *)aFnContext->mArgs;

    ulnLob       *sLob        = (ulnLob *)aColumn->mBuffer;
    ulnLobBuffer  sLobBuffer;
    acp_uint32_t  sSizeToRequest;

    /*
     * ulnLobBuffer 준비
     */

    ACI_TEST_RAISE(ulnLobBufferInitialize(&sLobBuffer,
                                          NULL,
                                          sLob->mType,
                                          ULN_CTYPE_WCHAR,
                                          aAppBuffer->mBuffer,
                                          aAppBuffer->mBufferSize) != ACI_SUCCESS,
                   LABEL_MEM_MAN_ERR);

    ACI_TEST(sLobBuffer.mOp->mPrepare(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * open LOB 및 get data
     */

    ACI_TEST(sLob->mOp->mOpen(aFnContext, sPtContext, sLob) != ACI_SUCCESS);

    /*
     * ulnLobGetData() 함수가 호출됨.
     *      이와같이 blob --> char 의 경우에는 ulnLobBufferDataInCHAR() 함수임.
     *
     * CHAR 로 변환을 할 것이기 때문에 두배의 버퍼가 필요하다.
     * 어차피 사용자에게 넘겨주지 못할 데이터를 다 받을 필요 없고,
     * 넘겨줄 수 있는 만큼만 서버에 요청하자.
     *
     * Note : BINARY --> CHAR 의 경우 아래의 규칙이 적용된다 :
     *
     *          1. 사용자 버퍼가 충분히 클 경우 (8 bytes)
     *              binary data    : 0x1B 0x2D 0x3F
     *              converted char : 1B2D3F\0
     *
     *          2. 사용자 버퍼가 작을 경우 (6 bytes)
     *              binary data    : 0x1B 0x2D 0x3F
     *              converted char : 1B2D\0   : 쓰여진 데이터 길이 : 5 bytes
     *                                          사용자 버퍼의 마지막 바이트 : 아무것도 안 씀.
     *
     *          3. 사용자 버퍼가 작을 경우 (5 bytes)
     *              binary data    : 0x1B 0x2D 0x3F
     *              converted char : 1B2D\0   : 쓰여진 데이터 길이 : 5 bytes
     *
     *        이처럼 사용자 버퍼가 작아서 null terminate 를 해야 하는 경우에,
     *        맨 마지막 16진수 한바이트가 잘리고 상위 니블만 나오게 될 경우에는
     *        두바이트를 다 잘라버리고 널 터미네이트 시켜야 한다.
     */

    if (aAppBuffer->mBuffer == NULL)
    {
        aLength->mWritten = 0;
    }
    else
    {
        sSizeToRequest = (aAppBuffer->mBufferSize == 0) ? 0 : ((aAppBuffer->mBufferSize / ACI_SIZEOF(ulWChar)) - 1) / 2;

        sSizeToRequest = ACP_MIN(sSizeToRequest, sLob->mSize - aColumn->mGDPosition);

        ACI_TEST(sLob->mOp->mGetData(aFnContext,
                                     sPtContext,
                                     sLob,
                                     &sLobBuffer,
                                     aColumn->mGDPosition,
                                     sSizeToRequest) != ACI_SUCCESS);

        /*
         * null terminating
         *
         * ulnConvDumpAsChar 가 리턴하는 것은 언제나 짝수이며,
         * 항상 null termination 을 위한 여지를 남겨 둔 길이를 리턴한다.
         *
         * 즉,
         *
         *      *(destination buffer + sLengthConverted) = 0;
         *
         * 과 같이 널 터미네이션을 안전하게 할 수 있다.
         *
         * 그리고, sLob 의 mSizeRetrieved 의 단위는 실제로 서버에서 수신된 데이터의
         * 바이트단위 길이이며, ulnConvDumpAsChar() 가 리턴한 값을 2 로 나눈 값이다.
         */

        *(ulWChar*)(sLobBuffer.mObject.mMemory.mBuffer + (sLob->mSizeRetrieved * ACI_SIZEOF(ulWChar) * 2)) = 0;

        /*
         * Note : Null Termination 길이를 더해주거나 하면 안된다.
         *        그것은 아래의 null terminate 함수에서 일괄적으로 처리한다.
         */
        aLength->mWritten = sLob->mSizeRetrieved * ACI_SIZEOF(ulWChar) * 2;
    }

    aLength->mNeeded  = (sLob->mSize * 2 - aColumn->mGDPosition * 2) * ACI_SIZEOF(ulWChar);

    if (sLob->mSizeRetrieved > 0)
    {
        aColumn->mGDPosition += sLob->mSizeRetrieved;
    }

    /*
     * ulnLobBuffer 정리
     */

    ACI_TEST(sLobBuffer.mOp->mFinalize(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * close LOB :
     *      1. scrollable 커서일 때는 커서가 닫힐 때.
     *         ulnCursorClose() 함수에서
     *      2. forward only 일 때에는 캐쉬 미스가 발생했을 때.
     *         ulnFetchFromCache() 함수에서
     *
     *      ulnCacheCloseLobInCurrentContents() 를 호출해서 종료시킴.
     */

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aColumn->mColumnNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulncBLOB_CHAR");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* 
 * PROJ-2047 Strengthening LOB - Partial Converting
 */
ACI_RC ulncCLOB_WCHAR(ulnFnContext  *aFnContext,
                      ulnAppBuffer  *aAppBuffer,
                      ulnColumn     *aColumn,
                      ulnLengthPair *aLength,
                      acp_uint16_t   aRowNumber)
{
    ulnPtContext *sPtContext  = (ulnPtContext *)aFnContext->mArgs;
    ulnDbc       *sDbc;
    ulnLob       *sLob        = (ulnLob *)aColumn->mBuffer;
    ulnLobBuffer  sLobBuffer;
    acp_uint32_t  sSizeToRequest = 0;

    ulnCharSet   *sCharSet = NULL;
    acp_uint32_t  sHeadSize = 0;
    acp_uint32_t  sRemainAppBufferSize = 0;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    if (aAppBuffer->mBuffer == NULL ||
        aAppBuffer->mBufferSize <= ACI_SIZEOF(ulWChar))
    {
        sRemainAppBufferSize = 0;
    }
    else
    {
        sRemainAppBufferSize = aAppBuffer->mBufferSize - ACI_SIZEOF(ulWChar);

        if (sRemainAppBufferSize % ACI_SIZEOF(ulWChar) == 1)
        {
            sRemainAppBufferSize -= 1;
        }
        else
        {
            /* Nothing */
        }
    }

    if (aColumn->mGDPosition == 0)
    {
        aColumn->mRemainTextLen = 0;
        ACI_RAISE(REQUEST_TO_SERVER);
    }
    else
    {
        /* Nothing */
    }

    /* 이전에 주지 못했던 데이터를 복사해 주자 */
    if (aColumn->mRemainTextLen > 0) 
    {
        sHeadSize = ACP_MIN(sRemainAppBufferSize, aColumn->mRemainTextLen);
        acpMemCpy(aAppBuffer->mBuffer, aColumn->mRemainText, sHeadSize);

        aColumn->mRemainTextLen -= sHeadSize;
        sRemainAppBufferSize    -= sHeadSize;
 
        /*
         * 사용자 버퍼에 이전에 남은 데이터를 넣을 공간이 부족하거나
         * 사용자 버퍼가 꽉 찼다면 서버에 데이터를 요청할 필요가 없다. 
         */
        if (aColumn->mRemainTextLen > 0)
        {
            /* aAppBuffer->mBuffer에 복사된 내용을 제거하자 */
            acpMemCpy(aColumn->mRemainText,
                      aColumn->mRemainText + sHeadSize,
                      aColumn->mRemainTextLen);
        }
        else
        {
            /* Nothing */
        }
    }
    else
    {
        /* Nothing */
    }

    ACI_EXCEPTION_CONT(REQUEST_TO_SERVER);

    // BUG-27515: 사용자 버퍼를 모두 채울 수 있을 정도로 충분히 얻어온다.
    sSizeToRequest = sRemainAppBufferSize *
                     sDbc->mCharsetLangModule->maxPrecision(1);

    sSizeToRequest = ACP_MIN(sSizeToRequest, sLob->mSize - aColumn->mGDPosition);

    /*
     * ulnLobBuffer 준비
     */
    ACI_TEST(ulnLobBufferInitialize(&sLobBuffer,
                                    NULL,
                                    sLob->mType,
                                    ULN_CTYPE_WCHAR,
                                    aAppBuffer->mBuffer + sHeadSize,
                                    sRemainAppBufferSize) != ACI_SUCCESS);

    ACI_TEST(sLobBuffer.mOp->mPrepare(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * open LOB 및 get data
     */
    ACI_TEST(sLob->mOp->mOpen(aFnContext, sPtContext, sLob) != ACI_SUCCESS);

    ACI_TEST(sLob->mOp->mGetData(aFnContext,
                                 sPtContext,
                                 sLob,
                                 &sLobBuffer,
                                 aColumn->mGDPosition,
                                 sSizeToRequest) != ACI_SUCCESS);

    sCharSet = &sLobBuffer.mCharSet;

    /*
     * sCharSet->mRemainTextLen, aColumn->mRemainTextLen 둘중 하나는 0이다.
     */
    aLength->mNeeded      = sCharSet->mDestLen + sHeadSize +
                            sCharSet->mRemainTextLen + aColumn->mRemainTextLen;
    aLength->mWritten     = sCharSet->mCopiedDesLen + sHeadSize;
    aColumn->mGDPosition += sCharSet->mConvedSrcLen;

    if (sCharSet->mRemainTextLen > 0)
    {
        aColumn->mRemainTextLen = sCharSet->mRemainTextLen;
        acpMemCpy(aColumn->mRemainText,
                  sCharSet->mRemainText,
                  sCharSet->mRemainTextLen);
    }
    else
    {
        /* Nothing */
    }

    /*
     * ulnLobBuffer 정리
     */
    ACI_TEST(sLobBuffer.mOp->mFinalize(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    // BUG-27515: 남은 길이를 정확하게 계산할 수 없으면 SQL_NO_TOTAL 반환
    // 주의: ODBC.NET에서는 처음에 SQL_NO_TOTAL를 받으면 마지막 데이타를
    // 받을때나 유효한 길이를 얻을 수 있다고 여긴다. 그러므로 데이타를
    // 다 받아온게 아니면 올바른 길이를 계산할 수 있더라도 SQL_NO_TOTAL을 줘야한다.
    if ((aColumn->mGDPosition < sLob->mSize) || (aColumn->mRemainTextLen > 0))
    {
        aLength->mNeeded = SQL_NO_TOTAL;
    }

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncNCHAR_WCHAR(ulnFnContext  *aFnContext,
                       ulnAppBuffer  *aAppBuffer,
                       ulnColumn     *aColumn,
                       ulnLengthPair *aLength,
                       acp_uint16_t   aRowNumber)
{
    ulnDbc* sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    aLength->mNeeded = aColumn->mDataLength - aColumn->mGDPosition;

    ACI_TEST(ulnConvCopyStr(aFnContext,
                            sDbc->mNcharCharsetLangModule,
                            sDbc->mWcharCharsetLangModule,
                            aAppBuffer,
                            aColumn,
                            (acp_char_t *)aColumn->mBuffer,
                            aColumn->mDataLength,
                            aLength) != ACI_SUCCESS);

    ACI_TEST(ulnConvWCharNullTerminate(aFnContext,
                                       aAppBuffer,
                                       aLength->mNeeded,
                                       aColumn->mColumnNumber,
                                       aRowNumber) != ACI_SUCCESS);
    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncNVARCHAR_WCHAR(ulnFnContext  *aFnContext,
                          ulnAppBuffer  *aAppBuffer,
                          ulnColumn     *aColumn,
                          ulnLengthPair *aLength,
                          acp_uint16_t   aRowNumber)
{
    return ulncNCHAR_WCHAR(aFnContext,
                           aAppBuffer,
                           aColumn,
                           aLength,
                           aRowNumber);
}
