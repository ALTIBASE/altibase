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
import java.io.Reader;
import java.sql.SQLException;

import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.cm.CmClobGetResult;
import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextLob;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class ClobReader extends Reader implements ConnectionSharable
{
    private final long           mLocatorId;
    private final long           mServByteLength;
    private long                 mOffset4Server;
    private boolean              mIsClosed;
    private CmProtocolContextLob mContext;
    private final byte[]         mByteCache;
    private final char[]         mCharCache;
    private char[]               mCharBuf;
    private int                  mCharBufPos;
    private int                  mCharBufRemain;
    private int                  mMaxCharLengthPerPacket;
    
    ClobReader(long aLocatorId, long aLobByteLength, byte[] aLobByteCache, char[] aLobCharCache)
    {
        mLocatorId = aLocatorId;
        mServByteLength = aLobByteLength;
        mByteCache = aLobByteCache;
        mCharCache = aLobCharCache;
        mIsClosed = true; // open 전까진 closed
    }

    void open(CmChannel aChannel) throws SQLException
    {
        // 불필요한 패킷 분할을 줄이기 위해 1패킷에 담을 수 있는 최대 크기만큼씩만 얻는다.
        mMaxCharLengthPerPacket = LobConst.LOB_BUFFER_SIZE / aChannel.getByteLengthPerChar();
        mCharBuf = new char[mMaxCharLengthPerPacket];
        if (mCharCache != null)
        {
            mOffset4Server = mByteCache.length;
            mCharBufRemain = mCharCache.length;
            System.arraycopy(mCharCache, 0, mCharBuf, 0, mCharCache.length);
        }
        else
        {
            mOffset4Server = 0;
            mCharBufRemain = 0;
        }
        mCharBufPos = 0;

        mContext = new CmProtocolContextLob(aChannel, mLocatorId, mServByteLength);
        mContext.setClobData(mCharBuf);

        mIsClosed = false;
    }

    public boolean isClosed()
    {
        return mIsClosed;
    }

    public void close() throws IOException
    {
        if (isClosed())
        {
            return;
        }

        mIsClosed = true;
    }
    
    public void freeLocator() throws SQLException, IOException
    {
        close();
        CmProtocol.free(mContext);
    }

    private boolean isServerDataRemain()
    {
        return mOffset4Server < mServByteLength;
    }

    private boolean isReadDataRemain()
    {
        return (mCharBufRemain > 0) || isServerDataRemain();
    }

    private long fillCharBufFromServer() throws IOException
    {
        return fillCharBufFromServer(mCharBuf.length);
    }

    private long fillCharBufFromServer(int aLength) throws IOException
    {
        if (mCharBufRemain == mCharBuf.length)
        {
            return 0;
        }
        if (mCharBufRemain == 0)
        {
            mCharBufPos = 0;
        }
        mContext.setClobData(mCharBuf);
        mContext.setDstOffset(mCharBufPos);
        long sFetchLength = Math.min(aLength, mCharBuf.length - mCharBufPos);
        sFetchLength = fetchFromServer(sFetchLength);
        mCharBufRemain += sFetchLength;
        return sFetchLength;
    }

    /**
     * 서버에서 지정한 길이만큼의 문자열을 얻어온다.
     * 
     * @param aFetchLength 서버로부터 얻어올 문자열 길이
     * @return 실제로 얻은 문자열 길이
     * @throws IOException 서버로부터 문자열을 얻는데 실패했을 경우
     */
    private long fetchFromServer(long aFetchLength) throws IOException
    {
        long sFetchedCharLen = 0;
        try
        {
            CmProtocol.getClobBytePos(mContext, mOffset4Server, aFetchLength);
            Error.processServerError(null, mContext.getError());
            CmClobGetResult sResult = mContext.getClobGetResult();
            sFetchedCharLen = sResult.getCharLength();
            mOffset4Server = sResult.getOffset() + sResult.getByteLength();
        } 
        catch (SQLException sEx)
        {
            sEx.printStackTrace();
            Error.throwIOException(sEx);
        }
        return sFetchedCharLen;
    }

    public int read(char[] aDest, int aOffset, int aLength) throws IOException
    {
        if (isClosed())
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }

        if ( (aOffset < 0) || (aOffset >= aDest.length) || (aLength < 0) ||
             ((aOffset + aLength) > aDest.length) || ((aOffset + aLength) < 0) )
        {
            throw new IndexOutOfBoundsException();
        }
        if (aLength == 0)
        {
            return 0;
        }
        if (!isReadDataRemain())
        {
            return -1;
        }

        int sNeedToRead = aLength;
        int sWriteOffset = aOffset;
        while (sNeedToRead > 0 && isReadDataRemain())
        {
            long sFetchLength = Math.min(sNeedToRead, mMaxCharLengthPerPacket);
            if (mCharBufRemain == 0 && !isCopyMode())
            {
                sFetchLength = Math.min(sFetchLength, aDest.length - sWriteOffset);
                mContext.setClobData(aDest);
                mContext.setDstOffset(sWriteOffset);
                sFetchLength = fetchFromServer(sFetchLength);
                mContext.setClobData(mCharBuf);
                if (sFetchLength == 0) 
                {
                    Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION);
                }
                sWriteOffset += sFetchLength;
                sNeedToRead -= sFetchLength;
            }
            else
            {
                if (mCharBufRemain == 0)
                {
                    fillCharBufFromServer(sNeedToRead);
                }

                int sCopySize = Math.min(sNeedToRead, mCharBufRemain);
                System.arraycopy(mCharBuf, mCharBufPos, aDest, sWriteOffset, sCopySize);
                mCharBufRemain -= sCopySize;
                mCharBufPos += sCopySize;
                sNeedToRead -= sCopySize;
                sWriteOffset += sCopySize;
            }
        }
        return sWriteOffset - aOffset;
    }



    // #region for ConnectionSharable

    public boolean isSameConnectionWith(CmChannel aChannel)
    {
        return mContext.channel().equals(aChannel) ? true : false;
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
        if (isServerDataRemain())
        {
            fillCharBufFromServer();
        }
    }
    
    // #endregion

    public void setOffset4Server(long aOffset4Server)
    {
        this.mOffset4Server = aOffset4Server;
    }
}
