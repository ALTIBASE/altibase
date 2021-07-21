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

import Altibase.jdbc.driver.sharding.core.DataNode;

import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class BatchPreparedStatementExecutor
{
    private final ExecutorEngine                            mExecutorEngine;
    private final Map<DataNode, BatchPreparedStatementUnit> mBatchPreparedStatementUnitMap;
    private final int                                       mBatchCount;

    public BatchPreparedStatementExecutor(ExecutorEngine aExecutorEngine,
                                          Map<DataNode, BatchPreparedStatementUnit> aBatchPreparedStatementUnitMap,
                                          int aBatchCount)
    {
        mExecutorEngine = aExecutorEngine;
        mBatchPreparedStatementUnitMap = aBatchPreparedStatementUnitMap;
        mBatchCount = aBatchCount;
    }

    /**
     * Execute batch.
     *
     * @return execute results
     * @throws SQLException SQL exception
     */
    public int[] executeBatch() throws SQLException
    {
        List<BatchPreparedStatementUnit> sBatchStmtUnits = new ArrayList<BatchPreparedStatementUnit>(
                mBatchPreparedStatementUnitMap.values());
        List<Statement> sStmtList = new ArrayList<Statement>();
        for (BatchPreparedStatementUnit sBatchUnit : sBatchStmtUnits)
        {
            sStmtList.add(sBatchUnit.getStatement());
        }

        return accumulate(mExecutorEngine.executeStatement(sStmtList,
                          new ExecuteCallback<int[]>()
            {
                public int[] execute(final Statement aStatement) throws SQLException
                {
                    return aStatement.executeBatch();
                }
            })
        );
    }

    private int[] accumulate(List<int[]> aResults)
    {
        int[] sResult = new int[mBatchCount];
        int sCount = 0;
        for (BatchPreparedStatementUnit sEach : mBatchPreparedStatementUnitMap.values())
        {
            for (Map.Entry<Integer, Integer> sEntry : sEach.getAddBatchCallTimesMap().entrySet())
            {
                int sValue = (aResults.get(sCount) == null) ? 0 : aResults.get(sCount)[sEntry.getValue()];
                // BUG-46500 하나의 addBatch에 여러개의 노드가 실행 될 수 있기 때문에 값을 누적해야 한다.
                sResult[sEntry.getKey()] += sValue;
            }
            sCount++;
        }

        return sResult;
    }

}
