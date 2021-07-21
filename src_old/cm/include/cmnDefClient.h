/**
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

#ifndef _O_CMN_DEF_CLIENT_H_
#define _O_CMN_DEF_CLIENT_H_ 1

#define CMN_TRC_DIR             "trc"

/*
 * Link Feature
 */

#define CMN_LINK_FEATURE_SYSDBA 0x00000002

/*
 * Link Flag
 */

#define CMN_LINK_FLAG_NONBLOCK  0x10000000

/*
    Define initial values of Handles and Sockets
*/
#define CMN_INVALID_SOCKET_HANDLE  -1 
#define CMN_INVALID_HANDLE    -1

/*
 * Duplex Connection
 */

typedef enum
{
    CMN_DIRECTION_RD   = ACP_SHUT_RD,
    CMN_DIRECTION_WR   = ACP_SHUT_WR,
    CMN_DIRECTION_RDWR = ACP_SHUT_RDWR
} cmnDirection;


/*
 * Link Type
 */

typedef enum
{
    CMN_LINK_TYPE_LISTEN  = 0,
    CMN_LINK_TYPE_PEER_SERVER,
    CMN_LINK_TYPE_PEER_CLIENT,
    CMN_LINK_TYPE_INVALID,
    CMN_LINK_TYPE_BASE    = 0,
    CMN_LINK_TYPE_MAX     = CMN_LINK_TYPE_INVALID
} cmnLinkType;


/*
 * Link Impl
 */

typedef enum
{
    CMN_LINK_IMPL_DUMMY   = 0,
    CMN_LINK_IMPL_TCP,
    CMN_LINK_IMPL_UNIX,
    CMN_LINK_IMPL_IPC,
    CMN_LINK_IMPL_SSL,     /* PROJ-2474 SSL/TLS */
    CMN_LINK_IMPL_IPCDA,   /* PROJ-2616 */
    CMN_LINK_IMPL_IB,      /* PROJ-2681 */
    CMN_LINK_IMPL_INVALID,
    CMN_LINK_IMPL_BASE    = CMN_LINK_IMPL_TCP,
    CMN_LINK_IMPL_MAX     = CMN_LINK_IMPL_INVALID
} cmnLinkImpl;


/*
 * Dispatcher Impl
 */

typedef enum
{
    CMN_DISPATCHER_IMPL_SOCK    = 0,
    CMN_DISPATCHER_IMPL_IPC,
    CMN_DISPATCHER_IMPL_IPCDA,   /* PROJ-2616 */
    CMN_DISPATCHER_IMPL_IB,      /* PROJ-2681 */
    CMN_DISPATCHER_IMPL_INVALID,
    CMN_DISPATCHER_IMPL_BASE    = 0,
    CMN_DISPATCHER_IMPL_MAX     = CMN_DISPATCHER_IMPL_INVALID
} cmnDispatcherImpl;


/*
 * Peer Link Info Key
 */

typedef enum cmnLinkInfoKey
{
    CMN_LINK_INFO_ALL = 0,
    CMN_LINK_INFO_IMPL,
    CMN_LINK_INFO_LOCAL_ADDRESS,
    CMN_LINK_INFO_LOCAL_IP_ADDRESS,
    CMN_LINK_INFO_LOCAL_PORT,
    CMN_LINK_INFO_REMOTE_ADDRESS,
    CMN_LINK_INFO_REMOTE_IP_ADDRESS,
    CMN_LINK_INFO_REMOTE_PORT,
    CMN_LINK_INFO_REMOTE_SOCKADDR,
    CMN_LINK_INFO_UNIX_PATH,
    CMN_LINK_INFO_IPC_KEY,
    CMN_LINK_INFO_IPCDA_KEY,             /*PROJ-2616 shared memory key*/
    CMN_LINK_INFO_TCP_KERNEL_STAT, /* PROJ-2625 */
    CMN_LINK_INFO_MAX
} cmnLinkInfoKey;


/*
 * Link Listen Arg
 */

typedef struct cmnLinkListenArgTCP
{
    acp_uint16_t  mPort;
    acp_uint32_t  mMaxListen;
    acp_uint32_t  mIPv6;
} cmnLinkListenArgTCP;

typedef struct cmnLinkListenArgUNIX
{
    acp_char_t   *mFilePath;
    acp_uint32_t  mMaxListen;
} cmnLinkListenArgUNIX;

