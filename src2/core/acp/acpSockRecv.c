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
 * $Id: acpSockRecv.c 9434 2010-01-07 08:11:41Z denisb $
 ******************************************************************************/

#include <acpPoll.h>
#include <acpSock.h>
#include <acpTest.h>

/**
 * receives data from a socket
 * @param aSock pointer to the socket object
 * @param aBuffer pointer to the memory buffer to store received data
 * @param aSize size of memory buffer @a aBuffer
 * @param aRecvSize pointer to a variable to store the length of received data
 *                  This value can be NULL not to get the received size.
 * @param aFlag flag;
 *      The value is composed of bitmasks <br>
 *          #ACP_MSG_DONTWAIT   : Enables non-blocking operation.
 *                                When this value is set and the operation would
 *                                block, the return value can be caught with
 *                                #ACP_RC_IS_EAGAIN(). <br>
 *          #ACP_MSG_OOB        : Enables receipt of out-of-band data
 *                                which would not be received in the normal data
 *                                stream. <br>
 *          #ACP_MSG_PEEK       : Causes the receive operation to return data
 *                                from the beginning of receive queue without
 *                                removing that data from the queue. The
 *                                subsequent #acpSockRecv() will receive the
 *                                same data again.
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT, WSAEFAULT(Windows)
 *      @a aSock or @a aBuffer points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EAGAIN, #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and there is no data to be receivied,
 *      or recieve timeout had been set and the timeout was expired
 *      before data was received.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EINTR
 *      Signaled while trying to receive.
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_ECONNREFUSED
 *      A remote host refused to allow the network connection.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EOPNOTSUPP
 *      #ACP_MSG_OOB was specified but @a *aSock is not
 *      a type of #ACP_SOCK_STREAM,
 *      or *aSock is unidirectional and supports only send operations.
 * @return #ACP_RC_ECONNABORTED
 *      The connection has been aborted. @a *aSock must be closed.
 * @return #ACP_RC_ETIMEDOUT
 *      The connection has been dropped because of a network failure or because
 *      the peer system failed to respond.
 * @return #ACP_RC_ECONNRESET
 *      The peer system executed a hard or abortive close. @a *aSock must be
 *      closed. If @a *aSock is #ACP_SOCK_DGRAM type, this error would indicate
 *      that the previous send operation resulted in an ICMP Port Unreachable
 *      message.
 * @return #ACP_RC_ESHUTDOWN
 *      The socket has been shut down with #ACP_SHUT_RD or #ACP_SHUT_RDWR.
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
 * @return WSAEINVAL(Windows)
 *      @a *aSock has not been bound with #acpSockBind(), or unknown flag in @a
 *      aFlag, or @a aSize is negative. Catch this with #ACP_RC_IS_EINVAL()
 */
