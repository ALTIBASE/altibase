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

package Altibase.jdbc.driver.sharding.core;

import Altibase.jdbc.driver.*;
import Altibase.jdbc.driver.cm.*;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.ex.ShardError;
import Altibase.jdbc.driver.ex.ShardJdbcException;
import Altibase.jdbc.driver.sharding.executor.ParallelProcessCallback;
import Altibase.jdbc.driver.sharding.executor.ExecutorEngine;
import Altibase.jdbc.driver.sharding.executor.ExecutorExceptionHandler;
import Altibase.jdbc.driver.sharding.executor.MultiThreadExecutorEngine;
import Altibase.jdbc.driver.util.AltiSqlProcessor;
import Altibase.jdbc.driver.util.AltibaseProperties;
import Altibase.jdbc.driver.util.StringUtils;

import javax.sql.DataSource;
import java.sql.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;
import static Altibase.jdbc.driver.util.AltibaseProperties.*;

/**
 * java.sql.Connection 을 구현하며 메타서버와 커넥션을 생성한 후 데이터노드 리스트를 전달받는다.
 */
public class AltibaseShardingConnection extends AbstractConnection
{
    public static final byte                DEFAULT_SHARD_CLIENT         = (byte)1;
    public static final byte                DEFAULT_SHARD_SESSION_TYPE   = (byte)0;
    private AltibaseConnection              mMetaConnection;
    private CmProtocolContextShardConnect   mShardContextConnect;
    private CmShardProtocol                 mCmShardProtocol;
    private Map<DataNode, Connection>       mCachedConnections;
    private AltibaseProperties              mProps;
    private ExecutorEngine                  mExecutorEngine;
    private boolean                         mIsClosed;
    private SQLWarning                      mWarning;
    private int                             mTransactionIsolation        = TRANSACTION_READ_UNCOMMITTED;
    private int                             mDefaultResultSetType        = ResultSet.TYPE_FORWARD_ONLY;
    private int                             mDefaultResultSetConcurrency = ResultSet.CONCUR_READ_ONLY;
    private int                             mDefaultResultSetHoldability = ResultSet.CLOSE_CURSORS_AT_COMMIT;
    private boolean                         mAutoCommit                  = true;
    private GlobalTransactionLevel          mGlobalTransactionLevel;
    private final LinkedList<Statement>     mOpenStatements;
    private boolean                         mIsLazyNodeConnect;
    private boolean                         mIsReshardEnabled;        // BUG-47460 resharding 사용여부
    private AltibaseShardingFailover        mShardFailover;
    private JdbcMethodInvoker               mJdbcMethodInvoker           = new JdbcMethodInvoker();
    private Shard2PhaseCommitState          mShard2PhaseCommitState;
    private short                           mShardStatementRetry;
    private int                             mIndoubtFetchTimeout;
    private short                           mIndoubtFetchMethod;


    public AltibaseShardingConnection(AltibaseProperties aProp) throws SQLException
    {
        mShardContextConnect = new CmProtocolContextShardConnect(this);
        // connstr에 autocommit 설정한 경우, AltibaseShardingConnection.mAutoCommit도 설정해야 함.
        mAutoCommit = aProp.isAutoCommit();
        mMetaConnection = new AltibaseConnection(aProp, null, this);
        mProps = mMetaConnection.getProp();
        mCmShardProtocol = new CmShardProtocol(mShardContextConnect);
        mOpenStatements = new LinkedList<Statement>();
        // mMetaConnection.mDistTxInfo를 mShardContextConnect.mDistTxInfo에 주입한다.
        // mShardContextConnect 생성 시점에 아직 mMetaConnection이 생성되지 않아서 이후에 setDistTxInfo를 별도로 수행해준다.
        mShardContextConnect.setDistTxInfo(mMetaConnection.getDistTxInfo());
        // TCP, SSL, IB
        mShardContextConnect.setShardConnType(mProps.isSet(PROP_SHARD_CONNTYPE) ?
                                              CmConnType.toConnType(mProps.getShardConnType()) :
                                              CmConnType.toConnType(mProps.getConnType()));
        mCachedConnections = new ConcurrentHashMap<DataNode, Connection>();
        mIsLazyNodeConnect = mProps.isShardLazyConnectEnabled();
        mIsReshardEnabled = mProps.isReshardEnabled();
        mShardFailover = new AltibaseShardingFailover(this);
        mShard2PhaseCommitState = Shard2PhaseCommitState.SHARD_2PC_NORMAL;

        try
        {
            mCmShardProtocol.getNodeList();
            // BUG-46513 getNodeList를 호출했을때 서버에러가 발생한 경우 처리해 준다.
            if (mShardContextConnect.getError() != null)
            {
                mWarning = Error.processServerError(mWarning, mShardContextConnect.getError());
            }
            mShardContextConnect.createDataSources(mProps);
            // PROJ-2690 shard_lazy_connect가 false일때는 Meta접속 후 바로 노드커넥션을 확보한다.
            if (!mIsLazyNodeConnect)
            {
                for (DataNode sEach : mShardContextConnect.getShardNodeConfig().getDataNodes())
                {
                    try
                    {
                        getNodeConnection(sEach, false);
                    }
                    catch (SQLException aEx)
                    {
                        // BUG-46513 lazy_connect false일 경우에는 node connect시 에러가 발생했을때 node name을 추가해 줘야 한다.
                        String sSqlState = aEx.getSQLState();
                        int sErrorCode = aEx.getErrorCode();
                        throw new SQLException("[" + sEach.getNodeName() + "] " + aEx.getMessage(), sSqlState, sErrorCode);
                    }
                }
            }
        }
        catch (SQLException aException)
        {
            AltibaseFailover.trySTF(mMetaConnection.failoverContext(), aException);
        }
        mShardContextConnect.setAutoCommit(mAutoCommit);
        mExecutorEngine = new MultiThreadExecutorEngine();
        mShardContextConnect.setServerCharSet(mMetaConnection.getServerCharacterSet());
    }

