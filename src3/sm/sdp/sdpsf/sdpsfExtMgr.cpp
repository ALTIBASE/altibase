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
#include <smErrorCode.h>
#include <sdr.h>
#include <sdp.h>
#include <sdpPhyPage.h>
#include <sdpSglRIDList.h>

#include <sdpsfDef.h>
#include <sdpsfSH.h>
#include <sdpReq.h>
#include <sdpsfExtent.h>
#include <sdpsfExtDirPageList.h>
#include <sdptbExtent.h>
#include <sdpsfExtMgr.h>

/***********************************************************************
 * Description:
 ***********************************************************************/
IDE_RC sdpsfExtMgr::initialize()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description:
 ***********************************************************************/
IDE_RC sdpsfExtMgr::destroy()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Segment를 Create당시의 초기상태로 만든다.
 *
 * Caution: aSegHdr에 XLatch가 걸려서 온다.
 *
 * aStatistics      - [IN] 통계 정보
 * aStartInfo       - [IN] Start Info
 * aSpaceID         - [IN] SpaceID
 * aSegHdr          - [IN] Segment Desc
 *
 * aNewExtRID       - [OUT] 할당된 Extent의 RID
 * aFstPIDOfExt     - [OUT] Extent의 첫번째 PID
 * aFstDataPIDOfExt - [OUT] 할당된 Extent의 First Data Page ID
 ***********************************************************************/
