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
import java.util.LinkedList;

import com.altibase.altilinker.util.TraceLogger;
import com.altibase.altilinker.worker.task.TaskType;

/**
 * Remote worker class to process remote operations
 */
public class RemoteWorker implements Worker
{
    private static TraceLogger mTraceLogger       = TraceLogger.getInstance();
    
    private static final int   DefaultThreadCount = 16;
    private static final int   MinimumThreadCount = 2;
    
    private int                mThreadSleepTime   = 0;
    private int                mThreadCount       = DefaultThreadCount;
    private ThreadPool         mThreadPool        = null;
    private TaskThread         mAbortThread       = null;
    
    /**
     * Constructor
     */
    protected RemoteWorker()
    {
        mTraceLogger.logTrace("RemoteWorker component created.");
    }
    
    /**
     * Close for this component
     */
    public void close()
    {
        mTraceLogger.logTrace("RemoteWorker component closed.");
    }
    
    /**
     * Set thread count for creating thread pool
     * 
     * @param aThreadCount      Thread count to create thread pool
     */
    public void setThreadCount(int aThreadCount)
    {
        if (aThreadCount < MinimumThreadCount)
        {
            return;
        }
        
        mThreadCount = aThreadCount;
    }

    /**
     * Set thread sleep time as millisecond time
     * 
     * @param aThreadSleepTime  Thread sleep time as millisecond time
     */
    public void setThreadSleepTime(int aThreadSleepTime)
    {
        mThreadSleepTime = aThreadSleepTime;
    }
    
    /**
     * Start thread pool
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start()
    {
        if (mThreadPool != null)
        {
            return true;
        }
        
        mThreadPool = new DedicatedThreadPool();
        mThreadPool.setDaemon(true);
        mThreadPool.setThreadSleepTime(mThreadSleepTime);
        mThreadPool.setThreadCount(mThreadCount - 1);
        
        if (mThreadPool.start() == false)
        {
            mThreadPool = null;
            
            return false;
        }
        
        mAbortThread = new TaskThread();
        mAbortThread.setDaemon(true);
        mAbortThread.setThreadSleepTime(mThreadSleepTime);
        mAbortThread.start();

        return true;
    }
    
    /**
     * Stop thread pool
     */
    public void stop()
    {
        if (mThreadPool != null)
        {
            // clear all of enqueued tasks
            mThreadPool.clearEnqueuedTasks();

            // abort remote statement for current processing task
            LinkedList sCurrentTaskList = mThreadPool.getCurrentTasks();
            if (mAbortThread != null)
            {
                Iterator sIterator = sCurrentTaskList.iterator();
                while (sIterator.hasNext() == true)
                {
                    Task sTask = (Task)sIterator.next();

                    Task sAbortTask = sTask.newAbortTask();
                    if (sAbortTask != null)
                    {
                        sAbortTask.setSendResultOperation(false);
                        
                        mAbortThread.requestTask(sAbortTask);
                    }
                }
            }
            
            mThreadPool.stop();
            mThreadPool = null;
        }

        if (mAbortThread != null)
        {
            mAbortThread.stop();
            mAbortThread = null;
        }
    }

    /**
     * Request to process a task
     * 
     * @param aTask     Task to process
     * @return          true, if task queue enqueue successfully. false, if fail.
     */
    public boolean requestTask(Task aTask)
    {
        if (mThreadPool == null || mAbortThread == null)
        {
            return false;
        }
        
        if (aTask.getType() == TaskType.AbortRemoteStatement)
        {
            mAbortThread.requestTask(aTask);
        }
        else
        {
            mThreadPool.requestTask(aTask);
        }
        
        return true;
    }
    
    /**
     * Un-assign to a thread for linker session
     * 
     * @param aLinkerSessionId  Linker session ID
     */
    public void unassignToThreadPool(int aLinkerSessionId)
    {
        if (mThreadPool != null)
        {
            mThreadPool.unassign(aLinkerSessionId);
        }
    }

    /**
     * RemoteWorker ThreadPool Stack Trace
     *
     * @return  true, if success. false, if fail.
     */
    public boolean getThreadDumpLog()
    {
        boolean sChkDumpLog = false;
	if (mThreadPool != null)
        {
	    sChkDumpLog =  mThreadPool.printAllStackTraces();
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
        mThreadPool.dropTask( aLinkerSessionID );
    }
}
