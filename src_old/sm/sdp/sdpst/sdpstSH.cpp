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
 * $Id: sdpstSH.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Segment Header 관련
 * STATIC 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <smErrorCode.h>
# include <sdr.h>

# include <sdpReq.h>
# include <sdpPhyPage.h>

# include <sdpstDef.h>
# include <sdpstSH.h>

# include <sdpstAllocPage.h>
# include <sdpstExtDir.h>
# include <sdpstBMP.h>
# include <sdpstLfBMP.h>
# include <sdpstItBMP.h>
# include <sdpstRtBMP.h>
# include <sdpstStackMgr.h>
# include <sdpstCache.h>
# include <ideErrorMgr.h>

/***********************************************************************
 * Description : Water Mark 설정
 *
 * aWM                - [IN] Water Mark를 설정할 변수
 * aWMPID             - [IN] Water Mark PID
 * aExtDirPID         - [IN] Water Mark Page가 포함된 ExtDir PID
 * aExtSlotNoInExtDir - [IN] Water Mark Page를 포함한 ExtDesc의 SlotNo
 * aPosStack          - [IN] Water Mark Stack
 ***********************************************************************/
void sdpstSH::updateWM( sdpstWM        * aWM,
                        scPageID         aWMPID,
                        scPageID         aExtDirPID,
                        SShort           aExtSlotNoInExtDir,
                        sdpstStack     * aPosStack )
{
    IDE_ASSERT( aWM != NULL );

    if ( sdpstStackMgr::getDepth( aPosStack ) != SDPST_LFBMP )
    {
        sdpstStackMgr::dump( aPosStack );
        IDE_ASSERT( 0 );
    }

    aWM->mWMPID           = aWMPID;
    aWM->mExtDirPID       = aExtDirPID;
    aWM->mSlotNoInExtDir  = aExtSlotNoInExtDir;
    idlOS::memcpy( &(aWM->mStack), aPosStack, ID_SIZEOF(sdpstStack) );
}

/***********************************************************************
 * Description : Segment Header를 초기화 한다.
 *
 * aSegHdr              - [IN] Segment Header
 * aSegHdrPID           - [IN] Segment Header PID
 * aSegType             - [IN] Segment Type
 * aPageCntInExt        - [IN] Extent 당 페이지 개수
 * aMaxSlotCntInExtDir  - [IN] Segment Header에 포함된 ExtDir의 최대 Slot 개수
 * aMaxSlotCntInRtBMP   - [IN] Segment Header에 포함된 RtBMP의 최대 Slot 개수
 ***********************************************************************/
void sdpstSH::initSegHdr( sdpstSegHdr       * aSegHdr,
                          scPageID            aSegHdrPID,
                          sdpSegType          aSegType,
                          UInt                aPageCntInExt,
                          UShort              aMaxSlotCntInExtDir,
                          UShort              aMaxSlotCntInRtBMP )
{
    scOffset   sBodyOffset;
    idBool     sDummy;

    IDE_ASSERT( aSegHdr != NULL );

    /* Segment Control Header 초기화 */
    aSegHdr->mSegType           = aSegType;
    aSegHdr->mSegState          = SDP_SEG_USE;
    aSegHdr->mLstLfBMP          = SD_NULL_PID;
    aSegHdr->mLstItBMP          = SD_NULL_PID;
    aSegHdr->mLstRtBMP          = aSegHdrPID;
    aSegHdr->mSegHdrPID         = aSegHdrPID;

    aSegHdr->mTotPageCnt        = 0;
    aSegHdr->mFreeIndexPageCnt  = 0;
    aSegHdr->mTotExtCnt         = 0;
    aSegHdr->mPageCntInExt      = aPageCntInExt;

    initWM( &aSegHdr->mHWM );

    // rt-bmp control header를 초기화하였으므로 sdpPhyPageHdr의
    // freeOffset과 total free size를 변경한다.
    sdpPhyPage::initLogicalHdr( sdpPhyPage::getHdr((UChar*)aSegHdr),
                                ID_SIZEOF(sdpstSegHdr) );

    sBodyOffset = sdpPhyPage::getDataStartOffset( ID_SIZEOF(sdpstSegHdr) );

    /* ExtDir Control Header 초기화 */
    sdpstExtDir::initExtDirHdr( &aSegHdr->mExtDirHdr,
                                aMaxSlotCntInExtDir,
                                sBodyOffset );

    /* Segment Header에 최대한 기록할 수 있는 ExtentDir Slot의
     * 개수를 고려하여 rt-bmp 페이지 map을 고려한다. */
    sBodyOffset += aMaxSlotCntInExtDir * ID_SIZEOF(sdpstExtDesc);

    /* Root Bitmap Control Header 초기화 */
    sdpstBMP::initBMPHdr( &aSegHdr->mRtBMPHdr,
                          SDPST_RTBMP,
                          sBodyOffset,
                          SD_NULL_PID,
                          0,
                          SD_NULL_PID,
                          SD_NULL_PID,
                          0,
                          aMaxSlotCntInRtBMP,
                          &sDummy );
}

