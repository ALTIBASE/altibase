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

package Altibase.jdbc.driver.cm;

import Altibase.jdbc.driver.AltibaseDataSource;
import Altibase.jdbc.driver.sharding.core.AltibaseShardingConnection;
import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardNodeConfig;
import Altibase.jdbc.driver.util.AltibaseProperties;
import Altibase.jdbc.driver.util.StringUtils;

import javax.sql.DataSource;
import java.util.*;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;
import static Altibase.jdbc.driver.util.AltibaseProperties.*;

public class CmProtocolContextShardConnect extends CmProtocolContextConnect
{
    private ShardNodeConfig            mShardNodeConfig;
    private DataNode                   mShardOnTransactionNode;
    private boolean                    mIsAutoCommit;
    private boolean                    mIsTouched;
    private boolean                    mIsNodeTransactionStarted;
    private Map<DataNode, DataSource>  mDataSourceMap;
    private String                     mServerCharSet;
    private CmConnType                 mShardConnType;
    private long                       mShardPin;
    private long                       mShardMetaNumber;
    private long                       mSMNOfDataNode;
    private boolean                    mNeedToDisconnect; // BUG-46513 smn 오류가 발생했을때 접속을 끊어야할지 여부
    private AltibaseShardingConnection mMetaConn;

    public CmProtocolContextShardConnect(AltibaseShardingConnection aMetaConn)
    {
        super();
        // BUG-46513 노드가 제거되었을때 해당하는 노드 커넥션을 정리하기 위해 meta connection 객체를 주입한다.
        mMetaConn = aMetaConn;
    }

    public ShardNodeConfig getShardNodeConfig()
    {
        return mShardNodeConfig;
    }

    public void setShardNodeConfig(ShardNodeConfig aShardNodeConfig)
    {
        mShardNodeConfig = aShardNodeConfig;
    }

    /**
     * getNodeList 프로토콜의 결과로 생성된 ShardNodeConfig객체를 주입한다.
     * @param aShardPin 샤드핀값
     * @param aShardMetaNumber getNodeList프로토콜을 통해 전달받은 ShardMetaNumber 값
     * @param aShardNodeConfig getNodeList프로토콜을 통해 새로 생성한 ShardNodeConfig 객체
     */
    public void setShardNodeConfig(long aShardPin, long aShardMetaNumber, ShardNodeConfig aShardNodeConfig)
    {
        if (mShardMetaNumber < aShardMetaNumber)
        {
            mShardPin = aShardPin;
            mShardMetaNumber = aShardMetaNumber;

            // BUG-46513 기존에 이미 ShardNodeConfig객체가 있는 경우 노드리스트를 병합한다.
            if (mShardNodeConfig != null)
            {
                Set<DataNode> sOldNodeSet = new LinkedHashSet<DataNode>(mShardNodeConfig.getDataNodes());
                Set<DataNode> sCurrNodeSet = new LinkedHashSet<DataNode>(aShardNodeConfig.getDataNodes());
                mergeNodeList(sOldNodeSet, sCurrNodeSet);
            }

            mShardNodeConfig = aShardNodeConfig;
        }
    }

    /**
     * 두개의 노드리스트를 병합한 결과를 ShardNodeConfig객체에 다시 셋팅한다.<br>
     * 제거된 노드는 내부적으로 close처리를 하고 추가된 노드에 대해서는 노드 정보만 업데이트 해 놓는다.<br>
     * 병합시 중복노드가 생성되는 것을 방지하기 위해 Set을 사용함.
     * @param aOldNodes 기존 노드리스트
     * @param aCurrNodes 갱신된 노드리스트
     */
    private void mergeNodeList(Set<DataNode> aOldNodes, Set<DataNode> aCurrNodes)
    {
        List<DataNode> sRemovedNodeList = new ArrayList<DataNode>();
        for (DataNode sEach : aOldNodes)
        {
            if (!aCurrNodes.contains(sEach))
            {
                sRemovedNodeList.add(sEach);
            }
        }
        // BUG-46513 제거된 노드커넥션에 대해 close 처리를 하고 OldNodes 에서 삭제
        for (DataNode sRemovedNode : sRemovedNodeList)
        {
            mMetaConn.removeNode(sRemovedNode);
            aOldNodes.remove(sRemovedNode);
        }
        aOldNodes.addAll(aCurrNodes); // BUG-46513 current 리스트를 OldNodes에 병합
        List<DataNode> sResult = new ArrayList<DataNode>(aOldNodes);
        mShardNodeConfig.setDataNodes(sResult);
        mShardNodeConfig.setNodeCount(sResult.size()); // BUG-47357 병합 후 node count 값도 업데이트 해야 한다.
    }

