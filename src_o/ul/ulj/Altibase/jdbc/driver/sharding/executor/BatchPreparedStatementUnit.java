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

package Altibase.jdbc.driver.sharding.executor;

import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.sharding.core.DataNode;

import java.sql.PreparedStatement;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class BatchPreparedStatementUnit implements BaseStatementUnit
{
    private final DataNode              mNode;
    private final PreparedStatement     mStatement;
    private       List<Column>          mParameters;
    private final Map<Integer, Integer> mAddBatchCallTimesMap;
    private       int                   mActualCallAddBatchTimes;

    public BatchPreparedStatementUnit(DataNode aNode, PreparedStatement aStatement,
                                      List<Column> aParameters)
    {
        this.mNode = aNode;
        this.mStatement = aStatement;
        this.mParameters = aParameters;
        this.mAddBatchCallTimesMap =  new LinkedHashMap<Integer, Integer>();
    }

    public DataNode getNode()
    {
        return mNode;
    }

    public PreparedStatement getStatement()
    {
        return mStatement;
    }

    public void mapAddBatchCount(final int aJdbcAddBatchTimes)
    {
        mAddBatchCallTimesMap.put(aJdbcAddBatchTimes, mActualCallAddBatchTimes++);
    }

    Map<Integer, Integer> getAddBatchCallTimesMap()
    {
        return mAddBatchCallTimesMap;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("BatchPreparedStatementUnit{");
        sSb.append("mNode=").append(mNode);
        sSb.append(", mStatement=").append(mStatement);
        sSb.append(", mParameters=").append(mParameters);
        sSb.append(", mActualCallAddBatchTimes=").append(mActualCallAddBatchTimes);
        sSb.append('}');
        return sSb.toString();
    }
}
