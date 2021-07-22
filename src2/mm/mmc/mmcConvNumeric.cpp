/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <idl.h>
#include <mmErrorCode.h>
#include <mmcConvNumeric.h>


IDE_RC mmcConvNumeric::addBIG(mmcConvNumeric *aNumeric, UInt aValue)
{
    SInt sLSB = aNumeric->mAllocSize - 1;
    SInt sMSB = aNumeric->mAllocSize - aNumeric->mSize;
    UInt sValue;
    UInt sCarry;
    SInt i;

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
        IDE_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i--;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Overflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcConvNumeric::addLITTLE(mmcConvNumeric *aNumeric, UInt aValue)
{
    UInt sValue;
    UInt sCarry;
    UInt i;

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
        IDE_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Overflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcConvNumeric::shiftLeftBIG(mmcConvNumeric *aNumeric)
{
    SInt sLSB = aNumeric->mAllocSize - 1;
    SInt sMSB = aNumeric->mAllocSize - aNumeric->mSize;
    SInt i;

    if (aNumeric->mBuffer[sMSB] >= 10)
    {
        IDE_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

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

    return IDE_SUCCESS;

    IDE_EXCEPTION(Overflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcConvNumeric::shiftRightBIG(mmcConvNumeric *aNumeric)
{
    SInt sLSB = aNumeric->mAllocSize - 1;
    SInt sMSB = aNumeric->mAllocSize - aNumeric->mSize;
    SInt i;

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

    return IDE_SUCCESS;
}

IDE_RC mmcConvNumeric::multiplyBIG(mmcConvNumeric *aNumeric, UInt aValue)
{
    SInt sLSB   = aNumeric->mAllocSize - 1;
    SInt sMSB   = aNumeric->mAllocSize - aNumeric->mSize;
    UInt sCarry = 0;
    UInt sValue;
    SInt i;

    for (i = sLSB; i >= sMSB; i--)
    {
        sValue               = aNumeric->mBuffer[i] * aValue + sCarry;
        aNumeric->mBuffer[i] = sValue % aNumeric->mBase;
        sCarry               = sValue / aNumeric->mBase;
    }

    while (sCarry > 0)
    {
        IDE_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i--;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Overflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcConvNumeric::multiplyLITTLE(mmcConvNumeric *aNumeric, UInt aValue)
{
    UInt sCarry = 0;
    UInt sValue;
    UInt i;

    for (i = 0; i < aNumeric->mSize; i++)
    {
        sValue               = aNumeric->mBuffer[i] * aValue + sCarry;
        aNumeric->mBuffer[i] = sValue % aNumeric->mBase;
        sCarry               = sValue / aNumeric->mBase;
    }

    while (sCarry > 0)
    {
        IDE_TEST_RAISE((aNumeric->mSize + 1) > aNumeric->mAllocSize, Overflow);

        aNumeric->mBuffer[i] = sCarry % aNumeric->mBase;
        sCarry               = sCarry / aNumeric->mBase;

        aNumeric->mSize++;
        i++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Overflow);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NUMERIC_CONVERSION_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcConvNumeric::convertBIG(mmcConvNumeric *aSrc, mmcConvNumeric *aDst)
{
    UChar *sBuffer = getBufferBIG(aSrc);
    UInt   i;

    for (i = 0; i < aSrc->mSize; i++)
    {
        IDE_TEST(aDst->multiply(aSrc->mBase) != IDE_SUCCESS);
        IDE_TEST(aDst->add(sBuffer[i]) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcConvNumeric::convertLITTLE(mmcConvNumeric *aSrc, mmcConvNumeric *aDst)
{
    SInt i;

    for (i = aSrc->mSize - 1; i >= 0; i--)
    {
        IDE_TEST(aDst->multiply(aSrc->mBase) != IDE_SUCCESS);
        IDE_TEST(aDst->add(aSrc->mBuffer[i]) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

mmcConvNumericOp mmcConvNumeric::mOpAll[MMC_BYTEORDER_MAX] =
{
    {
        mmcConvNumeric::getBufferBIG,
        mmcConvNumeric::shiftLeftBIG,
        mmcConvNumeric::shiftRightBIG,
        mmcConvNumeric::addBIG,
        mmcConvNumeric::multiplyBIG,
        mmcConvNumeric::convertBIG
    },
    {
        mmcConvNumeric::getBufferLITTLE,
        NULL,
        NULL,
        mmcConvNumeric::addLITTLE,
        mmcConvNumeric::multiplyLITTLE,
        mmcConvNumeric::convertLITTLE
    }
};
