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

package Altibase.jdbc.driver;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.sql.SQLException;

import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextLob;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class BlobOutputStream extends OutputStream implements ConnectionSharable
{
    private CmProtocolContextLob mContext;
    private byte[]               mBuf;
    private long                 mServerOffset;
    private boolean              mClosed;
    private int                  mPosition;
    
    BlobOutputStream(CmChannel aChannel, long aLocatorId, long aLobLength, long aStartPos)
    {
        mContext = new CmProtocolContextLob(aChannel, aLocatorId, aLobLength);
        mServerOffset = aStartPos;
        mClosed = false;
    }
    
    public void close() throws IOException
    {
        flush();

        if (mClosed)
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }

        mBuf = null;
        mClosed = true;
        mServerOffset = 0;
    }

    public void freeLocator() throws SQLException, IOException
    {
        if(!mClosed)
        {
            close();
        }
        CmProtocol.free(mContext);
    }
    
    public void flush() throws IOException
    {
        if(mBuf == null || mPosition == 0)
        {
            return;
        }
        
        writeToServer(mBuf, 0, mPosition);
        mPosition = 0;
    }

    private void initBuf()
    {
        if (mBuf == null)
        {
            mBuf = new byte[LobConst.LOB_BUFFER_SIZE];
        }
    }
    
    private boolean isWritableBuf(int aLength)
    {
        return LobConst.LOB_BUFFER_SIZE - mPosition >= aLength ? true : false;
    }
    
    private boolean isBufUsableDataLength(int aLength)
    {
        return aLength <= ((LobConst.LOB_BUFFER_SIZE / 3) * 2)  ? true : false; 
    }
    
    private void writeToBuf(byte[] aSource, int aSourceOffset, int aLength)
    {
        initBuf();
        
        System.arraycopy(aSource, aSourceOffset, mBuf, mPosition, aLength);
        mPosition += aLength;
    }

    private void writeToBuf(byte aByte)
    {
        initBuf();
        
        mBuf[mPosition++] = aByte;
    }
    
    private void writeToServer(byte[] aSource, int aOffset, int aLength) throws IOException
    {
        try
        {
            CmProtocol.putBlob(mContext, mServerOffset, aSource, aOffset, aLength);
            Error.processServerError(null, mContext.getError());
            mServerOffset += aLength;
        }
        catch (SQLException sEx)
        {
            Error.throwIOException(sEx);
        }
    }
    
    public void write(byte[] aSource, int aSourceOffset, int aLength) throws IOException
    {
        if(isWritableBuf(aLength))
        {
            writeToBuf(aSource, aSourceOffset, aLength);
        }
        else
        {
            flush();
            
            if(isBufUsableDataLength(aLength))
            {
                writeToBuf(aSource, aSourceOffset, aLength);
            }
            else
            {
                writeToServer(aSource, aSourceOffset, aLength);
            }
        }
    }

    public void write(byte[] aSource) throws IOException
    {
        write(aSource, 0, aSource.length);
    }

    public void write(int aByte) throws IOException
    {
        if(!isWritableBuf(1))
        {
            flush();
        }
        
        // This method doesn't need to check whether this data is writable into buffer or not.
        // That's because this method transfer only 1 byte.
        writeToBuf((byte)aByte);
    }
    
    public void write(InputStream aSource, long aLength) throws IOException
    {
        try
        {
            CmProtocol.putBlob(mContext, mServerOffset, aSource, aLength);
            Error.processServerError(null, mContext.getError());
            mServerOffset += aLength;
        }
        catch (SQLException sEx)
        {
            Error.throwIOException(sEx);
        }
    }
    
    public void setCopyMode()
    {
        mContext.setCopyMode();
    }
    
    public void releaseCopyMode()
    {
        mContext.releaseCopyMode();
    }
    
    public void initMode()
    {
        mContext.initMode();
    }

    public void setMode(byte aIsDirectAbled)
    {
        mContext.setMode(aIsDirectAbled);
    }

    public boolean isCopyMode()
    {
        return mContext.isCopyMode();
    }

    public boolean isSameConnectionWith(CmChannel aChannel)
    {
        return mContext.channel().equals(aChannel) ? true : false;
    }

    public void readyToCopy() throws IOException
    {
        Error.throwInternalError(ErrorDef.INVALID_METHOD_INVOCATION);
    }
}
