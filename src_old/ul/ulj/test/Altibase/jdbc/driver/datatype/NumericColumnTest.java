package Altibase.jdbc.driver.datatype;

import Altibase.jdbc.driver.AltibaseTestCase;

import java.math.BigDecimal;
import java.sql.*;

public class NumericColumnTest extends AltibaseTestCase
{
    private static final float[]      FLOAT_VALUES      = { 123, 2, 1.23f, 12.3f, -123, -2, -1.23f, -12.3f };
    private static final double[]     DOUBLE_VALUES     = { 123, 2, 1.23d, 12.3d, -123, -2, -1.23d, -12.3d };
    private static final String[]     STRING_VALUES     = { "123", "2", "1.23", "12.3", "-123", "-2", "-1.23", "-12.3" };
    private static final BigDecimal[] BIGDECIMAL_VALUES = { BigDecimal.valueOf(123), BigDecimal.valueOf(2), BigDecimal.valueOf(123, 2), BigDecimal.valueOf(123, 1), BigDecimal.valueOf(-123), BigDecimal.valueOf(-2), BigDecimal.valueOf(-123, 2), BigDecimal.valueOf(-123, 1) };

    private static final int          NUMERIC_STRING    = -20001;

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 NUMBER(20,10))"
        };
    }

    public void testInsertByNumericString() throws Exception
    {
        testInsertBy(NUMERIC_STRING);
    }

    public void testInsertByString() throws Exception
    {
        testInsertBy(Types.CHAR);
    }

    public void testInsertByFloat() throws Exception
    {
        testInsertBy(Types.FLOAT);
    }

    public void testInsertByDouble() throws Exception
    {
        testInsertBy(Types.DOUBLE);
    }

    public void testInsertByDecimal() throws Exception
    {
        testInsertBy(Types.DECIMAL);
    }

    private void testInsertBy(int aJDBCType) throws Exception
    {
        PreparedStatement sStmt;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setObject(1, null);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(null);

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        for (int i = 0; i < FLOAT_VALUES.length; i++)
        {
            switch (aJDBCType)
            {
                case Types.FLOAT:
                    sStmt.setFloat(1, FLOAT_VALUES[i]);
                    break;
                case Types.DOUBLE:
                    sStmt.setDouble(1, DOUBLE_VALUES[i]);
                    break;
                case Types.DECIMAL:
                    sStmt.setBigDecimal(1, BIGDECIMAL_VALUES[i]);
                    break;
                case Types.CHAR:
                case Types.VARCHAR:
                    sStmt.setString(1, STRING_VALUES[i]);
                    break;
                case NUMERIC_STRING:
                    sStmt.setObject(1, STRING_VALUES[i], Types.NUMERIC);
                    break;
            }
            assertEquals(i + "th update", 1, sStmt.executeUpdate());
            assertExecuteScalar(STRING_VALUES[i]);
        }
        sStmt.close();
    }

    public void testGetZero() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate("INSERT INTO t1 VALUES (0)"));
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals("0", sRS.getString(1));
        assertEquals("0", sRS.getObject(1).toString());
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }

    public void testInsertByDecimalWithScale() throws SQLException
    {
        PreparedStatement sStmt;
        int sUpdateCount;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setObject(1, "1.234", Types.DECIMAL, 1); // scale ¹«½Ã
        sUpdateCount = sStmt.executeUpdate();
        assertEquals(1, sUpdateCount);
        assertExecuteScalar("1.234");
        sStmt.close();
    }
}