/***********************************************************************
 * Description : HWM를 초기화한다.
 *
 * aWM      - [IN] 초기화할 WM
 ***********************************************************************/
void sdpstSH::initWM( sdpstWM  * aWM )
{
    aWM->mWMPID          = SD_NULL_PID;
    aWM->mExtDirPID      = SD_NULL_PID;
    aWM->mSlotNoInExtDir = SDPST_INVALID_SLOTNO;

    sdpstStackMgr::initialize( &aWM->mStack );
}

/***********************************************************************
 * Description : Segment Header 페이지 생성 및 초기화
 *
 * 첫번째 Extent의 bmp 페이지를 모두 생성한 후 Segment Header
 * 페이지를 생성한다.
 *
 * aStatistics   - [IN] 통계정보
 * aStartInfo    - [IN] Mtx의 StartInfo
 * aSpaceID      - [IN] Tablespace의 ID
 * aExtDesc      - [IN] Extent Desc Pointer
 * aBfrInfo      - [IN] Extent 이전 할당 정보
 * aAftInfo      - [IN] Extent 이후 할당 정보
 * aSegCache     - [IN] Segment Cache
 * aSegPID       - [OUT] Segment 헤더 페이지의 PID
 ***********************************************************************/
IDE_RC sdpstSH::createAndInitPage( idvSQL               * aStatistics,
                                   sdrMtxStartInfo      * aStartInfo,
                                   scSpaceID              aSpaceID,
                                   sdpstExtDesc         * aExtDesc,
                                   sdpstBfrAllocExtInfo * aBfrInfo,
                                   sdpstAftAllocExtInfo * aAftInfo,
                                   sdpstSegCache        * aSegCache,
                                   scPageID             * aSegPID )
{
    sdrMtx              sMtx;
    UInt                sState = 0 ;
    UChar             * sPagePtr;
    UShort              sMetaPageCnt;
    sdpstPBS            sPBS;
    scPageID            sSegHdrPID;
    sdpstSegHdr       * sSegHdrPtr;
    ULong               sSeqNo;
    sdpstRedoInitSegHdr sLogData;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aSegCache  != NULL );
    IDE_ASSERT( aExtDesc   != NULL );
    IDE_ASSERT( aAftInfo   != NULL );
    IDE_ASSERT( aSegPID    != NULL );

    /* 첫번째 Extent의 필요한 bmp 페이지들을 모두 생성하였으므로,
     * Segment Header 페이지의 PID를 계산한다. */
    sMetaPageCnt = (aAftInfo->mPageCnt[SDPST_RTBMP] +
                    aAftInfo->mPageCnt[SDPST_ITBMP] +
                    aAftInfo->mPageCnt[SDPST_LFBMP] +
                    aAftInfo->mPageCnt[SDPST_EXTDIR]);

    /* SegHdr 페이지에 설정할 SeqNo 와 SegPID를 계산한다. */
    sSeqNo     = aBfrInfo->mNxtSeqNo + sMetaPageCnt;
    sSegHdrPID = aExtDesc->mExtFstPID + (UInt)sMetaPageCnt;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    /*
     * 첫번째 Extent에는 Segment Header를 할당한다.
     * 생성된 Segment Header 페이지의 PID를 반환한다.
     */
    sPBS = ( SDPST_BITSET_PAGETP_META | SDPST_BITSET_PAGEFN_FUL );

    /* logical header는 아래에서 별도로 초기화한다. */
    IDE_TEST( sdpstAllocPage::createPage(
                                    aStatistics,
                                    &sMtx,
                                    aSpaceID,
                                    NULL, /* aSegHandle4DataPage */
                                    sSegHdrPID,
                                    sSeqNo,
                                    SDP_PAGE_TMS_SEGHDR,
                                    aExtDesc->mExtFstPID,
                                    SDPST_INVALID_PBSNO,
                                    sPBS,
                                    &sPagePtr ) != IDE_SUCCESS );

    sSegHdrPtr = getHdrPtr(sPagePtr);

    /* Segment Header 페이지 초기화 */
    initSegHdr( sSegHdrPtr,
                sSegHdrPID,
                aSegCache->mSegType,
                aExtDesc->mLength,
                aBfrInfo->mMaxSlotCnt[SDPST_EXTDIR],
                aBfrInfo->mMaxSlotCnt[SDPST_RTBMP] );

    /* INIT_SEGMENT_META_HEADER 로깅 */
    sLogData.mSegType               = aSegCache->mSegType;
    sLogData.mSegPID                = sSegHdrPID;
    sLogData.mPageCntInExt          = aExtDesc->mLength;
    sLogData.mMaxExtDescCntInExtDir = aBfrInfo->mMaxSlotCnt[SDPST_EXTDIR];
    sLogData.mMaxSlotCntInRtBMP     = aBfrInfo->mMaxSlotCnt[SDPST_RTBMP];

    IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx,
                                         (UChar*)sSegHdrPtr,
                                         &sLogData,
                                         ID_SIZEOF( sLogData ),
                                         SDR_SDPST_INIT_SEGHDR )
              != IDE_SUCCESS );

    IDE_TEST( sdpDblPIDList::initBaseNode( &sSegHdrPtr->mExtDirBase,
                                           &sMtx ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    *aSegPID = sSegHdrPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment Header에 WM를 갱신한다.
 *
 * aMtx             - [IN] Mini Transaction Pointer
 * aWM              - [IN] 갱신할 WM 변수
 * aWMPID           - [IN] Water Mark PID
 * aExtDirPID       - [IN] Water Mark Page가 포함된 ExtDir PID
 * aSlotNoInExtDir  - [IN] Water Mark Page를 포함한 ExtDesc의 SlotNo
 * aStack           - [IN] Water Mark Stack
 ***********************************************************************/
IDE_RC sdpstSH::logAndUpdateWM( sdrMtx         * aMtx,
                                sdpstWM        * aWM,
                                scPageID         aWMPID,
                                scPageID         aExtDirPID,
                                SShort           aSlotNoInExtDir,
                                sdpstStack     * aStack )
{
    sdpstRedoUpdateWM   sLogData;

    IDE_DASSERT( aWM             != NULL );
    IDE_DASSERT( aWMPID          != SD_NULL_PID );
    IDE_DASSERT( aExtDirPID      != SD_NULL_PID );
    IDE_DASSERT( aSlotNoInExtDir != SDPST_INVALID_SLOTNO );
    IDE_DASSERT( aStack          != NULL );
    IDE_DASSERT( aMtx            != NULL );

    // HWM 설정하기
    updateWM( aWM, aWMPID, aExtDirPID, aSlotNoInExtDir, aStack );

    // UPDATE WM logging
    sLogData.mWMPID          = aWMPID;
    sLogData.mExtDirPID      = aExtDirPID;
    sLogData.mSlotNoInExtDir = aSlotNoInExtDir;
    sLogData.mStack          = *aStack;

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)aWM,
                  &sLogData,
                  ID_SIZEOF( sLogData ),
                  SDR_SDPST_UPDATE_WM ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment Header 페이지에 새로운 ExtDir Page를 연결
 *
 * aMtx              - [IN] Mini Transaction Pointer
 * aSegHdrPagePtr    - [IN] Segment Header Page Pointer
 * aNewExtDirPagePtr - [IN] ExtDir Page Pointer
 ***********************************************************************/
IDE_RC  sdpstSH::addNewExtDirPage( sdrMtx    * aMtx,
                                   UChar     * aSegHdrPagePtr,
                                   UChar     * aNewExtDirPagePtr )
{
    sdpstSegHdr         * sSegHdr;
    sdpPhyPageHdr       * sPhyPageHdr;
    sdpDblPIDListNode   * sListNode;

    IDE_ASSERT( aMtx              != NULL );
    IDE_ASSERT( aSegHdrPagePtr    != NULL );
    IDE_ASSERT( aNewExtDirPagePtr != SD_NULL_PID );

    sSegHdr     = getHdrPtr( aSegHdrPagePtr );
    sPhyPageHdr = sdpPhyPage::getHdr( aNewExtDirPagePtr );
    sListNode   = sdpPhyPage::getDblPIDListNode( sPhyPageHdr );

    IDE_TEST( sdpDblPIDList::insertTailNode( NULL /*aStatistics*/,
                                             &sSegHdr->mExtDirBase,
                                             sListNode,
                                             aMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Extent 할당후 새로운 생성한 Bitmap 페이지 정보를
 *               Segment Header에 반영한다.
 *
 * A. Last Bitmap 페이지들 정보 설정한다.
 * B. Segment Size를 설정한다.
 *
 * aStartInfo     - [IN] Mtx의 StartInfo
 * aSegHdrPagePtr - [IN] Segment Header Page Pointer
 * aAllocPageCnt  - [IN] 할당된 페이지 개수
 * aNewRtBMPCnt   - [IN] 생성된 RtBMP개수
 * aLstLfBMP      - [IN] 변경할 마지막 LfBMP
 * aLstItBMP      - [IN] 변경할 마지막 ItBMP
 * aLstRtBMP      - [IN] 변경할 마지막 RtBMP
 * aBfrLstRtBMP   - [OUT] 변경전 마지막 RtBMP
 ***********************************************************************/
IDE_RC sdpstSH::logAndLinkBMPs( sdrMtx       * aMtx,
                                sdpstSegHdr  * aSegHdr,
                                ULong          aAllocPageCnt,
                                ULong          aMetaPageCnt,
                                UShort         aNewRtBMPCnt,
                                scPageID       aLstRtBMP,
                                scPageID       aLstItBMP,
                                scPageID       aLstLfBMP,
                                scPageID     * aBfrLstRtBMP )
{
    sdpstRedoAddExtToSegHdr   sLogData;

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSegHdr      != NULL );
    IDE_ASSERT( aLstRtBMP    != SD_NULL_PID );
    IDE_ASSERT( aLstItBMP    != SD_NULL_PID );
    IDE_ASSERT( aLstLfBMP    != SD_NULL_PID );
    IDE_ASSERT( aBfrLstRtBMP != NULL );

    /* linkBMPsToSegHdr()에서 변경되기 때문에 임시 저장 */
    *aBfrLstRtBMP = aSegHdr->mLstRtBMP;

    linkBMPsToSegHdr( aSegHdr,
                      aAllocPageCnt,
                      aMetaPageCnt,
                      aLstLfBMP,
                      aLstItBMP,
                      aLstRtBMP,
                      aNewRtBMPCnt );

    sLogData.mAllocPageCnt = aAllocPageCnt;
    sLogData.mMetaPageCnt = aMetaPageCnt;
    sLogData.mNewLstLfBMP = aLstLfBMP;
    sLogData.mNewLstItBMP = aLstItBMP;
    sLogData.mNewLstRtBMP = aLstRtBMP;
    sLogData.mNewRtBMPCnt = aNewRtBMPCnt;

    // SDR_SDPST_ADD_EXT_TO_SEGHDR 및 Total Page Count logging
    IDE_TEST( sdrMiniTrans::writeLogRec(
                            aMtx,
                            (UChar*)aSegHdr,
                            &sLogData,
                            ID_SIZEOF( sLogData ),
                            SDR_SDPST_ADD_EXT_TO_SEGHDR) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 할당된 새로운 Extent의 Bitmap 페이지들을 Segment
 *               Header에 연결하고, Segment 크기를 증가시킨다.
 *
 * aSegHdr          - [IN] Segment Header
 * aAllocPageCnt    - [IN] 할당한 Page Count
 * aNewLstLfBMP     - [IN] 변경할 마지막 LfBMP
 * aNewLstItBMP     - [IN] 변경할 마지막 ItBMP
 * aNewLstRtBMP     - [IN] 변경할 마지막 RtBMP
 * aNewRtBMPCnt     - [IN] 생성한 RtBMP Count
 **********************************************************************/
void sdpstSH::linkBMPsToSegHdr( sdpstSegHdr  * aSegHdr,
                                ULong          aAllocPageCnt,
                                ULong          aMetaPageCnt,
                                scPageID       aNewLstLfBMP,
                                scPageID       aNewLstItBMP,
                                scPageID       aNewLstRtBMP,
                                UShort         aNewRtBMPCnt )
{
    IDE_DASSERT( aSegHdr      != NULL );
    IDE_DASSERT( aAllocPageCnt > 0 );
    IDE_DASSERT( aNewLstLfBMP != SD_NULL_PID );
    IDE_DASSERT( aNewLstItBMP != SD_NULL_PID );
    IDE_DASSERT( aNewLstRtBMP != SD_NULL_PID );

    /* 할당된 Extent 만큼의 용량을 Segment에 반영한다. */
    aSegHdr->mTotPageCnt  += aAllocPageCnt;
    aSegHdr->mTotExtCnt   += 1;
    aSegHdr->mTotRtBMPCnt += aNewRtBMPCnt;
    aSegHdr->mFreeIndexPageCnt += aAllocPageCnt - aMetaPageCnt;
    aSegHdr->mLstSeqNo     = aSegHdr->mTotPageCnt - 1;

    /* 새로운 마지막 Bitmap 페이지 정보를 설정한다. */
    aSegHdr->mLstLfBMP = aNewLstLfBMP;
    aSegHdr->mLstItBMP = aNewLstItBMP;
    aSegHdr->mLstRtBMP = aNewLstRtBMP;
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
IDE_RC sdpstSH::getSegInfo( idvSQL        * aStatistics,
                            scSpaceID       aSpaceID,
                            scPageID        aSegPID,
                            void          * aTableHeader,
                            sdpSegInfo    * aSegInfo )
{
    UChar            * sPagePtr;
    sdpstSegHdr      * sSegHdr;
    sdpstWM          * sHWM;
    SInt               sState = 0;

    IDE_ASSERT( aSegInfo != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL, /* sdrMtx */
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );
    sState = 1;

    sSegHdr = sdpstSH::getHdrPtr( sPagePtr );

    /* BUG-43084 X$table_info조회 중 비정상종료 하거나
     * X_LOCK이 걸린 disk table이 조회되지 않을 수 있습니다. */
    if ( ( aTableHeader != NULL ) && 
         ( smLayerCallback::isNullSegPID4DiskTable( aTableHeader ) == ID_TRUE ) )
    {
        aSegInfo->mSegHdrPID = SD_NULL_PID;
        IDE_CONT( TABLE_IS_BEING_DROPPED );
    }
    else
    {
        /* nothing to do */
    }

    aSegInfo->mSegHdrPID    = aSegPID;
    aSegInfo->mType         = (sdpSegType)(sSegHdr->mSegType);
    aSegInfo->mState        = (sdpSegState)(sSegHdr->mSegState);

    aSegInfo->mPageCntInExt = sSegHdr->mPageCntInExt;
    aSegInfo->mFmtPageCnt   = sSegHdr->mTotPageCnt;
    aSegInfo->mExtCnt       = sSegHdr->mTotExtCnt;
    aSegInfo->mExtDirCnt    = sSegHdr->mExtDirBase.mNodeCnt + 1; /* 1은 Seghdr */

    if ( aSegInfo->mExtCnt <= 0 )
    {
        (void)dump( sPagePtr );
        IDE_ASSERT( 0 );
    }

    aSegInfo->mFstExtRID    =
        sdpstExtDir::getFstExtRID( &sSegHdr->mExtDirHdr );

    /*
     * BUG-22474     [valgrind]sdbMPRMgr::getMPRCnt에 UMR있습니다.
     */
    aSegInfo->mLstExtRID    = sSegHdr->mLstExtRID;

    /* HWM의 ExtDesc의 RID를 구한다. */
    sHWM = &(sSegHdr->mHWM);
    aSegInfo->mHWMPID      = sHWM->mWMPID;

    if ( sHWM->mExtDirPID != aSegPID )
    {
        aSegInfo->mExtRIDHWM  = SD_MAKE_RID(
            sHWM->mExtDirPID,
            sdpstExtDir::calcSlotNo2Offset( NULL,
                                          sHWM->mSlotNoInExtDir ));
    }
    else
    {
        aSegInfo->mExtRIDHWM = SD_MAKE_RID(
            sHWM->mExtDirPID,
            sdpstExtDir::calcSlotNo2Offset( &(sSegHdr->mExtDirHdr),
                                          sHWM->mSlotNoInExtDir ));
    }

    aSegInfo->mLstAllocExtRID      = aSegInfo->mExtRIDHWM;
    aSegInfo->mFstPIDOfLstAllocExt = aSegInfo->mExtRIDHWM;

    IDE_EXCEPTION_CONT( TABLE_IS_BEING_DROPPED );

    sState = 0;

    IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sSegHdr )
              != IDE_SUCCESS );

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
 * Description : [ INTERFACE ] segment 상태를 반환한다.
 *
 * aStatistics   - [IN] 통계정보
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegPID       - [IN] Segment 헤더 페이지의 PID
 * aSegState     - [IN] Segment 상태
 ***********************************************************************/
IDE_RC sdpstSH::getSegState( idvSQL        *aStatistics,
                             scSpaceID      aSpaceID,
                             scPageID       aSegPID,
                             sdpSegState   *aSegState )
{
    idBool        sDummy;
    UChar       * sPagePtr;
    sdpSegState   sSegState;
    UInt          sState = 0 ;

    IDE_ASSERT( aSegPID != SD_NULL_PID );

    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          &sPagePtr,
                                          &sDummy ) != IDE_SUCCESS );
    sState = 1;

    sSegState = getHdrPtr( sPagePtr )->mSegState;

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
 * Description : Segment Header에 Meta 페이지의 PID를 설정한다.
 *
 * aStatistics   - [IN] 통계정보
 * aMtx          - [IN] Mini Transaction Pointer
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegPID       - [IN] Segment 헤더 페이지의 PID
 * aIndex        - [IN] Meta PID Array 의 Index
 * aMetaPID      - [IN] 설정할 MetaPID
 ************************************************************************/
IDE_RC sdpstSH::setMetaPID( idvSQL        *aStatistics,
                            sdrMtx        *aMtx,
                            scSpaceID      aSpaceID,
                            scPageID       aSegPID,
                            UInt           aIndex,
                            scPageID       aMetaPID )
{
    UChar        * sPagePtr;
    sdpstSegHdr  * sSegHdr;

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

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sSegHdr->mArrMetaPID[ aIndex ],
                  &aMetaPID,
                  ID_SIZEOF( aMetaPID )) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Segment Header의 Meta 페이지의 PID를 반환한다.
 *
 * aStatistics   - [IN] 통계정보
 * aSpaceID      - [IN] Tablespace의 ID
 * aSegPID       - [IN] Segment 헤더 페이지의 PID
 * aIndex        - [IN] Meta PID Array 의 Index
 * aMetaPID      - [OUT] MetaPID
 ************************************************************************/
IDE_RC sdpstSH::getMetaPID( idvSQL        *aStatistics,
                            scSpaceID      aSpaceID,
                            scPageID       aSegPID,
                            UInt           aIndex,
                            scPageID      *aMetaPID )
{
    UChar            * sPagePtr;
    sdpstSegHdr      * sSegHdr;
    scPageID           sMetaPID;


    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL, /* sdrMtx */
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sSegHdr  = sdpstSH::getHdrPtr( sPagePtr );
    sMetaPID = sSegHdr->mArrMetaPID[ aIndex ];

    IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
              != IDE_SUCCESS );

    *aMetaPID = sMetaPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aMetaPID = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment Header를 dump한다.
 ***********************************************************************/
IDE_RC sdpstSH::dumpHdr( UChar    * aPagePtr,
                         SChar    * aOutBuf,
                         UInt       aOutSize )
{
    UChar         * sPagePtr;
    sdpstSegHdr   * sSegHdr;
    UInt            sLoop;

    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aOutBuf  != NULL );
    IDE_ERROR( aOutSize > 0 );

    /* Segment Header dump */
    sPagePtr = sdpPhyPage::getPageStartPtr( aPagePtr );
    sSegHdr  = getHdrPtr( sPagePtr );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "--------------- Segment Header Begin ----------------\n"
                     "Segment Type                         : %"ID_UINT32_FMT"\n"
                     "Segment State                        : %"ID_UINT32_FMT"\n"
                     "Segment Page ID                      : %"ID_UINT32_FMT"\n"
                     "Lst LfBMP Page ID                    : %"ID_UINT32_FMT"\n"
                     "Lst ItBMP Page ID                    : %"ID_UINT32_FMT"\n"
                     "Lst RtBMP Page ID                    : %"ID_UINT32_FMT"\n"
                     "Lst SeqNo                            : %"ID_UINT32_FMT"\n"
                     "Total Page Count                     : %"ID_UINT64_FMT"\n"
                     "Total RtBMP Count                    : %"ID_UINT64_FMT"\n"
                     "Free Index Page Count                : %"ID_UINT64_FMT"\n"
                     "Total Extent Count                   : %"ID_UINT64_FMT"\n"
                     "Page Count In Extent                 : %"ID_UINT32_FMT"\n"
                     "Lst Extent RID                       : %"ID_UINT64_FMT"\n"
                     "HWM.mWMPID                           : %"ID_UINT32_FMT"\n"
                     "HWM.mExtDirPID                       : %"ID_UINT32_FMT"\n"
                     "HWM.mSlotNoInExtDir                  : %"ID_INT32_FMT"\n"
                     "HWM.mStack.mDepth                    : %"ID_INT32_FMT"\n"
                     "HWM.mStack.mPosition[VTBMP].mNodePID : %"ID_UINT32_FMT"\n"
                     "HWM.mStack.mPosition[VTBMP].mIndex   : %"ID_INT32_FMT"\n"
                     "HWM.mStack.mPosition[RTBMP].mNodePID : %"ID_UINT32_FMT"\n"
                     "HWM.mStack.mPosition[RTBMP].mIndex   : %"ID_INT32_FMT"\n"
                     "HWM.mStack.mPosition[ITBMP].mNodePID : %"ID_UINT32_FMT"\n"
                     "HWM.mStack.mPosition[ITBMP].mIndex   : %"ID_INT32_FMT"\n"
                     "HWM.mStack.mPosition[LFBMP].mNodePID : %"ID_UINT32_FMT"\n"
                     "HWM.mStack.mPosition[LFBMP].mIndex   : %"ID_INT32_FMT"\n"
                     "ExtDir PIDList Base.mNodeCnt         : %"ID_UINT32_FMT"\n"
                     "ExtDir PIDList Base.mBase.mNext      : %"ID_UINT32_FMT"\n"
                     "ExtDir PIDList Base.mBase.mPrev      : %"ID_UINT32_FMT"\n",
                     sSegHdr->mSegType,
                     sSegHdr->mSegState,
                     sSegHdr->mSegHdrPID,
                     sSegHdr->mLstLfBMP,
                     sSegHdr->mLstItBMP,
                     sSegHdr->mLstRtBMP,
                     sSegHdr->mLstSeqNo,
                     sSegHdr->mTotPageCnt,
                     sSegHdr->mTotRtBMPCnt,
                     sSegHdr->mFreeIndexPageCnt,
                     sSegHdr->mTotExtCnt,
                     sSegHdr->mPageCntInExt,
                     sSegHdr->mLstExtRID,
                     sSegHdr->mHWM.mWMPID,
                     sSegHdr->mHWM.mExtDirPID,
                     sSegHdr->mHWM.mSlotNoInExtDir,
                     sSegHdr->mHWM.mStack.mDepth,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_VIRTBMP].mNodePID,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_VIRTBMP].mIndex,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_RTBMP].mNodePID,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_RTBMP].mIndex,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_ITBMP].mNodePID,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_ITBMP].mIndex,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_LFBMP].mNodePID,
                     sSegHdr->mHWM.mStack.mPosition[SDPST_LFBMP].mIndex,
                     sSegHdr->mExtDirBase.mNodeCnt,
                     sSegHdr->mExtDirBase.mBase.mNext,
                     sSegHdr->mExtDirBase.mBase.mPrev );

    /* Meta Page ID String을 만든다. */
    for ( sLoop = 0; sLoop < SDP_MAX_SEG_PID_CNT; sLoop++ )
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "MetaPID Array[%02"ID_UINT32_FMT"]"
                             "                     : %"ID_UINT32_FMT"\n",
                             sLoop,
                             sSegHdr->mArrMetaPID[sLoop] );
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "---------------- Segment Header End -----------------\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment Header를 dump한다.
 ***********************************************************************/
