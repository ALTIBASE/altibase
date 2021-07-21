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
 * 구조체와 함수명에 suffix 'Poll'을 추가.
 */

#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)

typedef struct cmnDispatcherSOCKPoll
{
    cmnDispatcher   mDispatcher;

    UInt            mPollFdCount;
    UInt            mPollFdSize;

    struct pollfd  *mPollFd;
    cmnLink       **mLink;
} cmnDispatcherSOCKPoll;


#ifdef CMN_DISPATCHER_VERIFY
static void cmnDispatcherVerifyLinkSOCKPoll(cmnDispatcherSOCKPoll *aDispatcher)
{
    PDL_HANDLE  sHandle;
    cmnLink    *sLink;
    UInt        sPollFdIndex;
    UInt        i;

    /*
     * ABW 검사
     */
    IDE_ASSERT(aDispatcher->mPollFdCount <= aDispatcher->mPollFdSize);

    for (i = 0; i < aDispatcher->mPollFdCount; i++)
    {
        /*
         * 각 Link에 대하여
         */
        sLink = aDispatcher->mLink[i];

        /*
         * socket 값이 일치하는지 검사
         */
        IDE_ASSERT(sLink->mOp->mGetDescriptor(sLink, &sHandle) == IDE_SUCCESS);

        IDE_ASSERT(aDispatcher->mPollFd[i].fd == sHandle);

        /*
         * DispatchInfo 값이 일치하는지 검사
         */
        IDE_ASSERT(sLink->mOp->mGetDispatchInfo(sLink, &sPollFdIndex) == IDE_SUCCESS);

        IDE_ASSERT(sPollFdIndex == i);
    }
}
#endif


