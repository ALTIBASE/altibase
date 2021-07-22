package Altibase.jdbc.driver;

import java.sql.SQLException;
import java.sql.Statement;

import Altibase.jdbc.driver.ex.ErrorDef;

public class StatementTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[0];
    }

    protected String[] getInitQueries()
    {
        return new String[0];
    }

    public void testFree() throws Exception
    {
        Statement sStmt = connection().createStatement();

        AltibaseTestCase.assertExecuteScalar(sStmt, "1", "Count(*)", "V$STATEMENT");
        Statement sTmpStmt1 = connection().prepareStatement("SELECT 1 FROM DUAL");
        AltibaseTestCase.assertExecuteScalar(sStmt, "2", "Count(*)", "V$STATEMENT");
        Statement sTmpStmt2 = connection().prepareStatement("SELECT 1 FROM DUAL");
        AltibaseTestCase.assertExecuteScalar(sStmt, "3", "Count(*)", "V$STATEMENT");
        sTmpStmt2.close();
        AltibaseTestCase.assertExecuteScalar(sStmt, "2", "Count(*)", "V$STATEMENT");
        sTmpStmt1.close();
        AltibaseTestCase.assertExecuteScalar(sStmt, "1", "Count(*)", "V$STATEMENT");

        sStmt.close();
    }

    public void testMaxStmt() throws SQLException
    {
        // 최대 65536개까지 만들 수 있지만, Connection에서 내부 처리용으로 1개 쓰므로 65535까지만 만들 수 있다.
        Statement[] sStmt = new Statement[65535];
        for (int i=0; i<sStmt.length; i++)
        {
            sStmt[i] = connection().createStatement();
        }
        try
        {
            connection().createStatement();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.TOO_MANY_STATEMENTS, sEx.getErrorCode());
        }

        // 닫고 만들면 만들어져야한다. (사용중인 CID셋이 제대로 정리되는지 확인)
        sStmt[0].close();
        sStmt[0] = connection().createStatement();
    }
}
