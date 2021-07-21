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
 
package com.altibase.altilinker.util;

import java.io.File;
import java.io.IOException;
import java.text.MessageFormat;
import java.util.logging.*;

/**
 * Trace logger class fro trace logging
 */
public class TraceLogger
{
    /**
     * Trace log level
     */
    private static class LogLevel extends java.util.logging.Level
    {
        // for serialization and deserialization
        // If this variable do not declare, deserialization can InvalidClassExceptions exception cause by default Java(TM) Object Serialization Specification.
        private static final long serialVersionUID = 7822026494106055346L;
        
        public static final java.util.logging.Level Off     = new LogLevel("",        java.util.logging.Level.OFF.intValue());
        public static final java.util.logging.Level Fatal   = new LogLevel("Fatal",   5);
        public static final java.util.logging.Level Error   = new LogLevel("Error",   4);
        public static final java.util.logging.Level Warning = new LogLevel("Warning", 3);
        public static final java.util.logging.Level Info    = new LogLevel("Info",    2);
        public static final java.util.logging.Level Debug   = new LogLevel("Debug",   1);
        public static final java.util.logging.Level Trace   = new LogLevel("Trace",   0);
        
        /**
         * Constructor
         * 
         * @param aName         Log level name
         * @param aValue        Log level value
         */
        public LogLevel(String aName, int aValue)
        {
            super(aName, aValue);
        }

        /**
         * Get java.util.logging.Level object for log level value
         * 
         * @param aLevel        Log level value
         * @return              java.util.logging.Level object
         */
        public static java.util.logging.Level getLevel(int aLevel)
        {
            java.util.logging.Level sLevel = null;
            
            switch (aLevel)
            {
            case LogLevelOff:     sLevel = Off;     break; 
            case LogLevelFatal:   sLevel = Fatal;   break; 
            case LogLevelError:   sLevel = Error;   break; 
            case LogLevelWarning: sLevel = Warning; break; 
            case LogLevelInfo:    sLevel = Info;    break; 
            case LogLevelDebug:   sLevel = Debug;   break; 
            case LogLevelTrace:   sLevel = Trace;   break; 
            default:              sLevel = Off;     break; 
            }
            
            return sLevel;
        }
    }
    
    public static final int     LogLevelOff     = 0;
    public static final int     LogLevelFatal   = 1;
    public static final int     LogLevelError   = 2;
    public static final int     LogLevelWarning = 3;
    public static final int     LogLevelInfo    = 4;
    public static final int     LogLevelDebug   = 5;
    public static final int     LogLevelTrace   = 6;
    
    private String              mFileName       = null;
    private String              mDumpFileName   = null;
    
    static private TraceLogger  mTraceLogger    = new TraceLogger();

    private Logger              mLogger         = java.util.logging.Logger.getLogger("TraceLogger");
    private FileHandler         mLogFileHandler = null;
    private boolean             mStarted        = false;
    private int                 mLevel          = LogLevelInfo;
    private String              mDirPath        = null;
    private long                mFileSize       = 10 * 1024 * 1024; // default 10 MB
    private int                 mFileCount      = 10;               // default 10 files
    
    /**
     * Constructor
     */
    private TraceLogger()
    {
        setDirPath(mDirPath);
        setLevel(mLevel);
    }

    /**
     * Get TraceLogger static object
     * 
     * @return  TraceLogger static object
     */
    static public TraceLogger getInstance()
    {
        return mTraceLogger;
    }
    
    /**
     * Close this object
     */
    public void close()
    {
        stop();
    }
    
    /**
     * Get trace logging directory
     * 
     * @return  Trace logging directory
     */
    public String getDirPath()
    {
        return mDirPath;
    }
    
    /**
     * Set trace logging directory
     * 
     * @param aDirPath  Trace logging directory
     */
    public void setDirPath(String aDirPath)
    {
        mDirPath = aDirPath;
    }

    /**
     * Set trace log file name
     * 
     * @param aFileName  Trace log file name
     */
    public void setFileName(String aFileName)
    {
        mFileName = aFileName;
        mDumpFileName = aFileName.replaceAll("\\.log$", "_dump.log");
    }

    /**
     * Get dump file name
     * 
     * @return dump file name
     */
    public String getDumpFileName()
    {
        return mDumpFileName;
    }
    
    /**
     * Set file limit by file size and file count
     * 
     * @param aFileSize         File size to limit
     * @param aFileCount        File count to limit
     */
    public void setFileLimit(long aFileSize, int aFileCount)
    {
        mFileSize  = aFileSize;
        mFileCount = aFileCount;
    }
    
