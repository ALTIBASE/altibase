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

# include <sdpstSH.h>
# include <sdptbExtent.h>
# include <sdpstAllocExtInfo.h>

/***********************************************************************
 * Description : sdpstBfrAllocExtInfo 구조체를 초기화한다.
 *
 * aBfrInfo     - [IN] Extent 할당 알고리즘에서 사용하는 이전 할당 정보
 ***********************************************************************/
IDE_RC sdpstAllocExtInfo::initialize( sdpstBfrAllocExtInfo *aBfrInfo )
{
    UInt    sLoop;

    idlOS::memset( aBfrInfo, 0x00, ID_SIZEOF(sdpstBfrAllocExtInfo));

    for ( sLoop = SDPST_EXTDIR; sLoop < SDPST_BMP_TYPE_MAX; sLoop++ )
    {
        aBfrInfo->mLstPID[sLoop]      = SD_NULL_PID;
        aBfrInfo->mFreeSlotCnt[sLoop] = SD_NULL_PID;
        aBfrInfo->mMaxSlotCnt[sLoop]  = SD_NULL_PID;
    }

    aBfrInfo->mTotPageCnt     = 0;
    aBfrInfo->mPageRange      = SDPST_PAGE_RANGE_NULL;
    aBfrInfo->mSlotNoInExtDir = -1;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : sdpstAftAllocExtInfo 구조체를 초기화한다.
 *
 * aAftInfo     - [IN] Extent 할당 알고리즘에서 사용하는 이후 할당 정보
 ***********************************************************************/
IDE_RC sdpstAllocExtInfo::initialize( sdpstAftAllocExtInfo *aAftInfo )
{
    UInt    sLoop;

    idlOS::memset( aAftInfo, 0x00, ID_SIZEOF(sdpstAftAllocExtInfo));

    for ( sLoop = SDPST_EXTDIR; sLoop < SDPST_BMP_TYPE_MAX; sLoop++ )
    {
        aAftInfo->mFstPID[sLoop]      = SD_NULL_PID;
        aAftInfo->mLstPID[sLoop]      = SD_NULL_PID;
        aAftInfo->mPageCnt[sLoop]     = SD_NULL_PID;
        aAftInfo->mFullBMPCnt[sLoop]  = SD_NULL_PID;
    }

    aAftInfo->mTotPageCnt     = 0;
    aAftInfo->mLfBMP4ExtDir   = SD_NULL_PID;
    aAftInfo->mPageRange      = SDPST_PAGE_RANGE_NULL;
    aAftInfo->mSegHdrCnt      = 0;  /* 0, 1값만 갖는다. */
    aAftInfo->mSlotNoInExtDir = -1;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Segment Header의 last bmp 페이지들의 정보를 수집한다.
 *
 * Segment에 Extent 할당은 트랜잭션에 의해 동시에
 * 수행되지 않는다. 여러 트랜잭션이 들어와도 한
 * 트랜잭션이 수행하고 나머지는 할당연산이 완료되기를
 * 대기한다.
 * 그러므로, Extent 할당 연산과정에서는 다른 트랜잭션에 의해
 * Segment Header의 last bmp 페이지 정보
 * 와 last bmp 페이지의 free slot 개수는 변경되지 않기 때문에
 * 미리 계산해두고 사용한다.
 *
 * 예를 들어, Extent 크기 : 512K 일때
 *
 * A. Segment 생성 크기 512K 로 생성
 *
 * - Extent 1개 할당 -> Segment 512K이므로 PageRange 16( <1M ) 결정
 *
 * - lf-BMPs 구하기
 *   PageRange 16으로 512 Extent를 표현하려면,
 *   512KB를 페이지로 표현하면, 64 페이지 : pages = 512/page_size
 *   lf-BMPs = 64 pages / PageRange_pages = 4 lf-BMPs,
 *
 * - it-BMPs 구하기
 *   생성될 lf-bmps 를 모두 기록할 만큼의 lf-bmp map 이 있는가?
 *   lf-BMP map에서 빈 lf-BMP slot이 있는지?
 *   있으면, 생성할 lf-BMPs에서 기존 lf-BMP에
 *   기록할 수 있는 개수를 뺀다 : lf-BMPs'
 *   it-BMPs = mod( lf-BMPs', lf-BMPs )
 *   나머지 없이 떨어지면 it-BMPs += 1
 *
 * - rt-BMPs 구하기
 *
 *   생성될 it-bmps 를 모두 기록할 만큼의 it-bmp map 이 있는가?
 *   it-BMP map에서 빈 it-BMP slot이 있는지?
 *   있으면, 생성할 it-BMPs에서 기존 it-BMP에
 *   기록할 수 있는 개수를 뺀다 : it-BMPs'
 *   rt-BMPs = mod( it-BMPs', it-BMPs )
 *   나머지 없이 떨어지면 rt-BMPs += 1
 *
 *
 * aStatistics   - [IN] 통계정보
 * aSpaceID      - [IN] SpaceID
 * aSegPID       - [IN] Segment Header PID
 * aMaxExtCnt    - [IN] Segment에서 허용가능한 최대 Extent 수
 * aExtDesc      - [IN] 할당된 Extent의 Desc
 * aBfrInfo      - [OUT] Extent 할당 알고리즘에서 사용하는 이전 할당 정보
 ***********************************************************************/
IDE_RC sdpstAllocExtInfo::getBfrAllocExtInfo( idvSQL               *aStatistics,
                                              scSpaceID             aSpaceID,
                                              scPageID              aSegPID,
                                              UInt                  aMaxExtCnt,
                                              sdpstBfrAllocExtInfo *aBfrInfo )
{
    UChar              * sSegHdrPagePtr;
    UChar              * sPagePtr;
    sdpstSegHdr        * sSegHdr;
    sdpstExtDirHdr     * sExtDirHdr;
    sdpstLfBMPHdr      * sLfBMPHdr;
    sdpstBMPHdr        * sBMPHdr;
    sdpstRangeSlot     * sRangeSlot;
    UInt                 sLoop;

    IDE_DASSERT( aBfrInfo != NULL );

    sSegHdrPagePtr = NULL;
    sPagePtr       = NULL;

    /* 0. Segment Header가 아직 생성되지 않은 경우 */
    if ( aSegPID == SD_NULL_PID )
    {
        aBfrInfo->mMaxSlotCnt[SDPST_RTBMP]   =
            SDPST_MAX_SLOT_CNT_PER_RTBMP_IN_SEGHDR();
        aBfrInfo->mMaxSlotCnt[SDPST_ITBMP]   = SDPST_MAX_SLOT_CNT_PER_ITBMP();
        aBfrInfo->mMaxSlotCnt[SDPST_LFBMP]   = SDPST_MAX_SLOT_CNT_PER_LFBMP();
        aBfrInfo->mMaxSlotCnt[SDPST_EXTDIR]  =
            SDPST_MAX_SLOT_CNT_PER_EXTDIR_IN_SEGHDR();

        aBfrInfo->mFreeSlotCnt[SDPST_RTBMP]  =
            SDPST_MAX_SLOT_CNT_PER_RTBMP_IN_SEGHDR();
        aBfrInfo->mFreeSlotCnt[SDPST_ITBMP]  = 0;
        aBfrInfo->mFreeSlotCnt[SDPST_LFBMP]  = 0;
        aBfrInfo->mFreeSlotCnt[SDPST_EXTDIR] =
            SDPST_MAX_SLOT_CNT_PER_EXTDIR_IN_SEGHDR();

        aBfrInfo->mNxtSeqNo                  = 0;

        IDE_CONT( no_segment_header );
    }

    /* 1. Segment Header가 존재하는 경우
       Segment Header로부터 last lf-bmp 페이지를 구하여 fix 한다. */
    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          &sSegHdrPagePtr ) != IDE_SUCCESS );

    sSegHdr = sdpstSH::getHdrPtr( sSegHdrPagePtr );

    IDE_TEST_RAISE( (sSegHdr->mTotExtCnt + 1) > aMaxExtCnt,
                    error_exceed_segment_maxextents );

    /* 새로운 마지막 bitmap 페이지를 미리 기존 마지막으로 설정해놓고,
     * 이후 변경될 경우에 한해서 수정한다. */
    aBfrInfo->mLstPID[SDPST_LFBMP]  = sSegHdr->mLstLfBMP;
    aBfrInfo->mLstPID[SDPST_ITBMP]  = sSegHdr->mLstItBMP;
    aBfrInfo->mLstPID[SDPST_RTBMP]  = sSegHdr->mLstRtBMP;
    aBfrInfo->mLstPID[SDPST_EXTDIR] = sdpstSH::getLstExtDir( sSegHdr );

    if ( (aBfrInfo->mLstPID[SDPST_LFBMP]  == SD_NULL_PID) ||
         (aBfrInfo->mLstPID[SDPST_ITBMP]  == SD_NULL_PID) ||
         (aBfrInfo->mLstPID[SDPST_RTBMP]  == SD_NULL_PID) ||
         (aBfrInfo->mLstPID[SDPST_EXTDIR] == SD_NULL_PID) )
    {
        sdpstSH::dump( sSegHdrPagePtr );
        IDE_ASSERT( 0 );
    }

    aBfrInfo->mTotPageCnt  = sSegHdr->mTotPageCnt;
    aBfrInfo->mNxtSeqNo    = sSegHdr->mLstSeqNo + 1;

    /* BMP 페이지들을 fix 해서 정보를 가져온다. */
    for ( sLoop = SDPST_RTBMP; sLoop < SDPST_BMP_TYPE_MAX; sLoop++ )
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                              aSpaceID,
                                              aBfrInfo->mLstPID[sLoop],
                                              &sPagePtr ) != IDE_SUCCESS );

        if ( sLoop == SDPST_LFBMP )
        {
            sLfBMPHdr = sdpstLfBMP::getHdrPtr( sPagePtr );
            sBMPHdr   = &sLfBMPHdr->mBMPHdr;
        }
        else
        {
            sBMPHdr   = sdpstBMP::getHdrPtr( sPagePtr );
        }

        aBfrInfo->mFreeSlotCnt[sLoop] = sBMPHdr->mFreeSlotCnt;
        aBfrInfo->mMaxSlotCnt[sLoop]  = sBMPHdr->mMaxSlotCnt;

        if ( sLoop == SDPST_LFBMP )
        {
            aBfrInfo->mPageRange        = sLfBMPHdr->mPageRange;
            aBfrInfo->mFreePageRangeCnt =
                sdpstLfBMP::getFreePageRangeCnt( sLfBMPHdr );

            sRangeSlot = sdpstLfBMP::getRangeSlotBySlotNo(
                                sdpstLfBMP::getMapPtr(sLfBMPHdr),
                                sLfBMPHdr->mBMPHdr.mSlotCnt - 1 );
            aBfrInfo->mLstPBSNo = sRangeSlot->mFstPBSNo +
                                  sRangeSlot->mLength - 1;
        }

        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                  != IDE_SUCCESS );
        sPagePtr = NULL;
    }

    /* 2. 마지막 ExtDir 페이지의 free slot 개수를 구한다. */
    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aBfrInfo->mLstPID[SDPST_EXTDIR],
                                          &sPagePtr ) != IDE_SUCCESS );
    sExtDirHdr = sdpstExtDir::getHdrPtr( sPagePtr );

    aBfrInfo->mFreeSlotCnt[SDPST_EXTDIR] = sdpstExtDir::getFreeSlotCnt(sExtDirHdr);
    aBfrInfo->mMaxSlotCnt[SDPST_EXTDIR]  = SDPST_MAX_SLOT_CNT_PER_EXTDIR();
    aBfrInfo->mSlotNoInExtDir            = sExtDirHdr->mExtCnt;

    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
              != IDE_SUCCESS );
    sPagePtr = NULL;

    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sSegHdrPagePtr )
              != IDE_SUCCESS );
    sSegHdrPagePtr = NULL;

    IDE_EXCEPTION_CONT( no_segment_header );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_exceed_segment_maxextents );
    {
         IDE_SET( ideSetErrorCode( smERR_ABORT_SegmentExceedMaxExtents,
                  aSpaceID,
                  SD_MAKE_FID(aSegPID),
                  SD_MAKE_FPID(aSegPID),
                  sSegHdr->mTotExtCnt,
                  aMaxExtCnt) );
    }
    IDE_EXCEPTION_END;

    if ( sPagePtr != NULL )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sPagePtr ) == IDE_SUCCESS );
    }
    if ( sSegHdrPagePtr != NULL )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sSegHdrPagePtr ) == IDE_SUCCESS );
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment에 Extent 할당시 새로 생성할 Bitmap 페이지의
 *               개수를 계산한다.
 *
 * 새로운 Extent와 Segment 크기를 기준으로 새로 생성해야할 Bitmap
 * 페이지들을 산정한다
 *
 * leaf bmp 페이지를 생성할 때 고려해야할 사항은 다음과 같다.
 *
 * 1. 이전 lf-bmp 페이지의 PageRange 와 현재 선택된 PageRange가 다른 경우
 *
 *  1-1. 이전 lf-bmp 페이지가 이전 PageRange를 만족하지 않았어도
 *       새로운 Extent를 새로운 lf-bmp를 할당하여 관리한다.
 *
 * 2. 이전 lf-bmp페이지의 PageRange와 현재 선택된 PageRange가 동일한 경우
 *
 *  2-1. 이전 lf-bmp 페이지가 이전 PageRange를 모두 사용하지 않은 경우
 *       새로운 Extent의 모든 페이지 개수가 이전 lf-bmp 페이지의
 *       사용되지 않은 PageRangeblocks 개수보다 작거나 같은 경우는
 *       이전 lf-bmp 페이지를 그대로 사용한다.
 *
 *  2-2  이전 lf-bmp 페이지가 이전 PageRange를 모두 사용하지 않은 경우
 *       새로운 Extent의 모든 페이지 개수가 이전 lf-bmp 페이지의
 *       사용되지 않은 PageRangeblocks 개수보다 큰 경우는
 *       새로운 lf-bmp 페이지를 생성한다.
 *
 * aAllocExtInfo - [IN]  Extent 할당시 필요한 정보의 Pointer
 * aExtDesc      - [IN]  Extent Slot의 Pointer
 * aBfrInfo      - [OUT] Extent 할당 알고리즘에서 사용하는 이전 할당 정보
 * aAftInfo      - [OUT] Extent 할당 알고리즘에서 사용하는 이후 할당 정보
 ***********************************************************************/
