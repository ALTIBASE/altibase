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
 
package com.altibase.altilinker.controller;

import java.util.LinkedList;
import java.util.Timer;
import java.util.TimerTask;

import com.altibase.altilinker.MainApp;
import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.type.DBCharSet;
import com.altibase.altilinker.adlp.type.Properties;
import com.altibase.altilinker.jdbc.JdbcDataManager;
import com.altibase.altilinker.jdbc.JdbcDriverManager;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.*;
import com.altibase.altilinker.worker.*;

/**
 * Main controller component class to control all of components
 */
public class MainController implements MainMediator
{
    public static final int LinkerStatusInactive          = 0;
    public static final int LinkerStatusListening         = 1;
    public static final int LinkerStatusRunning           = 2;
    public static final int LinkerStatusPreparingShutdown = 3;
    public static final int LinkerStatusPreparedShutdown  = 4;
    public static final int LinkerStatusShutdowning       = 5;
    
    private static final int TIMER_ALTIBASE_ALIVE_CHECK_PERIOD = 200; // 200 ms

    private static TraceLogger     mTraceLogger     = TraceLogger.getInstance();
    private static PropertyManager mPropertyManager = PropertyManager.getInstance();
    
    private int             mLinkerStatus    = LinkerStatusInactive;
    private ProtocolManager mProtocolManager = null;
    private SessionManager  mSessionManager  = null;
    private WorkerManager   mWorkerManager   = null;
    private Timer           mTimer           = null;

    private String          mProductInfo     = "";
    private int             mDBCharSet       = DBCharSet.None;
    
    /**
     * Constructor
     */
    public MainController()
    {
        mTraceLogger.logTrace("MainController component created.");

        // set trace log directory to property manager
        String sTraceDir = mTraceLogger.getDirPath();
        mPropertyManager.setTraceLogFileDir(sTraceDir);
        
        String sProtocolVersion = ProtocolManager.getProtocolVersionString();
        mTraceLogger.logInfo("Protocol Version: " + sProtocolVersion);

        // initialize components
        mProtocolManager = new ProtocolManager(this);
        mSessionManager  = new SessionManager();
        mWorkerManager   = new WorkerManager(this, mSessionManager);
    }
    
    /**
     * Close for this component
     */
    public void close()
    {
        stop();

        // finalize components
        mWorkerManager.close();
        mSessionManager.close();
        mProtocolManager.close();
        
        mWorkerManager   = null;
        mSessionManager  = null;
        mProtocolManager = null;
        
        mTraceLogger.logTrace("MainController component closed.");
    }
    
    /**
     * Start protocol manager and worker
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start()
    {
        boolean sSuccess = false;
        
        // start protocol manager
        sSuccess = mProtocolManager.start();
        if (sSuccess == false)
        {
            mTraceLogger.logError("Failed to start protocol manager.");
            
            return false;
        }
        
        // start control worker
        sSuccess = mWorkerManager.startControlWorker();
        if (sSuccess == false)
        {
            mProtocolManager.stop();
            
            mTraceLogger.logError("Failed to start control worker.");
            
            return false;
        }

        mTraceLogger.logInfo("MainController component started.");
        
        return true;
    }

    /**
     * Start TCP listening
     * 
     * @param aListenPort       TCP listen port
     * @return                  true, if success. false, if fail.
     */
    public boolean startListen(int aListenPort)
    {
        // set listen port to property manager
        mPropertyManager.setListenPort(aListenPort);

        // start TCP listening
        boolean sSuccess = mProtocolManager.startListen(aListenPort, "");
        if (sSuccess == false)
        {
            mTraceLogger.logError("Failed to listen as {0} port number.",
                    new Object[] { new Integer(aListenPort) } );
        }
        else
        {
            mTraceLogger.logInfo(
                    "TCP listening was started as {0} port number.",
                    new Object[] { new Integer(aListenPort) } );

            setLinkerStatus(LinkerStatusListening);
        }
        
        return sSuccess;
    }
    
    /**
     * Stop TCP listening
     */
    public void stopListen()
    {
        // stop TCP listening
        mProtocolManager.stopListen();
        
        mTraceLogger.logInfo("TCP listening stopped.");
    }
    
