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
 * $Id: sdpstSegDDL.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Create/Drop/Alter/Reset 연산의
 * STATIC 인터페이스들을 관리한다.
 *
 ***********************************************************************/

# include <smr.h>
# include <sdr.h>
# include <sdbBufferMgr.h>
# include <sdpReq.h>

# include <sdpstDef.h>
# include <sdpstSegDDL.h>

# include <sdpstSH.h>
# include <sdpstLfBMP.h>
# include <sdpstItBMP.h>
# include <sdpstRtBMP.h>
# include <sdpstExtDir.h>
# include <sdpstCache.h>
# include <sdpstAllocExtInfo.h>
# include <sdptbExtent.h>
# include <sdpstDPath.h>
# include <sdpstStackMgr.h>

/***********************************************************************
 * Description : [ INTERFACE ] Segment 할당 및 Segment Meta Header 초기화
 *
 * aStatistics - [IN]  통계정보
 * aMtx        - [IN]  Mini Transaction의 Pointer
 * aSpaceID    - [IN]  Tablespace의 ID
 * aSegType    - [IN]  Segment의 Type
 * aSegHandle  - [IN]  Segment의 Handle
 ***********************************************************************/
IDE_RC sdpstSegDDL::createSegment( idvSQL                * aStatistics,
                                   sdrMtx                * aMtx,
                                   scSpaceID               aSpaceID,
                                   sdpSegType              /* aSegType */,
                                   sdpSegHandle          * aSegHandle )
{
    IDE_ASSERT( aMtx != NULL );
    IDE_ASSERT( aSegHandle != NULL );

    IDE_TEST( allocateSegment( aStatistics,
                               aMtx,
                               aSpaceID,
                               aSegHandle ) != IDE_SUCCESS );

    IDE_ASSERT( aSegHandle->mSegPID != SD_NULL_PID );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : [ INTERFACE ] Segment 해제
 *
 * Segment에서 Extent를 해제하는 순서는 첫번재 ExtDir페이지부터
 * 마지막 ExtDir페이지까지 해제한 이후 SegHdr의 ExtDir영역을
 * 해제하여 모든 Extent를 해제한다.
 *
 * aStatistics - [IN]  통계정보
 * aMtx        - [IN]  Mini Transaction의 Pointer
 * aSpaceID    - [IN]  Tablespace의 ID
 * aSegPID     - [IN]  Segment의 Hdr PID
 ***********************************************************************/
IDE_RC sdpstSegDDL::dropSegment( idvSQL           * aStatistics,
                                 sdrMtx           * aMtx,
                                 scSpaceID          aSpaceID,
                                 scPageID           aSegPID )
{
    UChar           * sPagePtr;
    sdpstSegHdr     * sSegHdr;

    IDE_ASSERT( aMtx != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sSegHdr = sdpstSH::getHdrPtr( sPagePtr );

    IDE_TEST( freeAllExts( aStatistics,
                           aMtx,
                           aSpaceID,
                           sSegHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : [ INTERFACE ] Segment 리셋
 *
 * aStatistics - [IN]  통계정보
 * aMtx        - [IN]  Mini Transaction의 Pointer
 * aSpaceID    - [IN]  Tablespace의 ID
 * aSegHandle  - [IN]  Segment의 Handle
 * aSegType    - [IN]  Segment의 Type
 ***********************************************************************/
IDE_RC sdpstSegDDL::resetSegment( idvSQL           * aStatistics,
                                  sdrMtx           * aMtx,
                                  scSpaceID          aSpaceID,
                                  sdpSegHandle     * aSegHandle,
                                  sdpSegType         /*aSegType*/ )
{
    sdrMtxStartInfo      sStartInfo;
    sdrMtx               sMtx;
    sdpstSegHdr        * sSegHdr;
    scPageID             sSegPID = SD_NULL_PID;
    sdpstExtDesc         sExtDesc;
    sdpstExtDesc       * sExtDescPtr;
    UInt                 sState = 0 ;
    idBool               sIsFixedPage = ID_FALSE;
    sdpstBfrAllocExtInfo sBfrAllocExtInfo;
    sdpstAftAllocExtInfo sAftAllocExtInfo;
    UChar              * sPagePtr;
    sdpSegType           sSegType;
    sdRID                sAllocExtRID;
    sdpstSegCache      * sSegCache;

    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aMtx != NULL );

    sSegCache = (sdpstSegCache*)(aSegHandle->mCache);

    /*
     * A. 첫번째 Extent 정보를 얻는다.
     */
    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegHandle->mSegPID,
                                          &sPagePtr ) != IDE_SUCCESS );
    sIsFixedPage = ID_TRUE;

    sSegHdr  = sdpstSH::getHdrPtr( sPagePtr );
    sSegType = sSegHdr->mSegType;

    IDE_TEST( freeExtsExceptFst( aStatistics,
                                 aMtx,
                                 aSpaceID,
                                 sSegHdr ) != IDE_SUCCESS );

    /* 함수 내부적으로 별도의 Mtx를 가지고 작업을 수행한다.
     * 인자로 넘어온 aMtx은 Reset Segment 연산이후
     * 첫번째 Extent 초기화하는 작업에 사용한다. */

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    // 첫번째 Extent만 남기고 모두 해제되었으므로 첫번째 Extent로
    // 다시 생성한다.
    sdpstExtDir::initExtDesc( &sExtDesc );
    sdpstAllocExtInfo::initialize( &sBfrAllocExtInfo );
    sdpstAllocExtInfo::initialize( &sAftAllocExtInfo );

    /* 첫번째 Extent Slot을 반환한다. */
    sExtDescPtr = sdpstExtDir::getFstExtDesc( sdpstSH::getExtDirHdr(sPagePtr) );

    sExtDesc.mExtFstPID = sExtDescPtr->mExtFstPID;
    sExtDesc.mLength    = sExtDescPtr->mLength;

    sIsFixedPage = ID_FALSE;
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr ) != IDE_SUCCESS );

    sdpSegment::initCommonCache( &sSegCache->mCommon,
                                 sSegType,
                                 sExtDesc.mLength,
                                 sSegCache->mCommon.mTableOID,
                                 SM_NULL_INDEX_ID);

    /* Segment Header가 생성된 경우 기존 Treelist 구조에
     * 대한 정보를 모은다. */
    IDE_TEST( sdpstAllocExtInfo::getAllocExtInfo(
                                        aStatistics,
                                        aSpaceID,
                                        sSegPID,
                                        aSegHandle->mSegStoAttr.mMaxExtCnt,
                                        &sExtDesc,
                                        &sBfrAllocExtInfo,
                                        &sAftAllocExtInfo ) != IDE_SUCCESS );

    /* createMetaPage() 에서 Segment Header를 생성하게 한다.
     * mSegHdrCnt는 0 또는 1 값만 갖고, 이미 SegHdr가 존재하는 경우 항상 0
     * 이어야 한다. */
    sAftAllocExtInfo.mSegHdrCnt = 1;

    // 새로운 Extent에 대한 Bitmap Page들을 생성한다.
    IDE_TEST( createMetaPages( aStatistics,
                               &sStartInfo,
                               aSpaceID,
                               sSegCache,
                               &sExtDesc,
                               &sBfrAllocExtInfo,
                               &sAftAllocExtInfo,
                               &sSegPID ) != IDE_SUCCESS );

    IDE_TEST( createDataPages( aStatistics,
                               &sStartInfo,
                               aSpaceID,
                               aSegHandle,
                               &sExtDesc,
                               &sBfrAllocExtInfo,
                               &sAftAllocExtInfo ) != IDE_SUCCESS );

    IDE_ASSERT( aSegHandle->mSegPID == sSegPID );

    sdpstCache::initItHint( aStatistics,
                            sSegCache,
                            aSegHandle->mSegPID,
                            sAftAllocExtInfo.mFstPID[SDPST_ITBMP] );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
    sState = 1;

    /* Extent 단위로 Segment Header 페이지의 ExtDir 페이지에
     * 기록하고, Segment Header 페이지를 설정한다.
     * 이전 Lst BMP 페이지에 새로 생성한 bmp 페이지들의 정보를 기록한다. */
    IDE_TEST( linkNewExtToLstBMPs( aStatistics,
                                   &sMtx,
                                   aSpaceID,
                                   aSegHandle,
                                   &sExtDesc,
                                   &sBfrAllocExtInfo,
                                   &sAftAllocExtInfo ) != IDE_SUCCESS );

    /* Extent 단위로 Segment Header 페이지의 ExtDir 페이지에
     * 기록하고, Segment Header 페이지를 설정한다. */
    IDE_TEST( linkNewExtToExtDir( aStatistics,
                                  &sMtx,
                                  aSpaceID,
                                  sSegPID,
                                  &sExtDesc,
                                  &sBfrAllocExtInfo,
                                  &sAftAllocExtInfo,
                                  &sAllocExtRID ) != IDE_SUCCESS );

    /* Extent 할당 마지막 완료작업으로 Segment Header에
     * BMP 페이지를 연결한다. */
    IDE_TEST( linkNewExtToSegHdr( aStatistics,
                                  &sMtx,
                                  aSpaceID,
                                  sSegPID,
                                  &sExtDesc,
                                  &sBfrAllocExtInfo,
                                  &sAftAllocExtInfo,
                                  sAllocExtRID,
                                  ID_TRUE,
                                  sSegCache ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsFixedPage == ID_TRUE )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sPagePtr ) == IDE_SUCCESS );
    }
    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment를 할당한다.
 *               segment 생성시 초기 extent 개수를 할당한다. 만약, 생성하다가
 *               부족하다면 exception 처리한다.
 *
 *               [ 복구 알고리즘 ]
 *               alloc ext from space : alloc ext from space 연산으로 처리한다.
 *                                      반드시 Undo 되도록 한다.
 *               alloc ext to segment : alloc ext to seg 로깅하여 Undo 되도록 
 *                                      하기나, op null 로깅하여 Undo 되지
 *                                      않도록 한다.
 *               create segment(DDL)  : Table/Index/Lob Segment
 *                                      OP_CREATE_XX_SEGMENT 타입의 NTA에 대한
 *                                      UNDO 연산을 수행한다.
 *
 * aStatistics - [IN]  통계정보
 * aMtx        - [IN]  Mini Transaction의 Pointer
 * aSpaceID    - [IN]  Tablespace의 ID
 * aSegHandle  - [IN]  Segment의 Handle
 * aSegType    - [IN]  Segment의 Type
 ***********************************************************************/
