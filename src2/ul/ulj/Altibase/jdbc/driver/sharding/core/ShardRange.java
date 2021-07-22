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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * range와 subrange를 포함하는 데이터 노드의 정보를 나타내는 클래스
 */
public class ShardRange
{
    private List<Range<? extends Comparable>> mRanges;
    private DataNode                          mNode;

    public ShardRange(DataNode aNode, Range<? extends Comparable>... aRanges)
    {
        mRanges = new ArrayList<Range<? extends Comparable>>();
        mNode = aNode;
        Collections.addAll(mRanges, aRanges);
    }

    public Range<? extends Comparable> getRange()
    {
        return mRanges.get(0);
    }

    public Range<? extends Comparable> getSubRange()
    {
        return (mRanges.size() < 2) ? null : mRanges.get(1);
    }

    public DataNode getNode()
    {
        return mNode;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("ShardRange{");
        sSb.append("mRanges=").append(mRanges);
        sSb.append(", mNodeName='").append(mNode.getNodeName()).append('\'');
        sSb.append('}');
        return sSb.toString();
    }
}
