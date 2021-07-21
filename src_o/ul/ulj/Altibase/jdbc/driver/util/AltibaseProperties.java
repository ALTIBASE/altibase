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

import java.sql.DriverPropertyInfo;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import Altibase.jdbc.driver.cm.CmConnType;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.sharding.core.GlobalTransactionLevel;
import static Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection.DEFAULT_SHARD_CLIENT;
import static Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection.DEFAULT_SHARD_SESSION_TYPE;

/**
 * Altibase용 <tt>Properties</tt> 클래스.
 * <p>
 * 대소문자를 구별하지 않는 문자열을 키값으로 쓰며,
 * 편의를 위해 속성 값을 <tt>int</tt>나 <tt>boolean</tt>과 같은 값으로 얻거나 설정하는 메소드를 제공한다.
 */
public class AltibaseProperties extends CaseInsensitiveProperties
{
    private static final long   serialVersionUID                     = 7590410805794341396L;

    private static final int    MAX_OBJECT_NAME_LENGTH               = 128; /* = QC_MAX_OBJECT_NAME_LEN */
    public static final String  PROP_SERVER                          = "server";
    public static final String  PROP_SOCK_BIND_ADDR                  = "sock_bind_addr";
    public static final String  PROP_PORT                            = "port";
    public static final String  PROP_DBNAME                          = "database";
    private static final int    MAX_DBNAME_LENGTH                    = 127; /* = IDP_MAX_PROP_DBNAME_LEN = SM_MAX_DB_NAME - 1 */
    public static final String  PROP_USER                            = "user";
    private static final int    MAX_USER_LENGTH                      = MAX_OBJECT_NAME_LENGTH;
    public static final String  PROP_PASSWORD                        = "password";
    private static final int    MAX_PASSWORD_LENGTH                  = 40; /* = QC_MAX_NAME_LEN */
    public static final String  PROP_CONNECT_MODE                    = "privilege";
    public static final String  PROP_DESCRIPTION                     = "description";
    public static final String  PROP_DATASOURCE_NAME                 = "datasource";
    /** connect를 시도할 때의 timeout(초 단위)(Client side) */
    public static final String  PROP_LOGIN_TIMEOUT                   = "login_timeout";
    /** connect후 서버로부터의 모든 응답 receive 대기 시간(초 단위)(Client side) */
    public static final String  PROP_RESPONSE_TIMEOUT                = "response_timeout";
    /** 쿼리 수행 제한 시간(초 단위)(Server side). 알티베이스 프로퍼티 QUERY_TIMEOUT 참고. */
    public static final String  PROP_QUERY_TIMEOUT                   = "query_timeout";
    /** 세션 유휴 제한 시간(초 단위)(Server side). 알티베이스 프로퍼티 IDLE_TIMEOUT 참고. */
    public static final String  PROP_IDLE_TIMEOUT                    = "idle_timeout";
    /** FETCH 수행 제한 시간(초 단위)(Server side). 알티베이스 프로퍼티 FETCH_TIMEOUT 참고. */
    public static final String  PROP_FETCH_TIMEOUT                   = "fetch_timeout";
    /** 변경 연산(UPDATE, INSERT, DELETE) 수행 제한 시간(초 단위)(Server side). 알티베이스 프로퍼티 UTRANS_TIMEOUT 참고. */
    public static final String  PROP_UTRANS_TIMEOUT                  = "utrans_timeout";
    /** DDL 수행 제한 시간(초 단위)(Server side). 알티베이스 프로퍼티 DDL_TIMEOUT 참고. */
    public static final String  PROP_DDL_TIMEOUT                     = "ddl_timeout";
    public static final String  PROP_AUTO_COMMIT                     = "auto_commit";
    public static final String  PROP_DATE_FORMAT                     = "date_format";
    private static final int    MAX_DATE_FORMAT_LENGTH               = 64; /* = MMC_DATEFORMAT_MAX_LEN = MTC_TO_CHAR_MAX_PRECISION */
    public static final String  PROP_NCHAR_LITERAL_REPLACE           = "ncharliteralreplace";
    /** 한 session당 생성할수 있는 최대 statement 갯수 제약 */
    public static final String  PROP_MAX_STATEMENTS_PER_SESSION      = "max_statements_per_session";
    public static final String  PROP_APP_INFO                        = "app_info";
    private static final int    MAX_APP_INFO_LENGTH                  = 128; /* = MMC_APPINFO_MAX_LEN */
    public static final String  PROP_TXI_LEVEL                       = "isolation_level";
    /** Failover를 위한 대안 서버 목록 */
    public static final String  PROP_ALT_SERVERS                     = "alternateservers";
    public static final String  PROP_LOAD_BALANCE                    = "loadbalance";
    public static final boolean DEFAULT_LOAD_BALANCE                 = false;
    public static final String  PROP_CTF_RETRY_COUNT                 = "connectionretrycount";
    public static final String  PROP_CTF_RETRY_DELAY                 = "connectionretrydelay";
    public static final String  PROP_FAILOVER_USE_STF                = "sessionfailover";
    public static final String  PROP_FAILOVER_SOURCE                 = "failover_source";
    private static final int    MAX_FAILOVER_SOURCE_LENGTH           = 256; /* = MMC_FAILOVER_SOURCE_MAX_LEN */
    // IPV6
    public static final String  PROP_PREFER_IPV6                     = "prefer_ipv6";
    public static final boolean DEFAULT_PREFER_IPV6                  = false;
    /** Client Side Auto Commit 여부 */
    public static final String  PROP_CLIENT_SIDE_AUTO_COMMIT         = "clientside_auto_commit";
    public static final boolean DEFAULT_CLIENT_SIDE_AUTO_COMMIT      = false;
    // TimeZone
    public static final String  PROP_TIME_ZONE                       = "time_zone";
    private static final int    MAX_TIME_ZONE_LENGTH                 = 40; /* = MMC_TIMEZONE_MAX_LEN = MTC_TIMEZONE_NAME_LEN */
    public static final String  DB_TIME_ZONE                         = "DB_TZ";
    public static final String  LOCAL_TIME_ZONE                      = "OS_TZ";
    public static final String  DEFAULT_TIME_ZONE                    = DB_TIME_ZONE;
    
