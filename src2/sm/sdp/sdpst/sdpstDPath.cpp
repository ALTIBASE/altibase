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
 *
 * $Id: sdpstDPath.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Direct-Path Insert 관련 STATIC
 * 인터페이스를 관리한다. 
 *
 ***********************************************************************/

# include <sdpReq.h>
# include <sdpPhyPage.h>

# include <sdpstBMP.h>
# include <sdpstLfBMP.h>
# include <sdpstItBMP.h>
# include <sdpstRtBMP.h>

# include <sdpstFindPage.h>
# include <sdpstAllocPage.h>

# include <sdpstSH.h>
# include <sdpstStackMgr.h>
# include <sdpstCache.h>
# include <sdpstFreePage.h>
# include <sdpstSegDDL.h>
# include <sdpstExtDir.h>
# include <sdpstDPath.h>
# include <sdpstCache.h>

/***********************************************************************
 * Description : [ INTERFACE ] DIRECT-PATH INSERT를 위한 Append 방식의
 *               Page 할당 연산
 *
 * aStatistics      - [IN] 통계 정보
 * aMtx             - [IN] Mini Transaction Pointer
 * aSpaceID         - [IN] TableSpace ID
 * aSegHandle       - [IN] Segment Handle
 * aPrvAllocExtRID  - [IN] 이전에 Page를 할당받았던 Extent RID
 * aPrvAllocPageID  - [IN] 이전에 할당받은 PageID
 * aAllocExtRID     - [OUT] 새로운 Page가 할당된 Extent RID
 * aAllocPID        - [OUT] 새롭게 할당받은 PageID
 ***********************************************************************/
