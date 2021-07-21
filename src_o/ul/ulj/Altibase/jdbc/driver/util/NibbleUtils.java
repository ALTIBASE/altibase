/*
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

package Altibase.jdbc.driver.util;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

/**
 * nibble array를 위한 유틸리티 클래스.
 * <p>
 * nibble array는 nibble 값을 한 바이트로 하는 byte[]다.
 * 즉, nibble 데이타 'F0123'의 nibble array는 byte[]{0xF, 0x1, 0x2, 0x3}로 표현한다.
 * 이 값은 문자열로 'F0123'처럼 표현한다.
 */
public final class NibbleUtils
{
    private static final byte[] EMTRY_BYTE_ARRAY = new byte[0];

    private NibbleUtils()
    {
    }

    /**
     * nibble 값인지 확인한다.
     * 
     * @param aNibble 확인할 값
     * @return nibble 값이면 true, 아니면 false
     */
    public static boolean isNibble(int aNibble)
    {
        return (0 <= aNibble) && (aNibble <= 0xF);
    }

    /**
     * 유효한 nibble array인지 확인한다.
     *
     * @param aNibbleArray 확인한 byte array
     * @return byte array의 각 요소 값이 모두 nibble이면 true, 아니면 false
     */
    public static boolean isValid(byte[] aNibbleArray)
    {
        if (aNibbleArray == null)
        {
            return true;
        }

        for (int i = 0; i < aNibbleArray.length; i++)
        {
            if (!isNibble(aNibbleArray[i]))
            {
                return false;
            }
        }
        return true;
    }

