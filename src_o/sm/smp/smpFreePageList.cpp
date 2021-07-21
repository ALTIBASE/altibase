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
 * $Id: smpFreePageList.cpp 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#include <smpDef.h>
#include <smmDef.h>
#include <smpReq.h>
#include <smpAllocPageList.h>
#include <smpFreePageList.h>
#include <smuProperty.h>
#include <smmManager.h>

/**********************************************************************
 * Runtime Item을 NULL로 설정한다.
 * DISCARD/OFFLINE Tablespace에 속한 Table들에 대해 수행된다.
 *
 * aPageListEntry : 구축하려는 PageListEntry
 **********************************************************************/
IDE_RC smpFreePageList::setRuntimeNull( smpPageListEntry* aPageListEntry )
{
    aPageListEntry->mRuntimeEntry = NULL;
    
    return IDE_SUCCESS;
}


/**********************************************************************
 * PageListEntry에서 FreePage와 관련된 RuntimeEntry에 대한 초기화 및
 * FreePageList의 Mutex 초기화
 *
 * aPageListEntry : 구축하려는 PageListEntry
 **********************************************************************/

IDE_RC smpFreePageList::initEntryAtRuntime( smpPageListEntry* aPageListEntry )
{
    smOID                 sTableOID;
    UInt                  sPageListID;
    SChar                 sBuffer[128];
    smpFreePagePoolEntry* sFreePagePool;
    smpFreePageListEntry* sFreePageList;

    IDE_DASSERT( aPageListEntry != NULL );
    
    sTableOID = aPageListEntry->mTableOID;
   
    /* smpFreePageList_initEntryAtRuntime_malloc_RuntimeEntry.tc */
    IDU_FIT_POINT("smpFreePageList::initEntryAtRuntime::malloc::RuntimeEntry");
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_SM_SMP,
                                ID_SIZEOF(smpRuntimeEntry),
                                (void **)&(aPageListEntry->mRuntimeEntry),
                                IDU_MEM_FORCE )
             != IDE_SUCCESS);

    /*
     * BUG-25327 : [MDB] Free Page Size Class 개수를 Property화 해야 합니다.
     */
    aPageListEntry->mRuntimeEntry->mSizeClassCount = smuProperty::getMemSizeClassCount();
    
    // Record Count 관련 초기화
    aPageListEntry->mRuntimeEntry->mInsRecCnt = 0;
    aPageListEntry->mRuntimeEntry->mDelRecCnt = 0;

    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부하 및 Aging이 밀리는 현상이 더 심화됨.
    aPageListEntry->mRuntimeEntry->mOldVersionCount = 0;
    aPageListEntry->mRuntimeEntry->mUniqueViolationCount = 0;
    aPageListEntry->mRuntimeEntry->mUpdateRetryCount = 0;
    aPageListEntry->mRuntimeEntry->mDeleteRetryCount = 0;


    idlOS::snprintf( sBuffer, 128,
                     "TABLE_RECORD_COUNT_MUTEX_%"ID_XINT64_FMT"",
                     (ULong)sTableOID );
    
    IDE_TEST(aPageListEntry->mRuntimeEntry->mMutex.initialize(
                 sBuffer,
                 IDU_MUTEX_KIND_NATIVE,
                 IDV_WAIT_INDEX_NULL)
             != IDE_SUCCESS);

    // FreePageList&Pool 초기화
    initializeFreePageListAndPool( aPageListEntry );
    
    // FreePagePool의 Mutex 초기화
    sFreePagePool = &(aPageListEntry->mRuntimeEntry->mFreePagePool);
    
    idlOS::snprintf( sBuffer, 128,
                     "FREE_PAGE_POOL_MUTEX_%"ID_XINT64_FMT"",
                     (ULong)sTableOID );
    
    IDE_TEST(sFreePagePool->mMutex.initialize( sBuffer,
                                               IDU_MUTEX_KIND_NATIVE,
                                               IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);

    // FreePageList의 Mutex 초기화
    for( sPageListID = 0;
         sPageListID < SMP_PAGE_LIST_COUNT;
         sPageListID++ )
    {
        sFreePageList =
            &(aPageListEntry->mRuntimeEntry->mFreePageList[sPageListID]);
        
        idlOS::snprintf( sBuffer, 128,
                         "FREE_PAGE_LIST_MUTEX_%"ID_XINT64_FMT"_%"ID_UINT32_FMT"",
                         (ULong)sTableOID, sPageListID );
        
        IDE_TEST(sFreePageList->mMutex.initialize( sBuffer,
                                                   IDU_MUTEX_KIND_NATIVE,
                                                   IDV_WAIT_INDEX_NULL )
                 != IDE_SUCCESS);
        
    }

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * PageListEntry에서 FreePage와 관련된 RuntimeEntry에 대한 해제 및
 * FreePageList의 Mutex 제거
 *
 * aPageListEntry : 해제하려는 PageListEntry
 **********************************************************************/

IDE_RC smpFreePageList::finEntryAtRuntime( smpPageListEntry* aPageListEntry )
{
    UInt sPageListID;

    IDE_DASSERT( aPageListEntry != NULL );

    if ( aPageListEntry->mRuntimeEntry != NULL )
    {
        // for FreePagePool
        IDE_TEST( aPageListEntry->mRuntimeEntry->mFreePagePool.mMutex.destroy()
                  != IDE_SUCCESS );

        // fix BUG-13209
        for( sPageListID = 0;
             sPageListID < SMP_PAGE_LIST_COUNT;
             sPageListID++ )
        {
        
            // for FreePageList
            IDE_TEST(
                aPageListEntry->mRuntimeEntry->mFreePageList[sPageListID].mMutex.destroy()
                != IDE_SUCCESS);
        }

        // RuntimeEntry 제거
        // To Fix BUG-14185
        IDE_TEST(aPageListEntry->mRuntimeEntry->mMutex.destroy() != IDE_SUCCESS );
    
        IDE_TEST(iduMemMgr::free(aPageListEntry->mRuntimeEntry) != IDE_SUCCESS);

        aPageListEntry->mRuntimeEntry = NULL;
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
 * PageListEntry 해제를 위한 FreePage와 관련된 RuntimeEntry 정보 해제
 *
 * aPageListEntry : 해제하려는 PageListEntry
 **********************************************************************/

void smpFreePageList::initializeFreePageListAndPool(
    smpPageListEntry* aPageListEntry )
{
    UInt                  sPageListID;
    UInt                  sSizeClassID;
    smpFreePagePoolEntry* sFreePagePool;
    smpFreePageListEntry* sFreePageList;
    UInt                  sSizeClassCount;

    IDE_DASSERT( aPageListEntry != NULL );
 
    sSizeClassCount = SMP_SIZE_CLASS_COUNT( aPageListEntry->mRuntimeEntry );
   
    // for FreePagePool
    sFreePagePool = &(aPageListEntry->mRuntimeEntry->mFreePagePool);
    
    sFreePagePool->mPageCount = 0;
    sFreePagePool->mHead      = NULL;
    sFreePagePool->mTail      = NULL;

    for( sPageListID = 0;
         sPageListID < SMP_PAGE_LIST_COUNT;
         sPageListID++ )
    {
        // for FreePageList
        sFreePageList =
            &(aPageListEntry->mRuntimeEntry->mFreePageList[sPageListID]);
        
        for( sSizeClassID = 0;
             sSizeClassID < sSizeClassCount;
             sSizeClassID++ )
        {
            sFreePageList->mPageCount[sSizeClassID] = 0;
            sFreePageList->mHead[sSizeClassID]      = NULL;
            sFreePageList->mTail[sSizeClassID]      = NULL;
        }
    }

    
    return;
}

/**********************************************************************
 * FreeSlotList를 초기화한다.
 * ( Page내의 모든 FreeSlot은 연결되어 있으며,
 *   FreeSlotList를 FreePageHeader에 붙이는 작업을 한다. )
 *
 * aPageListEntry : 초기화하려는 PageListEntry
 * aPagePtr       : 초기화하려는 Page
 **********************************************************************/

void smpFreePageList::initializeFreeSlotListAtPage(
    scSpaceID         aSpaceID,
    smpPageListEntry* aPageListEntry,
    smpPersPage*      aPagePtr )
{
    UShort             sPageBodyOffset;
    smpFreePageHeader* sFreePageHeader;

    IDE_DASSERT( aPageListEntry != NULL );
    IDE_DASSERT( aPagePtr != NULL );
    
    // Page내의 첫 Slot위치를 위한 Offset을 구한다.
    sPageBodyOffset = (UShort)SMP_PERS_PAGE_BODY_OFFSET;
    
    if( SMP_GET_PERS_PAGE_TYPE( aPagePtr ) == SMP_PAGETYPE_VAR)
    {
        // VarPage인 경우 추가적인 VarPageHeader만큼 Offset을 더한다.
        sPageBodyOffset += ID_SIZEOF(smpVarPageHeader);
    }
    
    sFreePageHeader = getFreePageHeader(aSpaceID, aPagePtr);

    // 첫 Slot을 Head에 등록한다.
    
    sFreePageHeader->mFreeSlotHead  = (SChar*)aPagePtr + sPageBodyOffset;
    sFreePageHeader->mFreeSlotTail  = (SChar*)aPagePtr + sPageBodyOffset
        + aPageListEntry->mSlotSize * (aPageListEntry->mSlotCount - 1);
    sFreePageHeader->mFreeSlotCount = aPageListEntry->mSlotCount;


    return;
}

/**********************************************************************
 * FreePageList 구축
 *
 * refineDB에서 모든 Page를 Scan하면서 각 Page의 FreeSlotList를 구축했고,
 * 각 FreePage를 FreePageList[0]에 등록하였다.
 *
 * 여기서는 FreePageList[0]에서 각 FreePageList별로 FreePage를 나누어 준다.
 *
 * buildFreePageList은 refineDB시 admin 트랜잭션에 의해서만 수행되고,
 * Ager와 같은 다른 트랜잭션은 전혀 간섭을 하지 않으므로 Mutex를 안잡고 수행
 *
 * aTrans         : 작업을 수행하는 트랜잭션 객체
 * aPageListEntry : 구축하려는 PageListEntry
 **********************************************************************/

void smpFreePageList::distributePagesFromFreePageList0ToTheOthers(
    smpPageListEntry* aPageListEntry )
{
    UInt                  sPageListID;
    UInt                  sSizeClassID;
    UInt                  sAssignCount;
    UInt                  sNeedAssignCount;
    UInt                  sPageCounter;
    smpFreePageHeader*    sFreePageHeader;
    smpFreePageHeader*    sNextFreePageHeader;
    smpFreePageListEntry* sFreePageList0;
    smpFreePageListEntry* sFreePageList;
    UInt                  sSizeClassCount;
    UInt                  sEmptySizeClassID;

    IDE_DASSERT( aPageListEntry != NULL );
    
    sFreePageList0 = &(aPageListEntry->mRuntimeEntry->mFreePageList[0]);
    IDE_DASSERT( sFreePageList0 != NULL );

    sNeedAssignCount = SMP_FREEPAGELIST_MINPAGECOUNT * SMP_PAGE_LIST_COUNT;
    sSizeClassCount = SMP_SIZE_CLASS_COUNT( aPageListEntry->mRuntimeEntry );
    sEmptySizeClassID = SMP_EMPTYPAGE_CLASSID( aPageListEntry->mRuntimeEntry );
    
    IDE_DASSERT( sFreePageList0 != NULL );
    
    for( sSizeClassID = 0;
         sSizeClassID < sSizeClassCount;
         sSizeClassID++ )
    {
        IDE_DASSERT( isValidFreePageList(
                         sFreePageList0->mHead[sSizeClassID],
                         sFreePageList0->mTail[sSizeClassID],
                         sFreePageList0->mPageCount[sSizeClassID])
                     == ID_TRUE );
        
        if( (sSizeClassID == sEmptySizeClassID) &&
            (sFreePageList0->mPageCount[sSizeClassID] > sNeedAssignCount) )
        {
            // 마지막 SIZE_CLASS (EmptyPage)인 경우 무조건 1/N하는게 아니라
            // FREE_THRESHOLD 만큼씩만 나누어 주고 남은 것은 FreePagePool에
            // 반환한다.
            sAssignCount = SMP_FREEPAGELIST_MINPAGECOUNT;
        }
        else
        {
            sAssignCount = sFreePageList0->mPageCount[sSizeClassID]
                           / SMP_PAGE_LIST_COUNT;
        }
        
        if(sAssignCount > 0)
        {
            // sFreePageList[0] ~> [1]...[N] 나누어 준다.
            for( sPageListID = 1;
                 sPageListID < SMP_PAGE_LIST_COUNT;
                 sPageListID++ )
            {
                sFreePageList =
                    &(aPageListEntry->mRuntimeEntry->mFreePageList[sPageListID]);
                IDE_DASSERT( sFreePageList != NULL );

                // Before :
                // sFreePageList[0] : []-[]-[]-[]-[]-[]
                // After :
                // sFreePageList[0] :             []-[]
                // sFreePageList[1] : []-[]
                // sFreePageList[2] :       []-[]

                // List0의 Head를 나누어줄 List의 Head로 넘겨주고
                sFreePageHeader = sFreePageList0->mHead[sSizeClassID];
                
                sFreePageList->mHead[sSizeClassID] = sFreePageHeader;

                // AssignCount만큼 Skip하고
                for( sPageCounter = 0;
                     sPageCounter < sAssignCount - 1;
                     sPageCounter++ )
                {
                    IDE_DASSERT( sFreePageHeader != NULL );
                    
                    sFreePageHeader->mFreeListID  = sPageListID;
                    sFreePageHeader->mSizeClassID = sSizeClassID;

                    sFreePageHeader = sFreePageHeader->mFreeNext;
                }

                // 마지막 FreePage까지 NULL이 아니어야 한다.
                IDE_DASSERT( sFreePageHeader != NULL );
                
                // 마지막 FreePage를 Tail로 등록하고
                sFreePageList->mTail[sSizeClassID] = sFreePageHeader;
                sFreePageHeader->mFreeListID       = sPageListID;
                sFreePageHeader->mSizeClassID      = sSizeClassID;

                sNextFreePageHeader = sFreePageHeader->mFreeNext;

                // 마지막 FreePage의 링크를 끊어준다.
                if(sNextFreePageHeader == NULL)
                {
                    sFreePageList0->mTail[sSizeClassID] = NULL;
                }
                else
                {
                    sFreePageHeader->mFreeNext     = NULL;
                    sNextFreePageHeader->mFreePrev = NULL;
                }

                // 마지막 FreePage 다음 FreePage부터 List0로 다시 등록한다.
                sFreePageList0->mHead[sSizeClassID] = sNextFreePageHeader;

                sFreePageList0->mPageCount[sSizeClassID] -= sAssignCount;
                sFreePageList->mPageCount[sSizeClassID]  += sAssignCount;

                IDE_DASSERT( isValidFreePageList(
                                 sFreePageList->mHead[sSizeClassID],
                                 sFreePageList->mTail[sSizeClassID],
                                 sFreePageList->mPageCount[sSizeClassID])
                             == ID_TRUE );
            }
        }

        IDE_DASSERT( isValidFreePageList(
                         sFreePageList0->mHead[sSizeClassID],
                         sFreePageList0->mTail[sSizeClassID],
                         sFreePageList0->mPageCount[sSizeClassID])
                     == ID_TRUE );
    }


    return;
}

/**********************************************************************
 * FreePagePool 구축
 *
 * EmptyPage의 경우 각 FreePageList에 FREE_THRESHOLD만큼씩만 나눠주고,
 * 남았을 경우 FreePageList[0]에서 그 이상의 것을 FreePagePool로 반환한다.
 *
 * buildFreePagePool은 refineDB시 admin 트랜잭션에 의해서만 수행되고,
 * Ager와 같은 다른 트랜잭션은 전혀 간섭을 하지 않으므로 Mutex를 안잡고 수행
 *
 * aTrans         : 작업을 수행하는 트랜잭션 객체
 * aPageListEntry : 구축하려는 PageListEntry
 **********************************************************************/

IDE_RC smpFreePageList::distributePagesFromFreePageList0ToFreePagePool(
                                        void*             aTrans,
                                        scSpaceID         aSpaceID,
                                        smpPageListEntry* aPageListEntry )
{
    UInt                  sAssignCount;
    UInt                  sPageCounter = 0;
    smpFreePageHeader*    sOldFreePagePoolTail;
    smpFreePageHeader*    sFreePageHeader;
    smpFreePageHeader*    sNextFreePageHeader;
    smpFreePagePoolEntry* sFreePagePool;
    smpFreePageListEntry* sFreePageList0;
    UInt                  sEmptySizeClassID;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPageListEntry != NULL );
    
    sEmptySizeClassID = SMP_EMPTYPAGE_CLASSID( aPageListEntry->mRuntimeEntry );
    
    sFreePageList0 = &(aPageListEntry->mRuntimeEntry->mFreePageList[0]);

    IDE_DASSERT( sFreePageList0 != NULL );
    
    IDE_DASSERT( isValidFreePageList(
                     sFreePageList0->mHead[sEmptySizeClassID],
                     sFreePageList0->mTail[sEmptySizeClassID],
                     sFreePageList0->mPageCount[sEmptySizeClassID])
                 == ID_TRUE );

    if( sFreePageList0->mPageCount[sEmptySizeClassID]
        > SMP_FREEPAGELIST_MINPAGECOUNT )
    {
        // FreePageList[0]에 FreePageCount가 FREE_THRESHOLD보다 클 경우
        // FREE_THRESHOLD이상의 것을 FreePagePool로 반납한다.
        sAssignCount  = sFreePageList0->mPageCount[sEmptySizeClassID]
                      - SMP_FREEPAGELIST_MINPAGECOUNT;
        
        sFreePagePool = &(aPageListEntry->mRuntimeEntry->mFreePagePool);

        IDE_DASSERT( sFreePagePool->mHead == NULL );

        // SIZE_CLASS_COUNT가 1일 경우 Non-EmptyPage와 EmptyPage가 섞여있으므로,
        // FreePagePool에 들어갈 수 있는 EmptyPage만을 걸러야 한다.

        // Before :
        // sFreePageList[0] : []-[]-[]-[]
        // After :
        // sFreePageList[0] :       []-[]
        // sFreePagePool    : []-[]

        // List0의 Head를 Pool로 옮기고
        sFreePageHeader = sFreePageList0->mHead[sEmptySizeClassID];

        // AssignCount 만큼 Skip하고
        while( (sPageCounter < sAssignCount) && (sFreePageHeader != NULL) )
        {
            sNextFreePageHeader = sFreePageHeader->mFreeNext;

            /*
             * BUG-25327 : [MDB] Free Page Size Class 개수를 Property화 해야 합니다.
             * Free Page Size Class가 1인 경우는 Non-EmptyPage와 Empty페이지가 공존할수 있다.
             * Empty 페이지만을 Free Page Pool로 옮긴다.
             */
            if( aPageListEntry->mSlotCount == sFreePageHeader->mFreeSlotCount )
            {
                IDE_DASSERT( smpManager::validateScanList( aSpaceID,
                                                           sFreePageHeader->mSelfPageID,
                                                           SM_NULL_PID,
                                                           SM_NULL_PID )
                             == ID_TRUE );
                
                sFreePageHeader->mFreeListID  = SMP_POOL_PAGELISTID;
                sFreePageHeader->mSizeClassID = sEmptySizeClassID;

                sOldFreePagePoolTail = sFreePagePool->mTail;
                
                if( sFreePagePool->mTail == NULL )
                {
                    IDE_DASSERT( sFreePagePool->mHead == NULL );
                    sFreePagePool->mHead = sFreePageHeader;
                    sFreePagePool->mTail = sFreePageHeader;
                }
                else
                {
                    sFreePagePool->mTail->mFreeNext = sFreePageHeader;
                    sFreePagePool->mTail = sFreePageHeader;
                }
                
                if( sFreePageHeader->mFreePrev != NULL )
                {
                    sFreePageHeader->mFreePrev->mFreeNext = sFreePageHeader->mFreeNext;
                }

                if( sFreePageHeader->mFreeNext != NULL )
                {
                    sFreePageHeader->mFreeNext->mFreePrev = sFreePageHeader->mFreePrev;
                }

                if( sFreePageList0->mHead[sEmptySizeClassID] == sFreePageHeader )
                {
                    sFreePageList0->mHead[sEmptySizeClassID] = sFreePageHeader->mFreeNext;
                }

                if( sFreePageList0->mTail[sEmptySizeClassID] == sFreePageHeader )
                {
                    sFreePageList0->mTail[sEmptySizeClassID] = sFreePageHeader->mFreePrev;
                }

                sFreePageHeader->mFreeNext = NULL;
                sFreePageHeader->mFreePrev = sOldFreePagePoolTail;

                sPageCounter++;
            }
            
            sFreePageHeader = sNextFreePageHeader;
        }

        IDE_ERROR_MSG( sFreePageList0->mPageCount[sEmptySizeClassID]
                       > sAssignCount,
                       "mPageCount   : %"ID_UINT32_FMT"\n"
                       "sAssignCount : %"ID_UINT32_FMT"\n",
                       sFreePageList0->mPageCount[sEmptySizeClassID],
                       sAssignCount );

        sFreePageList0->mPageCount[sEmptySizeClassID] -= sPageCounter;
        sFreePagePool->mPageCount                     += sPageCounter;

        // Pool에 등록된 FreePage(EmptyPage)가 너무 많으면 DB로 반납한다.
        if( sFreePagePool->mPageCount > SMP_FREEPAGELIST_MINPAGECOUNT )
        {
            IDE_TEST( freePagesFromFreePagePoolToDB( aTrans,
                                                     aSpaceID,
                                                     aPageListEntry,
                                                     0  /* 0 : all*/)
                      != IDE_SUCCESS);
        }

        IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                          sFreePagePool->mTail,
                                          sFreePagePool->mPageCount )
                     == ID_TRUE );

        IDE_DASSERT( isValidFreePageList(
                         sFreePageList0->mHead[sEmptySizeClassID],
                         sFreePageList0->mTail[sEmptySizeClassID],
                         sFreePageList0->mPageCount[sEmptySizeClassID])
                     == ID_TRUE );
    }

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * FreePagePool로부터 FreePageList에 FREE_THRESHOLD만큼씩
 * FreePages를 할당할 수 있을지 검사하고 가능하면 할당한다.
 *
 * aPageListEntry : 작업할 PageListEntry
 * aPageListID    : 할당받을 FreePageList의 ListID
 * aRc            : 할당받았는지 결과를 반환한다.
 **********************************************************************/

IDE_RC smpFreePageList::tryForAllocPagesFromPool(
                                        smpPageListEntry* aPageListEntry,
                                        UInt              aPageListID,
                                        idBool*           aIsPageAlloced )
{
    UInt                  sState = 0;
    smpFreePagePoolEntry* sFreePagePool;

    IDE_DASSERT( aPageListEntry != NULL );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_DASSERT( aIsPageAlloced != NULL );
    
    sFreePagePool = &(aPageListEntry->mRuntimeEntry->mFreePagePool);

    IDE_DASSERT( sFreePagePool != NULL );

    *aIsPageAlloced = ID_FALSE;

    if(sFreePagePool->mPageCount >= SMP_MOVEPAGECOUNT_POOL2LIST)
    {
        // Pool의 갯수를 먼저 가져올 만큼 있는지 보고,
        // Pool에 lock을 잡고 작업한다. List의 lock은 하위 함수에서 잡는다.
        IDE_TEST(sFreePagePool->mMutex.lock( NULL /* idvSQL* */ )
                 != IDE_SUCCESS);
        sState = 1;
        
        // lock을 잡기전에 다른 Tx에 의해 변경되었는지 다시 확인
        if(sFreePagePool->mPageCount >= SMP_MOVEPAGECOUNT_POOL2LIST)
        {
            IDE_TEST(getPagesFromFreePagePool( aPageListEntry,
                                               aPageListID )
                     != IDE_SUCCESS);

            *aIsPageAlloced = ID_TRUE;
        }
        
        sState = 0;
        IDE_TEST(sFreePagePool->mMutex.unlock() != IDE_SUCCESS);
    }

    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT( sFreePagePool->mMutex.unlock() == IDE_SUCCESS );
    }
    
    IDE_POP();

    
    return IDE_FAILURE;
}

