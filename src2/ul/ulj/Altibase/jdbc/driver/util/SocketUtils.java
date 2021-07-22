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

package Altibase.jdbc.driver.util;

import java.io.FileDescriptor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.net.Socket;
import java.net.SocketImpl;

public class SocketUtils
{
    public static int getFileDescriptor(Socket aSocket) throws Exception
    {
        int sFD;

        Method SocketGetImpl = Socket.class.getDeclaredMethod("getImpl");
        Method SocketImplGetFileDescriptor = SocketImpl.class.getDeclaredMethod("getFileDescriptor");
        Field sPrivateFd = FileDescriptor.class.getDeclaredField("fd");

        SocketGetImpl.setAccessible(true);
        SocketImplGetFileDescriptor.setAccessible(true);
        sPrivateFd.setAccessible(true);

        SocketImpl sSocketImpl = (SocketImpl)SocketGetImpl.invoke(aSocket);
        FileDescriptor sFileDescriptor = (FileDescriptor)SocketImplGetFileDescriptor.invoke(sSocketImpl);
        sFD = ((Integer)sPrivateFd.get(sFileDescriptor)).intValue();

        return sFD;
    }
}
