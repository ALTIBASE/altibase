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
 * cmmSession 객체를 관리하는 구조
 *
 * Slot      : cmmSession 객체 포인터와 Next Free SessionID를 가지는 구조체
 * Page      : SessionSlot을 CMM_SESSION_ID_SLOT_MAX개만큼 가지는 메모리 블럭
 * PageTable : SessionPage를 CMM_SESSION_ID_PAGE_MAX개만큼 가지는 메모리 블럭
 *
 *   +------+      +------+------+----
 *   | Page | ---> | Slot | Slot |
 *   +------+      +------+------+----
 *   | Page |
 *   +------+
 *   |      |
 *
 *   PageTable
 */

#define CMM_SESSION_ID_SIZE_BIT  16
#define CMM_SESSION_ID_PAGE_BIT  7
#define CMM_SESSION_ID_SLOT_BIT  (CMM_SESSION_ID_SIZE_BIT - CMM_SESSION_ID_PAGE_BIT)

#define CMM_SESSION_ID_PAGE_MAX  (1 << CMM_SESSION_ID_PAGE_BIT)
#define CMM_SESSION_ID_SLOT_MAX  (1 << CMM_SESSION_ID_SLOT_BIT)

#define CMM_SESSION_ID_PAGE_MASK ((CMM_SESSION_ID_PAGE_MAX - 1) << CMM_SESSION_ID_SLOT_BIT)
#define CMM_SESSION_ID_SLOT_MASK (CMM_SESSION_ID_SLOT_MAX - 1)

#define CMM_SESSION_ID_PAGE(aSessionID) (((aSessionID) >> CMM_SESSION_ID_SLOT_BIT) & (CMM_SESSION_ID_PAGE_MAX - 1))
#define CMM_SESSION_ID_SLOT(aSessionID) ((aSessionID) & CMM_SESSION_ID_SLOT_MASK)

#define CMM_SESSION_ID(aPage, aSlot)    (((aPage) << CMM_SESSION_ID_SLOT_BIT) | ((aSlot) & CMM_SESSION_ID_SLOT_MASK))


typedef struct cmmSessionSlot
{
    UShort          mSlotID;
    UShort          mNextFreeSlotID;
    cmmSession     *mSession;
} cmmSessionSlot;

typedef struct cmmSessionPage
{
    UShort          mPageID;
    UShort          mSlotUseCount;
    UShort          mFirstFreeSlotID;
    UShort          mNextFreePageID;
    cmmSessionSlot *mSlot;
} cmmSessionPage;

typedef struct cmmSessionPageTable
{
    UShort          mFirstFreePageID;
    iduMutex        mMutex;
    cmmSessionPage  mPage[CMM_SESSION_ID_PAGE_MAX];
} cmmSessionPageTable;


static cmmSessionPageTable gPageTable;


