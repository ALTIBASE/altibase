/*
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

package Altibase.jdbc.driver.util;

import java.util.logging.FileHandler;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.io.File;

public final class AltibaseTraceLogger
{
    private static final String LOGGER_NAME   = AltibaseTraceLogger.class.getName();
    private static final String LOGFILE_NAME  = "jdbc.trc";

    private static Logger       mLogger;
    private static boolean      mIsDisabled   = false;
    private static Level        mCurrentLevel = Level.OFF;

    static
    {
        String isDisabledProp = RuntimeEnvironmentVariables.getVariable("ALTIBASE_JDBC_TRCLOG_DISABLE", "FALSE");
        mIsDisabled = Boolean.valueOf(isDisabledProp).booleanValue();
        
        if(!RuntimeEnvironmentVariables.isSet(AltibaseProperties.PROP_ALT_SERVERS))
        {
            mIsDisabled = true;
        }
        
        if(!mIsDisabled)
        {
            mLogger = Logger.getLogger(LOGGER_NAME);
            
            String sAltibaseHomePath = AltibaseEnvironmentVariables.getAltibaseHome();
            String sLogFilePath;

            if (sAltibaseHomePath != null)
            {
                //BUG-38934 JDBC use invalid file.separator
                if( sAltibaseHomePath.endsWith( File.separator ) ) 
                {
                    sLogFilePath = sAltibaseHomePath + "trc" + File.separator + LOGFILE_NAME;
                }
                else
                {
                    sLogFilePath = sAltibaseHomePath + File.separator + "trc" + File.separator + LOGFILE_NAME;
                }
            }
            else
            {
                sLogFilePath = LOGFILE_NAME;
            }
    
            try
            {
                Handler sHandler = new FileHandler(sLogFilePath);
                mLogger.addHandler(sHandler);
            }
            catch (Exception ex)
            {
                mIsDisabled = true;
            }
    
            // proj-1538 ipv6: add log level property for jdbc logging
            // set log level to the value which a user inputs. default: off
            String sLogLevelStr = RuntimeEnvironmentVariables.getVariable("ALTIBASE_JDBC_LOGGING_LEVEL", "OFF").toUpperCase();
    
            // if unknown value, set level to off by default.
            // we support only: off, all, info, warning, severe
            // level order: all < info < warning < severe < off
            // for more details, see java.util.logging.Level in java docs
            try
            {
                mCurrentLevel = Level.parse(sLogLevelStr);
            } 
            catch (IllegalArgumentException e)
            {
                // IGNORE THIS EXCEPTION
                // USING DEFAULT LEVEL
            }
    
            try
            {
                mLogger.setLevel(mCurrentLevel);
            } 
            catch (SecurityException e)
            {
                mIsDisabled = true;
            }
    
            // decide whether or not this logger prints its output to stderr.
            // default: false (not printed to stderr)
            String sLogStderr = RuntimeEnvironmentVariables.getVariable("ALTIBASE_JDBC_TRCLOG_PRINT_STDERR", "FALSE").toUpperCase();
            if (sLogStderr.equals("FALSE"))
            {
                mLogger.setUseParentHandlers(false);
            }
        }
    }

    public static void log(Level aLevel, String aLog, Exception aException)
    {
        if (!mIsDisabled)
        {
            mLogger.log(aLevel, aLog, aException);
        }
    }
    
    public static void log(Level aLevel, String aLog)
    {
        if (!mIsDisabled)
        {
            mLogger.log(aLevel, aLog);
        }
    }

    private AltibaseTraceLogger()
    {
    }
}
