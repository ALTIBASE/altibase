package Altibase.jdbc.driver;

import java.sql.CallableStatement;
import java.sql.SQLException;
import java.sql.Types;

public class OutParamNumDateTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP PROCEDURE PROC_IN_OUT",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE T1 ( I1 NUMERIC(38,16), I2 DATE DEFAULT TO_DATE('1996-OCT-03', 'RRRR-MON-DD'))",
                "CREATE PROCEDURE PROC_IN_OUT"
                +"("
                    +" P1 IN  NUMERIC(38,16),"
                    +" P2 OUT NUMERIC(38,16),"
                    +" P3 OUT DATE"
                +")"
                +"AS BEGIN"
                    +" INSERT INTO T1(I1) VALUES( P1 );"
                    +" SELECT I1 INTO P2 FROM T1 WHERE I1 = P1;"
                    +" SELECT I2 INTO P3 FROM T1 WHERE I1 = P1;"
                +"END",
        };
    }

    public void testOut() throws SQLException
    {
        CallableStatement sCallStmt = connection().prepareCall("EXEC PROC_IN_OUT(?, ?, ?)");
        sCallStmt.registerOutParameter(2, Types.NUMERIC);
        sCallStmt.registerOutParameter(3, Types.DATE);

        for (int i = 0; i < 4; i++)
        {
            double sDoubleValue = i * 111.111d;
            sCallStmt.setDouble(1, sDoubleValue);
            sCallStmt.execute();

            assertEquals(new Double(sDoubleValue), new Double(sCallStmt.getDouble(2)));
            assertEquals("1996-10-03", sCallStmt.getDate(3).toString());
        }
    }
}
