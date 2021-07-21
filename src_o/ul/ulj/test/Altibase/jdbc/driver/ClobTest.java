package Altibase.jdbc.driver;

import java.io.*;
import java.sql.*;

import static org.junit.Assert.assertArrayEquals;

public class ClobTest extends AltibaseTestCase
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
                "CREATE TABLE t1 (id INTEGER, val1 CLOB, val2 CLOB, val3 CLOB)",
                "INSERT INTO t1 (id, val1) VALUES (1, '" + CLOB_VAL1 + "')",
        };
    }

    private static final String CLOB_VAL1 = "test-value";
    private static final String CLOB_VAL2 = "test-clob-value-by-string";

    protected String getURL()
    {
        return super.getURL() + "?AUTO_COMMIT=0";
    }

    public void testClobToString() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1 WHERE id = 1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals(CLOB_VAL1, sRS.getString(2));
        assertEquals(CLOB_VAL1, sRS.getObject(2).toString());
        sStmt.close();
    }

    public void testSubString() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1 WHERE id = 1");
        assertEquals(true, sRS.next());
        Clob sClob = sRS.getClob(2);
        assertEquals(CLOB_VAL1.substring(2, 2+4), sClob.getSubString(3, 4));
        String sSubStr = sClob.getSubString(2147483650L, 1);
        assertEquals(0, sSubStr.length());
        assertEquals(false, sRS.next());
        sStmt.close();
    }

    public void testBatch() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?, ?, ?, ?)");
        for (int i = 0; i < 10; i++)
        {
            String sLobVal1 = "LOB-COLUMN-2-" + i;
            String sLobVal2 = "LOB-COLUMN-3-" + i;
            String sLobVal3 = "LOB-COLUMN-4-" + i;
            sInsStmt.setInt(1, 10 + i);
            sInsStmt.setCharacterStream(2, new StringReader(sLobVal1), sLobVal1.length());
            sInsStmt.setCharacterStream(3, new StringReader(sLobVal2), sLobVal2.length());
            sInsStmt.setCharacterStream(4, new StringReader(sLobVal3), sLobVal3.length());
            sInsStmt.addBatch();
        }
        int[] sBatchResult = sInsStmt.executeBatch();
        assertArrayEquals(new int[] { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, sBatchResult);
        sInsStmt.close();

        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1 WHERE id >= 10");
        for (int i = 0; i < 10; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(10 + i, sRS.getInt(1));
            assertEquals("LOB-COLUMN-2-" + i, sRS.getString(2));
            assertEquals("LOB-COLUMN-3-" + i, sRS.getString(3));
            assertEquals("LOB-COLUMN-4-" + i, sRS.getString(4));
        }
        assertEquals(false, sRS.next());
        sStmt.close();
    }

    public void testSetStringByCHAR() throws SQLException
    {
        testSetStringBy(Types.CHAR);
    }

    public void testSetStringByVARCHAR() throws SQLException
    {
        testSetStringBy(Types.VARCHAR);
    }

    public void testSetStringByLONGVARCHAR() throws SQLException
    {
        testSetStringBy(Types.LONGVARCHAR);
    }

    public void testSetStringByCLOB() throws SQLException
    {
        testSetStringBy(Types.CLOB);
    }

    private void testSetStringBy(int aJDBCType) throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) VALUES (?, ?)");
        sInsStmt.setInt(1, 11);
        sInsStmt.setObject(2, CLOB_VAL2, aJDBCType);
        assertEquals(false, sInsStmt.execute());
        sInsStmt.close();

        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1 WHERE id = 11");
        assertEquals(true, sRS.next());
        assertEquals(11, sRS.getInt(1));
        assertEquals(CLOB_VAL2, sRS.getString(2));
        assertEquals(false, sRS.next());
        sStmt.close();
    }

    public void testSetCharArray() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) VALUES (?, ?)");
        sInsStmt.setInt(1, 11);
        char[] sCharArr2 = CLOB_VAL2.toCharArray();
        sInsStmt.setObject(2, sCharArr2, Types.CLOB);
        assertEquals(false, sInsStmt.execute());
        sInsStmt.close();

        assertExecuteScalar(CLOB_VAL2, "val1", "t1 WHERE id = 11");
    }

    public void testSetCLOB() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals(CLOB_VAL1, sRS.getString(2));
        Clob sClob = sRS.getClob(2);
        assertEquals(false, sRS.next());
        sStmt.close();

        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) VALUES (?, ?)");
        sInsStmt.setInt(1, 2);
        sInsStmt.setObject(2, sClob, Types.CLOB);
        assertEquals(false, sInsStmt.execute());
        sInsStmt.close();

        sStmt = connection().createStatement();
        sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals(CLOB_VAL1, sRS.getString(2));
        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt(1));
        assertEquals(CLOB_VAL1, sRS.getString(2));
        assertEquals(false, sRS.next());
        sStmt.close();
    }

    public void testUpdateObjByClob() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) VALUES (?, ?)");
        sInsStmt.setInt(1, 2);
        sInsStmt.setString(2, CLOB_VAL2);
        assertEquals(false, sInsStmt.execute());

        // 2번째 row의 clob을 1번째 clob의 값으로 update
        Statement sUpdStmt = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        ResultSet sUpdRS = sUpdStmt.executeQuery("SELECT id, val1 FROM t1");
        assertEquals(true, sUpdRS.next());
        assertEquals(1, sUpdRS.getInt(1));
        assertEquals(CLOB_VAL1, sUpdRS.getString(2));
        Clob sClob = sUpdRS.getClob(2);
        assertEquals(true, sUpdRS.next());
        assertEquals(2, sUpdRS.getInt(1));
        assertEquals(CLOB_VAL2, sUpdRS.getString(2));
        sUpdRS.updateObject(2, sClob);
        sUpdRS.updateRow();
        assertEquals(false, sUpdRS.next());
        sUpdStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sSelRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sSelRS.next());
        assertEquals(1, sSelRS.getInt(1));
        assertEquals(CLOB_VAL1, sSelRS.getString(2));
        assertEquals(true, sSelRS.next());
        assertEquals(2, sSelRS.getInt(1));
        assertEquals(CLOB_VAL1, sSelRS.getString(2));
        assertEquals(false, sSelRS.next());
        sSelStmt.close();
    }
    
    public void testUpdateClob() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) VALUES (?, ?)");
        sInsStmt.setInt(1, 2);
        sInsStmt.setString(2, CLOB_VAL2);
        assertEquals(false, sInsStmt.execute());

        // 2번째 row의 clob을 1번째 clob의 값으로 update
        Statement sUpdStmt = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        ResultSet sUpdRS = sUpdStmt.executeQuery("SELECT id, val1 FROM t1");
        assertEquals(true, sUpdRS.next());
        assertEquals(1, sUpdRS.getInt(1));
        assertEquals(CLOB_VAL1, sUpdRS.getString(2));
        Clob sClob = sUpdRS.getClob(2);
        assertEquals(true, sUpdRS.next());
        assertEquals(2, sUpdRS.getInt(1));
        assertEquals(CLOB_VAL2, sUpdRS.getString(2));
        sUpdRS.updateClob(2, sClob);
        sUpdRS.updateRow();
        assertEquals(false, sUpdRS.next());
        sUpdStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sSelRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sSelRS.next());
        assertEquals(1, sSelRS.getInt(1));
        assertEquals(CLOB_VAL1, sSelRS.getString(2));
        assertEquals(true, sSelRS.next());
        assertEquals(2, sSelRS.getInt(1));
        assertEquals(CLOB_VAL1, sSelRS.getString(2));
        assertEquals(false, sSelRS.next());
        sSelStmt.close();
    }

    public void testSetNullByNULL() throws SQLException
    {
        testSetNullBy(Types.NULL);
    }

    public void testSetNullByCLOB() throws SQLException
    {
        testSetNullBy(Types.CLOB);
    }

    public void testSetNullByCHAR() throws SQLException
    {
        testSetNullBy(Types.CHAR);
    }

    public void testSetNullByVARCHAR() throws SQLException
    {
        testSetNullBy(Types.VARCHAR);
    }

    public void testSetNullByLONGVARCHAR() throws SQLException
    {
        testSetNullBy(Types.LONGVARCHAR);
    }

    public void testSetNullByAsciiStream() throws SQLException
    {
        testSetNullBy(TYPES_ASCII_STREAM);
    }

    public void testSetNullByCharacterStream() throws SQLException
    {
        testSetNullBy(TYPES_CHAR_STREAM);
    }

    private static final int TYPES_ASCII_STREAM  = -100001;
    private static final int TYPES_BINARY_STREAM = -100002;
    private static final int TYPES_CHAR_STREAM   = -100003;

    private void testSetNullBy(int aJDBCType) throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) VALUES (?, ?)");
        sInsStmt.setInt(1, 2);
        switch (aJDBCType)
        {
            case TYPES_ASCII_STREAM:
                sInsStmt.setAsciiStream(2, null, 0);
                break;
            case TYPES_BINARY_STREAM:
                sInsStmt.setBinaryStream(2, null, 0);
                break;
            case TYPES_CHAR_STREAM:
                sInsStmt.setCharacterStream(2,  null, 0);
                break;
            default:
                sInsStmt.setObject(2, null, aJDBCType);
                break;
        }
        assertEquals(1, sInsStmt.executeUpdate());
        sInsStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1 WHERE id = 2");
        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt(1));
        assertEquals(null, sRS.getObject(2));
        assertEquals(false, sRS.next());
        sSelStmt.close();
    }

    public void testSetByCharacterStream() throws SQLException, IOException
    {
        StringReader sCharIN1 = new StringReader(CLOB_VAL1);
        StringReader sCharIN2 = new StringReader(CLOB_VAL2);

        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) VALUES (?, ?)");
        sInsStmt.setInt(1, 101);
        sInsStmt.setCharacterStream(2, sCharIN1, CLOB_VAL1.length());
        assertEquals(false, sInsStmt.execute());
        sInsStmt.setInt(1, 102);
        sInsStmt.setCharacterStream(2, sCharIN2, CLOB_VAL2.length());
        assertEquals(false, sInsStmt.execute());
        sInsStmt.close();

        Statement sStmt = connection().createStatement();
        ResultSet sSelRS = sStmt.executeQuery("SELECT id, val1 FROM t1 WHERE id > 100");
        assertEquals(true, sSelRS.next());
        assertEquals(101, sSelRS.getInt(1));
        assertEquals(CLOB_VAL1, sSelRS.getString(2));
        assertEquals(true, sSelRS.next());
        assertEquals(102, sSelRS.getInt(1));
        assertEquals(CLOB_VAL2, sSelRS.getString(2));
        assertEquals(false, sSelRS.next());
        sStmt.close();
    }

    private void testUpdateObj(int aUpdateType) throws SQLException
    {
        Statement sUpdStmt = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        ResultSet sUpdRS = sUpdStmt.executeQuery("SELECT id, val1 FROM t1");
        assertEquals(true, sUpdRS.next());
        assertEquals(1, sUpdRS.getInt(1));
        assertEquals(CLOB_VAL1, sUpdRS.getString(2));
        switch (aUpdateType)
        {
            case UPDATE_BY_ASCII_STREAM:
                InputStream sAsciiIN2 = new StringBufferInputStream(CLOB_VAL2);
                try {
                    sUpdRS.updateObject(2, sAsciiIN2);
                    fail();
                }
                catch(SQLException e)
                {
                    sUpdStmt.close();
                    return;
                }
                break;
            case UPDATE_BY_CHAR_STREAM:
                StringReader sCharIN2 = new StringReader(CLOB_VAL2);
                try {
                    sUpdRS.updateObject(2, sCharIN2);
                    fail();
                }
                catch(SQLException e)
                {
                    sUpdStmt.close();
                    return;
                }
                break;
            case UPDATE_BY_CHAR_ARR:
                sUpdRS.updateObject(2, CLOB_VAL2.toCharArray());
                sUpdRS.updateRow();
                assertEquals(false, sUpdRS.next());
                break;
            case UPDATE_BY_STRING:
                sUpdRS.updateObject(2, CLOB_VAL2);
                sUpdRS.updateRow();
                assertEquals(false, sUpdRS.next());
                break;
        }
       
        sUpdStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sSelRS = sSelStmt.executeQuery("SELECT id, val1 FROM t1");
        assertEquals(true, sSelRS.next());
        assertEquals(1, sSelRS.getInt(1));
        assertEquals(CLOB_VAL2, sSelRS.getString(2));
        assertEquals(false, sSelRS.next());
        sSelStmt.close();
    }

    public void testUpdateObjByString() throws SQLException, IOException
    {
        testUpdateObj(UPDATE_BY_STRING);
    }

    public void testUpdateObjByCharArr() throws SQLException, IOException
    {
        testUpdateObj(UPDATE_BY_CHAR_ARR);
    }
    
    public void testUpdateObjByAsciiStream() throws SQLException, IOException
    {
        testUpdateObj(UPDATE_BY_ASCII_STREAM);
    }
    
    public void testUpdateClobByAsciiStream() throws SQLException, IOException
    {
        testUpdateClobByStream(UPDATE_BY_ASCII_STREAM);
    }

    public void testUpdateClobByCharacterStream() throws SQLException, IOException
    {
        testUpdateClobByStream(UPDATE_BY_CHAR_STREAM);
    }
    
    public void testUpdateObjByCharacterStream() throws SQLException, IOException
    {
        testUpdateObj(UPDATE_BY_CHAR_STREAM);
    }

    private static final int UPDATE_BY_ASCII_STREAM = 1;
    private static final int UPDATE_BY_CHAR_STREAM  = 2;
    private static final int UPDATE_BY_CLOB         = 3;
    private static final int UPDATE_BY_CHAR_ARR     = 4;
    private static final int UPDATE_BY_STRING       = 5;

    public void testUpdateClobByStream(int aUpdateType) throws SQLException, IOException
    {
        Statement sUpdStmt = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        ResultSet sUpdRS = sUpdStmt.executeQuery("SELECT id, val1 FROM t1");
        assertEquals(true, sUpdRS.next());
        assertEquals(1, sUpdRS.getInt(1));
        assertEquals(CLOB_VAL1, sUpdRS.getString(2));
        switch (aUpdateType)
        {
            case UPDATE_BY_ASCII_STREAM:
                InputStream sAsciiIN2 = new StringBufferInputStream(CLOB_VAL2);
                sUpdRS.updateAsciiStream(2, sAsciiIN2, CLOB_VAL2.length());
                break;
            case UPDATE_BY_CHAR_STREAM:
                StringReader sCharIN2 = new StringReader(CLOB_VAL2);
                sUpdRS.updateCharacterStream(2, sCharIN2, CLOB_VAL2.length());
                break;
        }
        sUpdRS.updateRow();
        assertEquals(false, sUpdRS.next());
        sUpdStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sSelRS = sSelStmt.executeQuery("SELECT id, val1 FROM t1");
        assertEquals(true, sSelRS.next());
        assertEquals(1, sSelRS.getInt(1));
        assertEquals(CLOB_VAL2, sSelRS.getString(2));
        assertEquals(false, sSelRS.next());
        sSelStmt.close();
    }

    public void testSupportCharacterStream_EN() throws SQLException, IOException
    {
        testSupportCharacterStream("test/resources/large_en.txt");
    }

    public void testSupportCharacterStream_EUCKR() throws SQLException, IOException
    {
        testSupportCharacterStream("test/resources/large_euckr.txt");
    }

    public void testSupportCharacterStream(String aFilePath) throws SQLException, IOException
    {
        File sFile = new File(aFilePath);
        FileReader sFileReader = new FileReader(sFile);
        char[] sBuf = new char[(int)sFile.length()];
        int sReaded = sFileReader.read(sBuf);
        sFileReader.close();
        String sOrg = String.valueOf(sBuf, 0, sReaded);

        int sSelCheckCnt = 0;
        // set as Object
        {
            PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 (id, val1) values (?, ?)");
            sInsStmt.setInt(1, 1001);
            sFileReader = new FileReader(aFilePath);
            // sInsStmt.setCharacterStream(2, sFileReader, Integer.MAX_VALUE);
            sInsStmt.setObject(2, sFileReader);
            assertEquals(1, sInsStmt.executeUpdate());
            sFileReader.close();
            sInsStmt.close();
            connection().commit();
        }
        // get as CharacterStream
        {
            Statement sSelStmt = connection().createStatement();
            ResultSet sRS = sSelStmt.executeQuery("SELECT val1 FROM t1 WHERE id = 1001");
            assertEquals(true, sRS.next());
            Reader sClobReader = sRS.getCharacterStream(1);
            char[] sCharBuf = new char[sOrg.length()];
            sClobReader.read(sCharBuf);
            String sStr = String.valueOf(sCharBuf);
            assertEquals(sOrg, sStr);
            assertEquals(false, sRS.next());
            sRS.close();
            sSelStmt.close();
        }
        // update by CharacterStream and updatable cursor
        {
            Statement sSelStmt = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE);
            ResultSet sUpdRS = sSelStmt.executeQuery("SELECT val1 FROM t1 ORDER BY id DESC");
            assertEquals(true, sUpdRS.next());
            Reader sClobReader = sUpdRS.getClob(1).getCharacterStream();
            assertEquals(true, sUpdRS.next());
            sUpdRS.updateCharacterStream(1, sClobReader, Integer.MAX_VALUE);
            sUpdRS.updateRow();
            assertEquals(false, sUpdRS.next());
            sUpdRS.close();
            sSelStmt.close();
            sSelCheckCnt++;
        }
        // update by String and updatable cursor
        if (false) // TODO
        {
            Statement sStmt = connection().createStatement();
            assertEquals(1, sStmt.executeUpdate("INSERT INTO t1 (id, val1) values (1003, null)"));

            Statement sSelStmt = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE);
            ResultSet sUpdRS = sSelStmt.executeQuery("SELECT val1 FROM t1 WHERE id = 1003");
            assertEquals(true, sUpdRS.next());
            sUpdRS.updateObject(1, sOrg);
            sUpdRS.updateRow();
            assertEquals(false, sUpdRS.next());
            sUpdRS.close();
            sSelStmt.close();
            sSelCheckCnt++;
        }
        // update by CLOB writer
        {
            Statement sStmt = connection().createStatement();
            assertEquals(1, sStmt.executeUpdate("INSERT INTO t1 (id, val1) values (1002, null)"));

            ResultSet sRS = sStmt.executeQuery("SELECT val1 FROM t1 WHERE id = 1002 FOR UPDATE");
            assertEquals(true, sRS.next());
            Clob sClob = sRS.getClob(1);
            if (sClob != null)  // BUG-47639 getClob() will return null if clob column is null
            {
                Writer sWriter = sClob.setCharacterStream(0);
                sWriter.write(sOrg);
            }
            sRS.close();
            sStmt.close();
            sSelCheckCnt++;
        }
        // check update
        {
            Statement sSelStmt = connection().createStatement();
            ResultSet sSelRS = sSelStmt.executeQuery("SELECT id, val1 FROM t1 WHERE id > 1000 ORDER BY id ASC");
            for (int i = 1; i <= sSelCheckCnt; i++)
            {
                assertEquals(i + "th fetch", true, sSelRS.next());
                int sID = sSelRS.getInt(1);
                Clob sClob = sSelRS.getClob(2);
                if (sClob != null)  // BUG-47639 getClob() will return null if clob column is null
                {
                    Reader sClobReader = sClob.getCharacterStream();
                    char[] sCharBuf = new char[sOrg.length()];
                    sClobReader.read(sCharBuf);
                    String sStr = String.valueOf(sCharBuf);
                    assertEquals("CLOB[" + sID + "]", sOrg, sStr);
                }
            }
            assertEquals(false, sSelRS.next());
            sSelRS.close();
            sSelStmt.close();
        }
    }

    public void testReadLength() throws SQLException, IOException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate("INSERT INTO t1 (id, val1) values (1001, '가나다')"));

        ResultSet sRS = sStmt.executeQuery("SELECT val1 FROM t1 WHERE id = 1001");
        assertEquals(true, sRS.next());
        Reader sReader = sRS.getCharacterStream(1);
        char[] sBuf = new char[10];
        int sReaded = sReader.read(sBuf);
        assertEquals("가나다", String.valueOf(sBuf, 0, sReaded));
        assertEquals(3, sReaded);
        sRS.close();
        sStmt.close();
    }
}
