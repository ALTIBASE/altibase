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

public class TinyIntColumn extends AbstractColumn
{
    private static final byte NULL_VALUE = (byte) 0x80;
    private static final int  TINYINT_BYTE_SIZE = 1;
    private byte              mByteValue = 0;

    TinyIntColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.OTHER);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.NONE;
    }

    public String getDBColumnTypeName()
    {
        return "TINYINT";
    }

    public String getObjectClassName()
    {
        return Byte.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return String.valueOf(Byte.MIN_VALUE).length();
    }

    public int getOctetLength()
    {
        return TINYINT_BYTE_SIZE;
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return TINYINT_BYTE_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeByte(mByteValue);

        return TINYINT_BYTE_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte) 0, 0, 0);
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new ByteDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof ByteDynamicArray);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ByteDynamicArray) aArray).put(mByteValue);
    }

    public void storeTo()
    {
        mValues.add(mByteValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mByteValue = aChannel.readByte();
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
        mByteValue = ((ByteDynamicArray) aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mByteValue = (Byte)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
        mByteValue = NULL_VALUE;
    }

    protected boolean isNullValueSet()
    {
        return mByteValue == NULL_VALUE;
    }

    protected boolean getBooleanSub() throws SQLException
    {
        return mByteValue != 0;
    }

    protected byte getByteSub() throws SQLException
    {
        return mByteValue;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        return new byte[] { mByteValue };
    }

    protected short getShortSub() throws SQLException
    {
        return mByteValue;
    }

    protected int getIntSub() throws SQLException
    {
        return mByteValue;
    }

    protected long getLongSub() throws SQLException
    {
        return mByteValue;
    }

    protected float getFloatSub() throws SQLException
    {
        return mByteValue;
    }

    protected double getDoubleSub() throws SQLException
    {
        return mByteValue;
    }

    protected BigDecimal getBigDecimalSub() throws SQLException
    {
        return new BigDecimal((double)mByteValue);
    }

    protected String getStringSub() throws SQLException
    {
        return String.valueOf(mByteValue);
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
        return (short)mByteValue;
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof TinyIntColumn)
        {
            mByteValue = ((TinyIntColumn)aValue).mByteValue;
        }
        else if (aValue instanceof Number)
        {
            mByteValue = ((Number) aValue).byteValue();
        }
        else if (aValue instanceof String)
        {
            mByteValue = Short.valueOf(aValue.toString()).byteValue();
        }
        else if (aValue instanceof Boolean)
        {
            boolean sBoolValue = (Boolean)aValue;

            if (sBoolValue)
            {
                mByteValue = 1;
            }
            else
            {
                mByteValue = 0;
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