    /**
     * Set trace logging level
     * 
     * @param aLevel    Trace logging level
     */
    public void setLevel(int aLevel)
    {
        java.util.logging.Level sLevel = LogLevel.getLevel(aLevel);
        
        if (sLevel != null)
        {
            mLevel = aLevel;
            
            mLogger.setLevel(sLevel);
        }
        else
        {
            // nothing to do
        }
    }
    
    /**
     * Start trace logging
     * 
     * @return  true, if success. false, if fail.
     */
    public boolean start()
    {
        if (mStarted == true)
        {
            return true;
        }
        
        if ((mDirPath == null)              ||
            (mFileSize < 0)                 ||
            (mFileSize > Integer.MAX_VALUE) ||
            (mFileCount < 0))
        {
            return false;
        }

        // log file path pattern
        //   %g - the generation number to distinguish rotated logs
        String sFilePathPattern = mDirPath + File.separator + mFileName + "-%g";

        try
        {
            // set log file handler
            if (mLogFileHandler != null)
            {
                mLogFileHandler.close();
                mLogFileHandler = null;
            }

            mLogFileHandler = new FileHandler(sFilePathPattern,
                                              (int)mFileSize,
                                              mFileCount,
                                              true);            // append
        }
        catch (SecurityException e)
        {
            return false;
        }
        catch (IOException e)
        {
            return false;
        }
        
        if (mLogFileHandler == null)
        {
            return false;
        }
        
        java.util.logging.Logger.getLogger("").addHandler(mLogFileHandler);

        mStarted = true;
        
        return true;
    }
    
    /**
     * Whether trace logging start or stop.
     * 
     * @return true, if trace logging start. false, if stop.
     */
    public boolean isStarted()
    {
        return mStarted;
    }
    
    /**
     * Stop trace logging
     */
    public void stop()
    {
        if (mStarted == false)
        {
            return;
        }
        
        logInfo("Trace logging stopped.");

        // close all of handlers of logger
        Handler sHandlers[] = java.util.logging.Logger.getLogger("").getHandlers();

        int i;
        for (i = 0; i < sHandlers.length; ++i)
        {
            sHandlers[i].flush();
            sHandlers[i].close();
        }

        mLogFileHandler = null;

        mStarted = false;
    }
    
