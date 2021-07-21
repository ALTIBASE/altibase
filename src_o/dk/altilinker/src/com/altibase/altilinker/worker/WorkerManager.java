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

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.SessionManager;
import com.altibase.altilinker.util.*;

/**
 * Worker manager component class to manage control worker and remote worker
 */
public class WorkerManager
{
    static private TraceLogger mTraceLogger     = TraceLogger.getInstance();
    static private TaskFactory mTaskFactory     = TaskFactory.getInstance();
    
    private MainMediator       mMainMediator    = null;
    private SessionManager     mSessionManager  = null;
    
    private Worker             mControlWorker   = null;
    private Worker             mRemoteWorker    = null;
    private int                mThreadSleepTime = 0;

    /**
     * Constructor
     * 
     * @param aMainMediator     MainMediator object to send events to upper component
     * @param aSessionManager   SessionManager object to manage sessions
     */
    public WorkerManager(MainMediator   aMainMediator,
                         SessionManager aSessionManager)
    {
        mTraceLogger.logTrace("WorkerManager component created.");

        mMainMediator   = aMainMediator;
        mSessionManager = aSessionManager;
    }
    
    /**
     * Close for this component
     */
    public void close()
    {
        stop();
        
        mTraceLogger.logTrace("WorkerManager component closed.");
    }
  
    /**
     * Set thread sleep time as millisecond unit
     * 
     * @param aThreadSleepTime  Thread sleep time as millisecond unit
     */
    public void setThreadSleepTime(int aThreadSleepTime)
    {
        mThreadSleepTime = aThreadSleepTime;
    }
    
    /**
     * Start control worker
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean startControlWorker()
    {
        if (mControlWorker != null)
        {
            return true;
        }

        mControlWorker = new ControlWorker();
        mControlWorker.setThreadSleepTime(mThreadSleepTime);
        
        if (mControlWorker.start() == false)
        {
            mControlWorker = null;
            return false;
        }

        return true;
    }
    
    /**
     * Start remote worker
     * 
     * @param aThreadCount      Thread count to create threads on thread pool
     * @return                  true, if success. false, if fail.
     */
    public boolean startRemoteWorker(int aThreadCount)
    {
        if (mRemoteWorker != null)
        {
            return true;
        }
        
        mRemoteWorker = new RemoteWorker();
        mRemoteWorker.setThreadCount(aThreadCount);
        mRemoteWorker.setThreadSleepTime(mThreadSleepTime);
        
        if (mRemoteWorker.start() == false)
        {
            mRemoteWorker = null;
            return false;
        }
        
        return true;
    }
    
    /**
     * Stop control worker
     */
    public void stopControlWorker()
    {
        if (mControlWorker == null)
        {
            return;
        }
        
        mControlWorker.stop();
        mControlWorker = null;
    }
    
    /**
     * Stop remote worker
     */
    public void stopRemoteWorker()
    {
        if (mRemoteWorker == null)
        {
            return;
        }
        
        mRemoteWorker.stop();
        mRemoteWorker = null;
    }
    
    /**
     * Stop all of workers
     */
    public void stop()
    {
        // stop control worker
        stopControlWorker();
        
        // stop remote worker
        stopRemoteWorker();
    }
    
    /**
     * Request to process a operation
     * 
     * @param aOperation        Operation object
     * @return                  true, if success. false, if fail.
     */
    public boolean requestOperation(Operation aOperation)
    {
        boolean sControlOperation = aOperation.isControlOperation();

        if (sControlOperation == true)
        {
            if (mControlWorker == null)
            {
                mTraceLogger.logError(
                        "WorkerManager component did not start control worker to process control operation.");
            
                return false;
            }
        }
        else
        {
            if (mRemoteWorker == null)
            {
                mTraceLogger.logError(
                        "WorkerManager component did not start remote worker to process remote operation.");
            
                return false;
            }
        }
        
        Task sNewTask = mTaskFactory.newTask(aOperation.getOperationId());
        if (sNewTask == null)
        {
            mTraceLogger.logFatal(
                    "Unknown task. (Operation Id = {0})",
                    new Object[] { new Integer(aOperation.getOperationId()) } );

            System.exit(-1);
            
            return false;
        }

        sNewTask.setOperation(aOperation);
        sNewTask.setSeqNo( aOperation.getSeqNo() );
        sNewTask.setMainMediator(mMainMediator);
        sNewTask.setSessionManager(mSessionManager);
        
        boolean sSuccess = false;
        
        if (sControlOperation == true)
        {
            sSuccess = mControlWorker.requestTask(sNewTask);
        }
        else
        {
            sSuccess = mRemoteWorker.requestTask(sNewTask);
        }
        
        if (sSuccess == false)
        {
            sNewTask.setOperation(null);
            sNewTask.close();
        }
        
        return sSuccess;
    }
    
    /**
     * Un-assign to a thread for linker session
     * 
     * @param aLinkerSessionId  Linker session ID
     */
    public void unassignToThreadPool(int aLinkerSessionId)
    {
        if (mRemoteWorker != null)
        {
            mRemoteWorker.unassignToThreadPool(aLinkerSessionId);
        }
    }
    
    /**
    * Get RemoteWorker ThreadPool Stack Trace
    * 
    * @return  true, if success. false, if fail.
    */
    public boolean getThreadDumpLog()
    {
        boolean sChkDumpLog = false;

        if (mRemoteWorker != null)
        {
    	    sChkDumpLog = mRemoteWorker.getThreadDumpLog();
        }
        else
        {
            mTraceLogger.logError(
                        "WorkerManager component did not start remote worker to process remote operation.");
        }

        return sChkDumpLog;
    }
    
    /**
     *  BUG-39001
     *  Drop Task from TaskThread
     *  
     *  @param aLinkserSessionID    Linker Session ID
     */
    public void dropTask( int aLinkerSessionID )
    {
        /*
         * Control Worker must not drop task. 
         */
        
        mRemoteWorker.dropTask( aLinkerSessionID );
    }
}
