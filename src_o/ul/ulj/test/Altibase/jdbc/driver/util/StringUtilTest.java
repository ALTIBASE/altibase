package Altibase.jdbc.driver.util;

import junit.framework.TestCase;

public class StringUtilTest extends TestCase
{
    public void testIsEmpty()
    {
        assertEquals(true, StringUtils.isEmpty(null));
        assertEquals(true, StringUtils.isEmpty(""));
        assertEquals(false, StringUtils.isEmpty(" "));
    }

    public void testStartsWithIgnoreCaseStringString()
    {
        assertEquals(true, StringUtils.startsWithIgnoreCase("ASDzxdQWE123", "asd"));
        assertEquals(false, StringUtils.startsWithIgnoreCase("ASDzxdQWE123", "ad"));
        assertEquals(true, StringUtils.startsWithIgnoreCase("ASDzxdQWE123", "ASD"));
        assertEquals(true, StringUtils.startsWithIgnoreCase("ASDzxdQWE123", "AsD"));
        assertEquals(true, StringUtils.startsWithIgnoreCase("ASDzxdQWE123", 1, "sD"));
        assertEquals(true, StringUtils.startsWithIgnoreCase("ASDzxdQWE123", 1, "Sdz"));
        assertEquals(false, StringUtils.startsWithIgnoreCase("ASDzxdQWE123", 1, "Sdd"));
    }

    public void testCompareIgnoreCaseStringString()
    {
        assertEquals(0, StringUtils.compareIgnoreCase("asd", "asd"));
        assertEquals(0, StringUtils.compareIgnoreCase("asd", "ASD"));
        assertEquals(0, StringUtils.compareIgnoreCase("asd", "AsD"));
        assertEquals(1, StringUtils.compareIgnoreCase("asd", "asc"));
        assertEquals(1, StringUtils.compareIgnoreCase("asd", "asa")); // 많이 차이나도 1
        assertEquals(-1, StringUtils.compareIgnoreCase("asd", "ase"));
        assertEquals(-1, StringUtils.compareIgnoreCase("asd", "asz")); // 많이 차이나도 -1
        assertEquals(1, StringUtils.compareIgnoreCase("asdf", "asd"));
        assertEquals(1, StringUtils.compareIgnoreCase("asdfghijk", "asd"));
        assertEquals(-1, StringUtils.compareIgnoreCase("asd", "asdfghijk"));
        assertEquals(0, StringUtils.compareIgnoreCase("asdf", 0, "asd", 0, 3));
        assertEquals(0, StringUtils.compareIgnoreCase("0asd", 1, "asd"));
        assertEquals(0, StringUtils.compareIgnoreCase("0asd", 1, "asd", 0, 3));
    }

}
