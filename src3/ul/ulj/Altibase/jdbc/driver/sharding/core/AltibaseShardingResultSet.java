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

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.net.URL;
import java.sql.*;
import java.sql.Date;
import java.util.*;

import Altibase.jdbc.driver.AbstractResultSet;
import Altibase.jdbc.driver.AltibaseStatement;
import Altibase.jdbc.driver.sharding.merger.IteratorStreamResultSetMerger;
import Altibase.jdbc.driver.sharding.merger.ResultSetMerger;
import Altibase.jdbc.driver.ex.Error;

public class AltibaseShardingResultSet extends AbstractResultSet
{
    private ResultSetMerger            mResultSetMerger;
    private List<ResultSet>            mResultSets;
    private Statement                  mStatement;
    private AltibaseShardingConnection mMetaConn;

    AltibaseShardingResultSet(List<ResultSet> aResultSets, ResultSetMerger aMerger,
                              Statement aStatement)
    {
        mResultSetMerger = aMerger;
        mResultSets = aResultSets;
        mStatement = aStatement;
        // BUG-46513 smn값을 비교하기 위해 meta connection을 주입받는다.
        mMetaConn = ((AltibaseShardingStatement)aStatement).getMetaConn();
    }

    public boolean next() throws SQLException
    {
        mMetaConn.checkCommitState();
        boolean sResult = mResultSetMerger.next();

        // BUG-46513 autocommit on 이고 smn값이 작다면 갱신해야 한다.
        if (mMetaConn.getAutoCommit() && mMetaConn.shouldUpdateShardMetaNumber())
        {
            mMetaConn.updateShardMetaNumber();
        }

        return sResult;
    }

    public void close() throws SQLException
    {
        for (ResultSet sEach : mResultSets)
        {
            sEach.close();
        }
        // BUG-46513 autocommit on 이고 smn값이 작다면 갱신해야 한다.
        if (mMetaConn.getAutoCommit() && mMetaConn.shouldUpdateShardMetaNumber())
        {
            mMetaConn.updateShardMetaNumber();
        }
    }

    public boolean wasNull() throws SQLException
    {
        return getCurrentResultSet().wasNull();
    }

    public String getString(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getString(aColumnIndex);
    }

    public String getString(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getString(aColumnName);
    }

    public boolean getBoolean(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getBoolean(aColumnIndex);
    }

    public boolean getBoolean(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getBoolean(aColumnName);
    }

    public byte getByte(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getByte(aColumnIndex);
    }

    public byte getByte(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getByte(aColumnName);
    }

    public short getShort(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getShort(aColumnIndex);
    }

    public short getShort(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getShort(aColumnName);
    }

    public int getInt(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getInt(aColumnIndex);
    }

    public int getInt(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getInt(aColumnName);
    }

    public long getLong(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getLong(aColumnIndex);
    }

    public long getLong(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getLong(aColumnName);
    }

    public float getFloat(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getFloat(aColumnIndex);
    }

    public float getFloat(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getFloat(aColumnName);
    }

    public double getDouble(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getDouble(aColumnIndex);
    }

    public double getDouble(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getDouble(aColumnName);
    }

    @SuppressWarnings("deprecation")
    public BigDecimal getBigDecimal(int aColumnIndex, int aScale) throws SQLException
    {
        return getCurrentResultSet().getBigDecimal(aColumnIndex, aScale);
    }

    public BigDecimal getBigDecimal(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getBigDecimal(aColumnIndex);
    }

    public BigDecimal getBigDecimal(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getBigDecimal(aColumnName);
    }

    @SuppressWarnings("deprecation")
    public BigDecimal getBigDecimal(String aColumnName, int aScale) throws SQLException
    {
        return getCurrentResultSet().getBigDecimal(aColumnName, aScale);
    }

    public byte[] getBytes(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getBytes(aColumnIndex);
    }

    public byte[] getBytes(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getBytes(aColumnName);
    }

    public Date getDate(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getDate(aColumnIndex);
    }

    public Date getDate(int aColumnIndex, Calendar aCal) throws SQLException
    {
        return getCurrentResultSet().getDate(aColumnIndex, aCal);
    }

    public Date getDate(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getDate(aColumnName);
    }