    /**
     * Prepare to stop ALTILINKER
     * 
     * @param aForce    true, if remote statement is alive and stop force.
     * @return          true, if success. false, if fail.
     */
    public boolean prepareStop(boolean aForce)
    {
        if (getLinkerStatus() >= LinkerStatusPreparedShutdown)
        {
            return true;
        }
        
        if (aForce == false)
        {
            // check for executing remote statements
            int sRemoteStatementCount = mSessionManager.getRemoteStatementCount();
            
            if (sRemoteStatementCount > 0)
            {
                mTraceLogger.logWarning(
                        "Remote statements is executing. (Count = {0})",
                        new Object[] { new Integer(sRemoteStatementCount) } );
                
                return false;
            }
        }
        
        setLinkerStatus(LinkerStatusPreparingShutdown);

        // stop TCP listening
        stopListen();
        
        // stop remote worker
        // (drop enqueued tasks and after, abort current remote statement)
        mWorkerManager.stopRemoteWorker();
        
        // close all of remote node sessions and all of remote statements
        mSessionManager.closeRemoteNodeSession();

        mTraceLogger.logInfo("MainController component prepared to stop.");

        setLinkerStatus(LinkerStatusPreparedShutdown);
        
        return true;
    }

    /**
     * Stop ALTILINKER
     */
    public void stop()
    {
        prepareStop(true);

        setLinkerStatus(LinkerStatusShutdowning);

        // stop timer
        stopTimer();
        
        // start protocol manager
        mProtocolManager.stop();

        // stop control worker
        mWorkerManager.stopControlWorker();

        mTraceLogger.logInfo("WorkerManager component stopped.");
    }
    
    /**
     * Set linker status
     * 
     * @param aLinkerStatus     Linker status
     */
    private void setLinkerStatus(int aLinkerStatus)
    {
        boolean sSetError = false;
        
        synchronized (this)
        {
            if (mLinkerStatus >= aLinkerStatus)
            {
                sSetError = true;
            }
            else
            {
                mLinkerStatus = aLinkerStatus;
            }
        }
        
        if (sSetError == true)
        {
            mTraceLogger.logError(
                    "Linker status change error (Linker status now: {0}, Linker status to set: {1})",
                    new Object[] {
                            new Integer(mLinkerStatus),
                            new Integer(aLinkerStatus) });
            
            return;
        }

        mTraceLogger.logInfo(
                "Linker status changed. (Linker Status = {0})",
                new Object[] { new Integer(aLinkerStatus) } );
    }
    
    /**
     * Get linker status
     * 
     * @return  Linker status
     */
    public int getLinkerStatus()
    {
        return mLinkerStatus;
    }
    
