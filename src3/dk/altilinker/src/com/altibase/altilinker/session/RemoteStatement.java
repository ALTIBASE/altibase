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
 
package com.altibase.altilinker.session;

import java.sql.SQLException;

import com.altibase.altilinker.util.*;

/**
 * Remote statement class to manage java.sql.Statement object
 */
public class RemoteStatement
{
    /**
     * Statement state types
     */
    public interface StatementState
    {
        public static final int None      = 0;
        public static final int Created   = 1;
        public static final int Executing = 2;
        public static final int Executed  = 3;
        public static final int Closing   = 4;
        public static final int Closed    = 5;
    }
    
    /**
     * Statement type types
     */
    public interface StatementType
    {
        public static final int None                   = 0;
        public static final int RemoteTable            = 1;
        public static final int RemoteExecuteImmediate = 2;
        public static final int ExecuteStatement       = 3;
        public static final int BatchStatement         = 4;
    }
    
    /**
     * Task type types
     */
    public interface TaskType
    {
        public static final int None    = 0;
        public static final int Prepare = 1;
        public static final int Bind    = 2;
        public static final int Execute = 3;
        public static final int Fetch   = 4;
        public static final int Abort   = 5;
    }
    
    private static TraceLogger mTraceLogger = TraceLogger.getInstance();
    
    private int        mRemoteNodeSessionId     = 0;
    private long       mStatementId             = 0;
    private int        mStatementState          = StatementState.None;
    private int        mStatementType           = StatementType.None;
    private String     mStatementString         = null;
    private int        mTaskType                = TaskType.None;
    private TaskResult mTaskResult              = null;
    private long       mTotalFetchedRecordCount = 0;

    // for batch statement
    private int        mTotalAddBatchCount      = 0;
    private int        mProcessedAddBatchCount  = 0;
    
    private java.sql.Statement         mJdbcStatement         = null;
    private java.sql.PreparedStatement mPreparedStatement     = null;
    private java.sql.ResultSet         mJdbcResultSet         = null;
    private java.sql.ResultSetMetaData mJdbcResultSetMetaData = null;

    /**
     * Constructor
     * 
     * @param aStatementId              Remote statement ID
     * @param aStatementString          Statement string
     * @param aRemoteNodeSessionId      Remote node session ID
     */
    protected RemoteStatement(long   aStatementId,
                              String aStatementString,
                              int    aRemoteNodeSessionId)
    {
        mStatementId         = aStatementId;
        mStatementString     = aStatementString;
        mRemoteNodeSessionId = aRemoteNodeSessionId;
    }
    
    /**
     * Close this statement
     */
    protected void close()
    {
        /*
         * BUG-39001
         * Statement should be cancel. 
         * It is prevented from block when jdbc connection is closed.
         */
        if ( mStatementState == StatementState.Executing )
        {
            if ( mJdbcStatement != null )
            {
                try
                {
                   mJdbcStatement.cancel();
                }
                catch ( SQLException e )
                {
                    mTraceLogger.log( e );
                }                
            }
            else
            {
                /* do nothing */ 
            }
        }
        else
        {
             /* do nothing */ 
        }       
        
        mStatementState = StatementState.Closing;
        
        if (mJdbcStatement != null)
        {
            try
            {
                mJdbcStatement.close();
            }
            catch (SQLException e)
            {
                mTraceLogger.log(e);
            }
            
            mJdbcStatement = null;
        }
        
        if (mJdbcResultSet != null)
        {
            try
            {
                mJdbcResultSet.close();
            }
            catch (SQLException e)
            {
                mTraceLogger.log(e);
            }
            
            mJdbcResultSet = null;
        }

        if (mJdbcResultSetMetaData != null)
        {
            mJdbcResultSetMetaData = null;
        }
        
        mStatementState = StatementState.Closed;
    }
    
    /**
     * Get remote node session ID
     * 
     * @return  Remote node session ID
     */
    public int getRemoteNodeSessionId()
    {
        return mRemoteNodeSessionId;
    }
    
    /**
     * Get remote statement ID
     * 
     * @return  Remote statement ID
     */
    public long getStatementId()
    {
        return mStatementId;
    }
    
    /**
     * Get remote statement state
     * 
     * @return  Remote statement state
     */
    public int getStatementState()
    {
        return mStatementState;
    }
    
