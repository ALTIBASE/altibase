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

package Altibase.jdbc.driver.sharding.core;

import Altibase.jdbc.driver.ex.Error;

import java.io.Reader;
import java.sql.*;

public abstract class AbstractShardingPreparedStatement extends AltibaseShardingStatement implements PreparedStatement
{
    AbstractShardingPreparedStatement(AltibaseShardingConnection aConn,
                                      int aResultSetType,
                                      int aResultSetConcurrency,
                                      int aResultSetHoldability) throws SQLException
    {
        super(aConn, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
    }

    @Override
    public void setNCharacterStream(int aParameterIndex, Reader aValue, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNCharacterStream(int aParameterIndex, Reader aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setRowId(int aParameterIndex, RowId aRowId) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNClob(int aParameterIndex, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNClob(int aParameterIndex, NClob aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNClob(int aParameterIndex, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setSQLXML(int aParameterIndex, SQLXML aXmlObject) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
