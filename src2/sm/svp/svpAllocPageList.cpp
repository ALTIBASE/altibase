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
 
#include <svmDef.h>
#include <svpReq.h>
#include <svpAllocPageList.h>
#include <smuProperty.h>
#include <svmManager.h>

/**********************************************************************
 * AllocPageList를 초기화한다.
 *
 * aAllocPageList : 초기화하려는 AllocPageList
 **********************************************************************/

void svpAllocPageList::initializePageListEntry(
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

/* Runtime Item을 NULL로 설정한다.
   DISCARD/OFFLINE Tablespace에 속한 Table들에 대해 수행된다.
 */
IDE_RC svpAllocPageList::setRuntimeNull(
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
IDE_RC svpAllocPageList::initEntryAtRuntime(
    smpAllocPageListEntry* aAllocPageList,
    smOID                  aTableOID,
    smpPageType            aPageType )
{
    UInt                    i           = 0;
    UInt                    sState      = 0;
    UInt                    sAllocCount = 0;
    UInt                    sPageListID;
    SChar                   sBuffer[128];
    smpAllocPageListEntry * sAllocPageList;

    IDE_DASSERT( aAllocPageList != NULL );

    for( sPageListID = 0;
         sPageListID < SMP_PAGE_LIST_COUNT;
         sPageListID++ )
    {
        /* BUG-32434 [sm-mem-resource] If the admin uses a volatile table
         * with the parallel free page list, the server hangs while server start
         * up.
         * Parallel FreePageList를 사용할 수도 있기 때문에, 초기화시 항상
         * PageListID를 Array 변수로 사용해야 한다. */
        sAllocPageList = & aAllocPageList[ sPageListID ];
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

        /* svpAllocPageList_initEntryAtRuntime_malloc_Mutex.tc */
        IDU_FIT_POINT("svpAllocPageList::initEntryAtRuntime::malloc::Mutex");
        IDE_TEST(iduMemMgr::malloc(
                     IDU_MEM_SM_SVP,
                     sizeof(iduMutex),
                     (void **)&(sAllocPageList->mMutex),
                     IDU_MEM_FORCE )
                 != IDE_SUCCESS);
        sAllocCount = sPageListID;
        sState      = 1;

        IDE_TEST(sAllocPageList->mMutex->initialize(
                     sBuffer,
                     IDU_MUTEX_KIND_NATIVE,
                     IDV_WAIT_INDEX_NULL )
                 != IDE_SUCCESS);

        /* volatile table이 초기화되어야 하기 때문에
           sAllocPageList의 head, tail은 모두 null pid이어야 한다. */
        sAllocPageList->mHeadPageID = SM_NULL_PID;
        sAllocPageList->mTailPageID = SM_NULL_PID;

        /* mPageCount도 0으로 초기화해야 한다. */
        sAllocPageList->mPageCount = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            for( i = 0 ; i <= sAllocCount ; i++ )
            {
                sAllocPageList = & aAllocPageList[ i ];
                IDE_ASSERT( iduMemMgr::free( sAllocPageList->mMutex )
                            == IDE_SUCCESS );
                sAllocPageList->mMutex = NULL;
            }
        default:
            break;
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * AllocPageList의 Runtime 정보 해제
 *
 * aAllocPageList : 해제하려는 AllocPageList
 **********************************************************************/

IDE_RC svpAllocPageList::finEntryAtRuntime(
    smpAllocPageListEntry* aAllocPageList )
{
    IDE_DASSERT( aAllocPageList != NULL );
    IDE_DASSERT( aAllocPageList->mMutex != NULL );

    IDE_TEST(aAllocPageList->mMutex->destroy() != IDE_SUCCESS);

    IDE_TEST(iduMemMgr::free(aAllocPageList->mMutex) != IDE_SUCCESS);

    aAllocPageList->mMutex = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * aAllocPageHead~aAllocPageTail를 AllocPageList에 추가
 *
 * aTrans         : 작업하려는 트잰잭션 객체
 * aAllocPageList : 추가하려는 AllocPageList
 * aAllocPageHead : 추가하려는 Page의 Head
 * aAllocPageTail : 추가하려는 Page의 Tail
 **********************************************************************/

IDE_RC svpAllocPageList::addPageList( scSpaceID               aSpaceID,
                                      smpAllocPageListEntry * aAllocPageList,
                                      smpPersPage           * aAllocPageHead,
                                      smpPersPage           * aAllocPageTail,
                                      UInt                    aAllocPageCnt )
{
    smpPersPage* sTailPagePtr = NULL;

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

        // Tail뒤에 sAllocPageHead를 붙이고

        sTailPagePtr->mHeader.mNextPageID = aAllocPageHead->mHeader.mSelfPageID;

        // sAllocPageHead 앞에 Tail을 붙인다.

        aAllocPageHead->mHeader.mPrevPageID = aAllocPageList->mTailPageID;

        aAllocPageList->mTailPageID = aAllocPageTail->mHeader.mSelfPageID;
    }

    //  PageListEntry 정보 갱신
    aAllocPageList->mPageCount += aAllocPageCnt;

    IDE_DASSERT( isValidPageList( aSpaceID,
                                  aAllocPageList->mHeadPageID,
                                  aAllocPageList->mTailPageID,
                                  aAllocPageList->mPageCount )
                 == ID_TRUE );

    return IDE_SUCCESS;
}

/**********************************************************************
 * AllocPageList제거 및 DB로 반납
 *
 * FreePage를 제거하기 위해서는 다른 Tx가 접근해서는 안된다.
 * refineDB때와 같이 하나의 Tx만 접근하다.
 *
 * aTrans         : 작업하려는 트랜잭션 객체
 * aAllocPageList : 제거하려는 AllocPageList
 * aDropFlag      : DROP TABLE -> ID_TRUE
 *                  ALTER TABLE -> ID_FALSE
 **********************************************************************/

IDE_RC svpAllocPageList::freePageListToDB(
    void*                  aTrans,
    scSpaceID              aSpaceID,
    smpAllocPageListEntry* aAllocPageList)
{
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

        IDE_TEST( svmManager::freePersPageList( aTrans,
                                                aSpaceID,
                                                sHead,
                                                sTail )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * AllocPageList에서 aPageID 제거하여 DB로 반납
 *
 * aTrans         : 작업하려는 트랜잭션 객체
 * aAllocPageList : 제거하려는 AllocPageList
 * aPageID        : 제거하려는 Page ID
 **********************************************************************/

IDE_RC svpAllocPageList::removePage( scSpaceID               aSpaceID,
                                     smpAllocPageListEntry * aAllocPageList,
                                     scPageID                aPageID )
{
    idBool                 sLocked = ID_FALSE;
    scPageID               sPrevPageID;
    scPageID               sNextPageID;
    scPageID               sListHeadPID;
    scPageID               sListTailPID;
    smpPersPageHeader*     sPrevPageHeader;
    smpPersPageHeader*     sNextPageHeader;
    smpPersPageHeader*     sCurPageHeader;

    IDE_DASSERT( aAllocPageList != NULL );

    IDE_TEST( aAllocPageList->mMutex->lock( NULL ) != IDE_SUCCESS );
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
  
        sPrevPageHeader->mNextPageID = sNextPageID;
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

        sNextPageHeader->mPrevPageID = sPrevPageID;

    }

    aAllocPageList->mHeadPageID = sListHeadPID;
    aAllocPageList->mTailPageID = sListTailPID;
    aAllocPageList->mPageCount--;

    sLocked = ID_FALSE;
    IDE_TEST( aAllocPageList->mMutex->unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

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

idBool svpAllocPageList::isValidPageList( scSpaceID aSpaceID,
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

IDE_RC svpAllocPageList::nextOIDall( scSpaceID              aSpaceID,
                                     smpAllocPageListEntry* aAllocPageList,
                                     SChar*                 aCurRow,
                                     vULong                 aSlotSize,
                                     SChar**                aNxtRow )
{
    scPageID            sNxtPID;
    smpPersPage       * sPage;
    SChar             * sNxtPtr;
    SChar             * sFence;
#if defined(DEBUG_SVP_NEXTOIDALL_PAGE_CHECK)
    idBool              sIsFreePage ;
    smpPersPageHeader * sPageHdr;
#endif // DEBUG_SVP_NEXTOIDALL_PAGE_CHECK

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

#if defined(DEBUG_SVP_NEXTOIDALL_PAGE_CHECK)
        // nextOIDall 스캔 대상이 되는 페이지는
        // 테이블에 할당된 Page이므로 Free Page일 수 없다.
        sPageHdr = (smpPersPageHeader *)sPage;
        IDE_TEST( svmExpandChunk::isFreePageID( sPageHdr->mSelfPageID,
                                                & sIsFreePage )
                  != IDE_SUCCESS );
        IDE_DASSERT( sIsFreePage == ID_FALSE );
#endif // DEBUG_SVP_NEXTOIDALL_PAGE_CHECK

        // 해당 Page에 대한 포인터 경계
        sFence = (SChar *)((smpPersPage *)sPage + 1);

        for( ;
             sNxtPtr+aSlotSize <= sFence;
             sNxtPtr += aSlotSize )
        {
            // In case of free slot
            if( SMP_SLOT_IS_NOT_USED( (smpSlotHeader *)sNxtPtr ) )
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

#if defined(DEBUG_SVP_NEXTOIDALL_PAGE_CHECK)

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#endif // DEBUG_SVP_NEXTOIDALL_PAGE_CHECK
}

/***********************************************************************
 * nextOIDall을 위해 aRow에서 해당 Page를 찾아준다.
 *
 * aAllocPageList : 순회하려는 PageListEntry
 * aRow           : 현재 Row
 * aPage          : aRow가 속한 Page를 찾아서 반환
 * aRowPtr        : aRow 다음 Row 포인터
 ***********************************************************************/

inline void svpAllocPageList::initForScan(
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

scPageID svpAllocPageList::getNextAllocPageID(
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
        sNextPageID = svpAllocPageList::getNextAllocPageID( aAllocPageList,
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
scPageID svpAllocPageList::getNextAllocPageID(
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

scPageID svpAllocPageList::getPrevAllocPageID(
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

scPageID svpAllocPageList::getFirstAllocPageID(
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

scPageID svpAllocPageList::getLastAllocPageID(
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
