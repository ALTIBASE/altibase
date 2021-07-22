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
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.channels.ReadableByteChannel;
import java.nio.channels.WritableByteChannel;
import java.sql.SQLException;

import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.util.SocketUtils;

class CmTcpSocket implements CmSocket
{
    Socket                      mSocket;
    private int                 mSocketFD       = INVALID_SOCKTFD; // PROJ-2625
    private int                 mSockSndBufSize = CmChannel.CM_DEFAULT_SNDBUF_SIZE;
    private int                 mSockRcvBufSize = CmChannel.CM_DEFAULT_RCVBUF_SIZE;
    private WritableByteChannel mWriteChannel;
    private ReadableByteChannel mReadChannel;

    CmTcpSocket()
    {
    }

    public CmConnType getConnType()
    {
        return CmConnType.TCP;
    }

    public void open(SocketAddress aSockAddr,
                     String        aBindAddr,
                     int           aLoginTimeout) throws SQLException
    {
        try
        {
            mSocket = new Socket();

            connectTcpSocket(aSockAddr, aBindAddr, aLoginTimeout);
        }
        // connect 실패시 날 수 있는 예외가 한종류가 아니므로 모든 예외를 잡아 연결 실패로 처리한다.
        // 예를들어, AIX 6.1에서는 ClosedSelectorException가 나는데 이는 RuntimeException이다. (ref. BUG-33341)
        catch (Exception e)
        {
            Error.throwCommunicationErrorException(e);
        }
    }

    void connectTcpSocket(SocketAddress aSockAddr, String aBindAddr, int aLoginTimeout) throws IOException
    {
        if (aBindAddr != null)
        {
            mSocket.bind(new InetSocketAddress(aBindAddr, 0));
        }

        // BUG-47492 최초 socket 접속시에는 login_timeout으로 socket so timeout을 설정한다.
        if (aLoginTimeout > 0)
        {
            mSocket.setSoTimeout(aLoginTimeout * 1000);
        }

        mSocket.setKeepAlive(true);
        mSocket.setReceiveBufferSize(mSockRcvBufSize); // PROJ-2625
        mSocket.setSendBufferSize(mSockSndBufSize);
        mSocket.setTcpNoDelay(true);  // BUG-45275 disable nagle algorithm

        mSocket.connect(aSockAddr, aLoginTimeout * 1000);

        mWriteChannel = Channels.newChannel(mSocket.getOutputStream());
        mReadChannel = Channels.newChannel(mSocket.getInputStream());
    }

    public void close() throws IOException
    {
        if (mWriteChannel != null)
        {
            mWriteChannel.close();
            mWriteChannel = null;
        }

        if (mReadChannel != null)
        {
            mReadChannel.close();
            mReadChannel = null;
        }

        if (mSocket != null)
        {
            mSocket.close();
            mSocket = null;
        }

        mSocketFD = INVALID_SOCKTFD;
    }

    public int read(ByteBuffer aBuffer) throws IOException
    {
        return mReadChannel.read(aBuffer);
    }

    public int write(ByteBuffer aBuffer) throws IOException
    {
        return mWriteChannel.write(aBuffer);
    }

    public int getSockSndBufSize()
    {
        return mSockSndBufSize;
    }

    public int getSockRcvBufSize()
    {
        return mSockRcvBufSize;
    }

    public void setSockSndBufSize(int aSockSndBufSize) throws IOException
    {
        if (mSocket != null)
        {
            mSocket.setSendBufferSize(aSockSndBufSize);
        }

        mSockSndBufSize = aSockSndBufSize;
    }

    public void setSockRcvBufSize(int aSockRcvBufSize) throws IOException
    {
        if (mSocket != null)
        {
            mSocket.setReceiveBufferSize(aSockRcvBufSize);
        }

        mSockRcvBufSize = aSockRcvBufSize;
    }

    public int getSocketFD() throws SQLException
    {
        try
        {
            if (mSocketFD == INVALID_SOCKTFD)
            {
                mSocketFD = SocketUtils.getFileDescriptor(mSocket);
            }
        }
        catch (Exception e)
        {
            Error.throwSQLException(ErrorDef.COMMUNICATION_ERROR, "Failed to get a file descriptor of the socket.", e);
        }

        return mSocketFD;
    }

    public void setResponseTimeout(int aResponseTimeout) throws SQLException
    {
        try
        {
            mSocket.setSoTimeout(aResponseTimeout * 1000);
        }
        catch (Exception aEx)
        {
            Error.throwCommunicationErrorException(aEx);
        }
    }

}
