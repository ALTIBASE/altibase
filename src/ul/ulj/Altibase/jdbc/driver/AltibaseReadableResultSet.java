/*
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

package Altibase.jdbc.driver;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.sql.*;

import Altibase.jdbc.driver.datatype.RowHandle;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

abstract class AltibaseReadableResultSet extends AltibaseResultSet
{
    /**
     * Fetch한 데이타에 접근하기 위한 handle을 얻는다.
     *
     * @return row 데이타에 접근하기 위한 handle
     */
    abstract protected RowHandle rowHandle();



    // #region scrollable interface 공통 구현

    public final boolean first() throws SQLException
    {
        beforeFirst();
        return next();
    }

    public final boolean last() throws SQLException
    {
        afterLast();
        return previous();
    }

    public final boolean relative(int aRows) throws SQLException
    {
        throwErrorForForwardOnly();
        if (isBeforeFirst() || isAfterLast())
        {
            Error.throwSQLException(ErrorDef.NO_CURRENT_ROW); 
        }

        if (aRows == 0)
        {
            return true;
        }

        int sNewRowPos = getRow() + aRows;
        if (sNewRowPos < 0)
        {
            sNewRowPos = 0;
        }
        return absolute(sNewRowPos);
    }

    public boolean rowUpdated() throws SQLException
    {
        throwErrorForClosed();
        return false;
    }

    public boolean rowInserted() throws SQLException
    {
        throwErrorForClosed();
        return false;
    }

    public boolean rowDeleted() throws SQLException
    {
        throwErrorForClosed();
        return false;
    }

    // #endregion



    // #region updatable interface 기본 구현
    // updatable은 AltibaseUpdatableResultSet으로 감싸서 만들므로,
    // AltibaseReadableResultSet은 모두 READ_ONLY라고 본다.

    public int getConcurrency() throws SQLException
    {
        throwErrorForClosed();
        return CONCUR_READ_ONLY;
    }

    public void updateNull(int columnIndex) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateBoolean(int columnIndex, boolean x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateByte(int columnIndex, byte x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateShort(int columnIndex, short x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateInt(int columnIndex, int x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateLong(int columnIndex, long x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateFloat(int columnIndex, float x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateDouble(int columnIndex, double x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateBigDecimal(int columnIndex, BigDecimal x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateString(int columnIndex, String x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateBytes(int columnIndex, byte[] x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateDate(int columnIndex, Date x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateTime(int columnIndex, Time x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateTimestamp(int columnIndex, Timestamp x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateAsciiStream(int columnIndex, InputStream x, long length) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateBinaryStream(int columnIndex, InputStream x, long length) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateCharacterStream(int columnIndex, Reader x, long length) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateObject(int columnIndex, Object x, int scale) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateObject(int columnIndex, Object x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void insertRow() throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateRow() throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void deleteRow() throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void cancelRowUpdates() throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void moveToInsertRow() throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void moveToCurrentRow() throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateBlob(int columnIndex, Blob x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateClob(int columnIndex, Clob x) throws SQLException
    {
        throwErrorForReadOnly();
    }

    @Override
    public void updateClob(int aColumnIndex, Reader aReader, long aLength) throws SQLException
    {
        throwErrorForReadOnly();
    }

    @Override
    public void updateNString(int aColumnIndex, String aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateBlob(int aColumnIndex, InputStream aStream) throws SQLException
    {
        throwErrorForReadOnly();
    }

    public void updateBlob(String aColumnName, InputStream aStream) throws SQLException
    {
        throwErrorForReadOnly();
    }

    @Override
    public void updateBlob(int aColumnIndex, InputStream aStream, long aLength) throws SQLException
    {
        throwErrorForReadOnly();
    }

    // #endregion
}
