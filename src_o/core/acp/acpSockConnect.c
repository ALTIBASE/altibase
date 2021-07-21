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
 * $Id: acpSockConnect.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acpPoll.h>
#include <acpSock.h>
#include <acpTest.h>

/**
 * connects to the specified address
 * @param aSock pointer to the socket object
 * @param aAddr pointer to the address structure to connect
 * @param aAddrLen length of address structure @a *aAddr
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock or @a aAddr points outside of current process' address space
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EACCES
 *      Permission is denied for Unix-domain socket.
 * @return #ACP_RC_EACCES, #ACP_RC_EPERM
 *      The user tried to connect to a broadcast address
 *      without having the socket broadcast flag enabled,
 *      or the connection request failed bacause of a local firewall rule.
 * @return #ACP_RC_EAGAIN
 *      There is no free local ports or routing cache is full.
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EISCONN
 *      @a *aSock is already connected.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EINTR
 *      Signaled while trying to connect.
 * @return #ACP_RC_ETIMEDOUT
 *      Timeout while trying to establish a connection.
 * @return #ACP_RC_ECONNREFUSED
 *      No one is listening on the remote address @a *aAddr.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is non-blocking and the connection
 *      cannot be completed immediatly.
 * @return #ACP_RC_ENETUNREACH
 *      Network is unreachable.
 * @return #ACP_RC_EADDRINUSE
 *      Local address already in use.
 * @return #ACP_RC_EALREADY
 *      The socket is non-blocking and a previous connection attempt
 *      has not yet been completed.
 * @return #ACP_RC_EAFNOSUPPORT
 *      The address does not match with @a aAddr->sa_family.
 *      Catch this with #ACP_RC_IS_ENOTSUP()
 * @return #ACP_RC_EADDRNOTAVAIL
 *      Tried to connect to an invalid address such as ADDR_ANY.
 * @return #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and the connection cannot be established
 *      immediately. Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EINVAL
 *      @a *aSock is a listening socket.
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 */
ACP_EXPORT acp_rc_t acpSockConnect(acp_sock_t      *aSock,
                                   acp_sock_addr_t *aAddr,
                                   acp_sock_len_t   aAddrLen)
{
    acp_sint32_t sRet;

    sRet = connect(aSock->mHandle, aAddr, aAddrLen);

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
 * connects to the specified address with timeout
 * @param aSock pointer to the socket object
 * @param aAddr pointer to the address structure to connect
 * @param aAddrLen length of address structure @a *aAddr
 * @param aTimeout timeout to connect
 * @return #ACP_RC_SUCCESS Successful.
 * @return #ACP_RC_EFAULT
 *      @a aSock or @a aAddr points outside of current process' address space
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a valid socket.
 * @return #ACP_RC_EISCONN
 *      @a *aSock is already connected.
 * @return #ACP_RC_ENOTSOCK
 *      @a *aSock is not a socket.
 * @return #ACP_RC_EACCES
 *      Permission is denied for Unix-domain socket.
 * @return #ACP_RC_EACCES, #ACP_RC_EPERM
 *      The user tried to connect to a broadcast address
 *      without having the socket broadcast flag enabled,
 *      or the connection request failed bacause of a local firewall rule.
 * @return #ACP_RC_EAGAIN
 *      There is no free local ports or routing cache is full.
 * @return #ACP_RC_EINTR
 *      Signaled while trying to connect
 * @return #ACP_RC_ETIMEDOUT
 *      Timeout while trying to establish a connection.
 * @return #ACP_RC_ECONNREFUSED
 *      No one is listening on the remote address @a *aAddr.
 * @return #ACP_RC_EINPROGRESS
 *      @a *aSock is non-blocking and the connection
 *      cannot be established immediatly.
 * @return #ACP_RC_ENETUNREACH
 *      Network is unreachable.
 * @return #ACP_RC_EHOSTUNREACH
 *      The host is unreachable.
 * @return #ACP_RC_EADDRINUSE
 *      Local address already in use.
 * @return #ACP_RC_EALREADY
 *      @a *aSock non-blocking and a previous connection attempt
 *      has not yet been completed.
 * @return #ACP_RC_EAFNOSUPPORT
 *      The address does not match with @a aAddr->sa_family.
 *      Catch this with #ACP_RC_ENOTSUP()
 * @return #ACP_RC_EWOULDBLOCK
 *      @a *aSock is non-blocking and the connection cannot be established
 *      immediately. Catch this with #ACP_RC_IS_EAGAIN()
 * @return #ACP_RC_EINVAL
 *      @a *aSock is a listening socket.
 * @return WSANOTINITIALISED(Windows)
 *      A successful #acpInitialize() call must precede using sockets.
 *      Catch this with #ACP_RC_IS_NOTINITIALIZED()
 * @return WSAENETDOWN(Windows)
 *      The network subsystem has failed.
 *      Catch this with #ACP_RC_IS_NETUNUSABLE()
 * @return WSAENOBUFS(Windows)
 *      Not enough memory is available.
 *      The socket cannot be created until sufficient resources are freed.
 *      Catch this with #ACP_RC_IS_ENOMEM()
 */
ACP_EXPORT acp_rc_t acpSockTimedConnect(acp_sock_t      *aSock,
                                        acp_sock_addr_t *aAddr,
                                        acp_sock_len_t   aAddrLen,
                                        acp_time_t       aTimeout)
{
#define ACP_RESTORE_BLOCK_MODE_AND_RETURN(aRC)          \
    if (sBlockMode == ACP_TRUE)                         \
    {                                                   \
        (void)acpSockSetBlockMode(aSock, ACP_TRUE);     \
    }                                                   \
    else                                                \
    {                                                   \
    }                                                   \
    return (aRC)

    acp_bool_t     sBlockMode;
    acp_sock_len_t sErrorLen;
    acp_sint32_t   sError;
    acp_sint32_t   sRet;
    acp_rc_t       sRC;

    /*
     * stores blocking mode
     */
    sBlockMode = aSock->mBlockMode;

    /*
     * uses acpSockConnect
     * if the aSock is in blocking mode and aTimeout is infinite
     */
    if ((sBlockMode == ACP_TRUE) && (aTimeout == ACP_TIME_INFINITE))
    {
        return acpSockConnect(aSock, aAddr, aAddrLen);
    }
    else
    {
        /* do nothing */
    }

    /*
     * sets non-blocking mode
     */
    if (sBlockMode == ACP_TRUE)
    {
        sRC = acpSockSetBlockMode(aSock, ACP_FALSE);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
    }

    /*
     * non-blocking connect
     */
    sRet = connect(aSock->mHandle, aAddr, aAddrLen);

    if (sRet != 0)
    {
        sRC = ACP_RC_GET_NET_ERROR();

        if (ACP_RC_IS_EINPROGRESS(sRC))
        {
            /*
             * waits event up to timeout
             */
            sRC = acpSockPoll(aSock, ACP_POLL_OUT, aTimeout);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                ACP_RESTORE_BLOCK_MODE_AND_RETURN(sRC);
            }
            else
            {
                /* do nothing */
            }

            /*
             * gets error
             */
            sErrorLen = (acp_sock_len_t)sizeof(sError);
            sRC       = acpSockGetOpt(aSock,
                                      SOL_SOCKET,
                                      SO_ERROR,
                                      &sError,
                                      &sErrorLen);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                ACP_RESTORE_BLOCK_MODE_AND_RETURN(sRC);
            }
            else
            {
                ACP_RESTORE_BLOCK_MODE_AND_RETURN(sError);
            }
        }
        else
        {
            ACP_RESTORE_BLOCK_MODE_AND_RETURN(sRC);
        }
    }
    else
    {
        ACP_RESTORE_BLOCK_MODE_AND_RETURN(ACP_RC_SUCCESS);
    }

#undef ACP_RESTORE_BLOCK_MODE_AND_RETURN
}
