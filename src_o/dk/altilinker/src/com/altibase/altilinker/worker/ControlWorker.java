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

import com.altibase.altilinker.util.TraceLogger;

/**
 * Control worker class to process control operations
 */
public class ControlWorker implements Worker
{
    private static TraceLogger mTraceLogger     = TraceLogger.getInstance();

    private int                mThreadSleepTime = 0;
    private TaskThread         mTaskThread      = null;
    
    /**
     * Constructor
     */
    protected ControlWorker()
    {
        mTraceLogger.logTrace("ControlWorker component created.");
    }
    
    /**
     * Close for this component
     */
    public void close()
    {
        mTraceLogger.logTrace("ControlWorker component closed.");
    }

    /**
     * Nothing
     */
    public void setThreadCount(int aThreadCount)
    {
        // nothing to do
    }

    /**
     * Set thread sleep time
     * 
     * @param aThreadSleepTime  Thread sleep time as millisecond unit
     */
    public void setThreadSleepTime(int aThreadSleepTime)
    {
        mThreadSleepTime = aThreadSleepTime;
    }
    
    /**
     * Start worker
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start()
    {
        if (mTaskThread != null)
        {
            return true;
        }
        
        mTaskThread = new TaskThread();
        mTaskThread.setDaemon(false);
        mTaskThread.setThreadName("ControlWorker");
        mTaskThread.setThreadSleepTime(mThreadSleepTime);
        mTaskThread.start();
        
        return true;
    }
    
    /**
     * Stop worker
     */
    public void stop()
    {
        if (mTaskThread == null)
        {
            return;
        }
        
        mTaskThread.stop();
        mTaskThread = null;
    }

    /**
     * Request to process a task
     * 
     * @param aTask     Task to process
     * @return          true, if task queue enqueue successfully. false, if fail.
     */
    public boolean requestTask(Task aTask)
    {
        if (mTaskThread == null)
        {
            return false;
        }
        
        return mTaskThread.requestTask(aTask);
    }

    /**
     * Nothing
     */
    public void unassignToThreadPool(int aLinkerSessionId)
    {
        // nothing to do
    }
    
    /**
     * ControlWorker Thread Stack Trace
     *
     * @return  true, if success. false, if fail.
     */
    public boolean getThreadDumpLog()
    {
        // nothing to do
        return false;
    }
    
    /**
     *  BUG-39001
     *  Drop Task from TaskThread
     *  
     *  @param aLinkserSessionID    Linker Session ID
     */
    public void dropTask( int aLinkerSessionID )
    {
        mTaskThread.dropTask( aLinkerSessionID );        
    }     
}
