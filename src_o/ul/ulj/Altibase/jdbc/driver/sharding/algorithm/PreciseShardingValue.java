/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.algorithm;

public class PreciseShardingValue<T extends Comparable<?>> implements ShardingValue
{
    private int mColumnIdx;
    private T mValue;

    public PreciseShardingValue(int aColumnIdx, T aValue)
    {
        this.mColumnIdx = aColumnIdx;
        this.mValue = aValue;
    }

    public int getColumnIdx()
    {
        return mColumnIdx;
    }

    public T getValue()
    {
        return mValue;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("PreciseShardingValue{");
        sSb.append("mColumnIdx=").append(mColumnIdx);
        sSb.append(", mValue=").append(mValue);
        sSb.append('}');
        return sSb.toString();
    }
}
