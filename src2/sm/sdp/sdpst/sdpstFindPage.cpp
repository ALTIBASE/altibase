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
 * $Id: sdpstFindPage.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 가용공간 탐색 연산 관련 STATIC
 * 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <smErrorCode.h>

# include <sdpReq.h>
# include <sdpPhyPage.h>

# include <sdpstDef.h>
# include <sdpstCache.h>

# include <sdpstFindPage.h>
# include <sdpstRtBMP.h>
# include <sdpstLfBMP.h>
# include <sdpstSegDDL.h>
# include <sdpstAllocPage.h>

# include <sdpstItBMP.h>
# include <sdpstStackMgr.h>

/***********************************************************************
 * Description : [ INTERFACE ] 가용공간 할당 가능한 Table Data 페이지를
 *               탐색한다.
 ***********************************************************************/
IDE_RC sdpstFindPage::findInsertablePage( idvSQL           * aStatistics,
                                          sdrMtx           * aMtx,
                                          scSpaceID          aSpaceID,
                                          sdpSegHandle     * aSegHandle,
                                          void             * aTableInfo,
                                          sdpPageType        aPageType,
                                          UInt               aRowSize,
                                          idBool          /* aNeedKeySlot*/,
                                          UChar           ** aPagePtr,
                                          UChar            * aCTSlotNo )
{
    sdpstSegCache     * sSegCache;
    smOID               sTableOID;
    scPageID            sHintDataPID = SD_NULL_PID;
    scPageID            sNewHintDataPID = SD_NULL_PID;
    UChar             * sPagePtr   = NULL;
    UChar               sCTSlotNo = SDP_CTS_IDX_NULL;

    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aPagePtr   != NULL );
    IDE_ASSERT( aCTSlotNo != NULL );

    sSegCache = (sdpstSegCache*)((sdpSegHandle*)aSegHandle)->mCache;
    sTableOID = sdpstCache::getTableOID( sSegCache );

    if ( smuProperty::getTmsIgnoreHintPID() == 0 )
    {
        /* Hint DataPID를 먼저 Trans에서 가져온다.
         * 만약 Trans에 없는 경우 SegCache에서 가져온다. */
        if ( (sTableOID != SM_NULL_OID) && (aTableInfo != NULL) )
        {
            smLayerCallback::getHintDataPIDofTableInfo( aTableInfo, &sHintDataPID );
        }

        if ( sHintDataPID == SD_NULL_PID )
        {
            sdpstCache::getHintDataPage( aStatistics,
                                         sSegCache,
                                         &sHintDataPID );
        }

        IDE_TEST( checkAndSearchHintDataPID( aStatistics,
                                             aMtx,
                                             aSpaceID,
                                             aSegHandle,
                                             aTableInfo,
                                             sHintDataPID,
                                             aRowSize,
                                             &sPagePtr,
                                             &sCTSlotNo,
                                             &sNewHintDataPID ) != IDE_SUCCESS );
    }

    if ( sPagePtr == NULL )
    {
        IDE_TEST( searchFreeSpace( aStatistics,
                                   aMtx,
                                   aSpaceID,
                                   aSegHandle,
                                   aRowSize,
                                   aPageType,
                                   SDPST_SEARCH_NEWSLOT,
                                   &sPagePtr,
                                   &sCTSlotNo,
                                   &sNewHintDataPID ) != IDE_SUCCESS );

        IDE_ASSERT( sPagePtr != NULL );
    }

    if ( sNewHintDataPID != sHintDataPID )
    {
        if ( (sTableOID != SM_NULL_OID) && (aTableInfo != NULL) )
        {
            smLayerCallback::setHintDataPIDofTableInfo( aTableInfo, sNewHintDataPID );
        }

        sdpstCache::setHintDataPage( aStatistics,
                                     sSegCache,
                                     sNewHintDataPID );
    }

    *aPagePtr   = sPagePtr;
    *aCTSlotNo = sCTSlotNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( (sTableOID != SM_NULL_OID) && (aTableInfo != NULL) )
    {
        if ( sPagePtr == NULL )
        {
            // Hint Data 페이지에 대해서 가용공간할당이 불가능하여
            // Hint Data 페이지를 초기화한다.
            smLayerCallback::setHintDataPIDofTableInfo( aTableInfo, SD_NULL_PID );
        }
    }

    *aPagePtr   = NULL;
    *aCTSlotNo = SDP_CTS_IDX_NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Search 유형에 따라 가용공간을 탐색한다.
 ***********************************************************************/
