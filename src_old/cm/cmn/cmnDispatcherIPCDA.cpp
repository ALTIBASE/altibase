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

#if !defined(CM_DISABLE_IPCDA)


typedef struct cmnDispatcherIPCDA
{
    cmnDispatcher  mDispatcher;

    PDL_SOCKET     mMaxHandle;

    fd_set         mFdSet;
} cmnDispatcherIPCDA;


IDE_RC cmnDispatcherInitializeIPCDA(cmnDispatcher *aDispatcher, UInt /*aMaxLink*/)
{
    cmnDispatcherIPCDA *sDispatcher = (cmnDispatcherIPCDA *)aDispatcher;

    /* 멤버 초기화 */
    sDispatcher->mMaxHandle  = PDL_INVALID_SOCKET;

    /* fdset 초기화 */
    FD_ZERO(&sDispatcher->mFdSet);

    return IDE_SUCCESS;
}

IDE_RC cmnDispatcherFinalizeIPCDA(cmnDispatcher * /*aDispatcher*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnDispatcherAddLinkIPCDA(cmnDispatcher *aDispatcher, cmnLink *aLink)
{
    cmnDispatcherIPCDA  *sDispatcher = (cmnDispatcherIPCDA *)aDispatcher;
    PDL_SOCKET         sHandle;

    /* Dispatcher에서 사용할 수 있는 Link Impl인지 검사 (PROJ-2681) */
    IDE_TEST_RAISE(cmiDispatcherImplForLink(aLink) != aDispatcher->mImpl, InvalidLinkImpl);

    /* Dispatcher의 Link List에 추가 */
    IDE_TEST(cmnDispatcherAddLink(aDispatcher, aLink) != IDE_SUCCESS);

    /* Link의 socket 획득 */
    IDE_TEST(aLink->mOp->mGetHandle(aLink, &sHandle) != IDE_SUCCESS);

    /* MaxHandle 세팅 */
    if ((sDispatcher->mMaxHandle == PDL_INVALID_SOCKET) ||
        (sDispatcher->mMaxHandle < sHandle))
    {
        sDispatcher->mMaxHandle = sHandle;
    }

    /* FdSet에 socket 세팅 */
    FD_SET(sHandle, &sDispatcher->mFdSet);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidLinkImpl)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_LINK_IMPL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveLinkIPCDA(cmnDispatcher */*aDispatcher*/, cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer*)aLink;

    /* bug-28277 ipc: server stop failed when idle clis exist
     * server stop시에만 shutdown_mode_force 넘기도록 함. */
    IDE_TEST(sLink->mPeerOp->mShutdown(sLink, CMN_DIRECTION_RDWR,
                                       CMN_SHUTDOWN_MODE_NORMAL)
             != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnDispatcherRemoveAllLinksIPCDA(cmnDispatcher * aDispatcher)
{
    cmnDispatcherIPCDA *sDispatcher = (cmnDispatcherIPCDA *)aDispatcher;

    IDE_TEST(cmnDispatcherRemoveAllLinks(aDispatcher) != IDE_SUCCESS);

    sDispatcher->mMaxHandle  = PDL_INVALID_SOCKET;

    FD_ZERO(&sDispatcher->mFdSet);


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnDispatcherSelectIPCDA(cmnDispatcher  *aDispatcher,
                                iduList        *aReadyList,
                                UInt           *aReadyCount,
                                PDL_Time_Value *aTimeout)
{
    cmnDispatcherIPCDA  *sDispatcher = (cmnDispatcherIPCDA *)aDispatcher;
    iduListNode         *sIterator;
    cmnLink             *sLink;
    PDL_SOCKET           sHandle;
    SInt                 sResult;

    IDU_LIST_INIT(aReadyList);

    /* select 수행 */
    sResult = idlOS::select(sDispatcher->mMaxHandle + 1,
            &sDispatcher->mFdSet,
            NULL,
            NULL,
            aTimeout);

    IDE_TEST_RAISE(sResult < 0, SelectError);

    /* Ready Count 세팅 */
    if (aReadyCount != NULL)
    {
        *aReadyCount = sResult;
    }

    /* Ready Link 검색 */
    IDU_LIST_ITERATE(&aDispatcher->mLinkList, sIterator)
    {
        sLink = (cmnLink *)sIterator->mObj;

        /* Link의 socket을 획득 */
        IDE_TEST(sLink->mOp->mGetHandle(sLink, &sHandle) != IDE_SUCCESS);

        /* ready 검사 */
        if (FD_ISSET(sHandle, &sDispatcher->mFdSet))
        {
            IDU_LIST_ADD_LAST(aReadyList, &sLink->mReadyListNode);
        }
        else
        {
            FD_SET(sHandle, &sDispatcher->mFdSet);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SelectError);
    {
        /* BUG-47714 에러 메세지에 sock number 추가 */
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SELECT_ERROR, sDispatcher->mMaxHandle));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

struct cmnDispatcherOP gCmnDispatcherOpIPCDA =
{
    (SChar *)"IPCDA",

    cmnDispatcherInitializeIPCDA,
    cmnDispatcherFinalizeIPCDA,

    cmnDispatcherAddLinkIPCDA,
    cmnDispatcherRemoveLinkIPCDA,
    cmnDispatcherRemoveAllLinksIPCDA,

    cmnDispatcherSelectIPCDA
};


IDE_RC cmnDispatcherMapIPCDA(cmnDispatcher *aDispatcher)
{
    /* 함수 포인터 세팅 */
    aDispatcher->mOp = &gCmnDispatcherOpIPCDA;

    return IDE_SUCCESS;
}

UInt cmnDispatcherSizeIPCDA()
{
    return ID_SIZEOF(cmnDispatcherIPCDA);
}

IDE_RC cmnDispatcherWaitLinkIPCDA(cmnLink         *aLink,
                                  cmnDirection     aDirection,
                                  PDL_Time_Value  *aTimeout)
{
    IDE_RC sRet = IDE_SUCCESS;

    PDL_Time_Value  sSleepTime;

    /* bug-27250 free Buf list can be crushed when client killed */
    if (aDirection == CMN_DIRECTION_WR)
    {
        /* receiver가 송신 허락 신호를 줄때까지 무한 대기
         * cmiWriteBlock에서 protocol end packet 송신시
         * pending block이 있는 경우 이 코드 수행 */
        if (aTimeout == NULL)
        {
            /* cmnLinkPeerIPCDA (defined in cmnLinkPeerIPCDA.cpp)
             * 구조체를 직접 접근할 수 없어서,한번더 호출처리. */
            sRet = cmnLinkPeerWaitSendServerIPCDA(aLink);
        }
        /* cmiWriteBlock에서 송신 대기 list를 넘어선 경우 수행. */
        else
        {
            sSleepTime.set(0, 1000); /* wait 1 msec */
            idlOS::sleep(sSleepTime);
        }
    }
    return sRet;
}
#endif
