package Altibase.jdbc.driver;

import junit.framework.TestCase;

import java.math.BigDecimal;
import java.math.BigInteger;

import static org.junit.Assert.assertArrayEquals;

public class AltibaseNumericTest extends TestCase
{
    private static final byte[] MANTISSA_123_256  = new byte[] { 0x7B };
    private static final byte[] MANTISSA_123_100  = new byte[] { 0x01, 0x17 };
    private static final byte[] MANTISSA_123_10   = new byte[] { 0x01, 0x02, 0x03 };
    private static final byte[] MANTISSA_n123_100 = new byte[] { 0x62, 0x4c };

    public void _TMP_testBigDecimal()
    {
        String[] VALUES = {
                "123", "1.23", "12.3",
                "-123", "-1.23", "-12.3",
        };

        for (int i = 0; i < VALUES.length; i++)
        {
            BigDecimal nc = new BigDecimal(VALUES[i]);
            System.out.println(nc + "\tscale: " + nc.scale() + "\tunscaledValue: " + nc.unscaledValue());
        }
    }

    public void testConvertFromBigDecimal()
    {
        assertConvertFromBigDecimal(MANTISSA_123_100, new BigDecimal("123"));
        assertConvertFromBigDecimal(MANTISSA_123_100, new BigDecimal("1.23"));
        assertConvertFromBigDecimal(MANTISSA_123_100, new BigDecimal("12.3"));
        assertConvertFromBigDecimal(MANTISSA_n123_100, new BigDecimal("-123"));
        assertConvertFromBigDecimal(MANTISSA_n123_100, new BigDecimal("-1.23"));
        assertConvertFromBigDecimal(MANTISSA_n123_100, new BigDecimal("-12.3"));
    }

    private void assertConvertFromBigDecimal(byte[] aExpectedMantissa, BigDecimal aBigDecValue)
    {
        BigInteger sUnscaledValue = aBigDecValue.unscaledValue();
        if (aBigDecValue.signum() < 0)
        {
            sUnscaledValue = sUnscaledValue.negate();
        }
        byte[] sHexMantisa = sUnscaledValue.toByteArray();
        AltibaseNumeric sSrcNum = new AltibaseNumeric(256, sHexMantisa, sHexMantisa.length);
        AltibaseNumeric sDstNum = new AltibaseNumeric(100, sSrcNum);
        if (aBigDecValue.signum() < 0)
        {
            sDstNum.negate();
        }
        assertArrayEquals(aExpectedMantissa, sDstNum.toByteArray());
    }

    public void testConvertRadix()
    {
        assertConvertRadix(new byte[] { 0x01 }, 0x100, new byte[] { 0x01 }, 100);
        assertConvertRadix(new byte[] { (byte)0xFF }, 0x100, new byte[] { 0x02, 0x37 }, 100);
        assertConvertRadix(new byte[] { 0x12 }, 0x100, new byte[] { 0x01, 0x02 }, 0x10);
        assertConvertRadix(MANTISSA_123_256, 0x100, MANTISSA_123_100, 100);
        assertConvertRadix(MANTISSA_123_100, 100, MANTISSA_123_10, 10);
    }

    private void assertConvertRadix(byte[] a, int ar, byte[] b, int br)
    {
        AltibaseNumeric sSrcNum = new AltibaseNumeric(ar, a, a.length);
        AltibaseNumeric sDstNum = new AltibaseNumeric(br, sSrcNum);
        assertArrayEquals(b, sDstNum.toByteArray());

        sSrcNum = new AltibaseNumeric(br, b, b.length);
        sDstNum = new AltibaseNumeric(ar, sSrcNum);
        assertArrayEquals(a, sDstNum.toByteArray());
    }
}