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
 * $Id:$
 ***********************************************************************/
#include <sdr.h>
#include <sdp.h>
#include <sdpPhyPage.h>
#include <sdpSglRIDList.h>

#include <sdpsfExtent.h>

#include <sdpsfDef.h>
#include <sdpsfSH.h>
#include <sdpReq.h>
#include <sdpsfExtMgr.h>
#include <sdpsfAllocPage.h>

/***********************************************************************
 * Description : 완전히 빈페이지를 찾는다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegHdr      - [IN] Segment Header
 * aSegHandle   - [IN] Segment Handle
 * aPageType    - [IN] Page Type
 *
 * aPagePtr     - [OUT] 완전히 빈 Page에 대한 Pointer가 설정된다.
 *                      return 시 해당 페이지에 XLatch가 걸려있다.
 *
 ***********************************************************************/
IDE_RC sdpsfAllocPage::allocNewPage( idvSQL        *aStatistics,
                                     sdrMtx        *aMtx,
                                     scSpaceID      aSpaceID,
                                     sdpSegHandle  *aSegHandle,
                                     sdpsfSegHdr   *aSegHdr,
                                     sdpPageType    aPageType,
                                     UChar        **aPagePtr )
{
    UChar             *sPagePtr = NULL;
    sdpPhyPageHdr     *sPageHdr = NULL;
    sdrMtxStartInfo    sStartInfo;
    scPageID           sAllocPID;
    sdrMtx             sAllocMtx;
    ULong              sNTAData[2];
    SInt               sState = 0;
    smLSN              sNTA;

    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aSpaceID    != 0 );
    IDE_ASSERT( aSegHandle  != NULL );
    IDE_ASSERT( aSegHdr     != NULL );
    IDE_ASSERT( aPageType   < SDP_PAGE_TYPE_MAX );
    IDE_ASSERT( aPagePtr    != NULL );

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    if( sStartInfo.mTrans != NULL )
    {
        sNTA = smLayerCallback::getLstUndoNxtLSN( sStartInfo.mTrans );
    }

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sAllocMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    /* Private PageList에서 찾는다. */
    IDE_TEST( sdpsfPvtFreePIDList::removeAtHead(
                  aStatistics,
                  &sAllocMtx,
                  aMtx, /* BM Create Page Mtx */
                  aSpaceID,
                  aSegHdr,
                  aPageType,
                  &sAllocPID,
                  &sPagePtr )
              != IDE_SUCCESS );

    if( sPagePtr == NULL )
    {
        /* UnFormat PageList에서 찾는다. */
        IDE_TEST( sdpsfUFmtPIDList::removeAtHead(
                      aStatistics,
                      &sAllocMtx,
                      aMtx, /* BM Create Page Mtx */
                      aSpaceID,
                      aSegHdr,
                      aPageType,
                      &sAllocPID,
                      &sPagePtr )
                  != IDE_SUCCESS );
    }

    if( sPagePtr == NULL )
    {
        /* ExtList에서 새로운 페이지를 확장한다. */
        IDE_TEST( sdpsfExtMgr::allocPage( aStatistics,
                                          &sAllocMtx,
                                          aMtx, /* BM Create Page Mtx */
                                          aSpaceID,
                                          aSegHdr,
                                          aSegHandle,
                                          aSegHandle->mSegStoAttr.mNextExtCnt,
                                          aPageType,
                                          &sAllocPID,
                                          &sPagePtr )
                  != IDE_SUCCESS );
    }

    sPageHdr = sdpPhyPage::getHdr( sPagePtr );

    IDE_ASSERT( sPagePtr != NULL );
    IDE_ASSERT( sPageHdr != NULL );

    /* FIT/ART/sm/Projects/PROJ-1671/freelist-seg/alloc_page.tc */
    IDU_FIT_POINT( "1.PROJ-1671@sdpsfAllocPage::allocNewPage" );

    if( aPageType != SDP_PAGE_DATA )
    {
        /* TABLE 페이지가 아닌경우는 UPDATE ONLY로 변경한다. FREE되지않는경우외에는
         * 다시 Free페이지가 될수 없다. */
        IDE_TEST( sdpPhyPage::setState( sPageHdr,
                                        (UShort)SDPSF_PAGE_USED_UPDATE_ONLY,
                                        aMtx )
                  != IDE_SUCCESS );

        sNTAData[0] = sdpPhyPage::getPageIDFromPtr( aSegHdr );
        sNTAData[1] = sAllocPID;

        sdrMiniTrans::setNTA( &sAllocMtx,
                              aSpaceID,
                              SDR_OP_SDPSF_ALLOC_PAGE,
                              &sNTA,
                              sNTAData,
                              2 /* DataCount */ );
    }
    else
    {
        IDE_TEST( sdpsfFreePIDList::addPage( &sAllocMtx,
                                             aSegHdr,
                                             sPagePtr )
                  != IDE_SUCCESS );

        /* Initialize Change Transaction Layer */
        IDE_TEST( smLayerCallback::logAndInitCTL( &sAllocMtx,
                                                  sPageHdr,
                                                  aSegHandle->mSegAttr.mInitTrans,
                                                  aSegHandle->mSegAttr.mMaxTrans )
                  != IDE_SUCCESS );

        IDE_TEST( sdpSlotDirectory::logAndInit(sPageHdr,
                                               &sAllocMtx)
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sAllocMtx ) != IDE_SUCCESS );
   
    /* FIT/ART/sm/Projects/PROJ-1671/freelist-seg/alloc_page.tc */
    IDU_FIT_POINT( "2.PROJ-1671@sdpsfAllocPage::allocNewPage" );

    *aPagePtr = sPagePtr;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sAllocMtx ) == IDE_SUCCESS );
    }

    *aPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Segment에 aCountWanted만큼의 Free 페이지가 존재하는지
 *               Check한다. 만약 없다면 새로운 Extent를 할당한다.
 *
 * aStatistics   - [IN] 통계 정보
 * aMtx          - [IN] Mini Transaction Pointer
 * aSpaceID      - [IN] TableSpace ID
 * aSegHandle    - [IN] Segment Handle
 * aCountWanted  - [IN] 확보하길 원하는 페이지 갯수
 *
 ***********************************************************************/
