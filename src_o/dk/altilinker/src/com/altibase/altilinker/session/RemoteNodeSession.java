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
 
package com.altibase.altilinker.session;

import java.sql.Connection;
import java.sql.SQLException;
import java.sql.Savepoint;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;

import javax.sql.XAConnection;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.altibase.altilinker.adlp.type.Target;
import com.altibase.altilinker.util.TraceLogger;

/**
 * Remote node session class to manage java.sql.Connection object
 */
public class RemoteNodeSession implements Session
{
    /**
     * Session state types
     */
    public interface SessionState
    {
        public static final int None          = 0;
        public static final int Connected     = 1;
        public static final int Executing     = 2;
        public static final int Executed      = 3;
        public static final int Disconnecting = 4;
        public static final int Disconnected  = 5;
    }

    /**
     * Task type types
     */
    public interface TaskType
    {
        public static final int None             = 0;
        public static final int Connection       = 1;
        public static final int AutoCommitChange = 2;
        public static final int Commit           = 3;
        public static final int Rollback         = 4;
        public static final int Savepoint        = 5;
        public static final int XACommit         = 6;
        public static final int XARollback       = 7;
        public static final int XAPrepare        = 8;
    }
    
    private static TraceLogger mTraceLogger = TraceLogger.getInstance();
    private static RemoteNodeSessionIdGenerator mRemoteNodeSessionIdGenerator =
                                     RemoteNodeSessionIdGenerator.getInstance();
    
    private int        mSessionId             = 0;
    private int        mSessionState          = SessionState.None;
    private Target     mTarget                = null;
    private int        mTaskType              = TaskType.None;
    private TaskResult mTaskResult            = null;
    private HashMap    mRemoteStatementMap    = new HashMap(); // <Remote Statement Id, RemoteStatement object>
    private long       mLastRemoteStatementId = 0;
    private boolean    mManuallyCommit        = false;
    private LinkedList      mJdbcSavepointList = new LinkedList();

    private Connection      mJdbcConnection    = null;
    private XAConnection    mXAConnection      = null;
    private XAResource      mXAResource        = null;
    private Xid             mXid               = null;
    private boolean         mIsXATransStarted  = false;

    /**
     * Constructor
     * 
     * @param aSessionId        Remote node session ID
     * @param aTarget           Target
     */
    protected RemoteNodeSession(int aSessionId, Target aTarget)
    {
        mSessionId = aSessionId;
        mTarget    = aTarget;
    }
    
    /**
     * Close this instance
     */
    protected void close()
    {
        mSessionState = SessionState.Disconnecting;

        Iterator sIterator = mRemoteStatementMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            RemoteStatement sRemoteStatement = (RemoteStatement)sEntry.getValue();

            sRemoteStatement.close();
        }
        
        mRemoteStatementMap.clear();
        
        if ( mXAResource != null )
        {
            try
            {
                if( mIsXATransStarted == true )
                {
                    mXAResource.end( mXid, XAResource.TMSUCCESS );
                    mIsXATransStarted = false;
                }
                else
                {
                    /*do nothing*/
                }
            }
            catch ( XAException e )
            {
                mTraceLogger.log(e);
            }
            // Does not rollback for notifysession
            if (mJdbcConnection != null)
            {
                try
                {
                    mJdbcConnection.close();
                } catch (SQLException e) {
                    mTraceLogger.log(e);
                }
                
                mJdbcConnection = null;
            }

            
            if( mXAConnection != null )
            {
                try
                {
                    mXAConnection.close();
                }
                catch (SQLException e)
                {
                    mTraceLogger.log(e);
                }

                mXAConnection = null;
            }

            mXAResource = null;
        }
        else
        {
            if (mJdbcConnection != null)
            {
                try
                {
                    // release all of savepoints
                    sIterator = mJdbcSavepointList.iterator();
                    while (sIterator.hasNext() == true)
                    {
                        Savepoint sJdbcSavepoint = (Savepoint)sIterator.next();
                        
                        mJdbcConnection.releaseSavepoint(sJdbcSavepoint);
                    }

                    /*
                     * Remark: JDBC's specification doesn't specify like following.
                     *         "If the close method is called and there is an active transaction, the results are
                     *         implementation-defined"
                     *         XDB's JDBC doesn't rollback. But HDB's JDBC do rollback.
                     */
                    if ( mJdbcConnection.getAutoCommit() == false )
                    {
                        mJdbcConnection.rollback();
                    }
                    else
                    {
                        /* nothing to do */
                    }

                    // close connection
                    mJdbcConnection.close();
                }
                catch (SQLException e)
                {
                    mTraceLogger.log(e);
                }
                
                mJdbcConnection = null;
            }        
        }
        
