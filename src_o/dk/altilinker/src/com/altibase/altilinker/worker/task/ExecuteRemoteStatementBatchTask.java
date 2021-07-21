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

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.controller.PropertyManager;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.TraceLogger;
import com.altibase.altilinker.worker.Task;

public class ExecuteRemoteStatementBatchTask extends Task
{
    protected long   mRemoteStatementId = 0;
    protected int    mResult[] = null;
    
    private static final int mMaxRemoteNodeSessionCount =
            PropertyManager.getInstance().getRemoteNodeSessionCount();
    private static final int mNonQueryTimeout = PropertyManager.getInstance().getNonQueryTimeout();
    private static final int mQueryTimeout    = PropertyManager.getInstance().getQueryTimeout();
    
    public ExecuteRemoteStatementBatchTask()
    {
        super(TaskType.ExecuteRemoteStatementBatch);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        ExecuteRemoteStatementBatchOp sRequestOperation = (ExecuteRemoteStatementBatchOp)aOperation;

        mRemoteStatementId = sRequestOperation.mRemoteStatementId;
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
    
    public void run()
    {
        MainMediator   sMainMediator         = getMainMediator();
        SessionManager sSessionManager       = getSessionManager();
        int            sLinkerSessionId      = getLinkerSessionId();
        short          sRemoteNodeSessionId  = 0;
        byte           sResultCode           = ResultCode.Success;

        if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
        {
            mTraceLogger.logDebug(
                    "Execute to remote statement batch. (Linker Data Sesion Id = {0}, Remote Statement Id = {1})",
                    new Object[] {
                            new Integer(sLinkerSessionId),
                            new Long(mRemoteStatementId) });
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
        }
        else
        {
            RemoteStatement sRemoteStatement = sLinkerDataSession.getRemoteStatement(mRemoteStatementId);
            
            if (sRemoteStatement == null)
            {
                mTraceLogger.logError(
                        "Unknown remote statement. (Remote Statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) });
                
                sResultCode = ResultCode.UnknownRemoteStatement;
            }
            else if (sRemoteStatement.isExecuting() == true)
            {
                mTraceLogger.logError(
                        "Remote statement was busy. (Remote Statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) } );

                sResultCode = ResultCode.Busy;
            }
            else
            {
                RemoteNodeSession sRemoteNodeSession = 
                        sLinkerDataSession.getRemoteNodeSession(
                                sRemoteStatement.getRemoteNodeSessionId());
                
            	// execute remote statement
                sResultCode = 
                        executeRemoteStatementBatch(sRemoteNodeSession,
                        		                    sRemoteStatement);
            }
        }
        
        // allocate new result operation
        ExecuteRemoteStatementBatchResultOp sResultOperation =
                (ExecuteRemoteStatementBatchResultOp)newResultOperation();
        
        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                    "Task could not allocate new result operation for {0} operation.",
                    new Object[] { new Integer(getRequestOperationId()) } );
            
            System.exit(-1);
            
            return;
        }

        // set common header of result operation
        sResultOperation.setSessionId(sLinkerSessionId);
        sResultOperation.setResultCode(sResultCode);
        sResultOperation.setSeqNo( this.getSeqNo() );
        
        // set payload contents of result operation
        sResultOperation.mRemoteStatementId   = mRemoteStatementId;
        sResultOperation.mResult = mResult;

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);

        sResultOperation.close();
    }
    
    private byte executeRemoteStatementBatch(
    		RemoteNodeSession aRemoteNodeSession,
    		RemoteStatement   aRemoteStatement)
    {
    	byte sResultCode = ResultCode.Success;
    	    	
        PreparedStatement sStatement = (PreparedStatement)aRemoteStatement.getJdbcStatementObject();
       	if (sStatement == null)
    	{
            mTraceLogger.logError(
                    "JDBC Statement object was null. (Remote Statement Id = {0})",
                    new Object[] { new Long(mRemoteStatementId) } );
            
            sResultCode = ResultCode.NotPreparedRemoteStatement;

            return sResultCode;
    	}

        // set executing state
        aRemoteStatement.executing(RemoteStatement.TaskType.Execute);

        Connection sConnection = aRemoteNodeSession.getJdbcConnectionObject();

        try
        {
        	// set non-query timeout
        	sStatement.setQueryTimeout(mNonQueryTimeout);

        	/*
        	 * BUG-39001
        	 * statement should set before executing
        	 */
        	aRemoteStatement.setJdbcStatementObject( sStatement );
        	aRemoteStatement.setJdbcResultSetObject( null );
        	aRemoteStatement.setJdbcResultSetMetaDataObject( null );
        	
        	// execute
        	mResult = sStatement.executeBatch();

        	// set executed state
        	aRemoteStatement.executed();
        }
        catch (SQLException e)
        {
        	// set execute state with remote server error
        	aRemoteStatement.executed(new RemoteServerError(e));

        	mTraceLogger.log(e);

        	if (aRemoteStatement.isRemoteServerConnectionError() == true)
        	{
        		sResultCode = ResultCode.RemoteServerConnectionFail;
        	}
        	else
        	{
        		sResultCode = ResultCode.RemoteServerError;
        	}
        }

        return sResultCode;
    }
}
