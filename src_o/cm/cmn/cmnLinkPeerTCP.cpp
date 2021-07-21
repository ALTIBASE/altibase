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

#if !defined(CM_DISABLE_TCP)

typedef struct cmnLinkPeerTCP
{
    cmnLinkPeer     mLinkPeer;

    cmnLinkDescTCP  mDesc;

    UInt            mDispatchInfo;

    cmbBlock       *mPendingBlock;
} cmnLinkPeerTCP;


IDE_RC cmnLinkPeerInitializeTCP(cmnLink *aLink)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * 멤버 초기화
     */
    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    sLink->mDispatchInfo = 0;

    sLink->mPendingBlock = NULL;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFinalizeTCP(cmnLink *aLink)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;
    cmbPool        *sPool = sLink->mLinkPeer.mPool;

    /*
     * socket이 열려있으면 닫음
     */
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    /*
     * Pending Block이 할당되어 있으면 해제
     */
    if (sLink->mPendingBlock != NULL)
    {
        IDE_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCloseTCP(cmnLink *aLink)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * socket이 열려있으면 닫음
     */
    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mDesc.mHandle);

        sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetHandleTCP(cmnLink *aLink, void *aHandle)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * socket을 돌려줌
     */
    *(PDL_SOCKET *)aHandle = sLink->mDesc.mHandle;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetDispatchInfoTCP(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(UInt *)aDispatchInfo = sLink->mDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetDispatchInfoTCP(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(UInt *)aDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetBlockingModeTCP(cmnLinkPeer *aLink, idBool aBlockingMode)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * Set Blocking Mode
     */
    IDE_TEST(cmnSockSetBlockingMode(sLink->mDesc.mHandle, aBlockingMode) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_SOCKET_SET_BLOCKING_MODE_FAIL, errno);
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetInfoTCP(cmnLinkPeer *aLink, SChar *aBuf, UInt aBufLen, cmnLinkInfoKey aKey)
{
    cmnLinkPeerTCP     *sLink = (cmnLinkPeerTCP *)aLink;
    struct sockaddr_storage  sAddr;
    SInt                sAddrLen;
    SInt                sRet = 0;
    SChar               sErrMsg[256];

    SChar               sAddrStr[IDL_IP_ADDR_MAX_LEN];
    SChar               sPortStr[IDL_IP_PORT_MAX_LEN];

    sErrMsg[0] = '\0';

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
            IDE_TEST_RAISE(idlOS::getsockname(sLink->mDesc.mHandle,
                                              (struct sockaddr *)&sAddr,
                                              &sAddrLen) != 0,
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
            break;  /* BUG-47090 */

#if defined(TCP_INFO)
        case CMN_LINK_INFO_TCP_KERNEL_STAT: /* PROJ-2625 */
            break;
#endif /* TCP_INFO */

        default:
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
            sRet = idlOS::snprintf(aBuf, aBufLen, "TCP %s:%s",
                                   sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_IMPL:
            sRet = idlOS::snprintf(aBuf, aBufLen, "TCP");
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

#if defined(TCP_INFO)
        /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
        case CMN_LINK_INFO_TCP_KERNEL_STAT:
            IDE_TEST_RAISE(idlOS::getsockopt(sLink->mDesc.mHandle,
                                             SOL_TCP,
                                             TCP_INFO,
                                             aBuf,
                                             (SInt *)&aBufLen) < 0, GetSockOptError);
            break;
#endif /* TCP_INFO */

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
#if defined(TCP_INFO)
    IDE_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "TCP_INFO", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETSOCKOPT_ERROR, sErrMsg));
    }
#endif /* TCP_INFO */
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetDescTCP(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * Desc를 돌려줌
     */
    *(cmnLinkDescTCP **)aDesc = &sLink->mDesc;

    return IDE_SUCCESS;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmnLinkPeerGetSndBufSizeTCP(cmnLinkPeer *aLink, SInt *aSndBufSize)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    IDE_TEST(cmnSockGetSndBufSize(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetSndBufSizeTCP(cmnLinkPeer *aLink, SInt aSndBufSize)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    IDE_TEST(cmnSockSetSndBufSize(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetRcvBufSizeTCP(cmnLinkPeer *aLink, SInt *aRcvBufSize)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    IDE_TEST(cmnSockGetRcvBufSize(sLink->mDesc.mHandle, aRcvBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetRcvBufSizeTCP(cmnLinkPeer *aLink, SInt aRcvBufSize)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    IDE_TEST(cmnSockSetRcvBufSize(sLink->mDesc.mHandle, aRcvBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmnLinkPeerConnByIP(cmnLinkPeer*    aLink,
                           cmnLinkConnectArg* aConnectArg,
                           PDL_Time_Value*    aTimeout,
                           SInt               aOption,
                           struct addrinfo*   aAddr);

static IDE_RC cmnLinkPeerConnByName(cmnLinkPeer*    aLink,
                           cmnLinkConnectArg* aConnectArg,
                           PDL_Time_Value*    aTimeout,
                           SInt               aOption,
                           struct addrinfo*   aAddrFirst);

IDE_RC cmnLinkPeerConnectTCP(cmnLinkPeer *aLink,
                             cmnLinkConnectArg *aConnectArg,
                             PDL_Time_Value *aTimeout,
                             SInt aOption)
{
    UChar                sTempAddr[ID_SIZEOF(struct in6_addr)];
    idBool               sHasIPAddr;
    struct addrinfo      sHints;
    SChar                sPortStr[IDL_IP_PORT_MAX_LEN];
    SInt                 sRet = 0;
    const SChar         *sErrStr = NULL;
    SChar                sErrMsg[256];
    struct addrinfo     *sAddrFirst = NULL;


    /* proj-1538 ipv6 */
    idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo));
    sHints.ai_socktype = SOCK_STREAM;
#if defined(AI_NUMERICSERV)
    sHints.ai_flags    = AI_NUMERICSERV; /* port no */
#endif

    /* inet_pton() returns a positive value on success */
    /* check to see if ipv4 address. ex) ddd.ddd.ddd.ddd */
    if (idlOS::inet_pton(AF_INET, aConnectArg->mTCP.mAddr,
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
    else if (idlOS::strchr(aConnectArg->mTCP.mAddr, ':') != NULL)
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

    idlOS::sprintf(sPortStr, "%"ID_UINT32_FMT"", aConnectArg->mTCP.mPort);
    sRet = idlOS::getaddrinfo(aConnectArg->mTCP.mAddr, sPortStr,
                              &sHints, &sAddrFirst);
    if ((sRet != 0) || (sAddrFirst == NULL))
    {
        IDE_RAISE(GetAddrInfoError);
    }

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
    return IDE_SUCCESS;

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
    return IDE_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the IP address directly */
static IDE_RC cmnLinkPeerConnByIP(cmnLinkPeer*       aLink,
                                  cmnLinkConnectArg* /* aConnectArg */,
                                  PDL_Time_Value*    aTimeout,
                                  SInt               aOption,
                                  struct addrinfo*   aAddr)
{
    cmnLinkPeerTCP      *sLink = (cmnLinkPeerTCP *)aLink;
    PDL_SOCKET           sHandle = PDL_INVALID_SOCKET;
    SInt                 sRet = 0;

    sHandle = idlOS::socket(aAddr->ai_family, SOCK_STREAM, 0);
    IDE_TEST_RAISE(sHandle == PDL_INVALID_SOCKET, SocketError);
    sRet = idlVA::connect_timed_wait(sHandle,
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

    /* save fd and IP address into link info */
    sLink->mDesc.mHandle = sHandle;
    idlOS::memcpy(&sLink->mDesc.mAddr, aAddr->ai_addr, aAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = aAddr->ai_addrlen;

    /* socket 초기화 */
    IDE_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(ConnectError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_ERROR, errno));
    }
    /* bug-30835: support link-local address with zone index. */
    IDE_EXCEPTION(ConnInvalidArg);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_INVALIDARG));
    }
    IDE_EXCEPTION(TimedOut);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sHandle != PDL_INVALID_SOCKET)
    {
        (void)idlOS::closesocket(sHandle);
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
    cmnLinkPeerTCP      *sLink = (cmnLinkPeerTCP *)aLink;
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
    if (aConnectArg->mTCP.mPreferIPv6 == 0)
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
        
        sHandle = idlOS::socket(sAddr->ai_family, SOCK_STREAM, 0);
        IDE_TEST_RAISE(sHandle == PDL_INVALID_SOCKET, SocketError);
        sRet = idlVA::connect_timed_wait(sHandle,
                                         sAddr->ai_addr,
                                         sAddr->ai_addrlen,
                                         aTimeout);
        if (sRet == 0)
        {
            break;
        }
        (void)idlOS::closesocket(sHandle);
        sHandle = PDL_INVALID_SOCKET;

        //printf("[debug] connerr [cnt: %d], errno [%d]\n", sTryCnt, errno);
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
    IDE_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(ConnectError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_ERROR, errno));
    }
    /* bug-30835: support link-local address with zone index. */
    IDE_EXCEPTION(ConnInvalidArg);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_INVALIDARG));
    }
    IDE_EXCEPTION(TimedOut);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sHandle != PDL_INVALID_SOCKET)
    {
        (void)idlOS::closesocket(sHandle);
    }
    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerAllocChannelTCP(cmnLinkPeer */*aLink*/, SInt */*aChannelID*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerHandshakeTCP(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetOptionsTCP(cmnLinkPeer *aLink, SInt aOption)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;
    SInt            sOption;
    linger          sLingerOption;

    /*
     * SO_KEEPALIVE 세팅
     */
    sOption = 1;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          SOL_SOCKET,
                          SO_KEEPALIVE,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_KEEPALIVE_FAIL, errno);
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

        if (idlOS::setsockopt(sLink->mDesc.mHandle,
                              SOL_SOCKET,
                              SO_LINGER,
                              (SChar *)&sLingerOption,
                              ID_SIZEOF(sLingerOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_LINGER", errno);
        }
    }
    else if (aOption == SO_REUSEADDR)
    {
        /*
         * SO_REUSEADDR 세팅
         */
        sOption = 1;

        if (idlOS::setsockopt(sLink->mDesc.mHandle,
                              SOL_SOCKET,
                              SO_REUSEADDR,
                              (SChar *)&sOption,
                              ID_SIZEOF(sOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_REUSEADDR", errno);
        }
    }

    /*
     * TCP_NODELAY 세팅
     */
    sOption = 1;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          IPPROTO_TCP,
                          TCP_NODELAY,
                          (char *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_TCP_NODELAY_FAIL, errno);
    }

    /* BUG-22028
     * SO_SNDBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          SOL_SOCKET,
                          SO_SNDBUF,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_SNDBUF", errno);
    }

    /* BUG-22028
     * SO_RCVBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          SOL_SOCKET,
                          SO_RCVBUF,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_RCVBUF", errno);
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownTCP(cmnLinkPeer *aLink,
                              cmnDirection aDirection,
                              cmnShutdownMode /*aMode*/)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * shutdown
     */
    IDE_TEST_RAISE(idlOS::shutdown(sLink->mDesc.mHandle, aDirection) != 0, ShutdownError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ShutdownError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_SHUTDOWN_FAILED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerRecvTCP(cmnLinkPeer *aLink, cmbBlock **aBlock, cmpHeader *aHeader, PDL_Time_Value *aTimeout)
{
    cmnLinkPeerTCP *sLink  = (cmnLinkPeerTCP *)aLink;
    cmbPool        *sPool  = aLink->mPool;
    cmbBlock       *sBlock = NULL;
    cmpHeader       sHeader;
    UShort          sPacketSize = 0;
    cmpPacketType   sPacketType = aLink->mLink.mPacketType;

    /*
     * Pending Block있으면 사용 그렇지 않으면 Block 할당
     */
    /* proj_2160 cm_type removal */
    /* A7 or CMP_PACKET_TYPE_UNKNOWN: block already allocated. */
    if (sPacketType != CMP_PACKET_TYPE_A5)
    {
        sBlock = *aBlock;
        if (sLink->mPendingBlock != NULL)
        {
            idlOS::memcpy(sBlock->mData,
                          sLink->mPendingBlock->mData,
                          sLink->mPendingBlock->mDataSize);
            sBlock->mDataSize = sLink->mPendingBlock->mDataSize;

            IDE_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock)
                     != IDE_SUCCESS);
            sLink->mPendingBlock = NULL;
        }
    }
    // A5: block will be allocated here.
    else
    {
        if (sLink->mPendingBlock != NULL)
        {
            sBlock               = sLink->mPendingBlock;
            sLink->mPendingBlock = NULL;
        }
        else
        {
            IDE_TEST(sPool->mOp->mAllocBlock(sPool, &sBlock) != IDE_SUCCESS);
        }
    }

    /*
     * Protocol Header Size 크기 이상 읽음
     */
    IDE_TEST_RAISE(cmnSockRecv(sBlock,
                               aLink,
                               sLink->mDesc.mHandle,
                               CMP_HEADER_SIZE,
                               aTimeout,
                               IDV_STAT_INDEX_RECV_TCP_BYTE) != IDE_SUCCESS, SockRecvError);

    /*
     * Protocol Header 해석
     */
    IDE_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != IDE_SUCCESS);
    sPacketSize = sHeader.mA7.mPayloadLength + CMP_HEADER_SIZE;

    /*
     * 패킷 크기 이상 읽음
     */
    IDE_TEST_RAISE(cmnSockRecv(sBlock,
                               aLink,
                               sLink->mDesc.mHandle,
                               sPacketSize,
                               aTimeout,
                               IDV_STAT_INDEX_RECV_TCP_BYTE) != IDE_SUCCESS, SockRecvError);

    /*
     * 패킷 크기 이상 읽혔으면 현재 패킷 이후의 데이터를 Pending Block으로 옮김
     */
    if (sBlock->mDataSize > sPacketSize)
    {
        IDE_TEST(sPool->mOp->mAllocBlock(sPool, &sLink->mPendingBlock) != IDE_SUCCESS);
        IDE_TEST(cmbBlockMove(sLink->mPendingBlock, sBlock, sPacketSize) != IDE_SUCCESS);
    }

    /*
     * Block과 Header를 돌려줌
     */
    /* proj_2160 cm_type removal
     *  Do not use mLink.mPacketType. instead, use sPacketType.
     * cuz, mLink's value could be changed in cmpHeaderRead().
     * and, this if-stmt shows explicitly that
     * it needs to return block ptr in only A5. */
    if (sPacketType == CMP_PACKET_TYPE_A5)
    {
        *aBlock  = sBlock;
    }
    *aHeader = sHeader;

    return IDE_SUCCESS;
    
    /* BUG-39127 If a network timeout occurs during replication sync 
     * then it fails by communication protocol error.
     */ 
    IDE_EXCEPTION( SockRecvError )
    {
        if ( sBlock->mDataSize != 0 )
        {
            if( sPool->mOp->mAllocBlock( sPool, &sLink->mPendingBlock ) == IDE_SUCCESS )
            {
                IDE_ASSERT( cmbBlockMove(sLink->mPendingBlock, sBlock, 0 ) == IDE_SUCCESS );
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
        if( sPacketType == CMP_PACKET_TYPE_A5 )
        {
            *aBlock = sBlock;
        }
        else
        {
            /* nothing to do */
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerReqCompleteTCP(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerResCompleteTCP(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSendTCP(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    /*
     * Block 전송
     */
    IDE_TEST(cmnSockSend(aBlock,
                         aLink,
                         sLink->mDesc.mHandle,
                         NULL,
                         IDV_STAT_INDEX_SEND_TCP_BYTE) != IDE_SUCCESS);

    /* proj_2160 cm_type removal
     * A7 use static-block for a session */
    if (aLink->mLink.mPacketType == CMP_PACKET_TYPE_A5)
    {
        IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCheckTCP(cmnLinkPeer *aLink, idBool *aIsClosed)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    return cmnSockCheck((cmnLink *)aLink, sLink->mDesc.mHandle, aIsClosed);
}

idBool cmnLinkPeerHasPendingRequestTCP(cmnLinkPeer *aLink)
{
    cmnLinkPeerTCP *sLink = (cmnLinkPeerTCP *)aLink;

    return (sLink->mPendingBlock != NULL) ? ID_TRUE : ID_FALSE;
}

IDE_RC cmnLinkPeerAllocBlockTCP(cmnLinkPeer *aLink, cmbBlock **aBlock)
{
    cmbBlock *sBlock;

    IDE_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != IDE_SUCCESS);

    /*
     * Write Block 초기화
     */
    sBlock->mDataSize = CMP_HEADER_SIZE;
    sBlock->mCursor   = CMP_HEADER_SIZE;

    /*
     * Write Block을 돌려줌
     */
    *aBlock = sBlock;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerFreeBlockTCP(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    /*
     * Block 해제
     */
    IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


struct cmnLinkOP gCmnLinkPeerOpTCP =
{
    "TCP-PEER",

    cmnLinkPeerInitializeTCP,
    cmnLinkPeerFinalizeTCP,

    cmnLinkPeerCloseTCP,

    cmnLinkPeerGetHandleTCP,

    cmnLinkPeerGetDispatchInfoTCP,
    cmnLinkPeerSetDispatchInfoTCP
};

struct cmnLinkPeerOP gCmnLinkPeerPeerOpTCP =
{
    cmnLinkPeerSetBlockingModeTCP,

    cmnLinkPeerGetInfoTCP,
    cmnLinkPeerGetDescTCP,

    cmnLinkPeerConnectTCP,
    cmnLinkPeerSetOptionsTCP,

    cmnLinkPeerAllocChannelTCP,
    cmnLinkPeerHandshakeTCP,

    cmnLinkPeerShutdownTCP,

    cmnLinkPeerRecvTCP,
    cmnLinkPeerSendTCP,

    cmnLinkPeerReqCompleteTCP,
    cmnLinkPeerResCompleteTCP,

    cmnLinkPeerCheckTCP,
    cmnLinkPeerHasPendingRequestTCP,

    cmnLinkPeerAllocBlockTCP,
    cmnLinkPeerFreeBlockTCP,

    /* TASK-5894 Permit sysdba via IPC */
    NULL,

    /* PROJ-2474 SSL/TLS */
    NULL,
    NULL, 
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    cmnLinkPeerGetSndBufSizeTCP,
    cmnLinkPeerSetSndBufSizeTCP,
    cmnLinkPeerGetRcvBufSizeTCP,
    cmnLinkPeerSetRcvBufSizeTCP
};


IDE_RC cmnLinkPeerMapTCP(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_TCP);

    /*
     * Shared Pool 획득
     */
    IDE_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != IDE_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerOpTCP;
    sLink->mPeerOp = &gCmnLinkPeerPeerOpTCP;

    /*
     * 멤버 초기화
     */
    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UInt cmnLinkPeerSizeTCP()
{
    return ID_SIZEOF(cmnLinkPeerTCP);
}


#endif
