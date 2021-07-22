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
 
package com.altibase.altilinker;

import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;
import java.io.File;
import java.nio.charset.Charset;

import com.altibase.altilinker.controller.MainController;
import com.altibase.altilinker.util.*;
import com.altibase.altilinker.Version;

/**
 * Main application class
 */
public class MainApp
{
    private static final String   mListenPortOption     = "-listen_port";
    private static final String   mTrcDirOption         = "-trc_dir";
    private static final String   mTrcFileNameOption    = "-trc_file_name";
    private static final String   mVersionOption        = "-v";

    private static final int      REQUIRED_JAVA_MAJOR_VERSION  = 1;
    private static final int      REQUIRED_JAVA_MINOR_VERSION  = 5;
    
    private static TraceLogger    mTraceLogger          = TraceLogger.getInstance();
    
    private static int            mListenPort           = -1;
    private static String         mTraceDir             = "";
    private static String         mTraceFileName        = "";

    private static String         mJavaVersion          = null;
    private static String         mJavaVendor           = null;
    private static String         mJavaSelectorProvider = null;
    private static String         mCPUArchitecture      = null;
    private static String         mOSName               = null;
    private static String         mOSVersion            = null;
    private static String         mDefaultCharset       = null;

    private static MainController mMainController       = null;

    /**
     * main method of AltiLinker application
     * 
     * @param aArgs     Command line arguments
     */
    public static void main(String[] aArgs)
    {
        // set JAVA system properties
        setJavaSystemProperties();
        
        // get system environments
        getSystemEnvironments();
        
        // parse command line arguments
        parseCmdLineArgs(aArgs);

        // validate command line arguments
        if (validateCmdLineArgs() == false)
        {
            printUsage();
            return;
        }

        // initialize trace logger
        mTraceLogger.setDirPath(mTraceDir);
        mTraceLogger.setFileName(mTraceFileName);
        mTraceLogger.setLevel(TraceLogger.LogLevelTrace);

        // start trace logging
        if (mTraceLogger.start() == false)
        {
            return;
        }

        // trace logging for environment
        logSystemEnvironments();
        
        // trace logging for command line arguments
        logCmdLineArgs(aArgs);

        if ( validateJavaVersion() == false )
        {
            mTraceLogger.logFatal(
                    "Altilinker needs JAVA VERSION {0}.{1}.XX",
                    new Object[] { new Integer( REQUIRED_JAVA_MAJOR_VERSION ),
                                   new Integer( REQUIRED_JAVA_MINOR_VERSION ) } );

            return;
        }
        else
        {
            /* do nothing */
        }
        
        // initialize Main Controller component
        mMainController = new MainController();
        
        if (mMainController.start() == false)
        {
            mTraceLogger.logFatal("MainController component could not start.");

            mMainController.close();
            mMainController = null;

            mTraceLogger.close();
            mTraceLogger = null;
            
            System.exit(-1);
            
            return;
        }
        
        if (mMainController.startListen(mListenPort) == false)
        {
            mTraceLogger.logFatal(
                    "AltiLinker could not listen as {0} port number.",
                    new Object[] { new Integer(mListenPort) } );

            mMainController.stop();
            mMainController.close();
            mMainController = null;

            mTraceLogger.close();
            mTraceLogger = null;
            
            System.exit(-1);
            
            return;
        }
    }
    
    /**
     * Shutdown ALTILINKER
     */
    public synchronized static void shutdown()
    {
        try
        {
            if ( mMainController != null )
            {
                mMainController.stop();
                mMainController.close();
                mMainController = null;
            }
        }
        catch ( Exception e )
        {
            mTraceLogger.log( e );
            System.exit( -1 );
        }

        if ( mTraceLogger != null )
        {
            mTraceLogger.stop();
            mTraceLogger.close();
            mTraceLogger = null;
        }
    }
    
