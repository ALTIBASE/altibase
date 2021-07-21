package Altibase.jdbc.driver;

import java.sql.PreparedStatement;

public class InsertTestDouble extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 DOUBLE)"
        };
    }

    public void testInsertDouble() throws Exception
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setDouble(1, 11.11d);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("11.11");

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setDouble(1, 22.22d);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("22.22");

        sStmt.setDouble(1, Double.NaN);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.setObject(1, null);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.close();
    }
}
