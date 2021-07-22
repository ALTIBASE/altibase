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
 
package com.altibase.altilinker.worker.task.test;

import java.util.LinkedList;

import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.*;
import com.altibase.altilinker.controller.*;
import com.altibase.altilinker.session.*;
import com.altibase.altilinker.util.*;
import com.altibase.altilinker.worker.task.CreateLinkerCtrlSessionTask;
import com.altibase.altilinker.worker.task.CreateLinkerDataSessionTask;
import com.altibase.altilinker.worker.task.ExecuteRemoteStatementTask;
import com.altibase.altilinker.worker.task.RequestFreeRemoteStatementTask;

public class MainMediatorTest implements MainMediator
{
    private LinkedList      mResultOperationList = new LinkedList();
    
    private TraceLogger     mTraceLogger         = TraceLogger.getInstance();
    private PropertyManager mPropertyManager     = PropertyManager.getInstance();
    private SessionManager  mSessionManager      = null;
    private Target          mTarget              = new Target();
    
    public MainMediatorTest()
    {
        mTraceLogger.setDirPath("$ALTIBASE_HOME/trc");
        mTraceLogger.setLevel(TraceLogger.LogLevelTrace);
        mTraceLogger.start();
        
        mSessionManager = new SessionManager();

        // for ALTIBASE v6
        mTarget.mName          = "test";
        mTarget.mDriver.mDriverPath = "/home/flychk/work/JDBCDrivers/Altibase6/Altibase.jar";
        mTarget.mConnectionUrl = "jdbc:Altibase://dbdm.altibase.in:21141/mydb";
        mTarget.mUser          = "SYS";
        mTarget.mPassword      = "MANAGER";

        // for ORACLE 10g (JDBC v10 - JDK 1.4)
//        mTarget.mName          = "test";
//        mTarget.mDriver        = "/home/flychk/work/JDBCDrivers/ojdbc14.jar";
//        mTarget.mConnectionUrl = "jdbc:oracle:thin:@dbdm.altibase.in:1521:ORCL";
//        mTarget.mUser          = "new_dblink";
//        mTarget.mPassword      = "new_dblink";

        // for ORACLE 11g (JDBC v11 - JDK 1.4, UX team)
//        mTarget.mName          = "test";
//        mTarget.mDriver        = "/home/flychk/work/JDBCDrivers/ojdbc14.jar";
//        mTarget.mConnectionUrl = "jdbc:oracle:thin:@192.168.3.149:1522:ORCL";
//        mTarget.mUser          = "testuser";
//        mTarget.mPassword      = "testuser";
        
        // for ORACLE 11g (JDBC v11 - JDK 1.5, UX team)
//        mTarget.mName          = "test";
//        mTarget.mDriver        = "/home/flychk/work/JDBCDrivers/ojdbc5.jar";
//        mTarget.mConnectionUrl = "jdbc:oracle:thin:@192.168.3.149:1522:ORCL";
//        mTarget.mUser          = "testuser";
//        mTarget.mPassword      = "testuser";
        
        // for ORACLE 10g (JDBC v10 - JDK 1.4)
//        mTarget.mName          = "test";
//        mTarget.mDriver        = "/home/flychk/work/JDBCDrivers/ojdbc14.jar";
//        mTarget.mConnectionUrl = "jdbc:oracle:thin:@dbdm.altibase.in:1521:ORCL";
//        mTarget.mUser          = "new_dblink";
//        mTarget.mPassword      = "new_dblink";
        
        // for MS-SQL Server 2008 (UX team)
//        mTarget.mName          = "test";
//        mTarget.mDriver        = "/home/flychk/work/JDBCDrivers/sqljdbc4.jar";
//        mTarget.mConnectionUrl = "jdbc:sqlserver://192.168.1.70:1020;databaseName=NEW_DBLINK";
//        mTarget.mUser          = "NEW_DBLINK";
//        mTarget.mPassword      = "NEW_DBLINK";
    }
    
    public void close()
    {
        mSessionManager.close();
        
        mSessionManager = null;
        mTarget         = null;
    }

    public void reset()
    {
        mResultOperationList.clear();
    }
    
    public SessionManager getSessionManager()
    {
        return mSessionManager;
    }
    
    public ResultOperation getResultOperation()
    {
        if (mResultOperationList.isEmpty() == true)
            return null;
        
        ResultOperation sResultOperation = (ResultOperation)mResultOperationList.getFirst();
        
        return sResultOperation;
    }

    public LinkedList getResultOperationList()
    {
        LinkedList sResultOperationList = (LinkedList)mResultOperationList.clone();
        
        return sResultOperationList;
    }
    
    public byte getResultCode()
    {
        ResultOperation sResultOperation = getResultOperation();
        if (sResultOperation == null)
            return ResultCode.None;
        
        return sResultOperation.getResultCode();
    }
    
    public void setTarget(Target aTarget)
    {
        if (aTarget != null)
            mTarget = aTarget;
    }
    
    public Target getTarget()
    {
        return mTarget;
    }
    
    public String getTargetName()
    {
        return mTarget.mName;
    }

