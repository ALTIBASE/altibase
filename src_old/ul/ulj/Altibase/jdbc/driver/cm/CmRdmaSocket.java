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
import java.net.Inet6Address;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.sql.SQLException;
import java.util.logging.Level;
import java.util.logging.Logger;

import Altibase.jdbc.driver.JniExtLoader;
import Altibase.jdbc.driver.JniExtRdmaSocket;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;
import Altibase.jdbc.driver.logging.LoggingProxy;
import Altibase.jdbc.driver.logging.TraceFlag;
import Altibase.jdbc.driver.util.AltibaseProperties;

public class CmRdmaSocket implements CmSocket
{
    private int mRdmaSockId = CmSocket.INVALID_SOCKTFD; // rsocket identifier
    private int mResponseTimeout;                       // milliseconds 단위
    private int mIBLatency;                             // for RDMA_LATENCY rsocket option
    private int mIBConChkSpin;                          // for RDMA_CONCHKSPIN rsocket option

    private int mSockSndBufSize = CmChannel.CM_DEFAULT_SNDBUF_SIZE;
    private int mSockRcvBufSize = CmChannel.CM_DEFAULT_RCVBUF_SIZE;
    private static transient Logger mLogger;

    static
    {
        initialize();
    }

    public CmRdmaSocket(AltibaseProperties aProps)
    {
        mIBLatency = aProps.getIBLatency();
        mIBConChkSpin = aProps.getIBConChkSpin();
    }

    public CmConnType getConnType()
    {
        return CmConnType.IB;
    }

    public void open(SocketAddress aSockAddr,
                     String        aBindAddr,
                     int           aLoginTimeout) throws SQLException
    {
        try
        {
            if (aSockAddr == null || !(aSockAddr instanceof InetSocketAddress))
            {
                throw new IllegalArgumentException("unsupported address type");
            }

            int sDomain = RdmaSocketDef.AF_INET;
            InetSocketAddress sSockAddr = (InetSocketAddress)aSockAddr;

            if (sSockAddr.getAddress() instanceof Inet6Address)
                sDomain = RdmaSocketDef.AF_INET6;

            // rsocket 생성
            mRdmaSockId = JniExtRdmaSocket.rsocket(sDomain);

            if (aBindAddr != null)
            {
                JniExtRdmaSocket.rbind(mRdmaSockId, aBindAddr);
            }

            setKeepAlive(true);
            setSockRcvBufSize(mSockRcvBufSize); // PROJ-2625
            setSockSndBufSize(mSockSndBufSize);
            setTcpNoDelay(true);  // BUG-45275 disable nagle algorithm

            // Berkeley socket 과 다르게 blocking mode 이더라도
            // rsockets API 의 rconnect() 함수는 'connecting' 상태로 반환될 수 있음.
            // 그러므로 JNI 내부에서 non-blocking 모드로 잠시 변경하고 timeout 동작시킴.
            JniExtRdmaSocket.rconnect(mRdmaSockId,
                                      sSockAddr.getAddress().getHostAddress(),
                                      sSockAddr.getPort(),
                                      aLoginTimeout * 1000);

            // set, only if RDMA_LATENCY rsocket option is ON
            if (mIBLatency != 0)
            {
                setIBLatency(mIBLatency);
            }
            // set, only if RDMA_CONCHKSPIN rsocket option is ON
            if (mIBConChkSpin != 0)
            {
                setIBConChkSpin(mIBConChkSpin);
            }

            // BUG-47492 최초 socket 접속 시점에서는 login_timeout으로 response_timeout을 설정한다.
            mResponseTimeout = aLoginTimeout * 1000;
        }
        // connect 실패시 날 수 있는 예외가 한종류가 아니므로 모든 예외를 잡아 연결 실패로 처리한다.
        // 예를들어, AIX 6.1에서는 ClosedSelectorException가 나는데 이는 RuntimeException이다. (ref. BUG-33341)
        catch (Exception e)
        {
            Error.throwCommunicationErrorException(e);
        }
    }

    public void close() throws IOException
    {
        if (mRdmaSockId != CmSocket.INVALID_SOCKTFD)
        {
            JniExtRdmaSocket.rclose(mRdmaSockId);
            mRdmaSockId = CmSocket.INVALID_SOCKTFD;
        }
    }

    /**
     * non-blocking 모드로 JNI 내부에서 timeout 동작
     */
    public int read(ByteBuffer aBuffer) throws IOException
    {
        int sOffset = aBuffer.position();
        int sLength = aBuffer.limit() - sOffset;

        int sRead = JniExtRdmaSocket.rrecv(mRdmaSockId, aBuffer, sOffset, sLength, mResponseTimeout);

        aBuffer.position(aBuffer.position() + sRead);

        return sRead;
    }

