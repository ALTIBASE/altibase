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

import java.util.LinkedHashMap;
import java.util.Iterator;
import java.util.Map;

import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.XAPrepareOp;
import com.altibase.altilinker.adlp.op.XAPrepareResultOp;
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

public class XAPrepareTask extends Task
{    
    private static final int mMaxRemoteNodeSessionCount =
        PropertyManager.getInstance().getRemoteNodeSessionCount();
    
    private LinkedHashMap mTargetAndXidMap = null;
    
    public XAPrepareTask()
    {   
        super( TaskType.XAPrepare );
    }
    
    public void setOperation( Operation aOperation )
    {
        super.setOperation(aOperation);

        XAPrepareOp sXAPrepareOp = (XAPrepareOp)aOperation;

        mTargetAndXidMap = new LinkedHashMap( sXAPrepareOp.mXidCount );

        for ( int i = 0 ; i < sXAPrepareOp.mXidCount ; i++ )
        {
            mTargetAndXidMap.put( sXAPrepareOp.mTargetName[i],
                    sXAPrepareOp.mXid[i].convertToXid() );
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
        XAPrepareResultOp sResultOperation = (XAPrepareResultOp)newResultOperation();
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
                
                if ( sRemoteNodeSession != null )
                {
                    if (sRemoteNodeSession.isExecuting() == true)
                    {
                        mTraceLogger.logWarning(
                                "Remote node session was busy. (Remote Node Session Id = {0})",
                                new Object[] { new Integer(sRemoteNodeSession.getSessionId()) } );
                        sResultOperation.addFailXid( sXid, ResultCode.Busy );
                        break;
                    }
                    
                    sRemoteNodeSession.executing( RemoteNodeSession.TaskType.XAPrepare );
                    
                    XAResource sXARes = sRemoteNodeSession.getXAResource();
                    
                    if ( sRemoteNodeSession.isXATransStarted() == true )
                    {
                        try
                        {
                            sXARes.end( sXid,
                                        XAResource.TMSUCCESS );
                            sRemoteNodeSession.setXATransEnded();
                        }
                        catch ( XAException e )
                        {
                            sRemoteNodeSession.executed( new TaskResult(TaskResult.Failed) );
    
                            mTraceLogger.log( e );
                            sResultOperation.addFailXid( sXid,
                                                         e.errorCode );
                            break;
                        }
                    }
                    
                    int sResult = prepare( sXARes,
                                           sXid,
                                           sRemoteNodeSession,
                                           sResultOperation );

                    if ( sResult == ResultCode.Failed )
                    {
                        break;
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
        
        // set common header of result operation
        sResultOperation.setSessionId(sLinkerSessionId);
        sResultOperation.setResultCode(sResultCode);
        sResultOperation.setSeqNo( this.getSeqNo() );
        sResultOperation.setXA( this.isXA() );

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);

        sResultOperation.close();
    }
    
    
    private RemoteNodeSession newRemoteNodeSession( Xid                aXid,
                                                    String             aTargetName, 
                                                    SessionManager     aSessionManager,
                                                    XAPrepareResultOp  aResultOperation, 
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
    
    private int prepare( XAResource        aXARes,
                         Xid               aXid,
                         RemoteNodeSession aRemoteNodeSession,
                         XAPrepareResultOp aResultOperation )
    {
        int sResult;
        
        try
        {
            sResult = aXARes.prepare( aXid );

            if ( sResult == XAResource.XA_OK )
            {
                aRemoteNodeSession.executed();
            }
            else if ( sResult == XAResource.XA_RDONLY )
            {
                aResultOperation.addRDOnlyXid( aXid );
                aRemoteNodeSession.executed();
            }
            else
            {
                aRemoteNodeSession.executed( new TaskResult(TaskResult.Failed) );
                
                mTraceLogger.logError(
                        "Prepare transaction failed (Result = {0}).",
                        new Object[] { new Integer( sResult ) } );
                aResultOperation.addFailXid( aXid, sResult);
                sResult = ResultCode.Failed;
            }
        }
        catch ( XAException e )
        {
            if ( e.errorCode == XAResource.XA_RDONLY )
            {
                aResultOperation.addRDOnlyXid( aXid );
                sResult = XAResource.XA_RDONLY;
                aRemoteNodeSession.executed();
            }
            else
            {                        
                aRemoteNodeSession.executed( new TaskResult(TaskResult.Failed) );
                
                mTraceLogger.log(e);
                aResultOperation.addFailXid( aXid,
                                             e.errorCode );
                sResult = ResultCode.Failed;
            }
        }
        
        return sResult;
    }
}
