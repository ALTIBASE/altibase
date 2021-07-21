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

import javax.sql.XAConnection;
import javax.transaction.xa.XAResource;

import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;

public class AltibaseXAConnection extends AltibasePooledConnection implements XAConnection
{
    private AltibaseXAResource mResource;
    private transient Logger mLogger;
    
    protected AltibaseLogicalConnection createLogicalConnection(AltibaseConnection aPhysicalConnection, AltibasePooledConnection aParent)
    {
        return new AltibaseXALogicalConnection(aPhysicalConnection, (AltibaseXAConnection)aParent);
    }
    
    public AltibaseXAConnection(Connection aPhysicalConnection) throws SQLException
    {
        super(aPhysicalConnection);
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            this.mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_XA);
            mLogger.log(Level.INFO, "Created XAConnection " + this);
        }
        mResource = new AltibaseXAResource((AltibaseConnection)aPhysicalConnection);
        mResource.xaOpen();
    }

    public void close() throws SQLException
    {
        mResource.xaClose();
        super.close();
    }
    
    public XAResource getXAResource() throws SQLException
    {
        return mResource;
    }
}
