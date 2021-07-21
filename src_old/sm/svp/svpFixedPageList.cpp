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
 
#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smErrorCode.h>
#include <svm.h>
#include <svpFixedPageList.h>
#include <svpFreePageList.h>
#include <svpAllocPageList.h>
#include <svpManager.h>
#include <svmExpandChunk.h>
#include <svpReq.h>

/**********************************************************************
 * Tx's PrivatePageList의 FreePage로부터 Slot를 할당할 수 있을지 검사하고
 * 가능하면 할당한다.
 *
 * aTrans     : 작업하는 트랜잭션 객체
 * aTableOID  : 할당하려는 테이블 OID
 * aRow       : 할당해서 반환하려는 Slot 포인터
 **********************************************************************/

IDE_RC svpFixedPageList::tryForAllocSlotFromPrivatePageList(
    void             * aTrans,
    scSpaceID          aSpaceID,
    smOID              aTableOID,
    smpPageListEntry * aFixedEntry,
    SChar           ** aRow )
{
    smpPrivatePageListEntry* sPrivatePageList = NULL;
    smpFreePageHeader*       sFreePageHeader;

    IDE_DASSERT(aTrans != NULL);
    IDE_DASSERT(aRow != NULL);

    *aRow = NULL;

    IDE_TEST( smLayerCallback::findVolPrivatePageList( aTrans,
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
                svpFreePageList::removeFixedFreePageFromPrivatePageList(
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
IDE_RC svpFixedPageList::tryForAllocSlotFromFreePageList(
    void*             aTrans,
    scSpaceID         aSpaceID,
    smpPageListEntry* aFixedEntry,
    UInt              aPageListID,
    SChar**           aRow )
{
    UInt                  sState = 0;
    UInt                  sSizeClassID;
    idBool                sIsPageAlloced;
    smpFreePageHeader*    sFreePageHeader;
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
                IDE_TEST(sFreePageHeader->mMutex.lock(NULL) != IDE_SUCCESS);
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
                    IDE_TEST(svpFreePageList::modifyPageSizeClass(
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

        IDE_TEST( svpFreePageList::tryForAllocPagesFromPool( aFixedEntry,
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
IDE_RC svpFixedPageList::unlinkScanList( scSpaceID          aSpaceID,
                                         scPageID           aPageID,
                                         smpPageListEntry * aFixedEntry )
{
    svmPCH               * sMyPCH;
    svmPCH               * sNxtPCH;
    svmPCH               * sPrvPCH;
    smpScanPageListEntry * sScanPageList;

    sScanPageList = &(aFixedEntry->mRuntimeEntry->mScanPageList);

    IDU_FIT_POINT("svpFixedPageList::unlinkScanList::wait1");

    IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

    IDE_DASSERT( sScanPageList->mHeadPageID != SM_NULL_PID );
    IDE_DASSERT( sScanPageList->mTailPageID != SM_NULL_PID );

    sMyPCH = (svmPCH*)(smmManager::getPCH( aSpaceID,
                                           aPageID ));
    IDE_DASSERT( sMyPCH != NULL );
    IDE_DASSERT( sMyPCH->mNxtScanPID != SM_NULL_PID );
    IDE_DASSERT( sMyPCH->mPrvScanPID != SM_NULL_PID );

    /* BUG-43463 Fullscan 동시성 개선,
     * - svnnMoveNextNonBlock()등 과의 동시성 향상을 위한 atomic 세팅
     * - 대상은 modify_seq, nexp_pid, prev_pid이다.
     * - modify_seq는 link, unlink시점에 변경된다.
     * - link, unlink 진행 중에는 modify_seq가 홀수이다
     * - 첫번째 page는 lock을 잡고 확인하므로 atomic으로 set하지 않아도 된다.
     * */
    SVM_PCH_SET_MODIFYING( sMyPCH );

    if( sScanPageList->mHeadPageID == aPageID )
    {
        if( sMyPCH->mNxtScanPID != SM_SPECIAL_PID )
        {
            IDE_DASSERT( sScanPageList->mTailPageID != aPageID );
            IDE_DASSERT( sMyPCH->mPrvScanPID == SM_SPECIAL_PID );
            sNxtPCH = (svmPCH*)(smmManager::getPCH( aSpaceID,
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
        sPrvPCH = (svmPCH*)(smmManager::getPCH( aSpaceID,
                                                sMyPCH->mPrvScanPID ));

        if( sMyPCH->mNxtScanPID != SM_SPECIAL_PID )
        {
            sNxtPCH = (svmPCH*)(smmManager::getPCH( aSpaceID,
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

    idCore::acpAtomicSet32( &( sMyPCH->mPrvScanPID),
                            SM_NULL_PID );

    SVM_PCH_SET_MODIFIED( sMyPCH );

    IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 *
 * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
 *
 * 주어진 페이지를 Scan List로부터 제거한다.
 *
 ****************************************************************************/
IDE_RC svpFixedPageList::linkScanList( scSpaceID          aSpaceID,
                                       scPageID           aPageID,
                                       smpPageListEntry * aFixedEntry )
{
    svmPCH               * sMyPCH;
    svmPCH               * sTailPCH;
    smpScanPageListEntry * sScanPageList;
#ifdef DEBUG
    smpFreePageHeader    * sFreePageHeader;
#endif
    sScanPageList = &(aFixedEntry->mRuntimeEntry->mScanPageList);

    IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

#ifdef DEBUG
    sFreePageHeader = svpFreePageList::getFreePageHeader( aSpaceID,
                                                          aPageID );

#endif
    IDE_DASSERT( (sFreePageHeader == NULL) ||
                 (sFreePageHeader->mFreeSlotCount != aFixedEntry->mSlotCount) );

    sMyPCH = (svmPCH*)(smmManager::getPCH( aSpaceID,
                                           aPageID ));
    IDE_DASSERT( sMyPCH != NULL );
    IDE_DASSERT( sMyPCH->mNxtScanPID == SM_NULL_PID );
    IDE_DASSERT( sMyPCH->mPrvScanPID == SM_NULL_PID );

    /* BUG-43463 Fullscan 동시성 개선,
     * - svnnMoveNextNonBlock()등 과의 동시성 향상을 위한 atomic 세팅
     * - 대상은 modify_seq, nexp_pid, prev_pid이다.
     * - modify_seq는 link, unlink시점에 변경된다.
     * - link, unlink 진행 중에는 modify_seq가 홀수이다
     * - 첫번째 page는 lock을 잡고 확인하므로 atomic으로 set하지 않아도 된다.
     * */
    SVM_PCH_SET_MODIFYING( sMyPCH );

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
        sTailPCH = (svmPCH*)(smmManager::getPCH( aSpaceID,
                                                 sScanPageList->mTailPageID ));

        idCore::acpAtomicSet32( &(sTailPCH->mNxtScanPID),
                                aPageID );

        idCore::acpAtomicSet32( &(sMyPCH->mNxtScanPID),
                                SM_SPECIAL_PID );

        idCore::acpAtomicSet32( &(sMyPCH->mPrvScanPID),
                                sScanPageList->mTailPageID );

        sScanPageList->mTailPageID = aPageID;
    }

    SVM_PCH_SET_MODIFIED( sMyPCH );

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
IDE_RC svpFixedPageList::setRuntimeNull( smpPageListEntry* aFixedEntry )
{
    IDE_DASSERT( aFixedEntry != NULL );

    // RuntimeEntry 초기화
    IDE_TEST(svpFreePageList::setRuntimeNull( aFixedEntry )
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
IDE_RC svpFixedPageList::initEntryAtRuntime(
    smOID                  aTableOID,
    smpPageListEntry*      aFixedEntry,
    smpAllocPageListEntry* aAllocPageList )
{
    SChar                   sBuffer[128];
    smpScanPageListEntry  * sScanPageList;
    UInt                    sState  = 0;

    IDE_DASSERT( aFixedEntry != NULL );
    IDE_ASSERT( aTableOID == aFixedEntry->mTableOID);
    IDE_DASSERT( aAllocPageList != NULL );

    // RuntimeEntry 초기화
    IDE_TEST(svpFreePageList::initEntryAtRuntime( aFixedEntry )
             != IDE_SUCCESS);

    aFixedEntry->mRuntimeEntry->mAllocPageList = aAllocPageList;
    sScanPageList = &(aFixedEntry->mRuntimeEntry->mScanPageList);

    sScanPageList->mHeadPageID = SM_NULL_PID;
    sScanPageList->mTailPageID = SM_NULL_PID;

    idlOS::snprintf( sBuffer, 128,
                     "SCAN_PAGE_LIST_MUTEX_%"
                     ID_XINT64_FMT"",
                     (ULong)aTableOID );

    /* svpFixedPageList_initEntryAtRuntime_malloc_Mutex.tc */
    IDU_FIT_POINT("svpFixedPageList::initEntryAtRuntime::malloc::Mutex");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SVP,
                                 sizeof(iduMutex),
                                 (void **)&(sScanPageList->mMutex),
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sScanPageList->mMutex->initialize( sBuffer,
                                                 IDU_MUTEX_KIND_NATIVE,
                                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sScanPageList->mMutex ) == IDE_SUCCESS );
            sScanPageList->mMutex = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * memory table의  fixed page list entry가 포함하는 runtime 정보 해제
 *
 * aFixedEntry : 해제하려는 PageListEntry
 ***********************************************************************/
IDE_RC svpFixedPageList::finEntryAtRuntime( smpPageListEntry* aFixedEntry )
{
    UInt                   sPageListID;
    smpScanPageListEntry * sScanPageList;

    IDE_DASSERT( aFixedEntry != NULL );

    if (aFixedEntry->mRuntimeEntry != NULL)
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
            svpAllocPageList::finEntryAtRuntime(
                &(aFixedEntry->mRuntimeEntry->mAllocPageList[sPageListID]) );
        }

        // RuntimeEntry 제거
        IDE_TEST(svpFreePageList::finEntryAtRuntime(aFixedEntry)
                 != IDE_SUCCESS);

        // svpFreePageList::finEntryAtRuntime에서 RuntimeEntry를 NULL로 세팅
        IDE_ASSERT( aFixedEntry->mRuntimeEntry == NULL );
    }
    else
    {
        /* Memory Table인 경우엔 aFixedEntry->mRuntimeEntry가 NULL인 경우
           (OFFLINE/DISCARD)가 있지만 Volatile Table에 대해서는
           aFixedEntry->mRuntimeEntry가 이미 NULL인 경우는 없다.
           만약 aFixedEntry->mRuntimeEntry가 NULL이라면
           어딘가에 버그가 있다는 의미이다. */

        IDE_DASSERT(0);
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
IDE_RC svpFixedPageList::freePageListToDB( void*             aTrans,
                                           scSpaceID         aSpaceID,
                                           smOID             aTableOID,
                                           smpPageListEntry* aFixedEntry )
{
    UInt sPageListID;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );
    IDE_ASSERT( aTableOID == aFixedEntry->mTableOID );

    // [0] FreePageList 제거

    svpFreePageList::initializeFreePageListAndPool(aFixedEntry);

    /* ----------------------------
     * [1] fixed page list를
     *     svmManager에 반환한다.
     * ---------------------------*/

    for(sPageListID = 0;
        sPageListID < SMP_PAGE_LIST_COUNT;
        sPageListID++)
    {
        // for AllocPageList
        IDE_TEST( svpAllocPageList::freePageListToDB(
                      aTrans,
                      aSpaceID,
                      &(aFixedEntry->mRuntimeEntry->mAllocPageList[sPageListID]) )
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
void svpFixedPageList::initializePage( vULong       aSlotSize,
                                       vULong       aSlotCount,
                                       UInt         aPageListID,
                                       smOID        aTableOID,
                                       smpPersPage* aPage)
{
    UInt               sSlotCounter;
    UShort             sCurOffset;
    smpFreeSlotHeader* sCurFreeSlotHeader = NULL;
    smpFreeSlotHeader* sNextFreeSlotHeader;

    // BUG-26937 CodeSonar::NULL Pointer Dereference (4)
    IDE_DASSERT( aSlotSize > 0 );
    IDE_ASSERT( aSlotCount > 0 );
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

    sCurFreeSlotHeader->mNextFreeSlot = NULL;

    IDL_MEM_BARRIER;

    return;
}

/**********************************************************************
 *  Slot Header를 altibase_sm.log에 덤프한다
 *
 *  aSlotHeader : dump할 slot 헤더
 **********************************************************************/
IDE_RC svpFixedPageList::dumpSlotHeader( smpSlotHeader     * aSlotHeader )
{
    smSCN sCreateSCN;
    smSCN sLimitSCN;
    smSCN sRowSCN;
    smTID sRowTID;
    smSCN sNxtSCN;
    smTID sNxtTID;

    IDE_ERROR( aSlotHeader != NULL );

    SM_SET_SCN( &sCreateSCN, &(aSlotHeader->mCreateSCN) );
    SM_SET_SCN( &sLimitSCN,  &(aSlotHeader->mLimitSCN) );

    SMX_GET_SCN_AND_TID( sCreateSCN, sRowSCN, sRowTID );
    SMX_GET_SCN_AND_TID( sLimitSCN,  sNxtSCN, sNxtTID );

    ideLog::log( SM_TRC_LOG_LEVEL_MPAGE,
                 "Slot Ptr       : 0x%X\n"
                 "Slot CreateSCN : 0x%llX\n"
                 "Slot CreateTID :   %llu\n"
                 "Slot LimitSCN  : 0x%llX\n"
                 "Slot LimitTID  :   %llu\n"
                 "Slot NextOID   : 0x%llX\n"
                 "Slot Offset    : %llu\n"
                 "Slot Flag      : %llX\n",
                 aSlotHeader,
                 SM_SCN_TO_LONG( sRowSCN ),
                 sRowTID,
                 SM_SCN_TO_LONG( sNxtSCN ),
                 sNxtTID,
                 SMP_SLOT_GET_NEXT_OID( aSlotHeader ),
                 (ULong)SMP_SLOT_GET_OFFSET( aSlotHeader ),
                 (UInt)SMP_SLOT_GET_FLAGS( aSlotHeader ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * DB에서 Page들을 할당받는다.
 *
 * fixed slot을 위한 persistent page를 system으로부터 할당받는다.
 *
 * aTrans      : 작업하는 트랜잭션 객체
 * aFixedEntry : 할당받을 PageListEntry
 ***********************************************************************/
IDE_RC svpFixedPageList::allocPersPages( void*             aTrans,
                                         scSpaceID         aSpaceID,
                                         smpPageListEntry* aFixedEntry )
{
    UInt                     sState = 0;
    UInt                     sPageListID;
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
    svmPCH                 * sPCH;
    UInt                     sAllocPageCnt = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );

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

    IDE_TEST( smLayerCallback::findVolPrivatePageList( aTrans,
                                                       aFixedEntry->mTableOID,
                                                       &sPrivatePageList )
              != IDE_SUCCESS );

    if(sPrivatePageList == NULL)
    {
        // 기존에 PrivatePageList가 없었다면 새로 생성한다.
        IDE_TEST( smLayerCallback::createVolPrivatePageList( aTrans,
                                                             aFixedEntry->mTableOID,
                                                             &sPrivatePageList )
                  != IDE_SUCCESS );
    }

    // DB에서 받아오고
    IDE_TEST( svmManager::allocatePersPageList( aTrans,
                                                aSpaceID,
                                                SMP_ALLOCPAGECOUNT_FROMDB,
                                                (void **)&sAllocPageHead,
                                                (void **)&sAllocPageTail,
                                                &sAllocPageCnt )
              != IDE_SUCCESS);

    IDE_DASSERT( svpAllocPageList::isValidPageList(
                     aSpaceID,
                     sAllocPageHead->mHeader.mSelfPageID,
                     sAllocPageTail->mHeader.mSelfPageID,
                     sAllocPageCnt )
                 == ID_TRUE );
    sState = 1;

    // 할당받은 HeadPage를 PrivatePageList에 등록한다.
    if( sPrivatePageList->mFixedFreePageTail == NULL )
    {
        IDE_DASSERT( sPrivatePageList->mFixedFreePageHead == NULL );

        sPrivatePageList->mFixedFreePageHead =
            svpFreePageList::getFreePageHeader(
                aSpaceID,
                sAllocPageHead->mHeader.mSelfPageID);
    }
    else
    {
        IDE_DASSERT( sPrivatePageList->mFixedFreePageHead != NULL );

        sPrivatePageList->mFixedFreePageTail->mFreeNext =
            svpFreePageList::getFreePageHeader(
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

        // PersPageHeader 초기화하고 (FreeSlot들을 연결한다.)
        initializePage( aFixedEntry->mSlotSize,
                        aFixedEntry->mSlotCount,
                        sPageListID,
                        aFixedEntry->mTableOID,
                        sPagePtr );

        sPCH = (svmPCH*)(smmManager::getPCH( aSpaceID,
                                             sPagePtr->mHeader.mSelfPageID ));
        sPCH->mNxtScanPID       = SM_NULL_PID;
        sPCH->mPrvScanPID       = SM_NULL_PID;
        sPCH->mModifySeqForScan = 0;

        // FreePageHeader 초기화하고
        svpFreePageList::initializeFreePageHeader(
            svpFreePageList::getFreePageHeader(aSpaceID, sPagePtr) );

        // FreeSlotList를 Page에 등록한다.
        svpFreePageList::initializeFreeSlotListAtPage( aSpaceID,
                                                       aFixedEntry,
                                                       sPagePtr );

        sNextPageID = sPagePtr->mHeader.mNextPageID;

        // FreePageHeader를 PrivatePageList 링크를 구성한다.
        svpFreePageList::addFreePageToPrivatePageList( aSpaceID,
                                                       sPagePtr->mHeader.mSelfPageID,
                                                       sPrevPageID,
                                                       sNextPageID );

        sPrevPageID = sPagePtr->mHeader.mSelfPageID;
    }

    IDE_DASSERT( sPagePtr == sAllocPageTail );

    // TailPage를 PrivatePageList에 등록한다.
    sPrivatePageList->mFixedFreePageTail =
        svpFreePageList::getFreePageHeader(aSpaceID,
                                           sAllocPageTail->mHeader.mSelfPageID);

    // 전체를 AllocPageList 등록
    IDE_TEST(sAllocPageList->mMutex->lock(NULL) != IDE_SUCCESS);

    IDE_TEST( svpAllocPageList::addPageList( aSpaceID,
                                             sAllocPageList,
                                             sAllocPageHead,
                                             sAllocPageTail,
                                             sAllocPageCnt )
              != IDE_SUCCESS );

    IDE_TEST(sAllocPageList->mMutex->unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sState)
    {
        case 1:
            /* rollback이 일어나면 안된다. 무조건 성공해야 한다. */
            /* BUGBUG assert 말고 다른 처리 방안 고려해볼 것 */
            IDE_ASSERT(0);
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
 ***********************************************************************/
IDE_RC svpFixedPageList::allocSlot( void*             aTrans,
                                    scSpaceID         aSpaceID,
                                    void*             aTableInfoPtr,
                                    smOID             aTableOID,
                                    smpPageListEntry* aFixedEntry,
                                    SChar**           aRow,
                                    smSCN             aInfinite,
                                    ULong             aMaxRow,
                                    SInt              aOptFlag)
{
    UInt                sPageListID;
    smOID               sRecOID;
    ULong               sRecordCnt;
    scPageID            sPageID;
    smpFreeSlotHeader * sCurFreeSlotHeader;
    smpSlotHeader     * sCurSlotHeader;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aFixedEntry != NULL );

    IDE_ASSERT( aRow != NULL );
    IDE_ASSERT( aTableOID == aFixedEntry->mTableOID );

    sPageID     = SM_NULL_PID;
    smLayerCallback::allocRSGroupID( aTrans, &sPageListID );

    /* BUG-19573 Table의 Max Row가 Disable되어있으면 이 값을 insert시에
     *           Check하지 않아야 한다. */
    if( (aTableInfoPtr != NULL) && (aMaxRow != ID_ULONG_MAX) )
    {
        // BUG-47368: allocSlot, mutex 제거, atomic 연산으로 변경
        sRecordCnt = idCore::acpAtomicGet64( &(aFixedEntry->mRuntimeEntry->mInsRecCnt) );
        sRecordCnt += smLayerCallback::getRecCntOfTableInfo( aTableInfoPtr );   

        IDE_TEST_RAISE((aMaxRow <= sRecordCnt) &&
                       ((aOptFlag & SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT)
                        == SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT),
                       err_exceed_maxrow);
    }

    /* need to alloc page from svmManager */
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

        setAllocatedSlot( aInfinite, *aRow );
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

        /* svpFixedPageList::setFreeSlot에서 SCN이 Delete Bit가
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
IDE_RC svpFixedPageList::freeSlot( void*             aTrans,
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

inline void svpFixedPageList::initForScan( scSpaceID         aSpaceID,
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
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID, 
                                                sPageID,
                                                (void**)aPage )
                    == IDE_SUCCESS );
        *aRowPtr = aRow + aFixedEntry->mSlotSize;
    }
    else
    {
        sPageID = svpManager::getFirstAllocPageID(aFixedEntry);

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
 * FreePageHeader에서 FreeSlot제거
 *
 * aFreePageHeader : 제거하려는 FreePageHeader
 * aRow            : 제거한 FreeSlot의 Row포인터 반환
 **********************************************************************/

void svpFixedPageList::removeSlotFromFreeSlotList(
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
IDE_RC svpFixedPageList::setFreeSlot( void*          aTrans,
                                      scSpaceID      aSpaceID,
                                      scPageID       aPageID,
                                      SChar*         aRow,
                                      smpTableType   aTableType )
{
    smpSlotHeader       sAfterSlotHeader;
    SInt                sState = 0;
    smpSlotHeader     * sCurSlotHeader;
    smpSlotHeader     * sTmpSlotHeader;
    smOID               sRecOID;

    IDE_DASSERT( ((aTrans != NULL) && (aTableType == SMP_TABLE_NORMAL)) ||
                 ((aTrans == NULL) && (aTableType == SMP_TABLE_TEMP)) );

    IDE_ASSERT( aPageID != SM_NULL_PID );
    IDE_ASSERT( aRow != NULL );
    
    ACP_UNUSED( aTrans );

    sCurSlotHeader = (smpSlotHeader*)aRow;

    if( SMP_SLOT_IS_USED( sCurSlotHeader ) )
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

        if(aTableType == SMP_TABLE_NORMAL)
        {
            IDE_ASSERT( smmManager::getOIDPtr( aSpaceID, 
                                               sRecOID,
                                               (void**)&sTmpSlotHeader )
                        == IDE_SUCCESS );

            IDE_ASSERT( SMP_SLOT_GET_OFFSET( sTmpSlotHeader )
                     == SMP_SLOT_GET_OFFSET( sCurSlotHeader ) );

            IDE_ASSERT( SMP_SLOT_GET_OFFSET( sTmpSlotHeader )
                     == SM_MAKE_OFFSET(sRecOID) );
        }

        // BUG-14373 ager와 seq-iterator와의 동시성 제어
        IDE_TEST(svmManager::holdPageXLatch(aSpaceID, aPageID) != IDE_SUCCESS);
        sState = 2;

        *sCurSlotHeader = sAfterSlotHeader;

        sState = 1;
        IDE_TEST(svmManager::releasePageLatch(aSpaceID, aPageID) != IDE_SUCCESS);

        sState = 0;
    }

    // BUG-37593
    // BUG-49109: ERROR 발생 할 경우 SCN 출력 
    IDE_ERROR_RAISE( SM_SCN_IS_DELETED( sCurSlotHeader->mCreateSCN ) &&
                     SM_SCN_IS_FREE_ROW( sCurSlotHeader->mLimitSCN ) &&
                     SMP_SLOT_HAS_NULL_NEXT_OID( sCurSlotHeader ), ERR_SLOT_SCN );
  
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SLOT_SCN )
    {
        ideLog::log(IDE_ERR_0, "svpFixedPageList::setFreeSlot\n"
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
            IDE_ASSERT( svmManager::releasePageLatch(aSpaceID,aPageID)
                        == IDE_SUCCESS );

        case 1:
        default:
            break;
    }

    IDE_POP();

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
IDE_RC svpFixedPageList::addFreeSlotPending( void*             aTrans,
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
    sFreePageHeader = svpFreePageList::getFreePageHeader(aSpaceID, sPageID);

    IDE_TEST(sFreePageHeader->mMutex.lock(NULL) != IDE_SUCCESS);
    sState = 1;

    // PrivatePageList에서는 FreeSlot되지 않는다.
    IDE_ASSERT(sFreePageHeader->mFreeListID != SMP_PRIVATE_PAGELISTID);

    // FreeSlot을 FreeSlotList에 추가
    addFreeSlotToFreeSlotList(sFreePageHeader, aRow);

    // FreeSlot이 추가된 다음 SizeClass가 변경되었는지 확인하여 조정한다.
    IDE_TEST(svpFreePageList::modifyPageSizeClass( aTrans,
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

void svpFixedPageList::addFreeSlotToFreeSlotList(
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
ULong svpFixedPageList::getRecordCount( smpPageListEntry* aFixedEntry )
{
    ULong sRecordCount;

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
 * PageList의 유효한 레코드 갯수 변경
 *
 * aFixedEntry  : 검색하고자 하는 PageListEntry
 * aRecordCount : 레코드 갯수
 **********************************************************************/
IDE_RC svpFixedPageList::setRecordCount( smpPageListEntry* aFixedEntry,
                                         ULong             aRecordCount )
{
    // BUG-47368: setRecordCount, mutex 제거, atomic 연산으로 변경 
    (void)idCore::acpAtomicSet64( &(aFixedEntry->mRuntimeEntry->mInsRecCnt), aRecordCount );

    return IDE_SUCCESS;
}

/**********************************************************************
 * PageListEntry를 초기화한다.
 *
 * aFixedEntry : 초기화하려는 PageListEntry
 * aTableOID   : PageListEntry의 테이블 OID
 * aSlotSize   : PageListEntry의 SlotSize
 **********************************************************************/
void svpFixedPageList::initializePageListEntry( smpPageListEntry* aFixedEntry,
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
void svpFixedPageList::setAllocatedSlot( smSCN  aInfinite,
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
svpFixedPageList::isValidFreeSlotList(smpFreePageHeader* aFreePageHeader )
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
 * Description: <aSpaceID, aPageID>에 있는 Record Header들을 altibase_boot.log
 *              에 찍는다.
 *
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] Page ID
 * aFxiedEntry - [IN] Page List Entry
 **********************************************************************/
IDE_RC svpFixedPageList::dumpFixedPage( scSpaceID         aSpaceID,
                                        scPageID          aPageID,
                                        smpPageListEntry *aFixedEntry )
{
    UInt                sSlotCnt  = aFixedEntry->mSlotCount;
    UInt                sSlotSize = aFixedEntry->mSlotSize;
    UInt                i;
    smpSlotHeader     * sCurSlotHeader;
    smpSlotHeader     * sNxtSlotHeader;
    UInt                sCurOffset;
    SChar             * sPagePtr;
    smpPersPageHeader * sHeader;

    IDE_ERROR( aFixedEntry != NULL );

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            aPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    sCurOffset     = (UShort)SMP_PERS_PAGE_BODY_OFFSET;
    sHeader        = (smpPersPageHeader*)sPagePtr;
    sNxtSlotHeader = (smpSlotHeader*)(sPagePtr + sCurOffset);

    ideLog::log(IDE_SERVER_0,
                "Volatile FixedPage\n"
                "SpaceID      : %u\n"
                "PageID       : %u\n"
                "SelfPID      : %u\n"
                "PrevPID      : %u\n"
                "NextPID      : %u\n"
                "Type         : %u\n"
                "TableOID     : %lu\n"
                "AllocListID  : %u\n",
                aSpaceID,
                aPageID,
                sHeader->mSelfPageID,
                sHeader->mPrevPageID,
                sHeader->mNextPageID,
                sHeader->mType,
                sHeader->mTableOID,
                sHeader->mAllocListID );

    for( i = 0; i < sSlotCnt; i++)
    {
        sCurSlotHeader = sNxtSlotHeader;

        dumpSlotHeader( sCurSlotHeader );

        sNxtSlotHeader = (smpSlotHeader*)((SChar*)sCurSlotHeader + sSlotSize);
        sCurOffset += sSlotSize;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
