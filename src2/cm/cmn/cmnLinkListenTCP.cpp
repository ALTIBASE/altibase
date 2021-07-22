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


typedef struct cmnLinkListenTCP
{
    cmnLinkListen mLinkListen;

    PDL_SOCKET    mHandle;
    UInt          mDispatchInfo;
} cmnLinkListenTCP;


IDE_RC cmnLinkListenInitializeTCP(cmnLink *aLink)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * Handle 초기화
     */
    sLink->mHandle = PDL_INVALID_SOCKET;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenFinalizeTCP(cmnLink *aLink)
{
    /*
     * socket이 열려있으면 닫음
     */
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnLinkListenCloseTCP(cmnLink *aLink)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * socket이 열려있으면 닫음
     */
    if (sLink->mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mHandle);

        sLink->mHandle = PDL_INVALID_SOCKET;
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenGetHandleTCP(cmnLink *aLink, void *aHandle)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * socket 을 돌려줌
     */
    *(PDL_SOCKET *)aHandle = sLink->mHandle;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenGetDispatchInfoTCP(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(UInt *)aDispatchInfo = sLink->mDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenSetDispatchInfoTCP(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenTCP *sLink = (cmnLinkListenTCP *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(UInt *)aDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenListenTCP(cmnLinkListen *aLink, cmnLinkListenArg *aListenArg)
{
    cmnLinkListenTCP    *sLink = (cmnLinkListenTCP *)aLink;
    SInt                 sOption;

    struct addrinfo      sHints;
    struct addrinfo     *sAddr = NULL;
    SInt                 sRet = 0;
    SChar                sPortStr[IDL_IP_PORT_MAX_LEN];
    const SChar          *sErrStr = NULL;
    SChar                sErrMsg[256];

    /*
     * socket이 이미 열려있는지 검사
     */
    IDE_TEST_RAISE(sLink->mHandle != PDL_INVALID_SOCKET, SocketAlreadyOpened);

    /* proj-1538 ipv6 */
    idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo));
    if (aListenArg->mTCP.mIPv6 == NET_CONN_IP_STACK_V4_ONLY)
    {
        sHints.ai_family   = AF_INET;
    }
    else
    {
        sHints.ai_family   = AF_INET6;
    }
    sHints.ai_socktype = SOCK_STREAM;
    sHints.ai_flags    = AI_PASSIVE;

    idlOS::sprintf(sPortStr, "%"ID_UINT32_FMT"", aListenArg->mTCP.mPort);
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
    
    /* create socket */
    sLink->mHandle = idlOS::socket(sAddr->ai_family,
                                   sAddr->ai_socktype,
                                   sAddr->ai_protocol);
    IDE_TEST_RAISE(sLink->mHandle == PDL_INVALID_SOCKET, SocketError);

    /* SO_REUSEADDR */
    sOption = 1;

    if (idlOS::setsockopt(sLink->mHandle,
                          SOL_SOCKET,
                          SO_REUSEADDR,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_REUSEADDR_FAIL, errno);
    }

#if defined(IPV6_V6ONLY)
    if ((sAddr->ai_family == AF_INET6) &&
        (aListenArg->mTCP.mIPv6 == NET_CONN_IP_STACK_V6_ONLY))
    {
        sOption = 1;
        if (idlOS::setsockopt(sLink->mHandle, IPPROTO_IPV6, IPV6_V6ONLY,
                              (SChar *)&sOption, ID_SIZEOF(sOption)) < 0)
        {
            idlOS::sprintf(sErrMsg, "IPV6_V6ONLY: %"ID_INT32_FMT, errno);
            IDE_RAISE(SetSockOptError);
        }
    }
#endif

    /* bind */
    if (idlOS::bind(sLink->mHandle,
                    (struct sockaddr *)sAddr->ai_addr,
                    sAddr->ai_addrlen) < 0)
    {
        // bug-34487: print port no
        ideLog::log(IDE_SERVER_0, CM_TRC_TCP_BIND_FAIL, sPortStr, errno);

        IDE_RAISE(BindError);
    }

    /* listen */
    IDE_TEST_RAISE(idlOS::listen(sLink->mHandle, aListenArg->mTCP.mMaxListen) < 0, ListenError);

    if (sAddr != NULL)
    {
        idlOS::freeaddrinfo(sAddr);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketAlreadyOpened);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_ALREADY_OPENED));
    }
    IDE_EXCEPTION(SocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(BindError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_BIND_ERROR, errno));
    }
    IDE_EXCEPTION(ListenError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_LISTEN_ERROR));
    }
    IDE_EXCEPTION(GetAddrInfoError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    IDE_EXCEPTION(SetSockOptError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    if (sAddr != NULL)
    {
        idlOS::freeaddrinfo(sAddr);
    }
    return IDE_FAILURE;
}

IDE_RC cmnLinkListenAcceptTCP(cmnLinkListen *aLink, cmnLinkPeer **aLinkPeer)
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
    IDE_TEST_RAISE(cmnLinkAlloc((cmnLink **)&sLinkPeer, CMN_LINK_TYPE_PEER_SERVER, CMN_LINK_IMPL_TCP) != IDE_SUCCESS, LinkError);

    /*
     * Desc 획득
     */
    IDE_TEST_RAISE(sLinkPeer->mPeerOp->mGetDesc(sLinkPeer, &sDesc) != IDE_SUCCESS, LinkError);

    /* TASK-3873 5.3.3 Release Static Analysis Code-sonar */
    /* Code-Sonar가 Function  Pointer를 follow하지 못해서..
       assert를 넣었습니다 */
    IDE_ASSERT( sDesc != NULL);
    
    /*
     * accept
     */
    sDesc->mAddrLen = ID_SIZEOF(sDesc->mAddr);

    sDesc->mHandle = idlOS::accept(sLink->mHandle,
                                   (struct sockaddr *)&(sDesc->mAddr),
                                   &sDesc->mAddrLen);

    IDE_TEST_RAISE(sDesc->mHandle == PDL_INVALID_SOCKET, AcceptError);

    /*
     * Link를 돌려줌
     */
    *aLinkPeer = sLinkPeer;

    /*
     * socket 초기화
     */
    IDE_TEST((*aLinkPeer)->mPeerOp->mSetOptions(*aLinkPeer, SO_NONE) != IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(LinkError);
    {
        /* BUG-29957 */
        // bug-33934: codesonar: 3번째인자 NULL 대신 길이 넘기도록 수정
        sTmpDesc.mAddrLen = ID_SIZEOF(sTmpDesc.mAddr);
        sTmpDesc.mHandle = idlOS::accept(sLink->mHandle,
                                         (struct sockaddr *)&(sTmpDesc.mAddr),
                                         &sTmpDesc.mAddrLen);

        if (sTmpDesc.mHandle != PDL_INVALID_SOCKET)
        {
            idlOS::closesocket(sTmpDesc.mHandle);
        }
    }
    IDE_EXCEPTION(AcceptError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_ACCEPT_ERROR));
    }
    IDE_EXCEPTION_END;
    {
        if (sLinkPeer != NULL)
        {
            IDE_ASSERT(cmnLinkFree((cmnLink *)sLinkPeer) == IDE_SUCCESS);
        }

        *aLinkPeer = NULL;
    }

    return IDE_FAILURE;
}


struct cmnLinkOP gCmnLinkListenOpTCP =
{
    "TCP-LISTEN",

    cmnLinkListenInitializeTCP,
    cmnLinkListenFinalizeTCP,

    cmnLinkListenCloseTCP,

    cmnLinkListenGetHandleTCP,

    cmnLinkListenGetDispatchInfoTCP,
    cmnLinkListenSetDispatchInfoTCP
};

struct cmnLinkListenOP gCmnLinkListenListenOpTCP =
{
    cmnLinkListenListenTCP,
    cmnLinkListenAcceptTCP
};


IDE_RC cmnLinkListenMapTCP(cmnLink *aLink)
{
    cmnLinkListen *sLink = (cmnLinkListen *)aLink;

    /*
     * Link 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_LISTEN);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_TCP);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp       = &gCmnLinkListenOpTCP;
    sLink->mListenOp = &gCmnLinkListenListenOpTCP;

    return IDE_SUCCESS;
}

UInt cmnLinkListenSizeTCP()
{
    return ID_SIZEOF(cmnLinkListenTCP);
}


#endif
