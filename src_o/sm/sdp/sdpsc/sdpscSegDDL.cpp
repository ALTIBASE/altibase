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
 * $Id: sdpscSegDDL.cpp 85837 2019-07-14 23:44:48Z emlee $
 *
 * 본 파일은 Circular-List Managed Segment의 Create/Drop/Alter/Reset 연산의
 * STATIC 인터페이스들을 관리한다.
 *
 ***********************************************************************/

# include <smErrorCode.h>
# include <sdr.h>
# include <sdpReq.h>

# include <sdpscDef.h>
# include <sdpscSegDDL.h>

# include <sdpscSH.h>
# include <sdpscED.h>
# include <sdpscCache.h>
# include <sdptbExtent.h>

/***********************************************************************
 *
 * Description : [ INTERFACE ] Segment 할당 및 Segment Header 초기화
 *
 * aStatistics - [IN]  통계정보
 * aSpaceID    - [IN]  Tablespace의 ID
 * aSegType    - [IN]  Segment의 Type
 * aMtx        - [IN]  Mini Transaction의 Pointer
 * aSegHandle  - [IN]  Segment의 Handle
 *
 ***********************************************************************/
IDE_RC sdpscSegDDL::createSegment( idvSQL                * aStatistics,
                                   sdrMtx                * aMtx,
                                   scSpaceID               aSpaceID,
                                   sdpSegType              aSegType,
                                   sdpSegHandle          * aSegHandle )
{
    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSegHandle != NULL );

    IDE_TEST_RAISE( !((aSegType == SDP_SEG_TYPE_UNDO) ||
                      (aSegType == SDP_SEG_TYPE_TSS)),
                    cannot_create_segment );

    IDE_TEST( allocateSegment( aStatistics,
                               aMtx,
                               aSpaceID,
                               aSegHandle,
                               aSegType ) != IDE_SUCCESS );

    IDE_ASSERT( aSegHandle->mSegPID != SD_NULL_PID );

    return IDE_SUCCESS;

    IDE_EXCEPTION( cannot_create_segment );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_CannotCreateSegInUndoTBS) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment를 할당한다.
 *
 * segment 생성시 초기 extent 개수를 할당한다. 만약, 생성하다가 부족하다면
 * exception 처리한다.
 *
 * aStatistics - [IN]  통계정보
 * aSpaceID    - [IN]  Tablespace의 ID
 * aSegHandle  - [IN]  Segment의 Handle
 * aSegType    - [IN]  Segment의 Type
 * aMtx        - [IN]  Mini Transaction의 Pointer
 *
 ***********************************************************************/
