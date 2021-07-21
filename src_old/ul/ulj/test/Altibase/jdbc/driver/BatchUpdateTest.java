package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.*;

import static org.junit.Assert.assertArrayEquals;

public class BatchUpdateTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE TEST",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE TEST (c1 VARCHAR(20))"
        };
    }

    public void testBatchUpdateForPreparedSelect() throws SQLException
    {
        PreparedStatement pstmt = connection().prepareStatement("SELECT C1 FROM TEST WHERE C1=?");
        pstmt.setObject(1, "010101", Types.VARCHAR);
        pstmt.addBatch();
        try {
            pstmt.executeBatch();
            fail();
        }
        catch(BatchUpdateException sEx)
        {
            assertArrayEquals(new int[] {}, sEx.getUpdateCounts());
            assertEquals(ErrorDef.INVALID_BATCH_OPERATION_WITH_SELECT, sEx.getErrorCode());
        }
    }

    public void testBatchUpdateForPreparedSelectWithNoParam() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM test");
        sStmt.addBatch();
        try
        {
            sStmt.executeBatch();
            fail();
        }
        catch (BatchUpdateException sEx)
        {
            assertArrayEquals(new int[] {}, sEx.getUpdateCounts());
            assertEquals(ErrorDef.INVALID_BATCH_OPERATION_WITH_SELECT, sEx.getErrorCode());
        }
        sStmt.close();
    }

    public void testBatchUpdateForSelectWithNoParam() throws SQLException
    {
        Statement sBatchStmt = connection().createStatement();
        sBatchStmt.addBatch("INSERT INTO test VALUES ('a')");
        sBatchStmt.addBatch("SELECT * FROM test");
        sBatchStmt.addBatch("INSERT INTO test VALUES ('b')");
        sBatchStmt.addBatch("INSERT INTO test VALUES ('c')");
        try
        {
            sBatchStmt.executeBatch();
            fail();
        }
        catch (SQLException sEx)
        {
            assertTrue(sEx instanceof BatchUpdateException);
            assertEquals(ErrorDef.BATCH_UPDATE_EXCEPTION_OCCURRED, sEx.getErrorCode());
            int[] sBatchResult = ((BatchUpdateException)sEx).getUpdateCounts();
            assertArrayEquals(new int[] { 1, Statement.EXECUTE_FAILED, 1, 1 }, sBatchResult);
            sEx = sEx.getNextException();
            assertEquals(ErrorDef.INVALID_BATCH_OPERATION_WITH_SELECT, sEx.getErrorCode());
            sEx = sEx.getNextException();
            assertEquals(null, sEx);
        }
        sBatchStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM test ORDER BY c1");
        assertEquals(true, sRS.next());
        assertEquals("a", sRS.getString(1));
        // BUGBUG (2013-01-03) 2번째 구문에서 실패하므로 그 다음 명령으로 삽입되는 b, c는 없어야 한다. (oracle-like)
        // 하지만, Altibase는 모두 수행시켜주고 SELECT 에러가 있음을 표시한다.
        assertEquals(true, sRS.next());
        assertEquals("b", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("c", sRS.getString(1));
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
    }

    public void testBUG19380() throws SQLException
    {
        connection().setAutoCommit(false);
        PreparedStatement ps = connection().prepareStatement("INSERT INTO test VALUES (?)");

        for (int i = 0; i < 10000; i++)
        {
            ps.setString(1, "asdasdasdasdasdasd");
            ps.addBatch();
        }
        ps.executeBatch();
        assertEquals(10000, ps.getUpdateCount());
        connection().commit();
    }

    public void testExecuteBatchWithoutClearBatch() throws SQLException
    {
        int[] sBatchResult;
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO test VALUES (?)");
        sInsStmt.setString(1, "a");
        sInsStmt.addBatch();
        sBatchResult = sInsStmt.executeBatch(); // executeBatch 후엔 clearBatch
        assertEquals(1, sInsStmt.getUpdateCount());
        assertArrayEquals(new int[] { 1 }, sBatchResult);

        sInsStmt.setString(1, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"); // DB type보다 큰 값
        sInsStmt.addBatch();
        try
        {
            sInsStmt.executeBatch(); // executeBatch 때 예외가 터지더라도 clearBatch
            fail();
        }
        catch (SQLException sEx)
        {
            assertTrue(sEx instanceof BatchUpdateException);
            assertArrayEquals(new int[] { Statement.EXECUTE_FAILED }, ((BatchUpdateException)sEx).getUpdateCounts());
            assertEquals(ErrorDef.BATCH_UPDATE_EXCEPTION_OCCURRED, sEx.getErrorCode());
            sEx = sEx.getNextException();
            assertEquals(ErrorDef.INVALID_DATA_TYPE_LENGTH, sEx.getErrorCode());
        }

        sInsStmt.setString(1, "c");
        sInsStmt.addBatch();
        sBatchResult = sInsStmt.executeBatch();
        assertEquals(1, sInsStmt.getUpdateCount());
        assertEquals(1, sBatchResult.length);
        assertArrayEquals(new int[] { 1 }, sBatchResult);
        sInsStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM test ORDER BY c1");
        assertEquals(true, sRS.next());
        assertEquals("a", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("c", sRS.getString(1));
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
    }

    public void testExecuteQueryAfterExecuteBatch() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.addBatch("INSERT INTO test VALUES ('c')");
        sStmt.executeBatch();
        assertEquals(1, sStmt.getUpdateCount());

        ResultSet sRS = sStmt.executeQuery("SELECT * FROM test");
        assertEquals(true, sRS.next());
        assertEquals("c", sRS.getString(1));
        assertEquals(false, sRS.next());
        sRS.close();

        sStmt.close();
    }

    public void testExecuteQueryAfterAddBatch() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.addBatch("UPDATE t1 SET c1='c', c2='d'");

        try
        {
            sStmt.executeQuery("SELECT * FROM t1");
            fail();
        }
        catch (BatchUpdateException sEx)
        {
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.SOME_BATCH_JOB, sEx.getErrorCode());
        }

        sStmt.close();
    }

    public void testExecuteEmptyBatchWithPrepare() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO test VALUES (?)");
        int[] sBatchResult = sInsStmt.executeBatch();
        assertEquals(0, sBatchResult.length);
        sInsStmt.close();
    }

    public void testExecuteEmptyBatch() throws SQLException
    {
        Statement sInsStmt = connection().createStatement();
        int[] sBatchResult = sInsStmt.executeBatch();
        assertEquals(0, sBatchResult.length);
        sInsStmt.close();
    }

    // natc : TC/Interface/newJDBC/Bugs/BUG-24704/BUG-24704.sql
    public void testAddBatchWithNoParam() throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO test VALUES ('t')");
        sInsStmt.addBatch();
        sInsStmt.addBatch();
        sInsStmt.addBatch();
        sInsStmt.addBatch();
        sInsStmt.addBatch();
        int[] sBatchResult = sInsStmt.executeBatch();
        assertEquals(1, sInsStmt.getUpdateCount());
        assertArrayEquals(new int[] { 1 }, sBatchResult);
        sInsStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM test");
        assertEquals(true, sRS.next());
        assertEquals("t", sRS.getString(1));
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
    }

    public void testClearParameters() throws SQLException
    {
        PreparedStatement sPreStmt = connection().prepareStatement("INSERT INTO test VALUES(?)");
        for (int i = 0; i < 10; i++)
        {
            sPreStmt.clearParameters();
            sPreStmt.setString(1, String.valueOf(i));
            sPreStmt.addBatch();
        }
        sPreStmt.executeBatch();
        assertEquals(10, sPreStmt.getUpdateCount());
        sPreStmt.close();

        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM test");
        for (int i = 0; i < 10; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(String.valueOf(i), sRS.getString(1));
        }
        sStmt.close();
    }

    public void testEmptyBatchExecute() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        int[] sResult = sStmt.executeBatch();
        assertEquals(0, sResult.length);
        assertEquals(-1, sStmt.getUpdateCount());

        sStmt = connection().prepareStatement("INSERT INTO TEST VALUES (?)");
        sResult = sStmt.executeBatch();
        assertEquals(0, sResult.length);
        assertEquals(-1, sStmt.getUpdateCount());
    }

    public void testTransactionalBehaviour() throws Exception
    {
        Statement sStmt  = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate("INSERT INTO test VALUES ('0')"));

        connection().setAutoCommit(false);
        sStmt.setQueryTimeout(10);
        sStmt.addBatch("UPDATE test SET c1 = '1'");
        sStmt.addBatch("UPDATE test SET c1 = '2'");
        sStmt.executeBatch();
        
        connection().rollback();
        assertExecuteScalar("0", "c1", "test");

        sStmt.clearBatch();
        sStmt.addBatch("UPDATE test SET c1 = '11'");
        sStmt.addBatch("UPDATE test SET c1 = '12'");

        // The statement has been added to the batch, but it should not yet
        // have been executed.
        assertExecuteScalar("0", "c1", "test");

        int[] updateCounts = sStmt.executeBatch();
        
        assertEquals(2, updateCounts.length);
        
        assertEquals(1, updateCounts[0]);
        assertEquals(1, updateCounts[1]);

        assertExecuteScalar("12", "c1", "test");
        connection().commit();
        assertExecuteScalar("12", "c1", "test");
        connection().rollback();
        assertExecuteScalar("12", "c1", "test");
    }
}
