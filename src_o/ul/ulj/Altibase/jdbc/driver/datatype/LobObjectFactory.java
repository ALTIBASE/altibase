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
import java.sql.Blob;
import java.sql.Clob;

public abstract class LobObjectFactory
{
    private static LobObjectFactory mFactoryInstance;
    
    protected static void setFactoryImpl(LobObjectFactory aFactoryInstance)
    {
        mFactoryInstance = aFactoryInstance;
    }
    
    public static InputStream createBinaryStream(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        return mFactoryInstance.createBinaryStreamImpl(aLocatorId, aLobLength, aLobCache);
    }
    
    public static InputStream createAsciiStream(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        return mFactoryInstance.createAsciiStreamImpl(aLocatorId, aLobLength, aLobCache);
    }
    
    public static Reader createCharacterStream(long aLocatorId, long aLobByteLength,  byte[] aLobByteCache, char[] aLobCharCache)
    {
        return mFactoryInstance.createCharacterStreamImpl(aLocatorId, aLobByteLength, aLobByteCache, aLobCharCache);
    }
    
    public static Blob createBlob(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        return mFactoryInstance.createBlobImpl(aLocatorId, aLobLength, aLobCache);
    }

    public static Clob createClob(long aLocatorId, long aLobByteLength, byte[] aLobByteCache, char[] aLobCharCache)
    {
        return mFactoryInstance.createClobImpl(aLocatorId, aLobByteLength, aLobByteCache, aLobCharCache);
    }

    protected abstract InputStream createBinaryStreamImpl(long aLocatorId, long aLobLength, byte[] aLobCache);
    protected abstract InputStream createAsciiStreamImpl(long aLocatorId, long aLobLength, byte[] aLobCache);
    protected abstract Reader createCharacterStreamImpl(long aLocatorId, long aLobByteLength, byte[] aLobByteCache, char[] aLobCharCache);
    protected abstract Blob createBlobImpl(long aLocatorId, long aLobLength, byte[] aLobCache);
    protected abstract Clob createClobImpl(long aLocatorId, long aLobByteLength, byte[] aLobByteCache, char[] aLobCache);
}
