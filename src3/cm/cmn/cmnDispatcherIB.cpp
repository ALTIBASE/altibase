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

typedef struct cmnDispatcherIB
{
    cmnDispatcher   mDispatcher;

    UInt            mPollFdCount;
    UInt            mPollFdSize;
    UInt            mBerkeleySockCount;

    struct pollfd  *mPollFd;
    cmnLink       **mLink;
} cmnDispatcherIB;

extern "C" cmnIB gIB;

IDE_RC cmnDispatcherInitializeIB(cmnDispatcher *aDispatcher, UInt aMaxLink)
{
    cmnDispatcherIB *sDispatcher = (cmnDispatcherIB *)aDispatcher;

    sDispatcher->mPollFdCount       = 0;
    sDispatcher->mPollFdSize        = aMaxLink;
    sDispatcher->mPollFd            = NULL;
    sDispatcher->mBerkeleySockCount = 0;
    sDispatcher->mLink              = NULL;

    IDU_FIT_POINT("cmnDispatcherIB::cmnDispatcherInitializeIB::malloc::PollFd");
    
    /* pollfd를 위한 메모리 할당 */
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_CMN,
                               ID_SIZEOF(struct pollfd) * sDispatcher->mPollFdSize,
                               (void **)&(sDispatcher->mPollFd),
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    IDU_FIT_POINT("cmnDispatcherIB::cmnDispatcherInitializeIB::malloc::Link");

    /* pollfd에 추가된 Link를 저장할 메모리 할당 */
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_CMN,
                               ID_SIZEOF(cmnLink *) * sDispatcher->mPollFdSize,
                               (void **)&(sDispatcher->mLink),
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sDispatcher->mPollFd != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(sDispatcher->mPollFd) == IDE_SUCCESS);
        sDispatcher->mPollFd = NULL;
    }
    else
    {
        /* nothing to do */
    }

    if (sDispatcher->mLink != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(sDispatcher->mLink) == IDE_SUCCESS);
        sDispatcher->mLink = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherFinalizeIB(cmnDispatcher *aDispatcher)
{
    cmnDispatcherIB *sDispatcher = (cmnDispatcherIB *)aDispatcher;

    sDispatcher->mPollFdCount       = 0;
    sDispatcher->mPollFdSize        = 0;
    sDispatcher->mBerkeleySockCount = 0;

    if (sDispatcher->mPollFd != NULL)
    {
        IDE_TEST(iduMemMgr::free(sDispatcher->mPollFd) != IDE_SUCCESS);
        sDispatcher->mPollFd = NULL;
    }
    else
    {
        /* nothing to do */
    }

    if (sDispatcher->mLink != NULL)
    {
        IDE_TEST(iduMemMgr::free(sDispatcher->mLink) != IDE_SUCCESS);
        sDispatcher->mLink = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherAddLinkIB(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    cmnDispatcherIB *sDispatcher = (cmnDispatcherIB *)aDispatcher;
    PDL_SOCKET       sHandle;

    /* allow TCP, UDS, SSL and IB link to add to this IB dispatcher */
    IDE_TEST_RAISE((aLink->mImpl != CMN_LINK_IMPL_IB) &&
                   (aLink->mImpl != CMN_LINK_IMPL_TCP) &&
                   (aLink->mImpl != CMN_LINK_IMPL_UNIX) &&
                   (aLink->mImpl != CMN_LINK_IMPL_SSL), InvalidLinkImpl);

    /* check maximum file descriptor that is polling available */
    IDE_TEST_RAISE(sDispatcher->mPollFdCount == sDispatcher->mPollFdSize, LinkLimitReach);

    IDE_TEST(cmnDispatcherAddLink(aDispatcher, aLink) != IDE_SUCCESS);

    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS);

    /* dispatch info = pollfd array index */
    IDE_TEST(aLink->mOp->mSetDispatchInfo(aLink, &sDispatcher->mPollFdCount) != IDE_SUCCESS);

    /* add polling event to pollfd */
    sDispatcher->mPollFd[sDispatcher->mPollFdCount].fd      = sHandle;
    sDispatcher->mPollFd[sDispatcher->mPollFdCount].events  = POLLIN;
    sDispatcher->mPollFd[sDispatcher->mPollFdCount].revents = 0;

    sDispatcher->mLink[sDispatcher->mPollFdCount] = aLink;

    sDispatcher->mPollFdCount++;

    if (aLink->mImpl != CMN_LINK_IMPL_IB)
    {
        sDispatcher->mBerkeleySockCount++;
    }
    else
    {
        /* nothing to do */
    }

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

IDE_RC cmnDispatcherRemoveLinkIB(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    cmnDispatcherIB *sDispatcher = (cmnDispatcherIB *)aDispatcher;
    cmnLink         *sLink;
    PDL_SOCKET       sHandle;
    UInt             sPollFdIndex;

    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS);

    IDE_TEST(aLink->mOp->mGetDispatchInfo(aLink, &sPollFdIndex) != IDE_SUCCESS);

    IDE_TEST(cmnDispatcherRemoveLink(aDispatcher, aLink) != IDE_SUCCESS);

    sDispatcher->mPollFdCount--;

    if (aLink->mImpl != CMN_LINK_IMPL_IB)
    {
        sDispatcher->mBerkeleySockCount--;
    }
    else
    {
        /* nothing to do */
    }

    /*
     * 삭제된 pollfd가 array의 마지막이 아니라면 마지막의 pollfd를 삭제된 pollfd위치로 이동
     */
    if (sDispatcher->mPollFdCount != sPollFdIndex)
    {
        /* pollfd array의 마지막 Link 획득 */
        sLink = sDispatcher->mLink[sDispatcher->mPollFdCount];

        /* Link의 socket 획득 */
        IDE_TEST(sLink->mOp->mGetHandle(sLink, &sHandle) != IDE_SUCCESS);

        /* pollfd의 socket과 일치여부 검사 */
        IDE_ASSERT(sDispatcher->mPollFd[sDispatcher->mPollFdCount].fd == sHandle);

        /* 삭제된 pollfd위치로 이동 */
        sDispatcher->mPollFd[sPollFdIndex].fd     = sDispatcher->mPollFd[sDispatcher->mPollFdCount].fd;
        sDispatcher->mPollFd[sPollFdIndex].events = sDispatcher->mPollFd[sDispatcher->mPollFdCount].events;

        /* 삭제된 Link위치로 이동 */
        sDispatcher->mLink[sPollFdIndex] = sLink;

        /* 새로운 pollfd array index를 세팅 */
        IDE_TEST(sLink->mOp->mSetDispatchInfo(sLink, &sPollFdIndex) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveAllLinksIB(cmnDispatcher *aDispatcher)
{
    cmnDispatcherIB *sDispatcher = (cmnDispatcherIB *)aDispatcher;

    IDE_TEST(cmnDispatcherRemoveAllLinks(aDispatcher) != IDE_SUCCESS);

    sDispatcher->mPollFdCount = 0;
    sDispatcher->mBerkeleySockCount = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherDetectIB(cmnDispatcher  *aDispatcher,
                             iduList        *aReadyList,
                             UInt           *aReadyCount,
                             PDL_Time_Value *aTimeout)
{
    cmnDispatcherIB *sDispatcher = (cmnDispatcherIB *)aDispatcher;
    iduListNode     *sIterator;
    cmnLink         *sLink;
    PDL_SOCKET       sHandle;
    SInt             sResult = 0;
    PDL_Time_Value  *sTimeout = aTimeout;

    /* BUG-37872 fix compilation error in cmnDispatcherIB.cpp */
    UInt                   sPollFdIndex;

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

            /* PROJ-2108 Dedicated thread mode which uses less CPU 
             * Set dedicated mode flag if
             * aTimeout->sec == magic number(765432) for infinite select()
             */
            sIsDedicatedMode = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }
    }

    if (sDispatcher->mPollFdCount == 0)
    {
        if (sIsDedicatedMode == ID_FALSE)
        {
            /**
             * Cannot call rpoll(), if fd set is empty. The call will be returned with an error (ENOMEM).
             */
            sResult = idlOS::poll(sDispatcher->mPollFd,
                                  sDispatcher->mPollFdCount,
                                  sTimeout);
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
        if (sIsDedicatedMode == ID_TRUE)
        {
            sTimeout = NULL;
        }
        else
        {
            /* nothing to do */
        }

        /**
         * poll(), if fd set is consisted of only Berkeley socket(s).
         */
        if (sDispatcher->mBerkeleySockCount != sDispatcher->mPollFdCount)
        {
            sResult = gIB.mFuncs.rpoll(sDispatcher->mPollFd,
                                       sDispatcher->mPollFdCount,
                                       (sTimeout != NULL) ? sTimeout->msec() : -1);
        }
        else
        {
            sResult = idlOS::poll(sDispatcher->mPollFd,
                                  sDispatcher->mPollFdCount,
                                  sTimeout);
        }
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

    return IDE_SUCCESS;

    IDE_EXCEPTION(PollError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


struct cmnDispatcherOP gCmnDispatcherOpIB =
{
    (SChar *)"IB",

    cmnDispatcherInitializeIB,
    cmnDispatcherFinalizeIB,

    cmnDispatcherAddLinkIB,
    cmnDispatcherRemoveLinkIB,
    cmnDispatcherRemoveAllLinksIB,

    cmnDispatcherDetectIB
};


IDE_RC cmnDispatcherMapIB(cmnDispatcher *aDispatcher)
{
    aDispatcher->mOp = &gCmnDispatcherOpIB;

    return IDE_SUCCESS;
}

UInt cmnDispatcherSizeIB()
{
    return ID_SIZEOF(cmnDispatcherIB);
}

IDE_RC cmnDispatcherWaitLinkIB(cmnLink        *aLink,
                               cmnDirection    aDirection,
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
        default:
            IDE_RAISE(InvalidDirection);
            break;
    }

    /*
     * poll 수행
     */
    sResult = gIB.mFuncs.rpoll(&sPollFd,
                               1,
                               (aTimeout != NULL) ? aTimeout->msec() : -1);

    IDE_TEST_RAISE(sResult < 0, PollError);
    IDE_TEST_RAISE(sResult == 0, TimedOut);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidDirection)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RPOLL_INVALID_DIRECTION, aDirection));
    }
    IDE_EXCEPTION(PollError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    IDE_EXCEPTION(TimedOut);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

