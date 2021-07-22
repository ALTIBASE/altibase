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
 * $Id: sdpstAllocPage.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 가용공간 할당 연산 관련 STATIC
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

/***********************************************************************
 * Description : [INTERFACE] Segment를 탐색하여 새 페이지를 할당한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] Table Space의 ID
 * aSegHandle   - [IN] Segment의 Handle
 * aPageType    - [IN] 할당하려는 페이지의 타입
 * aNewPagePtr  - [OUT] 할당한 페이지의 Pointer
 ***********************************************************************/
IDE_RC sdpstAllocPage::allocateNewPage( idvSQL             * aStatistics,
                                        sdrMtx             * aMtx,
                                        scSpaceID            aSpaceID,
                                        sdpSegHandle       * aSegHandle,
                                        sdpPageType          aPageType,
                                        UChar             ** aNewPagePtr )
{
    UChar                * sPagePtr;
    scPageID               sNewHintDataPID;
    UChar                  sCTSlotNo;

    IDE_DASSERT( aSegHandle  != NULL );
    IDE_DASSERT( aMtx        != NULL );
    IDE_DASSERT( aNewPagePtr != NULL );

    sPagePtr   = NULL;
    sCTSlotNo = SDP_CTS_IDX_NULL;

    /* Segment의 가용공간을 탐색한다.
     * Slot을 탐색하면서, 페이지를 생성하게 되면 Table 페이지타입으로
     * 초기화하여 반환한다. */
    IDE_TEST( sdpstFindPage::searchFreeSpace( aStatistics,
                                              aMtx,
                                              aSpaceID,
                                              aSegHandle,
                                              0, /* unuse rowsize */
                                              aPageType,
                                              SDPST_SEARCH_NEWPAGE,
                                              &sPagePtr,
                                              &sCTSlotNo,
                                              &sNewHintDataPID )
              != IDE_SUCCESS );

    // 반드시 페이지를 생성하여 반환한다. 공간부족으로 생성하지 못하는
    // 경우에는 Exception이 발생한다.
    IDE_ASSERT( sPagePtr   != NULL );

    *aNewPagePtr = sPagePtr;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : [ INTERFACE ] 요청된 Free 페이지 개수를 충족할 수 있도록
 *               Segment에 Extent를 미리 확보한다.
 *
 * 본 함수를 호출할때에는 동시에 다른 트랜잭션이 해당 Segment에서
 * 가용공간을 할당해 갈 수 없다. 즉, 동시성을 고려하지 않아도 된다.
 * 인덱스에서만 사용한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] Table Space의 ID
 * aSegHandle   - [IN] Segment의 Handle
 * aCountWanted - [IN] 필요한 페이지 개수
 ***********************************************************************/
IDE_RC sdpstAllocPage::prepareNewPages( idvSQL            * aStatistics,
                                        sdrMtx            * aMtx,
                                        scSpaceID           aSpaceID,
                                        sdpSegHandle      * aSegHandle,
                                        UInt                aCountWanted )
{
    SInt              sState = 0 ;
    UChar           * sPagePtr;
    sdrMtxStartInfo   sStartInfo;
    sdpstSegHdr     * sSegHdr;
    ULong             sFreePageCnt;

    IDE_DASSERT( aSegHandle!= NULL );
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( aCountWanted > 0 );

    sdrMiniTrans::makeStartInfo ( aMtx, &sStartInfo );

    while(1)
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                              aSpaceID,
                                              aSegHandle->mSegPID,
                                              &sPagePtr ) != IDE_SUCCESS );
        sState = 1;

        sSegHdr = sdpstSH::getHdrPtr(sPagePtr);

        sFreePageCnt = sSegHdr->mFreeIndexPageCnt;

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                  != IDE_SUCCESS );

        /* Free페이지가 부족하면 새로운 Extent를 TBS로 부터 요구한다. */
        if( sFreePageCnt < (ULong)aCountWanted )
        {
            IDE_TEST( sdpstSegDDL::allocateExtents(
                                   aStatistics,
                                   &sStartInfo,
                                   aSpaceID,
                                   aSegHandle,
                                   aSegHandle->mSegStoAttr.mNextExtCnt )
                      != IDE_SUCCESS );
        }
        else
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :  PageBitSet을 변경하면서 상위 bitmap 페이지의 MFNL을
 *                변경한다
 *
 * aStatistics     - [IN] 통계정보
 * aMtx            - [IN] Mini Transaction Pointer
 * aSpaceID        - [IN] Table Space의 ID
 * aSegHandle      - [IN] Segment의 Handle
 * aChangePhase    - [IN] MFNL을 변경할 BMP 단계
 * aChildPID       - [IN] aCurPID의 하위 PID
 * aPBSNo          - [IN] PageBitSet No
 * aChageState     - [IN] 변경할 State (PBS) LfBMP에서 사용
 * aCurPID         - [IN] 현재 BMP PID
 * aSlotNoInParent - [IN] 부모 BMP에 존재하는 현 BMP의 slot No
 * aRevStack       - [IN] Reverse Stack
 ***********************************************************************/
