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

import Altibase.jdbc.driver.AltibaseStatement;
import Altibase.jdbc.driver.cm.CmProtocolContextShardConnect;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.*;

/**
 *  java.sql.Statement 인터페이스 중 일부를 구현하며 내부적으로 서버사이드용 Statement를
 *  이용해 쿼리를 수행한다.
 */
public class ServerSideShardingStatement implements InternalShardingStatement
{
    protected AltibaseStatement             mServerSideStmt;
    private   AltibaseShardingConnection    mMetaConn;

    public ServerSideShardingStatement(Statement aServerSideStmt,
                                       AltibaseShardingConnection aMetaConn)
    {
        mServerSideStmt = (AltibaseStatement)aServerSideStmt;
        mMetaConn = aMetaConn;
    }

    public ResultSet executeQuery(String aSql) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.executeQuery(aSql);
    }

    public boolean execute(String aSql) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.execute(aSql);
    }

    public int executeUpdate(String aSql) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.executeUpdate(aSql);
    }

    public void close() throws SQLException
    {
        mServerSideStmt.close();
    }

    public int getMaxFieldSize() throws SQLException
    {
        return mServerSideStmt.getMaxFieldSize();
    }

    public void setMaxFieldSize(int aMax) throws SQLException
    {
        mServerSideStmt.setMaxFieldSize(aMax);
    }

    public int getMaxRows() throws SQLException
    {
        return mServerSideStmt.getMaxRows();
    }

    public void setMaxRows(int aMax) throws SQLException
    {
        mServerSideStmt.setMaxRows(aMax);
    }

    public void setEscapeProcessing(boolean aEnable) throws SQLException
    {
        mServerSideStmt.setEscapeProcessing(aEnable);
    }

    public int getQueryTimeout() throws SQLException
    {
        return mServerSideStmt.getQueryTimeout();
    }

    public void setQueryTimeout(int aSeconds) throws SQLException
    {
        mServerSideStmt.setQueryTimeout(aSeconds);
    }

    public void cancel() throws SQLException
    {
        mServerSideStmt.cancel();
    }

    public SQLWarning getWarnings() throws SQLException
    {
        SQLWarning sWarning = null;
        SQLWarning sServerWarning = mServerSideStmt.getWarnings();

        if (sServerWarning == null) return null;

        if (sServerWarning.getErrorCode() == ErrorDef.SHARD_META_NUMBER_INVALID)
        {
            if (mMetaConn.getShardContextConnect().needToDisconnect())
            {
                sWarning = sServerWarning;
            }
        }
        else
        {
            sWarning = sServerWarning;
        }

        return sWarning;
    }

    public void clearWarnings() throws SQLException
    {
        mServerSideStmt.clearWarnings();
    }

    public void setCursorName(String aName) throws SQLException
    {
        mServerSideStmt.setCursorName(aName);
    }

    public ResultSet getResultSet() throws SQLException
    {
        return mServerSideStmt.getResultSet();
    }

    public int getUpdateCount() throws SQLException
    {
        return mServerSideStmt.getUpdateCount();
    }

    public boolean getMoreResults() throws SQLException
    {
        return mServerSideStmt.getMoreResults();
    }

    public void setFetchSize(int aRows) throws SQLException
    {
        mServerSideStmt.setFetchSize(aRows);
    }

    public int getFetchSize() throws SQLException
    {
        return mServerSideStmt.getFetchSize();
    }

    public Connection getConnection() throws SQLException
    {
        return mServerSideStmt.getConnection();
    }

    public boolean getMoreResults(int aCurrent) throws SQLException
    {
        return mServerSideStmt.getMoreResults();
    }

    public ResultSet getGeneratedKeys() throws SQLException
    {
        return mServerSideStmt.getGeneratedKeys();
    }

    public int executeUpdate(String aSql, int[] aColumnIndexes) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.executeUpdate(aSql, aColumnIndexes);
    }

    public int executeUpdate(String aSql, String[] aColumnNames) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.executeUpdate(aSql, aColumnNames);
    }

    public int executeUpdate(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.executeUpdate(aSql, aAutoGeneratedKeys);
    }

    public boolean execute(String aSql, int[] aColumnIndexes) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.execute(aSql, aColumnIndexes);
    }

    public boolean execute(String aSql, String[] aColumnNames) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.execute(aSql, aColumnNames);
    }

    public boolean execute(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        touchMetaNode();
        calcDistTxInfo();
        return mServerSideStmt.execute(aSql, aAutoGeneratedKeys);
    }

    public boolean isPrepared()
    {
        return mServerSideStmt.isPrepared();
    }

    public boolean hasNoData()
    {
        return mServerSideStmt.cursorhasNoData();
    }

    public void makeQstrForGeneratedKeys(String aSql, int[] aColumnIndexes,
                                         String[] aColumnNames) throws SQLException
    {
        // BUG-47168 서버사이드일때는 Meta에서 생성한 Statement에 Generated Key를 위한 쿼리를 생성해 둔다.
        mServerSideStmt.makeQstrForGeneratedKeys(aSql, aColumnIndexes, aColumnNames);
    }

    public void clearForGeneratedKeys()
    {
        mServerSideStmt.clearForGeneratedKeys();
    }

    void touchMetaNode() throws SQLException
    {
        CmProtocolContextShardConnect sShardContextConnect = mMetaConn.getShardContextConnect();
        if (!sShardContextConnect.isAutoCommitMode())
        {
            if (mMetaConn.getGlobalTransactionLevel() == GlobalTransactionLevel.ONE_NODE)
            {
                Error.throwSQLException(ErrorDef.SHARD_SERVERSIDE_SINGLE_NODE_TOUCH_ERROR);
            }

            sShardContextConnect.setTouched(true);
        }
    }
    
    void calcDistTxInfo()
    {
        mServerSideStmt.getProtocolContext().getDistTxInfo().calcDistTxInfoForServerSide(mServerSideStmt.getMetaConn());
        mServerSideStmt.getAltibaseConnection().setDistTxInfoForVerify();
    }
}
