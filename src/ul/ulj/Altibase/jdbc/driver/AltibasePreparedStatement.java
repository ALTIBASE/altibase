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
import java.net.URL;
import java.sql.*;
import java.sql.Date;
import java.time.*;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.cm.*;
import Altibase.jdbc.driver.datatype.*;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.util.AltiSqlProcessor;

import static Altibase.jdbc.driver.sharding.core.AltibaseShardingFailover.*;

public class AltibasePreparedStatement extends AbstractPreparedStatement
{
    // protected RowHandle mBatchRowHandle;
    protected int mBatchJobCount;
    protected boolean mBatchAdded = false;
    protected List mContext;
    protected List<Column> mBindColumns;
    protected BatchRowHandle mBatchRowHandle;
    // LobUpdator는 해당 테이블의 LOB 컬럼에 INSERT나 UPDATE가 발생할 경우 사용된다.
    // PreparedStatement의 setter method를 해당 테이블의 LOB 컬럼에 대해 사용할 경우
    // execute protocol이 아닌 LOB 관련 protocol을 통해 데이터가 삽입된다.
    // 이를 위해 해당 column에 삽입할 데이터들을 LobUpdator에 저장해둔 후, execute 후에 LOB protocol을 통해 삽입한다.
    protected LobUpdator mLobUpdator = null;
    // PROJ-2368 Batch 작업에 Atomic Operation 적용 여부 Flag, Default 는 false
    private boolean mIsAtomicBatch = false;
    // BUG-40081 batch처리시 임시컬럼값을 저장할때 사용된다.
    private List mTempArgValueList = new ArrayList();
    private static Logger mLogger;
    private final boolean    mIsPingQuery;      // BUG-49143 ping query 판별용 flag

    public AltibasePreparedStatement(AltibaseConnection aConnection, String aSql, int aResultSetType, int aResultSetConcurrency, int aResultSetHoldability) throws SQLException
    {
        super(aConnection, aResultSetType, aResultSetConcurrency, aResultSetHoldability);

        if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
        {
            mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
        }  
        
        if (aSql == null)
        {
            Error.throwSQLException(ErrorDef.NULL_SQL_STRING);
        }

        if (mEscapeProcessing)
        {
            aSql = AltiSqlProcessor.processEscape(aSql);
        }
        setSql(aSql);

        mIsPingQuery = isPingSQL(aSql);
        if (mIsPingQuery) // BUG-49143 ping 쿼리일 경우 prepare 요청을 처리하지 않고 그냥 리턴한다.
        {
            return;
        }

        prepare(aSql);  // BUG-47357 prepare요청만 재사용이 가능하게 하기 위해 메소드 추출
    }

    @SuppressWarnings("unchecked")
    public void prepare(String aSql) throws SQLException
    {
        CmProtocolContextPrepExec sContext = (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);

        try
        {
            aSql = procDowngradeAndGetTargetSql(aSql);
            // BUG-42712 deferred 상태일때는 prepare 동작을 내부 ArrayList에 저장만 하고 넘어간다.
            if (mIsDeferred)
            {
                Map<String, Object> sMethodInfo = new HashMap<>();
                sMethodInfo.put("methodname", "prepare");
                sMethodInfo.put("args", new Object[] {
                        sContext,
                        mStmtCID,
                        aSql,
                        getResultSetHoldability() == ResultSet.HOLD_CURSORS_OVER_COMMIT,
                        usingKeySetDriven(),
                        mConnection.nliteralReplaceOn(),
                        mIsDeferred });
                sContext.addDeferredRequest(sMethodInfo);
            }
            else
            {
                CmProtocol.prepare(sContext,
                                   mStmtCID,
                                   aSql,
                                   (getResultSetHoldability() == ResultSet.HOLD_CURSORS_OVER_COMMIT),
                                   usingKeySetDriven(),
                                   mConnection.nliteralReplaceOn(),
                                   false);
            }
            mPrepareResult = sContext.getPrepareResult();
        }
        catch (SQLException aEx)
        {
            tryShardFailOver(mConnection, aEx);
        }

        if (sContext.getError() != null)
        {
            mWarning = Error.processServerError(mWarning, sContext.getError());
        }
        if (mPrepareResultColumns == null)
        {
            mPrepareResultColumns = getProtocolContext().getGetColumnInfoResult().getColumns();
        }

        //BUG-42424 deferred인 경우에는 파라메터의 갯수가 몇 개인지 알 수 없으므로 ArrayList로 선언
        mBindColumns = new ArrayList<Column>();
        if (mIsDeferred)
        {
            // BUG-42424 deferred인 경우 PrepareResult 정보를 수동생성한다.
            mPrepareResult.setResultSetCount(1);
            mPrepareResult.setStatementId(0);
        }
        else
        {
            // BUG-42424 deferred가 아닌 경우 파라메터 갯수만큼 List를 null로 채운다.
            for (int i = 0; i < mPrepareResult.getParameterCount(); i++)
            {
                mBindColumns.add(null);
            }
        }
        // BUG-48380 reuse_resultset jdbc 속성이 true일때만 ResultSet을 재사용한다.
        // BUG-48381 deferred 처리 후 reuse_resultset을 초기화 한다.
        if (mConnection.canReUseResultSet())
        {
            makeReUsableResultSet();
        }
        // BUG-47460 성능을 위해 select인 경우 prepared flag를 enable한다.
        if (mPrepareResult.isSelectStatement())
        {
            CmFetchResult sFetchResult = (CmFetchResult)sContext.getCmResult(CmFetchResult.MY_OP);
            sFetchResult.setPrepared(true);
        }
    }

    private void makeReUsableResultSet() throws SQLException
    {
        // BUG-48380 resultset 이 한 개 일때는 미리 생성해 둔다. 샤드 커넥션일때는 무시
        if (!mConnection.isShardConnection() && mPrepareResult.getResultSetCount() == 1
            && !(this instanceof CallableStatement))  // BUG-48431 PSM으로부터는 ResultSet이 여러개 생성될 수 있으므로 제외
        {
            if (mTargetResultSetType == ResultSet.TYPE_FORWARD_ONLY &&
                mTargetResultSetConcurrency == ResultSet.CONCUR_READ_ONLY)
            {
                mCurrentResultSet = new AltibaseForwardOnlyResultSet(this, this.getProtocolContext(), this.getFetchSize());
                mCurrentResultSet.setAllowLobNullSelect(mConnection.getAllowLobNullSelect());
                mFetchResult = getProtocolContext().getFetchResult();
                mFetchResult.setRowHandle(new ArrayListRowHandle());
                mFetchResult.setUseArrayListRowHandle(true);
                mReUseResultSet = true;
            }
        }
    }

