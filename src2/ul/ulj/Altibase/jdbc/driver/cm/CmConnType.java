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

package Altibase.jdbc.driver.cm;

public enum CmConnType
{
    None(0),
    TCP(1),
    SSL(6),
    IB(8);

    final int mConnType;

    private CmConnType(int aConnType)
    {
        mConnType = aConnType;
    }

    public int getValue()
    {
        return mConnType;
    }

    public static CmConnType toConnType(String aConnType)
    {
        for (CmConnType sConnType : CmConnType.values())
        {
            if (sConnType != CmConnType.None)
            {
                if (aConnType.equalsIgnoreCase(sConnType.toString()) ||          // e.g.) 'TCP' or 'tcp' (with ignoring case)
                    aConnType.equals(Integer.toString(sConnType.getValue())))    // e.g.) '1'
                {
                    return sConnType;
                }
            }
        }

        return CmConnType.None;
    }
}
