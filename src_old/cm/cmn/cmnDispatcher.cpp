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


typedef struct cmnDispatcherAllocInfo
{
    IDE_RC (*mMap)(cmnDispatcher *aDispatcher);
    UInt   (*mSize)();
} cmnDispatcherAllocInfo;


static cmnDispatcherAllocInfo gCmnDispatcherAllocInfo[CMN_DISPATCHER_IMPL_MAX] =
{
    {
#if defined(CM_DISABLE_TCP) && defined(CM_DISABLE_UNIX)
        NULL,
        NULL
#else
        cmnDispatcherMapSOCKSelect,
        cmnDispatcherSizeSOCKSelect
#endif
    },

    {
#if defined(CM_DISABLE_IPC)
        NULL,
        NULL
#else
        cmnDispatcherMapIPC,
        cmnDispatcherSizeIPC
#endif
    },

    {
#if defined(CM_DISABLE_IPCDA)
        NULL,
        NULL
#else
        cmnDispatcherMapIPCDA,
        cmnDispatcherSizeIPCDA
#endif
    },
    /* PROJ-2681 */
    {
        cmnDispatcherMapIB,
        cmnDispatcherSizeIB
    }
};


static IDE_RC (*gCmnDispatcherWaitLink[CMN_DISPATCHER_IMPL_MAX])(cmnLink *aLink,
                                                                 cmnDirection aDirection,
                                                                 PDL_Time_Value *aTimeout) =
{
#if defined(CM_DISABLE_TCP) && defined(CM_DISABLE_UNIX)
    NULL,
#else
    cmnDispatcherWaitLinkSOCKSelect,
#endif

#if defined(CM_DISABLE_IPC)
    NULL,
#else
    cmnDispatcherWaitLinkIPC,
#endif

#if defined(CM_DISABLE_IPCDA)
    NULL,
#else
    cmnDispatcherWaitLinkIPCDA,
#endif
    /* PROJ-2681 */
    cmnDispatcherWaitLinkIB
};


/* BUG-45240 */
SInt (*gCmnDispatcherCheckHandle)(PDL_SOCKET aHandle, PDL_Time_Value *aTimeout) =
#if defined(CM_DISABLE_TCP) && defined(CM_DISABLE_UNIX)
    NULL;
#else
    cmnDispatcherCheckHandleSOCKSelect;
#endif


idBool cmnDispatcherIsSupportedImpl(cmnDispatcherImpl aImpl)
{
    switch (aImpl)
    {
#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)
        case CMN_DISPATCHER_IMPL_SOCK:
            return ID_TRUE;
#endif

#if !defined(CM_DISABLE_IPC)
        case CMN_DISPATCHER_IMPL_IPC:
            return ID_TRUE;
#endif

#if !defined(CM_DISABLE_IPCDA)
        case CMN_DISPATCHER_IMPL_IPCDA:
            return ID_TRUE;
#endif

        /* PROJ-2681 */
        case CMN_DISPATCHER_IMPL_IB:
            return ID_TRUE;

        default:
            break;
    }

    return ID_FALSE;
}


/**
 * cmnDispatcherInitialize
 *
 * SockType에 따라 Select, Poll, Epoll에 맞는 함수를 설정한다.
 */