    /**
     * Check whether aLevel is loggable level or not.
     * 
     * @param aLevel    Log level to check loggable
     * @return          true, if loggable. false, if not.
     */
    public boolean isLoggable(int aLevel)
    {
        if (mLevel < aLevel)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Write fatal level log message
     * 
     * @param aMessage  Message to write
     */
    public void logFatal(String aMessage)
    {
        log(LogLevelFatal, aMessage);
    }
    
    /**
     * Write fatal level log message
     * 
     * @param aFormat   Message format to write
     * @param aArg      Message format argument
     */
    public void logFatal(String aFormat, Object aArg)
    {
        log(LogLevelFatal, aFormat, aArg);
    }

    /**
     * Write fatal level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logFatal(String aFormat, Object[] aArgs)
    {
        log(LogLevelFatal, aFormat, aArgs);
    }

    /**
     * Write error level log message
     * 
     * @param aMessage  Message to write
     */
    public void logError(String aMessage)
    {
        log(LogLevelError, aMessage);
    }
    
    /**
     * Write error level log message
     * 
     * @param aFormat   Message format to write
     * @param aArg      Message format argument
     */
    public void logError(String aFormat, Object aArg)
    {
        log(LogLevelError, aFormat, aArg);
    }

    /**
     * Write error level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logError(String aFormat, Object[] aArgs)
    {
        log(LogLevelError, aFormat, aArgs);
    }

    /**
     * Write warning level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logWarning(String aMessage)
    {
        log(LogLevelWarning, aMessage);
    }
    
    /**
     * Write warning level log message
     * 
     * @param aFormat   Message format to write
     * @param aArg      Message format argument
     */
    public void logWarning(String aFormat, Object aArg)
    {
        log(LogLevelWarning, aFormat, aArg);
    }
    
    /**
     * Write warning level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logWarning(String aFormat, Object[] aArgs)
    {
        log(LogLevelWarning, aFormat, aArgs);
    }
    
    /**
     * Write info level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logInfo(String aMessage)
    {
        log(LogLevelInfo, aMessage);
    }
    
    /**
     * Write info level log message
     * 
     * @param aFormat   Message format to write
     * @param aArg      Message format argument
     */
    public void logInfo(String aFormat, Object aArg)
    {
        log(LogLevelInfo, aFormat, aArg);
    }

    /**
     * Write info level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logInfo(String aFormat, Object[] aArgs)
    {
        log(LogLevelInfo, aFormat, aArgs);
    }
    
    /**
     * Write debug level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logDebug(String aMessage)
    {
        log(LogLevelDebug, aMessage);
    }

    /**
     * Write debug level log message
     * 
     * @param aFormat   Message format to write
     * @param aArg      Message format argument
     */
    public void logDebug(String aFormat, Object aArg)
    {
        log(LogLevelDebug, aFormat, aArg);
    }
    
    /**
     * Write debug level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logDebug(String aFormat, Object[] aArgs)
    {
        log(LogLevelDebug, aFormat, aArgs);
    }
    
    /**
     * Write trace level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logTrace(String aMessage)
    {
        log(LogLevelTrace, aMessage);
    }
    
    /**
     * Write trace level log message
     * 
     * @param aFormat   Message format to write
     * @param aArg      Message format argument
     */
    public void logTrace(String aFormat, Object aArg)
    {
        log(LogLevelTrace, aFormat, aArg);
    }

    /**
     * Write trace level log message
     * 
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void logTrace(String aFormat, Object[] aArgs)
    {
        log(LogLevelTrace, aFormat, aArgs);
    }
    
    /**
     * Write log message
     * 
     * @param aLevel    Log level
     * @param aMessage  Log message
     */
    public void log(int aLevel, String aMessage)
    {
        if (isStarted() == false)
        {
            return;
        }
        
        if (isLoggable(aLevel) == false || (aMessage == null))
        {
            return;
        }

        // get class name and method name of component caller 
        int    i                 = 0;
        String sSourceClassName  = null;
        String sSourceMethodName = null;
        
        StackTraceElement[] sCallStack = (new Throwable()).getStackTrace();

        while (i < sCallStack.length)
        {
            StackTraceElement sFrame = sCallStack[i];
            
            String sClassName = sFrame.getClassName();
            if (sClassName.equals("com.altibase.altilinker.util.TraceLogger") == true)
            {
                break;
            }
            
            ++i;
        }

        while (i < sCallStack.length)
        {
            StackTraceElement sFrame = sCallStack[i];
            
            String sClassName = sFrame.getClassName();
            if (sClassName.equals("com.altibase.altilinker.util.TraceLogger") == false)
            {
                sSourceClassName = sClassName;
                sSourceMethodName = sFrame.getMethodName();
                break;
            }
            
            ++i;
        }

        if (sSourceClassName == null)
        {
            sSourceClassName = "";
        }

        if (sSourceMethodName == null)
        {
            sSourceMethodName = "";
        }
        else if (sSourceMethodName.equals("<init>") == true)
        {
            sSourceMethodName = "constructor";
        }

        // get log level object from integer log level value 
        java.util.logging.Level sLevel = LogLevel.getLevel(aLevel);

        // trace logging
        mLogger.logp(sLevel, sSourceClassName, sSourceMethodName, aMessage);
    }

    /**
     * Write log message
     * 
     * @param aLevel    Log level
     * @param aFormat   Message format to write
     * @param aArg      Message format argument
     */
    public void log(int aLevel, String aFormat, Object aArg)
    {
        if (isStarted() == false)
        {
            return;
        }
        
        if (isLoggable(aLevel) || (aFormat == null) || (aArg == null))
        {
            return;
        }
        
        log(aLevel, MessageFormat.format(aFormat, new Object[] { aArg }));
    }
    
    /**
     * Write log message
     * 
     * @param aLevel    Log level
     * @param aFormat   Message format to write
     * @param aArgs     Message format arguments
     */
    public void log(int aLevel, String aFormat, Object[] aArgs)
    {
        if (isStarted() == false)
        {
            return;
        }
        
        if ((isLoggable(aLevel) == false) || (aFormat == null) || (aArgs == null))
        {
            return;
        }

        log(aLevel, MessageFormat.format(aFormat, aArgs));
    }

    /**
     * Write java.sql.Exception log message as error log level
     * 
     * @param sException        java.sql.Exception object
     */
    public void log(Exception sException)
    {
        if (sException == null)
        {
            return;
        }
        
        String sMessage = sException.toString();
        
        log(LogLevelError, sMessage);
    }

    /**
     * Write java.lang.Error log message as error log level
     * 
     * @param sException        java.sql.Exception object
     */
    public void log(Error sError)
    {
        if (sError == null)
        {
            return;
        }
        
        String sMessage = sError.toString();
        
        log(LogLevelError, sMessage);
    }
}
