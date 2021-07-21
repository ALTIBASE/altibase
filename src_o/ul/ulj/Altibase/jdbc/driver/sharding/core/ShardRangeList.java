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

import Altibase.jdbc.driver.sharding.util.Range;

import java.util.LinkedList;
import java.util.List;

/**
 * 샤드 범위객체를 포함하고 있는 클래스. <br>
 *     mIsPrimaryRangeChanged는 컴포지트 샤드키를 위한 플래그이다.
 */
public class ShardRangeList
{
    private LinkedList<ShardRange> mRangeList = new LinkedList<ShardRange>();
    private boolean                mIsPrimaryRangeChanged;

    public void addRange(ShardRange aShardRange)
    {
        mRangeList.add(aShardRange);
    }

    public List<ShardRange> getRangeList()
    {
        return mRangeList;
    }

    public Range getCurrRange()
    {
        if (mRangeList.size() == 0)
        {
            return null;
        }
        else
        {
            return mRangeList.getLast().getRange();
        }
    }

    public Range getCurrSubRange()
    {
        if (mIsPrimaryRangeChanged || mRangeList.size() == 0)
        {
            mIsPrimaryRangeChanged = false;
            return null;
        }
        else
        {
            return mRangeList.getLast().getSubRange();
        }
    }

    public void setPrimaryRangeChanged(boolean aPrimaryRangeChanged)
    {
        this.mIsPrimaryRangeChanged = aPrimaryRangeChanged;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("ShardRangeList{");
        sSb.append("mRangeList=").append(mRangeList);
        sSb.append(", mIsPrimaryRangeChanged=").append(mIsPrimaryRangeChanged);
        sSb.append('}');
        return sSb.toString();
    }
}
