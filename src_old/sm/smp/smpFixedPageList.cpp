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
 * $Id: smpFixedPageList.cpp 91157 2021-07-07 06:02:43Z jiwon.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smr.h>
#include <smpDef.h>
#include <smpFixedPageList.h>
#include <smpFreePageList.h>
#include <smpAllocPageList.h>
#include <smmExpandChunk.h>
#include <smpReq.h>

/**********************************************************************
 * Tx's PrivatePageList의 FreePage로부터 Slot를 할당할 수 있을지 검사하고
 * 가능하면 할당한다.
 *
 * aTrans     : 작업하는 트랜잭션 객체
 * aTableOID  : 할당하려는 테이블 OID
 * aRow       : 할당해서 반환하려는 Slot 포인터
 **********************************************************************/

IDE_RC smpFixedPageList::tryForAllocSlotFromPrivatePageList(
    void              * aTrans,
    scSpaceID           aSpaceID,
    smOID               aTableOID,
    smpPageListEntry  * aFixedEntry,
    SChar            ** aRow )
{
    smpPrivatePageListEntry* sPrivatePageList = NULL;
    smpFreePageHeader*       sFreePageHeader;

    IDE_DASSERT(aTrans != NULL);
    IDE_DASSERT(aRow != NULL);

    *aRow = NULL;

    IDE_TEST( smLayerCallback::findPrivatePageList( aTrans,
                                                    aTableOID,
                                                    &sPrivatePageList )
              != IDE_SUCCESS );

    if(sPrivatePageList != NULL)
    {
        sFreePageHeader = sPrivatePageList->mFixedFreePageHead;

        if(sFreePageHeader != NULL)
        {
            IDE_DASSERT(sFreePageHeader->mFreeSlotCount > 0);

            removeSlotFromFreeSlotList(sFreePageHeader, aRow);

            if( sFreePageHeader->mFreeSlotCount == (aFixedEntry->mSlotCount-1) )
            {
                IDE_TEST( linkScanList( aSpaceID,
                                        sFreePageHeader->mSelfPageID,
                                        aFixedEntry )
                          != IDE_SUCCESS );
            }

            if(sFreePageHeader->mFreeSlotCount == 0)
            {
                smpFreePageList::removeFixedFreePageFromPrivatePageList(
                    sPrivatePageList,
                    sFreePageHeader);
            }
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/***********************************************************************
 * FreePageList나 FreePagePool에서 FreeSlot을 할당할 수 있는지 시도
 * 할당이 되면 aRow로 반환하고 할당할 FreeSlot이 없다면 aRow를 NULL로 반환
 *
 * aTrans      : 작업하는 트랜잭션 객체
 * aFixedEntry : Slot을 할당하려는 PageListEntry
 * aPageListID : Slot을 할당하려는 PageListID
 * aRow        : 할당해서 반환하려는 Slot
 ***********************************************************************/
IDE_RC smpFixedPageList::tryForAllocSlotFromFreePageList(
    void*             aTrans,
    scSpaceID         aSpaceID,
    smpPageListEntry* aFixedEntry,
    UInt              aPageListID,
    SChar**           aRow )
{
    UInt                  sState = 0;
    UInt                  sSizeClassID;
    idBool                sIsPageAlloced;
    smpFreePageHeader*    sFreePageHeader = NULL;
    smpFreePageHeader*    sNextFreePageHeader;
    smpFreePageListEntry* sFreePageList;
    UInt                  sSizeClassCount;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_DASSERT( aRow != NULL );

    sFreePageList = &(aFixedEntry->mRuntimeEntry->mFreePageList[aPageListID]);

    IDE_DASSERT( sFreePageList != NULL );

    *aRow = NULL;

    sSizeClassCount = SMP_SIZE_CLASS_COUNT( aFixedEntry->mRuntimeEntry );

    while(1)
    {
        // FreePageList의 SizeClass를 순회하면서 tryAllocSlot한다.
        for(sSizeClassID = 0;
            sSizeClassID < sSizeClassCount;
            sSizeClassID++)
        {
            sFreePageHeader = sFreePageList->mHead[sSizeClassID];
            // list의 Head를 가져올때 lock을 안잡은 이유는
            // freeSlot에서는 PageLock잡고 ListLock잡기 때문에
            // 여기서 list Lock먼저 잡고 HeadPage Lock잡으면 데드락 발생
            // 그래서 먼저 Head를 가져와서 PageLock잡고 다시 확인하여 해결


            while(sFreePageHeader != NULL)
            {
                // 해당 Page에 대해 Slot을 할당하려고 하고
                // 할당하게되면 해당 Page의 속성이 변경되므로 lock으로 보호
                IDE_TEST(sFreePageHeader->mMutex.lock( NULL )
                         != IDE_SUCCESS);
                sState = 1;

                // lock잡기전에 해당 Page에 대해 다른 Tx에 의해 변경되었는지 검사
                if(sFreePageHeader->mFreeListID == aPageListID)
                {
                    IDE_ASSERT(sFreePageHeader->mFreeSlotCount > 0);

                    removeSlotFromFreeSlotList(sFreePageHeader, aRow);

                    if( sFreePageHeader->mFreeSlotCount == (aFixedEntry->mSlotCount-1) )
                    {
                        IDE_TEST( linkScanList( aSpaceID,
                                                sFreePageHeader->mSelfPageID,
                                                aFixedEntry )
                                  != IDE_SUCCESS );
                    }

                    // FreeSlot을 할당한 Page의 SizeClass가 변경되었는지
                    // 확인하여 조정
                    IDE_TEST(smpFreePageList::modifyPageSizeClass(
                                 aTrans,
                                 aFixedEntry,
                                 sFreePageHeader)
                             != IDE_SUCCESS);

                    sState = 0;
                    IDE_TEST(sFreePageHeader->mMutex.unlock()
                             != IDE_SUCCESS);

                    IDE_CONT(normal_case);
                }
                else
                {
                    // 해당 Page가 변경된 것이라면 List에서 다시 Head를 가져온다.
                    sNextFreePageHeader = sFreePageList->mHead[sSizeClassID];
                }

                sState = 0;
                IDE_TEST(sFreePageHeader->mMutex.unlock() != IDE_SUCCESS);

                sFreePageHeader = sNextFreePageHeader;
            }

        }

        // FreePageList에서 FreeSlot을 찾지 못했다면
        // FreePagePool에서 확인하여 가져온다.

        IDE_TEST( smpFreePageList::tryForAllocPagesFromPool( aFixedEntry,
                                                             aPageListID,
                                                             &sIsPageAlloced )
                  != IDE_SUCCESS );

        if(sIsPageAlloced == ID_FALSE)
        {
            // Pool에서 못가져왔다.
            IDE_CONT(normal_case);
        }
    }

    IDE_EXCEPTION_CONT( normal_case );

    IDE_ASSERT(sState == 0);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT( sFreePageHeader->mMutex.unlock() == IDE_SUCCESS );
    }

    IDE_POP();


    return IDE_FAILURE;
}

/****************************************************************************
 *
 * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
 *
 * 주어진 페이지를 Scan List로부터 제거한다.
 *
 ****************************************************************************/
IDE_RC smpFixedPageList::unlinkScanList( scSpaceID          aSpaceID,
                                         scPageID           aPageID,
                                         smpPageListEntry * aFixedEntry )
{
    smmPCH               * sMyPCH;
    smmPCH               * sNxtPCH;
    smmPCH               * sPrvPCH;
    smpScanPageListEntry * sScanPageList;

    sScanPageList = &(aFixedEntry->mRuntimeEntry->mScanPageList);

    IDU_FIT_POINT("smpFixedPageList::unlinkScanList::wait1");

    IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

    IDE_DASSERT( sScanPageList->mHeadPageID != SM_NULL_PID );
    IDE_DASSERT( sScanPageList->mTailPageID != SM_NULL_PID );

    sMyPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                           aPageID ));
    IDE_DASSERT( sMyPCH != NULL );
    IDE_DASSERT( sMyPCH->mNxtScanPID != SM_NULL_PID );
    IDE_DASSERT( sMyPCH->mPrvScanPID != SM_NULL_PID );

    /* BUG-43463 Fullscan 동시성 개선,
     * - smnnSeq::moveNextNonBlock()등 과의 동시성 향상을 위한 atomic 세팅
     * - 대상은 modify_seq, nexp_pid, prev_pid이다.
     * - modify_seq는 link, unlink시점에 변경된다.
     * - link, unlink 진행 중에는 modify_seq가 홀수이다
     * - 첫번째 page는 lock을 잡고 확인하므로 atomic으로 set하지 않아도 된다.
     * */
    SMM_PCH_SET_MODIFYING( sMyPCH );

    if( sScanPageList->mHeadPageID == aPageID )
    {
        if( sMyPCH->mNxtScanPID != SM_SPECIAL_PID )
        {
            IDE_DASSERT( sScanPageList->mTailPageID != aPageID );
            IDE_DASSERT( sMyPCH->mPrvScanPID == SM_SPECIAL_PID );
            sNxtPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                                    sMyPCH->mNxtScanPID ));
            sNxtPCH->mPrvScanPID       = sMyPCH->mPrvScanPID;
            sScanPageList->mHeadPageID = sMyPCH->mNxtScanPID;
        }
        else
        {
            IDE_DASSERT( sScanPageList->mTailPageID == aPageID );
            sScanPageList->mHeadPageID = SM_NULL_PID;
            sScanPageList->mTailPageID = SM_NULL_PID;
        }
    }
    else
    {
        IDE_DASSERT( sMyPCH->mPrvScanPID != SM_SPECIAL_PID );
        sPrvPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                                sMyPCH->mPrvScanPID ));

        if( sMyPCH->mNxtScanPID != SM_SPECIAL_PID )
        {
            sNxtPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                                    sMyPCH->mNxtScanPID ));
            idCore::acpAtomicSet32( &(sNxtPCH->mPrvScanPID),
                                    sMyPCH->mPrvScanPID );
        }
        else
        {
            IDE_DASSERT( sScanPageList->mTailPageID == aPageID );
            sScanPageList->mTailPageID = sMyPCH->mPrvScanPID;
        }
        idCore::acpAtomicSet32( &(sPrvPCH->mNxtScanPID),
                                sMyPCH->mNxtScanPID );
    }

    idCore::acpAtomicSet32( &(sMyPCH->mNxtScanPID),
                            SM_NULL_PID );

    idCore::acpAtomicSet32( &(sMyPCH->mPrvScanPID),
                            SM_NULL_PID );

    SMM_PCH_SET_MODIFIED( sMyPCH );

    IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 *
 * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
 *
 * 주어진 페이지를 Scan List로 추가한다.
 *
 ****************************************************************************/
IDE_RC smpFixedPageList::linkScanList( scSpaceID          aSpaceID,
                                       scPageID           aPageID,
                                       smpPageListEntry * aFixedEntry )
{
    smmPCH               * sMyPCH;
    smmPCH               * sTailPCH;
    smpScanPageListEntry * sScanPageList;
#ifdef DEBUG
    smpFreePageHeader    * sFreePageHeader;
#endif
    sScanPageList = &(aFixedEntry->mRuntimeEntry->mScanPageList);

    IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

#ifdef DEBUG
    sFreePageHeader = smpFreePageList::getFreePageHeader( aSpaceID,
                                                          aPageID );

    IDE_DASSERT( (sFreePageHeader == NULL) ||
                 (sFreePageHeader->mFreeSlotCount != aFixedEntry->mSlotCount) );
#endif
    sMyPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                           aPageID ));
    IDE_DASSERT( sMyPCH != NULL );
    IDE_DASSERT( sMyPCH->mNxtScanPID == SM_NULL_PID );
    IDE_DASSERT( sMyPCH->mPrvScanPID == SM_NULL_PID );

    /* BUG-43463 Fullscan 동시성 개선,
     * - smnnSeq::moveNextNonBlock()등 과의 동시성 향상을 위한 atomic 세팅
     * - 대상은 modify_seq, nexp_pid, prev_pid이다.
     * - modify_seq는 link, unlink시점에 변경된다.
     * - link, unlink 진행 중에는 modify_seq가 홀수이다
     * - 첫번째 page는 lock을 잡고 확인하므로 atomic으로 set하지 않아도 된다.
     * */
    SMM_PCH_SET_MODIFYING( sMyPCH );
    /*
     * Scan List가 비어 있다면
     */
    if( sScanPageList->mTailPageID == SM_NULL_PID )
    {
        IDE_DASSERT( sScanPageList->mHeadPageID == SM_NULL_PID );
        sScanPageList->mHeadPageID = aPageID;
        sScanPageList->mTailPageID = aPageID;

        sMyPCH->mNxtScanPID = SM_SPECIAL_PID;
        sMyPCH->mPrvScanPID = SM_SPECIAL_PID;
    }
    else
    {
        sTailPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                                 sScanPageList->mTailPageID ));
        idCore::acpAtomicSet32( &(sTailPCH->mNxtScanPID),
                                aPageID );

        idCore::acpAtomicSet32( &(sMyPCH->mNxtScanPID),
                                SM_SPECIAL_PID );

        idCore::acpAtomicSet32( &(sMyPCH->mPrvScanPID),
                                sScanPageList->mTailPageID );

        sScanPageList->mTailPageID = aPageID;
    }

    SMM_PCH_SET_MODIFIED( sMyPCH );

    IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Runtime Item을 NULL로 설정한다.
 * DISCARD/OFFLINE Tablespace에 속한 Table들에 대해 수행된다.
 *
 * aFixedEntry : 초기화하려는 PageListEntry
 ***********************************************************************/
