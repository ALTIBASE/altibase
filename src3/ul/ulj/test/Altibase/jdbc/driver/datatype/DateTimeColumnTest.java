package Altibase.jdbc.driver.datatype;

import Altibase.jdbc.driver.AltibaseTestCase;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.*;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class DateTimeColumnTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 DATE)"
        };
    }

    private static final String SQL_DATE_FORMAT = "yyyy-MM-dd HH24:mi:ss.FF6";
    private static final String SQL_TO_DATE     = "TO_DATE";
    private static final String SQL_COLUMN      = "TO_CHAR(c1, '" + SQL_DATE_FORMAT + "')";

    public void testInsertDate() throws Exception
    {
        PreparedStatement sStmt;
        Calendar sCalendar = Calendar.getInstance();
        sCalendar.clear();
        sCalendar.set(2007, 4 - 1, 26, 1, 23, 45);
        sCalendar.set(Calendar.MILLISECOND, 123);

        Date sDate1 = new Date(sCalendar.getTimeInMillis());
        Time sTime1 = new Time(sDate1.getTime());
        Timestamp sTimestamp1 = new Timestamp(sCalendar.getTimeInMillis());
        sTimestamp1.setNanos(987654321);

        sCalendar.clear();
        sCalendar.set(2007, 4 - 1, 30, 12, 34, 56);
        sCalendar.set(Calendar.MILLISECOND, 234);
        Date sDate2 = new Date(sCalendar.getTimeInMillis());
        Time sTime2 = new Time(sDate2.getTime());
        Timestamp sTimestamp2 = new Timestamp(sCalendar.getTimeInMillis());
        sTimestamp2.setNanos(876543219);

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (null)");
        assertEquals(1, sStmt.executeUpdate());
        sStmt.close();
        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");

        sStmt.setDate(1, sDate1);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 00:00:00.000000", SQL_COLUMN);
        sStmt.setObject(1, sDate1, Types.DATE);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 00:00:00.000000", SQL_COLUMN);
        sStmt.setObject(1, sDate1, Types.TIME);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.000000", SQL_COLUMN);
        sStmt.setObject(1, sDate1, Types.TIMESTAMP);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 00:00:00.000000", SQL_COLUMN);
        sStmt.setDate(1, sDate2);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-30 00:00:00.000000", SQL_COLUMN);

        sStmt.setTimestamp(1, sTimestamp1);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.987654", SQL_COLUMN);
        sStmt.setObject(1, sTimestamp1, Types.DATE);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.987654", SQL_COLUMN);
        sStmt.setObject(1, sTimestamp1, Types.TIME);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.000000", SQL_COLUMN);
        sStmt.setObject(1, sTimestamp1, Types.TIMESTAMP);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.987654", SQL_COLUMN);
        sStmt.setTimestamp(1, sTimestamp2);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-30 12:34:56.876543", SQL_COLUMN);

        sStmt.setTime(1, sTime1);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.000000", SQL_COLUMN);
        sStmt.setObject(1, sTime1, Types.TIME);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.000000", SQL_COLUMN);
        try
        {
            sStmt.setObject(1, sTime1, Types.DATE);
            assertEquals(1, sStmt.executeUpdate());
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.UNSUPPORTED_TYPE_CONVERSION, sEx.getErrorCode());
        }
        try
        {
            sStmt.setObject(1, sTime1, Types.TIMESTAMP);
            assertEquals(1, sStmt.executeUpdate());
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.UNSUPPORTED_TYPE_CONVERSION, sEx.getErrorCode());
        }
        sStmt.setTime(1, sTime2);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-30 12:34:56.000000", SQL_COLUMN);

        sStmt.setObject(1, new Long(sDate1.getTime()), Types.DATE);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2007-04-26 01:23:45.000000", SQL_COLUMN);

        sStmt.setDate(1, null);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(null);

        try
        {
            sStmt.setObject(1, "xx", Types.DATE);
            fail();
        }
        catch (IllegalArgumentException sEx)
        {
        }

        try
        {
            sStmt.setObject(1, new Integer(1), Types.DATE);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.UNSUPPORTED_TYPE_CONVERSION, sEx.getErrorCode());
        }

        sStmt.close();
    }

    public void testInsertByString() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("INSERT INTO t1 VALUES ("+ SQL_TO_DATE +"(?, 'YYYY-MM-DD'))");
        sStmt.setString(1, "2001-10-21");
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("2001-10-21 00:00:00.000000", SQL_COLUMN);
        sStmt.close();

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        // as DATE
        String sTS = "2001-12-23";
        sStmt.setObject(1, sTS, Types.DATE);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(sTS + " 00:00:00.000000", SQL_COLUMN);
        // as TIMESTAMP
        sTS = "2001-11-30 22:13:24.241623";
        sStmt.setObject(1, sTS, Types.TIMESTAMP);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(sTS, SQL_COLUMN);
        // as TIME
        sTS = "13:41:52";
        sStmt.setObject(1, sTS, Types.TIME);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("1970-01-01 " + sTS + ".000000", SQL_COLUMN);
        sStmt.close();
    }

    private static final DateFormat TIMESTAMP_FORM = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");

    public void testGetXXX() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sInsStmt.setObject(1, "1234-01-23 12:23:34.567123", Types.TIMESTAMP);
        assertEquals(1, sInsStmt.executeUpdate());
        sInsStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals("1234-01-23 00:00:00.000", TIMESTAMP_FORM.format(sRS.getDate(1)));
        assertEquals("1970-01-01 12:23:34.000", TIMESTAMP_FORM.format(sRS.getTime(1)));
        Timestamp sTS = sRS.getTimestamp(1);
        assertEquals("1234-01-23 12:23:34.567", TIMESTAMP_FORM.format(sTS));
        assertEquals(567123000, sTS.getNanos());
        assertEquals(false, sRS.next());
        sSelStmt.close();
    }

    private static final String OLDER_TIME_STRING = "1900-12-31 23:59:59.999";

    public void testInsertOlderTime() throws SQLException
    {
        Timestamp sExpTS = Timestamp.valueOf(OLDER_TIME_STRING);

        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sInsStmt.setTimestamp(1, sExpTS);
        assertEquals(1, sInsStmt.executeUpdate());
        sInsStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        Timestamp sActTS = sRS.getTimestamp(1);
        assertEquals(OLDER_TIME_STRING, TIMESTAMP_FORM.format(sActTS));
        assertEquals(sExpTS, sActTS);
        assertEquals(false, sRS.next());
        sSelStmt.close();
    }
}
