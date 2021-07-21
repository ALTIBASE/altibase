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

package Altibase.jdbc.driver.sharding.core;

import Altibase.jdbc.driver.cm.CmShardAnalyzeResult;

public class ShardValueInfoPrimaryStrategy implements ShardValueInfoStrategy
{
    private CmShardAnalyzeResult mShardAnalyzeResult;

    public ShardValueInfoPrimaryStrategy(CmShardAnalyzeResult aAnalyzeResult)
    {
        this.mShardAnalyzeResult = aAnalyzeResult;
    }

    public int getShardValueCount()
    {
        return mShardAnalyzeResult.getShardValueCount();
    }

    public ShardSplitMethod getShardSplitMethod()
    {
        return mShardAnalyzeResult.getShardSplitMethod();
    }

    public void addShardValue(ShardValueInfo aShardValueInfo)
    {
        mShardAnalyzeResult.addShardValue(aShardValueInfo);
    }

}
