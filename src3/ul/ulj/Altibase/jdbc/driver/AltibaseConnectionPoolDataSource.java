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
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.sql.ConnectionPoolDataSource;
import javax.sql.PooledConnection;

import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.LoggingProxyFactory;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class AltibaseConnectionPoolDataSource extends AltibaseDataSource implements ConnectionPoolDataSource
{
    private static final long serialVersionUID = 2846420679868467880L;
    private transient Logger            mLogger;
    
    public AltibaseConnectionPoolDataSource()
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_POOL);
        }
    }

    public AltibaseConnectionPoolDataSource(String aDataSourceName)
    {
        super(aDataSourceName);
    }

    AltibaseConnectionPoolDataSource(AltibaseProperties aProp)
    {
        super(aProp);
    }

    public PooledConnection getPooledConnection() throws SQLException
    {
        return createProxy(new AltibasePooledConnection((Connection)getConnection()));
    }

    public PooledConnection getPooledConnection(String aUser, String aPassword) throws SQLException
    {
        return createProxy(new AltibasePooledConnection((Connection)getConnection(aUser, aPassword)));
    }

    // PROJ-2583 jdbc logging
    private PooledConnection createProxy(PooledConnection aPooledCon)
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger.log(Level.INFO, "Created PooledConnection " + aPooledCon);
            aPooledCon = LoggingProxyFactory.createPooledConnectionProxy(aPooledCon);
        }
        return aPooledCon;
    }
}
