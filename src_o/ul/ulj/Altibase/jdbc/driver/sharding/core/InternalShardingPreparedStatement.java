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

import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.sql.*;
import java.util.Calendar;

/**
 * ServerSide 또는 ClientSide에서 공통적으로 실행되는 java.sql.PreparedStatement 인터페이스 모음
 */
public interface InternalShardingPreparedStatement extends InternalShardingStatement
{
    ResultSet executeQuery() throws SQLException;
    int executeUpdate() throws SQLException;
    int[] executeBatch() throws SQLException;
    void clearParameters() throws SQLException;
    boolean execute() throws SQLException;
    void addBatch() throws SQLException;
    ResultSetMetaData getMetaData() throws SQLException;
    ParameterMetaData getParameterMetaData() throws SQLException;
    void clearBatch() throws SQLException;
    void rePrepare(CmProtocolContextShardStmt aShardStmtCtx) throws SQLException;
    void setInt(int aParameterIndex, int aValue) throws SQLException;
    void setNull(int aParameterIndex, int aSqlType) throws SQLException;
    void setNull(int aParameterIndex, int aSqlType, String aTypeName) throws SQLException;
    void setBoolean(int aParameterIndex, boolean aValue) throws SQLException;
    void setByte(int aParameterIndex, byte aValue) throws SQLException;
    void setShort(int aParameterIndex, short aValue) throws SQLException;
    void setLong(int aParameterIndex, long aValue) throws SQLException;
    void setFloat(int aParameterIndex, float aValue) throws SQLException;
    void setDouble(int aParameterIndex, double aValue) throws SQLException;
    void setBigDecimal(int aParameterIndex, BigDecimal aValue) throws SQLException;
    void setString(int aParameterIndex, String aValue) throws SQLException;
    void setBytes(int aParameterIndex, byte[] aValue) throws SQLException;
    void setDate(int aParameterIndex, Date aValue) throws SQLException;
    void setDate(int aParameterIndex, Date aValue, Calendar aCal) throws SQLException;
    void setTime(int aParameterIndex, Time aValue) throws SQLException;
    void setTime(int aParameterIndex, Time aValue, Calendar aCal) throws SQLException;
    void setTimestamp(int aParameterIndex, Timestamp aValue) throws SQLException;
    void setTimestamp(int aParameterIndex, Timestamp aValue, Calendar aCal) throws SQLException;
    void setAsciiStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException;
    void setBinaryStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException;
    void setObject(int aParameterIndex, Object aValue, int aTargetSqlType, int aScale) throws SQLException;
    void setObject(int aParameterIndex, Object aValue, int aTargetSqlType) throws SQLException;
    void setCharacterStream(int aParameterIndex, Reader aReader, int aLength) throws SQLException;
    void setBlob(int aParameterIndex, Blob aValue) throws SQLException;
    void setClob(int aParameterIndex, Clob aValue) throws SQLException;
}
