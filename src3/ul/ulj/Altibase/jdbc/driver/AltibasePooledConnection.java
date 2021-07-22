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

import java.sql.Connection;
import java.sql.SQLException;
import java.util.LinkedList;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.sql.ConnectionEvent;
import javax.sql.ConnectionEventListener;
import javax.sql.PooledConnection;
import javax.sql.StatementEventListener;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;

public class AltibasePooledConnection implements PooledConnection
{
    private Connection                                mPhysicalConnection;
    private Connection                                mLogicalConnection;
    private final LinkedList<ConnectionEventListener> mListeners;
    private transient Logger                          mLogger;
    
    AltibasePooledConnection(Connection aPhysicalConnection)
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_POOL);
            int sSessId = ((AltibaseConnection)aPhysicalConnection).getSessionId();
            mLogger.log(Level.INFO, "Created {0} {1}.", new Object[] { "PhysicalConnection", "[SessId #" + sSessId + "] " });
        }
        mPhysicalConnection = aPhysicalConnection;
        mLogicalConnection = null;
        mListeners = new LinkedList<>();
    }
    
    protected Connection createLogicalConnection(Connection aPhysicalConnection, AltibasePooledConnection aParent)
    {
        return new AltibaseLogicalConnection(aPhysicalConnection, aParent);
    }
    
    void notifyLogicalConnectionClosed()
    {
        ConnectionEvent sEvent = new ConnectionEvent(this);
        synchronized (mListeners)
        {
            for (ConnectionEventListener sEach : mListeners)
            {
                sEach.connectionClosed(sEvent);
            }
        }
    }

    void notifyConnectionErrorOccurred(SQLException aException)
    {
        ConnectionEvent sEvent = new ConnectionEvent(this, aException);
        synchronized (mListeners)
        {
            for (ConnectionEventListener sEach : mListeners)
            {
                sEach.connectionErrorOccurred(sEvent);
            }
        }
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
        catch (SQLException sEx)
        {
            notifyConnectionErrorOccurred(sEx);
            mLogicalConnection = null;
            throw sEx;
        }
        return mLogicalConnection;
    }

    public void close() throws SQLException
    {
        long aStartTime = System.currentTimeMillis();
        if (mPhysicalConnection != null)
        {
            int sSessId = ((AltibaseConnection)mPhysicalConnection).getSessionId();
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.INFO, "{0} {1} closed. ", new Object[] { "[SessId #" + sSessId +"] ",
                                                                           "PhysicalConnection" });
            }
            mPhysicalConnection.close();
            mPhysicalConnection = null;
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                long sElapsedtime = System.currentTimeMillis() - aStartTime;
                mLogger.log(Level.FINE, "{0} closed in {1} msec", 
                            new Object[] { "[SessId #"+ sSessId + "] ", String.valueOf(sElapsedtime) });
            }
        }
    }

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

    @Override
    public void addStatementEventListener(StatementEventListener aListener)
    {
        // Ignore(StatementEventListener is not supported)
    }

    @Override
    public void removeStatementEventListener(StatementEventListener aListener)
    {
        // Ignore(StatementEventListener is not supported)
    }
}
