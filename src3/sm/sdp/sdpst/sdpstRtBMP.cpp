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
# include <sdpstLfBMP.h>
# include <sdpstItBMP.h>
# include <sdpstRtBMP.h>
# include <sdpstAllocPage.h>
# include <sdpstSH.h>
# include <sdpstStackMgr.h>
# include <sdpstCache.h>
# include <sdpstDPath.h>

sdpstBMPOps sdpstRtBMP::mVirtBMPOps =
{
    (sdpstGetBMPHdrFunc)NULL,
    (sdpstGetSlotOrPageCntFunc)NULL,
    (sdpstGetDistInDepthFunc)sdpstStackMgr::getDistInVtDepth,
    (sdpstIsCandidateChildFunc)NULL,
    (sdpstGetStartSlotOrPBSNoFunc)NULL,
    (sdpstGetChildCountFunc)NULL,
    (sdpstSetCandidateArrayFunc)NULL,
    (sdpstGetFstFreeSlotOrPBSNoFunc)NULL,
    (sdpstGetMaxCandidateCntFunc)NULL,
    (sdpstUpdateBMPUntilHWMFunc)sdpstDPath::updateBMPUntilHWMInVtBMP,
};

sdpstBMPOps sdpstRtBMP::mRtBMPOps =
{
    (sdpstGetBMPHdrFunc)sdpstBMP::getHdrPtr,
    (sdpstGetSlotOrPageCntFunc)NULL,
    (sdpstGetDistInDepthFunc)sdpstStackMgr::getDistInRtDepth,
    (sdpstIsCandidateChildFunc)NULL,
    (sdpstGetStartSlotOrPBSNoFunc)NULL,
    (sdpstGetChildCountFunc)NULL,
    (sdpstSetCandidateArrayFunc)NULL,
    (sdpstGetFstFreeSlotOrPBSNoFunc)NULL,
    (sdpstGetMaxCandidateCntFunc)NULL,
    (sdpstUpdateBMPUntilHWMFunc)sdpstDPath::updateBMPUntilHWMInRtAndItBMP,
};

/***********************************************************************
 * Description : Internal Hint를 재탐색한다.
 *
 * 서버구동시에 Segment Descriptor의 초기화 과정이나 가용공간 탐새과정
 * 에서 SegCache에 UpdateItHint flag가 TRUE인 경우에 기존 Internal Hint
 * 를 재설정하게 과정을 수행한다.
 *
 * aStatistics    - [IN]  통게정보
 * aSpaceID       - [IN]  테이블스페이스의 ID
 * aSegPID        - [IN]  Segment RID
 * aSearchType    - [IN]  Search Type
 * aSegCache      - [IN]  Segment Cache
 * aNewHintStack  - [OUT] 새로운 Hint Stack
 ***********************************************************************/