    public static final String  PROP_LOB_CACHE_THRESHOLD             = "lob_cache_threshold";
    public static final int     MIN_LOB_CACHE_THRESHOLD              = 0;
    public static final int     MAX_LOB_CACHE_THRESHOLD              = 512 * 1024;  /* BUG-46411 */

    public static final String  PROP_REMOVE_REDUNDANT_TRANSMISSION   = "remove_redundant_transmission";
    public static final int     DEFAULT_REMOVE_REDUNDANT_TRANSMISSION = 0;
    
    /** deferred prepare 사용 여부 */
    public static final String  PROP_DEFERRED_PREPARE                = "defer_prepares";
    public static final boolean DEFAULT_DEFERRED_PREPARE             = false;

    /** PROJ-2681 */
    public static final String PROP_CONNTYPE                         = "conntype";
    public static final String DEFAULT_CONNTYPE                      = "TCP";

    public static final String PROP_IB_LATENCY                       = "ib_latency";
    public static final int DEFAULT_IB_LATENCY                       = 0;

    public static final String PROP_IB_CONCHKSPIN                    = "ib_conchkspin";
    public static final int DEFAULT_IB_CONCHKSPIN                    = 0;

    /** PROJ-2474 ssl 보안 통신을 사용할지 여부 */
    public static final String  PROP_SSL_ENABLE                      = "ssl_enable";
    public static final boolean DEFAULT_SSL_ENABLE                   = false;
    
    /** Statement를 만들 때 기본으로 사용할 Fetch Size */
    public static final String  PROP_FETCH_ENOUGH                    = "fetch_enough";
    /** 딱히 갯수를 정해두지 않고, 한번 통신으로 얻을 수 있을 만큼 얻는다. */
    public static final int     DEFAULT_FETCH_ENOUGH                 = 0;

    /* PROJ-2690 ShardJDBC */
    public static final String PROP_SHARD_CONNTYPE                   = "shard_conntype";
    public static final String PROP_GLOBAL_TRANSACTION_LEVEL         = "global_transaction_level";
    public static final short  DEFAULT_GLOBAL_TRANSACTION_LEVEL      = GlobalTransactionLevel.NONE.getValue(); 
    public static final String PROP_SHARD_PIN                        = "shardpin";
    public static final String PROP_SHARD_NODE_NAME                  = "shard_node_name";
    public static final String PROP_SHARD_META_NUMBER                = "shard_meta_number";
    public static final String PROP_SHARD_LAZYCONNECT                = "shard_lazy_connect";

    // BUG-47460 resharding을 옵션처리하기 위한 속성
    public static final String PROP_RESHARD_ENABLE                   = "reshard_enable";
    // BUG-47324
    public static final String PROP_SHARD_CLIENT                     = "shard_client";
    public static final String PROP_SHARD_SESSION_TYPE               = "shard_session_type";

    // PROJ-2733
    public static final String PROP_SHARD_STATEMENT_RETRY            = "shard_statement_retry";
    public static final short  DEFAULT_SHARD_STATEMENT_RETRY         = 1; 
    public static final String PROP_INDOUBT_FETCH_TIMEOUT            = "indoubt_fetch_timeout";
    public static final String PROP_INDOUBT_FETCH_METHOD             = "indoubt_fetch_method";

    // BUG-47639 lob 컬럼값이 null일때 Lob 객체를 리턴할 수 있는지 여부
    public static final String PROP_LOB_NULL_SELECT                  = "lob_null_select";

    // BUG-48380 같은 PreparedStatement에서 executeQuery를 했을 때 ResultSet을 재사용할지 여부
    public static final String PROP_REUSE_RESULTSET                  = "reuse_resultset";

    /* BUG-37642 Improve performance to fetch */
    public static final int     MAX_FETCH_ENOUGH                     = Integer.MAX_VALUE;
    public static final String  RANGE_FETCH_ENOUGH                   = DEFAULT_FETCH_ENOUGH +
                                                                       " ~ " +
                                                                       MAX_FETCH_ENOUGH;

    // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
    public static final String  PROP_FETCH_ASYNC                     = "fetch_async";
    public static final String  PROP_FETCH_AUTO_TUNING               = "fetch_auto_tuning";
    public static final String  PROP_FETCH_AUTO_TUNING_INIT          = "fetch_auto_tuning_init";
    public static final String  PROP_FETCH_AUTO_TUNING_MIN           = "fetch_auto_tuning_min";
    public static final String  PROP_FETCH_AUTO_TUNING_MAX           = "fetch_auto_tuning_max";
    public static final String  PROP_SOCK_RCVBUF_BLOCK_RATIO         = "sock_rcvbuf_block_ratio";

    public static final String  PROP_VALUE_FETCH_ASYNC_OFF           = "off";
    public static final String  PROP_VALUE_FETCH_ASYNC_PREFERRED     = "preferred";

    /* BUG-48892 DatabaseMetaData.getProcedures()의 결과에 function이 포함될지 여부 */
    public static final String  PROP_GETPROCEDURES_RETURN_FUNCTIONS  = "getprocedures_return_functions";

    public static final String  DEFAULT_FETCH_ASYNC                  = PROP_VALUE_FETCH_ASYNC_OFF;
    public static final boolean DEFAULT_FETCH_AUTO_TUNING            = false;
    public static final int     DEFAULT_FETCH_AUTO_TUNING_INIT       = 0;
    public static final int     DEFAULT_FETCH_AUTO_TUNING_MIN        = 1;
    public static final int     DEFAULT_FETCH_AUTO_TUNING_MAX        = Integer.MAX_VALUE;
    public static final int     DEFAULT_SOCK_RCVBUF_BLOCK_RATIO      = 0;
    public static final int     MIN_SOCK_RCVBUF_BLOCK_RATIO          = 0;
    public static final int     MAX_SOCK_RCVBUF_BLOCK_RATIO          = 65535; // 2^16

