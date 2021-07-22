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

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLWarning;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.transaction.xa.XAResource;

import Altibase.jdbc.driver.AltibaseLob;
import Altibase.jdbc.driver.AltibasePooledConnection;

/**
 * JDBC 인터페이스를 hooking하여 로깅기능을 수행하는 프락시 클래스</br>
 * 
 * 이 클래스는 JDBC API를 hooking하여 로그관련 동작을 일부 추가한다.
 * 몇몇 메소드는 하위클래스에서 overrideing 된다.
 * 
 * @author yjpark
 *
 */
public class LoggingProxy implements InvocationHandler
{
    public static final String  JDBC_LOGGER_DEFAULT    = "altibase.jdbc";
    public static final String  JDBC_LOGGER_ROWSET     = "altibase.jdbc.rowset";
    public static final String  JDBC_LOGGER_POOL       = "altibase.jdbc.pool";
    public static final String  JDBC_LOGGER_FAILOVER   = "altibase.jdbc.failover";
    public static final String  JDBC_LOGGER_CM         = "altibase.jdbc.cm";
    public static final String  JDBC_LOGGER_XA         = "altibase.jdbc.xa";
    public static final String  JDBC_LOGGER_ASYNCFETCH = "altibase.jdbc.asyncfetch"; // PROJ-2625
    private static final String METHOD_PREFIX_TOSTRING = "toString";
    private static final String METHOD_PREFIX_GET      = "get";
    protected static final String METHOD_PREFIX_CLOSE    = "close";
    protected transient Logger  mLogger;
    protected Object            mTarget;
    protected String            mTargetName;
    
    protected LoggingProxy(Object aTarget, String aTargetName)
    {
        this(aTarget, aTargetName, JDBC_LOGGER_DEFAULT);
    }
    
    protected LoggingProxy(Object aTarget, String aTargetName, String aLoggerName)
    {
        this.mTarget = aTarget;
        this.mTargetName = aTargetName;
        this.mLogger = Logger.getLogger(aLoggerName);
    }

    /**
     * JDBC API가 hooking 될때 수행되는 메소드.</br>
     * JDBC API를 hooking해 일부 로깅 기능을 수행하고 invoke로 해당 메소드를 수행한다.
     */
    public Object invoke(Object aProxy, Method aMethod, Object[] aArgs) throws Throwable
    {
        Object sResult = null;

        try
        {
            logPublicStart(aMethod, aArgs);
            logSql(aMethod, sResult, aArgs);

            long sStartTime = System.currentTimeMillis();
            sResult = aMethod.invoke(mTarget, aArgs);
            
            logSqlTiming(aMethod, sStartTime);
            sResult = createProxyWithReturnValue(sResult);

            logClose(aMethod);
            logReturnValue(aMethod, sResult);
            logEnd(aMethod);
        }
        catch (InvocationTargetException sEx)
        {
            Throwable sCause = sEx.getTargetException();
            mLogger.log(Level.SEVERE, sCause.getLocalizedMessage(), sCause);
            throw sCause;
        }

        return sResult;
    }

    /**
     * 메소드가 시작하는 시점에 메소드이름과 아규먼트 정보를 로그로 남긴다.</br>
     * 이때 매소드명이 toString인 경우는 무시한다.
     * 
     * @param aMethod
     * @param aArgs
     */
    private void logPublicStart(Method aMethod, Object[] aArgs)
    {
        if (!aMethod.getName().equals(METHOD_PREFIX_TOSTRING))
        {
            mLogger.log(Level.FINE, "{0} Public Start : {1}.{2}", 
                        new Object[] { getUniqueId(), this.mTarget.getClass().getName(), aMethod.getName() } );
            logArgumentInfo(aArgs);
        }
    }

    /**
     * CONFIG레벨로 셋팅되었을 때 sql문을 출력한다.
     * @param aMethod
     * @param aResult
     * @param aArgs
     */
    protected void logSql(Method aMethod, Object aResult, Object[] aArgs)
    {
        // 기본적인 Proxy객체는 sql 로깅을 하지 않는다.
    }
    
    /**
     * sql문이 실행되는데 걸린 시간을 출력한다.
     * @param aMethod
     * @param aStartTime
     */
    protected void logSqlTiming(Method aMethod, long aStartTime)
    {
        // 기본적인 Proxy객체는 sql timing 정보를 로깅하지 않는다.
    }
    
