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

import java.io.IOException;
import java.nio.ByteBuffer;

public class JniExtRdmaSocket
{
    //
    // 'librdmacm.so' dynamic loading and its symbols loading
    //
    public static native void load() throws IOException;
    public static native void unload();

    //
    // It is very similar to rsockets API,
    // but some native functions for JNI including rconnect(), rrecv(), rsend() contains timeout functionality.
    //
    public static native int rsocket(int aDomain) throws IOException;
    public static native void rbind(int aSocket, String aAddr) throws IOException;
    public static native void rconnect(int aSocket, String aAddr, int aPort, int aTimeout) throws IOException;
    public static native void rclose(int aSocket) throws IOException;

    public static native int rrecv(int aSocket, ByteBuffer aBuffer, int aOffset, int aLength, int aTimeout) throws IOException;
    public static native int rsend(int aSocket, ByteBuffer aBuffer, int aOffset, int aLength, int aTimeout) throws IOException;

    public static native void rsetsockopt(int aSocket, int aLevel, int aOptName, int aValue) throws IOException;
    public static native void rgetsockopt(int aSocket, int aLevel, int aOptName, int aValue) throws IOException;
}
