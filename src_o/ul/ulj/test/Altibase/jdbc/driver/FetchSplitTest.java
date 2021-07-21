package Altibase.jdbc.driver;
import java.io.ByteArrayInputStream;
import java.sql.Connection;
import java.sql.Date;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Types;
import java.util.BitSet;
import java.util.Hashtable;
import java.util.Map;
import java.util.Random;

class DB_TYPE
{
    public String mTypeName;
    public int mMaxLength;
    
    DB_TYPE (String aTypeName, int aMaxLength)
    {
        mTypeName = aTypeName;
        mMaxLength = aMaxLength;
    }
}

public class FetchSplitTest extends AltibaseTestCase
{
    public static String TYPE_CHAR = "CHAR";
    public static String TYPE_VARCHAR = "VARCHAR";
    public static String TYPE_NCHAR = "NCHAR";
    public static String TYPE_NVARCHAR = "NVARCHAR";
    public static String TYPE_CLOB = "CLOB";
    public static String TYPE_BIGINT = "BIGINT";
    public static String TYPE_DECIMAL = "DECIMAL";
    public static String TYPE_DOUBLE = "DOUBLE";
    public static String TYPE_FLOAT = "FLOAT";
    public static String TYPE_INTEGER = "INTEGER";
    public static String TYPE_NUMBER = "NUMBER";
    public static String TYPE_NUMERIC = "NUMERIC";
    public static String TYPE_REAL = "REAL";
    public static String TYPE_SMALLINT = "SMALLINT";
    public static String TYPE_DATE = "DATE";
    public static String TYPE_BLOB = "BLOB";
    public static String TYPE_BYTE = "BYTE";
    public static String TYPE_NIBBLE = "NIBBLE";
    public static String TYPE_BIT = "BIT";
    public static String TYPE_VARBIT = "VARBIT";
    public static String TYPE_GEOMETRY = "GEOMETRY";
    public static String TYPE_INTERVAL = "INTERVAL";
    public static String TYPE_BINARY = "BINARY";
    
    public static Map mDataTypeList = new Hashtable();
    
    public static final int BUF_LENGTH = 32768;
    public static final int PACKET_HEADER_LENGTH = 16;
    
    public static int[] length = new int[3];
    
    protected String[] getCleanQueries()
    {
        return new String[] {
                "DROP TABLE FETCHTEST",
        };
    }
    
