package Altibase.jdbc.driver.util;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;

import Altibase.jdbc.driver.util.AltiSqlProcessor;
import junit.framework.TestCase;

public class AltiSqlProcessorTest extends TestCase
{
    private static final String SIMPLE_SELECT = "SELECT * FROM dual";

    public static void testIsSelect()
    {
        assertEquals(true, AltiSqlProcessor.isSelectQuery(SIMPLE_SELECT));
        assertEquals(true, AltiSqlProcessor.isSelectQuery("   " + SIMPLE_SELECT));
        assertEquals(true, AltiSqlProcessor.isSelectQuery("\t" + SIMPLE_SELECT));
        assertEquals(true, AltiSqlProcessor.isSelectQuery("\r" + SIMPLE_SELECT));
        assertEquals(true, AltiSqlProcessor.isSelectQuery("\n" + SIMPLE_SELECT));
        assertEquals(true, AltiSqlProcessor.isSelectQuery("-- asdasd\n" + SIMPLE_SELECT));
        assertEquals(true, AltiSqlProcessor.isSelectQuery("/* asd */" + SIMPLE_SELECT));
        assertEquals(true, AltiSqlProcessor.isSelectQuery("// asdasd\n" + SIMPLE_SELECT));
    }

    public static void testMakeRowSetSql()
    {
        assertRowSetSql("SELECT * ,_PROWID FROM t1 WHERE _PROWID IN (?)",
                        "SELECT * FROM t1");
        assertRowSetSql("SELECT * ,_PROWID FROM t1 WHERE _PROWID IN (?)",
                        " SELECT * FROM t1");
        assertRowSetSql("SELECT (SELECT 1 FROM 2 WHERE 3) ,_PROWID FROM t1 WHERE _PROWID IN (?)",
                        "SELECT (SELECT 1 FROM 2 WHERE 3) FROM t1");
        assertRowSetSql("SELECT c1 ,_PROWID FROM t1 a WHERE _PROWID IN (?)",
                        "SELECT c1 FROM t1 a WHERE c1 = ?");
        assertRowSetSql("SELECT c1 ,_PROWID FROM t1 AS a WHERE _PROWID IN (?)",
                        "SELECT c1 FROM t1 AS a WHERE c1 = ?");
        assertRowSetSql("SELECT c1 ,_PROWID FROM t1 WHERE _PROWID IN (?)",
                        "SELECT c1 FROM t1 ORDER BY c1");
        assertRowSetSql("SELECT c1 ,_PROWID FROM t1 WHERE _PROWID IN (?)",
                        "SELECT c1 FROM t1 LIMIT 1");
    }

    public void testGetNativeSql()
    {
        java.util.Date curdate = new java.util.Date();
        java.text.SimpleDateFormat sdf =
                new java.text.SimpleDateFormat(
                "yy-MM-dd hh:mm:ss");
        String datetext = "{ts '" + sdf.format(curdate) + "'}";
        assertEquals("UPDATE MYTABLE SET DATECOL =  to_date('" + sdf.format(curdate) + "', 'yyyy-MM-dd hh24:mi:ss')", "UPDATE MYTABLE SET DATECOL = " + AltiSqlProcessor.processEscape(datetext));
    }
    
    private static void assertRowSetSql(String e, String o)
    {
        try
        {
            assertEquals(e, AltiSqlProcessor.makeRowSetSql(o, 1));
        } catch (SQLException e1)
        {
            fail();
        }
    }