    public static final String  DEFAULT_SERVER                       = "localhost";
    public static final int     DEFAULT_PORT                         = 20300;
    // PROJ-2474 SSL 통신을 위한 기본포트 선언 (기본값:20443)
    public static final int     DEFAULT_SSL_PORT                     = 20443;
    public static final int     DEFAULT_IB_PORT                      = DEFAULT_PORT;
    public static final String  DEFAULT_DBNAME                       = "mydb";
    public static final int     DEFAULT_LOGIN_TIMEOUT                = 0;
    public static final boolean DEFAULT_AUTO_COMMIT                  = true;
    public static final boolean DEFAULT_NCHAR_LITERAL_REPLACE        = false;
    public static final boolean DEFAULT_FAILOVER_USE_STF             = false;

    public static final byte    PROP_CODE_CLIENT_PACKAGE_VERSION          = 0;
    public static final byte    PROP_CODE_CLIENT_PROTOCOL_VERSION         = 1;
    public static final byte    PROP_CODE_CLIENT_PID                      = 2;
    public static final byte    PROP_CODE_CLIENT_TYPE                     = 3;
    public static final byte    PROP_CODE_APP_INFO                        = 4;
    public static final byte    PROP_CODE_NLS                             = 5;
    public static final byte    PROP_CODE_AUTOCOMMIT                      = 6;
    public static final byte    PROP_CODE_EXPLAIN_PLAN                    = 7;
    public static final byte    PROP_CODE_ISOLATION_LEVEL                 = 8;
    public static final byte    PROP_CODE_OPTIMIZER_MODE                  = 9;
    public static final byte    PROP_CODE_HEADER_DISPLAY_MODE             = 10;
    public static final byte    PROP_CODE_STACK_SIZE                      = 11;
    public static final byte    PROP_CODE_IDLE_TIMEOUT                    = 12;
    public static final byte    PROP_CODE_QUERY_TIMEOUT                   = 13;
    public static final byte    PROP_CODE_FETCH_TIMEOUT                   = 14;
    public static final byte    PROP_CODE_UTRANS_TIMEOUT                  = 15;
    public static final byte    PROP_CODE_DATE_FORMAT                     = 16;
    public static final byte    PROP_CODE_NORMALFORM_MAXIMUM              = 17;
    public static final byte    PROP_CODE_SERVER_PACKAGE_VERSION          = 18;
    public static final byte    PROP_CODE_NLS_NCHAR_LITERAL_REPLACE       = 19;
    public static final byte    PROP_CODE_NLS_CHARACTERSET                = 20;
    public static final byte    PROP_CODE_NLS_NCHAR_CHARACTERSET          = 21;
    public static final byte    PROP_CODE_ENDIAN                          = 22;
    public static final byte    PROP_CODE_MAX_STATEMENTS_PER_SESSION      = 23; /* BUG-31144 */
    public static final byte    PROP_CODE_FAILOVER_SOURCE                 = 24; /* BUG-31390 */
    public static final byte    PROP_CODE_DDL_TIMEOUT                     = 25; /* BUG-32885 */
    public static final byte    PROP_CODE_TIME_ZONE                       = 26; /* PROJ-2209 */
    public static final byte    PROP_CODE_FETCH_PROTOCOL_TYPE             = 27;
    public static final byte    PROP_CODE_REMOVE_REDUNDANT_TRANSMISSION   = 28;
    public static final byte    PROP_CODE_LOB_CACHE_THRESHOLD             = 29; /* PROJ-2047 */
    public static final byte    PROP_CODE_SHARD_NODE_NAME                 = 32; /* PROJ-2690 ShardJDBC */
    public static final byte    PROP_CODE_SHARD_PIN                       = 33; /* PROJ-2690 ShardJDBC */
    public static final byte    PROP_CODE_GLOBAL_TRANSACTION_LEVEL        = 34; /* PROJ-2690 ShardJDBC */
    public static final byte    PROP_CODE_SHARD_META_NUMBER               = 35; /* PROJ-2690 ShardJDBC */
    public static final byte    PROP_CODE_SHARD_CLIENT                    = 36; /* PROJ-2690 ShardJDBC */
    public static final byte    PROP_CODE_SHARD_SESSION_TYPE              = 37; /* BUG-46790 internal인지 external인지 구분하는 용도 */
    public static final byte    PROP_CODE_SHARD_CLIENT_CONNECTION_REPORT  = 38; /* BUG-46790 shardfailover report를 위한 속성값. => BUG-46785 Deprecated */
    public static final byte    PROP_CODE_MESSAGE_CALLBACK                = 39; /* BUG-46019 */
    public static final byte    PROP_CODE_SHARD_STATEMENT_RETRY           = 75; /* PROJ-2733 */
    public static final byte    PROP_CODE_INDOUBT_FETCH_TIMEOUT           = 76; /* PROJ-2733 */
    public static final byte    PROP_CODE_INDOUBT_FETCH_METHOD            = 77; /* PROJ-2733 */
    public static final byte    PROP_CODE_MAX                             = 78;


    public AltibaseProperties()
    {
    }

    public AltibaseProperties(Properties aProp)
    {
        super(aProp);
    }

    protected Properties createProperties()
    {
        return new AltibaseProperties();
    }

