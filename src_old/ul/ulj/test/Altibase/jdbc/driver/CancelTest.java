package Altibase.jdbc.driver;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.ex.ErrorDef;

public class CancelTest extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 INTEGER)",
                "INSERT INTO t1 VALUES (1)",
                "INSERT INTO t1 VALUES (2)",
                "INSERT INTO t1 VALUES (3)",
                "INSERT INTO t1 VALUES (4)",
                "INSERT INTO t1 VALUES (5)",
                "INSERT INTO t1 VALUES (6)",
                "INSERT INTO t1 VALUES (7)",
                "INSERT INTO t1 VALUES (8)",
                "INSERT INTO t1 VALUES (9)",
        };
    }

    public void testCancelNoEffect() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.cancel();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        sStmt.cancel();
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
            sStmt.cancel();
        }
        assertEquals(false, sRS.next());
        sStmt.cancel();
        sRS.close();
        sStmt.close();
    }

    public void _NOTYET_testCancelSLockWait() throws SQLException, InterruptedException
    {
        testCancelLockWait("LOCK TABLE t1 IN SHARE MODE", "INSERT INTO t1 VALUES (10)");
    }

    public void testCancelXLockWait() throws SQLException, InterruptedException
    {
        testCancelLockWait("LOCK TABLE t1 IN SHARE MODE", "INSERT INTO t1 VALUES (10)");
    }

    private void testCancelLockWait(final String aLockQstr, final String aExecQstr) throws SQLException, InterruptedException
    {
        connection().setAutoCommit(false);
        Statement sLockStmt = connection().createStatement();
        sLockStmt.execute(aLockQstr);

        Connection sConn = getConnection();
        final Statement sUpdStmt = sConn.createStatement();
        Thread sUpdThread = new Thread(new Runnable() {
            public void run()
            {
                try
                {
                    sUpdStmt.execute(aExecQstr);
                    fail();
                }
                catch (SQLException sEx)
                {
                    assertEquals(ErrorDef.QUERY_CANCELED_BY_CLIENT, sEx.getErrorCode());
                }
            }
        });
        sUpdThread.start();

        Thread.sleep(3000); // Lock 때문에 멈춘 상태가 되는걸 확실히하기 위해 3초동안 기다림

        sUpdStmt.cancel();
        sUpdStmt.close();
        sLockStmt.close();
        connection().commit(); // unlock

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
    }
}