IDE_RC sdpstSegDDL::allocateSegment( idvSQL       * aStatistics,
                                     sdrMtx       * aMtx,
                                     scSpaceID      aSpaceID,
                                     sdpSegHandle * aSegHandle )
{
    sdrMtx               sMtx;
    UInt                 sLoop;
    sdpstExtDesc         sExtDesc;
    scPageID             sSegPID;
    UInt                 sState = 0;
    sdrMtxStartInfo      sStartInfo;
    sdpstBfrAllocExtInfo sBfrAllocExtInfo;
    sdpstAftAllocExtInfo sAftAllocExtInfo;
    smiSegStorageAttr  * sSegStoAttr;
    sdpstSegCache      * sSegCache;
    sdRID                sAllocExtRID;

    IDE_ASSERT( aMtx != NULL );
    IDE_ASSERT( aSegHandle != NULL );

    sSegPID    = SD_NULL_PID;

    sSegCache   = (sdpstSegCache*)(aSegHandle->mCache);
    sSegStoAttr = sdpSegDescMgr::getSegStoAttr( aSegHandle );

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    IDE_ASSERT( sSegStoAttr->mInitExtCnt > 0 );

    for ( sLoop = 0; sLoop < sSegStoAttr->mInitExtCnt; sLoop++ )
    {
        sdpstExtDir::initExtDesc( &sExtDesc );
        sdpstAllocExtInfo::initialize( &sBfrAllocExtInfo );
        sdpstAllocExtInfo::initialize( &sAftAllocExtInfo );

        /* A. TBS로부터 Extent를 할당한다.
         * 각 Extent 할당은 local group 단위로 op NTA 처리를 한다.
         * Op NTA - alloc extents from TBS */
        IDE_TEST( sdptbExtent::allocExts( aStatistics,
                                          &sStartInfo,
                                          aSpaceID,
                                          1,       /* Extent 개수 */
                                          (sdpExtDesc*)&sExtDesc )
                  != IDE_SUCCESS );


        /* Segment Header가 생성된 경우 기존 Treelist 구조에
         * 대한 정보를 모은다. */
        IDE_TEST( sdpstAllocExtInfo::getAllocExtInfo( aStatistics,
                                                      aSpaceID,
                                                      sSegPID,
                                                      sSegStoAttr->mMaxExtCnt,
                                                      &sExtDesc,
                                                      &sBfrAllocExtInfo,
                                                      &sAftAllocExtInfo )
                != IDE_SUCCESS );

        /* 새로운 Extent에 대한 Bitmap Page들을 생성한다. */
        IDE_TEST( createMetaPages( aStatistics,
                                   &sStartInfo,
                                   aSpaceID,
                                   sSegCache,
                                   &sExtDesc,
                                   &sBfrAllocExtInfo,
                                   &sAftAllocExtInfo,
                                   &sSegPID ) != IDE_SUCCESS );

        IDE_TEST( createDataPages( aStatistics,
                                   &sStartInfo,
                                   aSpaceID,
                                   aSegHandle,
                                   &sExtDesc,
                                   &sBfrAllocExtInfo,
                                   &sAftAllocExtInfo ) != IDE_SUCCESS );



        /* mSegHdrCnt는 0 또는 1 값만 갖고, 이미 SegHdr가 존재하는 경우 항상 0
         * 이어야 한다. */
        if ( sAftAllocExtInfo.mSegHdrCnt == 1 )
        {
            aSegHandle->mSegPID = sSegPID;

            sdpstCache::initItHint( aStatistics,
                                    sSegCache,
                                    sSegPID,
                                    sAftAllocExtInfo.mFstPID[SDPST_ITBMP] );
        }

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       &sStartInfo,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
        sState = 1;

        /* Extent 단위로 Segment Header 페이지의 ExtDir 페이지에
         * 기록하고, Segment Header 페이지를 설정한다.
         * 이전 Lst BMP 페이지에 새로 생성한 bmp 페이지들의 정보를 기록한다. */
        IDE_TEST( linkNewExtToLstBMPs( aStatistics,
                                       &sMtx,
                                       aSpaceID,
                                       aSegHandle,
                                       &sExtDesc,
                                       &sBfrAllocExtInfo,
                                       &sAftAllocExtInfo ) != IDE_SUCCESS );

        /* Extent 단위로 Segment Header 페이지의 ExtDir 페이지에
         * 기록하고, Segment Header 페이지를 설정한다. */
        IDE_TEST( linkNewExtToExtDir( aStatistics,
                                      &sMtx,
                                      aSpaceID,
                                      sSegPID,
                                      &sExtDesc,
                                      &sBfrAllocExtInfo,
                                      &sAftAllocExtInfo,
                                      &sAllocExtRID ) != IDE_SUCCESS );

        /* Extent 할당 마지막 완료작업으로 Segment Header에
         * BMP 페이지를 연결한다. */
        IDE_TEST( linkNewExtToSegHdr( aStatistics,
                                      &sMtx,
                                      aSpaceID,
                                      sSegPID,
                                      &sExtDesc,
                                      &sBfrAllocExtInfo,
                                      &sAftAllocExtInfo,
                                      sAllocExtRID,
                                      ID_TRUE,
                                      sSegCache ) != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    }

    sSegCache->mCommon.mPageCntInExt = sExtDesc.mLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aSegHandle->mSegPID = SD_NULL_PID;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 새로운 Extent를 위한 BMP 생성
 *
 * aStatistics   - [IN] 통계정보
 * aStartInfo    - [IN] Mtx의 StartInfo
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegCache     - [IN] Segment의 Cache
 * aExtDesc      - [IN] Extent Desc Pointer
 * aBfrInfo      - [IN] Extent 이전 할당 정보
 * aAftInfo      - [IN] Extent 이후 할당 정보
 * aSegPID       - [OUT] Segment 헤더 페이지의 PID
 ***********************************************************************/
IDE_RC sdpstSegDDL::createMetaPages( idvSQL               * aStatistics,
                                     sdrMtxStartInfo      * aStartInfo,
                                     scSpaceID              aSpaceID,
                                     sdpstSegCache        * aSegCache,
                                     sdpstExtDesc         * aExtDesc,
                                     sdpstBfrAllocExtInfo * aBfrInfo,
                                     sdpstAftAllocExtInfo * aAftInfo,
                                     scPageID             * aSegPID )
{
    scPageID    sSegPID;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aSpaceID   != SD_NULL_PID );
    IDE_ASSERT( aExtDesc   != NULL );
    IDE_ASSERT( aAftInfo   != NULL );

    if ( aAftInfo->mPageCnt[SDPST_LFBMP] > 0 )
    {
        IDE_TEST( sdpstLfBMP::createAndInitPages( aStatistics,
                                                  aStartInfo,
                                                  aSpaceID,
                                                  aExtDesc,
                                                  aBfrInfo,
                                                  aAftInfo ) != IDE_SUCCESS );
    }

    if ( aAftInfo->mPageCnt[SDPST_ITBMP] > 0 )
    {
        IDE_TEST( sdpstBMP::createAndInitPages( aStatistics,
                                                aStartInfo,
                                                aSpaceID,
                                                aExtDesc,
                                                SDPST_ITBMP,
                                                aBfrInfo,
                                                aAftInfo ) != IDE_SUCCESS );
    }

    if ( aAftInfo->mPageCnt[SDPST_RTBMP] > 0 )
    {
        IDE_TEST( sdpstBMP::createAndInitPages( aStatistics,
                                                aStartInfo,
                                                aSpaceID,
                                                aExtDesc,
                                                SDPST_RTBMP,
                                                aBfrInfo,
                                                aAftInfo ) != IDE_SUCCESS );
    }

    /* mSegHdrCnt는 0 또는 1 값만 갖고, 이미 SegHdr가 존재하는 경우 항상 0
     * 이어야 한다. */
    if ( aAftInfo->mSegHdrCnt == 1 )
    {
        IDE_TEST( sdpstSH::createAndInitPage( aStatistics,
                                              aStartInfo,
                                              aSpaceID,
                                              aExtDesc,
                                              aBfrInfo,
                                              aAftInfo,
                                              aSegCache,
                                              &sSegPID ) != IDE_SUCCESS );

        if ( aSegPID != NULL )
        {
            *aSegPID = sSegPID;
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
 
/***********************************************************************
 * Description : 새로운 Extent를 위한 BMP 생성
 *
 * aStatistics   - [IN] 통계정보
 * aStartInfo    - [IN] Mtx의 StartInfo
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegCache     - [IN] Segment의 Cache
 * aExtDesc      - [IN] Extent Desc Pointer
 * aBfrInfo      - [IN] Extent 이전 할당 정보
 * aAftInfo      - [IN] Extent 이후 할당 정보
 ***********************************************************************/
IDE_RC sdpstSegDDL::createDataPages( idvSQL               * aStatistics,
                                     sdrMtxStartInfo      * aStartInfo,
                                     scSpaceID              aSpaceID,
                                     sdpSegHandle         * aSegHandle,
                                     sdpstExtDesc         * aExtDesc,
                                     sdpstBfrAllocExtInfo * aBfrInfo,
                                     sdpstAftAllocExtInfo * aAftInfo )
{
    scPageID              sFstDataPID;
    scPageID              sLstDataPID;
    scPageID              sCurPID;
    scPageID              sLfBMP;
    scPageID              sPrvLfBMP;
    SShort                sPBSNo;
    sdpstPBS              sCreatePBS;
    sdrMtx                sMtx;
    UInt                  sState = 0 ;
    UChar               * sPagePtr;
    SShort                sLfBMPIdx;
    UInt                  sMetaPageCnt;
    ULong                 sDataSeqNo;

    sCreatePBS = (SDPST_BITSET_PAGETP_DATA | SDPST_BITSET_PAGEFN_FMT);

    sMetaPageCnt = aAftInfo->mPageCnt[SDPST_EXTDIR] +
                   aAftInfo->mPageCnt[SDPST_LFBMP] +
                   aAftInfo->mPageCnt[SDPST_ITBMP] +
                   aAftInfo->mPageCnt[SDPST_RTBMP] +
                   aAftInfo->mSegHdrCnt;

    /* fst, lst PID 계산 */
    sFstDataPID = aExtDesc->mExtFstPID + sMetaPageCnt;
    sLstDataPID = aExtDesc->mExtFstPID + aExtDesc->mLength - 1;

    /* 첫번째 데이터 페이지를 위한 SeqNo */
    sDataSeqNo = aBfrInfo->mNxtSeqNo + sMetaPageCnt;

    if ( aAftInfo->mPageCnt[SDPST_LFBMP] == 0 )
    {
        /* 이전 마지막 생성된 LfBMP에 할당한 Extent가
         * 모두 들어갈 수 있는 경우 */
        sLfBMP = aBfrInfo->mLstPID[SDPST_LFBMP];
        sPBSNo = aBfrInfo->mLstPBSNo + sFstDataPID - aExtDesc->mExtFstPID + 1;

        for ( sCurPID = sFstDataPID; sCurPID <= sLstDataPID; sCurPID++ )
        {
            sState = 0;
            IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                           &sMtx,
                                           aStartInfo,
                                           ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                           SM_DLOG_ATTR_DEFAULT )
                      != IDE_SUCCESS );
            sState = 1;

            IDE_TEST( sdpstAllocPage::createPage( aStatistics,
                                                  &sMtx,
                                                  aSpaceID,
                                                  aSegHandle,
                                                  sCurPID,
                                                  sDataSeqNo,
                                                  SDP_PAGE_FORMAT,
                                                  sLfBMP,
                                                  sPBSNo,
                                                  sCreatePBS,
                                                  &sPagePtr ) != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

            sPBSNo++;
            sDataSeqNo++;
        }
    }
    else
    {
        /* 현 할당한 Extent는 새로 생성한 LfBMP에 들어가는 경우. */
        sPBSNo = sFstDataPID - aExtDesc->mExtFstPID;
        sPrvLfBMP = aExtDesc->mExtFstPID + aAftInfo->mPageCnt[SDPST_EXTDIR];

        for ( sCurPID = sFstDataPID; sCurPID <= sLstDataPID; sCurPID++ )
        {
            /* 몇번째 LfBMP에 위치하는지 계산한다. */
            sLfBMPIdx = (sCurPID - aExtDesc->mExtFstPID) /
                        aAftInfo->mPageRange;
            sLfBMP    = aExtDesc->mExtFstPID +
                        aAftInfo->mPageCnt[SDPST_EXTDIR] +
                        sLfBMPIdx;

            if ( sPrvLfBMP != sLfBMP )
            {
                /* BUG-42820 leaf 에서 page range 기준으로 이미 계산되어 있다.
                 * leaf node에서와 동일하게 sPBSNo를 다시 계산한다. */
                sPBSNo = ( sCurPID - aExtDesc->mExtFstPID ) % aAftInfo->mPageRange;
            }

            sState = 0;
            IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                           &sMtx,
                                           aStartInfo,
                                           ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                           SM_DLOG_ATTR_DEFAULT )
                      != IDE_SUCCESS );
            sState = 1;

            IDE_TEST( sdpstAllocPage::createPage( aStatistics,
                                                  &sMtx,
                                                  aSpaceID,
                                                  aSegHandle,
                                                  sCurPID,
                                                  sDataSeqNo,
                                                  SDP_PAGE_FORMAT,
                                                  sLfBMP,
                                                  sPBSNo,
                                                  sCreatePBS,
                                                  &sPagePtr ) != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

            sPBSNo++;
            sPrvLfBMP = sLfBMP;
            sDataSeqNo++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 이전 Lst Bitmap 페이지에 새로 생성한 BMP 페이지들의
 *               정보 기록 이전 bmp 페이지에 기록하는 것으로 DDL 실패시
 *               트랜잭션 Undo가 필요하다.
 *
 * aStatistics   - [IN] 통계정보
 * aMtx          - [IN] Mtx의 Pointer
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegHandle    - [IN] Segment의 Handle
 * aExtDesc      - [IN] Extent Desc Pointer
 * aBfrInfo      - [IN] Extent 이전 할당 정보
 * aAftInfo      - [IN] Extent 이후 할당 정보
 ***********************************************************************/
IDE_RC sdpstSegDDL::linkNewExtToLstBMPs( idvSQL                * aStatistics,
                                         sdrMtx                * aMtx,
                                         scSpaceID               aSpaceID,
                                         sdpSegHandle          * aSegHandle,
                                         sdpstExtDesc          * aExtDesc,
                                         sdpstBfrAllocExtInfo  * aBfrInfo,
                                         sdpstAftAllocExtInfo  * aAftInfo )
{
    UChar             * sPagePtr;
    sdpstLfBMPHdr     * sLfBMPHdr;
    sdpstBMPHdr       * sBMPHdr;
    scPageID            sFstPID;
    scPageID            sLstPID;
    UShort              sFullLfBMPCnt;
    UShort              sFullItBMPCnt;
    idBool              sNeedToChangeMFNL;
    sdpstMFNL           sNewMFNL;
    sdpstStack          sRevStack;

    IDE_ASSERT( aSegHandle    != NULL );
    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aExtDesc      != NULL );
    IDE_ASSERT( aBfrInfo != NULL );

    /* lf-bmp 페이지를 생성하지 않고, 이전 lf-bmp에 추가하는 경우 */
    if ( aAftInfo->mPageCnt[SDPST_LFBMP] == 0 )
    {
        /* lf-bmp 페이지를 생성하지 않을때에는 last lf-bmp에
         * Range slot에 넣는다. */
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              aBfrInfo->mLstPID[SDPST_LFBMP],
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sPagePtr,
                                              NULL, /* aTrySuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );

        sLfBMPHdr = sdpstLfBMP::getHdrPtr( sPagePtr );

        if ( aAftInfo->mPageCnt[SDPST_EXTDIR] > 0 )
        {
            /* 이후에 ExtDir 페이지를 생성될것이고, 여기서 미리 ExtDir
             * 에 대한 처리를 수행한다.
             * Lf-BMP 페이지의 대표 MFNL은 변경될 수 없다. 왜냐하면,
             * Lf-BMP 새로할당하지 않았고, 기존 Lf-BMP에 추가했기 때문에
             * 기존 Lf-BMP 페이지의 MFNL은 적어도 UNF상태이기 때문이다. */
            IDE_ASSERT( aAftInfo->mLfBMP4ExtDir ==
                        aBfrInfo->mLstPID[SDPST_LFBMP] );
        }

        /* lf-bmp 페이지에 Extent를 Range Slot에 기록한다. */
        IDE_TEST( sdpstLfBMP::logAndAddPageRangeSlot(
                      aMtx,
                      sLfBMPHdr,
                      aExtDesc->mExtFstPID,
                      aExtDesc->mLength,
                      aAftInfo->mPageCnt[SDPST_EXTDIR],
                      aBfrInfo->mLstPID[SDPST_EXTDIR],
                      aBfrInfo->mSlotNoInExtDir,
                      &sNeedToChangeMFNL,
                      &sNewMFNL ) != IDE_SUCCESS );

        /* ExtDesc이 이전 Lf bmp에 Range Slot을 추가되는 경우이므로,
         * ExtDesc을 관리하는 lf-bmp는 last lf-bmp이고, 첫번째 data
         * page는 ExtDir 페이지를 고려해서 결정해야한다. */

        /* Extent를 할당하면, 우선적으로 Page들이 위치하게 되고, 이후에
         * Data 페이지 용도로 할당되게 된다. 그러므로, 첫번째 lf-bmp의
         * 페이지의 PID만 ExtDesc에 기록해두면 Sequential Scan시에 다음
         * lf-bmp를 참고하고자 할때, 바로 다음 페이지에 접근해보면 된다. */
        aExtDesc->mExtMgmtLfBMP  = aBfrInfo->mLstPID[SDPST_LFBMP];
        aExtDesc->mExtFstDataPID = aExtDesc->mExtFstPID +
                                   aAftInfo->mPageCnt[SDPST_EXTDIR];

        // 마지막 lf-bmp 페이지에 parent it-bmp는 last it-bmp이다.
        IDE_ASSERT( aBfrInfo->mLstPID[SDPST_ITBMP] ==
                    sLfBMPHdr->mBMPHdr.mParentInfo.mParentPID);

        if ( sNeedToChangeMFNL == ID_TRUE )
        {
            /* ItBMP 페이지의 lf-slot MFNL을 변경시도하여 상위노드로
             * 전파한다. 왜냐하면, leaf bmp의 MFNL이 변경되었기 때문이다. */
            sdpstStackMgr::initialize( &sRevStack );
            IDE_TEST( sdpstAllocPage::tryToChangeMFNLAndItHint(
                          aStatistics,
                          aMtx,
                          aSpaceID,
                          aSegHandle,
                          SDPST_CHANGEMFNL_ITBMP_PHASE,
                          aBfrInfo->mLstPID[SDPST_LFBMP],  // child pid
                          SDPST_INVALID_PBSNO,             // PBSNo for lf
                          (void*)&sNewMFNL,
                          aBfrInfo->mLstPID[SDPST_ITBMP],
                          sLfBMPHdr->mBMPHdr.mParentInfo.mIdxInParent,
                          &sRevStack ) != IDE_SUCCESS );
        }
    }
    else
    {
        /* 이전 it-bmp 페이지에 새로 생성한 lf-bmp 페이지들을 기록한다.
         * 이전 it-bmp에 기록하므로 undo에 대한 고려를 해야한다. */
        if ( (aAftInfo->mPageCnt[SDPST_LFBMP] > 0) &&
             (aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] > 0) )
        {
            // 이전 it-bmp 페이지에 기록될 첫번째 lf-bmp 페이지의 PID
            sFstPID = aExtDesc->mExtFstPID + aAftInfo->mPageCnt[SDPST_EXTDIR];

            // 이전 it-bmp 페이지에 기록될 마지막 lf-bmp 페이지의 PID
            sLstPID = sFstPID +
                (aAftInfo->mPageCnt[SDPST_LFBMP] >=
                    aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] ?
                 aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] :
                 aAftInfo->mPageCnt[SDPST_LFBMP]) - 1;

            /* LfBMP가 MFNL이 처음부터 FULL인경우가 있다.
             * ( Lfbmp 페이지에 모두 page만 있는 경우 ) */
            sFullLfBMPCnt = (aAftInfo->mFullBMPCnt[SDPST_LFBMP] <=
                                aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] ?
                             aAftInfo->mFullBMPCnt[SDPST_LFBMP] :
                             aBfrInfo->mFreeSlotCnt[SDPST_ITBMP]);

            IDE_TEST( sdbBufferMgr::getPageByPID(
                                            aStatistics,
                                            aSpaceID,
                                            aBfrInfo->mLstPID[SDPST_ITBMP],
                                            SDB_X_LATCH,
                                            SDB_WAIT_NORMAL,
                                            SDB_SINGLE_PAGE_READ,
                                            aMtx,
                                            &sPagePtr,
                                            NULL, /* aTrySuccess */
                                            NULL  /* aIsCorruptPage */ )
                      != IDE_SUCCESS );

            sBMPHdr = sdpstBMP::getHdrPtr( sPagePtr );

            /* 기존 마지막 ItBMP 페이지에 새로운 slot을 기록한다. */
            IDE_TEST( sdpstBMP::logAndAddSlots( aMtx,
                                                sBMPHdr,
                                                sFstPID,
                                                sLstPID,
                                                sFullLfBMPCnt,
                                                &sNeedToChangeMFNL,
                                                &sNewMFNL ) != IDE_SUCCESS );

            if ( sNeedToChangeMFNL == ID_TRUE )
            {
                /* RtBMP페이지의 it-slot MFNL을 변경시도하여 상위
                 * 노드로 전파한다. */
                sdpstStackMgr::initialize( &sRevStack );

                IDE_TEST( sdpstAllocPage::tryToChangeMFNLAndItHint(
                              aStatistics,
                              aMtx,
                              aSpaceID,
                              aSegHandle,
                              SDPST_CHANGEMFNL_RTBMP_PHASE,
                              aBfrInfo->mLstPID[SDPST_ITBMP], // child pid
                              SDPST_INVALID_PBSNO,            // unuse
                              (void*)&sNewMFNL,
                              sBMPHdr->mParentInfo.mParentPID,
                              sBMPHdr->mParentInfo.mIdxInParent,
                              &sRevStack ) != IDE_SUCCESS );
            }
        }

        /* 이전 rt-bmp 페이지에 새로 생성한 it-bmp 페이지들을 기록한다.
         * 이전 rt-bmp에 기록하므로 undo에 대한 고려를 해야한다. */
        if ( (aAftInfo->mPageCnt[SDPST_ITBMP] > 0) &&
             (aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] > 0) )
        {
            // 이전 rt-bmp 페이지에 기록될 첫번째 it-bmp 페이지의 PID
            sFstPID = aExtDesc->mExtFstPID + aAftInfo->mPageCnt[SDPST_EXTDIR] +
                      aAftInfo->mPageCnt[SDPST_LFBMP];

            // 이전 rt-bmp 페이지에 기록될 마지막 it-bmp 페이지의 PID
            sLstPID = sFstPID +
                (aAftInfo->mPageCnt[SDPST_ITBMP] >=
                    aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] ?
                 aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] :
                 aAftInfo->mPageCnt[SDPST_ITBMP]) - 1;

            sFullItBMPCnt =
                (aAftInfo->mFullBMPCnt[SDPST_ITBMP] <=
                    aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] ?
                 aAftInfo->mFullBMPCnt[SDPST_ITBMP] :
                 aBfrInfo->mFreeSlotCnt[SDPST_RTBMP]);

            IDE_TEST( sdbBufferMgr::getPageByPID(
                                            aStatistics,
                                            aSpaceID,
                                            aBfrInfo->mLstPID[SDPST_RTBMP],
                                            SDB_X_LATCH,
                                            SDB_WAIT_NORMAL,
                                            SDB_SINGLE_PAGE_READ,
                                            aMtx,
                                            &sPagePtr,
                                            NULL, /* aTrySuccess */
                                            NULL  /* aIsCorruptPage */ )
                      != IDE_SUCCESS );

            sBMPHdr = sdpstBMP::getHdrPtr( sPagePtr );

            /* RtBMP 페이지에 slot을 기록한다.
             * freeslots와 free slotno 까지 조정해준다. */
            IDE_TEST( sdpstBMP::logAndAddSlots( aMtx,
                                                sBMPHdr,
                                                sFstPID,
                                                sLstPID,
                                                sFullItBMPCnt,
                                                &sNeedToChangeMFNL,
                                                &sNewMFNL ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 새로운 하나의 Extent를 Segment에 할당하는 연산을 완료
 *
 * Segment Header 페이지에 last bmps 와 total 페이지 개수
 * Tot Extent 개수 그리고, extent slot을 기록한다.
 *
 * aStatistics   - [IN] 통계정보
 * aMtx          - [IN] Mtx의 Pointer
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegPID       - [IN] Segment Header의 PID
 * aExtDesc      - [IN] Extent Slot Pointer
 * aBfrInfo      - [IN] Extent 이전 할당 정보
 * aAftInfo      - [IN] Extent 이후 할당 정보
 * aAllocExtRID  - [IN] 할달된 Extent의 RID
 ***********************************************************************/
IDE_RC sdpstSegDDL::linkNewExtToExtDir( idvSQL                * aStatistics,
                                        sdrMtx                * aMtx,
                                        scSpaceID               aSpaceID,
                                        scPageID                aSegPID,
                                        sdpstExtDesc          * aExtDesc,
                                        sdpstBfrAllocExtInfo  * aBfrInfo,
                                        sdpstAftAllocExtInfo  * aAftInfo,
                                        sdRID                 * aAllocExtRID )
{

    UChar              * sSegHdrPtr;
    UChar              * sLstExtDirPtr;
    scPageID             sLstExtDirPID;
    UShort               sFreeExtSlotCnt;
    sdpstSegHdr        * sSegHdr;
    sdpstExtDirHdr     * sExtDirHdr;

    IDE_ASSERT( aExtDesc != NULL );
    IDE_ASSERT( aAftInfo != NULL );
    IDE_ASSERT( aMtx     != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sSegHdrPtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sSegHdr       = sdpstSH::getHdrPtr( sSegHdrPtr );
    sLstExtDirPID = sdpstSH::getLstExtDir( sSegHdr );

    if ( sLstExtDirPID == aSegPID )
    {
        sExtDirHdr      = sdpstExtDir::getHdrPtr( sSegHdrPtr );
        sFreeExtSlotCnt = sdpstExtDir::getFreeSlotCnt( sExtDirHdr );
        sLstExtDirPtr   = NULL;
    }
    else
    {
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sLstExtDirPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sLstExtDirPtr,
                                              NULL, /* aTrySuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );

        sExtDirHdr      = sdpstExtDir::getHdrPtr( sLstExtDirPtr );
        sFreeExtSlotCnt = sdpstExtDir::getFreeSlotCnt( sExtDirHdr );
    }

    if ( sFreeExtSlotCnt == 0 )
    {
        IDE_ASSERT( aAftInfo->mPageCnt[SDPST_EXTDIR] > 0 );

        /* 만약, extent에 page가 생성되어 있을 수도 있으니까
         * 첫번째 data 페이지를 ExtDir 페이지로 생성하고,
         * 첫번재 data 페이지를 갱신한다. */
        IDE_TEST( sdpstExtDir::createAndInitPage(
                                        aStatistics,
                                        aMtx,
                                        aSpaceID,
                                        aExtDesc,
                                        aBfrInfo,
                                        aAftInfo,
                                        &sLstExtDirPtr ) != IDE_SUCCESS );

        /* Segment Header에 생성한 ExtDir 페이지를 연결한다.
         * sExtDirHdr 다음에 연결할 것이라 반드시 sExtDirHdr Page에 
         * X-Latch 잡혀있어야 한다. */
        IDE_TEST( sdpstSH::addNewExtDirPage( aMtx,
                                             sSegHdrPtr,
                                             sLstExtDirPtr ) != IDE_SUCCESS );

        sExtDirHdr = sdpstExtDir::getHdrPtr( sLstExtDirPtr );
    }

    /* 마지막 ExtDir에 기록한다. */
    IDE_TEST( sdpstExtDir::logAndAddExtDesc( aMtx,
                                             sExtDirHdr,
                                             aExtDesc,
                                             aAllocExtRID )
              != IDE_SUCCESS );


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 마지막 BMP들을 Segment Header에 연결한다.
 *
 *
 * aStatistics   - [IN] 통계정보
 * aMtx          - [IN] Mtx의 Pointer
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegPID       - [IN] Segment Header의 PID
 * aExtDesc      - [IN] ExtDesc Pointer
 * aBfrInfo      - [IN] Extent 이전 할당 정보
 * aAftInfo      - [IN] Extent 이후 할당 정보
 * aAllocExtRID  - [IN] 할당한 Extent의 RID
 * aNeedToUpdateHWM - [IN] HWM 갱신 여부
 * aSegCache     - [IN] HWM 갱신하기 위해 사용
 ***********************************************************************/
IDE_RC sdpstSegDDL::linkNewExtToSegHdr( idvSQL               * aStatistics,
                                        sdrMtx               * aMtx,
                                        scSpaceID              aSpaceID,
                                        scPageID               aSegPID,
                                        sdpstExtDesc         * aExtDesc,
                                        sdpstBfrAllocExtInfo * aBfrInfo,
                                        sdpstAftAllocExtInfo * aAftInfo,
                                        sdRID                  aAllocExtRID,
                                        idBool                 aNeedToUpdateHWM,
                                        sdpstSegCache        * aSegCache)
{
    UChar       * sPagePtr;
    sdpstSegHdr * sSegHdr;
    scPageID      sBfrLstRtBMP;
    sdpstStack    sHWMStack;
    SShort        sSlotNoInRtBMP;
    SShort        sSlotNoInItBMP;
    SShort        sPBSNo;
    ULong         sMetaPageCnt;


    IDE_ASSERT( aSegPID  != SD_NULL_PID );
    IDE_ASSERT( aAftInfo != NULL );

    sMetaPageCnt = aAftInfo->mPageCnt[SDPST_EXTDIR] +
                   aAftInfo->mPageCnt[SDPST_LFBMP] +
                   aAftInfo->mPageCnt[SDPST_ITBMP] +
                   aAftInfo->mPageCnt[SDPST_RTBMP] +
                   aAftInfo->mSegHdrCnt;

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sSegHdr = sdpstSH::getHdrPtr( sPagePtr );

    /*
     * BUG-22474     [valgrind]sdbMPRMgr::getMPRCnt에 UMR있습니다.
     */
    IDE_TEST( sdpstSH::setLstExtRID( aMtx,
                                     sSegHdr,
                                     aAllocExtRID ) != IDE_SUCCESS );


    IDE_TEST( sdpstSH::logAndLinkBMPs( aMtx,
                                       sSegHdr,
                                       aAftInfo->mTotPageCnt -
                                           aBfrInfo->mTotPageCnt,
                                       sMetaPageCnt,
                                       aAftInfo->mPageCnt[SDPST_RTBMP],
                                       aAftInfo->mLstPID[SDPST_RTBMP],
                                       aAftInfo->mLstPID[SDPST_ITBMP],
                                       aAftInfo->mLstPID[SDPST_LFBMP],
                                       &sBfrLstRtBMP )
              != IDE_SUCCESS );

    /* SegCache에 format page count 갱신 */
    aSegCache->mFmtPageCnt = aAftInfo->mTotPageCnt;

    /* RtBMP 가 생성되었다면 마지막 RtBMP의 NxtRtBMP를 설정해준다. */
    if ( aAftInfo->mPageCnt[SDPST_RTBMP] > 0 )
    {
        IDE_TEST( sdpstRtBMP::setNxtRtBMP( aStatistics,
                                           aMtx,
                                           aSpaceID,
                                           sBfrLstRtBMP,
                                           aAftInfo->mFstPID[SDPST_RTBMP] )
                != IDE_SUCCESS );
    }

    /* HWM을 설정하기 위한 SlotNo를 계산한다. */
    if ( aNeedToUpdateHWM == ID_TRUE )
    {
        if ( aAftInfo->mPageCnt[SDPST_RTBMP] > 0 )
        {
            sSlotNoInRtBMP = aAftInfo->mPageCnt[SDPST_ITBMP] -
                             aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] - 1;
        }
        else
        {
            sSlotNoInRtBMP = ( ( aBfrInfo->mMaxSlotCnt[SDPST_RTBMP] -
                                 aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] ) +
                               aAftInfo->mPageCnt[SDPST_ITBMP] ) - 1;
        }

        if ( aAftInfo->mPageCnt[SDPST_ITBMP] > 0 )
        {
            sSlotNoInItBMP = aAftInfo->mPageCnt[SDPST_LFBMP] - 
                             aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] - 1;
        }
        else
        {
            sSlotNoInItBMP = ( ( aBfrInfo->mMaxSlotCnt[SDPST_ITBMP] -
                                 aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] ) +
                               aAftInfo->mPageCnt[SDPST_LFBMP] ) - 1;
        }

        if ( aAftInfo->mPageCnt[SDPST_LFBMP] > 0 )
        {
            sPBSNo = (aExtDesc->mLength - 1) % aAftInfo->mPageRange;
        }
        else
        {
            sPBSNo = aBfrInfo->mLstPBSNo + aExtDesc->mLength;
        }

        sdpstStackMgr::initialize( &sHWMStack );
        sdpstStackMgr::push( &sHWMStack, SD_NULL_PID, sSegHdr->mTotRtBMPCnt );
        sdpstStackMgr::push( &sHWMStack, sSegHdr->mLstRtBMP, sSlotNoInRtBMP );
        sdpstStackMgr::push( &sHWMStack, sSegHdr->mLstItBMP, sSlotNoInItBMP );
        sdpstStackMgr::push( &sHWMStack, sSegHdr->mLstLfBMP, sPBSNo );

        IDE_TEST( sdpstSH::logAndUpdateWM( aMtx,
                                           &sSegHdr->mHWM,
                                           aExtDesc->mExtFstPID +
                                           aExtDesc->mLength - 1,
                                           aAftInfo->mLstPID[SDPST_EXTDIR],
                                           aAftInfo->mSlotNoInExtDir,
                                           &sHWMStack ) != IDE_SUCCESS );
        sdpstCache::lockHWM4Write( aStatistics, aSegCache );
        aSegCache->mHWM = sSegHdr->mHWM;
        sdpstCache::unlockHWM( aSegCache );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : [INTERFACE] Segment에 1개이상의 Extent를 할당한다
 *
 * aStatistics  - [IN] 통계정보
 * aStartInfo   - [IN] Mtx의 StartInfo
 * aSpaceID     - [IN] Tablespace의 ID
 * aSegHandle   - [IN] Segment의 Handle
 * aExtCnt      - [IN] 할당할 Extent 개수
 ***********************************************************************/
IDE_RC sdpstSegDDL::allocateExtents( idvSQL           * aStatistics,
                                     sdrMtxStartInfo  * aStartInfo,
                                     scSpaceID          aSpaceID,
                                     sdpSegHandle     * aSegHandle,
                                     UInt               aExtCnt )
{
    sdRID   sAllocFstExtRID;

    IDE_ASSERT( aStartInfo  != NULL );
    IDE_ASSERT( aExtCnt > 0 );
    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aSegHandle->mSegPID != SD_NULL_PID );

    return allocNewExtsAndPage( aStatistics,
                                aStartInfo,
                                aSpaceID,
                                aSegHandle,
                                aExtCnt,
                                ID_TRUE, /* sNeedToFormatPage */
                                &sAllocFstExtRID,
                                NULL,    /* aMtx */
                                NULL     /* aFstDataPage */ );
}



/***********************************************************************
 * Description : Segment에 1개이상의 Extent를 할당한다.
 *               # DML/DDL
 *               Segment에 연결된 할당된 Extent마다 NTA logging 하여
 *               Undo되지 않도록 처리한다. 연결을 일단하게 되면,
 *               다른 DML 트랜잭션이 확장된 공간을 사용할 수 있기 때문에
 *               Undo 되어서는 안된다.
 *
 * aStatistics        - [IN]  통계정보
 * aStartInfo         - [IN]  Mtx 시작정보
 * aSpaceID           - [IN]  Segment의 테이블스페이스
 * aSegHandle         - [IN]  Segment Handle
 * aExtCnt            - [IN]  할당할 Extent 개수
 * aNeedToFormatPage  - [IN]  Extent 할당 후 HWM 갱신여부 플래그
 * aAllocFstExtRID    - [OUT] 할당받은 첫번째 Extent RID
 *                         중요: 이 output 변수는 DPath에서만 사용된다.
 *                              만약 다른 용도로 여러 트랜잭션에서 동시에
 *                              사용하면, 동시성 문제가 있다.
 *                              따라서 주의해서 사용해야 한다.
 * aMtx4Logging       - [OUT]
 * aFstDataPagePtr    - [OUT]
 ***********************************************************************/
IDE_RC sdpstSegDDL::allocNewExtsAndPage( idvSQL           * aStatistics,
                                         sdrMtxStartInfo  * aStartInfo,
                                         scSpaceID          aSpaceID,
                                         sdpSegHandle     * aSegHandle,
                                         UInt               aExtCnt,
                                         idBool             aNeedToFormatPage,
                                         sdRID            * aAllocFstExtRID,
                                         sdrMtx           * aMtx,
                                         UChar           ** aFstDataPage )
{
    sdrMtx                 sMtx;
    UInt                   sLoop;
    UInt                   sState = 0;
    sdpstExtDesc           sExtDesc;
    smLSN                  sOpNTA;
    sdpstBfrAllocExtInfo   sBfrAllocExtInfo;
    sdpstAftAllocExtInfo   sAftAllocExtInfo;
    scPageID               sSegPID;
    idBool                 sIsExtendExt;
    idBool                 sNeedToUpdateHWM = ID_FALSE;
    idBool                 sNeedToExtendExt;
    sdpstSegCache        * sSegCache;
    sdRID                  sAllocExtRID;
    ULong                  sData[ SM_DISK_NTALOG_DATA_COUNT ];

    IDE_DASSERT( aExtCnt              > 0 );
    IDE_DASSERT( aStartInfo          != NULL );
    IDE_DASSERT( aSegHandle          != NULL );
    IDE_DASSERT( aSegHandle->mSegPID != SD_NULL_PID );
    IDE_DASSERT( aAllocFstExtRID     != NULL );

    sSegPID      = aSegHandle->mSegPID;
    sSegCache    = (sdpstSegCache*)(aSegHandle->mCache);
    sIsExtendExt = ID_FALSE;

    if ( aFstDataPage != NULL )
    {
        *aFstDataPage = NULL;
    }

    // 다른 트랜잭션과의 동시성 제어를 위해 획득
    // 동시에 확장요청에 대해서 중복 확장하지 않고, 뒤늦게 들어온 트랜잭션은
    // 앞서 확장된 Extent를 사용할 수 있도록 한다.
    IDE_ASSERT( sdpstCache::prepareExtendExtOrWait( aStatistics,
                                                    sSegCache,
                                                    &sIsExtendExt )
                == IDE_SUCCESS );

    IDE_TEST_CONT( sIsExtendExt == ID_FALSE, already_extended );
    sState = 1;

    if ( aNeedToFormatPage == ID_TRUE )
    {
        IDE_TEST( tryToFormatPageAfterHWM( aStatistics,
                                           aStartInfo,
                                           aSpaceID,
                                           aSegHandle,
                                           aAllocFstExtRID,
                                           aMtx,
                                           aFstDataPage,
                                           &sNeedToExtendExt ) != IDE_SUCCESS );

        IDE_TEST_CONT( sNeedToExtendExt == ID_FALSE, skip_extend );
    }

    // Extent들에 대한 bmp 페이지들을 생성한다.
    for( sLoop = 0; sLoop < aExtCnt; sLoop++ )
    {
        sdpstExtDir::initExtDesc( &sExtDesc );
        sdpstAllocExtInfo::initialize( &sBfrAllocExtInfo );
        sdpstAllocExtInfo::initialize( &sAftAllocExtInfo );

        if ( aStartInfo->mTrans != NULL )
        {
            sOpNTA = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );
        }
        else
        {
             /* Temporary Table 생성시에는 트랜잭션이 NULL이
              * 내려올 수 있다. */
        }

        // A. TBS로부터 Extent를 할당한다.
        // 각 Extent 할당은 local group 단위로 op NTA 처리를 한다.
        // Op NTA - alloc extents from TBS
        IDE_TEST( sdptbExtent::allocExts( aStatistics,
                                          aStartInfo,
                                          aSpaceID,
                                          1,
                                          (sdpExtDesc*)&sExtDesc )
                != IDE_SUCCESS );

        /* mSegHdrCnt는 0 또는 1 값만 갖고, 이미 SegHdr가 존재하는 경우 항상 0
         * 이어야 한다. */
        IDE_ASSERT( sAftAllocExtInfo.mSegHdrCnt == 0 );


        /* Segment Header로부터 BMPs 정보를 판독한다. */
        IDE_TEST( sdpstAllocExtInfo::getAllocExtInfo(
                                        aStatistics,
                                        aSpaceID,
                                        sSegPID,
                                        aSegHandle->mSegStoAttr.mMaxExtCnt,
                                        &sExtDesc,
                                        &sBfrAllocExtInfo,
                                        &sAftAllocExtInfo ) != IDE_SUCCESS );

        /* 새로운 Extent에 대한 Bitmap Page들을 생성한다.
         * aSegType을 SDP_SEG_TYPE_NONE으로 넘겨주는데, allocNewExts 에서는
         * Segment Header를 생성하지 않기 때문에 aSegType 이 의미없다. */
        IDE_ASSERT( sAftAllocExtInfo.mSegHdrCnt == 0 );
        IDE_TEST( createMetaPages( aStatistics,
                                   aStartInfo,
                                   aSpaceID,
                                   sSegCache,
                                   &sExtDesc,
                                   &sBfrAllocExtInfo,
                                   &sAftAllocExtInfo,
                                   NULL ) != IDE_SUCCESS );

        /* BUG-29299 TC/Server/sm4/Bugs/BUG-28723/recovery.sql
         *           수행시 기존 insert된 데이터가 삭제됨
         * 
         * DPath 수행시 aNeedToFormatPage 플래가가 TRUE로 넘어오는데,
         * 이 경우 Data Page에 대해서 format을 수행하지 않는다. */
        if ( aNeedToFormatPage == ID_TRUE )
        {
            IDE_TEST( createDataPages( aStatistics,
                                       aStartInfo,
                                       aSpaceID,
                                       aSegHandle,
                                       &sExtDesc,
                                       &sBfrAllocExtInfo,
                                       &sAftAllocExtInfo ) != IDE_SUCCESS );
            sNeedToUpdateHWM = ID_TRUE;
        }

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aStartInfo,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
        sState = 2;

        /* Extent 단위로 Segment Header 페이지의 ExtDir 페이지에
         * 기록하고, Segment Header 페이지를 설정한다.
         * 이전 Lst BMP 페이지에 새로 생성한 bmp 페이지들의 정보를 기록한다. */
        IDE_TEST( linkNewExtToLstBMPs( aStatistics,
                                       &sMtx,
                                       aSpaceID,
                                       aSegHandle,
                                       &sExtDesc,
                                       &sBfrAllocExtInfo,
                                       &sAftAllocExtInfo ) != IDE_SUCCESS );

        /* Extent 단위로 Segment Header 페이지의 ExtDir 페이지에
         * 기록하고, Segment Header 페이지를 설정한다. */
        IDE_TEST( linkNewExtToExtDir( aStatistics,
                                      &sMtx,
                                      aSpaceID,
                                      sSegPID,
                                      &sExtDesc,
                                      &sBfrAllocExtInfo,
                                      &sAftAllocExtInfo,
                                      &sAllocExtRID ) != IDE_SUCCESS );

        /* Extent 할당 마지막 완료작업으로 Segment Header에
         * BMP 페이지를 연결한다. */
        IDE_TEST( linkNewExtToSegHdr( aStatistics,
                                      &sMtx,
                                      aSpaceID,
                                      sSegPID,
                                      &sExtDesc,
                                      &sBfrAllocExtInfo,
                                      &sAftAllocExtInfo,
                                      sAllocExtRID,
                                      sNeedToUpdateHWM,
                                      sSegCache ) != IDE_SUCCESS );

        // OnDemand 시에는 Undo 되지 못하도록 모두 NTA 처리한다.
        sData[0] = (ULong)sSegPID;
        sdrMiniTrans::setNTA( &sMtx,
                              aSpaceID,
                              SDR_OP_NULL,
                              &sOpNTA,
                              &sData[0],
                              1 /* Data Cnt */ );

        if ( sLoop == 0 )
        {
            *aAllocFstExtRID = sAllocExtRID;

            /*
             * [BUG-29137] [SM] [PROJ-2037] sdpstFindPage::tryToAllocExtsAndPage에서
             *             동시에 페이지 할당이 발생할수 있습니다.
             */
            if( aFstDataPage != NULL )
            {
                IDE_ASSERT( aMtx != NULL );
                IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                      aSpaceID,
                                                      sExtDesc.mExtFstDataPID,
                                                      SDB_X_LATCH,
                                                      SDB_WAIT_NORMAL,
                                                      SDB_SINGLE_PAGE_READ,
                                                      aMtx,
                                                      aFstDataPage,
                                                      NULL, /* aTrySuccess */
                                                      NULL  /* aIsCorruptPage */ )
                          != IDE_SUCCESS );
            }
        }


        sState = 1;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    }

    IDE_EXCEPTION_CONT( skip_extend );


    // 확장완료후에 ExtendExt Mutex를 해제하면서 대기하는 트랜잭션을 깨운다.
    sState = 0;
    IDE_ASSERT( sdpstCache::completeExtendExtAndWakeUp(
                                            aStatistics,
                                            sSegCache ) == IDE_SUCCESS );

    IDE_EXCEPTION_CONT( already_extended );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* Segment Cache의 ExtendExt Mutex를 획득한 상태에서 트랜잭션이
     * 트랜잭션이 Extent를 2개이상을 확장할때, 1개를 할당하여
     * Segment 연결한 순간에 다른 트랜잭션이 그 Extent를 보게된다.
     * 그리고 사용해 버렸고, 그 이후에 확장연산이 Undo가 된다면,
     * 다른 트랜잭션이 사용한 페이지는 반환되어 버린다.
     *
     * 위와 같은 문제를 방지하기 위해서는 이미 확장중간에 Segment
     * 에 연결된 Extent는 트랜잭션이 그 이후로 실패하더라도 Undo하지
     * 않는다. */
    switch ( sState )
    {
        case 2:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdpstCache::completeExtendExtAndWakeUp(
                            aStatistics,
                            sSegCache ) == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC sdpstSegDDL::tryToFormatPageAfterHWM(
                                    idvSQL           * aStatistics,
                                    sdrMtxStartInfo  * aStartInfo,
                                    scSpaceID          aSpaceID,
                                    sdpSegHandle     * aSegHandle,
                                    sdRID            * aFstExtRID,
                                    sdrMtx           * aMtx,
                                    UChar           ** aFstDataPage,
                                    idBool           * aNeedToExtendExt )
{
    UChar           * sPagePtr;
    sdpstSegHdr     * sSegHdr;
    sdpstSegCache   * sSegCache;
    sdRID             sHWMExtRID;
    sdRID             sFmtExtRID;
    sdpstStack        sHWMStack;
    sdrMtx            sMtx;
    scPageID          sFstFmtPID;
    scPageID          sLstFmtPID;
    scPageID          sLstFmtExtDirPID;
    SShort            sLstFmtSlotNoInExtDir;
    UInt              sState = 0;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aFstExtRID != NULL );
    IDE_ASSERT( aNeedToExtendExt != NULL );

    *aNeedToExtendExt = ID_FALSE;

    sSegCache = (sdpstSegCache*)aSegHandle->mCache;

    IDE_ASSERT( sSegCache != NULL );

    /* HWM을 포함하는 Extent의 RID를 구한다. */
    IDE_TEST( sdpstExtDir::makeExtRID( aStatistics,
                                       aSpaceID,
                                       sSegCache->mHWM.mExtDirPID,
                                       sSegCache->mHWM.mSlotNoInExtDir,
                                       &sHWMExtRID ) != IDE_SUCCESS );

    /* HWM을 포함하는 Extent의 다음 Extent를 가져온다. */
    IDE_TEST( sdpstExtDir::getNxtExtRID( aStatistics,
                                         aSpaceID,
                                         aSegHandle->mSegPID,
                                         sHWMExtRID,
                                         &sFmtExtRID ) != IDE_SUCCESS );

    /* 다음 Extent가 없으면 확장해야 한다. */
    if ( sFmtExtRID == SD_NULL_RID )
    {
        *aNeedToExtendExt = ID_TRUE;
        IDE_CONT( skip_format_page );
    }

    *aFstExtRID = sFmtExtRID;

    /* 다음 Extent부터 해당 Extent를 관리하는 LfBMP에 속한 모든 Extent를
     * format 한다. */
    IDE_TEST( sdpstExtDir::formatPageUntilNxtLfBMP( aStatistics, 
                                                    aStartInfo,
                                                    aSpaceID,
                                                    aSegHandle,
                                                    sFmtExtRID,
                                                    &sFstFmtPID,
                                                    &sLstFmtPID,
                                                    &sLstFmtExtDirPID,
                                                    &sLstFmtSlotNoInExtDir )
              != IDE_SUCCESS );

    IDE_ASSERT( sFstFmtPID != SD_NULL_PID );
    IDE_ASSERT( sLstFmtPID != SD_NULL_PID );
    IDE_ASSERT( sLstFmtExtDirPID      != SD_NULL_PID );
    IDE_ASSERT( sLstFmtSlotNoInExtDir != SDPST_INVALID_SLOTNO );

    /* 첫번째 페이지는 X-Latch 잡아서 바로 할당해준다. */
    if ( aFstDataPage != NULL )
    {
        IDE_ASSERT( aMtx != NULL );

        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sFstFmtPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              aFstDataPage,
                                              NULL, /* aTruSuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );
    }

    /* 마지막 페이지는 HWM 갱신을 위해 사용한다. */
    sdpstStackMgr::initialize( &sHWMStack );
    IDE_TEST( sdpstAllocPage::makeOrderedStackFromDataPage(
                                                aStatistics,
                                                aSpaceID,
                                                aSegHandle->mSegPID,
                                                sLstFmtPID,
                                                &sHWMStack ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
    sState = 2;

    /* HWM을 갱신한다. */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegHandle->mSegPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          &sMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
             != IDE_SUCCESS );

    sSegHdr = sdpstSH::getHdrPtr( sPagePtr );

    IDE_TEST( sdpstSH::logAndUpdateWM( &sMtx,
                                       &sSegHdr->mHWM,
                                       sLstFmtPID,
                                       sLstFmtExtDirPID,
                                       sLstFmtSlotNoInExtDir,
                                       &sHWMStack ) != IDE_SUCCESS );

    sdpstCache::lockHWM4Write( aStatistics, sSegCache );
    sSegCache->mHWM = sSegHdr->mHWM;
    sdpstCache::unlockHWM( sSegCache );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_format_page );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                    == IDE_SUCCESS );
    }

    if ( sState == 2 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;


}