    public synchronized Object put(Object aKey, Object aValue)
    {
        String sKey = aKey.toString();
        if (PROP_DBNAME.equalsIgnoreCase(sKey))
        {
            checkLengthAndThrow(sKey, aValue, MAX_DBNAME_LENGTH);
        }
        else if (PROP_USER.equalsIgnoreCase(sKey))
        {
            checkLengthAndThrow(sKey, aValue, MAX_USER_LENGTH);
        }
        else if (PROP_PASSWORD.equalsIgnoreCase(sKey))
        {
            checkLengthAndThrow(sKey, aValue, MAX_PASSWORD_LENGTH);
        }
        else if (PROP_DATE_FORMAT.equalsIgnoreCase(sKey))
        {
            checkLengthAndThrow(sKey, aValue, MAX_DATE_FORMAT_LENGTH);
        }
        else if (PROP_APP_INFO.equalsIgnoreCase(sKey))
        {
            checkLengthAndThrow(sKey, aValue, MAX_APP_INFO_LENGTH);
        }
        else if (PROP_FAILOVER_SOURCE.equalsIgnoreCase(sKey))
        {
            checkLengthAndThrow(sKey, aValue, MAX_FAILOVER_SOURCE_LENGTH);
        }
        else if (PROP_TIME_ZONE.equalsIgnoreCase(sKey))
        {
            checkLengthAndThrow(sKey, aValue, MAX_TIME_ZONE_LENGTH);
        }
        else if (PROP_SOCK_RCVBUF_BLOCK_RATIO.equalsIgnoreCase(sKey)) // PROJ-2625
        {
            checkValueAndThrow(sKey, aValue, MIN_SOCK_RCVBUF_BLOCK_RATIO, MAX_SOCK_RCVBUF_BLOCK_RATIO);
        }
        else if (PROP_LOB_CACHE_THRESHOLD.equalsIgnoreCase(sKey))  /* BUG-46411 */
        {
            checkValueAndThrow(sKey, aValue, MIN_LOB_CACHE_THRESHOLD, MAX_LOB_CACHE_THRESHOLD);
        }
        return super.put(aKey, aValue);
    }

    private void checkLengthAndThrow(String aKey, Object aValue, int aMaxLength)
    {
        if (aValue != null)
        {
            String sStrVal = (String)aValue;
            if (sStrVal.length() > aMaxLength)
            {
                throw new IllegalArgumentException("The length of the value set for "+ aKey.toLowerCase() +" exceeds the limit("+ aMaxLength +").");
            }
        }
    }

    private void checkValueAndThrow(String aKey, Object aValue, int aMinValue, int aMaxValue)
    {
        if (aValue != null)
        {
            int sIntValue = Integer.parseInt((String)aValue);
            if (sIntValue < aMinValue || aMaxValue < sIntValue)
            {
                throw new IllegalArgumentException("The range of the value for '"+ aKey.toLowerCase() +"' property is ("+ aMinValue +" ~ " + aMaxValue + ").");
            }
        }
    }

    public boolean isSet(String aKey)
    {
        return getProperty(aKey) != null;
    }

    /**
     * 속성 값을 <tt>int</tt>로 얻는다.
     * 
     * @param aKey 키값
     * @return <tt>int</tt>로 변환한 속성값, 속성값이 지정되어있지 않거나 올바르지 않은 포맷이면 0
     */
    public int getIntProperty(String aKey)
    {
        return getIntProperty(aKey, 0);
    }

    /**
     * 속성 값을 <tt>int</tt>로 얻는다.
     *
     * @param aKey 키값
     * @param aDefaultValue 기본값
     * @return <tt>int</tt>로 변환한 속성값, 속성값이 지정되어있지 않거나 올바르지 않은 포맷이면 기본값
     */
    public int getIntProperty(String aKey, int aDefaultValue)
    {
        int sIntValue = aDefaultValue;
        String sStrValue = getProperty(aKey);
        if (sStrValue != null)
        {
            try
            {
                sIntValue = Integer.parseInt(sStrValue);
            }
            catch (NumberFormatException ex)
            {
                // LOGGING
            }
        }
        return sIntValue;
    }

    /**
     * 속성 값을 <tt>short</tt>으로 얻는다.
     * 
     * @param aKey 키값
     * @return <tt>short</tt>으로 변환한 속성값, 속성값이 지정되어있지 않거나 올바르지 않은 포맷이면 0
     */
    public short getShortProperty(String aKey)
    {
        return getShortProperty(aKey, (short) 0);
    }

    /**
     * 속성 값을 <tt>short</tt>으로 얻는다.
     *
     * @param aKey 키값
     * @param aDefaultValue 기본값
     * @return <tt>short</tt>으로 변환한 속성값, 속성값이 지정되어있지 않거나 올바르지 않은 포맷이면 기본값
     */
    public short getShortProperty(String aKey, short aDefaultValue)
    {
        short sValue = aDefaultValue;
        String sPropValue = getProperty(aKey);
        if (sPropValue != null)
        {
            try
            {
                sValue = Short.parseShort(sPropValue);
            }
            catch (NumberFormatException ex)
            {
                // LOGGING
            }
        }
        return sValue;
    }

    /**
     * 속성 값을 <tt>long</tt>로 얻는다.
     * 
     * @param aKey 키값
     * @return <tt>long</tt>로 변환한 속성값, 속성값이 지정되어있지 않거나 올바르지 않은 포맷이면 0
     */
    public long getLongProperty(String aKey)
    {
        return getLongProperty(aKey, 0);
    }

    /**
     * 속성 값을 <tt>long</tt>으로 얻는다.
     *
     * @param aKey 키값
     * @param aDefaultValue 기본값
     * @return <tt>long</tt>으로 변환한 속성값, 속성값이 지정되어있지 않거나 올바르지 않은 포맷이면 기본값
     */
    public long getLongProperty(String aKey, long aDefaultValue)
    {
        long sLongValue = aDefaultValue;
        String sStrValue = getProperty(aKey);
        if (sStrValue != null)
        {
            try
            {
                sLongValue = Long.parseLong(sStrValue);
            }
            catch (NumberFormatException ex)
            {
                // LOGGING
            }
        }
        return sLongValue;
    }

    /**
     * 속성 값을 <tt>booelan</tt>으로 얻는다.
     * 
     * @param aKey 키값
     * @return <tt>booelan</tt>로 변환한 속성값, 속성값이 지정되어있지 않거나 속성값을 <tt>true</tt>라고 추정할 수 없다면 <tt>false</tt>
     */
    public boolean getBooleanProperty(String aKey)
    {
        return getBooleanProperty(aKey, false);
    }