void sdpstAllocExtInfo::getAftAllocExtInfo( scPageID                 aSegPID,
                                            sdpstExtDesc           * aExtDesc,
                                            sdpstBfrAllocExtInfo   * aBfrInfo,
                                            sdpstAftAllocExtInfo   * aAftInfo )
{
    scPageID    sNewSegPID;

    IDE_ASSERT( aExtDesc != NULL );
    IDE_ASSERT( aBfrInfo != NULL );
    IDE_ASSERT( aAftInfo != NULL );

    aAftInfo->mLstPID[SDPST_RTBMP]  = aBfrInfo->mLstPID[SDPST_RTBMP];
    aAftInfo->mLstPID[SDPST_ITBMP]  = aBfrInfo->mLstPID[SDPST_ITBMP];
    aAftInfo->mLstPID[SDPST_LFBMP]  = aBfrInfo->mLstPID[SDPST_LFBMP];
    aAftInfo->mLstPID[SDPST_EXTDIR] = aBfrInfo->mLstPID[SDPST_EXTDIR];

    aAftInfo->mSlotNoInExtDir       = aBfrInfo->mSlotNoInExtDir;

    if ( aBfrInfo->mFreeSlotCnt[SDPST_EXTDIR] == 0 )
    {
        /* ExtDesc에 ExtDir 페이지를 생성해서 기록해야 하는 경우 */
        aAftInfo->mPageCnt[SDPST_EXTDIR] = 1;
        aAftInfo->mLstPID[SDPST_EXTDIR]  = aExtDesc->mExtFstPID;
        aAftInfo->mSlotNoInExtDir        = 0;
    }

    /* 기존 SegHdr의 총 페이지 개수에 할당할 Extent 개수를 더한다. */
    aAftInfo->mTotPageCnt = aBfrInfo->mTotPageCnt + aExtDesc->mLength;

    /* 현재 Page Range를 선택한다. */
    aAftInfo->mPageRange = selectPageRange( aAftInfo->mTotPageCnt );

    if ( (aBfrInfo->mPageRange == aAftInfo->mPageRange) &&
         (aBfrInfo->mFreeSlotCnt[SDPST_LFBMP] > 0) &&
         (aBfrInfo->mFreePageRangeCnt >= aExtDesc->mLength) )
    {
        /* 위의 2-1 사항에 해당한다. 이전 lf-bmp를 사용하면 되므로
         * 새로 생성할 bmp 페이지들이 없다. */
        aAftInfo->mPageCnt[SDPST_LFBMP] = 0;
        aAftInfo->mPageCnt[SDPST_ITBMP] = 0;
        aAftInfo->mPageCnt[SDPST_RTBMP] = 0;

        if ( aAftInfo->mPageCnt[SDPST_EXTDIR] == 1 )
        {
            /* 새로 생성되는 ExtDir 페이지는 기존 LfBMP 에서 관리된다. */
            aAftInfo->mLfBMP4ExtDir = aAftInfo->mLstPID[SDPST_LFBMP];
        }
    }
    else
    {
        /* 1-1, 2-2에 해당하며, 새로운 lf-bmp를 생성하여야 한다.
         * 그러므로 필요하다면 상위 bmp 페이지들도 생성하여야 한다. */
        aAftInfo->mPageCnt[SDPST_LFBMP] =
            SDPST_EST_BMP_CNT_4NEWEXT( aExtDesc->mLength,
                                       aAftInfo->mPageRange );

        aAftInfo->mFstPID[SDPST_LFBMP] = aExtDesc->mExtFstPID +
                                         aAftInfo->mPageCnt[SDPST_EXTDIR];
        aAftInfo->mLstPID[SDPST_LFBMP] = aAftInfo->mFstPID[SDPST_LFBMP] +
                                         aAftInfo->mPageCnt[SDPST_LFBMP] - 1;

        if ( aAftInfo->mPageCnt[SDPST_LFBMP] >
                aBfrInfo->mFreeSlotCnt[SDPST_ITBMP] )
        {
            aAftInfo->mPageCnt[SDPST_ITBMP] =
                SDPST_EST_BMP_CNT_4NEWEXT( aAftInfo->mPageCnt[SDPST_LFBMP ] -
                                           aBfrInfo->mFreeSlotCnt[SDPST_ITBMP],
                                           aBfrInfo->mMaxSlotCnt[SDPST_ITBMP] );

            aAftInfo->mFstPID[SDPST_ITBMP] = aExtDesc->mExtFstPID +
                                             aAftInfo->mPageCnt[SDPST_EXTDIR] +
                                             aAftInfo->mPageCnt[SDPST_LFBMP];
            aAftInfo->mLstPID[SDPST_ITBMP] = aAftInfo->mFstPID[SDPST_ITBMP] +
                                             aAftInfo->mPageCnt[SDPST_ITBMP] - 1;
        }
        else
        {
            aAftInfo->mPageCnt[SDPST_ITBMP] = 0;
        }

        if ( aAftInfo->mPageCnt[SDPST_ITBMP] >
                aBfrInfo->mFreeSlotCnt[SDPST_RTBMP] )
        {
            // rt-bmps 개수를 구한다.
            // SegHdr 페이지에 저장될 itslots개수를 고려하여 새로 생성할
            // rt-bmp 페이지를 계산한다.
            aAftInfo->mPageCnt[SDPST_RTBMP] =
                SDPST_EST_BMP_CNT_4NEWEXT( aAftInfo->mPageCnt[SDPST_ITBMP] -
                                           aBfrInfo->mFreeSlotCnt[SDPST_RTBMP],
                                           aBfrInfo->mMaxSlotCnt[SDPST_RTBMP] );
            aAftInfo->mFstPID[SDPST_RTBMP] = aExtDesc->mExtFstPID +
                                             aAftInfo->mPageCnt[SDPST_EXTDIR] +
                                             aAftInfo->mPageCnt[SDPST_LFBMP] +
                                             aAftInfo->mPageCnt[SDPST_ITBMP];
            aAftInfo->mLstPID[SDPST_RTBMP] = aAftInfo->mFstPID[SDPST_RTBMP] +
                                             aAftInfo->mPageCnt[SDPST_RTBMP] - 1;
        }
        else
        {
            aAftInfo->mPageCnt[SDPST_RTBMP] = 0;
        }
    }

    if ( aSegPID == SD_NULL_PID )
    {
        /* Segment Header가 아직 생성되지 않은 상태에서는 Extent의
         * ExtDir의 PID와 ExtDesc slotNo를 강제로 설정해야한다. */
        sNewSegPID = aExtDesc->mExtFstPID +
                     (aAftInfo->mPageCnt[SDPST_EXTDIR] +
                      aAftInfo->mPageCnt[SDPST_LFBMP] +
                      aAftInfo->mPageCnt[SDPST_ITBMP] +
                      aAftInfo->mPageCnt[SDPST_RTBMP]);

        aBfrInfo->mLstPID[SDPST_EXTDIR] = sNewSegPID;
        aBfrInfo->mLstPID[SDPST_RTBMP]  = sNewSegPID;

        /* mSegHdrCnt는 0 또는 1 값만 갖고, 이미 SegHdr가 존재하는 경우 항상 0
         * 이어야 한다. */
        aAftInfo->mSegHdrCnt            = 1;
        aAftInfo->mLstPID[SDPST_EXTDIR] = sNewSegPID;
        aAftInfo->mLstPID[SDPST_RTBMP]  = sNewSegPID;
        aAftInfo->mSlotNoInExtDir       = 0;
    }
}

 /***********************************************************************
 * Description : Segment에 Extent 할당시 필요한 정보를 수집한다.
 ************************************************************************/
