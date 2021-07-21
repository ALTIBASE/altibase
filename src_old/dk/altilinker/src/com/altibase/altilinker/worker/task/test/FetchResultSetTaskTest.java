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

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.LinkedList;

import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.JdbcDataManager;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.worker.task.FetchResultSetTask;

public class FetchResultSetTaskTest extends TaskTestCase
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
    
    private LinkedList checkResultOperationForSimple()
    {
        LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
        assertTrue(sFetchResultRowList.size() != 0);

        Iterator sIterator = sFetchResultRowList.iterator();
        assertTrue(sIterator.hasNext());

        FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

        assertFalse(sIterator.hasNext());
        
        assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

        assertEquals(sFetchResultRowOp.getSessionId(), LinkerDataSessionId);

        LinkerDataSession sLinkerDataSession =
                mSessionManager.getLinkerDataSession(LinkerDataSessionId);

        assertNotNull(sLinkerDataSession);
        assertEquals(sLinkerDataSession.getRemoteNodeSessionCount(), 1);

        RemoteStatement sRemoteStatement = 
                sLinkerDataSession.getRemoteStatement(RemoteStatementId);

        if (sRemoteStatement == null)
        {
            fail("sRemoteStatement is null.");
            return null;
        }

        assertEquals(sRemoteStatement.getStatementId(),
                sFetchResultRowOp.mRemoteStatementId);

        assertEquals(sRemoteStatement.getStatementState(),
                RemoteStatement.StatementState.Executed);
        assertFalse(sRemoteStatement.isExecuting());
        assertEquals(sRemoteStatement.getTask(),
                RemoteStatement.TaskType.Fetch);
        assertEquals(sRemoteStatement.getTaskResult().getResult(),
                TaskResult.Success);
        
        short sRemoteNodeSessionId =
                (short)sRemoteStatement.getRemoteNodeSessionId();
        
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
        
        return sFetchResultRowList;
    }

    public void testSimple()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 integer)");
        
        int i;
        final int sInsertRowCount = 10;
        
        for (i = 1; i <= sInsertRowCount; ++i)
        {
            runExecuteRemoteStatement("insert into t1 values (" + i + ")");
        }
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");
        
        //
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        //
        // 4. fetch
        //
        {
            // 4-1. request to fetch for 10 records
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = 10;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();

            checkResultOperationForSimple();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 1);

            Iterator sIterator = sFetchResultRowList.iterator();
            sIterator.hasNext();

            FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

            assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

            assertNotNull(sFetchResultRowOp.mRowDataBuffer);

            int sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
            assertEquals(sRecordCount, 10);
            
            for (i = 1; i <= sRecordCount; ++i)
            {
                int sColumnCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                assertEquals(sColumnCount, 1);

                int j;
                for (j = 0; j < sColumnCount; ++j)
                {
                    int sColumnValueLength = sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnValueLength, 4);
                    
                    int sColumnValue = sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnValue, i);
                    
