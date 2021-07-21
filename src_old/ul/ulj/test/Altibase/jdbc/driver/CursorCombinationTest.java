package Altibase.jdbc.driver;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.CallableStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.SQLWarning;


public class CursorCombinationTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE TEST4RESULTSET",
                "DROP TYPESET p1_type",
                "DROP PROCEDURE p1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE TEST4RESULTSET ( C1 VARCHAR(10), C2 VARCHAR(10))",
                "INSERT INTO TEST4RESULTSET VALUES ('1', '1')",
                "INSERT INTO TEST4RESULTSET VALUES ('2', '2')",
                "INSERT INTO TEST4RESULTSET VALUES ('3', '3')",
                "INSERT INTO TEST4RESULTSET VALUES ('4', '4')",
                "INSERT INTO TEST4RESULTSET VALUES ('5', '5')",
                "CREATE TYPESET p1_type"
                + " AS "
                        + "TYPE cur_type IS REF CURSOR;"
                + " END",
                "CREATE PROCEDURE p1 (cur OUT p1_type.cur_type)"
                + " AS"
                + " BEGIN "
                    + "OPEN cur FOR 'SELECT * FROM TEST4RESULTSET';"
                + " END"
        };
    }

    private void assertResultSet(ResultSet aRS) throws SQLException
    {
        for (int i = 1; i <= 5; i++)
        {
            assertEquals(true, aRS.next());
            assertEquals(String.valueOf(i), aRS.getString(1));
            assertEquals(String.valueOf(i), aRS.getString(2));
        }
    }

    // 3 * 2 * 2 = 12 cases
    public void testCursorComb1() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        sStmt.setFetchSize(1);
        
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();

        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        int count = 0;
        
        assertEquals(rs.getMetaData().getColumnCount(), 2);
        
        while(rs.next())
        {
            count++;
        }
        
        assertEquals(5, count);
        
        sStmt.execute("SELECT C1 FROM TEST4RESULTSET");
        rs = sStmt.getResultSet();
        
        assertEquals(rs.getMetaData().getColumnCount(), 1);
        
        try{
            while(rs.next())
            {
                count++;
            }
        }
        catch (Exception e) {
            fail();
        }
        
        rs.close();
        sStmt.close();
    }
    
    public void testCursorComb2() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        sStmt.setFetchSize(1);
        
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();

        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        int count = 0;
        
        assertEquals(rs.getMetaData().getColumnCount(), 2);
        
        while(rs.next())
        {
            count++;
        }
        
        assertEquals(5, count);
        
        sStmt.execute("SELECT C1 FROM TEST4RESULTSET");
        rs = sStmt.getResultSet();
        
        assertEquals(rs.getMetaData().getColumnCount(), 1);
        
        try{
            while(rs.next())
            {
                count++;
            }
        }
        catch (Exception e) {
            fail();
        }
        
        rs.close();
        sStmt.close();
    }
    
    public void testCursorComb3() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt.setFetchSize(1);
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        
        int count = 0;

        assertEquals(rs.getMetaData().getColumnCount(), 2);
        
        while(rs.next())
        {
            count++;
            rs.updateString(1, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
            rs.updateString(2, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
            rs.updateRow();
        }
        rs.close();
        sStmt.close(); // BUG-44466 statement를 CONCUR_UPDATABLE로 열었기때문에 close를 하지 않으면 tearDown에서 several statements still open 에러가 발생한다.
        connection().commit();

        // With refresh
        assertEquals(5, count);
        count = 0;
        sStmt = (AltibaseStatement)connection().createStatement();
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        rs = sStmt.getResultSet();

        assertEquals(rs.getMetaData().getColumnCount(), 2);

        while(rs.next())
        {
            count++;
            assertEquals(Integer.parseInt(rs.getString(1)), count+1);
            assertEquals(Integer.parseInt(rs.getString(2)), count+1);
        }

        try{
            rs.beforeFirst();
            fail();
        }
        catch(Exception e)
        {

        }

        try{
            rs.refreshRow();
            fail();
        }
        catch(Exception e)
        {

        }

        assertEquals(5, count);
        rs.close();
        sStmt.close();
    }

    public void testCursorComb4() throws Exception  
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        sStmt.close();
    }

    public void testCursorComb5() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt.setFetchSize(1);
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        
        int count = 0;

        assertEquals(rs.getMetaData().getColumnCount(), 2);
                
        while(rs.next())
        {
            count++;
            try {
                rs.updateString(1, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
                rs.updateString(2, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
                rs.updateRow();
                fail();
            }
            catch (Exception e)
            {
            }
        }
        
        // With refresh
        assertEquals(5, count);
        rs.beforeFirst();

        rs.refreshRow();

        count = 0;
        while(rs.next())
        {
            count++;
            assertEquals(Integer.parseInt(rs.getString(1)), count);
            assertEquals(Integer.parseInt(rs.getString(2)), count);
        }
        
        // No refresh
        assertEquals(5, count);
        rs.beforeFirst();
        count = 0;
        
        while(rs.next())
        {
            count++;
            assertEquals(Integer.parseInt(rs.getString(1)), count);
            assertEquals(Integer.parseInt(rs.getString(2)), count);
        }
        assertEquals(5, count);
        rs.close();
        sStmt.close();
    }
    
    public void testCursorComb6() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
    }
    
    public void testCursorComb7() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt.setFetchSize(1);
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        
        int count = 0;

        assertEquals(rs.getMetaData().getColumnCount(), 2);
        
        while(rs.next())
        {
            count++;
            rs.updateString(1, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
            rs.updateString(2, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
            rs.updateRow();
        }
        
        connection().commit();
        
        // With refresh
        assertEquals(5, count);
        rs.beforeFirst();
        
        rs.refreshRow();
        
        count = 0;
        
        while(rs.next())
        {
            count++;
            assertEquals(Integer.parseInt(rs.getString(1)), count+1);
            assertEquals(Integer.parseInt(rs.getString(2)), count+1);
        }
        
        // No refresh
        assertEquals(5, count);
        rs.beforeFirst();
        count = 0;
        
        AltibaseStatement sStmt4ExternalUpdate = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt4ExternalUpdate.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs4ExternalUpdate = sStmt4ExternalUpdate.getResultSet();
        
        while(rs4ExternalUpdate.next())
        {
            rs4ExternalUpdate.updateString(1, "a");
            rs4ExternalUpdate.updateString(2, "a");
            rs4ExternalUpdate.updateRow();
        }
        
        connection().commit();
        
        while(rs.next())
        {
            count++;
            assertEquals(Integer.parseInt(rs.getString(1)), count+1);
            assertEquals(Integer.parseInt(rs.getString(2)), count+1);
        }
        assertEquals(5, count);
        
        count = 0;
        rs4ExternalUpdate.beforeFirst();
        while(rs4ExternalUpdate.next())
        {
            count++;
            assertEquals("a", rs4ExternalUpdate.getString(1));
            assertEquals("a", rs4ExternalUpdate.getString(2));
        }
        
        assertEquals(5, count);
        
        rs4ExternalUpdate.close();
        sStmt4ExternalUpdate.close();
        rs.close();
        sStmt.close();
    }
    
    public void testCursorComb8() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());

        rs.close();
        sStmt.close();
    }
    
    public void testCursorComb9() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt.setFetchSize(1);
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        assertEquals(ResultSet.TYPE_SCROLL_SENSITIVE, sStmt.getResultSetType());
        assertEquals(ResultSet.CONCUR_READ_ONLY, sStmt.getResultSetConcurrency());
        assertEquals(ResultSet.CLOSE_CURSORS_AT_COMMIT, sStmt.getResultSetHoldability());

        ResultSet rs = sStmt.getResultSet();
        assertEquals(ResultSet.TYPE_SCROLL_SENSITIVE, rs.getType());
        assertEquals(ResultSet.CONCUR_READ_ONLY, rs.getConcurrency());
        assertEquals(ResultSet.CLOSE_CURSORS_AT_COMMIT, ((AltibaseResultSet)rs).getHoldability());
        
        int count = 0;

        assertEquals(rs.getMetaData().getColumnCount(), 2);
        
        while(rs.next())
        {
            count++;
            try {
                rs.updateString(1, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
                rs.updateString(2, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
                rs.updateRow();
                fail();
            }
            catch(Exception e)
            {
                
            }
        }
        
        connection().commit();
        
        AltibaseStatement sStmt4ExternalUpdate = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt4ExternalUpdate.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs4ExternalUpdate = sStmt4ExternalUpdate.getResultSet();
        
        while(rs4ExternalUpdate.next())
        {
            rs4ExternalUpdate.updateString(1, "a");
            rs4ExternalUpdate.updateString(2, "a");
            rs4ExternalUpdate.updateRow();
        }
        
        connection().commit();
        
        count = 0;
        rs4ExternalUpdate.beforeFirst();
        while(rs4ExternalUpdate.next())
        {
            count++;
            assertEquals("a", rs4ExternalUpdate.getString(1));
            assertEquals("a", rs4ExternalUpdate.getString(2));
        }
        
        assertEquals(5, count);
        
        // No refresh
        assertEquals(5, count);
        rs.beforeFirst();
        count = 0;
        
        while(rs.next())
        {
            count++;
            assertEquals("a", rs.getString(1));
            assertEquals("a", rs.getString(2));
        }
        assertEquals(5, count);
        
        // With refresh
        assertEquals(5, count);
        rs.beforeFirst();
        
        count = 0;
        
        while(rs.next())
        {
            rs.refreshRow();
            count++;
            assertEquals("a", rs.getString(1));
            assertEquals("a", rs.getString(2));
        }
        
        rs4ExternalUpdate.close();
        sStmt4ExternalUpdate.close();
        rs.close();
        sStmt.close();
    }
    
    public void testCursorComb10() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        sStmt.close();
    }
    
    public void testCursorComb11() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt.setFetchSize(1);
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        
        int count = 0;

        assertEquals(rs.getMetaData().getColumnCount(), 2);
        
        while(rs.next())
        {
            count++;
            try {
                rs.updateString(1, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
                rs.updateString(2, String.valueOf(Integer.parseInt(rs.getString(2)) + 1));
                rs.updateRow();
            }
            catch(Exception e)
            {
                fail();
            }
        }
        
        connection().commit();
        
        // No refresh
        assertEquals(5, count);
        rs.beforeFirst();
        count = 0;
        
        while(rs.next())
        {
            count++;
            assertEquals(count+1, Integer.parseInt(rs.getString(1)));
            assertEquals(count+1, Integer.parseInt(rs.getString(2)));
        }
        assertEquals(5, count);
        
        // With refresh
        rs.beforeFirst();
        
        count = 0;
        
        while(rs.next())
        {
            rs.refreshRow();
            count++;
            assertEquals(count+1, Integer.parseInt(rs.getString(1)));
            assertEquals(count+1, Integer.parseInt(rs.getString(2)));
        }
        
        assertEquals(5, count);
        
        AltibaseStatement sStmt4ExternalUpdate = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        
        sStmt4ExternalUpdate.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs4ExternalUpdate = sStmt4ExternalUpdate.getResultSet();
        
        while(rs4ExternalUpdate.next())
        {
            rs4ExternalUpdate.updateString(1, "a");
            rs4ExternalUpdate.updateString(2, "a");
            rs4ExternalUpdate.updateRow();
        }
        
        connection().commit();
        
        count = 0;
        rs4ExternalUpdate.beforeFirst();
        while(rs4ExternalUpdate.next())
        {
            count++;
            assertEquals("a", rs4ExternalUpdate.getString(1));
            assertEquals("a", rs4ExternalUpdate.getString(2));
        }
        
        assertEquals(5, count);
        
        // No refresh
        assertEquals(5, count);
        rs.beforeFirst();
        count = 0;
        
        while(rs.next())
        {
            count++;
            assertEquals("a", rs.getString(1));
            assertEquals("a", rs.getString(2));
        }
        assertEquals(5, count);
        
        // With refresh
        assertEquals(5, count);
        rs.beforeFirst();
        
        count = 0;
        
        while(rs.next())
        {
            rs.refreshRow();
            count++;
            assertEquals("a", rs.getString(1));
            assertEquals("a", rs.getString(2));
        }
        
        rs4ExternalUpdate.close();
        sStmt4ExternalUpdate.close();
        rs.close();
        sStmt.close();
    }
    
    public void testCursorComb12() throws Exception
    {
        AltibaseStatement sStmt = (AltibaseStatement) connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        
        sStmt.execute("SELECT C1, C2 FROM TEST4RESULTSET");
        ResultSet rs = sStmt.getResultSet();
        
        assertEquals(sStmt.getResultSetType(), rs.getType());
        assertEquals(sStmt.getResultSetConcurrency(), rs.getConcurrency());
        assertEquals(sStmt.getResultSetHoldability(), ((AltibaseResultSet)rs).getHoldability());
        
        try{
            sStmt.execute("DROP TABLE TEST4RESULTSET");
        }
        catch(Exception e)
        {
            fail();
        }
    }

    public void testDowngradeForProcedure() throws SQLException
    {
        CallableStatement sStmt = connection().prepareCall("EXEC p1", ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        SQLWarning sWarn = sStmt.getWarnings();
        assertEquals(ErrorDef.OPTION_VALUE_CHANGED, sWarn.getErrorCode());

        sStmt.execute();
        ResultSet sRS = sStmt.getResultSet();
        assertResultSet(sRS);
    }
}
