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

import Altibase.jdbc.driver.datatype.Column;

public class ShardValueInfo
{
    private ShardValueType mType;
    private int            mBindParamId;
    private Column         mValue;

    public void setType(byte aType)
    {
        this.mType = ShardValueType.get(aType);
    }

    public ShardValueType getType()
    {
        return this.mType;
    }

    public void setBindParamid(int aValue)
    {
        this.mBindParamId = aValue;
    }

    public void setValue(Column aValue)
    {
        this.mValue = aValue;
    }

    public Column getValue()
    {
        return mValue;
    }

    public int getBindParamId()
    {
        return this.mBindParamId;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("ShardValueInfo{");
        sSb.append("mType=").append(mType);
        sSb.append(", mBindParamId=").append(mBindParamId);
        sSb.append(", mValue=").append(mValue);
        sSb.append('}');
        return sSb.toString();
    }
}
