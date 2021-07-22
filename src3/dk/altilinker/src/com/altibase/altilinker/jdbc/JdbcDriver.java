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
 
package com.altibase.altilinker.jdbc;

public class JdbcDriver 
{
    public String mDriverPath      = null;
    public String mDriverClassName = null;

    public String toString()
    {
        String sString = "";

        if( mDriverPath != null )
        {
            sString += mDriverPath;
        }
        else
        {
            /*do nothing*/
        }

        if( mDriverClassName != null )
        {
            sString += ": " + mDriverClassName;
        }
        else
        {
            /*do nothing*/
        }

        return sString;
    }
}
