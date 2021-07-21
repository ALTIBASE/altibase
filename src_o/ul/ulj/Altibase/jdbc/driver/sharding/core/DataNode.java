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

/**
 * Shard DataNode를 나타내며 각 정보는 통신 프로토콜을 통해 받아온다.
 */
public class DataNode implements Comparable<DataNode>
{
    private long          mSMN;
    private int           mNodeId;
    private String        mNodeName;
    private String        mServerIp;
    private int           mPortNo;
    private String        mAlternativeServerIp;
    private int           mAlternativePortNo;
    private boolean       mTouched;

    public DataNode()
    {
    }

    public long getSMN()
    {
        return mSMN;
    }

    public void setSMN(long aSMN)
    {
        this.mSMN = aSMN;
    }

    public int getNodeId()
    {
        return mNodeId;
    }

    public void setNodeId(int aNodeId)
    {
        this.mNodeId = aNodeId;
    }

    public String getNodeName()
    {
        return mNodeName;
    }

    public void setNodeName(String aNodeName)
    {
        this.mNodeName = aNodeName;
    }

    public void setServerIp(String aServerIp)
    {
        this.mServerIp = getTrimmedString(aServerIp);
    }

    public String getServerIp()
    {
        return mServerIp;
    }

    private String getTrimmedString(String aServerIp)
    {
        char[] sDst = new char[aServerIp.length()];
        char[] sSrc = aServerIp.toCharArray();
        for (int i = 0; i < aServerIp.length(); i++)
        {
            if (sSrc[i] == Character.MIN_VALUE)
            {
                break;
            }
            else
            {
                sDst[i] = sSrc[i];
            }
        }
        return new String(sDst).trim();
    }

    public void setPortNo(int aPortNo)
    {
        this.mPortNo = aPortNo;
    }

    public int getPortNo()
    {
        return mPortNo;
    }

    public void setAlternativeServerIp(String aAlternativeServerIp)
    {
        this.mAlternativeServerIp = getTrimmedString(aAlternativeServerIp);
    }

    public String getAlternativeServerIp()
    {
        return mAlternativeServerIp;
    }

    public void setAlternativePortNo(int aAlternativePortNo)
    {
        this.mAlternativePortNo = aAlternativePortNo;
    }

    public int getAlternativePortNo()
    {
        return mAlternativePortNo;
    }

    boolean isTouched()
    {
        return mTouched;
    }

    public void setTouched(boolean aTouched)
    {
        mTouched = aTouched;
    }

    @Override
    public boolean equals(Object aObject)
    {
        if (this == aObject)
        {
            return true;
        }
        if (aObject == null || getClass() != aObject.getClass())
        {
            return false;
        }

        DataNode dataNode = (DataNode)aObject;
        if (mNodeId != dataNode.mNodeId)
        {
            return false;
        }

        return mNodeName.equalsIgnoreCase(dataNode.mNodeName);
    }

    @Override
    public int hashCode()
    {
        int sResult = mNodeId;
        sResult = 31 * sResult + mNodeName.toUpperCase().hashCode();
        return sResult;
    }

    public int compareTo(DataNode aNode)
    {
        return (mNodeId < aNode.mNodeId ? -1 : (mNodeId == aNode.mNodeId ? 0 : 1));
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("DataNode{");
        sSb.append("mNodeId=").append(mNodeId);
        sSb.append(", mNodeName='").append(mNodeName).append('\'');
        sSb.append(", mServerIp='").append(mServerIp).append('\'');
        sSb.append(", mPortNo=").append(mPortNo);
        sSb.append(", mAlternativeServerIp='").append(mAlternativeServerIp).append('\'');
        sSb.append(", mAlternativePortNo=").append(mAlternativePortNo);
        sSb.append(", mTouched=").append(mTouched);
        sSb.append('}');
        return sSb.toString();
    }
}
