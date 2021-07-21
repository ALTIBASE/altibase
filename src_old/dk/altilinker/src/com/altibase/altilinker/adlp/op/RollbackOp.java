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
import com.altibase.altilinker.adlp.type.*;

public class RollbackOp extends RequestOperation
{
    public int     mRollbackFlag       = RollbackFlag.Transaction;
    public String  mSavepointName      = null;
    public short[] mRemoteNodeSessions = null;
   
    public RollbackOp()
    {
        super(OpId.Rollback, true);
    }

    public ResultOperation newResultOperation()
    {
        return new RollbackResultOp();
    }
    
    protected boolean readOperation(CommonHeader aCommonHeader,
                                    ByteBuffer   aOpPayload)
    {
        // common header
        setCommonHeader(aCommonHeader);

        mRollbackFlag = getOpSpecificParameter();

        if (aCommonHeader.getDataLength() > 0)
        {
            try
            {
                mSavepointName = readVariableString(aOpPayload, true);
                
                int sRemoteNodeSessionCount = readInt(aOpPayload);
                if (sRemoteNodeSessionCount > 0)
                {
                    mRemoteNodeSessions = new short[sRemoteNodeSessionCount];
                    
                    int i;
                    for (i = 0; i < sRemoteNodeSessionCount; ++i)
                    {
                        mRemoteNodeSessions[i] = readShort(aOpPayload);
                    }
                }
            }
            catch (BufferUnderflowException e)
            {
                return false;
            }
        }
        
        return true;
    }

    protected boolean validate()
    {
        if (getSessionId() == 0)
        {
            return false;
        }
        
        if ((mRollbackFlag != RollbackFlag.Transaction      ) &&
            (mRollbackFlag != RollbackFlag.SpecificSavepoint))
        {
            return false;
        }

        if (mRollbackFlag == RollbackFlag.SpecificSavepoint)
        {
            if (mSavepointName == null)
            {
                return false;
            }
            
            if (mSavepointName.length() == 0)
            {
                return false;
            }
        }
        
        return true;
    }
}