IDE_RC sdpsfAllocPage::prepareNewPages( idvSQL            * aStatistics,
                                        sdrMtx            * aMtx,
                                        scSpaceID           aSpaceID,
                                        sdpSegHandle      * aSegHandle,
                                        UInt                aCountWanted )
{
    sdpsfSegHdr      *sSegHdr;
    sdrMtx            sPrepMtx;
    sdrMtxStartInfo   sStartInfo;
    ULong             sAllocPageCnt;
    ULong             sPageCntNotAlloc;
    ULong             sFreePageCnt;
    SInt              sState = 0;

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSpaceID     != 0 );
    IDE_ASSERT( aSegHandle   != NULL );
    IDE_ASSERT( aCountWanted != 0 );

    sStartInfo.mTrans   = sdrMiniTrans::getTrans( aMtx );
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sPrepMtx,
                                   &sStartInfo,
                                   ID_TRUE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               &sPrepMtx,
                                               aSpaceID,
                                               aSegHandle->mSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );

    while( 1 )
    {
        sAllocPageCnt = sdpsfSH::getFmtPageCnt( sSegHdr );

        /* HWM이후에 존재하는 페이지갯수 계산 */
        sPageCntNotAlloc = sdpsfSH::getTotalPageCnt( sSegHdr ) - sAllocPageCnt;

        sFreePageCnt = sdpsfSH::getFreePageCnt( sSegHdr ) + sPageCntNotAlloc;

        /* Free페이지가 부족하면 새로운 Extent를 TBS로 부터 요구한다. */
        if( sFreePageCnt < aCountWanted )
        {
            if( sdpsfExtMgr::allocExt( aStatistics,
                                       &sStartInfo,
                                       aSpaceID,
                                       sSegHdr )
                != IDE_SUCCESS )
            {
                /* TBS FULL */
                sState = 0;
                IDE_TEST( sdrMiniTrans::commit( &sPrepMtx ) != IDE_SUCCESS );
                IDE_TEST( 1 );
            }
            else
            {
                /* nothing to do ... */
            }
        }
        else
        {
            break;
        }
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sPrepMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sPrepMtx )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 완전히 빈 페이지를 할당한다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegHandle   - [IN] Segment Handle
 * aPageType    - [IN] Page Type
 *
 * aPagePtr     - [OUT] Free공간을 가진 Page에 대한 Pointer가 설정된다.
 *                      return 시 해당 페이지에 XLatch가 걸려있다.
 *
 ***********************************************************************/
