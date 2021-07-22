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

package Altibase.jdbc.driver.cm;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CoderResult;
import java.nio.charset.CodingErrorAction;
import java.sql.SQLException;
import java.util.ArrayList;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.CharsetUtils;

import static Altibase.jdbc.driver.cm.CmChannel.CHAR_VARCHAR_COLUMN_SIZE;

// PROJ-2368
/**
 * @author pss4you
 * 기존 CmChannel 에 정의 되어있던 함수 중, ListBufferHandle 과 공유해서 사용할 함수 들을 뽑아낸 상위 클래스
 */
public abstract class CmBufferWriter
{
    private static final int STATE_NORMAL   = 1;
    private static final int STATE_QUOTED   = 2;
    private static final int STATE_NSTART   = 3;
    private static final int STATE_NLITERAL = 4;

    protected CharsetEncoder mDBEncoder       = CharsetUtils.newAsciiEncoder();
    protected CharsetEncoder mNCharEncoder    = CharsetUtils.newAsciiEncoder();
    protected CharsetEncoder mNLiteralEncoder = CharsetUtils.newUTF16Encoder();
    protected ByteBuffer     mBuffer;
    protected ByteBuffer     mCharVarcharColumnBuffer;
    private ByteBuffer       mEncodingBuf;
    // BUG-46465 String encoding시 재사용하기 위한 CharBuffer 선언
    private CharBuffer       mTempCharWrapper = CharBuffer.allocate(CHAR_VARCHAR_COLUMN_SIZE);
    private char[]           mTempChars       = mTempCharWrapper.array();
    
    /**
     * Buffer 에 쓸 데이터 공간이 충분한지 여부를 확인하여, 충분하지 않으면 필요한 공간을 만드는 함수 
     * CmChannel 과 ListBufferHandle 에서 각자에 맞게 구현한다.
     * 
     * @param aNeedToWrite 데이타를 쓰기 위해 필요한 bytes 숫자
     * @throws SQLException 데이타 전송에 실패했을 경우
     */
    public abstract void checkWritable(int aNeedToWrite) throws SQLException;

    /**
     * ByteBuffer 객체를 Buffer 에 삽입하는 함수 
     * CmChannel 과 ListBufferHandle 에서 각자에 맞게 구현한다.
     * 
     * @param aValue Buffer 에 저장할 ByteBuffer 객체
     * @throws SQLException 데이타 전송에 실패했을 경우
     */
    public abstract void writeBytes(ByteBuffer aValue) throws SQLException;
    
    public int prepareToWriteString(String aValue, int aConvType)
    {
        ensureAllocEncodingBuffer(getMaxEncodingBufferSize(aConvType, aValue.length()));
        if (aConvType == CmOperation.WRITE_STRING_MODE_DB)
        {
            CoderResult sResult = encodeWithCharsetEncoder(aValue, mEncodingBuf, mDBEncoder);
            throwErrorForBufferOverFlow(sResult);
        }
        else if (aConvType == CmOperation.WRITE_STRING_MODE_NCHAR)
        {
            CoderResult sResult = encodeWithCharsetEncoder(aValue, mEncodingBuf, mNCharEncoder);
            throwErrorForBufferOverFlow(sResult);
        }
        else if (aConvType == CmOperation.WRITE_STRING_MODE_NLITERAL)
        {
            // sql 구문이 "insert into t1 values (N'some string', 123)"과 같을 때,
            // sSubStrings는 다음처럼 구성된다.
            // sSubStrings[0] = "insert into t1 values (N'"
            // sSubStrings[1] = "some string"
            // sSubStrings[2] = "', 123)"
            // 첫번째 sSubStrings는 반드시 DB_CHARSET으로 구성되어 있고,
            // 그다음부터 NCHARSET과 DB_CHARSET이 번갈아 온다.
            CoderResult sResult;
            String[] sSubStrings = splitNLiteralString(aValue);
            for (int i = 0; i < sSubStrings.length; i++)
            {
                if ((i % 2) == 0) // sSubStrings[]가 짝수번째 인 것들은 DB_CHARSET으로 encoding한다.
                {
                    sResult = encodeWithCharsetEncoder(sSubStrings[i], mEncodingBuf, mDBEncoder);
                }
                else
                {
                    sResult = encodeWithCharsetEncoder(sSubStrings[i], mEncodingBuf, mNLiteralEncoder);
                }
                throwErrorForBufferOverFlow(sResult);
            }
        }

        mEncodingBuf.flip();
        return mEncodingBuf.remaining();
    }

    public void writePreparedString() throws SQLException
    {
        writeBytes(mEncodingBuf);
    }
    
    private int getMaxEncodingBufferSize(int aConvType, int aStringLength)
    {
        int sMaxBytesPerChar = 0;
        switch (aConvType)
        {
            case CmOperation.WRITE_STRING_MODE_DB:
                sMaxBytesPerChar = (int)mDBEncoder.maxBytesPerChar();
                break;
            case CmOperation.WRITE_STRING_MODE_NCHAR:
                sMaxBytesPerChar = (int)mNCharEncoder.maxBytesPerChar();
                break;
            case CmOperation.WRITE_STRING_MODE_NLITERAL:
                sMaxBytesPerChar = (int)Math.max(mDBEncoder.maxBytesPerChar(), mNCharEncoder.maxBytesPerChar());
                break;
            default:
                Error.throwInternalError(ErrorDef.INVALID_TYPE,
                                         "WRITE_STRING_MODE_DB | WRITE_STRING_MODE_NCHAR | WRITE_STRING_MODE_NLITERAL",
                                         String.valueOf(aConvType));
                break;
        }
        return aStringLength * sMaxBytesPerChar;
    }

