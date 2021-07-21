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

import java.util.Enumeration;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;

/**
 * key 값으로 대소문자를 구별하지 않는 <tt>Properties</tt> 클래스.
 */
public class CaseInsensitiveProperties extends Properties
{
    private static final long serialVersionUID = 1720730086780858495L;

    public CaseInsensitiveProperties()
    {
    }

    public CaseInsensitiveProperties(CaseInsensitiveProperties aProp)
    {
        this.defaults = aProp;
    }

    public CaseInsensitiveProperties(Properties aProp)
    {
        if (aProp instanceof CaseInsensitiveProperties)
        {
            this.defaults = aProp;
        }
        else if (aProp != null)
        {
            // key가 lowercase로 되어있는 Properties 생성
            Properties sProp = createProperties();
            Enumeration sKeys = aProp.keys();
            while (sKeys.hasMoreElements())
            {
                String sKey = (String) sKeys.nextElement();
                String sValue = aProp.getProperty(sKey);

                sProp.setProperty(sKey, sValue);
            }

            this.defaults = sProp;
        }
    }

    protected Properties createProperties()
    {
        return new CaseInsensitiveProperties();
    }

    public synchronized boolean containsKey(Object aKey)
    {
        return super.containsKey(caseInsentiveKey(aKey));
    }

    public synchronized Object get(Object aKey)
    {
        return super.get(caseInsentiveKey(aKey));
    }

    public synchronized Object put(Object aKey, Object aValue)
    {
        return super.put(caseInsentiveKey(aKey), aValue);
    }

    public synchronized Object remove(Object aKey)
    {
        return super.remove(caseInsentiveKey(aKey));
    }

    private String caseInsentiveKey(Object aKey)
    {
        String sCaseInsensitiveKey;
        if (aKey == null)
        {
            sCaseInsensitiveKey = null;
        }
        else
        {
            sCaseInsensitiveKey = aKey.toString().trim().toLowerCase();
        }
        return sCaseInsensitiveKey;
    }

    public synchronized void putAll(Map aMap)
    {
        if (aMap instanceof CaseInsensitiveProperties)
        {
            super.putAll(aMap);
        }
        else
        {
            Iterator sIt = aMap.keySet().iterator();
            while (sIt.hasNext())
            {
                Object sKey = sIt.next();
                put(sKey, aMap.get(sKey));
            }
        }
    }

    /**
     * Map에 있는 값쌍을 Preperty로 추가한다.
     * 단, Map의 key 값으로 얻을 수 있는 값이 이미 있다면 추가하지 않는다.
     *
     * @param aMap 추가할 값 쌍을 가진 객체
     */
    public synchronized void putAllExceptExist(Map aMap)
    {
        Iterator sIt = aMap.keySet().iterator();
        while (sIt.hasNext())
        {
            Object sKey = sIt.next();
            if (getProperty(caseInsentiveKey(sKey)) == null)
            {
                put(sKey, aMap.get(sKey));
            }
        }
    }

    public AltibaseProperties getDefaults()
    {
        return (AltibaseProperties)defaults;
    }

    @Override
    public String getProperty(String aKey)
    {
        return super.getProperty(caseInsentiveKey(aKey));
    }
}
