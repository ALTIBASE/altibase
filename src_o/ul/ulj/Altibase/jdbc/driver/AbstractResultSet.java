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
 * java.sql.ResultSet 인터페이스 중 구현하지 않는 인터페이스를 정의한다.
 */
public abstract class AbstractResultSet extends WrapperAdapter implements ResultSet
{
    @Override
    public RowId getRowId(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public RowId getRowId(String aColumnName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateRowId(int aColumnIndex, RowId aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateRowId(String aColumnName, RowId aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public NClob getNClob(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public NClob getNClob(String aColumnName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public SQLXML getSQLXML(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public SQLXML getSQLXML(String aColumnName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNClob(int aColumnIndex, NClob aNClob) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNClob(String aColumnName, NClob aNClob) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNClob(int aColumnIndex, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNClob(String aColumnName, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateSQLXML(int aColumnIndex, SQLXML aXmlObject) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateSQLXML(String aColumnName, SQLXML aXmlObject) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNClob(int aColumnIndex, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNClob(String aColumnName, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNCharacterStream(int aColumnIndex, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNCharacterStream(String aColumnName, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNCharacterStream(int aColumnIndex, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNCharacterStream(String aColumnName, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Reader getNCharacterStream(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Reader getNCharacterStream(String aColumnName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
