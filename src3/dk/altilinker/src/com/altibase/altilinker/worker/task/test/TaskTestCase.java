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
 
package com.altibase.altilinker.worker.task.test;

import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.*;
import com.altibase.altilinker.worker.task.BindRemoteVariableTask;
import com.altibase.altilinker.worker.task.CommitTask;
import com.altibase.altilinker.worker.task.ExecuteRemoteStatementTask;
import com.altibase.altilinker.worker.task.PrepareRemoteStatementTask;
import com.altibase.altilinker.worker.task.RollbackTask;
import com.altibase.altilinker.worker.task.SavepointTask;
import com.altibase.altilinker.worker.task.SetAutoCommitModeTask;

import junit.framework.TestCase;

public abstract class TaskTestCase extends TestCase
{
    protected MainMediatorTest mMainMediatorTest = null;
    protected SessionManager   mSessionManager   = null;
    protected TraceLogger      mTraceLogger      = TraceLogger.getInstance();

    protected static final int LinkerDataSessionId = 100; 
    protected static final int RemoteStatementId   = 500;

    protected void setUp() throws Exception
    {
        mMainMediatorTest = new MainMediatorTest();
        
        mSessionManager   = mMainMediatorTest.getSessionManager();
    }

    protected void tearDown() throws Exception
    {
        mMainMediatorTest.close();

        mMainMediatorTest = null;
        mSessionManager   = null;
    }

    protected void runExecuteRemoteStatement(String aStatementString)
    {
        String sTargetName     = mMainMediatorTest.getTargetName();
        String sTargetUser     = mMainMediatorTest.getTargetUser();
        String sTargetPassword = mMainMediatorTest.getTargetPassword();
        
        mMainMediatorTest.runExecuteRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId,
                sTargetName,
                sTargetUser,
                sTargetPassword,
                aStatementString);
    }

    protected void prepare(String aStatementString)
    {
        PrepareRemoteStatementOp sOperation =
                new PrepareRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementString   = aStatementString;
        
        PrepareRemoteStatementTask sTask = new PrepareRemoteStatementTask(); 

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        RemoteStatement sRemoteStatement = 
                sLinkerDataSession.getRemoteStatement(RemoteStatementId);

        if (sRemoteStatement == null)
        {
            fail("sRemoteStatement is null.");
            return;
        }
        
        assertEquals(sRemoteStatement.getTask(), RemoteStatement.TaskType.Prepare);
    }
    
    protected void bind(int aBindIndex, String aBindVariable)
    {
        BindRemoteVariableOp sOperation = new BindRemoteVariableOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mRemoteStatementId  = RemoteStatementId;
        sOperation.mBindVariableIndex  = aBindIndex;
        sOperation.mBindVariableType   = AltibaseSQLType.SQL_VARCHAR;
        sOperation.mBindVariableString = aBindVariable;
        
        BindRemoteVariableTask sTask = new BindRemoteVariableTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        BindRemoteVariableResultOp sResultOperation =
                (BindRemoteVariableResultOp)
                        mMainMediatorTest.getResultOperation(); 

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);
        assertEquals(sResultOperation.mRemoteStatementId, RemoteStatementId);

        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        RemoteStatement sRemoteStatement = 
                sLinkerDataSession.getRemoteStatement(RemoteStatementId);

        if (sRemoteStatement == null)
        {
            fail("sRemoteStatement is null.");
            return;
        }
        
        assertEquals(sRemoteStatement.getTask(), RemoteStatement.TaskType.Bind);
    }

    protected short execute(int aStatementType, String aStatementString)
    {
        short sRemoteNodeSessionId = 0;
        
        ExecuteRemoteStatementOp sOperation = new ExecuteRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementType     = aStatementType;
        sOperation.mStatementString   = aStatementString;
        
        ExecuteRemoteStatementTask sTask = new ExecuteRemoteStatementTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        ExecuteRemoteStatementResultOp sResultOperation =
                (ExecuteRemoteStatementResultOp)
                        mMainMediatorTest.getResultOperation(); 

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return 0;
        }
        
        sRemoteNodeSessionId = sResultOperation.mRemoteNodeSessionId;
        assertTrue(sRemoteNodeSessionId != 0);
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        RemoteStatement sRemoteStatement = 
                sLinkerDataSession.getRemoteStatement(RemoteStatementId);

        if (sRemoteStatement == null)
        {
            fail("sRemoteStatement is null.");
            return 0;
        }
        
        assertEquals(sRemoteStatement.getTask(), RemoteStatement.TaskType.Execute);
        
        return sRemoteNodeSessionId;
    }
    
    protected void setAutoCommitMode(boolean aAutoCommitMode)
    {
        SetAutoCommitModeOp sOperation = new SetAutoCommitModeOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mAutoCommitMode = aAutoCommitMode;
        
        SetAutoCommitModeTask sTask = new SetAutoCommitModeTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                SetAutoCommitModeResultOp);
        
        SetAutoCommitModeResultOp sResultOperation =
                (SetAutoCommitModeResultOp)
                        mMainMediatorTest.getResultOperation(); 

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);

        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        assertEquals(sLinkerDataSession.getAutoCommitMode(), aAutoCommitMode);
    }
    
    protected void commit()
    {
        CommitOp sOperation = new CommitOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        CommitTask sTask = new CommitTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                CommitResultOp);
        
        CommitResultOp sResultOperation =
                (CommitResultOp)
                        mMainMediatorTest.getResultOperation(); 

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);
    }
    
    protected void rollback(String aSpecificSavepointName,
                            short[] aRemoteNodeSessions)
    {
        RollbackOp sOperation = new RollbackOp();
        
        if (aSpecificSavepointName == null)
        {
            sOperation.mRollbackFlag = RollbackFlag.Transaction;
        }
        else
        {
            sOperation.mRollbackFlag       = RollbackFlag.SpecificSavepoint;
            sOperation.mSavepointName      = aSpecificSavepointName;
            sOperation.mRemoteNodeSessions = aRemoteNodeSessions;
        }
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        RollbackTask sTask = new RollbackTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                RollbackResultOp);
        
        RollbackResultOp sResultOperation =
                (RollbackResultOp)
                        mMainMediatorTest.getResultOperation(); 

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);
    }

    protected void savepoint(String aSavepointName)
    {
        SavepointOp sOperation = new SavepointOp();
        
        sOperation.mSavepointName = aSavepointName;
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        SavepointTask sTask = new SavepointTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                SavepointResultOp);
        
        SavepointResultOp sResultOperation =
                (SavepointResultOp)
                        mMainMediatorTest.getResultOperation(); 

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);
    }
}
