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

public class ExecuteRemoteStatementOp extends RequestOperation
{
    public String mTargetName        = null;
    public String mTargetUser        = null;
    public String mTargetPassword    = null;
    public long   mRemoteStatementId = 0;
    public int    mStatementType     = StatementType.Unknown;
    public String mStatementString   = null;
    public AldpXid    mAldpXid       = null;
    
    public ExecuteRemoteStatementOp()
    {
        super(OpId.ExecuteRemoteStatement, true, true);
    }

    public ResultOperation newResultOperation()
    {
        return new ExecuteRemoteStatementResultOp();
    }
    
    protected boolean isFragmented(CommonHeader aCommonHeader)
    {
        byte sOpSpecificParameter = aCommonHeader.getOpSpecificParameter();
        
        boolean sFragmented = (sOpSpecificParameter != 0) ? true : false;
        
        return sFragmented;
    }
    
    protected boolean readOperation(CommonHeader aCommonHeader,
                                    ByteBuffer   aOpPayload)
    {
        // common header
        setCommonHeader(aCommonHeader);
        
        if (aCommonHeader.getDataLength() > 0)
        {
            try
            {
                if ( isXA() == true )
                {
                    mAldpXid = readAldpXid( aOpPayload );
                }
                
                mTargetName        = readVariableString(aOpPayload);
                mTargetUser        = readVariableString(aOpPayload, true);
                mTargetPassword    = readVariableString(aOpPayload, true);
                mRemoteStatementId = readLong          (aOpPayload);
                mStatementType     = readInt           (aOpPayload);
                mStatementString   = readVariableString(aOpPayload, true);

                // convert to upper case
                if (mTargetName != null)
                    mTargetName = mTargetName.toUpperCase();
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
        
        if (mTargetName == null)
        {
            return false;
        }
        
        if (mTargetName.length() == 0)
        {
            return false;
        }
        
        if (mRemoteStatementId == 0)
        {
            return false;
        }

        if ((mStatementType != StatementType.RemoteTable              ) &&
            (mStatementType != StatementType.RemoteExecuteImmediate   ) &&
            (mStatementType != StatementType.ExecuteQueryStatement    ) &&
            (mStatementType != StatementType.ExecuteNonQueryStatement ) &&
            (mStatementType != StatementType.ExecuteStatement         ))
        {
            return false;
        }

        // mStatementString may empty only for executing.
       
        return true;
    }
}
