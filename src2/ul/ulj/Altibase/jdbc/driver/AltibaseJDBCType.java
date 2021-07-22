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

package Altibase.jdbc.driver;

import java.sql.SQLType;
import java.sql.Types;

public enum AltibaseJDBCType implements SQLType
{
    BIT(Types.BIT),
    TINYINT(Types.TINYINT),
    SMALLINT(Types.SMALLINT),
    INTEGER(Types.INTEGER),
    BIGINT(Types.BIGINT),
    FLOAT(Types.FLOAT),
    REAL(Types.REAL),
    DOUBLE(Types.DOUBLE),
    NUMERIC(Types.NUMERIC),
    DECIMAL(Types.DECIMAL),
    CHAR(Types.CHAR),
    VARCHAR(Types.VARCHAR),
    LONGVARCHAR(Types.LONGVARCHAR),
    DATE(Types.DATE),
    TIME(Types.TIME),
    TIMESTAMP(Types.TIMESTAMP),
    BINARY(Types.BINARY),
    VARBINARY(Types.VARBINARY),
    LONGVARBINARY(Types.LONGVARBINARY),
    NULL(Types.NULL),
    OTHER(Types.OTHER),
    JAVA_OBJECT(Types.JAVA_OBJECT),
    DISTINCT(Types.DISTINCT),
    STRUCT(Types.STRUCT),
    ARRAY(Types.ARRAY),
    BLOB(Types.BLOB),
    CLOB(Types.CLOB),
    REF(Types.REF),
    DATALINK(Types.DATALINK),
    BOOLEAN(Types.BOOLEAN),
    NCHAR(-8),
    NVARCHAR(-9),
    BYTE(20001),
    VARBYTE(20003),
    NIBBLE(20002),
    GEOMETRY(10003),
    VARBIT(-100),
    NUMBER(10002),
    INTERVAL(10);

    private final Integer mType;

    AltibaseJDBCType(final Integer aType)
    {
        this.mType = aType;
    }

    @Override
    public String getName()
    {
        return name();
    }

    @Override
    public String getVendor()
    {
        return "Altibase.jdbc.driver";
    }

    @Override
    public Integer getVendorTypeNumber()
    {
        return mType;
    }

    public static AltibaseJDBCType valueOf(int aType) {
        for( AltibaseJDBCType sSqlType : AltibaseJDBCType.class.getEnumConstants())
        {
            if(aType == sSqlType.mType)
            {
                return sSqlType;
            }
        }

        throw new IllegalArgumentException("Type:" + aType + " is not a valid "
                                           + "AltibaseJDBCType");
    }
}
