package Altibase.jdbc.driver.datatype;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.AltibaseTestCase;

public class SetNullTest2 extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 INT, c2 VARCHAR(20))",
        };
    }

    // natc : TC/Interface/newJDBC/Bugs/BUG-24389/BUG-24389.sql
    public void testSetNull() throws SQLException
    {
        PreparedStatement sPstmt = connection().prepareStatement("INSERT INTO t1 VALUES (?, ?)");

        sPstmt.setObject(1,null,java.sql.Types.NULL);
        sPstmt.setObject(2,null,java.sql.Types.NULL);
        sPstmt.addBatch();

        sPstmt.setObject(1,null,java.sql.Types.NULL);
        sPstmt.setObject(2,null,java.sql.Types.NULL);
        sPstmt.addBatch();

        sPstmt.setObject(1,null,java.sql.Types.NULL);
        sPstmt.setInt(1, 1);
        sPstmt.setObject(2,null,java.sql.Types.NULL);
        sPstmt.setString(2, "kim");
        sPstmt.addBatch();

        sPstmt.setObject(1,null,java.sql.Types.NULL);
        sPstmt.setInt(1, 2);
        sPstmt.setObject(2,null,java.sql.Types.NULL);
        sPstmt.setString(2, "sung");
        sPstmt.addBatch();

        sPstmt.setObject(1,null,java.sql.Types.NULL);
        sPstmt.setInt(1, 3);
        sPstmt.setObject(2,null,java.sql.Types.NULL);
        sPstmt.setString(2, "jai");
        sPstmt.addBatch();

        sPstmt.setObject(1,null,java.sql.Types.NULL);
        sPstmt.setInt(1, 4);
        sPstmt.setObject(2,null,java.sql.Types.NULL);
        sPstmt.addBatch();

        int [] sResult = sPstmt.executeBatch();
        assertEquals(1, sResult[0]);
        assertEquals(1, sResult[1]);
        assertEquals(1, sResult[2]);
        assertEquals(1, sResult[3]);
        assertEquals(1, sResult[4]);
        assertEquals(1, sResult[5]);

        sPstmt.close();

        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1 ORDER BY c1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals("kim", sRS.getString(2));
        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt(1));
        assertEquals("sung", sRS.getString(2));
        assertEquals(true, sRS.next());
        assertEquals(3, sRS.getInt(1));
        assertEquals("jai", sRS.getString(2));
        assertEquals(true, sRS.next());
        assertEquals(4, sRS.getInt(1));
        assertEquals(null, sRS.getString(2));
        assertEquals(true, sRS.next());
        assertEquals(null, sRS.getObject(1));
        assertEquals(null, sRS.getObject(2));
        assertEquals(true, sRS.next());
        assertEquals(null, sRS.getObject(1));
        assertEquals(null, sRS.getObject(2));
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }
}
