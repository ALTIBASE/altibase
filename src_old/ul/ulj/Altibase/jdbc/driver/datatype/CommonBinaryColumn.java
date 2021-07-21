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
import java.nio.ByteBuffer;
import java.sql.SQLException;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.ByteBufferInputStream;
import Altibase.jdbc.driver.util.ByteUtils;

abstract class CommonBinaryColumn extends AbstractBinaryColumn
{
    private ByteBufferInputStream mReusingInputStream = null;

    protected CommonBinaryColumn(int aLengthSize)
    {
        super(aLengthSize);
    }

    protected CommonBinaryColumn(int aLengthSize, int aPrecision)
    {
        super(aLengthSize, aPrecision);
    }

    protected int toByteLength(int aLength)
    {
        return aLength;
    }

    protected int nullLength()
    {
        return 0;
    }

    protected abstract int maxLength();

    public int getMaxDisplaySize()
    {
        // 한바이트당 두개의 문자로 출력된다. ex) F02A23D
        return getColumnInfo().getPrecision() * 2;
    }

    private int getReturnLength()
    {
        int sLength = mByteBuffer.remaining();
        if (getMaxBinaryLength() > 0 && getMaxBinaryLength() < sLength)
        {
            sLength = getMaxBinaryLength();
        }
        return sLength;
    }

    /**
     * @return byte 값
     * @exception binary stream이 1바이트를 넘을 경우
     */
    protected byte getByteSub() throws SQLException
    {
        byte sResult = 0;
        mByteBuffer.rewind();

        try
        {
            sResult = mByteBuffer.get();
        }
        catch (Exception sEx)
        {
            Error.throwSQLException(ErrorDef.INVALID_DATA_CONVERSION, getStringSub(), "byte");
        }
        if (mByteBuffer.hasRemaining())
        {
            Error.throwSQLException(ErrorDef.INVALID_DATA_CONVERSION, getStringSub(), "byte");
        }
        return sResult;
    }

    protected byte[] getBytesSub() throws SQLException
    {
        // 이 메소드는 mByteBuffer.array()를 호출해서는 안된다.
        // mByteBuffer는 precision만큼 capacity가 잡혀있기 때문에
        // array()를 호출하면 limit을 초과한 불필요한 데이터까지 반환한다.
        mByteBuffer.rewind();
        byte[] sResult = new byte[getReturnLength()];
        mByteBuffer.get(sResult);
        return sResult;
    }

    protected String getStringSub() throws SQLException
    {
        mByteBuffer.rewind();
		return ByteUtils.toHexString(mByteBuffer, 0, getReturnLength());
    }

    protected InputStream getBinaryStreamSub() throws SQLException
    {
        mByteBuffer.rewind();
        if (mReusingInputStream == null)
        {
            mReusingInputStream = new ByteBufferInputStream(mByteBuffer, 0, getReturnLength()); 
        }
        else
        {
            mReusingInputStream.reopen();
        }            
        return mReusingInputStream;         
    }

    protected Object getObjectSub() throws SQLException
    {
        return getBytes();
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
        if (this.getClass() == aValue.getClass())
        {
            CommonBinaryColumn sColumn = (CommonBinaryColumn)aValue;
            sColumn.mByteBuffer.rewind();
            ensureAlloc(sColumn.mByteBuffer.remaining());
            mByteBuffer.put(sColumn.mByteBuffer);
            mByteBuffer.flip();
            mLength = sColumn.mLength;
        }
        else
        {
            byte[] sSource = null;
            if (aValue instanceof String)
            {
                sSource = ByteUtils.parseByteArray((String)aValue);
            }
            else if (aValue instanceof byte[])
            {
                sSource = (byte[])((byte[])aValue).clone();
            }
            else
            {
                Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                        aValue.getClass().getName(), getDBColumnTypeName());
            }

            if (sSource.length > maxLength())
            {
                Error.throwSQLException(ErrorDef.VALUE_LENGTH_EXCEEDS,
                                        String.valueOf(mLength),
                                        String.valueOf(maxLength()));
            }

            if (mByteBuffer.capacity() < sSource.length)
            {
                mByteBuffer = ByteBuffer.wrap(sSource);
            }
            else
            {
                mByteBuffer.clear();
                mByteBuffer.put(sSource);
                mByteBuffer.flip();
            }
            mLength = sSource.length;
        }
        if (mLength > getColumnInfo().getPrecision())
        {
            getColumnInfo().modifyPrecision(mLength);
        }
    }
}
