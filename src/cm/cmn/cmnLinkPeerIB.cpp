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
#include <cmnIB.h>

typedef struct cmnLinkPeerIB
{
    cmnLinkPeer     mLinkPeer;

    cmnLinkDescIB   mDesc;

    UInt            mDispatchInfo;
    cmbBlock       *mPendingBlock;
} cmnLinkPeerIB;

extern "C" cmnIB gIB;

IDE_RC cmnLinkPeerInitializeIB(cmnLink *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    sLink->mDesc.mHandle  = PDL_INVALID_SOCKET;
    sLink->mDesc.mLatency = 0;
    sLink->mDesc.mConChkSpin = 0;

    sLink->mDispatchInfo  = 0;
    sLink->mPendingBlock  = NULL;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFinalizeIB(cmnLink *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;
    cmbPool       *sPool = sLink->mLinkPeer.mPool;

    /* socket이 열려있으면 닫음 */
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    /* Pending Block이 할당되어 있으면 해제 */
    if (sLink->mPendingBlock != NULL)
    {
        IDE_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCloseIB(cmnLink *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        gIB.mFuncs.rclose(sLink->mDesc.mHandle);

        sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetHandleIB(cmnLink *aLink, void *aHandle)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    *(PDL_SOCKET *)aHandle = sLink->mDesc.mHandle;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    *(UInt *)aDispatchInfo = sLink->mDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    sLink->mDispatchInfo = *(UInt *)aDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetBlockingModeIB(cmnLinkPeer *aLink, idBool aBlockingMode)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    IDE_TEST(cmnSockSetBlockingModeIB(sLink->mDesc.mHandle, aBlockingMode));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log(IDE_SERVER_2, CM_TRC_IB_RSOCKET_SET_BLOCKING_MODE_FAIL, errno);

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetInfoIB( cmnLinkPeer    *aLink,
                             SChar          *aBuf,
                             UInt            aBufLen,
                             cmnLinkInfoKey  aKey )
{
    cmnLinkPeerIB           *sLink = (cmnLinkPeerIB *)aLink;
    struct sockaddr_storage  sAddr;
    SInt                     sAddrLen;
    SInt                     sRet = 0;

    SChar                    sAddrStr[IDL_IP_ADDR_MAX_LEN];
    SChar                    sPortStr[IDL_IP_PORT_MAX_LEN];

    /* proj-1538 ipv6 */
    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_REMOTE_ADDRESS:
        case CMN_LINK_INFO_REMOTE_IP_ADDRESS:
        case CMN_LINK_INFO_REMOTE_PORT:

            IDE_TEST_RAISE(
                idlOS::getnameinfo((struct sockaddr *)&sLink->mDesc.mAddr,
                                   sLink->mDesc.mAddrLen,
                                   sAddrStr, IDL_IP_ADDR_MAX_LEN,
                                   sPortStr, IDL_IP_PORT_MAX_LEN,
                                   NI_NUMERICHOST | NI_NUMERICSERV) != 0,
                GetNameInfoError);
            break;

        case CMN_LINK_INFO_LOCAL_ADDRESS:
        case CMN_LINK_INFO_LOCAL_IP_ADDRESS:
        case CMN_LINK_INFO_LOCAL_PORT:

            sAddrLen = ID_SIZEOF(sAddr);
            IDE_TEST_RAISE(gIB.mFuncs.rgetsockname(sLink->mDesc.mHandle,
                                                   (struct sockaddr *)&sAddr,
                                                   (socklen_t *)&sAddrLen) != 0,
                           GetSockNameError);

            IDE_TEST_RAISE(
                idlOS::getnameinfo((struct sockaddr *)&sAddr,
                                   sAddrLen,
                                   sAddrStr, IDL_IP_ADDR_MAX_LEN,
                                   sPortStr, IDL_IP_PORT_MAX_LEN,
                                   NI_NUMERICHOST | NI_NUMERICSERV) != 0,
                GetNameInfoError);
            break;

        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_REMOTE_SOCKADDR:
            break;

        default:
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
            sRet = idlOS::snprintf(aBuf, aBufLen, "IB %s:%s",
                                   sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_IMPL:
            sRet = idlOS::snprintf(aBuf, aBufLen, "IB");
            break;

        case CMN_LINK_INFO_LOCAL_ADDRESS:
        case CMN_LINK_INFO_REMOTE_ADDRESS:
            sRet = idlOS::snprintf(aBuf, aBufLen, "%s:%s",
                                   sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_LOCAL_IP_ADDRESS:
        case CMN_LINK_INFO_REMOTE_IP_ADDRESS:
            sRet = idlOS::snprintf(aBuf, aBufLen, "%s", sAddrStr);
            break;

        case CMN_LINK_INFO_LOCAL_PORT:
        case CMN_LINK_INFO_REMOTE_PORT:
            sRet = idlOS::snprintf(aBuf, aBufLen, "%s", sPortStr);
            break;

        case CMN_LINK_INFO_REMOTE_SOCKADDR:
            IDE_TEST_RAISE(aBufLen < (UInt)(sLink->mDesc.mAddrLen),
                           StringTruncated);
            idlOS::memcpy(aBuf, &sLink->mDesc.mAddr, sLink->mDesc.mAddrLen);
            break;

        default:
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    IDE_TEST_RAISE(sRet < 0, StringOutputError);
    IDE_TEST_RAISE((UInt)sRet >= aBufLen, StringTruncated);

    return IDE_SUCCESS;

    IDE_EXCEPTION(StringOutputError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    IDE_EXCEPTION(StringTruncated);
    {
        IDE_SET(ideSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    IDE_EXCEPTION(GetSockNameError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETSOCKNAME_ERROR));
    }
    IDE_EXCEPTION(GetNameInfoError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETNAMEINFO_ERROR));
    }
    IDE_EXCEPTION(UnsupportedLinkInfoKey);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetDescIB(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    *(cmnLinkDescIB **)aDesc = &sLink->mDesc;

    return IDE_SUCCESS;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmnLinkPeerGetSndBufSizeIB(cmnLinkPeer *aLink, SInt *aSndBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    IDE_TEST(cmnSockGetSndBufSizeIB(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetSndBufSizeIB(cmnLinkPeer *aLink, SInt aSndBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    IDE_TEST(cmnSockSetSndBufSizeIB(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetRcvBufSizeIB(cmnLinkPeer *aLink, SInt *aRcvBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    IDE_TEST(cmnSockGetRcvBufSizeIB(sLink->mDesc.mHandle, aRcvBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetRcvBufSizeIB(cmnLinkPeer *aLink, SInt aRcvBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    IDE_TEST(cmnSockSetRcvBufSizeIB(sLink->mDesc.mHandle, aRcvBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmnLinkPeerConnByIP(cmnLinkPeer       *aLink,
                                  cmnLinkConnectArg *aConnectArg,
                                  PDL_Time_Value    *aTimeout,
                                  SInt               aOption,
                                  struct addrinfo   *aAddr);

static IDE_RC cmnLinkPeerConnByName(cmnLinkPeer       *aLink,
                                    cmnLinkConnectArg *aConnectArg,
                                    PDL_Time_Value    *aTimeout,
                                    SInt               aOption,
                                    struct addrinfo   *aAddrFirst);

static SInt cmnConnectTimedWait(PDL_SOCKET       aSock,
                                struct sockaddr *aAddr,
                                int              aAddrLen,
                                PDL_Time_Value  *aTimeout);

IDE_RC cmnLinkPeerConnectIB(cmnLinkPeer       *aLink,
                            cmnLinkConnectArg *aConnectArg,
                            PDL_Time_Value    *aTimeout,
                            SInt               aOption)
{
    UChar                sTempAddr[ID_SIZEOF(struct in6_addr)];
    idBool               sHasIPAddr;
    struct addrinfo      sHints;
    SChar                sPortStr[IDL_IP_PORT_MAX_LEN];
    SInt                 sRet = 0;
    const SChar         *sErrStr = NULL;
    SChar                sErrMsg[256];
    struct addrinfo     *sAddrFirst = NULL;

    /* error, if the libraryr for IB was failed to load */
    IDE_TEST_RAISE(gIB.mRdmaCmHandle.mHandle == NULL, FailedToLoadIBLibrary);

    /* proj-1538 ipv6 */
    idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo));
    sHints.ai_socktype = SOCK_STREAM;
#if defined(AI_NUMERICSERV)
    sHints.ai_flags    = AI_NUMERICSERV; /* port no */
#endif

    /* inet_pton() returns a positive value on success */
    /* check to see if ipv4 address. ex) ddd.ddd.ddd.ddd */
    if (idlOS::inet_pton(AF_INET, aConnectArg->mIB.mAddr,
                         sTempAddr) > 0)
    {
        sHasIPAddr = ID_TRUE;
        sHints.ai_family   = AF_INET;
        sHints.ai_flags    |= AI_NUMERICHOST;
    }
    /* check to see if ipv6 address. ex) h:h:h:h:h:h:h:h */
    /* bug-30835: support link-local address with zone index.
     *  before: inet_pton(inet6)
     *  after : strchr(':')
     *  inet_pton does not support zone index form. */
    else if (idlOS::strchr(aConnectArg->mIB.mAddr, ':') != NULL)
    {
        sHasIPAddr = ID_TRUE;
        sHints.ai_family   = AF_INET6;
        sHints.ai_flags    |= AI_NUMERICHOST;
    }
    /* hostname format. ex) db.server.com */
    else
    {
        sHasIPAddr = ID_FALSE;
        sHints.ai_family   = AF_UNSPEC;
    }

    idlOS::sprintf(sPortStr, "%"ID_UINT32_FMT"", aConnectArg->mIB.mPort);
    sRet = idlOS::getaddrinfo(aConnectArg->mIB.mAddr, sPortStr, &sHints, &sAddrFirst);

    IDE_TEST_RAISE((sRet != 0) || (sAddrFirst == NULL), GetAddrInfoError);

    /* in case that a user inputs the IP address directly */
    if (sHasIPAddr == ID_TRUE)
    {
        IDE_TEST(cmnLinkPeerConnByIP(aLink, aConnectArg, aTimeout,
                                     aOption, sAddrFirst)
                 != IDE_SUCCESS);
    }
    /* in case that a user inputs the hostname instead of IP */
    else
    {
        IDE_TEST(cmnLinkPeerConnByName(aLink, aConnectArg, aTimeout,
                                     aOption, sAddrFirst)
                 != IDE_SUCCESS);
    }

    if (sAddrFirst != NULL)
    {
        idlOS::freeaddrinfo(sAddrFirst);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(FailedToLoadIBLibrary)
    {
        IDE_SET(ideSetErrorCode( cmERR_ABORT_DLSYM,
                                 CMN_IB_RDMACM_LIB_NAME,
                                 gIB.mLibErrorMsg ));
    }
    IDE_EXCEPTION(GetAddrInfoError);
    {
        sErrStr = idlOS::gai_strerror(sRet);
        sErrMsg[0] = '\0';
        if (sErrStr == NULL)
        {
            idlOS::sprintf(sErrMsg, "%"ID_INT32_FMT, sRet);
        }
        else
        {
            idlOS::sprintf(sErrMsg, "%s", sErrStr);
        }
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    if (sAddrFirst != NULL)
    {
        idlOS::freeaddrinfo(sAddrFirst);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the IP address directly */
static IDE_RC cmnLinkPeerConnByIP(cmnLinkPeer       *aLink,
                                  cmnLinkConnectArg *aConnectArg,
                                  PDL_Time_Value    *aTimeout,
                                  SInt               aOption,
                                  struct addrinfo   *aAddr)
{
    cmnLinkPeerIB       *sLink = (cmnLinkPeerIB *)aLink;
    PDL_SOCKET           sHandle = PDL_INVALID_SOCKET;
    SInt                 sRet = 0;

    sHandle = gIB.mFuncs.rsocket(aAddr->ai_family, SOCK_STREAM, 0);

    IDE_TEST_RAISE(sHandle == PDL_INVALID_SOCKET, SocketError);

    sRet = cmnConnectTimedWait(sHandle,
                               aAddr->ai_addr,
                               aAddr->ai_addrlen,
                               aTimeout);

    /* bug-30835: support link-local address with zone index.
     * On linux, link-local addr without zone index makes EINVAL.
     *  .EINVAL   : Invalid argument to connect()
     *  .others   : Client unable to establish connection. */
    if (sRet != 0)
    {
        if (errno == EINVAL)
        {
            IDE_RAISE(ConnInvalidArg);
        }
        else
        {
            IDE_TEST_RAISE( errno == ETIMEDOUT, TimedOut );
            IDE_RAISE(ConnectError);
        }
    }
    else
    {
        /* connected successfully */
    }

    /* save fd and IP address into link info */
    sLink->mDesc.mHandle = sHandle;

    idlOS::memcpy(&sLink->mDesc.mAddr, aAddr->ai_addr, aAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = aAddr->ai_addrlen;

    /* socket 초기화 */
    sLink->mDesc.mLatency = aConnectArg->mIB.mLatency;
    sLink->mDesc.mConChkSpin = aConnectArg->mIB.mConChkSpin;

    IDE_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(ConnectError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RCONNECT_ERROR, errno));
    }
    /* bug-30835: support link-local address with zone index. */
    IDE_EXCEPTION(ConnInvalidArg);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RCONNECT_INVALIDARG));
    }
    IDE_EXCEPTION(TimedOut);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sHandle != PDL_INVALID_SOCKET)
    {
        (void)gIB.mFuncs.rclose(sHandle);
    }
    else
    {
        /* nothing to do */
    }

    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;

    return IDE_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the hostname instead of IP */
static IDE_RC cmnLinkPeerConnByName(cmnLinkPeer*       aLink,
                                    cmnLinkConnectArg* aConnectArg,
                                    PDL_Time_Value*    aTimeout,
                                    SInt               aOption,
                                    struct addrinfo*   aAddrFirst)
{
    cmnLinkPeerIB       *sLink = (cmnLinkPeerIB *)aLink;
    struct addrinfo     *sAddr = NULL;

#define CM_HOST_IP_MAX_COUNT 10
    struct addrinfo     *sAddrListV4[CM_HOST_IP_MAX_COUNT];
    struct addrinfo     *sAddrListV6[CM_HOST_IP_MAX_COUNT];
    struct addrinfo    **sAddrListFinal = NULL;

    SInt                 sAddrCntV4 = 0;
    SInt                 sAddrCntV6 = 0;
    SInt                 sAddrCntFinal = 0;

    SInt                 sIdx    = 0;
    SInt                 sTryCnt = 0;
    PDL_SOCKET           sHandle = PDL_INVALID_SOCKET;
    SInt                 sRet    = 0;


    idlOS::memset(sAddrListV4, 0x00, ID_SIZEOF(sAddrListV4));
    idlOS::memset(sAddrListV6, 0x00, ID_SIZEOF(sAddrListV6));

    for (sAddr = aAddrFirst;
         (sAddr != NULL) && (sAddrCntFinal < CM_HOST_IP_MAX_COUNT);
         sAddr = sAddr->ai_next)
    {
        /* IPv4 */
        if (sAddr->ai_family == AF_INET)
        {
            sAddrListV4[sAddrCntV4++] = sAddr;
            sAddrCntFinal++;
        }
        /* IPv6 */
        else if (sAddr->ai_family == AF_INET6)
        {
            sAddrListV6[sAddrCntV6++] = sAddr;
            sAddrCntFinal++;
        }
    }

    /* if prefer IPv4, then order is v4 -> v6 */
    if (aConnectArg->mIB.mPreferIPv6 == 0)
    {
        for (sIdx = 0;
             (sIdx < sAddrCntV6) && (sAddrCntFinal < CM_HOST_IP_MAX_COUNT);
             sIdx++)
        {
            sAddrListV4[sAddrCntV4] = sAddrListV6[sIdx];
            sAddrCntV4++;
        }
        sAddrListFinal = &sAddrListV4[0];
    }
    /* if prefer IPv6, then order is v6 -> v4 */
    else
    {
        for (sIdx = 0;
             (sIdx < sAddrCntV4) && (sAddrCntFinal < CM_HOST_IP_MAX_COUNT);
             sIdx++)
        {
            sAddrListV6[sAddrCntV6] = sAddrListV4[sIdx];
            sAddrCntV6++;
        }
        sAddrListFinal = &sAddrListV6[0];
    }

    sTryCnt = 0;
    for (sIdx = 0; sIdx < sAddrCntFinal; sIdx++)
    {
        sTryCnt++;
        sAddr = sAddrListFinal[sIdx];
        
        sHandle = gIB.mFuncs.rsocket(sAddr->ai_family, SOCK_STREAM, 0);
        IDE_TEST_RAISE(sHandle == PDL_INVALID_SOCKET, SocketError);

        sRet = cmnConnectTimedWait(sHandle,
                                   sAddr->ai_addr,
                                   sAddr->ai_addrlen,
                                   aTimeout);
        if (sRet == 0)
        {
            break;
        }

        (void)gIB.mFuncs.rclose(sHandle);
        sHandle = PDL_INVALID_SOCKET;
    }

    /* bug-30835: support link-local address with zone index.
     * On linux, link-local addr without zone index makes EINVAL. */
    if ((sRet != 0) || (sTryCnt == 0))
    {
        if (errno == EINVAL)
        {
            IDE_RAISE(ConnInvalidArg);
        }
        else
        {
            IDE_TEST_RAISE( errno == ETIMEDOUT, TimedOut );
            IDE_RAISE(ConnectError);
        }
    }

    /* save fd and IP address into link info */
    sLink->mDesc.mHandle = sHandle;

    idlOS::memcpy(&sLink->mDesc.mAddr, sAddr->ai_addr, sAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = sAddr->ai_addrlen;

    /* socket 초기화 */
    sLink->mDesc.mLatency = aConnectArg->mIB.mLatency;
    sLink->mDesc.mConChkSpin = aConnectArg->mIB.mConChkSpin;

    IDE_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(ConnectError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RCONNECT_ERROR, errno));
    }
    /* bug-30835: support link-local address with zone index. */
    IDE_EXCEPTION(ConnInvalidArg);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RCONNECT_INVALIDARG));
    }
    IDE_EXCEPTION(TimedOut);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sHandle != PDL_INVALID_SOCKET)
    {
        (void)gIB.mFuncs.rclose(sHandle);
    }
    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;

    return IDE_FAILURE;
}

/* It's similar to idlVA::connect_timed_wait() */
static SInt cmnConnectTimedWait(PDL_SOCKET       aSock,
                                struct sockaddr *aAddr,
                                int              aAddrLen,
                                PDL_Time_Value  *aTimeout)
{
    SInt   sRet;
    SInt   sError = 0;
    SInt   sLen;
    struct pollfd   sPollFd;
    acp_sint32_t    sTimeout = (aTimeout != NULL) ? aTimeout->msec() : -1;

    /* change non-blocking mode only when connecting */
    IDE_TEST(cmnSockSetBlockingModeIB(aSock, ID_FALSE) != IDE_SUCCESS);

    sRet = gIB.mFuncs.rconnect(aSock, aAddr, aAddrLen);
    if (sRet != 0)
    {
        /**
         * [CAUTION]
         *
         * Berkeley socket 과 다르게 blocking mode 이더라도
         * rconnect() 함수는 'connecting' 상태로 반환될 수 있음.
         */

        /* error, if the connection isn't now in progress */
        IDE_TEST((sRet < 0) && (errno != EINPROGRESS) && (errno != EWOULDBLOCK)); 

        /* wait until connecting */
        sPollFd.fd      = aSock;
        sPollFd.events  = POLLIN | POLLOUT;
        sPollFd.revents = 0;

        sRet = gIB.mFuncs.rpoll(&sPollFd, 1, sTimeout);
        IDE_TEST(sRet < 0); /* error */

        IDE_TEST_RAISE(sRet == 0, TimedOut); /* timed out */

        /* error, if it was not occured either read or write event */
        IDE_TEST(sPollFd.revents == 0);

        /* check socket error */
        sLen = (SInt)sizeof(sError);

        IDE_TEST(gIB.mFuncs.rgetsockopt(aSock,
                                        SOL_SOCKET,
                                        SO_ERROR,
                                        &sError,
                                        (socklen_t *)&sLen) < 0);

        IDE_TEST(sError != 0);
    }
    else
    {
        /* connected successfully */
    }

    (void)cmnSockSetBlockingModeIB(aSock, ID_TRUE);

    return 0;

    IDE_EXCEPTION(TimedOut)
    {
        sError = ETIMEDOUT;
    }
    IDE_EXCEPTION_END;

    (void)cmnSockSetBlockingModeIB(aSock, ID_TRUE);

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

IDE_RC cmnLinkPeerAllocChannelIB(cmnLinkPeer */*aLink*/, SInt */*aChannelID*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerHandshakeIB(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetOptionsIB(cmnLinkPeer *aLink, SInt aOption)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;
    SInt           sOption;
    linger         sLingerOption;

    /*
     * SO_KEEPALIVE 세팅
     */
    sOption = 1;

    if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                               SOL_SOCKET,
                               SO_KEEPALIVE,
                               &sOption,
                               ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_KEEPALIVE_FAIL, errno);
    }
    else
    {
        /* set socket options successfully */
    }

    // BUG-26484: 추가로 설정할 소켓 옵션을 지정
    if (aOption == SO_LINGER)
    {
        // 연속으로 연결했다 끊기를 반복하면 더 이상 연결할 수 없게된다.
        // 일반적으로 소켓은 close해도 TIME_WAIT 상태로 일정시간 대기하기 때문이다.
        // SO_LINGER 옵션 추가. (SO_REUSEADDR 옵션으로는 잘 안될 수도 있다;)
        /*
        * SO_LINGER 세팅
        */
        sLingerOption.l_onoff  = 1;
        sLingerOption.l_linger = 0;

        if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                                   SOL_SOCKET,
                                   SO_LINGER,
                                   (SChar *)&sLingerOption,
                                   ID_SIZEOF(sLingerOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_SET_OPTION_FAIL, "SO_LINGER", errno);
        }
        else
        {
            /* set socket options successfully */
        }
    }
    else if (aOption == SO_REUSEADDR)
    {
        /*
         * SO_REUSEADDR 세팅
         */
        sOption = 1;

        if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                                   SOL_SOCKET,
                                   SO_REUSEADDR,
                                   &sOption,
                                   ID_SIZEOF(sOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_SET_OPTION_FAIL, "SO_REUSEADDR", errno);
        }
        else
        {
            /* set socket options successfully */
        }
    }
    else
    {
        /* ignore unsupported option */
    }

    /*
     * TCP_NODELAY 세팅
     */
    sOption = 1;

    if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                               IPPROTO_TCP,
                               TCP_NODELAY,
                               &sOption,
                               ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_IB_TCP_NODELAY_FAIL, errno);
    }
    else
    {
        /* set socket options successfully */
    }

    /* BUG-22028
     * SO_SNDBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                               SOL_SOCKET,
                               SO_SNDBUF,
                               &sOption,
                               ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_SET_OPTION_FAIL, "SO_SNDBUF", errno);
    }
    else
    {
        /* set socket options successfully */
    }

    /* BUG-22028
     * SO_RCVBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                               SOL_SOCKET,
                               SO_RCVBUF,
                               &sOption,
                               ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_SET_OPTION_FAIL, "SO_RCVBUF", errno);
    }
    else
    {
        /* set socket options successfully */
    }

    /**
     * RDMA_LATENCY
     */
    if (sLink->mDesc.mLatency != 0)
    {
        sOption = sLink->mDesc.mLatency;

        if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                                   SOL_RDMA,
                                   RDMA_LATENCY,
                                   &sOption,
                                   ID_SIZEOF(sOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_SET_OPTION_FAIL, "RDMA_LATENCY", errno);
        }
        else
        {
            /* set socket options successfully */
        }
    }
    else
    {
        /* default RDMA_LATENCY = 0 in rsockets */
    }

    /* RDMA_CONCHKSPIN */
    if (sLink->mDesc.mConChkSpin != 0)
    {
        sOption = sLink->mDesc.mConChkSpin;

        if (gIB.mFuncs.rsetsockopt(sLink->mDesc.mHandle,
                                   SOL_RDMA,
                                   RDMA_CONCHKSPIN,
                                   &sOption,
                                   ID_SIZEOF(sOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_SET_OPTION_FAIL, "RDMA_CONCHKSPIN", errno);
        }
        else
        {
            /* set socket options successfully */
        }
    }
    else
    {
        /* default RDMA_CONCHKSPIN = 0 in rsockets */
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownIB(cmnLinkPeer *aLink,
                             cmnDirection aDirection,
                             cmnShutdownMode /*aMode*/)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;
    SInt           sRet;

    /* skip, if the library for IB was failed to load */
    IDE_TEST_CONT((gIB.mRdmaCmHandle.mHandle == NULL) ||
                  (sLink->mDesc.mHandle == PDL_INVALID_SOCKET),  /* BUG-46161 */
                  SkipShutdown);

    sRet = gIB.mFuncs.rshutdown(sLink->mDesc.mHandle, aDirection);

    IDE_TEST_RAISE((sRet != 0) && (errno != ENOTCONN), ShutdownError);

    IDE_EXCEPTION_CONT(SkipShutdown);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ShutdownError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSOCKET_SHUTDOWN_FAILED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerRecvIB( cmnLinkPeer     *aLink,
                          cmbBlock       **aBlock,
                          cmpHeader       *aHeader,
                          PDL_Time_Value  *aTimeout )
{
    cmnLinkPeerIB *sLink  = (cmnLinkPeerIB *)aLink;
    cmbPool       *sPool  = aLink->mPool;
    cmbBlock      *sBlock;
    cmpHeader      sHeader;
    UShort         sPacketSize = 0;
    cmpPacketType  sPacketType = aLink->mLink.mPacketType;

    /* not accept A5 protocol */
    IDE_TEST_RAISE(sPacketType == CMP_PACKET_TYPE_A5, UnsupportedNetworkProtocol);

    /*
     * Pending Block있으면 사용 그렇지 않으면 Block 할당
     */
    /* proj_2160 cm_type removal */
    /* A7 or CMP_PACKET_TYPE_UNKNOWN: block already allocated. */
    sBlock = *aBlock;

    if (sLink->mPendingBlock != NULL)
    {
        idlOS::memcpy(sBlock->mData,
                      sLink->mPendingBlock->mData,
                      sLink->mPendingBlock->mDataSize);
        sBlock->mDataSize = sLink->mPendingBlock->mDataSize;

        IDE_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != IDE_SUCCESS);
        sLink->mPendingBlock = NULL;
    }
    else
    {
        /* nothing to do */
    }

    /*
     * Protocol Header Size 크기 이상 읽음
     */
    IDE_TEST_RAISE(cmnSockRecvIB(sBlock,
                                 aLink,
                                 sLink->mDesc.mHandle,
                                 CMP_HEADER_SIZE,
                                 aTimeout,
                                 IDV_STAT_INDEX_RECV_IB_BYTE) != IDE_SUCCESS, SockRecvError);

    /*
     * Protocol Header 해석
     */
    IDE_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != IDE_SUCCESS);
    sPacketSize = sHeader.mA7.mPayloadLength + CMP_HEADER_SIZE;

    /*
     * 패킷 크기 이상 읽음
     */
    IDE_TEST_RAISE(cmnSockRecvIB(sBlock,
                                 aLink,
                                 sLink->mDesc.mHandle,
                                 sPacketSize,
                                 aTimeout,
                                 IDV_STAT_INDEX_RECV_IB_BYTE) != IDE_SUCCESS, SockRecvError);

    /*
     * 패킷 크기 이상 읽혔으면 현재 패킷 이후의 데이터를 Pending Block으로 옮김
     */
    if (sBlock->mDataSize > sPacketSize)
    {
        IDE_TEST(sPool->mOp->mAllocBlock(sPool, &sLink->mPendingBlock) != IDE_SUCCESS);
        IDE_TEST(cmbBlockMove(sLink->mPendingBlock, sBlock, sPacketSize) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    /*
     * Block과 Header를 돌려줌
     */
    /* proj_2160 cm_type removal
     *  Do not use mLink.mPacketType. instead, use sPacketType.
     * cuz, mLink's value could be changed in cmpHeaderRead().
     * and, this if-stmt shows explicitly that
     * it needs to return block ptr in only A5. */

    *aHeader = sHeader;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( UnsupportedNetworkProtocol )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_NETWORK_PROTOCOL));
    }
    /* BUG-39127 If a network timeout occurs during replication sync 
     * then it fails by communication protocol error.
     */ 
    IDE_EXCEPTION( SockRecvError )
    {
        if ( sBlock->mDataSize != 0 )
        {
            if ( sPool->mOp->mAllocBlock(sPool, &sLink->mPendingBlock) == IDE_SUCCESS )
            {
                IDE_ASSERT( cmbBlockMove(sLink->mPendingBlock, sBlock, 0) == IDE_SUCCESS );
            }
            else
            {
                /* when the alloc error occured, the error must return to upper module
                 * therefore in this exception, do not use IDE_PUSH() and IDE_POP()
                 */
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerReqCompleteIB(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerResCompleteIB(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSendIB(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    IDE_TEST(cmnSockSendIB(aBlock,
                           aLink,
                           sLink->mDesc.mHandle,
                           NULL,
                           IDV_STAT_INDEX_SEND_IB_BYTE) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCheckIB(cmnLinkPeer *aLink, idBool *aIsClosed)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    return cmnSockCheckIB((cmnLink *)aLink, sLink->mDesc.mHandle, aIsClosed);
}

idBool cmnLinkPeerHasPendingRequestIB(cmnLinkPeer *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    return (sLink->mPendingBlock != NULL) ? ID_TRUE : ID_FALSE;
}

IDE_RC cmnLinkPeerAllocBlockIB(cmnLinkPeer *aLink, cmbBlock **aBlock)
{
    cmbBlock *sBlock;

    IDE_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != IDE_SUCCESS);

    sBlock->mDataSize = CMP_HEADER_SIZE;
    sBlock->mCursor   = CMP_HEADER_SIZE;

    *aBlock = sBlock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerFreeBlockIB(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

struct cmnLinkOP gCmnLinkPeerOpIB =
{
    "IB-PEER",

    cmnLinkPeerInitializeIB,
    cmnLinkPeerFinalizeIB,

    cmnLinkPeerCloseIB,

    cmnLinkPeerGetHandleIB,

    cmnLinkPeerGetDispatchInfoIB,
    cmnLinkPeerSetDispatchInfoIB
};

struct cmnLinkPeerOP gCmnLinkPeerPeerOpIB =
{
    cmnLinkPeerSetBlockingModeIB,

    cmnLinkPeerGetInfoIB,
    cmnLinkPeerGetDescIB,

    cmnLinkPeerConnectIB,
    cmnLinkPeerSetOptionsIB,

    cmnLinkPeerAllocChannelIB,
    cmnLinkPeerHandshakeIB,

    cmnLinkPeerShutdownIB,

    cmnLinkPeerRecvIB,
    cmnLinkPeerSendIB,

    cmnLinkPeerReqCompleteIB,
    cmnLinkPeerResCompleteIB,

    cmnLinkPeerCheckIB,
    cmnLinkPeerHasPendingRequestIB,

    cmnLinkPeerAllocBlockIB,
    cmnLinkPeerFreeBlockIB,

    /* TASK-5894 Permit sysdba via IPC */
    NULL,

    /* PROJ-2474 SSL/TLS */
    NULL,
    NULL, 
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    cmnLinkPeerGetSndBufSizeIB,
    cmnLinkPeerSetSndBufSizeIB,
    cmnLinkPeerGetRcvBufSizeIB,
    cmnLinkPeerSetRcvBufSizeIB
};

IDE_RC cmnLinkPeerMapIB(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    IDE_ASSERT((aLink->mType == CMN_LINK_TYPE_PEER_SERVER) ||
               (aLink->mType == CMN_LINK_TYPE_PEER_CLIENT));
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IB);

    /* Shared Pool 획득 */
    IDE_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != IDE_SUCCESS);

    aLink->mOp     = &gCmnLinkPeerOpIB;
    sLink->mPeerOp = &gCmnLinkPeerPeerOpIB;

    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt cmnLinkPeerSizeIB(void)
{
    return ID_SIZEOF(cmnLinkPeerIB);
}

