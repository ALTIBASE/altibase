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

import java.util.BitSet;

public class AltibaseBitSet extends BitSet
{
    private int mSize;

    /**
     * 지정한 크기를 갖는 BitSet을 만든다.
     *
     * @param aBitSetSize BitSet의 크기
     */
    public AltibaseBitSet( int aBitSetSize )
    {
        super(aBitSetSize);

        mSize = aBitSetSize;
    }

    public int size()
    {
        return mSize;
    }

    public int length()
    {
        return mSize;
    }

    public void set( int aBitIndex )
    {
        if (aBitIndex >= mSize)
        {
            throw new IndexOutOfBoundsException("Bit index < "+ mSize +": "+ aBitIndex);
        }

        super.set(aBitIndex);
    }

    public void clear( int aBitIndex )
    {
        if (aBitIndex >= mSize)
        {
            throw new IndexOutOfBoundsException("Bit index < "+ mSize +": "+ aBitIndex);
        }

        super.clear(aBitIndex);
    }
}
