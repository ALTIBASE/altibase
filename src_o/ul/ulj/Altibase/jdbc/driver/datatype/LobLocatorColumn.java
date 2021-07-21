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

import java.math.BigInteger;
import java.sql.SQLException;

import Altibase.jdbc.driver.cm.CmBufferWriter;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.ObjectDynamicArray;

public abstract class LobLocatorColumn extends AbstractColumn
{
    private static final BigInteger NULL_LOCATOR = BigInteger.ZERO;
    private static final BigInteger NULL_LENGTH  = BigInteger.ZERO;

    private static final int        LOB_LOCATOR_OCTET_LEGNTH = 8;
    private static final long       LONG_MAX = -1;

    protected BigInteger      mLocatorId;
    protected BigInteger      mLength;
    protected CmChannel       mChannel;
    
    LobLocatorColumn()
    {
    }

    public int getMaxDisplaySize()
    {
        return getColumnInfo().getPrecision();
    }

    public int getOctetLength()
    {
        return LOB_LOCATOR_OCTET_LEGNTH;
    }

    public DynamicArray createTypedDynamicArray()
    {
        return new ObjectDynamicArray();
    }

    public boolean isArrayCompatible(DynamicArray aArray)
    {
        return (aArray instanceof ObjectDynamicArray);
    }

    public int prepareToWrite(CmBufferWriter aBufferWriter) throws SQLException
    {
        return 8 + 4;
    }

    public int writeTo(CmBufferWriter aBufferWriter) throws SQLException
    {
        Error.throwInternalError(ErrorDef.INTERNAL_ASSERTION);
        return -1;
    }

    public void getDefaultColumnInfo(ColumnInfo aColumnInfo)
    {
        aColumnInfo.modifyColumnInfo(getDBColumnType(), (byte)0, 0, 0);
     }

    public long getLocatorId()
    {
        return mLocatorId.longValue();
    }
    
    public long getLobLength()
    {
        return mLength.longValue();
    }
    
    protected void setNullValue()
    {
        mLength = NULL_LENGTH;
    }

    public boolean isNullLocator()
    {
        return (mLocatorId == null)
            || (NULL_LOCATOR.compareTo(mLocatorId) == 0);
    }

    protected boolean isNullValueSet()
    {
        if (mLength == null || mLength.longValue() == LONG_MAX)
        {
            mLength = NULL_LENGTH;
        }
        // BUG-37418 길이가 0일때도 null로 보아야한다.
        return isNullLocator()
            || (NULL_LENGTH.compareTo(mLength) == 0);
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        Error.throwSQLException(ErrorDef.INVALID_METHOD_INVOCATION);
    }
}
