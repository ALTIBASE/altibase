/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: smpAllocPageList.cpp 85392 2019-05-07 06:40:22Z seulki $
 **********************************************************************/

#include <smpDef.h>
#include <smmDef.h>
#include <smpReq.h>
#include <smpAllocPageList.h>
#include <smuProperty.h>
#include <smmManager.h>

/**********************************************************************
 * AllocPageList를 초기화한다.
 *
 * aAllocPageList : 초기화하려는 AllocPageList
 **********************************************************************/

void smpAllocPageList::initializePageListEntry(
                                         smpAllocPageListEntry* aAllocPageList )
{
    UInt sPageListID;

    IDE_DASSERT(aAllocPageList != NULL);

    for(sPageListID = 0; sPageListID < SMP_PAGE_LIST_COUNT; sPageListID++)
    {
        // AllocPageList 초기화
        aAllocPageList[sPageListID].mPageCount  = 0;
        aAllocPageList[sPageListID].mHeadPageID = SM_NULL_PID;
        aAllocPageList[sPageListID].mTailPageID = SM_NULL_PID;
        aAllocPageList[sPageListID].mMutex      = NULL;
    }


    return;
}

/**********************************************************************
 * Runtime Item을 NULL로 설정한다.
 * DISCARD/OFFLINE Tablespace에 속한 Table들에 대해 수행된다.
 **********************************************************************/

IDE_RC smpAllocPageList::setRuntimeNull(
                                    UInt                   aAllocPageListCount,
                                    smpAllocPageListEntry* aAllocPageListArray )
{
    UInt  sPageListID;

    IDE_DASSERT( aAllocPageListArray != NULL );

    for( sPageListID = 0;
         sPageListID < aAllocPageListCount;
         sPageListID++ )
    {

        aAllocPageListArray[sPageListID].mMutex = NULL;
    }

    return IDE_SUCCESS;
}

/**********************************************************************
 * AllocPageListEntry의 Runtime 정보(Mutex) 초기화
 *
 * aAllocPageList : 구축하려는 AllocPageList
 * aPageListID    : AllocPageList의 ID
 * aTableOID      : AllocPageList의 테이블 OID
 * aPageType      : List를 구성하는 Page 유형
 **********************************************************************/

