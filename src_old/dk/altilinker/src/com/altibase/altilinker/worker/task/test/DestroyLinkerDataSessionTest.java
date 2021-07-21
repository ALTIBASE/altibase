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

import com.altibase.altilinker.adlp.op.DestroyLinkerDataSessionOp;
import com.altibase.altilinker.adlp.type.ResultCode;
import com.altibase.altilinker.worker.task.DestroyLinkerDataSessionTask;

public class DestroyLinkerDataSessionTest extends TaskTestCase
{
    public void test()
    {
        DestroyLinkerDataSessionOp sOperation =
                new DestroyLinkerDataSessionOp();
        
        sOperation.setSessionId(100);
        
        DestroyLinkerDataSessionTask sTask = new DestroyLinkerDataSessionTask();

        sTask.setMainMediator(mMainMediatorTest);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        mMainMediatorTest.reset();
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), 
                                                     ResultCode.UnknownSession);
        assertFalse(mSessionManager.isCreatedLinkerCtrlSession());
        assertFalse(mSessionManager.isCreatedLinkerDataSession(100));
        
        mMainMediatorTest.reset();
        mMainMediatorTest.runCreateLinkerCtrlSessionTask(null);
        
        mMainMediatorTest.reset();
        mMainMediatorTest.runCreateLinkerDataSessionTask(100);

        mMainMediatorTest.reset();
        mMainMediatorTest.runCreateLinkerDataSessionTask(200);
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);
        assertTrue(mSessionManager.isCreatedLinkerCtrlSession());
        assertTrue(mSessionManager.isCreatedLinkerDataSession(100));
        assertTrue(mSessionManager.isCreatedLinkerDataSession(200));
        assertEquals(mSessionManager.getLinkerDataSessionCount(), 2);

        mMainMediatorTest.reset();
        sOperation.setSessionId(100);
        sTask.setOperation(sOperation);
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);
        assertTrue(mSessionManager.isCreatedLinkerCtrlSession());
        assertFalse(mSessionManager.isCreatedLinkerDataSession(100));
        assertTrue(mSessionManager.isCreatedLinkerDataSession(200));
        assertEquals(mSessionManager.getLinkerDataSessionCount(), 1);

        mMainMediatorTest.reset();
        sOperation.setSessionId(200);
        sTask.setOperation(sOperation);
        sTask.run();

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);
        assertTrue(mSessionManager.isCreatedLinkerCtrlSession());
        assertFalse(mSessionManager.isCreatedLinkerDataSession(100));
        assertFalse(mSessionManager.isCreatedLinkerDataSession(200));
        assertEquals(mSessionManager.getLinkerDataSessionCount(), 0);
    }
}