    public void setAtomicBatch(boolean aValue) throws SQLException
    {
        // Target Table 에 Lob Column 이 존재하는 경우, Atomic Batch 를 사용할 수 없음
        if ((aValue == true && mBatchRowHandle instanceof DynamicArrayRowHandle))
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "atomic batch with LOB column");
        }
        mIsAtomicBatch = aValue;
    }

    public boolean getAtomicBatch()
    {
        return mIsAtomicBatch;
    }

    protected void createProtocolContext()
    {
        if(mContext==null)
        {
            mContext = new ArrayList(1);
        }
        
        CmProtocolContextPrepExec sContext = new CmProtocolContextPrepExec(mConnection.channel());
        sContext.setDistTxInfo(mConnection.getDistTxInfo());
        mContext.add(sContext);
    }

    public CmProtocolContextDirExec getProtocolContext()
    {
        return (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);
    }
    
    protected void closeAllResultSet() throws SQLException
    {
        Iterator iter = mExecuteResultMgr.iterator();
        while(iter.hasNext())
        {
            ((ExecuteResult)iter.next()).mReturned = true;
        }
    }
    
    protected void closeAllPrevResultSet() throws SQLException
    {
        for(int i=0; i<mExecuteResultMgr.size(); i++)
        {
            mExecuteResultMgr.get(i).mReturned = true;
            if(i==mCurrentResultIndex)
            {
                break;
            }
        }
    }
    
    protected void closeCurrentResultSet() throws SQLException
    {
        if(mExecuteResultMgr.size() > 0 && mExecuteResultMgr.size() >= (mCurrentResultIndex - 1))
        {
            (mExecuteResultMgr.get(mCurrentResultIndex)).mReturned = true;
        }
    }

    protected Column getColumn(int aIndex, int aSqlType) throws SQLException
    {
        return getColumn(aIndex, aSqlType, null);
    }

    protected Column getColumn(int aIndex, int aSqlType, Object aValue) throws SQLException
    {
        throwErrorForClosed();
        checkBindColumnLength(aIndex);
        
        CmProtocolContextPrepExec sContext = (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);
        ColumnInfo sColumnInfo = sContext.getBindParamResult().getColumnInfo(aIndex);
        if (sColumnInfo == null)
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, "DataType: " + String.valueOf(aSqlType));
        }
        if (aSqlType == Types.NULL)
        {
            if (mBindColumns.get(aIndex - 1) == null)
            {
                createBindColumnInfo(aIndex - 1, sColumnInfo);
            }
        }
        else
        {
            // BUG-43719 setBytes를 했을때 byte[]의 크기에 따라 lob locator를 사용해야 하는지 여부를 결정한다.
            boolean sChangeToLob = (aValue instanceof byte[] &&
                                    ((byte[])aValue).length > ColumnConst.MAX_BYTE_LENGTH);
            changeBindColumnInfo(changeSqlType(aSqlType, sColumnInfo.getDataType(), sChangeToLob), aIndex, sColumnInfo);
        }
        return mBindColumns.get(aIndex - 1);
    }

    /**
     * Bind Column 정보가 없을 때 새로 구성한다.<br/>이때 ColumnInfo객체의 change type flag를 enable시킨다.
     * @param aIndex
     * @param aColumnInfo
     */
    private void createBindColumnInfo(int aIndex, ColumnInfo aColumnInfo)
    {
        int sColumntype = this.changeColumnType(aColumnInfo.getDataType());
        mBindColumns.set(aIndex, mConnection.channel().getColumnFactory().getInstance(sColumntype));
        ColumnInfo sClonedColumnInfo = (ColumnInfo)aColumnInfo.clone();
        // BUG-40081 setNull이 호출되었을때는 ColumnInfo 객체의 shouldChangeType flag를 true로 셋팅한다.
        sClonedColumnInfo.setShouldChangeType(true);
        mBindColumns.get(aIndex).getDefaultColumnInfo(sClonedColumnInfo);
        mBindColumns.get(aIndex).setColumnInfo(sClonedColumnInfo);
    }

    /**
     * 데이타를 주고받는데 사용하지 않는 타입은, 적합한 타입으로 바꿔준다.</br>
     * CLOB, BLOB, GEOMETRY, CHAR, NCHAR의 컬럼타입을 각각 CLOB_LOCATOR, BLOB_LOCATOR, BINARY, </br>
     * VARCHAR, NVARCHAR로 바꿔준다.
     */
    private int changeColumnType(int aColumnType)
    {
        int sResultColumnType = aColumnType;
        switch (aColumnType)
        {
            case ColumnTypes.CLOB:
                sResultColumnType = ColumnTypes.CLOB_LOCATOR;
                break;
            case ColumnTypes.BLOB:
                sResultColumnType = ColumnTypes.BLOB_LOCATOR;
                break;
            case ColumnTypes.GEOMETRY:
                sResultColumnType = ColumnTypes.BINARY;
                break;

            // Batch에서의 CHAR 문제 우회를 위한 꼼수 (ref. BUG-39624)
            case ColumnTypes.CHAR:
                sResultColumnType = ColumnTypes.VARCHAR;
                break;
            case ColumnTypes.NCHAR:
                sResultColumnType = ColumnTypes.NVARCHAR;
                break;
            default:
                break;
        }
        return sResultColumnType;
    }

    /**
     * NCHAR 컬럼과 GEOMETRY, LOB 컬럼은 상황에 맞게 보정해 준다.
     * @param aSqlType
     * @param aColumnType
     * @param aChangeToLob lob 컬럼으로 변경해야 하는지를 나타내는 flag
     * @return
     */
    private int changeSqlType(int aSqlType, int aColumnType, boolean aChangeToLob)
    {
        int sResultSqlType = aSqlType;

        switch (aSqlType)
        {
            // NCHAR 컬럼은 CHAR 컬럼과 charset이 다르므로 보정해주어야 한다.
            case Types.CHAR:
                if (ColumnTypes.isNCharType(aColumnType))
                {
                    sResultSqlType = AltibaseTypes.NVARCHAR;
                }
                else
                {
                    // CHAR 문제 우회를 위한 꼼수: padding (ref. BUG-27818), Batch (ref. BUG-39624)
                    sResultSqlType = AltibaseTypes.VARCHAR;
                }
                break;
            case Types.VARCHAR:
                if (ColumnTypes.isNCharType(aColumnType))
                {
                    sResultSqlType = AltibaseTypes.NVARCHAR;
                }
                break;

            // setBytes(), setObject(BINARY)로도 geometry를 설정할 수 있게 하기 위함.
            // BUGBUG (2012-12-14) GEOMFROMWKB() 등을 썼을때는 여전히 안된다; 왠지 ColumnType이 VARCHAR로 나와서리;
            case Types.BINARY:
                if (ColumnTypes.isGeometryType(aColumnType))
                {
                    sResultSqlType = AltibaseTypes.GEOMETRY;
                }
                /* clientsideautocommit이 활성화 되어 잇거나 BUG-43719 byte[] 사이즈가 65534를 넘을 때는 lob type으로 변경한다. */
                if (mConnection.isClientSideAutoCommit() || aChangeToLob)
                {
                    if (aColumnType == ColumnTypes.BLOB)
                    {
                        sResultSqlType = AltibaseTypes.BLOB;
                    }
                    else if (aColumnType == ColumnTypes.CLOB)
                    {
                        sResultSqlType = AltibaseTypes.CLOB;
                    }
                }

                break;
            case Types.NUMERIC:
                // BUG-41061 overflow처리를 위해 type이 float인 경우 numeric에서 float으로 변경시킨다. 
                if (aColumnType == ColumnTypes.FLOAT)
                {
                    sResultSqlType = AltibaseTypes.FLOAT;
                }
                break;
            default:
                break;
        }
        return sResultSqlType;
    }

    /**
     * bind column 정보가 없거나 type이 변경되었을때 정보를 재구성한다.<br/>
     * 이때 ColumnInfo 객체의 change flag가 enable 되어 있는 경우 exception을 발생시키지 않고 bind 컬럼타입을 교체한다.
     * @param aSqlType
     * @param aIndex
     * @param aColumnInfo
     * @throws SQLException
     */
    private void changeBindColumnInfo(int aSqlType, int aIndex, ColumnInfo aColumnInfo) throws SQLException
    {
        ColumnInfo sColumnInfo = aColumnInfo;
        int sColumnType = sColumnInfo.getDataType();
        Column sBindColumn = mBindColumns.get(aIndex - 1);
        if (sBindColumn == null || !sBindColumn.isMappedJDBCType(aSqlType))
        {
            // IN-OUT 타입은 서버에서 받아오는게 아니므로 백업해뒀다가 다시 써야한다.
            byte sInOutType = (sBindColumn == null) ? ColumnInfo.IN_OUT_TARGET_TYPE_NONE : 
                                  sBindColumn.getColumnInfo().getInOutTargetType(); 
            Column sMappedColumn = mConnection.channel().getColumnFactory().getMappedColumn(aSqlType);

            /* BUG-45572 bind type 변경시 columnInfo를 항상 clone 하도록 변경 */
            if (sMappedColumn == null)
            {
                Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, AltibaseTypes.toString(aSqlType) + " type");
            }
            
            // BUG-40081 batch모드이고 ColumnInfo의 changetype flag가 true일때는 bind column type을 바꿔준다.
            if (mBatchAdded && shouldChangeBindColumnType(aSqlType, aIndex))
            {
                mBatchRowHandle.changeBindColumnType(aIndex - 1, sMappedColumn, sColumnInfo, sInOutType);
            }
            else
            {
                sColumnInfo = (ColumnInfo)sColumnInfo.clone();

                if (sMappedColumn.getDBColumnType() != sColumnType)
                {
                    sColumnInfo.modifyInOutType(sInOutType);
                    sMappedColumn.getDefaultColumnInfo(sColumnInfo);
                }
                
                sMappedColumn.setColumnInfo(sColumnInfo);

                mBindColumns.set(aIndex - 1, sMappedColumn);
            }
        }
    }
    
    /**
     * mArgValueList의 크기만큼 루프를 돌면서 임지저장소에 저장했던 값들을 RowHandle에 store한다.
     * @throws SQLException
     */
    private void storeTempArgValuesToRowHandle() throws SQLException
    {
        ArrayList sArgOriValue = new ArrayList();
        // BUG-40081 원래 값들을 별도의 ArrayList에 저장한다.
        for (Column sColumn : mBindColumns)
        {
            sArgOriValue.add(sColumn.getObject());
        }

        int sStoreCursorPos = mTempArgValueList.size();
        // BUG-40081 임시저장소에 저장했던 값들을 bind column에 셋팅한 후 store한다.
        for (int sIdx=0 ; sIdx < sStoreCursorPos; sIdx++)
        {
            ArrayList sArgumentInfo = (ArrayList)mTempArgValueList.get(sIdx);
            for (int sColumnCnt = 0; sColumnCnt < mBindColumns.size(); sColumnCnt++)
            {
                setObjectInternal(sArgumentInfo.get(sColumnCnt), mBindColumns.get(sColumnCnt));
            }
            mBatchRowHandle.store();
        }
        mTempArgValueList.clear();
        // BUG-40081 원래 값들을 복원한다.
        for (int sIdx=0; sIdx < sArgOriValue.size(); sIdx++)
        {
            mBindColumns.get(sIdx).setValue(sArgOriValue.get(sIdx));
        }
    }

    /**
     * BUG-40081 batch 모드일때 바인드된 컬럼 중 type을 바꿔야 하는지 확인한다.</br>
     * ColumnInfo객체의 change flag가 false이거나 기존 컬럼이 LOB_LOCATOR인 경우에는 
     * CANNOT_BIND_THE_DATA_TYPE_DURING_ADDING_BATCH_JOB exception을 던진다.
     * @param aSqlType
     * @param aIndex
     * @return
     * @throws SQLException
     */
    private boolean shouldChangeBindColumnType(int aSqlType, int aIndex) throws SQLException
    {
        if (mBindColumns.get(aIndex - 1) != null)
        {
            ColumnInfo sColumnInfo = mBindColumns.get(aIndex - 1).getColumnInfo();
            // BUG-40081 기존 컬럼이 LOB_LOCATOR이거나 set null flag가 false일때는 Exception을 던진다.
            if (!(sColumnInfo.shouldChangeType()) || sColumnInfo.getDataType() == ColumnTypes.CLOB_LOCATOR || 
                    sColumnInfo.getDataType() == ColumnTypes.BLOB_LOCATOR)
            {
                Set<Integer> sMappedJdbcTypeSet = mBindColumns.get(aIndex - 1).getMappedJDBCTypes();
                Error.throwSQLException(ErrorDef.CANNOT_BIND_THE_DATA_TYPE_DURING_ADDING_BATCH_JOB,
                                        String.valueOf(sMappedJdbcTypeSet.iterator().next()),
                                        String.valueOf(aSqlType));
            }
            else 
            {
                return true;
            }
        }
        return false;
    }

    protected Column getColumnForInType(int aIndex, int aSqlType) throws SQLException
    {
        return getColumnForInType(aIndex, aSqlType, null);
    }

    private Column getColumnForInType(int aIndex, int aSqlType, Object aValue) throws SQLException
    {
        throwErrorForClosed();

        if (mIsDeferred)
        {
            // BUG-42424 deferred인 경우 ColumnInfo를 수동생성한다.
            addMetaColumnInfo(aIndex, aSqlType, getDefaultPrecisionForDeferred(aSqlType, aValue));
        }
        
        Column sColumn = getColumn(aIndex, aSqlType, aValue);
        sColumn.getColumnInfo().addInType();
        return sColumn;
    }

    public ResultSet getResultSet() throws SQLException
    {
        throwErrorForClosed();

        if(mCurrentResultIndex != 0)
        {
            createProtocolContext();
            copyProtocolContextExceptFetchContext();
        }
        return super.getResultSet();
    }
    
    protected void incCurrentResultSetIndex()
    {
        mCurrentResultIndex++;
        createProtocolContext();
        copyProtocolContextExceptFetchContext();
    }
    
    private void copyProtocolContextExceptFetchContext()
    {
        int sPrevResultIndex = mCurrentResultIndex - 1;
        CmProtocolContextPrepExec sCurProtocolContextPrepExec = (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);
        CmProtocolContextPrepExec sPrevProtocolContextPrepExec = (CmProtocolContextPrepExec)mContext.get(sPrevResultIndex);
        
        sCurProtocolContextPrepExec.addCmResult(sPrevProtocolContextPrepExec.getExecutionResult());
        sCurProtocolContextPrepExec.addCmResult(sPrevProtocolContextPrepExec.getPrepareResult());
    }
    
    private void checkParameters() throws SQLException
    {
        for (int i = 0; i < mBindColumns.size(); i++)
        {
            if (mBindColumns.get(i) == null)
            {
                Error.throwSQLException(ErrorDef.NEED_MORE_PARAMETER, String.valueOf(i + 1));
            }
        }
    }

    public void addBatch() throws SQLException
    {
        throwErrorForClosed();
        
        // 첫  번째 addBatch 일 때만 수행 
        if (!mBatchAdded)
        {
            checkParameters();
            
            if (mLobUpdator != null)
            {
                mBatchRowHandle = new DynamicArrayRowHandle();
            }
            else
            {
                mBatchRowHandle = new ListBufferHandle();
                // ListBufferHandle 의 Encoder 에 적용할 charset name 들을 가져와야한다.
                ((CmBufferWriter)mBatchRowHandle).setCharset(mConnection.channel().getCharset(),
                                                             mConnection.channel().getNCharset());
            }

            mBatchRowHandle.setColumns(mBindColumns);
            mBatchRowHandle.initToStore();
        }        
        // BUG-24704 parameter가 없으면 1건만 add
        else if (mBindColumns.size() == 0)
        {
            return;
        }
        
        if (mBatchRowHandle.size() == Integer.MAX_VALUE)
        {
            Error.throwSQLException(ErrorDef.TOO_MANY_BATCH_JOBS);
        }

        // BUG-40081 LOB컬럼이 없고 컬럼에 setNull이 호출되었다면 RowHandle에 저장하지않고 임지저장소에 value값들을 저장한다.
        if (mLobUpdator == null)
        {
            if (setNullColumnExist())
            {
                List sArgumentInfo = new ArrayList();
                for (int sIdx = 0; sIdx < mBindColumns.size(); sIdx++)
                {
                    sArgumentInfo.add(mBindColumns.get(sIdx).getObject());
                }
                mTempArgValueList.add(sArgumentInfo);
                mBatchAdded = true;
                return;
            }
            /* BUG-40081 null column은 없지만 임시argument값이 존재한다면 임시저장소에 있는 argument값들을 </br>
               RowHandle로 이동시킨다.  */
            else if (mTempArgValueList.size() > 0)
            {
                storeTempArgValuesToRowHandle();
            }
        }
        mBatchRowHandle.store();
        mBatchAdded = true;
    }

    /**
     * Bind된 컬럼 중 setNull에 의해 Null셋팅이 되어 있는 컬럼이 있는지 확인한다.
     */
    private boolean setNullColumnExist()
    {
        boolean sNullColumnExist = false;
        for (Column sColumn : mBindColumns)
        {
            if (sColumn.getColumnInfo().shouldChangeType())
            {
                sNullColumnExist = true;
                break;
            }
        }
        return sNullColumnExist;
    }

    public void clearParameters() throws SQLException
    {
        throwErrorForClosed();
        for (int i = 0; i < mBindColumns.size(); i++)
        {
            mBindColumns.set(i, null);
        }
    }

    public void clearBatch() throws SQLException
    {
        mBatchAdded = false;
        clearParameters();
    }

    public int[] executeBatch() throws SQLException
    {
        return toIntBatchUpdateCounts(executeLargeBatch());
    }

    @Override
    public long[] executeLargeBatch() throws SQLException
    {
        throwErrorForClosed();
        if (mPrepareResult.isSelectStatement())
        {
            Error.throwBatchUpdateException(ErrorDef.INVALID_BATCH_OPERATION_WITH_SELECT);
        }

        clearAllResults();

        if (!mBatchAdded)
        {
            mExecuteResultMgr.add(new ExecuteResult(false, -1));
            return EMPTY_LARGE_BATCH_RESULT;
        }

        // Lob Column 이 존재하는 경우 이전 RowHandle 사용
        if (mLobUpdator != null)
        {
            ((DynamicArrayRowHandle)mBatchRowHandle).beforeFirst();
        }
        else if (setNullColumnExist()) // BUG-40081 setNull컬럼 flag가 있는 경우 임시저장소의 값들을 RowHandle로 store한다.
        {
            storeTempArgValuesToRowHandle();
        }

        mBatchJobCount = mBatchRowHandle.size();

        if (mBatchJobCount == 0)
        {
            Error.throwSQLException(ErrorDef.NO_BATCH_JOB);
        }

        try
        {
            // LOB Column 이 존재하지 않는 경우에만 List Protocol 을 사용
            if(mLobUpdator == null)
            {
                CmProtocol.preparedExecuteBatchUsingList((CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex),
                                                          mBindColumns,
                                                          (ListBufferHandle)mBatchRowHandle,
                                                          mBatchJobCount,
                                                          mIsAtomicBatch);
            }
            else
            {
                DynamicArrayRowHandle sRowHandle = (DynamicArrayRowHandle)mBatchRowHandle;

                CmProtocol.preparedExecuteBatch((CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex),
                                               mBindColumns,
                                               sRowHandle,
                                               mBatchJobCount);
                sRowHandle.beforeFirst();
                mLobUpdator.updateLobColumns();
            }
            /* PROJ-2190 executeBatch 후 commit 프로토콜 전송  */
            CmProtocol.clientCommit((CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex), mConnection.isClientSideAutoCommit());
        }
        catch (SQLException aEx)
        {
            tryShardFailOver(mConnection, aEx);
        }
        finally
        {
            // BUGBUG (2012-11-28) 스펙에서는 executeBatch 했을때 clear되는게 명확하게 나오지 않은 듯. oracle에 따른다.
            clearBatch();
        }
        try
        {
            super.afterExecution();
        }
        catch (SQLException sEx)
        {
            CmExecutionResult sExecResult = getProtocolContext().getExecutionResult();
            long[] sLongUpdatedRowCounts = sExecResult.getUpdatedRowCounts();
            Error.throwBatchUpdateException(sEx, toIntBatchUpdateCounts(sLongUpdatedRowCounts));
        }
        long[] sUpdatedRowCounts = mExecutionResult.getUpdatedRowCounts();
        int sUpdateCount = 0;

        if (mIsAtomicBatch && mPrepareResult.isInsertStatement())
        {
            /* Atomic Operation Insert 에 성공 했을 경우, array index 0 에 삽입에 성공한 레코드 숫자의 합이 반환 된다.
               (Atomic 은 단일 Insert 구문만 고려하여 설계하였기 때문에, 단일 Insert 구문이 아닌 경우, 동작을 보장할 수 없다.)
               Atomic Operation 도중 실패하면 상단의 SQLException 절에서 처리 되므로 여기서는 성공 했을 경우만 고려하면 된다.
                              기존 executeBatch Interface 에 맞춰주기 위해 삽입에 성공한 레코드 숫자 Size 의 int array 를 만들고 전부 1 을 넣어 반환한다. */
            sUpdatedRowCounts = new long[(int)sUpdatedRowCounts[0]];

            for(int i = 0; i < sUpdatedRowCounts.length; i++)
            {
                sUpdatedRowCounts[i] = 1;
            }
        }
        else
        {
            for (int i = 0; i < sUpdatedRowCounts.length; i++)
            {
                sUpdateCount += sUpdatedRowCounts[i];
            }
        }

        mExecuteResultMgr.add(new ExecuteResult(false, sUpdateCount));
        return sUpdatedRowCounts;
    }

    public boolean execute() throws SQLException
    {
        throwErrorForClosed();
        throwErrorForBatchJob("execute");

        if (mIsPingQuery)  // BUG-49143 ping 쿼리일 경우에는 execute프로토콜을 서버로 전송하지 않고 내부적으로 처리한다.
        {
            pingAndCreateLightweightResultSet();
            return true;
        }

        checkParameters();

        clearAllResults();
        CmProtocolContextPrepExec sContext = (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);

        try
        {
            // PROJ-2427 cursor를 닫아야 하는 조건을 같이 넘겨준다.
            CmProtocol.preparedExecute(sContext,
                                       mBindColumns,
                                       mConnection.isClientSideAutoCommit(),
                                       mPrepareResult != null && mPrepareResult.isSelectStatement());
        }
        catch (SQLException aEx)
        {
            tryShardFailOver(mConnection, aEx);
        }
        super.afterExecution();

        if (mLobUpdator != null)
        {
            mLobUpdator.updateLobColumns(); 
            /* PROJ-2190 lob 컬럼이 포함되어 있을 때는 updateLobColumns 후 별도로 commit 프로토콜을 전송한다. */
            CmProtocol.clientCommit((CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex), mConnection.isClientSideAutoCommit());
        }
        
        boolean sResult = processExecutionResult();
        executeForGeneratedKeys();
        return sResult;
    }

    public ResultSet executeQuery() throws SQLException
    {
        throwErrorForClosed();
        throwErrorForBatchJob("executeQuery");

        if (mIsPingQuery)  // BUG-49143 ping 쿼리일 경우에는 execute프로토콜을 서버로 전송하지 않고 내부적으로 처리한다.
        {
            pingAndCreateLightweightResultSet();
            return mCurrentResultSet;
        }

        // BUGBUG (2013-02-21) ResultSet을 만들지 않는 PSM을 수행하면 예외는 나지만 수행은 된다.
        // 서버에서 확인하지 않으므로 ResultSet을 만들 가능성이 있는 SELECT, PSM, DEQUEUE가 아닐때만 바로 예외를 던진다.
        // PSM이 ResultSet을 만들지 않을 경우에는 processExecutionQueryResult()에서 예외를 올리는데,
        // 쿼리 수행은 이미 정상적으로 다 수행 된 다음이다.
        // BUG-42424 deferred 상태일때는 sql문의 타입을 알 수 없기때문에 해당 조건을 무시한다.
        if (!mIsDeferred && !mPrepareResult.isSelectStatement() &&
            !mPrepareResult.isStoredProcedureStatement() && !mPrepareResult.isDequeueStatement())
        {
            Error.throwSQLException(ErrorDef.NO_RESULTSET, getSql());
        }
        checkParameters();

        CmProtocolContextPrepExec sContext = (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);
        super.clearAllResults();

        try
        {
            // PROJ-2427 cursor를 닫아야 하는 조건을 같이 넘겨준다.
            CmProtocol.preparedExecuteAndFetch(sContext,
                                               mBindColumns,
                                               mFetchSize,
                                               mMaxRows,
                                               mMaxFieldSize,
                                               mPrepareResult != null && mPrepareResult.isSelectStatement());
        }
        catch (SQLException ex)
        {
            tryShardFailOver(mConnection, ex);
        }
        super.afterExecution();

        // BUG-48380 매번 executeQuery() 할 때마다 ResultSet을 생성하지 않고 기존에 미리 만들어 놓은 ResultSet객체를 재활용한다.
        ResultSet sResult = (mReUseResultSet) ? resetCurrentResultSet() : processExecutionQueryResult(getSql());
        executeForGeneratedKeys();
        return sResult;
    }

    private ResultSet resetCurrentResultSet() throws SQLException
    {
        mCurrentResultSet.setClosed(false);
        if (mFetchSize > 0)
        {
            mCurrentResultSet.setFetchSize(mFetchSize); // BUG-48380 ResultSet 객체에 Statement의 fetch size를 다시 셋팅해 줘야 한다.
        }
        mResultSetReturned = true;

        return mCurrentResultSet;
    }

    public int executeUpdate() throws SQLException
    {
        return toInt(executeLargeUpdate());
    }

    public ParameterMetaData getParameterMetaData() throws SQLException
    {
        throwErrorForClosed();
        // BUG-42424 deferred 일때는 먼저 파라메터 메타정보를 받아와야 getParameterMetaData를 수행할 수 있다.
        if (mIsDeferred)
        {
            receivePrepareResults();
        }
        return new AltibaseParameterMetaData(mBindColumns);
    }

    public void setArray(int aParameterIndex, Array aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Array type");
    }

    public void setAsciiStream(int aParameterIndex, InputStream aValue) throws SQLException
    {
        setAsciiStream(aParameterIndex, aValue, Long.MAX_VALUE);
    }

    public void setAsciiStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        setAsciiStream(aParameterIndex, aValue, (long)aLength);
    }

    public void setAsciiStream(int aParameterIndex, InputStream aValue, long aLength) throws SQLException
    {
        ClobLocatorColumn sClobColumn = (ClobLocatorColumn)getColumnForInType(aParameterIndex, Types.CLOB);
        
        if (mLobUpdator == null && aValue != null)
        {
            mLobUpdator = new LobUpdator(this);
        }
        
        if(aValue != null)
        {
            mLobUpdator.addLobColumn(sClobColumn, aValue, aLength);
        }
    }

    public void setBigDecimal(int aParameterIndex, BigDecimal aValue) throws SQLException
    {
        // BUG-48431 Float 컬럼일 경우 scale한계값 계산 방식이 틀리기 때문에 반드시 서버로부터 바인드파라메터 정보를 받아와야 한다.
        if (mIsDeferred)
        {
            receivePrepareResults();
        }
        getColumnForInType(aParameterIndex, Types.NUMERIC).setValue(aValue);
    }

    public void setBinaryStream(int aParameterIndex, InputStream aValue) throws SQLException
    {
        setBinaryStream(aParameterIndex, aValue, Long.MAX_VALUE);
    }

    public void setBinaryStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        setBinaryStream(aParameterIndex, aValue, (long)aLength);
    }

    public void setBinaryStream(int aParameterIndex, InputStream aValue, long aLength) throws SQLException
    {
        BlobLocatorColumn sBlobLocatorColumn = (BlobLocatorColumn)getColumnForInType(aParameterIndex, Types.BLOB);
        if (mLobUpdator == null && aValue != null)
        {
            mLobUpdator = new LobUpdator(this);
        }
        
        if(aValue != null)
        {
            mLobUpdator.addLobColumn(sBlobLocatorColumn, aValue, aLength);
        }
    }

    public void setBlob(int aParameterIndex, Blob aValue) throws SQLException
    {
        setObject(aParameterIndex, aValue, Types.BLOB);
    }

    public void setBoolean(int aParameterIndex, boolean aValue) throws SQLException
    {
        // BUG-42424 deferred일때는 boolean의 precision을 VARCHAR와 틀리게 보내야 하기때문에 value값을 같이 보낸다.
        ((CommonCharVarcharColumn)getColumnForInType(aParameterIndex, Types.VARCHAR, 
                                                     Boolean.valueOf(aValue))).setTypedValue(aValue);
    }

    public void setByte(int aParameterIndex, byte aValue) throws SQLException
    {
        ((SmallIntColumn)getColumnForInType(aParameterIndex, Types.SMALLINT)).setTypedValue(aValue);
    }

    public void setBytes(int aParameterIndex, byte[] aValue) throws SQLException
    {
        setObject(aParameterIndex, aValue, Types.BINARY);  // BUG-38043 clientAutoCommit이 활성화 되어 있는 경우 lob locator를 사용하도록 하기 위해 setObject를 호출한다.
    }

    public void setCharacterStream(int aParameterIndex, Reader aValue) throws SQLException
    {
        setCharacterStream(aParameterIndex, aValue, Long.MAX_VALUE);
    }

    public void setCharacterStream(int aParameterIndex, Reader aValue, int aLength) throws SQLException
    {
        setCharacterStream(aParameterIndex, aValue, (long)aLength);
    }

    public void setCharacterStream(int aParameterIndex, Reader aValue, long aLength) throws SQLException
    {
        ClobLocatorColumn sClobColumn = (ClobLocatorColumn)getColumnForInType(aParameterIndex, Types.CLOB);
        
        if (mLobUpdator == null && aValue != null)
        {
            mLobUpdator = new LobUpdator(this);
        }
        
        if(aValue != null)
        {
            mLobUpdator.addLobColumn(sClobColumn, aValue, aLength);
        }
    }

    public void setClob(int aParameterIndex, Clob aValue) throws SQLException
    {
        setObject(aParameterIndex, aValue, Types.CLOB);
    }

    public void setDate(int aParameterIndex, Date aValue, Calendar aCalendar) throws SQLException
    {
        DateColumn sColumn = (DateColumn)getColumnForInType(aParameterIndex, Types.DATE);
        sColumn.setLocalCalendar(aCalendar);
        sColumn.setValue(aValue);
    }

    public void setDate(int aParameterIndex, Date aValue) throws SQLException
    {
        getColumnForInType(aParameterIndex, Types.DATE).setValue(aValue);
    }

    public void setDouble(int aParameterIndex, double aValue) throws SQLException
    {
        ((DoubleColumn)getColumnForInType(aParameterIndex, Types.DOUBLE)).setTypedValue(aValue);
    }

    public void setFloat(int aParameterIndex, float aValue) throws SQLException
    {
        getColumnForInType(aParameterIndex, Types.REAL).setValue(String.valueOf(aValue));
    }

    public void setInt(int aParameterIndex, int aValue) throws SQLException
    {
        ((IntegerColumn)getColumnForInType(aParameterIndex, Types.INTEGER)).setTypedValue(aValue);
    }

    /**
     * deferred상태일때 컬럼 메타 정보를 수동으로 생성한다. 이때 생성하는 기준은 setXXX할때의 sqlType이다.
     * @param aIndex 컬럼 인덱스(1 base)
     * @param aDataType sql type
     * @param aPrecision Precision
     */
    protected void addMetaColumnInfo(int aIndex, int aDataType, int aPrecision)
    {
        CmProtocolContextPrepExec sContext = (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);
        CmGetBindParamInfoResult sBindParamInfoResult = (CmGetBindParamInfoResult)sContext.getCmResult(CmGetBindParamInfoResult.MY_OP);
        if (sBindParamInfoResult.getColumnInfoListSize() < aIndex ||
            sBindParamInfoResult.getColumnInfo(aIndex) == null) // BUG-42424 이미 컬럼메타정보가 생성되어 있다면 건너뛴다.
        {
            ColumnInfo sColumnInfo = new ColumnInfo();
            sColumnInfo.makeDefaultValues();
            sColumnInfo.setColumnMetaInfos(aDataType, aPrecision);
            sBindParamInfoResult.addColumnInfo(aIndex, sColumnInfo);
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.INFO, "created bind param info for deferred : {0}", sColumnInfo);
            }
        }

        /*
         * BUG-42879 mBindColumns는 changeBindColumnInfo메소드에서 만들어지기때문에 여기선 null로 리스트를 채운다.
         * setXXX(3, 1)과 같이 세번째 항목을 먼저 셋팅한 경우에는 첫번째와 두번째를 null로 채운후 마지막을 null로 채운다.
         */
        // BUG-48431 keyset driven 같은 경우 bind param 정보가 이미 있기 때문에 이 경우에도 mBindColumns에 추가해 줘야 한다.
        if (mBindColumns.size() < aIndex)
        {
            for (int i = mBindColumns.size(); i < aIndex - 1; i++)
            {
                mBindColumns.add(null);
            }
            mBindColumns.add(null);
        }
    }

    public void setLong(int aParameterIndex, long aValue) throws SQLException
    {
        ((BigIntColumn)getColumnForInType(aParameterIndex, Types.BIGINT)).setTypedValue(aValue);
    }

    public void setNull(int aParameterIndex, int aSqlType) throws SQLException
    {
        // BUG-38681 batch 처리 도중 sqlType이 바뀔 수 있고 lob 컬럼인 경우 해당 값이 null이더라도
        // loblocator를 생성해야하기 때문에 setObject(,,Types.NULL)로 호출한다.
        this.setObject(aParameterIndex, null, Types.NULL);
    }

    public void setNull(int aParameterIndex, int aSqlType, String aTypeName) throws SQLException
    {
        // REF, STRUCT, DISTINCT를 지원하지 않기 때문에 aTypeName을 무시한다.
        setNull(aParameterIndex, aSqlType);
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType, int aScale) throws SQLException
    {
        if ((aValue != null) && (aTargetSqlType == Types.NUMERIC || aTargetSqlType == Types.DECIMAL))  /* BUG-45838 */
        {
            // oracle이 scale을 무시한다. 우리도 무시;
            BigDecimal sDecimalValue = new BigDecimal(aValue.toString());
            setBigDecimal(aParameterIndex, sDecimalValue);
        }
        else
        {
            setObject(aParameterIndex, aValue, aTargetSqlType);
        }
    }

    public void setObject(int aParameterIndex, Object aValue, int aTargetSqlType) throws SQLException
    {
        // BUG-42424 setObject(int, Object, int)같은 경우 서버로부터 먼저 prepare결과를 받아야한다.
        if (mIsDeferred)
        {
            receivePrepareResults();
        }

        Object sObject = convertJava8TimeToSqlTime(aValue);
        Column sColumn = getColumnForInType(aParameterIndex, aTargetSqlType, sObject);

        // BLOB과 CLOB을 분리하는 이유는 삽입 객체 유형에 따라 삽입 가능한 객체가 다르기 때문이다.
        // instanceof 연산으로 객체의 타입을 찾는 연산은 비용이 크므로 이를 줄이기 위해 == 연산으로 처리할 수 있는 부분을 미리 처리함.
        setObjectInternal(sObject, sColumn);
    }

    // BUG-42424 getMetaData가 PreparedStatement의 인터페이스이기때문에 AltibaseStatement에서 AltibasePreparedStatement로 이동한다.
    public ResultSetMetaData getMetaData() throws SQLException
    {
        throwErrorForClosed();
        throwErrorForStatementNotPrepared();

        if (mIsDeferred)
        {
            // BUG-42424 deferred상태일때는 먼저 prepare 결과를 받아와야 한다.
            receivePrepareResults();
        }
        
        if (mPrepareResult.isSelectStatement())
        {
            int sColumnCount = usingKeySetDriven() ? mPrepareResultColumns.size() - 1 : mPrepareResultColumns.size();
            return new AltibaseResultSetMetaData(mPrepareResultColumns, sColumnCount);
        }
        else
        {
            return null;
        }
    }

    /**
     * deferred 상태일때 특정한 경우에는 먼저 prepare한 결과를 받아온다.
     * @throws SQLException
     */
    private void receivePrepareResults() throws SQLException
    {
        CmProtocolContextPrepExec sContext = (CmProtocolContextPrepExec)mContext.get(mCurrentResultIndex);
        CmPrepareResult sPrepareResult = (CmPrepareResult)sContext.getCmResult(CmPrepareResult.MY_OP);
        // BUG-42424 PrepareResult가 없을 경우 defer 요청을 무시하고 prepare요청을 먼저 보낸다.
        if (!sPrepareResult.isPrepared())
        {
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                mLogger.log(Level.INFO, "Cancel deferred and receive prepare results first.");
            }
            synchronized (mConnection.channel())
            {
                // BUG-42712 prepare결과를 받기전 먼저 deferred된 요청을 버퍼에 write한다.
                CmProtocol.invokeDeferredRequests(sContext.getDeferredRequests());
                // BUG-48431 CmProtocol.prepare()에서 writeGetBindParamInfo를 생략했기 때문에 다시 요청을 보내야 한다.
                CmProtocol.writeGetBindParamInfo(sContext);
                mConnection.channel().sendAndReceive();
                CmOperation.readProtocolResult(sContext);
                mPrepareResult = sContext.getPrepareResult();
                int sParamCnt = mPrepareResult.getParameterCount();
                int sBindColumnSize = mBindColumns.size();
                // BUG-42879 현재 바인드된 컬럼수가 서버로부터 받아온 컬럼수보다 작은 경우 나머지를 null로 채운다.
                for (int i = 0; i < sParamCnt - sBindColumnSize; i++)
                {
                    mBindColumns.add(null);
                }
                if (mPrepareResultColumns == null)
                {
                    mPrepareResultColumns = getProtocolContext().getGetColumnInfoResult().getColumns();
                }
            }
        }
    }

    /**
     * sqlType에 해당하는 기본 precision 값을 구한다.</br> VARCHAR타입인 경우 value를 이용해 precision을 계산해야 하기때문에
     * argument로 값을 전달받는다.</br> deferred인 경우에만 해당 메소드를 이용한다.
     * @param aTargetSqlType
     * @param aValue
     * @return
     */
    protected int getDefaultPrecisionForDeferred(int aTargetSqlType, Object aValue)
    {
        int sPrecision = 0;
        
        switch (aTargetSqlType)
        {
            case Types.CLOB:
            case Types.BLOB:
            case Types.BIGINT:
                sPrecision = 19;
                break;
            case Types.BOOLEAN:
                sPrecision = 1;
                break;
            case Types.VARBINARY:
            case AltibaseTypes.BINARY:
                sPrecision = 32000;
                break;
            case Types.DECIMAL:
            case Types.FLOAT:
            case Types.NUMERIC:
                sPrecision = 38;
                break;
            case Types.CHAR:
            case Types.LONGVARCHAR:
                sPrecision = 65534;
                break;
            case Types.VARCHAR:
                if (aValue instanceof Boolean)
                {
                    sPrecision = 5;
                }
                else if (aValue instanceof String)
                {
                    int sBytesOfString = ((String)aValue).length();
                    sBytesOfString *= mConnection.channel().getByteLengthPerChar();
                    sPrecision = sBytesOfString;
                    sPrecision = Math.min(sPrecision, 65534);
                    sPrecision = Math.max(sPrecision, 254);  // BUG-48431 nibble 컬럼을 위해 최소값을 254로 조정
                }
                else
                {
                    sPrecision = 65534;
                }
                break;
            case AltibaseTypes.NCHAR:
            case AltibaseTypes.NVARCHAR:
                sPrecision = 32766;
                break;
            case Types.SMALLINT:
                sPrecision = 5;
                break;
            case Types.DATE:
            case Types.TIME:
            case Types.TIMESTAMP:
                sPrecision = 30;
                break;
            case Types.DOUBLE:
                sPrecision = 15;
                break;
            case Types.REAL:
                sPrecision = 7;
                break;
            case AltibaseTypes.OTHER:
            case Types.INTEGER:
                sPrecision = 10;
                break;
            case Types.NULL:
                sPrecision = 4;
                break;
            case Types.BIT:
                sPrecision = 64 * 1024;
                break;
        }
        
        return sPrecision;
    }

    private void setObjectInternal(Object aValue, Column sColumn) throws SQLException
    {
        if(sColumn.getDBColumnType() == ColumnTypes.BLOB_LOCATOR)
        {
            if (mLobUpdator == null)
            {
                mLobUpdator = new LobUpdator(this);
            }
            
            mLobUpdator.addBlobColumn((BlobLocatorColumn)sColumn, aValue);
        }
        else if(sColumn.getDBColumnType() == ColumnTypes.CLOB_LOCATOR)
        {
            if (mLobUpdator == null)
            {
                mLobUpdator = new LobUpdator(this);
            }
            
            mLobUpdator.addClobColumn((ClobLocatorColumn)sColumn, aValue);
        }
        else
        {
            sColumn.setValue(aValue);
        }
    }

    public void setObject(int aParameterIndex, Object aValue) throws SQLException
    {
        Object sObject = convertJava8TimeToSqlTime(aValue);
        if (sObject instanceof String)
        {
            setString(aParameterIndex, (String)sObject);
        }
        else if (sObject instanceof BigDecimal)
        {
            setBigDecimal(aParameterIndex, (BigDecimal)sObject);
        }
        else if (sObject instanceof Boolean)
        {
            setBoolean(aParameterIndex, (Boolean)sObject);
        }
        else if (sObject instanceof Integer)
        {
            setInt(aParameterIndex, (Integer)sObject);
        }
        else if (sObject instanceof Short)
        {
            setShort(aParameterIndex, (Short)sObject);
        }
        else if (sObject instanceof Long)
        {
            setLong(aParameterIndex, (Long)sObject);
        }
        else if (sObject instanceof Float)
        {
            setFloat(aParameterIndex, (Float)sObject);
        }
        else if (sObject instanceof Double)
        {
            setDouble(aParameterIndex, (Double)sObject);
        }
        else if (sObject instanceof byte[])
        {
            setBytes(aParameterIndex, (byte[])sObject);
        }
        else if (sObject instanceof Date)
        {
            setDate(aParameterIndex, (Date)sObject);
        }
        else if (sObject instanceof Time)
        {
            setTime(aParameterIndex, (Time)sObject);
        }
        else if (sObject instanceof Timestamp)
        {
            setTimestamp(aParameterIndex, (Timestamp)sObject);
        }
        else if (sObject instanceof Blob)
        {
            setBlob(aParameterIndex, (Blob)sObject);
        }
        else if (sObject instanceof Clob)
        {
            setClob(aParameterIndex, (Clob)sObject);
        }
        else if (sObject instanceof InputStream)
        {
            setBinaryStream(aParameterIndex, (InputStream)sObject);
        }
        else if (sObject instanceof Reader)
        {
            setCharacterStream(aParameterIndex, (Reader)sObject);
        }
        else if (sObject instanceof BitSet)
        {
            getColumnForInType(aParameterIndex, Types.BIT).setValue(sObject);
        }
        else if (sObject instanceof char[])
        {
            setString(aParameterIndex, String.valueOf((char[])sObject));
        }
        else if (sObject == null) {
            setNull(aParameterIndex, Types.NULL);
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_FEATURE, aValue.getClass().getName());
        }
    }

    public void setRef(int i, Ref aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Ref type");
    }

    public void setShort(int aParameterIndex, short aValue) throws SQLException
    {
        ((SmallIntColumn)getColumnForInType(aParameterIndex, Types.SMALLINT)).setTypedValue(aValue);
    }

    public void setString(int aParameterIndex, String aValue) throws SQLException
    {
        // BUG-42424 precision 값 계산을 위해 value를 같이 넘겨준다.
        getColumnForInType(aParameterIndex, Types.VARCHAR, aValue).setValue(aValue);
    }

    public void setTime(int aParameterIndex, Time aValue, Calendar aCalendar) throws SQLException
    {
        TimeColumn sColumn = (TimeColumn)getColumnForInType(aParameterIndex, Types.TIME);
        sColumn.setLocalCalendar(aCalendar);
        sColumn.setValue(aValue);
    }

    public void setTime(int aParameterIndex, Time aValue) throws SQLException
    {
        getColumnForInType(aParameterIndex, Types.TIME).setValue(aValue);
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue, Calendar aCalendar) throws SQLException
    {
        TimestampColumn sColumn = (TimestampColumn)getColumnForInType(aParameterIndex, Types.TIMESTAMP);
        sColumn.setLocalCalendar(aCalendar);
        sColumn.setValue(aValue);
    }

    public void setTimestamp(int aParameterIndex, Timestamp aValue) throws SQLException
    {
        getColumnForInType(aParameterIndex, Types.TIMESTAMP).setValue(aValue);
    }

    public void setURL(int aParameterIndex, URL aValue) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("URL type");
    }

    @SuppressWarnings("deprecation")
    public void setUnicodeStream(int aParameterIndex, InputStream aValue, int aLength) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("Deprecated: setUnicodeStream");
    }

    public void addBatch(String aSql) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "addBatch(String sql)");
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

    public ResultSet executeQuery(String aSql) throws SQLException
    {
        Error.throwSQLException(ErrorDef.CANNOT_USE_PREPARED_STATEMENT_METHOD_WITH_SQL, "executeQuery(String sql)");
        return null;
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
    
    private void checkBindColumnLength(int aIndex) throws SQLException
    {
        if (mIsDeferred) return;    // BUG-42424 deferred상태일때는 바인드컬럼길이를 알지못하기 때문에 무시한다.
        
        if (mBindColumns.size() == 0 && aIndex > 0)
        {
            Error.throwSQLException(ErrorDef.INVALID_BIND_COLUMN);
        }
        if (aIndex <= 0 || aIndex > mBindColumns.size())
        {
            Error.throwSQLException(ErrorDef.INVALID_COLUMN_INDEX, "1 ~ " + mBindColumns.size(), 
                                    String.valueOf(aIndex));
        }
    }
    
    private void throwErrorForBatchJob(String aCommand) throws SQLException
    {
        if (mBatchAdded)
        {
            Error.throwSQLException(ErrorDef.SOME_BATCH_JOB, aCommand); 
        }
    }
    
    // PROJ-2583 jdbc logging
    public int getStmtId()
    {
        return mPrepareResult.getStatementId();
    }

    /**
     * Trace logging 시 statement 간 식별을 위해 unique id 를 반환한다.
     */
    String getTraceUniqueId()
    {
        return "[StmtId #" + getStmtId() + "] ";
    }

    @Override
    public String toString()
    {
        return "AltibasePreparedStatement{" + "mStmtCID=" + mStmtCID
               + ", mStmtID=" + getStmtId()
               + ", mQstr='" + mQstr + '\''
               + '}';
    }

    public LobUpdator getLobUpdator()
    {
        return mLobUpdator;
    }

    @Override
    public long executeLargeUpdate() throws SQLException
    {
        execute();
        return mExecuteResultMgr.getFirst().mUpdatedCount;
    }

    @Override
    public void setNString(int aParameterIndex, String aValue) throws SQLException
    {
        // PROJ-2707 NString인 경우 Types.NVARCHAR로 매핑한다.
        getColumnForInType(aParameterIndex, Types.NVARCHAR, aValue).setValue(aValue);
    }

    @Override
    public void setClob(int aParameterIndex, Reader aReader) throws SQLException
    {
        setCharacterStream(aParameterIndex, aReader);
    }

    @Override
    public void setClob(int aParameterIndex, Reader aReader, long aLength) throws SQLException
    {
        setCharacterStream(aParameterIndex, aReader, aLength);
    }

    @Override
    public void setBlob(int aParameterIndex, InputStream aInputStream) throws SQLException
    {
        setBinaryStream(aParameterIndex, aInputStream);
    }

    @Override
    public void setBlob(int aParameterIndex, InputStream aInputStream, long aLength) throws SQLException
    {
        setBinaryStream(aParameterIndex, aInputStream, aLength);
    }

    @Override
    public void setObject(int aParameterIndex, Object aValue, SQLType aTargetSqlType) throws SQLException
    {
        setObject(aParameterIndex, aValue, aTargetSqlType.getVendorTypeNumber());
    }

    @Override
    public void setObject(int aParameterIndex, Object aValue, SQLType aTargetSqlType,
                          int aScaleOrLength) throws SQLException
    {
        setObject(aParameterIndex, aValue, aTargetSqlType.getVendorTypeNumber(), aScaleOrLength);
    }

    /*
        BUG-48892 Java8의 Time API를 지원하기 위해 추가
     */
    private Object convertJava8TimeToSqlTime(Object aValue)
    {
        if (aValue instanceof LocalDate)
        {
            return Date.valueOf((LocalDate)aValue);
        }
        else if (aValue instanceof LocalDateTime)
        {
            return Timestamp.valueOf((LocalDateTime)aValue);
        }
        else if (aValue instanceof LocalTime)
        {
            return Time.valueOf((LocalTime)aValue);
        }

        return aValue;
    }
}