    public void testCHAR() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_CHAR, true);
        insertData4Fragmentation(connection(), TYPE_CHAR);
        select(connection(), TYPE_CHAR);
    }
    
    public void testVARCHAR() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_VARCHAR, true);
        insertData4Fragmentation(connection(), TYPE_VARCHAR);
        select(connection(), TYPE_VARCHAR);
    }
    
    public void testBIGINT() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_BIGINT, false);
        insertData4Fragmentation(connection(), TYPE_BIGINT);
        select(connection(), TYPE_BIGINT);
    }
    
    public void testBINARY() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_BINARY, true);
        insertData4Fragmentation(connection(), TYPE_BINARY);
        select(connection(), TYPE_BINARY);
    }
    
    public void testBIT() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_BIT, true);
        insertData4Fragmentation(connection(), TYPE_BIT);
        select(connection(), TYPE_BIT);
    }
    
    public void testBYTE() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_BYTE, true);
        insertData4Fragmentation(connection(), TYPE_BYTE);
        select(connection(), TYPE_BYTE);
    }
    
    public void testDATE() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_DATE, false);
        insertData4Fragmentation(connection(), TYPE_DATE);
        select(connection(), TYPE_DATE);
    }
    
    public void testDECIMAL() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_DOUBLE, false);
        insertData4Fragmentation(connection(), TYPE_DOUBLE);
        select(connection(), TYPE_DECIMAL);
    }
    
    public void testFLOAT() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_FLOAT, false);
        insertData4Fragmentation(connection(), TYPE_FLOAT);
        select(connection(), TYPE_FLOAT);
    }
    
    public void testINTEGER() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_INTEGER, false);
        insertData4Fragmentation(connection(), TYPE_INTEGER);
        select(connection(), TYPE_INTEGER);
    }
    
    public void testINTERVAL() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_INTERVAL, false);
        insertData4Fragmentation(connection(), TYPE_INTERVAL);
        select(connection(), TYPE_INTERVAL);
    }
    
    public void testNCHAR() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_NCHAR, true);
        insertData4Fragmentation(connection(), TYPE_NCHAR);
        select(connection(), TYPE_NCHAR);
    }
    
    public void testNIBBLE() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_NIBBLE, true);
        insertData4Fragmentation(connection(), TYPE_NIBBLE);
        select(connection(), TYPE_NIBBLE);
    }
    
    public void testNUMBER() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_NUMBER, true);
        insertData4Fragmentation(connection(), TYPE_NUMBER);
        select(connection(), TYPE_NUMBER);
    }
    
    public void testNUMERIC() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_NUMERIC, true);
        insertData4Fragmentation(connection(), TYPE_NUMERIC);
        select(connection(), TYPE_NUMERIC);
    }
    
    public void testNVARCHAR() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_NVARCHAR, true);
        insertData4Fragmentation(connection(), TYPE_NVARCHAR);
        select(connection(), TYPE_NVARCHAR);
    }
    
    public void testREAL() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_REAL, false);
        insertData4Fragmentation(connection(), TYPE_REAL);
        select(connection(), TYPE_REAL);
    }
    
    public void testSMALLINT() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_SMALLINT, false);
        insertData4Fragmentation(connection(), TYPE_SMALLINT);
        select(connection(), TYPE_SMALLINT);
    }
    
    public void testVARBIT() throws Exception
    {
        initDataTypes();
        createTable4Fragmentation(connection(), TYPE_VARBIT, true);
        insertData4Fragmentation(connection(), TYPE_VARBIT);
        select(connection(), TYPE_VARBIT);
    }

    private static void select(Connection aConn, String aType) throws Exception
    {
        ResultSet rs = null;
        Statement stmt = null;

        String sTestQuery4Interval = "SELECT C1, C2, (C3-C3) FROM FETCHTEST";
        String sTestQuery4Normal = "select C1, C2, C3 from fetchtest";

        stmt = aConn.createStatement();
        if (aType.equals(TYPE_INTERVAL))
        {
            rs = stmt.executeQuery(sTestQuery4Interval);
        } else
        {
            rs = stmt.executeQuery(sTestQuery4Normal);
        }
        try
        {
            while (rs.next())
            {
                for (int i = 1; i <= 3; i++)
                {
                    if (i == 3)
                    {
                        if (aType.equals(TYPE_BLOB))
                        {
                            // System.out.println("3rd Column : "
                            // + rs.getBlob(i).getBytes(1, 5));
                        } else
                        {
                            // System.out.println("3rd Column : "
                            // + rs.getString(i).length());
                        }
                    } else
                    {
                        // System.out.println(rs.getString(i).length());
                    }
                }
            }
        } catch (Exception e)
        {
            fail();
        }
        rs.close();
        stmt.close();
    }
    
    private void initDataTypes() throws SQLException
    {
        Statement stmt = connection().createStatement();
        ResultSet rs = stmt.executeQuery("SELECT TYPE_NAME, COLUMN_SIZE FROM V$DATATYPE");
        while(rs.next())
        {
            mDataTypeList.put(rs.getString(1), new DB_TYPE(rs.getString(1), rs.getInt(2)));
        }
        rs.close();
        stmt.close();
    }
    
    private void createTable4Fragmentation(Connection aConn, String aFragmentationTypeName, boolean hasLength) throws SQLException
    {
        int sLength = BUF_LENGTH;
        int s3rdColumnLength = 0;
        
        StringBuffer sb = new StringBuffer("CREATE TABLE FETCHTEST (");
        
        sLength -= PACKET_HEADER_LENGTH; 
        sLength -= ((DB_TYPE)mDataTypeList.get(TYPE_VARCHAR)).mMaxLength;
        
        if(aFragmentationTypeName.equals(TYPE_INTERVAL))
        {
            s3rdColumnLength = 539 + 22;
//            System.out.println(TYPE_INTERVAL);
            aFragmentationTypeName = TYPE_DATE;
        }
        else if(aFragmentationTypeName.equals(TYPE_SMALLINT))
        {
            s3rdColumnLength = 539 + 2;
        }
        else
        {
            s3rdColumnLength = 539;
        }
        
        if(hasLength)
        {
            if(aFragmentationTypeName.equals(TYPE_NUMBER))
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                .append(s3rdColumnLength)
                .append("), C3 ")
                .append(aFragmentationTypeName)
                .append("(38))");
            }
            else if(aFragmentationTypeName.equals(TYPE_NUMERIC))
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                .append(s3rdColumnLength)
                .append("), C3 ")
                .append(aFragmentationTypeName)
                .append("(38,5))");
            }
            else if(aFragmentationTypeName.equals(TYPE_BIT) || aFragmentationTypeName.equals(TYPE_VARBIT))
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                .append(s3rdColumnLength)
                .append("), C3 ")
                .append(aFragmentationTypeName)
                .append("(5))");
            }
            else if(aFragmentationTypeName.equals(TYPE_BINARY))
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                .append(s3rdColumnLength)
                .append("), C3 ")
                .append("GEOMETRY")
                .append(")");
            }
            else if(aFragmentationTypeName.equals(TYPE_NCHAR))
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                .append(s3rdColumnLength)
                .append("), C3 ")
                .append("NCHAR(8000") // utf8을 고려
                .append("))");
            }
            else if(aFragmentationTypeName.equals(TYPE_NVARCHAR))
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                .append(s3rdColumnLength)
                .append("), C3 ")
                .append("NVARCHAR(8000") // utf8을 고려
                .append("))");
            }            
            else if(aFragmentationTypeName.equals(TYPE_NIBBLE))
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                .append(s3rdColumnLength)
                .append("), C3 ")
                .append("NIBBLE(254")
                .append("))");
            }
            else
            {
                sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
                        .append(s3rdColumnLength)
                        .append("), C3 ")
                        .append(aFragmentationTypeName)
                        .append("(")
                        .append(((DB_TYPE) mDataTypeList.get(TYPE_VARCHAR)).mMaxLength)
                        .append("))");
            }
        }
        else
        {
            sb.append("C1 VARCHAR(32000), C2 VARCHAR(")
            .append(s3rdColumnLength)
            .append("), C3 ")
            .append(aFragmentationTypeName)
            .append(")");
        }
        
        length[0] = 32000;
        length[1] = s3rdColumnLength;
        length[2] = ((DB_TYPE) mDataTypeList.get(aFragmentationTypeName)).mMaxLength;
        
        Statement stmt = aConn.createStatement();
        
        stmt.execute(sb.toString());
        
        stmt.close();
    }
    
    private void insertNullData4Fragmentation(Connection conn, String aFragmentationTypeName) throws SQLException
    {
        StringBuffer sb = new StringBuffer("INSERT INTO FETCHTEST VALUES (");
        for(int i=1; i<3; i++) 
        {
            sb.append("?,");
        }
        sb.append("?)");
        PreparedStatement pstmt = conn.prepareStatement(sb.toString());
        
        StringBuffer buffer = new StringBuffer();
        Random random = new Random(); 
          
        String chars[] = "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z".split(",");
        
        // To create whole rows
        for (int k = 0; k < 5; k++) {
            // To create a row
            for (int j = 1; j <= 3; j++) {
                // To create a data which is dataLength long
                
                
                if(j==3) {
                    if(aFragmentationTypeName.equals(TYPE_VARCHAR))
                    {
                        for (int i = 0; i < length[j-1]; i++) {
                            buffer.append(chars[random.nextInt(chars.length)]);
                        }
                        
                        pstmt.setString(j, buffer.toString());
                        
                    }
                    else if(aFragmentationTypeName.equals(TYPE_CHAR))
                    {
                        for (int i = 0; i < length[j-1]; i++) {
                            buffer.append(chars[random.nextInt(chars.length)]);
                        }
                        
                        pstmt.setString(j, buffer.toString());
                        
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BIGINT))
                    {
                        pstmt.setNull(j, Types.BIGINT);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_DATE))
                    {
                        pstmt.setNull(j, Types.DATE);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_DOUBLE))
                    {
                        pstmt.setNull(j, Types.DOUBLE);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_INTEGER))
                    {
                        pstmt.setNull(j, Types.INTEGER);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BLOB))
                    {
                        pstmt.setNull(j, Types.BLOB);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_SMALLINT))
                    {
                        pstmt.setNull(j, Types.SMALLINT);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_REAL))
                    {
                        pstmt.setNull(j, Types.REAL);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_FLOAT))
                    {
                        pstmt.setNull(j, Types.FLOAT);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_NUMBER))
                    {
                        pstmt.setNull(j, Types.NUMERIC);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_NUMERIC))
                    {
                        pstmt.setNull(j, Types.NUMERIC);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BIT))
                    {
                        pstmt.setNull(j, Types.BIT);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_NIBBLE))
                    {
                        pstmt.setNull(j, Types.NULL);
                    }
                }
                else
                {
                    for (int i = 0; i < length[j-1]; i++) {
                        buffer.append(chars[random.nextInt(chars.length)]);
                    }
                    
                    pstmt.setString(j, buffer.toString());
                }
                
                buffer.setLength(0);
            }
            pstmt.executeUpdate();
        }
        pstmt.close();
    }
    
    private void insertData4Fragmentation(Connection aConn, String aFragmentationTypeName) throws SQLException
    {
        StringBuffer sb = new StringBuffer("INSERT INTO FETCHTEST VALUES (");
        for(int i=1; i<3; i++) 
        {
            sb.append("?,");
        }
        sb.append("?)");
        PreparedStatement pstmt = aConn.prepareStatement(sb.toString());
        
        StringBuffer buffer = new StringBuffer();
        Random random = new Random(); 
          
        String chars[] = "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z".split(",");
        
        // To create whole rows
        for (int k = 0; k < 5; k++) {
            // To create a row
            for (int j = 1; j <= 3; j++) {
                // To create a data which is dataLength long
                
                
                if(j==3) {
                    if(aFragmentationTypeName.equals(TYPE_VARCHAR))
                    {
                        for (int i = 0; i < length[j-1]; i++) {
                            buffer.append(chars[random.nextInt(chars.length)]);
                        }
                        
                        pstmt.setString(j, buffer.toString());
                        
                    }
                    else if(aFragmentationTypeName.equals(TYPE_CHAR))
                    {
                        for (int i = 0; i < length[j-1]; i++) {
                            buffer.append(chars[random.nextInt(chars.length)]);
                        }
                        
                        pstmt.setString(j, buffer.toString());
                        
                    }
                    else if(aFragmentationTypeName.equals(TYPE_NCHAR) || aFragmentationTypeName.equals(TYPE_NVARCHAR))
                    {
                        for (int i = 0; i < length[j-1]; i++) {
                        }
                        buffer.append(chars[random.nextInt(chars.length)]);
                        
                        pstmt.setString(j, buffer.toString());
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BIGINT))
                    {
                        pstmt.setInt(j, 0x7FFFFFFF);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_DATE) || aFragmentationTypeName.equals(TYPE_INTERVAL))
                    {
                        pstmt.setDate(j, new Date(System.currentTimeMillis()));
                    }
                    else if(aFragmentationTypeName.equals(TYPE_DOUBLE))
                    {
                        pstmt.setDouble(j, Double.MAX_VALUE);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_INTEGER))
                    {
                        pstmt.setInt(j, Integer.MAX_VALUE);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BLOB))
                    {
                        pstmt.setBinaryStream(j, new ByteArrayInputStream(new byte[]{2,1,5,0,0}), 5);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_SMALLINT))
                    {
                        pstmt.setShort(j, Short.MAX_VALUE);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_REAL))
                    {
                        pstmt.setFloat(j, Float.MAX_VALUE);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_FLOAT))
                    {
                        pstmt.setFloat(j, Float.MAX_VALUE);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_NUMBER))
                    {
                        pstmt.setString(j, "1.1111");
                    }
                    else if(aFragmentationTypeName.equals(TYPE_NUMERIC))
                    {
                        pstmt.setString(j, "1.1111");
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BIT) || aFragmentationTypeName.equals(TYPE_VARBIT))
                    {
                        BitSet bs = new BitSet(5);
                        bs.set(1);
                        pstmt.setObject(j, bs, Types.BIT);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BYTE))
                    {
                        pstmt.setByte(j, (byte)0x7F);
                    }
                    else if(aFragmentationTypeName.equals(TYPE_NIBBLE))
                    {
                        pstmt.setString(j, "1111");
                    }
                    else if(aFragmentationTypeName.equals(TYPE_BINARY))
                    {
                        byte[] in = new byte[]{-47, 7, 1, 1, 56, 0, 0, 0, 0, 0, 0, 0, 0, 0, -16, 63, 0, 0, 0, 0, 0, 0, -16, 63, 0, 0, 0, 0, 0, 0, -16, 63, 0, 0, 0, 0, 0, 0, -16, 63, 0, 0, 0, 0, 0, 0, -16, 63, 0, 0, 0, 0, 0, 0, -16, 63};
                        pstmt.setObject(j, in, AltibaseTypes.GEOMETRY);
                    }
                }
                else
                {
                    for (int i = 0; i < length[j-1]; i++) {
                        buffer.append(chars[random.nextInt(chars.length)]);
                    }
                    
                    pstmt.setString(j, buffer.toString());
                }
                
                buffer.setLength(0);
            }
            pstmt.executeUpdate();
        }
        pstmt.close();
    }

    public void testSplitReadWithFragmentedChar() throws SQLException
    {
        Statement sStmt = connection().createStatement();
        sStmt.execute("CREATE TABLE fetchtest (c1 VARCHAR(32000), c2 VARCHAR(32000), c3 INTEGER)");

        StringBuffer sStrBuf = new StringBuffer();
        while (sStrBuf.length() < 16000)
        {
            sStrBuf.append("가"); // 한글로 채워서 1문자가 쪼개질 수 있도록 한다.
        }
        String sVal1 = sStrBuf.toString();
        String sVal2 = sStrBuf.toString();
        PreparedStatement sInsStmt = connection().prepareStatement("INSERT INTO fetchtest VALUES (?, ?, ?)");
        sInsStmt.setString(1, sVal1);
        sInsStmt.setString(2, sVal2);
        sInsStmt.setInt(3, 3);
        assertEquals(1, sInsStmt.executeUpdate());

        ResultSet sRS = sStmt.executeQuery("SELECT * FROM fetchtest");
        assertEquals(true, sRS.next());
        assertEquals(sVal1, sRS.getString(1));
        assertEquals(sVal2, sRS.getString(2));
        assertEquals(3, sRS.getInt(3));
        sRS.close();
    }
}
