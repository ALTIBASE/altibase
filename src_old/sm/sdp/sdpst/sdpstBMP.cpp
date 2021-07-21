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
 * $Id: sdpstBMP.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Internal Bitmap 페이지 관련
 * STATIC 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <sdr.h>
# include <sdpReq.h>

# include <sdpstBMP.h>
# include <sdpstRtBMP.h>
# include <sdpstItBMP.h>
# include <sdpstLfBMP.h>
# include <sdpstAllocPage.h>

# include <sdpstSH.h>
# include <sdpstStackMgr.h>
# include <sdpstCache.h>
# include <sdpstAllocExtInfo.h>

sdpstBMPOps * sdpstBMP::mBMPOps[SDPST_BMP_TYPE_MAX] =
{
    &sdpstRtBMP::mVirtBMPOps,
    &sdpstRtBMP::mRtBMPOps,
    &sdpstItBMP::mItBMPOps,
    &sdpstLfBMP::mLfBMPOps,
};

/***********************************************************************
 * Description : 1개이상의 Internal Bitmap 페이지 생성 및 초기화
 *
 * internal bmp 페이지들은 할당된 extent의 lf-bmp 페이지들이 모두
 * 생성된 이후 페이지에서부터 위치한다.
 ***********************************************************************/
IDE_RC sdpstBMP::createAndInitPages( idvSQL               * aStatistics,
                                     sdrMtxStartInfo      * aStartInfo,
                                     scSpaceID              aSpaceID,
                                     sdpstExtDesc         * aExtDesc,
                                     sdpstBMPType           aBMPType,
                                     sdpstBfrAllocExtInfo * aBfrInfo,
                                     sdpstAftAllocExtInfo * aAftInfo )
{
    UInt                sState = 0;
    sdrMtx              sMtx;
    SInt                sLoop;
    sdpstBMPType        sChildBMPType;
    sdpstBMPType        sParentBMPType;
    scPageID            sCurPID;
    scPageID            sParentBMP = SD_NULL_PID ;
    scPageID            sFstChildBMP;
    scPageID            sLstChildBMP;
    UShort              sSlotNoInParent;
    UShort              sMaxSlotCnt;
    UShort              sMaxSlotCntInParent = SC_NULL_SLOTNUM;
    UShort              sNewSlotCnt;
    UShort              sTotFullBMPCnt;
    UShort              sFullBMPCnt;
    UShort              sSlotCnt;
    sdpstPBS            sPBS;
    UChar             * sNewPagePtr;
    idBool              sNeedToChangeMFNL = ID_FALSE ;
    sdpPageType         sPageType;
    ULong               sSeqNo;

    IDE_DASSERT( aExtDesc   != NULL );
    IDE_DASSERT( aBfrInfo   != NULL );
    IDE_DASSERT( aAftInfo   != NULL );
    IDE_DASSERT( aStartInfo != NULL );
    IDE_DASSERT( aBMPType == SDPST_RTBMP || aBMPType == SDPST_ITBMP );

    /*
     * Rt-BMP에서는 Parent가 없기 때문에 sParentBMPType 변수를 사용하면 안된다. 
     * 아래 매크로는 입력값 검증을 수행하지 않는다.
     */

    sParentBMPType = SDPST_BMP_PARENT_TYPE( aBMPType );
    sChildBMPType  = SDPST_BMP_CHILD_TYPE( aBMPType );

    /* createPage 할때, phyPage에 설정할 page type 을 지정한다. */
    sPageType      = aBMPType == SDPST_ITBMP ?
                     SDP_PAGE_TMS_ITBMP : SDP_PAGE_TMS_RTBMP;

    /* 생성된 마지막 하위 BMP 다음부터 생성을 시작한다.
     * Extent가 할당 될때, ExtDir, LfBMP, ItBMP, RtBMP 순으로 생성되기 때문.
     * 즉, 항상 하위 BMP 가 먼서 생성된다. */
    sCurPID     = aAftInfo->mLstPID[ sChildBMPType ] + 1;
    sMaxSlotCnt = aBfrInfo->mMaxSlotCnt[ aBMPType ];
    sPBS        = (SDPST_BITSET_PAGETP_META | SDPST_BITSET_PAGEFN_FUL);

    /* 상위 BMP의 최대 slot 개수와
     * 상위 BMP에서 이전 마지막 slot 위치 다음을 선택한다. (삽입위치)
     * - RtBMP 에서는 의미 없다.
     *
     * 또한 페이지에 설정할 SeqNo를 계산한다. */
    if ( aBMPType == SDPST_ITBMP )
    {
        sMaxSlotCntInParent = aBfrInfo->mMaxSlotCnt[ sParentBMPType ];
        sSlotNoInParent     = sMaxSlotCntInParent -
                              aBfrInfo->mFreeSlotCnt[ sParentBMPType ];

        sSeqNo = aBfrInfo->mNxtSeqNo +
                 aAftInfo->mPageCnt[SDPST_EXTDIR] +
                 aAftInfo->mPageCnt[SDPST_LFBMP];
    }
    else
    {
        sSlotNoInParent = 0;

        sSeqNo = aBfrInfo->mNxtSeqNo +
                 aAftInfo->mPageCnt[SDPST_EXTDIR] +
                 aAftInfo->mPageCnt[SDPST_LFBMP] +
                 aAftInfo->mPageCnt[SDPST_ITBMP];
    }

    /* 새로 생성한 BMP페이지에 기록된 하위 BMP 개수 */
    sNewSlotCnt    = aAftInfo->mPageCnt[ sChildBMPType ] -
                     aBfrInfo->mFreeSlotCnt[ aBMPType ];

    /* Full 상태로 기록되어야할 BMP 페이지 개수 */
    if ( aBfrInfo->mFreeSlotCnt[ aBMPType ] >=
         aAftInfo->mFullBMPCnt[ sChildBMPType ] )
    {
        sTotFullBMPCnt = 0;
    }
    else
    {
        sTotFullBMPCnt = aAftInfo->mFullBMPCnt[ sChildBMPType ] -
                         aBfrInfo->mFreeSlotCnt[ aBMPType ];

    }

    /* 생성해야할 페이지 개수만큼 반복 */
    for ( sLoop = 0; sLoop < aAftInfo->mPageCnt[ aBMPType ]; sLoop++ )
    {
        sState = 0;
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aStartInfo,
                                       ID_TRUE,/*Undoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT ) != IDE_SUCCESS );
        sState = 1;

        if ( aBMPType == SDPST_ITBMP )
        {
            /* Rt-BMP가 아닌 경우 초기화시 상위 BMP의 PID를 설정해야 한다.
             * 지금 생성되는 페이지는 이후  새로 생성될 상위 BMP 페이지에 기록
             * 될수도 있고, 기존 BMP에 기록될 수도 있다. */
            if ( aBfrInfo->mFreeSlotCnt[ sParentBMPType ] > sLoop )
            {
                sParentBMP = aBfrInfo->mLstPID[ sParentBMPType ];
            }
            else
            {
                sParentBMP = aExtDesc->mExtFstPID +
                             aAftInfo->mPageCnt[SDPST_EXTDIR] +
                             aAftInfo->mPageCnt[SDPST_LFBMP] +
                             aAftInfo->mPageCnt[SDPST_ITBMP] +
                             ((sLoop - aBfrInfo->mFreeSlotCnt[SDPST_RTBMP]) /
                              aBfrInfo->mMaxSlotCnt[SDPST_RTBMP]);

                if ( aAftInfo->mPageCnt[ sParentBMPType ] <= 0 )
                {
                    ideLog::log( IDE_SERVER_0,
                                 "Curr BMP Type: %u, "
                                 "Parent BMP Type: %u, "
                                 "Child BMP Type: %u\n",
                                 aBMPType,
                                 sParentBMPType,
                                 sChildBMPType );

                    sdpstAllocExtInfo::dump( aBfrInfo );
                    sdpstAllocExtInfo::dump( aAftInfo );

                    IDE_ASSERT( 0 );
                }
            }

            /* 여러개의 Parent BMP에 현 BMP가 들어갈 slot 순번을 고려해야 하므로
             * 한 페이지에 들어갈 수 있는 최대 slot개수로 mod 연산한다. */
            sSlotNoInParent = sSlotNoInParent % sMaxSlotCntInParent;

            /* 새 BMP 페이지에 기록될 첫번째 하위 BMP 페이지의 PID를 계산한다 */
            sFstChildBMP = aExtDesc->mExtFstPID +
                           aAftInfo->mPageCnt[SDPST_EXTDIR] +
                           aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] +
                           (sMaxSlotCnt * sLoop);
        }
        else
        {
            IDE_DASSERT( aBMPType == SDPST_RTBMP );
            sFstChildBMP = aExtDesc->mExtFstPID +
                           aAftInfo->mPageCnt[SDPST_EXTDIR] +
                           aAftInfo->mPageCnt[SDPST_LFBMP] +
                           aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] +
                           (sMaxSlotCnt * sLoop);
        }

        if ( sNewSlotCnt >= sMaxSlotCnt )
        {
            sSlotCnt = sMaxSlotCnt;
        }
        else
        {
            sSlotCnt = sNewSlotCnt;
        }

        /* 마지막 하위 BMP */
        sLstChildBMP = sFstChildBMP + sSlotCnt - 1;
        /* 기록했으므로 전체 기록해야할 개수에서 뺀다. */
        sNewSlotCnt -= sSlotCnt;

        IDE_TEST( sdpstAllocPage::createPage( aStatistics,
                                              &sMtx,
                                              aSpaceID,
                                              NULL /*aSegHandle4DataPage*/,
                                              sCurPID,
                                              sSeqNo,
                                              sPageType,
                                              aExtDesc->mExtFstPID,
                                              SDPST_INVALID_SLOTNO,
                                              sPBS,
                                              &sNewPagePtr ) != IDE_SUCCESS );

        if ( sTotFullBMPCnt > 0 )
        {
            sFullBMPCnt     = ( sTotFullBMPCnt <= sSlotCnt ?
                                sTotFullBMPCnt : sSlotCnt );
            sTotFullBMPCnt -= sFullBMPCnt;
        }
        else
        {
            sFullBMPCnt = 0;
        }

        /* ItBMP 초기화 및 write logging */
        IDE_TEST( logAndInitBMPHdr( &sMtx,
                                    getHdrPtr( sNewPagePtr ),
                                    aBMPType,
                                    sParentBMP,
                                    sSlotNoInParent,
                                    sFstChildBMP,
                                    sLstChildBMP,
                                    sFullBMPCnt,
                                    sMaxSlotCnt,
                                    &sNeedToChangeMFNL ) != IDE_SUCCESS );

        if ( sNeedToChangeMFNL == ID_TRUE )
        {
            /* FULL 상태로 변경해야할 상위 itslot의 개수 */
            aAftInfo->mFullBMPCnt[sParentBMPType]++;
        }

        if ( aBMPType == SDPST_RTBMP )
        {
            /* 마지막으로 생성될 RtBMP가 아닌 경우에만 다음 생성할 RtBMP PID
             * 를 설정한다. */
            if ( sLoop != aAftInfo->mPageCnt[ aBMPType ] - 1 )
            {
                IDE_TEST( sdpstRtBMP::setNxtRtBMP( aStatistics,
                                                   &sMtx,
                                                   aSpaceID,
                                                   sCurPID,
                                                   sCurPID + 1 )
                          != IDE_SUCCESS );
            }
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        /* 동일한 Extent내에서 다음 페이지 ID와 slotNo를 구한다 */
        sCurPID = sCurPID + 1;
        sSlotNoInParent++;
        sSeqNo++;
    }

    /* 새로운 BMP 페이지 구간을 설정한다. */
    aAftInfo->mFstPID[aBMPType] = aAftInfo->mLstPID[sChildBMPType] + 1;
    aAftInfo->mLstPID[aBMPType] = aAftInfo->mLstPID[sChildBMPType] +
                                  aAftInfo->mPageCnt[aBMPType];

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : It-BMP 페이지 헤더를 초기화 한다.
 *
 * aBMPHdr          - [IN] BMP header
 * aBMPType         - [IN] BMP Type ( IIBMP or RTBMP )
 * aBodyOffset      - [IN] Body Offset (RtBMP가 SegHdr에 있으면 이 값이 달라짐)
 * aParentBMP       - [IN] Parent BMP PID
 * aSlotNoInParent  - [IN] Parent BMP에서의 SlotNo
 * aFstBMP          - [IN] 첫번째 child BMP PID
 * aLstBMP          - [IN] 마지막 child BMP PID
 * aFullBMPs        - [IN] FULL 상태로 기록되어야할 Child BMP 개수
 * aMaxSlotCnt      - [IN] BMP의 최대 Slot 개수
 * aNeedToChangeMFNL    - [OUT] MFNL 갱신 여부
 ***********************************************************************/
