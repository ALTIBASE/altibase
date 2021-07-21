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
import Altibase.jdbc.driver.util.IntDynamicArray;

public class IntegerColumn extends AbstractColumn
{
    public static final int NULL_VALUE = (int)0x80000000;
    private static final int INTEGER_BYTE_SIZE = 4;

    private int mIntValue = 0;

    IntegerColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.INTEGER);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.INTEGER;
    }

    public String getDBColumnTypeName()
    {
        return "INTEGER";
    }
    
    public String getObjectClassName()
    {
        return Integer.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return String.valueOf(Integer.MIN_VALUE).length();
    }

    public int getOctetLength()
    {
        return INTEGER_BYTE_SIZE;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new IntDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof IntDynamicArray);
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return INTEGER_BYTE_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeInt(mIntValue);

        return INTEGER_BYTE_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public void storeTo()
    {
        mValues.add(mIntValue);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((IntDynamicArray) aArray).put(mIntValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mIntValue = aChannel.readInt();
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(aChannel.readInt());
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((IntDynamicArray)aArray).put(aChannel.readInt());
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        mIntValue = ((IntDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mIntValue = (Integer)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
        mIntValue = NULL_VALUE;
    }
    
    protected boolean isNullValueSet()
    {
        return mIntValue == NULL_VALUE;
    }
    
    protected boolean getBooleanSub() throws SQLException
    {
        return mIntValue != 0;
    }

    protected byte getByteSub() throws SQLException
    {
        return (byte) mIntValue;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        ByteBuffer sBuf = ByteBuffer.allocate(INTEGER_BYTE_SIZE);
        sBuf.putInt(mIntValue);
        return sBuf.array();
    }

    protected short getShortSub() throws SQLException
    {
        return (short) mIntValue;
    }

    protected int getIntSub() throws SQLException
    {
        return mIntValue;
    }

    protected long getLongSub() throws SQLException
    {
        return mIntValue;
    }

    protected float getFloatSub() throws SQLException
    {
        return mIntValue;
    }

    protected double getDoubleSub() throws SQLException
    {
        return mIntValue;
    }

    protected BigDecimal getBigDecimalSub() throws SQLException
    {
        return new BigDecimal((double)mIntValue);
    }

    protected String getStringSub() throws SQLException
    {
        return String.valueOf(mIntValue);
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
        return mIntValue;
    }
    
    public void setTypedValue(int aValue)
    {
        mIntValue = aValue;
        setNullOrNotNull();
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof IntegerColumn)
        {
            mIntValue = ((IntegerColumn)aValue).mIntValue;
        }
        else if (aValue instanceof Number) 
        {
            mIntValue = ((Number)aValue).intValue();
        }
        else if (aValue instanceof String) 
        {
            mIntValue = Integer.parseInt((String)aValue);
        }
        else if (aValue instanceof Boolean)
        {
            boolean sBoolValue = (Boolean)aValue;
            
            if (sBoolValue) 
            {
                mIntValue = 1;
            }
            else
            {
                mIntValue = 0;
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
