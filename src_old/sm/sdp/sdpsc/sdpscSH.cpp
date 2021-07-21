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
 * $Id: sdpscSH.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment의 Segment Header 관련
 * STATIC 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <smErrorCode.h>
# include <sdr.h>

# include <sdpReq.h>
# include <sdpPhyPage.h>

# include <sdpscDef.h>
# include <sdpscSH.h>
# include <sdpscCache.h>
# include <sdpscAllocPage.h>
# include <sdpscSegDDL.h>

/***********************************************************************
 *
 * Description : Segment Header를 초기화 한다.
 *
 * aSegHdr            - [IN] 세그먼트 메타 헤더 포인터
 * aSegPID            - [IN] 세그먼트 헤더 페이지의 PID
 * aSegType           - [IN] 세그먼트 타입
 * aPageCntInExt      - [IN] 하나의 Extent Desc.의 페이지 개수
 * aMaxExtCntInExtDir - [IN] 세그먼트 헤더 페이지의 Extent Map에 기록할 최대
 *                           Extent Desc. 개수
 *
 ***********************************************************************/
void sdpscSegHdr::initSegHdr( sdpscSegMetaHdr   * aSegHdr,
                              scPageID            aSegHdrPID,
                              sdpSegType          aSegType,
                              UInt                aPageCntInExt,
                              UShort              aMaxExtCntInExtDir )
{
    scOffset   sMapOffset;

    IDE_ASSERT( aSegHdr != NULL );

    /* Segment Control Header 초기화 */
    aSegHdr->mSegCntlHdr.mSegType     = aSegType;
    aSegHdr->mSegCntlHdr.mSegState    = SDP_SEG_USE;

    /* Extent Control Header 초기화 */
    aSegHdr->mExtCntlHdr.mTotExtCnt         = 0;
    aSegHdr->mExtCntlHdr.mLstExtDir         = aSegHdrPID;
    aSegHdr->mExtCntlHdr.mTotExtDirCnt      = 0;
    aSegHdr->mExtCntlHdr.mPageCntInExt      = aPageCntInExt;

    sdpPhyPage::initLogicalHdr( sdpPhyPage::getHdr((UChar*)aSegHdr),
                                ID_SIZEOF(sdpscSegMetaHdr) );

    sMapOffset = sdpPhyPage::getDataStartOffset(
                                ID_SIZEOF(sdpscSegMetaHdr) ); // Logical Header

    /* ExtDir Control Header 초기화 */
    sdpscExtDir::initCntlHdr( &aSegHdr->mExtDirCntlHdr,
                              aSegHdrPID,  /* aNxtExtDir */
                              sMapOffset,
                              aMaxExtCntInExtDir );

    return;
}