IDE_RC smpAllocPageList::initEntryAtRuntime(
                                        smpAllocPageListEntry* aAllocPageList,
                                        smOID                  aTableOID,
                                        smpPageType            aPageType )
{
    UInt  sPageListID;
    SChar sBuffer[128];

    IDE_DASSERT( aAllocPageList != NULL );

    for( sPageListID = 0;
         sPageListID < SMP_PAGE_LIST_COUNT;
         sPageListID++ )
    {
        if(aPageType == SMP_PAGETYPE_FIX)
        {
            idlOS::snprintf( sBuffer, 128,
                             "FIXED_ALLOC_PAGE_LIST_MUTEX_%"
                             ID_XINT64_FMT"_%"ID_UINT32_FMT"",
                             (ULong)aTableOID, sPageListID );
        }
        else
        {
            idlOS::snprintf(sBuffer, 128,
                            "VAR_ALLOC_PAGE_LIST_MUTEX_%"
                            ID_XINT64_FMT"_%"ID_UINT32_FMT"",
                            (ULong)aTableOID, sPageListID);
        }

        /* smpAllocPageList_initEntryAtRuntime_malloc_Mutex.tc */
        IDU_FIT_POINT("smpAllocPageList::initEntryAtRuntime::malloc::Mutex");
        IDE_TEST(iduMemMgr::malloc(
                     IDU_MEM_SM_SMP,
                     sizeof(iduMutex),
                     (void **)&((aAllocPageList+sPageListID)->mMutex),
                     IDU_MEM_FORCE )
                 != IDE_SUCCESS);

        IDE_TEST((aAllocPageList+sPageListID)->mMutex->initialize(
                     sBuffer,
                     IDU_MUTEX_KIND_NATIVE,
                     IDV_WAIT_INDEX_NULL)
                 != IDE_SUCCESS);
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * AllocPageList의 Runtime 정보 해제
 *
 * aAllocPageList : 해제하려는 AllocPageList
 **********************************************************************/

IDE_RC smpAllocPageList::finEntryAtRuntime(
                                         smpAllocPageListEntry* aAllocPageList )
{
    IDE_DASSERT( aAllocPageList != NULL );

    // OFFLINE/DISCARD Tablespace는 mMutex가 NULL일 수 있다
    if ( aAllocPageList->mMutex != NULL )
    {
        IDE_TEST(aAllocPageList->mMutex->destroy() != IDE_SUCCESS);

        IDE_TEST(iduMemMgr::free(aAllocPageList->mMutex) != IDE_SUCCESS);

        aAllocPageList->mMutex = NULL;
    }
    else
    {
        // Do Nothing
        // OFFLINE/DISCARD Tablespace는 mRuntimeEntry가 NULL일 수 있다
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * aAllocPageHead~aAllocPageTail를 AllocPageList에 추가
 *
 * aTrans         : 작업하려는 트잰잭션 객체
 * aAllocPageList : 추가하려는 AllocPageList
 * aTableOID      : AllocPageList의 테이블 OID
 * aAllocPageHead : 추가하려는 Page의 Head
 * aAllocPageTail : 추가하려는 Page의 Tail
 **********************************************************************/

IDE_RC smpAllocPageList::addPageList( void*                  aTrans,
                                      scSpaceID              aSpaceID,
                                      smpAllocPageListEntry* aAllocPageList,
                                      smOID                  aTableOID,
                                      smpPersPage*           aAllocPageHead,
                                      smpPersPage*           aAllocPageTail,
                                      UInt                   aAllocPageCnt )
{
    UInt         sState = 0;
    smpPersPage* sTailPagePtr = NULL;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aAllocPageList != NULL );
    IDE_DASSERT( aAllocPageHead != NULL );
    IDE_DASSERT( aAllocPageTail != NULL );

    IDE_DASSERT( isValidPageList( aSpaceID,
                                  aAllocPageList->mHeadPageID,
                                  aAllocPageList->mTailPageID,
                                  aAllocPageList->mPageCount )
                 == ID_TRUE );

    IDE_DASSERT( isValidPageList( aSpaceID,
                                  aAllocPageHead->mHeader.mSelfPageID,
                                  aAllocPageTail->mHeader.mSelfPageID,
                                  aAllocPageCnt )
                 == ID_TRUE );

    if(aAllocPageList->mHeadPageID == SM_NULL_PID)
    {
        // Head가 비었으면 그대로 aAllocPageHead~aAllocPageTail까지
        // AllocPageList의 Head~Tail로 대치한다.

        IDE_DASSERT( aAllocPageList->mTailPageID == SM_NULL_PID );

        IDE_TEST( smrUpdate::updateAllocInfoAtTableHead(
                      NULL, /* idvSQL* */
                      aTrans,
                      SM_MAKE_PID(aTableOID),
                      makeOffsetAllocPageList(aTableOID, aAllocPageList),
                      aAllocPageList,
                      aAllocPageList->mPageCount + aAllocPageCnt,
                      aAllocPageHead->mHeader.mSelfPageID,
                      aAllocPageTail->mHeader.mSelfPageID
                      ) != IDE_SUCCESS );


        aAllocPageList->mHeadPageID = aAllocPageHead->mHeader.mSelfPageID;
        aAllocPageList->mTailPageID = aAllocPageTail->mHeader.mSelfPageID;
    }
    else
    {
        // AllocPageList의 Tail뒤에 sAllocPageHead~sAllocPageTail을 붙인다.
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                                aAllocPageList->mTailPageID,
                                                (void**)&sTailPagePtr )
                    == IDE_SUCCESS );
 
        IDE_TEST( smrUpdate::updateLinkAtPersPage(
                      NULL, /* idvSQL* */
                      aTrans,
                      aSpaceID,
                      sTailPagePtr->mHeader.mSelfPageID,
                      sTailPagePtr->mHeader.mPrevPageID,
                      sTailPagePtr->mHeader.mNextPageID,
                      sTailPagePtr->mHeader.mPrevPageID,
                      aAllocPageHead->mHeader.mSelfPageID
                      ) != IDE_SUCCESS );

        sState = 1;

        // Tail뒤에 sAllocPageHead를 붙이고

        sTailPagePtr->mHeader.mNextPageID = aAllocPageHead->mHeader.mSelfPageID;

        sState = 0;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                sTailPagePtr->mHeader.mSelfPageID)
                  != IDE_SUCCESS );

        IDE_TEST( smrUpdate::updateLinkAtPersPage(
                      NULL, /* idvSQL* */
                      aTrans,
                      aSpaceID,
                      aAllocPageHead->mHeader.mSelfPageID,
                      aAllocPageHead->mHeader.mPrevPageID,
                      aAllocPageHead->mHeader.mNextPageID,
                      aAllocPageList->mTailPageID,
                      aAllocPageHead->mHeader.mNextPageID
                      ) != IDE_SUCCESS );

        sState = 2;

        // sAllocPageHead 앞에 Tail을 붙인다.

        aAllocPageHead->mHeader.mPrevPageID = aAllocPageList->mTailPageID;

        sState = 0;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                aAllocPageHead->mHeader.mSelfPageID)
                  != IDE_SUCCESS );

        IDE_TEST( smrUpdate::updateAllocInfoAtTableHead(
                      NULL, /* idvSQL* */
                      aTrans,
                      SM_MAKE_PID(aTableOID),
                      makeOffsetAllocPageList(aTableOID, aAllocPageList),
                      aAllocPageList,
                      aAllocPageList->mPageCount + aAllocPageCnt,
                      aAllocPageList->mHeadPageID,
                      aAllocPageTail->mHeader.mSelfPageID
                      ) != IDE_SUCCESS );


        aAllocPageList->mTailPageID = aAllocPageTail->mHeader.mSelfPageID;
    }

    //  PageListEntry 정보 갱신
    aAllocPageList->mPageCount += aAllocPageCnt;

    IDE_DASSERT( isValidPageList( aSpaceID,
                                  aAllocPageList->mHeadPageID,
                                  aAllocPageList->mTailPageID,
                                  aAllocPageList->mPageCount )
                 == ID_TRUE );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            SM_MAKE_PID(aTableOID))
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                  sTailPagePtr->mHeader.mSelfPageID)
                    == IDE_SUCCESS );
    }

    if(sState == 2)
    {
        IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(
                        aSpaceID,
                        aAllocPageHead->mHeader.mSelfPageID)
                    == IDE_SUCCESS );
    }

    IDE_ASSERT( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                               SM_MAKE_PID(aTableOID) )
                == IDE_SUCCESS );

    IDE_POP();


    return IDE_FAILURE;
}

