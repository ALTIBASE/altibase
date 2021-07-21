package Altibase.jdbc.driver.datatype;

import Altibase.jdbc.driver.AltibaseBlob;
import Altibase.jdbc.driver.AltibaseTestCase;

import java.io.StringReader;
import java.sql.*;

import static org.junit.Assert.assertArrayEquals;

public class BatchLobSetNullTest extends AltibaseTestCase
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
                "CREATE TABLE t1 (i1 INTEGER, i2 CLOB, i3 BLOB)",
        };
    }

    protected String getURL()
    {
        return super.getURL() + "?CLIENTSIDE_AUTO_COMMIT=on";
    }
    
    /**
     * clob,blob 컬럼에 각각 1번째 not null, 2번째 null, 3번째 not null을 넣은 후 두번째 로우의 값이 null인지 체크
     * @throws SQLException
     */
    public void testBatchSetNullAndWasNull() throws SQLException
    {
        insertData();
        updateData();
        selectData();
    }

    private void updateData() throws SQLException
    {
        String sClobText = "clobtext";
        PreparedStatement sStmt = null;
        try
        {
            sStmt = getConnection().prepareStatement("UPDATE t1 SET i2 = ?, i3 = ? WHERE i1 = ? ");
            for (int i=0; i < 3; i++)
            {
                if (i == 0)
                {
                    // 첫번째 행에는 기존에 있는 값을 null로 업데이트한다.
                    sStmt.setNull(1, Types.VARCHAR);
                    sStmt.setNull(2, Types.VARCHAR);
                }
                else
                {
                    // 두번째와 세번째 행에는 각각 데이터를 업데이트한다.
                    sStmt.setCharacterStream(1, new StringReader(sClobText), sClobText.length());
                    sStmt.setBytes(2, new byte[] {0x1f, 0x1d});
                }
                sStmt.setInt(3, i+1);
                sStmt.addBatch();
            }
            sStmt.executeBatch();
            sStmt.close();
        }
        finally
        {
            // clientside_auto_commit이 on이기때문에 finally절에서 Statement를 close해준다.
            sStmt.close();
        }
    }

    private void selectData() throws SQLException
    {
        Statement sSelStmt = null;
        ResultSet sRs = null;
        try
        {
            sSelStmt = getConnection().createStatement();
            sRs = sSelStmt.executeQuery("SELECT * FROM t1 ORDER BY i1 ");
            for (int i=0; sRs.next(); i++)
            {
                int idx = sRs.getInt(1);
                assertEquals(i + 1, idx);
                checkClobValue(i, sRs);

                if (i == 0)
                {
                    assertTrue(i+" th wasNull", sRs.wasNull());
                }
                else 
                {
                    assertFalse(i + "th wasNotNull", sRs.wasNull());
                }
                checkBlobValue(i, sRs);

                if (i == 0)
                {
                    assertTrue(i+" th wasNull", sRs.wasNull());
                }
                else 
                {
                    assertFalse(i + "th wasNotNull", sRs.wasNull());
                }
            }
        }
        finally
        {
            // clientside_auto_commit이 on이기때문에 ResultSet 및 Statement를 close해준다.
            sRs.close();
            sSelStmt.close();
        }
    }

    private void checkClobValue(int idx, ResultSet sRs) throws SQLException
    {
        Object sClobVal = sRs.getObject(2);

        if (idx == 0)
        {
            assertNull(sClobVal);
        }
        else
        {
            assertEquals("clobtext", sClobVal.toString());
        }
    }

    private void checkBlobValue(int idx, ResultSet sRs) throws SQLException
    {
        Object sBlobVal = sRs.getObject(3);

        if (idx == 0)
        {
            assertNull(sBlobVal);
        }
        else
        {
            assertArrayEquals(new byte[] { 0x1f, 0x1d }, ((AltibaseBlob)sBlobVal).getBytes(1, 2));
        }
    }

    private void insertData() throws SQLException
    {
        String sClobText = "clobtext";
        PreparedStatement sStmt = null;
        try
        {
            sStmt = getConnection().prepareStatement("INSERT INTO t1 VALUES (?, ?, ?); ");
            for (int i=0; i < 3; i++)
            {
                sStmt.setInt(1, i+1);
                if (i == 1)
                {
                    // 2번째 행에는 null값을 insert한다.
                    sStmt.setNull(2, Types.VARCHAR);
                    sStmt.setNull(3, Types.VARCHAR);
                }
                else
                {  
                    // 첫번째와 세번째 행에는 각각 데이터를 insert한다.
                    sStmt.setCharacterStream(2, new StringReader(sClobText), sClobText.length());
                    sStmt.setBytes(3, new byte[] {0x1f, 0x1d});
                }
                sStmt.addBatch();
            }
            sStmt.executeBatch();
            sStmt.close();
        }
        finally
        {
            // clientside_auto_commit이 on이기때문에 finally절에서 Statement를 close해준다.
            sStmt.close();
        }
    }
}