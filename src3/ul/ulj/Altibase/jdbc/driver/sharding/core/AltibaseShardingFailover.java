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
import Altibase.jdbc.driver.cm.CmOperation;
import Altibase.jdbc.driver.cm.CmProtocolContextShardConnect;
import Altibase.jdbc.driver.cm.CmShardProtocol;
import Altibase.jdbc.driver.ex.*;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.util.AltibaseProperties;

import java.sql.Connection;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

/**
 * 샤딩과 관련된 Failover 동작을 수행한다.
 */
public class AltibaseShardingFailover
{
    private AltibaseShardingConnection    mMetaConn;

    public AltibaseShardingFailover(AltibaseShardingConnection aMetaConn)
    {
        mMetaConn = aMetaConn;
    }

    public void notifyFailover(AltibaseConnection aConn)
    {
        if (aConn.isNodeConnection())
        {
            notifyFailoverOnDataNode(aConn);
        }
        else if (aConn.isMetaConnection())
        {
            notifyFailoverOnMeta(aConn);
        }
    }

    private void notifyFailoverOnMeta(AltibaseConnection aConn)
    {
        AltibaseShardingConnection sMetaConn = aConn.getMetaConnection();
        TreeMap<DataNode, Connection> sSortedNodeConnMap = new TreeMap<DataNode, Connection> (sMetaConn.getCachedConnections());
        /* BUG-46790 meta에서 failover가 발생한 경우 모든 데이터노드에서 notify를 수행해야 한다. 데이터 노드에 대한 연결은
           touch 시점에 모두 확보된다. */
        for (Connection sEach : sSortedNodeConnMap.values())
        {
            notifyFailoverOnDataNode((AltibaseConnection)sEach);
        }
    }

    private void notifyFailoverOnDataNode(AltibaseConnection aConn)
    {
        NodeConnectionReport sReport = getNodeConnectionReport(aConn);
        try
        {
            // BUG-46790 meta connection 이 close된 경우에는 connection report를 보내지 않는다.
            Connection sMetaConn = aConn.getMetaConnection();
            if (!sMetaConn.isClosed())
            {
                sReport.setNodeReportType(NodeConnectionReport.NodeReportType.SHARD_NODE_REPORT_TYPE_CONNECTION);
                sendNodeConnectionReport(sReport, mMetaConn.getShardProtocol());
            }
            // BUG-47145 ctf시점에는 autocommit여부가 아직 전파되기 전이기 때문에 metaConn의 autocommit여부로 비교한다.
            if (!sMetaConn.getAutoCommit())
            {
                mMetaConn.getShardContextConnect().setTouched(true);
            }
        }
        catch (SQLException aEx)
        {
            try
            {
                AltibaseFailover.trySTF(mMetaConn.getMetaConnection().failoverContext(), aEx);
            }
            catch (SQLException aSqlEx)
            {
                // BUG-46790 notify 도중 STF가 발생하면 결과에 상관없이 예외로 처리하지 않고 로그만 남긴다.
                shard_log(Level.INFO, aSqlEx.getMessage(), aSqlEx);
            }
        }
    }

    private void sendNodeConnectionReport(NodeConnectionReport aReport,
                                          CmShardProtocol aShardProtocol) throws SQLException
    {
        CmProtocolContextShardConnect sShardContextConnect = mMetaConn.getShardContextConnect();
        sShardContextConnect.clearProperties();
        aShardProtocol.shardNodeReport(aReport);
        if (sShardContextConnect.getError() != null)
        {
            SQLWarning sWarning = mMetaConn.getWarnings();
            sWarning = Error.processServerError(sWarning, sShardContextConnect.getError());
            mMetaConn.setWarning(sWarning);
        }
    }

    private NodeConnectionReport getNodeConnectionReport(AltibaseConnection aConn)
    {
        NodeConnectionReport sReport = new NodeConnectionReport();
        String sNodeName = aConn.getNodeName();
        DataNode sNode = mMetaConn.getShardNodeConfig().getNodeByName(sNodeName);
        AltibaseFailoverContext sFailoverContext = aConn.failoverContext();

        if (sFailoverContext == null)
        {
            // BUG-46790 alternate server 설정이 없는 경우에는 active로 접속한다.
            sReport.setDestination(DataNodeFailoverDestination.ACTIVE);
        }
        else
        {
            AltibaseFailoverServerInfo sCurServerInfo = sFailoverContext.getCurrentServer();
            if (sCurServerInfo.getServer().equals(sNode.getServerIp()) &&
                sCurServerInfo.getPort() == sNode.getPortNo())
            {
                sReport.setDestination(DataNodeFailoverDestination.ACTIVE);
            }
            else
            {
                sReport.setDestination(DataNodeFailoverDestination.ALTERNATE);
            }
        }

        sReport.setNodeId(sNode.getNodeId());

        return sReport;
    }

