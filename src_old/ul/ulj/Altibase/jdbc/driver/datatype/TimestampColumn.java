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

package Altibase.jdbc.driver.datatype;

import java.nio.ByteBuffer;
import java.sql.Date;
import java.sql.SQLException;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;
import java.util.Calendar;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class TimestampColumn extends CommonDateTimeColumn
{
    TimestampColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.TIMESTAMP);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.TIMESTAMP;
    }

    public String getObjectClassName()
    {
        return Timestamp.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return "yyyy-mm-dd hh:mm:ss.fffffffff".length();
    }

    /*
     * 이 메소드의 스펙은 다음과 같다.
     * 
     * [0]~[1] : 연도
     * [2]     : 월
     * [3]     : 일
     * [4]     : 시
     * [5]     : 분
     * [6]     : 초
     * [7]~[10]: 나노초
     */
    protected byte[] getBytesSub() throws SQLException
    {
        ByteBuffer sResult = ByteBuffer.allocate(11);
        mCalendar.setTimeInMillis(getTimeInMillis());
        sResult.putShort((short) mCalendar.get(Calendar.YEAR));
        sResult.put((byte) mCalendar.get(Calendar.MONTH));
        sResult.put((byte) mCalendar.get(Calendar.DATE));
        sResult.put((byte) mCalendar.get(Calendar.HOUR_OF_DAY));
        sResult.put((byte) mCalendar.get(Calendar.MINUTE));
        sResult.put((byte) mCalendar.get(Calendar.SECOND));
        sResult.putInt(getNanos());
        return sResult.array();
    }

    protected String getStringSub() throws SQLException
    {
        String sResult;
        Timestamp sTimestamp = getTimestampSub();
        String sDateFormat = getDateFormat();
        /* BUG-46513 date_format이 null이라면 timestamp의 toString을 호출하고 그렇지 않은 경우에는
           SimpleDateFormat으로 값을 formatting한다.  */
        if (sDateFormat == null)
        {
            sResult = sTimestamp.toString();
        }
        else
        {
            try
            {
                SimpleDateFormat sFormat = new SimpleDateFormat(sDateFormat);
                sResult = sFormat.format(sTimestamp);
            }
            catch (IllegalArgumentException aEx)
            {
                // BUG-46513 formatting시 오류가 발생한 경우에는 원본 timestamp객체의 toString을 호출한다.
                sResult = sTimestamp.toString();
            }
        }

        return sResult;
    }

    protected Object getObjectSub() throws SQLException
    {
        return getTimestampSub();
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof CommonDateTimeColumn)
        {
            setColumn((CommonDateTimeColumn)aValue);
            return;
        }

        long sTimeInMillis = 0;
        int sNanos = 0;
        if (aValue instanceof Timestamp)
        {
            Timestamp sTS = (Timestamp)aValue;
            sTimeInMillis = sTS.getTime();
            sNanos = sTS.getNanos();
        }
        else if (aValue instanceof Date)
        {
            // Oracle은 Date 타입으로 값을 설정할 때 yyyy-mm-dd만 받아들인다.
            mCalendar.setTime((Date)aValue);
            int sYY = mCalendar.get(Calendar.YEAR);
            int sMM = mCalendar.get(Calendar.MONTH);
            int sDD = mCalendar.get(Calendar.DATE);
            mCalendar.clear();
            mCalendar.set(sYY, sMM, sDD);
            sTimeInMillis = mCalendar.getTimeInMillis();
        }
        // Oracle은 Timestamp에 Time을 넣을 수 없다.
/*
        else if (aValue instanceof Time)
        {
            sTimeInMillis = ((Time)aValue).getTime();
        }
*/
        else if (aValue instanceof Long)
        {
            sTimeInMillis = ((Long)aValue).longValue();
        }
        else if (aValue instanceof String)
        {
            Timestamp sTS = Timestamp.valueOf((String)aValue);
            sTimeInMillis = sTS.getTime();
            sNanos = sTS.getNanos();
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
        setTime(sTimeInMillis, sNanos);
    }
}
