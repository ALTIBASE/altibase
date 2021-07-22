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

package Altibase.jdbc.driver.sharding.core;

import Altibase.jdbc.driver.AltibaseFailover;
import Altibase.jdbc.driver.AltibasePreparedStatement;
import Altibase.jdbc.driver.cm.CmProtocolContextShardConnect;
import Altibase.jdbc.driver.cm.CmProtocolContextShardStmt;
import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.ColumnFactory;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.net.URL;
import java.sql.*;
import java.sql.Date;
import java.util.*;

import static Altibase.jdbc.driver.sharding.util.ShardingTraceLogger.shard_log;

public class AltibaseShardingPreparedStatement extends AbstractShardingPreparedStatement
{
    private List<Column>                  mParameters;
    InternalShardingPreparedStatement     mInternalPstmt;
    private AltibasePreparedStatement     mServersidePstmt;
    private ColumnFactory                 mColumnFactory;
    private boolean                       mShouldRecordBindParameters;

    AltibaseShardingPreparedStatement(AltibaseShardingConnection aShardCon,
                                      String aSql, int aResultSetType, int aResultSetConcurrency,
                                      int aResultSetHoldability) throws SQLException
    {
        super(aShardCon, aResultSetType, aResultSetConcurrency, aResultSetHoldability);
        setSql(aSql);
        mParameters = new ArrayList<Column>();
        mSetParamInvocationMap = new LinkedHashMap<Integer, JdbcMethodInvocation>();
        mColumnFactory = new ColumnFactory();
        mColumnFactory.setProperties(aShardCon.getProps());
        // BUG-47460 bind paramter를 record해야 하는 조건
        mShouldRecordBindParameters = aShardCon.isLazyNodeConnect() || mIsReshardEnabled;

        try
        {
            // BUG-47274 analyze시 기존에 받아온 statement id를 이용한다.
            mMetaConn.getShardProtocol().shardAnalyze(mShardStmtCtx, aSql, mStatementForAnalyze.getID());
        }
        catch (SQLException aEx)
        {
            AltibaseFailover.trySTF(aShardCon.getMetaConnection().failoverContext(), aEx);
        }

        if (mShardStmtCtx.getShardAnalyzeResult().isCoordQuery())
        {
            createServerSideStatement(aShardCon, aSql, aResultSetType, aResultSetConcurrency,
                                      aResultSetHoldability);
        }
        else
        {
            mInternalPstmt = new DataNodeShardingPreparedStatement(aShardCon,
                                                                   aSql,
                                                                   aResultSetType,
                                                                   aResultSetConcurrency,
                                                                   aResultSetHoldability,
                                                                   this);
        }
        mInternalStmt = mInternalPstmt;
        mShardMetaNumber = mMetaConn.getShardContextConnect().getShardMetaNumber();
    }

    private void createServerSideStatement(AltibaseShardingConnection aShardCon, String aSql,
                                           int aResultSetType, int aResultSetConcurrency,
                                           int aResultSetHoldability) throws SQLException
    {
        // BUG-46513 ONENODE이고 analyze 결과가 성공이 아닐경우에는 예외를 던진다.
        if (mShardStmtCtx.getGlobalTransactionLevel() == GlobalTransactionLevel.ONE_NODE)
        {
            mSqlwarning = Error.processServerError(mSqlwarning, mShardStmtCtx.getError());
        }
        Connection sCon = aShardCon.getMetaConnection();
        mServersidePstmt = (AltibasePreparedStatement)sCon.prepareStatement(aSql, aResultSetType,
                                                                            aResultSetConcurrency,
                                                                            aResultSetHoldability);
        // BUG-46513 서버사이드 샤딩일때 fetch시점의 SMN값을 체크하기 위해 meta connection을 AltibaseStatement에 주입한다.
        mServersidePstmt.setMetaConnection(mMetaConn);
        mServerSideStmt = mServersidePstmt;
        mInternalPstmt = new ServerSideShardingPreparedStatement(mServersidePstmt, mMetaConn);
        mIsCoordQuery = true;
    }