/**********************************************************************
 * AllocPageList제거 및 DB로 반납
 *
 * FreePage를 제거하기 위해서는 다른 Tx가 접근해서는 안된다.
 * refineDB때와 같이 하나의 Tx만 접근하다.
 *
 * aTrans         : 작업하려는 트랜잭션 객체
 * aAllocPageList : 제거하려는 AllocPageList
 * aTableOID      : 제거하려는 테이블 OID
 * aDropFlag      : DROP TABLE -> ID_TRUE
 *                  ALTER TABLE -> ID_FALSE
 **********************************************************************/

IDE_RC smpAllocPageList::freePageListToDB(
    void*                  aTrans,
    scSpaceID              aSpaceID,
    smpAllocPageListEntry* aAllocPageList,
    smOID                  aTableOID)
{
    UInt                   sState = 0;
    smLSN                  sLsnNTA;
    smpPersPage*           sHead;
    smpPersPage*           sTail;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aAllocPageList != NULL );

    IDE_DASSERT( isValidPageList( aSpaceID,
                                  aAllocPageList->mHeadPageID,
                                  aAllocPageList->mTailPageID,
                                  aAllocPageList->mPageCount )
                 == ID_TRUE );

    if( aAllocPageList->mHeadPageID != SM_NULL_PID )
    {
        // Head가 NULL이라면 AllocPageList가 비었다.
        IDE_DASSERT( aAllocPageList->mTailPageID != SM_NULL_PID );

        // for NTA
        sLsnNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

        IDE_TEST( smrUpdate::updateAllocInfoAtTableHead(
                      NULL, /* idvSQL* */
                      aTrans,
                      SM_MAKE_PID(aTableOID),
                      makeOffsetAllocPageList(aTableOID, aAllocPageList),
                      aAllocPageList,
                      0,
                      SM_NULL_PID,
                      SM_NULL_PID
                      ) != IDE_SUCCESS );

        sState = 1;

        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                aAllocPageList->mHeadPageID,
                                                (void**)&sHead )
                    == IDE_SUCCESS );
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                aAllocPageList->mTailPageID,
                                                (void**)&sTail )
                    == IDE_SUCCESS );
 
        aAllocPageList->mPageCount  = 0;
        aAllocPageList->mHeadPageID = SM_NULL_PID;
        aAllocPageList->mTailPageID = SM_NULL_PID;

        IDE_TEST( smmManager::freePersPageList( aTrans,
                                                aSpaceID,
                                                sHead,
                                                sTail,
                                                &sLsnNTA )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                SM_MAKE_PID(aTableOID))
                  != IDE_SUCCESS);
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        (void)smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            SM_MAKE_PID(aTableOID));
    }

    IDE_POP();

    return IDE_FAILURE;
}

