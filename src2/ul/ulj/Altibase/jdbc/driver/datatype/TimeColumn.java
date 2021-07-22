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

import java.sql.Date;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.Calendar;

import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class TimeColumn extends CommonDateTimeColumn
{
    TimeColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.TIME);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.TIME;
    }

    public String getObjectClassName()
    {
        return Time.class.getName();
    }

    public int getMaxDisplaySize()
    {
        return "hh:mm:ss".length();
    }

    /*
     * 이 메소드의 스펙은 다음과 같다.
     * 
     * [0]: 시
     * [1]: 분
     * [2]: 초
     */
    protected byte[] getBytesSub() throws SQLException
    {
        byte[] sResult = new byte[3];
        mCalendar.setTimeInMillis(getTimeInMillis());
        sResult[0] = (byte)mCalendar.get(Calendar.HOUR_OF_DAY);
        sResult[1] = (byte)mCalendar.get(Calendar.MINUTE);
        sResult[2] = (byte)mCalendar.get(Calendar.SECOND);
        return sResult;
    }

    protected String getStringSub() throws SQLException
    {
        return getTimeSub().toString();
    }

    protected Object getObjectSub() throws SQLException
    {
        return getTimeSub();
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
        if (aValue instanceof CommonDateTimeColumn)
        {
            setColumn((CommonDateTimeColumn)aValue);
            return;
        }

        long sTimeInMillis = 0;
        if (aValue instanceof Timestamp)
        {
            Timestamp sTS = (Timestamp)aValue;
            sTimeInMillis = sTS.getTime();
        }
        else if (aValue instanceof Date)
        {
            sTimeInMillis = ((Date)aValue).getTime();
        }
        else if (aValue instanceof Time)
        {
            sTimeInMillis = ((Time)aValue).getTime();
        }
        else if (aValue instanceof Long)
        {
            sTimeInMillis = ((Long)aValue).longValue();
        }
        else if (aValue instanceof String)
        {
            sTimeInMillis = Time.valueOf((String)aValue).getTime();
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
        setTime(sTimeInMillis, 0);
    }
}