    public boolean isNodeTransactionStarted()
    {
        return mIsNodeTransactionStarted;
    }

    public void setNodeTransactionStarted(boolean aNodeTransactionStarted)
    {
        mIsNodeTransactionStarted = aNodeTransactionStarted;
    }

    public boolean isAutoCommitMode()
    {
        return mIsAutoCommit;
    }

    public void setAutoCommit(boolean aIsAutoCommit)
    {
        mIsAutoCommit = aIsAutoCommit;
    }

    public Map<DataNode, DataSource> getDataSourceMap()
    {
        return mDataSourceMap;
    }

    public void setDataSourceMap(Map<DataNode, DataSource> aDataSourceMap)
    {
        mDataSourceMap = aDataSourceMap;
    }

    public DataNode getShardOnTransactionNode()
    {
        return mShardOnTransactionNode;
    }

    public void setShardOnTransactionNode(DataNode aShardOnTransactionNode)
    {
        mShardOnTransactionNode = aShardOnTransactionNode;
    }

    public boolean isTouched()
    {
        return mIsTouched;
    }

    public void setTouched(boolean aTouched)
    {
        mIsTouched = aTouched;
    }

    public String getServerCharSet()
    {
        return mServerCharSet;
    }

    public void setServerCharSet(String aServerCharSet)
    {
        mServerCharSet = aServerCharSet;
    }

    public long getShardPin()
    {
        return mShardPin;
    }

    public void setShardMetaNumber(long aShardMetaNumber)
    {
        mShardMetaNumber = aShardMetaNumber;
    }

    public long getShardMetaNumber()
    {
        return mShardMetaNumber;
    }

    public void setSMNOfDataNode(long aSMNOfDataNode)
    {
        if ( mSMNOfDataNode < aSMNOfDataNode )
        {
            mSMNOfDataNode = aSMNOfDataNode;
        }
    }

    public long getSMNOfDataNode()
    {
        return mSMNOfDataNode;
    }

    /**
     * DataNode의 정보를 바탕으로 DataSource객체를 만든다. 이때 shardpin, shard_meta_number와 같은 값들도 함께<br>
     * 셋팅한다.
     * @param aProp AltibaseProperties 객체
     */
    public void createDataSources(AltibaseProperties aProp)
    {
        mDataSourceMap = new HashMap<DataNode, DataSource>();
        for (DataNode sNode : mShardNodeConfig.getDataNodes())
        {
            AltibaseProperties sPropsForDataNode = copyProps(aProp);
            removeConnAttributes(sPropsForDataNode);
            sPropsForDataNode.put(PROP_SERVER, sNode.getServerIp());
            sPropsForDataNode.put(PROP_PORT, String.valueOf(sNode.getPortNo()));
            // data node의 dbname이 넘어오지 않기때문에 공백문자로 셋팅한다. 이렇게 하면 서버에서는 dbname을 체크하지 않는다.
            sPropsForDataNode.put(PROP_DBNAME, "");
            sPropsForDataNode.put(PROP_SHARD_NODE_NAME, sNode.getNodeName());
            sPropsForDataNode.put(PROP_SHARD_PIN, String.valueOf(mShardPin));
            sPropsForDataNode.put(PROP_SHARD_META_NUMBER, String.valueOf(getShardMetaNumber()));
            if (mShardConnType != CmConnType.TCP)
            {
                sPropsForDataNode.put(PROP_CONNTYPE, mShardConnType.name());
                if (mShardConnType == CmConnType.SSL)
                {
                    // ssl인 경우에는 추가로 ssl_enable을 활성화 해줘야 한다.
                    sPropsForDataNode.put(PROP_SSL_ENABLE, "true");
                }
            }
            // BUG-47324
            sPropsForDataNode.put(PROP_SHARD_CLIENT, "1");
            sPropsForDataNode.put(PROP_SHARD_SESSION_TYPE, "2");
            makeAlternateServerProps(sNode, sPropsForDataNode);
            AltibaseDataSource sDataSource = new AltibaseDataSource(sPropsForDataNode);
            sDataSource.setUser(aProp.getUser());
            sDataSource.setPassword(aProp.getPassword());
            mDataSourceMap.put(sNode, sDataSource);
            shard_log("(MAKE DATASOURCE) {0}", sNode);
        }
    }

    /**
     * DataNode의 AlternateServer 정보를 바탕으로 connection string을 구성한다.
     * @param aNode 데이터노드 정보 객체
     * @param aPropsForDataNode alternateservers 속성을 포함하는 Property 객체
     */
    private void makeAlternateServerProps(DataNode aNode, AltibaseProperties aPropsForDataNode)
    {
        String sAlternateServerIp = aNode.getAlternativeServerIp();
        if (!StringUtils.isEmpty(sAlternateServerIp))
        {
            String sAlternateServerPort = String.valueOf(aNode.getAlternativePortNo());
            StringBuilder sSb = new StringBuilder();
            sSb.append("(").append(sAlternateServerIp)
               .append(":").append(sAlternateServerPort).append(")");
            aPropsForDataNode.put(PROP_ALT_SERVERS, sSb.toString());
        }
    }

