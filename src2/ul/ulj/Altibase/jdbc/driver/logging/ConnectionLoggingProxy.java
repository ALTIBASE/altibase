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
import java.sql.CallableStatement;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.util.logging.Level;

import Altibase.jdbc.driver.AltibaseConnection;
import Altibase.jdbc.driver.AltibaseLogicalConnection;

/**
 * java.sql.Connection 인터페이스를 hooking하는 Proxy클래스
 * 
 * @author yjpark
 *
 */
public class ConnectionLoggingProxy extends LoggingProxy
{
    private static final String METHOD_PREFIX_PREPARE = "prepare";

    /**
     * 기본적인 생성자</br>
     * uniqueid를 생성한 후 객체가 생성되었다는 로그를 남긴다.
     * 
     * @param aTarget
     */
    public ConnectionLoggingProxy(Object aTarget)
    {
        super(aTarget, "Connection", JDBC_LOGGER_DEFAULT);
        if (aTarget instanceof AltibaseLogicalConnection)
        {
            super.mTargetName = "LogicalConnection";
            String sSessId = "";
            try
            {
                sSessId = String.valueOf(((AltibaseLogicalConnection)aTarget).getSessionId());
            }
            catch (SQLException se)
            {
            }
            mLogger.log(Level.INFO, "{0} Created {1} {2}.", new Object[] { "[SessId #" + sSessId + "] ",  
                                                                           mTargetName, getUniqueId()});
        }
        else
        {
            mLogger.log(Level.INFO, "Created {0} {1}.", new Object[] { mTargetName, getUniqueId()});
        }
    }
    
    /**
     * java.sql.Connection 인터페이스의 prepare메소드가 호출되었을 때 해당 sql문을 CONFIG레벨로 남긴다.
     */
    public void logSql(Method aMethod, Object aResult, Object[] aArgs)
    {
        if (aMethod.getName().startsWith(METHOD_PREFIX_PREPARE))
        {
            mLogger.log(Level.CONFIG, "{0} Preparing sql : {1}", new Object[] { getUniqueId(), aArgs[0] });
        }
    }

    protected void logSqlTiming(Method aMethod, long aStartTime)
    {
        if (aMethod.getName().startsWith(METHOD_PREFIX_PREPARE))
        {
            long sElapsedtime = System.currentTimeMillis() - aStartTime;
            mLogger.log(Level.CONFIG, "{0} prepared in {1} msec", 
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
        Object sProxyObj;
        
        if (aResult instanceof CallableStatement)
        {
            sProxyObj = LoggingProxyFactory.createCallableStmtProxy(aResult, (Connection)mTarget);
        }
        else if (aResult instanceof PreparedStatement)
        {
            sProxyObj = LoggingProxyFactory.createPreparedStmtProxy(aResult, (Connection)mTarget);
        }
        else if (aResult instanceof Statement)
        {
            sProxyObj = LoggingProxyFactory.createStatementProxy(aResult, (Connection)mTarget);
        }
        else
        {
            sProxyObj = aResult;
        }
        
        // 메소드 실행결과가 SQLWarning인 경우엔 해당 정보를 WARNING레벨로 출력한다.
        if (aResult instanceof SQLWarning)
        {
            mLogger.log(Level.WARNING, "SQLWarning : ", (SQLWarning)aResult);
        }
        
        return sProxyObj;
    }
    
    /**
     * 고유한 아이디를 생성한다.</br>
     * AltibaseConnection인 경우에는 서버에서 받아온 세션ID로 셋팅하고 </br>
     * AltibaseLogicalConnection인 경우에는 그냥 hashcode를 사용한다.
     * 
     * @param aTarget
     */
    public String getUniqueId()
    {
        StringBuffer sUniquieId = new StringBuffer();
        sUniquieId.append("[SessId #");
        
        if (mTarget instanceof AltibaseConnection)
        {
            sUniquieId.append(String.valueOf(((AltibaseConnection)mTarget).getSessionId()));
        }
        else if (mTarget instanceof AltibaseLogicalConnection)
        {
            sUniquieId.append(mTarget.hashCode());
        }
        
        sUniquieId.append("] ");
       
        return sUniquieId.toString();
    }
}
