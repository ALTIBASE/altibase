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

import java.sql.SQLException;

import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.task.ExecuteRemoteStatementTask;

public class ExecuteRemoteStatementTaskTest extends TaskTestCase
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

    public void testRemoteExecuteImmediate()
    {
        ExecuteRemoteStatementOp sOperation = new ExecuteRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementType     = StatementType.RemoteExecuteImmediate;
        sOperation.mStatementString   = "insert into t1 values(1234567890)";
        
        ExecuteRemoteStatementTask sTask = new ExecuteRemoteStatementTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                ExecuteRemoteStatementResultOp);
        
        ExecuteRemoteStatementResultOp sResultOperation =
                (ExecuteRemoteStatementResultOp)
                        mMainMediatorTest.getResultOperation();
        
        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }

        assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);
        assertEquals(sResultOperation.mRemoteStatementId, RemoteStatementId);

        short sRemoteNodeSessionId = sResultOperation.mRemoteNodeSessionId;
        assertTrue(sRemoteNodeSessionId != 0);
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        assertNotNull(sLinkerDataSession);
        assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 1);
        
        RemoteNodeSession sRemoteNodeSession =
                sLinkerDataSession.getRemoteNodeSession(sRemoteNodeSessionId);

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
        
        assertNotNull(sRemoteStatement);
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
                RemoteStatement.TaskType.Execute);
        assertEquals(sRemoteStatement.getTaskResult().getResult(),
                TaskResult.Success);
        assertEquals(sRemoteStatement.getStatementString(),
                sOperation.mStatementString);
        
        java.sql.Connection sConnection =
                sRemoteNodeSession.getJdbcConnectionObject();
        
        try
        {
            java.sql.Statement sQueryStatement = sConnection.createStatement();

            boolean sIsQuery = sQueryStatement.execute("select * from t1");
            assertTrue(sIsQuery);
            
            java.sql.ResultSet sResultSet = sQueryStatement.getResultSet();
            
            boolean sNext = sResultSet.next();
            assertTrue(sNext);
            
            int sValue = sResultSet.getInt(1);
            assertEquals(sValue, 1234567890);
        }
        catch (SQLException e)
        {
            mTraceLogger.log(e);

            fail(e.toString());
        }
    }

    public void testRemoteExecuteImmediateFail()
    {
        ExecuteRemoteStatementOp sOperation = new ExecuteRemoteStatementOp();
        
        sOperation.setSessionId(LinkerDataSessionId);
        
        sOperation.mTargetName        = mMainMediatorTest.getTargetName();
        sOperation.mTargetUser        = mMainMediatorTest.getTargetUser();
        sOperation.mTargetPassword    = mMainMediatorTest.getTargetPassword();
        sOperation.mRemoteStatementId = RemoteStatementId;
        sOperation.mStatementType     = StatementType.RemoteExecuteImmediate;
        sOperation.mStatementString   = "insert into t1 values('test')";
        
        ExecuteRemoteStatementTask sTask = new ExecuteRemoteStatementTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(),
                ResultCode.RemoteServerError);

        assertTrue(mMainMediatorTest.getResultOperation() instanceof
                ExecuteRemoteStatementResultOp);
        
        ExecuteRemoteStatementResultOp sResultOperation =
                (ExecuteRemoteStatementResultOp)
                        mMainMediatorTest.getResultOperation();

        if (sResultOperation == null)
        {
            fail("sResultOperation is null.");
            return;
        }
        
        assertEquals(sResultOperation.mExecuteResult, -1);
    }
    
    public void testExecuteNonQueryStatement()
    {
        //
        // 1. prepare
        //
        prepare("insert into t1 values(1234567890)");

        // 
        // 2. execute
        //
        short sRemoteNodeSessionId =
                execute(StatementType.ExecuteNonQueryStatement, null);
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        RemoteNodeSession sRemoteNodeSession =
                sLinkerDataSession.getRemoteNodeSession(sRemoteNodeSessionId);
        
        java.sql.Connection sConnection =
                sRemoteNodeSession.getJdbcConnectionObject();
        
        try
        {
            java.sql.Statement sQueryStatement = sConnection.createStatement();

            boolean sIsQuery = sQueryStatement.execute("select * from t1");
            assertTrue(sIsQuery);
            
            java.sql.ResultSet sResultSet = sQueryStatement.getResultSet();
            
            assertTrue(sResultSet.next());
            
            int sValue = sResultSet.getInt(1);
            assertEquals(sValue, 1234567890);
        }
        catch (SQLException e)
        {
            mTraceLogger.log(e);

            fail(e.toString());
        }
    }

    public void testExecuteQueryStatement()
    {
        //
        // 1. insert
        //
        runExecuteRemoteStatement("insert into t1 values(123)");
        runExecuteRemoteStatement("insert into t1 values(456)");
        runExecuteRemoteStatement("insert into t1 values(789)");
        
        //
        // 2. prepare
        //
        prepare("select * from t1 where c1 = 456");

        // 
        // 3. execute
        //
        execute(StatementType.ExecuteQueryStatement, null);
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        RemoteStatement sRemoteStatement = 
                sLinkerDataSession.getRemoteStatement(RemoteStatementId);

        if (sRemoteStatement == null)
        {
            fail("sRemoteStatement is null.");
            return;
        }
        
        assertEquals(sRemoteStatement.getTask(), RemoteStatement.TaskType.Execute);
        
        try
        {
            java.sql.Statement sQueryStatement = 
                    sRemoteStatement.getJdbcStatementObject();

            java.sql.ResultSet sResultSet = sQueryStatement.getResultSet();
            
            assertTrue(sResultSet.next());
            
            int sValue = sResultSet.getInt(1);
            assertTrue(sValue != 123);
            assertEquals(sValue, 456);
            assertTrue(sValue != 789);

            assertFalse(sResultSet.next());
        }
        catch (SQLException e)
        {
            mTraceLogger.log(e);

            fail(e.toString());
        }
    }

    public void testRemoteTable()
    {
        //
        // 1. insert
        //
        runExecuteRemoteStatement("insert into t1 values(123)");
        runExecuteRemoteStatement("insert into t1 values(456)");
        runExecuteRemoteStatement("insert into t1 values(789)");
        
        //
        // 2. prepare
        //
        prepare("select * from t1");

        // 
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        RemoteStatement sRemoteStatement = 
                sLinkerDataSession.getRemoteStatement(RemoteStatementId);

        if (sRemoteStatement == null)
        {
            fail("sRemoteStatement is null.");
            return;
        }
        
        assertEquals(sRemoteStatement.getTask(), RemoteStatement.TaskType.Execute);
        
        try
        {
            java.sql.Statement sQueryStatement = 
                    sRemoteStatement.getJdbcStatementObject();

            java.sql.ResultSet sResultSet = sQueryStatement.getResultSet();
            
            assertTrue(sResultSet.next());
            assertEquals(sResultSet.getInt(1), 123);

            assertTrue(sResultSet.next());
            assertEquals(sResultSet.getInt(1), 456);

            assertTrue(sResultSet.next());
            assertEquals(sResultSet.getInt(1), 789);

            assertFalse(sResultSet.next());
        }
        catch (SQLException e)
        {
            mTraceLogger.log(e);

            fail(e.toString());
        }
    }
}