    /**
     * non-blocking 모드로 JNI 내부에서 timeout 동작
     */
    public int write(ByteBuffer aBuffer) throws IOException
    {
        int sOffset = aBuffer.position();
        int sLength = aBuffer.limit() - sOffset;

        int sWritten = JniExtRdmaSocket.rsend(mRdmaSockId, aBuffer, sOffset, sLength, mResponseTimeout);

        aBuffer.position(aBuffer.position() + sWritten);

        return sWritten;
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
        if (mRdmaSockId != CmSocket.INVALID_SOCKTFD)
        {
            JniExtRdmaSocket.rsetsockopt(mRdmaSockId,
                                         RdmaSocketDef.SOL_SOCKET,
                                         RdmaSocketDef.SO_SNDBUF,
                                         aSockSndBufSize);
        }

        mSockSndBufSize = aSockSndBufSize;
    }

    public void setSockRcvBufSize(int aSockRcvBufSize) throws IOException
    {
        if (mRdmaSockId != CmSocket.INVALID_SOCKTFD)
        {
            JniExtRdmaSocket.rsetsockopt(mRdmaSockId,
                                         RdmaSocketDef.SOL_SOCKET,
                                         RdmaSocketDef.SO_RCVBUF,
                                         aSockRcvBufSize);
        }

        mSockRcvBufSize = aSockRcvBufSize;
    }

    private void setKeepAlive(boolean aKeepAlive) throws IOException
    {
        JniExtRdmaSocket.rsetsockopt(mRdmaSockId,
                                     RdmaSocketDef.SOL_SOCKET,
                                     RdmaSocketDef.SO_KEEPALIVE,
                                     aKeepAlive ? 1 : 0);
    }

    private void setTcpNoDelay(boolean aTcpNoDelay) throws IOException
    {
        JniExtRdmaSocket.rsetsockopt(mRdmaSockId,
                                     RdmaSocketDef.IPPROTO_TCP,
                                     RdmaSocketDef.TCP_NODELAY,
                                     aTcpNoDelay ? 1 : 0);
    }

    /**
     * RDMA_LATENCY rsocket options 을 설정한다.
     */
    private void setIBLatency(int aIBLatency) throws IOException
    {
        try
        {
            JniExtRdmaSocket.rsetsockopt(mRdmaSockId,
                                         RdmaSocketDef.SOL_RDMA,
                                         RdmaSocketDef.RDMA_LATENCY,
                                         aIBLatency);
        }
        catch (Throwable e)
        {
            // ignore, even if the 'librdmacm' library doesn't have RDMA_LATENCY rsocket option.
            // Because the original library doesn't have it and JDBC driver should run with it.
        }
    }

    /**
     * RDMA_CONCHKSPIN rsocket options 을 설정한다.
     */
    private void setIBConChkSpin(int aIBConChkSpin) throws IOException
    {
        if (aIBConChkSpin < 1 || RdmaSocketDef.RDMA_CONCHKSPIN_MAX < aIBConChkSpin)
        {
            Error.throwIllegalArgumentException(ErrorDef.INVALID_ARGUMENT,
                                                "IBConChkSpin",
                                                "1 ~ "+ RdmaSocketDef.RDMA_CONCHKSPIN_MAX, String.valueOf(aIBConChkSpin));
        }

        try
        {
            JniExtRdmaSocket.rsetsockopt(mRdmaSockId,
                                         RdmaSocketDef.SOL_RDMA,
                                         RdmaSocketDef.RDMA_CONCHKSPIN,
                                         aIBConChkSpin);
        }
        catch (Throwable e)
        {
            // ignore, even if the 'librdmacm' library doesn't have RDMA_CONCHKSPIN rsocket option.
            // Because the original library doesn't have it and JDBC driver should run with it.
        }
    }

    public int getSocketFD() throws SQLException
    {
        return mRdmaSockId;
    }

    public void setResponseTimeout(int aResponseTimeout)
    {
        mResponseTimeout = aResponseTimeout * 1000;
    }

    private static void initialize()
    {
        try
        {
            if (JniExtLoader.checkLoading())
            {
                JniExtRdmaSocket.load();
            }
        }
        catch (IOException aEx)
        {
            if (TraceFlag.TRACE_COMPILE && TraceFlag.TRACE_ENABLED)
            {
                if (mLogger != null)
                {
                    mLogger = Logger.getLogger(LoggingProxy.JDBC_LOGGER_DEFAULT);
                }
                mLogger.log(Level.WARNING, "Cannot load Rdma Socket.", aEx);
            }
        }
    }
}
