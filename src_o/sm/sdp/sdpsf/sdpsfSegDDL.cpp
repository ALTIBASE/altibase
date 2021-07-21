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
 * $Id: sdpsfSegDDL.cpp 27226 2008-07-23 17:36:35Z newdaily $
 *
 * 본 파일은 Segment의 Create/Drop/Alter/Reset 연산의
 * STATIC 인터페이스들을 관리한다.
 *
 **********************************************************************/

# include <sdrMiniTrans.h>

# include <sdb.h>
# include <sdbBufferMgr.h>

# include <sdpDef.h>
# include <sdpReq.h>

# include <sdpsfExtMgr.h>
# include <sdpsfSH.h>
# include <sdpsfPvtFreePIDList.h>
# include <sdpSegDescMgr.h>
# include <sdpSglRIDList.h>
# include <sdpsfSegDDL.h>
# include <sdptbExtent.h>

/***********************************************************************
 * Description : Segment를 생성한다.
 *
 * aStatistics      - [IN] 통계 정보
 * aMtx             - [IN] Mini Transaction Pointer
 * aSpaceID         - [IN] SpaceID
 * aSegType         - [IN] Segment Type
 *
 * aSegHandle       - [OUT] Segment Handle에 Seg PID와 Segment Header Page
 *                          에서 Meta가 위치하는 Offset이 설정된다.
 ***********************************************************************/
