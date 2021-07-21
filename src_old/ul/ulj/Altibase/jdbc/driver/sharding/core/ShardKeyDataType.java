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

public enum ShardKeyDataType
{
    CHAR(1), INTEGER(4), SMALLINT(5), VARCHAR(12), BIGINT(-5);

    int mVal;

    private static final Map<Integer, ShardKeyDataType> mMap = new HashMap<Integer, ShardKeyDataType>();

    static
    {
        for (ShardKeyDataType sItem : ShardKeyDataType.values())
        {
            mMap.put(sItem.getValue(), sItem);
        }
    }

    ShardKeyDataType(int aVal)
    {
        this.mVal = aVal;
    }

    int getValue()
    {
        return this.mVal;
    }

    public static ShardKeyDataType get(int aValue)
    {
        return mMap.get(aValue);
    }
}
