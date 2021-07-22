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

import com.altibase.altilinker.adlp.op.PrepareRemoteStatementOp;
import com.altibase.altilinker.adlp.op.PrepareRemoteStatementResultOp;
import com.altibase.altilinker.adlp.type.ResultCode;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.task.PrepareRemoteStatementTask;

public class PrepareRemoteStatementTaskTest extends TaskTestCase
{
    protected void setUp() throws Exception
    {
        super.setUp();
        
        mMainMediatorTest.runCreateLinkerCtrlSessionTask(null);
        mMainMediatorTest.runCreateLinkerDataSessionTask(LinkerDataSessionId);
        
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 integer)");
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
    }

    protected void tearDown() throws Exception
    {
        mSessionManager.closeRemoteNodeSession();

        runExecuteRemoteStatement("drop table t1");
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();

        super.tearDown();
    }

    public void testLinkerDataSession()
    {
        assertTrue(mSessionManager.isCreatedLinkerCtrlSession());
        assertTrue(mSessionManager.isCreatedLinkerDataSession(
                                                          LinkerDataSessionId));
        assertEquals(mSessionManager.getLinkerDataSessionCount(), 1);

        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        assertNotNull(sLinkerDataSession);
    }
    
    public void test()
    {
        PrepareRemoteStatementOp sOperation =
                new PrepareRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementString   = "insert into t1 values(?)";
        
        PrepareRemoteStatementTask sTask = new PrepareRemoteStatementTask(); 

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                PrepareRemoteStatementResultOp);
        
        PrepareRemoteStatementResultOp sResultOperation =
                (PrepareRemoteStatementResultOp)
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

        assertNotNull(sLinkerDataSession);
        assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 1);

        RemoteNodeSession sRemoteNodeSession =
                sLinkerDataSession.getRemoteNodeSession(
                        sResultOperation.mRemoteNodeSessionId);
        
        assertNotNull(sRemoteNodeSession);
        assertEquals(sRemoteNodeSession.getRemoteStatementCount(), 1);
        assertEquals(sRemoteNodeSession.getSessionId(),
                sResultOperation.mRemoteNodeSessionId);
        assertEquals(sRemoteNodeSession.getSessionState(),
                RemoteNodeSession.SessionState.Executed);
        assertFalse(sRemoteNodeSession.isExecuting());
        assertEquals(sRemoteNodeSession.getTask(),
                RemoteNodeSession.TaskType.Connection);
        assertEquals(sRemoteNodeSession.getTaskResult().getResult(),
                TaskResult.Success);
        
        RemoteStatement sRemoteStatement = 
                sLinkerDataSession.getRemoteStatement(RemoteStatementId);
        
        if (sRemoteStatement == null)
        {
            fail("sRemoteStatement is null.");
            return;
        }
        
        assertEquals(sRemoteStatement.getStatementId(),
                sResultOperation.mRemoteStatementId);

        sRemoteStatement = 
                sRemoteNodeSession.getRemoteStatement(RemoteStatementId);
        
        assertNotNull(sRemoteStatement);
        assertEquals(sRemoteStatement.getStatementId(),
                sResultOperation.mRemoteStatementId);
        assertEquals(sRemoteStatement.getStatementState(),
                RemoteStatement.StatementState.Executed);
        assertFalse(sRemoteStatement.isExecuting());
        assertEquals(sRemoteStatement.getTask(),
                RemoteStatement.TaskType.Prepare);
        assertEquals(sRemoteStatement.getTaskResult().getResult(),
                TaskResult.Success);
        assertEquals(sRemoteStatement.getStatementString(),
                sOperation.mStatementString);
    }

    public void testDualPrepare()
    {
        PrepareRemoteStatementOp sOperation =
                new PrepareRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementString   = "insert into t1 values(?)";
        
        PrepareRemoteStatementTask sTask = new PrepareRemoteStatementTask(); 

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        PrepareRemoteStatementResultOp sResultOperation =
                (PrepareRemoteStatementResultOp)
                        mMainMediatorTest.getResultOperation();

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        int sRemoteNodeSessionId1 = sResultOperation.mRemoteNodeSessionId;

        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        sResultOperation = 
                (PrepareRemoteStatementResultOp)
                        mMainMediatorTest.getResultOperation();

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        int sRemoteNodeSessionId2 = sResultOperation.mRemoteNodeSessionId;

        assertEquals(sRemoteNodeSessionId1, sRemoteNodeSessionId2);
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);
        
        assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 1);
        
        RemoteNodeSession sRemoteNodeSession =
                sLinkerDataSession.getRemoteNodeSession(
                        sResultOperation.mRemoteNodeSessionId);

        assertEquals(sRemoteNodeSession.getRemoteStatementCount(), 1);
    }
    
    public void testUnknownTarget()
    {
        PrepareRemoteStatementOp sOperation =
                new PrepareRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = "unknown_target";
        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementString   = "insert into t1 values(?)";
        
        PrepareRemoteStatementTask sTask = new PrepareRemoteStatementTask(); 

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.WrongPacket);
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);
        
        assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 0);
    }

    public void testInvalidUser()
    {
        PrepareRemoteStatementOp sOperation =
                new PrepareRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = "InvalidUser";
        sOperation.mTargetPassword    = "InvalidPassword";
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementString   = "insert into t1 values(?)";
        
        PrepareRemoteStatementTask sTask = new PrepareRemoteStatementTask(); 

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(),
                ResultCode.RemoteServerConnectionFail);

        PrepareRemoteStatementResultOp sResultOperation = 
                (PrepareRemoteStatementResultOp)
                        mMainMediatorTest.getResultOperation();

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);
        
        RemoteNodeSession sRemoteNodeSession =
                sLinkerDataSession.getRemoteNodeSession(
                        sResultOperation.mRemoteNodeSessionId);
        
        assertNotNull(sRemoteNodeSession);
        assertEquals(sRemoteNodeSession.getRemoteStatementCount(), 0);
        assertFalse(sRemoteNodeSession.isExecuting());
        assertEquals(sRemoteNodeSession.getTask(),
                RemoteNodeSession.TaskType.Connection);
        
        TaskResult sTaskResult = sRemoteNodeSession.getTaskResult();
        
        assertEquals(sTaskResult.getResult(), TaskResult.RemoteServerError);
        assertNotNull(sTaskResult.getRemoteServerError());
        assertNotNull(sTaskResult.getRemoteServerError().getSQLState());
    }
    
    public void testOnlyUserAccountChange()
    {
        PrepareRemoteStatementOp sOperation =
                new PrepareRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = "InvalidUser";
        sOperation.mTargetPassword    = "InvalidPassword";
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementString   = "insert into t1 values(?)";
        
        PrepareRemoteStatementTask sTask = new PrepareRemoteStatementTask(); 

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();

        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                PrepareRemoteStatementResultOp);
        
        PrepareRemoteStatementResultOp sResultOperation =
                (PrepareRemoteStatementResultOp)
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

        assertNotNull(sLinkerDataSession);
        assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 2);
    }
}
