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
public enum GlobalTransactionLevel
{
    ONE_NODE((byte)0),
    MULTI_NODE((byte)1),
    GLOBAL((byte)2),
    GCTX((byte)3),
    NONE((byte)255);

    byte mValue;

    private static final Map<Byte, GlobalTransactionLevel> mMap = new HashMap<Byte, GlobalTransactionLevel>();

    static
    {
        for (GlobalTransactionLevel sItem : GlobalTransactionLevel.values())
        {
            mMap.put(sItem.getValue(), sItem);
        }
    }

    public byte getValue()
    {
        return mValue;
    }

    GlobalTransactionLevel(byte aValue)
    {
        this.mValue = aValue;
    }

    public static GlobalTransactionLevel get(short aLevel)
    {
        aLevel = (aLevel < 0 || aLevel > 3) ? NONE.getValue() : aLevel;
        return mMap.get((byte)aLevel);
    }
}