    /**
     * 속성 값을 <tt>booelan</tt>으로 얻는다.
     *
     * @param aKey 키값
     * @param aDefaultValue 기본값
     * @return <tt>true</tt>라고 추정할 수 있는 값(1, TRUE, T, ON, O, YES, Y)이면 <tt>true</tt>, 아니면 <tt>false</tt>.
     *          속성값이 지정되어있지 않다면 기본값
     */
    public boolean getBooleanProperty(String aKey, boolean aDefaultValue)
    {
        boolean sValue = aDefaultValue;
        String sPropValue = getProperty(aKey);
        if (sPropValue != null)
        {
            sPropValue = sPropValue.toUpperCase();
            if (sPropValue.equals("1") ||
                sPropValue.equals("TRUE") ||
                sPropValue.equals("T") ||
                sPropValue.equals("ON") ||
                sPropValue.equals("O") ||
                sPropValue.equals("YES") ||
                sPropValue.equals("Y"))
            {
                sValue = true;
            }
            else // 0, FALSE, F, OFF, X, NO, N, ...
            {
                sValue = false;
            }
        }
        return sValue;
    }

    /**
     * <tt>boolean</tt>으로 속성값을 설정한다.
     * 
     * @param aKey 속성 구별을 위한 키값
     * @param aValue 키값에 설정할 값
     * @return 이전에 설정되있던 값. 만약 전에 설정되어있던 값이 없었다면 null.
     */
    public Object setProperty(String aKey, boolean aValue)
    {
        return setProperty(aKey, (aValue == true ? "1" : "0"));
    }

    /**
     * <tt>int</tt>으로 속성값을 설정한다.
     * 
     * @param aKey 속성 구별을 위한 키값
     * @param aValue 키값에 설정할 값
     * @return 이전에 설정되있던 값. 만약 전에 설정되어있던 값이 없었다면 null.
     */
    public Object setProperty(String aKey, int aValue)
    {
        return setProperty(aKey, Integer.toString(aValue));
    }

    /**
     * <tt>short</tt>으로 속성값을 설정한다.
     * 
     * @param aKey 속성 구별을 위한 키값
     * @param aValue 키값에 설정할 값
     * @return 이전에 설정되있던 값. 만약 전에 설정되어있던 값이 없었다면 null.
     */
    public Object setProperty(String aKey, short aValue)
    {
        return setProperty(aKey, Short.toString(aValue));
    }

    // #region 편의를 위한 랩퍼

    public String getServer()
    {
        return getProperty(PROP_SERVER);
    }

    public void setServer(String aServer)
    {
        setProperty(PROP_SERVER, aServer);
    }

    public String getSockBindAddr()
    {
        return getProperty(PROP_SOCK_BIND_ADDR);
    }

    public void setSockBindAddr(String aSockBindAddr)
    {
        setProperty(PROP_SOCK_BIND_ADDR, aSockBindAddr);
    }

    public int getPort()
    {
        return getIntProperty(PROP_PORT);
    }

    public void setPort(int aPort)
    {
        setProperty(PROP_PORT, aPort);
    }

    public String getDatabase()
    {
        return getProperty(PROP_DBNAME);
    }

    public void setDatabase(String aDatabase)
    {
        setProperty(PROP_DBNAME, aDatabase);
    }

    public String getUser()
    {
        return getProperty(PROP_USER);
    }

    public void setUser(String aUser)
    {
        setProperty(PROP_USER, aUser);
    }

    public String getPassword()
    {
        return getProperty(PROP_PASSWORD);
    }

    public void setPassword(String aPassword)
    {
        setProperty(PROP_PASSWORD, aPassword);
    }

    public String getPrivilege()
    {
        return getProperty(PROP_CONNECT_MODE);
    }

    public void setPrivilege(String aPrivilege)
    {
        setProperty(PROP_CONNECT_MODE, aPrivilege);
    }

    public String getDescription()
    {
        return getProperty(PROP_DESCRIPTION);
    }

    public void setDescription(String aDescription)
    {
        setProperty(PROP_DESCRIPTION, aDescription);
    }

    public String getDataSource()
    {
        return getProperty(PROP_DATASOURCE_NAME);
    }

    public String getShardNodeName()
    {
        return getProperty(PROP_SHARD_NODE_NAME);
    }

    public long getShardPin()
    {
        return getLongProperty(PROP_SHARD_PIN);
    }

    public byte getShardClient()
    {
        String sValue = getProperty(PROP_SHARD_CLIENT);
        if (sValue != null)
        {
            return Byte.parseByte(sValue);
        }

        return DEFAULT_SHARD_CLIENT;
    }

    public byte getShardSessionType()
    {
        String sValue = getProperty(PROP_SHARD_SESSION_TYPE);
        if (sValue != null)
        {
            return Byte.parseByte(sValue);
        }

        return DEFAULT_SHARD_SESSION_TYPE;
    }
    
    public void setDataSource(String aDataSource)
    {
        setProperty(PROP_DATASOURCE_NAME, aDataSource);
    }

    public int getLoginTimeout()
    {
        return getIntProperty(PROP_LOGIN_TIMEOUT, DEFAULT_LOGIN_TIMEOUT);
    }

    public void setLoginTimeout(int aLoginTimeout)
    {
        setProperty(PROP_LOGIN_TIMEOUT, aLoginTimeout);
    }

    public int getResponseTimeout()
    {
        return getIntProperty(PROP_RESPONSE_TIMEOUT);
    }

    public void setResponseTimeout(int aResponseTimeout)
    {
        setProperty(PROP_RESPONSE_TIMEOUT, aResponseTimeout);
    }

    public int getQueryTimeout()
    {
        return getIntProperty(PROP_QUERY_TIMEOUT);
    }

