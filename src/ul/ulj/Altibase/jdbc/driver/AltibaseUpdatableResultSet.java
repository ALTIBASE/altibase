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
import java.math.BigInteger;
import java.sql.*;
import java.util.List;

import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltiSqlProcessor;

public class AltibaseUpdatableResultSet extends AltibaseResultSet
{
    private AltibaseReadableResultSet mBaseResultSet;
    private int                       mBaseResultRowIdColumnIndex;
    private AltibasePreparedStatement mUpdateStmt;
    private AltibasePreparedStatement mInsertStmt;
    private AltibasePreparedStatement mDeleteStmt;

    private AltibaseReadableResultSet mInsertRow;
    private AltibaseReadableResultSet mUpdateRow;
    private byte[]                    mUpdated;

    private boolean[]                 mUsedAsParameterForInsert;
    private boolean[]                 mUsedAsParameterForUpdate;
    private long[]                    mLobLength;  // BUG-47456 Support updateXXXStream(xxx, xxx, long)

    private static final byte         NOT_UPDATED           = 0x01;
    private static final byte         UPDATED_FOR_BINARY    = 0x02;
    private static final byte         UPDATED_FOR_CHARACTER = 0x04;
    private static final byte         UPDATED_FOR_ASCII     = 0x08;
    private static final byte         UPDATED_FOR_NORMAL    = 0x10;
    private static final byte         INVALID_UPDATED_STATE = 0x20;
    
    AltibaseUpdatableResultSet(AltibaseReadableResultSet aBaseResultSet) throws SQLException
    {
        mBaseResultSet = aBaseResultSet;
        mStatement = mBaseResultSet.mStatement;

        // row id는 target절 맨 끝에 덧붙는다.
        mBaseResultRowIdColumnIndex = mBaseResultSet.getTargetColumnCount();
        mUsedAsParameterForInsert = new boolean[mBaseResultRowIdColumnIndex - 1];
        mUsedAsParameterForUpdate = new boolean[mBaseResultRowIdColumnIndex - 1];
        mUpdated = new byte[mBaseResultRowIdColumnIndex - 1];
        clearUpdated();
        
        mDeleteStmt = (AltibasePreparedStatement)mStatement.mConnection.prepareStatement(AltiSqlProcessor.makeDeleteRowSql(getBaseTableName()));
        mInsertStmt = null; // insert, update 구문은 update되는 컬럼의 개수에 따라 sql문이 가변적이다.
        mUpdateStmt = null;
        mLobLength = new long[mBaseResultRowIdColumnIndex - 1];
        
        mInsertRow = new AltibaseTempResultSet(mStatement, mBaseResultSet.getTargetColumns());
        mUpdateRow = mBaseResultSet;
    }

    private String getBaseTableName() throws SQLException
    {
        // UpdatableResultSet을 얻으려면 원본 쿼리가 '단일 테이블에 대한 질의'여야 한다는 제약이 있다.
        // 그러므로 아무 컬럼에서나 테이블 이름을 얻어와도 된다.
        return mBaseResultSet.getTargetColumnInfo(1).getBaseTableName();
    }

    private String getBaseColumnName(int aColumnIndex)
    {
        return mBaseResultSet.getTargetColumnInfo(aColumnIndex).getBaseColumnName();
    }

    protected List getTargetColumns()
    {
        return mUpdateRow.getTargetColumns();
    }

    private void clearUpdated()
    {
        for (int i = 0; i < mUpdated.length; i++)
        {
            mUpdated[i] = NOT_UPDATED;
        }
    }

    private byte getUpdateState(int aIdx)
    {
        if( (mUpdated[aIdx] & NOT_UPDATED) == NOT_UPDATED)
        {
            return NOT_UPDATED;
        }
        else if( (mUpdated[aIdx] & UPDATED_FOR_ASCII) == UPDATED_FOR_ASCII)
        {
            return UPDATED_FOR_ASCII;
        }
        else if( (mUpdated[aIdx] & UPDATED_FOR_BINARY) == UPDATED_FOR_BINARY)
        {
            return UPDATED_FOR_BINARY;
        }
        else if( (mUpdated[aIdx] & UPDATED_FOR_CHARACTER) == UPDATED_FOR_CHARACTER)
        {
            return UPDATED_FOR_CHARACTER;
        }
        else if( (mUpdated[aIdx] & UPDATED_FOR_NORMAL) == UPDATED_FOR_NORMAL)
        {
            return UPDATED_FOR_NORMAL;
        }
        else
        {
            return INVALID_UPDATED_STATE;
        }
    }
    