/**********************************************************************
 * AllocPageList에서 aPageID 제거하여 DB로 반납
 *
 * aTrans         : 작업하려는 트랜잭션 객체
 * aAllocPageList : 제거하려는 AllocPageList
 * aPageID        : 제거하려는 Page ID
 * aTableOID      : 제거하려는 테이블 OID
 **********************************************************************/

IDE_RC smpAllocPageList::removePage( void*                  aTrans,
                                     scSpaceID              aSpaceID,
                                     smpAllocPageListEntry* aAllocPageList,
                                     scPageID               aPageID,
                                     smOID                  aTableOID )
{
    UInt                   sState = 0;
    idBool                 sLocked = ID_FALSE;
    scPageID               sPrevPageID;
    scPageID               sNextPageID;
    scPageID               sListHeadPID;
    scPageID               sListTailPID;
    smpPersPageHeader*     sPrevPageHeader;
    smpPersPageHeader*     sNextPageHeader;
    smpPersPageHeader*     sCurPageHeader;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aAllocPageList != NULL );

    IDE_TEST( aAllocPageList->mMutex->lock( NULL /* idvSQL* */ ) != IDE_SUCCESS );
    sLocked = ID_TRUE;

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                            aPageID,
                                            (void**)&sCurPageHeader )
                == IDE_SUCCESS );
    sPrevPageID    = sCurPageHeader->mPrevPageID;
    sNextPageID    = sCurPageHeader->mNextPageID;
    sListHeadPID   = aAllocPageList->mHeadPageID;
    sListTailPID   = aAllocPageList->mTailPageID;

    if(sPrevPageID == SM_NULL_PID)
    {
        // PrevPage가 NULL이라면 CurPage는 PageList의 Head이다.
        IDE_DASSERT( aPageID == sListHeadPID );

        sListHeadPID = sNextPageID;
    }
    else
    {
        // 앞쪽 링크를 끊어준다.
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                sPrevPageID,
                                                (void**)&sPrevPageHeader )
                    == IDE_SUCCESS );

        IDE_TEST( smrUpdate::updateLinkAtPersPage(
                      NULL /* idvSQL* */,
                      aTrans,
                      aSpaceID,
                      sPrevPageID,
                      sPrevPageHeader->mPrevPageID,
                      sPrevPageHeader->mNextPageID,
                      sPrevPageHeader->mPrevPageID,
                      sNextPageID
                      ) != IDE_SUCCESS );

        sState = 1;

        sPrevPageHeader->mNextPageID = sNextPageID;

        sState = 0;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID, sPrevPageID)
                  != IDE_SUCCESS );
    }

    if(sNextPageID == SM_NULL_PID)
    {
        // NextPage가 NULL이라면 CurPage는 PageList의 Tail이다.
        IDE_DASSERT( aPageID == sListTailPID );

        sListTailPID = sPrevPageID;
    }
    else
    {
        // 뒤쪽 링크를 끊어준다.
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                sNextPageID,
                                                (void**)&sNextPageHeader )
                    == IDE_SUCCESS );

        IDE_TEST( smrUpdate::updateLinkAtPersPage(
                      NULL /* idvSQL* */,
                      aTrans,
                      aSpaceID,
                      sNextPageID,
                      sNextPageHeader->mPrevPageID,
                      sNextPageHeader->mNextPageID,
                      sPrevPageID,
                      sNextPageHeader->mNextPageID
                      ) != IDE_SUCCESS );

        sState = 2;

        sNextPageHeader->mPrevPageID = sPrevPageID;

        IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID, sNextPageID)
                  != IDE_SUCCESS );
        sState = 0;
    }

    // AllocPageList의 Head/Tail/PageCount를 수정한다.
    IDE_TEST( smrUpdate::updateAllocInfoAtTableHead(
                  NULL /* idvSQL* */,
                  aTrans,
                  SM_MAKE_PID(aTableOID),
                  makeOffsetAllocPageList(aTableOID, aAllocPageList),
                  aAllocPageList,
                  aAllocPageList->mPageCount - 1,
                  sListHeadPID,
                  sListTailPID
                  ) != IDE_SUCCESS );

    sState = 3;

    aAllocPageList->mHeadPageID = sListHeadPID;
    aAllocPageList->mTailPageID = sListTailPID;
    aAllocPageList->mPageCount--;

    sState = 0;
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            SM_MAKE_PID(aTableOID))
              != IDE_SUCCESS );

    sLocked = ID_FALSE;
    IDE_TEST( aAllocPageList->mMutex->unlock() != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sState)
    {
        case 3:
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                      SM_MAKE_PID(aTableOID))
                        == IDE_SUCCESS );
            break;

        case 2:
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                      sNextPageID)
                        == IDE_SUCCESS );
            break;

        case 1:
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                      sPrevPageID)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    if(sLocked == ID_TRUE)
    {
        IDE_ASSERT( aAllocPageList->mMutex->unlock() == IDE_SUCCESS );
    }

    IDE_POP();


    return IDE_FAILURE;
}

