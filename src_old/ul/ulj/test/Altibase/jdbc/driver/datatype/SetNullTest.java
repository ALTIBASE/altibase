package Altibase.jdbc.driver.datatype;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Types;

import Altibase.jdbc.driver.AltibaseTestCase;

public class SetNullTest extends AltibaseTestCase
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
                "CREATE TABLE T1 (I1_NUMERIC NUMERIC, I2_DECIMAL DECIMAL, I3_NUMBER NUMBER, I4_FLOAT FLOAT, I5_DOUBLE DOUBLE, I6_REAL REAL, I7_BIGINT BIGINT, I8_INTEGER INTEGER, I9_SMALLINT SMALLINT, I10_CHAR CHAR, I11_VARCHAR VARCHAR, I12_DATE DATE, I13_BYTE BYTE, I14_NIBBLE NIBBLE, I15_CLOB CLOB, I16_BLOB BLOB, I17_GEOMETRY GEOMETRY)",
        };
    }

    public void testSetNullAnsWasNull() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("INSERT INTO T1 VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        for (int i = 1; i <= sStmt.getParameterMetaData().getParameterCount(); i++)
        {
            sStmt.setNull(i, Types.NULL);
        }
        assertEquals(1, sStmt.executeUpdate());
        sStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        for (int i = 1; i <= 17; i++)
        {
            Object sVal = sRS.getObject(i);
            assertEquals(i + "th value", null, sVal);
            assertEquals(i + "th wasNull()", true, sRS.wasNull());
        }
    }

    public void testWasNotNull() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate("INSERT INTO T1 VALUES (1,1,1,1,1,1,1,1,1,1,1,sysdate,'1a','a','a','a',GEOMETRY'point (1 1)')"));

        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        for (int i = 1; i <= 17; i++)
        {
            Object sVal = sRS.getObject(i);
            assertNotNull(i + "th value", sVal);
            assertEquals(i + "th wasNull()", false, sRS.wasNull());
        }
    }
}