IDE_RC sdpscSegDDL::allocateSegment( idvSQL       * aStatistics,
                                     sdrMtx       * aMtx,
                                     scSpaceID      aSpaceID,
                                     sdpSegHandle * aSegHandle,
                                     sdpSegType     aSegType )
{
    sdrMtx               sMtx;
    sdrMtxStartInfo      sStartInfo;
    sdpscExtDirInfo      sCurExtDirInfo;
    sdpscExtDesc         sExtDesc;
    sdRID                sFstExtDescRID = SD_NULL_RID;
    UInt                 sState         = 0;
    UInt                 sTotExtDescCnt = 0;

    IDE_ASSERT( aMtx                != NULL );
    IDE_ASSERT( aSegHandle          != NULL );
    IDE_ASSERT( aSegHandle->mSegPID == SD_NULL_PID );

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    /* Undo Tablespace는 시스템에 의해서 자동으로 관리되므로
     * Segment Storage Parameter 들을 모두 무시한다 */

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sdpscExtDir::initExtDesc( &sExtDesc );

    IDE_TEST( sdptbExtent::allocExts( aStatistics,
                                      &sStartInfo,
                                      aSpaceID,
                                      1,       // Extent 개수
                                      (sdpExtDesc*)&sExtDesc )
              != IDE_SUCCESS );

    IDE_ASSERT( sExtDesc.mExtFstPID != SD_NULL_PID );

    aSegHandle->mSegPID       = sExtDesc.mExtFstPID;
    sCurExtDirInfo.mExtDirPID = sExtDesc.mExtFstPID;
    sCurExtDirInfo.mIsFull    = ID_FALSE;
    sCurExtDirInfo.mTotExtCnt = 0;
    sCurExtDirInfo.mMaxExtCnt = sdpscSegHdr::getMaxExtDescCnt( 
                       ((sdpscSegCache*)aSegHandle->mCache)->mCommon.mSegType );

    IDE_TEST( sdpscSegHdr::createAndInitPage(
                               aStatistics,
                               &sStartInfo,
                               aSpaceID,
                               &sExtDesc,
                               aSegType,
                               sCurExtDirInfo.mMaxExtCnt )
              != IDE_SUCCESS );

    // Extent 단위로 Segment Header 페이지의 ExtDir 페이지에
    // 기록하고, Segment Header 페이지를 설정한다.
    IDE_TEST( addAllocExtDesc( aStatistics,
                               &sMtx,
                               aSpaceID,
                               aSegHandle->mSegPID,
                               &sCurExtDirInfo,
                               &sFstExtDescRID,
                               &sExtDesc,
                               &sTotExtDescCnt )
              != IDE_SUCCESS );

    IDE_ASSERT( sFstExtDescRID != SD_NULL_RID );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment에 1개이상의 Extent를 할당한다.
 *
 * Segment에 연결된 할당된 Extent 혹은 Extent Dir.는 Undo 되지 않는다.
 *
 * aStatistics     - [IN]  통계정보
 * aSpaceID        - [IN]  Segment의 테이블스페이스
 * aCurExtDir      - [IN]  현재 ExtDir의 PID
 * aStartInfo      - [IN]  Mtx 시작정보
 * aFreeListIdx    - [IN]  할당받을 테이블스페이스의 ExtDir FreeList 타입
 * aAllocExtRID    - [OUT] 할당받은 Extent RID
 * aFstPIDOfExt    - [OUT] 할당받은 Extent의 첫번째 페이지 ID
 * aFstDataPIDOfExt- [OUT] 할당받은 Extent의 첫번째 데이타 페이지 ID
 *
 ***********************************************************************/
IDE_RC sdpscSegDDL::allocNewExts( idvSQL           * aStatistics,
                                  sdrMtxStartInfo  * aStartInfo,
                                  scSpaceID          aSpaceID,
                                  sdpSegHandle     * aSegHandle,
                                  scPageID           aCurExtDir,
                                  sdpFreeExtDirType  aFreeListIdx,
                                  sdRID            * aAllocExtRID,
                                  scPageID         * aFstPIDOfExt,
                                  scPageID         * aFstDataPIDOfExt )
{
    sdrMtx               sMtx;
    scPageID             sSegPID;
    smLSN                sNTA;
    UInt                 sState          = 0;
    UInt                 sTotExtCntOfSeg = 0;
    sdpscAllocExtDirInfo sAllocExtDirInfo;
    sdpscExtDirInfo      sCurExtDirInfo;
    sdpscExtDesc         sExtDesc;
    sdRID                sExtDescRID;
    idBool               sTryReusable = ID_FALSE;

    IDE_ASSERT( aStartInfo          != NULL );
    IDE_ASSERT( aSegHandle          != NULL );
    IDE_ASSERT( aSegHandle->mSegPID != SD_NULL_PID );
    IDE_ASSERT( aCurExtDir          != SD_NULL_PID );
    IDE_ASSERT( aAllocExtRID        != NULL );
    IDE_ASSERT( aFstPIDOfExt        != NULL );
    IDE_ASSERT( aFstDataPIDOfExt    != NULL );

    /* BUG-46036 codesonar warning 제거 */
    sAllocExtDirInfo.mIsAllocNewExtDir = ID_FALSE;

retry:
    
    sSegPID     = aSegHandle->mSegPID;
    sExtDescRID = SD_NULL_RID;
    sdpscExtDir::initExtDesc( &sExtDesc );

    sNTA = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdpscExtDir::getCurExtDirInfo( aStatistics,
                                             aSpaceID,
                                             aSegHandle,
                                             aCurExtDir,
                                             &sCurExtDirInfo )
              != IDE_SUCCESS );

    if ( sCurExtDirInfo.mIsFull == ID_TRUE )
    {
        // ExtDir 페이지를 생성해야한다는 것은 기존 LstExtDir 페이지가
        // 모두 ExtDesc로 채워졌다는 것이다. 그러므로 현재 새로운
        // Extent Dir. 페이지는 할당되어져야한다는 의미로 SD_NULL_PID로
        // 설정해둔다.
        IDE_TEST( sdpscExtDir::tryAllocExtDir( aStatistics,
                                               aStartInfo,
                                               aSpaceID,
                                               (sdpscSegCache*)aSegHandle->mCache,
                                               sSegPID,
                                               aFreeListIdx,
                                               sCurExtDirInfo.mNxtExtDirPID,
                                               &sAllocExtDirInfo )
                  != IDE_SUCCESS );

        if ( sAllocExtDirInfo.mNewExtDirPID != SD_NULL_PID )
        {
            IDE_ASSERT( sAllocExtDirInfo.mFstExtDescRID != SD_NULL_RID );

            if ( (sAllocExtDirInfo.mIsAllocNewExtDir == ID_TRUE) ||
                 (sAllocExtDirInfo.mShrinkExtDirPID  != SD_NULL_PID) )
            {
                IDE_TEST( addOrShrinkAllocExtDir( aStatistics,
                                                  &sMtx,
                                                  aSpaceID,
                                                  sSegPID,
                                                  aFreeListIdx,
                                                  &sCurExtDirInfo,
                                                  &sAllocExtDirInfo ) 
                          != IDE_SUCCESS );
            }
            else
            {
                // Shrink 없이 재사용되는 경우 혹은 prepareNewPage4Append에 의해서
                // NxtExtDir이 생성된 경우는 세그먼트헤더에 아무런 작업을 하지 않는다.
            }
        }

        sExtDescRID     = sAllocExtDirInfo.mFstExtDescRID;
        sExtDesc        = sAllocExtDirInfo.mFstExtDesc;
        sTotExtCntOfSeg = sAllocExtDirInfo.mTotExtCntOfSeg;
    }

    if ( sExtDescRID == SD_NULL_RID )
    {
        // 위의 재사용이나 ExtDir를 할당받지 못하였다면,
        // 마지막으로 TBS로부터 Extent를 할당한다.
        if ( sdptbExtent::allocExts( aStatistics,
                                     aStartInfo,
                                     aSpaceID,
                                     1, /* aExtCnt */
                                     (sdpExtDesc*)&sExtDesc )
            != IDE_SUCCESS )
        {
            IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_SPACE );
            IDE_TEST( sTryReusable == ID_TRUE );
            IDE_CLEAR();

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

            /*
             * BUG-27288 [5.3.3] Undo Full 후 오랜시간이 지나도 해소되지 않음.
             * : TBS 할당이 실패한 경우는 ExtDir를 강제적으로 FULL을 만들어
             *   다시 한번 ExtDir 재사용여부를 확인한다.
             */
            IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                           &sMtx,
                                           aStartInfo,
                                           ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                           SM_DLOG_ATTR_DEFAULT )
                      != IDE_SUCCESS );
            sState = 1;
            
            IDE_TEST( sdpscExtDir::makeExtDirFull( aStatistics,
                                                   &sMtx,
                                                   aSpaceID,
                                                   aCurExtDir )
                      != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
            
            sTotExtCntOfSeg = 0;
            sTryReusable = ID_TRUE;
            
            goto retry;
        }
        else
        {
            /* nothing  to do */
        }

        /* Extent 단위로 Segment Header 페이지의 현재 ExtDir 페이지에
         * 기록하거나, 필요하다면 ExtDir를 생성하여 기록하고
         * Segment Header 페이지를 설정한다. */
        IDE_TEST( addAllocExtDesc( aStatistics,
                                   &sMtx,
                                   aSpaceID,
                                   sSegPID,
                                   &sCurExtDirInfo,
                                   &sExtDescRID,
                                   &sExtDesc,
                                   &sTotExtCntOfSeg )
                  != IDE_SUCCESS );
    }

    IDE_ASSERT( (sExtDesc.mExtFstPID != SD_NULL_PID) &&
                (sExtDesc.mLength > 0) );

    IDE_ASSERT( sExtDesc.mExtFstDataPID != SD_MAKE_PID( sExtDescRID ) );

    sdrMiniTrans::setNullNTA( &sMtx, aSpaceID, &sNTA );

    if ( sTotExtCntOfSeg > 0 )
    {
        sdpscCache::setSegSizeByBytes(
            aSegHandle,
            ((sdpscSegCache*)aSegHandle->mCache)->mCommon.mPageCntInExt *
            SD_PAGE_SIZE * sTotExtCntOfSeg );
    }
    else
    {
        // Shrink 없이 재사용되는 경우 혹은 prepareNewPage4Append에 의해서
        // NxtExtDir이 생성된 경우는 세그먼트헤더에 아무런 작업을 하지 않는다.
        IDE_ASSERT( (sAllocExtDirInfo.mIsAllocNewExtDir == ID_FALSE) ||
                    (sAllocExtDirInfo.mNewExtDirPID     == sCurExtDirInfo.mNxtExtDirPID) );
    }

    *aFstPIDOfExt     = sExtDesc.mExtFstPID;
    *aFstDataPIDOfExt = sExtDesc.mExtFstDataPID;

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    *aAllocExtRID     = sExtDescRID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    *aAllocExtRID     = SD_NULL_RID;
    *aFstPIDOfExt     = SD_NULL_PID;
    *aFstDataPIDOfExt = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 새로운 하나의 Extent를 Segment에 할당하는 연산을 완료
 *
 * Segment에 확장된 Extent Desc.에 대해서 필요하다면 Extent Dir. 페이지를
 * 생성하여 Extent Desc를 기록하거나 기존 Extent Dir. 페이지에 기록한다.
 *
 * aStatistics     - [IN]  통계정보
 * aMtx            - [IN]  Mtx의 Pointer
 * aSpaceID        - [IN]  Tablespace의 ID
 * aSegPID         - [IN]  세그먼트 헤더 PID
 * aCurExtDirInfo  - [IN]  현재 ExtDir 정보
 * aAllocExtRID    - [OUT] 할당받은 ExtDesc 의 RID
 * aExtDesc        - [OUT] ExtDesc 정보
 * sTotExtDescCnt  - [OUT] 확장이후에 총 ExtDesc 개수
 *
 ***********************************************************************/
