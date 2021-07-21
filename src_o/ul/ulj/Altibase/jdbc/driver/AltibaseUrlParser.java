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

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltibaseProperties;

import java.sql.SQLException;
import java.util.Properties;
import java.util.StringTokenizer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * 정규표현식을 이용해 connection string이나 alternate servers string을 파싱한다.
 */
public class AltibaseUrlParser
{
    public static final String   URL_PREFIX              = "jdbc:Altibase_" + AltibaseVersion.CM_VERSION_STRING + "://";

    // URL PATTERN GROUPS
    // 1: Shard Prefix
    // 2: Server or DSN
    // 3: WRAPPER for Port
    // 4: Port
    // 5: WRAPPER for DBName
    // 6: DBName
    // 7: WRAPPER for Properties
    // 8: Properties
    private static final int     URL_GRP_SHARD_PREFIX    = 1;
    private static final int     URL_GRP_SERVER_DSN      = 3;
    private static final int     URL_GRP_PORT            = 5;
    private static final int     URL_GRP_DBNAME          = 7;
    private static final int     URL_GRP_PROPERTIES      = 9;

    private static final String  URL_PATTERN_IP4         = "\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}";
    private static final String  URL_PATTERN_IP6         = "\\[[^\\]]+\\]";
    private static final String  URL_PATTERN_DOMAIN      = "(?:[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]|[a-zA-Z0-9])"
                                                           + "(?:\\.(?:[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]|[a-zA-Z0-9]))*";
    private static final String  URL_PATTERN_DSN         = "[a-zA-Z_][\\w]*";
    private static final String  URL_PATTERN_PORT        = "(:([\\d]+))?";
    private static final String  URL_PATTERN_DBNAME      = "(/(" + URL_PATTERN_DSN + "))?";
    private static final String  URL_PATTERN_PROPS       = "(\\?([a-zA-Z_][\\w]*=[^&]*(&[a-zA-Z_][\\w]*=[^&]*)*))?";
    private static final String  URL_PATTERN_ALT_SERVER  = "(" + URL_PATTERN_IP4 + "|" + URL_PATTERN_IP6 + "|" + URL_PATTERN_DOMAIN + "):[\\d]+" + URL_PATTERN_DBNAME;

    private static final Pattern URL_PATTERN_4PARSE      = Pattern.compile("^jdbc(:sharding)?:Altibase(_" + AltibaseVersion.CM_VERSION_STRING + ")?://("
                                                                           + URL_PATTERN_IP4 + "|"
                                                                           + URL_PATTERN_IP6 + "|"
                                                                           + URL_PATTERN_DOMAIN + "|"
                                                                           + URL_PATTERN_DSN + ")"
                                                                           + URL_PATTERN_PORT
                                                                           + URL_PATTERN_DBNAME
                                                                           + URL_PATTERN_PROPS + "$");
    private static final Pattern URL_PATTERN_4ACCEPTS    = Pattern.compile("^jdbc(:sharding)?:Altibase(_" + AltibaseVersion.CM_VERSION_STRING + ")?://.*$");
    private static final Pattern URL_PATTERN_4VARNAME    = Pattern.compile("^" + URL_PATTERN_DSN + "$");
    private static final Pattern URL_PATTERN_ALT_SERVERS = Pattern.compile("^\\s*\\(\\s*" + URL_PATTERN_ALT_SERVER + "(\\s*,\\s*" + URL_PATTERN_ALT_SERVER + ")*\\s*\\)\\s*$");

    private AltibaseUrlParser()
    {
    }

    public static boolean acceptsURL(String aURL)
    {
        return URL_PATTERN_4ACCEPTS.matcher(aURL).matches();
    }

