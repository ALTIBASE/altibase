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

#include <cmAll.h>

extern "C" cmnIB gIB;

#if !defined(MSG_DONTWAIT) && !defined(MSG_NONBLOCK)
static IDE_RC cmnSockCheckIOCTLIB(PDL_SOCKET aHandle, idBool *aIsClosed)
{
    struct pollfd  sPollFd;
    acp_sint32_t   sTimeout = 0;
    SInt           sRet;

    IDE_TEST_RAISE(aHandle == PDL_INVALID_SOCKET, InvalidHandle);

    sPollFd.fd     = aHandle;
    sPollFd.events = POLLIN;

    /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
    ACI_EXCEPTION_CONT(Restart);

    sPollFd.revents = 0;

    sRet = gIB.mFuncs.rpoll(&sPollFd, 1, sTimeout);

    if (sRet < 0)
    {
        ACI_TEST_RAISE(errno == EINTR, Restart);

        IDE_RAISE(PollError);
    }
    else
    {
        /* timed out, if sRet == 0 */
        *aIsClosed = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidHandle);
    {
        errno = EBADF;
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    IDE_EXCEPTION(PollError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

#if defined(MSG_PEEK)
static IDE_RC cmnSockCheckRECVIB( PDL_SOCKET  aHandle,
                                  SInt        aFlag,
                                  idBool     *aIsClosed )
{
    SChar   sBuf[1];
    ssize_t sRet;

    sRet = gIB.mFuncs.rrecv(aHandle, sBuf, 1, aFlag);

    switch ((acp_sint32_t)sRet)
    {
        case 1:
            *aIsClosed = ID_FALSE;
            break;

        case 0:
            *aIsClosed = ID_TRUE;
            break;

        default:
            switch (errno)
            {
#if (EWOULDBLOCK != EAGAIN)
                case EAGAIN:
#endif
                case EWOULDBLOCK:
                    *aIsClosed = ID_FALSE;
                    break;
                case ECONNRESET:
                case ECONNREFUSED:
                    *aIsClosed = ID_TRUE;
                    break;
                default:
                    IDE_RAISE(RecvError);
                    break;
            }
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(RecvError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RRECV_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

IDE_RC cmnSockCheckIB( cmnLink    *aLink,
                       PDL_SOCKET  aHandle,
                       idBool     *aIsClosed )
{
#if defined(MSG_PEEK) && defined(MSG_DONTWAIT)
    ACP_UNUSED(aLink);
    return cmnSockCheckRECVIB(aHandle, MSG_PEEK | MSG_DONTWAIT, aIsClosed);
#elif defined(MSG_PEEK) && defined(MSG_NONBLOCK)
    ACP_UNUSED(aLink);
    return cmnSockCheckRECVIB(aHandle, MSG_PEEK | MSG_NONBLOCK, aIsClosed);
#elif defined(MSG_PEEK)
    idBool sIsNonBlock;

    sIsNonBlock = (aLink->mFeature & CMN_LINK_FLAG_NONBLOCK) ? ID_TRUE : ID_FALSE;

    if (sIsNonBlock == ID_TRUE)
    {
        return cmnSockCheckRECVIB(aHandle, MSG_PEEK, aIsClosed);
    }
    else
    {
        return cmnSockCheckIOCTLIB(aHandle, aIsClosed);
    }
#else
    ACP_UNUSED(aLink);
    return cmnSockCheckIOCTLIB(aHandle, aIsClosed);
#endif
}

IDE_RC cmnSockSetBlockingModeIB(PDL_SOCKET aHandle, idBool aBlockingMode)
{
    SInt sFlags = gIB.mFuncs.rfcntl(aHandle, F_GETFL, 0);

    if (aBlockingMode == ID_TRUE)
    {
        IDE_TEST_RAISE(sFlags == -1, SetBlockingFail);

        IDE_TEST_RAISE(gIB.mFuncs.rfcntl(aHandle, F_SETFL, sFlags & ~O_NONBLOCK) == -1, SetBlockingFail);
    }
    else
    {
        IDE_TEST_RAISE(sFlags == -1, SetNonBlockingFail);

        IDE_TEST_RAISE(gIB.mFuncs.rfcntl(aHandle, F_SETFL, sFlags | O_NONBLOCK) == -1, SetNonBlockingFail);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SetBlockingFail);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSOCKET_SET_BLOCKING_FAILED));
    }
    IDE_EXCEPTION(SetNonBlockingFail);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSOCKET_SET_NONBLOCKING_FAILED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockRecvIB(cmbBlock       *aBlock,
                     cmnLinkPeer    *aLink,
                     PDL_SOCKET      aHandle,
                     UShort          aSize,
                     PDL_Time_Value *aTimeout,
                     idvStatIndex    aStatIndex)
{
    ssize_t sRet;

    /*
     * aSize 이상 aBlock으로 데이터 읽음
     */
    while (aBlock->mDataSize < aSize)
    {
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != NULL)
        {
            IDE_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                           CMN_DIRECTION_RD,
                                           aTimeout) != IDE_SUCCESS);
        }

        /*
         * Socket으로부터 읽음
         */
        sRet = gIB.mFuncs.rread(aHandle,
                                aBlock->mData + aBlock->mDataSize,
                                aBlock->mBlockSize - aBlock->mDataSize);

        IDE_TEST_RAISE(sRet == 0, ConnectionClosed);

        // To Fix BUG-21008
        IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_IB_COUNT, 1);
        
        if (sRet < 0)
        {
            switch (errno)
            {
#if (EWOULDBLOCK != EAGAIN)
                case EAGAIN:
#endif
                case EWOULDBLOCK:
                    IDE_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                                   CMN_DIRECTION_RD,
                                                   aTimeout) != IDE_SUCCESS);
                    break;

                case ECONNRESET:
                case ECONNREFUSED:
                    IDE_RAISE(ConnectionClosed);
                    break;

                default:
                    IDE_RAISE(RecvError);
                    break;
            }
        }
        else
        {
            aBlock->mDataSize += sRet;

            IDV_SESS_ADD(aLink->mStatistics, aStatIndex, (ULong)sRet);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConnectionClosed);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(RecvError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RRECV_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockSendIB(cmbBlock       *aBlock,
                     cmnLinkPeer    *aLink,
                     PDL_SOCKET      aHandle,
                     PDL_Time_Value *aTimeout,
                     idvStatIndex    aStatIndex)
{
    ssize_t sSize;

    if (aBlock->mCursor == aBlock->mDataSize)
    {
        aBlock->mCursor = 0;
    }

    while (aBlock->mCursor < aBlock->mDataSize)
    {
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != NULL)
        {
            IDE_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                           CMN_DIRECTION_WR,
                                           aTimeout) != IDE_SUCCESS);
        }

        /*
         * socket으로 데이터 씀
         */
        sSize = gIB.mFuncs.rwrite(aHandle,
                                  aBlock->mData + aBlock->mCursor,
                                  aBlock->mDataSize - aBlock->mCursor);

        // To Fix BUG-21008
        IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_IB_COUNT, 1);
        
        if (sSize < 0)
        {
            switch (errno)
            {
#if (EWOULDBLOCK != EAGAIN)
                case EAGAIN:
#endif
                case EWOULDBLOCK:
                    IDE_RAISE(Retry);
                    break;

                case EPIPE:
                    IDE_RAISE(ConnectionClosed);
                    break;

                case ETIME:
                    IDE_RAISE(TimedOut);
                    break;

                default:
                    IDE_RAISE(SendError);
                    break;
            }
        }
        else
        {
            aBlock->mCursor += sSize;

            IDV_SESS_ADD(aLink->mStatistics, aStatIndex, (ULong)sSize);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Retry);
    {
        IDE_SET(ideSetErrorCode(cmERR_RETRY_IB_RSOCKET_OPERATION_WOULD_BLOCK));
    }
    IDE_EXCEPTION(ConnectionClosed);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(TimedOut);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION(SendError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSEND_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmnSockGetSndBufSizeIB(PDL_SOCKET aHandle, SInt *aSndBufSize)
{
    SInt  sOptVal;
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(gIB.mFuncs.rgetsockopt(aHandle,
                                          SOL_SOCKET,
                                          SO_SNDBUF,
                                          &sOptVal,
                                          (socklen_t *)&sOptLen) < 0, GetSockOptError);

    *aSndBufSize = sOptVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_SNDBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RGETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockSetSndBufSizeIB(PDL_SOCKET aHandle, SInt aSndBufSize)
{
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(gIB.mFuncs.rsetsockopt(aHandle,
                                          SOL_SOCKET,
                                          SO_SNDBUF,
                                          &aSndBufSize,
                                          (socklen_t)sOptLen) < 0, SetSockOptError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_SNDBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockGetRcvBufSizeIB(PDL_SOCKET aHandle, SInt *aRcvBufSize)
{
    SInt  sOptVal;
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(gIB.mFuncs.rgetsockopt(aHandle,
                                          SOL_SOCKET,
                                          SO_RCVBUF,
                                          &sOptVal,
                                          (socklen_t *)&sOptLen) < 0, GetSockOptError);

    *aRcvBufSize = sOptVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_RCVBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RGETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockSetRcvBufSizeIB(PDL_SOCKET aHandle, SInt aRcvBufSize)
{
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(gIB.mFuncs.rsetsockopt(aHandle,
                                          SOL_SOCKET,
                                          SO_RCVBUF,
                                          &aRcvBufSize,
                                          (socklen_t )sOptLen) < 0, SetSockOptError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_RCVBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

