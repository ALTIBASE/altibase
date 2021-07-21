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

#ifndef _O_CMN_IB_H_
#define _O_CMN_IB_H_ 1

#include <acp.h>
#include <acl.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <aciVarString.h>

ACP_EXTERN_C_BEGIN

/* from $RDMA_CORE_HOME/librdmacm/rsocket.h */
#define SOL_RDMA 0x10000

enum {
    RDMA_SQSIZE     = 0,
    RDMA_RQSIZE     = 1,
    RDMA_INLINE     = 2,
    RDMA_IOMAPSIZE  = 3,
    RDMA_ROUTE      = 4,

    RDMA_LATENCY    = 1000,
    RDMA_CONCHKSPIN = 1001,
};

typedef struct cmnIBFuncs
{
    int (*rsocket)(int domain, int type, int protocol);
    int (*rbind)(int socket, const struct sockaddr *addr, socklen_t addrlen);
    int (*rlisten)(int socket, int backlog);
    int (*raccept)(int socket, struct sockaddr *addr, socklen_t *addrlen);
    int (*rconnect)(int socket, const struct sockaddr *addr, socklen_t addrlen);
    int (*rshutdown)(int socket, int how);
    int (*rclose)(int socket);

    ssize_t (*rrecv)(int socket, void *buf, size_t len, int flags);
    ssize_t (*rsend)(int socket, const void *buf, size_t len, int flags);
    ssize_t (*rread)(int socket, void *buf, size_t count);
    ssize_t (*rwrite)(int socket, const void *buf, size_t count);

    int (*rpoll)(struct pollfd *fds, nfds_t nfds, int timeout);
    int (*rselect)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

    int (*rgetpeername)(int socket, struct sockaddr *addr, socklen_t *addrlen);
    int (*rgetsockname)(int socket, struct sockaddr *addr, socklen_t *addrlen);

    int (*rsetsockopt)(int socket, int level, int optname, const void *optval, socklen_t optlen);
    int (*rgetsockopt)(int socket, int level, int optname, void *optval, socklen_t *optlen);
    int (*rfcntl)(int socket, int cmd, ... /* arg */ );
} cmnIBFuncs;

#define CMN_IB_RDMACM_LIB_NAME "rdmacm"

#define CMN_IB_LIB_ERROR_MSG_LEN (ACI_MAX_ERROR_MSG_LEN+256)

typedef struct cmnIB
{
    acp_dl_t     mRdmaCmHandle;
    cmnIBFuncs   mFuncs;

    /* This is to give the lazy error to client, when the shared library was failed to load. */
    acp_uint32_t mLibErrorCode;
    acp_char_t   mLibErrorMsg[CMN_IB_LIB_ERROR_MSG_LEN];
} cmnIB;

ACI_RC cmnIBInitialize(void);
ACI_RC cmnIBFinalize(void);

ACP_EXTERN_C_END

#endif /* _O_CMN_IB_H_ */
