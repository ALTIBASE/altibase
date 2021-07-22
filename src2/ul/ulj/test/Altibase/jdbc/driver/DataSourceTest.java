package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;
import junit.framework.TestCase;

import javax.sql.ConnectionPoolDataSource;
import javax.sql.PooledConnection;
import java.sql.Connection;
import java.sql.SQLException;
import java.sql.Statement;

public class DataSourceTest extends TestCase
{
    public void testPooledConnection() throws SQLException
    {
        ConnectionPoolDataSource sDS = new AltibaseConnectionPoolDataSource("newjdbc_unittest");
        PooledConnection sPConn = sDS.getPooledConnection();
        Connection sConn1 = sPConn.getConnection();
        sConn1.close();
        assertEquals(true, sConn1.isClosed());
        Connection sConn2 = sPConn.getConnection();
        Connection sConn2pc = ((AltibaseLogicalConnection)sConn2).mPhysicalConnection;
        Connection sConn3 = sPConn.getConnection();
        // 새로운 logical connection을 얻으면 이전것은 closed
        try
        {
            sConn2.createStatement();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CLOSED_CONNECTION, sEx.getErrorCode());
        }
        Statement sStmt3 = sConn3.createStatement();
        sStmt3.close();

        // physical connection은 동일
        assertEquals(sConn2pc,((AltibaseLogicalConnection)sConn3).mPhysicalConnection);

        sPConn.close();
    }
}
