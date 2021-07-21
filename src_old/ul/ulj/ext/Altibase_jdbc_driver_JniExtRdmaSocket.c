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

#include <acp.h>
#include <Altibase_jdbc_driver_JniExtRdmaSocket.h>

#include <cmErrorCodeClient.h>
#include <cmnIB.h>

extern cmnIB gIB;

extern const acp_uint32_t aciVersionID;

const acp_char_t *gCmErrorFactory[] =
{
#include "../../../cm/cmi/E_CM_US7ASCII.c"
};

static void throwException( JNIEnv           *aEnv,
                            const acp_char_t *aExceptionName,
                            const acp_char_t *aExceptionMessage )
{
    jclass sJavaExceptionClass;

    sJavaExceptionClass = (*aEnv)->FindClass(aEnv, aExceptionName);
    if (sJavaExceptionClass == NULL)
    {
        sJavaExceptionClass = (*aEnv)->FindClass(aEnv, "java/lang/NoClassDefFoundError");
        ACE_ASSERT(sJavaExceptionClass != NULL);
    }
    else
    {
        (*aEnv)->ThrowNew(aEnv, sJavaExceptionClass, aExceptionMessage);
    }
}

static void throwExceptionFromErrMgr(JNIEnv *aEnv)
{
    acp_char_t sExceptionMessage[256] = {'\0', };

    switch (aciGetErrorCode())
    {
        case cmERR_ABORT_TIMED_OUT:
            throwException(aEnv, "java/net/SocketTimeoutException", NULL);
            break;

        default:
            (void)acpSnprintf(sExceptionMessage,
                              ACI_SIZEOF(sExceptionMessage),
                              "ERR-%05"ACI_XINT32_FMT" : %s, errno = %"ACI_INT32_FMT,
                              ACI_E_ERROR_CODE(aciGetErrorCode()),
                              aciGetErrorMsg(aciGetErrorCode()),
                              aciGetSystemErrno());

            throwException(aEnv, "java/io/IOException", sExceptionMessage);
            break;
    }
}

JNIEXPORT void JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_load(
        JNIEnv *aEnv,
        jclass  aClass)
{
    ACP_UNUSED(aClass);

    (void)aciRegistErrorFromBuffer(ACI_E_MODULE_CM, 
            aciVersionID, 
            ACI_SIZEOF(gCmErrorFactory) / ACI_SIZEOF(gCmErrorFactory[0]), /* count */
            (acp_char_t **)&gCmErrorFactory);

    ACI_TEST(cmnIBInitialize() != ACI_SUCCESS);

    return;

    ACI_EXCEPTION_END;

    throwExceptionFromErrMgr(aEnv);

    return;
}

JNIEXPORT void JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_unload(
        JNIEnv *aEnv,
        jclass  aClass)
{
    ACP_UNUSED(aEnv);
    ACP_UNUSED(aClass);

    (void)cmnIBFinalize();
}

JNIEXPORT jint JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rsocket(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aDomain)
{
    acp_sint32_t sSocket;

    ACP_UNUSED(aClass);

    sSocket = gIB.mFuncs.rsocket(aDomain, SOCK_STREAM, 0);

    ACI_TEST_RAISE(sSocket == -1, SocketError);

    return sSocket;

    ACI_EXCEPTION(SocketError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSOCKET_OPEN_ERROR, errno));
    }
    ACI_EXCEPTION_END;

    throwExceptionFromErrMgr(aEnv);

    return -1;
}

JNIEXPORT void JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rbind(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aSocket,
        jstring  aAddr)
{
    const acp_char_t     *sAddr;
    acp_inet_addr_info_t *sAddrInfo = NULL;
    acp_char_t           *sErrStr = NULL;
    acp_char_t            sErrMsg[256];
    acp_rc_t              sRC;

    ACP_UNUSED(aClass);

    sAddr = (const acp_char_t *)((*aEnv)->GetStringUTFChars(aEnv, aAddr, NULL));

    sRC = acpInetGetAddrInfo(&sAddrInfo,
                             (acp_char_t *)sAddr,
                             NULL,
                             ACP_SOCK_STREAM,
                             ACP_INET_AI_NUMERICHOST,
                             ACP_AF_UNSPEC) != ACP_RC_SUCCESS;

    if (ACP_RC_NOT_SUCCESS(sRC) || (sAddr == NULL))
    {
        sErrMsg[0] = '\0';

        (void)acpInetGetStrError((acp_sint32_t)sRC, &sErrStr);
        if (sErrStr == NULL)
        {
            (void)acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%"ACI_INT32_FMT, sRC);
        }
        else
        {
            (void)acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%s", sErrStr);
        }

        ACI_RAISE(GetAddrInfoError);
    }
    else
    {
        /* nothing to do */
    }

    ACI_TEST_RAISE(gIB.mFuncs.rbind(aSocket,
                                     (acp_sock_addr_t *)sAddrInfo->ai_addr,
                                     sAddrInfo->ai_addrlen) < 0, BindError);

    (void)acpInetFreeAddrInfo(sAddrInfo);

    (*aEnv)->ReleaseStringUTFChars(aEnv, aAddr, sAddr);

    return;

    ACI_EXCEPTION(GetAddrInfoError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    ACI_EXCEPTION(BindError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RBIND_ERROR, errno));
    }
    ACI_EXCEPTION_END;

    if (sAddrInfo != NULL)
    {
        acpInetFreeAddrInfo(sAddrInfo);
    }
    else
    {
        /* nothing to do */
    }

    (*aEnv)->ReleaseStringUTFChars(aEnv, aAddr, sAddr);

    throwExceptionFromErrMgr(aEnv);

    return;
}

