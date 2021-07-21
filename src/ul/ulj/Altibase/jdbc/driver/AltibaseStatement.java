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
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Pattern;

import Altibase.jdbc.driver.cm.*;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.ColumnFactory;
import Altibase.jdbc.driver.datatype.ColumnInfo;
import Altibase.jdbc.driver.datatype.IntegerColumn;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.ex.ShardError;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.sharding.core.*;
import Altibase.jdbc.driver.util.AltiSqlProcessor;
import Altibase.jdbc.driver.util.AltibaseProperties;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.StringUtils;
import Altibase.jdbc.driver.sharding.core.GlobalTransactionLevel;

import static Altibase.jdbc.driver.sharding.core.AltibaseShardingFailover.*;

class ExecuteResult
{
    boolean mHasResultSet;
    boolean mReturned;
    long mUpdatedCount;
    
    ExecuteResult(boolean aHasResultSet, long aUpdatedCount)
    {
        mHasResultSet = aHasResultSet;
        mUpdatedCount = aUpdatedCount;
        mReturned = false;
    }
}

public class AltibaseStatement extends WrapperAdapter implements Statement
{
    static final int                   DEFAULT_CURSOR_HOLDABILITY = ResultSet.CLOSE_CURSORS_AT_COMMIT;
    public static final int            DEFAULT_UPDATE_COUNT = -1;
    // BUG-42424 ColumnInfo에서 BYTES_PER_CHAR를 사용하기 때문에 public으로 변경
    public static final int            BYTES_PER_CHAR             = 2;

    protected AltibaseConnection       mConnection;
    protected boolean                  mEscapeProcessing          = true;
    protected ExecuteResultManager     mExecuteResultMgr = new ExecuteResultManager();
    protected short                    mCurrentResultIndex;
    protected AltibaseResultSet        mCurrentResultSet          = null;
    protected List<ResultSet>          mResultSetList;
    protected boolean                  mIsClosed;
    protected SQLWarning               mWarning                   = null;
    /* BUG-37642 Improve performance to fetch */
    protected int                      mFetchSize                 = 0;
    protected int                      mMaxFieldSize;
    protected long                     mMaxRows;
    protected CmPrepareResult          mPrepareResult;
    protected List<Column>             mPrepareResultColumns;
    protected CmExecutionResult        mExecutionResult;
    protected CmFetchResult            mFetchResult;
    protected int                      mStmtCID;
    protected boolean                  mIsDeferred;        // BUG-42424 deferred prepare
    protected String                   mQstr;
    protected boolean                  mReUseResultSet;      // BUG-48380 ResultSet을 재사용할지 여부
    protected boolean                  mResultSetReturned;   // BUG-48380 execute 결과로 ResultSet이 생성되었는지 여부
    private String                     mQstrForGeneratedKeys;
    private CmProtocolContextDirExec   mContext;
    private LinkedList                 mBatchSqlList;
    private final int                  mResultSetType;
    private final int                  mResultSetConcurrency;
    private final int                  mResultSetHoldability;
    protected int                      mTargetResultSetType;
    protected int                      mTargetResultSetConcurrency;
    private AltibaseStatement          mInternalStatement;
    private boolean                    mIsInternalStatement; // PROJ-2625
    private SemiAsyncPrefetch          mSemiAsyncPrefetch;   // PROJ-2625
    private AltibaseResultSet          mGeneratedKeyResultSet;
    private int                        mQueryTimeout;
    private final AltibaseResultSet    mEmptyResultSet;
    private transient Logger           mLogger;
    private AltibaseShardingConnection mMetaConn;
    private boolean                    mCloseOnCompletion;
    // BUG-48892 스펙에 따라 Statement pool을 직접 구현하고 있지 않더라도 flag 값을 셋팅할 수 있도록 선언
    private boolean                    mIsPoolable;
    private boolean                    mIsSuccess = true;    // BUG-48762 sharding statementAC partial rollback 지원

    // BUG-49143 ping query 판별용 정규표현식 패턴
    private final Pattern              mPingQueryPattern = Pattern.compile("^\\s*/\\*\\s+(?i)ping\\s+\\*/\\s+(?i)select\\s+1\\s*$");

    protected class ExecuteResultManager
    {
        private LinkedList mExecuteResults;
        
        private ExecuteResultManager() {}
        
        protected ExecuteResult get(int aIdx) throws SQLException
        {
            throwErrorForStatementNotYetExecuted();
            return (ExecuteResult)mExecuteResults.get(aIdx);
        }
        
        protected void clear() throws SQLException
        {
            if(mExecuteResults != null)
            {
                mExecuteResults.clear();
            }
        }

        protected void add(ExecuteResult aResult)
        {
            if(mExecuteResults == null)
            {
                mExecuteResults = new LinkedList();
            }
            
            mExecuteResults.add(aResult);
        }

        protected int size()
        {
            if(mExecuteResults == null)
            {
                return 0;
            }
            else
            {
                return mExecuteResults.size();
            }
        }

        protected ExecuteResult getFirst() throws SQLException
        {
            throwErrorForStatementNotYetExecuted();
            return (ExecuteResult)mExecuteResults.getFirst();
        }

        public Iterator iterator() throws SQLException
        {
            throwErrorForStatementNotYetExecuted();
            return mExecuteResults.iterator();
        }

        private void throwErrorForStatementNotYetExecuted() throws SQLException
        {
            if (mExecuteResults == null)
            {
                Error.throwSQLException(ErrorDef.STATEMENT_NOT_YET_EXECUTED);
            }
        }
    }
    
    AltibaseStatement(AltibaseConnection aCon) throws SQLException
    {
        // internal statement를 위한 생성자.
        this(aCon, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, DEFAULT_CURSOR_HOLDABILITY);
    }