    /**
     * 메소드 실행결과를 이용해 필요하면 Proxy객체를 만들고 로그를 남긴다.
     * @param aResult
     * @return
     */
    protected Object createProxyWithReturnValue(Object aResult)
    {
        Object sProxy;
        
        if (aResult instanceof Connection)
        {
            sProxy = LoggingProxyFactory.createConnectionProxy(aResult);
        }
        else if (aResult instanceof XAResource)
        {
            sProxy = LoggingProxyFactory.createXaResourceProxy(aResult);
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
     * 메소드 실행 결과값을 로그로 남긴다.</br>
     * 메소드명이 toString인 경우는 무시하고 리턴값이 있을때만 FINE레벨로 해당 Object를 출력한다.
     * 
     * @param aMethod
     * @param aResult
     */
    protected void logReturnValue(Method aMethod, Object aResult)
    {
        if (!aMethod.getName().equals(METHOD_PREFIX_TOSTRING))
        {
            if (!aMethod.getReturnType().equals(Void.TYPE))
            {
                if (aMethod.getName().startsWith(METHOD_PREFIX_GET) && mTarget instanceof ResultSet 
                        && mLogger.getLevel() == Level.CONFIG)
                {
                    mLogger.log(Level.CONFIG, "ResultSet.{0} : {1}", new Object[] { aMethod.getName(), aResult });
                }
                mLogger.log(Level.FINE, "{0} Return : {1}", new Object[] { getUniqueId(),aResult });
            }
        }
    }
    
    /**
     * close 메소드를 로깅한다.</br>
     * 이때 해당하는 오브젝트의 unique id를 같이 출력한다.</br>
     * 예를 들어 Connection이나 Statement같은 경우 session id나 stmt id를 출력하고 그 이외의 경우에는</br> 
     * Object의 hashcode를 출력한다.
     * 
     * @param aMethod
     */
    protected void logClose(Method aMethod)
    {
        if (aMethod.getName().startsWith(METHOD_PREFIX_CLOSE))
        {
            mLogger.log(Level.INFO, "{0} {1} closed. ", new Object[] { getUniqueId(), mTargetName });
        }        
    }
    
    private void logEnd(Method aMethod)
    {
        if (!aMethod.getName().equals(METHOD_PREFIX_TOSTRING))
        {
            mLogger.log(Level.FINE, "{0} End", getUniqueId());
        }
    }
    
    /**
     * 아규먼트의 인덱스, 타입, 값 정보를 FINE레벨로 남긴다.
     * 
     * @param aArgs
     * @return
     */
    private void logArgumentInfo(Object[] aArgs)
    {
        if (aArgs == null) return;
        for (int sIdx = 0; sIdx < aArgs.length; sIdx++)
        {
            mLogger.log(Level.FINE, "{0} Argument Idx : {1}", new Object [] { getUniqueId(), String.valueOf(sIdx) });
            if (aArgs[sIdx] == null || aArgs[sIdx] instanceof AltibaseLob)
            {
                continue;
            }
            mLogger.log(Level.FINE, "{0} Argument Type : {1}", new Object [] { getUniqueId(), aArgs[sIdx].getClass().getName() });
            mLogger.log(Level.FINE, "{0} Argument Value : {1}", new Object [] { getUniqueId(), aArgs[sIdx] });
        }
    }

    /**
     * 아규먼트정보를 이용해 String을 구성한다..</br>
     * 이때 아규먼트가 String인 경우에는 "를 추가하여 아래와 같은 형태가 되며 CONFIG레벨로 sql을 출력할때 사용된다.</br>
     * (1, "aaa", "bbb", 222)
     * 
     * @param aArgs
     * @return
     */
    protected String makeArgStr(Object[] aArgs)
    {
        StringBuffer sStr = new StringBuffer();
        for (int sIdx = 0; sIdx < aArgs.length; sIdx++)
        {
            if (sIdx == 0)
            {
                sStr.append('(');
            }
            if (aArgs[sIdx] instanceof String)
            {
                sStr.append('\"');
            }
            
            if (aArgs[sIdx] != null && !(aArgs[sIdx] instanceof AltibaseLob))
            {
                sStr.append(aArgs[sIdx]);
            }
            
            if (aArgs[sIdx] instanceof String)
            {
                sStr.append('\"');
            }
            if (sIdx != aArgs.length - 1)
            {
                sStr.append(", ");
            }
            if (sIdx == aArgs.length - 1)
            {
                sStr.append(')');
            }
        }
        return sStr.toString();
    }
        
    protected String getUniqueId()
    {
        if (mTarget instanceof AltibasePooledConnection)
        {
            return mTarget.toString();
        }
        return "";
    }
}
