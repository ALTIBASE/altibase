package Altibase.jdbc.driver;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import javax.sql.XAConnection;
import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import Altibase.jdbc.driver.ex.ErrorDef;

public class XATest extends AltibaseTestCase
{
    private AltibaseXADataSource mXADataSource;
    private XAConnection         mXAConnection;
    private XAResource           mXAResource;

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP TABLE accountFrom",
                "DROP TABLE accountTo",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 INTEGER)",
                "CREATE TABLE accountFrom (accountno INTEGER PRIMARY KEY, balance INTEGER)",
                "INSERT INTO accountFrom VALUES (1000, 40000)",
                "CREATE TABLE accountTo (accountno INTEGER PRIMARY KEY, balance INTEGER)",
        };
    }

    protected void setUp() throws Exception
    {
        super.setUp();

        mXADataSource = new AltibaseXADataSource();
        mXADataSource.setURL(CONN_URL);
        mXADataSource.setUser(CONN_USER);
        mXADataSource.setPassword(CONN_PWD);
        mXAConnection = mXADataSource.getXAConnection(CONN_USER, CONN_PWD);
        mXAResource = xaConnection().getXAResource();
    }

    protected void tearDown() throws Exception
    {
        try
        {
            if (mXAConnection != null)
            {
                mXAConnection.close();
                mXAConnection = null;
            }
        }
        catch (SQLException sEx)
        {
            // quite
        }

        super.tearDown();
    }

    private XAConnection xaConnection()
    {
        return mXAConnection;
    }

    private XAResource xaResource()
    {
        return mXAResource;
    }

    static Xid createXid(int gd, int bd) throws XAException
    {
        byte[] gtrid = new byte[4];
        byte[] bqual = new byte[4];
        gtrid[0] = (byte)(gd >>> 24);
        gtrid[1] = (byte)(gd >>> 16);
        gtrid[2] = (byte)(gd >>> 8);
        gtrid[3] = (byte)(gd >>> 0);
        bqual[0] = (byte)(bd >>> 24);
        bqual[1] = (byte)(bd >>> 16);
        bqual[2] = (byte)(bd >>> 8);
        bqual[3] = (byte)(bd >>> 0);

        return new AltibaseXid(0x0L, gtrid, bqual);
    }

    public void testCommit() throws SQLException, XAException
    {
        Connection sConn = xaConnection().getConnection();
        Xid sXid = createXid('B', 'A');
        xaResource().start(sXid, XAResource.TMNOFLAGS);
        Statement sInsStmt = sConn.createStatement();
        assertEquals(1, sInsStmt.executeUpdate("INSERT INTO t1 VALUES (4321)"));
        sInsStmt.close();
        xaResource().end(sXid, XAResource.TMSUCCESS);
        assertEquals(0, xaResource().prepare(sXid));
        xaResource().commit(sXid, false);
        sConn.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(4321, sRS.getInt(1));
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
    }

    public void testRollback() throws SQLException, XAException
    {
        Connection sConn = xaConnection().getConnection();
        Xid sXid = createXid('B', 'A');
        xaResource().start(sXid, XAResource.TMNOFLAGS);
        Statement sInsStmt = sConn.createStatement();
        assertEquals(1, sInsStmt.executeUpdate("INSERT INTO t1 VALUES (4321)"));
        sInsStmt.close();
        xaResource().end(sXid, XAResource.TMSUCCESS);
        assertEquals(0, xaResource().prepare(sXid));
        xaResource().rollback(sXid);
        sConn.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
    }

    public void testRecover() throws SQLException, XAException
    {
        Connection sConn = xaConnection().getConnection();
        Xid sXid = createXid('B', 'A');
        xaResource().start(sXid, XAResource.TMNOFLAGS);
        Statement sInsStmt = sConn.createStatement();
        assertEquals(1, sInsStmt.executeUpdate("INSERT INTO t1 VALUES (4321)"));
        sInsStmt.close();
        xaResource().end(sXid, XAResource.TMSUCCESS);
        assertEquals(0, xaResource().prepare(sXid));
        Xid[] sResultXids = xaResource().recover(XAResource.TMSTARTRSCAN);
        assertEquals(1, sResultXids.length);
        assertEquals(sXid, sResultXids[0]);
        xaResource().rollback(sXid);
        sConn.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(false, sRS.next());
        sRS.close();
        sSelStmt.close();
    }

    public void testExNOTA() throws SQLException
    {
        XAResource sXaRes = xaConnection().getXAResource();
        Xid xid = new AltibaseXid(0x1L, null, null);
        try
        {
            sXaRes.start(xid, XAResource.TMNOFLAGS);
            fail();
        }
        catch (XAException sEx)
        {
            assertEquals(XAException.XAER_NOTA, sEx.errorCode);
            assertEquals(ErrorDef.getXAErrorMessage(XAException.XAER_NOTA), sEx.getMessage());
        }
    }
}
