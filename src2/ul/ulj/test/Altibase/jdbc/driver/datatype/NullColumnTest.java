package Altibase.jdbc.driver.datatype;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.AltibaseTestCase;

public class NullColumnTest extends AltibaseTestCase
{
    public void testSelectNull() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT null FROM dual");
        assertEquals(true, sRS.next());
        assertEquals(null, sRS.getObject(1));
        sRS.close();
        sStmt.close();
    }
}
