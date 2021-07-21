/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.jdbc;

import java.io.UnsupportedEncodingException;
import java.sql.*;
import java.util.Calendar;

import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.util.*;

/**
 * JDBC data manager for JDBC interface
 */
public class JdbcDataManager
{
    private static TraceLogger mTraceLogger = TraceLogger.getInstance();
    
    private static int    mDBCharSet     = DBCharSet.None;
    private static String mDBCharSetName = "";
    
    /**
     * Get database character set value of ALTIBASE server
     * 
     * @return  Database character set value
     */
    public static int getDBCharSet()
    {
        return mDBCharSet;
    }

    /**
     * Get database character set name on JAVA
     * 
     * @return  Database character set name
     */
    public static String getDBCharSetName()
    {
        return mDBCharSetName;
    }

    /**
     * Set database character set value of ALTIBASE server
     * 
     * @param aDBCharSet        Database character set value
     */
    public static void setDBCharSet(int aDBCharSet)
    {
        mDBCharSet     = aDBCharSet;
        mDBCharSetName = DBCharSet.getDBCharSetName(aDBCharSet);
    }
   
    public static int convertColumnPrecision( int aPrecision )
    {
        int sScale = DBCharSet.getCharSetScale( mDBCharSet );
        return aPrecision * sScale;
    } 

    /**
     * Convert SQL type on JDBC to SQL type on ADLP protocol
     * 
     * @param aJdbcSqlType      SQL type on JDBC
     * @return                  SQL type on ADLP protocol
     */
    public static int toSQLType(int aJdbcSqlType)
    {
        int sSQLType = AltibaseSQLType.SQL_NONE;

        switch (aJdbcSqlType)
        {
        case java.sql.Types.CHAR:      sSQLType = AltibaseSQLType.SQL_CHAR;      break;
        case java.sql.Types.VARCHAR:   sSQLType = AltibaseSQLType.SQL_VARCHAR;   break;
        case java.sql.Types.SMALLINT:  sSQLType = AltibaseSQLType.SQL_SMALLINT;  break;
        case java.sql.Types.TINYINT:   sSQLType = AltibaseSQLType.SQL_TINYINT;   break;
        case java.sql.Types.DECIMAL:   sSQLType = AltibaseSQLType.SQL_DECIMAL;   break;
        case java.sql.Types.NUMERIC:   sSQLType = AltibaseSQLType.SQL_NUMERIC;   break;
        case java.sql.Types.FLOAT:     sSQLType = AltibaseSQLType.SQL_FLOAT;     break;
        case java.sql.Types.INTEGER:   sSQLType = AltibaseSQLType.SQL_INTEGER;   break;
        case java.sql.Types.REAL:      sSQLType = AltibaseSQLType.SQL_REAL;      break;
        case java.sql.Types.DOUBLE:    sSQLType = AltibaseSQLType.SQL_DOUBLE;    break;
        case java.sql.Types.BIGINT:    sSQLType = AltibaseSQLType.SQL_BIGINT;    break;
        case java.sql.Types.DATE:      sSQLType = AltibaseSQLType.SQL_DATE;      break;
        case java.sql.Types.TIME:      sSQLType = AltibaseSQLType.SQL_TIME;      break;
        case java.sql.Types.TIMESTAMP: sSQLType = AltibaseSQLType.SQL_TIMESTAMP; break;
        case java.sql.Types.BINARY:    sSQLType = AltibaseSQLType.SQL_BINARY;    break;
        case java.sql.Types.BLOB:      sSQLType = AltibaseSQLType.SQL_BLOB;      break;
        case java.sql.Types.CLOB:      sSQLType = AltibaseSQLType.SQL_CLOB;      break;
        default:                       sSQLType = AltibaseSQLType.SQL_NONE;      break;
        }

        return sSQLType;
    }
    
    /**
     * Convert nullable type on JDBC to nullable type on ADLP protocol
     *  
     * @param aJdbcNullable     Nullable type on JDBC
     * @return                  Nullable type on ADLP protocol
     */
    public static int toNullable(int aJdbcNullable)
    {
        int sNullable = Nullable.Unknown;
        
        switch (aJdbcNullable)
        {
        case ResultSetMetaData.columnNoNulls:
            sNullable = Nullable.NoNull;
            break;
            
        case ResultSetMetaData.columnNullable:
            sNullable = Nullable.Nullable;
            break;
            
        case ResultSetMetaData.columnNullableUnknown:
            sNullable = Nullable.Unknown;
            break;
            
        default:
            sNullable = Nullable.Unknown;
            break;
        }
        
        return sNullable;
    }
    
