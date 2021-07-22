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
 
package com.altibase.altilinker.worker;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.OpId;
import com.altibase.altilinker.worker.task.*;

/**
 * Task factory static class to create concrete Task object by operation ID
 */
public class TaskFactory
{
    private static TaskFactory mTaskFactory = new TaskFactory();
    
    /**
     * Constructor
     */
    private TaskFactory()
    {
        // nothing to do
    }
    
    /**
     * Get static instance
     * 
     * @return Static instance
     */
    public static TaskFactory getInstance()
    {
        return mTaskFactory;
    }
    
    /**
     * Create new concrete Task object by operation ID
     * 
     * @param aOperationId      Operation ID
     * @return                  New Task object
     */
    public Task newTask(int aOperationId)
    {
        Task sTask = null;
        
        switch (aOperationId)
        {
        case OpId.CreateLinkerCtrlSession:
            sTask = new CreateLinkerCtrlSessionTask();
            break;
            
        case OpId.CreateLinkerDataSession:
            sTask = new CreateLinkerDataSessionTask();
            break;
            
        case OpId.DestroyLinkerDataSession:
            sTask = new DestroyLinkerDataSessionTask();
            break;
            
        case OpId.RequestLinkerStatus:
            sTask = new RequestLinkerStatusTask();
            break;
            
        case OpId.RequestPVInfoDBLinkAltiLinkerStatus:
            sTask = new RequestPVInfoDBLinkAltiLinkerStatusTask();
            break;
            
        case OpId.PrepareLinkerShutdown:
            sTask = new PrepareLinkerShutdownTask();
            break;
            
        case OpId.DoLinkerShutdown:
            sTask = new DoLinkerShutdownTask();
            break;
            
        case OpId.RequestCloseRemoteNodeSession:
            sTask = new RequestCloseRemoteNodeSessionTask();
            break;
            
        case OpId.SetAutoCommitMode:
            sTask = new SetAutoCommitModeTask();
            break;
            
        case OpId.CheckRemoteNodeSession:
            sTask = new CheckRemoteNodeSessionTask();
            break;
            
        case OpId.Commit:
            sTask = new CommitTask();
            break;
            
        case OpId.Rollback:
            sTask = new RollbackTask();
            break;
            
        case OpId.Savepoint:
            sTask = new SavepointTask();
            break;
            
        case OpId.AbortRemoteStatement:
            sTask = new AbortRemoteStatementTask();
            break;
            
        case OpId.PrepareRemoteStatement:
            sTask = new PrepareRemoteStatementTask();            
            break;
            
        case OpId.RequestRemoteErrorInfo:
            sTask = new RequestRemoteErrorInfoTask();
            break;
            
        case OpId.ExecuteRemoteStatement:
            sTask = new ExecuteRemoteStatementTask();
            break;
            
        case OpId.RequestFreeRemoteStatement:
            sTask = new RequestFreeRemoteStatementTask();
            break;
            
        case OpId.FetchResultSet:
            sTask = new FetchResultSetTask();
            break;
            
        case OpId.RequestResultSetMeta:
            sTask = new RequestResultSetMetaTask();
            break;
            
        case OpId.BindRemoteVariable:
            sTask = new BindRemoteVariableTask();
            break;
            
        case OpId.RequestLinkerDump:
            sTask = new RequestLinkerDumpTask();
            break;
            
        case OpId.PrepareRemoteStatementBatch:
        	sTask = new PrepareRemoteStatementBatchTask();
        	break;
        	
        case OpId.RequestFreeRemoteStatementBatch:
        	sTask = new RequestFreeRemoteStatementBatchTask();
        	break;
            
        case OpId.ExecuteRemoteStatementBatch:
        	sTask = new ExecuteRemoteStatementBatchTask();
        	break;
        	
        case OpId.PrepareAddBatch:
        	sTask = new PrepareAddBatchTask();
        	break;
   
        case OpId.AddBatch:
        	sTask = new AddBatchTask();
        	break;
        	
        case OpId.XAPrepare:
            sTask = new XAPrepareTask();
            break;
            
        case OpId.XACommit:
            sTask = new XACommitTask();
            break;
            
        case OpId.XARollback:
            sTask = new XARollbackTask();
            break;
            
        case OpId.XAForget:
            sTask = new XAForgetTask();
            break;
            
        default:
            sTask = null;
            break;
        }
        
        return sTask;
    }
}
