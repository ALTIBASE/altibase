package Altibase.jdbc.driver;

import Altibase.jdbc.driver.util.ByteUtils;
import Altibase.jdbc.driver.util.CharsetUtils;

import java.math.BigDecimal;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CharsetEncoder;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import static org.junit.Assert.assertArrayEquals;

public class MaxFieldSizeTest extends AltibaseTestCase
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
        };
    }

    private static final String[] VALUES_CHAR    = { null, "abcdefghi", "bcdefghi ", "cdefghi  ", "defghi   ", "efghi    ", "fghi     ", "ghi      ", "hi       ", "i        " };
    private static final String[] VALUES_VARCHAR = { null, "abcdefghi", "bcdefghi", "cdefghi", "defghi", "efghi", "fghi", "ghi", "hi", "i" };

    public void testMaxFieldSizeForChar() throws SQLException
    {
        testMaxFieldSizeForCharVarchar("CHAR");
    }

    public void testMaxFieldSizeForVarchar() throws SQLException
    {
        testMaxFieldSizeForCharVarchar("VARCHAR");
    }

    private void testMaxFieldSizeForCharVarchar(String aType) throws SQLException
    {
        Statement sInitStmt = connection().createStatement();
        sInitStmt.execute("CREATE TABLE t1 (id INTEGER, val " + aType + "(9))");
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (1, 'abcdefghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (2, 'bcdefghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (3, 'cdefghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (4, 'defghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (5, 'efghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (6, 'fghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (7, 'ghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (8, 'hi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (9, 'i')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (10, null)"));
        sInitStmt.close();

        String[] sExpValues = aType.equals("CHAR") ? VALUES_CHAR : VALUES_VARCHAR;
        PreparedStatement sStmt = connection().prepareStatement("SELECT val FROM t1 ORDER BY id");
        sStmt.setFetchSize(1);
        for (int sMaxFieldSize = 0; sMaxFieldSize <= 10; sMaxFieldSize++)
        {
            sStmt.setMaxFieldSize(sMaxFieldSize);
            ResultSet sRS = sStmt.executeQuery();
            for (int i = 1; i <= 9; i++)
            {
                assertEquals(true, sRS.next());
                String sExpValue = sExpValues[i];
                if (sMaxFieldSize > 0 && sMaxFieldSize < sExpValues[i].length())
                {
                    sExpValue = sExpValue.substring(0, sMaxFieldSize);
                }
                assertEquals(sExpValue, sRS.getString(1));
            }
            assertEquals(true, sRS.next());
            assertEquals(null, sRS.getString(1)); // null은 항상 null
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }

    private static final String[] VALUES_NCHAR    = { null, "abcdefghi", "bcdefghi ", "cdefghi  ", "defghi   ", "efghi    ", "fghi     ", "ghi      ", "hi       ", "i        " };
    private static final String[] VALUES_NVARCHAR = { null, "abcdefghi", "bcdefghi", "cdefghi", "defghi", "efghi", "fghi", "ghi", "hi", "i" };

    public void testMaxFieldSizeForNChar() throws SQLException, CharacterCodingException
    {
        testMaxFieldSizeForNCharNVarchar("NCHAR");
    }

    public void testMaxFieldSizeForNVarchar() throws SQLException, CharacterCodingException
    {
        testMaxFieldSizeForNCharNVarchar("NVARCHAR");
    }

    private CharsetEncoder NCHAR_ENCODER = CharsetUtils.newUTF16Encoder();

    private void testMaxFieldSizeForNCharNVarchar(String aType) throws SQLException, CharacterCodingException
    {
        Statement sInitStmt = connection().createStatement();
        sInitStmt.execute("CREATE TABLE t1 (id INTEGER, val " + aType + "(9))");
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (1, 'abcdefghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (2, 'bcdefghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (3, 'cdefghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (4, 'defghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (5, 'efghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (6, 'fghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (7, 'ghi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (8, 'hi')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (9, 'i')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (10, null)"));
        sInitStmt.close();

        String[] sExpValues = aType.equals("NCHAR") ? VALUES_NCHAR : VALUES_NVARCHAR;
        PreparedStatement sStmt = connection().prepareStatement("SELECT val FROM t1 ORDER BY id");
        sStmt.setFetchSize(1);
        for (int sMaxFieldSize = 0; sMaxFieldSize <= 10; sMaxFieldSize++)
        {
            sStmt.setMaxFieldSize(sMaxFieldSize);
            ResultSet sRS = sStmt.executeQuery();
            for (int i = 1; i <= 9; i++)
            {
                assertEquals(true, sRS.next());
                ByteBuffer sExpBuf = NCHAR_ENCODER.encode(CharBuffer.wrap(sExpValues[i]));
                byte[] sExpValue = new byte[(sMaxFieldSize > 0) ? Math.min(sMaxFieldSize, sExpBuf.remaining()) : sExpBuf.remaining()];
                sExpBuf.get(sExpValue);
                byte[] sActValue = sRS.getBytes(1);
                assertArrayEquals(i + "th value of MaxFieldSize " + sMaxFieldSize, sExpValue, sActValue);
            }
            assertEquals(true, sRS.next());
            assertEquals(null, sRS.getString(1)); // null은 항상 null
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }

    private static final byte[][] VALUES_BYTE = {
        null,
        {1,2,3,4,5},
        {2,3,4,5,0},
        {3,4,5,0,0},
        {4,5,0,0,0},
        {5,0,0,0,0},
    };

    public void testMaxFieldSizeForBYTE() throws SQLException
    {
        Statement sInitStmt = connection().createStatement();
        sInitStmt.execute("CREATE TABLE t1 (c1 BYTE(5))");
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BYTE'0102030405')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BYTE'02030405')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BYTE'030405')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BYTE'0405')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BYTE'05')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (null)"));
        sInitStmt.close();

        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        for (int sMaxFieldSize = 0; sMaxFieldSize <= 10; sMaxFieldSize++)
        {
            sStmt.setMaxFieldSize(sMaxFieldSize);
            ResultSet sRS = sStmt.executeQuery();
            for (int i = 1; i <= 5; i++)
            {
                assertEquals(true, sRS.next());
                byte[] sBytes = sRS.getBytes(1);
                assertTrue(sMaxFieldSize == 0 || sBytes.length <= sMaxFieldSize);
                for (int j = 0; j < sBytes.length; j++)
                {
                    assertEquals(VALUES_BYTE[i][j], sBytes[j]);
                }
                assertEquals(ByteUtils.toHexString(VALUES_BYTE[i], 0, sBytes.length), sRS.getString(1));
            }
            assertEquals(true, sRS.next());
            assertEquals(null, sRS.getBytes(1)); // null은 항상 null
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }

    public void testMaxFieldSizeForNIBBLE() throws SQLException
    {
        Statement sInitStmt = connection().createStatement();
        sInitStmt.execute("CREATE TABLE t1 (c1 NIBBLE(5))");
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (NIBBLE'12345')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (NIBBLE'2345')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (NIBBLE'345')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (NIBBLE'45')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (NIBBLE'5')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (null)"));
        sInitStmt.close();

        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        for (int sMaxFieldSize = 0; sMaxFieldSize <= 10; sMaxFieldSize++)
        {
            sStmt.setMaxFieldSize(sMaxFieldSize);
            ResultSet sRS = sStmt.executeQuery();
            for (int i = 1; i <= 5; i++)
            {
                assertEquals(true, sRS.next());
                byte[] sBytes = sRS.getBytes(1);
                String sStr = sRS.getString(1);
                if (sMaxFieldSize > 0)
                {
                    assertTrue(i + ": bytes.length expected <= " + sMaxFieldSize + ", but " + sBytes.length + " (" + ByteUtils.toHexString(sBytes) + ")", sBytes.length <= sMaxFieldSize);
                    assertTrue(i + ": string.length expected <= " + (sMaxFieldSize * 2) + ", but " + sStr.length() + " (" + sStr + ")", sStr.length() <= sMaxFieldSize * 2);
                }
                for (int j = 0; j < sStr.length(); j++)
                {
                    int sExpVal = i + j;
                    byte sB = sBytes[j / 2];
                    int sNB = (((j + 1) % 2) == 1) ? (sB >>> 4) : (sB & 0xF);
                    int sCh = sStr.charAt(j) - '0';
                    assertEquals(i + ": " + j + "th nibble expected " + sExpVal + ", but " + sNB, i + j, sNB);
                    assertEquals(i + ": " + j + "th char expected " + sExpVal + ", but " + sCh, i + j, sCh);
                }
            }
            assertEquals(true, sRS.next());
            assertEquals(null, sRS.getBytes(1)); // null은 항상 null
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }

    private static final String[] VALUES_BIT = {null, "01101", "10110", "10100", "01000", "10000"};
    private static final String[] VALUES_VARBIT = {null, "01101", "1011", "101", "01", "1"};

    public void testMaxFieldSizeForBit() throws SQLException
    {
        testMaxFieldSizeForBitVarbit("BIT");
    }

    public void testMaxFieldSizeForVarbit() throws SQLException
    {
        testMaxFieldSizeForBitVarbit("VARBIT");
    }

    private void testMaxFieldSizeForBitVarbit(String aType) throws SQLException
    {
        Statement sInitStmt = connection().createStatement();
        sInitStmt.execute("CREATE TABLE t1 (c1 " + aType + "(5))");
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BIT'01101')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BIT'1011')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BIT'101')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BIT'01')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (BIT'1')"));
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (null)"));
        sInitStmt.close();

        String[] sExpValues = aType.equals("BIT") ? VALUES_BIT : VALUES_VARBIT;
        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        for (int sMaxFieldSize = 0; sMaxFieldSize <= 10; sMaxFieldSize++)
        {
            sStmt.setMaxFieldSize(sMaxFieldSize);
            ResultSet sRS = sStmt.executeQuery();
            for (int i = 1; i <= 5; i++)
            {
                assertEquals(true, sRS.next());
                String sExpValue = sExpValues[i];
                if (sMaxFieldSize > 0 && sMaxFieldSize < sExpValue.length())
                {
                    sExpValue = sExpValue.substring(0, sMaxFieldSize);
                }
                assertEquals(sExpValue, sRS.getString(1));
            }
            assertEquals(true, sRS.next());
            assertEquals(null, sRS.getObject(1)); // null은 항상 null
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }

    private static final Number[] VALUES_INT    = { new Integer(123456789), new Integer(234567891), new Integer(345678912), new Integer(456789123), new Integer(567891234) };
    private static final Number[] VALUES_SHORT  = { new Short((short)123456789), new Short((short)234567891), new Short((short)345678912), new Short((short)456789123), new Short((short)567891234) };
    private static final Number[] VALUES_LONG   = { new Long(12345), new Long(23456), new Long(345678), new Long(45678), new Long(56789) };
    private static final Number[] VALUES_FLOAT  = { new Float(1.23456), new Float(2.345678), new Float(3.45), new Float(4.51235), new Float(5.687719) };
    private static final Number[] VALUES_DOUBLE = { new Double(1.2308972), new Double(2.312614), new Double(3.462355), new Double(4.531526), new Double(5.6198727) };
    private static final Number[] VALUES_NUMBER = { new BigDecimal(123456789), new BigDecimal(234567891), new BigDecimal(345678912), new BigDecimal(456789123), new BigDecimal(567891234) };

    public void testMaxFieldSizeForInt() throws SQLException
    {
        testMaxFieldSizeForNumber("INTEGER", VALUES_INT);
    }
    public void testMaxFieldSizeForBigInt() throws SQLException
    {
        testMaxFieldSizeForNumber("BIGINT", VALUES_LONG);
    }
    public void testMaxFieldSizeForSmallInt() throws SQLException
    {
        testMaxFieldSizeForNumber("SMALLINT", VALUES_SHORT);
    }
    public void testMaxFieldSizeForReal() throws SQLException
    {
        testMaxFieldSizeForNumber("REAL", VALUES_FLOAT);
    }
    public void testMaxFieldSizeForDouble() throws SQLException
    {
        testMaxFieldSizeForNumber("DOUBLE", VALUES_DOUBLE);
    }
    public void testMaxFieldSizeForNumber() throws SQLException
    {
        testMaxFieldSizeForNumber("NUMBER", VALUES_NUMBER);
    }
    public void testMaxFieldSizeForFloat() throws SQLException
    {
        testMaxFieldSizeForNumber("FLOAT", VALUES_NUMBER);
    }
    public void testMaxFieldSizeForDecimal() throws SQLException
    {
        testMaxFieldSizeForNumber("DECIMAL", VALUES_NUMBER);
    }

    private void testMaxFieldSizeForNumber(String aType, Object[] aValues) throws SQLException
    {
        Statement sInitStmt = connection().createStatement();
        sInitStmt.execute("CREATE TABLE t1 (c1 " + aType + ")");
        for (int i = 0; i < aValues.length; i++)
        {
            assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (" + aValues[i] + ")"));
        }
        assertEquals(1, sInitStmt.executeUpdate("INSERT INTO t1 VALUES (null)"));
        sInitStmt.close();

        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        for (int sMaxFieldSize = 0; sMaxFieldSize <= 10; sMaxFieldSize++)
        {
            sStmt.setMaxFieldSize(sMaxFieldSize);
            ResultSet sRS = sStmt.executeQuery();
            for (int i = 1; i <= 5; i++)
            {
                assertEquals(true, sRS.next());
                assertEquals(aValues[i - 1], sRS.getObject(1));
                assertEquals(aValues[i - 1].toString(), sRS.getString(1));
            }
            assertEquals(true, sRS.next());
            assertEquals(null, sRS.getObject(1)); // null은 항상 null
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }
}