IDE_RC sdpstAllocExtInfo::getAllocExtInfo( idvSQL                * aStatistics,
                                           scSpaceID               aSpaceID,
                                           scPageID                aSegPID,
                                           UInt                    aMaxExtCnt,
                                           sdpstExtDesc          * aExtDesc,
                                           sdpstBfrAllocExtInfo  * aBfrInfo,
                                           sdpstAftAllocExtInfo  * aAftInfo )
{
    IDE_TEST( getBfrAllocExtInfo( aStatistics,
                                  aSpaceID,
                                  aSegPID,
                                  aMaxExtCnt,
                                  aBfrInfo )
              != IDE_SUCCESS );

    getAftAllocExtInfo( aSegPID, aExtDesc, aBfrInfo, aAftInfo );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment 총 크기를 고려하여 Leaf Bitmap 페이지의
 *               Page Range를 결정한다
 ************************************************************************/
sdpstPageRange sdpstAllocExtInfo::selectPageRange( ULong aTotPages )
{
    sdpstPageRange sPageRange;

    if ( aTotPages < SDPST_PAGE_CNT_1M )
    {
        /* x < 1M */
        sPageRange = SDPST_PAGE_RANGE_16;
    }
    else
    {
        if ( aTotPages >= SDPST_PAGE_CNT_64M )
        {
            if ( aTotPages >= SDPST_PAGE_CNT_1024M )
            {
                /* x >= 1G */
                sPageRange = SDPST_PAGE_RANGE_1024;
            }
            else
            {
                /* 64M <= x < 1G */
                sPageRange = SDPST_PAGE_RANGE_256;
            }
        }
        else
        {   /* 1M <= x < 64M */
            sPageRange = SDPST_PAGE_RANGE_64;
        }
    }

    return sPageRange;
}

void sdpstAllocExtInfo::dump( sdpstBfrAllocExtInfo  * aBfrInfo )
{
    IDE_ASSERT( aBfrInfo != NULL );

    ideLog::log( IDE_SERVER_0,
                 "========================================================\n" );

    ideLog::logMem( IDE_SERVER_0,
                    (UChar*)aBfrInfo,
                    ID_SIZEOF(sdpstBfrAllocExtInfo) );

    ideLog::log( IDE_SERVER_0,
                 "--------------- Before AllocExtInfo Begin ----------------\n"
                 "Last PID [EXTDIR]         :%u\n"
                 "Last PID [RTBMP]          :%u\n"
                 "Last PID [ITBMP]          :%u\n"
                 "Last PID [LFBMP]          :%u\n"
                 "Free Slot Count [EXTDIR]  :%u\n"
                 "Free Slot Count [RTBMP]   :%u\n"
                 "Free Slot Count [ITBMP]   :%u\n"
                 "Free Slot Count [LFBMP]   :%u\n"
                 "Max Slot Count [EXTDIR]   :%u\n"
                 "Max Slot Count [RTBMP]    :%u\n"
                 "Max Slot Count [ITBMP]    :%u\n"
                 "Max Slot Count [LFBMP]    :%u\n"
                 "Total Page Count          :%llu\n"
                 "PageRange                 :%u\n"
                 "Free PageRange Count      :%u\n"
                 "Slot No In ExtDir         :%d\n"
                 "Last PBS No               :%d\n"
                 "Next SeqNo                :%llu\n"
                 "---------------  Before AllocExtInfo End  ----------------\n",
                 aBfrInfo->mLstPID[SDPST_EXTDIR],
                 aBfrInfo->mLstPID[SDPST_RTBMP],
                 aBfrInfo->mLstPID[SDPST_ITBMP],
                 aBfrInfo->mLstPID[SDPST_LFBMP],

                 aBfrInfo->mFreeSlotCnt[SDPST_EXTDIR],
                 aBfrInfo->mFreeSlotCnt[SDPST_RTBMP],
                 aBfrInfo->mFreeSlotCnt[SDPST_ITBMP],
                 aBfrInfo->mFreeSlotCnt[SDPST_LFBMP],

                 aBfrInfo->mMaxSlotCnt[SDPST_EXTDIR],
                 aBfrInfo->mMaxSlotCnt[SDPST_RTBMP],
                 aBfrInfo->mMaxSlotCnt[SDPST_ITBMP],
                 aBfrInfo->mMaxSlotCnt[SDPST_LFBMP],

                 aBfrInfo->mTotPageCnt,

                 aBfrInfo->mPageRange,
                 aBfrInfo->mFreePageRangeCnt,

                 aBfrInfo->mSlotNoInExtDir,
                 aBfrInfo->mLstPBSNo,
                 aBfrInfo->mNxtSeqNo );

    ideLog::log( IDE_SERVER_0,
                 "========================================================\n" );
}

void sdpstAllocExtInfo::dump( sdpstAftAllocExtInfo  * aAftInfo )
{
    IDE_ASSERT( aAftInfo != NULL );

    ideLog::log( IDE_SERVER_0,
                 "========================================================\n" );

    ideLog::logMem( IDE_SERVER_0,
                    (UChar*)aAftInfo,
                    ID_SIZEOF(sdpstAftAllocExtInfo) );

    ideLog::log( IDE_SERVER_0,
                 "--------------- After AllocExtInfo Begin ----------------\n"
                 "First PID [EXTDIR]        :%u\n"
                 "First PID [RTBMP]         :%u\n"
                 "First PID [ITBMP]         :%u\n"
                 "First PID [LFBMP]         :%u\n"
                 "Last PID [EXTDIR]         :%u\n"
                 "Last PID [RTBMP]          :%u\n"
                 "Last PID [ITBMP]          :%u\n"
                 "Last PID [LFBMP]          :%u\n"
                 "Page Count [EXTDIR]       :%u\n"
                 "Page Count [RTBMP]        :%u\n"
                 "Page Count [ITBMP]        :%u\n"
                 "Page Count [LFBMP]        :%u\n"
                 "Full BMP Count [EXTDIR]   :%u\n"
                 "Full BMP Count [RTBMP]    :%u\n"
                 "Full BMP Count [ITBMP]    :%u\n"
                 "Full BMP Count [LFBMP]    :%u\n"
                 "Total Page Count          :%llu\n"
                 "LfBMP for ExtDir          :%u\n"
                 "PageRange                 :%u\n"
                 "Segment Header (0 or 1)   :%u\n"
                 "Slot No In ExtDir         :%d\n"
                 "---------------  After AllocExtInfo End  ----------------\n",
                 aAftInfo->mFstPID[SDPST_EXTDIR],
                 aAftInfo->mFstPID[SDPST_RTBMP],
                 aAftInfo->mFstPID[SDPST_ITBMP],
                 aAftInfo->mFstPID[SDPST_LFBMP],

                 aAftInfo->mLstPID[SDPST_EXTDIR],
                 aAftInfo->mLstPID[SDPST_RTBMP],
                 aAftInfo->mLstPID[SDPST_ITBMP],
                 aAftInfo->mLstPID[SDPST_LFBMP],

                 aAftInfo->mPageCnt[SDPST_EXTDIR],
                 aAftInfo->mPageCnt[SDPST_RTBMP],
                 aAftInfo->mPageCnt[SDPST_ITBMP],
                 aAftInfo->mPageCnt[SDPST_LFBMP],

                 aAftInfo->mFullBMPCnt[SDPST_EXTDIR],
                 aAftInfo->mFullBMPCnt[SDPST_RTBMP],
                 aAftInfo->mFullBMPCnt[SDPST_ITBMP],
                 aAftInfo->mFullBMPCnt[SDPST_LFBMP],

                 aAftInfo->mTotPageCnt,
                 aAftInfo->mLfBMP4ExtDir,
                 aAftInfo->mSegHdrCnt,
                 aAftInfo->mSlotNoInExtDir );

    ideLog::log( IDE_SERVER_0,
                 "========================================================\n" );
}
