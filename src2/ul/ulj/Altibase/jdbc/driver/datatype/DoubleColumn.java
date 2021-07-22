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
import Altibase.jdbc.driver.util.DoubleDynamicArray;
import Altibase.jdbc.driver.util.DynamicArray;

public class DoubleColumn extends AbstractColumn
{
    private static final double NULL_VALUE = Double.NaN;
    private static final int DOUBLE_BYTE_SIZE = 8;

    private double mDoubleValue = 0;

    DoubleColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.DOUBLE);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.DOUBLE;
    }

    public String getDBColumnTypeName()
    {
        return "DOUBLE";
    }
    
    public String getObjectClassName()
    {
        return Double.class.getName();
    }

    public int getMaxDisplaySize()
    {
        // BUGBUG 예전 JDBC 드라이버 소스로부터...
        return 20;
    }

    public int getOctetLength()
    {
        return DOUBLE_BYTE_SIZE;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new DoubleDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof DoubleDynamicArray);
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return DOUBLE_BYTE_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeDouble(mDoubleValue);

        return DOUBLE_BYTE_SIZE;
    }
    
    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((DoubleDynamicArray) aArray).put(mDoubleValue);
    }

    public void storeTo()
    {
        mValues.add(mDoubleValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mDoubleValue = aChannel.readDouble();
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((DoubleDynamicArray)aArray).put(aChannel.readDouble());
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(aChannel.readDouble());
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        mDoubleValue = ((DoubleDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mDoubleValue = (Double)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
        mDoubleValue = NULL_VALUE;
    }
    
    protected boolean isNullValueSet()
    {
        return Double.isNaN(mDoubleValue);
    }
    
    protected boolean getBooleanSub() throws SQLException
    {
        return mDoubleValue != 0;
    }

    protected byte getByteSub() throws SQLException
    {
        return (byte) mDoubleValue;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        ByteBuffer sBuf = ByteBuffer.allocate(DOUBLE_BYTE_SIZE);
        sBuf.putDouble(mDoubleValue);
        return sBuf.array();
    }

    protected short getShortSub() throws SQLException
    {
        return (short) mDoubleValue;
    }

    protected int getIntSub() throws SQLException
    {
        return (int) mDoubleValue;
    }

    protected long getLongSub() throws SQLException
    {
        return (long) mDoubleValue;
    }

    protected float getFloatSub() throws SQLException
    {
        return (float) mDoubleValue;
    }

    protected double getDoubleSub() throws SQLException
    {
        return mDoubleValue;
    }

    protected BigDecimal getBigDecimalSub() throws SQLException
    {
        return new BigDecimal(mDoubleValue);
    }

    protected String getStringSub() throws SQLException
    {
        return String.valueOf(mDoubleValue);
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
        return mDoubleValue;
    }
    
    public void setTypedValue(double aValue)
    {
        mDoubleValue = aValue;
        setNullOrNotNull();
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof DoubleColumn)
        {
            mDoubleValue = ((DoubleColumn)aValue).mDoubleValue;
        }
        else if (aValue instanceof Number) 
        {
            mDoubleValue = ((Number)aValue).doubleValue();
        }
        else if (aValue instanceof String)
        {
            mDoubleValue = Double.valueOf(aValue.toString());
        }
        else if (aValue instanceof Boolean)
        {
            boolean sBoolValue = (Boolean)aValue;
            
            if (sBoolValue) 
            {
                mDoubleValue = 1;
            }
            else
            {
                mDoubleValue = 0;
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
