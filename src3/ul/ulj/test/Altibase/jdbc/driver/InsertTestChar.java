package Altibase.jdbc.driver;

import java.sql.PreparedStatement;

public class InsertTestChar extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 CHAR(10))"
        };
    }

    public void testInsertChar() throws Exception
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setString(1, "asd");
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("asd       ");

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setString(1, "123");
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("123       ");

        sStmt.setString(1, null);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.close();
    }
}