/**********************************************************************
 * FreePagePool로부터 FreePageList에 FREE_THRESHOLD만큼씩 FreePages 할당
 *
 * Pool에 대한 Lock을 잡고 들어와야 한다.
 *
 * aPageListEntry : 작업할 PageListEntry
 * aPageListID    : 할당받을 FreePageList ID
 **********************************************************************/

IDE_RC smpFreePageList::getPagesFromFreePagePool(
    smpPageListEntry* aPageListEntry,
    UInt              aPageListID )
{
    UInt                  sState = 0;
    UInt                  sPageCounter;
    UInt                  sAssignCount;
    smpFreePageHeader*    sCurFreePageHeader;
    smpFreePageHeader*    sTailFreePageHeader;
    smpFreePagePoolEntry* sFreePagePool;
    smpFreePageListEntry* sFreePageList;
    UInt                  sEmptySizeClassID;

    IDE_DASSERT( aPageListEntry != NULL );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    
    sEmptySizeClassID = SMP_EMPTYPAGE_CLASSID( aPageListEntry->mRuntimeEntry );
    sAssignCount  = SMP_MOVEPAGECOUNT_POOL2LIST;

    sFreePagePool = &(aPageListEntry->mRuntimeEntry->mFreePagePool);
    sFreePageList = &(aPageListEntry->mRuntimeEntry->mFreePageList[aPageListID]);

    IDE_DASSERT( sFreePagePool != NULL );
    IDE_DASSERT( sFreePageList != NULL );
    
    IDE_TEST(sFreePageList->mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 1;

    IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                      sFreePagePool->mTail,
                                      sFreePagePool->mPageCount )
                 == ID_TRUE );

    IDE_DASSERT( isValidFreePageList(
                     sFreePageList->mHead[sEmptySizeClassID],
                     sFreePageList->mTail[sEmptySizeClassID],
                     sFreePageList->mPageCount[sEmptySizeClassID])
                 == ID_TRUE );

    // Before :
    // sFreePagePool : []-[]-[]-[]
    // After :
    // sFreePageList : []-[]
    // sFreePagePool :       []-[]
        
     // FreePagePool의 Head를 FreePageList로 옮기고
    sCurFreePageHeader  = sFreePagePool->mHead;
    sTailFreePageHeader = sFreePageList->mTail[sEmptySizeClassID];
    
    if(sTailFreePageHeader == NULL)
    {
        // List의 Tail이 NULL이라면 List가 비어있다.
        IDE_DASSERT( sFreePageList->mHead[sEmptySizeClassID] == NULL );
        
        sFreePageList->mHead[sEmptySizeClassID] = sCurFreePageHeader;
    }
    else
    {
        IDE_DASSERT( sFreePageList->mHead[sEmptySizeClassID] != NULL );

        sTailFreePageHeader->mFreeNext     = sCurFreePageHeader;
        sCurFreePageHeader->mFreePrev      = sTailFreePageHeader;
    }

    // AssignCount만큼 Skip하고
    for( sPageCounter = 0;
         sPageCounter < sAssignCount;
         sPageCounter++ )
    {
        IDE_DASSERT( sCurFreePageHeader != NULL );
        
        sCurFreePageHeader->mFreeListID  = aPageListID;
        sCurFreePageHeader->mSizeClassID = sEmptySizeClassID;

        sCurFreePageHeader = sCurFreePageHeader->mFreeNext;
    }

    if(sCurFreePageHeader == NULL)
    {
        // 마지막 FreePage 다음이 NULL이라면 마지막이 Tail이다.
        sTailFreePageHeader = sFreePagePool->mTail;

        sFreePagePool->mTail = NULL;
    }
    else
    {
        sTailFreePageHeader = sCurFreePageHeader->mFreePrev;

        // 양쪽 리스트를 끊어준다.
        sTailFreePageHeader->mFreeNext = NULL;
        sCurFreePageHeader->mFreePrev  = NULL;
    }
    
    // AssignCount번째 Page를 FreePageList의 Tail로 등록
    sFreePageList->mTail[sEmptySizeClassID] = sTailFreePageHeader;
    sFreePagePool->mHead                    = sCurFreePageHeader;

    sFreePageList->mPageCount[sEmptySizeClassID] += sAssignCount;
    sFreePagePool->mPageCount                    -= sAssignCount;

    IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                      sFreePagePool->mTail,
                                      sFreePagePool->mPageCount )
                 == ID_TRUE );

    IDE_DASSERT( isValidFreePageList(
                     sFreePageList->mHead[sEmptySizeClassID],
                     sFreePageList->mTail[sEmptySizeClassID],
                     sFreePageList->mPageCount[sEmptySizeClassID])
                 == ID_TRUE );

    sState = 0;
    IDE_TEST(sFreePageList->mMutex.unlock() != IDE_SUCCESS);
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT(sFreePageList->mMutex.unlock() == IDE_SUCCESS);
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/**********************************************************************
 * FreePagePool에 FreePage 추가
 *
 * aPageListEntry  : 구축하려는 PageListEntry
 * aFreePageHeader : 추가하려는 FreePageHeader
 **********************************************************************/

