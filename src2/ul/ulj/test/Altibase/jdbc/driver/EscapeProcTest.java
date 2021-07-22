package Altibase.jdbc.driver;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.ex.ErrorDef;

public class EscapeProcTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP TABLE t2",
                "DROP PROCEDURE p1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (cINT INTEGER, cVARCHAR VARCHAR(20), cDATE DATE)",
                "INSERT INTO t1 VALUES (1, 'aa%bca', TO_DATE('2013-01-23 12:32:56.286195', 'yyyy-MM-dd hh24:mi:ss.ff6'))",
                "INSERT INTO t1 VALUES (2, 'ac%bac', TO_DATE('1987-10-24 01:23:45.678901', 'yyyy-MM-dd hh24:mi:ss.ff6'))",
                "CREATE TABLE t2 (cINT INTEGER, cBIGINT BIGINT)",
                "INSERT INTO t2 VALUES (2, 100)",
                "CREATE PROCEDURE p1()" 
                + " AS BEGIN "
                    + "UPDATE t1 SET cINT = cINT + 10;"
                + " END"
        };
    }

    public void testEscapeESCAPE() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.setEscapeProcessing(false);
        try
        {
            sStmt.executeQuery("SELECT cVARCHAR FROM t1 WHERE cVARCHAR LIKE 'a|%b' {escape '|'}");
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.SQL_SYNTAX_ERROR, sEx.getErrorCode());
        }
        sStmt.setEscapeProcessing(true);
        ResultSet sRS = sStmt.executeQuery("SELECT cVARCHAR FROM t1 WHERE cVARCHAR LIKE '%a|%b%' {escape '|'}");
        assertEquals(true, sRS.next());
        assertEquals("aa%bca", sRS.getString(1));
        assertEquals(false, sRS.next());
        sStmt.close();
    }

    public void testEscapeFN() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.setEscapeProcessing(false);
        try
        {
            sStmt.executeQuery("SELECT {fn concat('concat', 'test')} FROM dual");
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.SQL_SYNTAX_ERROR, sEx.getErrorCode());
        }
        sStmt.setEscapeProcessing(true);
        ResultSet sRS = sStmt.executeQuery("SELECT {fn concat('concat', 'test')} FROM dual");
        assertEquals(true, sRS.next());
        assertEquals("concattest", sRS.getString(1));
        assertEquals(false, sRS.next());
        sStmt.close();
    }

    private static final String DTS_ESCAPE_TESTSETS[][] = {
        { "{d  '1234-12-30'}"                 , "1234-12-30 00:00:00.0" },
        { "{t  '12:34:56'}"                   , new java.text.SimpleDateFormat("yyyy-MM").format(new java.util.Date()) + "-01 12:34:56.0" },
        { "{ts '2010-01-23 12:23:45'}"        , "2010-01-23 12:23:45.0" },
        { "{ts '2010-11-29 23:01:23.971589'}" , "2010-11-29 23:01:23.971589" },
    };

    public void testEscapeDTS() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        for (int i = 0; i < DTS_ESCAPE_TESTSETS.length; i++)
        {
            sStmt.setEscapeProcessing(false);
            try
            {
                sStmt.execute("UPDATE t1 SET cDATE = " + DTS_ESCAPE_TESTSETS[i][0]);
                fail();
            }
            catch (SQLException sEx)
            {
                assertEquals(ErrorDef.SQL_SYNTAX_ERROR, sEx.getErrorCode());
            }

            sStmt.setEscapeProcessing(true);
            assertEquals(2, sStmt.executeUpdate("UPDATE t1 SET cDATE = " + DTS_ESCAPE_TESTSETS[i][0]));
            assertExecuteScalar(DTS_ESCAPE_TESTSETS[i][1], "cDATE");
        }
        sStmt.close();
    }

    public void testEscapeCALL() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.setEscapeProcessing(false);
        try
        {
            sStmt.execute("{call p1()}");
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.SQL_SYNTAX_ERROR, sEx.getErrorCode());
        }
        sStmt.setEscapeProcessing(true);
        sStmt.execute("{call p1()}");
        assertExecuteScalar("11", "cINT");
        sStmt.close();
    }

    public void testEscapeOJ() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.setEscapeProcessing(false);
        try
        {
            sStmt.executeQuery("SELECT * FROM {oj t1 LEFT OUTER JOIN t2 ON t1.cINT = t2.cINT}");
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.SQL_SYNTAX_ERROR, sEx.getErrorCode());
        }
        sStmt.setEscapeProcessing(true);
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM {oj t1 LEFT OUTER JOIN t2 ON t1.cINT = t2.cINT}");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt("cINT"));
        assertEquals(null, sRS.getObject("cBIGINT"));
        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt("cINT"));
        assertEquals(100, sRS.getInt("cBIGINT"));
        assertEquals(false, sRS.next());
        sStmt.close();
    }
}
