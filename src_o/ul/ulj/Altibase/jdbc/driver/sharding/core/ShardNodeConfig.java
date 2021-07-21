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

import java.util.*;

/**
 * 샤드 데이터노드 구성정보를 나타내는 객체.
 */
public class ShardNodeConfig
{
    private boolean         mIsTestEnable;
    private int             mNodeCount;
    private List<DataNode>  mDataNodes;

    public ShardNodeConfig()
    {
        mDataNodes = new ArrayList<DataNode>();
    }

    public void setTestEnable(boolean aTestEnable)
    {
        mIsTestEnable = aTestEnable;
    }

    public void setNodeCount(int aNodeCount)
    {
        mNodeCount = aNodeCount;
    }

    public int getNodeCount()
    {
        return mNodeCount;
    }

    public List<DataNode> getDataNodes()
    {
        return mDataNodes;
    }

    public void addNode(DataNode aDataNode)
    {
        mDataNodes.add(aDataNode);
    }

    public DataNode getNode(int aNodeId)
    {
        for (DataNode sEach : mDataNodes)
        {
            if (sEach.getNodeId() == aNodeId)
            {
                return sEach;
            }
        }

        return null;
    }

    public DataNode getNodeByName(String aNodeName)
    {
        for (DataNode sEach : mDataNodes)
        {
            if (sEach.getNodeName().equals(aNodeName))
            {
                return sEach;
            }
        }

        return null;
    }

    List<DataNode> getTouchedNodeList()
    {
        List<DataNode> sResult = new ArrayList<DataNode>();
        for (DataNode sEach : mDataNodes)
        {
            if (sEach.isTouched())
            {
                sResult.add(sEach);
            }
        }

        return sResult;
    }

    void setTouchedToAllNodes()
    {
        for (DataNode sEach : mDataNodes)
        {
            sEach.setTouched(true);
        }

    }

    public void setDataNodes(List<DataNode> aDataNodes)
    {
        mDataNodes = aDataNodes;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("ShardNodeConfig{");
        sSb.append("mIsTestEnable=").append(mIsTestEnable);
        sSb.append(", mNodeCount=").append(mNodeCount);
        sSb.append(", mDataNodes=").append(mDataNodes);
        sSb.append('}');

        return sSb.toString();
    }
}
