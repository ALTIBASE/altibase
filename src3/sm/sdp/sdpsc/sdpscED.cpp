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
 * $Id: sdpscED.cpp 89495 2020-12-14 05:19:22Z emlee $
 *
 * 본 파일은 Circular-List Managed Segment의 ExtDir 페이지에 대한 STATIC
 * 인터페이스를 관리한다.
 *
 * 다음은 Segment 확장에 대한 설명이다.
 *
 * (1) Extent Dir.Page 단위 할당 연산
 *     Undo Tablespace의 ID 0 File Header에는 재사용 가능한 Free Extent Dir.
 *     페이지 List가 존재한다.재사용 가능한 Extent Dir. 페이지에는 Extent가
 *     max 개수만큼 가득 포함되어 있다.
 *     Segment에 연결할 때에는 현재 Extent Dir. 페이지와 Next Extent Dir.페이지
 *     사이에 위치시킨다.
 *
 * (2) Extent 단위 할당 연산
 *     Undo Tablespace의 ID 0 File Header에 재사용 가능한 Extent Dir. 페이지가
 *     없거나 다른 트랜잭션이 이미 Latch를 걸고 있다면  대기하지 않고, 바로 Undo
 *     Tablespace로부터 Extent를 할당받는다.
 *     할당받은 Extent를 기록할 Extent Dir. 페이지가 없다면 생성하고 현재 Extent
 *     Dir. 페이지와 Next Extent Dir.페이지 사이에 위치시킨다.
 *
 ***********************************************************************/

# include <sdpDef.h>

# include <smxTrans.h>
# include <smxTransMgr.h>

# include <sdptbDef.h>
# include <sdptbExtent.h>

# include <sdpscSH.h>
# include <sdpscAllocPage.h>
# include <sdpscSegDDL.h>
# include <sdpscCache.h>
# include <sdpscED.h>


/***********************************************************************
 *
 * Description : ExtDir Control Header 초기화
 *
 * aCntlHdr   - [IN] Extent Dir. 페이지의 Control 헤더 포인터
 * aNxtExtDir - [IN] NxtExtDir 페이지의 PID
 * aMapOffset - [IN] Extent Desc. 맵의 오프셋
 * aMaxExtCnt - [IN] Extent Dir. 페이지가 기록할 수 있는
 *                   최대 Extent Desc. 개수
 *
 ***********************************************************************/
void  sdpscExtDir::initCntlHdr( sdpscExtDirCntlHdr * aCntlHdr,
                                scPageID             aNxtExtDir,
                                scOffset             aMapOffset,
                                UShort               aMaxExtCnt )
{
    IDE_ASSERT( aCntlHdr   != NULL );
    IDE_ASSERT( aNxtExtDir != SD_NULL_PID );

    aCntlHdr->mExtCnt    = 0;           // 페이지내의 Extent 개수
    aCntlHdr->mNxtExtDir = aNxtExtDir;  // 다음 ExtDir 페이지의 PID
    aCntlHdr->mMapOffset = aMapOffset;  // Extent Map의 Offset
    aCntlHdr->mMaxExtCnt = aMaxExtCnt;  // 페이지내의 Max Extent 개수

    // 마지막 사용한 트랜잭션의 CommitSCN을 설정하여 재사용여부를
    // 판단하는 기준으로 사용
    SM_INIT_SCN( &(aCntlHdr->mLstCommitSCN) );

    // 자신이 사용한 것인지를 확인할 때 사용
    SM_INIT_SCN( &(aCntlHdr->mFstDskViewSCN) );

    return;
}

/***********************************************************************
 *
 * Description : Extent Dir. 페이지 생성 및 초기화
 *
 * aStatistics        - [IN]  통계정보
 * aMtx               - [IN]  Mtx 포인터
 * aSpaceID           - [IN]  테이블스페이스 ID
 * aNewExtDirPID      - [IN]  생성해야할 ExtDir PID
 * aNxtExtDirPID      - [IN]  NxtExtDir 페이지의 PID
 * aMaxExtCntInExtDir - [IN]  ExtDir 내의 ExtDesc 최대 저장 개수
 * aPagePtr           - [OUT] 생성된 Extent Dir. 페이지의 시작 포인터
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::createAndInitPage( idvSQL            * aStatistics,
                                       sdrMtx            * aMtx,
                                       scSpaceID           aSpaceID,
                                       scPageID            aNewExtDirPID,
                                       scPageID            aNxtExtDirPID,
                                       UShort              aMaxExtCntInExtDir,
                                       UChar            ** aPagePtr )
{
    UChar            * sPagePtr;
    sdpParentInfo      sParentInfo;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aNewExtDirPID  != SD_NULL_PID );

    sParentInfo.mParentPID   = aNewExtDirPID;
    sParentInfo.mIdxInParent = SDPSC_INVALID_IDX;

    IDE_TEST( sdpscAllocPage::createPage(
                                    aStatistics,
                                    aSpaceID,
                                    aNewExtDirPID,
                                    SDP_PAGE_CMS_EXTDIR,
                                    &sParentInfo,
                                    (sdpscPageBitSet)
                                    ( SDPSC_BITSET_PAGETP_META |
                                      SDPSC_BITSET_PAGEFN_FUL ),
                                    aMtx,
                                    aMtx,
                                    &sPagePtr ) != IDE_SUCCESS );

    IDE_TEST( logAndInitCntlHdr( aMtx,
                                 getHdrPtr( sPagePtr ),
                                 aNxtExtDirPID,
                                 aMaxExtCntInExtDir )
              != IDE_SUCCESS );

    *aPagePtr = sPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir Page Control Header 초기화 및 write logging
 *
 * aMtx          - [IN] Mtx 포인터
 * aCntlHdr      - [IN] Extent Dir.페이지의 Control 헤더 포인터
 * aNxtExtDirPID - [IN] NxtExtDir 페이지의 PID
 * aMaxExtCnt    - [IN] Extent Dir. 페이지가 기록할 수 있는
 *                      최대 Extent Desc. 개수
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::logAndInitCntlHdr( sdrMtx               * aMtx,
                                       sdpscExtDirCntlHdr   * aCntlHdr,
                                       scPageID               aNxtExtDirPID,
                                       UShort                 aMaxExtCnt )
{
    scOffset sMapOffset;

    IDE_ASSERT( aCntlHdr != NULL );
    IDE_ASSERT( aMtx     != NULL );

    // Segment Header의 ExtDir Offset과 ExtDir 페이지에서의
    // Map Offset은 다르다.
    sMapOffset =
        sdpPhyPage::getDataStartOffset( ID_SIZEOF( sdpscExtDirCntlHdr ) );

    // page range slot초기화도 해준다.
    initCntlHdr( aCntlHdr, aNxtExtDirPID, sMapOffset, aMaxExtCnt );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aCntlHdr,
                                         NULL,
                                         (ID_SIZEOF( aNxtExtDirPID ) +
                                          ID_SIZEOF( scOffset )      +
                                          ID_SIZEOF( aMaxExtCnt )),
                                         SDR_SDPSC_INIT_EXTDIR )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (UChar*)&aNxtExtDirPID,
                                   ID_SIZEOF( aNxtExtDirPID ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (UChar*)&sMapOffset,
                                   ID_SIZEOF( scOffset ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (UChar*)&aMaxExtCnt,
                                   ID_SIZEOF( aMaxExtCnt ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir에 Extent Desc. 기록
 *
 * aMapPtr  - [IN] Extent Dir. 페이지의 Extent Desc. 맵 포인터
 * aLstIdx  - [IN] Extent Desc.가 기록될 마지막 맵에서의 순번
 * aExtDesc - [IN] 기록할 Extent Desc. 포인터
 *
 ***********************************************************************/
void sdpscExtDir::addExtDescToMap( sdpscExtDirMap  * aMapPtr,
                                   SShort            aLstIdx,
                                   sdpscExtDesc    * aExtDesc )
{
    IDE_ASSERT( aMapPtr                  != NULL );
    IDE_ASSERT( aExtDesc                 != NULL );
    IDE_ASSERT( aLstIdx                  != SDPSC_INVALID_IDX );
    IDE_ASSERT( aExtDesc->mExtFstPID     != SD_NULL_PID );
    IDE_ASSERT( aExtDesc->mLength        >= SDP_MIN_EXTENT_PAGE_CNT );
    IDE_ASSERT( aExtDesc->mExtFstDataPID != SD_NULL_PID );

    // 다음 Emtpy Extent Slot 을 구한다.
    aMapPtr->mExtDesc[ aLstIdx ] = *aExtDesc;

    return;
}

/***********************************************************************
 *
 * Description : ExtDir에 extslot을 기록한다.
 *
 * aCntlHdr - [IN] Extent Dir.페이지의 Control 헤더 포인터
 * aLstIdx  - [IN] Extent Desc.가 기록될 마지막 맵에서의 순번
 * aExtDesc - [IN] 기록할 Extent Desc. 포인터
 *
 ***********************************************************************/
void sdpscExtDir::addExtDesc( sdpscExtDirCntlHdr * aCntlHdr,
                              SShort               aLstIdx,
                              sdpscExtDesc       * aExtDesc )
{
    IDE_ASSERT( aCntlHdr != NULL );
    IDE_ASSERT( aExtDesc != NULL );
    IDE_ASSERT( aLstIdx  != SDPSC_INVALID_IDX );

    addExtDescToMap( getMapPtr(aCntlHdr), aLstIdx, aExtDesc );
    aCntlHdr->mExtCnt++;

    return;
}

