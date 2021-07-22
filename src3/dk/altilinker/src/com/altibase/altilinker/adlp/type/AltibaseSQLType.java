/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.adlp.type;

/**
 * SQL type on ADLP protocol
 */
public interface AltibaseSQLType
{
    public static final int SQL_NONE      = 0;
    public static final int SQL_CHAR      = 1;
    public static final int SQL_VARCHAR   = 12;
    public static final int SQL_SMALLINT  = 5;
    public static final int SQL_TINYINT   = -6;
    public static final int SQL_DECIMAL   = 3;
    public static final int SQL_NUMERIC   = 2;
    public static final int SQL_FLOAT     = 6;
    public static final int SQL_INTEGER   = 4;
    public static final int SQL_REAL      = 7;
    public static final int SQL_DOUBLE    = 8;
    public static final int SQL_BIGINT    = -5;
    public static final int SQL_DATE      = 9;
    public static final int SQL_TIME      = 10;
    public static final int SQL_TIMESTAMP = 11;
    public static final int SQL_BINARY    = -2;
    public static final int SQL_BLOB      = 30;
    public static final int SQL_CLOB      = 40;
}
