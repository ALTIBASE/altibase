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
 

/*******************************************************************************
 * $Id: ulaConvFmMT.c 82075 2018-01-17 06:39:52Z jina.kim $
 ******************************************************************************/

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <mtccDef.h>
#include <mtcdTypes.h>
#include <mtcdTypeDef.h>

#include <cmAllClient.h>

#include <ulaErrorCode.h>
#include <ulaConvNumeric.h>

#include <ulaConvFmMT.h>

/*
 * -----------------------------------------------------------------------------
 *  ALA_GetODBCCValue() 함수는
 *      ALA_Value 를 입력으로 받아서 사용자가 aODBCCTypeID 에 지정한 타입으로
 *      타입을 변환한 후 그 결과 데이터를
 *      aOutODBCCValueBuffer 가 가리키는 버퍼에 담아 주는 함수이다.
 *
 *      이 때, 타입의 변환은
 *
 *      mt --> cmt --> ulnColumn --> odbc
 *
 *      의 순서로 이루어진다.
 *
 *      그 중 ul 쪽의 변환은 uln 의 함수들을 이용해서 할 수 있으나
 *      PROJ-1000 Client C Porting 당시 서버쪽의 모듈은 C 로 포팅하지 않아서
 *      mt --> cmt 의 변환에 사용된 mmcSession 을 이용할 수 없는 상황이었다.
 *
 *      본 파일 (ulaConv.c) 은 mmcSession 이 수행하던 mt --> cmt 의
 *      변환 코드를 그래도 가져와서 C 로 포팅한 코드이다.
 *
 *      mmcConvFmMT.cpp 파일 참조.
 * -----------------------------------------------------------------------------
 */

//fix BUG-17873
extern mtdModule mtcdBigint;
extern mtdModule mtcdBinary;
extern mtdModule mtcdBit;
extern mtdModule mtcdVarbit;
extern mtdModule mtcdBlob;
extern mtdModule mtcdBoolean;
extern mtdModule mtcdChar;
extern mtdModule mtcdNchar;
extern mtdModule mtcdDate;
extern mtdModule mtcdDouble;
extern mtdModule mtcdFloat;
extern mtdModule mtcdInteger;
extern mtdModule mtcdInterval;
extern mtdModule mtcdList;
extern mtdModule mtcdNull;
extern mtdModule mtcdNumber;
extern mtdModule mtcdNumeric;
extern mtdModule mtcdReal;
extern mtdModule mtcdSmallint;
extern mtdModule mtcdVarchar;
extern mtdModule mtcdNvarchar;
extern mtdModule mtcdByte;
extern mtdModule mtcdNibble;
extern mtdModule mtcdClob;
extern mtdModule mtsFile;
extern mtdModule mtcdBlobLocator;
extern mtdModule mtcdClobLocator;