    public String getTargetUser()
    {
        return mTarget.mUser;
    }

    public String getTargetPassword()
    {
        return mTarget.mPassword;
    }
    
    public void runCreateLinkerCtrlSessionTask(Target aTarget)
    {
        CreateLinkerCtrlSessionOp sOperation = new CreateLinkerCtrlSessionOp();
        
        sOperation.setSessionId(0);
        sOperation.mProductInfo = "ALTIBASE v7.1.1";
        sOperation.mDBCharSet   = DBCharSet.ASCII;
        sOperation.mProperties  = new Properties();
        sOperation.mProperties.mRemoteNodeReceiveTimeout = 3; // 3s
        sOperation.mProperties.mQueryTimeout             = 3; // 3s
        sOperation.mProperties.mNonQueryTimeout          = 3; // 3s
        sOperation.mProperties.mThreadCount              = 16;
        sOperation.mProperties.mThreadSleepTimeout       = 10;
        sOperation.mProperties.mRemoteNodeSessionCount   = 128;
        sOperation.mProperties.mTraceLogFileSize         = 10 * 1024 * 1024; // 10MB
        sOperation.mProperties.mTraceLogFileCount        = 10;
        sOperation.mProperties.mTraceLoggingLevel        = TraceLogger.LogLevelTrace;
        
        sOperation.mProperties.mTargetList = new LinkedList();
        sOperation.mProperties.mTargetList.add(
                (aTarget != null) ? aTarget : mTarget);
        
        CreateLinkerCtrlSessionTask sTask = new CreateLinkerCtrlSessionTask();

        sTask.setMainMediator(this);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        sTask.run();
    }

    public void runCreateLinkerDataSessionTask(int aLinkerSessionId)
    {
        CreateLinkerDataSessionOp sOperation = new CreateLinkerDataSessionOp();
        
        sOperation.setSessionId(aLinkerSessionId);
        
        CreateLinkerDataSessionTask sTask = new CreateLinkerDataSessionTask();

        sTask.setMainMediator(this);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        sTask.run();
    }

    public void runExecuteRemoteStatement(int    aLinkerDataSessionId,
                                          int    aRemoteStatementId,
                                          String aTargetName,
                                          String aTargetUser,
                                          String aTargetPassword,
                                          String aStatementString)
    {
        ExecuteRemoteStatementOp sOperation = new ExecuteRemoteStatementOp();
        
        sOperation.setSessionId(aLinkerDataSessionId);
        
        sOperation.mTargetName        = aTargetName;
        sOperation.mTargetUser        = aTargetUser;
        sOperation.mTargetPassword    = aTargetPassword;
        sOperation.mRemoteStatementId = aRemoteStatementId;
        sOperation.mStatementType     = StatementType.RemoteExecuteImmediate;
        sOperation.mStatementString   = aStatementString;
        
        ExecuteRemoteStatementTask sTask = new ExecuteRemoteStatementTask();

        sTask.setMainMediator(this);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        sTask.run();
    }
    
    public void runFreeRemoteStatement(int aLinkerDataSessionId,
                                       int aRemoteStatementId)
    {
        RequestFreeRemoteStatementOp sOperation = 
                new RequestFreeRemoteStatementOp();
        
        sOperation.setSessionId(aLinkerDataSessionId);

        sOperation.mRemoteStatementId = aRemoteStatementId;
        
        RequestFreeRemoteStatementTask sTask = 
                new RequestFreeRemoteStatementTask();

        sTask.setMainMediator(this);
        sTask.setSessionManager(mSessionManager);
        sTask.setOperation(sOperation);
        
        sTask.run();
    }
    
    public void onSetProductInfo(String aProductInfo, int aDBCharSet)
    {
        // set database character set of ALTIBASE server
        JdbcDataManager.setDBCharSet(aDBCharSet);
    }

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
                "Loaded properties. ({0})", mPropertyManager.toString());
        
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

    public int onGetProtocolVersion()
    {
        return ProtocolManager.getProtocolVersion();
    }

    public void onCreatedLinkerCtrlSession()
    {
    }

    public void onCreatedLinkerDataSession(int aSessionId)
    {
    }

    public boolean onDestroyLinkerSession(int aSessionId, boolean aForce)
    {
        return true;
    }

    public int onGetLinkerStatus()
    {
        return 0;
    }

    public boolean onPrepareLinkerShutdown()
    {
        return true;
    }

    public void onDoLinkerShutdown()
    {
    }

    public void onDisconnectedLinkerSession(int aSessionId)
    {
    }

    public boolean onSendOperation(Operation aOperation)
    {
        if ((aOperation instanceof ResultOperation) == true)
        {
            mResultOperationList.add(aOperation);
        }
        
        return true;
    }

    public boolean onReceivedOperation(Operation aOperation)
    {
        return true;
    }

    public boolean onGetThreadDumpLog()
    {
        return false;
    }
    
    public void onUnassignToThreadPool( int aSessionId )
    {  
    }
    
    public void onDropTask( int aLinkerSessionID )
    {   
    }
    
    public void onCreatedLinkerNotifySession(int aSessionId)
    {
        
    }
}
