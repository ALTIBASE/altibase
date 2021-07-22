package Altibase.jdbc.driver.datatype;

import Altibase.jdbc.driver.AltibaseTestCase;
import Altibase.jdbc.driver.util.CharsetUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.sql.*;

import static org.junit.Assert.assertArrayEquals;

public class NVarcharColumnTest extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 VARCHAR(32000), c2 NVARCHAR(16000), c3 INTEGER)",
        };
    }

    protected String getURL()
    {
        return super.getURL() + "?ncharliteralreplace=true";
    }

    private static String readText() throws IOException
    {
        File sFile = new File("test/resources/text.euckr");
        String sText = "";
        BufferedReader sInput = new BufferedReader(new FileReader(sFile));
        String sLine;
        while ((sLine = sInput.readLine()) != null)
        {
            sText = sText.concat(sLine);
        }
        return sText;
    }

    public void testInsertNLiteral() throws IOException, SQLException
    {
        String sText = readText();

        Statement sStmt = connection().createStatement();
        String sInsSql = "INSERT INTO t1 VALUES ('" + sText + "', N'" + sText + "', 3)";
        assertEquals(1, sStmt.executeUpdate(sInsSql));

        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1 WHERE c2 LIKE N'%미네르바%'");
        assertEquals(true, sRS.next());
        assertEquals(sText, sRS.getString(1));
        assertEquals(sText, sRS.getString(2));
        assertEquals(3, sRS.getInt(3));
        sRS.close();
        sStmt.close();
    }

    public void testInsertNVarchar() throws IOException, SQLException
    {
        String sText = readText();

        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?, ?, 3)");
        sInsStmt.setString(1, sText);
        sInsStmt.setString(2, sText);
        assertEquals(1, sInsStmt.executeUpdate());
        sInsStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1 WHERE c2 LIKE N'%미네르바%'");
        assertEquals(true, sRS.next());
        assertEquals(sText, sRS.getString(1));
        assertEquals(sText, sRS.getString(2));
        assertEquals(3, sRS.getInt(3));
        sRS.close();
        sSelStmt.close();
    }

    public void testGetBytes() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate("INSERT INTO t1 VALUES ('123', '234', 345)"));

        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals("123", sRS.getString(1));
        assertArrayEquals(new byte[] { 0x31, 0x32, 0x33 }, sRS.getBytes(1)); // DB CHARSET
        assertEquals("234", sRS.getString(2));
        assertArrayEquals(new byte[] { 0, 0x32, 0, 0x33, 0, 0x34 }, sRS.getBytes(2)); // NCHAR CHARSET
        assertEquals(false, sRS.next());
        sRS.close();

        sStmt.close();
    }

    public void testIllegalStateException() throws CharacterCodingException
    {
        // BUG-45156 IBM jdk인 경우 encode(CharsetBuffer)를 호출한 다음 다시 encode를 호출하면 IllegalStateException이 발생한다.
        CharsetUtils.newUTF16Encoder().encode(CharBuffer.wrap("test1"));
        try
        {
            CharsetUtils.newUTF16Encoder().encode(CharBuffer.wrap("test1"), ByteBuffer.allocate(16000), true);
        }
        catch (IllegalStateException sEx)
        {
            fail(sEx.getMessage());
        }
    }
}
