package Altibase.jdbc.driver.datatype;

import java.io.FileInputStream;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Types;

import Altibase.jdbc.driver.AltibaseTestCase;
import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.util.ByteUtils;

public class GeometryColumnTest extends AltibaseTestCase
{
    private static final byte[] POINT_10_10 = new byte[]{
        (byte)209,7,1,1,56,0,0,0,0,0,0,0,0,0,36,64,0,0,0,0,0,0,36,64,0,0,0,0,0,0,36,64,0,0,0,0,0,0,36,64,0,0,0,0,0,0,36,64,0,0,0,0,0,0,36,64
    };
    private static final byte[] POINT_1_1 = new byte[]{
        (byte)209,7,1,1,56,0,0,0,0,0,0,0,0,0,(byte)240,63,0,0,0,0,0,0,(byte)240,63,0,0,0,0,0,0,(byte)240,63,0,0,0,0,0,0,(byte)240,63,0,0,0,0,0,0,(byte)240,63,0,0,0,0,0,0,(byte)240,63
    };
    private static byte[] LARGE_GEOMETRY = null;

    static
    {
        try
        {
            InputStream sIn = new FileInputStream("test/resources/LargeGeometry.bin");
            LARGE_GEOMETRY = new byte[sIn.available()];
            sIn.read(LARGE_GEOMETRY);
        }
        catch (Exception sEx)
        {
            fail();
        }
    }

    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE t1",
        };
    }

    protected String[] getInitQueries()
    {
        return new String[] {
                "CREATE TABLE t1 (c1 GEOMETRY(64008))"
        };
    }

    public void testSetNullByNULL() throws SQLException
    {
        testSetNullBy(Types.NULL);
    }

    public void testSetNullByGEOMETRY() throws SQLException
    {
        testSetNullBy(AltibaseTypes.GEOMETRY);
    }

    public void testSetNullByBINARY() throws SQLException
    {
        testSetNullBy(AltibaseTypes.BINARY);
    }

    private void testSetNullBy(int aJDBCType) throws SQLException
    {
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sInsStmt.setObject(1, null, aJDBCType);
        assertEquals(1, sInsStmt.executeUpdate());
        sInsStmt.close();

        Statement sSelStmt = connection().createStatement();
        ResultSet sRS = sSelStmt.executeQuery("SELECT * FROM t1");
        assertEquals(true, sRS.next());
        assertEquals(null, sRS.getObject(1));
        assertEquals(false, sRS.next());
        sSelStmt.close();
    }

    // 에러가 떨어진다: Conversion not applicable
    // 리터럴로 GEOMETRY'POINT(1 1)'처럼 넣는건 되지만, 파라미터로는 안된다.
    // 무조건 바이너리로 변환한 뒤 넣어야 한다.
    // 나중에라도 JDBC에서 GEOMETRY를 지원한다면, WKT로 입력할 수도 있게 할건지 고민해봐야 할 것.
    public void _NOTSUPOORT_testInsertByWKT() throws Exception
    {
        PreparedStatement sStmt;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setString(1, "POINT(10 10)");
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("POINT(10 10)");
        sStmt.close();

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setString(1, "POINT(1 1)");
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("POINT(1 1)");

        sStmt.setString(1, null);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(null);

        sStmt.close();
    }

    // BUG-44738 endian 관련 diff가 발생할 수 있기때문에 skip처리
    public void _SKIP_testInsertByBinary() throws SQLException
    {
        testInsertBy(Types.BINARY);
    }

    // BUG-44738 endian 관련 diff가 발생할 수 있기때문에 skip처리
    public void _SKIP_testInsertByGeometry() throws SQLException
    {
        testInsertBy(AltibaseTypes.GEOMETRY);
    }

    private void testInsertBy(int aType) throws SQLException
    {
        PreparedStatement sStmt;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (?)");
        sStmt.setObject(1, POINT_10_10, aType);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(ByteUtils.toHexString(POINT_10_10));
        sStmt.close();

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = ?");
        sStmt.setObject(1, POINT_1_1, aType);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(ByteUtils.toHexString(POINT_1_1));

        // test for colum data split
        sStmt.setObject(1, LARGE_GEOMETRY, aType);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(ByteUtils.toHexString(LARGE_GEOMETRY));

        sStmt.setObject(1, null, aType);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar(null);

        sStmt.close();
    }

    public void testBatch() throws SQLException
    {
        byte[][] WKB_DATS = {
                null,
                { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 0, 1, 64, 24, 0, 0, 0, 0, 0, 0, 64, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 0, 1, 63, -16, 0, 0, 0, 0, 0, 0, 63, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 64, 20, 0, 0, 0, 0, 0, 0, 64, 20, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 0, 1, 64, 24, 0, 0, 0, 0, 0, 0, 64, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 64, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 0, 1, 64, 20, 0, 0, 0, 0, 0, 0, 64, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 63, -16, 0, 0, 0, 0, 0, 0, 63, -16, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 0, 1, 64, 36, 0, 0, 0, 0, 0, 0, 64, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 64, 42, 0, 0, 0, 0, 0, 0, 64, 42, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 0, 1, 64, 34, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 64, 24, 0, 0, 0, 0, 0, 0, 64, 36, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 0, 1, 64, 51, 0, 0, 0, 0, 0, 0, 64, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 64, 42, 0, 0, 0, 0, 0, 0, 64, 42, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 1, 63, -16, 0, 0, 0, 0, 0, 0, 63, -16, 0, 0, 0, 0, 0, 0 },
                { 0, 0, 0, 0, 1, 63, -16, 0, 0, 0, 0, 0, 0, 63, -16, 0, 0, 0, 0, 0, 0 },
                null,
        };

        PreparedStatement sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (GEOMFROMWKB(?))");
        for (int i = 0; i < WKB_DATS.length; i++)
        {
            sStmt.setObject(1, WKB_DATS[i], AltibaseTypes.GEOMETRY);
            sStmt.addBatch();
        }
        int[] sResult = sStmt.executeBatch();
        assertEquals(WKB_DATS.length, sResult.length);
        for (int i = 0; i < sResult.length; i++)
        {
            assertEquals(1, sResult[i]);
        }
        sStmt.close();
    }

    public void _NOTSUPOORT_testInsertFromWKBByBinary() throws SQLException
    {
        testInsertFromWKBBy(Types.BINARY);
    }

    public void testInsertFromWKBByGeometry() throws SQLException
    {
        testInsertFromWKBBy(AltibaseTypes.GEOMETRY);
    }

    public void testInsertFromWKBBy(int aType) throws SQLException
    {
        PreparedStatement sStmt;

        sStmt = connection().prepareStatement("INSERT INTO t1 VALUES (GEOMFROMWKB(?))");
        byte[] sWKBPoint_2_5 = createWKBPoint(2, 5);
        sStmt.setObject(1, sWKBPoint_2_5, aType);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("POINT(2 5) ", "ASTEXT(c1)");
        sStmt.close();

        sStmt = connection().prepareStatement("UPDATE t1 SET c1 = GEOMFROMWKB(?)");
        byte[] sWKBPoint_3_6 = createWKBPoint(3, 6);
        sStmt.setObject(1, sWKBPoint_3_6, aType);
        assertEquals(1, sStmt.executeUpdate());
        assertExecuteScalar("POINT(3 6) ", "ASTEXT(c1)");
        sStmt.close();
    }

    // #region WKB 함수

    private static final int  WKB_TYPE_POINT = 1;
    private static final byte WKB_ENDIAN_BE  = 0;
    private static final byte WKB_ENDIAN_LE  = 1;

    /**
     * WKB POINT(x y)를 생성한다.
     * <p>
     * WKB POINT의 구성은 다음과 같다:
     * <code><pre>
     * wkb_point
     * {
     *      byte   byte_order;
     *      int    wkb_type;
     *      double x;
     *      double y;
     * }
     * </pre></code>
     * 
     * @param aPointX x 값
     * @param aPointY y 값
     * @return x, y를 점으로 하는 WKB POINT
     */
    private static byte[] createWKBPoint(double aPointX, double aPointY)
    {
        ByteBuffer sBuf = ByteBuffer.allocate(1 + 4 + 8 + 8);
        sBuf.order(ByteOrder.nativeOrder()); // Adjust Byte Buffer with Server
        sBuf.put(getByteOrder());
        sBuf.putInt(WKB_TYPE_POINT);
        sBuf.putDouble(aPointX);
        sBuf.putDouble(aPointY);
        return sBuf.array();
    }

    private static byte getByteOrder()
    {
        return (ByteOrder.nativeOrder() == ByteOrder.BIG_ENDIAN) ? WKB_ENDIAN_BE : WKB_ENDIAN_LE;
    }

    // #endregion
}