    /**
     * hex string을 nibble array로 변환한다.
     * 
     * @param aHexString nibble array로 변환할 hex string
     * @return 변환된 nibble array. hex string이 null이면 null, 빈 문자열이면 길이가 0인 배열.
     * @exception IllegalArgumentException hex string이 올바르지 않을 경우
     */
    public static byte[] parseNibbleArray(String aHexString)
    {
        if (aHexString == null)
        {
            return null;
        }

        byte[] sBuf = new byte[aHexString.length()];
        for (int i=0; i<aHexString.length(); i++)
        {
            char c = Character.toLowerCase(aHexString.charAt(i));
            if (!ByteUtils.isHexCharacter(c))
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_HEX_STRING_ELEMENT,
                                                    String.valueOf(i),
                                                    String.valueOf(c));
            }
            int v = (c < 'a') ? (c - '0') : (10 + c - 'a');
            sBuf[i] = (byte)v;
        }
        return sBuf;
    }

    /**
     * nibble array를 byte array로 변환한다.
     *
     * @param aNibbleArray 변환할 nibble array
     * @return 변환한 byte array
     * @exception IllegalArgumentException 유효한 nibble array가 아닌 경우
     */
    public static byte[] toByteArray(byte[] aNibbleArray)
    {
        if (aNibbleArray == null)
        {
            return null;
        }
        if (aNibbleArray.length == 0)
        {
            return EMTRY_BYTE_ARRAY;
        }
        for (int i = 0; i < aNibbleArray.length; i++)
        {
            if (!isNibble(aNibbleArray[i]))
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_NIBBLE_ARRAY_ELEMENT,
                                                    String.valueOf(i),
                                                    String.valueOf(aNibbleArray[i]));
            }
        }

        int sBufLen = aNibbleArray.length / 2;
        if ((aNibbleArray.length % 2) == 1)
        {
            sBufLen++;
        }
        byte[] sBuf = new byte[sBufLen];
        for (int i = 0; i < aNibbleArray.length; i++)
        {
            int v = aNibbleArray[i];
            if ((i % 2) == 0)
            {
                v = v << 4;
            }
            sBuf[i / 2] |= v;
        }
        return sBuf;
    }

    /**
     * byte array를 nibble array로 변환한다.
     *
     * @param aByteArray nibble array로 변환할 byte array
     * @return 변환한 nibble array
     */
    public static byte[] fromByteArray(byte[] aByteArray)
    {
        return fromByteArray(aByteArray, Integer.MAX_VALUE);
    }

    /**
     * byte array를 nibble array로 변환한다.
     * <p>
     * max nibble length를 지정하면 byte array로부터 생성될 nibble array의 길이를 제한할 수 있다.
     * 단, max nibble length는 최대 길이를 제한하는데만 사용하므로,
     * 주어진 byte array로 만들 수 있는 nibble array의 길이가 max nibble array보다 작다면
     * 그만큼의 길이를 갖는 nibble array를 만든다.
     *
     * @param aByteArray nibble array로 변환할 byte array
     * @param aMaxNibbleLength nibble array의 최대 길이
     * @return 변환한 nibble array
     * @exception IllegalArgumentException max nibble length가 0보다 작을 경우
     */
    public static byte[] fromByteArray(byte[] aByteArray, int aMaxNibbleLength)
    {
        if (aMaxNibbleLength < 0)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "Max nibble length",
                                                "0 ~ Integer.MAX_VALUE",
                                                String.valueOf(aMaxNibbleLength));
        }
        if (aByteArray == null)
        {
            return null;
        }
        if ((aMaxNibbleLength == 0) || (aByteArray.length == 0))
        {
            return EMTRY_BYTE_ARRAY;
        }

        int sByteLen = Math.min(aMaxNibbleLength, aByteArray.length * 2);
        byte[] sNibbleArray = new byte[sByteLen];
        for (int i = 0; i < sByteLen; i++)
        {
            int v = ((i % 2) == 0)
                  ? ((aByteArray[i / 2] & 0xF0) >>> 4)
                  : ((aByteArray[i / 2] & 0x0F));
            sNibbleArray[i] = (byte)v;
        }
        return sNibbleArray;
    }

    /**
     * nibble array를 hex string으로 변환한다.
     * 
     * @param aNibbleArray hex string으로 변환할 nibble array
     * @return 변환된 hex string. nibble array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString(byte[] aNibbleArray)
    {
        return toHexString(aNibbleArray, 0);
    }

    /**
     * nibble array를 hex string으로 변환한다.
     * 
     * @param aNibbleArray hex string으로 변환할 nibble array
     * @param aSpacingBase 공백을 삽입할 단위. 0이면 공백을 붙이지 않는다.
     * @return 변환된 hex string. nibble array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString(byte[] aNibbleArray, int aSpacingBase)
    {
        return toHexString(aNibbleArray, aSpacingBase, " ");
    }

    /**
     * nibble array를 hex string으로 변환한다.
     * 
     * @param aNibbleArray hex string으로 변환할 nibble array
     * @param aAppendingBase aAppendingChar를 삽입할 단위. 0이면 붙이지 않는다.
     * @param aAppendingString aAppendingBase 마다 추가할 문자열
     * @return 변환된 hex string. nibble array가 null이면 "null", 길이가 0이면 빈 문자열.
     * @exception IllegalArgumentException array에 nibble 범위(0x0~0xF)를 넘어가는 값이 있을 경우
     */
    public static String toHexString(byte[] aNibbleArray, int aAppendingBase, String aAppendingString)
    {
        return toHexString(aNibbleArray, 0, (aNibbleArray == null) ? 0 : aNibbleArray.length, aAppendingBase, aAppendingString);
    }

    /**
     * nibble array를 hex string으로 변환한다.
     * 
     * @param aNibbleArray hex string으로 변환할 nibble array
     * @param aStartIdx 시작 index (inclusive)
     * @param aEndFence 끝 index (exclusive)
     * @return 변환된 hex string. nibble array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString(byte[] aNibbleArray, int aStartIdx, int aEndFence)
    {
        return toHexString(aNibbleArray, aStartIdx, aEndFence, 0, " ");
    }

    /**
     * nibble array를 hex string으로 변환한다.
     * 
     * @param aNibbleArray hex string으로 변환할 nibble array
     * @param aStartIdx 시작 index (inclusive)
     * @param aEndIndex 끝 index (exclusive)
     * @param aAppendingBase aAppendingChar를 삽입할 단위. 0이면 붙이지 않는다.
     * @param aAppendingString aAppendingBase 마다 추가할 문자열
     * @return 변환된 hex string. nibble array가 null이면 "null", 길이가 0이면 빈 문자열.
     * @exception IllegalArgumentException array에 nibble 범위(0x0~0xF)를 넘어가는 값이 있을 경우
     */
    public static String toHexString(byte[] aNibbleArray, int aStartIdx, int aEndIndex, int aAppendingBase, String aAppendingString)
    {
        if (aNibbleArray == null)
        {
            return "null";
        }
        if (aStartIdx < 0 || aStartIdx > aNibbleArray.length)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "Start index",
                                                "0 ~ " + aNibbleArray.length,
                                                String.valueOf(aStartIdx));
        }
        if (aEndIndex < aStartIdx || aEndIndex > aNibbleArray.length)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "End index",
                                                aStartIdx + " ~ " + aNibbleArray.length,
                                                String.valueOf(aEndIndex));
        }
        if (aEndIndex == aStartIdx)
        {
            return "";
        }

        int sBufSize = aEndIndex - aStartIdx;
        if (aAppendingString == null)
        {
            aAppendingBase = 0;
        }
        else if (aAppendingBase > 0)
        {
            sBufSize += (aNibbleArray.length % aAppendingBase) * aAppendingString.length();
        }

        StringBuffer sBuf = new StringBuffer(sBufSize);
        for (int i = aStartIdx; i < aEndIndex; i++)
        {
            if (!isNibble(aNibbleArray[i]))
            {
                Error.throwIllegalArgumentException(ErrorDef.INVALID_NIBBLE_ARRAY_ELEMENT,
                                                    String.valueOf(i),
                                                    String.valueOf(aNibbleArray[i]));
            }
            if ((aAppendingBase > 0) && (i > 0) && (i % aAppendingBase == 0))
            {
                sBuf.append(aAppendingString);
            }
            sBuf.append(Character.forDigit(aNibbleArray[i], 16));
        }
        return sBuf.toString();
    }
}
