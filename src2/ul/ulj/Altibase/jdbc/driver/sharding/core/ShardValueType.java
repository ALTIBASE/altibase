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
 */

package Altibase.jdbc.driver.sharding.core;

import java.util.HashMap;
import java.util.Map;

/**
 * Shard 값의 Type을 정의한다. ( 0 : Host Variable, 1 : Constant Variable )
 */
public enum ShardValueType
{
    HOST_VAR((byte)0), CONST_VAL((byte)1);

    byte mValue;

    ShardValueType(byte aType)
    {
        mValue = aType;
    }

    private static final Map<Byte, ShardValueType> mMap = new HashMap<Byte, ShardValueType>();

    static
    {
        for (ShardValueType sItem : ShardValueType.values())
        {
            mMap.put(sItem.getValue(), sItem);
        }
    }

    byte getValue()
    {
        return mValue;
    }

    static ShardValueType get(byte atype)
    {
        return mMap.get(atype);
    }
}
