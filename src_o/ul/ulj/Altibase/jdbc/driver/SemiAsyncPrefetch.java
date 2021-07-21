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
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.cm.CmSocket;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.util.AltibaseProperties;

/**
 * 비동기 fetch 방식 중 하나로서, TCP kernel buffer 을 이용해 double buffering 하는 semi-async prefetch 방식.
 * (PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning)
 */
class SemiAsyncPrefetch
{
    private AltibaseForwardOnlyResultSet mResultSet;
    private AltibaseStatement            mStatement;
    private AltibaseConnection           mConnection;
    private AltibaseProperties           mProperties;

    private SemiAsyncPrefetchAutoTuner   mAutoTuner;
    private JniExt                       mJniExt;
    private int                          mSocketFD = CmSocket.INVALID_SOCKTFD;

    private transient Logger             mLogger;
    private transient Logger             mAsyncLogger;

    SemiAsyncPrefetch(AltibaseForwardOnlyResultSet aResultSet) throws SQLException
    {
        mResultSet   = aResultSet;
        mStatement   = aResultSet.getAltibaseStatement();
        mConnection  = mStatement.getAltibaseConnection();
        mProperties  = mConnection.getProperties();

        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
            mAsyncLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_ASYNCFETCH);

            if (checkJniExtModule())
            {
                try
                {
                    mSocketFD = mConnection.channel().getSocketFD();
                }
                catch (Exception e)
                {
                    // Trace logging 용도로 socket file descriptor 를 구하기 때문에 예외 발생하여도 무시함.
                    mAsyncLogger.log(Level.WARNING, mStatement.getTraceUniqueId() + e);
                }
            }
        }

        if (checkAutoTuning())
        {
            try
            {
                mAutoTuner = new SemiAsyncPrefetchAutoTuner(aResultSet, mJniExt);
            }
            catch (Exception e)
            {
                // SemiAsyncPrefetchAutoTuner 객체 생성시 예외 발생하면 auto-tuning 기능을 OFF 시키고 예외 무시함.

                if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
                {
                    mAsyncLogger.log(Level.WARNING, mStatement.getTraceUniqueId() + e);
                }
            }
        }
    }

    /**
     * 비동기 prefetch 하고자 하는 다음 result set 을 설정한다.
     * e.g.) 둘 이상의 result set 갖는 프로시져 수행, close cursor 이후 execute 재수행
     */
    void nextResultSet(AltibaseForwardOnlyResultSet aResultSet)
    {
        mResultSet = aResultSet;

        if (mAutoTuner != null)
        {
            mAutoTuner.nextResultSet(aResultSet);
        }
    }

    /**
     * 비동기 fetch 프로토콜을 송신하였는지 여부를 확인한다.
     */
    boolean isAsyncSentFetch()
    {
        return mConnection.channel().isAsyncSent();
    }

    /**
     * Auto-tuning 설정 여부를 확인한다.
     */
    boolean isAutoTuning()
    {
        return (mAutoTuner != null);
    }

    /**
     * Auto-tuning 을 동작할 수 있는지 여부를 확인한다. 만약, auto-tuning 도중 실패한다면 다시 cursor 를 open 하기 전까지 OFF 된다.
     */
    boolean canAutoTuning()
    {
        if (mAutoTuner == null)
        {
            return false;
        }

        return mAutoTuner.canAutoTuning();
    }

    /**
     * Auto-tuning 이 동작될 경우 SemiAsyncPrefetchAutoTuner 객체를 반환한다.
     */
    SemiAsyncPrefetchAutoTuner getAutoTuner()
    {
        return mAutoTuner;
    }

    /**
     * Auto-tuning 기능을 동작할 수 있는지 검사하여 JNI ext. module 을 load 시킨다.
     */
    private boolean checkAutoTuning()
    {
        // check 'FetchAutoTuning' property
        if (!mProperties.isFetchAutoTuning())
        {
            return false;
        }

        return checkJniExtModule();
    }

    /**
     * JNI ext. module 을 load 시킨다.
     */
    private boolean checkJniExtModule()
    {
        if (!JniExtLoader.checkLoading())
        {
            return false;
        }

        mJniExt = new JniExt();

        return true;
    }

    /**
     * Heuristic 한 semi-async prefetch 동작에 대해 trace logging 한다.
     */
    void logStatHeuristic()
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            if (JniExtLoader.checkLoading() &&
                mSocketFD != CmSocket.INVALID_SOCKTFD)
            {
                try
                {
                    long sNetworkIdleTime = mJniExt.getTcpiLastDataRecv(mSocketFD);

                    int sFetchSize = mResultSet.getFetchSize();

                    mAsyncLogger.log(Level.INFO, mStatement.getTraceUniqueId() + "fetch size = " + sFetchSize + ", r = " + sNetworkIdleTime);
                }
                catch (Throwable e)
                {
                    // ignore exceptions

                    mAsyncLogger.log(Level.WARNING, e.toString());
                }
            }
        }
    }
}
