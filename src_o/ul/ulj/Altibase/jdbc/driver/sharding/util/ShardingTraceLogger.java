/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.util;

import Altibase.jdbc.driver.util.AltibaseEnvironmentVariables;
import Altibase.jdbc.driver.util.RuntimeEnvironmentVariables;

import java.io.File;
import java.util.logging.*;

public class ShardingTraceLogger
{
    private static final String LOGGER_NAME  = ShardingTraceLogger.class.getName();
    private static final String LOGFILE_NAME = "shardjdbc.trc";
    private static final Logger mLogger      = Logger.getLogger(LOGGER_NAME);;

    public static final int DEFAULT_LOG_FILE_SIZE = 15242880;
    public static final int DEFALUT_LOG_FILE_ROTATE_CNT = 5;

    static
    {
        mLogger.setLevel(Level.OFF);

        String sLogLevelStr = RuntimeEnvironmentVariables.getVariable("SHARD_JDBC_TRCLOG_LEVEL", "OFF").toUpperCase();
        // BUG-47775 샤드 로그레벨 프로퍼티가 셋팅 되어 있을 때만 logger 객체를 초기화 한다.
        if (!"OFF".equals(sLogLevelStr))
        {
            initializeLogger(sLogLevelStr);
        }
    }

    private static void initializeLogger(String aLogLevelStr)
    {
        try
        {
            Level sCurrentLevel = Level.parse(aLogLevelStr);
            mLogger.setLevel(sCurrentLevel);

            Handler sHandler = new FileHandler(getLogFilePath(), DEFAULT_LOG_FILE_SIZE, DEFALUT_LOG_FILE_ROTATE_CNT, true);
            sHandler.setFormatter(new ShardSingleLineFormatter());
            mLogger.addHandler(sHandler);

            // decide whether or not this logger prints its output to stderr.
            // default: false (not printed to stderr)
            String sLogStderr = RuntimeEnvironmentVariables.getVariable("SHARD_JDBC_TRCLOG_PRINT_STDERR", "FALSE").toUpperCase();
            if (sLogStderr.equals("FALSE"))
            {
                mLogger.setUseParentHandlers(false);
            }
        }
        catch (Exception aEx)
        {
            // BUG-47775 초기화중 예외가 발생한 경우 에러메세지를 찍고 레벨을 OFF로 설정한다.
            System.err.println("Exception thrown : " + aEx.getMessage());
            mLogger.setLevel(Level.OFF);
        }
    }

    private static String getLogFilePath()
    {
        String sAltibaseHomePath = AltibaseEnvironmentVariables.getAltibaseHome();
        String sLogFilePath;

        if (sAltibaseHomePath != null)
        {
            if (sAltibaseHomePath.endsWith(File.separator))
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

        return sLogFilePath;
    }

    public static void shard_log(Level aLevel, String aLog, Exception aException)
    {
        mLogger.log(aLevel, aLog, aException);
    }

    public static void shard_log(Level aLevel, Exception aException)
    {
        mLogger.log(aLevel, aException.getMessage(), aException);
    }

    public static void shard_log(Level aLevel, String aLog)
    {
        mLogger.log(aLevel, aLog);
    }

    public static void shard_log(String aLog)
    {
        mLogger.log(Level.INFO, aLog);
    }

    public static void shard_log(String aLog, Object aParam)
    {
        mLogger.log(Level.INFO, aLog, aParam);
    }

    public static void shard_log(Level aLevel, String aLog, Object aParam)
    {
        mLogger.log(aLevel, aLog, aParam);
    }

    public static void shard_log(String aLog, Exception aException)
    {
        mLogger.log(Level.INFO, aLog, aException);
    }

    public static void shard_log(String aLog, Object[] aParams)
    {
        mLogger.log(Level.INFO, aLog, aParams);
    }

    public static void shard_log(Level aLevel, String aLog, Object[] aParams)
    {
        mLogger.log(aLevel, aLog, aParams);
    }

    private ShardingTraceLogger()
    {
    }
}
