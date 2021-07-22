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
import com.altibase.altilinker.jdbc.*;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.task.SetAutoCommitModeTask;

public class SetAutoCommitModeTaskTest extends TaskTestCase
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
    
    public void testAutoCommitModeOn()
    {
        //
        // 1. change auto-commit mode
        //
        {
            SetAutoCommitModeOp sOperation = new SetAutoCommitModeOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mAutoCommitMode = true;
            
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

            assertTrue(sLinkerDataSession.getAutoCommitMode());
            assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 0);
        }
        
        //
        // 2. new connection
        //
        short sRemoteNodeSessionId = 0;
        
        {
            String sStatementString = "insert into t1 values(100)";
            
            sRemoteNodeSessionId =
                    execute(StatementType.RemoteExecuteImmediate, sStatementString);
    
            LinkerDataSession sLinkerDataSession =
                    mSessionManager.getLinkerDataSession(LinkerDataSessionId);
    
            assertTrue(sLinkerDataSession.getAutoCommitMode());
            assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 1);

            RemoteNodeSession sRemoteNodeSession =
                    sLinkerDataSession.getRemoteNodeSession(
                            sRemoteNodeSessionId);
            
            assertNotNull(sRemoteNodeSession);

            java.sql.Connection sConnection =
                    sRemoteNodeSession.getJdbcConnectionObject();
            
            try
            {
                assertTrue(sConnection.getAutoCommit());
            }
            catch (SQLException e)
            {
                fail(e.toString());
            }
        }
        
        //
        // 3. check result
        //
        {
            try
            {
                java.sql.Connection sConnection = 
                        JdbcDriverManager.getConnection(
                                mMainMediatorTest.getTarget());
                
                if (sConnection != null)
                {
                    java.sql.Statement sStatement = sConnection.createStatement();
                
                    boolean sIsQuery = sStatement.execute("select * from t1");
                    assertTrue(sIsQuery);
                
                    java.sql.ResultSet sResultSet = sStatement.getResultSet();
                    assertNotNull(sResultSet);

                    assertTrue(sResultSet.next());
                    assertEquals(sResultSet.getInt(1), 100);
                
                    assertFalse(sResultSet.next());
                
                    sStatement.close();
                    sConnection.close();
                }
            }
            catch (SQLException e)
            {
                fail(e.toString());
            }
        }
    }

    public void testAutoCommitModeOff()
    {
        //
        // 1. change auto-commit mode
        //
        {
            SetAutoCommitModeOp sOperation = new SetAutoCommitModeOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mAutoCommitMode = false;
            
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

            assertFalse(sLinkerDataSession.getAutoCommitMode());
            assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 0);
        }
        
        //
        // 2. new connection
        //
        short sRemoteNodeSessionId = 0;
        
        {
            String sStatementString = "insert into t1 values(100)";
            
            sRemoteNodeSessionId =
                    execute(StatementType.RemoteExecuteImmediate, sStatementString);
    
            LinkerDataSession sLinkerDataSession =
                    mSessionManager.getLinkerDataSession(LinkerDataSessionId);
    
            assertFalse(sLinkerDataSession.getAutoCommitMode());
            assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 1);

            RemoteNodeSession sRemoteNodeSession =
                    sLinkerDataSession.getRemoteNodeSession(
                            sRemoteNodeSessionId);
            
            assertNotNull(sRemoteNodeSession);

            java.sql.Connection sConnection =
                    sRemoteNodeSession.getJdbcConnectionObject();
            
            try
            {
                assertFalse(sConnection.getAutoCommit());
            }
            catch (SQLException e)
            {
                fail(e.toString());
            }
        }
        
        //
        // 3. check result
        //
        {
            try
            {
                java.sql.Connection sConnection = 
                        JdbcDriverManager.getConnection(
                                mMainMediatorTest.getTarget());
                
                if (sConnection != null)
                {
                    java.sql.Statement sStatement = sConnection.createStatement();
                
                    boolean sIsQuery = sStatement.execute("select * from t1");
                    assertTrue(sIsQuery);
                
                    java.sql.ResultSet sResultSet = sStatement.getResultSet();
                    assertNotNull(sResultSet);

                    assertFalse(sResultSet.next());
                
                    sStatement.close();
                    sConnection.close();
                }
            }
            catch (SQLException e)
            {
                fail(e.toString());
            }
        }
        
        //
        // 4. commit
        //
        {
            LinkerDataSession sLinkerDataSession =
                    mSessionManager.getLinkerDataSession(LinkerDataSessionId);

            RemoteNodeSession sRemoteNodeSession =
                    sLinkerDataSession.getRemoteNodeSession(sRemoteNodeSessionId);
            
            java.sql.Connection sConnection =
                    sRemoteNodeSession.getJdbcConnectionObject();
            
            try
            {
                sConnection.commit();
            }
            catch (SQLException e)
            {
                fail(e.toString());
            }
        }

        //
        // 5. check result
        //
        {
            try
            {
                java.sql.Connection sConnection = 
                        JdbcDriverManager.getConnection(
                                mMainMediatorTest.getTarget());
                
                if (sConnection != null)
                {
                    java.sql.Statement sStatement = sConnection.createStatement();
                
                    boolean sIsQuery = sStatement.execute("select * from t1");
                    assertTrue(sIsQuery);
                
                    java.sql.ResultSet sResultSet = sStatement.getResultSet();
                    assertNotNull(sResultSet);

                    assertTrue(sResultSet.next());
                    assertEquals(sResultSet.getInt(1), 100);
                
                    assertFalse(sResultSet.next());
                
                    sStatement.close();
                    sConnection.close();
                }
            }
            catch (SQLException e)
            {
                fail(e.toString());
            }
        }
    }
}
