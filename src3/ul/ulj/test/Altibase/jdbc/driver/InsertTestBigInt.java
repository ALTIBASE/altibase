package Altibase.jdbc.driver;

import java.sql.PreparedStatement;

public class InsertTestBigInt extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 BIGINT)"
        };
    }

    public void testInsertBigInt() throws Exception
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setLong(1, Long.MAX_VALUE);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(String.valueOf(Long.MAX_VALUE));

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setLong(1, Long.MIN_VALUE - 1);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(String.valueOf(Long.MIN_VALUE - 1));

        sStmt.setLong(1, Long.MIN_VALUE);
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
