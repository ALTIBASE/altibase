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
 * $Id: acpSockSend.c 9434 2010-01-07 08:11:41Z denisb $
 ******************************************************************************/

#include <acpPoll.h>
#include <acpSock.h>
#include <acpTest.h>

/**
 * sends data to a socket
 * @param aSock pointer to the socket object
 * @param aBuffer pointer to the memory buffer to send
 * @param aSize size of data to send
 * @param aSendSize pointer to a variable to store the length of send data
 * @param aFlag flag; it can be composed of <br>
 *          #ACP_MSG_DONTWAIT   : Enables non-blocking operation.
 *                                When this value is set and the operation would
 *                                block, the return value can be caught with
 *                                #ACP_RC_IS_EAGAIN(). <br>
 *          #ACP_MSG_OOB        : Enables receipt of out-of-band data
 *                                which would not be received in the normal data
 *                                stream.
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock or @a aBuffer points outside of current process' address space.
 * @return #ACP_RC_EINVAL
 *      Invalid argument passed.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EAGAIN, #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and aBuffer cannot be sent immediately
 * @return #ACP_RC_EINTR
 *      Signaled while trying to send.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EACCES, WSAEACCES(Windows)
 *      Permission to write to @a *aSock is denied.
 *      Catch this with #ACP_RC_IS_EACCES().
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_ECONNREFUSED
 *      A remote host refused to allow the network connection.
 * @return #ACP_RC_EOPNOTSUPP
 *      #ACP_MSG_OOB was specified but @a *aSock is not
 *      a type of #ACP_SOCK_STREAM,
 *      or *aSock is unidirectional and supports only receive operations.
 * @return #ACP_RC_ENOMEM
 *      Not enough memory is available.
 * @return #ACP_RC_EDESTADDRREQ
 *      @a *aSock is not connection-mode.
 * @return #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and aBuffer cannot be sent immediately.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EHOSTUNREACH
 *      The host is unreachable.
 * @return #ACP_RC_ECONNABORTED
 *      The connection has been aborted.
 * @return #ACP_RC_ECONNRESET
 *      The peer system executed a hard or abortive close. @a *aSock must be
 *      closed. If @a *aSock is #ACP_SOCK_DGRAM type, this error would indicate
 *      that the previous send operation resulted in an ICMP Port Unreachable
 *      message.
 * @return #ACP_RC_ETIMEDOUT
 *      The connection has been dropped because of a network failure or because
 *      the peer system failed to respond.
 * @return #ACP_RC_ESHUTDOWN
 *      The socket has been shut down with #ACP_SHUT_WR or #ACP_SHUT_RDWR.
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAENETRESET(Windows)
 *      For ACP_SOCK_STREAM, the connection has been broken while the operation
 *      was in progress. For ACP_SOCK_DGRAM, the TTL(time to live) has expired.
 *      Catch this with #ACP_RC_IS_ECONNRESET()
 * @return WSAEMSGSIZE(Windows)
 *      The received message is too large to fit into @a *aBuffer[@a *aSize].
 *      Catch this with #ACP_RC_IS_ETRUNC()
 * @return ENOBUFS(Unix), WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 * @return WSAEINVAL(Windows)
 *      @a *aSock has not been bound with #acpSockBind(), or unknown flag in @a
 *      aFlag, or @a aSize is negative. Catch this with #ACP_RC_IS_EINVAL().
 */
ACP_EXPORT acp_rc_t acpSockSend(acp_sock_t   *aSock,
                                const void   *aBuffer,
                                acp_size_t    aSize,
                                acp_size_t   *aSendSize,
                                acp_sint32_t  aFlag)
{
    acp_ssize_t sRet;

    if (aSize > (acp_size_t)ACP_SINT32_MAX)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    ACP_SOCK_DO_IO(aSock,
                   aFlag,
                   ACP_POLL_OUT,
                   sRet = send(aSock->mHandle,
                               aBuffer,
                               ACP_SOCK_SIZE_TYPE aSize,
                               aFlag));

    if (sRet == ACP_SOCK_ERROR)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        if (aSendSize != NULL)
        {
            *aSendSize = (acp_size_t)sRet;
        }
        else
        {
            /* do nothing */
        }

        return ACP_RC_SUCCESS;
    }
}



