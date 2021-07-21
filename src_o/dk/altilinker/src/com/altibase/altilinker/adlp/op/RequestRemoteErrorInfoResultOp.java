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
 
package com.altibase.altilinker.adlp.op;

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;

import com.altibase.altilinker.adlp.*;

public class RequestRemoteErrorInfoResultOp extends ResultOperation
{
    public long   mRemoteStatementId = 0;
    public int    mErrorCode         = 0;
    public String mSQLState          = null;
    public String mErrorMessage      = null;
    
    public RequestRemoteErrorInfoResultOp()
    {
        super(OpId.RequestRemoteErrorInfoResult, true);
    }
    
    protected boolean writeOperation(CommonHeader aCommonHeader,
                                     ByteBuffer   aOpPayload)
    {
        fillCommonHeader(aCommonHeader);
        
        try
        {
            if (canWriteData() == true)
            {
                String sSQLState = mSQLState;
                if (sSQLState == null)
                    sSQLState = "00000";
                
                // write data
                writeLong          (aOpPayload, mRemoteStatementId);
                writeInt           (aOpPayload, mErrorCode);
                writeFixedString   (aOpPayload, sSQLState, 5, false, '0');
                writeVariableString(aOpPayload, mErrorMessage);
            }
        }
        catch (BufferUnderflowException e)
        {
            return false;
        }

        return true;
    }
}
