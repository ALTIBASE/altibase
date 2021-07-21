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

import com.altibase.altilinker.adlp.type.*;

/**
 * Linker data session class for linker data session management
 */
public class LinkerDataSession implements LinkerSession
{
    private static RemoteNodeSessionIdGenerator mRemoteNodeSessionIdGenerator =
                                     RemoteNodeSessionIdGenerator.getInstance();
    
    private int     mSessionId            = 0;
    private int     mSessionState         = SessionState.None;
    private long    mThreadId             = 0;
    private boolean mAutoCommitMode       = true;
    private HashMap mRemoteNodeSessionMap = new HashMap(); // <Remote Node Session Id, RemoteNodeSession object>
    private HashMap mRemoteStatementIdMap = new HashMap(); // <Remote Statement Id, Remote Node Session Id>
    
    /**
     * Constructor
     * 
     * @param aSessionId        Linker data session ID
     */
    public LinkerDataSession(int aSessionId)
    {
        mSessionId    = aSessionId;
        mSessionState = SessionState.Created;
    }
    
    /**
     * Destroy linker data session with closing all of remote node session
     */
    public synchronized void destroy()
    {
        mSessionState = SessionState.Destroyed;

        closeRemoteNodeSession();
    }
    
    /**
     * Get session ID
     * 
     * @return  Session ID
     */
    public synchronized int getSessionId()
    {
        return mSessionId;
    }
    
    /**
     * Get session state
     * 
     * @return  Session state
     */
    public synchronized int getSessionState()
    {
        return mSessionState;
    }
    
    /**
     * Get thread ID, which this session is assigned to a specific thread on thread pool 
     * 
     * @return  Thread ID
     */
    public synchronized long getThreadId()
    {
        return mThreadId;
    }
    
    /**
     * Get auto-commit mode
     * 
     * @return  true, if auto-commit mode is on. false, if off.
     */
    public boolean getAutoCommitMode()
    {
        return mAutoCommitMode;
    }
    
    /**
     * Set session state
     * 
     * @param aSessionState     Session state
     */
    public void setSessionState(int aSessionState)
    {
        mSessionState = aSessionState;
    }
    
    /**
     * Set auto-commit mode
     * 
     * @param aAutoCommitMode   Auto-commit mode
     */
    public void setAutoCommitMode(boolean aAutoCommitMode)
    {
        mAutoCommitMode = aAutoCommitMode;
    }
    
    /**
     * Create new RemoteNodeSession object by target
     * 
     * @param aTarget   Target
     * @return          New RemoteNodeSession object
     */
    public synchronized RemoteNodeSession newRemoteNodeSession(Target aTarget)
    {
        int sRemoteNodeSessionId = mRemoteNodeSessionIdGenerator.generate();
        if (sRemoteNodeSessionId == 0)
        {
            return null;
        }

        RemoteNodeSession sRemoteNodeSession = 
                new RemoteNodeSession(sRemoteNodeSessionId, aTarget);
        
        mRemoteNodeSessionMap.put(
                new Integer(sRemoteNodeSessionId), sRemoteNodeSession);
        
        return sRemoteNodeSession;
    }

    /**
     * Create new RemoteStatement object on a specific remote node session
     * 
     * @param aRemoteNodeSessionId      Remote node session ID 
     * @param aRemoteStatementId        Remote statement ID
     * @param aStatementString          Statement string
     * @return                          New RemoteStatement object
     */
    public synchronized
    RemoteStatement newRemoteStatement(int    aRemoteNodeSessionId,
                                       long   aRemoteStatementId,
                                       String aStatementString)
    {
        RemoteNodeSession sRemoteNodeSession = getRemoteNodeSession(aRemoteNodeSessionId);

        if (sRemoteNodeSession == null)
        {
            return null;
        }
        
        RemoteStatement sRemoteStatement =
                sRemoteNodeSession.newRemoteStatement(aRemoteStatementId,
                                                      aStatementString,
                                                      aRemoteNodeSessionId);
        
        mRemoteStatementIdMap.put(new Long(aRemoteStatementId),
                                  new Integer(aRemoteNodeSessionId));
        
        return sRemoteStatement;
    }
    
    /**
     * Get RemoteStatement object
     * 
     * @param aRemoteStatementId        Remote statement ID
     * @return                          RemoteStatement object
     */
    public synchronized
    RemoteStatement getRemoteStatement(long aRemoteStatementId)
    {
        Integer           sRemoteNodeSessionId = null;
        RemoteNodeSession sRemoteNodeSession;
        RemoteStatement   sRemoteStatement;
        
        sRemoteNodeSessionId = (Integer)mRemoteStatementIdMap.get(new Long(aRemoteStatementId));
        
        if (sRemoteNodeSessionId == null)
        {
            return null;
        }

        sRemoteNodeSession = getRemoteNodeSession(sRemoteNodeSessionId.intValue());
        
        if (sRemoteNodeSession == null)
        {
            return null;
        }
       
        sRemoteStatement = sRemoteNodeSession.getRemoteStatement(aRemoteStatementId);
        
        return sRemoteStatement;
    }

    /**
     * Get remote node session map
     * 
     * @return  Remote node session map
     */
    public synchronized Map getRemoteNodeSessionMap()
    {
        HashMap sRemoteNodeSessionMap = (HashMap)mRemoteNodeSessionMap.clone();
        
        return sRemoteNodeSessionMap;
    }
    