IDE_RC smpFixedPageList::setRuntimeNull( smpPageListEntry* aFixedEntry )
{
    IDE_DASSERT( aFixedEntry != NULL );

    // RuntimeEntry 초기화
    IDE_TEST(smpFreePageList::setRuntimeNull( aFixedEntry )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * memory table의  fixed page list entry가 포함하는 runtime 정보 초기화
 *
 * aTableOID   : PageListEntry가 속하는 테이블 OID
 * aFixedEntry : 초기화하려는 PageListEntry
 ***********************************************************************/
IDE_RC smpFixedPageList::initEntryAtRuntime(
    smOID                  aTableOID,
    smpPageListEntry*      aFixedEntry,
    smpAllocPageListEntry* aAllocPageList )
{
    SChar                  sBuffer[128];
    smpScanPageListEntry * sScanPageList;

    IDE_DASSERT( aFixedEntry != NULL );
    IDE_DASSERT( aAllocPageList != NULL );

    IDE_ASSERT( aTableOID == aFixedEntry->mTableOID);

    // RuntimeEntry 초기화
    IDE_TEST(smpFreePageList::initEntryAtRuntime( aFixedEntry )
             != IDE_SUCCESS);

    aFixedEntry->mRuntimeEntry->mAllocPageList = aAllocPageList;
    sScanPageList = &(aFixedEntry->mRuntimeEntry->mScanPageList);

    sScanPageList->mHeadPageID = SM_NULL_PID;
    sScanPageList->mTailPageID = SM_NULL_PID;

    idlOS::snprintf( sBuffer, 128,
                     "SCAN_PAGE_LIST_MUTEX_%"
                     ID_XINT64_FMT"",
                     (ULong)aTableOID );

    /* smpFixedPageList_initEntryAtRuntime_malloc_Mutex.tc */
    IDU_FIT_POINT("smpFixedPageList::initEntryAtRuntime::malloc::Mutex");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMP,
                                 sizeof(iduMutex),
                                 (void **)&(sScanPageList->mMutex),
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );

    IDE_TEST( sScanPageList->mMutex->initialize( sBuffer,
                                                 IDU_MUTEX_KIND_NATIVE,
                                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * memory table의  fixed page list entry가 포함하는 runtime 정보 해제
 *
 * aFixedEntry : 해제하려는 PageListEntry
 ***********************************************************************/
IDE_RC smpFixedPageList::finEntryAtRuntime( smpPageListEntry* aFixedEntry )
{
    UInt                   sPageListID;
    smpScanPageListEntry * sScanPageList;

    IDE_DASSERT( aFixedEntry != NULL );

    if ( aFixedEntry->mRuntimeEntry != NULL )
    {
        sScanPageList = &(aFixedEntry->mRuntimeEntry->mScanPageList);

        IDE_TEST(sScanPageList->mMutex->destroy() != IDE_SUCCESS);
        IDE_TEST(iduMemMgr::free(sScanPageList->mMutex) != IDE_SUCCESS);

        sScanPageList->mMutex = NULL;

        for(sPageListID = 0;
            sPageListID < SMP_PAGE_LIST_COUNT;
            sPageListID++)
        {
            // AllocPageList의 Mutex 해제
            smpAllocPageList::finEntryAtRuntime(
                &(aFixedEntry->mRuntimeEntry->mAllocPageList[sPageListID]) );
        }

        // RuntimeEntry 제거
        IDE_TEST(smpFreePageList::finEntryAtRuntime(aFixedEntry)
                 != IDE_SUCCESS);

        // smpFreePageList::finEntryAtRuntime에서 RuntimeEntry를 NULL로 세팅
        IDE_ASSERT( aFixedEntry->mRuntimeEntry == NULL );
    }
    else
    {
        // Do Nothing
        // OFFLINE/DISCARD된 Table의 경우 mRuntimeEntry가 NULL일 수도 있다.
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PageListEntry를 완전히 제거하고 DB로 반납한다.
 *
 * aTrans      : 작업을 수행하는 트랜잭션 객체
 * aTableOID   : 제거할 테이블 OID
 * aFixedEntry : 제거할 PageListEntry
 * aDropFlag   : 제거하기 위한 OP FLAG
 ***********************************************************************/
IDE_RC smpFixedPageList::freePageListToDB( void*             aTrans,
                                           scSpaceID         aSpaceID,
                                           smOID             aTableOID,
                                           smpPageListEntry* aFixedEntry )
{
    UInt sPageListID;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );
    IDE_DASSERT( aTableOID == aFixedEntry->mTableOID );

    // [0] FreePageList 제거

    smpFreePageList::initializeFreePageListAndPool(aFixedEntry);

    /* ----------------------------
     * [1] fixed page list를
     *     smmManager에 반환한다.
     * ---------------------------*/

    for(sPageListID = 0;
        sPageListID < SMP_PAGE_LIST_COUNT;
        sPageListID++)
    {
        // for AllocPageList
        IDE_TEST( smpAllocPageList::freePageListToDB(
                      aTrans,
                      aSpaceID,
                      &(aFixedEntry->mRuntimeEntry->mAllocPageList[sPageListID]),
                      aTableOID )
                  != IDE_SUCCESS );
    }

    //BUG-25505
    //page list entry의 allocPageList까지 모두 DB로 반환했으면, ScanList또한 초기화 되어야 한다.
    aFixedEntry->mRuntimeEntry->mScanPageList.mHeadPageID = SM_NULL_PID;
    aFixedEntry->mRuntimeEntry->mScanPageList.mTailPageID = SM_NULL_PID;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Page를 초기화 한다.
 * Page내의 모든 Slot들도 초기화하며 Next 링크를 구성한다.
 *
 * aSlotSize   : Page에 들어가는 Slot의 크기
 * aSlotCount  : Page내의 모든 Slot 갯수
 * aPageListID : Page가 속할 PageListID
 * aPage       : 초기화할 Page
 ***********************************************************************/
void smpFixedPageList::initializePage( vULong       aSlotSize,
                                       vULong       aSlotCount,
                                       UInt         aPageListID,
                                       smOID        aTableOID,
                                       smpPersPage* aPage )
{
    UInt sSlotCounter;
    UShort sCurOffset;
    smpFreeSlotHeader* sCurFreeSlotHeader = NULL;
    smpFreeSlotHeader* sNextFreeSlotHeader;

    IDE_DASSERT( aSlotSize > 0 );
    IDE_DASSERT( aSlotCount > 0 );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_DASSERT( aPage != NULL );

    aPage->mHeader.mType        = SMP_PAGETYPE_FIX;
    aPage->mHeader.mAllocListID = aPageListID;
    aPage->mHeader.mTableOID    = aTableOID;

    sCurOffset = (UShort)SMP_PERS_PAGE_BODY_OFFSET;
    // BUG-32091 MemPage Body는 항상 8Byte align 된 상태여야 한다.
    IDE_DASSERT( idlOS::align8( sCurOffset ) == sCurOffset );
    sNextFreeSlotHeader = (smpFreeSlotHeader*)((SChar*)aPage + sCurOffset);

    for(sSlotCounter = 0; sSlotCounter < aSlotCount; sSlotCounter++)
    {
        sCurFreeSlotHeader = sNextFreeSlotHeader;

        SM_SET_SCN_FREE_ROW( &(sCurFreeSlotHeader->mCreateSCN) );
        SM_SET_SCN_FREE_ROW( &(sCurFreeSlotHeader->mLimitSCN) );
        SMP_SLOT_SET_OFFSET( sCurFreeSlotHeader, sCurOffset );

        sNextFreeSlotHeader = (smpFreeSlotHeader*)((SChar*)sCurFreeSlotHeader + aSlotSize);
        sCurFreeSlotHeader->mNextFreeSlot = sNextFreeSlotHeader;

        sCurOffset += aSlotSize;
    }

    // fix BUG-26934 : [codeSonar] Null Pointer Dereference
    IDE_ASSERT( sCurFreeSlotHeader != NULL );

    sCurFreeSlotHeader->mNextFreeSlot = NULL;

    IDL_MEM_BARRIER;


    return;
}
/***********************************************************************
 * DB에서 Page들을 할당받는다.
 *
 * fixed slot을 위한 persistent page를 system으로부터 할당받는다.
 *
 * aTrans      : 작업하는 트랜잭션 객체
 * aFixedEntry : 할당받을 PageListEntry
 ***********************************************************************/
IDE_RC smpFixedPageList::allocPersPages( void*             aTrans,
                                         scSpaceID         aSpaceID,
                                         smpPageListEntry* aFixedEntry )
{
    UInt                     sState = 0;
    UInt                     sPageListID;
    smLSN                    sNTA;
    smOID                    sTableOID;
    smpPersPage*             sPagePtr = NULL;
    smpPersPage*             sAllocPageHead;
    smpPersPage*             sAllocPageTail;
    scPageID                 sPrevPageID = SM_NULL_PID;
    scPageID                 sNextPageID = SM_NULL_PID;
#ifdef DEBUG
    smpFreePagePoolEntry*    sFreePagePool;
    smpFreePageListEntry*    sFreePageList;
#endif
    smpAllocPageListEntry*   sAllocPageList;
    smpPrivatePageListEntry* sPrivatePageList = NULL;
    smmPCH                 * sPCH;
    UInt                     sAllocPageCnt = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );

    sTableOID      = aFixedEntry->mTableOID;

    smLayerCallback::allocRSGroupID( aTrans, &sPageListID );

    sAllocPageList = &(aFixedEntry->mRuntimeEntry->mAllocPageList[sPageListID]);
#ifdef DEBUG
    sFreePagePool  = &(aFixedEntry->mRuntimeEntry->mFreePagePool);
    sFreePageList  = &(aFixedEntry->mRuntimeEntry->mFreePageList[sPageListID]);
#endif

    IDE_DASSERT( sAllocPageList != NULL );
    IDE_DASSERT( sFreePagePool != NULL );
    IDE_DASSERT( sFreePageList != NULL );

    // DB에서 Page들을 할당받으면 FreePageList를
    // Tx's Private Page List에 먼저 등록한 후
    // 트랜잭션이 종료될 때 해당 테이블의 PageListEntry에 등록하게 된다.

    IDE_TEST( smLayerCallback::findPrivatePageList( aTrans,
                                                    aFixedEntry->mTableOID,
                                                    &sPrivatePageList )
              != IDE_SUCCESS );

    if(sPrivatePageList == NULL)
    {
        // 기존에 PrivatePageList가 없었다면 새로 생성한다.
        IDE_TEST( smLayerCallback::createPrivatePageList(
                      aTrans,
                      aFixedEntry->mTableOID,
                      &sPrivatePageList )
                  != IDE_SUCCESS );
    }

    // Begin NTA
    sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );
    sState = 1;

    // DB에서 받아오고
    IDE_TEST( smmManager::allocatePersPageList( aTrans,
                                                aSpaceID,
                                                SMP_ALLOCPAGECOUNT_FROMDB,
                                                (void **)&sAllocPageHead,
                                                (void **)&sAllocPageTail,
                                                &sAllocPageCnt )
              != IDE_SUCCESS);

    IDE_DASSERT( smpAllocPageList::isValidPageList(
                     aSpaceID,
                     sAllocPageHead->mHeader.mSelfPageID,
                     sAllocPageTail->mHeader.mSelfPageID,
                     sAllocPageCnt )
                 == ID_TRUE );

    // 할당받은 HeadPage를 PrivatePageList에 등록한다.
    if( sPrivatePageList->mFixedFreePageTail == NULL )
    {
        IDE_DASSERT( sPrivatePageList->mFixedFreePageHead == NULL );

        sPrivatePageList->mFixedFreePageHead =
            smpFreePageList::getFreePageHeader(
                aSpaceID,
                sAllocPageHead->mHeader.mSelfPageID);
    }
    else
    {
        IDE_DASSERT( sPrivatePageList->mFixedFreePageHead != NULL );

        sPrivatePageList->mFixedFreePageTail->mFreeNext =
            smpFreePageList::getFreePageHeader(
                aSpaceID,
                sAllocPageHead->mHeader.mSelfPageID);

        sPrevPageID = sPrivatePageList->mFixedFreePageTail->mSelfPageID;
    }

    // 할당받은 페이지들을 초기화 한다.
    sNextPageID = sAllocPageHead->mHeader.mSelfPageID;

    while(sNextPageID != SM_NULL_PID)
    {
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                sNextPageID,
                                                (void**)&sPagePtr )
                    == IDE_SUCCESS );
        sState = 3;

        IDE_TEST( smrUpdate::initFixedPage(NULL, /* idvSQL* */
                                           aTrans,
                                           aSpaceID,
                                           sPagePtr->mHeader.mSelfPageID,
                                           sPageListID,
                                           aFixedEntry->mSlotSize,
                                           aFixedEntry->mSlotCount,
                                           aFixedEntry->mTableOID)
                  != IDE_SUCCESS);


        // PersPageHeader 초기화하고 (FreeSlot들을 연결한다.)
        initializePage( aFixedEntry->mSlotSize,
                        aFixedEntry->mSlotCount,
                        sPageListID,
                        aFixedEntry->mTableOID,
                        sPagePtr );

        sPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                             sPagePtr->mHeader.mSelfPageID ));
        sPCH->mNxtScanPID       = SM_NULL_PID;
        sPCH->mPrvScanPID       = SM_NULL_PID;
        sPCH->mModifySeqForScan = 0;

        sState = 2;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                sPagePtr->mHeader.mSelfPageID)
                  != IDE_SUCCESS );

        // FreePageHeader 초기화하고
        smpFreePageList::initializeFreePageHeader(
            smpFreePageList::getFreePageHeader(aSpaceID, sPagePtr) );

        // FreeSlotList를 Page에 등록한다.
        smpFreePageList::initializeFreeSlotListAtPage( aSpaceID,
                                                       aFixedEntry,
                                                       sPagePtr );

        sNextPageID = sPagePtr->mHeader.mNextPageID;

        // FreePageHeader를 PrivatePageList 링크를 구성한다.
        smpFreePageList::addFreePageToPrivatePageList( aSpaceID,
                                                       sPagePtr->mHeader.mSelfPageID,
                                                       sPrevPageID,
                                                       sNextPageID );

        sPrevPageID = sPagePtr->mHeader.mSelfPageID;
    }

    IDE_DASSERT( sPagePtr == sAllocPageTail );

    // TailPage를 PrivatePageList에 등록한다.
    sPrivatePageList->mFixedFreePageTail =
        smpFreePageList::getFreePageHeader(aSpaceID,
                                           sAllocPageTail->mHeader.mSelfPageID);

    // 전체를 AllocPageList 등록
    IDE_TEST(sAllocPageList->mMutex->lock(NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 2;

    IDE_TEST( smpAllocPageList::addPageList( aTrans,
                aSpaceID,
                sAllocPageList,
                sTableOID,
                sAllocPageHead,
                sAllocPageTail,
                sAllocPageCnt )
            != IDE_SUCCESS);

    // End NTA[-1-]
    IDE_TEST(smrLogMgr::writeNTALogRec( NULL, /* idvSQL* */
                                        aTrans,
                                        &sNTA,
                                        SMR_OP_NULL )
             != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(sAllocPageList->mMutex->unlock() != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sState)
    {
        case 3:
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(aSpaceID,
                                                      sPagePtr->mHeader.mSelfPageID)
                        == IDE_SUCCESS );

        case 2:
            // DB에서 TAB으로 Page들을 가져왔는데 미처 TAB에 달지 못했다면 롤백
            IDE_ASSERT( smrRecoveryMgr::undoTrans( NULL, /* idvSQL* */
                                                   aTrans,
                                                   &sNTA )
                        == IDE_SUCCESS );
            IDE_ASSERT( sAllocPageList->mMutex->unlock() == IDE_SUCCESS );
            break;

        case 1:
            // DB에서 TAB으로 Page들을 가져왔는데 미처 TAB에 달지 못했다면 롤백
            IDE_ASSERT( smrRecoveryMgr::undoTrans( NULL, /* idvSQL* */
                                                   aTrans,
                                                   &sNTA)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    IDE_POP();


    return IDE_FAILURE;
}

/***********************************************************************
 * temporary table header를 위한 fixed slot 할당
 *
 * temporary table header를 저장할경우, slot 할당에 대한 로깅은 하지 않도록
 * 처리하고, 단, system으로부터 persistent page를 할당하는 연산에 대해서는
 * 로깅을 하도록 한다.
 *
 * aTableOID   : 할당하려는 테이블의 OID
 * aFixedEntry : 할당하려는 PageListEntry
 * aRow        : 할당해서 반환하려는 Row 포인터
 * aInfinite   : SCN Infinite 값
 ***********************************************************************/
IDE_RC smpFixedPageList::allocSlotForTempTableHdr( scSpaceID         aSpaceID,
                                                   smOID             aTableOID,
                                                   smpPageListEntry* aFixedEntry,
                                                   SChar**           aRow,
                                                   smSCN             aInfinite )
{
    SInt                sState = 0;
    UInt                sPageListID;
    smSCN               sInfiniteSCN;
    smOID               sRecOID;
    scPageID            sPageID;
    smpFreeSlotHeader * sCurFreeSlotHeader;
    smpSlotHeader     * sCurSlotHeader;
    void*               sDummyTx;

    IDE_DASSERT(aFixedEntry != NULL);

    sPageID  = SM_NULL_PID;

    IDE_ASSERT(aRow != NULL);
    IDE_ASSERT(aTableOID == aFixedEntry->mTableOID);

    // BUG-8083
    // temp table 관련 statement는 untouchable 속성이므로
    // 로깅을 하지 못한다. 그러므로 새로운 tx를 할당하여
    // PageListID를 선택하고,
    // system으로부터 page를 할당받을때 로깅을 처리하도록 한다.
    IDE_TEST( smLayerCallback::allocTx( &sDummyTx ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smLayerCallback::beginTx( sDummyTx,
                                        SMI_TRANSACTION_REPL_DEFAULT, // Replicate
                                        NULL )     // SessionFlagPtr
              != IDE_SUCCESS );
    sState = 2;

    smLayerCallback::allocRSGroupID( sDummyTx, &sPageListID );

    while(1)
    {
        // 1) Tx's PrivatePageList에서 찾기
        IDE_TEST( tryForAllocSlotFromPrivatePageList( sDummyTx,
                                                      aSpaceID,
                                                      aTableOID,
                                                      aFixedEntry,
                                                      aRow )
                  != IDE_SUCCESS );

        if(*aRow != NULL)
        {
            break;
        }

        // 2) FreePageList에서 찾기
        IDE_TEST( tryForAllocSlotFromFreePageList( sDummyTx,
                                                   aSpaceID,
                                                   aFixedEntry,
                                                   sPageListID,
                                                   aRow )
                  != IDE_SUCCESS );

        if( *aRow != NULL)
        {
            break;
        }

        // 3) system으로부터 page를 할당받는다.
        IDE_TEST( allocPersPages( sDummyTx,
                                  aSpaceID,
                                  aFixedEntry )
                  != IDE_SUCCESS );
    }

    IDU_FIT_POINT( "1.BUG-48210@smpFixedPageList::allocSlotForTempTableHdr" );
    IDE_TEST( smLayerCallback::commitTx( sDummyTx ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smLayerCallback::freeTx( sDummyTx ) != IDE_SUCCESS );
    sState = 0;

    IDE_ASSERT( *aRow != NULL );

    /* ------------------------------------------------
     * 할당된 slot header를 초기화한다. temporary table
     * header를 저장하기 위한 slot 할당시에는
     * 로깅을 하지 않도록 처리한다. initFixedRow와
     * NTA에 대하여 로깅하지 않음
     * ----------------------------------------------*/
    sCurFreeSlotHeader = (smpFreeSlotHeader*)(*aRow);
    sPageID = SMP_SLOT_GET_PID((SChar *)sCurFreeSlotHeader);
    sRecOID = SM_MAKE_OID( sPageID,
                           SMP_SLOT_GET_OFFSET( sCurFreeSlotHeader ) );

    if( SMP_SLOT_IS_USED( sCurFreeSlotHeader ) )
    {
        sCurSlotHeader = (smpSlotHeader*)sCurFreeSlotHeader;

        ideLog::log(SM_TRC_LOG_LEVEL_MPAGE, SM_TRC_MPAGE_ALLOC_SLOT_FATAL1);
        ideLog::log(SM_TRC_LOG_LEVEL_MPAGE, SM_TRC_MPAGE_ALLOC_SLOT_FATAL2);
        ideLog::log(SM_TRC_LOG_LEVEL_MPAGE, SM_TRC_MPAGE_ALLOC_SLOT_FATAL3, (ULong)sPageID, (ULong)sRecOID);

        dumpSlotHeader( sCurSlotHeader );

        IDE_ASSERT(0);
    }

    sState = 3;

    // Init header of fixed row
    sInfiniteSCN = aInfinite;

    SM_SET_SCN_DELETE_BIT( &sInfiniteSCN );
    SM_SET_SCN( &(sCurFreeSlotHeader->mCreateSCN) , &sInfiniteSCN );

    SM_SET_SCN_FREE_ROW( &(sCurFreeSlotHeader->mLimitSCN) );

    SMP_SLOT_INIT_POSITION( sCurFreeSlotHeader );
    SMP_SLOT_SET_USED( sCurFreeSlotHeader );

    sState = 0;
    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID)
             != IDE_SUCCESS);

    /* BUG-47368: 테이블 헤더이기 때문에 MAX ROW 체크하지 않고
     *            mutex 제거, atomic 연산으로 변경
     */
    (void)idCore::acpAtomicInc64( &(aFixedEntry->mRuntimeEntry->mInsRecCnt) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch (sState)
    {
        case 3:
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID)
                        == IDE_SUCCESS );
            break;

        case 2: /* BUG-48210 commit 실패에 대한 예외 처리 보강 */
            IDE_ASSERT( smLayerCallback::abortTx( sDummyTx )
                        == IDE_SUCCESS );

        case 1:
            IDE_ASSERT( smLayerCallback::freeTx( sDummyTx )
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    IDE_POP();


    return IDE_FAILURE;

}

/***********************************************************************
 * fixed slot을 할당한다.
 *
 * aTrans          : 작업하려는 트랜잭션 객체
 * aTableOID       : 할당하려는 테이블의 OID
 * aFixedEntry     : 할당하려는 PageListEntry
 * aRow            : 할당해서 반환하려는 Row 포인터
 * aInfinite       : SCN Infinite
 * aMaxRow         : 최대 Row 갯수
 * aOptFlag        :
 *           1. SMP_ALLOC_FIXEDSLOT_NONE
 *              어떤작업도 수행하지 않는다.
 *           2. SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT
 *              Allocate를 요청하는 Table 의 Record Count를 증가시킨다.
 *           3. SMP_ALLOC_FIXEDSLOT_SET_SLOTHEADER
 *              할당받은 Slot의 Header를 Update하고 Logging하라.
 *
 * aOPType         : 할당하려는 작업 내용
 ***********************************************************************/
IDE_RC smpFixedPageList::allocSlot( void*             aTrans,
                                    scSpaceID         aSpaceID,
                                    void*             aTableInfoPtr,
                                    smOID             aTableOID,
                                    smpPageListEntry* aFixedEntry,
                                    SChar**           aRow,
                                    smSCN             aInfinite,
                                    ULong             aMaxRow,
                                    SInt              aOptFlag,
                                    smrOPType         aOPType)
{
    SInt                sState = 0;
    UInt                sPageListID;
    smLSN               sNTA;
    smOID               sRecOID;
    ULong               sRecordCnt;
    scPageID            sPageID;
    smpFreeSlotHeader * sCurFreeSlotHeader;
    smpSlotHeader     * sCurSlotHeader;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );

    IDE_ASSERT(( aOPType == SMR_OP_SMC_FIXED_SLOT_ALLOC ) ||
               ( aOPType == SMR_OP_SMC_TABLEHEADER_ALLOC ));

    IDE_ASSERT( aRow != NULL );
    IDE_ASSERT( aTableOID == aFixedEntry->mTableOID );

    sPageID     = SM_NULL_PID;
    smLayerCallback::allocRSGroupID( aTrans, &sPageListID );

    /* BUG-19573 Table의 Max Row가 Disable되어있으면 이 값을 insert시에
     *           Check하지 않아야 한다. */
    if(( aTableInfoPtr != NULL ) && ( aMaxRow != ID_ULONG_MAX ))
    {
        // BUG-47368: allocSlot, mutex 제거, atomic 연산으로 변경
        sRecordCnt = idCore::acpAtomicGet64( &(aFixedEntry->mRuntimeEntry->mInsRecCnt) );
        sRecordCnt += smLayerCallback::getRecCntOfTableInfo( aTableInfoPtr ); 

        IDE_TEST_RAISE((aMaxRow <= sRecordCnt) &&
                       ((aOptFlag & SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT)
                        == SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT),
                       err_exceed_maxrow);
    }

    /* need to alloc page from smmManager */
    while(1)
    {
        // 1) Tx's PrivatePageList에서 찾기
        IDE_TEST( tryForAllocSlotFromPrivatePageList( aTrans,
                                                      aSpaceID,
                                                      aTableOID,
                                                      aFixedEntry,
                                                      aRow )
                  != IDE_SUCCESS );

        if(*aRow != NULL)
        {
            break;
        }

        // 2) FreePageList에서 찾기
        IDE_TEST( tryForAllocSlotFromFreePageList( aTrans,
                                                   aSpaceID,
                                                   aFixedEntry,
                                                   sPageListID,
                                                   aRow )
                  != IDE_SUCCESS );

        if(*aRow != NULL)
        {
            break;
        }
        // 3) system으로부터 page를 할당받는다.
        IDE_TEST( allocPersPages( aTrans,
                                  aSpaceID,
                                  aFixedEntry )
                  != IDE_SUCCESS );
    }

    if( (aOptFlag & SMP_ALLOC_FIXEDSLOT_SET_SLOTHEADER)
        == SMP_ALLOC_FIXEDSLOT_SET_SLOTHEADER)
    {
        IDE_ASSERT( *aRow != NULL );

        // Begin NTA[-2-]
        sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

        sCurFreeSlotHeader = (smpFreeSlotHeader*)(*aRow);
        sPageID = SMP_SLOT_GET_PID((SChar *)sCurFreeSlotHeader);
        sRecOID = SM_MAKE_OID( sPageID,
                               SMP_SLOT_GET_OFFSET( sCurFreeSlotHeader ) );

        sState = 1;
        if( SMP_SLOT_IS_USED(sCurFreeSlotHeader) )
        {
            sCurSlotHeader = (smpSlotHeader*)sCurFreeSlotHeader;

            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE, SM_TRC_MPAGE_ALLOC_SLOT_FATAL1);
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE, SM_TRC_MPAGE_ALLOC_SLOT_FATAL2);
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE, SM_TRC_MPAGE_ALLOC_SLOT_FATAL3, (ULong)sPageID, (ULong)sRecOID);

            dumpSlotHeader( sCurSlotHeader );

            IDE_ASSERT(0);
        }

        // End NTA[-2-]
        IDE_TEST(smrLogMgr::writeNTALogRec( NULL, /* idvSQL* */
                                            aTrans,
                                            &sNTA,
                                            aSpaceID,
                                            aOPType,
                                            sRecOID,
                                            aTableOID )
                 != IDE_SUCCESS);


        setAllocatedSlot( aInfinite, *aRow );

        /* BUG-43291: mVarOID는 항상 초기화 되어야 한다. */
        sCurSlotHeader = (smpSlotHeader*)*aRow;
        sCurSlotHeader->mVarOID = SM_NULL_OID;

        sState = 0;
        IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID)
                 != IDE_SUCCESS);

    }
    else
    {
        /* BUG-14513:
           DML(insert, update, delete)시 Alloc Slot할때 여기서 header를 update하지
           않고 insert, update, delete log를 기록후에 header update를 수행한다.
           그런데 여기서 slot header의 tid를 setting하는 이유는 Update, Delete시
           다른 Transaction이 이미 update를 수행한 transaction을 기다릴때 next version
           의 tid로 기다릴 transaction을 결정하기 때문에 tid를 setting해야한다.
           그리고 tid에 대한 logging을 하지 않는 이유는 free slot의 tid가 어떤값이 되더라도
           문제가 안되기 때문이다.

           BUG-14953 :
           1. SCN에 infinite값을 세팅하여 다른 tx가 기다리도록 한다.
           2. alloc만 하고 DML 로그를 쓰기 전에 죽은 경우에 restart시
              refine이 되게 하기 위해 SCN Delete bit를 세팅한다.
              DML로깅시에 다시 delete bit가 clear된다.
        */
        sCurSlotHeader = (smpSlotHeader*)*aRow;

        SM_SET_SCN( &(sCurSlotHeader->mCreateSCN), &aInfinite );

        /* smpFixedPageList::setFreeSlot에서 SCN이 Delete Bit가
           Setting되어 있는지 Check함.*/
        SM_SET_SCN_DELETE_BIT( &(sCurSlotHeader->mCreateSCN) );

        sCurSlotHeader->mVarOID = SM_NULL_OID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_exceed_maxrow);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_ExceedMaxRows));
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sState)
    {
        case 1:
            IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(aSpaceID, sPageID)
                       == IDE_SUCCESS);
            break;

        default:
            break;
    }

    IDE_POP();


    return IDE_FAILURE;
}

