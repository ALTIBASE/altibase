/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.core;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.net.URL;
import java.sql.*;
import java.util.Calendar;
import java.util.Map;

public class AltibaseShardingCallableStatement extends AbstractShardingCallableStatement
{
    private CallableStatement                 mServerSideCstmt;
    private InternalShardingCallableStatement mInternalCstmt;

    AltibaseShardingCallableStatement(AltibaseShardingConnection aConnection, String aSql,
                                      int aResultSetType, int aResultSetConcurrency,
                                      int aResultSetHoldability) throws SQLException
    {
        super(aConnection, aSql, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
        if (mShardStmtCtx.getShardAnalyzeResult().isCoordQuery())
        {
            mServerSideCstmt = aConnection.getMetaConnection().prepareCall(aSql, aResultSetType,
                                                                           aResultSetConcurrency,
                                                                           aResultSetHoldability);
            mInternalCstmt = new ServerSideShardingCallableStatement(mServerSideCstmt, mMetaConn);
        }
        else
        {

            mInternalCstmt = new DataNodeShardingCallableStatement(aConnection, aSql, aResultSetType,
                                                                   aResultSetConcurrency,
                                                                   aResultSetHoldability,
                                                                   this);
        }
        mInternalPstmt = mInternalCstmt;
        mInternalStmt = mInternalPstmt;
    }

    public void registerOutParameter(int aParameterIndex, int aSqlType) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class,
                                                  "registerOutParameter",
                                                  new Class[] { int.class, int.class },
                                                  new Object[] { aParameterIndex, aSqlType });
        if (mIsCoordQuery)
        {
            mServerSideCstmt.registerOutParameter(aParameterIndex, aSqlType);
        }
    }

    public void registerOutParameter(int aParameterIndex, int aSqlType, int aScale) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class,
                                                  "registerOutParameter",
                                                  new Class[] {int.class, int.class, int.class},
                                                  new Object[] { aParameterIndex, aSqlType, aScale});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.registerOutParameter(aParameterIndex, aSqlType, aScale);
        }
    }

    public void registerOutParameter(int aParameterIndex, int aSqlType, String aTypeName) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "registerOutParameter",
                                                  new Class[] {int.class, int.class, String.class},
                                                  new Object[] { aParameterIndex, aSqlType, aTypeName});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.registerOutParameter(aParameterIndex, aSqlType, aTypeName);
        }
    }

    public void registerOutParameter(String aParameterName, int aSqlType) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "registerOutParameter",
                                                  new Class[] {String.class, int.class},
                                                  new Object[] { aParameterName, aSqlType});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.registerOutParameter(aParameterName, aSqlType);
        }
    }

    public void registerOutParameter(String aParameterName, int aSqlType, int aScale) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "registerOutParameter",
                                                  new Class[] {String.class, int.class, int.class},
                                                  new Object[] { aParameterName, aSqlType, aScale});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.registerOutParameter(aParameterName, aSqlType, aScale);
        }
    }

    public void registerOutParameter(String aParameterName, int aSqlType, String aTypeName) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "registerOutParameter",
                                                  new Class[] {String.class, int.class, String.class},
                                                  new Object[] { aParameterName, aSqlType, aTypeName});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.registerOutParameter(aParameterName, aSqlType, aTypeName);
        }
    }

    public boolean wasNull() throws SQLException
    {
        return mInternalCstmt.wasNull();
    }

    public String getString(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getString(aParameterIndex);
    }

    public String getString(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getString(aParameterName);
    }

    public boolean getBoolean(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getBoolean(aParameterIndex);
    }

    public boolean getBoolean(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getBoolean(aParameterName);
    }

    public byte getByte(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getByte(aParameterIndex);
    }

    public byte getByte(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getByte(aParameterName);
    }

    public short getShort(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getShort(aParameterIndex);
    }

    public short getShort(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getShort(aParameterName);
    }

    public int getInt(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getInt(aParameterIndex);
    }

    public int getInt(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getInt(aParameterName);
    }

    public long getLong(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getLong(aParameterIndex);
    }

    public long getLong(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getLong(aParameterName);
    }

    public float getFloat(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getFloat(aParameterIndex);
    }

    public float getFloat(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getFloat(aParameterName);
    }

    public double getDouble(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getDouble(aParameterIndex);
    }

    public double getDouble(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getDouble(aParameterName);
    }

    @SuppressWarnings("deprecation")
    public BigDecimal getBigDecimal(int aParameterIndex, int aScale) throws SQLException
    {
        return mInternalCstmt.getBigDecimal(aParameterIndex, aScale);
    }

    public byte[] getBytes(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getBytes(aParameterIndex);
    }

    public byte[] getBytes(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getBytes(aParameterName);
    }

    public Date getDate(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getDate(aParameterIndex);
    }

    public Date getDate(String aParameterName, Calendar aCal) throws SQLException
    {
        return mInternalCstmt.getDate(aParameterName, aCal);
    }

    public Date getDate(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getDate(aParameterName);
    }

    public Time getTime(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getTime(aParameterIndex);
    }

    public Time getTime(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getTime(aParameterName);
    }

    public Time getTime(String aParameterName, Calendar aCal) throws SQLException
    {
        return mInternalCstmt.getTime(aParameterName, aCal);
    }

    public Timestamp getTimestamp(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getTimestamp(aParameterIndex);
    }

    public Timestamp getTimestamp(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getTimestamp(aParameterName);
    }

    public Timestamp getTimestamp(String aParameterName, Calendar aCal) throws SQLException
    {
        return mInternalCstmt.getTimestamp(aParameterName, aCal);
    }

    public Object getObject(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getObject(aParameterIndex);
    }

    public Object getObject(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getObject(aParameterName);
    }

    public BigDecimal getBigDecimal(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getBigDecimal(aParameterIndex);
    }

    public BigDecimal getBigDecimal(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getBigDecimal(aParameterName);
    }

    public Object getObject(int aParameterIndex, Map<String, Class<?>> aMap) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "User defined type");
        return null;
    }

    public Object getObject(String aParameterName, Map<String, Class<?>> aMap) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "User defined type");
        return null;
    }

    public Ref getRef(int aParameterIndex) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Ref type");
        return null;
    }

    public Ref getRef(String aParameterName) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Ref type");
        return null;
    }

    public Blob getBlob(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getBlob(aParameterIndex);
    }

    public Blob getBlob(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getBlob(aParameterName);
    }

    public Clob getClob(int aParameterIndex) throws SQLException
    {
        return mInternalCstmt.getClob(aParameterIndex);
    }

    public Clob getClob(String aParameterName) throws SQLException
    {
        return mInternalCstmt.getClob(aParameterName);
    }

    public Array getArray(int aParameterIndex) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Array type");
        return null;
    }

    public Array getArray(String aParameterName) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Array type");
        return null;
    }

    public Date getDate(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return mInternalCstmt.getDate(aParameterIndex, aCal);
    }

    public Time getTime(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return mInternalCstmt.getTime(aParameterIndex, aCal);
    }

    public Timestamp getTimestamp(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return mInternalCstmt.getTimestamp(aParameterIndex, aCal);
    }

    public URL getURL(int aParameterIndex) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "URL type");
        return null;
    }

    public URL getURL(String aParameterName) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "URL type");
        return null;
    }

    public void setURL(String aParameterName, URL val) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "URL type");
    }

    public void setNull(String aParameterName, int aSqlType) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setNull",
                                                  new Class[] {String.class, int.class},
                                                  new Object[] { aParameterName, aSqlType});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setNull(aParameterName, aSqlType);
        }
    }

    public void setBoolean(String aParameterName, boolean aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setBoolean",
                                                  new Class[] {String.class, boolean.class},
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setBoolean(aParameterName, aValue);
        }
    }

    public void setByte(String aParameterName, byte aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class,
                                                  "setByte",
                                                  new Class[] {String.class, byte.class},
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setByte(aParameterName, aValue);
        }
    }

    public void setShort(String aParameterName, short aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setShort",
                                                  new Class[] { String.class, short.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setShort(aParameterName, aValue);
        }
    }

    public void setInt(String aParameterName, int aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setInt",
                                                  new Class[] { String.class, int.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setInt(aParameterName, aValue);
        }
    }

    public void setLong(String aParameterName, long aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setLong",
                                                  new Class[] { String.class, long.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setLong(aParameterName, aValue);
        }
    }

    public void setFloat(String aParameterName, float aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setFloat",
                                                  new Class[] { String.class, float.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setFloat(aParameterName, aValue);
        }
    }

    public void setDouble(String aParameterName, double aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setDouble",
                                                  new Class[] { String.class, double.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setDouble(aParameterName, aValue);
        }
    }

    public void setBigDecimal(String aParameterName, BigDecimal aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setBigDecimal",
                                                  new Class[] { String.class, BigDecimal.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setBigDecimal(aParameterName, aValue);
        }
    }

    public void setString(String aParameterName, String aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setString",
                                                  new Class[] { String.class, String.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setString(aParameterName, aValue);
        }
    }

    public void setBytes(String aParameterName, byte[] aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setBytes",
                                                  new Class[] { String.class, byte[].class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setBytes(aParameterName, aValue);
        }
    }

    public void setDate(String aParameterName, Date aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setDate",
                                                  new Class[] { String.class, Date.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setDate(aParameterName, aValue);
        }
    }

    public void setTime(String aParameterName, Time aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setTime",
                                                  new Class[] { String.class, Time.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setTime(aParameterName, aValue);
        }
    }

    public void setTimestamp(String aParameterName, Timestamp aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setTimestamp",
                                                  new Class[] { String.class, Timestamp.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setTimestamp(aParameterName, aValue);
        }
    }

    public void setAsciiStream(String aParameterName, InputStream aStream, int aLength) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setAsciiStream",
                                                  new Class[] { String.class, InputStream.class, int.class },
                                                  new Object[] { aParameterName, aStream, aLength});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setAsciiStream(aParameterName, aStream, aLength);
        }
    }

    public void setBinaryStream(String aParameterName, InputStream aStream, int aLength) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, "setBinaryStream",
                               new Class[] {String.class, InputStream.class, int.class},
                               new Object[] { aParameterName, aStream, aLength});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setBinaryStream(aParameterName, aStream, aLength);
        }
    }

    public void setObject(String aParameterName, Object aValue, int aTargetSqlType, int aScale)
            throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setObject",
                                                  new Class[] { String.class, Object.class, int.class, int.class },
                                                  new Object[] { aParameterName, aValue, aTargetSqlType, aScale});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setObject(aParameterName, aValue, aTargetSqlType, aScale);
        }
    }

    public void setObject(String aParameterName, Object aValue, int aTargetSqlType) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setObject",
                                                  new Class[] { String.class, Object.class, int.class },
                                                  new Object[] { aParameterName, aValue, aTargetSqlType});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setObject(aParameterName, aValue, aTargetSqlType);
        }
    }

    public void setObject(String aParameterName, Object aValue) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setObject",
                                                  new Class[] { String.class, Object.class },
                                                  new Object[] { aParameterName, aValue});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setObject(aParameterName, aValue);
        }
    }

    public void setCharacterStream(String aParameterName, Reader aReader, int aLength) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setCharacterStream",
                                                  new Class[] { String.class, Reader.class, int.class },
                                                  new Object[] { aParameterName, aReader, aLength});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setCharacterStream(aParameterName, aReader, aLength);
        }
    }

    public void setDate(String aParameterName, Date aValue, Calendar aCal) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setDate",
                                                  new Class[] { String.class, Date.class, Calendar.class },
                                                  new Object[] { aParameterName, aValue, aCal});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setDate(aParameterName, aValue, aCal);
        }
    }

    public void setTime(String aParameterName, Time aValue, Calendar aCal) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setTime",
                                                  new Class[] { String.class, Time.class, Calendar.class },
                                                  new Object[] { aParameterName, aValue, aCal});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setTime(aParameterName, aValue, aCal);
        }
    }

    public void setTimestamp(String aParameterName, Timestamp aValue, Calendar aCal) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setTimestamp",
                                                  new Class[] { String.class, Timestamp.class, Calendar.class },
                                                  new Object[] { aParameterName, aValue, aCal});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setTimestamp(aParameterName, aValue, aCal);
        }
    }

    public void setNull(String aParameterName, int aSqlType, String aTypeName) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(CallableStatement.class, 
                                                  "setNull",
                                                  new Class[] { String.class, int.class, String.class },
                                                  new Object[] { aParameterName, aSqlType, aTypeName});
        if (mIsCoordQuery)
        {
            mServerSideCstmt.setNull(aParameterName, aSqlType, aTypeName);
        }
    }

    @Override
    public void setNString(String parameterName, String value) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setClob(String parameterName, Reader reader, long length) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBlob(String parameterName, InputStream inputStream, long length) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public String getNString(int parameterIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public String getNString(String parameterName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Reader getCharacterStream(int parameterIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Reader getCharacterStream(String parameterName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBlob(String parameterName, Blob x) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setClob(String parameterName, Clob x) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setAsciiStream(String parameterName, InputStream x, long length) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBinaryStream(String parameterName, InputStream x, long length) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setCharacterStream(String parameterName, Reader reader, long length) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setAsciiStream(String parameterName, InputStream x) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBinaryStream(String parameterName, InputStream x) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setCharacterStream(String parameterName, Reader reader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setClob(String parameterName, Reader reader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBlob(String parameterName, InputStream inputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public <T> T getObject(int parameterIndex, Class<T> type) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public <T> T getObject(String parameterName, Class<T> type) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