/***********************************************************************
 *
 * Descripton : Segment Header로부터 마지막 ExtDir에 기록
 *
 * aMtx         - [IN] Mtx 포인터
 * aCntlHdr     - [IN] Extent Dir.페이지의 Control 헤더 포인터
 * aExtDesc     - [IN] 기록할 Extent Desc. 포인터
 * aAllocExtRID - [IN] 기록할 Extent Desc.의 RID
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::logAndAddExtDesc( sdrMtx             * aMtx,
                                      sdpscExtDirCntlHdr * aCntlHdr,
                                      sdpscExtDesc       * aExtDesc )
{
    IDE_ASSERT( aCntlHdr                 != NULL );
    IDE_ASSERT( aMtx                     != NULL );
    IDE_ASSERT( aExtDesc                 != NULL );
    IDE_ASSERT( aExtDesc->mLength        > 0 );
    IDE_ASSERT( aExtDesc->mExtFstPID     != SD_NULL_PID );
    IDE_ASSERT( aExtDesc->mExtFstDataPID != SD_NULL_PID );

    addExtDesc( aCntlHdr, aCntlHdr->mExtCnt, aExtDesc );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aCntlHdr,
                                         aExtDesc,
                                         ID_SIZEOF( sdpscExtDesc ),
                                         SDR_SDPSC_ADD_EXTDESC_TO_EXTDIR )
              != IDE_SUCCESS );

    IDE_ASSERT( aCntlHdr->mExtCnt > 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir Control Header가 속해있는 페이지를 release한다.
 *
 * aStatistics    - [IN] 통게정보
 * aCntlHdr       - [IN] ExtDir Cntl Header
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::releaseCntlHdr( idvSQL              * aStatistics,
                                    sdpscExtDirCntlHdr  * aCntlHdr )
{
    UChar *sPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aCntlHdr );

    IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : [ INTERFACE ] Squential Scan시 Extent 의 첫번재 Data
 *               페이지를 반환한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] 테이블스페이스 ID
 * aExtRID     - [IN] 정보를 얻을 대상 Extent Desc.의 RID
 * aExtInfo    - [OUT] Extent Desc.의 정보
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::getExtInfo( idvSQL       *aStatistics,
                                scSpaceID     aSpaceID,
                                sdRID         aExtRID,
                                sdpExtInfo   *aExtInfo )
{
    UInt             sState = 0;
    sdpscExtDesc   * sExtDesc;

    IDE_ASSERT( aExtInfo    != NULL );

    IDE_TEST( sdbBufferMgr::fixPageByRID( aStatistics, /* idvSQL */
                                          aSpaceID,
                                          aExtRID,
                                          (UChar**)&sExtDesc  )
              != IDE_SUCCESS );
    sState = 1;

    aExtInfo->mFstPID     = sExtDesc->mExtFstPID;
    aExtInfo->mFstDataPID = sExtDesc->mExtFstDataPID;
    aExtInfo->mLstPID     = sExtDesc->mExtFstPID + sExtDesc->mLength - 1;

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                       (UChar*)sExtDesc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    if (sState != 0)
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics, (UChar*)sExtDesc )
                    == IDE_SUCCESS );
    }
    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 다음 Extent Desc.의 RID 반환
 *
 * aStatistics - [IN]  통계정보
 * aSpaceID    - [IN]  테이블스페이스 ID
 * aSegHdrPID  - [IN]  세그먼트 헤더 페이지의 PID
 * aCurrExtRID - [IN]  현재 Extent Desc.의 RID
 * aNxtExtRID  - [OUT] 다음 Extent Desc.의 RID
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::getNxtExtRID( idvSQL       *aStatistics,
                                  scSpaceID     aSpaceID,
                                  scPageID    /*aSegHdrPID*/,
                                  sdRID         aCurrExtRID,
                                  sdRID        *aNxtExtRID )
{
    UInt                  sState = 0;
    scPageID              sNxtExtDir;
    sdpscExtDirCntlHdr  * sCntlHdr;
    SShort                sExtDescIdx;

    IDE_ASSERT( aNxtExtRID != NULL );

    IDE_TEST( fixAndGetCntlHdr4Read( aStatistics,
                                     NULL,  /* aMtx */
                                     aSpaceID,
                                     SD_MAKE_PID( aCurrExtRID ),
                                     &sCntlHdr  )
              != IDE_SUCCESS );
    sState = 1;

    sExtDescIdx = calcOffset2DescIdx( sCntlHdr,
                                      SD_MAKE_OFFSET( aCurrExtRID ) );

    IDE_ASSERT( sCntlHdr->mExtCnt > sExtDescIdx );

    if ( sCntlHdr->mExtCnt > ( sExtDescIdx + 1 ) )
    {
        // ExtDir 페이지에서 마지막 Extent Slot이 아닌경우
        *aNxtExtRID = SD_MAKE_RID( SD_MAKE_PID( aCurrExtRID ),
                                   calcDescIdx2Offset( sCntlHdr, (sExtDescIdx+1) ) );
    }
    else
    {
        // ExtDir 페이지에서 마지막 Extent Slot인 경우 NULL_RID 반환
        if ( sCntlHdr->mNxtExtDir == SD_NULL_PID )
        {
            *aNxtExtRID = SD_NULL_RID;
        }
        else
        {
            // Next Extent Dir. 페이지가 존재하는 경우
            sNxtExtDir = sCntlHdr->mNxtExtDir;

            sState = 0;
            IDE_TEST( releaseCntlHdr( aStatistics, sCntlHdr) != IDE_SUCCESS );

            IDE_TEST( fixAndGetCntlHdr4Read( aStatistics,
                                             NULL, /* aMtx */
                                             aSpaceID,
                                             sNxtExtDir,
                                             &sCntlHdr  ) != IDE_SUCCESS );
            sState = 1;

            *aNxtExtRID = SD_MAKE_RID( sNxtExtDir,
                                       calcDescIdx2Offset( sCntlHdr, 0 ));
        }
    }

    sState = 0;
    IDE_TEST( releaseCntlHdr( aStatistics, sCntlHdr) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( releaseCntlHdr( aStatistics, sCntlHdr ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : 직접 Extent로부터 새로운 페이지를 할당한다.
 *
 * aPrvAllocExtRID가 가리키는 Extent에 aPrvAllocPageID이후
 * Page가 존재하는 하는지 체크해서 없으면 새로운 다음
 * Extent로 이동하고 다음 Extent가 없으면 TBS로 부터 새로운
 * Extent를 할당받는다. 이후 Extent에서 Free Page를 찾아서
 * Page가 할당된 ExtRID와 PageID를 넘겨준다.
 *
 * aStatistics          - [IN] 통계 정보
 * aMtx                 - [IN] Mtx 포인터
 * aSpaceID             - [IN] 테이블스페이스 ID
 * aSegHandle           - [IN] 세그먼트 핸들 포인터
 * aPrvAllocExtRID      - [IN] 이전에 Page를 할당받았던 Extent RID
 * aFstPIDOfPrvAllocExt - [IN] 이전 Extent의 첫번째 페이지 ID
 * aPrvAllocPageID      - [IN] 이전에 할당받은 PageID
 * aAllocExtRID         - [OUT] 새로운 Page가 할당된 Extent RID
 * aFstPIDOfAllocExt    - [OUT] 할당해준 Extent Desc.의 첫번째 PageID
 * aAllocPID            - [OUT] 새롭게 할당받은 PageID
 * aParentInfo          - [OUT] 할당된 Page's Parent Info
 *
 **********************************************************************/
IDE_RC sdpscExtDir::allocNewPageInExt(
                                idvSQL         * aStatistics,
                                sdrMtx         * aMtx,
                                scSpaceID        aSpaceID,
                                sdpSegHandle   * aSegHandle,
                                sdRID            aPrvAllocExtRID,
                                scPageID         aFstPIDOfPrvAllocExt,
                                scPageID         aPrvAllocPageID,
                                sdRID          * aAllocExtRID,
                                scPageID       * aFstPIDOfAllocExt,
                                scPageID       * aAllocPID,
                                sdpParentInfo  * aParentInfo )
{
    sdRID                 sAllocExtRID;
    sdrMtxStartInfo       sStartInfo;
    sdpFreeExtDirType     sFreeListIdx;
    sdRID                 sPrvAllocExtRID;
    scPageID              sPrvAllocPageID;
    scPageID              sFstPIDOfPrvAllocExt;
    idBool                sIsNeedNewExt;
    scPageID              sFstPIDOfExt;
    scPageID              sFstDataPIDOfExt;

    IDE_ASSERT( aSegHandle        != NULL );
    IDE_ASSERT( aMtx              != NULL );
    IDE_ASSERT( aAllocExtRID      != NULL );
    IDE_ASSERT( aFstPIDOfAllocExt != NULL );
    IDE_ASSERT( aAllocPID         != NULL );

    sAllocExtRID  = SD_NULL_RID;
    sIsNeedNewExt = ID_TRUE;

    if ( sdpscCache::getSegType( aSegHandle ) == SDP_SEG_TYPE_TSS )
    {
        sFreeListIdx = SDP_TSS_FREE_EXTDIR_LIST;
    }
    else
    {
        sFreeListIdx = SDP_UDS_FREE_EXTDIR_LIST;
    }

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    sPrvAllocExtRID      = aPrvAllocExtRID;
    sFstPIDOfPrvAllocExt = aFstPIDOfPrvAllocExt;

    IDE_ASSERT( aPrvAllocExtRID != SD_NULL_RID );

    if ( aPrvAllocPageID == SD_NULL_PID )
    {
        IDE_ASSERT( sFstPIDOfPrvAllocExt == aSegHandle->mSegPID );

        sPrvAllocPageID = aSegHandle->mSegPID;
    }
    else
    {
        sPrvAllocPageID = aPrvAllocPageID;
    }

    if ( isFreePIDInExt( ((sdpscSegCache*)aSegHandle->mCache)->mCommon.mPageCntInExt,
                        sFstPIDOfPrvAllocExt,
                        sPrvAllocPageID ) == ID_TRUE )
    {
        sIsNeedNewExt = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    if ( sIsNeedNewExt == ID_TRUE )
    {
        /* aPrvAllocExtRID 다음 Extent가 존재하는지 Check한다. */
        IDE_TEST( getNxtExt4Alloc( aStatistics,
                                   aSpaceID,
                                   aPrvAllocExtRID,
                                   &sAllocExtRID,
                                   &sFstPIDOfExt,
                                   &sFstDataPIDOfExt ) 
                  != IDE_SUCCESS );

        if ( sFstDataPIDOfExt != SD_NULL_PID )
        {
            IDE_ASSERT( sFstDataPIDOfExt != SD_MAKE_PID( sAllocExtRID ) );
        }
        else
        {
            IDE_ASSERT( sAllocExtRID == SD_NULL_RID );
        }

        if ( sAllocExtRID == SD_NULL_RID )
        {
            /* 새로운 Extent를 TBS로부터 할당받는다. */
            IDE_TEST( sdpscSegDDL::allocNewExts( aStatistics,
                                                 &sStartInfo,
                                                 aSpaceID,
                                                 aSegHandle,
                                                 SD_MAKE_PID( sPrvAllocExtRID ),
                                                 sFreeListIdx,
                                                 &sAllocExtRID,
                                                 &sFstPIDOfExt,
                                                 &sFstDataPIDOfExt ) 
                      != IDE_SUCCESS );

            IDE_ASSERT( sFstDataPIDOfExt != SD_MAKE_PID( sAllocExtRID ) );
        }
        else
        {
                /* nothing to do */
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
        *aFstPIDOfAllocExt = aFstPIDOfPrvAllocExt;
        *aAllocExtRID      = aPrvAllocExtRID;

        IDE_ASSERT( *aAllocPID != SD_MAKE_PID( aPrvAllocExtRID ) );
    }

    aParentInfo->mParentPID   = *aAllocExtRID;
    aParentInfo->mIdxInParent = SDPSC_INVALID_IDX;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Segment의 ExtDir 페이지의 PID를 설정한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mtx 포인터
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aToLstExtDir - [IN] Next Extent Dir. 페이지 ID를 설정할 마지막 Extent Dir.
 *                     페이지의 PageID
 * aPageID      - [IN] 페이지 ID
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::setNxtExtDir( idvSQL          * aStatistics,
                                  sdrMtx          * aMtx,
                                  scSpaceID         aSpaceID,
                                  scPageID          aToLstExtDir,
                                  scPageID          aPageID )
{
    sdpscExtDirCntlHdr  * sCntlHdr;

    IDE_ASSERT( aToLstExtDir != SD_NULL_PID );
    IDE_ASSERT( aPageID      != SD_NULL_PID );
    IDE_ASSERT( aMtx         != NULL );

    IDE_TEST( fixAndGetCntlHdr4Write( aStatistics,
                                      aMtx,
                                      aSpaceID,
                                      aToLstExtDir,
                                      &sCntlHdr ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(sCntlHdr->mNxtExtDir),
                                         &aPageID,
                                         ID_SIZEOF( scPageID ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Segment의 ExtDir를 강제적으로 FULL 상태를 만든다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mtx 포인터
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aExtDir      - [IN] FULL 상태를 만들 Extent Dir. Page ID
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::makeExtDirFull( idvSQL     * aStatistics,
                                    sdrMtx     * aMtx,
                                    scSpaceID    aSpaceID,
                                    scPageID     aExtDir )
{
    sdpscExtDirCntlHdr  * sCntlHdr;

    IDE_ASSERT( aExtDir != SD_NULL_PID );
    IDE_ASSERT( aMtx    != NULL );

    IDE_TEST( fixAndGetCntlHdr4Write( aStatistics,
                                      aMtx,
                                      aSpaceID,
                                      aExtDir,
                                      &sCntlHdr ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(sCntlHdr->mMaxExtCnt),
                                         &sCntlHdr->mExtCnt,
                                         ID_SIZEOF( sCntlHdr->mExtCnt ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 갱신목적으로 ExtDir 페이지의 Control Header를 fix한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mtx 포인터
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aExtDirPID   - [IN] X-Latch를 획득할 Extent Dir. 페이지의 PID
 * aCntlHdr     - [OUT] Extnet Dir.페이지의 Control 헤더 포인터
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::fixAndGetCntlHdr4Write( idvSQL              * aStatistics,
                                            sdrMtx              * aMtx,
                                            scSpaceID             aSpaceID,
                                            scPageID              aExtDirPID,
                                            sdpscExtDirCntlHdr ** aCntlHdr )
{
    idBool               sDummy;
    UChar              * sPagePtr;
    sdpPageType          sPageType;
    sdpscSegMetaHdr    * sSegHdr;

    IDE_ASSERT( aExtDirPID != SD_NULL_PID );
    IDE_ASSERT( aCntlHdr   != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aExtDirPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL )
              != IDE_SUCCESS );

    sPageType = sdpPhyPage::getPageType( (sdpPhyPageHdr*)sPagePtr );

    if ( sPageType == SDP_PAGE_CMS_SEGHDR )
    {
        sSegHdr   = sdpscSegHdr::getHdrPtr( sPagePtr );
        *aCntlHdr = sdpscSegHdr::getExtDirCntlHdr( sSegHdr );
    }
    else
    {
        IDE_ASSERT( sPageType == SDP_PAGE_CMS_EXTDIR );
        *aCntlHdr = sdpscExtDir::getHdrPtr( sPagePtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Read 목적으로 ExtDir 페이지의 Control Header를 fix한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mtx 포인터
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aExtDirPID   - [IN] S-Latch를 획득할 Extent Dir. 페이지의 PID
 * aCntlHdr     - [OUT] Extnet Dir.페이지의 Control 헤더 포인터
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::fixAndGetCntlHdr4Read( idvSQL              * aStatistics,
                                           sdrMtx              * aMtx,
                                           scSpaceID             aSpaceID,
                                           scPageID              aExtDirPID,
                                           sdpscExtDirCntlHdr ** aCntlHdr )
{
    idBool            sDummy;
    UChar           * sPagePtr;
    sdpPageType       sPageType;
    sdpscSegMetaHdr * sSegHdr;

    IDE_ASSERT( aExtDirPID != SD_NULL_PID );
    IDE_ASSERT( aCntlHdr   != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aExtDirPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL )
              != IDE_SUCCESS );

    sPageType = (sdpPageType)sdpPhyPage::getPhyPageType( sPagePtr );

    if ( sPageType == SDP_PAGE_CMS_SEGHDR )
    {
        sSegHdr   = sdpscSegHdr::getHdrPtr( sPagePtr );
        *aCntlHdr = sdpscSegHdr::getExtDirCntlHdr( sSegHdr );
    }
    else
    {
        IDE_ASSERT( sPageType == SDP_PAGE_CMS_EXTDIR );

        *aCntlHdr = sdpscExtDir::getHdrPtr( sPagePtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : [ INTERFACE ] Extent에서 다음 Data 페이지를 반환한다.
 *
 * aStatistics  - [IN] 통계정보
 * aSpaceID     - [IN] 테이블스페이스 ID
 * aSegInfo     - [IN] 해당 세그먼트 정보 포인터
 * aSegCacheInfo- [IN] 해당 세그먼트 캐시 정보 포인터
 * aExtRID      - [IN/OUT] 현재 Extent Desc.의 RID
 * aExtInfo     - [IN/OUT] 현재 Extent Desc.의 정보
 * aPageID      - [OUT] 다음 할당된 페이지의 IDx
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::getNxtAllocPage( idvSQL             * aStatistics,
                                     scSpaceID            aSpaceID,
                                     sdpSegInfo         * aSegInfo,
                                     sdpSegCacheInfo    * /*aSegCacheInfo*/,
                                     sdRID              * aExtRID,
                                     sdpExtInfo         * aExtInfo,
                                     scPageID           * aPageID )
{
    sdRID               sNxtExtRID;

    IDE_ASSERT( aExtInfo    != NULL );

    /*
     * BUG-24566 UDS/TSS 세그먼트의 GetNxtAllocPage에서
     * Undo/TSS 페이지만을 반환하도록 수정해야함.
     *
     * aPageID 반환은 Data 페이지만을 반환한다. 왜냐하면,
     * 상위 모듈에서 세그먼트의 메타페이지를 접근할 수 없으며, 일반적으로
     * 할 필요도 없다. 부득이하게 메타에 대한 특정정보를 얻기위한 것이라면,
     * 별도의 인터페이스가 존재한다.
     */

    if ( *aPageID == SD_NULL_PID )
    {
        *aPageID = aExtInfo->mFstDataPID;
    }
    else
    {
        while ( 1 )
        {
            // 현재 Extent 범위를 넘어서는 페이지이면,
            // 다음 extent 정보를 얻어야 한다.
            if( *aPageID == aExtInfo->mLstPID )
            {
                IDE_TEST( getNxtExtRID( aStatistics,
                                        aSpaceID,
                                        aSegInfo->mSegHdrPID,
                                        *aExtRID,
                                        &sNxtExtRID )
                          != IDE_SUCCESS );

                if( sNxtExtRID == SD_NULL_RID )
                {
                    // 마지막 Extent Descriptor인 경우에는
                    // 첫번째 Extent Descriptor를 반환한다.
                    sNxtExtRID = aSegInfo->mFstExtRID;
                }

                IDE_TEST( getExtInfo( aStatistics,
                                      aSpaceID,
                                      sNxtExtRID,
                                      aExtInfo )
                          != IDE_SUCCESS );

                *aExtRID = sNxtExtRID;
                *aPageID = aExtInfo->mFstDataPID;
                break;
            }
            else
            {
                *aPageID += 1;
                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir 단위로 할당시도
 *
 * 첫번째, 세그먼트 내에서의 다음 ExtDir를 할당 할수 있는지 확인하고 할당한다.
 * 두번째, 테이블스페이스로부터 Free된 ExtDir 할당한다.
 *
 * aStatistics      - [IN]  통계정보
 * aStartInfo       - [IN]  Mtx 시작 정보
 * aSpaceID         - [IN]  테이블스페이스 ID
 * aSegCache        - [IN]  세그먼트 Cache 포인터
 * aSegPID          - [IN]  세그먼트 헤더 페이지 ID
 * aFreeListIdx     - [IN]  Free Extent Dir. List 타입번호
 * aNxtExtDirPID    - [IN]  다음 ExtDir 페이지의 PID
 * aAllocExtDirInfo - [OUT] 할당 가능한 ExtDir 페이지에 할당에 대한 정보
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::tryAllocExtDir( idvSQL               * aStatistics,
                                    sdrMtxStartInfo      * aStartInfo,
                                    scSpaceID              aSpaceID,
                                    sdpscSegCache        * aSegCache,
                                    scPageID               aSegPID,
                                    sdpFreeExtDirType      aFreeListIdx,
                                    scPageID               aNxtExtDirPID,
                                    sdpscAllocExtDirInfo * aAllocExtDirInfo )
{
    smSCN            sMyFstDskViewSCN;
    smSCN            sSysMinDskViewSCN;
    SInt             sLoop;
    scPageID         sExtDirPID;
    scPageID         sNxtExtDirPID;
    sdpscExtDesc     sFstExtDesc;
    sdRID            sFstExtDescRID;
    sdpscExtDirState sExtDirState[2];
    UInt             sExtCntInExtDir[2];

    IDE_ASSERT( aSegCache        != NULL );
    IDE_ASSERT( aAllocExtDirInfo != NULL );
    IDE_ASSERT( aNxtExtDirPID    != SD_NULL_PID );

    sMyFstDskViewSCN = smxTrans::getFstDskViewSCN( aStartInfo->mTrans );
    SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

    initAllocExtDirInfo( aAllocExtDirInfo );

    /* A. Next ExtDir 페이지를 재사용할 수 있는지 확인한다.
     *    또한 NextNext ExtDir페이지도 재사용가능한지 확인하여
     *    Shrink 할 수 있다면 Shrink 한다. */
    for ( sLoop = 0, sExtDirPID = aNxtExtDirPID; sLoop < 2; sLoop++ )
    {
        IDE_TEST( checkExtDirState4Reuse( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          sExtDirPID,
                                          &sMyFstDskViewSCN,
                                          &sSysMinDskViewSCN,
                                          &sExtDirState[sLoop],
                                          &sFstExtDescRID,
                                          &sFstExtDesc,
                                          &sNxtExtDirPID,
                                          &sExtCntInExtDir[sLoop] )
                  != IDE_SUCCESS );

        if ( sExtDirState[sLoop] != SDPSC_EXTDIR_EXPIRED )
        {
            break;
        }

        IDE_ASSERT( sFstExtDescRID != SD_NULL_RID );
        IDE_ASSERT( sNxtExtDirPID  != SD_NULL_PID );

        if ( sLoop > 0 )
        {
            // Nxt ExtDir은 Shrink 가능하고, NxtNxt ExtDir 페이지를 재사용한다.
            aAllocExtDirInfo->mShrinkExtDirPID      =
                                           aAllocExtDirInfo->mNewExtDirPID;
            aAllocExtDirInfo->mExtCntInShrinkExtDir = 
                                           sExtCntInExtDir[0];
        }

        /* 만약 NxtNxt ExtDir 페이지가 재사용 불가능하다면
         * Nxt ExtDir 페이지를 재사용한다. */
        aAllocExtDirInfo->mFstExtDescRID     = sFstExtDescRID;
        aAllocExtDirInfo->mFstExtDesc        = sFstExtDesc;
        aAllocExtDirInfo->mNewExtDirPID      = sExtDirPID;
        aAllocExtDirInfo->mNxtPIDOfNewExtDir = sNxtExtDirPID;

        if ( (aAllocExtDirInfo->mNewExtDirPID   == aSegPID)   ||
             (isExeedShrinkThreshold(aSegCache) == ID_FALSE) )
        {
            // Next ExtDir 페이지가 재사용은 가능한데 SegHdr 페이지이거나
            // SHRINK_THREADHOLD를 만족하지 못하면 Shrink는 불가능하다.
            break;
        }

        sExtDirPID = sNxtExtDirPID; // Go Go Next!!
    }

    if ( sExtDirState[0] != SDPSC_EXTDIR_EXPIRED )
    {
        IDE_ASSERT( aAllocExtDirInfo->mShrinkExtDirPID   == SD_NULL_PID );
        IDE_ASSERT( aAllocExtDirInfo->mNewExtDirPID      == SD_NULL_PID );
        IDE_ASSERT( aAllocExtDirInfo->mNxtPIDOfNewExtDir == SD_NULL_PID );

        if ( sExtDirState[0] == SDPSC_EXTDIR_UNEXPIRED )
        {
            /* B. 재사용할 수 없다면 테이블 스페이스로부터 ExtDir를 할당할수 있는지
             * 확인한다. 할당된 ExtDir.에는 Extent가 가득 차있다.
             * 하지만, 프로퍼티가 변경이 가능하여, 세그먼트 생성시점의 MaxExtInDir
             * 개수와는 다를수 있다. ( 재구동후 Undo TBS가 Reset이 안된 경우 ) */
            IDE_TEST( sdptbExtent::tryAllocExtDir(
                                      aStatistics,
                                      aStartInfo,
                                      aSpaceID,
                                      aFreeListIdx,
                                      &aAllocExtDirInfo->mNewExtDirPID )
                      != IDE_SUCCESS );

            if ( aAllocExtDirInfo->mNewExtDirPID != SD_NULL_PID )
            {
                aAllocExtDirInfo->mIsAllocNewExtDir = ID_TRUE;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            IDE_ASSERT( sExtDirState[0] == SDPSC_EXTDIR_PREPARED );

            /* C. prepareNewPage4Append에 의해서 이미 세그먼트에 할당된 경우는
             *    사용가능하다. (Next ExtDir가 SegHdr가 아님.)
             *
             *  BUG-25352 Index SMO 과정에서 prepareNewPage4Append로 인해
             *            UDS의 ExtDirList 상 NxtExtDir의 ExtDesc가 Full이
             *            아닐수 있음.
             *
             * prepareNewPage4Append로 미리 확보된 ExtDir의 경우에는 Segment에 이미
             * 추가되어 있긴하지만(공간확보만한다), Segment의 런타임 할당정보를 갱신하지
             * 않는다. 해당 ExtDir의 첫번째 ExtDesc 정보만 넘겨주면 된다.
             */
            aAllocExtDirInfo->mNewExtDirPID     = aNxtExtDirPID;
            aAllocExtDirInfo->mIsAllocNewExtDir = ID_FALSE;
        }

        if ( aAllocExtDirInfo->mNewExtDirPID != SD_NULL_PID )
        {
            IDE_TEST( getExtDescInfo( aStatistics,
                                      aSpaceID,
                                      aAllocExtDirInfo->mNewExtDirPID,
                                      0,  /* aIdx */
                                      &sFstExtDescRID,
                                      &sFstExtDesc ) != IDE_SUCCESS );

            aAllocExtDirInfo->mFstExtDescRID = sFstExtDescRID;
            aAllocExtDirInfo->mFstExtDesc    = sFstExtDesc;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir을 재사용할 수 있는지 확인
 *
 * ExtDir의 LatestCSCN과 FstDskViewSCN을 비교하여 다음조건을 만족하면
 * 재사용(OverWrite) 가능하다.
 *
 * (1) Segment를 확장하려는 트랜잭션의 MyFstDskViewSCN이 Extent Cntl Header의
 *     FstDskViewSCN과 일단 달라야 한다.
 *     왜냐하면, 자신이 할당한 Extent Dir. 페이지일 수 있기 때문이다.
 *
 *     aMyFstDskViewSCN != ExtDir.FstDskViewSCN
 *
 * (2) Segment를 확장하려는 트랜잭션이 알고 있는 aSysMinDskViewSCN이 Extent Dir.
 *     Page를 마지막으로 사용한 트랜잭션이 Commit 과정에서 Extent Cntl
 *     Header에 갱신했던 CommitSCN보다 커야 한다.
 *
 *     aSysMinDskViewSCN  > ExtDir.LatestCSCN
 *
 * aStatistics     - [IN]  통계정보
 * aSpaceID        - [IN]  테이블스페이스 ID
 * aCurExtDir      - [IN]  현재 Extent Dir. 페이지 ID (steal 대상)
 * aFstDskViewSCN  - [IN]  트랜잭션 첫번째 Dsk Stmt Begin SCN
 * aMinDskViewSCN  - [IN]  트랜잭션 시작시 시스템에서 제일 오랜된 SSCN
 * aExtDirState    - [OUT] 재사용가능여부
 * aAllocExtRID    - [OUT] 할당한 Extent Desc. RID
 * aFstExtDesc     - [OUT] 할당한 첫번째 Extent Desc. 포인터
 * aNxtExtDir      - [OUT] 다음 Extent Dir. 페이지 ID
 * aExtCntInExtDir - [OUT] aCurExtDir에 포함된 Extent 개수.
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::checkExtDirState4Reuse(
                                 idvSQL              * aStatistics,
                                 scSpaceID             aSpaceID,
                                 scPageID              aSegPID,
                                 scPageID              aCurExtDir,
                                 smSCN               * aMyFstDskViewSCN,
                                 smSCN               * aSysMinDskViewSCN,
                                 sdpscExtDirState    * aExtDirState,
                                 sdRID               * aAllocExtRID,
                                 sdpscExtDesc        * aFstExtDesc,
                                 scPageID            * aNxtExtDir,
                                 UInt                * aExtCntInExtDir )
{
    sdpscExtDirCntlHdr   * sCntlHdr       = NULL;
    sdpscExtDesc         * sFstExtDescPtr = NULL;
    UInt                   sState         = 0;

    IDE_ASSERT( aSegPID           != SD_NULL_PID );
    IDE_ASSERT( aCurExtDir        != SD_NULL_PID );
    IDE_ASSERT( aMyFstDskViewSCN  != NULL );
    IDE_ASSERT( aSysMinDskViewSCN != NULL );
    IDE_ASSERT( aExtDirState      != NULL );
    IDE_ASSERT( aNxtExtDir        != NULL );

    *aExtDirState    = SDPSC_EXTDIR_UNEXPIRED;
    *aNxtExtDir      = SD_NULL_PID;

    if ( aFstExtDesc != NULL )
    {
        *aAllocExtRID = SD_NULL_RID;
        initExtDesc( aFstExtDesc );
    }

    if ( aExtCntInExtDir != NULL )
    {
        *aExtCntInExtDir = 0;
    }

    IDE_TEST( fixAndGetCntlHdr4Read( aStatistics,
                                     NULL, /* aMtx */
                                     aSpaceID,
                                     aCurExtDir,
                                     &sCntlHdr ) != IDE_SUCCESS );
    sState = 1;

    IDE_ASSERT( sCntlHdr->mExtCnt > 0 );

    /* BUG-25352 Index SMO과정에서 prepareNewPage4Append로 인해 UDS의 ExtDirList
     *           상에 NxtExtDir이 Extent가 Full이 아닌 경우가 있음. */
    if ( sCntlHdr->mExtCnt != sCntlHdr->mMaxExtCnt )
    {
        /* BUG-34050 when there are XA prepared transactions
         *           and server restart, not reusable undo page is reused.
         * XA prepared transaction이 존재하는 경우
         * undo를 리셋하지 않기 때문에 restart이전에 사용되던 extent 구조가
         * 그대로 존재하게 되고, 또한 사용중인 undo 페이지가 flush되면
         * mFstDskViewSCN, mLstCommitSCN이 InitSCN이 아닐 수 있다.
         * 이런 경우는 prepareNewPage4Append 연산으로 미리 할당된 extent라고
         * 보기 어렵지만 prepared 상태로 리턴하여 재사용되지 않도록 한다. */
        *aExtDirState = SDPSC_EXTDIR_PREPARED;
        IDE_CONT( cont_cannot_reusable );
    }
    else
    {
        /* ExtDir의 MaxExtCnt가 프로퍼티에 따라서 1개로 설정되기도 하기때문에
         * ExtCnt == MaxExtCnt 경우가 있으며 다음 조건을 만족하면
         * prepareNewPage4Append에 의해 확보된 페이지로 판단한다. */
        if ( (sCntlHdr->mExtCnt == 1) &&
             (SM_SCN_IS_INIT(sCntlHdr->mFstDskViewSCN)) &&
             (SM_SCN_IS_INIT(sCntlHdr->mLstCommitSCN)))
        {
            *aExtDirState = SDPSC_EXTDIR_PREPARED;
            IDE_CONT( cont_cannot_reusable );
        }
    }

    IDE_TEST_CONT( 
            SM_SCN_IS_EQ( &sCntlHdr->mFstDskViewSCN, aMyFstDskViewSCN ),
            cont_cannot_reusable );

    IDE_TEST_CONT( 
            SM_SCN_IS_GE( &sCntlHdr->mLstCommitSCN, aSysMinDskViewSCN ),
            cont_cannot_reusable );

    *aExtDirState    = SDPSC_EXTDIR_EXPIRED;
    *aNxtExtDir      = sCntlHdr->mNxtExtDir;

    if ( aFstExtDesc != NULL )
    {
        sFstExtDescPtr = getExtDescByIdx( sCntlHdr, 0 );
        idlOS::memcpy( aFstExtDesc,
                       sFstExtDescPtr,
                       ID_SIZEOF(sdpscExtDesc) );
        *aAllocExtRID = sdpPhyPage::getRIDFromPtr( sFstExtDescPtr );
    }

    if ( aExtCntInExtDir != NULL )
    {
        *aExtCntInExtDir = sCntlHdr->mExtCnt;
    }

    IDE_EXCEPTION_CONT( cont_cannot_reusable );

    sState = 0;
    IDE_TEST( releaseCntlHdr( aStatistics, sCntlHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( releaseCntlHdr( aStatistics, sCntlHdr )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir에 마지막 사용한 트랜잭션의 CommitSCN을 설정함.
 *
 * CMS에서 ExtDir의 CommitSCN을 표시하여 이후 트랜잭션이 ExtDir를 재사용하려고
 * 할때 판단기준으로 사용한다.
 *
 * aCntlHdr   - [IN] Extent Dir. 페이지의 Control 헤더 포인터
 * aCommitSCN - [IN] Extent Dir. 페이지에 설정할 트랜잭션의 CommitSCN
 *
 ***********************************************************************/
void sdpscExtDir::setLatestCSCN( sdpscExtDirCntlHdr * aCntlHdr,
                                 smSCN              * aCommitSCN )
{
     SM_SET_SCN( &(aCntlHdr->mLstCommitSCN),
                 aCommitSCN );
}

/***********************************************************************
 *
 * Description : ExtDir를 사용한 트랜잭션의 FstDskViewSCN을 설정함.
 *
 * 자신이 사용하고 하는 ExtDir를 표시하여 덮어쓰기하지 않도록 하기 위함이다.
 *
 * aCntlHdr         - [IN] Extent Dir. 페이지의 Control 헤더 포인터
 * aMyFstDskViewSCN - [IN] Extent Dir. 페이지에 설정할 트랜잭션의 FstDskViewSCN
 *
 ***********************************************************************/
void sdpscExtDir::setFstDskViewSCN( sdpscExtDirCntlHdr * aCntlHdr,
                                    smSCN              * aMyFstDskViewSCN )
{
    SM_SET_SCN( &(aCntlHdr->mFstDskViewSCN), aMyFstDskViewSCN );
}

/***********************************************************************
 *
 * Description : 트랜잭션 Commit/Abort시 사용한 ExtDir 페이지에 SCN을 설정
 *
 * 자신이 사용하고 하는 ExtDir를 표시하여 덮어쓰기하지 않도록 하기 위함이다.
 * FstExtRID를 포함하는 Extent Dir.부터 LstExtRID를 포함하는 Extent Dir.
 * 까지 CSCN 또는 ASCN 을 설정한다.
 * No-Logging 으로 설정하고 페이지를 dirty시킨다.
 *
 * aStatistics     - [IN] 통계정보
 * aSpaceID        - [IN] 테이블스페이스 ID
 * aSegPID         - [IN] 세그먼트 페이지 ID
 * aFstExtRID      - [IN] 트랜잭션이 사용한 사용한 첫번째 Extent Desc. RID
 * aLstExtRID      - [IN] 트랜잭션이 사용한 사용한 마지막 Extent Desc. RID
 * aCSCNorASCN     - [IN] Extent Dir. 페이지에 설정할 트랜잭션의 CommitSCN
 *                        또는 AbortSCN(GSCN)
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::markSCN4ReCycle( idvSQL          * aStatistics,
                                     scSpaceID         aSpaceID,
                                     scPageID          aSegPID,
                                     sdpSegHandle    * aSegHandle,
                                     sdRID             aFstExtRID,
                                     sdRID             aLstExtRID,
                                     smSCN           * aCSCNorASCN )
{
    scPageID              sCurExtDir;
    scPageID              sLstExtDir;
    scPageID              sNxtExtDir;
    UInt                  sState   = 0;
    sdpscExtDirCntlHdr  * sCntlHdr = NULL;

    IDE_ASSERT( aSegPID    != SD_NULL_PID );
    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aFstExtRID != SD_NULL_RID );
    IDE_ASSERT( aLstExtRID != SD_NULL_RID );
    IDE_ASSERT( aCSCNorASCN != NULL );
    IDE_ASSERT( SM_SCN_IS_NOT_INIT( *aCSCNorASCN ) );

    sCurExtDir = SD_MAKE_PID( aFstExtRID );
    sLstExtDir = SD_MAKE_PID( aLstExtRID );

    while ( 1 )
    {
        IDE_TEST( fixAndGetCntlHdr4Write( aStatistics,
                                          NULL,  /* aMtx */
                                          aSpaceID,
                                          sCurExtDir,
                                          &sCntlHdr )
                  != IDE_SUCCESS );
        sState = 1;

        setLatestCSCN( sCntlHdr, aCSCNorASCN );

        sNxtExtDir = sCntlHdr->mNxtExtDir;

        sdbBufferMgr::setDirtyPageToBCB(
                                aStatistics,
                                sdpPhyPage::getPageStartPtr( sCntlHdr ) );

        sState = 0;
        IDE_TEST( releaseCntlHdr( aStatistics, sCntlHdr )
                  != IDE_SUCCESS );

        if ( sCurExtDir == sLstExtDir )
        {
            break;
        }

        sCurExtDir = sNxtExtDir;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( releaseCntlHdr( aStatistics, sCntlHdr )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * BUG-31055 Can not reuse undo pages immediately after it is used to 
 *           aborted transaction.
 *
 * Description : ED들을 Tablespace로 반환함.
 *
 * Abort된 Transaction이 사용한 ED들을 즉시 재활용하기 위해,  TableSpace에게
 * 반환한다.
 *
 * aStatistics     - [IN] 통계정보
 * aSpaceID        - [IN] 테이블스페이스 ID
 * aSegPID         - [IN] 세그먼트 페이지 ID
 * aSegHandle      - [IN] 세그먼트 핸들
 * aStartInfo      - [IN] Mtx 시작 정보
 * aFreeListIdx    - [IN] FreeList 번호 (Tran or Undo )
 * aFstExtRID      - [IN] 반환할 첫번째 Extent Desc. RID
 * aLstExtRID      - [IN] 반환할 마지막 Extent Desc. RID
 *
 * Issue :
 * Segment의 ExtentDirectoryList는 Singly-linked list로 관리된다.
 *
 * [First] -> [ A ] -> [ B ] -> [ C ] -> [ Last ] ->...
 * 
 * 1) First와 Last는 shrink하면 안된다.
 *    First의 경우, 앞 Transaction의 UndoRecord가 들어있기 때문이다.
 *    Last의 경우, 현재 커서가 가리키고 있기 때문에 곧 다시 사용될 것이기
 *    때문이다.
 *
 * 2) 한 페이지를 shrink하려면, PrevPID, CurrPID, NextPID가 필요하다.
 *    PrevPID의 ED가 NextPID를 가리키도록 수정한다.
 *    CurrPID는 Tablespace에 반환된다.
 *    또한 CurrPID가 SegHdr내 LastED일 경우, SegHdr의 mLastED를 갱신한다.
 *
 * 3) Shrink Threshold는 무시하고 모두 shrink한다.
 *    기본적으로 Seg 크기가 Shrink Threshold 이하일 경우, shrink 하지 않는다.
 *    하지만 Abort된 Transaction의 ED는 무조건 필요없는 UndoRecord를 갖고
 *    있기 때문에, 무조건 반환시켜 재활용될 수 있도록 한다.
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::shrinkExts( idvSQL            * aStatistics,
                                scSpaceID           aSpaceID,
                                scPageID            aSegPID,
                                sdpSegHandle      * aSegHandle,
                                sdrMtxStartInfo   * aStartInfo,
                                sdpFreeExtDirType   aFreeListIdx,
                                sdRID               aFstExtRID,
                                sdRID               aLstExtRID )
{
    scPageID              sCurExtDir;
    scPageID              sLstExtDir;
    scPageID              sPrvExtDir = SC_NULL_PID;
    scPageID              sNxtExtDir;
    UInt                  sState     = 0;
    sdpscExtDirCntlHdr  * sCntlHdr   = NULL;
    sdrMtx                sMtx;
    UInt                  sTotExtCnt = 0;
    UInt                  sCheckCnt  = 0;

    IDE_ASSERT( aSegPID    != SD_NULL_PID );
    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aFstExtRID != SD_NULL_RID );
    IDE_ASSERT( aLstExtRID != SD_NULL_RID );

    /* 1. Initialize */
    sPrvExtDir = SD_MAKE_PID( aFstExtRID );
    sLstExtDir = SD_MAKE_PID( aLstExtRID );

    /* First와 Last가 동일할 경우, Skip */
    IDE_TEST_CONT( SD_MAKE_PID( aFstExtRID ) == SD_MAKE_PID( aLstExtRID ),
                    SKIP );

    /* 2. Set CurExtDir
     * Cur = Prv->Next; */
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( fixAndGetCntlHdr4Read( aStatistics,
                                     &sMtx,
                                     aSpaceID,
                                     sPrvExtDir,
                                     &sCntlHdr )
              != IDE_SUCCESS );
    sCurExtDir = sCntlHdr->mNxtExtDir;

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    /* First 바로 다음이 Last일 경우도 Skip.
     * First와 Last 사이의 ExtDir들을 shrink해야 한다. */
    IDE_TEST_CONT( sCurExtDir == sLstExtDir, SKIP );

    /* 3. Loop */
    while ( 1 )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aStartInfo,
                                       ID_FALSE,/*Undoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( fixAndGetCntlHdr4Write( aStatistics,
                                          &sMtx,
                                          aSpaceID,
                                          sCurExtDir,
                                          &sCntlHdr )
                  != IDE_SUCCESS );

        /* 4. NXT = CUR->NXT */
        sNxtExtDir = sCntlHdr->mNxtExtDir;

        /* 다음 상황은 있을 수 없다. */
        IDE_TEST_RAISE( sCurExtDir == sPrvExtDir, ERR_ASSERT );
        IDE_TEST_RAISE( sCurExtDir == sNxtExtDir, ERR_ASSERT );
        IDE_TEST_RAISE( sPrvExtDir == sNxtExtDir, ERR_ASSERT );
        IDE_TEST_RAISE( sCurExtDir == sLstExtDir, ERR_ASSERT );

        /* 5. if CUR != SEGHDR
         * Segment Header는 shrink하면 안된다. */
        if ( sCurExtDir != aSegPID )
        {
            /* 6. PRV -> NXT = NXT */
            IDE_TEST( sdpscSegHdr::removeExtDir( aStatistics,
                                                 &sMtx,
                                                 aSpaceID,
                                                 aSegPID,
                                                 sPrvExtDir,
                                                 sCurExtDir,
                                                 sNxtExtDir,
                                                 sCntlHdr->mExtCnt,
                                                 &sTotExtCnt )
                      != IDE_SUCCESS );

            /* 하나의 Extent도 없는 상황은 될 수 없습니다. */
            /* Rollback과정에서 호출되는 함수입니다.
             * 실패하면 예외처리를 할 수 없기 때문에 사망합니다. */
            IDE_TEST_RAISE( sTotExtCnt == 0, ERR_ASSERT );

            IDE_TEST( sdptbExtent::freeExtDir( aStatistics,
                                               &sMtx,
                                               aSpaceID,
                                               aFreeListIdx,
                                               sCurExtDir )
                      != IDE_SUCCESS );
        }
        else
        {
            /* 7. PRV = CUR; */
            /* SegHdr를 Skip했기 때문에, Prv를 옮겨야 한다.
            *
             * 예)
             * 0 -> 1 -> 2 -> 3 -> 0   (0이 ExtDir)
             * 3 = Prv, 0 = Cur, 1 = Nxt인 상황
             *
             * 이 상황에서 Cur인 0은 Seghdr이기 때문에 Shrink하면 안된다.
             * 따라서 Skip후 1을 Shrink해야 하는 상황으로 만들어야 한다.
             * 그러므로,
             * 0 = Prv, 1 = Cur, 2 = Nxt인 상황으로 만들어야 한다. */
            sPrvExtDir = sCurExtDir;
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        /* 8. if( NXT == LST) Break */
        /* 다음 ED가 마지막이면 종료한다. */
        if ( sNxtExtDir == sLstExtDir )
        {
            break;
        }
        else
        {
            /* nothing to do */
        }

        /* 9. CUR = NXT */
        sCurExtDir = sNxtExtDir;

        sCheckCnt++;
        IDE_TEST_RAISE( sCheckCnt >= ( ID_UINT_MAX / 8 ), ERR_ASSERT );
        /* UndoED의 최소 크기는 Page 8개 이다.
         * (SYS_UNDO_TBS_EXTENT_SIZE의 실질적인 최소값이 65536이다)
         * 따라서 그 이상 루프를 돌고 있다는 것은 무한루프이다.
         * 이 경우 UndoTBS가 꼬여있을 것이기 때문에 사망시킨다. */
        /* 이 경우 MiniTransaction이 엄청난 Log를 남겼을 것이기
           때문에, 이를 보면 검증 가능하다. */
    }

    /* BUG-31171 set incorrect undo segment size when shrinking 
     * undo extent for aborted transaction. */
    /* Segment Cache에 저장되는 Segment Size는 이후 isExeedShrinkThreshold
     * 를 통해 Shrink 여부를 판단할때 이용된다. */
    if ( sTotExtCnt > 0 )
    {
        sdpscCache::setSegSizeByBytes(
            aSegHandle,
            ((sdpscSegCache*)aSegHandle->mCache)->mCommon.mPageCntInExt 
            * SD_PAGE_SIZE * sTotExtCnt );
    }
    else
    {
        /* sTotExtCnt가 0개란 이야기는 shrink를 하지 않았기에 설정되지
         * 않았다는 뜻이다. 따라서 아무것도 할 것이 없다. */
    }

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ASSERT );
    {
        ideLog::log( IDE_SERVER_0,
                     "aSpaceID          =%u\n"
                     "aSegPID           =%u\n"
                     "sPrvExtDir        =%u\n"
                     "sCurExtDir        =%u\n"
                     "sNxtExtDir        =%u\n"
                     "sCntlHdr->mExtCnt =%u\n"
                     "aFstExtRID        =%llu\n"
                     "aLstExtRID        =%llu\n"
                     "sTotExtCnt        =%u\n"
                     "sCheckCnt         =%u\n",
                     aSpaceID,
                     aSegPID,
                     sPrvExtDir,
                     sCurExtDir,
                     sNxtExtDir,
                     sCntlHdr->mExtCnt,
                     aFstExtRID,
                     aLstExtRID,
                     sTotExtCnt,
                     sCheckCnt );

        IDE_ASSERT( 0 );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : ExtDir을 split 해서 뒤쪽 ExtDir을 shrink 시킴
 *                      
 * aStatistics     - [IN] 통계정보
 * aSpaceID        - [IN] 테이블스페이스 ID
 * aToSegPID       - [IN] 세그먼트 페이지 ID
 * aToExtDirPID    - [IN] split 대상 ExtDirPID
 ***********************************************************************/
IDE_RC sdpscExtDir::shrinkExtDir( idvSQL                * aStatistics,
                                  sdrMtx                * aMtx,
                                  scSpaceID               aSpaceID,
                                  sdpSegHandle          * aToSegHandle,
                                  sdpscAllocExtDirInfo  * aAllocExtDirInfo )
{
    sdpSegType           sSegType;
    UChar              * sExtDirPagePtr;
    scPageID             sNewExtDirPID;
    sdpscExtDirCntlHdr * sNewExtDirCntlHdr;
    sdpscExtDirCntlHdr * sAllocExtDirCntlHdr;
    sdRID                sExtDescRID;
    sdRID                sNxtExtRID;
    sdpscExtDesc         sExtDesc;
    sdpFreeExtDirType    sFreeListIdx;
    SInt                 i;
    SInt                 sLoop;
    UShort               sMaxExtDescCnt;
    UInt                 sExtCnt;
    SInt                 sDebugLoopCnt;
    UInt                 sDebugTotalExtDescCnt;
    smLSN                sNTA;
    void               * sTrans;

    /* rp에서는 aStatistics이 null로 넘어올수 있다. */
    IDE_ERROR( aMtx != NULL );
    IDE_ERROR( aToSegHandle != NULL );
    IDE_ERROR( aAllocExtDirInfo != NULL );
    
    sSegType = ((sdpscSegCache*)aToSegHandle->mCache)->mCommon.mSegType; 
    sTrans   = sdrMiniTrans::getTrans( aMtx );
    sNTA     = smLayerCallback::getLstUndoNxtLSN( sTrans );

    IDE_ERROR( (sSegType == SDP_SEG_TYPE_TSS) ||
               (sSegType == SDP_SEG_TYPE_UNDO) );

    if ( sSegType == SDP_SEG_TYPE_TSS )
    {
        sFreeListIdx = SDP_TSS_FREE_EXTDIR_LIST;
    }
    else
    {
        sFreeListIdx = SDP_UDS_FREE_EXTDIR_LIST;
    }

    sMaxExtDescCnt = sdpscSegHdr::getMaxExtDescCnt( sSegType );

    /* TargetExtDir과 To Segment 의 MaxExtCnt 가 다르다면
       서로 다른 타입의 Segment에서 steal을 한 경우가 된다.
       split해서 To.mMaxExtCnt이후 Ext를 freelist에 매단다.  */
    if ( aAllocExtDirInfo->mExtCntInShrinkExtDir > sMaxExtDescCnt )
    {
        /* NewExtDir 의 ExtDirCntlHdr 를 얻어 놓고 시작 */
        IDE_TEST( fixAndGetCntlHdr4Write( aStatistics,
                                          aMtx,  /* aMtx */
                                          aSpaceID,
                                          aAllocExtDirInfo->mNewExtDirPID,
                                          &sAllocExtDirCntlHdr )
                   != IDE_SUCCESS );

        sLoop = ( aAllocExtDirInfo->mExtCntInShrinkExtDir + (sMaxExtDescCnt-1) ) / 
                sMaxExtDescCnt;

        /* loop 가 한번이면 이쪽으로 들어올수 없었음.  */
        IDE_DASSERT( sLoop > 1 ); 

        sDebugTotalExtDescCnt = 0;

        for ( i= (sLoop-1) ; i >= 0 ; i-- )
        { 
            sExtCnt = sMaxExtDescCnt;

            IDE_TEST( sdpscExtDir::getExtDescInfo( aStatistics,
                                                   aSpaceID,
                                                   aAllocExtDirInfo->mNewExtDirPID,
                                                   (UShort)(i * sMaxExtDescCnt), /*aIdx*/
                                                   &sExtDescRID,
                                                   &sExtDesc ) 
                      != IDE_SUCCESS );

            IDE_ERROR( sExtDesc.mExtFstPID != SD_NULL_RID );
            IDE_ERROR( sExtDesc.mLength > 0 );

            /* Ext에 ExtDir가 없으면 */
            if ( aAllocExtDirInfo->mNewExtDirPID != sExtDesc.mExtFstPID )
            {
                /* 가져온 Extent 첫 page에 ExtDir 생성 */
                sNewExtDirPID = sExtDesc.mExtFstPID;
                /* 첫번째 Ext의 FstDataPID 는 두번째 페이지 부터 */
                sExtDesc.mExtFstDataPID = sExtDesc.mExtFstPID + 1;

                IDE_TEST( sdpscExtDir::createAndInitPage(
                                    aStatistics,
                                    aMtx,
                                    aSpaceID,
                                    sNewExtDirPID,                       /* aNewExtDirPID */
                                    aAllocExtDirInfo->mNxtPIDOfNewExtDir,/* aNxtExtDirPID */
                                    sMaxExtDescCnt, 
                                    &sExtDirPagePtr )
                           != IDE_SUCCESS );

                sdrMiniTrans::setNTA( aMtx,
                                      aSpaceID,
                                      SDR_OP_NULL,
                                      &sNTA,
                                      NULL,
                                      0 /* Data Count */);

                IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Write(
                                                         aStatistics,
                                                         aMtx,
                                                         aSpaceID,
                                                         sNewExtDirPID,
                                                         &sNewExtDirCntlHdr )
                          != IDE_SUCCESS );

                IDE_TEST( sdpscExtDir::logAndAddExtDesc( aMtx,
                                                         sNewExtDirCntlHdr,
                                                         &sExtDesc )
                          != IDE_SUCCESS );
                sDebugTotalExtDescCnt++;

                sDebugLoopCnt = 0;
                while ( --sExtCnt > 0 )
                {
                    getNxtExt( sAllocExtDirCntlHdr, sExtDescRID, &sNxtExtRID, &sExtDesc ); 

                    if ( sNxtExtRID == SD_NULL_PID )
                    {
                        break;
                    }

                    IDE_ERROR( sExtDesc.mExtFstPID != SD_NULL_RID );
                    IDE_ERROR( sExtDesc.mLength > 0 );

                    sExtDescRID = sNxtExtRID;

                    IDE_TEST( sdpscExtDir::logAndAddExtDesc( aMtx,
                                                             sNewExtDirCntlHdr,
                                                             &sExtDesc )
                              != IDE_SUCCESS );
                    sDebugTotalExtDescCnt++;
                    /* xxSEG_EXTDESC_COUNT_PER_EXTDIR의 최대값은 128
                        그이상 loop 를 돌고 있다는 것은 에러 상황임 */
                    sDebugLoopCnt++;
                    IDE_ERROR( sDebugLoopCnt < 128 )
                }

                IDE_TEST( sdptbExtent::freeExtDir( aStatistics,
                                                   aMtx,
                                                   aSpaceID,
                                                   sFreeListIdx,
                                                   sNewExtDirPID )
                          != IDE_SUCCESS );
            }
            else
            {
                /* ExtDir이 있으므로 CntlHdr 값만 갱신함 */
                sAllocExtDirCntlHdr->mExtCnt = sMaxExtDescCnt;          
                sAllocExtDirCntlHdr->mMaxExtCnt = sMaxExtDescCnt;              

                IDE_TEST( sdrMiniTrans::writeNBytes( 
                                         aMtx,
                                         (UChar*)&(sAllocExtDirCntlHdr->mMaxExtCnt),
                                         &sAllocExtDirCntlHdr->mExtCnt,
                                         ID_SIZEOF( sAllocExtDirCntlHdr->mExtCnt ) )
                          != IDE_SUCCESS );

                sDebugTotalExtDescCnt += sAllocExtDirCntlHdr->mExtCnt ;
            }
        }

        IDE_ERROR( sDebugTotalExtDescCnt == aAllocExtDirInfo->mExtCntInShrinkExtDir );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션 가용공간 할당시 사용한 ExtDir 페이지에
 *               트랜잭션 첫번째 DskViewSCN 설정
 *
 * 자신이 사용하고 하는 ExtDir를 표시하여 덮어쓰기하지 않도록 하기 위함이다.
 * No-Logging 으로 설정하고 페이지를 dirty시킨다.
 *
 * aStatistics       - [IN] 통계정보
 * aSpaceID          - [IN] 테이블스페이스 ID
 * aExtRID           - [IN] 트랜잭션이 사용한 Extent Desc. RID
 * aMyFstDskViewSCN  - [IN] Extent Dir. 페이지에 설정할 FstDskViewSCN
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::setSCNAtAlloc( idvSQL        * aStatistics,
                                   scSpaceID       aSpaceID,
                                   sdRID           aExtRID,
                                   smSCN         * aMyFstDskViewSCN )
{
    sdpscExtDirCntlHdr  * sCntlHdr;
    smSCN                 sInfiniteSCN;

    IDE_ASSERT( aExtRID          != SD_NULL_RID );
    IDE_ASSERT( aMyFstDskViewSCN != NULL );

    IDE_TEST( fixAndGetCntlHdr4Write( aStatistics,
                                      NULL, /* aMtx */
                                      aSpaceID,
                                      SD_MAKE_PID( aExtRID ),
                                      &sCntlHdr )
              != IDE_SUCCESS );

    setFstDskViewSCN( sCntlHdr, aMyFstDskViewSCN );
    /* BUG-30567 Users need the function that check the amount of 
     * usable undo tablespace. 
     * UndoRecord기록시, LatestCommitSCN을 무한대로 박아줍니다.
     * 현재 진행중인 Transaction의 UndoPage들도 사용하지 못한다고
     * 인식하기 위함입니다.
     * 이후 Commit/Rollback시 모두 CommitSCN으로 다시 박히면서
     * 이 값은 복구됩니다.*/
    SM_SET_SCN_INFINITE( &sInfiniteSCN );
    setLatestCSCN( sCntlHdr, &sInfiniteSCN );

    sdbBufferMgr::setDirtyPageToBCB(
                        aStatistics,
                        sdpPhyPage::getPageStartPtr((UChar*)sCntlHdr) );

    IDE_TEST( releaseCntlHdr( aStatistics, sCntlHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : aCurExtRID가 가리키는 Extent에 aPrvAllocPageID이후
 *               Page가 존재하는 하는지 체크해서 없으면 새로운 다음
 *               Extent로 이동하고 다음 Extent에서 Free Page를 찾아서
 *               Page가 할당된 ExtRID와 PageID를 넘겨준다.
 *
 *               다음 Extent가 없으면 aNxtExtRID와 aFstDataPIDOfNxtExt
 *               에 SD_NULL_RID, SD_NULL_PID를 넘긴다.
 *
 * Caution:
 *
 *  1. 이 함수가 호출될때 SegHdr가 있는 페이지에 XLatch가
 *     걸려 있어야 한다.
 *
 * aStatistics         - [IN] 통계 정보
 * aSpaceID            - [IN] TableSpace ID
 * aSegHdr             - [IN] Segment Header
 * aCurExtRID          - [IN] 현재 Extent RID
 *
 * aNxtExtRID          - [OUT] 다음 Extent RID
 * aFstPIDOfExt        - [OUT] 다음 Extent의 첫번째 PageID
 * aFstDataPIDOfNxtExt - [OUT] 다음 Extent 의 첫번째 Data Page ID,
 *                             Extent의 첫번째 페이지가 Extent Dir
 *                             Page로 사용되기도 한다.
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::getNxtExt4Alloc( idvSQL       * aStatistics,
                                     scSpaceID      aSpaceID,
                                     sdRID          aCurExtRID,
                                     sdRID        * aNxtExtRID,
                                     scPageID     * aFstPIDOfExt,
                                     scPageID     * aFstDataPIDOfNxtExt )
{
    sdpscExtDesc          sExtDesc;
    scPageID              sExtDirPID;
    sdRID                 sNxtExtRID = SD_NULL_RID;
    sdpscExtDirCntlHdr  * sExtDirCntlHdr;
    SInt                  sState = 0;

    IDE_ASSERT( aSpaceID            != 0 );
    IDE_ASSERT( aCurExtRID          != SD_NULL_RID );
    IDE_ASSERT( aNxtExtRID          != NULL );
    IDE_ASSERT( aFstPIDOfExt        != NULL );
    IDE_ASSERT( aFstDataPIDOfNxtExt != NULL );

    sExtDirPID = SD_MAKE_PID( aCurExtRID );

    IDE_TEST( fixAndGetCntlHdr4Read( aStatistics,
                                     NULL,
                                     aSpaceID,
                                     sExtDirPID,
                                     &sExtDirCntlHdr )
              != IDE_SUCCESS );
    sState = 1;

    /* aCurExtRID 다음 Extent가 같은 ExtDirPage내에 존재하는지 검사 */
    getNxtExt( sExtDirCntlHdr, aCurExtRID, &sNxtExtRID, &sExtDesc );

    sState = 0;
    IDE_TEST( releaseCntlHdr( aStatistics, sExtDirCntlHdr )
              != IDE_SUCCESS );

    if ( sNxtExtRID != SD_NULL_RID )
    {
        *aNxtExtRID          = sNxtExtRID;
        *aFstPIDOfExt        = sExtDesc.mExtFstPID;
        *aFstDataPIDOfNxtExt = sExtDesc.mExtFstDataPID;
    }
    else
    {
        *aNxtExtRID          = SD_NULL_RID;
        *aFstDataPIDOfNxtExt = SD_NULL_PID;
        *aFstPIDOfExt        = SD_NULL_PID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if ( sState != 0 )
    {
        IDE_ASSERT( releaseCntlHdr( aStatistics, sExtDirCntlHdr )
                    == IDE_SUCCESS );
    }
    IDE_POP();

    *aNxtExtRID          = SD_NULL_RID;
    *aFstPIDOfExt        = SD_NULL_PID;
    *aFstDataPIDOfNxtExt = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : aExtDirPID가 가리키는 페이지에서 OUT인자의 정보를 가져
 *               온다. aExtDirPID가 가리키는 페이지는 Extent Directory
 *               Page이어야 한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] TableSpace ID
 * aExtDirPID  - [IN] Extent Directory PageID
 * aIdx        - [IN] 구하려는 ExtDescInfo의 인덱스 0:첫번째 ExtDesc
 * aExtRID     - [OUT] aIdx번째 Extent RID
 * aExtDescPtr - [OUT] aIdx번째 Extent Desc가 복사될 버퍼
 *
 **********************************************************************/
IDE_RC sdpscExtDir::getExtDescInfo( idvSQL        * aStatistics,
                                    scSpaceID       aSpaceID,
                                    scPageID        aExtDirPID,
                                    UShort          aIdx,
                                    sdRID         * aExtRID,
                                    sdpscExtDesc  * aExtDescPtr )
{
    SInt                 sState = 0;
    sdpscExtDesc       * sExtDescPtr;
    sdpscExtDirCntlHdr * sExtDirCntlHdr = NULL;

    IDE_ASSERT( aSpaceID    != 0 );
    IDE_ASSERT( aExtDirPID  != SD_NULL_PID );
    IDE_ASSERT( aExtRID     != NULL );
    IDE_ASSERT( aExtDescPtr != NULL );

    initExtDesc( aExtDescPtr );

    IDE_TEST( fixAndGetCntlHdr4Read( aStatistics,
                                     NULL,  /* aMtx */
                                     aSpaceID,
                                     aExtDirPID,
                                     &sExtDirCntlHdr )
              != IDE_SUCCESS );
    sState = 1;

    if ( sExtDirCntlHdr->mExtCnt != 0 )
    {
        *aExtRID = SD_MAKE_RID( aExtDirPID,
                                calcDescIdx2Offset( sExtDirCntlHdr, aIdx ) );

        sExtDescPtr  = getExtDescByIdx( sExtDirCntlHdr, aIdx );
        *aExtDescPtr = *sExtDescPtr;
    }
    else
    {
        *aExtRID = SD_NULL_RID;
    }

    sState = 0;
    IDE_TEST( releaseCntlHdr( aStatistics, sExtDirCntlHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( releaseCntlHdr( aStatistics, sExtDirCntlHdr )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Next ExtDir 페이지가 Expire가 되었는지 검사한 후 반환
 *
 * 세그먼트 내에서의 다음 ExtDir가 Expired되었는지 확인한다.
 *
 * aStatistics      - [IN]  통계정보
 * aStartInfo       - [IN]  Mtx 시작 정보
 * aSpaceID         - [IN]  테이블스페이스 ID
 * aSegHdrPID       - [IN]  세그먼트 헤더 페이지 ID
 * aNxtExtDirPID    - [IN]  Steal 가능한지 검사할 ExtDir PID
 * aAllocExtDirInfo - [OUT] Steal 가능한 ExtDir 페이지에 할당에 대한 정보
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::checkNxtExtDir4Steal( idvSQL               * aStatistics,
                                          sdrMtxStartInfo      * aStartInfo,
                                          scSpaceID              aSpaceID,
                                          scPageID               aSegHdrPID,
                                          scPageID               aNxtExtDirPID,
                                          sdpscAllocExtDirInfo * aAllocExtDirInfo )
{
    scPageID         sNewNxtExtDirPID;
    scPageID         sNewCurExtDirPID;
    smSCN            sMyFstDskViewSCN;
    smSCN            sSysMinDskViewSCN;
    sdpscExtDirState sExtDirState;
    UInt             sExtCntInExtDir;


    IDE_ASSERT( aStartInfo       != NULL );
    IDE_ASSERT( aSegHdrPID       != SD_NULL_PID );
    IDE_ASSERT( aNxtExtDirPID    != SD_NULL_PID );
    IDE_ASSERT( aAllocExtDirInfo != NULL );

    initAllocExtDirInfo( aAllocExtDirInfo );

    sNewCurExtDirPID = SD_NULL_PID;
    sNewNxtExtDirPID = SD_NULL_PID;
    sExtCntInExtDir  = 0;

    sMyFstDskViewSCN = smxTrans::getFstDskViewSCN( aStartInfo->mTrans );
    SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

    /* Next ExtDir이 SegHdr일 경우 스틸할수 없다 */
    IDE_TEST_CONT( aSegHdrPID == aNxtExtDirPID, CONT_CANT_STEAL ); 

    IDE_TEST( checkExtDirState4Reuse( aStatistics,
                                      aSpaceID,
                                      aSegHdrPID,
                                      aNxtExtDirPID,
                                      &sMyFstDskViewSCN,
                                      &sSysMinDskViewSCN,
                                      &sExtDirState,
                                      NULL, /* aAllocExtRID */
                                      NULL, /* aExtDesc */
                                      &sNewNxtExtDirPID,
                                      &sExtCntInExtDir ) 
              != IDE_SUCCESS );

    if ( sExtDirState == SDPSC_EXTDIR_EXPIRED )
    {
        sNewCurExtDirPID = aNxtExtDirPID;
    }
    else
    {
        // Next ExtDir를 재사용할 수 없거나 prepareNewPage4Append로 확보된
        // ExtDir는 Steal할 수 없다.
    }

    IDE_EXCEPTION_CONT( CONT_CANT_STEAL ); 

    aAllocExtDirInfo->mNewExtDirPID         = sNewCurExtDirPID;
    aAllocExtDirInfo->mNxtPIDOfNewExtDir    = sNewNxtExtDirPID;
    aAllocExtDirInfo->mExtCntInShrinkExtDir = sExtCntInExtDir;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : ExtDir Control Header에 Next ExtDir 페이지의 PID 설정
 *
 * aMtx           - [IN] Mtx 포인터
 * aExtDirCntlHdr - [IN] ExtDir Control Header 포인터
 * aNxtExtDirPID  - [IN] Next ExtDir 페이지의 PID
 *                       SD_NULL_PID인 경우는 마지막 ExtDir 페이지인 경우이다.
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::setNxtExtDir( sdrMtx             * aMtx,
                                  sdpscExtDirCntlHdr * aExtDirCntlHdr,
                                  scPageID             aNxtExtDirPID )
{
    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aExtDirCntlHdr != NULL );

    return sdrMiniTrans::writeNBytes( aMtx,
                                      (UChar*)&aExtDirCntlHdr->mNxtExtDir,
                                      &aNxtExtDirPID,
                                      ID_SIZEOF( aNxtExtDirPID ) );
}


/***********************************************************************
 *
 * Description : 현재 ExtDir 페이지의 다양한 정보를 수집한다.
 *
 * aStatistics    - [IN] 통계정보
 * aStartInfo     - [IN] Mtx 시작 정보
 * aSpaceID       - [IN] 테이블스페이스 ID
 * aSegHandle     - [IN] 세그먼트 핸들
 * aCurExtDir     - [IN] 자료를 수집할 Extent Dir. 페이지의 PID
 * aCurExtDirInfo - [OUT] 현재 Extent Dir. 페이지의 정보
 *
 ***********************************************************************/
IDE_RC sdpscExtDir::getCurExtDirInfo( idvSQL           * aStatistics,
                                      scSpaceID          aSpaceID,
                                      sdpSegHandle     * aSegHandle,
                                      scPageID           aCurExtDir,
                                      sdpscExtDirInfo  * aCurExtDirInfo )
{
    scPageID             sSegPID;
    sdpscSegMetaHdr    * sSegHdr;
    sdpscExtDirCntlHdr * sExtDirCntlHdr;
    UShort               sExtCntInExtDir;
    UInt                 sFreeDescCntOfExtDir;
    UInt                 sState = 0;

    IDE_ASSERT( aSegHandle     != NULL );
    IDE_ASSERT( aCurExtDir     != SD_NULL_PID );
    IDE_ASSERT( aCurExtDirInfo != NULL );

    sSegPID = aSegHandle->mSegPID;
    initExtDirInfo( aCurExtDirInfo );

    aCurExtDirInfo->mExtDirPID = aCurExtDir;

    if ( aCurExtDir != sSegPID )
    {
        IDE_TEST( fixAndGetCntlHdr4Read( aStatistics,
                                         NULL,     /* aMtx */
                                         aSpaceID,
                                         aCurExtDir,
                                         &sExtDirCntlHdr )
                  != IDE_SUCCESS );
        sState = 1;
    }
    else
    {
        // Segment Header가 존재하는 경우
        IDE_TEST( sdpscSegHdr::fixAndGetHdr4Read( 
                                           aStatistics,
                                           NULL, /* aMtx */
                                           aSpaceID,
                                           sSegPID,
                                           &sSegHdr )
                  != IDE_SUCCESS );
        sState = 2;

        IDE_ASSERT( aCurExtDirInfo->mExtDirPID == sSegPID );

        sExtDirCntlHdr = sdpscSegHdr::getExtDirCntlHdr( sSegHdr );
    }

    aCurExtDirInfo->mNxtExtDirPID = sExtDirCntlHdr->mNxtExtDir;
    aCurExtDirInfo->mMaxExtCnt    = sExtDirCntlHdr->mMaxExtCnt;

    // 현재 ExtDir 페이지의 freeSlot 개수를 구한다.
    sFreeDescCntOfExtDir = getFreeDescCnt( sExtDirCntlHdr );
    sExtCntInExtDir      = sExtDirCntlHdr->mExtCnt;

    if ( sState == 1 )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             (UChar*)sExtDirCntlHdr )
                  != IDE_SUCCESS );
    }

    if ( sState == 2 )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             (UChar*)sSegHdr )
                != IDE_SUCCESS );
    }

    aCurExtDirInfo->mTotExtCnt = sExtCntInExtDir;

    if ( sFreeDescCntOfExtDir == 0 )
    {
        // ExtDir Extent Map을 생성해서 기록해야하는 경우
        aCurExtDirInfo->mIsFull    = ID_TRUE;
    }
    else
    {
        // ExtDir을 이전 ExtDir 페이지에 기록할 수 있는 경우
        aCurExtDirInfo->mIsFull    = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch ( sState )
    {
        case 2 :
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   (UChar*)sSegHdr )
                        == IDE_SUCCESS );
            break;
        case 1:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   (UChar*)sExtDirCntlHdr )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}
