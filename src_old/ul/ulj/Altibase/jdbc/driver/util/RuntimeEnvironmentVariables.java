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

import java.util.Map;
import java.util.Properties;

public final class RuntimeEnvironmentVariables
{
    private static final Properties mEnvVars;

    static
    {
        mEnvVars = getEnvironmentVariables();
        mEnvVars.putAll(System.getProperties());
    }

    private RuntimeEnvironmentVariables()
    {
    }

    private synchronized static Properties getEnvironmentVariables()
    {
        // BUG-47115 서브프로세스를 생성시키지 않고 java.lang.System.getenv()를 통해 환경변수를 가져온다.
        Map<String, String> sSystemEnvMap = System.getenv();
        Properties sProps = new Properties();
        sProps.putAll(sSystemEnvMap);
        return sProps;
    }

    public static boolean isSet(String aKey)
    {
        return (getVariable(aKey) != null);
    }

    public static String getVariable(String aKey)
    {
        return getVariable(aKey, null);
    }

    public static String getVariable(String aKey, String aDefaultValue)
    {
        return mEnvVars.getProperty(aKey, aDefaultValue);
    }

    public static int getIntVariable(String aKey)
    {
        return getIntVariable(aKey, 0);
    }

    public static int getIntVariable(String aKey, int aDefaultValue)
    {
        String sValueStr = getVariable(aKey);

        return (sValueStr == null) ? aDefaultValue : Integer.parseInt(sValueStr);
    }

    public static boolean getBooleanVariable(String aKey)
    {
        return getBooleanVariable(aKey, false);
    }

    public static boolean getBooleanVariable(String aKey, boolean aDefaultValue)
    {
        boolean sValue = aDefaultValue;
        String sPropValue = getVariable(aKey);
        if (sPropValue != null)
        {
            sPropValue = sPropValue.toUpperCase();
            sValue = sPropValue.equals("1") ||
                     sPropValue.equals("TRUE") ||
                     sPropValue.equals("T") ||
                     sPropValue.equals("ON") ||
                     sPropValue.equals("O") ||
                     sPropValue.equals("YES") ||
                     sPropValue.equals("Y");
        }
        return sValue;
    }

    /**
     * Java 가상 머신이 이용할 수 있는 CPU 코어 수를 돌려 준다.
     * @return Java 가상 머신에서 사용할 수 있는 최대 코어 수
     */
    public static int getAvailableProcessors()
    {
        return Runtime.getRuntime().availableProcessors();
    }

    public static int getTotalVariableLength()
    {
        return mEnvVars.size();
    }
}
