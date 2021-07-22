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

import java.sql.*;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.charset.Charset;
import java.util.*;
import java.util.concurrent.Executor;
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.cm.*;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.ex.ShardError;
import Altibase.jdbc.driver.ex.ShardFailoverIsNotAvailableException;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection;
import Altibase.jdbc.driver.sharding.core.AltibaseShardingFailover;
import Altibase.jdbc.driver.sharding.core.ShardConnType;
import Altibase.jdbc.driver.sharding.core.GlobalTransactionLevel;
import Altibase.jdbc.driver.sharding.core.DistTxInfo;
import Altibase.jdbc.driver.sharding.util.DistTxInfoForVerify;
import Altibase.jdbc.driver.util.*;

import static Altibase.jdbc.driver.AutoCommitMode.*;
import static Altibase.jdbc.driver.util.AltibaseProperties.PROP_CODE_GLOBAL_TRANSACTION_LEVEL;
import static Altibase.jdbc.driver.util.AltibaseProperties.PROP_CODE_SHARD_STATEMENT_RETRY;

public final class AltibaseConnection extends AbstractConnection
{
    public static final byte              EXPLAIN_PLAN_OFF                          = 0;
    public static final byte              EXPLAIN_PLAN_ON                           = 1;
    public static final byte              EXPLAIN_PLAN_ONLY                         = 2;

    private static final String           PROP_VALUE_PRIVILEGE_SYSDBA               = "sysdba";
    private static final String           PROP_VALUE_PRIVILEGE_NORMAL               = "normal";
    // BUG-48892 jdbc 4.2 spec 부터는 client_type으로 NEW_JDBC42를 보낸다.
    private static final String           PROP_VALUE_CLIENT_TYPE                    = "NEW_JDBC42";
    private static final String           PROP_VALUE_NLS                            = "UTF16";
    private static final int              PROP_VALUE_HEADER_DISPLAY_MODE            = 1;

    private static final String           INTERNAL_SQL_ALTER_SESSION_SET_TXI_LEVEL  = "ALTER SESSION SET TRANSACTION ISOLATION LEVEL ";     /* BUG-39817 */

    private static final String           TX_LEVEL_SERIALIZABLE                     = "serializable";
    private static final String           TX_LEVEL_READ_COMMITTED                   = "read committed";
    private static final String           TX_LEVEL_REPEATABLE_READ                  = "repeatable read";

    /* BUG-39817 */
    private static final int              PROP_VALUE_SERVER_ISOLATION_LEVEL_UNKNOWN = -1;
    private static final int              PROP_VALUE_SERVER_READ_COMMITTED          = 0;
    private static final int              PROP_VALUE_SERVER_REPEATABLE_READ         = 1;
    private static final int              PROP_VALUE_SERVER_SERIALIZABLE            = 2;

    private static final int              STMT_CID_SEQ_BIT                          = 16;
    private static final int              STMT_CID_SEQ_MAX                          = (1 << STMT_CID_SEQ_BIT);
    private static final int              STMT_CID_SEQ_MASK                         = (STMT_CID_SEQ_MAX - 1);

    private static final Map              EMPTY_TYPEMAP                             = Collections.unmodifiableMap(new java.util.HashMap(0));

    // PROJ-2707 jdbc 4.x spec에 따른 권한 설정
    private static final SQLPermission    SQL_PERMISSION_NETWORK_TIMEOUT            = new SQLPermission("setNetworkTimeout");
    private static final SQLPermission    ABORT_PERM                                = new SQLPermission("abort");

    public  static final String           PROP_APPLICATION_NAME                     = "ApplicationName";

    private CmChannel                     mChannel;
    private CmProtocolContextConnect      mContext;
    private SQLWarning                    mWarning;
    private LinkedList                    mStatementList                            = new LinkedList();
    private AutoCommitMode                mAutoCommit                               = SERVER_SIDE_AUTOCOMMIT_ON;
    private int                           mDefaultResultSetType                     = ResultSet.TYPE_FORWARD_ONLY;
    private int                           mDefaultResultSetConcurrency              = ResultSet.CONCUR_READ_ONLY;
    private int                           mDefaultResultSetHoldability              = ResultSet.CLOSE_CURSORS_AT_COMMIT;
    private AltibaseProperties            mProp;
    private int                           mTxILevel                                 = TRANSACTION_NONE;  /* BUG-39817 */
    private AltibaseStatement             mInternalStatement;
    private int                           mCurrentCIDSeq                            = 0;
    private Object                        mCurrentCIDSeqLock                        = new Object();
    private BitSet                        mUsedCIDSet                               = new BitSet(STMT_CID_SEQ_MAX);
    private Properties                    mClientInfo                               = new CaseInsensitiveProperties();
    private boolean                       mNliteralReplace;
    private AltibaseDataSource            mDataSource;
    private AltibaseDatabaseMetaData      mMetaData;
    private AltibaseFailoverContext       mFailoverContext;
    private byte                          mExplainPlanMode;
    private String                        mSessionTimeZone;
    private String                        mDbTimeZone;
    private String                        mDBPkgVerStr;
    // PROJ-2583
    private transient Logger              mLogger;

    // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
    private AltibaseStatement             mAsyncPrefetchStatement;

    // PROJ-2690 shard jdbc
    private AltibaseShardingConnection    mMetaConnection;
    private ShardConnType                 mShardConnType;

    private String                        mServer;
    private int                           mPort;
    private boolean                       mAllowLobNullSelect;      // BUG-47639 lob column이 null일때 Lob객체가 리턴될 수 있는지 여부
    private boolean                       mReUseResultSet;          // BUG-48380 같은 PreparedStatement에서 executeQuery를 했을 때 ResultSet을 재사용할지 여부

    // PROJ-2733 : To Verify DistTxInfo
    public static final boolean           SHARD_JDBC_DISTTXINFO_VERIFY              = AltibaseEnvironmentVariables.getShardJdbcDisttxinfoVerify();
    private DistTxInfoForVerify           mDistTxInfoForVerify;

    // BUG-48892 DatabaseMetaData.getProcedures()에서 function이 리턴될 수 있는지 여부
    private boolean                       mGetProceduresReturnFunctions;

    static
    {
        // BUG-46325 LobObjectFactory의 초기화를 AltibaseDriver대신 AltibaseConnection에서 수행한다.
        LobObjectFactoryImpl.registerLobFactory();
    }

    public AltibaseConnection(Properties aProp, AltibaseDataSource aDataSource,
                              AltibaseShardingConnection aMetaConn) throws SQLException
    {
        createConnection(aProp, aDataSource, aMetaConn);
    }

    AltibaseConnection(Properties aProp, AltibaseDataSource aDataSource) throws SQLException
    {
        createConnection(aProp, aDataSource, null);
    }

