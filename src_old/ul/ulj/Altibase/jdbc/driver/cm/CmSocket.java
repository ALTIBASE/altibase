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

import java.io.IOException;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.sql.SQLException;

public interface CmSocket
{
    int INVALID_SOCKTFD = -1;

    CmConnType getConnType();

    void open(SocketAddress aSockAddr,
              String        aBindAddr,
              int           aLoginTimeout) throws SQLException;

    void close() throws IOException;

    int read(ByteBuffer aBuffer) throws IOException;
    int write(ByteBuffer aBuffer) throws IOException;

    int getSockSndBufSize();
    int getSockRcvBufSize();
    void setSockSndBufSize(int aSockSndBufSize) throws IOException;
    void setSockRcvBufSize(int aSockRcvBufSize) throws IOException;

    int getSocketFD() throws SQLException;

    void setResponseTimeout(int aResponseTimeout) throws SQLException;
}
