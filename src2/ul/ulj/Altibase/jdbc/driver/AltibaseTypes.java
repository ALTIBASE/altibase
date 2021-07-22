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

import java.sql.Types;

/**
 * {@link Types}의 확장.
 * <p>
 * {@link Types}에서 제공하는 contant 외에, Altibase를 위한 constant를 추가로 제공한다.
 */
public final class AltibaseTypes
{
    // #region JDBC types

    public static final int BIT           = Types.BIT;
    public static final int TINYINT       = Types.TINYINT;
    public static final int SMALLINT      = Types.SMALLINT;
    public static final int INTEGER       = Types.INTEGER;
    public static final int BIGINT        = Types.BIGINT;
    public static final int FLOAT         = Types.FLOAT;
    public static final int REAL          = Types.REAL;
    public static final int DOUBLE        = Types.DOUBLE;
    public static final int NUMERIC       = Types.NUMERIC;
    public static final int DECIMAL       = Types.DECIMAL;
    public static final int CHAR          = Types.CHAR;
    public static final int VARCHAR       = Types.VARCHAR;
    public static final int LONGVARCHAR   = Types.LONGVARCHAR;
    public static final int DATE          = Types.DATE;
    public static final int TIME          = Types.TIME;
    public static final int TIMESTAMP     = Types.TIMESTAMP;
    public static final int BINARY        = Types.BINARY;
    public static final int VARBINARY     = Types.VARBINARY;
    public static final int LONGVARBINARY = Types.LONGVARBINARY;
    public static final int NULL          = Types.NULL;
    public static final int OTHER         = Types.OTHER;
    public static final int JAVA_OBJECT   = Types.JAVA_OBJECT;
    public static final int DISTINCT      = Types.DISTINCT;
    public static final int STRUCT        = Types.STRUCT;
    public static final int ARRAY         = Types.ARRAY;
    public static final int BLOB          = Types.BLOB;
    public static final int CLOB          = Types.CLOB;
    public static final int REF           = Types.REF;
    public static final int DATALINK      = Types.DATALINK;
    public static final int BOOLEAN       = Types.BOOLEAN;

    // #endregion

    // #region Altibase specific types

    public static final int NCHAR         = -8;
    public static final int NVARCHAR      = -9;
    public static final int BYTE          = 20001;
    public static final int VARBYTE       = 20003;
    public static final int NIBBLE        = 20002;
    public static final int GEOMETRY      = 10003;
    public static final int VARBIT        = -100;
    public static final int NUMBER        = 10002;
    public static final int INTERVAL      = 10;

    // #endregion

    private AltibaseTypes()
    {
    }

    public static String toString(int aSqlType)
    {
        switch (aSqlType)
        {
            case BIT           : return "BIT";
            case TINYINT       : return "TINYINT";
            case SMALLINT      : return "SMALLINT";
            case INTEGER       : return "INTEGER";
            case BIGINT        : return "BIGINT";
            case FLOAT         : return "FLOAT";
            case REAL          : return "REAL";
            case DOUBLE        : return "DOUBLE";
            case NUMERIC       : return "NUMERIC";
            case DECIMAL       : return "DECIMAL";
            case CHAR          : return "CHAR";
            case VARCHAR       : return "VARCHAR";
            case LONGVARCHAR   : return "LONGVARCHAR";
            case DATE          : return "DATE";
            case TIME          : return "TIME";
            case TIMESTAMP     : return "TIMESTAMP";
            case BINARY        : return "BINARY";
            case VARBINARY     : return "VARBINARY";
            case LONGVARBINARY : return "LONGVARBINARY";
            case NULL          : return "NULL";
            case OTHER         : return "OTHER";
            case JAVA_OBJECT   : return "JAVA_OBJECT";
            case DISTINCT      : return "DISTINCT";
            case STRUCT        : return "STRUCT";
            case ARRAY         : return "ARRAY";
            case BLOB          : return "BLOB";
            case CLOB          : return "CLOB";
            case REF           : return "REF";
            case DATALINK      : return "DATALINK";
            case BOOLEAN       : return "BOOLEAN";
            case NCHAR         : return "NCHAR";
            case NVARCHAR      : return "NVARCHAR";
            case BYTE          : return "BYTE";
            case NIBBLE        : return "NIBBLE";
            case GEOMETRY      : return "GEOMETRY";
            case VARBIT        : return "VARBIT";
            case NUMBER        : return "NUMBER";
            case INTERVAL      : return "INTERVAL";
            default            : return "UNKNOWN(" + aSqlType + ")";
        }
    }
}