    public Date getDate(String aColumnName, Calendar aCal) throws SQLException
    {
        return getCurrentResultSet().getDate(aColumnName, aCal);
    }

    public Time getTime(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getTime(aColumnIndex);
    }

    public Time getTime(int aColumnIndex, Calendar aCal) throws SQLException
    {
        return getCurrentResultSet().getTime(aColumnIndex, aCal);
    }

    public Time getTime(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getTime(aColumnName);
    }

    public Time getTime(String aColumnName, Calendar aCal) throws SQLException
    {
        return getCurrentResultSet().getTime(aColumnName, aCal);
    }

    public Timestamp getTimestamp(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getTimestamp(aColumnIndex);
    }

    public Timestamp getTimestamp(int aColumnIndex, Calendar aCal) throws SQLException
    {
        return getCurrentResultSet().getTimestamp(aColumnIndex, aCal);
    }

    public Timestamp getTimestamp(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getTimestamp(aColumnName);
    }

    public Timestamp getTimestamp(String aColumnName, Calendar aCal) throws SQLException
    {
        return getCurrentResultSet().getTimestamp(aColumnName, aCal);
    }

    public InputStream getAsciiStream(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getAsciiStream(aColumnIndex);
    }

    public InputStream getAsciiStream(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getAsciiStream(aColumnName);
    }

    @SuppressWarnings("deprecation")
    public InputStream getUnicodeStream(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getUnicodeStream(aColumnIndex);
    }

    @SuppressWarnings("deprecation")
    public InputStream getUnicodeStream(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getUnicodeStream(aColumnName);
    }

    public InputStream getBinaryStream(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getBinaryStream(aColumnIndex);
    }

    public InputStream getBinaryStream(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getBinaryStream(aColumnName);
    }

    public SQLWarning getWarnings() throws SQLException
    {
        return getCurrentResultSet().getWarnings();
    }

    public void clearWarnings() throws SQLException
    {
        for (ResultSet sEach : mResultSets)
        {
            sEach.clearWarnings();
        }
    }

    public String getCursorName() throws SQLException
    {
        return getCurrentResultSet().getCursorName();
    }

    public ResultSetMetaData getMetaData() throws SQLException
    {
        return mResultSets.get(0).getMetaData();
    }

    public Object getObject(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getObject(aColumnIndex);
    }

    @SuppressWarnings("unchecked")
    public Object getObject(int aColumnIndex, Map aMap) throws SQLException
    {
        return getCurrentResultSet().getObject(aColumnIndex, aMap);
    }

    public Object getObject(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getObject(aColumnName);
    }

    @SuppressWarnings("unchecked")
    public Object getObject(String aColumnName, Map aMap) throws SQLException
    {
        return getCurrentResultSet().getObject(aColumnName, aMap);
    }

    public int findColumn(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().findColumn(aColumnName);
    }

    public Reader getCharacterStream(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getCharacterStream(aColumnIndex);
    }

    public Reader getCharacterStream(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getCharacterStream(aColumnName);
    }

    public boolean isBeforeFirst() throws SQLException
    {
        return getCurrentResultSet().isBeforeFirst();
    }

    public boolean isAfterLast() throws SQLException
    {
        return getCurrentResultSet().isAfterLast();
    }

    public boolean isFirst() throws SQLException
    {
        return getCurrentResultSet().isFirst();
    }

    public boolean isLast() throws SQLException
    {
        return getCurrentResultSet().isLast();
    }

    public void beforeFirst() throws SQLException
    {
        getCurrentResultSet().beforeFirst();
    }

    public void afterLast() throws SQLException
    {
        getCurrentResultSet().afterLast();
    }

    public boolean first() throws SQLException
    {
        return getCurrentResultSet().first();
    }

    public boolean last() throws SQLException
    {
        return getCurrentResultSet().last();
    }

    public int getRow() throws SQLException
    {
        return getCurrentResultSet().getRow();
    }

    public boolean absolute(int aRow) throws SQLException
    {
        return getCurrentResultSet().absolute(aRow);
    }

    public boolean relative(int aRows) throws SQLException
    {
        return getCurrentResultSet().relative(aRows);
    }

    public boolean previous() throws SQLException
    {
        return getCurrentResultSet().previous();
    }

