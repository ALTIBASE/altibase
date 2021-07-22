package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.SQLException;
import java.sql.Statement;

public class ProwIdTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP TABLE t2",
                "DROP VIEW v1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 INTEGER)",
                "CREATE TABLE t2 (c1 INTEGER)",
                "CREATE VIEW v1 as SELECT * FROM t1",
        };
    }

    public void test_CAN_SELECT_PROWID() throws Exception
    {
        Statement sStmt = connection().createStatement();

        // multiple tables
        sStmt.executeQuery("SELECT t1._PROWID FROM t1, t2");

        // using _prowid in subquery
        sStmt.executeQuery("SELECT * FROM (SELECT _PROWID FROM t1)");
    }

    public void test_CANNOT_SELECT_PROWID() throws Exception
    {
        Statement sStmt = connection().createStatement();

        // view
        try
        {
            sStmt.executeQuery("SELECT _PROWID FROM v1");
            fail();
        }
        catch (SQLException ex)
        {
            assertEquals(ErrorDef.COLUMN_NOT_FOUND, ex.getErrorCode());
        }
    }

    public void testSQLStateForServerError() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        try
        {
            sStmt.executeQuery("SELECT * FROM t3");
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals("42S02", sEx.getSQLState());
        }
    }
}