static acp_sint32_t connectTimedWait(acp_sint32_t     aSocket,
                                     acp_sock_addr_t *aAddr,
                                     acp_sint32_t     aAddrLen,
                                     acp_sint32_t     aTimeout);

JNIEXPORT void JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rconnect(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aSocket,
        jstring  aAddr,
        jint     aPort,
        jint     aTimeout)
{
    const acp_char_t     *sAddr;
    acp_inet_addr_info_t *sAddrInfo = NULL;
    acp_char_t            sPort[10];
    acp_char_t           *sErrStr = NULL;
    acp_char_t            sErrMsg[256];
    acp_rc_t              sRC;

    ACP_UNUSED(aClass);

    sAddr = (const acp_char_t *)((*aEnv)->GetStringUTFChars(aEnv, aAddr, NULL));

    (void)acpSnprintf(sPort, ACI_SIZEOF(sPort), "%"ACI_INT32_FMT, aPort);

    sRC = acpInetGetAddrInfo(&sAddrInfo,
                             (acp_char_t *)sAddr,
                             sPort,
                             ACP_SOCK_STREAM,
                             ACP_INET_AI_NUMERICHOST | ACP_INET_AI_NUMERICSERV,
                             ACP_AF_UNSPEC) != ACP_RC_SUCCESS;

    if (ACP_RC_NOT_SUCCESS(sRC) || (sAddr == NULL))
    {
        sErrMsg[0] = '\0';

        (void)acpInetGetStrError((acp_sint32_t)sRC, &sErrStr);
        if (sErrStr == NULL)
        {
            (void)acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%"ACI_INT32_FMT, sRC);
        }
        else
        {
            (void)acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%s", sErrStr);
        }

        ACI_RAISE(GetAddrInfoError);
    }
    else
    {
        /* nothing to do */
    }

    ACI_TEST_RAISE(connectTimedWait(aSocket,
                                    (acp_sock_addr_t *)sAddrInfo->ai_addr,
                                    sAddrInfo->ai_addrlen,
                                    aTimeout) < 0, ConnectError);

    (void)acpInetFreeAddrInfo(sAddrInfo);

    (*aEnv)->ReleaseStringUTFChars(aEnv, aAddr, sAddr);

    return;

    ACI_EXCEPTION(GetAddrInfoError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    ACI_EXCEPTION(ConnectError)
    {
        /* bug-30835: support link-local address with zone index.
         * On linux, link-local addr without zone index makes EINVAL.
         *  .EINVAL   : Invalid argument to connect()
         *  .others   : Client unable to establish connection.
         * caution: acpSockTimedConnect does not change errno.
         *  Instead, it returns errno directly. */
        if (errno == EINVAL)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RCONNECT_INVALIDARG));
        }
        else
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RCONNECT_ERROR, errno));
        }
    }
    ACI_EXCEPTION_END;

    if (sAddrInfo != NULL)
    {
        acpInetFreeAddrInfo(sAddrInfo);
    }
    else
    {
        /* nothing to do */
    }

    (*aEnv)->ReleaseStringUTFChars(aEnv, aAddr, sAddr);

    throwExceptionFromErrMgr(aEnv);

    return;
}

