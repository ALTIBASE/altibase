/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.worker.task;

import java.sql.*;

import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.jdbc.JdbcDataManager;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.RepeatableTask;
import com.altibase.altilinker.worker.Task;

public class FetchResultSetTask extends RepeatableTask
{
    protected long mRemoteStatementId = 0;
    protected int  mFetchRowCount     = 0;
    
    private boolean mFirstRun = true;
    private Fetcher mFetcher  = new Fetcher();
    protected int mBufferSize    = 0;
    protected final int mFetchSizeThreshold = 500 * 1024;    // 500 kbyte

    public FetchResultSetTask()
    {
        super(TaskType.FetchResultSet);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        FetchResultSetOp sRequestOperation = (FetchResultSetOp)aOperation;

        mRemoteStatementId = sRequestOperation.mRemoteStatementId;
        mFetchRowCount     = sRequestOperation.mFetchRowCount;
        mBufferSize        = sRequestOperation.mBufferSize;
    }

    public Task newAbortTask()
    {
        AbortRemoteStatementTask sNewAbortTask = new AbortRemoteStatementTask();
        
        sNewAbortTask.setMainMediator(getMainMediator());
        sNewAbortTask.setSessionManager(getSessionManager());

        sNewAbortTask.setRequestOperationId(getRequestOperationId());
        sNewAbortTask.setLinkerSessionId(getLinkerSessionId());
        sNewAbortTask.mRemoteStatementId = mRemoteStatementId;

        return sNewAbortTask;
    }

    private int setFetchSize( Statement aStatement,
                              int       aFetchSize )
    {
        int sSuccessFetchSize = 0;
        try 
        {
            aStatement.setFetchSize( aFetchSize );
            sSuccessFetchSize = aFetchSize;
        }
        catch ( Exception e )
        {
            /* This is for XDB JDBC Driver */
            if ( aFetchSize > Short.MAX_VALUE - 1 )
            {
                sSuccessFetchSize = setFetchSize( aStatement, Short.MAX_VALUE - 1 );
            }
            else if ( aFetchSize == 0 )
            {
                /* do nothing */
                sSuccessFetchSize = -1;
            }
            else
            {
                sSuccessFetchSize = setFetchSize( aStatement, 0 );
            }
        }
        return sSuccessFetchSize;
    }

