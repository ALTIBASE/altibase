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

/* 
 * BUG-38951 Support to choice a type of CM dispatcher on run-time
 *
 * 구조체와 함수명에 suffix 'Epoll'을 추가. (BUG-45240)
 */

#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)

IDE_RC cmnDispatcherFinalizeSOCKEpoll(cmnDispatcher *aDispatcher);

typedef struct cmnDispatcherSOCKEpoll
{
    cmnDispatcher       mDispatcher;

    UInt                mPollingFdCnt;  /* polling중인 Fd 개수 */

    SInt                mEpollFd;       /* A handle for Epoll */
    struct epoll_event *mEvents;        /* 반환된 이벤트 리스트 */
    UInt                mMaxEvents;     /* 감지할 이벤트 최대수 */
} cmnDispatcherSOCKEpoll;

IDE_RC cmnDispatcherInitializeSOCKEpoll(cmnDispatcher *aDispatcher, UInt aMaxLink)
{
    cmnDispatcherSOCKEpoll *sDispatcher = (cmnDispatcherSOCKEpoll *)aDispatcher;

    sDispatcher->mEpollFd      = PDL_INVALID_HANDLE;
    sDispatcher->mEvents       = NULL;
    sDispatcher->mMaxEvents    = 0;
    sDispatcher->mPollingFdCnt = 0;

    /*
     * Since Linux 2.6.8, the size argument is unused.
     * (The kernel dynamically sizes the required data structures without needing this initial hint.)
     */
    sDispatcher->mEpollFd = idlOS::epoll_create(aMaxLink);
    IDE_TEST_RAISE(sDispatcher->mEpollFd == PDL_INVALID_HANDLE, EpollCreateError);

    /* mEvents를 위한 메모리 할당 - 몇개가 적당할까? */
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMN,
                                     ID_SIZEOF(struct epoll_event) * aMaxLink,
                                     (void **)&(sDispatcher->mEvents),
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory);

    sDispatcher->mMaxEvents = aMaxLink;

    return IDE_SUCCESS;

    IDE_EXCEPTION(EpollCreateError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SYSTEM_CALL_ERROR, "epoll_create(aMaxLink)"));
    }
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    cmnDispatcherFinalizeSOCKEpoll(aDispatcher);

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherFinalizeSOCKEpoll(cmnDispatcher *aDispatcher)
{
    cmnDispatcherSOCKEpoll *sDispatcher = (cmnDispatcherSOCKEpoll *)aDispatcher;

    if (sDispatcher->mEpollFd != PDL_INVALID_HANDLE)
    {
        (void)idlOS::close(sDispatcher->mEpollFd);
        sDispatcher->mEpollFd = PDL_INVALID_HANDLE;
    }
    else
    {
        /* A obsolete convention */
    }

    /* BUG-41458 할당한 메모리 해제 */
    if (sDispatcher->mEvents != NULL)
    {
        (void)iduMemMgr::free(sDispatcher->mEvents);
        sDispatcher->mEvents = NULL;
    }
    else
    {
        /* A obsolete convention */
    }

    sDispatcher->mMaxEvents    = 0;
    sDispatcher->mPollingFdCnt = 0;

    return IDE_SUCCESS;
}

