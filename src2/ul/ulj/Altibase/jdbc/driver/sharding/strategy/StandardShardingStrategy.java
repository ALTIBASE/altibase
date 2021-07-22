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

package Altibase.jdbc.driver.sharding.strategy;

import Altibase.jdbc.driver.sharding.algorithm.*;
import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardRangeList;
import Altibase.jdbc.driver.sharding.core.ShardSplitMethod;

import java.sql.SQLException;
import java.util.*;
import java.util.logging.Level;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

/**
 * 샤딩 컬럼이 하나일때의 샤딩을 처리한다.
 */
public class StandardShardingStrategy implements ShardingStrategy
{
    private RangeShardingAlgorithm mDefaultShardingAlgorithm;
    private ShardSplitMethod       mShardSplitMethod;
    private DataNode               mDefaultNode;

    public StandardShardingStrategy(ShardRangeList aShardRangeList,
                                    ShardSplitMethod aShardSplitMethod, DataNode aDefaultNode)
    {
        mShardSplitMethod = aShardSplitMethod;
        mDefaultShardingAlgorithm = makeShardingAlgorithm(aShardRangeList);
        mDefaultNode = aDefaultNode;
    }

    private RangeShardingAlgorithm makeShardingAlgorithm(ShardRangeList aShardRangeList)
    {
        RangeShardingAlgorithm sResult = null;

        switch (mShardSplitMethod)
        {
            case HASH:
            case RANGE:
                sResult = new StandardRangeShardingAlgorithm(aShardRangeList);
                break;
            case LIST:
                sResult = new ListShardingAlgorithm(aShardRangeList);
                break;
        }

        return sResult;
    }

    @SuppressWarnings("unchecked")
    public List<DataNode> doSharding(List<Comparable<?>> aShardingValues) throws SQLException
    {
        Set<DataNode> sResults = new TreeSet<DataNode>(); // BUG-47653 중복을 제거하기 위해 TreeSet으로 선언
        for (Comparable<?> sEach : aShardingValues)
        {
            try
            {
                sResults.addAll(mDefaultShardingAlgorithm.doSharding(sEach, mDefaultNode));
            }
            catch (SQLException aEx)
            {
                shard_log(Level.SEVERE, aEx);
                throw aEx;
            }
        }

        return new ArrayList<DataNode>(sResults);
    }
}
