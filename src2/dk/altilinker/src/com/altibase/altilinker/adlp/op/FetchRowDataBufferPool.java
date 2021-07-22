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

import java.nio.ByteBuffer;
import java.util.LinkedList;

import com.altibase.altilinker.adlp.ProtocolManager;

public class FetchRowDataBufferPool
{
    protected static FetchRowDataBufferPool mBufferPool =
                                                   new FetchRowDataBufferPool();

    // 8 = Remote Statement Id (8)
    public static final int MaxResultRowDataSize =
                                      ProtocolManager.getMaxOpPayloadSize() - 8; 
    
    private LinkedList mBufferList = new LinkedList();

    public static FetchRowDataBufferPool getInstance()
    {
        return mBufferPool;
    }
    
    public synchronized void close()
    {
        mBufferList.clear();
    }
    
    public synchronized ByteBuffer allocate()
    {
        ByteBuffer sBuffer = null;
        
        if (mBufferList.isEmpty() == true)
        {
            sBuffer = ByteBuffer.allocateDirect(MaxResultRowDataSize);
        }
        else
        {
            sBuffer = (ByteBuffer)mBufferList.removeFirst();
        }
        
        return sBuffer;
    }
    
    public synchronized void free(ByteBuffer aBuffer)
    {
        if (aBuffer == null)
        {
            return;
        }
        
        aBuffer.clear();
        
        mBufferList.add(aBuffer);
    }
}
