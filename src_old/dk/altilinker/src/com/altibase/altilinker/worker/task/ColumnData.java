/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.worker.task;

import java.io.IOException;
import java.io.InputStream;
import java.io.Reader;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.sql.Blob;
import java.sql.Clob;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Timestamp;

import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.JdbcDataManager;
import com.altibase.altilinker.util.TraceLogger;

public abstract class ColumnData
{
    protected static TraceLogger mTraceLogger = TraceLogger.getInstance();
    
    protected static final int ColumnValueLengthFieldSize = 4;
    
    protected ResultSet mResultSet   = null;
    protected int       mColumnIndex = -1;
    
    public void close()
    {
    }

    public void setResultSet(ResultSet aResultSet, int aColumnIndex)
    {
        mResultSet   = aResultSet;
        mColumnIndex = aColumnIndex;
    }
    
    abstract byte write(ByteBuffer aWriteBuffer) throws SQLException;
    
    protected void writeColumnValueLength(ByteBuffer aWriteBuffer,
                                          int aColumnValueLength)
    {
        aWriteBuffer.putInt(aColumnValueLength);
    }
}

class SmallIntColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 2;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }
        
        short sValue = mResultSet.getShort(mColumnIndex);
        
        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);
            
            aWriteBuffer.putShort(sValue);
        }
        
        return ResultCode.Success;
    }
}

class IntegerColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 4;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }
        
        int sValue = mResultSet.getInt(mColumnIndex);
        
        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);

            aWriteBuffer.putInt(sValue);
        }
        
        return ResultCode.Success;
    }
}

class BigIntColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 8;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }
        
        long sValue = mResultSet.getLong(mColumnIndex);
        
        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);

            aWriteBuffer.putLong(sValue);
        }
        
        return ResultCode.Success;
    }
}

class RealColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 4;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }
        
        float sValue = mResultSet.getFloat(mColumnIndex);

        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);

            aWriteBuffer.putFloat(sValue);
        }
        
        return ResultCode.Success;
    }
}

class DoubleColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 8;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }
        
        double sValue = mResultSet.getDouble(mColumnIndex);
        
        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);

            aWriteBuffer.putDouble(sValue);
        }
        
        return ResultCode.Success;
    }
}

class DateColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 8;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }

        Timestamp sTimestamp = mResultSet.getTimestamp(mColumnIndex);

        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            JdbcDataManager.mtdDateType sMtdDateType =
                    JdbcDataManager.toMtdDateType(sTimestamp);
    
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);
    
            aWriteBuffer.putShort(sMtdDateType.mYear);
            aWriteBuffer.putShort(sMtdDateType.mMonDayHour);
            aWriteBuffer.putInt(sMtdDateType.mMinSecMic);
        }
        
        return ResultCode.Success;
    }
}

class TimeColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 8;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }

        Timestamp sTimestamp = mResultSet.getTimestamp(mColumnIndex);

        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            JdbcDataManager.mtdDateType sMtdDateType =
                    JdbcDataManager.toMtdDateType(sTimestamp);
    
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);
    
            aWriteBuffer.putShort(sMtdDateType.mYear);
            aWriteBuffer.putShort(sMtdDateType.mMonDayHour);
            aWriteBuffer.putInt(sMtdDateType.mMinSecMic);
        }
        
        return ResultCode.Success;
    }
}

class TimestampColumnData extends ColumnData
{
    private static final int ColumnValueFieldSize = 8;

    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + ColumnValueFieldSize))
        {
            return ResultCode.Fragmented;
        }

        Timestamp sTimestamp = mResultSet.getTimestamp(mColumnIndex);
        
        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
        }
        else
        {
            JdbcDataManager.mtdDateType sMtdDateType =
                    JdbcDataManager.toMtdDateType(sTimestamp);
    
            writeColumnValueLength(aWriteBuffer, ColumnValueFieldSize);
    
            aWriteBuffer.putShort(sMtdDateType.mYear);
            aWriteBuffer.putShort(sMtdDateType.mMonDayHour);
            aWriteBuffer.putInt(sMtdDateType.mMinSecMic);
        }
        
        return ResultCode.Success;
    }
}