IDE_RC sdpstFindPage::searchFreeSpace( idvSQL            * aStatistics,
                                       sdrMtx            * aMtx,
                                       scSpaceID           aSpaceID,
                                       sdpSegHandle      * aSegHandle,
                                       UInt                aRowSize,
                                       sdpPageType         aPageType,
                                       sdpstSearchType     aSearchType,
                                       UChar            ** aPagePtr,
                                       UChar             * aCTSlotNo,
                                       scPageID          * aNewHintDataPID )
{
    UChar             * sPagePtr  = NULL;
    UInt                sLoop;
    sdpstSegCache     * sSegCache;
    sdpstPosItem        sArrLfBMP[SDPST_MAX_CANDIDATE_LFBMP_CNT];
    UInt                sCandidateLfCount = 0;
    sdpstStack          sStack;
    sdpstPosItem        sCurrPos;
    idBool              sGoNxtIt        = ID_FALSE;
    scPageID            sNewHintDataPID = SD_NULL_PID;
    UChar               sCTSlotNo       = SDP_CTS_IDX_NULL;
    UChar             * sSegHdrPagePtr;
    sdpstSegHdr       * sSegHdr;
    sdrMtx              sMtx;
    UInt                sState = 0;
    sdrMtxStartInfo     sStartInfo;
    UInt                sSearchItBMPCnt = 0;
    idBool              sNeedToExtendExt = ID_FALSE;

    IDE_ASSERT( aSegHandle      != NULL );
    IDE_ASSERT( aMtx            != NULL );
    IDE_ASSERT( aPagePtr        != NULL );
    IDE_ASSERT( aNewHintDataPID != NULL );

    if ( aPageType != SDP_PAGE_DATA )
    {
        IDE_ASSERT( aSearchType == SDPST_SEARCH_NEWPAGE );
    }

    sSegCache  = (sdpstSegCache*)(aSegHandle->mCache);
    sdpstCache::copyItHint( aStatistics, sSegCache, aSearchType, &sStack );

    sSearchItBMPCnt    = 0;

    while ( 1 )
    {
        if ( sGoNxtIt == ID_TRUE )
        {
            /* BUG-29038
             * 탐색할 ItBMP 개수를 초과하거나 확장이 필요할때 확장하여
             * 확장한 Extent의 첫번째 페이지를 바로 할당한다. */
            if ( sSearchItBMPCnt <= SDPST_SEARCH_QUOTA_IN_RTBMP )
            {
                IDE_TEST( searchSpaceInRtBMP( aStatistics,
                                              aMtx,
                                              aSpaceID,
                                              aSegHandle,
                                              &sStack,
                                              aSearchType,
                                              &sNeedToExtendExt )
                          != IDE_SUCCESS );
            }
            else
            {
                sNeedToExtendExt = ID_TRUE;
            }

            if ( sNeedToExtendExt == ID_TRUE )
            {
                IDE_TEST( tryToAllocExtsAndPage( aStatistics,
                                                 aMtx,
                                                 aSpaceID,
                                                 aSegHandle,
                                                 aSearchType,
                                                 aPageType,
                                                 &sStack,
                                                 &sPagePtr ) != IDE_SUCCESS );

                IDU_FIT_POINT( "1.BUG-30667@sdpstFindPage::searchFreeSpace" );

                /* Extent 확장을 여러 트랜잭션이 동시에 수행하게 되면
                 * 한 트랜잭션만 확장을 진행하고 다른 트랜잭션은 대기하다가
                 * 확장이 완료되면 아무것도 안하고, 탐색할 ItHint만 sStack에 
                 * 설정하고 나오게 된다.
                 * 따라서 이 경우 sPagePtr이 NULL로 리턴되는데, 이때 해당
                 * ItHint에서 탐색을 더 봐야한다. */
                if ( sPagePtr != NULL )
                {
                    break;
                }
            }
        }

        sCurrPos = sdpstStackMgr::getCurrPos( &sStack );

        if ( sdpstStackMgr::getDepth( &sStack ) != SDPST_ITBMP )
        {
            ideLog::log( IDE_SERVER_0, "Search Stack is invalid\n" );

            ideLog::log( IDE_SERVER_0, "======== Search Stack Dump =======\n" );
            sdpstStackMgr::dump( &sStack );

            ideLog::log( IDE_SERVER_0, "======== SegCache Dump =======\n" );
            sdpstCache::dump( sSegCache );

            IDE_ASSERT( 0 );
        }

        /* 탐색할 ItBMP 개수를 제한한다. */
        sSearchItBMPCnt++;

        IDE_TEST( searchSpaceInItBMP(
                               aStatistics,
                               aMtx,
                               aSpaceID,
                               sCurrPos.mNodePID,
                               aSearchType,
                               sSegCache,
                               &sGoNxtIt,
                               sArrLfBMP,
                               &sCandidateLfCount ) != IDE_SUCCESS );

        if ( sGoNxtIt == ID_TRUE )
        {
            // itbmp 페이지가 가용하지 않다면, 다시 Root bmp 페이지에서의
            // 탐색과정으로 돌아간다.
            sdpstStackMgr::pop( &sStack );
            continue;
        }
        else
        {
            IDE_ASSERT( sCandidateLfCount > 0 );
        }

        if ( sdpstStackMgr::getDepth( &sStack ) != SDPST_ITBMP )
        {
            ideLog::log( IDE_SERVER_0, "Search Stack is invalid\n" );

            ideLog::log( IDE_SERVER_0, "======== Search Stack Dump =======\n" );
            sdpstStackMgr::dump( &sStack );

            ideLog::log( IDE_SERVER_0, "======== SegCache Dump =======\n" );
            sdpstCache::dump( sSegCache );

            IDE_ASSERT( 0 );
        }

        for ( sLoop = 0; sLoop < sCandidateLfCount; sLoop++ )
        {
            /* 후보 Data 페이지는 Stack에 저장할 경우는 HWM
             * 저장하기 위해서만 저장하며, traverse를 위해서는 저장
             * 하지 않는다. 그러므로, 여기까지는 Stack에는 Lf BMP
             * 까지 정보만 저장되어 있으며, Lf BMP 페이지에서 Data
             * 페이지의 Create 연산이 발생할 경우에만, Data 페이지의
             * 정보가 Stack에 저장된다. */

            /* internal depth의 하위 Slot Index를 재설정한다. */
            sdpstStackMgr::setCurrPos( &sStack,
                                       sCurrPos.mNodePID,
                                       sArrLfBMP[sLoop].mIndex );

            IDE_TEST( searchSpaceInLfBMP( aStatistics,
                                          aMtx,
                                          aSpaceID,
                                          aSegHandle,
                                          sArrLfBMP[ sLoop ].mNodePID,
                                          &sStack,
                                          aPageType,
                                          aSearchType,
                                          aRowSize,
                                          &sPagePtr,
                                          &sCTSlotNo ) != IDE_SUCCESS );

            IDU_FIT_POINT( "1.BUG-30667@sdpstFindPage::searchFreeSpace" );

            if ( sPagePtr != NULL )
            {
                sNewHintDataPID = sdpPhyPage::getPageID( sPagePtr );
                break;
            }
        }

        if ( sPagePtr == NULL ) // Not Found !!
        {
            /* 가용한 Data 페이지를 탐색하지 못했다면,
             * ItBMP 페이지의 상위 RtBMP 페이지부터 다시 탐색을 수행한다. */
            sdpstStackMgr::pop( &sStack );
            sGoNxtIt = ID_TRUE;
        }
        else
        {
            break;
        }
    }

    *aNewHintDataPID = sNewHintDataPID;
    *aPagePtr        = sPagePtr;
    *aCTSlotNo       = sCTSlotNo;

    /* lstAllocPage를 갱신한다. */
    sdpstCache::setLstAllocPage4AllocPage( aStatistics,
                                 aSegHandle,
                                 sdpPhyPage::getPageID( sPagePtr ),
                                 sdpPhyPage::getSeqNo( 
                                        sdpPhyPage::getHdr(sPagePtr) ) );

    /* Index Page인 경우 free index Page 개수를 갱신한다 */
    if ( aPageType != SDP_PAGE_DATA )
    {
        sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       &sStartInfo,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS ); 
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              aSegHandle->mSegPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              &sMtx,
                                              &sSegHdrPagePtr,
                                              NULL, /* aTrySuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );

        sSegHdr = sdpstSH::getHdrPtr( sSegHdrPagePtr );

        IDE_TEST( sdpstSH::setFreeIndexPageCnt( &sMtx,
                                                sSegHdr,
                                                sSegHdr->mFreeIndexPageCnt - 1 )
                  != IDE_SUCCESS );


        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    *aNewHintDataPID = SD_NULL_PID;
    *aPagePtr        = NULL;
    *aCTSlotNo       = SDP_CTS_IDX_NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 확장하고, 확장했다면 확장한 Extent의 첫번째 data페이지에
 * X-Latch를 잡아 리턴한다.
 *
 * 다른 Trans에서 이미 확장중인경우 확장하지 않는 경우도 있다.
 * 이때 확장한 마지막 ItBMP가 aStack에 설정된다.
 * 즉, 계속 탐색을 수행하면 된다.
 ***********************************************************************/
IDE_RC sdpstFindPage::tryToAllocExtsAndPage( idvSQL          * aStatistics,
                                             sdrMtx          * aMtx4Latch,
                                             scSpaceID         aSpaceID,
                                             sdpSegHandle    * aSegHandle,
                                             sdpstSearchType   aSearchType,
                                             sdpPageType       aPageType,
                                             sdpstStack      * aStack,
                                             UChar          ** aFstDataPagePtr )
{
    sdrMtxStartInfo     sStartInfo;
    sdpstPosItem        sPosItem;
    sdpstWM             sHWM;
    sdpstSegCache     * sSegCache;
    sdrMtx              sInitMtx;
    UChar             * sPagePtr = NULL;
    sdpstPBS            sPBS;
    sdpstStack          sRevStack;
    sdpParentInfo       sParentInfo;
    sdRID               sAllocFstExtRID;
    sdpstStack          sPrvItHint;
    UInt                sState = 0;

    IDE_ASSERT( aMtx4Latch      != NULL );
    IDE_ASSERT( aSegHandle      != NULL );
    IDE_ASSERT( aFstDataPagePtr != NULL );

    sSegCache = (sdpstSegCache*)aSegHandle->mCache;

    // 현재 Root bmp 페이지에 가용한 itbmp 페이지가 존재하지
    // 않기 때문에 Segment 확장을 수행한다.
    sdrMiniTrans::makeStartInfo( aMtx4Latch, &sStartInfo );

    IDE_TEST( sdpstSegDDL::allocNewExtsAndPage(
                                   aStatistics,
                                   &sStartInfo,
                                   aSpaceID,
                                   aSegHandle,
                                   aSegHandle->mSegStoAttr.mNextExtCnt,
                                   ID_TRUE, /* aNeedToUpdateHWM */
                                   &sAllocFstExtRID,
                                   aMtx4Latch,
                                   &sPagePtr )
              != IDE_SUCCESS );

    /* Extent를 직접 확장한 경우 First Data Page의 포인터가 리턴된다.
     * 해당 페이지를 초기화한다. */
    if ( sPagePtr != NULL )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sInitMtx,
                                       &sStartInfo,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdrMiniTrans::setDirtyPage( &sInitMtx, sPagePtr )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdpPhyPage::setPageType( sdpPhyPage::getHdr( sPagePtr ),
                                           aPageType,
                                           &sInitMtx ) != IDE_SUCCESS );

        sdbBufferMgr::setPageTypeToBCB( (UChar*)sPagePtr,
                                        (UInt)aPageType );
        
        if ( aPageType == SDP_PAGE_DATA )
        {
            IDE_TEST( smLayerCallback::logAndInitCTL( &sInitMtx,
                                                      sdpPhyPage::getHdr( sPagePtr ),
                                                      aSegHandle->mSegAttr.mInitTrans,
                                                      aSegHandle->mSegAttr.mMaxTrans )
                      != IDE_SUCCESS );

            IDE_TEST( sdpSlotDirectory::logAndInit(
                                        sdpPhyPage::getHdr(sPagePtr),
                                        &sInitMtx )
                    != IDE_SUCCESS );

        }
        else
        {
            sParentInfo = sdpPhyPage::getParentInfo( sPagePtr );

            sPBS = (sdpstPBS)
                   (SDPST_BITSET_PAGETP_DATA|SDPST_BITSET_PAGEFN_FUL);
            IDE_TEST( sdpPhyPage::setState( sdpPhyPage::getHdr(sPagePtr),
                                            sPBS,
                                            &sInitMtx )
                      != IDE_SUCCESS );

            // Table Page가 아닌경우에는 미리 bitmap을 갱신한다.
            sdpstStackMgr::initialize(&sRevStack);

            // leaf bmp 페이지부터 MFNL을 변경시도하여
            // 상위노드로 전파한다.
            IDE_TEST( sdpstAllocPage::tryToChangeMFNLAndItHint(
                          aStatistics,
                          &sInitMtx,
                          aSpaceID,
                          aSegHandle,
                          SDPST_CHANGEMFNL_LFBMP_PHASE,
                          sParentInfo.mParentPID,  // leaf pid
                          sParentInfo.mIdxInParent,
                          (void*)&sPBS,
                          SD_NULL_PID,            // unuse
                          SDPST_INVALID_SLOTNO,  // unuse
                          &sRevStack ) != IDE_SUCCESS );
        }
        
        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sInitMtx ) != IDE_SUCCESS );

        *aFstDataPagePtr = sPagePtr;
    }

    /* 확장된 이후에는 마지막 ItBMP를 Hint로 설정하고,
     * 해당 ItBMP에서 탐색을 시작한다.
     * 마지막 ItBMP는 HWM에 설정되어 있다. */
    sdpstCache::copyHWM( aStatistics, sSegCache, &sHWM );

    /* 현재 stack을 초기화하고, 마지막 ItBMP를 탐색 위치로
     * 설정한다. */
    sdpstStackMgr::initialize( aStack );

    if ( sdpstStackMgr::getDepth( aStack ) != SDPST_EMPTY )
    {
        ideLog::log( IDE_SERVER_0, "Search Stack is invalid\n" );

        ideLog::log( IDE_SERVER_0, "======== Search Stack Dump =======\n" );
        sdpstStackMgr::dump( aStack );

        ideLog::log( IDE_SERVER_0, "======== SegCache Dump =======\n" );
        sdpstCache::dump( sSegCache );

        IDE_ASSERT( 0 );
    }


    sPosItem = sdpstStackMgr::getSeekPos( &sHWM.mStack, SDPST_VIRTBMP);
    sdpstStackMgr::push( aStack, &sPosItem );

    sPosItem = sdpstStackMgr::getSeekPos( &sHWM.mStack, SDPST_RTBMP);
    sdpstStackMgr::push( aStack, &sPosItem );

    sPosItem = sdpstStackMgr::getSeekPos( &sHWM.mStack, SDPST_ITBMP );
    sPosItem.mIndex = 0;
    sdpstStackMgr::push( aStack, &sPosItem );

    if ( sdpstStackMgr::getDepth( aStack ) != SDPST_ITBMP )
    {
        ideLog::log( IDE_SERVER_0, "Search Stack is invalid\n" );

        ideLog::log( IDE_SERVER_0, "======== Search Stack Dump =======\n" );
        sdpstStackMgr::dump( aStack );

        ideLog::log( IDE_SERVER_0, "======== SegCache Dump =======\n" );
        sdpstCache::dump( sSegCache );

        IDE_ASSERT( 0 );
    }

    /* 이전 ItHint를 가져오고, */
    sdpstCache::copyItHint( aStatistics,
                            sSegCache,
                            aSearchType,
                            &sPrvItHint );

    /* 설정한 ItBMP를 Hint로 설정한다. */
    sdpstCache::setItHintIfGT( aStatistics,
                               sSegCache,
                               aSearchType,
                               aStack );

    /* 현재 ItHint와 비교하여 Hint가 앞당겨졌다면
     * rescan flag를 켜준다. */
    if ( sdpstStackMgr::compareStackPos( &sPrvItHint, aStack ) < 0 )
    {
        if ( aSearchType == SDPST_SEARCH_NEWSLOT )
        {
            sdpstCache::setUpdateHint4Slot( sSegCache, ID_TRUE );
        }
        else
        {
            IDE_ASSERT( aSearchType == SDPST_SEARCH_NEWPAGE );
            sdpstCache::setUpdateHint4Page( sSegCache, ID_TRUE );
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sInitMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Root BMP 페이지에서의 가용공간 탐색을 수행한다.
 *
 * Root BMP 페이지가 가용공간을 가진 Internal BMP 페이지를 가지고 있지
 * 않다면, It Hint를 변경해주어야 한다.
 ***********************************************************************/
IDE_RC sdpstFindPage::searchSpaceInRtBMP( idvSQL            * aStatistics,
                                          sdrMtx            * aMtx,
                                          scSpaceID           aSpaceID,
                                          sdpSegHandle      * aSegHandle,
                                          sdpstStack        * aStack,
                                          sdpstSearchType     aSearchType,
                                          idBool            * aNeedToExtendExt )
{
    sdrSavePoint        sInitSP;
    scPageID            sNxtItBMP;
    scPageID            sNxtRtBMP;
    SShort              sSlotNoInParent;
    sdpstBMPHdr       * sBMPHdr;
    UChar             * sPagePtr;
    sdpstPosItem        sPosItem;
    scPageID            sCurRtBMP;
    SShort              sNewSlotNoInParent;
    sdpstSegCache     * sSegCache;
    sdpstWM             sHWM;
    UInt                sState = 0;

    IDE_DASSERT( aMtx             != NULL );
    IDE_DASSERT( aSegHandle       != NULL );
    IDE_DASSERT( aStack           != NULL );
    IDE_DASSERT( aNeedToExtendExt != NULL );

    sSegCache = (sdpstSegCache*)aSegHandle->mCache;
    sNxtRtBMP = SD_NULL_PID;
    *aNeedToExtendExt = ID_FALSE;

    while( 1 )
    {
        if ( sdpstStackMgr::getDepth( aStack ) != SDPST_RTBMP )
        {
            ideLog::log( IDE_SERVER_0, "Search Stack is invalid\n" );

            ideLog::log( IDE_SERVER_0, "======== Search Stack Dump =======\n" );
            sdpstStackMgr::dump( aStack );

            ideLog::log( IDE_SERVER_0, "======== SegCache Dump =======\n" );
            sdpstCache::dump( sSegCache );

            IDE_ASSERT( 0 );
        }

        sdpstCache::copyHWM( aStatistics, sSegCache, &sHWM );

        sPosItem        = sdpstStackMgr::getCurrPos( aStack );
        sCurRtBMP       = sPosItem.mNodePID;
        sSlotNoInParent = sPosItem.mIndex;

        IDE_ASSERT( sCurRtBMP != SD_NULL_PID );

        sdrMiniTrans::setSavePoint( aMtx, &sInitSP );

        sState = 0;
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sCurRtBMP,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sPagePtr,
                                              NULL, /* aTrySuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );
        sState = 1;

        sBMPHdr = sdpstBMP::getHdrPtr( sPagePtr );

        /* RtBMP에서 다음에 탐색할 ItBMP를 구한다.
         * 세그먼트 공간 확장 이전에는 현 ItBMP 이후에 가용공간이 있는
         * ItBMP를 찾는다. 확장 후에는 확장된 Extent가 기존 ItBMP에 존재할
         * 수 있기 때문에 현 ItBMP부터 탐색을 시작한다. */
        sdpstRtBMP::findFreeItBMP( sBMPHdr,
                                   sSlotNoInParent + 1,
                                   SDPST_SEARCH_TARGET_MIN_MFNL(aSearchType),
                                   &sHWM,
                                   &sNxtItBMP,
                                   &sNewSlotNoInParent );

        if ( sNxtItBMP != SD_NULL_PID )
        {
            sState = 0;
            // 획득했던 root bmp 페이지를 unlatch 한다.
            IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                      != IDE_SUCCESS );

            // Internal Bitmap 페이지에서의 탐색과정으로 넘어간다.
            // Stack에 넣는다.
            sdpstStackMgr::setCurrPos( aStack,
                                       sCurRtBMP,
                                       sNewSlotNoInParent );

            sdpstStackMgr::push( aStack, sNxtItBMP, 0 );

            if ( sdpstStackMgr::getDepth( aStack ) != SDPST_ITBMP )
            {
                ideLog::log( IDE_SERVER_0, "Search Stack is invalid\n" );

                ideLog::log( IDE_SERVER_0, "======== Search Stack Dump =======\n" );
                sdpstStackMgr::dump( aStack );

                ideLog::log( IDE_SERVER_0, "======== SegCache Dump =======\n" );
                sdpstCache::dump( sSegCache );

                IDE_ASSERT( 0 );
            }

            break; // Found it !!
        }

        // 가용한 itbmp 페이지가 존재하지 않는다면 상위
        // rtbmp 페이지에서 다음 itbmp 페이지로 넘어간다.
        sNxtRtBMP          = sdpstRtBMP::getNxtRtBMP(sBMPHdr);
        sNewSlotNoInParent = 0; // Slot 순번을 첫번째로 설정한다.

        // 획득했던 root bmp 페이지를 unlatch 한다.
        sState = 0;
        IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                  != IDE_SUCCESS );

        if ( sNxtRtBMP == SD_NULL_PID )
        {
            /* 이전 ItBMP에 delete가 발생하여 해당 ItBMP의 MFNL이 갱신되는
             * 경우 UpdateItHint Flag가 True가 된다.
             * 더이상 가용 공간이 없는 경우 해당 플래그를 보고, 이전에 가용공간
             * 이 있는 경우 해당 ItBMP를 Hint로 설정하고 탐색을 시작한다. */
            if ( sdpstCache::needToUpdateItHint( sSegCache,
                                                 aSearchType ) == ID_TRUE )
            {
                IDE_TEST( sdpstRtBMP::rescanItHint( aStatistics,
                                                    aSpaceID,
                                                    aSegHandle->mSegPID,
                                                    aSearchType,
                                                    sSegCache,
                                                    aStack ) != IDE_SUCCESS );
                /* aStack에 ItBMP Hint가 설정되었다. ItBMP에서 탐색을 시작
                 * 한다. */
                if ( sdpstStackMgr::getDepth( aStack ) != SDPST_ITBMP )
                {
                    ideLog::log( IDE_SERVER_0, "Search Stack is invalid\n" );

                    ideLog::log( IDE_SERVER_0, "======== Search Stack Dump =======\n" );
                    sdpstStackMgr::dump( aStack );

                    ideLog::log( IDE_SERVER_0, "======== SegCache Dump =======\n" );
                    sdpstCache::dump( sSegCache );

                    IDE_ASSERT( 0 );
                }

                break;
            }
            else
            {
                /* 공간 확장을 수행한다. */
                *aNeedToExtendExt = ID_TRUE;
                break;
            }
        }
        else
        {
            // 현재 Root를 제거한다.
            sdpstStackMgr::pop( aStack );
            // 가상 Node를 제거한다.
            sPosItem = sdpstStackMgr::pop( aStack );

            // 가상 노드에서의 rtbmp 페이지 순번을 증가시킨다음,
            // 가상 Node를 다시 넣는다.
            sPosItem.mIndex++;
            sdpstStackMgr::push( aStack, &sPosItem );
            // 새로운 Root bmp 페이지를 Stack에 넣는다.
            sdpstStackMgr::push( aStack, sNxtRtBMP, sNewSlotNoInParent );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Internal Bitmap 페이지에서의 가용공간 탐색을 수행한다.
 ***********************************************************************/
IDE_RC sdpstFindPage::searchSpaceInItBMP(
                                 idvSQL          * aStatistics,
                                 sdrMtx          * aMtx,
                                 scSpaceID         aSpaceID,
                                 scPageID          aItBMP,
                                 sdpstSearchType   aSearchType,
                                 sdpstSegCache   * aSegCache,
                                 idBool          * aGoNxtIt,
                                 sdpstPosItem    * aArrLeafBMP,
                                 UInt            * aCandidateCount )
{
    UChar             * sItPagePtr;
    sdrSavePoint        sInitSP;
    sdpstWM             sHWM;
    UInt                sState = 0;

    IDE_DASSERT( aItBMP          != SD_NULL_PID );
    IDE_DASSERT( aMtx            != NULL );
    IDE_DASSERT( aSegCache       != NULL );
    IDE_DASSERT( aGoNxtIt        != NULL );
    IDE_DASSERT( aArrLeafBMP     != NULL );
    IDE_DASSERT( aCandidateCount != NULL );

    sdrMiniTrans::setSavePoint( aMtx, &sInitSP );

    sState = 0;
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aItBMP,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sItPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sState = 1;

    /* BUG-29325 TC/Server/sm4/Project3/PROJ-2037/conc_recv/sgl/upd_sqs.sql 에서
     *           FATAL 발생
     * 후보 선정 과정에서 HWM이 변경될 수 있기 때문에 해당 BMP 페이지에
     * S-Latch를 획득한 이후 HWM을 가져와야 한다. */
    sdpstCache::copyHWM( aStatistics, aSegCache, &sHWM );

    /*
     * internal bitmap 페이지에서 탐색할 leaf bmp 페이지들의 후보 목록을
     * 작성하고, Search Type에 따라 Available 하지 않다면 Cache Hint를
     * 변경한다.
     */
    sdpstBMP::makeCandidateChild( aSegCache,
                                  sItPagePtr,
                                  SDPST_ITBMP,
                                  aSearchType,
                                  &sHWM,
                                  aArrLeafBMP,
                                  aCandidateCount );

    sState = 0;
    IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "1.BUG-29325@sdpstFindPage::searchSpaceInItBMP" );

    if ( *aCandidateCount == 0 )
    {
        // itbmp 페이지가 가용한 후보 lfbmp 페이지들이 존재하지 않는다면
        // 상위 rtbmp 페이지에서 다음 itbmp 페이지로 넘어간다.
        // stack에 저장되어 있는 Root BMP 페이지의 PID와 It SlotNo를
        // 사용하여 진행하고, 현재 존재하는 internal bmp 페이지에 대한 정보는
        // stack에서 제거한다.
        *aGoNxtIt = ID_TRUE;
    }
    else
    {
        // 가용한 후보 leaf bmp 페이지들을 탐색했기 때문에
        // Stack에서 internal bmp에 대한 내용을 pop하지 않는다.
        *aGoNxtIt = ID_FALSE;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                    == IDE_SUCCESS );
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Leaf Bitmap 페이지에서의 가용공간 탐색을 수행한다.
 *
 * 후보 Data 페이지가 Unformat 상태이면, 페이지 할당을 수행한다.
 *
 * A. 후보 페이지가 INSERTABLE 페이지인경우
 *
 * leaf bmp 페이지를 S-latch로 획득한 상태에서 후보 Data 페이지 목록을
 * 작성하고 바로 해제한다. 왜냐하면, 할당이후 후보 페이지의 가용도를
 * 변경하기 위해서 leaf bmp 페이지에 대해서 X-latch를 획득하려고 해야
 * 하기 때문이다. 만약 S-latch를 해제하지 않으면, 다른 트랜잭션들도
 * 마찬가지로 해제하지 않기 때문에 페이지 가용도 변경을 하기 위해서
 * X-latch를 획득하지 못하는 교착상태에 빠질 수 있다.
 *
 * leaf bmp 페이지에 대한 unlatch이후에 후보 Data 페이지가 갑자기
 * format 페이지로 변경될 수 있다. 이를 고려해야한다.
 *
 * B. 후보 페이지가 UNFORMAT 페이지인경우
 * leaf bmp 페이지에 대한 S-latch를 해제하였기 때문에, 후보 unformat
 * 페이지에 대한 트랜잭션간의 경쟁이 발생할 수 있다. 그러므로 자신의
 * 후보 페이지가 unformat 페이지였다 할지라도 leaf bmp 페이지
 * 에 X-latch를 획득했을 때, PBS을 다시한번 확인해봐야 한다.
 *
 * 가용공간을 탐색하였고, MFNL을 변경해야한다면, 변경한다.
 ***********************************************************************/
IDE_RC sdpstFindPage::searchSpaceInLfBMP( idvSQL          * aStatistics,
                                          sdrMtx          * aMtx,
                                          scSpaceID         aSpaceID,
                                          sdpSegHandle    * aSegHandle,
                                          scPageID          aLeafBMP,
                                          sdpstStack      * aStack,
                                          sdpPageType       aPageType,
                                          sdpstSearchType   aSearchType,
                                          UInt              aRowSize,
                                          UChar          ** aPagePtr,
                                          UChar           * aCTSlotNo )
{
    sdrSavePoint         sInitSP;
    UChar              * sLfPagePtr;
    UChar              * sPagePtr   = NULL;
    UChar                sCTSlotNo = SDP_CTS_IDX_NULL;
    UInt                 sState = 0;
    sdpstWM              sHWM;
    sdpstCandidatePage   sArrData[SDPST_MAX_CANDIDATE_PAGE_CNT];
    UInt                 sCandidateDataCount = 0;

    IDE_ASSERT( aSegHandle      != NULL );
    IDE_ASSERT( aLeafBMP        != SD_NULL_PID );
    IDE_ASSERT( aMtx            != NULL );
    IDE_ASSERT( aPagePtr        != NULL );

    IDU_FIT_POINT( "1.BUG-29325@sdpstFindPage::searchSpaceInLfBMP" );

    sdrMiniTrans::setSavePoint( aMtx, &sInitSP );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aLeafBMP,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sLfPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );
    sState = 1;

    /* BUG-29325 TC/Server/sm4/Project3/PROJ-2037/conc_recv/sgl/upd_sqs.sql 에서
     *           FATAL 발생
     * 후보 선정 과정에서 HWM이 변경될 수 있기 때문에 해당 BMP 페이지에
     * S-Latch를 획득한 이후 HWM을 가져와야 한다. */
    sdpstCache::copyHWM( aStatistics,
                         (sdpstSegCache*)aSegHandle->mCache,
                         &sHWM );

    /* HWM 이후 탐색하려는 경우 */
    if ( sdpstStackMgr::compareStackPos( &sHWM.mStack, aStack ) < 0 )
    {
        ideLog::log( IDE_SERVER_0,
                "HWM PID: %u, "
                "ExtDir PID: %u, "
                "SlotNoInExtDir: %d\n"
                "LfBMP :%u\n",
                sHWM.mWMPID,
                sHWM.mExtDirPID,
                sHWM.mSlotNoInExtDir,
                aLeafBMP );

        sdpstStackMgr::dump( &sHWM.mStack );
        sdpstStackMgr::dump( aStack );
        sdpstCache::dump( (sdpstSegCache*)aSegHandle->mCache );
        sdpstLfBMP::dump( sLfPagePtr );

        IDE_ASSERT( 0 );
    }

    // A. 탐색할 Data 페이지 후보 목록을 작성한다.
    sdpstBMP::makeCandidateChild( (sdpstSegCache*)aSegHandle->mCache,
                                  sLfPagePtr,
                                  SDPST_LFBMP,
                                  aSearchType,
                                  &sHWM,
                                  (void*)sArrData,
                                  &sCandidateDataCount );

    // 후보 Data 페이지 목록을 작성하였다면, lfbmp 페이지에 대한 Latch를
    // 해제한다음, 후보 목록에 대해서 No-Wait 모드로 가용공간탐색을 수행한다.
    // itbmp 페이지에 대한 latch를 해제했기 때문에, 실제 후보 목록 작성당시의
    // 페이지 상태가 아닐수도 있다는 것을 고려해야한다.
    sState = 0;
    IDE_TEST( sdrMiniTrans::releaseLatchToSP(aMtx, &sInitSP) != IDE_SUCCESS );

    IDU_FIT_POINT( "1.BUG-33683@sdpstFindPage::searchSpaceInLfBMP" );

    if ( sCandidateDataCount > 0 )
    {
        // 후보 Data 페이지에 대해서 No-Wait 모드로 탐색을 수행한다.
        IDE_TEST( searchPagesInCandidatePage(
                                           aStatistics,
                                           aMtx,
                                           aSpaceID,
                                           aSegHandle,
                                           aLeafBMP,
                                           sArrData,
                                           aPageType,
                                           sCandidateDataCount,
                                           aSearchType,
                                           aRowSize,
                                           SDB_WAIT_NO,
                                           &sPagePtr,
                                           &sCTSlotNo ) != IDE_SUCCESS );

        /* BUG-33683 - [SM] in sdcRow::processOverflowData, Deadlock can occur
         *
         * sdcRow::processOverflowData에서 overflow 데이터를 처리하기 위해
         * 페이지를 할당하게 된다.
         * 이때, sdcRow에서 동시에 하나의 페이지에만 latch를 걸어야 한다는
         * 가정이 깨지기 때문에 데드락 발생 가능성이 있게 된다.
         * 따라서 이러한 경우를 해결하기 위해 TMS에서 No-wait 모드로 데이터 페이지를
         * 확인해본 후 wait 모드로 확인하고 있는데, 이를 no-wait 모드로만 확인하도록
         * 수정한다.
         *
         * 이렇게 수정함으로써 세그먼트의 공간확장이 조금 더 빈번해질 수 있다는 단점이
         * 있지만,
         * no-wait 모드로 getPage를 했을때 페이지를 획득하지 못한 경우는
         * 데이터 페이지 contention이 심한 경우라고 볼 수 있고,
         * 이런 경우 세그먼트 공간을 확장하여 contention을 줄이는 것이 성능에 도움이 된다.
         * 따라서 no-wait 모드로 변경하는 것이 더 합리적이다. */
        if ( sPagePtr == NULL )
        {
            // 후보 Data 페이지에 대해서 Wait 모드로 탐색한다.
            // 페이지할당 탐색연산은 Wait 모드로 탐색하지 않는다.
            IDE_TEST( searchPagesInCandidatePage(
                                             aStatistics,
                                             aMtx,
                                             aSpaceID,
                                             aSegHandle,
                                             aLeafBMP,
                                             sArrData,
                                             aPageType,
                                             sCandidateDataCount,
                                             aSearchType,
                                             aRowSize,
                                             SDB_WAIT_NO,
                                             &sPagePtr,
                                             &sCTSlotNo )
                    != IDE_SUCCESS );
        }
    }
    else
    {
        // 후보목록을 작성하지 못하였다면, 다음 후보 itbmp 페이지에 대한
        // 탐색과정으로 넘어간다.
    }

    *aPagePtr   = sPagePtr;
    *aCTSlotNo = sCTSlotNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 1:
            IDE_ASSERT( sdrMiniTrans::releaseLatchToSP(aMtx, &sInitSP)
                        == IDE_SUCCESS );
        default:
            break;
    }

    *aPagePtr   = NULL;
    *aCTSlotNo = SDP_CTS_IDX_NULL;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 후보 Data 페이지에 대해서 No-Wait 모드로 가용공간
 *               탐색을 수행한다.
 **********************************************************************/
IDE_RC sdpstFindPage::searchPagesInCandidatePage(
    idvSQL               * aStatistics,
    sdrMtx               * aMtx,
    scSpaceID              aSpaceID,
    sdpSegHandle         * aSegHandle,
    scPageID               aLeafBMP,
    sdpstCandidatePage   * aArrData,
    sdpPageType            aPageType,
    UInt                   aCandidateDataCount,
    sdpstSearchType        aSearchType,
    UInt                   aRowSize,
    sdbWaitMode            aWaitMode,
    UChar               ** aPagePtr,
    UChar                * aCTSlotNo )
{
    sdpPhyPageHdr     * sPhyPageHdr;
    sdpstPBS            sPBS;
    UInt                sLoop;
    sdrSavePoint        sInitSP;
    UChar             * sPagePtr = NULL;
    idBool              sTrySuccess;
    sdpParentInfo       sParentInfo;
    sdpstStack          sRevStack;
    sdrMtxStartInfo     sStartInfo;
    UInt                sState = 0;
    sdrMtx              sMtx;
    idBool              sCanAlloc;
    UChar               sCTSlotNo = SDP_CTS_IDX_NULL;
    sdpPageType         sOldPageType;
    ULong               sNTAData[2];
    smLSN               sNTA;

    IDE_DASSERT( aLeafBMP            != SD_NULL_PID );
    IDE_DASSERT( aArrData            != NULL );
    IDE_DASSERT( aMtx                != NULL );
    IDE_DASSERT( aPagePtr            != NULL );
    IDE_DASSERT( aCandidateDataCount > 0 );

    for ( sLoop = 0; sLoop < aCandidateDataCount; sLoop++ )
    {
        /* Unformat 상태는 없다. */
        if ( sdpstLfBMP::isEqFN( aArrData[sLoop].mPBS,
                                 SDPST_BITSET_PAGEFN_UNF ) == ID_TRUE )
        {
            ideLog::log( IDE_SERVER_0,
                         "sLoop: %u\n",
                         sLoop );

            ideLog::logMem(
                IDE_SERVER_0,
                (UChar*)aArrData,
                (ULong)ID_SIZEOF(sdpstCandidatePage) * aCandidateDataCount );

            for ( sLoop = 0; sLoop < aCandidateDataCount; sLoop++ )
            {
                ideLog::log( IDE_SERVER_0,
                             "aArrData[%u]  : %u\n",
                             sLoop, aArrData[sLoop].mPBS );
            }

            IDE_ASSERT( 0 );
        }


        // Unformat 페이지에 대한 탐색과정이 실패한 경우 Format 이상의
        // 가용도를 가진 페이지에 대해서 NoWait 모드로 수행해본다.
        // SLOT할당이 가능한 PBS을 검사한다.
        sdrMiniTrans::setSavePoint( aMtx, &sInitSP );

        // 후보 Data 페이지를 NoWait or Wiat모드로 X-latch 요청한다.
        sState = 0;
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              aArrData[ sLoop ].mPageID,
                                              SDB_X_LATCH,
                                              aWaitMode,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sPagePtr,
                                              &sTrySuccess,
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );
        sState = 1;

        if ( sTrySuccess == ID_FALSE )
        {
            sState   = 0;
            sPagePtr = NULL;
            continue;
        }

        /* 페이지가 이미 한번 format이 되었기 때문에 판독한 내용을
         * 믿을 수 있다. */
        sPBS = (sdpstPBS)sdpPhyPage::getState((sdpPhyPageHdr*)sPagePtr);
        IDE_DASSERT( sdpstLfBMP::isEqFN( sPBS, SDPST_BITSET_PAGEFN_UNF )
                     == ID_FALSE );

        if ( sdpstLfBMP::isEqFN( sPBS, SDPST_BITSET_PAGEFN_FUL ) == ID_TRUE )
        {
            sState = 0;
            IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                      != IDE_SUCCESS );
            sPagePtr   = NULL;
            continue;
        }

        if ( sdpstLfBMP::isEqFN( sPBS, SDPST_BITSET_PAGEFN_FMT ) == ID_TRUE )
        {
            /* 한번 Format이 되었던 페이지는 Read해서 판독해서 읽어보고
             * 아직도 Format이면 필요한 페이지타입으로 재포맷하고 사용한다 */
            sParentInfo = sdpPhyPage::getParentInfo( sPagePtr );

            if ( (sParentInfo.mParentPID != aLeafBMP) ||
                 (sParentInfo.mIdxInParent != aArrData[sLoop].mPBSNo) )
            {
                /* Page Hex Data And Header */
                (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                             sPagePtr,
                                             "Data Page's Parent Info is invalid "
                                             "(Page ID: %"ID_UINT32_FMT", "
                                             "ParentPID: %"ID_UINT32_FMT", "
                                             "IdxInParent: %"ID_UINT32_FMT", "
                                             "LfBMP PID: %"ID_UINT32_FMT", "
                                             "PBSNo In LfBMP: %"ID_UINT32_FMT")\n",
                                             aArrData[sLoop].mPageID,
                                             sParentInfo.mParentPID,
                                             sParentInfo.mIdxInParent,
                                             aLeafBMP,
                                             aArrData[sLoop].mPBSNo );

                IDE_ASSERT( 0 );
            }

            sOldPageType = sdpPhyPage::getPageType( (sdpPhyPageHdr*)sPagePtr );

            if( ( sOldPageType == SDP_PAGE_DATA ) &&
                ( aPageType    == SDP_PAGE_DATA ) )
            {
                /* BUG-32539 [sm-disk-page] The abnormal shutdown during
                 * executing INSERT make a DRDB Page unrecoverable.
                 * Page는 DataPage로 변경했는데, PBS는 Format는 변경 못한채
                 * 서버가 종료될 수 있음. 이 경우는 이미 DataPage로 변경
                 * 되었으니, 또 변경할 필요 없음. */
            }
            else
            {
                sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

                if( sStartInfo.mTrans != NULL )
                {
                    sNTA = smLayerCallback::getLstUndoNxtLSN( sStartInfo.mTrans );
                }
                   
                IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                               &sMtx,
                                               &sStartInfo,
                                               ID_TRUE,/*Undoable(PROJ-2162)*/
                                               SM_DLOG_ATTR_DEFAULT )
                          != IDE_SUCCESS );
                sState = 2;

                IDE_TEST( sdrMiniTrans::setDirtyPage( &sMtx, sPagePtr )
                          != IDE_SUCCESS );

                sPhyPageHdr = sdpPhyPage::getHdr( sPagePtr );

                IDE_TEST( sdpPhyPage::setPageType( sPhyPageHdr,
                                                   aPageType,
                                                   &sMtx )
                          != IDE_SUCCESS );

                sdbBufferMgr::setPageTypeToBCB( (UChar*)sPhyPageHdr,
                                                (UInt)aPageType );

                if( aPageType == SDP_PAGE_DATA )
                {
                    IDE_TEST( smLayerCallback::logAndInitCTL( &sMtx,
                                                              sPhyPageHdr,
                                                              aSegHandle->mSegAttr.mInitTrans,
                                                              aSegHandle->mSegAttr.mMaxTrans )
                              != IDE_SUCCESS );

                    IDE_TEST( sdpSlotDirectory::logAndInit(
                                  sPhyPageHdr,
                                  &sMtx) 
                              != IDE_SUCCESS );
                }
                else
                {
                    if( (aPageType == SDP_PAGE_LOB_DATA) ||
                        (aPageType == SDP_PAGE_LOB_INDEX) )
                    {
                        if( sMtx.mTrans != NULL )
                        {
                            IDE_ASSERT( sStartInfo.mTrans != NULL );
                            
                            sNTAData[0] = aSegHandle->mSegPID;
                            sNTAData[1] = sdpPhyPage::getPageIDFromPtr(sPagePtr);

                            (void)sdrMiniTrans::setNTA(
                                &sMtx,
                                aSpaceID,
                                SDR_OP_SDPST_ALLOC_PAGE,
                                &sNTA,
                                sNTAData,
                                2 /* DataCount */ );
                        }
                    }
                    
                    sPBS = (sdpstPBS)
                        (SDPST_BITSET_PAGETP_DATA|SDPST_BITSET_PAGEFN_FUL);
                    IDU_FIT_POINT("2.BUG-42505@sdpstFindPage::tryToAllocExtsAndPage::setState");
                    IDE_TEST( sdpPhyPage::setState( sPhyPageHdr, sPBS, &sMtx )
                              != IDE_SUCCESS );

                    // Table Page가 아닌경우에는 미리 bitmap을 갱신한다.
                    sdpstStackMgr::initialize(&sRevStack);

                    // leaf bmp 페이지부터 MFNL을 변경시도하여
                    // 상위노드로 전파한다.
                    IDE_TEST( sdpstAllocPage::tryToChangeMFNLAndItHint(
                                  aStatistics,
                                  &sMtx,
                                  aSpaceID,
                                  aSegHandle,
                                  SDPST_CHANGEMFNL_LFBMP_PHASE,
                                  sParentInfo.mParentPID,  // leaf pid
                                  sParentInfo.mIdxInParent,
                                  (void*)&sPBS,
                                  SD_NULL_PID,            // unuse
                                  SDPST_INVALID_SLOTNO,  // unuse
                                  &sRevStack ) != IDE_SUCCESS );
                }
                
                sState = 1;
                IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

                if ( sdpPhyPage::getPageType( (sdpPhyPageHdr*)sPagePtr )
                     != aPageType )
                {
                    /* Page Hex Data And Header */
                    (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                                 sPagePtr,
                                                 "Data Page has invalid Page Type "
                                                 "(Page ID: %"ID_UINT32_FMT", "
                                                 "PageType: %"ID_UINT32_FMT", "
                                                 "Requested PageType: %"ID_UINT32_FMT")\n",
                                                 aArrData[sLoop].mPageID,
                                                 sdpPhyPage::getPageType((sdpPhyPageHdr*)sPagePtr),
                                                 aPageType );

                    IDE_ASSERT( 0 );
                }
            }

            break;
        }
        else
        {
            if ( aSearchType == SDPST_SEARCH_NEWPAGE )
            {
                // 완전히 빈페이지가 아니여서 페이지 할당에 부적합하여
                // 다음 후보 데이타페이지로 넘어간다.
                sState = 0;
                IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                          != IDE_SUCCESS );
                sPagePtr   = NULL;
                continue;
            }

            // 실제로 X-Latch가 획득되면, Insert High Limit을 고려한
            // 가용공간크기와 Row 크기를 저장할 수 있을지 판단한다.
            // 만약 저장할 수 없다면, Data 페이지 상태를 Full로 변경한다.
            // 다른 Row크기를 저장할수 있을지도 모르지만, 공간탐색성능을
            // 위해 저장할 수 없는 경우는 페이지 상태를 Full로 변경하여
            // 탐색비용을 줄일수 있게 한다.
            // ( 기존 세그먼트와 동일한 정책이다)
            IDE_TEST(  checkSizeAndAllocCTS( aStatistics,
                                             aMtx,
                                             aSpaceID,
                                             aSegHandle,
                                             sPagePtr,
                                             aRowSize,
                                             &sCanAlloc,
                                             &sCTSlotNo ) != IDE_SUCCESS );

            if ( sCanAlloc == ID_TRUE )
            {
                break;
            }
            else
            {
                sState = 0;
                IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                          != IDE_SUCCESS );
            }
        }

        sPagePtr   = NULL;
        sCTSlotNo = SDP_CTS_IDX_NULL;
    }

    *aPagePtr  = sPagePtr;
    *aCTSlotNo = sCTSlotNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                        == IDE_SUCCESS );
        default:
            break;
    }

    *aPagePtr   = NULL;
    *aCTSlotNo = SDP_CTS_IDX_NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table Data 페이지의 Freeness 정보를 변경해야하는지
 *               확인한다.
 *
 *  aPageHdr       - [IN]  physical page header
 *  aSegHandle     - [IN]  Segment Handle
 *  aNewPBS        - [OUT] page bitset
 ***********************************************************************/