IDE_RC cmnDispatcherInitializeSOCKPoll(cmnDispatcher *aDispatcher, UInt aMaxLink)
{
    cmnDispatcherSOCKPoll *sDispatcher = (cmnDispatcherSOCKPoll *)aDispatcher;

    /*
     * 멤버 초기화
     */
    sDispatcher->mPollFdCount = 0;
    sDispatcher->mPollFdSize  = aMaxLink;
    sDispatcher->mPollFd      = NULL;
    sDispatcher->mLink        = NULL;

    IDU_FIT_POINT( "cmnDispatcherSOCKPOLL::cmnDispatcherInitializeSOCKPoll::malloc::PollFd" );
    
    /*
     * pollfd를 위한 메모리 할당
     */
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_CMN,
                               ID_SIZEOF(struct pollfd) * sDispatcher->mPollFdSize,
                               (void **)&(sDispatcher->mPollFd),
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    IDU_FIT_POINT( "cmnDispatcherSOCKPOLL::cmnDispatcherInitializeSOCKPoll::malloc::Link" );

    /*
     * pollfd에 추가된 Link를 저장할 메모리 할당
     */
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_CMN,
                               ID_SIZEOF(cmnLink *) * sDispatcher->mPollFdSize,
                               (void **)&(sDispatcher->mLink),
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sDispatcher->mPollFd != NULL)
        {
            IDE_ASSERT(iduMemMgr::free(sDispatcher->mPollFd) == IDE_SUCCESS);
            sDispatcher->mPollFd = NULL;
        }
        if (sDispatcher->mLink != NULL)
        {
            IDE_ASSERT(iduMemMgr::free(sDispatcher->mLink) == IDE_SUCCESS);
            sDispatcher->mLink = NULL;
        }
    }

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherFinalizeSOCKPoll(cmnDispatcher *aDispatcher)
{
    cmnDispatcherSOCKPoll *sDispatcher = (cmnDispatcherSOCKPoll *)aDispatcher;

    /*
     * 멤버 초기화
     */
    sDispatcher->mPollFdCount = 0;
    sDispatcher->mPollFdSize  = 0;

    /*
     * BUG-41458 할당한 메모리 해제
     */
    if (sDispatcher->mPollFd != NULL)
    {
        IDE_TEST(iduMemMgr::free(sDispatcher->mPollFd) != IDE_SUCCESS);
        sDispatcher->mPollFd = NULL;
    }
    else
    {
        /* Nothing */
    }
    if (sDispatcher->mLink != NULL)
    {
        IDE_TEST(iduMemMgr::free(sDispatcher->mLink) != IDE_SUCCESS);
        sDispatcher->mLink = NULL;
    }
    else
    {
        /* Nothing */
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnDispatcherAddLinkSOCKPoll(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    cmnDispatcherSOCKPoll *sDispatcher = (cmnDispatcherSOCKPoll *)aDispatcher;
    PDL_SOCKET             sHandle;

    /*
     * Dispatcher에서 사용할 수 있는 Link Impl인지 검사 (PROJ-2681)
     */
    IDE_TEST_RAISE(cmiDispatcherImplForLink(aLink) != sDispatcher->mDispatcher.mImpl, InvalidLinkImpl);

    /*
     * Link 갯수 초과 검사
     */
    IDE_TEST_RAISE(sDispatcher->mPollFdCount == sDispatcher->mPollFdSize, LinkLimitReach);

    /*
     * Dispatcher의 Link list에 추가
     */
    IDE_TEST(cmnDispatcherAddLink(aDispatcher, aLink) != IDE_SUCCESS);

    /*
     * Link의 socket 획득
     */
    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS);

    /*
     * Link의 DispatchInfo를 pollfd array의 index로 세팅
     */
    IDE_TEST(aLink->mOp->mSetDispatchInfo(aLink, &sDispatcher->mPollFdCount) != IDE_SUCCESS);

    /*
     * pollfd array에 socket 추가
     */
    sDispatcher->mPollFd[sDispatcher->mPollFdCount].fd      = sHandle;
    sDispatcher->mPollFd[sDispatcher->mPollFdCount].events  = POLLIN;
    sDispatcher->mPollFd[sDispatcher->mPollFdCount].revents = 0;

    /*
     * Link array에 Link 추가
     */
    sDispatcher->mLink[sDispatcher->mPollFdCount] = aLink;

    /*
     * pollfd array count 증가
     */
    sDispatcher->mPollFdCount++;

#ifdef CMN_DISPATCHER_VERIFY
    cmnDispatcherVerifyLinkSOCKPoll(sDispatcher);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(LinkLimitReach)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_LINK_LIMIT_REACH));
    }
    IDE_EXCEPTION(InvalidLinkImpl)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_LINK_IMPL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveLinkSOCKPoll(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    cmnDispatcherSOCKPoll *sDispatcher = (cmnDispatcherSOCKPoll *)aDispatcher;
    cmnLink               *sLink;
    PDL_SOCKET             sHandle;
    UInt                   sPollFdIndex;

    /*
     * Link의 socket 획득
     */
    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS);

    /*
     * Link의 pollfd array index를 획득
     */
    IDE_TEST(aLink->mOp->mGetDispatchInfo(aLink, &sPollFdIndex) != IDE_SUCCESS);

    /*
     * Link 삭제
     */
    IDE_TEST(cmnDispatcherRemoveLink(aDispatcher, aLink) != IDE_SUCCESS);

    /*
     * pollfd array count 감소
     */
    sDispatcher->mPollFdCount--;

    /*
     * 삭제된 pollfd가 array의 마지막이 아니라면 마지막의 pollfd를 삭제된 pollfd위치로 이동
     */
    if (sDispatcher->mPollFdCount != sPollFdIndex)
    {
        /*
         * pollfd array의 마지막 Link 획득
         */
        sLink = sDispatcher->mLink[sDispatcher->mPollFdCount];

        /*
         * Link의 socket 획득
         */
        IDE_TEST(sLink->mOp->mGetHandle(sLink, &sHandle) != IDE_SUCCESS);

        /*
         * pollfd의 socket과 일치여부 검사
         */
        IDE_ASSERT(sDispatcher->mPollFd[sDispatcher->mPollFdCount].fd == sHandle);

        /*
         * 삭제된 pollfd위치로 이동
         */
        sDispatcher->mPollFd[sPollFdIndex].fd     = sDispatcher->mPollFd[sDispatcher->mPollFdCount].fd;
        sDispatcher->mPollFd[sPollFdIndex].events = sDispatcher->mPollFd[sDispatcher->mPollFdCount].events;

        /*
         * 삭제된 Link위치로 이동
         */
        sDispatcher->mLink[sPollFdIndex] = sLink;

        /*
         * 새로운 pollfd array index를 세팅
         */
        IDE_TEST(sLink->mOp->mSetDispatchInfo(sLink, &sPollFdIndex) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveAllLinksSOCKPoll(cmnDispatcher *aDispatcher)
{
    cmnDispatcherSOCKPoll *sDispatcher = (cmnDispatcherSOCKPoll *)aDispatcher;

    IDE_TEST(cmnDispatcherRemoveAllLinks(aDispatcher) != IDE_SUCCESS);

    sDispatcher->mPollFdCount = 0;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnDispatcherDetectSOCKPoll(cmnDispatcher  *aDispatcher,
                                   iduList        *aReadyList,
                                   UInt           *aReadyCount,
                                   PDL_Time_Value *aTimeout)
{
    cmnDispatcherSOCKPoll *sDispatcher = (cmnDispatcherSOCKPoll *)aDispatcher;
    iduListNode           *sIterator;
    cmnLink               *sLink;
    PDL_SOCKET             sHandle;
    SInt                   sResult = 0;

    /* BUG-37872 fix compilation error in cmnDispatcherSOCK-POLL.cpp */
    UInt                   sPollFdIndex;

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    idBool                 sIsDedicatedMode = ID_FALSE;

    IDU_LIST_INIT(aReadyList);

    /*
     * BUG-39068 Verify the poll() system call in case
     *           the service threads are the dedicated mode
     */
    if ( aTimeout != NULL )
    {
        if ( aTimeout->sec() == DEDICATED_THREAD_MODE_TIMEOUT_FLAG )
        {

            /* PROJ-2108 Dedicated thread mode which uses less CPU 
             * Set dedicated mode flag if
             * aTimeout->sec == magic number(765432) for infinite select()
             */
            sIsDedicatedMode = ID_TRUE;
        }
    }

    if ( sIsDedicatedMode == ID_TRUE )
    {
        if ( sDispatcher->mPollFdCount > 0 )
        {
            sResult = idlOS::poll(sDispatcher->mPollFd,
                                  sDispatcher->mPollFdCount,
                                  NULL);
        }
        else
        {
            /* 
             * Client가 종료한 경우이며 DedicatedMode에서는 poll()을 호출할 필요가 없다.
             * DedicatedMode에서는 해당 Servicethread가 cond_wait 상태로 가기 때문이다.
             */
        }
    }
    else
    {
        sResult = idlOS::poll(sDispatcher->mPollFd,
                              sDispatcher->mPollFdCount,
                              aTimeout);
    }

    IDE_TEST_RAISE(sResult < 0, PollError);

    /*
     * ReadyCount 세팅
     */
    if (aReadyCount != NULL)
    {
        *aReadyCount = sResult;
    }

    /*
     * Ready Link 검색
     */
    if (sResult > 0)
    {
        IDU_LIST_ITERATE(&aDispatcher->mLinkList, sIterator)
        {
            sLink = (cmnLink *)sIterator->mObj;

            /*
             * Link의 socket을 획득
             */
            IDE_TEST(sLink->mOp->mGetHandle(sLink, &sHandle) != IDE_SUCCESS);

            /*
             * Link의 pollfd array index를 획득
             */
            IDE_TEST(sLink->mOp->mGetDispatchInfo(sLink, &sPollFdIndex) != IDE_SUCCESS);

            /*
             * pollfd array index의 범위 검사
             */
            IDE_ASSERT(sPollFdIndex < sDispatcher->mPollFdCount);

            /*
             * pollfd의 socket과 일치여부 검사
             */
            IDE_ASSERT(sDispatcher->mPollFd[sPollFdIndex].fd == sHandle);

            /*
             * ready 검사
             */
            if (sDispatcher->mPollFd[sPollFdIndex].revents != 0)
            {
                IDU_LIST_ADD_LAST(aReadyList, &sLink->mReadyListNode);
            }
        }
    }

#ifdef CMN_DISPATCHER_VERIFY
    cmnDispatcherVerifyLinkSOCKPoll(sDispatcher);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(PollError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_POLL_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


struct cmnDispatcherOP gCmnDispatcherOpSOCKPoll =
{
    (SChar *)"SOCK-POLL",

    cmnDispatcherInitializeSOCKPoll,
    cmnDispatcherFinalizeSOCKPoll,

    cmnDispatcherAddLinkSOCKPoll,
    cmnDispatcherRemoveLinkSOCKPoll,
    cmnDispatcherRemoveAllLinksSOCKPoll,

    cmnDispatcherDetectSOCKPoll
};


IDE_RC cmnDispatcherMapSOCKPoll(cmnDispatcher *aDispatcher)
{
    /*
     * 함수 포인터 세팅
     */
    aDispatcher->mOp = &gCmnDispatcherOpSOCKPoll;

    return IDE_SUCCESS;
}

UInt cmnDispatcherSizeSOCKPoll()
{
    return ID_SIZEOF(cmnDispatcherSOCKPoll);
}

IDE_RC cmnDispatcherWaitLinkSOCKPoll(cmnLink *aLink,
                                     cmnDirection aDirection,
                                     PDL_Time_Value *aTimeout)
{
    PDL_SOCKET    sHandle;
    SInt          sResult;
    struct pollfd sPollFd;

    /*
     * Link의 socket 획득
     */
    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS);

    /*
     * pollfd 세팅
     */
    sPollFd.fd      = sHandle;
    sPollFd.events  = 0;
    sPollFd.revents = 0;

    switch (aDirection)
    {
        case CMI_DIRECTION_RD:
            sPollFd.events = POLLIN;
            break;
        case CMI_DIRECTION_WR:
            sPollFd.events = POLLOUT;
            break;
        case CMI_DIRECTION_RDWR:
            sPollFd.events = POLLIN | POLLOUT;
            break;
    }

    /*
     * poll 수행
     */
    sResult = idlOS::poll(&sPollFd, 1, aTimeout);

    IDE_TEST_RAISE(sResult < 0, PollError);
    IDE_TEST_RAISE(sResult == 0, TimedOut);

    return IDE_SUCCESS;

    IDE_EXCEPTION(PollError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_POLL_ERROR));
    }
    IDE_EXCEPTION(TimedOut);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-45240 */
SInt cmnDispatcherCheckHandleSOCKPoll(PDL_SOCKET      aHandle,
                                      PDL_Time_Value *aTimeout)
{
    SInt          sResult = -1;
    struct pollfd sPollFd;

    /* pollfd 세팅 */
    sPollFd.fd      = aHandle;
    sPollFd.events  = 0;
    sPollFd.revents = 0;
    sPollFd.events  = POLLIN;

    /* poll 수행 */
    sResult = idlOS::poll(&sPollFd, 1, aTimeout);

    return sResult;
}

#endif  /* !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX) */
