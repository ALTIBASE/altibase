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

import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.controller.PropertyManager;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.ElapsedTimer;
import com.altibase.altilinker.worker.Task;

public class CommitTask extends Task
{
    private static final int RetrySleepTime           = 200; // 200ms
    private static final int RemoteNodeReceiveTimeout =
            PropertyManager.getInstance().getRemoteNodeReceiveTimeout() * 1000;
    
    public CommitTask()
    {
        super(TaskType.Commit);
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
            Map sRemoteNodeSessionMap =
                    sLinkerDataSession.getRemoteNodeSessionMap();

            Iterator sIterator = sRemoteNodeSessionMap.entrySet().iterator();
            while (sIterator.hasNext() == true)
            {
                Map.Entry sEntry = (Map.Entry)sIterator.next();

                RemoteNodeSession sRemoteNodeSession =
                        (RemoteNodeSession)sEntry.getValue();

                if (sRemoteNodeSession.isExecuting() == true)
                {
                    mTraceLogger.logWarning(
                            "Remote node session was busy. (Remote Node Session Id = {0})",
                            new Object[] { new Integer(sRemoteNodeSession.getSessionId()) } );
                }
                
                sResultCode = commit(sRemoteNodeSession);
            }

            // close all of remote node sessions
            sLinkerDataSession.closeRemoteNodeSession();
            
            sResultCode = ResultCode.Success;
        }
        
        // allocate new result operation
        CommitResultOp sResultOperation = (CommitResultOp)newResultOperation();
        
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

    private byte commit(RemoteNodeSession aRemoteNodeSession)
    {
        byte         sResultCode       = ResultCode.Success;
        int          sRetryCount       = 0;
        long         sTime1            = 0;
        long         sTime2            = 0;
        ElapsedTimer sElapsedTimer     = ElapsedTimer.getInstance();
        SQLException sLastSQLException = null;

        Connection sConnection = aRemoteNodeSession.getJdbcConnectionObject();

        // set executing state
        aRemoteNodeSession.executing(RemoteNodeSession.TaskType.Commit);

        do
        {
            try
            {
                sConnection.commit();
                
                aRemoteNodeSession.executed();

                sLastSQLException = null;
                
                break;
            }
            catch (SQLException e)
            {
                mTraceLogger.log(e);

                sLastSQLException = e;
            }
            
            sTime2 = sElapsedTimer.time();
            if ((sTime2 - sTime1) >= RemoteNodeReceiveTimeout)
            {
                break;
            }

            try
            {
                Thread.sleep(RetrySleepTime);
            }
            catch (InterruptedException e)
            {
                mTraceLogger.log(e);
            }
            
            ++sRetryCount;
            
            mTraceLogger.logDebug(
                    "Remote transaction retry to commit. (Retry Count = {0}",
                    new Object[] { new Integer(sRetryCount) } );
            
        } while (true);

        if (sLastSQLException != null)
        {
            // set executed state with remote server error
            aRemoteNodeSession.executed(new RemoteServerError(sLastSQLException));
            
            if (aRemoteNodeSession.isRemoteServerConnectionError() == true)
            {
                sResultCode = ResultCode.RemoteServerConnectionFail;
            }
            else
            {
                sResultCode = ResultCode.RemoteServerError;
            }
        }
        else
        {
            // set execute state
            aRemoteNodeSession.executed();
            
            sResultCode = ResultCode.Success;
        }
        
        return sResultCode;
    }
}