    public static void testMakePRowIDAddedSql()
    {
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM t1",
                             "SELECT * FROM t1");
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM t1",
                             " SELECT * FROM t1");
        assertPRowIDAddedSql("SELECT\"C1\",_PROWID FROM t1",
                             "SELECT\"C1\"FROM t1");
        assertPRowIDAddedSql("SELECT(c1),_PROWID FROM t1",
                             "SELECT(c1)FROM t1");
        assertPRowIDAddedSql("SELECT c1/* block comment */,_PROWID FROM t1",
                             "SELECT c1/* block comment */FROM t1");
        assertPRowIDAddedSql("SELECT c1-- line comment\n,_PROWID FROM t1",
                             "SELECT c1-- line comment\nFROM t1");
        assertPRowIDAddedSql("SELECT c1// c++ line comment\n,_PROWID FROM t1",
                             "SELECT c1// c++ line comment\nFROM t1");
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM\"T1\"",
                             "SELECT * FROM\"T1\"");
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM(SELECT 1 FROM DUAL)",
                             "SELECT * FROM(SELECT 1 FROM DUAL)");
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM/* block comment */t1",
                             "SELECT * FROM/* block comment */t1");
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM-- line comment\nt1",
                             "SELECT * FROM-- line comment\nt1");
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM// c++ line comment\nt1",
                             "SELECT * FROM// c++ line comment\nt1");
        assertPRowIDAddedSql("SELECT * ,_PROWID FROM t1 WHERE a=2",
                             "SELECT * FROM t1 WHERE a=2");
        assertPRowIDAddedSql("SELECT t1.* ,_PROWID FROM t1",
                             "SELECT t1.* FROM t1");
        assertPRowIDAddedSql("SELECT t1.* as a ,_PROWID FROM t1",
                             "SELECT t1.* as a FROM t1");
        assertPRowIDAddedSql("SELECT \"FROM\" ,_PROWID FROM t1",
                             "SELECT \"FROM\" FROM t1");
    }

    private static void assertPRowIDAddedSql(String e, String o)
    {
        try
        {
            assertEquals(e, AltiSqlProcessor.makePRowIDAddedSql(o));
        } catch (SQLException e1)
        {
            fail();
        }
    }

    public void testGetAllSequences()
    {
        assertSequences(new String[] { "seq" }, "INSERT INTO t1 VALUES (seq.nextval)");
        assertSequences(new String[] { "seq" }, "INSERT INTO t1 VALUES (asdfghj, seq.nextval)");
        assertSequences(new String[] { "seq" }, "INSERT INTO t1 VALUES (seq.nextval, asdfghj)");
        assertSequences(new String[] { "\"SeQ\"" }, "INSERT INTO t1 VALUES (\"SeQ\".nextval)");
        assertSequences(new String[] { "\"SeQ\"" }, "INSERT INTO t1 VALUES (asdfghj, \"SeQ\".nextval)");
        assertSequences(new String[] { "\"SeQ\"" }, "INSERT INTO t1 VALUES (\"SeQ\".nextval, asdfghj)");
    }

    private void assertSequences(String[] aExpSeqs, String aQstr)
    {
        ArrayList sActInfos = AltiSqlProcessor.getAllSequences(aQstr);
        assertEquals(aExpSeqs.length, sActInfos.size());
        for (int i = 0; i < aExpSeqs.length; i++)
        {
            AltiSqlProcessor.SequenceInfo sSeqInfo = (AltiSqlProcessor.SequenceInfo)sActInfos.get(i);
            assertEquals(aExpSeqs[i], sSeqInfo.mSeqName);
        }
    }

    public void testMakeKeySetSql() throws SQLException
    {
        assertEquals("SELECT _PROWID FROM t1", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1", null));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY c1", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY c1", null));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY c1 ASC", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY c1 ASC", null));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY c1 DESC", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY c1 DESC", null));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\"", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY \"C1\"", null));

        HashMap sOrderByMap = new HashMap();
        sOrderByMap.put("1", "C1");
        sOrderByMap.put("2", "C2");
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\"", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY 1", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER /* comment */ BY \"C1\"", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER /* comment */ BY -- comment\n // comment\n /* comment */ 1", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" ASC", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY 1 ASC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" DESC", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY 1 DESC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\",\"C2\"", AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 1, 2", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" ASC,\"C2\"", AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 1 ASC, 2", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" DESC,\"C2\"", AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 1 DESC, 2", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\",\"C2\" ASC", AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 1, 2 ASC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\",\"C2\" DESC", AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 1, 2 DESC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" ASC,\"C2\" ASC", AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 1 ASC, 2 ASC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" DESC,\"C2\" DESC", AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 1 DESC, 2 DESC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY '123'", AltiSqlProcessor.makeKeySetSql("SELECT c1 FROM t1 ORDER BY '123'", sOrderByMap));
        try
        {
            AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY 3", sOrderByMap);
        }
        catch (AssertionError sErr)
        {
        }
        try
        {
            AltiSqlProcessor.makeKeySetSql("SELECT c1, c2 FROM t1 ORDER BY a1", sOrderByMap);
        }
        catch (AssertionError sErr)
        {
        }
        sOrderByMap.put("A1", "C1"); 
        sOrderByMap.put("A2", "C2");
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\"", AltiSqlProcessor.makeKeySetSql("SELECT c1 a1 FROM t1 ORDER BY a1", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" ASC", AltiSqlProcessor.makeKeySetSql("SELECT c1 a1 FROM t1 ORDER BY a1 ASC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" DESC", AltiSqlProcessor.makeKeySetSql("SELECT c1 a1 FROM t1 ORDER BY a1 DESC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\",\"C2\"", AltiSqlProcessor.makeKeySetSql("SELECT c1 as a1, c2 a2 FROM t1 ORDER BY a1, a2", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" ASC,\"C2\"", AltiSqlProcessor.makeKeySetSql("SELECT c1 as a1, c2 a2 FROM t1 ORDER BY a1 ASC, a2", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" DESC,\"C2\"", AltiSqlProcessor.makeKeySetSql("SELECT c1 as a1, c2 a2 FROM t1 ORDER BY a1 DESC, a2", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\",\"C2\" ASC", AltiSqlProcessor.makeKeySetSql("SELECT c1 as a1, c2 a2 FROM t1 ORDER BY a1, a2 ASC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\",\"C2\" DESC", AltiSqlProcessor.makeKeySetSql("SELECT c1 as a1, c2 a2 FROM t1 ORDER BY a1, a2 DESC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" ASC,\"C2\" ASC", AltiSqlProcessor.makeKeySetSql("SELECT c1 as a1, c2 a2 FROM t1 ORDER BY a1 ASC, a2 ASC", sOrderByMap));
        assertEquals("SELECT _PROWID FROM t1 ORDER BY \"C1\" DESC,\"C2\" DESC", AltiSqlProcessor.makeKeySetSql("SELECT c1 as a1, c2 a2 FROM t1 ORDER BY a1 DESC, a2 DESC", sOrderByMap));
    }
}
