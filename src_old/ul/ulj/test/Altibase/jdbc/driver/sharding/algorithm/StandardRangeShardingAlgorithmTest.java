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

import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardRange;
import Altibase.jdbc.driver.sharding.core.ShardRangeList;
import Altibase.jdbc.driver.sharding.util.Range;
import junit.framework.TestCase;

import java.sql.SQLException;
import java.util.List;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

public class StandardRangeShardingAlgorithmTest extends TestCase
{
    @SuppressWarnings("unchecked")
    public void testShardingWithPrimaryShardingValue() throws SQLException
    {
        StandardRangeShardingAlgorithm<Integer> sAlgorithm =
                new StandardRangeShardingAlgorithm<Integer>(makeRangeInfo());

        Comparable<Integer> sShardingValue = 100;
        List<DataNode> sResult = sAlgorithm.doSharding(sShardingValue, null);
        assertThat(sResult.size(), is(1));
        assertThat(sResult.iterator().next().getNodeName(), is("NODE1"));

        sShardingValue = 300;
        sResult = sAlgorithm.doSharding(sShardingValue, null);
        assertThat(sResult.size(), is(1));
        assertThat(sResult.iterator().next().getNodeName(), is("NODE2"));

        sShardingValue = 600;
        sResult = sAlgorithm.doSharding(sShardingValue, null);
        assertThat(sResult.iterator().next().getNodeName(), is("NODE3"));
    }

    public void testShardingWithDefaultNode() throws SQLException
    {
        StandardRangeShardingAlgorithm<Integer> sAlgorithm =
                new StandardRangeShardingAlgorithm<Integer>(makeRangeInfo());
        Comparable<Integer> sShardingValue = 900;
        DataNode sNode4 = new DataNode();
        sNode4.setNodeId(4);
        sNode4.setNodeName("NODE4");
        List<DataNode> sResult = sAlgorithm.doSharding(sShardingValue, sNode4);
        assertThat(sResult.size(), is(1));
        assertThat(sResult.iterator().next(), is(sNode4));
    }

    public void testShardingWithNoRangeValues()
    {
        StandardRangeShardingAlgorithm<Integer> sAlgorithm =
                new StandardRangeShardingAlgorithm<Integer>(makeEmptyRangeInfo());

        Comparable<Integer> sShardingValue = 900;
        try
        {
            sAlgorithm.doSharding(sShardingValue, null);
        }
        catch (SQLException aException)
        {
            assertThat(aException.getErrorCode(), is(ErrorDef.SHARD_RANGE_NOT_FOUNDED));
        }
    }

    @SuppressWarnings("unchecked")
    private ShardRangeList makeRangeInfo()
    {
        ShardRangeList sShardRangeList = new ShardRangeList();
        DataNode sNode1 = new DataNode();
        sNode1.setNodeId(1);
        sNode1.setNodeName("NODE1");
        DataNode sNode2 = new DataNode();
        sNode2.setNodeId(2);
        sNode2.setNodeName("NODE2");
        DataNode sNode3 = new DataNode();
        sNode3.setNodeId(3);
        sNode3.setNodeName("NODE3");

        sShardRangeList.addRange(new ShardRange(sNode1, Range.between(Range.getNullRange(), 300)));
        sShardRangeList.addRange(new ShardRange(sNode2, Range.between(300, 600)));
        sShardRangeList.addRange(new ShardRange(sNode3, Range.between(600, 900)));

        return sShardRangeList;
    }

    private ShardRangeList makeEmptyRangeInfo()
    {
        ShardRangeList sShardRangeList = new ShardRangeList();
        return  sShardRangeList;
    }
}
