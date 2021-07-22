package Altibase.jdbc.driver;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class BigDecimalTest extends AltibaseTestCase
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
                "create table t1 (c1 smallint, c2 integer, c3 real, c4 float, c5 numeric(10,2), c6 char(10), c7 varchar(10))",
                "insert into t1 values (1, 22, 3, 4, 5, ' 66 ', ' 77 ')",
                "insert into t1 values (2, 22, 3.3, 4.4, 5.5, '6.6', '7.7')",
                "insert into t1 values (3, 22, 0.3, 0.4, 0.5, '0.6', '0.7')",
                "insert into t1 values (null, 22, 3.0, 4.0, 5.0, '6.0', '7.0')",
                "insert into t1 values (5, null, 3.3, null, 5.5, null, '77')",
                "insert into t1 values (6, null, null, null, null, null, null)",
                "insert into t1 values (-7, -22, -3, -4, -5, '-66', '-77')",
                "insert into t1 values (-8, -22, -3.3, -4.4, -5.5, '-6.6', '-7.7')",
                "insert into t1 values (-9, -22, -0.3, -0.4, -0.5, '-0.6', '-0.7')",
                "insert into t1 values (-10, -22, -3.0, -4.0, -5.0, '-6.0', '-7.0')",
        };
    }

    public static final String[][] VALUES = {
        {"1", "22", "3", "4", "5", "66", "77"},
        {"2", "22", "3.3", "4.4", "5.5", "6.6", "7.7"},
        {"3", "22", "0.3", "0.4", "0.5", "0.6", "0.7"},
        {"null", "22", "3", "4", "5", "6.0", "7.0"},
        {"5", "null", "3.3", "null", "5.5", "null", "77"},
        {"6", "null", "null", "null", "null", "null", "null"},
        {"-7", "-22", "-3", "-4", "-5", "-66", "-77"},
        {"-8", "-22", "-3.3", "-4.4", "-5.5", "-6.6", "-7.7"},
        {"-9", "-22", "-0.3", "-0.4", "-0.5", "-0.6", "-0.7"},
        {"-10", "-22", "-3", "-4", "-5", "-6.0", "-7.0"},
    };

    public void testGetBigDecimal() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("select * from t1");
        for (int i = 0; i < VALUES.length; i++)
        {
            assertEquals(true, sRS.next());
            for (int j = 0; j < VALUES[i].length; j++)
            {
                assertEquals(VALUES[i][j], String.valueOf(sRS.getBigDecimal(j + 1)));
            }
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }
}
