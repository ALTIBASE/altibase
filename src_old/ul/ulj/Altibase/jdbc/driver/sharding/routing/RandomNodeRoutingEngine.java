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
import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardRange;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * Shard split method 가 Clone일때 조회를 하는 경우 랜덤한 node id를 돌려준다.
 */
public class RandomNodeRoutingEngine implements RoutingEngine
{
    private CmProtocolContextShardStmt mShardStmtCtx;
    private Random mRandomGenerator;

    public RandomNodeRoutingEngine(CmProtocolContextShardStmt aShardStmtCtx)
    {
        mShardStmtCtx = aShardStmtCtx;
        mRandomGenerator = new Random();
    }

    public List<DataNode> route(String aSql, List<Column> aParameters) throws SQLException
    {
        CmProtocolContextShardConnect sShardConnectCtx = mShardStmtCtx.getShardContextConnect();
        DataNode sNode = null;
        if (sShardConnectCtx.isNodeTransactionStarted())
        {
            sNode = sShardConnectCtx.getShardOnTransactionNode();
        }
        else
        {
            CmShardAnalyzeResult sShardAnalyzeResult = mShardStmtCtx.getShardAnalyzeResult();
            int sRandomNodeIndex = mRandomGenerator.nextInt(sShardAnalyzeResult.getShardRangeInfoCnt());
            List<ShardRange> sShardRangeList = sShardAnalyzeResult.getShardRangeList().getRangeList();
            int sRangeIdx = 0;
            // BUG-46513 node list가 아닌 range에서 random하게 가져와야 한다.
            for (ShardRange sEach : sShardRangeList)
            {
                sNode = sEach.getNode();
                if (sRangeIdx == sRandomNodeIndex)
                {
                    break;
                }
                sRangeIdx++;
            }
        }
        if (sNode == null)
        {
            Error.throwSQLException(ErrorDef.SHARD_NODE_NOT_FOUNDED);
        }

        List<DataNode> sResult = new ArrayList<DataNode>();
        sResult.add(sNode);

        return sResult;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("RandomNodeRoutingEngine{");
        sSb.append("mShardStmtCtx=").append(mShardStmtCtx);
        sSb.append(", mRandomGenerator=").append(mRandomGenerator);
        sSb.append('}');
        return sSb.toString();
    }
}
