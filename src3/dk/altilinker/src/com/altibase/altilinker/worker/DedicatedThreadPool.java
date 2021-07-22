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

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import com.altibase.altilinker.session.LinkerDataSession;
import com.altibase.altilinker.session.SessionManager;
import com.altibase.altilinker.util.TraceLogger;

/**
 * Thread pool class dedicated for each linker session
 */
public class DedicatedThreadPool implements ThreadPool
{
    class ThreadEntry
    {
        public int        mLinkerDataSessionCount = 0;
        public TaskThread mTaskThread             = null;
    }
    
    private static TraceLogger mTraceLogger     = TraceLogger.getInstance();
    
    private boolean            mDaemon          = true;
    private int                mThreadSleepTime = 0;
    private int                mThreadCount     = 0;
    private ThreadEntry[]      mPooledThreads   = null;
    private HashMap            mThreadAssignMap = null; // <linker data session id, TaskThread>
    
    /**
     * Constructor
     */
    public DedicatedThreadPool()
    {
        // nothing to do
    }

    /**
     * Set daemon thread
     * 
     * @param aDaemon   true, if daemon set. false, if not.
     */
    public void setDaemon(boolean aDaemon)
    {
        mDaemon = aDaemon;
    }
    
    /**
     * Set thread count in this thread pool
     * 
     * @param aThreadCount      Thread count
     */
    public void setThreadCount(int aThreadCount)
    {
        mThreadCount = aThreadCount;
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
     * Start worker
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start()
    {
        if (mPooledThreads != null)
        {
            return true;
        }

        mThreadAssignMap = new HashMap();
        mPooledThreads   = new ThreadEntry[mThreadCount];

        for ( int i = 0; i < mThreadCount; ++i )
        {
        	TaskThread sThread = new TaskThread();
        	sThread.setThreadName( "Remote Worker Thread " + (i + 1) );
        	sThread.setDaemon( mDaemon );
        	sThread.setThreadSleepTime( mThreadSleepTime );
        	sThread.start();
        	
        	mPooledThreads[i] = new ThreadEntry();
        	mPooledThreads[i].mLinkerDataSessionCount = 0;
        	mPooledThreads[i].mTaskThread = sThread;
        }
        
        return true;
    }
    
    /**
     * Stop worker
     */
    public void stop()
    {
        if (mPooledThreads == null)
        {
            return;
        }
        
        int i;
        
        for (i = 0; i < mThreadCount; ++i)
        {
            mPooledThreads[i].mTaskThread.stop();
            mPooledThreads[i].mLinkerDataSessionCount = 0;
            mPooledThreads[i].mTaskThread = null;
        }
        
        mPooledThreads = null;
        mThreadAssignMap = null;
    }
    
    /**
     * Request to process a task
     * 
     * @param aTask     Task to process
     * @return          true, if task queue enqueue successfully. false, if fail.
     */
    public boolean requestTask(Task aTask)
    {
        int sLinkerSessionId = aTask.getLinkerSessionId();
        Integer sKey = new Integer(sLinkerSessionId);

        ThreadEntry sThreadEntry = null;
        
        synchronized( this )
        {
            sThreadEntry = (ThreadEntry)mThreadAssignMap.get( sKey );
            if ( sThreadEntry == null )
            {
                sThreadEntry = assign( sKey, aTask );
                assert( sThreadEntry != null );
            }
            else
            {
                /* do nothing */
            }
        }
       
       return sThreadEntry.mTaskThread.requestTask(aTask);
    }

    /**
     * Get minimum assigned thread for linker data sessions
     * 
     * @return TaskThread object
     */
    private ThreadEntry getMinimumAssignedThread()
    {           
        ThreadEntry sMinimumThreadAssignEntry = null;
        
        sMinimumThreadAssignEntry = mPooledThreads[0];
        for ( int i = 1; i < mThreadCount; i++ )
        {
            ThreadEntry sThreadEntry = mPooledThreads[i];

            if ( sMinimumThreadAssignEntry.mLinkerDataSessionCount == 0 )
            {
                break;
            }
            else
            {
                /* do nothing */
            }

            if ( sMinimumThreadAssignEntry.mLinkerDataSessionCount > 
                 sThreadEntry.mLinkerDataSessionCount )
            {
                sMinimumThreadAssignEntry = sThreadEntry;
            }
            else
            {
                /* do nothing */
            }
        }

        return sMinimumThreadAssignEntry;
    }

    /**
     * Clear enqueued tasks
     */
    public void clearEnqueuedTasks()
    {
        int i;
        for (i = 0; i < mPooledThreads.length; ++i)
        {
            mPooledThreads[i].mTaskThread.clearEnqueuedTasks();
        }
    }

    /**
     * Get current task
     * 
     * @return  Current task list
     */
    public LinkedList getCurrentTasks()
    {
        LinkedList sLinkedList = new LinkedList();

        int i;
        
        for (i = 0; i < mPooledThreads.length; ++i)
        {
            Task sCurrentTask = mPooledThreads[i].mTaskThread.getCurrentTask();
            if (sCurrentTask != null)
            {
                sLinkedList.add(sCurrentTask);
            }
        }
        
        return sLinkedList;
    }
    
    private ThreadEntry assign( Integer aKey, Task aTask )
    {
    	ThreadEntry sThreadEntry = getMinimumAssignedThread();
    	
        if ( sThreadEntry != null )
        {
            synchronized( this )
            {
                mThreadAssignMap.put( aKey, sThreadEntry );
                sThreadEntry.mLinkerDataSessionCount++;
            }
           
            // set the assigned thread ID of linker data session
            SessionManager sSessionManager = aTask.getSessionManager();
            LinkerDataSession sLinkerDataSession = sSessionManager.getLinkerDataSession( aTask.getLinkerSessionId() );

            sLinkerDataSession.setThreadId( sThreadEntry.mTaskThread.getThreadId() );
            mTraceLogger.logInfo( 
                "New linker data session was assigned to one of thread pool. (Linker data session Id = {0}, Thread Id = {1})",
                new Object[] { aKey, new Long(sThreadEntry.mTaskThread.getThreadId()) } );
        }
        else
        {
            mTraceLogger.logFatal(
                    "New linker data session was not able to assign to a specific thread. (Linker Data Session Id = {0})",
                    new Object[] { aKey } );
            
            System.exit(-1);
        }

        return sThreadEntry;
    }
    
    /**
     * Un-assign thread for linker session
     */
    public void unassign(int aLinkerSessionId)
    {
        ThreadEntry sThreadEntry = null;
        Integer sKey = new Integer(aLinkerSessionId);
        
        synchronized (this)
        {
            sThreadEntry = (ThreadEntry)mThreadAssignMap.remove(sKey);
            sThreadEntry.mLinkerDataSessionCount--;
        }
        
        if ( sThreadEntry != null )
        {
            mTraceLogger.logInfo(
                    "Linker data session was unassigned to one of thread pool. (Linker data session Id = {0}, Thread Id = {1})",
                    new Object[] { sKey, new Long(sThreadEntry.mTaskThread.getThreadId()) });
        }
    }

    /**
     * RemoteWorker ThreadPool Stack Trace
     *
     * @return  true, if success. false, if fail.
     */
    public boolean printAllStackTraces()
    {
        File sDumpFile = new File(mTraceLogger.getDirPath() + File.separator + mTraceLogger.getDumpFileName());
    	FileWriter sFileWriter = null;
    	String sStrContent = "";
    
        try
        {
    	    if (sDumpFile.exists())
    	    {
    	        sDumpFile.delete();
    	    }
    	     
    	    for (int i = 0; i < mThreadCount; ++i)
            {
                TaskThread sTaskThread = mPooledThreads[i].mTaskThread;
                Thread sTh = sTaskThread.getThread();

                sStrContent += "Thread Name : " + sTh.getName() + "\n";
                sStrContent += "Current Task : " + sTaskThread.getCurrentTask() +  "\n";
                sStrContent += "Thread Queue Count : " + sTaskThread.getTaskQueueCount() + "\n";
                sStrContent += "Thread Queue Task : " + sTaskThread.getTaskQueueList()+ "\n"; 	
                sStrContent += "Linker data session ID : ";

                Iterator sIter = mThreadAssignMap.entrySet().iterator();
                while (sIter.hasNext())
                {
                    Entry entry = (Entry) sIter.next();
                    ThreadEntry sThreadEntry = (ThreadEntry)entry.getValue();
                    if ( sTh.getName().equals(sThreadEntry.mTaskThread.getThreadName()) )
                    {
                        sStrContent += entry.getKey() + " ";
                    }
                }
                sStrContent += "\n";

                sStrContent += "Thread Dump\n";
                /* 
                 * JDK 1.5 Thread getStackTrace() API
                 */
                StackTraceElement[] strace = sTh.getStackTrace();
                for ( int j = 0; j < strace.length; j++ )
                {
                    sStrContent += "\tat " + strace[j] + "\n";
                }

                sStrContent += "\n";
            }

            sFileWriter = new FileWriter(sDumpFile);
            sFileWriter.write(sStrContent);
            sFileWriter.close();

            return true;
        }
        catch (IOException e)
        {
            mTraceLogger.log(e);
    	    return false;
        }
    }
    
    /**
     *  BUG-39001
     *  Drop Task from TaskThread
     *  
     *  @param aLinkserSessionID    Linker Session ID
     */
    public void dropTask( int aLinkerSessionID )
    {
        Integer         sKey = new Integer( aLinkerSessionID );
        ThreadEntry     sThreadEntry = null;
        boolean         sIsExisted = false;
        
        synchronized (this)
        {
            sThreadEntry = (ThreadEntry)mThreadAssignMap.get( sKey );
            if (sThreadEntry != null)
            {   
                sIsExisted = true;
            }
            else
            {
                mTraceLogger.logDebug( "The TaskThread is not assigned. ( Linker Session ID : {0} )",
                                        new Object[] { sKey } );
                sIsExisted = false;
            }
        }
        
        if ( sIsExisted == true )
        {
            sThreadEntry.mTaskThread.dropTask( aLinkerSessionID );
        }
        else
        {
            /* do nothing */
        }
        
        return;
    }     
}
