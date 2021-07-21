package Altibase.jdbc.driver.util;

import junit.framework.TestCase;

import static org.junit.Assert.assertArrayEquals;

public class NibbleUtilTest extends TestCase
{
    private static final byte[] NIBBLE_ARRAY_16 = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
    private static final byte[] NIBBLE_ARRAY_2  = { 0x1, 0x2 };
    private static final byte[] NIBBLE_ARRAY_3  = { 0x1, 0x2, 0x3 };

    public void testIsNibble()
    {
        for (int i = Short.MIN_VALUE; i < Short.MAX_VALUE; i++)
        {
            assertEquals(0 <= i && i <= 0xF, NibbleUtils.isNibble(i));
        }
    }

    public void testIsValid()
    {
        assertEquals(true, NibbleUtils.isValid(NIBBLE_ARRAY_2));
        assertEquals(true, NibbleUtils.isValid(NIBBLE_ARRAY_3));
        assertEquals(true, NibbleUtils.isValid(NIBBLE_ARRAY_16));

        assertEquals(false, NibbleUtils.isValid(new byte[] { 0x10 }));
        assertEquals(false, NibbleUtils.isValid(new byte[] { 0x0, 0xF, 0x10 }));
    }

    public void testToHexString()
    {
        assertEquals("null", NibbleUtils.toHexString(null));
        assertEquals("", NibbleUtils.toHexString(new byte[] {}));
        assertEquals("0", NibbleUtils.toHexString(new byte[] { 0x0 }));
        assertEquals("1", NibbleUtils.toHexString(new byte[] { 0x1 }));
        assertEquals("12", NibbleUtils.toHexString(NIBBLE_ARRAY_2));
        assertEquals("1 2", NibbleUtils.toHexString(NIBBLE_ARRAY_2, 1));
        assertEquals("1234567890abcdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16));
        assertEquals("1 2 3 4 5 6 7 8 9 0 a b c d e f", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 1));
        assertEquals("12 34 56 78 90 ab cd ef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 2));
        assertEquals("123 456 789 0ab cde f", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 3));
        assertEquals("1234 5678 90ab cdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 4));
        assertEquals("12345 67890 abcde f", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 5));
        assertEquals("123456 7890ab cdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 6));
        assertEquals("1234567 890abcd ef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 7));
        assertEquals("12345678 90abcdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 8));
        assertEquals("123456789 0abcdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 9));
        assertEquals("1234567890 abcdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 10));
        assertEquals("1234567890a bcdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 11));
        assertEquals("1234567890ab cdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 12));
        assertEquals("1234567890abc def", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 13));
        assertEquals("1234567890abcd ef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 14));
        assertEquals("1234567890abcde f", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 15));
        assertEquals("1234567890abcdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 16));
        assertEquals("234567890abcdef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 1, NIBBLE_ARRAY_16.length));
        assertEquals("234567890abcde", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 1, NIBBLE_ARRAY_16.length - 1));
        assertEquals("1, 2", NibbleUtils.toHexString(NIBBLE_ARRAY_2, 1, ", "));
        assertEquals("12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef", NibbleUtils.toHexString(NIBBLE_ARRAY_16, 2, ", 0x"));
    }

    public void testParseNibbleArray()
    {
        assertArrayEquals(null, NibbleUtils.parseNibbleArray(null));
        assertArrayEquals(new byte[0], NibbleUtils.parseNibbleArray(""));
        assertArrayEquals(NIBBLE_ARRAY_2, NibbleUtils.parseNibbleArray("12"));
        assertArrayEquals(NIBBLE_ARRAY_3, NibbleUtils.parseNibbleArray("123"));
        assertArrayEquals(NIBBLE_ARRAY_16, NibbleUtils.parseNibbleArray("1234567890abcdef"));

        try
        {
            NibbleUtils.parseNibbleArray("x");
            fail();
        }
        catch (IllegalArgumentException ex)
        {
            // quite
        }
    }

    public void testToByteArray()
    {
        assertArrayEquals(null, NibbleUtils.toByteArray(null));
        assertArrayEquals(new byte[0], NibbleUtils.toByteArray(new byte[0]));
        assertArrayEquals(new byte[] { 0x12 }, NibbleUtils.toByteArray(NIBBLE_ARRAY_2));
        assertArrayEquals(new byte[] { 0x12, 0x30 }, NibbleUtils.toByteArray(NIBBLE_ARRAY_3));
        assertArrayEquals(new byte[] { 0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xab, (byte)0xcd, (byte)0xef }, NibbleUtils.toByteArray(NIBBLE_ARRAY_16));

        try
        {
            NibbleUtils.toByteArray(new byte[] { 0x10 });
            fail();
        }
        catch (IllegalArgumentException ex)
        {
            // quite
        }
    }

    public void testFromByteArray()
    {
        assertArrayEquals(null, NibbleUtils.fromByteArray(null));
        assertArrayEquals(new byte[0], NibbleUtils.fromByteArray(new byte[0]));
        assertArrayEquals(new byte[0], NibbleUtils.fromByteArray(new byte[] { 0x12 }, 0));
        assertArrayEquals(NIBBLE_ARRAY_2, NibbleUtils.fromByteArray(new byte[] { 0x12 }));
        assertArrayEquals(new byte[] { 0x1, 0x2, 0x3, 0x0 }, NibbleUtils.fromByteArray(new byte[] { 0x12, 0x30 }));
        assertArrayEquals(NIBBLE_ARRAY_3, NibbleUtils.fromByteArray(new byte[] { 0x12, 0x30 }, 3));
        assertArrayEquals(NIBBLE_ARRAY_16, NibbleUtils.fromByteArray(new byte[] { 0x12, 0x34, 0x56, 0x78, (byte)0x90, (byte)0xab, (byte)0xcd, (byte)0xef }));
    }
}
