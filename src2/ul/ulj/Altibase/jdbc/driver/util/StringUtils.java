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

public final class StringUtils
{
    private StringUtils()
    {
    }

    /**
     * 문자열이 null이거나 비었는지 확인한다.
     * 
     * @param aStr 확인할 문자열
     * @return null 또는 길이가 0인 문자열인지 여부
     */
    public static boolean isEmpty(String aStr)
    {
        return (aStr == null) || (aStr.length() == 0);
    }

    /**
     * 대소문자 구별없이, starts string으로 시작하는지 확인한다.
     * 
     * @param aSrcStr 확인할 문자열
     * @param aStartsStr 시작 문자열
     * @return starts string으로 시작하면 true, 아니면 false
     */
    public static boolean startsWithIgnoreCase(String aSrcStr, String aStartsStr)
    {
        return startsWithIgnoreCase(aSrcStr, 0, aStartsStr);
    }

    /**
     * 대소문자 구별없이, 두 문자열을 비교한다.
     * 
     * @param aSrcStr 확인할 문자열
     * @param aStartIdx 확인할 문자열에서 비교를 시작할 위치(0 base)
     * @param aStartsStr 시작 문자열
     * @return start string으로 시작하면 true, 아니면 false
     */
    public static boolean startsWithIgnoreCase(String aSrcStr, int aStartIdx, String aStartsStr)
    {
        return compareIgnoreCase(aSrcStr, aStartIdx, aStartsStr, 0, aStartsStr.length()) == 0;
    }

    /**
     * 대소문자 구별없이, ends string으로 끝나는지 확인한다.
     * 
     * @param aSrcStr 확인할 문자열
     * @param aEndsStr 끝 문자열
     * @return ends string으로 끝나면 true, 아니면 false
     */
    public static boolean endsWithIgnoreCase(String aSrcStr, String aEndsStr)
    {
        int sPos = aSrcStr.length() - aEndsStr.length();
        return (sPos < 0) ? false : startsWithIgnoreCase(aSrcStr, sPos, aEndsStr);
    }

    /**
     * 대소문자 구별없이, 두 문자열을 비교한다.
     * 
     * @param aStr1 문자열 1
     * @param aStr2 문자열 2
     * @return 같으면 0, aStr1이 크거나 aStr2를 모두 포함하면서 길면 양수, 아니면 음수
     */
    public static int compareIgnoreCase(String aStr1, String aStr2)
    {
        return compareIgnoreCase(aStr1, 0, aStr2);
    }

    /**
     * 대소문자 구별없이, 두 문자열을 비교한다.
     * 
     * @param aStr1 문자열 1
     * @param aStartIdx1 aStr1에서 비교를 시작할 위치(0 base)
     * @param aStr2 문자열 2
     * @return 같으면 0, aStr1이 크거나 aStr2를 모두 포함하면서 길면 양수, 아니면 음수
     */
    public static int compareIgnoreCase(String aStr1, int aStartIdx1, String aStr2)
    {
        int sResult = compareIgnoreCase(aStr1, aStartIdx1, aStr2, 0, Math.min(aStr1.length(), aStr2.length()));
        if (sResult == 0)
        {
            // 비교 가능한 부분이 모두 같다면, 보다 긴 쪽이 크다.
            sResult = (aStr1.length() - aStartIdx1) - aStr2.length();
            sResult = (sResult > 0) ? 1 : (sResult < 0) ? -1 : 0;
        }
        return sResult;
    }

    /**
     * 대소문자 구별없이, 두 문자열을 비교한다.
     * 
     * @param aStr1 문자열 1
     * @param aStartIdx1 aStr1에서 비교를 시작할 위치(0 base)
     * @param aStr2 문자열 2
     * @param aStartIdx2 aStr2에서 비교를 시작할 위치(0 base)
     * @param aLength 비교할 길이
     * @return 같으면 0, aStr1이 크면 양수, 아니면 음수
     */
    public static int compareIgnoreCase(String aStr1, int aStartIdx1, String aStr2, int aStartIdx2, int aLength)
    {
        if ((aStr1 == aStr2) && (aStartIdx1 == aStartIdx2))
        {
            return 0;
        }

        int sStrLen1 = aStr1.length() - aStartIdx1;
        int sStrLen2 = aStr2.length() - aStartIdx2;
        if (sStrLen1 >= aLength && sStrLen2 < aLength)
        {
            return 1;
        }
        if (sStrLen1 < aLength && sStrLen2 >= aLength)
        {
            return -1;
        }

        for (int i = 0; i < aLength; i++)
        {
            char sChA = aStr1.charAt(aStartIdx1 + i);
            char sChB = aStr2.charAt(aStartIdx2 + i);
            int sCmp = Character.toLowerCase(sChA) - Character.toLowerCase(sChB);
            if (sCmp != 0)
            {
                return (sCmp > 0) ? 1 : -1;
            }
        }
        return 0;
    }
}
