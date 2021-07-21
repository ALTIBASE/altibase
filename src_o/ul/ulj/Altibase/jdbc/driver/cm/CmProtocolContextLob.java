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

import Altibase.jdbc.driver.LobConst;

public class CmProtocolContextLob extends CmProtocolContext
{
    private long   mLocatorId;
    private long   mLobLength;
    private byte[] mBuf4Blob;
    private char[] mBuf4Clob;
    private int    mOffset;
    private int    mOffset4Target;
    private int    mMode;
    
    public CmProtocolContextLob(CmChannel aChannel, long aLocatorId, long aLobLength)
    {
        super(aChannel);
        mLocatorId = aLocatorId;
        mLobLength = aLobLength;
        mMode = 0x00;
    }
    
    public CmBlobGetResult getBlobGetResult()
    {
        return (CmBlobGetResult)getCmResult(CmBlobGetResult.MY_OP);
    }
    
    public CmClobGetResult getClobGetResult()
    {
        return (CmClobGetResult)getCmResult(CmClobGetResult.MY_OP);
    }
    
    public void setBlobData(byte[] aValue)
    {
        mBuf4Blob = aValue;
    }
    
    public byte[] getBlobData()
    {
        return mBuf4Blob;
    }

    public void setClobData(char[] aValue)
    {
        mBuf4Clob = aValue;
    }
    
    public char[] getClobData()
    {
        return mBuf4Clob;
    }
    
    public int getByteLengthPerChar()
    {
        return channel().getByteLengthPerChar();
    }
    
    long locatorId()
    {
        return mLocatorId;        
    }
    
    long lobLength()
    {
        return mLobLength;
    }
    
    public void setDstOffset(int aOffset)
    {
        mOffset = aOffset;
    }
    
    public int getDstOffset()
    {
        return mOffset;
    }
    
    public boolean isCopyMode()
    {
        return (mMode & LobConst.LOB_COPY_MODE_FOR_INPUT_STREAM) == LobConst.LOB_COPY_MODE_FOR_INPUT_STREAM ? true : false;
    }
    
    public void setCopyMode()
    {
        mMode = mMode | LobConst.LOB_COPY_MODE_FOR_INPUT_STREAM;
    }
    
    public void initMode()
    {
        mMode = 0x00;
    }
    
    public void releaseCopyMode()
    {
        mMode = mMode & ~LobConst.LOB_COPY_MODE_FOR_INPUT_STREAM;
    }

    public void setMode(byte aIsDirectAbled)
    {
        mMode = mMode | aIsDirectAbled;
    }
}
