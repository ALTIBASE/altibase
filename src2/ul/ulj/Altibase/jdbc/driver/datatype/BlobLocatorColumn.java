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
import java.math.BigInteger;
import java.sql.BatchUpdateException;
import java.sql.Blob;
import java.sql.SQLException;

import Altibase.jdbc.driver.AltibaseBlob;
import Altibase.jdbc.driver.AltibaseTypes;
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.ObjectDynamicArray;

public class BlobLocatorColumn extends LobLocatorColumn
{
    private static class LocatorInfo
    {
        BigInteger mLocatorId;
        BigInteger mLobLength;
        byte[] mLobCache;

        LocatorInfo(BigInteger aLocatorId, BigInteger aLength, byte[] aLobCache)
        {
            mLocatorId = aLocatorId;
            mLobLength = aLength;
            mLobCache = aLobCache;
        }
    }

    private byte[] mLobCache;
    private Object mSource;

    BlobLocatorColumn()
    {
        addMappedJdbcTypeSet(AltibaseTypes.BLOB);
        addMappedJdbcTypeSet(AltibaseTypes.VARBINARY);
        addMappedJdbcTypeSet(AltibaseTypes.LONGVARBINARY);
    }

    public int getDBColumnType()
    {
        return ColumnTypes.BLOB_LOCATOR;
    }

    public String getDBColumnTypeName()
    {
        return "BLOB";
    }

    public String getObjectClassName()
    {
        return Blob.class.getName();
    }

    protected Object getObjectSub() throws SQLException
    {
        if (getLobLength() > 0)
        {
            return getBlobSub();
        }
        else
        {
            return null;
        }
    }

    protected InputStream getBinaryStreamSub() throws SQLException
    {
        return getBlobSub().getBinaryStream();
    }

    protected Blob getBlobSub() throws SQLException
    {
        return LobObjectFactory.createBlob(getLocatorId(), getLobLength(), getLobCache());
    }

    protected void readFromSub(CmChannel aChannel) throws SQLException
    {
        LocatorInfo sLocatorInfo = readLocatorInfo(aChannel);
        mLocatorId = sLocatorInfo.mLocatorId;
        mLength = sLocatorInfo.mLobLength;
        mLobCache = sLocatorInfo.mLobCache;
    }

    private LocatorInfo readLocatorInfo(CmChannel aChannel) throws SQLException
    {
        if (mChannel == null)
        {
            mChannel = aChannel;
        }

        BigInteger sLocatorId = aChannel.readUnsignedLong();
        BigInteger sLength = aChannel.readUnsignedLong();
        byte sHasCachedData = aChannel.readByte();

        byte[] sLobCache;
        // has cached data
        if (sHasCachedData == 1)
        {
            // BUGBUG need to consider reusable byte array
            sLobCache = new byte[(int)sLength.longValue()];
            aChannel.readBytes(sLobCache);
        }
        else
        {
            sLobCache = null;
        }

        return new LocatorInfo(sLocatorId, sLength, sLobCache);
    }

    protected void readFromSub(CmChannel aChannel, DynamicArray aArray) throws SQLException
    {
        ((ObjectDynamicArray)aArray).put(readLocatorInfo(aChannel));
    }

    protected void readAndStoreValue(CmChannel aChannel) throws SQLException
    {
        mValues.add(readLocatorInfo(aChannel));
    }

    void storeToLobArray(DynamicArray aArray) throws BatchUpdateException
    {
        Object sTarget = ((ObjectDynamicArray)aArray).get();

        if (sTarget != null)
        {
            ((LocatorInfo)sTarget).mLobCache = mLobCache;
            ((LocatorInfo)sTarget).mLobLength = mLength;
            ((LocatorInfo)sTarget).mLocatorId = mLocatorId;
        }
        else
        {
            Error.throwBatchUpdateException(ErrorDef.BATCH_UPDATE_EXCEPTION_OCCURRED);
        }
    }

    public void storeTo(DynamicArray aArray)
    {
        ((ObjectDynamicArray)aArray).put(new LocatorInfo(mLocatorId, mLength, mLobCache));
    }

    public void storeTo()
    {
        mValues.add(new LocatorInfo(mLocatorId, mLength, mLobCache));
    }

    protected void loadFromSub(DynamicArray aArray)
    {
        LocatorInfo sLocatorInfo = (LocatorInfo)((ObjectDynamicArray)aArray).get();
        mLocatorId = sLocatorInfo.mLocatorId;
        mLength = sLocatorInfo.mLobLength;
        mLobCache = sLocatorInfo.mLobCache;
    }

    protected void loadFromSub(int aLoadIndex)
    {
        LocatorInfo sLocatorInfo = (LocatorInfo)(mValues.get(aLoadIndex));
        mLocatorId = sLocatorInfo.mLocatorId;
        mLength = sLocatorInfo.mLobLength;
        mLobCache = sLocatorInfo.mLobCache;
    }

    private byte[] getLobCache()
    {
        return mLobCache;
    }

    public Object getObject() throws SQLException
    {
        if (mSource != null)
        {
            return mSource;
        }
        else
        {
            AltibaseBlob sBlob = (AltibaseBlob) getObjectSub();
            if (sBlob != null)
            {
                sBlob.open(mChannel);
            }
            return sBlob;
        }
    }

    //BUG-37584 java.sql.SQLException: The table structure has been modified.
    protected byte[] getBytesSub() throws SQLException
    {
        AltibaseBlob sBlob = (AltibaseBlob) getBlobSub();
        sBlob.open(mChannel);
        return sBlob.getBytes(1, (int)sBlob.length());
    }

    protected void setValueSub(Object aValue) throws SQLException
    {
        if ( (aValue instanceof InputStream) || (aValue instanceof Blob) || (aValue instanceof byte[]) )
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
