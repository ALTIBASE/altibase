/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.core;

import javax.sql.DataSource;
import java.io.PrintWriter;
import java.sql.Connection;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.Map;
import java.util.logging.Logger;

import Altibase.jdbc.driver.AltibaseUrlParser;
import Altibase.jdbc.driver.WrapperAdapter;
import Altibase.jdbc.driver.cm.CmProtocolContextShardConnect;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class AltibaseShardingDataSource extends WrapperAdapter implements DataSource
{
    private Map<DataNode, DataSource> mDataSourceMap;
    private AltibaseProperties        mProperties;
    private PrintWriter               mLogWriter;

    public AltibaseShardingDataSource()
    {
        mProperties = new AltibaseProperties();
    }

    public Connection getConnection() throws SQLException
    {
        AltibaseShardingConnection sShardCon = new AltibaseShardingConnection(mProperties);
        CmProtocolContextShardConnect sShardContextConnect = sShardCon.getShardContextConnect();
        if (mDataSourceMap != null)
        {
            sShardContextConnect.setDataSourceMap(mDataSourceMap);
        }
        return sShardCon;
    }

    public Connection getConnection(String aUserName, String aPassword) throws SQLException
    {
        AltibaseProperties sProp = (AltibaseProperties) mProperties.clone();
        sProp.setUser(aUserName);
        sProp.setPassword(aPassword);

        return new AltibaseShardingConnection(sProp);
    }

    public PrintWriter getLogWriter()
    {
        return mLogWriter;
    }

    public void setLogWriter(PrintWriter aWriter)
    {
        mLogWriter = aWriter;
    }

    public void setLoginTimeout(int aSeconds)
    {
        mProperties.setLoginTimeout(aSeconds);
    }

    public int getLoginTimeout()
    {
        return mProperties.getLoginTimeout();
    }

    public void setURL(String aURL) throws SQLException
    {
        AltibaseUrlParser.parseURL(aURL, mProperties);
    }

    public void setUser(String aUser)
    {
        mProperties.setUser(aUser);
    }

    public void setServerName(String aServerName)
    {
        mProperties.setServer(aServerName);
    }

    public void setPortNumber(int aPortNumber)
    {
        mProperties.setPort(aPortNumber);
    }

    public void setPassword(String aPassword)
    {
        mProperties.setPassword(aPassword);
    }

    public void setDataSourceMap(Map<DataNode, DataSource> aDataSourceMap)
    {
        mDataSourceMap = aDataSourceMap;
    }

    @Override
    public Logger getParentLogger() throws SQLFeatureNotSupportedException
    {
        return null;
    }
}
