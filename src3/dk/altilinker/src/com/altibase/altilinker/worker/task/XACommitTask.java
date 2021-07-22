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
 
package com.altibase.altilinker.worker.task;

import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;

import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.XACommitOp;
import com.altibase.altilinker.adlp.op.XACommitResultOp;
import com.altibase.altilinker.adlp.type.ResultCode;
import com.altibase.altilinker.adlp.type.Target;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.controller.PropertyManager;
import com.altibase.altilinker.jdbc.JdbcDriverManager;
import com.altibase.altilinker.session.LinkerDataSession;
import com.altibase.altilinker.session.RemoteNodeSession;
import com.altibase.altilinker.session.SessionManager;
import com.altibase.altilinker.session.TaskResult;
import com.altibase.altilinker.worker.Task;

public class XACommitTask extends Task
{
    private static final int mMaxRemoteNodeSessionCount =
        PropertyManager.getInstance().getRemoteNodeSessionCount();
    
    private LinkedHashMap mTargetAndXidMap = null;
    
    public XACommitTask()
    {
        super( TaskType.XACommit );
    }

    public void setOperation( Operation aOperation )
    {
        super.setOperation(aOperation);

        XACommitOp sXACommitOp = (XACommitOp)aOperation;

        mTargetAndXidMap = new LinkedHashMap( sXACommitOp.mXidCount );

        for ( int i = 0 ; i < sXACommitOp.mXidCount ; i++ )
        {
            mTargetAndXidMap.put( sXACommitOp.mTargetName[i],
                                  sXACommitOp.mXid[i].convertToXid() );
        }
    }
    
    public void run()
    {
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;

        // check for connecting linker data session
        LinkerDataSession sLinkerDataSession = 
                sSessionManager.getLinkerDataSession(sLinkerSessionId);

        // allocate new result operation
        XACommitResultOp sResultOperation = (XACommitResultOp)newResultOperation();

        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                    "Task could not allocate new result operation for {0} operation.",
                    new Object[] { new Integer(getRequestOperationId()) } );

