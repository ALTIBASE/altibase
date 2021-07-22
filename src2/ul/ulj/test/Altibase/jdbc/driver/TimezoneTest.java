package Altibase.jdbc.driver;

import Altibase.jdbc.driver.util.AltibaseProperties;

import java.sql.*;

public class TimezoneTest extends AltibaseTestCase
{
    private static final int    HOUR_UNIT       = 60 * 60 * 1000;
    private static final int    TIME_DIFF_EQUAL = 0;
    private static final int    TIME_DIFF_P0100 = 1 * HOUR_UNIT; // +09:00
    private static final int    TIME_DIFF_N0900 = -9 * HOUR_UNIT; // -09:00
    private static final String DB_TIME_ZONE    = "+09:00";

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 DATE)",
                "INSERT INTO t1 VALUES (sysdate)",
        };
    }

    public void testTimezone() throws SQLException
    {
        assertTimezone(DB_TIME_ZONE  , "DB_TZ"       , TIME_DIFF_EQUAL);
        // BUG-44466 os local time zone은 장비에 따라 틀려질 수 있기때문에 제외
        // assertTimezone("Asia/Seoul"  , "OS_TZ"       , TIME_DIFF_EQUAL);
        assertTimezone("KST"         , "KST"         , TIME_DIFF_EQUAL);
        assertTimezone("+09:00"      , "+09:00"      , TIME_DIFF_EQUAL);
        assertTimezone("UTC"         , "UTC"         , TIME_DIFF_N0900);
        assertTimezone("+00:00"      , "+00:00"      , TIME_DIFF_N0900);
        assertTimezone("Asia/Yakutsk", "Asia/Yakutsk", TIME_DIFF_P0100);
        assertTimezone("+10:00"      , "+10:00"      , TIME_DIFF_P0100);
    }

    private void assertTimezone(String aExpTimeZone, String aInputTimeZone, int aTimeDiff) throws SQLException
    {
        AltibaseProperties sProp = new AltibaseProperties();
        sProp.setTimeZone(aInputTimeZone);
        AltibaseConnection sConn = (AltibaseConnection)DriverManager.getConnection(AltibaseTestCase.CONN_URL, sProp);
        Statement sStmt = sConn.createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT session_timezone(), c1, conv_timezone(c1, db_timezone(), session_timezone()) FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(aExpTimeZone, sRS.getString(1));
        assertEquals(aExpTimeZone, sConn.getSessionTimeZone());
        // SQL DATE 커럼을 얻을 때 session_timezone()으로 자동변환 되진 않는다.
        // DATE 컬럼이 TIMEZONE 값을 갖고있지 않기 때문.
        Timestamp sTS1 = sRS.getTimestamp(2);
        Timestamp sTS2 = sRS.getTimestamp(3);
        long sTimeDiff = sTS2.getTime() - sTS1.getTime();
        assertEquals(aTimeDiff, sTimeDiff);
        sRS.close();
        sStmt.close();
        sConn.close();
    }

    public void testDbTimeZone() throws SQLException
    {
        AltibaseConnection sConn = (AltibaseConnection)DriverManager.getConnection(AltibaseTestCase.CONN_URL);
        assertEquals(DB_TIME_ZONE, sConn.getDbTimeZone());
        assertEquals(DB_TIME_ZONE, sConn.getSessionTimeZone());
        sConn.setSessionTimeZone("+01:00");
        assertEquals("+01:00", sConn.getSessionTimeZone());
        sConn.close();
    }
}
