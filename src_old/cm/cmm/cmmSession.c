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
    acp_uint16_t    mSlotID;
    acp_uint16_t    mNextFreeSlotID;
    cmmSession     *mSession;
} cmmSessionSlot;

typedef struct cmmSessionPage
{
    acp_uint16_t    mPageID;
    acp_uint16_t    mSlotUseCount;
    acp_uint16_t    mFirstFreeSlotID;
    acp_uint16_t    mNextFreePageID;
    cmmSessionSlot *mSlot;
} cmmSessionPage;

typedef struct cmmSessionPageTable
{
    acp_uint16_t    mFirstFreePageID;
    acp_thr_mutex_t mMutex;
    cmmSessionPage  mPage[CMM_SESSION_ID_PAGE_MAX];
} cmmSessionPageTable;


static cmmSessionPageTable gPageTable;


static ACI_RC cmmSessionAllocPage(cmmSessionPage *aPage)
{
    /*
     * Page에 Slot Array가 할당되어 있지 않으면 할당하고 초기화
     */

    if (aPage->mSlot == NULL)
    {
        acp_uint16_t sSlotID;

        /*
         * Slot Array 할당
         */

        ACI_TEST(acpMemAlloc((void **)&(aPage->mSlot),
                             ACI_SIZEOF(cmmSessionSlot) * CMM_SESSION_ID_SLOT_MAX)
                 != ACP_RC_SUCCESS);

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

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC cmmSessionFreePage(cmmSessionPage *aPage)
{
    /*
     * Page에 Slot이 하나도 사용되고 있지 않으면 Slot Array를 해제
     */

    if ((aPage->mPageID != 0) && (aPage->mSlot != NULL) && (aPage->mSlotUseCount == 0))
    {
        acpMemFree(aPage->mSlot);

        aPage->mFirstFreeSlotID = 0;
        aPage->mSlot            = NULL;
    }

    return ACI_SUCCESS;
}

static ACI_RC cmmSessionFindSlot(cmmSessionPage **aPage, cmmSessionSlot **aSlot, acp_uint16_t aSessionID)
{
    acp_uint16_t sPageID = CMM_SESSION_ID_PAGE(aSessionID);
    acp_uint16_t sSlotID = CMM_SESSION_ID_SLOT(aSessionID);

    /*
     * Page 검색
     */

    *aPage = &(gPageTable.mPage[sPageID]);

    /*
     * Page에 Slot Array가 할당되어 있는지 검사
     */

    ACI_TEST((*aPage)->mSlot == NULL);

    /*
     * Slot 검색
     */

    *aSlot = &((*aPage)->mSlot[sSlotID]);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}


ACI_RC cmmSessionInitializeStatic()
{
    cmmSession   sSession;
    acp_uint16_t sPageID;

    /*
     * cmmSession의 mSessionID 멤버 크기가 CMM_SESSION_ID_SIZE_BIT와 일치하는지 검사
     */
    ACE_ASSERT((ACI_SIZEOF(sSession.mSessionID) * 8) == CMM_SESSION_ID_SIZE_BIT);

    /*
     * PageTable 초기화
     */

    gPageTable.mFirstFreePageID = 0;

    ACI_TEST(acpThrMutexCreate(&gPageTable.mMutex, ACP_THR_MUTEX_DEFAULT)
             != ACP_RC_SUCCESS);

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

    ACI_TEST(cmmSessionAllocPage(&(gPageTable.mPage[0])) != ACI_SUCCESS);

    /*
     * SessionID = 0 에는 Session을 할당할 수 없음
     */

    gPageTable.mPage[0].mSlotUseCount    = 1;
    gPageTable.mPage[0].mFirstFreeSlotID = 1;

#ifdef CMM_SESSION_VERIFY
    cmmSessionVerify();
#endif

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmmSessionFinalizeStatic()
{
    acp_uint16_t sPageID;

    /*
     * Page들의 Slot Array 메모리를 해제
     */

    for (sPageID = 0; sPageID < CMM_SESSION_ID_PAGE_MAX; sPageID++)
    {
        if (gPageTable.mPage[sPageID].mSlot != NULL)
        {
            acpMemFree(gPageTable.mPage[sPageID].mSlot);

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

    ACI_TEST(acpThrMutexDestroy(&gPageTable.mMutex) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmmSessionAdd(cmmSession *aSession)
{
    acp_uint16_t    sPageID;
    acp_uint16_t    sSlotID;
    cmmSessionPage *sPage;
    cmmSessionSlot *sSlot;

    ACE_ASSERT(acpThrMutexLock(&gPageTable.mMutex) == ACP_RC_SUCCESS);

    /*
     * 할당가능한 Page가 없으면 실패
     */

    ACI_TEST_RAISE(gPageTable.mFirstFreePageID == CMM_SESSION_ID_PAGE_MAX, SessionLimitReach);

    /*
     * 할당가능한 Page 검색
     */

    sPageID = gPageTable.mFirstFreePageID;
    sPage   = &(gPageTable.mPage[sPageID]);

    /*
     * Page 메모리 할당
     */

    ACI_TEST(cmmSessionAllocPage(sPage) != ACI_SUCCESS);

    /*
     * Page내의 빈 Slot 검색
     */

    sSlotID = sPage->mFirstFreeSlotID;
    sSlot   = &(sPage->mSlot[sSlotID]);

    /*
     * Slot이 비어있는지 확인
     */

    ACE_ASSERT(sSlot->mSession == NULL);

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

    ACE_ASSERT(acpThrMutexUnlock(&gPageTable.mMutex) == ACP_RC_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SessionLimitReach);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SESSION_LIMIT_REACH));
    }
    ACI_EXCEPTION_END;
    {
#ifdef CMM_SESSION_VERIFY
        cmmSessionVerify();
#endif
    }

    ACE_ASSERT(acpThrMutexUnlock(&gPageTable.mMutex) == ACP_RC_SUCCESS);

    return ACI_FAILURE;
}

ACI_RC cmmSessionRemove(cmmSession *aSession)
{
    acp_uint16_t    sPageID;
    acp_uint16_t    sSlotID;
    cmmSessionPage *sPage;
    cmmSessionSlot *sSlot;

    ACE_ASSERT(acpThrMutexLock(&gPageTable.mMutex) == ACP_RC_SUCCESS);

    /*
     * Page 및 Slot 검색
     */

    ACI_TEST_RAISE(cmmSessionFindSlot(&sPage, &sSlot, aSession->mSessionID) != ACI_SUCCESS, SessionNotFound);

    sPageID = sPage->mPageID;
    sSlotID = sSlot->mSlotID;

    /*
     * Slot에 저장된 Session이 일치하는지 검사
     */

    ACI_TEST_RAISE(sSlot->mSession != aSession, InvalidSession);

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
            ACI_TEST(cmmSessionFreePage(&gPageTable.mPage[sPage->mNextFreePageID]) != ACI_SUCCESS);
        }
    }

    /*
     * Page가 empty이고 First FreePage가 아니면 Page 메모리 해제
     */

    if ((sPageID != 0) && (sPage->mSlotUseCount == 0) && (gPageTable.mFirstFreePageID != sPageID))
    {
        ACI_TEST(cmmSessionFreePage(sPage) != ACI_SUCCESS);
    }

#ifdef CMM_SESSION_VERIFY
    cmmSessionVerify();
#endif

    ACE_ASSERT(acpThrMutexUnlock(&gPageTable.mMutex) == ACP_RC_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SessionNotFound);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SESSION_DOES_NOT_EXIST));
    }
    ACI_EXCEPTION(InvalidSession);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_SESSION));
    }
    ACI_EXCEPTION_END;
    {
#ifdef CMM_SESSION_VERIFY
        cmmSessionVerify();
#endif

        ACE_ASSERT(acpThrMutexUnlock(&gPageTable.mMutex) == ACP_RC_SUCCESS);
    }

    return ACI_FAILURE;
}

ACI_RC cmmSessionFind(cmmSession **aSession, acp_uint16_t aSessionID)
{
    cmmSessionPage *sPage;
    cmmSessionSlot *sSlot;

    ACE_ASSERT(acpThrMutexLock(&gPageTable.mMutex) == ACP_RC_SUCCESS);

    /*
     * Page 및 Slot 검색
     */

    ACI_TEST(cmmSessionFindSlot(&sPage, &sSlot, aSessionID) != ACI_SUCCESS);

    /*
     * Slot에 Session이 존재하는지 검사
     */

    ACI_TEST(sSlot->mSession == NULL);

    /*
     * Slot의 Session을 돌려줌
     */

    *aSession = sSlot->mSession;

    ACE_ASSERT(acpThrMutexUnlock(&gPageTable.mMutex) == ACP_RC_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SESSION_DOES_NOT_EXIST));
        ACE_ASSERT(acpThrMutexUnlock(&gPageTable.mMutex) == ACP_RC_SUCCESS);
    }

    return ACI_FAILURE;
}
