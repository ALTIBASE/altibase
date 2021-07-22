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

#if !defined(CM_DISABLE_TCP)


typedef struct cmnLinkListenTCP
{
    cmnLinkListen mLinkListen;
    acp_sock_t    mSocket;
    acp_uint32_t  mDispatchInfo;
} cmnLinkListenTCP;


ACI_RC cmnLinkListenInitializeTCP(cmnLink *aLink)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * Handle 초기화
     */
    sLink->mSocket.mHandle = CMN_INVALID_SOCKET_HANDLE;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenFinalizeTCP(cmnLink *aLink)
{
    /*
     * socket이 열려있으면 닫음
     */
    ACI_TEST(aLink->mOp->mClose(aLink) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkListenCloseTCP(cmnLink *aLink)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * socket이 열려있으면 닫음
     */
    if (sLink->mSocket.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        acpSockClose(&(sLink->mSocket));

        sLink->mSocket.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenGetSocketTCP(cmnLink *aLink, void **aHandle)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * socket 을 돌려줌
     */
    *(acp_sock_t**)aHandle = &(sLink->mSocket);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenGetDispatchInfoTCP(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(acp_uint32_t *)aDispatchInfo = sLink->mDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenSetDispatchInfoTCP(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(acp_uint32_t *)aDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenListenTCP(cmnLinkListen *aLink, cmnLinkListenArg *aListenArg)
{
    cmnLinkListenTCP     *sLink = (cmnLinkListenTCP *)aLink;

    acp_sint32_t          sOption;
    acp_sint32_t          sAddrFamily = 0;
    acp_char_t            sPortStr[ACP_INET_IP_PORT_MAX_LEN];

    acp_rc_t              sRet = 0;
    acp_inet_addr_info_t *sAddr = NULL;
    acp_char_t           *sErrStr = NULL;
    acp_char_t            sErrMsg[256];

    sErrMsg[0] = '\0';
    /* socket이 이미 열려있는지 검사 */
    ACI_TEST_RAISE(sLink->mSocket.mHandle != CMN_INVALID_SOCKET_HANDLE, SocketAlreadyOpened);

    /* *********************************************************
     * proj-1538 ipv6: use getaddrinfo()
     * *********************************************************/
    if (aListenArg->mTCP.mIPv6 == NET_CONN_IP_STACK_V4_ONLY)
    {
        sAddrFamily = ACP_AF_INET;
    }
    else
    {
        sAddrFamily = ACP_AF_INET6;
    }

    acpSnprintf(sPortStr, ACI_SIZEOF(sPortStr),
                "%"ACI_UINT32_FMT"", aListenArg->mTCP.mPort);
    sRet = acpInetGetAddrInfo(&sAddr, NULL, sPortStr,
                              ACP_SOCK_STREAM,
                              ACP_INET_AI_PASSIVE,
                              sAddrFamily);

    if (ACP_RC_NOT_SUCCESS(sRet) || (sAddr == NULL))
    {
        (void) acpInetGetStrError((acp_sint32_t)sRet, &sErrStr);
        if (sErrStr == NULL)
        {
            acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%"ACI_INT32_FMT, sRet);
        }
        else
        {
            acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "%s", sErrStr);
        }
        ACI_RAISE(GetAddrInfoError);
    }
    
    /* create socket */
    sRet = acpSockOpen(&(sLink->mSocket),
                       sAddr->ai_family,
                       sAddr->ai_socktype,
                       sAddr->ai_protocol);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SocketError);

    // BUG-34045 Client's socket listen function can not use reuse address socket option
    sOption = 1;

    if (acpSockSetOpt(&(sLink->mSocket),
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      (acp_sint8_t *)&sOption,
                      ACI_SIZEOF(sOption)) != ACP_RC_SUCCESS)
    {
        acpSnprintf( sErrMsg, ACI_SIZEOF( sErrMsg ), "SO_REUSEADDR: %"ACI_INT32_FMT, errno );
        ACI_RAISE( SetSockOptError );
    }

#if defined(IPV6_V6ONLY)
    if ((sAddr->ai_family == ACP_AF_INET6) &&
        (aListenArg->mTCP.mIPv6 == NET_CONN_IP_STACK_V6_ONLY))
    {
        sOption = 1;
        if (acpSockSetOpt(&(sLink->mSocket),
                          IPPROTO_IPV6,
                          IPV6_V6ONLY,
                          (acp_sint8_t *)&sOption,
                          ACI_SIZEOF(sOption)) != ACP_RC_SUCCESS)
        {
            acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg),
                        "IPV6_V6ONLY: %"ACI_INT32_FMT, errno);
            ACI_RAISE(SetSockOptError);
        }
    }
#endif

    /* bind */
    sRet = acpSockBind(&(sLink->mSocket),
                       (acp_sock_addr_t*)sAddr->ai_addr,
                       sAddr->ai_addrlen,
                       ACP_TRUE);
    ACI_TEST_RAISE(sRet != ACP_RC_SUCCESS, BindError);

    /* listen */
    ACI_TEST_RAISE(acpSockListen(&(sLink->mSocket), aListenArg->mTCP.mMaxListen)
                   != ACP_RC_SUCCESS, ListenError);

    acpInetFreeAddrInfo(sAddr);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SocketAlreadyOpened);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_ALREADY_OPENED));
    }
    ACI_EXCEPTION(SocketError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, sRet));
    }
    ACI_EXCEPTION(BindError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_BIND_ERROR, sRet));
    }
    ACI_EXCEPTION(ListenError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_LISTEN_ERROR));
    }
    ACI_EXCEPTION(GetAddrInfoError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    ACI_EXCEPTION(SetSockOptError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION_END;

    if (sAddr != NULL)
    {
        acpInetFreeAddrInfo(sAddr);
    }
    return ACI_FAILURE;
}

ACI_RC cmnLinkListenAcceptTCP(cmnLinkListen *aLink, cmnLinkPeer **aLinkPeer)
{
    cmnLinkListenTCP  *sLink     = (cmnLinkListenTCP *)aLink;
    cmnLinkPeer       *sLinkPeer = NULL;
    cmnLinkDescTCP    *sDesc;
    cmnLinkDescTCP     sTmpDesc;

    /*
     * 새로운 Link를 할당
     */
   /* BUG-29957
    * cmnLinkAlloc 실패시 Connect를 요청한 Socket을 임시로 accept 해줘야 한다.
    */
    ACI_TEST_RAISE(cmnLinkAlloc((cmnLink **)&sLinkPeer, CMN_LINK_TYPE_PEER_SERVER, CMN_LINK_IMPL_TCP) != ACI_SUCCESS, LinkError);

    /*
     * Desc 획득
     */
    ACI_TEST_RAISE(sLinkPeer->mPeerOp->mGetDesc(sLinkPeer, &sDesc) != ACI_SUCCESS, LinkError);

    /* TASK-3873 5.3.3 Release Static Analysis Code-sonar */
    /* Code-Sonar가 Function  Pointer를 follow하지 못해서..
       assert를 넣었습니다 */
    ACE_ASSERT( sDesc != NULL);

    /*
     * accept
     */
    sDesc->mAddrLen = ACI_SIZEOF(sDesc->mAddr);

    ACI_TEST_RAISE(acpSockAccept(&(sDesc->mSock),
                                 &(sLink->mSocket),
                                 (acp_sock_addr_t*)&(sDesc->mAddr),
                                 &sDesc->mAddrLen) != ACP_RC_SUCCESS,
                   AcceptError);

    /*
     * Link를 돌려줌
     */
    *aLinkPeer = sLinkPeer;

    /*
     * socket 초기화
     */
    ACI_TEST((*aLinkPeer)->mPeerOp->mSetOptions(*aLinkPeer, SO_NONE) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LinkError);
    {
        /* BUG-29957 */
        sTmpDesc.mAddrLen = ACI_SIZEOF(sTmpDesc.mAddr);
        acpSockAccept(&(sTmpDesc.mSock),
                      &(sLink->mSocket),
                      (acp_sock_addr_t*)&(sTmpDesc.mAddr),
                      &sTmpDesc.mAddrLen);

        if (sTmpDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
        {
            acpSockClose(&(sTmpDesc.mSock));
        }
    }
    ACI_EXCEPTION(AcceptError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_ACCEPT_ERROR));
    }
    ACI_EXCEPTION_END;
    {
        if (sLinkPeer != NULL)
        {
            ACE_ASSERT(cmnLinkFree((cmnLink *)sLinkPeer) == ACI_SUCCESS);
        }

        *aLinkPeer = NULL;
    }

    return ACI_FAILURE;
}


struct cmnLinkOP gCmnLinkListenOpTCPClient =
{
    "TCP-LISTEN",

    cmnLinkListenInitializeTCP,
    cmnLinkListenFinalizeTCP,

    cmnLinkListenCloseTCP,

    cmnLinkListenGetSocketTCP,

    cmnLinkListenGetDispatchInfoTCP,
    cmnLinkListenSetDispatchInfoTCP
};

struct cmnLinkListenOP gCmnLinkListenListenOpTCPClient =
{
    cmnLinkListenListenTCP,
    cmnLinkListenAcceptTCP
};


ACI_RC cmnLinkListenMapTCP(cmnLink *aLink)
{
    cmnLinkListen *sLink = (cmnLinkListen *)aLink;

    /*
     * Link 검사
     */
    ACE_ASSERT(aLink->mType == CMN_LINK_TYPE_LISTEN);
    ACE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_TCP);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp       = &gCmnLinkListenOpTCPClient;
    sLink->mListenOp = &gCmnLinkListenListenOpTCPClient;

    return ACI_SUCCESS;
}

acp_uint32_t cmnLinkListenSizeTCP()
{
    return ACI_SIZEOF(cmnLinkListenTCP);
}


#endif
