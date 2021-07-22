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
import Altibase.jdbc.driver.sharding.core.ShardSplitMethod;
import Altibase.jdbc.driver.sharding.util.Range;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class StandardCompositeShardingAlgorithm implements CompositeShardingAlgorithm
{
    private ShardRangeList mShardRangeList;

    public StandardCompositeShardingAlgorithm(ShardRangeList aShardRangeList)
    {
        this.mShardRangeList = aShardRangeList;
    }

    public List<DataNode> doSharding(List<Comparable<?>> aShardValueList,
                                     List<ShardSplitMethod> aShardSplitMethodList, DataNode aDefaultNode,
                                     int aShardValueCnt, int aShardSubValueCnt)
            throws SQLException
    {
        if (mShardRangeList.getRangeList().isEmpty())
        {
            Error.throwSQLException(ErrorDef.SHARD_RANGE_NOT_FOUNDED);
        }
        List<DataNode> sResult = new ArrayList<DataNode>();
        // PROJ-2690 현재 복합샤드키는 두개까지 밖에 안되기 때문에 aShardValueList의 첫번째, 두번째 항목만 가져온다.

        ShardSplitMethod sShardSplitMethod = aShardSplitMethodList.get(0);
        ShardSplitMethod sShardSubSplitMethod = aShardSplitMethodList.get(1);

        List<Integer> sPrimaryNodeIndexList = new ArrayList<Integer>();
        List<Integer> sSubNodeIndexList = new ArrayList<Integer>();

        int sNodeIdx = 0;
        boolean sDefaultNodeExec = false;
        boolean[] sPrimaryFound = new boolean[aShardValueCnt];
        boolean[] sSecondaryFound = new boolean[aShardSubValueCnt];
        int sRangeSize = mShardRangeList.getRangeList().size();
        for (ShardRange sShardRange : mShardRangeList.getRangeList())
        {
            // PROJ-2690 첫번째 샤드키 값에 대한 range 체크. null인 경우에는 Sub키 값만 비교한다.
            int i;
            for (i = 0; i < aShardValueCnt; i++)
            {
                Comparable<?> sShardingValue = aShardValueList.get(i);
                if (sShardingValue != null)
                {
                    if (isInTheRange(sShardingValue, sShardSplitMethod, sShardRange.getRange()))
                    {
                        sPrimaryNodeIndexList.add(sNodeIdx);
                        sPrimaryFound[i] = true;
                    }
                    else
                    {
                        if (sNodeIdx == sRangeSize - 1 && !sPrimaryFound[i])
                        {
                            sDefaultNodeExec = true;
                        }
                    }
                }
            }
            // PROJ-2690  두번째 샤드키 값에 대한 range 체크
            for (int j = 0; j < aShardSubValueCnt; j++)
            {
                Comparable<?> sShardingSubValue = aShardValueList.get(i + j);
                if (sShardingSubValue != null)
                {
                    if (isInTheRange(sShardingSubValue, sShardSubSplitMethod, sShardRange.getSubRange()))
                    {
                        sSubNodeIndexList.add(sNodeIdx);
                        sSecondaryFound[j] = true;
                    }
                    else
                    {
                        if (sNodeIdx == sRangeSize - 1 && !sSecondaryFound[j])
                        {
                            sDefaultNodeExec = true;
                        }
                    }
                }
            }
            sNodeIdx++;
        }

        List<Integer> sIntersectList = new ArrayList<Integer>(sPrimaryNodeIndexList);
        sIntersectList.retainAll(sSubNodeIndexList);
        if (aShardValueCnt == 0)
        {
            // PROJ-2690 첫번째 샤드값이 null인 경우에는 서브샤드값의 결과만 포함시킨다.
            sIntersectList.addAll(sSubNodeIndexList);
        }
        for (int sEach : sIntersectList) // 공통되는 node index를 node id로 바꾼다.
        {
            sResult.add(mShardRangeList.getRangeList().get(sEach).getNode());
        }

        if (sResult.size() == 0 || sDefaultNodeExec)
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

        shard_log("(COMPOSITE SHARDING RESULT) shardValue={0}, nodeName={1}",
                  new Object[] { aShardValueList, sResult });

        return sResult;
    }

    @SuppressWarnings("unchecked")
    private boolean isInTheRange(Comparable<?> aShardingValue, ShardSplitMethod aShardSplitMethod,
                                 Range aRange)
    {
        boolean sFounded;

        if (aShardSplitMethod == ShardSplitMethod.LIST)
        {
            sFounded =  aRange.isEndedBy(aShardingValue);
        }
        else
        {
            /* PROJ-2690 composite인 경우에는 subkey값이 있기 때문에 containsEndedBy을 사용하지 않고
               min, max를 다 비교한다. */
            sFounded = aRange.containsEqualAndLessThan(aShardingValue);
        }

        return sFounded;
    }
}
