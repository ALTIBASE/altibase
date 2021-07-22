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

import com.altibase.altilinker.adlp.type.ResultCode;

public class CreateLinkerDataSessionTest extends TaskTestCase
{
    public void test()
    {
        mMainMediatorTest.reset();
        mMainMediatorTest.runCreateLinkerCtrlSessionTask(null);

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);
        assertTrue(mSessionManager.isCreatedLinkerCtrlSession());
        
        mMainMediatorTest.reset();
        mMainMediatorTest.runCreateLinkerDataSessionTask(100);
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);
        assertTrue(mSessionManager.isCreatedLinkerDataSession(100));
        assertFalse(mSessionManager.isCreatedLinkerDataSession(200));

        mMainMediatorTest.reset();
        mMainMediatorTest.runCreateLinkerDataSessionTask(200);

        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(), ResultCode.Success);
        assertTrue(mSessionManager.isCreatedLinkerDataSession(200));

        mMainMediatorTest.reset();
        mMainMediatorTest.runCreateLinkerDataSessionTask(200);
        
        assertNotNull(mMainMediatorTest.getResultOperation());
        assertEquals(mMainMediatorTest.getResultCode(),
                                              ResultCode.AlreadyCreatedSession);
        assertTrue(mSessionManager.isCreatedLinkerDataSession(100));
        assertTrue(mSessionManager.isCreatedLinkerDataSession(200));
    }
}