//                    int sColumnValueLength = sFetchResultRowOp.mRowDataBuffer.getInt();
//                    
//                    byte[] sColumnValue = new byte[sColumnValueLength];
//                    sFetchResultRowOp.mRowDataBuffer.get(sColumnValue);
//                    String sColumnValueString = new String(sColumnValue);
//                    assertTrue(sColumnValueString.compareTo(new Integer(i).toString()) == 0);
                }
            }

            // 4-2. request to fetch for 10 records
            sTask = new FetchResultSetTask();
            
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);

            mMainMediatorTest.reset();
            sTask.run();
            
            sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            assertTrue(sFetchResultRowList.size() == 1);

            sIterator = sFetchResultRowList.iterator();
            assertTrue(sIterator.hasNext());

            sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

            assertFalse(sIterator.hasNext());
            
            assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.EndOfFetch);
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

    public void test10of100()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 integer)");

        int i;
        final int sInsertRowCount = 100;
        
        for (i = 1; i <= sInsertRowCount; ++i)
        {
            runExecuteRemoteStatement("insert into t1 values (" + i + ")");
        }
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");
        
        //
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        //
        // 4. fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = 10;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 1);

            Iterator sIterator = sFetchResultRowList.iterator();
            sIterator.hasNext();

            FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

            assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

            assertNotNull(sFetchResultRowOp.mRowDataBuffer);

            int sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
            assertEquals(sRecordCount, 10);
            
            for (i = 1; i <= sRecordCount; ++i)
            {
                int sColumnCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                assertEquals(sColumnCount, 1);

                int j;
                for (j = 0; j < sColumnCount; ++j)
                {
                    int sColumnValueLength = sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnValueLength, 4);
                    
                    int sColumnValue = sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnValue, i);
                }
            }
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

    public void testFragmentedByRecord()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 bigint)");

        int i;
        final int sInsertRowCount = 64 * 1024 / 8;
        
        for (i = 1; i <= sInsertRowCount; ++i)
        {
            runExecuteRemoteStatement("insert into t1 values (" + i + ")");
        }
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");
        
        //
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        //
        // 4. fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = -1;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 6);

            int sRowNo = 1;
            int sTotalRecordCount = 0;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;
                
                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 1);
    
                    int j;
                    for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength = sFetchResultRowOp.mRowDataBuffer.getInt();
                        assertEquals(sColumnValueLength, 8);
                        
                        long sColumnValue = sFetchResultRowOp.mRowDataBuffer.getLong();
                        assertEquals(sColumnValue, sRowNo);
                        
                        ++sRowNo;
                    }
                }
            }
            
            assertEquals(sTotalRecordCount, sInsertRowCount);
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

    public void testFragmentedByColumn()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 char(101))");

        int i;
        final int sInsertRowCount = 64 * 1024 / 100;
        
        for (i = 1; i <= sInsertRowCount; ++i)
        {
            runExecuteRemoteStatement("insert into t1 values ('test" + i + "')");
        }
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");
        
        //
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        //
        // 4. fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = -1;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 4);

            int sRowNo = 1;
            int sTotalRecordCount = 0;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();
                
                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;

                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 1);
    
                    int j;
                    for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength = sFetchResultRowOp.mRowDataBuffer.getInt();
                        assertEquals(sColumnValueLength, 101);
                        
                        String sColumnValue = 
                                getStringColumnValue(
                                        sFetchResultRowOp.mRowDataBuffer,
                                        sColumnValueLength);
                        
                        assertTrue(sColumnValue.equals("test" + sRowNo));
                        
                        ++sRowNo;
                    }
                }
            }
            
            assertEquals(sTotalRecordCount, sInsertRowCount);
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

    public void testFragmentedByMultipleColumn()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 char(100), c2 integer, c3 bigint, c4 varchar(200), c5 double)");

        int i;
        final int sInsertRowCount = 1000;
        
        for (i = 1; i <= sInsertRowCount; ++i)
        {
            String sStatementString = "insert into t1 values ('test" + i + "'," + i + ", " + (10000 + i) + ", 'test" + (10000 + i) + "', " + (i + 0.5) + ")";
            runExecuteRemoteStatement(sStatementString);
        }
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");
        
        //
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        //
        // 4. fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = -1;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 6);

            int sRowNo = 1;
            int sTotalRecordCount = 0;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();
                
                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;

                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 5);
    
                    int j;
                    loop: for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength = sFetchResultRowOp.mRowDataBuffer.getInt();
                        
                        switch (j)
                        {
                        case 0:
                        case 3:
                        {
                            String sColumnValue = 
                                    getStringColumnValue(
                                            sFetchResultRowOp.mRowDataBuffer,
                                            sColumnValueLength);
                            
                            if (j == 0)
                            {
                                assertEquals(sColumnValueLength, 100);
                                assertTrue(sColumnValue.equals("test" + sRowNo));
                            }
                            else
                            {
                                assertEquals(sColumnValueLength, sColumnValue.length());
                                assertTrue(sColumnValue.equals("test" + (10000 + sRowNo)));
                            }
                            break;
                        }
                        
                        case 1:
                        {
                            assertEquals(sColumnValueLength, 4);
                            
                            int sColumnValue = sFetchResultRowOp.mRowDataBuffer.getInt();
                            
                            assertEquals(sColumnValue, sRowNo);
                            break;
                        }
                            
                        case 2:
                        {
                            assertEquals(sColumnValueLength, 8);
                            
                            long sColumnValue = sFetchResultRowOp.mRowDataBuffer.getLong();
                            
                            assertEquals(sColumnValue, 10000 + sRowNo);
                            break;
                        }
                            
                        case 4:
                        {
                            assertEquals(sColumnValueLength, 8);
                            
                            double sColumnValue = sFetchResultRowOp.mRowDataBuffer.getDouble();
                            
                            assertEquals(sColumnValue, sRowNo + 0.5, 0);
                            break;
                        }
                        
                        default:
                        {
                            break loop;
                        }
                        }
                    }
                    
                    ++sRowNo;
                }
            }
            
            assertEquals(sTotalRecordCount, sInsertRowCount);
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

    public void testFragmentedByLargeColumnSize()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 varchar(10000), c2 varchar(20000), c3 char(32000))");

        int i, j;
        String sPrefix = new String();
        final int sInsertRowCount = 100;
        
        for (i = 1; i <= 100; ++i)
            sPrefix += "test ";
        
        for (i = 1; i <= sInsertRowCount; ++i)
        {
            String sColumn1 = sPrefix + (        i);
            String sColumn2 = sPrefix + (10000 + i);
            String sColumn3 = "test " + (20000 + i);
            
            String sStatementString = "insert into t1 values ('" + sColumn1 + "', '" + sColumn2 + "', '" + sColumn3 + "')";
            runExecuteRemoteStatement(sStatementString);
        }
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");
        
        //
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        //
        // 4. fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = -1;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 201);

            int sRowNo = 1;
            int sEndColumnIndex;
            int sTotalColumnCount = 0;
            int sTotalRecordCount = 0;
            
            j = 0;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();
                
                byte sResultCode = sFetchResultRowOp.getResultCode();
                if (sResultCode == ResultCode.EndOfFetch)
                    break;

                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;

                if (sRecordCount > 0)
                {
                    assertEquals(sResultCode, ResultCode.Success);
                }
                else
                {
                    assertEquals(sResultCode, ResultCode.Fragmented);
                }

                int sColumnCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sColumnCount = sFetchResultRowOp.mRowDataBuffer.getInt();

                sTotalColumnCount += sColumnCount;
                sEndColumnIndex = j + sColumnCount - 1;

                loop: for (; j <= sEndColumnIndex; ++j)
                {
                    int sColumnValueLength = sFetchResultRowOp.mRowDataBuffer.getInt();
                    
                    switch (j)
                    {
                    case 0:
                    case 1:
                    case 2:
                    {
                        String sColumnValue = 
                                getStringColumnValue(
                                        sFetchResultRowOp.mRowDataBuffer,
                                        sColumnValueLength);
                        
                        if (j == 2)
                        {
                            assertEquals(sColumnValueLength, 32000);
                            assertTrue(sColumnValue.equals("test " + (20000 + sRowNo)));
                        }
                        else
                        {
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            if (j == 0)
                            {
                                assertTrue(sColumnValue.equals(sPrefix + sRowNo));
                            }
                            else
                            {
                                assertTrue(sColumnValue.equals(sPrefix + (10000 + sRowNo)));
                            }
                        }
                        break;
                    }
                    
                    default:
                    {
                        break loop;
                    }
                    }
                }
                
                if (sResultCode == ResultCode.Success)
                {
                    assertEquals(sTotalColumnCount, 3);
                    
                    ++sRowNo;
                    j = 0;
                    sTotalColumnCount = 0;
                }
            }
            
            assertEquals(sTotalRecordCount, sInsertRowCount);
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
    
    private String getStringColumnValue(ByteBuffer aRowDataBuffer,
                                        int        aColumnValueLength)
    {
        byte[] sColumnValueBytes = new byte[aColumnValueLength];
        aRowDataBuffer.get(sColumnValueBytes);
        
        String sColumnValue = null;
        
        try
        {
            sColumnValue =
                    new String(sColumnValueBytes,
                            JdbcDataManager.getDBCharSetName());
            
            sColumnValue = sColumnValue.trim();
        }
        catch (UnsupportedEncodingException e)
        {
            fail(e.toString());
        }
        
        return sColumnValue;
    }

    public void testFetchSomeRecords()
    {
        //
        // 1. create table
        //
        runExecuteRemoteStatement("drop table t1");
        runExecuteRemoteStatement("create table t1 (c1 varchar(100), c2 varchar(200), c3 varchar(300), c4 varchar(400), c5 char(500))");

        int i, j;
        final int sInsertRowCount = 500;
        
        for (i = 1; i <= sInsertRowCount; ++i)
        {
            String sColumn1 = "test " + (10000 + i);
            String sColumn2 = "test test " + (20000 + i);
            String sColumn3 = "test test test " + (30000 + i);
            String sColumn4 = "test test test test " + (40000 + i);
            String sColumn5 = "test test test test test " + (50000 + i);
            
            String sStatementString = "insert into t1 values ('" + sColumn1 + "', '" + sColumn2 + "', '" + sColumn3 + "', '" + sColumn4 + "', '" + sColumn5 + "')";
            runExecuteRemoteStatement(sStatementString);
        }
        
        mMainMediatorTest.runFreeRemoteStatement(
                LinkerDataSessionId,
                RemoteStatementId);
        
        mSessionManager.closeRemoteNodeSession();
        
        //
        // 2. prepare
        //
        prepare("select * from t1");
        
        //
        // 3. execute
        //
        execute(StatementType.RemoteTable, null);
        
        //
        // 4. first fetch
        //
        int sTotalRecordCount = 0;
        
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = 100;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 2);

            int sBeginRowNo = sTotalRecordCount + 1;
            int sRowNo = sBeginRowNo;
            sTotalRecordCount = 0;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;
                
                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount =
                            sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 5);
    
                    loop: for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength =
                                sFetchResultRowOp.mRowDataBuffer.getInt();

                        String sColumnValue = 
                                getStringColumnValue(
                                        sFetchResultRowOp.mRowDataBuffer,
                                        sColumnValueLength);
                        
                        switch (j)
                        {
                        case 0:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test " + (10000 + sRowNo)));
                            break;

                        case 1:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test " + (20000 + sRowNo)));
                            break;

                        case 2:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test " + (30000 + sRowNo)));
                            break;
                        
                        case 3:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test test " + (40000 + sRowNo)));
                            break;
                            
                        case 4:
                            assertEquals(sColumnValueLength, 500);
                            assertTrue(sColumnValue.equals(
                                    "test test test test test " + (50000 + sRowNo)));
                            break;
                            
                        default:
                            break loop;
                        }
                    }
                    
                    ++sRowNo;
                }
            }
            
            assertEquals(sRowNo - sBeginRowNo, sOperation.mFetchRowCount);
        }

        //
        // 4. second fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = 50;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 1);

            int sBeginRowNo = sTotalRecordCount + 1;
            int sRowNo = sBeginRowNo;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;
                
                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount =
                            sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 5);
    
                    loop: for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength =
                                sFetchResultRowOp.mRowDataBuffer.getInt();

                        String sColumnValue = 
                                getStringColumnValue(
                                        sFetchResultRowOp.mRowDataBuffer,
                                        sColumnValueLength);
                        
                        switch (j)
                        {
                        case 0:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test " + (10000 + sRowNo)));
                            break;

                        case 1:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test " + (20000 + sRowNo)));
                            break;

                        case 2:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test " + (30000 + sRowNo)));
                            break;
                        
                        case 3:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test test " + (40000 + sRowNo)));
                            break;
                            
                        case 4:
                            assertEquals(sColumnValueLength, 500);
                            assertTrue(sColumnValue.equals(
                                    "test test test test test " + (50000 + sRowNo)));
                            break;
                            
                        default:
                            break loop;
                        }
                    }
                    
                    ++sRowNo;
                }
            }
            
            assertEquals(sRowNo - sBeginRowNo, sOperation.mFetchRowCount);
        }

        //
        // 4. third fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = 200;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 4);

            int sBeginRowNo = sTotalRecordCount + 1;
            int sRowNo = sBeginRowNo;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;
                
                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount =
                            sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 5);
    
                    loop: for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength =
                                sFetchResultRowOp.mRowDataBuffer.getInt();

                        String sColumnValue = 
                                getStringColumnValue(
                                        sFetchResultRowOp.mRowDataBuffer,
                                        sColumnValueLength);
                        
                        switch (j)
                        {
                        case 0:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test " + (10000 + sRowNo)));
                            break;

                        case 1:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test " + (20000 + sRowNo)));
                            break;

                        case 2:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test " + (30000 + sRowNo)));
                            break;
                        
                        case 3:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test test " + (40000 + sRowNo)));
                            break;
                            
                        case 4:
                            assertEquals(sColumnValueLength, 500);
                            assertTrue(sColumnValue.equals(
                                    "test test test test test " + (50000 + sRowNo)));
                            break;
                            
                        default:
                            break loop;
                        }
                    }
                    
                    ++sRowNo;
                }
            }
            
            assertEquals(sRowNo - sBeginRowNo, sOperation.mFetchRowCount);
        }

        //
        // 4. fourth fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = 10;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 1);

            int sBeginRowNo = sTotalRecordCount + 1;
            int sRowNo = sBeginRowNo;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();

                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;
                
                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                loop: for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount =
                            sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 5);
    
                    for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength =
                                sFetchResultRowOp.mRowDataBuffer.getInt();

                        String sColumnValue = 
                                getStringColumnValue(
                                        sFetchResultRowOp.mRowDataBuffer,
                                        sColumnValueLength);
                        
                        switch (j)
                        {
                        case 0:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test " + (10000 + sRowNo)));
                            break;

                        case 1:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test " + (20000 + sRowNo)));
                            break;

                        case 2:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test " + (30000 + sRowNo)));
                            break;
                        
                        case 3:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test test " + (40000 + sRowNo)));
                            break;
                            
                        case 4:
                            assertEquals(sColumnValueLength, 500);
                            assertTrue(sColumnValue.equals(
                                    "test test test test test " + (50000 + sRowNo)));
                            break;
                            
                        default:
                            break loop;
                        }
                    }
                    
                    ++sRowNo;
                }
            }
            
            assertEquals(sRowNo - sBeginRowNo, sOperation.mFetchRowCount);
        }

        //
        // 4. fifth fetch
        //
        {
            FetchResultSetOp sOperation = new FetchResultSetOp();
            
            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteStatementId = RemoteStatementId;
            sOperation.mFetchRowCount     = -1;
            
            FetchResultSetTask sTask = new FetchResultSetTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            LinkedList sFetchResultRowList = mMainMediatorTest.getResultOperationList();
            
            assertEquals(sFetchResultRowList.size(), 4);

            int sBeginRowNo = sTotalRecordCount + 1;
            int sRowNo = sBeginRowNo;
            
            Iterator sIterator = sFetchResultRowList.iterator();
            while (sIterator.hasNext() == true)
            {
                FetchResultRowOp sFetchResultRowOp = (FetchResultRowOp)sIterator.next();
                
                if (sFetchResultRowOp.getResultCode() == ResultCode.EndOfFetch)
                    break;

                assertNotNull(sFetchResultRowOp.mRowDataBuffer);
                
                int sRecordCount = 0;
                if (sFetchResultRowOp.mRowDataBuffer != null)
                    sRecordCount = sFetchResultRowOp.mRowDataBuffer.getInt();
                
                sTotalRecordCount += sRecordCount;
                
                assertEquals(sFetchResultRowOp.getResultCode(), ResultCode.Success);

                loop: for (i = 1; i <= sRecordCount; ++i)
                {
                    int sColumnCount =
                            sFetchResultRowOp.mRowDataBuffer.getInt();
                    assertEquals(sColumnCount, 5);
    
                    for (j = 0; j < sColumnCount; ++j)
                    {
                        int sColumnValueLength =
                                sFetchResultRowOp.mRowDataBuffer.getInt();

                        String sColumnValue = 
                                getStringColumnValue(
                                        sFetchResultRowOp.mRowDataBuffer,
                                        sColumnValueLength);
                        
                        switch (j)
                        {
                        case 0:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test " + (10000 + sRowNo)));
                            break;

                        case 1:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test " + (20000 + sRowNo)));
                            break;

                        case 2:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test " + (30000 + sRowNo)));
                            break;
                        
                        case 3:
                            assertEquals(sColumnValueLength,
                                    sColumnValue.length());
                            assertTrue(sColumnValue.equals(
                                    "test test test test " + (40000 + sRowNo)));
                            break;
                            
                        case 4:
                            assertEquals(sColumnValueLength, 500);
                            assertTrue(sColumnValue.equals(
                                    "test test test test test " + (50000 + sRowNo)));
                            break;
                            
                        default:
                            break loop;
                        }
                    }
                    
                    ++sRowNo;
                }
            }
            
            assertEquals(sRowNo - sBeginRowNo, 140);
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