    public void setQueryTimeout(int aQueryTimeout)
    {
        setProperty(PROP_QUERY_TIMEOUT, aQueryTimeout);
    }

    public int getIdleTimeout()
    {
        return getIntProperty(PROP_IDLE_TIMEOUT);
    }

    public void setIdleTimeout(int aIdleTimeout)
    {
        setProperty(PROP_IDLE_TIMEOUT, aIdleTimeout);
    }

    public int getFetchTimeout()
    {
        return getIntProperty(PROP_FETCH_TIMEOUT);
    }

    public void setFetchTimeout(int aFetchTimeout)
    {
        setProperty(PROP_FETCH_TIMEOUT, aFetchTimeout);
    }

    public int getUtransTimeout()
    {
        return getIntProperty(PROP_UTRANS_TIMEOUT);
    }

    public void setUtransTimeout(int aUtransTimeout)
    {
        setProperty(PROP_UTRANS_TIMEOUT, aUtransTimeout);
    }

    public boolean isAutoCommit()
    {
        return getBooleanProperty(PROP_AUTO_COMMIT, DEFAULT_AUTO_COMMIT);
    }

    public void setAutoCommit(boolean aAutoCommit)
    {
        setProperty(PROP_AUTO_COMMIT, aAutoCommit);
    }

    public String getDateFormat()
    {
        return getProperty(PROP_DATE_FORMAT);
    }

    public void setDateFormat(String aDateFormat)
    {
        setProperty(PROP_DATE_FORMAT, aDateFormat);
    }

    public boolean useNCharLiteralReplace()
    {
        return getBooleanProperty(PROP_NCHAR_LITERAL_REPLACE, DEFAULT_NCHAR_LITERAL_REPLACE);
    }

    public void setNCharLiteralReplace(boolean aNCharLiteralReplace)
    {
        setProperty(PROP_NCHAR_LITERAL_REPLACE, aNCharLiteralReplace);
    }

    public int getMaxStatementsPerSession()
    {
        return getIntProperty(PROP_MAX_STATEMENTS_PER_SESSION);
    }

    public void setMaxStatementsPerSession(int aMaxStatementsPerSession)
    {
        setProperty(PROP_MAX_STATEMENTS_PER_SESSION, aMaxStatementsPerSession);
    }

    public String getAppInfo()
    {
        return getProperty(PROP_APP_INFO);
    }

    public void setAppInfo(String aAppInfo)
    {
        setProperty(PROP_APP_INFO, aAppInfo);
    }

    public int getIsolationLevel()
    {
        return getIntProperty(PROP_TXI_LEVEL);
    }

    public void setIsolationLevel(int aIsolationLevel)
    {
        setProperty(PROP_TXI_LEVEL, aIsolationLevel);
    }

    public String getAlternateServer()
    {
        return getProperty(PROP_ALT_SERVERS);
    }

    public void setAlternateServer(String aAlternateServer)
    {
        setProperty(PROP_ALT_SERVERS, aAlternateServer);
    }

    public boolean useLoadBalance()
    {
        return getBooleanProperty(PROP_LOAD_BALANCE, DEFAULT_LOAD_BALANCE);
    }

    public void setLoadBalance(boolean aLoadBalance)
    {
        setProperty(PROP_LOAD_BALANCE, aLoadBalance);
    }

    public int getConnectionRetryCount()
    {
        return getIntProperty(PROP_CTF_RETRY_COUNT);
    }

    public void getConnectionRetryCount(int aConnectionRetryCount)
    {
        setProperty(PROP_CTF_RETRY_COUNT, aConnectionRetryCount);
    }

    public int getConnectionRetryDelay()
    {
        return getIntProperty(PROP_CTF_RETRY_DELAY);
    }

    public void setConnectionRetryDelay(int aConnectionRetryDelay)
    {
        setProperty(PROP_CTF_RETRY_DELAY, aConnectionRetryDelay);
    }

    public String getFailoverSource()
    {
        return getProperty(PROP_FAILOVER_SOURCE);
    }

    public void setFailoverSource(String aFailoverSource)
    {
        setProperty(PROP_FAILOVER_SOURCE, aFailoverSource);
    }

    public boolean useSessionFailover()
    {
        return getBooleanProperty(PROP_FAILOVER_USE_STF, DEFAULT_FAILOVER_USE_STF);
    }

    public void setSessionFailover(boolean aSessionFailover)
    {
        setProperty(PROP_FAILOVER_USE_STF, aSessionFailover);
    }

    public String getTimeZone()
    {
        return getProperty(PROP_TIME_ZONE);
    }

    public void setTimeZone(String aTimeZone)
    {
        setProperty(PROP_TIME_ZONE, aTimeZone);
    }

    public int getFetchEnough()
    {
        return getIntProperty(PROP_FETCH_ENOUGH, DEFAULT_FETCH_ENOUGH);
    }

    public GlobalTransactionLevel getGlobalTransactionLevel()
    {
        short sTransactionLevel = getShortProperty(PROP_GLOBAL_TRANSACTION_LEVEL, DEFAULT_GLOBAL_TRANSACTION_LEVEL);
        return GlobalTransactionLevel.get(sTransactionLevel);
    }

    public void setGlobalTransactionLevel(short aTransactionLevel)
    {
        setProperty(PROP_GLOBAL_TRANSACTION_LEVEL, aTransactionLevel);
    }

    public short getShardStatementRetry()
    {
        return getShortProperty(PROP_SHARD_STATEMENT_RETRY, DEFAULT_SHARD_STATEMENT_RETRY);
    }

    public void setShardStatementRetry(short aShardStatementRetry)
    {
        setProperty(PROP_SHARD_STATEMENT_RETRY, aShardStatementRetry);
    }

    public int getIndoubtFetchTimeout()
    {
        return getIntProperty(PROP_INDOUBT_FETCH_TIMEOUT);
    }

    public void setIndoubtFetchTimeout(int aIndoubtFetchTimeout)
    {
        setProperty(PROP_INDOUBT_FETCH_TIMEOUT, aIndoubtFetchTimeout);
    }

