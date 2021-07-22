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

package Altibase.jdbc.driver.datatype;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.Reader;
import java.io.StringReader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.sql.SQLException;

import Altibase.jdbc.driver.AltibaseNumeric;
import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.ObjectDynamicArray;

abstract class CommonNumericColumn extends AbstractColumn
{
    private static final BigInteger ZERO_VALUE                    = BigInteger.ZERO;
    private static final BigDecimal NULL_VALUE                    = new BigDecimal((double)0x8000000000000000L);
    private static final byte[]     NULL_VALUE_MANTISSA           = new byte[0];

    private final static int        RADIX10BYTE                   = 100 & 0xFF;
    private final static BigInteger DIV                           = new BigInteger("10");
    private static final byte       SIGN_MINUS                    = -1;
    private static final byte       SIGN_PLUS                     = 1;
    private static final byte       SIGN_PLUS_FLAG                = (byte)0x80;
    private static final int        NUMERIC_PRECISION_MINIMUM     = 0;
    private static final int        NUMERIC_PRECISION_MAXIMUM     = 38;
    private static final int        NUMERIC_SCALE_MINIMUM         = -84;
    private static final int        NUMERIC_SCALE_MAXIMUM         = 128;
    private static final int        NUMERIC_SIGN_EXPONENT_MINIMUM = 0;
    private static final int        NUMERIC_SIGN_EXPONENT_MAXIMUM = 127;

    private short                   mSize                         = 0;
    private byte                    mSignExponent                 = 0;
    private byte[]                  mMantissa                     = NULL_VALUE_MANTISSA;
    private BigDecimal              mBigDecimalValue              = null;

    CommonNumericColumn()
    {
    }
    
    public String getObjectClassName()
    {
        return BigDecimal.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return getColumnInfo().getPrecision() + 2; // 부호 1바이트, 소수점 1바이트 추가
    }

