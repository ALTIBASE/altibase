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

public final class ColumnTypes
{
    public static final int NULL         = 0;
    public static final int BOOLEAN      = 16;
    public static final int SMALLINT     = 5;
    public static final int INTEGER      = 4;
    public static final int BIGINT       = -5;
    public static final int BINARY       = -2; // 사실상 BINARY가 GEOMETRY를 위한 타입
    public static final int BLOB         = 30;
    public static final int BLOB_LOCATOR = 31;
    public static final int CLOB         = 40;
    public static final int CLOB_LOCATOR = 41;
    public static final int REAL         = 7;
    public static final int DOUBLE       = 8;
    public static final int INTERVAL     = 10;
    public static final int CHAR         = 1;
    public static final int VARCHAR      = 12;
    public static final int NCHAR        = -8;
    public static final int NVARCHAR     = -9;
    public static final int ECHAR        = 60;
    public static final int EVARCHAR     = 61;
    public static final int BYTE         = 20001;
    public static final int VARBYTE      = 20003;
    public static final int NIBBLE       = 20002;
    public static final int DATE         = 9; // BUGBUG (2012-11-15) 지원하지 않는 타입이므로 DATE 타입으로 처리. 지원하게 되면 해당 값으로 바꿔야 한다.
    public static final int TIME         = 9; // BUGBUG (2012-11-15) 지원하지 않는 타입이므로 DATE 타입으로 처리. 지원하게 되면 해당 값으로 바꿔야 한다.
    public static final int TIMESTAMP    = 9;
    public static final int FLOAT        = 6;
    public static final int NUMBER       = 10002;
    public static final int NUMERIC      = 2;
    public static final int GEOMETRY     = 10003; // ColumnInfo로는 이 값을 받지만, 데이타를 주고 받을때는 쓰지 않는다.
    public static final int LIST         = 10001;
    public static final int FILETYPE     = 30001;
    public static final int BIT          = -7;
    public static final int VARBIT       = -100;
    public static final int NONE         = -999;

    private ColumnTypes()
    {
    }

    public static boolean isNCharType(int aType)
    {
        switch (aType)
        {
            case NCHAR:
            case NVARCHAR:
                return true;
            default:
                return false;
        }
    }

    public static boolean isGeometryType(int aType)
    {
        switch (aType)
        {
            case BINARY:
            case GEOMETRY:
                return true;
            default:
            	return false;
        }
    }

    public static String toString(int aType)
    {
        switch (aType)
        {
            case NULL         : return "NULL";
            case BOOLEAN      : return "BOOLEAN";
            case SMALLINT     : return "SMALLINT";
            case INTEGER      : return "INTEGER";
            case BIGINT       : return "BIGINT";
            case BINARY       : return "BINARY";
            case BLOB         : return "BLOB";
            case BLOB_LOCATOR : return "BLOB_LOCATOR";
            case CLOB         : return "CLOB";
            case CLOB_LOCATOR : return "CLOB_LOCATOR";
            case REAL         : return "REAL";
            case DOUBLE       : return "DOUBLE";
            case INTERVAL     : return "INTERVAL";
            case CHAR         : return "CHAR";
            case VARCHAR      : return "VARCHAR";
            case NCHAR        : return "NCHAR";
            case NVARCHAR     : return "NVARCHAR";
            case ECHAR        : return "ECHAR";
            case EVARCHAR     : return "EVARCHAR";
            case BYTE         : return "BYTE";
            case NIBBLE       : return "NIBBLE";
//          case DATE         : return "DATE";
//          case TIME         : return "TIME";
            case TIMESTAMP    : return "TIMESTAMP";
            case FLOAT        : return "FLOAT";
            case NUMBER       : return "NUMBER";
            case NUMERIC      : return "NUMERIC";
            case GEOMETRY     : return "GEOMETRY";
            case LIST         : return "LIST";
            case FILETYPE     : return "FILETYPE";
            case BIT          : return "BIT";
            case VARBIT       : return "VARBIT";
            case NONE         : return "NONE";
            default:
                return "UNKNOWN";
        }
    }
}
