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

import Altibase.jdbc.driver.AltibasePreparedStatement;
import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.ex.ShardJdbcException;
import Altibase.jdbc.driver.sharding.executor.*;
import Altibase.jdbc.driver.sharding.merger.IteratorStreamResultSetMerger;
import Altibase.jdbc.driver.sharding.routing.*;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.sql.*;
import java.sql.Date;
import java.util.*;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class DataNodeShardingPreparedStatement extends DataNodeShardingStatement implements InternalShardingPreparedStatement
{
    RoutingEngine                                     mRoutingEngine;
    String                                            mSql;
    List<Column>                                      mParameters;
    private int                                       mBatchCount;
    private Map<DataNode, BatchPreparedStatementUnit> mBatchStmtUnitMap;
    private PreparedStatementExecutor                 mPreparedStmtExecutor;
    private List<Statement>                           mPStmtUnit;

    DataNodeShardingPreparedStatement(AltibaseShardingConnection aShardCon, String aSql, int aResultSetType,
                                      int aResultSetConcurrency, int aResultSetHoldability,
                                      AltibaseShardingPreparedStatement aShardStmt) throws SQLException
    {
        super(aShardCon, aResultSetType, aResultSetConcurrency, aResultSetHoldability, aShardStmt);
        mSql = aSql;
        mBatchStmtUnitMap = new LinkedHashMap<DataNode, BatchPreparedStatementUnit>();
        mRoutingEngine = createRoutingEngine();
        mPreparedStmtExecutor = new PreparedStatementExecutor(mMetaConn.getExecutorEngine());
        // BUG-47145 shard_lazy_connect가 false일때는 바로 range의 전 노드에 prepare를 수행한다.
        if (!aShardCon.isLazyNodeConnect())
        {
            prepareNodeDirect(aShardCon);
        }
        mPStmtUnit = new ArrayList<Statement>();   // BUG-47460 route된 노드의 Statement들이 저장된다.
        // BUG-46513 Meta 커넥션에 있는 SMN값을 셋팅한다.
        mShardStmt.setShardMetaNumber(mMetaConn.getShardMetaNumber());
        mParameters = aShardStmt.getParameters();
    }

    private void prepareNodeDirect(AltibaseShardingConnection aShardCon) throws SQLException
    {
        ShardRangeList sRangeList = mShardStmtCtx.getShardAnalyzeResult().getShardRangeList();
        Map<DataNode, Connection> sNodeConnMap = aShardCon.getCachedConnections();
        shard_log("[RANGELIST_INFO] {0}", sRangeList.getRangeList());

        // BUG-47145 range에 해당하는 전 노드를 loop돌면서 SQLExecutionUnit 셋을 구성한다.
        List<DataNode> sNodes = new ArrayList<DataNode>();
        for (ShardRange sEach : sRangeList.getRangeList())
        {
            DataNode sNode = sEach.getNode();
            if (!sNodes.contains(sNode))
            {
                sNodes.add(sEach.getNode());
            }
        }

        prepareNodeStatementsForNonLazyMode(sNodes);

        boolean sDefaultNodeIncluded = false;
        int sDefaultNodeId = mShardStmtCtx.getShardAnalyzeResult().getShardDefaultNodeID();
        if (sDefaultNodeId >= 0)
        {
            for (ShardRange sEach : sRangeList.getRangeList())
            {
                if (sEach.getNode().getNodeId() == sDefaultNodeId)
                {
                    sDefaultNodeIncluded = true;
                    break;
                }
            }
            if (!sDefaultNodeIncluded)
            {
                DataNode sDefaultNode = mMetaConn.getShardNodeConfig().getNode(sDefaultNodeId);
                Connection sNodeConn = sNodeConnMap.get(sDefaultNode);
                PreparedStatement sPstmt = sNodeConn.prepareStatement(mSql, mResultSetType,
                                                                      mResultSetConcurrency,
                                                                      mResultSetHoldability);
                mRoutedStatementMap.put(sDefaultNode, sPstmt);
            }
        }
    }

    /**
     * shard_lazy_connect가 false일때 range의 범위만큼 병렬로 데이터노드의 PreparedStatement를 생성한다.
     *
     * @param aNodes 실행할 노드 리스트
     * @throws SQLException 노드 Statement 생성시 에러가 발생한 경우
     */
    private void prepareNodeStatementsForNonLazyMode(List<DataNode> aNodes) throws SQLException
    {
        // BUG-47145 lazy mode가 false일때도 병렬로 PreparedStatement를 생성한다.
        ExecutorEngine sExecutorEngine = mMetaConn.getExecutorEngine();
        sExecutorEngine.generateStatement(aNodes,
                          new GenerateCallback<PreparedStatement>()
                          {
                              public PreparedStatement generate(DataNode aNode) throws SQLException
                              {
                                  Connection sNodeCon = mMetaConn.getNodeConnection(aNode);
                                  PreparedStatement sStmt = sNodeCon.prepareStatement(mSql, mResultSetType,
                                                                    mResultSetConcurrency,
                                                                    mResultSetHoldability);
                                  shard_log("(NODE PREPARE) {0}", sStmt);
                                  // BUG-46513 Meta 커넥션에 있는 SMN값을 셋팅한다.
                                  mShardStmt.setShardMetaNumber(mMetaConn.getShardMetaNumber());
                                  mRoutedStatementMap.put(aNode, sStmt);
                                  return sStmt;
                              }
                          });

    }

    public ResultSet executeQuery() throws SQLException
    {
        List<Statement> sStatements = null;
        
        try
        {
            ResultSet sResult = null;
            sStatements = route();
            if (!mShardStmtCtx.isAutoCommitMode())
            {
                touchNodes();
            }
            List<ResultSet> sResultSets = mPreparedStmtExecutor.executeQuery(sStatements);

            // BUG-47460 ResultSet이 1개일 경우에는 따로 AltibaseShardingResultSet을 만들지 않고 바로 AltibaseResultSet을 리턴한다.
            if (sResultSets.size() == 1)
            {
                sResult = sResultSets.get(0);
            }
            else if (sResultSets.size() > 1)
            {
                sResult = new AltibaseShardingResultSet(sResultSets, new IteratorStreamResultSetMerger(sResultSets), mShardStmt);
            }
            // BUG-46513 update된 row count를 Statement context에 저장한다.
            mShardStmtCtx.setUpdateRowcount(getNodeUpdateRowCount());
            setOneNodeTransactionInfo(mRouteResult);
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());
            mCurrentResultSet = sResult;
            return sResult;
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
    }

    public int executeUpdate() throws SQLException
    {
        List<Statement> sStatements = null;
        
        try
        {
            sStatements = route();
            if (!mShardStmtCtx.isAutoCommitMode())
            {
                touchNodes();
            }

            int sUpdateCnt = mPreparedStmtExecutor.executeUpdate(sStatements);
            // BUG-46513 update된 row count를 Statement context에 저장한다.
            mShardStmtCtx.setUpdateRowcount(sUpdateCnt);
            setOneNodeTransactionInfo(mRouteResult);
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());

            return sUpdateCnt;
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
    }

    public int[] executeBatch() throws SQLException
    {
        List<Statement> sStatements = new ArrayList<Statement>();

        for (BatchPreparedStatementUnit sEach : mBatchStmtUnitMap.values())
        {
            sStatements.add(sEach.getStatement());
        }

        calcDistTxInfo(sStatements);

        try
        {
            int[] sResult = new BatchPreparedStatementExecutor(mMetaConn.getExecutorEngine(),
                                                               mBatchStmtUnitMap, mBatchCount).executeBatch();
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());

            return sResult;
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
            clearBatch();
        }
    }


    public void clearParameters() throws SQLException
    {
        mCurrentResultSet = null;

        for (Statement sEach : mRoutedStatementMap.values())
        {
            ((PreparedStatement)sEach).clearParameters();
        }
    }

    public boolean execute() throws SQLException
    {
        List<Statement> sStatements = null;
        
        try
        {
            sStatements = route();
            if (!mShardStmtCtx.isAutoCommitMode())
            {
                touchNodes();
            }

            boolean sResult = mPreparedStmtExecutor.execute(sStatements);
            // BUG-46513 update된 row count를 Statement context에 저장한다.
            mShardStmtCtx.setUpdateRowcount(getNodeUpdateRowCount());
            setOneNodeTransactionInfo(mRouteResult);
            getNodeSqlWarnings(mMetaConn.getShardContextConnect().needToDisconnect());

            return sResult;
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
    }

    public void addBatch() throws SQLException
    {
        try
        {
            for (BatchPreparedStatementUnit sEach : routeBatch())
            {
                sEach.getStatement().addBatch();
                sEach.mapAddBatchCount(mBatchCount);
            }
            if (!mShardStmtCtx.isAutoCommitMode())
            {
                touchNodes();
            }
            mBatchCount++;
        }
        finally
        {
            mCurrentResultSet = null;
        }
    }

    public ResultSetMetaData getMetaData() throws SQLException
    {
        route();

        if (mRoutedStatementMap.size() > 0)
        {
            return ((PreparedStatement)mRoutedStatementMap.values().iterator().next()).getMetaData();
        }

        return null;
    }

    public ParameterMetaData getParameterMetaData() throws SQLException
    {
        route();

        if (mRoutedStatementMap.size() > 0)
        {
            return ((PreparedStatement)mRoutedStatementMap.values().iterator().next()).getParameterMetaData();
        }

        return null;
    }

    public void clearBatch() throws SQLException
    {
        mCurrentResultSet = null;
        clearParameters();
        mBatchStmtUnitMap.clear();
        mBatchCount = 0;
    }

    public void rePrepare(CmProtocolContextShardStmt aShardStmtCtx) throws SQLException
    {
        mShardStmtCtx = aShardStmtCtx;
        mRoutingEngine = createRoutingEngine();

        /* BUG-47357 lazy node connect 모드가 아닐때는 prepare가 먼저 일어나기 때문에 reprepare요청을 보내야 한다.
           반면에 lazy mode에서는 prepare가 나중에 일어나기 때문에 reprepare를 할 필요가 없다. */
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                AltibasePreparedStatement sPstmt = (AltibasePreparedStatement)sEach;
                sPstmt.prepare(sPstmt.getSql());
            }
        }
    }

    public void setInt(int aParameterIndex, int aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setInt(aParameterIndex, aValue);
            }
        }
    }

    public void setNull(int aParameterIndex, int aSqlType) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setNull(aParameterIndex, aSqlType);
            }
        }
    }

    public void setNull(int aParameterIndex, int aSqlType, String aTypeName) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setNull(aParameterIndex, aSqlType, aTypeName);
            }
        }
    }

    public void setBoolean(int aParameterIndex, boolean aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setBoolean(aParameterIndex, aValue);
            }
        }
    }

    public void setByte(int aParameterIndex, byte aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setByte(aParameterIndex, aValue);
            }
        }
    }

    public void setShort(int aParameterIndex, short aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setShort(aParameterIndex, aValue);
            }
        }
    }

    public void setLong(int aParameterIndex, long aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setLong(aParameterIndex, aValue);
            }
        }
    }

    public void setFloat(int aParameterIndex, float aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setFloat(aParameterIndex, aValue);
            }
        }
    }

    public void setDouble(int aParameterIndex, double aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setDouble(aParameterIndex, aValue);
            }
        }
    }

    public void setBigDecimal(int aParameterIndex, BigDecimal aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setBigDecimal(aParameterIndex, aValue);
            }
        }
    }

    public void setString(int aParameterIndex, String aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setString(aParameterIndex, aValue);
            }
        }
    }

    public void setBytes(int aParameterIndex, byte[] aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setBytes(aParameterIndex, aValue);
            }
        }
    }

    public void setDate(int aParameterIndex, Date aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setDate(aParameterIndex, aValue);
            }
        }
    }

    public void setDate(int aParameterIndex, Date aValue, Calendar aCal) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setDate(aParameterIndex, aValue, aCal);
            }
        }
    }

    public void setTime(int aParameterIndex, Time aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setTime(aParameterIndex, aValue);
            }
        }
    }

    public void setTime(int aParameterIndex, Time aValue, Calendar aCal) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setTime(aParameterIndex, aValue, aCal);
            }
        }
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setTimestamp(aParameterIndex, aValue);
            }
        }
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue, Calendar aCal) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setTimestamp(aParameterIndex, aValue, aCal);
            }
        }
    }

    public void setAsciiStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setAsciiStream(aParameterIndex, aValue, aLength);
            }
        }
    }

    public void setBinaryStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setBinaryStream(aParameterIndex, aValue, aLength);
            }
        }
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType, int aScale) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((AltibasePreparedStatement)sEach).setObject(aParameterIndex, aValue, aTargetSqlType, aScale);
            }
        }
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setObject(aParameterIndex, aValue, aTargetSqlType);
            }
        }
    }

    public void setCharacterStream(int aParameterIndex, Reader aReader, int aLength) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setCharacterStream(aParameterIndex, aReader, aLength);
            }
        }
    }

    public void setBlob(int aParameterIndex, Blob aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setBlob(aParameterIndex, aValue);
            }
        }
    }

    public void setClob(int aParameterIndex, Clob aValue) throws SQLException
    {
        if (!mMetaConn.isLazyNodeConnect())
        {
            for (Statement sEach : mRoutedStatementMap.values())
            {
                ((PreparedStatement)sEach).setClob(aParameterIndex, aValue);
            }
        }
    }

    /**
     * 샤드키값을 기준으로 해당하는 노드들의 PreparedStatement객체를 리턴한다.
     * @return PreparedStatement 객체
     */
    protected List<Statement> route() throws SQLException
    {
        mCurrentResultSet = null;
        mRouteResult = mRoutingEngine.route(mSql, mParameters);

        // BUG-47460 lazy 모드일때만 병렬로 statement를 생성한다.
        ExecutorEngine sExecutorEngine = mMetaConn.getExecutorEngine();

        if (mMetaConn.isLazyNodeConnect())
        {
            mPStmtUnit = sExecutorEngine.generateStatement(mRouteResult, new GenerateCallback<Statement>()
            {
                public Statement generate(DataNode aNode) throws SQLException
                {
                    Connection sNodeCon = mMetaConn.getNodeConnection(aNode);
                    PreparedStatement sStmt = getNodeStatement(aNode, sNodeCon);
                    mRoutedStatementMap.put(aNode, sStmt);
                    return sStmt;
                }
            });
        }

        mPStmtUnit.clear();
        for (DataNode sEach : mRouteResult)
        {
            Statement sStmt = mRoutedStatementMap.get(sEach);
            // BUG-47460 lazy connect mode이거나 reshard가 활성화 되어 있는 경우에만 setXXX메소드를 replay한다.
            if (mMetaConn.isLazyNodeConnect() || mMetaConn.isReshardEnabled())
            {
                mShardStmt.replaySetParameter((PreparedStatement)sStmt);
                mShardStmt.replayMethodsInvocation(sStmt);
            }
            mPStmtUnit.add(sStmt);
        }

        // BUG-47096 lob 관련 operation은 여러개의 노드들에 대해 수행될 수 없기 때문에 예외를 올려야 한다.
        if (mPStmtUnit.size() > 1 && isMultipleLobLocatorExist(mPStmtUnit))
        {
            Error.throwSQLException(ErrorDef.SHARD_MULTIPLE_LOB_OPERATION_NOT_SUPPORTED);
        }
        
        calcDistTxInfo(mPStmtUnit);

        return mPStmtUnit;
    }

    /**
     * route된 PreparedStatementUnit을 순환하면서 lob locator가 존재하는지 확인한다.
     * @param aPreparedStmtUnits PreparedStatementUnit 객체리스트
     * @return true - lob locator가 생성된 node statement가 2개 이상일때 <br>
     *         false - lob locator가 생성된 node statement가 1개 이거나 또는 0일때
     */
    private boolean isMultipleLobLocatorExist(List<Statement> aPreparedStmtUnits)
    {
        int sLobStmtCnt = 0;
        for (Statement sEach : aPreparedStmtUnits)
        {
            AltibasePreparedStatement sStmt = (AltibasePreparedStatement)sEach;
            if (sStmt.getLobUpdator() != null)
            {
                sLobStmtCnt++;
                if (sLobStmtCnt > 1)
                {
                    return true;
                }
            }
        }

        return false;
    }

    private List<BatchPreparedStatementUnit> routeBatch() throws SQLException
    {
        List<BatchPreparedStatementUnit> sResult = new ArrayList<BatchPreparedStatementUnit>();
        mRouteResult = mRoutingEngine.route(mSql, mParameters);
        for (DataNode sEach : mRouteResult)
        {
            BatchPreparedStatementUnit sBatchStatementUnit = getPreparedBatchStatement(sEach);
            mShardStmt.replaySetParameter(sBatchStatementUnit.getStatement());
            mShardStmt.replayMethodsInvocation(sBatchStatementUnit.getStatement());
            sResult.add(sBatchStatementUnit);
        }

        return sResult;
    }

    private BatchPreparedStatementUnit getPreparedBatchStatement(DataNode aNode) throws SQLException
    {
        BatchPreparedStatementUnit sBatchStmtUnit = mBatchStmtUnitMap.get(aNode);
        if (sBatchStmtUnit == null)
        {
            Connection sNodeCon = mMetaConn.getNodeConnection(aNode);
            PreparedStatement sStmt = getNodeStatement(aNode, sNodeCon);
            // BUG-47406 addBatch시 Statement를 새로 생성한 경우 mRoutedStatementMap에 put을 해준다.
            mRoutedStatementMap.put(aNode, sStmt);
            sBatchStmtUnit = new BatchPreparedStatementUnit(aNode, sStmt, mParameters);
            mBatchStmtUnitMap.put(aNode, sBatchStmtUnit);
        }

        return sBatchStmtUnit;
    }

    PreparedStatement getNodeStatement(DataNode aNode, Connection aConn) throws SQLException
    {
        PreparedStatement sStmt = (PreparedStatement)mRoutedStatementMap.get(aNode);
        if (sStmt != null) return sStmt;

        sStmt = aConn.prepareStatement(mSql, mResultSetType, mResultSetConcurrency, mResultSetHoldability);
        shard_log("(NODE PREPARE) {0}", sStmt);
        // BUG-46513 Meta 커넥션에 있는 SMN값을 셋팅한다.
        mShardStmt.setShardMetaNumber(mMetaConn.getShardMetaNumber());
        return sStmt;
    }
}
