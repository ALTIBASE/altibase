package Altibase.jdbc.driver;

import java.sql.DatabaseMetaData;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.ex.ErrorDef;

public class AutoKeyGenerationTest extends AltibaseTestCase
{
    private static final String INSERT_SQL               = "INSERT INTO t1 VALUES (t1_id_seq.nextval, t1_val_seq.nextval, 1)";
    private static final String INSERT_SQL_WITH_COLNAMES = "INSERT INTO t1 (id, val, val2) VALUES (t1_id_seq.NEXTVAL, t1_val_seq.nextval, 1)";

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP SEQUENCE t1_id_seq",
                "DROP SEQUENCE t1_val_seq",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (id INTEGER, val INTEGER, val2 INTEGER)",
                "CREATE SEQUENCE t1_id_seq START WITH 1",
                "CREATE SEQUENCE t1_val_seq START WITH 101",
        };
    }

    public void testMeta() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();
        assertEquals(true, sMetaData.supportsGetGeneratedKeys());
    }

    public void testGetGeneratedKeys() throws SQLException
    {
        testGetGeneratedKeys(INSERT_SQL, 1);
        testGetGeneratedKeys(INSERT_SQL_WITH_COLNAMES, 3);
    }

    private void testGetGeneratedKeys(String aQstr, int aNextID) throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate(aQstr, Statement.RETURN_GENERATED_KEYS));
        assertGeneratedKeys(sStmt, aNextID, aNextID + 100);
        sStmt.close();

        PreparedStatement sPStmt = connection().prepareStatement(aQstr, Statement.RETURN_GENERATED_KEYS);
        assertEquals(1, sPStmt.executeUpdate());
        assertGeneratedKeys(sPStmt, aNextID + 1, aNextID + 101);
        sPStmt.close();
    }

    private void assertGeneratedKeys(Statement sStmt, int aExpID1) throws SQLException
    {
        assertGeneratedKeys(sStmt, aExpID1, 0);
    }

    private void assertGeneratedKeys(Statement sStmt, int aExpID1, int aExpID2) throws SQLException
    {
        ResultSet sKeys = sStmt.getGeneratedKeys();
        assertEquals(true, sKeys.next());
        assertEquals(aExpID1, sKeys.getInt(1));
        if (aExpID2 != 0)
        {
            assertEquals(aExpID2, sKeys.getInt(2));
        }
        assertEquals(false, sKeys.next());
        sKeys.close();
    }

    public void testGetGeneratedKeysWithColumnIndexes() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate(INSERT_SQL, new int[] { 1 }));
        assertGeneratedKeys(sStmt, 1);
        assertEquals(1, sStmt.executeUpdate(INSERT_SQL, new int[] { 2 }));
        assertGeneratedKeys(sStmt, 102);
        assertEquals(1, sStmt.executeUpdate(INSERT_SQL_WITH_COLNAMES, new int[] { 1 }));
        assertGeneratedKeys(sStmt, 3);
        assertEquals(1, sStmt.executeUpdate(INSERT_SQL_WITH_COLNAMES, new int[] { 2 }));
        assertGeneratedKeys(sStmt, 104);
        assertEquals(1, sStmt.executeUpdate(INSERT_SQL, new int[] { 1, 2 }));
        assertGeneratedKeys(sStmt, 5, 105);
        try
        {
            sStmt.executeUpdate(INSERT_SQL, new int[] { 3 });
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.INVALID_COLUMN_INDEX, sEx.getErrorCode());
        }
        sStmt.close();
    }

    public void testGetGeneratedKeysWithColumnIndexesUsingPreparedStatement() throws SQLException
    {
        PreparedStatement sStmt;
        sStmt = connection().prepareStatement(INSERT_SQL, new int[] { 1 });
        assertEquals(1, sStmt.executeUpdate());
        assertGeneratedKeys(sStmt, 1);
        sStmt = connection().prepareStatement(INSERT_SQL, new int[] { 2 });
        assertEquals(1, sStmt.executeUpdate());
        assertGeneratedKeys(sStmt, 102);
        sStmt = connection().prepareStatement(INSERT_SQL_WITH_COLNAMES, new int[] { 1 });
        assertEquals(1, sStmt.executeUpdate());
        assertGeneratedKeys(sStmt, 3);
        sStmt = connection().prepareStatement(INSERT_SQL_WITH_COLNAMES, new int[] { 2 });
        assertEquals(1, sStmt.executeUpdate());
        assertGeneratedKeys(sStmt, 104);
        sStmt.close();
    }

    public void testGetGeneratedKeysWithColumnNames() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        try
        {
            sStmt.executeUpdate(INSERT_SQL, new String[] { "id" });
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.INVALID_COLUMN_NAME, sEx.getErrorCode());
            assertEquals("id", sEx.getMessage().substring(sEx.getMessage().indexOf(": ") + 2));
        }
        try
        {
            sStmt.executeUpdate(INSERT_SQL, new String[] { "val" });
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.INVALID_COLUMN_NAME, sEx.getErrorCode());
            assertEquals("val", sEx.getMessage().substring(sEx.getMessage().indexOf(": ") + 2));
        }
        assertEquals(1, sStmt.executeUpdate(INSERT_SQL_WITH_COLNAMES, new String[] { "id" }));
        assertGeneratedKeys(sStmt, 1);
        assertEquals(1, sStmt.executeUpdate(INSERT_SQL_WITH_COLNAMES, new String[] { "val" }));
        assertGeneratedKeys(sStmt, 102);
        sStmt.close();
    }

    public void testGetGeneratedKeysWithColumnNamesUsingPreparedStatement() throws SQLException
    {
        try
        {
            connection().prepareStatement(INSERT_SQL, new String[] { "id" });
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.INVALID_COLUMN_NAME, sEx.getErrorCode());
            assertEquals("id", sEx.getMessage().substring(sEx.getMessage().indexOf(": ") + 2));
        }
        try
        {
            connection().prepareStatement(INSERT_SQL, new String[] { "val" });
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.INVALID_COLUMN_NAME, sEx.getErrorCode());
            assertEquals("val", sEx.getMessage().substring(sEx.getMessage().indexOf(": ") + 2));
        }
        PreparedStatement sStmt = connection().prepareStatement(INSERT_SQL_WITH_COLNAMES, new String[] { "id" });
        assertEquals(1, sStmt.executeUpdate());
        assertGeneratedKeys(sStmt, 1);
        sStmt = connection().prepareStatement(INSERT_SQL_WITH_COLNAMES, new String[] { "val" });
        assertEquals(1, sStmt.executeUpdate());
        assertGeneratedKeys(sStmt, 102);
        sStmt.close();
    }

    public void testInvalidTestForINSERT() throws SQLException
    {
        testInvalid(ErrorDef.INVALID_QUERY_STRING, "INSERT INTO t1 VALUES (0, 1, 2)", false);
    }

    public void testInvalidTestForSELECT() throws SQLException
    {
        testInvalid(ErrorDef.INVALID_QUERY_STRING, "SELECT * FROM t1", false);
    }

    private void testInvalid(int aExpErrorCode, String aQstr, boolean aCatchEx) throws SQLException
    {
        ResultSet sRS;

        Statement sStmt = connection().createStatement();
        sRS = sStmt.getGeneratedKeys();
        assertEquals(false, sRS.next());
        if (aCatchEx)
        {
            try
            {
                sStmt.execute(aQstr, Statement.RETURN_GENERATED_KEYS);
                fail();
            }
            catch (SQLException sEx)
            {
                assertEquals(ErrorDef.INVALID_QUERY_STRING, sEx.getErrorCode());
            }
        }
        else
        {
            sStmt.execute(aQstr, Statement.RETURN_GENERATED_KEYS);
        }
        sRS = sStmt.getGeneratedKeys();
        assertEquals(false, sRS.next());
        sStmt.close();

        if (aCatchEx)
        {
            try
            {
                connection().prepareStatement(aQstr, Statement.RETURN_GENERATED_KEYS);
                fail();
            }
            catch (SQLException sEx)
            {
                assertEquals(ErrorDef.INVALID_QUERY_STRING, sEx.getErrorCode());
            }
        }
        else
        {
            PreparedStatement sPStmt = connection().prepareStatement(aQstr, Statement.RETURN_GENERATED_KEYS);
            sRS = sPStmt.getGeneratedKeys();
            assertEquals(false, sRS.next());
            sPStmt.execute();
            sRS = sPStmt.getGeneratedKeys();
            assertEquals(false, sRS.next());
            sPStmt.close();
        }
    }
}
