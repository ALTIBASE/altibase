package Altibase.jdbc.driver.datatype;

import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Types;

import Altibase.jdbc.driver.AltibaseTestCase;
import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.ex.ErrorDef;

public class ValueLengthExceedsTest extends AltibaseTestCase
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
                "CREATE TABLE T1 (c1 CHAR)",
        };
    }

    public void testSetOverflowLengthValue() throws SQLException
    {
        PreparedStatement sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        StringBuffer sBuf = new StringBuffer(ColumnConst.MAX_CHAR_LENGTH + 1);
        while (sBuf.length() < ColumnConst.MAX_CHAR_LENGTH)
        {
            sBuf.append("asdqwezxcvbnfghrtyuiojklm,.1234567890");
        }
        sBuf.setLength(ColumnConst.MAX_CHAR_LENGTH + 1);
        String sLargeString = sBuf.toString();
        assertEquals(sLargeString.length(), ColumnConst.MAX_CHAR_LENGTH + 1);
        try
        {
            sStmt.setObject(1, sLargeString, Types.CHAR);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.VALUE_LENGTH_EXCEEDS, sEx.getErrorCode());
        }
        try
        {
            sStmt.setObject(1, sLargeString, Types.VARCHAR);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.VALUE_LENGTH_EXCEEDS, sEx.getErrorCode());
        }
        try
        {
            byte[] sLargeBytes = new byte[ColumnConst.MAX_BYTE_LENGTH + 1];
            sStmt.setObject(1, sLargeBytes, AltibaseTypes.BYTE);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.VALUE_LENGTH_EXCEEDS, sEx.getErrorCode());
        }
        try
        {
            byte[] sLargeNibble = new byte[ColumnConst.MAX_NIBBLE_LENGTH + 1];
            sStmt.setObject(1, sLargeNibble, AltibaseTypes.NIBBLE);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.VALUE_LENGTH_EXCEEDS, sEx.getErrorCode());
        }
    }
}
