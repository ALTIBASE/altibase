package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.DatabaseMetaData;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class DatabaseMetaDataTest extends AltibaseTestCase
{
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
                "DROP TABLE t3",
                "DROP TABLE t2",
                "DROP USER NEWJDBC_TESTUSER",
                "DROP VIEW v1",
                "DROP MATERIALIZED VIEW m1",
                "DROP SEQUENCE s1",
                "DROP QUEUE q1",
                "DROP SYNONYM a_t1",
                "DROP SYNONYM a_v1",
                "DROP SYNONYM a_m1",
                "DROP SYNONYM a_s1",
                "DROP SYNONYM a_q1",
                "DROP SYNONYM a_no",
                "DROP PROCEDURE p1",
                "DROP FUNCTION p2",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 INTEGER, c2 VARCHAR(20), c3 BIGINT)",
                "CREATE TABLE t2 (c1 INTEGER CONSTRAINT pk_t2 PRIMARY KEY, c2 VARCHAR(20), c3 BIGINT)",
                "CREATE TABLE t3 (c1 INTEGER CONSTRAINT fk_t3 REFERENCES t2(c1), c2 VARCHAR(20))",
                "CREATE USER NEWJDBC_TESTUSER IDENTIFIED BY asd123",
                "GRANT DELETE ON t2 to NEWJDBC_TESTUSER",
                "GRANT SELECT ON t3 to NEWJDBC_TESTUSER",
                "CREATE VIEW v1 AS SELECT * FROM t1",
                "CREATE MATERIALIZED VIEW m1 AS SELECT * FROM t1",
                "CREATE SEQUENCE s1",
                "CREATE QUEUE q1 (1)",
                "CREATE SYNONYM a_t1 FOR t1",
                "CREATE SYNONYM a_v1 FOR v1",
                "CREATE SYNONYM a_m1 FOR m1",
                "CREATE SYNONYM a_s1 FOR s1",
                "CREATE SYNONYM a_q1 FOR q1",
                "CREATE SYNONYM a_no FOR not_exists_obj",
                "CREATE PROCEDURE p1 (p1 IN INTEGER) AS v1 INTEGER; BEGIN SELECT p1 INTO v1 FROM dual; END",
                "CREATE FUNCTION p2 (p1 IN INTEGER, p2 OUT INTEGER) RETURN NUMBER AS BEGIN SELECT p1 INTO p2 FROM dual; RETURN p1; END",
        };
    }

    public void testGetCatalogs() throws SQLException
    {
        ResultSet sRS = connection().getMetaData().getCatalogs();
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString(1));
        assertEquals(false, sRS.next());
    }

    public void testGetSchemas() throws SQLException
    {
        ResultSet sRS = connection().getMetaData().getSchemas();
        assertEquals(true, sRS.next());
        assertEquals("NEWJDBC_TESTUSER", sRS.getString("TABLE_SCHEM"));
        assertEquals("mydb", sRS.getString("TABLE_CATALOG"));
        assertEquals(true, sRS.next());
//      assertEquals("PUBLIC", sRS.getString("TABLE_SCHEM"));  // BUG-45071 username만 포함시키기 때문에 role은 빠진다.
//      assertEquals(true, sRS.next());
        assertEquals("SYS", sRS.getString("TABLE_SCHEM"));
        assertEquals(true, sRS.next());
        assertEquals("SYSTEM_", sRS.getString("TABLE_SCHEM"));
        assertEquals(false, sRS.next());
    }

    public void testGetTables() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();
        ResultSet sRS = sMetaData.getTables("mydb", "SYS", "T1", null);
        assertEquals(true, sRS.next());
        assertEquals("SYS", sRS.getString("TABLE_SCHEM"));
        assertEquals("T1", sRS.getString("TABLE_NAME"));
        assertEquals("TABLE", sRS.getString("TABLE_TYPE"));
        assertEquals("USER", sRS.getString("REF_GENERATION"));
        assertEquals(3, sRS.getInt("COLUMN_COUNT"));
        assertEquals(false, sRS.next());
        sRS.close();
        sRS = sMetaData.getTables("mydb", null, "SYS_DUMMY_", null);
        assertEquals(true, sRS.next());
        assertEquals("SYSTEM_", sRS.getString("TABLE_SCHEM"));
        assertEquals("SYS_DUMMY_", sRS.getString("TABLE_NAME"));
        assertEquals("SYSTEM TABLE", sRS.getString("TABLE_TYPE"));
        assertEquals("SYSTEM", sRS.getString("REF_GENERATION"));
        assertEquals(1, sRS.getInt("COLUMN_COUNT"));
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void _IGNORE_testGetAllTables() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();
        ResultSet sRS;
        int sCount;

        sRS = sMetaData.getTables("mydb", null, null, null);
        for (sCount = 0; sRS.next(); sCount++)
            ;
        assertEquals(132, sCount);
        sRS.close();

        sRS = sMetaData.getTables("mydb", null, null, new String[] { "SYSTEM TABLE" });
        for (sCount = 0; sRS.next(); sCount++)
            ;
        assertEquals(66, sCount);
        sRS.close();

        sRS = sMetaData.getTables("mydb", null, null, new String[] { "TABLE" });
        for (sCount = 0; sRS.next(); sCount++)
            ;
        assertEquals(3, sCount);
        sRS.close();

        sRS = sMetaData.getTables("mydb", null, null, new String[] { "SYSTEM VIEW" });
        for (sCount = 0; sRS.next(); sCount++)
            ;
        assertEquals(6, sCount);
        sRS.close();

        sRS = sMetaData.getTables("mydb", null, null, new String[] { "VIEW" });
        assertEquals(true, sRS.next());
        assertEquals("V1", sRS.getString("TABLE_NAME"));
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getTables("mydb", null, null, new String[] { "MATERIALIZED VIEW" });
        assertEquals(true, sRS.next());
        assertEquals("M1", sRS.getString("TABLE_NAME"));
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getTables("mydb", null, null, new String[] { "QUEUE" });
        assertEquals(true, sRS.next());
        assertEquals("Q1", sRS.getString("TABLE_NAME"));
        assertEquals(false, sRS.next());
        sRS.close();

        /* BUG-45255 */
        sRS = sMetaData.getTables("mydb", null, null, new String[] { "SYNONYM" });
        for (sCount = 0; sRS.next(); sCount++)
            ;
        assertEquals(53, sCount);
        sRS.close();

        sRS = sMetaData.getTables("mydb", null, null, new String[] { "SEQUENCE" });
        assertEquals(true, sRS.next());
        assertEquals("S1", sRS.getString("TABLE_NAME"));
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void testGetTableTypes() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getTableTypes();
        ResultSetMetaData sRSMeta = sRS.getMetaData();
        assertEquals(1, sRSMeta.getColumnCount());
        assertEquals(true, sRS.next());
        assertEquals("MATERIALIZED VIEW", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("QUEUE", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("SEQUENCE", sRS.getString(1));
        /* BUG-45255 */
        assertEquals(true, sRS.next());
        assertEquals("SYNONYM", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("SYSTEM TABLE", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("SYSTEM VIEW", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("TABLE", sRS.getString(1));
        assertEquals(true, sRS.next());
        assertEquals("VIEW", sRS.getString(1));
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void testGetColumns() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();
        ResultSet sRS = sMetaData.getColumns("mydb", "SYS", "T1", null);
        assertEquals(true, sRS.next());
        assertEquals("C1", sRS.getString("COLUMN_NAME"));
        assertEquals("INTEGER", sRS.getString("TYPE_NAME"));
        assertEquals(1, sRS.getInt("ORDINAL_POSITION"));
        assertEquals(true, sRS.next());
        assertEquals("C2", sRS.getString("COLUMN_NAME"));
        assertEquals("VARCHAR", sRS.getString("TYPE_NAME"));
        assertEquals(2, sRS.getInt("ORDINAL_POSITION"));
        assertEquals(true, sRS.next());
        assertEquals("C3", sRS.getString("COLUMN_NAME"));
        assertEquals("BIGINT", sRS.getString("TYPE_NAME"));
        assertEquals(3, sRS.getInt("ORDINAL_POSITION"));
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void testGetPrimaryKeys() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS1 = sMetaData.getPrimaryKeys("mydb", "SYS", "T1");
        assertEquals(false, sRS1.next());
        sRS1.close();

        ResultSet sRS2 = sMetaData.getPrimaryKeys("mydb", "SYS", "T2");
        assertEquals(true, sRS2.next());
        assertEquals("mydb", sRS2.getString("TABLE_CAT"));
        assertEquals("SYS", sRS2.getString("TABLE_SCHEM"));
        assertEquals("T2", sRS2.getString("TABLE_NAME"));
        assertEquals("C1", sRS2.getString("COLUMN_NAME"));
        assertEquals(1, sRS2.getInt("COLUMN_CNT"));
        assertEquals(false, sRS2.next());
        sRS2.close();
    }

    public void testGetImportedKeys() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getImportedKeys("mydb", "SYS", "T2");
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getImportedKeys("mydb", "SYS", "T3");
        assertEquals(true, sRS.next());
        assertEquals("T2", sRS.getString("PKTABLE_NAME"));
        assertEquals("C1", sRS.getString("PKCOLUMN_NAME"));
        assertEquals("T3", sRS.getString("FKTABLE_NAME"));
        assertEquals("C1", sRS.getString("FKCOLUMN_NAME"));
        sRS.close();
    }

    public void testGetExportedKeys() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getExportedKeys("mydb", "SYS", "T3");
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getExportedKeys("mydb", "SYS", "T2");
        assertEquals(true, sRS.next());
        assertEquals("T2", sRS.getString("PKTABLE_NAME"));
        assertEquals("C1", sRS.getString("PKCOLUMN_NAME"));
        assertEquals("T3", sRS.getString("FKTABLE_NAME"));
        assertEquals("C1", sRS.getString("FKCOLUMN_NAME"));
        sRS.close();
    }

    public void testGetCrossReference() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getCrossReference("mydb", "SYS", "T2", "mydb", "SYS", "T1");
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getCrossReference("mydb", "SYS", "T2", "mydb", "SYS", "T3");
        assertEquals(true, sRS.next());
        assertEquals("T2", sRS.getString("PKTABLE_NAME"));
        assertEquals("C1", sRS.getString("PKCOLUMN_NAME"));
        assertEquals("T3", sRS.getString("FKTABLE_NAME"));
        assertEquals("C1", sRS.getString("FKCOLUMN_NAME"));
        sRS.close();

        sRS = sMetaData.getCrossReference("mydb", "SYS", "T3", "mydb", "SYS", "T2");
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void testGetIndexInfo() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getIndexInfo("mydb", "SYS", "T1", true, false);
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getIndexInfo("mydb", "SYS", "T2", true, false);
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("TABLE_CAT"));
        assertEquals("SYS", sRS.getString("TABLE_SCHEM"));
        assertEquals("T2", sRS.getString("TABLE_NAME"));
        assertEquals(false, sRS.getBoolean("NON_UNIQUE"));
        assertEquals(null, sRS.getString("INDEX_QUALIFIER"));
        assertEquals("PK_T2", sRS.getString("INDEX_NAME"));
        assertEquals(1, sRS.getShort("TYPE"));
        assertEquals(1, sRS.getShort("ORDINAL_POSITION"));
        assertEquals("C1", sRS.getString("COLUMN_NAME"));
        sRS.close();
    }

    public void testGetBestRowIdentifier() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getBestRowIdentifier("mydb", "SYS", "T1", 0, false);
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getBestRowIdentifier("mydb", "SYS", "T2", 0, false);
        assertEquals(true, sRS.next());
        assertEquals(2, sRS.getInt("SCOPE"));
        assertEquals("C1", sRS.getString("COLUMN_NAME"));
        assertEquals("INTEGER", sRS.getString("TYPE_NAME"));
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getBestRowIdentifier("mydb", "SYS", "T3", 0, false);
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void testGetTablePrivileges() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getTablePrivileges("mydb", "SYS", "T1");
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getTablePrivileges("mydb", "SYS", "T2");
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("TABLE_CAT"));
        assertEquals("SYS", sRS.getString("TABLE_SCHEM"));
        assertEquals("T2", sRS.getString("TABLE_NAME"));
        assertEquals("SYS", sRS.getString("GRANTOR"));
        assertEquals("NEWJDBC_TESTUSER", sRS.getString("GRANTEE"));
        assertEquals("DELETE", sRS.getString("PRIVILEGE"));
        assertEquals("NO", sRS.getString("IS_GRANTABLE"));
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getTablePrivileges("mydb", "SYS", "T3");
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("TABLE_CAT"));
        assertEquals("SYS", sRS.getString("TABLE_SCHEM"));
        assertEquals("T3", sRS.getString("TABLE_NAME"));
        assertEquals("SYS", sRS.getString("GRANTOR"));
        assertEquals("NEWJDBC_TESTUSER", sRS.getString("GRANTEE"));
        assertEquals("SELECT", sRS.getString("PRIVILEGE"));
        assertEquals("NO", sRS.getString("IS_GRANTABLE"));
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getTablePrivileges(null, null, null);
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("TABLE_CAT"));
        assertEquals("SYS", sRS.getString("TABLE_SCHEM"));
        assertEquals("T2", sRS.getString("TABLE_NAME"));
        assertEquals("SYS", sRS.getString("GRANTOR"));
        assertEquals("NEWJDBC_TESTUSER", sRS.getString("GRANTEE"));
        assertEquals("DELETE", sRS.getString("PRIVILEGE"));
        assertEquals("NO", sRS.getString("IS_GRANTABLE"));
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("TABLE_CAT"));
        assertEquals("SYS", sRS.getString("TABLE_SCHEM"));
        assertEquals("T3", sRS.getString("TABLE_NAME"));
        assertEquals("SYS", sRS.getString("GRANTOR"));
        assertEquals("NEWJDBC_TESTUSER", sRS.getString("GRANTEE"));
        assertEquals("SELECT", sRS.getString("PRIVILEGE"));
        assertEquals("NO", sRS.getString("IS_GRANTABLE"));
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void testGetProcedures() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getProcedures("mydb", "SYS", "P1");
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("PROCEDURE_CAT"));
        assertEquals("SYS", sRS.getString("PROCEDURE_SCHEM"));
        assertEquals("P1", sRS.getString("PROCEDURE_NAME"));
        assertEquals(1, sRS.getInt("NUM_INPUT_PARAMS"));
        assertEquals(0, sRS.getInt("NUM_OUTPUT_PARAMS"));
        assertEquals(1, sRS.getInt("PROCEDURE_TYPE"));
        assertEquals(false, sRS.next());

        sRS = sMetaData.getProcedures("mydb", "SYS", null);
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("PROCEDURE_CAT"));
        assertEquals("SYS", sRS.getString("PROCEDURE_SCHEM"));
        assertEquals("P1", sRS.getString("PROCEDURE_NAME"));
        assertEquals(1, sRS.getInt("NUM_INPUT_PARAMS"));
        assertEquals(0, sRS.getInt("NUM_OUTPUT_PARAMS"));
        assertEquals(1, sRS.getInt("PROCEDURE_TYPE"));
        assertEquals(true, sRS.next());
        assertEquals("mydb", sRS.getString("PROCEDURE_CAT"));
        assertEquals("SYS", sRS.getString("PROCEDURE_SCHEM"));
        assertEquals("P2", sRS.getString("PROCEDURE_NAME"));
        assertEquals(1, sRS.getInt("NUM_INPUT_PARAMS"));
        assertEquals(1, sRS.getInt("NUM_OUTPUT_PARAMS"));
        assertEquals(2, sRS.getInt("PROCEDURE_TYPE"));
        assertEquals(false, sRS.next());
    }

    public void testGetProcedureColumns() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getProcedureColumns("mydb", "SYS", "P1", null);
        assertEquals(true, sRS.next());
        assertEquals("P1", sRS.getString("PROCEDURE_NAME"));
        assertEquals("P1", sRS.getString("COLUMN_NAME"));
        assertEquals(1, sRS.getInt("COLUMN_TYPE"));
        assertEquals(1, sRS.getInt("ORDINAL_POSITION"));
        assertEquals(false, sRS.next());
        sRS.close();

        sRS = sMetaData.getProcedureColumns("mydb", "SYS", "P2", null);
        assertEquals(true, sRS.next());
        assertEquals("P2", sRS.getString("PROCEDURE_NAME"));
        assertEquals("RETURN_VALUE", sRS.getString("COLUMN_NAME"));
        assertEquals(5, sRS.getInt("COLUMN_TYPE"));
        assertEquals(0, sRS.getInt("ORDINAL_POSITION"));
        assertEquals(true, sRS.next());
        assertEquals("P2", sRS.getString("PROCEDURE_NAME"));
        assertEquals("P1", sRS.getString("COLUMN_NAME"));
        assertEquals(1, sRS.getInt("COLUMN_TYPE"));
        assertEquals(1, sRS.getInt("ORDINAL_POSITION"));
        assertEquals(true, sRS.next());
        assertEquals("P2", sRS.getString("PROCEDURE_NAME"));
        assertEquals("P2", sRS.getString("COLUMN_NAME"));
        assertEquals(4, sRS.getInt("COLUMN_TYPE"));
        assertEquals(2, sRS.getInt("ORDINAL_POSITION"));
        assertEquals(false, sRS.next());
        sRS.close();
    }

    public void testGetProcedureColumnsAll() throws SQLException
    {
        DatabaseMetaData sMetaData = connection().getMetaData();

        ResultSet sRS = sMetaData.getProcedureColumns("mydb", "SYS", null, null);
        assertEquals(true, sRS.next());
        assertEquals("P1", sRS.getString("PROCEDURE_NAME"));
        assertEquals("P1", sRS.getString("COLUMN_NAME"));
        assertEquals(true, sRS.next());
        assertEquals("P2", sRS.getString("PROCEDURE_NAME"));
        assertEquals("RETURN_VALUE", sRS.getString("COLUMN_NAME"));
        assertEquals(true, sRS.next());
        assertEquals("P2", sRS.getString("PROCEDURE_NAME"));
        assertEquals("P1", sRS.getString("COLUMN_NAME"));
        assertEquals(true, sRS.next());
        assertEquals("P2", sRS.getString("PROCEDURE_NAME"));
        assertEquals("P2", sRS.getString("COLUMN_NAME"));
        assertEquals(false, sRS.next());
        sRS.close();
    }

    private static final String[] TYPE_NAMES   = { "VARBIT", "NCHAR", "NVARCHAR", "BIT", "BIGINT", "BINARY", "CHAR", "NUMERIC", "INTEGER", "SMALLINT", "FLOAT", "REAL", "DOUBLE", "VARCHAR", "DATE", "BLOB", "CLOB", "NUMBER", "GEOMETRY", "BYTE", "NIBBLE", "VARBYTE" };
    private static final int[]    DATA_TYPES = { -100, -15, -9, -7, -5, -2, 1, 2, 4, 5, 6, 7, 8, 12, 93, 2004, 2005, 10002, 10003, 20001, 20002, 20003 };

    public void testGetTypeInfo() throws SQLException
    {
        ResultSet sRS = connection().getMetaData().getTypeInfo();
        for (int i=0; i<TYPE_NAMES.length; i++)
        {
            assertEquals(true, sRS.next());
            assertEquals(TYPE_NAMES[i], sRS.getString("TYPE_NAME"));
            assertEquals(DATA_TYPES[i], sRS.getInt("DATA_TYPE"));
        }
        assertEquals(false, sRS.next());
    }

    public void testGetAttributes() throws SQLException
    {
        ResultSet sRS = connection().getMetaData().getAttributes(null, null, null, null);
        assertEquals(false, sRS.next());
    }

    public void testGetSuperTables() throws SQLException
    {
        ResultSet sRS = connection().getMetaData().getSuperTables(null, null, null);
        assertEquals(false, sRS.next());
    }

    public void testGetSuperTypes() throws SQLException
    {
        ResultSet sRS = connection().getMetaData().getSuperTypes(null, null, null);
        assertEquals(false, sRS.next());
    }

    public void testGetColumnPrivileges()
    {
        try
        {
            connection().getMetaData().getColumnPrivileges(null, null, null, null);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.UNSUPPORTED_FEATURE, sEx.getErrorCode());
        }
    }

    public void testGetUDTs()
    {
        try
        {
            connection().getMetaData().getUDTs(null, null, null, null);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.UNSUPPORTED_FEATURE, sEx.getErrorCode());
        }
    }

    public void testGetSQLKeywords()
    {
        try
        {
            String sKeywordsFromMetaData = connection().getMetaData().getSQLKeywords();
            List<String> sKeywordsList = new ArrayList<String>(Arrays.asList(
                                                sKeywordsFromMetaData.split("\\s*,\\s*")));
            List<String> sKeywordsFromQp = new ArrayList<String>(Keywords.mKeywordsList);
            sKeywordsFromQp.removeAll(AltibaseDatabaseMetaData.getSql92Keywords());
            Collections.sort(sKeywordsList);
            assertEquals(sKeywordsList, sKeywordsFromQp);
        }
        catch (SQLException sEx)
        {
            sEx.printStackTrace();
            fail();
        }
    }
}