    /**
     * Date type for transmitting to ALTIBASE server
     */
    public static class mtdDateType
    {
        public short mYear       = 0;
        public short mMonDayHour = 0;
        public int   mMinSecMic  = 0;
    }
    
    /**
     * Convert java.util.Date object to mtdDateType object
     * 
     * @param aDate     java.util.Date object
     * @return          mtdDateType object
     */
    public static mtdDateType toMtdDateType(java.sql.Timestamp aTimestamp)
    {
        Calendar sCalendar = Calendar.getInstance();
        
        sCalendar.setTime(aTimestamp);

        int sYear        = sCalendar.get(Calendar.YEAR);
        int sMonth       = sCalendar.get(Calendar.MONTH) + 1; // calendar object is 0-based month.
        int sDay         = sCalendar.get(Calendar.DATE);
        int sHour        = sCalendar.get(Calendar.HOUR_OF_DAY);
        int sMinute      = sCalendar.get(Calendar.MINUTE);
        int sSecond      = sCalendar.get(Calendar.SECOND);
        int sMicrosecond = aTimestamp.getNanos() / 1000;
        
        if (sYear < 0)
        {
            // ALTIBASE is only supported A.C.(Anno Domini)
            // and is not supported B.C.(Before Christ).
            sYear = 0;
        }

        short sMonDayHour = JdbcDataManager.toMonDayHour(sMonth, sDay, sHour);
        int   sMinSecMic  = JdbcDataManager.toMinSecMic(sMinute, sSecond, sMicrosecond);

        mtdDateType sMtdDateType = new mtdDateType();
        sMtdDateType.mYear       = (short)sYear;
        sMtdDateType.mMonDayHour = sMonDayHour;
        sMtdDateType.mMinSecMic  = sMinSecMic;

        return sMtdDateType;
    }
    
    /**
     * Convert month, day and hour to short data type value for mtdDateType
     * 
     * @param aMonth    Month
     * @param aDay      Day
     * @param aHour     Hour
     * @return          short data type value for mtdDateType
     */
    public static short toMonDayHour(int aMonth, int aDay, int aHour)
    {
        short sMonDayHour = (short)((aMonth << 10) | (aDay << 5) | (0x1f & aHour));
        
        return sMonDayHour;
    }
    
    /**
     * Convert minute, second and microsecond to int data type value for mtdDateType
     * 
     * @param aMinute           Minute
     * @param aSecond           Second
     * @param aMicrosecond      Microsecond
     * @return                  int data type value for mtdDateType
     */
    public static int toMinSecMic(int aMinute, int aSecond, int aMicrosecond)
    {
        int sMinSecMic = (aMinute << 26) | (aSecond << 20) | (0xfffff & aMicrosecond);
        
        return sMinSecMic;
    }
    
    /**
     * Convert JAVA String object to byte array encoded database character set
     * 
     * @param aString   JAVA String object
     * @return          Byte array encoded database character set
     */
    public static byte[] toDBCharSetString(String aString)
    {
        if (aString == null)
        {
            return null;
        }

        byte[] sDBCharSetString = null;
        
        try
        {
            sDBCharSetString = aString.getBytes(mDBCharSetName);
        }
        catch (UnsupportedEncodingException e)
        {
            mTraceLogger.log(e);
        }
        
        return sDBCharSetString;
    }
    
    /**
     * Convert byte array encoded database character set to JAVA String ojbect
     * 
     * @param aDBCharSetString  Byte array encoded database character set
     * @return                  JAVA String object
     */
    public static String toString(byte[] aDBCharSetString)
    {
        if (aDBCharSetString == null)
        {
            return null;
        }

        String sString = null;
        
        try
        {
            sString = new String(aDBCharSetString, mDBCharSetName);
        }
        catch (UnsupportedEncodingException e)
        {
            mTraceLogger.log(e);
        }
        
        return sString;
    }
}
