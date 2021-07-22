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

#ifndef _O_MMC_CONV_NUMERIC_H_
#define _O_MMC_CONV_NUMERIC_H_ 1

#include <idl.h>
#include <ide.h>
#include <mmcDef.h>


class mmcConvNumeric;

typedef struct mmcConvNumericOp
{
    UChar *(*mGetBuffer)(mmcConvNumeric *aNumeric);
    IDE_RC (*mShiftLeft)(mmcConvNumeric *aNumeric);
    IDE_RC (*mShiftRight)(mmcConvNumeric *aNumeric);
    IDE_RC (*mAdd)(mmcConvNumeric *aNumeric, UInt aValue);
    IDE_RC (*mMultiply)(mmcConvNumeric *aNumeric, UInt aValue);
    IDE_RC (*mConvert)(mmcConvNumeric *aSrc, mmcConvNumeric *aDst);
} mmcConvNumericOp;


class mmcConvNumeric
{
private:
    static mmcConvNumericOp  mOpAll[MMC_BYTEORDER_MAX];

    UChar                   *mBuffer;
    UInt                     mAllocSize;
    UInt                     mSize;

    UInt                     mBase;
    mmcByteOrder             mByteOrder;

    mmcConvNumericOp        *mOp;

public:
    void   initialize(UChar        *aBuffer,
                      UChar         aAllocSize,
                      UChar         aSize,
                      UInt          aBase,
                      mmcByteOrder  aByteOrder);

    UChar *getBuffer();
    UInt   getSize();

    IDE_RC shiftLeft();
    IDE_RC shiftRight();
    IDE_RC add(UInt aValue);
    IDE_RC multiply(UInt aValue);
    IDE_RC convert(mmcConvNumeric *aNumeric);

private:
    static UChar *getBufferBIG(mmcConvNumeric *aNumeric);
    static UChar *getBufferLITTLE(mmcConvNumeric *aNumeric);
    static IDE_RC shiftLeftBIG(mmcConvNumeric *aNumeric);
    static IDE_RC shiftRightBIG(mmcConvNumeric *aNumeric);
    static IDE_RC addBIG(mmcConvNumeric *aNumeric, UInt aValue);
    static IDE_RC addLITTLE(mmcConvNumeric *aNumeric, UInt aValue);
    static IDE_RC multiplyBIG(mmcConvNumeric *aNumeric, UInt aValue);
    static IDE_RC multiplyLITTLE(mmcConvNumeric *aNumeric, UInt aValue);
    static IDE_RC convertBIG(mmcConvNumeric *aSrc, mmcConvNumeric *aDst);
    static IDE_RC convertLITTLE(mmcConvNumeric *aSrc, mmcConvNumeric *aDst);
};


inline void mmcConvNumeric::initialize(UChar        *aBuffer,
                                       UChar         aAllocSize,
                                       UChar         aSize,
                                       UInt          aBase,
                                       mmcByteOrder  aByteOrder)
{
    mBuffer    = aBuffer;
    mAllocSize = aAllocSize;
    mBase      = aBase;
    mByteOrder = aByteOrder;
    mOp        = &mOpAll[mByteOrder];

    /* fix BUG-31002, Remove unnecessary code dependency from the inline function of mm module. */
    if ((aAllocSize > 0) && (aSize == 0))
    {
        mSize = 1;

        /* fix BUG-31002, Remove unnecessary code dependency from the inline function of mm module. */
        if (aByteOrder == MMC_BYTEORDER_BIG_ENDIAN)
        {
            aBuffer[aAllocSize - 1] = 0;
        }
        else
        {
            aBuffer[0] = 0;
        }
    }
    /* fix BUG-31002, Remove unnecessary code dependency from the inline function of mm module. */
    else
    {
        mSize = aSize;
    }
}

inline UChar *mmcConvNumeric::getBuffer()
{
    return mOp->mGetBuffer(this);
}

inline UInt mmcConvNumeric::getSize()
{
    return mSize;
}

inline IDE_RC mmcConvNumeric::shiftLeft()
{
    IDE_DASSERT(mBase == 100);

    return mOp->mShiftLeft(this);
}

inline IDE_RC mmcConvNumeric::shiftRight()
{
    IDE_DASSERT(mBase == 100);

    return mOp->mShiftRight(this);
}

inline IDE_RC mmcConvNumeric::add(UInt aValue)
{
    return mOp->mAdd(this, aValue);
}

inline IDE_RC mmcConvNumeric::multiply(UInt aValue)
{
    return mOp->mMultiply(this, aValue);
}

inline IDE_RC mmcConvNumeric::convert(mmcConvNumeric *aNumeric)
{
    return mOp->mConvert(this, aNumeric);
}

inline UChar *mmcConvNumeric::getBufferBIG(mmcConvNumeric *aNumeric)
{
    return aNumeric->mBuffer + aNumeric->mAllocSize - aNumeric->mSize;
}

inline UChar *mmcConvNumeric::getBufferLITTLE(mmcConvNumeric *aNumeric)
{
    return aNumeric->mBuffer;
}


#endif
