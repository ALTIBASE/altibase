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

public class CmClobGetResult extends CmResult
{
    static final byte MY_OP = CmOperation.DB_OP_LOB_GET_BYTE_POS_CHAR_LEN_RESULT;
    
    private long mLocatorId;
    private long mOffset;
    private long mCharLengthToGet;
    private long mCharLength;
    private long mByteLength;

    public CmClobGetResult()
    {
    }

    public void init(long aLocatorId, long aOffset, long aCharLength)
    {
        mLocatorId = aLocatorId;
        mOffset = aOffset;
        mCharLengthToGet = aCharLength;
        mCharLength = 0;
        mByteLength = 0;
    }

    public long getLocatorId()
    {
        return mLocatorId;
    }
    
    public long getCharLength()
    {
        return mCharLength;
    }
    
    public void setCharLengthToGet(int aCharLength)
    {
        mCharLengthToGet = aCharLength;
    }
    
    public long getOffset()
    {
        return mOffset;
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }

    void setLocatorId(long aLocatorId)
    {
        mLocatorId = aLocatorId;
    }
    
    void setOffset(long aOffset)
    {
        mOffset = aOffset;
    }
    
    long getCharLengthToGet()
    {
        return mCharLengthToGet;
    }
    
    void setCharLength(long aCharLength)
    {
        mCharLength = aCharLength;
    }

    void addCharLength(long aCharLength)
    {
        mCharLength += aCharLength;
    }

    public long getByteLength()
    {
        return mByteLength;
    }

    public void setByteLength(long aByteLength)
    {
        mByteLength = aByteLength;
    }

    public void addByteLength(long aByteLength)
    {
        mByteLength += aByteLength;
    }
}
