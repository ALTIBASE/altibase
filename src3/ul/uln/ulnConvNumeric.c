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

/*
 * Note : 급조한 소스이므로 리팩토링 절실히 필요하다.
 *        나중에 몰래 하자.
 */

/*
 * 소숫점이 mBuffer 의 앞쪽 바깥에 있다고 가정하고,
 * mScale 만큼 오른쪽으로 소숫점을 옮겨야 제대로 된 숫자가 나온다.
 */
// bug-25777: null-byte error when numeric to char conversion
// 밑에서 sTargetPosition 과 BufferSize 비교시 -1을 해주도록 변경
// appl buffersize는 null byte를 포함하는 크기이기 때문.
// 안 그러면 buffersize와 데이터 크기가 동일한 경우 문제 발생
#define ULNC_DECIMAL_PRINT_A_NUMBER(aCharacter)                 \
    do {                                                        \
        if (sSourcePosition >= aStartingPositionInSource)       \
        {                                                       \
            if(sTargetPosition < (aBufferSize - 1))             \
            {                                                   \
                if (aBuffer != NULL)                            \
                {                                               \
                    aBuffer[sTargetPosition] = (aCharacter);    \
                    sWritten++;                                 \
                }                                               \
                sTargetPosition++;                              \
            }                                                   \
            sNeeded++;                                          \
        }                                                       \
        sSourcePosition++;                                      \
    } while(0)

void ulncDecimalPrint(ulncNumeric   *aDecimal,
                      acp_char_t     *aBuffer,
                      acp_uint32_t   aBufferSize,
                      acp_uint32_t   aStartingPositionInSource,
                      ulnLengthPair *aLength)
{
    acp_sint32_t i;
    acp_sint32_t sNeeded          = 0;
    acp_sint32_t sWritten         = 0;
    acp_uint32_t sTargetPosition  = 0;
    acp_uint32_t sSourcePosition  = 0;

    ACE_ASSERT(aDecimal->mBase   == 10);
    ACE_ASSERT(aDecimal->mEndian == ULNC_ENDIAN_BIG);

    if(aDecimal->mSign == 0)
    {
        ULNC_DECIMAL_PRINT_A_NUMBER('-');
    }

    if (aDecimal->mPrecision - aDecimal->mScale <= 0)
    {
        // 0.0000123
        // 0.123
        ULNC_DECIMAL_PRINT_A_NUMBER('0');
        ULNC_DECIMAL_PRINT_A_NUMBER('.');

        for (i = (acp_sint32_t)aDecimal->mAllocSize - aDecimal->mScale;
             i < (acp_sint32_t)aDecimal->mAllocSize;
             i++)
        {
            ULNC_DECIMAL_PRINT_A_NUMBER(aDecimal->mBuffer[i] + '0');
        }
    }
    else
    {
        // 190000
        // 3.1415926

        for (i = (acp_sint32_t)aDecimal->mAllocSize - aDecimal->mPrecision;
             i < (acp_sint32_t)aDecimal->mAllocSize;
             i++)
        {
            if ((acp_sint32_t)aDecimal->mAllocSize - aDecimal->mScale == i)
            {
                ULNC_DECIMAL_PRINT_A_NUMBER('.');
            }

            ULNC_DECIMAL_PRINT_A_NUMBER(aDecimal->mBuffer[i] + '0');
        }

        // 뒤에 따라붙는 trailing 0
        for (i = aDecimal->mScale; i < 0; i++)
        {
            ULNC_DECIMAL_PRINT_A_NUMBER('0');
        }
    }

    aLength->mWritten = sWritten;
    aLength->mNeeded  = sNeeded;
}

void ulncDecimalPrintW(ulncNumeric   *aDecimal,
                       ulWChar       *aBuffer,
                       acp_uint32_t   aBufferSize,
                       acp_uint32_t   aStartingPositionInSource,
                       ulnLengthPair *aLength)
{
    acp_sint32_t i;
    acp_sint32_t sNeeded          = 0;
    acp_sint32_t sWritten         = 0;
    acp_uint32_t sTargetPosition  = 0;
    acp_uint32_t sSourcePosition  = 0;

    ACE_ASSERT(aDecimal->mBase   == 10);
    ACE_ASSERT(aDecimal->mEndian == ULNC_ENDIAN_BIG);

    if(aDecimal->mSign == 0)
    {
        ULNC_DECIMAL_PRINT_A_NUMBER(0x002D /*'-'*/);
    }

    if (aDecimal->mPrecision - aDecimal->mScale <= 0)
    {
        // 0.0000123
        // 0.123
        ULNC_DECIMAL_PRINT_A_NUMBER(0x0030 /*'0'*/);
        ULNC_DECIMAL_PRINT_A_NUMBER(0x002E /*'.'*/);

        for (i = (acp_sint32_t)aDecimal->mAllocSize - aDecimal->mScale;
             i < (acp_sint32_t)aDecimal->mAllocSize;
             i++)
        {
            ULNC_DECIMAL_PRINT_A_NUMBER(aDecimal->mBuffer[i] + 0x0030 /*'0'*/);
        }
    }
    else
    {
        // 190000
        // 3.1415926

        for (i = (acp_sint32_t)aDecimal->mAllocSize - aDecimal->mPrecision;
             i < (acp_sint32_t)aDecimal->mAllocSize;
             i++)
        {
            if ((acp_sint32_t)aDecimal->mAllocSize - aDecimal->mScale == i)
            {
                ULNC_DECIMAL_PRINT_A_NUMBER(0x002E /*'.'*/);
            }

            ULNC_DECIMAL_PRINT_A_NUMBER(aDecimal->mBuffer[i] + 0x0030 /*'0'*/);
        }

        // 뒤에 따라붙는 trailing 0
        for (i = aDecimal->mScale; i < 0; i++)
        {
            ULNC_DECIMAL_PRINT_A_NUMBER(0x0030 /*'0'*/);
        }
    }

    aLength->mWritten = sWritten * ACI_SIZEOF(ulWChar);
    aLength->mNeeded  = sNeeded * ACI_SIZEOF(ulWChar);
}