class StringColumnData extends ColumnData
{
    private byte[] mValue       = null;
    private int    mValueLength = 0;
    
    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < ColumnValueLengthFieldSize)
        {
            return ResultCode.Fragmented;
        }

        if (mValue == null)
        {
            String sString = mResultSet.getString(mColumnIndex);
            
            if (mResultSet.wasNull() == true)
            {
                writeColumnValueLength(aWriteBuffer, -1);
                
                return ResultCode.Success;
            }
            else
            {
                mValue = JdbcDataManager.toDBCharSetString(sString);
                if (mValue == null)
                {
                    return ResultCode.Failed;
                }
                
                mValueLength = mValue.length;
            }
        }

        sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < (ColumnValueLengthFieldSize + mValueLength))
        {
            return ResultCode.Fragmented;
        }

        writeColumnValueLength(aWriteBuffer, mValueLength);
       
        aWriteBuffer.put(mValue, 0, mValueLength);
        
        return ResultCode.Success;
    }
}

abstract class InputStreamColumnData extends ColumnData
{
    private InputStream mInputStream = null;
    private byte[]      mBuffer      = null;
    private int         mOffset      = 0;
    private int         mLength      = 0;
    
    public void close()
    {
    }
    
    public byte write(ByteBuffer aWriteBuffer, InputStream aInputStream)
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < ColumnValueLengthFieldSize)
        {
            return ResultCode.Fragmented;
        }

        byte sResultCode = ResultCode.Success;
        
        try
        {
            if (mInputStream == null)
            {
                mInputStream = aInputStream;
                
                mLength = mInputStream.available();
                
                mBuffer = new byte[aWriteBuffer.limit()];
    
                writeColumnValueLength(aWriteBuffer, mLength);
            }
    
            sRemainedSize = aWriteBuffer.remaining();
            
            int sReadSize;
            if (sRemainedSize < (mLength - mOffset))
            {
                sReadSize = sRemainedSize;
                sResultCode = ResultCode.Fragmented;
            }
            else
            {
                sReadSize = mLength - mOffset;
            }
            
            int sLen;
            while ((sLen = mInputStream.read(mBuffer, 0, sReadSize)) != -1)
            {
                aWriteBuffer.put(mBuffer, 0, sLen);
                mOffset += sLen;
                
                if (sLen == sReadSize)
                {
                    break;
                }
                
                sReadSize -= sLen;
            }
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
            
            sResultCode = ResultCode.Failed;
        }
        
        return sResultCode;
    }
}

class BinaryColumnData extends InputStreamColumnData
{
    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        InputStream sInputStream = mResultSet.getBinaryStream(mColumnIndex);

        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
            
            return ResultCode.Success;
        }
        
        return super.write(aWriteBuffer, sInputStream);
    }
}

class BlobColumnData extends InputStreamColumnData
{
    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        Blob sBlob = mResultSet.getBlob(mColumnIndex);
        
        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
            
            return ResultCode.Success;
        }
        
        InputStream sInputStream = sBlob.getBinaryStream();
        
        return super.write(aWriteBuffer, sInputStream);
    }
}

class ClobColumnData extends ColumnData
{
    private static final int CharArraySize = 10240;

    private Reader         mReader     = null;
    private char[]         mCharArray  = null;
    private CharBuffer     mCharBuffer = null;
    private ByteBuffer     mBuffer     = null;
    private CharsetEncoder mEncoder    = null;
    private boolean        mEndOfRead  = false;
    