IDE_RC sdpsfAllocPage::allocPage( idvSQL        *aStatistics,
                                  sdrMtx        *aMtx,
                                  scSpaceID      aSpaceID,
                                  sdpSegHandle * aSegHandle,
                                  sdpPageType    aPageType,
                                  UChar        **aPagePtr )
{
    sdpsfSegHdr  *sSegHdr;
    SInt          sState = 0;

    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aSpaceID    != 0 );
    IDE_ASSERT( aSegHandle  != NULL );
    IDE_ASSERT( aPageType   < SDP_PAGE_TYPE_MAX );
    IDE_ASSERT( aPagePtr    != NULL );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               aSpaceID,
                                               aSegHandle->mSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( allocNewPage( aStatistics,
                            aMtx,
                            aSpaceID,
                            aSegHandle,
                            sSegHdr,
                            aPageType,
                            aPagePtr )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdpsfSH::releaseSegHdr( aStatistics,
                                      sSegHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdpsfSH::releaseSegHdr( aStatistics,
                                            sSegHdr )
                    == IDE_SUCCESS );
    }

    *aPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPagePtr이 가리키는 페이지가 완전히 빈 페이지가 되어서
 *               UFmtPageList에 추가한다. 이때 이미 페이지가 다른 PageList에
 *               속해 있다면 그냥 둔다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegHandle   - [IN] Segment Handle
 * aPagePtr     - [IN] Free할 Page Pointer
 *
 ***********************************************************************/
