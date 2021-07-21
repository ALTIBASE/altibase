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

import java.nio.charset.Charset;
import java.util.LinkedList;

import com.altibase.altilinker.adlp.Operation;
import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.jdbc.JdbcDriverManager;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.SessionManager;
import com.altibase.altilinker.worker.Task;

public class CreateLinkerCtrlSessionTask extends Task
{
    protected String     mProductInfo = null;
    protected int        mDBCharSet   = DBCharSet.None;
    protected Properties mProperties  = null;
    
    public CreateLinkerCtrlSessionTask()
    {
        super(TaskType.CreateLinkerCtrlSession);
    }

    public void setOperation(Operation aOperation)
    {
        super.setOperation(aOperation);
        
        CreateLinkerCtrlSessionOp sRequestOperation = (CreateLinkerCtrlSessionOp)aOperation;

        mProductInfo = sRequestOperation.mProductInfo;
        mDBCharSet   = sRequestOperation.mDBCharSet;
        mProperties  = sRequestOperation.mProperties;
    }
    
    public void run()
    {
        MainMediator   sMainMediator      = getMainMediator();
        SessionManager sSessionManager    = getSessionManager();
        LinkedList     sInvalidTargetList = null;
        byte           sResultCode        = ResultCode.Success;
        
        // check if already create linker control session 
        if (sSessionManager.isCreatedLinkerCtrlSession() == false)
        {
            // create linker control session
            if (sSessionManager.createLinkerCtrlSession() == true)
            {
                String sDBCharSetName = DBCharSet.getDBCharSetName(mDBCharSet);

                // check to support character set on java
                Charset sCharset = Charset.forName(sDBCharSetName);
                if (sCharset == null)
                {
                    mTraceLogger.logError(
                            "Not supported database character set. (DB character set name = {0} ({1})",
                            new Object[] {
                                    sDBCharSetName,
                                    new Integer(mDBCharSet) });
                    
                    sResultCode = ResultCode.NotSupportedDBCharSet;
                }
                else
                {
                    // set product info
                    sMainMediator.onSetProductInfo(mProductInfo, mDBCharSet);
    
                    // load properties
                    sMainMediator.onLoadProperties(mProperties);
        
                    // notify linker control session creation to owner
                    sMainMediator.onCreatedLinkerCtrlSession();
                    
                    sInvalidTargetList = JdbcDriverManager.getInvalidTargetList();

                    sResultCode = ResultCode.Success;
                }
            }
            else
            {
                mTraceLogger.logFatal(
                        "Linker control session failed to create.");
                
                System.exit(-1);
                
                return;
            }
        }
        else
        {
            mTraceLogger.logWarning("Linker control session created already.");
            
            sResultCode = ResultCode.AlreadyCreatedSession;
        }

        // allocate new result operation
        CreateLinkerCtrlSessionResultOp sResultOperation =
                (CreateLinkerCtrlSessionResultOp)newResultOperation();
        
        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                    "Task could not allocate new result operation for {0} operation.",
                    new Object[] { new Integer(getRequestOperationId()) } );
            
            System.exit(-1);
            
            return;
        }

        // set common header of result operation
        sResultOperation.setSessionId(getLinkerSessionId());
        sResultOperation.setResultCode(sResultCode);
        sResultOperation.setSeqNo( this.getSeqNo() );
        sResultOperation.setXA( this.isXA() );

        // set payload contents of result operation
        sResultOperation.mProtocolVersion   = sMainMediator.onGetProtocolVersion();
        sResultOperation.mInvalidTargetList = sInvalidTargetList; 

        // send result operation
        if ( sMainMediator.onSendOperation(sResultOperation) == true )
        {
            /* do nothing */
        }
        else
        {   
            sSessionManager.destroyLinkerCtrlSession();
            
            mTraceLogger.logInfo( "Linker Control session destroyed." );
        }
        
        sResultOperation.close();
    }
}
