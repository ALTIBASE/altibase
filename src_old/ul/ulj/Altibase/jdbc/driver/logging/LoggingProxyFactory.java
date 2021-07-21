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

import java.lang.reflect.Proxy;
import java.sql.CallableStatement;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;

import javax.sql.PooledConnection;
import javax.sql.XAConnection;
import javax.transaction.xa.XAResource;

/**
 * 타켓오브젝트를 받아서 해당하는 Proxy클래스를 생성하여 돌려주는 팩토리 클래스
 * 
 * @author yjpark
 *
 */
public final class LoggingProxyFactory
{
    /**
     * Connection, Statement 인터페이스는 각각 구현한 Proxy를 생성하고 나머지는 LoggingProxy를 생성한다.
     * 
     * @param aClassInfo
     * @param aOrigin
     * @return
     */
    
    private LoggingProxyFactory()
    {
    }
    
    private static Object createProxy(LoggingProxy aProxy, Class aInterface)
    {
        return Proxy.newProxyInstance(aProxy.getClass().getClassLoader(), new Class[] { aInterface }, aProxy); 
    }
    
    public static Connection createConnectionProxy(Object aConn)
    {
        return (Connection)createProxy(new ConnectionLoggingProxy(aConn), Connection.class);
    }
    
    public static CallableStatement createCallableStmtProxy(Object aStmt, Connection aConn)
    {
        return (CallableStatement)createProxy(new CallableStmtLoggingProxy(aStmt, aConn), CallableStatement.class); 
    }
    
    public static PreparedStatement createPreparedStmtProxy(Object aStmt, Connection aConn)
    {
        return (PreparedStatement)createProxy(new PreparedStmtLoggingProxy(aStmt, aConn), PreparedStatement.class);
    }

    public static Statement createStatementProxy(Object aStmt, Connection aConn)
    {
        return (Statement)createProxy(new StatementLoggingProxy(aStmt, aConn), Statement.class); 
    }
    
    public static ResultSet createResultSetProxy(Object aRs)
    {
        return (ResultSet)createProxy(new LoggingProxy(aRs, "ResultSet", LoggingProxy.JDBC_LOGGER_ROWSET), ResultSet.class); 
    }
    
    public static PooledConnection createPooledConnectionProxy(Object aPooledCon)
    {
        return (PooledConnection)createProxy(new LoggingProxy(aPooledCon, "PooledConnection", 
                                                              LoggingProxy.JDBC_LOGGER_POOL), PooledConnection.class); 
    }
    
    public static XAConnection createXaConnectionProxy(Object aXaCon)
    {
        return (XAConnection)createProxy(new LoggingProxy(aXaCon, "XAConnection", 
                                                          LoggingProxy.JDBC_LOGGER_XA), XAConnection.class); 
    }
    
    public static XAResource createXaResourceProxy(Object aXaResource)
    {
        return (XAResource)createProxy(new LoggingProxy(aXaResource, "XAResource", 
                                                        LoggingProxy.JDBC_LOGGER_XA), XAResource.class);
    }
}