IDE_RC sdpscSegDDL::addAllocExtDesc( idvSQL             * aStatistics,
                                     sdrMtx             * aMtx,
                                     scSpaceID            aSpaceID,
                                     scPageID             aSegPID,
                                     sdpscExtDirInfo    * aCurExtDirInfo,
                                     sdRID              * aAllocExtRID,
                                     sdpscExtDesc       * aExtDesc,
                                     UInt               * aTotExtDescCnt )
{
    sdpscSegMetaHdr    * sSegHdr;
    sdpscExtDirCntlHdr * sCurExtDirCntlHdr;
    sdpscExtDirCntlHdr * sNxtExtDirCntlHdr;
    UChar              * sNxtExtDirPagePtr;
    sdpscExtCntlHdr    * sExtCntlHdr;
    UInt                 sTotExtDescCnt;
    scPageID             sCurExtDirPID;
    sdRID                sAllocExtRID;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aSegPID        != SD_NULL_PID );
    IDE_ASSERT( aCurExtDirInfo != NULL );
    IDE_ASSERT( aAllocExtRID   != NULL );
    IDE_ASSERT( aExtDesc       != NULL );
    IDE_ASSERT( aTotExtDescCnt != NULL );

    IDE_TEST( sdpscSegHdr::fixAndGetHdr4Write( aStatistics,
                                               aMtx,
                                               aSpaceID,
                                               aSegPID,
                                               &sSegHdr ) 
              != IDE_SUCCESS );

    sExtCntlHdr = sdpscSegHdr::getExtCntlHdr( sSegHdr );

    if ( aCurExtDirInfo->mExtDirPID == aSegPID )
    {
        sCurExtDirCntlHdr = sdpscSegHdr::getExtDirCntlHdr( sSegHdr );
    }
    else
    {
        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write(
                                         aStatistics,
                                         aMtx,
                                         aSpaceID,
                                         aCurExtDirInfo->mExtDirPID,
                                         &sCurExtDirCntlHdr )
                  != IDE_SUCCESS );

    }

    IDE_ASSERT( aCurExtDirInfo->mTotExtCnt == sCurExtDirCntlHdr->mExtCnt );

    // 만약, extent에 page가 생성되어 있을 수도 있으니까
    // 첫번째 data 페이지를 ExtDir 페이지로 생성하고,
    // 첫번재 data 페이지를 갱신한다.
    if ( aCurExtDirInfo->mIsFull == ID_TRUE )
    {
        IDE_ASSERT( sdpscExtDir::getFreeDescCnt( sCurExtDirCntlHdr ) == 0 );

        /* makeExtDirFull() 를 호출하게 되면, 해당 ExtDir의 mMaxExtCnt를
         * mExtCnt로 변경하여 강제로 Full 상태로 만든다.
         * 이렇게 mMaxExtCnt가 변경된 ExtDir의 CurExtDirInfo에서 mMaxExtCnt를
         * 가져오면 변경된 Max값이 설정되고, 변경된 Max값이 계속해서 다음
         * ExtDir에 반영되게 된다. 따라서 Max값을 프로퍼티에서 가져오도록
         * 수정한다. */
        IDE_TEST( sdpscExtDir::createAndInitPage(
                               aStatistics,
                               aMtx,
                               aSpaceID,
                               aExtDesc->mExtFstPID,
                               aCurExtDirInfo->mNxtExtDirPID,
                               sdpscSegHdr::getMaxExtDescCnt( sSegHdr->mSegCntlHdr.mSegType ),
                               &sNxtExtDirPagePtr ) 
                  != IDE_SUCCESS );

        sNxtExtDirCntlHdr = sdpscExtDir::getHdrPtr( sNxtExtDirPagePtr );

        IDE_TEST( sdpscSegHdr::addNewExtDir( aMtx,
                                             sSegHdr,
                                             sCurExtDirCntlHdr,
                                             sNxtExtDirCntlHdr )
                  != IDE_SUCCESS );

        sCurExtDirCntlHdr = sNxtExtDirCntlHdr;
        sCurExtDirPID     = aExtDesc->mExtFstPID;
    }
    else
    {
        sCurExtDirPID = aCurExtDirInfo->mExtDirPID;
    }

    /* To Fix BUG-23271 [SD] UDS/TSS 확장시 첫번째 Extent Dir.에
     * 포함된 Extent의 First Data PageID가 잘못결정됨
     *
     * mIsNeedExtDir이 True이 경우와 (Extent Dir.페이지에 더이상
     * Extent Desc.를 기록할 수 없는 Overflow상황) Extent Desc.의
     * FstPID가 SegPID인 경우는 Extent의 FstPID와 FstDataPID가 다르다.
     */
    if ( (aCurExtDirInfo->mIsFull == ID_TRUE) ||
         (aExtDesc->mExtFstPID    == aSegPID) )
    {
        aExtDesc->mExtFstDataPID = aExtDesc->mExtFstPID + 1;
    }
    else
    {
        aExtDesc->mExtFstDataPID = aExtDesc->mExtFstPID;
    }

    sAllocExtRID =
        SD_MAKE_RID( sCurExtDirPID,
                     sdpscExtDir::calcDescIdx2Offset(
                                      sCurExtDirCntlHdr,
                                      sCurExtDirCntlHdr->mExtCnt ) );

    IDE_ASSERT( sAllocExtRID != SD_NULL_RID );

    IDE_TEST( sdpscExtDir::logAndAddExtDesc( aMtx,
                                             sCurExtDirCntlHdr,
                                             aExtDesc )
              != IDE_SUCCESS );

    sTotExtDescCnt = sExtCntlHdr->mTotExtCnt + 1;
    IDE_TEST( sdpscSegHdr::setTotExtCnt( aMtx,
                                         sExtCntlHdr,
                                         sTotExtDescCnt )
              != IDE_SUCCESS );

    *aAllocExtRID   = sAllocExtRID;
    *aTotExtDescCnt = sTotExtDescCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aAllocExtRID   = SD_NULL_RID;
    *aTotExtDescCnt = 0;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 새로운 하나의 ExtDir를 Segment에 연결하는 연산
 *
 * Extent Dir. 페이지 단위로 Extent를 할당해왔다면, 세그먼트의 현재 Extent Dir.
 * 페이지 앞에다 추가한다.
 * 만약, Nxt Extent Dir.가 Shrink 가 가능하다고 판단되었다면,
 * Undo TBS의 ExtDir. FreeList에 추가한다.
 *
 * aStatistics      - [IN] 통계정보
 * aMtx             - [IN] Mtx의 Pointer
 * aSpaceID         - [IN] Tablespace의 ID
 * aSegPID          - [IN] 세그먼트 헤더 PID
 * aFreeListIdx     - [IN] 할당받을 테이블스페이스의 ExtDir FreeList 타입
 * aCurExtDirInfo   - [IN] 현재 ExtDir 정보
 * aAllocExtDirInfo - [IN] 새로운 ExtDir 할당 정보
 *
 ***********************************************************************/
