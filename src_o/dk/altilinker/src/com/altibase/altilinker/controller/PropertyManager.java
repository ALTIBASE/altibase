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

import com.altibase.altilinker.adlp.type.*;

/**
 * Property manager static class to manage dblink.conf properties
 */
public class PropertyManager
{
    private static PropertyManager mPropertyManager = new PropertyManager();

    private int        mListenPort      = 0;
    private String     mTraceLogFileDir = null;
    private Properties mProperties      = null;
    
    /**
     * Constructor
     */
    private PropertyManager()
    {
        // nothing to do
    }
    
    /**
     * Get static object
     */
    public static PropertyManager getInstance()
    {
        return mPropertyManager;
    }

    /**
     * Close for this static class
     */
    public static void close()
    {
        mPropertyManager = null;
    }
    
    /**
     * Set TCP listen port
     * 
     * @param aListenPort       TCP listen port
     */
    public void setListenPort(int aListenPort)
    {
        mListenPort = aListenPort;
    }
    
    /**
     * Set trace log file directory
     * 
     * @param aTraceLogFileDir  Trace log file directory
     */
    public void setTraceLogFileDir(String aTraceLogFileDir)
    {
        mTraceLogFileDir = aTraceLogFileDir;
    }
    
    /**
     * Set properties
     * 
     * @param aProperties       Properties
     */
    public void setProperties(Properties aProperties)
    {
        mProperties = aProperties;
    }
    
    /**
     * Get TCP listen port
     * 
     * @return  TCP listen port
     */
    public int getListenPort()
    {
        return mListenPort;
    }
    
    /**
     * Get trace log file directory
     * 
     * @return  Trace log file directory
     */
    public String getTraceLogFileDir()
    {
        return mTraceLogFileDir;
    }

    /**
     * Get remote node receive timeout
     * 
     * @return  Remote node receive timeout
     */
    public int getRemoteNodeReceiveTimeout()
    {
        return mProperties.mRemoteNodeReceiveTimeout;
    }
    
    /**
     * Get query timeout
     * 
     * @return  Query timeout
     */
    public int getQueryTimeout()
    {
        return mProperties.mQueryTimeout;
    }
    
    /**
     * Get non-query timeout
     * 
     * @return  Non-query timeout
     */
    public int getNonQueryTimeout()
    {
        return mProperties.mNonQueryTimeout;
    }
    
    /**
     * Get thread count to create on thread pool
     * 
     * @return  Thread count to create on thread pool
     */
    public int getThreadCount()
    {
        return mProperties.mThreadCount;
    }
    
    /**
     * Get thread sleep timeout
     * 
     * @return  Thread sleep timeout
     */
    public int getThreadSleepTimeout()
    {
        return mProperties.mThreadSleepTimeout;
    }
    
    /**
     * Get remote node session count
     * 
     * @return  Remote node session count
     */
    public int getRemoteNodeSessionCount()
    {
        return mProperties.mRemoteNodeSessionCount;
    }
    
    /**
     * Get file size per a trace log file
     * 
     * @return  File size per a trace log file
     */
    public int getTraceLogFileSize()
    {
        return mProperties.mTraceLogFileSize;
    }
    
    /**
     * Get trace log file count
     * 
     * @return  Trace log file count
     */
    public int getTraceLogFileCount()
    {
        return mProperties.mTraceLogFileCount;
    }
    
    /**
     * Get trace logging level
     * 
     * @return  Trace logging level
     */
    public int getTraceLoggingLevel()
    {
        return mProperties.mTraceLoggingLevel;
    }
    
    /**
     * Get target by target name
     * 
     * @param aTargetName       Target name
     * @return                  Target object
     */
    public Target getTarget(String aTargetName)
    {
        if (aTargetName == null)
        {
            return null;
        }
        
        if (mProperties == null)
        {
            return null;
        }
        
        Target sTarget = mProperties.getTarget(aTargetName);
        
        return sTarget;
    }
    
    /**
     * Get target list
     * 
     * @return  Target list
     */
    public LinkedList getTargetList()
    {
        if (mProperties == null)
        {
            return null;
        }
        
        return mProperties.mTargetList;
    }
    
    /**
     * Set target list
     * 
     * @param aTargetList       Target list
     */
    public void setTargetList(LinkedList aTargetList)
    {
        if (mProperties == null)
        {
            return;
        }
        
        mProperties.mTargetList = aTargetList;
    }
    
    /**
     * String representation for this object
     */
    public String toString()
    {
        String sString = "";

        sString += "ALTILINKER_PORT_NO = "       + mListenPort      + ",";
        sString += "ALTILINKER_TRACE_LOG_DIR = " + mTraceLogFileDir + ",";

        sString += mProperties.toString();
        
        return sString;
    }
}
