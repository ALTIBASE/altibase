/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.algorithm;

import Altibase.jdbc.driver.datatype.*;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.sharding.core.ShardKeyDataType;
import Altibase.jdbc.driver.util.CharsetUtils;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.sql.SQLException;

public class HashGenerator
{
    private char[]  mHashPermut;
    private byte[]  mHashInitialValue;

    private static HashGenerator mInstance = new HashGenerator();

    private HashGenerator()
    {
        mHashPermut = new char[] {
                1, 87, 49, 12,176,178,102,166,121,193,  6, 84,249,230, 44,163,
                14,197,213,181,161, 85,218, 80, 64,239, 24,226,236,142, 38,200,
                110,177,104,103,141,253,255, 50, 77,101, 81, 18, 45, 96, 31,222,
                25,107,190, 70, 86,237,240, 34, 72,242, 20,214,244,227,149,235,
                97,234, 57, 22, 60,250, 82,175,208,  5,127,199,111, 62,135,248,
                174,169,211, 58, 66,154,106,195,245,171, 17,187,182,179,  0,243,
                132, 56,148, 75,128,133,158,100,130,126, 91, 13,153,246,216,219,
                119, 68,223, 78, 83, 88,201, 99,122, 11, 92, 32,136,114, 52, 10,
                138, 30, 48,183,156, 35, 61, 26,143, 74,251, 94,129,162, 63,152,
                170,  7,115,167,241,206,  3,150, 55, 59,151,220, 90, 53, 23,131,
                125,173, 15,238, 79, 95, 89, 16,105,137,225,224,217,160, 37,123,
                118, 73,  2,157, 46,116,  9,145,134,228,207,212,202,215, 69,229,
                27,188, 67,124,168,252, 42,  4, 29,108, 21,247, 19,205, 39,203,
                233, 40,186,147,198,192,155, 33,164,191, 98,204,165,180,117, 76,
                140, 36,210,172, 41, 54,159,  8,185,232,113,196,231, 47,146,120,
                51, 65, 28,144,254,221, 93,189,194,139,112, 43, 71,109,184,209
        };
        mHashInitialValue = new byte[] { 0x01, 0x02, 0x03, 0x04};
    }

    public static HashGenerator getInstance()
    {
        return mInstance;
    }

    private long doHashBigEndian(byte[] aBytes)
    {
        byte[] sHash = new byte[4];
        System.arraycopy(mHashInitialValue, 0, sHash, 0, 4);

        for (byte aByte : aBytes)
        {
            sHash[0] = (byte)mHashPermut[unsigned((byte)(sHash[0] ^ aByte))];
            sHash[1] = (byte)mHashPermut[unsigned((byte)(sHash[1] ^ aByte))];
            sHash[2] = (byte)mHashPermut[unsigned((byte)(sHash[2] ^ aByte))];
            sHash[3] = (byte)mHashPermut[unsigned((byte)(sHash[3] ^ aByte))];
        }

        return getUnsignedInt(shiftLeft(sHash));
    }

    private long doHashWithoutSpace(byte[] aBytes)
    {
        byte[] sHash = new byte[4];
        System.arraycopy(mHashInitialValue, 0, sHash, 0, 4);

        for (byte aByte : aBytes)
        {
            if (aByte != ' ' && aByte != 0x00)
            {
                sHash[0] = (byte)mHashPermut[unsigned((byte)(sHash[0] ^ aByte))];
                sHash[1] = (byte)mHashPermut[unsigned((byte)(sHash[1] ^ aByte))];
                sHash[2] = (byte)mHashPermut[unsigned((byte)(sHash[2] ^ aByte))];
                sHash[3] = (byte)mHashPermut[unsigned((byte)(sHash[3] ^ aByte))];
            }
        }

        return getUnsignedInt(shiftLeft(sHash));
    }


    private short unsigned(byte aValue)
    {
        return (short)(aValue & 0xFF);
    }

    private long getUnsignedInt(int aValue)
    {
        return aValue & 0x00000000ffffffffL;
    }

    public long doHash(Column aParameter, ShardKeyDataType aShardKeyDataType,
                       String aServerCharSet) throws SQLException
    {
        ByteBuffer sBuffer;
        long sResult = 0;
        switch (aShardKeyDataType)
        {
            case SMALLINT:
                sBuffer = ByteBuffer.allocate(2);
                sBuffer.putShort((aParameter.isNull()) ? SmallIntColumn.NULL_VALUE : aParameter.getShort());
                sResult = doHashBigEndian(sBuffer.array());
                break;
            case INTEGER:
                sBuffer = ByteBuffer.allocate(4);
                sBuffer.putInt((aParameter.isNull()) ? IntegerColumn.NULL_VALUE : aParameter.getInt());
                sResult = doHashBigEndian(sBuffer.array());
                break;
            case BIGINT:
                sBuffer = ByteBuffer.allocate(8);
                sBuffer.putLong((aParameter.isNull()) ? BigIntColumn.NULL_VALUE : aParameter.getLong());
                sResult = doHashBigEndian(sBuffer.array());
                break;
            case CHAR:
            case VARCHAR:
                String sStr = (aParameter.getString() == null) ? "" : aParameter.getString();
                try
                {
                    // BUG-47228 서버캐릭터셋으로 직접 encoding하지 않고 Charset으로 변환한 다음 시도한다.
                    Charset sCharset = CharsetUtils.getCharset(aServerCharSet);
                    sResult = doHashWithoutSpace(sStr.getBytes(sCharset.name()));
                }
                catch (Exception aException)
                {
                    /* BUG-47228 CharsetUtils.getCharset에서 RuntimeException이 올라올 수 있기때문에
                       Exception으로 catch하여 SHARD_CHARSET_ENCODE_ERROR 예외를 던진다. */
                    Error.throwSQLException(ErrorDef.SHARD_CHARSET_ENCODE_ERROR, sStr, aServerCharSet);
                }
                break;
        }

        return sResult;
    }

    private int shiftLeft(byte[] sHash)
    {
        return unsigned(sHash[0]) << 24 |
               unsigned(sHash[1]) << 16 |
               unsigned(sHash[2]) << 8  |
               unsigned(sHash[3]);
    }
}
