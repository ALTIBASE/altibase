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

typedef struct cmnLinkListenIB
{
    cmnLinkListen mLinkListen;

    acp_sint32_t  mSocket;
    acp_uint32_t  mDispatchInfo;
    acp_uint32_t  mLatency;    /* for RDMA_LATENCY rsocket option */
    acp_uint32_t  mConChkSpin; /* for RDMA_CONCHKSPIN rsocket option */
} cmnLinkListenIB;

extern cmnIB gIB;

ACI_RC cmnLinkListenInitializeIB(cmnLink *aLink)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    sLink->mSocket = CMN_INVALID_SOCKET_HANDLE;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenFinalizeIB(cmnLink *aLink)
{
    ACI_TEST(aLink->mOp->mClose(aLink) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkListenCloseIB(cmnLink *aLink)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    if (sLink->mSocket != CMN_INVALID_SOCKET_HANDLE)
    {
        (void)gIB.mFuncs.rclose(sLink->mSocket);

        sLink->mSocket = CMN_INVALID_SOCKET_HANDLE;
    }

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenGetSocketIB(cmnLink *aLink, void **aHandle)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    *(acp_sint32_t **)aHandle = &(sLink->mSocket);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenGetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    *(acp_uint32_t *)aDispatchInfo = sLink->mDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenSetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenIB *sLink = (cmnLinkListenIB *)aLink;

    sLink->mDispatchInfo = *(acp_uint32_t *)aDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenListenIB(cmnLinkListen *aLink, cmnLinkListenArg *aListenArg)
{
    cmnLinkListenIB      *sLink = (cmnLinkListenIB *)aLink;

    acp_sint32_t          sOption;
    acp_sint32_t          sAddrFamily = 0;
    acp_char_t            sPortStr[ACP_INET_IP_PORT_MAX_LEN];

    acp_rc_t              sRC = 0;
    acp_inet_addr_info_t *sAddr = NULL;
    acp_char_t           *sErrStr = NULL;
    acp_char_t            sErrMsg[256];

    sErrMsg[0] = '\0';
    /* socket이 이미 열려있는지 검사 */
    ACI_TEST_RAISE(sLink->mSocket != CMN_INVALID_SOCKET_HANDLE, SocketAlreadyOpened);

    /* *********************************************************
     * proj-1538 ipv6: use getaddrinfo()
     * *********************************************************/
    if (aListenArg->mIB.mIPv6 == NET_CONN_IP_STACK_V4_ONLY)
    {
        sAddrFamily = ACP_AF_INET;
    }
    else
    {
        sAddrFamily = ACP_AF_INET6;
    }

    acpSnprintf(sPortStr, ACI_SIZEOF(sPortStr),
                "%"ACI_UINT32_FMT"", aListenArg->mIB.mPort);
    sRC = acpInetGetAddrInfo(&sAddr, NULL, sPortStr,
                             ACP_SOCK_STREAM,
                             ACP_INET_AI_PASSIVE,
                             sAddrFamily);

    if (ACP_RC_NOT_SUCCESS(sRC) || (sAddr == NULL))
    {
        (void) acpInetGetStrError((acp_sint32_t)sRC, &sErrStr);
        if (sErrStr == NULL)
        {
            acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%"ACI_INT32_FMT, sRC);
        }
        else
        {
            acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%s", sErrStr);
        }
        ACI_RAISE(GetAddrInfoError);
    }
    else
    {
        /* nothing to do */
    }
    
    /* create socket */
    sLink->mSocket = gIB.mFuncs.rsocket(sAddr->ai_family,
                                        sAddr->ai_socktype,
                                        sAddr->ai_protocol);
    ACI_TEST_RAISE(sLink->mSocket != CMN_INVALID_SOCKET_HANDLE, SocketError);

    /* SO_REUSEADDR */
    /* BUG-34045 Client's socket listen function can not use reuse address socket option */
    sOption = 1;

    if (gIB.mFuncs.rsetsockopt(sLink->mSocket,
                               SOL_SOCKET,
                               SO_REUSEADDR,
                               &sOption,
                               ACI_SIZEOF(sOption)) < 0)
    {
        acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_REUSEADDR: %"ACI_INT32_FMT, errno);
        ACI_RAISE(SetSockOptError);
    }
    else
    {
        /* nothing to do */
    }

#if defined(IPV6_V6ONLY)
    if ((sAddr->ai_family == ACP_AF_INET6) &&
        (aListenArg->mIB.mIPv6 == NET_CONN_IP_STACK_V6_ONLY))
    {
        sOption = 1;

        if (gIB.mFuncs.rsetsockopt(sLink->mSocket,
                                   IPPROTO_IPV6,
                                   IPV6_V6ONLY,
                                   &sOption,
                                   ACI_SIZEOF(sOption)) < 0)
        {
            acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "IPV6_V6ONLY: %"ACI_INT32_FMT, errno);
            ACI_RAISE(SetSockOptError);
        }
    }
    else
    {
        /* nothing to do */
    }
#endif

    /* bind */
    ACI_TEST_RAISE(gIB.mFuncs.rbind(sLink->mSocket,
                                    (acp_sock_addr_t *)sAddr->ai_addr,
                                    sAddr->ai_addrlen) < 0, BindError);

    /* listen */
    ACI_TEST_RAISE(gIB.mFuncs.rlisten(sLink->mSocket,
                                      aListenArg->mIB.mMaxListen) < 0, ListenError);

    acpInetFreeAddrInfo(sAddr);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SocketAlreadyOpened);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_ALREADY_OPENED));
    }
    ACI_EXCEPTION(SocketError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSOCKET_OPEN_ERROR, errno));
    }
    ACI_EXCEPTION(BindError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RBIND_ERROR, errno));
    }
    ACI_EXCEPTION(ListenError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RLISTEN_ERROR));
    }
    ACI_EXCEPTION(GetAddrInfoError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    ACI_EXCEPTION(SetSockOptError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    if (sAddr != NULL)
    {
        acpInetFreeAddrInfo(sAddr);
    }
    else
    {
        /* nothing to do */
    }

    return ACI_FAILURE;
}

ACI_RC cmnLinkListenAcceptIB(cmnLinkListen *aLink, cmnLinkPeer **aLinkPeer)
{
    cmnLinkListenIB  *sLink     = (cmnLinkListenIB *)aLink;
    cmnLinkPeer      *sLinkPeer = NULL;
    cmnLinkDescIB    *sDesc;
    cmnLinkDescIB     sTmpDesc;

    /*
     * 새로운 Link를 할당
     */
    /* BUG-29957
     * cmnLinkAlloc 실패시 Connect를 요청한 Socket을 임시로 accept 해줘야 한다.
     */
    ACI_TEST_RAISE(cmnLinkAlloc((cmnLink **)&sLinkPeer, CMN_LINK_TYPE_PEER_SERVER, CMN_LINK_IMPL_IB)
                   != ACI_SUCCESS, LinkError);

    /* Desc 획득 */
    ACI_TEST_RAISE(sLinkPeer->mPeerOp->mGetDesc(sLinkPeer, &sDesc) != ACI_SUCCESS, LinkError);

    /* TASK-3873 5.3.3 Release Static Analysis Code-sonar */
    /* Code-Sonar가 Function  Pointer를 follow하지 못해서..
       assert를 넣었습니다 */
    ACE_ASSERT(sDesc != NULL);

    /* accept */
    sDesc->mAddrLen = ACI_SIZEOF(sDesc->mAddr);

    sDesc->mSock = gIB.mFuncs.raccept(sLink->mSocket,
                                      (struct sockaddr *)&(sDesc->mAddr),
                                      (socklen_t *)&sDesc->mAddrLen);

    ACI_TEST_RAISE(sDesc->mSock == CMN_INVALID_SOCKET_HANDLE, AcceptError);

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
    ACI_TEST(sLinkPeer->mPeerOp->mSetOptions(sLinkPeer, SO_NONE) != ACI_SUCCESS);

    *aLinkPeer = sLinkPeer;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LinkError);
    {
        /* BUG-29957 */
        sTmpDesc.mAddrLen = ACI_SIZEOF(sTmpDesc.mAddr);

        sTmpDesc.mSock = gIB.mFuncs.raccept(sLink->mSocket,
                                            (acp_sock_addr_t *)&(sTmpDesc.mAddr),
                                            &sTmpDesc.mAddrLen);

        if (sTmpDesc.mSock != CMN_INVALID_SOCKET_HANDLE)
        {
            (void)gIB.mFuncs.rclose(sTmpDesc.mSock);
        }
        else
        {
            /* nothing to do */
        }
    }
    ACI_EXCEPTION(AcceptError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RACCEPT_ERROR));
    }
    ACI_EXCEPTION_END;

    if (sLinkPeer != NULL)
    {
        ACE_ASSERT(cmnLinkFree((cmnLink *)sLinkPeer) == ACI_SUCCESS);
        sLinkPeer = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return ACI_FAILURE;
}


struct cmnLinkOP gCmnLinkListenOpIBClient =
{
    "IB-LISTEN",

    cmnLinkListenInitializeIB,
    cmnLinkListenFinalizeIB,

    cmnLinkListenCloseIB,

    cmnLinkListenGetSocketIB,

    cmnLinkListenGetDispatchInfoIB,
    cmnLinkListenSetDispatchInfoIB
};

struct cmnLinkListenOP gCmnLinkListenListenOpIBClient =
{
    cmnLinkListenListenIB,
    cmnLinkListenAcceptIB
};

ACI_RC cmnLinkListenMapIB(cmnLink *aLink)
{
    cmnLinkListen *sLink = (cmnLinkListen *)aLink;

    ACE_ASSERT(aLink->mType == CMN_LINK_TYPE_LISTEN);
    ACE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IB);

    aLink->mOp       = &gCmnLinkListenOpIBClient;
    sLink->mListenOp = &gCmnLinkListenListenOpIBClient;

    return ACI_SUCCESS;
}

acp_uint32_t cmnLinkListenSizeIB(void)
{
    return ACI_SIZEOF(cmnLinkListenIB);
}

