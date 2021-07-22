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

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.List;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class PreparedStatementExecutor
{
    private ExecutorEngine                    mExecutorEngine;
    private ExecuteCallback<ResultSet>        mResultSetCallback;

    public PreparedStatementExecutor(ExecutorEngine aExecutorEngine)
    {
        this.mExecutorEngine = aExecutorEngine;
        mResultSetCallback = new ExecuteCallback<ResultSet>()
        {
            public ResultSet execute(Statement aStatement) throws SQLException
            {
                shard_log("(NODE EXECUTEQUERY) {0}", aStatement);
                return ((PreparedStatement)aStatement).executeQuery();
            }
        };
    }

    public List<ResultSet> executeQuery(List<Statement> aStatements) throws SQLException
    {
        return mExecutorEngine.executeStatement(aStatements, mResultSetCallback);
    }

    public int executeUpdate(List<Statement> aStatements) throws SQLException
    {
        List<Integer> sResults = mExecutorEngine.executeStatement(aStatements, new ExecuteCallback<Integer>()
        {
            public Integer execute(Statement aStatement) throws SQLException
            {
                shard_log("(NODE EXECUTEUPDATE) {0}", aStatement);
                return ((PreparedStatement)aStatement).executeUpdate();
            }
        });

        return accumulate(sResults);
    }

    private int accumulate(List<Integer> aResults)
    {
        int sResult = 0;

        for (Integer sEach : aResults)
        {
            sResult += (sEach == null) ? 0 : sEach;
        }

        return sResult;
    }

    public boolean execute(List<Statement> aStatements) throws SQLException
    {
        List<Boolean> sResult = mExecutorEngine.executeStatement(aStatements, new ExecuteCallback<Boolean>()
        {
            public Boolean execute(Statement aStatement) throws SQLException
            {
                shard_log("(NODE EXECUTE) {0}", aStatement);
                return ((PreparedStatement)aStatement).execute();
            }
        });

        if (sResult == null || sResult.isEmpty() || sResult.get(0) == null)
        {
            return false;
        }

        return sResult.get(0);
    }
}