    /**
     * URL을 파싱해 aDestProp에 property로 설정한다. <br>
     * 이때 URL에 shard prefix가 포함되어 있는 경우 true를 리턴하고 그렇지 않은 경우 false를 리턴한다.
     * <p>
     * 유효한 URL 포맷은 다음과 같다:
     * <ul>
     * <li>jdbc:Altibase://123.123.123.123:20300/mydb</li>
     * <li>jdbc:Altibase://abc.abc.abc.abc:20300/mydb</li>
     * <li>jdbc:Altibase://123.123.123.123/mydb</li>
     * <li>jdbc:Altibase://[::1]:20300/mydb</li>
     * <li>jdbc:Altibase://[::1]:20300/mydb?prop1=val1&prop2=val2</li>
     * <li>jdbc:Altibase://[::1]/mydb</li>
     * <li>jdbc:Altibase://DataSourceName</li>
     * <li>jdbc:Altibase://DataSourceName:20300</li>
     * <li>jdbc:Altibase://DataSourceName:20300?prop1=val1&prop2=val2</li>
     * <li>jdbc:sharding:Altibase://123.123.123.123:20300/mydb</li>
     * </ul>
     * <p>
     * 연결 속성은 connection url에 따라 총 3가지의 설정이 충돌할 수 있는데, 그 때 설정값의 우선순위는 다음과 같다:
     * <ol>
     * <li>connection url로 받은값 (= aURL)</li>
     * <li>기존에 설정된 값 (= aDestProp = connect 메소드로 넘어온 값)</li>
     * <li>altibase_cli.ini 설정 (DSN을 사용하는 경우)</li>
     * </ol>
     *
     * @param aURL connection url
     * @param aDestProp 파싱 결과를 담을 Property
     * @return shard prefix가 포함되어 있는지 여부
     * @throws SQLException URL 구성이 올바르지 않은 경우
     */
    public static boolean parseURL(String aURL, AltibaseProperties aDestProp) throws SQLException
    {
        Matcher sMatcher = URL_PATTERN_4PARSE.matcher(aURL);
        throwErrorForInvalidConnectionUrl(!sMatcher.matches(), aURL);

        String sServerOrDSN = sMatcher.group(URL_GRP_SERVER_DSN);
        throwErrorForInvalidConnectionUrl(sServerOrDSN == null, aURL);

        String sProperties = sMatcher.group(URL_GRP_PROPERTIES);
        if (sProperties != null)
        {
            parseProperties(sProperties, aDestProp);
        }

        String sPort = sMatcher.group(URL_GRP_PORT);
        if (sPort != null)
        {
            aDestProp.setProperty(AltibaseProperties.PROP_PORT, sPort);
        }

        String sDbName = sMatcher.group(URL_GRP_DBNAME);
        if (sDbName == null) // DbName이 없으면 DSN으로 간주
        {
            throwErrorForInvalidConnectionUrl(!URL_PATTERN_4VARNAME.matcher(sServerOrDSN).matches(), aURL);
            aDestProp.setDataSource(sServerOrDSN);
        }
        else
        {
            aDestProp.setProperty(AltibaseProperties.PROP_DBNAME, sDbName);
            aDestProp.setServer(sServerOrDSN);
        }

        // PROJ-2690 sharding prefix가 포함되어 있는 경우 true를 리턴한다.
        return sMatcher.group(URL_GRP_SHARD_PREFIX) != null;
    }

