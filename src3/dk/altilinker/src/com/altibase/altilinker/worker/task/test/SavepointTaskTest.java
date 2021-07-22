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
import java.util.Iterator;
import java.util.LinkedList;

import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.JdbcDriverManager;
import com.altibase.altilinker.session.*;

public class SavepointTaskTest extends TaskTestCase
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

    public void testSavepoint()
    {
        //
        // 1. change auto-commit off mode
        //
        setAutoCommitMode(false);
        
        //
        // 2. savepoint
        //
        int sStatementType = StatementType.RemoteExecuteImmediate;
        
        short sRemoteNodeSessionId =
                execute(sStatementType, "insert into t1 values(100)");
        
        savepoint("sp1");

        execute(sStatementType, "insert into t1 values(100)");

        savepoint("sp2");

        execute(sStatementType, "insert into t1 values(100)");

        savepoint("sp3");
        
        //
        // 3. check result
        //
        LinkerDataSession sLinkerDataSession = 
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);
        
        RemoteNodeSession sRemoteNodeSession =
                sLinkerDataSession.getRemoteNodeSession(sRemoteNodeSessionId);

        LinkedList sSavepointList;
        Iterator sIterator;
        java.sql.Savepoint sSavepoint;
        
        sSavepointList = sRemoteNodeSession.getJdbcSavepointObject();
        sIterator = sSavepointList.iterator();

        try
        {
            assertTrue(sIterator.hasNext());
            sSavepoint = (java.sql.Savepoint)sIterator.next();
            assertTrue(sSavepoint.getSavepointName().equals("sp1"));

            assertTrue(sIterator.hasNext());
            sSavepoint = (java.sql.Savepoint)sIterator.next();
            assertTrue(sSavepoint.getSavepointName().equals("sp2"));

            assertTrue(sIterator.hasNext());
            sSavepoint = (java.sql.Savepoint)sIterator.next();
            assertTrue(sSavepoint.getSavepointName().equals("sp3"));
        }
        catch (SQLException e)
        {
            fail(e.toString());
        }
    }

    public void testRollbackToSpecificSavepoint()
    {
        //
        // 1. change auto-commit off mode
        //
        setAutoCommitMode(false);
        
        //
        // 2. savepoint -> rollback to a specific savepoint -> commit
        //
        int sStatementType = StatementType.RemoteExecuteImmediate;
        
        short sRemoteNodeSessionId =
                execute(sStatementType, "insert into t1 values(100)");
        
        savepoint("sp1");

        execute(sStatementType, "insert into t1 values(101)");
        execute(sStatementType, "insert into t1 values(102)");

        savepoint("sp2");
        
        execute(sStatementType, "insert into t1 values(103)");
        execute(sStatementType, "insert into t1 values(104)");
        execute(sStatementType, "insert into t1 values(105)");
        
        rollback("sp1", new short[] { sRemoteNodeSessionId });

        execute(sStatementType, "insert into t1 values(106)");
        execute(sStatementType, "insert into t1 values(107)");
        execute(sStatementType, "insert into t1 values(108)");
        
        commit();
        
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

                    assertTrue(sResultSet.next());
                    assertEquals(sResultSet.getInt(1), 106);

                    assertTrue(sResultSet.next());
                    assertEquals(sResultSet.getInt(1), 107);

                    assertTrue(sResultSet.next());
                    assertEquals(sResultSet.getInt(1), 108);

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
