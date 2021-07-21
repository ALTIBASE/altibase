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

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

import java.sql.SQLException;
import java.util.HashMap;

public enum ShardSplitMethod
{
    NONE((byte)0), HASH((byte)1), RANGE((byte)2), LIST((byte)3), CLONE((byte)4), SOLO((byte)5);

    byte mValue;
    private static final HashMap<Byte, ShardSplitMethod> mShardSplitMap = new HashMap<Byte, ShardSplitMethod>();

    static
    {
        for (ShardSplitMethod sItem : ShardSplitMethod.values())
        {
            mShardSplitMap.put(sItem.getValue(), sItem);
        }
    }

    ShardSplitMethod(byte aVal)
    {
        mValue = aVal;
    }

    public static ShardSplitMethod get(byte aVal) throws SQLException
    {
        ShardSplitMethod sResult = mShardSplitMap.get(aVal);
        if (sResult == null)
        {
            Error.throwSQLException(ErrorDef.SHARD_SPLIT_METHOD_NOT_SUPPORTED, String.valueOf(aVal));
        }
        return mShardSplitMap.get(aVal);
    }

    byte getValue()
    {
        return mValue;
    }
}