IDE_RC sdpsfExtMgr::allocExt( idvSQL          * aStatistics,
                              sdrMtxStartInfo * aStartInfo,
                              scSpaceID         aSpaceID,
                              sdpsfSegHdr     * aSegHdr )
{
    sdrMtx                sMtx;
    UInt                  sState = 0;
    idBool                sIsSegXLatched;
    sdRID                 sNewExtRID;
    UChar               * sSegPagePtr;
    idBool                sIsSuccess;
    smLSN                 sNTA;
    scPageID              sFstPIDOfExt;
    sdpExtDesc            sExtDesc;
    sdpsfExtDesc        * sAllocExtDesc;
    sdpsfExtDirCntlHdr  * sExtDirCntlHdr;

    IDE_ASSERT( aSpaceID         != 0 );
    IDE_ASSERT( aStartInfo       != NULL );
    IDE_ASSERT( aSegHdr          != NULL );

    sIsSegXLatched = ID_TRUE;

    if( aStartInfo->mTrans != NULL )
    {
        sNTA = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );
    }
    else
    {
        /* no logging */
    }

    sSegPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );

    /* SegHdr에 대해서 XLatch를 푼다. 여기서 Latch를 풀기때문에 SegHdr에 대해서
     * 갱신한 Mini Transaction중에서 Begin중인것이 있으면 안됩니다. */
    sIsSegXLatched = ID_FALSE;

    sdbBufferMgr::unlatchPage( sSegPagePtr );

    IDE_TEST( sdptbExtent::allocExts( aStatistics,
                                      aStartInfo,
                                      aSpaceID,
                                      1, /* alloc extent count */
                                      &sExtDesc ) != IDE_SUCCESS );

    sFstPIDOfExt = sExtDesc.mExtFstPID;


    sdbBufferMgr::latchPage( aStatistics,
                             sSegPagePtr,
                             SDB_X_LATCH,
                             SDB_WAIT_NORMAL,
                             &sIsSuccess  );
    sIsSegXLatched = ID_TRUE;

    /* 할당한 Extent를 Segment에 붙인다. */
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    /* SegHdr에 대해 갱신하기 때문에 Dirty로 등록한다. */
    IDE_TEST( sdrMiniTrans::setDirtyPage( &sMtx,
                                          sSegPagePtr )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfExtDirPageList::addExtDesc( aStatistics,
                                               &sMtx,
                                               aSpaceID,
                                               aSegHdr,
                                               sFstPIDOfExt,
                                               &sExtDirCntlHdr,
                                               &sNewExtRID,
                                               &sAllocExtDesc )
              != IDE_SUCCESS );

    IDE_ASSERT( sAllocExtDesc != NULL );

    if ( aStartInfo->mTrans != NULL )
    {
        sdrMiniTrans::setNullNTA( &sMtx,
                                  aSpaceID,
                                  &sNTA );
    }
    else
    {
        /* no logging */
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    if( sIsSegXLatched == ID_FALSE )
    {
        sdbBufferMgr::latchPage( aStatistics,
                                 sSegPagePtr,
                                 SDB_X_LATCH,
                                 SDB_WAIT_NORMAL,
                                 &sIsSuccess  );

        /* BUGBUG: 실패하는 경우가 있는가? */
        IDE_ASSERT( sIsSuccess == ID_TRUE );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment를 Create당시의 초기상태로 만든다.
 *
 * Caution: aSegHdr에 XLatch가 걸려서 온다.
 *
 * aStatistics      - [IN] 통계 정보
 * aStartInfo       - [IN] Start Info
 * aSpaceID         - [IN] SpaceID
 * aSegHdr          - [IN] Segment Header
 * aNxtExtCnt       - [IN] Segment확장시 늘어나는 Extent의 갯수
 *
 * aNewExtRID       - [OUT] 할당된 Extent의 RID
 * aFstPIDOfExt     - [OUT] Extent의 첫번재 PID
 * aFstDataPIDOfExt - [OUT] 할당된 Extent의 First Data Page ID
 ***********************************************************************/
IDE_RC sdpsfExtMgr::extend( idvSQL          * aStatistics,
                            sdrMtxStartInfo * aStartInfo,
                            scSpaceID         aSpaceID,
                            sdpsfSegHdr     * aSegHdr,
                            sdpSegHandle    * aSegHandle,
                            UInt              aNxtExtCnt )
{
    UInt      sLoop;
    UInt      sMaxExtCnt;
    UInt      sCurExtCnt;

    IDE_ASSERT( aStartInfo       != NULL );
    IDE_ASSERT( aSpaceID         != 0 );
    IDE_ASSERT( aSegHdr          != NULL );
    IDE_ASSERT( aNxtExtCnt       != 0 );

    sMaxExtCnt = aSegHandle->mSegStoAttr.mMaxExtCnt;
    sCurExtCnt = sdpsfExtMgr::getExtCnt( aSegHdr );

    IDE_TEST_RAISE( sCurExtCnt + 1 > sMaxExtCnt,
            error_exceed_segment_maxextents );

    for( sLoop = 0; sLoop < aNxtExtCnt; sLoop++ )
    {
        IDE_TEST( allocExt( aStatistics,
                            aStartInfo,
                            aSpaceID,
                            aSegHdr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_exceed_segment_maxextents );
    {
         IDE_SET( ideSetErrorCode( smERR_ABORT_SegmentExceedMaxExtents, 
                  aSpaceID,
                  SD_MAKE_FID( aSegHandle->mSegPID ),
                  SD_MAKE_FPID( aSegHandle->mSegPID ),
                  sCurExtCnt,
                  sMaxExtCnt) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment에 Extent를 aExtCount갯수만큼 Tablespace로 부터
 *               할당한다.
 *
 * aStatistics      - [IN] 통계 정보
 * aStartInfo       - [IN] Start Info
 * aSpaceID         - [IN] SpaceID
 * aSegHandle       - [IN] Segment Handle
 * aExtCount        - [IN] 할당하고자 하는 Extent갯수
 ***********************************************************************/
IDE_RC sdpsfExtMgr::allocMutliExt( idvSQL           * aStatistics,
                                   sdrMtxStartInfo  * aStartInfo,
                                   scSpaceID          aSpaceID,
                                   sdpSegHandle     * aSegHandle,
                                   UInt               aExtCount )
{
    sdpsfSegHdr *sSegHdr;
    SInt         sState = 0;
    UInt         i;
    UInt         sMaxExtCnt;
    UInt         sCurExtCnt;

    IDE_ASSERT( aStartInfo   != NULL );
    IDE_ASSERT( aSpaceID     != 0 );
    IDE_ASSERT( aSegHandle   != NULL );
    IDE_ASSERT( aExtCount    != 0 );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               aSpaceID,
                                               aSegHandle->mSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    sMaxExtCnt = aSegHandle->mSegStoAttr.mMaxExtCnt;

    for( i = 0 ; i < aExtCount; i++ )
    {
        sCurExtCnt = sdpsfExtMgr::getExtCnt( sSegHdr );

        IDE_TEST_RAISE( sCurExtCnt + 1 > sMaxExtCnt,
                        error_exceed_segment_maxextents );

        IDE_TEST( allocExt( aStatistics,
                            aStartInfo,
                            aSpaceID,
                            sSegHdr )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdpsfSH::releaseSegHdr( aStatistics,
                                      sSegHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_exceed_segment_maxextents );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_SegmentExceedMaxExtents, 
                                  aSpaceID,
                                  SD_MAKE_FID( aSegHandle->mSegPID ),
                                  SD_MAKE_FPID( aSegHandle->mSegPID ),
                                  sCurExtCnt,
                                  sMaxExtCnt) );
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdpsfSH::releaseSegHdr( aStatistics,
                                            sSegHdr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aExtRID에 해당하는 ExtDesc를 aExtDescPtr에 복사해서
 *               준다.
 *
 * Caution:
 *  1. ExtDesc를 얻을때 ExtDesc가 위치한 페이지에 대해 Fix만 한다. 어떤
 *     Latch도 잡지 않는다. 만약 얻고자 하는 ExtDesc가 갱신될 가능성이
 *     있다면 이 함수를 사용해서는 안된다.
 *
 *
 * aStatistics      - [IN] 통계 정보
 * aSpaceID         - [IN] TableSpace ID
 * aExtRID          - [IN] ExtDesc를 얻고자 하는 ExtRID
 *
 * aExtDescPtr      - [OUT] ExtDesc를 복사해줄 영역
 ***********************************************************************/
IDE_RC sdpsfExtMgr::getExtDesc( idvSQL       * aStatistics,
                                scSpaceID      aSpaceID,
                                sdRID          aExtRID,
                                sdpsfExtDesc * aExtDescPtr )
{
    sdpsfExtDesc *sExtDescPtr;
    UChar        *sPagePtr;
    SInt          sState = 0;

    IDE_ASSERT( aSpaceID    != 0 );
    IDE_ASSERT( aExtRID     != SD_NULL_RID );
    IDE_ASSERT( aExtDescPtr != NULL );

    IDE_TEST( sdbBufferMgr::fixPageByRID( aStatistics,
                                          aSpaceID,
                                          aExtRID,
                                          (UChar **)&sExtDescPtr )
              != IDE_SUCCESS );

    IDE_ASSERT( sExtDescPtr != NULL );

    /* 복사한다. */
    *aExtDescPtr = *sExtDescPtr;

    sPagePtr = sdpPhyPage::getPageStartPtr( sExtDescPtr);

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                       sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                       sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment의 모든 Extent를 Free시킨다.
 *
 * Caution:
 *  1. 이 함수가 Return될때 TBS Header에 XLatch가 걸려있다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegHdr      - [IN] Segment Hdr
 ***********************************************************************/
IDE_RC sdpsfExtMgr::freeAllExts( idvSQL         * aStatistics,
                                 sdrMtx         * aMtx,
                                 scSpaceID        aSpaceID,
                                 sdpsfSegHdr    * aSegHdr )
{
    sdpDblPIDListBase * sExtDirPIDLst;
    sdrMtxStartInfo     sStartInfo;

    IDE_ASSERT( aMtx     != NULL );
    IDE_ASSERT( aSpaceID != 0 );
    IDE_ASSERT( aSegHdr  != NULL );

    sStartInfo.mTrans = sdrMiniTrans::getTrans( aMtx );

    if( sStartInfo.mTrans != NULL )
    {
        sStartInfo.mLogMode = SDR_MTX_LOGGING;
    }
    else
    {
        sStartInfo.mLogMode = SDR_MTX_NOLOGGING;
    }

    sExtDirPIDLst = &aSegHdr->mExtDirPIDList;

    /* Parallel Direct Path Insert시 생성된 Segment일 경우
     * Merge후에 Temp Segment는 완전히 빈 Segment가 되게 된다. */
    if( sdpDblPIDList::getNodeCnt( sExtDirPIDLst ) != 0 )
    {
        IDE_TEST( freeExtsExceptFst( aStatistics,
                                     aMtx,
                                     aSpaceID,
                                     aSegHdr )
                  != IDE_SUCCESS );

        IDU_FIT_POINT( "1.PROJ-1671@sdpsfExtMgr::freeAllExts" );

        /* Parallel DPath Insert의 Merge Step에서 Segment Hdr가 포함된 첫번째
         * Extent가 이미 DPath Insert대상 Segment에 Move되었다면 이 Segment
         * Hdr이 속한 Extent를 TBS에 반환해서는 안된다. */
        if( sdpDblPIDList::getNodeCnt( sExtDirPIDLst ) != 0 )
        {
            IDE_TEST( sdpDblPIDList::initBaseNode( sExtDirPIDLst,
                                                   aMtx )
                      != IDE_SUCCESS );

            IDE_TEST( sdpsfExtDirPage::freeLstExt( aStatistics,
                                                   aMtx,
                                                   aSpaceID,
                                                   aSegHdr,
                                                   &aSegHdr->mExtDirCntlHdr )
                      != IDE_SUCCESS );
        }

        IDU_FIT_POINT( "2.PROJ-1671@sdpsfExtMgr::freeAllExts" );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment의 첫번재 Extent를 제외한 모든 Extent를 TBS에
 *               반환한다.
 *
 * Caution:
 *  1. 이 함수가 Return될때 TBS Header에 XLatch가 걸려있다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegHdr      - [IN] Segment Header
 ***********************************************************************/
IDE_RC sdpsfExtMgr::freeExtsExceptFst( idvSQL       * aStatistics,
                                       sdrMtx       * aMtx,
                                       scSpaceID      aSpaceID,
                                       sdpsfSegHdr  * aSegHdr )
{
    sdrMtx               sFreeMtx;
    ULong                sExtPageCount;
    sdrMtxStartInfo      sStartInfo;
    sdpsfExtDirCntlHdr * sExtDirCntlHdr;
    scPageID             sCurExtDirPID;
    scPageID             sPrvExtDirPID;
    UChar              * sExtDirPagePtr;
    sdpPhyPageHdr      * sPhyHdrOfExtDirPage;
    SInt                 sState = 0;

    IDE_ASSERT( aMtx     != NULL );
    IDE_ASSERT( aSpaceID != 0 );
    IDE_ASSERT( aSegHdr  != NULL );

    sStartInfo.mTrans   = sdrMiniTrans::getTrans( aMtx );

    if( sStartInfo.mTrans != NULL )
    {
        sStartInfo.mLogMode = SDR_MTX_LOGGING;
    }
    else
    {
        sStartInfo.mLogMode = SDR_MTX_NOLOGGING;
    }

    /* Segment Header의 ExtDirPage에 있는 Extent중 첫번째를 제외한 모든 Extent
     * 를 Free한다. 첫번째는 Segment Header를 포함한 Extent이므로 가장 마지막에
     * Free하도록 한다. */
    sExtPageCount = sdpDblPIDList::getNodeCnt( &aSegHdr->mExtDirPIDList );
    sCurExtDirPID = sdpDblPIDList::getListTailNode( &aSegHdr->mExtDirPIDList );

    while( ( sExtPageCount != 0 ) && ( aSegHdr->mSegHdrPID != sCurExtDirPID ) )
    {
        sExtPageCount--;

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sFreeMtx,
                                       &sStartInfo,
                                       ID_FALSE,/*Undoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        /* Extent Dir Page의 First Extent를 제외한 모든 Extent를
         * Free한다. */
        IDE_TEST( sdpsfExtDirPage::getPage4Update( aStatistics,
                                                   &sFreeMtx,
                                                   aSpaceID,
                                                   sCurExtDirPID,
                                                   &sExtDirPagePtr,
                                                   &sExtDirCntlHdr )
                  != IDE_SUCCESS );

        sPhyHdrOfExtDirPage = sdpPhyPage::getHdr( sExtDirPagePtr );
        sPrvExtDirPID       = sdpPhyPage::getPrvPIDOfDblList( sPhyHdrOfExtDirPage );

        IDE_TEST( sdpsfExtDirPage::freeAllExtExceptFst( aStatistics,
                                                        &sStartInfo,
                                                        aSpaceID,
                                                        aSegHdr,
                                                        sExtDirCntlHdr )
                  != IDE_SUCCESS );

        /* Extent Dir Page의 마지막 남은 Fst Extent를 Free하고 Fst Extent
         * 에 속해 있는 Extent Dir Page를 리스트에서 제거한다. 이 두연산은
         * 하나의 Mini Transaction으로 묶어야 한다. 왜냐면 Extent가 Free시
         * ExtDirPage가 free되기 때문에 이 페이지가 ExtDirPage List에서
         * 제거되어야 한다. */
        IDE_TEST( sdpsfExtDirPage::freeLstExt( aStatistics,
                                               &sFreeMtx,
                                               aSpaceID,
                                               aSegHdr,
                                               sExtDirCntlHdr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpsfExtDirPageList::unlinkPage( aStatistics,
                                                   &sFreeMtx,
                                                   aSegHdr,
                                                   sExtDirCntlHdr )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sFreeMtx )
                  != IDE_SUCCESS );

        sCurExtDirPID = sPrvExtDirPID;
    }

    /* FIT/ART/sm/Projects/PROJ-1671/freelist-seg/free_extent.tc */
    IDU_FIT_POINT( "1.PROJ-1671@sdpsfExtMgr::freeExtsExceptFst" );

    /* Parallel Direct Insert시 Temp Segemnt를 Target Segment에 Merge시에
     * Temp Segment의 첫번째 ExtDirPage의 모든 Extent를 옮기고 이 페이지를
     * Link에서 제거한 상태로 서버가 종료된다면 Temp Segment의 SegHdr가
     * 속한 Extent는 Target Segment에 add된 상태이고 SegHdr는 ExtDirPage
     * List에서 제거된 상태이다. 그러므로 Segment Hdr를 제외하고 모든
     * ExtDirPage를 제거했을 때 ExtDirPage List에 남아 있는 페이지가 없다면
     * 위와 같은 상황이 발생한 것이다. 이때는 SegHdr의 Extent는 무시한다. */
    if( sCurExtDirPID != SD_NULL_PID )
    {
        IDE_ASSERT( aSegHdr->mSegHdrPID == sCurExtDirPID );

        sExtDirCntlHdr = &aSegHdr->mExtDirCntlHdr;

        IDE_TEST( sdpsfExtDirPage::freeAllExtExceptFst( aStatistics,
                                                        &sStartInfo,
                                                        aSpaceID,
                                                        aSegHdr,
                                                        sExtDirCntlHdr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sFreeMtx ) ==
                    IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 한번도 할당되지 않은 새로운 페이지를 할당한다. 때문에
 *               HWM이 이동한다. HWM은 사용되지 않은 페이지 중 최근에
 *               할당된 페이지를 가리킨다.
 *
 * Caution:
 *  1. 이 함수가 호출될때 SegHeader가 있는 페이지에 XLatch가 걸려
 *     있어야 한다.
 *
 * aStatistics    - [IN] 통계 정보
 * aAllocMtx      - [IN] Page All Mini Transaction Pointer
 * aCrtMtx        - [IN] Page Create Mini Transaction Pointer
 * aSpaceID       - [IN] TableSpace ID
 * aSegHdr        - [IN] Segment Header
 * aNextExtCnt    - [IN] Segment 확장시 할당되는 Extent갯수
 * aPageType      - [IN] Page Type
 *
 * aPageID        - [OUT] 할당된 PageID
 * aAllocPagePtr  - [OUT] 할당된 Page Pointer
 ***********************************************************************/
IDE_RC sdpsfExtMgr::allocPage( idvSQL               * aStatistics,
                               sdrMtx               * aAllocMtx,
                               sdrMtx               * aCrtMtx,
                               scSpaceID              aSpaceID,
                               sdpsfSegHdr          * aSegHdr,
                               sdpSegHandle         * aSegHandle,
                               UInt                   aNextExtCnt,
                               sdpPageType            aPageType,
                               scPageID             * aPageID,
                               UChar               ** aAllocPagePtr )
{
    scPageID            sAllocPID;
    scPageID            sFstExtPID;
    sdRID               sAllocExtRID;
    UChar              *sSegPagePtr;

    IDE_ASSERT( aAllocMtx     != NULL );
    IDE_ASSERT( aCrtMtx       != NULL );
    IDE_ASSERT( aSpaceID      != 0 );
    IDE_ASSERT( aSegHdr       != NULL );
    IDE_ASSERT( aNextExtCnt   != 0 );
    IDE_ASSERT( aPageType     < SDP_PAGE_TYPE_MAX );
    IDE_ASSERT( aPageID       != NULL );
    IDE_ASSERT( aAllocPagePtr != NULL );

    sSegPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );

    /* Ext List에서 Free Page를 할당한다. */
    IDE_TEST( allocNewPage( aStatistics,
                            aAllocMtx,
                            aSpaceID,
                            aSegHdr,
                            aSegHandle,
                            aNextExtCnt,
                            aSegHdr->mAllocExtRID,
                            aSegHdr->mFstPIDOfAllocExt,
                            aSegHdr->mHWMPID,
                            &sAllocExtRID,
                            &sFstExtPID,
                            &sAllocPID,
                            ID_FALSE )
              != IDE_SUCCESS );

    IDE_ASSERT( sAllocPID != SD_NULL_PID );

    IDE_TEST( sdrMiniTrans::setDirtyPage( aAllocMtx, sSegPagePtr )
              != IDE_SUCCESS );

    /* Alloc Page갯수를 늘려 준다. */
    IDE_TEST( sdpsfSH::setFmtPageCnt( aAllocMtx,
                                      aSegHdr,
                                      aSegHdr->mFmtPageCnt + 1 )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setHWM( aAllocMtx, aSegHdr, sAllocPID )
              != IDE_SUCCESS );

    /* Alloc Extent가 이동하였으면 갱신해준다. */
    if( sAllocExtRID != aSegHdr->mAllocExtRID )
    {
        IDE_TEST( sdpsfSH::setAllocExtRID( aAllocMtx, aSegHdr, sAllocExtRID )
                  != IDE_SUCCESS );

        IDE_TEST( sdpsfSH::setFstPIDOfAllocExt( aAllocMtx, aSegHdr, sFstExtPID )
                  != IDE_SUCCESS );
    }

    /* Alloc된 페이지에 X Latch가 이 함수가 리턴되더라도 풀리지
     * 않게 한다. */
    IDE_TEST( sdpPhyPage::create( aStatistics,
                                  aSpaceID,
                                  sAllocPID,
                                  NULL,      /* Parent Info */
                                  SDPSF_PAGE_USED_INSERTABLE,
                                  aPageType,
                                  ((sdpSegCCache*)aSegHandle->mCache)
                                      ->mTableOID,
                                  ((sdpSegCCache*)aSegHandle->mCache)
                                      ->mIndexID,
                                  aCrtMtx,   /* Create Page Mtx */
                                  aAllocMtx, /* Init Page Mtx */
                                  aAllocPagePtr )
              != IDE_SUCCESS );

    *aPageID = sAllocPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aAllocPagePtr = NULL;
    *aPageID       = SD_NULL_PID;

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
 * aStatistics             - [IN] 통계 정보
 * aMtx                    - [IN] Mini Transaction Pointer
 * aSpaceID                - [IN] TableSpace ID
 * aSegHdr                 - [IN] Segment Header
 * aNxtExtCnt              - [IN] 확장시 할당되는 Extent의 갯수
 * aPrvAllocExtRID         - [IN] 이전에 Page를 할당한 Extent RID
 * aFstPIDOfPrvExtAllocExt - [IN] 이전에 Page를 할당한 Extent의 첫번째 PID
 * aPrvAllocPageID         - [IN] 이전에 할당받은 PageID
 *
 * aAllocExtRID      - [OUT] 새로운 Page가 할당된 Extent RID
 * aFstPIDOfAllocExt - [OUT] 새로운 Page가 할당된 Extent의 첫번재 PID
 * aAllocPID         - [OUT] 새롭게 할당받은 PageID
 *
 * [BUG-21111]
 * 여러 tx의 동시 extend 중에 sement latch를 놓으면 Append mode가 아닌 경우
 * aPrvAllocExtRID,aFstPIDOfPrvExtAllocExt, aPrvAllocPageID의 내용이 변경될
 * 수 있어서 extend이후에 다시 얻어온다
 *
 ***********************************************************************/
IDE_RC sdpsfExtMgr::allocNewPage( idvSQL              * aStatistics,
                                  sdrMtx              * aMtx,
                                  scSpaceID             aSpaceID,
                                  sdpsfSegHdr         * aSegHdr,
                                  sdpSegHandle        * aSegHandle,
                                  UInt                  aNxtExtCnt,
                                  sdRID                 aPrvAllocExtRID,
                                  scPageID              aFstPIDOfPrvExtAllocExt,
                                  scPageID              aPrvAllocPageID,
                                  sdRID               * aAllocExtRID,
                                  scPageID            * aFstPIDOfAllocExt,
                                  scPageID            * aAllocPID,
                                  idBool                aIsAppendMode)
{
    idBool             sIsNeedNewExt;
    sdRID              sAllocExtRID;
    sdrMtxStartInfo    sStartInfo;
    UChar             *sSegPagePtr;
    scPageID           sFstDataPIDOfExt;
    scPageID           sFstPIDOfExt = SD_NULL_PID;
    sdRID              sPrvAllocExtRID = aPrvAllocExtRID;
    scPageID           sFstPIDOfPrvExtAllocExt = aFstPIDOfPrvExtAllocExt;
    scPageID           sPrvAllocPageID = aPrvAllocPageID;

    IDE_ASSERT( aMtx               != NULL );
    IDE_ASSERT( aSpaceID           != 0 );
    IDE_ASSERT( aSegHdr            != NULL );
    IDE_ASSERT( aNxtExtCnt         != 0 );
    IDE_ASSERT( aAllocExtRID       != NULL );
    IDE_ASSERT( aFstPIDOfAllocExt  != NULL );
    IDE_ASSERT( aAllocPID          != NULL );

    sSegPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );

    sStartInfo.mTrans = sdrMiniTrans::getTrans( aMtx );

    if( sStartInfo.mTrans != NULL )
    {
        sStartInfo.mLogMode = SDR_MTX_LOGGING;
    }
    else
    {
        sStartInfo.mLogMode = SDR_MTX_NOLOGGING;
    }

  retry :

    sIsNeedNewExt = ID_TRUE;

    /* 이전에 페이지를 할당받은 Extent에 sPrvAllocPageID 다음
     * 페이지가 존재하는 지 Check한다. */
    if( sPrvAllocExtRID != SD_NULL_RID )
    {
        IDE_ASSERT( sPrvAllocPageID != SD_NULL_PID );

        if( isFreePIDInExt( aSegHdr,
                            sFstPIDOfPrvExtAllocExt,
                            sPrvAllocPageID) == ID_TRUE )
        {
            sIsNeedNewExt = ID_FALSE;
        }
    }

    sAllocExtRID  = SD_NULL_RID;

    if( sIsNeedNewExt == ID_TRUE )
    {
        /* sPrvAllocExtRID 다음 Extent가 존재하는지 Check한다. */
        if( sPrvAllocExtRID != SD_NULL_RID )
        {
            IDE_TEST( getNxtExt4Alloc( aStatistics,
                                       aSpaceID,
                                       aSegHdr,
                                       sPrvAllocExtRID,
                                       &sAllocExtRID,
                                       &sFstPIDOfExt,
                                       &sFstDataPIDOfExt ) != IDE_SUCCESS );

            if( sFstDataPIDOfExt != SD_NULL_PID )
            {
                IDE_ASSERT( sFstDataPIDOfExt != SD_MAKE_PID( sAllocExtRID ) );
            }
            else
            {
                IDE_ASSERT( sAllocExtRID == SD_NULL_RID );
            }
        }

        if( sAllocExtRID == SD_NULL_RID )
        {
            /* 새로운 Extent를 TBS로부터 할당받는다. */
            IDE_TEST( extend( aStatistics,
                              &sStartInfo,
                              aSpaceID,
                              aSegHdr,
                              aSegHandle,
                              aNxtExtCnt ) != IDE_SUCCESS );

            // BUG-21111
            // 동시에 2 tx 이상이 extend를 수행할 때
            // nextextents가 2 이상이면 중간에 segment x latch를 잠깐
            // 놓는 순간 순서가 역전되어 HWM 이동시 몇개의 extent를
            // 건너뛸 수 있음
            // -->자신이 할당한 첫 extent가 HWM 바로 다음 extent라고
            // 장담할 수 없음
            // D-Path Insert는 segment를 공유하지 않으므로 상관없다
            if( aIsAppendMode == ID_FALSE )
            {
                sPrvAllocExtRID = aSegHdr->mAllocExtRID;
                sFstPIDOfPrvExtAllocExt = aSegHdr->mFstPIDOfAllocExt;
                sPrvAllocPageID = aSegHdr->mHWMPID;
            }
            goto retry;
        }

        IDE_ASSERT( sFstPIDOfExt != SD_NULL_PID );

        *aAllocPID         = sFstDataPIDOfExt;
        *aFstPIDOfAllocExt = sFstPIDOfExt;
        *aAllocExtRID      = sAllocExtRID;
    }
    else
    {
        /* 한 Extent내의 페이지는 연속되어 있으므로 새로운
         * PageID는 이전 Alloc했던 페이지에 1더한 값이 된다. */
        *aAllocPID         = sPrvAllocPageID + 1;
        *aFstPIDOfAllocExt = sFstPIDOfPrvExtAllocExt;
        *aAllocExtRID      = sPrvAllocExtRID;

        IDE_ASSERT( *aAllocPID != SD_MAKE_PID( sPrvAllocExtRID ) );
    }

    IDE_TEST( sdrMiniTrans::setDirtyPage( aMtx, sSegPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aCurExtRID가 가리키는 Extent에 aPrvAllocPageID이후
 *               Page가 존재하는 하는지 체크해서 없으면 새로운 다음
 *               Extent로 이동하고 다음 Extent에서 Free Page를 찾아서
 *               Page가 할당된 ExtRID와 PageID를 넘겨준다.
 *
 *               다음 Extent가 없으면 aNxtExtRID와 aFstDataPIDOfNxtExt
 *               에 SD_NULL_RID, SD_NULL_PID를 넘긴다.
 *
 * Caution:
 *  1. 이 함수가 호출될때 SegHdr가 있는 페이지에 XLatch가 걸려 있어야 한다.
 *
 * aStatistics      - [IN] 통계 정보
 * aSpaceID         - [IN] TableSpace ID
 * aSegHdr          - [IN] Segment Header
 * aCurExtRID       - [IN] 현재 Extent RID
 *
 * aNxtExtRID          - [OUT] 다음 Extent RID
 * aFstPIDOfExt        - [OUT] 다음 Extent의 첫번째 PageID
 * aFstDataPIDOfNxtExt - [OUT] 다음 Extent 의 첫번째 Data Page ID, Extent의
 *                             첫번째 페이지가 Extent Dir Page로 사용되기도
 *                             한다.
 ***********************************************************************/
IDE_RC sdpsfExtMgr::getNxtExt4Alloc( idvSQL       * aStatistics,
                                     scSpaceID      aSpaceID,
                                     sdpsfSegHdr  * aSegHdr,
                                     sdRID          aCurExtRID,
                                     sdRID        * aNxtExtRID,
                                     scPageID     * aFstPIDOfExt,
                                     scPageID     * aFstDataPIDOfNxtExt )
{
    sdpsfExtDesc          sExtDesc;
    scPageID              sExtDirPID;
    scPageID              sNxtExtDirPID;
    scPageID              sNxtNxtExtPID;
    sdRID                 sNxtExtRID = SD_NULL_RID;
    sdRID                 sLstExtRID;
    UChar               * sExtDirPage;
    sdpsfExtDirCntlHdr  * sExtDirCntlHdr;
    sdpPhyPageHdr       * sExtPagePhyHdr;
    UShort                sExtCntInEDP;
    SInt                  sState = 0;

    IDE_ASSERT( aSpaceID            != 0 );
    IDE_ASSERT( aSegHdr             != NULL );
    IDE_ASSERT( aCurExtRID          != SD_NULL_RID );
    IDE_ASSERT( aNxtExtRID          != NULL );
    IDE_ASSERT( aFstPIDOfExt        != NULL );
    IDE_ASSERT( aFstDataPIDOfNxtExt != NULL );

    *aNxtExtRID          = SD_NULL_RID;
    *aFstDataPIDOfNxtExt = SD_NULL_PID;
    *aFstPIDOfExt        = SD_NULL_PID;

    sExtDirPID = SD_MAKE_PID( aCurExtRID );

    if( aSegHdr->mSegHdrPID != sExtDirPID )
    {
        IDE_TEST( sdpsfExtDirPage::fixPage( aStatistics,
                                            aSpaceID,
                                            sExtDirPID,
                                            &sExtDirPage,
                                            &sExtDirCntlHdr )
                  != IDE_SUCCESS );
        sState = 1;
    }
    else
    {
        sExtDirCntlHdr = &aSegHdr->mExtDirCntlHdr;
        sExtDirPage    = sdpPhyPage::getPageStartPtr( aSegHdr );
    }

    /* aCurExtRID 다음 Extent가 같은 ExtDirPage내에 존재하는
     * 지 검사 */
    IDE_TEST( sdpsfExtDirPage::getNxtExt( sExtDirCntlHdr,
                                          aCurExtRID,
                                          &sNxtExtRID,
                                          &sExtDesc )
              != IDE_SUCCESS );

    if( sNxtExtRID == SD_NULL_RID )
    {
        /* 같은 ExtentDirPage에 다음 Extent가 존재하지 않는다면
         * 다음 ExtentDirPage에 Extent가 존재하는 지 검사 */
        sExtPagePhyHdr = sdpPhyPage::getHdr( sExtDirPage );
        sNxtExtDirPID  = sdpPhyPage::getNxtPIDOfDblList( sExtPagePhyHdr );

        if( sState == 1 )
        {
            sState = 0;
            IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                               sExtDirPage )
                      != IDE_SUCCESS );
        }

        if( sNxtExtDirPID != aSegHdr->mSegHdrPID )
        {
            /* aCurExtRID가 속한 ExtDirPage의 Next ExtDirPage가 존재
             * 한다면 */
            IDE_TEST( sdpsfExtDirPage::getPageInfo( aStatistics,
                                                    aSpaceID,
                                                    sNxtExtDirPID,
                                                    &sExtCntInEDP,
                                                    &sNxtExtRID,
                                                    &sExtDesc,
                                                    &sLstExtRID,
                                                    &sNxtNxtExtPID )
                      != IDE_SUCCESS );
        }
    }

    if( sNxtExtRID != SD_NULL_RID )
    {
        *aNxtExtRID          = sNxtExtRID;
        *aFstPIDOfExt        = sdpsfExtent::getFstPID( &sExtDesc );
        *aFstDataPIDOfNxtExt = sdpsfExtent::getFstDataPID( &sExtDesc );
    }

    if( sState == 1 )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                           sExtDirPage )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sExtDirPage )
                    == IDE_SUCCESS );
    }

    *aNxtExtRID          = SD_NULL_RID;
    *aFstPIDOfExt        = SD_NULL_PID;
    *aFstDataPIDOfNxtExt = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aCurExtRID가 가리키는 Extent의 다음 Extent의 RID를 구한다.
 *
 * aStatistics         - [IN]  통계 정보
 * aSpaceID            - [IN]  TableSpace ID
* aSegHdrPID           - [IN]  SegHdr PID
 * aCurExtRID          - [IN]  현재 Extent RID
 * aNxtExtRID          - [OUT] 다음 Extent RID
 * aFstPIDOfNxtExt     - [OUT] 다음 Extent 첫번째 PageID
 * aFstDataPIDOfNxtExt - [OUT] 다음 Extent의 첫번재 Data PID
 ***********************************************************************/
IDE_RC sdpsfExtMgr::getNxtExt4Scan( idvSQL       * aStatistics,
                                    scSpaceID      aSpaceID,
                                    scPageID       aSegHdrPID,
                                    sdRID          aCurExtRID,
                                    sdRID        * aNxtExtRID,
                                    scPageID     * aFstPIDOfNxtExt,
                                    scPageID     * aFstDataPIDOfNxtExt )
{
    sdpsfExtDesc          sExtDesc;
    UChar               * sExtDirPage;
    sdRID                 sNxtExtRID;
    sdRID                 sLstExtRID;
    scPageID              sNxtNxtExtPID;
    UShort                sExtCntInEDP;
    scPageID              sNxtExtDirPID;
    sdpsfExtDirCntlHdr  * sExtDirCntlHdr;
    sdpPageType           sPageType;
    sdpPhyPageHdr       * sExtPhyPageHdr;
    sdpsfSegHdr         * sSegHdr;
    SInt                  sState = 0;

    IDE_ASSERT( aSpaceID            != 0 );
    IDE_ASSERT( aSegHdrPID          != SD_NULL_PID );
    IDE_ASSERT( aCurExtRID          != SD_NULL_RID );
    IDE_ASSERT( aNxtExtRID          != NULL );
    IDE_ASSERT( aFstPIDOfNxtExt     != NULL );
    IDE_ASSERT( aFstDataPIDOfNxtExt != NULL );

    *aNxtExtRID          = SD_NULL_RID;
    *aFstPIDOfNxtExt     = SD_NULL_PID;
    *aFstDataPIDOfNxtExt = SD_NULL_PID;

    IDE_TEST( sdpsfExtDirPage::fixPage( aStatistics,
                                        aSpaceID,
                                        SD_MAKE_PID( aCurExtRID ),
                                        &sExtDirPage,
                                        &sExtDirCntlHdr )
              != IDE_SUCCESS );
    sState = 1;

    sExtPhyPageHdr = sdpPhyPage::getHdr( sExtDirPage );
    sPageType      = sdpPhyPage::getPageType( sExtPhyPageHdr );

    if( sPageType == SDP_PAGE_FMS_SEGHDR )
    {
        sSegHdr        = sdpsfSH::getSegHdrFromPagePtr( sExtDirPage );
        sExtDirCntlHdr = &sSegHdr->mExtDirCntlHdr;
    }

    IDE_TEST( sdpsfExtDirPage::getNxtExt( sExtDirCntlHdr,
                                          aCurExtRID,
                                          &sNxtExtRID,
                                          &sExtDesc )
              != IDE_SUCCESS );

    if( sNxtExtRID == SD_NULL_RID )
    {
        sExtPhyPageHdr = sdpPhyPage::getHdr( sExtDirPage );
        sNxtExtDirPID  = sdpPhyPage::getNxtPIDOfDblList( sExtPhyPageHdr );

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                           sExtDirPage )
                  != IDE_SUCCESS );

        if( sNxtExtDirPID != aSegHdrPID )
        {
            IDE_TEST( sdpsfExtDirPage::getPageInfo( aStatistics,
                                                    aSpaceID,
                                                    sNxtExtDirPID,
                                                    &sExtCntInEDP,
                                                    &sNxtExtRID,
                                                    &sExtDesc,
                                                    &sLstExtRID,
                                                    &sNxtNxtExtPID )
                      != IDE_SUCCESS );
        }
    }

    if( sNxtExtRID != SD_NULL_RID )
    {
        *aNxtExtRID          = sNxtExtRID;
        *aFstPIDOfNxtExt     = sdpsfExtent::getFstPID( &sExtDesc );
        *aFstDataPIDOfNxtExt = sdpsfExtent::getFstDataPID( &sExtDesc );
    }

    if( sState == 1 )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                           sExtDirPage )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sExtDirPage )
                    == IDE_SUCCESS );
    }

    *aNxtExtRID          = SD_NULL_RID;
    *aFstPIDOfNxtExt     = SD_NULL_PID;
    *aFstDataPIDOfNxtExt = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aCurExtRID가 가리키는 Extent의 다음 Extent의 RID를 구한다.
 *
 * aStatistics         - [IN]  통계 정보
 * aSpaceID            - [IN]  TableSpace ID
 * aSegHdrPID          - [IN]  SegHdr PID
 * aCurExtRID          - [IN]  현재 Extent RID
 *
 * aNxtExtRID          - [OUT] 다음 Extent RID
 ***********************************************************************/
IDE_RC sdpsfExtMgr::getNxtExtRID( idvSQL       * aStatistics,
                                  scSpaceID      aSpaceID,
                                  scPageID       aSegHdrPID,
                                  sdRID          aCurExtRID,
                                  sdRID        * aNxtExtRID)
{
    scPageID     sFstPIDOfNxtExt;
    scPageID     sFstDataPIDOfNxtExt;

    IDE_ASSERT( aSpaceID    != 0 );
    IDE_ASSERT( aSegHdrPID  != SD_NULL_PID );
    IDE_ASSERT( aCurExtRID  != SD_NULL_RID );
    IDE_ASSERT( aNxtExtRID  != NULL );

    IDE_TEST( getNxtExt4Scan( aStatistics,
                              aSpaceID,
                              aSegHdrPID,
                              aCurExtRID,
                              aNxtExtRID,
                              &sFstPIDOfNxtExt,
                              &sFstDataPIDOfNxtExt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Extent List의 <Extent내 페이지갯수>, <첫번째 ExtRID>,
 *               <마지막 ExtRID>를 얻는다.
 *
 * aStatistics      - [IN] 통계 정보
 * aSpaceID         - [IN] TableSpace ID
 * aSegRID          - [IN] Segment Extent List
 *
 * aPageCntInExt    - [OUT] Extent내 페이지 갯수
 * aFstExtRID       - [OUT] 첫번째 Ext RID
 * aLstExtRID       - [OUT] 마지막 Ext RID
 ***********************************************************************/
IDE_RC sdpsfExtMgr::getExtListInfo( idvSQL    * aStatistics,
                                    scSpaceID   aSpaceID,
                                    scPageID    aSegPID,
                                    UInt      * aPageCntInExt,
                                    sdRID     * aFstExtRID,
                                    sdRID     * aLstExtRID )
{
    sdpsfSegHdr   * sSegHdr;
    UChar         * sSegPagePtr = NULL;
    SInt            sState      = 0;

    IDE_ASSERT( aSpaceID       != 0 );
    IDE_ASSERT( aSegPID        != SD_NULL_PID );
    IDE_ASSERT( aPageCntInExt  != NULL );
    IDE_ASSERT( aFstExtRID     != NULL );
    IDE_ASSERT( aLstExtRID     != NULL );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Read( aStatistics,
                                             aSpaceID,
                                             aSegPID,
                                             &sSegHdr )
              != IDE_SUCCESS );
    sSegPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)sSegHdr );
    sState = 1;

    *aPageCntInExt = sSegHdr->mPageCntInExt;
    *aFstExtRID    = sdpsfExtDirPage::getFstExtRID( &sSegHdr->mExtDirCntlHdr );
    *aLstExtRID    = sSegHdr->mAllocExtRID;

    sState = 0;
    IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                           sSegPagePtr )
                == IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                               sSegPagePtr )
                    == IDE_SUCCESS );
    }

    *aPageCntInExt = 0;
    *aFstExtRID    = SD_NULL_RID;
    *aLstExtRID    = SD_NULL_RID;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : *aPageID 다음 Alloc 페이지를 구한다. Segment Extent List
 *                를 따라가면서 다음 페이지를 구하는 것이기 때문에 현재
 *                Extent에서 페이가 더이상 없으면 다음 Extent로 이동한다.
 *                만약 현재 Extent가 Segment의 mLstAllocRID와 같거나
 *                다음 PageID가 Segment의 mHWM과 같다면 그 이후 페이지들은
 *                한번도 Alloc되지 않은 영역이기때문에 aExtRID, aPageID를
 *                SD_NULL_RID, SD_NULL_PID로 설정한다.
 *
 * aStatistics      - [IN] 통계 정보
 * aSpaceID         - [IN] TableSpace ID
 * aSegInfo         - [IN] Segment Info
 * aSegCacheInfo    - [IN] Segment Cache Info
 *
 * aExtRID          - [INOUT] Extent RID
 * aExtInfo         - [INOUT] aExtRID가 가리키는 Extent Info
 * aPageID          - [INOUT] IN: 이전에 할당한 페이지 ID, OUT: 새로 할당
 *                            된 페이지 ID
 ***********************************************************************/
IDE_RC sdpsfExtMgr::getNxtAllocPage( idvSQL             * aStatistics,
                                     scSpaceID            aSpaceID,
                                     sdpSegInfo         * aSegInfo,
                                     sdpSegCacheInfo    * /*aSegCacheInfo*/,
                                     sdRID              * aExtRID,
                                     sdpExtInfo         * aExtInfo,
                                     scPageID           * aPageID )
{
    sdRID    sNxtExtRID;
    sdRID    sCurExtRID;
    scPageID sFstPIDOfNxtExt;
    scPageID sFstDataPIDOfNxtExt;
    scPageID sNxtPID;
    scPageID sCurPID;

    IDE_ASSERT(  aSpaceID != 0 );
    IDE_ASSERT(  aSegInfo != NULL );
    IDE_ASSERT(  aExtRID  != NULL );
    IDE_ASSERT(  aExtInfo != NULL );
    IDE_ASSERT(  aPageID  != NULL );
    IDE_ASSERT( *aExtRID  != SD_NULL_RID );

    sCurPID    = *aPageID;
    sCurExtRID = *aExtRID;
    sNxtExtRID =  sCurExtRID;

    if( sCurPID == SD_NULL_PID )
    {
        sNxtPID = aExtInfo->mFstPID;
    }
    else
    {
        IDE_TEST_CONT( sCurPID == aSegInfo->mHWMPID,
                        cont_no_more_page );

        sNxtPID = sCurPID + 1;

        if( sdpsfExtent::isPIDInExt( aExtInfo,
                                     aSegInfo->mPageCntInExt,
                                     sNxtPID ) == ID_FALSE )
        {
            IDE_TEST_CONT( aSegInfo->mLstAllocExtRID == sCurExtRID,
                            cont_no_more_page );

            IDE_TEST( getNxtExt4Scan( aStatistics,
                                      aSpaceID,
                                      aSegInfo->mSegHdrPID,
                                      sCurExtRID,
                                      &sNxtExtRID,
                                      &sFstPIDOfNxtExt,
                                      &sFstDataPIDOfNxtExt ) != IDE_SUCCESS );

            IDE_ASSERT( sNxtExtRID != SD_NULL_RID );
            IDE_ASSERT( sFstDataPIDOfNxtExt != SD_NULL_PID );

            aExtInfo->mFstPID     = sFstPIDOfNxtExt;
            aExtInfo->mFstDataPID = sFstDataPIDOfNxtExt;

            sNxtPID = sFstPIDOfNxtExt;
        }
    }

    if( sNxtPID == aSegInfo->mHWMPID )
    {
        /* 마지막 Page가 Meta이면 더 이상 읽을 페이지가 존재하지 않는다. */
        IDE_TEST_CONT( sNxtPID < aExtInfo->mFstDataPID, cont_no_more_page );
    }

    if( sNxtPID == aExtInfo->mFstPID )
    {
        /* Segment의 Meta Page는 Skip한다 .*/
        sNxtPID = aExtInfo->mFstDataPID;
    }

    *aExtRID = sNxtExtRID;
    *aPageID = sNxtPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( cont_no_more_page );

    *aExtRID = SD_NULL_RID;
    *aPageID = SD_NULL_PID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExtRID = SD_NULL_RID;
    *aPageID = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aExtRID가 가리키는 Extent의 정보를 구한다.
 *
 * aStatistics   - [IN] 통계 정보
 * aSpaceID      - [IN] TableSpace ID
 * aExtRID       - [IN] Extent RID
 *
 * aExtInfo      - [OUT] aExtRID가 가리키는 ExtInfo
 ***********************************************************************/
IDE_RC sdpsfExtMgr::getExtInfo( idvSQL       *aStatistics,
                                scSpaceID     aSpaceID,
                                sdRID         aExtRID,
                                sdpExtInfo   *aExtInfo )
{
    sdpsfExtDesc sExtDesc;

    IDE_ASSERT( aSpaceID != 0 );
    IDE_ASSERT( aExtRID  != SD_NULL_RID );
    IDE_ASSERT( aExtInfo != NULL );

    IDE_TEST( getExtDesc( aStatistics,
                          aSpaceID,
                          aExtRID,
                          &sExtDesc )
              != IDE_SUCCESS );

    aExtInfo->mFstPID = sExtDesc.mFstPID;

    if( SDP_SF_IS_FST_EXTDIRPAGE_AT_EXT( sExtDesc.mFlag ) )
    {
        aExtInfo->mFstDataPID = sExtDesc.mFstPID + 1;
    }
    else
    {
        aExtInfo->mFstDataPID = sExtDesc.mFstPID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment에서 aExtRID 다음의 모든 Extent를 TBS에 반환한다.
 *
 * aStatistics   - [IN] 통계 정보
 * aMtx          - [IN] Mini Transaction Pointer
 * aSpaceID      - [IN] TableSpace ID
 * aSegHdr       - [IN] Segment Header
 * aExtRID       - [IN] Extent RID
 ***********************************************************************/
IDE_RC sdpsfExtMgr::freeAllNxtExt( idvSQL       * aStatistics,
                                   sdrMtx       * aMtx,
                                   scSpaceID      aSpaceID,
                                   sdpsfSegHdr  * aSegHdr,
                                   sdRID          aExtRID )
{
    sdrMtx               sFreeMtx;
    ULong                sExtPageCount;
    sdrMtxStartInfo      sStartInfo;
    UChar              * sExtDirPagePtr;
    sdpsfExtDirCntlHdr * sExtDirCntlHdr;
    scPageID             sCurExtDirPID;
    scPageID             sPrvExtDirPID;
    scPageID             sFstFreeExtDirPID;
    sdpPhyPageHdr      * sPhyHdrOfExtDirPage;
    SInt                 sState = 0;

    IDE_ASSERT( aMtx     != NULL );
    IDE_ASSERT( aSpaceID != 0 );
    IDE_ASSERT( aSegHdr  != NULL );
    IDE_ASSERT( aExtRID  != SD_NULL_RID );

    sStartInfo.mTrans   = sdrMiniTrans::getTrans( aMtx );

    if( sStartInfo.mTrans != NULL )
    {
        sStartInfo.mLogMode = SDR_MTX_LOGGING;
    }
    else
    {
        sStartInfo.mLogMode = SDR_MTX_NOLOGGING;
    }

    /* Segment Header의 ExtDirPage에 있는 Extent중 첫번째를 제외한 모든 Extent
     * 를 Free한다. 첫번째는 Segment Header를 포함한 Extent이므로 가장 마지막에
     * Free하도록 한다. */
    sExtDirCntlHdr    = &aSegHdr->mExtDirCntlHdr;
    sExtPageCount     = sdpDblPIDList::getNodeCnt( &aSegHdr->mExtDirPIDList );
    sCurExtDirPID     = sdpDblPIDList::getListTailNode( &aSegHdr->mExtDirPIDList );
    sFstFreeExtDirPID = SD_MAKE_PID( aExtRID );

    while( sExtPageCount > 0 )
    {
        sExtPageCount--;

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sFreeMtx,
                                       &sStartInfo,
                                       ID_FALSE,/*Undoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        if( sCurExtDirPID != aSegHdr->mSegHdrPID )
        {
            IDE_TEST( sdpsfExtDirPage::getPage4Update( aStatistics,
                                                       &sFreeMtx,
                                                       aSpaceID,
                                                       sCurExtDirPID,
                                                       &sExtDirPagePtr,
                                                       &sExtDirCntlHdr )
                  != IDE_SUCCESS );
        }
        else
        {
            sExtDirPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );
            sExtDirCntlHdr = &aSegHdr->mExtDirCntlHdr;

            IDE_TEST( sdrMiniTrans::setDirtyPage( &sFreeMtx, sExtDirPagePtr )
                      != IDE_SUCCESS );
        }

        sPhyHdrOfExtDirPage = sdpPhyPage::getHdr( sExtDirPagePtr );
        sPrvExtDirPID       = sdpPhyPage::getPrvPIDOfDblList( sPhyHdrOfExtDirPage );

        if( sFstFreeExtDirPID != sCurExtDirPID )
        {
            IDE_TEST( sdpsfExtDirPage::freeAllExtExceptFst( aStatistics,
                                                            &sStartInfo,
                                                            aSpaceID,
                                                            aSegHdr,
                                                            sExtDirCntlHdr )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( sdpsfExtDirPage::freeAllNxtExt( aStatistics,
                                                      &sStartInfo,
                                                      aSpaceID,
                                                      aSegHdr,
                                                      sExtDirCntlHdr,
                                                      aExtRID )
                      != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit( &sFreeMtx )
                      != IDE_SUCCESS );

            break;
        }

        /* Extent Dir Page의 마지막 남은 Fst Extent를 Free하고 Fst Extent
         * 에 속해 있는 Extent Dir Page를 리스트에서 제거한다. 이 두연산은
         * 하나의 Mini Transaction으로 묶어야 한다. 왜냐면 Extent가 Free시
         * ExtDirPage가 free되기 때문에 이 페이지가 ExtDirPage List에서
         * 제거되어야 한다. */
        IDE_TEST( sdpsfExtDirPage::freeLstExt( aStatistics,
                                               &sFreeMtx,
                                               aSpaceID,
                                               aSegHdr,
                                               sExtDirCntlHdr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpsfExtDirPageList::unlinkPage( aStatistics,
                                                   &sFreeMtx,
                                                   aSegHdr,
                                                   sExtDirCntlHdr )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sFreeMtx )
                  != IDE_SUCCESS );

        sCurExtDirPID = sPrvExtDirPID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sFreeMtx ) ==
                    IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


