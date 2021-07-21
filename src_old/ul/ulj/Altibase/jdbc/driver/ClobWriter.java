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
import java.io.Writer;
import java.sql.SQLException;

import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextLob;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class ClobWriter extends Writer implements ConnectionSharable
{
    private CmProtocolContextLob mContext;
    private long mServerOffset;
    private boolean mClosed;
    
    ClobWriter(CmChannel aChannel, long aLocatorId, long aLobLength, long aStartPos)
    {
        mContext = new CmProtocolContextLob(aChannel, aLocatorId, aLobLength);
        mServerOffset = aStartPos;
        mClosed = false;
    }
    
    public void close() throws IOException
    {
        if (mClosed)
        {
            return;
        }

        mClosed = true;
    }

    public void freeLocator() throws SQLException, IOException
    {
        close();
        CmProtocol.free(mContext);
    }
    
    public void flush() throws IOException
    {
        if (mClosed)
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }
    }

    public void write(char[] aSource, int aOffset, int aLength) throws IOException
    {
        if (mClosed)
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }

        if ( (aOffset < 0) || (aOffset > aSource.length) || (aLength < 0)
          || (aOffset + aLength) > aSource.length || (aOffset + aLength) < 0)
        {
            throw new IndexOutOfBoundsException();
        }

        // 서버에 반영해야 하므로 중간 버퍼를 거치지 않고 바로 putClob을 수행한다.
        try
        {
            long sWrited = CmProtocol.putClob(mContext, mServerOffset, aSource, aOffset, aLength);
            Error.processServerError(null, mContext.getError());
            mServerOffset += sWrited;
        }
        catch (SQLException sEx)
        {
            Error.throwIOException(sEx);
        }
    }

    public void write(Reader aSource, int aSourceLength) throws IOException
    {
        if (mClosed)
        {
            Error.throwIOException(ErrorDef.STREAM_ALREADY_CLOSED);
        }

        try
        {
            long sWrited = CmProtocol.putClob(mContext, mServerOffset, aSource, aSourceLength);
            Error.processServerError(null, mContext.getError());
            mServerOffset += sWrited;
        }
        catch (SQLException sEx)
        {
            Error.throwIOException(sEx);
        }
    }


    // #region for ConnectionSharable

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

    // #endregion
}