idBool sdpstFindPage::needToChangePageFN( sdpPhyPageHdr    * aPageHdr,
                                          sdpSegHandle     * aSegHandle,
                                          sdpstPBS         * aNewPBS )
{
    idBool          sIsInsertable = ID_FALSE;
    idBool          sIsChangePBS  = ID_FALSE;
    sdpstPBS        sPBS;
    UChar         * sPagePtr;

    IDE_DASSERT( aPageHdr  != NULL );
    IDE_DASSERT( aNewPBS != NULL );

    sPBS     = (sdpstPBS)sdpPhyPage::getState( aPageHdr );
    *aNewPBS = sPBS;

    switch ( sPBS )
    {
        case SDPST_BITSET_PAGEFN_FMT:
            sIsInsertable = ID_TRUE;

        case SDPST_BITSET_PAGEFN_INS:

            if( isPageUpdateOnly( 
                      aPageHdr, aSegHandle->mSegAttr.mPctFree ) == ID_TRUE )
            {
                *aNewPBS = SDPST_BITSET_PAGEFN_FUL;
                sIsChangePBS = ID_TRUE;
            }
            else
            {
                if ( sIsInsertable == ID_TRUE )
                {
                    *aNewPBS = SDPST_BITSET_PAGEFN_INS;
                    sIsChangePBS = ID_TRUE;
                }
            }
            break;

        case SDPST_BITSET_PAGEFN_FUL:

            if( isPageInsertable( 
                      aPageHdr, aSegHandle->mSegAttr.mPctUsed ) == ID_TRUE )
            {
                *aNewPBS = SDPST_BITSET_PAGEFN_INS;
                sIsChangePBS = ID_TRUE;
            }
            break;

        case SDPST_BITSET_PAGEFN_UNF:
        default:
            sPagePtr = sdpPhyPage::getPageStartPtr( aPageHdr );

            /* Page Hex Data And Header */
            (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                         sPagePtr,
                                         "Invalid PageBitSet: %"ID_UINT32_FMT"\n",
                                         sPBS );
            IDE_ASSERT( 0 );
            break;
    }

    return sIsChangePBS;
}

