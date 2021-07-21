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

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class JdbcMethodInvoker
{
    private final List<JdbcMethodInvocation> mJdbcMethodInvocations = new ArrayList<>();

    final void recordMethodInvocation(Class<?> aTargetClass, String aMethodName, Class<?>[]
            aArgumentTypes, Object[] aArguments) throws SQLException
    {
        try
        {
            JdbcMethodInvocation sMethodInvocation = new JdbcMethodInvocation(aTargetClass.getMethod(
                    aMethodName, aArgumentTypes), aArguments);
            mJdbcMethodInvocations.add(sMethodInvocation);
            shard_log("(RECORD METHOD INVOCATION) {0}", sMethodInvocation);
        }
        catch (NoSuchMethodException aEx)
        {
            Error.throwSQLException(ErrorDef.SHARD_JDBC_METHOD_INVOKE_ERROR, aEx.getMessage());
        }
    }

    final void replayMethodsInvocation(Object aTarget) throws SQLException
    {
        for (JdbcMethodInvocation sEach : mJdbcMethodInvocations)
        {
            shard_log("(REPLAY METHODS INVOCATION) {0}", sEach);
            sEach.invoke(aTarget);
        }
    }

    void throwSQLExceptionIfNecessary(List<SQLException> aExceptions) throws SQLException
    {
        if (aExceptions.isEmpty())
        {
            return;
        }

        SQLException sException = new SQLException();
        for (SQLException sEach : aExceptions)
        {
            sException.setNextException(sEach);
        }

        throw sException;
    }
}