    public String nativeSQL(String aSql)
    {
        return AltiSqlProcessor.processEscape(aSql);
    }

    public void setAutoCommit(boolean aAutoCommit) throws SQLException
    {
        mAutoCommit = aAutoCommit;
        mShardContextConnect.setAutoCommit(aAutoCommit);
        mJdbcMethodInvoker.recordMethodInvocation(Connection.class, "setAutoCommit",
                                                  new Class[] {boolean.class},
                                                  new Object[] {aAutoCommit});
        for (Connection sEach : mCachedConnections.values())
        {
            sEach.setAutoCommit(aAutoCommit);
        }
        mMetaConnection.setAutoCommit(mAutoCommit);
    }

    public void sendGlobalTransactionLevel(GlobalTransactionLevel aGlobalTransactionLevel) throws SQLException
    {
        AltibaseConnection sNodeConn;
        CmProtocolContextConnect sContext; 
        
        setGlobalTransactionLevel(aGlobalTransactionLevel);

        // alter session set을 서버로 전송했으므로 metaConn은 sendProperties 불필요. nodeConn에 대해서만 처리.
        for (Connection sEach : mCachedConnections.values())
        {
            sNodeConn = (AltibaseConnection)sEach;
            sContext = sNodeConn.getContext();
            sContext.clearProperties();
            sContext.addProperty(PROP_CODE_GLOBAL_TRANSACTION_LEVEL, aGlobalTransactionLevel.getValue());
            CmProtocol.sendProperties(sContext);
            if (sContext.getError() != null) 
            {
                mWarning = Error.processServerError(mWarning, sContext.getError());
            }
            sNodeConn.getProp().setProperty(AltibaseProperties.PROP_GLOBAL_TRANSACTION_LEVEL, aGlobalTransactionLevel.getValue());
        }
        
        getMetaConnection().getDistTxInfo().initDistTxInfo();
        getMetaConnection().setDistTxInfoForVerify();
    }

    public void sendShardStatementRetry(short aShardStatementRetry) throws SQLException
    {
        AltibaseConnection sNodeConn;
        CmProtocolContextConnect sContext; 
        
        setShardStatementRetry(aShardStatementRetry);

        // alter session set을 서버로 전송했으므로 metaConn은 sendProperties 불필요. nodeConn에 대해서만 처리.
        for (Connection sEach : mCachedConnections.values())
        {
            sNodeConn = (AltibaseConnection)sEach;
            sContext = sNodeConn.getContext();
            sContext.clearProperties();
            sContext.addProperty(PROP_CODE_SHARD_STATEMENT_RETRY, (byte)aShardStatementRetry);
            CmProtocol.sendProperties(sContext);
            if (sContext.getError() != null) 
            {
                mWarning = Error.processServerError(mWarning, sContext.getError());
            }
            sNodeConn.getProp().setProperty(AltibaseProperties.PROP_SHARD_STATEMENT_RETRY, aShardStatementRetry);
        }
    }

    public void sendIndoubtFetchTimeout(int aIndoubtFetchTimeout) throws SQLException
    {
        AltibaseConnection sNodeConn;
        CmProtocolContextConnect sContext; 
        
        setIndoubtFetchTimeout(aIndoubtFetchTimeout);

        // alter session set을 서버로 전송했으므로 metaConn은 sendProperties 불필요. nodeConn에 대해서만 처리.
        for (Connection sEach : mCachedConnections.values())
        {
            sNodeConn = (AltibaseConnection)sEach;
            sContext = sNodeConn.getContext();
            sContext.clearProperties();
            sContext.addProperty(PROP_CODE_INDOUBT_FETCH_TIMEOUT, aIndoubtFetchTimeout);
            CmProtocol.sendProperties(sContext);
            if (sContext.getError() != null) 
            {
                mWarning = Error.processServerError(mWarning, sContext.getError());
            }
            sNodeConn.getProp().setProperty(AltibaseProperties.PROP_INDOUBT_FETCH_TIMEOUT, aIndoubtFetchTimeout);
        }
    }

    public void sendIndoubtFetchMethod(short aIndoubtFetchMethod) throws SQLException
    {
        AltibaseConnection sNodeConn;
        CmProtocolContextConnect sContext; 
        
        setIndoubtFetchMethod(aIndoubtFetchMethod);

        // alter session set을 서버로 전송했으므로 metaConn은 sendProperties 불필요. nodeConn에 대해서만 처리.
        for (Connection sEach : mCachedConnections.values())
        {
            sNodeConn = (AltibaseConnection)sEach;
            sContext = sNodeConn.getContext();
            sContext.clearProperties();
            sContext.addProperty(PROP_CODE_INDOUBT_FETCH_METHOD, (byte)aIndoubtFetchMethod);
            CmProtocol.sendProperties(sContext);
            if (sContext.getError() != null) 
            {
                mWarning = Error.processServerError(mWarning, sContext.getError());
            }
            sNodeConn.getProp().setProperty(AltibaseProperties.PROP_INDOUBT_FETCH_METHOD, aIndoubtFetchMethod);
        }
    }