/***********************************************************************
 *
 * Description : [ INTERFACE ] segment 상태를 반환한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] 테이블스페이스 ID
 * aSegPID     - [IN] 세그먼트 헤더 페이지의 PID
 * aSegState   - [OUT] 세그먼트 상태값
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::getSegState( idvSQL        *aStatistics,
                                 scSpaceID      aSpaceID,
                                 scPageID       aSegPID,
                                 sdpSegState   *aSegState )
{
    idBool        sDummy;
    UChar       * sPagePtr;
    sdpSegState   sSegState;
    UInt          sState = 0;

    IDE_ASSERT( aSegPID != SD_NULL_PID );

    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          &sPagePtr,
                                          &sDummy ) != IDE_SUCCESS );
    sState = 1;

    sSegState = getSegCntlHdr( getHdrPtr(sPagePtr) )->mSegState;

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
              != IDE_SUCCESS );

    *aSegState = sSegState;

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
 *
 * Description : Segment Header 페이지 생성 및 초기화
 *
 * 첫번째 Extent의 bmp 페이지를 모두 생성한 후 Segment Header
 * 페이지를 생성한다.
 *
 * aStatistics        - [IN] 통계정보
 * aStartInfo         - [IN] Mtx 시작 정보
 * aSpaceID           - [IN] 테이블스페이스 ID
 * aNewSegPID         - [IN] 생성할 세그먼트 헤더 PID
 * aSegType           - [IN] 세그먼트 타입
 * aMaxExtCntInExtDir - [IN] 세그먼트 헤더 페이지의 Extent Map에
 *                           기록할 최대 Extent Desc. 개수
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::createAndInitPage( idvSQL            * aStatistics,
                                       sdrMtxStartInfo   * aStartInfo,
                                       scSpaceID           aSpaceID,
                                       sdpscExtDesc      * aFstExtDesc,
                                       sdpSegType          aSegType,
                                       UShort              aMaxExtCntInExtDir )
{
    sdrMtx             sMtx;
    UInt               sState = 0;
    UChar            * sPagePtr;
    scPageID           sSegHdrPID;
    sdpParentInfo      sParentInfo;

    IDE_ASSERT( aStartInfo  != NULL );
    IDE_ASSERT( aFstExtDesc != NULL );

    sSegHdrPID = aFstExtDesc->mExtFstPID;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sParentInfo.mParentPID   = sSegHdrPID;
    sParentInfo.mIdxInParent = SDPSC_INVALID_IDX;

    /*
     * 첫번째 Extent에는 Segment Header를 할당한다.
     * 생성된 Segment Header 페이지의 PID를 반환한다.
     * logical header는 아래에서 별도로 초기화한다.
     */
    IDE_TEST( sdpscAllocPage::createPage( aStatistics,
                                          aSpaceID,
                                          sSegHdrPID,
                                          SDP_PAGE_CMS_SEGHDR,
                                          &sParentInfo,
                                          (sdpscPageBitSet)
                                          ( SDPSC_BITSET_PAGETP_META |
                                            SDPSC_BITSET_PAGEFN_FUL ),
                                          &sMtx,
                                          &sMtx,
                                          &sPagePtr ) != IDE_SUCCESS );

    // Segment Header 페이지 초기화
    initSegHdr( getHdrPtr(sPagePtr),
                sSegHdrPID,
                aSegType,
                aFstExtDesc->mLength,
                aMaxExtCntInExtDir );

    // INIT_SEGMENT_META_HEADER 로깅
    IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx,
                                         (UChar*)getHdrPtr(sPagePtr),
                                         NULL,
                                         ID_SIZEOF( aSegType ) +
                                         ID_SIZEOF( sSegHdrPID ) +
                                         ID_SIZEOF( UInt ) +
                                         ID_SIZEOF( UShort ),
                                         SDR_SDPSC_INIT_SEGHDR )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (UChar*)&aSegType,
                                   ID_SIZEOF( aSegType ) ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (UChar*)&sSegHdrPID,
                                   ID_SIZEOF( sSegHdrPID ) ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (UChar*)&aFstExtDesc->mLength,
                                   ID_SIZEOF( UInt ) ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (UChar*)&aMaxExtCntInExtDir,
                                   ID_SIZEOF( UShort ) ) != IDE_SUCCESS );

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
 *
 * Description : Segment Header 페이지에 새로운 ExtDir를 추가
 *               Segment Header에 TotExtDescCnt도 증가시킨다.
 *
 * aStatistics    - [IN] 통계정보
 * aMtx           - [IN] Mtx 포인터
 * aSpaceID       - [IN] 테이블스페이스 ID
 * aSegHdrPID     - [IN] 세그먼트 헤더 페이지 ID
 * aCurExtDirPID  - [IN] 현재 Extent Dir. 페이지 Control 헤더 포인터
 * aNewExtDirPtr  - [IN] 추가할 Extent Dir. 페이지 시작 포인터
 * aTotExtDescCnt - [OUT] 추가된 ExtDir의 ExtDesc 개수
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::addNewExtDir( idvSQL             * aStatistics,
                                  sdrMtx             * aMtx,
                                  scSpaceID            aSpaceID,
                                  scPageID             aSegHdrPID,
                                  scPageID             aCurExtDirPID,
                                  scPageID             aNewExtDirPID,
                                  UInt               * aTotExtCntOfToSeg )
{
    sdpscSegMetaHdr    * sSegHdr;
    sdpscExtCntlHdr    * sExtCntlHdr;
    sdpscExtDirCntlHdr * sCurExtDirCntlHdr;
    sdpscExtDirCntlHdr * sNewExtDirCntlHdr;
    UInt                 sTotExtCntOfSeg;

    IDE_ASSERT( aMtx              != NULL );
    IDE_ASSERT( aSegHdrPID        != SD_NULL_PID );
    IDE_ASSERT( aCurExtDirPID     != SD_NULL_PID );
    IDE_ASSERT( aNewExtDirPID     != SD_NULL_PID );
    IDE_ASSERT( aTotExtCntOfToSeg != NULL );

    IDE_TEST( fixAndGetHdr4Write( aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  aSegHdrPID,
                                  &sSegHdr ) != IDE_SUCCESS );

    if ( aCurExtDirPID != aSegHdrPID )
    {
        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write( aStatistics,
                                                       aMtx,
                                                       aSpaceID,
                                                       aCurExtDirPID,
                                                       &sCurExtDirCntlHdr )
                  != IDE_SUCCESS );
    }
    else
    {
        sCurExtDirCntlHdr = getExtDirCntlHdr( sSegHdr );
    }

    IDE_ASSERT( sCurExtDirCntlHdr->mMaxExtCnt == sCurExtDirCntlHdr->mExtCnt );

    IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write( aStatistics,
                                                   aMtx,
                                                   aSpaceID,
                                                   aNewExtDirPID,
                                                   &sNewExtDirCntlHdr )
              != IDE_SUCCESS );
    /* full 상태의 Ext만 steal 대상이 됨 */
    IDE_ASSERT( sNewExtDirCntlHdr->mMaxExtCnt == sNewExtDirCntlHdr->mExtCnt );
    /* CurExtDir에 NewExtDir 연결 */
    IDE_TEST( sdpscSegHdr::addNewExtDir( aMtx,
                                         sSegHdr,
                                         sCurExtDirCntlHdr,
                                         sNewExtDirCntlHdr )
              != IDE_SUCCESS );

    sExtCntlHdr = getExtCntlHdr( sSegHdr );

    sTotExtCntOfSeg = sExtCntlHdr->mTotExtCnt + sNewExtDirCntlHdr->mExtCnt;

    IDE_TEST( setTotExtCnt( aMtx, sExtCntlHdr, sTotExtCntOfSeg )
              != IDE_SUCCESS );

    *aTotExtCntOfToSeg = sExtCntlHdr->mTotExtCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aTotExtCntOfToSeg = 0;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Segment Header 페이지에 새로운 ExtDir를 추가
 *               Segment Header에 TotExtDescCnt는 수정하지 않고,
 *               호출하는 쪽에서 작업한다.
 *
 * aStatistics       - [IN] 통계정보
 * aMtx              - [IN] Mtx 포인터
 * aSpaceID          - [IN] 테이블스페이스 ID
 * aSegHdr           - [IN] 세그먼트 헤더 포인터
 * aCurExtDirCntlHdr - [IN] 현재 Extent Dir. 페이지 Control 헤더 포인터
 * aNewExtDirPtr     - [IN] 추가할 Extent Dir. 페이지 시작 포인터
 *
 ***********************************************************************/
