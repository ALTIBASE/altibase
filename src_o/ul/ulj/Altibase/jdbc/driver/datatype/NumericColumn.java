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

public class NumericColumn extends CommonNumericColumn
{
    static final int NUMERIC_PRECISION = 38;
    static final int NUMERIC_SCALE = 20;
    
    NumericColumn()
    {
        super();
        addMappedJdbcTypeSet(AltibaseTypes.NUMERIC);
        addMappedJdbcTypeSet(AltibaseTypes.DECIMAL);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.NUMERIC;
    }

    public String getDBColumnTypeName()
    {
        return "NUMERIC";
    }
    
    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)2, NUMERIC_PRECISION, NUMERIC_SCALE);
    }
}