static ACI_RC ulaConvertMtNull(cmtAny *aTarget, void *aSource)
{
    ACP_UNUSED(aSource);

    ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtBoolean(cmtAny *aTarget, void *aSource)
{
    //fix BUG-17873
    if ((*mtcdBoolean.isNull)(NULL,
                              aSource,
                              MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL,INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteUChar(aTarget,
                                  (*(mtdBooleanType *)aSource
                                   == MTD_BOOLEAN_TRUE) ? 1 : 0)
                 != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtSmallInt(cmtAny *aTarget, void *aSource)
{
    //fix BUG-17873
    if ((*mtcdSmallint.isNull)(NULL,
                               aSource,
                               MTD_OFFSET_USELESS) == ACP_TRUE )
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteSShort(aTarget, *(mtdSmallintType *)aSource)
                 != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtInteger(cmtAny *aTarget, void *aSource)
{
    //fix BUG-17873
    if ((*mtcdInteger.isNull)(NULL,
                              aSource,
                              MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL,INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteSInt(aTarget,
                                 *(mtdIntegerType *)aSource) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtBigInt(cmtAny *aTarget, void *aSource)
{
    //fix BUG-17873
    if ((*mtcdBigint.isNull)(NULL,
                             aSource,
                             MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteSLong(aTarget,
                                  *(mtdBigintType *)aSource) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtBlobLocator(cmtAny      *aTarget,
                                      void        *aSource,
                                      acp_uint32_t aLobSize)
{
    //fix BUG-17873
    if ((*mtcdBlobLocator.isNull)(NULL,
                                  aSource,
                                  MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL,INVALID_SOURCE_DATA);
        // bug-19174
        ACI_TEST(cmtAnyWriteLobLocator(aTarget,
                                       *(acp_uint64_t *)aSource,
                                       aLobSize) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtClobLocator(cmtAny       *aTarget,
                                      void         *aSource,
                                      acp_uint32_t  aLobSize)
{
    //fix BUG-17873
    if ((*mtcdClobLocator.isNull)(NULL,
                                  aSource,
                                  MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        // bug-19174
        ACI_TEST(cmtAnyWriteLobLocator(aTarget,
                                       *(acp_uint64_t *)aSource,
                                       aLobSize) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtReal(cmtAny *aTarget, void *aSource)
{
    //fix BUG-17873
    if ((*mtcdReal.isNull)(NULL,
                           aSource,
                           MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteSFloat(aTarget,
                                   *(mtdRealType *)aSource) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}


static ACI_RC ulaConvertMtDouble(cmtAny *aTarget, void *aSource)
{
    //fix BUG-17873
    if ((*mtcdDouble.isNull)(NULL,
                             aSource,
                             MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL,INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteSDouble(aTarget,
                                    *(mtdDoubleType *)aSource) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));

    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtDate(cmtAny *aTarget, void *aSource)
{
    mtdDateType *sMtDate = (mtdDateType *)aSource;
    cmtDateTime *sCmDateTime;

    //fix BUG-17873
    if ((*mtcdDate.isNull)(NULL,
                           aSource,
                           MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyGetDateTimeForWrite(aTarget, &sCmDateTime)
                 != ACI_SUCCESS);

        sCmDateTime->mYear        = mtdDateInterfaceYear(sMtDate);
        sCmDateTime->mMonth       = mtdDateInterfaceMonth(sMtDate);
        sCmDateTime->mDay         = mtdDateInterfaceDay(sMtDate);
        sCmDateTime->mHour        = mtdDateInterfaceHour(sMtDate);
        sCmDateTime->mMinute      = mtdDateInterfaceMinute(sMtDate);
        sCmDateTime->mSecond      = mtdDateInterfaceSecond(sMtDate);
        sCmDateTime->mMicroSecond = mtdDateInterfaceMicroSecond(sMtDate);
        sCmDateTime->mTimeZone    = 0;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtInterval(cmtAny *aTarget, void *aSource)
{
    mtdIntervalType *sMtInterval = (mtdIntervalType *)aSource;
    cmtInterval     *sCmInterval;

    //fix BUG-17873
    if ((*mtcdInterval.isNull)(NULL,
                               aSource,
                               MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyGetIntervalForWrite(aTarget, &sCmInterval)
                 != ACI_SUCCESS);

        sCmInterval->mSecond      = sMtInterval->second;
        sCmInterval->mMicroSecond = sMtInterval->microsecond;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtNumeric(cmtAny           *aTarget,
                                  void             *aSource,
                                  ulaConvByteOrder  aByteOrder)
{
    //fix  BUG-17773.
    acp_uint8_t     sConvBuffer[MTD_NUMERIC_MANTISSA_MAXIMUM];
    acp_uint8_t     sMantissaBuffer[MTD_NUMERIC_MANTISSA_MAXIMUM];
    acp_uint8_t    *sConvMantissa = NULL;
    ulaConvNumeric  sSrc;
    ulaConvNumeric  sDst;
    cmtNumeric     *sCmNumeric = NULL;
    mtdNumericType *sMtNumeric = (mtdNumericType *)aSource;
    acp_sint16_t    sScale;
    acp_uint8_t     sPrecision;
    acp_char_t      sSign;
    acp_uint32_t    sMantissaLen;
    acp_uint32_t    sSize;
    acp_uint32_t    i;

    //fix BUG-17873
    if ((*mtcdNumeric.isNull)(NULL,
                              aSource,
                              MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        sMantissaLen = sMtNumeric->length - 1;
        ACI_TEST_RAISE(sMantissaLen > MTD_NUMERIC_MANTISSA_MAXIMUM,
                        INVALID_MANTISSA_LENGTH);
        sSign        = (sMtNumeric->signExponent & 0x80) ? 1 : 0;
        sScale       = (sMtNumeric->signExponent & 0x7F);
        sPrecision   = sMantissaLen * 2;


        if ((sScale != 0) && (sMantissaLen > 0))
        {
            sScale = (sMantissaLen - (sScale - 64)
                                    * ((sSign == 1) ? 1 : -1)) * 2;
            if (sSign != 1)
            {
                for (i = 0; i < sMantissaLen; i++)
                {
                    //fix  BUG-17773.
                    sMantissaBuffer[i] = 99 - sMtNumeric->mantissa[i];
                }
            }
            else
            {
                //fix  BUG-17773.
                acpMemCpy(sMantissaBuffer,sMtNumeric->mantissa,sMantissaLen);
            }
            //fix  BUG-17773.
            ulaConvNumericInitialize(&sSrc,
                                     sMantissaBuffer,
                                     sMantissaLen,
                                     sMantissaLen,
                                     100,
                                     ULA_BYTEORDER_BIG_ENDIAN);
            ulaConvNumericInitialize(&sDst,
                                     sConvBuffer,
                                     ULA_CONV_NUMERIC_BUFFER_SIZE,
                                     0,
                                     256,
                                     aByteOrder);

            if ((sMantissaBuffer[sMantissaLen - 1] % 10) == 0)
            {
                ulaConvNumericShiftRight(&sSrc);
                sScale--;
            }
            if (sMantissaBuffer[0] == 0)
            {
                sPrecision -= 2;
            }
            else if (sMantissaBuffer[0] < 10)
            {
                sPrecision--;
            }
            ACI_TEST(ulaConvNumericConvert(&sSrc, &sDst) != ACI_SUCCESS);
            sConvMantissa = ulaConvNumericGetBuffer(&sDst);
            sSize         = ulaConvNumericGetSize(&sDst);
        }
        else
        {
            sScale     = 0;
            sPrecision = 0;
            sSize      = 0;
        }

        ACI_TEST(cmtAnyGetNumericForWrite(aTarget,
                                          &sCmNumeric,
                                          sSize) != ACI_SUCCESS);
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        /*
         * if (sScale !=0 ....) 에서 else를 타면 sConvMantisaa가 null이다.
         */
        if (sConvMantissa != NULL)
        {
            acpMemCpy(sCmNumeric->mData, sConvMantissa, sSize);
        }
        else
        {
            //nothing to do
        }
        sCmNumeric->mSize      = sSize;
        sCmNumeric->mPrecision = sPrecision;
        sCmNumeric->mScale     = sScale;
        sCmNumeric->mSign      = sSign;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_MANTISSA_LENGTH)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_InvalidMantissaLength,
                                sMantissaLen));
    }
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtChar(cmtAny *aTarget, void *aSource)
{
    mtdCharType *sChar = (mtdCharType *)aSource;

    if ((*mtcdChar.isNull)(NULL,
                           aSource,
                           MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL,INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteVariable(aTarget,
                                     sChar->value,
                                     sChar->length) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtNchar(cmtAny *aTarget, void *aSource)
{
    mtdNcharType *sNchar = (mtdNcharType *)aSource;

    if ((*mtcdNchar.isNull)(NULL,
                            aSource,
                            MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL,INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteVariable(aTarget,
                                     sNchar->value,
                                     sNchar->length) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtBinary(cmtAny *aTarget, void *aSource)
{
    mtdBinaryType *sBinary = (mtdBinaryType *)aSource;

    if ((*mtcdBinary.isNull)(NULL,
                             aSource,
                             MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL,INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteBinary(aTarget,
                                   sBinary->mValue,
                                   sBinary->mLength) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtByte(cmtAny *aTarget, void *aSource)
{
    mtdByteType *sByte = (mtdByteType *)aSource;

    if ((*mtcdByte.isNull)(NULL,
                           aSource,
                           MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteBinary(aTarget,
                                   sByte->value,
                                   sByte->length) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtBit(cmtAny *aTarget, void *aSource)
{
    mtdBitType *sBit = (mtdBitType *)aSource;

    if ((*mtcdBit.isNull)(NULL,
                          aSource,
                          MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteBit(aTarget,
                                sBit->value,
                                sBit->length) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvertMtNibble(cmtAny *aTarget, void *aSource)
{
    mtdNibbleType *sNibble = (mtdNibbleType *)aSource;

    if ((*mtcdNibble.isNull)(NULL,
                             aSource,
                             MTD_OFFSET_USELESS) == ACP_TRUE)
    {
        ACI_TEST(cmtAnySetNull(aTarget) != ACI_SUCCESS);
    }
    else
    {
        //fix BUG-28927 MT->CM conversion function에서 safeguard가 필요.
        ACI_TEST_RAISE(aSource == NULL, INVALID_SOURCE_DATA);
        ACI_TEST(cmtAnyWriteNibble(aTarget,
                                   sNibble->value,
                                   sNibble->length) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION(INVALID_SOURCE_DATA)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NullSourceData));
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  ulaConvConvertFromMTToCMT()
 *
 *  원본은 mmcConvFromMT::convert 함수
 *
 *      - cmiProtocolContext 는 호출하는 곳에서 NULL 로 호출하므로 빼도 됨
 *      - mmcSession 은 단지 endian 을 알기 위한 목적으로만 쓰이므로
 *        원래의 ALA_GetODBCCValue() 함수에서 고정시켜 두었던
 *        MMC_BYTEORDER_LITTLE_ENDIAN 으로 사용 가능하다.
 *        따라서 mmcSession 은 필요없음.
 *
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaConvConvertFromMTToCMT(cmtAny           *aTarget,
                                 void             *aSource,
                                 acp_uint32_t      aSourceType,
                                 acp_uint32_t      aLobSize,
                                 ulaConvByteOrder  aByteOrder)
{
    // BUG-22609 AIX 최적화 오류 수정
    // switch 에 acp_uint32_t 형으로 음수값이 2번이상올때 서버 죽음
    acp_sint32_t    sType   = (acp_sint32_t)aSourceType;

    ACP_UNUSED(aByteOrder);

    switch (sType)
    {
        case MTD_NULL_ID:
            ACI_TEST(ulaConvertMtNull(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_BOOLEAN_ID:
            ACI_TEST(ulaConvertMtBoolean(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_SMALLINT_ID:
            ACI_TEST(ulaConvertMtSmallInt(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_INTEGER_ID:
            ACI_TEST(ulaConvertMtInteger(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_BIGINT_ID:
            ACI_TEST(ulaConvertMtBigInt(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_BLOB_LOCATOR_ID:
            // bug-19174
            ACI_TEST(ulaConvertMtBlobLocator(aTarget,
                                             aSource,
                                             aLobSize) != ACI_SUCCESS);
            break;

        case MTD_CLOB_LOCATOR_ID:
            // bug-19174
            ACI_TEST(ulaConvertMtClobLocator(aTarget,
                                             aSource,
                                             aLobSize) != ACI_SUCCESS);
            break;

        case MTD_REAL_ID:
            ACI_TEST(ulaConvertMtReal(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_DOUBLE_ID:
            ACI_TEST(ulaConvertMtDouble(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_DATE_ID:
            ACI_TEST(ulaConvertMtDate(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_INTERVAL_ID:
            ACI_TEST(ulaConvertMtInterval(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_FLOAT_ID:
        case MTD_NUMBER_ID:
        case MTD_NUMERIC_ID:
            ACI_TEST(ulaConvertMtNumeric(aTarget,
                                         aSource,
                                         ULA_BYTEORDER_LITTLE_ENDIAN)
                     != ACI_SUCCESS);
            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            ACI_TEST(ulaConvertMtChar(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            ACI_TEST(ulaConvertMtNchar(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_BINARY_ID:
            ACI_TEST(ulaConvertMtBinary(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            ACI_TEST(ulaConvertMtByte(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
            ACI_TEST(ulaConvertMtBit(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_NIBBLE_ID:
            ACI_TEST(ulaConvertMtNibble(aTarget, aSource) != ACI_SUCCESS);
            break;

        case MTD_BLOB_ID:
        case MTD_CLOB_ID:
        case MTD_LIST_ID:
        case MTD_NONE_ID:
        case MTS_FILETYPE_ID:
        default:
            ACI_RAISE(InvalidDataType);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidDataType)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_INVALID_DATA_CONVERSION));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

