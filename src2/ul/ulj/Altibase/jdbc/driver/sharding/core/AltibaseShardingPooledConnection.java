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

import javax.sql.ConnectionEvent;
import javax.sql.ConnectionEventListener;
import javax.sql.PooledConnection;
import javax.sql.StatementEventListener;
import java.sql.Connection;
import java.sql.SQLException;
import java.util.LinkedList;

public class AltibaseShardingPooledConnection implements PooledConnection
{
    private Connection mPhysicalConnection;
    private Connection mLogicalConnection;
    private LinkedList mListeners;

    public AltibaseShardingPooledConnection(Connection aPhysicalConnection)
    {
        mPhysicalConnection = aPhysicalConnection;
        mLogicalConnection = null;
        mListeners = new LinkedList();
    }

    public Connection getConnection() throws SQLException
    {
        try
        {
            if (mPhysicalConnection == null)
            {
                Error.throwSQLException(ErrorDef.CLOSED_CONNECTION);
            }

            if (mLogicalConnection != null)
            {
                mLogicalConnection.close();
            }
            mLogicalConnection = createLogicalConnection(mPhysicalConnection, this);
        }
        catch (SQLException aException)
        {
            notifyConnectionErrorOccurred(aException);
            mLogicalConnection = null;
            throw aException;
        }
        return mLogicalConnection;
    }

    private Connection createLogicalConnection(Connection aPhysicalConnection,
                                               AltibaseShardingPooledConnection aParent)
    {
        return new AltibaseShardingLogicalConnection(aPhysicalConnection, aParent);
    }

    public void close() throws SQLException
    {
        if (mPhysicalConnection != null)
        {
            mPhysicalConnection.close();
            mPhysicalConnection = null;
        }
    }

    @SuppressWarnings("unchecked")
    public void addConnectionEventListener(ConnectionEventListener aListener)
    {
        synchronized (mListeners)
        {
            mListeners.add(aListener);
        }
    }

    public void removeConnectionEventListener(ConnectionEventListener aListener)
    {
        synchronized (mListeners)
        {
            mListeners.remove(aListener);
        }
    }

    void notifyConnectionErrorOccurred(SQLException aException)
    {
        ConnectionEvent sEvent = new ConnectionEvent(this, aException);
        synchronized (mListeners)
        {
            for (Object aEach : mListeners)
            {
                ((ConnectionEventListener)aEach).connectionErrorOccurred(sEvent);
            }
        }
    }

    public void notifyLogicalConnectionClosed()
    {
        ConnectionEvent sEvent = new ConnectionEvent(this);
        synchronized (mListeners)
        {
            for (Object aEach : mListeners)
            {
                ((ConnectionEventListener)aEach).connectionClosed(sEvent);
            }
        }
    }

    @Override
    public void addStatementEventListener(StatementEventListener aListener)
    {
        // do not implements
    }

    @Override
    public void removeStatementEventListener(StatementEventListener aListener)
    {
        // do not implements
    }
}