IDE_RC sdpstRtBMP::rescanItHint( idvSQL          * aStatistics,
                                 scSpaceID         aSpaceID,
                                 scPageID          aSegPID,
                                 sdpstSearchType   aSearchType,
                                 sdpstSegCache   * aSegCache,
                                 sdpstStack      * aStack )
{
    scPageID             sCurRtBMP;
    idBool               sHintFlag;
    SShort               sRtBMPIdx;
    SShort               sCurIdx;
    sdpstStack           sNewStack4Hint;
    idBool               sDummy;

    IDE_ASSERT( aSegPID        != SD_NULL_PID );
    IDE_ASSERT( aSegCache      != NULL );

    /* root부터 처리한다. */
    sCurRtBMP = aSegPID;
    sCurIdx   = 0;
    sRtBMPIdx = 0;

    /* RtBMP를 따라가면서 search type을 만족하는 ItBMP를 찾아 반환한다. */
    IDE_TEST( findFstFreeSlot( aStatistics,
                               aSpaceID,
                               NULL,
                               sCurRtBMP,
                               sCurIdx,
                               aSearchType,
                               sRtBMPIdx,
                               aSegCache,
                               &sNewStack4Hint,
                               &sDummy ) != IDE_SUCCESS );

    /* 새로운 Internal Hint Stack을 비교해보고 설정한다. */
    sHintFlag = ID_FALSE;
    sdpstCache::setItHintIfLT( aStatistics,
                               aSegCache,
                               aSearchType,
                               &sNewStack4Hint,
                               &sHintFlag );

    if ( aStack != NULL )
    {
        *aStack = sNewStack4Hint;
        if ( sdpstStackMgr::getDepth( aStack ) != SDPST_ITBMP )
        {
            sdpstStackMgr::dump( &sNewStack4Hint );
            sdpstCache::dump( aSegCache );
            IDE_ASSERT( 0 );
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : rt-bmp의 이전 itbmp에서부터 다음 가용공간이 존재하는
 *               itbmp를 반환한다.
 ***********************************************************************/
void sdpstRtBMP::findFreeItBMP( sdpstBMPHdr       * aBMPHdr,
                                SShort              aSlotNoInParent,
                                sdpstMFNL           aTargetMFNL,
                                sdpstWM           * aHWM,
                                scPageID          * aNxtItBMP,
                                SShort            * aFreeSlotNo )
{
    scPageID          sNxtItBMP;
    SShort            sFreeSlotNo;
    SShort            sCurSlotNo;
    SShort            sSlotCnt;
    scPageID          sCurPID;
    sdpstPosItem      sHWMPos;
    sdpstBMPSlot    * sSlotPtr;


    IDE_DASSERT( aBMPHdr   != NULL );
    IDE_DASSERT( aNxtItBMP != NULL || aFreeSlotNo != NULL );

    sNxtItBMP   = SD_NULL_PID;
    sFreeSlotNo = SDPST_INVALID_SLOTNO;

    /* HWM과 동일 RtBMP이면, Free ItBMP 탐색은 HWM 까지만 한다. */
    sHWMPos = sdpstStackMgr::getSeekPos( &aHWM->mStack, SDPST_RTBMP );
    sCurPID = sdpPhyPage::getPageID( sdpPhyPage::getPageStartPtr(aBMPHdr) );

    if ( sHWMPos.mNodePID == sCurPID )
    {
        if ( sHWMPos.mIndex + 1 < aBMPHdr->mSlotCnt )
        {
            sSlotCnt = sHWMPos.mIndex + 1;
        }
        else
        {
            sSlotCnt = aBMPHdr->mSlotCnt;
        }
    }
    else
    {
        sSlotCnt = aBMPHdr->mSlotCnt;
    }


    for ( sCurSlotNo = aSlotNoInParent; sCurSlotNo < sSlotCnt; sCurSlotNo++ )
    {
        sSlotPtr = sdpstBMP::getSlot( aBMPHdr, sCurSlotNo );
        if ( sdpstFindPage::isAvailable( sSlotPtr->mMFNL,
                                         aTargetMFNL ) == ID_TRUE )
        {
            sFreeSlotNo = sCurSlotNo;
            break;
        }
    }

    if ( sFreeSlotNo != SDPST_INVALID_SLOTNO )
    {
        // 가용한 leaf bmp 페이지를 탐색한 경우
        sNxtItBMP = sdpstBMP::getSlot( aBMPHdr, sFreeSlotNo )->mBMP;
    }
    else
    {
        sNxtItBMP = SD_NULL_PID; // Not Found !!
    }

    if ( aNxtItBMP != NULL )
    {
        *aNxtItBMP    = sNxtItBMP;
    }

    if ( aFreeSlotNo != NULL )
    {
        *aFreeSlotNo = sFreeSlotNo;
    }
}

/***********************************************************************
 * Description : RtBMP 페이지들을 탐색하면서 첫번째로 TargetMFNL을
 *               만족하는 Internal Slot을 찾는다.
 ***********************************************************************/
IDE_RC sdpstRtBMP::findFstFreeSlot( idvSQL             * aStatistics,
                                    scSpaceID            aSpaceID,
                                    sdpstBMPHdr        * aBMPHdr,
                                    scPageID             aCurRtBMP,
                                    SShort               aCurSlotNo,
                                    sdpstSearchType      aSearchType,
                                    SShort               aSlotNoOfRtBMP,
                                    sdpstSegCache      * aSegCache,
                                    sdpstStack         * aStack,
                                    idBool             * aIsFound )
{
    SShort              sCurSlotNo;
    SShort              sSlotNoOfRtBMP;
    scPageID            sCurItBMP;
    scPageID            sCurRtBMP;
    sdpstMFNL           sTargetMFNL;
    UChar             * sPagePtr;
    sdpstBMPHdr       * sBMPHdr;
    sdpstWM             sHWM;
    UInt                sState = 0;

    IDE_ASSERT( aStack    != NULL );

    sTargetMFNL = SDPST_SEARCH_TARGET_MIN_MFNL( aSearchType );

    sSlotNoOfRtBMP = aSlotNoOfRtBMP;
    sCurSlotNo     = aCurSlotNo;
    sCurRtBMP      = aCurRtBMP;
    sCurItBMP      = SD_NULL_PID;
    sBMPHdr        = aBMPHdr;

    sdpstCache::copyHWM( aStatistics, aSegCache, &sHWM );

    while( 1 )
    {
        if ( sBMPHdr == NULL )
        {
            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                  aSpaceID,
                                                  sCurRtBMP,
                                                  SDB_S_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  NULL, /* sdrMtx */
                                                  &sPagePtr,
                                                  NULL, /* aTrySuccess */
                                                  NULL  /* aIsCorruptPage */ )
                      != IDE_SUCCESS );
            sState = 1;

            sBMPHdr = sdpstBMP::getHdrPtr( sPagePtr );
        }

        findFreeItBMP( sBMPHdr,
                       sCurSlotNo,
                       sTargetMFNL,
                       &sHWM,
                       NULL,
                       &sCurSlotNo );

        if ( sCurSlotNo == SDPST_INVALID_SLOTNO )
        {
            if ( getNxtRtBMP(sBMPHdr) == SD_NULL_PID )
            {
                // 끝이기 때문에 마지막으로 Hint를 설정한다.
                sCurSlotNo   = sBMPHdr->mSlotCnt - 1;
                sCurItBMP = (sdpstBMP::getMapPtr(sBMPHdr) + sCurSlotNo)->mBMP;
                *aIsFound  = ID_FALSE;
                break;
            }
            else
            {
                sCurSlotNo = 0;  // Direct Path Insert Merge
            }
            sSlotNoOfRtBMP++;
            sCurRtBMP = getNxtRtBMP(sBMPHdr);
        }
        else
        {
            // 찾았으므로 Hint를 설정한다.
            sCurItBMP = (sdpstBMP::getMapPtr(sBMPHdr) + sCurSlotNo)->mBMP;
            *aIsFound  = ID_TRUE;
            break;
        }

        if ( sState == 1 )
        {
            sState = 0;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sBMPHdr )
                      != IDE_SUCCESS );
        }
        sBMPHdr = NULL;
    }

    if ( sState == 1 )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sBMPHdr )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( sCurItBMP != SD_NULL_PID );
    IDE_DASSERT( sCurRtBMP != SD_NULL_PID );
    IDE_DASSERT( sSlotNoOfRtBMP != SDPST_INVALID_SLOTNO );
    IDE_DASSERT( sCurSlotNo   != SDPST_INVALID_SLOTNO );

    sdpstStackMgr::initialize( aStack );
    sdpstStackMgr::push( aStack, SD_NULL_PID, sSlotNoOfRtBMP );
    sdpstStackMgr::push( aStack, sCurRtBMP, sCurSlotNo );
    sdpstStackMgr::push( aStack, sCurItBMP, 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics, (UChar*)sBMPHdr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Internal Slot의 MFNL이 Full로 변경된 경우에는 Internal
 *               Hint의 변경여부를 검사하고 전진시킨다.
 ***********************************************************************/
IDE_RC sdpstRtBMP::forwardItHint( idvSQL           * aStatistics,
                                  sdrMtx           * aMtx,
                                  scSpaceID          aSpaceID,
                                  sdpstSegCache    * aSegCache,
                                  sdpstStack       * aRevStack,
                                  sdpstSearchType    aSearchType )
{
    idBool              sIsFound;
    UChar             * sPagePtr;
    SShort              sCurIdx;
    SShort              sRtBMPIdx;
    sdpstStack          sStack4Hint;
    sdpstPosItem        sCurPos;
    sdpstPosItem        sCmpPos;
    sdpstBMPHdr       * sBMPHdr;
    UInt                sState = 0;

    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( aRevStack != NULL );
    IDE_ASSERT( aMtx      != NULL );

    /*
     * 기존 Internal Hint 가 full로 변경된 경우이므로
     * 다음 Internal Hint를 찾아서 전진시킨다.
     */
    sdpstCache::copyItHint( aStatistics,
                            aSegCache,
                            aSearchType,
                            &sStack4Hint );

    if ( (sdpstStackMgr::getDepth( &sStack4Hint ) != SDPST_ITBMP) ||
         (sdpstStackMgr::getDepth( aRevStack )    != SDPST_RTBMP) )
    {
        ideLog::log( IDE_SERVER_0, "======== Stack4Hint Dump ========\n" );
        sdpstStackMgr::dump( &sStack4Hint );
        ideLog::log( IDE_SERVER_0, "======== RevStack Dump ========\n" );
        sdpstStackMgr::dump( aRevStack );
        IDE_ASSERT( 0 );
    }


    /* RevStack에서 Virtual에 저장된 것이 ItBMP이다. */
    sCmpPos = sdpstStackMgr::getSeekPos( aRevStack, SDPST_RTBMP );
    sCurPos = sdpstStackMgr::getSeekPos( &sStack4Hint, SDPST_RTBMP );

    /* forwardItHint는 다시 RootBMP에서 ItBMP가 변경된
     * 경우에만 들어 오며, Root Depth에서 동일한 lfbmp 페이지가
     * FULL로 변경된 경우에만 새로운 It Hint를 설정한다. */
    if ( sdpstStackMgr::getDist( &sCmpPos, &sCurPos ) != SDPST_FAR_AWAY_OFF )
    {
        sCurPos = sdpstStackMgr::getSeekPos( &sStack4Hint, SDPST_VIRTBMP );
        sRtBMPIdx   = sCurPos.mIndex;

        sCurPos     = sdpstStackMgr::getSeekPos( aRevStack, SDPST_RTBMP );

        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                              aSpaceID,
                                              sCurPos.mNodePID,
                                              &sPagePtr ) != IDE_SUCCESS );
        sState = 1;

        sBMPHdr = sdpstBMP::getHdrPtr( sPagePtr );

        // root bitmap 페이지를 따라서 search type을 만족하는
        // internal bitmap 페이지를 차아 반환한다.
        sCurIdx = sCurPos.mIndex; // 탐색시작위치
        IDE_TEST( findFstFreeSlot( aStatistics,
                                   aSpaceID,
                                   sBMPHdr,
                                   sCurPos.mNodePID,
                                   sCurIdx,
                                   aSearchType,
                                   sRtBMPIdx,
                                   aSegCache,
                                   &sStack4Hint,
                                   &sIsFound ) != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                  != IDE_SUCCESS );

        if ( sIsFound == ID_TRUE )
        {
            // 새로운 Internal Hint Stack을 비교해보고 설정한다.
            sdpstCache::setItHintIfGT( aStatistics,
                                       aSegCache,
                                       aSearchType,
                                       &sStack4Hint );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment에 Next Root BMP를 설정한다.
 ***********************************************************************/
IDE_RC sdpstRtBMP::setNxtRtBMP( idvSQL          * aStatistics,
                                sdrMtx          * aMtx,
                                scSpaceID         aSpaceID,
                                scPageID          aLstRtBMP,
                                scPageID          aNewRtBMP )
{
    UChar       * sPagePtr;
    sdpstBMPHdr * sRtBMPHdr;

    IDE_ASSERT( aMtx      != NULL );
    IDE_ASSERT( aLstRtBMP != SD_NULL_PID );
    IDE_ASSERT( aNewRtBMP != SD_NULL_PID );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aLstRtBMP,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          NULL, /* aTrySuccess */
                                          NULL  /* aIsCorruptPage */ )
          != IDE_SUCCESS );

    sRtBMPHdr = sdpstBMP::getHdrPtr( sPagePtr );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(sRtBMPHdr->mNxtRtBMP),
                                         &aNewRtBMP,
                                         ID_SIZEOF(scPageID) ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