/**
 * sends data with size from a socket
 * @param aSock pointer to the socket object
 * @param aBuffer pointer to the memory buffer to send
 * @param aSize size of data to send
 * @param aSendSize pointer to a variable to store the length of send data
 * @param aFlag flag; it can be composed of <br>
 *          #ACP_MSG_DONTWAIT   : Enables non-blocking operation.
 *                                When this value is set and the operation would
 *                                block, the return value can be caught with
 *                                #ACP_RC_IS_EAGAIN(). <br>
 *          #ACP_MSG_OOB        : Enables receipt of out-of-band data
 *                                which would not be received in the normal data
 *                                stream.
 * @param aTimeout maximum time to poll
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock or @a aBuffer points outside of current process' address space.
 * @return #ACP_RC_EINVAL
 *      Invalid argument passed.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EAGAIN, #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and aBuffer cannot be sent immediately
 * @return #ACP_RC_EINTR
 *      Signaled while trying to send.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EACCES, WSAEACCES(Windows)
 *      Permission to write to @a *aSock is denied.
 *      Catch this with #ACP_RC_IS_EACCES().
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_ECONNREFUSED
 *      A remote host refused to allow the network connection.
 * @return #ACP_RC_EOPNOTSUPP
 *      #ACP_MSG_OOB was specified but @a *aSock is not
 *      a type of #ACP_SOCK_STREAM,
 *      or *aSock is unidirectional and supports only receive operations.
 * @return #ACP_RC_ENOMEM
 *      Not enough memory is available.
 * @return #ACP_RC_EDESTADDRREQ
 *      @a *aSock is not connection-mode.
 * @return #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and aBuffer cannot be sent immediately.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EHOSTUNREACH
 *      The host is unreachable.
 * @return #ACP_RC_ECONNABORTED
 *      The connection has been aborted.
 * @return #ACP_RC_ECONNRESET
 *      The peer system executed a hard or abortive close. @a *aSock must be
 *      closed. If @a *aSock is #ACP_SOCK_DGRAM type, this error would indicate
 *      that the previous send operation resulted in an ICMP Port Unreachable
 *      message.
 * @return #ACP_RC_ETIMEDOUT
 *      The connection has been dropped because of a network failure or because
 *      the peer system failed to respond, or @a aTimeout was expired.
 * @return #ACP_RC_ESHUTDOWN
 *      The socket has been shut down with #ACP_SHUT_WR or #ACP_SHUT_RDWR.
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAENETRESET(Windows)
 *      For ACP_SOCK_STREAM, the connection has been broken while the operation
 *      was in progress. For ACP_SOCK_DGRAM, the TTL(time to live) has expired.
 *      Catch this with #ACP_RC_IS_ECONNRESET()
 * @return WSAEMSGSIZE(Windows)
 *      The received message is too large to fit into @a *aBuffer[@a *aSize].
 *      Catch this with #ACP_RC_IS_ETRUNC()
 * @return ENOBUFS(Unix), WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 * @return WSAEINVAL(Windows)
 *      @a *aSock has not been bound with #acpSockBind(), or unknown flag in @a
 */
