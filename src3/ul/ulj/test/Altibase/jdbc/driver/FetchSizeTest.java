package Altibase.jdbc.driver;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class FetchSizeTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 CHAR(24))",
                "INSERT INTO t1 VALUES (1)",
                "INSERT INTO t1 VALUES (2)",
                "INSERT INTO t1 VALUES (3)",
                "INSERT INTO t1 VALUES (4)",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
                "INSERT INTO t1 SELECT * FROM T1",
        };
    }

    public void testSize() throws SQLException
    {
        testSize(0      , 4         , 32768 , 8192  );
        testSize(4      , 10        , 4     , 1     );
        testSize(16384  , 10        , 16384 , 1639  );
        testSize(16384  , 16384     , 16384 , 1     );
        testSize(16384  , 0 /*1024*/, 16384 , 16    );
        testSize(0      , 0 /*1024*/, 32768 , 32    );
    }

    public void testSize(int sMaxRow, int sFetchRowSize, int aExpSelectCount, int aExpCmFetchCount) throws SQLException
    {
        PreparedStatement sCheckStmt = connection().prepareStatement("SELECT COUNT FROM V$DB_PROTOCOL WHERE OP_NAME = 'CMP_OP_DB_FetchV3'");

        int sBeforeCount = ((Number)executeScalar(sCheckStmt)).intValue();

        Statement sSelStmt = connection().createStatement();
        sSelStmt.setMaxRows(sMaxRow);
        assertEquals(sMaxRow, sSelStmt.getMaxRows());
        sSelStmt.setFetchSize(sFetchRowSize);
        assertEquals(sFetchRowSize, sSelStmt.getFetchSize());
        ResultSet sRS = sSelStmt.executeQuery("SELECT c1 FROM t1");
        int sRowCnt;
        for (sRowCnt = 0; sRS.next(); sRowCnt++)
            ;
        assertEquals(aExpSelectCount, sRowCnt);

        int sAfterCount = ((Number)executeScalar(sCheckStmt)).intValue();

        assertEquals(aExpCmFetchCount, sAfterCount - sBeforeCount - 1);

        sSelStmt.close();
        sCheckStmt.close();
    }
}
