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

import java.util.Calendar;
import java.util.TimeZone;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class TimeZoneUtils
{
    public static final TimeZone TZ_GMT             = TimeZone.getTimeZone("GMT");
    public static final TimeZone TZ_SEOUL           = TimeZone.getTimeZone("Asia/Seoul");
    public static final TimeZone TZ_LONDON          = TimeZone.getTimeZone("Europe/London");
    public static final TimeZone TZ_NEWYORK         = TimeZone.getTimeZone("America/New_York");
    public static final TimeZone TZ_HONGKONG        = TimeZone.getTimeZone("Asia/Hong_Kong");
    public static final TimeZone TZ_SHANGHAI        = TimeZone.getTimeZone("Asia/Shanghai");
    public static final TimeZone TZ_SINGAPORE       = TimeZone.getTimeZone("Asia/Singapore");

    private static final Pattern NUMERIC_TZ_PATTERN = Pattern.compile("^([+-])([0-9]{1,2}):([0-9]{1,2})$");

    private TimeZoneUtils()
    {
    }

    /**
     * ID에 해당하는 TimeZone을 얻는다.
     * 
     * @param aID "Asia/Seoul"과 같은 ID 문자열이나 "GMT+09:00"과 같은 offset 문자열.
     *            "+09:00"와 같은 형태이면 자동으로 "GMT+09:00"에 대한 TimeZone을 얻는다.
     * @return ID에 해당하는 TimeZone.
     *         만약, 해당하는 TimeZone이 없으면 GMT zone.
     */
    public static TimeZone getTimeZone(String aID)
    {
        Matcher sMatcher = NUMERIC_TZ_PATTERN.matcher(aID);
        if (sMatcher.find())
        {
            // Java에서 알아차릴 수 있는 형태로 수정
            aID = "GMT" + aID;
        }
        return TimeZone.getTimeZone(aID);
    }

    /**
     * TimeZone을 변환한다.
     * 
     * @param aMillis 바꿀 시간(ms 단위)
     * @param aSrcCal 원래 TimeZone이 설정된 Calendar
     * @param aDstCal 바꿀 TimeZone이 설정된 Calendar
     * @return TimeZone 변환한 milliseconds 값
     */
    public static long convertTimeZone(long aMillis, Calendar aSrcCal, Calendar aDstCal)
    {
        aSrcCal.setTimeInMillis(aMillis);

        aDstCal.set(Calendar.YEAR, aSrcCal.get(Calendar.YEAR));
        aDstCal.set(Calendar.MONTH, aSrcCal.get(Calendar.MONTH));
        aDstCal.set(Calendar.DATE, aSrcCal.get(Calendar.DATE));
        aDstCal.set(Calendar.HOUR_OF_DAY, aSrcCal.get(Calendar.HOUR_OF_DAY));
        aDstCal.set(Calendar.MINUTE, aSrcCal.get(Calendar.MINUTE));
        aDstCal.set(Calendar.SECOND, aSrcCal.get(Calendar.SECOND));
        aDstCal.set(Calendar.MILLISECOND, aSrcCal.get(Calendar.MILLISECOND));

        return aDstCal.getTimeInMillis();
    }

    /**
     * TimeZone을 변환한다.
     * 
     * @param aMillis 바꿀 시간(ms 단위)
     * @param aDstCal 바꿀 TimeZone이 설정된 Calendar
     * @return TimeZone 변환한 milliseconds 값
     */
    public static long convertTimeZone(long aMillis, Calendar aDstCal)
    {
        return convertTimeZone(aMillis, Calendar.getInstance(), aDstCal);
    }

    /**
     * TimeZone을 변환한다.
     * 
     * @param aMillis 바꿀 시간(ms 단위)
     * @param sSrcCal 원래 TimeZone
     * @param aDstCal 바꿀 TimeZone
     * @return TimeZone 변환한 milliseconds 값
     */
    public static long convertTimeZone(long aMillis, TimeZone aSrcTZ, TimeZone aDstTZ)
    {
        return convertTimeZone(aMillis, Calendar.getInstance(aSrcTZ), Calendar.getInstance(aDstTZ));
    }

    /**
     * TimeZone을 변환한다.
     * 
     * @param aMillis 바꿀 시간(ms 단위)
     * @param aDstCal 바꿀 TimeZone
     * @return TimeZone 변환한 milliseconds 값
     */
    public static long convertTimeZone(long aMillis, TimeZone aDstTZ)
    {
        return convertTimeZone(aMillis, Calendar.getInstance(aDstTZ));
    }
}
