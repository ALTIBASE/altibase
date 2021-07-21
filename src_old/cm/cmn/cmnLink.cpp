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


typedef struct cmnLinkAllocInfo
{
    IDE_RC (*mMap)(cmnLink *aLink);
    UInt   (*mSize)();
} cmnLinkAllocInfo;

static cmnLinkAllocInfo gCmnLinkAllocInfo[CMN_LINK_IMPL_MAX][CMN_LINK_TYPE_MAX] =
{
    /* DUMMY */
    {
        { NULL, NULL },
        { cmnLinkPeerServerMapDUMMY,  cmnLinkPeerServerSizeDUMMY },
        { NULL, NULL },
    },

    /* TCP */
    {
#if defined(CM_DISABLE_TCP)
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
#else
        { cmnLinkListenMapTCP,  cmnLinkListenSizeTCP  },
        { cmnLinkPeerMapTCP,    cmnLinkPeerSizeTCP    },
        { cmnLinkPeerMapTCP,    cmnLinkPeerSizeTCP    },
#endif
    },

    /* UNIX */
    {
#if defined(CM_DISABLE_UNIX)
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
#else
        { cmnLinkListenMapUNIX, cmnLinkListenSizeUNIX },
        { cmnLinkPeerMapUNIX,   cmnLinkPeerSizeUNIX   },
        { cmnLinkPeerMapUNIX,   cmnLinkPeerSizeUNIX   },
#endif
    },

    /* IPC */
    {
#if defined(CM_DISABLE_IPC)
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
#else
        { cmnLinkListenMapIPC,      cmnLinkListenSizeIPC     },
        { cmnLinkPeerServerMapIPC,  cmnLinkPeerServerSizeIPC },
        { cmnLinkPeerClientMapIPC,  cmnLinkPeerClientSizeIPC },
#endif
    },

    /* PROJ-2474 SSL/TLS */
    {
#if defined(CM_DISABLE_SSL)
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
#else
        { cmnLinkListenMapSSL, cmnLinkListenSizeSSL },
        { cmnLinkPeerMapSSL,   cmnLinkPeerSizeSSL   },
        { cmnLinkPeerMapSSL,   cmnLinkPeerSizeSSL   },
#endif
    },

    {
#if defined(CM_DISABLE_IPCDA)
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
#else
        { cmnLinkListenMapIPCDA, cmnLinkListenSizeIPCDA },
        { cmnLinkPeerServerMapIPCDA,  cmnLinkPeerServerSizeIPCDA },
        { cmnLinkPeerClientMapIPCDA,  cmnLinkPeerClientSizeIPCDA },
#endif
    },

    /* PROJ-2681 */
    {
        { cmnLinkListenMapIB, cmnLinkListenSizeIB },
        { cmnLinkPeerMapIB,   cmnLinkPeerSizeIB   },
        { cmnLinkPeerMapIB,   cmnLinkPeerSizeIB   },
    },
};

static UInt gCmnLinkFeature[CMN_LINK_IMPL_MAX][CMN_LINK_TYPE_MAX] =
{
    // bug-19279 remote sysdba enable + sys can kill session
    // 이 값이 사용되는 부분:
    // mmtServiceThread::connectProtocol -> mmcTask::authenticate()
    // 위의 함수에서 client가 sysdba로 접속한 경우 서버 task의 link가
    // CMN_LINK_FEATURE_SYSDBA 특성을 가져야만 접속이 허용된다.
    // 변경 내용:
    // task 생성시 기존에는 tcp link의 경우 sysdba특성이 없었는데
    // 원격 sysdba  접속을 허용하기 위해 모든 tcp link에 대해
    // sysdba특성을 부여한다
    // => 결국 서버 task의 link에 대한 sysdba 특성 필드가 무의미해 진다.
    // listen,   server,    client

    /* DUMMY */
    { 0, 0, 0 },

    /* TCP */
    { 0, CMN_LINK_FEATURE_SYSDBA, 0 },

    /* UNIX */
    { 0, CMN_LINK_FEATURE_SYSDBA, 0 },

    /* IPC */
    /* TASK-5894 Permit sysdba via IPC */
    { 0, CMN_LINK_FEATURE_SYSDBA, 0 },

    /* PROJ-2474 SSL/TLS */
    { 0, CMN_LINK_FEATURE_SYSDBA, 0 },

    /* IPCDA */
    { 0, 0, 0 },

    /* IB, PROJ-2681 */
    { 0, CMN_LINK_FEATURE_SYSDBA, 0 },
};


