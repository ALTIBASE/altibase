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
import Altibase.jdbc.driver.sharding.routing.SQLExecutionUnit;

import java.sql.CallableStatement;
import java.sql.Statement;
import java.util.List;

public class CallableStatementUnit implements BaseStatementUnit
{
    private SQLExecutionUnit  mSqlExecutionUnit;
    private CallableStatement mStatement;
    private List<Column>      mParameters;

    public CallableStatementUnit(SQLExecutionUnit aSqlExecutionUnit,
                                 CallableStatement aStatement,
                                 List<Column> aParameters)
    {
        this.mSqlExecutionUnit = aSqlExecutionUnit;
        this.mStatement = aStatement;
        this.mParameters = aParameters;
    }


    public SQLExecutionUnit getSqlExecutionUnit()
    {
        return mSqlExecutionUnit;
    }

    public Statement getStatement()
    {
        return mStatement;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("CallableStatementUnit{");
        sSb.append("mSqlExecutionUnit=").append(mSqlExecutionUnit);
        sSb.append(", mStatement=").append(mStatement);
        sSb.append(", mParameters=").append(mParameters);
        sSb.append('}');
        return sSb.toString();
    }
}
