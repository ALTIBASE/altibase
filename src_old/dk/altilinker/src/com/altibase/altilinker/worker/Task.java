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
 
package com.altibase.altilinker.worker;

import java.util.Iterator;
import java.util.Map;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.OperationFactory;
import com.altibase.altilinker.adlp.ResultOperation;
import com.altibase.altilinker.adlp.op.OpId;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.RemoteNodeSession;
import com.altibase.altilinker.session.SessionManager;
import com.altibase.altilinker.util.TraceLogger;
import com.altibase.altilinker.worker.task.TaskType;

/**
 * Task abstract class
 */
public abstract class Task implements Runnable
{
    protected static TraceLogger mTraceLogger         = TraceLogger.getInstance();

    private int                  mTaskType            = TaskType.None;
    private byte                 mRequestOperationId  = OpId.None;
    private int                  mLinkerSessionId     = 0;
    private MainMediator         mMainMediator        = null;
    private SessionManager       mSessionManager      = null;
    private boolean              mSendResultOperation = true;

    private long                 mSeqNo               = 0;
    private boolean              mXA                  = false;
    private boolean              mTxBegin             = false;
    
    /**
     * Constructor
     * 
     * @param aTaskType         Task type
     */
    public Task(int aTaskType)
    {
        mTaskType = aTaskType;
    }
    
    /**
     * Close for this object
     */
    void close()
    {
        // nothing to do
    }
    
    /**
     * Get task type
     * 
     * @return  Task type
     */
    public int getType()
    {
        return mTaskType;
    }
    
    /**
     * Get request operation ID
     * 
     * @return  Request operation ID
     */
    public byte getRequestOperationId()
    {
        return mRequestOperationId;
    }
    
    /**
     * Get linker session ID
     * 
     * @return  Linker session ID
     */
    public int getLinkerSessionId()
    {
        return mLinkerSessionId;
    }
    
    /**
     * Get MainMediator object
     * 
     * @return  MainMediator object
     */
    public MainMediator getMainMediator()
    {
        return mMainMediator;
    }
    
    /**
     * Get SessionManager object
     * 
     * @return SessionManager object
     */
    public SessionManager getSessionManager()
    {
        return mSessionManager;
    }
    
    /**
     * Whether result operation should send or not
     * 
     * @return  true, if result operation should send. false, if not.
     */
    public boolean isSendResultOperation()
    {
        return mSendResultOperation;
    }
    
    /**
     * Set request operation ID
     * 
     * @param aRequestOperationId       Request operation ID
     */
    public void setRequestOperationId(byte aRequestOperationId)
    {
        mRequestOperationId = aRequestOperationId;
    }
    
    /**
     * Set linker session ID
     * 
     * @param aLinkerSessionId  Linker session ID
     */
    public void setLinkerSessionId(int aLinkerSessionId)
    {
        mLinkerSessionId = aLinkerSessionId;
    }

    /**
     * Set MainMediator object for sending event to upper component.
     * 
     * @param aMainMediator     MainMediator object
     */
    public void setMainMediator(MainMediator aMainMediator)
    {
        mMainMediator = aMainMediator;
    }
    
    /**
     * Set SessionManager object for session management
     * 
     * @param aSessionManager   SessionManager object
     */
    public void setSessionManager(SessionManager aSessionManager)
    {
        mSessionManager = aSessionManager;
    }
    
    /**
     * Set operation for task processing
     * 
     * @param aOperation        Operation
     */
    public void setOperation(Operation aOperation)
    {
        mRequestOperationId = aOperation.getOperationId();
        mLinkerSessionId    = aOperation.getSessionId();
        mSeqNo              = aOperation.getSeqNo();
        setXA( aOperation.isXA() );
        setTxBegin( aOperation.isTxBegin() );
    }
    
    /**
     * Set flag whether result operation need to send or not
     * 
     * @param sSendResultOperation Send flag
     */
    public void setSendResultOperation(boolean sSendResultOperation)
    {
        mSendResultOperation = sSendResultOperation;
    }
    
    /**
     * Create new result operation for this request operation
     * 
     * @return New result operation
     */
    public ResultOperation newResultOperation()
    {
        OperationFactory sOperationFactory = OperationFactory.getInstance();
        
        ResultOperation sResultOperation = 
                (ResultOperation)sOperationFactory.newResultOperation(
                                                           mRequestOperationId);
        
        return sResultOperation;
    }
    
    /**
     * Create new abort task for remote statement abort
     * 
     * @return  New abort task
     */
    public Task newAbortTask()
    {
        return null;
    }
    
    /**
     * BUG-39201
     * Set SeqNo
     * 
     * @param aSeqNo
     */
    public void setSeqNo( long aSeqNo )
    {
        mSeqNo = aSeqNo;
    }
    
    /**
     * BUG-39201
     * Get SeqNo
     * 
     * @return SeqNo
     */    
    public long getSeqNo()
    {
        return mSeqNo;
    }
    
    public boolean isXA()
    {
        return mXA;
    }
    
    public void setXA( boolean aIsXA )
    {
        mXA = aIsXA;
    }
    
    public boolean isTxBegin()
    {
        return mTxBegin;
    }
    
    public void setTxBegin( boolean aIsTxBegin )
    {
        mTxBegin = aIsTxBegin;
    }

    public boolean isNotifySession()
    {
        return (mLinkerSessionId == SessionManager.NOTIFY_SESSION_ID ) ? true : false;
    }
       
    protected RemoteNodeSession findRemoteNodeSession( Map aRemoteNodeSessionMap, String aTargetName )
    {
        RemoteNodeSession sRemoteNodeSession = null;
        
        Iterator sRemoteNodeSessionIterator = aRemoteNodeSessionMap.entrySet().iterator();
        while ( sRemoteNodeSessionIterator.hasNext() == true )
        {
            Map.Entry sRemoteNodeSessionEntry = (Map.Entry)sRemoteNodeSessionIterator.next();

            RemoteNodeSession sCurrentRemoteNodeSession = 
                    (RemoteNodeSession)sRemoteNodeSessionEntry.getValue();
            
            if ( aTargetName.equals( sCurrentRemoteNodeSession.getTarget().mName ) == true)
            {
                sRemoteNodeSession = sCurrentRemoteNodeSession;
                break;
            }
        }
        
        return sRemoteNodeSession;
    }
    
}
