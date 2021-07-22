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
 
package com.altibase.altilinker.adlp.type;

import java.util.Iterator;
import java.util.LinkedList;

import com.altibase.altilinker.util.TraceLogger;

/**
 * dblink.conf properties class
 */
public class Properties
{
    public int        mRemoteNodeReceiveTimeout = 0;
    public int        mQueryTimeout             = 0;
    public int        mNonQueryTimeout          = 0;
    public int        mThreadCount              = 16;
    public int        mThreadSleepTimeout       = 200;
    public int        mRemoteNodeSessionCount   = 128;
    public int        mTraceLogFileSize         = 10 * 1024 * 1024;
    public int        mTraceLogFileCount        = 10;
    public int        mTraceLoggingLevel        = TraceLogger.LogLevelInfo;
    public LinkedList mTargetList               = null;
    
    /**
     * Validate properties
     * 
     * @return true, if valid. false, if invalid
     */
    public boolean validate()
    {
        if (mThreadCount <= 0)
        {
            return false;
        }
        
        if (mTraceLogFileSize <= 0)
        {
            return false;
        }
        
        if (mTraceLogFileCount <= 0)
        {
            return false;
        }
        
        switch (mTraceLoggingLevel)
        {
        case TraceLogger.LogLevelOff:
        case TraceLogger.LogLevelFatal:
        case TraceLogger.LogLevelError:
        case TraceLogger.LogLevelWarning:
        case TraceLogger.LogLevelInfo:
        case TraceLogger.LogLevelDebug:
        case TraceLogger.LogLevelTrace:
            break;
            
        default:
            return false;
        }
        
        if (mTargetList == null)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Get target by target name
     * 
     * @param aTargetName Target name
     * @return Target object
     */
    public Target getTarget(String aTargetName)
    {
        if (aTargetName == null)
            return null;
        
        Iterator sIterator = mTargetList.iterator();
        while (sIterator.hasNext() == true)
        {
            Target sTarget = (Target)sIterator.next();
            
            if (sTarget.mName.equals(aTargetName) == true)
                return sTarget;
        }

        return null;
    }
    
    /**
     * Get string representation for this object
     */
    public String toString()
    {
        String sString = "";
        
        sString += "ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT = " + mRemoteNodeReceiveTimeout + ",";
        sString += "ALTILINKER_QUERY_TIMEOUT = "               + mQueryTimeout             + ",";
        sString += "ALTILINKER_NON_QUERY_TIMEOUT = "           + mNonQueryTimeout          + ",";
        sString += "ALTILINKER_THREAD_COUNT = "                + mThreadCount              + ",";
        sString += "ALTILINKER_THREAD_SLEEP_TIME = "           + mThreadSleepTimeout       + ",";
        sString += "ALTILINKER_TRACE_LOG_FILE_SIZE = "         + mTraceLogFileSize         + ",";
        sString += "ALTILINKER_TRACE_LOG_FILE_COUNT = "        + mTraceLogFileCount        + ",";
        sString += "ALTILINKER_TRACE_LOGGING_LEVEL = "         + mTraceLoggingLevel        + ",";

        sString += "TARGETS = ( ";
        
        Iterator sIterator = mTargetList.iterator();
        while (sIterator.hasNext() == true)
        {
            Target sTarget = (Target)sIterator.next();

            sString += "( " + sTarget.toString() + " )";
            
            if (sIterator.hasNext() == false)
            {
                sString += ", ";
            }
        }
        
        sString += " )";
        
        return sString;
    }
}