/***********************************************************************
 * slot을 free 한다.
 *
 * BUG-14093 Ager Tx가 freeSlot한 것을 commit되지 않은 상황에서
 *           다른 Tx가 할당받아 사용했을때 서버 사망시 문제발생
 *           따라서 Ager Tx가 Commit이후에 FreeSlot을 FreeSlotList에 매단다.
 *
 * aTrans      : 작업을 수행하는 트랜잭션 객체
 * aFixedEntry : aRow가 속한 PageListEntry
 * aRow        : free하려는 slot
 * aTableType  : Temp Table의 slot인지에 대한 여부
 ***********************************************************************/
IDE_RC smpFixedPageList::freeSlot( void*             aTrans,
                                   scSpaceID         aSpaceID,
                                   smpPageListEntry* aFixedEntry,
                                   SChar*            aRow,
                                   smpTableType      aTableType,
                                   smSCN             aSCN )
{
    scPageID sPageID;
    smOID    sRowOID;

    IDE_DASSERT( ((aTrans != NULL) && (aTableType == SMP_TABLE_NORMAL)) ||
                 ((aTrans == NULL) && (aTableType == SMP_TABLE_TEMP)) );

    IDE_DASSERT( aFixedEntry != NULL );
    IDE_DASSERT( aRow != NULL );

    /* ----------------------------
     * BUG-14093
     * freeSlot에서는 slot에 대한 Free작업만 수행하고
     * ager Tx가 commit한 이후에 addFreeSlotPending을 수행한다.
     * ---------------------------*/

    (aFixedEntry->mRuntimeEntry->mDelRecCnt)++;

    sPageID = SMP_SLOT_GET_PID(aRow);

    IDE_TEST(setFreeSlot( aTrans,
                          aSpaceID,
                          sPageID,
                          aRow,
                          aTableType )
             != IDE_SUCCESS);

    if(aTableType == SMP_TABLE_NORMAL)
    {
        sRowOID = SM_MAKE_OID( sPageID,
                               SMP_SLOT_GET_OFFSET( (smpSlotHeader*)aRow ) );

        // BUG-14093 freeSlot하는 ager가 commit하기 전에는
        //           freeSlotList에 매달지 않고 ager TX가
        //           commit 이후에 매달도록 OIDList에 추가한다.
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aFixedEntry->mTableOID,
                                           sRowOID,
                                           aSpaceID,
                                           SM_OID_TYPE_FREE_FIXED_SLOT,
                                           aSCN )
                  != IDE_SUCCESS );
    }
    else
    {
        // TEMP Table은 바로 FreeSlotList에 추가한다.
        IDE_TEST( addFreeSlotPending(aTrans,
                                     aSpaceID,
                                     aFixedEntry,
                                     aRow)
                 != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * nextOIDall을 위해 aRow에서 해당 Page를 찾아준다.
 *
 * aFixedEntry : 순회하려는 PageListEntry
 * aRow        : 현재 Row
 * aPage       : aRow가 속한 Page를 찾아서 반환
 * aRowPtr     : aRow 다음 Row 포인터
 ***********************************************************************/

IDE_RC smpFixedPageList::initForScan( scSpaceID         aSpaceID,
                                      smpPageListEntry* aFixedEntry,
                                      SChar*            aRow,
                                      smpPersPage**     aPage,
                                      SChar**           aRowPtr )
{
    scPageID sPageID;

    IDE_DASSERT( aFixedEntry != NULL );
    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( aRowPtr != NULL );

    *aPage   = NULL;
    *aRowPtr = NULL;

    if(aRow != NULL)
    {
        sPageID = SMP_SLOT_GET_PID(aRow);
        IDE_ERROR_MSG( smmManager::getPersPagePtr( aSpaceID, 
                                                   sPageID,
                                                   (void**)aPage )
                       == IDE_SUCCESS,
                       "TableOID : %"ID_UINT64_FMT"\n"
                       "SpaceID  : %"ID_UINT32_FMT"\n"
                       "PageID   : %"ID_UINT32_FMT"\n",
                       aFixedEntry->mTableOID,
                       aSpaceID,
                       sPageID );
        *aRowPtr = aRow + aFixedEntry->mSlotSize;
    }
    else
    {
        sPageID = smpManager::getFirstAllocPageID(aFixedEntry);

        if(sPageID != SM_NULL_PID)
        {
            IDE_ERROR_MSG( smmManager::getPersPagePtr( aSpaceID, 
                                                       sPageID,
                                                       (void**)aPage )
                           == IDE_SUCCESS,
                           "TableOID : %"ID_UINT64_FMT"\n"
                           "SpaceID  : %"ID_UINT32_FMT"\n"
                           "PageID   : %"ID_UINT32_FMT"\n",
                           aFixedEntry->mTableOID,
                           aSpaceID,
                           sPageID );
            *aRowPtr = (SChar *)((*aPage)->mBody);
        }
        else
        {
            /* Allcate된 페이지가 존재하지 않는다.*/
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * PageList내의 모든 Row를 순회하면서 유효한 Row를 찾아준다.
 *
 * aSpaceID      [IN]  Tablespace ID
 * aFixedEntry   [IN]  순회하려는 PageListEntry
 * aCurRow       [IN]  찾기시작하려는 Row
 * aNxtRow       [OUT] 다음 유효한 Row를 찾아서 반환
 * aNxtPID       [OUT] 다음 Page를 반환함.
 **********************************************************************/
IDE_RC smpFixedPageList::nextOIDallForRefineDB( scSpaceID           aSpaceID,
                                                smpPageListEntry  * aFixedEntry,
                                                SChar             * aCurRow,
                                                SChar            ** aNxtRow,
                                                scPageID          * aNxtPID )
{
    scPageID            sNxtPID;
    smpPersPage       * sPage;
    smpFreePageHeader * sFreePageHeader;
    SChar             * sNxtPtr;
    SChar             * sFence;

    IDE_ERROR( aFixedEntry != NULL );
    IDE_ERROR( aNxtRow != NULL );

    IDE_TEST( initForScan( aSpaceID, aFixedEntry, aCurRow, &sPage, &sNxtPtr )
              != IDE_SUCCESS );

    *aNxtRow = NULL;

    while(sPage != NULL)
    {
        // 해당 Page에 대한 포인터 경계
        sFence          = (SChar *)((smpPersPage *)sPage + 1);
        sFreePageHeader = smpFreePageList::getFreePageHeader(aSpaceID,sPage);

        for( ;
             sNxtPtr + aFixedEntry->mSlotSize <= sFence;
             sNxtPtr += aFixedEntry->mSlotSize )
        {
            // In case of free slot
            if( SMP_SLOT_IS_NOT_USED((smpSlotHeader*)sNxtPtr) )
            {
                // refineDB때 FreeSlot은 FreeSlotList에 등록한다.
                addFreeSlotToFreeSlotList(sFreePageHeader,
                                          sNxtPtr);

                continue;
            }

            *aNxtRow = sNxtPtr;
            *aNxtPID = sPage->mHeader.mSelfPageID;

            IDE_CONT(normal_case);
        } /* for */

        // refineDB때 하나의 Page Scan을 마치면 FreePageList에 등록한다.
        if( sFreePageHeader->mFreeSlotCount > 0 )
        {
            // FreeSlot이 있어야 FreePage이고 FreePageList에 등록된다.
            IDE_TEST( smpFreePageList::addPageToFreePageListAtInit(
                          aFixedEntry,
                          smpFreePageList::getFreePageHeader(aSpaceID, sPage))
                      != IDE_SUCCESS );
        }

        sNxtPID = smpManager::getNextAllocPageID( aSpaceID,
                                                  aFixedEntry,
                                                  sPage->mHeader.mSelfPageID );

        if(sNxtPID == SM_NULL_PID)
        {
            // NextPage가 NULL이면 끝이다.
            IDE_CONT(normal_case);
        }

        IDE_ERROR_MSG( smmManager::getPersPagePtr( aSpaceID, 
                                                   sNxtPID,
                                                   (void**)&sPage )
                       == IDE_SUCCESS,
                       "TableOID : %"ID_UINT64_FMT"\n"
                       "SpaceID  : %"ID_UINT32_FMT"\n"
                       "PageID   : %"ID_UINT32_FMT"\n",
                       aFixedEntry->mTableOID,
                       aSpaceID,
                       sNxtPID  );

        sNxtPtr  = (SChar *)sPage->mBody;
    }

    IDE_EXCEPTION_CONT( normal_case );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**********************************************************************
 * FreePageHeader에서 FreeSlot제거
 *
 * aFreePageHeader : 제거하려는 FreePageHeader
 * aRow            : 제거한 FreeSlot의 Row포인터 반환
 **********************************************************************/

void smpFixedPageList::removeSlotFromFreeSlotList(
                                        smpFreePageHeader* aFreePageHeader,
                                        SChar**            aRow )
{
    smpFreeSlotHeader* sFreeSlotHeader;

    IDE_DASSERT(aRow != NULL);
    IDE_DASSERT(aFreePageHeader != NULL);
    IDE_DASSERT(aFreePageHeader->mFreeSlotCount > 0);

    IDE_DASSERT( isValidFreeSlotList(aFreePageHeader) == ID_TRUE );

    sFreeSlotHeader = (smpFreeSlotHeader*)(aFreePageHeader->mFreeSlotHead);

    *aRow = (SChar*)sFreeSlotHeader;

    aFreePageHeader->mFreeSlotCount--;



    if(sFreeSlotHeader->mNextFreeSlot == NULL)
    {
        // Next가 없다면 마지막 FreeSlot이다.
        IDE_ASSERT(aFreePageHeader->mFreeSlotCount == 0);

        aFreePageHeader->mFreeSlotTail = NULL;
    }
    else
    {
        // 다음 FreeSlot을 Head로 등록한다.
        IDE_ASSERT(aFreePageHeader->mFreeSlotCount > 0);
    }

    aFreePageHeader->mFreeSlotHead =
                            (SChar*)sFreeSlotHeader->mNextFreeSlot;

    IDE_DASSERT( isValidFreeSlotList(aFreePageHeader) == ID_TRUE );


    return ;
}

/**********************************************************************
 * FreeSlot 정보를 기록한다.
 *
 * aTrans     : 작업하려는 트랜잭션 객체
 * aPageID    : FreeSlot추가하려는 PageID
 * aRow       : FreeSlot의 Row 포인터
 * aTableType : Temp테이블인지 여부
 **********************************************************************/

IDE_RC smpFixedPageList::setFreeSlot( void         * aTrans,
                                      scSpaceID      aSpaceID,
                                      scPageID       aPageID,
                                      SChar        * aRow,
                                      smpTableType   aTableType )
{
    smpSlotHeader       sAfterSlotHeader;
    SInt                sState = 0;
    smpSlotHeader     * sCurSlotHeader;
    smpSlotHeader     * sTmpSlotHeader;
    smOID               sRecOID;

    IDE_DASSERT( ((aTrans != NULL) && (aTableType == SMP_TABLE_NORMAL)) ||
                 ((aTrans == NULL) && (aTableType == SMP_TABLE_TEMP)) );

    IDE_ERROR( aPageID != SM_NULL_PID );
    IDE_ERROR( aRow != NULL );

    ACP_UNUSED( aTrans  );

    sCurSlotHeader = (smpSlotHeader*)aRow;

    if( SMP_SLOT_IS_USED(sCurSlotHeader) )
    {

        sState = 1;

        sRecOID = SM_MAKE_OID( aPageID,
                               SMP_SLOT_GET_OFFSET( sCurSlotHeader ) );

        idlOS::memcpy( &sAfterSlotHeader,
                       (SChar*)sCurSlotHeader,
                       ID_SIZEOF(smpSlotHeader) );

        // slot header 정리
        SM_SET_SCN_FREE_ROW( &(sAfterSlotHeader.mCreateSCN) );
        SM_SET_SCN_FREE_ROW( &(sAfterSlotHeader.mLimitSCN) );
        SMP_SLOT_INIT_POSITION( &sAfterSlotHeader );
        sAfterSlotHeader.mVarOID   = SM_NULL_OID;

        if(aTableType == SMP_TABLE_NORMAL)
        {

            IDE_ERROR( smmManager::getOIDPtr( aSpaceID, 
                                               sRecOID,
                                               (void**)&sTmpSlotHeader )
                       == IDE_SUCCESS );

            IDE_ERROR( SMP_SLOT_GET_OFFSET( sTmpSlotHeader )
                       == SMP_SLOT_GET_OFFSET( sCurSlotHeader ) );

            IDE_ERROR( SMP_SLOT_GET_OFFSET( sTmpSlotHeader )
                       == SM_MAKE_OFFSET(sRecOID));
        }

        // BUG-14373 ager와 seq-iterator와의 동시성 제어
        IDE_TEST(smmManager::holdPageXLatch(aSpaceID, aPageID) != IDE_SUCCESS);
        sState = 2;

        *sCurSlotHeader = sAfterSlotHeader;

        sState = 1;
        IDE_TEST(smmManager::releasePageLatch(aSpaceID, aPageID) != IDE_SUCCESS);

        sState = 0;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage( aSpaceID, aPageID)
                  != IDE_SUCCESS );

    }

    // BUG-49109: ERROR 발생 할 경우 SCN 출력
    IDE_ERROR_RAISE( SM_SCN_IS_DELETED( sCurSlotHeader->mCreateSCN ) &&
                     SM_SCN_IS_FREE_ROW( sCurSlotHeader->mLimitSCN ) &&
                     SMP_SLOT_HAS_NULL_NEXT_OID( sCurSlotHeader ), ERR_SLOT_SCN );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SLOT_SCN )
    {
        ideLog::log(IDE_ERR_0, "smpFixedPageList::setFreeSlot\n"
                               "CreateSCN     : %"ID_UINT64_FMT"\n"
                               "LimitSCN      : %"ID_UINT64_FMT"\n"
                               "NextOIDIsNULL : %u\n", 
                                sCurSlotHeader->mCreateSCN, 
                                sCurSlotHeader->mLimitSCN, 
                                SMP_SLOT_HAS_NULL_NEXT_OID( sCurSlotHeader ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sState)
    {
        case 2:
            IDE_ASSERT( smmManager::releasePageLatch(aSpaceID,aPageID)
                        == IDE_SUCCESS );

        case 1:
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(aSpaceID,aPageID)
                        == IDE_SUCCESS );

        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/**********************************************************************
 * BUG-25611
 * Undo시 사용한 PrivatePageList들은 이후 진행될 RefineDB를 위해
 * ScanList를 제거해야 한다.
 * 따라서 절대 restart시 Undo할때에만 불리어야한다.
 *
 * aSpaceID        : 대상 테이블의 테이블 스페이스 ID
 * aFixedEntry      : 대상 테이블의 FixedEntry
 * aPageListEntry   : 스캔리스트를 제거할 FreePage들의 Head
 **********************************************************************/
IDE_RC smpFixedPageList::resetScanList( scSpaceID          aSpaceID,
                                        smpPageListEntry  *aPageListEntry)
{
    scPageID sPageID;

    IDE_DASSERT( aPageListEntry != NULL );

    sPageID = smpManager::getFirstAllocPageID(aPageListEntry);

    while(sPageID != SM_NULL_PID)
    {
        IDE_TEST( unlinkScanList( aSpaceID,
                                  sPageID,
                                  aPageListEntry )
                  != IDE_SUCCESS );

        sPageID = smpManager::getNextAllocPageID( aSpaceID,
                                                  aPageListEntry,
                                                  sPageID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * 실제 FreeSlot을 FreeSlotList에 추가한다.
 *
 * BUG-14093 Commit이후에 FreeSlot을 실제 FreeSlotList에 매단다.
 *
 * aTrans      : 작업하는 트랜잭션 객체
 * aFixedEntry : FreeSlot이 속한 PageListEntry
 * aRow        : FreeSlot의 Row 포인터
 **********************************************************************/
IDE_RC smpFixedPageList::addFreeSlotPending( void*             aTrans,
                                             scSpaceID         aSpaceID,
                                             smpPageListEntry* aFixedEntry,
                                             SChar*            aRow )
{
    UInt               sState = 0;
    scPageID           sPageID;
    smpFreePageHeader* sFreePageHeader;

    IDE_DASSERT(aFixedEntry != NULL);
    IDE_DASSERT(aRow != NULL);

    sPageID = SMP_SLOT_GET_PID(aRow);
    sFreePageHeader = smpFreePageList::getFreePageHeader(aSpaceID, sPageID);

    IDE_TEST(sFreePageHeader->mMutex.lock( NULL /* idvSQL* */) != IDE_SUCCESS);
    sState = 1;

    // PrivatePageList에서는 FreeSlot되지 않는다.
    IDE_ASSERT(sFreePageHeader->mFreeListID != SMP_PRIVATE_PAGELISTID);

    // FreeSlot을 FreeSlotList에 추가
    addFreeSlotToFreeSlotList(sFreePageHeader, aRow);

    // FreeSlot이 추가된 다음 SizeClass가 변경되었는지 확인하여 조정한다.
    IDE_TEST(smpFreePageList::modifyPageSizeClass( aTrans,
                                                   aFixedEntry,
                                                   sFreePageHeader )
             != IDE_SUCCESS);

    if( sFreePageHeader->mFreeSlotCount == aFixedEntry->mSlotCount )
    {
        IDE_TEST( unlinkScanList( aSpaceID,
                                  sPageID,
                                  aFixedEntry )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST(sFreePageHeader->mMutex.unlock() != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sState == 1)
    {
        IDE_ASSERT(sFreePageHeader->mMutex.unlock() == IDE_SUCCESS);
    }

    IDE_POP();

    return IDE_FAILURE;
}

/**********************************************************************
 * FreePageHeader에 있는 FreeSlotList에 FreeSlot추가
 *
 * aFreePageHeader : FreeSlot이 속한 Page의 FreePageHeader
 * aRow            : FreeSlot의 Row 포인터
 **********************************************************************/
void smpFixedPageList::addFreeSlotToFreeSlotList(
                                    smpFreePageHeader* aFreePageHeader,
                                    SChar*             aRow )
{
    smpFreeSlotHeader* sCurFreeSlotHeader;
    smpFreeSlotHeader* sTailFreeSlotHeader;

    IDE_DASSERT( aFreePageHeader != NULL );
    IDE_DASSERT( aRow != NULL );

    sCurFreeSlotHeader = (smpFreeSlotHeader*)aRow;

    IDE_DASSERT( isValidFreeSlotList(aFreePageHeader) == ID_TRUE );

    sCurFreeSlotHeader->mNextFreeSlot = NULL;
    sTailFreeSlotHeader = (smpFreeSlotHeader*)aFreePageHeader->mFreeSlotTail;

    /* BUG-32386       [sm_recovery] If the ager remove the MMDB slot and the
     * checkpoint thread flush the page containing the slot at same time, the
     * server can misunderstand that the freed slot is the allocated slot. 
     * DropFlag는 설정돼었는데, SCN이 초기화되지 않은 경우가 발생할 수 있음
     * SCN을 초기화 해줌*/
    SM_SET_SCN_FREE_ROW( &sCurFreeSlotHeader->mCreateSCN );
    SM_SET_SCN_FREE_ROW( &sCurFreeSlotHeader->mLimitSCN );
    SMP_SLOT_INIT_POSITION( sCurFreeSlotHeader );

    if(sTailFreeSlotHeader == NULL)
    {
        IDE_DASSERT( aFreePageHeader->mFreeSlotHead == NULL );

        aFreePageHeader->mFreeSlotHead = aRow;
    }
    else
    {
        sTailFreeSlotHeader->mNextFreeSlot = (smpFreeSlotHeader*)aRow;
    }

    aFreePageHeader->mFreeSlotTail = aRow;

    aFreePageHeader->mFreeSlotCount++;

    IDE_DASSERT( isValidFreeSlotList(aFreePageHeader) == ID_TRUE );


    return;
}

/**********************************************************************
 * PageList의 유효한 레코드 갯수 반환
 *
 * aFixedEntry  : 검색하고자 하는 PageListEntry
 * aRecordCount : 반환하는 레코드 갯수
 **********************************************************************/
ULong smpFixedPageList::getRecordCount( smpPageListEntry* aFixedEntry )
{
    ULong sRecordCount;

    /* PROJ-2334 PMT */
    if( aFixedEntry->mRuntimeEntry != NULL )
    {
        // BUG-47368: getRecordCount, mutex 제거, atomic 연산으로 변경
        sRecordCount = idCore::acpAtomicGet64( &(aFixedEntry->mRuntimeEntry->mInsRecCnt) );
    }
    else
    {
        sRecordCount = 0;
    }

    return sRecordCount;
}

/**********************************************************************
 * PageList의 유효한 레코드 갯수의 변경.
 *
 * aFixedEntry  : 검색하고자 하는 PageListEntry
 * aRecordCount : 반환하는 레코드 갯수
 **********************************************************************/
IDE_RC smpFixedPageList::setRecordCount( smpPageListEntry* aFixedEntry,
                                         ULong             aRecordCount )
{
    // BUG-47368: setRecordCount, mutex 제거, atomic 연산으로 변경
    (void)idCore::acpAtomicSet64( &(aFixedEntry->mRuntimeEntry->mInsRecCnt), aRecordCount);

    return IDE_SUCCESS;
}

/**********************************************************************
 * PageList의 FreeSlotList,FreePageList,FreePagePool을 재구축한다.
 *
 * FreeSlot/FreePage 관련 정보는 Disk에 저장되는 Durable한 정보가 아니기때문에
 * 서버가 restart되면 재구축해주어야 한다.
 *
 * aTrans      : 작업을 수행하는 트랜잭션 객체
 * aFixedEntry : 구축하려는 PageListEntry
 **********************************************************************/

IDE_RC smpFixedPageList::refinePageList( void*             aTrans,
                                         scSpaceID         aSpaceID,
                                         UInt              aTableType,
                                         smpPageListEntry* aFixedEntry )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );

    // Slot을 Refine하고 각 Page마다 FreeSlotList를 구성하고
    // 각 페이지가 FreePage이면 우선 FreePageList[0]에 등록한다.
    IDE_TEST( buildFreeSlotList( aTrans,
                                 aSpaceID,
                                 aTableType,
                                 aFixedEntry )
              != IDE_SUCCESS );

    // FreePageList[0]에서 N개의 FreePageList에 FreePage들을 나눠주고
    smpFreePageList::distributePagesFromFreePageList0ToTheOthers(aFixedEntry);

    // EmptyPage(전혀사용하지않는 FreePage)가 필요이상이면
    // FreePagePool에 반납하고 FreePagePool에도 필요이상이면
    // DB에 반납한다.
    IDE_TEST(smpFreePageList::distributePagesFromFreePageList0ToFreePagePool(
                 aTrans,
                 aSpaceID,
                 aFixedEntry )
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * FreeSlotList 구축
 *
 * aTrans      : 작업을 수행하는 트랜잭션 객체
 * aTableType  : Table Type
 * aFixedEntry : 구축하려는 PageListEntry
 * aPtrList    : Index Rebuild를 위해 유효한 레코드들의 리스트를 만들어 반환
 **********************************************************************/
IDE_RC smpFixedPageList::buildFreeSlotList( void*             aTrans,
                                            scSpaceID         aSpaceID,
                                            UInt              aTableType,
                                            smpPageListEntry* aFixedEntry )
{
    SChar      * sCurPtr;
    SChar      * sNxtPtr;
    smiColumn ** sArrLobColumn;
    UInt         sLobColumnCnt;
    UInt         sState = 0;
    void       * sTable;
    scPageID     sCurPageID = SM_NULL_PID;
    scPageID     sPrvPageID = SM_NULL_PID;
    idBool       sRefined;

    IDE_TEST( smLayerCallback::getTableHeaderFromOID( 
                    aFixedEntry->mTableOID,
                    (void**)&sTable )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::makeLobColumnList(
                  sTable,
                  &sArrLobColumn,
                  &sLobColumnCnt )
              != IDE_SUCCESS );
    sState = 1;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );

    sCurPtr = NULL;

    while(1)
    {
        // FreeSlot을 정리하고
        IDE_TEST( nextOIDallForRefineDB( aSpaceID,
                                         aFixedEntry,
                                         sCurPtr,
                                         &sNxtPtr,
                                         &sCurPageID )
                  != IDE_SUCCESS );

        if(sNxtPtr == NULL)
        {
            break;
        }

        // sNxtPtr을 refine한다.
        IDE_TEST( refineSlot( aTrans,
                              aSpaceID,
                              aTableType,
                              sArrLobColumn,
                              sLobColumnCnt,
                              aFixedEntry,
                              sNxtPtr,
                              &sRefined )
                  != IDE_SUCCESS );

        /*
         * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
         * refine되지 않은 레코드(유효한 레코드)라면 Scan List에 추가한다.
         */
        if( (sRefined == ID_FALSE) && (sCurPageID != sPrvPageID) )
        {
            IDE_TEST( smpFixedPageList::linkScanList( aSpaceID,
                                                      sCurPageID,
                                                      aFixedEntry )
                      != IDE_SUCCESS );
            sPrvPageID = sCurPageID;
        }

        sCurPtr = sNxtPtr;
    }

    sState = 0;
    IDE_TEST( smLayerCallback::destLobColumnList( sArrLobColumn )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( smLayerCallback::destLobColumnList( sArrLobColumn )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Slot이 정리되지 않은 FreeSlot인지 확인
 *
 * aTrans            : 작업을 수행하는 트랜잭션 객체
 * aTableType        : Table Type
 * aArrLobColumn     : Lob Column List
 * aLobColumnCnt     : Lob Column Count
 * aFixedEntry       : 확인하려는 Slot의 소속 PageListEntry
 * aCurRow           : 확인하려는 Slot
 **********************************************************************/
IDE_RC smpFixedPageList::refineSlot( void*             aTrans,
                                     scSpaceID         aSpaceID,
                                     UInt              aTableType,
                                     smiColumn**       aArrLobColumn,
                                     UInt              aLobColumnCnt,
                                     smpPageListEntry* aFixedEntry,
                                     SChar*            aCurRow,
                                     idBool          * aRefined )
{
    smpSlotHeader     * sCurRowHeader;
    smSCN               sSCN;
    smOID               sFixOid;
    scPageID            sPageID;
    idBool              sIsNeedFreeSlot;
    smpFreePageHeader * sFreePageHeader;
    smcTableHeader    * sTable;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aCurRow != NULL );

    sPageID = SMP_SLOT_GET_PID(aCurRow);
    sCurRowHeader = (smpSlotHeader*)aCurRow;

    sSCN = sCurRowHeader->mCreateSCN;

    sFreePageHeader = smpFreePageList::getFreePageHeader(aSpaceID, sPageID);
    *aRefined = ID_FALSE;

    IDE_ASSERT( smLayerCallback::getTableHeaderFromOID( 
                    aFixedEntry->mTableOID,
                    (void**)&sTable )
                == IDE_SUCCESS );

    /* NULL Row인 경우 */
    IDE_TEST_CONT( SM_SCN_IS_NULL_ROW(sSCN) ,
                    cont_refine_slot_end );

    /* BUG-32544 [sm-mem-collection] The MMDB alter table operation can
     * generate invalid null row. */
    sFixOid = SM_MAKE_OID(sPageID, SMP_SLOT_GET_OFFSET( sCurRowHeader ) );

    if( sTable->mNullOID == sFixOid )
    {
        ideLog::log( IDE_SERVER_0, 
                     "InvalidNullRow : %u, %u - %llu",
                     aSpaceID,
                     sPageID,
                     aFixedEntry->mTableOID );
        /* 위쪽 SM_SCN_IS_NULL_ROW로 걸러졌어야 하는데
         * 걸러지지 않았다는 것은 문제가 있다는 뜻 */
        IDE_DASSERT( 0 );

        /* 하지만 여기서 서버를 비정상종료 시키면,
         * 서버 구동 자체가 실패하니 Release모드에서는
         * 위험이 크다. 따라서 FreeSlot으로의 등록만 막
         * 아서 정상 수행을 유도한다. */
        IDE_CONT( cont_refine_slot_end );
    }

    /* TASK-4690
     * update -> recordLockValidate 후 checkpoint 발생하여 lock 걸린 상태로
     * 이미지가 내려갈 수 있다. (로깅 없이) */
    if( SMP_SLOT_IS_LOCK_TRUE( sCurRowHeader ) )
    {
        SMP_SLOT_SET_UNLOCK( sCurRowHeader );
    }

    /* Slot이 Free될 필요가 있는지 확인하여 Free한다. */
    IDE_TEST( isNeedFreeSlot( sCurRowHeader,
                              aSpaceID,
                              aFixedEntry,
                              &sIsNeedFreeSlot ) != IDE_SUCCESS );

    if( sIsNeedFreeSlot == ID_TRUE )
    {
        IDE_TEST( setFreeSlot( aTrans,
                               aSpaceID,
                               sPageID,
                               aCurRow,
                               SMP_TABLE_NORMAL )
                  != IDE_SUCCESS );

        addFreeSlotToFreeSlotList( sFreePageHeader,
                                   aCurRow );
        *aRefined = ID_TRUE;

        IDE_CONT( cont_refine_slot_end );
    }

    if(SM_SCN_IS_NOT_INFINITE( sSCN ) &&
       SM_SCN_IS_GT( &sSCN, &(smmDatabase::getDicMemBase()->mSystemSCN) ))
    {
        ideLog::log(SM_TRC_LOG_LEVEL_MPAGE,
                    SM_TRC_MPAGE_INVALID_SCN,
                    sFixOid,
                    SM_SCN_TO_LONG( sSCN ),
                    SM_SCN_TO_LONG( *(smmDatabase::getSystemSCN()) ) );

        /* BUG-32144 [sm-mem-recovery] If the record scn is larger than system
         * scn, Emergency-startup fails.
         * 비상 구동시에는 SCN Check를 하면 안됨. 즉 Emergency Property가 0일
         * 때만 비정상 종료시킴*/
        IDE_TEST_RAISE( smuProperty::getEmergencyStartupPolicy() == SMR_RECOVERY_NORMAL,
                        err_invalide_scn );
    }

    // LPCH를 rebuild한다.
    if(aTableType != SMI_TABLE_META)
    {

        IDE_TEST( smLayerCallback::rebuildLPCH(
                      aFixedEntry->mTableOID,
                      aArrLobColumn,
                      aLobColumnCnt,
                      aCurRow )
                  != IDE_SUCCESS );
    }

    /*
     * [BUG-26415] XA 트랜잭션중 Partial Rollback(Unique Volation)된 Prepare
     *             트랜잭션이 존재하는 경우 서버 재구동이 실패합니다.
     * : Prepare Transaction은 아직 커밋이 안된 상태이기 때문에 RuntimeEntry
     *   의 InsRecCnt를 늘려서는 안된다.
     */
    if( SM_SCN_IS_INFINITE( sSCN ) )
    {
       if ( ( smLayerCallback::incRecCnt4InDoubtTrans( SMP_GET_TID( sSCN ),
                                                       aFixedEntry->mTableOID ) )
              != IDE_SUCCESS )
        {
            ideLog::logMem( IDE_DUMP_0,
                            (UChar*)aCurRow,
                            aFixedEntry->mSlotSize,
                            "Currupt CreateSCN\n"
                            "[ FixedSlot ]\n"
                            "TableOID :%"ID_UINT64_FMT"\n"
                            "SpaceID  :%"ID_UINT32_FMT"\n"
                            "PageID   :%"ID_UINT32_FMT"\n"
                            "Offset   :%"ID_UINT32_FMT"\n",
                            aFixedEntry->mTableOID,
                            aSpaceID,
                            sPageID,
                            (UInt)SMP_SLOT_GET_OFFSET( sCurRowHeader ) ); 

            /* BUG-38515 Prepare Tx가 아닌 상태에서 SCN이 깨진 경우에도
             * __SM_SKIP_CHECKSCN_IN_STARTUP 프로퍼티가 켜져 있다면
             * 서버를 멈추지 않고 관련 정보를 출력하고 그대로 진행한다.
             * BUG-41600 SkipCheckSCNInStartup 프로퍼티의 값이 2인 경우를 추가한다.이에 따른
             * 함수의 리턴값 변경한다. */
            IDE_ASSERT( smuProperty::isSkipCheckSCNInStartup() != 0 );
        }
        else
        {
            /* do nothing... */
        }
    }
    else
    {
        (aFixedEntry->mRuntimeEntry->mInsRecCnt)++;
    }

    IDE_EXCEPTION_CONT( cont_refine_slot_end );


    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalide_scn);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_INVALID_ROW_SCN));
    }
    IDE_EXCEPTION_END;

    ideLog::logMem( IDE_DUMP_0,
                    (UChar*)aCurRow,
                    aFixedEntry->mSlotSize,
                    "[ FixedSlot ]\n"
                    "TableOID :%llu\n"
                    "SpaceID  :%u\n"
                    "PageID   :%u\n"
                    "Offset   :%u\n",
                    aFixedEntry->mTableOID,
                    aSpaceID,
                    sPageID,
                    (UInt)SMP_SLOT_GET_OFFSET( sCurRowHeader ) );

    return IDE_FAILURE;
}

/**********************************************************************
 * 내용 : Slot이 Free 될 필요가 있는지 확인한다.
 *        BUG-31521 Prepare Transaction이 있을 경우, 삭제되고 Aging되지
 *        않은 Memory Row로 인하여 refine에서 실패 할 수 있습니다.
 *
 * Slot이 Free 되는 경우는 다음 3가지 중 하나이다.
 *
 * 1. Update Rollback의 경우 New Version은 Free된다.
 * 2. Update Commit의 경우 Old Version은 Free된다.
 * 3. Delete Row 연산이 수행 된 경우 해당 Slot이 Free된다.
 *
 * 보통의 경우 서버 운영중에 Aging되지만, 아직 Aging되지 않은 경우
 * Restart시 Refine과정 중에 해당 Slot이 Free된다.
 *
 * Delete Row, Update등의 연산이 수행 되었다고 해도,
 * XA Prepare Transaction이 변경한 경우 제거하지 않는다.
 * 이 경우는
 *  1. Update - Slot Header의 Next가 가리키는 New Version의 SCN이
 *              무한대의 값을 가진다.
 *  2. Delete - Slot Header의 Next가 무한대의 값을 가진다.
 *
 * ----------------------------------------------------------------
 * recovery 이후 prepared transaction에 의해 접근되고 있지 않은
 * 모든 row들의 SCN은 무한대가 아니다.
 * 그러나 prepared transaction이 존재하는 경우
 * 그 트랜잭션이 access한 레코드에 대하여 record lock을 획득하므로
 * row의 상태는 무한대.
 * ---------------------------------------------------------------
 *
 * aCurRowHeader   - [IN] Free Slot여부를 판단한 Slot의 Header
 * SpaceID         - [IN] 해당 Slot이 포함된 TableSpace의 ID
 * aFixedEntry     - [IN] 해당 Slot이 포함된 PageListEntry
 * aIsNeedFreeSlot - [OUT] Free가 필요한 Slot인지의 여부를 반환
 *
 **********************************************************************/
IDE_RC smpFixedPageList::isNeedFreeSlot( smpSlotHeader    * aCurRowHeader,
                                         scSpaceID          aSpaceID,
                                         smpPageListEntry * aFixedEntry,
                                         idBool           * aIsNeedFreeSlot )
{
    smSCN           sSCN;
    smTID           sDummyTID;
    smSCN           sNextSCN;
    smTID           sNextTID;
    ULong           sNextOID;
    idBool          sIsNeedFreeSlot = ID_FALSE;
    smpSlotHeader * sSlotHeaderPtr  = NULL;

    sNextOID = SMP_SLOT_GET_NEXT_OID( aCurRowHeader );

    SMX_GET_SCN_AND_TID( aCurRowHeader->mCreateSCN, sSCN,     sDummyTID );
    SMX_GET_SCN_AND_TID( aCurRowHeader->mLimitSCN,  sNextSCN, sNextTID );

    if( SMP_SLOT_IS_SKIP_REFINE( aCurRowHeader ) )
    {
        /* Refine시 Free되면 안되는 예외적인 Slot일 경우 본 Flag가 기록되어 있다.
         * Refine시 까지만 사용되는 Flag로 확인후 바로 제거한다.*/
        SMP_SLOT_UNSET_SKIP_REFINE( aCurRowHeader );

        /* 예외는 현재로는 XA Prepare Transaction이 Update한 Slot의
         * Old Version인 경우 하나뿐이다. */
        /* 이 경우 Header Next는 New Version을 가리키는 OID이며
         * 아직 Commit되지 않았으므로 New Version의 SCN은 무한대이다.*/
        IDE_ERROR( SM_IS_OID( sNextOID ) );
        /* BUG-42724 limitSCN이 free일 경우 추가( XA + partial rollback이 발생한 경우) */
        IDE_ERROR( SM_SCN_IS_INFINITE( sNextSCN ) || SM_SCN_IS_FREE_ROW( sNextSCN) );
        IDE_ERROR( SM_SCN_IS_NOT_LOCK_ROW( sNextSCN ) );
        
        IDE_CONT( cont_is_need_free_slot_end );
    }

    // SCN에 Delete bit가 기록된 경우는 Rollback된 경우이다.
    // 더이상 New Version이 필요 없으므로, 바로 삭제한다.
    if( SM_SCN_IS_DELETED( sSCN ) )
    {
        sIsNeedFreeSlot = ID_TRUE;
        IDE_CONT( cont_is_need_free_slot_end );
    }

    /* BUG-41600 : refine 단계에서 invalid한 slot이 존재하는 메모리 테이블을 보정한다. */
    if( smuProperty::isSkipCheckSCNInStartup() == 2 )
    {
        if( SM_SCN_IS_INFINITE( sSCN ) )
        {
            if( SM_SCN_IS_FREE_ROW( sNextSCN ) )
            {
                /* Prepare Tx가 아닌 상태에서 SCN이 깨진 경우 */
                if ( smLayerCallback::getPreparedTransCnt() == 0 )
                {
                    ideLog::logMem( IDE_DUMP_0,
                                    (UChar*)aCurRowHeader,
                                    aFixedEntry->mSlotSize,
                                    "Currupt CreateSCN\n"
                                    "[ FixedSlot ]\n"
                                    "TableOID :%"ID_UINT64_FMT"\n"
                                    "SpaceID  :%"ID_UINT32_FMT"\n"
                                    "PageID   :%"ID_UINT32_FMT"\n"
                                    "Offset   :%"ID_UINT32_FMT"\n",
                                    aFixedEntry->mTableOID,
                                    aSpaceID,
                                    (UInt)SMP_SLOT_GET_PID( aCurRowHeader ),
                                    (UInt)SMP_SLOT_GET_OFFSET( aCurRowHeader ) );

                    sIsNeedFreeSlot = ID_TRUE;

                    IDE_CONT( cont_is_need_free_slot_end );
                }
            }
        }
    }

    /* Header Next를 확인해본다.
     * Header Next에는 다음 값 중 하나가 올 수 있다.
     *
     * 0. 0 - Free Slot대상이 아닌경우
     * 1. OID - Update시의 Old Version인 경우
     *          Header Next에는 New Verstion을 가리키는 OID가 온다.
     * 2. CommitSCN + Delete bit - Slot이 Delete된 경우
     * 3. Infinite SCN + Delete bit - XA Prepare Trans에 의해 Delete 된 경우 */

    /* 0. Free Slot대상이 아닌 경우 */
    IDE_TEST_CONT( SM_SCN_IS_FREE_ROW( sNextSCN ),
                    cont_is_need_free_slot_end );

    if( SM_SCN_IS_INFINITE( sNextSCN ) )
    {
        if( SM_SCN_IS_DELETED( sNextSCN ) )
        {
            /* DELETE BIT가 설정되어 있고, Next SCN이 무한대인 경우는
             * IN-DOUBT 트랜잭션이 DELETE를 수행한 레코드이다.
             * BUG-31521 Next가 무한대의 SCN인 경우에만 호출 하여야 합니다. */

            /* BUG-42724 : sTID -> sNextTID로 수정. next version에 대한 TID를 사용여야 한다. */
            if ( ( smLayerCallback::decRecCnt4InDoubtTrans( sNextTID, aFixedEntry->mTableOID ) )
                != IDE_SUCCESS )
            {    
                ideLog::logMem( IDE_DUMP_0,
                                (UChar*)aCurRowHeader,
                                aFixedEntry->mSlotSize,
                                "Currupt LimitSCN"
                                "[ FixedSlot ]\n"
                                "TableOID :%"ID_UINT64_FMT"\n"
                                "SpaceID  :%"ID_UINT32_FMT"\n"
                                "PageID   :%"ID_UINT32_FMT"\n"
                                "Offset   :%"ID_UINT32_FMT"\n",
                                aFixedEntry->mTableOID,
                                aSpaceID,
                                (UInt)SMP_SLOT_GET_PID( aCurRowHeader ),
                                (UInt)SMP_SLOT_GET_OFFSET( aCurRowHeader ) ); 

                /* BUG-38515 Prepare Tx가 아닌 상태에서 SCN이 깨진 경우에도
                 * __SM_SKIP_CHECKSCN_IN_STARTUP 프로퍼티가 켜져 있다면
                 * 서버를 멈추지 않고 관련 정보를 출력하고 그대로 진행한다.
                 * BUG-41600 SkipCheckSCNInStartup 프로퍼티의 값이 2인 경우를 추가한다.이에 따른
                 * 함수의 리턴값을 변경한다. */
                IDE_ASSERT( smuProperty::isSkipCheckSCNInStartup() != 0 ); 
            }    
            else 
            {    
                /* do nothing ... */
            } 
        }

        /* BUG-39233
         * next가 설정되어 있고, next의 createSCN 이 infinite가 아니면,
         * 현재 버전은 정리되어야 한다. */
        if( sNextOID != SMI_NULL_OID )
        {
            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                               sNextOID,
                                               (void**)&sSlotHeaderPtr )
                        == IDE_SUCCESS );

            if( SM_SCN_IS_NOT_INFINITE( sSlotHeaderPtr->mCreateSCN ) )
            {
                // trc
                ideLog::logMem( IDE_DUMP_0,
                                (UChar*)aCurRowHeader,
                                aFixedEntry->mSlotSize,
                                "Invalid Currupt LimitSCN\n"
                                "[ FixedSlot ]\n"
                                "TableOID :%"ID_UINT64_FMT"\n"
                                "SpaceID  :%"ID_UINT32_FMT"\n"
                                "PageID   :%"ID_UINT32_FMT"\n"
                                "Offset   :%"ID_UINT32_FMT"\n",
                                aFixedEntry->mTableOID,
                                aSpaceID,
                                (UInt)SMP_SLOT_GET_PID( aCurRowHeader ),
                                (UInt)SMP_SLOT_GET_OFFSET( aCurRowHeader ) ); 

                /* debug 모드에서는 assert 시키고,
                 * release 모드에서는 freeslot 한다. */
                IDE_DASSERT( 0 );

                sIsNeedFreeSlot = ID_TRUE;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sIsNeedFreeSlot = ID_TRUE;
    }

    IDE_EXCEPTION_CONT( cont_is_need_free_slot_end );

    *aIsNeedFreeSlot = sIsNeedFreeSlot;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * PageListEntry를 초기화한다.
 *
 * aFixedEntry : 초기화하려는 PageListEntry
 * aTableOID   : PageListEntry의 테이블 OID
 * aSlotSize   : PageListEntry의 SlotSize
 **********************************************************************/
void smpFixedPageList::initializePageListEntry( smpPageListEntry* aFixedEntry,
                                                smOID             aTableOID,
                                                vULong            aSlotSize )
{
    IDE_DASSERT(aFixedEntry != NULL);
    IDE_DASSERT(aTableOID != 0);
    IDE_DASSERT(aSlotSize > 0);

    aFixedEntry->mTableOID     = aTableOID;
    aFixedEntry->mSlotSize     = aSlotSize;
    aFixedEntry->mSlotCount    =
        SMP_PERS_PAGE_BODY_SIZE / aFixedEntry->mSlotSize;
    aFixedEntry->mRuntimeEntry = NULL;


    return;
}

/**********************************************************************
 * aRow의 SlotHeader를 Update하여 Allocate된 Slot으로 만든다.
 *
 * aTrans    : Transaction Pointer
 * aInfinite : Slot Header에 setting할 SCN값.
 * aRow      : Record의 Pointer
 **********************************************************************/
void smpFixedPageList::setAllocatedSlot( smSCN  aInfinite,
                                         SChar *aRow )
{
    smpSlotHeader  *sCurSlotHeader;

    // Init header of fixed row
    sCurSlotHeader = (smpSlotHeader*)aRow;

    SM_SET_SCN( &(sCurSlotHeader->mCreateSCN), &aInfinite );

    SM_SET_SCN_FREE_ROW( &(sCurSlotHeader->mLimitSCN) );

    SMP_SLOT_INIT_POSITION( sCurSlotHeader );
    SMP_SLOT_SET_USED( sCurSlotHeader );
}

#ifdef DEBUG
/**********************************************************************
 * Page내의 FreeSlotList의 연결이 올바른지 검사한다.
 *
 * aFreePageHeader : 검사하려는 FreeSlotList가 있는 Page의 FreePageHeader
 **********************************************************************/
idBool
smpFixedPageList::isValidFreeSlotList(smpFreePageHeader* aFreePageHeader )
{
    idBool             sIsValid;

    if ( iduProperty::getEnableRecTest() == 1 )
    {
        vULong             sPageCount = 0;
        smpFreeSlotHeader* sCurFreeSlotHeader = NULL;
        smpFreeSlotHeader* sNxtFreeSlotHeader;
        smpFreePageHeader* sFreePageHeader = aFreePageHeader;

        IDE_DASSERT( sFreePageHeader != NULL );

        sIsValid = ID_FALSE;

        sNxtFreeSlotHeader = (smpFreeSlotHeader*)sFreePageHeader->mFreeSlotHead;

        while(sNxtFreeSlotHeader != NULL)
        {
            sCurFreeSlotHeader = sNxtFreeSlotHeader;

            sPageCount++;

            sNxtFreeSlotHeader = sCurFreeSlotHeader->mNextFreeSlot;
        }

        if(aFreePageHeader->mFreeSlotCount == sPageCount &&
           aFreePageHeader->mFreeSlotTail  == (SChar*)sCurFreeSlotHeader)
        {
            sIsValid = ID_TRUE;
        }


        if ( sIsValid == ID_FALSE )
        {
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE,
                        SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST1,
                        (ULong)aFreePageHeader->mSelfPageID);
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE,
                        SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST2,
                        aFreePageHeader->mFreeSlotCount);
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE,
                        SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST3,
                        sPageCount);
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE,
                        SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST4,
                        aFreePageHeader->mFreeSlotHead);
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE,
                        SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST5,
                        aFreePageHeader->mFreeSlotTail);
            ideLog::log(SM_TRC_LOG_LEVEL_MPAGE,
                        SM_TRC_MPAGE_INVALID_FREE_SLOT_LIST6,
                        sCurFreeSlotHeader);

#    if defined(TSM_DEBUG)
            idlOS::printf( "Invalid Free Slot List Detected. Page #%"ID_UINT64_FMT"\n",
                           (ULong) aFreePageHeader->mSelfPageID );
            idlOS::printf( "Free Slot Count on Page ==> %"ID_UINT64_FMT"\n",
                           aFreePageHeader->mFreeSlotCount );
            idlOS::printf( "Free Slot Count on List ==> %"ID_UINT64_FMT"\n",
                           sPageCount );
            idlOS::printf( "Free Slot Head on Page ==> %"ID_xPOINTER_FMT"\n",
                           aFreePageHeader->mFreeSlotHead );
            idlOS::printf( "Free Slot Tail on Page ==> %"ID_xPOINTER_FMT"\n",
                           aFreePageHeader->mFreeSlotTail );
            idlOS::printf( "Free Slot Tail on List ==> %"ID_xPOINTER_FMT"\n",
                           sCurFreeSlotHeader );
            fflush( stdout );
#    endif  /* TSM_DEBUG */
        }

    }
    else
    {
        sIsValid = ID_TRUE;
    }


    return sIsValid;
}
#endif

/**********************************************************************
 *  BUG-31206 improve usability of DUMPCI and DUMPDDF
 *            Slot Header를 altibase_sm.log에 덤프한다
 *
 *  Slot Header를 덤프한다
 *
 *  aSlotHeader : dump할 slot 헤더
 *  aOutBuf     : 대상 버퍼
 *  aOutSize    : 대상 버퍼의 크기
 **********************************************************************/
IDE_RC smpFixedPageList::dumpSlotHeaderByBuffer(
                                        smpSlotHeader  * aSlotHeader,
                                        idBool           aDisplayTable,
                                        SChar          * aOutBuf,
                                        UInt             aOutSize )
{
    smSCN    sCreateSCN;
    smSCN    sLimitSCN;
    smSCN    sRowSCN;
    smTID    sRowTID;
    smSCN    sNxtSCN;
    smTID    sNxtTID;
    scPageID sPID;

    IDE_ERROR( aSlotHeader != NULL );
    IDE_ERROR( aOutBuf     != NULL );
    IDE_ERROR( aOutSize     > 0 );

    sPID = SMP_SLOT_GET_PID( aSlotHeader );
    SM_GET_SCN( &sCreateSCN, &(aSlotHeader->mCreateSCN) );
    SM_GET_SCN( &sLimitSCN,  &(aSlotHeader->mLimitSCN) );

    SMX_GET_SCN_AND_TID( sCreateSCN, sRowSCN, sRowTID );
    SMX_GET_SCN_AND_TID( sLimitSCN,  sNxtSCN, sNxtTID );

    if( aDisplayTable == ID_FALSE )
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "Slot PID      : %"ID_UINT32_FMT"\n"
                             "Slot CreateSCN : 0x%"ID_XINT64_FMT"\n"
                             "Slot CreateTID :   %"ID_UINT32_FMT"\n"
                             "Slot LimitSCN  : 0x%"ID_XINT64_FMT"\n"
                             "Slot LimitTID  :   %"ID_UINT32_FMT"\n"
                             "Slot NextOID   : 0x%"ID_XINT64_FMT"\n"
                             "Slot Offset    : %"ID_UINT64_FMT"\n"
                             "Slot Flag      : %"ID_XINT64_FMT"\n",
                             sPID,
                             SM_SCN_TO_LONG( sRowSCN ),
                             sRowTID,
                             SM_SCN_TO_LONG( sNxtSCN ),
                             sNxtTID, 
                             SMP_SLOT_GET_NEXT_OID( aSlotHeader ),
                             (ULong)SMP_SLOT_GET_OFFSET( aSlotHeader ),
                             (ULong)SMP_SLOT_GET_FLAGS( aSlotHeader ) );
    }
    else
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%7"ID_UINT32_FMT
                             "     0x%-14"ID_XINT64_FMT
                             "%-14"ID_UINT32_FMT
                             "0x%-11"ID_XINT64_FMT
                             "  %-15"ID_UINT32_FMT
                             "0x%-7"ID_XINT64_FMT
                             "%-12"ID_UINT64_FMT
                             "%-4"ID_XINT64_FMT,
                             sPID,
                             SM_SCN_TO_LONG( sRowSCN ),
                             sRowTID,
                             SM_SCN_TO_LONG( sNxtSCN ),
                             sNxtTID ,
                             SMP_SLOT_GET_NEXT_OID( aSlotHeader ),
                             (ULong)SMP_SLOT_GET_OFFSET( aSlotHeader ),
                             (ULong)SMP_SLOT_GET_FLAGS( aSlotHeader ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**********************************************************************
 *  BUG-31206 improve usability of DUMPCI and DUMPDDF
 *            Slot Header를 altibase_sm.log에 덤프한다
 *
 *  dumpSlotHeaderByBuffer 함수를 이용해 TRC boot log에 정보를 기록한다.
 *
 *  aSlotHeader : dump할 slot 헤더
 **********************************************************************/
IDE_RC smpFixedPageList::dumpSlotHeader( smpSlotHeader     * aSlotHeader )
{
    SChar          * sTempBuffer;

    IDE_ERROR( aSlotHeader != NULL );

    if( iduMemMgr::calloc( IDU_MEM_SM_SMC,
                           1,
                           ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                           (void**)&sTempBuffer )
        == IDE_SUCCESS )
    {
        sTempBuffer[0] = '\0';
        dumpSlotHeaderByBuffer( aSlotHeader,
                                ID_FALSE, // display table
                                sTempBuffer,
                                IDE_DUMP_DEST_LIMIT );

        ideLog::log( IDE_ERR_0,
                     "Slot Offset %"ID_xPOINTER_FMT"\n"
                     "%s\n",
                     aSlotHeader,
                     sTempBuffer );

        (void) iduMemMgr::free( sTempBuffer );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * BUG-31206 improve usability of DUMPCI and DUMPDDF
 *           Slot Header를 altibase_sm.log에 덤프한다
 *
 * Description: <aSpaceID, aPageID>에 있는 Record Header들을
 *              altibase_boot.log에 찍는다.
 *
 * aPagePtr    - [IN]  Dump할 page의 주소
 * aSlotSize   - [IN]  Slit의 크기
 * aOutBuf     : [OUT] 대상 버퍼
 * aOutSize    : [OUT] 대상 버퍼의 크기
 **********************************************************************/
IDE_RC smpFixedPageList::dumpFixedPageByBuffer( UChar            * aPagePtr,
                                                UInt               aSlotSize,
                                                SChar            * aOutBuf,
                                                UInt               aOutSize )
{
    UInt                sSlotCnt;
    UInt                sSlotSize;
    smpSlotHeader     * sCurSlotHeader;
    smpSlotHeader     * sNxtSlotHeader;
    UInt                sCurOffset;
    smpPersPageHeader * sHeader;
    UInt                i;

    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aOutBuf  != NULL );
    IDE_ERROR( aOutSize  > 0 );

    sSlotSize      = aSlotSize;

    sCurOffset     = (UShort)SMP_PERS_PAGE_BODY_OFFSET;
    sHeader        = (smpPersPageHeader*)aPagePtr;
    sNxtSlotHeader = (smpSlotHeader*)(aPagePtr + sCurOffset);

    ideLog::ideMemToHexStr( aPagePtr,
                            SM_PAGE_SIZE,
                            IDE_DUMP_FORMAT_NORMAL,
                            aOutBuf,
                            aOutSize );

    /* Flags[ Skip Refine : 4 | Drop : 2 | Used : 1 ] */
    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "\n= FixedPage =\n"
                         "SelfPID      : %"ID_UINT32_FMT"\n"
                         "PrevPID      : %"ID_UINT32_FMT"\n"
                         "NextPID      : %"ID_UINT32_FMT"\n"
                         "Type         : %"ID_UINT32_FMT"\n"
                         "TableOID     : %"ID_vULONG_FMT"\n"
                         "AllocListID  : %"ID_UINT32_FMT"\n\n"
                         "PID         CreateSCN             CreateTID         "
                         "LimitSCN              LimitTID          NextOID     "
                         "        Offset             Flags[SR|DR|US]\n",
                         sHeader->mSelfPageID,
                         sHeader->mPrevPageID,
                         sHeader->mNextPageID,
                         sHeader->mType,
                         sHeader->mTableOID,
                         sHeader->mAllocListID );

    /* 유효한 Slot 크기가 설정 되었으면 */
    if( ( sSlotSize > 0 ) &&
        ( sSlotSize < SMP_PERS_PAGE_BODY_SIZE ) )
    {
        sSlotCnt       = SMP_PERS_PAGE_BODY_SIZE / aSlotSize;

        for( i = 0; i < sSlotCnt; i++)
        {
            sCurSlotHeader = sNxtSlotHeader;

            dumpSlotHeaderByBuffer( sCurSlotHeader,
                                    ID_TRUE, // display table
                                    aOutBuf,
                                    aOutSize );

            sNxtSlotHeader =
                (smpSlotHeader*)((SChar*)sCurSlotHeader + sSlotSize);
            sCurOffset += sSlotSize;

            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "\n" );
        }
    }
    else
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "\nInvalidSlotSize : %"ID_UINT32_FMT
                             "( %"ID_UINT32_FMT" ~  %"ID_UINT32_FMT" )\n",
                             aSlotSize,
                             0,
                             SMP_PERS_PAGE_BODY_SIZE
                           );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * BUG-31206 improve usability of DUMPCI and DUMPDDF
 *           Slot Header를 altibase_sm.log에 덤프한다
 *
 * dumpFixedPageByByffer 함수를 이용해 TRC boot log에 정보를 기록한다.
 *
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] Page ID
 * aSlotSize   - [IN] Row Slot Size
 **********************************************************************/

IDE_RC smpFixedPageList::dumpFixedPage( scSpaceID         aSpaceID,
                                        scPageID          aPageID,
                                        UInt              aSlotSize )
{
    SChar     * sTempBuffer;
    UChar     * sPagePtr;

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            aPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );

    if( iduMemMgr::calloc( IDU_MEM_SM_SMC,
                           1,
                           ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                           (void**)&sTempBuffer )
        == IDE_SUCCESS )
    {
        sTempBuffer[0] = '\0';
        dumpFixedPageByBuffer( sPagePtr,
                               aSlotSize,
                               sTempBuffer,
                               IDE_DUMP_DEST_LIMIT );

        ideLog::log( IDE_SERVER_0,
                     "SpaceID      : %u\n"
                     "PageID       : %u\n"
                     "PageOffset   : %"ID_xPOINTER_FMT"\n"
                     "%s\n",
                     aSpaceID,
                     aPageID,
                     sPagePtr,
                     sTempBuffer );

        (void) iduMemMgr::free( sTempBuffer );
    }

    return IDE_SUCCESS;
}