void  sdpstBMP::initBMPHdr( sdpstBMPHdr    * aBMPHdr,
                            sdpstBMPType     aBMPType,
                            scOffset         aBodyOffset,
                            scPageID         aParentBMP,
                            UShort           aSlotNoInParent,
                            scPageID         aFstBMP,
                            scPageID         aLstBMP,
                            UShort           aFullBMPs,
                            UShort           aMaxSlotCnt,
                            idBool         * aNeedToChangeMFNL )
{
    UInt              sNewSlotCount;
    sdpstMFNL         sNewMFNL;
    UInt              sLoop;

    IDE_DASSERT( aBMPHdr != NULL );
    IDE_DASSERT( aFstBMP <= aLstBMP );

    /* 동일한 Extent 내의 페이지들이므로 PID로 페이지 개수를 구한다. */
    sNewSlotCount = aLstBMP - aFstBMP + 1;

    if ( aMaxSlotCnt < sNewSlotCount )
    {
        ideLog::log( IDE_SERVER_0,
                     "aMaxSlotCnt: %u, "
                     "sNewSlotCount: %u\n",
                     aMaxSlotCnt,
                     sNewSlotCount );
        (void)dump( sdpPhyPage::getPageStartPtr( aBMPHdr ) );
        IDE_ASSERT( 0 );
    }

    if ( sNewSlotCount < aFullBMPs )
    {
        ideLog::log( IDE_SERVER_0,
                     "sNewSlotCount: %u, ",
                     "aFullBMPs: %u\n",
                     sNewSlotCount,
                     aFullBMPs );
        (void)dump( sdpPhyPage::getPageStartPtr( aBMPHdr ) );
        IDE_ASSERT( 0 );
    }

    /* it-bmp control header를 초기화하였으므로 sdpPhyPageHdr의
     * freeOffset과 total free size를 변경한다. */
    sdpPhyPage::initLogicalHdr( sdpPhyPage::getHdr( (UChar*)aBMPHdr ),
                                ID_SIZEOF( sdpstBMPHdr ) );

    aBMPHdr->mType                    = aBMPType;
    aBMPHdr->mParentInfo.mParentPID   = aParentBMP;
    aBMPHdr->mParentInfo.mIdxInParent = aSlotNoInParent;
    aBMPHdr->mFstFreeSlotNo           = SDPST_INVALID_SLOTNO;
    aBMPHdr->mFreeSlotCnt             = aMaxSlotCnt;
    aBMPHdr->mMaxSlotCnt              = aMaxSlotCnt;
    aBMPHdr->mNxtRtBMP                = SD_NULL_PID;

    /* 페이지에서의 Body 영역의 절대 Offset을 구한다. */
    aBMPHdr->mBodyOffset = aBodyOffset;

    for ( sLoop = 0; sLoop < SDPST_MFNL_MAX; sLoop++ )
    {
        aBMPHdr->mMFNLTbl[sLoop] = 0;
    }

    aBMPHdr->mSlotCnt = 0;
    aBMPHdr->mMFNL    = SDPST_MFNL_FMT;

    if ( aFstBMP != SD_NULL_PID )
    {
        addSlots( aBMPHdr, (SShort)0, aFstBMP, aLstBMP, aFullBMPs,
                  aNeedToChangeMFNL, &sNewMFNL );
    }
}