    /**
     * Meta에 접속할때 생성된 Property파일을 DataNode에 사용하기 위해 복제한다.
     * @param aProp 원본 Property파일
     * @return 복제된 Property파일
     */
    private AltibaseProperties copyProps(AltibaseProperties aProp)
    {
        AltibaseProperties sResults = new AltibaseProperties();
        AltibaseProperties sProp = aProp.getDefaults();
        while (sProp != null)
        {
            setProperties(sResults, sProp);
            sProp = sProp.getDefaults();
        }
        setProperties(sResults, aProp);

        return sResults;
    }

    /**
     * 원본 프로퍼티의 key와 value를 loop를 돌면서 복제한다.
     * @param aResults 복사된 Property 객체
     * @param aSourceProps 원본 Property 객체
     */
    private void setProperties(AltibaseProperties aResults, AltibaseProperties aSourceProps)
    {
        for (Map.Entry sEntry : aSourceProps.entrySet())
        {
            String sKey = (String)sEntry.getKey();
            String sValue = (String)sEntry.getValue();
            if (!StringUtils.isEmpty(sKey))
            {
                aResults.setProperty(sKey, sValue);
            }
        }
    }

    /**
     * DataNode용 Property 파일을 생성할때 기본 프로퍼티를 제거한다.
     * @param aProp 원본 Property파일
     */
    private void removeConnAttributes(Properties aProp)
    {
        aProp.remove(AltibaseProperties.PROP_CONNTYPE);
        aProp.remove(AltibaseProperties.PROP_SHARD_CONNTYPE);
        aProp.remove(AltibaseProperties.PROP_SHARD_LAZYCONNECT);
        aProp.remove(AltibaseProperties.PROP_RESHARD_ENABLE);
        aProp.remove(AltibaseProperties.PROP_SSL_ENABLE);
        aProp.remove(AltibaseProperties.PROP_DATASOURCE_NAME);
        aProp.remove(AltibaseProperties.PROP_ALT_SERVERS);
        aProp.remove(AltibaseProperties.PROP_PORT);
        aProp.remove(AltibaseProperties.PROP_SERVER);
        aProp.remove(AltibaseProperties.PROP_DBNAME);
        aProp.remove(AltibaseProperties.PROP_LOAD_BALANCE);
        /* PROJ-2727 Global property handling 
         * nodeConn에 전파하지 않는 속성 추가 
         */
        aProp.remove(AltibaseProperties.PROP_DEFERRED_PREPARE);
        aProp.remove(AltibaseProperties.PROP_GLOBAL_DDL);
        aProp.remove(AltibaseProperties.PROP_CONNECT_MODE);
        aProp.remove(AltibaseProperties.PROP_SOCK_BIND_ADDR);
    }

    public void setShardConnType(CmConnType aShardConnType)
    {
        mShardConnType = aShardConnType;
    }

    public boolean needToDisconnect()
    {
        return mNeedToDisconnect;
    }

    public void setNeedToDisconnect(boolean aNeedToDisconnect)
    {
        mNeedToDisconnect = aNeedToDisconnect;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("CmProtocolContextShardConnect{");
        sSb.append("mShardNodeConfig=").append(mShardNodeConfig);
        //sSb.append(", mGlobalTransactionLevel=").append(mGlobalTransactionLevel);
        sSb.append(", mShardOnTransactionNode=").append(mShardOnTransactionNode);
        sSb.append(", mIsAutoCommit=").append(mIsAutoCommit);
        sSb.append(", mIsTouched=").append(mIsTouched);
        sSb.append(", mIsNodeTransactionStarted=").append(mIsNodeTransactionStarted);
        sSb.append(", mDataSourceMap=").append(mDataSourceMap);
        sSb.append(", mServerCharSet='").append(mServerCharSet).append('\'');
        sSb.append(", mShardConnType=").append(mShardConnType);
        sSb.append(", mShardPin=").append(mShardPin);
        sSb.append(", mShardMetaNumber=").append(mShardMetaNumber);
        sSb.append(", mSMNOfDataNode=").append(mSMNOfDataNode);
        sSb.append(", mNeedToDisconnect=").append(mNeedToDisconnect);
        sSb.append(", mMetaConn=").append(mMetaConn);
        sSb.append('}');

        return sSb.toString();
    }
}