IDE_RC sdpstDPath::allocNewPage4Append(
                            idvSQL              * aStatistics,
                            sdrMtx              * aMtx,
                            scSpaceID             aSpaceID,
                            sdpSegHandle        * aSegHandle,
                            sdRID                 aPrvAllocExtRID,
                            scPageID              /*aFstPIDOfPrvAllocExt*/,
                            scPageID              aPrvAllocPageID,
                            idBool                aIsLogging,
                            sdpPageType           aPageType,
                            sdRID               * aAllocExtRID,
                            scPageID            * /*aFstPIDOfAllocExt*/,
                            scPageID            * aAllocPID,
                            UChar              ** aAllocPagePtr )
{
    UChar          * sAllocPagePtr;
    sdpParentInfo    sParentInfo;
    ULong            sSeqNo;
    sdpstPBS         sPBS = (sdpstPBS)( SDPST_BITSET_PAGETP_DATA |
                                        SDPST_BITSET_PAGEFN_FUL );

    IDE_DASSERT( aSegHandle != NULL );
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( aAllocExtRID != NULL );
    IDE_DASSERT( aAllocPID != NULL );
    IDE_DASSERT( aAllocPagePtr != NULL );

    IDE_TEST( sdpstExtDir::allocNewPageInExt( aStatistics,
                                              aMtx,
                                              aSpaceID,
                                              aSegHandle,
                                              aPrvAllocExtRID,
                                              aPrvAllocPageID,
                                              aAllocExtRID,
                                              aAllocPID,
                                              &sParentInfo ) != IDE_SUCCESS );

    IDE_TEST( sdpstExtDir::makeSeqNo( aStatistics,
                                      aSpaceID,
                                      aSegHandle->mSegPID,
                                      *aAllocExtRID,
                                      *aAllocPID,
                                      &sSeqNo ) != IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::create4DPath( aStatistics,
                                        sdrMiniTrans::getTrans( aMtx ),
                                        aSpaceID,
                                        *aAllocPID,
                                        &sParentInfo,
                                        (UShort)sPBS,
                                        aPageType,
                                        ((sdpSegCCache*)aSegHandle->mCache)
                                            ->mTableOID,
                                        aIsLogging,
                                        &sAllocPagePtr )
              != IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::setSeqNo( sdpPhyPage::getHdr( sAllocPagePtr ),
                                    sSeqNo,
                                    aMtx ) != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::logAndInitCTL( aMtx,
                                              (sdpPhyPageHdr *)sAllocPagePtr,
                                              aSegHandle->mSegAttr.mInitTrans,
                                              aSegHandle->mSegAttr.mMaxTrans )
              != IDE_SUCCESS );

    IDE_TEST( sdpSlotDirectory::logAndInit((sdpPhyPageHdr *) sAllocPagePtr,
                                           aMtx)
                != IDE_SUCCESS );

    *aAllocPagePtr = sAllocPagePtr;

    /*
      ideLog::log( IDE_SERVER_0, "\t [ alloc New append ] %u\n",
                 sdpPhyPage::getPageIDFromPtr(sAllocPagePtr ));
     */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aAllocPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct-Path 연산에서 Slot들의 MFNL을 FULL로 한꺼번에
 *               변경한다.
 ***********************************************************************/
IDE_RC sdpstDPath::updateMFNLToFull4DPath( idvSQL             * aStatistics,
                                           sdrMtxStartInfo    * aStartInfo,
                                           sdpstBMPHdr        * aBMPHdr,
                                           SShort               aFmSlotNo,
                                           SShort               aToSlotNo,
                                           sdpstMFNL          * aNewMFNL )
{
    UInt            sState = 0;
    smLSN           sNTA;
    sdrMtx          sMtx;
    ULong           sArrData[5];
    scSpaceID       sSpaceID;
    scPageID        sPageID;
    UChar         * sPagePtr;
    sdpstMFNL       sFmMFNL;
    sdpstMFNL       sMFNLOfLstSlot;
    sdpstMFNL       sNewMFNL;
    UInt            sPageCnt;
    idBool          sNeedToChangeMFNL;

    IDE_ASSERT( aBMPHdr   != NULL );
    IDE_ASSERT( aFmSlotNo <= aToSlotNo );
    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aNewMFNL   != NULL );

    sFmMFNL        = (sdpstBMP::getMapPtr( aBMPHdr ) + aToSlotNo)->mMFNL;
    sMFNLOfLstSlot = *aNewMFNL;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
    sState = 1;

    sPagePtr = sdpPhyPage::getPageStartPtr( aBMPHdr );
    sSpaceID = sdpPhyPage::getSpaceID( sPagePtr );
    sPageID  = sdpPhyPage::getPageID( sPagePtr );

    IDE_TEST( sdrMiniTrans::setDirtyPage( &sMtx, sPagePtr )
              != IDE_SUCCESS );

    /* root에서의 bitmap 변경에 대한 Rollback연산을 위해서 여기서 NTA를 설정 */
    sNTA = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );

    sArrData[0] = sPageID;
    sArrData[1] = aFmSlotNo;        /* from slotNo */
    sArrData[2] = aToSlotNo;        /* to slotNo */
    sArrData[3] = sFmMFNL;          /* prv MFNL */
    sArrData[4] = sMFNLOfLstSlot;   /* MFNL of Lst Slot */

    sPageCnt = aToSlotNo - aFmSlotNo + 1;

    /* 마지막 slot을 제외한 나머지 slot의 MFNL 갱신 */
    if ( sPageCnt - 1 > 0 )
    {
        IDE_ASSERT( aToSlotNo > aFmSlotNo );

        IDE_TEST( sdpstBMP::logAndUpdateMFNL( &sMtx,
                                              aBMPHdr,
                                              aFmSlotNo,
                                              aToSlotNo - 1,
                                              sFmMFNL,
                                              SDPST_MFNL_FUL,
                                              sPageCnt - 1,
                                              &sNewMFNL,
                                              &sNeedToChangeMFNL )
                  != IDE_SUCCESS );
    }

    /* 마지막 slot에 대한 MFNL 갱신 */
    IDE_TEST( sdpstBMP::logAndUpdateMFNL( &sMtx,
                                          aBMPHdr,
                                          aToSlotNo,
                                          aToSlotNo,
                                          sFmMFNL,
                                          sMFNLOfLstSlot,
                                          1,
                                          &sNewMFNL,
                                          &sNeedToChangeMFNL )
              != IDE_SUCCESS );


    IDE_DASSERT( sdpstBMP::verifyBMP( aBMPHdr ) == IDE_SUCCESS );

    sdrMiniTrans::setNTA( &sMtx,
                          sSpaceID,
                          SDR_OP_SDPST_UPDATE_BMP_4DPATH,
                          &sNTA,
                          sArrData,
                          5 );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC sdpstDPath::reformatPage4DPath( idvSQL           *aStatistics,
                                       sdrMtxStartInfo  *aStartInfo,
                                       scSpaceID         aSpaceID,
                                       sdpSegHandle     *aSegHandle,
                                       sdRID             aLstAllocExtRID,
                                       scPageID          aLstPID )
{
    sdpstExtDesc        * sExtDescPtr;
    sdpstExtDesc          sExtDesc;
    sdRID                 sNxtExtRID;
    scPageID              sFstFmtPID;
    scPageID              sLstFmtPID;
    scPageID              sLstFmtExtDirPID;
    SShort                sLstFmtSlotNoInExtDir;
    UInt                  sState = 0;

    IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                          aSpaceID,
                                          aLstAllocExtRID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          NULL, /* sdrMtx */
                                          (UChar**)&sExtDescPtr,
                                          NULL  /* aTrySuccess */ )
              != IDE_SUCCESS );
    sState = 1;

    sExtDesc = *sExtDescPtr;

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sExtDescPtr )
              != IDE_SUCCESS );

    if ( sdpstExtDir::isFreePIDInExt( &sExtDesc, aLstPID ) == ID_FALSE )
    {
        IDE_CONT( skip_reformat_page );
    }

    /* 현 Extent 에서 마지막으로 할당된 페이지가 있다면
     * 해당 페이지 다음 페이지부터 reformat 한다. */
    IDE_TEST( sdpstAllocPage::formatDataPagesInExt(
                                            aStatistics,
                                            aStartInfo,
                                            aSpaceID,
                                            aSegHandle,
                                            &sExtDesc,
                                            aLstAllocExtRID,
                                            aLstPID + 1 ) != IDE_SUCCESS );


    /* 다음 Extent를 가져온다. */
    IDE_TEST( sdpstExtDir::getNxtExtRID( aStatistics,
                                         aSpaceID,
                                         aSegHandle->mSegPID,
                                         aLstAllocExtRID,
                                         &sNxtExtRID ) != IDE_SUCCESS );

    /* 
     * reformat한 Extent를 관리하는 LfBMP에 속한 다른 Extent 또한
     * reformat한다.
     * Extent에는 해당 Extent를 관리하고 있는 LfBMP에 대한 정보가
     * mExtMgmtLfBMP에 담겨있다.
     * Next Extent를 계속 따라가면서 맨 처음 따온 ExtMgmtLfBMP 값과 동일한 
     * mExtMgmtLfBMP를 갖는 Extent를 만나면 첫번째 Extent와 동일한 LfBMP에
     * 속한다 할 수 있기 때문에 reformat 한다.
     */
    if ( sNxtExtRID != SD_NULL_RID )
    {
        IDE_TEST( sdpstExtDir::formatPageUntilNxtLfBMP( aStatistics,
                                                        aStartInfo,
                                                        aSpaceID,
                                                        aSegHandle,
                                                        sNxtExtRID,
                                                        &sFstFmtPID,
                                                        &sLstFmtPID,
                                                        &sLstFmtExtDirPID,
                                                        &sLstFmtSlotNoInExtDir )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( skip_reformat_page );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                               (UChar*)sExtDescPtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : merge된 Segment의 HWM 및 MFNL을 조정한다.
 ***********************************************************************/
IDE_RC  sdpstDPath::updateWMInfo4DPath(
                                      idvSQL           *aStatistics,
                                      sdrMtxStartInfo  *aStartInfo,
                                      scSpaceID         aSpaceID,
                                      sdpSegHandle     *aSegHandle,
                                      scPageID          aFstAllocPID,
                                      sdRID             aLstAllocExtRID,
                                      scPageID        /*aFstPIDOfAllocExtOfFM*/,
                                      scPageID          aLstPID,
                                      ULong           /*aAllocPageCnt*/,
                                      idBool          /*aMergeMultiSeg*/ )
{
    sdrMtx              sMtx;
    SInt                sState = 0;
    scPageID            sSegPID;
    UChar             * sPagePtr;
    sdpstExtDesc      * sLstExtDesc;
    scPageID            sLstPIDInExt;
    scPageID            sLstExtDirPID;
    SShort              sLstSlotNoInExtDir;
    sdpstStack          sNewHWMStack;
    smLSN               sNTA;
    ULong               sNTAData[5];
    sdRID               sExtRID4HWM;
    sdpstSegCache     * sSegCache;
    sdpstSegHdr       * sSegHdr;

    IDE_ASSERT( aSegHandle      != NULL );
    IDE_ASSERT( aFstAllocPID    != SD_NULL_RID );
    IDE_ASSERT( aLstAllocExtRID != SD_NULL_RID );
    IDE_ASSERT( aLstPID         != SD_NULL_PID );
    IDE_ASSERT( aStartInfo      != NULL );

    sSegPID   = aSegHandle->mSegPID;
    sSegCache = (sdpstSegCache *)(aSegHandle->mCache);

    IDE_ASSERT( sSegPID != SD_NULL_PID );


    /* Serial Direct-Path Insert시에는 HWM를 옮기기전에
     * merge된 영역의 bitmap을 변경한다. */
    IDE_TEST( sdpstDPath::updateBMPUntilHWM( aStatistics,
                                             aSpaceID,
                                             sSegPID,
                                             aFstAllocPID,
                                             aLstPID,
                                             aStartInfo ) != IDE_SUCCESS );

    /* 마지막 ExtDesc을 얻어와 마지막 PID를 계산한다. */
    IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                          aSpaceID,
                                          aLstAllocExtRID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          NULL, /* sdrMtx */
                                          (UChar**)&sLstExtDesc,
                                          NULL  /* aTruSuccess */ )
              != IDE_SUCCESS );
    sState = 1;

    sPagePtr = sdpPhyPage::getPageStartPtr( sLstExtDesc );

    sLstPIDInExt       = sLstExtDesc->mExtFstPID + sLstExtDesc->mLength - 1;
    sLstExtDirPID      = sdpPhyPage::getPageID( sPagePtr );
    sLstSlotNoInExtDir = sdpstExtDir::calcOffset2SlotNo(
                                   sdpstExtDir::getHdrPtr( sPagePtr ),
                                   SD_MAKE_OFFSET( aLstAllocExtRID ) );

    /* 검증 */
    if ( (sLstExtDirPID != SD_MAKE_PID( aLstAllocExtRID ) ) ||
         (sLstSlotNoInExtDir > sdpstExtDir::getHdrPtr(sPagePtr)->mExtCnt - 1) )
    {
        ideLog::log( IDE_SERVER_0,
                     "LstAllocExtDirPID: %u, "
                     "ExtentCnt: %u, "
                     "ExtFstPID: %u, "
                     "ExtLstPID: %u, "
                     "SlotNoInExtDir: %d, "
                     "LstPID: %u\n",
                     SD_MAKE_PID( aLstAllocExtRID ),
                     sdpstExtDir::getHdrPtr(sPagePtr)->mExtCnt,
                     sLstExtDesc->mExtFstPID,
                     sLstPIDInExt,
                     aLstPID );
        sdpstExtDir::dump( sPagePtr );
        IDE_ASSERT( 0 );
    }

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sLstExtDesc )
              != IDE_SUCCESS );

    /* 마지막 위치에 대한 Stack을 생성한다. */
    sdpstStackMgr::initialize( &sNewHWMStack );
    IDE_TEST( sdpstAllocPage::makeOrderedStackFromDataPage(
                                         aStatistics,
                                         aSpaceID,
                                         sSegPID,
                                         sLstPIDInExt,
                                         &sNewHWMStack ) != IDE_SUCCESS );

    sNTA = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );

    /* 마지막 할당된 페이지 정보와 HWM을 갱신한다. */
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
    sState = 1;

    /* 마지막 할당된 페이지 정보 갱신
     * 마지막 alloc page는 마지막으로 DPath Insert된 페이지이다. */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aLstPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          &sMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sdpstCache::setLstAllocPage( aStatistics,
                                 aSegHandle,
                                 aLstPID,
                                 sdpPhyPage::getSeqNo(
                                     sdpPhyPage::getHdr(sPagePtr) ) );

    /* Segment Header 와 Segment Cache에 HWM 갱신 */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          sSegPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          &sMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sSegHdr = sdpstSH::getHdrPtr( sPagePtr );

    /* 이전 HWM을 임시로 저장한다. */
    calcExtInfo2ExtRID( sSegPID,
                        sPagePtr,
                        sSegHdr->mHWM.mExtDirPID,
                        sSegHdr->mHWM.mSlotNoInExtDir,
                        &sExtRID4HWM );

    sNTAData[0] = aSegHandle->mSegPID;
    sNTAData[1] = sExtRID4HWM;
    sNTAData[2] = sSegHdr->mHWM.mWMPID;

    IDE_TEST( sdpstSH::logAndUpdateWM( &sMtx,
                                       &(sSegHdr->mHWM),
                                       sLstPIDInExt,
                                       sLstExtDirPID,
                                       sLstSlotNoInExtDir,
                                       &sNewHWMStack ) != IDE_SUCCESS );

    sdpstCache::lockHWM4Write( aStatistics, sSegCache );
    sSegCache->mHWM = sSegHdr->mHWM;
    sdpstCache::unlockHWM( sSegCache );

    /* BUG-29203 [SM] DPath Insert 중, HWM 업데이트 직후 서버 KILL 했을 때
     *           restart recovery 중 ASSERT로 비정상 종료
     * rollback하기 위한 logical undo log를 기록한다. */
    sdrMiniTrans::setNTA( &sMtx,
                          aSpaceID,
                          SDR_OP_SDPST_UPDATE_WMINFO_4DPATH,
                          &sNTA,
                          sNTAData,
                          3 /* DataCount */ );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : merge된 영역의 bitmap을 변경한다.
 ***********************************************************************/