    public byte write(ByteBuffer aWriteBuffer) throws SQLException
    {
        int sRemainedSize = aWriteBuffer.remaining();
        
        if (sRemainedSize < ColumnValueLengthFieldSize)
        {
            return ResultCode.Fragmented;
        }
        
        Clob sClob = mResultSet.getClob(mColumnIndex);

        if (mResultSet.wasNull() == true)
        {
            writeColumnValueLength(aWriteBuffer, -1);
            
            return ResultCode.Success;
        }
        
        mReader = sClob.getCharacterStream();
        
        if (mCharArray == null)
        {
            Charset sCharset = 
                    Charset.forName(JdbcDataManager.getDBCharSetName());
            
            if (sCharset == null)
            {
                return ResultCode.Failed;
            }

            mCharArray = new char[CharArraySize];
            mBuffer    = ByteBuffer.allocateDirect(CharArraySize * 2);
            mEncoder   = sCharset.newEncoder();
            
            writeColumnValueLength(aWriteBuffer, (int)sClob.length());
        }

        if (mBuffer.remaining() > 0)
        {
            aWriteBuffer.put(mBuffer);
            
            if (aWriteBuffer.remaining() == 0)
            {
                return ResultCode.Fragmented;
            }
        }
        
        if (mEndOfRead == true)
        {
            return ResultCode.Success;
        }
        
        try
        {
            do
            {
                mBuffer.position(0);
                mBuffer.limit(mBuffer.capacity());
                
                int sCharLength = mReader.read(mCharArray);
                
                if (sCharLength == -1)
                {
                    mEndOfRead = true;
                }
                
                if (sCharLength > 0 || sCharLength == -1)
                {
                    mCharBuffer = CharBuffer.allocate(sCharLength);
                    mCharBuffer.put(mCharArray, 0, sCharLength);
                    
                    mCharBuffer.limit(mCharBuffer.position());
                    mCharBuffer.position(0);
                    
                    mEncoder.encode(mCharBuffer, mBuffer, mEndOfRead);

                    mBuffer.limit(mBuffer.position());
                    mBuffer.position(0);
                    
                    aWriteBuffer.put(mBuffer);
                    
                    if (aWriteBuffer.remaining() == 0)
                    {
                        return ResultCode.Fragmented;
                    }
                }
                else
                {
                    break;
                }
                
            } while (true);
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
            
            return ResultCode.Failed;
        }
        
        return ResultCode.Success;
    }
}

class ColumnDataFactory
{
    public static ColumnData newColumnData(int aSQLType)
    {
        ColumnData sNewColumnData = null;
        
        switch (aSQLType)
        {
        case AltibaseSQLType.SQL_CHAR:
        case AltibaseSQLType.SQL_VARCHAR:
        case AltibaseSQLType.SQL_DECIMAL:
        case AltibaseSQLType.SQL_NUMERIC:
        case AltibaseSQLType.SQL_FLOAT:
            sNewColumnData = new StringColumnData();
            break;
            
        case AltibaseSQLType.SQL_SMALLINT:
        case AltibaseSQLType.SQL_TINYINT:
            sNewColumnData = new SmallIntColumnData();
            break;
            
        case AltibaseSQLType.SQL_INTEGER:
            sNewColumnData = new IntegerColumnData();
            break;
            
        case AltibaseSQLType.SQL_REAL:
            sNewColumnData = new RealColumnData();
            break;
            
        case AltibaseSQLType.SQL_DOUBLE:
            sNewColumnData = new DoubleColumnData();
            break;
            
        case AltibaseSQLType.SQL_BIGINT:
            sNewColumnData = new BigIntColumnData();
            break;
            
        case AltibaseSQLType.SQL_DATE:
            sNewColumnData = new DateColumnData();
            break;
            
        case AltibaseSQLType.SQL_TIME:
            sNewColumnData = new TimeColumnData();
            break;
            
        case AltibaseSQLType.SQL_TIMESTAMP:
            sNewColumnData = new TimestampColumnData();
            break;
            
        case AltibaseSQLType.SQL_BINARY:
            sNewColumnData = new BinaryColumnData();
            break;
            
        case AltibaseSQLType.SQL_BLOB:
            sNewColumnData = new BlobColumnData();
            break;
            
        case AltibaseSQLType.SQL_CLOB:
            sNewColumnData = new ClobColumnData();
            break;
            
        default:
            sNewColumnData = null;
            break;
        }
        
        return sNewColumnData;
    }
}
