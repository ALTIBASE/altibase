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

import java.sql.*;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

/**
 * java.sql.Connection 인터페이스 중 구현하지 않는 인터페이스를 정의한다.
 */
public abstract class AbstractConnection extends WrapperAdapter implements Connection
{
    protected boolean mIsClosed = true;

    @Override
    public Clob createClob() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Blob createBlob() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setSchema(String aSchema) throws SQLException
    {
        throwErrorForClosed();
        // PROJ-2707 스펙에 따르면 스키마를 지원하지 않을 경우 그냥 무시하라고 되어 있다.
    }

    @Override
    public String getSchema() throws SQLException
    {
        throwErrorForClosed();
        return null;
    }

    @Override
    public NClob createNClob() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public SQLXML createSQLXML() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Array createArrayOf(String aTypeName, Object[] aElements) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public Struct createStruct(String aTypeName, Object[] aAttributes) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    void throwErrorForClosed() throws SQLException
    {
        if (mIsClosed)
        {
            Error.throwSQLException(ErrorDef.CLOSED_CONNECTION);
        }
    }
}