    public int getOctetLength()
    {
        // MTD_NUMERIC_SIZE() in MT module (mtdTypes.h)
        return 3 + (getColumnInfo().getPrecision() + 2) / 2;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new ObjectDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof ObjectDynamicArray);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ObjectDynamicArray) aArray).put(mBigDecimalValue);
    }

    public void storeTo()
    {
        mValues.add(mBigDecimalValue);
    }

    /* BUG-38332  jdbc failed Batch update exception occurred: [Batch 1]:Invalid size of data to bind to a host variable */
    public void storeTo(ListBufferHandle aBufferWriter) throws SQLException
    {
        prepareToWrite(aBufferWriter);
        writeTo(aBufferWriter);
    }
    
    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        /* BUG-38993 In some situation 'BatchUpdateException' occurs in case of Oracle to Altibase migration via Migration Center */
        return 1 + mSize;
    }

    /**
     * BUG-41061 BigDecimal 값을 signExponent와 mantissa로 변환한다. </br>
     * 이때 numeric data의 overflow여부도 체크한다. 
     */
    private void makeSignExponentAndMantissa()
    {
        /* BUG-38322 setBigDemical method error in JDBC */
        if (isNullValueSet())
        {
            mSize = 0;
        }
        else
        {
            // BUG-41592 unscale한 값으로 Zero값인지 여부를 판단한다.
            if (mBigDecimalValue.unscaledValue().equals((ZERO_VALUE)))
            {
                mSignExponent = SIGN_PLUS_FLAG;
                mMantissa = NULL_VALUE_MANTISSA;
            }
            else
            {
                BigInteger sUnscaledValue = mBigDecimalValue.unscaledValue();
                int sSign = (sUnscaledValue.signum() < 0) ? SIGN_MINUS : SIGN_PLUS;
                if (sSign == SIGN_MINUS)
                {
                    sUnscaledValue = sUnscaledValue.negate();
                }
                byte[] sHexMantisa = sUnscaledValue.toByteArray();
                AltibaseNumeric sSrcNum = new AltibaseNumeric(256, sHexMantisa, sHexMantisa.length);
                AltibaseNumeric sDstNum = new AltibaseNumeric(100, sSrcNum);
                
                int sScale = getEvenScale(sDstNum);
                
                byte[] sDstNumByteArry = sDstNum.toByteArray();
                int sDstNumSize = sDstNum.size();
                
                // BUG-41399 자리끝에 0이 있을경우 scale값을 빼준다. 
                for (int i = sDstNumSize - 1; (i >= 0) && (sDstNumByteArry[i] == 0); i--)
                {
                    sDstNumSize -= 1;
                    sScale -= 2;
                }
                
                int sPrecision = getPrecision(sDstNumByteArry, sDstNumSize);

                if (sSign == SIGN_MINUS)
                {
                    sDstNum.negate();
                }
                
                checkPrecisionOverflow(sPrecision);
                checkScaleOverflow(sScale);
                
                // BUG-39326 SignExponent로 자릿수를 표현하므로 뒷자리 0(음수일땐 99)은 뺄 수 있다.
                // 그런데, 서버에서는 저장공간을 줄이기위해 항상 trim 하므로 올바른 비교를위해 trim한다.
                mMantissa = sDstNum.toTrimmedByteArray(sDstNumSize);
                int sSignExponent = 64 + (sSign * (sDstNumSize - (sScale / 2)));
                checkSignExponentOverflow(sSignExponent);
                mSignExponent = (byte)sSignExponent;
                
                if (sSign == SIGN_PLUS)
                {
                    mSignExponent |= SIGN_PLUS_FLAG;
                }
            }

            mSize = (short)(mMantissa.length + 1);
        }
    }

    /**
     * 100진수 배열값을 참조해 precision값을 돌려준다.</br>
     * 이때 첫번째 100진수 배열값이 10보다 작을때 precision값을 1 차감하고 </br>
     * 또한 aDstNumSize - 1의 배열값이 10의 배수일 때도 precision값을 1 차감한다.
     * @param aDstNumByteArry 100진수 바이트 배열
     * @param aDstNumSize 100진수 배열의 mantissa 길이
     * @return precision 값
     */
    private int getPrecision(byte[] aDstNumByteArry, int aDstNumSize)
    {
        int sPrecision = aDstNumSize * 2;

        if (aDstNumByteArry[0] < 10)
        {
            sPrecision--;
        }

        if ((aDstNumByteArry[aDstNumSize - 1] % 10) == 0)
        {
            sPrecision--;
        }
        return sPrecision;
    }

    /**
     * scale값을 돌려준다. 이때 scale이 양수고 2의배수가 아니라면 scale에 1을 더한다.
     * 
     * @param aDstNum 100진수로 변환한 AltibaseNumeric값
     * @return scale 값
     */
    private int getEvenScale(AltibaseNumeric aDstNum)
    {
        int sScale = mBigDecimalValue.scale();

        if ((sScale % 2) == 1)
        {
            aDstNum.multiply(10);
            sScale++;
        }
        else if ((sScale % 2) == -1)
        {
            aDstNum.multiply(10);
        }

        return sScale;
    }

    protected void checkPrecisionOverflow(int aPrecision)
    {
        if (aPrecision < NUMERIC_PRECISION_MINIMUM || aPrecision > NUMERIC_PRECISION_MAXIMUM)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "Precision",
                                                NUMERIC_PRECISION_MINIMUM + " ~ " + NUMERIC_PRECISION_MAXIMUM,
                                                String.valueOf(aPrecision));
        }
    }
    
    // BUG-41061 float타입이 아닌경우에는 scale의 최소값과 최대값을 이용하여 비교한다.
    protected void checkScaleOverflow(int aScale)
    {
        if (aScale < NUMERIC_SCALE_MINIMUM || aScale > NUMERIC_SCALE_MAXIMUM)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "Scale",
                                                NUMERIC_SCALE_MINIMUM + " ~ " + NUMERIC_SCALE_MAXIMUM,
                                                String.valueOf(aScale));
        }
    }

    private void checkSignExponentOverflow(int aSignExponent)
    {
        // BUG-41061 signExponent의 값이 127보다 클 경우 오버플로우 처리한다.
        if (aSignExponent < NUMERIC_SIGN_EXPONENT_MINIMUM || aSignExponent > NUMERIC_SIGN_EXPONENT_MAXIMUM)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "signexponent",
                                                NUMERIC_SIGN_EXPONENT_MINIMUM + " ~ " + NUMERIC_SIGN_EXPONENT_MAXIMUM,
                                                String.valueOf(aSignExponent));
        }
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeByte((byte) mSize);
        /* BUG-38993 In some situation 'BatchUpdateException' occurs in case of Oracle to Altibase migration via Migration Center */
        if( mSize > 0 )
        {
            aBufferWriter.writeByte(mSignExponent);
            aBufferWriter.writeBytes(mMantissa);
        }
        return 1 + mSize;
    }

    protected void setNullValue()
    {
        mBigDecimalValue = NULL_VALUE;

        mSize = 0;
        mSignExponent = 0;
        mMantissa = NULL_VALUE_MANTISSA;
    }
    
    protected boolean isNullValueSet()
    {
        return mBigDecimalValue.equals(NULL_VALUE);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        this.mBigDecimalValue = readBigDecimal(aChannel);
    }

    private BigDecimal readBigDecimal(CmChannel aChannel) throws SQLException
    {
        BigDecimal sBigDecimalValue;
        short sSize = aChannel.readUnsignedByte();
        if (sSize == 0)
        {
            sBigDecimalValue = NULL_VALUE;
        }
        else
        {
            int sMantissaLen = sSize - 1;
            byte sSignExponent = aChannel.readByte();
            int sSign = (sSignExponent & SIGN_PLUS_FLAG) == 0 ? SIGN_MINUS : SIGN_PLUS;
            short sExponent = (short)(sSignExponent & ~SIGN_PLUS_FLAG);

            // 0인 경우
            if (sMantissaLen == 0 || sExponent == 0)
            {
                aChannel.skip(sMantissaLen);
                sBigDecimalValue = new BigDecimal((double)0);
            }
            else
            {
                int sScale4BigDecimal = ((sMantissaLen) - (sExponent - 64) * sSign) * 2;

                byte[] sMantissa = new byte[sMantissaLen + 1];
                aChannel.readBytes(sMantissa, sMantissaLen);

                byte[] x = new byte[sMantissaLen + 1];
                int z = 0;
                int pos = 0;
                int product;
                int carry;

                for (int l = sMantissaLen; l > 0; l--)
                {
                    // Multiply x array times word y
                    // in place, and add word z
                    z = ((sSign == SIGN_MINUS) ? (99 - sMantissa[pos++]) : sMantissa[pos++]);

                    // Perform the multiplication word by word
                    carry = 0;

                    for (int i = sMantissaLen; i >= l; i--)
                    {
                        product = RADIX10BYTE * (x[i] & 0xFF) + carry;
                        x[i] = (byte)product;
                        carry = product >>> 8;
                    }

                    // Perform the addition
                    int sum = (x[sMantissaLen] & 0xFF) + z;
                    x[sMantissaLen] = (byte)sum;
                    carry = sum >>> 8;

                    for (int i = sSize - 2; i >= l; i--)
                    {
                        sum = (x[i] & 0xFF) + carry;
                        x[i] = (byte)sum;
                        carry = sum >>> 8;
                    }
                }

                BigInteger sBigInt = new BigInteger((sSign == SIGN_MINUS) ? -1 : 1, x);

                // BUG-13178 last '0' fix
                if ((z % 10) == 0)
                {
                    sBigInt = sBigInt.divide(DIV);
                    sScale4BigDecimal--;
                }

                sBigDecimalValue = (new BigDecimal(sBigInt)).movePointLeft(sScale4BigDecimal);
            }
        }

        return sBigDecimalValue;
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((ObjectDynamicArray)aArray).put(readBigDecimal(aChannel));
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(readBigDecimal(aChannel));
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        mBigDecimalValue = (BigDecimal) ((ObjectDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mBigDecimalValue = (BigDecimal)mValues.get(aLoadIndex);
    }

    protected boolean getBooleanSub() throws SQLException
    {
        return mBigDecimalValue.intValue() != 0;
    }

    protected byte getByteSub() throws SQLException
    {
        return mBigDecimalValue.byteValue();
    }

    /*
     * 이 메소드의 스펙은 다음과 같다.
     * 
     * 총 byte[]의 길이: 3+n (n = mantissa.length)
     * [0]: sign
     * [1]~[2]: scale
     * [3]~[n+2]: mantissa
     */
    protected byte[] getBytesSub() throws SQLException
    {
        byte[] sMantissa = mBigDecimalValue.unscaledValue().toByteArray();
        ByteBuffer sBuf = ByteBuffer.allocate(3 + sMantissa.length);
        sBuf.put((byte)mBigDecimalValue.signum());
        sBuf.putShort((short)mBigDecimalValue.scale());
        sBuf.put(sMantissa);
        return sBuf.array();
    }

    protected short getShortSub() throws SQLException
    {
        return mBigDecimalValue.shortValue();
    }

    protected int getIntSub() throws SQLException
    {
        return mBigDecimalValue.intValue();
    }

    protected long getLongSub() throws SQLException
    {
        return mBigDecimalValue.longValue();
    }

    protected float getFloatSub() throws SQLException
    {
        return mBigDecimalValue.floatValue();
    }

    protected double getDoubleSub() throws SQLException
    {
        return mBigDecimalValue.doubleValue();
    }

    protected BigDecimal getBigDecimalSub() throws SQLException
    {
        return mBigDecimalValue;
    }

    protected String getStringSub() throws SQLException
    {
        return mBigDecimalValue.toString();
    }

    protected InputStream getAsciiStreamSub() throws SQLException
    {
        return new ByteArrayInputStream(getStringSub().getBytes());        
    }
    
    protected InputStream getBinaryStreamSub() throws SQLException
    {
        return new ByteArrayInputStream(getBytesSub()); 
    }
    
    protected Reader getCharacterStreamSub() throws SQLException
    {
        return new StringReader(getStringSub());
    }

    protected Object getObjectSub() throws SQLException
    {
        return mBigDecimalValue;
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof CommonNumericColumn)
        {
            CommonNumericColumn sColumn = (CommonNumericColumn)aValue;
            mBigDecimalValue = sColumn.mBigDecimalValue;
        }
        else if (aValue instanceof String) {
            mBigDecimalValue = new BigDecimal(aValue.toString());            
        }
        else if (aValue instanceof BigDecimal) {
            mBigDecimalValue = (BigDecimal)aValue;            
        }
        else if (aValue instanceof Integer) {
            mBigDecimalValue = new BigDecimal(((Integer)aValue).doubleValue());
        }
        else if (aValue instanceof Long) {
            mBigDecimalValue = new BigDecimal(((Long)aValue).doubleValue());
        }
        else if (aValue instanceof Float) {
            mBigDecimalValue = new BigDecimal(((Float)aValue).doubleValue());
        }
        else if (aValue instanceof Double) {
            mBigDecimalValue = new BigDecimal((Double)aValue);
        }
        else if (aValue instanceof BigInteger)
        {
            mBigDecimalValue = new BigDecimal(((BigInteger)aValue), 0);            
        }        
        else if (aValue instanceof Boolean) {
            Boolean aBoolValue = (Boolean)aValue;
            if (aBoolValue.equals(Boolean.TRUE)) {
                mBigDecimalValue = new BigDecimal("1");
            }
            else
            {
                mBigDecimalValue = new BigDecimal("0");
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
        
        // BUG-41061 overflow를 체크하기 위해 setValue했을 때 signExponent와 mantissa값을 생성한다.
        makeSignExponentAndMantissa();
    }
}