IDE_RC cmnDispatcherAddLinkSOCKEpoll(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    cmnDispatcherSOCKEpoll *sDispatcher = (cmnDispatcherSOCKEpoll *)aDispatcher;
    PDL_SOCKET              sHandle     = PDL_INVALID_SOCKET;
    struct epoll_event      sEvent;

    /* Link의 socket 획득 */
    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS);

    /* socket 추가 */
    sEvent.events   = EPOLLIN;
    sEvent.data.ptr = (void *)aLink;
    IDE_TEST_RAISE(idlOS::epoll_ctl(sDispatcher->mEpollFd,
                                    EPOLL_CTL_ADD,
                                    sHandle,
                                    &sEvent) < 0, EpollCtlError);

    sDispatcher->mPollingFdCnt++;

    /* Dispatcher의 Link list에 추가 */
    IDE_TEST(cmnDispatcherAddLink(aDispatcher, aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EpollCtlError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SYSTEM_CALL_ERROR, "epoll_ctl(add)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveLinkSOCKEpoll(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    cmnDispatcherSOCKEpoll *sDispatcher = (cmnDispatcherSOCKEpoll *)aDispatcher;
    PDL_SOCKET              sHandle     = PDL_INVALID_SOCKET;
    struct epoll_event      sEvent;

    /* Link의 socket 획득 */
    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS); 

    /* 2.6.9 이전 버전과 호환성 유지를 위해 sEvent 설정 */
    sEvent.events   = EPOLLIN;
    sEvent.data.ptr = (void *)aLink;
    IDE_TEST_RAISE(idlOS::epoll_ctl(sDispatcher->mEpollFd,
                                    EPOLL_CTL_DEL,
                                    sHandle,
                                    &sEvent) < 0, EpollCtlError);

    sDispatcher->mPollingFdCnt--;

    /* Dispatcher의 Link list에 삭제 */
    IDE_TEST(cmnDispatcherRemoveLink(aDispatcher, aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EpollCtlError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SYSTEM_CALL_ERROR, "epoll_ctl(del)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveAllLinksSOCKEpoll(cmnDispatcher *aDispatcher)
{
    cmnDispatcherSOCKEpoll *sDispatcher = (cmnDispatcherSOCKEpoll *)aDispatcher;

    IDE_TEST(cmnDispatcherRemoveAllLinks(aDispatcher) != IDE_SUCCESS);

    (void)idlOS::close(sDispatcher->mEpollFd);

    sDispatcher->mEpollFd = idlOS::epoll_create(sDispatcher->mMaxEvents);
    IDE_TEST_RAISE(sDispatcher->mEpollFd == PDL_INVALID_HANDLE, EpollCreateError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EpollCreateError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SYSTEM_CALL_ERROR, "epoll_create(mMaxEvents)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherDetectSOCKEpoll(cmnDispatcher  *aDispatcher,
                                    iduList        *aReadyList,
                                    UInt           *aReadyCount,
                                    PDL_Time_Value *aTimeout)
{
    cmnDispatcherSOCKEpoll *sDispatcher = (cmnDispatcherSOCKEpoll *)aDispatcher;
    cmnLink                *sLink       = NULL;
    SInt                    sResult     = 0;
    SInt                    i           = 0;

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    idBool                 sIsDedicatedMode = ID_FALSE;

    IDU_LIST_INIT(aReadyList);

    /*
     * BUG-39068 Verify the poll() system call in case
     *           the service threads are the dedicated mode
     */
    if (aTimeout != NULL)
    {
        if (aTimeout->sec() == DEDICATED_THREAD_MODE_TIMEOUT_FLAG)
        {

            /* 
             * PROJ-2108 Dedicated thread mode which uses less CPU 
             * Set dedicated mode flag if
             * aTimeout->sec == magic number(765432) for infinite select()
             */
            sIsDedicatedMode = ID_TRUE;
        }
    }

    if (sIsDedicatedMode == ID_TRUE)
    {
        if (sDispatcher->mPollingFdCnt > 0)
        {
            sResult = idlOS::epoll_wait( sDispatcher->mEpollFd,
                                         sDispatcher->mEvents,
                                         sDispatcher->mMaxEvents,
                                         NULL );
        }
        else
        {
            /* 
             * Client가 종료한 경우이며 DedicatedMode에서는 epoll_wait()을 호출할 필요가 없다.
             * DedicatedMode에서는 해당 Servicethread가 cond_wait 상태로 가기 때문이다.
             */
        }
    }
    else
    {
        sResult = idlOS::epoll_wait( sDispatcher->mEpollFd,
                                     sDispatcher->mEvents,
                                     sDispatcher->mMaxEvents,
                                     aTimeout );
    }

    IDE_TEST_RAISE(sResult < 0, EpollWaitError);

    /* Ready Link 검색 */
    for (i = 0; i < sResult; i++)
    {
        IDE_ASSERT(sDispatcher->mEvents[i].data.ptr != NULL);

        sLink = (cmnLink *)sDispatcher->mEvents[i].data.ptr;
        IDU_LIST_ADD_LAST(aReadyList, &sLink->mReadyListNode);
    }

    /* ReadyCount 세팅 */
    if (aReadyCount != NULL)
    {
        *aReadyCount = sResult;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(EpollWaitError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SYSTEM_CALL_ERROR, "epoll_wait()", errno));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


struct cmnDispatcherOP gCmnDispatcherOpSOCKEpoll =
{
    (SChar *)"SOCK-EPOLL",

    cmnDispatcherInitializeSOCKEpoll,
    cmnDispatcherFinalizeSOCKEpoll,

    cmnDispatcherAddLinkSOCKEpoll,
    cmnDispatcherRemoveLinkSOCKEpoll,
    cmnDispatcherRemoveAllLinksSOCKEpoll,

    cmnDispatcherDetectSOCKEpoll
};


IDE_RC cmnDispatcherMapSOCKEpoll(cmnDispatcher *aDispatcher)
{
    /* 함수 포인터 세팅 */
    aDispatcher->mOp = &gCmnDispatcherOpSOCKEpoll;

    return IDE_SUCCESS;
}

UInt cmnDispatcherSizeSOCKEpoll()
{
    return ID_SIZEOF(cmnDispatcherSOCKEpoll);
}

IDE_RC cmnDispatcherWaitLinkSOCKEpoll(cmnLink        * /*aLink*/,
                                      cmnDirection     /*aDirection*/,
                                      PDL_Time_Value * /*aTimeout*/)
{
    /* 
     * Epoll은 1회성 FD를 감시하는 용도로는 시스템 콜 비용이 더 크다.
     * Poll을 추천한다. (epoll_create() -> epoll_ctl() -> close()) 
     */
    IDE_ASSERT(0);

    return IDE_FAILURE;
}

SInt cmnDispatcherCheckHandleSOCKEpoll(PDL_SOCKET       /*aHandle*/,
                                       PDL_Time_Value * /*aTimeout*/)
{
    /* 
     * Epoll은 1회성 FD를 감시하는 용도로는 시스템 콜 비용이 더 크다.
     * Poll을 추천한다. (epoll_create() -> epoll_ctl() -> close()) 
     */
    IDE_ASSERT(0);

    return -1;
}

#endif  /* !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX) */
