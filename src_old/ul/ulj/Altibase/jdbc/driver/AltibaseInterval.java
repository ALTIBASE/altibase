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

import java.nio.ByteBuffer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

/**
 * Altibae INTERVAL은 second과 nanos로 이루어진 시간차 값으로, DATE간 차이를 구했을 때 그 결과로 얻는다.
 */
public class AltibaseInterval
{
    public static final int              BYTES_SIZE = 16;
    public static final AltibaseInterval NULL       = new AltibaseInterval(Long.MIN_VALUE, Long.MIN_VALUE);

    private final long                   mSecond;
    private final long                   mNanos;

    public AltibaseInterval(long aSecond, long aNanos)
    {
        mSecond = aSecond;
        mNanos = aNanos;
    }

    public boolean isNull()
    {
        return (mSecond == NULL.mSecond) &&
               (mNanos == NULL.mNanos);
    }

    /**
     * second 값을 얻는다.
     * 
     * @return second 값
     */
    public long getSecond()
    {
        return mSecond;
    }

    /**
     * nanos 값을 얻는다.
     * 
     * @return nanos 값을 얻는다.
     */
    public long getNanos()
    {
        return mNanos;
    }

    public int hashCode()
    {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int)(mNanos ^ (mNanos >>> 32));
        result = prime * result + (int)(mSecond ^ (mSecond >>> 32));
        return result;
    }

    public boolean equals(Object aObj)
    {
        if (this == aObj)
        {
            return true;
        }
        if (aObj == null)
        {
            return false;
        }
        if (getClass() != aObj.getClass())
        {
            return false;
        }

        AltibaseInterval sOther = (AltibaseInterval)aObj;
        return (mNanos == sOther.mNanos) &&
               (mSecond == sOther.mSecond);
    }

    public String toString()
    {
        return toString(mSecond, mNanos);
    }

    /**
     * Interval을 "dd hh:mm:ss.ff9" 형식의 문자열로 변환한다.
     * 여기서 ff9는 nanosecond 단위이다.
     * 
     * @param aInterval Interval 객체
     * @return "dd hh:mm:ss.ff9" 포맷으로 변환한 문자열
     */
    public static String toString(AltibaseInterval aInterval)
    {
        return toString(aInterval.mSecond, aInterval.mNanos);
    }

    /**
     * second, nanosecond를 "dd hh:mm:ss.ff9" 형식의 문자열로 변환한다.
     * 여기서 ff9는 nanosecond 단위이다.
     * 
     * @param aSecond second
     * @param aNanos nanosecond
     * @return "dd hh:mm:ss.ff9" 포맷으로 변환한 문자열
     */
    public static String toString(long aSecond, long aNanos)
    {
        long sSS = aSecond % 60;
        aSecond /= 60;
        long sMM = aSecond % 60;
        aSecond /= 60;
        long sHH = aSecond % 24;
        long sDD = aSecond / 24;
        long sFF = aNanos;

        return sDD + " " + sHH + ":" + sMM + ":" + sSS + "." + sFF;
    }

    private static final Pattern INTERVAL_PATTERN = Pattern.compile("^(\\d+) (\\d{1,2}):(\\d{1,2}):(\\d{1,2})\\.(\\d{1,9})$");
    private static final int     INTERVAL_GRP_DD  = 1;
    private static final int     INTERVAL_GRP_HH  = 2;
    private static final int     INTERVAL_GRP_MM  = 3;
    private static final int     INTERVAL_GRP_SS  = 4;
    private static final int     INTERVAL_GRP_FF  = 5;

    public static AltibaseInterval valueOf(String aStr)
    {
        Matcher sMatcher = INTERVAL_PATTERN.matcher(aStr);
        if (!sMatcher.matches())
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT, "Format of interval string", "dd hh:mm:ss.ff9",
                                                aStr);
        }

        String sStrVal;
        long sSecond = 0;
        if ((sStrVal = sMatcher.group(INTERVAL_GRP_DD)) != null)
        {
            sSecond = Long.parseLong(sStrVal) * (60 * 60 * 24);
        }
        if ((sStrVal = sMatcher.group(INTERVAL_GRP_HH)) != null)
        {
            long sHH = Long.parseLong(sStrVal);
            if (sHH > 23)
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT, "Hour", "0 ~ 23", String.valueOf(sHH));
            }
            sSecond += sHH * (60 * 60);
        }
        if ((sStrVal = sMatcher.group(INTERVAL_GRP_MM)) != null)
        {
            long sMM = Long.parseLong(sStrVal);
            if (sMM > 59)
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT, "Minute", "0 ~ 59", String.valueOf(sMM));
            }
            sSecond += sMM * 60;
        }
        if ((sStrVal = sMatcher.group(INTERVAL_GRP_SS)) != null)
        {
            long sSS = Long.parseLong(sStrVal);
            if (sSS > 59)
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT, "Second", "0 ~ 59", String.valueOf(sSS));
            }
            sSecond += sSS;
        }
        long sNanos = 0;
        if ((sStrVal = sMatcher.group(INTERVAL_GRP_FF)) != null)
        {
            sNanos = Long.parseLong(sStrVal);
        }
        return new AltibaseInterval(sSecond, sNanos);
    }

    private static final long SECOND_OF_DAY = 60L * 60L * 24L;
    private static final long NANOS_OF_DAY  = SECOND_OF_DAY * 1000000000L;

    /**
     * 날짜로 계산했을 때 얼마 차이인지를 구한다.
     * 하루가 안되는 시간은 소수점으로 표현한다.
     * 
     * @return 날짜 단위로 본 차이값
     */
    public double toNumberOfDays()
    {
        return toNumberOfDays(mSecond, mNanos);
    }

    /**
     * 날짜로 계산했을 때 얼마 차이인지를 구한다.
     * 하루가 안되는 시간은 소수점으로 표현한다.
     * 
     * @param aSecond second 값
     * @param aNanos nanos 값
     * @return 날짜 단위로 본 차이값
     */
    public static double toNumberOfDays(long aSecond, long aNanos)
    {
        return ((double)aSecond / SECOND_OF_DAY) +
               ((double)aNanos / NANOS_OF_DAY);
    }

    public byte[] toBytes()
    {
        return toBytes(mSecond, mNanos);
    }

    public static byte[] toBytes(long aSecond, long aNanos)
    {
        ByteBuffer sBuf = ByteBuffer.allocate(BYTES_SIZE);
        sBuf.putLong(aSecond);
        sBuf.putLong(aNanos);
        return sBuf.array();
    }

    public static AltibaseInterval valueOf(byte[] aBytes)
    {
        if (aBytes.length != BYTES_SIZE)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT, "Length of interval bytes", String.valueOf(BYTES_SIZE),
                                                String.valueOf(aBytes.length));
        }

        ByteBuffer sBuf = ByteBuffer.wrap(aBytes);
        long sSecond = sBuf.getLong();
        long sNanos = sBuf.getLong();
        return new AltibaseInterval(sSecond, sNanos);
    }
}
