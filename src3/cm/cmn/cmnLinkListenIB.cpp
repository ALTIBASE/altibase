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

typedef struct cmnLinkListenIB
{
    cmnLinkListen mLinkListen;

    PDL_SOCKET    mHandle;
    UInt          mDispatchInfo;
    UInt          mLatency;    /* for RDMA_LATENCY rsocket option */
    UInt          mConChkSpin; /* for RDMA_CONCHKSPIN rsocket option */
} cmnLinkListenIB;

extern "C" cmnIB gIB;

IDE_RC cmnLinkListenInitializeIB(cmnLink *aLink)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    sLink->mHandle = PDL_INVALID_SOCKET;
    sLink->mLatency = 0;
    sLink->mConChkSpin = 0;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenFinalizeIB(cmnLink *aLink)
{
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkListenCloseIB(cmnLink *aLink)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    if (sLink->mHandle != PDL_INVALID_SOCKET)
    {
        gIB.mFuncs.rclose(sLink->mHandle);

        sLink->mHandle = PDL_INVALID_SOCKET;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenGetHandleIB(cmnLink *aLink, void *aHandle)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    /* socket descriptor */
    *(PDL_SOCKET *)aHandle = sLink->mHandle;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenGetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    *(UInt *)aDispatchInfo = sLink->mDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenSetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    sLink->mDispatchInfo = *(UInt *)aDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenListenIB(cmnLinkListen *aLink, cmnLinkListenArg *aListenArg)
{
    cmnLinkListenIB     *sLink = (cmnLinkListenIB *)aLink;
    SInt                 sOption = 0;

    struct addrinfo      sHints;
    struct addrinfo     *sAddr = NULL;
    SInt                 sRet = 0;
    SChar                sPortStr[IDL_IP_PORT_MAX_LEN];
    const SChar         *sErrStr = NULL;
    SChar                sErrMsg[256];

    /* error, if rsocket was already opened */
    IDE_TEST_RAISE(sLink->mHandle != PDL_INVALID_SOCKET, SocketAlreadyOpened);

    /* proj-1538 ipv6 */
    idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo));
    if (aListenArg->mIB.mIPv6 == NET_CONN_IP_STACK_V4_ONLY)
    {
        sHints.ai_family   = AF_INET;
    }
    else
    {
        sHints.ai_family   = AF_INET6;
    }
    sHints.ai_socktype = SOCK_STREAM;
    sHints.ai_flags    = AI_PASSIVE;

    idlOS::sprintf(sPortStr, "%"ID_UINT32_FMT"", aListenArg->mIB.mPort);
    sRet = idlOS::getaddrinfo(NULL, sPortStr, &sHints, &sAddr);
    if ((sRet != 0) || (sAddr == NULL))
    {
        sErrStr = idlOS::gai_strerror(sRet);
        if (sErrStr == NULL)
        {
            idlOS::sprintf(sErrMsg, "%"ID_INT32_FMT, sRet);
        }
        else
        {
            idlOS::sprintf(sErrMsg, "%s", sErrStr);
        }
        IDE_RAISE(GetAddrInfoError);
    }
    else
    {
        /* nothing to do */
    }
    
    /* create rsocket */
    sLink->mHandle = gIB.mFuncs.rsocket(sAddr->ai_family,
                                        sAddr->ai_socktype,
                                        sAddr->ai_protocol);
    IDE_TEST_RAISE(sLink->mHandle == PDL_INVALID_SOCKET, SocketError);

    /* SO_REUSEADDR */
    sOption = 1;

    if (gIB.mFuncs.rsetsockopt(sLink->mHandle,
                               SOL_SOCKET,
                               SO_REUSEADDR,
                               &sOption,
                               ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_IB_RSOCKET_REUSEADDR_FAIL, errno);
    }
    else
    {
        /* nothing to do */
    }

    /* rsocket option */
    sLink->mLatency = aListenArg->mIB.mLatency;
    sLink->mConChkSpin = aListenArg->mIB.mConChkSpin;

#if defined(IPV6_V6ONLY)
    if ((sAddr->ai_family == AF_INET6) &&
        (aListenArg->mIB.mIPv6 == NET_CONN_IP_STACK_V6_ONLY))
    {
        sOption = 1;

        if (gIB.mFuncs.rsetsockopt(sLink->mHandle,
                                   IPPROTO_IPV6,
                                   IPV6_V6ONLY,
                                   &sOption,
                                   ID_SIZEOF(sOption)) < 0)
        {
            idlOS::sprintf(sErrMsg, "IPV6_V6ONLY: %"ID_INT32_FMT, errno);
            IDE_RAISE(SetSockOptError);
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
#endif

    /* bind */
    if (gIB.mFuncs.rbind(sLink->mHandle,
                         (struct sockaddr *)sAddr->ai_addr,
                         sAddr->ai_addrlen) < 0)
    {
        // bug-34487: print port no
        ideLog::log(IDE_SERVER_0, CM_TRC_IB_RBIND_FAIL, sPortStr, errno);

        IDE_RAISE(BindError);
    }
    else
    {
        /* nothing to do */
    }

    /* listen */
    IDE_TEST_RAISE(gIB.mFuncs.rlisten(sLink->mHandle,
                                      aListenArg->mIB.mMaxListen) < 0, ListenError);

    (void)idlOS::freeaddrinfo(sAddr);
    sAddr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketAlreadyOpened)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSOCKET_ALREADY_OPENED));
    }
    IDE_EXCEPTION(SocketError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(BindError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RBIND_ERROR, errno));
    }
    IDE_EXCEPTION(ListenError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RLISTEN_ERROR));
    }
    IDE_EXCEPTION(GetAddrInfoError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    IDE_EXCEPTION(SetSockOptError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    if (sAddr != NULL)
    {
        (void)idlOS::freeaddrinfo(sAddr);
        sAddr = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkListenAcceptIB(cmnLinkListen *aLink, cmnLinkPeer **aLinkPeer)
{
    cmnLinkListenIB  *sLink     = (cmnLinkListenIB *)aLink;
    cmnLinkPeer      *sLinkPeer = NULL;
    cmnLinkDescIB    *sDesc;
    cmnLinkDescIB     sTmpDesc;

    /* allocate IB link data structure */
    /* BUG-29957
     * cmnLinkAlloc 실패시 Connect를 요청한 Socket을 임시로 accept 해줘야 한다.
     */
    IDE_TEST_RAISE(cmnLinkAlloc((cmnLink **)&sLinkPeer, CMN_LINK_TYPE_PEER_SERVER, CMN_LINK_IMPL_IB)
                   != IDE_SUCCESS, LinkError);

    /* Desc 획득 */
    IDE_TEST_RAISE(sLinkPeer->mPeerOp->mGetDesc(sLinkPeer, &sDesc) != IDE_SUCCESS, LinkError);

    /* TASK-3873 5.3.3 Release Static Analysis Code-sonar */
    /* Code-Sonar가 Function  Pointer를 follow하지 못해서..
       assert를 넣었습니다 */
    IDE_ASSERT(sDesc != NULL);
    
    /* accept */
    sDesc->mAddrLen = ID_SIZEOF(sDesc->mAddr);

    sDesc->mHandle = gIB.mFuncs.raccept(sLink->mHandle,
                                        (struct sockaddr *)&(sDesc->mAddr),
                                        (socklen_t *)&sDesc->mAddrLen);

    IDE_TEST_RAISE(sDesc->mHandle == PDL_INVALID_SOCKET, AcceptError);

    /**
     * [CAUTION]
     *
     * Berkeley socket 과 다르게 blocking mode 이더라도
     * raccept() 함수는 'accpeting' 상태로 반환될 수 있음.
     */

    /* for rsocket option */
    sDesc->mLatency = sLink->mLatency;
    sDesc->mConChkSpin = sLink->mConChkSpin;

    /* set socket options */
    IDE_TEST(sLinkPeer->mPeerOp->mSetOptions(sLinkPeer, SO_NONE) != IDE_SUCCESS);

    *aLinkPeer = sLinkPeer;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(LinkError);
    {
        /* BUG-29957 */
        // bug-33934: codesonar: 3번째인자 NULL 대신 길이 넘기도록 수정
        sTmpDesc.mAddrLen = ID_SIZEOF(sTmpDesc.mAddr);

        sTmpDesc.mHandle = gIB.mFuncs.raccept(sLink->mHandle,
                                              (struct sockaddr *)&(sTmpDesc.mAddr),
                                              (socklen_t *)&sTmpDesc.mAddrLen);

        if (sTmpDesc.mHandle != PDL_INVALID_SOCKET)
        {
            (void)gIB.mFuncs.rclose(sTmpDesc.mHandle);
        }
        else
        {
            /* nothing to do */
        }
    }
    IDE_EXCEPTION(AcceptError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RACCEPT_ERROR));
    }
    IDE_EXCEPTION_END;

    if (sLinkPeer != NULL)
    {
        IDE_ASSERT(cmnLinkFree((cmnLink *)sLinkPeer) == IDE_SUCCESS);
        sLinkPeer = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

struct cmnLinkOP gCmnLinkListenOpIB =
{
    "IB-LISTEN",

    cmnLinkListenInitializeIB,
    cmnLinkListenFinalizeIB,

    cmnLinkListenCloseIB,

    cmnLinkListenGetHandleIB,

    cmnLinkListenGetDispatchInfoIB,
    cmnLinkListenSetDispatchInfoIB
};

struct cmnLinkListenOP gCmnLinkListenListenOpIB =
{
    cmnLinkListenListenIB,
    cmnLinkListenAcceptIB
};


IDE_RC cmnLinkListenMapIB(cmnLink *aLink)
{
    cmnLinkListen *sLink = (cmnLinkListen *)aLink;

    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_LISTEN);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IB);

    aLink->mOp       = &gCmnLinkListenOpIB;
    sLink->mListenOp = &gCmnLinkListenListenOpIB;

    return IDE_SUCCESS;
}

UInt cmnLinkListenSizeIB(void)
{
    return ID_SIZEOF(cmnLinkListenIB);
}

