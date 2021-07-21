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

import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.Task;

public class AddBatchTask extends Task
{
    protected long   mRemoteStatementId  = 0;
    protected int    mBindVariableCount  = 0;
    protected AddBatchOp.BindVariable[] mBindVariables = null;
        
    public AddBatchTask()
    {
        super(TaskType.AddBatch);
    }
    
    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        AddBatchOp sRequestOperation = (AddBatchOp)aOperation;

        mRemoteStatementId  = sRequestOperation.mRemoteStatementId;
        mBindVariableCount = sRequestOperation.mBindVariableCount;
        mBindVariables = sRequestOperation.mBindVariables;
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
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;
        RemoteStatement sRemoteStatement = null;
        
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
            sRemoteStatement = sLinkerDataSession.getRemoteStatement(mRemoteStatementId);
            
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
                
                sResultCode = addBatch(sRemoteNodeSession, sRemoteStatement);
            }            
        }
        
        if ((sResultCode != ResultCode.Success) ||
            (sRemoteStatement.isCompleteAddBatch() == true))
        {
            // allocate new result operation
            AddBatchResultOp sResultOperation =
                    (AddBatchResultOp)newResultOperation();
            
            if (sResultOperation == null)
            {
                mTraceLogger.logFatal(
                        "Task could not allocate new result operation for {0} operation.",
                        new Object[] { new Integer(getRequestOperationId()) });

                System.exit(-1);
                
                return;
            }

            // set common header of result operation
            sResultOperation.setSessionId(sLinkerSessionId);
            sResultOperation.setResultCode(sResultCode);
            sResultOperation.setSeqNo( this.getSeqNo() );
            
            // set payload contents of result operation
            sResultOperation.mRemoteStatementId = mRemoteStatementId;
            sResultOperation.mProcessedBatchCount = sRemoteStatement.getProcessedAddBatchCount();

            // send result operation
            sMainMediator.onSendOperation(sResultOperation);
            
            sResultOperation.close();        	
        }
        else
        {
        	/* nothing to do */
        }
    }
  
    private byte addBatch(RemoteNodeSession aRemoteNodeSession,
                          RemoteStatement aRemoteStatement)
    {
        byte sResultCode = ResultCode.Success;
        
        Statement sStatement = aRemoteStatement.getJdbcStatementObject();

        if (sStatement == null)
        {
            mTraceLogger.logError(
                    "JDBC Statement object was null. (Remote Statement Id = {0})",
                    new Object[] { new Long(mRemoteStatementId) } );
            
            sResultCode = ResultCode.NotPreparedRemoteStatement;
        }
        else
        {
            if ((sStatement instanceof PreparedStatement) == false)
            {
                mTraceLogger.logError(
                        "Remote statement did not preapre. (Remote statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) } );

                sResultCode = ResultCode.NotPreparedRemoteStatement;
            }
            else
            {
                PreparedStatement sPreparedStatement = (PreparedStatement)sStatement;

                // set laste remote statement
                aRemoteNodeSession.setLastRemoteStatementId(mRemoteStatementId);
                
                // set executing state
                aRemoteStatement.executing(RemoteStatement.TaskType.Bind);
                
                try
                {
                	for (int i = 0; i < mBindVariableCount; i++)
                	{
                		if (mBindVariables[i].mBindVariableType == AltibaseSQLType.SQL_VARCHAR)
                		{
                			sPreparedStatement.setString(mBindVariables[i].mBindVariableIndex, // 1-based index
                					mBindVariables[i].mBindVariableString);

                			sResultCode = ResultCode.Success;
                		}
                		else
                		{
                			// set executed state with failed
                			aRemoteStatement.executed(TaskResult.Failed);

                			mTraceLogger.logError(
                					"Not supported bind variable type. (Bind Variable Type = {0})",
                					new Object[] { new Integer(mBindVariables[i].mBindVariableType) } );

                			sResultCode = ResultCode.NotSupportedDataType;

                			break;
                		}
                	}

                	sPreparedStatement.addBatch();

                	aRemoteStatement.increaseProcessedAddBatchCount();
                	
                	// set executed state
                	aRemoteStatement.executed();
                }
                catch (SQLException e)
                {
                    // set executed state
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
            }
        }
        
        return sResultCode;
    }
}
