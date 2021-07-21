package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltibaseProperties;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class ConnectionTest extends AltibaseTestCase
{
    /**
     * 접속에 실패하면 예외가 떨어지는것을 확인
     */
    public void testConnectionFail_Port()
    {
        try
        {
            // 접속 실패하게 URL 설정. ..설마 서버를 80 port로 띄우진 않겠지;
            AltibaseConnection sConn = (AltibaseConnection)getConnection("jdbc:Altibase://localhost:80/mydb");
            fail("Connection object created : " + sConn.getURL());
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.COMMUNICATION_ERROR, sEx.getErrorCode());
        }
    }

    /**
     * 엉뚱한 DB Name을 쓰면 접속 실패
     */
    public void testConnectionFail_DbName()
    {
        AltibaseDataSource sDS = AltibaseDataSourceManager.getInstance().getDataSource(CONN_DSN);
        try
        {
            AltibaseConnection sConn = (AltibaseConnection)getConnection("jdbc:Altibase://" + sDS.getServerName() + ":" + sDS.getPortNumber() + "/no" + sDS.getDatabaseName());
            fail("Connection object created : " + sConn.getURL());
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.DB_NOT_FOUND, sEx.getErrorCode());
        }
    }

    /**
     * connect하면 session이 늘어나고, disconnect하면 session이 줄어드는것을 확인 
     *
     * @throws SQLException DB 접속이나 쿼리 수행에 실패한 경우
     */
    public void testDisconnect() throws SQLException
    {
        Connection sConn = connection();
        Statement sStmt = sConn.createStatement();

        AltibaseTestCase.assertExecuteScalar(sStmt, "1", "Count(*)", "X$SESSION");
        Connection sTmpConn1 = getConnection();
        AltibaseTestCase.assertExecuteScalar(sStmt, "2", "Count(*)", "X$SESSION");
        Connection sTmpConn2 = getConnection();
        AltibaseTestCase.assertExecuteScalar(sStmt, "3", "Count(*)", "X$SESSION");
        Connection sTmpConn3 = getConnection();
        AltibaseTestCase.assertExecuteScalar(sStmt, "4", "Count(*)", "X$SESSION");
        sTmpConn3.close();
        AltibaseTestCase.assertExecuteScalar(sStmt, "3", "Count(*)", "X$SESSION");
        sTmpConn2.close();
        AltibaseTestCase.assertExecuteScalar(sStmt, "2", "Count(*)", "X$SESSION");
        sTmpConn1.close();
        AltibaseTestCase.assertExecuteScalar(sStmt, "1", "Count(*)", "X$SESSION");

        sStmt.close();
    }

    public void testConnectByDSN() throws SQLException
    {
        AltibaseProperties sProp = new AltibaseProperties();
        sProp.setDataSource(AltibaseTestCase.CONN_DSN);
        AltibaseConnection sConn = new AltibaseConnection(sProp, null);
        Statement sStmt = sConn.createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT 1 FROM DUAL");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
        sConn.close();
    }

    public void testURL() throws SQLException
    {
        getConnection("jdbc:Altibase://" + AltibaseTestCase.CONN_DSN);
        getConnection("jdbc:Altibase_" + AltibaseVersion.CM_VERSION_STRING + "://" + AltibaseTestCase.CONN_DSN);
        try
        {
            getConnection("jdbc:Altibase4://" + AltibaseTestCase.CONN_DSN);
        }
        catch (SQLException sEx)
        {
            assertTrue(sEx.getMessage().startsWith("No suitable driver"));
        }
    }

    public void testDefaultHoldability() throws SQLException
    {
        assertEquals(ResultSet.CLOSE_CURSORS_AT_COMMIT, connection().getHoldability());
        assertHoldability(ResultSet.CLOSE_CURSORS_AT_COMMIT, connection());

        connection().setHoldability(ResultSet.HOLD_CURSORS_OVER_COMMIT);
        assertHoldability(ResultSet.HOLD_CURSORS_OVER_COMMIT, connection());

        try
        {
            connection().setHoldability(999); // invalid valid
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.INVALID_ARGUMENT, sEx.getErrorCode());
        }

        assertHoldability(ResultSet.HOLD_CURSORS_OVER_COMMIT, connection());
    }

    private void assertHoldability(int aExpHoldability, Connection aConn) throws SQLException
    {
        assertEquals(aExpHoldability, aConn.getHoldability());
        Statement sStmt = connection().createStatement();
        assertEquals(aExpHoldability, sStmt.getResultSetHoldability());
        sStmt.close();
    }
}