    public void updateShardNodeList(AltibaseConnection aConn) throws SQLException
    {
        if (aConn.isMetaConnection())
        {
            AltibaseShardingConnection sShardCon = aConn.getMetaConnection();
            CmShardProtocol sShardProtocol = sShardCon.getShardProtocol();
            AltibaseProperties sProps = sShardCon.getProps();
            try
            {
                sShardProtocol.updateNodeList();
                // BUG-46790 node list를 갱신한 후에는 cache map에 있는 DataNode객체들도 갱신해야 한다.
                ShardNodeConfig sShardNodeConfig = sShardCon.getShardNodeConfig();
                Map<DataNode, Connection> sOldCachedConnMap = sShardCon.getCachedConnections();
                Map<DataNode, Connection> sNewCachedConnMap = new ConcurrentHashMap<DataNode, Connection>();
                for (Map.Entry<DataNode, Connection> sEntry : sOldCachedConnMap.entrySet())
                {
                    DataNode sOldNode = sEntry.getKey();
                    Connection sConn = sEntry.getValue();
                    DataNode sNewNode = sShardNodeConfig.getNodeByName(sOldNode.getNodeName());
                    if (sNewNode != null)
                    {
                        sNewCachedConnMap.put(sNewNode, sConn);
                    }
                }
                sShardCon.setCachedConnections(sNewCachedConnMap);
            }
            catch (SQLException aEx)
            {
                AltibaseFailover.trySTF(aConn.failoverContext(), aEx);
            }
            CmProtocolContextShardConnect sShardConnectContext = sShardCon.getShardContextConnect();
            if (sShardConnectContext.getError() == null && aConn.isNodeConnection())
            {
                sShardConnectContext.createDataSources(sProps);
            }
        }
    }

    public void alignDataNodeConnection(AltibaseConnection aNodeConn,
                                        FailoverAlignInfo aAlignInfo) throws SQLException
    {
        if (aNodeConn.failoverContext() != null)
        {
            AltibaseFailoverContext sFailoverContext = aNodeConn.failoverContext();
            AltibaseFailoverServerInfo sOldServerInfo = sFailoverContext.getCurrentServer();
            setNextFailoverServer(sFailoverContext, sOldServerInfo);

            AltibaseFailoverServerInfo sNewServerInfo = sFailoverContext.getCurrentServer();
            boolean sFOResult = failoverConnectToSpecificServer(sFailoverContext, sNewServerInfo);

            try
            {
                if (sFOResult)
                {
                    throw new ShardFailOverSuccessException(aAlignInfo.getMessageText(),
                                                            ErrorDef.FAILOVER_SUCCESS,
                                                            aNodeConn.getNodeName(),
                                                            aNodeConn.getServer(),
                                                            sOldServerInfo.getPort());
                }
                else
                {
                    throw new ShardFailoverIsNotAvailableException(aAlignInfo.getMessageText(),
                                                                   ErrorDef.SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE,
                                                                   aNodeConn.getNodeName(),
                                                                   aNodeConn.getServer(),
                                                                   sOldServerInfo.getPort());
                }
            }
            finally
            {
                aAlignInfo.setNeedAlign(false);
            }
        }
        else
        {
            // BUG-46790 node에 failover설정이 없다면 에러를 발생시킨다.
            throw new ShardFailoverIsNotAvailableException(aAlignInfo.getMessageText(),
                                                           ErrorDef.SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE);
        }
    }

    private boolean failoverConnectToSpecificServer(AltibaseFailoverContext aContext,
                                                    AltibaseFailoverServerInfo aNewServerInfo) throws SQLException
    {
        shard_log("[failoverConnectToSpecificServer] Server: {0}:{1}",
                  new Object[] { aNewServerInfo.getServer(), aNewServerInfo.getPort() });

        boolean sFOSuccess = AltibaseFailover.doCTF(aContext, AltibaseFailover.FailoverType.STF,
                                                    aNewServerInfo);
        if (sFOSuccess)
        {
            aContext.getConnection().clearStatements4STF();
            // BUG-46790 shard connection인 경우에는 notify failover 메세지를 보낸다.
            if (aContext.getConnection().isShardConnection())
            {
                updateShardNodeList(aContext.getConnection());
                notifyFailover(aContext.getConnection());
            }
            AltibaseXAResource sXAResource = aContext.getRelatedXAResource();
            if ((sXAResource != null) && (sXAResource.isOpen()))
            {
                aContext.setCallbackState(AltibaseFailover.CallbackState.IN_PROGRESS);
                sXAResource.xaOpen();
                aContext.setCallbackState(AltibaseFailover.CallbackState.STOPPED);
            }
        }
        else
        {
            shard_log(Level.SEVERE, "FailoverConnectToSpecificServer failed : {0}", aNewServerInfo);
        }

        return sFOSuccess;
    }