/**********************************************************************
 * aHeadPage~aTailPage까지의 PageList의 연결이 올바른지 검사한다.
 *
 * aHeadPage  : 검사하려는 List의 Head
 * aTailPage  : 검사하려는 List의 Tail
 * aPageCount : 검사하려는 List의 Page의 갯수
 **********************************************************************/

idBool smpAllocPageList::isValidPageList( scSpaceID aSpaceID,
                                          scPageID  aHeadPage,
                                          scPageID  aTailPage,
                                          vULong    aPageCount )
{
    idBool     sIsValid;
    vULong     sPageCount = 0;
    scPageID   sCurPage = SM_NULL_PID;
    scPageID   sNxtPage;
    SChar    * sPagePtr;

    sIsValid = ID_FALSE;

    sNxtPage = aHeadPage;

    while(sNxtPage != SM_NULL_PID)
    {
        sCurPage = sNxtPage;

        sPageCount++;

        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                sCurPage,
                                                (void**)&sPagePtr )
                    == IDE_SUCCESS );
        sNxtPage = SMP_GET_NEXT_PERS_PAGE_ID( sPagePtr );
        
    }

    if(sCurPage == aTailPage)
    {
        if(sPageCount == aPageCount)
        {
            sIsValid = ID_TRUE;
        }
    }


    return sIsValid;
}

/**********************************************************************
 * aAllocPageList내의 모든 Row를 순회하면서 유효한 Row를 찾아준다.
 *
 * aAllocPageList : 순회하려는 PageListEntry
 * aCurRow        : 찾기시작하려는 Row
 * aNxtRow        : 다음 유효한 Row를 찾아서 반환
 **********************************************************************/

IDE_RC smpAllocPageList::nextOIDall( scSpaceID              aSpaceID,
                                     smpAllocPageListEntry* aAllocPageList,
                                     SChar*                 aCurRow,
                                     vULong                 aSlotSize,
                                     SChar**                aNxtRow )
{
    scPageID            sNxtPID;
    smpPersPage       * sPage;
    SChar             * sNxtPtr;
    SChar             * sFence;
#if defined(DEBUG_SMP_NEXTOIDALL_PAGE_CHECK)
    idBool              sIsFreePage ;
    smpPersPageHeader * sPageHdr;
#endif // DEBUG_SMP_NEXTOIDALL_PAGE_CHECK

    IDE_DASSERT( aAllocPageList != NULL );
    IDE_DASSERT( aNxtRow != NULL );

    initForScan( aSpaceID,
                 aAllocPageList,
                 aCurRow,
                 aSlotSize,
                 &sPage,
                 &sNxtPtr );

    *aNxtRow = NULL;


    while(sPage != NULL)
    {

#if defined(DEBUG_SMP_NEXTOIDALL_PAGE_CHECK)
        // nextOIDall 스캔 대상이 되는 페이지는
        // 테이블에 할당된 Page이므로 Free Page일 수 없다.
        sPageHdr = (smpPersPageHeader *)sPage;
        IDE_TEST( smmExpandChunk::isFreePageID( sPageHdr->mSelfPageID,
                                                & sIsFreePage )
                  != IDE_SUCCESS );
        IDE_DASSERT( sIsFreePage == ID_FALSE );
#endif // DEBUG_SMP_NEXTOIDALL_PAGE_CHECK

        // 해당 Page에 대한 포인터 경계
        sFence = (SChar *)((smpPersPage *)sPage + 1);

        for( ;
             sNxtPtr+aSlotSize <= sFence;
             sNxtPtr += aSlotSize )
        {
            // In case of free slot
            if( SMP_SLOT_IS_NOT_USED( (smpSlotHeader*)sNxtPtr ) )
            {
                continue;
            }

            *aNxtRow = sNxtPtr;

            IDE_CONT(normal_case);
        } /* for */

        sNxtPID = getNextAllocPageID( aSpaceID,
                                      aAllocPageList,
                                      sPage->mHeader.mSelfPageID );

        if(sNxtPID == SM_NULL_PID)
        {
            // NextPage가 NULL이면 끝이다.
            IDE_CONT(normal_case);
        }

        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                sNxtPID,
                                                (void**)&sPage )
                    == IDE_SUCCESS );

        sNxtPtr  = (SChar *)sPage->mBody;
    }

    IDE_EXCEPTION_CONT( normal_case );


    return IDE_SUCCESS;

