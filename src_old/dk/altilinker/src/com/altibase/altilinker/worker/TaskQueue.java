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

/**
 * Task queue for processing tasks by FIFO 
 */
public class TaskQueue
{
    protected static TraceLogger mTraceLogger         = TraceLogger.getInstance();
    
    private LinkedList mLinkedList = new LinkedList();
    
    /**
     * Constructor
     */
    public TaskQueue()
    {
        // nothing to do
    }
    
    /**
     * Enqueue a task
     * 
     * @param aTask     Task to process
     */
    public synchronized void enqueue(Task aTask)
    {
        mLinkedList.add(aTask);
    }
    
    /**
     * Dequeue a task
     * 
     * @return  Task object, if queue do not empty. null, if empty.
     */
    public synchronized Task dequeue()
    {
        if (mLinkedList.isEmpty() == true)
        {
            return null;
        }
        
        return (Task)mLinkedList.removeFirst();
    }
    
    /**
     * Get enqueued task count
     * 
     * @return Enqueued task count
     */
    public synchronized int size()
    {
        return mLinkedList.size();
    }
    
    /**
     * Whether queue is empty or not
     * 
     * @return  true, if queue is empty or not
     */
    public synchronized boolean empty()
    {
        return mLinkedList.isEmpty() ? true : false;
    }
    
    /**
     * Clear all of tasks
     */
    public synchronized void clear()
    {
        mLinkedList.clear();
    }
    
    /**
     * Get all tasks of mLinkedList
     */
    public synchronized String getCurrentAllTask()
    {
        String sTaskStr = "";
        Iterator sItr = mLinkedList.iterator();
        while (sItr.hasNext())
        {
            sTaskStr += (String) sItr.next() + " ";
        }
        
        return sTaskStr;
    }
    
    /**
     *  BUG-39001
     *  Drop Task from TaskQueue
     *  
     *  @param aLinkserSessionID    Linker Session ID
     */
    public synchronized void dropTask( int aLinkerSessionID )
    {
        Iterator sItr = mLinkedList.iterator();
        Task     sTask = null;
        
        while ( sItr.hasNext() )
        {
            sTask = (Task)sItr.next();
            
            if ( sTask.getLinkerSessionId() == aLinkerSessionID )
            {
                mTraceLogger.logInfo(
                        "The task( Type : {0}, Linker Session ID: {1}, Operation ID : {2} ) was dropped.",
                        new Object[] { new Integer( sTask.getType() ),
                                       new Integer( sTask.getLinkerSessionId() ),
                                       new Integer( sTask.getRequestOperationId() ) 
                                 } );
                
                sItr.remove();
            }
            else
            {
                /* do nothing */
            }            
        }        
    }     
}
