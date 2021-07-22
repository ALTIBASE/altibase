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

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.Reader;
import java.io.StringReader;
import java.sql.Date;
import java.sql.SQLException;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.Calendar;

import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.ObjectDynamicArray;
import Altibase.jdbc.driver.util.TimeZoneUtils;

abstract class CommonDateTimeColumn extends AbstractColumn
{
    private static final int   MTD_DATE_MON_SHIFT       = 10;
    private static final int   MTD_DATE_DAY_SHIFT       = 5;
    private static final int   MTD_DATE_MIN_SHIFT       = 26;
    private static final int   MTD_DATE_SEC_SHIFT       = 20;

    private static final int   MTD_DATE_MON_MASK        = 0x3C00;
    private static final int   MTD_DATE_DAY_MASK        = 0x03E0;
    private static final int   MTD_DATE_HOUR_MASK       = 0x001F;
    private static final int   MTD_DATE_MIN_MASK        = 0xFC000000;
    private static final int   MTD_DATE_SEC_MASK        = 0x03F00000;
    private static final int   MTD_DATE_MSEC_MASK       = 0x000FFFFF;

    private static final short MTD_DATE_NULL_YEAR       = -32768;
    private static final short MTD_DATE_NULL_MONDAYHOUR = 0;
    private static final int   MTD_DATE_NULL_MINSECMIC  = 0;

    private static final int   DATE_BYTE_SIZE           = 8;

    protected Calendar         mCalendar                = Calendar.getInstance();
    private Timestamp          mTimestamp               = new Timestamp(0);
    private boolean            mIsNullValueSet;
    // BUG-46513 date_format jdbc 속성값을 저장하기 위한 멤버변수
    private String             mDateFormat;

    CommonDateTimeColumn()
    {
    }

    public String getDBColumnTypeName()
    {
        return "DATE";
    }

    public int getOctetLength()
    {
        return DATE_BYTE_SIZE;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new ObjectDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof ObjectDynamicArray);
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return DATE_BYTE_SIZE;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        short sYear;
        short sMonDayHour;
        int sMinSecMic;

        if (isNullValueSet())
        {
            sYear       = MTD_DATE_NULL_YEAR;
            sMonDayHour = MTD_DATE_NULL_MONDAYHOUR;
            sMinSecMic  = MTD_DATE_NULL_MINSECMIC;
        }
        else
        {
            mCalendar.setTimeInMillis(mTimestamp.getTime());
            sYear = (short) mCalendar.get(Calendar.YEAR);
            sMonDayHour = (short)
                        ( ((mCalendar.get(Calendar.MONTH) + 1) << MTD_DATE_MON_SHIFT)
                        | (mCalendar.get(Calendar.DATE) << MTD_DATE_DAY_SHIFT)
                        | mCalendar.get(Calendar.HOUR_OF_DAY) );
            sMinSecMic = (mCalendar.get(Calendar.MINUTE) << MTD_DATE_MIN_SHIFT)
                       | (mCalendar.get(Calendar.SECOND) << MTD_DATE_SEC_SHIFT)
                       | (mTimestamp.getNanos() / 1000);
        }

        aBufferWriter.writeShort(sYear);
        aBufferWriter.writeShort(sMonDayHour);
        aBufferWriter.writeInt(sMinSecMic);

