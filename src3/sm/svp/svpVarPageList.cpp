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
#include <ide.h>
#include <smErrorCode.h>
#include <svm.h>
#include <svpVarPageList.h>
#include <svpAllocPageList.h>
#include <svpFreePageList.h>
#include <svpReq.h>

/* BUG-26939 
 * 이 배열의 값들은 다음 구조체의 크기 변경으로 값이 달라질 수 있다.
 *      SMP_PERS_PAGE_BODY_SIZE, smpVarPageHeader, smVCPieceHeader.
 * 또한 컴파일 bit 에 따라서 구조체 크기가 달라지므로 다르게 세팅한다.
 * debug mode에서 이 크기가 변경되지 않는지 initializePageListEntry에서 검사한다.
 * 이 파트가 수정된다면 svp에 똑같은 코드가 있으므로 동일하게 수정해야한다.
 * 값을 아래와 같이 결정한 자세한 이유는 NOK Variable Page slots 문서 참조
 */
#if defined(COMPILE_64BIT)
static UInt gVarSlotSizeArray[SM_VAR_PAGE_LIST_COUNT]=
{
    40, 64, 104, 152, 232, 320, 488, 664, 1000, 1344,
    2024, 2704, 4072, 5432, 8160, 10888, 16344, 32712
};
#else
static UInt gVarSlotSizeArray[SM_VAR_PAGE_LIST_COUNT]=
{
    48, 72, 112, 160, 240, 328, 496, 672, 1008, 1352,
    2032, 2720, 4080, 5448, 8176, 10904, 16360, 32728
};
#endif
/* size 별로 맞는 index를 매칭시켜주는 배열 */
static UChar gAllocArray[SMP_PERS_PAGE_BODY_SIZE] = {0, };

/***********************************************************************
 * Runtime Item을 NULL로 설정한다.
 * DISCARD/OFFLINE Tablespace에 속한 Table들에 대해 수행된다.
 *
 * [IN] aVarEntryCount : 초기화하려는 PageListEntry의 수
 * [IN] aVarEntryArray : 초기화하려는 PageListEntry들의 Array
 ***********************************************************************/