/***********************************************************************
 * Description : Internal BMP Control Header 초기화
 ***********************************************************************/
void  sdpstBMP::initBMPHdr( sdpstBMPHdr    * aBMPHdr,
                            sdpstBMPType     aBMPType,
                            scPageID         aParentBMP,
                            UShort           aSlotNoInParent,
                            scPageID         aFstBMP,
                            scPageID         aLstBMP,
                            UShort           aFullBMPs,
                            UShort           aMaxSlotCnt,
                            idBool         * aNeedToChangeMFNL )
{
    initBMPHdr( aBMPHdr,
                aBMPType,
                sdpPhyPage::getDataStartOffset(ID_SIZEOF(sdpstBMPHdr)),
                aParentBMP,
                aSlotNoInParent,
                aFstBMP,
                aLstBMP,
                aFullBMPs,
                aMaxSlotCnt,
                aNeedToChangeMFNL );
}


/***********************************************************************
 * Description : Internal BMP Control Header 초기화 및 write logging
 *
 * aMtx             - [IN] Mini Transaction Pointer
 * aBMPHdr          - [IN] BMP Header
 * aBMPType         - [IN] BMP Type ( IIBMP or RTBMP )
 * aParentBMP       - [IN] Parent BMP PID
 * aSlotNoInParent  - [IN] Parent BMP에서의 SlotNo
 * aFstBMP          - [IN] 첫번째 child BMP PID
 * aLstBMP          - [IN] 마지막 child BMP PID
 * aFullBMPs        - [IN] FULL 상태로 기록되어야할 Child BMP 개수
 * aMaxSlotCnt      - [IN] BMP의 최대 Slot 개수
 * aNeedToChangeMFNL    - [OUT] MFNL 갱신 여부
 ***********************************************************************/
