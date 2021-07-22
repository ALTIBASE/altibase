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

package Altibase.jdbc.driver.util;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class DynamicArrayCursor
{
    private int mChunkIndex; // 현재 chunk의 index: 0~9
    private int mDataIndex; // 현재 chunk에서 data의 index:

    public DynamicArrayCursor()
    {
        mChunkIndex = 0;
        mDataIndex = 0;        
    }

    public int getPosition()
    {
        int sResult = 0;
        for (int i = 0; i < mChunkIndex; i++)
        {
            sResult += DynamicArray.getChunkSizes().get(i);
        }
        sResult += mDataIndex;
        return sResult;
    }
    
    public void setPosition(int aNewPosition)
    {
        mChunkIndex = 0;
        mDataIndex = aNewPosition;
        while (mDataIndex >= DynamicArray.getChunkSizes().get(mChunkIndex))
        {
            mDataIndex -= DynamicArray.getChunkSizes().get(mChunkIndex);
            mChunkIndex++;
        }
    }
    
    public boolean geThan(DynamicArrayCursor aTarget)
    {
        if (mChunkIndex > aTarget.chunkIndex())
        {
            return true;
        }
        else if (mChunkIndex == aTarget.chunkIndex())
        {
            if (mDataIndex >= aTarget.dataIndex())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        return false;
    }
    
    public boolean equals(DynamicArrayCursor aTarget)
    {
        return (mChunkIndex == aTarget.chunkIndex()) && (mDataIndex == aTarget.dataIndex());
    }
    
    public boolean isBeforeFirst()
    {
        return (mChunkIndex == 0) && (mDataIndex == 0);
    }
    
    public boolean isFirst()
    {
        return (mChunkIndex == 0) && (mDataIndex == 1);
    }
    
    public void next()
    {
        mDataIndex++;
        
        // mDataIndex is the number of data
        // DynamicArray.CHUNK_SIZES.get(mChunkIndex) is the maximum number of current slot
        // If this expression is satisfied, the size of DynamicArray(slot) will be increased.
        if (mDataIndex == DynamicArray.getChunkSizes().get(mChunkIndex))
        {
            mChunkIndex++;
            if (mChunkIndex >= DynamicArray.getChunkSizes().size())
            {
                Error.throwInternalError(ErrorDef.DYNAMIC_ARRAY_OVERFLOW, String.valueOf(mChunkIndex));
            }
            mDataIndex = 0;
        }
    }
    
    public void previous()
    {
        mDataIndex--;
        if (mDataIndex < 0)
        {
            mChunkIndex--;
            if (mChunkIndex < 0)
            {
                Error.throwInternalError(ErrorDef.DYNAMIC_ARRAY_UNDERFLOW, String.valueOf(mChunkIndex));
            }
            mDataIndex = DynamicArray.getChunkSizes().get(mChunkIndex) - 1;
        }
    }
    
    int chunkIndex()
    {
        return mChunkIndex;
    }
    
    int dataIndex()
    {
        return mDataIndex;
    }
}
