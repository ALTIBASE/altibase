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

public class CmSetPropertyResult extends CmResult
{
    static final byte MY_OP = CmOperation.DB_OP_SET_PROPERTY_V3_RESULT;
    
    private int mPropertyID;
    private byte mGTxLevel;
    private long mSCN;

    public CmSetPropertyResult()
    {
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }

    public int getPropertyID() 
    {
        return mPropertyID;
    }

    void setPropertyID(int aPropertyID) 
    {
        mPropertyID = aPropertyID;
    }

    public byte getGTxLevel() 
    {
        return mGTxLevel;
    }

    void setGTxLevel(byte aGTxLevel) 
    {
        mGTxLevel = aGTxLevel;
    }

    public long getSCN() 
    {
        return mSCN;
    }

    void setSCN(long aSCN) 
    {
        mSCN = aSCN;
    }
}
