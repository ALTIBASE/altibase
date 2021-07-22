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

import java.util.logging.Level;
import java.util.logging.LogRecord;

import com.altibase.altilinker.util.*;

/**
 * Task thread class for processing task by task queue
 */
public class TaskThread implements Runnable
{
    private static final int   DefaultSleepTime = 10; // 10ms
    
    private static TraceLogger mTraceLogger     = TraceLogger.getInstance();

    private boolean            mDaemon          = true;
    private String             mThreadName      = null;
    private long               mThreadId        = 0;
    private int                mThreadSleepTime = DefaultSleepTime;
    private volatile Thread    mThread          = null;
    private volatile boolean   mStopped         = false;
    private TaskQueue          mTaskQueue       = null;
    private Task               mCurrentTask     = null; // current processing task
    
    /**
     * Constructor
     */
    public TaskThread()
    {
        // nothing to do
    }
    
    /**
     * Get thread ID
     * 
     * @return  Thread ID
     */
    public long getThreadId()
    {
        return mThreadId;
    }
    
    /**
     * Set daemon thread
     * 
     * @param aDaemon   Whether daemon thread or not
     */
    public void setDaemon(boolean aDaemon)
    {
        mDaemon = aDaemon;
    }
    
    /**
     * Set thread name
     * 
     * @param aThreadName       Thread name
     */
    public void setThreadName(String aThreadName)
    {
        mThreadName = aThreadName;
    }
    
    /**
     * Get thread name
     * 
     * @return  Thread Name
     */
    public String getThreadName()
    {
        return mThreadName;
    }
    
    /**
     * Set thread sleep time as millisecond unit
     * 
     * @param aThreadSleepTime  Thread sleep time as millisecond unit
     */
    public void setThreadSleepTime(int aThreadSleepTime)
    {
        if (aThreadSleepTime > 0)
        {
            mThreadSleepTime = aThreadSleepTime;
        }
    }
    
    /**
     * Start thread
     */
    public void start()
    {
        mStopped = false;
        
        if (mTaskQueue == null)
        {
            mTaskQueue = new TaskQueue();
        }
        
        if (mThread == null)
        {
            mThread = new Thread(this);
            mThread.setDaemon(mDaemon);

            if (mThreadName != null)
            {
                mThread.setName(mThreadName);
            }
            
            mThread.start();
        }
    }
    
    /**
     * Stop thread
     */
    public void stop()
    {
        /* BUG-37575 */
        if ( mThread == null )
        {
            return;
        }
        
        mThread    = null;
        mTaskQueue = null;
    }
    
    /**
     * Request to process a task
     * 
     * @param aTask     Task to process
     * @return          true, if task queue enqueue successfully. false, if fail.
     */
    public boolean requestTask(Task aTask)
    {
        if (mThread == null || mTaskQueue == null)
        {
            return false;
        }
        
        mTaskQueue.enqueue(aTask);
        
        return true;
    }

    /**
     * Clear enqueued tasks
     */
    public void clearEnqueuedTasks()
    {
        if (mTaskQueue != null)
        {
            mTaskQueue.clear();
        }
    }
    
    /**
     * Get current processing task
     * 
     * @return  Current processing task
     */
    public Task getCurrentTask()
    {
        return mCurrentTask;
    }
    
    /**
     * Thread run method
     */
    public void run()
    {
        // obtain thread ID by logger
        obtainThreadId();
        
        Thread sThisThread = Thread.currentThread();
        Task   sTask;

        while (mThread == sThisThread)
        {
            try
            {
                sTask = mTaskQueue.dequeue();
                if (sTask == null)
                {
                    Thread.sleep(mThreadSleepTime);
                    continue;
                }
                
                mCurrentTask = sTask;

                try
                {
                    
                    sTask.run();
                }
                catch (Exception e)
                {
                    mTraceLogger.log(e);
                }

                mCurrentTask = null;
                
                sTask.close();
            }
            catch (InterruptedException e)
            {
                mTraceLogger.log(e);
            }
        }
        
        mStopped = true;
    }
    
    /**
     * Obtain thread ID
     */
    private void obtainThreadId()
    {
        mThreadId = (new LogRecord(Level.OFF, "")).getThreadID();
    }
    
    /**
     * Get Thread Object
     * 
     * @return  Current Thread Object
     */
    public Thread getThread()
    {
        return mThread;
    }
    
    /**
     * Get Thread Task Queue Count
     * 
     * @return  Current Thread Task Queue Count
     */
    public int getTaskQueueCount()
    {
        return mTaskQueue.size();
    }
    
    /**
     * Get Task Queue List
     * 
     * @return  Current Thread Task Queue List 
     */
    public String getTaskQueueList()
    {
        String sStrContent = "";
        
        if(mTaskQueue.empty() != false)
        {
            sStrContent = mTaskQueue.getCurrentAllTask();
        }
        
        return sStrContent;
    }
    
    /**
     *  BUG-39001
     *  Drop Task from TaskThread
     *  
     *  @param aLinkserSessionID    Linker Session ID
     */
    public void dropTask( int aLinkerSessionID )
    {
        mTaskQueue.dropTask( aLinkerSessionID );
    }
}
