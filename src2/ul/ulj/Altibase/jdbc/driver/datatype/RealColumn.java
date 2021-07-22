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
import java.nio.ByteBuffer;
import java.sql.SQLException;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.FloatDynamicArray;

public class RealColumn extends AbstractColumn
{
    private static final float NULL_VALUE = Float.NaN;
    private static final int FLOAT_BYTE_SIZE = 4;

    private float mFloatValue = 0;

    RealColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.REAL);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.REAL;
    }

    public String getDBColumnTypeName()
    {
        return "REAL";
    }
    
    public String getObjectClassName()
    {
        return Float.class.getName();
    }

    public int getMaxDisplaySize()
    {
        // 예전 JDBC 드라이버 소스로부터...
        return 10;
    }

    public int getOctetLength()
    {
        return FLOAT_BYTE_SIZE;
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return FLOAT_BYTE_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeFloat(mFloatValue);

        return FLOAT_BYTE_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new FloatDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof FloatDynamicArray);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((FloatDynamicArray) aArray).put(mFloatValue);
    }

    public void storeTo()
    {
        mValues.add(mFloatValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mFloatValue = aChannel.readFloat();
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((FloatDynamicArray)aArray).put(aChannel.readFloat());
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(aChannel.readFloat());
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        mFloatValue = ((FloatDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mFloatValue = (Float)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
        mFloatValue = NULL_VALUE;
    }
    
    protected boolean isNullValueSet()
    {
        return Float.isNaN(mFloatValue);
    }
    
    protected boolean getBooleanSub() throws SQLException
    {
        return mFloatValue != 0;
    }

    protected byte getByteSub() throws SQLException
    {
        return (byte) mFloatValue;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        ByteBuffer sBuf = ByteBuffer.allocate(FLOAT_BYTE_SIZE);
        sBuf.putFloat(mFloatValue);
        return sBuf.array();
    }

    protected short getShortSub() throws SQLException
    {
        return (short) mFloatValue;
    }

    protected int getIntSub() throws SQLException
    {
        return (int) mFloatValue;
    }

    protected long getLongSub() throws SQLException
    {
        return (long) mFloatValue;
    }

    protected float getFloatSub() throws SQLException
    {
        return mFloatValue;
    }

    protected double getDoubleSub() throws SQLException
    {
        return mFloatValue;
    }

    protected BigDecimal getBigDecimalSub() throws SQLException
    {
        // 정수 값이면 소수점은 떼고 반환. ex) 3.0 ==> 3
        if (Float.compare(mFloatValue, (long)mFloatValue) == 0)
        {
            return new BigDecimal(Long.toString((long)mFloatValue));
        }
        return new BigDecimal(Float.toString(mFloatValue));
    }

    protected String getStringSub() throws SQLException
    {
        return String.valueOf(mFloatValue);
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
        return mFloatValue;
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof RealColumn)
        {
            mFloatValue = ((RealColumn)aValue).mFloatValue;
        }
        else if (aValue instanceof Number) 
        {
            mFloatValue = ((Number)aValue).floatValue();
        }
        else if (aValue instanceof String)
        {
            mFloatValue = Float.valueOf(aValue.toString());
        }
        else if (aValue instanceof Boolean)
        {
            boolean sBoolValue = (Boolean)aValue;
            
            if (sBoolValue) 
            {
                mFloatValue = 1;
            }
            else
            {
                mFloatValue = 0;
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
