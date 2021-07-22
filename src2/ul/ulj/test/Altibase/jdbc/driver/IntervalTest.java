package Altibase.jdbc.driver;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import static org.junit.Assert.assertArrayEquals;

public class IntervalTest extends AltibaseTestCase
{
    public void testConvertString()
    {
        AltibaseInterval sInterval = new AltibaseInterval(0, 1);
        String sStrVal = sInterval.toString();
        assertEquals("0 0:0:0.1", sStrVal);
        assertEquals(sInterval, AltibaseInterval.valueOf(sStrVal));
        byte[] sBytesVal = sInterval.toBytes();
        assertArrayEquals(new byte[]{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, sBytesVal);
        assertArrayEquals(sBytesVal, AltibaseInterval.toBytes(0, 1));
        assertEquals(sInterval, AltibaseInterval.valueOf(sBytesVal));
        double sDoubleVal = sInterval.toNumberOfDays();
        assertEquals(1.0 / (60L * 60L * 24L * 1000000000L), sDoubleVal, 0);
        assertEquals(sDoubleVal, AltibaseInterval.toNumberOfDays(0, 1), 0);
    }

    private static final String SQL_DATE_FORMAT = "yyyy-MM-dd HH24:mi:ss.FF3";
    private static final String SQL_TO_DATE     = "TO_DATE";

    public void testGetInterval() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT " + SQL_TO_DATE + "('1999-12-31 12:59:59.002','" + SQL_DATE_FORMAT + "') - " + SQL_TO_DATE + "('1999-12-31 12:59:59.001','" + SQL_DATE_FORMAT + "') FROM dual");
        assertEquals(true, sRS.next());
        assertEquals("0 0:0:0.1000000", sRS.getString(1));
        assertNotNull(sRS.getBytes(1));
        AltibaseInterval sInterval = (AltibaseInterval)sRS.getObject(1);
        assertEquals(0, sInterval.getSecond());
        assertEquals(1000000, sInterval.getNanos());
        // 하위 호환성을 위해 지원하는 getXXX
        assertEquals(1.0 / (60L * 60L * 24L * 1000L), sRS.getDouble(1), 0);
        assertEquals(0, sRS.getLong(1));
        sRS.close();
        sStmt.close();
    }
}