/***********************************************************************
 * Description : 트랜잭션 TableInfo의 마지막 가용공간 할당했던 Data
 *               페이지에서 가용공간 탐색 시작한다.
 ***********************************************************************/
IDE_RC sdpstFindPage::checkAndSearchHintDataPID(
                           idvSQL             * aStatistics,
                           sdrMtx             * aMtx,
                           scSpaceID            aSpaceID,
                           sdpSegHandle       * aSegHandle,
                           void               * aTableInfo,
                           scPageID             aHintDataPID,
                           UInt                 aRowSize,
                           UChar             ** aPagePtr,
                           UChar              * aCTSlotNo,
                           scPageID           * aNewHintDataPID )
{
    idBool          sCanAlloc;
    idBool          sTrySuccess;
    sdrSavePoint    sInitSP;
    UChar         * sPagePtr   = NULL;
    UChar           sCTSlotNo = SDP_CTS_IDX_NULL;
    UInt            sState = 0;

    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aTableInfo != NULL );
    IDE_ASSERT( aPagePtr   != NULL );
    IDE_ASSERT( aCTSlotNo  != NULL );

    *aNewHintDataPID = aHintDataPID;

    if ( aHintDataPID != SD_NULL_PID )
    {
        sdrMiniTrans::setSavePoint( aMtx, &sInitSP );

        // No-Wait 모드로 Data 페이지에 X-latch를 획득한다.
        sState = 0;
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              aHintDataPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NO,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sPagePtr,
                                              &sTrySuccess,
                                              NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );
        sState = 1;

        if ( sTrySuccess == ID_FALSE )
        {
            sState = 0;
            sPagePtr = NULL;
            sCTSlotNo = SDP_CTS_IDX_NULL;
            IDE_CONT( cont_skip_datapage );
        }


        if ( ((sdpstPBS)
               sdpPhyPage::getState( (sdpPhyPageHdr*)sPagePtr )) !=
               ((sdpstPBS)
               (SDPST_BITSET_PAGETP_DATA|SDPST_BITSET_PAGEFN_FUL)) )
        {
            // 마지막 insert한 Data 페이지에서 가용공간을 탐색한다.
            // 만약, Data 페이지의 가용공간이 존재하지 않는 경우에는
            // Hint Leaf BMP에서 탐색을 시도한다. Leaf BMP는 Insert
            // Data 페이지에서 얻어낸다.
            IDE_TEST( checkSizeAndAllocCTS( aStatistics,
                        aMtx,
                        aSpaceID,
                        aSegHandle,
                        sPagePtr,
                        aRowSize,
                        &sCanAlloc,
                        &sCTSlotNo ) != IDE_SUCCESS );

            if ( sCanAlloc == ID_FALSE )
            {
                sState = 0;
                IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                        != IDE_SUCCESS );

                sPagePtr   = NULL;
                sCTSlotNo = SDP_CTS_IDX_NULL;

                smLayerCallback::setHintDataPIDofTableInfo( aTableInfo, SD_NULL_PID );

                *aNewHintDataPID = SD_NULL_PID;
            }
            else
            {
                // 페이지에 가용공간할당이 가능하면 탐색연산을 완료한다.
                // 페이지 상태 변경은 할당이후에 처리한다.
            }
        }
        else
        {
            sState = 0;
            IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )
                      != IDE_SUCCESS );
            // Hint Data 페이지에 대해서 가용공간할당이 불가능하여
            // Hint Data 페이지를 초기화한다.
            smLayerCallback::setHintDataPIDofTableInfo( aTableInfo, SD_NULL_PID );

            sPagePtr = NULL;
            sCTSlotNo = SDP_CTS_IDX_NULL;
            *aNewHintDataPID = SD_NULL_PID;
        }
    }

    IDE_EXCEPTION_CONT( cont_skip_datapage );

    *aPagePtr   = sPagePtr;
    *aCTSlotNo = sCTSlotNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::releaseLatchToSP( aMtx, &sInitSP )      
                    == IDE_SUCCESS );
    }

    *aPagePtr   = NULL;
    *aCTSlotNo = SDP_CTS_IDX_NULL;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 페이지의 가용공간을 확인하고, TTS를 Bind한다.
 ***********************************************************************/
