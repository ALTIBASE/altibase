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

import java.sql.SQLException;
import java.sql.Statement;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.controller.PropertyManager;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.ElapsedTimer;
import com.altibase.altilinker.worker.Task;

public class AbortRemoteStatementTask extends Task
{
    private static final int RetrySleepTime = 200; // 200ms
    
    private final int mRemoteNodeReceiveTimeout =
            PropertyManager.getInstance().getRemoteNodeReceiveTimeout() * 1000;

    protected long mRemoteStatementId = 0;
    
    public AbortRemoteStatementTask()
    {
        super(TaskType.AbortRemoteStatement);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        AbortRemoteStatementOp sRequestOperation = (AbortRemoteStatementOp)aOperation;

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
            else
            {
                RemoteNodeSession sRemoteNodeSession = 
                        sLinkerDataSession.getRemoteNodeSession(
                                sRemoteStatement.getRemoteNodeSessionId());
                
                sResultCode = abort(sRemoteNodeSession, sRemoteStatement, sLinkerDataSession);
            }            
        }
        
        if (isSendResultOperation() == true)
        {
            // allocate new result operation
            AbortRemoteStatementResultOp sResultOperation =
                    (AbortRemoteStatementResultOp)newResultOperation();
            
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
            sResultOperation.mRemoteStatementId = mRemoteStatementId;
    
            // send result operation
            sMainMediator.onSendOperation(sResultOperation);
            
            sResultOperation.close();
        }
        else
        {
            // nothing to do
        }
    }
    
    private byte abort(RemoteNodeSession aRemoteNodeSession,
                       RemoteStatement   aRemoteStatement,
                       LinkerDataSession aLinkerDataSession)
    {
        byte sResultCode = ResultCode.Success;

        Statement sStatement = aRemoteStatement.getJdbcStatementObject();

        if (sStatement == null)
        {
            mTraceLogger.logWarning(
                    "JDBC Statement object was null. (Remote Statement Id = {0})",
                    new Object[] { new Long(aRemoteStatement.getStatementId()) } );
            aLinkerDataSession.closeRemoteStatement(aRemoteStatement.getStatementId());
            sResultCode = ResultCode.Success;
        }
        else
        {
            int          sRetryCount       = 0;
            long         sTime1;
            long         sTime2;
            ElapsedTimer sElapsedTimer     = ElapsedTimer.getInstance();
            SQLException sLastSQLException = null;

            // start timer
            sTime1 = sElapsedTimer.time();
            
            // set last executed remote statement
            aRemoteNodeSession.setLastRemoteStatementId(aRemoteStatement.getStatementId());
            
            // set executing state
            aRemoteStatement.executing(RemoteStatement.TaskType.Abort);
    
            do
            {
                try
                {
                    sStatement.cancel();
    
                    aRemoteStatement.executed();
                    
                    sLastSQLException = null;
                    
                    break;
                }
                catch (SQLException e)
                {
                    mTraceLogger.log(e);
                    
                    sLastSQLException = e;
                }

                sTime2 = sElapsedTimer.time();
                if ((sTime2 - sTime1) >= mRemoteNodeReceiveTimeout)
                {
                    // timeout
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
                        "Remote statement retry to abort. (Retry Count = {0}",
                        new Object[] { new Integer(sRetryCount) } );
                
            } while (true);

            if (sLastSQLException != null)
            {
                // set executed state with remote server error
                aRemoteStatement.executed(new RemoteServerError(sLastSQLException));
                aRemoteNodeSession.executed(new RemoteServerError(sLastSQLException));

                if (aRemoteStatement.isRemoteServerConnectionError() == true)
                {
                    sResultCode = ResultCode.RemoteServerConnectionFail;
                    if ( isXA() == true ) // Communication link failure
                    {
                        short sRemoteNodeSessionId = (short)aRemoteNodeSession.getSessionId();
                        aLinkerDataSession.closeRemoteStatement(aRemoteStatement.getStatementId());
                        aLinkerDataSession.closeRemoteNodeSession(sRemoteNodeSessionId);
                        
                        aRemoteNodeSession   = null;
                        sRemoteNodeSessionId = 0;
                    }
                }
                else
                {
                    aLinkerDataSession.closeRemoteStatement(aRemoteStatement.getStatementId());
                    sResultCode = ResultCode.RemoteServerError;
                }
            }
            else
            {
                // set executed state
                aRemoteStatement.executed();
                aLinkerDataSession.closeRemoteStatement(aRemoteStatement.getStatementId());
                sResultCode = ResultCode.Success;
            }
        }
        return sResultCode;
    }
}