    public boolean getAutoCommit()
    {
        return mAutoCommit;
    }

    public void commit() throws SQLException
    {
        try
        {
            doTransaction(new TransactionCallback()
            {
                public void doTransaction(Connection aConn) throws SQLException
                {
                    shard_log("(NODE CONNECTION COMMIT)");
                    aConn.commit();
                }
            }, true);
        }
        catch (SQLException aEx)
        {
            // BUG-46790 에외 상황에서는 Disconnect 여부에 상관없이 SMN 관련 SQLWarning을 처리해야 한다.
            getNodeSqlWarnings(true, true);
            throw aEx;
        }
        finally
        {
            // BUG-46790 SMN 업데이트는 예외가 발생한 상태에서도 수행되어야 한다.
            if (shouldUpdateShardMetaNumber())
            {
                updateShardMetaNumber();
            }
        }
    }

    public void rollback() throws SQLException
    {
        try
        {
            doTransaction(new TransactionCallback()
            {
                public void doTransaction(Connection aConn) throws SQLException
                {
                    shard_log("(NODE CONNECTION ROLLBACK)");
                    aConn.rollback();
                }
            }, false);
        }
        catch (SQLException aEx)
        {
            getNodeSqlWarnings(false, true);
            throw aEx;
        }
        finally
        {
            // BUG-46790 SMN 업데이트는 예외가 발생한 상태에서도 수행되어야 한다.
            if (shouldUpdateShardMetaNumber())
            {
                updateShardMetaNumber();
            }
        }
    }

    private void doTransaction(final TransactionCallback aTransactionCallback,
                               boolean aIsCommit) throws SQLException
    {
        ShardNodeConfig sShardNodeConfig = mShardContextConnect.getShardNodeConfig();
        if (mGlobalTransactionLevel == GlobalTransactionLevel.ONE_NODE)
        {
            shard_log("(ONE NODE TRANSACTION)");
            DataNode sNode;
            if (mShardContextConnect.isNodeTransactionStarted())
            {
                sNode = mShardContextConnect.getShardOnTransactionNode();
                shard_log("(ON TRANSACTION NODE) {0}", sNode.getNodeName());
                aTransactionCallback.doTransaction(getNodeConnection(sNode));
            }
            mShardContextConnect.setShardOnTransactionNode(null);
            mShardContextConnect.setNodeTransactionStarted(false);
        }
        else if (mGlobalTransactionLevel == GlobalTransactionLevel.MULTI_NODE)
        {
            shard_log("(MULTI NODE TRANSACTION)");
            doMultiNodeTransaction(aTransactionCallback);
        }
        else
        {
            shard_log("(GLOBAL TRANSACTION)");
            doGlobalTransaction(aIsCommit, sShardNodeConfig);

            // PROJ-2733
            // commit 성공 시, 메타노드의 DistTxInfo를 초기화 해준다. 데이터노드의 DistTxInfo는 매 사용전마다 PropagateDistTxInfoToNode에 의해 overwrite 되기 때문에 초기화 불필요.
            mMetaConnection.getDistTxInfo().initDistTxInfo();
            mMetaConnection.setDistTxInfoForVerify();
        }

        getNodeSqlWarnings(aIsCommit, mShardContextConnect.needToDisconnect());
    }

