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
 
package com.altibase.altilinker.session;

/**
 * Remote node session id generator class
 */
public class RemoteNodeSessionIdGenerator
{
    private static RemoteNodeSessionIdGenerator mRemoteNodeSessionIdGenerator =
                                             new RemoteNodeSessionIdGenerator();

    private boolean[] mRemoteNodeSessionIdArray = null;
    
    /**
     * Constructor
     */
    private RemoteNodeSessionIdGenerator()
    {
        // nothing to do
    }
    
    /**
     * Get static RemoteNodeSessionIdGenerator object
     * 
     * @return  static RemoteNodeSessionIdGenerator object
     */
    public static RemoteNodeSessionIdGenerator getInstance()
    {
        return mRemoteNodeSessionIdGenerator;
    }
    
    /**
     * Set maximum remote node session count
     * 
     * @param aMaxRemoteNodeSession     Maximum remote node session count
     */
    public void setMaxRemoteNodeSession(int aMaxRemoteNodeSession)
    {
        if (aMaxRemoteNodeSession <= 0)
        {
            return;
        }
        
        mRemoteNodeSessionIdArray = new boolean[aMaxRemoteNodeSession];
    }
    
    /**
     * Generate new remote node session ID
     * 
     * @return  New remote node session ID
     */
    public synchronized int generate()
    {
        if (mRemoteNodeSessionIdArray == null)
        {
            return 0;
        }
        
        int i;
        int sRemoteNodeSessionId = 0;

        for (i = 0; i < mRemoteNodeSessionIdArray.length; ++i)
        {
            if (mRemoteNodeSessionIdArray[i] == false)
            {
                mRemoteNodeSessionIdArray[i] = true;
                
                sRemoteNodeSessionId = i + 1;
                break;
            }
        }
        
        return sRemoteNodeSessionId;
    }
    
    /**
     * Release remote node session ID
     * 
     * @param aRemoteNodeSessionId      Remote node session ID to release
     */
    public synchronized void release(int aRemoteNodeSessionId)
    {
        if (aRemoteNodeSessionId <= 0 ||
            aRemoteNodeSessionId > mRemoteNodeSessionIdArray.length)
        {
            return;
        }
        
        mRemoteNodeSessionIdArray[aRemoteNodeSessionId - 1] = false;
    }
}
