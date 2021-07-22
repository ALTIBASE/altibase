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
import Altibase.jdbc.driver.util.ByteDynamicArray;
import Altibase.jdbc.driver.util.DynamicArray;

public class BooleanColumn extends AbstractColumn
{
    private static final byte NULL_VALUE = (byte)0x80;
    private static final int BOOLEAN_OCTET_LENGTH = 1;

    private byte mBooleanValue = 0;
    
    BooleanColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.BOOLEAN);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.BOOLEAN;
    }

    public String getDBColumnTypeName()
    {
        return "BOOLEAN";
    }
    
    public String getObjectClassName()
    {
        return Boolean.class.getName();
    }
    
    public int getMaxDisplaySize()
    {
        return Math.max(Boolean.toString(true).length(), Boolean.toString(false).length());
    }

    public int getOctetLength()
    {
        return BOOLEAN_OCTET_LENGTH;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new ByteDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof ByteDynamicArray);
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return 1;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeByte(mBooleanValue);

        return 1;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ByteDynamicArray) aArray).put(mBooleanValue);
    }

    public void storeTo()
    {
        mValues.add(mBooleanValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mBooleanValue = aChannel.readByte();
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((ByteDynamicArray)aArray).put(aChannel.readByte());
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(aChannel.readByte());
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        mBooleanValue = ((ByteDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mBooleanValue = (Byte)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
        mBooleanValue = NULL_VALUE;
    }
    
    protected boolean isNullValueSet()
    {
        return mBooleanValue == NULL_VALUE;
    }
    
    protected boolean getBooleanSub() throws SQLException
    {
        return (mBooleanValue == 1);
    }

    protected byte getByteSub() throws SQLException
    {
        return mBooleanValue;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        byte[] sResult = new byte[1];
        sResult[0] = getByteSub();
        return sResult;
    }

    protected short getShortSub() throws SQLException
    {
        return getByteSub();
    }

    protected int getIntSub() throws SQLException
    {
        return getByteSub();
    }

    protected long getLongSub() throws SQLException
    {
        return getByteSub();
    }

    protected float getFloatSub() throws SQLException
    {
        return getByteSub();
    }

    protected double getDoubleSub() throws SQLException
    {
        return getByteSub();
    }

    protected BigDecimal getBigDecimalSub() throws SQLException
    {
        return new BigDecimal((double)getByteSub());
    }

    protected String getStringSub() throws SQLException
    {
        return Boolean.toString(getBooleanSub());
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
        return getBooleanSub();
    }
    
    // BOOLEAN does not be supported
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof BooleanColumn)
        {
            mBooleanValue = ((BooleanColumn)aValue).mBooleanValue;
        }
        else if (aValue instanceof Boolean)
        {
            mBooleanValue = ((Boolean)aValue ? (byte)1 : (byte)0);
        }
        else if (aValue instanceof String)
        {
        	if(aValue.equals("1"))
        	{
        		mBooleanValue = (byte)1;
        	}
        	else if(aValue.equals("0"))
        	{
        		mBooleanValue = (byte)0;
        	}
        	else
        	{
        		mBooleanValue = (Boolean.valueOf(aValue.toString()) ? (byte)1 : (byte)0);
        	}
        }
        else if (aValue instanceof Number)
        {
            mBooleanValue = (((Number)aValue).intValue() == 0 ? (byte)0 : (byte)1);
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
