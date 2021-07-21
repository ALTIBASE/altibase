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

import Altibase.jdbc.driver.AltibasePreparedStatement;
import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.sql.*;
import java.util.Calendar;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

/**
 *  java.sql.PreparedStatement 인터페이스 중 일부를 구현하며 내부적으로 서버사이드용 PreparedStatement를
 *  이용해 쿼리를 수행한다.
 */
public class ServerSideShardingPreparedStatement extends ServerSideShardingStatement implements InternalShardingPreparedStatement
{
    private PreparedStatement mServersidePstmt;

    public ServerSideShardingPreparedStatement(PreparedStatement aServersidePstmt,
                                               AltibaseShardingConnection aMetaConn)
    {
        super(aServersidePstmt, aMetaConn);
        mServersidePstmt = aServersidePstmt;
    }

    public ResultSet executeQuery() throws SQLException
    {
        touchMetaNode();
        shard_log("(META NODE EXECUTEQUERY) {0}",
                  ((AltibasePreparedStatement)mServersidePstmt).getStmtId());
        calcDistTxInfo();
        return mServersidePstmt.executeQuery();
    }

    public int executeUpdate() throws SQLException
    {
        touchMetaNode();
        shard_log("(META NODE EXECUTEUPDATE) {0}",
                  ((AltibasePreparedStatement)mServersidePstmt).getStmtId());
        calcDistTxInfo();
        return mServersidePstmt.executeUpdate();
    }

    public int[] executeBatch() throws SQLException
    {
        touchMetaNode();
        shard_log("(META NODE EXECUTEBATCH) {}",
                  ((AltibasePreparedStatement)mServersidePstmt).getStmtId());
        calcDistTxInfo();
        return mServersidePstmt.executeBatch();
    }

    public void clearParameters() throws SQLException
    {
        mServersidePstmt.clearParameters();
    }

    public boolean execute() throws SQLException
    {
        touchMetaNode();
        shard_log("(META NODE EXECUTE) {}",
                  ((AltibasePreparedStatement)mServersidePstmt).getStmtId());
        calcDistTxInfo();
        return mServersidePstmt.execute();
    }

    public void addBatch() throws SQLException
    {
        mServersidePstmt.addBatch();
    }

    public ResultSetMetaData getMetaData() throws SQLException
    {
        return mServersidePstmt.getMetaData();
    }

    public ParameterMetaData getParameterMetaData() throws SQLException
    {
        return mServersidePstmt.getParameterMetaData();
    }

    public void clearBatch() throws SQLException
    {
        mServersidePstmt.clearBatch();
    }

    public void rePrepare(CmProtocolContextShardStmt aShardStmtCtx) throws SQLException
    {
        /* BUG-47357 server-side일때는 항상 prepare요청이 먼저 나가기 때문에 reprepare를 다시 해야 한다. */
        AltibasePreparedStatement sPstmt = (AltibasePreparedStatement)mServersidePstmt;
        sPstmt.prepare(sPstmt.getSql());
    }

    public void setInt(int aParameterIndex, int aValue) throws SQLException
    {
        mServersidePstmt.setInt(aParameterIndex, aValue);
    }

    public void setNull(int aParameterIndex, int aSqlType) throws SQLException
    {
        mServersidePstmt.setNull(aParameterIndex, aSqlType);
    }

    public void setNull(int aParameterIndex, int aSqlType, String aTypeName) throws SQLException
    {
        mServersidePstmt.setNull(aParameterIndex, aSqlType, aTypeName);
    }

    public void setBoolean(int aParameterIndex, boolean aValue) throws SQLException
    {
        mServersidePstmt.setBoolean(aParameterIndex, aValue);
    }

    public void setByte(int aParameterIndex, byte aValue) throws SQLException
    {
        mServersidePstmt.setByte(aParameterIndex, aValue);
    }

    public void setShort(int aParameterIndex, short aValue) throws SQLException
    {
        mServersidePstmt.setShort(aParameterIndex, aValue);
    }

    public void setLong(int aParameterIndex, long aValue) throws SQLException
    {
        mServersidePstmt.setLong(aParameterIndex, aValue);
    }

    public void setFloat(int aParameterIndex, float aValue) throws SQLException
    {
        mServersidePstmt.setFloat(aParameterIndex, aValue);
    }

    public void setDouble(int aParameterIndex, double aValue) throws SQLException
    {
        mServersidePstmt.setDouble(aParameterIndex, aValue);
    }

    public void setBigDecimal(int aParameterIndex, BigDecimal aValue) throws SQLException
    {
        mServersidePstmt.setBigDecimal(aParameterIndex, aValue);
    }

    public void setString(int aParameterIndex, String aValue) throws SQLException
    {
        mServersidePstmt.setString(aParameterIndex, aValue);
    }

    public void setBytes(int aParameterIndex, byte[] aValue) throws SQLException
    {
        mServersidePstmt.setBytes(aParameterIndex, aValue);
    }

    public void setDate(int aParameterIndex, Date aValue) throws SQLException
    {
        mServersidePstmt.setDate(aParameterIndex, aValue);
    }

    public void setDate(int aParameterIndex, Date aValue, Calendar aCal) throws SQLException
    {
        mServersidePstmt.setDate(aParameterIndex, aValue, aCal);
    }

    public void setTime(int aParameterIndex, Time aValue) throws SQLException
    {
        mServersidePstmt.setTime(aParameterIndex, aValue);
    }

    public void setTime(int aParameterIndex, Time aValue, Calendar aCal) throws SQLException
    {
        mServersidePstmt.setTime(aParameterIndex, aValue, aCal);
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue) throws SQLException
    {
        mServersidePstmt.setTimestamp(aParameterIndex, aValue);
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue, Calendar aCal) throws SQLException
    {
        mServersidePstmt.setTimestamp(aParameterIndex, aValue, aCal);
    }

    public void setAsciiStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        mServersidePstmt.setAsciiStream(aParameterIndex, aValue, aLength);
    }

    public void setBinaryStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        mServersidePstmt.setBinaryStream(aParameterIndex, aValue, aLength);
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType, int aScale) throws SQLException
    {
        mServersidePstmt.setObject(aParameterIndex, aValue, aTargetSqlType, aScale);
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType) throws SQLException
    {
        mServersidePstmt.setObject(aParameterIndex, aValue, aTargetSqlType);
    }

    public void setCharacterStream(int aParameterIndex, Reader aReader, int aLength) throws SQLException
    {
        mServersidePstmt.setCharacterStream(aParameterIndex, aReader, aLength);
    }

    public void setBlob(int aParameterIndex, Blob aValue) throws SQLException
    {
        mServersidePstmt.setBlob(aParameterIndex, aValue);
    }

    public void setClob(int aParameterIndex, Clob aValue) throws SQLException
    {
        mServersidePstmt.setClob(aParameterIndex, aValue);
    }
}
