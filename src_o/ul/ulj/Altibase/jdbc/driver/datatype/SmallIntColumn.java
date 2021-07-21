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
import java.sql.SQLException;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.ShortDynamicArray;

public class SmallIntColumn extends AbstractColumn
{
    public static final short NULL_VALUE = (short)0x8000;
    private static final int SMALLINT_BYTE_SIZE = 2;

    private short mShortValue = 0;

    SmallIntColumn()
    {
        // Altibase는 TINYINT를 지원하지 않는다. SMALLINT 타입으로 받아들인다.
        addMappedJdbcTypeSet(AltibaseTypes.SMALLINT);
        addMappedJdbcTypeSet(AltibaseTypes.TINYINT);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.SMALLINT;
    }

    public String getDBColumnTypeName()
    {
        return "SMALLINT";
    }
    
    public String getObjectClassName()
    {
        return Short.class.getName();
    }
    
    public int getMaxDisplaySize()
    {
        return String.valueOf(Short.MIN_VALUE).length();
    }

    public int getOctetLength()
    {
        return SMALLINT_BYTE_SIZE;
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return SMALLINT_BYTE_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeShort(mShortValue);

        return SMALLINT_BYTE_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new ShortDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof ShortDynamicArray);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ShortDynamicArray) aArray).put(mShortValue);
    }

    public void storeTo()
    {
        mValues.add(mShortValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mShortValue = aChannel.readShort();
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((ShortDynamicArray)aArray).put(aChannel.readShort());
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(aChannel.readShort());
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        mShortValue = ((ShortDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mShortValue = (Short)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
        mShortValue = NULL_VALUE;
    }
    
    protected boolean isNullValueSet()
    {
        return mShortValue == NULL_VALUE;
    }
    
    protected boolean getBooleanSub() throws SQLException
    {
        return mShortValue != 0;
    }

    protected byte getByteSub() throws SQLException
    {
        return (byte) mShortValue;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        byte[] x = new byte[2];
        x[0] = (byte) ((mShortValue & 0xFF00) >> 8);
        x[1] = (byte) (mShortValue & 0x00FF);
        return x;
    }

    protected short getShortSub() throws SQLException
    {
        return mShortValue;
    }

    protected int getIntSub() throws SQLException
    {
        return mShortValue;
    }

    protected long getLongSub() throws SQLException
    {
        return mShortValue;
    }

    protected float getFloatSub() throws SQLException
    {
        return mShortValue;
    }

    protected double getDoubleSub() throws SQLException
    {
        return mShortValue;
    }

    protected BigDecimal getBigDecimalSub() throws SQLException
    {
        return new BigDecimal((double)mShortValue);
    }

    protected String getStringSub() throws SQLException
    {
        return String.valueOf(mShortValue);
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
        return mShortValue;
    }
    
    public void setTypedValue(short aValue)
    {
        mShortValue = aValue;
        setNullOrNotNull();
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof SmallIntColumn)
        {
            mShortValue = ((SmallIntColumn)aValue).mShortValue;
        }
        else if (aValue instanceof Number) 
        {
            mShortValue = ((Number)aValue).shortValue();
        }
        else if (aValue instanceof String)
        {
            mShortValue = Short.valueOf(aValue.toString());
        }
        else if (aValue instanceof Boolean)
        {
            boolean sBoolValue = (Boolean)aValue;
            
            if (sBoolValue) 
            {
                mShortValue = 1;
            }
            else
            {
                mShortValue = 0;
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
