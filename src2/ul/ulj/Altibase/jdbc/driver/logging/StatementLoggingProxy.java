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

package Altibase.jdbc.driver.logging;

import java.lang.reflect.Method;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLWarning;
import java.util.logging.Level;

import Altibase.jdbc.driver.AltibaseConnection;
import Altibase.jdbc.driver.AltibaseLogicalConnection;

/**
 * java.sql.Statement 인터페이스를 hooking하는 Proxy클래스
 * 
 * @author yjpark
 */
public class StatementLoggingProxy extends LoggingProxy
{
    protected static final String METHOD_PREFIX_EXECUTE = "execute";
    
    public StatementLoggingProxy(Object aTarget, Connection aConn)
    {
        this(aTarget, "Statement", aConn);
    }
    
    public StatementLoggingProxy(Object aTarget, String aTargetName, Connection aConn)
    {
        super(aTarget, aTargetName);
        String sSessId = null;
        if (aConn instanceof AltibaseConnection)
        {
            sSessId = String.valueOf(((AltibaseConnection)aConn).getSessionId());
        }
        else if (aConn instanceof AltibaseLogicalConnection)
        {
            sSessId = String.valueOf(aConn.hashCode());
        }
        mLogger.log(Level.INFO, "[SessId #{0}] Created {1} {2}.", new Object[] { sSessId,  
                                                                                 mTargetName, getUniqueId() });
    }

    public void logSql(Method aMethod, Object aResult, Object[] aArgs)
    {
        // 메소드명이 execute로 시작하는 경우에만 sql문을 출력한다.
        if (aMethod.getName().startsWith(METHOD_PREFIX_EXECUTE) && aArgs != null)
        {
            mLogger.log(Level.CONFIG, "{0} executing sql : {1}", new Object[] { getUniqueId(), aArgs[0] });
        }
    }
    
    protected void logSqlTiming(Method aMethod, long aStartTime)
    {
        if (aMethod.getName().startsWith(METHOD_PREFIX_EXECUTE))
        {
            long sElapsedtime = System.currentTimeMillis() - aStartTime;
            mLogger.log(Level.CONFIG, "{0} executed in {1} msec", 
                        new Object[] { getUniqueId(), String.valueOf(sElapsedtime) });
        }
        else if (aMethod.getName().startsWith(METHOD_PREFIX_CLOSE))
        {
            long sElapsedtime = System.currentTimeMillis() - aStartTime;
            mLogger.log(Level.FINE, "{0} closed in {1} msec", 
                        new Object[] { getUniqueId(), String.valueOf(sElapsedtime) });
        } 
    }
    
    protected Object createProxyWithReturnValue(Object aResult)
    {
        Object sProxy;
        
        if (aResult instanceof ResultSet)
        {
            sProxy = LoggingProxyFactory.createResultSetProxy(aResult);
        }
        else
        {
            sProxy = aResult;
        }
        
        if (aResult instanceof SQLWarning)
        {
            mLogger.log(Level.WARNING, "SQLWarning : ", (SQLWarning)aResult);
        }
        
        return sProxy;
    }
    
    /**
     * Statement 같은 경우 Object의 hashcode를 unique id로 사용한다.
     */
    public String getUniqueId()
    {
        return "[StmtId #" + String.valueOf(mTarget.hashCode()) + "] ";
    }
}
