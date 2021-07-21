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

package Altibase.jdbc.driver.cm;

import Altibase.jdbc.driver.sharding.core.ShardKeyDataType;
import Altibase.jdbc.driver.sharding.core.ShardRangeList;
import Altibase.jdbc.driver.sharding.core.ShardSplitMethod;
import Altibase.jdbc.driver.sharding.core.ShardValueInfo;

import java.util.ArrayList;
import java.util.List;

public class CmShardAnalyzeResult extends CmStatementIdResult
{
    public static final byte                 MY_OP                  = CmShardOperation.DB_OP_SHARD_ANALYZE_RESULT;
    private             ShardSplitMethod     mShardSplitMethod;
    private             ShardSplitMethod     mShardSubSplitMethod;
    private             ShardKeyDataType     mShardKeyDataType;
    private             ShardKeyDataType     mShardSubKeyDataType;
    private             int                  mShardDefaultNodeID;
    private             boolean              mShardSubKeyExists;
    private             boolean              mShardCanMerge;
    private             int                  mShardValueCount;
    private             int                  mShardSubValueCount;
    private             List<ShardValueInfo> mShardValueInfoList    = new ArrayList<ShardValueInfo>();
    private             List<ShardValueInfo> mShardSubValueInfoList = new ArrayList<ShardValueInfo>();
    private             ShardRangeList       mShardRangeList;
    private             boolean              mIsCoordQuery;         // coordinate Äõ¸® ¿©ºÎ

    protected byte getResultOp()
    {
        return MY_OP;
    }

    public void setShardSplitMethod(ShardSplitMethod aValue)
    {
        mShardSplitMethod = aValue;
    }

    public ShardSplitMethod getShardSplitMethod()
    {
        return mShardSplitMethod;
    }

    public void setShardKeyDataType(ShardKeyDataType aValue)
    {
        mShardKeyDataType = aValue;
    }

    public ShardKeyDataType getShardKeyDataType()
    {
        return mShardKeyDataType;
    }

    public void setShardDefaultNodeID(int aShardDefaultNodeID)
    {
        mShardDefaultNodeID = aShardDefaultNodeID;
    }

    public void setShardSubKeyExists(byte aShardSubKeyExists)
    {
        mShardSubKeyExists = (aShardSubKeyExists == (byte)1);
    }

    public boolean isShardSubKeyExists()
    {
        return mShardSubKeyExists;
    }

    public void setShardCanMerge(byte aShardCanMerge)
    {
        mShardCanMerge = (aShardCanMerge == (byte)1);
        if (!mShardCanMerge)
        {
            mIsCoordQuery = true;
        }
    }

    public void setShardValueCount(int aShardValueCount)
    {
        mShardValueCount = aShardValueCount;
    }

    public int getShardValueCount()
    {
        return mShardValueCount;
    }

    public ShardSplitMethod getShardSubSplitMethod()
    {
        return mShardSubSplitMethod;
    }

    public void setShardSubSplitMethod(ShardSplitMethod aValue)
    {
        mShardSubSplitMethod = aValue;
    }

    public ShardKeyDataType getShardSubKeyDataType()
    {
        return mShardSubKeyDataType;
    }

    public void setShardSubKeyDataType(ShardKeyDataType aValue)
    {
        mShardSubKeyDataType = aValue;
    }

    public int getShardSubValueCount()
    {
        return mShardSubValueCount;
    }

    public void setShardSubValueCount(int aValue)
    {
        mShardSubValueCount = aValue;
    }

    public void addShardValue(ShardValueInfo aShardValueInfo)
    {
        mShardValueInfoList.add(aShardValueInfo);
    }

    public List<ShardValueInfo> getShardValueInfoList()
    {
        return mShardValueInfoList;
    }

    public void addShardSubValue(ShardValueInfo aShardValueInfo)
    {
        mShardSubValueInfoList.add(aShardValueInfo);
    }

    public List<ShardValueInfo> getShardSubValueInfoList()
    {
        return mShardSubValueInfoList;
    }

    public void setShardRangeInfo(ShardRangeList aShardRangeList)
    {
        this.mShardRangeList = aShardRangeList;
    }

    public ShardRangeList getShardRangeList()
    {
        return mShardRangeList;
    }

    public boolean canMerge()
    {
        return mShardCanMerge;
    }

    public void setShardCoordinate(boolean aShardCoordinate)
    {
        this.mIsCoordQuery = aShardCoordinate;
    }

    public int getShardRangeInfoCnt()
    {
        return mShardRangeList.getRangeList().size();
    }

    public int getShardDefaultNodeID()
    {
        return mShardDefaultNodeID;
    }

    public boolean isCoordQuery()
    {
        return mIsCoordQuery;
    }

    @Override public String toString()
    {
        final StringBuilder sSb = new StringBuilder("CmShardAnalyzeResult{");
        sSb.append("mShardSplitMethod=").append(mShardSplitMethod);
        sSb.append(", mShardSubSplitMethod=").append(mShardSubSplitMethod);
        sSb.append(", mShardKeyDataType=").append(mShardKeyDataType);
        sSb.append(", mShardSubKeyDataType=").append(mShardSubKeyDataType);
        sSb.append(", mShardDefaultNodeID=").append(mShardDefaultNodeID);
        sSb.append(", mShardSubKeyExists=").append(mShardSubKeyExists);
        sSb.append(", mShardCanMerge=").append(mShardCanMerge);
        sSb.append(", mShardValueCount=").append(mShardValueCount);
        sSb.append(", mShardSubValueCount=").append(mShardSubValueCount);
        sSb.append(", mShardValueInfoList=").append(mShardValueInfoList);
        sSb.append(", mShardSubValueInfoList=").append(mShardSubValueInfoList);
        sSb.append(", mIsCoordQuery=").append(mIsCoordQuery);
        sSb.append('}');
        return sSb.toString();
    }
}
