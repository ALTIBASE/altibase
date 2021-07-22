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
import java.sql.SQLException;
import java.sql.Statement;

import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.controller.PropertyManager;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.TraceLogger;
import com.altibase.altilinker.worker.Task;

public class ExecuteRemoteStatementTask extends Task
{
    protected Xid    mXid               = null;
    protected String mTargetName        = null;
    protected String mTargetUser        = null;
    protected String mTargetPassword    = null;
    protected long   mRemoteStatementId = 0;
    protected int    mStatementType     = -1;
    protected String mStatementString   = null;

    private static final int mMaxRemoteNodeSessionCount =
            PropertyManager.getInstance().getRemoteNodeSessionCount();
    private static final int mNonQueryTimeout = PropertyManager.getInstance().getNonQueryTimeout();
    private static final int mQueryTimeout    = PropertyManager.getInstance().getQueryTimeout();
    
    public ExecuteRemoteStatementTask()
    {
        super(TaskType.ExecuteRemoteStatement);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        ExecuteRemoteStatementOp sRequestOperation = (ExecuteRemoteStatementOp)aOperation;

        if ( isXA() == true )
        {
            mXid = sRequestOperation.mAldpXid.convertToXid();
        }
        
        mTargetName        = sRequestOperation.mTargetName;
        mTargetUser        = sRequestOperation.mTargetUser;
        mTargetPassword    = sRequestOperation.mTargetPassword;
        mRemoteStatementId = sRequestOperation.mRemoteStatementId;
        mStatementType     = sRequestOperation.mStatementType;
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
        MainMediator   sMainMediator         = getMainMediator();
        SessionManager sSessionManager       = getSessionManager();
        int            sLinkerSessionId      = getLinkerSessionId();
        short          sRemoteNodeSessionId  = 0;
        int            sExecuteStatementType = StatementType.Unknown;
        int            sExecuteResultValue   = 0;
        byte           sResultCode           = ResultCode.Success;

        if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
        {
            mTraceLogger.logDebug(
                    "Execute to remote statement. (Linker Data Sesion Id = {0}, Remote Statement Id = {1}, Target Name = {2}, Target User = {3}, Target Password = {4}, Statement String = {5})",
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
                    new Object[] { new Integer(sLinkerSessionId) } );

            sResultCode = ResultCode.UnknownSession;
        }
        else
        {
            Target sTarget = JdbcDriverManager.getTarget(mTargetName,
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
                    int sRemoteNodeSessionCount = sSessionManager.getRemoteNodeSessionCount();

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
                            
                            sRemoteNodeSession   = null;
                            sRemoteNodeSessionId = 0;
                        }
                    }
                }

