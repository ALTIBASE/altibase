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


typedef struct cmnLinkAllocInfo
{
    ACI_RC       (*mMap)(cmnLink *aLink);
    acp_uint32_t (*mSize)();
} cmnLinkAllocInfo;

static cmnLinkAllocInfo gCmnLinkAllocInfoClient[CMN_LINK_IMPL_MAX][CMN_LINK_TYPE_MAX] =
{
    /* DUMMY : client no use */
    {
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
    },

    /* TCP */
    {
#if defined(CM_DISABLE_TCP)
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
#else
        { cmnLinkListenMapTCP, cmnLinkListenSizeTCP },
        { cmnLinkPeerMapTCP,   cmnLinkPeerSizeTCP   },
        { cmnLinkPeerMapTCP,   cmnLinkPeerSizeTCP   },
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
        { NULL,                    NULL                     },
        { NULL,                    NULL                     },
        { cmnLinkPeerClientMapIPC, cmnLinkPeerClientSizeIPC },
#endif
    },

    /* SSL */
    {
#if defined(CM_DISABLE_SSL)
        { NULL, NULL },
        { NULL, NULL }, 
        { NULL, NULL }, 
#else
        { NULL, NULL }, 
        { cmnLinkPeerMapSSL, cmnLinkPeerSizeSSL },
        { cmnLinkPeerMapSSL, cmnLinkPeerSizeSSL },
#endif
    },

    /* IPCDA */
    {
#if defined(CM_DISABLE_IPCDA)
        { NULL, NULL },
        { NULL, NULL },
        { NULL, NULL },
#else
        { NULL,                    NULL                     },
        { NULL,                    NULL                     },
        { cmnLinkPeerClientMapIPCDA, cmnLinkPeerClientSizeIPCDA },
#endif
    },

    /* IB : PROJ-2681 */
    {
        { cmnLinkListenMapIB, cmnLinkListenSizeIB },
        { cmnLinkPeerMapIB,   cmnLinkPeerSizeIB   },
        { cmnLinkPeerMapIB,   cmnLinkPeerSizeIB   },
    }
};

static acp_uint32_t gCmnLinkFeatureClient[CMN_LINK_IMPL_MAX][CMN_LINK_TYPE_MAX] =
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
    { 0, 0, 0 },

    /* IPCDA */
    { 0, 0, 0 },

    /* IB : PROJ-2681 */
    { 0, CMN_LINK_FEATURE_SYSDBA, 0 },
};


acp_bool_t cmnLinkIsSupportedImpl(cmnLinkImpl aImpl)
{
    acp_bool_t sIsSupported = ACP_FALSE;

    switch (aImpl)
    {
#if !defined(CM_DISABLE_TCP)
        case CMN_LINK_IMPL_TCP:
#endif
#if !defined(CM_DISABLE_UNIX)
        case CMN_LINK_IMPL_UNIX:
#endif
#if !defined(CM_DISABLE_IPC)
        case CMN_LINK_IMPL_IPC:
#endif
#if !defined(CM_DISABLE_SSL)
        case CMN_LINK_IMPL_SSL:
#endif
#if !defined(CM_DISABLE_IPCDA)
        case CMN_LINK_IMPL_IPCDA:
#endif
        /* PROJ-2681 */    
        case CMN_LINK_IMPL_IB:
            sIsSupported = ACP_TRUE;
            break;

        default:
            break;
    }

    return sIsSupported;
}

ACI_RC cmnLinkAlloc(cmnLink **aLink, cmnLinkType aType, cmnLinkImpl aImpl)
{
    acp_uint32_t      sImpl = aImpl;
    cmnLinkAllocInfo *sAllocInfo;
    cmnLink          *sLink = NULL;

    /*
     * 지원하는 Impl인지 검사
     */
    ACI_TEST_RAISE(cmnLinkIsSupportedImpl(aImpl) != ACP_TRUE, UnsupportedLinkImpl);

    /*
     * AllocInfo 획득
     */
    sAllocInfo = &gCmnLinkAllocInfoClient[sImpl][aType];

    ACE_ASSERT(sAllocInfo->mMap  != NULL);
    ACE_ASSERT(sAllocInfo->mSize != NULL);

    /*
     * 메모리 할당
     */
    ACI_TEST(acpMemAlloc((void **)&sLink, sAllocInfo->mSize()) != ACP_RC_SUCCESS);

    /*
     * 멤버 초기화
     */
    sLink->mType    = aType;
    sLink->mImpl    = aImpl;
    sLink->mFeature = gCmnLinkFeatureClient[aImpl][aType];
    /* proj_2160 cm_type removal */
    sLink->mPacketType = CMP_PACKET_TYPE_UNKNOWN;

    acpListInitObj(&sLink->mDispatchListNode, sLink);
    acpListInitObj(&sLink->mReadyListNode, sLink);

    /*
     * 함수 포인터 매핑
     */
    ACI_TEST_RAISE(sAllocInfo->mMap(sLink) != ACI_SUCCESS, InitializeFail);

    /*
     * 초기화
     */
    ACI_TEST_RAISE(sLink->mOp->mInitialize(sLink) != ACI_SUCCESS, InitializeFail);

    *aLink = sLink;

    return ACI_SUCCESS;

    ACI_EXCEPTION(UnsupportedLinkImpl);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_IMPL));
    }
    ACI_EXCEPTION(InitializeFail);
    {
        acpMemFree(sLink);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkFree(cmnLink *aLink)
{
    /*
     * Dispatcher에 등록되어 있는 Link인지 검사
     */
    ACE_ASSERT(acpListIsEmpty(&aLink->mDispatchListNode) == ACP_TRUE);

    /*
     * 정리
     */
    ACI_TEST(aLink->mOp->mFinalize(aLink) != ACI_SUCCESS);

    /*
     * 메모리 해제
     */
    acpMemFree(aLink);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