ACP_EXPORT acp_rc_t acpSockRecv(acp_sock_t   *aSock,
                                void         *aBuffer,
                                acp_size_t    aSize,
                                acp_size_t   *aRecvSize,
                                acp_sint32_t  aFlag)
{
    acp_ssize_t sRet;
    acp_rc_t    sRC;

    if (aSize == 0)
    {
        sRet = 0;
        sRC  = ACP_RC_SUCCESS;
    }
    else
    {
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
                       ACP_POLL_IN,
                       sRet = recv(aSock->mHandle,
                                   aBuffer,
                                   ACP_SOCK_SIZE_TYPE aSize,
                                   aFlag));

        if (sRet == ACP_SOCK_ERROR)
        {
            return ACP_RC_GET_NET_ERROR();
        }
        else if (sRet == 0)
        {
            sRC = ACP_RC_EOF;
        }
        else
        {
            sRC = ACP_RC_SUCCESS;
        }
    }

    if (aRecvSize != NULL)
    {
        *aRecvSize = (acp_size_t)sRet;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * receives data with size from a socket
 * @param aSock pointer to the socket object
 * @param aBuffer pointer to the memory buffer to store received data
 * @param aSize size of memory buffer @a aBuffer
 * @param aRecvSize pointer to a variable to store the length of received data
 *                  This value can be NULL not to get the received size.
 * @param aFlag flag;
 *      The value is composed of bitmasks <br>
 *          #ACP_MSG_DONTWAIT   : Enables non-blocking operation.
 *                                When this value is set and the operation would
 *                                block, the return value can be caught with
 *                                #ACP_RC_IS_EAGAIN(). <br>
 *          #ACP_MSG_OOB        : Enables receipt of out-of-band data
 *                                which would not be received in the normal data
 *                                stream. <br>
 *          #ACP_MSG_PEEK       : Causes the receive operation to return data
 *                                from the beginning of receive queue without
 *                                removing that data from the queue. The
 *                                subsequent #acpSockRecv() will receive the
 *                                same data again.
 * @param aTimeout maximum time to poll
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT, WSAEFAULT(Windows)
 *      @a aSock or @a aBuffer points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EAGAIN, #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and there is no data to be receivied,
 *      or recieve timeout had been set and the timeout was expired
 *      before data was received.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EINTR
 *      Signaled while trying to receive.
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_ECONNREFUSED
 *      A remote host refused to allow the network connection.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EOPNOTSUPP
 *      #ACP_MSG_OOB was specified but @a *aSock is not
 *      a type of ACP_SOCK_STREAM,
 *      or *aSock is unidirectional and supports only send operations.
 * @return #ACP_RC_ECONNABORTED
 *      The connection has been aborted. @a *aSock must be closed.
 * @return #ACP_RC_ETIMEDOUT
 *      The connection has been dropped because of a network failure or because
 *      the peer system failed to respond, or @a aTimeOut was expired.
 * @return #ACP_RC_ECONNRESET
 *      The peer system executed a hard or abortive close. @a *aSock must be
 *      closed. If @a *aSock is #ACP_SOCK_DGRAM type, this error would indicate
 *      that the previous send operation resulted in an ICMP Port Unreachable
 *      message.
 * @return #ACP_RC_ESHUTDOWN
 *      The socket has been shut down with #ACP_SHUT_RD or #ACP_SHUT_RDWR.
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
 * @return WSAEINVAL(Windows)
 *      @a *aSock has not been bound with #acpSockBind(), or unknown flag in @a
 *      aFlag, or @a aSize is negative.
 */
ACP_EXPORT acp_rc_t acpSockRecvAll(acp_sock_t   *aSock,
                                   void         *aBuffer,
                                   acp_size_t    aSize,
                                   acp_size_t   *aRecvSize,
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
        sRC = acpSockPoll(aSock, ACP_POLL_IN, aTimeout);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            break;
        }
        else
        {
            sRet = recv(aSock->mHandle,
                        (acp_char_t *)aBuffer + sSize,
                        ACP_SOCK_SIZE_TYPE (aSize - sSize),
                        aFlag);

            if (sRet == ACP_SOCK_ERROR)
            {
                sRC = ACP_RC_GET_NET_ERROR();
                break;
            }
            else if (sRet == 0)
            {
                sRC = ACP_RC_EOF;
                break;
            }
            else
            {
                /* continue */
            }
        }
    }

    if (aRecvSize != NULL)
    {
        *aRecvSize = sSize;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * receives data from a socket
 * @param aSock pointer to the socket object
 * @param aBuffer pointer to the memory buffer to store received data
 * @param aSize size of memory buffer @a aBuffer
 * @param aRecvSize pointer to a variable to store the length of received data
 *                  This value can be NULL not to get the received size.
 * @param aFlag flag;
 *      The value is composed of bitmasks <br>
 *          #ACP_MSG_DONTWAIT   : Enables non-blocking operation.
 *                                When this value is set and the operation would
 *                                block, the return value can be caught with
 *                                #ACP_RC_IS_EAGAIN(). <br>
 *          #ACP_MSG_OOB        : Enables receipt of out-of-band data
 *                                which would not be received in the normal data
 *                                stream. <br>
 *          #ACP_MSG_PEEK       : Causes the receive operation to return data
 *                                from the beginning of receive queue without
 *                                removing that data from the queue. The
 *                                subsequent #acpSockRecv() will receive the
 *                                same data again.
 * @param aAddr pointer to the address structure for accepted socket.
 *                  This value can be NULL and in that case,
 *                  @a aAddrLen must also be NULL
 * @param aAddrLen pointer to the length of address structure @a *aAddr
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT, WSAEFAULT(Windows)
 *      @a aSock or @a aBuffer points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EAGAIN, #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and there is no data to be receivied,
 *      or recieve timeout had been set and the timeout was expired
 *      before data was received.
 *      Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EINTR
 *      Signaled while trying to receive.
 * @return #ACP_RC_ENOTCONN
 *      @a *aSock is not connected.
 * @return #ACP_RC_ECONNREFUSED
 *      A remote host refused to allow the network connection.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is in blocking and previous operation is not complete
 * @return #ACP_RC_EOPNOTSUPP
 *      #ACP_MSG_OOB was specified but @a *aSock is not
 *      a type of ACP_SOCK_STREAM,
 *      or *aSock is unidirectional and supports only send operations.
 * @return #ACP_RC_ECONNABORTED
 *      The connection has been aborted. @a *aSock must be closed.
 * @return #ACP_RC_ETIMEDOUT
 *      The connection has been dropped because of a network failure or because
 *      the peer system failed to respond.
 * @return #ACP_RC_ECONNRESET
 *      The peer system executed a hard or abortive close. @a *aSock must be
 *      closed. If @a *aSock is #ACP_SOCK_DGRAM type, this error would indicate
 *      that the previous send operation resulted in an ICMP Port Unreachable
 *      message.
 * @return #ACP_RC_ESHUTDOWN
 *      The socket has been shut down with #ACP_SHUT_RD or #ACP_SHUT_RDWR.
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
 * @return WSAEINVAL(Windows)
 *      @a *aSock has not been bound with #acpSockBind(), or unknown flag in @a
 *      aFlag, or @a aSize is negative.
 */
ACP_EXPORT acp_rc_t acpSockRecvFrom(acp_sock_t      *aSock,
                                    void            *aBuffer,
                                    acp_size_t       aSize,
                                    acp_size_t      *aRecvSize,
                                    acp_sint32_t     aFlag,
                                    acp_sock_addr_t *aAddr,
                                    acp_sock_len_t  *aAddrLen)
{
    acp_ssize_t sRet;
    acp_rc_t    sRC;

    if (aSize == 0)
    {
        sRet = 0;
        sRC  = ACP_RC_SUCCESS;
    }
    else
    {
        if (aSize > (acp_size_t)ACP_SINT32_MAX)
        {
            return ACP_RC_EINVAL;
        }
        else
        {
        }

#if defined(ALTI_CFG_OS_HPUX) && \
    defined(ACP_CFG_COMPILE_64BIT) && \
    defined(ALTI_CFG_CPU_PARISC)
        {
            acp_sint32_t sAddrLen = (acp_sint32_t)(*aAddrLen);
            ACP_SOCK_DO_IO(aSock,
                           aFlag,
                           ACP_POLL_IN,
                           sRet = recvfrom(aSock->mHandle,
                                           aBuffer,
                                           ACP_SOCK_SIZE_TYPE aSize,
                                           aFlag,
                                           aAddr,
                                           &sAddrLen));
            *aAddrLen = sAddrLen;
        }
#else
        ACP_SOCK_DO_IO(aSock,
                       aFlag,
                       ACP_POLL_IN,
                       sRet = recvfrom(aSock->mHandle,
                                       aBuffer,
                                       ACP_SOCK_SIZE_TYPE aSize,
                                       aFlag,
                                       aAddr,
                                       aAddrLen));
#endif

        if (sRet == ACP_SOCK_ERROR)
        {
            return ACP_RC_GET_NET_ERROR();
        }
        else if (sRet == 0)
        {
            sRC = ACP_RC_EOF;
        }
        else
        {
            sRC = ACP_RC_SUCCESS;
        }
    }

    if (aRecvSize != NULL)
    {
        *aRecvSize = (acp_size_t)sRet;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}