#if defined(DEBUG_SMP_NEXTOIDALL_PAGE_CHECK)

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#endif // DEBUG_SMP_NEXTOIDALL_PAGE_CHECK
}

/***********************************************************************
 * nextOIDall을 위해 aRow에서 해당 Page를 찾아준다.
 *
 * aAllocPageList : 순회하려는 PageListEntry
 * aRow           : 현재 Row
 * aPage          : aRow가 속한 Page를 찾아서 반환
 * aRowPtr        : aRow 다음 Row 포인터
 ***********************************************************************/

inline void smpAllocPageList::initForScan(
    scSpaceID              aSpaceID,
    smpAllocPageListEntry* aAllocPageList,
    SChar*                 aRow,
    vULong                 aSlotSize,
    smpPersPage**          aPage,
    SChar**                aRowPtr )
{
    scPageID sPageID;

    IDE_DASSERT( aAllocPageList != NULL );
    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( aRowPtr != NULL );

    *aPage   = NULL;
    *aRowPtr = NULL;

    if(aRow != NULL)
    {
        sPageID = SMP_SLOT_GET_PID(aRow);
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                sPageID,
                                                (void**)aPage )
                    == IDE_SUCCESS );

        *aRowPtr = aRow + aSlotSize;
    }
    else
    {
        sPageID = getFirstAllocPageID(aAllocPageList);

        if(sPageID != SM_NULL_PID)
        {
            IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                    sPageID,
                                                    (void**)aPage )
                        == IDE_SUCCESS );

            *aRowPtr = (SChar *)((*aPage)->mBody);
        }
        else
        {
            /* Allcate된 페이지가 존재하지 않는다.*/
        }
    }
}

/**********************************************************************
 * aAllocPageList에서 aPageID 다음 PageID
 *
 * 다중화되어있는 AllocPageList에서 aPageID가 Tail이라면
 * aPageID의 다음 Page는 다음 리스트의 Head가 된다.
 *
 * aAllocPageList : 탐색하려는 PageListEntry
 * aPageID        : 탐색하려는 PageID
 **********************************************************************/

scPageID smpAllocPageList::getNextAllocPageID(
    scSpaceID              aSpaceID,
    smpAllocPageListEntry* aAllocPageList,
    scPageID               aPageID )
{
    scPageID            sNextPageID;
    smpPersPageHeader * sCurPageHeader;

    IDE_DASSERT( aAllocPageList != NULL );

    if(aPageID == SM_NULL_PID)
    {
        sNextPageID = getFirstAllocPageID(aAllocPageList);
    }
    else
    {
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                aPageID,
                                                (void**)&sCurPageHeader )
                    == IDE_SUCCESS );
        sNextPageID = smpAllocPageList::getNextAllocPageID( aAllocPageList,
                                                            sCurPageHeader );
    }

    return sNextPageID;
}


/**********************************************************************
 * aAllocPageList에서 aPageID 다음 PageID
 *
 * 다중화되어있는 AllocPageList에서 aPageID가 Tail이라면
 * aPageID의 다음 Page는 다음 리스트의 Head가 된다.
 *
 * aAllocPageList : 탐색하려는 PageListEntry
 * aPagePtr       : 탐색하려는 Page의 시작 포인터
 **********************************************************************/