    private void ensureAllocEncodingBuffer(int sMaxBufSize)
    {
        if (mEncodingBuf == null || mEncodingBuf.capacity() < sMaxBufSize)
        {
            mEncodingBuf = ByteBuffer.allocate(sMaxBufSize);
        }
        else
        {
            mEncodingBuf.clear();
        }
    }

    private static String[] splitNLiteralString(String aString)
    {
        int sState = STATE_NORMAL;
        int sStartIndex = 0;
        ArrayList sResult = new ArrayList();
        for (int i = 0; i < aString.length(); i++)
        {
            char sChar = aString.charAt(i);
            switch (sState)
            {
                case STATE_NORMAL:
                    if (sChar == '\'')
                    {
                        sState = STATE_QUOTED;
                    }
                    else if (sChar == 'N' || sChar == 'n')
                    {
                        sState = STATE_NSTART;
                    }
                    break;
                case STATE_QUOTED:
                    if (sChar == '\'')
                    {
                        sState = STATE_NORMAL;
                    }
                    break;
                case STATE_NSTART:
                    if (sChar == '\'')
                    {
                        sResult.add(aString.substring(sStartIndex, i + 1));
                        sStartIndex = i + 1;
                        sState = STATE_NLITERAL;
                    }
                    else
                    {
                        sState = STATE_NORMAL;
                    }
                    break;
                case STATE_NLITERAL:
                    if (sChar == '\'')
                    {
                        sResult.add(aString.substring(sStartIndex, i));
                        sStartIndex = i;
                        sState = STATE_NORMAL;
                    }
                    break;
                default:
                    break;
            }
        }
        sResult.add(aString.substring(sStartIndex, aString.length()));
        return (String[])sResult.toArray(new String[0]);
    }

    public CoderResult encodeString(String aValue, ByteBuffer aBuf)
    {
        return encodeWithCharsetEncoder(aValue, aBuf, mDBEncoder);
    }

    /**
     * String 객체를 인코딩하여 aBuf 객체에 저장한다. 이때 String.getchars를 호출하여 내부 CharBuffer를 재사용한다.
     * @param aValue 인코딩할 String 객체
     * @param aBuf 인코딩 결과가 저장될 ByteBuffer 객체
     * @param aEncoder 인코딩에 사용할 CharsetEncoder 객체
     * @return encode 결과
     */
    private CoderResult encodeWithCharsetEncoder(String aValue, ByteBuffer aBuf, CharsetEncoder aEncoder)
    {
        CoderResult sResult = null;
        int sReadOffset = 0;
        boolean sDone = false;
        while (!sDone)
        {
            int sReadLength = aValue.length() - sReadOffset;
            if (sReadLength > mTempChars.length)
            {
                sReadLength = mTempChars.length;
            }
            // BUG-46465 chunk를 내부의 temporary char버퍼에 복사한다.
            aValue.getChars(sReadOffset, sReadOffset + sReadLength, mTempChars, 0);
            mTempCharWrapper.clear();
            mTempCharWrapper.limit(sReadLength);
            sReadOffset += sReadLength;

            sDone = (sReadOffset == aValue.length());
            sResult = aEncoder.encode(mTempCharWrapper, aBuf, sDone);
        }

        // BUG-47846 인코딩이 끝난 다음에는 CharsetEncoder 객체를 reset 해서 상태를 초기화 한다.
        aEncoder.reset();

        return sResult;
    }

    public void writeByte(byte aValue) throws SQLException
    {
        checkWritable(1);
        mBuffer.put(aValue);
    }

    public void writeShort(short aValue) throws SQLException
    {
        checkWritable(2);
        mBuffer.putShort(aValue);
    }

    public void writeInt(int aValue) throws SQLException
    {
        checkWritable(4);
        mBuffer.putInt(aValue);
    }

    public void writeUnsignedInt(long aValue) throws SQLException
    {
        checkWritable(4);
        mBuffer.putInt((int)(aValue & 0xFFFFFFFF));
    }

    public void writeLong(long aValue) throws SQLException
    {
        checkWritable(8);
        mBuffer.putLong(aValue);
    }
    
    public void writeFloat(float aValue) throws SQLException
    {
        checkWritable(4);
        mBuffer.putFloat(aValue);
    }

    public void writeDouble(double aValue) throws SQLException
    {
        checkWritable(8);
        mBuffer.putDouble(aValue);
    }

    public void writeBytes(byte[] aValue) throws SQLException
    {
        checkWritable(aValue.length);
        mBuffer.put(aValue);
    }

    
    public void setCharset(Charset aCharset, Charset aNCharset) 
    {
        mDBEncoder = aCharset.newEncoder();
        mNCharEncoder = aNCharset.newEncoder();

        mDBEncoder.onMalformedInput(CodingErrorAction.REPLACE);
        mDBEncoder.onUnmappableCharacter(CodingErrorAction.REPLACE);
        mNCharEncoder.onMalformedInput(CodingErrorAction.REPLACE);
        mNCharEncoder.onUnmappableCharacter(CodingErrorAction.REPLACE);
        // BUG-45156 NLiteralEncoder에 대한 에러처리 방식 추가
        mNLiteralEncoder.onMalformedInput(CodingErrorAction.REPLACE);
        mNLiteralEncoder.onUnmappableCharacter(CodingErrorAction.REPLACE);
    }

    private void throwErrorForBufferOverFlow(CoderResult sResult)
    {
        if (sResult == CoderResult.OVERFLOW)
        {
            Error.throwInternalError(ErrorDef.INTERNAL_BUFFER_OVERFLOW);
        }
    }
}
