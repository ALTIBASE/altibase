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
import Altibase.jdbc.driver.sharding.strategy.StandardShardingStrategy;

import java.sql.SQLException;
import java.util.List;

public class SimpleShardKeyRoutingEngine extends ShardKeyBaseRoutingEngine
{

    public SimpleShardKeyRoutingEngine(CmProtocolContextShardStmt aShardStmtCtx)
    {
        super(aShardStmtCtx);
        mShardingStrategy = new StandardShardingStrategy(mShardRangeList, mShardSplitMethod, getDefaultNode());
    }

    public List<DataNode> route(String aSql, List<Column> aParameters) throws SQLException
    {
        CmShardAnalyzeResult sShardAnalyzeResult = mShardStmtCtx.getShardAnalyzeResult();
        List<Comparable<?>> sShardingValues = getShardValues(aParameters, sShardAnalyzeResult.getShardValueInfoList(),
                                                             mShardSplitMethod, mShardKeyDataType, false);

        return mShardingStrategy.doSharding(sShardingValues);
    }

    @Override
    public String toString()
    {
        return "SimpleShardKeyRoutingEngine{" + "mShardParamIdx=" + mShardParamIdx
               + ", mShardSplitMethod=" + mShardSplitMethod
               + ", mShardKeyDataType=" + mShardKeyDataType
               + '}';
    }
}
