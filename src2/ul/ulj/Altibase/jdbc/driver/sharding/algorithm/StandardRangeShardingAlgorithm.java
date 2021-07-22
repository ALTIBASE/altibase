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

package Altibase.jdbc.driver.sharding.algorithm;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardRange;
import Altibase.jdbc.driver.sharding.core.ShardRangeList;
import Altibase.jdbc.driver.sharding.util.Range;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class StandardRangeShardingAlgorithm<T extends Comparable<T>> implements RangeShardingAlgorithm<T>
{
    private ShardRangeList mShardRangeList;

    public StandardRangeShardingAlgorithm(ShardRangeList aShardRangeList)
    {
        mShardRangeList = aShardRangeList;
    }

    @SuppressWarnings("unchecked")
    public List<DataNode> doSharding(Comparable<?> aShardingValue, DataNode aDefaultNode) throws SQLException
    {
        // BUG-46790 range정보가 없더라도 default node가 있는 경우 정상으로 처리해야 한다.
        if (mShardRangeList.getRangeList().isEmpty() && aDefaultNode == null)
        {
            Error.throwSQLException(ErrorDef.SHARD_RANGE_NOT_FOUNDED);
        }
        List<DataNode> sResult = new ArrayList<DataNode>();

        for (ShardRange sShardRange : mShardRangeList.getRangeList())
        {
            Range sRange = sShardRange.getRange();
            // PROJ-2690 range가 순차적으로 정렬되어 있기 때문에 앞에서 부터 max값만 비교하면 된다.
            if (sRange.containsEndedBy(aShardingValue))
            {
                sResult.add(sShardRange.getNode());
                break;
            }
        }

        if (sResult.size() == 0)
        {
            if (aDefaultNode != null)
            {
                sResult.add(aDefaultNode);
            }
            else
            {
                Error.throwSQLException(ErrorDef.SHARD_NODE_NOT_FOUNDED);
            }
        }

        shard_log("(RANGE SHARD RESULT) shardValue={0}, nodeId={1}", new Object[] { aShardingValue, sResult });
        return sResult;
    }
}