        return DATE_BYTE_SIZE;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
    }

    public void storeTo(DynamicArray aArray)
    {
        if (isNullValueSet())
        {
            ((ObjectDynamicArray) aArray).put(null);
        }
        else
        {
            Timestamp sTS = new Timestamp(mTimestamp.getTime());
            sTS.setNanos(mTimestamp.getNanos());
            ((ObjectDynamicArray) aArray).put(sTS);
        }
    }

    public void storeTo()
    {
        if (isNullValueSet())
        {
            mValues.add(null);
        }
        else
        {
            Timestamp sTS = new Timestamp(mTimestamp.getTime());
            sTS.setNanos(mTimestamp.getNanos());
            mValues.add(sTS);
        }
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        Timestamp sTimestamp = readTimestamp(aChannel);
        if (sTimestamp == null)
        {
            setNullValue();
        }
        else
        {
            this.mTimestamp = sTimestamp;
        }
    }

    private Timestamp readTimestamp(CmChannel aChannel) throws SQLException
    {
        short sYear = aChannel.readShort();
        short sMonDayHour = aChannel.readShort();
        int sMinSecMicroSec = aChannel.readInt();
        Timestamp sTimestamp = new Timestamp(0);

        if (isNullMtdDate(sYear, sMonDayHour, sMinSecMicroSec))
        {
            return null;
        }

        // BUG-47460 매번 Calendar객체를 생성하지 않고 멤버변수를 재활용 한다.
        mCalendar.set(sYear,
                      ((sMonDayHour & MTD_DATE_MON_MASK) >>> MTD_DATE_MON_SHIFT) - 1,
                      ((sMonDayHour & MTD_DATE_DAY_MASK) >>> MTD_DATE_DAY_SHIFT),
                      ((sMonDayHour & MTD_DATE_HOUR_MASK)),
                      ((sMinSecMicroSec & MTD_DATE_MIN_MASK) >>> MTD_DATE_MIN_SHIFT),
                      ((sMinSecMicroSec & MTD_DATE_SEC_MASK) >>> MTD_DATE_SEC_SHIFT));
        int sNanos = (sMinSecMicroSec & MTD_DATE_MSEC_MASK) * 1000;
        sTimestamp.setTime(mCalendar.getTimeInMillis());
        sTimestamp.setNanos(sNanos);

        return sTimestamp;
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((ObjectDynamicArray)aArray).put(readTimestamp(aChannel));
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(readTimestamp(aChannel));
    }

    private boolean isNullMtdDate(int aYear, int aMonDayHour, long aMinSecMic)
    {
        return (aYear == MTD_DATE_NULL_YEAR)
            && (aMonDayHour == MTD_DATE_NULL_MONDAYHOUR)
            && (aMinSecMic == MTD_DATE_NULL_MINSECMIC);
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        Timestamp sTS = (Timestamp)((ObjectDynamicArray) aArray).get();
        if (sTS == null)
        {
            setNullValue();
        }
        else
        {
            setTime(sTS.getTime(), sTS.getNanos());
        }
    }

    protected void loadFromSub(int aLoadIndex)
    {
        Timestamp sTS = (Timestamp)(mValues.get(aLoadIndex));
        if (sTS == null)
        {
            setNullValue();
        }
        else
        {
            setTime(sTS.getTime(), sTS.getNanos());
        }
    }

    protected void setNullValue()
    {
        mIsNullValueSet = true;
    }
    
    protected boolean isNullValueSet()
    {
        return mIsNullValueSet;
    }

    public long getTimeInMillis()
    {
        return mTimestamp.getTime();
    }

    public long getTimeInMillis(Calendar aCalendar)
    {
        if (aCalendar != null)
        {
            return TimeZoneUtils.convertTimeZone(mTimestamp.getTime(), aCalendar);
        }
        else
        {
            return mTimestamp.getTime();
        }
    }

    public int getNanos()
    {
        return mTimestamp.getNanos();
    }

    protected Date getDateSub(Calendar aCalendar)
    {
        Calendar sCal = Calendar.getInstance();
        sCal.clear();
        sCal.setTimeInMillis(getTimeInMillis(aCalendar));
        int sYY = sCal.get(Calendar.YEAR);
        int sMM = sCal.get(Calendar.MONTH);
        int sDD = sCal.get(Calendar.DAY_OF_MONTH);
        sCal.clear();
        sCal.set(Calendar.YEAR, sYY);
        sCal.set(Calendar.MONTH, sMM);
        sCal.set(Calendar.DAY_OF_MONTH, sDD);
        return new Date(sCal.getTimeInMillis());
    }

    protected Time getTimeSub(Calendar aCalendar)
    {
        Calendar sCal = Calendar.getInstance();
        sCal.clear();
        sCal.setTimeInMillis(getTimeInMillis(aCalendar));
        int sHH = sCal.get(Calendar.HOUR_OF_DAY);
        int sMI = sCal.get(Calendar.MINUTE);
        int sSS = sCal.get(Calendar.SECOND);
        sCal.clear();
        sCal.set(Calendar.HOUR_OF_DAY, sHH);
        sCal.set(Calendar.MINUTE, sMI);
        sCal.set(Calendar.SECOND, sSS);
        // Oracle은 fractional seconds 값을 안준다.
        //sCal.set(Calendar.MILLISECOND, mTimestamp.getNanos() / 1000000);
        return new Time(sCal.getTimeInMillis());
    }

    protected Timestamp getTimestampSub(Calendar aCalendar)
    {
        Timestamp sResult = new Timestamp(getTimeInMillis(aCalendar));
        sResult.setNanos(mTimestamp.getNanos());
        return sResult;
    }

    protected InputStream getAsciiStreamSub() throws SQLException
    {
        return new ByteArrayInputStream(getStringSub().getBytes());        
    }
    
    protected InputStream getBinaryStreamSub() throws SQLException
    {
        return new ByteArrayInputStream(getBytesSub()); 
    }
    
    protected Reader getCharacterStreamSub() throws SQLException
    {
        return new StringReader(getStringSub());
    }

    protected void setTime(long aTimeInMillis, int aNanos)
    {
        mTimestamp.setTime(aTimeInMillis);
        mTimestamp.setNanos(aNanos);
        mIsNullValueSet = false;
    }

    protected void setColumn(CommonDateTimeColumn aValue)
    {
        mCalendar.clear();
        mCalendar.setTimeZone(aValue.mCalendar.getTimeZone());
        mTimestamp.setTime(aValue.getTimeInMillis());
        mTimestamp.setNanos(aValue.getNanos());
        mIsNullValueSet = aValue.mIsNullValueSet;
    }

    public void setLocalCalendar(Calendar aCalendar)
    {
        mCalendar = aCalendar;
    }

    public void setDateFormat(String aDateFormat)
    {
        mDateFormat = aDateFormat;
    }

    public String getDateFormat()
    {
        return mDateFormat;
    }
}