static ACI_RC ulncNumericMultiplyBIG(ulncNumeric *aNumeric, acp_uint32_t aValue)
{
    acp_sint32_t sLSB   = aNumeric->mAllocSize - 1;
    acp_sint32_t sMSB   = aNumeric->mAllocSize - aNumeric->mSize;
    acp_uint32_t sCarry = 0;
    acp_uint32_t sValue;
    acp_sint32_t i;

    for (i = sLSB; i >= sMSB; i--)
    {
        sValue               = aNumeric->mBuffer[i] * aValue + sCarry;
        aNumeric->mBuffer[i] = sValue % aNumeric->mBase;
        sCarry               = sValue / aNumeric->mBase;
    }

    while (sCarry > 0)
    {
        /*
         * overflow
         */
        ACI_TEST(aNumeric->mSize + 1 > aNumeric->mAllocSize);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i--;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulncNumericDevideBIG(ulncNumeric *aNumeric, acp_uint32_t aValue)
{
    acp_sint32_t sLSB    = aNumeric->mAllocSize - 1;
    acp_sint32_t sMSB    = aNumeric->mAllocSize - aNumeric->mSize;
    acp_uint32_t sRemain = 0;
    acp_uint32_t sValue;
    acp_sint32_t i;

    if (aNumeric->mSize > 0)
    {
        for (i = sMSB; i <= sLSB; i++)
        {
            sValue               = aNumeric->mBuffer[i] + (sRemain * aNumeric->mBase);
            sRemain              = sValue % aValue;
            aNumeric->mBuffer[i] = sValue / aValue;
        }
        if (aNumeric->mBuffer[sMSB] == 0)
        {
            aNumeric->mSize--;
        }
        ACI_TEST(sRemain != 0);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulncNumericAddBIG(ulncNumeric *aNumeric, acp_uint32_t aValue)
{
    acp_sint32_t sLSB = aNumeric->mAllocSize - 1;
    acp_sint32_t sMSB = aNumeric->mAllocSize - aNumeric->mSize;
    acp_uint32_t sValue;
    acp_uint32_t sCarry;
    acp_sint32_t i;

    sValue                  = aNumeric->mBuffer[sLSB] + aValue;
    aNumeric->mBuffer[sLSB] = sValue % aNumeric->mBase;
    sCarry                  = sValue / aNumeric->mBase;

    for (i = sLSB - 1; (i >= sMSB) && (sCarry > 0); i--)
    {
        sValue               = aNumeric->mBuffer[i] + sCarry;
        aNumeric->mBuffer[i] = sValue % aNumeric->mBase;
        sCarry               = sValue / aNumeric->mBase;
    }

    while (sCarry > 0)
    {
        /*
         * overflow
         */
        ACI_TEST((aNumeric->mSize + 1) > aNumeric->mAllocSize);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i--;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulncNumericMultiplyLITTLE(ulncNumeric *aNumeric, acp_uint32_t aValue)
{
    acp_uint32_t sCarry = 0;
    acp_uint32_t sValue;
    acp_uint32_t i;

    for (i = 0; i < aNumeric->mSize; i++)
    {
        sValue               = aNumeric->mBuffer[i] * aValue + sCarry;
        aNumeric->mBuffer[i] = sValue % aNumeric->mBase;
        sCarry               = sValue / aNumeric->mBase;
    }

    while (sCarry > 0)
    {
        /*
         * overflow
         */
        ACI_TEST((aNumeric->mSize + 1) > aNumeric->mAllocSize);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i++;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulncNumericDevideLITTLE(ulncNumeric *aNumeric, acp_uint32_t aValue)
{
    acp_sint32_t sLSB    = 0;
    acp_sint32_t sMSB    = aNumeric->mSize - 1;
    acp_uint32_t sRemain = 0;
    acp_uint32_t sValue;
    acp_sint32_t i;

    if (aNumeric->mSize > 0)
    {
        for (i = sMSB; i >= sLSB; i--)
        {
            sValue               = aNumeric->mBuffer[i] + (sRemain * aNumeric->mBase);
            sRemain              = sValue % aValue;
            aNumeric->mBuffer[i] = sValue / aValue;
        }
        if (aNumeric->mBuffer[sMSB] == 0)
        {
            aNumeric->mSize--;
        }
        ACI_TEST(sRemain != 0);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulncNumericAddLITTLE(ulncNumeric *aNumeric, acp_uint32_t aValue)
{
    acp_uint32_t sValue;
    acp_uint32_t sCarry;
    acp_uint32_t i;

    sValue               = aNumeric->mBuffer[0] + aValue;
    aNumeric->mBuffer[0] = sValue % aNumeric->mBase;
    sCarry               = sValue / aNumeric->mBase;

    for (i = 1; (i < aNumeric->mSize) && (sCarry > 0); i++)
    {
        sValue               = aNumeric->mBuffer[i] + sCarry;
        aNumeric->mBuffer[i] = sValue % aNumeric->mBase;
        sCarry               = sValue / aNumeric->mBase;
    }

    while (sCarry > 0)
    {
        /*
         * overflow
         */
        ACI_TEST((aNumeric->mSize + 1) > aNumeric->mAllocSize);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i++;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulncNumericInitialize(ulncNumeric *aNumeric,
                           acp_sint32_t aBase,
                           ulncEndian   aEndian,
                           acp_uint8_t *aBuffer,
                           acp_uint32_t aBufferSize)
{
    aNumeric->mAllocSize = aBufferSize;
    aNumeric->mPrecision = 1;
    aNumeric->mBase      = aBase;
    aNumeric->mScale     = 0;
    aNumeric->mEndian    = aEndian;
    aNumeric->mSize      = 1;
    aNumeric->mBuffer    = aBuffer;

    switch(aEndian)
    {
        case ULNC_ENDIAN_BIG:
            aNumeric->add      = ulncNumericAddBIG;
            aNumeric->multiply = ulncNumericMultiplyBIG;
            aNumeric->devide   = ulncNumericDevideBIG;
            break;

        case ULNC_ENDIAN_LITTLE:
            aNumeric->add      = ulncNumericAddLITTLE;
            aNumeric->multiply = ulncNumericMultiplyLITTLE;
            aNumeric->devide   = ulncNumericDevideLITTLE;
            break;
    }

    acpMemSet(aNumeric->mBuffer, 0, aBufferSize);
}

void ulncNumericInitFromData(ulncNumeric *aNumeric,
                             acp_sint32_t aBase,
                             ulncEndian   aEndian,
                             acp_uint8_t *aBuffer,
                             acp_uint32_t aBufferSize,
                             acp_uint32_t aMantissaLen)
{
    aNumeric->mAllocSize = aBufferSize;
    aNumeric->mBase      = aBase;
    aNumeric->mEndian    = aEndian;
    aNumeric->mSize      = aMantissaLen;
    aNumeric->mBuffer    = aBuffer;

    switch(aEndian)
    {
        case ULNC_ENDIAN_BIG:
            aNumeric->add      = ulncNumericAddBIG;
            aNumeric->multiply = ulncNumericMultiplyBIG;
            aNumeric->devide   = ulncNumericDevideBIG;
            break;

        case ULNC_ENDIAN_LITTLE:
            aNumeric->add      = ulncNumericAddLITTLE;
            aNumeric->multiply = ulncNumericMultiplyLITTLE;
            aNumeric->devide   = ulncNumericDevideLITTLE;
            break;
    }
}

ACI_RC ulncCmtNumericToDecimal(cmtNumeric *aCmNumeric, ulncNumeric *aDecimal)
{
    acp_sint32_t i;

    ACE_ASSERT(aDecimal->mBase   == 10);
    ACE_ASSERT(aDecimal->mEndian == ULNC_ENDIAN_BIG);

    for (i = aCmNumeric->mSize - 1; i >= 0; i--)
    {
        ACI_TEST(aDecimal->multiply(aDecimal, ULN_CM_NUMERIC_BASE) != ACI_SUCCESS);
        ACI_TEST(aDecimal->add     (aDecimal, aCmNumeric->mData[i]) != ACI_SUCCESS);
    }

    /*
     * 주의 : 아래의 precision 은 aDecimal 이 10진수이기 때문에
     *        mSize 를 곧장 mPrecision 에 대입이 가능한 것이다.
     */
    aDecimal->mPrecision = aDecimal->mSize;

    aDecimal->mScale     = aCmNumeric->mScale;
    aDecimal->mSign      = aCmNumeric->mSign;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulncNumericToNumeric(ulncNumeric *aDst, ulncNumeric *aSrc)
{
    acp_sint32_t   i;
    acp_uint8_t *sBuffer;

    switch(aSrc->mEndian)
    {
        case ULNC_ENDIAN_BIG:
            sBuffer = aSrc->mBuffer + aSrc->mAllocSize - aSrc->mSize;

            for (i = 0; i < (acp_sint32_t)aSrc->mSize; i++)
            {
                ACI_TEST(aDst->multiply(aDst, aSrc->mBase) != ACI_SUCCESS);
                ACI_TEST(aDst->add     (aDst, sBuffer[i]) != ACI_SUCCESS);
            }
            break;

        case ULNC_ENDIAN_LITTLE:
            for (i = aSrc->mSize - 1; i >= 0; i--)
            {
                ACI_TEST(aDst->multiply(aDst, aSrc->mBase) != ACI_SUCCESS);
                ACI_TEST(aDst->add     (aDst, aSrc->mBuffer[i]) != ACI_SUCCESS);
            }
            break;
    }

    aDst->mSign      = aSrc->mSign;
    aDst->mPrecision = aSrc->mPrecision;
    aDst->mScale     = aSrc->mScale;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_double_t ulncCmtNumericToDouble( cmtNumeric * aNumeric )
{
    acp_sint32_t i;
    acp_double_t sDoubleValue = 0;
    acp_double_t sTempValue   = 1.0;

    // BUG-21610: NUMERIC을 double로 올바로 변환하지 못할 수 있다.
    // double은 비록 NUMERIC보다 표현가능한 precision은 작지만
    // 더 넓은 scale을 지원하므로 그냥 변환해도 문제없다.
    for (i = aNumeric->mSize - 1; i >= 0; i--)
    {
        sDoubleValue *= 256;
        sDoubleValue += aNumeric->mData[i];
    }

    if (aNumeric->mScale > 0)
    {
        for (i = 0; i < aNumeric->mScale; i++)
        {
            sTempValue *= 10.0;
        }
        sDoubleValue /= sTempValue;
    }
    else if (aNumeric->mScale < 0)
    {
        for (i = 0; i > aNumeric->mScale; i--)
        {
            sTempValue *= 10.0;
        }
        sDoubleValue *= sTempValue;
    }

    if (aNumeric->mSign == 0)
    {
        sDoubleValue *= -1;
    }

    return sDoubleValue;
}

acp_float_t ulncCmtNumericToFloat(cmtNumeric *aNumeric)
{
    acp_float_t  sFloatValue = 0;

    sFloatValue = (acp_float_t)ulncCmtNumericToDouble( aNumeric );

    return sFloatValue;
}

/*
 * cmtNumeric 에서 바로 ULong 으로 가자니, overflow detect 하기가 까다롭다.
 * 그래서 일단 ulncDecimal 로 변환 했다가 간다. -_-;
 *
 * BUGBUG : 아래의 함수 둘의 공통부분을 찾아서 하나의 코드패스를 밟도록 해야 한다.
 *          ulncDecimalToULong()
 *          ulncDecimalToSLong()
 */
acp_uint64_t ulncDecimalToULong(ulncNumeric *aNumeric, acp_bool_t *aIsOverFlow)
{
    acp_sint32_t  i;

    acp_uint64_t sULongValue = 0;
    acp_uint64_t sRound      = 0;

    acp_uint64_t sThreshold;
    acp_uint64_t sMaxIncreaseUnit;

    ACE_ASSERT(aNumeric->mBase   == 10);
    ACE_ASSERT(aNumeric->mEndian == ULNC_ENDIAN_BIG);

    if (aNumeric->mSign == 0)
    {
        // 음수일 경우
        *aIsOverFlow = ACP_TRUE;
        return ACP_UINT64_LITERAL(0);
    }

    //    원래는 반올림을 했었는데,
    //    테스트 케이스들을 보니 반올림을 해서는 안될거 같아서 없앰.
//    if (aNumeric->mScale > 0)
//    {
//        // 소숫점 아래 숫자가 있긴 하다. 반올림을 미리 해 둔다.
//        if (aNumeric->mBuffer[aNumeric->mAllocSize - aNumeric->mScale] >= 5)
//        {
//            sRound = 1;
//        }
//    }

    sThreshold       = (ACP_UINT64_MAX - sRound) / aNumeric->mBase;
    sMaxIncreaseUnit = (ACP_UINT64_MAX - sRound) % aNumeric->mBase;

    if (aNumeric->mPrecision - aNumeric->mScale <= 0)
    {
        // 소숫점 위에는 하나도 없는 경우
        *aIsOverFlow = ACP_FALSE;
        return sRound;
    }
    else
    {
        for (i = (acp_sint32_t)aNumeric->mAllocSize - aNumeric->mPrecision;
             i < (acp_sint32_t)aNumeric->mAllocSize; i++)
        {
            if ((acp_sint32_t)aNumeric->mAllocSize - aNumeric->mScale == i)
            {
                // 소숫점을 만났다. 그만하고 리턴한다.
                *aIsOverFlow = ACP_FALSE;
                return sULongValue + sRound;
            }

            if ((sULongValue > sThreshold) ||
               ((sULongValue == sThreshold) && (aNumeric->mBuffer[i] > sMaxIncreaseUnit)))
            {
                *aIsOverFlow = ACP_TRUE;
                return ACP_UINT64_LITERAL(0);
            }

            sULongValue *= aNumeric->mBase;
            sULongValue += aNumeric->mBuffer[i];
        }

        // 뒤에 따라붙는 trailing 0 만큼 10씩 곱해준다.
        for (i = aNumeric->mScale; i < 0; i++)
        {
            if ((sULongValue > sThreshold) ||
               ((sULongValue == sThreshold) && (aNumeric->mBuffer[i] > sMaxIncreaseUnit)))
            {
                *aIsOverFlow = ACP_TRUE;
                return ACP_UINT64_LITERAL(0);
            }

            sULongValue *= aNumeric->mBase;
        }
    }

    return sULongValue + sRound;
}

/*
 * 일단, ULong 을 리턴하면, 호출자가 알아서 부호를 곱해 주도록 한다.
 *
 * BUGBUG : 위의 함수와 아래의 함수의 공통부분을 찾아서 하나의 코드패스를 밟도록 해야 한다.
 */
acp_uint64_t ulncDecimalToSLong(ulncNumeric *aNumeric, acp_bool_t *aIsOverFlow)
{
    acp_sint32_t i;

    acp_uint64_t sULongValue = 0;
    acp_uint64_t sRound      = 0;

    acp_uint64_t sThreshold;
    acp_uint64_t sMaxIncreaseUnit;

    ACE_ASSERT(aNumeric->mBase   == 10);
    ACE_ASSERT(aNumeric->mEndian == ULNC_ENDIAN_BIG);

    //    원래는 반올림을 했었는데,
    //    테스트 케이스들을 보니 반올림을 해서는 안될거 같아서 없앰.
//    if (aNumeric->mScale > 0)
//    {
//        // 소숫점 아래 숫자가 있긴 하다. 반올림을 미리 해 둔다.
//        if (aNumeric->mBuffer[aNumeric->mAllocSize - aNumeric->mScale] >= 5)
//        {
//            sRound = 1;
//        }
//    }

    if (aNumeric->mSign == 0)
    {
        // 음수일 경우 양수보다 절대값 1 을 더 표시할 수 있다.
        sThreshold       = ((acp_uint64_t)ACP_SINT64_MAX + (acp_uint64_t)(ACP_SINT64_LITERAL(1)) - sRound) / 10;
        sMaxIncreaseUnit = ((acp_uint64_t)ACP_SINT64_MAX + (acp_uint64_t)(ACP_SINT64_LITERAL(1)) - sRound) % 10;
    }
    else
    {
        sThreshold       = ((acp_uint64_t)ACP_SINT64_MAX - sRound) / aNumeric->mBase;
        sMaxIncreaseUnit = ((acp_uint64_t)ACP_SINT64_MAX - sRound) % aNumeric->mBase;
    }


    if (aNumeric->mPrecision - aNumeric->mScale <= 0)
    {
        // 소숫점 위에는 하나도 없는 경우
        *aIsOverFlow = ACP_FALSE;
        return sRound;
    }
    else
    {
        for (i = (acp_sint32_t)aNumeric->mAllocSize - aNumeric->mPrecision;
             i < (acp_sint32_t)aNumeric->mAllocSize;
             i++)
        {
            if ((acp_sint32_t)aNumeric->mAllocSize - aNumeric->mScale == i)
            {
                // 소숫점을 만났다. 그만하고 리턴한다.
                *aIsOverFlow = ACP_FALSE;
                return sULongValue + sRound;
            }

            if ((sULongValue > sThreshold) ||
               ((sULongValue == sThreshold) && (aNumeric->mBuffer[i] > sMaxIncreaseUnit)))
            {
                *aIsOverFlow = ACP_TRUE;
                return ACP_UINT64_LITERAL(0);
            }

            sULongValue *= aNumeric->mBase;
            sULongValue += aNumeric->mBuffer[i];
        }

        // 뒤에 따라붙는 trailing 0 만큼 10씩 곱해준다.
        for (i = aNumeric->mScale; i < 0; i++)
        {
            if ((sULongValue > sThreshold) ||
               ((sULongValue == sThreshold) && (aNumeric->mBuffer[i] > sMaxIncreaseUnit)))
            {
                *aIsOverFlow = ACP_TRUE;
                return ACP_UINT64_LITERAL(0);
            }

            sULongValue *= aNumeric->mBase;
        }
    }

    return sULongValue + sRound;
}

/*
 * 아래와 같은 함수가 필요한 것은,
 * 12000 과 같은 경우, precision 과 scale 을 numeric->add, numeric->multiply 를 이용할 경우
 * 제대로 2, 3 이라고 알아차리지 못하기 때문이다.
 */
void ulncSLongToSQLNUMERIC(acp_sint64_t aLongValue, SQL_NUMERIC_STRUCT *aNumeric)
{
    acp_uint64_t sLongValue;
    acp_uint64_t sOriginalValue;

    acp_char_t   sScale     = 0;
    acp_uint8_t  sPrecision = 0;
    acp_uint32_t sRemainder = 0;
    acp_uint32_t sPosition  = 0;

    if (aLongValue < 0)
    {
        aNumeric->sign = 0;

        /*
         * signed long 일 때 음수의 절대치 최대값이 양수의 절대치 최대값보다 1 크다.
         */
        sOriginalValue = (aLongValue + 1) * (-1);
        sOriginalValue++;
    }
    else
    {
        aNumeric->sign = 1;
        sOriginalValue = aLongValue;
    }

    sLongValue = sOriginalValue;

    /*
     * scale 계산
     */
    while (sLongValue > 0)
    {
        sRemainder = sLongValue % 10;

        if (sRemainder != 0)
        {
            break;
        }

        sScale--;
        sLongValue = sLongValue / 10;
    }

    sOriginalValue = sLongValue;

    /*
     * trailing 0 을 제외한 숫자(precision 파트)를 256진수로 변환
     */
    while (sLongValue > 0)
    {
        sRemainder = sLongValue % 256;
        sLongValue = sLongValue / 256;

        aNumeric->val[sPosition] = (acp_uint8_t)sRemainder;
        sPosition++;
    }

    /*
     * precision 세기
     */
    sLongValue = sOriginalValue;

    while (sLongValue > 0)
    {
        sRemainder = sLongValue % 10;
        sLongValue = sLongValue / 10;

        sPrecision++;
    }

    aNumeric->precision = sPrecision;
    aNumeric->scale     = sScale;
}

/*
 * 아래의 함수는 mtc::makeNumeric 함수를 그대로 복사해 와서 약간 수정한 것이다.
 */
ulncConvResult ulncCharToNumeric(ulncNumeric       *aNumeric,
                                 acp_uint32_t       aMaximumMantissa,
                                 const acp_uint8_t *aString,
                                 acp_uint32_t       aLength)
{
    ulncConvResult rc = ULNC_SUCCESS;

    const acp_uint8_t* sFence;
    const acp_uint8_t* sMantissaStart;
    const acp_uint8_t* sMantissaEnd;
    const acp_uint8_t* sPointPosition;
    acp_sint32_t       sSign;
    acp_sint32_t       sExponentSign;
    acp_sint32_t       sExponent;
    acp_sint32_t       sMantissaIterator;

    if( aLength == 0 || aString == NULL )
    {
        // aNumeric->length = 0;
    }
    else
    {
        /*
         * leading whitespace 제거
         */
        sSign  = 1;
        sFence = aString + aLength;
        for( ; aString < sFence; aString++ )
        {
            if( *aString != ' ' && *aString != '\t' )
            {
                break;
            }
        }
        ACI_TEST_RAISE( aString >= sFence, ERR_INVALID_LITERAL );

        /*
         * 부호
         */
        switch( *aString )
        {
            case '-':
                sSign = 0;
            case '+':
                aString++;
                ACI_TEST_RAISE( aString >= sFence, ERR_INVALID_LITERAL );
        }

        aNumeric->mSign = sSign;

        /*
         * . 위치 찾기
         */
        if( *aString == '.' )
        {
            sMantissaStart = aString;
            sPointPosition = aString;
            aString++;
            ACI_TEST_RAISE( aString >= sFence, ERR_INVALID_LITERAL );
            ACI_TEST_RAISE( *aString < '0' || *aString > '9',
                            ERR_INVALID_LITERAL );
            aString++;
            for( ; aString < sFence; aString++ )
            {
                if( *aString < '0' || *aString > '9' )
                {
                    break;
                }
            }
            sMantissaEnd = aString - 1;
        }
        else
        {
            ACI_TEST_RAISE( *aString < '0' || *aString > '9',
                            ERR_INVALID_LITERAL );
            sMantissaStart = aString;
            sPointPosition = NULL;
            aString++;
            for( ; aString < sFence; aString++ )
            {
                if( *aString == '.' )
                {
                    sPointPosition = aString;
                    aString++;
                    break;
                }
                if( *aString < '0' || *aString > '9' )
                {
                    break;
                }
            }
            for( ; aString < sFence; aString++ )
            {
                if( *aString < '0' || *aString > '9' )
                {
                    break;
                }
            }
            sMantissaEnd = aString - 1;
        }

        /*
         * e 뒤의 exponent 값 구하기
         */
        sExponent = 0;
        if( aString < sFence )
        {
            if( *aString == 'E' || *aString == 'e' )
            {
                sExponentSign = 1;
                aString++;
                ACI_TEST_RAISE( aString >= sFence, ERR_INVALID_LITERAL );
                switch( *aString )
                {
                 case '-':
                    sExponentSign = -1;
                 case '+':
                    aString++;
                    ACI_TEST_RAISE( aString >= sFence, ERR_INVALID_LITERAL );
                }
                if( sExponentSign > 0 )
                {
                    ACI_TEST_RAISE( *aString < '0' || *aString > '9',
                                    ERR_INVALID_LITERAL );
                    for( ; aString < sFence; aString++ )
                    {
                        if( *aString < '0' || *aString > '9' )
                        {
                            break;
                        }
                        sExponent = sExponent * 10 + *aString - '0';
                        ACI_TEST_RAISE( sExponent > 100000000,
                                        ERR_VALUE_OVERFLOW );
                    }
                    sExponent *= sExponentSign;
                }
                else
                {
                    ACI_TEST_RAISE( *aString < '0' || *aString > '9',
                                    ERR_INVALID_LITERAL );
                    for( ; aString < sFence; aString++ )
                    {
                        if( *aString < '0' || *aString > '9' )
                        {
                            break;
                        }
                        if( sExponent < 10000000 )
                        {
                            sExponent = sExponent * 10 + *aString - '0';
                        }
                    }
                    sExponent *= sExponentSign;
                }
            }
        }

        /*
         * trailing white space 제거
         */
        for( ; aString < sFence; aString++ )
        {
            if( *aString != ' ' && *aString != '\t' )
            {
                break;
            }
        }
        ACI_TEST_RAISE( aString < sFence, ERR_INVALID_LITERAL );

        /*
         * leading 0 와 trailing 0 를 제거하는 부분
         */
        if( sPointPosition == NULL )
        {
            sPointPosition = sMantissaEnd + 1;
        }
        for( ; sMantissaStart <= sMantissaEnd; sMantissaStart++ )
        {
            if( *sMantissaStart != '0' && *sMantissaStart != '.' )
            {
                break;
            }
        }
        for( ; sMantissaEnd >= sMantissaStart; sMantissaEnd-- )
        {
            if( *sMantissaEnd != '0' && *sMantissaEnd != '.' )
            {
                break;
            }
        }

        /*
         * 실제 변환이 시작되는 곳이다.
         */
        if( sMantissaStart > sMantissaEnd )
        {
            /* Zero : do nothin' */
        }
        else
        {
            /*
             * 소숫점의 위치에 따라 exponent 값을 조정해 준다.
             */
            if( sMantissaStart < sPointPosition )
            {
                sExponent += sPointPosition - sMantissaStart;
            }
            else
            {
                sExponent -= sMantissaStart - sPointPosition - 1;
            }

            sMantissaIterator = 0;

            for( ; sMantissaIterator < (acp_sint32_t)aMaximumMantissa; sMantissaIterator++ )
            {
                if( sMantissaEnd < sMantissaStart )
                {
                    break;
                }
                if( *sMantissaStart == '.' )
                {
                    sMantissaStart++;
                }
                if( sMantissaEnd - sMantissaStart < 1 )
                {
                    break;
                }

                ACI_TEST_RAISE(aNumeric->multiply(aNumeric, 10) != ACI_SUCCESS,
                               ERR_VALUE_OVERFLOW);
                ACI_TEST_RAISE(aNumeric->add(aNumeric, sMantissaStart[0] - '0') != ACI_SUCCESS,
                               ERR_VALUE_OVERFLOW);

                // aNumeric->mBuffer[sMantissaIterator] = sMantissaStart[0] - '0';
                sMantissaStart++;
            }

            if( sMantissaStart <= sMantissaEnd )
            {
                if( *sMantissaStart == '.' )
                {
                    sMantissaStart++;
                }
            }

            /*
             *  마지막 꼬다리에 남은 숫자 하나를 처리하는 녀석임.
             */
            if( sMantissaIterator < (acp_sint32_t)aMaximumMantissa && sMantissaStart == sMantissaEnd )
            {
                ACI_TEST_RAISE(aNumeric->multiply(aNumeric, 10) != ACI_SUCCESS,
                               ERR_VALUE_OVERFLOW);
                ACI_TEST_RAISE(aNumeric->add(aNumeric, sMantissaStart[0] - '0') != ACI_SUCCESS,
                               ERR_VALUE_OVERFLOW);
                sMantissaStart++;
                sMantissaIterator++;
            }

            if( sMantissaStart <= sMantissaEnd )
            {
                /*
                 * 반올림
                 */
                if( sMantissaStart[0] >= '5' )
                {
                    ACI_TEST_RAISE(aNumeric->add(aNumeric, 5 - (sMantissaStart[0] - '0' - 5))
                                   != ACI_SUCCESS,
                                   ERR_VALUE_OVERFLOW);
                }
            }

            aNumeric->mPrecision = sMantissaIterator;
            aNumeric->mScale     = (sExponent - sMantissaIterator) * (-1);
        }
    }

    return ULNC_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_LITERAL)
    {
        rc =  ULNC_INVALID_LITERAL;
    }
    ACI_EXCEPTION(ERR_VALUE_OVERFLOW)
    {
        rc =  ULNC_VALUE_OVERFLOW;
    }
    ACI_EXCEPTION_END;
    ACE_ASSERT(rc != ULNC_SUCCESS);

    return rc;
}

// proj_2160 cm_type removal
// big-endian only
// this is just a copy of mmcConvNumeric::shiftLeftBIG() 
ACI_RC ulncShiftLeft(ulncNumeric *aNumeric)
{
    acp_sint32_t sLSB = aNumeric->mAllocSize - 1;
    acp_sint32_t sMSB = aNumeric->mAllocSize - aNumeric->mSize;
    acp_sint32_t i;

    if (aNumeric->mBuffer[sMSB] >= 10)
    {
        ACI_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[sMSB - 1] = aNumeric->mBuffer[sMSB] / 10;
        aNumeric->mBuffer[sMSB]     = (aNumeric->mBuffer[sMSB] % 10) * 10;

        aNumeric->mSize++;
    }
    else
    {
        aNumeric->mBuffer[sMSB] = (aNumeric->mBuffer[sMSB] % 10) * 10;
    }

    for (i = sMSB + 1; i <= sLSB; i++)
    {
        aNumeric->mBuffer[i - 1] += aNumeric->mBuffer[i] / 10;
        aNumeric->mBuffer[i]      = (aNumeric->mBuffer[i] % 10) * 10;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Overflow);
    {
        ACI_SET(aciSetErrorCode(ulERR_ABORT_NUMERIC_VALUE_OUT_OF_RANGE));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// big-endian only
static void ulncShiftRight(ulncNumeric *aNumeric)
{
    acp_sint32_t sLSB = aNumeric->mAllocSize - 1;
    acp_sint32_t sMSB = aNumeric->mAllocSize - aNumeric->mSize;
    acp_sint32_t i;

    for (i = sLSB; i >= sMSB + 1; i--)
    {
        aNumeric->mBuffer[i]  = aNumeric->mBuffer[i] / 10;
        aNumeric->mBuffer[i] += (aNumeric->mBuffer[i - 1] % 10) * 10;
    }

    aNumeric->mBuffer[sMSB] = aNumeric->mBuffer[sMSB] / 10;

    if ((aNumeric->mSize > 1) && (aNumeric->mBuffer[sMSB] == 0))
    {
        aNumeric->mSize--;
    }
}

/* copy convertMtNumeric func*/
ACI_RC ulncMtNumericToCmNumeric(cmtNumeric *aCmNumeric, mtdNumericType *aData)
{
    acp_uint8_t     sConvBuffer[MTD_NUMERIC_MANTISSA_MAXIMUM];
    acp_uint8_t     sMantissaBuffer[MTD_NUMERIC_MANTISSA_MAXIMUM];
    mtdNumericType *sMtNumeric = aData;
    ulncNumeric     sSrc;
    ulncNumeric     sDst;

    acp_sint16_t    sScale;
    acp_uint8_t     sPrecision;
    acp_sint8_t     sSign;
    acp_uint32_t    sMantissaLen;
    acp_uint32_t    i;


    // 차후에 사용자버퍼로 복사할때
    // SQL_MAX_NUMERIC_LEN(16bytes)만큼 무조건 복사하기 때문에
    // 초기화를 하지 않으면, 쓰레기가 복사된다.
    // cf) ulncNUMERIC_NUMERIC()
    acpMemSet(aCmNumeric, 0, ACI_SIZEOF(cmtNumeric));

    sMantissaLen = sMtNumeric->length - 1;
    sSign        = (sMtNumeric->signExponent & 0x80) ? 1 : 0;
    sScale       = (sMtNumeric->signExponent & 0x7F);
    sPrecision   = sMantissaLen * 2;

    if ((sScale != 0) && (sMantissaLen > 0))
    {
        sScale = (sMantissaLen - (sScale - 64) * ((sSign == 1) ? 1 : -1)) * 2;
        if (sSign != 1)
        {
            for (i = 0; i < sMantissaLen; i++)
            {
                sMantissaBuffer[i] = 99 - sMtNumeric->mantissa[i];
            }
        }
        else
        {
            acpMemCpy(sMantissaBuffer, sMtNumeric->mantissa, sMantissaLen);
        }

        ulncNumericInitFromData(&sSrc,
                                100,
                                ULNC_ENDIAN_BIG,
                                sMantissaBuffer,
                                sMantissaLen,
                                sMantissaLen);

        ulncNumericInitialize(&sDst,
                              256,
                              ULNC_ENDIAN_LITTLE,
                              sConvBuffer,
                              MTD_NUMERIC_MANTISSA_MAXIMUM);

        if ( (sMantissaBuffer[sMantissaLen - 1] % 10) == 0)
        {
            ulncShiftRight(&sSrc);
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
        ACI_TEST( ulncNumericToNumeric( &sDst, &sSrc) != ACI_SUCCESS);


        aCmNumeric->mSize      = sDst.mSize;
        aCmNumeric->mPrecision = sPrecision;
        aCmNumeric->mScale     = sScale;
        aCmNumeric->mSign      = sSign;
        acpMemCpy(aCmNumeric->mData, sDst.mBuffer, sDst.mSize);
    }
    else
    {
        aCmNumeric->mSize      = 0;
        aCmNumeric->mPrecision = 0;
        aCmNumeric->mScale     = 0;
        aCmNumeric->mSign      = sSign;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

