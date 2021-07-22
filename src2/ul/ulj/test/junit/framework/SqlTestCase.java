package junit.framework;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.util.ArrayList;

public abstract class SqlTestCase extends TestCase
{
    private static final String[] EMPTY_STRING_ARRAY = new String[] {};

    private Connection            mDefaultConn;
    private ArrayList             mConnList          = new ArrayList();

    protected String[] getCleanQueries()
    {
        return EMPTY_STRING_ARRAY;
    }

    protected String[] getInitQueries()
    {
        return EMPTY_STRING_ARRAY;
    }

    protected abstract void ensureLoadDriverClass() throws ClassNotFoundException;

    protected abstract String getURL();

    protected abstract String getUser();

    protected abstract String getPassword();

    protected boolean useConnectionPreserve()
    {
        return true;
    }

    private void addConnection(Connection aConn)
    {
        synchronized (mConnList)
        {
            mConnList.add(aConn);
        }
    }

    private void removeConnection(Connection aConn)
    {
        synchronized (mConnList)
        {
            mConnList.remove(aConn);
        }
    }

    private void clearConnection()
    {
        synchronized (mConnList)
        {
            for (int i = 0; i < mConnList.size(); i++)
            {
                try
                {
                    Connection sConn = (Connection)mConnList.get(i);
                    sConn.close();
                }
                catch (Exception sEx)
                {
                    // quite
                }
            }
            mConnList.clear();
        }
    }

    protected Connection getConnection() throws SQLException
    {
        Connection sConn = getConnection(getURL());
        addConnection(sConn);
        return sConn;
    }

    protected Connection getConnection(String aURL) throws SQLException
    {
        Connection sConn = getConnection(aURL, getUser(), getPassword());
        addConnection(sConn);
        return sConn;
    }

    protected Connection getConnection(String aURL, String aUser, String aPassword) throws SQLException
    {
        Connection sConn = DriverManager.getConnection(aURL, aUser, aPassword);
        addConnection(sConn);
        return sConn;
    }

    protected void setUp() throws Exception
    {
        ensureLoadDriverClass();

        try
        {
            int sExecCnt = getInitQueries().length + getCleanQueries().length;
            if (useConnectionPreserve() || sExecCnt > 0)
            {
                mDefaultConn = getConnection();
            }
            if (sExecCnt > 0)
            {
                Statement sStmt = (Statement)mDefaultConn.createStatement();
                cleanBy(sStmt);
                String[] sQstrs = getInitQueries();
                for (int i = 0; i < sQstrs.length; i++)
                {
                    sStmt.execute(sQstrs[i]);
                }
                sStmt.close();

                if (!useConnectionPreserve())
                {
                    removeConnection(mDefaultConn);

                    mDefaultConn.close();
                    mDefaultConn = null;
                }
            }
        }
        catch (Exception sEx)
        {
            clearConnection();
            throw sEx;
        }
    }

    protected void tearDown() throws Exception
    {
        try
        {
            if (getCleanQueries().length != 0)
            {
                if (!useConnectionPreserve())
                {
                    mDefaultConn = getConnection();
                }
                Statement sStmt = mDefaultConn.createStatement();
                cleanBy(sStmt);
                sStmt.close();
            }
        }
        finally
        {
            clearConnection();
        }
    }

    private void cleanBy(Statement aStmt) throws SQLException
    {
        String[] sQstrs = getCleanQueries();
        for (int i = 0; i < sQstrs.length; i++)
        {
            try
            {
                aStmt.execute(sQstrs[i]);
            }
            catch (SQLException ex)
            {
                if (!isIgnorableErrorForClean(ex.getErrorCode()))
                {
                    System.out.println("Error occurred while executing: " + sQstrs[i]);
                    printSQLException(ex, false);
                    throw ex;
                }
            }
        }
    }

    protected abstract boolean isIgnorableErrorForClean(int aErrorCode);

    protected Connection connection()
    {
        return mDefaultConn;
    }

    public void assertExecuteScalar(String aExpectedValue) throws SQLException
    {
        assertExecuteScalar(aExpectedValue, "c1");
    }

    public void assertExecuteScalar(String aExpectedValue, String aColumnName) throws SQLException
    {
        assertExecuteScalar(aExpectedValue, aColumnName, "t1");
    }

    public void assertExecuteScalar(String aExpectedValue, String aColumnName, String aTableName) throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertExecuteScalar(sStmt, aExpectedValue, aColumnName, aTableName);
        sStmt.close();
    }

    /**
     * 지정한 테이블의 컬럼을 가져와서, 첫번째 Row의 값이 기대값과 같은지 검증한다.
     * 
     * @param aStmt 쿼리를 수행할 Statement
     * @param aExpectedValue 기대값
     * @param aColumnName 값을 확인할 컬럼
     * @param aTableName 값을 가져올 테이블
     * @throws SQLException 쿼리에 실패한 경우
     * @throws AssertionFailedError 값이 없거나, 얻은 값이 기대값과 다른 경우
     */
    public static void assertExecuteScalar(Statement aStmt, String aExpectedValue, String aColumnName, String aTableName) throws SQLException
    {
        ResultSet sRS = aStmt.executeQuery("SELECT " + aColumnName + " FROM " + aTableName);
        if (sRS.next())
        {
            assertEquals(aExpectedValue, sRS.getString(1));
        }
        else
        {
            fail();
        }
        sRS.close();
    }

    public static Object executeScalar(Statement aStmt, String aQstr) throws SQLException
    {
        Object sResult = null;
        ResultSet sRS = aStmt.executeQuery(aQstr);
        if (sRS.next())
        {
            sResult = sRS.getObject(1);
        }
        sRS.close();
        return sResult;
    }

    public Object executeScalar(PreparedStatement aStmt) throws SQLException
    {
        Object sResult = null;
        ResultSet sRS = aStmt.executeQuery();
        if (sRS.next())
        {
            sResult = sRS.getObject(1);
        }
        sRS.close();
        return sResult;
    }

    public static void printSQLException(SQLException aEx)
    {
        printSQLException(aEx, false);
    }

    public static void printSQLException(SQLException aEx, boolean aPrintStackTrace)
    {
        System.out.println("ERR-" + toHexString(aEx.getErrorCode(), 5)
                           + " (" + aEx.getErrorCode() + ")"
                           + " [" + aEx.getSQLState() + "] " + aEx.getMessage());

        if (aPrintStackTrace)
        {
            aEx.printStackTrace();
        }
    }

    public static void printSQLWarning(SQLWarning aWarning) throws SQLException
    {
        for (int i = 0; aWarning != null; i++)
        {
            System.out.println(i + ": ERR-" + toHexString(aWarning.getErrorCode(), 5)
                               + " (" + aWarning.getErrorCode() + ")"
                               + " [" + aWarning.getSQLState() + "] " + aWarning.getMessage());
            aWarning = aWarning.getNextWarning();
        }
    }

    private static String toHexString(int aValue, int sMaxPadLen)
    {
        String sHexStr = Integer.toHexString(aValue).toUpperCase();
        if (sHexStr.length() < sMaxPadLen)
        {
            sHexStr = "00000".substring(0, sMaxPadLen - sHexStr.length()) + sHexStr;
        }
        return sHexStr;
    }
}
