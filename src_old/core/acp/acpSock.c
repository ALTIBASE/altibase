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
 
/*******************************************************************************
 * $Id: acpSock.c 9434 2010-01-07 08:11:41Z denisb $
 ******************************************************************************/

#include <acpPoll.h>
#include <acpSock.h>
#include <acpTest.h>

#if defined(ALTI_CFG_OS_WINDOWS)

static WSADATA gAcpSockWinInfo;

#endif


acp_rc_t acpSockInitialize(void)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    WORD         sVersion[] =
        {
            MAKEWORD(2, 2),
            MAKEWORD(2, 1),
            MAKEWORD(2, 0),
            0
        };
    acp_sint32_t sRet;
    acp_sint32_t i;

    for (i = 0; sVersion[i] != 0; i++)
    {
        sRet = WSAStartup(sVersion[i], &gAcpSockWinInfo);

        if ((sRet == 0) || (sRet != WSAVERNOTSUPPORTED))
        {
            break;
        }
    }

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_FROM_SYS_ERROR(sRet);
    }
#endif

    return ACP_RC_SUCCESS;
}

acp_rc_t acpSockFinalize(void)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_sint32_t sRet;

    sRet = WSACleanup();

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
#endif

    return ACP_RC_SUCCESS;
}

/**
 * opens a socket
 * @param aSock pointer to the socket object
 * @param aFamily address family to use;
 *      it can be one of <br>
 *          #ACP_AF_UNIX        : local communication <br>
 *          #ACP_AF_INET        : IPv4 Internet protocol <br>
 *          #ACP_AF_INET6       : IPv6 Internet protocol <br>
 * @param aType type of socket;
 *      it can be one of <br>
 *          #ACP_SOCK_STREAM    : Stream(TCP) type <br>
 *          #ACP_SOCK_DGRAM     : Datagram(UDP) type <br>
 * @param aProtocol protocol to use
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EACCES
 *      Permission to create a socket of
 *      the specified type and/or protocol is denied.
 * @return #ACP_RC_ENOTSUP
 *      Current implementation does not support the specified address family.
 * @return #ACP_RC_EINVAL
 *      Unknown protocol, or protocol family is not available.
 * @return #ACP_RC_EINVAL
 *      Invalid flags in @a aType.
 * @return #ACP_RC_EMFILE
 *      The calling process cannot open more socket.
 * @return #ACP_RC_ENFILE
 *      The system cannot open more socket.
 *      Not enough memory is available. 
 *      The socket cannot be created until sufficient resources are freed.
 * @return #ACP_RC_ENOMEM, ENOBUFS(Unix), WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 * @return #ACP_RC_EPROTONOSUPPORT, WSAEPROTOTYPE(Windows)
 *      The protocol type or the specified protocol is
 *      not supported within this domain.
 *      Catch this with #ACP_RC_IS_EPROTONOSUPPORT()
 * @return #ACP_RC_EAFNOSUPPORT, WSAESOCKTNOSUPPORT(Windows)
 *      Current implementation does not support the specified address family.
 *      Catch this with #ACP_RC_IS_ENOTSUP().
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 */
ACP_EXPORT acp_rc_t acpSockOpen(acp_sock_t   *aSock,
                                acp_sint32_t  aFamily,
                                acp_sint32_t  aType,
                                acp_sint32_t  aProtocol)
{
    aSock->mHandle = socket(aFamily, aType, aProtocol);

    if (aSock->mHandle == ACP_SOCK_INVALID_HANDLE)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        aSock->mBlockMode = ACP_TRUE;

        return ACP_RC_SUCCESS;
    }
}

/**
 * closes a socket
 * @param aSock pointer to the socket object
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aSock points outside of current process' address space.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EINTR
 *      #acpSockClose() was interrupted by a signal.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and there are data remaining to be sent.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 */
