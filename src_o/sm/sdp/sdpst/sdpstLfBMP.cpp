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
 * $Id: sdpstLfBMP.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Leaf Bitmap 페이지 관련
 * STATIC 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <sdpReq.h>

# include <sdpstBMP.h>
# include <sdpstItBMP.h>
# include <sdpstAllocPage.h>

# include <sdpstLfBMP.h>
# include <sdpstStackMgr.h>
# include <sdpstFindPage.h>
# include <sdpstDPath.h>
# include <sdpstCache.h>

sdpstBMPOps sdpstLfBMP::mLfBMPOps =
{
    (sdpstGetBMPHdrFunc)sdpstLfBMP::getBMPHdrPtr,
    (sdpstGetSlotOrPageCntFunc)sdpstLfBMP::getTotPageCnt,
    (sdpstGetDistInDepthFunc)sdpstStackMgr::getDistInLfDepth,
    (sdpstIsCandidateChildFunc)sdpstLfBMP::isCandidatePage,
    (sdpstGetStartSlotOrPBSNoFunc)sdpstLfBMP::getStartPBSNo,
    (sdpstGetChildCountFunc)sdpstLfBMP::getTotPageCnt,
    (sdpstSetCandidateArrayFunc)sdpstLfBMP::setCandidateArray,
    (sdpstGetFstFreeSlotOrPBSNoFunc)sdpstLfBMP::getFstFreePBSNo,
    (sdpstGetMaxCandidateCntFunc)smuProperty::getTmsCandidatePageCnt,
    (sdpstUpdateBMPUntilHWMFunc)sdpstDPath::updateBMPUntilHWMInLfBMP
};

/***********************************************************************
 * Description : LfBMP Header 초기화
 *
 * 페이지 생성시 한번에 설정해야할 정보를 모두 설정한다.
 ***********************************************************************/
void  sdpstLfBMP::initBMPHdr( sdpstLfBMPHdr     * aLfBMPHdr,
                              sdpstPageRange      aPageRange,
                              scPageID            aParentBMP,
                              UShort              aSlotNoInParent,
                              UShort              aNewPageCount,
                              UShort              aMetaPageCount,
                              scPageID            aExtDirPID,
                              SShort              aSlotNoInExtDir,
                              scPageID            aRangeFstPID,
                              UShort              aMaxSlotCnt,
                              idBool            * aNeedToChangeMFNL )
{
    sdpstBMPHdr     * sBMPHdr;
    sdpstMFNL         sNewMFNL;
    sdpstRangeMap   * sMapPtr;
    UInt              sLoop;

    IDE_DASSERT( aLfBMPHdr    != NULL );
    IDE_DASSERT( aParentBMP   != SD_NULL_PID );
    IDE_DASSERT( aRangeFstPID != SD_NULL_PID );

    /* lf-bmp header를 초기화하였으므로 sdpPhyPageHdr의
     * freeOffset과 total free size를 변경한다. */
    sdpPhyPage::initLogicalHdr( sdpPhyPage::getHdr( (UChar*)aLfBMPHdr ),
                                ID_SIZEOF( sdpstLfBMPHdr ) );

    sBMPHdr = &(aLfBMPHdr->mBMPHdr);

    sBMPHdr->mType                    = SDPST_LFBMP;
    sBMPHdr->mParentInfo.mParentPID   = aParentBMP;
    sBMPHdr->mParentInfo.mIdxInParent = aSlotNoInParent;
    sBMPHdr->mFstFreeSlotNo           = SDPST_INVALID_SLOTNO;
    sBMPHdr->mFreeSlotCnt             = aMaxSlotCnt;
    sBMPHdr->mMaxSlotCnt              = aMaxSlotCnt;
    sBMPHdr->mSlotCnt                 = 0;
    sBMPHdr->mMFNL                    = SDPST_MFNL_FMT;

    sBMPHdr->mBodyOffset = 
        sdpPhyPage::getDataStartOffset( ID_SIZEOF( sdpstLfBMPHdr ) );

    for ( sLoop = 0; sLoop < SDPST_MFNL_MAX; sLoop++ )
    {
        sBMPHdr->mMFNLTbl[sLoop] = 0;
    }

    aLfBMPHdr->mPageRange = aPageRange;

    /* LfBMP에서의 최초 Data 페이지의 PBSNo를 설정한다.
     * 이후로 변경되지 않는다. 물론 하나의 LfBMP가 Meta Page로 가득차서
     * Data 페이지가 없는 경우가 있는데 이경우에는 Invalid 설정을 한다. */
    if ( aMetaPageCount == aPageRange )
    {
        aLfBMPHdr->mFstDataPagePBSNo = SDPST_INVALID_PBSNO;
    }
    else
    {
        if ( aMetaPageCount >= aPageRange )
        {
            ideLog::log( IDE_SERVER_0,
                         "aMetaPageCount: %u, "
                         "aPageRange: %u\n",
                         aMetaPageCount,
                         aPageRange );

            (void)dump( sdpPhyPage::getPageStartPtr( aLfBMPHdr ) );

            IDE_ASSERT( 0 );
        }
        aLfBMPHdr->mFstDataPagePBSNo = aMetaPageCount;
    }

    /* RangeMap을 초기화한다. getMapPtr()을 호출하기 전에는 반드시
     * sdpstLfBMPHdr의 mBodyOffset은 설정되어 있어야 한다. */
    sMapPtr = getMapPtr( aLfBMPHdr );
    idlOS::memset( sMapPtr, 0x00, ID_SIZEOF(sdpstRangeMap) );

    /* 초기 page range를 추가한다.
     * TotPages와 MFNLtbl[SDPST_MFNL_FUL] 개수는 아래 함수에서 증가시킨다. */
    addPageRangeSlot( aLfBMPHdr,
                      aRangeFstPID,
                      aNewPageCount,
                      aMetaPageCount,
                      aExtDirPID,
                      aSlotNoInExtDir,
                      aNeedToChangeMFNL,
                      &sNewMFNL );
}

