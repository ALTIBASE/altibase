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

import java.util.Iterator;
import java.util.Map;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.Task;

public class RequestRemoteErrorInfoTask extends Task
{
    protected long mRemoteStatementId = 0;
    
    public RequestRemoteErrorInfoTask()
    {
        super(TaskType.RequestRemoteErrorInfo);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        RequestRemoteErrorInfoOp sRequestOperation =
                (RequestRemoteErrorInfoOp)aOperation;

        mRemoteStatementId = sRequestOperation.mRemoteStatementId;
    }
    
    public void run()
    {
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;
        TaskResult     sTaskResult      = null;
        
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
                // find remote node session for last remote statement
                RemoteNodeSession sFoundRemoteNodeSession = null;
                
                Map sRemoteNodeSessionMap =
                        sLinkerDataSession.getRemoteNodeSessionMap();
                
                Iterator sIterator = sRemoteNodeSessionMap.entrySet().iterator();
                while (sIterator.hasNext() == true)
                {
                    Map.Entry sEntry = (Map.Entry)sIterator.next();

                    RemoteNodeSession sRemoteNodeSession = 
                            (RemoteNodeSession)sEntry.getValue();
                    
                    if (mRemoteStatementId == sRemoteNodeSession.getLastRemoteStatementId())
                    {
                        sFoundRemoteNodeSession = sRemoteNodeSession;
                        break;
                    }
                }

                if (sFoundRemoteNodeSession != null)
                {
                    sTaskResult = sFoundRemoteNodeSession.getTaskResult();
                    
                    // close remote node session,
                    // if find and if sub remote statement is empty.
                    if (sFoundRemoteNodeSession.getRemoteStatementCount() == 0)
                    {
                        short sRemoteNodeSessionId =
                                (short)sFoundRemoteNodeSession.getSessionId();
                        
                        sLinkerDataSession.closeRemoteNodeSession(
                                sRemoteNodeSessionId);
                        
                        mTraceLogger.logDebug(
                                "When is requested remote error info, remote node session closed because its remote statements emptied. (Remote Statement Id = {0}, Remote Node Session = {1}",
                                new Object[] {
                                        new Long(mRemoteStatementId),
                                        new Short(sRemoteNodeSessionId) });
                    }
                }
                else
                {
                    mTraceLogger.logError(
                            "Unknown remote statement. (Remote Statement Id = {0})",
                            new Object[] { new Long(mRemoteStatementId) } );
                    
                    sResultCode = ResultCode.UnknownRemoteStatement;
                }
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
                sTaskResult = sRemoteStatement.getTaskResult();
            }
            
            if (sTaskResult != null &&
                sTaskResult.isRemoteServerError() == false)
            {
                sTaskResult = null;
            }
        }
        
        // allocate new result operation
        RequestRemoteErrorInfoResultOp sResultOperation = 
                (RequestRemoteErrorInfoResultOp)newResultOperation();
        
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
        sResultOperation.mRemoteStatementId = mRemoteStatementId;
        
        if (sResultCode == ResultCode.Success)
        {
            if (sTaskResult != null)
            {
                RemoteServerError sRemoteServerError = sTaskResult.getRemoteServerError();
                
                if (sRemoteServerError != null)
                {
                    sResultOperation.mErrorCode    = sRemoteServerError.getErrorCode();
                    sResultOperation.mSQLState     = sRemoteServerError.getSQLState();
                    sResultOperation.mErrorMessage = sRemoteServerError.getErrorMessage();
                }
            }
        }

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);

        sResultOperation.close();
    }
}
