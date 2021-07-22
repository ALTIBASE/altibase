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

import java.math.BigDecimal;
import java.sql.*;
import java.util.Calendar;

/**
 *  java.sql.CallableStatement 인터페이스 중 일부를 구현하며 내부적으로 서버사이드용 CallableStatement를
 *  이용해 쿼리를 수행한다.
 */
public class ServerSideShardingCallableStatement extends ServerSideShardingPreparedStatement implements InternalShardingCallableStatement
{
    private CallableStatement mServerSideCallableStmt;

    public ServerSideShardingCallableStatement(CallableStatement aServerSideCallableStmt,
                                               AltibaseShardingConnection aMetaConn)
    {
        super(aServerSideCallableStmt, aMetaConn);
        mServerSideCallableStmt = aServerSideCallableStmt;
    }

    public boolean wasNull() throws SQLException
    {
        return mServerSideCallableStmt.wasNull();
    }

    public String getString(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getString(aParameterIndex);
    }

    public boolean getBoolean(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getBoolean(aParameterIndex);
    }

    public byte getByte(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getByte(aParameterIndex);
    }

    public short getShort(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getShort(aParameterIndex);
    }

    public int getInt(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getInt(aParameterIndex);
    }

    public long getLong(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getLong(aParameterIndex);
    }

    public float getFloat(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getFloat(aParameterIndex);
    }

    public double getDouble(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getDouble(aParameterIndex);
    }

    public BigDecimal getBigDecimal(int aParameterIndex, int aScale) throws SQLException
    {
        return mServerSideCallableStmt.getBigDecimal(aParameterIndex, aScale);
    }

    public byte[] getBytes(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getBytes(aParameterIndex);
    }

    public Date getDate(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getDate(aParameterIndex);
    }

    public Time getTime(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getTime(aParameterIndex);
    }

    public Timestamp getTimestamp(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getTimestamp(aParameterIndex);
    }

    public Object getObject(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getObject(aParameterIndex);
    }

    public BigDecimal getBigDecimal(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getBigDecimal(aParameterIndex);
    }

    public Blob getBlob(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getBlob(aParameterIndex);
    }

    public Clob getClob(int aParameterIndex) throws SQLException
    {
        return mServerSideCallableStmt.getClob(aParameterIndex);
    }

    public Date getDate(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return mServerSideCallableStmt.getDate(aParameterIndex, aCal);
    }

    public Time getTime(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return mServerSideCallableStmt.getTime(aParameterIndex, aCal);
    }

    public Timestamp getTimestamp(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return mServerSideCallableStmt.getTimestamp(aParameterIndex, aCal);
    }

    public String getString(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getString(aParameterName);
    }

    public boolean getBoolean(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getBoolean(aParameterName);
    }

    public byte getByte(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getByte(aParameterName);
    }

    public short getShort(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getShort(aParameterName);
    }

    public int getInt(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getInt(aParameterName);
    }

    public long getLong(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getLong(aParameterName);
    }

    public float getFloat(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getFloat(aParameterName);
    }

    public double getDouble(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getDouble(aParameterName);
    }

    public byte[] getBytes(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getBytes(aParameterName);
    }

    public Date getDate(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getDate(aParameterName);
    }

    public Time getTime(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getTime(aParameterName);
    }

    public Timestamp getTimestamp(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getTimestamp(aParameterName);
    }

    public Object getObject(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getObject(aParameterName);
    }

    public BigDecimal getBigDecimal(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getBigDecimal(aParameterName);
    }

    public Blob getBlob(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getBlob(aParameterName);
    }

    public Clob getClob(String aParameterName) throws SQLException
    {
        return mServerSideCallableStmt.getClob(aParameterName);
    }

    public Date getDate(String aParameterName, Calendar aCal) throws SQLException
    {
        return mServerSideCallableStmt.getDate(aParameterName, aCal);
    }

    public Time getTime(String aParameterName, Calendar aCal) throws SQLException
    {
        return mServerSideCallableStmt.getTime(aParameterName, aCal);
    }

    public Timestamp getTimestamp(String aParameterName, Calendar aCal) throws SQLException
    {
        return mServerSideCallableStmt.getTimestamp(aParameterName, aCal);
    }
}
