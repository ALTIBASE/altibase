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

import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextDirExec;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.ColumnInfo;
import Altibase.jdbc.driver.datatype.LobLocatorColumn;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltibaseProperties;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.net.URL;
import java.sql.*;
import java.sql.Date;
import java.time.*;
import java.util.*;

public abstract class AltibaseResultSet extends AbstractResultSet
{
    private static boolean             mAllowLobNullSelect;      // BUG-47639 lob column이 null일때 Lob객체가 리턴될 수 있는지 여부
    protected AltibaseStatement        mStatement;
    protected SQLWarning               mWarning;
    protected int                      mFetchSize;
    protected CmProtocolContextDirExec mContext;

    private ResultSetMetaData          mMetaData;
    private boolean                    mClosed;
    private ArrayList<Object>          mListeners;
    private int                        mLastReadColumnIndex = 0;
    // getXXX() 호출때마다 매번 row 위치 테스트를 하는 비용을 줄이기 위한 필드
    private boolean                    mCursorTested        = false;

    static AltibaseResultSet createResultSet(AltibaseStatement aStatement, int aResultSetType, int aResultSetConcurrency) throws SQLException
    {
        AltibaseResultSet sResult = null;
        if ((!aStatement.mFetchResult.fetchRemains()) &&
            (aStatement.mFetchResult.rowHandle().size() == 0))
        {
            if(aStatement.getProtocolContext().getPrepareResult().getResultSetCount() > 1)
            {
                sResult = new AltibaseEmptyResultSet(aStatement, aStatement.getProtocolContext().getGetColumnInfoResult().getColumns(), aResultSetType, aResultSetConcurrency);
            }
            else
            {
                sResult = new AltibaseEmptyResultSet(aStatement, aStatement.mPrepareResultColumns, aResultSetType, aResultSetConcurrency);
            }
        }
        else
        {
            // BUGBUG 불필요한 객체 생성 회피
            // ResultSet을 만들기 전에 체크를 먼저 할 수 있게 하는 것이 좋겠음
            // 불필요한 객체 생성을 막을 필요가 있음
            switch (aResultSetType)
            {
                case ResultSet.TYPE_FORWARD_ONLY:
                    sResult = new AltibaseForwardOnlyResultSet(aStatement, aStatement.getProtocolContext(), aStatement.getFetchSize());
                    break;
                case ResultSet.TYPE_SCROLL_INSENSITIVE:
                    sResult = new AltibaseScrollInsensitiveResultSet(aStatement, aStatement.getProtocolContext(), aStatement.getFetchSize());
                    break;
                case ResultSet.TYPE_SCROLL_SENSITIVE:
                    sResult = new AltibaseScrollSensitiveResultSet(aStatement, aStatement.getProtocolContext(), aStatement.getFetchSize());
                    break;
                default:
                    Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION);
                    break;
            }
            if (aResultSetConcurrency == ResultSet.CONCUR_UPDATABLE)
            {
                sResult = new AltibaseUpdatableResultSet((AltibaseReadableResultSet)sResult);
            }
        }
        // BUG-47639 lob_null_select jdbc 속성값을 AltibaseConnection 객체로부터 받아온다.
        mAllowLobNullSelect = aStatement.mConnection.getAllowLobNullSelect();
        return sResult;
    }

    static void checkAttributes(int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        if (!AltibaseResultSet.isValidType(aResultSetType))
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "ResultSet type",
                                    "TYPE_FORWARD_ONLY | TYPE_SCROLL_INSENSITIVE | TYPE_SCROLL_SENSITIVE",
                                    String.valueOf(aResultSetType));
        }
        if (!AltibaseResultSet.isValidConcurrency(aResultSetConcurrency))
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "ResultSet concurrency",
                                    "CONCUR_READ_ONLY | CONCUR_UPDATABLE",
                                    String.valueOf(aResultSetConcurrency));
        }
        checkHoldability(aResultSetHoldability);
    }

    static void checkHoldability(int aResultSetHoldability) throws SQLException
    {
        if (!AltibaseResultSet.isValidHoldability(aResultSetHoldability))
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "ResultSet holdability",
                                    "HOLD_CURSORS_OVER_COMMIT | CLOSE_CURSORS_AT_COMMIT",
                                    String.valueOf(aResultSetHoldability));
        }
    }

    static boolean isValidType(int aResultSetType)
    {
        switch (aResultSetType)
        {
            case ResultSet.TYPE_FORWARD_ONLY:
            case ResultSet.TYPE_SCROLL_INSENSITIVE:
            case ResultSet.TYPE_SCROLL_SENSITIVE:
                return true;
            default:
                return false;
        }
    }

    static boolean isValidConcurrency(int aResultSetConcurrency)
    {
        switch (aResultSetConcurrency)
        {
            case ResultSet.CONCUR_READ_ONLY:
            case ResultSet.CONCUR_UPDATABLE:
                return true;
            default:
                return false;
        }
    }

    static boolean isValidHoldability(int aResultSetHoldability)
    {
        switch (aResultSetHoldability)
        {
            case ResultSet.HOLD_CURSORS_OVER_COMMIT:
            case ResultSet.CLOSE_CURSORS_AT_COMMIT:
                return true;
            default:
                return false;
        }
    }

    public static void checkFetchDirection(int aFetchDirection) throws SQLException
    {
        switch (aFetchDirection)
        {
            case ResultSet.FETCH_FORWARD:
                break;
            case ResultSet.FETCH_REVERSE:
            case ResultSet.FETCH_UNKNOWN:
                Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Non-forward direction");
                break;
            default:
                Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                        "Fetch direction",
                                        "FETCH_FORWARD | FETCH_REVERSE | FETCH_UNKNOWN",
                                        String.valueOf(aFetchDirection));
                break;
        }
    }

    /**
     * ResultSet의 크기(전체 row 수)를 얻는다.
     * 
     * @return 크기를 정확히 알 수 있으면 그 값, 아니면 큰 값(Integer.MAX_VALUE)
     */
    abstract int size();

    AltibaseStatement getAltibaseStatement()
    {
        return mStatement;
    }

    public final Statement getStatement() throws SQLException
    {
        throwErrorForClosed();
        if (mStatement == null) 
        {
            Error.throwSQLException(ErrorDef.RESULTSET_CREATED_BY_INTERNAL_STATEMENT);
        } 
        return mStatement;
    }

    public final void clearWarnings() throws SQLException
    {
        throwErrorForClosed();
        mWarning = null;
    }

    public final SQLWarning getWarnings() throws SQLException
    {
        throwErrorForClosed();
        return mWarning;
    }

    public final String getCursorName() throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Cursor name and positioned update");
    }

    /**
     * ResultSet이 닫혔는지 확인한다.
     * 
     * @return Resultset이 close되었으면 true, 아니면 false
     */
    public final boolean isClosed()
    {
        return mClosed;
    }

    public int getHoldability() throws SQLException
    {
        throwErrorForClosed();
        return mStatement.getResultSetHoldability();
    }
    
    protected void closeResultSetCursor() throws SQLException
    {
        CmProtocol.closeCursor(mStatement.getProtocolContext(),
                mStatement.getID(),
                mContext.getFetchResult().getResultSetId(),
                mStatement.mConnection.isClientSideAutoCommit());
        mWarning = Error.processServerError(mWarning, mStatement.getProtocolContext().getError());
    }
    
    public void close() throws SQLException
    {
        if (isClosed())
        {
            return;
        }

        mLastReadColumnIndex = 0;
        mClosed = true;

        // auto close
        try
        {
            if (mListeners != null)
            {
                for (Object sObj : mListeners)
                {
                    if (sObj instanceof Statement)
                    {
                        ((Statement)sObj).close();
                    }
                    else
                    {
                        Error.throwInternalError(ErrorDef.INVALID_TYPE, "Statement",
                                                 sObj.getClass().getName());
                    }
                }
                mListeners.clear();
                mListeners = null;
            }
        }
        finally
        {
            mStatement.checkCloseOnCompletion();
        }

        mClosed = true;
    }

    /**
     * ResultSet을 {@link #close()} 할 때 닫아줄 객체를 등록한다.
     * <p>
     * 등록할 수 있는 객체는 다음 중 하나여야 한다:
     * Statement
     *
     * @param aClosableObject ResultSet을 닫을 때 닫을 객체
     */
    protected final void registerTarget(Object aClosableObject)
    {
        if (mListeners == null)
        {
            mListeners = new ArrayList<>();
        }

        mListeners.add(aClosableObject);
    }

    public int getFetchSize() throws SQLException
    {
        throwErrorForClosed();
        return mFetchSize;
    }

    public void setFetchSize(int aRows) throws SQLException
    {
        throwErrorForClosed();
        if (aRows < 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_ARGUMENT,
                                    "Fetch size",
                                    AltibaseProperties.RANGE_FETCH_ENOUGH,
                                    String.valueOf(aRows));
        }
        mFetchSize = mStatement.downgradeFetchSize(aRows);
        if (mFetchSize < aRows)
        {
            mWarning = Error.createWarning(mWarning, ErrorDef.TOO_LARGE_FETCH_SIZE,
                                           String.valueOf(mFetchSize),
                                           String.valueOf(aRows));
        }
    }

    public final int getFetchDirection() throws SQLException
    {
        throwErrorForClosed();
        return ResultSet.FETCH_FORWARD;
    }

    public final void setFetchDirection(int aDirection) throws SQLException
    {
        throwErrorForClosed();
        checkFetchDirection(aDirection);
    }



    // #region getter interface를 위한 공통 구현
    // BUGBUG (2012-10-18) : getXXX() 메소드는 rowDeleted() 일 때 SQL NULL에 해당하는 값을 줘야한다.
    // SQL NULL일 때 반환하는 값은 보통 객체를 반환하는 메소드면 null 값을 반환하는 메소드면 0이다.

    /**
     * @return 타겟 컬럼맵
     */
    protected abstract List<Column> getTargetColumns();

    /**
     * @return 타겟 컬럼 갯수
     */
    protected final int getTargetColumnCount()
    {
        return getTargetColumns().size();
    }

    /**
     * column index에 해당하는 Column 객체를 얻는다.
     * 
     * @param aColumnIndex column index (1 base)
     * @return Column 객체
     */
    protected final Column getTargetColumn(int aColumnIndex)
    {
        return getTargetColumns().get(aColumnIndex - 1);
    }

    /**
     * column index에 해당하는 column 정보를 얻는다.
     * 
     * @param aColumnIndex column index (1 base)
     * @return 컬럼 정보
     */
    protected ColumnInfo getTargetColumnInfo(int aColumnIndex)
    {
        return getTargetColumn(aColumnIndex).getColumnInfo();
    }

    public final int findColumn(String aColName) throws SQLException
    {
        throwErrorForClosed();
        // (spec) column name은 case insensitive. 같은 이름이 있으면 앞의 거 반환.
        for (int i = 1; i <= getTargetColumnCount(); i++)
        {
            if (getTargetColumnInfo(i).getDisplayColumnName().equalsIgnoreCase(aColName))
            {
                return i;
            }
        }
        Error.throwSQLException(ErrorDef.INVALID_COLUMN_NAME, aColName);
        return 0;
    }

    public final ResultSetMetaData getMetaData() throws SQLException
    {
        throwErrorForClosed();
        
        if(getTargetColumns() == null)
        {
            // mMetaData is null
            return mMetaData;
        }
        
        if (mMetaData == null)
        {
            // _prowid를 쓰는 타입이면, _prowid는 떼고 넘겨준다.
            if (getType() == TYPE_SCROLL_SENSITIVE || getConcurrency() == CONCUR_UPDATABLE)
            {
                mMetaData = new AltibaseResultSetMetaData(getTargetColumns(), getTargetColumnCount() - 1);
            }
            else
            {
                mMetaData = new AltibaseResultSetMetaData(getTargetColumns());
            }
            ((AltibaseResultSetMetaData)mMetaData).setCatalogName(mStatement.mConnection.getCatalog());
        }
        return mMetaData;
    }

    /**
     * ResultSet 상태와 column index가 유효한지 확인한다.
     * 
     * @param aColumnIndex column index (1 base)
     * @throws SQLException ResultSet이 이미 닫힌 경우
     * @throws SQLException column index가 유효하지 않을 경우
     * @throws SQLException 커서 위치가 before first 또는 after last인 경우
     */
    private void checkStateAndColumnIndexForGetXXX(int aColumnIndex) throws SQLException
    {
        throwErrorForClosed();
        if (size() == 0)
        {
            Error.throwSQLException(ErrorDef.EMPTY_RESULTSET);
        }
        if (aColumnIndex < 1 || aColumnIndex > getTargetColumnCount())
        {
            Error.throwSQLException(ErrorDef.INVALID_COLUMN_INDEX, "1 ~ " + getTargetColumnCount(), String.valueOf(aColumnIndex));
        }
        if (!mCursorTested)
        {
            if (isBeforeFirst())
            { 
                Error.throwSQLException(ErrorDef.CURSOR_AT_BEFORE_FIRST);
            }
            if (isAfterLast())
            {   
                Error.throwSQLException(ErrorDef.CURSOR_AT_AFTER_LAST);
            }
            mCursorTested = true;
        }
    }

    /**
     * 커서 위치가 바꼈음을 알린다.
     * <p>
     * 커서 위치가 바뀌면 다음에 데이타를 얻을 때 유효한 위치인지 다시 확인한다.
     */
    protected final void cursorMoved()
    {
        mCursorTested = false;
        mLastReadColumnIndex = 0;
    }

    public final Array getArray(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Array type");
    }

    public final Array getArray(String aColumnName) throws SQLException
    {
        return getArray(findColumn(aColumnName));
    }

    public final InputStream getAsciiStream(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        InputStream sStream = getTargetColumn(aColumnIndex).getAsciiStream();
        if (sStream instanceof BlobInputStream)
        {
            BlobInputStream blobStream = (BlobInputStream)sStream;
            if (blobStream.isClosed())
            {
                blobStream.open(mStatement.mConnection.channel()); // BUG-38008 blobStream이 닫혀있을때는 채널을 열도록 처리
            }
        }
        return sStream;
    }

    public final InputStream getAsciiStream(String aColumnName) throws SQLException
    {
        return getAsciiStream(findColumn(aColumnName));
    }

    public final BigDecimal getBigDecimal(int aColumnIndex, int aScale) throws SQLException
    {
        BigDecimal sResult = getBigDecimal(aColumnIndex);
        
        if (sResult != null)
        {
            // BUG-43937 BigDecimal이 immutable이기 때문에 setScale결과를 다시 sResult로 할당한다.
            sResult = sResult.setScale(aScale, BigDecimal.ROUND_HALF_EVEN);
        }
        
        return sResult;
    }

    public final BigDecimal getBigDecimal(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getBigDecimal();
    }

    public final BigDecimal getBigDecimal(String aColumnName, int aScale) throws SQLException
    {
        return getBigDecimal(findColumn(aColumnName), aScale);
    }

    public final BigDecimal getBigDecimal(String aColumnName) throws SQLException
    {
        return getBigDecimal(findColumn(aColumnName));
    }

    public final InputStream getBinaryStream(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        Column sColumn = getTargetColumn(aColumnIndex);
        
        if (!(sColumn instanceof LobLocatorColumn))
        {
            Error.throwSQLException(ErrorDef.INVALID_TYPE_CONVERSION, sColumn.getDBColumnTypeName(), "BinaryStream");
        }

        BlobInputStream sStream = (BlobInputStream)getTargetColumn(aColumnIndex).getBinaryStream();
        if (sStream.isClosed()) // BUG-48892 sStream 닫혀있을때는 채널을 열도록 처리
        {
            sStream.open(mStatement.mConnection.channel());
        }

        return sStream;
    }

    public final InputStream getBinaryStream(String aColumnName) throws SQLException
    {
        return getBinaryStream(findColumn(aColumnName));
    }

    public final Blob getBlob(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        // IMPROVEMENT beforeFirst 후에 다시 가져오면 새로 만든다. 메모리 최적화가 필요함.
        Column sColumn = getTargetColumn(aColumnIndex);
        if (sColumn.isNull() && !mAllowLobNullSelect)
        {
            // BUG-47639 lob 컬럼값이 null이고 lob_null_select jdbc속성이 false 라면 Blob객체를 리턴하지 않고 null을 리턴한다.
            return null;
        }
        AltibaseBlob sBlob = (AltibaseBlob)sColumn.getBlob();
        if (sBlob != null)
        {
            sBlob.open(mStatement.mConnection.channel());
        }
        return sBlob;
    }

    public final Blob getBlob(String aColumnName) throws SQLException
    {
        return getBlob(findColumn(aColumnName));
    }

    public final boolean getBoolean(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return false;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getBoolean();
    }

    public final boolean getBoolean(String aColumnName) throws SQLException
    {
        return getBoolean(findColumn(aColumnName));
    }

    public final byte getByte(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return 0;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getByte();
    }

    public final byte getByte(String aColumnName) throws SQLException
    {
        return getByte(findColumn(aColumnName));
    }

    public final byte[] getBytes(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getBytes();
    }

    public final byte[] getBytes(String aColumnName) throws SQLException
    {
        return getBytes(findColumn(aColumnName));
    }

    public final Reader getCharacterStream(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        Reader sReader = getTargetColumn(aColumnIndex).getCharacterStream();
        if (sReader instanceof ClobReader)
        {
            ClobReader sClobReader = ((ClobReader)sReader);
            if (sClobReader.isClosed())
            {
                sClobReader.open(mStatement.mConnection.channel());
            }
        }
        return sReader;
    }

    public final Reader getCharacterStream(String aColumnName) throws SQLException
    {
        return getCharacterStream(findColumn(aColumnName));
    }

    public final Clob getClob(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;

        Column sColumn = getTargetColumn(aColumnIndex);
        if (sColumn.isNull() && !mAllowLobNullSelect)
        {
            // BUG-47639 lob 컬럼값이 null이고 lob_null_select jdbc속성이 false 라면 Clob객체를 리턴하지 않고 null을 리턴한다.
            return null;
        }
        AltibaseClob sClob = (AltibaseClob)sColumn.getClob();
        if (sClob != null)
        {
            sClob.open(mStatement.mConnection.channel());
        }
        return sClob;
    }

    public final Clob getClob(String aColumnName) throws SQLException
    {
        return getClob(findColumn(aColumnName));
    }

    public final Date getDate(int aColumnIndex, Calendar aCal) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getDate(aCal);
    }

    public final Date getDate(int aColumnIndex) throws SQLException
    {
        return getDate(aColumnIndex, null);
    }

    public final Date getDate(String aColumnName, Calendar aCal) throws SQLException
    {
        return getDate(findColumn(aColumnName), aCal);
    }

    public final Date getDate(String aColumnName) throws SQLException
    {
        return getDate(findColumn(aColumnName));
    }

    public final double getDouble(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return 0;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getDouble();
    }

    public final double getDouble(String aColumnName) throws SQLException
    {
        return getDouble(findColumn(aColumnName));
    }

    public final float getFloat(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return 0;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getFloat();
    }

    public final float getFloat(String aColumnName) throws SQLException
    {
        return getFloat(findColumn(aColumnName));
    }

    public final int getInt(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return 0;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getInt();
    }

    public final int getInt(String aColumnName) throws SQLException
    {
        return getInt(findColumn(aColumnName));
    }

    public final long getLong(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return 0;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getLong();
    }

    public final long getLong(String aColumnName) throws SQLException
    {
        return getLong(findColumn(aColumnName));
    }

    public final Object getObject(int aColumnIndex, Map aMap) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("User defined type");
    }

    public final Object getObject(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getObject();
    }

    public final Object getObject(String aColumnName, Map aMap) throws SQLException
    {
        return getObject(findColumn(aColumnName), aMap);
    }

    public final Object getObject(String aColumnName) throws SQLException
    {
        return getObject(findColumn(aColumnName));
    }

    public final Ref getRef(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Ref type");
    }

    public final Ref getRef(String aColumnName) throws SQLException
    {
        return getRef(findColumn(aColumnName));
    }

    public final short getShort(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return 0;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getShort();
    }

    public final short getShort(String aColumnName) throws SQLException
    {
        return getShort(findColumn(aColumnName));
    }

    public final String getString(int aColumnIndex) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getString();
    }

    public final String getString(String aColumnName) throws SQLException
    {
        return getString(findColumn(aColumnName));
    }

    public final Time getTime(int aColumnIndex, Calendar aCal) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getTime(aCal);
    }

    public final Time getTime(int aColumnIndex) throws SQLException
    {
        return getTime(aColumnIndex, null);
    }

    public final Time getTime(String aColumnName, Calendar aCal) throws SQLException
    {
        return getTime(findColumn(aColumnName), aCal);
    }

    public final Time getTime(String aColumnName) throws SQLException
    {
        return getTime(findColumn(aColumnName));
    }

    public final Timestamp getTimestamp(int aColumnIndex, Calendar aCal) throws SQLException
    {
        checkStateAndColumnIndexForGetXXX(aColumnIndex);
        if (rowDeleted())
        {
            return null;
        }
        mLastReadColumnIndex = aColumnIndex;
        return getTargetColumn(aColumnIndex).getTimestamp(aCal);
    }

    public final Timestamp getTimestamp(int aColumnIndex) throws SQLException
    {
        return getTimestamp(aColumnIndex, null);
    }

    public final Timestamp getTimestamp(String aColumnName, Calendar aCal) throws SQLException
    {
        return getTimestamp(findColumn(aColumnName), aCal);
    }

    public final Timestamp getTimestamp(String aColumnName) throws SQLException
    {
        return getTimestamp(findColumn(aColumnName));
    }

    public final URL getURL(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("URL type");
    }

    public final URL getURL(String aColumnName) throws SQLException
    {
        return getURL(findColumn(aColumnName));
    }

    public final InputStream getUnicodeStream(int aColumnIndex) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Deprecated: getUnicodeStream");
    }

    public final InputStream getUnicodeStream(String aColumnName) throws SQLException
    {
        return getUnicodeStream(findColumn(aColumnName));
    }

    public final boolean wasNull() throws SQLException
    {
        throwErrorForClosed();
        if (mLastReadColumnIndex == 0) 
        {  
            Error.throwSQLException(ErrorDef.WAS_NULL_CALLED_BEFORE_CALLING_GETXXX);
        }
        return getTargetColumn(mLastReadColumnIndex).isNull();
    }

    // #endregion



    // #region updatable interface 공통 구현

    public void updateArray(int aColumnIndex, Array aValue) throws SQLException
    {
        /* BUG-49233 기존에는 AltibaseReadableResultSet에서는 readonly에러가,
           AltibaseUpdatableResultSet에서는 Not Supported 에러가 발생했지만 어짜피 지원되지 않는 기능이기 때문에
           상위 클래스인 AltibaseResultSet에서 SQLFeatureNotSupportedException으로 처리한다.  */
        throw Error.createSQLFeatureNotSupportedException("Array type");
    }

    public void updateArray(String aColumnName, Array aValue) throws SQLException
    {
        updateArray(findColumn(aColumnName), aValue);
    }

    // BUG-47465 long 길이 인자를 가지는 updateAsciiStream, updateBinaryStream, updateCharacterStream 추가
    public void updateAsciiStream(String aColumnName, InputStream aValue) throws SQLException
    {
        updateAsciiStream(findColumn(aColumnName), aValue, Long.MAX_VALUE);
    }

    public void updateAsciiStream(String aColumnName, InputStream aValue, int aLength) throws SQLException
    {
        updateAsciiStream(findColumn(aColumnName), aValue, (long)aLength);
    }

    public void updateAsciiStream(String aColumnName, InputStream aValue, long aLength) throws SQLException
    {
        updateAsciiStream(findColumn(aColumnName), aValue, aLength);
    }

    public void updateAsciiStream(int aColumnIndex, InputStream aValue) throws SQLException
    {
        updateAsciiStream(aColumnIndex, aValue, Long.MAX_VALUE);
    }

    public void updateAsciiStream(int aColumnIndex, InputStream aValue, int aLength) throws SQLException
    {
        updateAsciiStream(aColumnIndex, aValue, (long)aLength);
    }

    public void updateBigDecimal(String aColumnName, BigDecimal aValue) throws SQLException
    {
        updateBigDecimal(findColumn(aColumnName), aValue);
    }

    public void updateBinaryStream(String aColumnName, InputStream aValue) throws SQLException
    {
        updateBinaryStream(findColumn(aColumnName), aValue, Long.MAX_VALUE);
    }

    public void updateBinaryStream(String aColumnName, InputStream aValue, int aLength) throws SQLException
    {
        updateBinaryStream(findColumn(aColumnName), aValue, (long)aLength);
    }

    public void updateBinaryStream(String aColumnName, InputStream aValue, long aLength) throws SQLException
    {
        updateBinaryStream(findColumn(aColumnName), aValue, aLength);
    }

    public void updateBinaryStream(int aColumnIndex, InputStream aValue) throws SQLException
    {
        updateBinaryStream(aColumnIndex, aValue, Long.MAX_VALUE);
    }

    public void updateBinaryStream(int aColumnIndex, InputStream aValue, int aLength) throws SQLException
    {
        updateBinaryStream(aColumnIndex, aValue, (long)aLength);
    }

    public void updateBlob(String aColumnName, Blob aValue) throws SQLException
    {
        updateBlob(findColumn(aColumnName), aValue);
    }

    public void updateBoolean(String aColumnName, boolean aValue) throws SQLException
    {
        updateBoolean(findColumn(aColumnName), aValue);
    }

    public void updateByte(String aColumnName, byte aValue) throws SQLException
    {
        updateByte(findColumn(aColumnName), aValue);
    }

    public void updateBytes(String aColumnName, byte[] aValue) throws SQLException
    {
        updateBytes(findColumn(aColumnName), aValue);
    }

    public void updateCharacterStream(String aColumnName, Reader aReader) throws SQLException
    {
        updateCharacterStream(findColumn(aColumnName), aReader, Long.MAX_VALUE);
    }

    public void updateCharacterStream(String aColumnName, Reader aReader, int aLength) throws SQLException
    {
        updateCharacterStream(findColumn(aColumnName), aReader, (long)aLength);
    }

    public void updateCharacterStream(String aColumnName, Reader aReader, long aLength) throws SQLException
    {
        updateCharacterStream(findColumn(aColumnName), aReader, aLength);
    }

    public void updateCharacterStream(int aColumnIndex, Reader aValue) throws SQLException
    {
        updateCharacterStream(aColumnIndex, aValue, Long.MAX_VALUE);
    }

    public void updateCharacterStream(int aColumnIndex, Reader aValue, int aLength) throws SQLException
    {
        updateCharacterStream(aColumnIndex, aValue, (long)aLength);
    }

    public void updateClob(String aColumnName, Clob aValue) throws SQLException
    {
        updateClob(findColumn(aColumnName), aValue);
    }

    public void updateDate(String aColumnName, Date aValue) throws SQLException
    {
        updateDate(findColumn(aColumnName), aValue);
    }

    public void updateDouble(String aColumnName, double aValue) throws SQLException
    {
        updateDouble(findColumn(aColumnName), aValue);
    }

    public void updateFloat(String aColumnName, float aValue) throws SQLException
    {
        updateFloat(findColumn(aColumnName), aValue);
    }

    public void updateInt(String aColumnName, int aValue) throws SQLException
    {
        updateInt(findColumn(aColumnName), aValue);
    }

    public void updateLong(String aColumnName, long aValue) throws SQLException
    {
        updateLong(findColumn(aColumnName), aValue);
    }

    public void updateNull(String aColumnName) throws SQLException
    {
        updateNull(findColumn(aColumnName));
    }

    public void updateObject(String aColumnName, Object aValue, int aScale) throws SQLException
    {
        updateObject(findColumn(aColumnName), aValue, aScale);
    }

    public void updateObject(String aColumnName, Object aValue) throws SQLException
    {
        updateObject(findColumn(aColumnName), aValue);
    }

    public void updateRef(int aColumnIndex, Ref aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Ref type");
    }

    public void updateRef(String aColumnName, Ref aValue) throws SQLException
    {
        updateRef(findColumn(aColumnName), aValue);
    }

    public void updateShort(String aColumnName, short aValue) throws SQLException
    {
        updateShort(findColumn(aColumnName), aValue);
    }

    public void updateString(String aColumnName, String aValue) throws SQLException
    {
        updateString(findColumn(aColumnName), aValue);
    }

    public void updateTime(String aColumnName, Time aValue) throws SQLException
    {
        updateTime(findColumn(aColumnName), aValue);
    }

    public void updateTimestamp(String aColumnName, Timestamp aValue) throws SQLException
    {
        updateTimestamp(findColumn(aColumnName), aValue);
    }

    // #endregion



    // #region Altibase 특수 기능

    /**
     * Plan text를 얻는다.
     * <p>
     * 관련 Statement에 대한 Plan text를 얻는 것이므로, 만약 ResultSet을 얻은 후 다른 구문을 수행했다면 다른
     * 쿼리에 대한 Plan text를 얻을 수 있다.
     * <p>
     * 단순 Plan text 조회를 원한다면 Prepare 또는 Execute 후에, 보다 정확한 Plan text 조회를 원한다면
     * Fetch 완료 후에 수행할것을 권장한다.
     * 
     * @return Plan text
     * @throws SQLException Plan text 요청이나 결과를 얻는데 실패했을 때
     */
    public final String getExplainPlan() throws SQLException
    {
        throwErrorForClosed();
        return mStatement.getExplainPlan();
    }

    // #endregion



    protected final void throwErrorForReadOnly() throws SQLException
    {
        throwErrorForClosed();
        if (getConcurrency() == ResultSet.CONCUR_READ_ONLY) 
        {
            Error.throwSQLException(ErrorDef.NOT_SUPPORTED_OPERATION_ON_READ_ONLY);
        }
    }

    protected final void throwErrorForForwardOnly() throws SQLException
    {
        throwErrorForClosed();
        if (getType() == ResultSet.TYPE_FORWARD_ONLY) 
        {
            Error.throwSQLException(ErrorDef.NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY);
        }
    }

    protected final void throwErrorForScrollInsensitive() throws SQLException
    {
        throwErrorForClosed();
        if (getType() == ResultSet.TYPE_SCROLL_INSENSITIVE) 
        {
            Error.throwSQLException(ErrorDef.NOT_SUPPORTED_OPERATION_ON_SCROLL_INSENSITIVE);
        }
    }

    protected final void throwErrorForClosed() throws SQLException
    {
        if (isClosed())
        {
            Error.throwSQLException(ErrorDef.CLOSED_RESULTSET);
        }
        mStatement.throwErrorForClosed();
    }

    // BUG-46513 cursor가 열려있는 statement가 있는지 확인하기 위해 추가
    public boolean fetchRemains()
    {
        return mContext.getFetchResult().fetchRemains();
    }

    public void setClosed(boolean aClosed)
    {
        this.mClosed = aClosed;
    }

    public void setAllowLobNullSelect(boolean aAllowLobNullSelect)
    {
        mAllowLobNullSelect = aAllowLobNullSelect;
    }

    @Override
    public void updateNString(String aColumnName, String aValue) throws SQLException
    {
        updateNString(findColumn(aColumnName), aValue);
    }

    @Override
    public String getNString(int aColumnIndex) throws SQLException
    {
        return getString(aColumnIndex);
    }

    @Override
    public String getNString(String aColumnName) throws SQLException
    {
        return getNString(findColumn(aColumnName));
    }

    @Override
    public void updateBlob(int aColumnIndex, InputStream aValue) throws SQLException
    {
        updateBlob(aColumnIndex, aValue, Long.MAX_VALUE);
    }

    @Override
    public void updateBlob(String aColumnName, InputStream aValue, long aLength) throws SQLException
    {
        updateBlob(findColumn(aColumnName), aValue, aLength);
    }

    @Override
    public void updateClob(String aColumnName, Reader aValue, long aLength) throws SQLException
    {
        updateClob(findColumn(aColumnName), aValue, aLength);
    }

    @Override
    public void updateClob(int aColumnIndex, Reader aValue) throws SQLException
    {
        updateClob(aColumnIndex, aValue, Long.MAX_VALUE);
    }

    @Override
    public void updateClob(String aColumnName, Reader aValue) throws SQLException
    {
        updateClob(findColumn(aColumnName), aValue);
    }

    @Override
    @SuppressWarnings("unchecked")
    public <T> T getObject(int aColumnIndex, Class<T> aType) throws SQLException
    {
        if (aType == null)
        {
            throw Error.createSQLException(ErrorDef.TYPE_PARAMETER_CANNOT_BE_NULL);
        }

        if (aType.equals(Struct.class) || aType.equals(RowId.class) || aType.equals(NClob.class) ||
            aType.equals(SQLXML.class) || aType.equals(Array.class) || aType.equals(Ref.class) ||
            aType.equals(URL.class))
        {
            throw Error.createSQLFeatureNotSupportedException();
        }

        if (aType.equals(LocalDate.class))
        {
            Date sDate = getDate(aColumnIndex);
            return sDate == null ? null : aType.cast(sDate.toLocalDate());
        }
        else if (aType.equals(LocalDateTime.class))
        {
            Timestamp sTimestamp = getTimestamp(aColumnIndex);
            return sTimestamp == null ? null : aType.cast(sTimestamp.toLocalDateTime());
        }
        else if (aType.equals(LocalTime.class))
        {
            Time sTime = getTime(aColumnIndex);
            return sTime == null ? null : aType.cast(sTime.toLocalTime());
        }
        else if (aType.equals(String.class))
        {
            return (T) getString(aColumnIndex);
        }
        else if (aType.equals(BigDecimal.class))
        {
            return (T) getBigDecimal(aColumnIndex);
        }
        else if (aType.equals(Boolean.class) || aType.equals(Boolean.TYPE))
        {
            return (T) Boolean.valueOf(getBoolean(aColumnIndex));
        }
        else if (aType.equals(Integer.class) || aType.equals(Integer.TYPE))
        {
            return (T) Integer.valueOf(getInt(aColumnIndex));
        }
        else if (aType.equals(Long.class) || aType.equals(Long.TYPE))
        {
            return (T) Long.valueOf(getLong(aColumnIndex));
        }
        else if (aType.equals(Float.class) || aType.equals(Float.TYPE))
        {
            return (T) Float.valueOf(getFloat(aColumnIndex));
        }
        else if (aType.equals(Double.class) || aType.equals(Double.TYPE))
        {
            return (T) Double.valueOf(getDouble(aColumnIndex));
        }
        else if (aType.equals(byte[].class))
        {
            return (T) getBytes(aColumnIndex);
        }
        else if (aType.equals(Date.class))
        {
            return (T) getDate(aColumnIndex);
        }
        else if (aType.equals(Time.class))
        {
            return (T) getTime(aColumnIndex);
        }
        else if (aType.equals(Timestamp.class))
        {
            return (T) getTimestamp(aColumnIndex);
        }
        else if (aType.equals(Clob.class))
        {
            return (T) getClob(aColumnIndex);
        }
        else if (aType.equals(Blob.class))
        {
            return (T) getBlob(aColumnIndex);
        }
        else
        {
            try
            {
                return aType.cast(getObject(aColumnIndex));
            }
            catch (ClassCastException aClassCastEx)
            {
                throw Error.createSQLException(ErrorDef.TYPE_CONVERSION_NOT_SUPPORTED, aType.getName(),
                                               aClassCastEx);
            }
        }
    }

    @Override
    public <T> T getObject(String aColumnName, Class<T> aType) throws SQLException
    {
        return getObject(findColumn(aColumnName), aType);
    }

    @Override
    public void updateBlob(String aColumnName, InputStream aValue) throws SQLException
    {
        updateBlob(findColumn(aColumnName), aValue);
    }
}