IDE_RC smpFreePageList::addPageToFreePagePool(
                                        smpPageListEntry*  aPageListEntry,
                                        smpFreePageHeader* aFreePageHeader )
{
    UInt                  sState = 0;
    smpFreePagePoolEntry* sFreePagePool;
    smpFreePageHeader*    sTailFreePageHeader;

    IDE_DASSERT(aPageListEntry != NULL);
    IDE_DASSERT(aFreePageHeader != NULL);
    
    sFreePagePool = &(aPageListEntry->mRuntimeEntry->mFreePagePool);

    IDE_DASSERT(sFreePagePool != NULL);

    // Pool에 들어오는 Page는 EmptyPage여야 한다.
    IDE_DASSERT(aFreePageHeader->mFreeSlotCount == aPageListEntry->mSlotCount);

    // FreePageHeader 정보 초기화
    aFreePageHeader->mFreeNext    = (smpFreePageHeader *)SM_NULL_PID;
    aFreePageHeader->mFreeListID  = SMP_POOL_PAGELISTID;
    aFreePageHeader->mSizeClassID = SMP_EMPTYPAGE_CLASSID( aPageListEntry->mRuntimeEntry );
    
    IDE_TEST(sFreePagePool->mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 1;

    IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                      sFreePagePool->mTail,
                                      sFreePagePool->mPageCount )
                 == ID_TRUE );

    sTailFreePageHeader = sFreePagePool->mTail;

    // FreePagePool의 Tail에 붙인다.
    if(sTailFreePageHeader == NULL)
    {
        IDE_DASSERT(sFreePagePool->mHead == NULL);
        
        sFreePagePool->mHead = aFreePageHeader;
    }
    else
    {
        IDE_DASSERT(sFreePagePool->mPageCount > 0);
        
        sTailFreePageHeader->mFreeNext = aFreePageHeader;
    }

    aFreePageHeader->mFreePrev = sTailFreePageHeader;
    sFreePagePool->mTail       = aFreePageHeader;
    sFreePagePool->mPageCount++;

    IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                      sFreePagePool->mTail,
                                      sFreePagePool->mPageCount )
                 == ID_TRUE );

    sState = 0;
    IDE_TEST(sFreePagePool->mMutex.unlock() != IDE_SUCCESS);
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if(sState == 1)
    {
        IDE_ASSERT(sFreePagePool->mMutex.unlock() == IDE_SUCCESS);
    }

    IDE_POP();
    

    return IDE_FAILURE;
}

