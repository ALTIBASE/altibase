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

import Altibase.jdbc.driver.AltibaseConnection;
import Altibase.jdbc.driver.AltibaseStatement;
import Altibase.jdbc.driver.cm.CmProtocolContextShardConnect;
import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;
import Altibase.jdbc.driver.ex.*;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.sharding.executor.*;
import Altibase.jdbc.driver.sharding.merger.IteratorStreamResultSetMerger;
import Altibase.jdbc.driver.sharding.routing.*;

import java.sql.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class DataNodeShardingStatement implements InternalShardingStatement
{
    AltibaseShardingConnection         mMetaConn;
    CmProtocolContextShardStmt         mShardStmtCtx;
    ResultSet                          mCurrentResultSet;
    // BUG-47460 RouteResult를 별도의 객체로 처리하지 않고 ArrayList로 처리
    List<DataNode>                     mRouteResult;
    AltibaseShardingStatement          mShardStmt;
    Map<DataNode, Statement>           mRoutedStatementMap;
    int                                mResultSetType;
    int                                mResultSetConcurrency;
    int                                mResultSetHoldability;
    private SQLWarning                 mSqlwarning;
    private int                        mFetchSize;
    private StatementExecutor          mStmtExecutor;

    DataNodeShardingStatement(AltibaseShardingConnection aMetaConn,
                              int aResultSetType, int aResultSetConcurrency,
                              int aResultSetHoldability, AltibaseShardingStatement aShardStmt)
    {
        mMetaConn = aMetaConn;
        mShardStmt = aShardStmt;
        mShardStmtCtx = aShardStmt.mShardStmtCtx;
        mResultSetType = aResultSetType;
        mResultSetConcurrency = aResultSetConcurrency;
        mResultSetHoldability = aResultSetHoldability;
        mRoutedStatementMap = new ConcurrentHashMap<DataNode, Statement>();
        mStmtExecutor = new StatementExecutor(mMetaConn.getExecutorEngine());
    }

    public ResultSet executeQuery(String aSql) throws SQLException
    {
        ResultSet sResult;
        List<Statement> sStatements = null;

        try
        {
            sStatements = route(aSql);
            List<ResultSet> sResultSets = mStmtExecutor.executeQuery(sStatements);
            mShardStmtCtx.setUpdateRowcount(getNodeUpdateRowCount());
            sResult = new AltibaseShardingResultSet(sResultSets,
                                                    new IteratorStreamResultSetMerger(sResultSets),
                                                    mShardStmt);
            setOneNodeTransactionInfo(mRouteResult);
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            clearRoutedStatementMap(aShardJdbcEx);
            getNodeSqlWarnings(true);
            throw aShardJdbcEx;
        }
        catch (SQLException aEx)
        {
            processExecuteError(sStatements, aEx);
            getNodeSqlWarnings(true);
            throw aEx;
        }
        finally
        {
            mCurrentResultSet = null;
        }

        mCurrentResultSet = sResult;
        return sResult;
    }

    public int executeUpdate(String aSql) throws SQLException
    {
        int sResult;
        List<Statement> sStatementUnits = null;

        try
        {
            sStatementUnits = route(aSql);
            sResult = mStmtExecutor.executeUpdate(sStatementUnits);
            mShardStmtCtx.setUpdateRowcount(sResult);
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            clearRoutedStatementMap(aShardJdbcEx);
            getNodeSqlWarnings(true);
            throw aShardJdbcEx;
        }
        catch (SQLException aEx)
        {
            processExecuteError(sStatementUnits, aEx);
            getNodeSqlWarnings(true);
            throw aEx;
        }
        setOneNodeTransactionInfo(mRouteResult);
        getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());

        return sResult;
    }

    public void close() throws SQLException
    {
        try
        {
            // BUG-47460 node statement close를 병렬로 처리
            mMetaConn.getExecutorEngine().closeStatements(mRoutedStatementMap.values());
        }
        finally
        {
            mRoutedStatementMap.clear();
        }
    }

    public int getMaxFieldSize() throws SQLException
    {
        return mRoutedStatementMap.isEmpty() ? 0 : mRoutedStatementMap.values().iterator().next().getMaxFieldSize();
    }

    public void setMaxFieldSize(int aMax) throws SQLException
    {
        for (Statement sEach : mRoutedStatementMap.values())
        {
            sEach.setMaxFieldSize(aMax);
        }
    }

    public int getMaxRows() throws SQLException
    {
        return mRoutedStatementMap.isEmpty() ? -1 : mRoutedStatementMap.values().iterator().next().getMaxRows();
    }

    public void setMaxRows(int aMax) throws SQLException
    {
        for (Statement sEach : mRoutedStatementMap.values())
        {
            sEach.setMaxRows(aMax);
        }
    }

    public void setEscapeProcessing(boolean aEnable) throws SQLException
    {
        for (Statement sEach : mRoutedStatementMap.values())
        {
            sEach.setEscapeProcessing(aEnable);
        }
    }

    public int getQueryTimeout() throws SQLException
    {
        return mRoutedStatementMap.isEmpty() ? 0 : mRoutedStatementMap.values().iterator().next().getQueryTimeout();
    }

    public void setQueryTimeout(int aSeconds) throws SQLException
    {
        for (Statement sEach : mRoutedStatementMap.values())
        {
            sEach.setQueryTimeout(aSeconds);
        }
    }

    public void cancel() throws SQLException
    {
        for (Statement sEach : mRoutedStatementMap.values())
        {
            sEach.cancel();
        }
    }

    public SQLWarning getWarnings()
    {
        return mSqlwarning;
    }

    public void clearWarnings()
    {
        mSqlwarning = null;
    }

    public void setCursorName(String aName) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "cursor name and positioned update");
    }

    public boolean execute(String aSql) throws SQLException
    {
        boolean sResult;
        List<Statement> sStatementUnits = null;
        
        try
        {
            sStatementUnits = route(aSql);
            sResult = mStmtExecutor.execute(sStatementUnits);
            mShardStmtCtx.setUpdateRowcount(getNodeUpdateRowCount());
            setOneNodeTransactionInfo(mRouteResult);
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            /* BUGBUG : 커넥션이 끊어졌는데 sessionfailover=off인 경우 exception을 그냥 올린다... 여기서 걸러지는가??
                        partial rollback 처리 필요?
                        failover와 일반 에러 둘 다 발생하면 어떤 exception이 올라오는지 확인 필요.
            */
            clearRoutedStatementMap(aShardJdbcEx);
            getNodeSqlWarnings(true);
            throw aShardJdbcEx;
        }
        catch (SQLException aEx)
        {
            processExecuteError(sStatementUnits, aEx);
            getNodeSqlWarnings(true);
            throw aEx;
        }
        finally
        {
            mCurrentResultSet = null;
        }

        return sResult;
    }

    public ResultSet getResultSet() throws SQLException
    {
        if (mCurrentResultSet != null)
        {
            return mCurrentResultSet;
        }

        // BUG-47360 prepare후 execute를 안하는 경우 route result가 없기때문에 이런 경우 예외를 올린다.
        if (mRouteResult == null)
        {
            Error.throwSQLException(ErrorDef.STATEMENT_NOT_YET_EXECUTED);
        }

        List<ResultSet> sResultSets = new ArrayList<ResultSet>(mRouteResult.size());

        // BUG-47360 mRouteResult로부터 route된 노드를 찾아 Statement.getResultSet()을 수행한다.
        for (DataNode sEach : mRouteResult)
        {
            ResultSet sRs = mRoutedStatementMap.get(sEach).getResultSet();
            if (sRs != null) // BUG-47360 ResultSet이 생성된 경우에만 sResultSets에 add한다.
            {
                sResultSets.add(sRs);
            }
        }

        if (sResultSets.size() == 1)
        {
            mCurrentResultSet = sResultSets.get(0);
        }
        else if (sResultSets.size() > 1)
        {
            // BUG-47360 node로부터 생성된 ResultSet이 2개 이상인 경우 AltibaseShardingResultSet을 생성한다.
            mCurrentResultSet = new AltibaseShardingResultSet(sResultSets,
                                                              new IteratorStreamResultSetMerger(sResultSets),
                                                              mShardStmt);
        }

        return mCurrentResultSet;
    }

    public int getUpdateCount()
    {
        int sResult = mShardStmtCtx.getUpdateRowcount();
        // BUG-47338 getUpdateCount가 한번 호출된 뒤엔 -1 값으로 초기화 해야 한다.
        mShardStmtCtx.setUpdateRowcount(AltibaseStatement.DEFAULT_UPDATE_COUNT);

        return sResult;
    }

    public boolean getMoreResults() throws SQLException
    {
        return getMoreResultsInternal(new GetMoreResultsTemplate()
        {
            public boolean getMoreResults(Statement aStmt) throws SQLException
            {
                return aStmt.getMoreResults();
            }
        });
    }

    public boolean getMoreResults(final int aCurrent) throws SQLException
    {
        return getMoreResultsInternal(new GetMoreResultsTemplate()
        {
            public boolean getMoreResults(Statement aStmt) throws SQLException
            {
                return aStmt.getMoreResults(aCurrent);
            }
        });
    }

    private boolean getMoreResultsInternal(GetMoreResultsTemplate aTemplate) throws SQLException
    {
        boolean sResult = true;
        int sNoDataCount = 0;

        for (DataNode sEach : mRouteResult)
        {
            Statement sStmt = mRoutedStatementMap.get(sEach);
            if (!aTemplate.getMoreResults(sStmt))
            {
                sNoDataCount++;
            }
        }
        /* BUG-47360 node statement 전체가 getMoreResults에 실패한 경우에만 결과를 false로 올린다.
           즉 한 노드라도 getMoreResults가 성공하면 true를 리턴한다. */
        if (sNoDataCount == mRouteResult.size())
        {
            sResult = false;
        }

        // BUG-47360 getMoreResults가 호출된 경우 다음 resultset을 가리켜야 하기때문에 mCurrentResultSet을 초기화 해준다.
        mCurrentResultSet = null;

        return sResult;
    }

    public void setFetchSize(int aRows) throws SQLException
    {
        mFetchSize = aRows;
        for (Statement sEach : mRoutedStatementMap.values())
        {
            sEach.setFetchSize(aRows);
        }
    }

    public int getFetchSize()
    {
        return mFetchSize;
    }

    public void clearBatch() throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "clearBatch is not supported in sharding");
    }

    public int[] executeBatch() throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "executeBatch is not supported in sharding");
        return null;
    }

    public Connection getConnection()
    {
        return mMetaConn;
    }

    public ResultSet getGeneratedKeys() throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "AutoGeneratedKey is not supported in client-side sharding");
        return null;
    }

    public int executeUpdate(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "AutoGeneratedKey is not supported in client-side sharding");
        return 0;
    }

    public int executeUpdate(String aSql, int[] aColumnIndexes) throws SQLException
    {
        int sResult;
        List<Statement> sStatementUnits = null;

        try
        {
            sStatementUnits = route(aSql);
            sResult =  mStmtExecutor.executeUpdate(aColumnIndexes, sStatementUnits);
            mShardStmtCtx.setUpdateRowcount(sResult);
            setOneNodeTransactionInfo(mRouteResult);
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            clearRoutedStatementMap(aShardJdbcEx);
            getNodeSqlWarnings(true);
            throw aShardJdbcEx;
        }
        catch (SQLException aEx)
        {
            processExecuteError(sStatementUnits, aEx);
            getNodeSqlWarnings(true);
            throw aEx;
        }
        finally
        {
            mCurrentResultSet = null;
        }

        return sResult;
    }

    public int executeUpdate(String aSql, String[] aColumnNames) throws SQLException
    {
        int sResult;
        List<Statement> sStatementUnits = null;

        try
        {
            sStatementUnits = route(aSql);
            sResult = mStmtExecutor.executeUpdate(aColumnNames, sStatementUnits);
            mShardStmtCtx.setUpdateRowcount(sResult);
            setOneNodeTransactionInfo(mRouteResult);
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());
        }
        catch (ShardJdbcException aShardJdbcEx)
        {
            clearRoutedStatementMap(aShardJdbcEx);
            getNodeSqlWarnings(true);
            throw aShardJdbcEx;
        }
        catch (SQLException aEx)
        {
            processExecuteError(sStatementUnits, aEx);
            getNodeSqlWarnings(true);
            throw aEx;
        }
        finally
        {
            mCurrentResultSet = null;
        }

        return sResult;
    }

    public boolean execute(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "AutoGeneratedKey is not supported in client-side sharding");
        return false;
    }

    public boolean execute(String aSql, int[] aColumnIndexes) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "AutoGeneratedKey is not supported in client-side sharding");
        return false;
    }

    public boolean execute(String aSql, String[] aColumnNames) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "AutoGeneratedKey is not supported in client-side sharding");
        return false;
    }

    private List<Statement> route(final String aSql) throws SQLException
    {
        mCurrentResultSet = null;
        mRouteResult = createRoutingEngine().route(aSql, null);
        ExecutorEngine sExecutorEngine = mMetaConn.getExecutorEngine();

        // PROJ-2690 ExecutorEngine을 통해 병렬로 Statement 객체 생성
        List<Statement> sResults = sExecutorEngine.generateStatement(
                                mRouteResult,
                                new GenerateCallback<Statement>()
                                {
                                    public Statement generate(DataNode aNode) throws SQLException
                                    {
                                        Connection sConn = mMetaConn.getNodeConnection(aNode);
                                        Statement sStatement = getStatement(aNode, sConn);
                                        ((AltibaseStatement)sStatement).setSql(aSql);
                                        mRoutedStatementMap.put(aNode, sStatement);
                                        return sStatement;
                                    }
                                });
        // BUG-47145 node statement가 정상적으로 생성된 다음 node touch를 수행한다.
        if (!mShardStmtCtx.isAutoCommitMode())
        {
            touchNodes();
        }

        calcDistTxInfo(sResults);

        return sResults;
    }
    
    void calcDistTxInfo(List<Statement> aResults)
    {
        AltibaseStatement sStatement;
        short sExecuteNodeCnt = (short)aResults.size();
        // CalcDistTxInfoForDataNode에서 sExecuteNodeCnt=1일때만 DistTxInfo.mBeforeExecutedNodeId과 sNodeIndex를 비교하므로, sNodeIndex에 첫번째 노드의 id만 저장하면 됨.
        // 참고로 일반 execute일 때는 mRouteResult에 처리할 모든 노드가 저장되지만, batch의 경우 마지막 addBatch의 route 결과만 저장되어 있음.
        // addBatch 때마다 route 결과가 mRouteResult에 overwrite됨.
        short sNodeIndex = (short)mRouteResult.get(0).getNodeId();
        mMetaConn.getMetaConnection().getDistTxInfo().calcDistTxInfoForDataNode(mMetaConn, sExecuteNodeCnt, sNodeIndex);

        for (int i = 0; i < sExecuteNodeCnt; i++)
        {
            sStatement = (AltibaseStatement)aResults.get(i);
            sStatement.getProtocolContext().getDistTxInfo().propagateDistTxInfoToNode(mMetaConn.getMetaConnection().getDistTxInfo());
        }

        mMetaConn.getMetaConnection().setDistTxInfoForVerify();
    }

    private Statement getStatement(DataNode aNode, Connection aConn) throws SQLException
    {
        Statement sStatement = mRoutedStatementMap.get(aNode);
        if (sStatement != null) return sStatement;
        
        // routed statemap에 해당 statement가 없다면 새로 생성한다.
        sStatement = aConn.createStatement(mResultSetType, mResultSetConcurrency, mResultSetHoldability);
        mShardStmt.setShardMetaNumber(mMetaConn.getShardMetaNumber());
        mShardStmt.replayMethodsInvocation(sStatement);
        
        return sStatement;
    }

    RoutingEngine createRoutingEngine()
    {
        int sShardValueCnt = mShardStmtCtx.getShardAnalyzeResult().getShardValueCount();
        int sShardSubValueCnt = mShardStmtCtx.getShardAnalyzeResult().getShardSubValueCount();
        ShardSplitMethod sSplitMethod = mShardStmtCtx.getShardAnalyzeResult().getShardSplitMethod();
        switch (sSplitMethod)
        {
            case CLONE:
                /* shard value가 없는 경우는 random하게 노드를 선택해야 하며 그렇지 않은
                   경우에는 모든 노드가 실행대상이 된다. */
                return (sShardValueCnt == 0) ? new RandomNodeRoutingEngine(mShardStmtCtx) :
                       new AllNodesRoutingEngine(mShardStmtCtx);
            case SOLO:
                return new AllNodesRoutingEngine(mShardStmtCtx);
            case LIST:
            case RANGE:
            case HASH:
                if (sShardValueCnt >= 0 && sShardSubValueCnt > 0)
                {
                    return new CompositeShardKeyRoutingEngine(mShardStmtCtx);
                }
                else if (sShardValueCnt > 0 && sShardSubValueCnt == 0)
                {
                    return new SimpleShardKeyRoutingEngine(mShardStmtCtx);
                }
                else  // shard value 가 없다면 모든 노드가 수행 대상이 된다.
                {
                    return new AllNodesRoutingEngine(mShardStmtCtx);
                }
        }

        return null;
    }

    /**
     * 트랜잭션 모드일때 노드의 touch 플래그를 셋팅한다. 또한 이때 onenode transaction인 경우에는 <br>
     * 트잰잭션이 유효한지 체크한다.
     * @throws SQLException onenode일때 정상적인 트랜잭션 상태가 아닌 경우
     */
    void touchNodes() throws SQLException
    {
        if (mMetaConn.getGlobalTransactionLevel() == GlobalTransactionLevel.ONE_NODE)
        {
            /* one node tx에서 1개이상 노드가 선택된 경우 에러 */
            if (mRouteResult.size() > 1)
            {
                Error.throwSQLException(ErrorDef.SHARD_MULTINODE_TRANSACTION_REQUIRED);
            }
            // one node tx에서 이전 노드와 다르면 에러
            DataNode sShardOnTransactionNode = mShardStmtCtx.getShardContextConnect().getShardOnTransactionNode();
            DataNode sCurrentNode = mRouteResult.get(0);
            if (!isValidTransaction(sShardOnTransactionNode, sCurrentNode))
            {
                Error.throwSQLException(ErrorDef.SHARD_INVALID_NODE_TOUCH);
            }
        }

        // touch nodes
        for (DataNode sEach : mRouteResult)
        {
            sEach.setTouched(true);
        }
    }

    private boolean isValidTransaction(DataNode sOldNode, DataNode sCurrentNode)
    {
        return !mShardStmtCtx.getShardContextConnect().isNodeTransactionStarted() ||
               sOldNode.equals(sCurrentNode);
    }

    void setOneNodeTransactionInfo(List<DataNode> aSqlExecutionUnits)
    {
        if (aSqlExecutionUnits.size() == 0) return;

        if (mMetaConn.getGlobalTransactionLevel() == GlobalTransactionLevel.ONE_NODE)
        {
            DataNode sNode = aSqlExecutionUnits.iterator().next();
            CmProtocolContextShardConnect sShardContextConnect = mShardStmtCtx.getShardContextConnect();
            sShardContextConnect.setShardOnTransactionNode(sNode);
            sShardContextConnect.setNodeTransactionStarted(true);
        }
    }

    void setShardStmtCtx(CmProtocolContextShardStmt aShardStmtCtx)
    {
        mShardStmtCtx = aShardStmtCtx;
    }

    /**
     * ShardRetryAvailableException이나 STF success 상황인 경우에는 statement가 유효하지 않기 때문에 <br>
     * 해당 statement를 routedStatementMap에서 clear한다.
     * @param aShardJdbcEx ShardJdbcException 객체
     */
    void clearRoutedStatementMap(ShardJdbcException aShardJdbcEx)
    {
        ShardNodeConfig sNodeConfig = mMetaConn.getShardNodeConfig();
        String sNodeName = aShardJdbcEx.getNodeName();
        mRoutedStatementMap.remove(sNodeConfig.getNodeByName(sNodeName));
    }

    /**
     * RoutedStatementMap을 순환하면서 SqlWarning이 있으면 조합한다.
     * @throws SQLException 노드 Statement에서 SqlWarning을 가져오는 도중 에러가 발생한 경우
     */
    void getNodeSqlWarnings(boolean aNeedToDisconnect) throws SQLException
    {
        boolean sWarningExists = false;
        for (Statement sEach : mRoutedStatementMap.values())
        {
            AltibaseStatement sStmt = (AltibaseStatement)sEach;
            if (!sStmt.isClosed() && sEach.getWarnings() != null)
            {
                sWarningExists = true;
                break;
            }
        }
        if (!sWarningExists) return;

        // 순서대로 SQLWarning을 저장하기 위해 TreeMap을 사용한다.
        Map<DataNode, Statement> sSortedMap = new TreeMap<DataNode, Statement>(mRoutedStatementMap);
        for (Map.Entry<DataNode, Statement> sEntry : sSortedMap.entrySet())
        {
            Statement sStmt = sEntry.getValue();
            SQLWarning sSqlWarning = sStmt.getWarnings();
            if (sSqlWarning == null) continue;
            if (sSqlWarning.getErrorCode() == ErrorDef.SHARD_META_NUMBER_INVALID )
            {
                if (aNeedToDisconnect)
                {
                    mSqlwarning = Error.createWarning(mSqlwarning,
                                                      ErrorDef.SHARD_SMN_OPERATION_FAILED,
                                                      "[" + sEntry.getKey().getNodeName() + "] The " + "Execute",
                                                      sSqlWarning.getMessage());
                }
            }
            else
            {
                getSQLWarningFromDataNode(sEntry.getKey().getNodeName(), sSqlWarning);
            }
        }
    }

    private void getSQLWarningFromDataNode(String aNodeName, SQLWarning aWarning)
    {
        SQLWarning sNewWarning = new SQLWarning("[" + aNodeName + "] " + aWarning.getMessage(),
                                                aWarning.getSQLState(), aWarning.getErrorCode());
        if (mSqlwarning == null)
        {
            mSqlwarning = sNewWarning;
        }
        else
        {
            mSqlwarning.setNextWarning(sNewWarning);
        }

    }

    public boolean isPrepared()
    {
        for (Statement sEach : mRoutedStatementMap.values())
        {
            if (((AltibaseStatement)sEach).isPrepared())
            {
                return true;
            }
        }

        return false;
    }

    public boolean hasNoData()
    {
        for (Statement sEach : mRoutedStatementMap.values())
        {
            AltibaseStatement sStmt = (AltibaseStatement)sEach;
            if (sStmt.isPrepared() && !sStmt.cursorhasNoData())
            {
                return false;
            }
        }

        return true;
    }

    void removeFromStmtRouteMap(DataNode aNode)
    {
        // BUG-46513 connection이 정리될때 Statement Free를 수행하므로 여기서는 메핑정보만 정리한다.
        mRoutedStatementMap.remove(aNode);
    }

    int getNodeUpdateRowCount()
    {
        long sResult = 0;
        boolean sHasResult = false;
        for (Statement sEach : mRoutedStatementMap.values())
        {
            int sUpdatedCount = 0;
            try
            {
                sUpdatedCount = sEach.getUpdateCount();
            }
            catch (SQLException aEx)
            {
                /* BUG-46513 lazyConnect 가 false일때는 statement가 아직 실행전일수 있기때문에 해당 exception이
                   발생하면 로그를 남기고 넘어간다. */
                if (aEx.getErrorCode() == ErrorDef.STATEMENT_NOT_YET_EXECUTED)
                {
                    shard_log("Statement not yet executed : {0}", sEach);
                    continue;
                }
            }
            if (sUpdatedCount > -1)
            {
                sHasResult = true;
                sResult += sUpdatedCount;
            }
        }
        if (sResult > Integer.MAX_VALUE)
        {
            sResult = Integer.MAX_VALUE;
        }

        return sHasResult ? Long.valueOf(sResult).intValue() : -1;
    }

    public void makeQstrForGeneratedKeys(String aSql, int[] aColumnIndexes,
                                         String[] aColumnNames) throws SQLException
    {
        // BUG-47168 클라이언트 사이드일때는 AutoGenerated Key를 지원하지 않는다.
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "AutoGeneratedKey is not supported in client-side sharding.");
    }

    public void clearForGeneratedKeys() throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                "AutoGeneratedKey is not supported in client-side sharding.");
    }

    void processExecuteError(List<Statement> aStatementUnits, SQLException aEx) throws SQLException
    {
        // BUG-46785 : 에러가 하나라도 있으면 partial rollback 처리

        // one node 수행시에는 partial rollback 불필요.
        if (aStatementUnits == null || aStatementUnits.size() < 2)
        {
            return;
        }
        
        List<SQLException> sExceptionList = new ArrayList<SQLException>();
        List<SQLWarning> sWarningList = new ArrayList<SQLWarning>();

        sExceptionList.add(aEx);
        // BUGBUG : sWarningList.add(e) 필요한가?
        
        // select for update의 경우 partial rollback 전에 close cursor 수행
        try
        {
            // BUGBUG : select여부를 먼저 체크하여 select일때만 closeCursor 호출?
            // AltibaseStatement sStmt0 = (AltibaseStatement)aStatementUnits.get(0);
            mMetaConn.getExecutorEngine().closeCursor(aStatementUnits, 
                    new ParallelProcessCallback<Statement>()
                    {
                        public Void processInParallel(Statement aStmt) throws SQLException 
                        {
                            ((AltibaseStatement)aStmt).closeCursor();
                            return null;
                        }
                    });

            mMetaConn.getExecutorEngine().doPartialRollback(aStatementUnits,
                    new ParallelProcessCallback<Statement>()
                    {
                          public Void processInParallel(Statement aStmt) throws SQLException
                          {
                              if (((AltibaseStatement) aStmt).getIsSuccess())  // 성공인 노드만 partial rollback 수행.
                              {
                                  AltibaseConnection sConn = (AltibaseConnection) aStmt.getConnection();
                                  sConn.shardStmtPartialRollback();
                              }
                              return null;
                          }
                    });
        }
        catch (ShardJdbcException sShardJdbcEx1)
        {
            // BUGBUG : aShardJdbcEx.next 안해봐도 되나?
            sExceptionList.add(sShardJdbcEx1);
            clearRoutedStatementMap(sShardJdbcEx1);
            getNodeSqlWarnings(true);
            ShardError.throwSQLExceptionIfExists(sExceptionList);
        }
        catch (SQLException sEx1)
        {
            sExceptionList.add(sEx1);
            
            try
            {
                mMetaConn.sendNodeTransactionBrokenReport();
            }
            catch (ShardJdbcException sShardJdbcEx2)
            {
                clearRoutedStatementMap(sShardJdbcEx2);
                getNodeSqlWarnings(true);
                throw sShardJdbcEx2;
            }
            catch (SQLException sEx2)
            {
                sExceptionList.add(Error.createSQLException(ErrorDef.SHARD_INTERNAL_ERROR,
                                   "ShardNodeReport(Transaction Broken) fails."));
            }
            
            ShardError.throwSQLExceptionIfExists(sExceptionList);
        }
    }

    private interface GetMoreResultsTemplate
    {
        boolean getMoreResults(Statement aStmt) throws SQLException;
    }
}
