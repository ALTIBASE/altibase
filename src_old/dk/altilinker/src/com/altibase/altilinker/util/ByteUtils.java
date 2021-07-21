/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.util;

import java.nio.ByteBuffer;
import java.lang.String;

public final class ByteUtils
{
    private static final char[] HEX_LITERALS_L   = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    private static final char[] HEX_LITERALS_U   = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    private ByteUtils()
    {
    }

    /**
     * Hex String으로 쓸 수 있는 문자인지 확인한다.
     *
     * @param c 확인할 문자
     * @return [0-9a-fA-F]이면 true, 아니면 false
     */
    public static boolean isHexCharacter(char c)
    {
        if (('0' <= c && c <= '9') ||
            ('a' <= c && c <= 'f') ||
            ('A' <= c && c <= 'F'))
        {
            return true;
        }
        return false;
    }

    /**
     * hex string을 byte array로 변환한다.
     * <p>
     * 만약 hex string이 짝수가 아니라면, 마지막 바이트의 하위 4bit는 0으로 채운다.
     * 
     * @param aHexString byte array로 변환할 hex string
     * @return 변환된 byte array. hex string이 null이면 null, 빈 문자열이면 길이가 0인 배열.
     */
    public static byte[] parseByteArray(String aHexString) throws Exception
    {
        return parseByteArray(aHexString, true);
    }

    /**
     * hex string을 byte array로 변환한다.
     * <p>
     * hex string의 길이가 2의 배수가 아닐때, 0으로 패딩할 수도 있고 예외를 낼 수도 있다.
     * 만약 2의 배수가 아닐 때, 패딩을 사용한다면 마지막 바이트의 하위 4bit는 0으로 채우고,
     * 패딩을 사용하지 않는다면 예외를 던진다.
     *
     * @param aHexString byte array로 변환할 hex string
     * @param aUsePadding hex string이 2의 배수가 아닐 때, 0으로 패딩할지 여부.
     * @return 변환된 byte array. hex string이 null이면 null, 빈 문자열이면 길이가 0인 배열.
     * @exception IllegalArgumentException hex string이 올바르지 않을 경우
     * @exception IllegalArgumentException 패딩을 사용하지 않을 때, hex string이 2의 배수가 아닌 경우
     */
    public static byte[] parseByteArray(String aHexString, boolean aUsePadding) throws Exception
    {
        if (aHexString == null)
        {
            return null;
        }

        int sBufSize = aHexString.length() / 2;
        if ((aHexString.length() % 2) == 1)
        {
            if (aUsePadding)
            {
                sBufSize++;
            }
            else
            {
                throw new Exception("Invalid hex string length");
            }
        }
        byte[] sBuf = new byte[sBufSize];
        for (int i = 0; i < aHexString.length(); i++)
        {
            char c = aHexString.charAt(i);
            if (!ByteUtils.isHexCharacter(c))
            {    
                String sExString = new String( "Invalid hex the " + String.valueOf(i) + 
                                               " element: "+ String.valueOf(c) ) ; 
                throw new Exception( sExString );

            }
            c |= 0x20; // to lowercase. hext char임을 확인했으므로 이렇게 해도 된다.
            int v = (c < 'a') ? (c - '0') : (10 + c - 'a');
            if ((i % 2) == 0)
            {
                v = v << 4;
            }
            sBuf[i / 2] |= v;
        }
        return sBuf;
    }

    /**
     * byte array를 hex string으로 변환한다.
     * 
     * @param aByteArray hex string으로 변환할 byte array
     * @return 변환된 hex string. byte array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString(byte[] aByteArray) throws Exception
    {
        return toHexString(aByteArray, 0);
    }

    /**
     * byte array를 hex string으로 변환한다.
     * 
     * @param aByteArray hex string으로 변환할 byte array
     * @param aStartIdx 첫 index (inclusive)
     * @param aEndIdx 끝 index (exclusive)
     * @return 변환된 hex string. byte array가 null이면 "null", 길이가 0이면 빈 문자열.
     * @exception IllegalArgumentException 인자가 올바르지 않을 경우
     */
    public static String toHexString( byte[] aByteArray, 
                                      int    aStartIdx,
                                      int    aEndIdx ) throws Exception
    {
        return toHexString(aByteArray, aStartIdx, aEndIdx, 0, null, false);
    }