    /**
     * Set JAVA system properties
     */
    private static void setJavaSystemProperties()
    {
        // 'IOException: Invalid argument' exception of Selector.select method
        // call on Solaris 10 (on sparcv9 and x64)
        // (reference: http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6782668)

        String sOSName    = System.getProperty("os.name");
        String sOSVersion = System.getProperty("os.version");

        final String sPollClassName = "sun.nio.ch.PollSelectorProvider";

        if ("SunOS".equals(sOSName) == true)
        {
            if (canCreateClassName(sPollClassName) == true)
            {
                System.setProperty("java.nio.channels.spi.SelectorProvider", sPollClassName);
            }
        }
        else if ("Linux".equals(sOSName) == true)
        {
            if (canCreateClassName(sPollClassName) == true)
            {
                System.setProperty("java.nio.channels.spi.SelectorProvider", sPollClassName);
            }
        }
    }
    
    /**
     * Whether create or fail the class name
     * 
     * @param   aClassName  Class name to create
     * @return  true, if create. false, if fail.
     */
    private static boolean canCreateClassName(String aClassName)
    {
        Class sClass = null;

        try
        {
            sClass = Class.forName(aClassName);
        }
        catch (Exception e)
        {
            e.toString();
        }
        
        return (sClass != null) ? true : false;
    }
    
    /**
     * Get system environments
     */
    private static void getSystemEnvironments()
    {
        mJavaVersion          = System.getProperty("java.version");
        mJavaVendor           = System.getProperty("java.vendor");
        mJavaSelectorProvider = System.getProperty("java.nio.channels.spi.SelectorProvider");
        mOSName               = System.getProperty("os.name");
        mCPUArchitecture      = System.getProperty("os.arch");
        mOSVersion            = System.getProperty("os.version");
        mDefaultCharset       = getDefaultCharset().toString();
    }

    /**
     * Get default character set
     * 
     * @return  Charset object
     */
    private static Charset getDefaultCharset()
    {
        Charset sDefaultCharset = null;

        String sFileEncoding = System.getProperty("file.encoding");

        sDefaultCharset = Charset.forName(sFileEncoding);
        if (sDefaultCharset == null)
        {
            sDefaultCharset = Charset.forName("UTF-8");
        }

        return sDefaultCharset;
    }
    
    /**
     * Trace logging for system environments
     */
    private static void logSystemEnvironments()
    {
        String sSystemEnvironments = getSystemEnvironmentsString();
        
        mTraceLogger.logInfo(sSystemEnvironments);
    }

    /**
     * Get system environments
     * 
     * @return  System environments
     */
    private static String getSystemEnvironmentsString()
    {
        String sEnvironment = "System environments (";
        
        LinkedList sLinkedList = new LinkedList();
        sLinkedList.add("Java Version = "           + mJavaVersion);
        sLinkedList.add("Java Vendor = "            + mJavaVendor);
        sLinkedList.add("Java Selector Provider = " + mJavaSelectorProvider);
        sLinkedList.add("CPU Architecture = "       + mCPUArchitecture);
        sLinkedList.add("OS Name = "                + mOSName);
        sLinkedList.add("OS Version = "             + mOSVersion);
        sLinkedList.add("Default Character Set = "  + mDefaultCharset);

        Iterator sIterator = sLinkedList.iterator();
        if (sIterator.hasNext() == true)
        {
            sEnvironment += ((String)sIterator.next());
        }

        while (sIterator.hasNext() == true)
        {
            sEnvironment += ", " + ((String)sIterator.next());
        }
        
        sEnvironment += ")";
        
        return sEnvironment;
    }

    /**
     * Parse command line arguments
     * 
     * @param aArgs     Command line arguments
     */
    private static void parseCmdLineArgs(String[] aArgs)
    {
        HashMap sCmdLineArgsMap = parseCmdLineArgsMap(aArgs);

        Iterator sIterator = sCmdLineArgsMap.entrySet().iterator();
        while (sIterator.hasNext() == true)
        {
            Map.Entry sEntry = (Map.Entry)sIterator.next();

            String sKey = (String)sEntry.getKey();
            String sValue = (String)sEntry.getValue();

            if (sKey.compareTo(mListenPortOption) == 0)
            {
                if (sValue.length() > 0)
                {
                    mListenPort = Integer.valueOf(sValue).intValue();
                }
            }
            else if (sKey.compareTo(mTrcDirOption) == 0)
            {
                mTraceDir = new String(sValue);
            }
            else if (sKey.compareTo(mTrcFileNameOption) == 0)
            {
                mTraceFileName = new String(sValue);
            }
            else if ( sKey.compareTo( mVersionOption ) == 0 )
            {
                System.out.println( Version.getVersion() );
                System.exit( 0 );
            }
        }
    }
    
