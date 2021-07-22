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

import java.sql.SQLException;

import Altibase.jdbc.driver.AltibaseInterval;
import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.ObjectDynamicArray;

public class IntervalColumn extends AbstractColumn
{
    static final int INTERVAL_OCTET_LENGTH = 16; // sizeof(cmtInterval)

    private long mSecond = 0;
    private long mNanos = 0;

    IntervalColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.INTERVAL);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.INTERVAL;
    }

    public String getDBColumnTypeName()
    {
        return "INTERVAL";
    }
    
    public String getObjectClassName()
    {
        return AltibaseInterval.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return String.valueOf(Long.MIN_VALUE).length() + 1 + "999999".length();
    }

    public int getOctetLength()
    {
        return INTERVAL_OCTET_LENGTH;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new ObjectDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof ObjectDynamicArray);
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return AltibaseInterval.BYTES_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        aBufferWriter.writeLong(mSecond);
        aBufferWriter.writeLong(mNanos / 1000);

        return AltibaseInterval.BYTES_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ObjectDynamicArray) aArray).put(new AltibaseInterval(mSecond, mNanos));
    }

    public void storeTo()
    {
        mValues.add(new AltibaseInterval(mSecond, mNanos));
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        AltibaseInterval sInterval = readInterval(aChannel);
        mSecond = sInterval.getSecond();
        mNanos = sInterval.getNanos();
    }

    private AltibaseInterval readInterval(CmChannel aChannel) throws SQLException
    {
        return new AltibaseInterval(aChannel.readLong(), aChannel.readLong() * 1000);
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((ObjectDynamicArray) aArray).put(readInterval(aChannel));
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(readInterval(aChannel));
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        AltibaseInterval sThis = (AltibaseInterval) ((ObjectDynamicArray) aArray).get();
        mSecond = sThis.getSecond();
        mNanos = sThis.getNanos();
    }

    protected void loadFromSub(int aLoadIndex)
    {
        AltibaseInterval sThis = (AltibaseInterval)(mValues.get(aLoadIndex));
        mSecond = sThis.getSecond();
        mNanos = sThis.getNanos();
    }

    protected void setNullValue()
    {
        mSecond = AltibaseInterval.NULL.getSecond();
        mNanos = AltibaseInterval.NULL.getNanos();
    }
    
    protected boolean isNullValueSet()
    {
        return (mSecond == AltibaseInterval.NULL.getSecond()) &&
               (mNanos == AltibaseInterval.NULL.getNanos());
    }

    protected byte[] getBytesSub() throws SQLException
    {
        return AltibaseInterval.toBytes(mSecond, mNanos);
    }

    protected long getLongSub() throws SQLException
    {
        // 하위 호환성
        return (long)getDoubleSub();
    }

    protected double getDoubleSub() throws SQLException
    {
        // 하위 호환성
        return AltibaseInterval.toNumberOfDays(mSecond, mNanos);
    }

    protected String getStringSub() throws SQLException
    {
        return AltibaseInterval.toString(mSecond, mNanos);
    }

    protected Object getObjectSub() throws SQLException
    {
        return new AltibaseInterval(mSecond, mNanos);
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof IntervalColumn)
        {
            IntervalColumn sColumn = (IntervalColumn)aValue;
            mSecond = sColumn.mSecond;
            mNanos = sColumn.mNanos;
        }
        else
        {
            // Interval 타입은 값을 서버로 보낼 일이 전혀 없다.
            Error.throwInternalError(ErrorDef.INVALID_METHOD_INVOCATION);
        }
    }
}
