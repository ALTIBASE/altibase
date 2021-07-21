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
 */

package Altibase.jdbc.driver;

import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.ex.Error;

/**
 * java.sql.Statement 인터페이스 중 구현하지 않는 인터페이스를 정의한다.
 */
public abstract class AbstractStatement extends WrapperAdapter implements Statement
{
    @Override
    public void setPoolable(boolean aPoolable) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public boolean isPoolable() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
