/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.ex;

import Altibase.jdbc.driver.cm.CmExecutionResult;
import Altibase.jdbc.driver.cm.CmProtocolContext;
import junit.framework.TestCase;

public class SmnErrorParseTest extends TestCase
{
    private String mErrMsg = "The shard meta number(SMN) is invalid.: [ Session SMN=13, Data Node SMN=14, Disconnect=N, Protocol=ExecuteV2Result 65537 1 1 18446744073709551615 0 0  ]";

    public void testParseExecuteV3()
    {
        CmProtocolContext sContext = new CmProtocolContext();
        CmExecutionResult sExecResult = new CmExecutionResult();
        sContext.addCmResult(sExecResult);
        ShardError.parseExecuteResults(sContext, mErrMsg);
        assertEquals(sExecResult.getStatementId(), 65537);
        assertEquals(sExecResult.getRowNumber(), 1);
        assertEquals(sExecResult.getResultSetCount(), 1);
        assertEquals(sExecResult.getUpdatedRowCount(), -1);
    }

    public void testParseSMN()
    {
        long sSmn = ShardError.getSMNOfDataNodeFromErrorMsg(mErrMsg);
        assertEquals(sSmn, 14);
    }

    public void testDisconnect()
    {
        boolean sDisconnect = ShardError.getShardDiconnectFromErrorMsg(mErrMsg);
        assertFalse(sDisconnect);
    }
}
