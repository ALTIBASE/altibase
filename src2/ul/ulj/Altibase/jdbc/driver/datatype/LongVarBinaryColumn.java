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

import java.nio.ByteBuffer;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class LongVarBinaryColumn extends CommonBinaryColumn
{
    public LongVarBinaryColumn()
    {
        super(LENGTH_SIZE_INT32);
        addMappedJdbcTypeSet(AltibaseTypes.LONGVARBINARY);
    }

    private int mWrittenByteLength = 0;

    public int getDBColumnType()
    {
        return ColumnTypes.BINARY;
    }

    public String getDBColumnTypeName()
    {
        return "LONGVARBINARY";
    }

    public String getObjectClassName()
    {
        return (new byte[0]).getClass().getName();
    }

    protected int maxLength()
    {
        return ColumnConst.MAX_BINARY_LENGTH;
    }

    public int getWrittenByteLength()
    {
        return mWrittenByteLength;
    }

    public void setWrittenByteLength(int aLength)
    {
        mWrittenByteLength = aLength;
    }

    public void setBinaryValue(byte[] aValue)
    {
        setBinaryValue(aValue, 0, 0);
    }

    public void setBinaryValue(byte[] aValue, int aOffset, int aLength)
    {
        mByteBuffer.clear();
        if (aValue != null)
        {
            if (aOffset < 0 || aOffset >= aValue.length)
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                    "Offset",
                                                    "0 ~ " + aValue.length,
                                                    String.valueOf(aOffset));
            }
            if (aLength > aValue.length)
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                    "Length",
                                                    "0 ~ " + aValue.length,
                                                    String.valueOf(aLength));
            }

            if (aLength == 0)
            {
                aLength = aValue.length - aOffset;
            }
            ensureAlloc(aLength);
            mByteBuffer.put(aValue, aOffset, aLength);
        }
        mByteBuffer.flip();
    }

    public void setBinaryValue(ByteBuffer aValue)
    {
        mByteBuffer.clear();
        mByteBuffer.put(aValue);
        mByteBuffer.flip();
    }
}
