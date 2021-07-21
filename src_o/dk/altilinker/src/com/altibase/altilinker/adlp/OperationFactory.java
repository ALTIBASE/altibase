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
 
package com.altibase.altilinker.adlp;

import com.altibase.altilinker.adlp.op.*;

/**
 * Operation factory static class to create concrete Operation object by operation ID
 */
public class OperationFactory
{
    private static OperationFactory mOperationFactory = new OperationFactory();

    /**
     * Constructor
     */
    private OperationFactory()
    {
        // nothing to do
    }
    
    /**
     * Get static instance
     * 
     * @return  Static instance
     */
    static public OperationFactory getInstance()
    {
        return mOperationFactory;
    }

    /**
     * create new concrete Operation object by operation ID
     * 
     * @param aOperationId      Operation ID
     * @return                  New Operation class object
     */
    public Operation newOperation(byte aOperationId)
    {
        Operation sOperation = null;
        
        switch (aOperationId)
        {
        case OpId.CreateLinkerCtrlSession:
            sOperation = new CreateLinkerCtrlSessionOp();
            break;
            
        case OpId.CreateLinkerCtrlSessionResult:
            sOperation = new CreateLinkerCtrlSessionResultOp();
            break;
            
        case OpId.CreateLinkerDataSession:
            sOperation = new CreateLinkerDataSessionOp();
            break;
            
        case OpId.CreateLinkerDataSessionResult:
            sOperation = new CreateLinkerDataSessionResultOp();
            break;
            
        case OpId.DestroyLinkerDataSession:
            sOperation = new DestroyLinkerDataSessionOp();
            break;
            
        case OpId.DestroyLinkerDataSessionResult:
            sOperation = new DestroyLinkerDataSessionResultOp();
            break;
            
        case OpId.RequestLinkerStatus:
            sOperation = new RequestLinkerStatusOp();
            break;
            
        case OpId.RequestLinkerStatusResult:
            sOperation = new RequestLinkerStatusResultOp();
            break;
            
        case OpId.RequestPVInfoDBLinkAltiLinkerStatus:
            sOperation = new RequestPVInfoDBLinkAltiLinkerStatusOp();
            break;
            
        case OpId.RequestPVInfoDBLinkAltiLinkerStatusResult:
            sOperation = new RequestPVInfoDBLinkAltiLinkerStatusResultOp();
            break;
            
        case OpId.PrepareLinkerShutdown:
            sOperation = new PrepareLinkerShutdownOp();
            break;
            
        case OpId.PrepareLinkerShutdownResult:
            sOperation = new PrepareLinkerShutdownResultOp();
            break;
            
        case OpId.DoLinkerShutdown:
            sOperation = new DoLinkerShutdownOp();
            break;
            
        case OpId.RequestCloseRemoteNodeSession:
            sOperation = new RequestCloseRemoteNodeSessionOp();
            break;
            
        case OpId.RequestCloseRemoteNodeSessionResult:
            sOperation = new RequestCloseRemoteNodeSessionResultOp();
            break;
            
        case OpId.SetAutoCommitMode:
            sOperation = new SetAutoCommitModeOp();
            break;
            
        case OpId.SetAutoCommitModeResult:
            sOperation = new SetAutoCommitModeResultOp();
            break;
            
        case OpId.CheckRemoteNodeSession:
            sOperation = new CheckRemoteNodeSessionOp();
            break;
            
        case OpId.CheckRemoteNodeSessionResult:
            sOperation = new CheckRemoteNodeSessionResultOp();
            break;
            
        case OpId.Commit:
            sOperation = new CommitOp();
            break;
            
        case OpId.CommitResult:
            sOperation = new CommitResultOp();
            break;
            
        case OpId.Rollback:
            sOperation = new RollbackOp();
            break;
            
        case OpId.RollbackResult:
            sOperation = new RollbackResultOp();
            break;
            
        case OpId.Savepoint:
            sOperation = new SavepointOp();
            break;
            
        case OpId.SavepointResult:
            sOperation = new SavepointResultOp();
            break;
            
        case OpId.AbortRemoteStatement:
            sOperation = new AbortRemoteStatementOp();
            break;
            
        case OpId.AbortRemoteStatementResult:
            sOperation = new AbortRemoteStatementResultOp();
            break;
            
        case OpId.UknownOperation:
            sOperation = new UnknownOp();
            break;
            
        case OpId.PrepareRemoteStatement:
            sOperation = new PrepareRemoteStatementOp();
            break;
            
        case OpId.PrepareRemoteStatementResult:
            sOperation = new PrepareRemoteStatementResultOp();
            break;
            
        case OpId.RequestRemoteErrorInfo:
            sOperation = new RequestRemoteErrorInfoOp();
            break;
            
        case OpId.RequestRemoteErrorInfoResult:
            sOperation = new RequestRemoteErrorInfoResultOp();
            break;
            
        case OpId.ExecuteRemoteStatement:
            sOperation = new ExecuteRemoteStatementOp();
            break;
            
        case OpId.ExecuteRemoteStatementResult:
            sOperation = new ExecuteRemoteStatementResultOp();
            break;
            
        case OpId.RequestFreeRemoteStatement:
            sOperation = new RequestFreeRemoteStatementOp();
            break;
            
        case OpId.RequestFreeRemoteStatementResult:
            sOperation = new RequestFreeRemoteStatementResultOp();
            break;
            
        case OpId.FetchResultSet:
            sOperation = new FetchResultSetOp();
            break;
            
        case OpId.FetchResultRow:
            sOperation = new FetchResultRowOp();
            break;
            
        case OpId.RequestResultSetMeta:
            sOperation = new RequestResultSetMetaOp();
            break;
            
        case OpId.RequestResultSetMetaResult:
            sOperation = new RequestResultSetMetaResultOp();
            break;
            
        case OpId.BindRemoteVariable:
            sOperation = new BindRemoteVariableOp();
            break;
            
        case OpId.BindRemoteVariableResult:
            sOperation = new BindRemoteVariableResultOp();
            break;
            
        case OpId.RequestLinkerDump:
            sOperation = new RequestLinkerDumpOp();
            break;
     
        case OpId.RequestLinkerDumpResult:
            sOperation = new RequestLinkerDumpResultOp();
            break;
      
        case OpId.PrepareRemoteStatementBatch:
            sOperation = new PrepareRemoteStatementBatchOp();
            break;
        	
        case OpId.PrepareRemoteStatementBatchResult:
            sOperation = new PrepareRemoteStatementBatchResultOp();
            break;
        	
        case OpId.RequestFreeRemoteStatementBatch:
            sOperation = new RequestFreeRemoteStatementBatchOp();
            break;
        	
        case OpId.RequestFreeRemoteStatementBatchResult:
            sOperation = new RequestFreeRemoteStatementBatchResultOp();
            break;
        	
        case OpId.ExecuteRemoteStatementBatch:
            sOperation = new ExecuteRemoteStatementBatchOp();
            break;
        	
        case OpId.ExecuteRemoteStatementBatchResult:
            sOperation  = new ExecuteRemoteStatementBatchResultOp();
            break;
        	
        case OpId.PrepareAddBatch:
            sOperation = new PrepareAddBatchOp();
            break;
        	
        case OpId.PrepareAddBatchResult:
            sOperation = new PrepareAddBatchResultOp();
            break;
        	
        case OpId.AddBatch:
            sOperation = new AddBatchOp();
            break;
        	
        case OpId.AddBatchResult:
            sOperation = new AddBatchResultOp();
            break;

        case OpId.XACommit:
            sOperation = new XACommitOp();
            break;

        case OpId.XACommitResult:
            sOperation = new XACommitResultOp();
            break;
            
        case OpId.XARollback:
            sOperation = new XARollbackOp();
            break;
            
        case OpId.XARollbackResult:
            sOperation = new XARollbackResultOp();
            break;
            
        case OpId.XAPrepare:
            sOperation = new XAPrepareOp();
            break;
            
        case OpId.XAPrepareResult:
            sOperation = new XAPrepareResultOp();
            break;
            
        case OpId.XAForget:
            sOperation = new XAForgetOp();
            break;

        case OpId.XAForgetResult:
            sOperation = new XAForgetResultOp();
            break;
            
        default:
            sOperation = null;
            break;
        }
        
        return sOperation;
    }

