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

package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.*;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.Executor;

public class AltibaseLogicalConnection extends AbstractConnection
{
    protected Connection               mPhysicalConnection;
    protected AltibasePooledConnection mPooledConnection;
    protected boolean                  mClosed;

    AltibaseLogicalConnection(Connection aPhysicalConnection, AltibasePooledConnection aParent)
    {
        mPhysicalConnection = aPhysicalConnection;
        mPooledConnection = aParent;
        mClosed = false;
    }

    public void close() throws SQLException
    {
        if (mClosed)
        {
            return;
        }
        if (!mPhysicalConnection.getAutoCommit())
        {
            mPhysicalConnection.rollback();
        }
        mPhysicalConnection.clearWarnings();
        mClosed = true;
        mPooledConnection.notifyLogicalConnectionClosed();
    }

    public boolean isClosed() throws SQLException
    {
        return mClosed;
    }

    public Statement createStatement() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.createStatement();
    }

    public PreparedStatement prepareStatement(String aSql) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareStatement(aSql);
    }

    public CallableStatement prepareCall(String aSql) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareCall(aSql);
    }

    public String nativeSQL(String aSql) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.nativeSQL(aSql);
    }

    public void setAutoCommit(boolean aAutoCommit) throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.setAutoCommit(aAutoCommit);
    }

    public boolean getAutoCommit() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.getAutoCommit();
    }

    public void commit() throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.commit();
    }

    public void rollback() throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.rollback();
    }

    public DatabaseMetaData getMetaData() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.getMetaData();
    }

    public void setReadOnly(boolean aReadOnly) throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.setReadOnly(aReadOnly);
    }

    public boolean isReadOnly() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.isReadOnly();
    }

    public void setCatalog(String aCatalog) throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.setCatalog(aCatalog);
    }

    public String getCatalog() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.getCatalog();
    }

    public void setTransactionIsolation(int aLevel) throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.setTransactionIsolation(aLevel);
    }

    public int getTransactionIsolation() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.getTransactionIsolation();
    }

    public SQLWarning getWarnings() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.getWarnings();
    }

    public void clearWarnings() throws SQLException
    {
        mPhysicalConnection.clearWarnings();
    }

    public Statement createStatement(int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.createStatement(aResultSetType, aResultSetConcurrency);
    }

    public PreparedStatement prepareStatement(String aSql, int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareStatement(aSql, aResultSetType, aResultSetConcurrency);
    }

    public CallableStatement prepareCall(String aSql, int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareCall(aSql, aResultSetType, aResultSetConcurrency);
    }

    public Map getTypeMap() throws SQLException
    {
        return mPhysicalConnection.getTypeMap();
    }

    public void setTypeMap(Map aMap) throws SQLException
    {
        mPhysicalConnection.setTypeMap(aMap);
    }

    public void setHoldability(int aHoldability) throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.setHoldability(aHoldability);
    }

    public int getHoldability() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.getHoldability();
    }

    public Savepoint setSavepoint() throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.setSavepoint();
    }

    public Savepoint setSavepoint(String aName) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.setSavepoint(aName);
    }

    public void rollback(Savepoint aSavepoint) throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.rollback(aSavepoint);
    }

    public void releaseSavepoint(Savepoint aSavepoint) throws SQLException
    {
        throwErrorForClosed();
        mPhysicalConnection.releaseSavepoint(aSavepoint);
    }

    public Statement createStatement(int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.createStatement(aResultSetType, aResultSetConcurrency, aResultSetHoldability);
    }

    public PreparedStatement prepareStatement(String aSql, int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareStatement(aSql, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
    }

    public CallableStatement prepareCall(String aSql, int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareCall(aSql, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
    }

    public PreparedStatement prepareStatement(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareStatement(aSql, aAutoGeneratedKeys);
    }

    public PreparedStatement prepareStatement(String aSql, int[] aColumnIndexes) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareStatement(aSql, aColumnIndexes);
    }

    public PreparedStatement prepareStatement(String aSql, String[] aColumnNames) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.prepareStatement(aSql, aColumnNames);
    }

    // PROJ-2583 jdbc logging
    public int getSessionId() throws SQLException
    {
        throwErrorForClosed();
        return ((AltibaseConnection)mPhysicalConnection).getSessionId();
    }
    
    protected void throwErrorForClosed() throws SQLException
    {
        if (mClosed)
        {
            Error.throwSQLException(ErrorDef.CLOSED_CONNECTION);            
        }
    }

    @Override
    public boolean isValid(int aTimeout) throws SQLException
    {
        throwErrorForClosed();
        return mPhysicalConnection.isValid(aTimeout);
    }

    @Override
    public void setClientInfo(String aName, String aValue) throws SQLClientInfoException
    {
        mPhysicalConnection.setClientInfo(aName, aValue);
    }

    @Override
    public void setClientInfo(Properties aProperties) throws SQLClientInfoException
    {
        mPhysicalConnection.setClientInfo(aProperties);
    }

    @Override
    public String getClientInfo(String aName) throws SQLException
    {
        return mPhysicalConnection.getClientInfo(aName);
    }

    @Override
    public Properties getClientInfo() throws SQLException
    {
        return mPhysicalConnection.getClientInfo();
    }

    @Override
    public void abort(Executor aExecutor) throws SQLException
    {
        mPhysicalConnection.abort(aExecutor);
    }

    @Override
    public void setNetworkTimeout(Executor aExecutor, int aMilliseconds) throws SQLException
    {
        mPhysicalConnection.setNetworkTimeout(aExecutor, aMilliseconds);
    }

    @Override
    public int getNetworkTimeout() throws SQLException
    {
        return mPhysicalConnection.getNetworkTimeout();
    }
}
