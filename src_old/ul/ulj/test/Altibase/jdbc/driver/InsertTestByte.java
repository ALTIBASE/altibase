package Altibase.jdbc.driver;

import Altibase.jdbc.driver.util.ByteUtils;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import static org.junit.Assert.assertArrayEquals;

public class InsertTestByte extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 BYTE(3), c2 BYTE(32000))"
        };
    }

    public void _NOTSUPPORT_testInsertByString() throws Exception
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?, NULL)");
        sStmt.setString(1, "1234");
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("1234");

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setString(1, "2345");
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("2345");

        sStmt.setString(1, null);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.close();
    }

    public void testInsertByByteArray() throws SQLException
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?, NULL)");
        sStmt.setBytes(1, new byte[] { 0x56, 0x78 });
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("567800");

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setBytes(1, new byte[] { 0x1a, 0x2b });
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("1a2b00");

        sStmt.setBytes(1, null);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt = connection().prepareStatement("UPDATE t1 SET c2 = ?");
        byte[] sByteArray = new byte[32000];
        for (int i=0; i<sByteArray.length; i++)
        {
            sByteArray[i] = (byte)i;
        }
        sStmt.setBytes(1, sByteArray);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(ByteUtils.toHexString(sByteArray), "c2");
    }

    public void testPrecisionInit() throws SQLException
    {
        Statement sInsStmt = connection().createStatement();
        assertEquals(1, sInsStmt.executeUpdate("INSERT INTO t1 VALUES (null, null)"));
        sInsStmt.close();

        PreparedStatement sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setObject(1, new byte[] { 0x56, 0x78 });
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("567800");

        // ColumnInfo를 초기화하기 위해서 다른 타입으로 한번 set 한다.
        // 지원하지 않는 타입이므로 당연히 에러가 떨어진다.
        sStmt.setObject(1, new java.sql.Date(1));
        try
        {
            sStmt.executeUpdate();
            fail();
        }
        catch (SQLException sEx)
        {
        }

        // BYTE 타입용 ColumnInfo를 다시 만드므로 Precision이 0으로 초기화되어있다.
        sStmt.setObject(1, new byte[] { 0x1a, 0x2b });
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("1a2b00");

        sStmt.setObject(1, null);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(null);

        sStmt.close();
    }

    private static final byte[] DAT_32_VAL1 = new byte[]{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f};
    private static final byte[] DAT_32_VAL2 = new byte[]{0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

    public void testUserDataConsistency() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (c2) VALUES (?)");
        sInsStmt.setBytes(1, DAT_32_VAL1);
        assertEquals(false, sInsStmt.execute());
        sInsStmt.setBytes(1, DAT_32_VAL2);
        assertEquals(false, sInsStmt.execute());

        // 사용자가 넘긴 데이타(DAT_32_VAL?)는 건드리지 않아야 한다.
        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT c2 FROM t1");
        assertEquals(true, sRS.next());
        assertArrayEquals(fillZero(DAT_32_VAL1), sRS.getBytes(1));
        assertEquals(true, sRS.next());
        assertArrayEquals(fillZero(DAT_32_VAL2), sRS.getBytes(1));
        assertEquals(false, sRS.next());
        sSelStmt.close();
    }

    private byte[] fillZero(byte[] aDAT_32_VAL)
    {
        byte[] aResult = new byte[32000];
        System.arraycopy(aDAT_32_VAL, 0, aResult, 0, aDAT_32_VAL.length);

        return aResult;
    }
}
