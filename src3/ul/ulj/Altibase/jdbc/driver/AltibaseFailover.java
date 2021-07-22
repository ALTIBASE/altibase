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

import java.sql.Connection;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.cm.CmOperation;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.ex.ShardFailOverSuccessException;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection;
import Altibase.jdbc.driver.sharding.core.AltibaseShardingFailover;
import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardNodeConfig;
import Altibase.jdbc.driver.util.AltibaseProperties;
import Altibase.jdbc.driver.util.StringUtils;

import static Altibase.jdbc.driver.util.AltibaseProperties.PROP_SHARD_PIN;

public final class AltibaseFailover
{
    public enum CallbackState
    {
        STOPPED,
        IN_PROGRESS,
    }

    /* BUG-31390 Failover info for v$session */
    public enum FailoverType
    {
        CTF,
        STF,
    }

    private static Logger    mLogger;

    static
    {
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_FAILOVER);
        }
    }

    private AltibaseFailover()
    {
    }

    /**
     * CTF를 시도하고, 결과에 따라 경고를 남기거나 원래났던 예외를 다시 던진다.
     *
     * @param aContext Failover context
     * @param aWarning 경고를 담을 객체. null이면 새 객체를 만든다.
     * @param aException 원래 났던 예외
     * @return 경고를 담은 객체
     * @throws SQLException CTF를 할 상황이 아니거나 CTF에 실패한 경우
     */
    static SQLWarning tryCTF(AltibaseFailoverContext aContext, SQLWarning aWarning, SQLException aException) throws SQLException
    {
        if (!AltibaseFailover.checkNeedCTF(aContext, aException))
        {
            throw aException;
        }
        AltibaseConnection sConn = aContext.getConnection();
        if (!AltibaseFailover.doCTF(aContext))
        {
            /* BUG-47145 node connection이고 lazy모드일때 ctf에 실패한 경우
               ShardFailoverIsNotAvailableException을 올린다. */
            if (sConn.isNodeConnection() && sConn.getMetaConnection().isLazyNodeConnect())
            {
                CmOperation.throwShardFailoverIsNotAvailableException(sConn.getNodeName(),
                                                                      sConn.getServer(),
                                                                      sConn.getPort());
            }
            throw aException;
        }
        // BUG-46790 shard connection인 경우에는 notify failover 메세지를 보낸다.
        if (sConn.isShardConnection())
        {
            AltibaseShardingConnection sMetaCon = sConn.getMetaConnection();
            AltibaseShardingFailover sShardFailover = sMetaCon.getShardFailover();
            sShardFailover.notifyFailover(sConn);

            /* BUG-47145 node connection에서 CTF가 발생했을때 autocommit false, lazy node connect 상태라면
               STF success를 올려야 한다. */
            if (sConn.isNodeConnection() && !sMetaCon.getAutoCommit() && sMetaCon.isLazyNodeConnect())
            {
                ShardNodeConfig sNodeConfg = sMetaCon.getShardNodeConfig();
                DataNode sNode = sNodeConfg.getNodeByName(sConn.getNodeName());
                Map<DataNode, Connection> sCachedMap = sMetaCon.getCachedConnections();
                // BUG-47145 CTF로 복구된 connection을 cached node connection map에 저장한다.
                sCachedMap.put(sNode, sConn);
                throw new ShardFailOverSuccessException(aException.getMessage(),
                                                        ErrorDef.FAILOVER_SUCCESS,
                                                        sConn.getNodeName(), sConn.getServer(), sConn.getPort());
            }
        }

        /* BUG-32917 Connection Time Failover(CTF) information should be logged */
        aWarning = Error.createWarning(aWarning, ErrorDef.FAILOVER_SUCCESS, aException.getSQLState(), aException.getMessage(), aException);
        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger.log(Level.WARNING, "Failover completed");
        }

        return aWarning;
    }

    /**
     * STF를 시도하고, 결과에 따라 예외를 던진다.
     * 성공했을때도 예외를 던져 STF 성공을 알린다.
     * <p>
     * 만약 failover context가 초기화되지 않았다면 원래 났던 예외를 던지거나 사용자가 넘긴 경고 객체를 그대로 반환한다.
     *
     * @param aContext Failover context
     * @param aException 원래 났던 예외
     * @throws SQLException STF에 실패했다면 원래 났던 예외, 아니면 STF 성공을 알리는 예외
     */
    public static void trySTF(AltibaseFailoverContext aContext, SQLException aException) throws SQLException
    {
        if (!AltibaseFailover.checkNeedSTF(aContext, aException))
        {
            throw aException;
        }
        AltibaseFailoverServerInfo sOldServerInfo = aContext.getCurrentServer();
        boolean sSTFSuccess = AltibaseFailover.doSTF(aContext);
        AltibaseConnection sConn = aContext.getConnection();
        /* BUG-46790 meta connection 이고 autocommit이 false 인 경우에는 stf성공여부에 상관없이
           전노드들에 touch를 해준다. */
        AltibaseProperties sProps = aContext.connectionProperties();
        if (sConn.isMetaConnection() && !sProps.isAutoCommit())
        {
            AltibaseShardingFailover sShardFailover = sConn.getMetaConnection().getShardFailover();
            sShardFailover.setTouchedToAllNodes();
        }
        if (sSTFSuccess)
        {
            // BUG-46790 shard connection(meta, node)인 경우에는 ShardFailOverSuccessException을 던진다.
            if (sConn.isShardConnection())
            {
                throw new ShardFailOverSuccessException(aException.getMessage(),
                                                        ErrorDef.FAILOVER_SUCCESS,
                                                        sConn.getNodeName(),
                                                        sConn.getServer(),
                                                        sOldServerInfo.getPort());
            }
            Error.throwSQLExceptionForFailover(aException);
        }
        else
        {
            // BUG-47145 node connection이고 stf에 실패한 경우 ShardFailoverIsNotAvailableException을 올린다.
            if (sConn.isNodeConnection())
            {
                CmOperation.throwShardFailoverIsNotAvailableException(sConn.getNodeName(),
                                                                      sConn.getServer(),
                                                                      sConn.getPort());
            }
            throw aException;
        }
    }

    /**
     * CTF를 위해 alternateservers에서 사용 가능한 서버를 선택해 접속한다.
     *
     * @param aContext Failover context
     * @return CTF 성공 여부
     */
    private static boolean doCTF(AltibaseFailoverContext aContext)
    {
        return doCTF(aContext, FailoverType.CTF);
    }

    /**
     * shard failover align을 할때 수행되며 특정 서버로 CTF를 시도한다.
     * @param aContext Failover context 객체
     * @param aFailoverType Failover 타입
     * @param aNewServerInfo 접속할 서버
     * @return aNewServerInfo 서버로의 CTF 성공여부
     */
    public static boolean doCTF(AltibaseFailoverContext aContext, FailoverType aFailoverType,
                                AltibaseFailoverServerInfo aNewServerInfo)
    {

        return connectToAlternateServer(aContext, aFailoverType, aContext.connectionProperties(), aNewServerInfo);

    }

    /**
     * CTF 또는 STF를 위해 alternateservers에서 사용 가능한 서버를 선택해 접속한다.
     *
     * @param aContext Failover context
     * @param aFailoverType Failover 유형
     * @return CTF 성공 여부
     */
    private static boolean doCTF(AltibaseFailoverContext aContext, FailoverType aFailoverType)
    {
        final AltibaseProperties sProp = aContext.connectionProperties();
        final AltibaseFailoverServerInfo sOldServerInfo = aContext.getCurrentServer();

        List<AltibaseFailoverServerInfo> sTryList = aContext.getFailoverServerList().getList();
        if (sProp.useLoadBalance())
        {
            Collections.shuffle(sTryList);
        }
        if (sTryList.get(0) != sOldServerInfo)
        {
            sTryList.remove(sOldServerInfo);
            sTryList.add(0, sOldServerInfo);
        }
        for (AltibaseFailoverServerInfo sNewServerInfo : sTryList)
        {
            if (connectToAlternateServer(aContext, aFailoverType, sProp, sNewServerInfo))
            {
                return true;
            }
        }

        return false;
    }

    public static boolean connectToAlternateServer(AltibaseFailoverContext aContext,
                                                    FailoverType aFailoverType, 
                                                    AltibaseProperties aProp, 
                                                    AltibaseFailoverServerInfo aNewServerInfo)
    {
        int sMaxConnectionRetry = aProp.getConnectionRetryCount();
        int sConnectionRetryDelay = aProp.getConnectionRetryDelay() * 1000; // millisecond
        AltibaseFailoverServerInfo sOldServerInfo = aContext.getCurrentServer();
        
        for (int sRemainRetryCnt = Math.max(1, sMaxConnectionRetry); sRemainRetryCnt > 0; sRemainRetryCnt--)
        {
            /* BUG-43219 매 접속시마다 채널을 재생성해야 한다. 그렇지 않으면 첫번째 접속 정보가 뒤에오는 접속에 영향을 끼친다. */
            CmChannel sNewChannel = new CmChannel();
            try
            {
                sNewChannel.open(aNewServerInfo.getServer(),
                                 aProp.getSockBindAddr(),
                                 aNewServerInfo.getPort(),
                                 aProp.getLoginTimeout());
                setFailoverSource(aContext, aFailoverType, sOldServerInfo);
                aContext.setCurrentServer(aNewServerInfo);
                AltibaseFailover.changeChannelAndConnect(aContext, sNewChannel, aNewServerInfo);
                if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
                {
                    mLogger.log(Level.INFO, "Success to connect to (" + aNewServerInfo + ")");
                }
                
                return true;
            }
            catch (Exception aEx)
            {
                if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
                {
                    mLogger.log(Level.INFO, "Failure to connect to (" + aNewServerInfo + ")", aEx);
                    mLogger.log(Level.INFO, "Sleep " + sConnectionRetryDelay);
                }
                try
                {
                    Thread.sleep(sConnectionRetryDelay);
                }
                catch (InterruptedException sExSleep)
                {
                    // ignore
                }
            }
        }

        return false;
    }

    /**
     * <tt>CmChannel</tt>을 바꾸고, 그를 이용해 <tt>connect</tt>를 시도한다.
     *
     * @param aContext Failover contest
     * @param aChannel 바꿀 <tt>CmChannel</tt>
     * @param aServerInfo AltibaseFailoverServerInfo 정보
     * @throws SQLException 새 <tt>CmChannel</tt>을 이용하 <tt>connect</tt>에 실패한 경우
     */
    private static void changeChannelAndConnect(AltibaseFailoverContext aContext, CmChannel aChannel,
                                                AltibaseFailoverServerInfo aServerInfo) throws SQLException
    {
        AltibaseConnection sConn = aContext.getConnection();
        AltibaseProperties sProps = aContext.connectionProperties();
        sConn.setChannel(aChannel);
        sConn.handshake();
        if (sConn.isShardConnection())
        {
            sConn.shardHandshake();
            if (sConn.isMetaConnection())  // BUG-46790 meta connection인 경우에는 CTF 성공후 channel객체를 교체해 준다.
            {
                AltibaseShardingConnection sMetaConn = sConn.getMetaConnection();
                sMetaConn.setChannel(aChannel);
                long sShardPin = sMetaConn.getShardContextConnect().getShardPin();
                // BUG-46790 meta node인 경우 channel을 교체한 후 기존 shardpin 값을 다시 셋팅해 준다.
                sProps.put(PROP_SHARD_PIN, String.valueOf(sShardPin));
            }
        }
        sConn.setClosed(false);
        sProps.setDatabase(aServerInfo.getDatabase());
        sProps.setAutoCommit(sConn.getAutoCommit());
        sConn.connect(sProps);
    }

    /**
     * CTF와 Failover callback을 수행한다.
     * callback 결과에 따라 STF를 계속 진행할지 그만둘지를 결정한다.
     *
     * @param aContext Failover context
     * @return STF 성공 여부
     * @throws SQLException STF를 위해 <tt>Statement</tt>를 정리하다가 실패한 경우
     */
    private static boolean doSTF(AltibaseFailoverContext aContext) throws SQLException
    {
        int sFailoverIntention;
        boolean sFOSuccess;
        int sFailoverEvent;
        AltibaseFailoverCallback sFailoverCallback = aContext.getCallback();
        AltibaseConnection sConn = aContext.getConnection();

        // BUG-46790 node connection에서 발생한 STF일 경우에는 dummy callback으로 셋팅한다.
        if (sConn.isNodeConnection() && !(sFailoverCallback instanceof AltibaseFailoverCallbackDummy))
        {
            sFailoverCallback = new AltibaseFailoverCallbackDummy();
            aContext.setCallback(sFailoverCallback);
        }

        aContext.setCallbackState(CallbackState.IN_PROGRESS);
        sFailoverIntention = sFailoverCallback.failoverCallback(aContext.getConnection(),
                                                                aContext.getAppContext(),
                                                                AltibaseFailoverCallback.Event.BEGIN);
        aContext.setCallbackState(CallbackState.STOPPED);

        if (sFailoverIntention == AltibaseFailoverCallback.Result.QUIT)
        {
            sFOSuccess = false;
        }
        else
        {
            sFOSuccess = doCTF(aContext, FailoverType.STF);
            sConn.clearStatements4STF();

            if (sFOSuccess)
            {
                // BUG-46790 shard connection인 경우에는 notify failover 메세지를 보낸다.
                if (aContext.getConnection().isShardConnection())
                {
                    notifyShardFailover(aContext);
                }
                aContext.setCallbackState(CallbackState.IN_PROGRESS);
                try
                {
                    AltibaseXAResource sXAResource = aContext.getRelatedXAResource();
                    if ((sXAResource != null) && (sXAResource.isOpen()))
                    {
                        sXAResource.xaOpen();
                    }
                    sFailoverEvent = AltibaseFailoverCallback.Event.COMPLETED;
                }
                catch (SQLException e)
                {
                    sFailoverEvent = AltibaseFailoverCallback.Event.ABORT;
                }
                sFailoverIntention = sFailoverCallback.failoverCallback(aContext.getConnection(), aContext.getAppContext(), sFailoverEvent);
                aContext.setCallbackState(CallbackState.STOPPED);

                if ((sFailoverEvent == AltibaseFailoverCallback.Event.ABORT) ||
                    (sFailoverIntention == AltibaseFailoverCallback.Result.QUIT))
                {
                    sFOSuccess = false;
                    aContext.getConnection().quiteClose();
                }
            }
            else
            {
                aContext.setCallbackState(CallbackState.IN_PROGRESS);
                sFailoverCallback.failoverCallback(aContext.getConnection(), aContext.getAppContext(), AltibaseFailoverCallback.Event.ABORT);
                aContext.setCallbackState(CallbackState.STOPPED);
                // BUG-46790 CTF에 실패한 경우 close flag를 셋팅해 준다.
                sConn.setClosed(true);
            }
        }
        return sFOSuccess;

    }

    private static void notifyShardFailover(AltibaseFailoverContext aContext) throws SQLException
    {
        AltibaseShardingConnection sMetaCon = aContext.getConnection().getMetaConnection();
        AltibaseShardingFailover sShardFailover = sMetaCon.getShardFailover();
        sShardFailover.updateShardNodeList(aContext.getConnection());
        sShardFailover.notifyFailover(aContext.getConnection());
    }

    /* BUG-31390 Failover info for v$session */
    /**
     * V$SESSION.FAILOVER_SOURCE로 보여줄 값을 생성해 Connection property로 설정한다.
     *
     * @param aContext Failover Context
     * @param aFailoverType Failover 유형
     * @param aServerInfo 설정할 서버 정보
     */
    private static void setFailoverSource(AltibaseFailoverContext aContext, FailoverType aFailoverType, AltibaseFailoverServerInfo aServerInfo)
    {
        StringBuilder sStrBdr = new StringBuilder();

        switch (aFailoverType)
        {
            case CTF:
                sStrBdr.append("CTF ");
                break;

            case STF:
                sStrBdr.append("STF ");
                break;

            default:
                // 내부에서만 쓰므로, 이런일은 절대 일어나선 안된다.
                Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION);
                break;
        }

        sStrBdr.append(aServerInfo.getServer());
        sStrBdr.append(':');
        sStrBdr.append(aServerInfo.getPort());
        if (!StringUtils.isEmpty(aServerInfo.getDatabase()))
        {
            sStrBdr.append('/');
            sStrBdr.append(aServerInfo.getDatabase());
        }
        aContext.setFailoverSource(sStrBdr.toString());
    }

    /**
     * CTF가 필요한지 확인한다.
     *
     * @param aContext Failover에 대한 정보를 담은 객체
     * @param aException CTF가 필요한지 볼 예외
     * @return CTF가 필요하면 true, 아니면 false
     */
    private static boolean checkNeedCTF(AltibaseFailoverContext aContext, SQLException aException)
    {
        return (aContext != null) &&
               (aContext.getFailoverServerList().size() > 0) &&
               (isNeedToFailover(aException));
    }

    /**
     * STF가 필요한지 확인한다.
     *
     * @param aContext Failover에 대한 정보를 담은 객체
     * @param aException STF가 필요한지 볼 예외
     * @return STF가 필요하면 true, 아니면 false
     */
    private static boolean checkNeedSTF(AltibaseFailoverContext aContext, SQLException aException)
    {
        return (aContext != null) &&
               (aContext.useSessionFailover()) &&
               (aContext.getFailoverServerList().size() > 0) &&
               (isNeedToFailover(aException)) &&
               (aContext.getCallbackState() == CallbackState.STOPPED);
    }

    /**
     * Failover가 필요한 예외인지 확인한다.
     *
     * @param aException Failover가 필요한지 볼 예외
     * @return Failover가 필요하면 true, 아니면 false
     */
    public static boolean isNeedToFailover(SQLException aException)
    {
        switch (aException.getErrorCode())
        {
            case ErrorDef.COMMUNICATION_ERROR:
            case ErrorDef.UNKNOWN_HOST:
            case ErrorDef.CLIENT_UNABLE_ESTABLISH_CONNECTION:
            case ErrorDef.CLOSED_CONNECTION:
            case ErrorDef.RESPONSE_TIMEOUT:
            case ErrorDef.ADMIN_MODE_ERROR:
                return true;
            default:
                return false;
        }
    }
}
