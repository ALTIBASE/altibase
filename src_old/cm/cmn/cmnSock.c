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

#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)


#if !defined(ACP_MSG_DONTWAIT) && !defined(ACP_MSG_NONBLOCK)
static ACI_RC cmnSockCheckIOCTL(acp_sock_t *aSock, acp_bool_t *aIsClosed)
{
    fd_set         sFdSet;
    acp_rc_t       sRC
    acp_sint32_t   sSize = 0;

    ACI_TEST_RAISE(aSock->mHandle == CMN_INVALID_SOCKET_HANDLE, InvalidHandle);

    /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
    ACI_EXCEPTION_CONT(Restart);

    sRC = acpSockPoll(aSock, ACP_POLL_IN, ACP_TIME_IMMEDIATE);

    switch (sRC)
    {
        case ACP_RC_SUCCESS:
            /*
             * BUGBUG
             *
             *
            if (idlOS::ioctl(aSock, FIONREAD, &sSize) < 0)
            {
                *aIsClosed = ACP_TRUE;
            }
            else
            {
                *aIsClosed = (sSize == 0) ? ACP_TRUE : ACP_FALSE;
            }
            */

            break;

        case ACP_RC_ETIMEDOUT:
            *aIsClosed = ACP_FALSE;
            break;

        /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
        case ACP_RC_EINTR:
            ACI_RAISE(Restart);
            break;

        default:
            ACI_RAISE(SelectError);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(InvalidHandle);
    {
        errno = EBADF;
        /* BUG-47714 에러 메세지에 sock number 추가 */
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SELECT_ERROR, aSock->mHandle));
    }
    ACI_EXCEPTION(SelectError);
    {
        /* BUG-47714 에러 메세지에 sock number 추가 */
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SELECT_ERROR, aSock->mHandle));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
#endif

#if defined(ACP_MSG_PEEK)
static ACI_RC cmnSockCheckRECV(acp_sock_t *aSock, acp_sint32_t aFlag, acp_bool_t *aIsClosed)
{
    acp_sint8_t   sBuff[1];
    acp_size_t    sSize;
    acp_rc_t      sRC;
    acp_bool_t    sBreak = ACP_TRUE;

    do
    {
        sBreak = ACP_TRUE;

        sRC = acpSockRecv(aSock, sBuff, 1, &sSize, aFlag);

        switch (sRC)
        {
            case ACP_RC_SUCCESS:
                *aIsClosed = ACP_FALSE;
                break;

            case ACP_RC_EOF:
                *aIsClosed = ACP_TRUE;
                break;

#if (EWOULDBLOCK != EAGAIN)
            case ACP_RC_EAGAIN:
#endif
            case ACP_RC_EWOULDBLOCK:
                *aIsClosed = ACP_FALSE;
                break;
            case ACP_RC_ECONNRESET:
            case ACP_RC_ECONNREFUSED:
                *aIsClosed = ACP_TRUE;
                break;

            /* BUG-35783 Add to handle SIGINT while calling recv() */
            case ACP_RC_EINTR:
                sBreak = ACP_FALSE;
                break;

            default:
                ACI_RAISE(RecvError);
                break;
        }
    } while (sBreak != ACP_TRUE);

    return ACI_SUCCESS;

    ACI_EXCEPTION(RecvError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_RECV_ERROR));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
#endif

ACI_RC cmnSockCheck(cmnLink *aLink, acp_sock_t *aSock, acp_bool_t *aIsClosed)
{
#if defined(ACP_MSG_PEEK) && defined(ACP_MSG_DONTWAIT)
    aLink = aLink ; // To Fix Compiler Warning
    return cmnSockCheckRECV(aSock, ACP_MSG_PEEK | ACP_MSG_DONTWAIT, aIsClosed);
#elif defined(ACP_MSG_PEEK) && defined(ACP_MSG_NONBLOCK)
    aLink = aLink ; // To Fix Compiler Warning
    return cmnSockCheckRECV(aSock, ACP_MSG_PEEK | ACP_MSG_NONBLOCK, aIsClosed);
#elif defined(ACP_MSG_PEEK)
    acp_bool_t sIsNonBlock;

    sIsNonBlock = (aLink->mFeature & CMN_LINK_FLAG_NONBLOCK) ? ACP_TRUE : ACP_FALSE;

    if (sIsNonBlock == ACP_TRUE)
    {
        return cmnSockCheckRECV(aSock, ACP_MSG_PEEK, aIsClosed);
    }
    else
    {
        return cmnSockCheckIOCTL(aSock, aIsClosed);
    }
#else
    aLink = aLink ; // To Fix Compiler Warning
    return cmnSockCheckIOCTL(aSock, aIsClosed);
#endif
}

