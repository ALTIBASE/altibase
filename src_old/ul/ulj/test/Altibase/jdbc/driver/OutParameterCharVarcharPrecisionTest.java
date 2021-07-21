package Altibase.jdbc.driver;

import java.sql.CallableStatement;
import java.sql.SQLException;
import java.sql.Types;

public class OutParameterCharVarcharPrecisionTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP PROCEDURE TTT1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE OR REPLACE PROCEDURE TTT1("
                    + "PRESULT1 OUT VARCHAR(6),"
                    + "PRESULT2 OUT CHAR(5)"
                + ")"
                + " AS BEGIN "
                    + "PRESULT1 := 'ABC';"
                    + "PRESULT2 := 'DE';"
                + " END"
        };
    }

    private static int[][] TYPES_PAIR = {
        {Types.VARCHAR, Types.VARCHAR},
        {Types.VARCHAR, Types.CHAR   },
        {Types.CHAR   , Types.CHAR   },
    };

    public void testBUG_27818() throws SQLException, InterruptedException
    {
        CallableStatement sCallStmt = connection().prepareCall("EXEC TTT1(?, ?)");
        for (int i = 0; i < TYPES_PAIR.length; i++)
        {
            sCallStmt.registerOutParameter(1, TYPES_PAIR[i][0]);
            sCallStmt.registerOutParameter(2, TYPES_PAIR[i][1]);
            assertEquals(false, sCallStmt.execute());
            assertEquals("ABC", sCallStmt.getString(1));
            assertEquals("DE   ", sCallStmt.getString(2));
        }
        sCallStmt.close();
    }
}
