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
import java.io.InputStream;
import java.nio.ByteBuffer;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class ByteBufferInputStream extends InputStream
{
    private ByteBuffer mBuf;
    private boolean mClosed;
    
    public ByteBufferInputStream(ByteBuffer aBuf)
    {
        mBuf = aBuf;
        mClosed = false;
    }

    public ByteBufferInputStream(ByteBuffer aBuf, int aStartIdx, int aLength)
    {
        byte[] sBytes = new byte[aLength];
        aBuf.position(aBuf.position() + aStartIdx);
        aBuf.get(sBytes);

        mBuf = ByteBuffer.wrap(sBytes);
        mClosed = false;
    }
    
    public void reopen()
    {
        mClosed = false;
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

    public synchronized void mark(int aReadLimit)
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

    public int read(byte[] aBuf, int aOffset, int aLen) throws IOException
    {
        int sPos = mBuf.position();
        mBuf.get(aBuf, aOffset, aLen);
        return mBuf.position() - sPos;
    }

    public int read(byte[] aBuf) throws IOException
    {
        int sPos = mBuf.position();
        mBuf.get(aBuf);
        return mBuf.position() - sPos;
    }

    public synchronized void reset() throws IOException
    {        
    }

    public long skip(long aSkipLen) throws IOException
    {
        int sBefore = mBuf.position();
        mBuf.position(sBefore + (int)aSkipLen);
        return mBuf.position() - sBefore;
    }
}