    private boolean isUpdated(int aIdx)
    {
        if( (mUpdated[aIdx] & NOT_UPDATED) == NOT_UPDATED )
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    
    private void checkUpdateStmtReusable() throws SQLException
    {
        if (mUpdateStmt == null || !checkParameters(mUsedAsParameterForUpdate))
        {
            StringBuffer sBuf = new StringBuffer("UPDATE ");
            sBuf.append(getBaseTableName());
            sBuf.append(" SET ");
            boolean sFirst = true;
            int sUpdatedCount = 0;
            for (int i = 1; i < mBaseResultRowIdColumnIndex; i++)
            {
                mUsedAsParameterForUpdate[i - 1] = false;
                if (isUpdated(i - 1))
                {
                    if (sFirst)
                    {
                        sFirst = false;
                    }
                    else
                    {
                        sBuf.append(",");
                    }
                    sBuf.append(getBaseColumnName(i));
                    sBuf.append("=?");
                    mUsedAsParameterForUpdate[i - 1] = true;
                    sUpdatedCount++;
                }
            }
            if (sUpdatedCount == 0)
            {
                Error.throwSQLException(ErrorDef.DOES_NOT_MATCH_COLUMN_LIST);
            }
            sBuf.append(" WHERE _PROWID=?");
            if (mUpdateStmt != null)
            {
                mUpdateStmt.close();
            }
            mUpdateStmt = (AltibasePreparedStatement)mStatement.mConnection.prepareStatement(sBuf.toString());
        }
    }

    private void checkInsertStmtReusable() throws SQLException
    {
        if (mInsertStmt == null || !checkParameters(mUsedAsParameterForInsert))
        {
            StringBuffer sBuf = new StringBuffer("INSERT INTO ");
            sBuf.append(getBaseTableName());
            sBuf.append(" (");
            boolean sFirst = true;
            int sUpdatedCount = 0;
            for (int i = 1; i < mBaseResultRowIdColumnIndex; i++)
            {
                mUsedAsParameterForInsert[i - 1] = false;
                if (isUpdated(i - 1))
                {
                    if (sFirst)
                    {
                        sFirst = false;
                    }
                    else
                    {
                        sBuf.append(",");
                    }
                    sBuf.append(getBaseColumnName(i));
                    mUsedAsParameterForInsert[i - 1] = true;
                    sUpdatedCount++;
                }
            }
            if (sUpdatedCount == 0) 
            {
                Error.throwSQLException(ErrorDef.DOES_NOT_MATCH_COLUMN_LIST);
            }
            sBuf.append(") VALUES (?");
            sFirst = true;
            for (int i = 2; i <= sUpdatedCount; i++)
            {
                sBuf.append(",?");
            }
            sBuf.append(")");
            if (mInsertStmt != null)
            {
                mInsertStmt.close();
            }
            mInsertStmt = (AltibasePreparedStatement)mStatement.mConnection.prepareStatement(sBuf.toString());
        }
    }

    private boolean checkParameters(boolean[] aUsedAsParameter)
    {
        for (int i = 1; i < mBaseResultRowIdColumnIndex; i++)
        {
            if (aUsedAsParameter[i - 1])
            {
                if (!isUpdated(i - 1))
                {
                    return false;
                }
            }
            else
            {
                if (isUpdated(i - 1))
                {
                    return false;
                }
            }
        }
        return true;
    }

    public void close() throws SQLException
    {
        if (isClosed())
        {
            return;
        }

        mBaseResultSet.close();

        mDeleteStmt.close();
        if (mInsertStmt != null)
        {
            mInsertStmt.close();
        }
        if (mUpdateStmt != null)
        {
            mUpdateStmt.close();
        }

        super.close();
    }

    public int getConcurrency() throws SQLException
    {
        throwErrorForClosed();
        return CONCUR_UPDATABLE;
    }

    public void cancelRowUpdates() throws SQLException
    {
        throwErrorForClosed();
        if (mUpdateRow == mInsertRow)
        {
            Error.throwSQLException(ErrorDef.CURSOR_AT_INSERTING_ROW);
        }

        mUpdateRow.rowHandle().reload();
        clearUpdated();
    }

    public void moveToCurrentRow() throws SQLException
    {
        throwErrorForClosed();
        mUpdateRow = mBaseResultSet;
    }

    public void moveToInsertRow() throws SQLException
    {
        throwErrorForClosed();
        mUpdateRow = mInsertRow;
    }

    public void deleteRow() throws SQLException
    {
        throwErrorForClosed();
        if (mUpdateRow == mInsertRow)
        {
            Error.throwSQLException(ErrorDef.CURSOR_AT_INSERTING_ROW);
        }

        mDeleteStmt.setLong(1, mBaseResultSet.getLong(mBaseResultRowIdColumnIndex));
        mDeleteStmt.executeUpdate();

        if (mBaseResultSet instanceof AltibaseScrollableResultSet)
        {
            ((AltibaseScrollableResultSet)mBaseResultSet).deleteRowInCache();
        }
        cursorMoved();
    }

    public void insertRow() throws SQLException
    {
        throwErrorForClosed();
        if (mUpdateRow != mInsertRow)
        {
            Error.throwSQLException(ErrorDef.CURSOR_NOT_AT_INSERTING_ROW);
        }

        checkInsertStmtReusable();

        int sPstmtParamIndex = 1;
        for (int i = 1; i < mBaseResultRowIdColumnIndex; i++)
        {
            if (isUpdated(i - 1))
            {
                mInsertStmt.setObject(sPstmtParamIndex++, mUpdateRow.getObject(i));
            }
        }
        mInsertStmt.executeUpdate();
    }

    public void updateRow() throws SQLException
    {
        throwErrorForClosed();
        if (mUpdateRow == mInsertRow)
        {
            Error.throwSQLException(ErrorDef.CURSOR_AT_INSERTING_ROW);
        }
        if (rowDeleted())
        {
            Error.throwSQLException(ErrorDef.CURSOR_OPERATION_CONFLICT);
        }

        // DB update
        checkUpdateStmtReusable();
        int sPstmtParamIndex = 1;
        for (int i = 1; i < mBaseResultRowIdColumnIndex; i++)
        {
            byte sUpdateState = getUpdateState(i - 1);
            switch(sUpdateState)
            {
                case NOT_UPDATED:
                    // Skip
                    break;
                case UPDATED_FOR_NORMAL:
                    mUpdateStmt.setObject(sPstmtParamIndex++, mUpdateRow.getObject(i));
                    break;
                case UPDATED_FOR_BINARY:
                    mUpdateStmt.setBinaryStream(sPstmtParamIndex++, (InputStream) mUpdateRow.getObject(i), mLobLength[i - 1]);
                    break;
                case UPDATED_FOR_CHARACTER:
                    mUpdateStmt.setCharacterStream(sPstmtParamIndex++, (Reader) mUpdateRow.getObject(i), mLobLength[i - 1]);
                    break;
                case UPDATED_FOR_ASCII:
                    mUpdateStmt.setAsciiStream(sPstmtParamIndex++, (InputStream) mUpdateRow.getObject(i), mLobLength[i - 1]);
                    break;
                default:
                    Error.throwInternalError(ErrorDef.INVALID_STATE,
                                             "NOT_UPDATED | UPDATED_FOR_NORMAL | UPDATED_FOR_BINARY | UPDATED_FOR_CHARACTER | UPDATED_FOR_ASCII",
                                             String.valueOf(sUpdateState));
                    break;
            }
        }
        // 마지막으로 where 조건의 _prowid를 세팅한다.
        mUpdateStmt.setLong(sPstmtParamIndex, mBaseResultSet.getLong(mBaseResultRowIdColumnIndex));
        mUpdateStmt.executeUpdate();

        mUpdateRow.rowHandle().update();
        clearUpdated();
    }

    public boolean rowDeleted() throws SQLException
    {
        throwErrorForClosed();
        return mUpdateRow.rowDeleted();
    }

    public boolean rowInserted() throws SQLException
    {
        throwErrorForClosed();
        return mUpdateRow.rowInserted();
    }

    public boolean rowUpdated() throws SQLException
    {
        throwErrorForClosed();
        return mUpdateRow.rowUpdated();
    }

    public void updateAsciiStream(int aColumnIndex, InputStream aValue, long aLength) throws SQLException
    {
        throwErrorForClosed();
        mUpdated[aColumnIndex - 1] = UPDATED_FOR_ASCII;
        mLobLength[aColumnIndex - 1] = aLength;
        updateObject(aColumnIndex, aValue);
    }

    public void updateBigDecimal(int aColumnIndex, BigDecimal aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateBinaryStream(int aColumnIndex, InputStream aValue, long aLength) throws SQLException
    {
        throwErrorForClosed();
        mUpdated[aColumnIndex - 1] = UPDATED_FOR_BINARY; 
        mLobLength[aColumnIndex - 1] = aLength;
        updateObject(aColumnIndex, aValue);
    }

    public void updateBlob(int aColumnIndex, Blob aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateBoolean(int aColumnIndex, boolean aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateByte(int aColumnIndex, byte aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateBytes(int aColumnIndex, byte[] aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateCharacterStream(int aColumnIndex, Reader aValue, long aLength) throws SQLException
    {
        throwErrorForClosed();
        mUpdated[aColumnIndex - 1] = UPDATED_FOR_CHARACTER;
        mLobLength[aColumnIndex - 1] = aLength;
        updateObject(aColumnIndex, aValue);
    }

    public void updateClob(int aColumnIndex, Clob aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateDate(int aColumnIndex, Date aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateDouble(int aColumnIndex, double aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateFloat(int aColumnIndex, float aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateInt(int aColumnIndex, int aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateLong(int aColumnIndex, long aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateNull(int aColumnIndex) throws SQLException
    {
        updateObject(aColumnIndex, null);
    }

    public void updateObject(int aColumnIndex, Object aValue, int aScale) throws SQLException
    {
        try
        {
            BigDecimal sDecimalValue = new BigDecimal(new BigInteger(aValue.toString()), aScale);
            updateObject(aColumnIndex, sDecimalValue);
        }
        catch (NumberFormatException sException)
        {
            updateObject(aColumnIndex, aValue);
        }
    }

    public void updateObject(int aColumnIndex, Object aValue) throws SQLException
    {
        throwErrorForClosed();

        Column sColumn = getTargetColumn(aColumnIndex);
        byte sUpdateState = getUpdateState(aColumnIndex - 1);

        if ((aValue instanceof InputStream) && (sUpdateState != UPDATED_FOR_BINARY) &&
            (sUpdateState != UPDATED_FOR_ASCII))
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), sColumn.getDBColumnTypeName());
        }
        if ((aValue instanceof Reader) && (sUpdateState != UPDATED_FOR_CHARACTER))
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), sColumn.getDBColumnTypeName());
        }
        
        sColumn.setValue(aValue);
        
        if(sUpdateState == NOT_UPDATED)
        {
            mUpdated[aColumnIndex - 1] = UPDATED_FOR_NORMAL;
        }
    }

    public void updateShort(int aColumnIndex, short aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateString(int aColumnIndex, String aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateTime(int aColumnIndex, Time aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public void updateTimestamp(int aColumnIndex, Timestamp aValue) throws SQLException
    {
        updateObject(aColumnIndex, aValue);
    }

    public boolean isAfterLast() throws SQLException
    {
        throwErrorForClosed();
        return mBaseResultSet.isAfterLast();
    }

    public boolean isBeforeFirst() throws SQLException
    {
        throwErrorForClosed();
        return mBaseResultSet.isBeforeFirst();
    }

    public boolean isFirst() throws SQLException
    {
        throwErrorForClosed();
        return mBaseResultSet.isFirst();
    }

    public boolean isLast() throws SQLException
    {
        throwErrorForClosed();
        return mBaseResultSet.isLast();
    }

    public boolean absolute(int aRow) throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        cursorMoved();
        return mBaseResultSet.absolute(aRow);
    }

    public boolean relative(int aRows) throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        cursorMoved();
        return mBaseResultSet.relative(aRows);
    }

    public void afterLast() throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        mBaseResultSet.afterLast();
        cursorMoved();
    }

    public void beforeFirst() throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        mBaseResultSet.beforeFirst();
        cursorMoved();
    }

    public boolean first() throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        cursorMoved();
        return mBaseResultSet.first();
    }

    public boolean last() throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        cursorMoved();
        return mBaseResultSet.last();
    }

    public boolean previous() throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        cursorMoved();
        return mBaseResultSet.previous();
    }

    public boolean next() throws SQLException
    {
        cancelRowUpdates();
        moveToCurrentRow();
        cursorMoved();
        return mBaseResultSet.next();
    }

    public void refreshRow() throws SQLException
    {
        cancelRowUpdates();
        mBaseResultSet.refreshRow();
    }

    public int getFetchSize() throws SQLException
    {
        throwErrorForClosed();
        return mBaseResultSet.getFetchSize();
    }

    public void setFetchSize(int aRows) throws SQLException
    {
        throwErrorForClosed();
        mBaseResultSet.setFetchSize(aRows);
    }

    public int getRow() throws SQLException
    {
        throwErrorForClosed();
        return mBaseResultSet.getRow();
    }

    public int getType() throws SQLException
    {
        throwErrorForClosed();
        return mBaseResultSet.getType();
    }

    int size()
    {
        return mBaseResultSet.size();
    }

    @Override
    public void updateBlob(int aColumnIndex, InputStream aValue, long aLength) throws SQLException
    {
        updateBinaryStream(aColumnIndex, aValue, aLength);
    }

    @Override
    public void updateClob(int aColumnIndex, Reader aReader, long aLength) throws SQLException
    {
        updateCharacterStream(aColumnIndex, aReader, aLength);
    }

    @Override
    public void updateNString(int aColumnIndex, String aValue) throws SQLException
    {
        updateString(aColumnIndex, aValue);
    }
}
