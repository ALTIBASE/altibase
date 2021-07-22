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

import java.io.PrintWriter;
import java.io.Serializable;
import java.sql.Connection;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.naming.NamingException;
import javax.naming.Reference;
import javax.naming.Referenceable;
import javax.naming.StringRefAddr;
import javax.sql.DataSource;

import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class AltibaseDataSource extends WrapperAdapter implements DataSource, Serializable, Referenceable
{
    private static final long serialVersionUID = 2931597340257536711L;
    private transient Logger mLogger;
    
    protected AltibaseProperties mProperties;
    protected transient PrintWriter mLogWriter;

    public AltibaseDataSource()
    {
        mProperties = new AltibaseProperties();
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_POOL);
        }
    }

    public AltibaseDataSource(String aDataSourceName)
    {
        this();
        setDataSourceName(aDataSourceName);
    }

    public AltibaseDataSource(AltibaseProperties aProp)
    {
        mProperties = aProp;
    }

    public Connection getConnection() throws SQLException
    {
        logConnectionProperties();

        return new AltibaseConnection(mProperties, this);
    }

    // BUG-46790 node connection을 생성할때 호출된다.
    public Connection getConnection(AltibaseShardingConnection aMetaConn) throws SQLException
    {
        logConnectionProperties();

        return new AltibaseConnection(mProperties, this, aMetaConn);
    }

    private void logConnectionProperties()
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger.log(Level.INFO, "Creating a non-pooled connection at {0} ", 
                           AltibaseConnection.getURL(mProperties));
        }
    }

    public Connection getConnection(String aUserName, String aPassword) throws SQLException
    {
        AltibaseProperties sProp = (AltibaseProperties) mProperties.clone();
        sProp.setUser(aUserName);
        sProp.setPassword(aPassword);
        
        Connection conn;
        
        try 
        {
            logConnectionProperties();
            conn = new AltibaseConnection(sProp, this);
        }
        catch (SQLException e)
        {
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.SEVERE, "Failed to create a non-pooled connection for " + 
                            aUserName + "at " + AltibaseConnection.getURL(mProperties), e);
            }
            
            throw e;
        }
        
        return conn;
    }

    public int getLoginTimeout()
    {
        return mProperties.getLoginTimeout();
    }

    public PrintWriter getLogWriter()
    {
        return mLogWriter;
    }
    
    public void setLoginTimeout(int aTimeout)
    {
        mProperties.setLoginTimeout(aTimeout);
    }
    
    public void setLogWriter(PrintWriter aWriter)
    {
        mLogWriter = aWriter;
    }
    
    // #region for Properties

    public String getDataSourceName()
    {
        return mProperties.getDataSource();
    }
    
    public String getServerName()
    {
        return mProperties.getServer();
    }
    
    public int getPortNumber()
    {
        return mProperties.getPort();
    }
    
    public String getDatabaseName()
    {
        return mProperties.getDatabase();
    }
    
    public String getUser()
    {
        return mProperties.getUser();
    }
    
    public String getPassword()
    {
        return mProperties.getPassword();
    }
    
    public String getDescription()
    {
        return mProperties.getDescription();
    }
    
    public String getProperty(String aKey)
    {
        return mProperties.getProperty(aKey);                
    }
    
    public void setURL(String aURL) throws SQLException
    {
        AltibaseDriver.parseURL(aURL, mProperties);
    }
    
    public final void setDataSourceName(String aDataSourceName)
    {
        mProperties.setDataSource(aDataSourceName);
    }
    
    public void setServerName(String aServerName)
    {
        mProperties.setServer(aServerName);
    }

    public void setPortNumber(int aPortNumber)
    {
        mProperties.setPort(aPortNumber);
    }

    public void setDatabaseName(String aDBName)
    {
        mProperties.setDatabase(aDBName);
    }

    public void setUser(String aUser)
    {
        mProperties.setUser(aUser);
    }
    
    public void setPassword(String aPassword)
    {
        mProperties.setPassword(aPassword);
    }
    
    public void setDescription(String aDescription) throws SQLException
    {
        mProperties.setDescription(aDescription);
    }

    public Object setProperty(String aKey, String aValue)
    {
        return mProperties.setProperty(aKey, aValue);
    }

    AltibaseProperties properties()
    {
        return mProperties;
    }

    // #endregion

    /**
     * aDSN에 해당하는 AltibaseDataSource를 얻는다.
     * 
     * @param aDSN Data Source Name (case insensitive)
     * @return aDSN에 해당하는 AltibaseDataSource 객체. 없으면 null.
     */
    public static DataSource getDataSource(String aDSN)
    {
        AltibaseDataSourceManager sManager = AltibaseDataSourceManager.getInstance();
        return sManager.getDataSource(aDSN);
    }

    public Reference getReference() throws NamingException
    {
        Reference ref = new Reference(getClass().getName(), AltibaseDataSourceFactory.class.getName(), null);
        ref.add(new StringRefAddr("properties", mProperties.toString()));
        return ref;
    }

    @Override
    public Logger getParentLogger()
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            return Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
        }

        return null;
    }
}
