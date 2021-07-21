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

import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;
import Altibase.jdbc.driver.cm.CmShardAnalyzeResult;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.sharding.core.DataNode;
import Altibase.jdbc.driver.sharding.core.ShardKeyDataType;
import Altibase.jdbc.driver.sharding.core.ShardSplitMethod;
import Altibase.jdbc.driver.sharding.strategy.CompositeShardingStrategy;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

public class CompositeShardKeyRoutingEngine extends ShardKeyBaseRoutingEngine
{
    private ShardSplitMethod           mShardSubSplitMethod;
    private ShardKeyDataType           mShardSubKeyDataType;

    public CompositeShardKeyRoutingEngine(CmProtocolContextShardStmt aShardStmtCtx)
    {
        super(aShardStmtCtx);
        mShardSubSplitMethod = mShardStmtCtx.getShardAnalyzeResult().getShardSubSplitMethod();
        mShardSubKeyDataType = mShardStmtCtx.getShardAnalyzeResult().getShardSubKeyDataType();

        List<ShardSplitMethod> sShardSplitMethodList = new ArrayList<ShardSplitMethod>();
        sShardSplitMethodList.add(mShardSplitMethod);
        sShardSplitMethodList.add(mShardSubSplitMethod);
        mShardingStrategy = new CompositeShardingStrategy(mShardRangeList, getDefaultNode(), sShardSplitMethodList,
                                                          mShardValueCnt, mShardSubValueCnt);
    }

    public List<DataNode> route(String aSql, List<Column> aParameters) throws SQLException
    {
        CmShardAnalyzeResult sShardAnalyzeResult = mShardStmtCtx.getShardAnalyzeResult();
        List<Comparable<?>> sShardValues = getShardValues(aParameters,
                                                          sShardAnalyzeResult.getShardValueInfoList(),
                                                          mShardSplitMethod,
                                                          mShardKeyDataType,
                                                          false);

        List<Comparable<?>> sShardSubValues = getShardValues(aParameters,
                                                             sShardAnalyzeResult.getShardSubValueInfoList(),
                                                             mShardSubSplitMethod,
                                                             mShardSubKeyDataType,
                                                             true);
        List<Comparable<?>> sShardingValues = new ArrayList<Comparable<?>>(sShardValues);
        sShardingValues.addAll(sShardSubValues);

        return mShardingStrategy.doSharding(sShardingValues);
    }

    @Override
    public String toString()
    {
        return "CompositeShardKeyRoutingEngine{" + "mShardStmtCtx=" + mShardStmtCtx
               + ", mShardingStrategy=" + mShardingStrategy
               + ", mShardSplitMethod=" + mShardSplitMethod
               + ", mShardKeyDataType=" + mShardKeyDataType
               + ", mShardSubSplitMethod=" + mShardSubSplitMethod
               + ", mShardSubKeyDataType=" + mShardSubKeyDataType
               + ", mShardParamIdx=" + mShardParamIdx
               + ", mShardSubParamIdx=" + mShardSubParamIdx
               + '}';
    }
}