    public void setFetchDirection(int aDirection) throws SQLException
    {
        getCurrentResultSet().setFetchDirection(aDirection);
    }

    public int getFetchDirection() throws SQLException
    {
        return getCurrentResultSet().getFetchDirection();
    }

    public void setFetchSize(int aRows) throws SQLException
    {
        getCurrentResultSet().setFetchSize(aRows);
    }

    public int getFetchSize() throws SQLException
    {
        return getCurrentResultSet().getFetchSize();
    }

    public int getType() throws SQLException
    {
        return getCurrentResultSet().getType();
    }

    public int getConcurrency() throws SQLException
    {
        return getCurrentResultSet().getConcurrency();
    }

    public boolean rowUpdated() throws SQLException
    {
        return getCurrentResultSet().rowUpdated();
    }

    public boolean rowInserted() throws SQLException
    {
        return getCurrentResultSet().rowInserted();
    }

    public boolean rowDeleted() throws SQLException
    {
        return getCurrentResultSet().rowDeleted();
    }

    public void updateNull(int aColumnIndex) throws SQLException
    {
        getCurrentResultSet().updateNull(aColumnIndex);
    }

    public void updateNull(String aColumnName) throws SQLException
    {
        getCurrentResultSet().updateNull(aColumnName);
    }

    public void updateBoolean(int aColumnIndex, boolean aValue) throws SQLException
    {
        getCurrentResultSet().updateBoolean(aColumnIndex, aValue);
    }

    public void updateBoolean(String aColumnName, boolean aValue) throws SQLException
    {
        getCurrentResultSet().updateBoolean(aColumnName, aValue);
    }

    public void updateByte(int aColumnIndex, byte aValue) throws SQLException
    {
        getCurrentResultSet().updateByte(aColumnIndex, aValue);
    }

    public void updateByte(String aColumnName, byte aValue) throws SQLException
    {
        getCurrentResultSet().updateByte(aColumnName, aValue);
    }

    public void updateShort(int aColumnIndex, short aValue) throws SQLException
    {
        getCurrentResultSet().updateShort(aColumnIndex, aValue);
    }

    public void updateShort(String aColumnName, short aValue) throws SQLException
    {
        getCurrentResultSet().updateShort(aColumnName, aValue);
    }

    public void updateInt(int aColumnIndex, int aValue) throws SQLException
    {
        getCurrentResultSet().updateInt(aColumnIndex, aValue);
    }

    public void updateInt(String aColumnName, int aValue) throws SQLException
    {
        getCurrentResultSet().updateInt(aColumnName, aValue);
    }

    public void updateLong(int aColumnIndex, long aValue) throws SQLException
    {
        getCurrentResultSet().updateLong(aColumnIndex, aValue);
    }

    public void updateLong(String aColumnName, long aValue) throws SQLException
    {
        getCurrentResultSet().updateLong(aColumnName, aValue);
    }

    public void updateFloat(int aColumnIndex, float aValue) throws SQLException
    {
        getCurrentResultSet().updateFloat(aColumnIndex, aValue);
    }

    public void updateFloat(String aColumnName, float aValue) throws SQLException
    {
        getCurrentResultSet().updateFloat(aColumnName, aValue);
    }

    public void updateDouble(int aColumnIndex, double aValue) throws SQLException
    {
        getCurrentResultSet().updateDouble(aColumnIndex, aValue);
    }

    public void updateDouble(String aColumnName, double aValue) throws SQLException
    {
        getCurrentResultSet().updateDouble(aColumnName, aValue);
    }

    public void updateBigDecimal(int aColumnIndex, BigDecimal aValue) throws SQLException
    {
        getCurrentResultSet().updateBigDecimal(aColumnIndex, aValue);
    }

    public void updateBigDecimal(String aColumnName, BigDecimal aValue) throws SQLException
    {
        getCurrentResultSet().updateBigDecimal(aColumnName, aValue);
    }

    public void updateString(int aColumnIndex, String aValue) throws SQLException
    {
        getCurrentResultSet().updateString(aColumnIndex, aValue);
    }

    public void updateString(String aColumnName, String aValue) throws SQLException
    {
        getCurrentResultSet().updateString(aColumnName, aValue);
    }

