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

public class RequestCloseRemoteNodeSessionTask extends Task
{
    protected short mRemoteNodeSessionId = 0;
    
    public RequestCloseRemoteNodeSessionTask()
    {
        super(TaskType.RequestCloseRemoteNodeSession);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        RequestCloseRemoteNodeSessionOp sRequestOperation =
                (RequestCloseRemoteNodeSessionOp)aOperation;

        mRemoteNodeSessionId = sRequestOperation.mRemoteNodeSessionId;
    }
    
    public void run()
    {
        MainMediator   sMainMediator                   = getMainMediator();
        SessionManager sSessionManager                 = getSessionManager();
        int            sLinkerSessionId                = getLinkerSessionId();
        byte           sResultCode                     = ResultCode.Success;
        int            sRemainedRemoteNodeSessionCount = 0;
        
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
            if (mRemoteNodeSessionId == 0)
            {
                int sRemoteStatementCount =
                        sLinkerDataSession.getRemoteStatementCount();

                if (sRemoteStatementCount > 0)
                {
                    mTraceLogger.logError(
                            "All of remote node sessions could not close, becauste any remote statements was executing.");
                    
                    sResultCode = ResultCode.Busy;
                }
                else
                {
                    sLinkerDataSession.closeRemoteNodeSession();

                    sResultCode = ResultCode.Success;
                }
            }
            else
            {
                RemoteNodeSession sRemoteNodeSession =
                        sLinkerDataSession.getRemoteNodeSession(
                                mRemoteNodeSessionId);
                
                if (sRemoteNodeSession == null)
                {
                    mTraceLogger.logError(
                            "Unknown remote node session. (Remote Node Session Id = {0})",
                            new Object[] { new Integer(mRemoteNodeSessionId) } );

                    sResultCode = ResultCode.UnknownSession;
                }
                else
                {
                    int sRemoteStatementCount =
                            sLinkerDataSession.getRemoteStatementCount(
                                    mRemoteNodeSessionId);

                    if (sRemoteStatementCount > 0)
                    {
                        mTraceLogger.logError(
                                "Remote node session could not close, becauste remote statement was executing. (Remote Node Session Id = {0})",
                                new Object[] { new Integer(mRemoteNodeSessionId) } );
                        
                        sResultCode = ResultCode.Busy;
                    }
                    else
                    {
                        sLinkerDataSession.closeRemoteNodeSession(mRemoteNodeSessionId);
                        
                        sRemainedRemoteNodeSessionCount =
                                sLinkerDataSession.getRemoteNodeSessionCount();

                        sResultCode = ResultCode.Success;
                    }
                }
            }
        }
        
        // allocate new result operation
        RequestCloseRemoteNodeSessionResultOp sResultOperation =
                (RequestCloseRemoteNodeSessionResultOp)newResultOperation();
        
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
        sResultOperation.setXA( this.isXA() );

        // set payload contents of result operation
        sResultOperation.mRemoteNodeSessionId = mRemoteNodeSessionId;
        sResultOperation.mRemainedRemoteNodeSessionCount =
                sRemainedRemoteNodeSessionCount;

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);
        
        sResultOperation.close();
    }
}
