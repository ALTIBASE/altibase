package Altibase.jdbc.driver.util;

import junit.framework.TestCase;

import java.util.TimeZone;

public class TimeZoneUtilTest extends TestCase
{
    private static final int OFFSET_P0900 = 9 * 60 * 60 * 1000;

    public void testGetTimeZone()
    {
        assertTimeZone("Asia/Seoul", OFFSET_P0900, TimeZoneUtils.getTimeZone("Asia/Seoul"));
        assertTimeZone("Etc/GMT-9", OFFSET_P0900, TimeZoneUtils.getTimeZone("Etc/GMT-9"));
        assertTimeZone("GMT+09:00", OFFSET_P0900, TimeZoneUtils.getTimeZone("GMT+9:00"));
        assertTimeZone("GMT+09:00", OFFSET_P0900, TimeZoneUtils.getTimeZone("GMT+09:00"));
        assertTimeZone("GMT+09:00", OFFSET_P0900, TimeZoneUtils.getTimeZone("+9:00"));
        assertTimeZone("GMT+09:00", OFFSET_P0900, TimeZoneUtils.getTimeZone("+09:00"));
    }

    private void assertTimeZone(String aID, int aOffset, TimeZone aTZ)
    {
        assertEquals(aID, aTZ.getID());
        assertEquals(aOffset, aTZ.getRawOffset());
    }
}
