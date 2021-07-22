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

import Altibase.jdbc.driver.AltibaseStatement;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.List;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class StatementExecutor
{
    private ExecutorEngine      mExecutorEngine;

    public StatementExecutor(ExecutorEngine aExecutorEngine)
    {
        this.mExecutorEngine = aExecutorEngine;
    }

    public int executeUpdate(List<Statement> aStatements) throws SQLException
    {
        return executeUpdate(new Updater() {
            public int executeUpdate(final Statement aStatement, final String aSql) throws SQLException
            {
                return aStatement.executeUpdate(aSql);
            }
        }, aStatements);
    }

    public int executeUpdate(final int[] aColumnIndexes, List<Statement> aStatements) throws SQLException
    {
        return executeUpdate(new Updater()
        {
            public int executeUpdate(Statement aStatement, String aSql) throws SQLException
            {
                return aStatement.executeUpdate(aSql, aColumnIndexes);
            }
        }, aStatements);
    }

    public int executeUpdate(final String[] aColumnNames, List<Statement> aStatements) throws SQLException
    {
        return executeUpdate(new Updater()
        {
            public int executeUpdate(Statement aStatement, String aSql) throws SQLException
            {
                return aStatement.executeUpdate(aSql, aColumnNames);
            }
        }, aStatements);
    }

    private int executeUpdate(final Updater aUpdater, List<Statement> aStatements) throws SQLException
    {
        List<Integer> sResults = mExecutorEngine.executeStatement(aStatements, new ExecuteCallback<Integer>()
        {
            public Integer execute(Statement aStatement) throws SQLException
            {
                shard_log("(NODE DIRECT EXECUTEUPDATE) {0}", aStatement);
                return aUpdater.executeUpdate(aStatement, ((AltibaseStatement)aStatement).getSql());
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

    public List<ResultSet> executeQuery(List<Statement> aStatements) throws SQLException
    {
        return mExecutorEngine.executeStatement(aStatements, new ExecuteCallback<ResultSet>()
        {
            public ResultSet execute(Statement aStatement) throws SQLException
            {
                shard_log("(NODE DIRECT EXECUTEQUERY) {0}", aStatement);
                return aStatement.executeQuery(((AltibaseStatement)aStatement).getSql());
            }
        });
    }

    public boolean execute(List<Statement> aStatements) throws SQLException
    {
        return execute(new Executor()
        {
            public boolean execute(Statement aStatement, String aSql) throws SQLException
            {
                return aStatement.execute(aSql);
            }
        }, aStatements);
    }

    public boolean execute(final int[] aColumnIndexes,
                           List<Statement> aStatements) throws SQLException
    {
        return execute(new Executor()
        {
            public boolean execute(Statement aStatement, String aSql) throws SQLException
            {
                return aStatement.execute(aSql, aColumnIndexes);
            }
        }, aStatements);
    }

    public boolean execute(final String[] aColumnNames, List<Statement> aStatements) throws SQLException
    {
        return execute(new Executor()
        {
            public boolean execute(Statement aStatement, String aSql) throws SQLException
            {
                return aStatement.execute(aSql, aColumnNames);
            }
        }, aStatements);
    }

    private boolean execute(final Executor aExecutor, List<Statement> aStatements) throws SQLException
    {
        List<Boolean> sResult = mExecutorEngine.executeStatement(aStatements, new ExecuteCallback<Boolean>()
        {
            public Boolean execute(Statement aStatement) throws SQLException
            {
                shard_log("(NODE DIRECT EXECUTE) {0}", aStatement);
                return aExecutor.execute(aStatement, ((AltibaseStatement)aStatement).getSql());
            }
        });

        if (sResult == null || sResult.isEmpty() || sResult.get(0) == null)
        {
            return false;
        }

        return sResult.get(0);
    }

    private interface Updater
    {
        int executeUpdate(Statement aStatement, String aSql) throws SQLException;
    }

    private interface Executor
    {
        boolean execute(Statement aStatement, String aSql) throws SQLException;
    }
}
