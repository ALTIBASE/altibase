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
 * $Id: sdpPageList.cpp 86435 2019-12-13 05:59:48Z jiwon.kim $
 *
 * Description :
 *
 * # sdp layer의 기능 구분
 *
 * sdc가 page에 저장되는 레코드에 관련된 기능을 수행한다.
 * sdc는 logical header, keymap, slot등의 작업을 위하여
 * sdp를 호출하고 sdp는 이러한 기능을 sdc에 제공한다.
 *
 *
 *      sdc - record internal - 컬럼, 레코드 헤더, var col hdr관리
 *            page에 레코드 insert, delete, update
 *
 * -------------------------------------
 *
 *
 *      sdp - physical page 관리
 *            logical page 관리
 *            keymap 관리
 *            slot 관리 ( alloc, free )
 *
 *
 * sdp layer는 page internal의 관리를 지원 index, temp table을 지원하기 위한
 * sdpPage 모듈과 table을 지원하기 위한 sdpPageList 모듈, sdpDataPage 모듈로
 * 구분한다. sdpPageList, sdpDataPage, index, temp table에서는
 * sdpPage의 기능을 호출한다.
 *
 *       sdpDataPage - data page internal 작업을 위한 기능 제공
 *
 *       sdpPageList - table을 지원하기 위한 기능 제공
 *                     sdpPageListEntry 클래스 관리
 *
 * -------------------------------------------------
 *
 *       sdpPage  - index, temp table의 페이지 internal작업을 위한
 *                  기능 제공
 *                  page의 physical header관리
 *                  page의 free size관리
 *                  keymap의 관리
 *                  page의 free offset관리
 *
 * 이 외에도 sdp layer에는 Segment, TableSpace, Extent가 있으며
 * page, extent를 관리한다.
 *
 * # data page-list entry 구현
 *
 * 하나의 테이블에 대한 페이지 할당,
 * 페이지 내에서 slot의 관리에 대한 기능을 수행한다.
 *
 * - parameter type의 원칙
 *   logical header가 필요한 곳은 sdpDataPageHdr * type을 받는다.
 *   insertable page list entry가 필요한 곳은 sdpTableMetaPageHdr * type을 받는다.
 *
 **********************************************************************/
#include <smErrorCode.h>
#include <smiDef.h>
#include <smu.h>
#include <sdd.h>
#include <sdr.h>
#include <smr.h>
#include <sct.h>

#include <sdpDef.h>
#include <sdpTableSpace.h>
#include <sdpPageList.h>
#include <sdpReq.h>

#include <sdpModule.h>
#include <sdpDPathInfoMgr.h>
#include <sdpSegDescMgr.h>
#include <sdpPhyPage.h>

/***********************************************************************
 * Description : page list entry 초기화
 ***********************************************************************/
void sdpPageList::initializePageListEntry( sdpPageListEntry*  aPageEntry,
                                           vULong             aSlotSize,
                                           smiSegAttr         aSegmentAttr,
                                           smiSegStorageAttr  aSegmentStoAttr )

{
    // fixed page list
    aPageEntry->mSlotSize = aSlotSize;

    aPageEntry->mSegDesc.mSegHandle.mSpaceID    = SC_NULL_SPACEID;
    aPageEntry->mSegDesc.mSegHandle.mSegPID     = SD_NULL_PID;
    aPageEntry->mSegDesc.mSegHandle.mSegAttr    = aSegmentAttr;
    aPageEntry->mSegDesc.mSegHandle.mSegStoAttr = aSegmentStoAttr;

    aPageEntry->mRecCnt           = 0;

    return;
}

