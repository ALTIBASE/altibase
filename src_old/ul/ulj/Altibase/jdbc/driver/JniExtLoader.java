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

package Altibase.jdbc.driver;

import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;

public class JniExtLoader
{
    /* JNI module for ALTIBASE JDBC extension */
    static private final String JNI_EXT_MODULE = "altijext";
    private static transient Logger mLogger;
    private static boolean mIsLinux;

    static
    {
        initialize();
    }

    /**
     * JNI ext. module 을 load 시킨다.
     */
    private static void initialize()
    {
        // check OS
        String sOSName = System.getProperty("os.name");
        if (!"Linux".equalsIgnoreCase(sOSName))
        {
            return;
        }

        // check to load JNI ext. module
        try
        {
            System.loadLibrary(JNI_EXT_MODULE);
            mIsLinux = true;
        }
        catch (Throwable e)
        {
            // ignore exceptions including SecurityException, NullPointerException
            // ignore runtime error including UnsatisfiedLinkError (if the library does not exist)

            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                if (mLogger != null)
                {
                    mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
                }

                mLogger.log(Level.WARNING, "Cannot load JNI ext. module", e);
            }
        }
    }

    public static boolean checkLoading()
    {
        return mIsLinux;
    }
}