/**********************************************************************
 * FreePage들을 PageListEntry에서 제거
 *
 * FreePage를 제거하기 위해서는 다른 Tx가 접근해서는 안된다.
 * refineDB때와 같이 하나의 Tx만 접근해야 된다.
 *
 * aTrans         : 작업을 수행하는 트랜잭션 객체
 * aPageListEntry : 제거하려는 PageListEntry
 * aPages         : # of Pages for compaction
 **********************************************************************/

IDE_RC smpFreePageList::freePagesFromFreePagePoolToDB(
                                            void             * aTrans,
                                            scSpaceID          aSpaceID,
                                            smpPageListEntry * aPageListEntry,
                                            UInt               aPages )
{
    UInt                    sState = 0;
    UInt                    sPageListID;
    UInt                    sPageCounter;
    UInt                    sRemovePageCount;
    smLSN                   sLsnNTA;
    smOID                   sTableOID;
    scPageID                sCurPageID = SM_NULL_PID;
    scPageID                sRemovePrevID;
    scPageID                sFreeNextID;
    smpPersPage           * sRemoveHead = NULL;
    smpPersPage           * sRemoveTail = NULL;
    smpPersPageHeader     * sCurPageHeader      = NULL;
    smpFreePageHeader     * sCurFreePageHeader  = NULL;
    smpFreePageHeader     * sHeadFreePageHeader = NULL; // To fix BUG-28189
    smpFreePageHeader     * sTailFreePageHeader = NULL; // To fix BUG-28189
    smpFreePagePoolEntry  * sFreePagePool  = NULL;
    smpAllocPageListEntry * sAllocPageList = NULL;
    UInt                    sLimitMinFreePageCount; // To fix BUG-28189
    idBool                  sIsLocked = ID_FALSE;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPageListEntry != NULL );
    
    sTableOID              = aPageListEntry->mTableOID;
    sFreePagePool          = &(aPageListEntry->mRuntimeEntry->mFreePagePool);
    sLimitMinFreePageCount = SMP_FREEPAGELIST_MINPAGECOUNT;

    IDE_ASSERT( sFreePagePool != NULL );

    // To fix BUG-28189
    // Ager 가 접근하지 못하도록 lock 을 잡는다.
    IDE_TEST( sFreePagePool->mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    // To fix BUG-28189
    // 최소한의 page count 와 같거나 작으면 더 이상 compact 를 할 필요가 없다.
    if( sFreePagePool->mPageCount <= sLimitMinFreePageCount )
    {
        sIsLocked = ID_FALSE;
        IDE_TEST( sFreePagePool->mMutex.unlock() != IDE_SUCCESS );
        IDE_CONT( no_more_need_compact );
    }

    IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                      sFreePagePool->mTail,
                                      sFreePagePool->mPageCount)
                 == ID_TRUE );

    // FreePagePool에서 FREE_THRESHOLD만큼만 남기고 제거한다.
    sRemovePageCount = sFreePagePool->mPageCount - sLimitMinFreePageCount;

    /* BUG-43464 compaction 기능에서 페이지 수를 제한할수 있도록 합니다.
     * #compact pages 가 입력되었고 freePage 수보다 적을 경우는
     * 입력된 수 만큼만 compaction 수행 */
    if ( (sRemovePageCount > aPages) && (aPages > 0) )
    {
        sRemovePageCount = aPages;
    }
    else
    {
        /* nothing to do */
    }

    ideLog::log( IDE_SM_0,
                 "[COMPACT TABLE] TABLEOID(%"ID_UINT64_FMT") "
                 "FreePages %"ID_UINT32_FMT" / %"ID_UINT32_FMT"\n",
                 sTableOID, 
                 sRemovePageCount, 
                 sFreePagePool->mPageCount );

    // for NTA
    sLsnNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

    // Before :
    // sFreePagePool : []-[]-[]-[]
    // After :
    // sFreePagePool :       []-[]
        
    sCurFreePageHeader = sFreePagePool->mHead;

    /* BUG-27516 [5.3.3 release] Klocwork SM (5) 
     * CurFreePageHeader가 Null이란 뜻은 FreePagePool이 비었다는 뜻으로
     * DB로 반환할 페이지가 없다는 뜻이다.
     */
    if ( sCurFreePageHeader == NULL )
    {
        sIsLocked = ID_FALSE;
        IDE_TEST( sFreePagePool->mMutex.unlock() != IDE_SUCCESS );
        IDE_CONT( SKIP_FREE_PAGES );
    }
    else
    {
        /* nothing to do */
    }

    // To fix BUG-28189
    // 제거하고자 하는 FreePagePool 의 위치를 기억해둔다.
    sHeadFreePageHeader = sFreePagePool->mHead;
    sTailFreePageHeader = sFreePagePool->mHead;

    // FreePagePool의 Head부터 sRemovePageCount만큼
    // sRemoveHead~sRemoveTail을 만들어 제거
    IDE_ERROR_MSG( smmManager::getPersPagePtr( aSpaceID,
                                               sCurFreePageHeader->mSelfPageID,
                                               (void**)&sRemoveHead )
                   == IDE_SUCCESS,
                   "TableOID : %"ID_UINT64_FMT"\n"
                   "SpaceID  : %"ID_UINT32_FMT"\n"
                   "PageID   : %"ID_UINT32_FMT"\n",
                   aPageListEntry->mTableOID,
                   aSpaceID,
                   sCurFreePageHeader->mSelfPageID );

    sRemovePrevID = SM_NULL_PID;

    // To fix BUG-28189
    // sTailFreePageHeader 의 위치를 맨 끝으로 옮긴다.
    // 제거하고자 하는 count 보다 1 작게 옮겨야 list 의 끝으로 옮길 수 있다.
    // 그렇지 않으면 제거하고자 하는 list 의 범위를 벗어나게 된다.
    for( sPageCounter = 0;
         sPageCounter < (sRemovePageCount - 1);
         sPageCounter++ )
    {
        // BUG-29271 [Codesonar] Null Pointer Dereference
        IDE_ERROR( sTailFreePageHeader != NULL );

        sTailFreePageHeader = sTailFreePageHeader->mFreeNext;
    }

    // BUG-28990, BUG-29271 [Codesonar] Null Pointer Dereference
    // SMP_FREEPAGELIST_MINPAGECOUNT 값으로 sRemovePageCount를 최소한 한페이지를
    // 남기고 제거하도록 조절해 주기 때문에 sTailFreePageHeader의
    // mFreeNext는 NULL이 되면 안된다.
    IDE_ERROR( sTailFreePageHeader != NULL );
    IDE_ERROR( sTailFreePageHeader->mFreeNext != NULL );

    // To fix BUG-28189
    // 제거한 sRemoveTail의 다음 Page가 FreePagePool의 Head가 되어야 한다.
    sFreePagePool->mHead            = sTailFreePageHeader->mFreeNext;
    sFreePagePool->mHead->mFreePrev = NULL;
    sFreePagePool->mPageCount      -= sRemovePageCount;
    sTailFreePageHeader->mFreeNext  = NULL;


    /* 부분적인 페이지 반환이 있기 때문에 남은 페이지의 수는 limit 보다
       크거나(주어진 #page 만큼 free), 같기만(max로 free) 하면 된다 */
    IDE_ERROR_MSG( sFreePagePool->mPageCount >= sLimitMinFreePageCount,
                   "FreePageCount  : %"ID_UINT32_FMT"\n"
                   "LimitPageCount : %"ID_UINT32_FMT"\n",
                   sFreePagePool->mPageCount,
                   sLimitMinFreePageCount );

    IDE_DASSERT( isValidFreePageList( sHeadFreePageHeader,
                                      sTailFreePageHeader,
                                      sRemovePageCount )
                 == ID_TRUE );

    IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                      sFreePagePool->mTail,
                                      sFreePagePool->mPageCount )
                 == ID_TRUE );

    sIsLocked = ID_FALSE;
    IDE_TEST( sFreePagePool->mMutex.unlock() != IDE_SUCCESS );

    // To fix BUG-28189
    // FreePageList 에서 제거하고자 하는 List 를 떼어냈으면 sState 가 1 이다.
    sState = 1;

    // To fix BUG-28189
    // compact table 과 Ager 사이에 동시성 제어가 되지 않습니다.
    IDU_FIT_POINT( "1.BUG-28189@smpFreePageList::freePagesFromFreePagePoolToDB" );

    for( sPageCounter = 0;
         sPageCounter < sRemovePageCount;
         sPageCounter++ )
    {
        IDE_ERROR(sCurFreePageHeader != NULL);
        
        sCurPageID = sCurFreePageHeader->mSelfPageID;

        // FreePage를 반납하기 전에 AllocPageList에서도 해당 Page를 제거해야 한다.

        IDE_ERROR_MSG( smmManager::getPersPagePtr( aSpaceID,
                                                   sCurPageID,
                                                   (void**)&sCurPageHeader )
                       == IDE_SUCCESS,
                       "TableOID : %"ID_UINT64_FMT"\n"
                       "SpaceID  : %"ID_UINT32_FMT"\n"
                       "PageID   : %"ID_UINT32_FMT"\n",
                       aPageListEntry->mTableOID,
                       aSpaceID,
                       sCurPageID );

        sPageListID    = sCurPageHeader->mAllocListID;
        sAllocPageList = &(aPageListEntry->mRuntimeEntry->mAllocPageList[sPageListID]);

        IDE_ERROR_MSG( sCurFreePageHeader->mFreeSlotCount
                       == aPageListEntry->mSlotCount,
                       "FreePage.FreeSlotCount : %"ID_UINT32_FMT"\n"
                       "PageList.SlotCount     : %"ID_UINT32_FMT"\n",
                       sCurFreePageHeader->mFreeSlotCount,
                       aPageListEntry->mSlotCount );

        // DB로 반납할 PageList의 링크를 연결하기 위해 Next를 구한다.
        if(sPageCounter == sRemovePageCount - 1)
        {
            // sRemoveTail의 Next는 NULL
            sFreeNextID = SM_NULL_PID;
        }
        else
        {
            // fix BUG-26934 : [codeSonar] Null Pointer Dereference
            IDE_ERROR( sCurFreePageHeader->mFreeNext != NULL );
            
            sFreeNextID = sCurFreePageHeader->mFreeNext->mSelfPageID;
        }

        IDE_TEST( smpAllocPageList::removePage( aTrans,
                                                aSpaceID,
                                                sAllocPageList,
                                                sCurPageID,
                                                sTableOID )
                  != IDE_SUCCESS );
        
        // BUGBUG : DB에 반납하는 페이지리스트는 Durable하지 않기 때문에 로깅 불필요
        
        IDE_TEST( smrUpdate::updateLinkAtPersPage(
                                              NULL /* idvSQL* */,
                                              aTrans,
                                              aSpaceID,
                                              sCurPageID,
                                              sCurPageHeader->mPrevPageID,
                                              sCurPageHeader->mNextPageID,
                                              sRemovePrevID,
                                              sFreeNextID ) 
                  != IDE_SUCCESS );
        
        
        sCurPageHeader->mPrevPageID = sRemovePrevID;
        // 다음 RemovePage의 RemovePrevID는 CurPageID이다.
        sRemovePrevID         = sCurPageID;

        // 반납할 FreePage를 sRemoveHead ~ sRemoveTail 의 리스트로 만들어 반납한다.
        
        sRemoveTail = (smpPersPage*)sCurPageHeader;
        sCurPageHeader->mNextPageID = sFreeNextID;
        
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID, sCurPageID)
                  != IDE_SUCCESS);

        sCurFreePageHeader = sCurFreePageHeader->mFreeNext;

        // To fix BUG-28189
        // compact table 과 Ager 사이에 동시성 제어가 되지 않습니다.
        IDU_FIT_POINT( "2.BUG-28189@smpFreePageList::freePagesFromFreePagePoolToDB" );
    }

    IDE_DASSERT( smpAllocPageList::isValidPageList( aSpaceID,
                                                    sRemoveHead->mHeader.mSelfPageID,
                                                    sRemoveTail->mHeader.mSelfPageID,
                                                    sRemovePageCount )
                 == ID_TRUE );

    // sRemoveHead~sRemoveTail을 DB로 반납
    IDE_TEST(smmManager::freePersPageList( aTrans,
                                           aSpaceID,
                                           sRemoveHead,
                                           sRemoveTail,
                                           &sLsnNTA ) != IDE_SUCCESS);
    
    IDE_EXCEPTION_CONT( SKIP_FREE_PAGES );


    IDE_EXCEPTION_CONT( no_more_need_compact );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( sFreePagePool->mMutex.unlock() == IDE_SUCCESS );
    }
    else 
    {
        /* 순서상 sState 가 1이려면 lock 은 풀려 있어야 한다. */
        IDE_DASSERT( sState == 1 ); 
    }

    if( sState == 1 )
    {
        // To fix BUG-28189
        // freePage 를 수행하는 도중 예외가 발생하면 FreePagePool 을 다시 원복시켜줘야 한다.
        IDE_ASSERT( sFreePagePool->mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS );

        sFreePagePool->mHead->mFreePrev = sTailFreePageHeader;
        sTailFreePageHeader->mFreeNext  = sFreePagePool->mHead;
        sFreePagePool->mHead            = sHeadFreePageHeader;
        sFreePagePool->mPageCount      += sRemovePageCount;
            
        IDE_DASSERT( isValidFreePageList( sFreePagePool->mHead,
                                          sFreePagePool->mTail,
                                          sFreePagePool->mPageCount )
                     == ID_TRUE );

        IDE_ASSERT( sFreePagePool->mMutex.unlock() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;    
}

/**********************************************************************
 * FreePageHeader 생성 혹은 DB에서 페이지 할당시 초기화
 *
 * aFreePageHeader : 초기화하려는 FreePageHeader
 **********************************************************************/

void smpFreePageList::initializeFreePageHeader(
                                    smpFreePageHeader* aFreePageHeader )
{
    IDE_DASSERT( aFreePageHeader != NULL );
    
    aFreePageHeader->mFreePrev      = NULL;
    aFreePageHeader->mFreeNext      = NULL;
    aFreePageHeader->mFreeSlotHead  = NULL;
    aFreePageHeader->mFreeSlotTail  = NULL;
    aFreePageHeader->mFreeSlotCount = 0;
    aFreePageHeader->mSizeClassID   = SMP_SIZECLASSID_NULL;
    aFreePageHeader->mFreeListID    = SMP_PAGELISTID_NULL;


    return;
}

/**********************************************************************
 * PageListEntry의 모든 FreePageHeader 초기화
 *
 * aPageListEntry : 초기화하려는 PageListEntry
 **********************************************************************/

void smpFreePageList::initAllFreePageHeader( scSpaceID         aSpaceID,
                                             smpPageListEntry* aPageListEntry )
{
    scPageID sPageID;
    
    IDE_DASSERT( aPageListEntry != NULL );

    sPageID = smpManager::getFirstAllocPageID(aPageListEntry);
    
    while(sPageID != SM_NULL_PID)
    {
        initializeFreePageHeader( getFreePageHeader(aSpaceID, sPageID) );

        sPageID = smpManager::getNextAllocPageID( aSpaceID,
                                                  aPageListEntry,
                                                  sPageID );
    }

    
    return;
}

/**********************************************************************
 * FreePageHeader 초기화
 *
 * aPageID : 초기화하려는 Page의 ID
 **********************************************************************/

IDE_RC smpFreePageList::initializeFreePageHeaderAtPCH( scSpaceID aSpaceID,
                                                       scPageID  aPageID )
{
    SChar               sBuffer[128];
    smmPCH            * sPCH;
    smpFreePageHeader * sFreePageHeader;
    UInt                sState  = 0;

    // FreePageHeader 초기화

    // BUGBUG : aPageID 범위 검사 필요!!
    sPCH = smmManager::getPCH(aSpaceID, aPageID);

    IDE_DASSERT( sPCH != NULL );
    
    /* smpFreePageList_initializeFreePageHeaderAtPCH_malloc_FreePageHeader.tc */
    IDU_FIT_POINT("smpFreePageList::initializeFreePageHeaderAtPCH::malloc::FreePageHeader");
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_SM_SMP,
                                ID_SIZEOF(smpFreePageHeader),
                                (void**)&(sPCH->mFreePageHeader),
                                IDU_MEM_FORCE )
             != IDE_SUCCESS);
    sState = 1;

    sFreePageHeader = (smpFreePageHeader*)(sPCH->mFreePageHeader);

    sFreePageHeader->mSelfPageID = aPageID;

    initializeFreePageHeader( sFreePageHeader );
    
    idlOS::snprintf(sBuffer, 128, "FREE_PAGE_MUTEX_%"ID_XINT64_FMT"", aPageID);

    IDE_TEST(sFreePageHeader->mMutex.initialize( sBuffer,
                                                 IDU_MUTEX_KIND_NATIVE,
                                                 IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sPCH->mFreePageHeader ) == IDE_SUCCESS );
            sPCH->mFreePageHeader = NULL;
        default:
            break;
    }


    return IDE_FAILURE;    
}