/***********************************************************************
 * Description : 1개이상의 Leaf Bitmap 페이지 생성 및 초기화
 *
 * LfBMP를 생성하지 않아도 되는 조건은 새로운 extent를
 * 기존 last LfBMP에 추가하여도 page range 범위내에 포함된다면,
 * 새로운 extent를 위해서 lf-bmp 페이지를 생성하지 않아도 되므로,
 * 기존에 계산했던 lf-bmp 페이지 생성개수를 수정한다.
 *
 * lf-bmp 페이지들은 할당된 extent의 첫번째 페이지부터 인접하게
 * 위치하므로 bmp 중에 처음으로 생성한다.
 *
 ***********************************************************************/
IDE_RC sdpstLfBMP::createAndInitPages( idvSQL               * aStatistics,
                                       sdrMtxStartInfo      * aStartInfo,
                                       scSpaceID              aSpaceID,
                                       sdpstExtDesc         * aExtDesc,
                                       sdpstBfrAllocExtInfo * aBfrInfo,
                                       sdpstAftAllocExtInfo * aAftInfo )
{
    UInt              sState = 0 ;
    sdrMtx            sMtx;
    UInt              sLoop;
    scPageID          sCurrLfPID;
    scPageID          sParentItPID;
    UShort            sSlotNoInParent;
    UChar           * sNewPagePtr;
    sdpstPageRange    sCurrPageRange;
    UShort            sNewPageCnt;
    sdpstPBS          sPBS;
    UShort            sTotMetaCnt;
    UShort            sMetaCnt;
    ULong             sTotPageCnt;
    idBool            sNeedToChangeMFNL;
    scPageID          sRangeFstPID;
    ULong             sSeqNo;

    IDE_DASSERT( aExtDesc         != NULL );
    IDE_DASSERT( aBfrInfo != NULL );
    IDE_DASSERT( aAftInfo != NULL );
    IDE_DASSERT( aStartInfo       != NULL );
    IDE_DASSERT( aAftInfo->mPageCnt[SDPST_LFBMP] > 0 );

    /* 이전 마지막으로 사용된 SlotNo의 다음 SlotNo를 구한다. */
    if ( aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] > 0 )
    {
        /* 이전 마지막 it-bmp에 가용한 slot이 존재하는 경우 */
        sSlotNoInParent = aBfrInfo->mMaxSlotCnt[SDPST_ITBMP] -
                          aBfrInfo->mFreeSlotCnt[SDPST_ITBMP];
    }
    else
    {
        sSlotNoInParent = 0;
    }

    if ( aAftInfo->mPageCnt[SDPST_EXTDIR] > 0 )
    {
       IDE_ASSERT( aExtDesc->mExtFstPID ==
                   aAftInfo->mLstPID[SDPST_EXTDIR] );
    }
    else
    {
        IDE_ASSERT( aBfrInfo->mLstPID[SDPST_EXTDIR] ==
                    aAftInfo->mLstPID[SDPST_EXTDIR] );
    }

    /* 모든 Meta 페이지의 PageBitSet을 처리한다.
     * 페이지 타입은 META 이고, FULL 상태이다. */
    sPBS = (sdpstPBS) (SDPST_BITSET_PAGETP_META | SDPST_BITSET_PAGEFN_FUL);

    sTotPageCnt = aExtDesc->mLength;
    sTotMetaCnt = aAftInfo->mPageCnt[SDPST_EXTDIR] +
                  aAftInfo->mPageCnt[SDPST_LFBMP] +
                  aAftInfo->mPageCnt[SDPST_ITBMP] +
                  aAftInfo->mPageCnt[SDPST_RTBMP] +
                  aAftInfo->mSegHdrCnt;

    aAftInfo->mFullBMPCnt[SDPST_LFBMP] = 0; // 초기화

    /* 현재 Extent의 선택된 Page Range */
    sCurrPageRange = aAftInfo->mPageRange;

    /* 할당된 Extent의 처음 페이지의 PID :
     * ExtDir 페이지 개수를 고려하여 처리한다. */
    sCurrLfPID     = aExtDesc->mExtFstPID +
                     aAftInfo->mPageCnt[SDPST_EXTDIR];
  
    sNewPagePtr    = NULL;

    /* aExtDesc에 대해서는 첫번째 lf-bmp 페이지의 PID와
     * 첫번째 Data 페이지의 PID를 저장해야한다.
     * 하지만 본 함수에서는 첫번재 lf-bmp 페이지의
     * PID만을 알수 있다. */
    aExtDesc->mExtMgmtLfBMP  = sCurrLfPID;
    aExtDesc->mExtFstDataPID = aExtDesc->mExtFstPID + sTotMetaCnt;

    /* BMP 페이지에 설정할 SeqNo를 계산한다. */
    sSeqNo = aBfrInfo->mNxtSeqNo + aAftInfo->mPageCnt[SDPST_EXTDIR];

    /* 생성해야할 lf-bmp 페이지 개수만큼 반복 */
    for ( sLoop = 0;
          sLoop < aAftInfo->mPageCnt[SDPST_LFBMP];
          sLoop++ )
    {
        sState = 0;
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aStartInfo,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        /* lf-bmp 초기화시에 it-bmp 페이지의 pid를 설정해야한다.
         * it-bmp가 여러개 생성될 수 있기도하고, 이전 it-bmp에 저장되기도
         * 하기 때문에 이를 계산해내야 한다.(I/O 비용제거)
         *
         * 이전 it-bmp에 저장될 lf-bmp 페이지의 개수를
         * 이후 생성될 it-bmp 페이지의 PID를 max slot개수로
         * 나누어 계산할 수 있다. */
        if ( aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] > sLoop )
        {
            /* 이전 마지막 itbmp 페이지의 pid */
            sParentItPID  = aBfrInfo->mLstPID[SDPST_ITBMP];
        }
        else
        {
            /*
             * BUG-22958 bitmap segment에서 it-bmp및 rt-bmp를구성하는중 
             * 죽는경우가 있음.
             */
            /* Extent의 첫번째 pid부터 lf bmps들을 다 뺀다음
             * 그 이후의 it bmp가 위치하므로, 그 위치의
             * 페이지의 PID를 계산한다. */
            sParentItPID  = aExtDesc->mExtFstPID +
                            aAftInfo->mPageCnt[SDPST_EXTDIR] +
                            aAftInfo->mPageCnt[SDPST_LFBMP] +
                            ( (sLoop - aBfrInfo->mFreeSlotCnt[SDPST_ITBMP]) /
                              aBfrInfo->mMaxSlotCnt[SDPST_ITBMP] ) ;

            IDE_ASSERT( aAftInfo->mPageCnt[SDPST_ITBMP] > 0 );
        }

        /* LfBMP에 추가될 페이지의 첫번째 PageID를 구한다.  */
        sRangeFstPID =  aExtDesc->mExtFstPID + (aAftInfo->mPageRange * sLoop);

        /* 여러개의 parent It-bmp에 slot 순번을 고려해야하므로
         * 한페이지 들어갈 수 있는 최대 slot 개수로 mod 연산한다.
         * sSlotNoInParent가 max slot 에 도달하게 되면 다시 0부터 시작하게 
         * 하기 위해 mod 연산함. */
        sSlotNoInParent = sSlotNoInParent % aBfrInfo->mMaxSlotCnt[SDPST_ITBMP];

        IDE_TEST( sdpstAllocPage::createPage( aStatistics,
                                              &sMtx,
                                              aSpaceID,
                                              NULL, /* aSegHandle4DataPage */
                                              sCurrLfPID,
                                              sSeqNo,
                                              SDP_PAGE_TMS_LFBMP,
                                              sParentItPID,
                                              sSlotNoInParent,
                                              sPBS,
                                              &sNewPagePtr ) != IDE_SUCCESS );

        /* new LfBMP 의 페이지 개수를 지정한다. */
        sNewPageCnt = ( sTotPageCnt < (UShort)sCurrPageRange ?
                        (UShort)sTotPageCnt : (UShort)sCurrPageRange );
        /* Leaf bmp에 배정된 페이지 개수를 뺀다. */
        sTotPageCnt -= sNewPageCnt;

        /* 생성된 LfBMP에서 Meta 페이지를 관리해야 한다면,
         * PBS 미리 계산하여 변경한다.
         * 변경할 PBS는 sPBS에 담겨있다. */
        if ( sTotMetaCnt > 0 )
        {
            sMetaCnt = ( sTotMetaCnt < sNewPageCnt ?
                         sTotMetaCnt : sNewPageCnt );

            /* PBS을 설정한 meta page 개수를 뺀다. */
            sTotMetaCnt -= sMetaCnt;
        }
        else
        {
            sMetaCnt = 0; // Meta Page에 대한 PageBitSet 설정이 완료됨
        }

        /* LfBMP 페이지 초기화 및 write logging */
        IDE_TEST( logAndInitBMPHdr( &sMtx,
                                    getHdrPtr(sNewPagePtr),
                                    sCurrPageRange,
                                    sParentItPID,
                                    sSlotNoInParent,
                                    sNewPageCnt,
                                    sMetaCnt,
                                    aAftInfo->mLstPID[SDPST_EXTDIR],
                                    aAftInfo->mSlotNoInExtDir,
                                    sRangeFstPID,
                                    aBfrInfo->mMaxSlotCnt[SDPST_LFBMP],
                                    &sNeedToChangeMFNL ) != IDE_SUCCESS );

        if ( sNeedToChangeMFNL == ID_TRUE )
        {
            // FULL 상태로 변경해야할 상위 BMP의 slot의 개수
            aAftInfo->mFullBMPCnt[SDPST_LFBMP]++;
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        // 동일한 Extent내에서 다음 페이지 ID를 구한다.
        sCurrLfPID++;

        // sSlotNo를 순서대로 증가시킨다.
        sSlotNoInParent++;

        /* SeqNo를 증가시킨다. */
        sSeqNo++;
    }

    /* 새로운 leaf bmp 페이지구간을 설정한다. */
    aAftInfo->mFstPID[SDPST_LFBMP] = aExtDesc->mExtFstPID +
                                     aAftInfo->mPageCnt[SDPST_EXTDIR];

    aAftInfo->mLstPID[SDPST_LFBMP] = aExtDesc->mExtFstPID +
                                     aAftInfo->mPageCnt[SDPST_EXTDIR] +
                                     aAftInfo->mPageCnt[SDPST_LFBMP] - 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Leaf BMP Control Header 초기화 및 write logging
 *
 * aMtx             - [IN] Mini Transaction Pointer
 * aLfBMPHdr        - [IN] LfBMP Header
 * aPageRange       - [IN] Page Range
 * aParentItBMP     - [IN] Parent ItBMP
 * aSlotNoInParent  - [IN] Parent에서의 SlotNo
 * aNewPageCnt      - [IN] LfBMP에 추가된 페이지 개수
 * aExtDirPID       - [IN] 추가될 페이지가 포함된 ExtDirPID
 * aSlotNoInExtDir  - [IN] ExtDir 페이지의 SlotNo
 * aRangeFstPID     - [IN] 추가될 첫번째 페이지의 PID
 * aMaxSlotCnt      - [IN] LfBMP의 최대 RangeSlot 개수
 * aNeedToChangeMFNL    - [OUT] MFNL 변경 여부
 ***********************************************************************/
