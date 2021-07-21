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

import java.util.LinkedList;

/**
 * Thread pool interface
 */
public interface ThreadPool
{
    /**
     * Set daemon thread
     * 
     * @param aDaemon   Whether daemon thread set or not
     */
    public void setDaemon(boolean aDaemon);
    
    /**
     * Set thread count
     * 
     * @param aThreadCount      Thread count
     */
    public void setThreadCount(int aThreadCount);
    
    /**
     * Set thread sleep time as millisecond unit
     * 
     * @param aThreadSleepTime  Thread sleep time as millisecond unit
     */
    public void setThreadSleepTime(int aThreadSleepTime);
    
    /**
     * Start thread pool
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start();
    
    /**
     * Stop thread pool
     */
    public void stop();
    
    /**
     * Request to process a task
     * 
     * @param aTask     Task to process
     * @return          true, if task queue enqueue successfully. false, if fail.
     */
    public boolean requestTask(Task aTask);

    /**
     * Clear enqueued tasks
     */
    public void clearEnqueuedTasks();

    /**
     * Get current processing task
     * 
     * @return  Current processing task
     */
    public LinkedList getCurrentTasks();
    
    /**
     * Un-assign to a thread for linker session
     * 
     * @param aLinkerSessionId  Linker session ID
     */
    public void unassign(int aLinkerSessionId);
    
    /**
     * Get RemoteWorker ThreadPool Stack Trace
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean printAllStackTraces();
    
    /**
     *  BUG-39001
     *  Drop Task from TaskThread
     *  
     *  @param aLinkserSessionID    Linker Session ID
     */
    public void dropTask( int aLinkerSessionID );
}