IDE_RC sdpscSegDDL::addOrShrinkAllocExtDir(
                            idvSQL                * aStatistics,
                            sdrMtx                * aMtx,
                            scSpaceID               aSpaceID,
                            scPageID                aSegPID,
                            sdpFreeExtDirType       aFreeListIdx,
                            sdpscExtDirInfo       * aCurExtDirInfo,
                            sdpscAllocExtDirInfo  * aAllocExtDirInfo )
{
    sdpscSegMetaHdr    * sSegHdr;
    sdpscExtDirCntlHdr * sCurExtDirCntlHdr = NULL;
    sdpscExtDirCntlHdr * sNewExtDirCntlHdr = NULL;
    sdpscExtCntlHdr    * sExtCntlHdr       = NULL;
    UInt                 sTotExtCntOfSeg   = 0;

    IDE_ASSERT( aMtx             != NULL );
    IDE_ASSERT( aCurExtDirInfo   != NULL );
    IDE_ASSERT( aAllocExtDirInfo != NULL );

    IDE_ASSERT( aAllocExtDirInfo->mNewExtDirPID != SD_NULL_PID );

    IDE_TEST( sdpscSegHdr::fixAndGetHdr4Write( aStatistics,
                                               aMtx,
                                               aSpaceID,
                                               aSegPID,
                                               &sSegHdr ) 
              != IDE_SUCCESS );

    sExtCntlHdr     = sdpscSegHdr::getExtCntlHdr( sSegHdr );
    sTotExtCntOfSeg = sExtCntlHdr->mTotExtCnt;
    IDE_ASSERT( sTotExtCntOfSeg > 0 );

    if ( aAllocExtDirInfo->mIsAllocNewExtDir == ID_TRUE )
    {
        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write(
                                  aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  aCurExtDirInfo->mExtDirPID,
                                  &sCurExtDirCntlHdr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write(
                                  aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  aAllocExtDirInfo->mNewExtDirPID,
                                  &sNewExtDirCntlHdr )
                  != IDE_SUCCESS );

        // 재사용하지 않는 경우에는 Shrink 하지 않는다.
        IDE_ASSERT( aAllocExtDirInfo->mShrinkExtDirPID == SD_NULL_PID );

        /* Extent Dir. 단위 할당이 발생할때에는 다른 세그먼트에서 사용하던
         * Extent Dir. 페이지를 할당해오는 것이다. 그러므로 만약 UndoTBS가
         * 재구동시 리셋이 되지 않은 경우에는 이전 구동시에 MaxExtInExtDir
         * 프로퍼티에 의해 생성된 Extent Dir.로 존재할 수 있다.
         * 일반적으로는 재구동시 UndoTBS를 리셋하기 때문에 다르지 않다. */
        IDE_TEST( sdpscSegHdr::addNewExtDir( aMtx,
                                             sSegHdr,
                                             sCurExtDirCntlHdr,
                                             sNewExtDirCntlHdr ) 
                  != IDE_SUCCESS );

        /* BUG-29709 undo segment의 total extent count가 잘못 관리되고 있습니다.
         *
         * Segment에 추가된 ExtDir의 헤더(sNewExtDirCntlHdr)에서 Extent 개수를
         * 가져온다. */
        sTotExtCntOfSeg += sNewExtDirCntlHdr->mExtCnt;
    }
    else
    {
        // Nxt ExtDir을 재사용하는 경우에는 ExtDir List에 연결이
        // 이미 되어 있다. 이 경우에는 ShrinkExtDir만 고려해주면 된다.
    }

    if ( aAllocExtDirInfo->mShrinkExtDirPID != SD_NULL_PID )
    {
        IDE_ASSERT( sCurExtDirCntlHdr == NULL );

        // Shrink 연산이 가능하다는 것은 Nxt Nxt를
        // 재사용하기 때문에, 새로운 Extent 혹은 Ext Dir. 할당이 없다.
        IDE_ASSERT( aAllocExtDirInfo->mIsAllocNewExtDir == ID_FALSE );
        IDE_ASSERT( aAllocExtDirInfo->mShrinkExtDirPID  != aSegPID );

        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write(
                                  aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  aCurExtDirInfo->mExtDirPID,
                                  &sCurExtDirCntlHdr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpscSegHdr::removeExtDir(
                                  aMtx,
                                  sSegHdr,
                                  sCurExtDirCntlHdr,
                                  aAllocExtDirInfo->mShrinkExtDirPID,
                                  aAllocExtDirInfo->mNewExtDirPID )
                  != IDE_SUCCESS );

        IDE_TEST( sdptbExtent::freeExtDir(
                                  aStatistics,
                                  aMtx,
                                  aSpaceID,
                                  aFreeListIdx,
                                  aAllocExtDirInfo->mShrinkExtDirPID )
                  != IDE_SUCCESS );

        /* BUG-29709 undo segment의 total extent count가 잘못 관리되고 있습니다.
         *
         * Segment에서 삭제된 Extent Dir에 포함된 Extent 개수를 빼주어야 한다.
         *
         * mExtCntInShrinkExtDir은 sdpscExtDir::tryAllocExtDir()에서
         * Next ExtDir과 Next Next ExtDir 이 재사용 가능할때,
         * Next ExtDir을 shrink하고, Next Next ExtDir을 재사용하는데,
         * shrink할 Next ExtDir PID를 AllocExtDirInfo에 설정할때 함께
         * 설정한다. */
        IDE_ASSERT( sTotExtCntOfSeg > aAllocExtDirInfo->mExtCntInShrinkExtDir );
        sTotExtCntOfSeg -= aAllocExtDirInfo->mExtCntInShrinkExtDir;
    }
    else
    {
        // 새로 추가도 Shrink 도 없이 재사용하는 경우에는
        // ExtDesc 개수의 변함이 없다.
    }

    if ( sExtCntlHdr->mTotExtCnt != sTotExtCntOfSeg )
    {
        IDE_TEST( sdpscSegHdr::setTotExtCnt( aMtx, 
                                             sExtCntlHdr, 
                                             sTotExtCntOfSeg )
                  != IDE_SUCCESS );
    }

    aAllocExtDirInfo->mTotExtCntOfSeg = sTotExtCntOfSeg;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aAllocExtDirInfo->mTotExtCntOfSeg = 0;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : [INTERFACE] From 세그먼트에서 To 세그먼트로
 *               Extent Dir를 옮긴다
 *
 * aStatistics  - [IN] 통계정보
 * aStartInfo   - [IN] Mtx 시작정보
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aFrSegHandle - [IN] From 세그먼트의 핸들
 * aFrSegPID    - [IN] From 세그먼트의 PID
 * aFrCurExtDir - [IN] From 세그먼트의 현재 ExtDir 페이지의 PID
 * aToSegHandle - [IN] To 세그먼트의 핸들
 * aToSegPID    - [IN] To 세그먼트의 PID
 * aToCurExtDir - [IN] To 세그먼트의 현재 ExtDir 페이지의 PID
 * aTrySuccess  - [OUT] Steal연산의 성공여부 반환
 *
 ***********************************************************************/
IDE_RC sdpscSegDDL::tryStealExts( idvSQL          * aStatistics,
                                  sdrMtxStartInfo * aStartInfo,
                                  scSpaceID         aSpaceID,
                                  sdpSegHandle    * aFrSegHandle,
                                  scPageID          aFrSegPID,
                                  scPageID          aFrCurExtDir,
                                  sdpSegHandle    * aToSegHandle,
                                  scPageID          aToSegPID,
                                  scPageID          aToCurExtDir,
                                  idBool          * aTrySuccess )
{
    sdrMtx                 sMtx;
    sdpscAllocExtDirInfo   sAllocExtDirInfo;
    sdpscExtDirInfo        sCurExtDirInfo;
    sdpscExtDirInfo        sNxtExtDirInfo;
    sdpscExtDirInfo        sOldExtDirInfo;
    UInt                   sTotExtCntOfFrSeg;
    UInt                   sTotExtCntOfToSeg;
    UInt                   sState = 0;

    IDE_ASSERT( aStartInfo   != NULL );
    IDE_ASSERT( aFrSegHandle != NULL );
    IDE_ASSERT( aFrSegPID    != SD_NULL_PID );
    IDE_ASSERT( aToSegHandle != NULL );
    IDE_ASSERT( aToSegPID    != SD_NULL_PID );
    IDE_ASSERT( aTrySuccess  != NULL );

    *aTrySuccess = ID_FALSE;
    /*
     * (1) From Segment의 Current Extent Dir의
     *     Next Extent Dir이 재사용 가능한지 체크해본다.
     */
    IDE_TEST( sdpscExtDir::getCurExtDirInfo( aStatistics,
                                             aSpaceID,
                                             aFrSegHandle,
                                             aFrCurExtDir,
                                             &sCurExtDirInfo )
              != IDE_SUCCESS );

    /* BUG-30897 When the CurAllocExt locates in the Segment Header Page, 
     *           An UndoSegemnt can not steal an Extent from other Segment.
     * Next Extend Directory가 SegHdr일 경우, 상대의 SegmentHeader가 들어간
     * ExtentDirectory는 재활용 할 수 없기 때문에 다음 Extent를 선택한다. */
    if( aFrSegPID == sCurExtDirInfo.mNxtExtDirPID )
    {
        /* 자기 혼자밖에 남지않았으면 볼 필요 없다.*/
        IDE_TEST_CONT( sCurExtDirInfo.mExtDirPID == 
                             sCurExtDirInfo.mNxtExtDirPID,
                        CONT_CANT_STEAL );

        idlOS::memcpy( (void*)&sOldExtDirInfo, 
                       (void*)&sCurExtDirInfo, 
                       ID_SIZEOF( sdpscExtDirInfo ) );

        /* 다음 ExtentDirectory를 얻는다. */
        aFrCurExtDir = sCurExtDirInfo.mNxtExtDirPID;

        IDE_TEST( sdpscExtDir::getCurExtDirInfo( aStatistics,
                                                 aSpaceID,
                                                 aFrSegHandle,
                                                 aFrCurExtDir,
                                                 &sCurExtDirInfo )

                  != IDE_SUCCESS );       


        /* NextOfNext가 Old(원래의 ED)일 경우, 한바퀴 돌았다는 뜻으로
         * 사용중인 ED다. 따라서, 볼 필요가 없다. */
        IDE_TEST_CONT( sCurExtDirInfo.mNxtExtDirPID == sOldExtDirInfo.mExtDirPID,
                       CONT_CANT_STEAL );
    }

    /* NxtExt 가 steal 가능한지 확인한다 */
    IDE_TEST( sdpscExtDir::checkNxtExtDir4Steal( aStatistics,
                                                 aStartInfo,
                                                 aSpaceID,
                                                 aFrSegPID,
                                                 sCurExtDirInfo.mNxtExtDirPID,
                                                 &sAllocExtDirInfo )
                  != IDE_SUCCESS );

    IDE_TEST_CONT( sAllocExtDirInfo.mNewExtDirPID == SD_NULL_PID,
                    CONT_CANT_STEAL );

    /*
     * BUG-29709 undo segment의 total extent count가 잘못 관리되고 있습니다.
     *
     * (2) From Segment에서 steal 가능한(NxtExt)ExtDirInfo를 가져온다.
     */
    IDE_TEST( sdpscExtDir::getCurExtDirInfo( aStatistics,
                                             aSpaceID,
                                             aFrSegHandle,
                                             sAllocExtDirInfo.mNewExtDirPID,
                                             &sNxtExtDirInfo )
              != IDE_SUCCESS );

    /*
     * (3) From Segment로부터 찾은 Extent Dir 페이지를 떼어내고,
     *     To Segment에 Current Extent Dir의 Next에 추가한다.
     *     연산이 실패해도 그만이다.
     */
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_ASSERT( sAllocExtDirInfo.mNewExtDirPID != aFrSegPID );
    
    IDE_ERROR( sNxtExtDirInfo.mTotExtCnt == (SShort)sAllocExtDirInfo.mExtCntInShrinkExtDir );
    IDE_ERROR( sNxtExtDirInfo.mExtDirPID == sAllocExtDirInfo.mNewExtDirPID );
    IDE_ERROR( sNxtExtDirInfo.mNxtExtDirPID == sAllocExtDirInfo.mNxtPIDOfNewExtDir );

    /* From Segment에서 NxtExt를 (steal 가능한) 제거  */
    IDE_TEST( sdpscSegHdr::removeExtDir( 
                         aStatistics,
                         &sMtx,
                         aSpaceID,
                         aFrSegPID,                 
                         sCurExtDirInfo.mExtDirPID,           /*aPrvPIDOfExtDir*/
                         sAllocExtDirInfo.mNewExtDirPID,      /*aRemExtDirPID */ 
                         sAllocExtDirInfo.mNxtPIDOfNewExtDir, /*aNxtPIDOfNewExtDir */
                         sAllocExtDirInfo.mExtCntInShrinkExtDir,
                         &sTotExtCntOfFrSeg )
              != IDE_SUCCESS );

    /* steal한 ExtCnt보다 To Segment에서 관리하는 MaxExtCnt가 많다면
       나머지 부분을 free */
    IDE_TEST( sdpscExtDir::shrinkExtDir( aStatistics,
                                         &sMtx,
                                         aSpaceID,
                                         aToSegHandle,
                                         &sAllocExtDirInfo )
              != IDE_SUCCESS ); 

    /* To Segment에 NxtExt를 (steal 가능한) add */
    IDE_TEST( sdpscSegHdr::addNewExtDir( 
                             aStatistics,
                             &sMtx,
                             aSpaceID,
                             aToSegPID,
                             aToCurExtDir,                    /* aCurExtDirPID */
                             sAllocExtDirInfo.mNewExtDirPID,  /* aNewExtDirPID*/
                             &sTotExtCntOfToSeg )             /* aTotExtCntOfToSeg */
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    sdpscCache::setSegSizeByBytes(
                aFrSegHandle,
                ((sdpscSegCache*)aFrSegHandle->mCache)->mCommon.mPageCntInExt *
                SD_PAGE_SIZE * sTotExtCntOfFrSeg );

    sdpscCache::setSegSizeByBytes(
                aToSegHandle,
                ((sdpscSegCache*)aToSegHandle->mCache)->mCommon.mPageCntInExt *
                SD_PAGE_SIZE * sTotExtCntOfToSeg );

    *aTrySuccess = ID_TRUE;

    IDE_EXCEPTION_CONT( CONT_CANT_STEAL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}