    public short getIndoubtFetchMethod()
    {
        return getShortProperty(PROP_INDOUBT_FETCH_METHOD);
    }

    public void setIndoubtFetchMethod(short aIndoubtFetchMethod)
    {
        setProperty(PROP_INDOUBT_FETCH_METHOD, aIndoubtFetchMethod);
    }

    /* unused */
    public void setFetchEnough(int aFetchEnough)
    {
        setProperty(PROP_FETCH_ENOUGH, aFetchEnough);
    }

    public boolean isPreferIPv6()
    {
        return getBooleanProperty(PROP_PREFER_IPV6, DEFAULT_PREFER_IPV6);
    }

    public void setPreferIPv6(boolean aPreferIPv6)
    {
        setProperty(PROP_PREFER_IPV6, aPreferIPv6);
    }

    /**
     * ClientAutoCommit이 셋팅되어 있는지 확인한다.
     * @return true clientside_auto_commit이 활성화되어 있다. <br/> false clientside_auto_commit 이 false이거나 셋팅되지 않았을때
     */
    public boolean isClientSideAutoCommit()
    {
        return getBooleanProperty(PROP_CLIENT_SIDE_AUTO_COMMIT, DEFAULT_CLIENT_SIDE_AUTO_COMMIT);
    }
    
    /**
     * Deferred Prepare가 활성화 되어 있는지 확인한다.
     * @return
     */
    public boolean isDeferredPrepare()
    {
        return getBooleanProperty(PROP_DEFERRED_PREPARE, DEFAULT_DEFERRED_PREPARE);
    }

    public boolean isOnRedundantDataTransmission()
    {
        return getIntProperty(PROP_REMOVE_REDUNDANT_TRANSMISSION, DEFAULT_REMOVE_REDUNDANT_TRANSMISSION) == 1 ? true : false;
    }
    
    public int getLobCacheThreshold()
    {
        return getIntProperty(PROP_LOB_CACHE_THRESHOLD);
    }

    /**
     * Property 로 설정된 비동기 fetch 방식을 얻는다.
     * 
     * @return <code>0</code>, if synchronous prefetch.
     *         <code>1</code>, if semi-async prefetch.
     */
    public String getFetchAsync()
    {
        return getProperty(PROP_FETCH_ASYNC, DEFAULT_FETCH_ASYNC);
    }

    /**
     * Property 로 설정된 auto-tuning 활성화 여부를 확인한다.
     * 
     * @return <code>true</code>, if auto-tuning is enable.
     *         <code>false</code>, otherwise.
     */
    public boolean isFetchAutoTuning()
    {
        return getBooleanProperty(PROP_FETCH_AUTO_TUNING, DEFAULT_FETCH_AUTO_TUNING);
    }

    /**
     * Property 에 fetch auto-tuning 여부를 설정한다.
     */
    public void setFetchAutoTuning(boolean aIsFetchAutoTuning)
    {
        setProperty(PROP_FETCH_AUTO_TUNING, aIsFetchAutoTuning);
    }

    /**
     * Property 로 설정된 auto-tuning 시작시 초기 prefetch rows 를 얻는다.
     */
    public int getFetchAutoTuningInit()
    {
        return getIntProperty(PROP_FETCH_AUTO_TUNING_INIT, DEFAULT_FETCH_AUTO_TUNING_INIT);
    }

    /**
     * Property 로 설정된 auto-tuning 동작시 최소 제한 크기의 prefetch rows 를 얻는다.
     */
    public int getFetchAutoTuningMin()
    {
        return getIntProperty(PROP_FETCH_AUTO_TUNING_MIN, DEFAULT_FETCH_AUTO_TUNING_MIN);
    }

    /**
     * Property 로 설정된 auto-tuning 동작시 최대 제한 크기의 prefetch rows 를 얻는다.
     */
    public int getFetchAutoTuningMax()
    {
        return getIntProperty(PROP_FETCH_AUTO_TUNING_MAX, DEFAULT_FETCH_AUTO_TUNING_MAX);
    }

    /**
     * Property 로 설정한 socket receive buffer 크기의 CM block 비율을 얻는다.
     */
    public int getSockRcvBufBlockRatio()
    {
        return getIntProperty(PROP_SOCK_RCVBUF_BLOCK_RATIO, DEFAULT_SOCK_RCVBUF_BLOCK_RATIO);
    }

    /**
     * Property 에 socket receive buffer 크기의 CM block 비율을 설정한다.
     */
    public void setSockRcvBufBlockRatio(int aSockRcvBufBlockRatio)
    {
        setProperty(PROP_SOCK_RCVBUF_BLOCK_RATIO, aSockRcvBufBlockRatio);
    }

    public void sslEnabledToConnType()
    {
        if (getBooleanProperty(PROP_SSL_ENABLE, DEFAULT_SSL_ENABLE))
        {
            setConnType(CmConnType.SSL.toString());
        }
    }

    public String getConnType()
    {
        return getProperty(PROP_CONNTYPE, DEFAULT_CONNTYPE);
    }

    public String getShardConnType()
    {
        return getProperty(PROP_SHARD_CONNTYPE, DEFAULT_CONNTYPE);
    }

    public boolean isShardLazyConnectEnabled()
    {
        return getBooleanProperty(PROP_SHARD_LAZYCONNECT, true);
    }

    public boolean isReshardEnabled()
    {
        return getBooleanProperty(PROP_RESHARD_ENABLE, false);
    }

    public void setConnType(String aConnType)
    {
        setProperty(PROP_CONNTYPE, aConnType);
    }

    public int getIBLatency()
    {
        return getIntProperty(PROP_IB_LATENCY, DEFAULT_IB_LATENCY);
    }

    public int getIBConChkSpin()
    {
        return getIntProperty(PROP_IB_CONCHKSPIN, DEFAULT_IB_CONCHKSPIN);
    }

