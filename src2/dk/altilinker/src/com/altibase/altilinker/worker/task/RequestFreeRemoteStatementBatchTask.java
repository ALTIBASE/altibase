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

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.Task;

public class RequestFreeRemoteStatementBatchTask extends Task
{
    protected long mRemoteStatementId = 0;
    
    public RequestFreeRemoteStatementBatchTask()
    {
        super(TaskType.RequestFreeRemoteStatementBatch);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        RequestFreeRemoteStatementBatchOp sRequestOperation =
                (RequestFreeRemoteStatementBatchOp)aOperation;

        mRemoteStatementId = sRequestOperation.mRemoteStatementId;
    }
    
    public void run()
    {
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;
        
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
            RemoteStatement sRemoteStatement =
                    sLinkerDataSession.getRemoteStatement(mRemoteStatementId);
            
            if (sRemoteStatement == null)
            {
                mTraceLogger.logError(
                        "Unknown remote statement. (Remote Statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) } );
                
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

                // set last remote statement
                sRemoteNodeSession.setLastRemoteStatementId(mRemoteStatementId);

                // closer remote statement
                sLinkerDataSession.closeRemoteStatement(mRemoteStatementId);

                mTraceLogger.logInfo(
                        "Remote statement with JDBC statement closed. (Remote Statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) } );
            }            
        }
        
        // allocate new result operation
        RequestFreeRemoteStatementBatchResultOp sResultOperation =
                (RequestFreeRemoteStatementBatchResultOp)newResultOperation();
        
        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                    "Task could not allocate new result operation for {0} operation.",
                    new Object[] { new Integer(getRequestOperationId()) } );
            
            System.exit(-1);
            
            return;
        }

        // set common header of result operation
        sResultOperation.setSessionId(getLinkerSessionId());
        sResultOperation.setResultCode(sResultCode);
        sResultOperation.setSeqNo( this.getSeqNo() );
        
        // set payload contents of result operation
        sResultOperation.mRemoteStatementId = mRemoteStatementId;

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);
        
        sResultOperation.close();
    }
}
