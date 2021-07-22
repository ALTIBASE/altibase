package Altibase.jdbc.driver;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Savepoint;
import java.sql.Statement;

public class SavepointTest extends AltibaseTestCase
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
            "CREATE TABLE t1 (seq INTEGER, c1 INTEGER)",
            "INSERT INTO t1 VALUES (1, 1)",
            "INSERT INTO t1 VALUES (2, 2)",
            "INSERT INTO t1 VALUES (3, 3)",
            "INSERT INTO t1 VALUES (4, 4)",
            "INSERT INTO t1 VALUES (5, 5)",
        };
    }

    private void assertResultSet() throws SQLException
    {
        assertResultSet(new int[] { 1, 2, 3, 4, 5 });
    }

    private void assertResultSet(int[] aExpVals) throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT c1 FROM t1 ORDER BY seq");
        for (int i = 0; i < aExpVals.length; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(aExpVals[i], sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sStmt.close();
    }

    public void testRollback() throws SQLException
    {
        testRollback(null);
    }

    public void testRollbackByName() throws SQLException
    {
        testRollback("testSP");
    }

    private void testRollback(String aSavepointName) throws SQLException
    {
        connection().setAutoCommit(false);
        assertResultSet();
        Savepoint sSP = connection().setSavepoint(aSavepointName);
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate("UPDATE t1 SET c1 = c1 + 10 WHERE seq = 3"));
        sStmt.close();
        assertResultSet(new int[] { 1, 2, 13, 4, 5 });
        connection().rollback(sSP);
        assertResultSet();
        connection().releaseSavepoint(sSP);
    }
}
