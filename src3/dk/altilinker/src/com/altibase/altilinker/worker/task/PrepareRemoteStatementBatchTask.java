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
import java.sql.PreparedStatement;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Types;

import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.*;
import com.altibase.altilinker.controller.*;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.TraceLogger;
import com.altibase.altilinker.worker.*;

public class PrepareRemoteStatementBatchTask extends Task
{
    protected Xid    mXid               = null;
    protected String mTargetName        = null;
    protected String mTargetUser        = null;
    protected String mTargetPassword    = null;
    protected long   mRemoteStatementId = 0;
    protected String mStatementString   = null;

    private final int mMaxRemoteNodeSessionCount =
            PropertyManager.getInstance().getRemoteNodeSessionCount();
    
    public PrepareRemoteStatementBatchTask()
    {
        super(TaskType.PrepareRemoteStatementBatch);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        PrepareRemoteStatementBatchOp sRequestOperation = (PrepareRemoteStatementBatchOp)aOperation;

        if ( isXA() == true )
        {
            mXid = sRequestOperation.mAldpXid.convertToXid();
        }
        
        mTargetName        = sRequestOperation.mTargetName;
        mTargetUser        = sRequestOperation.mTargetUser;
        mTargetPassword    = sRequestOperation.mTargetPassword;
        mRemoteStatementId = sRequestOperation.mRemoteStatementId;
        mStatementString   = sRequestOperation.mStatementString;
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
        MainMediator   sMainMediator        = getMainMediator();
        SessionManager sSessionManager      = getSessionManager();
        int            sLinkerSessionId     = getLinkerSessionId();
        short          sRemoteNodeSessionId = 0;
        byte           sResultCode          = ResultCode.Success;

        if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
        {
            mTraceLogger.logDebug("Prepare to remote statement. (Linker Data Sesion Id = {0}, Remote Statement Id = {1}, Target Name = {2}, Target User = {3}, Target Password = {4}, Statement String = {5})",
                    new Object[] {
                            new Integer(sLinkerSessionId),
                            new Long(mRemoteStatementId),
                            mTargetName,
                            (mTargetUser == null) ? "" : mTargetUser,
                            (mTargetPassword == null) ? "" : mTargetPassword,
                            mStatementString });
        }
        
        // check for connecting linker data session
        LinkerDataSession sLinkerDataSession =
                sSessionManager.getLinkerDataSession(sLinkerSessionId);
        
        if (sLinkerDataSession == null)
        {
            mTraceLogger.logError(
                    "Unknown linker data session. (Linker Data Session Id = {0})",
                    new Object[] { new Integer(sLinkerSessionId) });

            sResultCode = ResultCode.UnknownSession;
        }
        else
        {
            Target sTarget =
                    JdbcDriverManager.getTarget(
                            mTargetName,
                            mTargetUser,
                            mTargetPassword);
            
            if (sTarget == null)
            {
                sResultCode = ResultCode.RemoteServerConnectionFail;
            }
            else
            {
                RemoteNodeSession sRemoteNodeSession =
                        sLinkerDataSession.getRemoteNodeSession(sTarget);
                
                if (sRemoteNodeSession == null)
                {
                    int sRemoteNodeSessionCount = 
                            sSessionManager.getRemoteNodeSessionCount();
                    
                    if (sRemoteNodeSessionCount >= mMaxRemoteNodeSessionCount)
                    {
                        mTraceLogger.logError(
                                "Maximum remote node session excessed. (Maximum {0}).",
                                new Object[] { new Integer(mMaxRemoteNodeSessionCount) } );
                        
                        sResultCode = ResultCode.ExcessRemoteNodeSessionCount;
                    }
                    else
                    {
                        // allocate new remote node session
                        sRemoteNodeSession =
                                sLinkerDataSession.newRemoteNodeSession(sTarget);
                        
                        // connect to remote server
                        sResultCode =
                                JdbcDriverManager.connectToRemoteServer(
                                        sLinkerDataSession,
                                        sRemoteNodeSession,
                                        isXA() );
                        
                        if (sResultCode != ResultCode.Success)
                        {
                            sRemoteNodeSessionId = (short)sRemoteNodeSession.getSessionId();
                            
                            sLinkerDataSession.closeRemoteNodeSession(sRemoteNodeSessionId);
                            
                            sRemoteNodeSession = null;
                            sRemoteNodeSessionId = 0;
                        }
                    }
                }

                if (sRemoteNodeSession != null)
                {
                    // set last remote statement
                    sRemoteNodeSession.setLastRemoteStatementId(mRemoteStatementId);
                    
                    sRemoteNodeSessionId = (short)sRemoteNodeSession.getSessionId();
                    
                    if (sRemoteNodeSession.isExecuting() == true)
                    {
                        mTraceLogger.logError(
                                "Remote node session was busy. (Remote Node Session Id = {0})",
                                new Object[] { new Short(sRemoteNodeSessionId) } );
                        
                        sResultCode = ResultCode.Busy;
                    }
                    else
                    {
                        RemoteStatement sRemoteStatement =
                                sLinkerDataSession.getRemoteStatement(
                                        mRemoteStatementId);

                        if (sRemoteStatement != null)
                        {
                            // if this statement is still executing,
                            // then the requested statement cannot prepare.
                            if (sRemoteStatement.isExecuting() == true)
                            {
                                mTraceLogger.logError(
                                        "Remote statement was busy. (Remote Statement Id = {0})",
                                        new Object[] { new Long(mRemoteStatementId) } );
                                
                                sResultCode = ResultCode.Busy;
                            }
                            else
                            {
                                // if this statement already exist,
                                // then close the statement.
                                sLinkerDataSession.closeRemoteStatement(
                                        sRemoteStatement.getStatementId());
                                    
                                sRemoteStatement = null;
                            }
                        }
                        
                        // XA start
                        if( ( isXA() == true ) &&
                            ( isTxBegin() == true ) )
                        {
                            if ( sRemoteNodeSession.isXATransStarted() == true )
                            {
                                mTraceLogger.logError(
                                        "XATrans started. (Remote Statement Id = {0})",
                                        new Object[] { new Long(mRemoteStatementId) } );

                                System.exit(-1);
                                return;
                            }
                                    
                            XAResource sXARes = sRemoteNodeSession.getXAResource();
                            sRemoteNodeSession.setXid( mXid );
                            
                            try
                            {
                                sXARes.start( mXid, XAResource.TMNOFLAGS );
                                sRemoteNodeSession.setXATransStarted();
                            }
                            catch ( XAException e )
                            {
                                mTraceLogger.log( e );
                                sResultCode = ResultCode.XAException;
                            }
                        }

                        if (sResultCode == ResultCode.Success)
                        {
                            // allocate new remote statement
                            sRemoteStatement =
                                    sLinkerDataSession.newRemoteStatement(
                                           sRemoteNodeSessionId,
                                           mRemoteStatementId,
                                           mStatementString);

                            mTraceLogger.logInfo(
                                    "Remote statement created. (Remote Statement Id = {0})",
                                    new Object[] { new Long(mRemoteStatementId) } );
                            
                            sRemoteStatement.setStatementType(RemoteStatement.StatementType.BatchStatement);
                            sRemoteStatement.setStatementState(
                                    RemoteStatement.StatementState.Created);

                            // prepare remote statement
                            sResultCode = 
                                    prepareRemoteStatement(sLinkerDataSession,
                                                           sRemoteNodeSession,
                                                           sRemoteStatement);
                        }
                    }
                }
            }
        }
        
        // allocate new result operation
        PrepareRemoteStatementBatchResultOp sResultOperation =
                (PrepareRemoteStatementBatchResultOp)newResultOperation();
        
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
        sResultOperation.mRemoteStatementId   = mRemoteStatementId;
        sResultOperation.mRemoteNodeSessionId = sRemoteNodeSessionId;

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);
        