IDE_RC sdpstFindPage::checkSizeAndAllocCTS( idvSQL          * aStatistics,
                                            sdrMtx          * aMtx,
                                            scSpaceID         aSpaceID,
                                            sdpSegHandle    * aSegHandle,
                                            UChar           * aPagePtr,
                                            UInt              aRowSize,
                                            idBool          * aCanAlloc,
                                            UChar           * aCTSlotNo )
{
    idBool            sUpdatePageStateToFull;
    idBool            sCanAllocSlot;
    idBool            sAfterSelfAging;
    sdrMtxStartInfo   sStartInfo;
    UChar             sCTSlotNo;
    sdpPhyPageHdr   * sPageHdr;
    sdpSelfAgingFlag  sCheckFlag = SDP_SA_FLAG_NOTHING_TO_AGING;

    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aPagePtr   != NULL );
    IDE_ASSERT( aCTSlotNo != NULL );

    sCanAllocSlot          = ID_FALSE;
    sUpdatePageStateToFull = ID_TRUE;
    sCTSlotNo             = SDP_CTS_IDX_NULL;
    sAfterSelfAging        = ID_FALSE;

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    while( 1 )
    {
        /*
         * PRJ-1704 Disk MVCC Renewal
         *
         * Insert 수행 가능한 페이지에 대해서 TTS 할당과정에서
         * Soft Row TimeStamping을 할수 있는 TTS들에 대해 수행하여
         * 확보할 수 있는 가용공간을 확보하기도 한다.
         *
         * 무엇보다 더 TTS를 할당하는 것이 더 중요하기 때문에 Insert의
         * 경우 확장을 하지도 못하고 Free인 TTS가 존재하지 않아서
         * 할당하지 못하면, 다른 Page를 검색하도록 해당 Page에 대한
         * 확인과정을 완료한다.
         */
        sPageHdr = (sdpPhyPageHdr*)aPagePtr;

        IDE_TEST( smLayerCallback::allocCTSAndSetDirty( aStatistics,
                                                        NULL,         /* aFixMtx */
                                                        &sStartInfo,  /* for Logging */
                                                        sPageHdr,
                                                        &sCTSlotNo ) != IDE_SUCCESS );

        if ( ( smLayerCallback::getCountOfCTS( sPageHdr ) > 0 ) &&
             ( sCTSlotNo == SDP_CTS_IDX_NULL ) )
        {
            sCanAllocSlot = ID_FALSE;
        }
        else
        {
            sCanAllocSlot = sdpPhyPage::canAllocSlot( (sdpPhyPageHdr*)aPagePtr,
                                                      aRowSize,
                                                      ID_TRUE /* create slotentry */,
                                                      SDP_1BYTE_ALIGN_SIZE );
        }

        if ( sCanAllocSlot == ID_TRUE )
        {
            sUpdatePageStateToFull = ID_FALSE;
            break;
        }
        else
        {
            // Available Free Size가 부족하지만,
            // Self-Aging 하면 가능할 수도 있다.

            // Page의 Available FreeSize가 충분하지 않아도 이후에
            // Total Free Size는 충분하기 때문에 다시 접근할 수 있도록
            // 여지를 만들어두어야 한다. 그러므로 페이지 상태를
            // FULL 상태로 변경하지 않는다.
            sCTSlotNo = SDP_CTS_IDX_NULL;
        }

        if ( sAfterSelfAging == ID_TRUE )
        {
            if ( sCheckFlag != SDP_SA_FLAG_NOTHING_TO_AGING )
            {
                // 만약 Long-term 트랜잭션이 존재해서
                // SelfAging을 할수 없는 경우에는 Insertable Page가
                // 더 많아져서 Insert 성능저하의 원인이 될수있다.
                sUpdatePageStateToFull = ID_FALSE;
            }
            break;
        }

        IDE_TEST( smLayerCallback::checkAndRunSelfAging( aStatistics,
                                                         &sStartInfo,
                                                         (sdpPhyPageHdr*)aPagePtr,
                                                         &sCheckFlag )
                  != IDE_SUCCESS );

        sAfterSelfAging = ID_TRUE;
    }

    if ( sUpdatePageStateToFull == ID_TRUE )
    {
        IDE_TEST( sdpstAllocPage::updatePageFNtoFull( aStatistics,
                                                      aMtx,
                                                      aSpaceID,
                                                      aSegHandle,
                                                      aPagePtr ) 
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    *aCanAlloc  = sCanAllocSlot;
    *aCTSlotNo = sCTSlotNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
