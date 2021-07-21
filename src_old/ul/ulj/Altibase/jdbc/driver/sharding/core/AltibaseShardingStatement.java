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

import Altibase.jdbc.driver.*;
import Altibase.jdbc.driver.cm.CmPrepareResult;
import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.*;
import java.util.Map;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class AltibaseShardingStatement extends WrapperAdapter implements Statement
{
    CmProtocolContextShardStmt                 mShardStmtCtx;
    SQLWarning                                 mSqlwarning;
    AltibaseShardingConnection                 mMetaConn;
    String                                     mSql;
    Map<Integer, JdbcMethodInvocation>         mSetParamInvocationMap;
    InternalShardingStatement                  mInternalStmt;
    Statement                                  mServerSideStmt;
    AltibaseStatement                          mStatementForAnalyze; // BUG-47274 analyze를 위한 내부 statement
    long                                       mShardMetaNumber;
    int                                        mResultSetType;
    int                                        mResultSetConcurrency;
    int                                        mResultSetHoldability;
    boolean                                    mIsCoordQuery;
    boolean                                    mIsReshardEnabled;
    private static int                         DEFAULT_CURSOR_HOLDABILITY = ResultSet.CLOSE_CURSORS_AT_COMMIT;
    private Class<? extends Statement>         mTargetClass;
    protected final JdbcMethodInvoker          mJdbcMethodInvoker = new JdbcMethodInvoker();

    AltibaseShardingStatement(AltibaseShardingConnection aConn) throws SQLException
    {
        this(aConn, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY,
             DEFAULT_CURSOR_HOLDABILITY);
    }

    AltibaseShardingStatement(AltibaseShardingConnection aConn, int aResultSetType,
                              int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        mMetaConn = aConn;
        mResultSetType = aResultSetType;
        mResultSetConcurrency = aResultSetConcurrency;
        mResultSetHoldability = aResultSetHoldability;
        mIsReshardEnabled = aConn.isReshardEnabled();
        // BUG-47274 Analyze용 statement를 따로 생성한다.
        mStatementForAnalyze = new AltibaseStatement(aConn.getMetaConnection(),
                                                     aResultSetType,
                                                     aResultSetConcurrency,
                                                     aResultSetHoldability);
        mShardStmtCtx = new CmProtocolContextShardStmt(aConn, mStatementForAnalyze);
        mTargetClass = Statement.class;
        if (aResultSetType == ResultSet.TYPE_SCROLL_SENSITIVE ||
                aResultSetType == ResultSet.TYPE_SCROLL_INSENSITIVE)
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE,
                                    "scrollable resultset is not supported in sharding.");
        }
        aConn.registerStatement(this);
    }

    AltibaseShardingStatement(AltibaseShardingConnection aConn, int aResultSetType,
                              int aResultSetConcurrency) throws SQLException
    {
        this(aConn, aResultSetType, aResultSetConcurrency, DEFAULT_CURSOR_HOLDABILITY);
    }

    public ResultSet executeQuery(String aSql) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        ResultSet sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();

        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.executeQuery(aSql);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }
        clearClosedResultSets(sResult);

        return sResult;
    }

    public int executeUpdate(String aSql) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        int sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();

        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.executeUpdate(aSql);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public void close() throws SQLException
    {
        mMetaConn.unregisterStatement(this);

        SQLException sEx = null;
        try
        {
            // BUG-47274 analyze를 위해 생성한 statement를 해제한다.
            mStatementForAnalyze.close();
        }
        catch (SQLException aEx)
        {
            sEx = aEx;
        }

        try
        {
            if (mInternalStmt != null)
            {
                mInternalStmt.close();
            }
        }
        catch (SQLException aEx)
        {
            if (sEx == null)
            {
                sEx = aEx;
            }
            else // BUG-47274 이미 analyze용 statement에서 예외가 발생한 경우 chain으로 묶는다.
            {
                sEx.setNextException(aEx);
            }
        }
        if (sEx != null)
        {
            throw sEx;
        }
    }

    public int getMaxFieldSize() throws SQLException
    {
        return (mInternalStmt == null) ? 0 : mInternalStmt.getMaxFieldSize();
    }

    public void setMaxFieldSize(int aMax) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(mTargetClass,
                                                  "setMaxFieldSize",
                                                  new Class[] { int.class },
                                                  new Object[] { aMax });
        if (mInternalStmt != null)
        {
            mInternalStmt.setMaxFieldSize(aMax);
        }
    }

    public int getMaxRows() throws SQLException
    {
        return (mInternalStmt == null) ? 0 : mInternalStmt.getMaxRows();
    }

    public void setMaxRows(int aMax) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(mTargetClass,
                                                  "setMaxRows",
                                                  new Class[] {int.class},
                                                  new Object[] {aMax});
        if (mInternalStmt != null)
        {
            mInternalStmt.setMaxRows(aMax);
        }
    }

    public void setEscapeProcessing(boolean aEnable) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(mTargetClass,
                                                  "setEscapeProcessing",
                                                  new Class[] { boolean.class },
                                                  new Object[] { aEnable });
        if (mInternalStmt != null)
        {
            mInternalStmt.setEscapeProcessing(aEnable);
        }
    }

    public int getQueryTimeout() throws SQLException
    {
        return (mInternalStmt == null) ? 0 : mInternalStmt.getQueryTimeout();
    }

    public void setQueryTimeout(int aSeconds) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(mTargetClass,
                                                  "setQueryTimeout",
                                                  new Class[] { int.class },
                                                  new Object[] { aSeconds });
        if (mInternalStmt != null)
        {
            mInternalStmt.setQueryTimeout(aSeconds);
        }
    }

    public void cancel() throws SQLException
    {
        if (mInternalStmt != null)
        {
            mInternalStmt.cancel();
        }
    }

    public SQLWarning getWarnings() throws SQLException
    {
        if (mInternalStmt != null)
        {
            if (mSqlwarning == null)
            {
                mSqlwarning = mInternalStmt.getWarnings();
            }
            else
            {
                mSqlwarning.setNextWarning(mInternalStmt.getWarnings());
            }
        }

        return mSqlwarning;
    }

    public void clearWarnings() throws SQLException
    {
        if (mInternalStmt != null)
        {
            mInternalStmt.clearWarnings();
        }

        mSqlwarning = null;
    }

    public void setCursorName(String aName) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "cursor name and positioned update");
    }

    public boolean execute(String aSql) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        boolean sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();
        
        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.execute(aSql);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    protected void checkStmtTooOld(SQLException aEx, short aRetryCount) throws SQLException
    {
        SQLException sEx;
        
        if (mInternalStmt instanceof ServerSideShardingStatement ||
            !mMetaConn.getGlobalTransactionLevel().equals(GlobalTransactionLevel.GCTX) ||
            aRetryCount == 0)
        {
            throw aEx;
        }

        // 모든 노드에서 넘어온 exception 중에 STATEMENT_TOO_OLD가 아닌 에러가 있으면 throw aEx
        // 즉, 모든 exception이 STATEMENT_TOO_OLD 일때만 stmt retry 수행
        sEx = aEx;
        while(sEx != null)
        {
            if (sEx.getErrorCode() != ErrorDef.STATEMENT_TOO_OLD)
            {
                throw aEx;
            }
            sEx = sEx.getNextException();
        }
    }
    
    public ResultSet getResultSet() throws SQLException
    {
        /* BUG-47127 아직 analyze 전 이라면 Statement Not Yet Execute예외를 올려야 한다.  */
        if (mInternalStmt == null)
        {
            Error.throwSQLException(ErrorDef.STATEMENT_NOT_YET_EXECUTED);
        }

        return  mInternalStmt.getResultSet();
    }

    public int getUpdateCount() throws SQLException
    {
        return (mInternalStmt == null) ? AltibaseStatement.DEFAULT_UPDATE_COUNT :
                                         mInternalStmt.getUpdateCount();
    }

    public boolean getMoreResults() throws SQLException
    {
        if (mInternalStmt == null)
        {
            return false;
        }
        boolean sResult = mInternalStmt.getMoreResults();

        if (mMetaConn.getAutoCommit() && !sResult && mMetaConn.shouldUpdateShardMetaNumber())
        {
            mMetaConn.updateShardMetaNumber();
        }

        return sResult;
    }

    public void setFetchDirection(int aDirection) throws SQLException
    {
        AltibaseResultSet.checkFetchDirection(aDirection);
    }

    public int getFetchDirection()
    {
        return ResultSet.FETCH_FORWARD;
    }

    public void setFetchSize(int aRows) throws SQLException
    {
        mJdbcMethodInvoker.recordMethodInvocation(mTargetClass,
                                                  "setFetchSize",
                                                  new Class[] {int.class},
                                                  new Object[] {aRows});
        if (mInternalStmt != null)
        {
            mInternalStmt.setFetchSize(aRows);
        }
    }

    public int getFetchSize() throws SQLException
    {
        return (mInternalStmt == null) ? 0 : mInternalStmt.getFetchSize();
    }

    public int getResultSetConcurrency()
    {
        return mResultSetConcurrency;
    }

    public int getResultSetType()
    {
        return mResultSetType;
    }

    public void addBatch(String aSql) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "addBatch is not supported in sharding");
    }

    public void clearBatch() throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "clearBatch is not supported in sharding");
    }

    public int[] executeBatch() throws SQLException
    {
        return null;
    }

    public Connection getConnection()
    {
        // BUG-47127 internal statement까지 갈 필요없이 바로 AltibaseShardingConnection을 리턴한다.
        return mMetaConn;
    }

    public boolean getMoreResults(int aCurrent) throws SQLException
    {
        if (mInternalStmt != null)
        {
            return mInternalStmt.getMoreResults(aCurrent);
        }

        return false;
    }

    public ResultSet getGeneratedKeys() throws SQLException
    {
        return (mInternalStmt == null) ? null : mInternalStmt.getGeneratedKeys();
    }

    public int executeUpdate(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        int sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();
        
        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.executeUpdate(aSql, aAutoGeneratedKeys);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public int executeUpdate(String aSql, int[] aColumnIndexes) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        int sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();
        
        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.executeUpdate(aSql, aColumnIndexes);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public int executeUpdate(String aSql, String[] aColumnNames) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        int sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();
        
        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.executeUpdate(aSql, aColumnNames);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public boolean execute(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        boolean sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();
        
        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.execute(aSql, aAutoGeneratedKeys);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    /**
     * 오토커밋 ON 상태이고 DataNode의 SMN 값이 더 높다면 SMN값을 업데이트 한다.
     * @throws SQLException shard meta number 업데이트 중 에러가 발생했을 때
     */
    void updateShardMetaNumberIfNecessary() throws SQLException
    {
        // BUG-47460 resharding이 활성화 되어 있을 때만 meta numer update를 수행한다.
        if (mIsReshardEnabled)
        {
            if (mMetaConn.getAutoCommit() && mMetaConn.shouldUpdateShardMetaNumber())
            {
                mMetaConn.updateShardMetaNumber();
            }
        }
    }

    void clearClosedResultSets(ResultSet aResultSet)
    {
        if (mIsReshardEnabled)
        {
            if (aResultSet instanceof AltibaseShardingResultSet)
            {
                AltibaseShardingResultSet sShardingResultSet = (AltibaseShardingResultSet)aResultSet;
                sShardingResultSet.clearClosedResultSets();
            }
        }
    }

    public boolean execute(String aSql, int[] aColumnIndexes) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        boolean sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();
        
        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.execute(aSql, aColumnIndexes);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public boolean execute(String aSql, String[] aColumnNames) throws SQLException
    {
        mMetaConn.checkCommitState();
        shardAnalyze(aSql);
        boolean sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();
        
        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalStmt.execute(aSql, aColumnNames);
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public int getResultSetHoldability()
    {
        return mResultSetHoldability;
    }

    private void shardAnalyze(String aSql) throws SQLException
    {
        CmPrepareResult sPrepareResult = mShardStmtCtx.getShardPrepareResult();
        // BUG-47274 기존에 있던 prepare result를 재활용한다.
        mShardStmtCtx = new CmProtocolContextShardStmt(mMetaConn, mStatementForAnalyze, sPrepareResult);

        try
        {
            mMetaConn.getShardProtocol().shardAnalyze(mShardStmtCtx, aSql, mStatementForAnalyze.getID());
        }
        catch (SQLException aEx)
        {
            AltibaseFailover.trySTF(mMetaConn.getMetaConnection().failoverContext(), aEx);
        }

        if (mShardStmtCtx.getShardAnalyzeResult().isCoordQuery())
        {
            // BUG-46513 ONENODE이고 analyze 결과가 성공이 아닐경우에는 예외를 던진다.
            if (mShardStmtCtx.getGlobalTransactionLevel() == GlobalTransactionLevel.ONE_NODE)
            {
                mSqlwarning = Error.processServerError(mSqlwarning, mShardStmtCtx.getError());
            }
            Connection sConn = mMetaConn.getMetaConnection();
            // BUG-46790 이미 생성한 serverside statement가 있는 경우에는 재사용한다.
            if (mServerSideStmt == null)
            {
                mServerSideStmt = sConn.createStatement(mResultSetType, mResultSetConcurrency,
                                                        mResultSetHoldability);
            }
            mIsCoordQuery = true;
            // BUG-46513 서버사이드일때는 meta connection을 AltibaseStatement에 주입한다.
            ((AltibaseStatement)mServerSideStmt).setMetaConnection(mMetaConn);
            mInternalStmt = new ServerSideShardingStatement(mServerSideStmt, mMetaConn);
        }
        else
        {
            mInternalStmt = new DataNodeShardingStatement(mMetaConn,
                                                          mResultSetType, mResultSetConcurrency,
                                                          mResultSetHoldability,
                                                          this);
            ((DataNodeShardingStatement)mInternalStmt).setShardStmtCtx(mShardStmtCtx);
        }
    }

    public boolean isPrepared()
    {
        if (mInternalStmt != null)
        {
            return mInternalStmt.isPrepared();
        }

        return false;
    }

    public boolean hasNoData()
    {
        if (mInternalStmt != null)
        {
            return mInternalStmt.hasNoData();
        }

        return false;
    }

    public AltibaseShardingConnection getMetaConn()
    {
        return mMetaConn;
    }

    public void setSql(String aSql)
    {
        mSql = aSql;
    }

    public void setShardMetaNumber(long aShardMetaNumber)
    {
        mShardMetaNumber = aShardMetaNumber;
    }

    public void removeFromStmtRouteMap(DataNode aNode)
    {
        if (!mIsCoordQuery)
        {
            ((DataNodeShardingStatement)mInternalStmt).removeFromStmtRouteMap(aNode);
        }
    }

    void replaySetParameter(PreparedStatement aPreparedStatement) throws SQLException
    {
        for (JdbcMethodInvocation sEach : mSetParamInvocationMap.values())
        {
            sEach.invoke(aPreparedStatement);
            shard_log("(REPLAY SET PARAMETER) {0}", sEach);
        }
    }

    void makeQstrForGeneratedKeys(String aSql, int[] aColumnIndexes,
                                  String[] aColumnNames) throws SQLException
    {
        mInternalStmt.makeQstrForGeneratedKeys(aSql, aColumnIndexes, aColumnNames);
    }

    void clearForGeneratedKeys() throws SQLException
    {
        mInternalStmt.clearForGeneratedKeys();
    }

    public void replayMethodsInvocation(Object aObj) throws SQLException
    {
        mJdbcMethodInvoker.replayMethodsInvocation(aObj);
    }

    @Override
    public boolean isClosed() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void closeOnCompletion() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public boolean isCloseOnCompletion() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    /* BUG-48892 AbstractStatement.java를 삭제하고 4.2 인터페이스인 setPoolable(), isPoolabe()을 직접 구현 */
    @Override
    public void setPoolable(boolean aPoolable) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public boolean isPoolable() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