    /**
     * byte array를 hex string으로 변환한다.
     * 
     * @param aByteArray hex string으로 변환할 byte array
     * @param aSpacingBase 공백을 삽입할 단위. 0이면 공백을 붙이지 않는다.
     * @return 변환된 hex string. byte array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString( byte[] aByteArray, 
                                      int    aSpacingBase ) throws Exception
    {
        return toHexString(aByteArray, aSpacingBase, " ");
    }

    /**
     * byte array를 hex string으로 변환한다.
     * 
     * @param aByteArray hex string으로 변환할 byte array
     * @param aAppendingBase aAppendingChar를 삽입할 단위. 0이면 붙이지 않는다.
     * @param aAppendingString aAppendingBase 마다 추가할 문자열
     * @return 변환된 hex string. byte array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString( byte[]    aByteArray, 
                                      int       aAppendingBase,
                                      String    aAppendingString ) throws Exception
    {
        return toHexString( aByteArray, 0, 0, aAppendingBase, aAppendingString, false);
    }

    /**
     * byte array를 hex string으로 변환한다.
     * 
     * @param aByteArray hex string으로 변환할 byte array
     * @param aStartIdx 첫 index (inclusive)
     * @param aEndIdx 끝 index (exclusive)
     * @param aAppendingBase aAppendingChar를 삽입할 단위. 0이면 붙이지 않는다.
     * @param aAppendingString aAppendingBase 마다 추가할 문자열
     * @param aToUpper Upper case로 변환할것인지 여부
     * @return 변환된 hex string. byte array가 null이면 "null", 길이가 0이면 빈 문자열.
     * @exception IllegalArgumentException 인자가 올바르지 않을 경우
     */
    public static String toHexString( byte[] aByteArray, 
                                      int    aStartIdx,
                                      int    aEndIdx, 
                                      int    aAppendingBase, 
                                      String aAppendingString, 
                                      boolean aToUpper ) throws Exception
    {
        String sExString = null;
        
        if (aByteArray == null)
        {
            return "null";
        }
        if (aByteArray.length == 0)
        {
            return "";
        }
        if (aEndIdx == 0)
        {
            aEndIdx = aByteArray.length;
        }
        if (aEndIdx == aStartIdx)
        {
            return "";
        }
        
        if (aStartIdx < 0)
        {
            sExString = new String( "Start index [" + Integer.toString(aStartIdx) + "] " +
                                    "should be between 0 and Integer.MAX_VALUE" );
            throw new Exception( sExString );
        }
        
        if (aEndIdx < aStartIdx || aByteArray.length < aEndIdx)
        {
           sExString = new String( "End index [" + Integer.toString( aEndIdx )+"] " + 
                                   "should be between " + Integer.toString( aStartIdx ) + 
                                   "and " + Integer.toString( aByteArray.length ) );

            throw new Exception( sExString );
        }

        int sBufSize = (aEndIdx - aStartIdx) * 2;
        
        if (aAppendingString == null)
        {
            aAppendingBase = 0;
        }
        else if (aAppendingBase > 0)
        {
            sBufSize += (aByteArray.length % aAppendingBase) * aAppendingString.length();
        }

        final char[] HEX_LITERALS = aToUpper ? HEX_LITERALS_U : HEX_LITERALS_L;
        StringBuffer sBuf = new StringBuffer(sBufSize);
        for (int i = aStartIdx; i < aEndIdx; i++)
        {
            if ((aAppendingBase > 0) && (i > 0) && (i % aAppendingBase == 0))
            {
                sBuf.append(aAppendingString);
            }
            sBuf.append(HEX_LITERALS[(aByteArray[i] & 0xF0) >>> 4]);
            sBuf.append(HEX_LITERALS[(aByteArray[i] & 0x0F)]);
        }
        return sBuf.toString();
    }

    /**
     * ByteBuffer를 hex string으로 변환한다.
     * 
     * @param aByteArray hex string으로 변환할 ByteBuffer
     * @return 변환된 hex string. byte array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString(ByteBuffer mByteBuffer) throws Exception
    {
        if (mByteBuffer == null)
        {
            return "null";
        }
        return toHexString(mByteBuffer, 0, mByteBuffer.remaining());
    }

    /**
     * ByteBuffer를 hex string으로 변환한다.
     * 
     * @param aByteArray hex string으로 변환할 ByteBuffer
     * @param aStartIdx 첫 index (inclusive)
     * @param aEndIdx 끝 index (exclusive)
     * @return 변환된 hex string. byte array가 null이면 "null", 길이가 0이면 빈 문자열.
     */
    public static String toHexString( ByteBuffer mByteBuffer, 
                                      int        aStartIdx,
                                      int        aEndIdx) throws Exception
    {
        if (mByteBuffer == null)
        {
            return "null";
        }
        if (mByteBuffer.remaining() == 0)
        {
            return "";
        }

        if (mByteBuffer.hasArray())
        {
            return toHexString(mByteBuffer.array(), mByteBuffer.position() + aStartIdx, mByteBuffer.position() + aEndIdx);
        }
        else
        {
            byte[] sBuf = new byte[mByteBuffer.remaining()];
            int sOrgPos = mByteBuffer.position();
            mByteBuffer.get(sBuf);
            mByteBuffer.position(sOrgPos);
            return toHexString(sBuf, aStartIdx, aEndIdx);
        }
    }
}