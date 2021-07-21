/*
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

package Altibase.jdbc.driver;

import java.math.BigDecimal;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public final class AltibaseNumeric extends Number implements Comparable
{
    public static final int   MIN_BASE          = 2;
    public static final int   MAX_BASE          = 256;
    public static final int   MIN_PRECISION     = 0;
    public static final int   MAX_PRECISION     = 38;
    public static final int   DEFAULT_PRECISION = MAX_PRECISION;
    public static final int   MIN_SCALE         = -84;
    public static final int   MAX_SCALE         = 128;
    public static final int   DEFAULT_SCALE     = 0;
    public static final int   SIGN_PLUS         = 1;
    public static final int   SIGN_MINUS        = -1;
    public static final int   DEFAUL_SIGN       = SIGN_PLUS;

    private static final int  MAX_MANTISSA      = ((38 + 2) / 2);
    private static final long serialVersionUID  = 2863039045804461329L;

    private final int         mBase;
    private int               mSize;
    private int               mPrecision;
    private int               mScale;
    private int               mSign;
    private byte[]            mMantissa;

    private AltibaseNumeric(int aBase)
    {
        this(aBase, DEFAULT_PRECISION, DEFAULT_SCALE, DEFAUL_SIGN, null, 0);
    }

    private AltibaseNumeric(int aBase, int aPrecision, int aScale)
    {
        this(aBase, aPrecision, aScale, 1, null, 0);
    }

    private AltibaseNumeric(int aBase, byte[] aMantissa)
    {
        this(aBase, DEFAULT_PRECISION, DEFAULT_SCALE, DEFAUL_SIGN, aMantissa, 0);
    }

    private AltibaseNumeric(int aBase, int aPrecision, int aScale, int aSign, byte[] aMantissa)
    {
        this(aBase, aPrecision, aScale, aSign, aMantissa, 0);
    }

    public AltibaseNumeric(int aBase, byte[] aMantissa, int aMaxMantissa)
    {
        this(aBase, DEFAULT_PRECISION, DEFAULT_SCALE, DEFAUL_SIGN, aMantissa, aMaxMantissa);
    }

    private AltibaseNumeric(int aBase, int aPrecision, int aScale, int aSign, byte[] aMantissa, int aMaxMantissa)
    {
        if (aBase < MIN_BASE || MAX_BASE < aBase)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "Base",
                                                MIN_BASE + " ~ " + MAX_BASE,
                                                String.valueOf(aBase));
        }
        mPrecision = aPrecision;
        mBase = aBase;
        mScale = aScale;
        mSign = aSign;
        if (aMaxMantissa == 0)
        {
            int sMP = MAX_BASE / aBase;
            if ((MAX_BASE % aBase) > 0)
            {
                sMP++;
            }
            aMaxMantissa = MAX_MANTISSA * sMP;
        }
        mMantissa = new byte[aMaxMantissa];
        if (aMantissa != null && aMantissa.length != 0)
        {
            System.arraycopy(aMantissa, 0, mMantissa, 0, aMantissa.length);
            mSize = aMantissa.length;
        }
        else
        {
            mSize = 1;
        }
    }

    public AltibaseNumeric(int aBase, AltibaseNumeric aSrc)
    {
        this(aBase);

        synchronized (aSrc)
        {
            int sLen = aSrc.mMantissa.length;
            for (int i = sLen - aSrc.mSize; i < sLen; i++)
            {
                multiply(aSrc.mBase);
                add(aSrc.mMantissa[i] & 0xFF);
            }
        }
    }

    public int size()
    {
        return mSize;
    }

    public void add(int aValue)
    {
        int sLSB = mMantissa.length - 1;
        int sMSB = mMantissa.length - mSize;

        int sValue = mMantissa[sLSB] + aValue;
        mMantissa[sLSB] = (byte)(sValue % mBase);
        int sCarry = sValue / mBase;

        int i = sLSB - 1;
        for (; (sCarry > 0) && (i >= sMSB); i--)
        {
            sValue = mMantissa[i] + sCarry;
            mMantissa[i] = (byte)(sValue % mBase);
            sCarry = sValue / mBase;
        }
        for (; sCarry > 0; i--)
        {
            if ((mSize + 1) > mMantissa.length)
            {
                // BUG-41585 mantissa overflow
                Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                    "Mantissa",
                                                    0 + " ~ " + mMantissa.length,
                                                    String.valueOf(mSize + 1));
            }
            mMantissa[i] = (byte)(sCarry % mBase);
            sCarry /= mBase;
            mSize++;
        }
    }

    public void multiply(int aMultiplicand)
    {
        int sLSB = mMantissa.length - 1;
        int sMSB = mMantissa.length - mSize;
        int sValue;
        int sCarry = 0;

        int i = sLSB;
        for (; i >= sMSB; i--)
        {
            sValue = mMantissa[i] * aMultiplicand + sCarry;
            mMantissa[i] = (byte)(sValue % mBase);
            sCarry = sValue / mBase;
        }
        for (; sCarry > 0; i--)
        {
            if ((mSize + 1) > mMantissa.length)
            {
                // BUG-41585 mantissa overflow
                Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                    "Mantissa",
                                                    0 + " ~ " + mMantissa.length,
                                                    String.valueOf(mSize + 1));
            }
            mMantissa[i] = (byte)(sCarry % mBase);
            sCarry /= mBase;
            mSize++;
        }
    }

    public void negate()
    {
        int sLSB = mMantissa.length - 1;
        int sMSB = mMantissa.length - mSize;
        for (int i = sLSB; i >= sMSB; i--)
        {
            mMantissa[i] = (byte)((mBase - 1) - mMantissa[i]);
        }
        mSign = -mSign;
    }

    public byte[] toByteArray()
    {
        byte[] sByteArray = new byte[mSize];
        System.arraycopy(mMantissa, mMantissa.length - mSize, sByteArray, 0, mSize);
        return sByteArray;
    }

    public byte[] toTrimmedByteArray(int aTrimSize)
    {
        byte[] sByteArray = new byte[aTrimSize];
        System.arraycopy(mMantissa, mMantissa.length - mSize, sByteArray, 0, aTrimSize);
        return sByteArray;
    }

    public BigDecimal toBigDecimal()
    {
        // IMPROVEMENT (2013-03-25) 요구가 생기면 처리
        return null;
    }

    // #region Number class

    public int intValue()
    {
        // IMPROVEMENT (2013-03-25) 요구가 생기면 처리
        return 0;
    }

    public long longValue()
    {
        // IMPROVEMENT (2013-03-25) 요구가 생기면 처리
        return 0;
    }

    public float floatValue()
    {
        // IMPROVEMENT (2013-03-25) 요구가 생기면 처리
        return 0;
    }

    public double doubleValue()
    {
        // IMPROVEMENT (2013-03-25) 요구가 생기면 처리
        return 0;
    }

    // #endregion

    // #region Comparable interface

    /**
     * 지정한 object와 비교해서 이 객체가 크면 양수, 같으면 0, 작으면 음수를 반환한다.
     *
     * @return 이 객체가 크면 양수, 같으면 0, 작으면 음수
     * @throws ClassCastException <tt>aObject</tt> is not a BigInteger.
     */
    public int compareTo(Object aObject)
    {
        if (aObject == this)
        {
            return 0;
        }
        return compareTo((AltibaseNumeric)aObject);
    }

    private int compareTo(AltibaseNumeric aNumeric)
    {
        // IMPROVEMENT (2013-03-25) 요구가 생기면 처리
        return 0;
    }

    // #endregion
}