IDE_RC sdpsfAllocPage::freePage( idvSQL            * aStatistics,
                                 sdrMtx            * aMtx,
                                 scSpaceID           aSpaceID,
                                 sdpSegHandle      * aSegHandle,
                                 UChar             * aPagePtr )
{
    sdpsfSegHdr  *sSegHdr;

    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aSpaceID    != 0 );
    IDE_ASSERT( aSegHandle  != NULL );
    IDE_ASSERT( aPagePtr    != NULL );

    /* Page가 이미 특정 Free PID List에 속해있는지 Check한다. */
    if( ((sdpPhyPageHdr*)aPagePtr)->mLinkState == SDP_PAGE_LIST_UNLINK )
    {
        IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                                   aMtx,
                                                   aSpaceID,
                                                   aSegHandle->mSegPID,
                                                   &sSegHdr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpsfUFmtPIDList::add2Head( aMtx,
                                              sSegHdr,
                                              aPagePtr )
                  != IDE_SUCCESS );

        /* FIT/ART/sm/Projects/PROJ-1671/freelist-seg/free_page.tc */
        IDU_FIT_POINT( "1.PROJ-1671@sdpsfAllocPage::freePage" );

        /* BUG-32942 When executing rebuild Index stat, abnormally shutdown */
        IDE_TEST( sdpPhyPage::setState( (sdpPhyPageHdr*)aPagePtr,
                                        (UShort)SDPSF_PAGE_FREE,
                                        aMtx )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 페이지가 Free된 여부를 반환한다.
 *
 * BUG-32942 When executing rebuild Index stat, abnormally shutdown
 *
 * aPagePtr     - [IN] Page Pointer
 *
 ***********************************************************************/
idBool sdpsfAllocPage::isFreePage( UChar * aPagePtr )
{
    idBool sRet = ID_FALSE;

    if( ((sdpPhyPageHdr*)aPagePtr)->mPageState == SDPSF_PAGE_FREE )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

/***********************************************************************
 * Description : aPagePtr를 Free PID List에 추가한다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegRID      - [IN] Segment RID
 * aPagePtr     - [IN] Free할 Page Pointer
 *
 ***********************************************************************/
IDE_RC sdpsfAllocPage::addPageToFreeList( idvSQL          * aStatistics,
                                          sdrMtx          * aMtx,
                                          scSpaceID         aSpaceID,
                                          scPageID          aSegPID,
                                          UChar           * aPagePtr )
{
    sdpsfSegHdr      *sSegHdr;
    sdrMtxStartInfo   sStartInfo;
    sdrMtx            sAddMtx;
    SInt              sState = 0;

    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aSpaceID    != 0 );
    IDE_ASSERT( aSegPID     != SD_NULL_PID );
    IDE_ASSERT( aPagePtr    != NULL );

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sAddMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               aMtx,
                                               aSpaceID,
                                               aSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfFreePIDList::addPage( aMtx,
                                         sSegHdr,
                                         aPagePtr )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sAddMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sAddMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPrvAllocExtRID가 가리키는 Extent에 aPrvAllocPageID이후
 *               Page가 존재하는 하는지 체크해서 없으면 새로운 다음
 *               Extent로 이동하고 다음 Extent가 없으면 TBS로 부터 새로운
 *               Extent를 할당받는다. 이후 Extent에서 Free Page를 찾아서
 *               Page가 할당된 ExtRID와 PageID를 넘겨준다.
 *
 * Caution:
 *  1. 이 함수가 호출될때 SegHdr가 있는 페이지에 XLatch가 걸려 있어야 한다.
 *
 * aStatistics          - [IN] 통계 정보
 * aMtx                 - [IN] Mini Transaction Pointer
 * aSpaceID             - [IN] TableSpace ID
 * aSegHandle           - [IN] Segment Handle
 * aPrvAllocExtRID      - [IN] 이전에 Page를 할당받았던 Extent RID
 * aFstPIDOfPrvAllocExt - [IN] 이전에 할당된 Extent의 첫번째 PID
 * aPrvAllocPageID      - [IN] 이전에 할당받은 PageID
 *
 * aAllocExtRID      - [OUT] 새로운 Page가 할당된 Extent RID
 * aFstPIDOfAllocExt - [OUT] 할당받은 Page가 있는 Extent의 첫번째 페이지 ID
 * aAllocPID         - [OUT] 새롭게 할당받은 PageID
 * aAllocPagePtr     - [OUT] 할당된 페이지에 대한 Pointer, 리턴될때 XLatch가
 *                           걸려있다.
 ***********************************************************************/
IDE_RC sdpsfAllocPage::allocNewPage4Append( idvSQL               *aStatistics,
                                            sdrMtx               *aMtx,
                                            scSpaceID             aSpaceID,
                                            sdpSegHandle         *aSegHandle,
                                            sdRID                 aPrvAllocExtRID,
                                            scPageID              aFstPIDOfPrvAllocExt,
                                            scPageID              aPrvAllocPageID,
                                            idBool                aIsLogging,
                                            sdpPageType           aPageType,
                                            sdRID                *aAllocExtRID,
                                            scPageID             *aFstPIDOfAllocExt,
                                            scPageID             *aAllocPID,
                                            UChar               **aAllocPagePtr )
{
    sdpsfSegHdr    *sSegHdr;
    UChar          *sAllocPagePtr;
    SInt            sState = 0;
    sdrMtx          sMtx;
    sdrMtxStartInfo sStartInfo;

    IDE_ASSERT( aMtx              != NULL );
    IDE_ASSERT( aSpaceID          != 0 );
    IDE_ASSERT( aSegHandle        != NULL );
    IDE_ASSERT( aPageType          < SDP_PAGE_TYPE_MAX );
    IDE_ASSERT( aAllocExtRID      != NULL );
    IDE_ASSERT( aFstPIDOfAllocExt != NULL );
    IDE_ASSERT( aAllocPID         != NULL );
    IDE_ASSERT( aAllocPagePtr     != NULL );

    sStartInfo.mTrans   = sdrMiniTrans::getTrans( aMtx );
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               &sMtx,
                                               aSpaceID,
                                               aSegHandle->mSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfExtMgr::allocNewPage( aStatistics,
                                         &sMtx,
                                         aSpaceID,
                                         sSegHdr,
                                         aSegHandle,
                                         1, /* Next Ext Cnt */
                                         aPrvAllocExtRID,
                                         aFstPIDOfPrvAllocExt,
                                         aPrvAllocPageID,
                                         aAllocExtRID,
                                         aFstPIDOfAllocExt,
                                         aAllocPID,
                                         ID_TRUE )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::create4DPath( aStatistics,
                                        sdrMiniTrans::getTrans( aMtx ),
                                        aSpaceID,
                                        *aAllocPID,
                                        NULL, /* Parent Info */
                                        SDPSF_PAGE_USED_UPDATE_ONLY,
                                        aPageType,
                                        ((sdpSegCCache*)aSegHandle->mCache)
                                            ->mTableOID,
                                        aIsLogging,
                                        &sAllocPagePtr )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::logAndInitCTL( aMtx,
                                              (sdpPhyPageHdr*)sAllocPagePtr,
                                              aSegHandle->mSegAttr.mInitTrans,
                                              aSegHandle->mSegAttr.mMaxTrans )
              != IDE_SUCCESS );

    IDE_TEST( sdpSlotDirectory::logAndInit((sdpPhyPageHdr*)sAllocPagePtr,
                                           aMtx)
              != IDE_SUCCESS );

    *aAllocPagePtr = sAllocPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    *aAllocExtRID  = SD_NULL_RID;
    *aAllocPID     = SD_NULL_PID;
    *aAllocPagePtr = NULL;

    return IDE_FAILURE;
}
