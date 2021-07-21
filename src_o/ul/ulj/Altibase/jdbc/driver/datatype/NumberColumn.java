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

package Altibase.jdbc.driver.datatype;

import Altibase.jdbc.driver.AltibaseTypes;

public class NumberColumn extends CommonNumericColumn
{
    NumberColumn()
    {
        super();
        addMappedJdbcTypeSet(AltibaseTypes.NUMBER);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.NUMBER;
    }

    public String getDBColumnTypeName()
    {
        return "NUMBER";
    }
    
    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)2, NumericColumn.NUMERIC_PRECISION, NumericColumn.NUMERIC_SCALE);
    }
}