    /**
     * Parse command line arguments
     * 
     * @param aArgs     Command line arguments
     * @return          Option map of command line arguments parsing
     */
    private static HashMap parseCmdLineArgsMap(String[] aArgs)
    {
        HashMap sCmdLineArgsMap = new HashMap();
        
        int i;
        String sArg;
        
        for (i = 0; i < aArgs.length; ++i)
        {
            sArg = aArgs[i];

            if (sArg.length() > 1)
            {
                if (sArg.charAt(0) == '-')
                {
                    String[] sKeyValue = sArg.split("=", 2);

                    if (sKeyValue.length > 0)
                    {
                        if (sKeyValue.length == 1)
                        {
                            sCmdLineArgsMap.put(sKeyValue[0], "");
                        }
                        else
                        {
                            sCmdLineArgsMap.put(sKeyValue[0], sKeyValue[1]);
                        }
                    }
                }
            }
        }

        return sCmdLineArgsMap;
    }
    
    /**
     * Validate command line arguments
     * 
     * @return  true, if validate. false, if not.
     */
    private static boolean validateCmdLineArgs()
    {
        boolean sValidated = false;
        
        if (mListenPort > 0)
        {
            if (mTraceDir != null)
            {
                String sTraceDir = mTraceDir;
    
                File sFile = new File(sTraceDir);
                if (sFile.exists() == true)
                {
                    if (mTraceFileName != null)
                    {
                        sValidated = true;
                    }
                }
            }
        }
        
        return sValidated;
    }

    /**
     * Validate JAVA Version
     * 
     * @return  true, if validate. false, if not.
     */
    private static boolean validateJavaVersion()
    {
        boolean     sRC = false;
        String[]    sVersion = mJavaVersion.split( "\\." );
        int         sMajorVersion = 0;
        int         sMinorVersion = 0;

        sMajorVersion = Integer.parseInt( sVersion[0] );

        if ( sMajorVersion > REQUIRED_JAVA_MAJOR_VERSION ) 
        {
            sRC = true;
        }
        else if ( sMajorVersion == REQUIRED_JAVA_MAJOR_VERSION )
        {
            sMinorVersion = Integer.parseInt( sVersion[1] );
            if ( sMinorVersion >= REQUIRED_JAVA_MINOR_VERSION )
            {
                sRC = true;
            }
            else
            {
                sRC = false;
            }
        }
        else
        {
            sRC = false;
        }

        return sRC;
    }

    /**
     * Trace logging for command line arguments
     * 
     * @param aArgs     Command line arguments
     */
    private static void logCmdLineArgs(String[] aArgs)
    {
        String sCmdLineArgs = "Commnd line arguments (";

        if (aArgs.length > 0)
        {
            int i;
            for (i = 0; i < (aArgs.length - 1); ++i)
            {
                sCmdLineArgs += aArgs[i] + " ";
            }
    
            sCmdLineArgs += aArgs[aArgs.length - 1];
        }
        
        sCmdLineArgs += ")";
        
        mTraceLogger.logInfo(sCmdLineArgs);
    }
    
    /**
     * Print usage of AltiLinker application
     */
    private static void printUsage()
    {
        System.out.println("Usage: altilinker [OPTION]...");
        System.out.println("AltiLinker for ALTIBASE DB-Link");
        System.out.println("");
        System.out.println("OPTION:");
        System.out.println("  -listen_port               TCP listen port number (mandantory option)");
        System.out.println("  -trc_dir                   Directory for trace logging (mandantory option)");
        System.out.println("  -trc_file_name             Trace log file name (mandantory option)");
        System.out.println("  -v                         Print Altilinker Version (optional option)");
        System.out.println("");
    }
}
