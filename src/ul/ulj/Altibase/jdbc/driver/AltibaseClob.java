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
import java.io.Reader;
import java.io.Writer;
import java.sql.Clob;
import java.sql.SQLException;

import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.datatype.LobObjectFactory;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class AltibaseClob extends AltibaseLob implements Clob
{
    private static final int EXCEEDED_OFFSET_ERROR_CODE = 286052508;
    
    private BlobInputStream mAsciiStream;
    private ClobReader mReader;
    private byte[] mLobByteCache;
    private char[] mLobCharCache;

    AltibaseClob(long aLocatorId, long aLobLength, byte[] aLobByteCache, char[] aLobCharCache)
    {
        super(aLocatorId, aLobLength);
        mLobByteCache = aLobByteCache;
        mLobCharCache = aLobCharCache;
    }

    public InputStream getAsciiStream() throws SQLException
    {
        if (mAsciiStream == null)
        {
            mAsciiStream = (BlobInputStream)LobObjectFactory.createAsciiStream(mLocatorId, mLobLength, mLobByteCache);
            if (mChannel != null) // BUG-38008 채널이 오픈되어 있는지 확인한다.
            {
                mAsciiStream.open(mChannel);
            }
        }
        return mAsciiStream;
    }

    public Reader getCharacterStream() throws SQLException
    {
        if (mReader == null)
        {
            mReader = (ClobReader)LobObjectFactory.createCharacterStream(mLocatorId, mLobLength, mLobByteCache, mLobCharCache);
            if (mChannel != null)
            {
                mReader.open(mChannel);
            }
        }
        return mReader;
    }

    public String getSubString(long aStartPos, int aLength) throws SQLException
    {
        if( aLength <= 0)
        {
            return "";
        }

        aLength = Math.min((int)length(), aLength);
        mContext.setClobData(new char[aLength * 2]);
        mContext.setDstOffset(0);
        CmProtocol.getClobCharPos(mContext, aStartPos == 0 ? aStartPos : aStartPos - 1, aLength);
        
        if(mContext.getError() != null && mContext.getError().getErrorCode() == EXCEEDED_OFFSET_ERROR_CODE)
        {
            return "";
        }
        
        Error.processServerError(null, mContext.getError());
        return String.valueOf(mContext.getClobData(), 0, mContext.getDstOffset());
    }

    public long length() throws SQLException
    {
        CmProtocol.getCharLength(mContext);
        Error.processServerError(null, mContext.getError());
        return mContext.getClobGetResult().getCharLength();
    }

    public long position(Clob aPattern, long aStartPos) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("searching clob data");
    }

    public long position(String aPattern, long aStartPos) throws SQLException
    {
        throw Error.createSQLFeatureNotSupportedException("searching clob data");
    }

    public OutputStream setAsciiStream(long aStartPos) throws SQLException
    {
        CmProtocol.getBytePos(mContext, (int) (aStartPos == 0 ? aStartPos : aStartPos - 1));
        Error.processServerError(null, mContext.getError());
        long sByteStartOffset = mContext.getClobGetResult().getOffset();
        return new BlobOutputStream(mChannel, mLocatorId, mLobLength, sByteStartOffset);
    }

    public Writer setCharacterStream(long aStartPos) throws SQLException
    {
        CmProtocol.getBytePos(mContext, (int) (aStartPos == 0 ? aStartPos : aStartPos - 1));
        Error.processServerError(null, mContext.getError());
        long sByteStartOffset = mContext.getClobGetResult().getOffset();
        return new ClobWriter(mChannel, mLocatorId, mLobLength, sByteStartOffset);
    }

    public int setString(long aStartPos, String aSource, int aOffset, int aLength) throws SQLException
    {
        CmProtocol.getBytePos(mContext, (int) (aStartPos == 0 ? aStartPos : aStartPos - 1));
        Error.processServerError(null, mContext.getError());
        try
        {
            CmProtocol.putClob(mContext, aOffset, aSource.toCharArray());
        } 
        catch (IOException e)
        {
            Error.throwSQLException(ErrorDef.STREAM_ALREADY_CLOSED);
        }
        Error.processServerError(null, mContext.getError());
        return aLength;
    }

    public int setString(long aStartPos, String aSource) throws SQLException
    {
        return setString(aStartPos, aSource, 0, aSource.length());
    }
    
    public String toString()
    {
        if (mLobCharCache != null)
        {
            return String.valueOf(mLobCharCache);
        }
        else
        {
            try
            {
                return getSubString(1, (int) length());
            } 
            catch (SQLException e)
            {
                // BUGBUG need to log
                return null;
            }
        }
    }

    @Override
    public Reader getCharacterStream(long aPos, long aLength) throws SQLException
    {
        if (mReader == null)
        {
            mReader = (ClobReader)LobObjectFactory.createCharacterStream(mLocatorId, aLength, mLobByteCache, mLobCharCache);
            if (mChannel != null)
            {
                mReader.open(mChannel);
            }
        }
        // PROJ-2707 position으로 서버 offset을 맞춘다.
        mReader.setOffset4Server(aPos);

        return mReader;
    }
}
