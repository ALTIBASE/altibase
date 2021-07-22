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

import java.sql.SQLException;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.ResultSet;
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;

public class AltibaseFailoverCallbackDummy implements AltibaseFailoverCallback
{
    private transient Logger mLogger;
    
    public AltibaseFailoverCallbackDummy()
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_FAILOVER);
        }
    }
    
    public int failoverCallback(Connection aConnection, Object aAppContext, int aFailoverEvent)
    {
        int sRet = AltibaseFailoverCallback.Result.GO;

        switch (aFailoverEvent)
        {
            case AltibaseFailoverCallback.Event.BEGIN:
                logFailoverEvent("Failover Begin! ", (AltibaseConnection)aConnection);
                break;

            case AltibaseFailoverCallback.Event.COMPLETED:
                logFailoverEvent("Failover SuccessFully End ! ", (AltibaseConnection)aConnection);
                if (executeSimpleFailoverValidation(aConnection) == false)
                {
                    // give-up fail over
                    sRet = AltibaseFailoverCallback.Result.QUIT;
                }
                break;

            case AltibaseFailoverCallback.Event.ABORT:
                logFailoverEvent("Failover Fail End ! ", (AltibaseConnection)aConnection);
                break;
            default:
                return sRet;
        }
        return sRet;
    }
    
    public void logFailoverEvent(String aMsg, AltibaseConnection aCon)
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger.log(Level.INFO, aMsg + aCon.toString());
        }
    }

    // execte Session Failover Validation Query
    // select 1 from dual;
    private boolean executeSimpleFailoverValidation(Connection aConnection)
    {
        Statement sStmt = null;
        ResultSet sRes = null;
        boolean sValSuccess = false;

        try
        {
            sStmt = aConnection.createStatement();
        }
        catch (SQLException ex1)
        {
            return false;
        }

        for (int i = 0; i < 10; i++)
        {
            try
            {
                sRes = sStmt.executeQuery("select 1 from dual");
                while (sRes.next())
                {
                    if (sRes.getInt(1) == 1)
                    {
                        sValSuccess = true;
                        break;
                    }
                }
            }
            catch (SQLException ex2)
            {
                // re-execute.
                try
                {

                    Thread.sleep(1000);
                }
                catch (InterruptedException exSleep)
                {
                    // ignore error
                }
                continue;
            }
        }

        try
        {
            sStmt.close();
        }
        catch (SQLException ex3)
        {
            /* do nothing */
        }

        return sValSuccess;
    }
}
