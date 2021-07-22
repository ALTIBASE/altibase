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

public final class ColumnConst
{
    public static final int   MAX_CHAR_LENGTH   = 65534;             // MTD_CHAR_PRECISION_MAXIMUM
    public static final int   MAX_BIT_LENGTH    = 64000;             // MTD_BIT_STORE_PRECISION_MAXIMUM
    public static final short MAX_NIBBLE_LENGTH = 254 * 2;           // MTD_NIBBLE_PRECISION_MAXIMUM
    public static final int   MAX_BYTE_LENGTH   = 65534;             // MTD_BYTE_PRECISION_MAXIMUM
    public static final int   MAX_BINARY_LENGTH = Integer.MAX_VALUE; // MTD_BINARY_PRECISION_MAXIMUM
}