scPageID smpAllocPageList::getNextAllocPageID(
    smpAllocPageListEntry * aAllocPageList,
    smpPersPageHeader     * aPagePtr )
{
    scPageID           sNextPageID;
    UInt               sPageListID;

    IDE_DASSERT( aAllocPageList != NULL );

    if(aPagePtr == NULL)
    {
        sNextPageID = getFirstAllocPageID(aAllocPageList);
    }
    else
    {
        sNextPageID = aPagePtr->mNextPageID;

        if(sNextPageID == SM_NULL_PID)
        {
            for( sPageListID = aPagePtr->mAllocListID + 1;
                 sPageListID < SMP_PAGE_LIST_COUNT;
                 sPageListID++ )
            {
                sNextPageID =
                    aAllocPageList[sPageListID].mHeadPageID;

                if(sNextPageID != SM_NULL_PID)
                {
                    break;
                }
            }
        }
    }

    return sNextPageID;
}



/**********************************************************************
 * aAllocPageList에서 aPageID 이전 PageID
 *
 * 다중화되어있는 AllocPageList에서 aPageID가 Head라면
 * aPageID의 이전 Page는 이전 리스트의 Tail이 된다.
 *
 * aAllocPageList : 탐색하려는 PageListEntry
 * aPageID        : 탐색하려는 PageID
 **********************************************************************/

scPageID smpAllocPageList::getPrevAllocPageID(
    scSpaceID              aSpaceID,
    smpAllocPageListEntry* aAllocPageList,
    scPageID               aPageID )
{
    smpPersPageHeader* sCurPageHeader;
    scPageID           sPrevPageID;
    UInt               sPageListID;

    IDE_DASSERT( aAllocPageList != NULL );

    if(aPageID == SM_NULL_PID)
    {
        sPrevPageID = getLastAllocPageID(aAllocPageList);
    }
    else
    {
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                aPageID,
                                                (void**)&sCurPageHeader )
                    == IDE_SUCCESS );

        sPrevPageID = sCurPageHeader->mPrevPageID;
        sPageListID = sCurPageHeader->mAllocListID;

        if((sPrevPageID == SM_NULL_PID) && (sPageListID > 0))
        {
            do
            {
                sPageListID--;

                sPrevPageID =
                    aAllocPageList[sPageListID].mTailPageID;

                if(sPrevPageID != SM_NULL_PID)
                {
                    break;
                }
            } while(sPageListID != 0);
        }
    }

    return sPrevPageID;
}

/**********************************************************************
 * aAllocPageList의 첫 PageID를 반환
 *
 * AllocPageList가 다중화되어 있기 때문에
 * 0번째 리스트의 Head가 NULL이더라도 1번째 리스트의 Head가 NULL이 아니라면
 * 첫 PageID는 1번째 리스트의 Head가 된다.
 *
 * aAllocPageList : 탐색하려는 PageListEntry
 **********************************************************************/

scPageID smpAllocPageList::getFirstAllocPageID(
    smpAllocPageListEntry* aAllocPageList )
{
    UInt     sPageListID;
    scPageID sHeadPID = SM_NULL_PID;

    IDE_DASSERT( aAllocPageList != NULL );

    for( sPageListID = 0;
         sPageListID < SMP_PAGE_LIST_COUNT;
         sPageListID++ )
    {
        if(aAllocPageList[sPageListID].mHeadPageID != SM_NULL_PID)
        {
            sHeadPID = aAllocPageList[sPageListID].mHeadPageID;

            break;
        }
    }

    return sHeadPID;
}

/**********************************************************************
 * aAllocPageList의 마지막 PageID를 반환
 *
 * 다중화되어 있는 AllocPageList에서 NULL이 아닌 가장 뒤에 있는 Tail이
 * aAllocPageList의 마지막 PageID가 된다.
 *
 * aAllocPageList : 탐색하려는 PageListEntry
 **********************************************************************/

scPageID smpAllocPageList::getLastAllocPageID(
    smpAllocPageListEntry* aAllocPageList )
{
    UInt     sPageListID;
    scPageID sTailPID = SM_NULL_PID;

    IDE_DASSERT( aAllocPageList != NULL );

    sPageListID = SMP_PAGE_LIST_COUNT;

    do
    {
        sPageListID--;

        if(aAllocPageList[sPageListID].mTailPageID != SM_NULL_PID)
        {
            sTailPID = aAllocPageList[sPageListID].mTailPageID;

            break;
        }
    } while(sPageListID != 0);

    return sTailPID;
}
