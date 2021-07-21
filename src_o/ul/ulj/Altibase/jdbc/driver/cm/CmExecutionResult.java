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

public class CmExecutionResult extends CmStatementIdResult
{
    static final byte MY_OP = CmOperation.DB_OP_EXECUTE_V3_RESULT;
    
    private int     mRowNumber;
    private int     mResultSetCount;
    private long    mUpdatedRowCount;
    private boolean mBatchMode = false;
    private long[]  mUpdateCountList;         // PROJ-2707 update count long 지원
    private int     mUpdateCountListSize;
    // PROJ-2733 분산 트랜잭션 정합성 
    private short   mSessionPropID;
    private String  mSessionPropValueStr = null;
    
    public CmExecutionResult()
    {
    }

    public int getRowNumber()
    {
        return mRowNumber;
    }

    public void setRowNumber(int aRowNumber)
    {
        mRowNumber = aRowNumber;
    }

    public int getResultSetCount()
    {
        return mResultSetCount;
    }
    
    public long getUpdatedRowCount()
    {
        return mUpdatedRowCount;
    }
    
    public long[] getUpdatedRowCounts()
    {
        if (mUpdateCountList == null)
        {
            return null;
        }

        long[] sResult = new long[mUpdateCountListSize];
        System.arraycopy(mUpdateCountList, 0, sResult, 0, mUpdateCountListSize);
        return sResult;
    }
    
    public void clearBatchUpdateCount()
    {
        mUpdateCountListSize = 0;
    }

    public boolean isBatchMode()
    {
        return mBatchMode;
    }
    
    public void setBatchMode(boolean aMode)
    {
        mBatchMode = aMode;
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }

    // BUG-46513 smn 오류일때 에러메세지로 부터 결과를 파싱하여 셋팅하기 때문에 public으로 변경
    public void setResultSetCount(int aResultSetCount)
    {
        mResultSetCount = aResultSetCount;
    }

    public void setUpdatedRowCount(long aUpdatedRowCount)
    {
        if (isBatchMode())
        {
            if (mUpdateCountList == null)
            {
                mUpdateCountList = new long[32];
            }
            else if (mUpdateCountListSize == mUpdateCountList.length)
            {
                long[] sNewList = new long[mUpdateCountList.length + 32];
                System.arraycopy(mUpdateCountList, 0, sNewList, 0, mUpdateCountList.length);
                mUpdateCountList = sNewList;
            }
            mUpdateCountList[mUpdateCountListSize++] = aUpdatedRowCount;
        }
        else
        {
            mUpdatedRowCount = aUpdatedRowCount;
        }
    }

    int getUpdatedRowCountArraySize()
    {
        return mUpdateCountListSize;
    }

    public short getSessionPropID()
    {
        return mSessionPropID;
    }

    public void setSessionPropID(short aSessionPropID)
    {
        mSessionPropID = aSessionPropID;
    }

    public String getSessionPropValueStr()
    {
        return mSessionPropValueStr;
    }

    public void setSessionPropValueStr(String aSessionPropValueStr)
    {
        mSessionPropValueStr = aSessionPropValueStr;
    }
}
