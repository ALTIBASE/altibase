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

import Altibase.jdbc.driver.sharding.routing.SQLExecutionUnit;

import java.sql.Statement;

public class StatementUnit implements BaseStatementUnit
{
    private SQLExecutionUnit mSqlExecutionUnit;
    private Statement        mStatement;

    public StatementUnit(SQLExecutionUnit aSqlExecutionUnit, Statement aStatement)
    {
        this.mSqlExecutionUnit = aSqlExecutionUnit;
        this.mStatement = aStatement;
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
        final StringBuilder sSb = new StringBuilder("StatementUnit{");
        sSb.append("mSqlExecutionUnit=").append(mSqlExecutionUnit);
        sSb.append(", mStatement=").append(mStatement);
        sSb.append('}');
        return sSb.toString();
    }
}
