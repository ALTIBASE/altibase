package Altibase.jdbc.driver;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Types;

public class ResultSetMetaDataTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP TABLE t2",
                "DROP TABLE t3",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 INTEGER)",
                "CREATE TABLE t2 (d1 INTEGER, d2 VARCHAR(20))",
                "CREATE TABLE t3 (e1 INTEGER, e2 VARCHAR(20), e3 BIGINT)",
        };
    }

    private static final String[][] COLUMN_NAMES = {
        {"C1"},
        {"D1", "D2"},
        {"E1", "E2", "E3"},
    };

    private static final int[][] COLUMN_TYPES = {
        {Types.INTEGER},
        {Types.INTEGER, Types.VARCHAR},
        {Types.INTEGER, Types.VARCHAR, Types.BIGINT},
    };

    public void testMetaDataFromResultSet() throws SQLException
    {
        testMetaDataFromResultSet(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY);
        testMetaDataFromResultSet(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE);
        testMetaDataFromResultSet(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
        testMetaDataFromResultSet(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE);
        testMetaDataFromResultSet(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY);
        testMetaDataFromResultSet(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
    }

    private void testMetaDataFromResultSet(int aType, int aConcurrency) throws SQLException
    {
        Statement sStmt = connection().createStatement(aType, aConcurrency);
        for (int i = 1; i <= 3; i++)
        {
            ResultSet sRS = sStmt.executeQuery("SELECT t" + i + ".* FROM t" + i);
            assertEquals(aType, sRS.getType());
            assertEquals(aConcurrency, sRS.getConcurrency());
            assertMetaData(sRS.getMetaData(), i);
            sRS.close();
        }
        sStmt.close();
    }

    public void testMetaDataFromStatement() throws SQLException
    {
        testMetaDataFromStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY);
        testMetaDataFromStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE);
        testMetaDataFromStatement(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
        testMetaDataFromStatement(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE);
        testMetaDataFromStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY);
        testMetaDataFromStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
    }

    private void testMetaDataFromStatement(int aType, int aConcurrency) throws SQLException
    {
        for (int i = 1; i <= 3; i++)
        {
            PreparedStatement sStmt = connection().prepareStatement("SELECT t" + i + ".* FROM t" + i, aType, aConcurrency);
            assertMetaData(sStmt.getMetaData(), i);
            sStmt.close();
        }
    }

    private void assertMetaData(ResultSetMetaData sMetaData, int aTableNo) throws SQLException
    {
        assertEquals(COLUMN_NAMES[aTableNo - 1].length, sMetaData.getColumnCount());
        for (int sColIdx = 0; sColIdx < sMetaData.getColumnCount(); sColIdx++)
        {
            assertEquals("T" + aTableNo, sMetaData.getTableName(sColIdx + 1));
            assertEquals(COLUMN_NAMES[aTableNo - 1][sColIdx], sMetaData.getColumnName(sColIdx + 1));
            assertEquals(COLUMN_TYPES[aTableNo - 1][sColIdx], sMetaData.getColumnType(sColIdx + 1));
        }
    }

    public void testColumnName() throws SQLException
    {
        testColumnName("SELECT 1.0/3.0 FROM dual", "1.0/3.0", "1.0/3.0");
        testColumnName("SELECT 1 \"ANFET_1/0.08~Vtbb\" from DUAL", "ANFET_1/0.08~Vtbb", "ANFET_1/0.08~Vtbb");
    }

    private void testColumnName(String aQstr, String aExpColName, String aExpColLabel) throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery(aQstr);
        ResultSetMetaData sMetaData = sRS.getMetaData();
        assertEquals(aExpColName, sMetaData.getColumnName(1));
        assertEquals(aExpColLabel, sMetaData.getColumnLabel(1));
        assertEquals(true, sRS.next());
        assertEquals(sRS.getObject(1), sRS.getObject(sMetaData.getColumnName(1)));
        sRS.close();
        sStmt.close();
    }
}
