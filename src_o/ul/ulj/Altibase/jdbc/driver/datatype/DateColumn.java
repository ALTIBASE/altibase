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
import java.util.Calendar;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class DateColumn extends CommonDateTimeColumn
{
    DateColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.DATE);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.DATE;
    }

    public String getObjectClassName()
    {
        return Date.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return "yyyy-mm-dd".length();
    }

    /*
     * 이 메소드의 스펙은 다음과 같다.
     * 
     * [0]~[1]: 연도
     * [2]    : 월
     * [3]    : 일
     */
    protected byte[] getBytesSub() throws SQLException
    {
        ByteBuffer sResult = ByteBuffer.allocate(11);
        mCalendar.setTimeInMillis(getTimeInMillis());
        sResult.putShort((short) mCalendar.get(Calendar.YEAR));
        sResult.put((byte) mCalendar.get(Calendar.MONTH));
        sResult.put((byte) mCalendar.get(Calendar.DATE));
        return sResult.array();
    }

    protected String getStringSub() throws SQLException
    {
        return getDateSub().toString();
    }

    protected Object getObjectSub() throws SQLException
    {
        return getDateSub();
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
            sTimeInMillis = Date.valueOf((String)aValue).getTime();
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
        setTime(sTimeInMillis, sNanos);
    }
}