IDE_RC sdpsfSegDDL::createSegment( idvSQL           * aStatistics,
                                   sdrMtx           * aMtx,
                                   scSpaceID          aSpaceID,
                                   sdpSegType         aSegType,
                                   sdpSegHandle     * aSegHandle )
{
    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSpaceID   != 0 );
    IDE_ASSERT( aSegType   <  SDP_SEG_TYPE_MAX );
    IDE_ASSERT( aSegHandle != NULL );

    IDE_TEST( allocateSegment( aStatistics,
                               aMtx,
                               aSpaceID,
                               aSegType,
                               aSegHandle ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment를 Drop한다.
 *
 * aStatistics    - [IN] 통계 정보
 * aMtx           - [IN] Mini Transaction Pointer
 * aSpaceID       - [IN] SpaceID
 * aSegPID        - [IN] Segment PID
 ***********************************************************************/
IDE_RC sdpsfSegDDL::dropSegment( idvSQL           * aStatistics,
                                 sdrMtx           * aMtx,
                                 scSpaceID          aSpaceID,
                                 scPageID           aSegPID )
{
    sdpsfSegHdr  * sSegHdrPtr;

    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSpaceID   != 0 );
    IDE_ASSERT( aSegPID    != SD_NULL_PID );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update(
                  aStatistics,
                  aMtx,
                  aSpaceID,
                  aSegPID,
                  &sSegHdrPtr )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfExtMgr::freeAllExts( aStatistics,
                                        aMtx,
                                        aSpaceID,
                                        sSegHdrPtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment를 Create당시의 초기상태로 만든다.
 *
 * aStatistics      - [IN] 통계 정보
 * aMtx             - [IN] Mini Transaction Pointer
 * aSpaceID         - [IN] SpaceID
 * aSegHandle       - [IN] Segment Handle
 * aSegType         - [IN] Segment Type
 ***********************************************************************/
IDE_RC sdpsfSegDDL::resetSegment( idvSQL           * aStatistics,
                                  sdrMtx           * aMtx,
                                  scSpaceID          aSpaceID,
                                  sdpSegHandle     * aSegHandle,
                                  sdpSegType         aSegType )
{
    sdpsfSegHdr *sSegHdrPtr;

    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSpaceID   != 0 );
    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aSegType   <  SDP_SEG_TYPE_MAX );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update(
                  aStatistics,
                  aMtx,
                  aSpaceID,
                  aSegHandle->mSegPID,
                  &sSegHdrPtr )
              != IDE_SUCCESS );

    /* 첫번째 Extent만 제외하고 모든 Extent를 Free한다. */
    IDE_TEST( sdpsfExtMgr::freeExtsExceptFst(
                  aStatistics,
                  aMtx,
                  aSpaceID,
                  sSegHdrPtr )
              != IDE_SUCCESS );

    /* Segment Desc를 초기화 한다. */
    IDE_TEST( sdpsfSH::init( aStatistics,
                             aMtx,
                             aSpaceID,
                             sSegHdrPtr,
                             sSegHdrPtr->mSegHdrPID,
                             sSegHdrPtr->mSegHdrPID,
                             sSegHdrPtr->mPageCntInExt,
                             aSegType )
              != IDE_SUCCESS );

    IDE_TEST( sdpSegment::initCommonCache( 
                  (sdpSegCCache*)aSegHandle->mCache,
                  aSegType,
                  sSegHdrPtr->mPageCntInExt,
                  ( (sdpsfSegCache*)aSegHandle->mCache )->mCommon.mTableOID,
                  SM_NULL_INDEX_ID)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment의 aExtRID이후 Extent를 데이타 베이스에 반환한다
 *
 * aStatistics      - [IN] 통계 정보
 * aMtx             - [IN] Mini Transaction Pointer
 * aSpaceID         - [IN] SpaceID
 * aSegHdr          - [IN] Segment Hdr
 * aExtRID          - [IN] Extent RID
 ***********************************************************************/
IDE_RC sdpsfSegDDL::trim( idvSQL           *aStatistics,
                          sdrMtx           *aMtx,
                          scSpaceID         aSpaceID,
                          sdpsfSegHdr      *aSegHdr,
                          sdRID             aExtRID )
{
    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSpaceID   != 0 );
    IDE_ASSERT( aSegHdr    != NULL );
    IDE_ASSERT( aExtRID    != SD_NULL_RID );

    IDE_TEST( sdpsfExtMgr::freeAllNxtExt( aStatistics,
                                          aMtx,
                                          aSpaceID,
                                          aSegHdr,
                                          aExtRID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment 할당 및 Segment Meta Header 초기화
 *
 * aStatistics      - [IN]     통계 정보
 * aMtx             - [IN]     Mini Transaction Pointer
 * aSpaceID         - [IN]     SpaceID
 * aSegType         - [IN/OUT] Segment Type
 * aSegHandle       - [OUT]    Segment Handle
 ***********************************************************************/
IDE_RC sdpsfSegDDL::allocateSegment( idvSQL        * aStatistics,
                                     sdrMtx        * aMtx,
                                     scSpaceID       aSpaceID,
                                     sdpSegType      aSegType,
                                     sdpSegHandle  * aSegHandle )
{
    sdrMtxStartInfo      sStartInfo;
    scPageID             sFstPID;
    UChar              * sFstPagePtr;
    sdpsfSegHdr        * sSegHdrPtr;
    UInt                 sPageCntINExt;
    sdrMtx               sCrtMtx;
    smiSegStorageAttr  * sSegStoAttr;
    sdpExtDesc           sExtDesc;
    SInt                 sState = 0;

    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSpaceID   != 0 );
    IDE_ASSERT( aSegType   <  SDP_SEG_TYPE_MAX );
    IDE_ASSERT( aSegHandle != NULL );

    sStartInfo.mTrans   = sdrMiniTrans::getTrans( aMtx );

    if( sStartInfo.mTrans != NULL )
    {
        sStartInfo.mLogMode = SDR_MTX_LOGGING;
    }
    else
    {
        sStartInfo.mLogMode = SDR_MTX_NOLOGGING;
    }

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sCrtMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdptbExtent::allocExts( aStatistics,
                                      &sStartInfo,
                                      aSpaceID,
                                      1, /* alloc extent count */
                                      &sExtDesc ) 
              != IDE_SUCCESS );

    sFstPID       = sExtDesc.mExtFstPID;
    sPageCntINExt = sExtDesc.mLength;

    /* SegHdr에 대한 Latch를 풀지 않기 위해서 인자로 넘겨오는
     * Mini Trans로 Buffer Create를 한다. */
    IDE_TEST( sdpPhyPage::create( aStatistics,
                                  aSpaceID,
                                  sFstPID,
                                  NULL,        /* Parent Info */
                                  0,           /* Page Insertable State */
                                  SDP_PAGE_FMS_SEGHDR,
                                  ( (sdpSegCCache*) aSegHandle->mCache )
                                    ->mTableOID,
                                  ( (sdpSegCCache*) aSegHandle->mCache )
                                    ->mIndexID,
                                  &sCrtMtx,    /* Create Page Mtx */
                                  &sCrtMtx,    /* Init Page Mtx */
                                  &sFstPagePtr )
              != IDE_SUCCESS );

    sSegHdrPtr = sdpsfSH::getSegHdrFromPagePtr( sFstPagePtr );

    IDE_TEST( sdpsfSH::init( aStatistics,
                             &sCrtMtx,
                             aSpaceID,
                             sSegHdrPtr,
                             sFstPID,
                             sFstPID,
                             sPageCntINExt,
                             aSegType )
              != IDE_SUCCESS );

    /* 생성된 SegHdr의 PID를 넘겨준다. */
    aSegHandle->mSegPID  =
        sdpPhyPage::getPageIDFromPtr( sSegHdrPtr );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sCrtMtx ) != IDE_SUCCESS );

    sSegStoAttr = sdpSegDescMgr::getSegStoAttr( aSegHandle );

    if( sSegStoAttr->mInitExtCnt > 1 )
    {
        IDE_TEST( sdpsfExtMgr::allocMutliExt( aStatistics,
                                              &sStartInfo,
                                              aSpaceID,
                                              aSegHandle,
                                              sSegStoAttr->mInitExtCnt - 1 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aSegHandle->mSegPID  = SD_NULL_PID;

    if( sState != 0 )
    {
        sState = 0;
        IDE_ASSERT( sdrMiniTrans::rollback( &sCrtMtx )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