    /**
     * create new concrete result operation instance by request operation ID
     * 
     * @param aRequestOperationId       Request operation ID
     * @return                          Operation class instance
     */
    public Operation newResultOperation(byte aRequestOperationId)
    {
        byte sResultOperationId = OpId.None;
        
        switch (aRequestOperationId)
        {
        case OpId.CreateLinkerCtrlSession:
            sResultOperationId = OpId.CreateLinkerCtrlSessionResult;
            break;
            
        case OpId.CreateLinkerDataSession:
            sResultOperationId = OpId.CreateLinkerDataSessionResult;
            break;
            
        case OpId.DestroyLinkerDataSession:
            sResultOperationId = OpId.DestroyLinkerDataSessionResult;
            break;
            
        case OpId.RequestLinkerStatus:
            sResultOperationId = OpId.RequestLinkerStatusResult;
            break;
            
        case OpId.RequestPVInfoDBLinkAltiLinkerStatus:
            sResultOperationId = 
                    OpId.RequestPVInfoDBLinkAltiLinkerStatusResult;
            break;
            
        case OpId.PrepareLinkerShutdown:
            sResultOperationId = OpId.PrepareLinkerShutdownResult;
            break;
            
        case OpId.RequestCloseRemoteNodeSession:
            sResultOperationId = OpId.RequestCloseRemoteNodeSessionResult;
            break;
            
        case OpId.SetAutoCommitMode:
            sResultOperationId = OpId.SetAutoCommitModeResult;
            break;
            
        case OpId.CheckRemoteNodeSession:
            sResultOperationId = OpId.CheckRemoteNodeSessionResult;
            break;
            
        case OpId.Commit:
            sResultOperationId = OpId.CommitResult;
            break;
            
        case OpId.Rollback:
            sResultOperationId = OpId.RollbackResult;
            break;
            
        case OpId.Savepoint:
            sResultOperationId = OpId.SavepointResult;
            break;
            
        case OpId.AbortRemoteStatement:
            sResultOperationId = OpId.AbortRemoteStatementResult;
            break;
            
        case OpId.PrepareRemoteStatement:
            sResultOperationId = OpId.PrepareRemoteStatementResult;
            break;
            
        case OpId.RequestRemoteErrorInfo:
            sResultOperationId = OpId.RequestRemoteErrorInfoResult;
            break;
            
        case OpId.ExecuteRemoteStatement:
            sResultOperationId = OpId.ExecuteRemoteStatementResult;
            break;
            
        case OpId.RequestFreeRemoteStatement:
            sResultOperationId = OpId.RequestFreeRemoteStatementResult;
            break;
            
        case OpId.FetchResultSet:
            sResultOperationId = OpId.FetchResultRow;
            break;
            
        case OpId.RequestResultSetMeta:
            sResultOperationId = OpId.RequestResultSetMetaResult;
            break;
            
        case OpId.BindRemoteVariable:
            sResultOperationId = OpId.BindRemoteVariableResult;
            break;
            
        case OpId.RequestLinkerDump:
            sResultOperationId = OpId.RequestLinkerDumpResult;
            break;
            
        case OpId.PrepareRemoteStatementBatch:
            sResultOperationId = OpId.PrepareRemoteStatementBatchResult;
            break;
        	
        case OpId.RequestFreeRemoteStatementBatch:
            sResultOperationId = OpId.RequestFreeRemoteStatementBatchResult;
            break;
        	
        case OpId.ExecuteRemoteStatementBatch:
            sResultOperationId = OpId.ExecuteRemoteStatementBatchResult;
            break;
        	
        case OpId.PrepareAddBatch:
            sResultOperationId = OpId.PrepareAddBatchResult;
            break;
        	
        case OpId.AddBatch:
            sResultOperationId = OpId.AddBatchResult;
            break;
        	
        case OpId.XACommit:
            sResultOperationId = OpId.XACommitResult;
            break;
            
        case OpId.XARollback:
            sResultOperationId = OpId.XARollbackResult;
            break;
       
        case OpId.XAPrepare:
            sResultOperationId = OpId.XAPrepareResult;
            break;
            
        case OpId.XAForget:
            sResultOperationId = OpId.XAForgetResult;
            break;
            
        default:
            sResultOperationId = OpId.None;
            break;
        }
        
        return newOperation(sResultOperationId);
    }
}
