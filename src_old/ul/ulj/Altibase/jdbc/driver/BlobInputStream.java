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
import java.sql.SQLException;

import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextLob;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class BlobInputStream extends InputStream implements ConnectionSharable
{
    private long                 mLocatorId;
    private final long           mLobLength;
    private CmProtocolContextLob mContext;
    private int                  mFetchRemains4Buf;
    private long                 mFetchRemains4Server;
    private long                 mCurrentFetchSize4Server;
    private long                 mOffset4Server;
    private boolean              mClosed;
    private byte[]               mBuf;
    private boolean              isCached = false;
    private int                  mPosition4StreamBuf;

    BlobInputStream(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        mLocatorId = aLocatorId;
        mLobLength = aLobLength;
        mBuf = aLobCache;
        if (mBuf != null)
        {
            isCached = true;
        }
        mClosed = true; // BUG-38008 open 전까진 closed로 처리
    }

    void open(CmChannel aChannel) throws SQLException
    {
        mContext = new CmProtocolContextLob(aChannel, mLocatorId, mLobLength);
        initialize();
    }

    private void initialize() throws SQLException
    {
        mClosed = false;

        if (isCached)
        {
            mFetchRemains4Server = 0;
            mFetchRemains4Buf = mBuf.length;
        } 
        else
        {
            mFetchRemains4Server = mLobLength;
            mFetchRemains4Buf = 0;
            mBuf = new byte[LobConst.LOB_BUFFER_SIZE];
        }
        
        mContext.setBlobData(mBuf);
        mPosition4StreamBuf = 0;
        mOffset4Server = 0;
    }

    public int available() throws IOException
    {
        return (int) ((mFetchRemains4Buf + mFetchRemains4Server) & 0x7FFFFFFF);
    }

    public boolean isClosed()
    {
        return mClosed;
    }
    
    public void close() throws IOException
    {
        if (mClosed) 
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }

        mClosed = true;
    }

    public void freeLocator() throws SQLException, IOException
    {
        if(!mClosed)
        {
            close();
        }
        CmProtocol.free(mContext);
    }
    
    public void mark(int aReadlimit)
    {
        // not implemented
    }

    public boolean markSupported()
    {
        return false;
    }

    public int read() throws IOException
    {
        if (available() <= 0)
        {
            return -1;
        }
        
        if(mFetchRemains4Buf > 0)
        {
            mFetchRemains4Buf--;
        }
        else
        {
            readIntoStreamBufFromServer(LobConst.LOB_BUFFER_SIZE);
            mFetchRemains4Buf--;
        }
        
        return mBuf[mPosition4StreamBuf++];
    }

    public int read(byte[] aDest, int aDestOffset, int aLengthWantToRead) throws IOException
    {
        if (aLengthWantToRead == 0)
        {
            return 0;
        }

        if(aDest.length <= aDestOffset)
        {
            return 0;
        }
        
        if (available() <= 0)
        {
            return -1;
        }

        int sReadLength = 0;

        if (aLengthWantToRead > available())
        {
            sReadLength = available();
        } 
        else
        {
            sReadLength = aLengthWantToRead;
        }

        int sActualWritableLength4Dest = aDest.length - aDestOffset;
        int sActualReadLength = 0;

        if (sReadLength > sActualWritableLength4Dest)
        {
            sActualReadLength = sActualWritableLength4Dest;
        } else
        {
            sActualReadLength = (int) sReadLength;
        }

        return readFrom(aDest, aDestOffset, sActualReadLength);
    }

    private int readFrom(byte[] aDest, int aDestOffset, int aLengthWantToRead) throws IOException
    {
        long sTotalReadSize = 0;
        int sReadSize = 0;
        
        while(aLengthWantToRead > 0)
        {
            mCurrentFetchSize4Server = Math.min(aLengthWantToRead, LobConst.LOB_BUFFER_SIZE);
            mContext.setDstOffset(aDestOffset);
            if(canGetDirectly())
            {
                getBlobDirectly(aDest);
                aDestOffset += mCurrentFetchSize4Server;
                sTotalReadSize += mCurrentFetchSize4Server;
                aLengthWantToRead -= mCurrentFetchSize4Server;
            }
            else
            {
                sReadSize = readFromBuf(aDest, aDestOffset, aLengthWantToRead);
                sTotalReadSize += sReadSize;
                aLengthWantToRead -= sReadSize;
                aDestOffset += sReadSize;
                
                if(aLengthWantToRead > 0)
                {
                    // BUG-40158 서버로부터 데이터를 받아와야 하기때문에 offset값을 초기화 해준다.
                    mContext.setDstOffset(0);
                    readIntoStreamBufFromServer(aLengthWantToRead);
                }
            }
        }
        
        return (int)(sTotalReadSize & 0x7FFFFFFF);
    }
    
    private long readIntoStreamBufFromServer(int aLengthWantToRead) throws IOException
    {
        if(isCached || mFetchRemains4Server == 0)
        {
            return 0;
        }
        
        mCurrentFetchSize4Server = Math.min(LobConst.LOB_BUFFER_SIZE, mFetchRemains4Server);
        fetchBlobDataFromServer();
        mFetchRemains4Buf += mCurrentFetchSize4Server;
        mPosition4StreamBuf = 0;
        return mCurrentFetchSize4Server;
    }

    private int readFromBuf(byte[] aDest, int aDestOffset, int aLengthWantToRead) throws IOException
    {
        if(mFetchRemains4Buf == 0)
        {
            return 0;
        }
        
        int sReadSize = Math.min(aLengthWantToRead, mFetchRemains4Buf);
        System.arraycopy(mBuf, mPosition4StreamBuf, aDest, aDestOffset, sReadSize);
        mFetchRemains4Buf -= sReadSize;
        mPosition4StreamBuf += sReadSize;
        return sReadSize;
    }
    
    private boolean canGetDirectly()
    {
        if(mFetchRemains4Buf == 0 && !isCopyMode())
        {
            if((mCurrentFetchSize4Server >= ((LobConst.LOB_BUFFER_SIZE / 3)*2)))
            {
                return true;
            }
            else
            {
                if(mFetchRemains4Server <= LobConst.LOB_BUFFER_SIZE)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
        else
        {
            return false;
        }
    }
    
    private void getBlobDirectly(byte[] aDest) throws IOException
    {
        mContext.setBlobData(aDest);
        fetchBlobDataFromServer();
        mContext.setBlobData(mBuf);
    }
    
    private void fetchBlobDataFromServer() throws IOException
    {
        try
        {
            CmProtocol.getBlob(mContext, mOffset4Server, mCurrentFetchSize4Server);
            Error.processServerError(null, mContext.getError());
            mFetchRemains4Server -= mCurrentFetchSize4Server;
            mOffset4Server += mCurrentFetchSize4Server;
        }
        catch (SQLException sEx)
        {
            Error.throwIOException(sEx);
        }
    }

    public int read(byte[] aBuf) throws IOException
    {
        return read(aBuf, 0, aBuf.length);
    }

    public void reset() throws IOException
    {
        // not implemented
    }

    public long skip(long aLen) throws IOException
    {
        if(mClosed)
        {
            return -1;
        }
        
        long sSkip = 0;
        
        if(aLen > available())
        {
            
        }
        else /* aLen <= TotalRemains */ 
        {
            aLen -= mFetchRemains4Buf;
            
            if(aLen > 0)
            {   sSkip += mFetchRemains4Buf;
                mFetchRemains4Buf = 0;
                aLen = Math.min(mFetchRemains4Server, aLen);
                mFetchRemains4Server -= aLen;
                sSkip += aLen;
                mOffset4Server += aLen;
            }
            else
            {
                mFetchRemains4Buf += aLen;
                sSkip -= aLen;
            }
        }
        
        return sSkip;
    }

    public boolean isCopyMode()
    {
        return mContext.isCopyMode();
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

    public void readyToCopy() throws IOException
    {
        readIntoStreamBufFromServer(LobConst.LOB_BUFFER_SIZE);
    }

    public boolean isSameConnectionWith(CmChannel aChannel)
    {
        return mContext.channel().equals(aChannel) ? true : false;
    }

    public void setOffset4Server(long aOffset)
    {
        this.mOffset4Server = aOffset;
    }
}
