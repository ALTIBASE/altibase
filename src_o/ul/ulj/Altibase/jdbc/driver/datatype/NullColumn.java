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

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.Date;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.Calendar;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.util.ByteDynamicArray;
import Altibase.jdbc.driver.util.DynamicArray;

public class NullColumn extends AbstractColumn
{
    private static final int NULL_BYTE_SIZE = 1;
    private static final int NULL_OCTET_LENGTH = 1;

    private byte             mValue;

    NullColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.NULL);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.NULL;
    }

    public String getDBColumnTypeName()
    {
        return "NULL";
    }

    public String getObjectClassName()
    {
        return "NULL";
    }

    public int getMaxDisplaySize()
    {
        return "null".length();
    }

    public int getOctetLength()
    {
        return NULL_OCTET_LENGTH;
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
        aBufferWriter.writeByte(mValue);

        return NULL_BYTE_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ByteDynamicArray)aArray).put(mValue);
    }

    public void storeTo()
    {
        mValues.add(mValue);
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        mValue = aChannel.readByte();
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
        mValue = ((ByteDynamicArray)aArray).get();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        mValue = (Byte)mValues.get(aLoadIndex);
    }

    protected void setNullValue()
    {
    }

    protected boolean isNullValueSet()
    {
        return true;
    }

    protected boolean getBooleanSub()
    {
        return false;
    }

    protected byte getByteSub()
    {
        return 0;
    }

    protected byte[] getBytesSub()
    {
        return null;
    }

    protected short getShortSub()
    {
        return 0;
    }

    protected int getIntSub()
    {
        return 0;
    }

    protected long getLongSub()
    {
        return 0;
    }

    protected float getFloatSub()
    {
        return Float.NaN;
    }

    protected double getDoubleSub()
    {
        return Double.NaN;
    }

    protected BigDecimal getBigDecimalSub()
    {
        return null;
    }

    protected Date getDateSub(Calendar aCalendar)
    {
        return null;
    }

    protected Time getTimeSub(Calendar aCalendar)
    {
        return null;
    }

    protected Timestamp getTimestampSub(Calendar aCalendar)
    {
        return null;
    }

    protected String getStringSub()
    {
        return null;
    }

    protected InputStream getAsciiStreamSub()
    {
        return null;
    }

    protected InputStream getBinaryStreamSub()
    {
        return null;
    }

    protected Reader getCharacterStreamSub()
    {
        return null;
    }

    protected Blob getBlobSub()
    {
        return null;
    }

    protected Clob getClobSub()
    {
        return null;
    }

    protected Object getObjectSub()
    {
        return null;
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
    }
}