    /**
     * Whether java.sql.Statement is executing or not
     * 
     * @return  true, if java.sql.Statement is executing. false, if not.
     */
    public boolean isExecuting()
    {
        if (mStatementState != StatementState.Executing)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Get statement type
     * 
     * @return  Statement type
     */
    public int getStatementType()
    {
        return mStatementType;
    }
    
    /**
     * Get statement string
     * 
     * @return  Statement string
     */
    public String getStatementString()
    {
        return mStatementString;
    }
    
    /**
     * Get java.sql.Statement object
     * 
     * @return  java.sql.Statement object
     */
    public java.sql.Statement getJdbcStatementObject()
    {
        return mJdbcStatement;
    }
    
    public java.sql.PreparedStatement getPreparedStatementObject()
    {
        return mPreparedStatement;
    }
    /**
     * Get java.sql.ResultSet object
     * 
     * @return  java.sql.ResultSet object
     */
    public java.sql.ResultSet getJdbcResultSetObject()
    {
        return mJdbcResultSet;
    }

    /**
     * Get java.sql.ResultSetMetaData object
     * 
     * @return  java.sql.ResultSetMetaData object
     */
    public java.sql.ResultSetMetaData getJdbcResultSetMetaDataObject()
    {
        return mJdbcResultSetMetaData;
    }
    
    /**
     * Get task type
     * 
     * @return  Task type
     */
    public int getTask()
    {
        return mTaskType;
    }
    
    /**
     * Get task result
     * 
     * @return  TaskResult object
     */
    public TaskResult getTaskResult()
    {
        return mTaskResult;
    }

    /**
     * Get remote server error when JDBC interface cause remote server error.
     * 
     * @return  RemoteServerError object
     */
    public RemoteServerError getRemoteServerError()
    {
        if (mTaskResult == null)
            return null;
        
        return mTaskResult.getRemoteServerError();
    }
    
    /**
     * Whether JDBC interface cause remote server error or not
     * 
     * @return  true, if JDBC interface cause remote server error. false, if not.
     */
    public boolean isRemoteServerError()
    {
        RemoteServerError sRemoteServerError = getRemoteServerError();

        return (sRemoteServerError != null) ? true : false;
    }
    
    /**
     * Whether JDBC interface cause remote server connection or not.
     * 
     * @return  true, if JDBC interface cause remote server connection. false, if not.
     */
    public boolean isRemoteServerConnectionError()
    {
        RemoteServerError sRemoteServerError = getRemoteServerError();
        if (sRemoteServerError == null)
            return false;
        
        return sRemoteServerError.isConnectionError();
    }
    
    /**
     * Get total fetched record count
     *  
     * @return  Total fetched record count
     */
    public long getFetchedRecordCount()
    {
        return mTotalFetchedRecordCount;
    }
    
    /**
     * Set statement state
     * 
     * @param aStatementState   Statement state
     */
    public void setStatementState(int aStatementState)
    {
        mStatementState = aStatementState;
    }
    
    /**
     * Set statement type
     * 
     * @param aStatementType    Statement type
     */
    public void setStatementType(int aStatementType)
    {
        mStatementType = aStatementType;
    }

    /**
     * Set task type which JDBC java.sql.Statement interface is executing now.
     *  
     * @param aTaskType         Task type
     */
    public void executing(int aTaskType)
    {
        mTaskType   = aTaskType;
        mTaskResult = null;
        
        mStatementState = StatementState.Executing;
    }

    /**
     * Set success task result after JDBC java.sql.Statement interface is executed successfully
     */
    public void executed()
    {
        executed(TaskResult.Success);
    }
    
    /**
     * Set task result after JDBC java.sql.Statement interface is executed
     * 
     * @param aTaskResult       Task result value
     */
    public void executed(int aTaskResult)
    {
        executed(new TaskResult(aTaskResult));
    }

    /**
     * Set java.sql.Exception after JDBC java.sql.Statement interface fail to execute.
     * 
     * @param aRemoteServerError        RemoteServerError object
     */
    public void executed(SQLException e)
    {
        executed(new TaskResult(new RemoteServerError(e)));
    }

    /**
     * Set remote server error task result after JDBC java.sql.Statement interface fail to execute.
     * 
     * @param aRemoteServerError        RemoteServerError object
     */
    public void executed(RemoteServerError aRemoteServerError)
    {
        executed(new TaskResult(aRemoteServerError));
    }
    
    /**
     * Set task result object after JDBC java.sql.Statement interface is executed. 
     * 
     * @param aTaskResult       TaskResult object
     */
    public void executed(TaskResult aTaskResult)
    {
        mTaskResult = aTaskResult;
        
        mStatementState = StatementState.Executed;
    }

    /**
     * Increase fetch record count
     * 
     * @return  Increased total fetch record count
     */
    public long increaseFetchedRecord()
    {
        ++mTotalFetchedRecordCount;
        
        return mTotalFetchedRecordCount;
    }
    
    /**
     * Set java.sql.Statement object
     * 
     * @param aJdbcStatement    java.sql.Statement object
     */
    public void setJdbcStatementObject(java.sql.Statement aJdbcStatement)
    {
        mJdbcStatement = aJdbcStatement;
    }
    
    public void setJdbcStatementObject(java.sql.PreparedStatement aJdbcStatement)
    {
        mJdbcStatement = aJdbcStatement;
        mPreparedStatement = aJdbcStatement;
    }
    /**
     * Set java.sql.ResultSet object
     * 
     * @param aJdbcResultSet    java.sql.ResultSet object
     */
    public void setJdbcResultSetObject(java.sql.ResultSet aJdbcResultSet)
    {
        mJdbcResultSet = aJdbcResultSet;
    }

    /**
     * Set java.sql.ResultSetMetaData object
     * 
     * @param aJdbcResultSet    java.sql.ResultSetMetaData object
     */
    public void setJdbcResultSetMetaDataObject(
                              java.sql.ResultSetMetaData aJdbcResultSetMetaData)
    {
        mJdbcResultSetMetaData = aJdbcResultSetMetaData;
    }
    
    /**
     * Set total AddBatch count
     * 
     * @param aTotalAddBatchCount total AddBatch count
     */
    public void setTotalAddBatchCount(int aTotalAddBatchCount)
    {
    	mTotalAddBatchCount = aTotalAddBatchCount;
    }
    
    /**
     * Get processed AddBatch count
     * 
     * @return processed AddBatch count
     */
    public int getProcessedAddBatchCount()
    {
    	return mProcessedAddBatchCount;
    }
    
    /**
     * Increase processed AddBatch count
     */
    public void increaseProcessedAddBatchCount()
    {
    	mProcessedAddBatchCount++;
    }
    
    /**
     * Check AddBatch is complete
     * 
     * @return if AddBatch is complete, return true. if not, return false
     */
    public boolean isCompleteAddBatch()
    {
    	return (mTotalAddBatchCount <= mProcessedAddBatchCount) ? true : false;
    }
}
