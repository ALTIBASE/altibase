package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.*;

// from TC/Interface/newJDBC/Bugs/BUG-20775/BUG-20775.sql
public class BatchUpdateExceptionTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP PROCEDURE PROC_UPDATE",
                "DROP TABLE T_STATEMENT",
                "DROP TABLE T_PREPARED",
                "DROP TABLE T_CALLABLE",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                // Table for Statement
                "CREATE TABLE T_STATEMENT ( I1 INTEGER, I2 INTEGER UNIQUE )",
                "INSERT INTO T_STATEMENT VALUES ( 1, 1 );",
                "INSERT INTO T_STATEMENT VALUES ( 2, 2 )",
                "INSERT INTO T_STATEMENT VALUES ( 3, 3 )",
                "INSERT INTO T_STATEMENT VALUES ( 4, 4 )",
                "INSERT INTO T_STATEMENT VALUES ( 5, 5 )",
                "INSERT INTO T_STATEMENT VALUES ( 6, 6 )",
                "INSERT INTO T_STATEMENT VALUES ( 7, 7 )",
                "INSERT INTO T_STATEMENT VALUES ( 8, 8 )",
                "INSERT INTO T_STATEMENT VALUES ( 9, 9 )",
                "INSERT INTO T_STATEMENT VALUES ( 10, 10 )",
                "INSERT INTO T_STATEMENT VALUES ( 16, 16 )",
                "INSERT INTO T_STATEMENT VALUES ( 20, 20 )",

                // Table for PreparedStatement
                "CREATE TABLE T_PREPARED ( I1 INTEGER, I2 INTEGER UNIQUE )",
                "INSERT INTO T_PREPARED VALUES ( 1, 1 );",
                "INSERT INTO T_PREPARED VALUES ( 2, 2 )",
                "INSERT INTO T_PREPARED VALUES ( 3, 3 )",
                "INSERT INTO T_PREPARED VALUES ( 4, 4 )",
                "INSERT INTO T_PREPARED VALUES ( 5, 5 )",
                "INSERT INTO T_PREPARED VALUES ( 6, 6 )",
                "INSERT INTO T_PREPARED VALUES ( 7, 7 )",
                "INSERT INTO T_PREPARED VALUES ( 8, 8 )",
                "INSERT INTO T_PREPARED VALUES ( 9, 9 )",
                "INSERT INTO T_PREPARED VALUES ( 10, 10 )",
                "INSERT INTO T_PREPARED VALUES ( 16, 16 )",
                "INSERT INTO T_PREPARED VALUES ( 20, 20 )",

                // Table for CallableStatement
                "CREATE TABLE T_CALLABLE ( I1 INTEGER, I2 INTEGER UNIQUE )",
                "INSERT INTO T_CALLABLE VALUES ( 1, 1 )",
                "INSERT INTO T_CALLABLE VALUES ( 2, 2 )",
                "INSERT INTO T_CALLABLE VALUES ( 3, 3 )",
                "INSERT INTO T_CALLABLE VALUES ( 4, 4 )",
                "INSERT INTO T_CALLABLE VALUES ( 5, 5 )",
                "INSERT INTO T_CALLABLE VALUES ( 6, 6 )",
                "INSERT INTO T_CALLABLE VALUES ( 7, 7 )",
                "INSERT INTO T_CALLABLE VALUES ( 8, 8 )",
                "INSERT INTO T_CALLABLE VALUES ( 9, 9 )",
                "INSERT INTO T_CALLABLE VALUES ( 10, 10 )",
                "INSERT INTO T_CALLABLE VALUES ( 16, 16 )",
                "INSERT INTO T_CALLABLE VALUES ( 20, 20 )",
                "CREATE PROCEDURE PROC_UPDATE( v1 INTEGER, v2 INTEGER )"
                        + " AS BEGIN "
                        + " UPDATE T_CALLABLE SET I2 = I2 + 10 WHERE I1 BETWEEN v1 AND v2;"
                        + " END",
        };
    }

    public void testBUG20775() throws SQLException
    {
        Statement sStmt = connection().createStatement();

        sStmt.addBatch("UPDATE T_STATEMENT SET I2 = I2 + 10 WHERE I1 BETWEEN 1 AND 2");
        sStmt.addBatch("UPDATE T_STATEMENT SET I2 = I2 + 10 WHERE I1 BETWEEN 3 AND 4");
        // Should Be Failed To Update By Unique Violation
        sStmt.addBatch("UPDATE T_STATEMENT SET I2 = I2 + 10 WHERE I1 BETWEEN 5 AND 6");
        sStmt.addBatch("UPDATE T_STATEMENT SET I2 = I2 + 10 WHERE I1 BETWEEN 7 AND 8");
        // Should Be Failed To Update By Unique Violation
        sStmt.addBatch("UPDATE T_STATEMENT SET I2 = I2 + 10 WHERE I1 BETWEEN 9 AND 10");

        checkBatchException(sStmt);
        sStmt.close();

        PreparedStatement sPreStmt = connection().prepareStatement("UPDATE T_PREPARED SET I2 = I2 + 10 WHERE I1 BETWEEN ? AND ? ");

        // UPDATE T_PREPARED SET I2 = I2 + 10 WHERE I1 BETWEEN 1 AND 2
        sPreStmt.setInt(1, 1);
        sPreStmt.setInt(2, 2);
        sPreStmt.addBatch();

        // UPDATE T_PREPARED SET I2 = I2 + 10 WHERE I1 BETWEEN 3 AND 4
        sPreStmt.setInt(1, 3);
        sPreStmt.setInt(2, 4);
        sPreStmt.addBatch();

        // UPDATE T_PREPARED SET I2 = I2 + 10 WHERE I1 BETWEEN 5 AND 6
        // Should Be Failed To Update By Unique Violation
        sPreStmt.setInt(1, 5);
        sPreStmt.setInt(2, 6);
        sPreStmt.addBatch();

        // UPDATE T_PREPARED SET I2 = I2 + 10 WHERE I1 BETWEEN 7 AND 8
        sPreStmt.setInt(1, 7);
        sPreStmt.setInt(2, 8);
        sPreStmt.addBatch();

        // UPDATE T_PREPARED SET I2 = I2 + 10 WHERE I1 BETWEEN 9 AND 10
        // Should Be Failed To Update By Unique Violation
        sPreStmt.setInt(1, 9);
        sPreStmt.setInt(2, 10);
        sPreStmt.addBatch();

        checkBatchException(sPreStmt);
        sPreStmt.close();

        CallableStatement sCallStmt = connection().prepareCall("EXEC PROC_UPDATE( ?, ? )");

        // PROC_UPDATE( 1, 2 )
        sCallStmt.setInt(1, 1);
        sCallStmt.setInt(2, 2);
        sCallStmt.addBatch();

        // PROC_UPDATE( 3, 4 )
        sCallStmt.setInt(1, 3);
        sCallStmt.setInt(2, 4);
        sCallStmt.addBatch();

        // Unique Violation Error
        // PROC_UPDATE( 5, 6 )
        sCallStmt.setInt(1, 5);
        sCallStmt.setInt(2, 6);
        sCallStmt.addBatch();

        // PROC_UPDATE( 7, 8 )
        sCallStmt.setInt(1, 7);
        sCallStmt.setInt(2, 8);
        sCallStmt.addBatch();

        // Unique Violation Error
        // PROC_UPDATE( 9, 10 )
        sCallStmt.setInt(1, 9);
        sCallStmt.setInt(2, 10);
        sCallStmt.addBatch();

        checkBatchException(sCallStmt);
        sCallStmt.close();
    }

    private void checkBatchException(Statement sStmt) throws SQLException
    {
        try
        {
            sStmt.executeBatch();
            fail();
        }
        catch (BatchUpdateException sEx)
        {
            assertEquals(ErrorDef.BATCH_UPDATE_EXCEPTION_OCCURRED, sEx.getErrorCode());
            int[] sUpdateCount = sEx.getUpdateCounts();
            assertEquals(5, sUpdateCount.length);
            for (int i = 0; i < sUpdateCount.length; i++)
            {
                switch (i)
                {
                    case 0: case 1: case 3:
                         assertEquals(2, sUpdateCount[i]);
                        break;
                    case 2: case 4:
                        assertEquals(Statement.EXECUTE_FAILED, sUpdateCount[i]);
                        break;
                    default:
                        fail();
                        break;
                }
            }
            for (SQLException sEX = sEx; (sEX = sEX.getNextException()) != null;)
            {
                assertEquals(0x11058, sEX.getErrorCode());
            }
        }
    }
}
