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

import Altibase.jdbc.driver.sharding.core.DataNode;

public class SQLExecutionUnit implements Comparable<SQLExecutionUnit>
{
    private DataNode mNode;
    private String   mSql;

    public SQLExecutionUnit(DataNode aNode, String aSql)
    {
        mNode = aNode;
        mSql = aSql;
    }

    public String getSql()
    {
        return mSql;
    }

    public DataNode getNode()
    {
        return mNode;
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

        SQLExecutionUnit sThat = (SQLExecutionUnit)aObject;

        if (!mNode.equals(sThat.mNode))
        {
            return false;
        }

        return mSql.equals(sThat.mSql);
    }

    @Override
    public int hashCode()
    {
        int sResult = mNode.hashCode();
        sResult = 31 * sResult + mSql.hashCode();
        return sResult;
    }

    public int compareTo(SQLExecutionUnit aTarget)
    {
        return mNode.compareTo(aTarget.mNode);
    }
}