ACI_RC cmnSockSetBlockingMode(acp_sock_t *aSock, acp_bool_t aBlockingMode)
{
    if (aBlockingMode == ACP_TRUE)
    {
        ACI_TEST_RAISE(acpSockSetBlockMode(aSock, ACP_TRUE) != ACP_RC_SUCCESS,
                       SetBlockingFail);
    }
    else
    {
        ACI_TEST_RAISE(acpSockSetBlockMode(aSock, ACP_FALSE) != ACP_RC_SUCCESS,
                       SetNonBlockingFail);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(SetBlockingFail);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_SET_BLOCKING_FAILED));
    }
    ACI_EXCEPTION(SetNonBlockingFail);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_SET_NONBLOCKING_FAILED));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockRecv(cmbBlock       *aBlock,
                   cmnLinkPeer    *aLink,
                   acp_sock_t     *aSock,
                   acp_uint16_t    aSize,
                   acp_time_t      aTimeout)
{
    acp_size_t  sSize = 0;
    acp_rc_t    sRC;

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
        sRC = acpSockRecv(aSock,
                          aBlock->mData + aBlock->mDataSize,
                          aBlock->mBlockSize - aBlock->mDataSize,
                          &sSize,
                          0);

        switch (sRC)
        {
            case ACP_RC_SUCCESS:
                aBlock->mDataSize += sSize;
                break;

#if (EWOULDBLOCK != EAGAIN)
            case ACP_RC_EAGAIN:
#endif
            case ACP_RC_EWOULDBLOCK:
                ACI_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                               CMN_DIRECTION_RD,
                                               aTimeout) != ACI_SUCCESS);
                break;

            case ACP_RC_ECONNRESET:
            case ACP_RC_ECONNREFUSED:
            case ACP_RC_EOF:  /* BUG-44159 */
                ACI_RAISE(ConnectionClosed);
                break;

            /* BUG-35783 Add to handle SIGINT while calling recv() */
            case ACP_RC_EINTR:
                break;

            default:
                ACI_RAISE(RecvError);
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(RecvError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_RECV_ERROR));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockSend(cmbBlock       *aBlock,
                   cmnLinkPeer    *aLink,
                   acp_sock_t     *aSock,
                   acp_time_t      aTimeout)
{
    acp_size_t  sSize = 0;
    acp_rc_t    sRC;

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
        sRC = acpSockSend(aSock,
                          aBlock->mData + aBlock->mCursor,
                          aBlock->mDataSize - aBlock->mCursor,
                          &sSize,
                          0);

        switch (sRC)
        {
            case ACP_RC_SUCCESS:
                aBlock->mCursor += sSize;
                break;

#if (EWOULDBLOCK != EAGAIN)
            case ACP_RC_EAGAIN:
#endif
            case ACP_RC_EWOULDBLOCK:
                ACI_RAISE(Retry);
                break;

                /*
                 * BUGBUG
                 *
                 case EPIPE:
                 ACI_RAISE(ConnectionClosed);
                 break;
                */

            case ACP_RC_ETIMEDOUT:
                ACI_RAISE(TimedOut);
                break;

            /* BUG-35783 Add to handle SIGINT while calling recv() */
            case ACP_RC_EINTR:
                break;

            default:
                ACI_RAISE(SendError);
                break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Retry);
    {
        ACI_SET(aciSetErrorCode(cmERR_RETRY_SOCKET_OPERATION_WOULD_BLOCK));
    }
    /*
     * BUGBUG
     *
    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    */
    ACI_EXCEPTION(TimedOut);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    ACI_EXCEPTION(SendError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SEND_ERROR));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmnSockGetSndBufSize(acp_sock_t *aSock, acp_sint32_t *aSndBufSize)
{
    acp_sint32_t   sOptVal = 0;
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_rc_t       sRet;
    acp_char_t     sErrMsg[256];

    sRet = acpSockGetOpt(aSock,
                         SOL_SOCKET,
                         SO_SNDBUF,
                         &sOptVal,
                         &sOptLen);

    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), GetSockOptError);

    *aSndBufSize = sOptVal;

    return ACI_SUCCESS;

    ACI_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg),
                                "SO_SNDBUF", ACP_RC_TO_SYS_ERROR(sRet));

        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockSetSndBufSize(acp_sock_t *aSock, acp_sint32_t aSndBufSize)
{
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_rc_t       sRet;
    acp_char_t     sErrMsg[256];

    sRet = acpSockSetOpt(aSock,
                         SOL_SOCKET,
                         SO_SNDBUF,
                         &aSndBufSize,
                         sOptLen);

    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SetSockOptError);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg),
                                "SO_SNDBUF", ACP_RC_TO_SYS_ERROR(sRet));

        ACI_SET(aciSetErrorCode(cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockGetRcvBufSize(acp_sock_t *aSock, acp_sint32_t *aRcvBufSize)
{
    acp_sint32_t   sOptVal = 0;
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_rc_t       sRet;
    acp_char_t     sErrMsg[256];

    sRet = acpSockGetOpt(aSock,
                         SOL_SOCKET,
                         SO_RCVBUF,
                         &sOptVal,
                         &sOptLen);

    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), GetSockOptError);

    *aRcvBufSize = sOptVal;

    return ACI_SUCCESS;

    ACI_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg),
                                "SO_RCVBUF", ACP_RC_TO_SYS_ERROR(sRet));

        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnSockSetRcvBufSize(acp_sock_t *aSock, acp_sint32_t aRcvBufSize)
{
    acp_sock_len_t sOptLen = (acp_sock_len_t)ACI_SIZEOF(acp_sint32_t);
    acp_rc_t       sRet;
    acp_char_t     sErrMsg[256];

    sRet = acpSockSetOpt(aSock,
                         SOL_SOCKET,
                         SO_RCVBUF,
                         &aRcvBufSize,
                         sOptLen);

    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SetSockOptError);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg),
                                "SO_RCVBUF", ACP_RC_TO_SYS_ERROR(sRet));

        ACI_SET(aciSetErrorCode(cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

#endif
