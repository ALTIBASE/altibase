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

#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)


#if !defined(MSG_DONTWAIT) && !defined(MSG_NONBLOCK)
static IDE_RC cmnSockCheckIOCTL(PDL_SOCKET aHandle, idBool *aIsClosed)
{
    PDL_Time_Value sTimeout;
    SInt           sCount;
    SInt           sSize = 0;

    IDE_TEST_RAISE(aHandle == PDL_INVALID_SOCKET, InvalidHandle);

    sTimeout.set(0);

    sCount = cmnDispatcherCheckHandle(aHandle, &sTimeout);  /* BUG-45240 */

    switch (sCount)
    {
        case 1:
            if (idlOS::ioctl(aHandle, FIONREAD, &sSize) < 0)
            {
                *aIsClosed = ID_TRUE;
            }
            else
            {
                *aIsClosed = (sSize == 0) ? ID_TRUE : ID_FALSE;
            }

            break;

        case 0:
            *aIsClosed = ID_FALSE;
            break;

        default:
            IDE_RAISE(SelectError);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidHandle);
    {
        errno = EBADF;
        /* BUG-47714 에러 메세지에 sock number 추가 */
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SELECT_ERROR, aHandle));
    }
    IDE_EXCEPTION(SelectError);
    {
        /* BUG-47714 에러 메세지에 sock number 추가 */
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SELECT_ERROR, aHandle));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

#if defined(MSG_PEEK)
static IDE_RC cmnSockCheckRECV(PDL_SOCKET aHandle, SInt aFlag, idBool *aIsClosed)
{
    SChar   sBuff[1];
    ssize_t sSize;

    sSize = idlOS::recv(aHandle, sBuff, 1, aFlag);

    switch (sSize)
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
        IDE_SET(ideSetErrorCode(cmERR_ABORT_RECV_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

IDE_RC cmnSockCheck(cmnLink *aLink, PDL_SOCKET aHandle, idBool *aIsClosed)
{
#if defined(MSG_PEEK) && defined(MSG_DONTWAIT)
    aLink = aLink ; // To Fix Compiler Warning
    return cmnSockCheckRECV(aHandle, MSG_PEEK | MSG_DONTWAIT, aIsClosed);
#elif defined(MSG_PEEK) && defined(MSG_NONBLOCK)
    aLink = aLink ; // To Fix Compiler Warning
    return cmnSockCheckRECV(aHandle, MSG_PEEK | MSG_NONBLOCK, aIsClosed);
#elif defined(MSG_PEEK)
    idBool sIsNonBlock;

    sIsNonBlock = (aLink->mFeature & CMN_LINK_FLAG_NONBLOCK) ? ID_TRUE : ID_FALSE;

    if (sIsNonBlock == ID_TRUE)
    {
        return cmnSockCheckRECV(aHandle, MSG_PEEK, aIsClosed);
    }
    else
    {
        return cmnSockCheckIOCTL(aHandle, aIsClosed);
    }
#else
    aLink = aLink ; // To Fix Compiler Warning
    return cmnSockCheckIOCTL(aHandle, aIsClosed);
#endif
}

IDE_RC cmnSockSetBlockingMode(PDL_SOCKET aHandle, idBool aBlockingMode)
{
    if (aBlockingMode == ID_TRUE)
    {
        IDE_TEST_RAISE(idlVA::setBlock(aHandle) != 0, SetBlockingFail);
    }
    else
    {
        IDE_TEST_RAISE(idlVA::setNonBlock(aHandle) != 0, SetNonBlockingFail);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SetBlockingFail);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_SET_BLOCKING_FAILED));
    }
    IDE_EXCEPTION(SetNonBlockingFail);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_SET_NONBLOCKING_FAILED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockRecv(cmbBlock       *aBlock,
                   cmnLinkPeer    *aLink,
                   PDL_SOCKET      aHandle,
                   UShort          aSize,
                   PDL_Time_Value *aTimeout,
                   idvStatIndex    aStatIndex)
{
    ssize_t sSize;

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
        sSize = idlVA::recv_i(aHandle,
                              aBlock->mData + aBlock->mDataSize,
                              aBlock->mBlockSize - aBlock->mDataSize);

        IDE_TEST_RAISE(sSize == 0, ConnectionClosed);

        // To Fix BUG-21008
        IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_SOCKET_COUNT, 1 );
        
        if (sSize < 0)
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
            aBlock->mDataSize += sSize;

            IDV_SESS_ADD(aLink->mStatistics, aStatIndex, (ULong)sSize);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConnectionClosed);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(RecvError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_RECV_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockSend(cmbBlock       *aBlock,
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
        sSize = idlVA::send_i(aHandle,
                              aBlock->mData + aBlock->mCursor,
                              aBlock->mDataSize - aBlock->mCursor);

        // To Fix BUG-21008
        IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_SOCKET_COUNT, 1 );
        
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
        IDE_SET(ideSetErrorCode(cmERR_RETRY_SOCKET_OPERATION_WOULD_BLOCK));
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
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SEND_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmnSockGetSndBufSize(PDL_SOCKET aHandle, SInt *aSndBufSize)
{
    SInt  sOptVal;
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(idlOS::getsockopt(aHandle,
                                     SOL_SOCKET,
                                     SO_SNDBUF,
                                     (SChar *)&sOptVal,
                                     &sOptLen) < 0, GetSockOptError);

    *aSndBufSize = sOptVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_SNDBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockSetSndBufSize(PDL_SOCKET aHandle, SInt aSndBufSize)
{
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(idlOS::setsockopt(aHandle,
                                     SOL_SOCKET,
                                     SO_SNDBUF,
                                     (const SChar *)&aSndBufSize,
                                     sOptLen) < 0, SetSockOptError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_SNDBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockGetRcvBufSize(PDL_SOCKET aHandle, SInt *aRcvBufSize)
{
    SInt  sOptVal;
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(idlOS::getsockopt(aHandle,
                                     SOL_SOCKET,
                                     SO_RCVBUF,
                                     (SChar *)&sOptVal,
                                     &sOptLen) < 0, GetSockOptError);

    *aRcvBufSize = sOptVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_RCVBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnSockSetRcvBufSize(PDL_SOCKET aHandle, SInt aRcvBufSize)
{
    SInt  sOptLen = ID_SIZEOF(SInt);
    SChar sErrMsg[256];

    IDE_TEST_RAISE(idlOS::setsockopt(aHandle,
                                     SOL_SOCKET,
                                     SO_RCVBUF,
                                     (const SChar *)&aRcvBufSize,
                                     sOptLen) < 0, SetSockOptError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "SO_RCVBUF", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif
