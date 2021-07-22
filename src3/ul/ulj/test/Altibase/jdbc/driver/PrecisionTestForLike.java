   package Altibase.jdbc.driver;

import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

public class PrecisionTestForLike extends AltibaseTestCase
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
                "CREATE TABLE t1 (i1 INTEGER, i2 INTEGER, i3 VARCHAR(20), i4 VARCHAR(20), i5 VARCHAR(10), i6 CHAR(100))",
                "INSERT INTO t1 VALUES (1, 1, 'aaa', 'bbb', 'ccc', 'ddd')",
        };
    }

    // natc : TC/Interface/jdbc/Bugs/PR-13866
    // nok : http://nok.altibase.com/pages/viewpage.action?pageId=19476071
    public void testLike() throws SQLException
    {
        PreparedStatement ps = connection().prepareStatement("SELECT * FROM t1 WHERE i1=? AND i2=? AND i3 LIKE '%' || ? || '%' AND i4 LIKE '%' || ? || '%'  AND i5 LIKE '%' || ? || '%'  AND i6 LIKE '%' || ? || '%'");
        assertEquals(6, ps.getParameterMetaData().getParameterCount());
        ps.setString(1, "1");
        ps.setString(2, "1");
        ps.setString(3, "a");
        ps.setString(4, "b");
        ps.setString(5, "c");
        ps.setString(6, "d");

        ResultSet rs = ps.executeQuery();
        assertEquals(true, rs.next());
        assertEquals(1, rs.getInt(1));
        assertEquals(1, rs.getInt(2));
        assertEquals("aaa", rs.getString(3));
        assertEquals("bbb", rs.getString(4));
        assertEquals("ccc", rs.getString(5));
        assertEquals("ddd", rs.getString(6).trim());
        assertEquals(false, rs.next());
        ps.close();
    }
}
