package Altibase.jdbc.driver.util;

import junit.framework.TestCase;

public class AltibasePropertiesTest extends TestCase
{
    public void testStringConv()
    {
        AltibaseProperties sProp1 = new AltibaseProperties();
        sProp1.setServer("localhost");
        sProp1.setPort(12345);
        assertEquals(2, sProp1.size());
        assertEquals("{port=12345, server=localhost}", sProp1.toString());
        AltibaseProperties sProp2 = AltibaseProperties.valueOf("{port=12345, server=localhost}");
        assertEquals(2, sProp2.size());
        assertEquals(sProp1, sProp2);
        assertEquals(sProp1.getServer(), sProp2.getServer());
        assertEquals(sProp1.getPort(), sProp2.getPort());

        AltibaseProperties sProp3 = AltibaseProperties.valueOf("{asd=123}");
        assertEquals(1, sProp3.size());
        assertEquals("123", sProp3.get("asd"));

        AltibaseProperties sProp4 = AltibaseProperties.valueOf("{asd=123 asd}");
        assertEquals(1, sProp4.size());
        assertEquals("123 asd", sProp4.get("asd"));

        AltibaseProperties sProp5 = AltibaseProperties.valueOf("{asd=(asd=1, qwe=2), qwe=234, zxc=3456}");
        assertEquals(3, sProp5.size());
        assertEquals("3456", sProp5.get("zxc"));
        assertEquals("234", sProp5.get("qwe"));
        assertEquals("(asd=1, qwe=2)", sProp5.get("asd"));

        try
        {
            AltibaseProperties.valueOf("{123=asd}");
            fail();
        }
        catch (IllegalArgumentException sEx)
        {
        }
    }
}
