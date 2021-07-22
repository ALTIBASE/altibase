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

package Altibase.jdbc.driver.cm;

import java.sql.SQLException;
import java.util.ArrayList;

import Altibase.jdbc.driver.datatype.Column;

public class CmGetPropertyResult extends CmResult
{
    static final byte MY_OP = CmOperation.DB_OP_GET_PROPERTY_RESULT;
    
    private ArrayList mPropKeyList = new ArrayList();
    private ArrayList mPropValueList = new ArrayList();

    public CmGetPropertyResult()
    {
    }

    public Column getPropertyColumn(short aKey)
    {
        for (int i = 0; i < mPropKeyList.size(); i++)
        {
            if (((Short) mPropKeyList.get(i)).shortValue() == aKey)
            {
                return (Column) mPropValueList.get(i);
            }
        }
        return null;
    }

    public String getProperty(short aKey) throws SQLException
    {
        Column sColumn = getPropertyColumn(aKey);
        return (sColumn == null) ? null : sColumn.getString();
    }

    protected byte getResultOp()
    {
        return MY_OP;
    }

    void addProperty(short aKey, Column aValue)
    {
        mPropKeyList.add(aKey);
        mPropValueList.add(aValue);
    }
}