IDE_RC sdpstBMP::logAndInitBMPHdr( sdrMtx            * aMtx,
                                   sdpstBMPHdr       * aBMPHdr,
                                   sdpstBMPType        aBMPType,
                                   scPageID            aParentBMP,
                                   UShort              aSlotNoInParent,
                                   scPageID            aFstBMP,
                                   scPageID            aLstBMP,
                                   UShort              aFullBMPs,
                                   UShort              aMaxSlotCnt,
                                   idBool            * aNeedToChangeMFNL)
{
    sdpstRedoInitBMP    sLogData;

    IDE_DASSERT( aBMPHdr       != NULL );
    IDE_DASSERT( aMtx          != NULL );
    IDE_DASSERT( aNeedToChangeMFNL != NULL );

    initBMPHdr( aBMPHdr,
                aBMPType,
                aParentBMP,
                aSlotNoInParent,
                aFstBMP,
                aLstBMP,
                aFullBMPs,
                aMaxSlotCnt,
                aNeedToChangeMFNL );

    /* INIT_BMP 페이지 logging */
    sLogData.mType        = aBMPType;
    sLogData.mParentInfo.mParentPID   = aParentBMP;
    sLogData.mParentInfo.mIdxInParent = aSlotNoInParent;
    sLogData.mFstChildBMP = aFstBMP;
    sLogData.mLstChildBMP = aLstBMP;
    sLogData.mFullBMPCnt  = aFullBMPs;
    sLogData.mMaxSlotCnt  = aMaxSlotCnt;

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aBMPHdr,
                                         &sLogData,
                                         ID_SIZEOF( sLogData ),
                                         SDR_SDPST_INIT_BMP )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : BMP 페이지에 Slot을 추가한다.
 ***********************************************************************/
void sdpstBMP::addSlots( sdpstBMPHdr * aBMPHdr,
                         SShort        aLstSlotNo,
                         scPageID      aFstBMP,
                         scPageID      aLstBMP,
                         UShort        aFullBMPs,
                         idBool      * aNeedToChangeMFNL,
                         sdpstMFNL   * aNewMFNL )
{
    sdpstMFNL   sNewMFNL;
    UShort      sNewSlotCount;

    IDE_ASSERT( aBMPHdr != NULL );
    sNewSlotCount = aLstBMP - aFstBMP + 1;

    addSlotsToMap( getMapPtr(aBMPHdr),
                   aLstSlotNo,
                   aFstBMP,
                   aLstBMP,
                   aFullBMPs );

    aBMPHdr->mSlotCnt     += sNewSlotCount;
    aBMPHdr->mFreeSlotCnt -= sNewSlotCount;

    aBMPHdr->mMFNLTbl[SDPST_MFNL_FUL] += aFullBMPs;
    aBMPHdr->mMFNLTbl[SDPST_MFNL_FMT]  += (sNewSlotCount - aFullBMPs);

    sNewMFNL = sdpstAllocPage::calcMFNL(aBMPHdr->mMFNLTbl);

    if ( sNewMFNL == SDPST_MFNL_FUL )
    {
        (void)dump( sdpPhyPage::getPageStartPtr( aBMPHdr ) );
        IDE_ASSERT( 0 );
    }


    if ( aBMPHdr->mFstFreeSlotNo == SDPST_INVALID_SLOTNO )
    {
        aBMPHdr->mFstFreeSlotNo = aLstSlotNo + aFullBMPs;
    }
    else
    {
        /* 이전 MFNL상태가 FULL에서 FstFree가 마지막 Slot이었다면,
         * 새로 추가된 Slot으로 갱신한다. */
        if ( (aBMPHdr->mMFNL == SDPST_MFNL_FUL ) &&
             (aBMPHdr->mFstFreeSlotNo + 1 == aLstSlotNo ))
        {
            aBMPHdr->mFstFreeSlotNo  = aLstSlotNo + aFullBMPs;
        }
    }

    if ( aBMPHdr->mMFNL != sNewMFNL )
    {
        aBMPHdr->mMFNL = sNewMFNL;
        *aNeedToChangeMFNL = ID_TRUE;
        *aNewMFNL      = sNewMFNL;
    }
    else
    {
        /* slot이 추가되어도 MFNL이 변경되지 않는다.  */
        *aNeedToChangeMFNL = ID_FALSE;
        *aNewMFNL      = aBMPHdr->mMFNL;
    }
}

