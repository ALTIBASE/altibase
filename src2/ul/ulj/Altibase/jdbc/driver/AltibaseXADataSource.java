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

import javax.sql.XAConnection;
import javax.sql.XADataSource;

import Altibase.jdbc.driver.logging.LoggingProxyFactory;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class AltibaseXADataSource extends AltibaseDataSource implements XADataSource
{
    private static final long serialVersionUID = 438793250406109353L;

    public AltibaseXADataSource()
    {
    }

    AltibaseXADataSource(AltibaseProperties aProp)
    {
        super(aProp);
    }

    public XAConnection getXAConnection() throws SQLException
    {
        Connection sPhysicalConnection = getConnection();
        sPhysicalConnection.setAutoCommit(false);
        return createProxy(new AltibaseXAConnection(sPhysicalConnection));
    }

    public XAConnection getXAConnection(String aUser, String aPassword) throws SQLException
    {
        AltibaseConnection sPhysicalConnection = (AltibaseConnection) getConnection(aUser, aPassword);
        sPhysicalConnection.setAutoCommit(false);
        return createProxy(new AltibaseXAConnection(sPhysicalConnection));
    }
    
    private XAConnection createProxy(XAConnection aXAConnection) throws SQLException
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED) 
        {
            aXAConnection = LoggingProxyFactory.createXaConnectionProxy(aXAConnection);
        }
        return aXAConnection;
    }
}
