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

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltibaseEnvironmentVariables;

/**
 * DataSource를 관리하기 위한 클래스.
 * <p>
 * DataSource 설정을 읽고, DSN에 해당하는 DataSource 객체를 반환해준다.
 */
final class AltibaseDataSourceManager
{
    private static final String              ALTIBASE_CLI_CONFIG_FILENAME = "altibase_cli.ini";
    private static final char                T_COMMENT                    = '#';
    private static final char                T_SEPERATOR                  = '=';
    private static final char                T_DSN_OPEN                   = '[';
    private static final char                T_DSN_CLOSE                  = ']';

    private static AltibaseDataSourceManager mInstance                    = new AltibaseDataSourceManager();

    private HashMap                          mDataSourceHash;

    public static AltibaseDataSourceManager getInstance()
    {
        return mInstance;
    }

    // for SingleTone declare private constructor.
    private AltibaseDataSourceManager()
    {
        mDataSourceHash = new HashMap();
        try
        {
            load();
        }
        catch (Exception e)
        {
            // ignore.
        }
    }

    /**
     * aDSN에 해당하는 AltibaseDataSource를 얻는다.
     * 
     * @param aDSN Data Source Name (case insensitive)
     * @return aDSN에 해당하는 AltibaseDataSource 객체. 없으면 null.
     */
    public AltibaseDataSource getDataSource(String aDSN)
    {
        if (aDSN != null)
        {
            aDSN = aDSN.toLowerCase();
        }
        return (AltibaseDataSource) mDataSourceHash.get(aDSN);
    }

    /**
     * DataSource 설정 파일을 읽는다.
     * <p>
     * 우선순위는 다음과 같으며, 이 중 한 파일만 읽는다:
     * <ol>
     * <li>./altibase_cli.ini</li>
     * <li>$HOME/altibase_cli.ini</li>
     * <li>$ALTIBASE_HOME/conf/altibase_cli.ini</li>
     * </ol>
     */
    private void load()
    {
        String sConfPaths[] = new String[3];
        int sConfPathCnt = 0;
        sConfPaths[sConfPathCnt++] = "./" + ALTIBASE_CLI_CONFIG_FILENAME;
        String sEnvHome = AltibaseEnvironmentVariables.getHome();
        if (sEnvHome != null)
        {
            sConfPaths[sConfPathCnt++] = sEnvHome + "/" + ALTIBASE_CLI_CONFIG_FILENAME;
        }
        String sEnvAltiHome = AltibaseEnvironmentVariables.getAltibaseHome();
        if (sEnvAltiHome != null)
        {
            sConfPaths[sConfPathCnt++] = sEnvAltiHome + "/conf/" + ALTIBASE_CLI_CONFIG_FILENAME;
        }

        for (int i = 0; i < sConfPathCnt; i++)
        {
            try
            {
                load(sConfPaths[i]);
                break;
            }
            catch (FileNotFoundException ex)
            {
                continue;
            }
        }
    }

    /**
     * 지정한 DataSource 설정 파일을 읽는다.
     * 
     * @param aConfFilePath 읽을 설정파일 경로
     * @throws FileNotFoundException 설정파일이 없는 경우
     */
    private void load(String aConfFilePath) throws FileNotFoundException
    {
        FileReader sFileReader = new FileReader(aConfFilePath);
        
        BufferedReader sBufferedReader = new BufferedReader(sFileReader);
        String sCurDSN = null;
        while (true)
        {
            String sString = null;
            
            try
            {
                sString = sBufferedReader.readLine();
            }
            catch (IOException e)
            {
                try
                {
                    sBufferedReader.close();
                    sFileReader.close();
                } 
                catch (IOException sCloseException)
                {
                    //IGNORE
                }
                break;
            }
            
            if (sString == null)
            {
                break;
            }

            sString = sString.trim();
            if ((sString.length() == 0) || (sString.charAt(0) == T_COMMENT))
            {
                continue;
            }

            if (sString.charAt(0) == T_DSN_OPEN)
            {
                int sDataSourceNameEndPosition = sString.indexOf(T_DSN_CLOSE);
                if (sDataSourceNameEndPosition == -1)
                {
                    // [ DataSource .... , "]" missing.
                    continue;
                }

                sCurDSN = sString.substring(1, sDataSourceNameEndPosition);
                sCurDSN = sCurDSN.trim().toLowerCase();
                if (mDataSourceHash.containsKey(sCurDSN) == false)
                {
                    mDataSourceHash.put(sCurDSN, new AltibaseDataSource(sCurDSN));
                }
                else
                {
                    sCurDSN = null;
                }
            }
            else
            {
                if (sCurDSN == null)
                {
                    // wrong state skip
                    continue;
                }

                AltibaseDataSource sDataSource = (AltibaseDataSource) mDataSourceHash.get(sCurDSN);
                if (sDataSource == null)
                {
                    Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION);
                }
                int sAssignOpPosition = sString.indexOf(T_SEPERATOR);
                if (sAssignOpPosition == -1)
                {
                    // woring line skip
                    continue;
                }

                String sAttr = sString.substring(0, sAssignOpPosition).trim();
                String sValue = sString.substring(sAssignOpPosition + 1, sString.length());
                int sValueEndPosition = sValue.indexOf(T_COMMENT);
                if (sValueEndPosition != -1)
                {
                    // skip tailing comment. ex) MEM_DB_DIR = ?/dbs # Memory DB Directory
                    sValue = sValue.substring(0, sValueEndPosition);
                }
                sValue = sValue.trim();
                sDataSource.setProperty(sAttr, sValue);
            }
        }
        
        try
        {
            if(sBufferedReader != null)
            {
                sBufferedReader.close();
            }
            
            if(sFileReader != null)
            {
                sFileReader.close();
            }
        } 
        catch (IOException sCloseException)
        {
            //IGNORE
        }
    }
}
