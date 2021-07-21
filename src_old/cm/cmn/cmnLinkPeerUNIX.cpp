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

#if !defined(CM_DISABLE_UNIX)


typedef struct cmnLinkPeerUNIX
{
    cmnLinkPeer      mLinkPeer;

    cmnLinkDescUNIX  mDesc;

    UInt             mDispatchInfo;

    cmbBlock        *mPendingBlock;
} cmnLinkPeerUNIX;


IDE_RC cmnLinkPeerInitializeUNIX(cmnLink *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * 멤버 초기화
     */
    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    sLink->mDispatchInfo = 0;

    sLink->mPendingBlock = NULL;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFinalizeUNIX(cmnLink *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;
    cmbPool         *sPool = sLink->mLinkPeer.mPool;

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

IDE_RC cmnLinkPeerCloseUNIX(cmnLink *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

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

IDE_RC cmnLinkPeerGetHandleUNIX(cmnLink *aLink, void *aHandle)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * socket을 돌려줌
     */
    *(PDL_SOCKET *)aHandle = sLink->mDesc.mHandle;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetDispatchInfoUNIX(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(UInt *)aDispatchInfo = sLink->mDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetDispatchInfoUNIX(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(UInt *)aDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetBlockingModeUNIX(cmnLinkPeer *aLink, idBool aBlockingMode)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

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

IDE_RC cmnLinkPeerGetInfoUNIX(cmnLinkPeer * /*aLink*/, SChar *aBuf, UInt aBufLen, cmnLinkInfoKey aKey)
{
    SInt sRet;


    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_UNIX_PATH:
            sRet = idlOS::snprintf(aBuf, aBufLen, "UNIX");

            IDE_TEST_RAISE(sRet < 0, StringOutputError);
            IDE_TEST_RAISE((UInt)sRet >= aBufLen, StringTruncated);

            break;

        default:
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(StringOutputError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    IDE_EXCEPTION(StringTruncated);
    {
        IDE_SET(ideSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    IDE_EXCEPTION(UnsupportedLinkInfoKey);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetDescUNIX(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * Desc를 돌려줌
     */
    *(cmnLinkDescUNIX **)aDesc = &sLink->mDesc;

    return IDE_SUCCESS;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmnLinkPeerGetSndBufSizeUNIX(cmnLinkPeer *aLink, SInt *aSndBufSize)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    IDE_TEST(cmnSockGetSndBufSize(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetSndBufSizeUNIX(cmnLinkPeer *aLink, SInt aSndBufSize)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    IDE_TEST(cmnSockSetSndBufSize(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerConnectUNIX(cmnLinkPeer *aLink, cmnLinkConnectArg *aConnectArg, PDL_Time_Value *aTimeout, SInt aOption)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;
    UInt             sPathLen;

    /*
     * socket 생성
     */
    sLink->mDesc.mHandle = idlOS::socket(AF_UNIX, SOCK_STREAM, 0);

    IDE_TEST_RAISE(sLink->mDesc.mHandle == PDL_INVALID_SOCKET, SocketError);

    /*
     * UNIX 파일이름 길이 검사
     */
    sPathLen = idlOS::strlen(aConnectArg->mUNIX.mFilePath);

    IDE_TEST_RAISE(ID_SIZEOF(sLink->mDesc.mAddr.sun_path) <= sPathLen, UnixPathTruncated);

    /*
     * UNIX 주소 세팅
     */
    sLink->mDesc.mAddr.sun_family = AF_UNIX;

    idlOS::snprintf(sLink->mDesc.mAddr.sun_path,
                    ID_SIZEOF(sLink->mDesc.mAddr.sun_path),
                    "%s",
                    aConnectArg->mUNIX.mFilePath);

    /*
     * connect
     */
    IDE_TEST_RAISE(idlVA::connect_timed_wait(sLink->mDesc.mHandle,
                                             (struct sockaddr *)(&sLink->mDesc.mAddr),
                                             ID_SIZEOF(sLink->mDesc.mAddr),
                                             aTimeout) < 0,
                   ConnectError);

    /*
     * socket 초기화
     */
    IDE_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnixPathTruncated);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNIX_PATH_TRUNCATED));
    }
    IDE_EXCEPTION(SocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(ConnectError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_ERROR, errno));
    }
    IDE_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close 해야 합니다
    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        (void)idlOS::closesocket(sLink->mDesc.mHandle);
        sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerAllocChannelUNIX(cmnLinkPeer */*aLink*/, SInt */*aChannelID*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerHandshakeUNIX(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetOptionsUNIX(cmnLinkPeer * /*aLink*/, SInt /*aOption*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownUNIX(cmnLinkPeer *aLink,
                               cmnDirection aDirection,
                               cmnShutdownMode /*aMode*/)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

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

IDE_RC cmnLinkPeerRecvUNIX(cmnLinkPeer *aLink, cmbBlock **aBlock, cmpHeader *aHeader, PDL_Time_Value *aTimeout)
{
    cmnLinkPeerUNIX *sLink  = (cmnLinkPeerUNIX *)aLink;
    cmbPool         *sPool  = aLink->mPool;
    cmbBlock        *sBlock = NULL;
    cmpHeader        sHeader;
    UShort           sPacketSize = 0;
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
    IDE_TEST(cmnSockRecv(sBlock,
                         aLink,
                         sLink->mDesc.mHandle,
                         CMP_HEADER_SIZE,
                         aTimeout,
                         IDV_STAT_INDEX_RECV_UNIX_BYTE) != IDE_SUCCESS);

    /*
     * Protocol Header 해석
     */
    IDE_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != IDE_SUCCESS);
    sPacketSize = sHeader.mA7.mPayloadLength + CMP_HEADER_SIZE;

    /*
     * 패킷 크기 이상 읽음
     */
    IDE_TEST(cmnSockRecv(sBlock,
                         aLink,
                         sLink->mDesc.mHandle,
                         sPacketSize,
                         aTimeout,
                         IDV_STAT_INDEX_RECV_UNIX_BYTE) != IDE_SUCCESS);

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
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSendUNIX(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    /*
     * Block 전송
     */
    IDE_TEST(cmnSockSend(aBlock,
                         aLink,
                         sLink->mDesc.mHandle,
                         NULL,
                         IDV_STAT_INDEX_SEND_UNIX_BYTE) != IDE_SUCCESS);

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

IDE_RC cmnLinkPeerReqCompleteUNIX(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerResCompleteUNIX(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerCheckUNIX(cmnLinkPeer *aLink, idBool *aIsClosed)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    return cmnSockCheck((cmnLink *)aLink, sLink->mDesc.mHandle, aIsClosed);
}

idBool cmnLinkPeerHasPendingRequestUNIX(cmnLinkPeer *aLink)
{
    cmnLinkPeerUNIX *sLink = (cmnLinkPeerUNIX *)aLink;

    return (sLink->mPendingBlock != NULL) ? ID_TRUE : ID_FALSE;
}

IDE_RC cmnLinkPeerAllocBlockUNIX(cmnLinkPeer *aLink, cmbBlock **aBlock)
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

IDE_RC cmnLinkPeerFreeBlockUNIX(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    /*
     * Block 해제
     */
    IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


struct cmnLinkOP gCmnLinkPeerOpUNIX =
{
    "UNIX-PEER",

    cmnLinkPeerInitializeUNIX,
    cmnLinkPeerFinalizeUNIX,

    cmnLinkPeerCloseUNIX,

    cmnLinkPeerGetHandleUNIX,

    cmnLinkPeerGetDispatchInfoUNIX,
    cmnLinkPeerSetDispatchInfoUNIX
};

struct cmnLinkPeerOP gCmnLinkPeerPeerOpUNIX =
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


IDE_RC cmnLinkPeerMapUNIX(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_UNIX);

    /*
     * Shared Pool 획득
     */
    IDE_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != IDE_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerOpUNIX;
    sLink->mPeerOp = &gCmnLinkPeerPeerOpUNIX;

    /*
     * 멤버 초기화
     */
    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UInt cmnLinkPeerSizeUNIX()
{
    return ID_SIZEOF(cmnLinkPeerUNIX);
}


#endif
