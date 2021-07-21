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
 * $Id: acpSockPoll.c 9434 2010-01-07 08:11:41Z denisb $
 ******************************************************************************/

#include <acpPoll.h>
#include <acpSock.h>
#include <acpTest.h>

/**
 * waits an event from a socket
 * @param aSock pointer to the socket object
 * @param aEvent bitmask of event to wait = NULL;
 *      The value is composed of bitmasks <br>
 *          #ACP_POLL_IN    : to poll data received<br>
 *          #ACP_POLL_OUT   : to poll data sent
 * @param aTimeout waiting timeout
 * @return #ACP_RC_SUCCESS
 *      If the event is detected.
 * @return #ACP_RC_EFAULT
 *      @a aSock points outside of current process' address space.
 *      Catch this with #ACP_RC_IS_EFAULT().
 * @return #ACP_RC_EBADF
 *      @a *aSock is not a Socket.
 * @return #ACP_RC_EINVAL
 *      @a *aSock or @a aTimeout is invalid.
 * @return #ACP_RC_EINTR
 *      A signal was caught while waiting.
 * @return #ACP_RC_ETIMEDOUT
 *      Waiting for @a aTimeOut is expired.
 * @return #ACP_RC_ENOMEM
 *      There was no space to allocate file descriptor tables.
 */
ACP_EXPORT acp_rc_t acpSockPoll(acp_sock_t   *aSock,
                                acp_sint32_t  aEvent,
                                acp_time_t    aTimeout)
{
#if defined(ACP_POLL_USES_SELECT)
    fd_set          sHandleSet;
    struct timeval  sTimeout;
    struct timeval *sTimeoutPtr = NULL;
    acp_sint32_t    sRet;
    acp_rc_t        sRC;

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeoutPtr      = NULL;
    }
    else
    {
        sTimeout.tv_sec  = (long)acpTimeToSec(aTimeout);
        sTimeout.tv_usec = (long)acpTimeGetUsec(aTimeout);
        sTimeoutPtr      = &sTimeout;
    }

    FD_ZERO(&sHandleSet);
    FD_SET(aSock->mHandle, &sHandleSet);

#if defined(ALTI_CFG_OS_WINDOWS)

    sRet = select(0,
                  ((aEvent & ACP_POLL_IN)  != 0) ? &sHandleSet : NULL,
                  ((aEvent & ACP_POLL_OUT) != 0) ? &sHandleSet : NULL,
                  &sHandleSet,
                  sTimeoutPtr);
#else

    sRet = select(aSock->mHandle + 1,
                  ((aEvent & ACP_POLL_IN)  != 0) ? &sHandleSet : NULL,
                  ((aEvent & ACP_POLL_OUT) != 0) ? &sHandleSet : NULL,
                  &sHandleSet,
                  sTimeoutPtr);
#endif

    if (sRet == ACP_SOCK_ERROR)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }
    else if (sRet == 0)
    {
        sRC = ACP_RC_ETIMEDOUT;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
#elif defined(ACP_POLL_USES_EPOLL)

    struct epoll_event sChange; 
    struct epoll_event sEvent;

    acp_sint32_t       sTimeout;

    acp_sint32_t       sHandle;
    acp_sint32_t       sRet;
    acp_rc_t           sRC;

    sHandle = epoll_create(1);
    ACP_TEST_RAISE((sHandle == -1), E_POLLCREATE);

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeout = -1;
    }
    else
    {
        sTimeout = (acp_sint32_t)acpTimeToMsec(aTimeout + 999);
    }

    memset(&sEvent, 0, sizeof(struct epoll_event));
    sEvent.events   = aEvent;

    sRet = epoll_ctl(sHandle,
                     EPOLL_CTL_ADD,
                     aSock->mHandle,
                     &sEvent);

    ACP_TEST_RAISE((sRet == -1), E_POLLADD);

    sRet = epoll_wait(sHandle,
                      &sChange,
                      1,
                      sTimeout);

    if (sRet == -1)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }
    else if (sRet == 0)
    {
        sRC = ACP_RC_ETIMEDOUT;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    sRet = epoll_ctl(sHandle, EPOLL_CTL_DEL, aSock->mHandle, &sEvent);
    ACP_TEST_RAISE((sRet == -1), E_POLLDEL);

    (void)close(sHandle);

    return sRC;

    /* Handle exceptions */
    ACP_EXCEPTION(E_POLLCREATE)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }

    ACP_EXCEPTION(E_POLLADD)
    {
        sRC = ACP_RC_GET_NET_ERROR();

        (void)close(sHandle);
    }

    ACP_EXCEPTION(E_POLLDEL)
    {
        sRC = ACP_RC_GET_NET_ERROR();

        (void)close(sHandle);
    }

    ACP_EXCEPTION_END;

    return sRC;
#elif defined (ACP_POLL_USES_KQUEUE)

    acp_rc_t         sRC;
    acp_sint32_t     sHandle;
    acp_sint32_t     sEventNum;

    struct kevent    sChange;
    struct kevent    sEvent;

    struct timespec  *sTimeoutPtr = NULL;
    struct timespec  sTimeout;
    acp_sint64_t     sNsec;

    sHandle = kqueue();
    ACP_TEST_RAISE((sHandle == -1), E_KQUEUE);

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeoutPtr = NULL;
    }
    else
    {
        sNsec = acpTimeToMsec(aTimeout) * ACP_SINT64_LITERAL(1000000);
                                          
        sTimeout.tv_sec  = sNsec / ACP_SINT64_LITERAL(1000000000);
        sTimeout.tv_nsec = sNsec - 
                           sTimeout.tv_sec * ACP_SINT64_LITERAL(1000000000);

        sTimeoutPtr = &sTimeout;
    }

    EV_SET(&sChange, aSock->mHandle, aEvent, EV_ADD, 0, 0, 0);

    sEventNum = kevent(sHandle,
                       &sChange,
                       1,
                       &sEvent,
                       1,
                       sTimeoutPtr);

    if (sEventNum < 0)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }
    else if (sEventNum == 0)
    {
        sRC = ACP_RC_ETIMEDOUT;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    (void)close(sHandle);

    return sRC;

    /* Handle exceptions */
    ACP_EXCEPTION(E_KQUEUE)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }

    ACP_EXCEPTION_END;

    return sRC;
#else

    struct pollfd sPoll;
    acp_sint32_t  sTimeout;
    acp_sint32_t  sRet;
    acp_rc_t      sRC;

    sPoll.fd      = aSock->mHandle;
    sPoll.events  = aEvent;
    sPoll.revents = 0;

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeout = -1;
    }
    else
    {
        sTimeout = (acp_sint32_t)acpTimeToMsec(aTimeout + 999);
    }

    sRet = poll(&sPoll, 1, sTimeout);

    if (sRet == ACP_SOCK_ERROR)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }
    else if (sRet == 0)
    {
        sRC = ACP_RC_ETIMEDOUT;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }

    return sRC;
#endif
}
