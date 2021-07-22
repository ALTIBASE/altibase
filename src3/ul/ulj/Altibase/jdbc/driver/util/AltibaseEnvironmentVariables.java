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

package Altibase.jdbc.driver.util;

public final class AltibaseEnvironmentVariables
{
    public static final String ENV_ALTIBASE_HOME                    = "ALTIBASE_HOME";
    public static final String ENV_ALTIBASE_PORT                    = "ALTIBASE_PORT_NO";
    public static final String ENV_ALTIBASE_SSL_PORT                = "ALTIBASE_SSL_PORT_NO";
    public static final String ENV_ALTIBASE_IB_PORT                 = "ALTIBASE_IB_PORT_NO";
    public static final String ENV_ALTIBASE_CONNTYPE_FORCE_FOR_TEST = "ALTIBASE_CONNTYPE_FORCE_FOR_TEST";
    public static final String ENV_ALTIBASE_HOST_FORCE_FOR_TEST     = "ALTIBASE_HOST_FORCE_FOR_TEST";
    public static final String ENV_ALTIBASE_TIME_ZONE               = "ALTIBASE_TIME_ZONE";
    public static final String ENV_ALTIBASE_NLS_USE                 = "ALTIBASE_NLS_USE";
    public static final String ENV_ALTIBASE_NCHAR_LITERAL_REPLACE   = "ALTIBASE_NLS_NCHAR_LITERAL_REPLACE";
    public static final String ENV_ALTIBASE_DATE_FORMAT             = "ALTIBASE_DATE_FORMAT";
    public static final String ENV_RESPONSE_TIMEOUT                 = "ALTIBASE_RESPONSE_TIMEOUT";
    public static final String ENV_HOME                             = "HOME";
    public static final String ENV_ALTIBASE_JDBC_TRACE              = "ALTIBASE_JDBC_TRACE";
    public static final String ENV_SOCK_RCVBUF_BLOCK_RATIO          = "ALTIBASE_SOCK_RCVBUF_BLOCK_RATIO"; // PROJ-2625
    public static final String ENV_SHARD_JDBC_DISTTXINFO_VERIFY     = "SHARD_JDBC_DISTTXINFO_VERIFY";     // PROJ-2733

    private AltibaseEnvironmentVariables()
    {
    }

    public static boolean isSet(String aKey)
    {
        return RuntimeEnvironmentVariables.isSet(aKey);
    }

    public static String getAltibaseHome()
    {
        return RuntimeEnvironmentVariables.getVariable(ENV_ALTIBASE_HOME);
    }

    public static int getPort()
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_ALTIBASE_PORT);
    }

    public static int getPort(int aDefaultPort)
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_ALTIBASE_PORT, aDefaultPort);
    }

    public static int getSslPort()
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_ALTIBASE_SSL_PORT);
    }

    public static int getSslPort(int aDefaultSslPort)
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_ALTIBASE_SSL_PORT, aDefaultSslPort);
    }

    public static int getIBPort(int aDefaultIBPort)
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_ALTIBASE_IB_PORT, aDefaultIBPort);
    }

    public static String getTimeZone()
    {
        return RuntimeEnvironmentVariables.getVariable(ENV_ALTIBASE_TIME_ZONE);
    }

    public static String getNlsUse()
    {
        return RuntimeEnvironmentVariables.getVariable(ENV_ALTIBASE_NLS_USE);
    }

    public static boolean useNCharLiteralReplace()
    {
        return RuntimeEnvironmentVariables.getBooleanVariable(ENV_ALTIBASE_NCHAR_LITERAL_REPLACE);
    }

    public static String getHome()
    {
        return RuntimeEnvironmentVariables.getVariable(ENV_HOME);
    }

    public static String getDateFormat()
    {
        return RuntimeEnvironmentVariables.getVariable(ENV_ALTIBASE_DATE_FORMAT);
    }
    
    public static String getConnTypeForceForTest()
    {
        return RuntimeEnvironmentVariables.getVariable(ENV_ALTIBASE_CONNTYPE_FORCE_FOR_TEST);
    }

    public static String getHostForceForTest()
    {
        return RuntimeEnvironmentVariables.getVariable(ENV_ALTIBASE_HOST_FORCE_FOR_TEST);
    }

    public static int getResponseTimeout()
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_RESPONSE_TIMEOUT);
    }

    public static int getResponseTimeout(int aDefaultTimeout)
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_RESPONSE_TIMEOUT, aDefaultTimeout);
    }
    
    // PROJ-2583 get jdbc trace flag from system environment variable
    public static boolean useJdbcTrace()
    {
        return RuntimeEnvironmentVariables.getBooleanVariable(ENV_ALTIBASE_JDBC_TRACE);
    }

    /**
     * ALTIBASE_SOCK_RCVBUF_BLOCK_RATIO 환경변수를 읽는다.
     */
    public static int getSockRcvBufBlockRatio()
    {
        return RuntimeEnvironmentVariables.getIntVariable(ENV_SOCK_RCVBUF_BLOCK_RATIO);
    }

    public static boolean getShardJdbcDisttxinfoVerify()
    {
        return RuntimeEnvironmentVariables.getBooleanVariable(ENV_SHARD_JDBC_DISTTXINFO_VERIFY);
    }
}
