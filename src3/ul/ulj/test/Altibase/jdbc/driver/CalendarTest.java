package Altibase.jdbc.driver;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Timestamp;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.TimeZone;

public class CalendarTest extends AltibaseTestCase
{
    private static final String     TIMESTAMP_STR               = "1981-10-17 12:34:56.987654";
    private static final String     TIMESTAMP_STR_P0900         = "1981-10-17 21:34:56.987654";
    private static final String     TIMESTAMP_STR_P1800         = "1981-10-18 06:34:56.987654";
    private static final String     TIMESTAMP_STR_N0900         = "1981-10-17 03:34:56.987654";
    private static final String     TIMESTAMP_STR_N1800         = "1981-10-16 18:34:56.987654";
    private static final String     TIMESTAMP_FORM_STR_FOR_SQL  = "YYYY-MM-DD HH:MI:SS.SSSSSS";
    private static final String     TIMESTAMP_FORM_STR_FOR_JAVA = "yyyy-MM-dd HH:mm:ss.SSS";
    private static final DateFormat TIMESTAMP_FORM              = new SimpleDateFormat(TIMESTAMP_FORM_STR_FOR_JAVA);

    private static final Calendar   CALENDAR_P0900              = Calendar.getInstance(TimeZone.getTimeZone("GMT+09:00"));
    private static final Calendar   CALENDAR_GMT                = Calendar.getInstance(TimeZone.getTimeZone("GMT"));
    private static final Calendar   CALENDAR_N0900              = Calendar.getInstance(TimeZone.getTimeZone("GMT-09:00"));

    private static final int        IDX_ID                      = 1;
    private static final int        IDX_DATE                    = 2;

    static
    {
        assertEquals(9 * 60 * 60 * 1000, CALENDAR_P0900.getTimeZone().getRawOffset());
        assertEquals(0, CALENDAR_GMT.getTimeZone().getRawOffset());
        assertEquals(-9 * 60 * 60 * 1000, CALENDAR_N0900.getTimeZone().getRawOffset());
    }

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (id INTEGER, c1 DATE)",
                "INSERT INTO t1 VALUES (1, TO_DATE('" + TIMESTAMP_STR + "', '" + TIMESTAMP_FORM_STR_FOR_SQL + "'))",
                "INSERT INTO t1 VALUES (2, null)",
        };
    }

    public void testGetWithCalendar() throws SQLException
    {
        // session timezone과 상관 없을 것.
        // 왜냐하면 Altibase는 TimeZone을 지원하지 않기 때문이다.
        testGetWithCalendar("+09:00",
                            TIMESTAMP_STR,
                            TIMESTAMP_STR,
                            TIMESTAMP_STR_P0900,
                            TIMESTAMP_STR_P1800);
        testGetWithCalendar("-09:00",
                            TIMESTAMP_STR,
                            TIMESTAMP_STR,
                            TIMESTAMP_STR_P0900,
                            TIMESTAMP_STR_P1800);
    }

    private void testGetWithCalendar(String aSessionTimeZone, String aTS, String aTS2P9, String aTS2UTC, String aTS2N9) throws SQLException
    {
        ((AltibaseConnection)connection()).setSessionTimeZone(aSessionTimeZone);

        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());

        assertEquals(aTS, sRS.getString(IDX_DATE));
        assertEquals(aTS.substring(0, 23), TIMESTAMP_FORM.format(sRS.getTimestamp(IDX_DATE)));

        // to +09:00
        assertEquals(aTS2P9, sRS.getTimestamp(IDX_DATE, CALENDAR_P0900).toString());
        assertEquals(aTS2P9.substring(0, 23), TIMESTAMP_FORM.format(sRS.getTimestamp(IDX_DATE, CALENDAR_P0900)));
        assertEquals(aTS2P9.substring(11, 19), sRS.getTime(IDX_DATE, CALENDAR_P0900).toString());
        // to +00:00
        assertEquals(aTS2UTC, sRS.getTimestamp(IDX_DATE, CALENDAR_GMT).toString());
        assertEquals(aTS2UTC.substring(0, 23), TIMESTAMP_FORM.format(sRS.getTimestamp(IDX_DATE, CALENDAR_GMT)));
        assertEquals(aTS2UTC.substring(11, 19), sRS.getTime(IDX_DATE, CALENDAR_GMT).toString());
        // to -09:00
        assertEquals(aTS2N9, sRS.getTimestamp(IDX_DATE, CALENDAR_N0900).toString());
        assertEquals(aTS2N9.substring(0, 23), TIMESTAMP_FORM.format(sRS.getTimestamp(IDX_DATE, CALENDAR_N0900)));
        assertEquals(aTS2N9.substring(11, 19), sRS.getTime(IDX_DATE, CALENDAR_N0900).toString());

        assertEquals(true, sRS.next());
        assertEquals(null, sRS.getObject(IDX_DATE));

        sRS.close();
        sStmt.close();
    }

    public void testSetWithCalendar() throws SQLException, ParseException
    {
        PreparedStatement sUpdStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?, ?)");
        Timestamp sTS = new Timestamp(TIMESTAMP_FORM.parse(TIMESTAMP_STR.substring(0, 23)).getTime());
        sUpdStmt.setInt(IDX_ID, 101);
        sUpdStmt.setTimestamp(IDX_DATE, sTS);
        assertEquals(1, sUpdStmt.executeUpdate());
        sUpdStmt.setInt(IDX_ID, 102);
        sUpdStmt.setTimestamp(IDX_DATE, sTS, CALENDAR_P0900);
        assertEquals(1, sUpdStmt.executeUpdate());
        sUpdStmt.setInt(IDX_ID, 103);
        sUpdStmt.setTimestamp(IDX_DATE, sTS, CALENDAR_GMT);
        assertEquals(1, sUpdStmt.executeUpdate());
        sUpdStmt.setInt(IDX_ID, 104);
        sUpdStmt.setTimestamp(IDX_DATE, sTS, CALENDAR_N0900);
        assertEquals(1, sUpdStmt.executeUpdate());
        sUpdStmt.close();

        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1 ORDER BY id ASC");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(IDX_ID));
        assertEquals(TIMESTAMP_STR, sRS.getString(IDX_DATE));
        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt(IDX_ID));
        assertEquals(null, sRS.getString(IDX_DATE));
        assertEquals(true, sRS.next());
        assertEquals(101, sRS.getInt(IDX_ID));
        assertEquals(TIMESTAMP_STR.substring(0, 23), sRS.getString(IDX_DATE));
        assertEquals(true, sRS.next());
        assertEquals(102, sRS.getInt(IDX_ID));
        assertEquals(TIMESTAMP_STR.substring(0, 23), sRS.getString(IDX_DATE));
        assertEquals(true, sRS.next());
        assertEquals(103, sRS.getInt(IDX_ID));
        assertEquals(TIMESTAMP_STR_N0900.substring(0, 23), sRS.getString(IDX_DATE));
        assertEquals(true, sRS.next());
        assertEquals(104, sRS.getInt(IDX_ID));
        assertEquals(TIMESTAMP_STR_N1800.substring(0, 23), sRS.getString(IDX_DATE));
        assertEquals(false, sRS.next());
    }
}
