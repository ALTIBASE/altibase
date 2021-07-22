package Altibase.jdbc.driver;

import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.AltibaseProperties;

import java.sql.Driver;
import java.sql.DriverManager;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;

public class DriverTest extends AltibaseTestCase
{
    protected boolean useConnectionPreserve()
    {
        return false;
    }

    private AltibaseProperties testParseUrl(String aURL, String aDSN, String aServer, String aPort, String aDBName) throws SQLException
    {
        AltibaseProperties sProp = new AltibaseProperties();
        AltibaseUrlParser.parseURL(aURL, sProp);
        assertEquals(aDSN, sProp.getDataSource());
        assertEquals(aServer, sProp.getServer());
        assertEquals(aPort, sProp.getProperty(AltibaseProperties.PROP_PORT));
        assertEquals(aDBName, sProp.getDatabase());

        // Properties to URL 검증
        if (aURL.indexOf(AltibaseVersion.CM_VERSION_STRING) == -1)
        {
            aURL = aURL.replaceAll("jdbc:Altibase://", "jdbc:Altibase_" + AltibaseVersion.CM_VERSION_STRING + "://");
        }
        String sURL2 = AltibaseConnection.getURL(sProp);
        if (!aURL.equals(sURL2))
        {
            // properties는 순서가 바뀔 수도 있다. 그럴땐 각 요소를 따로 검사해야한다.
            assertEquals(aURL.length(), sURL2.length());

            AltibaseProperties sProp2 = new AltibaseProperties();
            AltibaseUrlParser.parseURL(sURL2, sProp2);
            assertEquals(sProp.keySet().size(), sProp2.keySet().size());
            assertEquals(aDSN, sProp2.getDataSource());
            assertEquals(aServer, sProp2.getServer());
            assertEquals(aPort, sProp2.getProperty(AltibaseProperties.PROP_PORT));
            assertEquals(aDBName, sProp2.getDatabase());
        }
        return sProp;
    }

    public void testSimpleURL() throws SQLException
    {
        testParseUrl("jdbc:Altibase://127.0.0.1:123/mydb2",
                     null, "127.0.0.1", "123", "mydb2");

        try
        {
            // no DbName
            testParseUrl("jdbc:Altibase://127.0.0.1:123", null, null, null, null);
            fail();
        }
        catch (SQLException sEx)
        {
            assertEquals(ErrorDef.INVALID_CONNECTION_URL, sEx.getErrorCode());
        }
    }

    public void testDSN() throws SQLException
    {
        testParseUrl("jdbc:Altibase://rndux",
                     "rndux", null, null, null);

        // port override
        testParseUrl("jdbc:Altibase://rndux:20311",
                     "rndux", null, "20311", null);

        // property override
        AltibaseProperties sProp = testParseUrl("jdbc:Altibase://rndux?ncharliteralreplace=true",
                                                "rndux", null, null, null);
        assertEquals("true", sProp.getProperty("ncharliteralreplace"));
    }

    public void testManyProperties() throws SQLException
    {
        AltibaseProperties sProp = testParseUrl("jdbc:Altibase://127.0.0.1:123/mydb?prop1=val1&prop2=2&prop3=val3",
                                                null, "127.0.0.1", "123", "mydb");
        assertEquals("val1", sProp.getProperty("prop1"));
        assertEquals("2", sProp.getProperty("prop2"));
        assertEquals("val3", sProp.getProperty("prop3"));
    }

    public void testVersionSpecifiedURL() throws SQLException
    {
        testParseUrl("jdbc:Altibase_" + AltibaseVersion.CM_VERSION_STRING + "://127.0.0.1:123/mydb2",
                     null, "127.0.0.1", "123", "mydb2");
    }

    public void testNullURL()
    {
        AltibaseProperties sProp = new AltibaseProperties();
        try
        {
            AltibaseUrlParser.parseURL(null, sProp);
            fail();
        }
        catch (Exception sEx)
        {
            // quite
        }
    }

    public void testPropertyInfo() throws SQLException
    {
        String sURL = "jdbc:Altibase://127.0.0.1:1234/yourdb";
        Driver sDriver = DriverManager.getDriver(sURL);
        DriverPropertyInfo[] sPropInfo = sDriver.getPropertyInfo(sURL, null);
        assertEquals(39, sPropInfo.length);
        sPropInfo = sDriver.getPropertyInfo(sURL + "?prop1=val1", null);
        assertEquals(39, sPropInfo.length);
    }
}
