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

/**
 * PROJ-2681
 * rsockets 에서 지원하는 socket domain, socket options 등 정의(Linux 기준)
 */
public class RdmaSocketDef
{
    // socket domain (from socket.h)
    public static final int AF_INET             = 2;  // PF_INET
    public static final int AF_INET6            = 10; // PF_INET6

    // socket option levels
    public static final int SOL_SOCKET          = 1;
    public static final int IPPROTO_TCP         = 6;
    public static final int IPPROTO_IPV6        = 41;
    public static final int SOL_RDMA            = 0x10000;

    // socket options of SOL_SOCKET (from socket.h)
    public static final int SO_REUSEADDR        = 2;
    public static final int SO_SNDBUF           = 7;
    public static final int SO_RCVBUF           = 8;
    public static final int SO_KEEPALIVE        = 9;
    public static final int SO_OOBINLINE        = 10;
    public static final int SO_LINGER           = 13;

    // socket options of IPPROTO_TCP (from tcp.h)
    public static final int TCP_NODELAY         = 1;
    public static final int TCP_MAXSEG          = 2;
    public static final int TCP_KEEPIDLE        = 4;
    public static final int TCP_KEEPINTVL       = 5;
    public static final int TCP_KEEPCNT         = 6;

    // socket options of IPPROTP_IPV6 (from in.h)
    public static final int IPV6_ONLY           = 26;

    // socket options of SOL_RDMA (from rsocket.h)
    public static final int RDMA_SQSIZE         = 0;
    public static final int RDMA_RQSIZE         = 1;
    public static final int RDMA_INLINE         = 2;
    public static final int RDMA_IOMAPSIZE      = 3;
    public static final int RDMA_ROUTE          = 4;
    public static final int RDMA_LATENCY        = 1000;
    public static final int RDMA_CONCHKSPIN     = 1001;
    public static final int RDMA_CONCHKSPIN_MAX = 2147483; // 1000 단위
}