IDE_RC sdpstDPath::updateBMPUntilHWM( idvSQL           * aStatistics,
                                      scSpaceID          aSpaceID,
                                      scPageID           aSegPID,
                                      scPageID           aFstAllocPIDOfFM,
                                      scPageID           aLstAllocPIDOfFM,
                                      sdrMtxStartInfo  * aStartInfo )
{
    sdpstMFNL          sNewMFNL;
    idBool             sIsFinish;
    sdpstBMPType       sDepth;
    sdpstBMPType       sPrvDepth;
    SShort             sLfFstSlotNo;
    SShort             sItFstSlotNo;
    sdpstStack         sFstPageStack;
    sdpstStack         sLstPageStack;
    sdpstStack       * sCurStack;

    IDE_ASSERT( aSegPID          != SD_NULL_PID );
    IDE_ASSERT( aLstAllocPIDOfFM != SD_NULL_RID );
    IDE_ASSERT( aStartInfo       != NULL );

    /* 기존 첫번째로 할당했던 페이지의 Stack 생성한다. */
    sdpstStackMgr::initialize( &sFstPageStack );
    IDE_TEST( sdpstAllocPage::makeOrderedStackFromDataPage(
                                         aStatistics,
                                         aSpaceID,
                                         aSegPID,
                                         aFstAllocPIDOfFM,
                                         &sFstPageStack ) != IDE_SUCCESS );

    /* 마지막 생성했던 페이지에 대한 Stack 을 생성한다. */
    sdpstStackMgr::initialize( &sLstPageStack );
    IDE_TEST( sdpstAllocPage::makeOrderedStackFromDataPage(
                                         aStatistics,
                                         aSpaceID,
                                         aSegPID,
                                         aLstAllocPIDOfFM,
                                         &sLstPageStack ) != IDE_SUCCESS );

    /* Append를 위해서 새로 할당된 페이지가 없을 때는 본 함수를
     * 호출하지 않는다. */
    if ( sdpstStackMgr::compareStackPos( &sLstPageStack,
                                         &sFstPageStack ) < 0 )
    {
        ideLog::log( IDE_SERVER_0, "========= Lst Page Stack ========\n" );
        sdpstStackMgr::dump( &sLstPageStack );
        ideLog::log( IDE_SERVER_0, "========= Fst Page Stack ========\n" );
        sdpstStackMgr::dump( &sFstPageStack );
        IDE_ASSERT( 0 );
    }

    /*
     * 다음은 페이지 bitmap 변경하는 알고리즘이다.
     *
     * A-update.
     * 만약, 마지막 페이지부터 시작했다면, 다음 leaf를 찾는다. => B-move
     * 동일한 leaf에 New HHWM가 없다면, 마지막 페이지까지 data 페이지만
     * 모두 Full상태로 변경한다. 그 후 다음 leaf를 찾는다. => B-move
     * 하지만, New HHWM가 있다면 New HHWM까지 Full상태로 변경하고,
     * 상위로 올라간다. => B-move
     *
     * B-move.
     * 동일한 internal에 다음 leaf가 New HHWM이거나 그렇지 않거나
     * 다음 leaf로 진행한다. => A-update
     *
     * B-update.
     * New HHWM 이면 해당 lf slot을 주어진 MFNL로 변경하고,
     * 상위로 진행한다. => C-move
     * 만약, 다음 leaf가 없다면, lfslot들의 MFNL을 FULL로 변경하고,
     * 상위로 진행한다. => C-move
     *
     * C-move.
     * 동일한 root에 다음 internal이 New HHWM이거나 그렇지 않거나
     * 다음 internal로 진행한다. => B-move
     *
     * C-update.
     * New HHWM이면 itslot의 MFNL을 주어진 MFNL로 변경하고
     * 연산을 완료한다. => end
     * 만약, 다음 internal이 없다면 itslot들의 MFNL을 FULL로 변경하고,
     * 다음 root로 이동한다. => C-move
     */

    /* LeafBMP 및 InternalBMP에 대한 SlotNumber.
     * 이후 BMP의 MFNL을 갱신할때, 어디서 부터 갱신해야 할지를 Traverse
     * 하는 동안 보관한다. */
    sLfFstSlotNo = SDPST_INVALID_SLOTNO;
    sItFstSlotNo = SDPST_INVALID_SLOTNO;
    sIsFinish    = ID_FALSE;

    sCurStack   = &sFstPageStack;

    /* Lf-BMP에서 시작하므로 이전 depth를 It-BMP로 설정해 놓는다. */
    sPrvDepth   = SDPST_ITBMP;
    if ( sdpstStackMgr::getDepth( sCurStack ) != SDPST_LFBMP )
    {
        sdpstStackMgr::dump( sCurStack );
        IDE_ASSERT( 0 );
    }

    while( 1 )
    {
        sDepth = sdpstStackMgr::getDepth( sCurStack );

        if ( (sDepth != SDPST_VIRTBMP) &&
             (sDepth != SDPST_LFBMP) &&
             (sDepth != SDPST_ITBMP) &&
             (sDepth != SDPST_RTBMP) )
        {
            sdpstStackMgr::dump( sCurStack );
            IDE_ASSERT( 0 );
        }

        IDE_TEST( sdpstBMP::mBMPOps[sDepth]->mUpdateBMPUntilHWM(
                                                aStatistics,
                                                aStartInfo,
                                                aSpaceID,
                                                aSegPID,
                                                &sLstPageStack,
                                                sCurStack,
                                                &sLfFstSlotNo,
                                                &sItFstSlotNo,
                                                &sPrvDepth,
                                                &sIsFinish,
                                                &sNewMFNL ) != IDE_SUCCESS );

        /* 종료 조건 */
        if ( (sDepth == SDPST_RTBMP) && (sIsFinish == ID_TRUE) )
        {
            break;
        }
    }

    /* 이 값들은 MFNL을 수정할 필요가 있을때 설정된다. 따라서 INVALID_SLOTNO, 즉
     * -1이 아닌 다른 값일 경우, 수정할 필요가 있는데 수정을 안한 경우가 된다*/
    if( ( sLfFstSlotNo != SDPST_INVALID_SLOTNO ) ||
        ( sItFstSlotNo != SDPST_INVALID_SLOTNO ) )
    {
        ideLog::log( IDE_SERVER_0, 
                     "LeafSlot     - %d\n"
                     "InternalSlot - %d\n"
                     "SpaceID      - %u\n"
                     "SegPID       - %u\n",
                     sLfFstSlotNo,
                     sItFstSlotNo,
                     aSpaceID,
                     aSegPID );
        sdpstStackMgr::dump( sCurStack );
        sdpstStackMgr::dump( &sLstPageStack );
        IDE_ASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Direct-Path 연산에서 할당된 페이지 bitmap을 update 한다.
 ***********************************************************************/
IDE_RC sdpstDPath::updateBMPUntilHWMInLfBMP(
                                       idvSQL            * aStatistics,
                                       sdrMtxStartInfo   * aStartInfo,
                                       scSpaceID           aSpaceID,
                                       scPageID            /*aSegPID*/,
                                       sdpstStack        * aHWMStack,
                                       sdpstStack        * aCurStack,
                                       SShort            * /*aFmLfSlotNo*/,
                                       SShort            * /*aFmItSlotNo*/,
                                       sdpstBMPType      * aPrvDepth,
                                       idBool            * aIsFinish,
                                       sdpstMFNL         * aNewMFNL )
{
    UInt                 sState = 0;
    smLSN                sNTA;
    sdrMtx               sMtx;
    ULong                sArrData[4];
    SShort               sFmPBSNo;
    SShort               sToPBSNo;
    sdpstPosItem         sHwmPos;
    sdpstPosItem         sCurPos;
    UShort               sMetaPageCnt;
    UShort               sPageCnt;
    SShort               sDist;
    idBool               sIsFinish;
    idBool               sNeedToChangeMFNL;
    UChar              * sPagePtr;
    sdpstLfBMPHdr      * sLfBMPHdr;
    sdpstRedoUpdateLfBMP4DPath sLogData;

    IDE_ASSERT( aHWMStack  != NULL );
    IDE_ASSERT( aCurStack  != NULL );
    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aNewMFNL   != NULL );
    IDE_ASSERT( aIsFinish  != NULL );
    IDE_ASSERT( *aPrvDepth == SDPST_ITBMP);

    /* Lf-BMP에서 HWM을 만나기 전까지는 계속 다음 단계로 진행한다. */
    sIsFinish  = ID_FALSE;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    // HWM가 같은 leaf 페이지에 있는가?
    sDist = sdpstBMP::mBMPOps[SDPST_LFBMP]->mGetDistInDepth(
                           sdpstStackMgr::getAllPos(aHWMStack),
                           sdpstStackMgr::getAllPos(aCurStack) );

    if ( sDist < 0 )
    {
        sdpstStackMgr::dump( aHWMStack );
        sdpstStackMgr::dump( aCurStack );
        IDE_ASSERT( 0 );
    }

    sCurPos = sdpstStackMgr::getCurrPos( aCurStack );
    IDE_ASSERT( sCurPos.mNodePID != SD_NULL_PID );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          sCurPos.mNodePID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          &sMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sLfBMPHdr = sdpstLfBMP::getHdrPtr( sPagePtr );

    if ( sdpstLfBMP::isLstPBSNo( sPagePtr, sCurPos.mIndex ) == ID_TRUE )
    {
        // 마지막 페이지라면 다음 leaf로 이동해야한다. 하지만, New HHWM
        // 라면, GoNext가 ID_FALSE로 설정되어 상위로 올라가야한다.
        // 아래부분에서 처리한다.
        *aNewMFNL = sLfBMPHdr->mBMPHdr.mMFNL;
    }

    /* 변경할 구간을 찾는다. */
    sFmPBSNo = sCurPos.mIndex;
    if ( sDist == SDPST_FAR_AWAY_OFF )
    {
        // HHWM와 동일한 페이지가 아닌경우 leaf bmp페이지의 마지막까지
        // 검사한다.
        sToPBSNo = sLfBMPHdr->mTotPageCnt - 1;
    }
    else
    {
        // HWM와 동일한 페이지인 경우 HWM까지만 탐색한다.
        sHwmPos  = sdpstStackMgr::getSeekPos( aHWMStack, SDPST_LFBMP );
        sToPBSNo = sHwmPos.mIndex;

        // HHWM를 만났기 때문에 연산을 root에서 완료시켜야한다.
        sIsFinish = ID_TRUE;
    }

    sPageCnt = sToPBSNo - sFmPBSNo + 1;

    /* Lf-BMP 영역을 벗어난 경우에는 PBS를 변경하지 않는다. */
    if ( sPageCnt == 0 )
    {
        IDE_CONT( leave_update_bmp_in_lfbmp );
    }
    else
    {
        IDE_ASSERT( sPageCnt > 0 );
    }

    /* leaf에서의 bitmap 변경에 대한 Rollback 연산을 위해서 여기서
     * NTA를 설정한다. */
    sNTA = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );

    /* from -> to 까지 PBS 갱신 */
    sdpstLfBMP::updatePBS( sdpstLfBMP::getMapPtr(sLfBMPHdr),
                                      sFmPBSNo,
                                      (sdpstPBS)(SDPST_BITSET_PAGETP_DATA |
                                                 SDPST_BITSET_PAGEFN_FUL),
                                      sPageCnt,
                                      &sMetaPageCnt );

    sLogData.mFmPBSNo = sFmPBSNo;
    sLogData.mPageCnt = sPageCnt;

    IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx,
                                         (UChar*)sdpstLfBMP::getMapPtr(sLfBMPHdr),
                                         &sLogData,
                                         ID_SIZEOF( sLogData ),
                                         SDR_SDPST_UPDATE_LFBMP_4DPATH )
              != IDE_SUCCESS );



    /* Lf-BMP 페이지의 MFNL Table과 대표 MFNL을 갱신한다. */
    IDE_TEST( sdpstBMP::logAndUpdateMFNL(
                                    &sMtx,
                                    sdpstLfBMP::getBMPHdrPtr(sLfBMPHdr),
                                    SDPST_INVALID_SLOTNO,
                                    SDPST_INVALID_SLOTNO,
                                    SDPST_MFNL_FMT,
                                    SDPST_MFNL_FUL,
                                    sPageCnt - sMetaPageCnt,
                                    aNewMFNL,
                                    &sNeedToChangeMFNL ) != IDE_SUCCESS );

    sArrData[0] = sCurPos.mNodePID;
    sArrData[1] = sFmPBSNo;
    sArrData[2] = sToPBSNo;
    sArrData[3] = sLfBMPHdr->mBMPHdr.mMFNL;

    IDE_DASSERT( sdpstLfBMP::verifyBMP( sLfBMPHdr ) == IDE_SUCCESS );

    sdrMiniTrans::setNTA( &sMtx,
                          aSpaceID,
                          SDR_OP_SDPST_UPDATE_MFNL_4DPATH,
                          &sNTA,
                          sArrData,
                          4 /* DataCount */ );

    IDE_EXCEPTION_CONT( leave_update_bmp_in_lfbmp );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    sdpstStackMgr::pop( aCurStack );

    *aPrvDepth = SDPST_LFBMP;
    *aIsFinish = sIsFinish;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct-Path 연산에서 할당된 페이지 Bitmap들을
 *               갱신한다.
 ***********************************************************************/