IDE_RC sdpstAllocPage::tryToChangeMFNLAndItHint(
                                idvSQL               * aStatistics,
                                sdrMtx               * aMtx,
                                scSpaceID              aSpaceID,
                                sdpSegHandle         * aSegHandle,
                                sdpstChangeMFNLPhase   aChangePhase,
                                scPageID               aChildPID,
                                SShort                 aPBSNo,
                                void                 * aChangeState,
                                scPageID               aCurPID,
                                SShort                 aSlotNoInParent,
                                sdpstStack           * aRevStack )
{
    sdpstMFNL                sNewMFNL;
    scPageID                 sChildPID;
    scPageID                 sCurPID;
    SShort                   sSlotNoInParent;
    idBool                   sNeedToChangeMFNL;
    scPageID                 sParentPID;
    sdpstSegCache          * sSegCache;

    IDE_DASSERT( aSegHandle   != NULL );
    IDE_DASSERT( aMtx         != NULL );
    IDE_DASSERT( aChangeState != NULL );
    IDE_DASSERT( aRevStack    != NULL );

    sChildPID = aChildPID;
    sCurPID   = aCurPID;
    sSlotNoInParent = aSlotNoInParent;

    sNeedToChangeMFNL = ID_TRUE; // 처음에는 무조건 TRUE이다.

    if ( aChangePhase != SDPST_CHANGEMFNL_LFBMP_PHASE )
    {
        sNewMFNL = *(sdpstMFNL*)(aChangeState);
    }

    switch( aChangePhase )
    {
        case SDPST_CHANGEMFNL_LFBMP_PHASE:

            IDE_ASSERT( aPBSNo != SDPST_INVALID_PBSNO );

            // Leaf BMP 페이지의 가용도를 변경한다.
            IDE_TEST( sdpstLfBMP::tryToChangeMFNL(
                          aStatistics,
                          aMtx,
                          aSpaceID,
                          sChildPID,
                          aPBSNo,
                          *(sdpstPBS*)(aChangeState),
                          &sNeedToChangeMFNL,
                          &sNewMFNL,
                          &sParentPID,
                          &sSlotNoInParent ) != IDE_SUCCESS );

            sCurPID = sParentPID;

        case SDPST_CHANGEMFNL_ITBMP_PHASE:
            if ( sNeedToChangeMFNL == ID_TRUE )
            {
                // push (itbmp, lfslotidx)
                sdpstStackMgr::push( aRevStack,
                                     sCurPID,
                                     sSlotNoInParent );

                // Internal BMP 페이지의 가용도를 변경한다.
                IDE_TEST( sdpstBMP::tryToChangeMFNL( aStatistics,
                                                     aMtx,
                                                     aSpaceID,
                                                     sCurPID,
                                                     sSlotNoInParent,
                                                     sNewMFNL,
                                                     &sNeedToChangeMFNL,
                                                     &sNewMFNL,
                                                     &sParentPID,
                                                     &sSlotNoInParent )
                          != IDE_SUCCESS );

                sChildPID = sCurPID;
                sCurPID  = sParentPID;
            }

        case SDPST_CHANGEMFNL_RTBMP_PHASE:

            if ( sNeedToChangeMFNL == ID_TRUE )
            {
                // push (rtbmp, itslotidx)
                sdpstStackMgr::push( aRevStack,
                                     sCurPID,
                                     sSlotNoInParent );

                // Root BMP 페이지의 가용도를 변경한다.
                // Root BMP와 SMH가 동일한 경우에 위에서
                // 미리 획득된 경우가 있지만 X-latch를
                // 2번 획득하는 경우이므로 문제가 되지 않는다.
                IDE_TEST( sdpstBMP::tryToChangeMFNL( aStatistics,
                                                     aMtx,
                                                     aSpaceID,
                                                     sCurPID,
                                                     sSlotNoInParent,
                                                     sNewMFNL,
                                                     &sNeedToChangeMFNL,   /* ignored */
                                                     &sNewMFNL,
                                                     &sParentPID,
                                                     &sSlotNoInParent )
                          != IDE_SUCCESS );

                if ( sNewMFNL == SDPST_MFNL_FUL )
                {
                    // Internal Slot의 MFNL이 Full로 변경된 경우에는
                    // Internal Hint의 변경여부를 검사한다.
                    sSegCache   = (sdpstSegCache*)aSegHandle->mCache;

                    IDE_TEST( sdpstRtBMP::forwardItHint(
                                  aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  sSegCache,
                                  aRevStack,
                                  SDPST_SEARCH_NEWSLOT ) != IDE_SUCCESS );

                    IDE_TEST( sdpstRtBMP::forwardItHint(
                                  aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  sSegCache,
                                  aRevStack,
                                  SDPST_SEARCH_NEWPAGE ) != IDE_SUCCESS );
                }
            }
            break;
        default:
            ideLog::log( IDE_SERVER_0,
                         "aChangePhase: %u\n",
                         aChangePhase );
            IDE_ASSERT( 0 );
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 페이지를 생성하고, 필요한 페이지 타입 초기화 및 logical
 *               Header를 초기화한다.
 ***********************************************************************/
IDE_RC sdpstAllocPage::createPage( idvSQL           * aStatistics,
                                   sdrMtx           * aMtx,
                                   scSpaceID          aSpaceID,
                                   sdpSegHandle     * aSegHandle,
                                   scPageID           aNewPageID,
                                   ULong              aSeqNo,
                                   sdpPageType        aPageType,
                                   scPageID           aParentPID,
                                   SShort             aPBSNoInParent,
                                   sdpstPBS           aPBS,
                                   UChar           ** aNewPagePtr )
{
    UChar         * sNewPagePtr;

    IDE_DASSERT( aNewPageID   != SD_NULL_PID );
    IDE_DASSERT( aParentPID   != SD_NULL_PID );
    IDE_DASSERT( aMtx         != NULL );
    IDE_DASSERT( aNewPagePtr  != NULL );

    IDE_TEST( sdbBufferMgr::createPage( aStatistics,
                                        aSpaceID,
                                        aNewPageID,
                                        aPageType,
                                        aMtx,
                                        &sNewPagePtr ) != IDE_SUCCESS );

    IDE_TEST( formatPageHdr( aMtx,
                             aSegHandle,
                             (sdpPhyPageHdr*)sNewPagePtr,
                             aNewPageID,
                             aSeqNo,
                             aPageType,
                             aParentPID,
                             aPBSNoInParent,
                             aPBS ) != IDE_SUCCESS );

    *aNewPagePtr = sNewPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : phyical hdr와 logical hdr를 page type에 맞게 format한다.
 ***********************************************************************/
IDE_RC sdpstAllocPage::formatPageHdr( sdrMtx           * aMtx,
                                      sdpSegHandle     * aSegHandle4DataPage,
                                      sdpPhyPageHdr    * aNewPagePtr,
                                      scPageID           aNewPageID,
                                      ULong              aSeqNo,
                                      sdpPageType        aPageType,
                                      scPageID           aParentPID,
                                      SShort             aPBSNo,
                                      sdpstPBS           aPBS )
{
    sdpParentInfo    sParentInfo;
    smOID            sTableOID;
    UInt             sIndexID;

    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aNewPagePtr != NULL );
    IDE_ASSERT( aNewPageID  != SD_NULL_PID );

    sParentInfo.mParentPID   = aParentPID;
    sParentInfo.mIdxInParent = aPBSNo;

    if( aSegHandle4DataPage != NULL )
    {
        sTableOID = ((sdpSegCCache*) aSegHandle4DataPage->mCache )->mTableOID;
        sIndexID  = ((sdpSegCCache*) aSegHandle4DataPage->mCache )->mIndexID;
    }
    else
    {
        // Segment Meta Page
        sTableOID = SM_NULL_OID;
        sIndexID  = SM_NULL_INDEX_ID;
    }

    IDE_TEST( sdpPhyPage::logAndInit( aNewPagePtr,
                                      aNewPageID,
                                      &sParentInfo,
                                      (UShort)aPBS,
                                      aPageType,
                                      sTableOID,
                                      sIndexID,
                                      aMtx ) 
              != IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::setSeqNo( aNewPagePtr,
                                    aSeqNo,
                                    aMtx ) 
              != IDE_SUCCESS );


    /* To Fix BUG-23667 [AT-F5 ART] Disk Table Insert시
     * 가용공간 탐색과정에서 FMS FreePageList에 CTL이 초기화되지 않은
     * 경우가 있음 */
    if ( aPageType == SDP_PAGE_DATA )
    {
        if( aSegHandle4DataPage != NULL )
        {
            IDE_TEST( smLayerCallback::logAndInitCTL( aMtx,
                                                      aNewPagePtr,
                                                      aSegHandle4DataPage->mSegAttr.mInitTrans,
                                                      aSegHandle4DataPage->mSegAttr.mMaxTrans )
                      != IDE_SUCCESS );
        }

        IDE_TEST( sdpSlotDirectory::logAndInit(aNewPagePtr,
                                               aMtx)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Data 페이지에서 할당 혹은 페이지에서 가용공간 해제
 *               이후에 페이지의 가용도 변경여부를 확인한다.
 *               Data 페이지의 가용도 변경으로 인해서 상위 BMP페이지의
 *               MFNL을 변경하기도 한다.
 *
 *  aStatistics   - [IN] 통계 정보
 *  aMtx          - [IN] Mini Transaction Pointer
 *  aSpaceID      - [IN] TableSpace ID
 *  aSegHandle    - [IN] Segment Handle
 *  aDataPagePtr  - [IN] insert, update, delete가 발생한 Data Page Ptr
 *
 ***********************************************************************/
IDE_RC sdpstAllocPage::updatePageFN( idvSQL         * aStatistics,
                                     sdrMtx         * aMtx,
                                     scSpaceID        aSpaceID,
                                     sdpSegHandle   * aSegHandle,
                                     UChar          * aDataPagePtr )
{
    sdpPhyPageHdr    * sPageHdr;
    sdpstPBS           sPBS;
    sdpstPBS           sNewPageBitSet;
    sdpParentInfo      sParentInfo;
    sdpstStack         sRevStack;
    sdpstStack         sItHintStack;
    idBool             sIsTurnOn;
    sdpstSegCache    * sSegCache;

    IDE_DASSERT( aMtx         != NULL );
    IDE_DASSERT( aSegHandle   != NULL );
    IDE_DASSERT( aDataPagePtr != NULL );

    sPageHdr = sdpPhyPage::getHdr( aDataPagePtr );

    // 페이지의 가용도 레벨을 조정해야한다면 조정한다.
    if ( sdpstFindPage::needToChangePageFN( sPageHdr,
                                      aSegHandle,
                                      &sNewPageBitSet ) == ID_TRUE )
    {
        // Leaf Bitmap 페이지에서의 후보 Data 페이지의 PageBitSet을 변경한다.
        // 이로인한, 상위에 변경해야할 모든 Bitmap 페이지의
        // MFNL 상태를 변경한다.

        // 페이지 상태 변경이 필요한 경우 처리한다.
        sParentInfo = sdpPhyPage::getParentInfo(aDataPagePtr);
        sPBS        = (sdpstPBS) sdpPhyPage::getState( sPageHdr );

        // 같은 PageBitSet이 들어올수 없다.
        if ( sPBS == sNewPageBitSet )
        {
            ideLog::log( IDE_SERVER_0,
                         "Page ID: %u, "
                         "Before PBS: %u, "
                         "After PBS: %u\n",
                         sdpPhyPage::getPageID( aDataPagePtr ),
                         sPBS,
                         sNewPageBitSet );

            (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                         aDataPagePtr,
                                         "============= Data Page Dump =============\n"  );

            IDE_ASSERT( 0 );
        }

        sSegCache = (sdpstSegCache*)aSegHandle->mCache;
        sdpstStackMgr::initialize(&sRevStack);

        // leaf bmp 페이지부터 MFNL을 변경시도하여 상위노드로 전파한다.
        IDE_TEST( tryToChangeMFNLAndItHint(
                      aStatistics,
                      aMtx,
                      aSpaceID,
                      aSegHandle,
                      SDPST_CHANGEMFNL_LFBMP_PHASE,
                      sParentInfo.mParentPID,  // leaf pid
                      sParentInfo.mIdxInParent,
                      (void*)&sNewPageBitSet,
                      SD_NULL_PID,            // unuse : parent pid
                      SDPST_INVALID_SLOTNO,  // unuse : slotidxinparent
                      &sRevStack ) != IDE_SUCCESS );

        // tryToChangeMFNL을 한 후에 Data 페이지의 가용도를 변경해야한다.
        // 왜냐하면, 위의함수에서는 이전 가용도를 참고하기 때문이다.
        IDE_TEST( sdpPhyPage::setState( sPageHdr, (UShort)sNewPageBitSet, aMtx )
                  != IDE_SUCCESS );

        // FreePage 함수는 별도로 존재하기 때문에 본 함수로 Free Page를
        // 처리하지 않는다.
        IDE_ASSERT( sdpstLfBMP::isEqFN( 
                    sNewPageBitSet, SDPST_BITSET_PAGEFN_FMT ) == ID_FALSE );

        /* 새로운 pagebitset이 가용공간이 늘어난경우( freeslot을 한경우 )
         * segment cache의 it hint가 변경될수 도 있다.
         * sRevStack Depth가 root일때만 변경한다. 왜냐하면 다른 itbmp가 선택되었기
         * 때문이다. */
        if ( sdpstStackMgr::getDepth( &sRevStack ) == SDPST_RTBMP )
        {
            if ( (sPBS & SDPST_BITSET_PAGEFN_MASK) >
                 (sNewPageBitSet & SDPST_BITSET_PAGEFN_MASK) )
            {
                if ( sdpstCache::needToUpdateItHint( sSegCache,
                                                 SDPST_SEARCH_NEWSLOT )
                     == ID_FALSE )
                {
                    sdpstCache::copyItHint( aStatistics,
                                            sSegCache,
                                            SDPST_SEARCH_NEWSLOT,
                                            &sItHintStack );

                    if ( sdpstStackMgr::getDepth( &sItHintStack ) 
                         != SDPST_ITBMP )
                    {
                        ideLog::log( IDE_SERVER_0, "Invalid Hint Stack\n" );

                        sdpstCache::dump( sSegCache );
                        sdpstStackMgr::dump( &sItHintStack );

                        IDE_ASSERT( 0 );
                    }


                    IDE_TEST( sdpstFreePage::checkAndUpdateHintItBMP(
                                                            aStatistics,
                                                            aSpaceID,
                                                            aSegHandle->mSegPID,
                                                            &sRevStack,
                                                            &sItHintStack,
                                                            &sIsTurnOn ) 
                              != IDE_SUCCESS );

                    if ( sIsTurnOn == ID_TRUE )
                    {
                        sdpstCache::setUpdateHint4Slot( sSegCache, ID_TRUE );
                    }
                }
                else
                {
                    // 이미 on되어 있는 상태이므로 skip 한다.
                }
            }
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  할당이 불가능한 페이지를 Full로 상태 변경한다.
 ***********************************************************************/
IDE_RC sdpstAllocPage::updatePageFNtoFull( idvSQL         * aStatistics,
                                           sdrMtx         * aMtx,
                                           scSpaceID        aSpaceID,
                                           sdpSegHandle   * aSegHandle,
                                           UChar          * aDataPagePtr )
{
    sdrMtxStartInfo  sStartInfo;
    sdpstStack       sRevStack;
    sdpParentInfo    sParentInfo;
    sdpstPBS         sNewPBS;
    sdrMtx           sMtx;
    SInt             sState = 0;

    IDE_DASSERT( aSegHandle   != NULL );
    IDE_DASSERT( aDataPagePtr != NULL );
    IDE_DASSERT( aMtx         != NULL );

    sParentInfo = sdpPhyPage::getParentInfo( aDataPagePtr );

    // Data 페이지에 할당이 불가능한 상태이면, Leaf bitmap 페이지를
    // X-latch로 획득하고 페이지상태를 FULL로 변경한다.
    // 변경시 실제 Data 페이지에 대해서도 freeness 상태를 변경해준다.
    sNewPBS = (sdpstPBS) (SDPST_BITSET_PAGETP_DATA | SDPST_BITSET_PAGEFN_FUL );

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdrMiniTrans::setDirtyPage( &sMtx, aDataPagePtr )
              != IDE_SUCCESS );

    sdpPhyPage::setState( (sdpPhyPageHdr*)aDataPagePtr,
                          (UShort)sNewPBS,
                          &sMtx );

    // leaf bmp 페이지부터 MFNL을 변경시도하여 상위노드로 전파한다.
    sdpstStackMgr::initialize( &sRevStack );

    IDE_TEST( tryToChangeMFNLAndItHint(
                        aStatistics,
                        &sMtx,
                        aSpaceID,
                        aSegHandle,
                        SDPST_CHANGEMFNL_LFBMP_PHASE,
                        sParentInfo.mParentPID,     // leafbmp pid
                        sParentInfo.mIdxInParent,
                        (void*)&sNewPBS,
                        SD_NULL_PID,            // unuse : parent pid
                        SDPST_INVALID_SLOTNO,  // unuse : slotidxinparent
                        &sRevStack ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : [ INTERFACE ] 페이지 가용공간 할당 혹은 해제 이후 가용도
 *               변경 freepage 연산은 별도로 처리한다.
 *
 *  aStatistics   - [IN] 통계 정보
 *  aMtx          - [IN] Mini Transaction Pointer
 *  aSpaceID      - [IN] TableSpace ID
 *  aSegHandle    - [IN] Segment Handle
 *  aPagePtr      - [IN] insert, update, delete가 발생한 Data Page Ptr
 *
 ***********************************************************************/
IDE_RC sdpstAllocPage::updatePageState( idvSQL             * aStatistics,
                                        sdrMtx             * aMtx,
                                        scSpaceID            aSpaceID,
                                        sdpSegHandle       * aSegHandle,
                                        UChar              * aPagePtr )
{
    IDE_DASSERT( aSegHandle  != NULL );
    IDE_DASSERT( aPagePtr    != NULL );

    IDE_TEST( updatePageFN( aStatistics,
                            aMtx,
                            aSpaceID,
                            aSegHandle,
                            aPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 현재 bitmap 페이지의 MFNL을 반환한다.
 ***********************************************************************/
sdpstMFNL sdpstAllocPage::calcMFNL( UShort  * aMFNLtbl )
{
    sdpstMFNL sMFNL = SDPST_MFNL_FUL;
    SInt      sLoop;

    IDE_DASSERT( aMFNLtbl != NULL );

    for (sLoop = (SInt)SDPST_MFNL_UNF; sLoop >= (SInt)SDPST_MFNL_FUL; sLoop--)
    {
        if ( aMFNLtbl[sLoop] > 0 )
        {
            sMFNL = (sdpstMFNL)sLoop;
            break;
        }
    }
    return sMFNL;
}

/***********************************************************************
 * Description : data page부터 역으로 순서에 맞는 stack을 만들어낸다
 ***********************************************************************/
IDE_RC sdpstAllocPage::makeOrderedStackFromDataPage( idvSQL       * aStatistics,
                                                     scSpaceID      aSpaceID,
                                                     scPageID       aSegPID,
                                                     scPageID       aDataPageID,
                                                     sdpstStack   * aOrderedStack )
{
    UChar             * sPagePtr;
    SShort              sRtBMPIdx;
    sdpstStack          sRevStack;
    sdpParentInfo       sParentInfo;
    sdpstBMPHdr       * sBMPHdr;
    sdpstPosItem        sCurPos = { 0, 0, {0, 0} };
    scPageID            sCurRtBMP;
    UInt                sLoop;
    scPageID            sCurPID;
    UInt                sState = 0 ;

    IDE_DASSERT( aSegPID != SD_NULL_PID );
    IDE_DASSERT( aDataPageID != SD_NULL_PID );
    IDE_DASSERT( aOrderedStack != NULL );
    IDE_DASSERT( sdpstStackMgr::getDepth( aOrderedStack )
                == SDPST_EMPTY );

    sdpstStackMgr::initialize( &sRevStack );

    sCurPID = aDataPageID;

    /* sLoop는 상위 BMP 를 의미한다.
     * sLoop는  SDPST_LFBMP -> SDPST_RTBMP 순으로 변경되는 동안 fix하는 페이지는
     * Data page -> It-BMP 순으로 수행된다.  */
    for ( sLoop = (UInt)SDPST_LFBMP; sLoop >= SDPST_RTBMP; sLoop-- )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                              aSpaceID,
                                              sCurPID,
                                              &sPagePtr ) != IDE_SUCCESS );
        sState = 1;

        if ( sLoop == (UInt)SDPST_LFBMP )
        {
            sParentInfo = sdpPhyPage::getParentInfo( sPagePtr );

            sCurPos.mNodePID = sParentInfo.mParentPID;
            sCurPos.mIndex   = sParentInfo.mIdxInParent;
        }
        else
        {
            sBMPHdr = sLoop == (UInt)SDPST_ITBMP ?
                      sdpstLfBMP::getBMPHdrPtr( sPagePtr ) :
                      sdpstBMP::getHdrPtr( sPagePtr );

            sCurPos.mNodePID = sBMPHdr->mParentInfo.mParentPID;
            sCurPos.mIndex   = sBMPHdr->mParentInfo.mIdxInParent;
        }

        sdpstStackMgr::push( &sRevStack, &sCurPos );

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                  != IDE_SUCCESS );

        sCurPID = sCurPos.mNodePID;
    }

    /*
     * RtBMP의 순번을 찾는다.
     * 여기까지 오게 되면, sCurPID에는 RtBMP의 PID가 담겨있게 된다.
     * Segment Header부터 시작하여 sCurPID 인 rt-BMP가 몇번째에 위치해 있는지
     * 찾는다.
     */

    sRtBMPIdx  = SDPST_INVALID_SLOTNO;
    sCurRtBMP  = aSegPID;
    IDE_ASSERT( sCurRtBMP != SD_NULL_PID );

    while( sCurRtBMP != SD_NULL_PID )
    {
        sRtBMPIdx++;
        if ( sCurPID == sCurRtBMP )
        {
            break; // found it
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                              aSpaceID,
                                              sCurRtBMP,
                                              &sPagePtr ) != IDE_SUCCESS );
        sState = 1;

        sBMPHdr   = sdpstBMP::getHdrPtr( sPagePtr );
        sCurRtBMP = sdpstRtBMP::getNxtRtBMP( sBMPHdr );

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                  != IDE_SUCCESS );
    }

    sdpstStackMgr::push( aOrderedStack, SD_NULL_PID, sRtBMPIdx );

    while( sdpstStackMgr::getDepth( &sRevStack ) != SDPST_EMPTY )
    {
        sCurPos = sdpstStackMgr::pop( &sRevStack );
        sdpstStackMgr::push( aOrderedStack, &sCurPos );
    }

    if ( sdpstStackMgr::getDepth( aOrderedStack ) != SDPST_LFBMP )
    {
        ideLog::log( IDE_SERVER_0, "Ordered Stack depth is invalid\n" );
        ideLog::log( IDE_SERVER_0, "========= Ordered Stack Dump ========\n" );
        sdpstStackMgr::dump( aOrderedStack );
        IDE_ASSERT( 0 );
    }


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 주어진 Extent에 속한 Data Page를 format 한다.
 *
 * aStatistics      - [IN] 통계정보
 * aStartInfo       - [IN] sdrStartInfo
 * aSpaceID         - [IN] Tablespace ID
 * aSegHandle       - [IN] Segment Handle
 * aExtDesc         - [IN] Extent Desc
 * aBeginPID        - [IN] format을 시작할 PID
 * aBeginSeqNo      - [IN] format될 첫번째 Page의 SeqNo
 * aBeginLfBMP      - [IN] format될 페이지를 관리하는 첫번째 LfBMP
 * aBeginPBSNo      - [IN] format될 페이지의 PBSNo
 * aLfBMPHdr        - [IN] format될 페이지를 관리하는 첫번째 LfBMP Header
 ***********************************************************************/
IDE_RC sdpstAllocPage::formatDataPagesInExt(
                                        idvSQL              * aStatistics,
                                        sdrMtxStartInfo     * aStartInfo,
                                        scSpaceID             aSpaceID,
                                        sdpSegHandle        * aSegHandle,
                                        sdpstExtDesc        * aExtDesc,
                                        sdRID                 aExtRID,
                                        scPageID              aBeginPID )
{
    sdrMtx              sMtx;
    UChar             * sPagePtr;
    scPageID            sCurPID;
    sdpstPBS            sPBS;
    SShort              sPBSNo;
    scPageID            sLfBMP;
    scPageID            sPrvLfBMP;
    ULong               sSeqNo;
    sdpParentInfo       sParentInfo;
    sdpstRangeMap     * sRangeMap = NULL;
    UChar             * sLfBMPPagePtr = NULL;
    sdpstLfBMPHdr     * sLfBMPHdr = NULL;
    UInt                sState = 0;
    UInt                sLfBMPState = 0;
    sdpstPageRange      sPageRange;

    IDE_ASSERT( aStartInfo  != NULL );
    IDE_ASSERT( aSegHandle  != NULL );
    IDE_ASSERT( aExtDesc    != NULL );
    IDE_ASSERT( aExtRID     != SD_NULL_RID );
    IDE_ASSERT( aBeginPID   != SD_NULL_PID );

    sPBS = SDPST_BITSET_PAGETP_DATA | SDPST_BITSET_PAGEFN_FMT;

    /* BeginPID가 주어진 Extent에 속하는지 확인한다. */
    if ( (aExtDesc->mExtFstPID > aBeginPID) &&
         (aExtDesc->mExtFstPID + aExtDesc->mLength - 1 < aBeginPID) )
    {
        IDE_CONT( finish_reformat_data_pages );
    }

    IDE_TEST( sdpstExtDir::makeSeqNo( aStatistics,
                                      aSpaceID,
                                      aSegHandle->mSegPID,
                                      aExtRID,
                                      aBeginPID,
                                      &sSeqNo ) != IDE_SUCCESS );

    IDE_TEST( sdpstExtDir::calcLfBMPInfo( aStatistics,
                                          aSpaceID,
                                          aExtDesc,
                                          aBeginPID,
                                          &sParentInfo ) != IDE_SUCCESS );

    sPrvLfBMP = SD_NULL_PID;
    sLfBMP    = sParentInfo.mParentPID;
    sPBSNo    = sParentInfo.mIdxInParent;

    /* format 시작 */
    for ( sCurPID = aBeginPID;
          sCurPID < aExtDesc->mExtFstPID + aExtDesc->mLength;
          sCurPID++ )
    {
        if ( sPrvLfBMP != sLfBMP )
        {
            if ( sLfBMPState == 1 )
            {
                sLfBMPState = 0;
                IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                     sLfBMPPagePtr )
                          != IDE_SUCCESS );
            }

            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                  aSpaceID,
                                                  sLfBMP,
                                                  SDB_S_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  NULL, /* sdrMtx */
                                                  &sLfBMPPagePtr,
                                                  NULL, /* aTrySuccess */
                                                  NULL  /* aIsCorruptPage */ )
                      != IDE_SUCCESS );
            sLfBMPState = 1;

            sPrvLfBMP = sLfBMP;

            sLfBMPHdr = sdpstLfBMP::getHdrPtr( sLfBMPPagePtr );
            sRangeMap = sdpstLfBMP::getMapPtr( sLfBMPHdr );
        }
        else
        {
            /* 이전에 돌았던 루프로 인해, 이 Bmp를 가져온 상태 */
            IDE_ERROR( sLfBMPHdr != NULL );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aStartInfo,
                                       ID_TRUE,/*Undoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT) != IDE_SUCCESS );
        sState = 1;

        /* Meta 페이지는 format 하면 안된다. */
        if ( (sRangeMap->mPBSTbl[sPBSNo] & SDPST_BITSET_PAGETP_MASK)
              == SDPST_BITSET_PAGETP_DATA )
        {
            IDE_TEST( sdpstAllocPage::createPage( aStatistics,
                                                  &sMtx,
                                                  aSpaceID,
                                                  aSegHandle,
                                                  sCurPID,
                                                  sSeqNo,
                                                  SDP_PAGE_FORMAT,
                                                  sLfBMP,
                                                  sPBSNo,
                                                  sPBS,
                                                  &sPagePtr ) != IDE_SUCCESS );
        }

        sPageRange = sLfBMPHdr->mPageRange;

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        sPBSNo++;
        sSeqNo++;

        /* 한 Extent는 여러 LfBMP에 의해 관리될 수 있다.
         * Extent에 속한 페이지들을 format하기 전에 ParentInfo를 계산해낸다. */
        if ( sPBSNo == sPageRange )
        {
            sLfBMP++;
            sPBSNo = 0;
        }

        if ( sPBSNo > sPageRange )
        {
            ideLog::log( IDE_SERVER_0,
                         "aExtRID: %llu, "
                         "aBeginPID: %u\n",
                         aExtRID,
                         aBeginPID );

            ideLog::log( IDE_SERVER_0,
                         "sPrvLfBMP: %u\n"
                         "sLfBMP: %u\n"
                         "sPBSNo: %d\n"
                         "sSeqNo: %llu\n"
                         "sPageRange: %u\n",
                         sPrvLfBMP,
                         sLfBMP,
                         sPBSNo,
                         sSeqNo,
                         sPageRange );

            if( sLfBMPPagePtr != NULL )
            {
                sdpstLfBMP::dump( sLfBMPPagePtr );
            }
            else
            {
                /* nothing to do ... */
            }

            IDE_ASSERT( 0 );
        }
    }

    if ( sLfBMPState == 1 )
    {
        sLfBMPState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sLfBMPPagePtr )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( finish_reformat_data_pages );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLfBMPState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics, sLfBMPPagePtr )
                    == IDE_SUCCESS );
    }

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}
