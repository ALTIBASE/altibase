package Altibase.jdbc.driver;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.CallableStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;


public class MultipleResultSetTest extends AltibaseTestCase
{
    public void testGetMoreResult() throws SQLException, InterruptedException
    {
        connection().setAutoCommit(false);
        CallableStatement stmt = connection().prepareCall("EXEC PROC1('abcde')");
        try
        {
            stmt.executeQuery();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.MULTIPLE_RESULTSET_RETURNED, sEx.getErrorCode());
        }
        stmt.close();

        stmt = connection().prepareCall("EXEC PROC1('abcde')");
        stmt.execute();
        
        ResultSet sRs;
        ResultSetMetaData sRsMd;
        int sResultSetCnt = 0;
        do
        {
            sResultSetCnt++;
            sRs = stmt.getResultSet();
            if (sRs != null)
            {
                sRsMd = sRs.getMetaData();
                if (sRsMd != null)
                {
                    while (sRs.next())
                    {
                        // do something
                        for (int i = 1; i <= sRsMd.getColumnCount(); i++)
                        {
                            assertEquals(( i == 1) ? "abcde" : "abc" , sRs.getString(i));
                        }
                    }
                }
            }
            
        } while(stmt.getMoreResults());

        assertEquals(2, sResultSetCnt);

        stmt.close();
    }

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE fetchtest",
                "drop procedure proc1",
                "drop typeset my_type"
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE FETCHTEST (c1 VARCHAR(10), c2 varchar(10))",
                "INSERT INTO FETCHTEST VALUES ('abc', 'abc')",
                "INSERT INTO FETCHTEST VALUES ('abc', 'abc')",
                "CREATE TYPESET MY_TYPE AS TYPE MY_CUR IS REF CURSOR; END;",
                "create or replace procedure PROC1 (P1 out MY_TYPE.MY_CUR, P4 in VARCHAR(10)) as begin OPEN P1 FOR 'SELECT * FROM fetchtest'; UPDATE fetchtest SET C1= P4; end;",
        };
    }

}
