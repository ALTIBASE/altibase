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
import java.util.LinkedList;
import java.util.Map;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.controller.PropertyManager;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.ElapsedTimer;
import com.altibase.altilinker.worker.Task;

public class RollbackTask extends Task
{
    protected int     mRollbackFlag       = RollbackFlag.Transaction;
    protected String  mSavepointName      = null;
    protected short[] mRemoteNodeSessions = null;
    
    private static final int RetrySleepTime = 200; // 200ms

    private final int mRemoteNodeReceiveTimeout =
            PropertyManager.getInstance().getRemoteNodeReceiveTimeout() * 1000;
    
    public RollbackTask()
    {
        super(TaskType.Rollback);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        RollbackOp sRequestOperation = (RollbackOp)aOperation;

        mRollbackFlag       = sRequestOperation.mRollbackFlag;
        mSavepointName      = sRequestOperation.mSavepointName;
        mRemoteNodeSessions = sRequestOperation.mRemoteNodeSessions;
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
            boolean sTransactionRollback = false;
            
            if (mRollbackFlag == RollbackFlag.Transaction)
            {
                sTransactionRollback = true;
            }
            else if (mRollbackFlag == RollbackFlag.SpecificSavepoint)
            {
                if (mSavepointName == null)
                {
                    sTransactionRollback = true;
                }
            }
            
            if (sTransactionRollback == true)
            {
                // rollback to transaction for all of remote node sessions
                RemoteNodeSession sRemoteNodeSession;
                Map               sRemoteNodeSessionMap;
                Map.Entry         sEntry;
                Iterator          sIterator;
                
                sRemoteNodeSessionMap = sLinkerDataSession.getRemoteNodeSessionMap();
    
                sIterator = sRemoteNodeSessionMap.entrySet().iterator();
                while (sIterator.hasNext() == true)
                {
                    sEntry = (Map.Entry)sIterator.next();
    
                    sRemoteNodeSession = (RemoteNodeSession)sEntry.getValue();
    
                    if (sRemoteNodeSession.isExecuting() == true)
                    {
                        mTraceLogger.logWarning(
                                "Remote node session was busy. (Remote Node Session Id = {0})",
                                new Object[] { new Integer(sRemoteNodeSession.getSessionId()) } );
                    }
                    
                    sResultCode = 
                            rollback(sRemoteNodeSession,
                                     true,
                                     mSavepointName);
                }
    
                // close all of remote node sessions
                sLinkerDataSession.closeRemoteNodeSession();
                
                sResultCode = ResultCode.Success;
            }
            else
            {
                int               i;
                int               sRemoteNodeSessionId;
                boolean           sRemoteNodeSessionTransactionRollback;
                RemoteNodeSession sRemoteNodeSession;
                Map               sRemoteNodeSessionMap;
                Map.Entry         sEntry;
                Iterator          sIterator;
                
                sRemoteNodeSessionMap = sLinkerDataSession.getRemoteNodeSessionMap();
                
                sIterator = sRemoteNodeSessionMap.entrySet().iterator();
                while (sIterator.hasNext() == true)
                {
                    sEntry = (Map.Entry)sIterator.next();
    
                    sRemoteNodeSession   = (RemoteNodeSession)sEntry.getValue();
                    sRemoteNodeSessionId = sRemoteNodeSession.getSessionId();

                    sRemoteNodeSessionTransactionRollback = true;
                    
                    if (mRemoteNodeSessions != null)
                    {
                        for (i = 0; i < mRemoteNodeSessions.length; ++i)
                        {
                            if (sRemoteNodeSessionId == mRemoteNodeSessions[i])
                            {
                                sRemoteNodeSessionTransactionRollback = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        // transaction rollback for remote node session
                    }
                            
                    if (sRemoteNodeSession.isExecuting() == true)
                    {
                        mTraceLogger.logWarning(
                                "Remote node session was busy. (Remote Node Session Id = {0})",
                                new Object[] { new Integer(sRemoteNodeSessionId) } );
                    }

                    sResultCode = 
                            rollback(sRemoteNodeSession,
                                     sRemoteNodeSessionTransactionRollback,
                                     mSavepointName);
                }
            }
            
            sResultCode = ResultCode.Success;
        }
        
        // allocate new result operation
        RollbackResultOp sResultOperation =
                (RollbackResultOp)newResultOperation();
        
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
    
    private byte rollback(RemoteNodeSession aRemoteNodeSession,
                          boolean           aTransactionRollback,
                          String            aSavepointName)
    {
        byte         sResultCode       = ResultCode.Success;
        int          sRetryCount       = 0;
        long         sTime1            = 0;
        long         sTime2            = 0;
        ElapsedTimer sElapsedTimer     = ElapsedTimer.getInstance();
        Connection   sConnection       = null;
        SQLException sLastSQLException = null;

        // start timer
        sTime1 = sElapsedTimer.time();
        
        // set executing state
        aRemoteNodeSession.executing(RemoteNodeSession.TaskType.Rollback);

        sConnection = aRemoteNodeSession.getJdbcConnectionObject();
        
        do
        {
            try
            {
                if (aTransactionRollback == false)
                {
                    Savepoint sSavepoint =
                            aRemoteNodeSession.getJdbcSavepointObject(aSavepointName);
                    
                    if (sSavepoint != null)
                    {
                        sConnection.rollback(sSavepoint);
    
                        // release previous savepoints
                        LinkedList sSavepointList = aRemoteNodeSession.rollbackedJdbcSavepointObject(sSavepoint);
                        sSavepointList.clear();
                        
                        aRemoteNodeSession.executed();
                    }
                    else
                    {
                        // if a specific savepoint do not exist in this transaction,
                        // we rollback to transaction.
                        sConnection.rollback();

                        // release all of savepoints
                        LinkedList sSavepointList = aRemoteNodeSession.rollbackedJdbcSavepointObject(); 
                        sSavepointList.clear();
                        
                        aRemoteNodeSession.executed();
                    }
                }
                else
                {
                    sConnection.rollback();

                    // release all of savepoints
                    LinkedList sSavepointList = aRemoteNodeSession.rollbackedJdbcSavepointObject();
                    sSavepointList.clear();
                    
                    aRemoteNodeSession.executed();
                }

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
                    "Remote transaction retry to rollback. (Retry Count = {0}",
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
            // set executed state
            aRemoteNodeSession.executed();

            sResultCode = ResultCode.Success;
        }
        
        return sResultCode;
    }
    
    private void releaseSavepoints(Connection aConnection,
                                   LinkedList aSavepointList) throws SQLException
    {
        Iterator sIterator = aSavepointList.iterator();
        while (sIterator.hasNext() == true)
        {
            Savepoint sReleaseSavepoint = (Savepoint)sIterator.next();
            
            aConnection.releaseSavepoint(sReleaseSavepoint);
            
            sIterator.remove();
        }
        
        aSavepointList.clear();
    }
}
