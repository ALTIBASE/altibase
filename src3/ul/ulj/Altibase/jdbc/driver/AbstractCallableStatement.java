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

import java.io.Reader;
import java.sql.*;

import Altibase.jdbc.driver.ex.Error;

/**
 * java.sql.CallableStatement 인터페이스 중 구현하지 않는 인터페이스를 정의한다.
 */
public abstract class AbstractCallableStatement extends AltibasePreparedStatement implements CallableStatement
{
    public AbstractCallableStatement(AltibaseConnection aConnection,
                                     String aSql,
                                     int aResultSetType,
                                     int aResultSetConcurrency,
                                     int aResultSetHoldability) throws SQLException
    {
        super(aConnection, aSql, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
    }

    @Override
    public RowId getRowId(int aParameterIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public RowId getRowId(String aParameterName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setRowId(String aParameterName, RowId aRowId) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNCharacterStream(String aParameterName, Reader aValue, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNClob(String aParameterName, NClob aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNClob(String aParameterName, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNCharacterStream(String aParameterName, Reader aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNClob(String aParameterName, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Reader getNCharacterStream(int aParameterIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public NClob getNClob(int aParameterIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public NClob getNClob(String aParameterName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setSQLXML(String aParameterName, SQLXML aXmlObject) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public SQLXML getSQLXML(int aParameterIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public SQLXML getSQLXML(String aParameterName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Reader getNCharacterStream(String aParameterName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
