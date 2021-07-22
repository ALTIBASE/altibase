package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class ResultSetTest extends AltibaseTestCase
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
                "CREATE TABLE t2 (c1 INTEGER)",
                "CREATE TABLE t3 (c1 INTEGER, c2 VARCHAR(20))",
                "INSERT INTO t1 VALUES (1)",
                "INSERT INTO t1 VALUES (2)",
                "INSERT INTO t1 VALUES (3)",
                "INSERT INTO t1 VALUES (4)",
                "INSERT INTO t1 VALUES (5)",
                "INSERT INTO t1 VALUES (6)",
                "INSERT INTO t1 VALUES (7)",
                "INSERT INTO t1 VALUES (8)",
                "INSERT INTO t1 VALUES (9)",
                "INSERT INTO t3 VALUES (1, 'a')",
                "INSERT INTO t3 VALUES (2, 'b')",
                "INSERT INTO t3 VALUES (3, 'c')",
                "INSERT INTO t3 VALUES (4, 'd')",
                "INSERT INTO t3 VALUES (5, 'e')",
                "INSERT INTO t3 VALUES (6, 'f')",
                "INSERT INTO t3 VALUES (7, 'g')",
                "INSERT INTO t3 VALUES (8, 'h')",
                "INSERT INTO t3 VALUES (9, 'i')",
                "INSERT INTO t3 VALUES (10, null)",
                "INSERT INTO t3 VALUES (11, 'k')",
        };
    }

    private static final String[] VALUES_T3_C2 = { null, "a", "b", "c", "d", "e", "f", "g", "h", "i", null, "k" };

    public void testEmptyResultSetFR() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t2");
        assertEquals(AltibaseEmptyResultSet.class, sRS.getClass());
        assertFalse(sRS.next());
        try
        {
            sRS.absolute(1);
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY, sEx.getErrorCode());
        }
        try
        {
            sRS.relative(0);
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY, sEx.getErrorCode());
        }
        try
        {
            sRS.first();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY, sEx.getErrorCode());
        }
        try
        {
            sRS.last();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY, sEx.getErrorCode());
        }
        sRS.close();
        sStmt.close();
    }

    public void testEmptyResultSetSU() throws SQLException
    {
        Statement sStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t2");
        assertEquals(AltibaseEmptyResultSet.class, sRS.getClass());
        // empty result set은 항상 after last에 있는것으로 간주
        assertEquals(0, sRS.getRow());
        assertEquals(false, sRS.isLast());
        assertEquals(false, sRS.isFirst());
        assertEquals(false, sRS.isBeforeFirst());
        assertEquals(false, sRS.isAfterLast());
        try
        {
            sRS.getObject(1);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.EMPTY_RESULTSET, sEx.getErrorCode());
        }
        assertFalse(sRS.next());
        assertEquals(0, sRS.getRow());
        assertEquals(false, sRS.isLast());
        assertEquals(false, sRS.isFirst());
        assertEquals(false, sRS.isBeforeFirst());
        assertEquals(false, sRS.isAfterLast());
        try
        {
            sRS.getObject(1);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.EMPTY_RESULTSET, sEx.getErrorCode());
        }
        assertFalse(sRS.absolute(1));
        assertFalse(sRS.absolute(-1));
        assertFalse(sRS.first());
        assertFalse(sRS.last());
        sRS.beforeFirst();
        sRS.afterLast();
        sRS.close();
        sStmt.close();
    }

    public void testFetchSize() throws Exception
    {
        Statement sStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        // fetch size가 제대로 적용됨을 보기 위한 external update
        PreparedStatement sUpdStmt = connection().prepareStatement("UPDATE t1 SET c1 = 100 + c1");
        sStmt.setFetchSize(1);
        ResultSet sRS = sStmt.executeQuery("SELECT t1.* FROM t1");
        sUpdStmt.executeUpdate();
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1)); // cache 때문에 예전값
        assertEquals(true, sRS.next());
        assertEquals(102, sRS.getInt(1));
        sRS.setFetchSize(2);
        sUpdStmt.executeUpdate();
        assertEquals(true, sRS.next());
        assertEquals(203, sRS.getInt(1));
        assertEquals(true, sRS.next());
        assertEquals(204, sRS.getInt(1));
        assertEquals(true, sRS.next());
        assertEquals(205, sRS.getInt(1));
        sRS.setFetchSize(1);
        sUpdStmt.executeUpdate();
        assertEquals(true, sRS.next());
        assertEquals(206, sRS.getInt(1)); // cache 때문에 예전값
        assertEquals(true, sRS.next());
        assertEquals(307, sRS.getInt(1));
        assertEquals(true, sRS.next());
        assertEquals(308, sRS.getInt(1));
        sUpdStmt.executeUpdate();
        assertEquals(true, sRS.next());
        assertEquals(409, sRS.getInt(1));
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
        sUpdStmt.close();
    }

    public void testResultSetCloseAndReuse() throws Exception
    {
        testResultSetCloseAndReuse(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY);
        testResultSetCloseAndReuse(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE);
        testResultSetCloseAndReuse(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY);
        testResultSetCloseAndReuse(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        testResultSetCloseAndReuse(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
        testResultSetCloseAndReuse(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE);
    }

    private void testResultSetCloseAndReuse(int aType, int aConcur) throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT t1.* FROM t1", aType, aConcur);
        for (int i = 0; i < 3; i++)
        {
            ResultSet sRS = sStmt.executeQuery();
            for (int j = 1; j <= 9; j++)
            {
                assertEquals(true, sRS.next());
                assertEquals(j, sRS.getInt(1));
            }
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }

    public void testScroll() throws Exception
    {
        Statement sStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        sStmt.setFetchSize(1);
        ResultSet sRS = sStmt.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, sRS.next());
        sRS.relative(-100);
        assertEquals(true, sRS.isBeforeFirst());
        // beforeFirst에서 몇번을 previous해도 계속 beforeFirst에 있어야 하며, 그 후 next하면 첫번째 row를 얻어야 한다.
        assertEquals(false, sRS.previous());
        assertEquals(false, sRS.previous());
        assertEquals(false, sRS.previous());
        assertEquals(false, sRS.previous());
        assertEquals(false, sRS.previous());
        assertEquals(false, sRS.previous());
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        sRS.beforeFirst();
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        assertEquals(true, sRS.isAfterLast());
        sRS.absolute(2);
        sRS.relative(-100);
        assertEquals(true, sRS.isBeforeFirst());
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        sRS.relative(100);
        assertEquals(true, sRS.isAfterLast());
        // afterLast에서 몇번을 next해도 계속 afterLast에 있어야 하며, 그 후 previous하면 마지막 row를 얻어야 한다.
        assertEquals(false, sRS.next());
        assertEquals(false, sRS.next());
        assertEquals(false, sRS.next());
        assertEquals(false, sRS.next());
        assertEquals(false, sRS.next());
        assertEquals(false, sRS.next());
        assertEquals(false, sRS.next());
        assertEquals(true, sRS.previous());
        assertEquals(9, sRS.getInt(1));
        // refresh가 적용됨을 보기 위해 external update
        {
            Statement sUpdStmt = connection().createStatement();
            assertEquals(9, sUpdStmt.executeUpdate("UPDATE t1 SET c1 = 100 + c1"));
            sUpdStmt.close();
        }
        sRS.refreshRow();
        assertEquals(false, sRS.next());
        assertEquals(true, sRS.isAfterLast());
        for (int i = 9; i >= 1; i--)
        {
            assertEquals(true, sRS.previous());
            assertEquals(100 + i, sRS.getInt(1));
        }
        assertEquals(false, sRS.previous());
        sRS.close();
        sStmt.close();
    }

    public void testResultSetInsert() throws SQLException
    {
        Statement sStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        sStmt.setFetchSize(1);
        ResultSet sRS = sStmt.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(true, sRS.next());
        assertEquals(true, sRS.next());
        assertEquals(3, sRS.getInt(1));

        // insert 준비
        sRS.moveToInsertRow();
        sRS.updateInt(1, 10);

        // insert 상태이므로 update, sensitive 관련 연산은 실패해야 한다.
        try
        {
            sRS.updateRow();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CURSOR_AT_INSERTING_ROW, sEx.getErrorCode());
        }
        try
        {
            sRS.refreshRow();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CURSOR_AT_INSERTING_ROW, sEx.getErrorCode());
        }
        // BUGBUG (2013-03-07) oracle은 spec과 달리 insert row에서 cancelRowUpdates()를 호출해도 예외를 던지지 않는다.
        try
        {
            sRS.cancelRowUpdates();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CURSOR_AT_INSERTING_ROW, sEx.getErrorCode());
        }

        // insert 수행
        sRS.insertRow();

        sRS.close();
        sStmt.close();

        // insert 10이 잘 됐나 확인. dynamic이 아니므로 cursor를 다시 열어야 한다.
        sStmt = connection().createStatement();
        sRS = sStmt.executeQuery("SELECT * FROM t1");
        for (int i = 1; i <= 10; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }

    public void testResultSetUpdate() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT t3.* FROM t3 ORDER BY c1", ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);

        // internal update가 refresh 안해도 ResultSet에 바로 적용되는지 확인
        sStmt.setFetchSize(9);
        ResultSet sRS = sStmt.executeQuery();
        assertEquals(true, sRS.next());
        assertEquals(true, sRS.next());
        assertEquals(true, sRS.next());
        assertEquals(3, sRS.getInt(1));
        assertEquals(VALUES_T3_C2[3], sRS.getString(2));
        sRS.updateString(2, "c-updated");
        sRS.updateRow();
        assertEquals(true, sRS.previous());
        assertEquals(true, sRS.next());
        assertEquals(3, sRS.getInt(1));
        assertEquals("c-updated", sRS.getString(2));
        assertEquals(true, sRS.next());
        assertEquals(4, sRS.getInt(1));
        assertEquals(VALUES_T3_C2[4], sRS.getString(2));
        sRS.close();

        // 재확인
        sRS = sStmt.executeQuery();
        for (int i = 1; i <= 11; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
            if (i == 3)
            {
                assertEquals("c-updated", sRS.getString(2));
            }
            else
            {
                assertEquals(VALUES_T3_C2[i], sRS.getString(2));
            }
        }
        assertEquals(false, sRS.next());
        sRS.close();

        sStmt.close();
    }

    public void testSensitiveUpdate() throws SQLException
    {
        Statement sSelStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        ResultSet sRS = sSelStmt.executeQuery("SELECT t1.* FROM t1");
        try
        {
            sRS.refreshRow();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CURSOR_AT_BEFORE_FIRST, sEx.getErrorCode());
        }
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        assertEquals(true, sRS.isAfterLast());
        try
        {
            sRS.refreshRow();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CURSOR_AT_AFTER_LAST, sEx.getErrorCode());
        }

        Statement sUpdStmt = connection().createStatement();
        sUpdStmt.executeUpdate("UPDATE t1 SET c1 = c1 + 100");
        sUpdStmt.close();

        assertEquals(true, sRS.isAfterLast());
        for (int i = 109; i >= 101; i--)
        {
            assertEquals(true, sRS.previous());
            sRS.refreshRow();
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.previous());

        sRS.close();
        sSelStmt.close();
    }

    // BUGBUG (2013-03-08) oracle은 hole을 감지할 수 없다.
    // #region Hole test
    public void testSensitiveForInternalDelete() throws SQLException
    {
        Statement sSelStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        ResultSet sRS = sSelStmt.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, connection().getMetaData().deletesAreDetected(sRS.getType()));
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());

        sRS.absolute(3);
        assertEquals(3, sRS.getRow());
        sRS.deleteRow();
        assertEquals(2, sRS.getRow()); // deleteRow() 후에는 한칸 앞으로 이동
        assertEquals(2, sRS.getInt(1));

        sRS.absolute(1);
        assertEquals(1, sRS.getRow());
        sRS.deleteRow();
        assertEquals(0, sRS.getRow()); // deleteRow() 후에는 한칸 앞으로 이동
        assertEquals(true, sRS.isBeforeFirst());
        try
        {
            sRS.getInt(1);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CURSOR_AT_BEFORE_FIRST, sEx.getErrorCode());
        }

        // internal delete로 인한 hole 처리가 제대로 됐나 확인
        {
            // refresh가 적용됨을 보기 위해 external update
            Statement sUpdStmt = connection().createStatement();
            assertEquals(7, sUpdStmt.executeUpdate("UPDATE t1 SET c1 = 100 + c1")); // 2 rows deleted
            sUpdStmt.close();
        }
        sRS.absolute(0);
        for (int i = 1; i <= 9; i++)
        {
            if (i == 1 || i == 3)
            {
                continue; // deleted hole
            }

            assertEquals(true, sRS.next());
            sRS.refreshRow();
            assertEquals(i + 100, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();

        // internal delete가 db에 잘 반영됐나 확인
        sRS = sSelStmt.executeQuery("SELECT t1.* FROM t1 ORDER BY c1");
        for (int i = 1; i <= 9; i++)
        {
            if (i == 1 || i == 3)
            {
                continue; // deleted row
            }

            assertEquals(true, sRS.next());
            assertEquals(100 + i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());

        sRS.close();
        sSelStmt.close();
    }

    public void testSensitiveForExternalDeleteWithScrollDown() throws SQLException
    {
        Statement sSelStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        ResultSet sRS = sSelStmt.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, connection().getMetaData().deletesAreDetected(sRS.getType()));
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());

        Statement sDelStmt = connection().createStatement();
        sDelStmt.executeUpdate("DELETE t1 WHERE c1 < 5");
        sDelStmt.close();

        sRS.absolute(0);
        // refresh가 적용됨을 보기 위해 external update
        {
            Statement sUpdStmt = connection().createStatement();
            assertEquals(5, sUpdStmt.executeUpdate("UPDATE t1 SET c1 = 100 + c1"));
            sUpdStmt.close();
        }
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            sRS.refreshRow();
            assertEquals(i < 5, sRS.rowDeleted());
            if (i < 5)
            {
                // Hole은 SQL NULL에 해당하는 반환
                assertEquals(0, sRS.getInt(1));
                assertEquals(null, sRS.getObject(1));
            }
            else
            {
                assertEquals(i + 100, sRS.getInt(1));
            }
        }
        assertEquals(false, sRS.next());

        sRS.close();
        sSelStmt.close();
    }

    public void testSensitiveForExternalDeleteWithScrollUp() throws SQLException
    {
        Statement sSelStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        ResultSet sRS = sSelStmt.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, connection().getMetaData().deletesAreDetected(sRS.getType()));
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());

        Statement sDelStmt = connection().createStatement();
        sDelStmt.executeUpdate("DELETE t1 WHERE c1 < 5");
        sDelStmt.close();

        // refresh가 적용됨을 보기 위해 external update
        {
            Statement sUpdStmt = connection().createStatement();
            assertEquals(5, sUpdStmt.executeUpdate("UPDATE t1 SET c1 = 100 + c1"));
            sUpdStmt.close();
        }
        for (int i = 9; i >= 1; i--)
        {
            assertEquals(true, sRS.previous());
            sRS.refreshRow();
            assertEquals(i < 5, sRS.rowDeleted());
            if (i < 5)
            {
                // Hole은 SQL NULL에 해당하는 반환
                assertEquals(0, sRS.getInt(1));
                assertEquals(null, sRS.getObject(1));
            }
            else
            {
                assertEquals(i + 100, sRS.getInt(1));
            }
        }
        assertEquals(false, sRS.previous());

        sRS.close();
        sSelStmt.close();
    }

    public void testUpdateFailForExternalDeletedRow() throws Exception
    {
        PreparedStatement sSelStmt = connection().prepareStatement("SELECT t1.* FROM t1", ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
        sSelStmt.setFetchSize(1);

        ResultSet sRS = sSelStmt.executeQuery();
        assertEquals(ResultSet.TYPE_SCROLL_SENSITIVE, sRS.getType());
        assertEquals(ResultSet.CONCUR_UPDATABLE, sRS.getConcurrency());

        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));

        Statement sDelStmt = connection().createStatement();
        assertEquals(1, sDelStmt.executeUpdate("DELETE t1 WHERE c1 = 3"));
        sDelStmt.close();

        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt(1));

        assertEquals(true, sRS.next());
        // Hole은 0 또는 null
        assertEquals(true, sRS.rowDeleted());
        assertEquals(0, sRS.getInt(1));
        assertEquals(null, sRS.getObject(1));

        sRS.updateInt(1,  101);
        try
        {
            sRS.updateRow();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CURSOR_OPERATION_CONFLICT, sEx.getErrorCode());
        }

        sSelStmt.close();
    }

    // #endregion Hole test

    // BUGBUG (2013-03-08) oracle은 CLOSE_CURSORS_AT_COMMIT을 지원하지 않는 듯.
    // #region CLOSE_CURSORS_AT_COMMIT test

    public void testNonHold() throws SQLException
    {
        fetchAfterEndTrans(true);
        fetchAfterEndTrans(false);
    }

    private void fetchAfterEndTrans(boolean aCommit) throws SQLException
    {
        connection().setAutoCommit(false);

        Statement sStmt = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        sStmt.setFetchSize(1);
        ResultSet sRS = sStmt.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));

        if (aCommit)
        {
            connection().commit();
        }
        else
        {
            connection().rollback();
        }
        try
        {
            sRS.next();
            fail();
        }
        catch (SQLException ex)
        {
            assertEquals(ErrorDef.FETCH_OUT_OF_SEQUENCE, ex.getErrorCode());
        }
        sRS.close();
        sStmt.close();

        connection().setAutoCommit(true);
    }

    public void testHold() throws SQLException
    {
        connection().setAutoCommit(false);

        Statement sStmtC = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        sStmtC.setFetchSize(1);
        ResultSet sRSC = sStmtC.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, sRSC.next());
        assertEquals(1, sRSC.getInt(1));

        connection().commit();
        assertEquals(true, sRSC.next());
        assertEquals(2, sRSC.getInt(1));

        Statement sStmtR = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        sStmtR.setFetchSize(1);
        ResultSet sRSR = sStmtR.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, sRSR.next());
        assertEquals(1, sRSR.getInt(1));

        connection().rollback();
        assertEquals(true, sRSC.next());
        assertEquals(3, sRSC.getInt(1));

        /* BUG-45841 Fetch Across Rollback에 의해 커서가 유지된다. */
        assertEquals(true, sRSR.next());
        assertEquals(2, sRSR.getInt(1));

        Statement sStmtRR = connection().createStatement(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, ResultSet.HOLD_CURSORS_OVER_COMMIT);
        sStmtRR.setFetchSize(1);
        assertEquals(1, sStmtRR.executeUpdate("UPDATE t1 SET c1 = 2 WHERE c1 = 2"));
        ResultSet sRSRR = sStmtRR.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, sRSRR.next());
        assertEquals(1, sRSRR.getInt(1));

        connection().rollback();  /* Update에 의해 sRSRR 커서는 닫힌다. */
        assertEquals(true, sRSC.next());
        assertEquals(4, sRSC.getInt(1));
        assertEquals(true, sRSR.next());
        assertEquals(3, sRSR.getInt(1));

        try
        {
            sRSRR.next();
            fail("Expected FETCH_OUT_OF_SEQUENCE, but failed.");
        }
        catch (SQLException ex)
        {
            assertEquals(ErrorDef.FETCH_OUT_OF_SEQUENCE, ex.getErrorCode());
        }

        sRSR.close();
        sStmtR.close();
        sRSC.close();
        sStmtC.close();
        sStmtRR.close();
        sRSRR.close();
    }

    // #endregion CLOSE_CURSORS_AT_COMMIT test



    // #region Result Set Downgrade

    public void testNotDowngrade() throws SQLException
    {
        testNotDowngrade("SELECT t1.* FROM t1", true);
        testNotDowngrade("SELECT t3.* FROM t3", true);
        testNotDowngrade("SELECT a.* FROM t1 a", true);
        // BUGBUG (2013-03-08) oracle은 다음과같은 에러를 뱉는다(wtf): ORA-00933 SQL command not properly ended
        testNotDowngrade("SELECT a.* FROM t1 AS a", true);
        testNotDowngrade("SELECT c1 FROM t1", true);
        testNotDowngrade("SELECT c1, c2 FROM t3", true);

        testNotDowngrade("SELECT 1, c1 FROM t3", false);

        assertNotDowngradeRI("SELECT * FROM t1");
        assertNotDowngradeRI("SELECT * FROM t1, t2");

        testNotDowngrade("SELECT * FROM t1", true);
    }

    private void testNotDowngrade(String aQstr, boolean aUpdatable) throws SQLException
    {
        if (aUpdatable)
        {
            assertNotDowngradeUS(aQstr);
            assertNotDowngradeUI(aQstr);
            assertNotDowngradeUF(aQstr);
        }

        assertNotDowngradeRS(aQstr);
        assertNotDowngradeRI(aQstr);
        assertNotDowngradeRF(aQstr);
    }

    public void testDowngrade() throws SQLException
    {
        testDowngrade("SELECT a.* FROM t1 a, t1 b");
        testDowngrade("SELECT * FROM t1, t2");
        // BUGBUG (2013-03-08) oracle은 updatable 안한 컬럼이 있어도 그냥 updatable로 열어준다.
        testDowngrade("SELECT c1, 1 FROM t3");
    }

    private void testDowngrade(String aQstr) throws SQLException
    {
        assertDowngradeUS2RI(aQstr);
        assertDowngradeUI2RI(aQstr);
        assertNotDowngradeRI(aQstr);
        assertNotDowngradeRF(aQstr);
    }

    private void assertNotDowngradeRF(String aQstr) throws SQLException
    {
        assertNotDowngrade(aQstr, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY);
    }

    private void assertNotDowngradeUF(String aQstr) throws SQLException
    {
        assertNotDowngrade(aQstr, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE);
    }

    private void assertNotDowngradeRI(String aQstr) throws SQLException
    {
        assertNotDowngrade(aQstr, ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
    }

    private void assertNotDowngradeUI(String aQstr) throws SQLException
    {
        assertNotDowngrade(aQstr, ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE);
    }

    private void assertNotDowngradeRS(String aQstr) throws SQLException
    {
        assertNotDowngrade(aQstr, ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY);
    }

    private void assertNotDowngradeUS(String aQstr) throws SQLException
    {
        assertNotDowngrade(aQstr, ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
    }

    private void assertNotDowngrade(String aQstr, int aType, int aConcur) throws SQLException
    {
        assertCursorAttrs(aQstr, aType, aConcur, aType, aConcur);
    }

    private void assertDowngradeUI2RI(String aQstr) throws SQLException
    {
        assertCursorAttrs(aQstr,
                          ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE,
                          ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
    }

    private void assertDowngradeUS2RI(String aQstr) throws SQLException
    {
        assertCursorAttrs(aQstr,
                          ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE,
                          ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
    }

    private void assertCursorAttrs(String aQstr, int aOrgType, int aOrgConcur, int aDownType, int aDownConcur) throws SQLException
    {
        // prepare + execute-fetch
        PreparedStatement sPStmt = connection().prepareStatement(aQstr, aOrgType, aOrgConcur);
        ResultSet sRS = sPStmt.executeQuery();
        assertEquals(aOrgType, sPStmt.getResultSetType());
        assertEquals(aOrgConcur, sPStmt.getResultSetConcurrency());
        assertEquals(aDownType, sRS.getType());
        assertEquals(aDownConcur, sRS.getConcurrency());
        sRS.close();
        sPStmt.close();

        // direct-execute-fetch
        Statement sStmt = connection().createStatement(aOrgType, aOrgConcur);
        sRS = sStmt.executeQuery(aQstr);
        assertEquals(aOrgType, sStmt.getResultSetType());
        assertEquals(aOrgConcur, sStmt.getResultSetConcurrency());
        assertEquals(aDownType, sRS.getType());
        assertEquals(aDownConcur, sRS.getConcurrency());
        sRS.close();
        sStmt.close();

        // direct-executge + fetch
        sStmt = connection().createStatement(aOrgType, aOrgConcur);
        assertTrue(sStmt.execute(aQstr));
        sRS = sStmt.getResultSet();
        assertEquals(aOrgType, sStmt.getResultSetType());
        assertEquals(aOrgConcur, sStmt.getResultSetConcurrency());
        assertEquals(aDownType, sRS.getType());
        assertEquals(aDownConcur, sRS.getConcurrency());
        sRS.close();
        sStmt.close();
    }

    // #endregion



    public void testMaxRows() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        for (int sMaxRows = 0; sMaxRows <= 9; sMaxRows++)
        {
            if (sMaxRows != 0)
            {
                sStmt.setMaxRows(sMaxRows);
            }
            ResultSet sRS = sStmt.executeQuery();
            for (int i = 1; i <= (sMaxRows == 0 ? 9 : sMaxRows); i++)
            {
                assertEquals(true, sRS.next());
                assertEquals(i, sRS.getInt(1));
            }
            assertEquals(false, sRS.next());
            sRS.close();
        }
        sStmt.close();
    }

    public void testReExecuteWithoutCursorClose() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.setFetchSize(1);
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals(9, sStmt.executeUpdate("UPDATE t1 SET c1 = c1 + 100"));
        sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(101, sRS.getInt(1));
        assertEquals(false, sStmt.execute("UPDATE t1 SET c1 = c1 - 100"));
        assertEquals(9, sStmt.getUpdateCount());
        assertEquals(true, sStmt.execute("SELECT * FROM t1"));
        sRS = sStmt.getResultSet();
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }

    // Altibase 전용
    public void testStatementAutoClose() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        ((AltibaseResultSet)sRS).registerTarget(sStmt);
        for (int i=1; i<=9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();
        try
        {
            sStmt.execute("SELECT * FROM dual");
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CLOSED_STATEMENT, sEx.getErrorCode());
        }
        sStmt.close();
    }

    public void testExecuteAndGetResultSetByStatement() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(true, sStmt.execute("SELECT * FROM t1"));
        ResultSet sRS = sStmt.getResultSet();
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }

    public void testExecuteAndGetResultSetByPreparedStatement() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        assertEquals(true, sStmt.execute());
        ResultSet sRS = sStmt.getResultSet();
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }

    public void testResetLastReadColumnIndex() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        assertEquals(1, sStmt.executeUpdate("INSERT INTO t1 VALUES (NULL)"));
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals(false, sRS.wasNull());
        assertEquals(true, sRS.next());
        try
        {
            sRS.wasNull();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.WAS_NULL_CALLED_BEFORE_CALLING_GETXXX, sEx.getErrorCode());
        }
        // BUGBUG (2013-03-08) oracle은 한번 getXXX()를 하면, 그 컬럼 인덱스를 계속 써서 wasNull을 검사한다.
        // spec에는 딱히 그런 얘기가 없다. 단지, 다른 getXXX를 하기 전에 wasNull로 확인하라고 할 뿐.
        assertEquals(2, sRS.getInt(1));
        assertEquals(false, sRS.wasNull());
        assertEquals(true, sRS.next()); // 3
        assertEquals(true, sRS.next()); // 4
        assertEquals(true, sRS.next()); // 5
        assertEquals(true, sRS.next()); // 6
        assertEquals(true, sRS.next()); // 7
        assertEquals(true, sRS.next()); // 8
        assertEquals(true, sRS.next()); // 9
        assertEquals(true, sRS.next()); // NULL
        assertEquals(0, sRS.getInt(1));
        assertEquals(true, sRS.wasNull());
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }

    public void testGetResultSetAfterExecuteQuery() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        sStmt.executeQuery();
        ResultSet sRS = sStmt.getResultSet();
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        assertEquals(false, sRS.wasNull());
        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt(1));
        assertEquals(false, sRS.wasNull());
        sRS.close();
        sStmt.close();
    }

    public void testParamBind() throws SQLException
    {
        testParamBind(true);
        testParamBind(false);
    }

    private static final int[] PARAM_BIND_TEST_IN = { 8, 9, 10, 8 };

    private void testParamBind(boolean aUseExecuteQuery) throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1 WHERE c1 > ?");
        ResultSet sRS;
        for (int i = 0; i < PARAM_BIND_TEST_IN.length; i++)
        {
            sStmt.setInt(1, PARAM_BIND_TEST_IN[i]);
            if (aUseExecuteQuery)
            {
                sRS = sStmt.executeQuery();
            }
            else
            {
                assertEquals(true, sStmt.execute());
                sRS = sStmt.getResultSet();
            }
            if (PARAM_BIND_TEST_IN[i] == 8)
            {
                assertEquals(true, sRS.next());
                assertEquals(9, sRS.getInt(1));
            }
            else
            {
                assertEquals(false, sRS.next());
            }
            sRS.close();
        }
        sStmt.close();
    }

    public void testGetResultsetAfterExecuteUpdate() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.executeUpdate("SELECT * FROM t1");
        ResultSet sRS = sStmt.getResultSet();
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
    }

    public void testGetResultsetAfterPrepareExecuteUpdate() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("SELECT * FROM t1");
        sStmt.executeUpdate();
        ResultSet sRS = sStmt.getResultSet();
        for (int i = 1; i <= 9; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(i, sRS.getInt(1));
        }
        assertEquals(false, sRS.next());
    }

    public void testNoCurrentRow() throws SQLException
    {
        Statement sStmt = connection().createStatement(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        sRS.afterLast();
        try
        {
            sRS.relative(1);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NO_CURRENT_ROW, sEx.getErrorCode());
        }
        sRS.beforeFirst();
        try
        {
            sRS.relative(-1);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NO_CURRENT_ROW, sEx.getErrorCode());
        }
    }

    public void testNextAfterStmtClose() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.setFetchSize(1);
        ResultSet sRS = sStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));
        sStmt.close();
        try
        {
            sRS.next();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.CLOSED_STATEMENT, sEx.getErrorCode());
        }
    }

    public void testRefreshRowExForForwardOnlyReadOnly()
    {
        try
        {
            testRefreshRowExFor(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY, sEx.getErrorCode());
        }
    }

    public void testRefreshRowExForForwardOnlyUpdatable()
    {
        try
        {
            testRefreshRowExFor(ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_UPDATABLE);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.NOT_SUPPORTED_OPERATION_ON_FORWARD_ONLY, sEx.getErrorCode());
        }
    }

    public void testRefreshRowExForScrollInsensitiveReadOnly() throws SQLException
    {
        testRefreshRowExFor(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_READ_ONLY);
    }

    public void testRefreshRowExForScrollInsensitiveUpdatable() throws SQLException
    {
        testRefreshRowExFor(ResultSet.TYPE_SCROLL_INSENSITIVE, ResultSet.CONCUR_UPDATABLE);
    }

    public void testRefreshRowExForScrollSensitiveReadOnly() throws SQLException
    {
        testRefreshRowExFor(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY);
    }

    public void testRefreshRowExForScrollSensitiveUpdatable() throws SQLException
    {
        testRefreshRowExFor(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_UPDATABLE);
    }

    private void testRefreshRowExFor(int aType, int aConcurrency) throws SQLException
    {
        Statement sStmt = connection().createStatement(aType, aConcurrency, ResultSet.CLOSE_CURSORS_AT_COMMIT);
        ResultSet sRS = sStmt.executeQuery("SELECT t1.* FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(1, sRS.getInt(1));

        Statement sUpdStmt = connection().createStatement();
        assertEquals(9, sUpdStmt.executeUpdate("UPDATE t1 SET c1 = 100 + c1"));
        sUpdStmt.close();

        try
        {
            if (aConcurrency == ResultSet.CONCUR_UPDATABLE)
            {
                sRS.updateInt(1, 201);
                sRS.updateRow();
            }

            // 내부 데이타 정리를 위해 커서를 한번 이동
            if (aType != ResultSet.TYPE_FORWARD_ONLY)
            {
                sRS.previous();
                sRS.next();
            }
            sRS.refreshRow();

            // updatable이면 internal update 값이 보여야 한다.
            if (aConcurrency == ResultSet.CONCUR_UPDATABLE)
            {
                assertEquals(201, sRS.getInt(1));
            }
            // read-only더라도 sensitive이면 external update 값이 보여야 한다.
            else if (aType == ResultSet.TYPE_SCROLL_SENSITIVE)
            {
                assertEquals(101, sRS.getInt(1));
            }
            // read-only, insensitive이면 기존 값이 보여야 한다.
            else
            {
                assertEquals(1, sRS.getInt(1));
            }
        }
        finally
        {
            sStmt.close();
        }
    }

    public void testSensitiveWithOrderBy() throws SQLException
    {
        testSensitiveWithOrderBy(true, true);
        testSensitiveWithOrderBy(true, false);
        testSensitiveWithOrderBy(false, true);
        testSensitiveWithOrderBy(false, false);
    }

    private void testSensitiveWithOrderBy(boolean aUseNum, boolean aAsc) throws SQLException
    {
        Statement sStmt = connection().createStatement(ResultSet.TYPE_SCROLL_SENSITIVE, ResultSet.CONCUR_READ_ONLY);
        sStmt.setFetchSize(1);
        ResultSet sRS = sStmt.executeQuery("SELECT c1 as a1, c2 a2 FROM t3 ORDER BY " + (aUseNum ? "1" : "a1") + " " + (aAsc ? "ASC" : "DESC"));
        assertEquals(ResultSet.TYPE_SCROLL_SENSITIVE, sRS.getType());
        for (int i = 1; i <= 11; i++)
        {
            int sExpVal = (aAsc ? i : (12 - i));
            assertEquals(true, sRS.next());
            assertEquals(sExpVal, sRS.getInt(1));
            if (sExpVal == 10)
            {
                assertEquals(null, sRS.getString(2));
            }
            else
            {
                assertEquals(Character.toString((char)('a' + sExpVal - 1)), sRS.getString(2));
            }
        }
        assertEquals(false, sRS.next());
        sRS.close();
        sStmt.close();
    }
}
