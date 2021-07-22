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

package Altibase.jdbc.driver.logging;

import java.io.IOException;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;
import java.util.logging.FileHandler;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.LogRecord;
import java.util.logging.XMLFormatter;

/**
 * 세션별로 별도의 파일이 생성되어 로그를 남길수 있도록 FileHandler를 확장하여 구현함.</br>
 * 내부적으로 FileHandler의 집합을 Hashtable로 구현하고 있으며 세션아이디별로 FileHandler를 생성하여 관리한다.
 * 
 * @author yjpark
 *
 */
public class MultipleFileHandler extends FileHandler
{
    public static final String PROPERTY_PATTERN     = "Altibase.jdbc.driver.logging.MultipleFileHandler.pattern";
    public static final String PROPERTY_LIMIT       = "Altibase.jdbc.driver.logging.MultipleFileHandler.limit";
    public static final String PROPERTY_COUNT       = "Altibase.jdbc.driver.logging.MultipleFileHandler.count";
    public static final String PROPERTY_APPEND      = "Altibase.jdbc.driver.logging.MultipleFileHandler.append";
    public static final String PROPERTY_LEVEL       = "Altibase.jdbc.driver.logging.MultipleFileHandler.level";
    public static final String CM_HANDLERS          = "altibase.jdbc.cm.handlers";
    public static final String HANDLER_NAME         = "Altibase.jdbc.driver.logging.MultipleFileHandler";
    public static final String DEFAULT_PATTERN      = "%h/jdbc_net_%s.log";
    public static final String DEFAULT_APPEND       = "false";
    public static final String DEFAULT_LIMIT        = "10000000";
    public static final String DEFAULT_COUNT        = "1";
    public static final String DEFAULT_LEVEL        = "FINEST";
    String              mLocalPattern;
    String              mLevel;
    boolean             mLocalAppend;
    int                 mLocalLimit;
    int                 mLocalCount;
    Map                 mHandlerMap      = new Hashtable(); // 쓰레드간 경합이슈가 있기때문에 HashMap대신 Hashtable을 사용한다.

    public MultipleFileHandler() throws IOException
    {
        super(getFilename(getProperty(PROPERTY_PATTERN, "%h/jdbc_net_%s.log"), "MAIN"), 
              Integer.parseInt(getProperty(PROPERTY_LIMIT, DEFAULT_LIMIT)), 
              Integer.parseInt(getProperty(PROPERTY_COUNT, DEFAULT_COUNT)), 
              Boolean.getBoolean(getProperty(PROPERTY_APPEND, DEFAULT_APPEND)));
    }

    public MultipleFileHandler(String aPattern) throws IOException
    {
        super(getFilename(aPattern, "MAIN"), 
              Integer.parseInt(getProperty(PROPERTY_LIMIT, DEFAULT_LIMIT)), 
              Integer.parseInt(getProperty(PROPERTY_COUNT, DEFAULT_COUNT)), 
              Boolean.getBoolean(getProperty(PROPERTY_APPEND, DEFAULT_APPEND)));
    }

    public MultipleFileHandler(String aPattern, boolean aAppend) throws IOException
    {
        super(getFilename(aPattern, "MAIN"), 
              Integer.parseInt(getProperty(PROPERTY_LIMIT, DEFAULT_LIMIT)), 
              Integer.parseInt(getProperty(PROPERTY_COUNT, DEFAULT_COUNT)), aAppend);
    }

    public MultipleFileHandler(String aPattern, int aLimit, int aCount) throws IOException
    {
        super(getFilename(aPattern, "MAIN"), aLimit, aCount, 
              Boolean.getBoolean(getProperty(PROPERTY_APPEND, DEFAULT_APPEND)));
    }

    public MultipleFileHandler(String aPattern, int aLimit, int aCount, boolean aAppend) throws IOException
    {
        super(getFilename(aPattern, "MAIN"), aLimit, aCount, aAppend);
    }

    void initValues()
    {
        this.mLocalPattern = getProperty(PROPERTY_PATTERN, "%h/jdbc_net_%s.trc");
        this.mLocalLimit = Integer.parseInt(getProperty(PROPERTY_LIMIT, DEFAULT_LIMIT));
        this.mLocalCount = Integer.parseInt(getProperty(PROPERTY_COUNT, DEFAULT_COUNT));
        this.mLocalAppend = Boolean.getBoolean(getProperty(PROPERTY_APPEND, DEFAULT_APPEND));
        this.mLevel = getProperty(PROPERTY_LEVEL, DEFAULT_LEVEL);
    }

    static final String getFilename(String aPattern, String aSuffix)
    {
        String sResult = (aPattern == null) ? "%h/jdbc_net_%s.log" : aPattern;
        
        if (sResult.indexOf("%s") >= 0)
        {
            sResult = sResult.replaceAll("%s", aSuffix);
        }
        else
        {
            sResult = sResult + "." + aSuffix;
        }

        return sResult;
    }

    static String getProperty(String aName, String aDefaultValue)
    {
        String sProperty = LogManager.getLogManager().getProperty(aName);
        return sProperty != null ? sProperty : aDefaultValue;
    }

    public void publish(LogRecord aRecord)
    {
        Object[] sParam = aRecord.getParameters();
        if (sParam != null && sParam.length > 0)
        {
            Handler sHandler = (Handler)this.mHandlerMap.get(sParam[0]);

            // Hashtable에 해당하는 세션아이디의 FileHandler가 없는 경우 새로 생성하여 추가한다.
            if (sHandler == null)
            {
                if (this.mLocalPattern == null)
                {
                    initValues();
                }
                
                try
                {
                    sHandler = new FileHandler(getFilename(this.mLocalPattern, (String)sParam[0]), this.mLocalLimit, 
                                              this.mLocalCount, this.mLocalAppend);
                    sHandler.setFormatter(new XMLFormatter());
                    sHandler.setFilter(getFilter());
                    sHandler.setLevel(Level.parse(this.mLevel));
                    sHandler.setEncoding(getEncoding());
                    sHandler.setErrorManager(getErrorManager());
                }
                catch (IOException sIOE)
                {
                    reportError("Unable open FileHandler", sIOE, 0);
                }

                this.mHandlerMap.put(sParam[0], sHandler);
            }
            sHandler.publish(aRecord);
        }
        else
        {
            super.publish(aRecord);
        }
    }

    public void close()
    {
        for (Iterator sItr = this.mHandlerMap.values().iterator(); sItr.hasNext();)
        {
            Handler sHandler = (Handler)sItr.next();
            sHandler.close();
        }
        super.close();
    }
}
