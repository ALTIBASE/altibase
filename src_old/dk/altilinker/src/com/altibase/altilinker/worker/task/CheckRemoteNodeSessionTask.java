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

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.Task;

public class CheckRemoteNodeSessionTask extends Task
{
    protected short mRemoteNodeSessionId = 0;
    
    public CheckRemoteNodeSessionTask()
    {
        super(TaskType.CheckRemoteNodeSession);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        CheckRemoteNodeSessionOp sRequestOperation =
                (CheckRemoteNodeSessionOp)aOperation;

        mRemoteNodeSessionId = sRequestOperation.mRemoteNodeSessionId;
    }
    
    public void run()
    {
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;
        
        CheckRemoteNodeSessionResultOp.RemoteNodeSession[] sRemoteNodeSessions = null;
        
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
                HashMap sRemoteNodeSessionMap = (HashMap)sLinkerDataSession.getRemoteNodeSessionMap();
    
                int sRemoteNodeSessionCount = sRemoteNodeSessionMap.size();
                if (sRemoteNodeSessionCount > 0)
                {
                    sRemoteNodeSessions =
                            new CheckRemoteNodeSessionResultOp.RemoteNodeSession[sRemoteNodeSessionCount];
        
                    int i = 0;
                    Iterator sIterator;
                    
                    sIterator = sRemoteNodeSessionMap.entrySet().iterator();
                    while (sIterator.hasNext() == true)
                    {
                        Map.Entry sEntry = (Map.Entry)sIterator.next();
        
                        Integer sRemoteNodeSessionId = (Integer)sEntry.getKey();
                        RemoteNodeSession sRemoteNodeSession = (RemoteNodeSession)sEntry.getValue();
        
                        int sRemoteNodeSessionStatus = getRemoteNodeSessionStatus(sRemoteNodeSession, sLinkerDataSession);
                        
                        sRemoteNodeSessions[i] = new CheckRemoteNodeSessionResultOp.RemoteNodeSession();
                        sRemoteNodeSessions[i].mRemoteNodeSessionId     = sRemoteNodeSessionId.shortValue();
                        sRemoteNodeSessions[i].mRemoteNodeSessionStatus = sRemoteNodeSessionStatus;
                        
                        ++i;
                    }
                }
            }
            else
            {
                RemoteNodeSession sRemoteNodeSession = 
                        sLinkerDataSession.getRemoteNodeSession(mRemoteNodeSessionId);

                if (sRemoteNodeSession != null)
                {
                    sRemoteNodeSessions = new CheckRemoteNodeSessionResultOp.RemoteNodeSession[1];
                    
                    int sRemoteNodeSessionStatus = getRemoteNodeSessionStatus(sRemoteNodeSession, sLinkerDataSession);
                    
                    sRemoteNodeSessions[0] = new CheckRemoteNodeSessionResultOp.RemoteNodeSession();
                    sRemoteNodeSessions[0].mRemoteNodeSessionId     = mRemoteNodeSessionId;
                    sRemoteNodeSessions[0].mRemoteNodeSessionStatus = sRemoteNodeSessionStatus;
                }
            }
        }

        // allocate new result operation
        CheckRemoteNodeSessionResultOp sResultOperation =
                (CheckRemoteNodeSessionResultOp)newResultOperation();
        
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
        sResultOperation.mRemoteNodeSessions = sRemoteNodeSessions;

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);
        
        sResultOperation.close();
    }
    
    private int getRemoteNodeSessionStatus(RemoteNodeSession aRemoteNodeSession, LinkerDataSession aLinkerDataSession)
    {
        int sRemoteNodeSessionStatus = RemoteNodeSessionStatus.Disconnected;

        switch (aRemoteNodeSession.getSessionState())
        {
        case RemoteNodeSession.SessionState.Connected:
        case RemoteNodeSession.SessionState.Executing:
        case RemoteNodeSession.SessionState.Executed:
            {
                sRemoteNodeSessionStatus = RemoteNodeSessionStatus.Connected;

                // check remote node session
                if (aRemoteNodeSession.isRemoteServerConnectionError() == true)
                {
                    sRemoteNodeSessionStatus = RemoteNodeSessionStatus.Disconnected;
                }
                else
                {
                    // check last remote statement
                    long sLastRemoteStatementId = aRemoteNodeSession.getLastRemoteStatementId();
                    if (sLastRemoteStatementId > 0)
                    {
                        RemoteStatement sLastRemoteStatement =
                                aRemoteNodeSession.getRemoteStatement(sLastRemoteStatementId);
                        
                        if (sLastRemoteStatement != null)
                        {
                            if (sLastRemoteStatement.isRemoteServerConnectionError() == true)
                            {
                                sRemoteNodeSessionStatus = RemoteNodeSessionStatus.Disconnected;
                                if ( isXA() == true ) 
                                {
                                    short sRemoteNodeSessionId = (short)aRemoteNodeSession.getSessionId();
                                    
                                    aLinkerDataSession.closeRemoteNodeSession(sRemoteNodeSessionId);
                                    
                                    aRemoteNodeSession   = null;
                                    sRemoteNodeSessionId = 0;    
                                }
                            }
                        }
                    }
                }
                
                break;
            }
            
        default:
            {
                sRemoteNodeSessionStatus = RemoteNodeSessionStatus.Disconnected;
                break;
            }
        }
        
        return sRemoteNodeSessionStatus;
    }
}
