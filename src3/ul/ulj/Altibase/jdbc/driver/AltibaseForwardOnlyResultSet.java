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

import java.sql.SQLException;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.cm.CmFetchResult;
import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextDirExec;
import Altibase.jdbc.driver.datatype.RowHandle;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ShardError;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class AltibaseForwardOnlyResultSet extends AltibaseReadableResultSet
{
    private final CmFetchResult mFetchResult;
    private       int           mCacheOutCount;

    // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
    private SemiAsyncPrefetch  mSemiAsyncPrefetch;
    private transient Logger   mAsyncLogger;

    protected AltibaseForwardOnlyResultSet(AltibaseStatement aStatement, CmProtocolContextDirExec aContext, int aFetchSize) throws SQLException
    {
        mContext = aContext;
        mStatement = aStatement;
        mFetchResult = mContext.getFetchResult();
        mFetchSize = aFetchSize;
        mCacheOutCount = 0;

        AltibaseProperties sProperties = aStatement.getAltibaseConnection().getProperties();

        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
        if (sProperties.getFetchAsync().equalsIgnoreCase(AltibaseProperties.PROP_VALUE_FETCH_ASYNC_PREFERRED) &&
            !aStatement.isInternalStatement())
        {
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mAsyncLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_ASYNCFETCH);
            }

            beginFetchAsync();
        }
    }

    public void close() throws SQLException
    {
        if (isClosed())
        {
            return;
        }

        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
        if (mStatement.isAsyncPrefetch())
        {
            endFetchAsync();
        }

        int sResultSetCount = mContext.getPrepareResult().getResultSetCount();

        // PROJ-2427 성능을 위해 close cursor프로토콜을 전송하지 않는다.
        if (mStatement instanceof AltibasePreparedStatement)
        {
            if (sResultSetCount > 1)
            {
                if (mStatement.mCurrentResultIndex == (sResultSetCount - 1))
                {
                    ((AltibasePreparedStatement)mStatement).closeAllResultSet();
                }
                else
                {
                    ((AltibasePreparedStatement)mStatement).closeCurrentResultSet();
                }
            }
            else
            {
                ((AltibasePreparedStatement)mStatement).closeCurrentResultSet();
            }
        }
        else
        {
        }

        super.close();
    }

    protected RowHandle rowHandle()
    {
        return mFetchResult.rowHandle();
    }

    protected List<Column> getTargetColumns()
    {
        return mFetchResult.getColumns();
    }

    public boolean isAfterLast() throws SQLException
    {
        throwErrorForClosed();
        return !mFetchResult.fetchRemains() && mFetchResult.rowHandle().isAfterLast();
    }

    public int getRow() throws SQLException
    {
        if (isBeforeFirst() || isAfterLast())
        {
            return 0;
        }
        return mCacheOutCount + mFetchResult.rowHandle().getPosition();
    }

    public boolean isBeforeFirst() throws SQLException
    {
        throwErrorForClosed();
        return (mCacheOutCount == 0) && (mFetchResult.rowHandle().isBeforeFirst());
    }

    public boolean isFirst() throws SQLException
    {
        throwErrorForClosed();
        return (mCacheOutCount == 0) && (mFetchResult.rowHandle().isFirst());
    }

    public boolean isLast() throws SQLException
    {
        throwErrorForClosed();
        if (mFetchResult.fetchRemains())
        {
            // fetch할게 남아 있으면 최소한 한 row 이상은 있다고 가정한 것이다.
            // 만약 fetch할게 남아 있다고 했지만 실제 fetchNext를 요청했을 때
            // row가 하나도 안오고 fetch end가 올 수 있다면 이 코드는 버그이다.
            return false;
        }
        else
        {
            return mFetchResult.rowHandle().isLast();
        }
    }

    /**
     * 비동기 fetch 를 시작한다.
     */
    private void beginFetchAsync() throws SQLException
    {
        if (mStatement.getAltibaseConnection().setAsyncPrefetchStatement(mStatement))
        {
            int sFetchSize = mFetchSize;

            mSemiAsyncPrefetch = mStatement.getSemiAsyncPrefetch();
            if (mSemiAsyncPrefetch == null)
            {
                mSemiAsyncPrefetch = new SemiAsyncPrefetch(this);
                mStatement.setSemiAsyncPrefetch(mSemiAsyncPrefetch);
            }
            else
            {
                // when re-executes after closing cursor or has multiple result sets in stored procedure
                mSemiAsyncPrefetch.nextResultSet(this);
            }

            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                if (mFetchResult.getResultSetId() == 0)
                {
                    mAsyncLogger.log(Level.INFO, mStatement.getTraceUniqueId() + "fetch async begin : semi-async prefetch");
                }
                else
                {
                    mAsyncLogger.log(Level.INFO, mStatement.getTraceUniqueId() + "fetch async begin : semi-async prefetch (result set id = " + mFetchResult.getResultSetId() + ")");
                }
            }

            if (mSemiAsyncPrefetch.isAutoTuning())
            {
                try
                {
                    mSemiAsyncPrefetch.getAutoTuner().beginAutoTuning();

                    sFetchSize = mSemiAsyncPrefetch.getAutoTuner().doAutoTuning();
                }
                catch (Throwable e)
                {
                    // Auto-tuning 도중 예외 발생시 무시하고, 다시 cursor open 하기 전까지 auto-tuning OFF 시킴.

                    mSemiAsyncPrefetch.getAutoTuner().endAutoTuning();
                }
            }

            CmProtocol.sendFetchNextAsync(mContext, sFetchSize);
        }
    }

    /**
     * Cursor close 시 비동시 fetch 를 종료한다.
     */
    void endFetchAsync() throws SQLException
    {
        if (mSemiAsyncPrefetch.isAsyncSentFetch())
        {
            CmProtocol.receivefetchNextAsync(mContext);
        }

        if (mSemiAsyncPrefetch.isAutoTuning())
        {
            mSemiAsyncPrefetch.getAutoTuner().endAutoTuning();
        }

        mSemiAsyncPrefetch = null;

        mStatement.getAltibaseConnection().clearAsyncPrefetchStatement();

        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mAsyncLogger.log(Level.INFO, mStatement.getTraceUniqueId() + "fetch async end");
        }
    }

    public boolean next() throws SQLException
    {
        throwErrorForClosed();

        cursorMoved();

        boolean sResult = false;

        if (isClosed())
        {
            return sResult;
        }

        while (true)
        {
            sResult = mFetchResult.rowHandle().next();
            if (sResult)
            {
                break;
            }
            else
            {
                if (mFetchResult.fetchRemains())
                {
                    mCacheOutCount += mFetchResult.rowHandle().size();
                    mFetchResult.rowHandle().initToStore(); // 캐시를 모두 비우고 다시
                                                            // 채운다.
                    try
                    {
                        // PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
                        if (mSemiAsyncPrefetch == null)
                        {
                            // synchronous prefetch
                            CmProtocol.fetchNext(mContext, mFetchSize);
                        }
                        else
                        {
                            if (!mSemiAsyncPrefetch.canAutoTuning())
                            {
                                fetchNextAsync();
                            }
                            else
                            {
                                fetchNextAsyncWithAutoTuning();
                            }
                        }
                    }
                    catch (SQLException ex)
                    {
                        AltibaseFailover.trySTF(mStatement.getAltibaseConnection().failoverContext(), ex);
                    }
                    if (mStatement.getProtocolContext().getError() != null)
                    {
                        try
                        {
                            mWarning = Error.processServerError(mWarning, mStatement.getProtocolContext().getError());
                        }
                        finally
                        {
                            // BUG-46790 Exception이 발생하더라도 shard align작업을 수행해야 한다.
                            ShardError.processShardError(mStatement.getMetaConn(), mStatement.getProtocolContext().getError());
                        }
                    }
                    mFetchResult.rowHandle().beforeFirst();
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        // BUG-46513 serverside shard fetch 인 경우 SMN의 값 변화를 체크해야 한다.
        AltibaseShardingConnection sMetaConn = mStatement.getMetaConn();
        if (sMetaConn != null && !sResult && sMetaConn.getAutoCommit() &&
            sMetaConn.shouldUpdateShardMetaNumber())
        {
            sMetaConn.updateShardMetaNumber();
        }

        return sResult;
    }

    /**
     * 비동기적으로 fetch 를 수행한다.
     * 만약, 비동기로 send 하지 않았다면 동기로 송수신하고 비동기로 송신해 놓음.
     */
    private void fetchNextAsync() throws SQLException
    {
        boolean sIsAsyncPrefetch = true;

        // synchronous prefetch, if already read by other protocol
        if (!mSemiAsyncPrefetch.isAsyncSentFetch())
        {
            CmProtocol.sendFetchNextAsync(mContext, mFetchSize);

            sIsAsyncPrefetch = false;
        }

        CmProtocol.receivefetchNextAsync(mContext);

        /* check to close the server cursor */
        if (mFetchResult.fetchRemains())
        {
            if (sIsAsyncPrefetch)
            {
                mSemiAsyncPrefetch.logStatHeuristic();
            }

            CmProtocol.sendFetchNextAsync(mContext, mFetchSize);
        }
    }

    /**
     * 비동기적으로 fetch 를 수행하는데 prefetch 양을 auto-tuning 한다. 최적의 prefetch 양은 network idle time 이 0 에 근사함.
     * 만약, auto-tuning 도중 에외가 발생하면 OFF 시키고 이후부터는 비동기로만 동작됨.
     */
    private void fetchNextAsyncWithAutoTuning() throws SQLException
    {
        int sFetchSize = mFetchSize;
        boolean sIsAsyncPrefetch = true;

        // synchronous prefetch, if already read by other protocol
        if (!mSemiAsyncPrefetch.isAsyncSentFetch())
        {
            CmProtocol.sendFetchNextAsync(mContext, sFetchSize);

            sIsAsyncPrefetch = false;
        }
        else
        {
            mSemiAsyncPrefetch.getAutoTuner().doAutoTuningBeforeReceive();
        }

        CmProtocol.receivefetchNextAsync(mContext);

        /* check to close the server cursor */
        if (!mFetchResult.fetchRemains())
        {
            mSemiAsyncPrefetch.getAutoTuner().endAutoTuning();
        }
        else
        {
            if (sIsAsyncPrefetch)
            {
                try
                {
                    sFetchSize = mSemiAsyncPrefetch.getAutoTuner().doAutoTuning();
                }
                catch (Throwable e)
                {
                    // Auto-tuning 도중 예외 발생시 무시하고, 다시 cursor open 하기 전까지 auto-tuning OFF 시킴.

                    mSemiAsyncPrefetch.getAutoTuner().endAutoTuning();
                }
            }
            else
            {
                mSemiAsyncPrefetch.getAutoTuner().skipAutoTuning();
            }

            CmProtocol.sendFetchNextAsync(mContext, sFetchSize);
        }
    }

    /**
     * TC 에서 접근하기 위한 메소드
     */
    private boolean isAutoTuning()
    {
        if (mSemiAsyncPrefetch == null)
        {
            return false;
        }

        return mSemiAsyncPrefetch.isAutoTuning();
    }

    public boolean absolute(int aRow) throws SQLException
    {
        throwErrorForForwardOnly();
        return false;
    }

    public void afterLast() throws SQLException
    {
        throwErrorForForwardOnly();
    }

    public void beforeFirst() throws SQLException
    {
        throwErrorForForwardOnly();
    }

    public boolean previous() throws SQLException
    {
        throwErrorForForwardOnly();
        return false;
    }

    public int getType() throws SQLException
    {
        throwErrorForClosed();
        return TYPE_FORWARD_ONLY;
    }

    public void refreshRow() throws SQLException
    {
        throwErrorForForwardOnly();
    }

    int size()
    {
        if (mFetchResult.fetchRemains())
        {
            return Integer.MAX_VALUE;
        }
        else
        {
            return mCacheOutCount + mFetchResult.rowHandle().size();
        }
    }
}
