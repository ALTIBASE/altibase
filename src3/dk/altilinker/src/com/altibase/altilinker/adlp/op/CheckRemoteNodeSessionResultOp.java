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

public class CheckRemoteNodeSessionResultOp extends ResultOperation
{
    public RemoteNodeSession[] mRemoteNodeSessions = null;
    
    public static class RemoteNodeSession
    {
        public short mRemoteNodeSessionId     = 0;
        public int   mRemoteNodeSessionStatus = RemoteNodeSessionStatus.Disconnected;
    }
    
    public CheckRemoteNodeSessionResultOp()
    {
        super(OpId.CheckRemoteNodeSessionResult, true);
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
                if (mRemoteNodeSessions != null)
                {
                    writeInt(aOpPayload, mRemoteNodeSessions.length);
                    
                    int i;
                    for (i = 0; i < mRemoteNodeSessions.length; ++i)
                    {
                        writeShort(aOpPayload,
                                mRemoteNodeSessions[i].mRemoteNodeSessionId);
                        writeInt  (aOpPayload,
                                mRemoteNodeSessions[i].mRemoteNodeSessionStatus);
                    }
                }
                else
                {
                    writeInt(aOpPayload, 0);
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
