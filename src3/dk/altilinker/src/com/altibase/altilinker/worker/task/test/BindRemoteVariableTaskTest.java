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
import com.altibase.altilinker.worker.task.BindRemoteVariableTask;

public class BindRemoteVariableTaskTest extends TaskTestCase
{
    protected void setUp() throws Exception
    {
        super.setUp();
        
        mMainMediatorTest.runCreateLinkerCtrlSessionTask(null);
        mMainMediatorTest.runCreateLinkerDataSessionTask(LinkerDataSessionId);
    }

    protected void tearDown() throws Exception
    {
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
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 integer)");
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("insert into t1 values(?)");

        //
        // 3. bind
        //
        {
            BindRemoteVariableOp sOperation = new BindRemoteVariableOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId  = RemoteStatementId;
            sOperation.mBindVariableIndex  = 0;
            sOperation.mBindVariableType   = AltibaseSQLType.SQL_VARCHAR;
            sOperation.mBindVariableString = "1234567890";
            
            BindRemoteVariableTask sTask = new BindRemoteVariableTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            assertNotNull(mMainMediatorTest.getResultOperation());
            assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

            assertTrue(mMainMediatorTest.getResultOperation() instanceof
                    BindRemoteVariableResultOp);
            
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

            assertNotNull(sLinkerDataSession);
            assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 1);

            RemoteStatement sRemoteStatement = 
                    sLinkerDataSession.getRemoteStatement(RemoteStatementId);

            if (sRemoteStatement == null)
            {
                fail("sRemoteStatement is null.");
                return;
            }
            
            assertEquals(sRemoteStatement.getStatementId(),
                    sResultOperation.mRemoteStatementId);

            assertEquals(sRemoteStatement.getStatementState(),
                    RemoteStatement.StatementState.Executed);
            assertFalse(sRemoteStatement.isExecuting());
            assertEquals(sRemoteStatement.getTask(),
                    RemoteStatement.TaskType.Bind);
            assertEquals(sRemoteStatement.getTaskResult().getResult(),
                    TaskResult.Success);
            
            short sRemoteNodeSessionId =
                    (short)sRemoteStatement.getRemoteNodeSessionId();
            assertTrue(sRemoteNodeSessionId != 0);
            
            RemoteNodeSession sRemoteNodeSession =
                    sLinkerDataSession.getRemoteNodeSession(sRemoteNodeSessionId);

            assertNotNull(sRemoteNodeSession);
            assertEquals(sRemoteNodeSession.getRemoteStatementCount(), 1);
            assertEquals(sRemoteNodeSession.getSessionId(),
                    sRemoteNodeSessionId);
            assertEquals(sRemoteNodeSession.getSessionState(),
                    RemoteNodeSession.SessionState.Executed);
            assertFalse(sRemoteNodeSession.isExecuting());
            assertEquals(sRemoteNodeSession.getTask(),
                    RemoteNodeSession.TaskType.Connection);
            assertEquals(sRemoteNodeSession.getTaskResult().getResult(),
                    TaskResult.Success);
        }

        //
        // 4. execute
        //
        execute(StatementType.ExecuteNonQueryStatement, null);

        //
        // 5. bind
        //
        bind(0, "1987654320");

        //
        // 6. execute
        //
        short sRemoteNodeSessionId =
                execute(StatementType.ExecuteNonQueryStatement, null);
        
        //
        // 7. check result
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
                java.sql.Statement sQueryStatement = sConnection.createStatement();
    
                boolean sIsQuery = sQueryStatement.execute("select * from t1");
                assertTrue(sIsQuery);
                
                java.sql.ResultSet sResultSet = sQueryStatement.getResultSet();
                
                assertTrue(sResultSet.next());
                
                int sValue = sResultSet.getInt(1);
                assertEquals(sValue, 1234567890);

                assertTrue(sResultSet.next());
                
                sValue = sResultSet.getInt(1);
                assertEquals(sValue, 1987654320);
                
                assertFalse(sResultSet.next());
            }
            catch (SQLException e)
            {
                mTraceLogger.log(e);
    
                fail(e.toString());
            }
        }
        
        //
        // 8. clean-up
        //
        runExecuteRemoteStatement("drop table t1");
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
    }

    public void testDataTypes()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        
        String sStatementString = "create table t1 (";
        sStatementString += "c00 char(10), ";
        sStatementString += "c01 varchar(10), ";
        sStatementString += "c02 smallint, ";
        sStatementString += "c03 integer, ";
        sStatementString += "c04 bigint, ";
        sStatementString += "c05 real, ";
        sStatementString += "c06 float, ";
        sStatementString += "c07 double, ";
        sStatementString += "c08 date, ";
        sStatementString += "c09 numeric(10,5), ";
        sStatementString += "c10 decimal(10,5), ";
        sStatementString += "c11 number";
        sStatementString += ")";
        
        runExecuteRemoteStatement(sStatementString);
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("insert into t1 values(?,?,?,?,?,?,?,?,?,?,?,?)");

        //
        // 3. bind
        //
        bind( 0, "abcdefg"); 
        bind( 1, "abcdefg");
        bind( 2, "666");
        bind( 3, "1234567890");
        bind( 4, "901234567890");
        bind( 5, "10.5");
        bind( 6, "5.11");
        bind( 7, "1000.12345");
        bind( 8, "31-JUL-2012");
        bind( 9, "1234.1234");
        bind(10, "4567.4567");
        bind(11, "9012.9012");

        //
        // 4. execute
        //
        short sRemoteNodeSessionId =
                execute(StatementType.ExecuteNonQueryStatement, null);

        //
        // 5. check result
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
                java.sql.Statement sQueryStatement = sConnection.createStatement();
    
                boolean sIsQuery = sQueryStatement.execute("select * from t1");
                assertTrue(sIsQuery);
                
                java.sql.ResultSet sResultSet = sQueryStatement.getResultSet();
                
                assertTrue(sResultSet.next());
                
                assertTrue(sResultSet.getString( 1).equals("abcdefg   "));
                assertTrue(sResultSet.getString( 2).equals("abcdefg"));
                assertTrue(sResultSet.getString( 3).equals("666"));
                assertTrue(sResultSet.getString( 4).equals("1234567890"));
                assertTrue(sResultSet.getString( 5).equals("901234567890"));
                assertTrue(sResultSet.getString( 6).equals("10.5"));
                assertTrue(sResultSet.getString( 7).equals("5.11"));
                assertTrue(sResultSet.getString( 8).equals("1000.12345"));
                assertTrue(sResultSet.getString( 9).equals("2012-07-31 00:00:00.0"));
                assertTrue(sResultSet.getString(10).equals("1234.1234"));
                assertTrue(sResultSet.getString(11).equals("4567.4567"));
                assertTrue(sResultSet.getString(12).equals("9012.9012"));

                assertFalse(sResultSet.next());
            }
            catch (SQLException e)
            {
                mTraceLogger.log(e);
    
                fail(e.toString());
            }
        }
        
        //
        // 6. clean-up
        //
        runExecuteRemoteStatement("drop table t1");
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
    }
}