    private void setNextFailoverServer(AltibaseFailoverContext aFailoverContext,
                                       AltibaseFailoverServerInfo aOldServerInfo)
    {
        AltibaseFailoverServerInfoList sFailoverServerList = aFailoverContext.getFailoverServerList();

        for (AltibaseFailoverServerInfo sServerInfo : sFailoverServerList.getList())
        {
            if (sServerInfo == aOldServerInfo)
            {
                continue;
            }
            aFailoverContext.setCurrentServer(sServerInfo);

            break;
        }
    }

    public void setTouchedToAllNodes() throws SQLException
    {
        ShardNodeConfig sNodeConfig = mMetaConn.getShardContextConnect().getShardNodeConfig();
        sNodeConfig.setTouchedToAllNodes();
        for (DataNode sNode : sNodeConfig.getDataNodes())
        {
            // BUG-46790 노드커넥션이 없는 경우에는 새로 생성시킨다.
            if (!mMetaConn.getCachedConnections().containsKey(sNode))
            {
                mMetaConn.getNodeConnection(sNode);
            }
        }
    }

    private void tryReconnect(AltibaseConnection aNodeConn) throws ShardJdbcException
    {
        AltibaseProperties sProps = aNodeConn.getProp();
        AltibaseFailoverServerInfoList sServerList = new AltibaseFailoverServerInfoList();
        AltibaseFailoverServerInfo sPrimaryServerInfo = new AltibaseFailoverServerInfo(sProps.getServer(),
                                                                                       sProps.getPort(),
                                                                                       sProps.getDatabase());
        sServerList.add(0, sPrimaryServerInfo);

        AltibaseFailoverContext sFailoverContext = new AltibaseFailoverContext(aNodeConn, sProps, sServerList);
        sFailoverContext.setCurrentServer(sPrimaryServerInfo);

        boolean sResult = AltibaseFailover.connectToAlternateServer(sFailoverContext,
                                                                    AltibaseFailover.FailoverType.CTF,
                                                                    sProps,
                                                                    sPrimaryServerInfo);
        if (sResult)
        {
            throw new ShardFailOverSuccessException("Reconnect Success", ErrorDef.FAILOVER_SUCCESS,
                                                    aNodeConn.getNodeName(),
                                                    aNodeConn.getServer(),
                                                    aNodeConn.getPort());
        }
        else
        {
            CmOperation.throwShardFailoverIsNotAvailableException(aNodeConn.getNodeName(),
                                                                  aNodeConn.getServer(),
                                                                  aNodeConn.getPort());
        }
    }

    /**
     * 노드커넥션이고 통신에러가 났을때 alternateservers 셋팅이 없으면 FailoverIsNotAvailableException을 올리고
     * 그렇지 않은 경우 STF로 처리한다.
     * @param aEx 발생한 익셉션
     * @throws SQLException prepare, execute, fetch 도중 오류가 발생한 경우
     */
    public static void tryShardFailOver(AltibaseConnection aNodeConn, SQLException aEx) throws SQLException
    {
        if (aNodeConn.isNodeConnection() && AltibaseFailover.isNeedToFailover(aEx) &&
            ( aNodeConn.failoverContext() == null))
        {
            AltibaseShardingConnection sMetaConn = aNodeConn.getMetaConnection();
            /* BUG-46790 failover 설정이 안되어 있더라도 stf가 true인 경우 reconnect를 시도해야 한다.  */
            if (sMetaConn.isSessionFailoverOn())
            {
                AltibaseShardingFailover sShardFailover = sMetaConn.getShardFailover();
                sShardFailover.tryReconnect(aNodeConn);
            }
            else
            {
                CmOperation.throwShardFailoverIsNotAvailableException(aNodeConn.getNodeName(),
                                                                      aNodeConn.getServer(),
                                                                      aNodeConn.getPort());
            }
        }
        else
        {
            AltibaseFailover.trySTF(aNodeConn.failoverContext(), aEx);
        }
    }
}
