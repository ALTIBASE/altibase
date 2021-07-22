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

import com.altibase.altilinker.adlp.op.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.SessionManager;
import com.altibase.altilinker.worker.Task;

public class CreateLinkerDataSessionTask extends Task
{
    public CreateLinkerDataSessionTask()
    {
        super(TaskType.CreateLinkerDataSession);
    }

    public void run()
    {
        MainMediator   sMainMediator    = getMainMediator();
        SessionManager sSessionManager  = getSessionManager();
        int            sLinkerSessionId = getLinkerSessionId();
        byte           sResultCode      = ResultCode.Success;
        
        // check if already create linker data session 
        if (sSessionManager.isCreatedLinkerDataSession(sLinkerSessionId) == false)
        {
            /* do nothing */   
        }
        else
        {
            mTraceLogger.logWarning(
                    "Linker data session created already. (Linker Data Session Id = {0})",
                    new Object[] { new Integer(sLinkerSessionId) } );
            
            /*
             * BUG-39001
             * Linkser Session is already created.
             * So Old linker Session should be destroyed.
             */
            sMainMediator.onDropTask( sLinkerSessionId );
            sSessionManager.destroyLinkerDataSession( sLinkerSessionId );
            sMainMediator.onUnassignToThreadPool( sLinkerSessionId );
        }
        
        // create linker data session
        if ( sSessionManager.createLinkerDataSession( sLinkerSessionId ) == true )
        {
            // notify linker data session creation to owner
            sMainMediator.onCreatedLinkerDataSession( sLinkerSessionId );
            
            sResultCode = ResultCode.Success;
        }
        else
        {
            mTraceLogger.logFatal(
                    "Failed to create the linker data session. (Linker Data Session Id = {0}",
                    new Object[] { new Integer(sLinkerSessionId) } );
            
            System.exit(-1);
            
            return;
        }

        // allocate new result operation
        CreateLinkerDataSessionResultOp sResultOperation =
                (CreateLinkerDataSessionResultOp)newResultOperation();
        
        if (sResultOperation == null)
        {
            mTraceLogger.logFatal(
                    "Task could not allocate new result operation for {0} operation.",
                    new Object[] { new Integer(getRequestOperationId()) } );
            
            System.exit(-1);
            
            return;
        }

        // set common header of result operation
        sResultOperation.setSessionId(sLinkerSessionId);
        sResultOperation.setResultCode(sResultCode);
        sResultOperation.setSeqNo( this.getSeqNo() );
        sResultOperation.setXA( this.isXA() );

        // nothing payload contents of result operation

        // send result operation
        sMainMediator.onSendOperation(sResultOperation);

        sResultOperation.close();
    }
}
