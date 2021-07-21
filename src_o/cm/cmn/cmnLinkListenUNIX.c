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

#if !defined(CM_DISABLE_UNIX)


typedef struct cmnLinkListenUNIX
{
    cmnLinkListen mLinkListen;
    acp_sock_t    mSocket;
    acp_uint32_t  mDispatchInfo;
} cmnLinkListenUNIX;


ACI_RC cmnLinkListenInitializeUNIX(cmnLink *aLink)
{
    cmnLinkListenUNIX *sLink = (cmnLinkListenUNIX *)aLink;

    /*
     * Handle 초기화
     */
    sLink->mSocket.mHandle = CMN_INVALID_SOCKET_HANDLE;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenFinalizeUNIX(cmnLink *aLink)
{
    /*
     * socket이 열려있으면 닫음
     */
    ACI_TEST(aLink->mOp->mClose(aLink) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkListenCloseUNIX(cmnLink *aLink)
{
    cmnLinkListenUNIX *sLink = (cmnLinkListenUNIX *)aLink;

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

ACI_RC cmnLinkListenGetSocketUNIX(cmnLink *aLink, void **aHandle)
{
    cmnLinkListenUNIX *sLink = (cmnLinkListenUNIX *)aLink;

    /*
     * socket 을 돌려줌
     */
    *(acp_sock_t **)aHandle = &(sLink->mSocket);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenGetDispatchInfoUNIX(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenUNIX *sLink = (cmnLinkListenUNIX *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(acp_uint32_t *)aDispatchInfo = sLink->mDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenSetDispatchInfoUNIX(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenUNIX *sLink = (cmnLinkListenUNIX *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(acp_uint32_t *)aDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkListenListenUNIX(cmnLinkListen *aLink, cmnLinkListenArg *aListenArg)
{
    cmnLinkListenUNIX  *sLink = (cmnLinkListenUNIX *)aLink;
    acp_sint32_t        sOption;
    acp_uint32_t        sPathLen;
    acp_sock_addr_un_t  sAddr;
    acp_char_t          sErrMsg[256];
    acp_rc_t            sRet;

    /*
     * socket이 이미 열려있는지 검사
     */
    ACI_TEST_RAISE(sLink->mSocket.mHandle != CMN_INVALID_SOCKET_HANDLE, SocketAlreadyOpened);

    /*
     * UNIX 파일이름 길이 검사
     */
    sPathLen = acpCStrLen(aListenArg->mUNIX.mFilePath, ACP_SINT32_MAX);

    ACI_TEST_RAISE(ACI_SIZEOF(sAddr.sun_path) <= sPathLen, UnixPathTruncated);

    /*
     * socket 생성
     */
    sRet = acpSockOpen(&(sLink->mSocket), AF_UNIX, SOCK_STREAM, 0);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SocketError);

    // BUG-34045 Client's socket listen function can not use reuse address socket option
    sOption = 1;

    if (acpSockSetOpt(&(sLink->mSocket),
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      (acp_sint8_t *)&sOption,
                      ACI_SIZEOF(sOption)) != ACP_RC_SUCCESS) 
    {
        acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_REUSEADDR: %"ACI_INT32_FMT, errno);
        ACI_RAISE(SetSockOptError);
    }

    /*
     * UNIX 파일이름 생성
     */
    acpSnprintf(sAddr.sun_path,
                ACI_SIZEOF(sAddr.sun_path),
                "%s",
                aListenArg->mUNIX.mFilePath);
    /*
     * UNIX 파일 삭제
     */
    acpFileRemove(sAddr.sun_path);

    /*
     * bind
     */
    sAddr.sun_family = AF_UNIX;

    sRet = acpSockBind(&(sLink->mSocket),
                       (acp_sock_addr_t *)&sAddr,
                       ACI_SIZEOF(sAddr),
                       ACP_TRUE);
    ACI_TEST_RAISE(sRet != ACP_RC_SUCCESS, BindError);

    /*
     * listen
     */
    ACI_TEST_RAISE(acpSockListen(&(sLink->mSocket), aListenArg->mUNIX.mMaxListen)
                   != ACP_RC_SUCCESS, ListenError);

    return ACI_SUCCESS;

    ACI_EXCEPTION(UnixPathTruncated);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNIX_PATH_TRUNCATED));
    }
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
    ACI_EXCEPTION( SetSockOptError );
    {
        ACI_SET( aciSetErrorCode( cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg ) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkListenAcceptUNIX(cmnLinkListen *aLink, cmnLinkPeer **aLinkPeer)
{
    cmnLinkListenUNIX  *sLink     = (cmnLinkListenUNIX *)aLink;
    cmnLinkPeer        *sLinkPeer = NULL;
    cmnLinkDescUNIX    *sDesc;
    acp_sock_len_t      sAddrLen;
    cmnLinkDescUNIX     sTmpDesc;

    /*
     * 새로운 Link를 할당
     */
   /* BUG-29957
    * cmnLinkAlloc 실패시 Connect를 요청한 Socket을 임시로 accept 해줘야 한다.
   */
    ACI_TEST_RAISE(cmnLinkAlloc((cmnLink **)&sLinkPeer, CMN_LINK_TYPE_PEER_SERVER, CMN_LINK_IMPL_UNIX) != ACI_SUCCESS, LinkError);

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
    sAddrLen = ACI_SIZEOF(sDesc->mAddr);

    ACI_TEST_RAISE(acpSockAccept(&(sDesc->mSock),
                                 &(sLink->mSocket),
                                 (acp_sock_addr_t *)&(sDesc->mAddr),
                                 &sAddrLen) != ACP_RC_SUCCESS,
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
        sAddrLen = ACI_SIZEOF(sTmpDesc.mAddr);
        acpSockAccept(&(sTmpDesc.mSock),
                      &(sLink->mSocket),
                      (acp_sock_addr_t*)&(sTmpDesc.mAddr),
                      &sAddrLen);

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


struct cmnLinkOP gCmnLinkListenOpUNIXClient =
{
    "UNIX-LISTEN",

    cmnLinkListenInitializeUNIX,
    cmnLinkListenFinalizeUNIX,

    cmnLinkListenCloseUNIX,

    cmnLinkListenGetSocketUNIX,

    cmnLinkListenGetDispatchInfoUNIX,
    cmnLinkListenSetDispatchInfoUNIX
};

struct cmnLinkListenOP gCmnLinkListenListenOpUNIXClient =
{
    cmnLinkListenListenUNIX,
    cmnLinkListenAcceptUNIX
};


ACI_RC cmnLinkListenMapUNIX(cmnLink *aLink)
{
    cmnLinkListen *sLink = (cmnLinkListen *)aLink;

    /*
     * Link 검사
     */
    ACE_ASSERT(aLink->mType == CMN_LINK_TYPE_LISTEN);
    ACE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_UNIX);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp       = &gCmnLinkListenOpUNIXClient;
    sLink->mListenOp = &gCmnLinkListenListenOpUNIXClient;

    return ACI_SUCCESS;
}

acp_uint32_t cmnLinkListenSizeUNIX()
{
    return ACI_SIZEOF(cmnLinkListenUNIX);
}


#endif
