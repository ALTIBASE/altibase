package Altibase.jdbc.driver;

import java.sql.PreparedStatement;
import java.sql.Types;

public class InsertTestSmallInt extends AltibaseTestCase
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
                "CREATE TABLE t1 (c1 SMALLINT)"
        };
    }

    public void testInsertSmallInt() throws Exception
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setShort(1, (short) 11);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("11");

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setShort(1, (short) 123);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("123");

        sStmt.setShort(1, Short.MIN_VALUE);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.setString(1, null);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.close();
    }

    public void testInsertSmallIntBySmallInt() throws Exception
    {
        testInsertSmallIntBy(Types.SMALLINT);
    }

    public void testInsertSmallIntByTinyInt() throws Exception
    {
        testInsertSmallIntBy(Types.TINYINT);
    }

    public void testInsertSmallIntBy(int aJDBCType) throws Exception
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setObject(1, new Short((short)11), aJDBCType);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("11");

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setObject(1, new Short((short)123), aJDBCType);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("123");

        sStmt.setObject(1, new Short(Short.MIN_VALUE), aJDBCType);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.setObject(1, null, aJDBCType);
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar(null);

        sStmt.close();
    }
}
