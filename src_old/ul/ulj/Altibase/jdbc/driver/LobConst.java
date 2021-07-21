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

public interface LobConst
{
    public static final int     LOB_BUFFER_SIZE                   = 32000;
    public static final int     FIXED_LOB_OP_LENGTH_FOR_PUTLOB    = 13;

    public static final byte    LOB_INITIAL_MODE_FOR_INPUT_STREAM = 0x00;
    public static final byte    LOB_COPY_MODE_FOR_INPUT_STREAM    = 0x01;
}