/**********************************************************************
 * FreePageHeader 제거
 *
 * aPageID : 제거하려는 Page의 ID
 **********************************************************************/

IDE_RC smpFreePageList::destroyFreePageHeaderAtPCH( scSpaceID aSpaceID,
                                                    scPageID  aPageID )
{
    smmPCH*            sPCH;
    smpFreePageHeader* sFreePageHeader;

    sFreePageHeader = getFreePageHeader(aSpaceID, aPageID);

    IDE_DASSERT( sFreePageHeader != NULL );
    
    IDE_TEST(sFreePageHeader->mMutex.destroy() != IDE_SUCCESS);
    IDE_TEST(iduMemMgr::free(sFreePageHeader) != IDE_SUCCESS);

    sPCH = smmManager::getPCH(aSpaceID, aPageID);

    IDE_DASSERT( sPCH != NULL );
    
    sPCH->mFreePageHeader = NULL;
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/**********************************************************************
 * FreePage에 대한 SizeClass 변경
 *
 * SizeClass를 변경하기 위해 해당 Page는 lock을 잡고 들어와야하며,
 * List/Pool은 옮기는 작업을 할때 lock을 잡고 작업한다.
 *
 * aTrans          : 작업하는 트랜잭션 객체
 * aPageListEntry  : FreePage의 소속 PageListEntry
 * aFreePageHeader : SizeClass 변경하려는 FreePageHeader
 **********************************************************************/
IDE_RC smpFreePageList::modifyPageSizeClass( void*              aTrans,
                                             smpPageListEntry*  aPageListEntry,
                                             smpFreePageHeader* aFreePageHeader )
{
#ifdef DEBUG
    UInt                  sOldSizeClassID;
    UInt                  sSizeClassCount;
#endif
    UInt                  sNewSizeClassID;
    UInt                  sOldPageListID;
    UInt                  sNewPageListID;
    smpFreePageListEntry* sFreePageList;

    IDE_DASSERT( aPageListEntry != NULL );
    IDE_DASSERT( aFreePageHeader != NULL );
    
    IDE_ASSERT( aFreePageHeader->mFreeListID != SMP_POOL_PAGELISTID );
    
#ifdef DEBUG
    sOldSizeClassID = aFreePageHeader->mSizeClassID;
    sSizeClassCount = SMP_SIZE_CLASS_COUNT( aPageListEntry->mRuntimeEntry );
#endif
    sOldPageListID  = aFreePageHeader->mFreeListID;
        
    // 변경된 SizeClassID 값을 구한다.
    sNewSizeClassID = getSizeClass( aPageListEntry->mRuntimeEntry,
                                    aPageListEntry->mSlotCount,
                                    aFreePageHeader->mFreeSlotCount );

#ifdef DEBUG
    IDE_DASSERT( sNewSizeClassID < sSizeClassCount );
#endif
    if( sOldPageListID == SMP_PAGELISTID_NULL )
    {
        IDE_DASSERT(sOldSizeClassID == SMP_SIZECLASSID_NULL);
        
        // Slot전체가 사용중이던 Page라서 FreePageList에 없었다면
        // 현재 트랜잭션의 RSGroupID값의 FreePageList에 등록한다.

        smLayerCallback::allocRSGroupID( aTrans,
                                         &sNewPageListID );
    }
    else
    {
        sNewPageListID = sOldPageListID;
    }

    if( (sNewSizeClassID != aFreePageHeader->mSizeClassID) ||
        (aFreePageHeader->mFreeSlotCount == 0) ||
        (aFreePageHeader->mFreeSlotCount == aPageListEntry->mSlotCount) )
    {
        sFreePageList =
            &(aPageListEntry->mRuntimeEntry->mFreePageList[sNewPageListID]);

        // 현재 Free Page가 페이지 리스트에 있었는가?
        if(sOldPageListID != SMP_PAGELISTID_NULL)
        {
            IDE_DASSERT(sOldSizeClassID != SMP_SIZECLASSID_NULL);
            
            // 기존 SizeClass에서 Page 분리
            IDE_TEST( removePageFromFreePageList(aPageListEntry,
                                                 sOldPageListID,
                                                 aFreePageHeader->mSizeClassID,
                                                 aFreePageHeader)
                      != IDE_SUCCESS );
        }
            
        // FreeSlot이 하나라도 있는가?
        // FreeSlot이 없다면 FreePage가 아니므로 FreePageList에 등록하지 않는다.
        if(aFreePageHeader->mFreeSlotCount != 0)
        {
            if((aFreePageHeader->mFreeSlotCount == aPageListEntry->mSlotCount) &&
               (sFreePageList->mPageCount[sNewSizeClassID] >= SMP_FREEPAGELIST_MINPAGECOUNT))
            {
                // Page의 Slot 전체가 FreeSlot이면
                // aFreePageHeader는 완전 EmptyPage이다.
                
                // FreePageList에 PageCount가 FREE_THRESHOLD보다 많다면
                // FreePagePool에 등록한다.
                IDE_TEST( addPageToFreePagePool( aPageListEntry,
                                                 aFreePageHeader )
                          != IDE_SUCCESS );
            }
            else
            {
                // EmptyPage가 아니거나 FreePageList에 아직 여유가 있다.

                // BUGBUG: addPageToFreePageListHead로 인해 Emergency용
                //         FreeSlot이 변경되어 INSERT 순서와 SELECT 순서가
                //         맞지 않는 문제 발생으로 우선 무조건 Tail로 등록
                //         추후 Head로 이동하는 것이 성능에 좋은지,
                //         혹은 Emergency용 FreeSlot대신 DB에서 Emergency용
                //         FreePage를 갖는 방법으로 개선할 필요있음.

                IDE_TEST( addPageToFreePageListTail( aPageListEntry,
                                                     sNewPageListID,
                                                     sNewSizeClassID,
                                                     aFreePageHeader )
                          != IDE_SUCCESS );
            }   
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/**********************************************************************
 * FreePageList에서 FreePage 제거
 *
 * aPageListEntry  : FreePage의 소속 PageListEntry
 * aPageListID     : FreePage의 소속 FreePageList ID
 * aSizeClassID    : FreePage의 소속 SizeClass ID
 * aFreePageHeader : 제거하려는 FreePageHeader
 **********************************************************************/

IDE_RC smpFreePageList::removePageFromFreePageList(
                                    smpPageListEntry  * aPageListEntry,
                                    UInt                aPageListID,
                                    UInt                aSizeClassID,
                                    smpFreePageHeader * aFreePageHeader )
{
    UInt                  sState = 0;
    smpFreePageListEntry* sFreePageList;
    smpFreePageHeader*    sPrevFreePageHeader;
    smpFreePageHeader*    sNextFreePageHeader;

    IDE_DASSERT( aPageListEntry != NULL );
    IDE_DASSERT( aFreePageHeader != NULL );
    
    sFreePageList = &(aPageListEntry->mRuntimeEntry->mFreePageList[aPageListID]);

    IDE_DASSERT( sFreePageList != NULL );
    
    IDE_TEST( sFreePageList->mMutex.lock(NULL /* idvSQL* */)
              != IDE_SUCCESS );
    sState = 1;
    
    IDE_DASSERT( aSizeClassID == aFreePageHeader->mSizeClassID );
    IDE_DASSERT( aPageListID  == aFreePageHeader->mFreeListID  );
    
    sPrevFreePageHeader = aFreePageHeader->mFreePrev;
    sNextFreePageHeader = aFreePageHeader->mFreeNext;

    if(sPrevFreePageHeader == NULL)
    {
        // FreePrev가 없다면 HeadPage이다.
        IDE_DASSERT( sFreePageList->mHead[aSizeClassID] == aFreePageHeader );
        
        sFreePageList->mHead[aSizeClassID] = sNextFreePageHeader;
    }
    else
    {
        IDE_DASSERT( sFreePageList->mHead[aSizeClassID] != aFreePageHeader );

        // 앞쪽 링크를 끊는다.
        sPrevFreePageHeader->mFreeNext = sNextFreePageHeader;
    }

    if(sNextFreePageHeader == NULL)
    {
        // FreeNext가 없다면 TailPage이다.
        IDE_DASSERT( sFreePageList->mTail[aSizeClassID] == aFreePageHeader );
        
        sFreePageList->mTail[aSizeClassID] = sPrevFreePageHeader;
    }
    else
    {
        IDE_DASSERT( sFreePageList->mTail[aSizeClassID] != aFreePageHeader );

        // 뒤쪽 링크를 끊는다.
        sNextFreePageHeader->mFreePrev = sPrevFreePageHeader;
    }

    aFreePageHeader->mFreePrev    = NULL;
    aFreePageHeader->mFreeNext    = NULL;
    aFreePageHeader->mFreeListID  = SMP_PAGELISTID_NULL;
    aFreePageHeader->mSizeClassID = SMP_SIZECLASSID_NULL;

    sFreePageList->mPageCount[aSizeClassID]--;
 
    IDE_DASSERT( isValidFreePageList( sFreePageList->mHead[aSizeClassID],
                                      sFreePageList->mTail[aSizeClassID],
                                      sFreePageList->mPageCount[aSizeClassID] )
                 == ID_TRUE );

    sState = 0;
    IDE_TEST( sFreePageList->mMutex.unlock() != IDE_SUCCESS );
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT(sFreePageList->mMutex.unlock() == IDE_SUCCESS);
    }

    IDE_POP();
    

    return IDE_FAILURE;
}

/**********************************************************************
 * FreePageList의 Tail에 FreePage 등록
 *
 * aPageListEntry  : FreePage의 소속 PageListEntry
 * aPageListID     : FreePage의 소속 FreePageList ID
 * aSizeClassID    : FreePage의 소속 SizeClass ID
 * aFreePageHeader : 등록하려는 FreePageHeader
 **********************************************************************/

IDE_RC smpFreePageList::addPageToFreePageListTail(
                                        smpPageListEntry  * aPageListEntry,
                                        UInt                aPageListID,
                                        UInt                aSizeClassID,
                                        smpFreePageHeader * aFreePageHeader )
{
    UInt                  sState = 0;
    smpFreePageListEntry* sFreePageList;
    smpFreePageHeader*    sTailPageHeader;

    IDE_DASSERT( aPageListEntry != NULL );
    IDE_DASSERT( aFreePageHeader != NULL );
    IDE_ERROR( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_ERROR( aSizeClassID < SMP_SIZE_CLASS_COUNT( aPageListEntry->mRuntimeEntry ) );
    
    sFreePageList = &(aPageListEntry->mRuntimeEntry->mFreePageList[aPageListID]);

    IDE_ERROR( sFreePageList != NULL );
    
    // aFreePageHeader가 유효한지 검사
    IDE_ERROR( aFreePageHeader->mFreeSlotCount > 0 );
    IDE_ERROR( aFreePageHeader->mFreeListID == SMP_PAGELISTID_NULL );
    IDE_ERROR( aFreePageHeader->mSizeClassID == SMP_SIZECLASSID_NULL );
    
    IDE_TEST( sFreePageList->mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS );
    sState = 1;
    
    sTailPageHeader = sFreePageList->mTail[aSizeClassID];

    if(sTailPageHeader == NULL)
    {
        IDE_ERROR( sFreePageList->mHead[aSizeClassID] == NULL );
        
        // Head가 비어있다면 FreePage로 Head/Tail을 다 채워야 한다.
        sFreePageList->mHead[aSizeClassID] = aFreePageHeader;
    }
    else /* sTailPageHeader != NULL */
    {        
        IDE_ERROR( sFreePageList->mHead[aSizeClassID] != NULL );
        
        sTailPageHeader->mFreeNext = aFreePageHeader;
    }
    
    sFreePageList->mTail[aSizeClassID] = aFreePageHeader;

    aFreePageHeader->mFreePrev    = sTailPageHeader;
    aFreePageHeader->mFreeNext    = (smpFreePageHeader *)SM_NULL_PID;
    aFreePageHeader->mSizeClassID = aSizeClassID;
    aFreePageHeader->mFreeListID  = aPageListID;
    
    sFreePageList->mPageCount[aSizeClassID]++;

    IDE_DASSERT( isValidFreePageList( sFreePageList->mHead[aSizeClassID],
                                      sFreePageList->mTail[aSizeClassID],
                                      sFreePageList->mPageCount[aSizeClassID] )
                 == ID_TRUE );

    sState = 0;
    IDE_TEST( sFreePageList->mMutex.unlock() != IDE_SUCCESS );
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT(sFreePageList->mMutex.unlock() == IDE_SUCCESS);
    }

    IDE_POP();

    return IDE_FAILURE;
}


/**********************************************************************
 * refineDB 때 각 Page를 Scan하면서 FreePage일 경우 0번째 FreePageList에
 * 등록한 후, buildFreePageList때 0번째 FreePageList에서 각 FreePageList에
 * FreePage들을 나누어 준다.
 *
 * aPageListEntry : 검사하려는 PageListEntry
 * aRecordCount   : Record 갯수
 **********************************************************************/

IDE_RC smpFreePageList::addPageToFreePageListAtInit(
    smpPageListEntry*  aPageListEntry,
    smpFreePageHeader* aFreePageHeader )
{
    UInt sSizeClassID = 0;

    IDE_DASSERT( aPageListEntry != NULL );
    IDE_DASSERT( aFreePageHeader != NULL );
    IDE_ERROR( aFreePageHeader->mFreeSlotCount > 0 );
    IDE_ERROR( aFreePageHeader->mFreeListID == SMP_PAGELISTID_NULL );
    
    sSizeClassID = getSizeClass(aPageListEntry->mRuntimeEntry,
                                aPageListEntry->mSlotCount,
                                aFreePageHeader->mFreeSlotCount);
    
    IDE_TEST( addPageToFreePageListTail( aPageListEntry,
                                         0,
                                         sSizeClassID,
                                         aFreePageHeader )
              != IDE_SUCCESS );
    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "mTableOID     :%llu\n"
                 "mSlotCount    :%u\n"
                 "mSlotSize     :%llu\n"
                 "sSizeClassID  :%u\n"
                 "mFreeListID   :%u\n"
                 "mSizeClassID  :%u\n"
                 "mFreeSlotCount:%u\n"
                 "mSelfPageID   :%u\n",
                 aPageListEntry->mTableOID,
                 aPageListEntry->mSlotCount,
                 aPageListEntry->mSlotSize,
                 sSizeClassID,
                 aFreePageHeader->mFreeListID,
                 aFreePageHeader->mSizeClassID,
                 aFreePageHeader->mFreeSlotCount,
                 aFreePageHeader->mSelfPageID );


    return IDE_FAILURE;
}

/**********************************************************************
 * PrivatePageList의 FreePage들을 aPageListEntry에 추가한다.
 *
 * aTrans           : 작업하는 트랜잭션 객체
 * aPageListEntry   : 추가가 될 테이블의 PageListEntry
 * aHeadFreePage    : 추가하려는 FreePage들의 Head
 **********************************************************************/

IDE_RC smpFreePageList::addFreePagesToTable( void*              aTrans,
                                             smpPageListEntry*  aPageListEntry,
                                             smpFreePageHeader* aFreePageHead )
{
    smpFreePageHeader* sFreePageHeader = NULL;
    smpFreePageHeader* sNxtFreePageHeader;
    
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPageListEntry != NULL );

    sNxtFreePageHeader = aFreePageHead;

    while(sNxtFreePageHeader != NULL)
    {
        sFreePageHeader    = sNxtFreePageHeader;
        sNxtFreePageHeader = sFreePageHeader->mFreeNext;
        
        sFreePageHeader->mFreeListID  = SMP_PAGELISTID_NULL;
        sFreePageHeader->mSizeClassID = SMP_SIZECLASSID_NULL;

        IDE_TEST( modifyPageSizeClass( aTrans,
                                       aPageListEntry,
                                       sFreePageHeader )
                  != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/**********************************************************************
 * PrivatePageList에 FreePage 연결
 *
 * aCurPID  : FreeNext를 설정할 PageID
 * aPrevPID : FreePrev인 PageID
 * aNextPID : FreeNext인 PageID
 **********************************************************************/

void smpFreePageList::addFreePageToPrivatePageList( scSpaceID aSpaceID,
                                                    scPageID  aCurPID,
                                                    scPageID  aPrevPID,
                                                    scPageID  aNextPID )
{
    smpFreePageHeader* sCurFreePageHeader;
    smpFreePageHeader* sPrevFreePageHeader;
    smpFreePageHeader* sNextFreePageHeader;

    IDE_DASSERT(aCurPID != SM_NULL_PID);

    sCurFreePageHeader = getFreePageHeader(aSpaceID, aCurPID);

    if(aPrevPID == SM_NULL_PID)
    {
        sPrevFreePageHeader = NULL;
    }
    else
    {
        sPrevFreePageHeader = getFreePageHeader(aSpaceID, aPrevPID);
    }

    if(aNextPID == SM_NULL_PID)
    {
        sNextFreePageHeader = NULL;
    }
    else
    {
        sNextFreePageHeader = getFreePageHeader(aSpaceID, aNextPID);
    }

    // FreePageHeader 정보 초기화
    sCurFreePageHeader->mFreeListID  = SMP_PRIVATE_PAGELISTID;
    sCurFreePageHeader->mSizeClassID = SMP_PRIVATE_SIZECLASSID;
    sCurFreePageHeader->mFreePrev    = sPrevFreePageHeader;
    sCurFreePageHeader->mFreeNext    = sNextFreePageHeader;


    return;
}

/**********************************************************************
 * PrivatePageList에서 FixedFreePage 제거
 *
 * aPrivatePageList : 제거될 PrivatePageList
 * aFreePageHeader  : 제거할 FreePageHeader
 **********************************************************************/

void smpFreePageList::removeFixedFreePageFromPrivatePageList(
    smpPrivatePageListEntry* aPrivatePageList,
    smpFreePageHeader*       aFreePageHeader )
{
    IDE_DASSERT(aPrivatePageList != NULL);
    IDE_DASSERT(aFreePageHeader != NULL);

    if(aFreePageHeader->mFreePrev == NULL)
    {
        // Prev가 없다면 Head이다.
        IDE_DASSERT( aPrivatePageList->mFixedFreePageHead == aFreePageHeader );

        aPrivatePageList->mFixedFreePageHead = aFreePageHeader->mFreeNext;
    }
    else
    {
        IDE_DASSERT( aPrivatePageList->mFixedFreePageHead != NULL );

        aFreePageHeader->mFreePrev->mFreeNext = aFreePageHeader->mFreeNext;
    }

    if(aFreePageHeader->mFreeNext == NULL)
    {
        // Next가 없다면 Tail이다.
        IDE_DASSERT( aPrivatePageList->mFixedFreePageTail == aFreePageHeader );

        aPrivatePageList->mFixedFreePageTail = aFreePageHeader->mFreePrev;
    }
    else
    {
        IDE_DASSERT( aPrivatePageList->mFixedFreePageTail != NULL );

        aFreePageHeader->mFreeNext->mFreePrev = aFreePageHeader->mFreePrev;
    }

    initializeFreePageHeader(aFreePageHeader);


    return;
}

#ifdef DEBUG
/**********************************************************************
 * aHeadFreePage~aTailFreePage까지의 FreePageList의 연결이 올바른지 검사한다.
 *
 * aHeadFreePage : 검사하려는 List의 Head
 * aTailFreePage : 검사하려는 List의 Tail
 * aPageCount    : 검사하려는 List의 Page의 갯수
 **********************************************************************/
idBool smpFreePageList::isValidFreePageList( smpFreePageHeader* aHeadFreePage,
                                             smpFreePageHeader* aTailFreePage,
                                             vULong             aPageCount )
{
    idBool             sIsValid;
    vULong             sPageCount = 0;
    smpFreePageHeader* sCurFreePage = NULL;
    smpFreePageHeader* sNxtFreePage;
    
    if ( iduProperty::getEnableRecTest() == 1 )
    {
        sIsValid = ID_FALSE;
    
        sNxtFreePage = aHeadFreePage;
    
        while(sNxtFreePage != NULL)
        {
            sCurFreePage = sNxtFreePage;
        
            sPageCount++;

            sNxtFreePage = sCurFreePage->mFreeNext;
        }

        if(sCurFreePage == aTailFreePage)
        {
            if(sPageCount == aPageCount)
            {
                sIsValid = ID_TRUE;
            }
        }
    }
    else
    {
       sIsValid = ID_TRUE;
    }

     return sIsValid;
}
#endif
