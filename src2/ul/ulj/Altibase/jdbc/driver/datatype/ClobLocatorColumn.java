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

import java.io.InputStream;
import java.io.Reader;
import java.math.BigInteger;
import java.sql.BatchUpdateException;
import java.sql.Clob;
import java.sql.SQLException;

import Altibase.jdbc.driver.AltibaseClob;
import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.ObjectDynamicArray;

public class ClobLocatorColumn extends LobLocatorColumn
{
    private static class LocatorInfo
    {
        BigInteger mLocatorId;
        BigInteger mLobLength;
        byte[] mLobByteCache;
        char[] mLobCharCache;
        
        LocatorInfo(BigInteger aLocatorId, BigInteger aLength, byte[] aLobByteCache, char[] aLobCharCache)
        {
            mLocatorId = aLocatorId;
            mLobLength = aLength;
            mLobByteCache = aLobByteCache;
            mLobCharCache = aLobCharCache;
        }
    }

    private byte[] mLobByteCache;
    private char[] mLobCharCache;
    private Object mSource;
    
    ClobLocatorColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.CLOB);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.CLOB_LOCATOR;
    }

    public String getDBColumnTypeName()
    {
        return "CLOB";
    }
    
    public String getObjectClassName()
    {
        return Clob.class.getName();
    }

    protected Object getObjectSub() throws SQLException
    {
        if (getLobLength() > 0)
        {
            return getClobSub();
        }
        else
        {
            return null;
        }
    }

    protected InputStream getAsciiStreamSub() throws SQLException
    {
        return getClobSub().getAsciiStream();
    }

    protected Reader getCharacterStreamSub() throws SQLException
    {
        return getClobSub().getCharacterStream();
    }

    protected Clob getClobSub() throws SQLException
    {
        return LobObjectFactory.createClob(getLocatorId(), getLobLength(), getLobByteCache(), getLobCharCache());
    }

    private byte[] getLobByteCache()
    {
        return mLobByteCache;
    }

    private char[] getLobCharCache()
    {
        return mLobCharCache;
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ObjectDynamicArray)aArray).put(new LocatorInfo(mLocatorId, mLength, mLobByteCache, mLobCharCache));
    }

    public void storeTo()
    {
        mValues.add(new LocatorInfo(mLocatorId, mLength, mLobByteCache, mLobCharCache));
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        LocatorInfo sLocatorInfo = (LocatorInfo)((ObjectDynamicArray)aArray).get();
        mLocatorId = sLocatorInfo.mLocatorId;
        mLength = sLocatorInfo.mLobLength;
        mLobByteCache = sLocatorInfo.mLobByteCache;
        mLobCharCache = sLocatorInfo.mLobCharCache;
    }

    protected void loadFromSub(int aLoadIndex)
    {
        LocatorInfo sLocatorInfo = (LocatorInfo)(mValues.get(aLoadIndex));
        mLocatorId = sLocatorInfo.mLocatorId;
        mLength = sLocatorInfo.mLobLength;
        mLobByteCache = sLocatorInfo.mLobByteCache;
        mLobCharCache = sLocatorInfo.mLobCharCache;
    }

    void storeToLobArray(DynamicArray aArray) throws BatchUpdateException
    {
        Object sTarget = ((ObjectDynamicArray)aArray).get();
        
        if (sTarget != null)
        {
            ((LocatorInfo)sTarget).mLobByteCache = mLobByteCache;
            ((LocatorInfo)sTarget).mLobCharCache = mLobCharCache;
            ((LocatorInfo)sTarget).mLobLength = mLength;
            ((LocatorInfo)sTarget).mLocatorId = mLocatorId;
        }
        else
        {
            Error.throwBatchUpdateException(ErrorDef.BATCH_UPDATE_EXCEPTION_OCCURRED);
        }
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        LocatorInfo sLocatorInfo = readLocatorInfo(aChannel);
        mLocatorId = sLocatorInfo.mLocatorId;
        mLength = sLocatorInfo.mLobLength;
        mLobByteCache = sLocatorInfo.mLobByteCache;
        mLobCharCache = sLocatorInfo.mLobCharCache;
    }

    private LocatorInfo readLocatorInfo(CmChannel aChannel) throws SQLException
    {
        mChannel = aChannel;
        BigInteger sLocatorId = aChannel.readUnsignedLong();
        BigInteger sLength = aChannel.readUnsignedLong();
        byte aHasCachedData = aChannel.readByte();
        byte[] sLobByteCache;
        char[] sLobCharCache;

        // has cached data
        if (aHasCachedData == 1)
        {
            // BUGBUG need to consider reusable byte array
            sLobByteCache = new byte[sLength.intValue()];
            aChannel.readBytes(sLobByteCache);
            aChannel.checkDecodingBuffer(sLength.intValue());  /* BUG-46411 */
            String sCachedStr = aChannel.readString(sLobByteCache);
            sLobCharCache = sCachedStr.toCharArray();
        }
        else
        {
            sLobByteCache = null;
            sLobCharCache = null;
        }

        return new LocatorInfo(sLocatorId, sLength, sLobByteCache, sLobCharCache);
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((ObjectDynamicArray)aArray).put(readLocatorInfo(aChannel));
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(readLocatorInfo(aChannel));
    }

    //BUG-37584 java.sql.SQLException: The table structure has been modified.
    protected String getStringSub() throws SQLException
    {
        if (mLobCharCache != null)
        {
            return String.valueOf(mLobCharCache);
        }
        else
        {
            AltibaseClob sClob = (AltibaseClob) getClobSub();
            sClob.open(mChannel);
            return sClob.getSubString(1, (int) sClob.length());
        }
    }

    public Object getObject() throws SQLException
    {
        if (mSource != null)
        {
            return mSource;
        }
        else
        {
            AltibaseClob sClob = (AltibaseClob) getObjectSub();
            if (sClob != null)
            {
                sClob.open(mChannel);
            }
            return sClob;
        }
    }
    
    protected void setValueSub(Object aValue) throws SQLException
    {
        if ( (aValue instanceof Reader) || (aValue instanceof InputStream) || (aValue instanceof Clob) || (aValue instanceof char[]) || (aValue instanceof String))
        {
            mSource = aValue;
        }
        else
        {
            Error.throwSQLException(ErrorDef.UNSUPPORTED_TYPE_CONVERSION,
                                    aValue.getClass().getName(), getDBColumnTypeName());
        }
    }
}
