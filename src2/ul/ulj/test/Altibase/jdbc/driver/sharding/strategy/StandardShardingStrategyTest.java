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
 */

package Altibase.jdbc.driver.sharding.strategy;

import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardRange;
import Altibase.jdbc.driver.sharding.core.ShardRangeList;
import Altibase.jdbc.driver.sharding.core.ShardSplitMethod;
import Altibase.jdbc.driver.sharding.util.Range;
import junit.framework.TestCase;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/*
   Default Node : NODE3
       ~  300 : NODE1
   300 ~  600 : NODE2
   600 ~  900 : NODE3
 */
public class StandardShardingStrategyTest extends TestCase
{
    private DataNode mNode1;
    private DataNode mNode2;
    private DataNode mNode3;
    private ShardRangeList mShardRangeList;

    public void setUp()
    {
        mNode1 = new DataNode();
        mNode1.setNodeId(1);
        mNode1.setNodeName("NODE1");
        mNode2 = new DataNode();
        mNode2.setNodeId(2);
        mNode2.setNodeName("NODE2");
        mNode3 = new DataNode();
        mNode3.setNodeId(3);
        mNode3.setNodeName("NODE3");

        mShardRangeList = new ShardRangeList();
        mShardRangeList.addRange(new ShardRange(mNode1, Range.between(Range.getNullRange(), 300)));
        mShardRangeList.addRange(new ShardRange(mNode2, Range.between(300, 600)));
        mShardRangeList.addRange(new ShardRange(mNode3, Range.between(600, 900)));
    }

    public void testDoShardingWithTwohardValues() throws SQLException
    {
        StandardShardingStrategy sStrategy = new StandardShardingStrategy(mShardRangeList,
                                                                          ShardSplitMethod.RANGE,
                                                                          mNode3);
        List<Comparable<?>> sShardValues = new ArrayList<Comparable<?>>();
        sShardValues.add(100);  // NODE1
        sShardValues.add(300);  // NODE2
        List<DataNode> sResult = sStrategy.doSharding(sShardValues);
        assertEquals(2, sResult.size());
        assertEquals(mNode1, sResult.get(0));
        assertEquals(mNode2, sResult.get(1));
    }

    public void testDoShardingWithThreeShardValues() throws SQLException
    {
        StandardShardingStrategy sStrategy = new StandardShardingStrategy(mShardRangeList,
                                                                          ShardSplitMethod.RANGE,
                                                                          mNode3);
        List<Comparable<?>> sShardValues = new ArrayList<Comparable<?>>();
        sShardValues.add(100);  // NODE1
        sShardValues.add(300);  // NODE2
        sShardValues.add(400);  // NODE2
        List<DataNode> sResult = sStrategy.doSharding(sShardValues);
        assertEquals(2, sResult.size());
        assertEquals(mNode1, sResult.get(0));
        assertEquals(mNode2, sResult.get(1));
    }
}