                if (sRemoteNodeSession != null)
                {
                    // set last remote statement
                    sRemoteNodeSession.setLastRemoteStatementId(mRemoteStatementId);
                    
                    sRemoteNodeSessionId = (short)sRemoteNodeSession.getSessionId();

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
                    }
                    else
                    {
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
                        
                        if ( sResultCode == ResultCode.Success )
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
                        }
                    }

                    if (sResultCode == ResultCode.Success)
                    {
                        int sStatementType = RemoteStatement.StatementType.None;
                        switch (mStatementType)
                        {
                        case StatementType.RemoteTable:
                            sStatementType = RemoteStatement.StatementType.RemoteTable;
                            break;
                            
                        case StatementType.RemoteExecuteImmediate:
                            sStatementType = RemoteStatement.StatementType.RemoteExecuteImmediate;
                            break;
                            
                        case StatementType.ExecuteStatement:
                            sStatementType = RemoteStatement.StatementType.ExecuteStatement;
                            break;
                            
                        default:
                            sStatementType = RemoteStatement.StatementType.None;
                            break;
                        }

                        sRemoteStatement.setStatementType(sStatementType);
                        sRemoteStatement.setStatementState(RemoteStatement.StatementState.Created);

                        // execute remote statement
                        ExecuteResult sExecuteResult = 
                                executeRemoteStatement(sRemoteNodeSession,
                                                       sRemoteStatement,
                                                       sStatementType,
                                                       mStatementString,
                                                       sLinkerDataSession);

                        sResultCode           = sExecuteResult.mResultCode;
                        sExecuteStatementType = sExecuteResult.mStatementType;
                        sExecuteResultValue   = sExecuteResult.mExecuteResultValue;
                    }
                } // if (sRemoteNodeSession != null)
            }
        }
        
        // allocate new result operation
        ExecuteRemoteStatementResultOp sResultOperation =
                (ExecuteRemoteStatementResultOp)newResultOperation();
        
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
        sResultOperation.mRemoteStatementId   = mRemoteStatementId;
        sResultOperation.mRemoteNodeSessionId = sRemoteNodeSessionId;
        sResultOperation.mStatementType       = sExecuteStatementType;
        sResultOperation.mExecuteResult       = sExecuteResultValue;

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);

        sResultOperation.close();
    }
    
    private static class ExecuteResult
    {
        public byte mResultCode         = ResultCode.Success;
        public int  mStatementType      = StatementType.Unknown;
        public int  mExecuteResultValue = 0;
    }
    
    private ExecuteResult executeRemoteStatement(RemoteNodeSession aRemoteNodeSession,
                                                 RemoteStatement   aRemoteStatement,
                                                 int               aStatementType,
                                                 String            aStatementString,
                                                 LinkerDataSession aLinkerDataSession)
    {
        ExecuteResult sExecuteResult = new ExecuteResult();

        Statement sStatement = aRemoteStatement.getJdbcStatementObject();

        if (aStatementType == RemoteStatement.StatementType.RemoteExecuteImmediate)
        {
            // set executing state
            aRemoteStatement.executing(RemoteStatement.TaskType.Execute);
            
            Connection sConnection = aRemoteNodeSession.getJdbcConnectionObject();

            try
            {
                if (sStatement == null)
                {
                    sStatement = sConnection.createStatement();

                    mTraceLogger.logInfo(
                            "JDBC statement created. (Remote Statement Id = {0})",
                            new Object[] { new Long(mRemoteStatementId) } );
                }

                // set non-query timeout
                sStatement.setQueryTimeout(mNonQueryTimeout);
                
                /*
                 * BUG-39001
                 * statement should set before executing
                 */
                aRemoteStatement.setJdbcStatementObject( sStatement );
                aRemoteStatement.setJdbcResultSetObject( null );
                aRemoteStatement.setJdbcResultSetMetaDataObject( null );

                // execute
                boolean sIsQuery = sStatement.execute(aStatementString);
                
                if (sIsQuery == true)
                {
                    sExecuteResult.mResultCode         = ResultCode.StatementTypeError;
                    sExecuteResult.mStatementType      = StatementType.ExecuteQueryStatement;
                    sExecuteResult.mExecuteResultValue = -1;
                }
                else
                {
                    // 0, if this statement is DDL and success.
                    // Updated row count, if this statement is DML and success.
                    // Exception will cause, if fail.
                    sExecuteResult.mStatementType      = StatementType.ExecuteNonQueryStatement;
                    sExecuteResult.mExecuteResultValue = sStatement.getUpdateCount();

                    if (aRemoteNodeSession.isManuallyCommit() == true)
                    {
                        sConnection.commit();
                    }
                }
                
                // set executed state
                aRemoteStatement.executed();
            }
            catch (SQLException e)
            {
                // set execute state with remote server error
                aRemoteStatement.executed(new RemoteServerError(e));
                
                mTraceLogger.log(e);
                
                if (aRemoteStatement.isRemoteServerConnectionError() == true)
                {
                    sExecuteResult.mResultCode = ResultCode.RemoteServerConnectionFail;
                    
                    if ( isXA() == true )
                    {
                        short sRemoteNodeSessionId = (short)aRemoteNodeSession.getSessionId();   
                        
                        aLinkerDataSession.closeRemoteNodeSession(sRemoteNodeSessionId);
                        
                        aRemoteNodeSession   = null;
                        sRemoteNodeSessionId = 0;    
                    }
                }
                else
                {
                    sExecuteResult.mResultCode = ResultCode.RemoteServerError;
                }

                sExecuteResult.mStatementType      = StatementType.Unknown;
                sExecuteResult.mExecuteResultValue = -1;
            }
        }
        else
        {
            // one of following type:
            //   - REMOTE_TABLE
            //   - REMOTE_EXECUTE_STATEMENT

            // set executing state
            aRemoteStatement.executing(RemoteStatement.TaskType.Execute);

            // close statement, if sStatement is not the prepared statement.
            if (sStatement != null)
            {
                if ((sStatement instanceof PreparedStatement) == false)
                {
                    mTraceLogger.logError(
                            "Not preapred remote statement. (Remote statement Id = {0})",
                            new Object[] { new Long(aRemoteStatement.getStatementId()) } );
    
                    try
                    {
                        sStatement.close();
                    }
                    catch (SQLException e)
                    {
                        mTraceLogger.log(e);
                    }
                    
                    sStatement = null;
                    
                    aRemoteStatement.setJdbcStatementObject(null);
                    
                    mTraceLogger.logInfo(
                            "JDBC statement closed. (Remote Statement Id = {0})",
                            new Object[] { new Long(mRemoteStatementId) } );
                }
            }

            Connection sConnection = aRemoteNodeSession.getJdbcConnectionObject();

            try
            {
                // create and prepare statement, if sStatement is null
                if (sStatement == null)
                {
                    sStatement = sConnection.prepareStatement(aStatementString);

                    mTraceLogger.logInfo(
                            "JDBC prepared statement created. (Remote Statement Id = {0})",
                            new Object[] { new Long(aRemoteStatement.getStatementId()) } );
                    
                    aRemoteStatement.setJdbcStatementObject(sStatement);
                }

                PreparedStatement sPreparedStatement = (PreparedStatement)sStatement;
                
                if (aStatementType == RemoteStatement.StatementType.RemoteTable ||
                    aStatementType == RemoteStatement.StatementType.ExecuteStatement)
                {
                    // set query timeout
                    sStatement.setQueryTimeout(mQueryTimeout);
                }
                else
                {
                    // set non-query timeout
                    sStatement.setQueryTimeout(mNonQueryTimeout);
                }
                
                /*
                 * BUG-39001
                 * statement should set before executing
                 */
                aRemoteStatement.setJdbcStatementObject( sStatement );
                aRemoteStatement.setJdbcResultSetObject( null );
                aRemoteStatement.setJdbcResultSetMetaDataObject( null );
                
                // execute
                boolean sIsQuery = sPreparedStatement.execute();

                if (aStatementType == RemoteStatement.StatementType.RemoteTable)
                {
                    if (sIsQuery == true)
                    {
                        sExecuteResult.mResultCode         = ResultCode.Success;
                        sExecuteResult.mStatementType      = StatementType.ExecuteQueryStatement;
                        sExecuteResult.mExecuteResultValue = 0;
                    }
                    else
                    {
                        sExecuteResult.mResultCode         = ResultCode.StatementTypeError;
                        sExecuteResult.mStatementType      = StatementType.ExecuteNonQueryStatement;
                        sExecuteResult.mExecuteResultValue = sPreparedStatement.getUpdateCount();
                    }
                }
                else // REMOTE_EXECUTE_STATEMENT
                {
                    if (sIsQuery == true)
                    {
                        sExecuteResult.mResultCode         = ResultCode.Success;
                        sExecuteResult.mStatementType      = StatementType.ExecuteQueryStatement;
                        sExecuteResult.mExecuteResultValue = 0;
                    }
                    else
                    {
                        sExecuteResult.mResultCode         = ResultCode.Success;
                        sExecuteResult.mStatementType      = StatementType.ExecuteNonQueryStatement;
                        sExecuteResult.mExecuteResultValue = sPreparedStatement.getUpdateCount();

                        // manually auto-commit
                        if (aRemoteNodeSession.isManuallyCommit() == true)
                        {
                            sConnection.commit();
                        }
                    }
                }
                
                // set executed state
                aRemoteStatement.executed();
            }
            catch (SQLException e)
            {
                // set executed state with remote server
                aRemoteStatement.executed(new RemoteServerError(e));
                
                mTraceLogger.log(e);
                
                if (aRemoteStatement.isRemoteServerConnectionError() == true)
                {
                    sExecuteResult.mResultCode = ResultCode.RemoteServerConnectionFail;
                    if ( isXA() == true ) 
                    {
                        aLinkerDataSession.closeRemoteNodeSession();
                    }
                }
                else
                {
                    sExecuteResult.mResultCode = ResultCode.RemoteServerError;
                }

                sExecuteResult.mStatementType      = StatementType.Unknown;
                sExecuteResult.mExecuteResultValue = -1;
            }
        }

        return sExecuteResult;
    }
}