    public void sendNodeTransactionBrokenReport() throws SQLException
    {
        if (mMetaConnection.isClosed())
        {
            Error.throwSQLException(ErrorDef.CLOSED_CONNECTION);
        }

        // transaction broken일 경우는 meta connection한테만 node report 보내면 됨.
        NodeConnectionReport sReport = new NodeConnectionReport();
        sReport.setNodeReportType(NodeConnectionReport.NodeReportType.SHARD_NODE_REPORT_TYPE_TRANSACTION_BROKEN);
        getShardContextConnect().setTouched(true);
            
        try
        {
            mCmShardProtocol.shardNodeReport(sReport);
        }
        catch (SQLException aEx)
        {
            AltibaseFailover.trySTF(mMetaConnection.failoverContext(), aEx);
        }
        if (mShardContextConnect.getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, mShardContextConnect.getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(this, mShardContextConnect.getError());
            }
        }
    }
    
    private void doGlobalTransaction(final boolean aIsCommit,
                                     ShardNodeConfig aShardNodeConfig) throws SQLException
    {
        int sTouchedNodeCnt = 0;
        for (DataNode sEach : aShardNodeConfig.getDataNodes())
        {
            if (sEach.isTouched())
            {
                sTouchedNodeCnt++;
            }
        }
        if (mShardContextConnect.isTouched()) // BUG-46790 touch된 노드를 카운트할때 메타노드도 추가해야 한다.
        {
            sTouchedNodeCnt++;
        }
        
        try
        {
            /* 2노드 이상이고 Commit인 경우만 global transaction을 사용한다. */
            if (sTouchedNodeCnt >= 2 && aIsCommit) 
            {
                sendShardTransactionCommitRequest(aShardNodeConfig.getTouchedNodeList());
            } 
            else 
            {
                doMultiNodeTransaction(new TransactionCallback() 
                {
                    public void doTransaction(Connection aConn) throws SQLException 
                    {
                        shard_log("(NODE CONNECTION shardTransaction)");
                        ((AltibaseConnection) aConn).shardTransaction(aIsCommit);
                    }
                });
            }
            setShard2PhaseCommitState(Shard2PhaseCommitState.SHARD_2PC_NORMAL);
        }
        catch (SQLException aEx)
        {
            setShard2PhaseCommitState(Shard2PhaseCommitState.SHARD_2PC_COMMIT_FAIL);
            throw aEx;
        }
    }

    /**
     * 데이터노드로부터 SQLWarning이 생성되었다면 조합하여 mSqlWarning에 저장한다.<br>
     * 이때 warning 메세지에 노드네임을 포함시킨다.
     * @throws SQLException 노드로부터 SQLWarning을 받는 도중 에러가 발생한 경우.
     */
    private void getNodeSqlWarnings(boolean aIsCommit, boolean aNeedToDisconnect) throws SQLException
    {
        // node name 순으로 정렬하여 SQLWarning을 저장하기 위해 TreeMap 사용
        Map<DataNode, Connection> sSortedMap = new TreeMap<DataNode, Connection>(mCachedConnections);
        for (Connection sEach : sSortedMap.values())
        {
            SQLWarning sSqlWarning = sEach.getWarnings();
            if (sSqlWarning == null) continue;
            if (sSqlWarning.getErrorCode() == ErrorDef.SHARD_META_NUMBER_INVALID)
            {
                if (aNeedToDisconnect)
                {
                    mWarning = Error.createWarning(mWarning,
                                                   ErrorDef.SHARD_SMN_OPERATION_FAILED,
                                                   "[" + ((AltibaseConnection)sEach).getNodeName() + "] The " +
                                                   ((aIsCommit) ? "Commit" : "Rollback"), sSqlWarning.getMessage());
                }
            }
            else
            {
                getSQLWarningFromDataNode(sEach);
            }
        }
    }

    private void doMultiNodeTransaction(final TransactionCallback aTransactionCallback) throws SQLException
    {
        try
        {
            /* BUG-46790 meta node는 병렬로 처리하지 않고 별도로 처리한다. 
             * 또한 node 트랜잭션 처리시 예외가 발생하더라도 수행되어야 한다.  
             */
            if (mShardContextConnect.isTouched())
            {
                aTransactionCallback.doTransaction(mMetaConnection);
                mShardContextConnect.setTouched(false);
            }
        }
        finally
        {
            mExecutorEngine.doTransaction(getTouchedNodeConnections(),
                  new ParallelProcessCallback<Connection>()
                  {
                      public Void processInParallel(Connection aConn) throws SQLException
                      {
                          try
                          {
                              aTransactionCallback.doTransaction(aConn);
                              // BUG-46790 node touch clear는 노드별로 트랜잭션이 성공했을때 따로 수행해야 한다.
                              clearNodeTouch((AltibaseConnection)aConn);
                          }
                          catch (ShardJdbcException aShardJdbcEx)
                          {
                              throw aShardJdbcEx;
                          }
                          catch (SQLException aException)
                          {
                              String sNodeName = ((AltibaseConnection)aConn).getNodeName();
                              ExecutorExceptionHandler.handleException(aException, sNodeName);
                          }
                          return null;
                      }
                  });
        }
    }

    private void clearNodeTouch(AltibaseConnection aNodeConn)
    {
        String sNodeName = aNodeConn.getNodeName();
        DataNode sNode = getShardNodeConfig().getNodeByName(sNodeName);
        sNode.setTouched(false);
    }

    private void sendShardTransactionCommitRequest(List<DataNode> aTouchedNodeList) throws SQLException
    {
        if (mMetaConnection.isClosed())
        {
            Error.throwSQLException(ErrorDef.CLOSED_CONNECTION);
        }

        if (!mAutoCommit)
        {
            try
            {
                shard_log("(SEND SHARD TRANSACTION COMMIT REQUEST) {0}", aTouchedNodeList);
                
                /* sendShardTransactionCommitRequest이  실패 한 경우
                   rollback을 수행할 때 meta connection으로도 rollback을 보내기 위해
                 */   
                mShardContextConnect.setTouched(true);  

                mCmShardProtocol.sendShardTransactionCommitRequest(aTouchedNodeList);
            }
            catch (SQLException aEx)
            {
                AltibaseFailover.trySTF(mMetaConnection.failoverContext(), aEx);
            }
            if (mShardContextConnect.getError() != null)
            {
                try
                {
                    mWarning = Error.processServerError(mWarning, mShardContextConnect.getError());
                }
                finally
                {
                    // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                    ShardError.processShardError(this, mShardContextConnect.getError());
                }
            }

            // BUG-46790 global transaction 프로토콜 전송이 성공적으로 끝나면 노드터치를 clear 해준다.
            for (DataNode sEach : getShardNodeConfig().getDataNodes())
            {
                sEach.setTouched(false);
            }
            mShardContextConnect.setTouched(false);
        }
    }

    private List<Connection> getTouchedNodeConnections()
    {
        List<DataNode> sTouchedNodeList = getShardNodeConfig().getTouchedNodeList();
        List<Connection> sResult = new ArrayList<Connection>();
        for (DataNode sEach : sTouchedNodeList)
        {
            Connection sTouchedNodeConn = mCachedConnections.get(sEach);
            if (sTouchedNodeConn != null)
            {
                sResult.add(sTouchedNodeConn);
            }
        }

        return sResult;
    }

    public void rollback(Savepoint aSavepoint) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "rollback(savepoint) is not supported in sharding");
    }

    public void close() throws SQLException
    {
        if (mIsClosed) return;
        closeAllOpenStatements();

        List<SQLException> sExceptions = new LinkedList<SQLException>();
        for (Map.Entry<DataNode, Connection> sEntry : mCachedConnections.entrySet())
        {
            try
            {
                Connection sCon = sEntry.getValue();
                shard_log("(NODE CONNECTION CLOSE) {0}", sEntry);
                sCon.close();
            }
            catch (SQLException aEx)
            {
                sExceptions.add(aEx);
            }
            mCachedConnections.remove(sEntry.getKey());
        }
        mJdbcMethodInvoker.throwSQLExceptionIfNecessary(sExceptions);
        shard_log("(META CONNECTION CLOSE) {0}", mMetaConnection);
        mMetaConnection.close();
        mIsClosed = true;
    }

    private void closeAllOpenStatements() throws SQLException
    {
        List<SQLException> sExceptions = new LinkedList<SQLException>();

        synchronized (mOpenStatements)
        {
            while (!mOpenStatements.isEmpty())
            {
                Statement sStmt = mOpenStatements.getFirst();
                try
                {
                    sStmt.close();
                }
                catch (SQLException aException)
                {
                    // BUG-46513 close 도중 error 가 발생해도 일단 다 닫고 나중에 체인으로 묶는다.
                    sExceptions.add(aException);
                }
            }
        }
        mJdbcMethodInvoker.throwSQLExceptionIfNecessary(sExceptions);
    }

    void registerStatement(AltibaseShardingStatement aStatement)
    {
        synchronized (mOpenStatements)
        {
            mOpenStatements.add(aStatement);
        }
    }

    void unregisterStatement(AltibaseShardingStatement aStatement)
    {
        synchronized (mOpenStatements)
        {
            mOpenStatements.remove(aStatement);
        }
    }

    public boolean isClosed()
    {
        return mIsClosed;
    }

    public DatabaseMetaData getMetaData() throws SQLException
    {
        // BUG-46513 굳이 node connection에서 meta data값을 가져오지 않고 바로 meta connection에서 가져온다.
        return mMetaConnection.getMetaData();
    }

    public void setReadOnly(boolean aReadOnly)
    {
        mWarning = (aReadOnly) ? Error.createWarning(mWarning,
                                                     ErrorDef.READONLY_CONNECTION_NOT_SUPPORTED) : mWarning;
    }

    public boolean isReadOnly()
    {
        return false;
    }

    public void setCatalog(String catalog)
    {
        // 아무런 동작을 하지 않는다. dbname을 바꿀 수 없다.
        mWarning = Error.createWarning(mWarning, ErrorDef.CANNOT_RENAME_DB_NAME);
    }

    public String getCatalog()
    {
        return mProps.getDatabase();
    }

    public void setTransactionIsolation(int aLevel) throws SQLException
    {
        mTransactionIsolation = aLevel;
        mJdbcMethodInvoker.recordMethodInvocation(Connection.class, "setTransactionIsolation",
                                                  new Class[] {int.class},
                                                  new Object[] {aLevel});
        for (Connection sEach : mCachedConnections.values())
        {
            sEach.setTransactionIsolation(aLevel);
        }
    }

    public int getTransactionIsolation()
    {
        return mTransactionIsolation;
    }

    public SQLWarning getWarnings()
    {
        return mWarning;
    }

    public void clearWarnings() throws SQLException
    {
        mWarning = null;
        for (Connection sEach : mCachedConnections.values())
        {
            sEach.clearWarnings();
        }
    }

    public Statement createStatement() throws SQLException
    {
        return new AltibaseShardingStatement(this);
    }

    public Statement createStatement(int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        return new AltibaseShardingStatement(this, aResultSetType, aResultSetConcurrency);
    }

    public Statement createStatement(int aResultSetType, int aResultSetConcurrency,
                                     int aResultSetHoldability) throws SQLException
    {
        return new AltibaseShardingStatement(this, aResultSetType, aResultSetConcurrency,
                                             aResultSetHoldability);
    }

    public PreparedStatement prepareStatement(String aSql) throws SQLException
    {
        return new AltibaseShardingPreparedStatement(this, aSql, mDefaultResultSetType,
                                                     mDefaultResultSetConcurrency,
                                                     mDefaultResultSetHoldability);
    }

    public PreparedStatement prepareStatement(String aSql, int aResultSetType,
                                              int aResultSetConcurrency) throws SQLException
    {
        return new AltibaseShardingPreparedStatement(this, aSql, aResultSetType, aResultSetConcurrency,
                                                     mDefaultResultSetHoldability);
    }

    public PreparedStatement prepareStatement(String aSql, int aResultSetType, int aResultSetConcurrency,
                                              int aResultSetHoldability) throws SQLException
    {
        return new AltibaseShardingPreparedStatement(this, aSql, aResultSetType, aResultSetConcurrency,
                                                     aResultSetHoldability);
    }

    public PreparedStatement prepareStatement(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        AltibaseStatement.checkAutoGeneratedKeys(aAutoGeneratedKeys);

        AltibaseShardingPreparedStatement sShardStmt =
                new AltibaseShardingPreparedStatement(this, aSql, mDefaultResultSetType,
                                                      mDefaultResultSetConcurrency,
                                                      mDefaultResultSetHoldability);
        if (aAutoGeneratedKeys == Statement.RETURN_GENERATED_KEYS)
        {
            // BUG-47168 AutoGeneratedKey 값을 가져오는 쿼리를 생성한다.
            sShardStmt.makeQstrForGeneratedKeys(aSql, null, null);
        }
        else
        {
            sShardStmt.clearForGeneratedKeys();
        }

        return sShardStmt;
    }

    public PreparedStatement prepareStatement(String aSql, int[] aColumnIndexes) throws SQLException
    {
        AltibaseShardingPreparedStatement sShardStmt =
                new AltibaseShardingPreparedStatement(this, aSql, mDefaultResultSetType,
                                                      mDefaultResultSetConcurrency,
                                                      mDefaultResultSetHoldability);
        sShardStmt.makeQstrForGeneratedKeys(aSql, aColumnIndexes, null);

        return sShardStmt;
    }

    public PreparedStatement prepareStatement(String aSql, String[] aColumnNames) throws SQLException
    {
        AltibaseShardingPreparedStatement sShardStmt =
                new AltibaseShardingPreparedStatement(this, aSql, mDefaultResultSetType,
                                                      mDefaultResultSetConcurrency,
                                                      mDefaultResultSetHoldability);
        sShardStmt.makeQstrForGeneratedKeys(aSql, null, aColumnNames);

        return sShardStmt;
    }


    public CallableStatement prepareCall(String aSql) throws SQLException
    {
        return new AltibaseShardingCallableStatement(this, aSql, mDefaultResultSetType,
                                                     mDefaultResultSetConcurrency,
                                                     mDefaultResultSetHoldability);
    }

    public CallableStatement prepareCall(String aSql, int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        return new AltibaseShardingCallableStatement(this, aSql, aResultSetType,
                                                     aResultSetConcurrency,
                                                     mDefaultResultSetHoldability);
    }

    public CallableStatement prepareCall(String aSql, int aResultSetType, int aResultSetConcurrency,
                                         int aResultSetHoldability) throws SQLException
    {
        return new AltibaseShardingCallableStatement(this, aSql, aResultSetType,
                                                     aResultSetConcurrency,
                                                     aResultSetHoldability);
    }

    public Map<String, Class<?>> getTypeMap() throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "getTypeMap is not supported");
        return null;
    }

    public void setTypeMap(Map aMap) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "User defined type");
    }

    public void setHoldability(int aHoldability)
    {
    }

    public int getHoldability()
    {
        return mDefaultResultSetHoldability;
    }

    public Savepoint setSavepoint() throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "setSavepoint is not supported in sharding");
        return null;
    }

    public Savepoint setSavepoint(String aName) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "setSavepoint is not supported in sharding");
        return null;
    }

    public void releaseSavepoint(Savepoint aSavepoint) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "releaseSavepoint is not supported in sharding");
    }

    public void setChannel(CmChannel aChannel)
    {
        mShardContextConnect.setChannel(aChannel);
        // BUG-46790 CmShardProtocol 객체에도 channel객체가 참조되기 때문에 setChannel로 초기화 해준다.
        if (mCmShardProtocol != null)
        {
            mCmShardProtocol.setChannel(aChannel);
        }
    }

    public CmShardProtocol getShardProtocol()
    {
        return mCmShardProtocol;
    }

    Connection getNodeConnection(DataNode aNode) throws SQLException
    {
        return getNodeConnection(aNode, false);
    }

    /**
     * 데이터노드의 Connection을 가져온다. 만약 해당하는 노드의 Connection이 캐시에 존재하면 바로 리턴하고 <br>
     * 아닌경우 새로 생성한다.
     * @param aNode 데이터노드 객체
     * @param aIsToAlternate alternate로의 접속 여부
     * @return 데이터노드 커넥션
     * @throws SQLException 커넥션 생성 도중 에러가 발생한 경우
     */
    public Connection getNodeConnection(DataNode aNode, boolean aIsToAlternate) throws SQLException
    {
        if (mCachedConnections.containsKey(aNode))
        {
            shard_log("(CACHED NODE CONNECTION EXISTS) {0}", aNode);
            return mCachedConnections.get(aNode);
        }
        Connection sResult = makeNodeConnection(aNode, aIsToAlternate);
        mCachedConnections.put(aNode, sResult);
        mJdbcMethodInvoker.replayMethodsInvocation(sResult);
        return sResult;
    }

    private Connection makeNodeConnection(DataNode aNode, boolean aIsToAlternate) throws SQLException
    {
        Map<DataNode, DataSource> sDataSourceMap = mShardContextConnect.getDataSourceMap();
        Connection sResult;
        try
        {
            AltibaseDataSource sDataSource = (AltibaseDataSource)sDataSourceMap.get(aNode);
            // BUG-46790 failover align시 data node 연결 수립 전이라면 alternate로 설정한다.
            if (aIsToAlternate)
            {
                String sAlternateIp = aNode.getAlternativeServerIp();
                int sAlternatePort = aNode.getAlternativePortNo();
                if (!StringUtils.isEmpty(sAlternateIp) && sAlternatePort > 0)
                {
                    sDataSource.setServerName(sAlternateIp);
                    sDataSource.setPortNumber(sAlternatePort);
                }
            }
            sResult = sDataSource.getConnection(this);
            SQLWarning sWarning = sResult.getWarnings();
            if (sWarning != null)
            {
                if (sWarning.getErrorCode() == ErrorDef.SHARD_META_NUMBER_INVALID)
                {
                    if (mShardContextConnect.needToDisconnect())
                    {
                        getSQLWarningFromDataNode(sResult);
                    }
                }
                else
                {
                    getSQLWarningFromDataNode(sResult);
                }
            }
            if (aIsToAlternate) // BUG-46790 alternate와의 연결을 수립한 후에는 primary server를 다시 맞춰준다.
            {
                setPrimaryServerInfo((AltibaseConnection)sResult, aNode);
            }
        }
        catch (SQLException aException)
        {
            // BUG-45967 노드커넥션 도중 SMN Invalid 에러가 발생한 경우에는 SQLException을 던진다.
            if (aException.getErrorCode() == ErrorDef.SHARD_META_NUMBER_INVALID)
            {
                Error.throwSQLException(ErrorDef.SHARD_SMN_OPERATION_FAILED,
                                        "The Node Connection", aException.getMessage());
            }
            throw aException;
        }

        shard_log("(DATASOURCE GETCONNECTION) {0}", aNode);

        return sResult;
    }

    /**
     * alternate server로 바로 접속했기 때문에 failover server list가 { alternate ip, alternate ip } 와 같이 구성된다.
     * primary 정보를 갱신해 { primary ip, alternate ip}와 같이 바꿔야 한다.
     * @param aNodeConn 노드 커넥션
     * @param aNode 데이터 노드 객체
     */
    private void setPrimaryServerInfo(AltibaseConnection aNodeConn, DataNode aNode)
    {
        AltibaseFailoverContext sFailoverContext = aNodeConn.failoverContext();
        AltibaseFailoverServerInfoList sFailoverServerInfoList = sFailoverContext.getFailoverServerList();
        AltibaseProperties sProps = sFailoverContext.connectionProperties();
        AltibaseFailoverServerInfo sPrimaryServerInfo = new AltibaseFailoverServerInfo(aNode.getServerIp(), aNode.getPortNo(),
                                                                                       sProps.getDatabase());
        sFailoverServerInfoList.set(0, sPrimaryServerInfo);
    }

    private void getSQLWarningFromDataNode(Connection aNodeConn) throws SQLException
    {
        SQLWarning sWarning = aNodeConn.getWarnings();
        String sNodeName = ((AltibaseConnection)aNodeConn).getNodeName();
        SQLWarning sNewWarning = new SQLWarning("[" + sNodeName + "] " + sWarning.getMessage(),
                                                sWarning.getSQLState(),
                                                sWarning.getErrorCode());
        if (mWarning == null)
        {
            mWarning = sNewWarning;
        }
        else
        {
            mWarning.setNextWarning(sNewWarning);
        }
    }

    private void setShardPinToDataNode(Connection aConnection, long aShardPin) throws SQLException
    {
        /* 관련된 인터페이스가 없기 때문에 setCatalog를 이용한다.
           ex : conn.setCatalog("shardpin:213213123123123");
         */
        aConnection.setCatalog("shardpin:" + aShardPin);
    }

    ExecutorEngine getExecutorEngine()
    {
        return mExecutorEngine;
    }

    public AltibaseConnection getMetaConnection()
    {
        return mMetaConnection;
    }

    public Map<DataNode, Connection> getCachedConnections()
    {
        return mCachedConnections;
    }

    public AltibaseProperties getProps()
    {
        return mProps;
    }

    public ShardNodeConfig getShardNodeConfig()
    {
        return mShardContextConnect.getShardNodeConfig();
    }

    /**
     * DataNode의 SMN이 클라이언트의 SMN보다 클 경우에는 DataNode의 SMN을 복사한다.
     * @param aSMN error message에서 파싱한 SMN값
     */
    public void setShardMetaNumberOfDataNode(long aSMN)
    {
        mShardContextConnect.setSMNOfDataNode(aSMN);
    }

    public void setNeedToDisconnect(boolean aNeedToDisconnect)
    {
        mShardContextConnect.setNeedToDisconnect(aNeedToDisconnect);
    }

    public boolean isLazyNodeConnect()
    {
        return mIsLazyNodeConnect;
    }

    boolean isReshardEnabled()
    {
        return mIsReshardEnabled;
    }

    /**
     * Meta가 가지고 있는 ShardMetaNumer와 DataNode의 SMN값을 비교하여 ShardMetaNumber가 DataNode SMN보다
     * 작을 경우 true를 리턴한다.
     * @return DataNode SMN이 더 최신인지 여부
     */
    public boolean shouldUpdateShardMetaNumber()
    {
        long sSMN = mShardContextConnect.getShardMetaNumber();
        long sSMNOfDataNode = mShardContextConnect.getSMNOfDataNode();
        boolean sResult = false;

        if ( ( sSMN != 0 ) &&
             ( sSMN < sSMNOfDataNode ) &&
             ( !mShardContextConnect.needToDisconnect() ) &&
             ( hasNodata()) )
        {
            sResult = true;
        }

        return sResult;
    }

    private boolean hasNodata()
    {
        synchronized (mOpenStatements)
        {
            for (Statement sEach : mOpenStatements)
            {
                AltibaseShardingStatement sStmt = (AltibaseShardingStatement)sEach;
                if (sStmt.isPrepared() && !sStmt.hasNoData())
                {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * 메타서버에 getNodeList프로토콜을 다시 요청해 SMN값을 갱신한다.
     */
    public void updateShardMetaNumber() throws SQLException
    {
        long sOldShardPin = mShardContextConnect.getShardPin();
        long sOldSMN = mShardContextConnect.getShardMetaNumber();
        mCmShardProtocol.getNodeList();
        if (mShardContextConnect.getError() != null)
        {
            mWarning = Error.processServerError(mWarning, mShardContextConnect.getError());
        }

        if (sOldShardPin != mShardContextConnect.getShardPin())
        {
            Error.throwSQLException(ErrorDef.SHARD_PIN_CHANGED);
        }

        mShardContextConnect.createDataSources(mProps);

        long sNewSMN = mShardContextConnect.getShardMetaNumber();
        if (sOldSMN < sNewSMN)
        {
            for (Connection sEach : mCachedConnections.values())
            {
                AltibaseConnection sNodeConn = (AltibaseConnection)sEach;
                sNodeConn.sendSMNProperty(sNewSMN);
            }
        }
    }

    long getShardMetaNumber()
    {
        return mShardContextConnect.getShardMetaNumber();
    }

    public void removeNode(DataNode aNode)
    {
        try
        {
            Connection sRemovedConn =  mCachedConnections.remove(aNode);
            if (sRemovedConn != null)
            {
                sRemovedConn.close();
            }
        }
        catch (SQLException aEx)
        {
            shard_log("(CLOSE NODE CONNECTION) Error occurred : {0} {1} ",
                      new Object[] { aNode, aEx.getMessage() });
        }
        /* BUG-46513 node connection을 close하더라도 해당하는 connection의 하위 statement가 statement route map
           에 존재할 수 있기 때문에 clear해준다. */
        synchronized (mOpenStatements)
        {
            for (Statement sEach : mOpenStatements)
            {
                ((AltibaseShardingStatement)sEach).removeFromStmtRouteMap(aNode);
            }
        }
    }

    private interface TransactionCallback
    {
        void doTransaction(Connection acon) throws SQLException;
    }

    public CmProtocolContextShardConnect getShardContextConnect()
    {
        return mShardContextConnect;
    }

    public AltibaseShardingFailover getShardFailover()
    {
        return mShardFailover;
    }

    public void setWarning(SQLWarning aWarning)
    {
        mWarning = aWarning;
    }

    public void registerFailoverCallback(AltibaseFailoverCallback aFailoverCallback,
                                         Object aAppContext) throws SQLException
    {
        // BUG-46790 meta connection stf를 위해 registerFailoverCallback을 등록한다.
        mMetaConnection.registerFailoverCallback(aFailoverCallback, aAppContext);
    }

    public void deregisterFailoverCallback() throws SQLException
    {
        mMetaConnection.deregisterFailoverCallback();
    }

    public void setClosed(boolean aIsClosed)
    {
        mIsClosed = aIsClosed;
    }

    void setCachedConnections(Map<DataNode, Connection> aCachedConnections)
    {
        mCachedConnections = aCachedConnections;
    }

    boolean isSessionFailoverOn()
    {
        return mProps.getBooleanProperty(AltibaseProperties.PROP_FAILOVER_USE_STF);
    }

    public GlobalTransactionLevel getGlobalTransactionLevel()
    {
        return mGlobalTransactionLevel;
    }

    public void setGlobalTransactionLevel(GlobalTransactionLevel aGlobalTransactionLevel)
    {
        mGlobalTransactionLevel = aGlobalTransactionLevel;
    }

    public short getShardStatementRetry()
    {
        return mShardStatementRetry;
    }

    public void setShardStatementRetry(short aShardStatementRetry)
    {
        mShardStatementRetry = aShardStatementRetry;
    }

    public int getIndoubtFetchTimeout()
    {
        return mIndoubtFetchTimeout;
    }

    public void setIndoubtFetchTimeout(int aIndoubtFetchTimeout)
    {
        mIndoubtFetchTimeout = aIndoubtFetchTimeout;
    }

    public short getIndoubtFetchMethod()
    {
        return mIndoubtFetchMethod;
    }

    public void setIndoubtFetchMethod(short aIndoubtFetchMethod)
    {
        mIndoubtFetchMethod = aIndoubtFetchMethod;
    }

    public void setShard2PhaseCommitState(Shard2PhaseCommitState aShard2PhaseCommitState)
    {
        mShard2PhaseCommitState = aShard2PhaseCommitState;
    }

    public Shard2PhaseCommitState getShard2PhaseCommitState()
    {
        return mShard2PhaseCommitState;
    }

    public void checkCommitState() throws SQLException
    {
        if (getShard2PhaseCommitState() == Shard2PhaseCommitState.SHARD_2PC_COMMIT_FAIL)
        {   
            Error.throwSQLException(ErrorDef.SHARD_NEED_ROLLBACK);
        }  
    }

    @Override
    public boolean isValid(int aTimeout) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setClientInfo(String aName, String aValue) throws SQLClientInfoException
    {
        throw new SQLClientInfoException("setClientInfo is not supported in sharding", null);
    }

    @Override
    public void setClientInfo(Properties aProperties) throws SQLClientInfoException
    {
        throw new SQLClientInfoException("setClientInfo is not supported in sharding", null);
    }

    @Override
    public String getClientInfo(String aName) throws SQLException
    {
        throw new SQLClientInfoException("getClientInfo is not supported in sharding", null);
    }

    @Override
    public Properties getClientInfo() throws SQLException
    {
        throw new SQLClientInfoException("getClientInfo is not supported in sharding", null);
    }

    @Override
    public void abort(Executor aExecutor) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNetworkTimeout(Executor aExecutor, int aMillisecs) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public int getNetworkTimeout() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
