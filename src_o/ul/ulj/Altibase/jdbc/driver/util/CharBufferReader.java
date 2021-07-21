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

import java.io.IOException;
import java.io.Reader;
import java.nio.CharBuffer;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class CharBufferReader extends Reader
{
    private CharBuffer mBuf;
    private int mByteLength;
    private boolean mClosed;
    
    public CharBufferReader(CharBuffer aBuf, int aByteLength)
    {
        mBuf = aBuf;
        mByteLength = aByteLength;
        mClosed = false;
    }
    
    public void reopen()
    {
        mClosed = false;
    }
    
    public int getByteLength()
    {
        return mByteLength;
    }
    
    public int available() throws IOException
    {
        if (mClosed)
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }

        return mBuf.remaining();
    }
    
    public void close() throws IOException
    {
        if (mClosed)
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }

        mClosed = true;        
    }

    public void mark(int aReadAheadLimit) throws IOException
    {
    }

    public boolean markSupported()
    {
        return false;
    }

    public int read() throws IOException
    {
        return mBuf.get();
    }

    public int read(char[] aBuf, int aOffset, int aLen) throws IOException
    {
        int sPos = mBuf.position();
        mBuf.get(aBuf, aOffset, aLen);
        return mBuf.position() - sPos;
    }

    public int read(char[] aBuf) throws IOException
    {
        int sPos = mBuf.position();
        mBuf.get(aBuf);
        return mBuf.position() - sPos;
    }

    public boolean ready() throws IOException
    {
        return true;
    }

    public void reset() throws IOException
    {
    }

    public long skip(long aSkipLen) throws IOException
    {
        int sBefore = mBuf.position();
        mBuf.position(sBefore + (int)aSkipLen);
        return mBuf.position() - sBefore;
    }
}

