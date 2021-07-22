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

import com.altibase.altilinker.controller.MainMediator;
import com.altibase.altilinker.session.SessionManager;
import com.altibase.altilinker.worker.Task;

public class DoLinkerShutdownTask extends Task
{
    public DoLinkerShutdownTask()
    {
        super(TaskType.DoLinkerShutdown);
    }
    
    public void run()
    {
        MainMediator   sMainMediator   = getMainMediator();
        SessionManager sSessionManager = getSessionManager();
        
        // check for connecting linker control session
        if (sSessionManager.isCreatedLinkerCtrlSession() == true)
        {
            mTraceLogger.logInfo(
                    "AltiLinker will shutdown by requesting DK module of ALTIBASE server.");
        }
        else
        {
            mTraceLogger.logError("Linker control session was not created.");
        }
        
        sMainMediator.onDoLinkerShutdown();
    }
}