IDE_RC  sdpscSegHdr::addNewExtDir( sdrMtx             * aMtx,
                                   sdpscSegMetaHdr    * aSegHdr,
                                   sdpscExtDirCntlHdr * aCurExtDirCntlHdr,
                                   sdpscExtDirCntlHdr * aNewExtDirCntlHdr )
{
    sdpscExtCntlHdr    * sExtCntlHdr;
    scPageID             sNewExtDirPID;

    IDE_ASSERT( aMtx              != NULL );
    IDE_ASSERT( aSegHdr           != NULL );
    IDE_ASSERT( aCurExtDirCntlHdr != NULL );
    IDE_ASSERT( aNewExtDirCntlHdr != NULL );

    sNewExtDirPID = sdpPhyPage::getPageID(
        sdpPhyPage::getPageStartPtr((UChar*)aNewExtDirCntlHdr) );

    /*
     * Segment Header에 총 Extent Dir. 페이지 개수를 증가시킴
     */
    sExtCntlHdr = getExtCntlHdr( aSegHdr );

    // 현재 ExtDir 페이지가 세그먼트의 마지막이라면 마지막 ExtDir를 갱신한다.
    if ( sdpPhyPage::getPageID(
             sdpPhyPage::getPageStartPtr((UChar*)aCurExtDirCntlHdr) )
         == sExtCntlHdr->mLstExtDir )
    {
        IDE_ASSERT( aCurExtDirCntlHdr->mNxtExtDir != SD_NULL_PID );

        IDE_TEST( setLstExtDir( aMtx,
                                sExtCntlHdr,
                                sNewExtDirPID ) != IDE_SUCCESS );
    }

    IDE_TEST( setTotExtDirCnt( aMtx,
                               sExtCntlHdr,
                               (sExtCntlHdr->mTotExtDirCnt + 1) )
              != IDE_SUCCESS );

    /*
     * New Ext Dir에 Next Extent Dir. 설정함
     *
     * Cur가 Last Extent Dir.페이지 였다면 Nxt는 SD_NULL_PID이었을 것이고,
     * 그렇지 않다면, Nxt Extent Dir 페이지 ID가 설정될것이다
     */
    IDE_TEST( sdpscExtDir::setNxtExtDir( aMtx,
                                         aNewExtDirCntlHdr,
                                         aCurExtDirCntlHdr->mNxtExtDir )
              != IDE_SUCCESS );

    /*
     * 현재 ExtDir의 Next ExtDir를 New ExtDir PID를 설정함.
     */
    IDE_TEST( sdpscExtDir::setNxtExtDir( aMtx,
                                         aCurExtDirCntlHdr,
                                         sNewExtDirPID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Segment Header 페이지로부터 ExtDir 제거
 *               Segment Header에 TotExtDescCnt는 수정한다.
 *
 * aStatistics           - [IN] 통계정보
 * aMtx                  - [IN] Mtx 포인터
 * aSpaceID              - [IN] 테이블스페이스 ID
 * aSegHdrPID            - [IN] 세그먼트 헤더 포인터
 * aPrvPIDOfExtDir       - [IN] 이전 Extent Dir. 페이지 Control 헤더 포인터
 * aRemExtDirPID         - [IN] 제거할 Extent Dir. 페이지 시작 포인터
 * aNxtPIDOfNewExtDir    - [IN] 제거한 Extent Dir. 페이지의 NxtPID
 * aTotExtCntOfRemExtDir - [IN] 제거한 Extent Dir.의 ExtDesc 개수 
 * aTotExtCntOfFrSeg     - [OUT] 제거한 후의 Segment의 총 ExtDesc 개수
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::removeExtDir( idvSQL             * aStatistics,
                                  sdrMtx             * aMtx,
                                  scSpaceID            aSpaceID,
                                  scPageID             aSegHdrPID,
                                  scPageID             aPrvPIDOfExtDir,
                                  scPageID             aRemExtDirPID,
                                  scPageID             aNxtPIDOfNewExtDir,
                                  UShort               aTotExtCntOfRemExtDir,
                                  UInt               * aTotExtCntOfFrSeg )
{
    sdpscSegMetaHdr    * sSegHdr;
    sdpscExtCntlHdr    * sExtCntlHdr;
    sdpscExtDirCntlHdr * sPrvExtDirCntlHdr;
    UInt                 sTotExtCntOfSeg = 0;

    IDE_ASSERT( aSegHdrPID           != SD_NULL_PID );
    IDE_ASSERT( aPrvPIDOfExtDir      != SD_NULL_PID );
    IDE_ASSERT( aRemExtDirPID        != SD_NULL_PID );
    IDE_ASSERT( aNxtPIDOfNewExtDir   != SD_NULL_PID );
    IDE_ASSERT( aTotExtCntOfRemExtDir > 0 );
    IDE_ASSERT( aTotExtCntOfFrSeg    != NULL );

    /* BUG-31055 Can not reuse undo pages immediately after it is used to 
     *           aborted transaction.
     * Segment Header를 제거하면 안된다 */
    if( aSegHdrPID == aRemExtDirPID )
    {
        ideLog::log( IDE_SERVER_0, 
                     "aSpaceID              = %u\n"
                     "aSegHdrPID            = %u\n"
                     "aPrvPIDOfExtDir       = %u\n"
                     "aRemExtDirPID         = %u\n"
                     "aNxtPIDOfNewExtDir    = %u\n"
                     "aTotExtCntOfRemExtDir = %u\n",
                     aSpaceID,
                     aSegHdrPID,
                     aPrvPIDOfExtDir,
                     aRemExtDirPID,
                     aNxtPIDOfNewExtDir,
                     aTotExtCntOfRemExtDir );

        IDE_ASSERT( 0 );
    }

    IDE_TEST( fixAndGetHdr4Write( aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  aSegHdrPID,
                                  &sSegHdr ) != IDE_SUCCESS );

    if ( aPrvPIDOfExtDir != aSegHdrPID )
    {
        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write( aStatistics,
                                                       aMtx,
                                                       aSpaceID,
                                                       aPrvPIDOfExtDir,
                                                       &sPrvExtDirCntlHdr )
                  != IDE_SUCCESS );
    }
    else
    {
        sPrvExtDirCntlHdr = getExtDirCntlHdr( sSegHdr );
    }

    IDE_TEST( removeExtDir( aMtx,
                            sSegHdr,
                            sPrvExtDirCntlHdr,
                            aRemExtDirPID,
                            aNxtPIDOfNewExtDir ) != IDE_SUCCESS );

    sExtCntlHdr = getExtCntlHdr( sSegHdr );

    IDE_ASSERT( sExtCntlHdr->mTotExtCnt > aTotExtCntOfRemExtDir );

    sTotExtCntOfSeg = sExtCntlHdr->mTotExtCnt - aTotExtCntOfRemExtDir;

    IDE_TEST( setTotExtCnt( aMtx, sExtCntlHdr, sTotExtCntOfSeg )
              != IDE_SUCCESS );

    *aTotExtCntOfFrSeg = sExtCntlHdr->mTotExtCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Segment Header 페이지로부터 ExtDir 제거
 *               Segment Header에 TotExtDescCnt는 수정하지 않고,
 *               호출하는 쪽에서 작업한다.
 *
 * 예를들어, 다음과 같은 경우
 *
 * ExtDir0   ExtDir1    ExtDir2    ExtDir3
 *
 * [ 67 ] -> [ 131 ] -> [ 163 ] -> [ 195 ] -> NULL
 *  Cur        shk       NewCur
 *
 * 에서 ExtDir1 [ 131 ] 이 Shrink 대상이라면 [67]에 [163]을 연결해야한다.
 *
 * ExtDir0   ExtDir1    ExtDir2
 *
 * [ 67 ] -> [ 163 ] -> [ 195 ] -> NULL
 *  Cur        shk       NewCur
 *
 * 예를들어, 다음과 같은 경우
 *
 * ExtDir0   ExtDir1
 *
 * [ 67 ] -> [ 131 ] -> NULL
 *  Cur        Shk
 *
 * 에서 ExtDir1 [ 131 ] 이 Shrink 대상이라면 [67]에 NULL 을 연결해야한다.
 *
 * ExtDir0   ExtDir1    ExtDir2
 *
 * [ 67 ] -> NULL
 *  NewCur
 *
 * aMtx              - [IN] Mtx 포인터
 * aSegHdrPtr        - [IN] 세그먼트 헤더 포인터
 * aPrvExtDirCntlHdr - [IN] 이전 Extent Dir. Control 헤더 포인터
 * aRemExtDirPID     - [IN] 제거할 Extent Dir. 페이지 ID
 * aNewNxtExtDir     - [IN] 다음 Extent Dir. 페이지 ID
 *
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::removeExtDir( sdrMtx             * aMtx,
                                  sdpscSegMetaHdr    * aSegHdr,
                                  sdpscExtDirCntlHdr * aPrvExtDirCntlHdr,
                                  scPageID             aRemExtDirPID,
                                  scPageID             aNewNxtExtDir )
{
    scPageID            sPrvExtDirPID;
    sdpscExtCntlHdr   * sExtCntlHdr;
    UInt                sTotExtDirCnt;

    IDE_ASSERT( aSegHdr           != NULL );
    IDE_ASSERT( aPrvExtDirCntlHdr != NULL );
    IDE_ASSERT( aMtx              != NULL );
    IDE_ASSERT( aRemExtDirPID     != SD_NULL_PID );
    IDE_ASSERT( aNewNxtExtDir     != SD_NULL_PID );

    sExtCntlHdr   = getExtCntlHdr( aSegHdr );

    IDE_ERROR_RAISE( sExtCntlHdr->mTotExtDirCnt > 0,
                     ERR_ZERO_TOTEXTDIRCNT );

    sTotExtDirCnt = sExtCntlHdr->mTotExtDirCnt - 1;
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sExtCntlHdr->mTotExtDirCnt,
                                         &sTotExtDirCnt,
                                         ID_SIZEOF( sTotExtDirCnt ) )
              != IDE_SUCCESS );

    if ( aRemExtDirPID == sExtCntlHdr->mLstExtDir )
    {
        // 제거할 ExtDir 페이지가 세그먼트의 LstExtDir이라면 마지막 ExtDir를 현재 갱신한다.
        sPrvExtDirPID = sdpPhyPage::getPageID(
            sdpPhyPage::getPageStartPtr(aPrvExtDirCntlHdr) );

        IDE_TEST( sdrMiniTrans::writeNBytes(
                                aMtx,
                                (UChar*)&sExtCntlHdr->mLstExtDir,
                                &sPrvExtDirPID,
                                ID_SIZEOF( scPageID ) )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(aPrvExtDirCntlHdr->mNxtExtDir),
                  &aNewNxtExtDir,
                  ID_SIZEOF( scPageID ) ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ZERO_TOTEXTDIRCNT);
    {
        ideLog::log(
            IDE_ERR_0,
            "PrvExtDirCntlHdr : mExtCnt        : %u\n"
            "                   mNxtExtDir     : %u\n"
            "                   mMaxExtCnt     : %u\n"
            "                   mMapOffset     : %u\n"
            "                   mLstCommitSCN  : %llu\n"
            "                   mFstDskViewSCN : %llu\n"
            "RemExtDirPID     : %u\n"
            "NewNxtExtDir     : %u\n"
            "ExtCntlHdr       : mTotExtCnt     : %u\n"
            "                   mLstExtDir     : %u\n"
            "                   mTotExtDirCnt  : %u\n"
            "                   mPageCntInExt  : %u\n",
             aPrvExtDirCntlHdr->mExtCnt,
             aPrvExtDirCntlHdr->mNxtExtDir,
             aPrvExtDirCntlHdr->mMaxExtCnt,
             aPrvExtDirCntlHdr->mMapOffset,
#ifdef COMPILE_64BIT
            aPrvExtDirCntlHdr->mLstCommitSCN,
#else
            ( ((ULong)aPrvExtDirCntlHdr->mLstCommitSCN.mHigh) << 32 ) |
                (ULong)aPrvExtDirCntlHdr->mLstCommitSCN.mLow,
#endif
#ifdef COMPILE_64BIT
            aPrvExtDirCntlHdr->mFstDskViewSCN,
#else
            ( ((ULong)aPrvExtDirCntlHdr->mFstDskViewSCN.mHigh) << 32 ) |
                (ULong)aPrvExtDirCntlHdr->mFstDskViewSCN.mLow,
#endif
            aRemExtDirPID,
            aNewNxtExtDir,
            sExtCntlHdr->mTotExtCnt,
            sExtCntlHdr->mLstExtDir,
            sExtCntlHdr->mTotExtDirCnt,
            sExtCntlHdr->mPageCntInExt );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Sequential Scan를 위한 Segment의 정보를 반환한다.
 *
 * aStatistics  - [IN] 통계정보
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aSegPID      - [IN] 세그먼트 헤더 페이지의 PID
 * aSegInfo     - [OUT] 수집된 세그먼트 정보 자료구조
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::getSegInfo( idvSQL        * aStatistics,
                                scSpaceID       aSpaceID,
                                scPageID        aSegPID,
                                void          * /*aTableHeader*/,
                                sdpSegInfo    * aSegInfo )
{
    sdpscSegMetaHdr  * sSegHdr;
    SInt               sState = 0;

    IDE_ASSERT( aSegInfo != NULL );

    idlOS::memset( aSegInfo, 0x00, ID_SIZEOF(sdpSegInfo) );

    IDE_TEST( fixAndGetHdr4Read( aStatistics,
                                 NULL,
                                 aSpaceID,
                                 aSegPID,
                                 &sSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    aSegInfo->mSegHdrPID    = aSegPID;
    aSegInfo->mType         = (sdpSegType)(sSegHdr->mSegCntlHdr.mSegType);
    aSegInfo->mState        = (sdpSegState)(sSegHdr->mSegCntlHdr.mSegState);
    aSegInfo->mPageCntInExt = sSegHdr->mExtCntlHdr.mPageCntInExt;
    aSegInfo->mExtCnt       = sSegHdr->mExtCntlHdr.mTotExtCnt;
    aSegInfo->mExtDirCnt    = sSegHdr->mExtCntlHdr.mTotExtDirCnt;

    IDE_ASSERT( aSegInfo->mExtCnt > 0 );

    aSegInfo->mFstExtRID    = SD_MAKE_RID(
        aSegPID,
        sdpscExtDir::calcDescIdx2Offset(&(sSegHdr->mExtDirCntlHdr), 0) );

    aSegInfo->mFstPIDOfLstAllocExt = aSegPID;

    sState = 0;
    IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics, (UChar*)sSegHdr )
                == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics, (UChar*)sSegHdr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : FmtPageCnt를 얻어준다.
 *
 * aStatistics  - [IN] 통계 정보
 * aSpaceID     - [IN] TableSpace ID
 * aSegHandle   - [IN] Segment Handle
 *
 * aSegCache    - [OUT] SegInfo가 설정된다.
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::getFmtPageCnt( idvSQL        *aStatistics,
                                   scSpaceID      aSpaceID,
                                   sdpSegHandle  *aSegHandle,
                                   ULong         *aFmtPageCnt )
{
    sdpSegInfo  sSegInfo;

    IDE_ASSERT( aSegHandle  != NULL );
    IDE_ASSERT( aFmtPageCnt != NULL );

    IDE_TEST( getSegInfo( aStatistics,
                          aSpaceID,
                          aSegHandle->mSegPID,
                          NULL, /* aTableHeader */
                          &sSegInfo ) != IDE_SUCCESS );

    *aFmtPageCnt = sSegInfo.mFmtPageCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 갱신목적으로 SegHdr 페이지의 Control Header를 fix한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mtx 포인터
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aSegHdrPID   - [IN] X-Latch를 획득할 SegHdr 페이지의 PID
 * aSegHdr      - [OUT] SegHdr.페이지의 헤더 포인터
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::fixAndGetHdr4Write(
                      idvSQL           * aStatistics,
                      sdrMtx           * aMtx,
                      scSpaceID          aSpaceID,
                      scPageID           aSegHdrPID,
                      sdpscSegMetaHdr ** aSegHdr )
{
    idBool   sDummy;
    UChar  * sPagePtr;

    IDE_ASSERT( aSegHdrPID != SD_NULL_PID );
    IDE_ASSERT( aSegHdr    != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegHdrPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL )
              != IDE_SUCCESS );

    IDE_ASSERT( sdpPhyPage::getPhyPageType( sPagePtr ) ==
                SDP_PAGE_CMS_SEGHDR );

    *aSegHdr = getHdrPtr( sPagePtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aSegHdr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 갱신목적으로 SegHdr 페이지의 Control Header를 fix한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mtx 포인터
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aSegHdrPID   - [IN] S-Latch를 획득할 SegHdr 페이지의 PID
 * aSegHdr      - [OUT] SegHdr.페이지의 헤더 포인터
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::fixAndGetHdr4Read(
                      idvSQL             * aStatistics,
                      sdrMtx             * aMtx,
                      scSpaceID            aSpaceID,
                      scPageID             aSegHdrPID,
                      sdpscSegMetaHdr   ** aSegHdr )
{
    idBool               sDummy;
    UChar              * sPagePtr;

    IDE_ASSERT( aSegHdrPID != SD_NULL_PID );
    IDE_ASSERT( aSegHdr    != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegHdrPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL )
              != IDE_SUCCESS );

    IDE_ASSERT( sdpPhyPage::getPhyPageType( sPagePtr ) ==
                SDP_PAGE_CMS_SEGHDR );

    *aSegHdr = getHdrPtr( sPagePtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aSegHdr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 세그먼트 Extent Control Header에 총 ExtDir 개수 설정
 *
 * aMtx           - [IN] Mtx 포인터
 * aExtCntlHdr    - [IN] 세그먼트 Extent Control Header 포인터
 * aTotExtDirCnt - [IN] 세그먼트 총 ExtDir 페이지 개수
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::setTotExtDirCnt( sdrMtx          * aMtx,
                                     sdpscExtCntlHdr * aExtCntlHdr,
                                     UInt              aTotExtDirCnt )
{
    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aExtCntlHdr   != NULL );

    return sdrMiniTrans::writeNBytes( aMtx,
                                      (UChar*)&aExtCntlHdr->mTotExtDirCnt,
                                      &aTotExtDirCnt,
                                      ID_SIZEOF( aTotExtDirCnt ) );
}


/***********************************************************************
 *
 * Description : 세그먼트 Extent Control Header에 LstExtDirPageID 설정
 *
 * aMtx          - [IN] Mtx 포인터
 * aExtCntlHdr   - [IN] 세그먼트 Extent Control Header 포인터
 * aLstExtDirPID - [IN] 마지막 ExtDirPage의 페이지 ID
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::setLstExtDir( sdrMtx             * aMtx,
                                  sdpscExtCntlHdr    * aExtCntlHdr,
                                  scPageID             aLstExtDirPID )
{
    IDE_ASSERT( aMtx              != NULL );
    IDE_ASSERT( aExtCntlHdr       != NULL );

    return sdrMiniTrans::writeNBytes( aMtx,
                                      (UChar*)&aExtCntlHdr->mLstExtDir,
                                      &aLstExtDirPID,
                                      ID_SIZEOF( aLstExtDirPID ) );
}


/***********************************************************************
 *
 * Description : 세그먼트 Extent Control Header에 총 ExtDesc 개수 설정
 *
 * aMtx           - [IN] Mtx 포인터
 * aExtCntlHdr    - [IN] 세그먼트 Extent Control Header 포인터
 * aTotExtDescCnt - [IN] 세그먼트 총 ExtDesc 개수
 *
 ***********************************************************************/
IDE_RC sdpscSegHdr::setTotExtCnt( sdrMtx          * aMtx,
                                  sdpscExtCntlHdr * aExtCntlHdr,
                                  UInt              aTotExtDescCnt )
{
    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aExtCntlHdr   != NULL );

    return sdrMiniTrans::writeNBytes( aMtx,
                                      (UChar*)&aExtCntlHdr->mTotExtCnt,
                                      &aTotExtDescCnt,
                                      ID_SIZEOF( aTotExtDescCnt ) );
}

