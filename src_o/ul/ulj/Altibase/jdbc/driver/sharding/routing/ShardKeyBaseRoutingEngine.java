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

package Altibase.jdbc.driver.sharding.routing;

import Altibase.jdbc.driver.cm.CmProtocolContextShardConnect;
import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;
import Altibase.jdbc.driver.cm.CmShardAnalyzeResult;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.sharding.algorithm.HashGenerator;
import Altibase.jdbc.driver.sharding.core.*;
import Altibase.jdbc.driver.sharding.strategy.ShardingStrategy;

import java.sql.SQLException;
import java.util.LinkedList;
import java.util.List;

import static Altibase.jdbc.driver.sharding.core.ShardValueType.*;
import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public abstract class ShardKeyBaseRoutingEngine implements RoutingEngine
{
    protected CmProtocolContextShardStmt mShardStmtCtx;
    ShardingStrategy                     mShardingStrategy;
    ShardSplitMethod                     mShardSplitMethod;
    ShardKeyDataType                     mShardKeyDataType;
    ShardRangeList                       mShardRangeList;
    int                                  mShardParamIdx;
    int                                  mShardSubParamIdx;
    int                                  mShardValueCnt;
    int                                  mShardSubValueCnt;
    private String                       mServerCharSet;

    ShardKeyBaseRoutingEngine(CmProtocolContextShardStmt aShardStmtCtx)
    {
        mShardStmtCtx = aShardStmtCtx;
        CmShardAnalyzeResult sAnalyzeResult = mShardStmtCtx.getShardAnalyzeResult();
        mShardSplitMethod = sAnalyzeResult.getShardSplitMethod();
        mShardKeyDataType = sAnalyzeResult.getShardKeyDataType();
        mShardRangeList = sAnalyzeResult.getShardRangeList();
        CmProtocolContextShardConnect sShardContextConn = aShardStmtCtx.getShardContextConnect();
        mServerCharSet = sShardContextConn.getServerCharSet();
        mShardValueCnt = sAnalyzeResult.getShardValueCount();
        mShardSubValueCnt = sAnalyzeResult.getShardSubValueCount();
    }

    List<Comparable<?>> getShardValues(List<Column> aParameters,
                                       List<ShardValueInfo> aShardValueInfoList,
                                       ShardSplitMethod aShardSplitMethod,
                                       ShardKeyDataType aShardKeyDataType,
                                       boolean aIsSub) throws SQLException
    {
        List<Comparable<?>> sResult = new LinkedList<Comparable<?>>();
        for (ShardValueInfo sShardValueInfo : aShardValueInfoList)
        {
            Column sParameter = getShardParameterInfo(aParameters, aIsSub, sShardValueInfo);
            // BUG-47314 샤드파라메터가 셋팅되지 않은 경우 예외를 올린다.
            if (sParameter == null)
            {
                Error.throwSQLException(ErrorDef.SHARD_BIND_PARAMETER_MISSING,
                                        String.valueOf(sShardValueInfo.getBindParamId() + 1));
            }
            if (aShardSplitMethod == ShardSplitMethod.HASH)
            {
                long sHashValue = HashGenerator.getInstance().doHash(sParameter, aShardKeyDataType,
                                                                     mServerCharSet);
                int sHashModValue = (int)(sHashValue % 1000);
                shard_log("(SHARD HASH VALUE) {0}", sHashValue);
                sResult.add(sHashModValue);
            }
            else
            {
                if (sParameter.isNull())
                {
                    sResult.add(null);
                }
                else
                {
                    switch (aShardKeyDataType)
                    {
                        case SMALLINT:
                            sResult.add(sParameter.getShort());
                            break;
                        case INTEGER:
                            sResult.add(sParameter.getInt());
                            break;
                        case BIGINT:
                            sResult.add(sParameter.getLong());
                            break;
                        case CHAR:
                        case VARCHAR:
                            sResult.add(sParameter.getString());
                    }
                }            
            }
        }

        return sResult;
    }

    private Column getShardParameterInfo(List<Column> aParameters, boolean aIsSub, 
                                         ShardValueInfo aShardValueInfo)
    {
        Column sParameter = null;
        if (aShardValueInfo.getType() == HOST_VAR)
        {
            int sBindParamId = aShardValueInfo.getBindParamId();
            if (aIsSub)
            {
                mShardSubParamIdx = sBindParamId;
            }
            else
            {
                mShardParamIdx = sBindParamId;
            }
            // BUG-47314 샤드파라메터가 셋팅된 경우에만 값을 가져온다.
            if (aParameters.size() >= sBindParamId + 1)
            {
                sParameter = aParameters.get(sBindParamId);
            }
        }
        else     // bind가 아닐경우
        {
            sParameter = aShardValueInfo.getValue();
        }
        return sParameter;
    }

    DataNode getDefaultNode()
    {
        int sDefaultNodeId = mShardStmtCtx.getShardAnalyzeResult().getShardDefaultNodeID();
        DataNode sDefaultNode = null;
        if (sDefaultNodeId > 0)
        {
            ShardNodeConfig sShardNodeConfig = mShardStmtCtx.getShardContextConnect().getShardNodeConfig();
            for (DataNode sEach : sShardNodeConfig.getDataNodes())
            {
                if (sDefaultNodeId == sEach.getNodeId())
                {
                    sDefaultNode = sEach;
                    break;
                }
            }
        }
        return sDefaultNode;
    }
}