/***********************************************************************
 * Description : Segment의 첫번재 Extent를 제외한 모든 Extent를 TBS에
 *               반환한다.
 *
 * Caution     : 이 함수가 Return될때 TBS Header에 X-Latch가 걸려있다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegHdr      - [IN] Segment Header
 ***********************************************************************/
IDE_RC sdpstSegDDL::freeExtsExceptFst( idvSQL       * aStatistics,
                                       sdrMtx       * aMtx,
                                       scSpaceID      aSpaceID,
                                       sdpstSegHdr  * aSegHdr )
{
    sdrMtx               sFreeMtx;
    ULong                sExtDirCount;
    sdrMtxStartInfo      sStartInfo;
    UChar              * sPagePtr;
    sdpstExtDirHdr     * sExtDirHdr;
    scPageID             sCurExtDirPID;
    scPageID             sPrvExtDirPID;
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
    sExtDirCount = sdpstSH::getTotExtDirCnt( aSegHdr );
    sCurExtDirPID = sdpDblPIDList::getListTailNode( &aSegHdr->mExtDirBase );

    while( ( sExtDirCount > 1 ) || ( aSegHdr->mSegHdrPID != sCurExtDirPID ) )
    {
        sExtDirCount--;

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sFreeMtx,
                                       &sStartInfo,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
        sState = 1;

        /* Extent Dir Page의 First Extent를 제외한 모든 Extent를
         * Free한다. */
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sCurExtDirPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              &sFreeMtx,
                                              &sPagePtr,
                                              NULL, /* aTrySuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );

        sExtDirHdr          = sdpstExtDir::getHdrPtr( sPagePtr );

        sPhyHdrOfExtDirPage = sdpPhyPage::getHdr( (UChar*)sExtDirHdr );
        sPrvExtDirPID       = sdpPhyPage::getPrvPIDOfDblList( sPhyHdrOfExtDirPage );

        IDE_TEST( sdpstExtDir::freeAllExtExceptFst( aStatistics,
                                                    &sStartInfo,
                                                    aSpaceID,
                                                    aSegHdr,
                                                    sExtDirHdr )
                  != IDE_SUCCESS );

        /* Extent Dir Page의 마지막 남은 Fst Extent를 Free하고 Fst Extent
         * 에 속해 있는 Extent Dir Page를 리스트에서 제거한다. 이 두연산은
         * 하나의 Mini Transaction으로 묶어야 한다. 왜냐면 Extent가 Free시
         * ExtDirPage가 free되기 때문에 이 페이지가 ExtDirPage List에서
         * 제거되어야 한다. */
        IDE_TEST( sdpstExtDir::freeLstExt( aStatistics,
                                           &sFreeMtx,
                                           aSpaceID,
                                           aSegHdr,
                                           sExtDirHdr ) != IDE_SUCCESS );

        IDE_TEST( sdpDblPIDList::removeNode( aStatistics,
                                             &aSegHdr->mExtDirBase,
                                             sdpPhyPage::getDblPIDListNode(
                                                        sPhyHdrOfExtDirPage ),
                                             &sFreeMtx ) != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sFreeMtx ) != IDE_SUCCESS );

        sCurExtDirPID = sPrvExtDirPID;
    }

    /* SegHdr의 ExtDir만 남는다. */
    IDE_ASSERT( sCurExtDirPID == aSegHdr->mSegHdrPID );

    IDE_TEST( sdpstExtDir::freeAllExtExceptFst( aStatistics,
                                                &sStartInfo,
                                                aSpaceID,
                                                aSegHdr,
                                                &aSegHdr->mExtDirHdr )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sFreeMtx ) == IDE_SUCCESS );
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
IDE_RC sdpstSegDDL::freeAllExts( idvSQL         * aStatistics,
                                 sdrMtx         * aMtx,
                                 scSpaceID        aSpaceID,
                                 sdpstSegHdr    * aSegHdr )
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

    IDE_ASSERT( sdpstSH::getTotExtDirCnt( aSegHdr ) > 0 );
    
    IDE_TEST( freeExtsExceptFst( aStatistics,
                                 aMtx,
                                 aSpaceID,
                                 aSegHdr )
              != IDE_SUCCESS );


    sExtDirPIDLst = &aSegHdr->mExtDirBase;

    IDE_ASSERT( sdpstSH::getTotExtDirCnt( aSegHdr ) == 1 );

    IDE_TEST( sdpDblPIDList::initBaseNode( sExtDirPIDLst, aMtx )
              != IDE_SUCCESS );

    /* SegHdr가 포함된 ExtDir 만 남게 된다. */
    IDE_TEST( sdpstExtDir::freeLstExt( aStatistics,
                                       aMtx,
                                       aSpaceID,
                                       aSegHdr,
                                       &aSegHdr->mExtDirHdr )
              != IDE_SUCCESS );


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