IDE_RC svpVarPageList::setRuntimeNull( UInt              aVarEntryCount,
                                       smpPageListEntry* aVarEntryArray )
{
    UInt i;

    IDE_DASSERT( aVarEntryArray != NULL );

    for ( i=0; i<aVarEntryCount; i++)
    {
        // RuntimeEntry 초기화
        IDE_TEST(svpFreePageList::setRuntimeNull( & aVarEntryArray[i] )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * memory table의  variable page list entry가 포함하는 runtime 정보 초기화
 *
 * aTableOID : PageListEntry가 속하는 테이블 OID
 * aVarEntry : 초기화하려는 PageListEntry
 **********************************************************************/
IDE_RC svpVarPageList::initEntryAtRuntime(
    smOID                  aTableOID,
    smpPageListEntry*      aVarEntry,
    smpAllocPageListEntry* aAllocPageList )
{
    UInt i;

    IDE_DASSERT( aVarEntry != NULL );
    IDE_ASSERT( aTableOID == aVarEntry->mTableOID );
    IDE_DASSERT( aAllocPageList != NULL );

    for( i = 0; i < SM_VAR_PAGE_LIST_COUNT ; i++ )
    {
        // RuntimeEntry 초기화
        IDE_TEST(svpFreePageList::initEntryAtRuntime( &(aVarEntry[i]) )
                 != IDE_SUCCESS);

        aVarEntry[i].mRuntimeEntry->mAllocPageList = aAllocPageList;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * memory table의  variable page list entry가 포함하는 runtime 정보 해제
 *
 * aVarEntry : 해제하려는 PageListEntry
 **********************************************************************/
IDE_RC svpVarPageList::finEntryAtRuntime( smpPageListEntry* aVarEntry )
{
    UInt i;
    UInt sPageListID;

    IDE_DASSERT( aVarEntry != NULL );

    if (aVarEntry->mRuntimeEntry != NULL)
    {
        for( sPageListID = 0;
             sPageListID < SMP_PAGE_LIST_COUNT;
             sPageListID++ )
        {
            // AllocPageList의 Mutex 해제
            IDE_TEST( svpAllocPageList::finEntryAtRuntime(
                          &(aVarEntry->mRuntimeEntry->
                               mAllocPageList[sPageListID]) )
                      != IDE_SUCCESS );
        }

        for( i = 0; i < SM_VAR_PAGE_LIST_COUNT; i++ )
        {
            // RuntimeEntry 제거
            IDE_TEST(svpFreePageList::finEntryAtRuntime(&(aVarEntry[i]))
                     != IDE_SUCCESS);
        }
    }
    else
    {
        /* Memory Table인 경우엔 aVarEntry->mRuntimeEntry가 NULL인 경우
           (OFFLINE/DISCARD)가 있지만 Volatile Table에 대해서는
           aVarEntry->mRuntimeEntry가 이미 NULL인 경우는 없다.
           만약 aVarEntry->mRuntimeEntry가 NULL이라면
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
 * aTrans    : 작업을 수행하는 트랜잭션 객체
 * aTableOID : 제거할 테이블 OID
 * aVarEntry : 제거할 PageListEntry
 ***********************************************************************/
IDE_RC svpVarPageList::freePageListToDB(void*             aTrans,
                                        scSpaceID         aSpaceID,
                                        smOID             aTableOID,
                                        smpPageListEntry* aVarEntry )
{
    UInt i;
    UInt sPageListID;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aVarEntry != NULL );
    IDE_ASSERT( aTableOID == aVarEntry->mTableOID );

    for( i = 0; i < SM_VAR_PAGE_LIST_COUNT; i++ )
    {
        // FreePageList 제거
        svpFreePageList::initializeFreePageListAndPool(&(aVarEntry[i]));
    }

    for( sPageListID = 0;
         sPageListID < SMP_PAGE_LIST_COUNT;
         sPageListID++ )
    {
        // for AllocPageList
        IDE_TEST( svpAllocPageList::freePageListToDB(
                      aTrans,
                      aSpaceID,
                      &(aVarEntry->mRuntimeEntry->mAllocPageList[sPageListID]) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC svpVarPageList::setValue(scSpaceID       aSpaceID,
                                smOID           aPieceOID,
                                const void*     aValue,
                                UInt            aLength)
{
    smVCPieceHeader  *sVCPieceHeader;

    IDE_DASSERT( aPieceOID != SM_NULL_OID );
    IDE_DASSERT( aValue != NULL );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       aPieceOID,
                                       (void**)&sVCPieceHeader )
                == IDE_SUCCESS );

    /* Set Length Of VCPieceHeader*/
    sVCPieceHeader->length = aLength;

    /* Set Value */
    idlOS::memcpy( (SChar*)(sVCPieceHeader + 1), aValue, aLength );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description: Variable Length Data를 읽는다. 만약 길이가 최대 Piece길이 보다
 *              작고 읽어야 할 Piece가 하나의 페이지에 존재한다면 읽어야 할 위치의
 *              첫번째 바이트가 위치한 메모리 포인터를 리턴한다. 그렇지 않을 경우 Row의
 *              값을 복사해서 넘겨준다.
 *
 * aSpaceID     - [IN] Space ID
 * aBeginPos    - [IN] 시작 위치
 * aReadLen     - [IN] 읽을 데이타의 길이
 * aFstPieceOID - [IN] 첫번째 Piece의 OID
 * aBuffer      - [IN] value가 복사됨.
 *
 ***********************************************************************/
SChar* svpVarPageList::getValue( scSpaceID       aSpaceID,
                                 UInt            aBeginPos,
                                 UInt            aReadLen,
                                 smOID           aFstPieceOID,
                                 SChar          *aBuffer )
{
    smVCPieceHeader  *sVCPieceHeader;
    SLong             sRemainedReadSize;
    UInt              sPos;
    SChar            *sRowBuffer;
    SInt              sReadPieceSize;
    SChar            *sRet;

    IDE_ASSERT( aFstPieceOID != SM_NULL_OID );

    /* 첫번째 VC Piece Header를 가져온다. */
    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       aFstPieceOID,
                                       (void**)&sVCPieceHeader )
                == IDE_SUCCESS );

    /* 읽을 첫번째 바이트 위치를 찾는다. */
    sPos = aBeginPos;
    while( sPos >= SMP_VC_PIECE_MAX_SIZE )
    {
        sPos -= sVCPieceHeader->length;

        IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                           sVCPieceHeader->nxtPieceOID,
                                           (void**)&sVCPieceHeader )
                    == IDE_SUCCESS );
    }

    IDE_ASSERT( sPos < SMP_VC_PIECE_MAX_SIZE );

    sRet = svpVarPageList::getPieceValuePtr( sVCPieceHeader, sPos );

    /* 데이타의 길이가 SMP_VC_PIECE_MAX_SIZE보다 작거나 같고 읽을 데이타
     * 가 페이지에 걸쳐서 저장되지 않고 하나의 페이지에 들어간다면 */
    if( sPos + aReadLen > SMP_VC_PIECE_MAX_SIZE )
    {
        sRemainedReadSize = aReadLen;
        sRowBuffer = aBuffer;

        sRet = aBuffer;
        while( sRemainedReadSize > (SLong)SMP_VC_PIECE_MAX_SIZE )
        {
            sReadPieceSize = sVCPieceHeader->length - sPos;
            IDE_ASSERT( sReadPieceSize > 0 );
            // BUG-27649 CodeSonar::Null Pointer Dereference (9)
            IDE_ASSERT( sRowBuffer != NULL );

            sPos = 0;

            idlOS::memcpy(
                sRowBuffer,
                svpVarPageList::getPieceValuePtr( sVCPieceHeader, sPos ),
                sReadPieceSize );
            sRowBuffer += sReadPieceSize;
            sRemainedReadSize -= sReadPieceSize;

            if( sRemainedReadSize != 0 )
            {
                IDE_ASSERT( smmManager::getOIDPtr( 
                                aSpaceID,
                                sVCPieceHeader->nxtPieceOID,
                                (void**)&sVCPieceHeader )
                            == IDE_SUCCESS );
            }
        }

        if( sRemainedReadSize != 0 )
        {
            // BUG-27649 CodeSonar::Null Pointer Dereference (9)
            IDE_ASSERT( sRowBuffer != NULL );

            idlOS::memcpy(
                sRowBuffer,
                svpVarPageList::getPieceValuePtr( sVCPieceHeader, sPos ),
                sRemainedReadSize );

            sRowBuffer += sRemainedReadSize;

            // BUG-27331 CodeSonar::Uninitialized Variable
            sRemainedReadSize = 0;
        }

        /* 버퍼에 복사된 데이타의 길이는 읽을 데이타의 길이와
         * 동일해야 합니다. */
        IDE_ASSERT( (UInt)(sRowBuffer - aBuffer) == aReadLen );
    }

    return sRet;
}

/**********************************************************************
 *  Varialbe Column을 위한 Slot Header를 altibase_sm.log에 덤프한다
 *
 *  aVarSlotHeader : dump할 slot 헤더
 **********************************************************************/
IDE_RC svpVarPageList::dumpVarColumnHeader( smVCPieceHeader *aVCPieceHeader )
{
    IDE_ERROR( aVCPieceHeader != NULL );

    ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                SM_TRC_MPAGE_DUMP_VAR_COL_HEAD,
                (ULong)aVCPieceHeader->nxtPieceOID,
                aVCPieceHeader->length,
                aVCPieceHeader->flag);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**********************************************************************
 * system으로부터 persistent page를 할당받는다.
 *
 * aTrans    : 작업하는 트랜잭션 객체
 * aVarEntry : 할당받을 PageListEntry
 * aIdx      : 할당받을 VarSlot 크기 idx
 **********************************************************************/
IDE_RC svpVarPageList::allocPersPages( void*             aTrans,
                                       scSpaceID         aSpaceID,
                                       smpPageListEntry* aVarEntry,
                                       UInt              aIdx )
{
    UInt                    sState   = 0;
    smpPersPage*            sPagePtr = NULL;
    smpPersPage*            sAllocPageHead;
    smpPersPage*            sAllocPageTail;
    scPageID                sNextPageID;
    UInt                    sPageListID;
#ifdef DEBUG
    smpFreePagePoolEntry*   sFreePagePool;
    smpFreePageListEntry*   sFreePageList;
#endif
    smpAllocPageListEntry*  sAllocPageList;
    smpPrivatePageListEntry* sPrivatePageList = NULL;
    UInt                    sAllocPageCnt = 0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aVarEntry != NULL );
    IDE_DASSERT( aIdx < SM_VAR_PAGE_LIST_COUNT );

    smLayerCallback::allocRSGroupID( aTrans,
                                     &sPageListID );

    sAllocPageList = &(aVarEntry->mRuntimeEntry->mAllocPageList[sPageListID]);

#ifdef DEBUG
    sFreePagePool  = &(aVarEntry->mRuntimeEntry->mFreePagePool);
    sFreePageList  = &(aVarEntry->mRuntimeEntry->mFreePageList[sPageListID]);
#endif
    IDE_DASSERT( sAllocPageList != NULL );
    IDE_DASSERT( sFreePagePool != NULL );
    IDE_DASSERT( sFreePageList != NULL );

    // DB에서 Page들을 할당받으면 FreePageList를
    // Tx's Private Page List에 먼저 등록한 후
    // 트랜잭션이 종료될 때 해당 테이블의 PageListEntry에 등록하게 된다.

    IDE_TEST( smLayerCallback::findVolPrivatePageList( aTrans,
                                                       aVarEntry->mTableOID,
                                                       &sPrivatePageList )
              != IDE_SUCCESS );

    if(sPrivatePageList == NULL)
    {
        // 기존에 PrivatePageList가 없었다면 새로 생성한다.
        IDE_TEST( smLayerCallback::createVolPrivatePageList( aTrans,
                                                             aVarEntry->mTableOID,
                                                             &sPrivatePageList )
                  != IDE_SUCCESS );
    }

    // [1] svmManager로부터 page 할당
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
    IDE_DASSERT( sPrivatePageList->mVarFreePageHead[aIdx] == NULL );

    sPrivatePageList->mVarFreePageHead[aIdx] =
        svpFreePageList::getFreePageHeader(aSpaceID,
                                           sAllocPageHead->mHeader.mSelfPageID);

    // [2] page 초기화
    sNextPageID = sAllocPageHead->mHeader.mSelfPageID;

    while(sNextPageID != SM_NULL_PID)
    {
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                                sNextPageID,
                                                (void**)&sPagePtr )
                    == IDE_SUCCESS );

        // PersPageHeader 초기화하고 (FreeSlot들을 연결한다.)
        initializePage( aIdx,
                        sPageListID,
                        aVarEntry->mSlotSize,
                        aVarEntry->mSlotCount,
                        aVarEntry->mTableOID,
                        sPagePtr );


        // FreePageHeader 초기화하고
        svpFreePageList::initializeFreePageHeader(
            svpFreePageList::getFreePageHeader(aSpaceID, sPagePtr) );

        // FreeSlotList를 Page에 등록한다.
        svpFreePageList::initializeFreeSlotListAtPage( aSpaceID,
                                                       aVarEntry,
                                                       sPagePtr );

        sNextPageID = sPagePtr->mHeader.mNextPageID;

        // FreePageHeader를 PrivatePageList로 연결한다.
        // PrivatePageList에 Var영역은 단방향리스트이기때문에 Prev가 NULL로 셋팅된다.
        svpFreePageList::addFreePageToPrivatePageList( aSpaceID,
                                                       sPagePtr->mHeader.mSelfPageID,
                                                       SM_NULL_PID,  // Prev
                                                       sNextPageID );
    }

    // [3] AllocPageList 등록
    IDE_TEST( sAllocPageList->mMutex->lock(NULL) != IDE_SUCCESS );

    IDE_TEST( svpAllocPageList::addPageList( aSpaceID,
                                             sAllocPageList,
                                             sAllocPageHead,
                                             sAllocPageTail,
                                             sAllocPageCnt )
              != IDE_SUCCESS );

    IDE_TEST(sAllocPageList->mMutex->unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

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

    return IDE_FAILURE;
}

/***********************************************************************
 * var slot을 할당한다.
 *
 * aTrans    : 작업하려는 트랜잭션 객체
 * aTableOID : 할당하려는 테이블의 OID
 * aVarEntry : 할당하려는 PageListEntry
 * aPieceSize: Variable Column을 구성할 Piece의 크기
 * aNxtPieceOID  : 할당할 Piece의 nextPieceOID
 * aPieceOID : 할당된 Piece의 OID
 * aPiecePtr      : 할당해서 반환하려는 Row 포인터
 ***********************************************************************/
IDE_RC svpVarPageList::allocSlot( void*              aTrans,
                                  scSpaceID          aSpaceID,
                                  smOID              aTableOID,
                                  smpPageListEntry*  aVarEntry,
                                  UInt               aPieceSize,
                                  smOID              aNxtPieceOID,
                                  smOID*             aPieceOID,
                                  SChar**            aPiecePtr)
{
    UInt              sIdx   = 0;
    UInt              sPageListID;
    smVCPieceHeader*  sCurVCPieceHeader = NULL;
    smVCPieceHeader   sAfterVCPieceHeader;
    smpPageListEntry* sVarPageList = NULL;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aVarEntry != NULL );
    IDE_DASSERT( aPieceOID != NULL );
    IDE_DASSERT( aPiecePtr != NULL );

    /* ----------------------------
     * 크기에 맞는 variable page list 선택
     * ---------------------------*/
    IDE_TEST( calcVarIdx( aPieceSize, &sIdx ) != IDE_SUCCESS );

    IDE_ASSERT( sIdx < SM_VAR_PAGE_LIST_COUNT );
    IDE_ASSERT( aTableOID == aVarEntry[sIdx].mTableOID );

    sVarPageList = aVarEntry + sIdx;

    smLayerCallback::allocRSGroupID( aTrans,
                                     &sPageListID );

    while(1)
    {
        // 1) Tx's PrivatePageList에서 찾기
        IDE_TEST( tryForAllocSlotFromPrivatePageList( aTrans,
                                                      aSpaceID,
                                                      aTableOID,
                                                      sIdx,
                                                      aPieceOID,
                                                      aPiecePtr )
                  != IDE_SUCCESS );

        if(*aPiecePtr != NULL)
        {
            break;
        }

        // 2) FreePageList에서 찾기
        IDE_TEST( tryForAllocSlotFromFreePageList( aTrans,
                                                   aSpaceID,
                                                   aVarEntry,
                                                   sIdx,
                                                   sPageListID,
                                                   aPieceOID,
                                                   aPiecePtr )
                  != IDE_SUCCESS );

        if( *aPiecePtr != NULL)
        {
            break;
        }

        // 3) system으로부터 page를 할당받는다.
        IDE_TEST( allocPersPages( aTrans,
                                  aSpaceID,
                                  sVarPageList,
                                  sIdx )
                  != IDE_SUCCESS );
    }

    sCurVCPieceHeader = (smVCPieceHeader*)(*aPiecePtr);

    //update var row head
    sAfterVCPieceHeader      = *sCurVCPieceHeader;
    sAfterVCPieceHeader.flag = SM_VCPIECE_FREE_NO |
                               SM_VCPIECE_TYPE_OTHER;

    /* BUG-15354: [A4] SM VARCHAR 32K: Varchar저장시 PieceHeader에 대한 logging이
     * 누락되어 PieceHeader에 대한 Redo, Undo가 되지않음. */
    sAfterVCPieceHeader.length      = aPieceSize;
    sAfterVCPieceHeader.nxtPieceOID = aNxtPieceOID;

    *sCurVCPieceHeader = sAfterVCPieceHeader;

    // Record Count 증가 // BUG-47706
    idCore::acpAtomicInc64( &(sVarPageList->mRuntimeEntry->mInsRecCnt) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Tx's PrivatePageList의 FreePage로부터 Slot를 할당할 수 있을지 검사하고
 * 가능하면 할당한다.
 *
 * aTrans    : 작업하는 트랜잭션 객체
 * aTableOID : 할당하려는 테이블 OID
 * aIdx      : 할당하려는 VarPage의 Idx값
 * aPieceOID : 할당해서 반환하려는 Slot 의 OID
 * aPiecePtr : 할당해서 반환하려는 Slot 포인터
 **********************************************************************/

IDE_RC svpVarPageList::tryForAllocSlotFromPrivatePageList(
    void*       aTrans,
    scSpaceID   aSpaceID,
    smOID       aTableOID,
    UInt        aIdx,
    smOID*      aPieceOID,
    SChar**     aPiecePtr )
{
    smpPrivatePageListEntry* sPrivatePageList = NULL;
    smpFreePageHeader*       sFreePageHeader;

    IDE_DASSERT(aTrans != NULL);
    IDE_DASSERT(aIdx < SM_VAR_PAGE_LIST_COUNT);
    IDE_DASSERT(aPiecePtr != NULL);

    *aPiecePtr = NULL;

    IDE_TEST( smLayerCallback::findVolPrivatePageList( aTrans,
                                                       aTableOID,
                                                       &sPrivatePageList )
              != IDE_SUCCESS );

    if(sPrivatePageList != NULL)
    {
        sFreePageHeader = sPrivatePageList->mVarFreePageHead[aIdx];

        if(sFreePageHeader != NULL)
        {
            IDE_DASSERT(sFreePageHeader->mFreeSlotCount > 0);

            removeSlotFromFreeSlotList( aSpaceID,
                                        sFreePageHeader,
                                        aPieceOID,
                                        aPiecePtr );

            if(sFreePageHeader->mFreeSlotCount == 0)
            {
                sPrivatePageList->mVarFreePageHead[aIdx] =
                    sFreePageHeader->mFreeNext;

                svpFreePageList::initializeFreePageHeader(sFreePageHeader);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * FreePageList나 FreePagePool에서 FreeSlot을 할당할 수 있는지 시도
 * 할당이 되면 aPiecePtr로 반환하고 할당할 FreeSlot이 없다면 aPiecePtr를 NULL로 반환
 *
 * aTrans      : 작업하는 트랜잭션 객체
 * aVarEntry   : Slot을 할당하려는 PageListEntry
 * aPageListID : Slot을 할당하려는 PageListID
 * aPieceOID   : 할당해서 반환하려는 Piece OID
 * aPiecePtr   : 할당해서 반환하려는 Piece Ptr
 ***********************************************************************/
IDE_RC svpVarPageList::tryForAllocSlotFromFreePageList(
    void*             aTrans,
    scSpaceID         aSpaceID,
    smpPageListEntry* aVarEntryArray,
    UInt              aIdx,
    UInt              aPageListID,
    smOID*            aPieceOID,
    SChar**           aPiecePtr )
{
    UInt                  sState = 0;
    UInt                  sSizeClassID;
    idBool                sIsPageAlloced;
    smpFreePageHeader*    sFreePageHeader;
    smpFreePageListEntry* sFreePageList;
    UInt                  sSizeClassCount;
    smpPageListEntry    * sVarEntry = NULL;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aVarEntryArray != NULL );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_DASSERT( aPiecePtr != NULL );

    sVarEntry     = aVarEntryArray + aIdx;
    sFreePageList = &(sVarEntry->mRuntimeEntry->mFreePageList[aPageListID]);

    IDE_DASSERT( sFreePageList != NULL );

    *aPiecePtr = NULL;
    *aPieceOID = SM_NULL_OID;

    sSizeClassCount = SMP_SIZE_CLASS_COUNT( sVarEntry->mRuntimeEntry );

    while(1)
    {
        // FreePageList의 SizeClass를 순회하면서 tryAllocSlot한다.
        for( sSizeClassID = 0;
             sSizeClassID < sSizeClassCount;
             sSizeClassID++ )
        {
            sFreePageHeader  = sFreePageList->mHead[sSizeClassID];

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

                    removeSlotFromFreeSlotList(aSpaceID,
                                               sFreePageHeader,
                                               aPieceOID,
                                               aPiecePtr);

                    // FreeSlot을 할당한 Page의 SizeClass가 변경되었는지
                    // 확인하여 조정
                    IDE_TEST(svpFreePageList::modifyPageSizeClass(
                                 aTrans,
                                 sVarEntry,
                                 sFreePageHeader)
                             != IDE_SUCCESS);

                    sState = 0;
                    IDE_TEST(sFreePageHeader->mMutex.unlock() != IDE_SUCCESS);

                    IDE_CONT(normal_case);
                }

                sState = 0;
                IDE_TEST(sFreePageHeader->mMutex.unlock() != IDE_SUCCESS);

                // 해당 Page가 변경된 것이라면 List에서 다시 Head를 가져온다.
                sFreePageHeader = sFreePageList->mHead[sSizeClassID];
            }
        }

        // FreePageList에서 FreeSlot을 찾지 못했다면
        // FreePagePool에서 확인하여 가져온다.

        IDE_TEST( svpFreePageList::tryForAllocPagesFromPool( sVarEntry,
                                                             aPageListID,
                                                             &sIsPageAlloced )
                  != IDE_SUCCESS );

        if ( sIsPageAlloced == ID_FALSE )
        {
            /* BUG-47358
               FreePagePool에서 페이지를 가져오지 못했다면,
               (Slot 크기가) 다른 Variable Page의 FreePagePool에서 페이지를 가져온다. */
            tryForAllocPagesFromOtherPools( aSpaceID,
                                            aVarEntryArray,
                                            aIdx,
                                            aPageListID,
                                            &sIsPageAlloced );

            if ( sIsPageAlloced == ID_FALSE )
            {
                /* Pool에서 못가져왔다. */
                IDE_CONT(normal_case);
            }
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

/***********************************************************************
 * slot을 free 한다.
 *
 * BUG-14093 Ager Tx가 freeSlot한 것을 commit되지 않은 상황에서
 *           다른 Tx가 할당받아 사용했을때 서버 사망시 문제발생
 *           따라서 Ager Tx가 Commit이후에 FreeSlot을 FreeSlotList에 매단다.
 *
 * aTrans     : 작업을 수행하는 트랜잭션 객체
 * aVarEntry  : aPiecePtr가 속한 PageListEntry
 * aPieceOID  : free하려는 Piece OID
 * aPiecePtr  : free하려는 Piece Ptr
 * aTableType : Temp Table의 slot인지에 대한 여부
 ***********************************************************************/
IDE_RC svpVarPageList::freeSlot( void*             aTrans,
                                 scSpaceID         aSpaceID,
                                 smpPageListEntry* aVarEntry,
                                 smOID             aPieceOID,
                                 SChar*            aPiecePtr,
                                 smpTableType      aTableType,
                                 smSCN             aSCN )
{

    UInt                sIdx;
    scPageID            sPageID;
    smpPageListEntry  * sVarPageList;
    SChar             * sPagePtr;

    IDE_DASSERT( ((aTrans != NULL) && (aTableType == SMP_TABLE_NORMAL)) ||
                 ((aTrans == NULL) && (aTableType == SMP_TABLE_TEMP)) );

    IDE_DASSERT( aVarEntry != NULL );
    IDE_DASSERT( aPiecePtr != NULL );

    /* ----------------------------
     * BUG-14093
     * freeSlot에서는 slot에 대한 Free작업만 수행하고
     * ager Tx가 commit한 이후에 addFreeSlotPending을 수행한다.
     * ---------------------------*/

    sPageID        = SM_MAKE_PID(aPieceOID);
    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            sPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    sIdx = getVarIdx( sPagePtr );

    IDE_ASSERT(sIdx < SM_VAR_PAGE_LIST_COUNT);

    sVarPageList    = aVarEntry + sIdx;

    IDE_TEST( setFreeSlot( aTrans,
                           sPageID,
                           aPieceOID,
                           aPiecePtr,
                           aTableType )
              != IDE_SUCCESS );

    // Record Count 조정 // BUG-47706
    idCore::acpAtomicInc64( &(sVarPageList->mRuntimeEntry->mDelRecCnt) );

    if(aTableType == SMP_TABLE_NORMAL)
    {
        // BUG-14093 freeSlot하는 ager가 commit하기 전에는
        //           freeSlotList에 매달지 않고 ager TX가
        //           commit 이후에 매달도록 OIDList에 추가한다.
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aVarEntry->mTableOID,
                                           aPieceOID,
                                           aSpaceID,
                                           SM_OID_TYPE_FREE_VAR_SLOT,
                                           aSCN )
                  != IDE_SUCCESS );
    }
    else
    {
        // TEMP Table은 바로 FreeSlotList에 추가한다.
        IDE_TEST( addFreeSlotPending(aTrans,
                                     aSpaceID,
                                     aVarEntry,
                                     aPieceOID,
                                     aPiecePtr)
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Page를 초기화 한다.
 * Page내의 모든 Slot들도 초기화하며 Next 링크를 구성한다.
 *
 * aIdx        : Page의 VarEntry의 Idx 값
 * aPageListID : Page가 속할 PageListID
 * aSlotSize   : Page에 들어가는 Slot의 크기
 * aSlotCount  : Page내의 모든 Slot 갯수
 * aPage       : 초기화할 Page
 ***********************************************************************/
void svpVarPageList::initializePage( vULong       aIdx,
                                     UInt         aPageListID,
                                     vULong       aSlotSize,
                                     vULong       aSlotCount,
                                     smOID        aTableOID,
                                     smpPersPage* aPage )
{
    UInt                   i;
    smVCPieceHeader*       sCurVCFreePieceHeader = NULL;
    smVCPieceHeader*       sNxtVCFreePieceHeader;
    smpVarPageHeader*      sVarPageHeader;

    // BUG-26937 CodeSonar::NULL Pointer Dereference (4)
    IDE_DASSERT( aIdx < SM_VAR_PAGE_LIST_COUNT );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_DASSERT( aSlotSize > 0 );
    IDE_ASSERT( aSlotCount > 0 );
    IDE_DASSERT( aPage != NULL );

    sVarPageHeader       = (smpVarPageHeader*)(aPage->mBody);
    sVarPageHeader->mIdx = aIdx;

    aPage->mHeader.mType        = SMP_PAGETYPE_VAR;
    aPage->mHeader.mAllocListID = aPageListID;
    aPage->mHeader.mTableOID    = aTableOID;

    sNxtVCFreePieceHeader   = (smVCPieceHeader*)(sVarPageHeader + 1);

    // Variable Page에 속해있는 모든 VC Piece를 free로 Setting.
    for( i = 0; i < aSlotCount; i++ )
    {
        sCurVCFreePieceHeader = sNxtVCFreePieceHeader;

        // Set VC Free Piece
        sCurVCFreePieceHeader->flag = SM_VCPIECE_FREE_OK ;

        sNxtVCFreePieceHeader = (smVCPieceHeader*)((SChar*)sCurVCFreePieceHeader + aSlotSize);
        sCurVCFreePieceHeader->nxtPieceOID  = (smOID)sNxtVCFreePieceHeader;
    }

    // 마지막 Next = NULL
    sCurVCFreePieceHeader->nxtPieceOID = (smOID)NULL;

    IDL_MEM_BARRIER;

    return;
}

/**********************************************************************
 * FreeSlot 정보를 기록한다.
 *
 * aTrans      : 작업하려는 트랜잭션 객체
 * aPageID     : FreeSlot추가하려는 PageID
 * aVCPieceOID : Free할려는 Piece OID
 * aVCPiecePtr : Free할려는 Piece Ptr
 * aTableType  : Temp테이블인지 여부
 **********************************************************************/
IDE_RC svpVarPageList::setFreeSlot( void        * aTrans,
                                    scPageID      aPageID,
                                    smOID         aVCPieceOID,
                                    SChar       * aVCPiecePtr,
                                    smpTableType  aTableType )
{
    smVCPieceHeader* sCurFreeVCPieceHdr;

    IDE_ASSERT( ((aTrans != NULL) && (aTableType == SMP_TABLE_NORMAL)) ||
                ((aTrans == NULL) && (aTableType == SMP_TABLE_TEMP)) );

    IDE_ASSERT( aPageID     != SM_NULL_PID );
    IDE_ASSERT( aVCPieceOID != SM_NULL_OID );
    IDE_DASSERT( aVCPiecePtr != NULL );

    sCurFreeVCPieceHdr = (smVCPieceHeader*)aVCPiecePtr;
    sCurFreeVCPieceHdr->nxtPieceOID = SM_NULL_OID;

    /* Variable Column은 Transaction이 Commit이전에 Variable Column삭제시
       미리 Variable Column의 모든 VC Piece의 Flag에 Free되었다고 표시한다.
       하지만 Transaction들은 Variable Column이 속한 Fixed Row를 통해 자신이
       이 Variable Column을 읽어야 할지 결정하기 때문에 Transaction Commit이전에
       Flag를 설정하여도 문제가 안된다. 이 Flag를 참조하는 것은 refine시 Free
       VC Piece List를 구성할 때이다. 오직 Transaction은 Alloc, Free때 Flag를
       Setting만하지 참조하지는 않는다.*/
    if( (sCurFreeVCPieceHdr->flag & SM_VCPIECE_FREE_MASK)
        == SM_VCPIECE_FREE_NO )
    {
        sCurFreeVCPieceHdr = (smVCPieceHeader*)aVCPiecePtr;

        // Set Variable Column To be Free.
        sCurFreeVCPieceHdr->flag = SM_VCPIECE_FREE_OK;
    }

    return IDE_SUCCESS;
}

/**********************************************************************
 * 실제 FreeSlot을 FreeSlotList에 추가한다.
 *
 * BUG-14093 Commit이후에 FreeSlot을 실제 FreeSlotList에 매단다.
 *
 * aTrans    : 작업하는 트랜잭션 객체
 * aVarEntry : FreeSlot이 속한 PageListEntry
 * aPieceOID : FreeSlot의 OID
 * aPiecePtr : FreeSlot의 Row 포인터
 **********************************************************************/

IDE_RC svpVarPageList::addFreeSlotPending( void*             aTrans,
                                           scSpaceID         aSpaceID,
                                           smpPageListEntry* aVarEntry,
                                           smOID             aPieceOID,
                                           SChar*            aPiecePtr )
{
    UInt               sState = 0;
    UInt               sIdx;
    scPageID           sPageID;
    SChar            * sPagePtr;
    smpFreePageHeader* sFreePageHeader;

    IDE_DASSERT(aVarEntry != NULL);
    IDE_DASSERT(aPiecePtr != NULL);

    sPageID         = SM_MAKE_PID(aPieceOID);
    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            sPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    sIdx            = getVarIdx( sPagePtr );
    sFreePageHeader = svpFreePageList::getFreePageHeader(aSpaceID, sPageID);

    IDE_ASSERT(sIdx < SM_VAR_PAGE_LIST_COUNT);

    IDE_TEST(sFreePageHeader->mMutex.lock(NULL) != IDE_SUCCESS);
    sState = 1;

    // PrivatePageList에서는 FreeSlot되지 않는다.
    IDE_ASSERT(sFreePageHeader->mFreeListID != SMP_PRIVATE_PAGELISTID);

    // FreeSlot을 FreeSlotList에 추가
    addFreeSlotToFreeSlotList(aSpaceID, sPageID, aPiecePtr);

    // FreeSlot이 추가된 다음 SizeClass가 변경되었는지 확인하여 조정한다.
    IDE_TEST(svpFreePageList::modifyPageSizeClass( aTrans,
                                                   aVarEntry + sIdx,
                                                   sFreePageHeader )
             != IDE_SUCCESS);

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
 * FreePageHeader에서 FreeSlot제거
 *
 * 해당 Page에 Lock을 잡고 들어와야 한다.
 *
 * aFreePageHeader : 제거하려는 FreePageHeader
 * aPieceOID       : 제거한 Variable Column Piece OID 반환
 * aPiecePtr       : 제거한 Free VC Piece의 포인터 반환
 **********************************************************************/
void svpVarPageList::removeSlotFromFreeSlotList(
                                            scSpaceID          aSpaceID,
                                            smpFreePageHeader* aFreePageHeader,
                                            smOID*             aPieceOID,
                                            SChar**            aPiecePtr )
{
    smVCPieceHeader* sFreeVCPieceHdr;
    SChar          * sPagePtr;

    IDE_DASSERT(aFreePageHeader != NULL);
    IDE_DASSERT(aFreePageHeader->mFreeSlotCount > 0);
    IDE_DASSERT(aPieceOID != NULL);
    IDE_DASSERT(aPiecePtr != NULL);

    IDE_DASSERT( isValidFreeSlotList(aFreePageHeader) == ID_TRUE );

    sFreeVCPieceHdr = (smVCPieceHeader*)(aFreePageHeader->mFreeSlotHead);

    if ( (sFreeVCPieceHdr->flag & SM_VCPIECE_FREE_MASK)
         == SM_VCPIECE_FREE_NO )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_FATAL, SM_TRC_MPAGE_REMOVE_VAR_PAGE_FATAL1);
        ideLog::log(SM_TRC_LOG_LEVEL_FATAL, SM_TRC_MPAGE_REMOVE_VAR_PAGE_FATAL2);
        (void)dumpVarColumnHeader( sFreeVCPieceHdr );

        IDE_ASSERT(0);
    }

    IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                            aFreePageHeader->mSelfPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );
    *aPieceOID = SM_MAKE_OID(aFreePageHeader->mSelfPageID,
                             (SChar*)sFreeVCPieceHdr - (SChar*)sPagePtr);

    *aPiecePtr = (SChar*)sFreeVCPieceHdr;

    aFreePageHeader->mFreeSlotCount--;

    if(sFreeVCPieceHdr->nxtPieceOID == (smOID)NULL)
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

    aFreePageHeader->mFreeSlotHead = (SChar*)(sFreeVCPieceHdr->nxtPieceOID);

    IDE_DASSERT( isValidFreeSlotList(aFreePageHeader) == ID_TRUE );

    return;
}

/***********************************************************************
 * nextOIDall을 위해 aPiecePtr의  Page와 Next Piece Ptr, OID를 구한다.
 * aPiecePtr이 만약 NULL이라면 aVarEntry에서 첫번째 Allocated Row를 찾는다.
 *
 * aVarEntry : 순회하려는 PageListEntry
 * aPieceOID : Current Piece OID
 * aPiecePtr : Current Piece Ptr
 * aPage     : aPiecePtr이 속한 Page를 찾아서 반환
 * aNxtPieceOID : Next Piece OID
 * aPiecePtrPtr : Next Piece Ptr
 ***********************************************************************/
void svpVarPageList::initForScan( scSpaceID         aSpaceID,
                                  smpPageListEntry* aVarEntry,
                                  smOID             aPieceOID,
                                  SChar*            aPiecePtr,
                                  smpPersPage**     aPage,
                                  smOID*            aNxtPieceOID,
                                  SChar**           aNxtPiecePtr )
{
    UInt             sIdx;
    scPageID         sPageID;

    IDE_DASSERT( aVarEntry != NULL );
    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( aNxtPiecePtr != NULL );

    *aPage   = NULL;
    *aNxtPiecePtr = NULL;

    if(aPiecePtr != NULL)
    {
        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                                SM_MAKE_PID(aPieceOID),
                                                (void**)aPage )
                    == IDE_SUCCESS );
        sIdx     = getVarIdx( *aPage );
        *aNxtPiecePtr = aPiecePtr + aVarEntry[sIdx].mSlotSize;
        *aNxtPieceOID = aPieceOID + aVarEntry[sIdx].mSlotSize;
    }
    else
    {
        sPageID = svpManager::getFirstAllocPageID(aVarEntry);

        if(sPageID != SM_NULL_PID)
        {
            IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                                    sPageID,
                                                    (void**)aPage )
                        == IDE_SUCCESS );
            *aNxtPiecePtr = (SChar *)((*aPage)->mBody + ID_SIZEOF(smpVarPageHeader));
            *aNxtPieceOID = SM_MAKE_OID(sPageID,
                                        (SChar*)(*aNxtPiecePtr) - (SChar*)((*aPage)));
        }
        else
        {
            /* Allocate된 페이지가 존재하지 않는다.*/
        }
    }
}

/**********************************************************************
 * FreePageHeader에 FreeSlot추가
 *
 * aPageID    : FreeSlot추가하려는 PageID
 * aPiecePtr  : FreeSlot의 Row 포인터
 **********************************************************************/

void svpVarPageList::addFreeSlotToFreeSlotList( scSpaceID aSpaceID,
                                                scPageID  aPageID,
                                                SChar*    aPiecePtr )
{
    smpFreePageHeader *sFreePageHeader;
    smVCPieceHeader   *sCurVCPieceHdr;
    smVCPieceHeader   *sTailVCPieceHdr;

    IDE_DASSERT( aPageID != SM_NULL_PID );
    IDE_DASSERT( aPiecePtr != NULL );

    sFreePageHeader = svpFreePageList::getFreePageHeader(aSpaceID, aPageID);
    sCurVCPieceHdr = (smVCPieceHeader*)aPiecePtr;

    IDE_DASSERT( isValidFreeSlotList(sFreePageHeader) == ID_TRUE );

    sCurVCPieceHdr->nxtPieceOID = (smOID)NULL;
    sTailVCPieceHdr = (smVCPieceHeader*)sFreePageHeader->mFreeSlotTail;

    if(sTailVCPieceHdr == NULL)
    {
        IDE_DASSERT( sFreePageHeader->mFreeSlotHead == NULL );

        sFreePageHeader->mFreeSlotHead = aPiecePtr;
    }
    else
    {
        sTailVCPieceHdr->nxtPieceOID = (smOID)aPiecePtr;
    }

    sFreePageHeader->mFreeSlotTail = aPiecePtr;
    sFreePageHeader->mFreeSlotCount++;

    IDE_DASSERT( isValidFreeSlotList(sFreePageHeader) == ID_TRUE );

    return;
}

/**********************************************************************
 * PageList의 유효한 레코드 갯수 반환,
 * for X$TABLE_INFO, X$TEMP_TABLE_INFO 
 *
 * aVarEntry    : 검색하고자 하는 PageListEntry
 * return       : 반환하는 레코드 갯수
 **********************************************************************/
ULong svpVarPageList::getRecordCount( smpPageListEntry *aVarEntry )
{
    ULong sInsertCount;
    ULong sDeleteCount;

    IDE_DASSERT( aVarEntry != NULL );

    /* BUG-47706 insert, delete 모두 다 증가만 하는 값으로 
     * delete 를 먼저 받고 insert 를 받아야 음수가 되지 않는다. */
    sDeleteCount = idCore::acpAtomicGet64( &(aVarEntry->mRuntimeEntry->mDelRecCnt) );
    sInsertCount = idCore::acpAtomicGet64( &(aVarEntry->mRuntimeEntry->mInsRecCnt) );

    return sInsertCount - sDeleteCount;
}


/**********************************************************************
 * PageListEntry를 초기화한다.
 *
 * aVarEntry : 초기화하려는 PageListEntry
 * aTableOID : PageListEntry의 테이블 OID
 **********************************************************************/
void svpVarPageList::initializePageListEntry( smpPageListEntry* aVarEntry,
                                              smOID             aTableOID )
{
    UInt i;

    IDE_DASSERT(aTableOID != 0);
    IDE_DASSERT(aVarEntry != NULL);

    /* BUG-26939 inefficient variable slot
     * Slot size is decided by slot counts(2^n -> basic slots).
     * avalable space(which in 1 page) / slot count = slot size
     * new median slots have inserted between basic slots.
     * Basic slots have even slot numbers (sEven).
     * And median slots have odd slot numbers (sOdd).
     */
#if defined(DEBUG)
    UInt sEven;
    UInt sOdd;
    UInt sCount;
    UInt sSlotSize;

    sEven   = 1 << ( SM_VAR_PAGE_LIST_COUNT / 2 );
    sOdd    = ( sEven + ( sEven >> 1 )) / 2 ;
    
    for( i = 0; i < SM_VAR_PAGE_LIST_COUNT; i++ )
    {
        switch( i%2 )
        {
            case 0 :
                sCount = sEven;
                sEven >>= 1;
                break;
            case 1 : 
                sCount = sOdd;
                sOdd >>= 1;
                break;
            default : 
                /* do nothing */
                break;
        }

        sSlotSize = ((SMP_PERS_PAGE_BODY_SIZE - ID_SIZEOF(smpVarPageHeader))
                / sCount) - ID_SIZEOF(smVCPieceHeader);
        /* 8 byte Align */
        sSlotSize -= (sSlotSize % 8 );

        IDE_DASSERT( sSlotSize == gVarSlotSizeArray[i] );
    }
#endif


    for( i = 0; i < SM_VAR_PAGE_LIST_COUNT; i++)
    {
        aVarEntry[i].mSlotSize  = gVarSlotSizeArray[i] + ID_SIZEOF(smVCPieceHeader);
        aVarEntry[i].mSlotCount =
            (SMP_PERS_PAGE_BODY_SIZE - ID_SIZEOF(smpVarPageHeader))
            /aVarEntry[i].mSlotSize;

        aVarEntry[i].mTableOID     = aTableOID;
        aVarEntry[i].mRuntimeEntry = NULL;
    }

    return;
}

/**********************************************************************
 * VarPage의 VarIdx값을 구한다.
 *
 * aPagePtr : Idx값을 알고자하는 Page 포인터
 **********************************************************************/
UInt svpVarPageList::getVarIdx( void* aPagePtr )
{
    UInt sIdx;

    IDE_DASSERT( aPagePtr != NULL );

    sIdx = ((smpVarPageHeader *)((SChar *)aPagePtr +
                                 ID_SIZEOF(smpPersPageHeader)))->mIdx;

    IDE_DASSERT( sIdx < SM_VAR_PAGE_LIST_COUNT );

    return sIdx;
}

/**********************************************************************
 * aValue에 대한 VarEntry의 Idx값을 구한다.
 *
 * aLength : VarEntry에 입력할 Value의 길이.
 * aIdx    : aValue에 대한 VarIdx값
 **********************************************************************/
IDE_RC svpVarPageList::calcVarIdx( UInt     aLength,
                                   UInt*    aVarIdx )
{
    IDE_DASSERT( aLength != 0 );
    IDE_DASSERT( aVarIdx != NULL );

    IDE_TEST_RAISE( aLength > gVarSlotSizeArray[SM_VAR_PAGE_LIST_COUNT -1],
                    too_long_var_item);

    *aVarIdx = (UInt)gAllocArray[aLength];

    return IDE_SUCCESS;

    IDE_EXCEPTION( too_long_var_item );
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Too_Long_Var_Data));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#ifdef DEBUG
/**********************************************************************
 * Page내의 FreeSlotList의 연결이 올바른지 검사한다.
 *
 * aFreePageHeader : 검사하려는 FreeSlotList가 있는 Page의 FreePageHeader
 **********************************************************************/
idBool svpVarPageList::isValidFreeSlotList( smpFreePageHeader* aFreePageHeader )
{
    idBool                sIsValid;

    if ( iduProperty::getEnableRecTest() == 1 )
    {
        vULong                sPageCount = 0;
        smVCPieceHeader*      sCurFreeSlotHeader = NULL;
        smVCPieceHeader*      sNxtFreeSlotHeader;

        IDE_DASSERT( aFreePageHeader != NULL );

        sIsValid = ID_FALSE;

        sNxtFreeSlotHeader = (smVCPieceHeader*)aFreePageHeader->mFreeSlotHead;

        while(sNxtFreeSlotHeader != NULL)
        {
            sCurFreeSlotHeader = sNxtFreeSlotHeader;

            sPageCount++;

            sNxtFreeSlotHeader =
                (smVCPieceHeader*)sCurFreeSlotHeader->nxtPieceOID;
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
#    endif // TSM_DEBUG
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
 *
 * Description: calcVarIdx 를 빠르게 하기 위한 gAllocArray를 세팅한다.
 *              각 사이즈값에 해당되는 인덱스를 미리 세팅해놓아
 *              사이즈로 바로 인덱스를 찾을 수 있게 한다.
 *
 **********************************************************************/
void svpVarPageList::initAllocArray()
{
    UInt sIdx;
    UInt sSize;

    for ( sIdx = 0, sSize = 0; sIdx < SM_VAR_PAGE_LIST_COUNT; sIdx++ )
    {
        for (; sSize <= gVarSlotSizeArray[sIdx]; sSize++ )
        {
            gAllocArray[sSize] = sIdx;
        }
    }

    IDE_DASSERT( ( sSize - 1) == ( SMP_PERS_PAGE_BODY_SIZE
                                 - ID_SIZEOF(smpVarPageHeader)
                                 - ID_SIZEOF(smVCPieceHeader))
               );

}

/***********************************************************************
 * slot size가 다른 FreePagePool에서 빈 페이지가 있는지 확인한다.
 * 빈 페이지가 있다면 현재 FreePageList로 빈 페이지를 받아온다.
 *
 * aSpaceID       : TableSpace ID
 * aVarEntryArray : PageListEntry Array 
 * aDstIdx        : Free Page를 받아오려는 FreePageList의 PageListEntry index
 * aPageListID    : PageListID
 * aIsPageAlloced : 빈 페이지를 받아왔는지 여부
 ***********************************************************************/
void svpVarPageList::tryForAllocPagesFromOtherPools( scSpaceID          aSpaceID,
                                                     smpPageListEntry * aVarEntryArray,
                                                     UInt               aDstIdx,
                                                     UInt               aPageListID,
                                                     idBool           * aIsPageAlloced )
{
    UInt                   sState           = 0;
    UInt                   sIdx             = 0;
    idBool                 sIsPageAlloced   = ID_FALSE;
    smpPageListEntry     * sSrcEntry        = NULL;
    smpFreePagePoolEntry * sSrcPool         = NULL;
    smpPageListEntry     * sDstEntry        = NULL;

    IDE_DASSERT( aVarEntryArray != NULL );
    IDE_DASSERT( aDstIdx < SM_VAR_PAGE_LIST_COUNT );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_DASSERT( aIsPageAlloced != NULL );

    sDstEntry = aVarEntryArray + aDstIdx;

    for ( sIdx = 0;
          sIdx < SM_VAR_PAGE_LIST_COUNT;
          sIdx++ )
    {
        sSrcEntry = aVarEntryArray + sIdx;

        if ( sSrcEntry == sDstEntry )
        {
            /* 동일한 Page List Entry인 경우 skip 한다. */ 
            continue;
        }

        sSrcPool = &(sSrcEntry->mRuntimeEntry->mFreePagePool);

        IDE_DASSERT( sSrcPool != NULL );

        if ( sSrcPool->mPageCount >= SMP_MOVEPAGECOUNT_POOL2LIST )
        {
            /* Pool의 갯수를 먼저 가져올 만큼 있는지 보고,
               Pool에 lock을 잡고 다시 확인한다. List의 lock은 하위 함수에서 잡는다. */
            (void)(sSrcPool->mMutex.lock( NULL /* idvSQL* */ ));
            sState = 1;

            if ( sSrcPool->mPageCount >= SMP_MOVEPAGECOUNT_POOL2LIST )
            {
                getPagesFromOtherPool( aSpaceID,
                                       sSrcPool,
                                       sDstEntry,
                                       aDstIdx,
                                       aPageListID,
                                       &sState );

                sIsPageAlloced = ID_TRUE;
                break;
            }

            if ( sState == 1 )
            {
                sState = 0;
                (void)(sSrcPool->mMutex.unlock());
            }
        }
    }

    if ( sState == 1 )
    {
        (void)(sSrcPool->mMutex.unlock());
    }

    *aIsPageAlloced = sIsPageAlloced;
}

/***********************************************************************
 * 특정 slot size의 FreePagePool(source)의 빈페이지를
 * 다른 slot size의 FreePageList(destination)로 이동시킨다.
 *
 * 주의 : 이 함수 호출전에 aSrcPool LOCK 을 잡아야한다.
 *
 * aSpaceID          : TableSpace ID
 * aSrcPool          : 빈페이지를 갖고 있는 source가 되는 FreePagePool
 * aDstEntry         : target이 되는 PageListEntry
 * aDstIdx           : target이 되는 PageListEntry의 index 
 * aPageListID       : PageListID
 * aSrcPoolLockState : source가 되는 FreePagePool의 LOCK 여부.
 ***********************************************************************/
void svpVarPageList::getPagesFromOtherPool( scSpaceID              aSpaceID,
                                            smpFreePagePoolEntry * aSrcPool,
                                            smpPageListEntry     * aDstEntry,
                                            UInt                   aDstIdx,
                                            UInt                   aPageListID,
                                            UInt                 * aSrcPoolLockState )
{
    UInt                   sPageCounter;
    UInt                   sAssignCount;
    smpFreePageHeader    * sNode    = NULL;
    smpFreePageListEntry * sDstList = NULL;

    smpFreePageHeader    * sSrcPoolHead = NULL;
    smpFreePageHeader    * sMovHead = NULL;
    smpFreePageHeader    * sMovTail = NULL;

    UInt                   sEmptySizeClassID;
    UInt                   sSrcPoolLockState;
    smpPersPage          * sPagePtr = NULL;

    IDE_DASSERT( aSrcPool != NULL );
    IDE_DASSERT( aDstEntry != NULL );
    IDE_DASSERT( aDstIdx < SM_VAR_PAGE_LIST_COUNT );
    IDE_DASSERT( aPageListID < SMP_PAGE_LIST_COUNT );
    IDE_DASSERT( ( aSrcPoolLockState != NULL ) && 
                 ( *aSrcPoolLockState == 1 ) );

    sSrcPoolLockState = *aSrcPoolLockState;

    sEmptySizeClassID = SMP_EMPTYPAGE_CLASSID( aDstEntry->mRuntimeEntry );
    sAssignCount      = SMP_MOVEPAGECOUNT_POOL2LIST;

    sDstList = &(aDstEntry->mRuntimeEntry->mFreePageList[aPageListID]);

    IDE_DASSERT( sDstList != NULL );

    IDE_DASSERT( svpFreePageList::isValidFreePageList( aSrcPool->mHead,
                                                       aSrcPool->mTail,
                                                       aSrcPool->mPageCount )
                 == ID_TRUE );

    /********************************************************/
    /* 1. source의 FreePagePool 앞부분에서 페이지들을 뗀다. */
    /********************************************************/

    sSrcPoolHead = aSrcPool->mHead;
    sMovHead     = aSrcPool->mHead;

    IDE_DASSERT( sAssignCount >= 1 );

    for( sPageCounter = 0;
         sPageCounter < sAssignCount;
         sPageCounter++ )
    {
        sMovTail     = sSrcPoolHead;
        sSrcPoolHead = sSrcPoolHead->mFreeNext;
    }

    if ( sSrcPoolHead == NULL )
    {
        /* pool에 남아잇는 페이지가 없는경우 */

        sMovTail->mFreeNext = NULL;

        aSrcPool->mTail = NULL;
        aSrcPool->mHead = NULL;
    }
    else
    {
        sMovTail->mFreeNext     = NULL;
        sSrcPoolHead->mFreePrev = NULL;

        aSrcPool->mHead = sSrcPoolHead;
    }

    aSrcPool->mPageCount -= sAssignCount;

    IDE_DASSERT( svpFreePageList::isValidFreePageList( aSrcPool->mHead,
                                                       aSrcPool->mTail,
                                                       aSrcPool->mPageCount )
                 == ID_TRUE );

    /* Source FreePagePool UNLOCK */
    if ( sSrcPoolLockState == 1 )
    {
        sSrcPoolLockState = 0;
        (void)(aSrcPool->mMutex.unlock());
    }

    /********************************************************/
    /* 2. source에서 뗀 페이지들의 정보를 dest의 정보로 변경한다. */
    /********************************************************/

    for ( sNode = sMovHead;
          sNode != NULL;
          sNode = sNode->mFreeNext )
    {
        sNode->mFreeListID  = aPageListID;
        sNode->mSizeClassID = sEmptySizeClassID;

        IDE_ASSERT( smmManager::getPersPagePtr( aSpaceID,
                                                sNode->mSelfPageID,
                                                (void**)&sPagePtr )
                    == IDE_SUCCESS );

        initializePage( aDstIdx,
                        aPageListID,
                        aDstEntry->mSlotSize,
                        aDstEntry->mSlotCount,
                        aDstEntry->mTableOID,
                        sPagePtr );

        svpFreePageList::initializeFreeSlotListAtPage( aSpaceID,
                                                       aDstEntry,
                                                       sPagePtr );
    }

    /********************************************************/
    /* 3. dest의 FreePageList의 뒷부분에 연결한다. */
    /********************************************************/

    /* Destination FreePageList LOCK */
    (void)(sDstList->mMutex.lock(NULL /* idvSQL* */) );

    if ( sDstList->mTail[sEmptySizeClassID] == NULL )
    {
        IDE_DASSERT( sDstList->mHead[sEmptySizeClassID] == NULL );

        sDstList->mHead[sEmptySizeClassID] = sMovHead;
    }
    else
    {
        IDE_DASSERT( sDstList->mHead[sEmptySizeClassID] != NULL );

        sDstList->mTail[sEmptySizeClassID]->mFreeNext = sMovHead;
        sMovHead->mFreePrev = sDstList->mTail[sEmptySizeClassID];
    }
    sDstList->mTail[sEmptySizeClassID] = sMovTail;

    sDstList->mPageCount[sEmptySizeClassID] += sAssignCount;

    IDE_DASSERT( svpFreePageList::isValidFreePageList( sDstList->mHead[sEmptySizeClassID],
                                                       sDstList->mTail[sEmptySizeClassID],
                                                       sDstList->mPageCount[sEmptySizeClassID])
                 == ID_TRUE );

    /* Destination FreePageList UNLOCK */
    (void)(sDstList->mMutex.unlock());

    *aSrcPoolLockState = sSrcPoolLockState;
}