        mJdbcSavepointList.clear();
        
        mSessionState = SessionState.Disconnected;
        
        mRemoteNodeSessionIdGenerator.release(mSessionId);
    }
    
    /**
     * Get remote node session ID
     * 
     * @return  Remote node session ID
     */
    public int getSessionId()
    {
        return mSessionId;
    }
    
    /**
     * Get remote node session state
     * 
     * @return  Remote node session state
     */
    public int getSessionState()
    {
        return mSessionState;
    }

    /**
     * Whether JDBC interface is executing or not
     * 
     * @return  true, if JDBC interface is executing. false, if not.
     */
    public boolean isExecuting()
    {
        if (mSessionState != SessionState.Executing)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Get JDBC java.sql.Connection object for this remote node session
     * 
     * @return  JDBC java.sql.Connection object
     */
    public Connection getJdbcConnectionObject()
    {
        return mJdbcConnection;
    }
    
    /**
     * Get target for this remote node session
     * 
     * @return  Target object
     */
    public Target getTarget()
    {
        return mTarget;
    }
    
    /**
     * Get task type
     * 
     * @return  Task type
     */
    public int getTask()
    {
        return mTaskType;
    }
    
    /**
     * Get task result
     * 
     * @return  TaskResult object
     */
    public TaskResult getTaskResult()
    {
        return mTaskResult;
    }
    
    /**
     * Get remote server error when JDBC interface cause remote server error.
     * 
     * @return  RemoteServerError object
     */
    public RemoteServerError getRemoteServerError()
    {
        if (mTaskResult == null)
        {
            return null;
        }
        
        return mTaskResult.getRemoteServerError();
    }

    /**
     * Whether JDBC interface cause remote server error or not
     * 
     * @return  true, if JDBC interface cause remote server error. false, if not.
     */
    public boolean isRemoteServerError()
    {
        RemoteServerError sRemoteServerError = getRemoteServerError();

        return (sRemoteServerError != null) ? true : false;
    }
    
    /**
     * Whether JDBC interface cause remote server connection or not.
     * 
     * @return  true, if JDBC interface cause remote server connection. false, if not.
     */
    public boolean isRemoteServerConnectionError()
    {
        RemoteServerError sRemoteServerError = getRemoteServerError();
        if (sRemoteServerError == null)
        {
            return false;
        }
        
        return sRemoteServerError.isConnectionError();
    }
    
    /**
     * Whether this remote node session should commit manually such as auto-commit mode ON or not
     * 
     * @return  true, if this remote node session is manually auto-commit mode. false, if not.
     */
    public boolean isManuallyCommit()
    {
        return mManuallyCommit;
    }
    
    /**
     * Set manually auto-commit mode ON
     * 
     * @param aManuallyCommit   true, if manually auto-commit mode ON is set. false, if not
     */
    public void setManuallyCommit(boolean aManuallyCommit)
    {
        mManuallyCommit = aManuallyCommit;
    }
    
    /**
     * Create new RemoteStatement object by remote statement ID
     * 
     * @param aStatementId      Remote statement ID
     * @param aStatementString  Statement string
     * @param aNodeSessionId    Remote node session ID
     * @return                  New RemoteStatement object
     */
    protected RemoteStatement newRemoteStatement(long   aStatementId,
                                                 String aStatementString,
                                                 int    aNodeSessionId)
    {
        RemoteStatement sRemoteStatement = 
                new RemoteStatement(aStatementId,
                                    aStatementString,
                                    aNodeSessionId);

        mRemoteStatementMap.put(new Long(aStatementId), sRemoteStatement);
        
        return sRemoteStatement;
    }

    /**
     * Get all of remote statements
     * 
     * @return  Remote statement map
     */
    public Map getRemoteStatementMap()
    {
        HashMap sRemoteStatementMap = (HashMap)mRemoteStatementMap.clone();
        
        return sRemoteStatementMap;
    }
    
    /**
     * Get remote statement by remote statement ID
     * 
     * @param aStatementId      Remote statement ID
     * @return                  RemoteStatement object
     */
    public RemoteStatement getRemoteStatement(long aStatementId)
    {
        RemoteStatement sRemoteStatement =
                (RemoteStatement)mRemoteStatementMap.get(
                        new Long(aStatementId));
        
        return sRemoteStatement;
    }
    
    /**
     * Get remote statement count
     * 
     * @return  Remote statement count
     */
    public int getRemoteStatementCount()
    {
        return mRemoteStatementMap.size();
    }
    
    /**
     * Get remote statement ID which JDBC interface executed last.
     * 
     * @return  Last executed remote statement ID
     */
    public long getLastRemoteStatementId()
    {
        return mLastRemoteStatementId;
    }
    
    /**
     * Set remote node session state
     * 
     * @param   Remote node session state
     */
    public void setSessionState(int aSessionState)
    {
        mSessionState = aSessionState;
    }

    /**
     * Set JDBC java.sql.Connection object
     * 
     * @param aJdbcConnection   JDBC java.sql.Connection object
     */
    public void setJdbcConnectionObject(Connection   aJdbcConnection)
    {
        mJdbcConnection = aJdbcConnection;
    }
  
    /**
     * Set remote statement ID which JDBC interface executed last.
     * 
     * @param aLastRemoteStatementId    Last executed remote statement ID
     * @return                          Old last execute remote statement ID
     */
    public long setLastRemoteStatementId(long aLastRemoteStatementId)
    {
        long sOldLastRemoteStatementId = mLastRemoteStatementId;
        
        mLastRemoteStatementId = aLastRemoteStatementId;
        
        return sOldLastRemoteStatementId; 
    }
    
    /**
     * Set task type which JDBC java.sql.Connection interface is executing now.
     *  
     * @param aTaskType         Task type
     */
    public void executing(int aTaskType)
    {
        mTaskType   = aTaskType;
        mTaskResult = null;
        
        mSessionState = SessionState.Executing;
    }
    
    /**
     * Set success task result after JDBC java.sql.Connection interface is executed successfully
     */
    public void executed()
    {
        executed(TaskResult.Success);
    }
    
    /**
     * Set task result after JDBC java.sql.Connection interface is executed
     * 
     * @param aTaskResult       Task result value
     */
    public void executed(int aTaskResult)
    {
        executed(new TaskResult(aTaskResult));
    }

    /**
     * Set java.sql.Exception after JDBC java.sql.Statement interface fail to execute.
     * 
     * @param aRemoteServerError RemoteServerError object
     */
    public void executed(SQLException e)
    {
        executed(new TaskResult(new RemoteServerError(e)));
    }
    
    /**
     * Set remote server error task result after JDBC java.sql.Connection interface fail to execute.
     * 
     * @param aRemoteServerError        RemoteServerError object
     */
    public void executed(RemoteServerError aRemoteServerError)
    {
        executed(new TaskResult(aRemoteServerError));
    }

    /**
     * Set task result object after JDBC java.sql.Connection interface is executed. 
     * 
     * @param aTaskResult       TaskResult object
     */
    public void executed(TaskResult aTaskResult)
    {
        mTaskResult = aTaskResult;
        
        mSessionState = SessionState.Executed;
    }
    
    /**
     * Close a specific remote statement
     * 
     * @param aStatementId      Remote statement ID
     */
    protected void closeRemoteStatement(long aStatementId)
    {
        RemoteStatement sRemoteStatement = 
                (RemoteStatement)mRemoteStatementMap.remove(
                        new Long(aStatementId));
        
        if (sRemoteStatement != null)
        {
            sRemoteStatement.close();
            sRemoteStatement = null;
        }
    }
    
    /**
     * Add java.sql.Savepoint object.
     * During transaction is alive, savepoint list will keep.
     * 
     * @param aJdbcSavepoint    java.sql.Savepoint object
     */
    public void addJdbcSavepointObect(java.sql.Savepoint aJdbcSavepoint)
    {
        if (aJdbcSavepoint == null)
        {
            return;
        }
        
        mJdbcSavepointList.add(aJdbcSavepoint);
    }
    
    /**
     * Get java.sql.savepoint object by savepoint name
     * 
     * @param aSavepointName    Savepoint name
     * @return                  java.sql.Savepoint object
     */
    public java.sql.Savepoint getJdbcSavepointObject(String aSavepointName)
    {
        if (aSavepointName == null)
        {
            return null;
        }
        
        Iterator sIterator = mJdbcSavepointList.iterator();
        while (sIterator.hasNext() == true)
        {
            java.sql.Savepoint sJdbcSavepoint = (java.sql.Savepoint)sIterator.next();
            
            try
            {
                if (sJdbcSavepoint.getSavepointName().equals(aSavepointName) == true)
                {
                    return sJdbcSavepoint;
                }
            }
            catch (SQLException e)
            {
                mTraceLogger.log(e);
            }
        }
        
        return null;
    }
    
    /**
     * Clear all of java.sql.savepoint objects after transaction rollback.
     * 
     * @return  Old savepoint list
     */
    public LinkedList rollbackedJdbcSavepointObject()
    {
        LinkedList sJdbcSavepointList = mJdbcSavepointList;
        
        mJdbcSavepointList.clear();
        
        return sJdbcSavepointList;
    }

    /**
     * Clear java.sql.savepoint objects after transaction rollback to a specific savepoint.
     * 
     * @param aJdbcSavepoint    Savepoint to rollback
     * @return                  Old savepoint list
     */
    public LinkedList rollbackedJdbcSavepointObject(java.sql.Savepoint aJdbcSavepoint)
    {
        LinkedList sJdbcSavepointList = new LinkedList();

        boolean sRemove = false;

        Iterator sIterator = mJdbcSavepointList.iterator();
        while (sIterator.hasNext() == true)
        {
            java.sql.Savepoint sJdbcSavepoint =
                    (java.sql.Savepoint)sIterator.next();

            if (sRemove == true)
            {
                sJdbcSavepointList.add(sJdbcSavepoint);
                sIterator.remove();
            }
            else
            {
                if (sJdbcSavepoint == aJdbcSavepoint)
                {
                    sRemove = true;
                }
            }

        }

        return sJdbcSavepointList;
    }
    
    /**
     * Get java.sql.Savepoint object list
     * 
     * @return  java.sql.Savepoint object list
     */
    public LinkedList getJdbcSavepointObject()
    {
        LinkedList sJdbcSavepointList = new LinkedList();
        
        sJdbcSavepointList.addAll(mJdbcSavepointList);
        
        return sJdbcSavepointList;
    }
    
    public void setXAConnection( XAConnection aXAConnection )
    {
        mXAConnection = aXAConnection;
    }
    
    public XAConnection getXAConnection()
    {
        return mXAConnection;
    }
    
    public void setXAResource( XAResource aXAResource )
    {
        mXAResource = aXAResource;
    }
    
    public XAResource getXAResource()
    {
        return mXAResource;
    }
    
    public void setXid( Xid aXid )
    {
        mXid = aXid;
    }
    
    public Xid getXid()
    {
        return mXid;
    }
    
    public void setXATransStarted()
    {
        mIsXATransStarted = true;
    }
    
    public void setXATransEnded()
    {
        mIsXATransStarted = false;
    }
    
    public boolean isXATransStarted()
    {
        return mIsXATransStarted;
    }
}
