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

import java.lang.reflect.Method;
import java.sql.SQLException;
import java.util.Arrays;

/**
 *  JDBC 메소드 콜을 리플렉션을 이용해 콜한다.
 *
 *  @author yjpark
 */
public class JdbcMethodInvocation
{
    private final Method mMethod;
    private final Object[] mArguments;

    JdbcMethodInvocation(Method aMethod, Object[] aArguments)
    {
        this.mMethod = aMethod;
        this.mArguments = aArguments;
    }

    void invoke(final Object aTarget) throws SQLException
    {
        try
        {
            mMethod.invoke(aTarget, mArguments);
        }
        catch (Exception aEx)
        {
            Error.throwSQLException(ErrorDef.SHARD_JDBC_METHOD_INVOKE_ERROR, aEx.getMessage());
        }
    }

    Object[] getArguments()
    {
        return mArguments;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("JdbcMethodInvocation{");
        sSb.append("mMethod=").append(mMethod.getName());
        sSb.append(", mArguments=").append(Arrays.toString(mArguments));
        sSb.append('}');
        return sSb.toString();
    }
}