typedef struct cmnLinkListenArgIPC
{
    acp_char_t   *mFilePath;
    acp_uint32_t  mMaxListen;
} cmnLinkListenArgIPC;

/* PROJ-2616 */
typedef struct cmnLinkListenArgIPCDA
{
    acp_char_t   *mFilePath;
    acp_uint32_t  mMaxListen;
} cmnLinkListenArgIPCDA;

/* PROJ-2681 */
typedef struct cmnLinkListenArgIB
{
    acp_uint16_t  mPort;
    acp_uint32_t  mMaxListen;
    acp_uint32_t  mIPv6;
    acp_uint32_t  mLatency;    /* for RDMA_LATENCY rsocket option */
    acp_uint32_t  mConChkSpin; /* for RDMA_CONCHKSPIN rsocket option */
} cmnLinkListenArgIB;

typedef union cmnLinkListenArg
{
    cmnLinkListenArgTCP   mTCP;
    cmnLinkListenArgUNIX  mUNIX;
    cmnLinkListenArgIPC   mIPC;
    cmnLinkListenArgIPCDA mIPCDA; /* PROJ-2616 */
    cmnLinkListenArgIB    mIB;    /* PROJ-2681 */
} cmnLinkListenArg;

/* proj-1538 ipv6
 * these macro values are defined according to those of
 * NET_CONN_IP_STACK server property.
 * So, be careful if you wanna change these values. */
#define NET_CONN_IP_STACK_V4_ONLY  0
#define NET_CONN_IP_STACK_V6_DUAL  1
#define NET_CONN_IP_STACK_V6_ONLY  2

/*
 * Link Connect Arg
 */

typedef struct cmnLinkConnectArgTCP
{
    acp_char_t   *mBindAddr;
    acp_char_t   *mAddr;
    acp_uint16_t  mPort;
    acp_uint32_t  mPreferIPv6;
} cmnLinkConnectArgTCP;

typedef struct cmnLinkConnectArgUNIX
{
    acp_char_t   *mFilePath;
} cmnLinkConnectArgUNIX;

typedef struct cmnLinkConnectArgIPC
{
    acp_char_t   *mFilePath;
} cmnLinkConnectArgIPC;

/* PROJ-2616 */
typedef struct cmnLinkConnectArgIPCDA
{
    acp_char_t   *mFilePath;
} cmnLinkConnectArgIPCDA;

/* PROJ-2474 SSL/TLS */
typedef struct cmnLinkConnectArgSSL
{
    /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
    acp_char_t   *mBindAddr;
    acp_char_t   *mAddr;
    acp_uint16_t  mPort;
    acp_uint32_t  mPreferIPv6;
    acp_char_t   *mCert;
    acp_char_t   *mCa;
    acp_char_t   *mCaPath;
    acp_char_t   *mCipher;
    acp_char_t   *mKey;
    acp_bool_t    mVerify;
} cmnLinkConnectArgSSL;

/* PROJ-2681 */
typedef struct cmnLinkConnectArgIB
{
    acp_char_t   *mBindAddr;
    acp_char_t   *mAddr;
    acp_uint16_t  mPort;
    acp_uint32_t  mPreferIPv6;
    acp_uint32_t  mLatency;    /* for RDMA_LATENCY rsocket option */
    acp_uint32_t  mConChkSpin; /* for RDMA_CONCHKSPIN rsocket option */
} cmnLinkConnectArgIB;

typedef union cmnLinkConnectArg
{
    cmnLinkConnectArgTCP   mTCP;
    cmnLinkConnectArgUNIX  mUNIX;
    cmnLinkConnectArgIPC   mIPC;
    cmnLinkConnectArgSSL   mSSL;   /* PROJ-2474 SSL/TLS */
    cmnLinkConnectArgIPCDA mIPCDA; /* PROJ-2616 */
    cmnLinkConnectArgIB    mIB;    /* PROJ-2681 */
} cmnLinkConnectArg;

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#define CMN_SET_ERRMSG_SOCK_OPT(aErrMsg, aMaxErrMsgLen, aOptStr, aErrno) \
    (void)acpSnprintf(aErrMsg, aMaxErrMsgLen,                            \
                      aOptStr", errno=%"ACI_INT32_FMT,                   \
                      aErrno)

#endif