    /**
     * Get remote node session by target
     * 
     * @param aTarget   Target
     * @return          RemoteNodeSession object
     */
    public synchronized RemoteNodeSession getRemoteNodeSession(Target aTarget)
    {
        Iterator sIterator = mRemoteNodeSessionMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            RemoteNodeSession sRemoteNodeSession = (RemoteNodeSession)sEntry.getValue();

            if (sRemoteNodeSession.getTarget().equals(aTarget) == true)
            {
                return sRemoteNodeSession;
            }
        }
        
        return null;
    }

    /**
     * Get remote node session by remote node session ID
     * 
     * @param aRemoteNodeSessionId      Remote node session ID
     * @return                          RemoteNodeSession object
     */
    public synchronized
    RemoteNodeSession getRemoteNodeSession(int aRemoteNodeSessionId)
    {
        RemoteNodeSession sRemoteNodeSession =
                (RemoteNodeSession)mRemoteNodeSessionMap.get(
                        new Integer(aRemoteNodeSessionId));
        
        return sRemoteNodeSession;
    }
    
    /**
     * Get remote node session which has remote statement
     * 
     * @param aRemoteStatementId        Remote statement ID
     * @return                          RemoteNodeSession object
     */
    public synchronized
    RemoteNodeSession getRemoteNodeSessionFromRemoteStatement(long aRemoteStatementId)
    {
        Integer           sRemoteNodeSessionId = null;
        RemoteNodeSession sRemoteNodeSession;
        
        sRemoteNodeSessionId = 
                (Integer)mRemoteStatementIdMap.get(new Long(aRemoteStatementId));
        
        if (sRemoteNodeSessionId == null)
        {
            return null;
        }

        sRemoteNodeSession = getRemoteNodeSession(sRemoteNodeSessionId.intValue());
        
        return sRemoteNodeSession;
    }
    
    /**
     * Get remote node session count
     * 
     * @return  Remote node session count
     */
    public synchronized int getRemoteNodeSessionCount()
    {
        return mRemoteNodeSessionMap.size();
    }

    /**
     * Get all of remote statement count on this linker data session
     * 
     * @return  Remote statement count
     */
    public synchronized int getRemoteStatementCount()
    {
        int sTotalRemoteStatementCount = 0;
        int sRemoteStatementCount      = 0;
        
        Iterator sIterator = mRemoteNodeSessionMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            RemoteNodeSession sRemoteNodeSession = (RemoteNodeSession)sEntry.getValue();

            sRemoteStatementCount = sRemoteNodeSession.getRemoteStatementCount();
            
            sTotalRemoteStatementCount += sRemoteStatementCount; 
        }
        
        return sTotalRemoteStatementCount;
    }

    /**
     * Get all of remote statement count on a specific remote node session
     * 
     * @param aRemoteNodeSessionId      Remote node session ID
     * @return                          Remote statement count
     */
    public synchronized int getRemoteStatementCount(int aRemoteNodeSessionId)
    {
        RemoteNodeSession sRemoteNodeSession = getRemoteNodeSession(aRemoteNodeSessionId);
        
        if (sRemoteNodeSession == null)
        {
            return 0;
        }

        int sRemoteStatementCount = sRemoteNodeSession.getRemoteStatementCount();
        
        return sRemoteStatementCount;
    }
    
    /**
     * Set thread ID, which this session is assigned to a specific thread on thread pool 
     * 
     * @param aThreadId Thread ID
     */
    public synchronized void setThreadId(long aThreadId)
    {
        mThreadId = aThreadId;
    }
    
    /**
     * Close all of remote node sessions
     */
    public synchronized void closeRemoteNodeSession()
    {
        Iterator sIterator = mRemoteNodeSessionMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            RemoteNodeSession sRemoteNodeSession = (RemoteNodeSession)sEntry.getValue();

            sRemoteNodeSession.close();
        }
        
        mRemoteNodeSessionMap.clear();
        mRemoteStatementIdMap.clear();
    }

    /**
     * Close a specific remote node session
     * 
     * @param aRemoteNodeSessionId      Remote node session ID
     */
    public synchronized void closeRemoteNodeSession(int aRemoteNodeSessionId)
    {
        RemoteNodeSession sRemoteNodeSession =
                (RemoteNodeSession)mRemoteNodeSessionMap.remove(
                        new Integer(aRemoteNodeSessionId));
        
        if (sRemoteNodeSession == null)
        {
            return;
        }
        
        sRemoteNodeSession.close();

        Map sRemoteStatementMap = sRemoteNodeSession.getRemoteStatementMap();

        Iterator sIterator = sRemoteStatementMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            Long sRemoteStatementId = (Long)sEntry.getKey();

            mRemoteStatementIdMap.remove(sRemoteStatementId);
        }
    }
    
    /**
     * Close a specific remote statement
     * 
     * @param aRemoteStatementId        Remote statement ID
     */
    public synchronized void closeRemoteStatement(long aRemoteStatementId)
    {
        RemoteNodeSession sRemoteNodeSession =
                getRemoteNodeSessionFromRemoteStatement(aRemoteStatementId);
        
        if (sRemoteNodeSession != null)
        {
            sRemoteNodeSession.closeRemoteStatement(aRemoteStatementId);
        }
        
        mRemoteStatementIdMap.remove(new Long(aRemoteStatementId));
    }
}