    public AltibaseStatement(AltibaseConnection aCon, int aResultSetType, int aResultSetConcurrency,
                             int aResultSetHoldability) throws SQLException
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
        }

        AltibaseResultSet.checkAttributes(aResultSetType, aResultSetConcurrency, aResultSetHoldability);

        mConnection = aCon;
        mMaxFieldSize = 0;
        mMaxRows = 0;
        mFetchSize = downgradeFetchSize(aCon.getProperties().getFetchEnough());
        createFetchSizeWarning(aCon.getProperties().getFetchEnough());
        mIsDeferred = mConnection.isDeferredPrepare();    // BUG-42424 deferred prepare

        /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
        if (mFetchSize < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "fetch_enough",
                                    AltibaseProperties.RANGE_FETCH_ENOUGH,
                                    String.valueOf(mFetchSize));
        }

        mCurrentResultIndex = 0;
        mResultSetList = new ArrayList<ResultSet>();
        mIsClosed = false;
        mStmtCID = mConnection.makeStatementCID();
        createProtocolContext();
        mTargetResultSetType = mResultSetType = aResultSetType;
        mTargetResultSetConcurrency = mResultSetConcurrency = aResultSetConcurrency;
        mResultSetHoldability = aResultSetHoldability;

        // generated keys를 위한 빈 ResultSet
        IntegerColumn sColumn = ColumnFactory.createIntegerColumn();
        ColumnInfo sInfo = new ColumnInfo();
        sColumn.getDefaultColumnInfo(sInfo);
        sColumn.setColumnInfo(sInfo);
        List sColumnList = new ArrayList();
        sColumnList.add(sColumn);
        mEmptyResultSet = new AltibaseEmptyResultSet(this, sColumnList, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY);
        mGeneratedKeyResultSet = mEmptyResultSet;
    }

    int getCID()
    {
        return mStmtCID;
    }

    public int getID()
    {
        return (mPrepareResult == null) ? 0 : mPrepareResult.getStatementId();
    }

    public String getSql()
    {
        return mQstr;
    }

    public void setSql(String aSql)
    {
        mQstr = aSql;
    }
    
    protected void createProtocolContext()
    {
        mContext = new CmProtocolContextDirExec(mConnection.channel());
        mContext.setDistTxInfo(mConnection.getDistTxInfo());
    }
    
    public CmProtocolContextDirExec getProtocolContext()
    {
        return mContext;
    }
    
    protected void afterExecution() throws SQLException
    {
        if (getProtocolContext().getError() != null)
        {
            CmErrorResult sErrorResult = getProtocolContext().getError();
            try
            {
                mWarning = Error.processServerError(mWarning, sErrorResult);
            }
            catch (SQLException aEx)
            {
                mIsSuccess = false;  // For partial rollback
                /* BUG-46790 direct execute일 경우 SQLException이 발생하면 prepare결과를 초기화 한다.
                   그렇지 않으면 Failure to find statement 오류가 발생할 수 있다. */
                if (!(this instanceof PreparedStatement))
                {
                    mPrepareResult = null;
                }
                throw aEx;
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(mConnection.getMetaConnection(), sErrorResult);
            }
        }
        mPrepareResult = getProtocolContext().getPrepareResult();
        if(getProtocolContext().getPrepareResult().getResultSetCount() > 1) 
        {
            mPrepareResultColumns = getProtocolContext().getGetColumnInfoResult().getColumns();
        }
        
        if(mPrepareResultColumns == null)
        {
            mPrepareResultColumns = getProtocolContext().getGetColumnInfoResult().getColumns();
        }
        
        mExecutionResult = getProtocolContext().getExecutionResult();
        mFetchResult = getProtocolContext().getFetchResult();
        
        setProperty4Nodes();
    }
    
    private void setProperty4Nodes()  throws SQLException
    {
        // BUGBUG : alter session set global_transaction_level ~ && shardjdbc인 경우
        if (mMetaConn == null)
        {
            return;
        }
        
        // BUGBUG : CmGetPropertyResult를 사용하는게 더 나을 지 확인해보기...
        short sPropID = mExecutionResult.getSessionPropID();
        String sPropValue = mExecutionResult.getSessionPropValueStr();
        
        // Node Connections에 대해 sendProperties 전송
        switch (sPropID)
        {
            case (AltibaseProperties.PROP_CODE_GLOBAL_TRANSACTION_LEVEL):
                mMetaConn.sendGlobalTransactionLevel(GlobalTransactionLevel.get(Short.parseShort(sPropValue)));
                break;
            case (AltibaseProperties.PROP_CODE_SHARD_STATEMENT_RETRY):
                mMetaConn.sendShardStatementRetry(Short.parseShort(sPropValue));
                break;
            case (AltibaseProperties.PROP_CODE_INDOUBT_FETCH_TIMEOUT):
                mMetaConn.sendIndoubtFetchTimeout(Integer.parseInt(sPropValue));
                break;
            case (AltibaseProperties.PROP_CODE_INDOUBT_FETCH_METHOD):
                mMetaConn.sendIndoubtFetchMethod(Short.parseShort(sPropValue));
                break;
            default:
                break;
        }
    }

    protected void clearAllResults() throws SQLException
    {
        // PROJ-2427 closeAllCursor 프로토콜을 별도로 보내지 않고 execute, executeQuery에서 한꺼번에 보낸다.
        mExecuteResultMgr.clear();
        mResultSetList.clear();

        // BUG-48380 ResultSet을 재활용할때는 mCurrentResultSet을 null로 초기화 해서는 안된다.
        if (!mReUseResultSet)
        {
            mCurrentResultSet = null;
        }

        if (mGeneratedKeyResultSet != mEmptyResultSet)
        {
            mGeneratedKeyResultSet.close();
            mGeneratedKeyResultSet = mEmptyResultSet;
        }
    }

    /**
     * Generated Keys를 얻기위한 쿼리문을 만든다.
     * <p>
     * column indexes와 column names는 동시에 사용하지 않는다.
     * 만약, 둘을 모두 넘겼다면 column indexes만 사용하고 column names 값은 무시한다.
     * <p>
     * 만약 원본 쿼리가 Generated Keys를 얻기에 적합하지 않다면, 조용히 넘어간다.
     * 
     * @param aSql 원본 쿼리문. 반드시 INSERT 구문이어야 한다.
     * @param aColumnIndexes an array of the indexes of the columns in the inserted row that should be made available for retrieval by a call to the method {@link #getGeneratedKeys()}
     * @param aColumnNames an array of the names of the columns in the inserted row that should be made available for retrieval by a call to the method {@link #getGeneratedKeys()}
     */
    public void makeQstrForGeneratedKeys(String aSql, int[] aColumnIndexes, String[] aColumnNames) throws SQLException
    {
        mQstrForGeneratedKeys = null;

        if (!AltiSqlProcessor.isInsertQuery(aSql))
        {
            return; // INSERT 쿼리여야 한다.
        }

        ArrayList sSeqs = AltiSqlProcessor.getAllSequences(aSql);
        if (sSeqs.size() == 0)
        {
            // BUG-39571 시퀀스가 없는 경우 SQLException을 발생시키지 않고 그냥 리턴시킨다.
            return;
        }

        if (aColumnIndexes != null)
        {
            mQstrForGeneratedKeys = AltiSqlProcessor.makeGenerateKeysSql(sSeqs, aColumnIndexes);
        }
        else if (aColumnNames != null)
        {
            mQstrForGeneratedKeys = AltiSqlProcessor.makeGenerateKeysSql(sSeqs, aColumnNames);
        }
        else
        {
            mQstrForGeneratedKeys = AltiSqlProcessor.makeGenerateKeysSql(sSeqs);
        }
    }

    public void clearForGeneratedKeys()
    {
        mQstrForGeneratedKeys = null;
    }

    /**
     * Generated Keys를 얻기 위해 준비한 쿼리문이 있으면 수행한다.
     * <p>
     * 결과는 {@link #getGeneratedKeys()}를 통해 얻을 수 있다.
     * 
     * @throws SQLException 쿼리 수행에 실패한 경우
     */
    void executeForGeneratedKeys() throws SQLException
    {
        if (mQstrForGeneratedKeys == null)
        {
            mGeneratedKeyResultSet = mEmptyResultSet;
            return;
        }

        if (mInternalStatement == null)
        {
            mInternalStatement = AltibaseStatement.createInternalStatement(mConnection);
        }
        mGeneratedKeyResultSet = (AltibaseResultSet)mInternalStatement.executeQuery(mQstrForGeneratedKeys);
    }

    protected boolean processExecutionResult() throws SQLException
    {
        for (int i=0; i<mPrepareResult.getResultSetCount(); i++)
        {
            mExecuteResultMgr.add(new ExecuteResult(true, DEFAULT_UPDATE_COUNT));
        }
        
        if (mExecutionResult.getUpdatedRowCount() > 0)
        {
            mExecuteResultMgr.add(new ExecuteResult(false, mExecutionResult.getUpdatedRowCount()));
        }
        else
        {
            if (mExecuteResultMgr.size() == 0)
            {
                // 결과가 하나도 없을 경우 update count = 0을 세팅한다.
                mExecuteResultMgr.add(new ExecuteResult(false,  0));
            }
        }
        
        mCurrentResultIndex = 0;
        
        return mExecuteResultMgr.getFirst().mHasResultSet;
    }

    protected ResultSet processExecutionQueryResult(String aSql) throws SQLException
    {
        throwErrorForResultSetCount(aSql);
        ExecuteResult sExecResult = new ExecuteResult(true, DEFAULT_UPDATE_COUNT);
        mExecuteResultMgr.add(sExecResult);

        mCurrentResultSet = AltibaseResultSet.createResultSet(this, mTargetResultSetType, mTargetResultSetConcurrency);
        // BUG-47639 lob_null_select jdbc 속성값을 AltibaseConnection 객체로부터 받아온다.
        mCurrentResultSet.setAllowLobNullSelect(mConnection.getAllowLobNullSelect());
        mResultSetList.add(mCurrentResultSet);
        sExecResult.mReturned = true;

        return mCurrentResultSet;
    }
    
    public void addBatch(String aSql) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForNullSqlString(aSql);

        if(mBatchSqlList == null)
        {
            mBatchSqlList = new LinkedList();
        }
        else
        {
            if (mBatchSqlList.size() == Integer.MAX_VALUE) 
            {
                Error.throwSQLException(ErrorDef.TOO_MANY_BATCH_JOBS);
            }
        }

        if (mEscapeProcessing)
        {
            aSql = AltiSqlProcessor.processEscape(aSql);
        }
        mBatchSqlList.add(aSql);
    }

    public void cancel() throws SQLException
    {
        throwErrorForClosed();

        AltibaseConnection sPrivateConnection = mConnection.cloneConnection();
        CmProtocolContextDirExec sContext = new CmProtocolContextDirExec(sPrivateConnection.channel());
        try
        {
            CmProtocol.cancelStatement(sContext, mStmtCID);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
        }
        try
        {
            if (sContext.getError() != null)
            {
                Error.processServerError(null, sContext.getError());
            }
        }
        finally
        {
            sPrivateConnection.close();
        }
    }

    public void closeCursor() throws SQLException
    {
        throwErrorForClosed();

        if (!shouldCloseCursor())
        {
            return;
        }

        CmProtocolContextDirExec sContext = getProtocolContext();
        try
        {
            CmProtocol.closeCursorInternal(sContext);
        }
        catch (SQLException ex)
        {
            // BUGBUG : 언제 tryShardFailOver 사용하고, 언제 trySTF 사용하나?
            tryShardFailOver(mConnection, ex);   
            //AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
        }
        if (getProtocolContext().getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, getProtocolContext().getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(getMetaConn(), getProtocolContext().getError());
            }
        }
    }

    boolean shouldCloseCursor()
    {
        return mPrepareResult != null && mPrepareResult.isSelectStatement();
    }

    public void clearBatch() throws SQLException
    {
        throwErrorForClosed();
        if(mBatchSqlList != null)
        {
            mBatchSqlList.clear();
        }
    }

    public void clearWarnings() throws SQLException
    {
        throwErrorForClosed();

        mWarning = null;
    }

    public void close() throws SQLException
    {
        if (isClosed() == true)
        {
            return;
        }

        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
        if (isAsyncPrefetch())
        {
            ((AltibaseForwardOnlyResultSet)mCurrentResultSet).endFetchAsync();
        }

        mSemiAsyncPrefetch = null;

        if (mPrepareResult != null)
        {
            CmProtocol.freeStatement(getProtocolContext(), mPrepareResult.getStatementId());
            CmProtocol.clientCommit(getProtocolContext(), mConnection.isClientSideAutoCommit());
        }
        mConnection.removeStatement(this);

        if (mInternalStatement != null)
        {
            mInternalStatement.close();
        }

        mIsClosed = true;
    }

    /**
     * {@link Connection#close()}를 위한 상태 전이(opened ==> closed)용 메소드
     */
    void closeForRelease()
    {
        if (mInternalStatement != null)
        {
            mInternalStatement.closeForRelease();
        }
        mIsClosed = true;
    }

    synchronized void close4STF() throws SQLException
    {
        if (mIsClosed)
        {
            return;
        }

        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
        if (isAsyncPrefetch())
        {
            mConnection.clearAsyncPrefetchStatement();
        }

        mSemiAsyncPrefetch = null;

        // for clearAllResults
        mExecuteResultMgr.clear();
        mResultSetList.clear();
        mCurrentResultSet = null;
        mGeneratedKeyResultSet = null;

        clearBatch();
        clearWarnings();
        mQstr = null;
        mStmtCID = 0;
        mContext = null;

        if (mInternalStatement != null)
        {
            mInternalStatement.close4STF();
        }

        mIsClosed = true;
    }

    public synchronized boolean execute(String aSql, int aAutoGeneratedKeys)
            throws SQLException
    {
        checkAutoGeneratedKeys(aAutoGeneratedKeys);

        if (aAutoGeneratedKeys == RETURN_GENERATED_KEYS)
        {
            makeQstrForGeneratedKeys(aSql, null, null);
        }
        else
        {
            clearForGeneratedKeys();
        }
        boolean sResult = execute(aSql);
        executeForGeneratedKeys();
        return sResult;
    }

    public boolean execute(String aSql, int[] aColumnIndexes) throws SQLException
    {
        makeQstrForGeneratedKeys(aSql, aColumnIndexes, null);
        boolean sResult = execute(aSql);
        executeForGeneratedKeys();
        return sResult;
    }

    public boolean execute(String aSql, String[] aColumnNames)
            throws SQLException
    {
        makeQstrForGeneratedKeys(aSql, null, aColumnNames);
        boolean sResult = execute(aSql);
        executeForGeneratedKeys();
        return sResult;
    }

    /*
     * 이 메소드의 리턴값에 대해 스펙과 조금 다른 부분이 있다.
     * PSM같은 query를 실행하면 여러개의 result(ResultSet일 수도있고,
     * update count일 수도 있다)가 발생할 수 있는데,
     * JDBC Spec에는 첫번째 result가 ResultSet이 아닌 경우
     * 이 메소드의 리턴값은 false라고 정의한다. 
     * 즉 Spec에 따르면, 이 메소드가 false를 리턴한다고 해서
     * result에 ResultSet이 없다고 보장할 수 없다.
     * 하지만, 실제 구현에서는 ResultSet이 있으면 반드시
     * 첫번째 result부터 ResultSet이 나오도록 구현했다.
     * 따라서 이 메소드가 false를 리턴한다는 것은
     * ResultSet이 result에 포함되어 있지 않다는 것을 보장한다. 
     */
    public boolean execute(String aSql) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForNullSqlString(aSql);
        throwErrorForBatchJob("execute");

        clearAllResults();
        
        if (mEscapeProcessing)
        {
            aSql = AltiSqlProcessor.processEscape(aSql);
        }        
        setSql(aSql);

        // BUG-39149 ping sql일 경우에는 내부적으로 ping메소드를 호출하고 light-weight ResultSet을 리턴한다.
        if (isPingSQL(aSql))
        {
            pingAndCreateLightweightResultSet(); 
            return true;
        }
        
        try
        {
            aSql = procDowngradeAndGetTargetSql(aSql);
            // PROJ-2427 cursor를 닫아야하는 조건을 매개변수로 넘겨준다.
            CmProtocol.directExecute(getProtocolContext(),
                                     mStmtCID,
                                     aSql,
                                     (mResultSetHoldability == ResultSet.HOLD_CURSORS_OVER_COMMIT),
                                     usingKeySetDriven(),
                                     mConnection.nliteralReplaceOn(), mConnection.isClientSideAutoCommit(),
                                     mPrepareResult != null && mPrepareResult.isSelectStatement());
        }
        catch (SQLException ex)
        {
            mIsSuccess = false;  // For partial rollback
            tryShardFailOver(mConnection, ex);
        }
        afterExecution();

        return processExecutionResult();
    }

    protected static final long[] EMPTY_LARGE_BATCH_RESULT = new long[0];

    public int[] executeBatch() throws SQLException
    {
        return toIntBatchUpdateCounts(executeLargeBatch());
    }

    @Override
    public long[] executeLargeBatch() throws SQLException
    {
        throwErrorForClosed();
        clearAllResults();

        if (mBatchSqlList == null || mBatchSqlList.isEmpty())
        {
            mExecuteResultMgr.add(new ExecuteResult(false, DEFAULT_UPDATE_COUNT));
            return EMPTY_LARGE_BATCH_RESULT;
        }

        try
        {
            CmProtocol.directExecuteBatch(getProtocolContext(),
                                          mStmtCID,
                                          (String[])mBatchSqlList.toArray(new String[0]),
                                          mConnection.nliteralReplaceOn(), mConnection.isClientSideAutoCommit());
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
        }
        finally
        {
            // BUGBUG (2012-11-28) 스펙에서는 executeBatch 했을때 clear되는게 명확하게 나오지 않은 듯. oracle에 따른다.
            clearBatch();
        }
        try
        {
            afterExecution();
        }
        catch (SQLException sEx)
        {
            CmExecutionResult sExecResult = getProtocolContext().getExecutionResult();
            long[] sLongUpdatedRowCounts = sExecResult.getUpdatedRowCounts();
            Error.throwBatchUpdateException(sEx, toIntBatchUpdateCounts(sLongUpdatedRowCounts));
        }
        long[] sUpdatedRowCounts = mExecutionResult.getUpdatedRowCounts();
        long sUpdateCount = 0;
        for (long sUpdatedRowCount : sUpdatedRowCounts)
        {
            sUpdateCount += sUpdatedRowCount;
        }
        mExecuteResultMgr.add(new ExecuteResult(false, sUpdateCount));
        return sUpdatedRowCounts;
    }

    public ResultSet executeQuery(String aSql) throws SQLException
    {
        throwErrorForClosed();
        throwErrorForNullSqlString(aSql);
        throwErrorForBatchJob("executeQuery");
        
        clearAllResults();
        
        if (mEscapeProcessing)
        {
            aSql = AltiSqlProcessor.processEscape(aSql);
        }
        setSql(aSql);

        // BUG-39149 ping sql일 경우에는 내부적으로 ping메소드를 호출하고 light-weight ResultSet을 리턴한다.
        if (isPingSQL(aSql))
        {
            pingAndCreateLightweightResultSet(); 
            return mCurrentResultSet;
        }
        
        try
        {
            aSql = procDowngradeAndGetTargetSql(aSql);
            // PROJ-2427 cursor를 닫아야 하는 조건을 같이 넘겨준다.
            CmProtocol.directExecuteAndFetch(getProtocolContext(),
                                             mStmtCID, 
                                             aSql,
                                             mFetchSize,
                                             mMaxRows,
                                             mMaxFieldSize,
                                             mResultSetHoldability == ResultSet.HOLD_CURSORS_OVER_COMMIT,
                                             usingKeySetDriven(),
                                             mConnection.nliteralReplaceOn(),
                                             mPrepareResult != null && mPrepareResult.isSelectStatement());
        }
        catch (SQLException ex)
        {
            tryShardFailOver(mConnection, ex);
        }
        afterExecution();
        
        return processExecutionQueryResult(aSql);
    }

    /**
     *  내부적으로 ping메소드를 호출한 다음 정상적일 경우 SELECT 1의 결과에 해당하는 row를 수동으로 생성한 후 반환한다.
     *  @throws SQLException Column객체에 setValue를 수행했을때 에러가 발생한 경우.
     */
    protected void pingAndCreateLightweightResultSet() throws SQLException
    {
        mConnection.ping();
        ExecuteResult sExecResult = new ExecuteResult(true, DEFAULT_UPDATE_COUNT);
        mExecuteResultMgr.add(sExecResult);
        List<Column> aColumns = new ArrayList<>();
        aColumns.add(ColumnFactory.createSmallintColumn());
        aColumns.get(0).setValue(1);
        ColumnInfo sColumnInfo = new ColumnInfo();
        // BUG-39149 select 1 쿼리의 column meta 정보를 수동으로 구성해 준다.
        sColumnInfo.setColumnInfo(Types.SMALLINT,                               // dataType
                                  0,                                            // language
                                  (byte)0,                                      // arguments
                                  0,                                            // precision
                                  0,                                            // scale
                                  ColumnInfo.IN_OUT_TARGET_TYPE_TARGET,         // in-out type
                                  true,                                         // nullable
                                  false,                                        // updatable
                                  null,                                         // catalog name
                                  null,                                         // table name
                                  null,                                         // base table name
                                  null,                                         // col name
                                  "1" ,                                         // display name
                                  null,                                         // base column name
                                  null,                                         // schema name
                                  BYTES_PER_CHAR);                              // bytes per char
        aColumns.get(0).setColumnInfo(sColumnInfo);
        mCurrentResultSet = new AltibaseLightWeightResultSet(this, aColumns, this.mResultSetType);
        sExecResult.mReturned = true;
    }

    /**
     * sql이 validation check용 ping 쿼리인지 여부를 정규 표현식을 이용해 판별한다.
     * @param aSql 쿼리 스트링
     * @return ping 쿼리인지 여부
     */
    protected boolean isPingSQL(String aSql)
    {
        if (StringUtils.isEmpty(aSql))
        {
            return false;
        }

        return mPingQueryPattern.matcher(aSql).matches();
    }

    // BUGBUG (2012-11-06) 지원하는 것이 스펙에서 설명하는것과 조금 다르다.
    // 스펙에서는 AUTO_INCREMENT나 ROWID처럼 자동으로 생성되는 유일값을 얻을 수 있는 메소드로 소개한다.
    // 그런데, Altibase는 AUTO_INCREMENT나 ROWID를 제공하지 않고,
    // 또 INSERT된 ROW의 유일한 식별자를 얻을 방법도 없으므로,
    // INSERT에 사용한 SEQUENCE의 값(CURRVAL)을 반환한다.
    public int executeUpdate(String aSql, int aAutoGeneratedKeys)
            throws SQLException
    {
        boolean sHasResult = execute(aSql, aAutoGeneratedKeys);        
        // BUGBUG (2013-01-31) spec에 따르면 예외를 던져야 한다. 그런데, oracle은 안그런다. 더러운 oracle..
//        Error.checkAndThrowSQLException(sHasResult, ErrorDef.SQL_RETURNS_RESULTSET, aSql);
        return toInt(mExecuteResultMgr.getFirst().mUpdatedCount);
    }

    // BUGBUG (2012-11-06) 스펙과 다르다.
    // 스펙 설명에 따르면 column index는 DB TABLE에서의 컬럼 순번을 의미한다. (3rd, p955)
    // 그래서 스펙대로라면 INSERT 쿼리문에 없는 컬럼이라도 지정할 수 있다.
    // 하지만, Altibase JDBC는 사용한 SEQUENCE 값을 반환하므로 테이블 컬럼의 순번을 사용할 수 없다.
    // 대신, SEQUENCE가 나열된 순번을 사용한다. 이 때, SEQUENCE가 아닌 것은 셈하지 않는다.
    // 예를들어, "INSERT INTO t1 VALUES (SEQ1.NEXTVAL, '1', SEQ2.NEXTVAL)"와 같은 INSERT문을 쓸 때,
    // SEQ1.CURRVAL을 얻으려면 1, SEQ2.CURRVAL을 얻으려면 2를 사용해야 한다.
    public int executeUpdate(String aSql, int[] aColumnIndexes)
            throws SQLException
    {
        boolean sHasResult = execute(aSql, aColumnIndexes);
        // BUGBUG (2013-01-31) spec에 따르면 예외를 던져야 한다. 그런데, oracle은 안그런다. 더러운 oracle..
//        Error.checkAndThrowSQLException(sHasResult, ErrorDef.SQL_RETURNS_RESULTSET, aSql);
        return toInt(mExecuteResultMgr.getFirst().mUpdatedCount);
    }

    // BUGBUG (2012-11-06) 스펙과 다르다.
    // 스펙 설명에 따르면 column name은 DB TABLE에서의 컬럼 이름을 의미한다. (3rd, p955)
    // 그래서 스펙대로라면 INSERT 쿼리문에 없는 컬럼이라도 지정할 수 있다.
    // 하지만, Altibase JDBC는 사용한 SEQUENCE 값을 반환하므로 테이블 컬럼의 이름을 사용할 수 없다.
    // 대신, INSERT 문에 사용한 컬럼 이름을 사용한다.
    // 그러므로, 이 메소드를 사용하기 위해서는 반드시
    // "INSERT INTO t1 (c1, c2) VALUES (SEQ1.NEXTVAL, '1')"처럼
    // 컬럼 이름을 명시한 INSERT 문을 사용해야한다.
    public int executeUpdate(String aSql, String[] aColumnNames)
            throws SQLException
    {
        boolean sHasResult = execute(aSql, aColumnNames);        
        // BUGBUG (2013-01-31) spec에 따르면 예외를 던져야 한다. 그런데, oracle은 안그런다. 더러운 oracle..
//        Error.checkAndThrowSQLException(sHasResult, ErrorDef.SQL_RETURNS_RESULTSET, aSql);
        return toInt(mExecuteResultMgr.getFirst().mUpdatedCount);
    }

    public int executeUpdate(String aSql) throws SQLException
    {
        return toInt(executeLargeUpdate(aSql));
    }

    public AltibaseConnection getAltibaseConnection()
    {
        return mConnection;
    }

    public Connection getConnection() throws SQLException
    {
        throwErrorForClosed();

        return mConnection;
    }

    public int getFetchDirection() throws SQLException
    {
        throwErrorForClosed();

        return ResultSet.FETCH_FORWARD;
    }

    public int getFetchSize() throws SQLException
    {
        throwErrorForClosed();

        return mFetchSize;
    }

    public ResultSet getGeneratedKeys() throws SQLException
    {
        throwErrorForClosed();

        return mGeneratedKeyResultSet;
    }

    public int getMaxFieldSize() throws SQLException
    {
        throwErrorForClosed();

        return mMaxFieldSize;
    }

    public int getMaxRows() throws SQLException
    {
        throwErrorForClosed();

        return (int)mMaxRows;
    }

    public boolean getMoreResults() throws SQLException
    {
        throwErrorForClosed();

        if (mCurrentResultIndex >= mExecuteResultMgr.size() - 1)
        {
            /* BUG-47360 getMoreResults()의 결과가 없을때는 mCurrentResultIndex를 초기화 해줘야 한다. 그렇지 않으면 Statement를
               다시 execute할때 에러가 발생한다. */
            mCurrentResultIndex = 0;
            return false;
        }
        
        incCurrentResultSetIndex();
        
        return true;
    }
    
    protected void incCurrentResultSetIndex()
    {
        mCurrentResultIndex++;
    }

    public boolean getMoreResults(int current) throws SQLException
    {
        throwErrorForClosed();
        
        if (mCurrentResultIndex >= mExecuteResultMgr.size() - 1)
        {
            mCurrentResultIndex = 0;
            return false;
        }
        
        switch(current)
        {
            case Statement.KEEP_CURRENT_RESULT :
                break;
            case Statement.CLOSE_CURRENT_RESULT :
                mCurrentResultSet.close();
                break;
            case Statement.CLOSE_ALL_RESULTS :
                for(int i = 0; i <= mCurrentResultIndex; i++)
                {
                    ResultSet rs = mResultSetList.get(i);
                    rs.close();
                }
                break;
            default :
                Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                        "Current",
                                        "KEEP_CURRENT_RESULT | CLOSE_CURRENT_RESULT | CLOSE_ALL_RESULTS",
                                        String.valueOf(current));
                return false;
        }
        
        incCurrentResultSetIndex();
        
        return true;
    }

    public int getQueryTimeout() throws SQLException
    {
        throwErrorForClosed();

        // BUGBUG (!) 일단 무시. 서버의 timeout 속성은 session 단위이기 때문.
        return mQueryTimeout;
    }

    public ResultSet getResultSet() throws SQLException
    {
        throwErrorForClosed();

        getProtocolContext().setResultSetId((short)mCurrentResultIndex);

        // BUG-48380 이미 ResultSet이 존재한다면 기존에 있던 ResultSet을 리턴한다.
        if (mResultSetReturned)
        {
            return mCurrentResultSet;
        }
        ExecuteResult sResult = (ExecuteResult)mExecuteResultMgr.get(mCurrentResultIndex);
        if (sResult.mReturned)
        {
            return mCurrentResultSet;
        }
        if (!sResult.mHasResultSet)
        {
            return null;
        }
        
        try
        {
            CmProtocol.fetch(getProtocolContext(), mFetchSize, mMaxRows, mMaxFieldSize);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
        }
        if (getProtocolContext().getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, getProtocolContext().getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(getMetaConn(), getProtocolContext().getError());
            }
        }
        mFetchResult = getProtocolContext().getFetchResult();
        
        mCurrentResultSet = AltibaseResultSet.createResultSet(this, mTargetResultSetType, mTargetResultSetConcurrency);
        mCurrentResultSet.setAllowLobNullSelect(mConnection.getAllowLobNullSelect());
        mResultSetList.add(mCurrentResultSet);

        sResult.mReturned = true;
        return mCurrentResultSet;
    }

    public int getResultSetConcurrency() throws SQLException
    {
        throwErrorForClosed();

        return mResultSetConcurrency;
    }

    public int getResultSetHoldability() throws SQLException
    {
        throwErrorForClosed();

        return mResultSetHoldability;
    }

    public int getResultSetType() throws SQLException
    {
        throwErrorForClosed();

        return mResultSetType;
    }

    public int getUpdateCount() throws SQLException
    {
        return toInt(getLargeUpdateCount());
    }

    public SQLWarning getWarnings() throws SQLException
    {
        throwErrorForClosed();

        return mWarning;
    }
    
    /**
     * Statement가 닫혔는지 확인한다. 
     * 
     * @return close가 잘 수행되었거나 이미 Connection이 close되었으면 true, 아니면 false
     */
    public boolean isClosed() throws SQLException
    {
        return mIsClosed || mConnection.isClosed() == true;
    }

    public void setCursorName(String aName) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "cursor name and positioned update");
    }

    public void setEscapeProcessing(boolean aEnable) throws SQLException
    {
        throwErrorForClosed();

        mEscapeProcessing = aEnable;
    }

    public void setFetchDirection(int aDirection) throws SQLException
    {
        throwErrorForClosed();

        AltibaseResultSet.checkFetchDirection(aDirection);
    }

    public void setFetchSize(int aRows) throws SQLException
    {
        throwErrorForClosed();
        if (aRows < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Fetch size",
                                    AltibaseProperties.RANGE_FETCH_ENOUGH,
                                    String.valueOf(aRows));
        }
        mFetchSize = downgradeFetchSize(aRows);
        createFetchSizeWarning(aRows);
    }

    private void createFetchSizeWarning(int aRows)
    {
        if (mFetchSize < aRows)
        {
            mWarning = Error.createWarning(mWarning, ErrorDef.TOO_LARGE_FETCH_SIZE,
                                           String.valueOf(mFetchSize),
                                           String.valueOf(aRows));
        }
    }

    /**
     * Dynamic Array에서 허용할 수 있는 최대 범위로 fetchSize를 보정한다.
     * @param aRows fetchSize
     */
    protected int downgradeFetchSize(int aRows)
    {
        int sMaxFetchSize = getMaxFetchSize();

        /* BUG-43263 aRows가 DynamicArray가 수용할 수 있는 범위를 벗어나면 허용할 수 있는 최대값으로 값을 보정한다. */
        if (aRows > sMaxFetchSize)
        {
            /* BUG-43263 첫번째 chunk의 0번째 인덱스는 beforeFirst를 위해 비워두고 loadcursor에서 index를 먼저 증가시키기 때문에 실제로
               들어갈 수 있는 최대 rows값은 DynamicArray최대치 - 2 이다. */
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.INFO, "Fetch size downgraded from {0} to {1} ",
                            new Object[] { aRows, sMaxFetchSize });
            }
            aRows = sMaxFetchSize;
        }

        return aRows;
    }

    /**
     * 최대 fetch size 를 반환한다.
     */
    protected static int getMaxFetchSize()
    {
        return DynamicArray.getDynamicArrySize() - 2;
    }

    public void setMaxFieldSize(int aMax) throws SQLException
    {
        throwErrorForClosed();
        if (aMax < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Max field size",
                                    "0 ~ Integer.MAX_VALUE",
                                    String.valueOf(aMax));
        }

        mMaxFieldSize = aMax;
    }

    public void setMaxRows(int aMax) throws SQLException
    {
        throwErrorForClosed();
        if (aMax < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Max rows",
                                    "0 ~ Integer.MAX_VALUE",
                                    String.valueOf(aMax));
        }

        mMaxRows = aMax;
    }

    public void setQueryTimeout(int aSeconds) throws SQLException
    {
        throwErrorForClosed();
        if (aSeconds < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Query timeout",
                                    "0 ~ Integer.MAX_VALUE",
                                    String.valueOf(aSeconds));
        }

        // BUGBUG (!) 일단 무시. 서버의 timeout 속성은 session 단위이기 때문.
        mQueryTimeout = aSeconds;
    }

    /**
     * Plan text를 얻는다.
     *
     * @return Plan text
     * @throws SQLException Plan text 요청이나 결과를 얻는데 실패했을 때
     */
    public String getExplainPlan() throws SQLException
    {
        throwErrorForClosed();
        throwErrorForExplainTurnedOff();
        throwErrorForStatementNotPrepared();

        try
        {
            CmProtocol.getPlan(getProtocolContext(), mPrepareResult.getStatementId(),
                               getProtocolContext().getDeferredRequests());
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
        }
        if (getProtocolContext().getError() != null)
        {
            try
            {
                mWarning = Error.processServerError(mWarning, getProtocolContext().getError());
            }
            finally
            {
                // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                ShardError.processShardError(getMetaConn(), getProtocolContext().getError());
            }
        }
        CmGetPlanResult sResult = getProtocolContext().getGetPlanResult();
        if (sResult.getStatementId() != mPrepareResult.getStatementId())
        {
            Error.throwSQLException(ErrorDef.STMT_ID_MISMATCH,
                                    String.valueOf(mPrepareResult.getStatementId()),
                                    String.valueOf(sResult.getStatementId()));
        }
        return sResult.getPlanText();
    }

    // #region Result Set Downgrade

    /**
     * ResultSet 설정이 KeySet driven을 쓰게 되어있는지 확인한다.
     *
     * @return KeySet driven을 쓰는지 여부
     * @throws SQLException ResultSet 설정을 확인하는데 실패한 경우
     */
    final boolean usingKeySetDriven() throws SQLException
    {
        return (mTargetResultSetType == ResultSet.TYPE_SCROLL_SENSITIVE) ||
               (mTargetResultSetConcurrency == ResultSet.CONCUR_UPDATABLE);
    }

    /**
     * 커서 속성을 Downgrade 한다.
     * <p>
     * Downgrade Rule은 다음과 같다:
     * <ul>
     * <li>ResultSetType : TYPE_SCROLL_SENSITIVE --> TYPE_SCROLL_INSENSITIVE --> TYPE_FORWARD_ONLY</li>
     * <li>ResultSetConcurrency : CONCUR_UPDATABLE --> CONCUR_READ_ONLY</li>
     * <ul>
     * <p>
     * 이 중 TYPE_SCROLL_INSENSITIVE --> TYPE_FORWARD_ONLY는 실제로는 일어나지 않는다.
     * 왜냐하면 TYPE_SCROLL_INSENSITIVE이 TYPE_FORWARD_ONLY을 기반으로 한 것이라,
     * 쿼리문 자체가 잘못된게 아니라면 실패할 일이 없기 때문이다.
     * <p>
     * Downgrade는 ResultSetType와 ResultSetConcurrency가 동시에 일어난다.
     */
    private final void downgradeTargetResultSetAttrs()
    {
        if (mTargetResultSetType == ResultSet.TYPE_SCROLL_SENSITIVE)
        {
            mTargetResultSetType = ResultSet.TYPE_SCROLL_INSENSITIVE;
            mWarning = Error.createWarning(mWarning, ErrorDef.OPTION_VALUE_CHANGED,
                                           "ResultSet type downgraded to TYPE_SCROLL_INSENSITIVE");
        }

        if (mTargetResultSetConcurrency == ResultSet.CONCUR_UPDATABLE)
        {
            mTargetResultSetConcurrency = ResultSet.CONCUR_READ_ONLY;
            mWarning = Error.createWarning(mWarning, ErrorDef.OPTION_VALUE_CHANGED,
                                           "ResultSet concurrency changed to CONCUR_READ_ONLY");
        }
    }

    /**
     * Downgrade가 필요하면 처리하고, 실제 사용할 변환된 쿼리문을 얻는다.
     * <p>
     * 만약 Downgrade나 쿼리 변환이 필요 없다면 조용히 넘어가고 원본 쿼리문을 반환한다.
     * 
     * @param aOrgQstr 원본 쿼리문
     * @return 실제로 사용할 쿼리문
     * @throws SQLException Downgrade 여부 확인 또는 Downgrade에 실패한 경우
     */
    protected final String procDowngradeAndGetTargetSql(String aOrgQstr) throws SQLException
    {
        mTargetResultSetType = getResultSetType();
        mTargetResultSetConcurrency = getResultSetConcurrency();
        mPrepareResultColumns = null;

        if (!usingKeySetDriven())
        {
            return aOrgQstr;
        }

        CmGetColumnInfoResult sColumnInfoResult = null;
        String sQstr = AltiSqlProcessor.makePRowIDAddedSql(aOrgQstr);
        if (sQstr != null)
        {
            // BUG-42424 keyset driven에서는 prepare결과를 바로 받아와야 하기때문에 deferred를 false로 보낸다.
            CmProtocol.prepare(getProtocolContext(),
                               mStmtCID,
                               sQstr,
                               (getResultSetHoldability() == ResultSet.HOLD_CURSORS_OVER_COMMIT),
                               true,
                               mConnection.nliteralReplaceOn(),
                               false);

            sColumnInfoResult = getProtocolContext().getGetColumnInfoResult();
        }
        if (sQstr == null ||
            Error.hasServerError(getProtocolContext().getError()) ||
            (mTargetResultSetConcurrency == ResultSet.CONCUR_UPDATABLE &&
             checkUpdatableColumnInfo(sColumnInfoResult) == false))
        {
            downgradeTargetResultSetAttrs();
            sQstr = aOrgQstr;
        }
        else
        {
            // sensitive일 때는 keyset만 쌓고 데이타는 따로 가져온다.
            if (mTargetResultSetType == ResultSet.TYPE_SCROLL_SENSITIVE)
            {
                // getMetaData()를 위해 컬럼 정보를 백업해둔다.
                mPrepareResultColumns = sColumnInfoResult.getColumns();

                HashMap sOrderByMap = new HashMap();
                for (int i = 0; i < mPrepareResultColumns.size(); i++)
                {
                    ColumnInfo sColumnInfo = mPrepareResultColumns.get(i).getColumnInfo();
                    sOrderByMap.put(String.valueOf(i + 1), sColumnInfo.getBaseColumnName());
                    sOrderByMap.put(sColumnInfo.getColumnName(), sColumnInfo.getBaseColumnName());
                    sOrderByMap.put(sColumnInfo.getDisplayColumnName(), sColumnInfo.getBaseColumnName());
                }
                sQstr = AltiSqlProcessor.makeKeySetSql(aOrgQstr, sOrderByMap);
            }
        }
        if (mIsDeferred)  // BUG-42712 deferred 상태일때는 Context의 Error를 clear해준다.
        {
            getProtocolContext().clearError();
        }
        return sQstr;
    }

    private boolean checkUpdatableColumnInfo(CmGetColumnInfoResult aGetColumnInfoResult) throws SQLException
    {
        List<Column> sColumns = aGetColumnInfoResult.getColumns();
        if (sColumns == null || sColumns.size() == 0)
        {
            return false;
        }

        // updatable이려면 단일 테이블에대한 쿼리여야하고 모든 컬럼은 테이블 컬럼이어야 한다.
        ColumnInfo sColInfo = sColumns.get(0).getColumnInfo();
        if (StringUtils.isEmpty(sColInfo.getBaseColumnName()))
        {
            return false;
        }
        String sBaseTableName = sColInfo.getBaseTableName();
        if (StringUtils.isEmpty(sBaseTableName))
        {
            return false;
        }
        for (int i = 1; i < sColumns.size(); i++)
        {
            sColInfo = sColumns.get(i).getColumnInfo();
            if (StringUtils.isEmpty(sColInfo.getBaseColumnName()))
            {
                return false;
            }
            if (StringUtils.isEmpty(sColInfo.getBaseTableName()))
            {
                return false;
            }
            if (sBaseTableName.compareToIgnoreCase(sColInfo.getBaseTableName()) != 0)
            {
                return false;
            }
        }
        return true;
    }

    // #endregion

    public static void checkAutoGeneratedKeys(int aAutoGeneratedKeys) throws SQLException
    {
        if (!isValidAutoGeneratedKeys(aAutoGeneratedKeys))
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Auto generated keys",
                                    "RETURN_GENERATED_KEYS | NO_GENERATED_KEYS",
                                    String.valueOf(aAutoGeneratedKeys));
        }
    }

    static boolean isValidAutoGeneratedKeys(int aAutoGeneratedKeys)
    {
        switch (aAutoGeneratedKeys)
        {
            case RETURN_GENERATED_KEYS:
            case NO_GENERATED_KEYS:
                return true;
            default:
                return false;
        }
    }

    void throwErrorForClosed() throws SQLException
    {
        if (mIsClosed)
        {
            Error.throwSQLException(ErrorDef.CLOSED_STATEMENT);            
        }
        if (mConnection.isClosed())
        {
            Error.throwSQLException(ErrorDef.CLOSED_CONNECTION);            
        }
    }

    private void throwErrorForNullSqlString(String aSql) throws SQLException
    {
        if (aSql == null) 
        {
            Error.throwSQLException(ErrorDef.NULL_SQL_STRING);
        }
    }
    
    private void throwErrorForBatchJob(String aCommand) throws SQLException
    {
        if (mBatchSqlList != null && !mBatchSqlList.isEmpty())  
        { 
            Error.throwSQLException(ErrorDef.SOME_BATCH_JOB, aCommand); 
        }
    }
    
    private void throwErrorForResultSetCount(String aSql) throws SQLException
    {
        if (mPrepareResult.getResultSetCount() == 0) 
        {
            Error.throwSQLException(ErrorDef.NO_RESULTSET, aSql);
        }
        if (mPrepareResult.getResultSetCount() > 1)  
        {
            Error.throwSQLException(ErrorDef.MULTIPLE_RESULTSET_RETURNED, aSql);
        }
    }
    
    // BUG-42424 AltibasePreparedStatement.getMetaData에서도 사용되기 때문에 default scope로 변경
    void throwErrorForStatementNotPrepared() throws SQLException
    {
        if (mPrepareResult == null)
        {
            Error.throwSQLException(ErrorDef.STATEMENT_IS_NOT_PREPARED);
        }
    }
    
    private void throwErrorForExplainTurnedOff() throws SQLException
    {
        if (mConnection.explainPlanMode() == AltibaseConnection.EXPLAIN_PLAN_OFF) 
        {
            Error.throwSQLException(ErrorDef.EXPLAIN_PLAN_TURNED_OFF);
        }
    }

    /**
     * 비동기적으로 fetch 를 수행하고 있는 statement 여부인지 확인한다.
     */
    protected boolean isAsyncPrefetch()
    {
        return (mConnection.getAsyncPrefetchStatement() == this);
    }

    /**
     * Internal statement 생성한다.
     */
    static protected AltibaseStatement createInternalStatement(AltibaseConnection sConnection) throws SQLException
    {
        AltibaseStatement sInternalStatement = new AltibaseStatement(sConnection);
        sInternalStatement.mIsInternalStatement = true;

        return sInternalStatement;
    }

    /**
     * Internal statement 여부를 확인하다.
     */
    protected boolean isInternalStatement()
    {
        return mIsInternalStatement;
    }

    /** Prepared 여부를 리턴한다. */
    public boolean isPrepared()
    {
        if (mPrepareResult == null) return false;
        return  mPrepareResult.isPrepared();
    }

    /**
     * Semi-async prefetch 동작을 위한 SemiAsyncPrefetch 객체를 반환한다.
     */
    synchronized SemiAsyncPrefetch getSemiAsyncPrefetch()
    {
        return mSemiAsyncPrefetch;
    }

    /**
     * Semi-async prefetch 동작 중인 SemiAsyncPrefetch 객체를 설정하고 re-execute 시 재사용한다.
     */
    synchronized void setSemiAsyncPrefetch(SemiAsyncPrefetch aSemiAsyncPrefetch)
    {
        mSemiAsyncPrefetch = aSemiAsyncPrefetch;
    }

    /**
     * Trace logging 시 statement 간 식별을 위해 unique id 를 반환한다.
     */
    String getTraceUniqueId()
    {
        return "[StmtId #" + String.valueOf(hashCode()) + "] ";
    }

    /**
     * 서버 커서가 열려있는지 여부를 리턴한다.
     * @return true Select쿼리가 아니거나 서버커서가 모두 닫힌 경우
     */
    public boolean cursorhasNoData()
    {
        boolean sResult = false;

        AltibaseResultSet sCurrResultSet = mCurrentResultSet;
        if (sCurrResultSet == null || sCurrResultSet instanceof AltibaseEmptyResultSet)
        {
            return true;
        }

        if (mPrepareResult.getResultSetCount() == 0)
        {
            sResult = true;
        }
        // BUG-46513 ResultSet이 여러개인 경우 마지막 ResultSet인지 확인한다.
        else if ((mCurrentResultIndex >= mExecuteResultMgr.size() - 1) &&
                 (!sCurrResultSet.fetchRemains()))
        {
            sResult = true;
        }

        return sResult;
    }

    public void setMetaConnection(AltibaseShardingConnection aMetaConn)
    {
        mMetaConn = aMetaConn;
    }

    public AltibaseShardingConnection getMetaConn()
    {
        return mMetaConn;
    }

    public void setPrepareResult(CmPrepareResult aPrepareResult)
    {
        // BUG-47274 Sharding statement로부터 CmPrepareResult객체를 주입받는다.
        this.mPrepareResult = aPrepareResult;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("AltibaseStatement{");
        sSb.append("mStmtCID=").append(mStmtCID);
        sSb.append("mStmtID=").append(getID());
        sSb.append(", mQstr='").append(mQstr).append('\'');
        sSb.append('}');
        return sSb.toString();
    }

    /**
     * ResultSet이 close될때 호출되며 closeOnCompletion이 활성화 되어 있는 경우 Statement에 딸린 ResultSet이
     * 모두 close 되었을 때 Statement자원을 해제한다.
     * @throws SQLException ResultSet이 close되었는지 확인하는 경우 예외가 발생할때
     */
    void checkCloseOnCompletion() throws SQLException
    {
        if (!mCloseOnCompletion)
        {
            return;
        }
        // PROJ-2707 아직 getMoreResults()로 더 가져올 resultset이 있다면 그냥 리턴.
        if (mCurrentResultIndex < mExecuteResultMgr.size() - 1)
        {
            return;
        }
        for (ResultSet sEach : mResultSetList)
        {
            if (!sEach.isClosed())
            {
                return; // PROJ-2707 하나라도 close안된 resultset이 있으면 그냥 return 한다.
            }
        }
        try
        {
            close();
        }
        catch (SQLException aEX)
        {
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.SEVERE, "Statement close failed during closeOnCompletion", aEX);
            }
        }
    }

    @Override
    public void closeOnCompletion() throws SQLException
    {
        mCloseOnCompletion = true;
    }

    @Override
    public boolean isCloseOnCompletion() throws SQLException
    {
        return mCloseOnCompletion;
    }

    @Override
    public long executeLargeUpdate(String aSql) throws SQLException
    {
        execute(aSql);
        return mExecuteResultMgr.getFirst().mUpdatedCount;
    }

    @Override
    public long getLargeUpdateCount() throws SQLException
    {
        throwErrorForClosed();

        if (mResultSetReturned)
        {
            return DEFAULT_UPDATE_COUNT;
        }

        ExecuteResult sResult = mExecuteResultMgr.get(mCurrentResultIndex);

        long sUpdateCount;
        // BUG-38657 결과값이 ResultSet이거나 더이상 결과값이 없을때는 -1을 리턴하고
        // 그 외에는 업데이트 된 로우수를 리턴한다.
        if (sResult.mHasResultSet)
        {
            sUpdateCount = DEFAULT_UPDATE_COUNT;
        }
        else
        {
            sUpdateCount = sResult.mUpdatedCount;
        }
        // BUG-38657 getUpdateCount가 두 번 이상 실행되는 경우에는 -1을 리턴하도록 한다.
        // 이 메소드는 결과당 한번만 실행되어야 한다.
        sResult.mUpdatedCount = DEFAULT_UPDATE_COUNT;

        return sUpdateCount;
    }

    @Override
    public long getLargeMaxRows() throws SQLException
    {
        return mMaxRows;
    }

    @Override
    public void setLargeMaxRows(long aMax) throws SQLException
    {
        throwErrorForClosed();
        if (aMax < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Max rows",
                                    "0 ~ Long.MAX_VALUE",
                                    String.valueOf(aMax));
        }

        mMaxRows = aMax;
    }

    @Override
    public void setPoolable(boolean aPoolable)
    {
        this.mIsPoolable = aPoolable;
    }

    @Override
    public boolean isPoolable()
    {
        return mIsPoolable;
    }

    protected int[] toIntBatchUpdateCounts(long aLongUpdateCounts[])
    {
        int[] sIntUpdateCounts = new int[aLongUpdateCounts.length];
        for (int i = 0; i < aLongUpdateCounts.length; i++)
        {
            sIntUpdateCounts[i] = toInt(aLongUpdateCounts[i]);
        }
        return sIntUpdateCounts;
    }

    protected int toInt(long aUpdateCount)
    {
        return aUpdateCount > Integer.MAX_VALUE ? Statement.SUCCESS_NO_INFO : (int)aUpdateCount;
    }

    public boolean getIsSuccess()
    {
        return mIsSuccess;
    }
}
