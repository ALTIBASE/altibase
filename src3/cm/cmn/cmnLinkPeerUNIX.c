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


typedef struct cmnLinkPeerUNIX
{
    cmnLinkPeer      mLinkPeer;

    cmnLinkDescUNIX  mDesc;

    acp_uint32_t     mDispatchInfo;

    cmbBlock        *mPendingBlock;
} cmnLinkPeerUNIX;


ACI_RC cmnLinkPeerInitializeUNIX(cmnLink *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * 멤버 초기화
     */
    sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    sLink->mDispatchInfo       = 0;

    sLink->mPendingBlock       = NULL;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerFinalizeUNIX(cmnLink *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;
    cmbPool         *sPool = sLink->mLinkPeer.mPool;

    /*
     * socket이 열려있으면 닫음
     */
    ACI_TEST(aLink->mOp->mClose(aLink) != ACI_SUCCESS);

    /*
     * Pending Block이 할당되어 있으면 해제
     */
    if (sLink->mPendingBlock != NULL)
    {
        ACI_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerCloseUNIX(cmnLink *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * socket이 열려있으면 닫음
     */
    if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        ACI_TEST(acpSockClose(&sLink->mDesc.mSock) != ACP_RC_SUCCESS);

        sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetSockUNIX(cmnLink *aLink, void **aSock)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * socket을 돌려줌
     */
    *(acp_sock_t **)aSock = &sLink->mDesc.mSock;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerGetDispatchInfoUNIX(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(acp_uint32_t *)aDispatchInfo = sLink->mDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetDispatchInfoUNIX(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(acp_uint32_t *)aDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetBlockingModeUNIX(cmnLinkPeer *aLink, acp_bool_t aBlockingMode)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * Set Blocking Mode
     */
    ACI_TEST(cmnSockSetBlockingMode(&sLink->mDesc.mSock, aBlockingMode) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetInfoUNIX(cmnLinkPeer * aLink, acp_char_t *aBuf, acp_uint32_t aBufLen, cmnLinkInfoKey aKey)
{
    acp_rc_t  sRet;

    ACP_UNUSED(aLink);

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_UNIX_PATH:
            sRet = acpSnprintf(aBuf, aBufLen, "UNIX");

            ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), StringTruncated);
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), StringOutputError);

            break;

        default:
            ACI_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(StringOutputError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    ACI_EXCEPTION(StringTruncated);
    {
        ACI_SET(aciSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    ACI_EXCEPTION(UnsupportedLinkInfoKey);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetDescUNIX(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * Desc를 돌려줌
     */
    *(cmnLinkDescUNIX **)aDesc = &sLink->mDesc;

    return ACI_SUCCESS;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmnLinkPeerGetSndBufSizeUNIX(cmnLinkPeer *aLink, acp_sint32_t *aSndBufSize)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    ACI_TEST(cmnSockGetSndBufSize(&sLink->mDesc.mSock, aSndBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSetSndBufSizeUNIX(cmnLinkPeer *aLink, acp_sint32_t aSndBufSize)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    ACI_TEST(cmnSockSetSndBufSize(&sLink->mDesc.mSock, aSndBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerConnectUNIX(cmnLinkPeer *aLink, cmnLinkConnectArg *aConnectArg, acp_time_t aTimeout, acp_sint32_t aOption)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;
    acp_rc_t         sRet;

    /*
     * socket 생성
     */
    sRet = acpSockOpen(&sLink->mDesc.mSock,
                       ACP_AF_UNIX,
                       ACP_SOCK_STREAM,
                       0);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SocketError);

    /*
     * UNIX 주소 세팅
     */
    sLink->mDesc.mAddr.sun_family = AF_UNIX;
    sRet = acpSnprintf(sLink->mDesc.mAddr.sun_path,
                       ACI_SIZEOF(sLink->mDesc.mAddr.sun_path),
                       "%s",
                       aConnectArg->mUNIX.mFilePath);

    /*
     * UNIX 파일이름 길이 검사
     */
    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), UnixPathTruncated);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRet));


    /*
     * connect
     */
    sRet = acpSockTimedConnect(&sLink->mDesc.mSock,
                               (acp_sock_addr_t *)(&sLink->mDesc.mAddr),
                               ACI_SIZEOF(sLink->mDesc.mAddr),
                               aTimeout);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ConnectError);

    /*
     * socket 초기화
     */
    ACI_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(UnixPathTruncated);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNIX_PATH_TRUNCATED));
    }
    ACI_EXCEPTION(SocketError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, sRet));
    }
    ACI_EXCEPTION(ConnectError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECT_ERROR, sRet));
    }
    ACI_EXCEPTION_END;

    /*
     * BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close 해야 합니다
     */
    if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        (void)acpSockClose(&sLink->mDesc.mSock);
        sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerAllocChannelUNIX(cmnLinkPeer *aLink, acp_sint32_t *aChannelID)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aChannelID);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerHandshakeUNIX(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetOptionsUNIX(cmnLinkPeer *aLink, acp_sint32_t aOption)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aOption);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerShutdownUNIX(cmnLinkPeer    *aLink,
                               cmnDirection    aDirection,
                               cmnShutdownMode aMode)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;
    acp_rc_t         sRet;

    ACP_UNUSED(aMode);

    /*
     * shutdown
     */
    sRet = acpSockShutdown(&sLink->mDesc.mSock, aDirection);

    switch(sRet)
    {
        case ACP_RC_SUCCESS:
        case ACP_RC_ENOTCONN:
            break;
        default:
            ACI_RAISE(ShutdownError);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ShutdownError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_SHUTDOWN_FAILED));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerRecvUNIX(cmnLinkPeer *aLink, cmbBlock **aBlock, cmpHeader *aHeader, acp_time_t aTimeout)
{
    cmnLinkPeerUNIX *sLink  = (cmnLinkPeerUNIX *)aLink;
    cmbPool         *sPool  = aLink->mPool;
    cmbBlock        *sBlock = NULL;
    cmpHeader        sHeader;
    acp_uint16_t     sPacketSize = 0;
    cmpPacketType    sPacketType = aLink->mLink.mPacketType;

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
            acpMemCpy(sBlock->mData,
                      sLink->mPendingBlock->mData,
                      sLink->mPendingBlock->mDataSize);
            sBlock->mDataSize = sLink->mPendingBlock->mDataSize;

            ACI_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock)
                     != ACI_SUCCESS);
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
            ACI_TEST(sPool->mOp->mAllocBlock(sPool, &sBlock) != ACI_SUCCESS);
        }
    }

    /*
     * Protocol Header Size 크기 이상 읽음
     */
    ACI_TEST(cmnSockRecv(sBlock,
                         aLink,
                         &sLink->mDesc.mSock,
                         CMP_HEADER_SIZE,
                         aTimeout) != ACI_SUCCESS);

    /*
     * Protocol Header 해석
     */
    ACI_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != ACI_SUCCESS);
    sPacketSize = sHeader.mA7.mPayloadLength + CMP_HEADER_SIZE;

    /*
     * 패킷 크기 이상 읽음
     */
    ACI_TEST(cmnSockRecv(sBlock,
                         aLink,
                         &sLink->mDesc.mSock,
                         sPacketSize,
                         aTimeout) != ACI_SUCCESS);

    /*
     * 패킷 크기 이상 읽혔으면 현재 패킷 이후의 데이터를 Pending Block으로 옮김
     */
    if (sBlock->mDataSize > sPacketSize)
    {
        /*
         * Pending Block 할당
         */
        ACI_TEST(sPool->mOp->mAllocBlock(sPool, &sLink->mPendingBlock) != ACI_SUCCESS);

        /*
         * Pending Block으로 데이터 이동
         */
        ACI_TEST(cmbBlockMove(sLink->mPendingBlock, sBlock, sPacketSize) != ACI_SUCCESS);
    }

    /* proj_2160 cm_type removal */
    if (sPacketType == CMP_PACKET_TYPE_A5)
    {
        *aBlock  = sBlock;
    }
    *aHeader = sHeader;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSendUNIX(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * Block 전송
     */
    ACI_TEST(cmnSockSend(aBlock,
                         aLink,
                         &sLink->mDesc.mSock,
                         ACP_TIME_INFINITE) != ACI_SUCCESS);

    /* proj_2160 cm_type removal */
    if (aLink->mLink.mPacketType == CMP_PACKET_TYPE_A5)
    {
        ACI_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerReqCompleteUNIX(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerResCompleteUNIX(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerCheckUNIX(cmnLinkPeer *aLink, acp_bool_t *aIsClosed)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    return cmnSockCheck((cmnLink *)aLink, &sLink->mDesc.mSock, aIsClosed);
}

acp_bool_t cmnLinkPeerHasPendingRequestUNIX(cmnLinkPeer *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    return (sLink->mPendingBlock != NULL) ? ACP_TRUE : ACP_FALSE;
}

ACI_RC cmnLinkPeerAllocBlockUNIX(cmnLinkPeer *aLink, cmbBlock **aBlock)
{
    cmbBlock *sBlock;

    ACI_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != ACI_SUCCESS);

    /*
     * Write Block 초기화
     */
    sBlock->mDataSize = CMP_HEADER_SIZE;
    sBlock->mCursor   = CMP_HEADER_SIZE;

    /*
     * Write Block을 돌려줌
     */
    *aBlock = sBlock;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerFreeBlockUNIX(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    /*
     * Block 해제
     */
    ACI_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}


struct cmnLinkOP gCmnLinkPeerOpUNIXClient =
{
    "UNIX-PEER",

    cmnLinkPeerInitializeUNIX,
    cmnLinkPeerFinalizeUNIX,

    cmnLinkPeerCloseUNIX,

    cmnLinkPeerGetSockUNIX,

    cmnLinkPeerGetDispatchInfoUNIX,
    cmnLinkPeerSetDispatchInfoUNIX
};

struct cmnLinkPeerOP gCmnLinkPeerPeerOpUNIXClient =
{
    cmnLinkPeerSetBlockingModeUNIX,

    cmnLinkPeerGetInfoUNIX,
    cmnLinkPeerGetDescUNIX,

    cmnLinkPeerConnectUNIX,
    cmnLinkPeerSetOptionsUNIX,

    cmnLinkPeerAllocChannelUNIX,
    cmnLinkPeerHandshakeUNIX,

    cmnLinkPeerShutdownUNIX,

    cmnLinkPeerRecvUNIX,
    cmnLinkPeerSendUNIX,

    cmnLinkPeerReqCompleteUNIX,
    cmnLinkPeerResCompleteUNIX,

    cmnLinkPeerCheckUNIX,
    cmnLinkPeerHasPendingRequestUNIX,

    cmnLinkPeerAllocBlockUNIX,
    cmnLinkPeerFreeBlockUNIX,

    /* TASK-5894 Permit sysdba via IPC */
    NULL,

    /* PROJ-2474 SSL/TLS */
    NULL,
    NULL,
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    cmnLinkPeerGetSndBufSizeUNIX,
    cmnLinkPeerSetSndBufSizeUNIX,
    NULL,
    NULL 
};


ACI_RC cmnLinkPeerMapUNIX(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    ACE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);
    ACE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_UNIX);

    /*
     * Shared Pool 획득
     */
    ACI_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != ACI_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerOpUNIXClient;
    sLink->mPeerOp = &gCmnLinkPeerPeerOpUNIXClient;

    /*
     * 멤버 초기화
     */
    sLink->mUserPtr    = NULL;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

acp_uint32_t cmnLinkPeerSizeUNIX()
{
    return ACI_SIZEOF(cmnLinkPeerUNIX);
}


#endif