ACP_EXPORT acp_rc_t acpSockClose(acp_sock_t *aSock)
{
    acp_sint32_t sRet;

#if defined(ALTI_CFG_OS_WINDOWS)
    sRet = closesocket(aSock->mHandle);
#else
    sRet = close(aSock->mHandle);
#endif

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
}
/**
 * shutdowns one or both of read/write part of full-duplex connection
 * @param aSock pointer to the socket object
 * @param aHow part to shutdown; it can be one of <br>
 *          #ACP_SHUT_RD    : Shutdown read part of @a *aSock.<br>
 *          #ACP_SHUT_WR    : Shutdown write part of @a *aSock.<br>
 *          #ACP_SHUT_RDWR  : Shutdown both read and write part of @a *aSock.
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aSock points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAEINVAL(Windows)
 *      Invalid value in @a aHow. Catch this with #ACP_RC_IS_EINVAL().
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 */
ACP_EXPORT acp_rc_t acpSockShutdown(acp_sock_t *aSock, acp_sint32_t aHow)
{
    acp_sint32_t sRet;

    sRet = shutdown(aSock->mHandle, aHow);

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
}

/**
 * gets the blocking mode of a socket
 * @param aSock pointer to the socket object
 * @param aBlockMode pointer to the variable to get blocking mode
 *      @a *aBlockMode is set to<br>
 *                  #ACP_TRUE is set @a *aSock in blocking-mode,<br>
 *                  #ACP_FALSE otherwise.
 *      after return.
 * @return #ACP_RC_SUCCESS Successful
 * @return #ACP_RC_EFAULT
 *      @a aSock or @a aBlockMode points outside
 *      of current process' address space.
 */
ACP_EXPORT acp_rc_t acpSockGetBlockMode(acp_sock_t *aSock,
                                        acp_bool_t *aBlockMode)
{
    *aBlockMode = aSock->mBlockMode;

    return ACP_RC_SUCCESS;
}

/**
 * sets the blocking mode of a socket
 * @param aSock pointer to the socket object
 * @param aBlockMode
 *          #ACP_TRUE to set @a *aSock in blocking mode.<br>
 *          #ACP_FALSE to set @a *aSock in non-blocking mode.
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EACCES
 *      Operation is prohibited by locks held by other application.
 * @return #ACP_RC_EAGAIN
 *      Operation is prohibited by locks held by other application.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOLCK
 *      The system cannot open more locks.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 */
ACP_EXPORT acp_rc_t acpSockSetBlockMode(acp_sock_t *aSock,
                                        acp_bool_t  aBlockMode)
{
    acp_sint32_t sRet;

#if defined(ALTI_CFG_OS_WINDOWS)
    u_long       sVal;

    switch (aBlockMode)
    {
        case ACP_FALSE:
            sVal = 1;
            break;
        case ACP_TRUE:
        default:
            sVal = 0;
            break;
    }

    sRet = ioctlsocket(aSock->mHandle, FIONBIO, &sVal);

    if (sRet == 0)
    {
        aSock->mBlockMode = aBlockMode;

        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
#else
    sRet = fcntl(aSock->mHandle, F_GETFL, 0);

    if (sRet == ACP_SOCK_ERROR)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        switch (aBlockMode)
        {
            case ACP_FALSE:
                sRet |= O_NONBLOCK;
                break;
            case ACP_TRUE:
            default:
                sRet &= ~O_NONBLOCK;
                break;
        }

        sRet = fcntl(aSock->mHandle, F_SETFL, sRet);

        if (sRet == ACP_SOCK_ERROR)
        {
            return ACP_RC_GET_NET_ERROR();
        }
        else
        {
            aSock->mBlockMode = aBlockMode;

            return ACP_RC_SUCCESS;
        }
    }
#endif
}

/**
 * gets the option of a socket
 * @param aSock pointer to the socket object
 * @param aLevel option level
 * @param aOptName option name to get
 * @param aOptVal pointer to the memory to get option value
 * @param aOptLen pointer to the variable to get the length of the option value
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock, @a aOptVal or @a aOptLen points outside of
 *      current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EINVAL
 *      @a *aOptLen is invalid or @a *aOptVal is invalid with @a *aOptName
 * @return #ACP_RC_ENOPROTOOPT
 *      The option in @a *aOptVal is unknown.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 */
ACP_EXPORT acp_rc_t acpSockGetOpt(acp_sock_t     *aSock,
                                  acp_sint32_t    aLevel,
                                  acp_sint32_t    aOptName,
                                  void           *aOptVal,
                                  acp_sock_len_t *aOptLen)
{
#if defined(ALTI_CFG_OS_HPUX) && \
    defined(ACP_CFG_COMPILE_64BIT) && \
    defined(ALTI_CFG_CPU_PARISC)
    acp_sint32_t sRet;
    acp_sint32_t sOptLen;
    acp_rc_t     sRC;

    ACP_TEST_RAISE(NULL == aOptLen, HANDLE_FAULT);

    sOptLen = (acp_sint32_t)(*aOptLen);
    sRet = getsockopt(aSock->mHandle, aLevel, aOptName, aOptVal, &sOptLen);

    ACP_TEST_RAISE(sRet != 0, HANDLE_ERROR);

    *aOptLen = (acp_sock_len_t)sOptLen;
    if ((aLevel == SOL_SOCKET) && (aOptName == SO_ERROR))
    {
        acp_sint32_t *sError = (acp_sint32_t *)aOptVal;
        *sError = ACP_RC_FROM_SYS_ERROR(*sError);
    }
    else
    {
        /* do nothing */
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(HANDLE_FAULT);
    sRC = ACP_RC_EFAULT;

    ACP_EXCEPTION(HANDLE_ERROR);
    sRC = ACP_RC_GET_NET_ERROR();

    ACP_EXCEPTION_END;
    return sRC;
#else
    acp_sint32_t sRet;

    sRet = getsockopt(aSock->mHandle, aLevel, aOptName, aOptVal, aOptLen);

    if (sRet == 0)
    {
        if ((aLevel == SOL_SOCKET) && (aOptName == SO_ERROR))
        {
            acp_sint32_t *sError = (acp_sint32_t *)aOptVal;

            *sError = ACP_RC_FROM_SYS_ERROR(*sError);
        }
        else
        {
            /* do nothing */
        }

        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
#endif
}

/**
 * sets the option of a socket
 * @param aSock pointer to the socket object
 * @param aLevel option level
 * @param aOptName option name to set
 * @param aOptVal pointer to the memory that stores the option value
 * @param aOptLen length of the option value
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock, @a aOptVal or @a aOptLen points outside of
 *      current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket
 * @return #ACP_RC_EINVAL
 *      @a aOptLen is invalid, or @a *aOptVal is invalid with @a *aOptName
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a Socket.
 * @return #ACP_RC_ENOPROTOOPT
 *      The option in @a *aOptVal is unknown.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_ENOTCONN
 *      The connection has been reset.
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAENETRESET(Windows)
 *      The connection has timed out. Catch this with #ACP_RC_IS_ECONNRESET()
 */
ACP_EXPORT acp_rc_t acpSockSetOpt(acp_sock_t     *aSock,
                                  acp_sint32_t    aLevel,
                                  acp_sint32_t    aOptName,
                                  const void     *aOptVal,
                                  acp_sock_len_t  aOptLen)
{
    acp_sint32_t sRet;

    sRet = setsockopt(aSock->mHandle, aLevel, aOptName, aOptVal, aOptLen);

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
}




/**
 * binds an address to a socket
 * @param aSock pointer to the socket object
 * @param aAddr pointer to the address structure to bind
 * @param aAddrLen length of address structure
 * @param aReuseAddr
 *                  #ACP_TRUE if the address can be reused,<br>
 *                  #ACP_FALSE if not
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock, @a aOptVal or @a aOptLen points outside of
 *      current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a Socket.
 * @return #ACP_RC_EINVAL
 *      @a aAddrLen is wrong or @a *aSock is not in the family supported.
 * @return #ACP_RC_ENAMETOOLONG
 *      @a *aAddr is too long.
 * @return #ACP_RC_EACCES
 *      The address @a *aAddr is protected and the user is not a superuser.
 * @return #ACP_RC_EADDRINUSE
 *      The given address @a *aAddr is already in use.
 * @return #ACP_RC_ENOENT
 *      The file does not exist.
 * @return #ACP_RC_ENOMEM
 *      Insufficient kernel memory was available.
 * @return #ACP_RC_ENOTDIR
 *      A component of the path prefix is not a directory.
 * @return #ACP_RC_ELOOP
 *      Too many symbolic link during resolving @a *aAddr.
 * @return #ACP_RC_EADDRNOTAVAIL
 *      The requested interface does not exists,
 *      or the requested address is not local.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAENOBUFS(Windows)
 *      There are too many connections.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 */
ACP_EXPORT acp_rc_t acpSockBind(acp_sock_t            *aSock,
                                const acp_sock_addr_t *aAddr,
                                acp_sock_len_t         aAddrLen,
                                acp_bool_t             aReuseAddr)
{
    acp_sint32_t sReuseOpt = 1;
    acp_sint32_t sRet;
    acp_rc_t     sRC;

    if (aReuseAddr == ACP_TRUE)
    {
        sRC = acpSockSetOpt(aSock,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            &sReuseOpt,
                            (acp_sock_len_t)sizeof(sReuseOpt));
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        sRet = bind(aSock->mHandle, aAddr, aAddrLen);

        if (sRet == 0)
        {
            sRC = ACP_RC_SUCCESS;
        }
        else
        {
            sRC = ACP_RC_GET_NET_ERROR();
        }
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * listens a connection
 * @param aSock pointer to the socket object to listen
 * @param aBackLog size of listen queue
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EADDRINUSE
 *      Another socket is already listening on the same port.
 * @return #ACP_RC_EOPNOTSUPP
 *      @a *aSock is not a type of listening socket.
 * @return #ACP_RC_EADDRINUSE
 *      The given address @a *aAddr is already in use.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EISCONN
 *      @a *aSock is already connected.
 * @return WSAEINVAL(Windows)
 *      @a *aSock is not bound with #acpSockBind().
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAEMFILE(Windows)
 *      The calling process cannot open more socket.
 *      Catch this with #ACP_RC_IS_EMFILE().
 * @return WSAENOBUFS(Windows)
 *      No buffer space available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 */
ACP_EXPORT acp_rc_t acpSockListen(acp_sock_t   *aSock,
                                  acp_sint32_t  aBackLog)
{
    acp_sint32_t sRet;

    sRet = listen(aSock->mHandle, aBackLog);

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
}

/**
 * accepts a connection from a listening socket
 * @param aAcceptSock pointer to the socket object to accept
 *                  It shall inherit the blocking mode of aListenSock
 * @param aListenSock pointer to the listening socket object
 * @param aAddr pointer to the address structure for accepted socket.
 *                  This value can be NULL and in that case,
 *                  @a aAddrLen must also be NULL
 * @param aAddrLen pointer to the length of address structure @a *aAddr
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock, @a aListenSock, @a aAddr or @a aAddrLen
 *      points outside of current process' address space.
 * @return #ACP_RC_EAGAIN
 *      @a *aSock is non-blocking and no connections are present.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EACCES
 *      Operation is prohibited by locks held by other application.
 * @return #ACP_RC_ENOLCK
 *      The system cannot open more locks.
 * @return #ACP_RC_ECONNABORTED
 *      The connection has been aborted.
 * @return #ACP_RC_EMFILE, WSAEMFILE(Windows).
 *      The calling process cannot open more socket.
 *      Catch this with #ACP_RC_IS_EMFILE()
 * @return #ACP_RC_ENFILE
 *      The system cannot open more socket.
 * @return #ACP_RC_ENOMEM, ENOBUFS(Unix), WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 * @return #ACP_RC_EPERM
 *      Firewall rules forbid connection.
 * @return #ACP_RC_EOPNOTSUPP
 *      @a *aSock is not a type of #ACP_SOCK_STREAM.
 * @return #ACP_RC_EPROTO
 *      Protocol error. Catch this with #ACP_RC_IS_ECONNABORTED().
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and no connections are present to be accpted.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAEINVAL(Windows)
 *      The #acpSockListen() function call must precede #acpSockAccept().
 *      Catch this with #ACP_RC_IS_EINVAL().
 * @return WSAENOBUFS(Windows)
 *      No buffer space available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 */
ACP_EXPORT acp_rc_t acpSockAccept(acp_sock_t      *aAcceptSock,
                                  acp_sock_t      *aListenSock,
                                  acp_sock_addr_t *aAddr,
                                  acp_sock_len_t  *aAddrLen)
{
#if defined(ALTI_CFG_OS_HPUX) && \
    defined(ACP_CFG_COMPILE_64BIT) && \
    defined(ALTI_CFG_CPU_PARISC)
    acp_sint32_t sAddrLen;
    acp_rc_t sRC;
    if(NULL == aAddrLen)
    {
        aAcceptSock->mHandle = accept(aListenSock->mHandle, aAddr, NULL);
    }
    else
    {
        sAddrLen = (acp_sint32_t)(*aAddrLen);
        aAcceptSock->mHandle = accept(aListenSock->mHandle, aAddr, &sAddrLen);
    }

    if (aAcceptSock->mHandle == ACP_SOCK_INVALID_HANDLE)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        /* Accepted socket will inherit mBlockMode of listening socket */
        sRC = acpSockSetBlockMode(aAcceptSock, aListenSock->mBlockMode);
        if (ACP_RC_NOT_SUCCESS(sRC) &&
            aAcceptSock->mHandle != ACP_SOCK_INVALID_HANDLE)
        {
            (void)acpSockClose(aAcceptSock);
        }
        else
        {
            if(NULL != aAddrLen)
            {
                *aAddrLen = (acp_sock_len_t)sAddrLen;
            }
            else
            {
                /* Do nothing */
            }
        }
    }
    
    return sRC;
#else
    
    acp_rc_t sRC;
    
    aAcceptSock->mHandle = accept(aListenSock->mHandle, aAddr, aAddrLen);

    if (aAcceptSock->mHandle == ACP_SOCK_INVALID_HANDLE)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        /* Accepted socket will inherit mBlockMode of listening socket */
        sRC = acpSockSetBlockMode(aAcceptSock, aListenSock->mBlockMode);
        if (ACP_RC_NOT_SUCCESS(sRC) &&
            aAcceptSock->mHandle != ACP_SOCK_INVALID_HANDLE)
        {
            (void)acpSockClose(aAcceptSock);
        }
        else
        {
            /* do nothing */
        }
    }
    
    return sRC;
#endif
}

/**
 * gets the address of remote peer of the socket
 * @param aSock pointer to the socket object
 * @param aName pointer to the address structure
 * @param aNameLen pointer to the length of address structure
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT, WSAEFAULT(Windows).
 *      @a aSock, @a aName or @a aNameLen points
 *      outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EINVAL
 *      @a *aNameLen is invalid.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return ENOBUFS(Unix), WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 */
ACP_EXPORT acp_rc_t acpSockGetName(acp_sock_t      *aSock,
                                   acp_sock_addr_t *aName,
                                   acp_sock_len_t  *aNameLen)
{
#if defined(ALTI_CFG_OS_HPUX) && \
    defined(ACP_CFG_COMPILE_64BIT) && \
    defined(ALTI_CFG_CPU_PARISC)
    acp_sint32_t sRet;
    acp_sint32_t sNameLen;

    if(NULL == aNameLen)
    {
        sNameLen = sizeof(acp_sock_addr_t);
    }
    else
    {
        sNameLen = (acp_sint32_t)(*aNameLen);
    }
    sRet = getsockname(aSock->mHandle, aName, &sNameLen);

    if (sRet == 0)
    {
        if(NULL != aNameLen)
        {
            *aNameLen = (acp_sock_len_t)sNameLen;
        }
        else
        {
            /* Do nothing */
        }
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
#else
    acp_sint32_t sRet;
    sRet = getsockname(aSock->mHandle, aName, aNameLen);

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
#endif
}

/**
 * gets the address of remote peer of the socket
 * @param aSock pointer to the socket object
 * @param aName pointer to the address structure
 * @param aNameLen pointer to the length of address structure
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock, @a aName or @a aNameLen points
 *      outside of current process' address space.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EINVAL
 *      @a *aNameLen is invalid.
 * @return #ACP_RC_ENOMEM, ENOBUFS(Unix), WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 */
ACP_EXPORT acp_rc_t acpSockGetPeerName(acp_sock_t      *aSock,
                                       acp_sock_addr_t *aName,
                                       acp_sock_len_t  *aNameLen)
{
#if defined(ALTI_CFG_OS_HPUX) && \
    defined(ACP_CFG_COMPILE_64BIT) && \
    defined(ALTI_CFG_CPU_PARISC)
    acp_sint32_t sRet;
    acp_sint32_t sNameLen;

    if(NULL == aNameLen)
    {
        sNameLen = sizeof(acp_sock_addr_t);
    }
    else
    {
        sNameLen = (acp_sint32_t)(*aNameLen);
    }
    sRet = getpeername(aSock->mHandle, aName, &sNameLen);

    if (sRet == 0)
    {
        if(NULL != aNameLen)
        {
            *aNameLen = (acp_sock_len_t)sNameLen;
        }
        else
        {
            /* Do nothing */
        }
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
#else
    acp_sint32_t sRet;

    sRet = getpeername(aSock->mHandle, aName, aNameLen);

    if (sRet == 0)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_NET_ERROR();
    }
#endif
}
