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

import java.sql.PreparedStatement;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.JdbcDataManager;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.Task;

public class RequestResultSetMetaTask extends Task
{
    public long mRemoteStatementId = 0;
    
    public RequestResultSetMetaTask()
    {
        super(TaskType.RequestResultSetMeta);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        RequestResultSetMetaOp sRequestOperation = (RequestResultSetMetaOp)aOperation;

        mRemoteStatementId = sRequestOperation.mRemoteStatementId;
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
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;
        
        RequestResultSetMetaResultOp.ColumnMeta[] sColumnMeta = null;
        
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
            else if (sRemoteStatement.isExecuting() == true)
            {
                mTraceLogger.logError(
                        "Remote statement was busy. (Remote Statement Id = {0})",
                        new Object[] { new Long(mRemoteStatementId) } );

                sResultCode = ResultCode.Busy;
            }
            else
            {
                Statement sStatement = sRemoteStatement.getJdbcStatementObject();

                if (sStatement == null)
                {
                    mTraceLogger.logError(
                            "JDBC Statement object was null. (Remote Statement Id = {0})",
                            new Object[] { new Long(mRemoteStatementId) } );
                    
                    sResultCode = ResultCode.NotPreparedRemoteStatement;
                }
                else if ((sStatement instanceof PreparedStatement) == false)
                {
                    mTraceLogger.logError(
                            "JDBC Statement object was not prepared statement. (Remote Statement Id = {0})",
                            new Object[] { new Long(mRemoteStatementId) } );
                    
                    sResultCode = ResultCode.NotPreparedRemoteStatement;
                }
                else
                {
                    PreparedStatement sPreparedStatement = (PreparedStatement)sStatement;
                    
                    ColumnMetaResult sColumnMetaResult =
                                            getColumnMeta(sPreparedStatement,sRemoteStatement);

                    sColumnMeta = sColumnMetaResult.mColumnMeta;
                    sResultCode = sColumnMetaResult.mResultCode;
                }
            }            
        }
        
        // allocate new result operation
        RequestResultSetMetaResultOp sResultOperation =
                (RequestResultSetMetaResultOp)newResultOperation();
        
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
        sResultOperation.mColumnMeta        = sColumnMeta;

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);
        
        sResultOperation.close();
    }
    
    private static class ColumnMetaResult
    {
        public RequestResultSetMetaResultOp.ColumnMeta[] mColumnMeta = null;
        public byte                                      mResultCode = ResultCode.Success;
    }
    
    private ColumnMetaResult getColumnMeta(PreparedStatement aPreparedStatement, RemoteStatement aRemoteStatement)
    {
        ColumnMetaResult sColumnMetaResult = new ColumnMetaResult();

        try
        {
            ResultSetMetaData sResultSetMetaData = aPreparedStatement.getMetaData();
            
            if (sResultSetMetaData == null)
            {
                sColumnMetaResult.mResultCode = ResultCode.StatementTypeError;
            }
            else
            {
                int sColumnCount = sResultSetMetaData.getColumnCount();
                if (sColumnCount > 0)
                {
                    RequestResultSetMetaResultOp.ColumnMeta[] sColumnMeta = 
                            new RequestResultSetMetaResultOp.ColumnMeta[sColumnCount];
    
                    int i;
                    for (i = 0; i < sColumnCount; ++i)
                    {
                        sColumnMeta[i] = new RequestResultSetMetaResultOp.ColumnMeta();
                        sColumnMeta[i].mColumnName = sResultSetMetaData.getColumnName(i + 1);
                        sColumnMeta[i].mColumnType = JdbcDataManager.toSQLType(sResultSetMetaData.getColumnType(i + 1));
                                     
                        /* >> BUG-37394 */
                        if( ( sColumnMeta[i].mColumnType != AltibaseSQLType.SQL_CLOB )  &&  ( sColumnMeta[i].mColumnType != AltibaseSQLType.SQL_BLOB ) )
                        {
                            sColumnMeta[i].mPrecision  = JdbcDataManager.convertColumnPrecision( sResultSetMetaData.getPrecision(i + 1) );
                            sColumnMeta[i].mScale      = sResultSetMetaData.getScale(i + 1);
                            sColumnMeta[i].mNullable   = JdbcDataManager.toNullable(sResultSetMetaData.isNullable(i + 1));
                        }/* << BUG-37394 */
                        else
                        {
                       	    // nothing to do
                        }
                    }
                    
                    sColumnMetaResult.mColumnMeta = sColumnMeta;
                }

                sColumnMetaResult.mResultCode = ResultCode.Success;
            }
        }
        catch (SQLException e)
        {
        	
            mTraceLogger.log(e);

            if (RemoteServerError.isConnectionError(e.getSQLState()) == true)
            {
                sColumnMetaResult.mResultCode = ResultCode.RemoteServerConnectionFail;
            }
            else
            {
                sColumnMetaResult.mResultCode = ResultCode.RemoteServerError;
            }
        }
        catch (Exception e) /* >> BUG-37394  Else Exception */
        {
          	
            // set execute state with remote server error      	
       	    aRemoteStatement.executed(new RemoteServerError(null, 0 , "\""+ e.toString() + "\" at RequestResultSetMetaTask.getColumnMeta() "));
      
            mTraceLogger.log(e);
            sColumnMetaResult.mResultCode = ResultCode.RemoteServerError;   

        } /* << BUG-37394 */
        
        
        
        
        return sColumnMetaResult;
    }
}
