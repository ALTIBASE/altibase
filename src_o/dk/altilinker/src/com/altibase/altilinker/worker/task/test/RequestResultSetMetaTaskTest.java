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
import com.altibase.altilinker.worker.task.RequestResultSetMetaTask;

public class RequestResultSetMetaTaskTest extends TaskTestCase
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

        String sStatementString =
                "create table t1 (" +
                "c00 char(10), " +
                "c01 varchar(10), " +
                "c02 smallint, " +
                "c03 integer, " +
                "c04 bigint, " +
                "c05 real, " +
                "c06 float, " +
                "c07 double, " +
                "c08 date, " +
                "c09 numeric(10, 9), " +
                "c10 decimal(38,-84), " +
                "c11 number" +
                ")";
        
        runExecuteRemoteStatement(sStatementString);
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");

        //
        // 3. get meta data of result set
        //
        RequestResultSetMetaResultOp sResultOperation = null;
        
        {
            RequestResultSetMetaOp sOperation = new RequestResultSetMetaOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            
            RequestResultSetMetaTask sTask = new RequestResultSetMetaTask();

            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();

            assertNotNull(mMainMediatorTest.getResultOperation());
            assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

            assertTrue(mMainMediatorTest.getResultOperation() instanceof
                    RequestResultSetMetaResultOp);
            
            sResultOperation =
                    (RequestResultSetMetaResultOp)
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
        // 4. check result 
        //
        {
            assertEquals(sResultOperation.mColumnMeta.length, 12);

            RequestResultSetMetaResultOp.ColumnMeta sColumnMeta;
            
            sColumnMeta = sResultOperation.mColumnMeta[0];
            assertTrue(sColumnMeta.mColumnName.equals("C00"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_CHAR);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 10);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[1];
            assertTrue(sColumnMeta.mColumnName.equals("C01"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_VARCHAR);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 10);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[2];
            assertTrue(sColumnMeta.mColumnName.equals("C02"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_SMALLINT);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 0);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[3];
            assertTrue(sColumnMeta.mColumnName.equals("C03"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_INTEGER);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 0);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[4];
            assertTrue(sColumnMeta.mColumnName.equals("C04"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_BIGINT);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 0);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[5];
            assertTrue(sColumnMeta.mColumnName.equals("C05"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_REAL);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 0);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[6];
            assertTrue(sColumnMeta.mColumnName.equals("C06"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_FLOAT);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 38);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[7];
            assertTrue(sColumnMeta.mColumnName.equals("C07"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_DOUBLE);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 0);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[8];
            assertTrue(sColumnMeta.mColumnName.equals("C08"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_TIMESTAMP);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 0);
            assertEquals(sColumnMeta.mScale, 0);

            sColumnMeta = sResultOperation.mColumnMeta[9];
            assertTrue(sColumnMeta.mColumnName.equals("C09"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_NUMERIC);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 38);
            assertEquals(sColumnMeta.mScale, 20);

            sColumnMeta = sResultOperation.mColumnMeta[10];
            assertTrue(sColumnMeta.mColumnName.equals("C10"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_NUMERIC);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 38);
            assertEquals(sColumnMeta.mScale, 20);

            sColumnMeta = sResultOperation.mColumnMeta[11];
            assertTrue(sColumnMeta.mColumnName.equals("C11"));
            assertEquals(sColumnMeta.mColumnType, AltibaseSQLType.SQL_FLOAT);
            assertEquals(sColumnMeta.mNullable, Nullable.Nullable);
            assertEquals(sColumnMeta.mPrecision, 38);
            assertEquals(sColumnMeta.mScale, 0);
        }
        
        //
        // 5. clean-up
        //
        runExecuteRemoteStatement("drop table t1");
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
    }
}
