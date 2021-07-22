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

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import com.altibase.altilinker.util.TraceLogger;

/**
 * Session manager component class, which manages linker sessions and remote node sessions
 */
public class SessionManager
{
    public static final int  NOTIFY_SESSION_ID = 0x7FFFFFFF;
    
    private static TraceLogger mTraceLogger = TraceLogger.getInstance();
    
    private LinkerCtrlSession mLinkerCtrlSession    = null;
    private HashMap           mLinkerDataSessionMap = new HashMap();
    
    /**
     * Constructor
     */
    public SessionManager()
    {
        mTraceLogger.logTrace("SessionManager component created.");
    }
    
    /**
     * Close this component
     */
    public void close()
    {
        mTraceLogger.logTrace("SessionManager component closed.");
    }
    
    /**
     * Create linker control session
     * 
     * @return  true, if linker control session create successfully. false, if fail.
     */
    public synchronized boolean createLinkerCtrlSession()
    {
        if (mLinkerCtrlSession != null)
        {
            return false;
        }
        
        mLinkerCtrlSession = new LinkerCtrlSession();
        
        return true;
    }
    
    /**
     * Create linker data session
     *
     * @param aSessionId        Linker data session ID
     * @return                  true, if linker data session create successfully. false, if fail.
     */
    public synchronized boolean createLinkerDataSession( int     aSessionId )
    {
        if (mLinkerDataSessionMap.get(new Integer(aSessionId)) != null)
        {
            return false;
        }
        
        LinkerDataSession sLinkerDataSession = new LinkerDataSession( aSessionId );
        mLinkerDataSessionMap.put(new Integer(aSessionId), sLinkerDataSession);
        
        return true;
    }
    
    /**
     * Destroy linker control session
     */
    public synchronized void destroyLinkerCtrlSession()
    {
        if (mLinkerCtrlSession != null)
        {
            mLinkerCtrlSession.destroy();
            mLinkerCtrlSession = null;
        }
    }

    /**
     * Destroy linker data session by linker data session ID
     * 
     * @param aSessionId        Linker data session ID
     */
    public synchronized void destroyLinkerDataSession(int aSessionId)
    {
        if (mLinkerDataSessionMap.containsKey(new Integer(aSessionId)) == true)
        {
            LinkerDataSession sLinkerDataSession =
                    (LinkerDataSession)mLinkerDataSessionMap.remove(new Integer(aSessionId));
            
            if (sLinkerDataSession != null)
            {
                sLinkerDataSession.destroy();
                sLinkerDataSession = null;
            }
        }
    }
    
    /**
     * Destroy all of linker data sessions
     */
    public synchronized void destroyLinkerDataSession()
    {
        mLinkerDataSessionMap.clear();
    }
    
    /**
     * Get linker control session
     * 
     * @return  LinkerCtrlSession object
     */
    public synchronized LinkerCtrlSession getLinkerCtrlSession()
    {
        return mLinkerCtrlSession;
    }
    
    /**
     * Get linker data session by linker data session ID
     * 
     * @param aSessionId        Linker data session ID
     * @return                  LinkerDataSession object
     */
    public synchronized LinkerDataSession getLinkerDataSession(int aSessionId)
    {
        LinkerDataSession sLinkerDataSession;
        
        sLinkerDataSession = 
                (LinkerDataSession)mLinkerDataSessionMap.get(new Integer(aSessionId));
        
        return sLinkerDataSession;
    }
    
    /**
     * Get linker data session count
     * 
     * @return  Linker data session count
     */
    public synchronized int getLinkerDataSessionCount()
    {
        return mLinkerDataSessionMap.size();
    }
    
    /**
     * Get remote node session count
     * 
     * @return  Remote node session count
     */
    public synchronized int getRemoteNodeSessionCount()
    {
        int sTotalRemoteNodeSessionCount = 0;
        int sRemoteNodeSessionCount      = 0;

        Iterator sIterator = mLinkerDataSessionMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            LinkerDataSession sLinkerDataSession = (LinkerDataSession)sEntry.getValue();

            sRemoteNodeSessionCount = sLinkerDataSession.getRemoteNodeSessionCount();
            
            sTotalRemoteNodeSessionCount += sRemoteNodeSessionCount;
        }
        
        return sTotalRemoteNodeSessionCount;
    }

    /**
     * Get remote statement count on a specific linker data session
     * 
     * @param aSessionId        Linker data session ID
     * @return                  Remote statement count
     */
    public synchronized int getRemoteStatementCount(int aSessionId)
    {
        LinkerDataSession sLinkerDataSession = 
                (LinkerDataSession)mLinkerDataSessionMap.get(new Integer(aSessionId));
        
        if (sLinkerDataSession == null)
        {
            return 0;
        }

        int sRemoteStatementCount = sLinkerDataSession.getRemoteStatementCount();
        
        return sRemoteStatementCount;
    }
    
    /**
     * Get remote statement count
     * 
     * @return  Remote statement count
     */
    public synchronized int getRemoteStatementCount()
    {
        int sTotalRemoteStatementCount = 0;
        int sRemoteStatementCount      = 0;

        Iterator sIterator = mLinkerDataSessionMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            LinkerDataSession sLinkerDataSession = (LinkerDataSession)sEntry.getValue();

            sRemoteStatementCount = sLinkerDataSession.getRemoteStatementCount();
            
            sTotalRemoteStatementCount += sRemoteStatementCount;
        }
        
        return sTotalRemoteStatementCount;
    }
    
    /**
     * Whether linker control session create or not
     * 
     * @return  true, if linker control session create. false, if not.
     */
    public synchronized boolean isCreatedLinkerCtrlSession()
    {
        if (mLinkerCtrlSession == null)
        {
            return false;
        }
        
        return true;
    }

    /**
     * Whether linker data session create or not
     * 
     * @param aSessionId        Linker data session ID
     * @return                  true, if linker data session create. false, if not.
     */
    public synchronized boolean isCreatedLinkerDataSession(int aSessionId)
    {
        if (mLinkerDataSessionMap.get(new Integer(aSessionId)) == null)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * close all of remote node sessions
     */
    public synchronized void closeRemoteNodeSession()
    {
        Iterator sIterator = mLinkerDataSessionMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            LinkerDataSession sLinkerDataSession = (LinkerDataSession)sEntry.getValue();

            sLinkerDataSession.closeRemoteNodeSession();
        }
    }
}
