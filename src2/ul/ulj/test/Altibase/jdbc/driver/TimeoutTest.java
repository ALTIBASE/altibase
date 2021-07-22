package Altibase.jdbc.driver;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class TimeoutTest extends AltibaseTestCase
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

    protected boolean useConnectionPreserve()
    {
        return false;
    }

    // BUGBUG 자동으로 테스트하기 어렵다; 너무 빨리 붙어서리;;
    public void _NOTYET_testLoginTimeout()
    {
        AltibaseProperties sProp = new AltibaseProperties();
        sProp.setDataSource(CONN_DSN);
        sProp.setUser(CONN_USER);
        sProp.setPassword(CONN_PWD);
        sProp.setLoginTimeout(1);
        try
        {
            new AltibaseConnection(sProp, null);
            fail();
        }
        catch (SQLException sEx)
        {
            printSQLException(sEx);
        }
    }

    public void _NOTYET_testResponseTimeout() throws SQLException, InterruptedException
    {
        testTimeout("response_timeout", null, ErrorDef.RESPONSE_TIMEOUT, true, true);
        testTimeout("response_timeout", null, ErrorDef.RESPONSE_TIMEOUT, true, false);
    }

    public void testQueryTimeout() throws SQLException, InterruptedException
    {
        testTimeout("query_timeout", "QUERY_TIME_LIMIT", ErrorDef.QUERY_TIMEOUT, false, true);
        testTimeout("query_timeout", "QUERY_TIME_LIMIT", ErrorDef.QUERY_TIMEOUT, false, false);
    }

    private void testTimeout(String aTimeoutPropKey, String aSessionField, int aExpErrorCode, boolean aShouldClose, boolean aUseURL) throws SQLException, InterruptedException
    {
        Connection sConn4Lock = getConnection();
        sConn4Lock.setAutoCommit(false);
        Statement sSelStmt = sConn4Lock.createStatement();
        Statement sLockStmt = sConn4Lock.createStatement();
        sLockStmt.execute("LOCK TABLE t1 IN SHARE MODE");
        Object sId = executeScalar(sSelStmt, "SELECT id FROM X$SESSION");
        assertExecuteScalar(sSelStmt, "1", "Count(*)", "X$SESSION");

        Connection sConn4DML;
        if (aUseURL)
        {
            sConn4DML = getConnection(CONN_URL + "?" + aTimeoutPropKey + "=1");
        }
        else
        {
            AltibaseProperties sProp = new AltibaseProperties();
            sProp.setDataSource(CONN_DSN);
            sProp.setUser(CONN_USER);
            sProp.setPassword(CONN_PWD);
            sProp.setProperty(aTimeoutPropKey, 1);
            sConn4DML = new AltibaseConnection(sProp, null);
        }
        if (aSessionField != null)
        {
            assertExecuteScalar(sSelStmt, "1", aSessionField, "X$SESSION WHERE id != "+ sId);
        }
        assertExecuteScalar(sSelStmt, "2", "Count(*)", "X$SESSION");
        Statement sUpdStmt = sConn4DML.createStatement();
        try
        {
            sUpdStmt.execute("INSERT INTO t1 VALUES (10)");
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(aExpErrorCode, sEx.getErrorCode());
        }

        sLockStmt.close();
        sConn4Lock.commit();

        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        // response timeout은 원자성(Atomicity)을 보장 못하므로 execute에 성공 할 수도 있다.
        // 다만, 여기서는 execute 전 lock 대기하다 나는거기 때문에 insert되지 않아야 한다.
        assertEquals(false, sRS.next());
        sRS.close();
        if (aShouldClose)
        {
            // X$로 조회하면 정보가 아직 남아있어 실패할 수 있다.
            assertExecuteScalar(sSelStmt, "1", "Count(*)", "V$SESSION");
        }
        else
        {
            assertExecuteScalar(sSelStmt, "2", "Count(*)", "X$SESSION");
            sConn4DML.close();
            assertExecuteScalar(sSelStmt, "1", "Count(*)", "X$SESSION");
        }
        sSelStmt.close();
        sConn4Lock.close();
        sConn4DML.close(); // 안전을 위해 close
    }

    public void testIdleTimeout() throws SQLException, InterruptedException
    {
        testTimeout2("idle_timeout", "IDLE_TIME_LIMIT", true);
        testTimeout2("idle_timeout", "IDLE_TIME_LIMIT", false);
    }

    public void testFetchTimeout() throws SQLException, InterruptedException
    {
        testTimeout2("fetch_timeout", "FETCH_TIME_LIMIT", true);
        testTimeout2("fetch_timeout", "FETCH_TIME_LIMIT", false);
    }

    public void testUtransTimeout() throws SQLException, InterruptedException
    {
        testTimeout2("utrans_timeout", "UTRANS_TIME_LIMIT", true);
        testTimeout2("utrans_timeout", "UTRANS_TIME_LIMIT", false);
    }

    public void testDDLTimeout() throws SQLException, InterruptedException
    {
        testTimeout2("ddl_timeout", "DDL_TIME_LIMIT", true);
        testTimeout2("ddl_timeout", "DDL_TIME_LIMIT", false);
    }

    private void testTimeout2(String aTimeoutPropKey, String aSessionField, boolean aUseURL) throws SQLException
    {
        Connection sConn;
        if (aUseURL)
        {
            sConn = getConnection(CONN_URL + "?"+ aTimeoutPropKey +"=1");
        }
        else
        {
            AltibaseProperties sProp = new AltibaseProperties();
            sProp.setDataSource(CONN_DSN);
            sProp.setUser(CONN_USER);
            sProp.setPassword(CONN_PWD);
            sProp.setProperty(aTimeoutPropKey, 1);
            sConn = new AltibaseConnection(sProp, null);
        }
        Statement sSelStmt = sConn.createStatement();
        if (aSessionField != null)
        {
            assertExecuteScalar(sSelStmt, "1", aSessionField, "X$SESSION");
        }
        sConn.setAutoCommit(false);

        // TODO Timeout이 떨어질만한 쿼리를 함 날려보고, 실제로 에러 떨어지는걸 확인해보면 좋겠다. 음;

        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
        sConn.rollback();
        sConn.close();
    }
}
