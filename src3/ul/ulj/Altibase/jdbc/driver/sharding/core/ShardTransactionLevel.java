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

package Altibase.jdbc.driver.sharding.core;

import java.util.HashMap;
import java.util.Map;

/**
 * 샤드 트랜잭션을 나타내는 Enum.
 */
public enum ShardTransactionLevel
{
    ONE_NODE((byte)0),
    MULTI_NODE((byte)1),
    GLOBAL((byte)2);

    byte mValue;

    private static final Map<Byte, ShardTransactionLevel> mMap = new HashMap<Byte, ShardTransactionLevel>();

    static
    {
        for (ShardTransactionLevel sItem : ShardTransactionLevel.values())
        {
            mMap.put(sItem.getValue(), sItem);
        }
    }

    public byte getValue()
    {
        return mValue;
    }

    ShardTransactionLevel(byte aValue)
    {
        this.mValue = aValue;
    }

    public static ShardTransactionLevel get(int aLevel)
    {
        aLevel = (aLevel < 0 || aLevel > 2) ? 1 : aLevel;
        return mMap.get((byte)aLevel);
    }
}