        sResultOperation.close();
    }

    private static
    byte prepareRemoteStatement(LinkerDataSession aLinkerDataSession,
                                RemoteNodeSession aRemoteNodeSession,
                                RemoteStatement   aRemoteStatement)
    {
        byte sResultCode = ResultCode.Success;

        Connection sConnection = aRemoteNodeSession.getJdbcConnectionObject();

        // set executing state
        aRemoteStatement.executing(RemoteStatement.TaskType.Prepare);

        PreparedStatement sPreparedStatement = null;

        try
        {
            sPreparedStatement = 
                    sConnection.prepareStatement(
                            aRemoteStatement.getStatementString());
            
            mTraceLogger.logInfo(
                    "Remote statement with JDBC prepared statement created. (Remote Statement Id = {0})",
                    new Object[] { new Long(aRemoteStatement.getStatementId()) } );
            
            // set remote node session receive timeout
            // before executing statement
            sPreparedStatement.setQueryTimeout(
                    PropertyManager.getInstance().getRemoteNodeReceiveTimeout());
            
            /*
             * BUG-39001
             * statement should set before executing
             */
            aRemoteStatement.setJdbcStatementObject( sPreparedStatement );
            aRemoteStatement.setJdbcResultSetObject( null );
            aRemoteStatement.setJdbcResultSetMetaDataObject( null );

            aRemoteStatement.executed();
        }
        catch (SQLException e)
        {
            // set executed state with remote server error
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

        return sResultCode;
    }
}
