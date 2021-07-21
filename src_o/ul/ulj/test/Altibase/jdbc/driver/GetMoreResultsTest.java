package Altibase.jdbc.driver;
import java.sql.CallableStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

import Altibase.jdbc.driver.ex.ErrorDef;


public class GetMoreResultsTest extends AltibaseTestCase
{
    public ResultSet rs1 = null;
    public ResultSet rs2 = null;
    
    public void testGetMoreResult() throws SQLException, InterruptedException
    {
        connection().setAutoCommit(false);
        CallableStatement stmt = connection().prepareCall("EXEC PROC1('abcde')");
        try
        {
            stmt.executeQuery();
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.MULTIPLE_RESULTSET_RETURNED, sEx.getErrorCode());
        }
        stmt.close();

        stmt = connection().prepareCall("EXEC PROC2()");
        rs1 = stmt.executeQuery();
        assertEquals(false, rs1.next());
        rs1.close();
        stmt.close();

        stmt = connection().prepareCall("EXEC PROC1('abcde')");
        stmt.executeUpdate(); // executeUpdate로도 ResultSet이 있는 쿼리 수행을 허용 (oracle-like)
        rs1 = stmt.getResultSet();
        assertEquals(false, rs1.next());
        rs1.close();
        stmt.close();
        
        stmt = connection().prepareCall("EXEC PROC1('abcde')");
        stmt.setFetchSize(1);
        assertEquals(true, stmt.execute());
        
        rs1 = stmt.getResultSet();
        ResultSet rsTarget = stmt.getResultSet();

        assertEquals(rs1, rsTarget);
        assertEquals(true, stmt.getMoreResults());
        rs2 = stmt.getResultSet();
        assertNotSame(rs1, rs2);
        assertEquals(false, stmt.getMoreResults());
        
        Thread t1 = new Thread(new Runnable() {
            
            public void run() {
                int count = 0;
                try {
                    while (rs1.next()) {
                        count++;
                    }                    
                    rs1.close();
                } catch (SQLException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                    fail();
                } 
            }
        });
        Thread t2 = new Thread(new Runnable() {
            
            public void run() {
                int count = 0;
                try {
                    while (rs2.next()) {
                        count++;
                    }                
                    rs2.close();
                } catch (SQLException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                    fail();
                } 
            }
        });
        
        t1.start();
        t2.start();
        t1.join();
        t2.join();
        stmt.close();
    }

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE fetchtest",
                "DROP TABLE fetchtest2",
                "drop procedure proc1",
                "drop procedure proc2",
                "drop typeset my_type"
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE FETCHTEST (c1 VARCHAR(10), c2 varchar(10))",
                "CREATE TABLE FETCHTEST2 (c1 VARCHAR(10), c2 varchar(10))",
//                "INSERT INTO FETCHTEST VALUES ('abc', 'abc')",
//                "INSERT INTO FETCHTEST VALUES ('abc', 'abc')",
//                "INSERT INTO FETCHTEST2 VALUES ('abc', 'abc')",
//                "INSERT INTO FETCHTEST2 VALUES ('abc', 'abc')",
                "CREATE TYPESET MY_TYPE AS TYPE MY_CUR IS REF CURSOR; END;",
                "create or replace procedure PROC1 (P1 out MY_TYPE.MY_CUR, P2 out MY_TYPE.MY_CUR, P3 in VARCHAR(10)) as begin OPEN P1 FOR 'SELECT * FROM fetchtest'; UPDATE fetchtest2 SET C1= P3; end;",
                "create or replace procedure PROC2 (P1 out MY_TYPE.MY_CUR) as begin OPEN P1 FOR 'SELECT * FROM fetchtest'; end;",
        };
    }

}