/***********************************************************************
 * Description : Internal Bitmap 페이지의 lf-slot들을 추가하고
 *               MFNLtbl과 MFNL을 변경하는 로깅을 한다.
 ************************************************************************/
IDE_RC sdpstBMP::logAndAddSlots( sdrMtx        * aMtx,
                                 sdpstBMPHdr   * aBMPHdr,
                                 scPageID        aFstBMP,
                                 scPageID        aLstBMP,
                                 UShort          aFullBMPs,
                                 idBool        * aNeedToChangeMFNL,
                                 sdpstMFNL     * aNewMFNL )
{
    SShort              sLstSlotNo;
    sdpstRedoAddSlots   sLogData;

    IDE_DASSERT( aMtx          != NULL );
    IDE_DASSERT( aBMPHdr       != NULL );
    IDE_DASSERT( aNeedToChangeMFNL != NULL );
    IDE_DASSERT( aNewMFNL      != NULL );

    // 페이지 내의 slot 개수를 얻어서 다음 기록할 slot위치를 구한다.
    sLstSlotNo = aBMPHdr->mSlotCnt;

    if ( aBMPHdr->mMaxSlotCnt <= sLstSlotNo )
    {
        ideLog::log( IDE_SERVER_0,
                     "MaxSlotCnt: %u, "
                     "LstSlotNo : %d\n",
                     aBMPHdr->mMaxSlotCnt,
                     sLstSlotNo );
        (void)dump( sdpPhyPage::getPageStartPtr( aBMPHdr ) );
        IDE_ASSERT( 0 );
    }


    addSlots( aBMPHdr, sLstSlotNo, aFstBMP, aLstBMP, aFullBMPs,
              aNeedToChangeMFNL, aNewMFNL );

    sLogData.mLstSlotNo   = sLstSlotNo;
    sLogData.mFstChildBMP = aFstBMP;
    sLogData.mLstChildBMP = aLstBMP;
    sLogData.mFullBMPCnt  = aFullBMPs;

    // ADD_SLOT logging
    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aBMPHdr,
                                         &sLogData,
                                         ID_SIZEOF( sLogData ),
                                         SDR_SDPST_ADD_SLOTS ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Bitmap 페이지의 map 영역에 slot을 추가한다.
 ***********************************************************************/
void sdpstBMP::addSlotsToMap( sdpstBMPSlot    * aMapPtr,
                              SShort            aSlotNo,
                              scPageID          aFstBMP,
                              scPageID          aLstBMP,
                              UShort            aFullBMPs )
{
    UInt             sLoop;
    sdpstBMPSlot   * sSlot;
    scPageID         sCurrBMP;
    UShort           sFullBMPs;

    IDE_ASSERT( aMapPtr != NULL );

    sFullBMPs = aFullBMPs;

    for( sLoop = 0, sCurrBMP = aFstBMP;
         sCurrBMP <= aLstBMP;
         sLoop++, sCurrBMP++ )
    {
        sSlot = &(aMapPtr[ aSlotNo + sLoop ]);

        sSlot->mBMP = sCurrBMP;
        if ( sFullBMPs >  0 )
        {
            sSlot->mMFNL  = SDPST_MFNL_FUL;
            sFullBMPs--;
        }
        else
        {
            sSlot->mMFNL  = SDPST_MFNL_FMT;
        }
    }
}

/***********************************************************************
 * Description : 페이지의 MFNL 개수 및 BMP 대표 MFNL 결정하여 갱신한다.
 ***********************************************************************/
IDE_RC sdpstBMP::logAndUpdateMFNL( sdrMtx            * aMtx,
                                   sdpstBMPHdr       * aBMPHdr,
                                   SShort              aFmSlotNo,
                                   SShort              aToSlotNo,
                                   sdpstMFNL           aOldSlotMFNL,
                                   sdpstMFNL           aNewSlotMFNL,
                                   UShort              aPageCount,
                                   sdpstMFNL         * aNewBMPMFNL,
                                   idBool            * aNeedToChangeMFNL )
{
    sdpstRedoUpdateMFNL sLogData;

    updateMFNL( aBMPHdr,
                aFmSlotNo,
                aToSlotNo,
                aOldSlotMFNL,
                aNewSlotMFNL,
                aPageCount,
                aNewBMPMFNL,
                aNeedToChangeMFNL );

    sLogData.mFmMFNL        = aOldSlotMFNL;
    sLogData.mToMFNL        = aNewSlotMFNL;
    sLogData.mPageCnt       = aPageCount;
    sLogData.mFmSlotNo      = aFmSlotNo;
    sLogData.mToSlotNo      = aToSlotNo;

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aBMPHdr,
                                         &sLogData,
                                         ID_SIZEOF(sLogData),
                                         SDR_SDPST_UPDATE_MFNL )
            != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}



/***********************************************************************
 * Description : 페이지의 MFNL 개수 및 BMP 대표 MFNL 결정하여 갱신한다.
 *
 * aBMPHdr           - [IN] BMP Header
 * aFmSlotNo         - [IN] From SlotNo
 * aToSlotNo         - [IN] To SlotNo
 * aOldSlotMFNL      - [IN] 변경전 MFNL
 * aNewSlotMFNL      - [IN] 변경후 MFNL
 * aPageCount        - [IN] 변경할 CHILD 페이지 개수
 * aNewBMPMFNL       - [OUT] 변경된 해당 BMP의 MFNL
 * aNeedToChangeMFNL - [OUT] MFNL 변경 여부
 ***********************************************************************/
