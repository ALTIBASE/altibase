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

#include <cmAllClient.h>

extern cmnIB gIB;

#if !defined(ACP_MSG_DONTWAIT) && !defined(ACP_MSG_NONBLOCK)
static ACI_RC cmnSockCheckIOCTLIB(acp_sint32_t *aSock, acp_bool_t *aIsClosed)
{
    struct pollfd  sPollFd;
    acp_sint32_t   sTimeout = 0;
    fd_set         sFdSet;
    acp_sint32_t   sRet;
    acp_sint32_t   sSize = 0;

    ACI_TEST_RAISE(*aSock == CMN_INVALID_SOCKET_HANDLE, InvalidHandle);

    sPollFd.fd     = *aSock;
    sPollFd.events = POLLIN;

    /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
    ACI_EXCEPTION_CONT(Restart);

    sPollFd.revents = 0;

    sRet = gIB.mFuncs.rpoll(&sPollFd, 1, sTimeout);

    if (sRet < 0)
    {
        ACI_TEST_RAISE(errno == EINTR, Restart);

        ACI_RAISE(PollError);
    }
    else
    {
        /* timed out, if sRet == 0 */
        *aIsClosed = ID_FALSE;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidHandle);
    {
        errno = EBADF;
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    ACI_EXCEPTION(PollError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
#endif

#if !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT 0x4000
#endif

#if defined(MSG_PEEK)
static ACI_RC cmnSockCheckRECVIB(acp_sint32_t *aSock,
                                 acp_sint32_t  aFlag,
                                 acp_bool_t    aBlockingMode,
                                 acp_bool_t   *aIsClosed)
{
    acp_sint8_t    sBuf[1];
    acp_ssize_t    sRet;

#if !defined(MSG_DONTWAIT) && !defined(MSG_NONBLOCK)

    struct pollfd  sPollFd;
    acp_sint32_t   sTimeout = 0;

    sPollFd.fd     = *aSock;
    sPollFd.events = POLLIN;

    /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
    ACI_EXCEPTION_CONT(Restart);

    sPollFd.revents = 0;

    if ((aBlockingMode == ACP_TRUE) && ((aFlag & MSG_DONTWAIT) == MSG_DONTWAIT))
    {
        sRet = gIB.mFuncs.rpoll(&sPollFd, 1, sTimeout);

        if (sRet < 0)
        {
            ACI_TEST_RAISE(errno == EINTR, Restart);

            ACI_RAISE(PollError);
        }
        else if (sRet == 1)
        {
            *aIsClosed = ACP_TRUE;

            ACI_RAISE(SkipRecv);
        }
        else
        {
            sRet = gIB.mFuncs.rrecv(*aSock, sBuf, 1, aFlag);
        }
    }
    else
    {
        sRet = gIB.mFuncs.rrecv(*aSock, sBuf, 1, aFlag);
    }

#else

    ACP_UNUSED(aBlockingMode);

    ACI_EXCEPTION_CONT(Restart);

    sRet = gIB.mFuncs.rrecv(*aSock, sBuf, 1, aFlag);

#endif

    if (sRet == -1)
    {
        switch (errno)
        {
#if (EWOULDBLOCK != EAGAIN)
        case EAGAIN:
#endif
        case EWOULDBLOCK:
            *aIsClosed = ACP_FALSE;
            break;

        case ECONNRESET:
        case ECONNREFUSED:
            *aIsClosed = ACP_TRUE;
            break;

        /* BUG-35783 Add to handle SIGINT while calling recv() */
        case EINTR:
            ACI_RAISE(Restart);
            break;

        default:
            ACI_RAISE(RecvError);
            break;
        }
    }
    else if (sRet == 0)
    {
        *aIsClosed = ACP_TRUE;
    }
    else
    {
        *aIsClosed = ACP_FALSE;
    }

    return ACI_SUCCESS;

#if !defined(MSG_DONTWAIT) && !defined(MSG_NONBLOCK)

    ACI_EXCEPTION(PollError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }

#endif

    ACI_EXCEPTION(RecvError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RRECV_ERROR));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
#endif

ACI_RC cmnSockCheckIB( cmnLink      *aLink,
                       acp_sint32_t *aSock,
                       acp_bool_t   *aIsClosed )
{
    acp_bool_t sIsNonBlock = (aLink->mFeature & CMN_LINK_FLAG_NONBLOCK) ? ACP_TRUE : ACP_FALSE;

#if defined(MSG_PEEK) && defined(ACP_MSG_DONTWAIT)
    return cmnSockCheckRECVIB(aSock, MSG_PEEK | ACP_MSG_DONTWAIT, !sIsNonBlock, aIsClosed);
#elif defined(MSG_PEEK) && defined(ACP_MSG_NONBLOCK)
    return cmnSockCheckRECVIB(aSock, MSG_PEEK | ACP_MSG_NONBLOCK, !sIsNonBlock, aIsClosed);
#elif defined(MSG_PEEK)
    if (sIsNonBlock == ACP_TRUE)
    {
        return cmnSockCheckRECVIB(aSock, MSG_PEEK, aIsClosed);
    }
    else
    {
        return cmnSockCheckIOCTLIB(aSock, aIsClosed);
    }
#else
    ACP_UNUSED(sIsNonBlock);
    return cmnSockCheckIOCTLIB(aSock, aIsClosed);
#endif
}

ACI_RC cmnSockSetBlockingModeIB(acp_sint32_t *aSock, acp_bool_t aBlockingMode)
{
    acp_sint32_t sFlags = gIB.mFuncs.rfcntl(*aSock, F_GETFL, 0);

    if (aBlockingMode == ACP_TRUE)
    {
        ACI_TEST_RAISE(sFlags == -1, SetBlockingFail);

        ACI_TEST_RAISE(gIB.mFuncs.rfcntl(*aSock, F_SETFL, sFlags & ~O_NONBLOCK) == -1, SetBlockingFail);
    }
    else
    {
        ACI_TEST_RAISE(sFlags == -1, SetNonBlockingFail);

        ACI_TEST_RAISE(gIB.mFuncs.rfcntl(*aSock, F_SETFL, sFlags | O_NONBLOCK) == -1, SetNonBlockingFail);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(SetBlockingFail);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSOCKET_SET_BLOCKING_FAILED));
    }
    ACI_EXCEPTION(SetNonBlockingFail);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSOCKET_SET_NONBLOCKING_FAILED));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockRecvIB(cmbBlock       *aBlock,
                     cmnLinkPeer    *aLink,
                     acp_sint32_t   *aSock,
                     acp_uint16_t    aSize,
                     acp_time_t      aTimeout)
{
    acp_ssize_t sRet = 0;

    /*
     * aSize 이상 aBlock으로 데이터 읽음
     */
    while (aBlock->mDataSize < aSize)
    {
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != ACP_TIME_INFINITE)
        {
            ACI_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                           CMN_DIRECTION_RD,
                                           aTimeout) != ACI_SUCCESS);
        }

        /*
         * Socket으로부터 읽음
         */
        sRet = gIB.mFuncs.rrecv(*aSock,
                                 aBlock->mData + aBlock->mDataSize,
                                 (acp_size_t)(aBlock->mBlockSize - aBlock->mDataSize),
                                 0);

        ACI_TEST_RAISE(sRet == 0, ConnectionClosed);

        if (sRet == -1)
        {
            switch (errno)
            {
#if (EWOULDBLOCK != EAGAIN)
                case EAGAIN:
#endif
                case EWOULDBLOCK:
                    ACI_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                                   CMN_DIRECTION_RD,
                                                   aTimeout) != ACI_SUCCESS);
                    break;

                case ECONNRESET:
                case ECONNREFUSED:
                    ACI_RAISE(ConnectionClosed);
                    break;

                /* BUG-35783 Add to handle SIGINT while calling recv() */
                case EINTR:
                    break;

                default:
                    ACI_RAISE(RecvError);
                    break;
            }
        }
        else
        {
            aBlock->mDataSize += sRet;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(RecvError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RRECV_ERROR));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockSendIB(cmbBlock       *aBlock,
                     cmnLinkPeer    *aLink,
                     acp_sint32_t   *aSock,
                     acp_time_t      aTimeout)
{
    acp_ssize_t sRet;

    if (aBlock->mCursor == aBlock->mDataSize)
    {
        aBlock->mCursor = 0;
    }

    while (aBlock->mCursor < aBlock->mDataSize)
    {
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != ACP_TIME_INFINITE)
        {
            ACI_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                           CMN_DIRECTION_WR,
                                           aTimeout) != ACI_SUCCESS);
        }

        /*
         * socket으로 데이터 씀
         */
        sRet = gIB.mFuncs.rsend(*aSock,
                                aBlock->mData + aBlock->mCursor,
                                aBlock->mDataSize - aBlock->mCursor,
                                0);

        if (sRet == -1)
        {
            switch (errno)
            {
#if (EWOULDBLOCK != EAGAIN)
                case EAGAIN:
#endif
                case EWOULDBLOCK:
                    ACI_RAISE(Retry);
                    break;

                case EPIPE:
                    ACI_RAISE(ConnectionClosed);
                    break;

                case ETIMEDOUT:
                    ACI_RAISE(TimedOut);
                    break;

                /* BUG-35783 Add to handle SIGINT while calling recv() */
                case EINTR:
                    break;

                default:
                    ACI_RAISE(SendError);
                    break;
            }
        }
        else
        {
            aBlock->mCursor += sRet;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Retry);
    {
        ACI_SET(aciSetErrorCode(cmERR_RETRY_IB_RSOCKET_OPERATION_WOULD_BLOCK));
    }
    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(TimedOut);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    ACI_EXCEPTION(SendError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSEND_ERROR));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmnSockGetSndBufSizeIB(acp_sint32_t *aSock, acp_sint32_t *aSndBufSize)
{
    acp_sint32_t   sOptVal;
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_char_t     sErrMsg[256];

    ACI_TEST_RAISE(gIB.mFuncs.rgetsockopt(*aSock,
                                          SOL_SOCKET,
                                          SO_SNDBUF,
                                          (acp_char_t *)&sOptVal,
                                          (acp_sock_len_t *)&sOptLen) < 0, GetSockOptError);

    *aSndBufSize = sOptVal;

    return ACI_SUCCESS;

    ACI_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_SNDBUF", errno);

        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RGETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockSetSndBufSizeIB(acp_sint32_t *aSock, acp_sint32_t aSndBufSize)
{
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_char_t     sErrMsg[256];

    ACI_TEST_RAISE(gIB.mFuncs.rsetsockopt(*aSock,
                                          SOL_SOCKET,
                                          SO_SNDBUF,
                                          (const acp_char_t *)&aSndBufSize,
                                          (acp_sock_len_t)sOptLen) < 0, SetSockOptError);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_SNDBUF", errno);

        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockGetRcvBufSizeIB(acp_sint32_t *aSock, acp_sint32_t *aRcvBufSize)
{
    acp_sint32_t   sOptVal = 0;
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_char_t     sErrMsg[256];

    ACI_TEST_RAISE(gIB.mFuncs.rgetsockopt(*aSock,
                                          SOL_SOCKET,
                                          SO_RCVBUF,
                                          (acp_char_t *)&sOptVal,
                                          (acp_sock_len_t *)&sOptLen) < 0, GetSockOptError);

    *aRcvBufSize = sOptVal;

    return ACI_SUCCESS;

    ACI_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_RCVBUF", errno);

        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RGETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockSetRcvBufSizeIB(acp_sint32_t *aSock, acp_sint32_t aRcvBufSize)
{
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_char_t     sErrMsg[256];

    ACI_TEST_RAISE(gIB.mFuncs.rsetsockopt(*aSock,
                                          SOL_SOCKET,
                                          SO_RCVBUF,
                                          (const acp_char_t *)&aRcvBufSize,
                                          (acp_sock_len_t )sOptLen) < 0, SetSockOptError);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_RCVBUF", errno);

        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

