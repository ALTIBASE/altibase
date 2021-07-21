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
import java.util.LinkedList;

import com.altibase.altilinker.adlp.*;

public class ExecuteRemoteStatementBatchResultOp extends ResultOperation
{
    public long  mRemoteStatementId   = 0;
    public int  mResult[] = null;
    
    protected LinkedList mFragmentPosHintList = null;
    
    public ExecuteRemoteStatementBatchResultOp()
    {
        super(OpId.ExecuteRemoteStatementBatchResult, true, true);
    }
    
    public void close()
    {
        mFragmentPosHintList = null;
        
        super.close();
    }
    
    /**
     * Get operation payload size
     * 
     * @return  Operation payload size
     */
    protected int getOpPayloadSize()
    {
    	int length = 0;
    	
    	if (mResult != null)
    	{
    		length = mResult.length;
    	}
    	
        return (8 + 4 + (length * 4));
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
                writeLong(aOpPayload, mRemoteStatementId);

                if (mResult != null)
                {
                	writeInt(aOpPayload, mResult.length);

                	if (mResult.length >= 1)
                	{
                		mFragmentPosHintList = new LinkedList();
                	}
                
                	for (int i = 0; i < mResult.length; i++)
                	{
                		writeInt(aOpPayload, mResult[i]);
                	
                		mFragmentPosHintList.add(new Integer(aOpPayload.position()));
                	}
                }
                else
                {
                	int zero = 0;
                	writeInt(aOpPayload, zero);
                }
            }
        }
        catch (BufferUnderflowException e)
        {
            return false;
        }
        catch (Exception e)
        {
        	return false;
        }

        return true;
    }
    
    protected LinkedList getFragmentPosHint()
    {
        return mFragmentPosHintList;
    }
}
