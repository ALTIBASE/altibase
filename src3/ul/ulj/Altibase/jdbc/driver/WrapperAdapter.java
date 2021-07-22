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

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.SQLException;
import java.sql.Wrapper;

public abstract class WrapperAdapter implements Wrapper
{
    @Override
    public <T> T unwrap(Class<T> aIface) throws SQLException
    {
        if (aIface.isAssignableFrom(getClass()))
        {
            return aIface.cast(this);
        }

        throw Error.createSQLException(ErrorDef.CANNOT_BE_UNWRAPPED, getClass().getName(),
                                       aIface.getName());
    }

    @Override
    public boolean isWrapperFor(Class<?> aIface) throws SQLException
    {
        return aIface.isAssignableFrom(getClass());
    }
}