static IDE_RC cmmSessionAllocPage(cmmSessionPage *aPage)
{
    /*
     * Page에 Slot Array가 할당되어 있지 않으면 할당하고 초기화
     */

    if (aPage->mSlot == NULL)
    {
        UShort sSlotID;

        IDU_FIT_POINT_RAISE( "cmmSession::cmmSessionAllocPage::malloc::Slot",
                              InsufficientMemory );
        /*
         * Slot Array 할당
         */

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMM,
                                         ID_SIZEOF(cmmSessionSlot) * CMM_SESSION_ID_SLOT_MAX,
                                         (void **)&(aPage->mSlot),
                                         IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        /*
         * 각 Slot 초기화
         */

        for (sSlotID = 0; sSlotID < CMM_SESSION_ID_SLOT_MAX; sSlotID++)
        {
            aPage->mSlot[sSlotID].mSlotID         = sSlotID;
            aPage->mSlot[sSlotID].mNextFreeSlotID = sSlotID + 1;
            aPage->mSlot[sSlotID].mSession        = NULL;
        }

        /*
         * Page의 Slot 속성값 초기화
         */

        aPage->mSlotUseCount    = 0;
        aPage->mFirstFreeSlotID = 0;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }    
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC cmmSessionFreePage(cmmSessionPage *aPage)
{
    /*
     * Page에 Slot이 하나도 사용되고 있지 않으면 Slot Array를 해제
     */

    if ((aPage->mPageID != 0) && (aPage->mSlot != NULL) && (aPage->mSlotUseCount == 0))
    {
        IDE_TEST(iduMemMgr::free(aPage->mSlot) != IDE_SUCCESS);

        aPage->mFirstFreeSlotID = 0;
        aPage->mSlot            = NULL;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC cmmSessionFindSlot(cmmSessionPage **aPage, cmmSessionSlot **aSlot, UShort aSessionID)
{
    UShort sPageID = CMM_SESSION_ID_PAGE(aSessionID);
    UShort sSlotID = CMM_SESSION_ID_SLOT(aSessionID);

    /*
     * Page 검색
     */

    *aPage = &(gPageTable.mPage[sPageID]);

    /*
     * Page에 Slot Array가 할당되어 있는지 검사
     */

    IDE_TEST((*aPage)->mSlot == NULL);

    /*
     * Slot 검색
     */

    *aSlot = &((*aPage)->mSlot[sSlotID]);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC cmmSessionInitializeStatic()
{
    cmmSession sSession;
    UShort     sPageID;

    /*
     * cmmSession의 mSessionID 멤버 크기가 CMM_SESSION_ID_SIZE_BIT와 일치하는지 검사
     */
    IDE_ASSERT((ID_SIZEOF(sSession.mSessionID) * 8) == CMM_SESSION_ID_SIZE_BIT);

    /*
     * PageTable 초기화
     */

    gPageTable.mFirstFreePageID = 0;

    IDE_TEST(gPageTable.mMutex.initialize((SChar *)"CMM_SESSION_MUTEX",
                                          IDU_MUTEX_KIND_NATIVE,
                                          IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    /*
     * 각 Page 초기화
     */

    for (sPageID = 0; sPageID < CMM_SESSION_ID_PAGE_MAX; sPageID++)
    {
        gPageTable.mPage[sPageID].mPageID          = sPageID;
        gPageTable.mPage[sPageID].mSlotUseCount    = 0;
        gPageTable.mPage[sPageID].mFirstFreeSlotID = 0;
        gPageTable.mPage[sPageID].mNextFreePageID  = sPageID + 1;
        gPageTable.mPage[sPageID].mSlot            = NULL;
    }

    /*
     * 첫번째 Page 미리 할당
     */

    IDE_TEST(cmmSessionAllocPage(&(gPageTable.mPage[0])) != IDE_SUCCESS);

    /*
     * SessionID = 0 에는 Session을 할당할 수 없음
     */

    gPageTable.mPage[0].mSlotUseCount    = 1;
    gPageTable.mPage[0].mFirstFreeSlotID = 1;

#ifdef CMM_SESSION_VERIFY
    cmmSessionVerify();
#endif

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmmSessionFinalizeStatic()
{
    UShort sPageID;

    /*
     * Page들의 Slot Array 메모리를 해제
     */

    for (sPageID = 0; sPageID < CMM_SESSION_ID_PAGE_MAX; sPageID++)
    {
        if (gPageTable.mPage[sPageID].mSlot != NULL)
        {
            IDE_TEST(iduMemMgr::free(gPageTable.mPage[sPageID].mSlot)!=IDE_SUCCESS);

            gPageTable.mPage[sPageID].mSlotUseCount    = 0;
            gPageTable.mPage[sPageID].mFirstFreeSlotID = 0;
            gPageTable.mPage[sPageID].mNextFreePageID  = 0;
            gPageTable.mPage[sPageID].mSlot            = NULL;
        }
    }

    /*
     * PageTable 해제
     */

    gPageTable.mFirstFreePageID = 0;

    IDE_TEST(gPageTable.mMutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmmSessionAdd(cmmSession *aSession)
{
    UShort          sPageID;
    UShort          sSlotID;
    cmmSessionPage *sPage;
    cmmSessionSlot *sSlot;

    IDE_ASSERT( gPageTable.mMutex.lock(NULL /* idvSQL* */)
                == IDE_SUCCESS );

    /*
     * 할당가능한 Page가 없으면 실패
     */

    IDE_TEST_RAISE(gPageTable.mFirstFreePageID == CMM_SESSION_ID_PAGE_MAX, SessionLimitReach);

    /*
     * 할당가능한 Page 검색
     */

    sPageID = gPageTable.mFirstFreePageID;
    sPage   = &(gPageTable.mPage[sPageID]);

    /*
     * Page 메모리 할당
     */

    IDE_TEST(cmmSessionAllocPage(sPage) != IDE_SUCCESS);

    /*
     * Page내의 빈 Slot 검색
     */

    sSlotID = sPage->mFirstFreeSlotID;
    sSlot   = &(sPage->mSlot[sSlotID]);

    /*
     * Slot이 비어있는지 확인
     */

    IDE_ASSERT(sSlot->mSession == NULL);

    /*
     * Session 추가
     */

    sSlot->mSession      = aSession;
    aSession->mSessionID = CMM_SESSION_ID(sPageID, sSlotID);

    /*
     * UsedSlotCount 증가
     */

    sPage->mSlotUseCount++;

    /*
     * FreeSlot List 갱신
     */

    sPage->mFirstFreeSlotID = sSlot->mNextFreeSlotID;
    sSlot->mNextFreeSlotID  = CMM_SESSION_ID_SLOT_MAX;

    /*
     * Page가 full이면 FreePage List 갱신
     */

    if (sPage->mFirstFreeSlotID == CMM_SESSION_ID_SLOT_MAX)
    {
        gPageTable.mFirstFreePageID = sPage->mNextFreePageID;
        sPage->mNextFreePageID      = CMM_SESSION_ID_PAGE_MAX;
    }

#ifdef CMM_SESSION_VERIFY
    cmmSessionVerify();
#endif

    IDE_ASSERT(gPageTable.mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SessionLimitReach);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SESSION_LIMIT_REACH));
    }
    IDE_EXCEPTION_END;
    {
#ifdef CMM_SESSION_VERIFY
        cmmSessionVerify();
#endif
    }

    IDE_ASSERT(gPageTable.mMutex.unlock() == IDE_SUCCESS);

    return IDE_FAILURE;
}

IDE_RC cmmSessionRemove(cmmSession *aSession)
{
    UShort          sPageID;
    UShort          sSlotID;
    cmmSessionPage *sPage;
    cmmSessionSlot *sSlot;

    IDE_ASSERT(gPageTable.mMutex.lock(NULL /* idvSQL* */)
               == IDE_SUCCESS);

    /*
     * Page 및 Slot 검색
     */

    IDE_TEST_RAISE(cmmSessionFindSlot(&sPage, &sSlot, aSession->mSessionID) != IDE_SUCCESS, SessionNotFound);

    sPageID = sPage->mPageID;
    sSlotID = sSlot->mSlotID;

    /*
     * Slot에 저장된 Session이 일치하는지 검사
     */

    IDE_TEST_RAISE(sSlot->mSession != aSession, InvalidSession);

    /*
     * Session 삭제
     */

    sSlot->mSession      = NULL;
    aSession->mSessionID = 0;

    /*
     * UsedSlotCount 감소
     */

    sPage->mSlotUseCount--;

    /*
     * FreeSlot List 갱신
     */

    sSlot->mNextFreeSlotID  = sPage->mFirstFreeSlotID;
    sPage->mFirstFreeSlotID = sSlotID;

    /*
     * Page가 full이었으면 FreePage List 갱신
     */

    if (sPage->mSlotUseCount == (CMM_SESSION_ID_SLOT_MAX - 1))
    {
        sPage->mNextFreePageID      = gPageTable.mFirstFreePageID;
        gPageTable.mFirstFreePageID = sPageID;

        /*
         * Next FreePage가 empty이면 Page 메모리 해제
         */

        if (gPageTable.mPage[sPage->mNextFreePageID].mSlotUseCount == 0)
        {
            IDE_TEST(cmmSessionFreePage(&gPageTable.mPage[sPage->mNextFreePageID]) != IDE_SUCCESS);
        }
    }

    /*
     * Page가 empty이고 First FreePage가 아니면 Page 메모리 해제
     */

    if ((sPageID != 0) && (sPage->mSlotUseCount == 0) && (gPageTable.mFirstFreePageID != sPageID))
    {
        IDE_TEST(cmmSessionFreePage(sPage) != IDE_SUCCESS);
    }

#ifdef CMM_SESSION_VERIFY
    cmmSessionVerify();
#endif

    IDE_ASSERT(gPageTable.mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SessionNotFound);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SESSION_DOES_NOT_EXIST));
    }
    IDE_EXCEPTION(InvalidSession);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_SESSION));
    }
    IDE_EXCEPTION_END;
    {
#ifdef CMM_SESSION_VERIFY
        cmmSessionVerify();
#endif

        IDE_ASSERT(gPageTable.mMutex.unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

IDE_RC cmmSessionFind(cmmSession **aSession, UShort aSessionID)
{
    cmmSessionPage *sPage;
    cmmSessionSlot *sSlot;

    IDE_ASSERT(gPageTable.mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    /*
     * Page 및 Slot 검색
     */

    IDE_TEST(cmmSessionFindSlot(&sPage, &sSlot, aSessionID) != IDE_SUCCESS);

    /*
     * Slot에 Session이 존재하는지 검사
     */

    IDE_TEST(sSlot->mSession == NULL);

    /*
     * Slot의 Session을 돌려줌
     */

    *aSession = sSlot->mSession;

    IDE_ASSERT(gPageTable.mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SESSION_DOES_NOT_EXIST));

        IDE_ASSERT(gPageTable.mMutex.unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}
