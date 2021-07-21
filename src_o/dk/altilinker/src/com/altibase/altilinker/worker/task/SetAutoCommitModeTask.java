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
import java.sql.SQLException;
import java.util.Iterator;
import java.util.Map;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.Task;

public class SetAutoCommitModeTask extends Task
{
    protected boolean mAutoCommitMode = false;
    
    public SetAutoCommitModeTask()
    {
        super(TaskType.SetAutoCommitMode);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        SetAutoCommitModeOp sRequestOperation = (SetAutoCommitModeOp)aOperation;

        mAutoCommitMode = sRequestOperation.mAutoCommitMode;
    }
    
    public void run()
    {
        MainMediator   sMainMediator   = getMainMediator();
        SessionManager sSessionManager = getSessionManager();

        int sLinkerSessionId = getLinkerSessionId();

        byte sResultCode = ResultCode.Success;
        
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
            // for new remote server connection in the future
            sLinkerDataSession.setAutoCommitMode(mAutoCommitMode);

            // change auto-commit mode for all of connected remote node sessions
            Map sRemoteNodeSessionMap =
                    sLinkerDataSession.getRemoteNodeSessionMap();

            Iterator sIterator = sRemoteNodeSessionMap.entrySet().iterator();
            while (sIterator.hasNext() == true)
            {
                Map.Entry sEntry = (Map.Entry)sIterator.next();

                RemoteNodeSession sRemoteNodeSession =
                        (RemoteNodeSession)sEntry.getValue();

                Connection sConnection =
                        sRemoteNodeSession.getJdbcConnectionObject();

                sRemoteNodeSession.executing(
                        RemoteNodeSession.TaskType.AutoCommitChange);
                
                try
                {
                    sConnection.setAutoCommit(mAutoCommitMode);
                    
                    sRemoteNodeSession.executed();

                    sRemoteNodeSession.setManuallyCommit(false);
                }
                catch (SQLException e)
                {
                    sRemoteNodeSession.executed(new RemoteServerError(e));
                    
                    mTraceLogger.log(e);

                    if (mAutoCommitMode == true)
                    {
                        sRemoteNodeSession.setManuallyCommit(true);
                        
                        mTraceLogger.logWarning(
                                "Manually commit mode is on. (Linker Data Session Id = {0}, Remote Node Session Id = {1})",
                                new Object[] {
                                        new Integer(sLinkerSessionId),
                                        new Integer(sRemoteNodeSession.getSessionId()) });
                    }
                    else
                    {
                        sRemoteNodeSession.setManuallyCommit(false);
                    }
                }
            }
            
            sResultCode = ResultCode.Success;
        }
        
        // allocate new result operation
        SetAutoCommitModeResultOp sResultOperation =
                (SetAutoCommitModeResultOp)newResultOperation();
        
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

        // nothing payload contents of result operation

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);
        
        sResultOperation.close();
    }
}