static ACI_RC setBlockingMode(acp_sint32_t aSocket, acp_bool_t aBlockingMode)
{
    acp_sint32_t sFlags = gIB.mFuncs.rfcntl(aSocket, F_GETFL, 0);

    if (aBlockingMode == ACP_TRUE)
    {
        ACI_TEST_RAISE(sFlags == -1, SetBlockingFail);

        ACI_TEST_RAISE(gIB.mFuncs.rfcntl(aSocket, F_SETFL, sFlags & ~O_NONBLOCK) == -1, SetBlockingFail);
    }
    else
    {
        ACI_TEST_RAISE(sFlags == -1, SetNonBlockingFail);

        ACI_TEST_RAISE(gIB.mFuncs.rfcntl(aSocket, F_SETFL, sFlags | O_NONBLOCK) == -1, SetNonBlockingFail);
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

static acp_sint32_t connectTimedWait(acp_sint32_t     aSocket,
                                     acp_sock_addr_t *aAddr,
                                     acp_sint32_t     aAddrLen,
                                     acp_sint32_t     aTimeout)
{
    acp_sint32_t    sRet;
    acp_sint32_t    sError = 0;
    acp_sint32_t    sLen;
    struct pollfd   sPollFd;
    acp_sint32_t    sTimeout = (aTimeout != 0) ? aTimeout : -1;
    acp_bool_t      sChangedBlockingMode = ACP_FALSE;

    /* change non-blocking mode only when connecting */
    ACI_TEST(setBlockingMode(aSocket, ACP_FALSE) != ACI_SUCCESS);

    sChangedBlockingMode = ACP_TRUE;

    sRet = gIB.mFuncs.rconnect(aSocket, aAddr, aAddrLen);
    if (sRet != 0)
    {
        /* error, if the connection isn't now in progress */
        ACI_TEST((sRet < 0) && (errno != EINPROGRESS) && (errno != EWOULDBLOCK)); 

        /**
         * [CAUTION]
         *
         * Berkeley socket 과 다르게 blocking mode 이더라도
         * rconect() 함수는 'connecting' 상태로 반환될 수 있음.
         */

        /* wait until connecting */
        sPollFd.fd      = aSocket;
        sPollFd.events  = POLLIN | POLLOUT;
        sPollFd.revents = 0;

        sRet = gIB.mFuncs.rpoll(&sPollFd, 1, sTimeout);
        ACI_TEST(sRet < 0); /* error */

        ACI_TEST_RAISE(sRet == 0, TimedOut); /* timed out */

        /* error, if it was not occured either read or write event */
        ACI_TEST(sPollFd.revents == 0);

        /* check socket error */
        sLen = (acp_sint32_t)sizeof(sError);

        ACI_TEST(gIB.mFuncs.rgetsockopt(aSocket,
                                        SOL_SOCKET,
                                        SO_ERROR,
                                        &sError,
                                        (acp_sock_len_t *)&sLen) < 0);

        ACI_TEST(sError != 0);
    }
    else
    {
        /* connected successfully */
    }

    (void)setBlockingMode(aSocket, ACP_TRUE);

    return 0;

    ACI_EXCEPTION(TimedOut)
    {
        sError = ETIMEDOUT;
    }
    ACI_EXCEPTION_END;

    if (sChangedBlockingMode == ACP_TRUE)
    {
        (void)setBlockingMode(aSocket, ACP_TRUE);
    }
    else
    {
        /* nothing to do */
    }

    if (sError != 0)
    {
        errno = sError;
    }
    else
    {
        /* nothing to do */
    }

    return -1;
}

JNIEXPORT void JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rclose(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aSocket)
{
    ACP_UNUSED(aEnv);
    ACP_UNUSED(aClass);

    (void)gIB.mFuncs.rclose(aSocket);
}

static ACI_RC waitSocketEvents(acp_sint32_t aSocket,
                               acp_sint32_t aEvents,
                               acp_sint32_t aTimeout);

JNIEXPORT jint JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rrecv(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aSocket,
        jobject  aBuffer,
        jint     aOffset,
        jint     aLength,
        jint     aTimeout)
{
    acp_char_t   *sBuffer;
    acp_sint32_t  sRet;

    ACP_UNUSED(aClass);

    sBuffer = (acp_char_t *)((*aEnv)->GetDirectBufferAddress(aEnv, aBuffer));

    ACI_EXCEPTION_CONT(Retry);

    /* 0 means INFINITE */
    if (aTimeout != 0)
    {
        ACI_TEST(waitSocketEvents(aSocket, POLLIN, aTimeout) != ACI_SUCCESS);
    }

    sRet = gIB.mFuncs.rrecv(aSocket,
                            sBuffer + aOffset,
                            aLength,
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
                ACI_RAISE(Retry);
                break;

            case ECONNRESET:
            case ECONNREFUSED:
                ACI_RAISE(ConnectionClosed);
                break;

            /* BUG-35783 Add to handle SIGINT while calling recv() */
            case EINTR:
                ACI_RAISE(Retry);
                break;

            default:
                ACI_RAISE(RecvError);
                break;
        }
    }

    return sRet;

    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(RecvError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RRECV_ERROR));
    }
    ACI_EXCEPTION_END;

    throwExceptionFromErrMgr(aEnv);

    return -1;
}