ACP_EXPORT acp_rc_t acpSockSendAll(acp_sock_t   *aSock,
                                   const void   *aBuffer,
                                   acp_size_t    aSize,
                                   acp_size_t   *aSendSize,
                                   acp_sint32_t  aFlag,
                                   acp_time_t    aTimeout)
{
    acp_size_t  sSize;
    acp_ssize_t sRet;
    acp_rc_t    sRC = ACP_RC_SUCCESS;

    if (aSize > (acp_size_t)ACP_SINT32_MAX)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    for (sSize = 0; sSize < aSize; sSize += (acp_size_t)sRet)
    {
        sRC = acpSockPoll(aSock, ACP_POLL_OUT, aTimeout);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            break;
        }
        else
        {
            sRet = send(aSock->mHandle,
                        (acp_char_t *)aBuffer + sSize,
                        ACP_SOCK_SIZE_TYPE (aSize - sSize),
                        aFlag);

            if (sRet == ACP_SOCK_ERROR)
            {
                sRC = ACP_RC_GET_NET_ERROR();
                break;
            }
            else
            {
                /* continue */
            }
        }
    }

    if (aSendSize != NULL)
    {
        *aSendSize = sSize;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * sends data to a socket
 * @param aSock pointer to the socket object
 * @param aBuffer pointer to the memory buffer to send
 * @param aSize size of data to send
 * @param aSendSize pointer to a variable to store the length of send data
 * @param aFlag flag; it can be composed of <br>
 *          #ACP_MSG_DONTWAIT   : Enables non-blocking operation.
 *                                When this value is set and the operation would
 *                                block, the return value can be caught with
 *                                #ACP_RC_IS_EAGAIN(). <br>
 *          #ACP_MSG_OOB        : Enables receipt of out-of-band data
 *                                which would not be received in the normal data
 *                                stream.
 * @param aAddr pointer to the address structure for accepted socket.
 *                  This value can be NULL and in that case,
 *                  @a aAddrLen must also be NULL
 * @param aAddrLen pointer to the length of address structure @a *aAddr
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock or @a aBuffer points outside of current process' address space.
 * @return #ACP_RC_EINVAL
 *      Invalid argument passed.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EAGAIN, #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and aBuffer cannot be sent immediately
 * @return #ACP_RC_EINTR
 *      Signaled while trying to send.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EACCES, WSAEACCES(Windows)
 *      Permission to write to @a *aSock is denied.
 *      Catch this with #ACP_RC_IS_EACCES().
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_ECONNREFUSED
 *      A remote host refused to allow the network connection.
 * @return #ACP_RC_EOPNOTSUPP
 *      #ACP_MSG_OOB was specified but @a *aSock is not
 *      a type of #ACP_SOCK_STREAM,
 *      or *aSock is unidirectional and supports only receive operations.
 * @return #ACP_RC_ENOMEM
 *      Not enough memory is available.
 * @return #ACP_RC_EDESTADDRREQ
 *      @a *aSock is not connection-mode and @a *aAddr is not specified.
 * @return #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and aBuffer cannot be sent immediately.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EHOSTUNREACH
 *      The host is unreachable.
 * @return #ACP_RC_ECONNABORTED
 *      The connection has been aborted.
 * @return #ACP_RC_EAFNOSUPPORT
 *      Current implementation does not support the specified address family.
 *      Catch this with #ACP_RC_IS_ENOTSUP().
 * @return #ACP_RC_ECONNRESET
 *      The peer system executed a hard or abortive close. @a *aSock must be
 *      closed. If @a *aSock is #ACP_SOCK_DGRAM type, this error would indicate
 *      that the previous send operation resulted in an ICMP Port Unreachable
 *      message.
 * @return #ACP_RC_ETIMEDOUT
 *      The connection has been dropped because of a network failure or because
 *      the peer system failed to respond.
 * @return #ACP_RC_EADDRNOTAVAIL
 *      Tried to send to an invalid address such as ADDR_ANY.
 * @return #ACP_RC_ESHUTDOWN
 *      The socket has been shut down with #ACP_SHUT_WR or #ACP_SHUT_RDWR.
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAENETRESET(Windows)
 *      For ACP_SOCK_STREAM, the connection has been broken while the operation
 *      was in progress. For ACP_SOCK_DGRAM, the TTL(time to live) has expired.
 *      Catch this with #ACP_RC_IS_ECONNRESET()
 * @return WSAEMSGSIZE(Windows)
 *      The received message is too large to fit into @a *aBuffer[@a *aSize].
 *      Catch this with #ACP_RC_IS_ETRUNC()
 * @return ENOBUFS(Unix), WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 * @return WSAEINVAL(Windows)
 *      @a *aSock has not been bound with #acpSockBind(), or unknown flag in @a
 */
ACP_EXPORT acp_rc_t acpSockSendTo(acp_sock_t            *aSock,
                                  const void            *aBuffer,
                                  acp_size_t             aSize,
                                  acp_size_t            *aSendSize,
                                  acp_sint32_t           aFlag,
                                  const acp_sock_addr_t *aAddr,
                                  acp_sock_len_t         aAddrLen)
{
    acp_ssize_t sRet;

    if (aSize > (acp_size_t)ACP_SINT32_MAX)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
    }

    ACP_SOCK_DO_IO(aSock,
                   aFlag,
                   ACP_POLL_OUT,
                   sRet = sendto(aSock->mHandle,
                                 aBuffer,
                                 ACP_SOCK_SIZE_TYPE aSize,
                                 aFlag,
                                 aAddr,
                                 aAddrLen));

    if (sRet == ACP_SOCK_ERROR)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        if (aSendSize != NULL)
        {
            *aSendSize = (acp_size_t)sRet;
        }
        else
        {
            /* do nothing */
        }

        return ACP_RC_SUCCESS;
    }
}
