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
 * $Id: ulaConvNumeric.c 82075 2018-01-17 06:39:52Z jina.kim $
 ******************************************************************************/

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <aciTypes.h>
#include <aciErrorMgr.h>

#include <ulaConvNumeric.h>
#include <ulaErrorCode.h>

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
 *      ulaConvNumeric.cpp 파일 참조.
 * -----------------------------------------------------------------------------
 */

static acp_uint8_t *ulaConvNumericGetBufferBIG(ulaConvNumeric *aNumeric)
{
    return aNumeric->mBuffer + aNumeric->mAllocSize - aNumeric->mSize;
}

static acp_uint8_t *ulaConvNumericGetBufferLITTLE(ulaConvNumeric *aNumeric)
{
    return aNumeric->mBuffer;
}


static ACI_RC ulaConvNumericShiftLeftBIG(ulaConvNumeric *aNumeric)
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

    ACI_EXCEPTION(Overflow)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaConvNumericShiftRightBIG(ulaConvNumeric *aNumeric)
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

    return ACI_SUCCESS;
}

static ACI_RC ulaConvNumericAddBIG(ulaConvNumeric *aNumeric,
                                   acp_uint32_t    aValue)
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
        ACI_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i--;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Overflow)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaConvNumericAddLITTLE(ulaConvNumeric *aNumeric,
                                      acp_uint32_t    aValue)
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
        ACI_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i++;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Overflow)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaConvNumericMultiplyBIG(ulaConvNumeric *aNumeric,
                                        acp_uint32_t    aValue)
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
        ACI_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i--;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Overflow)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaConvNumericMultiplyLITTLE(ulaConvNumeric *aNumeric,
                                           acp_uint32_t    aValue)
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
        ACI_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i++;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Overflow)
    {
        ACI_SET(aciSetErrorCode(ulaERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaConvNumericConvertBIG(ulaConvNumeric *aSrc,
                                       ulaConvNumeric *aDst)
{
    acp_uint8_t *sBuffer = ulaConvNumericGetBufferBIG(aSrc);
    acp_uint32_t   i;

    for (i = 0; i < aSrc->mSize; i++)
    {
        ACI_TEST(ulaConvNumericMultiply(aDst, aSrc->mBase) != ACI_SUCCESS);
        ACI_TEST(ulaConvNumericAdd(aDst, sBuffer[i]) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC ulaConvNumericConvertLITTLE(ulaConvNumeric *aSrc,
                                          ulaConvNumeric *aDst)
{
    acp_sint32_t i;

    for (i = aSrc->mSize - 1; i >= 0; i--)
    {
        ACI_TEST(ulaConvNumericMultiply(aDst, aSrc->mBase) != ACI_SUCCESS);
        ACI_TEST(ulaConvNumericAdd(aDst, aSrc->mBuffer[i]) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ulaConvNumericOp ulaConvNumericMOpAll[ULA_BYTEORDER_MAX] =
{
    {
        ulaConvNumericGetBufferBIG,
        ulaConvNumericShiftLeftBIG,
        ulaConvNumericShiftRightBIG,
        ulaConvNumericAddBIG,
        ulaConvNumericMultiplyBIG,
        ulaConvNumericConvertBIG
    },
    {
        ulaConvNumericGetBufferLITTLE,
        NULL,
        NULL,
        ulaConvNumericAddLITTLE,
        ulaConvNumericMultiplyLITTLE,
        ulaConvNumericConvertLITTLE
    }
};


void ulaConvNumericInitialize(ulaConvNumeric   *aNumeric,
                              acp_uint8_t      *aBuffer,
                              acp_uint8_t       aAllocSize,
                              acp_uint8_t       aSize,
                              acp_uint32_t      aBase,
                              ulaConvByteOrder  aByteOrder)
{
    aNumeric->mBuffer    = aBuffer;
    aNumeric->mAllocSize = aAllocSize;
    aNumeric->mSize      = aSize;
    aNumeric->mBase      = aBase;
    aNumeric->mByteOrder = aByteOrder;
    aNumeric->mOp        = &ulaConvNumericMOpAll[aNumeric->mByteOrder];

    if ((aNumeric->mAllocSize > 0) && (aNumeric->mSize == 0))
    {
        aNumeric->mSize = 1;

        if (aNumeric->mByteOrder == ULA_BYTEORDER_BIG_ENDIAN)
        {
            aNumeric->mBuffer[aNumeric->mAllocSize - 1] = 0;
        }
        else
        {
            aNumeric->mBuffer[0] = 0;
        }
    }
}

acp_uint8_t *ulaConvNumericGetBuffer(ulaConvNumeric *aNumeric)
{
    return (*aNumeric->mOp->mGetBuffer)(aNumeric);
}

acp_uint32_t ulaConvNumericGetSize(ulaConvNumeric *aNumeric)
{
    return aNumeric->mSize;
}

ACI_RC ulaConvNumericShiftLeft(ulaConvNumeric *aNumeric)
{
    ACE_DASSERT(aNumeric->mBase == 100);

    return (*aNumeric->mOp->mShiftLeft)(aNumeric);
}

ACI_RC ulaConvNumericShiftRight(ulaConvNumeric *aNumeric)
{
    ACE_DASSERT(aNumeric->mBase == 100);

    return (*aNumeric->mOp->mShiftRight)(aNumeric);
}

ACI_RC ulaConvNumericAdd(ulaConvNumeric *aNumeric, acp_uint32_t aValue)
{
    return (*aNumeric->mOp->mAdd)(aNumeric, aValue);
}

ACI_RC ulaConvNumericMultiply(ulaConvNumeric *aNumeric, acp_uint32_t aValue)
{
    return (*aNumeric->mOp->mMultiply)(aNumeric, aValue);
}

ACI_RC ulaConvNumericConvert(ulaConvNumeric *aSrc, ulaConvNumeric *aDst)
{
    return (*aSrc->mOp->mConvert)(aSrc, aDst);
}

