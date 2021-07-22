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

import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardRange;
import Altibase.jdbc.driver.sharding.core.ShardRangeList;
import Altibase.jdbc.driver.sharding.core.ShardSplitMethod;
import Altibase.jdbc.driver.sharding.util.Range;
import junit.framework.TestCase;

import java.sql.SQLException;
import java.util.*;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

public class StandardCompositeShardingAlgorithmTest extends TestCase
{
    public void testShardingWithPrimarySubShardingValue() throws SQLException
    {
        CompositeShardingAlgorithm sAlgorithm = new StandardCompositeShardingAlgorithm(makeRangeInfo());
        List<Comparable<?>> sShardValueList = new ArrayList<Comparable<?>>();
        sShardValueList.add(587);
        sShardValueList.add(731);

        List<DataNode> sResult = sAlgorithm.doSharding(sShardValueList, Arrays.asList(ShardSplitMethod.HASH,
                                                                                     ShardSplitMethod.HASH),
                                                      null, 1, 1);
        assertThat(sResult.size(), is(1));
        assertThat(sResult.iterator().next().getNodeName(), is("NODE3"));

        sShardValueList = new ArrayList<Comparable<?>>();
        sShardValueList.add(299);
        sShardValueList.add(300);

        sResult = sAlgorithm.doSharding(sShardValueList, Arrays.asList(ShardSplitMethod.HASH,
                                                                       ShardSplitMethod.HASH),
                                                    null, 1, 1);
        assertThat(sResult.size(), is(1));
        assertThat(sResult.iterator().next().getNodeName(), is("NODE2"));
    }

    public void testShardingWithOnlySubShardingValue() throws SQLException
    {
        CompositeShardingAlgorithm sAlgorithm = new StandardCompositeShardingAlgorithm(makeRangeInfo());
        List<Comparable<?>> sShardValueList = new ArrayList<Comparable<?>>();
        sShardValueList.add(731);

        List<DataNode> sResult = sAlgorithm.doSharding(sShardValueList, Arrays.asList(ShardSplitMethod.HASH,
                                                                                     ShardSplitMethod.HASH),
                                                     null, 0, 1);
        assertThat(sResult.size(), is(2));
    }

    public void testShardingWithDefaultNodeName() throws SQLException
    {
        CompositeShardingAlgorithm sAlgorithm = new StandardCompositeShardingAlgorithm(makeRangeInfo());
        List<Comparable<?>> sShardValueList = new ArrayList<Comparable<?>>();
        sShardValueList.add(300);
        sShardValueList.add(1000);

        DataNode sDefaultNode = new DataNode();
        sDefaultNode.setNodeId(4);
        sDefaultNode.setNodeName("NODE4");
        List<DataNode> sResult = sAlgorithm.doSharding(sShardValueList, Arrays.asList(ShardSplitMethod.HASH,
                                                                                     ShardSplitMethod.HASH),
                                                      sDefaultNode, 1, 1);
        assertThat(sResult.size(), is(1));
        assertThat(sResult.iterator().next(), is(sDefaultNode));
    }

    /*
            composite
            300 ,  300  -  NODE1
            300 , 1000  -  NODE2
            600 ,  300  -  NODE1
            600 ,  600  -  NODE2
            600 , 1000  -  NODE3
     */
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


        sShardRangeList.addRange(new ShardRange(sNode1, Range.between(Range.getNullRange(), 300),
                                                Range.between(Range.getNullRange(), 300)));
        sShardRangeList.addRange(new ShardRange(sNode2, Range.between(Range.getNullRange(), 300),
                                                Range.between(300, 1000)));
        sShardRangeList.addRange(new ShardRange(sNode1, Range.between(300, 600),
                                                Range.between(Range.getNullRange(), 300)));
        sShardRangeList.addRange(new ShardRange(sNode2, Range.between(300, 600),
                                                Range.between(300, 600)));
        sShardRangeList.addRange(new ShardRange(sNode3, Range.between(300, 600),
                                                Range.between(600, 1000)));

        return sShardRangeList;
    }
}