IDE_RC cmnDispatcherInitialize()
{
#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)
    /* BUG-38951 Support to choice a type of CM dispatcher on run-time */
    cmnDispatcherSockPollType sSockPollType = CMN_DISPATCHER_SOCK_INVALID;
    cmnDispatcherImpl         sImpl         = CMN_DISPATCHER_IMPL_SOCK;

    sSockPollType = (cmnDispatcherSockPollType) cmuProperty::getCmDispatcherSockPollType();

    switch (sSockPollType)
    {
        case CMN_DISPATCHER_SOCK_SELECT:
            ideLog::log(IDE_SERVER_0, "cmnDispatcherInitialize: SOCK-SELECT");
            break;

        case CMN_DISPATCHER_SOCK_POLL:
            ideLog::log(IDE_SERVER_0, "cmnDispatcherInitialize: SOCK-POLL");

            gCmnDispatcherAllocInfo[sImpl].mMap  = cmnDispatcherMapSOCKPoll;
            gCmnDispatcherAllocInfo[sImpl].mSize = cmnDispatcherSizeSOCKPoll;
            gCmnDispatcherWaitLink[sImpl]        = cmnDispatcherWaitLinkSOCKPoll;
            gCmnDispatcherCheckHandle            = cmnDispatcherCheckHandleSOCKPoll;
            break;

        case CMN_DISPATCHER_SOCK_EPOLL:  /* BUG-45240 */
            ideLog::log(IDE_SERVER_0, "cmnDispatcherInitialize: SOCK-EPOLL");

            gCmnDispatcherAllocInfo[sImpl].mMap  = cmnDispatcherMapSOCKEpoll;
            gCmnDispatcherAllocInfo[sImpl].mSize = cmnDispatcherSizeSOCKEpoll;
            /* 효율성을 위해 Poll 사용 */
            gCmnDispatcherWaitLink[sImpl]        = cmnDispatcherWaitLinkSOCKPoll;
            gCmnDispatcherCheckHandle            = cmnDispatcherCheckHandleSOCKPoll;
            break;

        /* Non-reachable */
        default:
            ideLog::log(IDE_SERVER_0, "cmnDispatcherInitialize: SOCK-INVALID");
            IDE_TEST(1);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#else
    return IDE_SUCCESS;
#endif  /* !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX) */
}

IDE_RC cmnDispatcherAlloc(cmnDispatcher **aDispatcher, cmnDispatcherImpl aImpl, UInt aMaxLink)
{
    cmnDispatcherAllocInfo *sAllocInfo;

    /*
     * 지원하는 Impl인지 검사
     */
    IDE_TEST_RAISE(cmnDispatcherIsSupportedImpl(aImpl) != ID_TRUE, UnsupportedDispatcherImpl);

    /*
     * AllocInfo 획득
     */
    sAllocInfo = &gCmnDispatcherAllocInfo[aImpl];

    IDE_ASSERT(sAllocInfo->mMap  != NULL);
    IDE_ASSERT(sAllocInfo->mSize != NULL);

    IDU_FIT_POINT_RAISE( "cmnDispatcher::cmnDispatcherAlloc::malloc::Dispatcher",
                          InsufficientMemory );

    /*
     * 메모리 할당
     */
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMN,
                                     sAllocInfo->mSize(),
                                     (void **)aDispatcher,
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

    /*
     * 멤버 초기화
     */
    (*aDispatcher)->mImpl      = aImpl;
    (*aDispatcher)->mLinkCount = 0;

    IDU_LIST_INIT(&(*aDispatcher)->mLinkList);

    /*
     * 함수 포인터 매핑
     */
    IDE_TEST_RAISE(sAllocInfo->mMap(*aDispatcher) != IDE_SUCCESS, InitializeFail);

    /*
     * 초기화
     */
    IDE_TEST_RAISE((*aDispatcher)->mOp->mInitialize(*aDispatcher, aMaxLink) != IDE_SUCCESS, InitializeFail);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION(UnsupportedDispatcherImpl);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_DISPATCHER_IMPL));
    }
    IDE_EXCEPTION(InitializeFail);
    {
        IDE_ASSERT(iduMemMgr::free(*aDispatcher) == IDE_SUCCESS);
        *aDispatcher = NULL;  /* BUG-45240 */
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherFree(cmnDispatcher *aDispatcher)
{
    /*
     * 모든 Link 삭제
     */
    IDE_TEST(cmnDispatcherRemoveAllLinks(aDispatcher) != IDE_SUCCESS);

    /*
     * 정리
     */
    IDE_TEST(aDispatcher->mOp->mFinalize(aDispatcher) != IDE_SUCCESS);

    /*
     * 메모리 해제
     */
    IDE_TEST(iduMemMgr::free(aDispatcher) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

cmnDispatcherImpl cmnDispatcherImplForLinkImpl(cmnLinkImpl aLinkImpl)
{
    /*
     * Link Impl에 따른 Dispatcher Impl 반환
     */
    switch (aLinkImpl)
    {
        case CMN_LINK_IMPL_TCP:
            return CMN_DISPATCHER_IMPL_SOCK;

        case CMN_LINK_IMPL_UNIX:
            return CMN_DISPATCHER_IMPL_SOCK;

        case CMN_LINK_IMPL_IPC:
            return CMN_DISPATCHER_IMPL_IPC;
        
        case CMN_LINK_IMPL_IPCDA:
            return CMN_DISPATCHER_IMPL_IPCDA;

        /* PROJ-2474 SSL/TLS */
        case CMN_LINK_IMPL_SSL:
            return CMN_DISPATCHER_IMPL_SOCK;

        /* PROJ-2681 */
        case CMN_LINK_IMPL_IB:
            return CMN_DISPATCHER_IMPL_IB;

        default:
            break;
    }

    /*
     * 존재하지 않는 Link Impl인 경우
     */
    return CMN_DISPATCHER_IMPL_INVALID;
}

IDE_RC cmnDispatcherWaitLink(cmnLink *aLink, cmiDirection aDirection, PDL_Time_Value *aTimeout)
{
    cmnDispatcherImpl sImpl;

    /*
     * Dispatcher Impl 획득
     */
    sImpl = cmnDispatcherImplForLinkImpl(aLink->mImpl);

    /*
     * Dispatcher Impl 범위 검사
     */
    IDE_ASSERT(sImpl >= CMN_DISPATCHER_IMPL_BASE);
    IDE_ASSERT(sImpl <  CMN_DISPATCHER_IMPL_MAX);

    /*
     * WaitLink 함수 호출
     */
    IDE_TEST(gCmnDispatcherWaitLink[sImpl](aLink, aDirection, aTimeout) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* BUG-45240 - aHandle에서 Read 이벤트 발생 여부 확인 */
SInt cmnDispatcherCheckHandle(PDL_SOCKET aHandle, PDL_Time_Value *aTimeout)
{
    return gCmnDispatcherCheckHandle(aHandle, aTimeout);
}

#ifdef CMN_DISPATCHER_VERIFY
static void cmnDispatcherVerifyLinkList(cmnDispatcher *aDispatcher)
{
    iduListNode *sIterator;
    UInt         sCount = 0;

    IDU_LIST_ITERATE(&aDispatcher->mLinkList, sIterator)
    {
        sCount++;
    }

    IDE_ASSERT(sCount == aDispatcher->mLinkCount);
}
#endif

IDE_RC cmnDispatcherAddLink(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    /*
     * 이미 등록된 Link인지 검사
     */
    IDE_TEST_RAISE(IDU_LIST_IS_EMPTY(&aLink->mDispatchListNode) != ID_TRUE, LinkAlreadyInDispatching);

    /*
     * Link List에 추가
     */
    IDU_LIST_ADD_LAST(&aDispatcher->mLinkList, &aLink->mDispatchListNode);

    /*
     * Link Count 증가
     */
    aDispatcher->mLinkCount++;

#ifdef CMN_DISPATCHER_VERIFY
    cmnDispatcherVerifyLinkList(aDispatcher);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(LinkAlreadyInDispatching)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_LINK_ALREADY_IN_DISPATCHING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveLink(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    /*
     * 노드 삭제
     */
    IDU_LIST_REMOVE(&aLink->mDispatchListNode);

    /*
     * 노드 초기화
     */
    IDU_LIST_INIT_OBJ(&aLink->mDispatchListNode, aLink);

    /*
     * Link Count 감소
     */
    aDispatcher->mLinkCount--;

#ifdef CMN_DISPATCHER_VERIFY
    cmnDispatcherVerifyLinkList(aDispatcher);
#endif

    return IDE_SUCCESS;
}

IDE_RC cmnDispatcherRemoveAllLinks(cmnDispatcher *aDispatcher)
{
    iduListNode *sIterator;
    iduListNode *sNextNode;
    cmnLink     *sLink;

    /*
     * Link List 정리
     */
    IDU_LIST_ITERATE_SAFE(&aDispatcher->mLinkList, sIterator, sNextNode)
    {
        sLink = (cmnLink *)sIterator->mObj;

        IDE_TEST(cmnDispatcherRemoveLink(aDispatcher, sLink) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
