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
import java.sql.Blob;
import java.sql.SQLException;

import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.datatype.LobObjectFactory;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class AltibaseBlob extends AltibaseLob implements Blob
{
    private BlobInputStream mInputStream;
    private byte[] mLobCache;
    
    AltibaseBlob(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        super(aLocatorId, aLobLength);
        mLobCache = aLobCache;
    }

    public InputStream getBinaryStream() throws SQLException
    {
        if (mInputStream == null)
        {
            mInputStream = (BlobInputStream)LobObjectFactory.createBinaryStream(mLocatorId, mLobLength, mLobCache);
            if (mChannel != null) // BUG-48892 채널이 오픈되어 있는지 확인한다.
            {
                mInputStream.open(mChannel);
            }
        }
        return mInputStream;
    }

    public byte[] getBytes(long aOffset, int aLength) throws SQLException
    {
        mContext.setBlobData(new byte[aLength]);
        mContext.setDstOffset(0);
        CmProtocol.getBlob(mContext, (int)aOffset - 1, aLength);
        if (mContext.getError() != null)
        {
            Error.processServerError(null, mContext.getError());
        }
        return mContext.getBlobData();
    }

    public long length() throws SQLException
    {
        if (!mLobUpdated)
        {
            return mLobLength;
        }
        CmProtocol.getLobByteLength(mContext);
        if (mContext.getError() != null)
        {
            Error.processServerError(null, mContext.getError());
        }
        return mContext.getBlobGetResult().getLobLength();
    }

    public long position(Blob aPattern, long aStartPos) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("searching blob data");
    }

    public long position(byte[] aPattern, long aStartPos) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("searching blob data");
    }

    public OutputStream setBinaryStream(long aStartPos) throws SQLException
    {
        mLobUpdated = true;

        return new BlobOutputStream(mChannel, mLocatorId, mLobLength, aStartPos == 0 ? aStartPos : aStartPos - 1);
    }

    public int setBytes(long aStartPos, byte[] aData, int aOffset, int aLength) throws SQLException
    {
        try
        {
            CmProtocol.putBlob(mContext, (aStartPos == 0 ? aStartPos : aStartPos - 1), aData, aOffset, aLength);
        } 
        catch (IOException e)
        {
            Error.throwSQLException(ErrorDef.STREAM_ALREADY_CLOSED);
        }
        if (mContext.getError() != null)
        {
            Error.processServerError(null, mContext.getError());
        }
        mLobUpdated = true;
        return aLength;
    }

    public int setBytes(long aStartPos, byte[] aData) throws SQLException
    {
        try
        {
            CmProtocol.putBlob(mContext, (aStartPos == 0 ? aStartPos : aStartPos - 1), aData, 0, aData.length);
        } 
        catch (IOException e)
        {
            Error.throwSQLException(ErrorDef.STREAM_ALREADY_CLOSED);
        }
        if (mContext.getError() != null)
        {
            Error.processServerError(null, mContext.getError());
        }
        mLobUpdated = true;
        return aData.length;
    }

    @Override
    public InputStream getBinaryStream(long aPos, long aLobLength) throws SQLException
    {
        if (mInputStream == null)
        {
            mInputStream = (BlobInputStream)LobObjectFactory.createBinaryStream(mLocatorId, aLobLength, mLobCache);
            if (mChannel != null) // BUG-48892 채널이 오픈되어 있는지 확인한다.
            {
                mInputStream.open(mChannel);
            }
        }
        // PROJ-2707 position으로 서버 offset을 맞춘다.
        mInputStream.setOffset4Server(aPos);

        return mInputStream;
    }
}
