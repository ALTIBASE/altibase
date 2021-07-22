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

import Altibase.jdbc.driver.sharding.executor.*;

import java.math.BigDecimal;
import java.sql.*;
import java.util.Calendar;
import java.util.List;

public class DataNodeShardingCallableStatement extends DataNodeShardingPreparedStatement
        implements InternalShardingCallableStatement
{
    DataNodeShardingCallableStatement(AltibaseShardingConnection aShardCon, String aSql,
                                      int aResultSetType, int aResultSetConcurrency,
                                      int aResultSetHoldability,
                                      AltibaseShardingPreparedStatement aShardStmt) throws SQLException
    {
        super(aShardCon, aSql, aResultSetType, aResultSetConcurrency, aResultSetHoldability, aShardStmt);
    }

    public boolean wasNull() throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).wasNull();
    }

    public String getString(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getString(aParameterIndex);
    }

    private DataNode getRoutedNode()
    {
        DataNode sResult = null;
        if (mRouteResult != null && mRouteResult.size() > 0)
        {
            sResult = mRouteResult.get(0);
        }
        
        return sResult;
    }

    public boolean getBoolean(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBoolean(aParameterIndex);
    }

    public byte getByte(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getByte(aParameterIndex);
    }

    public short getShort(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getShort(aParameterIndex);
    }

    public int getInt(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getInt(aParameterIndex);
    }

    public long getLong(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getLong(aParameterIndex);
    }

    public float getFloat(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getFloat(aParameterIndex);
    }

    public double getDouble(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getDouble(aParameterIndex);
    }

    public BigDecimal getBigDecimal(int aParameterIndex, int aScale)
    {
        return null;
    }

    public byte[] getBytes(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBytes(aParameterIndex);
    }

    public Date getDate(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getDate(aParameterIndex);
    }

    public Time getTime(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTime(aParameterIndex);
    }

    public Timestamp getTimestamp(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTimestamp(aParameterIndex);
    }

    public Object getObject(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getObject(aParameterIndex);
    }

    public BigDecimal getBigDecimal(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBigDecimal(aParameterIndex);
    }

    public Blob getBlob(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBlob(aParameterIndex);
    }

    public Clob getClob(int aParameterIndex) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getClob(aParameterIndex);
    }

    public Date getDate(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getDate(aParameterIndex, aCal);
    }

    public Time getTime(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTime(aParameterIndex, aCal);
    }

    public Timestamp getTimestamp(int aParameterIndex, Calendar aCal) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTimestamp(aParameterIndex, aCal);
    }

    public String getString(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getString(aParameterName);
    }

    public boolean getBoolean(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBoolean(aParameterName);
    }

    public byte getByte(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getByte(aParameterName);
    }

    public short getShort(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getShort(aParameterName);
    }

    public int getInt(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getInt(aParameterName);
    }

    public long getLong(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getLong(aParameterName);
    }

    public float getFloat(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getFloat(aParameterName);
    }

    public double getDouble(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getDouble(aParameterName);
    }

    public byte[] getBytes(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBytes(aParameterName);
    }

    public Date getDate(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getDate(aParameterName);
    }

    public Time getTime(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTime(aParameterName);
    }

    public Timestamp getTimestamp(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTimestamp(aParameterName);
    }

    public Object getObject(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getObject(aParameterName);
    }

    public BigDecimal getBigDecimal(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBigDecimal(aParameterName);
    }

    public Blob getBlob(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getBlob(aParameterName);
    }

    public Clob getClob(String aParameterName) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getClob(aParameterName);
    }

    public Date getDate(String aParameterName, Calendar aCal) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getDate(aParameterName, aCal);
    }

    public Time getTime(String aParameterName, Calendar aCal) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTime(aParameterName, aCal);
    }

    public Timestamp getTimestamp(String aParameterName, Calendar aCal) throws SQLException
    {
        return ((CallableStatement)mRoutedStatementMap.get(getRoutedNode())).getTimestamp(aParameterName, aCal);
    }

    protected List<Statement> route() throws SQLException
    {
        mRouteResult = mRoutingEngine.route(mSql, mParameters);

        ExecutorEngine sExecutorEngine = mMetaConn.getExecutorEngine();
        return sExecutorEngine.generateStatement(
                mRouteResult,
                new GenerateCallback<Statement>()
                {
                    public Statement generate(DataNode aNode) throws SQLException
                    {
                        Connection sNodeCon = mMetaConn.getNodeConnection(aNode);
                        CallableStatement sStmt = getNodeStatement(aNode, sNodeCon);
                        mShardStmt.replayMethodsInvocation(sStmt);  // CallableStatement method replay
                        mShardStmt.replaySetParameter(sStmt);
                        mRoutedStatementMap.put(aNode, sStmt);
                        return sStmt;
                    }
                }
        );
    }

    CallableStatement getNodeStatement(DataNode aNode, Connection aConn) throws SQLException
    {
        CallableStatement sStmt = (CallableStatement)mRoutedStatementMap.get(aNode);
        if (sStmt != null) return sStmt;

        sStmt = aConn.prepareCall(mSql, mResultSetType, mResultSetConcurrency, mResultSetHoldability);
        // BUG-46513 Meta 커넥션에 있는 SMN값을 셋팅한다.
        mShardStmt.setShardMetaNumber(mMetaConn.getShardMetaNumber());

        return  sStmt;
    }
}