    public void updateBytes(int aColumnIndex, byte[] aValue) throws SQLException
    {
        getCurrentResultSet().updateBytes(aColumnIndex, aValue);
    }

    public void updateBytes(String aColumnName, byte[] aValue) throws SQLException
    {
        getCurrentResultSet().updateBytes(aColumnName, aValue);
    }

    public void updateDate(int aColumnIndex, Date aValue) throws SQLException
    {
        getCurrentResultSet().updateDate(aColumnIndex, aValue);
    }

    public void updateDate(String aColumnName, Date aValue) throws SQLException
    {
        getCurrentResultSet().updateDate(aColumnName, aValue);
    }

    public void updateTime(int aColumnIndex, Time aValue) throws SQLException
    {
        getCurrentResultSet().updateTime(aColumnIndex, aValue);
    }

    public void updateTime(String aColumnName, Time aValue) throws SQLException
    {
        getCurrentResultSet().updateTime(aColumnName, aValue);
    }

    public void updateTimestamp(int aColumnIndex, Timestamp aValue) throws SQLException
    {
        getCurrentResultSet().updateTimestamp(aColumnIndex, aValue);
    }

    public void updateTimestamp(String aColumnName, Timestamp aValue) throws SQLException
    {
        getCurrentResultSet().updateTimestamp(aColumnName, aValue);
    }

    public void updateAsciiStream(int aColumnIndex, InputStream aValue, int aLength) throws SQLException
    {
        getCurrentResultSet().updateAsciiStream(aColumnIndex, aValue, aLength);
    }

    public void updateAsciiStream(String aColumnName, InputStream aValue, int aLength) throws SQLException
    {
        getCurrentResultSet().updateAsciiStream(aColumnName, aValue, aLength);
    }

    public void updateBinaryStream(int aColumnIndex, InputStream aValue, int aLength) throws SQLException
    {
        getCurrentResultSet().updateBinaryStream(aColumnIndex, aValue, aLength);
    }

    public void updateBinaryStream(String aColumnName, InputStream aValue, int aLength) throws SQLException
    {
        getCurrentResultSet().updateBinaryStream(aColumnName, aValue, aLength);
    }

    public void updateCharacterStream(int aColumnIndex, Reader aReader, int aLength) throws SQLException
    {
        getCurrentResultSet().updateCharacterStream(aColumnIndex, aReader, aLength);
    }

    public void updateCharacterStream(String aColumnName, Reader aReader, int aLength) throws SQLException
    {
        getCurrentResultSet().updateCharacterStream(aColumnName, aReader, aLength);
    }

    public void updateObject(int aColumnIndex, Object aValue, int aScale) throws SQLException
    {
        getCurrentResultSet().updateObject(aColumnIndex, aValue, aScale);
    }

    public void updateObject(int aColumnIndex, Object aValue) throws SQLException
    {
        getCurrentResultSet().updateObject(aColumnIndex, aValue);
    }

    public void updateObject(String aColumnName, Object aValue, int aScale) throws SQLException
    {
        getCurrentResultSet().updateObject(aColumnName, aValue, aScale);
    }

    public void updateObject(String aColumnName, Object aValue) throws SQLException
    {
        getCurrentResultSet().updateObject(aColumnName, aValue);
    }

    public void insertRow() throws SQLException
    {
        getCurrentResultSet().insertRow();
    }

    public void updateRow() throws SQLException
    {
        getCurrentResultSet().updateRow();
    }

    public void deleteRow() throws SQLException
    {
        getCurrentResultSet().deleteRow();
    }

    public void refreshRow() throws SQLException
    {
        getCurrentResultSet().refreshRow();
    }

    public void cancelRowUpdates() throws SQLException
    {
        getCurrentResultSet().cancelRowUpdates();
    }

    public void moveToInsertRow() throws SQLException
    {
        getCurrentResultSet().moveToInsertRow();
    }

    public void moveToCurrentRow() throws SQLException
    {
        getCurrentResultSet().moveToCurrentRow();
    }

    public Statement getStatement()
    {
        return mStatement;
    }

    public Ref getRef(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getRef(aColumnIndex);
    }

    public Ref getRef(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getRef(aColumnName);
    }

    public Blob getBlob(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getBlob(aColumnIndex);
    }

    public Blob getBlob(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getBlob(aColumnName);
    }