/*
  Page List Entry의 Runtime Item을 NULL로 세팅한다.

  OFFLINE/DISCARD된 Tablespace안의 Table에 대해 수행된다.
*/
IDE_RC sdpPageList::setRuntimeNull( sdpPageListEntry* aPageEntry )
{
    sdpSegMgmtOp   * sSegMgmtOp;
    IDE_DASSERT( aPageEntry != NULL );

    aPageEntry->mMutex = NULL;
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(aPageEntry);
    IDE_ERROR( sSegMgmtOp != NULL );

    // Segment 공간관리 Cache를 해제한다.
    IDE_TEST ( sSegMgmtOp->mDestroy( &(aPageEntry->mSegDesc.mSegHandle) )
               != IDE_SUCCESS );

    aPageEntry->mSegDesc.mSegHandle.mCache = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : page list entry의 runtime 정보 초기화
 ***********************************************************************/
IDE_RC sdpPageList::initEntryAtRuntime( scSpaceID         aSpaceID,
                                        smOID             aTableOID,
                                        UInt              aIndexID,
                                        sdpPageListEntry* aPageEntry,
                                        idBool            aDoInitMutex /* ID_TRUE */ )
{
    UInt        sState  = 0;

    IDE_DASSERT( aPageEntry != NULL );

    if (aDoInitMutex == ID_TRUE)
    {
        SChar sBuffer[128];
        idlOS::memset(sBuffer, 0, 128);

        idlOS::snprintf(sBuffer,
                        128,
                        "DISK_PAGE_LIST_MUTEX_%"ID_XINT64_FMT,
                        (ULong)aTableOID);

        /* sdpPageList_initEntryAtRuntime_malloc_Mutex.tc */
        IDU_FIT_POINT("sdpPageList::initEntryAtRuntime::malloc::Mutex");
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SDP,
                                   ID_SIZEOF(iduMutex),
                                   (void **)&(aPageEntry->mMutex),
                                   IDU_MEM_FORCE)
                 != IDE_SUCCESS);
        sState  = 1;

        IDE_TEST(aPageEntry->mMutex->initialize(
                     sBuffer,
                     IDU_MUTEX_KIND_NATIVE,
                     IDV_WAIT_INDEX_LATCH_FREE_DRDB_PAGE_LIST_ENTRY )
                 != IDE_SUCCESS);
    }
    else
    {
        aPageEntry->mMutex = NULL;
    }

    // Segment Cache 초기화
    IDE_TEST( sdpSegDescMgr::initSegDesc(
                  &aPageEntry->mSegDesc,
                  aSpaceID,
                  sdpSegDescMgr::getSegPID( &aPageEntry->mSegDesc ),
                  SDP_SEG_TYPE_TABLE,
                  aTableOID,
                  aIndexID) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( aPageEntry->mMutex ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : page list entry의 runtime 정보 해제
 ***********************************************************************/
IDE_RC sdpPageList::finEntryAtRuntime( sdpPageListEntry* aPageEntry )
{
    IDE_DASSERT( aPageEntry != NULL );

    if ( aPageEntry->mMutex != NULL )
    {
        /* page list entry의 mutex 해제 */
        IDE_TEST(aPageEntry->mMutex->destroy() != IDE_SUCCESS);

        IDE_TEST(iduMemMgr::free(aPageEntry->mMutex) != IDE_SUCCESS);
        aPageEntry->mMutex = NULL;
    }
    else
    {
        // Do Nothing
        // OFFLINE/DISCARD Tablespace에 속한 Table의 경우
        // mMutex가 NULL일 수도 있다.
    }

    if ( aPageEntry->mSegDesc.mSegHandle.mCache != NULL )
    {
        /* BUG-29941 - SDP 모듈에 메모리 누수가 존재합니다.
         * 여기서 initEntryAtRuntime에서 할당한 메모리를 해제한다. */
        IDE_TEST( sdpSegDescMgr::destSegDesc( &aPageEntry->mSegDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Do Nothing
        // OFFLINE/DISCARD Tablespace에 속한 Table의 경우
        // Segment Cache가 NULL일 수도 있다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * PROJ-1566
 *
 * Description : record를 append할 free page를 찾음
 *
 * Implementation :
 *    (1) Free Page 할당받음
 *    (2) 이전 Page와 연결 정보 설정
 *    sdpPageList::findSlot4AppendRec()에서 호출
 *
 *    - In
 *      aStatistics     : statistics
 *      aTrans          : transaction
 *      aSpaceID        : tablespace ID
 *      aPageEntry      : page list entry
 *      aDPathSegInfo   : Direct-Path Insert Information
 *      aMtx            : mini transaction
 *      aPrevPhyPageHdr : 이전 physical page header
 *      aPrevPageID     : 이전 page ID
 *      aStartExtRID    : free page가 존재하는 첫번째 extent RID
 *      aIsLogging      : Logging 여부
 *    - Out
 *      aFindPagePtr     : 찾은 free page pointer
 **********************************************************************/
IDE_RC sdpPageList::allocPage4AppendRec( idvSQL           * aStatistics,
                                         scSpaceID          aSpaceID,
                                         sdpPageListEntry * aPageEntry,
                                         sdpDPathSegInfo  * aDPathSegInfo,
                                         sdpPageType        aPageType,
                                         sdrMtx           * aMtx,
                                         idBool             aIsLogging,
                                         UChar           ** aAllocPagePtr )
{
    sdRID              sPrvAllocExtRID;
    sdRID              sFstPIDOfPrvAllocExt;
    scPageID           sPrvAllocPID;

    scPageID           sFstPIDOfAllocExt;
    sdRID              sAllocExtRID;
    scPageID           sAllocPID;
    UChar              sIsConsistent;
    sdpSegHandle     * sSegHandle;
    UChar            * sAllocPagePtr;
    sdpSegMgmtOp     * sSegMgmtOp;

    sPrvAllocPID         = aDPathSegInfo->mLstAllocPID;
    sPrvAllocExtRID      = aDPathSegInfo->mLstAllocExtRID;
    sFstPIDOfPrvAllocExt = aDPathSegInfo->mFstPIDOfLstAllocExt;
    sSegHandle           = &aDPathSegInfo->mSegDesc->mSegHandle;
    sSegMgmtOp           = sdpSegDescMgr::getSegMgmtOp( aPageEntry );
    IDE_ERROR( sSegMgmtOp != NULL ); 

    IDE_TEST( sSegMgmtOp->mAllocNewPage4Append( aStatistics,
                                                aMtx,
                                                aSpaceID,
                                                sSegHandle,
                                                sPrvAllocExtRID,
                                                sFstPIDOfPrvAllocExt,
                                                sPrvAllocPID,
                                                aIsLogging,
                                                aPageType,
                                                &sAllocExtRID,
                                                &sFstPIDOfAllocExt,
                                                &sAllocPID,
                                                &sAllocPagePtr )
              != IDE_SUCCESS );

    IDE_ASSERT( sAllocPagePtr != NULL );

    aDPathSegInfo->mTotalPageCount++;

    // PROJ-1665
    if ( aIsLogging == ID_FALSE )
    {
        // no logging mode일 경우,
        // page상태가 inconsistent 하다는 것을 정보를 로깅
        // 단, page 상태가 inconsistent 하다고 설정하지는 않음
        //---------------------------------------------------------
        // < 이와 같이 처리하는 이유 >
        // Media Recovery 시에 Direct-Path INSERT가 nologging으로
        // 수행 완료된 Page에 대하여 DML 구문이 오면 해당 Page에
        // 아무 내용이 적혀 있지 않기 때문에 오동작 할 수 있다.
        // 따라서 이런 문제를 방지하기 위하여
        // media recovery시에 아래와 같은 log를 보면
        // 해당 Page 상태를 in-consistent로 변경한다.
        sIsConsistent = SDP_PAGE_INCONSISTENT;

        IDE_TEST( smrLogMgr::writePageConsistentLogRec( aStatistics,
                                                        aSpaceID,
                                                        sAllocPID,
                                                        sIsConsistent )
                  != IDE_SUCCESS );
    }

    /* DPath시 할당한 마지막 페이지와 그 페이지가 속한 ExtRID를
     * 가지고 있는다. 왜냐하면 Commit시 Target 테이블의 ExtList와
     * 연결과 HWM의 갱신을 위해서 이다. */

    aDPathSegInfo->mLstAllocPID         = sAllocPID;
    aDPathSegInfo->mLstAllocExtRID      = sAllocExtRID;
    aDPathSegInfo->mFstPIDOfLstAllocExt = sFstPIDOfAllocExt;

    *aAllocPagePtr = sAllocPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aAllocPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1566
 *
 * Description : append를 위한 slot을 찾음
 * Implementation :
 *    (1) Max Row를 넘지 않는지 검사
 *    (2) 새로운 Free page가 필요한 지 검사한 후,
 *        새로운 Free Page가 필요하면 새로 할당 받음
 *    (3) 기존 마지막 Page 또는 새로운 Free page에 Append할 slot 위치
 *        찾아 설정해줌
 *
 *    - In
 *      aStatistics    : statistics
 *      aTrans         : transaction
 *      aSpaceID       : tablespace ID
 *      aPageEntry     : page list entry
 *      aTableInfo     : table info
 *      aRecSize       : record size
 *      aMaxRow        : table이 가질수 있는 max row 개수
 *      aIsLoggingMode : Direct-Path INSERT가 Logging Mode임
 *      aStartInfo     : mini transaction start info
 *    - Out
 *      aPageHdr       : append 할 record가 속한 page header
 *      aAllocSlotRID  : append 할 record가 저장될 slot의 RID
 **********************************************************************/
IDE_RC sdpPageList::findSlot4AppendRec( idvSQL           * aStatistics,
                                        scSpaceID          aSpaceID,
                                        sdpPageListEntry * aPageEntry,
                                        sdpDPathSegInfo  * aDPathSegInfo,
                                        UInt               aRecSize,
                                        idBool             aIsLogging,
                                        sdrMtx           * aMtx,
                                        UChar           ** aPageHdr,
                                        UChar            * aNewCTSlotIdx )
{
    UChar            * sLstAllocPagePtr;
    UChar            * sFreePagePtr;
    idBool             sIsNeedNewPage;
    scPageID           sFreePID;
    UChar              sNewCTSlotIdx;

    IDE_DASSERT( aPageEntry != NULL );
    IDE_DASSERT( aMtx       != NULL );

    sFreePagePtr   = NULL;
    sIsNeedNewPage = ID_TRUE;

    /* Append이기때문에 마지막 페이지에서 공간이 있는지 검사여
     * 새로운 Free Page 할당 여부를 결정한다. */
    sLstAllocPagePtr    = aDPathSegInfo->mLstAllocPagePtr;

    if( sLstAllocPagePtr != NULL )
    {
        if( canInsert2Page4Append( aPageEntry, sLstAllocPagePtr, aRecSize )
            == ID_TRUE )
        {
            sIsNeedNewPage = ID_FALSE;
            sFreePagePtr   = sLstAllocPagePtr;
        }
    }

    if( sIsNeedNewPage == ID_TRUE )
    {
        IDE_TEST( allocPage4AppendRec( aStatistics,
                                       aSpaceID,
                                       aPageEntry,
                                       aDPathSegInfo,
                                       SDP_PAGE_DATA,
                                       aMtx,
                                       aIsLogging,
                                       &sFreePagePtr )
                  != IDE_SUCCESS );

        sFreePID = sdpPhyPage::getPageIDFromPtr( sFreePagePtr );

        if( sLstAllocPagePtr != NULL )
        {
            /* update only로 변한 Page를 DPath Buffer의 flush list에
             * 연결함 */
            IDE_TEST( sdbDPathBufferMgr::setDirtyPage( (void*)sLstAllocPagePtr )
                      != IDE_SUCCESS );
        }

        aDPathSegInfo->mLstAllocPagePtr = sFreePagePtr;

        if( aDPathSegInfo->mFstAllocPID == SD_NULL_PID )
        {
            aDPathSegInfo->mFstAllocPID = sFreePID;
        }

        IDE_ASSERT( sFreePagePtr != NULL );

    }

    // Direct-Path Insert는 무조건 0번째 CTS를 할당한다.
    IDE_ASSERT( smLayerCallback::allocCTS( aStatistics,
                                           aMtx,   /* aFixMtx */
                                           aMtx,   /* aLogMtx */
                                           SDB_SINGLE_PAGE_READ,
                                           (sdpPhyPageHdr*)sFreePagePtr,
                                           &sNewCTSlotIdx )
                == IDE_SUCCESS );

    *aPageHdr      = sFreePagePtr;
    *aNewCTSlotIdx = sNewCTSlotIdx;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Insert 가능한 페이지를 탐색한다.
 ***********************************************************************/
IDE_RC sdpPageList::findPage( idvSQL            *aStatistics,
                              scSpaceID          aSpaceID,
                              sdpPageListEntry  *aPageEntry,
                              void              *aTableInfoPtr,
                              UInt               aRecSize,
                              ULong              aMaxRow,
                              idBool             aAllocNewPage,
                              idBool             aNeedSlotEntry,
                              sdrMtx            *aMtx,
                              UChar            **aPagePtr,
                              UChar             *aNewCTSlotIdx )
{
    UChar            sNewCTSlotIdx;
    sdpPhyPageHdr  * sPagePtr;
    sdpSegHandle   * sSegHandle;
    sdpSegMgmtOp   * sSegMgmtOp;

    IDE_DASSERT( aRecSize      != 0 );
    IDE_DASSERT( aMaxRow       != 0 );
    IDE_DASSERT( aPageEntry    != NULL );
    IDE_DASSERT( aPagePtr      != NULL );
    IDE_DASSERT( aNewCTSlotIdx != NULL );

    sPagePtr      = NULL;
    sNewCTSlotIdx = SDP_CTS_IDX_NULL;
    sSegHandle    = &(aPageEntry->mSegDesc.mSegHandle);

    if (aTableInfoPtr != NULL)
    {
        // max row를 넘지 않는지 검사
        IDE_TEST( validateMaxRow( aPageEntry,
                                  aTableInfoPtr,
                                  aMaxRow )
                  != IDE_SUCCESS );
    }

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( aPageEntry );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    if( aAllocNewPage == ID_TRUE )
    {
        IDE_TEST( sSegMgmtOp->mAllocNewPage(
                              aStatistics,
                              aMtx,
                              aSpaceID,
                              sSegHandle,
                              SDP_PAGE_DATA,
                              (UChar**)&sPagePtr )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sSegMgmtOp->mFindInsertablePage(
                              aStatistics,
                              aMtx,
                              aSpaceID,
                              sSegHandle,
                              aTableInfoPtr,
                              SDP_PAGE_DATA,
                              aRecSize,
                              aNeedSlotEntry,
                              (UChar**)&sPagePtr,
                              &sNewCTSlotIdx )
                  != IDE_SUCCESS );
    }

    if ( sNewCTSlotIdx == SDP_CTS_IDX_NULL )
    {
        IDE_ASSERT( smLayerCallback::allocCTS( aStatistics,
                                               aMtx,  /* aFixMtx */
                                               aMtx,  /* aLogMtx */
                                               SDB_SINGLE_PAGE_READ,
                                               sPagePtr,
                                               &sNewCTSlotIdx )
                     == IDE_SUCCESS );
    }
    else
    {
        /* Insert 시에는 가용페이지가 있는한 CTS를 할당 못할 수가 없다. */
    }

    IDE_ASSERT( sPagePtr != NULL );

    *aPagePtr   = (UChar*)sPagePtr;
    *aNewCTSlotIdx = sNewCTSlotIdx;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 실제로 insert를 위한 slot 할당
 * findSlot4InsRec을 먼저 수행하여 slot을 확보하게 된 후 호출된다.
 **********************************************************************/
IDE_RC sdpPageList::allocSlot( UChar*       aPagePtr,
                               UShort       aSlotSize,
                               UChar**      aSlotPtr,
                               sdSID*       aAllocSlotSID )
{
    sdpPhyPageHdr      *sPhyPageHdr;
    scSlotNum           sAllocSlotNum;
    scOffset            sAllocSlotOffset;

    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aSlotPtr != NULL );

    sPhyPageHdr = sdpPhyPage::getHdr(aPagePtr);

    IDE_TEST( sdpPhyPage::allocSlot4SID( sPhyPageHdr,
                                         aSlotSize,
                                         aSlotPtr,
                                         &sAllocSlotNum,
                                         &sAllocSlotOffset )
              != IDE_SUCCESS );

    IDE_ERROR( aSlotPtr != NULL );

    *aAllocSlotSID =
        SD_MAKE_SID( sdpPhyPage::getPageID((UChar*)sPhyPageHdr),
                     sAllocSlotNum );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


// 공간할당이후 페이지의 가용도 및 관련 Segment 자료구조를 변경한다.
IDE_RC sdpPageList::updatePageState( idvSQL            * aStatistics,
                                     scSpaceID           aSpaceID,
                                     sdpPageListEntry  * aEntry,
                                     UChar             * aPagePtr,
                                     sdrMtx            * aMtx )
{
    IDE_ASSERT( aEntry != NULL );
    IDE_ASSERT( aPagePtr != NULL );
    IDE_ASSERT( aMtx != NULL );

    IDE_TEST( sdpSegDescMgr::getSegMgmtOp( aEntry )->mUpdatePageState(
                  aStatistics,
                  aMtx,
                  aSpaceID,
                  sdpSegDescMgr::getSegHandle( aEntry ),
                  aPagePtr ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : slot을 free 한다.
 **********************************************************************/
IDE_RC sdpPageList::freeSlot( idvSQL            *aStatistics,
                              scSpaceID          aTableSpaceID,
                              sdpPageListEntry  *aPageEntry,
                              UChar             *aSlotPtr,
                              scGRID             aSlotGRID,
                              sdrMtx            *aMtx )
{
    UChar               * sRowSlot     = aSlotPtr;
    sdpPhyPageHdr       * sPhyPageHdr;
    UChar               * sSlotDirPtr;
    UShort                sRowSlotSize = 0;

    IDE_DASSERT( aPageEntry     != NULL );
    IDE_DASSERT( aSlotPtr       != NULL );
    IDE_DASSERT( aMtx           != NULL );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aSlotGRID) );

    sPhyPageHdr  = sdpPhyPage::getHdr((UChar*)sRowSlot);
    sRowSlotSize = smLayerCallback::getRowPieceSize( sRowSlot );

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        aSlotGRID,
                                        (void*)&sRowSlotSize,
                                        ID_SIZEOF(sRowSlotSize),
                                        SDR_SDP_FREE_SLOT_FOR_SID )
              != IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::freeSlot4SID( sPhyPageHdr,
                                        sRowSlot,
                                        SC_MAKE_SLOTNUM( aSlotGRID ),
                                        sRowSlotSize )
              != IDE_SUCCESS );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPhyPageHdr);

    if( sdpSlotDirectory::getCount(sSlotDirPtr) == 0 )
    {
        /* 완전히 빈페이지는 UnFormat Page List에 넣어준다. */
        IDE_TEST( sdpSegDescMgr::getSegMgmtOp( aPageEntry )->mFreePage(
                                              aStatistics,
                                              aMtx,
                                              aTableSpaceID,
                                              &(aPageEntry->mSegDesc.mSegHandle),
                                              (UChar*)sPhyPageHdr )
                  != IDE_SUCCESS );
    }
    else
    {
        if( sdpPhyPage::getPageType(sPhyPageHdr)
            == SDP_PAGE_DATA )
        {
            IDE_TEST( updatePageState( aStatistics,
                                       aTableSpaceID,
                                       aPageEntry,
                                       (UChar*)sPhyPageHdr,
                                       aMtx )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 로깅에 필요한 page list entry 정보 반환(request api)
 **********************************************************************/
void sdpPageList::getPageListEntryInfo( void*       aPageEntry,
                                        scPageID*   aSegPID )
{
    sdpPageListEntry* sPageEntry;

    IDE_ASSERT( aPageEntry != NULL );
    IDE_ASSERT( aSegPID    != NULL );

    sPageEntry = (sdpPageListEntry*)aPageEntry;

    *aSegPID = getTableSegDescPID( sPageEntry );
}

/***********************************************************************
 *
 * Description :
 *  insert rowpiece 연산시에, allocNewPage를 할지 또는 findPage를 할지
 *  여부를 검사하는 함수이다.
 *
 *  empty page에 aDataSize 크기의 row를 넣는 경우에,
 *  PCTFREE 조건을 만족하는지 체크한다.
 *
 *  1. 페이지의 freespace의 비율이 PCTFREE보다 작으면
 *     무조건 allocNewPage를 해야한다.
 *     왜나하면 기존페이지에 row를 넣게되면
 *     그 페이지는 100% PCTFREE 조건을 만족하지 않을 것이기 때문이다.
 *     (empty page에 row를 넣어도 PCTFREE 조건을 만족하지 않았다.)
 *     기존에 저장된 row가 update시 사용할 공간을 남겨두어야 한다.
 *
 *  2. 페이지의 freespace의 비율이 PCTFREE보다 크면
 *     findPage를 시도해본다.
 *
 *  aEntry    - [IN] page list entry
 *  aDataSize - [IN] insert하려는 data의 크기
 *
 **********************************************************************/
idBool sdpPageList::needAllocNewPage( sdpPageListEntry *aEntry,
                                      UInt              aDataSize )
{
    UInt   sRemainSize;
    UInt   sPctFree;
    UInt   sPctFreeSize;

    IDE_DASSERT( aEntry !=NULL );
    IDE_DASSERT( aDataSize >0 );

    sPctFree =
        sdpSegDescMgr::getSegAttr( &(aEntry->mSegDesc) )->mPctFree;

    sPctFreeSize = ( SD_PAGE_SIZE * sPctFree )  / 100 ;

    sRemainSize = SD_PAGE_SIZE
        - sdpPhyPage::getPhyHdrSize()
        - ID_SIZEOF(sdpPageFooter)
        - aDataSize
        - ID_SIZEOF(sdpSlotEntry);

    if( sRemainSize < sPctFreeSize )
    {
        /* 페이지의 freespace의 비율이 PCTFREE보다 작으면
         * 무조건 allocNewPage를 해야한다.
         * 왜나하면 기존페이지에 row를 넣게되면
         * 그 페이지는 100% PCTFREE 조건을 만족하지 않을 것이기 때문이다.
         * (empty page에 row를 넣어도 PCTFREE 조건을 만족하지 않았다.)
         * 기존에 저장된 row가 update시 사용할 공간을 남겨두어야 한다. */
        return ID_TRUE;
    }
    else
    {
        /* 페이지의 freespace의 비율이 PCTFREE보다 크면
         * findPage를 시도해본다. */
        return ID_FALSE;
    }
}

/***********************************************************************
 *
 * Description :
 *  이 페이지에 데이터를 insert할 수 있는지 체크한다.
 *  1. canAllocSlot 가능한지 체크
 *  2. 이 페이지에 데이터를 넣었을 경우,
 *     freespace의 비율이 PCTFREE 보다 작아지지는
 *     않는지 체크
 *
 *  aEntry    - [IN] page list entry
 *  aPagePtr  - [IN] page ptr
 *  aDataSize - [IN] insert하려는 data의 크기
 *
 **********************************************************************/
idBool sdpPageList::canInsert2Page4Append( sdpPageListEntry *aEntry,
                                           UChar            *aPagePtr,
                                           UInt              aDataSize )
{
    UInt            sRemainSize;
    UInt            sPctFree;
    UInt            sPctFreeSize;
    idBool          sCanAllocSlot;
    sdpPhyPageHdr * sPageHdr;

    IDE_DASSERT(aEntry!=NULL);
    IDE_DASSERT(aDataSize>0);

    sPageHdr = sdpPhyPage::getHdr( aPagePtr );

    sCanAllocSlot =  sdpPhyPage::canAllocSlot( sPageHdr,
                                               aDataSize,
                                               ID_TRUE, /* Use Key Slot */
                                               SDP_8BYTE_ALIGN_SIZE );

    if( sCanAllocSlot == ID_TRUE )
    {
        sPctFree =
            sdpSegDescMgr::getSegAttr( &(aEntry->mSegDesc) )->mPctFree;

        sPctFreeSize = ( SD_PAGE_SIZE * sPctFree )  / 100 ;

        sRemainSize = sdpPhyPage::getTotalFreeSize( sPageHdr )
                      - ID_SIZEOF(sdpSlotEntry)
                      - aDataSize;

        if( sRemainSize < sPctFreeSize )
        {
            /* 이 페이지에 새로운 row를 넣게되면,
             * 페이지내 freespace의 비율이
             * PCTFREE 보다 작아지게 된다.
             * 그래서 이 페이지에 insert할 수 없다. */
            sCanAllocSlot = ID_FALSE;
        }
    }

    return sCanAllocSlot;
}

/***********************************************************************
 * Description : record count 반환
 * record count는 insert된 record - delete된 record로 결정
 ***********************************************************************/
#ifndef COMPILE_64BIT
IDE_RC sdpPageList::getRecordCount( idvSQL*           aStatistics,
                                    sdpPageListEntry* aPageEntry,
                                    ULong*            aRecordCount,
                                    idBool            aLockMutex /* ID_TRUE */)
#else
IDE_RC sdpPageList::getRecordCount( idvSQL*           /* aStatistics */,
                                    sdpPageListEntry* aPageEntry,
                                    ULong*            aRecordCount,
                                    idBool            /* aLockMutex */)
#endif
{
    IDE_DASSERT( aPageEntry != NULL );
    IDE_DASSERT( aRecordCount != NULL );

    /*
     * TASK-4690
     * 64비트에서는 64비트 변수를 atomic하게 read/write할 수 있다.
     * 즉, 아래 mutex를 잡고 푸는 것은 불필요한 lock이다.
     * 따라서 64비트인 경우엔 lock을 잡지 않도록 한다.
     */
#ifndef COMPILE_64BIT
    /* mutex lock 획득 */
    if (aLockMutex == ID_TRUE)
    {
        IDE_TEST( lock( aStatistics, aPageEntry ) != IDE_SUCCESS );
    }
#endif

    /* record count 계산 */
    // BUG-47368: atomic 연산으로 변경 
    *aRecordCount = idCore::acpAtomicGet64( &(aPageEntry->mRecCnt) );

#ifndef COMPILE_64BIT
    /* mutex lock release  */
    if (aLockMutex == ID_TRUE)
    {
        IDE_TEST( unlock( aPageEntry ) != IDE_SUCCESS );
    }
#endif

    return IDE_SUCCESS;

#ifndef COMPILE_64BIT
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/***********************************************************************
 * Description : page list entry의 item size를 align시킨 slot size 반환
 **********************************************************************/
UInt sdpPageList::getSlotSize( sdpPageListEntry* aPageEntry )
{

    IDE_DASSERT( aPageEntry != NULL );

    return aPageEntry->mSlotSize;

}

/***********************************************************************
 * Description : page list entry의 seg desc rID 반환
 **********************************************************************/
scPageID sdpPageList::getTableSegDescPID( sdpPageListEntry*   aPageEntry )
{
    IDE_DASSERT( aPageEntry != NULL );

    return aPageEntry->mSegDesc.mSegHandle.mSegPID;
}

/***********************************************************************
 * Description : page list entry의 insert count를 get
 * 이 함수가 호출되기전에 반드시 entry의 mutex가 획득된 상태여야한다.
 * return aPageEntry->insert count
 **********************************************************************/
ULong sdpPageList::getRecCnt( sdpPageListEntry*  aPageEntry )
{
    return aPageEntry->mRecCnt;
}

/***********************************************************************
 PROJ-1566
 Description : write pointer의 page가 table page인지 검사

***********************************************************************/
idBool sdpPageList::isTablePageType ( UChar * aWritePtr )
{
    sdpPageType sPageType;

    sPageType = sdpPhyPage::getPageType( sdpPhyPage::getHdr(aWritePtr) );

    if ( sPageType == SDP_PAGE_DATA )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************

  Description : MAX ROW를 넘는지 검사

***********************************************************************/
IDE_RC sdpPageList::validateMaxRow( sdpPageListEntry * aPageEntry,
                                    void             * aTableInfoPtr,
                                    ULong              aMaxRow )
{
    ULong sRecordCnt;

    /* BUG-19573 Table의 Max Row가 Disable되어있으면 이 값을 insert시에
     *           Check하지 않아야 한다. */
    if( aMaxRow != ID_ULONG_MAX )
    {
        // BUG-47368: mutex 제거, atomic 연산으로 변경 
        sRecordCnt = idCore::acpAtomicGet64( &(aPageEntry->mRecCnt) );
        sRecordCnt += smLayerCallback::getRecCntOfTableInfo( aTableInfoPtr );

        IDE_TEST_RAISE(aMaxRow <= sRecordCnt, err_exceed_maxrow);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_exceed_maxrow);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_ExceedMaxRows));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpPageList::getAllocPageCnt( idvSQL*         aStatistics,
                                     scSpaceID       aSpaceID,
                                     sdpSegmentDesc *aSegDesc,
                                     ULong          *aFmtPageCnt )
{
    IDE_TEST( aSegDesc->mSegMgmtOp->mGetFmtPageCnt( aStatistics,
                                                    aSpaceID,
                                                    &aSegDesc->mSegHandle,
                                                    aFmtPageCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpPageList::addPage4TempSeg( idvSQL           *aStatistics,
                                     sdpSegHandle     *aSegHandle,
                                     UChar            *aPrvPagePtr,
                                     scPageID          aPrvPID,
                                     UChar            *aPagePtr,
                                     scPageID          aPID,
                                     sdrMtx           *aMtx )
{
    sdpPhyPageHdr       *sPageHdr;
    sdpDblPIDListNode   *sPageNode;
    sdpPhyPageHdr       *sPrvPageHdr;
    sdpDblPIDListNode   *sPrvPageNode;
    sdpPhyPageHdr       *sNxtPageHdr;
    sdpDblPIDListNode   *sNxtPageNode;
    sdpSegCCache        *sCommonCache;
    UChar               *sNxtPagePtr;
    UChar               *sPrvPagePtr;
    idBool               sTrySuccess;
    scSpaceID            sSpaceID;

    sCommonCache = (sdpSegCCache*) ( aSegHandle->mCache );
    sPageHdr     = sdpPhyPage::getHdr( aPagePtr );
    sPageNode    = sdpPhyPage::getDblPIDListNode( sPageHdr );
    sSpaceID     = sdpPhyPage::getSpaceID( aPagePtr );

    if( aPrvPID == SD_NULL_PID )
    {
        if( sCommonCache->mTmpSegTail != SD_NULL_PID )
        {
            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                  sSpaceID,
                                                  sCommonCache->mTmpSegTail,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  aMtx,
                                                  &sPrvPagePtr,
                                                  &sTrySuccess,
                                                  NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

            sPrvPageHdr  = sdpPhyPage::getHdr( sPrvPagePtr );
            sPrvPageNode = sdpPhyPage::getDblPIDListNode( sPrvPageHdr );

            IDE_ASSERT( sPrvPageNode->mNext == SD_NULL_PID );

            sPrvPageNode->mNext = aPID;
        }
        else
        {
            sCommonCache->mTmpSegHead = aPID;
        }

        sPageNode->mPrev = sCommonCache->mTmpSegTail;
        sPageNode->mNext = SD_NULL_PID;

        sCommonCache->mTmpSegTail = aPID;
    }
    else
    {
        sPrvPageHdr  = sdpPhyPage::getHdr( aPrvPagePtr );
        sPrvPageNode = sdpPhyPage::getDblPIDListNode( sPrvPageHdr );

        sPageNode->mNext = sPrvPageNode->mNext;
        sPageNode->mPrev = aPrvPID;

        sPrvPageNode->mNext = aPID;

        if( sCommonCache->mTmpSegTail == aPrvPID )
        {
            sCommonCache->mTmpSegTail = aPID;
        }
        else
        {
            IDE_ASSERT( sPageNode->mNext != SD_NULL_PID );
            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                  sSpaceID,
                                                  sPageNode->mNext,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  aMtx,
                                                  &sNxtPagePtr,
                                                  &sTrySuccess,
                                                  NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

            sNxtPageHdr  = sdpPhyPage::getHdr( sNxtPagePtr );
            sNxtPageNode = sdpPhyPage::getDblPIDListNode( sNxtPageHdr );

            sNxtPageNode->mPrev = aPID;
        }
    }

    IDE_ASSERT( sCommonCache->mTmpSegHead != SD_NULL_PID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  Data Page 생성시 초기 CTL Size를 계산하여 반환
 *
 * DataPage는 InitTrans 속성값을 고려하여 Max RowPiece를
 * 크기를 제한할 수 있다.
 **********************************************************************/
UShort sdpPageList::getSizeOfInitTrans( sdpPageListEntry * aPageEntry )
{
    UShort sInitTrans =
        sdpSegDescMgr::getSegAttr(
            &aPageEntry->mSegDesc )->mInitTrans;

    return ( (sInitTrans * ID_SIZEOF(sdpCTS)) +
             idlOS::align8(ID_SIZEOF(sdpCTL)));
}
