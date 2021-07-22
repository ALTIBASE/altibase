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

package Altibase.jdbc.driver;

import java.sql.ParameterMetaData;
import java.sql.SQLException;
import java.util.List;
import java.util.Set;

import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.ColumnInfo;

public class AltibaseParameterMetaData extends WrapperAdapter implements ParameterMetaData
{
    private List mColumns;

    AltibaseParameterMetaData(List aColumns)
    {
        mColumns = aColumns;
    }

    public int getParameterCount() throws SQLException
    {
        return mColumns.size();
    }

    public String getParameterClassName(int aParamIndex) throws SQLException
    {
        return ((Column)mColumns.get(aParamIndex - 1)).getObjectClassName();
    }

    public int getParameterMode(int aParamIndex) throws SQLException
    {
        int sInOutMode = ((Column)mColumns.get(aParamIndex - 1)).getColumnInfo().getInOutTargetType();
        if (sInOutMode == ColumnInfo.IN_OUT_TARGET_TYPE_IN)
        {
            return parameterModeIn;
        }
        else if (sInOutMode == ColumnInfo.IN_OUT_TARGET_TYPE_INOUT)
        {
            return parameterModeInOut;
        }
        else if (sInOutMode == ColumnInfo.IN_OUT_TARGET_TYPE_OUT)
        {
            return parameterModeOut;
        }
        return parameterModeUnknown;
    }

    public int getParameterType(int aParamIndex) throws SQLException
    {
        Set<Integer> sMappedJdbcTypeSet = ((Column)mColumns.get(aParamIndex - 1)).getMappedJDBCTypes();
        return sMappedJdbcTypeSet.iterator().next();
    }

    public String getParameterTypeName(int aParamIndex) throws SQLException
    {
        return ((Column)mColumns.get(aParamIndex - 1)).getDBColumnTypeName();
    }

    public int getPrecision(int aParamIndex) throws SQLException
    {
        return ((Column)mColumns.get(aParamIndex - 1)).getColumnInfo().getPrecision();
    }

    public int getScale(int aParamIndex) throws SQLException
    {
        return ((Column)mColumns.get(aParamIndex - 1)).getColumnInfo().getScale();
    }

    public int isNullable(int aParamIndex) throws SQLException
    {
        boolean sNullable = ((Column)mColumns.get(aParamIndex - 1)).getColumnInfo().getNullable();
        if (sNullable == ColumnInfo.NULLABLE)
        {
            return parameterNullable;
        }
        return parameterNoNulls;
    }

    public boolean isSigned(int aParamIndex) throws SQLException
    {
        return ((Column)mColumns.get(aParamIndex - 1)).isNumberType();
    }
}