IDE_RC sdpstLfBMP::logAndInitBMPHdr( sdrMtx            * aMtx,
                                     sdpstLfBMPHdr     * aLfBMPHdr,
                                     sdpstPageRange      aPageRange,
                                     scPageID            aParentItBMP,
                                     UShort              aSlotNoInParent,
                                     UShort              aNewPageCnt,
                                     UShort              aMetaPageCnt,
                                     scPageID            aExtDirPID,
                                     SShort              aSlotNoInExtDir,
                                     scPageID            aRangeFstPID,
                                     UShort              aMaxSlotCnt,
                                     idBool            * aNeedToChangeMFNL )
{
    sdpstRedoInitLfBMP  sLogData;

    IDE_DASSERT( aMtx    != NULL );
    IDE_DASSERT( aLfBMPHdr != NULL );

    /* page range slot초기화도 해준다. */
    initBMPHdr( aLfBMPHdr,
                aPageRange,
                aParentItBMP,
                aSlotNoInParent,
                aNewPageCnt,
                aMetaPageCnt,
                aExtDirPID,
                aSlotNoInExtDir,
                aRangeFstPID,
                aMaxSlotCnt,
                aNeedToChangeMFNL );

    /* Logging */
    sLogData.mPageRange      = aPageRange;
    sLogData.mParentInfo.mParentPID   = aParentItBMP;
    sLogData.mParentInfo.mIdxInParent = aSlotNoInParent;
    sLogData.mRangeFstPID    = aRangeFstPID;
    sLogData.mExtDirPID      = aExtDirPID;
    sLogData.mNewPageCnt     = aNewPageCnt;
    sLogData.mMetaPageCnt    = aMetaPageCnt;
    sLogData.mSlotNoInExtDir = aSlotNoInExtDir;
    sLogData.mMaxSlotCnt     = aMaxSlotCnt;

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aLfBMPHdr,
                                         &sLogData,
                                         ID_SIZEOF( sLogData ),
                                         SDR_SDPST_INIT_LFBMP )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : leaf bmp에서 data 페이지의 PageBitSet을 변경한다.
 *
 * leaf bmp 페이지의 MFNL이 변경되는 경우에만 호출해야하므로 호출되기
 * 전에 변경여부를 미리 판단하여야한다.
 ***********************************************************************/