    /**
     * alternate servers string을 파싱해 <tt>AltibaseFailoverServerInfoList</tt>를 만든다.
     * <p>
     * alternate servers string은 "( host_name:port[/dbname][, host_name:port[/dbname]]* )"와 같은 포맷이어야 한다.
     * 예를 들면 다음과 같다:<br />
     * (192.168.3.54:20300, 192.168.3.55:20301)
     * (abc.abc.abc.abc:20300, abc.abc.abc.abc:20301)
     * (192.168.3.54:20300/mydb1, 192.168.3.55:20301/mydb2)
     * (abc.abc.abc.abc:20300/mydb1, abc.abc.abc.abc:20301/mydb2)
     * <p>
     * 만약 alternate servers string에 IPv6 형태의 주소를 쓰고자 한다면, ip를 [, ]로 감싸야 한다.
     * 예를들어, "::1"을 쓰고자 한다면 "[::1]:20300" 처럼 써야지 "::1:20300" 처럼 쓰면 안된다.
     * 이런 경우에는 ParseException을 낸다.
     * <p>
     * alternate servers string이 null이거나 서버 정보가 없어도 null 대신 빈 리스트를 반환한다.
     * 이는 받는쪽에서 null 처리 없이 쓸 수 있게 하기 위함이다.
     *
     * @param aAlternateServersStr 대안 서버 목록을 담은 문자열
     * @return 변환된 <tt>AltibaseFailoverServerInfoList</tt> 객체
     * @throws SQLException alternate servers string의 포맷이 올바르지 않을 경우
    */
    public static AltibaseFailoverServerInfoList parseAlternateServers(String aAlternateServersStr) throws SQLException
    {
        AltibaseFailoverServerInfoList sServerList = new AltibaseFailoverServerInfoList();

        if (aAlternateServersStr == null)
        {
            return sServerList;
        }

        matchAlternateServers(aAlternateServersStr);

        int l = aAlternateServersStr.indexOf('(');
        int r = aAlternateServersStr.lastIndexOf(')');
        String sTokenizableStr = aAlternateServersStr.substring(l + 1, r);

        StringTokenizer sTokenizer = new StringTokenizer(sTokenizableStr, ",");
        while (sTokenizer.hasMoreTokens())
        {
            String sTokenStr = sTokenizer.nextToken();
            int sIdxColon = sTokenStr.lastIndexOf(':');
            String sServer = sTokenStr.substring(0, sIdxColon).trim();
            String sPortStr = sTokenStr.substring(sIdxColon + 1).trim();

            /* BUG-43219 parse dbname property */
            int sIdxSlash = sPortStr.lastIndexOf("/");
            String sDbName = "";
            if (sIdxSlash > 0)
            {
                sDbName = sPortStr.substring(sIdxSlash + 1).trim();
                sPortStr = sPortStr.substring(0, sIdxSlash).trim();
            }

            int sPort = Integer.parseInt(sPortStr);
            sServerList.add(sServer, sPort, sDbName);
        }
        return sServerList;
    }

    /**
     * connection url에 포함된 인자를 파싱해 aDestProp에 property로 설정한다.
     *
     * @param aArg 인자를 담은 "{k}={v}(&{k}={v})*" 형태의 문자열
     * @param aDestProp 파싱 결과를 담을 Property
     * @throws SQLException 문자열 형식이 올바르지 않을 경우
     */
    private static void parseProperties(String aArg, Properties aDestProp) throws SQLException
    {
        try
        {
            String[] sPropExpr = aArg.split("&");
            for (String aSPropExpr : sPropExpr)
            {
                int sEqIndex = aSPropExpr.indexOf("=");
                String sKey = aSPropExpr.substring(0, sEqIndex);
                String sValue = aSPropExpr.substring(sEqIndex + 1, aSPropExpr.length());
                aDestProp.setProperty(sKey, sValue);
            }
        }
        catch (Exception ex)
        {
            Error.throwSQLException(ErrorDef.INVALID_CONNECTION_URL, aArg, ex);
        }
    }

    /**
     * alternateservers url이 정상적인 포맷인지 정규표현식을 이용해 확인한다.<br>
     *
     * @param aAlternateServersStr alternate servers url
     * @throws SQLException INVALID_FORMAT_OF_ALTERNATE_SERVERS
     */
    private static void matchAlternateServers(String aAlternateServersStr) throws SQLException
    {
        Matcher sMatcher = URL_PATTERN_ALT_SERVERS.matcher(aAlternateServersStr);
        if (!sMatcher.matches())
        {
            Error.throwSQLException(ErrorDef.INVALID_FORMAT_OF_ALTERNATE_SERVERS, aAlternateServersStr);
        }
    }

    private static void throwErrorForInvalidConnectionUrl(boolean aTest, String aURL) throws SQLException
    {
        if (aTest)
        {
            Error.throwSQLException(ErrorDef.INVALID_CONNECTION_URL, aURL);
        }
    }
}