    public Clob getClob(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getClob(aColumnIndex);
    }

    public Clob getClob(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getClob(aColumnName);
    }

    public Array getArray(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getArray(aColumnIndex);
    }

    public Array getArray(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getArray(aColumnName);
    }

    public URL getURL(int aColumnIndex) throws SQLException
    {
        return getCurrentResultSet().getURL(aColumnIndex);
    }

    public URL getURL(String aColumnName) throws SQLException
    {
        return getCurrentResultSet().getURL(aColumnName);
    }

    public void updateRef(int aColumnIndex, Ref aValue) throws SQLException
    {
        getCurrentResultSet().updateRef(aColumnIndex, aValue);
    }

    public void updateRef(String aColumnName, Ref aValue) throws SQLException
    {
        getCurrentResultSet().updateRef(aColumnName, aValue);
    }

    public void updateBlob(int aColumnIndex, Blob aValue) throws SQLException
    {
        getCurrentResultSet().updateBlob(aColumnIndex, aValue);
    }

    public void updateBlob(String aColumnName, Blob aValue) throws SQLException
    {
        getCurrentResultSet().updateBlob(aColumnName, aValue);
    }

    public void updateClob(int aColumnIndex, Clob aValue) throws SQLException
    {
        getCurrentResultSet().updateClob(aColumnIndex, aValue);
    }

    public void updateClob(String aColumnName, Clob aValue) throws SQLException
    {
        getCurrentResultSet().updateClob(aColumnName, aValue);
    }

    public void updateArray(int aColumnIndex, Array aValue) throws SQLException
    {
        getCurrentResultSet().updateArray(aColumnIndex, aValue);
    }

    public void updateArray(String aColumnName, Array aValue) throws SQLException
    {
        getCurrentResultSet().updateArray(aColumnName, aValue);
    }

    private ResultSet getCurrentResultSet() throws SQLException
    {
        return mResultSetMerger.getCurrentResultSet();
    }

    void clearClosedResultSets()
    {
        List<ResultSet> sClosedResultSets = new ArrayList<ResultSet>();

        // BUG-46513 ResultSet에 연결된 Statement가 이미 close된 경우에는 ResultSet List에서 제외시킨다.
        for (ResultSet sEach : mResultSets)
        {
            try
            {
                AltibaseStatement sStmt = (AltibaseStatement)sEach.getStatement();
                if (sStmt.isClosed())
                {
                    sClosedResultSets.add(sEach);
                }
            }
            catch (SQLException aEx)
            {
                if (aEx.getSQLState().equals("01C01")) // Statement already closed.
                {
                    sClosedResultSets.add(sEach);
                }
            }
        }

        for (ResultSet sEach : sClosedResultSets)
        {
            mResultSets.remove(sEach);
        }

        mResultSetMerger = new IteratorStreamResultSetMerger(mResultSets);
    }

    @Override
    public int getHoldability() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public boolean isClosed() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNString(int aColumnIndex, String aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateNString(String aColumnName, String aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public String getNString(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public String getNString(String aColumnName) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateAsciiStream(int aColumnIndex, InputStream aInputStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBinaryStream(int aColumnIndex, InputStream aInputStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateCharacterStream(int aColumnIndex, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateAsciiStream(String aColumnName, InputStream aInputStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBinaryStream(String aColumnName, InputStream aInputStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateCharacterStream(String aColumnName, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBlob(int aColumnIndex, InputStream aInputStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBlob(String aColumnName, InputStream aInputStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateClob(int aColumnIndex, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateClob(String aColumnName, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateAsciiStream(int aColumnIndex, InputStream aInputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBinaryStream(int aColumnIndex, InputStream aInputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateCharacterStream(int aColumnIndex, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateAsciiStream(String aColumnName, InputStream aInputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBinaryStream(String aColumnName, InputStream aInputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateCharacterStream(String aColumnName, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBlob(int aColumnIndex, InputStream aInputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateBlob(String aColumnName, InputStream aInputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateClob(int aColumnIndex, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void updateClob(String aColumnName, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public <T> T getObject(int aColumnIndex, Class<T> aType) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public <T> T getObject(String aColumnName, Class<T> aType) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
