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
import com.altibase.altilinker.worker.task.CheckRemoteNodeSessionTask;

public class CheckRemoteNodeSessionTaskTest extends TaskTestCase
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
        //
        // 1. execute
        //
        short sRemoteNodeSessionId =
                execute(StatementType.RemoteExecuteImmediate,
                        "insert into t1 values(100)");
        
        //
        // 2. check remote node session status for a specific
        //
        {
            CheckRemoteNodeSessionOp sOperation =
                    new CheckRemoteNodeSessionOp();

            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteNodeSessionId = sRemoteNodeSessionId;
            
            CheckRemoteNodeSessionTask sTask = new CheckRemoteNodeSessionTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            assertNotNull(mMainMediatorTest.getResultOperation());
            assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

            assertTrue(mMainMediatorTest.getResultOperation() instanceof
                    CheckRemoteNodeSessionResultOp);
            
            CheckRemoteNodeSessionResultOp sResultOperation =
                    (CheckRemoteNodeSessionResultOp)
                            mMainMediatorTest.getResultOperation(); 
            
            if (sResultOperation == null)
            {
                fail("sResultOperation is null.");
                return;
            }

            assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);
            
            assertNotNull(sResultOperation.mRemoteNodeSessions);
            assertEquals(sResultOperation.mRemoteNodeSessions.length, 1);
            
            assertEquals(
                    sResultOperation.mRemoteNodeSessions[0].mRemoteNodeSessionId,
                    sRemoteNodeSessionId);
            
            assertEquals(
                    sResultOperation.mRemoteNodeSessions[0].mRemoteNodeSessionStatus,
                    RemoteNodeSessionStatus.Connected);
        }
        
        //
        // 3. check remote node session status of all of remote node sessions
        //
        {
            CheckRemoteNodeSessionOp sOperation =
                    new CheckRemoteNodeSessionOp();

            sOperation.setSessionId(LinkerDataSessionId);
            
            sOperation.mRemoteNodeSessionId = 0;
            
            CheckRemoteNodeSessionTask sTask = new CheckRemoteNodeSessionTask();
    
            sTask.setMainMediator(mMainMediatorTest);
            sTask.setSessionManager(mSessionManager);
            sTask.setOperation(sOperation);
            
            mMainMediatorTest.reset();
            sTask.run();
            
            assertNotNull(mMainMediatorTest.getResultOperation());
            assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);

            assertTrue(mMainMediatorTest.getResultOperation() instanceof
                    CheckRemoteNodeSessionResultOp);
            
            CheckRemoteNodeSessionResultOp sResultOperation =
                    (CheckRemoteNodeSessionResultOp)
                            mMainMediatorTest.getResultOperation(); 
            
            assertEquals(sResultOperation.getSessionId(), LinkerDataSessionId);

            assertNotNull(sResultOperation.mRemoteNodeSessions);
            
            int i;
            for (i = 0; i < sResultOperation.mRemoteNodeSessions.length; ++i)
            {
                assertEquals(
                        sResultOperation.mRemoteNodeSessions[i].mRemoteNodeSessionId,
                        sRemoteNodeSessionId);
                
                assertEquals(
                        sResultOperation.mRemoteNodeSessions[i].mRemoteNodeSessionStatus,
                        RemoteNodeSessionStatus.Connected);
            }
        }
    }
}
