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
import java.sql.Savepoint;
import java.util.Iterator;
import java.util.Map;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.Task;

public class SavepointTask extends Task
{
    protected String mSavepointName = null;
    
    public SavepointTask()
    {
        super(TaskType.Savepoint);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        SavepointOp sRequestOperation = (SavepointOp)aOperation;

        mSavepointName = sRequestOperation.mSavepointName;
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
            sResultCode = savepoint(sLinkerDataSession);
        }
        
        // allocate new result operation
        SavepointResultOp sResultOperation =
                (SavepointResultOp)newResultOperation();
        
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
    
    private byte savepoint(LinkerDataSession aLinkerDataSession)
    {
        byte sResultCode = ResultCode.None;
        
        Map sRemoteNodeSessionMap = aLinkerDataSession.getRemoteNodeSessionMap();
        if (sRemoteNodeSessionMap.size() == 0)
        {
            sResultCode = ResultCode.Success;
        }
        else
        {
            sResultCode = ResultCode.None;
            
            Iterator sIterator = sRemoteNodeSessionMap.entrySet().iterator();
            while (sIterator.hasNext() == true)
            {
                Map.Entry sEntry = (Map.Entry)sIterator.next();

                RemoteNodeSession sRemoteNodeSession =
                        (RemoteNodeSession)sEntry.getValue();

                Connection sConnection =
                        sRemoteNodeSession.getJdbcConnectionObject();

                sRemoteNodeSession.executing(RemoteNodeSession.TaskType.Commit);
                
                try
                {
                    /*
                     * Remark: This is workaround code.
                     *         XDB's JDBC did not implement JDBC's specification.
                     *         if connect is auto commit mode, then the setSavepoint() call have to raise
                     *         an exception.
                     */
                    boolean sAutoCommitMode = sConnection.getAutoCommit();
                    if ( sAutoCommitMode == true )
                    {
                        sRemoteNodeSession.executed(TaskResult.Failed);
                        
                        sResultCode = ResultCode.FailedToSetSavepoint;
                    }
                    else
                    {
                        Savepoint sSavepoint = sConnection.setSavepoint(mSavepointName);
                    
                        if (sSavepoint != null)
                        {
                            sRemoteNodeSession.addJdbcSavepointObect(sSavepoint);
                            
                            sRemoteNodeSession.executed();
                        }
                        else
                        {
                            sRemoteNodeSession.executed(TaskResult.Failed);
                        }
                    
                        sResultCode = ResultCode.Success;
                    }
                }
                catch (SQLException e)
                {
                    sRemoteNodeSession.executed(new RemoteServerError(e));

                    boolean sAutoCommitMode = aLinkerDataSession.getAutoCommitMode();
                    
                    try
                    {
                        sAutoCommitMode = sConnection.getAutoCommit();
                    }
                    catch (SQLException e1)
                    {
                        sResultCode = ResultCode.RemoteServerConnectionFail;
                    }
                    
                    if (sResultCode == ResultCode.None)
                    {
                        if (sAutoCommitMode == true)
                        {
                            sResultCode = ResultCode.FailedToSetSavepoint;
                        }
                        else
                        {
                            sResultCode = ResultCode.RemoteServerConnectionFail;
                        }
                    }
                    
                    mTraceLogger.log(e);
                }
            }
        } 
        
        return sResultCode;
    }
}