    public ResultSet executeQuery() throws SQLException
    {
        mMetaConn.checkCommitState();
        reAnalyzeIfNecessary();

        ResultSet sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();

        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalPstmt.executeQuery();
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            // BUG-46790 SMN update는 execute 처리후 에러가 발생해도 수행해야 한다.
            updateShardMetaNumberIfNecessary();
        }
        // BUG-46513 autocommit on 상태이고 노드가 제거된 상태라면 해당하는 Statement의 resultset을 정리해준다.
        clearClosedResultSets(sResult);

        return sResult;
    }

    public ResultSet executeQuery(String aSql) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "executeQuery(String sql)");
        return null;
    }

    public int executeUpdate() throws SQLException
    {
        mMetaConn.checkCommitState();
        reAnalyzeIfNecessary();

        int sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();

        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalPstmt.executeUpdate();
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public int executeUpdate(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "executeUpdate(String sql, int autoGenKey)");
        return 0;
    }

    public int executeUpdate(String aSql, int[] aColumnIndexes) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "executeUpdate(String sql, int[] columnIndex)");
        return 0;
    }

    public int executeUpdate(String aSql, String[] aColumnNames) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "executeUpdate(String sql, String[] columnNames)");
        return 0;
    }

    public int executeUpdate(String aSql) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "executeUpdate(String sql)");
        return 0;
    }

    public void setNull(int aParameterIndex, int aSqlType) throws SQLException
    {
        setParameter(aParameterIndex, null, Types.NULL);
        if (mShouldRecordBindParameters)
        {
            recordSetParameterForNull(new Class[] { int.class, int.class }, aParameterIndex, aSqlType);
        }
        mInternalPstmt.setNull(aParameterIndex, aSqlType);
    }

    public void setBoolean(int aParameterIndex, boolean aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.VARCHAR);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setBoolean", new Class[] { int.class, boolean.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setBoolean(aParameterIndex, aValue);
    }

    public void setByte(int aParameterIndex, byte aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.SMALLINT);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setByte", new Class[] { int.class, byte.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setByte(aParameterIndex, aValue);
    }

    public void setShort(int aParameterIndex, short aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.SMALLINT);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setShort", new Class[] { int.class, short.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setShort(aParameterIndex, aValue);
    }

    public void setInt(int aParameterIndex, int aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.INTEGER);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setInt", new Class[] { int.class, int.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setInt(aParameterIndex, aValue);
    }

    public void setLong(int aParameterIndex, long aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.BIGINT);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setLong", new Class[] { int.class, long.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setLong(aParameterIndex, aValue);
    }

    public void setFloat(int aParameterIndex, float aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.REAL);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setFloat", new Class[] { int.class, float.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setFloat(aParameterIndex, aValue);
    }

    public void setDouble(int aParameterIndex, double aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.DOUBLE);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setDouble", new Class[] { int.class, double.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setDouble(aParameterIndex, aValue);
    }

    public void setBigDecimal(int aParameterIndex, BigDecimal aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.NUMERIC);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setBigDecimal", new Class[] { int.class, BigDecimal.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setBigDecimal(aParameterIndex, aValue);
    }

    public void setString(int aParameterIndex, String aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.VARCHAR);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setString", new Class[] { int.class, String.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setString(aParameterIndex, aValue);
    }

    public void setBytes(int aParameterIndex, byte[] aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.BINARY);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setBytes", new Class[] { int.class, byte[].class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setBytes(aParameterIndex, aValue);
    }

    public void setDate(int aParameterIndex, Date aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.DATE);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setDate", new Class[] { int.class, Date.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setDate(aParameterIndex, aValue);
    }

    public void setTime(int aParameterIndex, Time aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.TIME);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setTime", new Class[] { int.class, Time.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setTime(aParameterIndex, aValue);
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.TIMESTAMP);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setTimestamp", new Class[] { int.class, Timestamp.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setTimestamp(aParameterIndex, aValue);
    }

    public void setAsciiStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.CLOB);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setAsciiStream", new Class[] { int.class, InputStream.class, int.class },
                               aParameterIndex, aValue, aLength);
        }
        mInternalPstmt.setAsciiStream(aParameterIndex, aValue, aLength);
    }

    public void setUnicodeStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Deprecated: setUnicodeStream");
    }

    public void setBinaryStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.BLOB);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setBinaryStream", new Class[] { int.class, InputStream.class, int.class },
                               aParameterIndex, aValue, aLength);
        }
        mInternalPstmt.setBinaryStream(aParameterIndex, aValue, aLength);
    }

    public void clearParameters() throws SQLException
    {
        mParameters.clear();
        mSetParamInvocationMap.clear();

        mInternalPstmt.clearParameters();
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType, int aScale) throws SQLException
    {
        setParameter(aParameterIndex, aValue, aTargetSqlType);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setObject", new Class[] { int.class, Object.class, int.class, int.class },
                               aParameterIndex, aValue, aTargetSqlType, aScale);
        }
        mInternalPstmt.setObject(aParameterIndex, aValue, aTargetSqlType, aScale);
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType) throws SQLException
    {
        setParameter(aParameterIndex, aValue, aTargetSqlType);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setObject", new Class[] { int.class, Object.class, int.class },
                               aParameterIndex, aValue, aTargetSqlType);
        }
        mInternalPstmt.setObject(aParameterIndex, aValue, aTargetSqlType);
    }

    public void setObject(int aParameterIndex, Object aValue) throws SQLException
    {
        if (aValue instanceof String)
        {
            setString(aParameterIndex, (String)aValue);
        }
        else if (aValue instanceof BigDecimal)
        {
            setBigDecimal(aParameterIndex, (BigDecimal)aValue);
        }
        else if (aValue instanceof Boolean)
        {
            setBoolean(aParameterIndex, (Boolean)aValue);
        }
        else if (aValue instanceof Integer)
        {
            setInt(aParameterIndex, (Integer)aValue);
        }
        else if (aValue instanceof Short)
        {
            setShort(aParameterIndex, (Short)aValue);
        }
        else if (aValue instanceof Long)
        {
            setLong(aParameterIndex, (Long)aValue);
        }
        else if (aValue instanceof Float)
        {
            setFloat(aParameterIndex, (Float)aValue);
        }
        else if (aValue instanceof Double)
        {
            setDouble(aParameterIndex, (Double)aValue);
        }
        else if (aValue instanceof byte[])
        {
            setBytes(aParameterIndex, (byte[])aValue);
        }
        else if (aValue instanceof Date)
        {
            setDate(aParameterIndex, (Date)aValue);
        }
        else if (aValue instanceof Time)
        {
            setTime(aParameterIndex, (Time)aValue);
        }
        else if (aValue instanceof Timestamp)
        {
            setTimestamp(aParameterIndex, (Timestamp)aValue);
        }
        else if (aValue instanceof Blob)
        {
            setBlob(aParameterIndex, (Blob)aValue);
        }
        else if (aValue instanceof Clob)
        {
            setClob(aParameterIndex, (Clob)aValue);
        }
        else if (aValue instanceof InputStream)
        {
            setBinaryStream(aParameterIndex, (InputStream)aValue, Integer.MAX_VALUE);
        }
        else if (aValue instanceof Reader)
        {
            setCharacterStream(aParameterIndex, (Reader)aValue, Integer.MAX_VALUE);
        }
        else if (aValue instanceof BitSet)
        {
            setParameter(aParameterIndex, aValue, Types.BIT);
        }
        else if (aValue instanceof char[])
        {
            setString(aParameterIndex, String.valueOf((char[])aValue));
        }
        else if (aValue == null)
        {
            setNull(aParameterIndex, Types.NULL);
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, aValue.getClass().getName());
        }
    }

    public boolean execute() throws SQLException
    {
        mMetaConn.checkCommitState();
        reAnalyzeIfNecessary();

        boolean sResult;
        short sRetryCount = mMetaConn.getShardStatementRetry();

        try
        {
            while(true)
            {
                try
                {
                    sResult = mInternalPstmt.execute();
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResult;
    }

    public boolean execute(String aSql, int aAutoGeneratedKeys) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "execute(String sql, int autoGenKey)");
        return false;
    }

    public boolean execute(String aSql, int[] aColumnIndexes) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "execute(String sql, int[] columnIndex)");
        return false;
    }

    public boolean execute(String aSql, String[] aColumnNames) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "execute(String sql, String[] columnNames)");
        return false;
    }

    public boolean execute(String aSql) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "execute(String sql)");
        return false;
    }

    public void addBatch() throws SQLException
    {
        mInternalPstmt.addBatch();
    }

    public int[] executeBatch() throws SQLException
    {
        mMetaConn.checkCommitState();
        reAnalyzeIfNecessary();

        int[] sResults;
        short sRetryCount = mMetaConn.getShardStatementRetry();

        try
        {
            while(true)
            {
                try
                {
                    sResults = mInternalPstmt.executeBatch();
                    break;
                }
                catch (SQLException aEx)
                {
                    // STATEMENT_TOO_OLD 이 외 에러가 있을 경우는 stmt retry 하지 않는다. 
                    checkStmtTooOld(aEx, sRetryCount);
                    --sRetryCount;
                }
            }
        }
        finally
        {
            updateShardMetaNumberIfNecessary();
        }

        return sResults;
    }

    public void setCharacterStream(int aParameterIndex, Reader aReader, int aLength) throws SQLException
    {
        setParameter(aParameterIndex, aReader, Types.CLOB);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setCharacterStream", new Class[] { int.class, Reader.class, int.class },
                               aParameterIndex, aReader, aLength);
        }
        mInternalPstmt.setCharacterStream(aParameterIndex, aReader, aLength);
    }

    public void setRef(int aParameterIndex, Ref aValue) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Ref type");
    }

    public void setBlob(int aParameterIndex, Blob aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.BLOB);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setBlob", new Class[] { int.class, Blob.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setBlob(aParameterIndex, aValue);
    }

    public void setClob(int aParameterIndex, Clob aValue) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.CLOB);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setClob", new Class[] { int.class, Clob.class }, aParameterIndex, aValue);
        }
        mInternalPstmt.setClob(aParameterIndex, aValue);
    }

    public void setArray(int aParameterIndex, Array aValue) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "Array type");
    }

    public ResultSetMetaData getMetaData() throws SQLException
    {
        return mInternalPstmt.getMetaData();
    }

    public void setDate(int aParameterIndex, Date aValue, Calendar aCal) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.DATE);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setDate", new Class[] { int.class, Date.class, Calendar.class },
                               aParameterIndex, aValue, aCal);
        }
        mInternalPstmt.setDate(aParameterIndex, aValue, aCal);
    }

    public void setTime(int aParameterIndex, Time aValue, Calendar aCal) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.TIME);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setTime", new Class[] { int.class, Time.class, Calendar.class }, aParameterIndex, aValue, aCal);
        }
        mInternalPstmt.setTime(aParameterIndex, aValue, aCal);
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue, Calendar aCal) throws SQLException
    {
        setParameter(aParameterIndex, aValue, Types.TIMESTAMP);
        if (mShouldRecordBindParameters)
        {
            recordSetParameter("setTimestamp", new Class[] { int.class, Timestamp.class, Calendar.class },
                               aParameterIndex, aValue, aCal);
        }
        mInternalPstmt.setTimestamp(aParameterIndex, aValue, aCal);
    }

    public void setNull(int aParameterIndex, int aSqlType, String aTypeName) throws SQLException
    {
        setParameter(aParameterIndex, null, Types.NULL);
        if (mShouldRecordBindParameters)
        {
            recordSetParameterForNull(new Class[] { int.class, int.class, String.class },
                                      aParameterIndex, aSqlType, aTypeName);
        }
        mInternalPstmt.setNull(aParameterIndex, aSqlType, aTypeName);
    }

    public void setURL(int aParameterIndex, URL aValue) throws SQLException
    {
        Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "URL type");
    }

    public ParameterMetaData getParameterMetaData() throws SQLException
    {
        return mInternalPstmt.getParameterMetaData();
    }

    public void clearBatch() throws SQLException
    {
        mInternalPstmt.clearBatch();
    }

    /**
     * SMN 값이 맞지 않은 경우 analyze를 다시 수행한다.
     * @throws SQLException shard analyze 중 에러가 발생했을때
     */
    private void reAnalyzeIfNecessary() throws SQLException
    {
        if (!mIsReshardEnabled)
        {
            return;
        }
        CmProtocolContextShardConnect sShardContextConnect = mMetaConn.getShardContextConnect();
        if (mShardMetaNumber >= sShardContextConnect.getShardMetaNumber())
        {
            return;
        }
        mShardStmtCtx = new CmProtocolContextShardStmt(mMetaConn, mStatementForAnalyze);
        try
        {
            mMetaConn.getShardProtocol().shardAnalyze(mShardStmtCtx, mSql, mStatementForAnalyze.getID());
        }
        catch (SQLException aEx)
        {
            AltibaseFailover.trySTF(mMetaConn.getMetaConnection().failoverContext(), aEx);
        }
        // BUG-46513 ONENODE이고 analyze 결과가 성공이 아닐경우에는 예외를 던진다.
        if (mShardStmtCtx.getGlobalTransactionLevel() == GlobalTransactionLevel.ONE_NODE)
        {
            mSqlwarning = Error.processServerError(mSqlwarning, mShardStmtCtx.getError());
        }
        // BUG-47357 mInternalPstmt를 새로 만들지 않고 재활용한다.
        mInternalPstmt.rePrepare(mShardStmtCtx);

        if (mShardStmtCtx.getShardAnalyzeResult().isCoordQuery())
        {
            // BUG-47357 서버사이드인 경우 prepare후 setParameter를 다시 해줘야 한다.
            replaySetParameter(mServersidePstmt);
        }
        setShardMetaNumber(sShardContextConnect.getShardMetaNumber());
    }

    private void setParameter(int aParameterIndex, Object aValue, int aType) throws SQLException
    {
        Column sColumn;
        if (mParameters.size() == aParameterIndex - 1)
        {
            sColumn = mColumnFactory.getMappedColumn(aType);
            sColumn.setValue(aValue);
            mParameters.add(sColumn);
            return;
        }
        for (int i = mParameters.size(); i <= aParameterIndex - 1; i++)
        {
            mParameters.add(null);
        }

        if (mParameters.get(aParameterIndex - 1) == null)
        {
            sColumn = mColumnFactory.getMappedColumn(aType);
            sColumn.setValue(aValue);
        }
        else
        {
            sColumn = mParameters.get(aParameterIndex - 1);
            if (sColumn.getDBColumnType() != aType)
            {
                sColumn = mColumnFactory.getMappedColumn(aType);
            }
            sColumn.setValue(aValue);
        }
        mParameters.set(aParameterIndex - 1, sColumn);
    }

    private void recordSetParameter(String aMethodName, Class[] aArgTypes, Object... aArgs) throws SQLException
    {
        try
        {
            JdbcMethodInvocation sMethodInvocation = new JdbcMethodInvocation(
                    PreparedStatement.class.getMethod(aMethodName, aArgTypes), aArgs);
            mSetParamInvocationMap.put((Integer)aArgs[0], sMethodInvocation);
            shard_log("(RECORD SET PARAMETER) {0}", sMethodInvocation);
        }
        catch (NoSuchMethodException aException)
        {
            Error.throwSQLException(ErrorDef.SHARD_JDBC_METHOD_INVOKE_ERROR, aException.getMessage());
        }
    }

    private void recordSetParameterForNull(Class[] aArgumentTypes, Object... aArguments) throws SQLException
    {
        try
        {
            JdbcMethodInvocation sMethodInvocation = new JdbcMethodInvocation(
                    PreparedStatement.class.getMethod("setNull", aArgumentTypes),
                    aArguments);
            mSetParamInvocationMap.put((Integer)aArguments[0], sMethodInvocation);
            shard_log("(RECORD SET PARAMETER FOR NULL ) {0}", sMethodInvocation);
        }
        catch (NoSuchMethodException aException)
        {
            Error.throwSQLException(ErrorDef.SHARD_JDBC_METHOD_INVOKE_ERROR, aException.getMessage());
        }
    }

    List<Column> getParameters()
    {
        return mParameters;
    }

    @Override
    public void setAsciiStream(int aParameterIndex, InputStream aStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBinaryStream(int aParameterIndex, InputStream aStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setCharacterStream(int aParameterIndex, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setAsciiStream(int aParameterIndex, InputStream aStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBinaryStream(int aParameterIndex, InputStream aStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setCharacterStream(int aParameterIndex, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setClob(int aParameterIndex, Reader aReader) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBlob(int aParameterIndex, InputStream aInputStream) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setNString(int aParameterIndex, String aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setClob(int aParameterIndex, Reader aReader, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }

    @Override
    public void setBlob(int aParameterIndex, InputStream aInputStream, long aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException();
    }
}