    private void createConnection(Properties aProp, AltibaseDataSource aDataSource,
                                  AltibaseShardingConnection aMetaConn) throws SQLException
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
        }
        mShardConnType = ShardConnType.NONE;
        mDataSource = aDataSource;
        mChannel = new CmChannel();
        mContext = new CmProtocolContextConnect(mChannel);
        mMetaConnection = aMetaConn;

        // PROJ-2733 DistTxInfo : for natc
        if (SHARD_JDBC_DISTTXINFO_VERIFY)
        {    
            mDistTxInfoForVerify = new DistTxInfoForVerify();
        }    

        // BUG-46790 aMetaConn객체가 있는 경우에는 shard connection 이므로 shard connection type을 설정해 준다.
        if (aMetaConn != null)
        {
            mShardConnType = (aDataSource == null) ? ShardConnType.META_CONNECTION :
                             ShardConnType.NODE_CONNECTION;
        }
        loadProperties(aProp);

        if (mFailoverContext != null && mProp.useLoadBalance())
        {
            AltibaseFailoverServerInfo sServerInfo = mFailoverContext.getFailoverServerList().getRandom();
            mServer = sServerInfo.getServer();
            mPort   = sServerInfo.getPort();
        }
        else
        {
            mServer = mProp.getServer();
            mPort   = mProp.getPort();
        }

        try
        {
            mChannel.open(mServer, mProp.getSockBindAddr(), mPort, mProp.getLoginTimeout());

            handshake();

            // BUG-46790 meta connection이나 node connection일 때는 shard handshake를 수행한다.
            if (aMetaConn != null)
            {
                shardHandshake();
                if (aDataSource == null) // BUG-46790 meta connection 인 경우에는 channel객체를 주입한다.
                {
                    aMetaConn.setChannel(mChannel);
                }
            }

            connect(mProp);

            // BUG-47492 db login 이후 response_timeout을 설정한다.
            mChannel.setResponseTimeout(mProp.getResponseTimeout());
        }
        catch (SQLException aEx)
        {
            mChannel.quietClose();
            checkShardFailoverIsNotAvailable(aEx);
            mWarning = AltibaseFailover.tryCTF(mFailoverContext, mWarning, aEx);
        }
        mIsClosed = false;

        mInternalStatement = AltibaseStatement.createInternalStatement(this);
        mStatementList.add(mInternalStatement);
    }

    /**
     * node connection이고 failover가 셋팅되어 있지 않은 경우 통신에러일때 FailoverIsNotAvailableException
     * 예외를 올린다.
     * @param aException SQLException
     * @throws ShardFailoverIsNotAvailableException Shard Failover Is Not Available Exception
     */
    private void checkShardFailoverIsNotAvailable(SQLException aException) throws ShardFailoverIsNotAvailableException
    {
        if (getNodeName() != null && ((mFailoverContext == null) ||
                                      (mFailoverContext.getFailoverServerList().size() <= 0)) &&
            AltibaseFailover.isNeedToFailover(aException))
        {
            CmOperation.throwShardFailoverIsNotAvailableException(getNodeName(), getServer(), getPort());
        }
    }

    public String getNodeName()
    {
        return mProp.getProperty(AltibaseProperties.PROP_SHARD_NODE_NAME);
    }

    /**
     * 메타커넥션이나 노드커넥션인지 여부를 체크한다.
     * @return true 메타커넥션이나 노드커넥션 <br>
     *         false 일반 커넥션
     */
    public boolean isShardConnection()
    {
        return mShardConnType == ShardConnType.META_CONNECTION ||
               mShardConnType == ShardConnType.NODE_CONNECTION;
    }

    private void loadProperties(Properties aProp) throws SQLException
    {
        mProp = new AltibaseProperties(aProp);

        loadDataSourceProps();
        loadDefaultValues();

        // PROJ-2474, PROJ-2681
        // Regression test 를 위해 ALTIBASE_CONNTYPE_FORCE_FOR_TEST 환경변수가 설정되어 있다면,
        // 'conntype' 프로퍼티를 이 환경변수 값으로 조정한다.
        if (AltibaseEnvironmentVariables.isSet(AltibaseEnvironmentVariables.ENV_ALTIBASE_CONNTYPE_FORCE_FOR_TEST))
        {
            if (AltibaseEnvironmentVariables.getConnTypeForceForTest().equals("SSL"))
            {
                mProp.setConnType(CmConnType.SSL.toString());
                mProp.setProperty(SSLProperties.VERIFY_SERVER_CERTIFICATE, false);
                mProp.setPort(AltibaseEnvironmentVariables.getSslPort());
            }
            else if (AltibaseEnvironmentVariables.getConnTypeForceForTest().equals("IB"))
            {
                mProp.setConnType(CmConnType.IB.toString());

                if (AltibaseEnvironmentVariables.isSet(AltibaseEnvironmentVariables.ENV_ALTIBASE_HOST_FORCE_FOR_TEST))
                    mProp.setServer(AltibaseEnvironmentVariables.getHostForceForTest());
            }
        }
        
        // Determine whether connection type is IPv6 or IPv4
        // This determination is only used for physical connection
        if (mProp.isPreferIPv6())
        {
            mChannel.setPreferredIPv6();
        }

        // PROJ-2681
        mChannel.setConnType(mProp.getConnType());
        mChannel.setProps(mProp);

        // PROJ-2331
        if (mProp.isOnRedundantDataTransmission())
        {
            mChannel.setRemoveRedundantMode(true, mProp);
        }
        else
        {
            mChannel.setRemoveRedundantMode(false, mProp);
        }

        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
        try
        {
            int sSockRcvBufBlockRatio = mProp.getSockRcvBufBlockRatio();

            mChannel.setSockRcvBufBlockRatio(sSockRcvBufBlockRatio);
        }
        catch (IOException sCauseEx)
        {
            Error.throwCommunicationErrorException(sCauseEx);
        }

        initFailoverContext(mProp);
    }

    /**
     * 속성값이 셋팅되어 있지 않을때 default 속성값을 적용시킨다.
     */
    private void loadDefaultValues()
    {
        // PROJ-2681 프로퍼티 우선순위 적용 : 'ssl_enable=true' -> 'conntype=SSL'
        mProp.sslEnabledToConnType();

        if (!mProp.isSet(AltibaseProperties.PROP_SERVER))
        {
            mProp.setServer(AltibaseProperties.DEFAULT_SERVER);
        }
        if (!mProp.isSet(AltibaseProperties.PROP_PORT))
        {
            if (CmConnType.IB.toString().equalsIgnoreCase(mProp.getConnType()))
            {
                mProp.setPort(AltibaseEnvironmentVariables.getIBPort(AltibaseProperties.DEFAULT_IB_PORT));   
            }
            else if (CmConnType.SSL.toString().equalsIgnoreCase(mProp.getConnType()))
            {
                mProp.setPort(AltibaseEnvironmentVariables.getSslPort(AltibaseProperties.DEFAULT_SSL_PORT));   
            }
            else 
            {
                mProp.setPort(AltibaseEnvironmentVariables.getPort(AltibaseProperties.DEFAULT_PORT));
            }
        }
        if (!mProp.isSet(AltibaseProperties.PROP_DBNAME))
        {
            mProp.setDatabase(AltibaseProperties.DEFAULT_DBNAME);
        }
        if (!mProp.isSet(AltibaseProperties.PROP_NCHAR_LITERAL_REPLACE) &&
            AltibaseEnvironmentVariables.isSet(AltibaseEnvironmentVariables.ENV_ALTIBASE_NCHAR_LITERAL_REPLACE))
        {
            mProp.setNCharLiteralReplace(AltibaseEnvironmentVariables.useNCharLiteralReplace());
        }
        if (!mProp.isSet(AltibaseProperties.PROP_TIME_ZONE))
        {
            if (AltibaseEnvironmentVariables.getTimeZone() != null)
            {
                mProp.setTimeZone(AltibaseEnvironmentVariables.getTimeZone());
            }
        }
        if (!mProp.isSet(AltibaseProperties.PROP_LOGIN_TIMEOUT))
        {
            mProp.setLoginTimeout(DriverManager.getLoginTimeout());
        }
        if (!mProp.isSet(AltibaseProperties.PROP_DATE_FORMAT) &&
            AltibaseEnvironmentVariables.isSet(AltibaseEnvironmentVariables.ENV_ALTIBASE_DATE_FORMAT))
        {
            mProp.setDateFormat(AltibaseEnvironmentVariables.getDateFormat());
        }

        if(!mProp.isSet(AltibaseProperties.PROP_RESPONSE_TIMEOUT))
        {
            if(AltibaseEnvironmentVariables.isSet(AltibaseEnvironmentVariables.ENV_RESPONSE_TIMEOUT))
            {
                mProp.setResponseTimeout(AltibaseEnvironmentVariables.getResponseTimeout());
            }
        }

        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
        if (!mProp.isSet(AltibaseProperties.PROP_SOCK_RCVBUF_BLOCK_RATIO))
        {
            if (AltibaseEnvironmentVariables.isSet(AltibaseEnvironmentVariables.ENV_SOCK_RCVBUF_BLOCK_RATIO))
            {
                mProp.setSockRcvBufBlockRatio(AltibaseEnvironmentVariables.getSockRcvBufBlockRatio());
            }
        }

        if (!mProp.isSet(AltibaseProperties.PROP_FETCH_AUTO_TUNING))
        {
            if (mProp.getFetchAsync().equalsIgnoreCase(AltibaseProperties.PROP_VALUE_FETCH_ASYNC_PREFERRED))
            {
                String sOSName = System.getProperty("os.name");
                if ("Linux".equalsIgnoreCase(sOSName))
                {
                    mProp.setFetchAutoTuning(true);
                }
            }
        }
    }

    private void loadDataSourceProps() throws SQLException
    {
        if (mProp.isSet(AltibaseProperties.PROP_DATASOURCE_NAME))
        {
            AltibaseDataSource sDataSource = (AltibaseDataSource)AltibaseDataSource.getDataSource(mProp.getDataSource());
            if (sDataSource != null)
            {
                mProp.putAllExceptExist(sDataSource.properties());
                if (mDataSource == null)
                {
                    mDataSource = sDataSource;
                }
            }
            else
            {
                Error.throwSQLException(ErrorDef.NO_AVAILABLE_DATASOURCE);
            }
        }
    }

    private void initFailoverContext(AltibaseProperties aProp) throws SQLException
    {
        if (!aProp.isSet(AltibaseProperties.PROP_ALT_SERVERS))
        {
            return;
        }

        String sPropAltServers = aProp.getAlternateServer();
        AltibaseFailoverServerInfoList sFailoverServerList = AltibaseUrlParser.parseAlternateServers(sPropAltServers);
        /* BUG-43219 add dbname property to FailoverServerInfo */
        AltibaseFailoverServerInfo sPrimaryServerInfo = new AltibaseFailoverServerInfo(aProp.getServer(), aProp.getPort(), aProp.getDatabase());
        sFailoverServerList.add(0, sPrimaryServerInfo);

        mFailoverContext = new AltibaseFailoverContext(this, aProp, sFailoverServerList);
        mFailoverContext.setCurrentServer(sPrimaryServerInfo);
    }

    AltibaseConnection cloneConnection() throws SQLException
    {
        return new AltibaseConnection(mProp, mDataSource, mMetaConnection);
    }

    PrintWriter getLogWriter()
    {
        if (mDataSource == null)
        {
            return DriverManager.getLogWriter();
        }
        else
        {
            return mDataSource.getLogWriter();
        }        
    }

    /**
     * BUG-39149 handshake 프로토콜을 이용해 커넥션이 유효한지 체크한다.
     */
    public void ping() throws SQLException
    {
        throwErrorForClosed();
        handshake();
    }
    
    void handshake() throws SQLException
    {
        CmProtocol.handshake(mContext);
        if (mContext.getError() != null)
        {
            mWarning = Error.processServerError(mWarning, mContext.getError());
        }
    }

    void shardHandshake() throws SQLException
    {
        CmProtocol.shardHandshake(mContext);
        CmShardHandshakeResult sShardHandshakeResult = mContext.getShardHandshakeResult();
        if (sShardHandshakeResult.getError() != null)
        {
            mWarning = Error.processServerError(mWarning, sShardHandshakeResult.getError());
        }
    }

    void connect(AltibaseProperties aProp) throws SQLException
    {
        String sDBName = aProp.getDatabase();
        String sUser = aProp.getUser();
        String sPassword = aProp.getPassword();
        short sModeInt = CmOperation.CONNECT_MODE_NORMAL;
        
        checkDBNameAndUserInfo(sDBName, sUser, sPassword);

        if (aProp.isSet(AltibaseProperties.PROP_CONNECT_MODE))
        {
            String sMode = aProp.getPrivilege();
            if (PROP_VALUE_PRIVILEGE_SYSDBA.equalsIgnoreCase(sMode))
            {
                sModeInt = CmOperation.CONNECT_MODE_SYSDBA;
            }
            else if (!PROP_VALUE_PRIVILEGE_NORMAL.equalsIgnoreCase(sMode))
            {
                mWarning = Error.createWarning(mWarning, ErrorDef.INVALID_PROPERTY_VALUE, "normal|sysdba", sMode);
            }
        }

        mContext.addProperty(AltibaseProperties.PROP_CODE_CLIENT_PACKAGE_VERSION, AltibaseVersion.ALTIBASE_VERSION_STRING);
        mContext.addProperty(AltibaseProperties.PROP_CODE_CLIENT_PROTOCOL_VERSION, AltibaseVersion.CM_PROTOCOL_VERSION);
        mContext.addProperty(AltibaseProperties.PROP_CODE_CLIENT_PID, (long)Thread.currentThread().hashCode());
        mContext.addProperty(AltibaseProperties.PROP_CODE_CLIENT_TYPE, PROP_VALUE_CLIENT_TYPE);
        
        // BUG-46790 shard 관련된 속성값을 먼저 버퍼에 write 한다.
        if (mMetaConnection != null || mProp.isSet(AltibaseProperties.PROP_SHARD_NODE_NAME))
        {
            setShardProperties(aProp);
        }

        mContext.addProperty(AltibaseProperties.PROP_CODE_NLS, PROP_VALUE_NLS);
        mContext.addProperty(AltibaseProperties.PROP_CODE_HEADER_DISPLAY_MODE, PROP_VALUE_HEADER_DISPLAY_MODE); // 없앨 수도 있음. BUG-33625

        /* BUG-46019 JDBC는 Connection 전에 MESSAGE_CALLBACK을 등록할 수 없다. */
        mContext.addProperty(AltibaseProperties.PROP_CODE_MESSAGE_CALLBACK, false);

        // 부가적인 프로퍼티 세팅
        setOptionalProperties(aProp);

        // PROJ-2331
        if (mProp.isOnRedundantDataTransmission())
        {
            mChannel.setRemoveRedundantMode(true, mProp);
        }
        else
        {
            mChannel.setRemoveRedundantMode(false, mProp);
        }
        
        //  PROJ-2583 log session property info
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED) 
        {
            mLogger.log(Level.INFO, "Properties Infos : {0} ", mContext);
        }
        CmProtocol.connect(mContext, sDBName, sUser, sPassword, sModeInt, mShardConnType);
        
        if (mContext.getError() != null)
        {
            setOffUnsupportedProperty(mContext.getError());
            
            mWarning = Error.processServerError(mWarning, mContext.getError());
        }
        
        Charset sCharset = CharsetUtils.getCharset(mContext.getCharsetName());
        Charset sNCharset = CharsetUtils.getCharset(mContext.getNCharsetName());
        // PROJ-2583 log characterset info
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger.log(Level.INFO, "Server Charset, NCharset Infos : {0}, {1} ", new Object[] {sCharset, sNCharset});
        }
        mChannel.setCharset(sCharset, sNCharset);
        mChannel.setLobCacheThreshold(aProp.getLobCacheThreshold());
        mExplainPlanMode = mContext.getExplainPlanMode();
        if (isValidExplainPlanMode(mExplainPlanMode) == false)
        {
            mWarning = Error.createWarning(mWarning, ErrorDef.INVALID_PROPERTY_VALUE,
                                           "EXPLAIN_PLAN_OFF | EXPLAIN_PLAN_ON | EXPLAIN_PLAN_ONLY",
                                           String.valueOf(mExplainPlanMode));
        }

        /* BUG-39817 */
        if (mContext.isSetPropertyResult(AltibaseProperties.PROP_CODE_ISOLATION_LEVEL))
        {
            int sServerIsolationLevel = mContext.getIsolationLevel();
            mTxILevel = getPropIsolationLevel(sServerIsolationLevel);
            mProp.setIsolationLevel(sServerIsolationLevel);
        }
        if (mContext.isSetPropertyResult(AltibaseProperties.PROP_CODE_GLOBAL_TRANSACTION_LEVEL))
        {
            GlobalTransactionLevel sGlobalTransactionLevel = mContext.getGlobalTransactionLevel();
            // shardjdbc가 아닐 경우 mMetaConnection이 null일 수 있다.
            if (mMetaConnection != null)
            {
                mMetaConnection.setGlobalTransactionLevel(sGlobalTransactionLevel);
            }
            // BUGBUG : failover 등 reconnect가 필요할 때... 서버에서 받아온 프로퍼티 정보는 reconnect 시 다시 받아오기?
            //          아니면 나의 마지막 정보를 저장했다가 connect 시 사용? 그렇다면 어디다 저장할 것인가??
            mProp.setProperty(AltibaseProperties.PROP_GLOBAL_TRANSACTION_LEVEL, sGlobalTransactionLevel.getValue());
        }
        if (mContext.isSetPropertyResult(AltibaseProperties.PROP_CODE_SHARD_STATEMENT_RETRY))
        {
            short sShardStatementRetry = mContext.getShardStatementRetry();
            if (mMetaConnection != null)
            {
                mMetaConnection.setShardStatementRetry(sShardStatementRetry);
            }
            mProp.setProperty(AltibaseProperties.PROP_SHARD_STATEMENT_RETRY, sShardStatementRetry);
        }
        if (mContext.isSetPropertyResult(AltibaseProperties.PROP_CODE_INDOUBT_FETCH_TIMEOUT))
        {
            int sIndoubtFetchTimeout = mContext.getIndoubtFetchTimeout();
            if (mMetaConnection != null)
            {
                mMetaConnection.setIndoubtFetchTimeout(sIndoubtFetchTimeout);
            }
            mProp.setProperty(AltibaseProperties.PROP_INDOUBT_FETCH_TIMEOUT, sIndoubtFetchTimeout);
        }
        if (mContext.isSetPropertyResult(AltibaseProperties.PROP_CODE_INDOUBT_FETCH_METHOD))
        {
            short sIndoubtFetchMethod = mContext.getIndoubtFetchMethod();
            if (mMetaConnection != null)
            {
                mMetaConnection.setIndoubtFetchMethod(sIndoubtFetchMethod);
            }
            mProp.setProperty(AltibaseProperties.PROP_INDOUBT_FETCH_METHOD, sIndoubtFetchMethod);
        }
    }

    /* BUG-41908 Add processing the error 'mmERR_IGNORE_UNSUPPORTED_PROPERTY' in JDBC 
     * 특정 property에 대해서 서버에서 지원하지 않는다는 응답을 보냈을 경우 JDBC에서도 해당 property을 off 한다. 
     * 추후에 client와 server와 지원하는 property가 맞지 않을경우 정확히는 client만 지원하는 property가 생길경우 아래와 같이 처리한다. */
    private void setOffUnsupportedProperty(CmErrorResult aCmErrorResult) throws SQLException
    {
        CmErrorResult sCmErrorResult = aCmErrorResult;
        while (sCmErrorResult != null)
        {
            if( Error.toClientSideErrorCode(sCmErrorResult.getErrorCode()) == ErrorDef.UNSUPPORTED_PROPERTY)
            {
                switch (sCmErrorResult.getErrorIndex())
                {
                    case AltibaseProperties.PROP_CODE_REMOVE_REDUNDANT_TRANSMISSION:
                        mProp.setProperty(AltibaseProperties.PROP_REMOVE_REDUNDANT_TRANSMISSION, 0);
                        break;
                    case AltibaseProperties.PROP_CODE_LOB_CACHE_THRESHOLD:
                        mProp.setProperty(AltibaseProperties.PROP_LOB_CACHE_THRESHOLD, 0);
                        break;
                    case AltibaseProperties.PROP_CODE_MESSAGE_CALLBACK:  /* BUG-46019 */
                        /* MessageCallback은 이미 설정되어 있다. */
                        break;
                    default:
                        /* 반드시 필요한 property일 경우 접속을 해제한다. */
                        Error.throwSQLException(ErrorDef.NOT_SUPPORTED_MANDATORY_PROPERTY);
                        break;
                }
            }
            sCmErrorResult = sCmErrorResult.getNextError();
        }
    }
    
    private void setOptionalProperties(AltibaseProperties aProp) throws SQLException
    {
        String sValue;

        // shardjdbc가 아니어도 전송한다.
        if (aProp.isSet(AltibaseProperties.PROP_GLOBAL_TRANSACTION_LEVEL))
        {
            GlobalTransactionLevel sGlobalTransactionLevel = aProp.getGlobalTransactionLevel();
            mContext.addProperty( PROP_CODE_GLOBAL_TRANSACTION_LEVEL,
                                  sGlobalTransactionLevel.getValue() );
            if (mMetaConnection != null)
            {
                mMetaConnection.setGlobalTransactionLevel(sGlobalTransactionLevel);
            }
        }

        mAutoCommit = (aProp.isAutoCommit()) ? SERVER_SIDE_AUTOCOMMIT_ON : SERVER_SIDE_AUTOCOMMIT_OFF;
        if (aProp.isClientSideAutoCommit())
        {
            if (aProp.isAutoCommit())
            {
                mAutoCommit = CLIENT_SIDE_AUTOCOMMIT_ON;
            }
            mDefaultResultSetHoldability = ResultSet.HOLD_CURSORS_OVER_COMMIT; // PROJ-2190 client side autocommit이 활성화 되어 있을때는 HOLD_CURSOR로 바꿔준다.
        }

        mContext.addProperty(AltibaseProperties.PROP_CODE_AUTOCOMMIT, isServerSideAutoCommit() ? true : false);

        sValue = aProp.getDateFormat();
        if (sValue != null)
        {
            mContext.addProperty(AltibaseProperties.PROP_CODE_DATE_FORMAT, sValue.toUpperCase());
        }

        mNliteralReplace = aProp.useNCharLiteralReplace();
        if (mNliteralReplace == true)
        {
            mContext.addProperty(AltibaseProperties.PROP_CODE_NLS_NCHAR_LITERAL_REPLACE, 1);
        }
        else
        {
            // 디폴트가 false이므로 서버에 굳이 알릴 필요없다.
        }

        setOptionalIntProperty(AltibaseProperties.PROP_CODE_MAX_STATEMENTS_PER_SESSION,
                               aProp.getProperty(AltibaseProperties.PROP_MAX_STATEMENTS_PER_SESSION));

        sValue = aProp.getTimeZone();
        if (sValue != null)
        {
            if (sValue.equalsIgnoreCase(AltibaseProperties.LOCAL_TIME_ZONE))
            {
                sValue = Calendar.getInstance().getTimeZone().getID();
            }
            mContext.addProperty(AltibaseProperties.PROP_CODE_TIME_ZONE, sValue);
            if (!sValue.equalsIgnoreCase(AltibaseProperties.DB_TIME_ZONE))
            {
                mSessionTimeZone = sValue;
            }
        }

        sValue = aProp.getAppInfo();
        if (sValue != null)
        {
            mContext.addProperty(AltibaseProperties.PROP_CODE_APP_INFO, sValue);
            // PROJ-2707 app_info jdbc 속성이 있을 경우 clientinfo 해쉬맵에도 셋팅해 준다.
            mClientInfo.put(PROP_APPLICATION_NAME, sValue);
        }
        else
        {
            mClientInfo.put(PROP_APPLICATION_NAME, "");
        }

        /* BUG-31390 v$session에 정보 출력 */
        sValue = aProp.getFailoverSource();
        if (sValue != null)
        {
            mContext.addProperty(AltibaseProperties.PROP_CODE_FAILOVER_SOURCE, sValue);
        }

        // BUG-47639 connect 시점에 lob_null_select jdbc 속성을 체크한다.(기본값은 false)
        mAllowLobNullSelect = aProp.getBooleanProperty(AltibaseProperties.PROP_LOB_NULL_SELECT, false);

        // BUG-48380 ResultSet 재사용 여부를 reuse_resultset jdbc 속성 으로 부터 가져온다.
        mReUseResultSet = aProp.getBooleanProperty(AltibaseProperties.PROP_REUSE_RESULTSET, true);

        setOptionalIntProperty(AltibaseProperties.PROP_CODE_IDLE_TIMEOUT,
                               aProp.getProperty(AltibaseProperties.PROP_IDLE_TIMEOUT));
        setOptionalIntProperty(AltibaseProperties.PROP_CODE_QUERY_TIMEOUT,
                               aProp.getProperty(AltibaseProperties.PROP_QUERY_TIMEOUT));
        setOptionalIntProperty(AltibaseProperties.PROP_CODE_FETCH_TIMEOUT,
                               aProp.getProperty(AltibaseProperties.PROP_FETCH_TIMEOUT));
        setOptionalIntProperty(AltibaseProperties.PROP_CODE_UTRANS_TIMEOUT,
                               aProp.getProperty(AltibaseProperties.PROP_UTRANS_TIMEOUT));
        setOptionalIntProperty(AltibaseProperties.PROP_CODE_DDL_TIMEOUT,
                               aProp.getProperty(AltibaseProperties.PROP_DDL_TIMEOUT));
        setOptionalIntProperty(AltibaseProperties.PROP_CODE_LOB_CACHE_THRESHOLD,
                               aProp.getProperty(AltibaseProperties.PROP_LOB_CACHE_THRESHOLD));
        setOptionalIntProperty(AltibaseProperties.PROP_CODE_REMOVE_REDUNDANT_TRANSMISSION,
                               aProp.getProperty(AltibaseProperties.PROP_REMOVE_REDUNDANT_TRANSMISSION));
        /* BUG-39817 */
        setOptionalIsolationLevelProperty(AltibaseProperties.PROP_CODE_ISOLATION_LEVEL,
                                          aProp.getProperty(AltibaseProperties.PROP_TXI_LEVEL));

        mGetProceduresReturnFunctions = aProp.getBooleanProperty(AltibaseProperties.PROP_GETPROCEDURES_RETURN_FUNCTIONS, true);

        // PROJ-2733
        if (aProp.isSet(AltibaseProperties.PROP_SHARD_STATEMENT_RETRY))
        {
            mContext.addProperty( AltibaseProperties.PROP_CODE_SHARD_STATEMENT_RETRY, (byte)aProp.getShardStatementRetry());
            if (mMetaConnection != null)
            {
                mMetaConnection.setShardStatementRetry(aProp.getShardStatementRetry());
            }
        }
        if (aProp.isSet(AltibaseProperties.PROP_INDOUBT_FETCH_TIMEOUT))
        {
            mContext.addProperty( AltibaseProperties.PROP_CODE_INDOUBT_FETCH_TIMEOUT, aProp.getIndoubtFetchTimeout());
            if (mMetaConnection != null)
            {
                mMetaConnection.setIndoubtFetchTimeout(aProp.getIndoubtFetchTimeout());
            }
        }
        if (aProp.isSet(AltibaseProperties.PROP_INDOUBT_FETCH_METHOD))
        {
            mContext.addProperty( AltibaseProperties.PROP_CODE_INDOUBT_FETCH_METHOD, (byte)aProp.getIndoubtFetchMethod());
            if (mMetaConnection != null)
            {
                mMetaConnection.setIndoubtFetchMethod(aProp.getIndoubtFetchMethod());
            }
        }
    }

    /**
     * 샤딩과 관련된 속성을 보낸다.
     * @param aProp AltibaseProperties 객체
     * @throws SQLException 속성을 추가하는 도중 예외가 발생한 경우
     */
    private void setShardProperties(AltibaseProperties aProp) throws SQLException
    {
        /* Shard session 구분을 위한 중요한 Property는 먼저 전송하자.
         * ( ULN_PROPERTY_SHARD_NODE_NAME,
         *   ULN_PROPERTY_SHARD_SESSION_TYPE,
         *   ULN_PROPERTY_SHARD_CLIENT,
         *   ULN_PROPERTY_SHARD_PIN,
         *   ULN_PROPERTY_SHARD_META_NUMBER )
         * 위 순서는 변경에 주의 해야 한다.
         * ex) ULN_PROPERTY_SHARD_META_NUMBER 를 수신하면
         *     ULN_PROPERTY_SHARD_SESSION_TYPE 값을 이용해 분기한다.
         */
        
        String sNodeName = aProp.getShardNodeName();
        if (!StringUtils.isEmpty(sNodeName))
        {
            mContext.addProperty(AltibaseProperties.PROP_CODE_SHARD_NODE_NAME, sNodeName.toUpperCase());
        }
        
        // BUG-47324
        mContext.addProperty(AltibaseProperties.PROP_CODE_SHARD_SESSION_TYPE, aProp.getShardSessionType());
        mContext.addProperty(AltibaseProperties.PROP_CODE_SHARD_CLIENT, aProp.getShardClient());

        long sShardPin = aProp.getShardPin();
        if (sShardPin > 0)
        {
            mContext.addProperty(AltibaseProperties.PROP_CODE_SHARD_PIN, sShardPin);
        }
        long sShardMetaNumber = aProp.getLongProperty(AltibaseProperties.PROP_SHARD_META_NUMBER);
        if (sShardMetaNumber != 0)
        {
            mContext.addProperty(AltibaseProperties.PROP_CODE_SHARD_META_NUMBER, sShardMetaNumber);
        }
    }

    private boolean isServerSideAutoCommit()
    {
        return mAutoCommit == SERVER_SIDE_AUTOCOMMIT_ON;
    }

    private void setOptionalIntProperty(byte aPropCode, String aPropValue) throws SQLException
    {
        if (aPropValue == null)
        {
            return;
        }

        try
        {
            int sIntValue = Integer.parseInt(aPropValue);
            if (sIntValue >= 0)
            {
                mContext.addProperty(aPropCode, sIntValue);
            }
            else
            {
                mWarning = Error.createWarning(mWarning, ErrorDef.INVALID_PROPERTY_VALUE, ">=0", aPropValue);
            }
        }
        catch (NumberFormatException sEx)
        {
            mWarning = Error.createWarning(mWarning, ErrorDef.INVALID_PROPERTY_VALUE, "Number", aPropValue, sEx);
        }
    }

    /* BUG-39817 */
    private void setOptionalIsolationLevelProperty(byte aPropCode, String aPropIsolationLevelValue) throws SQLException
    {
        if (aPropIsolationLevelValue == null)
        {
            // will receive the isolation level from the server
            return;
        }

        try
        {
            int sPropIsolationLevel = Integer.parseInt(aPropIsolationLevelValue);
            if (sPropIsolationLevel == TRANSACTION_READ_UNCOMMITTED)
            {
                mWarning = Error.createWarning(mWarning,
                                               ErrorDef.OPTION_VALUE_CHANGED,
                                               "TRANSACTION_READ_UNCOMMITTED(1) changed to TRANSACTION_READ_COMMITTED(2)");
                sPropIsolationLevel = TRANSACTION_READ_COMMITTED;
            }

            int sServerIsolationLevel = getServerIsolationLevel(sPropIsolationLevel);
            if (sServerIsolationLevel != PROP_VALUE_SERVER_ISOLATION_LEVEL_UNKNOWN)
            {
                mTxILevel = getPropIsolationLevel(sServerIsolationLevel);

                mContext.addProperty(aPropCode, sServerIsolationLevel);
            }
            else
            {
                mWarning = Error.createWarning(mWarning,
                                               ErrorDef.INVALID_PROPERTY_VALUE,
                                               TRANSACTION_READ_COMMITTED + "|" + TRANSACTION_REPEATABLE_READ + "|" + TRANSACTION_SERIALIZABLE,
                                               aPropIsolationLevelValue);
            }
        }
        catch (NumberFormatException sEx)
        {
            mWarning = Error.createWarning(mWarning, ErrorDef.INVALID_PROPERTY_VALUE, "Number", aPropIsolationLevelValue, sEx);
        }
    }

    private static int getServerIsolationLevel(int aPropIsolationLevel)
    {
        int sServerIsolationLevel;
        switch (aPropIsolationLevel)
        {
            case TRANSACTION_READ_COMMITTED:
                sServerIsolationLevel = PROP_VALUE_SERVER_READ_COMMITTED;
                break;
            case TRANSACTION_REPEATABLE_READ:
                sServerIsolationLevel = PROP_VALUE_SERVER_REPEATABLE_READ;
                break;
            case TRANSACTION_SERIALIZABLE:
                sServerIsolationLevel = PROP_VALUE_SERVER_SERIALIZABLE;
                break;
            default:
                sServerIsolationLevel = PROP_VALUE_SERVER_ISOLATION_LEVEL_UNKNOWN;
                break;
        }
        return sServerIsolationLevel;
    }

    private static int getPropIsolationLevel(int aServerIsolationLevel)
    {
        int sPropIsolationLevel;
        switch (aServerIsolationLevel)
        {
            case PROP_VALUE_SERVER_READ_COMMITTED:
                sPropIsolationLevel = TRANSACTION_READ_COMMITTED;
                break;
            case PROP_VALUE_SERVER_REPEATABLE_READ: 
                sPropIsolationLevel = TRANSACTION_REPEATABLE_READ;
                break;
            case PROP_VALUE_SERVER_SERIALIZABLE:
                sPropIsolationLevel = TRANSACTION_SERIALIZABLE;
                break;
            default:
                sPropIsolationLevel = TRANSACTION_NONE;
                break;
        }
        return sPropIsolationLevel;
    }

    public String getURL()
    {
        return AltibaseConnection.getURL(mProp);
    }

    /**
     * DB 연결 정보가 담긴 Properties를 이용해 URL string을 얻는다.
     * 
     * @param aProps DB 연결 정보가 담긴 Properties
     * @return URL string
     */
    public static String getURL(Properties aProps)
    {
        StringBuffer sBuf = new StringBuffer(AltibaseUrlParser.URL_PREFIX);
        String sServerOrDSN = aProps.getProperty(AltibaseProperties.PROP_SERVER);
        if (sServerOrDSN == null)
        {
            sServerOrDSN = aProps.getProperty(AltibaseProperties.PROP_DATASOURCE_NAME);
            if (sServerOrDSN == null)
            {
                sServerOrDSN = AltibaseProperties.DEFAULT_SERVER;
            }
        }
        sBuf.append(sServerOrDSN);
        String sPort = aProps.getProperty(AltibaseProperties.PROP_PORT);
        if (sPort != null)
        {
            sBuf.append(':');
            sBuf.append(sPort);
        }
        String sDBName = aProps.getProperty(AltibaseProperties.PROP_DBNAME);
        if (sDBName != null)
        {
            sBuf.append('/');
            sBuf.append(sDBName);
        }

        
        boolean sIsFirst = true;
        Enumeration sKeyEnum = aProps.propertyNames();
        while (sKeyEnum.hasMoreElements())
        {
            String sKey = (String)sKeyEnum.nextElement();

            if (AltibaseProperties.PROP_SERVER.equals(sKey) ||
                AltibaseProperties.PROP_DATASOURCE_NAME.equals(sKey) ||
                AltibaseProperties.PROP_DBNAME.equals(sKey) ||
                AltibaseProperties.PROP_PORT.equals(sKey))
            {
                continue;
            }

            if (sIsFirst == true)
            {
                sBuf.append('?');
                sIsFirst = false;
            }
            else
            {
                sBuf.append('&');
            }

            sBuf.append(sKey);
            sBuf.append('=');
            sBuf.append(aProps.getProperty(sKey));
        }
        return sBuf.toString();
    }

    CmChannel channel()
    {
        return mChannel;
    }

    void setChannel(CmChannel aNewChannel)
    {
        mChannel = aNewChannel;
        mContext = new CmProtocolContextConnect(mChannel);
    }

    AltibaseProperties getProperties()
    {
        return (AltibaseProperties) mProp.clone();
    }

    int makeStatementCID()
    {
        int sStmtCID;
        synchronized (mCurrentCIDSeqLock)
        {
            while (mUsedCIDSet.get(mCurrentCIDSeq))
            {
                mCurrentCIDSeq = (mCurrentCIDSeq + 1) % STMT_CID_SEQ_MAX;
            }
            mUsedCIDSet.set(mCurrentCIDSeq);

            sStmtCID = (getSessionId() << STMT_CID_SEQ_BIT) | mCurrentCIDSeq;
            mCurrentCIDSeq = (mCurrentCIDSeq + 1) % STMT_CID_SEQ_MAX;
        }
        return sStmtCID;
    }

    boolean nliteralReplaceOn()
    {
        return mNliteralReplace;
    }

    void removeStatement(AltibaseStatement aStatement)
    {
        synchronized (mStatementList)
        {
            mUsedCIDSet.clear(aStatement.getCID() & STMT_CID_SEQ_MASK);
            mStatementList.remove(aStatement);
        }
    }

    public void clearWarnings() throws SQLException
    {
        throwErrorForClosed();

        mWarning = null;
    }

    public void close() throws SQLException
    {
        if (mIsClosed)
        {
            return;
        }

        try
        {
            // BUGBUG 이거 꼭 이래야 되나? synchronized 제거할 수 없나?
            // BUGBUG (1013-02-04) 비용이 좀 들긴 하지만, rollbkack 할 때 stmt 남은걸 확인하므로 모두 close 해주어야 안전하다.
            synchronized (mStatementList)
            {
                while (!mStatementList.isEmpty())
                {
                    AltibaseStatement sStmt = (AltibaseStatement)mStatementList.getFirst();
                    sStmt.close();
                }
            }

            // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
            clearAsyncPrefetchStatement();

            if (!isServerSideAutoCommit())
            {
                // autocommit이 아닌 경우 그냥 disconnect를 해서
                // rollback/commit 여부를 서버에 맡기는게 옳은 정책이지만,
                // 하위 버전과의 호환성을 위해 rollback을 호출한다.
                rollback();
            }
        }
        finally
        {
            // BUG-49087 close 도중 예외가 발생하더라도 socket을 정리할 수 있도록 finally 절 안에서 disconnect()를 처리한다.
            disconnect();
        }
    }

    public int getSessionId()
    {
        return mContext.getConnectExResult().getSessionID();
    }

    /**
     * Disconnect를 수행하고 CM 연결을 끝는다.
     * 
     * @throws SQLException
     */
    void disconnect() throws SQLException
    {
        // RESPONSE_TIMEOUT 등으로 channel이 닫혔다면 조용히 넘어간다.
        if (!mContext.channel().isClosed())
        {
            try
            {
                if (!mIsClosed)
                {
                    CmProtocol.disconnect(mContext);
                }
                if (mContext.getError() != null)
                {
                    mWarning = Error.processServerError(mWarning, mContext.getError());
                }
            }
            finally
            {
                try
                {
                    // BUG-49087 diconnect 프로토콜 처리시 에러가 발생한 경우에도 socket을 정리해야 한다.
                    mChannel.close();
                }
                catch (IOException sIOEx)
                {
                    logExceptionToLogWriter(sIOEx);
                }
            }
        }

        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
        clearAsyncPrefetchStatement();

        mIsClosed = true;
    }

    /**
     * Connection을 닫는다.
     * 이 때, 예외가 발생하면 LogWriter에 메세지를 기록한다.
     */
    void quiteClose()
    {
        try
        {
            close();
        }
        catch (SQLException sEx)
        {
            logExceptionToLogWriter(sEx);
        }
    }

    /**
     * PROJ-2109 DriverManager.setLogWriter로 셋팅한 logWriter로 Exception을 출력한다.
     * @param sEx
     */
    private void logExceptionToLogWriter(Exception sEx)
    {
        PrintWriter logWriter = getLogWriter();
        if (logWriter == null)
        {
            return; // DriverManager 단에서 LogWriter를 설정하지 않은 경우 그냥 리턴한다.
        }
        logWriter.print(sEx.getMessage());
    }

    public void commit() throws SQLException
    {
        throwErrorForClosed();
        // BUG-23343 JDBC spec에 따르면 예외를 내야 하지만, 사용자 편의를 위해 무시.
        if (isServerSideAutoCommit())
        {
            return;
        }
        try
        {
            CmProtocol.commit(mContext);
        }
        catch (SQLException aEx)
        {
            AltibaseShardingFailover.tryShardFailOver(this, aEx);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
                setSMNInvalidErrorResults();
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
    }

    /* PROJ-2733
     * library session으로 DB_OP_SHARD_TRANSACTION_V3을 전송하기 위한 함수
     */
    public void shardTransaction(boolean aIsCommit) throws SQLException
    {
        throwErrorForClosed();
        // BUG-23343 JDBC spec에 따르면 예외를 내야 하지만, 사용자 편의를 위해 무시.
        if (isServerSideAutoCommit())
        {
            return;
        }
        try
        {
            mMetaConnection.getShardProtocol().shardTransaction(mContext, aIsCommit);
        }
        catch (SQLException aEx)
        {
            AltibaseShardingFailover.tryShardFailOver(this, aEx);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
                setSMNInvalidErrorResults();
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
    }

    public void shardStmtPartialRollback() throws SQLException
    {
        throwErrorForClosed();

        try
        {
            mMetaConnection.getShardProtocol().shardStmtPartialRollback(mContext);
        }
        catch (SQLException aEx)
        {
            AltibaseShardingFailover.tryShardFailOver(this, aEx);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
                setSMNInvalidErrorResults();
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
    }

    /**
     * SMN Invalid 에러가 넘어온 경우 파싱한 SMN값과 needToDisconnect 값을 node connection 및
     * meta connection에 저장한다.
     */
    private void setSMNInvalidErrorResults()
    {
        if (mContext.getError().isInvalidSMNError())
        {
            long sSMN = mContext.getError().getSMNOfDataNode();
            getMetaConnection().setShardMetaNumberOfDataNode(sSMN);

            boolean sIsNeedToDisconnect = mContext.getError().isNeedToDisconnect();
            getMetaConnection().setNeedToDisconnect(sIsNeedToDisconnect);
        }
    }

    public Statement createStatement() throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();

        AltibaseStatement sStatement = new AltibaseStatement(this, mDefaultResultSetType, mDefaultResultSetConcurrency, mDefaultResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public Statement createStatement(int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        
        AltibaseStatement sStatement = new AltibaseStatement(this, aResultSetType, aResultSetConcurrency, mDefaultResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public Statement createStatement(int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        
        AltibaseStatement sStatement = new AltibaseStatement(this, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public boolean getAutoCommit() throws SQLException
    {
        throwErrorForClosed();
        return isServerSideAutoCommit();
    }

    public String getCatalog() throws SQLException
    {
        throwErrorForClosed();
        return mProp.getDatabase();
    }

    public String getUserName() throws SQLException
    {
        throwErrorForClosed();
        return mProp.getUser();
    }

    public int getHoldability() throws SQLException
    {
        throwErrorForClosed();
        return mDefaultResultSetHoldability;
    }

    public DatabaseMetaData getMetaData() throws SQLException
    {
        throwErrorForClosed();
        if (mMetaData == null)
        {
            mMetaData = new AltibaseDatabaseMetaData(this);
        }
        return mMetaData;
    }

    public int getTransactionIsolation() throws SQLException
    {
        throwErrorForClosed();
        return mTxILevel;
    }

    public Map getTypeMap() throws SQLException
    {
        throwErrorForClosed();
        return EMPTY_TYPEMAP;
    }

    public SQLWarning getWarnings() throws SQLException
    {
        throwErrorForClosed();
        return mWarning;
    }

    public boolean isClosed() throws SQLException
    {
        return mIsClosed;
    }

    public boolean isReadOnly() throws SQLException
    {
        throwErrorForClosed();
        // read-only 모드를 지원하지 않는다.
        return false;
    }

    public String nativeSQL(String aSql) throws SQLException
    {
        throwErrorForClosed();

        return AltiSqlProcessor.processEscape(aSql);        
    }

    public CallableStatement prepareCall(String aSql, int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        throwErrorForClosed();
        AltibaseCallableStatement sStatement = new AltibaseCallableStatement(this, aSql, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public CallableStatement prepareCall(String aSql, int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        throwErrorForClosed();
        AltibaseCallableStatement sStatement = new AltibaseCallableStatement(this, aSql, aResultSetType, aResultSetConcurrency, mDefaultResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public CallableStatement prepareCall(String aSql) throws SQLException
    {
        throwErrorForClosed();
        AltibaseCallableStatement sStatement = new AltibaseCallableStatement(this, aSql, mDefaultResultSetType, mDefaultResultSetConcurrency, mDefaultResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public PreparedStatement prepareStatement(String aSql, int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        
        AltibasePreparedStatement sStatement = new AltibasePreparedStatement(this, aSql, aResultSetType, aResultSetConcurrency, mDefaultResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public PreparedStatement prepareStatement(String aSql, int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        
        AltibasePreparedStatement sStatement = new AltibasePreparedStatement(this, aSql, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    // BUGBUG (2012-11-06) 지원하는 것이 스펙에서 설명하는것과 조금 다르다.
    // 자세한 내용은 Statement.executeUpdate(String,int) 참고.
    public PreparedStatement prepareStatement(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        AltibaseStatement.checkAutoGeneratedKeys(aAutoGeneratedKeys);

        AltibasePreparedStatement sStatement = new AltibasePreparedStatement(this, aSql, mDefaultResultSetType, mDefaultResultSetConcurrency, mDefaultResultSetHoldability);
        if (aAutoGeneratedKeys == Statement.RETURN_GENERATED_KEYS)
        {
            sStatement.makeQstrForGeneratedKeys(aSql, null, null);
        }
        else
        {
            sStatement.clearForGeneratedKeys();
        }
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    // BUGBUG (2012-11-06) 스펙과 다르다.
    // 자세한 내용은 Statement.executeUpdate(String,int[]) 참고.
    public PreparedStatement prepareStatement(String aSql, int[] aColumnIndexes) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        
        AltibasePreparedStatement sStatement = new AltibasePreparedStatement(this, aSql, mDefaultResultSetType, mDefaultResultSetConcurrency, mDefaultResultSetHoldability);
        sStatement.makeQstrForGeneratedKeys(aSql, aColumnIndexes, null);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    // BUGBUG (2012-11-06) 스펙과 다르다.
    // 자세한 내용은 Statement.executeUpdate(String,String[]) 참고.
    public PreparedStatement prepareStatement(String aSql, String[] aColumnNames) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        
        AltibasePreparedStatement sStatement = new AltibasePreparedStatement(this, aSql, mDefaultResultSetType, mDefaultResultSetConcurrency, mDefaultResultSetHoldability);
        sStatement.makeQstrForGeneratedKeys(aSql, null, aColumnNames);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public PreparedStatement prepareStatement(String aSql) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForTooManyStatements();
        
        AltibasePreparedStatement sStatement = new AltibasePreparedStatement(this, aSql, mDefaultResultSetType, mDefaultResultSetConcurrency, mDefaultResultSetHoldability);
        synchronized (mStatementList)
        {
            mStatementList.add(sStatement);
        }
        return sStatement;
    }

    public void releaseSavepoint(Savepoint aSavepoint) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForInvalidSavePoint(aSavepoint);
        // BUG-23343 JDBC spec에 따르면 예외를 내야 하지만, 사용자 편의를 위해 무시.
        if (isServerSideAutoCommit())
        {
            return;
        }

        ((AltibaseSavepoint)aSavepoint).releaseSavepoint();
    }

    public void rollback() throws SQLException
    {
        throwErrorForClosed();
        // BUG-23343 JDBC spec에 따르면 예외를 내야 하지만, 사용자 편의를 위해 무시.
        if (isServerSideAutoCommit())
        {
            return;
        }

        try
        {
            CmProtocol.rollback(mContext);
        }
        catch (SQLException aEx)
        {
            AltibaseShardingFailover.tryShardFailOver(this, aEx);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
                setSMNInvalidErrorResults();
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
    }

    public void rollback(Savepoint aSavepoint) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForInvalidSavePoint(aSavepoint);
        // BUG-23343 JDBC spec에 따르면 예외를 내야 하지만, 사용자 편의를 위해 무시.
        if (isServerSideAutoCommit())
        {
            return;
        }
        
        ((AltibaseSavepoint)aSavepoint).rollback();
    }

    public void setAutoCommit(boolean aAutoCommit) throws SQLException
    {
        throwErrorForClosed();
        if (mProp.isClientSideAutoCommit()) 
        {
            /* PROJ-2190 ClientAutoCommit이 활성화 되어 있을 때는 서버로 커밋정보를 전송하지 않고 flag값만 셋팅한다. */
            
            if (!aAutoCommit)
            {
                mAutoCommit = SERVER_SIDE_AUTOCOMMIT_OFF;
            }
            else
            {
                mAutoCommit = CLIENT_SIDE_AUTOCOMMIT_ON;
            }
            return;
        }
        
        if (isServerSideAutoCommit() == aAutoCommit)
        {
            return;
        }

        // BUGBUG spec에선 off ==> on 일 때만 commit된다는 말이 없다. 무조건 commit 해줘야 할까?
        if (isServerSideAutoCommit() == false)
        {
            commit();
        }

        mContext.clearProperties();
        mContext.addProperty(AltibaseProperties.PROP_CODE_AUTOCOMMIT, aAutoCommit);
        try
        {
            CmProtocol.sendProperties(mContext);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mFailoverContext, ex);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
        
        mAutoCommit = (aAutoCommit) ? SERVER_SIDE_AUTOCOMMIT_ON : SERVER_SIDE_AUTOCOMMIT_OFF;
        
        if (mAutoCommit == SERVER_SIDE_AUTOCOMMIT_OFF)
        {
            mContext.initDistTxInfo();
            setDistTxInfoForVerify();
        }
    }

    public void setCatalog(String aCatalog) throws SQLException
    {
        throwErrorForClosed();

        // shard connection일 경우 catalog에 shardpin정보가 있으면 shardpin을 셋팅한다.
        if (!StringUtils.isEmpty(aCatalog) && aCatalog.startsWith("shardpin:"))
        {
            int sIndex = aCatalog.indexOf("shardpin:");
            String sShardPin = aCatalog.substring(sIndex+9, aCatalog.length());
            mContext.clearProperties();
            mContext.addProperty(AltibaseProperties.PROP_CODE_SHARD_PIN, sShardPin);
            CmProtocol.sendProperties(mContext);
            if (mContext.getError() != null)
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
            }

            return;
        }

        // 아무런 동작을 하지 않는다. dbname을 바꿀 수 없다.
        mWarning = Error.createWarning(mWarning, ErrorDef.CANNOT_RENAME_DB_NAME);
    }

    public void setHoldability(int aHoldability) throws SQLException
    {
        throwErrorForClosed();
        AltibaseResultSet.checkHoldability(aHoldability);

        mDefaultResultSetHoldability = aHoldability;
    }

    public void setReadOnly(boolean aReadOnly) throws SQLException
    {
        throwErrorForClosed();

        // 아무런 동작을 하지 않는다. read-only 모드를 지원하지 않는다.
        mWarning = (aReadOnly) ? Error.createWarning(mWarning, ErrorDef.READONLY_CONNECTION_NOT_SUPPORTED) : mWarning;
    }
    
    public Savepoint setSavepoint() throws SQLException
    {
        throwErrorForClosed();
        throwErrorForSavePoint();
        AltibaseSavepoint sSavepoint = new AltibaseSavepoint(this);
        sSavepoint.setSavepoint();
        return sSavepoint;
    }

    public Savepoint setSavepoint(String aName) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForSavePoint();
        AltibaseSavepoint sSavepoint = new AltibaseSavepoint(this, aName);
        sSavepoint.setSavepoint();
        return sSavepoint;
    }

    public void setTransactionIsolation(int aLevel) throws SQLException
    {
        throwErrorForClosed();
        if (isServerSideAutoCommit())
        {
            Error.throwSQLException(ErrorDef.TXI_LEVEL_CANNOT_BE_MODIFIED_FOR_AUTOCOMMIT);            
        }

        /* BUG-39817 */
        if (aLevel == TRANSACTION_READ_UNCOMMITTED)
        {
            mWarning = Error.createWarning(mWarning,
                                           ErrorDef.OPTION_VALUE_CHANGED,
                                           "TRANSACTION_READ_UNCOMMITTED(1) changed to TRANSACTION_READ_COMMITTED(2)");
            aLevel = TRANSACTION_READ_COMMITTED;
        }

        String sTxIsolationLevel = null;
        switch (aLevel)
        {
            case TRANSACTION_READ_COMMITTED:
                sTxIsolationLevel = TX_LEVEL_READ_COMMITTED;
                break;
            case TRANSACTION_REPEATABLE_READ:
                sTxIsolationLevel = TX_LEVEL_REPEATABLE_READ;
                break;
            case TRANSACTION_SERIALIZABLE:
                sTxIsolationLevel = TX_LEVEL_SERIALIZABLE;
                break;
            default:
                Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                        "Transcation isolation level",
                                        "TRANSACTION_READ_COMMITTED | TRANSACTION_REPEATABLE_READ | TRANSACTION_SERIALIZABLE ",
                                        String.valueOf(aLevel));
                break;
        }
        mInternalStatement.executeUpdate(INTERNAL_SQL_ALTER_SESSION_SET_TXI_LEVEL + sTxIsolationLevel);
        mTxILevel = aLevel;
    }

    public void setTypeMap(Map map) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "User defined type");
    }
    
    public boolean isClientSideAutoCommit()
    {
        return mAutoCommit == CLIENT_SIDE_AUTOCOMMIT_ON;
    }

    protected boolean isDeferredPrepare()
    {
        return mProp.isDeferredPrepare();
    }
    
    /**
     * 연결된 DB의 패키지 버전을 얻는다.
     * 
     * @return 연결된 DB의 패키지 버전 문자열
     * @throws SQLException 패키지 버전 문자열을 얻는데 실패했을 경우
     */
    public String getDatabaseVersion() throws SQLException
    {
        throwErrorForClosed();

        if (mDBPkgVerStr == null)
        {
            try
            {
                CmProtocol.getProperty(mContext, AltibaseProperties.PROP_CODE_SERVER_PACKAGE_VERSION);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mFailoverContext, ex);
            }
            if (mContext.getError() != null)
            {
                try
                {
                    mWarning = Error.processServerError(mWarning, mContext.getError());
                }
                finally
                {
                    // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                    ShardError.processShardError(mMetaConnection, mContext.getError());
                }
            }
            mDBPkgVerStr = mContext.getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_SERVER_PACKAGE_VERSION);
        }
        return mDBPkgVerStr;
    }

    /*
     * JDBC 스펙에 없는 public 메소드로서 다음  메소드를 제공한다.
     * XAResult의 getTransactionTimeout을 위해 UTRANS_TIMEOUT 속성을 얻고, 세팅하는 메소드를 제공한다.
     */
    public int getTransTimeout() throws SQLException
    {
        throwErrorForClosed();

        try
        {
            CmProtocol.getProperty(mContext, AltibaseProperties.PROP_CODE_UTRANS_TIMEOUT);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mFailoverContext, ex);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
        return Integer.parseInt(mContext.getPropertyResult().getProperty(AltibaseProperties.PROP_CODE_UTRANS_TIMEOUT));
    }

    public void setTransTimeout(int aTimeoutSec) throws SQLException
    {
        throwErrorForClosed();

        mContext.clearProperties();
        mContext.addProperty(AltibaseProperties.PROP_CODE_UTRANS_TIMEOUT, aTimeoutSec);
        try
        {
            CmProtocol.sendProperties(mContext);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mFailoverContext, ex);
        }
        if (mContext.getError() != null)
        {
            mWarning = Error.processServerError(mWarning, mContext.getError());
        }
    }

    // #region TimeZone

    /**
     * 서버에 설정된 TimeZone을 얻는다.
     *
     * @return 서버 TimeZone 값
     * @throws SQLException DB TimeZone을 얻는데 실패했을 경우
     */
    public String getDbTimeZone() throws SQLException
    {
        throwErrorForClosed();

        if (mDbTimeZone == null)
        {
            // 프로토콜이 없으니 쿼리로 얻어온다.
            ResultSet sRS = mInternalStatement.executeQuery("SELECT db_timezone() FROM DUAL");
            if (sRS.next())
            {
                mDbTimeZone = sRS.getString(1);
            }
            sRS.close();
        }
        return mDbTimeZone;
    }

    /**
     * Session에 설정된 TimeZone을 얻는다.
     *
     * @return 서버 TimeZone 값
     * @throws SQLException Session TimeZone을 얻는데 실패했을 경우
     */
    public String getSessionTimeZone() throws SQLException
    {
        throwErrorForClosed();

        if (mSessionTimeZone == null)
        {
            // 프로토콜이 없으니 쿼리로 얻어온다.
            ResultSet sRS = mInternalStatement.executeQuery("SELECT session_timezone() FROM DUAL");
            if (sRS.next())
            {
                mSessionTimeZone = sRS.getString(1);
            }
            sRS.close();
        }
        return mSessionTimeZone;
    }

    /**
     * Session TimeZone을 바꾼다.
     *
     * @param aTimeZone 바꿀 TimeZone 값
     * @throws SQLException TimeZone을 바꾸는데 실패했을 경우
     */
    public void setSessionTimeZone(String aTimeZone) throws SQLException
    {
        throwErrorForClosed();

        if (aTimeZone.equalsIgnoreCase(mSessionTimeZone))
        {
            return;
        }

        if (aTimeZone.equalsIgnoreCase(AltibaseProperties.LOCAL_TIME_ZONE))
        {
            aTimeZone = Calendar.getInstance().getTimeZone().getID();
        }
        mContext.clearProperties();
        mContext.addProperty(AltibaseProperties.PROP_CODE_TIME_ZONE, aTimeZone);
        try
        {
            CmProtocol.sendProperties(mContext);
        }
        catch (SQLException sEx)
        {
            AltibaseFailover.trySTF(mFailoverContext, sEx);
        }
        if (mContext.getError() != null)
        {
            mWarning = Error.processServerError(mWarning, mContext.getError());
        }
        mSessionTimeZone = aTimeZone.equalsIgnoreCase(AltibaseProperties.DB_TIME_ZONE) ? getDbTimeZone() : aTimeZone;
    }

    // #endregion

    // #region Explain Plan

    private boolean isValidExplainPlanMode(byte aExplainPlanMode)
    {
        switch (aExplainPlanMode)
        {
            case EXPLAIN_PLAN_OFF:
            case EXPLAIN_PLAN_ON:
            case EXPLAIN_PLAN_ONLY:
                return true;
            default:
                return false;
        }
    }

    int explainPlanMode()
    {
        return mExplainPlanMode;
    }

    /**
     * Explain Plan Mode를 설정한다.
     *
     * @param aExplainPlanMode Explain Plan Mode. 다음 값 중 하나:
     *                         {@link #EXPLAIN_PLAN_OFF},
     *                         {@link #EXPLAIN_PLAN_ON},
     *                         {@link #EXPLAIN_PLAN_ONLY}
     *
     * @exception IllegalArgumentException Explain Plan Mode가 올바르지 않을 경우
     * @exception SQLException Explain Plan 속성 설정에 실패한 경우
     */
    public void setExplainPlan(byte aExplainPlanMode) throws SQLException
    {
        throwErrorForClosed();
        if (isValidExplainPlanMode(aExplainPlanMode) == false)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Explain plan mode",
                                    "EXPLAIN_PLAN_OFF | EXPLAIN_PLAN_ON | EXPLAIN_PLAN_ONLY",
                                    String.valueOf(aExplainPlanMode));
        }
        mContext.clearProperties();
        mContext.addProperty(AltibaseProperties.PROP_CODE_EXPLAIN_PLAN, aExplainPlanMode);
        try
        {
            CmProtocol.sendProperties(mContext);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mFailoverContext, ex);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
        mExplainPlanMode = aExplainPlanMode;
    }

    /**
     * Explain Plan을 사용할지 여부를 설정한다.
     *
     * @param aUseExplainPlan ExplainPlan을 사용할지 여부
     *
     * @exception SQLException Explain Plan 속성 설정에 실패한 경우
     *
     * @deprecated Replaced by {@link #setExplainPlan(byte)}
     */
    public void setExplainPlan(boolean aUseExplainPlan) throws SQLException
    {
        setExplainPlan(aUseExplainPlan ? EXPLAIN_PLAN_ON : EXPLAIN_PLAN_OFF);
    }

    // #endregion

    /* Failover */

    public void registerFailoverCallback(AltibaseFailoverCallback aFailoverCallback, Object aAppContext) throws SQLException
    {
        throwErrorForClosed();

        if (mFailoverContext == null)
        {
            return;
        }

        mFailoverContext.setCallback(aFailoverCallback);
        mFailoverContext.setAppContext(aAppContext);
    }

    public void deregisterFailoverCallback() throws SQLException
    {
        throwErrorForClosed();

        if (mFailoverContext == null)
        {
            return;
        }

        mFailoverContext.setCallback(null);
        mFailoverContext.setAppContext(null);
    }

    /**
     * DB Message 프로토콜을 처리할 콜백을 등록한다.
     * @param aMessageCallback 콜백 객체
     */
    public void registerMessageCallback(AltibaseMessageCallback aMessageCallback) throws SQLException
    {
        throwErrorForClosed();

        AltibaseMessageCallback sOldMessageCallback = mChannel.getMessageCallback();

        mChannel.setMessageCallback(aMessageCallback);

        /* BUG-46019 필요에 따라 서버에 콜백 등록 여부를 전달한다. */
        if ((sOldMessageCallback == aMessageCallback) ||
            (sOldMessageCallback != null) && (aMessageCallback != null))
        {
            return;
        }

        mContext.clearProperties();
        mContext.addProperty(AltibaseProperties.PROP_CODE_MESSAGE_CALLBACK, aMessageCallback != null);
        try
        {
            CmProtocol.sendProperties(mContext);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mFailoverContext, ex);
        }
        if (mContext.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mContext.getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mMetaConnection, mContext.getError());
            }
        }
    }

    /**
     * PROJ-2474 가능한 ciphersuite 리스트를 돌려준다.</br>
     * ssl_enable이 true일때만 소켓을 통해 값을 가져오며 그외에는 null을 리턴한다.
     */
    public String[] getCipherSuiteList()
    {
        return mChannel.getCipherSuitList();
    }
    
    public AltibaseFailoverContext failoverContext()
    {
        return mFailoverContext;
    }

    /**
     * STF를 수행하기 전에, 무효화된 Statement의 상태를 정리하기 위해서 수행한다.
     */
    public void clearStatements4STF()
    {
        synchronized (mStatementList)
        {
            try
            {
                Iterator sIt = mStatementList.iterator();
                while (sIt.hasNext())
                {
                    AltibaseStatement sStmt = (AltibaseStatement)sIt.next();
                    sStmt.close4STF();
                }
                mStatementList.clear();
            }
            catch (SQLException sEx)
            {
                // Statement.close4STF()에서 사용하는 메소드의 인터페이스 정의가
                // SQLException을 던질 수 있게 되어있어서 그렇지 구현상 절대 날 일이 없다.
                Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION, sEx);
            }
            mUsedCIDSet.clear();
        }
    }

    void setRelatedXAResource(AltibaseXAResource aXAResource)
    {
        if (mFailoverContext == null)
        {
            return;
        }

        mFailoverContext.setRelatedXAResource(aXAResource);
    }



    // #region for Savepoint ID
    // BUGBUG (2013-02-06) 굳이 lock으로 인한 약간의 성능저하를 감수하면서 이렇게까지 해야하나 싶기도 하다.
    // oracle은 그 순차값만 동시에 가져올 수 없게 synchronized하고 뺑뺑이 돌며 재사용하게 했던데.. 쩝.

    private static final int STMT_SPID_SEQ_BIT   = 16;
    private static final int STMT_SPID_SEQ_MAX   = (1 << STMT_SPID_SEQ_BIT);
    private static final int STMT_SPID_SEQ_MASK  = (STMT_SPID_SEQ_MAX - 1);

    private int              mCurrentSPIDSeq     = 0;
    private Object           mCurrentSPIDSeqLock = new Object();
    private BitSet           mUsedSPIDSet        = new BitSet(STMT_SPID_SEQ_MAX);

    int newSavepointId()
    {
        int sSavepointId;
        synchronized (mCurrentSPIDSeqLock)
        {
            while (mUsedSPIDSet.get(mCurrentSPIDSeq))
            {
                mCurrentSPIDSeq = (mCurrentSPIDSeq + 1) % STMT_SPID_SEQ_MAX;
            }
            mUsedSPIDSet.set(mCurrentSPIDSeq);

            sSavepointId = (getSessionId() << STMT_SPID_SEQ_BIT) | mCurrentSPIDSeq;
            mCurrentSPIDSeq = (mCurrentSPIDSeq + 1) % STMT_SPID_SEQ_MAX;
        }
        return sSavepointId;
    }

    void releaseSavepointId(int aSavepointId)
    {
        synchronized (mCurrentSPIDSeqLock)
        {
            mUsedSPIDSet.clear(aSavepointId & STMT_SPID_SEQ_MASK);
        }
    }

    private void throwErrorForTooManyStatements() throws SQLException
    {
        if (mStatementList.size() >= STMT_CID_SEQ_MAX)
        {
            Error.throwSQLException(ErrorDef.TOO_MANY_STATEMENTS);
        }
    }
    
    private void checkDBNameAndUserInfo(String aDBName, String aUser, String aPassword) throws SQLException
    {
        if (aDBName == null)
        {
            Error.throwSQLException(ErrorDef.NO_DB_NAME_SPECIFIED);
        }
        if (aUser == null)
        {
            Error.throwSQLException(ErrorDef.NO_USER_ID_SPECIFIED);
        }
        if (aPassword == null)
        {
            Error.throwSQLException(ErrorDef.NO_PASSWORD_SPECIFIED);
        }
    }
    
    private void throwErrorForInvalidSavePoint(Savepoint aSavepoint) throws SQLException
    {
        if (!(aSavepoint instanceof AltibaseSavepoint) ||
            ((AltibaseSavepoint)aSavepoint).getConnection() != this)
        {
            Error.throwSQLException(ErrorDef.INVALID_SAVEPOINT);
        }
    }

    private void throwErrorForSavePoint() throws SQLException
    {
        if (isServerSideAutoCommit())
        {
            Error.throwSQLException(ErrorDef.CANNOT_SET_SAVEPOINT_AT_AUTO_COMMIT_MODE);
        }
    }
    // #endregion

    /**
     * Asynchronous fetch 기능이 동작 중인 statement 를 저장한다.
     */
    protected boolean setAsyncPrefetchStatement(AltibaseStatement aStatement)
    {
        synchronized (mStatementList)
        {
            if (mAsyncPrefetchStatement != null)
            {
                if (mAsyncPrefetchStatement != aStatement)
                {
                    return false;
                }
            }

            mAsyncPrefetchStatement = aStatement;

            return true;
        }
    }

    /**
     * Asynchronous fetch 기능이 동작 중인 statement 를 저장한다.
     */
    protected void clearAsyncPrefetchStatement()
    {
        synchronized (mStatementList)
        {
            mAsyncPrefetchStatement = null;
        }
    }

    /**
     * Asynchronous fetch 기능이 동작 중인 statement 를 얻는다.
     */
    protected AltibaseStatement getAsyncPrefetchStatement()
    {
        synchronized (mStatementList)
        {
            return mAsyncPrefetchStatement;
        }
    }

    public boolean isNodeConnection()
    {
        return mShardConnType == ShardConnType.NODE_CONNECTION;
    }

    public boolean isMetaConnection()
    {
        return mShardConnType == ShardConnType.META_CONNECTION;
    }

    public AltibaseShardingConnection getMetaConnection()
    {
        return mMetaConnection;
    }

    public String getServerCharacterSet() throws SQLException
    {
        return mContext.getCharsetName();
    }

    public AltibaseProperties getProp()
    {
        return mProp;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("AltibaseConnection{");
        sSb.append("mIsClosed=").append(mIsClosed);
        sSb.append(", mAutoCommit=").append(mAutoCommit);
        sSb.append(", mID=").append(getSessionId());
        sSb.append(", mShardConnType=").append(mShardConnType);
        sSb.append('}');
        return sSb.toString();
    }

    public void sendSMNProperty(long aNewSMN) throws SQLException
    {
        mContext.clearProperties();
        mContext.addProperty(AltibaseProperties.PROP_CODE_SHARD_META_NUMBER, aNewSMN);
        CmProtocol.sendProperties(mContext);

        if (mContext.getError() != null)
        {
            mWarning = Error.processServerError(mWarning, mContext.getError());
        }
    }

    public String getServer()
    {
        return mServer;
    }

    public int getPort()
    {
        return mPort;
    }

    public void setClosed(boolean aClosed)
    {
        mIsClosed = aClosed;
        // BUG-46790 meta connection의 close flag도 같이 셋팅해 준다.
        if (isMetaConnection())
        {
            getMetaConnection().setClosed(aClosed);
        }
    }

    public boolean getAllowLobNullSelect()
    {
        return mAllowLobNullSelect;
    }

    public DistTxInfo getDistTxInfo()
    {
        return mContext.getDistTxInfo();
    }

    public void setDistTxInfoForVerify()
    {
        if (SHARD_JDBC_DISTTXINFO_VERIFY)
        {
            mDistTxInfoForVerify.mEnvSCN          = CmProtocolContext.getSCN();
            mDistTxInfoForVerify.mSCN             = getDistTxInfo().getSCN();
            mDistTxInfoForVerify.mTxFirstStmtSCN  = getDistTxInfo().getTxFirstStmtSCN();
            mDistTxInfoForVerify.mTxFirstStmtTime = getDistTxInfo().getTxFirstStmtTime();
            mDistTxInfoForVerify.mDistLevel       = getDistTxInfo().getDistLevel();
        }
    }

    public DistTxInfoForVerify getDistTxInfoForVerify()
    {
        DistTxInfoForVerify sDistTxInfoForVerify = null;
        
        try
        {
            sDistTxInfoForVerify = (DistTxInfoForVerify)mDistTxInfoForVerify.clone();
        }
        catch (CloneNotSupportedException e)
        {   
            e.printStackTrace();
        }   

        return sDistTxInfoForVerify;
    }
    
    public CmProtocolContextConnect getContext()
    {
        return mContext;
    }

    public boolean canReUseResultSet()
    {
        return mReUseResultSet;
    }

    @Override
    public boolean isValid(int aTimeout) throws SQLException
    {
        if (aTimeout < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT, "Connection.isValid() timeout", "0 ~ Integer.MAX_VALUE",
                                    String.valueOf(aTimeout));
        }
        if (mIsClosed)
        {
            return false;
        }
        int sOldNetworkTimeout = getNetworkTimeout();
        try
        {
            setNetworkTimeout(null, aTimeout * 1000);
            ping();

            return true;
        }
        catch (SQLException aEx)
        {
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.SEVERE, "Validate connection failed.", aEx);
            }
            mChannel.quietClose(); // PROJ-2707 ping 프로토콜이 실패한 경우 네트워크 자원을 해제한다.
            mIsClosed = true;
            return false;
        }
        finally
        {
            if (!mChannel.isClosed())
            {
                setNetworkTimeout(null, sOldNetworkTimeout);
            }
        }
    }
    
    @Override
    public void abort(Executor aExecutor) throws SQLException
    {
        SecurityManager sSecurityManager = System.getSecurityManager();
        if (sSecurityManager != null)
        {
            sSecurityManager.checkPermission(ABORT_PERM);
        }
        if (aExecutor == null)
        {
            Error.throwSQLException(ErrorDef.EXECUTOR_CANNOT_BE_NULL);
        }
        else
        {
            aExecutor.execute(() -> {
                // PROJ-2707 abort같은 경우 네트워크 자원만 close한다.
                if (!mChannel.isClosed())
                {
                    mChannel.quietClose();
                }
                mIsClosed = true;
            });
        }
    }

    @Override
    public void setNetworkTimeout(Executor aExecutor, int aMilliseconds) throws SQLException
    {
        // PROJ-2707 aExecutor는 그냥 무시한다.
        throwErrorForClosed();
        if (aMilliseconds < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT, "Connection.setNetworkTimeout() timeout",
                                    "0 ~ Integer.MAX_VALUE", String.valueOf(aMilliseconds));
        }
        SecurityManager sSecurityManager = System.getSecurityManager();
        if (sSecurityManager != null)
        {
            sSecurityManager.checkPermission(SQL_PERMISSION_NETWORK_TIMEOUT);
        }

        // PROJ-2707 response_timeout은 초단위이다.
        mChannel.setResponseTimeout(aMilliseconds / 1000);
    }

    @Override
    public int getNetworkTimeout() throws SQLException
    {
        throwErrorForClosed();

        return mChannel.getResponseTimeout() * 1000;
    }

    @Override
    public void setClientInfo(String aName, String aValue) throws SQLClientInfoException
    {
        try
        {
            throwErrorForClosed();
        }
        catch (SQLException aCause)
        {
            /* PROJ-2707 커넥션이 close된 경우에는 Properties객체 안에 있는 모든 속성을 unknown으로 바꾸고
               SQLClientInfoException 예외를 던진다.  */
            Map<String, ClientInfoStatus> sFailures = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
            sFailures.put(aName, ClientInfoStatus.REASON_UNKNOWN);
            throw new SQLClientInfoException("This connection has been closed.", sFailures, aCause);
        }
        // PROJ-2707 현재는 클라이언트 속성 중 ApplicationName만 지원한다.
        if (PROP_APPLICATION_NAME.equalsIgnoreCase(aName))
        {
            if (aValue == null)
            {
                aValue = "";
            }
            final String sOldValue = mProp.getAppInfo();
            if (aValue.equals(sOldValue))
            {
                return;
            }
            try
            {
                mContext.clearProperties();
                mContext.addProperty(AltibaseProperties.PROP_CODE_APP_INFO, aValue);
                CmProtocol.sendProperties(mContext);
            }
            catch (SQLException aEx)
            {
                Map<String, ClientInfoStatus> sFailures = new HashMap<>();
                sFailures.put(aName, ClientInfoStatus.REASON_UNKNOWN);
                throw new SQLClientInfoException("Failed to set ClientInfo property: " + aName, sFailures);
             }
            mClientInfo.put(aName, aValue);
            mProp.setAppInfo(aValue);
        }
    }

    @Override
    public void setClientInfo(Properties aProperties) throws SQLClientInfoException
    {
        try
        {
            throwErrorForClosed();
        }
        catch (SQLException aEx)
        {
            /* PROJ-2707 커넥션이 close된 경우에는 Properties객체 안에 있는 모든 속성을 unknown으로 바꾸고
               SQLClientInfoException 예외를 던진다.  */
            Map<String, ClientInfoStatus> sFailures = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
            for (Map.Entry<Object, Object> sEach : aProperties.entrySet())
            {
                sFailures.put((String)sEach.getKey(), ClientInfoStatus.REASON_UNKNOWN);
            }
            throw new SQLClientInfoException("This connection has been closed.", sFailures, aEx);
        }
        Map<String, ClientInfoStatus> sFailures = new HashMap<>();
        try
        {
            if (aProperties.containsKey(PROP_APPLICATION_NAME))
            {
                setClientInfo(PROP_APPLICATION_NAME, aProperties.getProperty(PROP_APPLICATION_NAME, null));
            }
        }
        catch (SQLClientInfoException aEx)
        {
            sFailures.putAll(aEx.getFailedProperties());
        }

        if (!sFailures.isEmpty())
        {
            throw new SQLClientInfoException("One or more ClientInfo failed.", sFailures);
        }
    }

    @Override
    public String getClientInfo(String aName) throws SQLException
    {
        throwErrorForClosed();

        return mClientInfo.getProperty(aName);
    }

    @Override
    public Properties getClientInfo() throws SQLException
    {
        return mClientInfo;
    }

    public boolean getProceduresReturnFunctions()
    {
        return mGetProceduresReturnFunctions;
    }
}
