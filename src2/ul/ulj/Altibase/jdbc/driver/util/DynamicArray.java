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

import java.util.ArrayList;
import java.util.List;

public abstract class DynamicArray
{
    /*
     * chunk의 모양은 다음과 같다.
     * data[][]: 2차원 배열
     * data[0] = [][][][][]... : 64개
     * data[1] = [][][][][][][][][][][][][][][][]... : 256개 (4배씩 늘어난다) ...
     */
    private static final int     INIT_CHUNK_SIZE   = 64;
    private static final int     MAX_CHUNK_SIZE    = 65536;
    private static final int     GROW_FACTOR       = 4;
    private static int           DYNAMIC_ARRY_SIZE = 349504; // BUG-43263 기존에 DynamicArray에 들어갈 수 있었던 최대치
    private static List<Integer> CHUNK_SIZES       = makeChunkSizeArray();

    protected DynamicArrayCursor mStoreCursor;
    protected DynamicArrayCursor mLoadCursor;
    
    private int mLastAllocatedChunkIndex = -1;

    public DynamicArray()
    {
        allocChunkEntry(CHUNK_SIZES.size());
    }

    /**
     * BUG-43263 DynamicArray에 들어갈 수 있는 최대값을 이용해 CHUNK SIZE 리스트를 만든다.<br>
     * CHUNK SIZE의 초기값은 64이고 4배씩 증가하며 65536을 초과하지 않는다.
     * @return the list of chunk sizes.
     */
    private static List<Integer> makeChunkSizeArray()
    {
        List<Integer> sChunkSizeArray = new ArrayList<Integer>();
        int sChunkSize = INIT_CHUNK_SIZE;
        int sDynamicArrySize = DYNAMIC_ARRY_SIZE;

        while (sDynamicArrySize > 0)
        {
            sChunkSizeArray.add(sChunkSize);
            sDynamicArrySize -= sChunkSize;
            if (sChunkSize < MAX_CHUNK_SIZE)
            {
                sChunkSize *= GROW_FACTOR;
            }
        }

        return sChunkSizeArray;
    }

    public void setCursor(DynamicArrayCursor aStoreCursor, DynamicArrayCursor aLoadCursor)
    {
        mStoreCursor = aStoreCursor;
        mLoadCursor = aLoadCursor;
    }

    protected void checkChunk()
    {
        while (true)
        {
            if (mLastAllocatedChunkIndex < mStoreCursor.chunkIndex())
            {
                mLastAllocatedChunkIndex++;
                expand(mLastAllocatedChunkIndex, CHUNK_SIZES.get(mLastAllocatedChunkIndex));
            }
            else
            {
                break;
            }
        }
    }

    public static int getDynamicArrySize()
    {
        return DYNAMIC_ARRY_SIZE;
    }

    public static List<Integer> getChunkSizes()
    {
        return CHUNK_SIZES;
    }

    protected abstract void allocChunkEntry(int aChunkCount);
    
    protected abstract void expand(int aChunkIndex, int aChunkSize);
}