    /**
     * Compare linker status
     * 
     * @param aLinkerStatus     Linker status to compare
     * @return                  true, if equals. false, if not.
     */
    public boolean isLinkerStatus(int aLinkerStatus)
    {
        if (getLinkerStatus() != aLinkerStatus)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Start timer
     * 
     * @return  true, if success. false, if not.
     */
    private boolean startTimer()
    {
        if (mTimer != null)
        {
            return true;
        }
        
        mTimer = new Timer(true); // daemon timer

        mTraceLogger.logInfo("Timer was started.");
        
        // ALTIBASE process alive checking
        mTimer.scheduleAtFixedRate(new AltibaseProcessAliveCheckerTask(),
                                   0,
                                   TIMER_ALTIBASE_ALIVE_CHECK_PERIOD);

        return true;
    }
    
    /**
     * Stop timer
     */
    private void stopTimer()
    {
        if (mTimer == null)
        {
            return;
        }
        
        mTimer.cancel();
        mTimer = null;
        
        mTraceLogger.logInfo("Timer stopped.");
    }
    
    /**
     * ALTIBASE process alive checker task class
     */
    class AltibaseProcessAliveCheckerTask extends TimerTask
    {
        public AltibaseProcessAliveCheckerTask()
        {
            mTraceLogger.logInfo("ALTIBASE process alive checking started.");
        }
        
        public void run()
        {
            if (mSessionManager.isCreatedLinkerCtrlSession() == false)
            {
                mTraceLogger.logError(
                        "Alive checker detected linker control session destroyed.");

                // shutdown ALTILINKER
                try
                {
                    MainApp.shutdown();
                }
                catch ( Exception e )
                {
                    System.exit( -1 );
                }
            }
        }
    }

    /**
     * Created linker control session event from lower components
     */
    public void onCreatedLinkerCtrlSession()
    {
        mTraceLogger.logInfo("Linker control session created.");

        // start remote worker
        mWorkerManager.startRemoteWorker(mPropertyManager.getThreadCount());

        // start ALTIBASE process alive checking
        startTimer();

        setLinkerStatus(LinkerStatusRunning);
    }
    
    /**
     * Created linker data session event from lower components
     * 
     * @param aSessionId        Linker data session ID
     */
    public void onCreatedLinkerDataSession(int aSessionId)
    {
        mTraceLogger.logInfo(
                "Linker data session created. (Linker data session Id = {0})",
                new Object[] { new Integer(aSessionId) } );
    }

    /**
     * Set product info event from lower components
     * 
     * @param aProductInfo      Product info
     * @param aDBCharSet        Database character set of ALTIBASE server
     */
    public void onSetProductInfo(String aProductInfo, int aDBCharSet)
    {
        mProductInfo = aProductInfo;
        mDBCharSet   = aDBCharSet;
        
        mTraceLogger.logInfo(
                "Product: " + mProductInfo + ", DB CharSet: " + mDBCharSet);

        // set database character set of ALTIBASE server
        JdbcDataManager.setDBCharSet(mDBCharSet);
    }
    
    /**
     * Load properties event for lower components 
     * 
     * @param aProperties       dblink.conf properties
     */
    public void onLoadProperties(Properties aProperties)
    {
        LinkedList sTargetList;
        LinkedList sValidTargetList;
        
        // set properties to property manager
        mPropertyManager.setProperties(aProperties);

        // get target list from properties
        sTargetList = mPropertyManager.getTargetList();
        
        // load all of JDBC drivers for targets and get valid target list
        sValidTargetList = JdbcDriverManager.loadDrivers(sTargetList);

        // set valid target list
        mPropertyManager.setTargetList(sValidTargetList);

        mTraceLogger.logInfo(
                "Loaded properties. ({0})", new Object[] { mPropertyManager.toString() } );
        
        // set login timeout (second unit)
        JdbcDriverManager.setLoginTimeout(
                mPropertyManager.getRemoteNodeReceiveTimeout());

        // set maximum remote node session count
        RemoteNodeSessionIdGenerator.getInstance().setMaxRemoteNodeSession(
                mPropertyManager.getRemoteNodeSessionCount());
        
        // set trace logger properties
        mTraceLogger.setFileLimit(mPropertyManager.getTraceLogFileSize(),
                                  mPropertyManager.getTraceLogFileCount());
        mTraceLogger.setLevel(mPropertyManager.getTraceLoggingLevel());
    }
    
    /**
     * Get ADLP protocol version event from lower components
     * 
     * @return  ADLP protocol version
     */
    public int onGetProtocolVersion()
    {
        return ProtocolManager.getProtocolVersion();
    }
    
    /**
     * Destroy linker session event from lower components
     * 
     * @param aSessionId        Linker session Id
     * @param aForce            true, if remote statement is alive and stop force.
     */
    public boolean onDestroyLinkerSession(int aSessionId, boolean aForce)
    {
        if (aForce == false)
        {
            // check for executing remote statements
            int sRemoteStatementCount = 
                    mSessionManager.getRemoteStatementCount(
                            aSessionId);
            
            if (sRemoteStatementCount > 0)
            {
                mTraceLogger.logWarning(
                        "Linker data session could not destroy, because remote statements of that was executing. (Linker data session Id = {0}, Count = {1})",
                        new Object[] { 
                                new Integer(aSessionId),
                                new Integer(sRemoteStatementCount) });
                
                return false;
            }
        }

        // destroy linker session in protocol manager
        mProtocolManager.destroyLinkerSession(aSessionId);

        onUnassignToThreadPool( aSessionId );
        
        mTraceLogger.logInfo(
                "Linker session destroyed. (Linker Session Id = {0})",
                new Object[] { new Integer(aSessionId) } );
        
        return true;
    }
    
    /**
     * Get linker status event from lower components
     * 
     * @return Linker status
     */
    public int onGetLinkerStatus()
    {
        return getLinkerStatus();
    }
    
    /**
     * Prepare to shutdown ALTILINKER event from lower components
     * 
     * @return true, if prepare to shutdown. false, if fail.
     */
    public boolean onPrepareLinkerShutdown()
    {
        return prepareStop(false);
    }
    
    /**
     * Shutdown ALTILINKER event from lower components
     */
    public void onDoLinkerShutdown()
    {
        try
        {
            MainApp.shutdown();
        }
        catch ( Exception e )
        {
            System.exit( -1 );
        }
    }

    /**
     * Disconnected linker session from lower components
     * 
     * @param aSessionId        Linker session ID
     */
    public void onDisconnectedLinkerSession(int aSessionId)
    {
        mTraceLogger.logDebug(
                "Linker session disconnected. (Linker session Id = {0})",
                new Object[] { new Integer(aSessionId) } );

        // if session Id is 0, then this is linker control session.
        if (aSessionId == 0)
        {
            mSessionManager.destroyLinkerCtrlSession();
        }
    }
    
    /**
     * Operation send event from lower components
     * 
     * @param aOperation        Operation to send
     */
    public boolean onSendOperation(Operation aOperation)
    {
        // if AltiLinker is preparing to shutdown,
        // any remote operations cannot not send.
        if (getLinkerStatus() >= LinkerStatusPreparingShutdown)
        {
            if (aOperation.isRemoteOperation() == true)
            {
                mTraceLogger.logDebug(
                        "Remote operation could not send, because AltiLinker was preparing to shutdown. (Operation Id = {0})",
                        new Object[] { new Integer(aOperation.getOperationId()) } );
                
                return false;
            }
        }

        // send operation
        boolean sSuccess = mProtocolManager.sendOperation(aOperation);
        if (sSuccess == false)
        {
            mTraceLogger.logError(
                    "Operation did not send. (Operation Id = {0})",
                    new Object[] { new Integer(aOperation.getOperationId()) } );
        }
        else
        {
            if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
            {
                mTraceLogger.logDebug(
                        "Operation sent. (Operation Id = {0})",
                        new Object[] { new Integer(aOperation.getOperationId()) } );
            }
        }
        
        return sSuccess;
    }

    /**
     * Received operation event from lower components
     * 
     * @param aOperation        A received operation
     */
    public boolean onReceivedOperation(Operation aOperation)
    {
        // if AltiLinker is preparing to shutdown,
        // any remote operations cannot not process.
        if (getLinkerStatus() >= LinkerStatusPreparingShutdown)
        {
            if (aOperation.isRemoteOperation() == true)
            {
                mTraceLogger.logDebug(
                        "Remote operation could not process, because AltiLinker was preparing to shutdown. (Operation Id = {0})",
                        new Object[] { new Integer(aOperation.getOperationId()) } );
                
                return false;
            }
        }
        
        boolean sSuccess = mWorkerManager.requestOperation(aOperation);
        if (sSuccess == false)
        {
            mTraceLogger.logError(
                    "The received operation did not process. (Operation Id = {0})",
                    new Object[] { new Integer(aOperation.getOperationId()) } );
        }
        else
        {
            if (mTraceLogger.isLoggable(TraceLogger.LogLevelDebug) == true)
            {
                mTraceLogger.logDebug(
                        "The received operation requested to process to WorkerManager component. (Operation Id = {0})",
                        new Object[] { new Integer(aOperation.getOperationId()) } );
            }
        }
        
        return sSuccess;
    }
    
    /**
     * Get Thread Dump Log Information
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean onGetThreadDumpLog()
    {
    	boolean sChkDumpLog = false;

        sChkDumpLog = mWorkerManager.getThreadDumpLog();

        return sChkDumpLog;
    }
    
    /**
     *  BUG-39001
     *  Un-assign to a thread for linker session
     *  
     *  @param aLinkerSessionId  Linker session ID
     */
    public void onUnassignToThreadPool( int aSessionId )
    {
        if ( aSessionId != 0 )
        {   
            // un-assign to thread pool of remote worker
            mWorkerManager.unassignToThreadPool( aSessionId );
        }
        else
        {
            /* do nothing */
        }
    }
    
    /**
     *  BUG-39001
     *  Drop Task from TaskThread
     *  
     *  @param aLinkerSessionId  Linker session ID
     */
    public void onDropTask( int aLinkerSessionID )
    {
        mWorkerManager.dropTask( aLinkerSessionID );
    }
}
