package Altibase.jdbc.driver;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Types;

import static org.junit.Assert.assertArrayEquals;

public class InsertTestNibble extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 NIBBLE(4))"
        };
    }

    public void testInsertByString() throws Exception
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setString(1, "123");
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("123");

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setString(1, "234");
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("234");

        sStmt.setString(1, null);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.close();
    }

    public void testGetBytesAndSetByBINARY() throws SQLException
    {
        testGetBytesAndSetBy(Types.BINARY, new byte[] { 0x12, 0x34 }, "1234");
    }

    public void testGetBytesAndSetByBYTE() throws SQLException
    {
        testGetBytesAndSetBy(AltibaseTypes.BYTE, new byte[] { 0x12, 0x34 }, "1234");
    }

    public void testGetBytesAndSetByNIBBLE4() throws SQLException
    {
        testGetBytesAndSetBy(AltibaseTypes.NIBBLE, new byte[] { 0x12, 0x34 }, "1234");
    }

    public void testGetBytesAndSetByNIBBLE3() throws SQLException
    {
        testGetBytesAndSetBy(AltibaseTypes.NIBBLE, new byte[] { 0x12, 0x30 }, "123");
    }

    public void testGetBytesAndSetBy(int aJDBCType, byte[] aBytesData, String aStrData) throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sInsStmt.setObject(1, aBytesData, aJDBCType);
        assertEquals(1, sInsStmt.executeUpdate());
        sInsStmt.close();

        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(aStrData, sRS.getString(1));
        assertArrayEquals(aBytesData, sRS.getBytes(1));
        assertEquals(false, sRS.next());
        sStmt.close();
    }
}