IDE_RC sdpstLfBMP::tryToChangeMFNL( idvSQL          * aStatistics,
                                    sdrMtx          * aMtx,
                                    scSpaceID         aSpaceID,
                                    scPageID          aLfBMP,
                                    SShort            aPBSNo,
                                    sdpstPBS          aNewPBS,
                                    idBool          * aNeedToChangeMFNL,
                                    sdpstMFNL       * aNewMFNL,
                                    scPageID        * aParentItBMP,
                                    SShort          * aSlotNoInParent )
{
    UChar           * sPagePtr;
    sdpstLfBMPHdr   * sLfBMPHdr;
    sdpstRangeMap   * sMapPtr;
    sdpstMFNL         sPageNewFN;
    sdpstMFNL         sPagePrvFN;
    sdpstPBS        * sPBSPtr;
    UShort            sMetaPageCount;

    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( aPBSNo != SDPST_INVALID_PBSNO );
    IDE_DASSERT( aNeedToChangeMFNL != NULL );
    IDE_DASSERT( aNewMFNL != NULL );
    IDE_DASSERT( aParentItBMP != NULL );
    IDE_DASSERT( aSlotNoInParent != NULL );

    /* LfBMP에서 PBS를 변경한다. */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aLfBMP,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sLfBMPHdr = getHdrPtr( sPagePtr );
    sMapPtr   = sdpstLfBMP::getMapPtr( sLfBMPHdr );
    IDE_DASSERT( verifyBMP( sLfBMPHdr ) == IDE_SUCCESS );

    sPBSPtr = &(sMapPtr->mPBSTbl[ aPBSNo ]);

    if ( *sPBSPtr == aNewPBS )
    {
        ideLog::log( IDE_SERVER_0,
                     "Before PBS: %u, "
                     "After PBS : %u\n",
                     *sPBSPtr,
                     aNewPBS );
        (void)dump( sPagePtr );
        IDE_ASSERT( 0 );
    }

    /* 단일 페이지의 가용도 변경을 수행할때는 Unformat인 경우는 없다.
     * 앞서서 페이지가 한꺼번에 포맷이 되기때문이다. */
    if ( isEqFN( *sPBSPtr, SDPST_BITSET_PAGEFN_UNF ) == ID_TRUE )
    {
        ideLog::log( IDE_SERVER_0,
                     "PBS: %u\n",
                     *sPBSPtr );
        (void)dump( sPagePtr );
        IDE_ASSERT( 0 );
    }

    if ( isDataPage( *sPBSPtr ) != ID_TRUE )
    {
        ideLog::log( IDE_SERVER_0,
                     "PBS: %u\n",
                     *sPBSPtr );
        (void)dump( sPagePtr );
        IDE_ASSERT( 0 );
    }

    sPagePrvFN = convertPBSToMFNL( *sPBSPtr );

    if ( isDataPage( aNewPBS ) != ID_TRUE )
    {
        ideLog::log( IDE_SERVER_0,
                     "New PBS: %u\n",
                     aNewPBS );
        (void)dump( sPagePtr );
        IDE_ASSERT( 0 );
    }

    sPageNewFN = convertPBSToMFNL( aNewPBS );

    // Data 페이지의 BitSet을 변경한다.
    IDE_TEST( logAndUpdatePBS( aMtx,
                               sMapPtr,
                               aPBSNo,
                               aNewPBS,
                               1,
                               &sMetaPageCount ) != IDE_SUCCESS );

    /* lf-BMP 헤더의 MFNL Table과 대표 MFNL 을 변경한다. */
    IDE_TEST( sdpstBMP::logAndUpdateMFNL( aMtx,
                                          getBMPHdrPtr(sLfBMPHdr),
                                          SDPST_INVALID_SLOTNO,
                                          SDPST_INVALID_SLOTNO,
                                          sPagePrvFN,
                                          sPageNewFN,
                                          1,
                                          aNewMFNL,
                                          aNeedToChangeMFNL ) != IDE_SUCCESS );

    IDE_DASSERT( verifyBMP( sLfBMPHdr ) == IDE_SUCCESS );

    // 상위 상태를 변경하기 위해서 부모 it-bmp 페이지의 PID를 구한다.
    *aParentItBMP     = sLfBMPHdr->mBMPHdr.mParentInfo.mParentPID;
    *aSlotNoInParent  = sLfBMPHdr->mBMPHdr.mParentInfo.mIdxInParent;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page Range Slot을 추가한다.
 ***********************************************************************/
void sdpstLfBMP::addPageRangeSlot( sdpstLfBMPHdr    * aLfBMPHdr,
                                   scPageID           aFstPID,
                                   UShort             aNewPageCount,
                                   UShort             aMetaPageCount,
                                   scPageID           aExtDirPID,
                                   SShort             aSlotNoInExtDir,
                                   idBool           * aNeedToChangeMFNL,
                                   sdpstMFNL        * aNewMFNL )
{
    SShort            sCurrSlotNo;
    sdpstRangeMap   * sMapPtr;
    SShort            sFstPBSNoInRange;
    sdpstMFNL         sNewMFNL;
    UShort            sMetaPageCount;

    IDE_DASSERT( aFstPID         != SD_NULL_PID );
    IDE_DASSERT( aNewPageCount   <= (UShort)SDPST_PAGE_RANGE_1024 );
    IDE_DASSERT( aNeedToChangeMFNL   != NULL );
    IDE_DASSERT( aNewMFNL        != NULL );
    IDE_DASSERT( aExtDirPID      != SD_NULL_PID );
    IDE_DASSERT( aSlotNoInExtDir != SDPST_INVALID_SLOTNO );


    sCurrSlotNo = aLfBMPHdr->mBMPHdr.mSlotCnt;
    sMapPtr     = getMapPtr( aLfBMPHdr );

    /* Range Slot의 첫번째 PBSNo을 계산한다. */
    if ( sCurrSlotNo == 0 )
    {
        /* 아무것도 없으면 0부터 시작 */
        sFstPBSNoInRange = 0;
    }
    else
    {
        /* 이전 RangeSlot의 마지막 PBSNo 다음 PBSNo */
        sFstPBSNoInRange = sMapPtr->mRangeSlot[ (sCurrSlotNo-1) ].mFstPBSNo +
                           sMapPtr->mRangeSlot[ (sCurrSlotNo-1) ].mLength;
    }

    sMapPtr->mRangeSlot[ sCurrSlotNo ].mFstPID         = aFstPID;
    sMapPtr->mRangeSlot[ sCurrSlotNo ].mLength         = aNewPageCount;
    sMapPtr->mRangeSlot[ sCurrSlotNo ].mFstPBSNo       = sFstPBSNoInRange;
    sMapPtr->mRangeSlot[ sCurrSlotNo ].mExtDirPID      = aExtDirPID;
    sMapPtr->mRangeSlot[ sCurrSlotNo ].mSlotNoInExtDir = aSlotNoInExtDir;

    if ( aMetaPageCount > 0 )
    {
        /* 이 함수는 DATA 페이지의 PBS만 갱신한다.
         * 하지만 초기화시 PBS를 0x00 (DATA | UNF)으로 변경하였기 때문에
         * Meta 페이지 수 만큼 PBS를 변경할 수 있다. */
        updatePBS( sMapPtr,
                   sFstPBSNoInRange,
                   (sdpstPBS)(SDPST_BITSET_PAGETP_META|SDPST_BITSET_PAGEFN_FUL),
                   aMetaPageCount,
                   &sMetaPageCount);
    }

    updatePBS( sMapPtr,
               sFstPBSNoInRange + aMetaPageCount,
               (sdpstPBS)(SDPST_BITSET_PAGETP_DATA|SDPST_BITSET_PAGEFN_FMT),
               aNewPageCount - aMetaPageCount,
               &sMetaPageCount );

    /* Range Slot에 추가된 페이지 개수만큼 총 page 개수를 증가시킨다. */
    aLfBMPHdr->mTotPageCnt                       += aNewPageCount;
    aLfBMPHdr->mBMPHdr.mSlotCnt                  += 1;
    aLfBMPHdr->mBMPHdr.mFreeSlotCnt              -= 1;
    aLfBMPHdr->mBMPHdr.mMFNLTbl[SDPST_MFNL_FUL]  += aMetaPageCount;
    aLfBMPHdr->mBMPHdr.mMFNLTbl[SDPST_MFNL_FMT]  += ( aNewPageCount - aMetaPageCount );

    IDE_DASSERT( isValidTotPageCnt( aLfBMPHdr ) == ID_TRUE );

    /* MFNL을 결정한다.초기값은 SDPST_MFNL_UNF이다.
     * calcMFNL 함수가 호출되기 전에 MFNLtbl의 설정이 완료되어야 한다. */
    sNewMFNL = sdpstAllocPage::calcMFNL( aLfBMPHdr->mBMPHdr.mMFNLTbl );

    if ( aLfBMPHdr->mBMPHdr.mMFNL != sNewMFNL )
    {
        aLfBMPHdr->mBMPHdr.mMFNL = sNewMFNL;
        *aNeedToChangeMFNL = ID_TRUE;
        *aNewMFNL = sNewMFNL;
    }
    else
    {
        *aNeedToChangeMFNL = ID_FALSE;
        *aNewMFNL = aLfBMPHdr->mBMPHdr.mMFNL;
    }
}

/***********************************************************************
 * Description : RangeSlot이 추가될때 TotPage 개수를 검증한다.
 ***********************************************************************/
idBool sdpstLfBMP::isValidTotPageCnt( sdpstLfBMPHdr * aLfBMPHdr )
{
   UInt   sLoop;
   UShort sPageCnt;

   sPageCnt = 0;

   for( sLoop = 0; sLoop < (UInt)SDPST_MFNL_MAX; sLoop++ )
   {
       sPageCnt += aLfBMPHdr->mBMPHdr.mMFNLTbl[sLoop];
   }

   if ( sPageCnt == aLfBMPHdr->mTotPageCnt )
   {
      return ID_TRUE;
   }
   else
   {
      return ID_FALSE;
   }
}

/***********************************************************************
 * Description : Page Range Slot을 추가한다.
 ***********************************************************************/
IDE_RC sdpstLfBMP::logAndAddPageRangeSlot(
                                sdrMtx               * aMtx,
                                sdpstLfBMPHdr        * aLfBMPHdr,
                                scPageID               aFstPID,
                                UShort                 aLength,
                                UShort                 aMetaPageCnt,
                                scPageID               aExtDirPID,
                                SShort                 aSlotNoInExtDir,
                                idBool               * aNeedToChangeMFNL,
                                sdpstMFNL            * aNewMFNL )
{
    sdpstRedoAddRangeSlot   sLogData;

    IDE_DASSERT( aLfBMPHdr  != NULL );
    IDE_DASSERT( aMtx      != NULL );
    IDE_DASSERT( aNeedToChangeMFNL != NULL );

    addPageRangeSlot( aLfBMPHdr,
                      aFstPID,
                      aLength,  // 총 페이지 개수 */
                      aMetaPageCnt,
                      aExtDirPID,
                      aSlotNoInExtDir,
                      aNeedToChangeMFNL,
                      aNewMFNL );

    // ADD_RANGESLOT logging
    sLogData.mFstPID         = aFstPID;
    sLogData.mLength         = aLength;
    sLogData.mMetaPageCnt    = aMetaPageCnt;
    sLogData.mExtDirPID      = aExtDirPID;
    sLogData.mSlotNoInExtDir = aSlotNoInExtDir;

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aLfBMPHdr,
                                         &sLogData,
                                         ID_SIZEOF( sLogData ),
                                         SDR_SDPST_ADD_RANGESLOT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : data 페이지들의 pagebitset을 갱신한다.
 ***********************************************************************/
IDE_RC sdpstLfBMP::logAndUpdatePBS( sdrMtx        * aMtx,
                                    sdpstRangeMap * aMapPtr,
                                    SShort          aFstPBSNo,
                                    sdpstPBS        aPBS,
                                    UShort          aPageCnt,
                                    UShort        * aMetaPageCnt )
{
    sdpstRedoUpdatePBS  sLogData;

    updatePBS( aMapPtr,
               aFstPBSNo,
               aPBS,
               aPageCnt,
               aMetaPageCnt );

    sLogData.mFstPBSNo = aFstPBSNo;
    sLogData.mPBS      = aPBS;
    sLogData.mPageCnt  = aPageCnt;

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aMapPtr,
                                         &sLogData,
                                         ID_SIZEOF(sLogData),
                                         SDR_SDPST_UPDATE_PBS )
            != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : data 페이지들의 pagebitset을 갱신한다.
 ***********************************************************************/
void sdpstLfBMP::updatePBS( sdpstRangeMap * aMapPtr,
                            SShort          aFstPBSNo,
                            sdpstPBS        aPBS,
                            UShort          aPageCnt,
                            UShort        * aMetaPageCnt )
{
    UShort             sLoop;
    UShort             sMetaPageCnt;
    sdpstPBS         * sPBSPtr;

    IDE_DASSERT( aMapPtr != NULL );

    sMetaPageCnt   = 0;
    sPBSPtr = aMapPtr->mPBSTbl;

    for( sLoop = aFstPBSNo; sLoop < aFstPBSNo + aPageCnt; sLoop++ )
    {
        if ( isDataPage(sPBSPtr[sLoop]) == ID_TRUE )
        {
            sPBSPtr[sLoop] = aPBS;
        }
        else
        {
            sMetaPageCnt++;
        }
    }

    *aMetaPageCnt = sMetaPageCnt;
}

/***********************************************************************
 * Description : Extent의 첫번째 페이지의 PID에 해당하는 Leaf Bitmap
 *               페이지에서의 페이지 위치를 반환한다.
 ***********************************************************************/
IDE_RC sdpstLfBMP::getPBSNoByExtFstPID( idvSQL          * aStatistics,
                                       scSpaceID         aSpaceID,
                                       scPageID          aLfBMP,
                                       scPageID          aExtFstPID,
                                       SShort          * aPBSNo )
{
    idBool               sDummy;
    UChar              * sPagePtr;
    SShort               sSlotNo;
    sdpstLfBMPHdr      * sLfBMPHdr;

    IDE_DASSERT( aLfBMP     != SD_NULL_PID );
    IDE_DASSERT( aExtFstPID != SD_NULL_PID );
    IDE_DASSERT( aPBSNo   != NULL );

    // 이전 leaf를 fix해 봐야 한다.
    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aLfBMP,
                                          &sPagePtr,
                                          &sDummy) != IDE_SUCCESS );

    sLfBMPHdr = getHdrPtr(sPagePtr );

    sSlotNo = findSlotNoByExtPID( sLfBMPHdr, aExtFstPID );

    *aPBSNo = getRangeSlotBySlotNo( getMapPtr(sLfBMPHdr), sSlotNo )->mFstPBSNo;

    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                       sPagePtr ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstLfBMP::verifyBMP( sdpstLfBMPHdr  * aBMPHdr )
{
    SShort             sCurIdx;
    UShort             sLoop;
    UShort             sPageCnt;
    sdpstMFNL          sMFNL;
    UShort             sMFNLtbl[ SDPST_MFNL_MAX ];
    sdpstPBS         * sPBSPtr;

    sPageCnt = aBMPHdr->mTotPageCnt;
    sPBSPtr  = getMapPtr(aBMPHdr)->mPBSTbl;

    idlOS::memset( &sMFNLtbl, 0x00, ID_SIZEOF(UShort) * SDPST_MFNL_MAX );

    for( sLoop = 0, sCurIdx = 0; sLoop < sPageCnt;
         sCurIdx++, sLoop++ )
    {
        sMFNL = convertPBSToMFNL( *(sPBSPtr + sCurIdx) );
        sMFNLtbl[sMFNL] += 1;
    }

    for ( sLoop = 0; sLoop < SDPST_MFNL_MAX; sLoop++ )
    {
        if ( aBMPHdr->mBMPHdr.mMFNLTbl[ sLoop ] != sMFNLtbl[ sLoop ] )
        {
            ideLog::log( IDE_SERVER_0,
                         "BMP Header.mMFNLTbl[FUL] = %u\n"
                         "BMP Header.mMFNLTbl[INS] = %u\n"
                         "BMP Header.mMFNLTbl[FMT] = %u\n"
                         "BMP Header.mMFNLTbl[UNF] = %u\n"
                         "sMFNLTbl[FUL]            = %u\n"
                         "sMFNLTbl[INS]            = %u\n"
                         "sMFNLTbl[FMT]            = %u\n"
                         "sMFNLTbl[UNF]            = %u\n",
                         aBMPHdr->mBMPHdr.mMFNLTbl[SDPST_MFNL_FUL],
                         aBMPHdr->mBMPHdr.mMFNLTbl[SDPST_MFNL_INS],
                         aBMPHdr->mBMPHdr.mMFNLTbl[SDPST_MFNL_FMT],
                         aBMPHdr->mBMPHdr.mMFNLTbl[SDPST_MFNL_UNF],
                         sMFNLtbl[SDPST_MFNL_FUL],
                         sMFNLtbl[SDPST_MFNL_INS],
                         sMFNLtbl[SDPST_MFNL_FMT],
                         sMFNLtbl[SDPST_MFNL_UNF] );

            (void)dump( sdpPhyPage::getPageStartPtr( aBMPHdr ) );

            IDE_ASSERT( 0 );
        }
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : LfBMP를 dump한다.
 ***********************************************************************/
IDE_RC sdpstLfBMP::dumpHdr( UChar    * aPagePtr,
                            SChar    * aOutBuf,
                            UInt       aOutSize )
{
    UChar           * sPagePtr;
    sdpstLfBMPHdr   * sLfBMPHdr;
    sdpstBMPHdr     * sBMPHdr;

    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aOutBuf  != NULL );
    IDE_ERROR( aOutSize > 0 );

    sPagePtr  = sdpPhyPage::getPageStartPtr( aPagePtr );
    sLfBMPHdr = getHdrPtr( sPagePtr );
    sBMPHdr   = &sLfBMPHdr->mBMPHdr;

    /* LfBMP Header */
    idlOS::snprintf(
        aOutBuf,
        aOutSize,
        "--------------- LfBMP Header Begin ----------------\n"
        "Type                   : %"ID_UINT32_FMT"\n"
        "MFNL                   : %"ID_UINT32_FMT"\n"
        "MFNL Table (UNF)       : %"ID_UINT32_FMT"\n"
        "MFNL Table (FMT)       : %"ID_UINT32_FMT"\n"
        "MFNL Table (INS)       : %"ID_UINT32_FMT"\n"
        "MFNL Table (FUL)       : %"ID_UINT32_FMT"\n"
        "Slot Count             : %"ID_UINT32_FMT"\n"
        "Free Slot Count        : %"ID_UINT32_FMT"\n"
        "Max Slot Count         : %"ID_UINT32_FMT"\n"
        "First Free SlotNo      : %"ID_INT32_FMT"\n"
        "Parent Page ID         : %"ID_UINT32_FMT"\n"
        "SlotNo In Parent       : %"ID_INT32_FMT"\n"
        "Next RtBMP Page ID     : %"ID_UINT32_FMT"\n"
        "Body Offset            : %"ID_UINT32_FMT"\n"
        "Page Range             : %"ID_UINT32_FMT"\n"
        "Total Page Count       : %"ID_UINT32_FMT"\n"
        "First Data Page PBS No : %"ID_INT32_FMT"\n"
        "---------------  LfBMP Header End  ----------------\n",
        sBMPHdr->mType,
        sBMPHdr->mMFNL,
        sBMPHdr->mMFNLTbl[SDPST_MFNL_UNF],
        sBMPHdr->mMFNLTbl[SDPST_MFNL_FMT],
        sBMPHdr->mMFNLTbl[SDPST_MFNL_INS],
        sBMPHdr->mMFNLTbl[SDPST_MFNL_FUL],
        sBMPHdr->mSlotCnt,
        sBMPHdr->mFreeSlotCnt,
        sBMPHdr->mMaxSlotCnt,
        sBMPHdr->mFstFreeSlotNo,
        sBMPHdr->mParentInfo.mParentPID,
        sBMPHdr->mParentInfo.mIdxInParent,
        sBMPHdr->mNxtRtBMP,
        sBMPHdr->mBodyOffset,
        sLfBMPHdr->mPageRange,
        sLfBMPHdr->mTotPageCnt,
        sLfBMPHdr->mFstDataPagePBSNo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LfBMP를 dump한다.
 ***********************************************************************/
IDE_RC sdpstLfBMP::dumpBody( UChar    * aPagePtr,
                             SChar    * aOutBuf,
                             UInt       aOutSize )
{
    UChar           * sPagePtr;
    sdpstLfBMPHdr   * sLfBMPHdr;
    sdpstBMPHdr     * sBMPHdr;
    sdpstRangeMap   * sRangeMap;
    sdpstRangeSlot  * sRangeSlot;
    sdpstPBS        * sPBSTbl;
    UInt              sLoop;
    SChar           * sDumpBuf;

    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aOutBuf  != NULL );

    sPagePtr  = sdpPhyPage::getPageStartPtr( aPagePtr );
    sLfBMPHdr = getHdrPtr( sPagePtr );
    sBMPHdr   = &sLfBMPHdr->mBMPHdr;
    sRangeMap = getMapPtr( sLfBMPHdr );

    /* BMP RangeSlot */
    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "--------------- LfBMP RangeSlot Begin ----------------\n" );

    for ( sLoop = 0; sLoop < sBMPHdr->mSlotCnt; sLoop++ )
    {
        sRangeSlot = &sRangeMap->mRangeSlot[sLoop];

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "[%"ID_UINT32_FMT"] "
                             "FstPID: %"ID_UINT32_FMT", "
                             "Length: %"ID_UINT32_FMT", "
                             "FstPBSNo: %"ID_INT32_FMT", "
                             "ExtDirPID: %"ID_UINT32_FMT", "
                             "SlotNoInExtDir: %"ID_INT32_FMT"\n",
                             sLoop,
                             sRangeSlot->mFstPID,
                             sRangeSlot->mLength,
                             sRangeSlot->mFstPBSNo,
                             sRangeSlot->mExtDirPID,
                             sRangeSlot->mSlotNoInExtDir );
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "---------------  LfBMP RangeSlot End  ----------------\n" );

    if( iduMemMgr::calloc(
            IDU_MEM_SM_SDP, 1,
            ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
            (void**)&sDumpBuf ) == IDE_SUCCESS )
    {
        /* BMP PBS Table */
        sPBSTbl = sRangeMap->mPBSTbl;

        ideLog::ideMemToHexStr( sPBSTbl,
                                SDPST_PAGE_BITSET_TABLE_SIZE,
                                IDE_DUMP_FORMAT_NORMAL,
                                sDumpBuf,
                                IDE_DUMP_DEST_LIMIT );

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "--------------- LfBMP PBS Table Begin ----------------\n"
                             "%s\n"
                             "---------------  LfBMP PBS Table End  ----------------\n",
                             sDumpBuf );

        (void)iduMemMgr::free( sDumpBuf );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LfBMP를 dump한다.
 ***********************************************************************/
IDE_RC sdpstLfBMP::dump( UChar    * aPagePtr )
{
    UChar           * sPagePtr;
    SChar           * sDumpBuf;

    IDE_ERROR( aPagePtr != NULL );

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
        /* LfBMP Header */
        if( dumpHdr( sPagePtr,
                     sDumpBuf,
                     IDE_DUMP_DEST_LIMIT ) == IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "%s", sDumpBuf );
        }

        /* LfBMP Body */
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