    private int autoTunedSetFetchSize( Statement aStatement,
                                       ResultSetMetaData  aMetaData )
    {
        int sTotalColSize = 0;
        // BLOB, CLOB check 
        boolean sChkBlobClobType = false;
        int sColumnType = 0;
        int sJdbcSetFetchSize = 0;

        try
        {
            if ( ( mBufferSize > 0 ) && ( aMetaData != null ) )
            {
                for( int columnIdx = 1; columnIdx <= aMetaData.getColumnCount(); columnIdx++ )
                {
                    sColumnType = aMetaData.getColumnType( columnIdx );
                    
                    if ( ( sColumnType == Types.BLOB ) || ( sColumnType == Types.CLOB ) )
                    {
                        sChkBlobClobType = true;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    
                    sTotalColSize += aMetaData.getColumnDisplaySize( columnIdx );
                }
                
                // Jdbc Set Fetch Size
                if ( ( sTotalColSize > 0 ) && ( sChkBlobClobType == false ) ) 
                {
                    // set fetch size depending on the scheme size (500 Kb)
                    if ( sTotalColSize < mFetchSizeThreshold )
                    {
                        // If the size is less than the low 500kbyte fetch count is set to half.
                        sJdbcSetFetchSize = ( mBufferSize / sTotalColSize ) / 2;
                    }
                    else
                    {
                        sJdbcSetFetchSize = mBufferSize / sTotalColSize; 
                    }
                    sJdbcSetFetchSize = setFetchSize( aStatement, sJdbcSetFetchSize );
                }
                else
                {
                    // Default fetch size = 10
                    sJdbcSetFetchSize = -1;
                }
            }
        }
        catch ( Exception e )
        {
            /*do nothing*/
        }
        
        return sJdbcSetFetchSize;
    }


    public void run()
    {
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;
        boolean        sFirstRun        = mFirstRun;
        int            sFetchSize       = 0;
 
        if (mFirstRun == true)
        {
            mFirstRun = false;
        }
        
        // check for connecting linker data session
        LinkerDataSession sLinkerDataSession =
                sSessionManager.getLinkerDataSession(sLinkerSessionId);
        
        if (sLinkerDataSession == null)
        {
            mTraceLogger.logError(
                    "Unknown linker data session. (Linker Data Session Id = {0})",
                    new Object[] { new Integer(sLinkerSessionId) } );

            sResultCode = ResultCode.UnknownSession;
            
            setCompleted();
        }
        else
        {
            RemoteStatement sRemoteStatement =
                    sLinkerDataSession.getRemoteStatement(mRemoteStatementId);
            
            if (sRemoteStatement == null)
            {
                mTraceLogger.logError(
                        "Unknown remote statement. (Remote Statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) });
                
                sResultCode = ResultCode.UnknownRemoteStatement;
                
                setCompleted();
            }
            else if (sRemoteStatement.isExecuting() == true)
            {
                mTraceLogger.logError(
                        "Remote statement was busy. (Remote Statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) } );

                sResultCode = ResultCode.Busy;
                
                setCompleted();
            }
            else
            {
                RemoteNodeSession sRemoteNodeSession = 
                        sLinkerDataSession.getRemoteNodeSession(
                                sRemoteStatement.getRemoteNodeSessionId());
                
                Statement         sStatement         = sRemoteStatement.getJdbcStatementObject();
                ResultSet         sResultSet         = sRemoteStatement.getJdbcResultSetObject();
                ResultSetMetaData sResultSetMetaData = sRemoteStatement.getJdbcResultSetMetaDataObject();
                PreparedStatement sPreparedStatement = sRemoteStatement.getPreparedStatementObject();
                
                if (sResultSet == null)
                {
                    // set last remote statement
                    sRemoteNodeSession.setLastRemoteStatementId(mRemoteStatementId);

                    // set executing state
                    sRemoteStatement.executing(RemoteStatement.TaskType.Fetch);

                    // get meta data of result set
                    try
                    {
                        if( sPreparedStatement != null )
                        {
                            sResultSetMetaData = sPreparedStatement.getMetaData();
                            sFetchSize = autoTunedSetFetchSize( sStatement, sResultSetMetaData ); 
                        }
                        else
                        {
                            /*do nothing*/
                        }
                        sResultSet = sStatement.getResultSet();
                        
                        sRemoteStatement.setJdbcResultSetObject(sResultSet);
                        sRemoteStatement.setJdbcResultSetMetaDataObject(sResultSetMetaData);

                        mTraceLogger.logTrace( " Remote Statement: " + sRemoteStatement.getStatementId() + "," + 
                                               " Query: " + sRemoteStatement.getStatementString() + ", " +
                                               " setFetchSize: " + sFetchSize );
                    }
                    catch (SQLException e)
                    {
                        // set executed state with remote server error
                        sRemoteStatement.executed(new RemoteServerError(e));
                        
                        mTraceLogger.log(e);
                        
                        if (sRemoteStatement.isRemoteServerConnectionError() == true)
                        {
                            sResultCode = ResultCode.RemoteServerConnectionFail;
                        }
                        else
                        {
                            sResultCode = ResultCode.RemoteServerError;
                        }

                        setCompleted();
                    }
                }

                if (sResultSet != null)
                {
                    if (sFirstRun == true)
                    {
                        try
                        {
                            mFetcher.setRemoteStatement(sRemoteStatement);
                            mFetcher.setRequestRecordCount(mFetchRowCount);
                            mFetcher.setResultSet(sResultSet, sResultSetMetaData);
                        }
                        catch (SQLException e)
                        {
                            // set executed state with remote server error
                            sRemoteStatement.executed(new RemoteServerError(e));
                            
                            mTraceLogger.log(e);
                            
                            if (sRemoteStatement.isRemoteServerConnectionError() == true)
                            {
                                sResultCode = ResultCode.RemoteServerConnectionFail;
                            }
                            else
                            {
                                sResultCode = ResultCode.RemoteServerError;
                            }

                            setCompleted();
                        }
                    }

                    if (sResultCode == ResultCode.Success)
                    {
                        fetchResultRow(sMainMediator, sRemoteNodeSession, sRemoteStatement, sLinkerDataSession);
                    }
                }
            }            
        }

        if (sResultCode != ResultCode.Success)
        {
            FetchResultRowOp sFetchResultRowOp = allocateFetchResultRowOp(false);

            if (sFetchResultRowOp == null)
            {
                mTraceLogger.logFatal(
                        "Task could not allocate new result operation for {0} operation.",
                        new Object[] { new Integer(getRequestOperationId()) } );
    
                System.exit(-1);
                
                return;
            }
            
            sendFetchResultRowOp(sFetchResultRowOp,
                    sResultCode,
                    sMainMediator);
        }
    }
    
    private void fetchResultRow(MainMediator      aMainMediator,
                                RemoteNodeSession aRemoteNodeSession,
                                RemoteStatement   aRemoteStatement,
                                LinkerDataSession aLinkerDataSession)
    {
        byte             sResultCode           = ResultCode.Success;
        FetchResultRowOp sFetchResultRowOp     = allocateFetchResultRowOp(true);
        FetchResultRowOp sNextFetchResultRowOp = null;

        try
        {
            do
            {
                sResultCode = mFetcher.fetch(sFetchResultRowOp);

                if ((sResultCode == ResultCode.Fragmented) &&
                        (sFetchResultRowOp.mRecordCount >= 1))
                {
                    if (sFetchResultRowOp.isRecordWriteCompleted() == false)
                    {
                        sNextFetchResultRowOp = allocateFetchResultRowOp(true);
                       
                        sFetchResultRowOp.moveLastRecordTo(sNextFetchResultRowOp);
                       
                        sResultCode = ResultCode.Success;
                    }
                    else  
                    {
                    	// record write completed is TRUE and buffer remaining less than 4 byte
                    	if (sFetchResultRowOp.mRowDataBuffer.remaining() < 4)
                    	{
                    	    sResultCode = ResultCode.Success;
                    	}
                   
                    }
                }
                
                if (sResultCode == ResultCode.Fragmented)
                {
                    sendFetchResultRowOp(sFetchResultRowOp,
                                         sResultCode,
                                         aMainMediator);
                }
                else if (sResultCode == ResultCode.EndOfFetch)
                {
                    aRemoteStatement.executed();
                    
                    sFetchResultRowOp.freeRowDataBuffer();

                    sendFetchResultRowOp(sFetchResultRowOp,
                                         ResultCode.EndOfFetch,
                                         aMainMediator);
                    
                    setCompleted();
                    
                    break;
                }
                else if (sResultCode == ResultCode.Failed)
                {
                    aRemoteStatement.executed();
                    
                    sFetchResultRowOp.freeRowDataBuffer();
                    
                    sendFetchResultRowOp(sFetchResultRowOp,
                                         sResultCode,
                                         aMainMediator);
                    
                    setCompleted();
                    
                    break;
                }
                else
                {
                    sendFetchResultRowOp(sFetchResultRowOp,
                                         sResultCode,
                                         aMainMediator);

                    if (mFetcher.needMoreFetch() == false)
                    {
                        aRemoteStatement.executed();

                        setCompleted();
                        
                        break;
                    }
                }

                //sFetchResultRowOp.close();
                sFetchResultRowOp = null;
                
                if (sNextFetchResultRowOp != null)
                {
                    sFetchResultRowOp = sNextFetchResultRowOp;
                    sNextFetchResultRowOp = null;
                }
                else
                {
                    sFetchResultRowOp = allocateFetchResultRowOp(true);
                }
                
            } while (true);
        }
        catch (SQLException e)
        {
            // set executed state with remote server error
            aRemoteStatement.executed(new RemoteServerError(e));

            mTraceLogger.log(e);

            if (aRemoteStatement.isRemoteServerConnectionError() == true)
            {
                sResultCode = ResultCode.RemoteServerConnectionFail;
                if ( isXA() == true )
                {
                    short sRemoteNodeSessionId = (short)aRemoteNodeSession.getSessionId(); 
                    
                    aLinkerDataSession.closeRemoteNodeSession(sRemoteNodeSessionId);
                    
                    aRemoteNodeSession   = null;
                    sRemoteNodeSessionId = 0;    
                }
            }
            else
            {
                sResultCode = ResultCode.RemoteServerError;
            }
            
            sFetchResultRowOp.freeRowDataBuffer();
            
            sendFetchResultRowOp(sFetchResultRowOp,
                                 sResultCode,
                                 aMainMediator);
        }
    }
    
    private static class Fetcher
    {
        private RemoteStatement   mRemoteStatement    = null;
        private int               mRequestRecordCount = 0;
        private long              mFetchedRecordCount = 0;
        private boolean           mEndOfFetch         = false;
        
        private ResultSet         mResultSet          = null;
        private ResultSetMetaData mResultSetMetaData  = null;
        private int               mColumnCount        = 0;
        private ColumnType[]      mColumnTypes        = null;
        private int               mColumnIndex        = 1;
        private ColumnData        mColumnData         = null;
        
        private static class ColumnType
        {
            private int mJdbcColumnType = java.sql.Types.NULL;
            private int mSQLType        = AltibaseSQLType.SQL_NONE;
        }
        
        public void setRemoteStatement(RemoteStatement aRemoteStatement)
        {
            mRemoteStatement = aRemoteStatement;
        }
        
        public void setResultSet(ResultSet         aResultSet,
                                 ResultSetMetaData aResultSetMetaData)
                                                             throws SQLException
        {
            mResultSet         = aResultSet;
            mResultSetMetaData = aResultSetMetaData;
            
            mColumnCount = aResultSetMetaData.getColumnCount();
            if (mColumnCount > 0)
            {
                mColumnTypes = new ColumnType[mColumnCount];
    
                int i;
                int sJdbcColumnType;
                int sSQLType;
                
                for (i = 1; i <= mColumnCount; ++i)
                {
                    sJdbcColumnType = mResultSetMetaData.getColumnType(i);
                    sSQLType        = JdbcDataManager.toSQLType(sJdbcColumnType);
                    
                    mColumnTypes[i - 1] = new ColumnType();
                    mColumnTypes[i - 1].mJdbcColumnType = sJdbcColumnType;
                    mColumnTypes[i - 1].mSQLType        = sSQLType;
                }
            }
        }
        
        public void setRequestRecordCount(int aRequestRecordCount)
        {
            mRequestRecordCount = aRequestRecordCount;
        }
        
        public byte fetch(FetchResultRowOp aFetchResultRowOp) throws SQLException
        {
            if (mEndOfFetch == true)
            {
                return ResultCode.EndOfFetch;
            }
            
            byte sResultCode = ResultCode.Success;

            if (aFetchResultRowOp.isSetRecordBegin() == false)
            {
                // write record count (4bytes)
                aFetchResultRowOp.writeRecordCount();
    
                // memorize last record begin position
                aFetchResultRowOp.setRecordBegin();
                
                aFetchResultRowOp.resetColumnCount();
            }

            // write column count (4bytes)
            aFetchResultRowOp.writeColumnCount();
            
            fetch: do
            {
                int sJdbcColumnType;
                int sSQLType;

                // record begin
                if (mColumnIndex == 1 && mColumnData == null)
                {
                    if (needMoreFetch() == false)
                    {
                        sResultCode = ResultCode.Success;
                        
                        break;
                    }

                    // next record
                    if (mResultSet.next() == false)
                    {
                        if (aFetchResultRowOp.getRecordCount() > 0)
                        {
                            sResultCode = ResultCode.Success;
                        }
                        else
                        {
                            mEndOfFetch = true;
                            
                            sResultCode = ResultCode.EndOfFetch;
                        }
                        
                        break;
                    }
                }

                for (; mColumnIndex <= mColumnCount; ++mColumnIndex)
                {
                    if (mColumnData == null)
                    {
                        sJdbcColumnType = mColumnTypes[mColumnIndex - 1].mJdbcColumnType;
                        sSQLType        = mColumnTypes[mColumnIndex - 1].mSQLType;

                        mColumnData = ColumnDataFactory.newColumnData(sSQLType);
                        if (mColumnData == null)
                        {
                            mTraceLogger.logError(
                                    "Unknown column data type. (Column Type = {0})",
                                    new Object[] { new Integer(sJdbcColumnType) } );
    
                            sResultCode = ResultCode.Failed;
                            
                            break fetch;
                        }
                        
                        mColumnData.setResultSet(mResultSet, mColumnIndex);
                    }

                    // write column value length (4bytes) and column value (v)
                    sResultCode = mColumnData.write(aFetchResultRowOp.mRowDataBuffer);
                    
                    if (sResultCode == ResultCode.Success)
                    {
                        // write column count (4bytes)
                        aFetchResultRowOp.increaseColumnCount();
                        aFetchResultRowOp.writeColumnCount();
                        
                        mColumnData = null;
                    }
                    else
                    {
                        // Result code may be ResultCode.Fragemented or ResultCode.Failed. 
                        break fetch;
                    }
                }

                // write record count (4bytes)
                aFetchResultRowOp.increaseRecordCount();
                aFetchResultRowOp.writeRecordCount();
                
                mColumnIndex = 1;
                
                increaseFetchedRecord();

                // memorize last record begin position
                aFetchResultRowOp.setRecordBegin();

                // if these lack of buffer, result code is ResultCode.Fragemented.
                if (aFetchResultRowOp.mRowDataBuffer.remaining() < 4)
                {
                    sResultCode = ResultCode.Fragmented;
                    break;
                }

                // write column count (4bytes)
                aFetchResultRowOp.resetColumnCount();
                aFetchResultRowOp.writeColumnCount();
                
            } while (true);
        
            return sResultCode;
        }

        private long increaseFetchedRecord()
        {
            ++mFetchedRecordCount;
            
            if (mRemoteStatement != null)
            {
                mRemoteStatement.increaseFetchedRecord();
            }
            
            return mFetchedRecordCount;
        }

        private boolean needMoreFetch()
        {
            if (mEndOfFetch == true)
            {
                return false;
            }
            
            if (mRequestRecordCount == -1) // unlimited, if -1
            {
                return true;
            }
            
            if (mFetchedRecordCount >= mRequestRecordCount)
            {
                return false;
            }
            
            return true;
        }
    }
    
    private FetchResultRowOp allocateFetchResultRowOp(boolean aAllocRowDataBuffer)
    {
        FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)newResultOperation();
        
        if (sFetchResultRowOp == null)
        {
            mTraceLogger.logFatal(
                    "Task could not allocate new result operation for {0} operation.",
                    new Object[] { new Integer(getRequestOperationId()) } );

            System.exit(-1);
            
            return null;
        }

        if (aAllocRowDataBuffer == true)
        {
            // allocate fetch row data from buffer pool
            sFetchResultRowOp.allocateRowDataBuffer();
        }

        return sFetchResultRowOp;
    }
    
    private void sendFetchResultRowOp(FetchResultRowOp aFetchResultRowOp,
                                      byte             aResultCode,
                                      MainMediator     aMainMediator)
    {        
        // set common header of result operation
        aFetchResultRowOp.setSessionId(getLinkerSessionId());
        aFetchResultRowOp.setResultCode(aResultCode);
        aFetchResultRowOp.setSeqNo( this.getSeqNo() );
        aFetchResultRowOp.setXA( this.isXA() );

        // set payload contents of result operation
        aFetchResultRowOp.mRemoteStatementId = mRemoteStatementId;
        
        if (aFetchResultRowOp.mRowDataBuffer != null)
        {
            int sRowDataSize = aFetchResultRowOp.mRowDataBuffer.position();
            
            aFetchResultRowOp.mRowDataBuffer.limit(sRowDataSize);
            aFetchResultRowOp.mRowDataBuffer.position(0);
        }

        // send result operation
        aMainMediator.onSendOperation(aFetchResultRowOp);

        //aFetchResultRowOp.close();
    }
}