IDE_RC sdpstDPath::updateBMPUntilHWMInRtAndItBMP(
                                       idvSQL            * aStatistics,
                                       sdrMtxStartInfo   * aStartInfo,
                                       scSpaceID           aSpaceID,
                                       scPageID            /*aSegPID*/,
                                       sdpstStack        * aHWMStack,
                                       sdpstStack        * aCurStack,
                                       SShort            * aFmLfSlotNo,
                                       SShort            * aFmItSlotNo,
                                       sdpstBMPType      * aPrvDepth,
                                       idBool            * aIsFinish,
                                       sdpstMFNL         * aNewMFNL )
{
    UInt                 sState = 0;
    sdrMtx               sMtx;
    UChar              * sPagePtr;
    SShort             * sFmSlotNo;
    SShort               sToSlotNo = 0;
    SShort               sNxtSlotNo;
    sdpstPosItem         sCurPos;
    SShort               sDist;
    sdpstBMPHdr        * sBMPHdr;
    idBool               sIsUpdateMFNL;
    idBool               sIsLstSlot;
    idBool               sGoDown;
    sdrMtxStartInfo      sStartInfo;
    sdpstBMPType         sCurDepth;

    IDE_ASSERT( aHWMStack   != NULL );
    IDE_ASSERT( aCurStack   != NULL );
    IDE_ASSERT( aStartInfo  != NULL );
    IDE_ASSERT( aFmLfSlotNo != NULL );
    IDE_ASSERT( aFmItSlotNo != NULL );
    IDE_ASSERT( aNewMFNL    != NULL );
    IDE_ASSERT( aIsFinish   != NULL );
    IDE_ASSERT( (*aPrvDepth == SDPST_LFBMP) ||
                (*aPrvDepth == SDPST_ITBMP) ||
                (*aPrvDepth == SDPST_RTBMP) ||
                (*aPrvDepth == SDPST_VIRTBMP) );

    sIsUpdateMFNL = ID_FALSE;
    sCurDepth     = sdpstStackMgr::getDepth( aCurStack );

    /* HWM 이 동일 It-BMP에 존재하는가? */
    sDist = sdpstBMP::mBMPOps[sCurDepth]->mGetDistInDepth(
                           sdpstStackMgr::getAllPos(aHWMStack),
                           sdpstStackMgr::getAllPos(aCurStack) );

    /* 내려올때는 sDist > 0, 올라올때는 sDist >= 0 */
    if ( sDist < 0 )
    {
        sdpstStackMgr::dump( aHWMStack );
        sdpstStackMgr::dump( aCurStack );
        IDE_ASSERT( 0 );
    }

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
    sState = 1;

    sCurPos = sdpstStackMgr::getCurrPos( aCurStack );

    /* BUG-32164 [sm-disk-page] If Table Segment is expanded to 12gb by
     * Direct-path insert operation, Server fatal
     * 현재가 RootBMP이면 internalSlotNumber를 사용하고,
     * 현재가 InternalBMP이면 LeafSlotNumber를 사용한다. */
    if( sCurDepth == SDPST_ITBMP )
    {
        sFmSlotNo = aFmLfSlotNo;
        sToSlotNo = sCurPos.mIndex;
    }
    else
    {
        sFmSlotNo = aFmItSlotNo;
        sToSlotNo = sCurPos.mIndex;
    }

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          sCurPos.mNodePID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          &sMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );
    sState = 1;

    sBMPHdr = sdpstBMP::getHdrPtr( sPagePtr );

    sIsLstSlot = sdpstBMP::isLstSlotNo( sPagePtr, sCurPos.mIndex );

    // [1] Bitmap 변경여부를 조사한다.
    // [1-1] Bitmap을 변경해야할 마지막 ToSlotIdx를 구한다.
    if ( SDPST_BMP_CHILD_TYPE( sCurDepth ) == *aPrvDepth )
    {
        // * leaf에서 올라온 경우 *
        // 현재 slot이 마지막이거나 New HHWM라면
        // 현재 internal bmp의 변경하지 않았던 lf slot들에 대해서
        // 한번에 mfnl 변경을 수행한다.

        /*    |
         * +-----+-+-+-+-+  +-----+-+-+-+
         * |0IBMP|0|1|2|3|--|1IBMP|0|1|2|
         * +-----+-+-+-+-+  +-----+-+-+-+
         *        | | | |          | | | 
         *  <-Case2-2-> |          <Case1>
         *        <Case2-1>
         * 각 IBMP의 마지막 Slot을 갱신하는 경우라면, MFNL 갱신*/

        if ( sDist == 0 )
        {
            /* CASE 1 */
            // leaf에서 New HHWM까지 bitmap을 변경하고 올라가는 중이다.
            // 더이상 next로 가지 않는다.
            IDE_ASSERT( *aIsFinish == ID_TRUE );

            sIsUpdateMFNL = ID_TRUE;
        }
        else
        {
            if ( sIsLstSlot == ID_TRUE )
            {
                /* CASE 2-1 */
                // 마지막 Slot이면 aFmLfSlotIdx 부터 끝까지
                // Full로 변경한다.
                IDE_ASSERT( sCurPos.mIndex == sBMPHdr->mSlotCnt - 1  );
                sIsUpdateMFNL = ID_TRUE;
            }
            else
            {
                /* CASE 2-2 */
                // Leaf에서 올라왔는데 마지막이 아니면
                // 다음 leaf로 내려간다.
                sIsUpdateMFNL = ID_FALSE;
            }
        }
    }
    else
    {
        /* 상위 BMP에서 내려오는 것이라면 무조건 내려간다. */
    }

    // [1-2] Bitmap 변경 시작을 설정한다.
    if ( *sFmSlotNo == SDPST_INVALID_SLOTNO )
    {
        // root에서 처음 내려온 경우 혹은
        // leaf에서 올라오는 중에는 bitmap 변경시작 LfBMP slot의
        // 순번을 설정한다.
        // 만약, 이미 이전 HWM가 포함된 LfBMP 부터 pageBitmap을
        // 중복설정하여도 문제는 없다.
        *sFmSlotNo = sCurPos.mIndex;
    }

    // [1-3] 이전이 RtBMP 이던 LfBMP이던 MFNL이 FULL아 아닌경우
    if ( *aNewMFNL != SDPST_MFNL_FUL )
    {
        if( *aPrvDepth == SDPST_BMP_CHILD_TYPE( sCurDepth ) )
        {
            // leaf의 mfnl이 full이 아닌 경우에는 바로 lfslot의
            // mfnl을 변경한다.
            sIsUpdateMFNL = ID_TRUE;
        }
    }
    else
    {
        // 이전이 RtBMP 이던 LfBMP이던 MFNL이 FULL인 경우
    }

    // [3] Bitmap을 변경한다.
    if ( sIsUpdateMFNL == ID_TRUE )
    {
        sdrMiniTrans::makeStartInfo( &sMtx, &sStartInfo );

        IDE_ASSERT( sToSlotNo >= *sFmSlotNo );
        IDE_ASSERT( *sFmSlotNo != SDPST_INVALID_SLOTNO );

        IDE_TEST( sdpstDPath::updateMFNLToFull4DPath( aStatistics,
                                    &sStartInfo,
                                    sBMPHdr,
                                    *sFmSlotNo,
                                    sToSlotNo,
                                    aNewMFNL ) != IDE_SUCCESS );

        // update를 하였기 때문에 sFmSlotNo를 초기화해준다.
        *sFmSlotNo = SDPST_INVALID_SLOTNO;
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    if ( (sIsLstSlot == ID_TRUE) || (*aIsFinish == ID_TRUE) )
    {
        if ( *aPrvDepth == SDPST_BMP_CHILD_TYPE( sCurDepth ) )
        {
            sdpstStackMgr::pop( aCurStack );
            sGoDown = ID_FALSE;
        }
        else
        {
            sGoDown = ID_TRUE;
        }
    }
    else
    {
        sGoDown = ID_TRUE;
    }

    if ( sGoDown == ID_TRUE )
    {
        /* 하위 BMP에서 올라온 경우 다음 하위 BMP로 내려가야 한다.
         * (다음 Slot으로 이동)
         * 상위 BMP에서 내려온 경우 그냥 바로 내려간다. */
        if ( *aPrvDepth == SDPST_BMP_CHILD_TYPE( sCurDepth ) )
        {
            sNxtSlotNo = sCurPos.mIndex + 1;
        }
        else
        {
            sNxtSlotNo = sCurPos.mIndex;
        }

        sdpstStackMgr::setCurrPos( aCurStack,
                                   sCurPos.mNodePID,
                                   sNxtSlotNo );

        sdpstStackMgr::push(
            aCurStack,
            sdpstBMP::getSlot(sBMPHdr, sNxtSlotNo)->mBMP,
            0 );
    }

    *aPrvDepth = sCurDepth;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Direct-Path 연산에서 할당된 페이지 Bitmap들을
 *               갱신하기 위해, 다음 BMP를 push하여 넘겨준다.
 ***********************************************************************/
IDE_RC sdpstDPath::updateBMPUntilHWMInVtBMP(
                                       idvSQL            * aStatistics,
                                       sdrMtxStartInfo   * /*aStartInfo*/,
                                       scSpaceID           aSpaceID,
                                       scPageID            aSegPID,
                                       sdpstStack        * aHWMStack,
                                       sdpstStack        * aCurStack,
                                       SShort            * /*aFmSlotNo*/,
                                       SShort            * /*aFmItSlotNo*/,
                                       sdpstBMPType      * aPrvDepth,
                                       idBool            * /*aIsFinish*/,
                                       sdpstMFNL         * /*aNewMFNL*/ )
{
    sdpstPosItem         sCurPos;
    SShort               sNxtSlotNo;
    scPageID             sCurRtBMP;
    UChar              * sPagePtr;
    sdpstBMPHdr        * sBMPHdr;
    UInt                 sState = 0;
    UInt                 i;

    IDE_ASSERT( aCurStack != NULL );
    IDE_ASSERT( aPrvDepth != NULL );

    /* VirtualBMP는 실제하지 않는 BMP로 Root의 위치를 가리키기 위해 존재한다. 즉
     * VirtualBMP의 0번 Slot은 첫번째 RootBMP, 1번 Slot은 다음 RootBMP를
     * 의미한다. 따라서 VirtualBMP는 BMP를 갱신(MFNL 수정)을 하지 않아도 되며,
     * 오로지 다음번 Root만 탐색하면 된다.
     * 이때 다음번 Root는 첫번째 Root가 존재하는 SegmentPage로부터 NextRoot를
     * 찾아 탐색하면 된다. */

    /* 최초의 Root는 무조건 SegmentPage에 있다. */
    sCurRtBMP = aSegPID;
    IDE_ASSERT( sCurRtBMP != SD_NULL_PID );

    /* 다음번 Root를 향해 탐색  */
    sCurPos   = sdpstStackMgr::getCurrPos( aCurStack );
    sNxtSlotNo = sCurPos.mIndex + 1;

    for( i = 0 ; i < (UInt)sNxtSlotNo ; i ++ )
    {
        IDE_TEST( sdbBufferMgr::getPage( aStatistics,
                                         aSpaceID,
                                         sCurRtBMP,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sPagePtr,
                                         NULL  /* aTrySuccess */ )
                  != IDE_SUCCESS );
        sState = 1;

        sBMPHdr   = sdpstBMP::getHdrPtr( sPagePtr );
        sCurRtBMP = sdpstRtBMP::getNxtRtBMP( sBMPHdr );

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sPagePtr )
                  != IDE_SUCCESS );

        /* NxtSlotNo가 실제 Root개수보다 클 경우 */
        if( sCurRtBMP == SC_NULL_PID )
        {
            ideLog::log( IDE_SERVER_0, 
                         "i            - %d\n"
                         "sNxtSlotNo   - %d\n"
                         "SpaceID      - %u\n"
                         "SegPID       - %u\n",
                         i,
                         sNxtSlotNo,
                         aSpaceID,
                         aSegPID );
            sdpstStackMgr::dump( aCurStack );
            sdpstStackMgr::dump( aHWMStack );

            IDE_ASSERT( sCurRtBMP == SC_NULL_PID );
        }
    }

    /* 다음 Slot을 가리키도록 조정한다. */
    sdpstStackMgr::setCurrPos( aCurStack,
                               sCurPos.mNodePID,
                               sNxtSlotNo );

    sdpstStackMgr::push(
        aCurStack,
        sCurRtBMP,   /* 다음번 Bmp */
        0 );         /* Slot Number */

    *aPrvDepth = SDPST_VIRTBMP;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                               (UChar*)sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


