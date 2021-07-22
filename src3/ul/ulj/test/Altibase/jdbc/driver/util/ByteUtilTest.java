package Altibase.jdbc.driver.util;

import junit.framework.TestCase;

import java.nio.ByteBuffer;

import static org.junit.Assert.assertArrayEquals;

public class ByteUtilTest extends TestCase
{
    private static final byte[] BYTE_ARRAY_8 = { 0x12, 0x34, 0x56, 0x78, (byte) 0x90, (byte) 0xab, (byte) 0xcd, (byte) 0xef };
    private static final byte[] BYTE_ARRAY_2 = { 0x12, 0x34 };

    public void testToHexStringFromByteArray()
    {
        assertEquals("null", ByteUtils.toHexString((byte[])null));
        assertEquals("", ByteUtils.toHexString(new byte[] {}));
        assertEquals("00", ByteUtils.toHexString(new byte[] { 0x00 }));
        assertEquals("01", ByteUtils.toHexString(new byte[] { 0x01 }));
        assertEquals("1234", ByteUtils.toHexString(BYTE_ARRAY_2));
        assertEquals("12 34", ByteUtils.toHexString(BYTE_ARRAY_2, 1));
        assertEquals("1234567890abcdef", ByteUtils.toHexString(BYTE_ARRAY_8));
        assertEquals("12 34 56 78 90 ab cd ef", ByteUtils.toHexString(BYTE_ARRAY_8, 1));
        assertEquals("1234 5678 90ab cdef", ByteUtils.toHexString(BYTE_ARRAY_8, 2));
        assertEquals("123456 7890ab cdef", ByteUtils.toHexString(BYTE_ARRAY_8, 3));
        assertEquals("12345678 90abcdef", ByteUtils.toHexString(BYTE_ARRAY_8, 4));
        assertEquals("1234567890 abcdef", ByteUtils.toHexString(BYTE_ARRAY_8, 5));
        assertEquals("1234567890ab cdef", ByteUtils.toHexString(BYTE_ARRAY_8, 6));
        assertEquals("1234567890abcd ef", ByteUtils.toHexString(BYTE_ARRAY_8, 7));
        assertEquals("1234567890abcdef", ByteUtils.toHexString(BYTE_ARRAY_8, 8));
        assertEquals("12, 34", ByteUtils.toHexString(BYTE_ARRAY_2, 1, ", "));
        assertEquals("12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef", ByteUtils.toHexString(BYTE_ARRAY_8, 1, ", 0x"));
    }

    public void testToHexStringFromHeapByteBuffer()
    {
        ByteBuffer sByteBuffer = ByteBuffer.allocate(8);
        testToHexStringFromByteBuffer(sByteBuffer);
    }

    public void testToHexStringFromDirectByteBuffer()
    {
        ByteBuffer sByteBuffer = ByteBuffer.allocateDirect(8);
        testToHexStringFromByteBuffer(sByteBuffer);
    }

    private void testToHexStringFromByteBuffer(ByteBuffer aByteBuffer)
    {
        aByteBuffer.put(BYTE_ARRAY_8);
        aByteBuffer.flip();
        assertEquals("1234567890abcdef", ByteUtils.toHexString(aByteBuffer));
        assertEquals("34567890abcdef", ByteUtils.toHexString(aByteBuffer, 1, aByteBuffer.remaining()));
        assertEquals("34567890abcd", ByteUtils.toHexString(aByteBuffer, 1, aByteBuffer.remaining() - 1));
        aByteBuffer.get();
        assertEquals("34567890abcdef", ByteUtils.toHexString(aByteBuffer));
        aByteBuffer.clear();
        aByteBuffer.put(BYTE_ARRAY_2);
        aByteBuffer.flip();
        assertEquals("1234", ByteUtils.toHexString(aByteBuffer));
        aByteBuffer.get();
        assertEquals("34", ByteUtils.toHexString(aByteBuffer));
    }

    public void testParseNibbleArray()
    {
        assertArrayEquals(BYTE_ARRAY_2, ByteUtils.parseByteArray("1234"));
        assertArrayEquals(BYTE_ARRAY_8, ByteUtils.parseByteArray("1234567890abcdef"));
        assertArrayEquals(BYTE_ARRAY_8, ByteUtils.parseByteArray("1234567890ABCDEF"));

        assertArrayEquals(new byte[]{0x12, 0x34, 0x50}, ByteUtils.parseByteArray("12345"));
        try
        {
            ByteUtils.parseByteArray("12345", false);
            fail();
        }
        catch (IllegalArgumentException ex)
        {
            // quite
        }

        try
        {
            ByteUtils.parseByteArray("x");
            fail();
        }
        catch (IllegalArgumentException ex)
        {
            // quite
        }
    }
}
