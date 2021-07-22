package Altibase.jdbc.driver;

import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.sql.*;

/**
 * Created by yjpark on 17. 2. 10.
 */
public class ClobCharStreamTest extends AltibaseTestCase
{
    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 int, c2 clob)"
        };
    }

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
        };
    }

    public void testSetCharStream() throws Exception
    {
        Statement sStmt = null;
        ResultSet sRs = null;

        insertClobData();

        try
        {
            connection().setAutoCommit(false);
            sStmt = connection().createStatement();
            sRs = sStmt.executeQuery("SELECT * FROM t1");
            if (sRs.next())
            {
                String sValue = makeStrFromClobData(sRs);
                assertEquals(65530, sValue.length());
            }
        }
        finally
        {
            if (sRs != null)    { try { sRs.close();    } catch (Exception ex) {} }
            if (sStmt != null)  { try { sStmt.close();  } catch (Exception ex) {} }
        }
    }

    private void insertClobData() throws SQLException
    {
        char[] sCharArry = new char[65530];
        for (int i = 0; i < 65530; i++)
        {
            sCharArry[i] = 'a';
        }
        connection().setAutoCommit(false);
        PreparedStatement sPstmt = connection().prepareStatement("INSERT INTO t1 VALUES (?, ?)");
        sPstmt.setInt(1, 0);
        sPstmt.setCharacterStream(2, new StringReader(new String(sCharArry)), sCharArry.length);
        sPstmt.executeUpdate();
        sPstmt.close();
        connection().commit();
    }

    private String makeStrFromClobData(ResultSet aRs) throws Exception
    {
        int sBufferSize = 1024;
        StringBuilder sStringBuilder = null;
        Clob sClob;
        Reader sReader = null;
        char[] sReadBuffer = new char[sBufferSize];
        int sResultSize;
        try
        {
            sStringBuilder = new StringBuilder();
            sClob = aRs.getClob(2);

            sReader = sClob.getCharacterStream();

            while ((sResultSize = sReader.read(sReadBuffer)) != -1)
            {
                if (sResultSize == sBufferSize)
                {
                    sStringBuilder.append(sReadBuffer);
                }
                else
                {
                    char[] aTemp = new char[sResultSize];
                    for (int i = 0; i < sResultSize; i++)
                    {
                        sStringBuilder.append(sReadBuffer[i]);
                        aTemp[i] = sReadBuffer[i];
                    }
                }
            }
        }
        finally
        {
            try
            {
                sReader.close();
            }
            catch (IOException e)
            {
            }
        }

        return sStringBuilder.toString();
    }
}