            System.exit(-1);
            return;
        }
        
        if (sLinkerDataSession == null)
        {
            mTraceLogger.logError(
                    "Unknown linker data session. (Linker Data Session Id = {0})",
                    new Object[] { new Integer(sLinkerSessionId) } );

            sResultCode = ResultCode.UnknownSession;
        }
        else
        {
            Map sRemoteNodeSessionMap =
                sLinkerDataSession.getRemoteNodeSessionMap();
            
            Iterator sIterator = mTargetAndXidMap.entrySet().iterator();
            
            while ( sIterator.hasNext() == true )
            {
                Map.Entry sEntry = ( Map.Entry )sIterator.next();
                
                String sTargetName = ( String )sEntry.getKey();
                Xid    sXid = ( Xid )sEntry.getValue();
                
                RemoteNodeSession sRemoteNodeSession = findRemoteNodeSession( sRemoteNodeSessionMap, sTargetName );
                
                if ( sRemoteNodeSession == null )
                {
                    sRemoteNodeSession = newRemoteNodeSession( sXid,
                                                               sTargetName, 
                                                               sSessionManager,
                                                               sResultOperation, 
                                                               sLinkerDataSession );

                    if ( sRemoteNodeSession == null )
                    {
                        break;
                    }   
                }
                
                if (sRemoteNodeSession != null)
                {
                    if (sRemoteNodeSession.isExecuting() == true)
                    {
                        mTraceLogger.logWarning(
                                "Remote node session was busy. (Remote Node Session Id = {0})",
                                new Object[] { new Integer(sRemoteNodeSession.getSessionId()) } );
                        sResultOperation.addFailXid( sXid, ResultCode.Busy );
                        continue;
                    }
                    
                    sRemoteNodeSession.executing( RemoteNodeSession.TaskType.XACommit );
                    XAResource sXARes = sRemoteNodeSession.getXAResource();
                    
                    if ( isNotifySession() == true )
                    {
                        Xid[] sXidList;
                        
                        try
                        {
                            sXidList = sXARes.recover( XAResource.TMSTARTRSCAN );
                        }
                        catch( XAException e )
                        {
                            sRemoteNodeSession.executed( new TaskResult(TaskResult.Failed) );
                            
                            mTraceLogger.log( e );
                            sResultOperation.addFailXid( sXid,
                                                         e.errorCode );
                            continue;
                        }
                        
                        checkAndCommit( sXARes,
                                        sXid,
                                        sRemoteNodeSession,
                                        sResultOperation,
                                        sXidList );
                  
                    }
                    else
                    {
                        commit( sXARes,
                                sXid,
                                sRemoteNodeSession,
                                sResultOperation );
                    }
                }
            } // while
            
            if( sResultOperation.mFailXidList.size() > 0 )
            {
                sResultCode = ResultCode.Failed;
            }
            else
            {
                sResultCode = ResultCode.Success;
            }
        }

        // close all of remote node sessions
        sLinkerDataSession.closeRemoteNodeSession();
        
        // set common header of result operation
        sResultOperation.setSessionId( sLinkerSessionId );
        sResultOperation.setResultCode( sResultCode );
        sResultOperation.setSeqNo( this.getSeqNo() );
        sResultOperation.setXA( this.isXA() );

        // nothing payload contents of result operation

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);

        sResultOperation.close();
    }
    
    private void checkAndCommit( XAResource        aXARes,
                                 Xid               aXid,
                                 RemoteNodeSession aRemoteNodeSession,
                                 XACommitResultOp  aResultOperation,
                                 Xid[]             aXidList )
    {
        for ( int i = 0; i < aXidList.length; i++ )
        {
            if ( aXid.equals( aXidList[i] ) )
            {
                commit( aXARes,
                        aXid,
                        aRemoteNodeSession,
                        aResultOperation );
                break;
            }
            else
            {
                /* Nothing to do : it's completed already */
            }
        }    
    }

    
    private RemoteNodeSession newRemoteNodeSession( Xid                aXid,
                                                    String             aTargetName, 
                                                    SessionManager     aSessionManager,
                                                    XACommitResultOp   aResultOperation, 
                                                    LinkerDataSession  aLinkerDataSession )
    {
        byte              sResultCode           = ResultCode.Success;
        short             sRemoteNodeSessionId  = 0;
        RemoteNodeSession sRemoteNodeSession    = null;
        
        
        Target sTarget = JdbcDriverManager.getTarget( aTargetName, null, null );
        
        int sRemoteNodeSessionCount = aSessionManager.getRemoteNodeSessionCount();

        if (sRemoteNodeSessionCount >= mMaxRemoteNodeSessionCount)
        {
            mTraceLogger.logError(
                    "Maximum remote node session excessed. (Maximum {0}).",
                    new Object[] { new Integer(mMaxRemoteNodeSessionCount) } );
            
            aResultOperation.addFailXid( aXid, ResultCode.ExcessRemoteNodeSessionCount );
        }
        else
        {
            // allocate new remote node session
            sRemoteNodeSession =
                    aLinkerDataSession.newRemoteNodeSession(sTarget);
            sRemoteNodeSession.setXATransStarted();
            sRemoteNodeSession.setXid( aXid );
            
            // connect to remote server
            sResultCode =
                    JdbcDriverManager.connectToRemoteServer( aLinkerDataSession,
                                                             sRemoteNodeSession,
                                                             isXA() );

            if (sResultCode != ResultCode.Success)
            {
                sRemoteNodeSessionId = (short)sRemoteNodeSession.getSessionId();
                
                aLinkerDataSession.closeRemoteNodeSession(sRemoteNodeSessionId);
                
                sRemoteNodeSession   = null;
                sRemoteNodeSessionId = 0;
                
                aResultOperation.addFailXid( aXid, sResultCode );
            }
        }
    
        return sRemoteNodeSession;    
    }
    
    private void commit( XAResource        aXARes,
                         Xid               aXid,
                         RemoteNodeSession aRemoteNodeSession,
                         XACommitResultOp  aResultOperation )
    {
        try
        {
            aXARes.commit( aXid,
                           false /* do not use one-phase commit */);
            aRemoteNodeSession.executed();
        }
        catch( XAException e )
        {
            if ( e.errorCode == XAException.XA_HEURCOM )
            {
                try
                {
                    aXARes.forget( aXid );
                    aRemoteNodeSession.executed();
                }
                catch ( XAException forget_e )
                {
                    aRemoteNodeSession.executed( new TaskResult(TaskResult.Failed) );

                    mTraceLogger.log( forget_e  );
                    aResultOperation.addFailXid( aXid,
                                                 forget_e.errorCode );
                }
            }
            else if ( ( e.errorCode == XAException.XA_HEURHAZ ) ||
                      ( e.errorCode == XAException.XA_HEURRB  ) ||
                      ( e.errorCode == XAException.XA_HEURMIX ) )
            {
                try
                {                    
                    mTraceLogger.log( e );
                    aResultOperation.addHeuristicXid( aXid );
                    
                    aXARes.forget( aXid );                
                    aRemoteNodeSession.executed();
                }
                catch ( XAException forget_e )
                {
                    aRemoteNodeSession.executed( new TaskResult(TaskResult.Failed) );
                    
                    mTraceLogger.log( forget_e );
                    aResultOperation.addFailXid( aXid,
                                                 forget_e.errorCode );
                }
            }
            else
            {
                aRemoteNodeSession.executed( new TaskResult(TaskResult.Failed) );

                mTraceLogger.log(e);
                aResultOperation.addFailXid( aXid,
                                             e.errorCode );
            }
        }
    }
}
