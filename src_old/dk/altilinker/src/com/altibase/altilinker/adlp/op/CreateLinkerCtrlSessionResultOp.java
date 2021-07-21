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
import java.util.Iterator;
import java.util.LinkedList;

import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.type.Target;

public class CreateLinkerCtrlSessionResultOp extends ResultOperation
{
    public int        mProtocolVersion   = 0;
    public LinkedList mInvalidTargetList = null; 
    
    public CreateLinkerCtrlSessionResultOp()
    {
        super(OpId.CreateLinkerCtrlSessionResult, false);
    }
    
    protected boolean writeOperation(CommonHeader aCommonHeader,
                                     ByteBuffer   aOpPayload)
    {
        fillCommonHeader(aCommonHeader);
        
        try
        {
            if (canWriteData() == true)
            {
                // write data
                writeInt(aOpPayload, mProtocolVersion);

                if (mInvalidTargetList == null)
                {
                    writeInt(aOpPayload, 0);
                }
                else
                {
                    // write invalid target count
                    int sInvalidTargetCount = mInvalidTargetList.size();
                    writeInt(aOpPayload, sInvalidTargetCount);

                    // write invalid target name
                    Iterator sIterator = mInvalidTargetList.iterator();
                    while (sIterator.hasNext() == true)
                    {
                        Target sTarget = (Target)sIterator.next();
                        
                        writeVariableString(aOpPayload, sTarget.mName);
                    }
                }
            }
        }
        catch (BufferUnderflowException e)
        {
            return false;
        }

        return true;
    }
}
