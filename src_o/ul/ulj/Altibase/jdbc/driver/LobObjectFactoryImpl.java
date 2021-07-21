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

import java.io.InputStream;
import java.io.Reader;
import java.sql.Blob;
import java.sql.Clob;

import Altibase.jdbc.driver.datatype.LobObjectFactory;

class LobObjectFactoryImpl extends LobObjectFactory
{
    static void registerLobFactory()
    {
        setFactoryImpl(new LobObjectFactoryImpl());
    }

    LobObjectFactoryImpl()
    {
    }
    
    protected InputStream createBinaryStreamImpl(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        return new BlobInputStream(aLocatorId, aLobLength, aLobCache);
    }

    protected InputStream createAsciiStreamImpl(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        return new BlobInputStream(aLocatorId, aLobLength, aLobCache);
    }
    
    protected Reader createCharacterStreamImpl(long aLocatorId, long aLobByteLength, byte[] aLobByteCache, char[] aLobCharCache)
    {
        return new ClobReader(aLocatorId, aLobByteLength, aLobByteCache, aLobCharCache);
    }
    
    protected Blob createBlobImpl(long aLocatorId, long aLobLength, byte[] aLobCache)
    {
        return new AltibaseBlob(aLocatorId, aLobLength, aLobCache);
    }
    
    protected Clob createClobImpl(long aLocatorId, long aLobLength, byte[] aLobByteCache, char[] aLobCharCache)
    {
        return new AltibaseClob(aLocatorId, aLobLength, aLobByteCache, aLobCharCache);
    }
}
