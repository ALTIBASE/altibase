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

public class CmBlobGetResult extends CmResult
{
    static final byte MY_OP = CmOperation.DB_OP_LOB_GET_RESULT;
    
    private long mLocatorId;
    private long mOffset;
    private long mLobLength;

    public CmBlobGetResult()
    {
    }
    
    public long getLocatorId()
    {
        return mLocatorId;
    }
    
    public long getLobLength()
    {
        return mLobLength;
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }

    void setLocatorId(long aLocatorId)
    {
        mLocatorId = aLocatorId;
    }
    
    void setOffset(int aOffset)
    {
        mOffset = aOffset;
    }
    
    void setLobLength(long aLobLength)
    {
        mLobLength = aLobLength;
    }
    
    long getOffset()
    {
        return mOffset;
    }
}