JNIEXPORT jint JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rsend(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aSocket,
        jobject  aBuffer,
        jint     aOffset,
        jint     aLength,
        jint     aTimeout)
{
    acp_char_t   *sBuffer;
    acp_sint32_t  sRet;

    ACP_UNUSED(aClass);

    sBuffer = (acp_char_t *)((*aEnv)->GetDirectBufferAddress(aEnv, aBuffer));

    ACI_EXCEPTION_CONT(Retry);

    /* 0 means INFINITE */
    if (aTimeout != 0)
    {
        ACI_TEST(waitSocketEvents(aSocket, POLLOUT, aTimeout) != ACI_SUCCESS);
    }

    sRet = gIB.mFuncs.rsend(aSocket,
                            sBuffer + aOffset,
                            aLength,
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
                ACI_RAISE(Retry);
                break;

            case ECONNRESET:
            case EPIPE:
                ACI_RAISE(ConnectionClosed);
                break;

            case ETIMEDOUT:
                ACI_RAISE(TimedOut);
                break;

            /* BUG-35783 Add to handle SIGINT while calling recv() */
            case EINTR:
                ACI_RAISE(Retry);
                break;

            default:
                ACI_RAISE(SendError);
                break;
        }
    }

    return sRet;

    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(SendError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSEND_ERROR));
    }
    ACI_EXCEPTION(TimedOut);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    ACI_EXCEPTION_END;

    throwExceptionFromErrMgr(aEnv);

    return -1;
}

static ACI_RC waitSocketEvents(acp_sint32_t aSocket,
                               acp_sint32_t aEvents,
                               acp_sint32_t aTimeout)
{
    struct pollfd sPollFd;
    acp_sint32_t  sRet;

    sPollFd.fd     = aSocket;
    sPollFd.events = aEvents;

    /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
    ACI_EXCEPTION_CONT(Restart);

    sPollFd.revents = 0;

    sRet = gIB.mFuncs.rpoll(&sPollFd, 1, aTimeout);

    ACI_TEST_RAISE(sRet == 0, TimedOut);

    if (sRet < 0)
    {
        /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
        ACI_TEST_RAISE(errno == EINTR, Restart);

        ACI_RAISE(PollError);
    }
    else
    {
        /* some events was occurred */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(PollError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    ACI_EXCEPTION(TimedOut);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

JNIEXPORT void JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rsetsockopt(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aSocket,
        jint     aLevel,
        jint     aOptName,
        jint     aValue)
{
    acp_char_t sErrMsg[256];

    ACP_UNUSED(aClass);

    ACI_TEST_RAISE(gIB.mFuncs.rsetsockopt(aSocket,
                                          aLevel,
                                          aOptName,
                                          &aValue,
                                          sizeof(aValue)) == -1, SetSockOptError);

    return;

    ACI_EXCEPTION(SetSockOptError)
    {
        sErrMsg[0] = '\0';

        (void)acpSnprintf(sErrMsg,
                          ACI_SIZEOF(sErrMsg),
                          "optname = %"ACI_INT32_FMT", errno=%"ACI_INT32_FMT,
                          aOptName,
                          errno);

        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    throwExceptionFromErrMgr(aEnv);

    return;
}

JNIEXPORT void JNICALL Java_Altibase_jdbc_driver_JniExtRdmaSocket_rgetsockopt(
        JNIEnv  *aEnv,
        jclass   aClass,
        jint     aSocket,
        jint     aLevel,
        jint     aOptName,
        jint     aValue)
{
    acp_char_t     sErrMsg[256];
    acp_sock_len_t sOptLen = sizeof(aValue);

    ACP_UNUSED(aClass);

    ACI_TEST_RAISE(gIB.mFuncs.rgetsockopt(aSocket,
                                          aLevel,
                                          aOptName,
                                          &aValue,
                                          &sOptLen) == -1, GetSockOptError);

    return;

    ACI_EXCEPTION(GetSockOptError)
    {
        sErrMsg[0] = '\0';

        (void)acpSnprintf(sErrMsg,
                          ACI_SIZEOF(sErrMsg),
                          "optname = %"ACI_INT32_FMT", errno=%"ACI_INT32_FMT,
                          aOptName,
                          errno);

        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RGETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    throwExceptionFromErrMgr(aEnv);

    return;
}

