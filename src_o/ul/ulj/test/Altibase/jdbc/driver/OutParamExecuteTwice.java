package Altibase.jdbc.driver;

import java.sql.CallableStatement;
import java.sql.SQLException;
import java.sql.Types;

public class OutParamExecuteTwice extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP PROCEDURE GET_DATA",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE T1 (I1 NUMBER(20,10), I2 INTEGER, I3 BIGINT, I4 REAL, I5 DOUBLE, I6 FLOAT)",
                "CREATE OR REPLACE PROCEDURE GET_DATA"
                +"("
                    +"P1 IN OUT DOUBLE,"
                    +"P2 OUT INTEGER,"
                    +"P3 OUT BIGINT,"
                    +"P4 OUT REAL,"
                    +"P5 OUT DOUBLE,"
                    +"P6 OUT FLOAT"
                +")"
                +"AS BEGIN "
                    +"SELECT I5, I2, I3, I4, I5, I6 INTO P1, P2, P3, P4, P5, P6 FROM T1 WHERE I1 = P1;"
                +"END",
                "INSERT INTO T1 VALUES ( 1, 2, 3, 4, 5, 6 )",
                "INSERT INTO T1 VALUES ( 2, NULL, NULL, NULL, NULL, NULL )",
        };
    }

    // from BUG-32521
    public void testOutExecuteTwice() throws SQLException
    {
        CallableStatement sCallStmt = connection().prepareCall("EXEC GET_DATA(?, ?, ?, ?, ?, ?) ");
        sCallStmt.registerOutParameter(1, Types.SMALLINT);
        sCallStmt.registerOutParameter(2, Types.INTEGER);
        sCallStmt.registerOutParameter(3, Types.NUMERIC);
        sCallStmt.registerOutParameter(4, Types.NUMERIC);
        sCallStmt.registerOutParameter(5, Types.BIGINT);
        sCallStmt.registerOutParameter(6, Types.SMALLINT);

        sCallStmt.setInt(1, 1);
        sCallStmt.execute();
        assertEquals(5, sCallStmt.getShort(1));
        assertEquals(2, sCallStmt.getInt(2));
        assertEquals(3, sCallStmt.getInt(3));
        assertEquals(4, sCallStmt.getInt(4));
        assertEquals(5, sCallStmt.getInt(5));
        assertEquals(6, sCallStmt.getInt(6));

        sCallStmt.setInt(1, 2);
        sCallStmt.execute();
        assertEquals(null, sCallStmt.getObject(1));
        assertEquals(null, sCallStmt.getObject(2));
        assertEquals(null, sCallStmt.getObject(3));
        assertEquals(null, sCallStmt.getObject(4));
        assertEquals(null, sCallStmt.getObject(5));
        assertEquals(null, sCallStmt.getObject(6));

        sCallStmt.close();
    }
}
