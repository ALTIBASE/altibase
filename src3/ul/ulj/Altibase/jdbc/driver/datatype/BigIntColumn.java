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
import Altibase.jdbc.driver.util.LongDynamicArray;

public class BigIntColumn extends AbstractColumn
{
    public static final long NULL_VALUE = (long)0x8000000000000000L;
    private static final int LONG_BYTE_SIZE = 8;
    
    private long mLongValue = 0;

    BigIntColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.BIGINT);
    }

    public BigIntColumn(long aValue)
    {
        mLongValue = aValue;
    }

    public int getDBColumnType()
    {
        return ColumnTypes.BIGINT;
    }

    public String getDBColumnTypeName()
    {
        return "BIGINT";
    }
    
    public String getObjectClassName()
    {
        return Long.class.getName();
    }
    
    public int getMaxDisplaySize()
    {
        return String.valueOf(Integer.MIN_VALUE).length();
    }

    public int getOctetLength()
    {
        return LONG_BYTE_SIZE;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new LongDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof LongDynamicArray);
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return LONG_BYTE_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeLong(mLongValue);

        return LONG_BYTE_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }
    
    public void storeTo(DynamicArray aArray)
    {
        ((LongDynamicArray) aArray).put(mLongValue);
    }

    public void storeTo()
    {
        mValues.add(mLongValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mLongValue = aChannel.readLong();
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((LongDynamicArray)aArray).put(aChannel.readLong());
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(aChannel.readLong());
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        mLongValue = ((LongDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mLongValue = (Long)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
        mLongValue = NULL_VALUE;
    }
    
    protected boolean isNullValueSet()
    {
        return mLongValue == NULL_VALUE;
    }
    
    protected boolean getBooleanSub()
    {
        return mLongValue != 0;
    }

    protected byte getByteSub()
    {
        return (byte) mLongValue;
    }

    protected byte[] getBytesSub()
    {
        ByteBuffer sBuf = ByteBuffer.allocate(LONG_BYTE_SIZE);
        sBuf.putLong(mLongValue);
        return sBuf.array();
    }

    protected short getShortSub()
    {
        return (short) mLongValue;
    }

    protected int getIntSub()
    {
        return (int) mLongValue;
    }

    protected long getLongSub()
    {
        return mLongValue;
    }

    protected float getFloatSub()
    {
        return mLongValue;
    }

    protected double getDoubleSub()
    {
        return mLongValue;
    }

    protected BigDecimal getBigDecimalSub()
    {
        return BigDecimal.valueOf(mLongValue);  /* BUG-44443 */
    }

    protected String getStringSub()
    {
        return String.valueOf(mLongValue);
    }

    protected InputStream getAsciiStreamSub()
    {
        return new ByteArrayInputStream(getStringSub().getBytes());        
    }
    
    protected InputStream getBinaryStreamSub()
    {
        return new ByteArrayInputStream(getBytesSub()); 
    }
    
    protected Reader getCharacterStreamSub()
    {
        return new StringReader(getStringSub());
    }

    protected Object getObjectSub()
    {
        return mLongValue;
    }

    public void setTypedValue(long aValue)
    {
        mLongValue = aValue;
        setNullOrNotNull();
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof BigIntColumn)
        {
            mLongValue = ((BigIntColumn)aValue).mLongValue;
        }
        else if (aValue instanceof String)
        {
            mLongValue = Long.parseLong((String)aValue);
        }
        else if (aValue instanceof Number)
        {
            mLongValue = ((Number)aValue).longValue();
        }
        else if (aValue instanceof Boolean)
        {
            boolean sBoolValue = (Boolean)aValue;
            if (sBoolValue) {
               mLongValue = 1; 
            }
            else
            {
                mLongValue = 0;
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