void sdpstBMP::updateMFNL( sdpstBMPHdr       * aBMPHdr,
                           SShort              aFmSlotNo,
                           SShort              aToSlotNo,
                           sdpstMFNL           aOldSlotMFNL,
                           sdpstMFNL           aNewSlotMFNL,
                           UShort              aPageCount,
                           sdpstMFNL         * aNewBMPMFNL,
                           idBool            * aNeedToChangeMFNL )
{
    sdpstMFNL       sNewBMPMFNL;
    sdpstBMPSlot  * sSlotPtr;
    UShort          sLoop;

    IDE_DASSERT( aBMPHdr   != NULL );
    IDE_DASSERT( aPageCount > 0 );

    if ( aBMPHdr->mMFNLTbl[aOldSlotMFNL] < aPageCount )
    {
        ideLog::log( IDE_SERVER_0,
                     "MFNL Table[UNF]: %u, "
                     "MFNL Table[FMT]: %u, "
                     "MFNL Table[INS]: %u, "
                     "MFNL Table[FUL]: %u, "
                     "Page Count     : %u\n",
                     aBMPHdr->mMFNLTbl[SDPST_MFNL_UNF],
                     aBMPHdr->mMFNLTbl[SDPST_MFNL_FMT],
                     aBMPHdr->mMFNLTbl[SDPST_MFNL_INS],
                     aBMPHdr->mMFNLTbl[SDPST_MFNL_FUL],
                     aPageCount );
        (void)dump( sdpPhyPage::getPageStartPtr( aBMPHdr ) );
        IDE_ASSERT( 0 );
    }

    /* Slot MFNL 을 갱신한다. 단, Lf-BMP에서는 갱신하지 않는다. (없기때문) */
    if ( (aBMPHdr->mType == SDPST_RTBMP) || (aBMPHdr->mType == SDPST_ITBMP) )
    {
        for ( sLoop = aFmSlotNo; sLoop <= aToSlotNo; sLoop++ )
        {
            sSlotPtr = getSlot( aBMPHdr, sLoop );
            sSlotPtr->mMFNL = aNewSlotMFNL;
        }
    }

    /* MFNL Table 을 갱신한다. */
    aBMPHdr->mMFNLTbl[aOldSlotMFNL] -= aPageCount;
    aBMPHdr->mMFNLTbl[aNewSlotMFNL] += aPageCount;

    /* BMP의 대표 MFNL을 결정하여 갱신한다. */
    sNewBMPMFNL = sdpstAllocPage::calcMFNL( aBMPHdr->mMFNLTbl );
    if ( aBMPHdr->mMFNL != sNewBMPMFNL )
    {
        aBMPHdr->mMFNL = sNewBMPMFNL;
        *aNeedToChangeMFNL = ID_TRUE;
    }
    else
    {
        *aNeedToChangeMFNL = ID_FALSE;
    }

    *aNewBMPMFNL = sNewBMPMFNL;
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
IDE_RC sdpstBMP::tryToChangeMFNL( idvSQL          * aStatistics,
                                  sdrMtx          * aMtx,
                                  scSpaceID         aSpaceID,
                                  scPageID          aCurBMP,
                                  SShort            aSlotNo,
                                  sdpstMFNL         aNewSlotMFNL,
                                  idBool          * aNeedToChangeMFNL,
                                  sdpstMFNL       * aNewMFNL,
                                  scPageID        * aParentBMP,
                                  SShort          * aSlotNoInParent )
{
    UChar               * sPagePtr;
    sdpstBMPHdr         * sBMPHdr;
    sdpstBMPSlot        * sSlotPtr;

    IDE_DASSERT( aMtx            != NULL );
    IDE_DASSERT( aCurBMP         != SD_NULL_PID );
    IDE_DASSERT( aSlotNo         != SDPST_INVALID_SLOTNO );
    IDE_DASSERT( aNeedToChangeMFNL   != NULL );
    IDE_DASSERT( aNewMFNL        != NULL );
    IDE_DASSERT( aParentBMP      != NULL );
    IDE_DASSERT( aSlotNoInParent != NULL );
   
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aCurBMP,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
              != IDE_SUCCESS );

    sBMPHdr = getHdrPtr( sPagePtr );

    sSlotPtr = getSlot( sBMPHdr, aSlotNo );

    IDE_TEST( logAndUpdateMFNL( aMtx,
                                sBMPHdr,
                                aSlotNo,
                                aSlotNo,
                                sSlotPtr->mMFNL, /* old slot MFNL */
                                aNewSlotMFNL,
                                1,
                                aNewMFNL,
                                aNeedToChangeMFNL ) != IDE_SUCCESS );

    IDE_DASSERT( sdpstBMP::verifyBMP( sBMPHdr ) == IDE_SUCCESS );

    *aParentBMP      = sBMPHdr->mParentInfo.mParentPID;
    *aSlotNoInParent = sBMPHdr->mParentInfo.mIdxInParent;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC sdpstBMP::verifyBMP( sdpstBMPHdr  * aBMPHdr )
{
    UShort             sLoop;
    UShort             sSlotCnt;
    sdpstMFNL          sMFNL;
    UShort             sMFNLtbl[ SDPST_MFNL_MAX ];
    sdpstBMPSlot     * sSlotPtr;

    sSlotCnt       = aBMPHdr->mSlotCnt;
    sSlotPtr       = getMapPtr(aBMPHdr);

    idlOS::memset( &sMFNLtbl, 0x00, ID_SIZEOF(UShort) * SDPST_MFNL_MAX );

    /* MFNL Table 값 검증 */
    for( sLoop = 0; sLoop < sSlotCnt; sLoop++ )
    {
        if ( sSlotPtr[sLoop].mBMP == SD_NULL_PID )
        {
            ideLog::log( IDE_SERVER_0,
                         "sLoop: %u\n",
                         sLoop );
            (void)dump( sdpPhyPage::getPageStartPtr( aBMPHdr ) );
            IDE_ASSERT( 0 );
        }

        sMFNL            = sSlotPtr[sLoop].mMFNL;
        sMFNLtbl[sMFNL] += 1;
    }

    for ( sLoop = 0; sLoop < SDPST_MFNL_MAX; sLoop++ )
    {
        if ( aBMPHdr->mMFNLTbl[ sLoop ] != sMFNLtbl[ sLoop ] )
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
                         aBMPHdr->mMFNLTbl[SDPST_MFNL_FUL],
                         aBMPHdr->mMFNLTbl[SDPST_MFNL_INS],
                         aBMPHdr->mMFNLTbl[SDPST_MFNL_FMT],
                         aBMPHdr->mMFNLTbl[SDPST_MFNL_UNF],
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
 * Description : Search Type에 따라 탐색할 Lf BMP 페이지의 후보목록을
 *               작성한다.
 *
 * 가용 Slot을 탐색대상을 탐색할 수도 있고, 가용 페이지를 탐색대상으로
 * 탐색할수도 있다.
 ***********************************************************************/
void sdpstBMP::makeCandidateChild( sdpstSegCache    * aSegCache,
                                   UChar            * aPagePtr,
                                   sdpstBMPType       aBMPType,
                                   sdpstSearchType    aSearchType,
                                   sdpstWM          * aHWM,
                                   void             * aCandidateArray,
                                   UInt             * aCandidateCount )
{
    sdpstBMPHdr       * sBMPHdr;
    UShort              sMaxCandidateCount;
    UShort              sCandidateCount;
    UShort              sSlotNo;
    SShort              sManualSlotNo = -1;
    sdpstMFNL           sTargetMFNL;
    SShort              sCurIdx;
    SShort              sLoop;
    SShort              sSlotCnt;
    UShort              sHintIdx;
    sdpstBMPOps       * sCurOps;

    IDE_DASSERT( aSegCache       != NULL );
    IDE_DASSERT( aPagePtr        != NULL );
    IDE_DASSERT( aCandidateArray != NULL );
    IDE_DASSERT( aBMPType        == SDPST_ITBMP ||
                 aBMPType        == SDPST_LFBMP );

    sCurOps         = mBMPOps[aBMPType];
    sTargetMFNL     = SDPST_SEARCH_TARGET_MIN_MFNL( aSearchType );
    sCandidateCount = 0;

    sBMPHdr = sCurOps->mGetBMPHdr( aPagePtr );

    if ( sdpstFindPage::isAvailable( sBMPHdr->mMFNL, 
                                     sTargetMFNL ) == ID_FALSE )
    {
        IDE_CONT( bmp_not_available );
    }

    sMaxCandidateCount = sCurOps->mGetMaxCandidateCnt();

    // 탐색 시작위치를 Hashing 해서 선택한다음, 후보 목록을
    // 최대 10개까지 작성한다.
    // (어차피 힌트이기 때문에 Race Condition이 발생해도 상관없다)
    sHintIdx = aSegCache->mHint4CandidateChild++;

    /* BUG-33683 - [SM] in sdcRow::processOverflowData, Deadlock can occur
     * 특정 페이지 할당을 유도하기 위해 프로퍼티 설정된 경우 해당 프로퍼티
     * 값으로 sSlotNo를 설정한다. */
    if ( aBMPType == SDPST_LFBMP )
    {
        sManualSlotNo = smuProperty::getTmsManualSlotNoInLfBMP();
    }
    else
    {
        if ( aBMPType == SDPST_ITBMP )
        {
            sManualSlotNo = smuProperty::getTmsManualSlotNoInItBMP();
        }
        else
        {
            /* nothing */
        }
    }

    if ( sManualSlotNo < 0 )
    {
        sSlotNo  = sCurOps->mGetStartSlotOrPBSNo( aPagePtr, sHintIdx );
        sSlotCnt = sCurOps->mGetChildCount( aPagePtr, aHWM );
    }
    else
    {
        sSlotNo  = (UShort)sManualSlotNo;
        sSlotCnt = sCurOps->mGetChildCount( aPagePtr, aHWM );
    }

    /* 마지막을 넘어서면 마지막 slot 부터 */
    if ( sSlotNo > (sSlotCnt - 1) )
    {
        sSlotNo = sSlotCnt - 1;
    }

    for ( sLoop = 0, sCurIdx = sSlotNo; sLoop < sSlotCnt; sLoop++ )
    {
        if ( sCurIdx == SDPST_INVALID_SLOTNO )
        {
            ideLog::log( IDE_SERVER_0,
                         "sSlotNo: %u, "
                         "sSlotCnt: %d, "
                         "sHintIdx: %u, "
                         "sLoop: %d\n",
                         sSlotNo,
                         sSlotCnt,
                         sHintIdx,
                         sLoop );

            /* BMP Page dump */
            if ( aBMPType == SDPST_ITBMP )
            {
                (void)dump( aPagePtr );
            }
            else
            {
                if ( aBMPType == SDPST_LFBMP )
                {
                    sdpstLfBMP::dump( aPagePtr );
                }
                else
                {
                    ideLog::logMem( IDE_SERVER_0, aPagePtr, SD_PAGE_SIZE );
                }
            }

            /* SegCache dump */
            sdpstCache::dump( aSegCache );

            IDE_ASSERT( 0 );
        }

        if ( sCurOps->mIsCandidateChild( aPagePtr,
                                         sCurIdx,
                                         sTargetMFNL ) == ID_TRUE )
        {
            sCurOps->mSetCandidateArray( aPagePtr,
                                         sCurIdx,
                                         sCandidateCount,
                                         aCandidateArray );
            sCandidateCount++;
        }

        /* 모든 후보를 선택할때까지 */
        if ( sCandidateCount > sMaxCandidateCount - 1)
        {
            break;
        }

        /* 모든 slot을 확인 해야하므로 마지막 slot까지
         * 확인했다면 다시 첫번째 slot부터 확인한다. */
        if ( sCurIdx == (sSlotCnt - 1) )
        {
            sCurIdx = 0;
        }
        else
        {
            if ( sCurIdx >= (sSlotCnt - 1) )
            {
                ideLog::log( IDE_SERVER_0,
                             "sSlotNo: %u, "
                             "sSlotCnt: %d, "
                             "sHintIdx: %u, "
                             "sLoop: %d\n",
                             sSlotNo,
                             sSlotCnt,
                             sHintIdx,
                             sLoop );

                /* BMP Page dump */
                if ( aBMPType == SDPST_ITBMP )
                {
                    (void)dump( aPagePtr );
                }
                else
                {
                    if ( aBMPType == SDPST_LFBMP )
                    {
                        sdpstLfBMP::dump( aPagePtr );
                    }
                    else
                    {
                        ideLog::logMem( IDE_SERVER_0, aPagePtr, SD_PAGE_SIZE );
                    }
                }

                /* SegCache dump */
                sdpstCache::dump( aSegCache );

                IDE_ASSERT( 0 );
            }
            sCurIdx++;
        }
    }

    IDE_EXCEPTION_CONT( bmp_not_available );

    *aCandidateCount = sCandidateCount;
}

/***********************************************************************
 * Description : RtBMP/ItBMP Header를 dump한다.
 ***********************************************************************/
IDE_RC sdpstBMP::dumpHdr( UChar    * aPagePtr,
                          SChar    * aOutBuf,
                          UInt       aOutSize )
{
    UChar         * sPagePtr;
    sdpstBMPHdr   * sBMPHdr;

    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aOutBuf  != NULL );
    IDE_ERROR( aOutSize > 0 );

    sPagePtr = sdpPhyPage::getPageStartPtr( aPagePtr );
    sBMPHdr  = getHdrPtr( sPagePtr );

    /* BMP Header */
    idlOS::snprintf(
                 aOutBuf,
                 aOutSize,
                 "--------------- BMP Header Begin ----------------\n"
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
                 "---------------  BMP Header End  ----------------\n",
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
                 sBMPHdr->mBodyOffset );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : RtBMP/ItBMP Body를 dump한다.
 ***********************************************************************/
IDE_RC sdpstBMP::dumpBody( UChar    * aPagePtr,
                           SChar    * aOutBuf,
                           UInt       aOutSize )
{
    UChar         * sPagePtr;
    sdpstBMPHdr   * sBMPHdr;
    sdpstBMPSlot  * sSlotPtr;
    SShort          sSlotNo;

    IDE_ERROR( aPagePtr != NULL );

    sPagePtr = sdpPhyPage::getPageStartPtr( aPagePtr );
    sBMPHdr  = getHdrPtr( sPagePtr );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "--------------- BMP Body Begin ----------------\n" );

    /* BMP Body */
    for ( sSlotNo = 0; sSlotNo < sBMPHdr->mSlotCnt; sSlotNo++ )
    {
        sSlotPtr = getSlot( sBMPHdr, sSlotNo );

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "[%"ID_INT32_FMT"] "
                             "BMP: %"ID_UINT32_FMT", "
                             "MFNL: %"ID_UINT32_FMT"\n",
                             sSlotNo,
                             sSlotPtr->mBMP,
                             sSlotPtr->mMFNL );
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "---------------- BMP Body End -----------------\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : RtBMP/ItBMP Header를 dump한다.
 ***********************************************************************/
IDE_RC sdpstBMP::dump( UChar    * aPagePtr )
{
    UChar         * sPagePtr;
    SChar         * sDumpBuf;

    IDE_ERROR( aPagePtr != NULL );

    sPagePtr = sdpPhyPage::getPageStartPtr( aPagePtr );

    ideLog::log( IDE_SERVER_0,
                 "========================================================\n" );

    /* Physical Page */
    (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                 sPagePtr,
                                 "Physical Page:" );

    if( iduMemMgr::calloc(
            IDU_MEM_SM_SDP, 1,
            ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
            (void**)&sDumpBuf ) == IDE_SUCCESS )
    {
        /* BMP Header */
        if( dumpHdr( sPagePtr,
                     sDumpBuf,
                     IDE_DUMP_DEST_LIMIT ) == IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "%s", sDumpBuf );
        }

        if( dumpBody( sPagePtr,
                      sDumpBuf,
                      IDE_DUMP_DEST_LIMIT ) == IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "%s", sDumpBuf );
        }

        (void)iduMemMgr::free( sDumpBuf );
    }

    ideLog::log( IDE_SERVER_0,
                 "========================================================\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
