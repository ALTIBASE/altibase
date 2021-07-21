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
 
package com.altibase.altilinker.util;

/**
 * Data type related static methods
 */
public class DataType
{
    /**
     * Convert int (unsigned int) to long
     * 
     * @param aValue    int data type value
     * @return          long data type value
     */
    public static long uint2long(int aValue)
    {
        long sValue = (long)(0xffffffff & (long)aValue);
        return sValue;
    }

    /**
     * Convert short (unsigned short) to int
     * 
     * @param aValue    short data type value
     * @return          int data type value
     */
    public static int ushort2int(short aValue)
    {
        int sValue = (int)(0xffff & (int)aValue);
        return sValue;
    }

    /**
     * Convert byte (unsigned byte) to short
     * 
     * @param aValue    byte data type value
     * @return          short data type value
     */
    public static short ubyte2short(byte aValue)
    {
        short sValue = (short)(0xff & (short)aValue);
        return sValue;
    }
}