    private static final String  PROP_PAIR_PATTERN_STR = "([a-zA-Z_][\\w]*\\s*)=(\\s*(?:\\([^\\)]*\\)|[^,\\{\\}\\(\\)]*))";
    private static final Pattern PROP_PAIR_PATTERN     = Pattern.compile(PROP_PAIR_PATTERN_STR);
    private static final Pattern PROP_STR_PATTERN      = Pattern.compile("^\\s*\\{\\s*" + PROP_PAIR_PATTERN_STR + "(?:\\s*,\\s*" + PROP_PAIR_PATTERN_STR + ")*\\s*\\}\\s*$");

    /**
     * 문자열을 객체로 변환한다.
     * 
     * @param aPropStr {@link #toString()}와 같은 "{key1=val1, key2=val2, ...}" 형태의 문자열.
     * @return 문자열로부터 생성한 AltibaseProperties 객체
     */
    public static AltibaseProperties valueOf(String aPropStr)
    {
        Matcher sMatcher = PROP_STR_PATTERN.matcher(aPropStr);
        if (!sMatcher.matches())
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "Format for properties",
                                                "{key1=val1, key2=val2, ...}",
                                                aPropStr);
        }

        AltibaseProperties sProp = new AltibaseProperties();
        Matcher m = PROP_PAIR_PATTERN.matcher(aPropStr);
        while (m.find())
        {
            sProp.setProperty(m.group(1), m.group(2));
        }
        return sProp;
    }

    /* PROPERTY DEFAULT VALUES */
    private static final DriverPropertyInfo[] DEFAULT_PROPERTY_INFO = {
        new AltibaseDriverPropertyInfo( PROP_SERVER                     , DEFAULT_SERVER                , null                              , true  , null ),
        new AltibaseDriverPropertyInfo( PROP_SOCK_BIND_ADDR             , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_PORT                       , DEFAULT_PORT                  , null                              , true  , null ),
        new AltibaseDriverPropertyInfo( PROP_DBNAME                     , DEFAULT_DBNAME                , null                              , true  , null ),
        new AltibaseDriverPropertyInfo( PROP_USER                       , null                          , null                              , true  , null ),
        new AltibaseDriverPropertyInfo( PROP_PASSWORD                   , null                          , null                              , true  , null ),
        new AltibaseDriverPropertyInfo( PROP_CONNTYPE                   , DEFAULT_CONNTYPE              , new String[] {"tcp", "1",
                                                                                                                        "ssl", "6",
                                                                                                                        "ib", "8" }         , false , null ),
        new AltibaseDriverPropertyInfo( PROP_IB_LATENCY                 , DEFAULT_IB_LATENCY            , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_IB_CONCHKSPIN              , DEFAULT_IB_CONCHKSPIN         , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_CONNECT_MODE               , null                          , new String[] {"normal", "sysdba"} , false , null ),
        new AltibaseDriverPropertyInfo( PROP_DESCRIPTION                , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_DATASOURCE_NAME            , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_LOGIN_TIMEOUT              , DEFAULT_LOGIN_TIMEOUT         , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_RESPONSE_TIMEOUT           , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_QUERY_TIMEOUT              , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_IDLE_TIMEOUT               , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FETCH_TIMEOUT              , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_UTRANS_TIMEOUT             , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_DDL_TIMEOUT                , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_AUTO_COMMIT                , DEFAULT_AUTO_COMMIT                                               , false , null ),
        new AltibaseDriverPropertyInfo( PROP_DATE_FORMAT                , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_NCHAR_LITERAL_REPLACE      , DEFAULT_NCHAR_LITERAL_REPLACE                                     , false , null ),
        new AltibaseDriverPropertyInfo( PROP_MAX_STATEMENTS_PER_SESSION , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_APP_INFO                   , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_TXI_LEVEL                  , null                          , new String[] {"2", "4", "8"}      , false , null ),
        new AltibaseDriverPropertyInfo( PROP_ALT_SERVERS                , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_LOAD_BALANCE               , DEFAULT_LOAD_BALANCE                                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_CTF_RETRY_COUNT            , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_CTF_RETRY_DELAY            , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FAILOVER_USE_STF           , DEFAULT_FAILOVER_USE_STF                                          , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FAILOVER_SOURCE            , null                          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_PREFER_IPV6                , DEFAULT_PREFER_IPV6                                               , false , null ),
        new AltibaseDriverPropertyInfo( PROP_TIME_ZONE                  , DEFAULT_TIME_ZONE             , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FETCH_ENOUGH               , DEFAULT_FETCH_ENOUGH          , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FETCH_ASYNC                , DEFAULT_FETCH_ASYNC           , new String[] { PROP_VALUE_FETCH_ASYNC_OFF,
                                                                                                                         PROP_VALUE_FETCH_ASYNC_PREFERRED } , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FETCH_AUTO_TUNING          , DEFAULT_FETCH_AUTO_TUNING                                         , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FETCH_AUTO_TUNING_INIT     , DEFAULT_FETCH_AUTO_TUNING_INIT, null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FETCH_AUTO_TUNING_MIN      , DEFAULT_FETCH_AUTO_TUNING_MIN , null                              , false , null ),
        new AltibaseDriverPropertyInfo( PROP_FETCH_AUTO_TUNING_MAX      , DEFAULT_FETCH_AUTO_TUNING_MAX , null                              , false , null ),
    };

    public static DriverPropertyInfo[] toPropertyInfo(Properties aProp)
    {
        DriverPropertyInfo[] sPropInfo = (DriverPropertyInfo[])DEFAULT_PROPERTY_INFO.clone();
        for (int i = 0; i < sPropInfo.length; i++)
        {
            String sValue = aProp.getProperty(sPropInfo[i].name);
            if (sValue != null)
            {
                sPropInfo[i].value = sValue;
            }
        }
        return sPropInfo;
    }

    public DriverPropertyInfo[] toPropertyInfo()
    {
        return toPropertyInfo(this);
    }

    // #endregion
}
