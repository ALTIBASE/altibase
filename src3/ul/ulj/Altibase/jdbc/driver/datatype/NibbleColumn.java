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
import java.nio.ByteBuffer;
import java.sql.SQLException;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.ByteUtils;

public class NibbleColumn extends AbstractBinaryColumn
{
    private static final short NULL_VALUE_LENGTH = (short)0xFF;

    NibbleColumn()
    {
        super(LENGTH_SIZE_INT8);
        addMappedJdbcTypeSet(AltibaseTypes.NIBBLE);
    }

    NibbleColumn(int aPrecision)
    {
        super(LENGTH_SIZE_INT8, aPrecision);
    }

    protected int toByteLength(int aNibbleLength)
    {
        return (aNibbleLength + 1) >> 1;
    }

    protected int nullLength()
    {
        return NULL_VALUE_LENGTH;
    }

    public int getDBColumnType()
    {
        return ColumnTypes.NIBBLE;
    }

    public String getDBColumnTypeName()
    {
        return "NIBBLE";
    }

    public String getObjectClassName()
    {
        return (new byte[0]).getClass().getName();
    }

    public int getMaxDisplaySize()
    {
        return getColumnInfo().getPrecision();
    }

    private int getReturnLength()
    {
        int sLength = toByteLength(mLength);
        if (getMaxBinaryLength() > 0 && getMaxBinaryLength() < sLength)
        {
            sLength = getMaxBinaryLength();
        }
        return sLength;
    }

    /**
     * @return byte 값
     * @exception nibble stream의 길이가 2가 아닌 경우
     */
    protected byte getByteSub() throws SQLException
    {
        if (mLength != 2)
        {
            Error.throwSQLException(ErrorDef.INVALID_DATA_CONVERSION, getStringSub(), "byte");
        }
        mByteBuffer.rewind();
        return mByteBuffer.get();
    }

    /**
     * nibble을 byte[]로 변환해 반환한다.
     * 이 때 한 byte는 2개의 nibble 값을 포함한다.
     */
    protected byte[] getBytesSub() throws SQLException
    {
        mByteBuffer.rewind();
        byte[] sBytes = new byte[getReturnLength()];
        mByteBuffer.get(sBytes);
        return sBytes;
    }

    protected String getStringSub() throws SQLException
    {
        mByteBuffer.rewind();
        int sBinLen = getReturnLength();
        String sHexStr = ByteUtils.toHexString(mByteBuffer, 0, sBinLen);
        return sHexStr.substring(0, Math.min(sBinLen * 2, mLength));
    }

    protected InputStream getBinaryStreamSub() throws SQLException
    {
        byte[] sBytes = getBytesSub();
        return new ByteArrayInputStream(sBytes);
    }

    protected Object getObjectSub() throws SQLException
    {
        return getBytes();
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof NibbleColumn)
        {
            NibbleColumn sColumn = (NibbleColumn)aValue;

            sColumn.mByteBuffer.rewind();
            ensureAlloc(sColumn.mByteBuffer.limit());
            mByteBuffer.put(sColumn.mByteBuffer);
            mByteBuffer.flip();
            mLength = sColumn.mLength;

            return;
        }

        byte[] sByteArray = null;
        int sNibbleLength = 0;

        if (aValue instanceof String) // hex string
        {
            String sStrValue = (String)aValue;
            sByteArray = ByteUtils.parseByteArray(sStrValue);
            sNibbleLength = sStrValue.length();
        }
        else if (aValue instanceof byte[]) // byte array
        {
            sByteArray = (byte[])aValue;
            sNibbleLength = sByteArray.length * 2;
            if ((sByteArray[sByteArray.length - 1] & 0x0F) == 0)
            {
                sNibbleLength--; // trimed length
            }
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }

        if (sNibbleLength > ColumnConst.MAX_NIBBLE_LENGTH)
        {
            Error.throwSQLException(ErrorDef.VALUE_LENGTH_EXCEEDS,
                                    String.valueOf(sNibbleLength),
                                    String.valueOf(ColumnConst.MAX_NIBBLE_LENGTH));
        }

        if (mByteBuffer.capacity() < sByteArray.length)
        {
            mByteBuffer = ByteBuffer.wrap(sByteArray);
        }
        else
        {
            mByteBuffer.clear();
            mByteBuffer.put(sByteArray);
            mByteBuffer.flip();
        }
        mLength = sNibbleLength;
    }
}