IDE_RC sdpstSH::dumpBody( UChar    * aPagePtr,
                          SChar    * aOutBuf,
                          UInt       aOutSize )
{
    UChar         * sPagePtr;
    SChar         * sDumpBuf;

    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aOutBuf  != NULL );
    IDE_ERROR( aOutSize > 0 );

    if( iduMemMgr::calloc(
            IDU_MEM_SM_SDP, 1,
            ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
            (void**)&sDumpBuf ) == IDE_SUCCESS )
    {
        /* Segment Header dump */
        sPagePtr = sdpPhyPage::getPageStartPtr( aPagePtr );

        /* RtBMP In Segment Header dump */
        sdpstBMP::dumpHdr( sPagePtr, sDumpBuf, IDE_DUMP_DEST_LIMIT );
        idlOS::snprintf( aOutBuf,
                         aOutSize,
                         "%s",
                         sDumpBuf );

        sdpstBMP::dumpBody( sPagePtr, sDumpBuf, IDE_DUMP_DEST_LIMIT );
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%s",
                             sDumpBuf );

        /* ExtDir In Segment Header dump */
        sdpstExtDir::dumpHdr( sPagePtr, sDumpBuf, IDE_DUMP_DEST_LIMIT );
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%s",
                             sDumpBuf );

        sdpstExtDir::dumpBody( sPagePtr, sDumpBuf, IDE_DUMP_DEST_LIMIT );
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%s",
                             sDumpBuf );

        (void)iduMemMgr::free( sDumpBuf );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Segment Header를 dump한다.
 ***********************************************************************/
IDE_RC sdpstSH::dump( UChar    * aPagePtr )
{
    UChar         * sPagePtr;
    SChar         * sDumpBuf;

    IDE_ERROR( aPagePtr != NULL );

    /* Segment Header dump */
    sPagePtr = sdpPhyPage::getPageStartPtr( aPagePtr );

    ideLog::log( IDE_SERVER_0,
                 "==========================================================" );

    /* Physical Page */
    (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                 sPagePtr,
                                 "Physical Page:" );

    if( iduMemMgr::calloc(
            IDU_MEM_SM_SDP, 1,
            ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
            (void**)&sDumpBuf ) == IDE_SUCCESS )
    {
        /* Header Dump */
        if( dumpHdr( sPagePtr,
                     sDumpBuf,
                     IDE_DUMP_DEST_LIMIT ) == IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "%s", sDumpBuf );
        }

        /* Body(RtBMP, ExtDir) Dump */
        if( dumpBody( sPagePtr,
                      sDumpBuf,
                      IDE_DUMP_DEST_LIMIT ) == IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "%s", sDumpBuf );
        }

        (void)iduMemMgr::free( sDumpBuf );
    }

    ideLog::log( IDE_SERVER_0,
                 "==========================================================" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