idBool cmnLinkIsSupportedImpl(cmnLinkImpl aImpl)
{
    idBool sIsSupported = ID_FALSE;

    switch (aImpl)
    {
        case CMN_LINK_IMPL_DUMMY:
#if !defined(CM_DISABLE_TCP)
        case CMN_LINK_IMPL_TCP:
#endif
#if !defined(CM_DISABLE_UNIX)
        case CMN_LINK_IMPL_UNIX:
#endif
#if !defined(CM_DISABLE_IPC)
        case CMN_LINK_IMPL_IPC:
#endif
/* PROJ-2474 SSL/TLS */
#if !defined(CM_DISABLE_SSL)
        case CMN_LINK_IMPL_SSL:
#endif
#if !defined(CM_DISABLE_IPCDA)
        case CMN_LINK_IMPL_IPCDA:
#endif
        /* PROJ-2681 */
        case CMN_LINK_IMPL_IB:
            sIsSupported = ID_TRUE;
            break;

        default:
            break;
    }

    return sIsSupported;
}

IDE_RC cmnLinkAlloc(cmnLink **aLink, cmnLinkType aType, cmnLinkImpl aImpl)
{
    UInt sImpl = aImpl;
    cmnLinkAllocInfo *sAllocInfo;

    /*
     * 지원하는 Impl인지 검사
     */
    IDE_TEST_RAISE(cmnLinkIsSupportedImpl(aImpl) != ID_TRUE, UnsupportedLinkImpl);

    /*
     * AllocInfo 획득
     */
    sAllocInfo = &gCmnLinkAllocInfo[sImpl][aType];

    IDE_ASSERT(sAllocInfo->mMap  != NULL);
    IDE_ASSERT(sAllocInfo->mSize != NULL);

    IDU_FIT_POINT_RAISE( "cmnLink::cmnLinkAlloc::malloc::Link",
                          InsufficientMemory );

    /*
     * 메모리 할당
     */
     
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMN,
                                     sAllocInfo->mSize(),
                                     (void **)aLink,
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

    /*
     * 멤버 초기화
     */
    (*aLink)->mType    = aType;
    (*aLink)->mImpl    = aImpl;
    (*aLink)->mFeature = gCmnLinkFeature[aImpl][aType];
    /* proj_2160 cm_type removal */
    /* this is why mPacketType exists in cmnLink, not in cmnLinkPeer */
    (*aLink)->mPacketType = CMP_PACKET_TYPE_UNKNOWN;

    IDU_LIST_INIT_OBJ(&(*aLink)->mDispatchListNode, *aLink);
    IDU_LIST_INIT_OBJ(&(*aLink)->mReadyListNode, *aLink);

    /*
     * 함수 포인터 매핑
     */
    IDE_TEST_RAISE(sAllocInfo->mMap(*aLink) != IDE_SUCCESS, InitializeFail);

    /*
     * 초기화
     */
    IDE_TEST_RAISE((*aLink)->mOp->mInitialize(*aLink) != IDE_SUCCESS, InitializeFail);

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnsupportedLinkImpl);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_IMPL));
    }
    IDE_EXCEPTION(InitializeFail);
    {
        IDE_ASSERT(iduMemMgr::free(*aLink) == IDE_SUCCESS);
        *aLink = NULL;
    }
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkFree(cmnLink *aLink)
{
    /*
     * Dispatcher에 등록되어 있는 Link인지 검사
     */
    IDE_ASSERT(IDU_LIST_IS_EMPTY(&aLink->mDispatchListNode) == ID_TRUE);

    /*
     * 정리
     */
    IDE_TEST(aLink->mOp->mFinalize(aLink) != IDE_SUCCESS);

    /*
     * 메모리 해제
     */
    IDE_TEST(iduMemMgr::free(aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

